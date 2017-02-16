
#ifndef ABOUT_H
#define ABOUT_H 1

#include <gtk/gtk.h>

/* public */
gboolean app_start1(gpointer data);
gboolean app_start2(gpointer data);

void     about_dialog();

/* private */
void     about_destroy(GtkWidget *w, gpointer data);
gboolean about_delete(GtkWidget *w,GdkEvent *e, gpointer data);



#endif
