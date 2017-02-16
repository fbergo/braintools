
#ifndef LABELS_H
#define LABELS_H 1

#include <gtk/gtk.h>
#include <libip.h>

#define MAXLABELS 12

typedef struct _labels {
  int  count;
  char names[MAXLABELS][128];
  int  colors[256]; 
  int  visible[MAXLABELS];
  GtkWidget *widget;
  CImg *img;
  int current;
} LabelControl;

void      labels_init();
gboolean  labels_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  labels_configure(GtkWidget *widget,GdkEventConfigure *ec,gpointer data);
void      labels_draw();
gboolean  labels_press(GtkWidget *widget,GdkEventButton *eb,gpointer data);
gboolean  labels_release(GtkWidget *widget,GdkEventButton *eb,gpointer data);
void      labels_edit(int i);

void      labels_edit_ok(GtkButton *w, gpointer data);
void      labels_edit_cancel(GtkButton *w, gpointer data);
void      labels_edit_color(GtkButton *w, gpointer data);
void      labels_edit_kill(GtkButton *w, gpointer data);

void      labels_ensure_count(int count);
void      labels_new_object();
int       labels_new_named_object(const char *name, int color);

void      labels_changed();

void label_save(const char *name);
void label_load(const char *name);

#endif
