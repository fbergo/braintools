/*

   LIBIP - Image Processing Library
   (C) 2002-2013
   
   Felipe P.G. Bergo <fbergo at gmail dot com>
   Alexandre X. Falcao <afalcao at ic dot unicamp dot br>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/**************************************************************
 $Id: visualization.c,v 1.21 2013/11/11 22:34:13 bergo Exp $

 visualization.c - scene projection and shading

***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libip.h"

/* private call declarations */
void  ProjectVolume(Scene *scene, TaskLog *tlog);
void  ProjectSeeThru(Scene *scene, TaskLog *tlog);
void  ProjectPaths(Scene *scene, TaskLog *tlog);
void  RecalcDirectProjection(Scene *scene, int *B);
void  RecalcDirectProjectionWithCostThreshold(Scene *scene, int *B);
void  RecalcDirectProjectionWithRestriction(Scene *scene, int *B, BMap *mask);
void  RecalcFacesProjection(Scene *scene, int *B);
void  SplatProjection2x2(Scene *scene, int *B);
void  ProjectionCore(Scene *scene, int i, int j, int k, Transform *a, int *B);
void  CastProjection(Scene *scene, int *B);
void  RenderPaths(Scene *scene);
void  MIPOverlay(Scene *scene);

void  SmoothSurface(Scene *scene);
void  CalcSurface(Scene *s);
int   CalcShade(Scene *scene,int c,int x,int y);
void  CalcZLimit(Scene *scene);
void  CalcEShade(Scene *s);

#define PhongDiffuse(a) (a)
float PhongSpecular(float angcos, int n);

typedef float (*VVFunc) (Scene *,int,int);

float Estimate8(Scene *s,int x,int y);
float Estimate16(Scene *s,int x,int y);
float EstimateA(Scene *s,int x,int y);
float EstimateL(Scene *s,int x,int y);
float EstimateImgGrad(Scene *s,int x,int y);

void FindMesh(Scene *scene);

void MakeCubeEdges(Point *p, Point *q, char *vis, Transform *T,
		   float x1, float y1, float z1,
		   float x2, float y2, float z2);

/*

                       RenderScene (1)
                            |
                            |
            +---------------+---------------+---------------------------+
            |                               |                           |
      ProjectSeeThru (2)              ProjectVolume (3)        ProjectPaths (9)
            |                               |
          (...)                   +---------+--------------+
                                  |                        |
                      RecalcFacesProjection(4)  RecalcDirectProjection(5)
                                  |                        |
                                  |                        |
                                  +------------+-----------+
                                               |
                                     SplatProjection2x2 (6)
                                               |
                                       CastProjection (7)
                                               |
                                          FindMesh (8)
                                               |
                                         MIPOverlay (10)
                                               |  
                                              ///


       vmode_seethru: (1)-(2), (2) calls (5)-(6)-(7) once for each object,
                               combines the outputs and then calls (8).

       vmode_src:     (1)-(3)-(4)-(6)-(7)-(8)-(10)

       vmode_paths:   (1)-(9)

       other modes:   (1)-(3)-(5)-(6)-(7)-(8)-(10)

*/

void RenderScene(Scene *scene, int width, int height, TaskLog *tlog) 
{
  /* ensure that vinfo, xyzbuffer and vout exist in the
     correct size, and that scene->invalid is correct */

  if (scene->pvt.vinfo != 0) {
    if (scene->pvt.vinfo->W != width || scene->pvt.vinfo->H != height) {
      VolumeDestroy(scene->pvt.vinfo);
      scene->pvt.vinfo = 0;
      VolumeDestroy(scene->pvt.xyzbuffer);
      scene->pvt.xyzbuffer = 0;
      VolumeDestroy(scene->pvt.surface);
      scene->pvt.surface = 0;
    }
  }
  if (scene->pvt.vinfo == 0) {
    scene->pvt.vinfo = VolumeNew(width, height, 3, vt_integer_32);
    scene->invalid = 1;
  }
  if (scene->pvt.xyzbuffer == 0) {
    scene->pvt.xyzbuffer = VolumeNew(width, height, 3, vt_float_32);
    scene->invalid = 1;
  }
  if (scene->pvt.surface == 0) {
    scene->pvt.surface = VolumeNew(width, height, 2, vt_float_32);
    scene->invalid = 1;
  }
  ScenePrepareImage(scene, width, height);
  CalcEShade(scene);

  if (!scene->invalid)
    SceneCheckValidity(scene);

  switch(scene->options.vmode) {
  case vmode_seethru: ProjectSeeThru(scene, tlog); break;
  case vmode_paths:   ProjectPaths(scene, tlog); break;
  default:            ProjectVolume(scene, tlog); break;
  }

  SceneValidate(scene);
}

void ProjectPaths(Scene *scene, TaskLog *tlog) {
  FullTimer t1;
  char z[128];

  if (scene->invalid) {
    StartTimer(&t1);
    scene->amesh.validmesh = 0; /* no mesh in paths mode */

    RenderPaths(scene);

    if (tlog) {
      sprintf(z,"Scene Projection (V:%d,S:%d)", 
	      (int)(scene->options.vmode),
	      (int)(scene->options.shading));
      TaskLogAdd(tlog, z, t1.elapsed, t1.S0, t1.S1);
    }

    StopTimer(&t1);
  }
}

void ProjectVolume(Scene *scene, TaskLog *tlog) {
  FullTimer t1;
  char z[128];
  int  B[4];

  if (scene->invalid) {
    StartTimer(&t1);

    if (scene->options.vmode == vmode_src)
      RecalcFacesProjection(scene,B);
    else
      RecalcDirectProjection(scene,B);
    CastProjection(scene,B);

    if (scene->options.vmode == vmode_src || !scene->wantmesh) {
      scene->amesh.validmesh = 0;
    } else {
      FindMesh(scene);
    }

    if (scene->enable_mip != 0)
      MIPOverlay(scene);

    StopTimer(&t1);

    if (tlog) {
      sprintf(z,"Scene Projection (V:%d,S:%d)", 
	      (int)(scene->options.vmode),
	      (int)(scene->options.shading));
      TaskLogAdd(tlog, z, t1.elapsed, t1.S0, t1.S1);
    }
  }
}

void ProjectSeeThru(Scene *scene, TaskLog *tlog) {
  Scene *dummy;
  int width, height;
  XAnnVolume *avol = scene->avol;
  FullTimer t1;
  char z[128];
  int B[4];

  int i,j,k,l, r,s, aR,aG,aB, Top;
  CImg *objects[10];
  int wantobj[10];
  
  float fa,fb;

  if (scene->invalid) {
    StartTimer(&t1);

    width  = scene->pvt.vinfo->W;
    height = scene->pvt.vinfo->H;

    VolumeFill(scene->pvt.vinfo, VLARGE);
    VolumeFillF(scene->pvt.xyzbuffer, VLARGE_D);

    dummy = SceneNew();
    SceneCopy(dummy, scene);

    dummy->pvt.vinfo     = VolumeNew(width, height, 3, vt_integer_32);
    dummy->pvt.xyzbuffer = VolumeNew(width, height, 3, vt_float_32);
    dummy->pvt.surface   = VolumeNew(width, height, 2, vt_float_32);
    dummy->vout = CImgNew(width, height);

    for(i=0;i<10;i++) {
      wantobj[i] = 0;
      objects[i] = 0;
    }

    for(i=0;i<avol->N;i++)
      wantobj[ avol->vd[i].label & LABELMASK ] = 1;

    for(i=0;i<10;i++) 
      if (scene->hollow[i])
	wantobj[i] = 0;

    dummy->options.vmode = vmode_srclabel;
    
    for(i=0;i<10;i++) {

      if (wantobj[i]) {
	MemSet(dummy->hollow, 1, 256 * sizeof(char));
	dummy->hollow[i] = 0;
	dummy->invalid = 1;
	CImgFill(dummy->vout, dummy->colors.bg);
	RecalcDirectProjection(dummy,B);
	CastProjection(dummy,B);

	objects[i] = CImgNew(dummy->vout->W, dummy->vout->H);
	CImgFullCopy(objects[i], dummy->vout);

	/* build the global vinfo / xyzbuffer */
	for(k=0;k<height;k++)
	  for(j=0;j<width;j++) {
	    r = VolumeGetI32(dummy->pvt.vinfo, j,k, NPROJ);
	    if (r!=VLARGE) {
	      fa = VolumeGetF32(dummy->pvt.xyzbuffer, j,k,2);
	      fb = VolumeGetF32(scene->pvt.xyzbuffer, j,k,2);
	      
	      if (fb == VLARGE_D || fa < fb) {
		VolumeSetI32(scene->pvt.vinfo, j,k,ZBUFFER,
			     VolumeGetI32(dummy->pvt.vinfo, j,k, ZBUFFER));
		VolumeSetI32(scene->pvt.vinfo, j,k,NPROJ,
			     VolumeGetI32(dummy->pvt.vinfo, j,k, NPROJ));
		VolumeSetI32(scene->pvt.vinfo, j,k,SEED,
			     VolumeGetI32(dummy->pvt.vinfo, j,k, SEED));
		for(l=0;l<3;l++)
		  VolumeSetF32(scene->pvt.xyzbuffer, j,k,l,
			       VolumeGetF32(dummy->pvt.xyzbuffer, j,k,l));
	      }
	    }
	  }	
      }
    } // loop that projects up to 10 objects
    
    for(k=0;k<height;k++)
      for(j=0;j<width;j++) {
	r=0;
	aR=aG=aB=0;
	Top = 0;
	for(i=0;i<10;i++) {
	  if (wantobj[i]) {
	    s = CImgGet(objects[i], j, k);
	    if (s != dummy->colors.bg) {
	      aR += t0(s);
	      aG += t1(s);
	      aB += t2(s);
	      ++r;

	      l = VolumeGetI32(scene->pvt.vinfo,
			       SceneInvX(scene,j),
			       SceneInvY(scene,k),NPROJ);
	      if (l!=VLARGE)
		if ( (avol->vd[l].label & LABELMASK) == i ) Top = s;
	    }	      
	  }
	} // for i
	if (r) {
	  s = triplet(aR/r, aG/r, aB/r);
	  s = mergeColorsRGB(s, Top, 0.20);
	  CImgSet(scene->vout, j, k, s);
	}

      } // for k / j

    CImgDestroy(dummy->vout);
    SceneDestroy(dummy);
    for(i=0;i<10;i++)
      if (wantobj[i])
	CImgDestroy(objects[i]);

    if (!scene->wantmesh) {
      scene->amesh.validmesh = 0;
    } else {
      FindMesh(scene);
    }

    StopTimer(&t1);
    if (tlog) {
      sprintf(z,"Scene Projection (V:%d,S:%d)", 
	      (int)(scene->options.vmode),
	      (int)(scene->options.shading));
      TaskLogAdd(tlog, z, t1.elapsed, t1.S0, t1.S1);
    }

  } // scene->invalid
}

void CalcZLimit(Scene *scene) {
  Vector v;
  float  diag;
  

  v.x = ((float)(scene->avol->W)) / 2.0;
  v.y = ((float)(scene->avol->H)) / 2.0;
  v.z = ((float)(scene->avol->D)) / 2.0;

  diag = ceil(VectorLength(&v));
  
  scene->pvt.minz = (int) (-diag);
  scene->pvt.maxz = (int) (diag);
}

