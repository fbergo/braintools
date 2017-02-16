
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

struct _undo {
  int ready;
  int orient;
  int frame;
  i8_t *data;
} undo;

void undo_init() {
  undo.data = 0;
  undo.ready = 0;
  undo.orient = 0;
  undo.frame = 0;
}

void undo_reset() {
  if (undo.data) {
    MemFree(undo.data);
    undo.data = 0;
  }
  undo.ready = 0;
  undo.orient = 0;
  undo.frame = 0;
}

void undo_pop() {
  int i,j,k,W,H,vw,vh,vd,vwh;

  if (!undo.ready) return;
  if (!voldata.label) return;
  MutexLock(voldata.masterlock);

  vw = voldata.label->W;
  vh = voldata.label->H;
  vd = voldata.label->D;
  vwh = vw*vh;

  if (undo.ready) {
    switch(undo.orient) {
    case 0:
      W = vw;
      H = vh;
      k = undo.frame;
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  voldata.label->u.i8[i+j*vw+k*vwh] = undo.data[i+j*W];
      break;
    case 1:
      W = vd;
      H = vh;
      k = undo.frame;
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  voldata.label->u.i8[k+j*vw+i*vwh] = undo.data[i+j*W];
       break;
    case 2:
      W = vw;
      H = vd;
      k = undo.frame;
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  voldata.label->u.i8[i+k*vw+j*vwh] = undo.data[i+j*W];
      break;
    }
  }
  undo_reset();
  undo_update();
  force_redraw_2D();
  if (v3d.obj.enable) {
    view3d_objects_trash();
    force_redraw_3D();
  }
  MutexUnlock(voldata.masterlock);
}

void undo_push(int orient) {
  int i,j,k,W,H,vw,vh,vd,vwh;

  undo_reset();
  if (!voldata.label) return;
  MutexLock(voldata.masterlock);

  vw = voldata.label->W;
  vh = voldata.label->H;
  vd = voldata.label->D;
  vwh = vw*vh;

  undo.orient = orient;
  switch(orient) {
  case 0:
    W = vw;
    H = vh;
    undo.frame = k = v2d.cz;
    undo.data  = (i8_t *) MemAlloc(W*H);
    if (!undo.data) break;
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	undo.data[i+j*W] = voldata.label->u.i8[i+j*vw+k*vwh];
    undo.ready = 1;
    break;
  case 1:
    W = vd;
    H = vh;
    undo.frame = k = v2d.cx;
    undo.data = (i8_t *) MemAlloc(W*H);
    if (!undo.data) break;
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	undo.data[i+j*W] = voldata.label->u.i8[k+j*vw+i*vwh];
    undo.ready = 1;
    break;
  case 2:
    W = vw;
    H = vd;
    undo.frame = k = v2d.cy;
    undo.data  = (i8_t *) MemAlloc(W*H);
    if (!undo.data) break;
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	undo.data[i+j*W] = voldata.label->u.i8[i+k*vw+j*vwh];
    undo.ready = 1;
    break;
  }
  MutexUnlock(voldata.masterlock);
  undo_update();
}

void undo_update() {
  gtk_widget_set_sensitive(gui.edit_undo, undo.ready ? TRUE:FALSE);
}

