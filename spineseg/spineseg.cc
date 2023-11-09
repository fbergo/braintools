/*

    SpineSeg
    http://www.lni.hc.unicamp.br/app/spineseg
    https://github.com/fbergo/braintools
    Copyright (C) 2009-2017 Felipe P.G. Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#pragma GCC diagnostic ignored "-Wparentheses"

#include "ellipsefit.h"
#include "geometry.h"
#include "ift.h"
#include "image.h"
#include "mutex.h"
#include "tokenizer.h"
#include "volume.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "spineseg_icon.xpm"

#include "auto0.xpm"
#include "auto1.xpm"
#include "dist0.xpm"
#include "dist1.xpm"
#include "manual0.xpm"
#include "manual1.xpm"
#include "normal0.xpm"
#include "normal1.xpm"
#include "pan0.xpm"
#include "pan1.xpm"
#include "zoom0.xpm"
#include "zoom1.xpm"

#define VERSION "2.1"

#define TOOL_ZOOM 0
#define TOOL_PAN 1
#define TOOL_RESLICE 2
#define TOOL_AUTOSEG 3
#define TOOL_MANUALSEG 4
#define TOOL_RULER 5

#define gc_color(a, b) gdk_rgb_gc_set_foreground(a, b)
#define redraw(a) gdk_window_invalidate_rect(a->window, NULL, TRUE);
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))

Volume<int> *vol = NULL;   // original (interpolated volume)
Volume<int> *rsd = NULL;   // resliced volume
Volume<char> *seg = NULL;  // segmentation labels
Volume<char> *seed = NULL; // seeds
spineseg::EllipseFit efit[1024];
int mline[1024][4];

int rsdirty = 0;
int withspline = 0;
int rspx[1024], rspy[1024], rsn = 0;
int rzpz[1024], rzpy[1024], rzn = 0;
double splinex[1024], splinez[1024];

double GradRadius = 1.0;

char volfile[1024];
int vy = 0, vz = 0, vx = 0;
int vmax = 1, nmax = 1;
float zoom = 1.0, uzoom = 1.0, ezoom = 1.0, axzoom = 1.0, paxzoom = 1.0,
      pipzoom = 1.0;
float opacity = 0.50;
int axpx = 0, axpy = 0;
int showfit = 1;
int showpip = 1;
int sagcor = 0;
int pan[2] = {0, 0};
int tool = 3, zs[4] = {0, 0, 0, 0}, ps[3] = {0, 0, 0};
int debug = 0;
int stackcompose = 0;
int zmode = 0; // axial zoom mode 0=1x 1=2x 2=4x
double axialres = 0.50, defres = 0.50;
Volume<int>::Orientation orient = Volume<int>::sagittal_lr;

struct _pane {
  int x, y, w, h;
} panes[7]; // left, right, window, opacity, fit, pipcontrol, pipwindow

GtkWidget *mw, *canvas, *sbar, *menubar;
GdkGC *gc = NULL;
PangoLayout *pl = NULL;
PangoFontDescription *sans10 = NULL, *sans10b = NULL;
int bgcolor = 0x606066;

GdkPixmap *toolsp[12];
GdkBitmap *toolsm[12];

class ResliceTask {
public:
  ResliceTask() { first = last = 0; }
  ResliceTask(int f, int l) {
    first = f;
    last = l;
  }
  ResliceTask &operator=(const ResliceTask &a) {
    first = a.first;
    last = a.last;
    return (*this);
  }
  int first, last;
};

Mutex rsq_mutex;
queue<ResliceTask> rsq;
volatile int rsqactive = 0, rsqtotal, rsqdone;
gint rsqtoid = -1;

Mutex vio_mutex;
volatile int vio_active = 0, vio_loading = 0;
char vio_status[512], vio_name[512];
int *vio_total = NULL, *vio_partial = NULL;
gint vio_toid = -1;

void gui();
gboolean ss_del(GtkWidget *w, GdkEvent *e, gpointer data);
void ss_destroy(GtkWidget *w, gpointer data);
gboolean ss_expose(GtkWidget *w, GdkEventExpose *e, gpointer data);
gboolean ss_press(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean ss_release(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean ss_drag(GtkWidget *w, GdkEventMotion *e, gpointer data);
gboolean ss_key(GtkWidget *w, GdkEventKey *e, gpointer data);

int inbox(int x, int y, int x0, int y0, int w, int h);
void draw_button(int x, int y, int w, int h, const char *label, int c1, int c2,
                 int c3);
void draw_checkbox(int x, int y, int w, int h, const char *text, int checked);
void left_label(int x, int y, int w, int h, const char *label, int color,
                int shadow);
void center_label(int x, int y, int w, int h, const char *label, int color,
                  int shadow);
int lut(int value);
double mark_distance(int y);

void status(const char *msg);
void set_icon(GtkWidget *widget, char **xpm);

int get_filename(GtkWidget *parent, const char *title, char *dest);
void scn_basename(char *dest, const char *src);
void scn_namecat(char *dest, const char *src, const char *cat,
                 const char *prefix);
void scn_namecat_ext(char *dest, const char *src, const char *cat,
                     const char *prefix, const char *ext);

void ss_title();
void ss_load(GtkWidget *w, gpointer data);
void ss_load_special(GtkWidget *w, gpointer data);
void ss_save(GtkWidget *w, gpointer data);
void *ss_save_bg(void *arg);
void *ss_load_bg(void *arg);
void ss_unsave(GtkWidget *w, gpointer data);
void ss_clear(GtkWidget *w, gpointer data);
void ss_clear_slice(GtkWidget *w, gpointer data);
void ss_blur(GtkWidget *w, gpointer data);
void ss_median(GtkWidget *w, gpointer data);
void ss_about(GtkWidget *w, gpointer data);
void ss_quit(GtkWidget *w, gpointer data);
void ss_segment2D(int x, int y, int z, int incremental);
void ss_manualseg(int x, int y, int z, int add);
void ss_tool(int t);

void reslice_reset();
void reslice_compute_all();
void reslice_compute_interval(int y0, int y1);
void _reslice_compute_interval(int y0, int y1);
void _reslice_compute_single(int y);
void reslice_spline();
void reslice_spline_x();
void reslice_spline_z();
void reslice_set_sag_node(int x, int y);
void reslice_del_sag_node(int y);
void reslice_set_cor_node(int x, int y);
void reslice_del_cor_node(int y);
void reslice_get_dxdy(int y, double *dx, double *dy);
void reslice_get_x(int y, double *x);
void reslice_get_dzdy(int y, double *dz, double *dy);
void reslice_get_z(int y, double *z);
void compute_stack_compose();
gboolean rsq_timer(gpointer data);
gboolean vio_timer(gpointer data);

void *reslice_thread(void *data);

void debug_reslice_sag(Image *dst, float dstzoom);
void debug_reslice_cor(Image *dst, float dstzoom);

// from mygtk.h
typedef struct _dropbox {
  GtkWidget *widget;
  int value;
  GtkWidget *o[32];
  int no;
  void (*callback)(void *);
} DropBox;

DropBox *dropbox_new(char *s);
void dropbox_destroy(DropBox *d);
int dropbox_get_selection(DropBox *d);
void dropbox_set_selection(DropBox *d, int i);
void dropbox_set_changed_callback(DropBox *d, void (*callback)(void *));

static GtkItemFactoryEntry ss_menu[] = {
    {"/_File", NULL, NULL, 0, "<Branch>"},
    {"/File/_Load Volume...", NULL, GTK_SIGNAL_FUNC(ss_load), 0, NULL},
    {"/File/Loa_d Special...", NULL, GTK_SIGNAL_FUNC(ss_load_special), 0, NULL},
    {"/File/sep", NULL, NULL, 0, "<Separator>"},
    {"/File/_Save State", NULL, GTK_SIGNAL_FUNC(ss_save), 0, NULL},
    {"/File/_Clear Saved State", NULL, GTK_SIGNAL_FUNC(ss_unsave), 0, NULL},
    {"/File/sep", NULL, NULL, 0, "<Separator>"},
    {"/File/_About SpineSeg...", NULL, GTK_SIGNAL_FUNC(ss_about), 0, NULL},
    {"/File/_Quit", NULL, GTK_SIGNAL_FUNC(ss_quit), 0, NULL},
    {"/_Processing", NULL, NULL, 0, "<Branch>"},
    {"/Processing/_Gaussian Blur", NULL, GTK_SIGNAL_FUNC(ss_blur), 0, NULL},
    {"/Processing/_Median Filter", NULL, GTK_SIGNAL_FUNC(ss_median), 0, NULL},
    {"/Processing/sep", NULL, NULL, 0, "<Separator>"},
    {"/Processing/_Clear Volume", NULL, GTK_SIGNAL_FUNC(ss_clear), 0, NULL},
    {"/Processing/Clear _Slice", NULL, GTK_SIGNAL_FUNC(ss_clear_slice), 0,
     NULL},
};

void ss_title() {
  char msg[1024];
  if (vol != NULL)
    snprintf(msg, 1023, "SpineSeg - %s", volfile);
  else
    strcpy(msg, "SpineSeg");
  gtk_window_set_title(GTK_WINDOW(mw), msg);
}

void status(const char *msg) {
  gtk_statusbar_push(GTK_STATUSBAR(sbar), 0, msg);
  redraw(sbar)
}

void ss_blur(GtkWidget *w, gpointer data) {
  Volume<int> *tmp;
  int i, j, k, p, a, b;
  int m[9];

  if (vol == NULL)
    return;

  tmp = new Volume<int>(vol);

  for (i = 0; i < vol->H; i++) {
    for (j = 1; j < vol->W - 1; j++)
      for (k = 1; k < vol->D - 1; k++) {
        p = 0;
        for (a = -1; a <= 1; a++)
          for (b = -1; b <= 1; b++)
            m[p++] = tmp->voxel(j + a, i, k + b);
        vol->voxel(j, i, k) = (m[0] + 2 * m[1] + m[2] + 2 * m[3] + 4 * m[4] +
                               2 * m[5] + m[6] + 2 * m[7] + m[8]) /
                              16;
      }
  }
  delete tmp;

  reslice_compute_all();

  status("Gaussian blur finished.");
  redraw(canvas);
}

void ss_median(GtkWidget *w, gpointer data) {
  Volume<int> *tmp;
  int i, j, k, p, a, b, c, cp;
  int m[9];

  if (vol == NULL)
    return;
  tmp = new Volume<int>(vol);

  for (i = 0; i < vol->H; i++) {
    for (j = 1; j < vol->W - 1; j++)
      for (k = 1; k < vol->D - 1; k++) {
        p = 0;
        for (a = -1; a <= 1; a++)
          for (b = -1; b <= 1; b++)
            m[p++] = tmp->voxel(j + a, i, k + b);

        // selection sort
        for (a = 0; a < 8; a++) {
          c = m[a];
          cp = a;
          for (b = a + 1; b < 9; b++)
            if (m[b] < c) {
              c = m[b];
              cp = b;
            }
          m[cp] = m[a];
          m[a] = c;
        }
        vol->voxel(j, i, k) = m[4];
      }
  }
  delete tmp;

  reslice_compute_all();

  status("Median filter finished.");
  redraw(canvas);
}

void scn_namecat(char *dest, const char *src, const char *cat,
                 const char *prefix) {
  scn_namecat_ext(dest, src, cat, prefix, "scn");
}

void scn_namecat_ext(char *dest, const char *src, const char *cat,
                     const char *prefix, const char *ext) {
  char tmp[1024], *p;
  scn_basename(tmp, src);
  snprintf(dest, 1023, "%s_%s.%s", tmp, cat, ext);
  p = strrchr(dest, '/');
  if (p == NULL) {
    snprintf(tmp, 1023, "%s_%s", prefix, dest);
    strcpy(dest, tmp);
  } else {
    snprintf(tmp, 1023, "%s_%s", prefix, p + 1);
    strcpy(p + 1, tmp);
  }
}

void scn_basename(char *dest, const char *src) {
  if (!strcasecmp(&src[strlen(src) - 4], ".scn")) {
    strcpy(dest, src);
    dest[strlen(dest) - 4] = 0;
  } else {
    strcpy(dest, src);
  }
}

void ss_manualseg(int x, int y, int z, int add) {
  int i, j;
  Volume<char> *tmp;

  if (rsd == NULL)
    return;
  if (!rsd->valid(x, y, z))
    return;

  seg->voxel(x, y, z) = add ? 1 : 0;

  tmp = new Volume<char>(rsd->W, 1, rsd->D);

  for (i = 0; i < rsd->W; i++)
    for (j = 0; j < rsd->D; j++)
      tmp->voxel(i, 0, j) = (seg->voxel(i, y, j) != 0) ? 1 : 0;

  tmp->incrementBorder(1.0f);

  for (i = 0; i < rsd->W; i++)
    for (j = 0; j < rsd->D; j++)
      seg->voxel(i, y, j) = tmp->voxel(i, 0, j);

  delete tmp;

  if (y >= 0 && y < 1024) {
    efit[y].clear();
    for (i = 0; i < rsd->W; i++)
      for (j = 0; j < rsd->D; j++)
        if (seg->voxel(i, y, j) == 2)
          efit[y].append((double)j, (double)i);
    efit[y].fit();
  }

  redraw(canvas);
}

void ss_segment2D(int x, int y, int z, int incremental) {

  int i, j;
  Volume<int> *slice, *grad;
  Volume<char> *seeds, *label;

  slice = new Volume<int>(rsd->W, 1, rsd->D);
  seeds = new Volume<char>(rsd->W, 1, rsd->D);

  for (i = 0; i < rsd->W; i++)
    for (j = 0; j < rsd->D; j++)
      slice->voxel(i, 0, j) = rsd->voxel(i, y, j);

  if (!incremental)
    for (i = 0; i < rsd->W; i++)
      for (j = 0; j < rsd->D; j++)
        seed->voxel(i, y, j) = 0;
  seed->voxel(x, y, z) = 1;

  grad = slice->morphGradient3D(GradRadius);

  // copy seeds from 3D to 2D
  for (i = 0; i < rsd->W; i++)
    for (j = 0; j < rsd->D; j++)
      seeds->voxel(i, 0, j) = seed->voxel(i, y, j);

  label = grad->treePruning(seeds);
  label->incrementBorder(1.0);

  for (i = 0; i < rsd->W; i++)
    for (j = 0; j < rsd->D; j++)
      seg->voxel(i, y, j) = label->voxel(i, 0, j);

  if (y >= 0 && y < 1024) {
    efit[y].clear();
    for (i = 0; i < rsd->W; i++)
      for (j = 0; j < rsd->D; j++)
        if (label->voxel(i, 0, j) == 2)
          efit[y].append((double)j, (double)i);
    efit[y].fit();
  }

  if (debug) {
    slice->writeSCN("debug-slice.scn", 16, true);
    grad->writeSCN("debug-grad.scn", 16, true);
  }

  delete slice;
  delete seeds;
  delete grad;
  delete label;
}

void ss_clear_slice(GtkWidget *w, gpointer data) {
  int i, j;
  if (vol != NULL) {
    for (i = 0; i < rsd->W; i++)
      for (j = 0; j < rsd->D; j++) {
        seg->voxel(i, vy, j) = 0;
        seed->voxel(i, vy, j) = 0;
      }
    if (vy >= 0 && vy < 1024) {
      efit[vy].clear();
      mline[vy][0] = -1;
      mline[vy][2] = -1;
    }
    redraw(canvas);
  }
}

void ss_clear(GtkWidget *w, gpointer data) {
  int i;
  if (vol != NULL) {
    seg->fill(0);
    seed->fill(0);
    for (i = 0; i < 1024; i++) {
      efit[i].clear();
      mline[i][0] = -1;
      mline[i][2] = -1;
    }
    redraw(canvas);
  }
}

void ss_tool(int t) {
  tool = t;

  switch (tool) {
  case TOOL_ZOOM:
    status("Zoom tool: LEFT DRAG to change zoom, RIGHT CLICK to zoom to fit");
    break;
  case TOOL_PAN:
    status("Pan tool: LEFT DRAG to pan view, RIGHT CLICK to reset pan");
    break;
  case TOOL_RESLICE:
    status(
        "Reslice tool: LEFT CLICK sagittal/coronal to set spine node, RIGHT "
        "CLICK sagittal/coronal to delete node, LEFT CLICK axial to navigate");
    break;
  case TOOL_AUTOSEG:
    status("Autoseg tool: LEFT CLICK to navigate, RIGHT CLICK axial to segment "
           "slice, CTRL+RIGHT CLICK to add multiple seeds. MID CLICK axial for "
           "zoom.");
    break;
  case TOOL_MANUALSEG:
    status("Manualseg tool: LEFT CLICK sagittal/coronal to navigate, LEFT "
           "CLICK axial to add points, RIGHT CLICK axial to remove points. MID "
           "CLICK axial for zoom.");
    break;
  case TOOL_RULER:
    status("Ruler tool: LEFT CLICK sagittal/coronal to navigate, LEFT CLICK "
           "axial to set points. MID CLICK axial for zoom.");
    break;
  }
  redraw(canvas);
}

void ss_quit(GtkWidget *w, gpointer data) { gtk_main_quit(); }

void *ss_save_bg(void *arg) {
  char tmp[1024], base[512], *bp;
  int i, j, k, nv, nv2, xa, xb;
  FILE *f;

  vio_active = 1;

  // printf("bg running\n");

  if (vol == NULL)
    goto over;

  vio_mutex.lock();
  snprintf(vio_status, 511, "Saving volume 1 of 4 (first interpolation)");
  vio_total = vol->getSizeTotalPtr();
  vio_partial = vol->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  scn_namecat(tmp, volfile, "vol", "zzz");
  if (vol->writeBzSCN(tmp, 16, true, 9) != 0) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Saving volume 2 of 4 (second interpolation)");
  vio_total = rsd->getSizeTotalPtr();
  vio_partial = rsd->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  scn_namecat(tmp, volfile, "rsd", "zzz");
  if (rsd->writeBzSCN(tmp, 16, true, 9) != 0) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Saving volume 3 of 4 (automatic segmentation)");
  vio_total = seg->getSizeTotalPtr();
  vio_partial = seg->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  scn_namecat(tmp, volfile, "seg", "zzz");
  if (seg->writeBzSCN(tmp, 8, true, 9) != 0) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Saving volume 4 of 4 (automatic segmentation)");
  vio_total = seed->getSizeTotalPtr();
  vio_partial = seed->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  scn_namecat(tmp, volfile, "see", "zzz");
  if (seed->writeBzSCN(tmp, 8, true, 9) != 0) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Saving textual parameters");
  vio_total = &xb;
  vio_partial = &xa;
  xa = 0;
  xb = 4;
  vio_mutex.unlock();

  scn_namecat_ext(tmp, volfile, "cfg", "zzz", "txt");
  f = fopen(tmp, "w");
  if (f == NULL) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }
  fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n#spineseg state\n", tool,
          vy, vz, (int)(100.0 * uzoom), pan[0], pan[1], nmax,
          (int)(100 * opacity), showfit, showpip, vx, sagcor, stackcompose);
  fprintf(f, "#tool vy vz zoom panx pany nmax opacity showfit showpip vx "
             "sagcor stackcompose\n");
  fclose(f);

  xa = 1;

  scn_namecat_ext(tmp, volfile, "rsv", "zzz", "txt");
  f = fopen(tmp, "w");
  if (f == NULL) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }
  for (i = 0; i < rsn; i++)
    fprintf(f, "x %d %d\n", rspx[i], rspy[i]);
  for (i = 0; i < rzn; i++)
    fprintf(f, "z %d %d\n", rzpz[i], rzpy[i]);
  if (withspline)
    fprintf(f, "spline\n");
  fclose(f);

  xa = 2;

  scn_namecat_ext(tmp, volfile, "lin", "zzz", "txt");
  f = fopen(tmp, "w");
  if (f == NULL) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }
  for (i = 0; i < 1024; i++)
    if (mline[i][0] >= 0) {
      // printf("i=%d mline i 0 = %d\n",i,mline[i][0]);
      fprintf(f, "%d %d %d %d %d\n", i, mline[i][0], mline[i][1], mline[i][2],
              mline[i][3]);
    }
  fclose(f);

  xa = 3;

  scn_namecat_ext(tmp, volfile, "fit", "zzz", "txt");
  f = fopen(tmp, "w");
  if (f == NULL) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Save failed, unable to write %s", tmp);
    vio_mutex.unlock();
    goto over;
  }

  fprintf(f, "0file,y,ecc,a/"
             "b,a(mm),b(mm),perim(mm),area(mm2),seg-area-min(mm2),seg-area-max("
             "mm2),line-dist(mm)\n");

  scn_basename(base, volfile);
  bp = strrchr(base, '/');
  if (bp == NULL)
    bp = base;

  for (i = 0; i < vol->H; i++) {

    if (i >= 1024)
      break;

    if (efit[i].fitted) {
      nv = nv2 = 0;
      for (k = 0; k < rsd->D; k++)
        for (j = 0; j < rsd->W; j++) {
          if (seg->voxel(j, i, k) != 0)
            nv++;
          if (seg->voxel(j, i, k) == 1)
            nv2++;
        }

      fprintf(f, "%s,%d,%.4f,%.4f,%.2f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f\n", bp, i,
              efit[i].eccentricity(), efit[i].flatness(),
              efit[i].major_semiaxis() * 2.0 * rsd->dx,
              efit[i].minor_semiaxis() * 2.0 * rsd->dx,
              efit[i].perimeter() * rsd->dx, efit[i].area() * rsd->dx * rsd->dz,
              rsd->dx * rsd->dz * nv2, rsd->dx * rsd->dz * nv,
              mark_distance(i));
    }
  }

  fclose(f);

  xa = 4;
  vio_mutex.lock();
  snprintf(vio_status, 511, "State saved.");
  vio_mutex.unlock();

over:

  // printf("bg done\n");
  vio_active = 0;
  return NULL;
}

void ss_save(GtkWidget *w, gpointer data) {
  pthread_t tid;

  if (vol == NULL) {
    status("No volume loaded, save failed.");
    return;
  }

  if (vio_active) {
    status("Application busy, can't save now.");
    return;
  }

  status("Saving state...");

  gtk_widget_set_sensitive(menubar, FALSE);
  vio_active = -1;
  pthread_create(&tid, NULL, ss_save_bg, NULL);
  vio_toid = gtk_timeout_add(100, vio_timer, NULL);
  while (vio_active != 0) {
    if (gtk_events_pending())
      gtk_main_iteration();
    else
      usleep(50000);
  }
  if (vio_toid >= 0) {
    gtk_timeout_remove(vio_toid);
    vio_toid = -1;
  }
  pthread_join(tid, NULL);
  status(vio_status);
  gtk_widget_set_sensitive(menubar, TRUE);
}

void ss_unsave(GtkWidget *w, gpointer data) {
  char tmp[1024];

  if (vol == NULL) {
    status("No volume loaded, clear state failed.");
    return;
  }

  scn_namecat(tmp, volfile, "vol", "zzz");
  unlink(tmp);
  scn_namecat(tmp, volfile, "rsd", "zzz");
  unlink(tmp);
  scn_namecat(tmp, volfile, "seg", "zzz");
  unlink(tmp);
  scn_namecat(tmp, volfile, "see", "zzz");
  unlink(tmp);
  scn_namecat_ext(tmp, volfile, "cfg", "zzz", "txt");
  unlink(tmp);
  scn_namecat_ext(tmp, volfile, "lin", "zzz", "txt");
  unlink(tmp);
  scn_namecat_ext(tmp, volfile, "fit", "zzz", "txt");
  unlink(tmp);
  scn_namecat_ext(tmp, volfile, "rsv", "zzz", "txt");
  unlink(tmp);

  status("State clear complete.");
}

void *ss_load_bg(void *arg) {
  char aname[512], name[512];
  Volume<int> *tmp;
  FILE *f;
  char s[1024];
  int i, j, y, xa = -1, xb = -1;

  vio_loading = 1;
  vio_active = 1;

  strcpy(name, vio_name);
  scn_namecat(aname, name, "vol", "zzz");

  tmp = new Volume<int>();

  vio_mutex.lock();
  snprintf(vio_status, 511, "Loading main volume");
  vio_total = tmp->getSizeTotalPtr();
  vio_partial = tmp->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  tmp->readSCN(aname);
  if (!tmp->ok()) {
    tmp->readSCN(name);
    if (tmp->ok() && orient != Volume<int>::sagittal_lr) {
      Volume<int> *tmp2;
      tmp2 = tmp->changeOrientation(orient, Volume<int>::sagittal_lr);
      delete tmp;
      tmp = tmp2;
    }
  }
  if (!tmp->ok()) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Failed to read volume from %s", name);
    vio_total = &xa;
    vio_partial = &xb;
    vio_mutex.unlock();
    delete tmp;
    goto over;
  }

  if (vol != NULL) {
    delete vol;
    delete rsd;
    delete seg;
    delete seed;
    vol = NULL;
    seg = NULL;
    seed = NULL;
    rsd = NULL;
  }
  for (i = 0; i < 1024; i++) {
    efit[i].clear();
    mline[i][0] = -1;
    mline[i][2] = -1;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Interpolating volume");
  vio_total = &xa;
  vio_partial = &xb;
  vio_mutex.unlock();

  vol = tmp->isometricInterpolation();
  delete tmp;

  vio_mutex.lock();
  snprintf(vio_status, 511, "Loading saved resliced volume");
  rsd = new Volume<int>();
  vio_total = rsd->getSizeTotalPtr();
  vio_partial = rsd->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  // try to load resliced vol from state
  scn_namecat(aname, name, "rsd", "zzz");
  rsd->readSCN(aname);
  if (!rsd->ok()) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Interpolating axial volume");
    vio_total = &xa;
    vio_partial = &xb;
    vio_mutex.unlock();
    delete rsd;
    rsd = vol->interpolate(axialres, vol->dy, axialres);
    if (stackcompose)
      compute_stack_compose();
  } else {
    axialres = rsd->dx;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Loading automatic segmentation");
  seg = new Volume<char>();
  vio_total = seg->getSizeTotalPtr();
  vio_partial = seg->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  // try to load seg from state
  scn_namecat(aname, name, "seg", "zzz");
  seg->readSCN(aname);
  if (!seg->ok()) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Initializing segmentation");
    vio_total = &xa;
    vio_partial = &xb;
    vio_mutex.unlock();
    delete seg;
    seg = new Volume<char>(rsd->W, rsd->H, rsd->D);
    seg->fill(0);
  }

  // refit all ellipses
  for (y = 0; y < vol->H && y < 1024; y++) {
    efit[y].clear();
    for (i = 0; i < rsd->W; i++)
      for (j = 0; j < rsd->D; j++)
        if (seg->voxel(i, y, j) == 2)
          efit[y].append((double)j, (double)i);
    efit[y].fit();
    mline[y][0] = mline[y][2] = -1;
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Loading manual segmentation");
  seed = new Volume<char>();
  vio_total = seed->getSizeTotalPtr();
  vio_partial = seed->getSizePartialPtr();
  *vio_total = -1;
  vio_mutex.unlock();

  // try to load seed from state
  scn_namecat(aname, name, "see", "zzz");
  seed->readSCN(aname);
  if (!seed->ok()) {
    vio_mutex.lock();
    snprintf(vio_status, 511, "Initializing manual segmentation");
    vio_total = &xa;
    vio_partial = &xb;
    vio_mutex.unlock();
    delete seed;
    seed = new Volume<char>(rsd->W, rsd->H, rsd->D);
    seed->fill(0);
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Initializing state");
  vio_total = &xa;
  vio_partial = &xb;
  vio_mutex.unlock();

  strcpy(volfile, name);
  vmax = vol->maximum();
  nmax = vmax;
  vy = vol->H / 2;
  vz = vol->D / 2;
  vx = vol->W / 2;

  // read reslicing line
  scn_namecat_ext(aname, name, "rsv", "zzz", "txt");
  f = fopen(aname, "r");
  if (f != NULL) {
    int val[2];
    rsn = 0;
    rzn = 0;
    withspline = 0;
    while (fgets(s, 1024, f) != NULL) {
      if (sscanf(s, "z %d %d", &val[0], &val[1]) == 2) {
        rzpz[rzn] = val[0];
        rzpy[rzn] = val[1];
        rzn++;
        continue;
      }
      if (sscanf(s, "x %d %d", &val[0], &val[1]) == 2) {
        rspx[rsn] = val[0];
        rspy[rsn] = val[1];
        rsn++;
        continue;
      }
      // compatibility with older spineseg versions
      if (sscanf(s, "%d %d", &val[0], &val[1]) == 2) {
        rspx[rsn] = val[0];
        rspy[rsn] = val[1];
        rsn++;
        continue;
      }
      if (strncmp(s, "spline", 6) == 0)
        withspline = 1;
    }
    fclose(f);
    reslice_spline();
  } else {
    reslice_reset();
  }

  // load interface state if available
  scn_namecat_ext(aname, name, "cfg", "zzz", "txt");
  f = fopen(aname, "r");
  if (f != NULL) {
    int val[13];
    fgets(s, 1024, f);
    if (sscanf(s, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &val[0], &val[1],
               &val[2], &val[3], &val[4], &val[5], &val[6], &val[7], &val[8],
               &val[9], &val[10], &val[11], &val[12]) == 13) {
      tool = val[0];
      if (tool < TOOL_ZOOM && tool > TOOL_RULER)
        tool = 0;
      vy = val[1];
      if (vy < 0 || vy >= vol->H)
        vy = vol->H / 2;
      vz = val[2];
      if (vz < 0 || vz >= vol->D)
        vz = vol->D / 2;
      uzoom = val[3] / 100.0;
      if (uzoom < 0.10 || uzoom > 16.0)
        uzoom = 1.0;
      pan[0] = val[4];
      pan[1] = val[5];
      nmax = val[6];
      if (nmax > vmax)
        nmax = vmax;
      opacity = val[7] / 100.0;
      if (opacity < 0.0 || opacity > 1.0)
        opacity = 0.50;
      showfit = val[8];
      showpip = val[9];
      vx = val[10];
      sagcor = val[11];
      stackcompose = val[12];
    } else if (sscanf(s, "%d %d %d %d %d %d %d %d %d %d %d %d", &val[0],
                      &val[1], &val[2], &val[3], &val[4], &val[5], &val[6],
                      &val[7], &val[8], &val[9], &val[10], &val[11]) == 12) {
      tool = val[0];
      if (tool < TOOL_ZOOM && tool > TOOL_RULER)
        tool = 0;
      vy = val[1];
      if (vy < 0 || vy >= vol->H)
        vy = vol->H / 2;
      vz = val[2];
      if (vz < 0 || vz >= vol->D)
        vz = vol->D / 2;
      uzoom = val[3] / 100.0;
      if (uzoom < 0.10 || uzoom > 16.0)
        uzoom = 1.0;
      pan[0] = val[4];
      pan[1] = val[5];
      nmax = val[6];
      if (nmax > vmax)
        nmax = vmax;
      opacity = val[7] / 100.0;
      if (opacity < 0.0 || opacity > 1.0)
        opacity = 0.50;
      showfit = val[8];
      showpip = val[9];
      vx = val[10];
      sagcor = val[11];
      stackcompose = 0;
    } else if (sscanf(s, "%d %d %d %d %d %d %d %d %d", &val[0], &val[1],
                      &val[2], &val[3], &val[4], &val[5], &val[6], &val[7],
                      &val[8]) == 9) {
      tool = val[0];
      if (tool < TOOL_ZOOM && tool > TOOL_RULER)
        tool = 0;
      vy = val[1];
      if (vy < 0 || vy >= vol->H)
        vy = vol->H / 2;
      vz = val[2];
      if (vz < 0 || vz >= vol->D)
        vz = vol->D / 2;
      uzoom = val[3] / 100.0;
      if (uzoom < 0.10 || uzoom > 16.0)
        uzoom = 1.0;
      pan[0] = val[4];
      pan[1] = val[5];
      nmax = val[6];
      if (nmax > vmax)
        nmax = vmax;
      opacity = val[7] / 100.0;
      if (opacity < 0.0 || opacity > 1.0)
        opacity = 0.50;
      showfit = val[8];
      stackcompose = 0;
    }
    fclose(f);
  }

  scn_namecat_ext(aname, name, "lin", "zzz", "txt");
  f = fopen(aname, "r");
  if (f != NULL) {
    int val[5];
    while (fgets(s, 1024, f) != NULL) {
      if (sscanf(s, "%d %d %d %d %d", &val[0], &val[1], &val[2], &val[3],
                 &val[4]) == 5) {
        if (val[0] >= 0 && val[0] < 1024) {
          mline[val[0]][0] = val[1];
          mline[val[0]][1] = val[2];
          mline[val[0]][2] = val[3];
          mline[val[0]][3] = val[4];
        }
      }
    }
    fclose(f);
  }

  vio_mutex.lock();
  snprintf(vio_status, 511, "Finished loading %s", name);
  vio_mutex.unlock();

over:
  vio_loading = 0;
  vio_active = 0;
  return NULL;
}

struct _lsd {
  GtkWidget *window, *filename;
  DropBox *orient;
  GtkWidget *res;
  GtkWidget *compose;
} lsd = {NULL, NULL, NULL, NULL};

gboolean ls_del(GtkWidget *w, GdkEvent *e, gpointer data) { return FALSE; }

void ls_destroy(GtkWidget *w, gpointer data) {
  if (lsd.orient != NULL) {
    dropbox_destroy(lsd.orient);
    lsd.orient = NULL;
  }
  gtk_grab_remove(w);
}

void ls_browse(GtkWidget *w, gpointer data) {
  char dest[512];
  if (get_filename(lsd.window, "Select File", dest))
    gtk_entry_set_text(GTK_ENTRY(lsd.filename), dest);
}

void ls_ok(GtkWidget *w, gpointer data) {
  char name[512];
  int o;
  pthread_t tid;

  strcpy(name, gtk_entry_get_text(GTK_ENTRY(lsd.filename)));
  o = dropbox_get_selection(lsd.orient);
  axialres = gtk_spin_button_get_value(GTK_SPIN_BUTTON(lsd.res));

  switch (o) {
  case 0:
    orient = Volume<int>::sagittal_lr;
    break;
  case 1:
    orient = Volume<int>::sagittal_rl;
    break;
  case 2:
    orient = Volume<int>::coronal_ap;
    break;
  case 3:
    orient = Volume<int>::coronal_pa;
    break;
  case 4:
    orient = Volume<int>::axial_fh;
    break;
  case 5:
    orient = Volume<int>::axial_hf;
    break;
  }

  stackcompose =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lsd.compose)) ? 1 : 0;

  gtk_widget_destroy(lsd.window);

  strcpy(vio_name, name);
  status("Loading volume...");

  gtk_widget_set_sensitive(menubar, FALSE);
  vio_active = -1;
  vio_loading = -1;
  pthread_create(&tid, NULL, ss_load_bg, NULL);
  vio_toid = gtk_timeout_add(100, vio_timer, NULL);
  while (vio_active != 0) {
    if (gtk_events_pending())
      gtk_main_iteration();
    else
      usleep(50000);
  }
  if (vio_toid >= 0) {
    gtk_timeout_remove(vio_toid);
    vio_toid = -1;
  }
  pthread_join(tid, NULL);
  status(vio_status);
  gtk_widget_set_sensitive(menubar, TRUE);
  ss_title();
  redraw(mw);
}

void ls_cancel(GtkWidget *w, gpointer data) { gtk_widget_destroy(lsd.window); }

void ss_load_special(GtkWidget *w, gpointer data) {
  GtkWidget *ls, *vb, *hb[3], *hb2, *ok, *cancel, *lb[3], *browse;

  ls = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  lsd.window = ls;
  gtk_window_set_title(GTK_WINDOW(ls), "Load Special");
  gtk_window_set_position(GTK_WINDOW(ls), GTK_WIN_POS_CENTER);
  gtk_window_set_transient_for(GTK_WINDOW(ls), GTK_WINDOW(mw));
  gtk_container_set_border_width(GTK_CONTAINER(ls), 6);

  vb = gtk_vbox_new(FALSE, 4);
  hb[0] = gtk_hbox_new(FALSE, 0);
  lb[0] = gtk_label_new("Filename:");
  lsd.filename = gtk_entry_new();
  browse = gtk_button_new_with_label("Browse...");

  hb[1] = gtk_hbox_new(FALSE, 0);
  lb[1] = gtk_label_new("Orientation:");
  lsd.orient = dropbox_new(
      ",Sagittal (left to right),Sagittal (right to left),Coronal (anterior to "
      "posterior),Coronal (posterior to anterior),Transversal (bottom to "
      "top),Transversal (top to bottom)");

  hb[2] = gtk_hbox_new(FALSE, 0);
  lb[2] = gtk_label_new("Axial Sampling Resolution (mm):");
  lsd.res = gtk_spin_button_new_with_range(0.05, 3.0, 0.01);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(lsd.res), 0.50);

  lsd.compose = gtk_check_button_new_with_label(
      "Compose axial slices by averaging the 3 nearest ones");

  hb2 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb2), 5);
  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);

  gtk_container_add(GTK_CONTAINER(ls), vb);
  gtk_box_pack_start(GTK_BOX(vb), hb[0], FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(hb[0]), lb[0], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hb[0]), lsd.filename, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hb[0]), browse, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vb), hb[1], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb[1]), lb[1], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hb[1]), lsd.orient->widget, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vb), hb[2], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb[2]), lb[2], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hb[2]), lsd.res, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vb), lsd.compose, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vb), hb2, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hb2), ok, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hb2), cancel, FALSE, FALSE, 2);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ls), "delete_event", GTK_SIGNAL_FUNC(ls_del),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(ls), "destroy", GTK_SIGNAL_FUNC(ls_destroy),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(browse), "clicked", GTK_SIGNAL_FUNC(ls_browse),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(ls_ok), NULL);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(ls_cancel),
                     NULL);
  gtk_widget_show_all(ls);
  gtk_grab_add(ls);
}

void ss_load(GtkWidget *w, gpointer data) {
  char name[512];
  pthread_t tid;

  if (get_filename(mw, "Load Volume", name)) {

    axialres = defres;
    orient = Volume<int>::sagittal_lr;
    strcpy(vio_name, name);
    status("Loading volume...");

    gtk_widget_set_sensitive(menubar, FALSE);
    vio_active = -1;
    vio_loading = -1;
    pthread_create(&tid, NULL, ss_load_bg, NULL);
    vio_toid = gtk_timeout_add(100, vio_timer, NULL);
    while (vio_active != 0) {
      if (gtk_events_pending())
        gtk_main_iteration();
      else
        usleep(50000);
    }
    if (vio_toid >= 0) {
      gtk_timeout_remove(vio_toid);
      vio_toid = -1;
    }
    pthread_join(tid, NULL);
    status(vio_status);
    gtk_widget_set_sensitive(menubar, TRUE);
    ss_title();
    redraw(mw);
  }
}

int inbox(int x, int y, int x0, int y0, int w, int h) {
  return (x >= x0 && y >= y0 && x <= (x0 + w) && y <= (y0 + h));
}

void draw_checkbox(int x, int y, int w, int h, const char *text, int checked) {
  gc_color(gc, 0xffffff);
  gdk_draw_rectangle(canvas->window, gc, TRUE, x + 2, y + 2, h - 4, h - 4);
  gc_color(gc, 0);
  gdk_draw_rectangle(canvas->window, gc, FALSE, x + 2, y + 2, h - 4, h - 4);
  if (checked)
    gdk_draw_rectangle(canvas->window, gc, TRUE, x + h / 4, y + h / 4,
                       h / 2 + 1, h / 2 + 1);
  left_label(x + h + 5, y, w - h - 5, h, text, 0xffffff, 1);
}

void left_label(int x, int y, int w, int h, const char *label, int color,
                int shadow) {
  int tw, th;

  pango_layout_set_text(pl, label, -1);
  pango_layout_get_pixel_size(pl, &tw, &th);
  if (shadow) {
    gc_color(gc, 0);
    gdk_draw_layout(canvas->window, gc, x + 1, y + 1 + (h - th) / 2, pl);
  }
  gc_color(gc, color);
  gdk_draw_layout(canvas->window, gc, x, y + (h - th) / 2, pl);
}

void center_label(int x, int y, int w, int h, const char *label, int color,
                  int shadow) {
  int tw, th;
  pango_layout_set_text(pl, label, -1);
  pango_layout_get_pixel_size(pl, &tw, &th);
  if (shadow) {
    gc_color(gc, 0);
    gdk_draw_layout(canvas->window, gc, x + 1 + (w - tw) / 2,
                    y + 1 + (h - th) / 2, pl);
  }
  gc_color(gc, color);
  gdk_draw_layout(canvas->window, gc, x + (w - tw) / 2, y + (h - th) / 2, pl);
}

void draw_button(int x, int y, int w, int h, const char *label, int c1, int c2,
                 int c3) {
  int tw, th;

  gc_color(gc, c1);
  gdk_draw_rectangle(canvas->window, gc, TRUE, x, y, w, h);
  gc_color(gc, c2);
  gdk_draw_rectangle(canvas->window, gc, FALSE, x, y, w, h);
  gc_color(gc, c3);
  pango_layout_set_text(pl, label, -1);
  pango_layout_get_pixel_size(pl, &tw, &th);
  gdk_draw_layout(canvas->window, gc, x + (w - tw) / 2, y + (h - th) / 2, pl);
}

gboolean ss_del(GtkWidget *w, GdkEvent *e, gpointer data) { return FALSE; }

void ss_destroy(GtkWidget *w, gpointer data) { gtk_main_quit(); }

gboolean ss_expose(GtkWidget *w, GdkEventExpose *e, gpointer data) {
  int W, H;
  char msg[1024];
  Image *sag, *axi, *cor, *tmp, *pip, *dst, *ztmp;
  float zw, zh, dstzoom;
  int i, j, evz, evx, ox, oy;
  GdkRectangle axrect;
  Color c, green(0x00ff00), yellow(0xffff00), red(0xff0000), orange(0xff8000),
      pink(0xffc0c0), lgreen(0x80d080), darkgreen(0x003300);
  const char *zms[4] = {"", "[2X]", "[4X]", "[8X]"};

  gdk_window_get_size(GDK_WINDOW(w->window), &W, &H);
  gdk_rgb_gc_set_foreground(gc, bgcolor);
  gdk_draw_rectangle(w->window, gc, TRUE, 0, 0, W, H);

  pango_layout_set_font_description(pl, sans10b);

  if (vol == NULL && vio_loading == 0)
    return TRUE;

  if (vol != NULL && vio_loading == 0) {
    if (!sagcor)
      zw = (W - 30.0) / (vol->W + vol->D);
    else
      zw = (W - 30.0) / (vol->D + vol->D);
    zh = (H - 70.0) / Max(vol->H, vol->W);
    zoom = Min(zw, zh);
    ezoom = zoom * uzoom;
    pipzoom = ezoom / 5.0;
    axzoom = paxzoom = ezoom * (rsd->dx / vol->dx);

    //  printf("%f  %f  %f\n",zw,zh,zoom);

    // sagittal
    sag = new Image(vol->W, vol->H);
    for (j = 0; j < vol->H; j++)
      for (i = 0; i < vol->W; i++) {
        c.gray(lut(vol->voxel(i, j, vz)));
        // if (j==vy) c.mix(green, 0.33);
        // if (seg->voxel(i,j,vz) != 0) c.mix(yellow, opacity);
        // if (seed->voxel(i,j,vz) != 0) c.mix(red, 1.0);
        sag->set(i, j, c);
      }

    // coronal
    cor = new Image(vol->D, vol->H);
    for (j = 0; j < vol->H; j++)
      for (i = 0; i < vol->D; i++) {
        c.gray(lut(vol->voxel(vx, j, i)));
        cor->set(i, j, c);
      }

    if (sagcor) {
      tmp = cor->scale(ezoom);
      pip = sag->scale(pipzoom);
    } else {
      tmp = sag->scale(ezoom);
      pip = cor->scale(pipzoom);
    }

    // sagittal overlays
    // draw reslice guides

    if (!sagcor) {
      dst = tmp;
      dstzoom = ezoom;
    } else {
      dst = pip;
      dstzoom = pipzoom;
    }
    for (i = 0; i < rsn; i++) {

      dst->rect((int)((rspx[i] * dstzoom) - 2), (int)((rspy[i] * dstzoom) - 2),
                5, 5, pink, false);

      if ((rsn < 3 || !withspline) && (i < rsn - 1))
        dst->line((int)(rspx[i] * dstzoom), (int)(rspy[i] * dstzoom),
                  (int)(rspx[i + 1] * dstzoom), (int)(rspy[i + 1] * dstzoom),
                  pink);
    }

    // draw spline

    if (rsn >= 3 && withspline) {
      for (i = 0; i < rspy[rsn - 1]; i++)
        dst->line((int)(splinex[i] * dstzoom), (int)(i * dstzoom),
                  (int)(splinex[i + 1] * dstzoom), (int)((i + 1) * dstzoom),
                  pink);
    }

    // draw reslice plane

    {
      double px, dx, dy, ang;
      double kpx, kdx, kdy, kang;
      R3 va, vb, kva, kvb;
      double s[4];

      reslice_get_dxdy(vy, &dx, &dy);
      reslice_get_x(vy, &px);
      va.set(1.0, 0.0, 0.0);
      vb.set(-dx, dy, 0.0);
      vb.normalize();
      ang = acos(va.inner(vb));
      ang += M_PI / 2.0;
      while (ang > M_PI / 2.0)
        ang -= M_PI;

      s[0] = 0;
      s[1] = vy + px * tan(ang);

      s[2] = vol->W - 1;
      s[3] = (vy + px * tan(ang)) - s[2] * tan(ang);

      dst->line((int)(px * dstzoom) - 10, (int)(vy * dstzoom),
                (int)(px * dstzoom) + 10, (int)(vy * dstzoom), lgreen);

      dst->line((int)(px * dstzoom), (int)(vy * dstzoom) - 10,
                (int)(px * dstzoom), (int)(vy * dstzoom) + 10, lgreen);

      dst->line((int)(s[0] * dstzoom), (int)(s[1] * dstzoom),
                (int)(s[2] * dstzoom), (int)(s[3] * dstzoom), green);

      // mark slices with markings/segmentation (orange segments)
      for (i = 0; i < vol->H; i++)
        if (efit[i].fitted || mline[i][0] >= 0) {
          reslice_get_dxdy(i, &kdx, &kdy);
          reslice_get_x(i, &kpx);
          kva.set(1.0, 0.0, 0.0);
          kvb.set(-kdx, kdy, 0.0);
          kvb.normalize();
          kang = acos(kva.inner(kvb));
          kang += M_PI / 2.0;
          while (kang > M_PI / 2.0)
            kang -= M_PI;

          s[0] = kpx - 20;
          s[1] = i + 20 * tan(kang);

          s[2] = kpx + 20;
          s[3] = i - 20 * tan(kang);

          dst->line((int)(s[0] * dstzoom), (int)(s[1] * dstzoom),
                    (int)(s[2] * dstzoom), (int)(s[3] * dstzoom), orange);
        }
    }

    if (debug)
      debug_reslice_sag(dst, dstzoom);

    // coronal overlays (todo)
    // draw reslice guides

    if (sagcor) {
      dst = tmp;
      dstzoom = ezoom;
    } else {
      dst = pip;
      dstzoom = pipzoom;
    }
    for (i = 0; i < rzn; i++) {

      dst->rect((int)((rzpz[i] * dstzoom) - 2), (int)((rzpy[i] * dstzoom) - 2),
                5, 5, pink, false);

      if ((rzn < 3 || !withspline) && (i < rzn - 1))
        dst->line((int)(rzpz[i] * dstzoom), (int)(rzpy[i] * dstzoom),
                  (int)(rzpz[i + 1] * dstzoom), (int)(rzpy[i + 1] * dstzoom),
                  pink);
    }

    // draw spline
    if (rzn >= 3 && withspline) {
      for (i = 0; i < rzpy[rzn - 1]; i++)
        dst->line((int)(splinez[i] * dstzoom), (int)(i * dstzoom),
                  (int)(splinez[i + 1] * dstzoom), (int)((i + 1) * dstzoom),
                  pink);
    }

    // draw reslice plane
    {
      double pz, dz, dy, ang;
      double kpz, kdz, kdy, kang;
      R3 va, vb, kva, kvb;
      double s[4];

      reslice_get_dzdy(vy, &dz, &dy);
      reslice_get_z(vy, &pz);
      va.set(1.0, 0.0, 0.0);
      vb.set(-dz, dy, 0.0);
      vb.normalize();
      ang = acos(va.inner(vb));
      ang += M_PI / 2.0;
      while (ang > M_PI / 2.0)
        ang -= M_PI;

      s[0] = 0;
      s[1] = vy + pz * tan(ang);

      s[2] = vol->D - 1;
      s[3] = (vy + pz * tan(ang)) - s[2] * tan(ang);

      dst->line((int)(pz * dstzoom) - 10, (int)(vy * dstzoom),
                (int)(pz * dstzoom) + 10, (int)(vy * dstzoom), lgreen);

      dst->line((int)(pz * dstzoom), (int)(vy * dstzoom) - 10,
                (int)(pz * dstzoom), (int)(vy * dstzoom) + 10, lgreen);

      dst->line((int)(s[0] * dstzoom), (int)(s[1] * dstzoom),
                (int)(s[2] * dstzoom), (int)(s[3] * dstzoom), green);

      // mark slices with markings/segmentation (orange segments)
      for (i = 0; i < vol->H; i++)
        if (efit[i].fitted || mline[i][0] >= 0) {
          reslice_get_dzdy(i, &kdz, &kdy);
          reslice_get_z(i, &kpz);
          kva.set(1.0, 0.0, 0.0);
          kvb.set(-kdz, kdy, 0.0);
          kvb.normalize();
          kang = acos(kva.inner(kvb));
          kang += M_PI / 2.0;
          while (kang > M_PI / 2.0)
            kang -= M_PI;

          s[0] = kpz - 20;
          s[1] = i + 20 * tan(kang);

          s[2] = kpz + 20;
          s[3] = i - 20 * tan(kang);

          dst->line((int)(s[0] * dstzoom), (int)(s[1] * dstzoom),
                    (int)(s[2] * dstzoom), (int)(s[3] * dstzoom), orange);
        }
    }

    if (debug)
      debug_reslice_cor(dst, dstzoom);

    // coronal overlays -- end

    panes[0].x = 10 + pan[0];
    panes[0].y = 10 + pan[1];
    panes[0].w = tmp->W;
    panes[0].h = tmp->H;

    gdk_draw_rgb_image(w->window, gc, panes[0].x, panes[0].y, tmp->W, tmp->H,
                       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3 * (tmp->W));

    if (showpip) {
      panes[6].x = panes[0].x + panes[0].w - pip->W;
      panes[6].y = panes[0].y;
      panes[6].w = pip->W;
      panes[6].h = pip->H;
      gdk_draw_rgb_image(w->window, gc, panes[6].x, panes[6].y, pip->W, pip->H,
                         GDK_RGB_DITHER_NORMAL, pip->getBuffer(), 3 * (pip->W));
      gc_color(gc, 0xffff00);
      gdk_draw_rectangle(w->window, gc, FALSE, panes[6].x, panes[6].y,
                         panes[6].w, panes[6].h);
    }

    delete sag;
    delete cor;
    delete pip;
    delete tmp;

    // axial
    axi = new Image(rsd->D, rsd->W);
    evz = (int)(vz * (vol->dz / rsd->dz));
    evx = (int)(vx * (vol->dx / rsd->dx));

    for (j = 0; j < rsd->W; j++)
      for (i = 0; i < rsd->D; i++) {
        c.gray(lut(rsd->voxel(j, vy, i)));
        if (i == evz || j == evx)
          c.mix(green, 0.33);
        if (seg->voxel(j, vy, i) == 1)
          c.mix(yellow, opacity);
        if (seg->voxel(j, vy, i) == 2)
          c.mix(orange, opacity);
        if (seed->voxel(j, vy, i) != 0)
          c.mix(red, 1.0);
        if (rsdirty) {
          c.G /= 4;
          c.B /= 4;
        }
        axi->set(i, j, c);
      }

    switch (zmode) {
    case 0:
      axpx = axpy = 0;
      tmp = axi->scale(axzoom);
      break;
    case 1:
      ox = evz - axi->W / 4;
      oy = evx - axi->H / 4;
      axzoom *= 2.0;
      axpx = -(axzoom * ox);
      axpy = -(axzoom * oy);
      ztmp = new Image(axi->W / 2, axi->H / 2);
      ztmp->fill(darkgreen);
      ztmp->blit(*axi, ox, oy, 0, 0, axi->W / 2, axi->H / 2);
      tmp = ztmp->scale(axzoom);
      delete ztmp;
      break;
    case 2:
      ox = evz - axi->W / 8;
      oy = evx - axi->H / 8;
      axzoom *= 4.0;
      axpx = -(axzoom * ox);
      axpy = -(axzoom * oy);
      ztmp = new Image(axi->W / 4, axi->H / 4);
      ztmp->fill(darkgreen);
      ztmp->blit(*axi, ox, oy, 0, 0, axi->W / 4, axi->H / 4);
      tmp = ztmp->scale(axzoom);
      delete ztmp;
      break;
    case 3:
      ox = evz - axi->W / 16;
      oy = evx - axi->H / 16;
      axzoom *= 8.0;
      axpx = -(axzoom * ox);
      axpy = -(axzoom * oy);
      ztmp = new Image(axi->W / 8, axi->H / 8);
      ztmp->fill(darkgreen);
      ztmp->blit(*axi, ox, oy, 0, 0, axi->W / 8, axi->H / 8);
      tmp = ztmp->scale(axzoom);
      delete ztmp;
      break;
    }

    panes[1].x = panes[0].x + panes[0].w + 10;
    panes[1].y = 10 + pan[1];
    panes[1].w = tmp->W;
    panes[1].h = tmp->H;
    gdk_draw_rgb_image(w->window, gc, panes[1].x, panes[1].y, tmp->W, tmp->H,
                       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3 * (tmp->W));

    axrect.x = panes[1].x;
    axrect.y = panes[1].y;
    axrect.width = panes[1].w;
    axrect.height = panes[1].h;
    gdk_gc_set_clip_rectangle(gc, &axrect);

    int x0, y0, x1, y1;
    if (vy >= 0 && vy < 1024 && showfit) {
      if (efit[vy].fitted) {
        spineseg::array<double> eplot(100, 2);
        efit[vy].plot(eplot);
        gc_color(gc, 0x00ffff);
        for (i = 0; i < 100; i++) {
          x0 = panes[1].x + axpx + (int)(0.5 + eplot.e(i, 0) * axzoom);
          y0 = panes[1].y + axpy + (int)(0.5 + eplot.e(i, 1) * axzoom);
          x1 = panes[1].x + axpx +
               (int)(0.5 + eplot.e((i + 1) % 100, 0) * axzoom);
          y1 = panes[1].y + axpy +
               (int)(0.5 + eplot.e((i + 1) % 100, 1) * axzoom);
          gdk_draw_line(w->window, gc, x0, y0, x1, y1);
        }
      }
    }

    // distance
    if (vy >= 0 && vy < 1024) {
      if (mline[vy][0] >= 0 && mline[vy][2] >= 0) {
        gc_color(gc, 0xff00ff);
        x0 = panes[1].x + axpx + (int)(0.5 + mline[vy][0] * axzoom);
        y0 = panes[1].y + axpy + (int)(0.5 + mline[vy][1] * axzoom);
        x1 = panes[1].x + axpx + (int)(0.5 + mline[vy][2] * axzoom);
        y1 = panes[1].y + axpy + (int)(0.5 + mline[vy][3] * axzoom);
        gdk_draw_line(w->window, gc, x0, y0, x1, y1);
      }
      if (mline[vy][0] >= 0) {
        gc_color(gc, 0xff00ff);
        x0 = panes[1].x + axpx + (int)(0.5 + mline[vy][0] * axzoom);
        y0 = panes[1].y + axpy + (int)(0.5 + mline[vy][1] * axzoom);
        gdk_draw_rectangle(w->window, gc, FALSE, x0 - 2, y0 - 2, 5, 5);
      }
      if (mline[vy][2] >= 0) {
        gc_color(gc, 0xff00ff);
        x0 = panes[1].x + axpx + (int)(0.5 + mline[vy][2] * axzoom);
        y0 = panes[1].y + axpy + (int)(0.5 + mline[vy][3] * axzoom);
        gdk_draw_rectangle(w->window, gc, FALSE, x0 - 2, y0 - 2, 5, 5);
      }
    }

    if (zmode > 0)
      left_label(panes[1].x + 10, panes[1].y, panes[1].w, 20, zms[zmode],
                 0xffff00, 0);

    gdk_gc_set_clip_rectangle(gc, NULL);

    if (rsdirty)
      center_label(panes[1].x, panes[1].y, panes[1].w, panes[1].h,
                   "Invalid View: Reslicing Required", 0xffff00, 1);

    delete axi;
    delete tmp;

    // toolbar
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_ZOOM ? 1 : 0], 0, 0, 5,
                      5 + 34 * 0, 32, 32);
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_PAN ? 3 : 2], 0, 0, 5,
                      5 + 34 * 1, 32, 32);
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_RESLICE ? 5 : 4], 0, 0,
                      5, 5 + 34 * 2, 32, 32);
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_AUTOSEG ? 7 : 6], 0, 0,
                      5, 5 + 34 * 3, 32, 32);
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_MANUALSEG ? 9 : 8], 0,
                      0, 5, 5 + 34 * 4, 32, 32);
    gdk_draw_drawable(w->window, gc, toolsp[tool == TOOL_RULER ? 11 : 10], 0, 0,
                      5, 5 + 34 * 5, 32, 32);

    if (tool == TOOL_RESLICE) {
      gc_color(gc, withspline ? 0xe1f39a : 0xc9c9c9);
      gdk_draw_rectangle(w->window, gc, TRUE, 42, 5 + 34 * 2, 64, 32);
      gc_color(gc, 0);
      gdk_draw_rectangle(w->window, gc, FALSE, 42, 5 + 34 * 2, 64, 32);
      center_label(42, 5 + 34 * 2, 64, 32, withspline ? "Spline" : "Linear", 0,
                   0);

      if (rsdirty) {
        gc_color(gc, 0xc9c9c9);
        gdk_draw_rectangle(w->window, gc, TRUE, 42 + 69, 5 + 34 * 2, 64, 32);
        gc_color(gc, 0);
        gdk_draw_rectangle(w->window, gc, FALSE, 42 + 69, 5 + 34 * 2, 64, 32);
        center_label(42 + 69, 5 + 34 * 2, 64, 32, "Reslice", 0, 0);
      }
    }

    // text
    snprintf(msg, 255,
             "Dimension: %dx%dx%d (%.2fx%.2fx%.2f mm) Axial Sampling: %.2f mm "
             "Compose: %s",
             vol->W, vol->H, vol->D, vol->dx, vol->dy, vol->dz, rsd->dx,
             stackcompose ? "Yes" : "No");
    left_label(5, H - 60, W - 10, 20, msg, 0xffffff, 1);
    snprintf(msg, 255, "Sagittal Z=%d/%d, Axial Y=%d/%d, Coronal X=%d/%d",
             vz + 1, vol->D, vy + 1, vol->H, vx + 1, vol->W);
    left_label(5, H - 40, W - 10, 20, msg, 0xffffff, 1);

    if (vy < 1024) {
      if (efit[vy].fitted) {
        int nv = 0, nv2 = 0;
        for (i = 0; i < rsd->W; i++)
          for (j = 0; j < rsd->D; j++) {
            if (seg->voxel(i, vy, j) != 0)
              nv++;
            if (seg->voxel(i, vy, j) == 1)
              nv2++;
          }

        // printf("nv=%d nv2=%d dx*dz=%.4f\n",nv,nv2,vol->dx*vol->dz);

        snprintf(
            msg, 255,
            "Ellipse properties: ecc=%.4f a/b=%.4f a=%.2f mm b=%.2f mm "
            "perimeter=%.1f mm area=%.1f mm² (seg=%.1f-%.1f mm²) line=%.1f mm",
            efit[vy].eccentricity(), efit[vy].flatness(),
            efit[vy].major_semiaxis() * 2.0 * rsd->dx,
            efit[vy].minor_semiaxis() * 2.0 * rsd->dx,
            efit[vy].perimeter() * rsd->dx, efit[vy].area() * rsd->dx * rsd->dz,
            rsd->dx * rsd->dz * nv2, rsd->dx * rsd->dz * nv, mark_distance(vy));
        left_label(5, H - 20, W - 10, 20, msg, 0xeeffee, 1);
      }
    }

    // viewing window
    panes[2].x = W - 110;
    panes[2].y = H - 80;
    panes[2].w = 100;
    panes[2].h = 20;
    gc_color(gc, 0);
    gdk_draw_rectangle(w->window, gc, TRUE, panes[2].x, panes[2].y, panes[2].w,
                       panes[2].h);

    j = (100 * nmax) / vmax;
    gc_color(gc, 0xc0c080);
    gdk_draw_rectangle(w->window, gc, TRUE, panes[2].x, panes[2].y, j,
                       panes[2].h);

    gc_color(gc, 0xffffff);
    gdk_draw_rectangle(w->window, gc, FALSE, panes[2].x, panes[2].y, panes[2].w,
                       panes[2].h);
    center_label(panes[2].x, panes[2].y, panes[2].w, panes[2].h, "Window",
                 0xffffff, 1);

    // segmentation opacity
    panes[3].x = W - 110;
    panes[3].y = H - 50;
    panes[3].w = 100;
    panes[3].h = 20;
    gc_color(gc, 0);
    gdk_draw_rectangle(w->window, gc, TRUE, panes[3].x, panes[3].y, panes[3].w,
                       panes[3].h);

    j = (int)(100 * opacity);
    gc_color(gc, 0xc0c080);
    gdk_draw_rectangle(w->window, gc, TRUE, panes[3].x, panes[3].y, j,
                       panes[3].h);

    gc_color(gc, 0xffffff);
    gdk_draw_rectangle(w->window, gc, FALSE, panes[3].x, panes[3].y, panes[3].w,
                       panes[3].h);
    center_label(panes[3].x, panes[3].y, panes[3].w, panes[3].h, "Opacity",
                 0xffffff, 1);

    // show fit
    panes[4].x = W - 210;
    panes[4].y = H - 50;
    panes[4].w = 100;
    panes[4].h = 20;
    draw_checkbox(panes[4].x, panes[4].y, panes[4].w, panes[4].h, "Show fit",
                  showfit);

    // show pip
    panes[5].x = W - 310;
    panes[5].y = H - 50;
    panes[5].w = 100;
    panes[5].h = 20;
    draw_checkbox(panes[5].x, panes[5].y, panes[5].w, panes[5].h, "Show PIP",
                  showpip);

    // overlays

    if (tool == TOOL_ZOOM && zs[0]) {
      int zw;

      gc_color(gc, 0);
      gdk_draw_rectangle(w->window, gc, TRUE, zs[1], zs[2] - 4, 200, 10);

      zw = zs[3] - zs[1];
      if (zw < 0)
        zw = 0;
      if (zw > 200)
        zw = 200;
      gc_color(gc, 0xdeff00);
      gdk_draw_rectangle(w->window, gc, TRUE, zs[1], zs[2] - 4, zw, 10);

      gc_color(gc, 0xffffff);
      gdk_draw_rectangle(w->window, gc, FALSE, zs[1], zs[2] - 4, 200, 10);

      snprintf(msg, 511, "Zoom: %d%%", (int)(100.0 * ezoom));
      center_label(zs[1], zs[2] - 20, 200, 20, msg, 0xdeff00, 1);
    }

    if (tool == TOOL_PAN && ps[0]) {
      snprintf(msg, 511, "Pan: (%+d,%+d)", pan[0], pan[1]);
      center_label(0, 0, W, H, msg, 0xdeff00, 1);
    }
  } // vol && !vio_loading

  if (rsqactive) {
    gc_color(gc, 0x202080);
    gdk_draw_rectangle(w->window, gc, TRUE, W / 2 - 200, H / 2 - 20, 400, 40);
    gc_color(gc, 0x000000);
    gdk_draw_rectangle(w->window, gc, FALSE, W / 2 - 200, H / 2 - 20, 400, 40);

    snprintf(msg, 511, "Reslicing: %d of %d", rsqdone, rsqtotal);
    center_label(0, 0, W, H, msg, 0xdeff00, 1);
  }

  if (vio_active > 0) {

    gc_color(gc, 0x202080);
    gdk_draw_rectangle(w->window, gc, TRUE, W / 2 - 300, H / 2 - 40, 600, 80);
    gc_color(gc, 0x000000);
    gdk_draw_rectangle(w->window, gc, FALSE, W / 2 - 300, H / 2 - 40, 600, 80);

    vio_mutex.lock();
    center_label(W / 2 - 300, H / 2 - 40, 600, 40, vio_status, 0xdeff00, 1);
    int p, t;
    p = *vio_partial;
    t = *vio_total;
    vio_mutex.unlock();

    if (t > 0) {
      gc_color(gc, 0xffffff);
      gdk_draw_rectangle(w->window, gc, TRUE, W / 2 - 250, H / 2, 500, 30);

      int fp = (int)(500.0 * ((double)p) / ((double)t));

      gc_color(gc, 0x8888ff);
      gdk_draw_rectangle(w->window, gc, TRUE, W / 2 - 250, H / 2, fp, 30);

      gc_color(gc, 0);
      gdk_draw_rectangle(w->window, gc, FALSE, W / 2 - 250, H / 2, 500, 30);
    }
  }

  return TRUE;
}

int lut(int value) {
  if (value > nmax)
    return 255;
  return ((int)((255.0 * value) / nmax));
}

gboolean ss_press(GtkWidget *w, GdkEventButton *e, gpointer data) {
  int x, y, b, ctrl;
  int px, py, pz;

  if (vio_active)
    return TRUE;
  if (rsqactive)
    return TRUE;

  x = (int)e->x;
  y = (int)e->y;
  b = (int)e->button;
  ctrl = ((e->state & GDK_CONTROL_MASK) != 0);

  if (vol != NULL) {
    // tool selection
    if (b == 1 && inbox(x, y, 5, 5, 32, 32)) {
      ss_tool(0);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, 5, 5 + 34, 32, 32)) {
      ss_tool(1);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, 5, 5 + 68, 32, 32)) {
      ss_tool(2);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, 5, 5 + 102, 32, 32)) {
      ss_tool(3);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, 5, 5 + 136, 32, 32)) {
      ss_tool(4);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, 5, 5 + 170, 32, 32)) {
      ss_tool(5);
      return TRUE;
    }

    // view window, opacity, showfit, showpip
    if (b == 1 && inbox(x, y, panes[2].x, panes[2].y, panes[2].w, panes[2].h)) {
      nmax = (vmax * (x - panes[2].x)) / 100;
      redraw(canvas);
      return TRUE;
    }
    if (b == 3 && inbox(x, y, panes[2].x, panes[2].y, panes[2].w, panes[2].h)) {
      nmax = vmax;
      redraw(canvas);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, panes[3].x, panes[3].y, panes[3].w, panes[3].h)) {
      opacity = (x - panes[3].x) / 100.0;
      redraw(canvas);
      return TRUE;
    }
    if (b == 3 && inbox(x, y, panes[3].x, panes[3].y, panes[3].w, panes[3].h)) {
      opacity = 0.50;
      redraw(canvas);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, panes[4].x, panes[4].y, panes[4].w, panes[4].h)) {
      showfit = !showfit;
      redraw(canvas);
      return TRUE;
    }
    if (b == 1 && inbox(x, y, panes[5].x, panes[5].y, panes[5].w, panes[5].h)) {
      showpip = !showpip;
      redraw(canvas);
      return TRUE;
    }

    // click in pip window
    if (showpip & inbox(x, y, panes[6].x, panes[6].y, panes[6].w, panes[6].h)) {
      sagcor = !sagcor;
      redraw(canvas);
      return TRUE;
    }

    // zoom
    if (tool == TOOL_ZOOM && b == 1) {
      if (!zs[0]) {
        zs[0] = 1;
        zs[1] = x - (int)(200.0 * log(10.0 * uzoom) / log(160.0));
        zs[2] = y;
        zs[3] = x;
      } else {
        uzoom = exp((log(160.0) * (x - zs[1]) / 200.0) - log(10.0));
        if (uzoom < 0.10)
          uzoom = 0.10;
        if (uzoom > 16.0)
          uzoom = 16.0;
        zs[3] = x;
      }
      redraw(canvas);
      return TRUE;
    }
    if (tool == TOOL_ZOOM && b == 3) {
      uzoom = 1.0;
      redraw(canvas);
      return TRUE;
    }
    // pan
    if (tool == TOOL_PAN && b == 1) {
      if (!ps[0]) {
        ps[0] = 1;
        ps[1] = x - pan[0];
        ps[2] = y - pan[1];
      } else {
        pan[0] = x - ps[1];
        pan[1] = y - ps[2];
      }
      redraw(canvas);
      return TRUE;
    }
    if (tool == TOOL_PAN && b == 3) {
      pan[0] = pan[1] = 0;
      redraw(canvas);
      return TRUE;
    }

    // reslice controls
    if (tool == TOOL_RESLICE && b == 1) {

      if (inbox(x, y, 42, 5 + 34 * 2, 64, 32)) {
        withspline = !withspline;
        rsdirty = 1;
        redraw(canvas);
        return TRUE;
      }

      if (rsdirty && inbox(x, y, 42 + 69, 5 + 34 * 2, 64, 32)) {
        reslice_compute_all();
        return TRUE;
      }
    }

    // sagittal/coronal+left
    if (b == 1 && inbox(x, y, panes[0].x, panes[0].y, panes[0].w, panes[0].h)) {

      switch (tool) {
      case TOOL_RESLICE:
        if (!sagcor)
          reslice_set_sag_node((int)((x - panes[0].x) / ezoom),
                               (int)((y - panes[0].y) / ezoom));
        else
          reslice_set_cor_node((int)((x - panes[0].x) / ezoom),
                               (int)((y - panes[0].y) / ezoom));
        redraw(canvas);
        return TRUE;
      default:
        vy = (int)((y - panes[0].y) / ezoom);
        if (vy < 0)
          vy = 0;
        if (vy >= vol->H)
          vy = vol->H - 1;
        redraw(canvas);
        return TRUE;
      }
    }

    if (b == 3 && inbox(x, y, panes[0].x, panes[0].y, panes[0].w, panes[0].h) &&
        tool == TOOL_RESLICE) {
      if (!sagcor)
        reslice_del_sag_node((int)((y - panes[0].y) / ezoom));
      else
        reslice_del_cor_node((int)((y - panes[0].y) / ezoom));
      redraw(canvas);
      return TRUE;
    }

    // axial+left
    if (b == 1 && inbox(x, y, panes[1].x, panes[1].y, panes[1].w, panes[1].h)) {

      switch (tool) {
      case TOOL_ZOOM:
      case TOOL_PAN:
      case TOOL_AUTOSEG:
      case TOOL_RESLICE:
        vz = (int)((x - panes[1].x - axpx) * (rsd->dz / vol->dz) / axzoom);
        if (vz < 0)
          vz = 0;
        if (vz >= vol->D)
          vz = vol->D - 1;
        vx = (int)((y - panes[1].y - axpy) * (rsd->dx / vol->dx) / axzoom);
        if (vx < 0)
          vx = 0;
        if (vx >= vol->W)
          vx = vol->W - 1;
        redraw(canvas);
        break;
      case TOOL_MANUALSEG:
        ss_manualseg((int)((y - panes[1].y - axpy) / axzoom), vy,
                     (int)((x - panes[1].x - axpx) / axzoom), 1);
        redraw(canvas);
        break;
      case TOOL_RULER:
        px = (int)((y - panes[1].y - axpy) / axzoom);
        py = vy;
        pz = (int)((x - panes[1].x - axpx) / axzoom);
        if (py >= 0 && py < 1024) {

          if (mline[py][0] < 0) {
            mline[py][0] = pz;
            mline[py][1] = px;
            mline[py][2] = -1;
          } else {
            if (mline[py][2] >= 0 &&
                hypot(pz - mline[py][0], px - mline[py][1]) <
                    hypot(pz - mline[py][2], px - mline[py][3])) {

              mline[py][0] = pz;
              mline[py][1] = px;

            } else {

              mline[py][2] = pz;
              mline[py][3] = px;
            }
          }

          redraw(canvas);
        }

        break;
      }

      return TRUE;
    }

    // axial+middle
    if (b == 2) {
      zmode = (zmode + 1) % 4;
      redraw(canvas);
      return TRUE;
    }

    // axial+right
    if (b == 3 && inbox(x, y, panes[1].x, panes[1].y, panes[1].w, panes[1].h)) {

      if (tool == TOOL_AUTOSEG) {
        pz = (int)((x - panes[1].x - axpx) / axzoom);
        px = (int)((y - panes[1].y - axpy) / axzoom);
        py = vy;
        ss_segment2D(px, py, pz, ctrl ? 1 : 0);
        redraw(canvas);
      }
      if (tool == TOOL_MANUALSEG) {
        ss_manualseg((int)((y - panes[1].y - axpy) / axzoom), vy,
                     (int)((x - panes[1].x - axpx) / axzoom), 0);
        redraw(canvas);
      }
      return TRUE;
    }
  }

  return TRUE;
}

gboolean ss_release(GtkWidget *w, GdkEventButton *e, gpointer data) {
  if (vio_active)
    return TRUE;

  if (tool == TOOL_ZOOM) {
    zs[0] = 0;
    redraw(canvas);
  }
  if (tool == TOOL_PAN) {
    ps[0] = 0;
    redraw(canvas);
  }

  return TRUE;
}

gboolean ss_drag(GtkWidget *w, GdkEventMotion *e, gpointer data) {
  int x, y, b;
  GdkEventButton eb;

  if (vio_active)
    return TRUE;

  x = (int)e->x;
  y = (int)e->y;
  b = 0;

  if (e->state & GDK_BUTTON3_MASK)
    b = 3;
  if (e->state & GDK_BUTTON2_MASK)
    b = 2;
  if (e->state & GDK_BUTTON1_MASK)
    b = 1;

  if (b != 0) {
    eb.x = x;
    eb.y = y;
    eb.button = b;
    eb.state = e->state;
    ss_press(w, &eb, data);
  }

  return TRUE;
}

gboolean ss_key(GtkWidget *w, GdkEventKey *e, gpointer data) {

  if (vio_active)
    return TRUE;
  if ((e->state & GDK_MOD1_MASK) != 0)
    return FALSE; // ignore Alt+anything

  switch (e->keyval) {
  case GDK_Left:
    if (vol != NULL) {
      if (!sagcor) {
        vz--;
        if (vz < 0)
          vz = 0;
      } else {
        vx--;
        if (vx < 0)
          vx = 0;
      }
      redraw(canvas);
    }
    break;
  case GDK_Right:
    if (vol != NULL) {
      if (!sagcor) {
        vz++;
        if (vz >= vol->D)
          vz = vol->D - 1;
      } else {
        vx++;
        if (vx >= vol->W)
          vx = vol->W + 1;
      }
      redraw(canvas);
    }
    break;
  case GDK_Up:
    if (vol != NULL) {
      vy--;
      if (vy < 0)
        vy = 0;
      redraw(canvas);
    }
    break;
  case GDK_Down:
    if (vol != NULL) {
      vy++;
      if (vy >= vol->H)
        vy = vol->H - 1;
      redraw(canvas);
    }
    break;
  case GDK_Escape:
    ss_clear(NULL, NULL);
    break;
  case GDK_f:
  case GDK_F:
    showfit = !showfit;
    redraw(canvas);
    break;
  case GDK_l:
  case GDK_L:
    ss_load(NULL, NULL);
    break;
  case GDK_S:
  case GDK_s:
    if (opacity < 0.1)
      opacity = 0.50;
    else
      opacity = 0.0;
    redraw(canvas);
    break;
  case GDK_z:
  case GDK_Z:
    zmode = (zmode + 1) % 4;
    redraw(canvas);
    break;
  case GDK_KP_Subtract:
  case GDK_minus:
    if (vol != NULL) {
      nmax = (int)(1.1 * nmax);
      if (nmax > vmax)
        nmax = vmax;
      redraw(canvas);
    }
    break;
  case GDK_KP_Add:
  case GDK_plus:
  case GDK_equal:
    if (vol != NULL) {
      nmax = (int)(0.9 * nmax);
      redraw(canvas);
    }
    break;
  case GDK_9:
    if (vol != NULL) {
      opacity -= 0.05;
      if (opacity < 0.0)
        opacity = 0.0;
      redraw(canvas);
    }
    break;
  case GDK_0:
    if (vol != NULL) {
      opacity += 0.05;
      if (opacity > 1.0)
        opacity = 1.0;
      redraw(canvas);
    }
    break;
  case GDK_F1:
    ss_tool(0);
    break;
  case GDK_F2:
    ss_tool(1);
    break;
  case GDK_F3:
    ss_tool(2);
    break;
  case GDK_F4:
    ss_tool(3);
    break;
  case GDK_F5:
    ss_tool(4);
    break;
  case GDK_F6:
    ss_tool(5);
    break;
  case GDK_Delete:
  case GDK_KP_Delete:
    ss_clear_slice(NULL, NULL);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void gui() {
  GtkWidget *v, *mb;
  GtkItemFactory *gif;
  GtkAccelGroup *mag;
  GtkStyle *style;
  int nitems = sizeof(ss_menu) / sizeof(ss_menu[0]);

  mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(mw), "SpineSeg");
  gtk_window_set_default_size(GTK_WINDOW(mw), 960, 768);
  gtk_widget_set_events(mw, GDK_EXPOSURE_MASK);
  gtk_widget_realize(mw);

  set_icon(mw, spineseg_icon_xpm);

  // load tool pixmaps
  style = gtk_widget_get_style(mw);
  toolsp[0] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)zoom0_xpm);
  toolsp[1] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)zoom1_xpm);
  toolsp[2] = gdk_pixmap_create_from_xpm_d(
      mw->window, &toolsm[0], &style->bg[GTK_STATE_NORMAL], (gchar **)pan0_xpm);
  toolsp[3] = gdk_pixmap_create_from_xpm_d(
      mw->window, &toolsm[0], &style->bg[GTK_STATE_NORMAL], (gchar **)pan1_xpm);
  toolsp[4] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)normal0_xpm);
  toolsp[5] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)normal1_xpm);
  toolsp[6] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)auto0_xpm);
  toolsp[7] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)auto1_xpm);
  toolsp[8] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)manual0_xpm);
  toolsp[9] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                           &style->bg[GTK_STATE_NORMAL],
                                           (gchar **)manual1_xpm);
  toolsp[10] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                            &style->bg[GTK_STATE_NORMAL],
                                            (gchar **)dist0_xpm);
  toolsp[11] = gdk_pixmap_create_from_xpm_d(mw->window, &toolsm[0],
                                            &style->bg[GTK_STATE_NORMAL],
                                            (gchar **)dist1_xpm);

  v = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(mw), v);

  // menu
  mag = gtk_accel_group_new();
  gif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);
  gtk_item_factory_create_items(gif, nitems, ss_menu, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(mw), mag);
  menubar = mb = gtk_item_factory_get_widget(gif, "<main>");
  gtk_box_pack_start(GTK_BOX(v), mb, FALSE, TRUE, 0);

  // canvas
  canvas = gtk_drawing_area_new();
  gtk_widget_set_events(canvas, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
                                    GDK_BUTTON_RELEASE_MASK |
                                    GDK_BUTTON_MOTION_MASK);
  gtk_box_pack_start(GTK_BOX(v), canvas, TRUE, TRUE, 0);

  // status

  sbar = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(v), sbar, FALSE, TRUE, 0);

  if (pl == NULL)
    pl = gtk_widget_create_pango_layout(canvas, NULL);
  if (sans10 == NULL)
    sans10 = pango_font_description_from_string("Sans 10");
  if (sans10b == NULL)
    sans10b = pango_font_description_from_string("Sans Bold 10");
  if (gc == NULL)
    gc = gdk_gc_new(mw->window);

  gtk_signal_connect(GTK_OBJECT(mw), "delete_event", GTK_SIGNAL_FUNC(ss_del),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(mw), "destroy", GTK_SIGNAL_FUNC(ss_destroy),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(canvas), "expose_event",
                     GTK_SIGNAL_FUNC(ss_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(canvas), "button_press_event",
                     GTK_SIGNAL_FUNC(ss_press), NULL);
  gtk_signal_connect(GTK_OBJECT(canvas), "button_release_event",
                     GTK_SIGNAL_FUNC(ss_release), NULL);
  gtk_signal_connect(GTK_OBJECT(canvas), "motion_notify_event",
                     GTK_SIGNAL_FUNC(ss_drag), NULL);
  gtk_signal_connect(GTK_OBJECT(mw), "key_press_event", GTK_SIGNAL_FUNC(ss_key),
                     NULL);

  gtk_widget_show_all(mw);
}

int main(int argc, char **argv) {
  pthread_t tid;
  int i;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-debug")) {
      debug = 1;
      continue;
    }
    if (!strcmp(argv[i], "-c")) {
      stackcompose = 1;
      printf("each axial slice will be composed of the 3 nearest ones by "
             "averaging.\n");
      continue;
    }
    if (!strcmp(argv[i], "-h") || (!strcmp(argv[i], "--help"))) {
      printf("SpineSeg version %s - (c) 2009-2017 Felipe Bergo "
             "<fbergo\x40gmail.com>\n\n",
             VERSION);
      printf("Command line options:\n");
      printf(" -debug     enable debugging actions.\n");
      printf(" -h         show this help message and exit.\n");
      printf(" -r value   override axial interpolation resolution (default "
             "0.50 mm)\n");
      printf(" -c         compose each axial slice from the nearest 3\n");
      printf(" -g value   override morphological gradient radius (default 1.0 "
             "pixels)\n\n");
      exit(0);
    }
    if (!strcmp(argv[i], "-r") && i < argc - 1) {
      axialres = defres = atof(argv[++i]);
      printf("bypassing default axial resolution: %.2f mm\n", axialres);
      continue;
    }
    if (!strcmp(argv[i], "-g") && i < argc - 1) {
      GradRadius = atof(argv[++i]);
      if (GradRadius < 1.0)
        GradRadius = 1.0;
      if (GradRadius > 10.0)
        GradRadius = 10.0;
      printf("bypassing default gradient radius: %.2f\n", GradRadius);
      continue;
    }
  }

  gtk_init(&argc, &argv);
  gdk_rgb_init();

  gui();

  pthread_create(&tid, NULL, reslice_thread, NULL);
  gtk_main();

  return 0;
}

struct _fdlgs {
  int done;
  int ok;
  GtkWidget *dlg;
  char *dest;
} fdlg_data;

void fdlg_destroy(GtkWidget *w, gpointer data) {
  gtk_grab_remove(fdlg_data.dlg);
  fdlg_data.done = 1;
}

void fdlg_ok(GtkWidget *w, gpointer data) {
  strcpy(fdlg_data.dest,
         gtk_file_selection_get_filename(GTK_FILE_SELECTION(fdlg_data.dlg)));

  fdlg_data.ok = 1;
  gtk_widget_destroy(fdlg_data.dlg);
}

int get_filename(GtkWidget *parent, const char *title, char *dest) {
  GtkWidget *fdlg;

  fdlg = gtk_file_selection_new(title);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fdlg)->ok_button), "clicked",
                     GTK_SIGNAL_FUNC(fdlg_ok), 0);
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fdlg)->cancel_button),
                            "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(fdlg));
  gtk_signal_connect(GTK_OBJECT(fdlg), "destroy", GTK_SIGNAL_FUNC(fdlg_destroy),
                     0);

  fdlg_data.done = 0;
  fdlg_data.ok = 0;
  fdlg_data.dlg = fdlg;
  fdlg_data.dest = dest;

  gtk_widget_show(fdlg);
  gtk_grab_add(fdlg);

  int x;
  while (!fdlg_data.done) {
    x = 5;
    while (gtk_events_pending() && x > 0) {
      gtk_main_iteration();
      --x;
    }
    usleep(5000);
  }

  return (fdlg_data.ok ? 1 : 0);
}

void about_destroy(GtkWidget *w, gpointer data) { gtk_grab_remove(w); }

void about_ok(GtkWidget *w, gpointer data) {
  gtk_widget_destroy((GtkWidget *)data);
}

void ss_about(GtkWidget *w, gpointer data) {
  GtkWidget *ad, *vb, *hb, *hb2, *l, *ok, *i;
  char z[1024];
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkStyle *style;

  style = gtk_widget_get_style(mw);
  icon = gdk_pixmap_create_from_xpm_d(mw->window, &mask,
                                      &style->bg[GTK_STATE_NORMAL],
                                      (gchar **)spineseg_icon_xpm);

  ad = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ad), "About SpineSeg");
  gtk_window_set_position(GTK_WINDOW(ad), GTK_WIN_POS_CENTER);
  gtk_window_set_transient_for(GTK_WINDOW(ad), GTK_WINDOW(mw));
  gtk_container_set_border_width(GTK_CONTAINER(ad), 6);
  gtk_widget_realize(ad);

  i = gtk_image_new_from_pixmap(icon, mask);

  vb = gtk_vbox_new(FALSE, 4);
  hb = gtk_hbox_new(FALSE, 0);
  snprintf(
      z, 1024,
      "SpineSeg version %s\n(C) 2009-2017 Felipe "
      "Bergo\n<fbergo\x40gmail.com>\nhttp://www.lni.hc.unicamp.br/app/"
      "spineseg\n\nThis program is free software; you can redistribute\nit "
      "and/or modify it under the terms of the GNU General\nPublic License as "
      "published by the Free Software\nFoundation; either version 2 of the "
      "License, or\n(at your option) any later version.",
      VERSION);

  l = gtk_label_new(z);
  hb2 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb2), 5);
  ok = gtk_button_new_with_label("Close");
  GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);

  gtk_container_add(GTK_CONTAINER(ad), vb);
  gtk_box_pack_start(GTK_BOX(vb), hb, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb), i, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hb), l, FALSE, FALSE, 8);
  gtk_box_pack_start(GTK_BOX(vb), hb2, FALSE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hb2), ok, FALSE, FALSE, 2);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ad), "delete_event", GTK_SIGNAL_FUNC(ss_del),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(ad), "destroy", GTK_SIGNAL_FUNC(about_destroy),
                     NULL);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(about_ok),
                     (gpointer)ad);

  gtk_widget_show_all(ad);
  gtk_grab_add(ad);
}

void set_icon(GtkWidget *widget, char **xpm) {
  GdkPixbuf *pb;
  pb = gdk_pixbuf_new_from_xpm_data((const char **)xpm);
  gtk_window_set_icon(GTK_WINDOW(widget), pb);
}

double mark_distance(int y) {
  if (y < 0 || y >= 1024)
    return 0.0;
  if (vol == NULL)
    return 0.0;
  if (mline[y][0] < 0 || mline[y][2] < 0)
    return 0.0;
  return (rsd->dx *
          hypot(mline[y][0] - mline[y][2], mline[y][1] - mline[y][3]));
}

void reslice_reset() {
  if (vol != NULL) {
    rsn = rzn = 2;
    rspx[0] = rspx[1] = vol->W / 2;
    rspy[0] = 0;
    rspy[1] = vol->H - 1;

    rzpz[0] = rzpz[1] = vol->D / 2;
    rzpy[0] = 0;
    rzpy[1] = vol->H - 1;
  } else {
    rsn = 0;
  }
}

void reslice_compute_all() {
  if (rsn > 0 || rzn > 0)
    reslice_compute_interval(Min(rspy[0], rzpy[0]),
                             Max(rspy[rsn - 1], rzpy[rzn - 1]));
}

gboolean rsq_timer(gpointer data) {
  redraw(canvas);
  if (rsqactive == 0) {
    rsqtoid = -1;
    return FALSE;
  } else
    return TRUE;
}

gboolean vio_timer(gpointer data) {
  redraw(canvas);
  if (vio_active == 0) {
    vio_toid = -1;
    return FALSE;
  }
  return TRUE;
}

void reslice_compute_interval(int y0, int y1) {
  ResliceTask rt(y0, y1);
  rsq_mutex.lock();
  rsq.push(rt);
  rsq_mutex.unlock();

  if (rsqtoid < 0)
    rsqtoid = gtk_timeout_add(100, rsq_timer, NULL);
}

void _reslice_compute_interval(int y0, int y1) {
  int i;
  for (i = y0; i <= y1; i++) {
    _reslice_compute_single(i);
    rsqdone++;
  }
}

void debug_reslice_sag(Image *dst, float dstzoom) {
  R3 oldsky, newsky, raxis;
  R3 p, q, c, f;
  double dx, dy1, dz, dy2, bx, by, bz, ang;
  T4 t0, t1;
  int i, j, k;
  int ix, iy, iz;
  Color cc;

  by = (double)vy;
  reslice_get_x(vy, &bx);
  reslice_get_z(vy, &bz);
  reslice_get_dxdy(vy, &dx, &dy1);
  reslice_get_dzdy(vy, &dz, &dy2);

  oldsky.set(0.0, -1.0, 0.0);
  newsky.set(-dx / dy1, -1.0, -dz / dy2);
  newsky.normalize();

  printf("(vx,vy,vz) = (%d,%d,%d), (bx,by,bz) = (%.1f,%.1f,%.1f)\n", vx, vy, vz,
         bx, by, bz);
  printf("dx=%.1f, dy=%.1f dx/dy=%.1f | dz=%.1f dy=%.1f dz/dy=%.1f\n", dx, dy1,
         dx / dy1, dz, dy2, dz / dy2);
  raxis = newsky.cross(oldsky);
  raxis.normalize();
  ang = oldsky.angle(newsky);

  oldsky.print("oldsky");
  newsky.print("newsky");
  raxis.print("raxis");
  printf("ang = %.2f deg\n", ang * 180.0 / M_PI);

  t0.scale(rsd->dx / vol->dx, rsd->dy / vol->dy, rsd->dz / vol->dz);
  t1.axisrot(raxis, -ang * 180.0 / M_PI, bx, by, bz);
  t0 *= t1;

  k = (int)by;
  cc = 0xff00ff;
  for (j = 0; j < rsd->D; j++)
    for (i = 0; i < rsd->W; i++) {

      p.set(i, k, j);
      q = t0.apply(p);

      // near borders use NN
      ix = (int)q.X;
      iy = (int)q.Y;
      iz = (int)q.Z;

      if (vol->valid(ix, iy, iz) && iz == vz)
        dst->set((int)(ix * dstzoom), (int)(iy * dstzoom), cc);
    }
}

void debug_reslice_cor(Image *dst, float dstzoom) {
  R3 oldsky, newsky, raxis;
  R3 p, q, c, f;
  double dx, dy1, dz, dy2, bx, by, bz, ang;
  T4 t0, t1;
  int i, j, k;
  int ix, iy, iz;
  Color cc;

  by = (double)vy;
  reslice_get_x(vy, &bx);
  reslice_get_z(vy, &bz);
  reslice_get_dxdy(vy, &dx, &dy1);
  reslice_get_dzdy(vy, &dz, &dy2);

  oldsky.set(0.0, -1.0, 0.0);
  newsky.set(-dx / dy1, -1.0, -dz / dy2);
  newsky.normalize();

  raxis = newsky.cross(oldsky);
  raxis.normalize();
  ang = oldsky.angle(newsky);

  t0.scale(rsd->dx / vol->dx, rsd->dy / vol->dy, rsd->dz / vol->dz);
  t1.axisrot(raxis, -ang * 180.0 / M_PI, bx, by, bz);
  t0 *= t1;

  k = (int)by;
  cc = 0xff00ff;
  for (j = 0; j < rsd->D; j++)
    for (i = 0; i < rsd->W; i++) {

      p.set(i, k, j);
      q = t0.apply(p);

      // near borders use NN
      ix = (int)q.X;
      iy = (int)q.Y;
      iz = (int)q.Z;

      if (vol->valid(ix, iy, iz) && ix == vx)
        dst->set((int)(iz * dstzoom), (int)(iy * dstzoom), cc);
    }
}

void _reslice_compute_single(int y) {
  R3 oldsky, newsky, raxis;
  R3 p, q, c, f;
  double dx, dy1, dz, dy2, bx, by, bz, ang;
  double cx[8], cy[8], cz[8];
  double w, sw, sv;
  T4 t0, t1;
  int i, j, k, t;
  int ix, iy, iz;

  by = (double)y;
  reslice_get_x(y, &bx);
  reslice_get_z(y, &bz);
  reslice_get_dxdy(y, &dx, &dy1);
  reslice_get_dzdy(y, &dz, &dy2);

  oldsky.set(0.0, -1.0, 0.0);
  newsky.set(-dx / dy1, -1.0, -dz / dy2);
  newsky.normalize();

  raxis = newsky.cross(oldsky);
  raxis.normalize();
  ang = oldsky.angle(newsky);

  t0.scale(rsd->dx / vol->dx, rsd->dy / vol->dy, rsd->dz / vol->dz);
  t1.axisrot(raxis, -ang * 180.0 / M_PI, bx, by, bz);
  t0 *= t1;

  k = (int)by;

  for (j = 0; j < rsd->D; j++)
    for (i = 0; i < rsd->W; i++) {

      p.set(i, k, j);
      q = t0.apply(p);

      f.set(floor(q.X), floor(q.Y), floor(q.Z));
      c.set(ceil(q.X), ceil(q.Y), ceil(q.Z));

      if (vol->valid((int)(f.X), (int)(f.Y), (int)(f.Z)) &&
          vol->valid((int)(c.X), (int)(c.Y), (int)(c.Z))) {

        // interpolate on the general case

        cx[0] = cx[1] = cx[2] = cx[3] = f.X;
        cx[4] = cx[5] = cx[6] = cx[7] = c.X;

        cy[0] = cy[1] = cy[4] = cy[5] = f.Y;
        cy[2] = cy[3] = cy[6] = cy[7] = c.Y;

        cz[0] = cz[2] = cz[4] = cz[6] = f.Z;
        cz[1] = cz[3] = cz[5] = cz[7] = c.Z;

        sw = 0.0;
        sv = 0.0;
        for (t = 0; t < 8; t++) {
          w = 1.0 / sqrt((cx[t] - q.X) * (cx[t] - q.X) +
                         (cy[t] - q.Y) * (cy[t] - q.Y) +
                         (cz[t] - q.Z) * (cz[t] - q.Z));
          if (w > 20.0)
            w = 20.0;
          sw += w;
          sv += w * (vol->voxel((int)(cx[t]), (int)(cy[t]), (int)(cz[t])));
        }
        rsd->voxel(i, k, j) = (int)(sv / sw);

      } else {
        // near borders use NN
        ix = (int)q.X;
        iy = (int)q.Y;
        iz = (int)q.Z;

        if (vol->valid(ix, iy, iz))
          rsd->voxel(i, k, j) = vol->voxel(ix, iy, iz);
        else
          rsd->voxel(i, k, j) = 0;
      }
    }
}

void reslice_get_dzdy(int y, double *dz, double *dy) {
  int i;

  if (rzn < 2) {
    *dz = 0.0;
    *dy = 1.0;
    return;
  }

  if (withspline && rzn >= 3) {
    i = y;
    if (i <= 0)
      i = 1;
    if (i >= vol->H - 1)
      i = vol->H - 1;
    ;
    *dz = splinez[i + 1] - splinez[i - 1];
    *dy = 2.0;
    return;
  }

  for (i = 0; i < rzn; i++)
    if (y <= rzpy[i])
      break;

  if (i == 0)
    i++;
  if (i == rzn)
    i--;

  *dz = (double)(rzpz[i] - rzpz[i - 1]);
  *dy = (double)(rzpy[i] - rzpy[i - 1]);
}

void reslice_get_z(int y, double *z) {
  int i;
  double dz, dy, a;

  if (rzn < 2) {
    *z = vol->D / 2;
    return;
  }

  if (withspline && rzn >= 3 && y >= 0 && y < 1024) {
    *z = splinez[y];
    return;
  }

  for (i = 0; i < rzn; i++)
    if (y <= rzpy[i])
      break;

  if (i == rzn)
    i--;

  if (y == rzpy[i]) {
    *z = (double)rzpz[i];
    return;
  }

  if (i == 0)
    i++;

  dz = (double)(rzpz[i] - rzpz[i - 1]);
  dy = (double)(rzpy[i] - rzpy[i - 1]);

  a = (y - rzpy[i - 1]) / dy;
  *z = rzpz[i - 1] + a * dz;
}

void reslice_get_dxdy(int y, double *dx, double *dy) {
  int i;

  if (rsn < 2) {
    *dx = 0.0;
    *dy = 1.0;
    return;
  }

  if (withspline && rsn >= 3) {
    i = y;
    if (i <= 0)
      i = 1;
    if (i >= vol->H - 1)
      i = vol->H - 1;
    ;
    *dx = splinex[i + 1] - splinex[i - 1];
    *dy = 2.0;
    return;
  }

  for (i = 0; i < rsn; i++)
    if (y <= rspy[i])
      break;

  if (i == 0)
    i++;
  if (i == rsn)
    i--;

  *dx = (double)(rspx[i] - rspx[i - 1]);
  *dy = (double)(rspy[i] - rspy[i - 1]);
}

void reslice_get_x(int y, double *x) {
  int i;
  double dx, dy, a;

  if (rsn < 2) {
    *x = vol->W / 2;
    return;
  }

  if (withspline && rsn >= 3 && y >= 0 && y < 1024) {
    *x = splinex[y];
    return;
  }

  for (i = 0; i < rsn; i++)
    if (y <= rspy[i])
      break;

  if (i == rsn)
    i--;

  if (y == rspy[i]) {
    *x = (double)rspx[i];
    return;
  }

  if (i == 0)
    i++;

  dx = (double)(rspx[i] - rspx[i - 1]);
  dy = (double)(rspy[i] - rspy[i - 1]);

  a = (y - rspy[i - 1]) / dy;
  *x = rspx[i - 1] + a * dx;

  /*
    printf("P1=(%d,%d) P2=(%d,%d) R=(%d,%d) dx=%.2f dy=%.2f\n",
    rspx[i-1],rspy[i-1],
    rspx[i],rspy[i],
    (int)(*x), y, dx, dy);
  */
}

