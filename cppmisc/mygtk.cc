
#pragma GCC diagnostic ignored "-Wparentheses"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mygtk.h"

#define CREATE(t)  ((t *)malloc(sizeof(t)))
#define DESTROY(a) free(a)

void dropbox_select(GtkWidget *w,gpointer data);

void dropbox_destroy(DropBox *d) {
  if (d!=NULL)
    DESTROY(d);
}

DropBox * dropbox_new(char *s) {
  DropBox *d;
  GtkMenu *m;
  char sep[2], *p, *cp;

  d = CREATE(DropBox);
  if (!d) return 0;
  d->no = 0;
  d->value = 0;
  d->callback = 0;

  m = GTK_MENU(gtk_menu_new());

  cp = (char *) malloc(strlen(s) + 1);
  if (!cp) { free(d); return 0; }
  strcpy(cp, s);

  sep[0] = s[0]; sep[1] = 0;
  p = strtok(&cp[1],sep);
  while(p) {
    d->o[d->no] = gtk_menu_item_new_with_label(p);
    gtk_menu_append(m,d->o[d->no]);
    gtk_signal_connect(GTK_OBJECT(d->o[d->no]), "activate",
                       GTK_SIGNAL_FUNC(dropbox_select),
                       (gpointer) d);
    gtk_widget_show(d->o[d->no]);
    ++(d->no);
    p=strtok(0,sep);
  }

  gtk_widget_show(GTK_WIDGET(m));

  d->widget = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(d->widget),GTK_WIDGET(m));
  gtk_widget_show(d->widget);
  return d;
}

int dropbox_get_selection(DropBox *d) {
  return(d->value);
}

void dropbox_set_selection(DropBox *d, int i) {
  gtk_option_menu_set_history(GTK_OPTION_MENU(d->widget), i);
  d->value = i;
}

void dropbox_set_changed_callback(DropBox *d, void (*callback)(void*)) {
  d->callback = callback;
}

void dropbox_select(GtkWidget *w,gpointer data) {
  DropBox *me = (DropBox *) data;
  int i;

  for(i=0;i<me->no;i++) {
    if (me->o[i] == w) {
      me->value = i;
      if (me->callback) me->callback((void *)me);
      break;
    }
  }
}


void set_icon(GtkWidget *widget, const char **xpm, char *text) {
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkStyle *style;
  style=gtk_widget_get_style(widget);
  icon = gdk_pixmap_create_from_xpm_d (widget->window, &mask,
                                       &style->bg[GTK_STATE_NORMAL],
                                       (gchar **) xpm);
  gdk_window_set_icon (widget->window, NULL, icon, mask);
  gdk_window_set_icon_name(widget->window,(gchar *)text);
}

GtkWidget *xpm_image(GtkWidget *widget, const char **xpm) {
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkWidget *img;
  GtkStyle *style;
  style=gtk_widget_get_style(widget);
  icon = gdk_pixmap_create_from_xpm_d (widget->window, &mask,
                                       &style->bg[GTK_STATE_NORMAL],
                                       (gchar **) xpm);
  img = gtk_image_new_from_pixmap(icon, mask);
  return(img);
}

GtkWidget *image_button(GtkWidget *parent, const char **xpm) {
  GtkWidget *b;
  b = gtk_button_new();
  gtk_button_set_image(GTK_BUTTON(b),xpm_image(parent, xpm));
  return b;
}

GtkWidget *image_toggle(GtkWidget *parent, const char **xpm) {
  GtkWidget *b;
  b = gtk_toggle_button_new();
  gtk_button_set_image(GTK_BUTTON(b),xpm_image(parent, xpm));
  return b;
}

struct _fdlgs {
  int done;
  int ok;
  GtkWidget *dlg;
  char *dest;
} fdlg_data;

void fdlg_destroy (GtkWidget * w, gpointer data) {
  gtk_grab_remove(fdlg_data.dlg);
  fdlg_data.done = 1;
}

void fdlg_ok (GtkWidget * w, gpointer data) {
  strcpy(fdlg_data.dest,
         gtk_file_selection_get_filename(GTK_FILE_SELECTION(fdlg_data.dlg)));

  fdlg_data.ok = 1;
  gtk_widget_destroy(fdlg_data.dlg);
}

int  get_filename(GtkWidget *parent, const char *title, char *dest) {
  GtkWidget *fdlg;

  fdlg = gtk_file_selection_new(title);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fdlg)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(fdlg_ok), 0 );
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
                                         (fdlg)->cancel_button),
                             "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             GTK_OBJECT (fdlg));
  gtk_signal_connect (GTK_OBJECT (fdlg), "destroy",
                      GTK_SIGNAL_FUNC(fdlg_destroy), 0 );

  fdlg_data.done = 0;
  fdlg_data.ok   = 0;
  fdlg_data.dlg  = fdlg;
  fdlg_data.dest = dest;

  gtk_widget_show(fdlg);
  gtk_grab_add(fdlg);

  int x;
  while(!fdlg_data.done) {
    x=5;
    while (gtk_events_pending() && x>0) {
      gtk_main_iteration();
      --x;
    }
    usleep(5000);
  }

  return (fdlg_data.ok ? 1 : 0);
}

