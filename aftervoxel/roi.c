
#define SOURCE_ROI_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

ROIdata roi = { .active = 0 };

void roi_tool() {
  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  roi.x[0] = 0;
  roi.y[0] = 0;
  roi.z[0] = 0;
  roi.x[1] = voldata.original->W - 1;
  roi.y[1] = voldata.original->H - 1;
  roi.z[1] = voldata.original->D - 1;
  roi.active = 1;

  roi_label_update();

  gtk_widget_show(gui.roipanel);
  force_redraw_2D();
  ToolbarDisable(gui.t3d,3,0);
  app_t2d_changed(0);
}

void roi_clip(void *args) {
  roi_cancel(0);
  measure_kill_all();
  undo_reset();
  undo_update();
  TaskNew(res.bgqueue,"Clipping ROI...",(Callback) &roi_compute,0);
}

void roi_compute(void *args) {
  int x[2],y[2],z[2];
  Volume *v,*t;
  int j,k,nw,nh,nd,w,h,p,q;

  x[0] = roi.x[0];
  x[1] = roi.x[1];
  y[0] = roi.y[0];
  y[1] = roi.y[1];
  z[0] = roi.z[0];
  z[1] = roi.z[1];

  MutexLock(voldata.masterlock);

  nw = x[1] - x[0] + 1;
  nh = y[1] - y[0] + 1;
  nd = z[1] - z[0] + 1;

  SubTaskInit(3);
  SubTaskSetText(0,"Clipping Original...");
  SubTaskSetText(0,"Clipping Objects...");
  SubTaskSetText(0,"Updating Reslicing...");

  SetProgress(0,nd);
  SubTaskSet(0);
  v = VolumeNew(nw,nh,nd,vt_integer_16);
  w = voldata.original->W;
  h = voldata.original->H;

  for(k=0;k<nd;k++) {
    SetProgress(k,nd);
    for(j=0;j<nh;j++) {
      p = (x[0]) + (j+y[0])*w + (k+z[0])*w*h;
      q = j*nw + k*nw*nh;
      MemCpy(v->u.i16+q,voldata.original->u.i16+p,2*nw);
    }
  }
  
  t = voldata.original;
  voldata.original = v;
  VolumeDestroy(t);

  SetProgress(0,nd);
  SubTaskSet(1);
  v = VolumeNew(nw,nh,nd,vt_integer_8);
  w = voldata.label->W;
  h = voldata.label->H;

  for(k=0;k<nd;k++) {
    SetProgress(k,nd);
    for(j=0;j<nh;j++) {
      p = (x[0]) + (j+y[0])*w + (k+z[0])*w*h;
      q = j*nw + k*nw*nh;
      MemCpy(v->u.i8+q,voldata.label->u.i8+p,nw);
    }
  }  

  t = voldata.label;
  voldata.label = v;
  VolumeDestroy(t);

  SetProgress(0,3);
  SubTaskSet(2);

  SetProgress(1,3);
  SetProgress(2,3);
  EndProgress();

  v2d.cx = nw / 2;
  v2d.cy = nh / 2;
  v2d.cz = nd / 2;

  notify.segchanged++;
  view3d_invalidate(1,1);
  MutexUnlock(voldata.masterlock);
}

void roi_edit(int x,int y,int z,int pane) {
  int t;

  if (pane!=1)
    roi.x[abs(x-roi.x[0]) < abs(x-roi.x[1])?0:1] = x;

  if (pane!=2)
    roi.y[abs(y-roi.y[0]) < abs(y-roi.y[1])?0:1] = y;

  if (pane!=0)
    roi.z[abs(z-roi.z[0]) < abs(z-roi.z[1])?0:1] = z;
  
  if (roi.x[0] > roi.x[1]) {
    t = roi.x[0]; roi.x[0] = roi.x[1]; roi.x[1] = t;
  }
  if (roi.y[0] > roi.y[1]) {
    t = roi.y[0]; roi.y[0] = roi.y[1]; roi.y[1] = t;
  }
  if (roi.z[0] > roi.z[1]) {
    t = roi.z[0]; roi.z[0] = roi.z[1]; roi.z[1] = t;
  }
  roi_label_update();
}

void roi_cancel(void *args) {
  roi.active = 0;
  gtk_widget_hide(gui.roipanel);
  ToolbarEnable(gui.t3d,3);
  force_redraw_2D();
  app_t2d_changed(0);
}

void roi_label_update() {
  char z[128];
  int ow,oh,od,w,h,d;

  if (!voldata.original) {
    ow=oh=od=w=h=d=1;
  } else {
    ow = voldata.original->W;
    oh = voldata.original->H;
    od = voldata.original->D;

    w = roi.x[1] - roi.x[0] + 1;
    h = roi.y[1] - roi.y[0] + 1;
    d = roi.z[1] - roi.z[0] + 1;
  }
  
  snprintf(z,128,"x=[%d,%d] (%d)",roi.x[0]+1,roi.x[1]+1,w);
  LabelSetText(gui.roil[0],z);
  snprintf(z,128,"y=[%d,%d] (%d)",roi.y[0]+1,roi.y[1]+1,h);
  LabelSetText(gui.roil[1],z);
  snprintf(z,128,"z=[%d,%d] (%d)",roi.z[0]+1,roi.z[1]+1,d);
  LabelSetText(gui.roil[2],z);
  snprintf(z,128,"Vol.: %d %%",(100*w*h*d)/(ow*oh*od));
  LabelSetText(gui.roil[3],z);
}
