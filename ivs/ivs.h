
#ifndef IVS_H
#define IVS_H 1

#define M_ROT     0x0001
#define M_CLIP    0x0002
#define M_LIGHT   0x0004
#define M_COLOR   0x0008
#define M_ADD     0x0010
#define M_DEL     0x0020

#define M_SOLID   0x0040
#define M_MODE    0x0080
#define M_REPAINT 0x1000
#define M_VIEWS   0x2000
#define M_VIEW2D  0x4000

#define M_ALL     0xffff

#define IVS_CANVAS_FONT "Sans 10"

#include <gtk/gtk.h>
#include <libip.h>
#include "util.h"
#include "iconfig.h"

#define NTOOLS 8
struct SegPane {
  int tool;
  GtkWidget *tools[NTOOLS], *tcontrol[NTOOLS];
  GtkStyle  *active_tool, *inactive_tool;

  GtkWidget *itself;
  int        toid;

  struct _overlay {
    GtkWidget *mipenable;
    GtkWidget *miplighten;
    GtkWidget *mipcurve;
    DropBox   *mipsource;
  } overlay;

  struct _view2d {
    DropBox *mode;
    GtkWidget *label;
  } view2d;

  struct _rotate {
    GtkAdjustment *xa, *ya, *za;
    GtkWidget     *xr, *yr, *zr, *preview;
  } rot;

  struct _clip {
    GtkAdjustment *xa[2], *ya[2], *za[2];
    GtkWidget     *xr[2], *yr[2], *zr[2];
  } clip;

  struct _light {
    GtkAdjustment *ka[3], *za[2], *na;
    GtkWidget     *kr[3], *zr[2], *nr;
    GtkWidget     *preview;
    Scene         *ps;
  } light;

  struct _add {
    Picker *label;
    int    lx, ly, ln;
    GtkWidget *count;
  } add;

  struct _del {
    int    ln;
    GtkWidget *count;
  } del;

  Picker  *solid;
  DropBox *views, *mode, *zoom, *src, *shading; 
  ViewMapCtl *vrange;
  VSlide     *tonalization;

  struct _seg {
    GtkWidget *widget;
    GtkWidget *go;
    DropBox   *method;   
  } seg;

  struct _zoominfo {
    int x,y;
    int active;
    int s[3],svalid;
  } zinfo;

};

struct SegCanvas {
  GtkWidget *itself;
  GdkPixmap *vista;
  GdkGC     *gc;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  int        W,H;
  int        SW,SH;
  int        fourview;
  int        wiremode;
  Cuboid     fullcube, clipcube;
  Point      cc[3][12]; // 0..3: track endpoints, 
                        // 4..6: 1st mark, 7..9: 2nd mark
                        // 10: 1st mark ruler 11: 2nd mark ruler

  int        closer;
  int        t[2];
  float      fullnorm, xysin;
  Vector     vfrom;

  int        keysel;
  int        tvoxel; /* voxel whose path we show */

  CImg *     flat[3];
  int        fvx,fvy,fvz;
  int        fvr[3], fvtx[3], fvty[3];
  int        fox[3], foy[3];
  int        fvasd[4]; // (a)dd (s)eed (d)rag
  int        fvdirty;
  int        fvpan[3];
  CImg *     optvout;

  int        odelta[2]; /* canvas origin displacement, for zoom < 100% */

};

void ivs_set_status(char *text, int timeout);
void ivs_load_new(char *path);
void ivs_load_convert_minc(char *path);
void ivs_go_to_pane(int i);

void ivs_warn_no_volume();

// inter-thread GUI services
void ivs_queue_popup(char *title, char *text, int icon);
void ivs_queue_status(char *text, int timeout);

void ivs_rot(GtkAdjustment *adj, gpointer data);
void ivs_tog(GtkToggleButton *tog, gpointer data);

/* 

   IVS now (1.11) can be compiled in 3 different modes, chosen
   by the configure command.

   web , unicamp and unrestricted.

   web mode will have only watershed and fuzzy segmentations,
   and a narrow selection of preprocessing filters.

   unicamp mode will have only stable algorithms.

   unrestricted mode will have everything. The compilation
   mode can be seen in the Help -> About dialog

   To compile in a different mode:

   make clean
   ./configure MODE
   make

   where MODE can be web, unicamp or empty (for unrestricted), like

   make clean
   ./configure web
   make

*/

// ===========================================
//
// WEB DEFINITIONS
//
// ===========================================
#if defined(AUDIENCE) && AUDIENCE == 2
// building for web distribution:
// disable weird segmentation methods
// and shading modes

#define IVS_TAG "web"

#define SEG_OPERATORS ";DIFT/watershed;DIFT/fuzzy"

#define SEG_OP_WATERSHED           0
#define SEG_OP_FUZZY               1

#define SEG_OP_STRICT_FUZZY        98
#define SEG_OP_IFT_PLUS_WATERSHED  99
#define SEG_OP_DMT                 97
#define SEG_OP_LAZYSHED_L1         96
#define SEG_OP_DISCSHED_1          95
#define SEG_OP_DISCSHED_2          94

#define SHADING_OPTIONS ";auto;none;depth;surface;cartoon"

#define SH_OP_AUTO     0
#define SH_OP_NONE     1
#define SH_OP_DEPTH    2
#define SH_OP_SURFACE  3
#define SH_OP_CARTOON  4

#define SH_OP_GRAD     97
#define SH_OP_N2       98
#define SH_OP_N3       99

#define PP_OPERATORS ";Linear stretch;Gaussian stretch;Gaussian blur;Median;Gradients;Isometric Interpolation;Region Clip"

