/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2017
   
   Felipe P. G. Bergo 
   fbergo at gmail.com

   and
   
   Alexandre X. Falcao
   afalcao at ic.unicamp.br

   ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "preproc.h"
#include "util.h"
#include "be.h"
#include "ivs.h"
#include "guiparser.h"

extern Scene *scene;
extern TaskQueue *qseg;

extern int sync_ppchange;
extern int sync_newdata;
extern GtkWidget *mainwindow;

struct PreProc preproc;

gboolean
ivs_preproc_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean
ivs_preproc_press(GtkWidget *widget,GdkEventButton *eb,gpointer data);
gboolean
ivs_preproc_release(GtkWidget *widget,GdkEventButton *eb,gpointer data);
gboolean
ivs_preproc_drag(GtkWidget *widget,GdkEventMotion *em,gpointer data);

void ivs_preproc_draw_wire(GtkWidget *widget);

void preproc_open_ppdialog();
gboolean preproc_ppdialog_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void preproc_ppdialog_destroy(GtkWidget *w, gpointer data);
void preproc_filter_change(SMenu *sm, int id);
void cb_default_repaint(PreProcFilter *pp);
Volume * preproc_prepare_preview();

void preproc_refresh_preview();

void preproc_clip(GdkRectangle *r, int what);

void ivs_select_colormap(void *obj);
void ivs_select_zoom(void *obj);
void ivs_select_display(void *obj);
void ivs_apply_revert(GtkWidget *w, gpointer data);
void ivs_apply_swap(GtkWidget *w, gpointer data);

void compose_preproc_vista();

void preproc_viewmap_calc();

int preproc_tool_id();

/* space transforms */

/* screen space to volume space 
   return: 0 = in XY view, 1 = in YZ view, 2 = in XZ view, -1 : no where */
int  ivs_pp_s2v(Point *screen, Point *volume);

/* volume space to screen space */
void ivs_pp_v2s(Point *volume, Point *xy, Point *yz, Point *xz, int clip);

void create_preproc_gui(GtkWidget *parent) {
  int i;
  GtkWidget *tmp[60];
  GtkStyle *black;

  SMenu *fm;

  MemSet(&preproc, 0 , sizeof(preproc));
  MemSet(tmp,0,sizeof(GtkWidget *)*60);

  preproc.chcolor[0] = 0x00ff00;
  preproc.chcolor[1] = 0x0080ff;
  preproc.chcolor[2] = 0xff00ff;
  preproc.chcolor[3] = 0xff00ff;
  preproc.chcolor[4] = 0x0000ff;
  preproc.chcolor[5] = 0xffff00;

  preproc.cmap = 0;

  preproc.bounds.omin = -1;
  preproc.bounds.vmin = -1;
  preproc.ppspin = 0;

  preproc.viewrange.vmap_o = ViewMapNew();
  preproc.viewrange.vmap_c = ViewMapNew();
  preproc.viewrange.vmap_x = ViewMapNew();
  preproc.viewrange.active = preproc.viewrange.vmap_c;

  preproc.zoom = 1.0;
  preproc.panx = 0;
  preproc.pany = 0;
  preproc.wiredrag = 0;

  preproc.ppdialog = 0;
  preproc.nfilters = 18;
  preproc.ppf = (PreProcFilter **) malloc(64 * sizeof(PreProcFilter *));
  preproc.lastfilter = -1;

  preproc.gotpreview = 0;
  preproc.preview = 0;
  
  black = gtk_style_new();
  set_style_bg(black, 0);

  preproc.canvas = gtk_drawing_area_new();
  gtk_widget_set_style(preproc.canvas, black);
  gtk_widget_ensure_style(preproc.canvas);
  gtk_widget_set_events(preproc.canvas, GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|
			GDK_BUTTON2_MOTION_MASK|GDK_BUTTON1_MOTION_MASK);

  gtk_box_pack_start(GTK_BOX(parent), preproc.canvas, TRUE, TRUE, 0);

  preproc.pl =  gtk_widget_create_pango_layout(preproc.canvas, NULL);
  preproc.pfd = pango_font_description_from_string("Sans 10");
  if (preproc.pl != NULL && preproc.pfd != NULL)
    pango_layout_set_font_description(preproc.pl, preproc.pfd);

  preproc.pane=gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(preproc.pane),2);
  gtk_box_pack_start(GTK_BOX(parent), preproc.pane, FALSE, TRUE, 0);

  tmp[0] = gtk_label_new("Preprocessing Filters");
  tmp[26] = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(tmp[26]),256,1);

  gtk_box_pack_start(GTK_BOX(preproc.pane),tmp[26], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(preproc.pane),tmp[0], FALSE, TRUE, 2);

  fm = smenu_new();
  smenu_set_colors(fm, 
		   0xd0c8b0, 0x000000,
		   0xe5b070, 0x000000,
		   0xff8000, 0x000000,
		   0x000000, 0xffffff);
  smenu_set_callback(fm, preproc_filter_change);
  smenu_append_item(fm, smenuitem_new("Local Filters",1,0));
  smenu_append_item(fm, smenuitem_new("Linear Stretch",0,1));
  smenu_append_item(fm, smenuitem_new("Gaussian Stretch",0,2));
  smenu_append_item(fm, smenuitem_new("Multi-Band Gaussian Stretch",0,3));
  smenu_append_item(fm, smenuitem_new("Threshold",0,4));

  preproc.ppf[0] = filter_ls_create(1);
  preproc.ppf[1] = filter_gs_create(2);
  preproc.ppf[2] = filter_mbgs_create(3);
  preproc.ppf[3] = filter_thres_create(4);

  smenu_append_item(fm, smenuitem_new("Smoothing Filters",1,0));
  smenu_append_item(fm, smenuitem_new("Gaussian Blur",0,11));
  smenu_append_item(fm, smenuitem_new("Modal",0,12));
  smenu_append_item(fm, smenuitem_new("Median",0,13));

  preproc.ppf[4] = filter_gblur_create(11);
  preproc.ppf[5] = filter_modal_create(12);
  preproc.ppf[6] = filter_median_create(13);

  smenu_append_item(fm, smenuitem_new("Gradient Filters",1,0));
  smenu_append_item(fm, smenuitem_new("Morphological Gradient",0,21));
  smenu_append_item(fm, smenuitem_new("Directional Gradient",0,22));
  smenu_append_item(fm, smenuitem_new("Sobel Gradient",0,23));

  preproc.ppf[7] = filter_mgrad_create(21);
  preproc.ppf[8] = filter_dgrad_create(22);
  preproc.ppf[9] = filter_sgrad_create(23);

  smenu_append_item(fm, smenuitem_new("Resizing Filters",1,0));
  smenu_append_item(fm, smenuitem_new("Linear Interpolation",0,31));
  smenu_append_item(fm, smenuitem_new("Region Clip",0,32));

  preproc.ppf[10] = filter_interp_create(31);
  preproc.ppf[11] = filter_crop_create(32);

  smenu_append_item(fm, smenuitem_new("Application-Specific",1,0));
  smenu_append_item(fm, smenuitem_new("Phantom Lesion",0,41));
  //smenu_append_item(fm, smenuitem_new("Brain Gradient (SSR)",0,42));
  smenu_append_item(fm, smenuitem_new("Overlay Registration",0,43));
  //smenu_append_item(fm, smenuitem_new("Slice Homogenization",0,44));
  smenu_append_item(fm, smenuitem_new("Morphology",0,45));
  smenu_append_item(fm, smenuitem_new("Arithmetic",0,46));
  smenu_set_selection_id(fm, 1);

  preproc.ppf[12] = filter_plesion_create(41);
  preproc.ppf[13] = filter_ssr_create(42);
  preproc.ppf[14] = filter_reg_create(43);
  preproc.ppf[15] = filter_homog_create(44);
  preproc.ppf[16] = filter_morph_create(45);
  preproc.ppf[17] = filter_arith_create(46);

  gtk_box_pack_start(GTK_BOX(preproc.pane),fm->widget, FALSE, TRUE, 2);
  gtk_widget_show(fm->widget);

  /* revert / auto box */

  tmp[21] = gtk_vbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[21]), 4);

  tmp[22] = gtk_button_new_with_label("Revert to Original");
  tmp[23] = gtk_button_new_with_label("Swap with Overlay");

  gtk_box_pack_start(GTK_BOX(preproc.pane), tmp[21], FALSE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(tmp[21]), tmp[22], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tmp[21]), tmp[23], FALSE, TRUE, 0);

  /* view mode */

  tmp[24] = gtk_table_new(3,2,FALSE);

  tmp[25] = gtk_label_new("Colors:");
  preproc.colormaps = dropbox_new(";Gray;Hotmetal;Spectrum A;Spectrum B;Red/Yellow;Neon");

  tmp[36] = gtk_label_new("Zoom:");
  preproc.zoomw = dropbox_new(";0.5x;1x;2x;3x;4x");

  tmp[47] = gtk_label_new("Display:");
  preproc.ovlw = dropbox_new(";Segmentation Volume;Overlay Volume");

  tmp[37] = gtk_hbox_new(FALSE,0);
  tmp[38] = gtk_hbox_new(FALSE,0);
  tmp[48] = gtk_hbox_new(FALSE,0);

  gtk_box_pack_start(GTK_BOX(tmp[37]),preproc.colormaps->widget,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[38]),preproc.zoomw->widget,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[48]),preproc.ovlw->widget,FALSE,FALSE,0);

  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[25],0,1,0,1,
		   GTK_SHRINK,GTK_SHRINK,4,2);
  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[37],1,2,0,1,
		   GTK_FILL,GTK_SHRINK,4,2);

  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[36],0,1,1,2,
		   GTK_SHRINK,GTK_SHRINK,4,2);
  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[38],1,2,1,2,
		   GTK_FILL,GTK_SHRINK,4,2);

  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[47],0,1,2,3,
		   GTK_SHRINK,GTK_SHRINK,4,2);
  gtk_table_attach(GTK_TABLE(tmp[24]),tmp[48],1,2,2,3,
		   GTK_FILL,GTK_SHRINK,4,2);

  gtk_box_pack_end(GTK_BOX(preproc.pane), tmp[24], FALSE, TRUE,0);
  
  /* ---- */

  for(i=0;i<49;i++)
    if (tmp[i])
      gtk_widget_show(tmp[i]);

  gtk_widget_show(preproc.canvas);
  gtk_widget_show(preproc.pane);

  /* CANVAS */
  gtk_signal_connect(GTK_OBJECT(preproc.canvas),"expose_event",
                     GTK_SIGNAL_FUNC(ivs_preproc_expose), 0);
  gtk_signal_connect(GTK_OBJECT(preproc.canvas),"button_press_event",
                     GTK_SIGNAL_FUNC(ivs_preproc_press), 0);
  gtk_signal_connect(GTK_OBJECT(preproc.canvas),"button_release_event",
                     GTK_SIGNAL_FUNC(ivs_preproc_release), 0);
  gtk_signal_connect(GTK_OBJECT(preproc.canvas),"motion_notify_event",
                     GTK_SIGNAL_FUNC(ivs_preproc_drag), 0);

  /* REVERT */
  gtk_signal_connect(GTK_OBJECT(tmp[22]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_apply_revert), 0);  
  /* SWAP */
  gtk_signal_connect(GTK_OBJECT(tmp[23]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_apply_swap), 0);  

  dropbox_set_selection(preproc.zoomw,1);
  dropbox_set_changed_callback(preproc.colormaps, &ivs_select_colormap);
  dropbox_set_changed_callback(preproc.zoomw, &ivs_select_zoom);
  dropbox_set_changed_callback(preproc.ovlw, &ivs_select_display);
}

