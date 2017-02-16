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
#include "msgbox.h"

#include "xpm/info.xpm"
#include "xpm/exclam.xpm"
#include "xpm/bong.xpm"

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
  char **xpm[3] = { bong_xpm, exclam_xpm, info_xpm };

  m=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  if (parent != 0)
    gtk_window_set_transient_for(GTK_WINDOW(m), GTK_WINDOW(parent));

  gtk_window_set_position(GTK_WINDOW(m),GTK_WIN_POS_CENTER);
  gtk_widget_realize(m);
  gtk_window_set_resizable(GTK_WINDOW(m), FALSE);
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
				    (gchar **) xpm[icon%3]);
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
