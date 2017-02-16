
#define SOURCE_VIEW3D_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libip.h>
#include <math.h>
#include <gtk/gtk.h>
#include "aftervoxel.h"

View3D v3d;
OctantCube ocube;

void view3d_init() {
  int i;

  v3d.invalid = 1;
  TransformIdentity(&(v3d.rotation));
  v3d.base = 0;
  v3d.scaled = 0;

  for(i=0;i<3;i++) {
    v3d.sbase[i] = 0;
    v3d.sscaled[i] = 0;
  }

  v3d.wireframe = 0;

  v3d.show_measures = 1;

  v3d.zbuf = 0;
  v3d.zlen  = 0;
  v3d.szbuf = 0;

  v3d.panx = 0.0;
  v3d.pany = 0.0;
  v3d.zoom = 1.0;

  v3d.savepanx = 0.0;
  v3d.savepany = 0.0;

  VectorAssign(&(v3d.light), 0.0,0.0,-1.0);
  v3d.ka = 0.20;
  v3d.kd = 0.60;
  v3d.ks = 0.40;
  v3d.sre = 10;
  v3d.zgamma = 0.25;
  v3d.zstretch = 1.25;

  v3d.skin.invalid = 1;
  v3d.skin.out = 0;
  v3d.skin.zbuf = 0;
  v3d.skin.ebuf = 0;
  v3d.skin.zlen = 0;
  v3d.skin.tmap = 0;
  v3d.skin.tcount = 0;
  v3d.skin.enable = 1;
  v3d.skin.opacity = 1.0;
  v3d.skin.value = 400;
  v3d.skin.color = 0xe2c3ae;
  v3d.skin.smoothing = 1;
  v3d.skin.ready    = 0;

  v3d.octant.invalid  = 1;
  v3d.octant.out      = 0;
  v3d.octant.zbuf     = 0;
  v3d.octant.enable   = 0;
  v3d.octant.usedepth = 0;
  v3d.octant.ready    = 0;
  v3d.octant.zlen     = 0;

  v3d.obj.invalid = 1;
  v3d.obj.out     = 0;
  v3d.obj.zbuf    = 0;
  v3d.obj.enable  = 1;
  v3d.obj.opacity = 1.0;
  v3d.obj.ready   = 0;
  v3d.obj.zlen    = 0;

  v3d.measure_color = 0x4040cc;
  v3d.georuler_color = 0xff1500;

  v3d.skin.lock = MutexNew();
  v3d.octant.lock = MutexNew();
  v3d.obj.lock = MutexNew();
}

void view3d_reset() {
  v3d.invalid = 1;
  TransformIdentity(&(v3d.rotation));

  v3d.wireframe = 0;

  v3d.panx = 0.0;
  v3d.pany = 0.0;
  v3d.zoom = 1.0;

  v3d.savepanx = 0.0;
  v3d.savepany = 0.0;

  VectorAssign(&(v3d.light), 0.0,0.0,-1.0);
  v3d.ka = 0.20;
  v3d.kd = 0.60;
  v3d.ks = 0.40;
  v3d.sre = 10;
  v3d.zgamma = 0.25;
  v3d.zstretch = 1.25;

  v3d.skin.invalid = 1;
  v3d.skin.enable = 1;
  v3d.skin.opacity = 1.0;
  v3d.skin.value = 400;
  v3d.skin.color = 0xffddbb;
  v3d.skin.smoothing = 1;
  v3d.skin.ready    = 0;

  v3d.octant.invalid  = 1;
  v3d.octant.usedepth = 0;
  v3d.octant.ready    = 0;
  v3d.octant.enable   = 0;

  v3d.obj.invalid = 1;
  v3d.obj.enable  = 1;
  v3d.obj.opacity = 1.0;
  v3d.obj.ready   = 0;
}

void view3d_set_gui() {
  CheckBoxSetState(gui.skin, v3d.skin.enable);
  CheckBoxSetState(gui.oct , v3d.octant.enable);
  CheckBoxSetState(gui.obj,  v3d.obj.enable);
  SliderSetValue(gui.oskin, v3d.skin.opacity);
  SliderSetValue(gui.oobj,  v3d.obj.opacity);
}

void view3d_alloc(int what,int dw,int dh) {
  int zlen;

  zlen = dw*dh;

  if (what&M_SKIN) {

    if (v3d.skin.out) {
      if (!CImgSizeEq(v3d.skin.out,dw,dh)) {
	CImgDestroy(v3d.skin.out);
	v3d.skin.out = 0;
      }
    }
    if (!v3d.skin.out) v3d.skin.out = CImgNew(dw,dh);

    if (v3d.skin.zlen != zlen) {
      if (v3d.skin.zbuf) {
	MemFree(v3d.skin.zbuf);
	v3d.skin.zbuf = 0;
      }
      if (v3d.skin.ebuf) {
	MemFree(v3d.skin.ebuf);
	v3d.skin.ebuf = 0;
      }
    }

    if (!v3d.skin.zbuf)
      v3d.skin.zbuf = (float *) MemAlloc(sizeof(float) * zlen);
    if (!v3d.skin.ebuf)
      v3d.skin.ebuf = (float *) MemAlloc(sizeof(float) * zlen);
    v3d.skin.zlen = zlen;

  }

  if (what&M_OCT) {
    if (v3d.octant.out) {
      if (!CImgSizeEq(v3d.octant.out,dw,dh)) {
	CImgDestroy(v3d.octant.out);
	v3d.octant.out = 0;
      }
    }
    if (!v3d.octant.out) v3d.octant.out = CImgNew(dw,dh);

    if (v3d.octant.zlen != zlen) {
      if (v3d.octant.zbuf) {
	MemFree(v3d.octant.zbuf);
	v3d.octant.zbuf = 0;
      }
    }
    if (!v3d.octant.zbuf)
      v3d.octant.zbuf = (float *) MemAlloc(sizeof(float) * zlen);
    v3d.octant.zlen = zlen;
  }

  if (what&M_OBJ) {
    if (v3d.obj.out) {
      if (!CImgSizeEq(v3d.obj.out,dw,dh)) {
	CImgDestroy(v3d.obj.out);
	v3d.obj.out = 0;
      }
    }
    if (!v3d.obj.out) v3d.obj.out = CImgNew(dw,dh);

    if (v3d.obj.zlen != zlen) {
      if (v3d.obj.zbuf) {
	MemFree(v3d.obj.zbuf);
	v3d.obj.zbuf = 0;
      }
    }
    if (!v3d.obj.zbuf)
      v3d.obj.zbuf = (float *) MemAlloc(sizeof(float) * zlen);
    v3d.obj.zlen = zlen;
  }

}