void preproc_filter_change(SMenu *sm, int id) {
  int i,j,lj;

  preproc_open_ppdialog();

  for(i=0;i<preproc.nfilters;i++)
    if (preproc.ppf[i]->id != id)
      gtk_widget_hide(preproc.ppf[i]->control);

  j = -1;
  for(i=0;i<preproc.nfilters;i++)
    if (preproc.ppf[i]->id == id) {
      gtk_widget_show(preproc.ppf[i]->control);
      j = i;
      break;
    }

  if (j<0) return;
  if (j==preproc.lastfilter) return;
  
  lj = preproc.lastfilter;
  preproc.lastfilter = -1;

  if (lj >= 0) {
    if (preproc.ppf[lj]->deactivate)
      preproc.ppf[lj]->deactivate(preproc.ppf[lj]);
  }

  if (preproc.gotpreview) {
    preproc.invalid = 1;
    preproc.bounds.vmin = -1;
  }
  preproc.gotpreview = 0;
  
  preproc.lastfilter = j;

  if (preproc.ppf[j]->activate)
    preproc.ppf[j]->activate(preproc.ppf[j]);
}

void cb_default_repaint(PreProcFilter *pp) {
  preproc.invalid = 1;
  preproc.bounds.vmin = -1;
  refresh(preproc.canvas);
}

void cb_default_deactivate(PreProcFilter *pp) {
  preproc.gotpreview = 0;
  preproc.invalid = 1;
  preproc.bounds.vmin = -1;
  refresh(preproc.canvas);
}

void preproc_open_ppdialog() {
  int i;
  GtkWidget *v;

  if (preproc.ppdialog != 0) {
    gtk_widget_show(preproc.ppdialog);
    return;
  }

  preproc.ppdialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(preproc.ppdialog),"IVS - Filter Options");
  gtk_window_set_resizable(GTK_WINDOW(preproc.ppdialog),FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(preproc.ppdialog),
			       GTK_WINDOW(mainwindow));

  v = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(preproc.ppdialog), v);

  for(i=0;i<preproc.nfilters;i++)
    gtk_box_pack_start(GTK_BOX(v), preproc.ppf[i]->control, FALSE, TRUE, 0);

  gtk_widget_show(v);
  gtk_widget_show(preproc.ppdialog);

  gtk_signal_connect(GTK_OBJECT(preproc.ppdialog),"delete_event",
		     GTK_SIGNAL_FUNC(preproc_ppdialog_delete),0);
  gtk_signal_connect(GTK_OBJECT(preproc.ppdialog),"destroy",
		     GTK_SIGNAL_FUNC(preproc_ppdialog_destroy),0);
}

gboolean preproc_ppdialog_delete(GtkWidget *w, GdkEvent *e, gpointer data) 
{ 
  gtk_widget_hide(w);
  return TRUE;
}

void preproc_ppdialog_destroy(GtkWidget *w, gpointer data) {
  if (preproc.gotpreview) {
    preproc.invalid = 1;
    preproc.bounds.vmin = -1;
    preproc.gotpreview = 0;
    refresh(preproc.canvas);
  }
  preproc.ppdialog = 0;
}

void ivs_select_colormap(void *obj) {
  preproc.cmap = dropbox_get_selection(preproc.colormaps);
  preproc.invalid = 1;

  ViewMapLoadPalette(preproc.viewrange.vmap_o, preproc.cmap);
  ViewMapLoadPalette(preproc.viewrange.vmap_c, preproc.cmap);
  ViewMapLoadPalette(preproc.viewrange.vmap_x, preproc.cmap);

  refresh(preproc.canvas);
}

void ivs_select_display(void *obj) {
  int i;
  i = dropbox_get_selection(preproc.ovlw);

  if (scene->mipvol == 0 && i>0) {
    dropbox_set_selection(preproc.ovlw, 0);
    PopMessageBox(mainwindow, "Error", "There is no overlay volume loaded.\nUse the Tools menu to load a new one.",MSG_ICON_ERROR);
    return;
  }

  preproc.show_overlay = (i==0) ? 0 : 1;
  preproc.invalid = 1;
  refresh(preproc.canvas);
}

void ivs_select_zoom(void *obj) {
  int i;

  i = dropbox_get_selection(preproc.zoomw);

  switch(i) {
  case 0:  preproc.zoom = 0.5; break;
  default: preproc.zoom = (float) i; break;
  }

  preproc.invalid = 1;
  refresh(preproc.canvas);
}

void ivs_apply_swap(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  if (!scene->mipvol) {
    PopMessageBox(mainwindow, "Error", "There is no overlay volume loaded.\nUse the Tools menu to load a new one.",MSG_ICON_ERROR);
    return;
  }

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  TaskNew(qseg, "Filter: swap overlay", (Callback) &be_filter_swap,
	  (void *) args);
}

void ivs_apply_revert(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  TaskNew(qseg, "Filter: revert to original", (Callback) &be_filter_revert,
	  (void *) args);
}

gboolean
ivs_preproc_drag(GtkWidget *widget,GdkEventMotion *em,gpointer data)
{
  int x,y;
  GdkEventButton e;

  if (em->state & GDK_BUTTON2_MASK && preproc.wiredrag) {
    x = (int) (em->x);
    y = (int) (em->y);

    preproc.panx += preproc.pandrag.px - x;
    preproc.pany += preproc.pandrag.py - y;

    if (preproc.panx > preproc.rW) preproc.panx = preproc.rW;
    if (preproc.panx < -preproc.vW) preproc.panx = -preproc.vW;
    if (preproc.pany > preproc.rH) preproc.pany = preproc.rH;
    if (preproc.pany < -preproc.vH) preproc.pany = -preproc.vH;

    preproc.pandrag.px = x;
    preproc.pandrag.py = y;
    refresh(preproc.canvas);
  }
  if (em->state & GDK_BUTTON1_MASK) {
    e.x = em->x;
    e.y = em->y;
    e.button = 1;
    e.state = em->state;
    ivs_preproc_press(widget,&e,0);
  }
  return FALSE;
}

gboolean
ivs_preproc_release(GtkWidget *widget,GdkEventButton *eb,gpointer data)
{
  if (preproc.wiredrag) {
    if (preproc.rgc) {
      gdk_gc_destroy(preproc.rgc);
      preproc.rgc = 0;
    }
    if (preproc.wire) {
      gdk_pixmap_unref(preproc.wire);
      preproc.wire = 0;
    }
    
    preproc.wiredrag = 0;
    gtk_grab_remove(preproc.canvas);
    refresh(preproc.canvas);
    return FALSE;
  }

  if (scene->avol)
    if (preproc.show_original) {
      preproc.show_original = 0;
      preproc.invalid = 1;
      refresh(preproc.canvas);
    }
  return FALSE;
}

