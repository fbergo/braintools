/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2010
   
   Felipe Paulo Guazzi Bergo 
   <fbergo@gmail.com>

   and
   
   Alexandre Xavier Falcao
   <afalcao@ic.unicamp.br>

   Distribution, trade, publication or bulk reproduction
   of this source code or any derivative works are
   strictly forbidden. Source code is provided to allow
   compilation on different architectures.

   ----------------------------------------------------- */

#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "be.h"
#include "util.h"
#include "fuzzy.h"
#include "ivs.h"

struct _fuzzypar_dlg {
  GtkWidget *mean[10], *stddev[10], *label[10];
  GtkAdjustment *n;
  GtkAdjustment *slice;
  GtkWidget *axis[3], *canvas, *cbzoom, *cbcolorize;
  GtkWidget *dlg;
  CImg *preview, *preview2x;
  int   msz, maxval;
  int   toid;
  int   oksegments;
  int   zoom, colorize;
} FPDlg;

extern fob        FuzzyPars;
extern Scene     *scene;
extern GtkWidget *mainwindow;
extern TaskQueue *qseg;
extern int        sync_objmode;
extern int        fuzzyvariant;
extern int        FirstFuzzyDone;

void     ivs_fuzzy_pars_cancel(GtkWidget *w, gpointer data);
void     ivs_fuzzy_pars_ok(GtkWidget *w, gpointer data);
void     ivs_fuzzy_pars_n_changed(GtkAdjustment *a, gpointer data);
gboolean ivs_fuzzy_pars_expose_preview(GtkWidget *widget, 
				       GdkEventExpose *ee,
				       gpointer data);
void     ivs_fuzzy_pars_make_preview();
gboolean ivs_fuzzy_refresh_preview(gpointer data);
void     ivs_fuzzy_delayed_preview();
void     ivs_fuzzy_pars_zoom(GtkToggleButton *widget, gpointer data);
void     ivs_fuzzy_pars_colorize(GtkToggleButton *widget, gpointer data);

void     ivs_fuzzy_pars_edited(GtkEditable *w, gpointer data);
void     ivs_fuzzy_pars_toggled(GtkToggleButton *w, gpointer data);
void     ivs_fuzzy_pars_adj_changed(GtkAdjustment *w, gpointer data);

void ivs_fuzzy_pars_cancel(GtkWidget *w, gpointer data) {
  GtkWidget *d = (GtkWidget *) data;
  gtk_widget_destroy(d);
  CImgDestroy(FPDlg.preview);
  CImgDestroy(FPDlg.preview2x);
}

void ivs_do_fuzzy() {
  ArgBlock *args;
  char z[256];
  int i;

  args = ArgBlockNew();
  args->scene  = scene;
  args->ack    = &sync_objmode;

  switch(fuzzyvariant) {
  case 0:
    strcpy(z,"DIFT/Fuzzy");
    args->ia[0]  = SEG_OP_FUZZY; break;
  case 1:
    strcpy(z,"DIFT/Strict Fuzzy");
    args->ia[0]  = SEG_OP_STRICT_FUZZY; break;
  default:
    fprintf(stderr,"something very wrong happened.\n");
    exit(5);
  }

  args->ia[1]  = FuzzyPars.n;
  for(i=0;i<10;i++) {
    args->fa[2*i]   = FuzzyPars.mean[i];
    args->fa[2*i+1] = FuzzyPars.stddev[i];
  }

  FirstFuzzyDone = 1;
  TaskNew(qseg, z, (Callback) &be_segment, (void *) args);
}

void ivs_fuzzy_pars_ok(GtkWidget *w, gpointer data) {
  GtkWidget *d = (GtkWidget *) data;
  int i;

  FuzzyPars.n = FPDlg.n->value;

  for(i=0;i<FuzzyPars.n;i++) {
    FuzzyPars.mean[i]=atof(gtk_entry_get_text(GTK_ENTRY(FPDlg.mean[i])));
    FuzzyPars.stddev[i]=atof(gtk_entry_get_text(GTK_ENTRY(FPDlg.stddev[i])));
  }
  gtk_widget_destroy(d);
  CImgDestroy(FPDlg.preview);
  CImgDestroy(FPDlg.preview2x);
  if (FPDlg.oksegments)
    ivs_do_fuzzy();
}