void view3d_compose(int w,int h,int bg) {
  int i,dw,dh;
  float b;
  int *lookup;

  if (v3d.scaled) {
    CImgDestroy(v3d.scaled);
    v3d.scaled = 0;
  }

  if (v3d.sbase[0])
    for(i=0;i<3;i++) {
      CImgDestroy(v3d.sbase[i]);
      v3d.sbase[i] = 0;
    }
  if (v3d.sscaled[0])
    for(i=0;i<3;i++) {
      CImgDestroy(v3d.sscaled[i]);
      v3d.sscaled[i] = 0;
    }

  dw = 2 + (int) (((float)w) / v3d.zoom);
  dh = 2 + (int) (((float)h) / v3d.zoom);

  if (v3d.base)
    if (v3d.base->W != dw || v3d.base->H != dh) {
      CImgDestroy(v3d.base);
      v3d.base = 0;
      v3d.skin.invalid = 1;
      v3d.octant.invalid = 1;
      v3d.obj.invalid = 1;
    }

  if (!v3d.base)
    v3d.base = CImgNew(dw,dh);
  if (!v3d.base) {
    error_memory();
    return;
  }

  CImgFill(v3d.base, bg);

  if (voldata.original != NULL) {

    if (!v3d.wireframe) {

      if (v3d.zbuf && v3d.zlen!= dw*dh) {
	MemFree(v3d.zbuf);
	v3d.zbuf = 0;
	v3d.zlen = 0;
      } 

      if (!v3d.zbuf) {
	v3d.zlen = dw*dh;
	v3d.zbuf = (float *) MemAlloc(v3d.zlen * sizeof(float));
	if (!v3d.zbuf) {
	  error_memory();
	  return;
	}
      }

      /* avoid rendering bad data */

      if (v3d.skin.ready)
	if (!CImgSizeEq(v3d.skin.out,dw,dh))
	  v3d.skin.ready = 0;
      if (v3d.octant.ready)
	if (!CImgSizeEq(v3d.octant.out,dw,dh))
	  v3d.octant.ready = 0;
      if (v3d.obj.ready)
	if (!CImgSizeEq(v3d.obj.out,dw,dh))
	  v3d.obj.ready = 0;

      /* render everything that we have ready */

      if (v3d.skin.ready && v3d.skin.enable) {

	CImgBitBlm(v3d.base, 0,0,
		   v3d.skin.out, 0,0,
		   v3d.skin.out->W, v3d.skin.out->H, v3d.skin.opacity);

	MemCpy(v3d.zbuf,v3d.skin.zbuf,v3d.zlen * sizeof(float));

      }

      if (v3d.octant.ready && v3d.octant.enable) {

	if (v3d.skin.enable && v3d.skin.ready) {
	  view3d_overlay_skin_octant();
	} else {

	  CImgBitBlm(v3d.base, 0,0,
		     v3d.octant.out, 0,0,
		     v3d.octant.out->W, v3d.octant.out->H,
		     v3d.skin.opacity);
	  MemCpy(v3d.zbuf,v3d.octant.zbuf,v3d.zlen * sizeof(float));

	}

      }

      if (v3d.obj.ready && v3d.obj.enable) {

	if ( (v3d.skin.enable && v3d.skin.ready) ||
	     (v3d.octant.enable && v3d.octant.ready) ) {
	  view3d_overlay_obj();
	} else {
	  CImgBitBlm(v3d.base, 0,0,
		     v3d.obj.out, 0,0,
		     v3d.obj.out->W, v3d.obj.out->H,
		     v3d.obj.opacity);
	  MemCpy(v3d.zbuf,v3d.obj.zbuf,v3d.zlen * sizeof(float));
	}
      }

      /* queue everything that is invalid for background rendering */

      // skin
      if (v3d.skin.invalid && v3d.skin.enable) {
	MutexLock(v3d.skin.lock);
	view3d_alloc(M_SKIN,dw,dh);
	MutexUnlock(v3d.skin.lock);
	TaskNew(res.bgqueue,"Rendering Skin...",(Callback) &render_skin, 0);
      }

      // octant
      if (v3d.octant.invalid && v3d.octant.enable) {
	MutexLock(v3d.octant.lock);
	view3d_alloc(M_OCT,dw,dh);
	MutexUnlock(v3d.octant.lock);
	TaskNew(res.bgqueue,"Rendering Octant...",(Callback) &render_octant, 0);
      }

      // objects
      if (v3d.obj.invalid && v3d.obj.enable) {
	MutexLock(v3d.obj.lock);
	view3d_alloc(M_OBJ,dw,dh);
	MutexUnlock(v3d.obj.lock);
	TaskNew(res.bgqueue,"Rendering Objects...",(Callback) &render_objects, 0);
      }

    } else { // wireframe

      if (v3d.skin.enable) {
	MutexLock(v3d.skin.lock);
	view3d_alloc(M_SKIN,dw,dh);
	MutexUnlock(v3d.skin.lock);
	CImgFill(v3d.skin.out, view.bg);
	FloatFill(v3d.skin.zbuf,MAGIC,dw*dh);
	FloatFill(v3d.skin.ebuf,-MAGIC,dw*dh);

	render_quick_threshold(v3d.skin.out,&(v3d.wirerotation),
			       v3d.panx / v3d.zoom,v3d.pany / v3d.zoom,
			       voldata.original->W,
			       voldata.original->H,
			       voldata.original->D,
			       v3d.skin.tmap,v3d.skin.tcount,
			       v3d.skin.zbuf,v3d.skin.ebuf,v3d.skin.color);
      } else if (v3d.octant.enable) {
	MutexLock(v3d.octant.lock);
	view3d_alloc(M_OCT,dw,dh);
	MutexUnlock(v3d.octant.lock);
	CImgFill(v3d.octant.out, view.bg);
	FloatFill(v3d.octant.zbuf,MAGIC,dw*dh);
	lookup = app_calc_lookup();
	render_quick_full_octant(v3d.octant.out,&(v3d.wirerotation),
				 v3d.panx / v3d.zoom, v3d.pany / v3d.zoom,
				 voldata.original, v2d.cx, v2d.cy, v2d.cz,
				 lookup,v3d.octant.zbuf);
	MemFree(lookup);
      }
      if (v3d.obj.enable) {
	MutexLock(v3d.obj.lock);
	view3d_alloc(M_OBJ,dw,dh);
	MutexUnlock(v3d.obj.lock);
	CImgFill(v3d.obj.out, view.bg);
	render_quick_labels(v3d.obj.out,&(v3d.wirerotation),
			    v3d.panx / v3d.zoom, v3d.pany / v3d.zoom,
			    voldata.label, v3d.obj.zbuf);
      }

      // overlay everything

      if (v3d.skin.enable) {
	CImgBitBlt(v3d.base, 0,0,
		   v3d.skin.out, 0,0,
		   v3d.skin.out->W, v3d.skin.out->H);
	b = 1.0;
      } else if (v3d.octant.enable) {
	CImgBitBlt(v3d.base, 0,0,
		   v3d.octant.out, 0,0,
		   v3d.octant.out->W, v3d.octant.out->H);
	b = 1.0;
      } else {
	CImgFill(v3d.base,view.bg);
	b = 0.0;
      }

      if (v3d.obj.enable) {
	b+=1.0;
	CImgBitBlm(v3d.base,0,0,v3d.obj.out,0,0,
		   v3d.obj.out->W,v3d.obj.out->H,
		   1.0/b);
      }

    }

  }

  if (v3d.scaled) {
    CImgDestroy(v3d.scaled);
    v3d.scaled = 0;
  }

  v3d.scaled = CImgScale(v3d.base, v3d.zoom);

  if (v3d.show_measures && voldata.original!=NULL)
    view3d_draw_scaled_measures(v3d.scaled, v3d.wireframe ? &(v3d.wirerotation) : &(v3d.rotation), v3d.zoom);

  if (v3d.skin.enable && v3d.show_measures && voldata.original!=NULL)
    view3d_draw_scaled_georulers(v3d.scaled, v3d.base, v3d.wireframe ? &(v3d.wirerotation) : &(v3d.rotation), v3d.zoom);

  v3d.invalid = 0;
}

void view3d_draw_scaled_measures(CImg *dest, Transform *rot, float zoom) {
  int i,mc;
  int W,H,D,W2,H2,D2,w,h,w2,h2,px,py,qx,qy,rx,ry;
  Distance *dt;
  Angle *at;
  Point A,B,C,A1,B1,C1,A0,B0,C0;
  Transform ZT;
  char msg[64];

  Volume *vol;

  vol = voldata.original;
  if (vol==NULL) return;

  W = vol->W;
  H = vol->H;
  D = vol->D;
  W2 = W/2;
  H2 = H/2;
  D2 = D/2;

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  TransformScale(&ZT,zoom,zoom,1.0);

  mc = dist_n();
  for(i=0;i<mc;i++) {

    dt = dist_get(i);
    if (dt->done < 2) continue;

    A.x = dt->x[0] - W2;
    A.y = dt->y[0] - H2;
    A.z = dt->z[0] - D2;

    B.x = dt->x[1] - W2;
    B.y = dt->y[1] - H2;
    B.z = dt->z[1] - D2;

    PointTransform(&A0,&A,rot);
    PointTransform(&A1,&A0,&ZT);
    PointTransform(&B0,&B,rot);    
    PointTransform(&B1,&B0,&ZT);    

    px = (int) (A1.x + w2 + v3d.panx);
    py = (int) (A1.y + h2 + v3d.pany);
    qx = (int) (B1.x + w2 + v3d.panx);
    qy = (int) (B1.y + h2 + v3d.pany);

    CImgDrawLine(dest,px,py,qx,qy,v3d.measure_color);
    sprintf(msg,"%.1f mm",dt->value);
    CImgDrawShieldedSText(dest,res.font[4],(px+qx)/2,(py+qy)/2,view.measures,0,msg);
  }

  mc = angle_n();
  for(i=0;i<mc;i++) {

    at = angle_get(i);
    if (at->done < 3) continue;

    A.x = at->x[0] - W2;
    A.y = at->y[0] - H2;
    A.z = at->z[0] - D2;

    B.x = at->x[1] - W2;
    B.y = at->y[1] - H2;
    B.z = at->z[1] - D2;

    C.x = at->x[2] - W2;
    C.y = at->y[2] - H2;
    C.z = at->z[2] - D2;

    PointTransform(&A0,&A,rot);
    PointTransform(&A1,&A0,&ZT);
    PointTransform(&B0,&B,rot);
    PointTransform(&B1,&B0,&ZT);
    PointTransform(&C0,&C,rot);
    PointTransform(&C1,&C0,&ZT);

    px = (int) (A1.x + w2 + v3d.panx);
    py = (int) (A1.y + h2 + v3d.pany);
    qx = (int) (B1.x + w2 + v3d.panx);
    qy = (int) (B1.y + h2 + v3d.pany);
    rx = (int) (C1.x + w2 + v3d.panx);
    ry = (int) (C1.y + h2 + v3d.pany);

    CImgDrawLine(dest,px,py,qx,qy,v3d.measure_color);
    CImgDrawLine(dest,px,py,rx,ry,v3d.measure_color);
    CImgFillCircle(dest,px,py,3,v3d.measure_color);

    sprintf(msg,"%.1f\260",at->value);
    CImgDrawShieldedSText(dest,res.font[4],(px+qx+rx)/3,(py+qy+ry)/3,view.measures,0,msg);
  }
}