void  RecalcDirectProjectionWithRestriction(Scene *scene, int *B, BMap *mask) {
  XAnnVolume *av    = scene->avol;
  Volume *xyzbuffer = scene->pvt.xyzbuffer;
  Volume *vinfo     = scene->pvt.vinfo;
  Transform Ta,Tb,Tc;
  Point p,q,r,s;
  int i,j,k,N,c, px,py;

  int z0,zf,zi, y0,yf,yi, x0, xf, xi;
      
  VolumeFill(vinfo, VLARGE);
  VolumeFillF(xyzbuffer, VLARGE_D);

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  CalcZLimit(scene);

  //  scene->pvt.minz =  1000;
  //  scene->pvt.maxz = -1000;

  B[0]=scene->pvt.outw;
  B[2]=scene->pvt.outh;
  B[1] = B[3] = 0;

  // find front-to-back directions

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = scene->clip.v[2][0]; q.z = scene->clip.v[2][1];
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    z0 = scene->clip.v[2][0]; 
    zf = scene->clip.v[2][1] + 1;
    zi = 1;
  } else {
    z0 = scene->clip.v[2][1]; 
    zf = scene->clip.v[2][0] - 1;
    zi = -1;
  }

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = scene->clip.v[1][0]; q.y = scene->clip.v[1][1];
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    y0 = scene->clip.v[1][0]; 
    yf = scene->clip.v[1][1] + 1;
    yi = 1;
  } else {
    y0 = scene->clip.v[1][1]; 
    yf = scene->clip.v[1][0] - 1;
    yi = -1;
  }

  p.x = scene->clip.v[0][0]; q.x = scene->clip.v[0][1];
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    x0 = scene->clip.v[0][0]; 
    xf = scene->clip.v[0][1] + 1;
    xi = 1;
  } else {
    x0 = scene->clip.v[0][1]; 
    xf = scene->clip.v[0][0] - 1;
    xi = -1;
  }

  for(k=z0;k!=zf;k+=zi)
    for(j=y0;j!=yf;j+=yi)
      for(i=x0;i!=xf;i+=xi) {

	N=i + av->tbrow[j] + av->tbframe[k];

	if  (scene->options.vmode != vmode_src &&
	     scene->hollow[av->vd[N].label & LABELMASK])
	  continue;

	if (!_fast_BMapGet(mask, N)) continue;

	p.x = i - (av->W >> 1);
	p.y = j - (av->H >> 1);
	p.z = k - (av->D >> 1);

	PointTransform(&q,&p,&Ta);

	px = scene->pvt.oxc + q.x;
	py = scene->pvt.oyc + q.y;

	if (px < 0 || px >= scene->pvt.outw) continue; 
	if (py < 0 || py >= scene->pvt.outh) continue; 

	if (q.z < VolumeGetF32(xyzbuffer, px, py, 2)) {
          if (px < B[0]) B[0] = px; else
	    if (px > B[1]) B[1] = px;
          if (py < B[2]) B[2] = py; else
	    if (py > B[3]) B[3] = py;

	  // HERE
	  // 0 = zbuffer (inteiro)
	  // 1 = N do voxel projetado
	  // 2 = semente, se houver, -1 se nao
	      
          if (q.z > scene->pvt.maxz) scene->pvt.maxz=q.z;
          if (q.z < scene->pvt.minz) scene->pvt.minz=q.z;

	  VolumeSetI32(vinfo, px, py, ZBUFFER, q.z);
	  VolumeSetI32(vinfo, px, py, NPROJ, N);
	  
	  c=MapOf(av->addseeds, N); /* -1 if not a seed */
	  VolumeSetI32(vinfo, px, py, SEED, c);
	  
	  // store floating-point coordinates for smooth normals
	  VolumeSetF32(xyzbuffer, px, py, 0, q.x);
	  VolumeSetF32(xyzbuffer, px, py, 1, q.y);
	  VolumeSetF32(xyzbuffer, px, py, 2, q.z);

	} // if q.z < ...
      } // for i-j-k

  SplatProjection2x2(scene,B);
}

void RecalcDirectProjectionWithCostThreshold(Scene *scene, int *B) {
  XAnnVolume *av    = scene->avol;
  Volume *xyzbuffer = scene->pvt.xyzbuffer;
  Volume *vinfo     = scene->pvt.vinfo;
  Transform Ta,Tb,Tc;
  Point p,q,r,s;
  int i,j,k,N,c, px,py;
  int cmin, cmax;

  int z0,zf,zi, y0,yf,yi, x0, xf, xi;
      
  VolumeFill(vinfo, VLARGE);
  VolumeFillF(xyzbuffer, VLARGE_D);

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  CalcZLimit(scene);

  //  scene->pvt.minz =  1000;
  //  scene->pvt.maxz = -1000;

  B[0]=scene->pvt.outw;
  B[2]=scene->pvt.outh;
  B[1] = B[3] = 0;

  // find front-to-back directions

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = scene->clip.v[2][0]; q.z = scene->clip.v[2][1];
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    z0 = scene->clip.v[2][0]; 
    zf = scene->clip.v[2][1] + 1;
    zi = 1;
  } else {
    z0 = scene->clip.v[2][1]; 
    zf = scene->clip.v[2][0] - 1;
    zi = -1;
  }

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = scene->clip.v[1][0]; q.y = scene->clip.v[1][1];
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    y0 = scene->clip.v[1][0]; 
    yf = scene->clip.v[1][1] + 1;
    yi = 1;
  } else {
    y0 = scene->clip.v[1][1]; 
    yf = scene->clip.v[1][0] - 1;
    yi = -1;
  }

  p.x = scene->clip.v[0][0]; q.x = scene->clip.v[0][1];
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    x0 = scene->clip.v[0][0]; 
    xf = scene->clip.v[0][1] + 1;
    xi = 1;
  } else {
    x0 = scene->clip.v[0][1]; 
    xf = scene->clip.v[0][0] - 1;
    xi = -1;
  }

  cmin = scene->cost_threshold_min;
  cmax = scene->cost_threshold_max;

  for(k=z0;k!=zf;k+=zi)
    for(j=y0;j!=yf;j+=yi)
      for(i=x0;i!=xf;i+=xi) {

	N=i + av->tbrow[j] + av->tbframe[k];

	if  (scene->options.vmode != vmode_src &&
	     scene->hollow[av->vd[N].label & LABELMASK])
	  continue;

	if (av->vd[N].cost < cmin || av->vd[N].cost > cmax)
	  continue;

	p.x = i - (av->W >> 1);
	p.y = j - (av->H >> 1);
	p.z = k - (av->D >> 1);

	PointTransform(&q,&p,&Ta);

	px = scene->pvt.oxc + q.x;
	py = scene->pvt.oyc + q.y;

	if (px < 0 || px >= scene->pvt.outw) continue; 
	if (py < 0 || py >= scene->pvt.outh) continue; 

	if (q.z < VolumeGetF32(xyzbuffer, px, py, 2)) {
          if (px < B[0]) B[0] = px; else
	    if (px > B[1]) B[1] = px;
          if (py < B[2]) B[2] = py; else
	    if (py > B[3]) B[3] = py;

	  // HERE
	  // 0 = zbuffer (inteiro)
	  // 1 = N do voxel projetado
	  // 2 = semente, se houver, -1 se nao
	      
          if (q.z > scene->pvt.maxz) scene->pvt.maxz=q.z;
          if (q.z < scene->pvt.minz) scene->pvt.minz=q.z;

	  VolumeSetI32(vinfo, px, py, ZBUFFER, q.z);
	  VolumeSetI32(vinfo, px, py, NPROJ, N);
	  
	  c=MapOf(av->addseeds, N); /* -1 if not a seed */
	  VolumeSetI32(vinfo, px, py, SEED, c);
	  
	  // store floating-point coordinates for smooth normals
	  VolumeSetF32(xyzbuffer, px, py, 0, q.x);
	  VolumeSetF32(xyzbuffer, px, py, 1, q.y);
	  VolumeSetF32(xyzbuffer, px, py, 2, q.z);

	} // if q.z < ...
      } // for i-j-k

  SplatProjection2x2(scene,B);
}

void RecalcDirectProjection(Scene *scene, int *B) {

  XAnnVolume *av    = scene->avol;
  Volume *xyzbuffer = scene->pvt.xyzbuffer;
  Volume *vinfo     = scene->pvt.vinfo;
  Transform Ta,Tb,Tc;
  Point p,q,r,s;
  int i,j,k,N,c, px,py;

  int z0,zf,zi, y0,yf,yi, x0, xf, xi;

  if (scene->with_restriction == 1)
    if (scene->cost_threshold_min != 0 || scene->cost_threshold_max != 32767) {
      RecalcDirectProjectionWithCostThreshold(scene, B);
      return;
    }
  if (scene->with_restriction == 2) {
    RecalcDirectProjectionWithRestriction(scene, B, scene->restriction);
    return;
  }
      
  VolumeFill(vinfo, VLARGE);
  VolumeFillF(xyzbuffer, VLARGE_D);

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  CalcZLimit(scene);

  //  scene->pvt.minz =  1000;
  //  scene->pvt.maxz = -1000;

  B[0]=scene->pvt.outw;
  B[2]=scene->pvt.outh;
  B[1] = B[3] = 0;

  // find front-to-back directions

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = scene->clip.v[2][0]; q.z = scene->clip.v[2][1];
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    z0 = scene->clip.v[2][0]; 
    zf = scene->clip.v[2][1] + 1;
    zi = 1;
  } else {
    z0 = scene->clip.v[2][1]; 
    zf = scene->clip.v[2][0] - 1;
    zi = -1;
  }

  p.x = av->W / 2.0;         q.x = av->W / 2.0;
  p.y = scene->clip.v[1][0]; q.y = scene->clip.v[1][1];
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    y0 = scene->clip.v[1][0]; 
    yf = scene->clip.v[1][1] + 1;
    yi = 1;
  } else {
    y0 = scene->clip.v[1][1]; 
    yf = scene->clip.v[1][0] - 1;
    yi = -1;
  }

  p.x = scene->clip.v[0][0]; q.x = scene->clip.v[0][1];
  p.y = av->H / 2.0;         q.y = av->H / 2.0;
  p.z = av->D / 2.0;         q.z = av->D / 2.0;
  PointTransform(&r,&p,&Ta); PointTransform(&s,&q,&Ta);

  if (r.z < s.z) { 
    x0 = scene->clip.v[0][0]; 
    xf = scene->clip.v[0][1] + 1;
    xi = 1;
  } else {
    x0 = scene->clip.v[0][1]; 
    xf = scene->clip.v[0][0] - 1;
    xi = -1;
  }

  for(k=z0;k!=zf;k+=zi)
    for(j=y0;j!=yf;j+=yi)
      for(i=x0;i!=xf;i+=xi) {

	N=i + av->tbrow[j] + av->tbframe[k];

	if  (scene->options.vmode != vmode_src &&
	     scene->hollow[av->vd[N].label & LABELMASK])
	  continue;

	p.x = i - (av->W >> 1);
	p.y = j - (av->H >> 1);
	p.z = k - (av->D >> 1);

	PointTransform(&q,&p,&Ta);

	px = scene->pvt.oxc + q.x;
	py = scene->pvt.oyc + q.y;

	if (px < 0 || px >= scene->pvt.outw) continue; 
	if (py < 0 || py >= scene->pvt.outh) continue; 

	if (q.z < VolumeGetF32(xyzbuffer, px, py, 2)) {
          if (px < B[0]) B[0] = px; else
	    if (px > B[1]) B[1] = px;
          if (py < B[2]) B[2] = py; else
	    if (py > B[3]) B[3] = py;

	  // HERE
	  // 0 = zbuffer (inteiro)
	  // 1 = N do voxel projetado
	  // 2 = semente, se houver, -1 se nao
	      
          if (q.z > scene->pvt.maxz) scene->pvt.maxz=q.z;
          if (q.z < scene->pvt.minz) scene->pvt.minz=q.z;

	  VolumeSetI32(vinfo, px, py, ZBUFFER, q.z);
	  VolumeSetI32(vinfo, px, py, NPROJ, N);
	  
	  c=MapOf(av->addseeds, N); /* -1 if not a seed */
	  VolumeSetI32(vinfo, px, py, SEED, c);
	  
	  // store floating-point coordinates for smooth normals
	  VolumeSetF32(xyzbuffer, px, py, 0, q.x);
	  VolumeSetF32(xyzbuffer, px, py, 1, q.y);
	  VolumeSetF32(xyzbuffer, px, py, 2, q.z);

	} // if q.z < ...
      } // for i-j-k

  SplatProjection2x2(scene,B);
  //  printf("resulting Z range = [%d, %d]\n", scene->pvt.minz, scene->pvt.maxz);
}