void ivs_fuzzy_pars_n_changed(GtkAdjustment *a, gpointer data) {
  int i,n;
  n = (int) (FPDlg.n->value);
  for(i=0;i<10;i++)
    if (n>i) {
      gtk_widget_show(FPDlg.mean[i]);
      gtk_widget_show(FPDlg.stddev[i]);
    } else {
      gtk_widget_hide(FPDlg.mean[i]);
      gtk_widget_hide(FPDlg.stddev[i]);
    }
}

gboolean ivs_fuzzy_pars_expose_preview(GtkWidget *widget, 
				       GdkEventExpose *ee,
				       gpointer data)
{
  if (FPDlg.zoom == 1) {
    gdk_draw_rgb_image(widget->window,
		       widget->style->black_gc,0,0,
		       FPDlg.msz, FPDlg.msz,
		       GDK_RGB_DITHER_NORMAL,
		       FPDlg.preview->val,
		       FPDlg.preview->rowlen);
  } else {
    gdk_draw_rgb_image(widget->window,
		       widget->style->black_gc,0,0,
		       FPDlg.msz * 2, FPDlg.msz * 2,
		       GDK_RGB_DITHER_NORMAL,
		       FPDlg.preview2x->val,
		       FPDlg.preview2x->rowlen);
  }
  return FALSE;
}

void ivs_fuzzy_pars_make_preview() {
  int i,j,k,axis=2;
  int *tmp;
  char *pert;
  XAnnVolume *avol = scene->avol;
  int 
    W = scene->avol->W,
    H = scene->avol->H,
    D = scene->avol->D;
  int best, bestk, c, riw;  

  int   nobjs;
  float mean[10], stddev[10];

  int colors[10] = {0xffff00, 0xff0000, 0x0000ff, 0x800080, 0xff8000,
		    0x800000, 0x008000, 0xff00ff, 0x000080, 0x80ff00};

  ViewMap *vmap;

  /* get dialog pars */

  nobjs  = FPDlg.n->value;
  for(i=0;i<nobjs;i++) {
    mean[i]=atof(gtk_entry_get_text(GTK_ENTRY(FPDlg.mean[i])));
    stddev[i]=atof(gtk_entry_get_text(GTK_ENTRY(FPDlg.stddev[i])));
  }

  /* get selected scene slice */

  for(i=0;i<3;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FPDlg.axis[i])))
      { axis = i; break; }

  k = FPDlg.slice->value;

  riw = FPDlg.msz;

  tmp = (int *) malloc(riw * riw * sizeof(int));
  MemSet(tmp, 0, riw * riw * sizeof(int));

  pert = (char *) malloc(riw * riw);
  MemSet(pert, 0, riw * riw);

  switch(axis) {
  case 0: // X
    if (k >= W) k = W-1;
    for(j=0;j<H;j++)
      for(i=0;i<D;i++)
	tmp[riw * j + i] = XAGetValue(avol, 
				     k + avol->tbrow[j] + avol->tbframe[i]);
    break;
  case 1: // Y
    if (k >= H) k = H-1;
    for(j=0;j<W;j++)
      for(i=0;i<D;i++)
	tmp[riw * j + i] = XAGetValue(avol, 
				     j + avol->tbrow[k] + avol->tbframe[i]);
    break;
  case 2:
    if (k >= D) k = D-1;
    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	tmp[riw * j + i] = XAGetValue(avol, 
				     i + avol->tbrow[j] + avol->tbframe[k]);
    break;
  default:
    return; // never
  }

  /* paint slice bg */

  vmap = ViewMapNew();
  ViewMapCopy(vmap, scene->vmap);
  vmap->maxval = FPDlg.maxval;
  ViewMapUpdate(vmap);

  for(i=0;i<riw;i++)
    for(j=0;j<riw;j++)
      CImgSet(FPDlg.preview, j,i, ViewMapColor(vmap, tmp[riw * i + j]));

  /* calculate pertinence grid */

  for(i=0;i<riw;i++)
    for(j=0;j<riw;j++) {
      best  = 35000;
      bestk = 0;
      for(k=0;k<nobjs;k++) {
	c = FuzzyCost(tmp[riw * i + j], mean[k], stddev[k]*stddev[k], 32000);
	if (c < best) { 
	  best = c; 
	  bestk = k;
	}
      }
      pert[riw * i + j] = (char) bestk;
    }

  if (FPDlg.colorize) {
    for(i=0;i<riw;i++)
      for(j=0;j<riw;j++)
	CImgSet(FPDlg.preview, j,i,
		mergeColorsRGB(colors[(int)(pert[riw*i+j])],
			       CImgGet(FPDlg.preview,j,i),0.66));
  } else {
    for(i=1;i<riw-1;i++)
      for(j=1;j<riw-1;j++) {
	if (
	    (pert[(riw * (i)) + j] ^ pert[(riw * (i-1)) + j]) ||
	    (pert[(riw * (i)) + j] ^ pert[(riw * (i+1)) + j]) ||
	    (pert[(riw * (i)) + j] ^ pert[(riw * (i)) + j - 1]) ||
	    (pert[(riw * (i)) + j] ^ pert[(riw * (i)) + j + 1])
	    )
	  CImgSet(FPDlg.preview, j, i, 0x00ff00);
      }
  }

  ViewMapDestroy(vmap);
  free(pert);
  free(tmp);

  if (FPDlg.preview2x)
    CImgDestroy(FPDlg.preview2x);
  FPDlg.preview2x = CImgIntegerZoom(FPDlg.preview, 2);
}

