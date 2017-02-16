
#define SOURCE_VIEW2D_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

View2D v2d = { .base = {0,0,0}, .scaled = {0,0,0},
	       .panx = {0,0,0}, .pany = {0,0,0} };

void v2d_init() {
  int i;
  float fi;

  v2d.pmap     = 0;
  v2d.ptable   = (int *) MemAlloc(65536 * sizeof(int));
  if (!v2d.ptable)
    error_memory();

  v2d.zoom     = 1.0;
  v2d.invalid  = 1;

  v2d.npal = 6;
  v2d.palname = (char **) MemAlloc(sizeof(char *) * v2d.npal);
  v2d.palname[0] = "Gray";
  v2d.palname[1] = "Inverse";
  v2d.palname[2] = "Hotmetal";
  v2d.palname[3] = "Neon";
  v2d.palname[4] = "Red/Yellow";
  v2d.palname[5] = "Spectrum";
  v2d.paldata = (int **) MemAlloc(sizeof(int *) * v2d.npal);

  for(i=0;i<v2d.npal;i++)
    v2d.paldata[i] = (int *) MemAlloc(256 * sizeof(int));

  v2d.cpal = 0;
  v2d.pal = v2d.paldata[0];

  v2d.mx = v2d.my = v2d.mpane = -1;

  for(i=0;i<256;i++) {
    fi = (float) i;

    v2d.paldata[0][i] = gray(i);
    v2d.paldata[1][i] = gray(255-i);
    v2d.paldata[2][i] = triplet(MIN(255.0*fi/182.0,255.0),
				MAX(MIN(255.0*(fi-128.0)/91.0,255.0),0),
				MAX(MIN(255.0*(fi-192.0)/64.0,255.0),0));
    v2d.paldata[3][i] = 
      (i<128 ? mergeColorsRGB(0x000000,0x00ff00,fi/128.0) :
       mergeColorsRGB(0x00ff00,0xffffff,(fi-128.0)/128.0));
    v2d.paldata[4][i] = mergeColorsRGB(0x400000,0xffff80,fi/255.0);
    v2d.paldata[5][i] = HSV2RGB(triplet(180-180.0*(fi/255.0),255,255));
  }

  v2d.overlay[0] = v2d.overlay[1] = 0;

  for(i=0;i<3;i++)
    MemSet(v2d.info[i],0,256);
}

void v2d_ensure_label() {
  if (voldata.original!=NULL && voldata.label == NULL) {
    voldata.label = VolumeNew(voldata.original->W,
			      voldata.original->H,
			      voldata.original->D,
			      vt_integer_8);
    if (!voldata.label) {
      error_memory();
      return;
    }
    voldata.label->dx = voldata.original->dx;
    voldata.label->dy = voldata.original->dy;
    voldata.label->dz = voldata.original->dz;

    VolumeFill(voldata.label,0);
  }
}

void v2d_reset() {
  int i;

  for(i=0;i<3;i++)
    v2d.panx[i] = v2d.pany[i] = 0;

  if (v2d.base[0]!=NULL) {
    for(i=0;i<3;i++) {
      CImgDestroy(v2d.base[i]);
      v2d.base[i] = NULL;
    }
  }

  if (v2d.scaled[0] != NULL) {
    for(i=0;i<3;i++) {
      CImgDestroy(v2d.scaled[i]);
      v2d.scaled[i] = NULL;
    }
  }

  v2d_ensure_label();
  
  if (voldata.original!=NULL) {
    v2d.cx = voldata.original->W / 2;
    v2d.cy = voldata.original->H / 2;
    v2d.cz = voldata.original->D / 2;

    v2d.vmax = VolumeMax(voldata.original);
    if (v2d.vmax == 0) v2d.vmax = 1;
  }

  v2d.overlay[0] = v2d.overlay[1] = 0;
  v2d.invalid = 1;

  for(i=0;i<3;i++)
    MemSet(v2d.info[i],0,256);
}

void v2d_left(int i) {
  int *ip = 0;

  if (voldata.original==NULL) return;
  if (MutexTryLock(voldata.masterlock)!=0) return;

  switch(i) {
  case 0: ip = &(v2d.cz); break;
  case 1: ip = &(v2d.cx); break;
  case 2: ip = &(v2d.cy); break;
  }

  if (ip!=0) {
    if ((*ip) > 0) { 
      (*ip)--;
      v2d.invalid = 1;
      notify.viewchanged++;
      if (v3d.octant.enable)
	app_queue_octant_redraw();
    }
  }

  MutexUnlock(voldata.masterlock);
}