void SplatProjection2x2(Scene *scene, int *B) {
  static int splatx[3] = {1,0,1};
  static int splaty[3] = {0,1,1};

  int i,j,k,px,py;
  f32_t za, zb;
  Volume *vinfo     = scene->pvt.vinfo;
  Volume *xyzbuffer = scene->pvt.xyzbuffer;

  for(i=B[1]; i>=B[0] ; i--)
    for(j=B[3]; j>=B[2] ; j--) {
      
      if (VolumeGetI32(vinfo, i, j, NPROJ) == VLARGE)
	continue;

      za = VolumeGetF32(xyzbuffer, i, j, 2);

      for(k=0;k<3;k++) {
	px = i+splatx[k]; py = j+splaty[k];
	if (px >= scene->pvt.outw || py >= scene->pvt.outh) 
	  continue;

	zb = VolumeGetF32(xyzbuffer, px, py, 2);
	
	if (za < zb) {

	  VolumeSetI32(vinfo,px,py,ZBUFFER,
		       VolumeGetI32(vinfo,i,j,ZBUFFER));
	  VolumeSetI32(vinfo,px,py,NPROJ,
		       VolumeGetI32(vinfo,i,j,NPROJ));
	  VolumeSetI32(vinfo,px,py,SEED,
		       VolumeGetI32(vinfo,i,j,SEED));

	  VolumeSetF32(xyzbuffer,px,py,0,VolumeGetF32(xyzbuffer,i,j,0));
	  VolumeSetF32(xyzbuffer,px,py,1,VolumeGetF32(xyzbuffer,i,j,1));
	  VolumeSetF32(xyzbuffer,px,py,2,za);
	}
      } // for k
    } // for j-i
}

void RecalcFacesProjection(Scene *scene, int *B)
{
  Transform Ta,Tb,Tc;
  unsigned int i,j,k;

  VolumeFill(scene->pvt.vinfo, VLARGE);
  VolumeFillF(scene->pvt.xyzbuffer, VLARGE_D);
  
  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  CalcZLimit(scene);

  // bx0 bx1 by0 by1
  B[0]=scene->pvt.outw;
  B[2]=scene->pvt.outh;
  B[1] = B[3] = 0;  

  k=scene->clip.v[2][0];
  for(j=scene->clip.v[1][0];j<= scene->clip.v[1][1]; j++)
    for(i=scene->clip.v[0][0]; i<= scene->clip.v[0][1]; i++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  k=scene->clip.v[2][1];
  for(j=scene->clip.v[1][0];j<= scene->clip.v[1][1]; j++)
    for(i=scene->clip.v[0][0]; i<= scene->clip.v[0][1]; i++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  j=scene->clip.v[1][0];
  for(k=scene->clip.v[2][0];k<= scene->clip.v[2][1]; k++)
    for(i=scene->clip.v[0][0]; i<= scene->clip.v[0][1]; i++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  j=scene->clip.v[1][1];
  for(k=scene->clip.v[2][0];k<= scene->clip.v[2][1]; k++)
    for(i=scene->clip.v[0][0]; i<= scene->clip.v[0][1]; i++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  i=scene->clip.v[0][0];
  for(k=scene->clip.v[2][0];k<= scene->clip.v[2][1]; k++)
    for(j=scene->clip.v[1][0];j<= scene->clip.v[1][1]; j++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  i=scene->clip.v[0][1];
  for(k=scene->clip.v[2][0];k<= scene->clip.v[2][1]; k++)
    for(j=scene->clip.v[1][0];j<= scene->clip.v[1][1]; j++)
      ProjectionCore(scene,i,j,k,&Ta,B);

  SplatProjection2x2(scene,B);
}

void ProjectionCore(Scene *scene, int i, int j, int k, Transform *a, int *B) 
{
  XAnnVolume *av        = scene->avol;
  Volume     *vinfo     = scene->pvt.vinfo;
  Volume     *xyzbuffer = scene->pvt.xyzbuffer;
  Point p,q;
  int c, N, px,py;
	
  N=i + av->tbrow[j] + av->tbframe[k];

  if (scene->options.vmode != vmode_src &&
      scene->hollow[av->vd[N].label & LABELMASK])
    return;
  
  p.x = i - (av->W >> 1);
  p.y = j - (av->H >> 1);
  p.z = k - (av->D >> 1);
  
  PointTransform(&q,&p,a);
  
  px = scene->pvt.oxc + q.x;
  py = scene->pvt.oyc + q.y;
  
  if (px < 0 || px >= scene->pvt.outw) return;
  if (py < 0 || py >= scene->pvt.outh) return; 
  
  if (q.z < VolumeGetF32(xyzbuffer, px, py, 2)) {
    if (px < B[0]) B[0] = px;
    if (px > B[1]) B[1] = px;
    if (py < B[2]) B[2] = py;
    if (py > B[3]) B[3] = py;
    
    // 0 = zbuffer
    // 1 = N do voxel projetado
    // 2 = semente, se houver, -1 se nao
    
    VolumeSetI32(vinfo, px, py, ZBUFFER, q.z);
    VolumeSetI32(vinfo, px, py, NPROJ, N);

    c=MapOf(av->addseeds, N); /* -1 if not a seed */
    VolumeSetI32(vinfo, px, py, SEED, c);
    
    // store floating-point coordinates for smooth normals
    VolumeSetF32(xyzbuffer, px, py, 0, q.x);
    VolumeSetF32(xyzbuffer, px, py, 1, q.y);
    VolumeSetF32(xyzbuffer, px, py, 2, q.z);
    
  } // if q.z < ...
}

void CastProjection(Scene *scene, int *B)
{
  unsigned int i,j;
  int c,d,m,n,N,rc,zp, p,q;
  int maxval=1;
  int W,H;
  XAnnVolume *av    = scene->avol;
  CImg *dest        = scene->vout;
  Volume *vinfo     = scene->pvt.vinfo;
  BMap *isborder;
  int *rootmap;
  VModeType emode;
  ViewMap *vmap = scene->vmap;

  static int tx[8] = { -1, 0,  1,  1, 1, 0, -1 , -1};
  static int ty[8] = { -1, -1, -1, 0, 1, 1,  1 , 0};
  
  zp = ScenePixel(scene);

  W = scene->pvt.outw;
  H = scene->pvt.outh;

  CImgFillRect(dest,
	       SceneX(scene,B[0]),
	       SceneY(scene,B[2]),
	       SceneW(scene,B[1]-B[0]+1),
	       SceneH(scene,B[3]-B[2]+1),
	       scene->colors.bg);

  CalcSurface(scene);

  emode = scene->options.vmode;
  if (scene->options.srcmix < 0.05 && emode == vmode_srclabel)
    emode = vmode_label;

  switch(emode) {

  case vmode_src:

    switch(scene->options.src) {
    case src_orig:  maxval = XAGetOrigMax(av);   break;
    case src_value: maxval = XAGetValueMax(av);  break;
    case src_cost:  maxval = XAGetCostMax(av);   break;
    }

    if (vmap->maxval != maxval) {
      vmap->maxval = maxval;
      ViewMapUpdate(vmap);
    }

    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {
	
	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;
	
	d=VolumeGetI32(vinfo,i,j,SEED);
	if (d<0) {
	  switch(scene->options.src) {
	  case src_orig:  rc = ViewMapColor(vmap,av->vd[N].orig);  break;
	  case src_value: rc = ViewMapColor(vmap,av->vd[N].value); break;
	  case src_cost:  rc = ViewMapColor(vmap,av->vd[N].cost);  break;
	  default: rc=0;
	  }
	} else
	  rc=scene->colors.label[d];

	if (scene->options.shading > 0) rc=CalcShade(scene,rc,i,j);

	CImgFillRect(dest,SceneX(scene,i),SceneY(scene,j),zp,zp,rc);
      }       
    break;
    
  case vmode_label:
    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {

	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;

	d=VolumeGetI32(vinfo,i,j,SEED);
	if (d<0) {
	  rc=scene->colors.label[(av->vd[N].label&LABELMASK) % 10];
	  if (av->vd[N].label & REMOVEMASK)
	    rc=inverseColor(rc);
	} else
	  rc=scene->colors.label[d];

	if (scene->options.shading > 0) rc=CalcShade(scene,rc,i,j);

	CImgFillRect(dest,SceneX(scene,i),SceneY(scene,j),zp,zp,rc);
      }
    
    break;

  case vmode_srclabel:
  case vmode_seethru:

    switch(scene->options.src) {
    case src_orig:  maxval = XAGetOrigMax(av);  break;
    case src_value: maxval = XAGetValueMax(av); break;
    case src_cost:  maxval = XAGetCostMax(av);  break;
    }

    if (vmap->maxval != maxval) {
      vmap->maxval = maxval;
      ViewMapUpdate(vmap);
    }

    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {

	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;

	d=VolumeGetI32(vinfo,i,j,SEED);
	if (d<0) {
	  n=scene->colors.label[(av->vd[N].label&LABELMASK) % 10];
	  if (av->vd[N].label & REMOVEMASK)
	    n=inverseColor(n);

	  switch(scene->options.src) {
	  case src_orig:  m = ViewMapColor(vmap,av->vd[N].orig);  break;
	  case src_value: m = ViewMapColor(vmap,av->vd[N].value); break;
	  case src_cost:  m = ViewMapColor(vmap,av->vd[N].cost);  break;
	  default: m=0;
	  }

	  rc=mergeColorsRGB(m,n,1.0 - scene->options.srcmix);
	} else
	  rc=scene->colors.label[d];

	if (scene->options.shading > 0) rc=CalcShade(scene,rc,i,j);

	CImgFillRect(dest,SceneX(scene,i),SceneY(scene,j),zp,zp,rc);
      }       
    
    break;

  case vmode_srcborder:

    isborder = BMapNew(W*H);
    BMapFill(isborder, 0);

    switch(scene->options.src) {
    case src_orig:  maxval = XAGetOrigMax(av);  break;
    case src_value: maxval = XAGetValueMax(av); break;
    case src_cost:  maxval = XAGetCostMax(av);  break;
    }

    if (vmap->maxval != maxval) {
      vmap->maxval = maxval;
      ViewMapUpdate(vmap);
    }

    for(i=B[0]+1;i<B[1];i++)
      for(j=B[2]+1;j<B[3];j++) {
	p = VolumeGetI32(vinfo,i,j,NPROJ);
	if (p==VLARGE) continue;
	p = av->vd[p].label & LABELMASK;	
	for(c=0;c<8;c++) {
	  q = VolumeGetI32(vinfo,i+tx[c],j+ty[c],NPROJ);
	  if (q==VLARGE) { 
	    BMapSet(isborder, i+ j*W, 1);
	    break;
	  }
	  q = av->vd[q].label & LABELMASK;
	  if (q!=p) { 
	    BMapSet(isborder, i+ j*W, 1);
	    break;
	  }
	}
      }

    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {

	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;

	d=VolumeGetI32(vinfo,i,j,SEED);
	if (d<0) {

	  switch(scene->options.src) {
	  case src_orig:  m = ViewMapColor(vmap,av->vd[N].orig);  break;
	  case src_value: m = ViewMapColor(vmap,av->vd[N].value); break;
	  case src_cost:  m = ViewMapColor(vmap,av->vd[N].cost);  break;
	  default: m=0;
	  }

	  if (BMapGet(isborder,i+j*W)) {
	    n=scene->colors.label[(av->vd[N].label&LABELMASK) % 10];
	    if (av->vd[N].label & REMOVEMASK)
	      n=inverseColor(n);
	    rc=mergeColorsRGB(m,n,0.66);
	    if (scene->colors.whiteborder)
	      rc=0xffffff;
	  } else {
	    rc = m;
	  }
	} else
	  rc=scene->colors.label[d];

	if (scene->options.shading > 0) rc=CalcShade(scene,rc,i,j);

	CImgFillRect(dest,SceneX(scene,i),SceneY(scene,j),zp,zp,rc);
      }

    BMapDestroy(isborder);
    break;

  case vmode_srctrees:

    isborder = BMapNew(W*H);
    BMapFill(isborder, 0);

    rootmap = (int *) MemAlloc(W * H * sizeof(int));
    for(i=0;i<W*H;i++)
      rootmap[i] = -1;

    switch(scene->options.src) {
    case src_orig:  maxval = XAGetOrigMax(av);  break;
    case src_value: maxval = XAGetValueMax(av); break;
    case src_cost:  maxval = XAGetCostMax(av);  break;
    }

    if (vmap->maxval != maxval) {
      vmap->maxval = maxval;
      ViewMapUpdate(vmap);
    }

    // calculate root map

    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {
	p = VolumeGetI32(vinfo,i,j,NPROJ);
	if (p!=VLARGE)
	  rootmap[i+j*W] = XAVolGetRootN(av, p);
      }

    for(i=B[0]+1;i<B[1];i++)
      for(j=B[2]+1;j<B[3];j++) {
	for(c=0;c<8;c++) {
	  if (rootmap[i+j*W] != rootmap[i+tx[c]+W*(j+ty[c])]) {
	    BMapSet(isborder, i + j*W, 1);
	    break;
	  }
	}
      }

    MemFree(rootmap);

    for(i=B[0];i<=B[1];i++)
      for(j=B[2];j<=B[3];j++) {

	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;

	d=VolumeGetI32(vinfo,i,j,SEED);
	if (d<0) {

	  switch(scene->options.src) {
	  case src_orig:  m = ViewMapColor(vmap,av->vd[N].orig);  break;
	  case src_value: m = ViewMapColor(vmap,av->vd[N].value); break;
	  case src_cost:  m = ViewMapColor(vmap,av->vd[N].cost);  break;
	  default: m=0;
	  }

	  if (BMapGet(isborder,i+j*W)) {
	    n=scene->colors.label[(av->vd[N].label&LABELMASK) % 10];
	    if (av->vd[N].label & REMOVEMASK)
	      n=inverseColor(n);
	    rc=mergeColorsRGB(m,n,0.66);
	    if (scene->colors.whiteborder)
	      rc=0xffffff;
	  } else {
	    rc = m;
	  }
	} else
	  rc=scene->colors.label[d];

	if (scene->options.shading > 0) rc=CalcShade(scene,rc,i,j);
	
	CImgFillRect(dest,SceneX(scene,i),SceneY(scene,j),zp,zp,rc);
      }

    // paint tree roots
    for(i=B[0]+1;i<B[1];i++)
      for(j=B[2]+1;j<B[3];j++) {
	N=VolumeGetI32(vinfo,i,j,NPROJ);
	if (N==VLARGE) continue;

	if (av->vd[N].pred == 0)
	  CImgFillRect(dest,SceneX(scene,i-1),SceneY(scene,j-1),
		       3*zp,3*zp,0x40ff40);
      }
    
    BMapDestroy(isborder);
    break;

  default:
    /* will never happen */
    break;

  } // switch vmode
}

/* returns the cosine of the angle between PhongLight and surface */

float Estimate8(Scene *s,int x,int y) {
  int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
  int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

  Volume *xyzbuffer = s->pvt.xyzbuffer;

  unsigned int i;
  int k, nv;
  float est;
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  if (!inbox(x,y,1,1,s->pvt.outw-2,s->pvt.outh-2)) return 1.0;

  nv=0;
  k = 1;
  for(i=0;i<8;i++) {

    p[0].x = VolumeGetF32(xyzbuffer, x, y, 0);
    if (p[0].x >= VLARGE_D) continue;
    p[0].y = VolumeGetF32(xyzbuffer, x, y, 1);
    p[0].z = VolumeGetF32(xyzbuffer, x, y, 2);

    p[1].x = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 0);
    if (p[1].x >= VLARGE_D) continue;
    p[1].y = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 1);
    p[1].z = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 2);

    p[2].x = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 0);
    if (p[2].x >= VLARGE_D) continue;
    p[2].y = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 1);
    p[2].z = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 2);
    
    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) return 1.0;
  VectorNormalize(&normal);

  // VectorNormalize(&(s->options.phong.light)); -- called once in CalcSurface
  est = VectorInnerProduct(&(s->options.phong.light), &normal);

  if (est < 0.0) est=0.0;

  return est;
}