#define PP_OP_LS     0
#define PP_OP_GS     1
#define PP_OP_BLUR   2
#define PP_OP_MEDIAN 3
#define PP_OP_GRAD   4
#define PP_OP_ISOIN  5
#define PP_OP_CLIP   6
#undef SSR_BUTTON

#define PP_OP_MBGS   10
#define PP_OP_SSR    11
#define PP_OP_AFF    12
#define PP_OP_MODAL  13
#define PP_OP_PL     14

#define PP_LAST      6

#define VIEW_MODES ";solid box;objects;object borders;tree borders"

#define VM_SBOX   0
#define VM_OBJ    1
#define VM_OBOR   2
#define VM_TBOR   3

#define VM_TRANS  90
#define VM_PATHS  91

#define TERMS_OF_USE lic_web

// ===========================================
//
// UNICAMP DEFINITIONS
//
// ===========================================
#elif defined(AUDIENCE) && AUDIENCE == 1
// building for internal unicamp usage:
// just avoid too experimental methods

#define IVS_TAG "unicamp"

#define SEG_OPERATORS ";DIFT/watershed;DIFT/fuzzy;DIFT/strict fuzzy"

#define SEG_OP_WATERSHED           0
#define SEG_OP_FUZZY               1
#define SEG_OP_STRICT_FUZZY        2

#define SEG_OP_IFT_PLUS_WATERSHED  99
#define SEG_OP_DMT                 98
#define SEG_OP_LAZYSHED_L1         96
#define SEG_OP_DISCSHED_1          95
#define SEG_OP_DISCSHED_2          94

#define SHADING_OPTIONS ";auto;none;depth;surface;surface(grad);cartoon"

#define SH_OP_AUTO     0
#define SH_OP_NONE     1
#define SH_OP_DEPTH    2
#define SH_OP_SURFACE  3
#define SH_OP_GRAD     4
#define SH_OP_CARTOON  5

#define SH_OP_N2       98
#define SH_OP_N3       99

#define PP_OPERATORS ";Linear stretch;Gaussian stretch;Multi-Band Gaussian stretch;Gaussian blur;Modal;Median;Gradients;Isometric Interpolation;Brain Gradient: SSR;Affinity;Region Clip;Phantom Lesion"

#define PP_OP_LS     0
#define PP_OP_GS     1
#define PP_OP_MBGS   2
#define PP_OP_BLUR   3
#define PP_OP_MODAL  4
#define PP_OP_MEDIAN 5
#define PP_OP_GRAD   6
#define PP_OP_ISOIN  7
#define PP_OP_SSR    8
#define PP_OP_AFF    9
#define PP_OP_CLIP   10
#define PP_OP_PL     11
#define SSR_BUTTON 1

#define PP_LAST      11

#define VIEW_MODES ";solid box;objects;object borders;tree borders;translucid objects;pathlines"

#define VM_SBOX   0
#define VM_OBJ    1
#define VM_OBOR   2
#define VM_TBOR   3
#define VM_TRANS  4
#define VM_PATHS  5

#define TERMS_OF_USE lic_unicamp

// ===========================================
//
// UNRESTRICTED DEFINITIONS
//
// ===========================================
#else
// building for development, enable everything
// and the kitchen sink

#define IVS_TAG "unrestricted"

#define SEG_OPERATORS ";DIFT/watershed;IFT+/watershed (unreliable);DIFT/fuzzy;DIFT/strict fuzzy;DIFT/DMT;DIFT/lazyshed L1;DIFT/discshed(4-2-1);IFT/Kappaconn"

#define SEG_OP_WATERSHED           0
#define SEG_OP_IFT_PLUS_WATERSHED  1
#define SEG_OP_FUZZY               2
#define SEG_OP_STRICT_FUZZY        3
#define SEG_OP_DMT                 4
#define SEG_OP_LAZYSHED_L1         5
#define SEG_OP_DISCSHED_1          6
#define SEG_OP_DISCSHED_2          7

#define SHADING_OPTIONS ";auto;none;depth;surface;surface(grad);surface(n2);surface(n3);cartoon"

#define SH_OP_AUTO     0
#define SH_OP_NONE     1
#define SH_OP_DEPTH    2
#define SH_OP_SURFACE  3
#define SH_OP_GRAD     4
#define SH_OP_N2       5
#define SH_OP_N3       6
#define SH_OP_CARTOON  7

#define PP_OPERATORS ";Linear stretch;Gaussian stretch;Multi-Band Gaussian stretch;Gaussian blur;Modal;Median;Gradients;Isometric Interpolation;Brain Gradient: SSR;Affinity;Region Clip;Phantom Lesion"

#define PP_OP_LS     0
#define PP_OP_GS     1
#define PP_OP_MBGS   2
#define PP_OP_BLUR   3
#define PP_OP_MODAL  4
#define PP_OP_MEDIAN 5
#define PP_OP_GRAD   6
#define PP_OP_ISOIN  7
#define PP_OP_SSR    8
#define PP_OP_AFF    9
#define PP_OP_CLIP   10
#define PP_OP_PL     11
#define SSR_BUTTON 1

#define PP_LAST      11

#define VIEW_MODES ";solid box;objects;object borders;tree borders;translucid objects;pathlines"

#define VM_SBOX   0
#define VM_OBJ    1
#define VM_OBOR   2
#define VM_TBOR   3
#define VM_TRANS  4
#define VM_PATHS  5

#define TERMS_OF_USE lic_unicamp

#endif

#endif
