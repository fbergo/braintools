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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libip.h>
#include "ip_scene.h"
#include "ip_visualization.h"
#include "ip_mem.h"
#include "mvf.h"

static int schema[4][13] = {
  {0x208080, 0x000000, 0xffffff, 0x4040c0, 0x2020ff,
   0xc040c0, 0xff2020, 0xff8080, 0xffc0c0, 0xffff80,
   0xff8040, 0x208020, 0x80ff80},
  
  {0xcebb96, 0x000000, 0xffffff,
   0x4040c0, 0x772222, 0xa02e2e, 0xd63e3e, 0xf22929,
   0xefc2a0, 0xefd5a0, 0xefe4c3, 0xbcb3a2, 0x939c7f},

  {0x000000, 0xff0000, 0xff8000,
   0x505060, 0x000000, 0x202020, 0x404040, 0x606060,
   0x707070, 0x909090, 0xb0b0b0, 0xd0d0d0, 0xf0f0f0},

  {0xffffff, 0x000080, 0x800080,
   0x505060, 0x000000, 0x202020, 0x404040, 0x606060,
   0x707070, 0x909090, 0xb0b0b0, 0xd0d0d0, 0xf0f0f0}
};

Scene *SceneNew() {
  Scene *s;
  int i,j;
  
  s = (Scene *) MemAlloc(sizeof(Scene));
  if (!s) return 0;

  s->avol = 0;
  s->vout = 0;
  s->wantmesh = 1;

  PointAssign(&(s->rotation), 0.0, 0.0, 0.0);
  
  for(i=0;i<3;i++)
    for(j=0;j<2;j++)
      s->clip.v[i][j] = 0;

  s->options.src     = src_orig;
  s->options.vmode   = vmode_src;
  s->options.zoom    = zoom_100;
  s->options.shading = shading_auto;
  s->options.srcmix  = 0.30;

  s->options.phong.ambient  = 0.20;
  s->options.phong.diffuse  = 0.60;
  s->options.phong.specular = 0.40;
  s->options.phong.zgamma   = 0.25;
  s->options.phong.zstretch = 1.25;
  s->options.phong.sre      = 10;
  PointAssign(&(s->options.phong.light),0.0,0.0,-1.0);

  SceneLoadColorScheme(s,0);

  for(i=1;i<256;i++)
    s->hollow[i] = 0;
  s->hollow[0] = 1;

  s->pvt.vinfo     = 0;
  s->pvt.xyzbuffer = 0;
  s->pvt.surface   = 0;

  //  s->amesh.mtid = 0;
  s->amesh.srct  = 0;
  s->amesh.tmpt  = 0;
  s->amesh.tsort = 0;
  s->amesh.tz = 0;
  s->amesh.tl = 0;

  s->vmap = ViewMapNew();

  s->with_restriction    = 0;
  s->cost_threshold_min  = 0;
  s->cost_threshold_max  = 32767;
  s->restriction         = 0;

  s->enable_mip = 0;
  s->mip_lighten_only = 0;
  s->mip_ramp_min = 0.0;
  s->mip_ramp_max = 1.0;
  s->mipvol = 0;
  s->mip_source = MIPSRC_ORIG;

  s->invalid = 1;
  return s;
}

