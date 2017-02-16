
#ifndef VLX_TASKLOG_H
#define VLX_TASKLOG_H 1

#include <gtk/gtk.h>
#include <libip.h>

GtkWidget * create_tasklog_pane(TaskLog *log);
void        update_tasklog_pane();

#endif