void view3d_draw_scaled_georulers(CImg *dest, CImg *base, Transform *rot, float zoom) {
  int i,j,gc;
  GeoRuler *gr;
  Transform ZT;

  Volume *vol;
  int W,H,D,WH,W2,H2,D2,w,h,w2,h2,c,uw,uh,uw2,uh2;
  int px,py,p,qx,qy,q,p0,q0;
  Point A,B,A1,B1,A0,B0;
  char msg[64];

  int segs[8192*5]; // x0,y0,x1,y1,c
  float segz[8192], minz, maxz, deltaz;
  int nseg;

  vol = voldata.original;
  if (vol==NULL) return;

  W = vol->W;
  H = vol->H;
  D = vol->D;
  WH = W*H;
  W2 = W/2;
  H2 = H/2;
  D2 = D/2;

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  uw = base->W;
  uh = base->H;
  uw2 = uw/2;
  uh2 = uh/2;

  TransformScale(&ZT,zoom,zoom,1.0);
  
  gc = georuler_count();
  for(i=0;i<gc;i++) {
    gr = georuler_get(i);
    
    if (gr->marks>=2) {

      nseg = 0;
      for(j=0;j<gr->np-1;j++) {

	if (nseg >= 8192) {
	  fprintf(stderr,"** error in view3d.c ** georuler has more than 8192 segments.\n");
	  break;
	}

	A.z = gr->data[j] / WH;
	A.y = (gr->data[j] % WH) / W;
	A.x = (gr->data[j] % WH) % W;
	A.x -= W2;
	A.y -= H2;
	A.z -= D2;

	B.z = gr->data[j+1] / WH;
	B.y = (gr->data[j+1] % WH) / W;
	B.x = (gr->data[j+1] % WH) % W;
	B.x -= W2;
	B.y -= H2;
	B.z -= D2;

	PointTransform(&A0,&A,rot);
	PointTransform(&B0,&B,rot);
	PointTransform(&A1,&A0,&ZT);
	PointTransform(&B1,&B0,&ZT);

	px = (int) (A0.x + uw2 + v3d.panx);
	py = (int) (A0.y + uh2 + v3d.pany);
	qx = (int) (B0.x + uw2 + v3d.panx);
	qy = (int) (B0.y + uh2 + v3d.pany);
	p0 = px + uw*py;
	q0 = qx + uw*qy;

	px = (int) (A1.x + w2 + v3d.panx);
	py = (int) (A1.y + h2 + v3d.pany);
	qx = (int) (B1.x + w2 + v3d.panx);
	qy = (int) (B1.y + h2 + v3d.pany);
	p = px + w*py;
	q = qx + w*qy;

	if (q < 0 || q >= w*h || p<0 || p>=w*h) continue;
	if (q0 < 0 || q0 >= uw*uh || p0 < 0 || p0 >= uw*uh) continue;

	if (j == gr->np / 2) {
	  sprintf(msg,"%.0f mm",gr->distance);
	  CImgDrawShieldedSText(dest,res.font[4],px+10,py,view.measures,0,msg);
	}

	/*
	if ( (A0.z > v3d.skin.zbuf[p0] + 2.0) &&
	     (B0.z > v3d.skin.zbuf[q0] + 2.0) ) {
	  segs[nseg*5+4] = 0xdd00dd;
	} else {
	  segs[nseg*5+4] = 0xffee00;
	}
	*/

	segs[nseg*5+4] = v3d.georuler_color;
	segs[nseg*5+0] = px;
	segs[nseg*5+1] = py;
	segs[nseg*5+2] = qx;
	segs[nseg*5+3] = qy;
	segz[nseg] = A0.z < B0.z ? A0.z : B0.z;
	nseg++;

      }

      maxz = minz = segz[0];
      for(j=0;j<nseg;j++) {
	if (segz[j] < minz) minz = segz[j];
	if (segz[j] > maxz) maxz = segz[j];
      }
      deltaz = maxz - minz;
      if (deltaz < 1.0) deltaz = 1.0;

      for(j=0;j<nseg;j++) {
	c = mergeColorsRGB(segs[j*5+4], 0, 0.50 * (segz[j] - minz) / deltaz);
	CImgDrawLine(dest,segs[5*j+0],segs[5*j+1],segs[5*j+2],segs[5*j+3],c);
      }

    }

    if (gr->marks >= 1) {
	A.z = gr->sv / WH;
	A.y = (gr->sv % WH) / W;
	A.x = (gr->sv % WH) % W;
	A.x -= W2;
	A.y -= H2;
	A.z -= D2;
	PointTransform(&A0,&A,rot);
	PointTransform(&A1,&A0,&ZT);
	px = (int) (A1.x + w2 + v3d.panx);
	py = (int) (A1.y + h2 + v3d.pany);
	p = px + w*py;
	if (p>=0 && p<w*h)
	  CImgDrawRect(dest,px-2,py-2,5,5,v3d.georuler_color);
    }

    if (gr->marks >= 2) {
	A.z = gr->dv / WH;
	A.y = (gr->dv % WH) / W;
	A.x = (gr->dv % WH) % W;
	A.x -= W2;
	A.y -= H2;
	A.z -= D2;
	PointTransform(&A0,&A,rot);
	PointTransform(&A1,&A0,&ZT);
	px = (int) (A1.x + w2 + v3d.panx);
	py = (int) (A1.y + h2 + v3d.pany);
	p = px + w*py;
	if (p>=0 && p<w*h)
	  CImgDrawRect(dest,px-2,py-2,5,5,v3d.georuler_color);
    }

  }
}

void view3d_invalidate(int data,int rotclip) {
  v3d.invalid = 1;
  view3d_skin_trash(data, rotclip+data);
  view3d_octant_trash();
  view3d_objects_trash();
}

void view3d_skin_trash(int map, int rendering) {
  if (map) {
    v3d.skin.invalid = 1;
    if (v3d.skin.tmap) { 
      MemFree(v3d.skin.tmap); 
      v3d.skin.tmap = 0; 
      v3d.skin.tcount = 0; 
    }
  }
  if (rendering) {
    v3d.skin.invalid = 1;
    v3d.skin.ready = 0;
  }
}

void view3d_octant_trash() {
  v3d.octant.invalid = 1;
  v3d.octant.ready = 0;
}

void view3d_objects_trash() {
  v3d.obj.invalid = 1;
  v3d.obj.ready = 0;
}

void view3d_overlay_skin_octant() {
  CImg *tmp;
  int w,h,i,wh;
  component *s,*o;
  float *sd, *od, *bd;
  component bg[3];

  tmp = CImgNew(w=v3d.octant.out->W,h=v3d.octant.out->H);
  if (!tmp) {
    error_memory();
    return;
  }

  CImgFill(tmp, view.bg);

  CImgBitBlm(tmp, 0,0, v3d.octant.out, 0,0,w,h,v3d.skin.opacity);
  MemCpy(v3d.zbuf,v3d.skin.zbuf,v3d.zlen * sizeof(float));

  wh = w*h;
  s = v3d.base->val;
  o = tmp->val;
  sd = v3d.skin.zbuf;
  od = v3d.octant.zbuf;
  bd = v3d.skin.ebuf;
  
  bg[0] = t0(view.bg);
  bg[1] = t1(view.bg);
  bg[2] = t2(view.bg);

  for(i=0;i<wh;i++) {
    
    if (*od != MAGIC) {
      if (*od >= *sd && *od <= *bd) {
	s[0] = o[0];
	s[1] = o[1];
	s[2] = o[2];
	v3d.zbuf[i] = v3d.octant.zbuf[i];
      }
      if (*od > *bd) {
	s[0] = bg[0];
	s[1] = bg[1];
	s[2] = bg[2];
	v3d.zbuf[i] = MAGIC;
      }
    }
        
    s+=3;
    o+=3;
    od++;
    sd++;
    bd++;
  }

  CImgDestroy(tmp);
}