void SceneCopy(Scene *dest, Scene *src) {
  dest->avol = src->avol;
  dest->vout = 0;
  MemCpy(&(dest->rotation), &(src->rotation), sizeof(Vector));
  MemCpy(&(dest->clip), &(src->clip), sizeof(Clipping));
  MemCpy(&(dest->options), &(src->options), sizeof(RenderingOptions));
  MemCpy(&(dest->colors), &(src->colors), sizeof(SceneColors));
  MemCpy(dest->hollow, src->hollow, 256 * sizeof(char));

  dest->invalid = 1;

  dest->amesh.srct = 0;
  dest->amesh.tmpt = 0;
  dest->amesh.tsort = 0;
  dest->wantmesh = src->wantmesh;

  MemCpy(&(dest->pvt), &(src->pvt), sizeof(ScenePvt));

  dest->pvt.vinfo     = 0;
  dest->pvt.xyzbuffer = 0;
  dest->pvt.surface   = 0;

  ViewMapCopy(dest->vmap, src->vmap);

  dest->with_restriction    = src->with_restriction;
  dest->cost_threshold_min  = src->cost_threshold_min;
  dest->cost_threshold_max  = src->cost_threshold_max;
  dest->restriction         = src->restriction;

  dest->eshade = src->eshade;
  dest->enable_mip = src->enable_mip;
  dest->mip_lighten_only = src->mip_lighten_only;
  dest->mip_ramp_min = src->mip_ramp_min;
  dest->mip_ramp_max = src->mip_ramp_max;
  dest->mipvol = src->mipvol;
  dest->mip_source = src->mip_source;
}

void SceneDestroy(Scene *s) {
  if (s->pvt.vinfo)
    VolumeDestroy(s->pvt.vinfo);
  if (s->pvt.xyzbuffer)
    VolumeDestroy(s->pvt.xyzbuffer);
  if (s->pvt.surface)
    VolumeDestroy(s->pvt.surface);
  if (s->vmap)
    ViewMapDestroy(s->vmap);
  if (s->mipvol)
    VolumeDestroy(s->mipvol);
  MemFree(s);
}

void SceneValidate(Scene *s) {
  int i,j;
  
  VectorCopy(&(s->pvt.rotation),&(s->rotation));

  for(i=0;i<3;i++)
    for(j=0;j<2;j++)
      s->pvt.clip.v[i][j] = s->clip.v[i][j];

  MemCpy(&(s->pvt.popts),&(s->options),sizeof(RenderingOptions));
  MemCpy(&(s->pvt.pcolors),&(s->colors),sizeof(SceneColors));
  MemCpy(s->pvt.hollow,s->hollow,256);

  s->invalid = 0;
}

int  SceneX(Scene *scene, int x) {
  switch(scene->options.zoom) {
  case zoom_100:  return x;
  case zoom_150:  return(scene->pvt.oxc+(int)((1.50)*((double)(x-scene->pvt.oxc))));
  case zoom_200:  return(scene->pvt.oxc+2*(x-scene->pvt.oxc));
  case zoom_300:  return(scene->pvt.oxc+3*(x-scene->pvt.oxc));
  case zoom_400:  return(scene->pvt.oxc+4*(x-scene->pvt.oxc));
  case zoom_50:   return(scene->pvt.oxc+(x-scene->pvt.oxc)/2);
  default: return 0;
  }
}

int  SceneY(Scene *scene, int y) {
  switch(scene->options.zoom) {
  case zoom_100:  return y;
  case zoom_150:  return(scene->pvt.oyc+(int)((1.50)*((double)(y-scene->pvt.oyc))));
  case zoom_200:  return(scene->pvt.oyc+2*(y-scene->pvt.oyc));
  case zoom_300:  return(scene->pvt.oyc+3*(y-scene->pvt.oyc));
  case zoom_400:  return(scene->pvt.oyc+4*(y-scene->pvt.oyc));
  case zoom_50:   return(scene->pvt.oyc+(y-scene->pvt.oyc)/2);
  default: return 0;
  }
}

int  SceneW(Scene *scene, int w) {
  switch(scene->options.zoom) {
  case zoom_100:  return w;
  case zoom_150:  return(w+w/2);
  case zoom_200:  return(2*w);
  case zoom_300:  return(3*w);
  case zoom_400:  return(4*w);
  case zoom_50:   return(w/2);
  default: return 0;
  }
}

int  SceneH(Scene *scene, int h) {
  switch(scene->options.zoom) {
  case zoom_100:  return h;
  case zoom_150:  return(h+h/2);
  case zoom_200:  return(2*h);
  case zoom_300:  return(3*h);
  case zoom_400:  return(4*h);
  case zoom_50:   return(h/2);
  default: return 0;
  }
}

