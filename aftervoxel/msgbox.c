
#define SOURCE_MSGBOX_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "aftervoxel.h"

#include "xpm/info.xpm"
#include "xpm/exclam.xpm"
#include "xpm/bong.xpm"
#include "xpm/question.xpm"

void MsgOk(GtkWidget *w, gpointer data) {
  GtkWidget *dlg = (GtkWidget *) data;
  gtk_grab_remove(dlg);
  gtk_widget_destroy(dlg);
}

void PopMessageBox(GtkWidget *parent,char *title, char *text, int icon) {
  GtkWidget *m, *v, *h, *pm, *label, *hs, *h2, *ok;
  GdkPixmap *dp;
  GdkBitmap *dm;
  GtkStyle *style;
  char **xpm[4] = { bong_xpm, exclam_xpm, info_xpm, question_xpm };

  m=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  if (parent != 0)
    gtk_window_set_transient_for(GTK_WINDOW(m), GTK_WINDOW(parent));

  gtk_window_set_position(GTK_WINDOW(m),GTK_WIN_POS_CENTER);
  gtk_widget_realize(m);
  gtk_window_set_policy(GTK_WINDOW(m),TRUE,TRUE,TRUE);
  if (title)
    gtk_window_set_title (GTK_WINDOW(m), title);
  else
    gtk_window_set_title (GTK_WINDOW(m), "<Untitled Message Box>");

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(m), v);

  h=gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(v),h, TRUE,TRUE,2);

  style=gtk_widget_get_style(m);  
  dp=gdk_pixmap_create_from_xpm_d(m->window, &dm,
				    &(style->bg[GTK_STATE_NORMAL]),
				    (gchar **) xpm[icon%4]);
  pm=gtk_pixmap_new(dp,dm);
  gtk_box_pack_start(GTK_BOX(h), pm, FALSE, TRUE, 0);

  label = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT); 
  gtk_box_pack_start(GTK_BOX(h), label, FALSE, TRUE, 2);

  hs=gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  h2=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h2), 5);
  gtk_box_pack_start(GTK_BOX(v), h2, FALSE, TRUE, 2);

  ok=gtk_button_new_with_label("Ok");
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(h2),ok,TRUE,TRUE,0);

  gtk_container_set_border_width(GTK_CONTAINER(m),6);
  gtk_widget_grab_default(ok);

  gtk_widget_show(ok);
  gtk_widget_show(h2);
  gtk_widget_show(hs);
  gtk_widget_show(label);
  gtk_widget_show(pm);
  gtk_widget_show(h);
  gtk_widget_show(v);
  gtk_widget_show(m);

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
                     GTK_SIGNAL_FUNC(MsgOk),(gpointer) m);
  gtk_grab_add(m);
}

int PopYesNoBox(GtkWidget *parent, char *title, char *text, int icon) {
  return(PopOptionBox(parent,title,text,"Yes","No",icon));
}

int opboxdone = 0;
int opboxresult = 0;

void MsgOp1(GtkWidget *w, gpointer data) {
  GtkWidget *dlg = (GtkWidget *) data;
  opboxresult = 0;
  gtk_widget_destroy(dlg);
}

void MsgOp2(GtkWidget *w, gpointer data) {
  GtkWidget *dlg = (GtkWidget *) data;
  opboxresult = 1;
  gtk_widget_destroy(dlg);
}

void MsgDead(GtkWidget *w, gpointer data) {
  opboxdone = 1;
}

int  PopOptionBox(GtkWidget *parent, char *title, char *text, char *op1, char *op2, int icon) 
{

  GtkWidget *m, *v, *h, *pm, *label, *hs, *h2, *ok, *cancel;
  GdkPixmap *dp;
  GdkBitmap *dm;
  GtkStyle *style;
  char **xpm[4] = { bong_xpm, exclam_xpm, info_xpm, question_xpm };

  opboxdone = 0;

  m=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  if (parent != 0)
    gtk_window_set_transient_for(GTK_WINDOW(m), GTK_WINDOW(parent));
  gtk_window_set_modal(GTK_WINDOW(m), TRUE);

  gtk_window_set_position(GTK_WINDOW(m),GTK_WIN_POS_CENTER);
  gtk_widget_realize(m);
  gtk_window_set_policy(GTK_WINDOW(m),TRUE,TRUE,TRUE);
  if (title)
    gtk_window_set_title (GTK_WINDOW(m), title);
  else
    gtk_window_set_title (GTK_WINDOW(m), "<Untitled Message Box>");

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(m), v);

  h=gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(v),h, TRUE,TRUE,2);

  style=gtk_widget_get_style(m);  
  dp=gdk_pixmap_create_from_xpm_d(m->window, &dm,
				    &(style->bg[GTK_STATE_NORMAL]),
				    (gchar **) xpm[icon%4]);
  pm=gtk_pixmap_new(dp,dm);
  gtk_box_pack_start(GTK_BOX(h), pm, FALSE, TRUE, 0);

  label = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT); 
  gtk_box_pack_start(GTK_BOX(h), label, FALSE, TRUE, 2);

  hs=gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  h2=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h2), 5);
  gtk_box_pack_start(GTK_BOX(v), h2, FALSE, TRUE, 2);

  ok=gtk_button_new_with_label(op1);
  cancel=gtk_button_new_with_label(op2);
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(h2),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(h2),cancel,TRUE,TRUE,0);

  gtk_container_set_border_width(GTK_CONTAINER(m),6);
  gtk_widget_grab_default(ok);

  gtk_widget_show(ok);
  gtk_widget_show(cancel);
  gtk_widget_show(h2);
  gtk_widget_show(hs);
  gtk_widget_show(label);
  gtk_widget_show(pm);
  gtk_widget_show(h);
  gtk_widget_show(v);
  gtk_widget_show(m);

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
                     GTK_SIGNAL_FUNC(MsgOp1),(gpointer) m);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(MsgOp2),(gpointer) m);
  gtk_signal_connect(GTK_OBJECT(m),"destroy",
		     GTK_SIGNAL_FUNC(MsgDead),0);

  while(opboxdone==0) {
    if (gtk_events_pending())
      gtk_main_iteration();
  }
  return(opboxresult);
}
