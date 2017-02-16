
#ifndef MSGBOX_H
#define MSGBOX_H 1

#define MSG_ICON_ERROR 0
#define MSG_ICON_WARN  1
#define MSG_ICON_INFO  2

#include <gtk/gtk.h>

void PopMessageBox(GtkWidget *parent,char *title, char *text, int icon);

#endif
