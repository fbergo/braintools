
#ifndef PREPROC_H
#define PREPROC_H 1

#include <gtk/gtk.h>
#include <libip.h>
#include "util.h"
#include "ivs.h"

/*

     AB
     C

     A = WxH B = DxH
     C = WxD

*/

typedef struct _preprocfilter {

  int id;
  GtkWidget *control;
  
  void (*activate)(struct _preprocfilter *);
  void (*deactivate)(struct _preprocfilter *);
  void (*repaint)(struct _preprocfilter *);
  void (*update)(struct _preprocfilter *);
  void (*updatepreview)(struct _preprocfilter *);
  int  (*whatever)(struct _preprocfilter *,int,int,int,int,int);

  void *data;

  int  *xslice, *yslice, *zslice;

} PreProcFilter;

/* callbacks on above struct:

  activate:   called when the filter is selected on the side bar.

  deactivate: called when another filter is selected on the 
              side bar.
            
  repaint:    this is what the filter should can when it wants the
              3 views to be invalidated and repainted.

  updatepreview: called when the view position changes (thus the
                 filter may need to reapply a preview operation 
                 and call repaint on its own)

  update:      called when the input image was changed

  whatever:    filter-dependent operation

*/


PreProcFilter * preproc_filter_new();

struct PreProc {
  GtkWidget *canvas;
  GtkWidget *pane;

  PangoLayout *pl;
  PangoFontDescription *pfd;

  GdkPixmap *vista, *wire;
  GdkGC     *gc, *rgc;

  int        nfilters;
  PreProcFilter **ppf;
  GtkWidget  *ppdialog;
  int        lastfilter;

  int        slice[3];
  int        invalid;
  int        wiredrag;
  int        ppspin;

  float      zoom;        /* zoom factor */
  int        panx, pany;  /* screen (0,0) on panned image */

  int        rW,rH,vW,vH; /* real and virtual W/H */
  CImg      *cut[3][3];
  int        boxsz[3][2];
  int        boxszz[3][2];
  int        boxlu[3][2];
  int        rot[3]; /* 0, 90, 180, 270 */

  struct _rotc {
    int        x[3][2];
    int        y[3][2];
  } rotc;

  DropBox    *colormaps;
  DropBox    *zoomw;
  DropBox    *ovlw;

  int        cmap;
  int        chcolor[6];

  struct _bounds {
    int omin, omax;
    int vmin, vmax;
  } bounds;

  struct _pdrag {
    int px, py;
  } pandrag;

  int show_original;
  int show_overlay;

  /* preview service */
  int gotpreview;
  Volume *preview;

  int hasundo;
  Volume *undo;
  int undo_vmax;

  struct _pr {
    int s[3];
    float zoom;
  } prev;

  struct _viewrange {
    ViewMap *vmap_o;
    ViewMap *vmap_c;
    ViewMap *vmap_x;
    ViewMap *active;
    int      ly, uy;
  } viewrange;

};

void create_preproc_gui(GtkWidget *parent);
void open_histogram(GtkWidget *parent);
void preproc_update_ppdialog();

void cb_default_deactivate(PreProcFilter *pp);

/* filters */

PreProcFilter * filter_ls_create(int id); // preproc_ls.c
PreProcFilter * filter_gs_create(int id); // preproc_gs.c
PreProcFilter * filter_mbgs_create(int id); // preproc_mbgs.c
PreProcFilter * filter_thres_create(int id); // preproc_thres.c
PreProcFilter * filter_gblur_create(int id); // preproc_gblur.c
PreProcFilter * filter_modal_create(int id); // preproc_modal.c
PreProcFilter * filter_median_create(int id); // preproc_median.c
PreProcFilter * filter_mgrad_create(int id); // preproc_mgrad.c
PreProcFilter * filter_dgrad_create(int id); // preproc_dgrad.c
PreProcFilter * filter_sgrad_create(int id); // preproc_sgrad.c
PreProcFilter * filter_interp_create(int id); // preproc_interp.c
PreProcFilter * filter_crop_create(int id); // preproc_crop.c
PreProcFilter * filter_plesion_create(int id); // preproc_plesion.c
PreProcFilter * filter_ssr_create(int id); // preproc_ssr.c
PreProcFilter * filter_reg_create(int id); // preproc_reg.c
PreProcFilter * filter_homog_create(int id); // preproc_homog.c
PreProcFilter * filter_morph_create(int id); // preproc_morph.c
PreProcFilter * filter_arith_create(int id); // preproc_arith.c

#define PP_CROP_ID      32
#define PP_PLESION_ID   41
#define PP_REG_ID       43
#endif