int  ivs_pp_s2v(Point *screen, Point *volume) {
  XAnnVolume *avol = scene->avol;
  int x,y,i,j,k,t;
  x = (int) (screen->x);
  y = (int) (screen->y);

  if (!avol) return -1;

  /* XY */
  if (inbox(x+preproc.panx,y+preproc.pany, 
	    preproc.boxlu[0][0], preproc.boxlu[0][1], 
	    preproc.boxsz[0][0], preproc.boxsz[0][1])) {

    i = (x - preproc.boxlu[0][0] + preproc.panx) / preproc.zoom;
    j = (y - preproc.boxlu[0][1] + preproc.pany) / preproc.zoom;
    k = preproc.slice[0];

    switch(preproc.rot[0]) {
    case 1: t = i; i = j; j = avol->H - t - 1; break;
    case 2: i = avol->W - i - 1; j = avol->H - j - 1; break;
    case 3: t = i; i = avol->W - j - 1; j = t; break;
    }

    PointAssign(volume, i, j, k);
    return 0;
  }

  if (inbox(x+preproc.panx,y+preproc.pany, 
	    preproc.boxlu[1][0], preproc.boxlu[1][1], 
	    preproc.boxsz[1][0], preproc.boxsz[1][1])) {
    i = (x - preproc.boxlu[1][0] + preproc.panx) / preproc.zoom;
    j = (y - preproc.boxlu[1][1] + preproc.pany) / preproc.zoom;
    k = preproc.slice[1];

    switch(preproc.rot[1]) {
    case 1: t = i; i = j; j = avol->H - t - 1; break;
    case 2: i = avol->D - i - 1; j = avol->H - j - 1; break;
    case 3: t = i; i = avol->D - j - 1; j = t; break;
    }
    
    PointAssign(volume, k, j, i);
    return 1;
  }

  if (inbox(x+preproc.panx,y+preproc.pany, 
	    preproc.boxlu[2][0], preproc.boxlu[2][1], 
	    preproc.boxsz[2][0], preproc.boxsz[2][1])) {
    i = (x - preproc.boxlu[2][0] + preproc.panx) / preproc.zoom;
    j = (y - preproc.boxlu[2][1] + preproc.pany) / preproc.zoom;
    k = preproc.slice[2];

    switch(preproc.rot[2]) {
    case 1: t = i; i = j; j = avol->D - t - 1; break;
    case 2: i = avol->W - i - 1; j = avol->D - j - 1; break;
    case 3: t = i; i = avol->W - j - 1; j = t; break;
    }

    PointAssign(volume, i, k, j);
    return 2;
  }

  return -1;
}

void ivs_pp_v2s(Point *volume, Point *xy, Point *yz, Point *xz, int clip) {
  XAnnVolume * avol = scene->avol;
  int W = avol->W, H=avol->H, D=avol->D;
  int i,j,k,xt,yt,xs,ys;
  Transform R;
  Point P;

  if (!avol) return;  

  if (clip) {
    if (volume->x < 0) volume->x = 0;
    if (volume->y < 0) volume->y = 0;
    if (volume->z < 0) volume->z = 0;
    if (volume->x >= W) volume->x = W-1;
    if (volume->y >= H) volume->y = H-1;
    if (volume->z >= D) volume->z = D-1;
  }

  /* XY */
  i = (volume->x * preproc.zoom) + preproc.boxlu[0][0];
  j = (volume->y * preproc.zoom) + preproc.boxlu[0][1];
  k = preproc.slice[0];

  xt = preproc.boxlu[0][0] + (preproc.zoom * W) / 2;
  yt = preproc.boxlu[0][1] + (preproc.zoom * H) / 2;
  xs = preproc.boxlu[0][0] + preproc.boxsz[0][0] / 2;
  ys = preproc.boxlu[0][1] + preproc.boxsz[0][1] / 2;

  PointAssign(&P, i - xt, j - yt, k);
  TransformZRot(&R, 90.0 * preproc.rot[0]);
  PointTransform(xy, &P, &R);
  PointTranslate(xy, xs, ys, 0);

  /* ZY */
  i = (volume->z * preproc.zoom) + preproc.boxlu[1][0];
  j = (volume->y * preproc.zoom) + preproc.boxlu[1][1];
  k = preproc.slice[1];

  xt = preproc.boxlu[1][0] + (preproc.zoom * D) / 2;
  yt = preproc.boxlu[1][1] + (preproc.zoom * H) / 2;
  xs = preproc.boxlu[1][0] + preproc.boxsz[1][0] / 2;
  ys = preproc.boxlu[1][1] + preproc.boxsz[1][1] / 2;

  PointAssign(&P, i - xt, j - yt, k);
  TransformZRot(&R, 90.0 * preproc.rot[1]);
  PointTransform(yz, &P, &R);
  PointTranslate(yz, xs, ys, 0);

  /* XZ */
  i = (volume->x * preproc.zoom) + preproc.boxlu[2][0];
  j = (volume->z * preproc.zoom) + preproc.boxlu[2][1];
  k = preproc.slice[2];

  xt = preproc.boxlu[2][0] + (preproc.zoom * W) / 2;
  yt = preproc.boxlu[2][1] + (preproc.zoom * D) / 2;
  xs = preproc.boxlu[2][0] + preproc.boxsz[2][0] / 2;
  ys = preproc.boxlu[2][1] + preproc.boxsz[2][1] / 2;

  PointAssign(&P, i - xt, j - yt, k);
  TransformZRot(&R, 90.0 * preproc.rot[2]);
  PointTransform(xz, &P, &R);
  PointTranslate(xz, xs, ys, 0);
}


gboolean
ivs_preproc_press(GtkWidget *widget,GdkEventButton *eb,gpointer data)
{
  int x,y,i,j,tool,toolid;
  float f;
  XAnnVolume * avol = scene->avol;
  Point p,q;

  int crop_ab, cx,cy,cz;

  x = (int) eb->x; y = (int) eb->y;

  if (!avol) return FALSE;

  //  tool = dropbox_get_selection(preproc.filters);
  tool = preproc.lastfilter;
  if (tool >= 0 && tool < preproc.nfilters) {
    toolid = preproc.ppf[tool]->id;
  } else {
    toolid = -1;
  }

  if (eb->button == 3 && toolid != PP_CROP_ID) {

    if (inbox(x+preproc.panx,y+preproc.pany, 
	      1, 1, 24, 256)) {
      preproc.viewrange.active->minlevel = 0.0;
      preproc.viewrange.active->maxlevel = 1.0;
      preproc.invalid = 1;
      refresh(preproc.canvas);
      return FALSE;
    }

    preproc.show_original = 1;
    preproc.invalid = 1;
    refresh(preproc.canvas);
    return FALSE;
  }

  if (eb->button == 2) {
    if (scene->avol) {
      preproc.pandrag.px = x;
      preproc.pandrag.py = y;
      preproc.wiredrag = 1;
      gtk_grab_add(preproc.canvas);

      if (preproc.rgc) {
	gdk_gc_destroy(preproc.rgc);
	preproc.rgc = 0;
      }
      if (preproc.wire) {
	gdk_pixmap_unref(preproc.wire);
	preproc.wire = 0;
      }

      preproc.wire = gdk_pixmap_new(preproc.canvas->window,
				    preproc.rW, preproc.rH, -1);
      preproc.rgc = gdk_gc_new(preproc.wire);

      refresh(preproc.canvas);
      return FALSE;
    }
  }

  p.x = x;
  p.y = y;
  i = ivs_pp_s2v(&p, &q);

  // click sets crop endpoint
  if (toolid == PP_CROP_ID && ((eb->state&GDK_CONTROL_MASK) == 0)) {
    if (i>=0) {
      crop_ab = eb->button == 3 ? 1 : 0;

      cx = q.x;
      cy = q.y;
      cz = q.z;
      
      switch(i) {
      case 0: cz = -1; break;
      case 1: cx = -1; break;
      case 2: cy = -1; break;
      }
      
      if (preproc.ppf[tool]->whatever != 0)
	preproc.ppf[tool]->whatever(preproc.ppf[tool],0,crop_ab,cx,cy,cz);
      return FALSE;
    }
  }
  
  // click sets overlay registration mark
  if (toolid == PP_REG_ID && ((eb->state&GDK_CONTROL_MASK) == 0)) {
    if (i>=0) {
      cx = q.x;
      cy = q.y;
      cz = q.z;

      switch(i) {
      case 0: cz = preproc.slice[0]; break;
      case 1: cx = preproc.slice[1]; break;
      case 2: cy = preproc.slice[2]; break;
      }

      if (preproc.ppf[tool]->whatever != 0)
	preproc.ppf[tool]->whatever(preproc.ppf[tool],
				    0,preproc.show_overlay,cx,cy,cz);
      return FALSE;
    }
  }

  switch(i) {
  case 0:
    preproc.slice[1] = q.x;
    preproc.slice[2] = q.y;
    preproc_refresh_preview();
    refresh(preproc.canvas);
    return FALSE;
  case 1:
    preproc.slice[0] = q.z;
    preproc.slice[2] = q.y;
    preproc_refresh_preview();
    refresh(preproc.canvas);
    return FALSE;
  case 2:
    preproc.slice[1] = q.x;
    preproc.slice[0] = q.z;
    preproc_refresh_preview();
    refresh(preproc.canvas);
    return FALSE;
  }    

  if (inbox(x+preproc.panx,y+preproc.pany, 
	    1, 1, 24, 256)) {
    
    y += preproc.pany;

    f = (256.0 - y) / 255.0;

    if (fabs(f-preproc.viewrange.active->minlevel) < 
	fabs(f-preproc.viewrange.active->maxlevel)) {
      preproc.viewrange.active->minlevel = f;
      if (preproc.viewrange.active->maxlevel < f)
	preproc.viewrange.active->maxlevel = f + 0.1;
    } else {
      preproc.viewrange.active->maxlevel = f;
      if (preproc.viewrange.active->minlevel > f)
	preproc.viewrange.active->minlevel = f - 0.1;
    }

    if (preproc.viewrange.active->maxlevel > 1.0)
      preproc.viewrange.active->maxlevel = 1.0;
    if (preproc.viewrange.active->minlevel > 1.0)
      preproc.viewrange.active->minlevel = 1.0;
    if (preproc.viewrange.active->maxlevel < 0.0)
      preproc.viewrange.active->maxlevel = 0.0;
    if (preproc.viewrange.active->minlevel < 0.0)
      preproc.viewrange.active->minlevel = 0.0;

    preproc.invalid = 1;
    refresh(preproc.canvas);
    return FALSE;
  }

  for(i=0;i<3;i++)
    for(j=0;j<2;j++) {

      if (inbox(x+preproc.panx,y+preproc.pany,
		preproc.rotc.x[i][j],
		preproc.rotc.y[i][j],
		16,16)) {
	preproc.rot[i] += (j==0) ? 1 : 3;
	preproc.rot[i] %= 4;
	preproc.invalid = 1;
	refresh(preproc.canvas);
	return FALSE;
      }

    }
  

  return FALSE;
}