void view3d_overlay_obj() {
  int i,w,h,n;
  component *s,*o;
  float *sd, *od;
  float ratio, comp, r2,c2;

  s = v3d.base->val;
  o = v3d.obj.out->val;

  w = v3d.obj.out->W;
  h = v3d.obj.out->H;

  sd = v3d.zbuf;
  od = v3d.obj.zbuf;

  ratio = v3d.obj.opacity;
  comp = 1.0 - ratio;

  r2 = v3d.skin.opacity;

  r2 = r2+comp;
  if (r2 > 1.0) r2 = 1.0;
  c2 = 1.0 - r2;;

  n = w*h;
  for(i=0;i<n;i++) {

    if (*od != MAGIC) {

      if (*od < *sd) {
	s[0] = (component) ((ratio * (float)(o[0])) + (comp * (float)(s[0])));
	s[1] = (component) ((ratio * (float)(o[1])) + (comp * (float)(s[1])));
	s[2] = (component) ((ratio * (float)(o[2])) + (comp * (float)(s[2])));
	*sd = *od;
      } else {
	s[0] = (component) ((c2 * (float)(o[0])) + (r2 * (float)(s[0])));
	s[1] = (component) ((c2 * (float)(o[1])) + (r2 * (float)(s[1])));
	s[2] = (component) ((c2 * (float)(o[2])) + (r2 * (float)(s[2])));
      }
    }

    s+=3;
    o+=3;
    od++;
    sd++;

  }

}


GtkWidget *dlg3d = 0;

static struct _v3ddlgctl {
  GtkWidget  *skin_value;
  GtkWidget  *skin_smoothing;
  int         skin_color;
  ColorLabel *skin_color_label;

  GtkWidget  *octant_usedepth;

  GtkWidget  *light_ka;
  GtkWidget  *light_kd;
  GtkWidget  *light_ks;
  GtkWidget  *light_sre;
  GtkWidget  *light_zgamma;
  GtkWidget  *light_zdepth;
  Vector      light_src;

  GtkWidget  *light_preview;
  float      *light_zbuf;
  float      *light_normal;
  int         lp_w, lp_h;
  CImg       *light_picture;

  int georuler_color, measure_color;
  ColorLabel *georuler_clabel, *measure_clabel;

  int save_skin_value;
  int save_skin_smoothing;
  int save_skin_color;
  int save_octant_usedepth;

  int save_georuler_color;
  int save_measure_color;

  float  save_light_par[6];
  Vector save_light_src;

} dlgctl;

gboolean view3d_dlg_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  return TRUE;
}

void view3d_dlg_destroy(GtkWidget *w, gpointer data) {
  dlg3d = 0;
  MemFree(dlgctl.light_zbuf);
  MemFree(dlgctl.light_normal);
  CImgDestroy(dlgctl.light_picture);
}

void view3d_dlg_apply(GtkWidget *w, gpointer data) {
  v3d.skin.value = (int) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlgctl.skin_value));
  v3d.skin.smoothing = (int) gtk_range_get_value(GTK_RANGE(dlgctl.skin_smoothing));
  v3d.skin.color = dlgctl.skin_color;
  v3d.octant.usedepth = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlgctl.octant_usedepth));

  v3d.ka = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_ka));
  v3d.kd = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_kd));
  v3d.ks = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_ks));
  v3d.sre = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_sre));
  v3d.zgamma = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_zgamma));
  v3d.zstretch = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_zdepth));
  VectorCopy(&(v3d.light), &(dlgctl.light_src));
  VectorNormalize(&(v3d.light));

  v3d.measure_color  = dlgctl.measure_color;
  v3d.georuler_color = dlgctl.georuler_color;

  view3d_invalidate(1,1);
  force_redraw_3D();
}

void view3d_dlg_ok(GtkWidget *w, gpointer data) {
  view3d_dlg_apply(w,data);
  gtk_widget_destroy(dlg3d);
}

void view3d_dlg_cancel(GtkWidget *w, gpointer data) {

  if (dlgctl.save_skin_value != v3d.skin.value ||
      dlgctl.save_skin_smoothing != v3d.skin.smoothing ||
      dlgctl.save_skin_color != v3d.skin.color ||
      dlgctl.save_octant_usedepth != v3d.octant.usedepth) {
    v3d.skin.value      = dlgctl.save_skin_value;
    v3d.skin.smoothing  = dlgctl.save_skin_smoothing;
    v3d.skin.color      = dlgctl.save_skin_color;
    v3d.octant.usedepth = dlgctl.save_octant_usedepth;
    view3d_invalidate(1,1);
  } else if (dlgctl.save_light_par[0] != v3d.ka ||
	     dlgctl.save_light_par[1] != v3d.kd ||
	     dlgctl.save_light_par[2] != v3d.ks ||
	     dlgctl.save_light_par[3] != v3d.sre ||
	     dlgctl.save_light_par[4] != v3d.zgamma ||
	     dlgctl.save_light_par[5] != v3d.zstretch ||
	     !VectorEqual(&(dlgctl.save_light_src),
			  &(v3d.light))) {
    v3d.ka = dlgctl.save_light_par[0];
    v3d.kd = dlgctl.save_light_par[1];
    v3d.ks = dlgctl.save_light_par[2];
    v3d.sre = (int) dlgctl.save_light_par[3];
    v3d.zgamma = dlgctl.save_light_par[4];
    v3d.zstretch = dlgctl.save_light_par[5];
    VectorCopy(&(v3d.light),&(dlgctl.save_light_src));
    view3d_invalidate(0,1);
  }
  v3d.georuler_color = dlgctl.save_georuler_color;
  v3d.measure_color  = dlgctl.save_measure_color;
  force_redraw_3D();
  gtk_widget_destroy(dlg3d);
}

void view3d_dlg_edit_skin_color(GtkWidget *w, gpointer data) {
  dlgctl.skin_color = util_get_color(dlg3d, dlgctl.skin_color,"Skin Color");
  ColorLabelSetColor(dlgctl.skin_color_label,dlgctl.skin_color);
}

void view3d_dlg_edit_measure_color(GtkWidget *w, gpointer data) {
  dlgctl.measure_color = util_get_color(dlg3d, dlgctl.measure_color,"Rulers and Angles");
  ColorLabelSetColor(dlgctl.measure_clabel,dlgctl.measure_color);
}

void view3d_dlg_edit_georuler_color(GtkWidget *w, gpointer data) {
  dlgctl.georuler_color = util_get_color(dlg3d, dlgctl.georuler_color,"Geodesic Rulers");
  ColorLabelSetColor(dlgctl.georuler_clabel,dlgctl.georuler_color);
}

void view3d_preview_render() {
  int i,j,w,h,p,c,ca;
  float *zbuf,*norm;
  float diag,z,dz,kz,fa,fb,Y;
  float par[6];
  CImg *dest;
  Vector src;

  w = dlgctl.lp_w;
  h = dlgctl.lp_h;

  c = 0xff8000;  

  VectorNormalize(&(dlgctl.light_src));
  zbuf = dlgctl.light_zbuf;
  norm = dlgctl.light_normal;
  dest = dlgctl.light_picture;
  CImgFill(dest,view.bg);
  CImgDrawRect(dest,0,0,w,h,0);

  for(j=6;j<h-6;j++)
    for(i=6;i<w-6;i++)
      render_calc_s6_normal(i,j,w,zbuf,norm,&(dlgctl.light_src));

  diag = 42.0;

  par[0] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_ka));
  par[1] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_kd));
  par[2] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_ks));
  par[3] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_sre));
  par[4] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_zgamma));
  par[5] = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(dlgctl.light_zdepth));

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {

      p = i + j*w;
      if (zbuf[p] == MAGIC) continue;

      z = zbuf[p];
      dz = (diag - z);
      kz = (dz*dz) / (4.0*diag*diag);
      kz = par[4] + par[5] * kz;

      fa = norm[p];
      fb = phong_specular(fa, (int) par[3]);
	
      ca=RGB2YCbCr(c);
      Y = (float) t0(ca);
      Y /= 255.0;
	
      Y = par[0] + kz * ( par[1] * Y * fa + par[2] * Y * fb);
      Y *= 255.0;

      if (Y > 255.0) Y=255.0;
      if (Y < 0.0) Y=0.0;
      ca = triplet((int) Y, t1(ca), t2(ca));
      ca = YCbCr2RGB(ca);
      CImgSet(dest,i,j,ca);
    }

  VectorCopy(&src,&(dlgctl.light_src));
  VectorNormalize(&src);
  VectorScalarProduct(&src, 42.0);

  i = w/2 + (int) src.x;
  j = h/2 + (int) src.y;

  CImgDrawCircle(dest,i,j,6,0);
  CImgDrawCircle(dest,i,j,3,0);
  CImgSet(dest,i,j,0);
  
}

