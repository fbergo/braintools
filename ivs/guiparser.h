
#ifndef GUIPARSER_H
#define GUIPARSER_H 1

/* creates GTK+ widget trees from textual descriptions,
   in the long run, much easier than making everything
   from gtk_xxx_create() calls */

#include <gtk/gtk.h>

/* returns the root widget */
GtkWidget * guip_create(const char *desc);

/* gets a reference to a named object in the last-created
   tree */
GtkWidget     * guip_get(const char *name);
GtkAdjustment * guip_get_adj(const char *name);
void          * guip_get_ext(const char *name);



#endif