void preproc_refresh_preview() {

  if (preproc.ppdialog)
    if (preproc.lastfilter >= 0)
      if (preproc.ppf[preproc.lastfilter]->updatepreview)
	preproc.ppf[preproc.lastfilter]->updatepreview(preproc.ppf[preproc.lastfilter]);
}

void ivs_preproc_draw_wire(GtkWidget *widget) {
  int i,j,c1,c2;
  float dx, dy;
  PangoRectangle pri, prl;

  char z[64];
  int ww, wh;

  gc_color(preproc.rgc,0);
  gdk_draw_rectangle(preproc.wire,preproc.rgc,TRUE,0,0,
		     preproc.rW,preproc.rH);
  
  gc_color(preproc.rgc, scene->colors.bg);
  gdk_draw_rectangle(preproc.wire,preproc.rgc,TRUE,
		     -preproc.panx, -preproc.pany,
		     preproc.vW, preproc.vH);
  
  c1 = scene->colors.clipcube;
  c2 = mergeColorsRGB(c1,scene->colors.bg,0.50);
  gc_color(preproc.rgc, c1);
  gdk_draw_rectangle(preproc.wire,preproc.rgc,FALSE,
		     -preproc.panx, -preproc.pany,
		     preproc.vW, preproc.vH);
  
  if (scene->avol) {   
    
    gc_color(preproc.rgc, c2);

    for(j=0;j<3;j++) {
      dx = ((float) (preproc.boxsz[j][0])) / 10.0;
      dy = ((float) (preproc.boxsz[j][1])) / 10.0;

      for(i=0;i<11;i++) {
	gdk_draw_line(preproc.wire,preproc.rgc,
		      -preproc.panx + preproc.boxlu[j][0] + i * dx,
		      -preproc.pany + preproc.boxlu[j][1],
		      -preproc.panx + preproc.boxlu[j][0] + i * dx,
		      -preproc.pany + preproc.boxlu[j][1] + preproc.boxsz[j][1]);
	
	gdk_draw_line(preproc.wire,preproc.rgc,
		      -preproc.panx + preproc.boxlu[j][0],
		      -preproc.pany + preproc.boxlu[j][1] + i * dy,
		      -preproc.panx + preproc.boxlu[j][0] + preproc.boxsz[j][0],
		      -preproc.pany + preproc.boxlu[j][1] + i * dy);
      }
    }
    
    gc_color(preproc.rgc, c1);
    for(i=0;i<3;i++)
      gdk_draw_rectangle(preproc.wire,preproc.rgc,FALSE,
			 -preproc.panx + preproc.boxlu[i][0],
			 -preproc.pany + preproc.boxlu[i][1],
			 preproc.boxsz[i][0],
			 preproc.boxsz[i][1]);
  }
  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  preproc.wire,
		  0,0,
		  0,0,
		  preproc.rW,preproc.rH);

  gdk_window_get_size(widget->window, &ww, &wh);

  sprintf(z,"[Pan View by %d %d]",-preproc.panx, -preproc.pany);

  pango_layout_set_text(preproc.pl, z, -1);
  pango_layout_get_pixel_extents(preproc.pl, &pri, &prl);

  ww/=2;
  wh/=2;
  ww-= prl.width / 2;
  wh-= prl.height / 2;

  for(i=-1;i<=1;i++)
    for(j=-1;j<=1;j++)
      if (i || j)
	gdk_draw_layout(widget->window,
			widget->style->black_gc,
			ww+i, wh+j, preproc.pl);

  gdk_draw_layout(widget->window,
		  widget->style->white_gc,
		  ww,wh,preproc.pl);
}

gboolean
ivs_preproc_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data)
{
  int w,h,d,ww,wh,vw,vh,i,j;
  
  gdk_window_get_size(widget->window, &ww, &wh);

  if (scene->avol) {

    w = scene->avol->W;
    h = scene->avol->H;
    d = scene->avol->D;

    vw = (max3(w,h,d)+max2(d,h))*preproc.zoom + 128;
    vh = (2*max3(w,h,d)*preproc.zoom) + 128;

    if (vw < ww) vw = ww;
    if (vh < wh) vh = wh;

  } else {

    w=h=d=0;
    vw = ww;
    vh = wh;

  }

  preproc.rW = ww;
  preproc.rH = wh;  

  /* calculate box dims and locations*/

  preproc.boxsz[0][0] = preproc.rot[0] % 2 ? h : w;
  preproc.boxsz[0][1] = preproc.rot[0] % 2 ? w : h;
  preproc.boxsz[1][0] = preproc.rot[1] % 2 ? h : d;
  preproc.boxsz[1][1] = preproc.rot[1] % 2 ? d : h;
  preproc.boxsz[2][0] = preproc.rot[2] % 2 ? d : w;
  preproc.boxsz[2][1] = preproc.rot[2] % 2 ? w : d;

  for(i=0;i<3;i++)
    for(j=0;j<2;j++) {
      preproc.boxszz[i][j] = preproc.boxsz[i][j];
      preproc.boxsz[i][j] = (int) ceil(preproc.boxsz[i][j]*preproc.zoom);
    }

  /* locate origin and axis directions */

  preproc.boxlu[0][0] = 32;
  preproc.boxlu[0][1] = 32;

  preproc.boxlu[1][0] = 32 + preproc.boxsz[0][0] + 24;
  preproc.boxlu[1][1] = 32;

  preproc.boxlu[2][0] = 32;
  preproc.boxlu[2][1] = 32 + preproc.boxsz[0][1] + 24;

  /* ensure size of preproc.vista pixmap is correct */
  if (preproc.vista == 0 || vw != preproc.vW || vh != preproc.vH) {
    if (preproc.vista) {
      gdk_pixmap_unref(preproc.vista);
      preproc.vista = 0;
    }
    if (preproc.gc) {
      gdk_gc_destroy(preproc.gc);
      preproc.gc = 0;
    }
    preproc.vW = vw;
    preproc.vH = vh;

    preproc.vista = gdk_pixmap_new(widget->window, preproc.vW, preproc.vH, -1);
    preproc.invalid = 1;
  }

  if (preproc.wiredrag) {
    ivs_preproc_draw_wire(widget);
    return FALSE;
  }

  compose_preproc_vista();

  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  preproc.vista,
		  preproc.panx,preproc.pany,
		  0,0,
		  ww,wh);

  return FALSE;
}