gboolean view3d_dlg_lp_click(GtkWidget *w,GdkEventButton *eb,gpointer data)
{
  int x,y;
  Vector src;

  if (eb->button != 1) return FALSE;
  x = (int) (eb->x);
  y = (int) (eb->y);

  src.z = dlgctl.light_zbuf[x + y*dlgctl.lp_w];
  if (src.z == MAGIC) return FALSE;

  src.x = x - dlgctl.lp_w / 2.0;
  src.y = y - dlgctl.lp_h / 2.0;

  VectorNormalize(&src);
  VectorCopy(&(dlgctl.light_src),&src);
  refresh(dlgctl.light_preview);

  return FALSE;
}

gboolean view3d_dlg_lp_drag(GtkWidget *w,GdkEventMotion *em,gpointer data) {
  GdkEventButton eb;

  if (em->state & GDK_BUTTON1_MASK) {
    eb.x = em->x;
    eb.y = em->y;
    eb.button = 1;
    view3d_dlg_lp_click(w,&eb,0);
  }

  return FALSE;
}

gboolean view3d_dlg_lp_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data)
{
  CImg *t;
  view3d_preview_render();
  
  t = dlgctl.light_picture;
  gdk_draw_rgb_image(w->window, w->style->black_gc,
		     0,0,t->W,t->H,
		     GDK_RGB_DITHER_NORMAL,
		     t->val, t->rowlen);

  return FALSE;
}

void view3d_alloc_dialog_zbuf() {
  int w,h,i,j;
  float *zbuf,*norm;
  float x,y,r,z;

  dlgctl.lp_w = w = 96;
  dlgctl.lp_h = h = 96;

  zbuf = dlgctl.light_zbuf = (float *) MemAlloc(sizeof(float) * w * h);
  norm = dlgctl.light_normal = (float *) MemAlloc(sizeof(float) * w * h);

  if (!zbuf || !norm) {
    error_memory();
    return;
  }

  FloatFill(zbuf,MAGIC,w*h);
  FloatFill(norm,0.0,w*h);

  r = 42.0;

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      x = i - (w/2.0);
      y = j - (h/2.0);
      z = r*r - x*x - y*y;
      if (z > 0.0)
	zbuf[i+j*w] = -sqrt(z);
    }

  dlgctl.light_picture = CImgNew(w,h);
  if (!dlgctl.light_picture)
    error_memory();
}

void view3d_dlg_lp_change(GtkSpinButton *sb,gpointer data) {
  refresh(dlgctl.light_preview);
}