void reslice_del_sag_node(int y) {
  int i, j, md, mdi;

  if (rsn < 3) {
    reslice_reset();
    reslice_compute_all();
    return;
  }

  mdi = 1;
  md = 5000;

  for (i = 1; i < rsn - 1; i++)
    if (abs(y - rspy[i]) < md) {
      md = abs(y - rspy[i]);
      mdi = i;
    }

  for (j = mdi; j < rsn - 1; j++) {
    rspx[j] = rspx[j + 1];
    rspy[j] = rspy[j + 1];
  }
  rsn--;
  reslice_spline();
  rsdirty = 1;
}

void reslice_set_sag_node(int x, int y) {
  int i, j;

  if (vol == NULL)
    return;

  for (i = 0; i < rsn; i++)
    if (y <= rspy[i])
      break;

  if (y == rspy[i]) {

    rspx[i] = x;

  } else {

    for (j = rsn; j > i; j--) {
      rspx[j] = rspx[j - 1];
      rspy[j] = rspy[j - 1];
    }
    rsn++;
    rspx[i] = x;
    rspy[i] = y;
  }

  reslice_spline();
  rsdirty = 1;
}

void reslice_del_cor_node(int y) {
  int i, j, md, mdi;

  if (rzn < 3) {
    reslice_reset();
    reslice_compute_all();
    return;
  }

  mdi = 1;
  md = 5000;

  for (i = 1; i < rzn - 1; i++)
    if (abs(y - rzpy[i]) < md) {
      md = abs(y - rzpy[i]);
      mdi = i;
    }

  for (j = mdi; j < rzn - 1; j++) {
    rzpz[j] = rzpz[j + 1];
    rzpy[j] = rzpy[j + 1];
  }
  rzn--;
  reslice_spline();
  rsdirty = 1;
}