// better quality, but slower
float Estimate16(Scene *s,int x,int y) {
  int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
  int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

  int qx[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
  int qy[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };

  Volume *xyzbuffer = s->pvt.xyzbuffer;

  unsigned int i;
  int k, nv;
  float est;
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  if (!inbox(x,y,2,2,s->pvt.outw-4,s->pvt.outh-4)) return 1.0;

  nv=0;
  k = 1;
  for(i=0;i<8;i++) {

    p[0].x = VolumeGetF32(xyzbuffer, x, y, 0);
    if (p[0].x >= VLARGE_D) continue;
    p[0].y = VolumeGetF32(xyzbuffer, x, y, 1);
    p[0].z = VolumeGetF32(xyzbuffer, x, y, 2);

    p[1].x = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 0);
    if (p[1].x >= VLARGE_D) continue;
    p[1].y = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 1);
    p[1].z = VolumeGetF32(xyzbuffer, x+nx[(i+k)%8], y+ny[(i+k)%8], 2);

    p[2].x = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 0);
    if (p[2].x >= VLARGE_D) continue;
    p[2].y = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 1);
    p[2].z = VolumeGetF32(xyzbuffer, x+nx[i], y+ny[i], 2);
    
    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  for(i=0;i<8;i++) {

    p[0].x = VolumeGetF32(xyzbuffer, x, y, 0);
    if (p[0].x >= VLARGE_D) continue;
    p[0].y = VolumeGetF32(xyzbuffer, x, y, 1);
    p[0].z = VolumeGetF32(xyzbuffer, x, y, 2);

    p[1].x = VolumeGetF32(xyzbuffer, x+qx[(i+k)%8], y+qy[(i+k)%8], 0);
    if (p[1].x >= VLARGE_D) continue;
    p[1].y = VolumeGetF32(xyzbuffer, x+qx[(i+k)%8], y+qy[(i+k)%8], 1);
    p[1].z = VolumeGetF32(xyzbuffer, x+qx[(i+k)%8], y+qy[(i+k)%8], 2);

    p[2].x = VolumeGetF32(xyzbuffer, x+qx[i], y+qy[i], 0);
    if (p[2].x >= VLARGE_D) continue;
    p[2].y = VolumeGetF32(xyzbuffer, x+qx[i], y+qy[i], 1);
    p[2].z = VolumeGetF32(xyzbuffer, x+qx[i], y+qy[i], 2);
    
    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) return 1.0;
  VectorNormalize(&normal);

  // VectorNormalize(&(s->options.phong.light)); -- called once in CalcSurface
  est = VectorInnerProduct(&(s->options.phong.light), &normal);

  if (est < 0.0) est=0.0;

  return est;
}

Transform RTrans;

void UpdateRTrans(Scene *s) {
  Transform Ta,Tb,Tc;
  TransformXRot(&Ta, s->rotation.x);
  TransformYRot(&Tb, s->rotation.y);
  TransformZRot(&Tc, s->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);
  TransformCopy(&RTrans, &Ta);
}

// gradient from the src values
// RTrans must be the current rotation transform
// implementado por desencargo de consciencia
float EstimateImgGrad(Scene *s,int x,int y) {
  Vector g,h;
  Voxel  v;
  int    nv,nx[2],ny[2],nz[2];
  float  est;

  nv = VolumeGetI32(s->pvt.vinfo,x,y,NPROJ);
  if (nv == VLARGE) return 1.0;

  XAVolumeVoxelOf(s->avol, nv, &v);

  nx[0] = nx[1] = ny[0] = ny[1] = nz[0] = nz[1] = nv;

  if (v.x > 0)            nx[0] -= 1;
  if (v.x < s->avol->W-1) nx[1] += 1;

  if (v.y > 0)            ny[0] -= s->avol->tbrow[1];
  if (v.y < s->avol->H-1) ny[1] += s->avol->tbrow[1];

  if (v.z > 0)            nz[0] -= s->avol->tbframe[1];
  if (v.z < s->avol->D-1) nz[1] += s->avol->tbframe[1];

  g.x = (float) (s->avol->vd[nx[1]].orig - s->avol->vd[nx[0]].orig);
  g.y = (float) (s->avol->vd[ny[1]].orig - s->avol->vd[ny[0]].orig);
  g.z = (float) (s->avol->vd[nz[1]].orig - s->avol->vd[nz[0]].orig);

  PointTransform(&h,&g,&RTrans);
  VectorNormalize(&h);

  est = VectorInnerProduct(&(s->options.phong.light), &h);
  if (est < 0.0)
    est = -est;
  return est;
}

// nao funciona
float EstimateA(Scene *s,int x,int y) {
  Volume *xyz = s->pvt.xyzbuffer;
  Vector rx, ry, a, b;
  int i,j;
  float est;

  char s0[256], s1[64];

  if (!inbox(x,y,2,2,s->pvt.outw-4,s->pvt.outh-4)) return 1.0;

  for(j=y-1;j<=y+1;j++)
    for(i=x-1;i<=x+1;i++)
      if (VolumeGetF32(xyz, i,j,0) == VLARGE_D)
	return(Estimate8(s,x,y));

  rx.x  = VolumeGetF32(xyz, x+1, y, 0) - VolumeGetF32(xyz, x-1, y, 0);
  rx.y  = VolumeGetF32(xyz, x+1, y, 1) - VolumeGetF32(xyz, x-1, y, 1);
  rx.z  = VolumeGetF32(xyz, x+1, y, 2) - VolumeGetF32(xyz, x-1, y, 2);

  sprintf(s1,"rx=<%.3f,%.3f,%.3f> ",rx.x,rx.y,rx.z);
  strcpy(s0, s1);

  ry.x  = VolumeGetF32(xyz, x, y+1, 0) - VolumeGetF32(xyz, x, y-1, 0);
  ry.y  = VolumeGetF32(xyz, x, y+1, 1) - VolumeGetF32(xyz, x, y-1, 1);
  ry.z  = VolumeGetF32(xyz, x, y+1, 2) - VolumeGetF32(xyz, x, y-1, 2);

  sprintf(s1,"ry=<%.3f,%.3f,%.3f> ",ry.x,ry.y,ry.z);
  strcat(s0, s1);

  VectorCrossProduct(&a, &rx, &ry);

  sprintf(s1,"rx x ry=<%.3f,%.3f,%.3f> ",a.x,a.y,a.z);
  strcat(s0, s1);

  /*
  rx.x  = VolumeGetF32(xyz, x+1, y-1, 0) - VolumeGetF32(xyz, x-1, y+1, 0);
  rx.y  = VolumeGetF32(xyz, x+1, y-1, 1) - VolumeGetF32(xyz, x-1, y+1, 1);
  rx.z  = VolumeGetF32(xyz, x+1, y-1, 2) - VolumeGetF32(xyz, x-1, y+1, 2);

  ry.x  = VolumeGetF32(xyz, x-1, y+1, 0) - VolumeGetF32(xyz, x+1, y-1, 0);
  ry.y  = VolumeGetF32(xyz, x-1, y+1, 1) - VolumeGetF32(xyz, x+1, y-1, 1);
  ry.z  = VolumeGetF32(xyz, x-1, y+1, 2) - VolumeGetF32(xyz, x+1, y-1, 2);

  VectorCrossProduct(&b, &rx, &ry);
  */

  //VectorAdd(&a, &b);

  VectorScalarProduct(&a, -1.0);
  VectorNormalize(&a);

  est = VectorInnerProduct(&(s->options.phong.light), &a);
  if (est < 0.0) {
    est=0.0;
    VectorCopy(&b, &(s->options.phong.light));
    printf("negative, normal=<%.3f,%.3f,%.3f>, light=<%.3f,%.3f,%.3f>\n",
	   a.x,a.y,a.z, b.x,b.y,b.z);
    printf("%s\n",s0);
    return(Estimate8(s,x,y));
  }
  return est;
}