void view3d_dialog_options() {

  GtkWidget *v,*nb,*bh,*apply,*cancel,*ok;
  GtkWidget *skin,*ls,*octant,*lo,*light,*ll, *lc, *color;

  GtkWidget *h1,*l1,*h2,*l2,*h3,*l3,*b3,*h4,*lex;
  GtkWidget *t1,*tl1,*tl2,*tl3,*tl4,*tl5,*tl6,*tl7,*tl8;
  GdkColor blue;
  GtkWidget *lct, *lc1,*lc2,*lcb1,*lcb2;
  
  if (dlg3d != 0) {
    gtk_window_present(GTK_WINDOW(dlg3d));
    return;
  }

  dlgctl.save_skin_value      = v3d.skin.value;
  dlgctl.save_skin_smoothing  = v3d.skin.smoothing;
  dlgctl.save_skin_color      = v3d.skin.color;
  dlgctl.save_octant_usedepth = v3d.octant.usedepth;

  dlgctl.save_measure_color      = v3d.measure_color;
  dlgctl.save_georuler_color      = v3d.georuler_color;

  dlgctl.save_light_par[0] = v3d.ka;
  dlgctl.save_light_par[1] = v3d.kd;
  dlgctl.save_light_par[2] = v3d.ks;
  dlgctl.save_light_par[3] = v3d.sre;
  dlgctl.save_light_par[4] = v3d.zgamma;
  dlgctl.save_light_par[5] = v3d.zstretch;
  VectorCopy(&(dlgctl.save_light_src),&(v3d.light));

  dlg3d = util_dialog_new("3D Options",mw,1,&v,&bh,-1,-1);
  
  nb = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(v),nb,TRUE,TRUE,0);

  /* skin tab */

  skin = gtk_vbox_new(FALSE,2);
  ls = gtk_label_new("Skin");
  gtk_notebook_append_page(GTK_NOTEBOOK(nb), skin, ls);
  
  h1 = gtk_hbox_new(FALSE,2);
  l1 = gtk_label_new("Skin Threshold:");
  dlgctl.skin_value = gtk_spin_button_new_with_range(0.0,32767.0,1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.skin_value),0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.skin_value), 
			    (gdouble) v3d.skin.value);

  lex = gtk_label_new(""); // HERE
  gdk_color_parse("#000080",&blue);
  gtk_widget_modify_fg(lex,GTK_STATE_NORMAL,&blue);

  gtk_box_pack_start(GTK_BOX(skin), h1, FALSE,TRUE,2);
  gtk_box_pack_start(GTK_BOX(h1), l1, FALSE, FALSE,2);
  gtk_box_pack_start(GTK_BOX(h1), dlgctl.skin_value, FALSE, FALSE,2);
  gtk_box_pack_start(GTK_BOX(h1), lex, FALSE,TRUE,2);

  h2 = gtk_hbox_new(FALSE,2);
  l2 = gtk_label_new("Skin Smoothing:");
  dlgctl.skin_smoothing = gtk_hscale_new_with_range(1.0,12.0,1.0);
  gtk_scale_set_digits(GTK_SCALE(dlgctl.skin_smoothing), 0);
  gtk_scale_set_draw_value(GTK_SCALE(dlgctl.skin_smoothing), TRUE);
  gtk_range_set_value(GTK_RANGE(dlgctl.skin_smoothing), 
		      (gdouble) v3d.skin.smoothing);

  gtk_box_pack_start(GTK_BOX(skin), h2, FALSE,TRUE,2);
  gtk_box_pack_start(GTK_BOX(h2), l2, FALSE, FALSE,2);
  gtk_box_pack_start(GTK_BOX(h2), dlgctl.skin_smoothing, TRUE, TRUE,2);

  h3 = gtk_hbox_new(FALSE,2);
  l3 = gtk_label_new("Skin Color:");
  dlgctl.skin_color = v3d.skin.color;
  dlgctl.skin_color_label = ColorLabelNew(64,16,v3d.skin.color,0);
  b3 = gtk_button_new_with_label("Change...");

  gtk_box_pack_start(GTK_BOX(skin), h3, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(h3), l3, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(h3), dlgctl.skin_color_label->widget, 
		     FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(h3), b3, FALSE, FALSE, 2);

  /* octant tab */

  octant = gtk_vbox_new(FALSE,2);
  lo = gtk_label_new("Octant");
  gtk_notebook_append_page(GTK_NOTEBOOK(nb), octant, lo);

  h4 = gtk_hbox_new(FALSE,2);
  dlgctl.octant_usedepth = gtk_check_button_new_with_label("Depth Shading");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlgctl.octant_usedepth),
			       v3d.octant.usedepth ? TRUE : FALSE);
  gtk_box_pack_start(GTK_BOX(octant), h4,FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(h4), dlgctl.octant_usedepth, FALSE, FALSE, 0);

  /* light tab */

  light = gtk_vbox_new(FALSE,2);
  ll = gtk_label_new("Lighting");
  gtk_notebook_append_page(GTK_NOTEBOOK(nb), light, ll);

  t1 = gtk_table_new(7,4,FALSE);
  tl1 = gtk_label_new("Ambient");
  tl2 = gtk_label_new("Diffuse");
  tl3 = gtk_label_new("Specular");
  tl4 = gtk_label_new("Dispersion");
  tl5 = gtk_label_new("Atenuation Range");
  tl6 = gtk_label_new("Atenuation Rate");

  gtk_box_pack_start(GTK_BOX(light), t1, TRUE, TRUE, 2);
  gtk_table_attach(GTK_TABLE(t1),tl1,0,1,0,1,GTK_SHRINK,GTK_SHRINK,2,2);
  gtk_table_attach(GTK_TABLE(t1),tl2,0,1,1,2,GTK_SHRINK,GTK_SHRINK,2,2);
  gtk_table_attach(GTK_TABLE(t1),tl3,0,1,2,3,GTK_SHRINK,GTK_SHRINK,2,2);
  gtk_table_attach(GTK_TABLE(t1),tl4,0,1,3,4,GTK_SHRINK,GTK_SHRINK,2,2);
  gtk_table_attach(GTK_TABLE(t1),tl5,0,1,4,5,GTK_SHRINK,GTK_SHRINK,2,2);
  gtk_table_attach(GTK_TABLE(t1),tl6,0,1,5,6,GTK_SHRINK,GTK_SHRINK,2,2);

  dlgctl.light_ka     = gtk_spin_button_new_with_range(0.0,1.0,0.05);
  dlgctl.light_kd     = gtk_spin_button_new_with_range(0.0,1.0,0.05);
  dlgctl.light_ks     = gtk_spin_button_new_with_range(0.0,1.0,0.05);
  dlgctl.light_sre    = gtk_spin_button_new_with_range(1.0,50.0,1.0);
  dlgctl.light_zgamma = gtk_spin_button_new_with_range(0.0,1.0,0.05);
  dlgctl.light_zdepth = gtk_spin_button_new_with_range(0.0,3.0,0.05);
  VectorCopy(&(dlgctl.light_src),&(v3d.light));

  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_ka),2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_ka),(gdouble) v3d.ka);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_kd),2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_kd),(gdouble) v3d.kd);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_ks),2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_ks),(gdouble) v3d.ks);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_sre),0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_sre),(gdouble) v3d.sre);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_zgamma),2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_zgamma),(gdouble) v3d.zgamma);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(dlgctl.light_zdepth),2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlgctl.light_zdepth),(gdouble) v3d.zstretch);

  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_ka,1,2,0,1);
  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_kd,1,2,1,2);
  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_ks,1,2,2,3);
  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_sre,1,2,3,4);
  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_zgamma,1,2,4,5);
  gtk_table_attach_defaults(GTK_TABLE(t1),dlgctl.light_zdepth,1,2,5,6);

  tl7 = gtk_label_new("Visualization:");
  gtk_table_attach(GTK_TABLE(t1),tl7,2,3,0,1,GTK_SHRINK,GTK_SHRINK,0,0);

  dlgctl.light_preview = gtk_drawing_area_new();
  gtk_widget_set_events(dlgctl.light_preview,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON1_MOTION_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(dlgctl.light_preview), 96,96);
  
  gtk_table_attach(GTK_TABLE(t1),dlgctl.light_preview,2,4,1,6,GTK_SHRINK,GTK_SHRINK,4,4);

  tl8 = gtk_label_new("Click over the sphere");
  gtk_widget_modify_fg(tl8,GTK_STATE_NORMAL,&blue);
  gtk_table_attach(GTK_TABLE(t1),tl8,2,4,6,7,GTK_SHRINK,GTK_SHRINK,0,0);

  view3d_alloc_dialog_zbuf();

  /* color tab */

  color = gtk_vbox_new(FALSE,2);
  lc = gtk_label_new("Colors");
  gtk_notebook_append_page(GTK_NOTEBOOK(nb), color, lc);

  lct = gtk_table_new(2,3,FALSE);

  lc1 = gtk_label_new("Rulers and Angles");
  lc2 = gtk_label_new("Geodesic Rulers");

  dlgctl.measure_color = v3d.measure_color;
  dlgctl.georuler_color = v3d.georuler_color;

  dlgctl.measure_clabel = ColorLabelNew(64,16,v3d.measure_color,0);
  dlgctl.georuler_clabel = ColorLabelNew(64,16,v3d.georuler_color,0);

  lcb1 = gtk_button_new_with_label("Change...");
  lcb2 = gtk_button_new_with_label("Change...");

  gtk_table_attach_defaults(GTK_TABLE(lct),lc1,0,1,0,1);
  gtk_table_attach(GTK_TABLE(lct),dlgctl.measure_clabel->widget,1,2,0,1,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,4,4);
  gtk_table_attach(GTK_TABLE(lct),lcb1,2,3,0,1,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,4,4);

  gtk_table_attach_defaults(GTK_TABLE(lct),lc2,0,1,1,2);
  gtk_table_attach(GTK_TABLE(lct),dlgctl.georuler_clabel->widget,1,2,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,4,4);
  gtk_table_attach(GTK_TABLE(lct),lcb2,2,3,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,4,4);

  gtk_box_pack_start(GTK_BOX(color), lct, FALSE, FALSE, 2);

  /* bottom */
  
  ok     = gtk_button_new_with_label("Ok");
  apply  = gtk_button_new_with_label("Apply");
  cancel = gtk_button_new_with_label("Cancel");

  gtk_box_pack_start(GTK_BOX(bh), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(bh), apply, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(bh), cancel, TRUE, TRUE, 0);

  /* connect signals */

  gtk_signal_connect(GTK_OBJECT(dlg3d),"destroy",
		     GTK_SIGNAL_FUNC(view3d_dlg_destroy),0);

  gtk_signal_connect(GTK_OBJECT(dlg3d),"delete_event",
		     GTK_SIGNAL_FUNC(view3d_dlg_delete),0);

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_ok),0);

  gtk_signal_connect(GTK_OBJECT(apply),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_apply),0);

  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_cancel),0);

  gtk_signal_connect(GTK_OBJECT(b3),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_edit_skin_color),0);

  gtk_signal_connect(GTK_OBJECT(lcb1),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_edit_measure_color),0);

  gtk_signal_connect(GTK_OBJECT(lcb2),"clicked",
		     GTK_SIGNAL_FUNC(view3d_dlg_edit_georuler_color),0);

  gtk_signal_connect(GTK_OBJECT(dlgctl.light_preview),"expose_event",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_expose),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_preview),"button_press_event",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_click),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_preview),"motion_notify_event",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_drag),0);

  gtk_signal_connect(GTK_OBJECT(dlgctl.light_ka),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_kd),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_ks),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_sre),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_zgamma),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.light_zdepth),"value_changed",
		     GTK_SIGNAL_FUNC(view3d_dlg_lp_change),0);

  /* show everything */

  gtk_widget_show_all(dlg3d);
}

void view3d_octctl_init(int w,int h,float maxside) {
  ocube.w = w;
  ocube.h = h;
  ocube.maxside = maxside;
  ocube.out = CImgNew(w,h);
  ocube.zbuf = (float *) MemAlloc(sizeof(float) * w * h);
  ocube.alpha = (float *) MemAlloc(sizeof(float) * w * h);
  ocube.beta  = (float *) MemAlloc(sizeof(float) * w * h);
  ocube.gamma = (float *) MemAlloc(sizeof(float) * w * h);
  ocube.val   = (char *) MemAlloc(sizeof(char) * w * h);
  ocube.pending = 0;
  ocube.ox = ocube.oy = 0;

  FloatFill(ocube.zbuf,  MAGIC, w*h);
  FloatFill(ocube.alpha, MAGIC, w*h);
  FloatFill(ocube.beta,  MAGIC, w*h);
  FloatFill(ocube.gamma, MAGIC, w*h);  
  MemSet(ocube.val, 0 ,w*h);
}

