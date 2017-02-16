
#ifndef VIEW3D_H
#define VIEW3D_H 1

#include <gtk/gtk.h>
#include <libip.h>
#include "util.h"

typedef struct _view3d {

  Transform rotation, wirerotation;
  int invalid;
  int wireframe;
  int show_measures;

  float *zbuf;
  int    zlen;

  float panx,pany,savepanx,savepany;
  float zoom;

  struct {
    CImg  *out;
    float *zbuf;
    float *ebuf;
    int    invalid;    
    int    zlen;

    i16_t  value;
    int    smoothing; /* 1-12 */
    int    color;

    int         *tmap;
    int          tcount;
    int          enable;
    float        opacity;

    Mutex *lock;
    volatile int ready;
  } skin;

  struct {
    CImg  *out;
    float *zbuf;
    int    invalid;
    int    zlen;

    int       usedepth;

    int enable;

    Mutex *lock;
    volatile int ready;
  } octant;

  struct {
    CImg  *out;
    float *zbuf;
    int    invalid;
    float  opacity;
    int    enable;
    int    zlen;

    Mutex *lock;
    volatile int ready;
  } obj;

  CImg *base, *scaled;
  CImg *sbase[3],*sscaled[3]; /* extra split views */
  float *szbuf;

  /* light model */
  Vector light;
  float  ka,kd,ks;
  float  zgamma,zstretch;
  int    sre;

  /* other colors */
  int georuler_color;
  int measure_color;

} View3D;

void view3d_init();
void view3d_reset();
void view3d_set_gui();
void view3d_compose(int w,int h,int bg);

void view3d_invalidate(int data,int rotclip);

void view3d_skin_trash(int map, int rendering);
void view3d_octant_trash();
void view3d_objects_trash();

void view3d_overlay_skin_octant();
void view3d_overlay_obj();

#define M_SKIN 0x01
#define M_OCT  0x02
#define M_OBJ  0x04
#define M_ALL  0xff
void view3d_alloc(int what,int dw,int dh);

void view3d_dialog_options();

typedef struct _octctl {
  int    w,h;
  float  maxside;
  CImg  *out;
  float *zbuf;
  float *alpha, *beta, *gamma;
  char  *val;
  int    pending;
  int    ox,oy;
} OctantCube;

void view3d_octctl_init(int w,int h,float maxside);
void view3d_octctl_render();

gboolean view3d_oct_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data);
gboolean view3d_oct_press(GtkWidget *w, GdkEventButton *eb, gpointer data);
gboolean view3d_oct_release(GtkWidget *w, GdkEventButton *eb, gpointer data);
gboolean view3d_oct_drag(GtkWidget *w, GdkEventMotion *em, gpointer data);
void     view3d_oct_action(int x,int y);

void view3d_find_skin(int *x, int *y, int *z);
void view3d_draw_scaled_georulers(CImg *dest, CImg *base, Transform *rot, float zoom);
void view3d_draw_scaled_measures(CImg *dest, Transform *rot, float zoom);

int * skin_rmap(int w,int h, Transform *rot, float panx, float pany,
		int W,int H,int D, int *map, int mapcount);

#endif