void compose_preproc_vista() {
  XAnnVolume *avol = scene->avol;
  Volume *pvol = preproc.preview;
  int i,j,k,m,n,tx,ty,tmpint[3];
  int mhd,fh;
  int c, tc1, tc2;
  char z[256];
  CImg *tmpimg;
  PreProcFilter *ppf;
  Volume *mipvol = scene->mipvol;
  PangoRectangle pri, prl;

  float vu, vl;

  float dx,dy,dz;

  Point p,q,r,s,q1,r1,s1,t0[3],tf[3];

  GdkRectangle gclip;
  static gchar dash3[2] = {1,1}, dash4[2] = {4,4};

  if (!preproc.gc)
    preproc.gc = gdk_gc_new(preproc.vista);

  // create images

  if (avol) {

    if (preproc.bounds.omin == -1) {
      preproc.bounds.omin = SMAX;
      preproc.bounds.omax = 0;
      for(i=0;i<avol->N;i++)
	if (avol->vd[i].orig > preproc.bounds.omax)
	  preproc.bounds.omax = avol->vd[i].orig;
	else if (avol->vd[i].orig < preproc.bounds.omin)
	  preproc.bounds.omin = avol->vd[i].orig;
    }

    if (preproc.bounds.vmin == -1 || preproc.gotpreview) {
      preproc.bounds.vmin = SMAX;
      preproc.bounds.vmax = 0;
      for(i=0;i<avol->N;i++)
	if (avol->vd[i].value > preproc.bounds.vmax)
	  preproc.bounds.vmax = avol->vd[i].value;
	else if (avol->vd[i].value < preproc.bounds.vmin)
	  preproc.bounds.vmin = avol->vd[i].value;
    }    

    if (preproc.cut[0][0]) {
      if (preproc.cut[0][0]->W != preproc.boxszz[0][0] || 
	  preproc.cut[0][0]->H != preproc.boxszz[0][1] ||
	  preproc.cut[0][1]->W != preproc.boxszz[1][0] || 
	  preproc.cut[0][1]->H != preproc.boxszz[1][1] ||
	  preproc.cut[0][2]->W != preproc.boxszz[2][0] || 
	  preproc.cut[0][2]->H != preproc.boxszz[2][1] ) {
	for(j=0;j<3;j++)
	  for(i=0;i<3;i++)
	    CImgDestroy(preproc.cut[j][i]);
	preproc.cut[0][0] = 0;
      }
    }

    if (preproc.cut[0][0] == 0) {
      for(i=0;i<3;i++)
	for(j=0;j<3;j++)
	  preproc.cut[j][i] = CImgNew(preproc.boxszz[i][0],
				      preproc.boxszz[i][1]);
      preproc.invalid = 1;
    }    

    if (preproc.slice[0] < 0) preproc.slice[0] = 0;
    if (preproc.slice[1] < 0) preproc.slice[1] = 0;
    if (preproc.slice[2] < 0) preproc.slice[2] = 0;
    if (preproc.slice[0] >= avol->D ) preproc.slice[0] = avol->D - 1;
    if (preproc.slice[1] >= avol->W ) preproc.slice[1] = avol->W - 1;
    if (preproc.slice[2] >= avol->H ) preproc.slice[2] = avol->H - 1;

    for(i=0;i<3;i++)
      if (preproc.slice[i] != preproc.prev.s[i])
	preproc.invalid = 1;
    if (preproc.zoom != preproc.prev.zoom)
      preproc.invalid = 1;

    if (!preproc.invalid)
      return;

    gc_color(preproc.gc, scene->colors.bg);
    gdk_draw_rectangle(preproc.vista, preproc.gc, TRUE, 0,0, 
		       preproc.vW, preproc.vH);

    preproc.viewrange.active = preproc.show_original ?
      preproc.viewrange.vmap_o : preproc.viewrange.vmap_c;
    if (preproc.show_overlay)
      preproc.viewrange.active = preproc.viewrange.vmap_x;

    preproc_viewmap_calc();

    switch(preproc.rot[0]) {
    case 0: /*   0 */
    case 2: /* 180 */

      if (!preproc.gotpreview) {
	for(j=0;j<avol->H;j++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[j] + avol->tbframe[ preproc.slice[0] ];
	    CImgSet(preproc.cut[0][0], i,j, 
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][0], i,j, 
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][0], i,j,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[j]+mipvol->tbframe[ preproc.slice[0] ]]));
				   
	  }
      } else {
	for(j=0;j<avol->H;j++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[j] + avol->tbframe[ preproc.slice[0] ];
	    CImgSet(preproc.cut[0][0], i,j, 
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][0], i,j, 
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][0], i,j,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[j]+mipvol->tbframe[ preproc.slice[0] ]]));
	  }
      }
      break;

    case 1: /*  90 */
    case 3: /* 270 */
    
      m = avol->H - 1;
      if (!preproc.gotpreview) {
	for(j=0;j<avol->H;j++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[j] + avol->tbframe[ preproc.slice[0] ];
	    CImgSet(preproc.cut[0][0], m-j,i,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][0], m-j,i, 
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][0], m-j,i,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[j]+mipvol->tbframe[ preproc.slice[0] ]]));
	  }
      } else {
	for(j=0;j<avol->H;j++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[j] + avol->tbframe[ preproc.slice[0] ];
	    CImgSet(preproc.cut[0][0], m-j,i,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][0], m-j,i, 
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][0], m-j,i,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[j]+mipvol->tbframe[ preproc.slice[0] ]]));
	  }
      }
      break;

    }

    switch(preproc.rot[1]) {
    case 0:
    case 2:

      if (!preproc.gotpreview) {
	for(k=0;k<avol->D;k++)
	  for(j=0;j<avol->H;j++) {
	    n = preproc.slice[1] + avol->tbrow[j] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][1], k,j,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][1], k,j,
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][1], k,j,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[preproc.slice[1]+mipvol->tbrow[j]+mipvol->tbframe[k]]));
	  }
      } else {
	for(k=0;k<avol->D;k++)
	  for(j=0;j<avol->H;j++) {
	    n = preproc.slice[1] + avol->tbrow[j] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][1], k,j,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][1], k,j,
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][1], k,j,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[preproc.slice[1]+mipvol->tbrow[j]+mipvol->tbframe[k]]));
	  }
      }
      break;
    case 1:
    case 3:
      m = avol->H - 1;
      if (!preproc.gotpreview) {
	for(k=0;k<avol->D;k++)
	  for(j=0;j<avol->H;j++) {
	    n = preproc.slice[1] + avol->tbrow[j] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][1], m-j,k,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][1], m-j,k,
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][1], m-j,k,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[preproc.slice[1]+mipvol->tbrow[j]+mipvol->tbframe[k]]));
	  }
      } else {
	for(k=0;k<avol->D;k++)
	  for(j=0;j<avol->H;j++) {
	    n = preproc.slice[1] + avol->tbrow[j] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][1], m-j,k,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][1], m-j,k,
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][1], m-j,k,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[preproc.slice[1]+mipvol->tbrow[j]+mipvol->tbframe[k]]));
	  }
      }
      break;
    }
    
    switch(preproc.rot[2]) {
    case 0:
    case 2:
      if (!preproc.gotpreview) {
	for(k=0;k<avol->D;k++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[ preproc.slice[2] ] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][2], i,k,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][2], i,k,
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][2], i,k,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[preproc.slice[2]]+mipvol->tbframe[k]]));
	  }
      } else {
	for(k=0;k<avol->D;k++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[ preproc.slice[2] ] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][2], i,k,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][2], i,k,
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][2], i,k,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[preproc.slice[2]]+mipvol->tbframe[k]]));
	  }
      }
      break;
    case 1:
    case 3:
      m = avol->D - 1;
      if (!preproc.gotpreview) {
	for(k=0;k<avol->D;k++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[ preproc.slice[2] ] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][2], m-k,i,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][2], m-k,i,
		    ViewMapColor(preproc.viewrange.vmap_c, avol->vd[n].value));
	    if (mipvol)
	      CImgSet(preproc.cut[2][2], m-k,i,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[preproc.slice[2]]+mipvol->tbframe[k]]));
	  }
      } else {
	for(k=0;k<avol->D;k++)
	  for(i=0;i<avol->W;i++) {
	    n = i + avol->tbrow[ preproc.slice[2] ] + avol->tbframe[k];
	    CImgSet(preproc.cut[0][2], m-k,i,
		    ViewMapColor(preproc.viewrange.vmap_o, avol->vd[n].orig));
	    CImgSet(preproc.cut[1][2], m-k,i,
		    ViewMapColor(preproc.viewrange.vmap_c, pvol->u.i16[n]));
	    if (mipvol)
	      CImgSet(preproc.cut[2][2], m-k,i,
		      ViewMapColor(preproc.viewrange.vmap_x,mipvol->u.i16[i+mipvol->tbrow[preproc.slice[2]]+mipvol->tbframe[k]]));
	  }
      }
      break;
    }

    if (preproc.rot[0] > 1) {
      CImgRot180(preproc.cut[0][0]);
      CImgRot180(preproc.cut[1][0]);
      if (mipvol) CImgRot180(preproc.cut[2][0]);
    }
    if (preproc.rot[1] > 1) {
      CImgRot180(preproc.cut[0][1]);
      CImgRot180(preproc.cut[1][1]);
      if (mipvol) CImgRot180(preproc.cut[2][1]);
    }
    if (preproc.rot[2] > 1) {
      CImgRot180(preproc.cut[0][2]);
      CImgRot180(preproc.cut[1][2]);
      if (mipvol) CImgRot180(preproc.cut[2][2]);
    }

    preproc.invalid = 0;
    for(i=0;i<3;i++)
      preproc.prev.s[i] = preproc.slice[i];
    preproc.prev.zoom = preproc.zoom;

    mhd = avol->H; if (avol->D > mhd) mhd = avol->D;

    tx = preproc.boxlu[1][0];
    ty = preproc.boxlu[2][1];

    pango_layout_set_text(preproc.pl, "X", -1);
    pango_layout_get_pixel_extents(preproc.pl, &pri, &prl);
     
    fh = prl.height;
    fh += fh/2;

    c = scene->colors.bg;
    c = t0(c) + t1(c) + t2(c);
    c /= 3;
    if (c < 128) {
      tc1 = 0xffffff;
      tc2 = darker(scene->colors.bg, 6);
    } else {
      tc1 = 0;
      tc2 = darker(scene->colors.bg, 2);
    }

    shadow_text(preproc.vista, preproc.pl, preproc.gc, 32,preproc.boxlu[0][1]-18, 
		tc1, tc2,
		preproc.show_overlay?"Overlay Data":(
		preproc.show_original?"Original Data":
		                     "Current Preprocessed Data"));

    n = preproc.slice[1] + 
      avol->tbrow[preproc.slice[2]] + 
      avol->tbframe[ preproc.slice[0] ];

    sprintf(z,"voxel (%d,%d,%d) = %d",
	    preproc.slice[1], preproc.slice[2], preproc.slice[0],
	    preproc.show_overlay?mipvol->u.i16[n]:(
	    preproc.show_original?avol->vd[n].orig:avol->vd[n].value));

    shadow_text(preproc.vista, preproc.pl, preproc.gc, tx, ty,
		tc1, tc2, z);

    if (!preproc.show_overlay)
      sprintf(z,"min=%d max=%d",
	      preproc.show_original?preproc.bounds.omin:preproc.bounds.vmin, 
	      preproc.show_original?preproc.bounds.omax:preproc.bounds.vmax);
    else
      strcpy(z,"N/A");

    shadow_text(preproc.vista, preproc.pl, preproc.gc, tx, ty + fh,
		tc1, tc2, z);

    for(i=0;i<3;i++) {
      sprintf(z,"rot: %d", preproc.rot[i] * 90);
      shadow_text(preproc.vista, preproc.pl, preproc.gc, preproc.boxlu[i][0], 
		  preproc.boxlu[i][1] + preproc.boxsz[i][1],
		  tc1, tc2, z);
    }

    n = preproc.show_original ? 0 : 1;
    if (preproc.show_overlay) n = 2;

    /* zoom if required */

    if (preproc.zoom > 1.0) {
      for(i=0;i<3;i++)
	for(j=0;j<3;j++) {
	  tmpimg = CImgIntegerZoom(preproc.cut[j][i],preproc.zoom);
	  CImgDestroy(preproc.cut[j][i]);
	  preproc.cut[j][i] = tmpimg;
	}
    } else if (preproc.zoom < 0.9) {
      for(i=0;i<3;i++)
	for(j=0;j<3;j++) {
	  tmpimg = CImgHalfScale(preproc.cut[j][i]);
	  CImgDestroy(preproc.cut[j][i]);
	  preproc.cut[j][i] = tmpimg;
	}
    }

    PointAssign(&p, 0, 0, 0);
    ivs_pp_v2s(&p, &t0[0], &t0[1], &t0[2], 1);
    PointAssign(&p, avol->W - 1, avol->H - 1, avol->D - 1);
    ivs_pp_v2s(&p, &tf[0], &tf[1], &tf[2], 1);

    // img 0

    for(i=0;i<3;i++) {
      gdk_draw_rgb_image(preproc.vista, preproc.gc,
			 preproc.boxlu[i][0],preproc.boxlu[i][1],
			 preproc.cut[n][i]->W,preproc.cut[n][i]->H,
			 GDK_RGB_DITHER_NORMAL,
			 preproc.cut[n][i]->val, preproc.cut[n][i]->rowlen);

      gc_color(preproc.gc, scene->colors.clipcube);
      gdk_draw_rectangle(preproc.vista, preproc.gc, FALSE, 
			 preproc.boxlu[i][0], preproc.boxlu[i][1],
			 preproc.boxsz[i][0], preproc.boxsz[i][1]);
    }

    gc_color(preproc.gc, preproc.chcolor[preproc.cmap]);

    PointAssign(&p, preproc.slice[1], preproc.slice[2], preproc.slice[0]);
    ivs_pp_v2s(&p, &q, &r, &s, 1);

    gdk_draw_line(preproc.vista, preproc.gc, q.x, t0[0].y, q.x, tf[0].y);
    gdk_draw_line(preproc.vista, preproc.gc, t0[0].x, q.y, tf[0].x, q.y);

    gdk_draw_line(preproc.vista, preproc.gc, r.x, t0[1].y, r.x, tf[1].y);
    gdk_draw_line(preproc.vista, preproc.gc, t0[1].x, r.y, tf[1].x, r.y);

    gdk_draw_line(preproc.vista, preproc.gc, s.x, t0[2].y, s.x, tf[2].y);
    gdk_draw_line(preproc.vista, preproc.gc, t0[2].x, s.y, tf[2].x, s.y);

    /* lesion sphere, if tool is phantom lesion */
    if (preproc_tool_id() == PP_PLESION_ID) {

      ppf = preproc.ppf[preproc.lastfilter];

      if (ppf->whatever != 0) {

	j = ppf->whatever(ppf,0,0,0,0,0);

	dx = ((float)j) / avol->dx;
	dy = ((float)j) / avol->dy;
	dz = ((float)j) / avol->dz;

	PointAssign(&p,
		    preproc.slice[1] - dx,
		    preproc.slice[2] - dy,
		    preproc.slice[0] - dz);
	ivs_pp_v2s(&p, &q, &r, &s, 0);
	PointAssign(&p,
		    preproc.slice[1] + dx,
		    preproc.slice[2] + dy,
		    preproc.slice[0] + dz);
	ivs_pp_v2s(&p, &q1, &r1, &s1, 0);

	gdk_gc_set_line_attributes(preproc.gc, 1, GDK_LINE_ON_OFF_DASH, 
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_dashes(preproc.gc, 0, (gint8 *) dash3, 2);

	preproc_clip(&gclip, 0);
	gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	i = MIN(q.x, q1.x);
	j = MIN(q.y, q1.y);
	m = abs(q.x - q1.x);
	n = abs(q.y - q1.y);
	
	gdk_draw_arc(preproc.vista, preproc.gc, FALSE, 
		     i, j, m, n, 0, 64*360);


	preproc_clip(&gclip, 1);
	gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	i = MIN(r.x, r1.x);
	j = MIN(r.y, r1.y);
	m = abs(r.x - r1.x);
	n = abs(r.y - r1.y);
	
	gdk_draw_arc(preproc.vista, preproc.gc, FALSE, 
		     i, j, m, n, 0, 64*360);

	preproc_clip(&gclip, 2);
	gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	i = MIN(s.x, s1.x);
	j = MIN(s.y, s1.y);
	m = abs(s.x - s1.x);
	n = abs(s.y - s1.y);
	
	gdk_draw_arc(preproc.vista, preproc.gc, FALSE, 
		     i, j, m, n, 0, 64*360);

	gdk_gc_set_line_attributes(preproc.gc, 1, GDK_LINE_SOLID, 
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	gdk_gc_set_dashes(preproc.gc, 0, (gint8 *) dash4, 2);
	preproc_clip(&gclip, -1);
	gdk_gc_set_clip_rectangle(preproc.gc, &gclip);
      }

    }

    /* registration points, if tool is registration */

    if (preproc_tool_id() == PP_REG_ID) {

      ppf = preproc.ppf[preproc.lastfilter];

      if (ppf->whatever != 0) {
	j = preproc.show_overlay ? 1 : 0;
	for(i=0;i<4;i++) {
	  PointAssign(&p,
		      tmpint[0] = ppf->whatever(ppf,1,j,i,0,0),
		      tmpint[1] = ppf->whatever(ppf,1,j,i,1,0),
		      tmpint[2] = ppf->whatever(ppf,1,j,i,2,0));
	  ivs_pp_v2s(&p,&q,&r,&s,1);

	  sprintf(z,"#%d",i+1);

	  preproc_clip(&gclip, 0);
	  gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	  gdk_draw_line(preproc.vista, preproc.gc, 
			q.x-10, q.y-10, q.x+10, q.y+10);
	  gdk_draw_line(preproc.vista, preproc.gc, 
			q.x-10, q.y+10, q.x+10, q.y-10);

	  if (preproc.slice[0] == tmpint[2])
	    gdk_draw_arc(preproc.vista, preproc.gc, FALSE,
			 q.x-5,q.y-5,11,11,0,64*360);

	  pango_layout_set_text(preproc.pl, z, -1);
	  gdk_draw_layout(preproc.vista,preproc.gc,q.x,q.y,preproc.pl);

	  preproc_clip(&gclip, 1);
	  gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	  gdk_draw_line(preproc.vista, preproc.gc, 
			r.x-10, r.y-10, r.x+10, r.y+10);
	  gdk_draw_line(preproc.vista, preproc.gc, 
			r.x-10, r.y+10, r.x+10, r.y-10);

	  if (preproc.slice[1] == tmpint[0])
	    gdk_draw_arc(preproc.vista, preproc.gc, FALSE,
			 r.x-5,r.y-5,11,11,0,64*360);

	  pango_layout_set_text(preproc.pl, z, -1);
	  gdk_draw_layout(preproc.vista,preproc.gc,r.x,r.y,preproc.pl);

	  preproc_clip(&gclip, 2);
	  gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	  gdk_draw_line(preproc.vista, preproc.gc, 
			s.x-10, s.y-10, s.x+10, s.y+10);
	  gdk_draw_line(preproc.vista, preproc.gc, 
			s.x-10, s.y+10, s.x+10, s.y-10);
	  
	  if (preproc.slice[2] == tmpint[1])
	    gdk_draw_arc(preproc.vista, preproc.gc, FALSE,
			 s.x-5,s.y-5,11,11,0,64*360);

	  pango_layout_set_text(preproc.pl, z, -1);
	  gdk_draw_layout(preproc.vista,preproc.gc,s.x,s.y,preproc.pl);

	  preproc_clip(&gclip, -1);
	  gdk_gc_set_clip_rectangle(preproc.gc, &gclip);

	} // for i

      } // if ppf->whatever!=0

    } // if pp_reg_id


    /* clip rectangle, if tool is clip */

    if (preproc_tool_id() == PP_CROP_ID) {

      ppf = preproc.ppf[preproc.lastfilter];

      if (ppf->whatever != 0) {
	PointAssign(&p, 
		    ppf->whatever(ppf,1,0,0,0,0),
		    ppf->whatever(ppf,1,0,1,0,0),
		    ppf->whatever(ppf,1,0,2,0,0));
	ivs_pp_v2s(&p, &q, &r, &s, 1);
	PointAssign(&p, 
		    ppf->whatever(ppf,1,1,0,0,0),
		    ppf->whatever(ppf,1,1,1,0,0),
		    ppf->whatever(ppf,1,1,2,0,0));
	ivs_pp_v2s(&p, &q1, &r1, &s1, 1);
	
	strcpy(z,"A(L)");
	pango_layout_set_text(preproc.pl, z, -1);
	gdk_draw_layout(preproc.vista, preproc.gc, q.x, q.y, preproc.pl);
	gdk_draw_layout(preproc.vista, preproc.gc, r.x, r.y, preproc.pl);
	gdk_draw_layout(preproc.vista, preproc.gc, s.x, s.y, preproc.pl);
	
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, q.x-3, q.y-3, 6,6, 0, 64*360);
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, r.x-3, r.y-3, 6,6, 0, 64*360);
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, s.x-3, s.y-3, 6,6, 0, 64*360);
	
	strcpy(z,"B(R)");
	pango_layout_set_text(preproc.pl, z, -1);
	gdk_draw_layout(preproc.vista, preproc.gc, q1.x, q1.y, preproc.pl);
	gdk_draw_layout(preproc.vista, preproc.gc, r1.x, r1.y, preproc.pl);
	gdk_draw_layout(preproc.vista, preproc.gc, s1.x, s1.y, preproc.pl);
	
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, q1.x-3, q1.y-3, 6,6, 0, 64*360);
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, r1.x-3, r1.y-3, 6,6, 0, 64*360);
	gdk_draw_arc(preproc.vista, preproc.gc, TRUE, s1.x-3, s1.y-3, 6,6, 0, 64*360);
	
	i = (int) (MIN(q.x, q1.x));
	j = (int) (MAX(q.x, q1.x));
	m = (int) (MIN(q.y, q1.y));
	n = (int) (MAX(q.y, q1.y));
	
	gc_color(preproc.gc, preproc.chcolor[preproc.cmap]);
	gdk_gc_set_line_attributes(preproc.gc, 1, GDK_LINE_ON_OFF_DASH, 
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
	
	i = (int) (MIN(q.x, q1.x));
	j = (int) (MAX(q.x, q1.x));
	m = (int) (MIN(q.y, q1.y));
	n = (int) (MAX(q.y, q1.y));
	
	gdk_draw_rectangle(preproc.vista, preproc.gc, FALSE, i,m,j-i,n-m);
	
	i = (int) (MIN(r.x, r1.x));
	j = (int) (MAX(r.x, r1.x));
	m = (int) (MIN(r.y, r1.y));
	n = (int) (MAX(r.y, r1.y));
	
	gdk_draw_rectangle(preproc.vista, preproc.gc, FALSE, i,m,j-i,n-m);
	
	i = (int) (MIN(s.x, s1.x));
	j = (int) (MAX(s.x, s1.x));
	m = (int) (MIN(s.y, s1.y));
	n = (int) (MAX(s.y, s1.y));
	
	gdk_draw_rectangle(preproc.vista, preproc.gc, FALSE, i,m,j-i,n-m);
	
	gdk_gc_set_line_attributes(preproc.gc, 1, GDK_LINE_SOLID, 
				   GDK_CAP_BUTT, GDK_JOIN_MITER);
      }
    }

    /* rotation controls */

    c = scene->colors.bg;
    c = t0(c) + t1(c) + t2(c);
    c /= 3;
    if (c < 128)
      c = lighter(scene->colors.bg, 4);
    else
      c = darker(scene->colors.bg, 4);
    for(i=0;i<3;i++) {
      preproc.rotc.x[i][0] = preproc.boxlu[i][0] + preproc.boxsz[i][0] - 16;
      preproc.rotc.x[i][1] = preproc.boxlu[i][0] + preproc.boxsz[i][0] + 1;
      
      preproc.rotc.y[i][0] = preproc.boxlu[i][1] + preproc.boxsz[i][1] + 1;
      preproc.rotc.y[i][1] = preproc.boxlu[i][1] + preproc.boxsz[i][1] - 16;
      
      for(j=0;j<2;j++) {
	bevel_box(preproc.vista,
		  preproc.gc, c,
		  preproc.rotc.x[i][j], preproc.rotc.y[i][j],
		  16, 16);
	m = preproc.rotc.x[i][j];
	n = preproc.rotc.y[i][j];

	gc_color(preproc.gc, 0xffff00);
	gdk_draw_arc(preproc.vista, preproc.gc, FALSE,
		     m+3-11,n+3-11,21,21,270*64, 90*64);
	if (j) {
	  gdk_draw_line(preproc.vista, preproc.gc, m+13,n+2,m+9,n+6);
	  gdk_draw_line(preproc.vista, preproc.gc, m+13,n+2,m+15,n+6);
	} else {
	  gdk_draw_line(preproc.vista, preproc.gc, m+2,n+13,m+6,n+9);
	  gdk_draw_line(preproc.vista, preproc.gc, m+2,n+13,m+6,n+15);
	}
      }
    }

  } else { // if avol
    gc_color(preproc.gc, scene->colors.bg);
    gdk_draw_rectangle(preproc.vista, preproc.gc, TRUE, 0,0, 
		       preproc.vW, preproc.vH);
  }

  /* color scale / range control */
  for(i=0;i<256;i++) {
    j = ((float)(255-i)) * (preproc.viewrange.active->maxval / 255.0);
    gc_color(preproc.gc,
	     ViewMapColor(preproc.viewrange.active,j));
    gdk_draw_line(preproc.vista, preproc.gc, 4, i+1, 20, i+1);
  }
  
  gc_color(preproc.gc, 0xffffff);
  gdk_draw_rectangle(preproc.vista,preproc.gc,FALSE,4,0,16,257);

  vl = 255.0 * preproc.viewrange.active->minlevel;
  vu = 255.0 * preproc.viewrange.active->maxlevel;

  preproc.viewrange.ly = 256 - vl;
  bevel_box(preproc.vista, preproc.gc, 0xff8000,
	    0, 256-vl-4, 24, 8);

  preproc.viewrange.uy = 256 - vu;
  bevel_box(preproc.vista, preproc.gc, 0xff8000,
	    0, 256-vu-4, 24, 8);

}