void view3d_octctl_render() {
  int i,j,w,h,n,p,ii,jj;
  int W,H,D,W2,H2,D2,w2,h2,mz,ox,oy,oz;
  float ms,ma,mb,mc,alpha,beta,gamma;
  float *depth;
  Transform T,S;
  double MA[9],MB[3];
  Vector R0,R,P0a,P0b,P1,P2,P0ar,P0br,P1r,P2r,VA,VB,VC,L;
  Point  C[8],Ct[8];
  float N[7], xi[3], yi[3], zi[3], vz;

  w = ocube.w;
  h = ocube.h;
  n = w*h;
  depth = ocube.zbuf;

  FloatFill(ocube.zbuf, MAGIC, n);
  CImgFill(ocube.out, view.pbg);
  MemSet(ocube.val, 0, n);

  if (voldata.original == NULL) return;
  
  W = voldata.original->W;
  H = voldata.original->H;
  D = voldata.original->D;
  ms = (float) (MAX(MAX(W,H),D));
  W2 = W/2;
  H2 = H/2;
  D2 = D/2;
  w2 = w/2;
  h2 = h/2;

  ma = W2;
  mb = H2;
  mc = D2;

  TransformCopy(&T,v3d.wireframe?&(v3d.wirerotation):&(v3d.rotation));
  TransformIsoScale(&S,ocube.maxside/ms);
  TransformCompose(&S,&T);

  // XY planes

  VectorAssign(&R, 0.0, 0.0, 1.0);
  VectorAssign(&P0a, W2, H2, 0);
  VectorAssign(&P0b, W2, H2, D);

  VectorAssign(&P1, 1.0, 0.0, 0.0);
  VectorAssign(&P2, 0.0, 1.0, 0.0);
  PointTranslate(&P0a,-W2,-H2,-D2);
  PointTransform(&P0ar, &P0a, &S);
  PointTranslate(&P0ar,W2,H2,D2);
  PointTranslate(&P0b,-W2,-H2,-D2);
  PointTransform(&P0br, &P0b, &S);
  PointTranslate(&P0br,W2,H2,D2);
  PointTransform(&P1r, &P1, &S);
  PointTransform(&P2r, &P2, &S);
  VectorCrossProduct(&VA,&P1r,&P2r);

  MA[0] = R.x; MA[1] = -P1r.x; MA[2] = -P2r.x;
  MA[3] = R.y; MA[4] = -P1r.y; MA[5] = -P2r.y;
  MA[6] = R.z; MA[7] = -P1r.z; MA[8] = -P2r.z;

  if (minv(MA,3) == 0) {
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	
	R0.x = i - w2 + W2; R0.y = j - h2 + H2; R0.z = 0.0;
	MB[0] = P0ar.x - R0.x;
	MB[1] = P0ar.y - R0.y;
	MB[2] = P0ar.z - R0.z;
	
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -ma && beta < ma && gamma > -mb && gamma < mb) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 1;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}

	MB[0] = P0br.x - R0.x;
	MB[1] = P0br.y - R0.y;
	MB[2] = P0br.z - R0.z;
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -ma && beta < ma && gamma > -mb && gamma < mb) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 2;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}
      }
  }

  // XZ planes

  VectorAssign(&R, 0.0, 0.0, 1.0);
  VectorAssign(&P0a, W2, 0, D2);
  VectorAssign(&P0b, W2, H, D2);

  VectorAssign(&P1, 1.0, 0.0, 0.0);
  VectorAssign(&P2, 0.0, 0.0, 1.0);
  PointTranslate(&P0a,-W2,-H2,-D2);
  PointTransform(&P0ar, &P0a, &S);
  PointTranslate(&P0ar,W2,H2,D2);
  PointTranslate(&P0b,-W2,-H2,-D2);
  PointTransform(&P0br, &P0b, &S);
  PointTranslate(&P0br,W2,H2,D2);
  PointTransform(&P1r, &P1, &S);
  PointTransform(&P2r, &P2, &S);
  VectorCrossProduct(&VB,&P1r,&P2r);

  MA[0] = R.x; MA[1] = -P1r.x; MA[2] = -P2r.x;
  MA[3] = R.y; MA[4] = -P1r.y; MA[5] = -P2r.y;
  MA[6] = R.z; MA[7] = -P1r.z; MA[8] = -P2r.z;

  if (minv(MA,3) == 0) {
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	
	R0.x = i - w2 + W2; R0.y = j - h2 + H2; R0.z = 0.0;
	MB[0] = P0ar.x - R0.x;
	MB[1] = P0ar.y - R0.y;
	MB[2] = P0ar.z - R0.z;
	
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -ma && beta < ma && gamma > -mc && gamma < mc) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 3;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}

	MB[0] = P0br.x - R0.x;
	MB[1] = P0br.y - R0.y;
	MB[2] = P0br.z - R0.z;
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -ma && beta < ma && gamma > -mc && gamma < mc) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 4;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}
      }
  }

  // YZ planes

  VectorAssign(&R, 0.0, 0.0, 1.0);
  VectorAssign(&P0a, 0, H2, D2);
  VectorAssign(&P0b, W, H2, D2);

  VectorAssign(&P1, 0.0, 1.0, 0.0);
  VectorAssign(&P2, 0.0, 0.0, 1.0);
  PointTranslate(&P0a,-W2,-H2,-D2);
  PointTransform(&P0ar, &P0a, &S);
  PointTranslate(&P0ar,W2,H2,D2);
  PointTranslate(&P0b,-W2,-H2,-D2);
  PointTransform(&P0br, &P0b, &S);
  PointTranslate(&P0br,W2,H2,D2);
  PointTransform(&P1r, &P1, &S);
  PointTransform(&P2r, &P2, &S);
  VectorCrossProduct(&VC,&P1r,&P2r);

  MA[0] = R.x; MA[1] = -P1r.x; MA[2] = -P2r.x;
  MA[3] = R.y; MA[4] = -P1r.y; MA[5] = -P2r.y;
  MA[6] = R.z; MA[7] = -P1r.z; MA[8] = -P2r.z;

  if (minv(MA,3) == 0) {
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	
	R0.x = i - w2 + W2; R0.y = j - h2 + H2; R0.z = 0.0;
	MB[0] = P0ar.x - R0.x;
	MB[1] = P0ar.y - R0.y;
	MB[2] = P0ar.z - R0.z;
	
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -mb && beta < mb && gamma > -mc && gamma < mc) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 5;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}

	MB[0] = P0br.x - R0.x;
	MB[1] = P0br.y - R0.y;
	MB[2] = P0br.z - R0.z;
	mmul(MA,MB,3);
	alpha = MB[0]; beta  = MB[1]; gamma = MB[2];

	if (beta > -mb && beta < mb && gamma > -mc && gamma < mc) {
	  p = i+j*w;
	  if (alpha < depth[p]) {
	    depth[p] = alpha;
	    ocube.val[p] = 6;
	    ocube.alpha[p] = alpha;
	    ocube.beta[p] = beta;
	    ocube.gamma[p] = gamma;
	  }
	}
      }
  }

  // compute plane normals

  VectorNormalize(&VA);
  VectorNormalize(&VB);
  VectorNormalize(&VC);
  VectorAssign(&L,0.0,0.0,-1.0);
  
  N[0] = 0.0;
  N[1] = N[2] = VectorInnerProduct(&L,&VA);
  N[3] = N[4] = VectorInnerProduct(&L,&VB);
  N[5] = N[6] = VectorInnerProduct(&L,&VC);
  for(i=0;i<7;i++)
    if (N[i] < 0.0) N[i] = -N[i];

  // compute octant intervals

  PointAssign(&C[0], 0,0,0);
  PointAssign(&C[1], W,0,0);
  PointAssign(&C[2], W,H,0);
  PointAssign(&C[3], 0,H,0);
  PointAssign(&C[4], 0,0,D);
  PointAssign(&C[5], W,0,D);
  PointAssign(&C[6], W,H,D);
  PointAssign(&C[7], 0,H,D);

  for(i=0;i<8;i++) {
    PointTranslate(&C[i],-W2,-H2,-D2);
    PointTransform(&Ct[i],&C[i],&T);
  }
  vz = Ct[0].z;
  mz = 0;
  for(i=0;i<8;i++)
    if (Ct[i].z < vz) {
      vz = Ct[i].z;
      mz = i;
    }

  ox = v2d.cx;
  oy = v2d.cy;
  oz = v2d.cz;

  if (mz==0 || mz==3 || mz==4 || mz==7) {
    xi[0] = 0.0 - W2; xi[1] = ox - W2; xi[2] = 0.0 - W2;
  } else {
    xi[0] = ox - W2;  xi[1] = W2; xi[2] = W2;
  }

  if (mz==0 || mz==1 || mz==4 || mz==5) {
    yi[0] = 0.0 - H2; yi[1] = oy - H2; yi[2] = 0.0 - H2;
  } else {
    yi[0] = oy - H2; yi[1] = H2; yi[2] = H2;
  }

  if (mz<=3) {
    zi[0] = 0.0 - D2; zi[1] = oz - D2; zi[2] = 0.0 - D2;
  } else {
    zi[0] = oz - D2; zi[1] = D2; zi[2] = D2;
  }

  // make outline
  for(j=0;j<h-1;j++)
    for(i=0;i<w-1;i++) {
      p = i+j*w;
      if (ocube.val[p] != ocube.val[p+1] ||
	  ocube.val[p] != ocube.val[p+w])
	ocube.val[p] = 7;
    }

  // make hidden lines
  for(vz = xi[0]; vz <= xi[1]; vz += 1.0) {
    for(ii=0;ii<2;ii++)
      for(jj=0;jj<2;jj++) {
	PointAssign(&R, vz, yi[ii], zi[jj]);
	PointTransform(&R0, &R, &S);
	PointTranslate(&R0, w2, h2, 0.0);
	i = (int) R0.x;
	j = (int) R0.y;
	if (i>=0 && j>=0 && i<w && j<h) {
	  p = i+j*w;
	  if (ocube.val[p]!=7) ocube.val[p] |= 0x80;
	}
      }
  }
  for(vz = yi[0]; vz <= yi[1]; vz += 1.0) {
    for(ii=0;ii<2;ii++)
      for(jj=0;jj<2;jj++) {
	PointAssign(&R, xi[ii], vz, zi[jj]);
	PointTransform(&R0, &R, &S);
	PointTranslate(&R0, w2, h2, 0.0);
	i = (int) R0.x;
	j = (int) R0.y;
	if (i>=0 && j>=0 && i<w && j<h) {
	  p = i+j*w;
	  if (ocube.val[p]!=7) ocube.val[p] |= 0x80;
	}
      }
  }
  for(vz = zi[0]; vz <= zi[1]; vz += 1.0) {
    for(ii=0;ii<2;ii++)
      for(jj=0;jj<2;jj++) {
	PointAssign(&R, xi[ii], yi[jj], vz);
	PointTransform(&R0, &R, &S);
	PointTranslate(&R0, w2, h2, 0.0);
	i = (int) R0.x;
	j = (int) R0.y;
	if (i>=0 && j>=0 && i<w && j<h) {
	  p = i+j*w;
	  if (ocube.val[p]!=7) ocube.val[p] |= 0x80;
	}
      }
  }

  // render it

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;
      if (ocube.val[p]) {

	mz = 0x8080ff;

	switch(ocube.val[p]&0x0f) {
	case 1: case 2: // XY
	  if (ocube.beta[p] >= xi[0] && ocube.beta[p] <= xi[1] &&
	      ocube.gamma[p] >= yi[0] && ocube.gamma[p] <= yi[1])
	    mz = 0xffff40;
	  break;
	case 3: case 4: // XZ
	  if (ocube.beta[p] >= xi[0] && ocube.beta[p] <= xi[1] &&
	      ocube.gamma[p] >= zi[0] && ocube.gamma[p] <= zi[1])
	    mz = 0xffff40;
	  break;
	case 5: case 6:
	  if (ocube.beta[p] >= yi[0] && ocube.beta[p] <= yi[1] &&
	      ocube.gamma[p] >= zi[0] && ocube.gamma[p] <= zi[1])
	    mz = 0xffff40;
	  break;
	case 7:
	  mz = 0;
	  break;
	}

	
	mz = RGB2YCbCr(mz);
	vz = (float) t0(mz);
	vz = 0.20*vz + 0.80*vz*N[(int)(0x0f&ocube.val[p])];
	if (ocube.val[p]&0x80) vz*=0.90;
	mz = YCbCr2RGB(triplet((int)vz,t1(mz),t2(mz)));
	CImgSet(ocube.out, i, j, mz);
      }
    }
}