gboolean ivs_fuzzy_refresh_preview(gpointer data) {
  ivs_fuzzy_pars_make_preview();
  refresh(FPDlg.canvas);
  FPDlg.toid = -1;  
  return FALSE;
}

void ivs_fuzzy_pars_colorize(GtkToggleButton *widget, gpointer data) {
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FPDlg.cbcolorize)))
    FPDlg.colorize = 1;
  else
    FPDlg.colorize = 0;
  ivs_fuzzy_delayed_preview();
}

void ivs_fuzzy_pars_zoom(GtkToggleButton *widget, gpointer data) {

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(FPDlg.cbzoom)))
    FPDlg.zoom = 2;
  else
    FPDlg.zoom = 1;

  switch(FPDlg.zoom) {
  case 1:
    gtk_drawing_area_size(GTK_DRAWING_AREA(FPDlg.canvas), 
			  FPDlg.msz, FPDlg.msz);
    refresh(FPDlg.canvas);
    refresh(FPDlg.dlg);
    break;
  case 2:
    gtk_drawing_area_size(GTK_DRAWING_AREA(FPDlg.canvas), 
			  FPDlg.msz * 2, FPDlg.msz * 2);
    refresh(FPDlg.canvas);
    refresh(FPDlg.dlg);
    break;
  } 
}

void ivs_fuzzy_delayed_preview() {
  if (FPDlg.toid < 0)
    FPDlg.toid = gtk_timeout_add(250, ivs_fuzzy_refresh_preview, 0);
}

void ivs_fuzzy_pars_edited(GtkEditable *w, gpointer data) {
  ivs_fuzzy_delayed_preview();
}

void ivs_fuzzy_pars_toggled(GtkToggleButton *w, gpointer data) {
  ivs_fuzzy_delayed_preview();
}

void ivs_fuzzy_pars_adj_changed(GtkAdjustment *w, gpointer data) {
  ivs_fuzzy_delayed_preview();
}