void reslice_set_cor_node(int x, int y) {
  int i, j;

  if (vol == NULL)
    return;

  for (i = 0; i < rzn; i++)
    if (y <= rzpy[i])
      break;

  if (y == rzpy[i]) {

    rzpz[i] = x;

  } else {

    for (j = rzn; j > i; j--) {
      rzpz[j] = rzpz[j - 1];
      rzpy[j] = rzpy[j - 1];
    }
    rzn++;
    rzpz[i] = x;
    rzpy[i] = y;
  }

  reslice_spline();
  rsdirty = 1;
}

void *reslice_thread(void *data) {
  ResliceTask rt;

  for (;;) {

    rsq_mutex.lock();
    if (rsq.empty()) {
      rsq_mutex.unlock();
      usleep(50000);
    } else {

      rsdirty = 0;
      rt = rsq.front();
      rsq.pop();
      rsqactive = 1;
      rsqdone = 0;
      rsqtotal = rt.last - rt.first + 1;
      rsq_mutex.unlock();

      _reslice_compute_interval(rt.first, rt.last);
      if (stackcompose)
        compute_stack_compose();
      rsqactive = 0;
    }
  }

  return NULL;
}

void reslice_spline() {
  reslice_spline_x();
  reslice_spline_z();
}

void reslice_spline_x() {
  double t1[1027];
  int t2[1027];
  int i, j, n;
  double u, u3, u2, s;

  for (i = 0; i < 1024; i++)
    splinex[i] = 0.0;

  n = 1;
  t1[0] = rspx[0];
  t2[0] = rspy[0] - 2;

  for (i = 0; i < rsn; i++) {
    t1[n] = rspx[i];
    t2[n] = rspy[i];
    n++;
  }
  for (i = 0; i < 4; i++) {
    t1[n] = t1[n - 1];
    t2[n] = t2[n - 1] + 2;
    n++;
  }

  for (i = 0; i < rsn; i++)
    splinex[rspy[i]] = rspx[i];

  s = 0.5;
  for (i = 1; i <= n - 3; i++) {

    for (j = t2[i] + 1; j < t2[i + 1]; j++) {

      if (j < 0 || j >= 1024)
        continue;

      u = ((double)(j - t2[i])) / ((double)(t2[i + 1] - t2[i]));
      u2 = u * u;
      u3 = u2 * u;

      splinex[j] = t1[i - 1] * (-s * u3 + 2 * s * u2 - s * u) +
                   t1[i] * ((2 - s) * u3 + (s - 3) * u2 + 1) +
                   t1[i + 1] * ((s - 2) * u3 + (3 - 2 * s) * u2 + s * u) +
                   t1[i + 2] * (s * u3 - s * u2);
    }
  }
}