/*
   y(x) = sum[k=0...N] f_k * l_k

   where l_k = (x - x0) * (x - x1) * ... (x - xk-1) * (x - xk+1) ... (x - xkn)
               -------------------------------------------------------------
                (xk - x0) * (xk - x1) * ... (xk - xk-1) * (xk - xk+1) ...
*/

float LagrangeDerivative(float *x, float *y, int np) {
  int i,k;
  float dy, den;

  float P[2];

  dy = 0.0;
  for(k=0;k<np;k++) {

    P[0] = 1.0; P[1] = 0.0;
    den = 1.0;
    for(i=0;i<np;i++)
      if (i!=k) {
	P[1] = P[0] + P[1] * -x[i];
	P[0] *= -x[i];
	den *= x[k] - x[i];
      }

    if (den != 0.0)
      dy += y[k] * P[1] / den;
  }
  
  return dy;
}

// outra ideia furada
float EstimateL(Scene *s,int x,int y) {
  Volume *xyz = s->pvt.xyzbuffer;
  float px[32], py[32], est;
  int i, np;
  Vector a,b,d;

  px[0] = 0.0;
  py[0] = VolumeGetF32(xyz, x, y, 2);
  if (py[0] == VLARGE_D) return 1.0;
  np=1;
  
  // interpolate X-direction
  
  for(i=-1;i>=-1;i--) {
    px[np] = i;
    py[np] = VolumeGetF32(xyz, x+i, y, 2);
    if (py[np]==VLARGE_D)
      break;
    ++np;
  }

  for(i=1;i<=1;i++) {
    px[np] = i;
    py[np] = VolumeGetF32(xyz, x+i, y, 2);
    if (py[np]==VLARGE_D)
      break;
    ++np;
  }

  if (np==1) return 0.5;

  a.x = 1.0;
  a.y = 0.0;
  a.z = LagrangeDerivative(px,py,np);

  // interpolate Y-direction

  np = 1;
  for(i=-1;i>=-1;i--) {
    px[np] = i;
    py[np] = VolumeGetF32(xyz, x, y+i, 2);
    if (py[np]==VLARGE_D)
      break;
    ++np;
  }

  for(i=1;i<=1;i++) {
    px[np] = i;
    py[np] = VolumeGetF32(xyz, x, y+i, 2);
    if (py[np]==VLARGE_D)
      break;
    ++np;
  }

  if (np==1) return 0.50;

  b.x = 0.0;
  b.y = 1.0;
  b.z = LagrangeDerivative(px,py,np);

  VectorCrossProduct(&d, &b, &a);

  VectorNormalize(&d);

  est = VectorInnerProduct(&(s->options.phong.light), &d);

  if (est < 0.0) est=0.0;

  return est;
}

void CalcEShade(Scene *s) {
  int i,j;
  s->eshade = s->options.shading;
  if (s->eshade == shading_auto) {
    switch(s->options.vmode) {
    case vmode_src:     
      s->eshade = shading_none;
      break;
    default:
      j = 0;
      for(i=0;i<10;i++) if (s->hollow[i]) ++j;
      s->eshade = (j>0) ? shading_phong : shading_none;
      break;
    }
  }
}

void  CalcSurface(Scene *s) {
  int i,j,W,H,vW,vWH;
  VVFunc estimator = Estimate16;
  ShadingType eshade = s->eshade;
  float x,vcos[3];
  int nx[8] = { -1,0,1,1,1,0,-1,-1 };
  int ny[8] = { -1,-1,-1,0,1,1,1,0 };
  int k,T[12];

  W = s->pvt.outw;
  H = s->pvt.outh;

  if (eshade == shading_none ||
      eshade == shading_lin_z_dist ||
      eshade == shading_quad_z_dist)
    return;

  VolumeFillF(s->pvt.surface, 0.0);

  // normalize the light vector
  VectorNormalize(&(s->options.phong.light));

  if (eshade == shading_grad_phong) {
    estimator = EstimateImgGrad;
    UpdateRTrans(s);
  }

  switch(eshade) {
  case shading_phong_sq:
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (VolumeGetI32(s->pvt.vinfo, i,j, NPROJ) != VLARGE) {
	  x = estimator(s,i,j);
	  VolumeSetF32(s->pvt.surface, i,j,0, x*x);
	}
    break;
  case shading_phong_cube:
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (VolumeGetI32(s->pvt.vinfo, i,j, NPROJ) != VLARGE) {
	  x = estimator(s,i,j);
	  VolumeSetF32(s->pvt.surface, i,j,0, x*x*x);
	}
    break;
  case shading_phong_cartoon:
    vcos[0] = cos( 75.0 * M_PI / 180.0 );
    vcos[1] = cos( 50.0 * M_PI / 180.0 );
    vcos[2] = cos( 15.0 * M_PI / 180.0 );

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (VolumeGetI32(s->pvt.vinfo, i,j, NPROJ) != VLARGE) {
	  x = estimator(s,i,j);
	  if (x < vcos[0]) x = 0.01; else
	    if (x < vcos[1]) x = 0.25; else
	      if (x < vcos[2]) x = 0.75; else
		x = 1.00;
	  VolumeSetF32(s->pvt.surface, i,j,0, x);
	}

    vW = s->avol->W;
    vWH = vW * s->avol->H;
    
    for(j=1;j<H-1;j++)
      for(i=1;i<W-1;i++) {
	T[0] = VolumeGetI32(s->pvt.vinfo, i,j, NPROJ);
	if (T[0] != VLARGE) {
	  T[10] = VolumeGetI32(s->pvt.vinfo, i,j, SEED);
	  T[1] = VOXELX(T[0],vW,vWH);
	  T[2] = VOXELY(T[0],vW,vWH);
	  T[3] = VOXELZ(T[0],vW,vWH);
	  T[9] = 0;
	  for(k=0;k<8;k++) {
	    T[4] = VolumeGetI32(s->pvt.vinfo,i+nx[k],j+ny[k],NPROJ);
	    if (T[4] == VLARGE) { T[9]=1; break; }
	    T[11] = VolumeGetI32(s->pvt.vinfo, i+nx[k],j+ny[k], SEED);
	    if (T[10] != T[11]) { T[9]=1; break; }
	    T[5] = VOXELX(T[4],vW,vWH);
	    T[6] = VOXELY(T[4],vW,vWH);
	    T[7] = VOXELZ(T[4],vW,vWH);
	    T[8] = (T[1]-T[5])*(T[1]-T[5]) + 
	      (T[2]-T[6])*(T[2]-T[6]) + 
	      (T[3]-T[7])*(T[3]-T[7]);
	    if (T[8] > 25) { T[9]=1; break; }
	  }
	  if (T[9]) VolumeSetF32(s->pvt.surface,i,j,0, 0.0);
	}
      }
    break;
  default:
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (VolumeGetI32(s->pvt.vinfo, i,j, NPROJ) != VLARGE)
	  VolumeSetF32(s->pvt.surface, i,j,0, estimator(s,i,j));
  }

  if (eshade == shading_smooth_phong) {
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	VolumeSetF32(s->pvt.surface, i,j,1, VolumeGetF32(s->pvt.surface,i,j,0));
    SmoothSurface(s);
  }
}

void   SmoothSurface(Scene *scene) {
  int i,j, m,n, W,H;
  float a[5][5],
    kernel[5][5] = { {1.0, 2.0,  2.0, 2.0, 1.0} , 
		     {2.0, 4.0,  4.0, 4.0, 2.0} ,
		     {2.0, 5.0, 40.0, 5.0, 2.0} ,
		     {2.0, 4.0,  5.0, 4.0, 2.0} ,
		     {1.0, 2.0,  2.0, 2.0, 1.0} }, v, netweight;

  W = scene->pvt.outw;
  H = scene->pvt.outh;

  for(i=2;i<W-2;i++)
    for(j=2;j<H-2;j++) {      
      netweight = 0.0;
      v = 0.0;
      for(m=0;m<5;m++)
	for(n=0;n<5;n++) {
	  a[m][n] = VolumeGetF32(scene->pvt.surface, i+m-2, j+n-2, 1);
	  if (a[m][n] > 1.5) continue;
	  v += a[m][n] * kernel[m][n];
	  netweight += kernel[m][n];
	}
      if (netweight > 0.0)
	VolumeSetF32(scene->pvt.surface,i,j,0, v/netweight);
      else
	VolumeSetF32(scene->pvt.surface,i,j,0, 0.0);
    }
}

/* 
   returns the color c (RGB triplet) as the resulting
   color after the shading.
*/
int  CalcShade(Scene *scene,int c,int x,int y) {
  int z;
  float dz, dr, Y, ncos;
  int lum;
  ShadingType eshade = scene->eshade;

  Volume *vinfo          = scene->pvt.vinfo;
  PhongParameters *phong = &(scene->options.phong);

  // ambient, diffuse, specular, zdist
  float ka, kd, ks, kz;

  switch(eshade) {
  case shading_none: return c;
  case shading_lin_z_dist: // zdist
    if (scene->pvt.minz == scene->pvt.maxz) return c;
    z=VolumeGetI32(vinfo, x, y, ZBUFFER);
    dr = (float)(scene->pvt.maxz - scene->pvt.minz);
    dz = (float)(z - scene->pvt.minz);
    dz = 1.0 - dz/dr;
    dz = 0.3333 + 0.6666 * dz;
    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y *= dz;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;
  case shading_quad_z_dist: // quadratic zdist
    if (scene->pvt.minz == scene->pvt.maxz) return c;
    z=VolumeGetI32(vinfo, x, y, ZBUFFER);
    dr = (float)(scene->pvt.maxz - scene->pvt.minz);
    dz = (float)(scene->pvt.maxz - z);
    dz = (dz*dz) / (dr*dr);
    dz = 0.3333 + 0.6666 * dz;
    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y *= dz;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;
  case shading_normal: // estimate normal
    //    dz = EstimateVoxelNormal(scene,x,y);
    dz = VolumeGetF32(scene->pvt.surface, x, y, 0);
    dz = 0.25 + 0.75 * dz;
    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y *= dz;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;
  case shading_quad_z_normal: // estimate normal + quad z dist

    if (scene->pvt.minz == scene->pvt.maxz) {
      dz = 1.0;
    } else {
      z=VolumeGetI32(vinfo, x, y, ZBUFFER);
      dr = (float)(scene->pvt.maxz - scene->pvt.minz);
      dz = (float)(scene->pvt.maxz - z);
      dz = (dz*dz) / (dr*dr);
    }

    dz *= 1.25;
    dz += 0.25;
    //    dz *= EstimateVoxelNormal(scene,x,y);
    dz *= VolumeGetF32(scene->pvt.surface, x, y, 0);
    dz = 0.25 + 0.75 * dz;
    lum=RGB2YCbCr(c);

    Y = (float) t0(lum);
    Y *= dz;
    if (Y > 255.0) Y=255.0;
    if (Y < 0.0) Y=0.0;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;

  case shading_phong:             // phong
  case shading_smooth_phong:      // smooth phong (not used in IVS)
  case shading_grad_phong:        // phong from scene gradient
  case shading_phong_sq:          // phong with squared cosines
  case shading_phong_cube:        // phong with cubed cosines
    if (scene->pvt.minz == scene->pvt.maxz) {
      kz = 1.0;
    } else {
      z=VolumeGetI32(vinfo, x, y, ZBUFFER);
      dr = (float)(scene->pvt.maxz - scene->pvt.minz);
      dz = (float)(scene->pvt.maxz - z);
      kz = (dz*dz) / (dr*dr);
    }

    kz = phong->zgamma + phong->zstretch * kz;
    ka = phong->ambient;

    ncos = VolumeGetF32(scene->pvt.surface, x, y, 0);
    kd = PhongDiffuse(ncos);
    ks = PhongSpecular(ncos, phong->sre);

    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y /= 255.0;
    
    Y = ka + kz * ( phong->diffuse * Y * kd + phong->specular * Y * ks);
    Y *= 255.0;
    if (Y > 255.0) Y=255.0;
    if (Y < 0.0) Y=0.0;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;

  case shading_phong_cartoon:     // phong with 4-quatized normals

    if (scene->pvt.minz == scene->pvt.maxz) {
      kz = 1.0;
    } else {
      z=VolumeGetI32(vinfo, x, y, ZBUFFER);
      dr = (float)(scene->pvt.maxz - scene->pvt.minz);
      dz = (float)(scene->pvt.maxz - z);
      kz = (dz*dz) / (dr*dr);
    }

    kz = phong->zgamma + phong->zstretch * kz;
    ka = phong->ambient;

    ncos = VolumeGetF32(scene->pvt.surface, x, y, 0);
    kd = PhongDiffuse(ncos);

    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y /= 255.0;
    
    Y = ka + kz * Y * kd ;
    Y *= 255.0;
    if (Y > 255.0) Y=255.0;
    if (Y < 0.0) Y=0.0;
    if (ncos == 0.0) Y = 0.0;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;

    /*
  case shading_smooth_phong: // smoothed phong
    if (scene->pvt.minz == scene->pvt.maxz) {
      kz = 1.0;
    } else {
      z=VolumeGetI32(vinfo, x, y, ZBUFFER);
      dr = (float)(scene->pvt.maxz - scene->pvt.minz);
      dz = (float)(scene->pvt.maxz - z);
      kz = (dz*dz) / (dr*dr);
    }

    kz = phong->zgamma + phong->zstretch * kz;
    ka = phong->ambient;

    ncos = VolumeGetF32(scene->pvt.surface, x, y, 0);
    kd = PhongDiffuse(ncos);
    ks = PhongSpecular(ncos, phong->sre);

    lum=RGB2YCbCr(c);
    Y = (float) t0(lum);
    Y /= 255.0;
    
    Y = ka + kz * ( phong->diffuse * Y * kd +  phong->specular * Y * ks);
    Y *= 255.0;
    if (Y > 255.0) Y=255.0;
    if (Y < 0.0) Y=0.0;
    lum = triplet((int) Y, t1(lum), t2(lum));
    lum = YCbCr2RGB(lum);
    return lum;
    */

  default:
    return 0;
  }
}