gboolean view3d_oct_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data)
{
  int W,i;
  view3d_octctl_render();

  W = w->allocation.width;
  i = ocube.out->W;

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     (W-i)/2,0,ocube.out->W,ocube.out->H,
		     GDK_RGB_DITHER_NORMAL,
		     ocube.out->val,
		     ocube.out->rowlen);

  ocube.ox = (W-i)/2;
  ocube.oy = 0;

  return FALSE;
}

void view3d_oct_action(int x,int y) {
  int p,ox,oy,oz;
  int W2,H2,D2;
  char v;

  x-=ocube.ox;
  y-=ocube.oy;

  if (!inbox(x,y,0,0,ocube.w,ocube.h)) return;
  p = x + y*ocube.w;
  
  v = ocube.val[p] & 0x0f;
  
  if (v < 1 || v > 6)
    return;
  
  if (!voldata.original) return;
  W2 = voldata.original->W / 2;
  H2 = voldata.original->H / 2;
  D2 = voldata.original->D / 2;
  
  ox = v2d.cx;
  oy = v2d.cy;
  oz = v2d.cz;
  
  switch(v) {
  case 1: case 2: // XY 
    ox = ocube.beta[p] + W2;
    oy = ocube.gamma[p] + H2;
    break;
  case 3: case 4: // XZ
    ox = ocube.beta[p] + W2;
    oz = ocube.gamma[p] + D2;
    break;
  case 5: case 6: // YZ
    oy = ocube.beta[p] + H2;
    oz = ocube.gamma[p] + D2;
    break;
  }
  v2d.cx = ox;
  v2d.cy = oy;
  v2d.cz = oz;
  ocube.pending = 1;
  force_redraw_2D();
}

gboolean view3d_oct_press(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  int x,y;
  x = (int) (eb->x);
  y = (int) (eb->y);
  if (eb->button == 1)
    view3d_oct_action(x,y);
  return FALSE;
}

gboolean view3d_oct_release(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  int x,y;
  x = (int) (eb->x);
  y = (int) (eb->y);
  if (eb->button == 1)
    view3d_oct_action(x,y);
  if (ocube.pending) {
    v3d.octant.invalid = 1;
    force_redraw_3D();
    ocube.pending = 0;
  }
  return FALSE;
}

gboolean view3d_oct_drag(GtkWidget *w, GdkEventMotion *em, gpointer data) {
  int x,y;
  x = (int) (em->x);
  y = (int) (em->y);
  if (em->state & GDK_BUTTON1_MASK)
    view3d_oct_action(x,y);
  return FALSE;
}

void view3d_find_skin(int *x, int *y, int *z) {

  int i,sx,sy,sz,w,h,v,bi,bv,px,py,pz,W,H,D,WH;
  int *rmap;
  Volume *vol;

  sx = *x;
  sy = *y;
  sz = *z;

  vol = voldata.original;

  w = v3d.skin.out->W;
  h = v3d.skin.out->H;
  rmap = skin_rmap(w,h,&(v3d.rotation),
		   v3d.panx / v3d.zoom,v3d.pany / v3d.zoom,
		   vol->W,vol->H,vol->D,
		   v3d.skin.tmap,v3d.skin.tcount);

  W = vol->W;
  H = vol->H;
  D = vol->D;
  WH = W*H;

  bi = -1;
  bv = W*W + H*H + D*D;

  for(i=0;i<w*h;i++) {
    if (rmap[i] < 0) continue;
    pz = rmap[i] / WH;
    py = (rmap[i] % WH) / W;
    px = (rmap[i] % WH) % W;

    v = (sx-px)*(sx-px) + (sy-py)*(sy-py) + (sz-pz)*(sz-pz);
    if (v < bv) { bv = v; bi = rmap[i]; }
  }

  if (bi>=0) {
    *z = bi / WH;
    *y = (bi % WH) / W;
    *x = (bi % WH) % W;
  }

  dist_add_full(sx,sy,sz,*x,*y,*z,vol);

  MemFree(rmap);
}

int * skin_rmap(int w,int h, Transform *rot, float panx, float pany,
		int W,int H,int D, int *map, int mapcount)
{
  int w2,h2,W2,H2,D2,WH;
  int i,j,k,n,p,q;
  float fa,fb;
  Point A,B;
  int *pm;
  float *depth;
  int *rmap;

  int px,py,pz;

  int spx[3] = {1,0,1};
  int spy[3] = {0,1,1};

  w2 = w/2;
  h2 = h/2;

  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  WH = W*H;

  /* init z-buffer and float (x,y) buffers (for normals) */
  depth     = (float *) MemAlloc( w * h * sizeof(float) );
  rmap = (int *) MemAlloc(w * h * sizeof(int) );

  if (!depth || !rmap) {
    error_memory();
    return NULL;
  }

  FloatFill(depth, MAGIC, w*h);
  for(i=0;i<w*h;i++) rmap[i] = -1;

  /* make projection */

  pm = map;
  for(i=0;i<mapcount;i++,pm++) {

    n = *pm;
    pz = n / WH;
    py = (n % WH) / W;
    px = (n % WH) % W;

    A.x = px - W2;
    A.y = py - H2;
    A.z = pz - D2;

    PointTransform(&B,&A,rot);
    px = w2 + B.x + panx;
    py = h2 + B.y + pany;

    if (px < 0 || py < 0 || px >= w || py >= h) continue;

    p = px + py*w;

    if (B.z < depth[p]) {
      depth[p] = B.z;
      rmap[p] = n;
    }
  }

  /* forward-only 2x2 splatting */

  for(j=h-2;j>=0;j--)
    for(i=w-2;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(k=0;k<3;k++) {
	  fb = depth[q = p + spx[k] + w*spy[k]];
	  if (fa < fb) {
	    depth[q] = fa;
	    rmap[q] = rmap[p];
	  }
	}
      }
    }

  MemFree(depth);
  return rmap;
}