void preproc_viewmap_calc() {
  XAnnVolume *avol = scene->avol;
  Volume *pvol = preproc.preview;
  Volume *mipvol = scene->mipvol;
  int i, mo=0, mv=0, mx=0;

  for(i=0;i<avol->N;i++) {
    mo = MAX(mo, avol->vd[i].orig);
    mv = MAX(mv, avol->vd[i].value);
  }
  
  if (preproc.gotpreview) {
    mv = 0;
    for(i=0;i<avol->N;i++)
      mv = MAX(mv, pvol->u.i16[i]);
  }

  mx = 1;
  if (mipvol)
    for(i=0;i<mipvol->N;i++)
      mx = MAX(mx, mipvol->u.i16[i]);

  preproc.viewrange.vmap_o->maxval = mo;
  preproc.viewrange.vmap_c->maxval = mv;
  preproc.viewrange.vmap_x->maxval = mx;

  preproc.viewrange.vmap_o->minlevel = preproc.viewrange.active->minlevel;
  preproc.viewrange.vmap_o->maxlevel = preproc.viewrange.active->maxlevel;
  preproc.viewrange.vmap_c->minlevel = preproc.viewrange.active->minlevel;
  preproc.viewrange.vmap_c->maxlevel = preproc.viewrange.active->maxlevel;
  preproc.viewrange.vmap_x->minlevel = preproc.viewrange.active->minlevel;
  preproc.viewrange.vmap_x->maxlevel = preproc.viewrange.active->maxlevel;

  ViewMapUpdate(preproc.viewrange.active);
}