int  SceneInvX(Scene *scene, int x) {  
  switch(scene->options.zoom) {
  case zoom_100:  return x;
  case zoom_150:  return( (int) (scene->pvt.oxc + ((double)(x-scene->pvt.oxc))/1.50) );
  case zoom_200:  return(scene->pvt.oxc+(x-scene->pvt.oxc)/2);
  case zoom_300:  return(scene->pvt.oxc+(x-scene->pvt.oxc)/3);
  case zoom_400:  return(scene->pvt.oxc+(x-scene->pvt.oxc)/4);
  case zoom_50:   return(scene->pvt.oxc+(x-scene->pvt.oxc)*2);
  default: return 0;
  }
}

int  SceneInvY(Scene *scene, int y) {
  switch(scene->options.zoom) {
  case zoom_100:  return y;
  case zoom_150:  return( (int) (scene->pvt.oyc + ((double)(y-scene->pvt.oyc))/1.50) );
  case zoom_200:  return(scene->pvt.oyc+(y-scene->pvt.oyc)/2);
  case zoom_300:  return(scene->pvt.oyc+(y-scene->pvt.oyc)/3);
  case zoom_400:  return(scene->pvt.oyc+(y-scene->pvt.oyc)/4);
  case zoom_50:   return(scene->pvt.oyc+(y-scene->pvt.oyc)*2);
  default: return 0;
  }
}

int  ScenePixel(Scene *scene) {
  switch(scene->options.zoom) {
  case zoom_100: return 1;
  case zoom_150: return 2;
  case zoom_200: return 2;
  case zoom_300: return 3;
  case zoom_400: return 4;
  case zoom_50:  return 1;
  default: return 0;
  }
}

double  SceneScale(Scene *scene) {
  switch(scene->options.zoom) {
  case zoom_100: return 1.00;
  case zoom_150: return 1.50;
  case zoom_200: return 2.00;
  case zoom_300: return 3.00;
  case zoom_400: return 4.00;
  case zoom_50:  return 0.50;
  default: return 0.0;
  }
}

int SceneVoxelAt(Scene *scene, int x, int y) {
  int rx, ry;
  rx = SceneInvX(scene, x);
  ry = SceneInvY(scene, y);
  if (rx<0 || rx>=scene->pvt.outw) return VLARGE;
  if (ry<0 || ry>=scene->pvt.outh) return VLARGE;
  return( VolumeGetI32(scene->pvt.vinfo, rx, ry, NPROJ) );
}

int SceneVoxelAtT(Scene *scene, int x, int y) {
  if (x<0 || x>=scene->pvt.outw) return VLARGE;
  if (y<0 || y>=scene->pvt.outh) return VLARGE;
  return( VolumeGetI32(scene->pvt.vinfo, x, y, NPROJ) );
}

void SceneLoadColorScheme(Scene *scene, int scheme) {
  
  int i,j=0;
  
  switch(scheme) {
  case SCHEME_DEFAULT:   j=0; break;
  case SCHEME_BONEFLESH: j=1; break;
  case SCHEME_GSBLACK:   j=2; break;
  case SCHEME_GSWHITE:   j=3; break;
  }

  for(i=0;i<256;i++)
    scene->colors.label[i] = 0;
  for(i=0;i<10;i++)
    scene->colors.label[i] = schema[j][i+3];

  scene->colors.bg       = schema[j][0];
  scene->colors.clipcube = schema[j][2];
  scene->colors.fullcube = schema[j][1];
  scene->colors.whiteborder = 0;
}

void SceneRandomColorScheme(Scene *scene) {
  int i;

  for(i=0;i<256;i++)
    scene->colors.label[i] = 0;
  for(i=0;i<10;i++)
    scene->colors.label[i] = randomColor();
  
  scene->colors.bg       = randomColor();
  scene->colors.clipcube = randomColor();
  scene->colors.fullcube = randomColor();
  scene->colors.whiteborder = 0;
}