float PhongSpecular(float angcos, int n) {
  float a,r;

  a=acos(angcos);
  if (a > M_PI / 4.0)
    return 0.0;

  a=cos(2.0 * a);
  r=a;
  while(n!=1) { r*=a; --n; }
  return r;
}

#define HGRID 128
#define VGRID 128
void FindMesh(Scene *scene) {
  int n[HGRID][VGRID];
  int i,j,k,w,h;
  Voxel a,b,c;
  SceneInitAMesh(scene, HGRID, VGRID);

  if (!scene->avol)
    return;

  for(i=0;i<HGRID;i++)
    for(j=0;j<VGRID;j++)
      n[i][j] = -1;
  
  w = scene->pvt.outw;
  h = scene->pvt.outh;

  for(i=0;i<HGRID;i++)
    for(j=0;j<VGRID;j++) {
      k = VolumeGetI32(scene->pvt.vinfo, i*(w/HGRID), j*(h/VGRID), NPROJ);
      n[i][j] = (k==VLARGE) ? -1 : k;
    }

  k = 0;
  for(j=0;j<VGRID-1;j++)
    for(i=0;i<HGRID-1;i++) {
      if (n[i+1][j]!=-1 && n[i][j+1]!=-1) {
	XAVolumeVoxelOf(scene->avol, n[i+1][j], &a);
	XAVolumeVoxelOf(scene->avol, n[i][j+1], &b);

	if (n[i][j]!=-1) {
	  XAVolumeVoxelOf(scene->avol, n[i][j], &c);
	  scene->amesh.srct[k].p[0].x = c.x;
	  scene->amesh.srct[k].p[0].y = c.y;
	  scene->amesh.srct[k].p[0].z = c.z;
	  scene->amesh.srct[k].p[1].x = a.x;
	  scene->amesh.srct[k].p[1].y = a.y;
	  scene->amesh.srct[k].p[1].z = a.z;
	  scene->amesh.srct[k].p[2].x = b.x;
	  scene->amesh.srct[k].p[2].y = b.y;
	  scene->amesh.srct[k].p[2].z = b.z;
	  scene->amesh.tl[k] = scene->avol->vd[n[i][j]].label&LABELMASK;
	  ++k;
	}

	if (n[i+1][j+1]!=-1) {
	  XAVolumeVoxelOf(scene->avol, n[i+1][j+1], &c);
	  scene->amesh.srct[k].p[0].x = c.x;
	  scene->amesh.srct[k].p[0].y = c.y;
	  scene->amesh.srct[k].p[0].z = c.z;
	  scene->amesh.srct[k].p[1].x = a.x;
	  scene->amesh.srct[k].p[1].y = a.y;
	  scene->amesh.srct[k].p[1].z = a.z;
	  scene->amesh.srct[k].p[2].x = b.x;
	  scene->amesh.srct[k].p[2].y = b.y;
	  scene->amesh.srct[k].p[2].z = b.z;
	  scene->amesh.tl[k] = scene->avol->vd[n[i+1][j+1]].label&LABELMASK;
	  ++k;
	}

      } else if (n[i][j]!=-1 && n[i+1][j+1]!=-1) {
	XAVolumeVoxelOf(scene->avol, n[i][j], &a);
	XAVolumeVoxelOf(scene->avol, n[i+1][j+1], &b);

	if (n[i+1][j]!=-1) {
	  XAVolumeVoxelOf(scene->avol, n[i+1][j], &c);
	  scene->amesh.srct[k].p[0].x = c.x;
	  scene->amesh.srct[k].p[0].y = c.y;
	  scene->amesh.srct[k].p[0].z = c.z;
	  scene->amesh.srct[k].p[1].x = a.x;
	  scene->amesh.srct[k].p[1].y = a.y;
	  scene->amesh.srct[k].p[1].z = a.z;
	  scene->amesh.srct[k].p[2].x = b.x;
	  scene->amesh.srct[k].p[2].y = b.y;
	  scene->amesh.srct[k].p[2].z = b.z;
	  scene->amesh.tl[k] = scene->avol->vd[n[i+1][j]].label&LABELMASK;
	  ++k;
	}

	if (n[i][j+1]!=-1) {
	  XAVolumeVoxelOf(scene->avol, n[i][j+1], &c);
	  scene->amesh.srct[k].p[0].x = c.x;
	  scene->amesh.srct[k].p[0].y = c.y;
	  scene->amesh.srct[k].p[0].z = c.z;
	  scene->amesh.srct[k].p[1].x = a.x;
	  scene->amesh.srct[k].p[1].y = a.y;
	  scene->amesh.srct[k].p[1].z = a.z;
	  scene->amesh.srct[k].p[2].x = b.x;
	  scene->amesh.srct[k].p[2].y = b.y;
	  scene->amesh.srct[k].p[2].z = b.z;
	  scene->amesh.tl[k] = scene->avol->vd[n[i][j+1]].label&LABELMASK;
	  ++k;
	}

      }

    }

  scene->amesh.ntriangles = k;
  scene->amesh.validmesh  = 1;
}

int CalcAMesh(Scene *scene, float rx, float ry, float rz) {
  Transform Ta,Tb,Tc,Td;
  float ax,ay,az,bx,by,zmin,zmax,mz;
  int i,nt;
  Triangle t,*tp;
  Queue *Q;

  if (!scene->amesh.validmesh)
    return 0;
  if (!scene->avol) {
    scene->amesh.validmesh=0;
    return 0;
  }

  TransformXRot(&Ta, rx);
  TransformYRot(&Tb, ry);
  TransformZRot(&Tc, rz);
  TransformIsoScale(&Td, SceneScale(scene));
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);
  TransformCompose(&Td,&Ta);

  nt = scene->amesh.ntriangles;

  ax = (float) (- scene->avol->W / 2);
  ay = (float) (- scene->avol->H / 2);
  az = (float) (- scene->avol->D / 2);

  bx = (float)(scene->pvt.oxc);
  by = (float)(scene->pvt.oyc);

  zmin = 8000.0;
  zmax = -8000.0;
  for(i=0;i<nt;i++) {
    MemCpy(&t, &(scene->amesh.srct[i]), sizeof(Triangle));
    TriangleTranslate(&t, ax, ay, az);
    tp = &(scene->amesh.tmpt[i]);
    TriangleTransform(tp,&t,&Td);
    TriangleTranslate(tp, bx, by, 0);
    mz = (tp->p[0].z + tp->p[1].z + tp->p[2].z) / 3.0; 
    if (mz > zmax) zmax = mz;
    if (mz < zmin) zmin = mz;
    scene->amesh.tz[i] = mz;
  }

  if (zmax - zmin < 1.0) ++zmax;

  for(i=0;i<nt;i++)
    scene->amesh.tz[i] = (scene->amesh.tz[i] - zmin) / (zmax - zmin);

  // heap-sort the triangles in z-order
  Q = CreateQueue(257, nt+1);
  for(i=0;i<nt;i++) {
    InsertQueue(Q, (int) (scene->amesh.tz[i] * 256.0), i);
  }
  for(i=nt-1;i>=0;i--) {
    scene->amesh.tsort[i] = RemoveQueue(Q);
  }

  DestroyQueue(&Q);
  SceneShrinkAMesh(scene);
  return 1;
}

float AMeshCos(Scene *scene, int i) {
  Vector a,b,c,d;
  float r;

  a.x = scene->amesh.tmpt[i].p[0].x - scene->amesh.tmpt[i].p[1].x;
  a.y = scene->amesh.tmpt[i].p[0].y - scene->amesh.tmpt[i].p[1].y;
  a.z = scene->amesh.tmpt[i].p[0].z - scene->amesh.tmpt[i].p[1].z;

  b.x = scene->amesh.tmpt[i].p[0].x - scene->amesh.tmpt[i].p[2].x;
  b.y = scene->amesh.tmpt[i].p[0].y - scene->amesh.tmpt[i].p[2].y;
  b.z = scene->amesh.tmpt[i].p[0].z - scene->amesh.tmpt[i].p[2].z;

  d.x = d.y = 0.0;
  d.z = -1.0;

  VectorCrossProduct(&c,&a,&b);
  VectorNormalize(&c);
  r = VectorInnerProduct(&c,&d);
  if (r < 0.0) r = -r;
  return r;
}


  /*
                    _
      4 --- 5       /| (Z)
     /     /|      /
    0 --- 1 |     *--> (X)
    | 7   | 6     |
    |     |/     \|/
    3 --- 2      (Y)

    0 = <x1,y1,z1>    6 = <x2,y2,z2>

    edge index: 
                 0 :=: 0-1
                 1 :=: 0-3
                 2 :=: 0-4

                 3 :=: 6-7
		 4 :=: 6-5
		 5 :=: 6-2

                 6 :=: 2-1
		 7 :=: 2-3

		 8  :=: 5-1
		 9  :=: 5-4

		 10 :=: 7-3
		 11 :=: 7-4
  */

void MakeCubeEdges(Point *p, Point *q, char *vis, Transform *T,
		   float x1, float y1, float z1,
		   float x2, float y2, float z2) 
{
  int i,j,k;
  Point a,b;
  float maxz=-10000.0;

  for(i=0;i<12;i++) vis[i] = 1;

  PointAssign(&p[0], x1, y1, z1);  PointAssign(&q[0], x2, y1, z1);
  PointAssign(&p[1], x1, y1, z1);  PointAssign(&q[1], x1, y2, z1);
  PointAssign(&p[2], x1, y1, z1);  PointAssign(&q[2], x1, y1, z2);

  PointAssign(&p[3], x2, y2, z2);  PointAssign(&q[3], x1, y2, z2);
  PointAssign(&p[4], x2, y2, z2);  PointAssign(&q[4], x2, y1, z2);
  PointAssign(&p[5], x2, y2, z2);  PointAssign(&q[5], x2, y2, z1);

  PointAssign(&p[6], x2, y2, z1);  PointAssign(&q[6], x2, y1, z1);
  PointAssign(&p[7], x2, y2, z1);  PointAssign(&q[7], x1, y2, z1);

  PointAssign(&p[8], x2, y1, z2);  PointAssign(&q[8], x2, y1, z1);
  PointAssign(&p[9], x2, y1, z2);  PointAssign(&q[9], x1, y1, z2);

  PointAssign(&p[10], x1, y2, z2); PointAssign(&q[10], x1, y2, z1);
  PointAssign(&p[11], x1, y2, z2); PointAssign(&q[11], x1, y1, z2);

  if ((x1 != x2) && (y1 != y2) && (z1 != z2)) {

    for(i=0;i<2;i++)
      for(j=0;j<2;j++)
	for(k=0;k<2;k++) {
	  PointAssign(&a, i?x1:x2, j?y1:y2, k?z1:z2);
	  PointTransform(&b, &a, T);
	  if (b.z > maxz) maxz = b.z;
	}

    for(i=0;i<12;i++) {
      PointTransform(&b, &p[i], T);
      if (b.z == maxz) {
	vis[i] = 0;
	continue;
      }
      PointTransform(&b, &q[i], T);
      if (b.z == maxz)
	vis[i] = 0;
    }

  }
  
}

