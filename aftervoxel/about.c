
#define ABOUT_SOURCE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

gboolean app_start2(gpointer data) {
  app_tdebug(1000);
#ifdef TODEBUG
  printf("about start2 in\n");
#endif

  app_gui();
  app_set_status("Aftervoxel started.",15);

#ifdef TODEBUG
  printf("about start2 iout\n");
#endif
  return FALSE;
}

gboolean app_start1(gpointer data) {
  app_tdebug(1001);
#ifdef TODEBUG
  printf("about start1 in\n");
#endif

  res.bgqueue = TaskQueueNew();
  StartThread(res.bgqueue);

  orient_init();
  dicom_init();
  info_init();
  seg_init();
  labels_init();
  undo_init();
  v2d_init();
  view3d_init();
  app_pop_init();
  view3d_octctl_init(64,64,42);

#ifdef TODEBUG
  printf("about start1 out\n");
#endif
  return FALSE;
}

static struct _about {
  GtkWidget *w;
} about;

void about_destroy(GtkWidget *w, gpointer data) {
  app_tdebug(1004);
  about.w = 0;
}

gboolean about_delete(GtkWidget *w,GdkEvent *e, gpointer data) {
  return TRUE;
}

void about_close(GtkWidget *w, gpointer data) {
  app_tdebug(1005);
  gtk_widget_destroy(about.w);
}

void about_dialog() {
  GtkWidget *v,*hb,*close,*text;
  char z[1024];

  app_tdebug(1006);

  about.w = util_dialog_new("About Aftervoxel",mw,1,&v,&hb,300,150);
  //  util_set_bg(about.w, 0xffffff);

  text = gtk_label_new(" ");
  gtk_box_pack_start(GTK_BOX(v), text, FALSE, TRUE, 4);
  
  snprintf(z,1024,
	   "Aftervoxel %s\n"\
	   "(c) 2004-2012 Felipe Bergo <fbergo@gmail.com>\n"\
	   "http://www.liv.ic.unicamp.br/~bergo/aftervoxel\n"\
	   "http://www.bergo.eng.br/aftervoxel\n\n"\
	   "This software is freeware and comes with no warranty.\n",
	   VERSION);
  gtk_label_set_text(GTK_LABEL(text), z);		       

  close  = gtk_button_new_with_label("Close");
  gtk_box_pack_start(GTK_BOX(hb), close, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(about.w), "destroy",
		     GTK_SIGNAL_FUNC(about_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(about.w), "delete_event",
		     GTK_SIGNAL_FUNC(about_delete), 0);
  gtk_signal_connect(GTK_OBJECT(close), "clicked",
		     GTK_SIGNAL_FUNC(about_close), 0);

  gtk_widget_show_all(about.w);
}
