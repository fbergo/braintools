
#ifndef VIEW2D_H
#define VIEW2D_H 1

#include <libip.h>
#include "app.h"
#include "util.h"

typedef struct _view2d {
  int vmax;
  CImg  *base[3], *scaled[3];
  int   cx,cy,cz;
  int   panx[3], pany[3];
  float zoom;
  int   invalid;
  int   mpane, mx, my;

  int tpan[2];

  int   pmap;
  int  *ptable;

  int    npal;
  char **palname;
  int  **paldata;
  int    cpal;
  int   *pal;

  int overlay[2];
  char info[3][256];

} View2D;

void v2d_init();
void v2d_reset();
void v2d_ensure_label();
void v2d_compose();
void v2d_left(int i);
void v2d_right(int i);
void v2d_v2s(int x,int y,int z,int i, int *ox, int *oy);
void v2d_s2v(int x,int y,int i,int *ox, int *oy, int *oz);
void v2d_trace(int x,int y,int z,int i,int update,int label,int sphere);
void v2d_flood(int x,int y,int z,int pane,int update,int label);
void v2d_purge(int x,int y,int z,int pane,int update);

void v2d_pen_cursor(GdkPixmap *dest, GdkGC *gc);

void v2d_roi_overlay();

#endif
