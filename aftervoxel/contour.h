
#ifndef CONTOUR_H
#define CONTOUR_H 1

#include <libip.h>

void contour_reset();
void contour_locate(int frame,int orientation);
void contour_add_point(int x,int y);
void contour_close(Volume *label, char value);

int  contour_get_count();
int  contour_get_frame();
int  contour_get_orientation();
void contour_get_point(int i,int *x,int *y);

void contour_undo(Volume *label); // undoes the last contour_close operation

#endif
