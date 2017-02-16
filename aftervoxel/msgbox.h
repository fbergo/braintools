
#ifndef MSGBOX_H
#define MSGBOX_H 1

#define MSG_ICON_ERROR 0
#define MSG_ICON_WARN  1
#define MSG_ICON_INFO  2
#define MSG_ICON_ASK   3

#define MSG_YES 0
#define MSG_NO  1

#include <gtk/gtk.h>

void PopMessageBox(GtkWidget *parent,char *title, char *text, int icon);
int  PopOptionBox(GtkWidget *parent, char *title, char *text, char *op1, char *op2, int icon);
int  PopYesNoBox(GtkWidget *parent, char *title, char *text, int icon);

#endif