void SceneSetVolume(Scene *scene, XAnnVolume *v) {
  if (v!=scene->avol) {
    scene->invalid = 1;
    scene->avol = v;
  }
}

void ScenePrepareImage(Scene *scene, int w, int h) {
  if (scene->vout) {
    if (scene->vout->W == w && scene->vout->H == h)
      return;
    CImgDestroy(scene->vout);
  }

  scene->vout = CImgNew(w,h);
  scene->pvt.outw = w;
  scene->pvt.outh = h;
  scene->pvt.oxc  = w/2;
  scene->pvt.oyc  = h/2;
}

void SceneCheckValidity(Scene *s) {
  int i,j;

  if (s->rotation.x != s->pvt.rotation.x ||
      s->rotation.y != s->pvt.rotation.y ||
      s->rotation.z != s->pvt.rotation.z)
    goto out_invalid;
    
  for(i=0;i<3;i++)
    for(j=0;j<2;j++)
      if (s->clip.v[i][j] != s->pvt.clip.v[i][j])
	goto out_invalid;

  if (memcmp(&(s->options),&(s->pvt.popts),sizeof(RenderingOptions))!=0)
    goto out_invalid;

  if (memcmp(&(s->colors),&(s->pvt.pcolors),sizeof(SceneColors))!=0)
    goto out_invalid;

  for(i=0;i<256;i++)
    if (s->hollow[i] != s->pvt.hollow[i])
      goto out_invalid;

  return;
 out_invalid:
  s->invalid = 1; 
}

int SceneMarkSeed(Scene *scene, int x, int y, int z, int label) {
  int n;
  int i,j,w,h,ps;

  if (!scene->avol) return 0;
  if (x < 0 || x >= scene->avol->W ||
      y < 0 || y >= scene->avol->H ||
      z < 0 || z >= scene->avol->D) return 0;
      
  n = x + scene->avol->tbrow[y] + scene->avol->tbframe[z];
  MapSet(scene->avol->addseeds, n, label);

  // paint on 3D projection
  if (scene->vout && scene->pvt.vinfo) {
    w = scene->pvt.vinfo->W;
    h = scene->pvt.vinfo->H;
    ps = ScenePixel(scene);
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	if (VolumeGetI32(scene->pvt.vinfo, i,j, NPROJ) == n) {
	  VolumeSetI32(scene->pvt.vinfo, i,j, SEED, label);
	  CImgFillRect(scene->vout,i,j,ps,ps,scene->colors.label[label&0xff]);
	}
      }
  }

  return 1;
}

int SceneMarkSeedOnProjection(Scene *scene, int x,int y, int label) {
  int n;
  int ix, iy;
  int ps;

  if (!scene->avol) return 0;

  n = SceneVoxelAt(scene, x, y);
  if (n==VLARGE) return 0;

  ix = SceneInvX(scene, x);
  iy = SceneInvY(scene, y);

  VolumeSetI32(scene->pvt.vinfo, ix, iy, SEED, label);
  MapSet(scene->avol->addseeds, n, label);

  if (scene->vout) {
    ps = ScenePixel(scene);
    CImgFillRect(scene->vout, SceneX(scene, ix), SceneY(scene, iy), ps, ps,
		 scene->colors.label[label&0xff]);
  }
  
  return 1;
}

int SceneMarkSeedLine(Scene *scene, int x0, int y0, int z0, 
		      int x1,int y1,int z1, int label)
{
  int hx,hy,hz,c=0;
  if (!scene->avol) return 0;

  c = SceneMarkSeed(scene, x0, y0, z0, label);
  c |= SceneMarkSeed(scene, x1, y1, z1, label);

  hx = (x0 + x1) / 2;
  hy = (y0 + y1) / 2;
  hz = (z0 + z1) / 2;

  if ( (hx==x0 && hy==y0 && hz==z0) ||
       (hx==x1 && hy==y1 && hz==z1) )
    return c;

  return(c|SceneMarkSeedLine(scene,x0,y0,z0,hx,hy,hz,label)|
	 SceneMarkSeedLine(scene,hx,hy,hz,x1,y1,z1,label));
}