void reslice_spline_z() {
  double t1[1027];
  int t2[1027];
  int i, j, n;
  double u, u3, u2, s;

  for (i = 0; i < 1024; i++)
    splinez[i] = 0.0;

  n = 1;
  t1[0] = rzpz[0];
  t2[0] = rzpy[0] - 2;

  for (i = 0; i < rzn; i++) {
    t1[n] = rzpz[i];
    t2[n] = rzpy[i];
    n++;
  }
  for (i = 0; i < 4; i++) {
    t1[n] = t1[n - 1];
    t2[n] = t2[n - 1] + 2;
    n++;
  }

  for (i = 0; i < rzn; i++)
    splinez[rzpy[i]] = rzpz[i];

  s = 0.5;
  for (i = 1; i <= n - 3; i++) {

    for (j = t2[i] + 1; j < t2[i + 1]; j++) {

      if (j < 0 || j >= 1024)
        continue;

      u = ((double)(j - t2[i])) / ((double)(t2[i + 1] - t2[i]));
      u2 = u * u;
      u3 = u2 * u;

      splinez[j] = t1[i - 1] * (-s * u3 + 2 * s * u2 - s * u) +
                   t1[i] * ((2 - s) * u3 + (s - 3) * u2 + 1) +
                   t1[i + 1] * ((s - 2) * u3 + (3 - 2 * s) * u2 + s * u) +
                   t1[i + 2] * (s * u3 - s * u2);
    }
  }
}

