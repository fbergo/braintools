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
#include <time.h>
#include <pthread.h>
#include "tasklog.h"

struct _tasklog_pane {
  GtkWidget *widget, *list;
  TaskLog   *log;
} logpane;

GtkWidget * create_tasklog_pane(TaskLog *log) {
  GtkWidget *sw,*cl;

  logpane.log = log;

  sw = logpane.widget = gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  
  cl = logpane.list = gtk_clist_new(5);
  gtk_clist_set_shadow_type(GTK_CLIST(cl),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(cl),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(cl),0,"Thread ID");
  gtk_clist_set_column_title(GTK_CLIST(cl),1,"Description");
  gtk_clist_set_column_title(GTK_CLIST(cl),2,"Elapsed (secs)");
  gtk_clist_set_column_title(GTK_CLIST(cl),3,"Started");
  gtk_clist_set_column_title(GTK_CLIST(cl),4,"Finished");
  gtk_clist_column_titles_passive(GTK_CLIST(cl));
  gtk_clist_column_titles_show(GTK_CLIST(cl));
  
  gtk_container_add(GTK_CONTAINER(sw),cl);

  gtk_widget_show(cl);
  gtk_widget_show(sw);

  update_tasklog_pane();
  return sw;
}

void update_tasklog_pane() {
  static int lastn = -1;
  char *zp[5];
  char b[128],c[128],d[128],e[16];
  char *z;
  int n;
  TaskLogNode *p;

  n = TaskLogGetCount(logpane.log);

  if (lastn == n) return;

  gtk_clist_freeze(GTK_CLIST(logpane.list));
  gtk_clist_clear(GTK_CLIST(logpane.list));

  TaskLogLock(logpane.log);
  lastn = n = TaskLogGetCount(logpane.log);
  for(p=logpane.log->head;p!=0;p=p->next) {
    sprintf(b,"%.6f",p->task.real_elapsed);
    strcpy(c, ctime(& (p->task.start) ));
    strcpy(d, ctime(& (p->task.finish) ));
    z=strchr(c,'\n'); if (z) *z = 0;
    z=strchr(d,'\n'); if (z) *z = 0;
    sprintf(e,"%d",p->task.tid);

    zp[0] = e;
    zp[1] = p->task.desc;
    zp[2] = b;
    zp[3] = c;
    zp[4] = d;

    gtk_clist_append(GTK_CLIST(logpane.list),zp);
  }
  TaskLogUnlock(logpane.log);
  
  gtk_clist_thaw(GTK_CLIST(logpane.list));
  gtk_clist_columns_autosize(GTK_CLIST(logpane.list));
  gtk_clist_moveto(GTK_CLIST(logpane.list),n-1,0,0.8,0.0);
}