/* HISTOGRAM WINDOW */

GtkWidget *hw=0, *grp;
DropBox *hsrc, *hscale;

struct _histogram {
  int *orig;
  int *curr;
  int maxxo, maxxc, maxyo, maxyc;
  int initialized;
  PangoLayout *pl;
  PangoFontDescription *pfd;
} Histogram;

void ivs_histogram_close(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(hw);
  hw = 0;
  if (Histogram.initialized) {
    free(Histogram.orig);
    free(Histogram.curr);
    Histogram.initialized = 0;
  }
}

gboolean  ivs_histogram_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  ivs_histogram_close(0,0);
  return TRUE;
}

gboolean
ivs_histogram_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data)
{
  int w,h;
  int i,j,k;
  GdkGC *gc;
  float x,y,z;
  int m1, m2;

  int *H;
  int mx,my;

  char t[32];

  if (!scene->avol)
    return FALSE;

  gdk_window_get_size(widget->window, &w, &h);

  if (!Histogram.initialized) {
    Histogram.initialized = 1;
    Histogram.orig = (int *) malloc(sizeof(int) * 32768);
    Histogram.curr = (int *) malloc(sizeof(int) * 32768);
    MemSet(Histogram.orig, 0, 32768 * sizeof(int));
    MemSet(Histogram.curr, 0, 32768 * sizeof(int));
    Histogram.maxxo = Histogram.maxxc = Histogram.maxyo = Histogram.maxyc = 0;

    for(i=0;i<scene->avol->N;i++) {
      Histogram.orig[ j = scene->avol->vd[i].orig ]++;
      Histogram.curr[ k = scene->avol->vd[i].value ]++;
      if (j > Histogram.maxxo)
	Histogram.maxxo = j;
      if (k > Histogram.maxxc)
	Histogram.maxxc = k;
      if (Histogram.orig[j] > Histogram.maxyo)
	Histogram.maxyo = Histogram.orig[j];
      if (Histogram.orig[k] > Histogram.maxyc)
	Histogram.maxyc = Histogram.orig[k];
    }
  }

  gc = gdk_gc_new(widget->window);
  gc_color(gc,0xffffff);
  gdk_draw_rectangle(widget->window, gc, TRUE, 0,0, w, h);

  m1 = dropbox_get_selection(hsrc);
  m2 = dropbox_get_selection(hscale);

  switch(m1) {
  case 0: H = Histogram.orig; mx = Histogram.maxxo; my = Histogram.maxyo; break;
  case 1: H = Histogram.curr; mx = Histogram.maxxc; my = Histogram.maxyc; break;
  default: H = Histogram.orig; mx = 1; my = 1;
  }
  if (my == 0) my = 1;
  
  switch(m2) {
  case 0: y = ((float)h) / ((float)my); break;
  case 1: y = ((float)h) / (log10(my)); break;
  default: y = 1.0;
  }  

  /* draw gridlines */
  gc_color(gc,0x808080);
  for(x=0.0;x<w;x+=100.0) {
    i = (int) (x * (((float)mx)/((float)w)));
    gdk_draw_line(widget->window, gc, 
		  (int) x, 0, (int) x, h);
    sprintf(t,"%d",i);
    pango_layout_set_text(Histogram.pl, t, -1);
    gdk_draw_layout(widget->window, gc, 4 + (int) x, 2, Histogram.pl);
  }

  gc_color(gc,0x4040c0);
  if (m2 == 0) {
    for(x=0.0;x<=h;x+=36.0) {
      i = (int) (x * (((float)my)/((float)h)));
      gdk_draw_line(widget->window, gc, 
		    (int) 0, h-1-x, w, h-1-x);
      sprintf(t,"%d",i);
      pango_layout_set_text(Histogram.pl, t, -1);
      gdk_draw_layout(widget->window, gc, w-80, h-1-x+2, Histogram.pl);
    }

  } else {
    for(x=1.0;;x++) {
      i = x * y;
      if (i>h) break;
      i = h - 1 - i;
      gdk_draw_line(widget->window, gc, 
		    (int) 0, i, w, i);
      sprintf(t,"10**%d",(int)x);
      pango_layout_set_text(Histogram.pl, t, -1);
      gdk_draw_layout(widget->window, gc, w-80, i+2, Histogram.pl);
    }
  }

  /* draw histogram data */

  gc_color(gc,0xff0000);

  for(x=0.0;x<w;x++) {
    i = (int) (x * (((float)mx)/((float)w)));
    j = (int) ((x+1) * (((float)mx)/((float)w)));

    z = 0.0;
    for(k=i;k<j;k++) {
      z += H[k];
    }
    z /= (float)(j-i+1);
    
    if (m2==1) z = log10(z);
    gdk_draw_line(widget->window, gc, 
		  (int) x, h-1, (int) x,
		  h-1- (int)(z*y));
  }

  gc_color(gc,0);
  gdk_draw_rectangle(widget->window, gc, FALSE, 0,0, w-1, h-1);

  gdk_gc_destroy(gc);
  return FALSE;
}

