
#define SOURCE_CONTOUR_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libip.h>
#include "aftervoxel.h"

int psize = 0;
int *xval = 0;
int *yval = 0;

int cnt_orient;
int cnt_frame;
int cnt_count;

struct _contour_undo {
  Volume *plabel;
  int W,H,D;

  int orient, frame;

  i8_t * data;
  int dw,dh;
} cundo = { .data = 0 };

void contour_save_undo(Volume *label);

void contour_reset() {
  if (psize > 0) {
    MemFree(xval);
    MemFree(yval);
    psize = 0;
    xval = 0;
    yval = 0;
  }
  cnt_count  = 0;
  cnt_frame  = 0;
  cnt_orient = 0;
}

void contour_locate(int frame,int orientation) {

  if (orientation != cnt_orient || cnt_frame != frame)
    contour_reset();

  cnt_orient = orientation;
  cnt_frame  = frame;
}

void contour_add_point(int x,int y) {
  if (psize <= cnt_count) {
    psize += 64;
    if (xval) 
      xval = (int *) MemRealloc(xval, psize * sizeof(int));
    else
      xval = (int *) MemAlloc(psize * sizeof(int));
    if (yval) 
      yval = (int *) MemRealloc(yval, psize * sizeof(int));
    else
      yval = (int *) MemAlloc(psize * sizeof(int));

    if (xval==0 || yval==0) {
      error_memory();
      return;
    }
  }

  if (psize > 0 && xval[cnt_count-1]==x && yval[cnt_count-1]==y)
    return;

  xval[cnt_count] = x;
  yval[cnt_count] = y;
  cnt_count++;
}

void contour_close(Volume *label, char value) {
  int maxx,maxy,w,h;
  int i,j;
  CImg *placebo;
  int *Q,p,x,y,W,WH;
  int qget,qput;

  if (cnt_count < 1) return;
  contour_save_undo(label);

  maxx = xval[0];
  maxy = yval[0];
  for(i=0;i<cnt_count;i++) {
    maxx = MAX(maxx,xval[i]);
    maxy = MAX(maxy,yval[i]);
  }

  placebo = CImgNew(w=maxx+5,h=maxy+5);
  CImgFill(placebo, 0);

  for(i=0;i<cnt_count;i++)
    CImgDrawLine(placebo, xval[i], yval[i],
		 xval[(i+1)%cnt_count],yval[(i+1)%cnt_count],2);

  // flood fill image
  Q = MemAlloc(sizeof(int) * w * h * 8);
  if (!Q) {
    error_memory();
    goto bailout;
  }
  qget = qput = 0;
  Q[qput++] = (w * h) - 1;

  while(qget!=qput) {
    p = Q[qget++];
    x = p%w;
    y = p/w;
    if (CImgGet(placebo,x,y) == 0) {
      CImgSet(placebo,x,y,1);
      if (x-1 >= 0)   Q[qput++] = (x-1) + y*w;
      if (x+1 <= w-1) Q[qput++] = (x+1) + y*w;
      if (y-1 >= 0)   Q[qput++] = x + (y-1)*w;
      if (y+1 <= h-1) Q[qput++] = x + (y+1)*w;
    }
  }
  MemFree(Q);

  W = label->W;
  WH = W * label->H;

  switch(cnt_orient) {
  case 0: //XY view
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	if (CImgGet(placebo,i,j)!=1) {
	  // VolumeSetI8(label, i,j,cnt_frame, value);
	  label->u.i8[ i + j*W + cnt_frame*WH] = value;
	}
      }
    break;
  case 1: //ZY view (x means z, y means y)
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	if (CImgGet(placebo,i,j)!=1) {
	  // VolumeSetI8(label, cnt_frame ,j, i, value);
	  label->u.i8[ cnt_frame + j*W + i*WH] = value;
	}
      }
    break;
    
  case 2: //XZ view (x means x, y means z)
    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	if (CImgGet(placebo,i,j)!=1) {
	  // VolumeSetI8(label, i,cnt_frame,j value);
	  label->u.i8[ i + cnt_frame*W + j*WH] = value;
	}
      }
    break;
  }

 bailout:
  CImgDestroy(placebo);
  contour_reset();
}

int  contour_get_count() {
  return(cnt_count);
}

int  contour_get_frame() {
  return(cnt_frame);
}

int  contour_get_orientation() {
  return(cnt_orient);
}

void contour_get_point(int i,int *x,int *y) {
  if (i < 0) i = 0;
  if (i >= cnt_count)
    i = cnt_count - 1;
  *x = xval[i];
  *y = yval[i];
}

void contour_save_undo(Volume *label) {
  int i,j;

  if (cundo.data != 0) {
    MemFree(cundo.data);
    cundo.data = 0;
  }

  cundo.plabel = label;
  cundo.W = label->W;
  cundo.H = label->H;
  cundo.D = label->D;

  cundo.orient = cnt_orient;
  cundo.frame  = cnt_frame;

  switch(cnt_orient) {
  case 0:
    cundo.data = (i8_t *) MemAlloc(cundo.W * cundo.H * sizeof(i8_t));
    if (!cundo.data) { error_memory(); break; }
    for(i=0;i<cundo.H;i++)
      for(j=0;j<cundo.W;j++)
	cundo.data[j + i*cundo.W] = label->u.i8[j+i*cundo.W+cundo.frame*cundo.W*cundo.H];
    break;
  case 1:
    cundo.data = (i8_t *) MemAlloc(cundo.D * cundo.H * sizeof(i8_t));
    if (!cundo.data) { error_memory(); break; }
    for(i=0;i<cundo.H;i++)
      for(j=0;j<cundo.D;j++)
	cundo.data[j + i*cundo.D] = label->u.i8[cundo.frame+i*cundo.W+j*cundo.W*cundo.H];
    break;
  case 2:
    cundo.data = (i8_t *) MemAlloc(cundo.W * cundo.D * sizeof(i8_t));
    if (!cundo.data) { error_memory(); break; }
    for(i=0;i<cundo.D;i++)
      for(j=0;j<cundo.W;j++)
	cundo.data[j + i*cundo.W] = label->u.i8[j+cundo.frame*cundo.W+i*cundo.W*cundo.H];
    break;
  default:
    cundo.data = 0;
  } 
}

void contour_undo(Volume *label) {
  int i,j;

  if (cundo.plabel != label) return;
  if (cundo.W != label->W) return;
  if (cundo.H != label->H) return;
  if (cundo.D != label->D) return;
  if (cundo.data == 0) return;

  switch(cundo.orient) {
  case 0:
    for(i=0;i<cundo.H;i++)
      for(j=0;j<cundo.W;j++)
	 label->u.i8[j+i*cundo.W+cundo.frame*cundo.W*cundo.H] = cundo.data[j + i*cundo.W];
    break;
  case 1:
    for(i=0;i<cundo.H;i++)
      for(j=0;j<cundo.D;j++)
	label->u.i8[cundo.frame+i*cundo.W+j*cundo.W*cundo.H] = cundo.data[j + i*cundo.D];
    break;
  case 2:
    for(i=0;i<cundo.D;i++)
      for(j=0;j<cundo.W;j++)
	label->u.i8[j+cundo.frame*cundo.W+i*cundo.W*cundo.H] = cundo.data[j + i*cundo.W];
    break;
  } 

  MemFree(cundo.data);
  cundo.data = 0;
  cundo.plabel = 0;
}