void RenderClipCube(Scene *scene) {
  
  Point p[12], q[12] , r[12], s[12];
  char vis[12];
  int i;
  Transform Ta,Tb,Tc;
  int hw, hh, hd, w, h, d;
  Volume *xyzbuffer = scene->pvt.xyzbuffer;

  w = scene->avol->W;
  h = scene->avol->H;
  d = scene->avol->D;
  hw = w >> 1;
  hh = h >> 1;
  hd = d >> 1;

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  /* full volume cube */
  MakeCubeEdges(p,q, vis, &Ta, 0,0,0, w-1,h-1,d-1);

  for(i=0;i<12;i++) {
    PointTranslate(&p[i], -hw, -hh, -hd);
    PointTransform(&r[i], &p[i], &Ta);
    PointTranslate(&r[i], scene->pvt.oxc, scene->pvt.oyc, 0);
    PointTranslate(&q[i], -hw, -hh, -hd);
    PointTransform(&s[i], &q[i], &Ta);
    PointTranslate(&s[i], scene->pvt.oxc, scene->pvt.oyc, 0);
  }

  for(i=0;i<12;i++)
    if (vis[i])
      if (r[i].z <= VolumeGetF32(xyzbuffer, (int) (r[i].x), (int) (r[i].y), 2)
	  &&
	  s[i].z <= VolumeGetF32(xyzbuffer, (int) (s[i].x), (int) (s[i].y), 2))
	CImgDrawLine(scene->vout, 
		     SceneX(scene, r[i].x), SceneY(scene,r[i].y), 
		     SceneX(scene, s[i].x), SceneY(scene,s[i].y), 
		     scene->colors.fullcube);

  /* clip cube */

  MakeCubeEdges(p,q, vis, &Ta,
		scene->clip.v[0][0],
		scene->clip.v[1][0],
		scene->clip.v[2][0],
		scene->clip.v[0][1],
		scene->clip.v[1][1],
		scene->clip.v[2][1]);

  for(i=0;i<12;i++) {
    PointTranslate(&p[i], -hw, -hh, -hd);
    PointTransform(&r[i], &p[i], &Ta);
    PointTranslate(&r[i], scene->pvt.oxc, scene->pvt.oyc, 0);
    PointTranslate(&q[i], -hw, -hh, -hd);
    PointTransform(&s[i], &q[i], &Ta);
    PointTranslate(&s[i], scene->pvt.oxc, scene->pvt.oyc, 0);
  }

  for(i=0;i<12;i++)
    if (vis[i])
      if (r[i].z <= VolumeGetF32(xyzbuffer, (int) (r[i].x), (int) (r[i].y), 2)
	  &&
	  s[i].z <= VolumeGetF32(xyzbuffer, (int) (s[i].x), (int) (s[i].y), 2))
	CImgDrawLine(scene->vout, 
		     SceneX(scene, r[i].x), SceneY(scene,r[i].y), 
		     SceneX(scene, s[i].x), SceneY(scene,s[i].y), 
		     scene->colors.clipcube);

}

void RenderPaths(Scene *scene) {
  int W,H,HW,HH,HD;
  double zp;
  int i, j, k, id[3], jd[3], kd[3];
  int n,p;
  Transform Ta,Tb,Tc,Td;
  Point a,b,c,d;
  XAnnVolume *avol = scene->avol;
  Volume *vinfo = scene->pvt.vinfo;
  int c1,c2,c3, sp[4];
  float diag,base,blend;
  Voxel v;
  int u;

  W = scene->pvt.outw;
  H = scene->pvt.outh;
  zp = SceneScale(scene);

  CImgFillRect(scene->vout,0,0,W,H,scene->colors.bg);

  VolumeFill(scene->pvt.vinfo, VLARGE);
  VolumeFillF(scene->pvt.xyzbuffer, VLARGE_D);

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformIsoScale(&Td, (float) zp);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);
  TransformCompose(&Td,&Ta);

  HW = avol->W / 2;
  HH = avol->H / 2;
  HD = avol->D / 2;

  CalcZLimit(scene);
  base  = (float) (scene->pvt.minz);
  base *= (float) zp;

  diag  = (float) (scene->pvt.maxz - scene->pvt.minz);
  diag *= (float) zp;

  PointAssign(&a, 0.0, 0.0, 0.0);
  PointAssign(&b, 0.0, 0.0, (float) (scene->avol->D));
  
  PointTransform(&c,&a,&Td);
  PointTransform(&d,&b,&Td);

  if (c.z > d.z) {
    kd[0] = scene->clip.v[2][0];
    kd[1] = scene->clip.v[2][1]+1;
    kd[2] = 1;
  } else {
    kd[0] = scene->clip.v[2][1];
    kd[1] = scene->clip.v[2][0]-1;
    kd[2] = -1;
  }

  PointAssign(&b, 0.0, (float) (scene->avol->H), 0.0);
  PointTransform(&d,&b,&Td);

  if (c.z > d.z) {
    jd[0] = scene->clip.v[1][0];
    jd[1] = scene->clip.v[1][1]+1;
    jd[2] = 1;
  } else {
    jd[0] = scene->clip.v[1][1];
    jd[1] = scene->clip.v[1][0]-1;
    jd[2] = -1;
  }

  PointAssign(&b, (float) (scene->avol->W), 0.0, 0.0);
  PointTransform(&d,&b,&Td);

  if (c.z > d.z) {
    id[0] = scene->clip.v[0][0];
    id[1] = scene->clip.v[0][1]+1;
    id[2] = 1;
  } else {
    id[0] = scene->clip.v[0][1];
    id[1] = scene->clip.v[0][0]-1;
    id[2] = -1;
  }

  c1 = scene->colors.bg;

  for(k=kd[0];k!=kd[1];k+=kd[2]) 
    for(j=jd[0];j!=jd[1];j+=jd[2])
      for(i=id[0];i!=id[1];i+=id[2]) {
	n = i + avol->tbrow[j] + avol->tbframe[k];
	if (scene->hollow[avol->vd[n].label & LABELMASK])
	  continue;
	p = XAGetDecPredecessor(avol, n);
	a.x = i - HW; a.y = j - HH; a.z = k - HD;
	PointTransform(&c,&a,&Td);

	sp[0] = scene->pvt.oxc + c.x;
	sp[1] = scene->pvt.oyc + c.y;
	if (sp[0] < 3 || sp[0] >= (scene->pvt.outw-3)) continue; 
	if (sp[1] < 3 || sp[1] >= (scene->pvt.outh-3)) continue; 

	u = VolumeGetI32(vinfo, sp[0], sp[1], ZBUFFER);

	if (u < c.z) continue;

	VolumeSetI32(vinfo, sp[0], sp[1], ZBUFFER, (int) (c.z));
	VolumeSetI32(vinfo, sp[0], sp[1], NPROJ, n);

	c2 = scene->colors.label[avol->vd[n].label & LABELMASK];
	if (p==n) {
	  blend = (c.z - base) / diag;
	  c3 = mergeColorsRGB(c2, c1, blend);
	  CImgFillRect(scene->vout, sp[0]-1, sp[1]-1, 3, 3, c3);
	} else {
	  XAVolumeVoxelOf(avol, p, &v);
	  sp[0] = scene->pvt.oxc + c.x;
	  sp[1] = scene->pvt.oyc + c.y;
	  b.x = v.x - HW; b.y = v.y - HH; b.z = v.z - HD;
	  PointTransform(&d,&b,&Td);
	  sp[2] = scene->pvt.oxc + d.x;
	  sp[3] = scene->pvt.oyc + d.y;
	  if (sp[0] < 0 || sp[0] >= scene->pvt.outw) continue; 
	  if (sp[1] < 0 || sp[1] >= scene->pvt.outh) continue; 
	  if (sp[2] < 0 || sp[2] >= scene->pvt.outw) continue; 
	  if (sp[3] < 0 || sp[3] >= scene->pvt.outh) continue; 
	  blend = (((c.z+d.z)/2.0) - base) / diag;
	  c3 = mergeColorsRGB(c2, c1, blend);
	  CImgDrawLine(scene->vout, sp[0], sp[1], sp[2], sp[3], c3);
	}
      }
}

