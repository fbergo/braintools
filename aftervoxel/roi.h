
#ifndef ROI_H
#define ROI_H 1

typedef struct _roidata {
  int active;
  int x[2],y[2],z[2];  
} ROIdata;

void roi_tool();
void roi_clip(void *args);
void roi_cancel(void *args);
void roi_edit(int x,int y,int z,int pane);
void roi_compute(void *args);

void roi_label_update();

#endif