int SceneMarkSeedLineOnProjection(Scene *scene, int x0,int y0, 
				  int x1, int y1,int label)
{
  int hx, hy, c=0;
  if (!scene->avol) return 0;

  c=SceneMarkSeedOnProjection(scene, x0, y0, label);
  c|=SceneMarkSeedOnProjection(scene, x1, y1, label);

  hx = (x0 + x1) / 2;
  hy = (y0 + y1) / 2;

  if ( (hx == x0 && hy == y0)  || (hx == x1 && hy == y1) )
    return c;

  return(c|SceneMarkSeedLineOnProjection(scene,x0,y0,hx,hy,label)|
	 SceneMarkSeedLineOnProjection(scene,hx,hy,x1,y1,label));
}

int SceneMarkTreeOnProjection(Scene *scene, int x,int y) 
{
  int n,root;
  XAnnVolume *avol = scene->avol;

  if (!avol) return 0;

  n = SceneVoxelAt(scene,x,y);
  if (n==VLARGE) return 0;

  root = XAVolGetRootN(avol, n);
  XAVolToggleRemoval(avol, root);

  if (avol->vd[root].label & REMOVEMASK)
    MapSet(avol->delseeds, root, LABELMASK & avol->vd[root].label);
  else
    MapRemove(avol->delseeds, root);

  return 1;
}

int  SceneMarkTree(Scene *scene, int x,int y,int z) {
  int n,root;
  XAnnVolume *avol = scene->avol;

  if (!avol) return 0;

  if (x < 0 || x >= avol->W ||
      y < 0 || y >= avol->H ||
      z < 0 || z >= avol->D) return 0;
      
  n = x + avol->tbrow[y] + avol->tbframe[z];

  root = XAVolGetRootN(avol, n);
  XAVolToggleRemoval(avol, root);

  if (avol->vd[root].label & REMOVEMASK)
    MapSet(avol->delseeds, root, LABELMASK & avol->vd[root].label);
  else
    MapRemove(avol->delseeds, root);

  return 1;
}


ROI * ROINew() {
  ROI *x;
  int i;

  x = (ROI *) MemAlloc(sizeof(ROI));
  if (!x) return 0;

  for(i=0;i<256;i++)
    x->labelroi[i] = 1;
  x->clip_only = 0;
  return x;
}

void ROIDestroy(ROI *x) {
  if (x)
    MemFree(x);
}