void ivs_get_fuzzy_pars(int oksegments) {
  GtkWidget *dlg, *v, *t, *hs, *hb, *ok, *cancel;
  GtkWidget *n, *lb[13];
  GtkWidget *h, *v2, *canvas, *ls, *slide, *rf, *v3;
  int i,msz;
  char z[16];
  GSList *L;

  if (!scene->avol)
    return;
  
  FPDlg.oksegments = oksegments;
  FPDlg.toid     = -1;
  FPDlg.zoom     = 1;  
  FPDlg.colorize = 0;  

  FPDlg.dlg = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), "Fuzzy Connectness Parameters");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);

  h = gtk_hbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(dlg), h);

  /* left pane */

  v=gtk_vbox_new(FALSE,4);
  gtk_box_pack_start(GTK_BOX(h), v, TRUE, TRUE, 0);

  t = gtk_table_new(12, 3, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(t), 4);
  gtk_table_set_row_spacings(GTK_TABLE(t), 4);
  gtk_box_pack_start(GTK_BOX(h), t, TRUE,TRUE, 4);

  for(i=0;i<10;i++) {
    sprintf(z,"Object #%d", i);
    FPDlg.label[i] = lb[i] = gtk_label_new(z);
    gtk_table_attach_defaults(GTK_TABLE(t), lb[i], 0, 1, 2+i, 3+i);
    gtk_widget_show(lb[i]);

    FPDlg.mean[i] = gtk_entry_new();
    sprintf(z,"%.4f",FuzzyPars.mean[i]);
    gtk_entry_set_text(GTK_ENTRY(FPDlg.mean[i]), z);
    gtk_table_attach_defaults(GTK_TABLE(t), FPDlg.mean[i], 1, 2, 2+i, 3+i);
    gtk_widget_show(FPDlg.mean[i]);

    FPDlg.stddev[i] = gtk_entry_new();
    sprintf(z,"%.4f",FuzzyPars.stddev[i]);
    gtk_entry_set_text(GTK_ENTRY(FPDlg.stddev[i]), z);
    gtk_table_attach_defaults(GTK_TABLE(t), FPDlg.stddev[i], 2, 3, 2+i, 3+i);
    gtk_widget_show(FPDlg.stddev[i]);

  }
  lb[10] = gtk_label_new("Number of objects:");
  lb[11] = gtk_label_new("Mean");
  lb[12] = gtk_label_new("Std. Deviation");

  gtk_table_attach_defaults(GTK_TABLE(t), lb[10], 0, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(t), lb[11], 1, 2, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(t), lb[12], 2, 3, 1, 2);
  gtk_widget_show(lb[10]);
  gtk_widget_show(lb[11]);
  gtk_widget_show(lb[12]);
  
  FPDlg.n = (GtkAdjustment *) gtk_adjustment_new(FuzzyPars.n,
						1, 10, 1, 2, 0);
  n = gtk_spin_button_new(FPDlg.n, 1.0, 0);

  gtk_table_attach_defaults(GTK_TABLE(t), n, 2, 3, 0, 1);
  gtk_widget_show(n);  

  gtk_widget_show(t);
  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE,TRUE, 4);

  /* right pane */

  v2 = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(h), v2, TRUE, TRUE, 4);

  msz = MAX(scene->avol->W, scene->avol->H);
  msz = MAX(msz, scene->avol->D);  
  FPDlg.msz = msz;
  FPDlg.preview = CImgNew(msz, msz);
  FPDlg.preview2x = CImgNew(2*msz, 2*msz);
  CImgFill(FPDlg.preview, 0);
  FPDlg.maxval = XAGetValueMax(scene->avol);

  FPDlg.canvas = canvas = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas), msz, msz);
  gtk_box_pack_start(GTK_BOX(v2), canvas, FALSE, TRUE, 0);

  ls = gtk_label_new("Preview Slice");
  gtk_box_pack_start(GTK_BOX(v2), ls, FALSE, TRUE, 0);

  FPDlg.slice = GTK_ADJUSTMENT(gtk_adjustment_new(scene->avol->D / 2,
						  0, msz, 1.0, 5.0, 0.0));
  slide = gtk_hscale_new(FPDlg.slice);
  gtk_box_pack_start(GTK_BOX(v2), slide, FALSE, TRUE,0);

  FPDlg.cbzoom = gtk_check_button_new_with_label("2X zoom");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FPDlg.cbzoom), FALSE);
  gtk_box_pack_start(GTK_BOX(v2), FPDlg.cbzoom, FALSE, TRUE,0);

  FPDlg.cbcolorize = gtk_check_button_new_with_label("Colorize regions / Green borders");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FPDlg.cbcolorize), FALSE);
  gtk_box_pack_start(GTK_BOX(v2), FPDlg.cbcolorize, FALSE, TRUE,0);

  rf = gtk_frame_new("Slice Axis");
  gtk_box_pack_start(GTK_BOX(v2), rf, FALSE, TRUE,0);
  
  FPDlg.axis[0] = gtk_radio_button_new_with_label(0,"X (ZY view)");
  L = gtk_radio_button_group(GTK_RADIO_BUTTON(FPDlg.axis[0]));
  FPDlg.axis[1] = gtk_radio_button_new_with_label(L,"Y (ZX view)");
  L = gtk_radio_button_group(GTK_RADIO_BUTTON(FPDlg.axis[1]));
  FPDlg.axis[2] = gtk_radio_button_new_with_label(L,"Z (XY view)");

  v3 = gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(rf), v3);

  gtk_box_pack_start(GTK_BOX(v3), FPDlg.axis[0], FALSE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(v3), FPDlg.axis[1], FALSE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(v3), FPDlg.axis[2], FALSE, TRUE,0);

  /* bottom */

  hb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
  gtk_box_pack_start(GTK_BOX(v2),hb,FALSE,TRUE,2);
  
  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hb), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb), cancel, TRUE, TRUE, 0);

  gtk_widget_show(hb);
  gtk_widget_show(ok);
  gtk_widget_show(cancel);
  gtk_widget_show(hs);
  gtk_widget_show(v);

  gtk_widget_show(v2);
  gtk_widget_show(canvas);
  gtk_widget_show(ls);
  gtk_widget_show(slide);
  gtk_widget_show(FPDlg.cbzoom);
  gtk_widget_show(FPDlg.cbcolorize);
  gtk_widget_show(rf);
  gtk_widget_show(v3);
  gtk_widget_show(FPDlg.axis[0]);
  gtk_widget_show(FPDlg.axis[1]);
  gtk_widget_show(FPDlg.axis[2]);

  gtk_widget_show(h);
  gtk_widget_show(dlg);

  ivs_fuzzy_pars_n_changed(0,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FPDlg.axis[2]), TRUE);

  ivs_fuzzy_pars_make_preview();

  gtk_widget_grab_default(ok);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_cancel), (gpointer) dlg);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_ok), (gpointer) dlg);
  gtk_signal_connect(GTK_OBJECT(FPDlg.n), "value_changed",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_n_changed),(gpointer) dlg);

  gtk_signal_connect(GTK_OBJECT(FPDlg.canvas), "expose_event",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_expose_preview),0);

  gtk_signal_connect(GTK_OBJECT(FPDlg.cbzoom), "toggled",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_zoom), (gpointer) dlg);
  gtk_signal_connect(GTK_OBJECT(FPDlg.cbcolorize), "toggled",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_colorize), (gpointer) dlg);

  for(i=0;i<10;i++) {
    gtk_signal_connect(GTK_OBJECT(FPDlg.mean[i]), "changed",
		       GTK_SIGNAL_FUNC(ivs_fuzzy_pars_edited),0);
    gtk_signal_connect(GTK_OBJECT(FPDlg.stddev[i]), "changed",
		       GTK_SIGNAL_FUNC(ivs_fuzzy_pars_edited),0);
  }

  for(i=0;i<3;i++)
    gtk_signal_connect(GTK_OBJECT(FPDlg.axis[i]), "toggled",
		       GTK_SIGNAL_FUNC(ivs_fuzzy_pars_toggled),0);

  gtk_signal_connect(GTK_OBJECT(FPDlg.n), "value_changed",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_adj_changed),0);
  gtk_signal_connect(GTK_OBJECT(FPDlg.slice), "value_changed",
		     GTK_SIGNAL_FUNC(ivs_fuzzy_pars_adj_changed),0);
}