void  RenderFrames(Scene *scene, VModeType mode,
		   CImg **xy, CImg **zy, CImg **xz,
		   int fx, int fy, int fz)
{
  int W,H,D;
  int i,j,k,p,q,r,sx,sy,psz,c,d,m,n,x,y;
  ltype l1,l2;
  double scale;
  XAnnVolume *avol = scene->avol;
  ViewMap *vmap = scene->vmap;
  AdjRel *A;
  Voxel v;

  W = avol->W;
  H = avol->H;
  D = avol->D;
  A = Spherical(1.0);

  psz = ScenePixel(scene);
  scale = SceneScale(scene);

  // XY view
  sx = (int) ceil(scale * ((double) W));
  sy = (int) ceil(scale * ((double) H));

  if ( (*xy) != 0 ) {
    if ( (*xy)->W != sx || (*xy)->H != sy ) {
      CImgDestroy(*xy);
      *xy = 0;
    }
  }
  if ( *xy == 0 ) {
    *xy = CImgNew(sx, sy);
  }
  CImgFill(*xy, scene->colors.bg);

  for(j=0;j<H;j++)
    for(i=0;i<W;i++) {
      n = i + avol->tbrow[j] + avol->tbframe[fz];

      // underlying color
      switch(scene->options.src) {
      case src_orig:  c = ViewMapColor(vmap,avol->vd[n].orig);  break;
      case src_value: c = ViewMapColor(vmap,avol->vd[n].value); break;
      case src_cost:  c = ViewMapColor(vmap,avol->vd[n].cost);  break;
      default: c=0;
      }

      // mix with label color
      if (mode == vmode_srclabel) {
	d= scene->colors.label[(avol->vd[n].label&LABELMASK) % 10];
	if (avol->vd[n].label & REMOVEMASK)
	  d=inverseColor(d);
	c=mergeColorsRGB(c,d,1.0 - scene->options.srcmix);
      }

      x = (int) (scale * ((double)i));
      y = (int) (scale * ((double)j));
      if (psz > 1)
	CImgFillRect(*xy, x,y, psz, psz, c);
      else
	CImgSet(*xy, x, y, c);
    }

  // paint borders
  if (mode == vmode_srcborder) {
    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	n = i + avol->tbrow[j] + avol->tbframe[fz];
	l1 = avol->vd[n].label & LABELMASK;
	
	for(k=1;k<A->n;k++) {
	  p = i  + A->dx[k];
	  q = j  + A->dy[k];
	  r = fz + A->dz[k];
	  
	  if (p < 0 || p >= W || q < 0 || q >= H || r < 0 || r >= D)
	    continue;
	  
	  m = p + avol->tbrow[q] + avol->tbframe[r];
	  l2 = avol->vd[m].label & LABELMASK;
	  
	  if (l1 != l2) {
	    c = scene->colors.label[l1];
	    
	    x = (int) (scale * ((double)i));
	    y = (int) (scale * ((double)j));
	    if (psz > 1)
	      CImgFillRect(*xy, x,y, psz, psz, c);
	    else
	      CImgSet(*xy, x, y, c);
	    
	    break;
	  }
	  
	} // for k
      } // for i, for j
  } // vmode == srcborder

  // paint seeds
  j = MapSize(avol->addseeds);
  for(i=0;i<j;i++) {
    k = MapGet(avol->addseeds, i, &p, &q);
    XAVolumeVoxelOf(avol,p,&v);
    if (v.z == fz) {
      c = scene->colors.label[q % 10];
      x = (int) (scale * ((double)(v.x)));
      y = (int) (scale * ((double)(v.y)));
      if (psz > 1)
	CImgFillRect(*xy, x,y, psz, psz, c);
      else
	CImgSet(*xy, x, y, c);      
    }
  }


  // ZY view
  sx = (int) ceil(scale * ((double) D));
  sy = (int) ceil(scale * ((double) H));

  if ( (*zy) != 0 ) {
    if ( (*zy)->W != sx || (*zy)->H != sy ) {
      CImgDestroy(*zy);
      *zy = 0;
    }
  }
  if ( *zy == 0 ) {
    *zy = CImgNew(sx, sy);
  }
  CImgFill(*zy, scene->colors.bg);

  for(j=0;j<H;j++)
    for(i=0;i<D;i++) {
      n = fx + avol->tbrow[j] + avol->tbframe[i];

      // underlying color
      switch(scene->options.src) {
      case src_orig:  c = ViewMapColor(vmap,avol->vd[n].orig);  break;
      case src_value: c = ViewMapColor(vmap,avol->vd[n].value); break;
      case src_cost:  c = ViewMapColor(vmap,avol->vd[n].cost);  break;
      default: c=0;
      }

      // mix with label color
      if (mode == vmode_srclabel) {
	d= scene->colors.label[(avol->vd[n].label&LABELMASK) % 10];
	if (avol->vd[n].label & REMOVEMASK)
	  d=inverseColor(d);
	c=mergeColorsRGB(c,d,1.0 - scene->options.srcmix);
      }

      x = (int) (scale * ((double)i));
      y = (int) (scale * ((double)j));
      if (psz > 1)
	CImgFillRect(*zy, x,y, psz, psz, c);
      else
	CImgSet(*zy, x, y, c);
    }

  // paint borders
  if (mode == vmode_srcborder) {
    for(j=0;j<H;j++)
      for(i=0;i<D;i++) {
	n = fx + avol->tbrow[j] + avol->tbframe[i];
	l1 = avol->vd[n].label & LABELMASK;
	
	for(k=1;k<A->n;k++) {
	  p = fx + A->dx[k];
	  q = j  + A->dy[k];
	  r = i  + A->dz[k];
	  
	  if (p < 0 || p >= W || q < 0 || q >= H || r < 0 || r >= D)
	    continue;
	  
	  m = p + avol->tbrow[q] + avol->tbframe[r];
	  l2 = avol->vd[m].label & LABELMASK;
	  
	  if (l1 != l2) {
	    c = scene->colors.label[l1];
	    
	    x = (int) (scale * ((double)i));
	    y = (int) (scale * ((double)j));
	    if (psz > 1)
	      CImgFillRect(*zy, x,y, psz, psz, c);
	    else
	      CImgSet(*zy, x, y, c);
	    
	    break;
	  }
	  
	} // for k
      } // for i, for j
  } // vmode == srcborder

  // paint seeds
  j = MapSize(avol->addseeds);
  for(i=0;i<j;i++) {
    k = MapGet(avol->addseeds, i, &p, &q);
    XAVolumeVoxelOf(avol,p,&v);
    if (v.x == fx) {
      c = scene->colors.label[q % 10];
      x = (int) (scale * ((double)(v.z)));
      y = (int) (scale * ((double)(v.y)));
      if (psz > 1)
	CImgFillRect(*zy, x,y, psz, psz, c);
      else
	CImgSet(*zy, x, y, c);      
    }
  }


  // XZ view
  sx = (int) ceil(scale * ((double) W));
  sy = (int) ceil(scale * ((double) D));

  if ( (*xz) != 0 ) {
    if ( (*xz)->W != sx || (*xz)->H != sy ) {
      CImgDestroy(*xz);
      *xz = 0;
    }
  }
  if ( *xz == 0 ) {
    *xz = CImgNew(sx, sy);
  }
  CImgFill(*xz, scene->colors.bg);

  for(j=0;j<D;j++)
    for(i=0;i<W;i++) {
      n = i + avol->tbrow[fy] + avol->tbframe[j];

      // underlying color
      switch(scene->options.src) {
      case src_orig:  c = ViewMapColor(vmap,avol->vd[n].orig);  break;
      case src_value: c = ViewMapColor(vmap,avol->vd[n].value); break;
      case src_cost:  c = ViewMapColor(vmap,avol->vd[n].cost);  break;
      default: c=0;
      }

      // mix with label color
      if (mode == vmode_srclabel) {
	d= scene->colors.label[(avol->vd[n].label&LABELMASK) % 10];
	if (avol->vd[n].label & REMOVEMASK)
	  d=inverseColor(d);
	c=mergeColorsRGB(c,d,1.0 - scene->options.srcmix);
      }

      x = (int) (scale * ((double)i));
      y = (int) (scale * ((double)j));
      if (psz > 1)
	CImgFillRect(*xz, x,y, psz, psz, c);
      else
	CImgSet(*xz, x, y, c);
    }

  // paint borders
  if (mode == vmode_srcborder) {
    for(j=0;j<D;j++)
      for(i=0;i<W;i++) {
	n = i + avol->tbrow[fy] + avol->tbframe[j];
	l1 = avol->vd[n].label & LABELMASK;
	
	for(k=1;k<A->n;k++) {
	  p = i  + A->dx[k];
	  q = fy + A->dy[k];
	  r = j  + A->dz[k];
	  
	  if (p < 0 || p >= W || q < 0 || q >= H || r < 0 || r >= D)
	    continue;
	  
	  m = p + avol->tbrow[q] + avol->tbframe[r];
	  l2 = avol->vd[m].label & LABELMASK;
	  
	  if (l1 != l2) {
	    c = scene->colors.label[l1];
	    
	    x = (int) (scale * ((double)i));
	    y = (int) (scale * ((double)j));
	    if (psz > 1)
	      CImgFillRect(*xz, x,y, psz, psz, c);
	    else
	      CImgSet(*xz, x, y, c);
	    
	    break;
	  }
	  
	} // for k
      } // for i, for j
  } // vmode == srcborder
  
  // paint seeds
  j = MapSize(avol->addseeds);
  for(i=0;i<j;i++) {
    k = MapGet(avol->addseeds, i, &p, &q);
    XAVolumeVoxelOf(avol,p,&v);
    if (v.y == fy) {
      c = scene->colors.label[q % 10];
      x = (int) (scale * ((double)(v.x)));
      y = (int) (scale * ((double)(v.z)));
      if (psz > 1)
	CImgFillRect(*xz, x,y, psz, psz, c);
      else
	CImgSet(*xz, x, y, c);      
    }
  }

  DestroyAdjRel(&A);
}

void MIPOverlay(Scene *scene) {
  float *lookup;
  Volume *src, *tmp;
  XAnnVolume *avol = scene->avol;
  int i,j,k,W,H,D,N,w,h;
  int maxval = 0;
  i16_t val;

  Transform Ta,Tb,Tc;
  Point Pa,Pb;
  float f, fmax;
  float r1, r2, rlen;

  int t0[3], t1[3];
  int c1,c2,y1,y2;

  if (!scene->avol) return;

  W = avol->W;
  H = avol->H;
  D = avol->D;
  N = avol->N;

  if (scene->mip_source == MIPSRC_OVERLAY && scene->mipvol != 0) {
    src = scene->mipvol;
    for(i=0;i<N;i++)
      if (src->u.i16[i] > maxval)
	maxval = src->u.i16[i];
  } else {
    src = VolumeNew(W,H,D, vt_integer_16);
    if (!src) return;

    switch(scene->mip_source) {
    case MIPSRC_ORIG:
    case MIPSRC_OVERLAY: /* user chose overlay but there isn't one */
      for(i=0;i<N;i++) {
	src->u.i16[i] = val = avol->vd[i].orig;
	if (val > maxval) maxval = val;
      }
      break;
    case MIPSRC_CURR:
      for(i=0;i<N;i++) {
	src->u.i16[i] = val = avol->vd[i].value;
	if (val > maxval) maxval = val;
      }
      break;
    case MIPSRC_COST:
      for(i=0;i<N;i++) {
	src->u.i16[i] = val = avol->vd[i].cost;
	if (val > maxval) maxval = val;
      }
      break;
    }
  }

  w = scene->pvt.xyzbuffer->W;
  h = scene->pvt.xyzbuffer->H;

  lookup = (float *) MemAlloc(sizeof(float) * 65536);
  if (!lookup) {
    VolumeDestroy(src);
    return;
  }

  for(i=0;i<65536;i++)
    lookup[i] = 0.0;

  r1 = scene->mip_ramp_min * ((float) maxval);
  r2 = scene->mip_ramp_max * ((float) maxval);
  rlen = r2 - r1;
  
  for(i=0;i<maxval;i++) {
    if (i < r1) lookup[i] = 0.0;
    else if (i > r2) lookup[i] = 1.0;
    else lookup[i] = (((float)i)-r1) / rlen;
  }

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);

  tmp = VolumeNew(w,h,1,vt_float_32);
  VolumeFillF(tmp, -1.0);

  t0[0] = -W / 2;
  t0[1] = -H / 2;
  t0[2] = -D / 2;

  t1[0] = w / 2;
  t1[1] = h / 2;
  t1[2] = 0;
  
  for(k=0;k<D;k++)
    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	Pa.x = i + t0[0];
	Pa.y = j + t0[1];
	Pa.z = k + t0[2];
	PointTransform(&Pb,&Pa,&Ta);
	Pb.x += t1[0];
	Pb.y += t1[1];
	Pb.z += t1[2];

	if (Pb.x < 0 || Pb.x >= w ||
	    Pb.y < 0 || Pb.y >= h) continue;

	if (Pb.z >= VolumeGetF32(scene->pvt.xyzbuffer,
				 (int) (Pb.x), (int) (Pb.y), 2))
	  continue;

	f = lookup[ VolumeGetI16(src, i, j, k) ];
	if (f > VolumeGetF32(tmp, Pb.x, Pb.y, 0)) {
	  VolumeSetF32(tmp, Pb.x, Pb.y, 0, f);
	}
      }

  /* find mip max */
  fmax = -1.0;
  for(i=0;i<tmp->N;i++)
    if (tmp->u.f32[i] > fmax) fmax = tmp->u.f32[i];

  k = ScenePixel(scene);

  if (scene->mip_lighten_only) {
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	f = VolumeGetF32(tmp,i,j,0);
	if (f < 0.0) continue;
	f = (255.0 * f) / fmax;
	
	c1 = gray((int) f);
	c2 = CImgGet(scene->vout, SceneX(scene, i), SceneY(scene,j));

	y1 = t0( RGB2YCbCr(c1) );
	y2 = t0( RGB2YCbCr(c2) );

	if (y1 < y2) continue;

	c1 = mergeColorsRGB(c1,c2,0.50);

	CImgFillRect(scene->vout, 
		     SceneX(scene,i),
		     SceneY(scene,j),
		     k,k, c1);
      }
  } else {
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	f = VolumeGetF32(tmp,i,j,0);
	if (f < 0.0) continue;
	f = (255.0 * f) / fmax;
	
	c1 = gray((int) f);
	c2 = CImgGet(scene->vout, SceneX(scene, i), SceneY(scene,j));
	c1 = mergeColorsRGB(c1,c2,0.50);
	CImgFillRect(scene->vout, 
		     SceneX(scene,i),
		     SceneY(scene,j),
		     k,k, c1);
      }
  }

  if (scene->mipvol == 0)
    VolumeDestroy(src);
  VolumeDestroy(tmp);
  MemFree(lookup);
}