void SceneSaveView(Scene *scene, char *path) {
  MVF_File *mvf;
  int i,j,k;

  if (!mvf_check_format(path))
    return;
  mvf = mvf_open_rw(path);

  if (!mvf)
    return;

  j = mvf_lookup_chunk(mvf, "VIEW");
  if (j<0) {
    mvf_close(mvf);
    return;
  }
  mvf_goto(mvf, j, 0);

  mvf_write_string(mvf, 8, "rot");
  mvf_write_int_string(mvf, 8, 8*3);
  mvf_write_float_string(mvf, 8, scene->rotation.x);
  mvf_write_float_string(mvf, 8, scene->rotation.y);
  mvf_write_float_string(mvf, 8, scene->rotation.z);

  mvf_write_string(mvf, 8, "clip");
  mvf_write_int_string(mvf, 8, 8*6);
  for(i=0;i<3;i++)
    for(k=0;k<2;k++)
      mvf_write_int_string(mvf, 8, scene->clip.v[i][k]);

  mvf_write_string(mvf, 8, "opt");
  mvf_write_int_string(mvf, 8, 8*14);

  mvf_write_int_string(mvf, 8, (int) (scene->options.src));
  mvf_write_int_string(mvf, 8, (int) (scene->options.vmode));
  mvf_write_int_string(mvf, 8, (int) (scene->options.zoom));
  mvf_write_int_string(mvf, 8, (int) (scene->options.shading));
  
  mvf_write_float_string(mvf, 8, scene->options.phong.ambient);
  mvf_write_float_string(mvf, 8, scene->options.phong.diffuse);
  mvf_write_float_string(mvf, 8, scene->options.phong.specular);
  mvf_write_float_string(mvf, 8, scene->options.phong.zgamma);
  mvf_write_float_string(mvf, 8, scene->options.phong.zstretch);
  mvf_write_int_string(mvf, 8, scene->options.phong.sre);
  mvf_write_float_string(mvf, 8, scene->options.phong.light.x);
  mvf_write_float_string(mvf, 8, scene->options.phong.light.y);
  mvf_write_float_string(mvf, 8, scene->options.phong.light.z);

  mvf_write_float_string(mvf, 8, (int) (scene->options.srcmix));

  mvf_write_string(mvf, 8, "clr");
  mvf_write_int_string(mvf, 8, (12*(3+256))+4);

  mvf_write_int_string(mvf, 12, scene->colors.bg);
  mvf_write_int_string(mvf, 12, scene->colors.clipcube);
  mvf_write_int_string(mvf, 12, scene->colors.fullcube);

  for(i=0;i<256;i++)
    mvf_write_int_string(mvf, 12, scene->colors.label[i]);

  mvf_write_int_string(mvf, 4, scene->colors.whiteborder);

  mvf_write_string(mvf, 8, "hollow");
  mvf_write_int_string(mvf, 8, 2*256);
  for(i=0;i<256;i++)
    mvf_write_int_string(mvf, 2, scene->hollow[i]);

  mvf_write_string(mvf, 8, "end");

  mvf_close(mvf);
}

void SceneLoadView(Scene *scene, char *path) {
  MVF_File *mvf;
  int i,j,k;
  char key[12];
  int  L;

  if (!mvf_check_format(path))
    return;
  mvf = mvf_open(path);

  if (!mvf)
    return;

  j = mvf_lookup_chunk(mvf, "VIEW");
  if (j<0) {
    mvf_close(mvf);
    return;
  }

  mvf_goto(mvf, j, 0);

  for(;;) {
    mvf_read_string(mvf, 8, key);
    if (key[0] == 0)
      break;
    if (!strcmp(key, "end"))
      break;
    L = mvf_read_int_string(mvf, 8);

    if (!strcmp(key, "rot")) {
      scene->rotation.x = mvf_read_float_string(mvf, 8);
      scene->rotation.y = mvf_read_float_string(mvf, 8);
      scene->rotation.z = mvf_read_float_string(mvf, 8);
      continue;
    }

    if (!strcmp(key, "clip")) {
      for(i=0;i<3;i++)
	for(k=0;k<2;k++)
	  scene->clip.v[i][k] = mvf_read_int_string(mvf, 8);
      continue;
    }

    if (!strcmp(key, "opt")) {
      scene->options.src      = (SrcType)     mvf_read_int_string(mvf ,8);
      scene->options.vmode    = (VModeType)   mvf_read_int_string(mvf ,8);
      scene->options.zoom     = (ZoomType)    mvf_read_int_string(mvf ,8);
      scene->options.shading  = (ShadingType) mvf_read_int_string(mvf ,8);
      scene->options.phong.ambient  = mvf_read_float_string(mvf, 8);
      scene->options.phong.diffuse  = mvf_read_float_string(mvf, 8);
      scene->options.phong.specular = mvf_read_float_string(mvf, 8);
      scene->options.phong.zgamma   = mvf_read_float_string(mvf, 8);
      scene->options.phong.zstretch = mvf_read_float_string(mvf, 8);
      scene->options.phong.sre      = mvf_read_int_string(mvf, 8);
      scene->options.phong.light.x  = mvf_read_float_string(mvf, 8);
      scene->options.phong.light.y  = mvf_read_float_string(mvf, 8);
      scene->options.phong.light.z  = mvf_read_float_string(mvf, 8);
      scene->options.srcmix = mvf_read_float_string(mvf, 8);
      continue;
    }

    if (!strcmp(key, "clr")) {
      scene->colors.bg        = mvf_read_int_string(mvf, 12);
      scene->colors.clipcube  = mvf_read_int_string(mvf, 12);
      scene->colors.fullcube  = mvf_read_int_string(mvf, 12);
      for(i=0;i<256;i++)
	scene->colors.label[i] = mvf_read_int_string(mvf, 12);
      scene->colors.whiteborder = mvf_read_int_string(mvf, 4);
      continue;
    }

    if (!strcmp(key, "hollow")) {
      for(i=0;i<256;i++)
	scene->hollow[i] = mvf_read_int_string(mvf, 2);
      continue;
    }

    mvf_skip(mvf, L);
  }

  mvf_close(mvf);
  scene->invalid = 1;
}


