
#ifndef APP_H
#define APP_H 1

#include <libip.h>
#include "util.h"

#define VERSION "2.7"

#define APPNAME    "Aftervoxel"
#define APPBG      0xddccaa
#define APPFG      0xffffff
#define APPCBG     0xcccccc
#define APPCFG     0xffdd90
#define DTITLE     5
#define BFUNC      darker
#define BVAL       5

typedef struct viewstate {
  int W,H;
  int bx[4],by[4],bw[4],bh[4],enable[4];
  int maximized; /* -1 = none, 0...3 maximized window */
  int bg,fg,crosshair,measures;
  int invalid;
  int focus;

  Box bmax[4];
  Box blef[4];
  Box brig[4];

  CImg *buf[4];
  
  int   zopane;
  CImg *zobuf;
  int   zolen,zox,zoy;

  int   bcpane;
  int   bcw,bch;
  float bcpx,bcpy;

  int   pfg, pbg;

} ViewState;

typedef struct resources {
  Glyph *gmax, *gmin, *gleft, *gright;
  SFont *font[6];
  TaskQueue *bgqueue;
  char Home[512];
} Resources;

typedef struct _voldata {
  Volume *original;
  Volume *label;

  Mutex  *masterlock;
} VolData;

typedef struct _notifydata {
  int datachanged;
  int viewchanged;
  int renderstep;
  int segchanged;
  int autoseg;
  int sessionloaded;
  int labelsaver;
} NotifyData;

typedef struct _gui {
  Toolbar *t2d, *t3d;

  GtkWidget *status, *f2d, *f3d;
  GtkWidget *penlabel;
  GtkWidget *kl,*kf;
  Toolbar *tpen;

  Slider *bri,*con,*ton;

  Slider *oskin,*oobj;
  CheckBox *skin,*oct,*obj,*meas;
  Glyph *null;

  // MAXLABELS in labels.h must be kept consistent
  CheckBox *vis_mark[12];
  Label    *vis_name[12];

  GtkWidget *palcanvas;
  GdkCursor *cursor2[9], *cursor3[5];
  int        cstate;

  GtkWidget *octpanel, *octcw;
  GtkWidget *edit_undo;

  GtkWidget *roipanel;
  Label  *roil[4];
  Button *roib[2];

  Label     *ovl[2];
  CheckBox  *overlay_info, *overlay_orient;  
} GUI;

void app_set_status(char *text, int timeout);
void force_redraw_2D();
void force_redraw_3D();

int   *app_calc_lookup();
void   app_queue_octant_redraw();
void   app_queue_obj_redraw();

void   app_pop_message(char *title, char *text, int icon);
void   app_gui();
void   app_pop_init();

void app_update_oct_panel();

void app_t2d_changed(void *t);
void app_t3d_changed(void *t);

void app_set_title(char *text);
void app_set_title_file(char *text);

void app_tdebug(int id);

#endif
