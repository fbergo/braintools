
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
#include "volumetry.h"
#include "util.h"

extern Scene     *scene;
extern GtkWidget *mainwindow;

GtkWidget *vv_dlg;
GtkWidget *vv_mark[10];
GtkWidget *vv_label[11][3];

int vv_N[10];
int marked[10] = {0,0,0,0,0,0,0,0,0,0};

void volumetry_ok(GtkWidget *w, gpointer data);
void volumetry_calc();
void volumetry_calc_mark();
void volumetry_modify_mark(GtkToggleButton *w, gpointer data);

void ivs_volumetry_open() {
  
  int i;
  GtkWidget *dlg,*v,*t,*hs,*h,*ok;
  GtkWidget *tmp[10], *aux;
  ColorTag *tag;
  char z[16];

  vv_dlg = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), "Analysis: Volumetry");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);
  
  v=gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(dlg),v);

  t = gtk_table_new(14, 6, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(t), 8);
  gtk_table_set_row_spacings(GTK_TABLE(t), 2);
  gtk_box_pack_start(GTK_BOX(v), t, TRUE, TRUE, 2);

  // ----------------------
#define IVS_ATTACH(a,b,c,d,e,f) gtk_table_attach_defaults(GTK_TABLE(a),b,c,d,e,f)

  tmp[0] = gtk_label_new("Mark");
  tmp[1] = gtk_label_new("Object");
  tmp[2] = gtk_label_new("Volume (u^3)");
  tmp[3] = gtk_label_new("Voxels");
  tmp[4] = gtk_label_new("% of Total");
  tmp[5] = gtk_hseparator_new();

  IVS_ATTACH(t,tmp[0],0,1,0,1);
  IVS_ATTACH(t,tmp[1],1,3,0,1);
  IVS_ATTACH(t,tmp[2],3,4,0,1);
  IVS_ATTACH(t,tmp[3],4,5,0,1);
  IVS_ATTACH(t,tmp[4],5,6,0,1);
  IVS_ATTACH(t,tmp[5],0,6,1,2);

  for(i=0;i<10;i++) {
    sprintf(z,"#%d",i);
    if (!i) strcpy(z,"BG");

    vv_mark[i] = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vv_mark[i]),
				 marked[i]?TRUE:FALSE);

    gtk_signal_connect(GTK_OBJECT(vv_mark[i]), "toggled",
		       GTK_SIGNAL_FUNC(volumetry_modify_mark), 0);

    aux = gtk_label_new(z);
    tag = colortag_new(scene->colors.label[i],24,12);
    vv_label[i][0] = gtk_label_new("XX");
    vv_label[i][1] = gtk_label_new("YY");
    vv_label[i][2] = gtk_label_new("ZZ");

    IVS_ATTACH(t,vv_mark[i],    0,1,i+2,i+3);
    IVS_ATTACH(t,aux,           1,2,i+2,i+3);
    IVS_ATTACH(t,tag->widget,   2,3,i+2,i+3);
    IVS_ATTACH(t,vv_label[i][0],3,4,i+2,i+3);
    IVS_ATTACH(t,vv_label[i][1],4,5,i+2,i+3);
    IVS_ATTACH(t,vv_label[i][2],5,6,i+2,i+3);

    gtk_widget_show(aux);
    gtk_widget_show(vv_mark[i]);
    gtk_widget_show(vv_label[i][0]);
    gtk_widget_show(vv_label[i][1]);
    gtk_widget_show(vv_label[i][2]);

  }

  tmp[7] = gtk_hseparator_new();
  tmp[6] = gtk_label_new("Marked Objects");
  vv_label[10][0] = gtk_label_new("XX");
  vv_label[10][1] = gtk_label_new("YY");
  vv_label[10][2] = gtk_label_new("ZZ");
  
  IVS_ATTACH(t,tmp[7],0,6,12,13);
  IVS_ATTACH(t,tmp[6],0,3,13,14);
  IVS_ATTACH(t,vv_label[10][0],3,4,13,14);
  IVS_ATTACH(t,vv_label[10][1],4,5,13,14);
  IVS_ATTACH(t,vv_label[10][2],5,6,13,14);

  gtk_widget_show(vv_label[10][0]);
  gtk_widget_show(vv_label[10][1]);
  gtk_widget_show(vv_label[10][2]);

  for(i=0;i<8;i++)
    gtk_widget_show(tmp[i]);

#undef IVS_ATTACH
  // ----------------------

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 8);

  h=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h), 5);
  gtk_box_pack_start(GTK_BOX(v),h,FALSE,TRUE,2);
  
  ok = gtk_button_new_with_label("Close");

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(h), ok, TRUE, TRUE, 0);

  gtk_widget_show(ok);
  gtk_widget_show(h);
  gtk_widget_show(hs);
  gtk_widget_show(t);
  gtk_widget_show(v);

  volumetry_calc();

  gtk_widget_show(dlg);

  gtk_widget_grab_default(ok);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(volumetry_ok), 0);

}