// mygtk.cc
#define CREATE(t) ((t *)malloc(sizeof(t)))
#define DESTROY(a) free(a)

void dropbox_select(GtkWidget *w, gpointer data);

void dropbox_destroy(DropBox *d) {
  if (d != NULL)
    DESTROY(d);
}

DropBox *dropbox_new(char *s) {
  DropBox *d;
  GtkMenu *m;
  char sep[2], *p, *cp;

  d = CREATE(DropBox);
  if (!d)
    return 0;
  d->no = 0;
  d->value = 0;
  d->callback = 0;

  m = GTK_MENU(gtk_menu_new());

  cp = (char *)malloc(strlen(s) + 1);
  if (!cp) {
    free(d);
    return 0;
  }
  strcpy(cp, s);

  sep[0] = s[0];
  sep[1] = 0;
  p = strtok(&cp[1], sep);
  while (p) {
    d->o[d->no] = gtk_menu_item_new_with_label(p);
    gtk_menu_append(m, d->o[d->no]);
    gtk_signal_connect(GTK_OBJECT(d->o[d->no]), "activate",
                       GTK_SIGNAL_FUNC(dropbox_select), (gpointer)d);
    gtk_widget_show(d->o[d->no]);
    ++(d->no);
    p = strtok(0, sep);
  }

  gtk_widget_show(GTK_WIDGET(m));

  d->widget = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(d->widget), GTK_WIDGET(m));
  gtk_widget_show(d->widget);
  return d;
}

int dropbox_get_selection(DropBox *d) { return (d->value); }

void dropbox_set_selection(DropBox *d, int i) {
  gtk_option_menu_set_history(GTK_OPTION_MENU(d->widget), i);
  d->value = i;
}

void dropbox_set_changed_callback(DropBox *d, void (*callback)(void *)) {
  d->callback = callback;
}

void dropbox_select(GtkWidget *w, gpointer data) {
  DropBox *me = (DropBox *)data;
  int i;

  for (i = 0; i < me->no; i++) {
    if (me->o[i] == w) {
      me->value = i;
      if (me->callback)
        me->callback((void *)me);
      break;
    }
  }
}

void compute_stack_compose() {
  Volume<int> *tmp, *orsd;
  int i, j, k;

  tmp = new Volume<int>(rsd);

  for (j = 1; j < tmp->H - 1; j++)
    for (k = 0; k < tmp->D; k++)
      for (i = 0; i < tmp->W; i++)
        tmp->voxel(i, j, k) = (rsd->voxel(i, j, k) + rsd->voxel(i, j - 1, k) +
                               rsd->voxel(i, j + 1, k)) /
                              3;

  orsd = rsd;
  rsd = tmp;
  delete orsd;
}
