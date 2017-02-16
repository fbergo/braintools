
#ifndef MYGTK_H
#define MYGTK_H 1

#include <gtk/gtk.h>

#define gc_color(a,b) gdk_rgb_gc_set_foreground(a,b)
#define redraw(a) gdk_window_invalidate_rect(a->window,NULL,TRUE);

typedef struct _dropbox {
  GtkWidget *widget;
  int        value;
  GtkWidget *o[32];
  int        no;
  void (*callback)(void *);
} DropBox;

/* s[0] = separator, s[1]...s[n] = list of options separated by s[0]. */
DropBox * dropbox_new(char *s);
void      dropbox_destroy(DropBox *d);
int       dropbox_get_selection(DropBox *d);
void      dropbox_set_selection(DropBox *d, int i);
void      dropbox_set_changed_callback(DropBox *d, void (*callback)(void*));

void      set_icon(GtkWidget *widget, const char **xpm, char *text);
int       get_filename(GtkWidget *parent, const char *title, char *dest);

GtkWidget *xpm_image(GtkWidget *widget, const char **xpm);
GtkWidget *image_button(GtkWidget *parent, const char **xpm);
GtkWidget *image_toggle(GtkWidget *parent, const char **xpm);

#endif