/*
      hgrid = 4
    .__.  .  .  .
    | /|
    ./_.  .  .  .
                  vgrid = 3
    .  .  .  .  .

    .  .  .  .  .

    maxtriangles = hgrid * vgrid * 2

*/

void SceneInitAMesh(Scene *scene, int hgrid, int vgrid) {
  scene->amesh.validmesh = 0;
  if (scene->amesh.srct  != 0) MemFree(scene->amesh.srct);
  if (scene->amesh.tmpt  != 0) MemFree(scene->amesh.tmpt);
  if (scene->amesh.tsort != 0) MemFree(scene->amesh.tsort);
  if (scene->amesh.tz    != 0) MemFree(scene->amesh.tz);
  if (scene->amesh.tl    != 0) MemFree(scene->amesh.tl);
  
  scene->amesh.hgrid = hgrid;
  scene->amesh.vgrid = vgrid;
  scene->amesh.maxtriangles = hgrid * vgrid * 2;

  scene->amesh.ntriangles = 0;

  scene->amesh.srct = (Triangle *) MemAlloc(sizeof(Triangle) * 
					  scene->amesh.maxtriangles);
  scene->amesh.tmpt = (Triangle *) MemAlloc(sizeof(Triangle) * 
					  scene->amesh.maxtriangles);
  scene->amesh.tsort = (int *) MemAlloc(sizeof(int) *
				      scene->amesh.maxtriangles);
  scene->amesh.tz = (float *) MemAlloc(sizeof(float) *
				   scene->amesh.maxtriangles);
  scene->amesh.tl = (ltype *) MemAlloc(sizeof(ltype) *
				   scene->amesh.maxtriangles);
}

void SceneShrinkAMesh(Scene *scene) {

  if (scene->amesh.validmesh) {
    scene->amesh.srct = (Triangle *) realloc(scene->amesh.srct,
					     sizeof(Triangle) * 
					     scene->amesh.ntriangles);
    scene->amesh.tmpt = (Triangle *) realloc(scene->amesh.tmpt,
					     sizeof(Triangle) * 
					     scene->amesh.ntriangles);
    scene->amesh.tsort = (int *) realloc(scene->amesh.tsort,
					 sizeof(int) *
					scene->amesh.ntriangles);
    scene->amesh.tz = (float *) realloc(scene->amesh.tz,
					sizeof(float) *
				       scene->amesh.ntriangles);
    scene->amesh.tl = (ltype *) realloc(scene->amesh.tl,
					sizeof(ltype) *
					scene->amesh.ntriangles);

  } else {
    if (scene->amesh.srct  != 0) MemFree(scene->amesh.srct);
    if (scene->amesh.tmpt  != 0) MemFree(scene->amesh.tmpt);
    if (scene->amesh.tsort != 0) MemFree(scene->amesh.tsort);
    if (scene->amesh.tz    != 0) MemFree(scene->amesh.tz);
    if (scene->amesh.tl    != 0) MemFree(scene->amesh.tl);
    scene->amesh.srct  = 0;
    scene->amesh.tmpt  = 0;
    scene->amesh.tsort = 0;
    scene->amesh.tz    = 0;
    scene->amesh.tl    = 0;
  }
}