void v2d_right(int i) {
  int *ip=0,l=0;

  if (voldata.original==NULL) return;
  if (MutexTryLock(voldata.masterlock)!=0) return;
  
  switch(i) {
  case 0: ip = &(v2d.cz); l=voldata.original->D; break;
  case 1: ip = &(v2d.cx); l=voldata.original->W; break;
  case 2: ip = &(v2d.cy); l=voldata.original->H; break;
  }

  if (ip!=0) {
    if ((*ip) < l-1) { 
      (*ip)++;
      v2d.invalid = 1;
      notify.viewchanged++;
      if (v3d.octant.enable)
	app_queue_octant_redraw();
    }
  }
  MutexUnlock(voldata.masterlock);
}

void v2d_compose() {
  int i,j,k,m,W,H,D,da,db,p,c1,c2,mv;
  int *lookup;
  int *tbrow, *tbframe;
  char l;
  int ci,cf,cn,px,py,qx,qy,label,discardlookup;
  Distance *dt;
  Angle *at;
  int mx[3],my[3];
  char msg[64];

  float maskarea[3] = {0.0f, 0.0f, 0.0f}, maskavg[3] = {0.0f,0.0f,0.0f};
  int maskn[3] = {0,0,0}, maskmin[3] = {32767,32767,32767}, maskmax[3] = {-32768,-32768,-32768};

  static int tmpsz = 0;
  static char * tmp = 0;

  if (voldata.original==NULL || voldata.label==NULL) return;

  /* volume is being exchanged right now, no data to show */
  if (MutexTryLock(voldata.masterlock)!=0) {

    if (v2d.base[0]!=NULL)
      for(i=0;i<3;i++) CImgDestroy(v2d.base[i]);
    for(i=0;i<3;i++)
      v2d.base[i] = NULL;

    return;
  }

  if (v2d.pmap) {
    lookup = v2d.ptable;
    discardlookup = 0;
  } else {
    lookup = app_calc_lookup();
    if (lookup==NULL) {
      MutexUnlock(voldata.masterlock);
      return;
    }
    discardlookup = 1;
  }

  W = voldata.original->W;
  H = voldata.original->H;
  D = voldata.original->D;

  i = MAX(W*H,D*H);
  i = MAX(i,W*D);
  if (i > tmpsz) {
    if (tmp!=0) MemFree(tmp);
    tmpsz = i;
    tmp = (char *) MemAlloc(tmpsz);
    if (!tmp) {
      error_memory();
      MutexUnlock(voldata.masterlock);
      return;
    }
  }

  if (v2d.base[0]!=NULL)
    if (!CImgSizeEq(v2d.base[0],W,H)||
	!CImgSizeEq(v2d.base[1],D,H)||
	!CImgSizeEq(v2d.base[2],W,D)) {
      for(i=0;i<3;i++) {
	CImgDestroy(v2d.base[i]);
	v2d.base[i] = NULL;
      }
    }

  if (v2d.base[0] == NULL) {
    v2d.base[0] = CImgNew(W,H);
    v2d.base[1] = CImgNew(D,H);
    v2d.base[2] = CImgNew(W,D);

    if (!v2d.base[0] || !v2d.base[1] || !v2d.base[2]) {
      error_memory();
      MutexUnlock(voldata.masterlock);
      return;
    }

  }

  tbrow   = voldata.original->tbrow;
  tbframe = voldata.original->tbframe;

  /* view 1 */
  k = tbframe[v2d.cz];

  // build border/interior map
  MemSet(tmp, 0, W*H);
  da = 1;
  db = tbrow[1];
  for(j=1;j<H-1;j++) {
    m = tbrow[j];
    for(i=1;i<W-1;i++) {
      p = k+m+i;
      l = voldata.label->u.i8[p];
      if (l != voldata.label->u.i8[p+da] ||
	  l != voldata.label->u.i8[p-da] ||
	  l != voldata.label->u.i8[p+db] ||
	  l != voldata.label->u.i8[p-db])
	tmp[i+W*j] = 1;
    }
  }

  for(j=0;j<H;j++) {
    m = tbrow[j];
    for(i=0;i<W;i++) {
      p = k+m+i;
      c1 = v2d.pal[lookup[voldata.original->u.i16[p]]];

      if (voldata.label->u.i8[p] > 0) {

	maskn[0]++;
	mv = voldata.original->u.i16[p];
	maskavg[0] += mv;
	if (mv < maskmin[0]) maskmin[0] = mv;
	if (mv > maskmax[0]) maskmax[0] = mv;

	c2 = labels.colors[voldata.label->u.i8[p]-1];
	if (tmp[i+W*j]==0)
	  c1 = mergeColorsRGB(c1,c2,gui.ton->value);
	else
	  c1 = c2;
      }

      CImgSet(v2d.base[0], i,j, c1);
    }
  }

  maskarea[0] = maskn[0] * voldata.original->dx * voldata.original->dy;
  if (maskn[0] > 0) maskavg[0] /= maskn[0];

  /* view 2 */
  k = v2d.cx;

  // build border/interior map
  MemSet(tmp, 0, D*H);
  da = tbframe[1];
  db = tbrow[1];
  for(j=1;j<H-1;j++) {
    m = tbrow[j];
    for(i=1;i<D-1;i++) {
      p = k+m+tbframe[i];
      l = voldata.label->u.i8[p];
      if (l != voldata.label->u.i8[p+da] ||
	  l != voldata.label->u.i8[p-da] ||
	  l != voldata.label->u.i8[p+db] ||
	  l != voldata.label->u.i8[p-db])
	tmp[i+D*j] = 1;
    }
  }

  for(j=0;j<H;j++) {
    m = tbrow[j];
    for(i=0;i<D;i++) {
      p = k+m+tbframe[i];

      c1 = v2d.pal[lookup[voldata.original->u.i16[p]]];

      if (voldata.label->u.i8[p] > 0) {

	maskn[1]++;
	mv = voldata.original->u.i16[p];
	maskavg[1] += mv;
	if (mv < maskmin[1]) maskmin[1] = mv;
	if (mv > maskmax[1]) maskmax[1] = mv;

	c2 = labels.colors[voldata.label->u.i8[p]-1];
	if (tmp[i+D*j]==0)
	  c1 = mergeColorsRGB(c1,c2,gui.ton->value);
	else
	  c1 = c2;
      }

      CImgSet(v2d.base[1], i,j, c1);
    }
  }

  maskarea[1] = maskn[1] * voldata.original->dz * voldata.original->dy;
  if (maskn[1] > 0) maskavg[1] /= maskn[1];

  /* view 3 */
  k = tbrow[v2d.cy];

  // build border/interior map
  MemSet(tmp, 0, W*D);
  da = 1;
  db = tbframe[1];
  for(j=1;j<D-1;j++) {
    m = tbframe[j];
    for(i=1;i<W-1;i++) {
      p = k+m+i;
      l = voldata.label->u.i8[p];
      if (l != voldata.label->u.i8[p+da] ||
	  l != voldata.label->u.i8[p-da] ||
	  l != voldata.label->u.i8[p+db] ||
	  l != voldata.label->u.i8[p-db])
	tmp[i+W*j] = 1;
    }
  }

  for(j=0;j<D;j++) {
    m = tbframe[j];
    for(i=0;i<W;i++) {
      p = k+m+i;

      c1 = v2d.pal[lookup[voldata.original->u.i16[p]]];

      if (voldata.label->u.i8[p] > 0) {

	maskn[2]++;
	mv = voldata.original->u.i16[p];
	maskavg[2] += mv;
	if (mv < maskmin[2]) maskmin[2] = mv;
	if (mv > maskmax[2]) maskmax[2] = mv;

	c2 = labels.colors[voldata.label->u.i8[p]-1];
	if (tmp[i+W*j]==0)
	  c1 = mergeColorsRGB(c1,c2,gui.ton->value);
	else
	  c1 = c2;
      }

      CImgSet(v2d.base[2], i,j, c1);
    }
  }

  maskarea[2] = maskn[2] * voldata.original->dx * voldata.original->dz;
  if (maskn[2] > 0) maskavg[2] /= maskn[2];

  // draw contour, if any
  
  cn = contour_get_count();
  if (cn) {
    ci = contour_get_orientation();
    cf = contour_get_frame();
    label = labels.current + 1;

    switch(ci) {
    case 0:
      if (cf == v2d.cz) {
	contour_get_point(0, &px,&py);
	CImgSet(v2d.base[0], px,py, labels.colors[label-1]);
	for(i=0;i<cn-1;i++) {
	  contour_get_point(i, &px,&py);
	  contour_get_point((i+1)%cn, &qx,&qy);
	  CImgDrawLine(v2d.base[0], px,py, qx,qy, labels.colors[label-1]);
	}
      }
      break;
    case 1:
      if (cf == v2d.cx) {
	contour_get_point(0, &px,&py);
	CImgSet(v2d.base[1], px,py, labels.colors[label-1]);
	for(i=0;i<cn-1;i++) {
	  contour_get_point(i, &px,&py);
	  contour_get_point((i+1)%cn, &qx,&qy);
	  CImgDrawLine(v2d.base[1], px,py, qx,qy, labels.colors[label-1]);
	}
      }
      break;
    case 2:
      if (cf == v2d.cy) {
	contour_get_point(0, &px,&py);
	CImgSet(v2d.base[2], px,py, labels.colors[label-1]);
	for(i=0;i<cn-1;i++) {
	  contour_get_point(i, &px,&py);
	  contour_get_point((i+1)%cn, &qx,&qy);
	  CImgDrawLine(v2d.base[2], px,py, qx,qy, labels.colors[label-1]);
	}
      }
      break;
    }
  }

  if (v2d.scaled[0]!=NULL) {
    for(i=0;i<3;i++) {
      CImgDestroy(v2d.scaled[i]);
      v2d.scaled[i] = NULL;
    }
  }

  for(i=0;i<3;i++)
    if (view.zopane < 0 || v2d.zoom < 2.0)
      v2d.scaled[i] = CImgScale(v2d.base[i], v2d.zoom);
    else
      v2d.scaled[i] = CImgFastScale(v2d.base[i], v2d.zoom);

  // draw distances

  cn = dist_n();

  for(i=0;i<cn;i++) {

    dt = dist_get(i);

    if (dt->done >= 2)
      snprintf(msg, 64, "%.1f mm", dt->value);

    // draw in pane 1 ?
    if ( (dt->done >= 1 && dt->z[0] == v2d.cz) || (dt->done >= 2 && dt->z[1] == v2d.cz) ) {

      mx[0] = (int) (((float) dt->x[0]) * v2d.zoom);
      my[0] = (int) (((float) dt->y[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[0], mx[0], my[0], 3, view.measures);
      
      if (dt->z[0] != v2d.cz) 
	CImgDrawCircle(v2d.scaled[0], mx[0], my[0], 5, view.measures);


      if (dt->done >= 2) {
	mx[1] = (int) (((float) dt->x[1]) * v2d.zoom);
	my[1] = (int) (((float) dt->y[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[0], mx[1], my[1], 3, view.measures);      

	if (dt->z[1] != v2d.cz) 
	  CImgDrawCircle(v2d.scaled[0], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[0], mx[0], my[0], mx[1], my[1], view.measures);
	CImgDrawShieldedSText(v2d.scaled[0], res.font[4],
			      (mx[0]+mx[1])/2,
			      (my[0]+my[1])/2,
			      view.measures,0,msg);
      }

    }

    // draw in pane 2 ?
    if ( (dt->done >= 1 && dt->x[0] == v2d.cx) || (dt->done >= 2 && dt->x[1] == v2d.cx) ) {

      mx[0] = (int) (((float) dt->z[0]) * v2d.zoom);
      my[0] = (int) (((float) dt->y[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[1], mx[0], my[0], 3, view.measures);
      
      if (dt->x[0] != v2d.cx) 
	CImgDrawCircle(v2d.scaled[1], mx[0], my[0], 5, view.measures);


      if (dt->done >= 2) {
	mx[1] = (int) (((float) dt->z[1]) * v2d.zoom);
	my[1] = (int) (((float) dt->y[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[1], mx[1], my[1], 3, view.measures);      

	if (dt->x[1] != v2d.cx) 
	  CImgDrawCircle(v2d.scaled[1], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[1], mx[0], my[0], mx[1], my[1], view.measures);
	CImgDrawShieldedSText(v2d.scaled[1], res.font[4],
			      (mx[0]+mx[1])/2,
			      (my[0]+my[1])/2,
			      view.measures,0,msg);
      }

    }

    // draw in pane 3 ?
    if ( (dt->done >= 1 && dt->y[0] == v2d.cy) || (dt->done >= 2 && dt->y[1] == v2d.cy) ) {

      mx[0] = (int) (((float) dt->x[0]) * v2d.zoom);
      my[0] = (int) (((float) dt->z[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[2], mx[0], my[0], 3, view.measures);
      
      if (dt->y[0] != v2d.cy) 
	CImgDrawCircle(v2d.scaled[2], mx[0], my[0], 5, view.measures);


      if (dt->done >= 2) {
	mx[1] = (int) (((float) dt->x[1]) * v2d.zoom);
	my[1] = (int) (((float) dt->z[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[2], mx[1], my[1], 3, view.measures);      

	if (dt->y[1] != v2d.cy) 
	  CImgDrawCircle(v2d.scaled[2], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[2], mx[0], my[0], mx[1], my[1], view.measures);
	CImgDrawShieldedSText(v2d.scaled[2], res.font[4],
			      (mx[0]+mx[1])/2,
			      (my[0]+my[1])/2,
			      view.measures,0,msg);
      }

    }
  }

  // draw angles
  
  cn = angle_n();

  for(i=0;i<cn;i++) {

    at = angle_get(i);
    mx[1]=my[1]=0;

    if (at->done >= 3)
      snprintf(msg, 64, "%.1f\260", at->value);

    // draw in pane 1 ?
    if ( (at->done >= 1 && at->z[0] == v2d.cz) || 
	 (at->done >= 2 && at->z[1] == v2d.cz) ||
	 (at->done >= 3 && at->z[2] == v2d.cz)) {

      mx[0] = (int) (((float) at->x[0]) * v2d.zoom);
      my[0] = (int) (((float) at->y[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[0], mx[0], my[0], 3, view.measures);
      
      if (at->z[0] != v2d.cz) 
	CImgDrawCircle(v2d.scaled[0], mx[0], my[0], 5, view.measures);

      if (at->done >= 2) {
	mx[1] = (int) (((float) at->x[1]) * v2d.zoom);
	my[1] = (int) (((float) at->y[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[0], mx[1], my[1], 3, view.measures);

	if (at->z[1] != v2d.cz) 
	  CImgDrawCircle(v2d.scaled[0], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[0], mx[0], my[0], mx[1], my[1], view.measures);
      }

      if (at->done >= 3) {
	mx[2] = (int) (((float) at->x[2]) * v2d.zoom);
	my[2] = (int) (((float) at->y[2]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[0], mx[2], my[2], 3, view.measures);

	if (at->z[2] != v2d.cz) 
	  CImgDrawCircle(v2d.scaled[0], mx[2], my[2], 5, view.measures);

	CImgDrawLine(v2d.scaled[0], mx[0], my[0], mx[2], my[2], view.measures);
	CImgDrawShieldedSText(v2d.scaled[0], res.font[4],
			      (mx[0]+mx[1]+mx[2])/3,
			      (my[0]+my[1]+my[2])/3,
			      view.measures,0,msg);
      }
    }

    // draw in pane 2 ?
    if ( (at->done >= 1 && at->x[0] == v2d.cx) || 
	 (at->done >= 2 && at->x[1] == v2d.cx) ||
	 (at->done >= 3 && at->x[2] == v2d.cx)) {

      mx[0] = (int) (((float) at->z[0]) * v2d.zoom);
      my[0] = (int) (((float) at->y[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[1], mx[0], my[0], 3, view.measures);
      
      if (at->x[0] != v2d.cx) 
	CImgDrawCircle(v2d.scaled[1], mx[0], my[0], 5, view.measures);

      if (at->done >= 2) {
	mx[1] = (int) (((float) at->z[1]) * v2d.zoom);
	my[1] = (int) (((float) at->y[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[1], mx[1], my[1], 3, view.measures);

	if (at->x[1] != v2d.cx) 
	  CImgDrawCircle(v2d.scaled[1], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[1], mx[0], my[0], mx[1], my[1], view.measures);
      }

      if (at->done >= 3) {
	mx[2] = (int) (((float) at->z[2]) * v2d.zoom);
	my[2] = (int) (((float) at->y[2]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[1], mx[2], my[2], 3, view.measures);

	if (at->x[2] != v2d.cx) 
	  CImgDrawCircle(v2d.scaled[1], mx[2], my[2], 5, view.measures);

	CImgDrawLine(v2d.scaled[1], mx[0], my[0], mx[2], my[2], view.measures);
	CImgDrawShieldedSText(v2d.scaled[1], res.font[4],
			      (mx[0]+mx[1]+mx[2])/3,
			      (my[0]+my[1]+my[2])/3,
			      view.measures,0,msg);
      }
    }

    // draw in pane 3 ?
    if ( (at->done >= 1 && at->y[0] == v2d.cy) || 
	 (at->done >= 2 && at->y[1] == v2d.cy) ||
	 (at->done >= 3 && at->y[2] == v2d.cy)) {

      mx[0] = (int) (((float) at->x[0]) * v2d.zoom);
      my[0] = (int) (((float) at->z[0]) * v2d.zoom);
      
      CImgDrawCircle(v2d.scaled[2], mx[0], my[0], 3, view.measures);
      
      if (at->y[0] != v2d.cy) 
	CImgDrawCircle(v2d.scaled[2], mx[0], my[0], 5, view.measures);

      if (at->done >= 2) {
	mx[1] = (int) (((float) at->x[1]) * v2d.zoom);
	my[1] = (int) (((float) at->z[1]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[2], mx[1], my[1], 3, view.measures);

	if (at->y[1] != v2d.cy) 
	  CImgDrawCircle(v2d.scaled[2], mx[1], my[1], 5, view.measures);

	CImgDrawLine(v2d.scaled[2], mx[0], my[0], mx[1], my[1], view.measures);
      }

      if (at->done >= 3) {
	mx[2] = (int) (((float) at->x[2]) * v2d.zoom);
	my[2] = (int) (((float) at->z[2]) * v2d.zoom);

	CImgDrawCircle(v2d.scaled[2], mx[2], my[2], 3, view.measures);

	if (at->y[2] != v2d.cy) 
	  CImgDrawCircle(v2d.scaled[2], mx[2], my[2], 5, view.measures);

	CImgDrawLine(v2d.scaled[2], mx[0], my[0], mx[2], my[2], view.measures);
	CImgDrawShieldedSText(v2d.scaled[2], res.font[4],
			      (mx[0]+mx[1]+mx[2])/3,
			      (my[0]+my[1]+my[2])/3,
			      view.measures,0,msg);
      }
    }
  }

  // draw mask stats

  if (maskn[0] > 0)
    sprintf(v2d.info[0],"slice mask area %.1f mm² avg %.1f range %d-%d",maskarea[0],maskavg[0],maskmin[0],maskmax[0]);
  else 
    v2d.info[0][0] = 0;

  if (maskn[1] > 0)
    sprintf(v2d.info[1],"mask area %.1f mm² avg %.1f range %d-%d",maskarea[1],maskavg[1],maskmin[1],maskmax[1]);
  else
    v2d.info[1][0] = 0;

  if (maskn[2] > 0)
    sprintf(v2d.info[2],"mask area %.1f mm² avg %.1f range %d-%d",maskarea[2],maskavg[2],maskmin[2],maskmax[2]);
  else
    v2d.info[2][0] = 0;

  // draw roi overlay, if active

  v2d_roi_overlay();

  v2d.invalid = 0;
  if (discardlookup)
    MemFree(lookup);
  MutexUnlock(voldata.masterlock);
}

void v2d_roi_overlay() {
  CImg *tmp;
  int x,y,w,h;
  if (!roi.active) return;

  tmp = CImgNew(v2d.scaled[0]->W,v2d.scaled[0]->H);
  CImgFullCopy(tmp,v2d.scaled[0]);
  x = v2d.zoom * roi.x[0];
  y = v2d.zoom * roi.y[0];
  w = v2d.zoom*(roi.x[1]-roi.x[0]+1);
  h =  v2d.zoom*(roi.y[1]-roi.y[0]+1);
  CImgFillRect(tmp,x,y,w,h,0xff0000);
  CImgBitBlm(v2d.scaled[0],0,0,tmp,0,0,tmp->W,tmp->H,0.30);
  CImgDestroy(tmp);
  CImgDrawRect(v2d.scaled[0],x,y,w,h,0xff0000);
  CImgFillRect(v2d.scaled[0],x-4,y-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[0],x-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[0],x+w-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[0],x+w-4,y-4,8,8,0xff0000);

  tmp = CImgNew(v2d.scaled[1]->W,v2d.scaled[1]->H);
  CImgFullCopy(tmp,v2d.scaled[1]);
  x = v2d.zoom * roi.z[0];
  y = v2d.zoom * roi.y[0];
  w = v2d.zoom*(roi.z[1]-roi.z[0]+1);
  h =  v2d.zoom*(roi.y[1]-roi.y[0]+1);
  CImgFillRect(tmp,x,y,w,h,0xff0000);
  CImgBitBlm(v2d.scaled[1],0,0,tmp,0,0,tmp->W,tmp->H,0.30);
  CImgDestroy(tmp);
  CImgDrawRect(v2d.scaled[1],x,y,w,h,0xff0000);
  CImgFillRect(v2d.scaled[1],x-4,y-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[1],x-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[1],x+w-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[1],x+w-4,y-4,8,8,0xff0000);

  tmp = CImgNew(v2d.scaled[2]->W,v2d.scaled[2]->H);
  CImgFullCopy(tmp,v2d.scaled[2]);
  x = v2d.zoom * roi.x[0];
  y = v2d.zoom * roi.z[0];
  w = v2d.zoom*(roi.x[1]-roi.x[0]+1);
  h =  v2d.zoom*(roi.z[1]-roi.z[0]+1);
  CImgFillRect(tmp,x,y,w,h,0xff0000);
  CImgBitBlm(v2d.scaled[2],0,0,tmp,0,0,tmp->W,tmp->H,0.30);
  CImgDestroy(tmp);
  CImgDrawRect(v2d.scaled[2],x,y,w,h,0xff0000);
  CImgFillRect(v2d.scaled[2],x-4,y-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[2],x-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[2],x+w-4,y+h-4,8,8,0xff0000);
  CImgFillRect(v2d.scaled[2],x+w-4,y-4,8,8,0xff0000);
}

/* volume -> screen coords */
void v2d_v2s(int x,int y,int z,int i, int *ox, int *oy) {
  int bx,by;
  float fx,fy,fz;

  if (voldata.original==NULL) {
    *ox = 0;
    *oy = 0;
    return;
  }

  bx = (view.bw[i] / 2) - v2d.scaled[i]->W / 2 + v2d.panx[i];
  by = (view.bh[i] / 2) - v2d.scaled[i]->H / 2 + v2d.pany[i];

  fx = (float) x;
  fy = (float) y;
  fz = (float) z;

  switch(i) {
  case 0: bx += (int) (fx * v2d.zoom); by += (int) (fy * v2d.zoom); break;
  case 1: bx += (int) (fz * v2d.zoom); by += (int) (fy * v2d.zoom); break;
  case 2: bx += (int) (fx * v2d.zoom); by += (int) (fz * v2d.zoom); break;
  }
  *ox = bx;
  *oy = by;
}

/* screen -> volume coords */
void v2d_s2v(int x,int y,int i,int *ox, int *oy, int *oz) {
  int bx,by;
  float fx,fy;

  if (voldata.original==NULL) {
    *ox = 0;
    *oy = 0;
    *oz = 0;
    return;
  }

  bx = (view.bw[i] / 2) - v2d.scaled[i]->W / 2 + v2d.panx[i];
  by = (view.bh[i] / 2) - v2d.scaled[i]->H / 2 + v2d.pany[i];

  bx = x-bx;
  by = y-by;

  fx = ((float) bx) / v2d.zoom;
  fy = ((float) by) / v2d.zoom;
  
  switch(i) {
  case 0:
    *ox = (int) fx;
    *oy = (int) fy;
    *oz = v2d.cz;
    break;
  case 1:
    *oz = (int) fx;
    *oy = (int) fy;
    *ox = v2d.cx;
    break;
  case 2:
    *ox = (int) fx;
    *oz = (int) fy;
    *oy = v2d.cy;
    break;
  }
}

void v2d_purge(int x,int y,int z,int pane,int update) {
  int i,j,W,H,D,WH;

  W = voldata.label->W;
  H = voldata.label->H;
  D = voldata.label->D;
  WH = W*H;

  switch(pane) {

  case 0:
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	voldata.label->u.i8[i+W*j+z*WH] = 0;
    break;
  case 1:
    for(i=0;i<D;i++)
      for(j=0;j<H;j++)
	voldata.label->u.i8[x+W*j+i*WH] = 0;
    break;
  case 2:
    for(j=0;j<D;j++)
      for(i=0;i<W;i++)
	voldata.label->u.i8[i+W*y+j*WH] = 0;
  }
  v2d.invalid = 1;
  if (v3d.obj.enable)
    app_queue_obj_redraw();
  if (update)
    refresh(cw);
}

void v2d_flood(int x,int y,int z,int pane,int update,int label) {
  //  XYZX
  static int nx[8] = { -1, 1, 0, 0, 0, 0, -1, 1 };
  static int ny[8] = {  0, 0,-1, 1, 0, 0,  0, 0 };
  static int nz[8] = {  0, 0, 0, 0,-1, 1,  0, 0 };
  int i,j,k,base,px,py,pz,qx,qy,qz;
  int W,H,D,WH;

  int *fifo, putp=0, getp=0;
  int md;

  W = voldata.label->W;
  H = voldata.label->H;
  D = voldata.label->D;
  WH = W*H;
  md = MAX(MAX(W,H),D);

  fifo = (int *) MemAlloc(md * md * sizeof(int));
  if (!fifo) return;
  putp = getp = 0;

  fifo[putp++] = i = x + y*W + z*WH;
  voldata.label->u.i8[i] = label;

  switch(pane) {
  case 0: // xy plane
    base = 0; break;
  case 1: // yz plane
    base = 2; break;
  case 2: // xz plane
    base = 4; break;
  default: // never
    MemFree(fifo);
    return;
  }

  while(getp!=putp) {
    i = fifo[getp++];
    pz = i / WH;
    py = (i % WH) / W;
    px = (i % WH) % W;

    for(j=0;j<4;j++) {
      qx = px + nx[base+j];
      qy = py + ny[base+j];
      qz = pz + nz[base+j];
      
      if (qx < 0 || qy < 0 || qz < 0 || qx >= W || qy >= H || qz >= D)
	continue;

      k = qx + qy*W + qz*WH;

      if (voldata.label->u.i8[k] != label) {
	voldata.label->u.i8[k] = label;
	fifo[putp++] = k;
      }
    }
  }

  MemFree(fifo);

  v2d.invalid = 1;
  if (v3d.obj.enable)
    app_queue_obj_redraw();
  if (update)
    refresh(cw);
}

void v2d_pen_cursor(GdkPixmap *dest, GdkGC *gc) {
  // v2d. mx,my,mpane
  int x,y,z,R,a,b,W,H,D,x1,x2,y1,y2;
  int sum, n, *tbrow, *tbframe;
  double avg;
  char msg[256];

  if (voldata.original == NULL) return;
  W = voldata.original->W;
  H = voldata.original->H;
  D = voldata.original->D;
  tbrow   = voldata.original->tbrow;
  tbframe = voldata.original->tbframe;
  msg[0] = 0;

  v2d_s2v(v2d.mx-view.bx[v2d.mpane],v2d.my-view.by[v2d.mpane],v2d.mpane,&x,&y,&z);

  if (x>=0 && y>=0 && z>=0 && x<W && y<H && z<D) 
    {
      R = gui.tpen->selected;

      switch(gui.t2d->selected) {
      case 3:
	gc_color(gc,0x6666ff);
	break;
      case 4:
	gc_color(gc,0xff6666);
	break;
      default:
	gc_color(gc,0xffff00);
      }

      sum=n=0;
      
      switch(v2d.mpane) {

      case 0: // XY
	for(b=-R;b<=R;b++)
	  for(a=-R;a<=R;a++) {
	    if ((a==0 && b==0) || (a*a+b*b <= R*R))
	      if (x+a >= 0 && x+a < W && y+b >= 0 && y+b < H) {
		sum += voldata.original->u.i16[(x+a) + tbrow[(y+b)] + tbframe[z]];
		n++;
		v2d_v2s(x+a,y+b,z, v2d.mpane, &x1, &y1);
		v2d_v2s(x+a+1,y+b+1,z, v2d.mpane, &x2, &y2);
		gdk_draw_rectangle(dest,gc,FALSE,view.bx[v2d.mpane]+x1,view.by[v2d.mpane]+y1,x2-x1,y2-y1);
	      }
	  }
	break;
      case 1: // ZY
	for(b=-R;b<=R;b++)
	  for(a=-R;a<=R;a++) {
	    if ((a==0 && b==0) || (a*a+b*b <= R*R))
	      if (z+a >= 0 && z+a < D && y+b >= 0 && y+b < H) {
		sum += voldata.original->u.i16[(x) + tbrow[(y+b)] + tbframe[(z+a)]];
		n++;
		v2d_v2s(x,y+b,z+a, v2d.mpane, &x1, &y1);
		v2d_v2s(x,y+b+1,z+a+1, v2d.mpane, &x2, &y2);
		gdk_draw_rectangle(dest,gc,FALSE,view.bx[v2d.mpane]+x1,view.by[v2d.mpane]+y1,x2-x1,y2-y1);
	      }
	  }
	break;
      case 2: // XZ
	for(b=-R;b<=R;b++)
	  for(a=-R;a<=R;a++) {
	    if ((a==0 && b==0) || (a*a+b*b <= R*R))
	      if (x+a >= 0 && x+a < W && z+b >= 0 && z+b < D) {
		sum += voldata.original->u.i16[(x+a) + tbrow[(y)] + tbframe[(z+b)]];
		n++;
		v2d_v2s(x+a,y,z+b, v2d.mpane, &x1, &y1);
		v2d_v2s(x+a+1,y,z+b+1, v2d.mpane, &x2, &y2);
		gdk_draw_rectangle(dest,gc,FALSE,view.bx[v2d.mpane]+x1,view.by[v2d.mpane]+y1,x2-x1,y2-y1);
	      }
	  }
	break;
      }

      if (n>0) {
	avg = ((double) sum) / ((double) n);
	if (n > 1)
	  sprintf(msg,"Mask center (%d,%d,%d), Average Intensity %.2f (%d voxels).",x+1,y+1,z+1,avg,n);
	else
	  sprintf(msg,"Voxel (%d,%d,%d), Intensity %d.",x+1,y+1,z+1,sum);
      }
      
    }
  
  app_set_status(msg,20);
}

void v2d_trace(int x,int y,int z,int i,int update,int label,int sphere) {
  int R;
  int a,b,c,W,H,D,WH,c0,c1;

  R=gui.tpen->selected;

  W = voldata.original->W;
  H = voldata.original->H;
  D = voldata.original->D;
  WH = W*H;

  if (sphere) { c0 = -R; c1 = R; } else { c0=c1=0; }

  switch(i) {

  case 0: // XY
    for(c=c0;c<=c1;c++)
      for(b=-R;b<=R;b++)
	for(a=-R;a<=R;a++) {
	  if ((a==0 && b==0 && c==0) || (a*a+b*b+c*c <= R*R))
	    if (x+a >= 0 && x+a < W &&
		y+b >= 0 && y+b < H &&
		z+c >= 0 && z+c < D)
	      voldata.label->u.i8[ (z+c)*WH + (y+b)*W + (x+a) ] = label;
	  
	}
    break;
  case 1: // ZY
    for(c=c0;c<=c1;c++)
      for(b=-R;b<=R;b++)
	for(a=-R;a<=R;a++) {
	  if ((a==0 && b==0 && c==0) || (a*a+b*b+c*c <= R*R))
	    if (z+a >= 0 && z+a < D &&
		y+b >= 0 && y+b < H &&
		x+c >= 0 && x+c < W)
	      voldata.label->u.i8[ (x+c) + (y+b)*W + (z+a)*WH ] = label;
	}
    break;
  case 2: // XZ
    for(c=c0;c<=c1;c++)
      for(b=-R;b<=R;b++)
	for(a=-R;a<=R;a++) {
	  if ((a==0 && b==0 && c==0) || (a*a+b*b+c*c <= R*R))
	    if (x+a >= 0 && x+a < W &&
		z+b >= 0 && z+b < D &&
		y+c >= 0 && y+c < H)
	      voldata.label->u.i8[ (z+b)*WH + (y+c)*W + (x+a) ] = label;
	}
    break;
  }
  
  v2d.invalid = 1;
  
  if (v3d.obj.enable)
    app_queue_obj_redraw();
  
  if (update)
    refresh(cw);
}


