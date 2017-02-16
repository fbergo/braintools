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
#include <gtk/gtk.h>
#include <libip.h>
#include "ivs.h"
#include "be.h"

/* cost threshold code */

gboolean  ct_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void      ct_destroy(GtkWidget *w, gpointer data);
void      ct_apply(GtkWidget *w, gpointer data);
void      ct_close(GtkWidget *w, gpointer data);

void      ct_torig(GtkWidget *w, gpointer data);
void      ct_tvalue(GtkWidget *w, gpointer data);

extern Scene     *scene;
extern GtkWidget *mainwindow;
extern int        sync_ppchange;
extern TaskQueue *qseg;

struct _nameless {
  GtkWidget *dlg;
  GtkAdjustment *cmin, *cmax;  
} ct = {0,0,0};

void ivs_ct_open() {
  GtkWidget *dlg, *v, *h1, *h2, *h3, *l1, *l2, *hs, *apply, *close, *t1, *t2;
  GtkWidget *s1, *s2;

  if (ct.dlg != 0)
    return;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), "Analysis: Cost Threshold");
  gtk_window_set_modal(GTK_WINDOW(dlg), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);

  v = gtk_vbox_new(FALSE,2);
  h1 = gtk_hbox_new(FALSE,2);

  l1 = gtk_label_new("Lower Cost Threshold: ");
  l2 = gtk_label_new("Upper Cost Threshold: ");

  ct.cmin = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 32767.0, 1.0, 10.0, 0.0));
  ct.cmax = GTK_ADJUSTMENT(gtk_adjustment_new(32767.0, 0.0, 32767.0, 1.0, 10.0, 0.0));
  
  s1 = gtk_spin_button_new(ct.cmin, 1.5, 0);
  s2 = gtk_spin_button_new(ct.cmax, 1.5, 0);

  gtk_container_add(GTK_CONTAINER(dlg), v);
  gtk_box_pack_start(GTK_BOX(v), h1, FALSE, TRUE, 4);

  gtk_box_pack_start(GTK_BOX(h1), l1, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h1), s1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h1), l2, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h1), s2, TRUE, TRUE, 0);

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 4);

  h2 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h2), 5);

  h3 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h3), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h3), 5);

  apply = gtk_button_new_with_label("Make Projection");
  t1 = gtk_button_new_with_label("Threshold Original");
  t2 = gtk_button_new_with_label("Threshold Current");
  close = gtk_button_new_with_label("Close");

  gtk_box_pack_start(GTK_BOX(v), h2, FALSE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(v), h3, FALSE, TRUE, 4);

  gtk_box_pack_start(GTK_BOX(h2), apply, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h2), t1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h2), t2, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(h3), close, TRUE, TRUE, 0);

  gtk_widget_show(apply);
  gtk_widget_show(t1);
  gtk_widget_show(t2);
  gtk_widget_show(close);
  gtk_widget_show(h1);
  gtk_widget_show(h2);
  gtk_widget_show(h3);
  gtk_widget_show(hs);
  gtk_widget_show(s1);
  gtk_widget_show(s2);
  gtk_widget_show(l1);
  gtk_widget_show(l2);
  gtk_widget_show(v);
  gtk_widget_show(dlg);

  gtk_signal_connect(GTK_OBJECT(dlg),"delete_event",
                     GTK_SIGNAL_FUNC(ct_delete), 0);
  gtk_signal_connect(GTK_OBJECT(dlg),"destroy",
                     GTK_SIGNAL_FUNC(ct_destroy), 0);

  gtk_signal_connect(GTK_OBJECT(apply),"clicked",
                     GTK_SIGNAL_FUNC(ct_apply), 0);
  gtk_signal_connect(GTK_OBJECT(close),"clicked",
                     GTK_SIGNAL_FUNC(ct_close), 0);

  gtk_signal_connect(GTK_OBJECT(t1),"clicked",
                     GTK_SIGNAL_FUNC(ct_torig), 0);
  gtk_signal_connect(GTK_OBJECT(t2),"clicked",
                     GTK_SIGNAL_FUNC(ct_tvalue), 0);

  ct.dlg = dlg;
}

void ct_apply(GtkWidget *w, gpointer data) {
  int a,b;

  a = ct.cmin->value;
  b = ct.cmax->value;
  if (b<a) b=a;

  scene->with_restriction = 1;
  scene->cost_threshold_min = a;
  scene->cost_threshold_max = b;
  scene->invalid = 1;
  ivs_rot(0,0);
}

gboolean  ct_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  return FALSE;
}

void      ct_destroy(GtkWidget *w, gpointer data) {
  scene->with_restriction = 0;
  scene->invalid = 1;
  ivs_rot(0,0);
  ct.dlg = 0;
}

void ct_close(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(ct.dlg);
}

void      ct_torig(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args = ArgBlockNew();
  args->scene  = scene;
  args->ack    = &sync_ppchange;
  args->ia[0]  = (int) (ct.cmin->value);
  args->ia[1]  = (int) (ct.cmax->value);
  args->ia[2]  = 0;
  args->ia[3]  = 0; /* no gui for inversion yet */
  TaskNew(qseg, "Cost Threshold", 
	  (Callback) &be_cost_threshold, (void *) args);
}

void      ct_tvalue(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args = ArgBlockNew();
  args->scene  = scene;
  args->ack    = &sync_ppchange;
  args->ia[0]  = (int) (ct.cmin->value);
  args->ia[1]  = (int) (ct.cmax->value);
  args->ia[2]  = 1;
  args->ia[3]  = 0; /* no gui for inversion yet */
  TaskNew(qseg, "Cost Threshold", 
	  (Callback) &be_cost_threshold, (void *) args);
}