/* ===================================================== */

ViewMap *ViewMapNew() {
  ViewMap *vmap;

  vmap = (ViewMap *) MemAlloc(sizeof(ViewMap));
  if (!vmap) return 0;
  
  vmap->maxval   = 32767.0;
  vmap->maxlevel = 1.0;
  vmap->minlevel = 0.0;

  vmap->map = (unsigned char *) MemAlloc(32768);
  
  if (!vmap->map) {
    MemFree(vmap);
    return 0;
  }

  ViewMapLoadPalette(vmap, VMAP_GRAY);
  ViewMapUpdate(vmap);
  return vmap;
}

void     ViewMapDestroy(ViewMap *vmap) {
  MemFree(vmap->map);
  MemFree(vmap);
}

void ViewMapLoadPalette(ViewMap *vmap, int p) {
  int i;
  switch(p) {

  case VMAP_GRAY:
    for(i=0;i<256;i++)
      vmap->palette[i]=gray(i);
    break;

  case VMAP_HOTMETAL:
    for(i=0;i<256;i++)
      vmap->palette[i] = triplet(MIN(255.0*i/182.0,255.0),
				 MAX(MIN(255.0*(i-128.0)/91.0,255.0),0),
				 MAX(MIN(255.0*(i-192.0)/64.0,255.0),0));
    break;

  case VMAP_SPECTRUM:
    for(i=0;i<256;i++)
      vmap->palette[i] = HSV2RGB(triplet(180-180.0*(((float)i)/255.0),
					 255,255));
    break;

  case VMAP_SPECTRUM_B:
    for(i=0;i<256;i++)
      vmap->palette[i] = HSV2RGB(triplet(180-180.0*(((float)i)/255.0),
					 255,255));
    for(i=0;i<256;i++)
      vmap->palette[i] = mergeColorsRGB(0,vmap->palette[i],
					0.10+((float)i)/300.0);
    break;

  case VMAP_YELLOW_RED:
    for(i=0;i<256;i++)
      vmap->palette[i] = mergeColorsRGB(0x400000,0xffff80,i/255.0);
    break;

  case VMAP_NEON:
    for(i=0;i<256;i++)
      vmap->palette[i] = (i<128 ? mergeColorsRGB(0x000000,0x00ff00,i/128.0) :
			  mergeColorsRGB(0x00ff00,0xffffff,(i-128.0)/128.0));
    break;
  }
}

void ViewMapUpdate(ViewMap *vmap) {
  int i,j,k;
  float vmin, vmax, delta, ratio;

  if (vmap->minlevel >= vmap->maxlevel)
    vmap->minlevel = 0.0;

  vmin = vmap->maxval * vmap->minlevel;
  vmax = vmap->maxval * vmap->maxlevel;

  delta = vmax - vmin;
  if (delta > 0.001)
    ratio = 255.0 / delta;
  else
    ratio = 0.0;

  for(i=((int)vmin);i>=0;i--)
    vmap->map[i] = 0;
  for(i=((int)vmax);i<32768;i++)
    vmap->map[i] = 255;
  
  j = (int) vmin;
  k = (int) vmax;

  for(i=j+1;i<k;i++)
    vmap->map[i] = (unsigned char) (ratio * (i-vmin));
}

void ViewMapCopy(ViewMap *dest, ViewMap *src) {
  dest->maxval   = src->maxval;
  dest->maxlevel = src->maxlevel;
  dest->minlevel = src->minlevel;

  MemCpy(dest->palette, src->palette, 256 * sizeof(int));
  MemCpy(dest->map, src->map, 32768);
}