void ivs_histogram_update(void *obj) {
  gtk_widget_queue_draw(hw);
}

void open_histogram(GtkWidget *parent) {
  GtkWidget *v, *close, *curve; 

  if (hw!=0)
    return;
  Histogram.initialized = 0;
  
  hw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_title(GTK_WINDOW(hw), "Analysis: Histogram");
  gtk_window_set_modal(GTK_WINDOW(hw), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(hw), GTK_WINDOW(parent));
  gtk_container_set_border_width(GTK_CONTAINER(hw),6);
  
  Histogram.pl =  gtk_widget_create_pango_layout(hw, NULL);
  Histogram.pfd = pango_font_description_from_string("Sans 10");
  if (Histogram.pl != NULL && Histogram.pfd != NULL)
    pango_layout_set_font_description(Histogram.pl, Histogram.pfd);
  
  v = guip_create("vbox[gap=2,border=2]("\
		  "curve:drawingarea[w=512,h=200]{gap=2,expand,fill},"\
		  "table[w=2,h=2]{gap=2}("\
		  "label[text='Histogram for:']{l=0,t=0},"\
		  "source:dropbox[options=';Original Image;Current Image']{l=1,t=0},"\
		  "label[text='Scale Mode:']{l=0,t=1},"\
		  "scale:dropbox[options=';Linear;Logarithmic']{l=1,t=1}),"\
		  "hbox[gap=2]{gap=2}(close:button[text=' Close ']{packend}))");
		  
  hsrc   =  (DropBox *) guip_get_ext("source");
  hscale =  (DropBox *) guip_get_ext("scale");
  curve  = guip_get("curve");
  close  = guip_get("close");

  dropbox_set_changed_callback(hsrc, &ivs_histogram_update);
  dropbox_set_changed_callback(hscale, &ivs_histogram_update);

  gtk_container_add(GTK_CONTAINER(hw), v);

  gtk_signal_connect(GTK_OBJECT(close),"clicked",
		     GTK_SIGNAL_FUNC(ivs_histogram_close), 0);
  gtk_signal_connect(GTK_OBJECT(curve),"expose_event",
                     GTK_SIGNAL_FUNC(ivs_histogram_expose), 0);
  gtk_signal_connect(GTK_OBJECT(hw),"delete_event",
                     GTK_SIGNAL_FUNC(ivs_histogram_delete), 0);

  gtk_widget_show(v);
  gtk_widget_show(hw);
}

// =============================================================

Volume * preproc_prepare_preview() {
  int i,N;
  if (!scene->avol) return 0;
  if (preproc.preview) {
    if (preproc.preview->W != scene->avol->W ||
	preproc.preview->H != scene->avol->H ||
	preproc.preview->D != scene->avol->D) {
      VolumeDestroy(preproc.preview);
      preproc.preview = 0;
    }
  }
  if (!preproc.preview)
    preproc.preview = VolumeNew(scene->avol->W, scene->avol->H,
				scene->avol->D, vt_integer_16);
  N = scene->avol->N;
  for(i=0;i<N;i++)
    preproc.preview->u.i16[i] = scene->avol->vd[i].value;

  return(preproc.preview);
}

PreProcFilter * preproc_filter_new() {
  PreProcFilter *pp;

  pp = CREATE(PreProcFilter);
  if (!pp) return 0;

  pp->id            = 0;
  pp->control       = 0;

  pp->activate      = 0;
  pp->deactivate    = cb_default_deactivate;
  pp->repaint       = cb_default_repaint;
  pp->update        = 0;
  pp->updatepreview = 0;
  pp->whatever      = 0;

  pp->data          = 0;

  pp->xslice        = &(preproc.slice[1]);
  pp->yslice        = &(preproc.slice[2]);
  pp->zslice        = &(preproc.slice[0]);

  return pp;
}

void preproc_update_ppdialog() {
  if (preproc.ppdialog)
    if (preproc.lastfilter >= 0)
      if (preproc.ppf[preproc.lastfilter]->update)
	preproc.ppf[preproc.lastfilter]->update(preproc.ppf[preproc.lastfilter]);
}

int preproc_tool_id() {
  if (preproc.lastfilter <0 || preproc.lastfilter >= preproc.nfilters)
    return -1;
  return(preproc.ppf[preproc.lastfilter]->id);
}

void preproc_clip(GdkRectangle *r, int what) {
  switch(what) {
  case 0:
  case 1:
  case 2:
    r->x = preproc.boxlu[what][0];
    r->y = preproc.boxlu[what][1];
    r->width  = preproc.boxsz[what][0];
    r->height = preproc.boxsz[what][1];
    break;
  default:
    r->x = 0;
    r->y = 0;
    r->width = preproc.vW;
    r->height = preproc.vH;
  }
}

#include "preproc_ls.c"
#include "preproc_gs.c"
#include "preproc_mbgs.c"
#include "preproc_thres.c"
#include "preproc_gblur.c"
#include "preproc_modal.c"
#include "preproc_median.c"
#include "preproc_mgrad.c"
#include "preproc_dgrad.c"
#include "preproc_sgrad.c"
#include "preproc_interp.c"
#include "preproc_crop.c"
#include "preproc_plesion.c"
#include "preproc_ssr.c"
#include "preproc_reg.c"
#include "preproc_homog.c"
#include "preproc_morph.c"
#include "preproc_arith.c"