void volumetry_ok(GtkWidget *w, gpointer data) {
  int i;
  for(i=0;i<10;i++)
    marked[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vv_mark[i]))?
      1:0;
  gtk_widget_destroy(vv_dlg);
}

void volumetry_calc() {
  int i;
  float v1,v2;
  char z[64];
  for(i=0;i<10;i++) vv_N[i] = 0;

  for(i=0;i<scene->avol->N;i++)
    ++vv_N[ (scene->avol->vd[i].label & LABELMASK) % 10 ];

  v1 = scene->avol->dx * scene->avol->dy * scene->avol->dz;
  for(i=0;i<10;i++) {
    sprintf(z,"%d",vv_N[i]);
    gtk_label_set_text(GTK_LABEL(vv_label[i][1]),z);

    v2 = v1 * ((float)(vv_N[i]));
    sprintf(z,"%.4f",v2);
    gtk_label_set_text(GTK_LABEL(vv_label[i][0]),z);

    v2 = 100.0 * ((float)(vv_N[i])) / ((float)(scene->avol->N));
    sprintf(z,"%.2f%%",v2);
    gtk_label_set_text(GTK_LABEL(vv_label[i][2]),z);
  }
  
  volumetry_calc_mark();
}

void volumetry_calc_mark() {
  int i,mN;
  float v1,v2;
  char z[64];

  v1 = scene->avol->dx * scene->avol->dy * scene->avol->dz;
  mN = 0;
  for(i=0;i<10;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vv_mark[i])))
      mN += vv_N[i];

  sprintf(z,"%d",mN);
  gtk_label_set_text(GTK_LABEL(vv_label[10][1]),z);

  v2 = v1 * ((float)(mN));
  sprintf(z,"%.4f",v2);
  gtk_label_set_text(GTK_LABEL(vv_label[10][0]),z);

  v2 = 100.0 * ((float)(mN)) / ((float)(scene->avol->N));
  sprintf(z,"%.2f%%",v2);
  gtk_label_set_text(GTK_LABEL(vv_label[10][2]),z);
}

void volumetry_modify_mark(GtkToggleButton *w, gpointer data) {
  volumetry_calc_mark();
  refresh(vv_dlg);
}

GtkWidget *evsdlg, *vse[3];

void evs_ok(GtkWidget *w, gpointer data) {
  float dx,dy,dz;
  dx = atof(gtk_entry_get_text(GTK_ENTRY(vse[0])));
  dy = atof(gtk_entry_get_text(GTK_ENTRY(vse[1])));
  dz = atof(gtk_entry_get_text(GTK_ENTRY(vse[2])));
  scene->avol->dx = dx;
  scene->avol->dy = dy;
  scene->avol->dz = dz;
  gtk_widget_destroy(evsdlg);
}

void evs_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(evsdlg);
}

void ivs_voxel_size_edit() {
  GtkWidget *dlg, *v, *t, *l[3], *hs, *hb, *ok, *cancel;
  char z[32];

  evsdlg = dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), "Edit Voxel Size");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);

  v = gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(dlg), v);

  t = gtk_table_new(3,2,FALSE);
  gtk_box_pack_start(GTK_BOX(v), t, FALSE, TRUE, 0);

  l[0] = gtk_label_new("Voxel X size (mm):");
  l[1] = gtk_label_new("Voxel Y size (mm):");
  l[2] = gtk_label_new("Voxel Z size (mm):");

  vse[0] = gtk_entry_new();
  vse[1] = gtk_entry_new();
  vse[2] = gtk_entry_new();

  gtk_table_attach_defaults(GTK_TABLE(t), l[0], 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(t), vse[0], 1,2, 0,1);

  gtk_table_attach_defaults(GTK_TABLE(t), l[1], 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(t), vse[1], 1,2, 1,2);

  gtk_table_attach_defaults(GTK_TABLE(t), l[2], 0,1, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(t), vse[2], 1,2, 2,3);

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 0);

  hb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
  gtk_box_pack_start(GTK_BOX(v),hb,FALSE,TRUE,2);
  
  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hb), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb), cancel, TRUE, TRUE, 0);

  gtk_widget_show(ok);
  gtk_widget_show(cancel);
  gtk_widget_show(hb);
  gtk_widget_show(hs);
  gtk_widget_show(vse[0]);
  gtk_widget_show(vse[1]);
  gtk_widget_show(vse[2]);
  gtk_widget_show(l[0]);
  gtk_widget_show(l[1]);
  gtk_widget_show(l[2]);
  gtk_widget_show(t);
  gtk_widget_show(v);
  gtk_widget_show(dlg);

  sprintf(z,"%f",scene->avol->dx); gtk_entry_set_text(GTK_ENTRY(vse[0]), z);
  sprintf(z,"%f",scene->avol->dy); gtk_entry_set_text(GTK_ENTRY(vse[1]), z);
  sprintf(z,"%f",scene->avol->dz); gtk_entry_set_text(GTK_ENTRY(vse[2]), z);

  gtk_widget_grab_default(ok);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(evs_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(evs_cancel), 0);
}
