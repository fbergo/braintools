
#define SOURCE_UTIL_C 1

#pragma GCC diagnostic ignored "-Wformat-truncation"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include "aftervoxel.h"

gboolean  toolbar_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  toolbar_press(GtkWidget *widget,GdkEventButton *eb,gpointer data);
gboolean  toolbar_enter(GtkWidget *widget,GdkEventCrossing *ec,gpointer data);
gboolean  toolbar_leave(GtkWidget *widget,GdkEventCrossing *ec,gpointer data);
gboolean  toolbar_motion(GtkWidget *widget,GdkEventMotion *em,gpointer data);

gboolean cb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
gboolean cb_press(GtkWidget *w,GdkEventButton *eb,gpointer data);
gboolean cb_release(GtkWidget *w,GdkEventButton *eb,gpointer data);

gboolean lb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
gboolean lb_press(GtkWidget *w,GdkEventButton *eb,gpointer data);
gboolean lb_release(GtkWidget *w,GdkEventButton *eb,gpointer data);

gboolean button_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
gboolean button_press(GtkWidget *w,GdkEventButton *eb,gpointer data);
gboolean button_release(GtkWidget *w,GdkEventButton *eb,gpointer data);

void     dropbox_select(GtkWidget *w,gpointer data);

void gc_color(GdkGC *gc, int color) {
  gdk_rgb_gc_set_foreground(gc, color);
}

void refresh(GtkWidget *w) {
  gtk_widget_queue_resize(w);
}

void BoxAssign(Box *b, int x,int y,int w,int h) {
  b->x = x;
  b->y = y;
  b->w = w;
  b->h = h;
}

int  InBox(int x,int y, Box *b) {
  return(x >= b->x && x < (b->x + b->w) &&
	 y >= b->y && y < (b->y + b->h) );
}

int  InBoxXYWH(int x,int y, int x0,int y0,int w,int h) {
  return(x >= x0 && x < (x0 + w) &&
	 y >= y0 && y < (y0 + h) );
}

Toolbar * ToolbarNew(int nitems,int cols,int w,int h) {
  Toolbar *t;
  int i;

  t = (Toolbar *) MemAlloc(sizeof(Toolbar));
  if (!t) return 0;

  t->bgi = 0xc0c0c0;
  t->bga = 0xc0c0ff;
  t->fgi = 0x000000;
  t->fga = 0xffff00;

  t->n    = nitems;
  t->cols = cols;
  t->rows = (nitems+cols-1)/cols;
  t->w    = w;
  t->h    = h;
  t->tmpcount = 0;
  t->callback = 0;
  t->tips = 0;
  t->tipc = 0;
  t->pl   = 0;
  t->hover = -1;
  t->thide = 0;

  t->img = (Glyph **) MemAlloc(sizeof(Glyph *) * t->n);
  if (!t->img) return 0;
  t->tip = (char **) MemAlloc(sizeof(char *) * t->n);
  if (!t->tip) return 0;

  t->buf = CImgNew(w*cols, h*(t->rows));
  t->selected = 0;

  t->enabled = (int *) MemAlloc(sizeof(int) * t->n);
  for(i=0;i<t->n;i++)
    t->enabled[i] = 1;

  t->widget = gtk_drawing_area_new();
  gtk_widget_set_events(t->widget,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|
			GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(t->widget), 
			t->buf->W, t->buf->H);
  gtk_widget_show(t->widget);

  gtk_signal_connect(GTK_OBJECT(t->widget),"expose_event",
                     GTK_SIGNAL_FUNC(toolbar_expose), (gpointer) t);

  gtk_signal_connect(GTK_OBJECT(t->widget),"button_press_event",
                     GTK_SIGNAL_FUNC(toolbar_press), (gpointer) t);

  gtk_signal_connect(GTK_OBJECT(t->widget),"enter_notify_event",
                     GTK_SIGNAL_FUNC(toolbar_enter), (gpointer) t);
  gtk_signal_connect(GTK_OBJECT(t->widget),"leave_notify_event",
                     GTK_SIGNAL_FUNC(toolbar_leave), (gpointer) t);
  gtk_signal_connect(GTK_OBJECT(t->widget),"motion_notify_event",
                     GTK_SIGNAL_FUNC(toolbar_motion), (gpointer) t);

  return t;
}

void      ToolbarAddButton(Toolbar *t,Glyph *g,const char *tip) {
  t->img[t->tmpcount] = g;
  t->tip[t->tmpcount] = strdup(tip);
  t->enabled[t->tmpcount] = 1;
  t->tmpcount++;
}
void      ToolbarSetSelection(Toolbar *t, int i) {
  if (t->enabled[i]) {
    t->selected = i;
    refresh(t->widget);
  }
}

int       ToolbarGetSelection(Toolbar *t) {
  return(t->selected);
}

void      ToolbarSetCallback(Toolbar *t,  void (*callback)(void *)) {
  t->callback = callback;
}

void      ToolbarDisable(Toolbar *t, int i,int ns) {
  t->enabled[i] = 0;
  if (t->selected == i) {
    t->selected = ns;
    if (t->callback) t->callback((void *)t);
  }
  refresh(t->widget);
}

void      ToolbarEnable(Toolbar *t, int i) {
  t->enabled[i] = 1;
  refresh(t->widget);
}

gboolean  toolbar_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) 
{
  Toolbar *t = (Toolbar *) data;
  int i;

  CImgDrawBevelBox(t->buf,0,0,t->buf->W,t->buf->H,
		   darker(t->bgi,7), 0, 0);

  for(i=0;i<t->tmpcount;i++) {

    if (t->enabled[i]) {
      CImgDrawBevelBox(t->buf,
		       t->w * (i % t->cols),
		       t->h * (i / t->cols),
		       t->w+1, t->h+1,
		       i == t->selected ? t->bga : t->bgi,
		       0, i!=t->selected);

      CImgDrawGlyph(t->buf,t->img[i],
		    t->w * (i % t->cols) + (t->w - t->img[i]->W)/2,
		    t->h * (i / t->cols) + (t->h - t->img[i]->H)/2,
		    t->selected == i ? t->fga : t->fgi);
    } else {

      CImgFillRect(t->buf,
		   t->w * (i % t->cols),
		   t->h * (i / t->cols),
		   t->w+1, t->h+1, darker(t->bgi, 7));
      CImgDrawGlyph(t->buf,t->img[i],
		    t->w * (i % t->cols) + (t->w - t->img[i]->W)/2,
		    t->h * (i / t->cols) + (t->h - t->img[i]->H)/2,
		    darker(t->bgi,3));

    }
  }

  gdk_draw_rgb_image(widget->window,widget->style->black_gc,
		     0,0,t->buf->W,t->buf->H,
		     GDK_RGB_DITHER_NORMAL,
		     t->buf->val, t->buf->rowlen);
  return FALSE;
}

gboolean  toolbar_press(GtkWidget *widget,GdkEventButton *eb,gpointer data)
{
  Toolbar *t = (Toolbar *) data;
  int i,x,y;

  if (eb->button == 1) {
    x = (int) (eb->x);
    y = (int) (eb->y);

    i = (t->cols * (y / t->h)) + (x / t->w);
    if (i >= 0 && i < t->tmpcount && t->enabled[i]) {
      t->selected = i;
      refresh(t->widget);
      if (t->callback != 0) {
	t->callback((void *) t);
      }
    }
  }
  return FALSE;
}

gboolean tips_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data) {
  Toolbar *t = (Toolbar *) data;
  int W,H,rW,rH;
  gdk_window_get_size(w->window,&W,&H);

  //  printf("tips_expose\n");

  if (t->thide) return FALSE;

  if (!t->pl)
    t->pl = gtk_widget_create_pango_layout(w," ");

  if (t->hover >= 0) {
    pango_layout_set_text(t->pl,t->tip[t->hover],-1);
  } else {
    pango_layout_set_text(t->pl," ",-1);
  }
  pango_layout_get_pixel_size(t->pl, &rW, &rH);
  rW += 6;
  rH += 6;

  if (W < rW || H < rH) {
    //printf("case 1 %d %d %d %d\n",W,rW,H,rH);
    gtk_drawing_area_size(GTK_DRAWING_AREA(t->tipc),rW,rH);
    gtk_window_resize(GTK_WINDOW(t->tips),rW,rH);
    return FALSE;
  }
  if (W-rW > 6 || H-rH > 6) {
    //printf("case 2 %d %d %d %d\n",W,rW,H,rH);
    gtk_drawing_area_size(GTK_DRAWING_AREA(t->tipc),rW,rH);
    gtk_window_resize(GTK_WINDOW(t->tips),rW,rH);
    return FALSE;
  }

  gdk_draw_layout(w->window,w->style->black_gc, 3,3, t->pl);

  gdk_draw_line(w->window,w->style->black_gc,0,0,W,0);
  gdk_draw_line(w->window,w->style->black_gc,0,H-1,W,H-1);
  gdk_draw_line(w->window,w->style->black_gc,0,0,0,H-1);
  gdk_draw_line(w->window,w->style->black_gc,W-1,0,W-1,H-1);

  return FALSE;
}

gboolean  toolbar_enter(GtkWidget *widget,GdkEventCrossing *ec,gpointer data)
{
  Toolbar *t = (Toolbar *) data;

  if (t->tips) {
    gtk_widget_show(t->tips);
    t->thide = 0;
    gtk_window_move(GTK_WINDOW(t->tips), (int) (ec->x_root), 
		    24 + (int) (ec->y_root));
    return FALSE;
  }

  t->tips = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_policy(GTK_WINDOW(t->tips),TRUE,TRUE,TRUE);
  t->tipc = gtk_drawing_area_new();
  gtk_widget_set_events(t->tipc,GDK_EXPOSURE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(t->tipc),8,8);
  gtk_container_add(GTK_CONTAINER(t->tips), t->tipc);
  util_set_bg(t->tips, 0xffffcc);
  util_set_bg(t->tipc, 0xffffcc);
  gtk_widget_show_all(t->tips);
  gtk_window_move(GTK_WINDOW(t->tips), (int) (ec->x_root), 
		  24 + (int) (ec->y_root));
  gtk_signal_connect(GTK_OBJECT(t->tipc),"expose_event",
		     GTK_SIGNAL_FUNC(tips_expose), data);
  return FALSE;
}

gboolean  toolbar_leave(GtkWidget *widget,GdkEventCrossing *ec,gpointer data)
{
  Toolbar *t = (Toolbar *) data;
  if (t->tips) {
    gtk_widget_hide(t->tips);
    t->thide = 1;
  }
  return FALSE;
}

gboolean  toolbar_motion(GtkWidget *widget,GdkEventMotion *em,gpointer data)
{
  Toolbar *t = (Toolbar *) data;
  int x,y,i;

  if (t->tips != 0) {

    x = (int) (em->x);
    y = (int) (em->y);

    i = (t->cols * (y / t->h)) + (x / t->w);
    if (i >= 0 && i < t->tmpcount) {
      t->hover = i;
      if (t->thide) {
	gtk_widget_show(t->tips);
	t->thide = 0;
      }
    } else {
      t->hover = -1;
      gtk_widget_hide(t->tips);
      t->thide = 1;
    }
    gtk_window_move(GTK_WINDOW(t->tips), 
		    (int) (em->x_root), 24 + (int) (em->y_root));
    refresh(t->tipc);
  }

  return FALSE;
}

// file dialog

struct _fdlgs {
  int done;
  int ok;
  GtkWidget *dlg,*ask;
  char *dest;
  int   askresult;
  char  lastpath[1024];  
  int   gotlast;
} fdlg_data = { .gotlast = 0 };

void fdlg_destroy (GtkWidget * w, gpointer data) {
  fdlg_data.done = 1;
}

void fdlg_ok (GtkWidget * w, gpointer data) {
  char *path,*dir;

  strncpy(fdlg_data.dest,
	  gtk_file_selection_get_filename(GTK_FILE_SELECTION(fdlg_data.dlg)),AV_PATHLEN);

  if (fdlg_data.ask != 0)
    fdlg_data.askresult = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fdlg_data.ask));

  path = strdup(fdlg_data.dest);
  dir = dirname(path);
  strncpy(fdlg_data.lastpath,dir, AV_PATHLEN);
  if (dir[strlen(dir)-1] != '/')
    strncat(fdlg_data.lastpath,"/",AV_PATHLEN);
  free(path);
  fdlg_data.gotlast = 1;

  fdlg_data.ok = 1;
  gtk_widget_destroy(fdlg_data.dlg);
}

int  util_get_dir(GtkWidget *parent, char *title, char *dest) {
  GtkWidget *fdlg;
  char *p;
  struct stat s;

  fdlg = gtk_file_selection_new(title);
  gtk_window_set_modal(GTK_WINDOW(fdlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(fdlg), GTK_WINDOW(parent));
  
  gtk_widget_hide(GTK_FILE_SELECTION(fdlg)->selection_entry);
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
  fdlg_data.ask  = 0;
  fdlg_data.askresult = 0;

  gtk_widget_show(fdlg);
  gtk_window_present(GTK_WINDOW(fdlg));

  while(!fdlg_data.done)
    if (gtk_events_pending())
      gtk_main_iteration();

  if (fdlg_data.ok) {
    if (dest[strlen(dest)-1]=='/')
      dest[strlen(dest)-1] = 0;
    for(;;) {
      if (stat(dest,&s)==0) {
	if (S_ISDIR(s.st_mode))
	  break;
	p = strrchr(dest,'/');
	if (!p) return 0;
	*p = 0;
      }
    }
  }

  return(fdlg_data.ok?1:0);
}

int  util_get_filename2(GtkWidget *parent, char *title, char *dest, int flags,
			char *asktext, int *askresult)
{
  GtkWidget *fdlg;
  FILE *f;
  char z[1024];

  fdlg = gtk_file_selection_new(title);
  gtk_window_set_modal(GTK_WINDOW(fdlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(fdlg), GTK_WINDOW(parent));
  
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fdlg)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC(fdlg_ok), 0 );
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
                                         (fdlg)->cancel_button),
                             "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             GTK_OBJECT (fdlg));
  gtk_signal_connect (GTK_OBJECT (fdlg), "destroy",
                      GTK_SIGNAL_FUNC(fdlg_destroy), 0 ); 

  fdlg_data.done       = 0;
  fdlg_data.ok         = 0;
  fdlg_data.dlg        = fdlg;
  fdlg_data.dest       = dest;
  fdlg_data.ask        = gtk_check_button_new_with_label(asktext);
  fdlg_data.askresult  = 0;

  gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(fdlg)->main_vbox),
		     fdlg_data.ask, FALSE, FALSE, 4);

  if (fdlg_data.gotlast)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fdlg),
				    fdlg_data.lastpath);

  gtk_widget_show(fdlg_data.ask);
  gtk_widget_show(fdlg);
  gtk_window_present(GTK_WINDOW(fdlg));

  while(!fdlg_data.done)
    if (gtk_events_pending())
      gtk_main_iteration();

  if (fdlg_data.ok) {

    if (flags & GF_ENSURE_WRITEABLE) {
      f = fopen(dest, "w");
      if (!f) {
        snprintf(z,1024,"Unable to open\n%s\nfor writing.",dest);
        PopMessageBox(parent,"Error",z,MSG_ICON_ERROR);
        return 0;
      }
      fclose(f);
    }
    if (flags & GF_ENSURE_READABLE) {
      f = fopen(dest, "r");
      if (!f) {
        snprintf(z,1024,"Unable to open\n%s\nfor reading.",dest);
        PopMessageBox(parent,"Error",z,MSG_ICON_ERROR);
        return 0;
      }
      fclose(f);
    }
    *askresult = fdlg_data.askresult;
    return 1;
  } else {
    return 0;
  }
}

int  util_get_filename(GtkWidget *parent, char *title, char *dest, int flags) 
{
  GtkWidget *fdlg;
  FILE *f;
  char z[1024];

  fdlg = gtk_file_selection_new(title);
  gtk_window_set_modal(GTK_WINDOW(fdlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(fdlg), GTK_WINDOW(parent));
  
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
  fdlg_data.ask  = 0;
  fdlg_data.askresult = 0;

  if (fdlg_data.gotlast)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fdlg),
				    fdlg_data.lastpath);

  gtk_widget_show(fdlg);
  gtk_window_present(GTK_WINDOW(fdlg));

  while(!fdlg_data.done)
    if (gtk_events_pending())
      gtk_main_iteration();

  if (fdlg_data.ok) {

    if (flags & GF_ENSURE_WRITEABLE) {
      f = fopen(dest, "w");
      if (!f) {
        snprintf(z,1024,"Unable to open\n%s\nfor writing.",dest);
        PopMessageBox(parent,"Error",z,MSG_ICON_ERROR);
        return 0;
      }
      fclose(f);
    }
    if (flags & GF_ENSURE_READABLE) {
      f = fopen(dest, "r");
      if (!f) {
        snprintf(z,1024,"Unable to open\n%s\nfor reading.",dest);
        PopMessageBox(parent,"Error",z,MSG_ICON_ERROR);
        return 0;
      }
      fclose(f);
    }
    return 1;
  } else {
    return 0;
  }
}

GtkWidget *csd = 0;
int csd_done = 0;
int csd_color = 0;

void util_csd_ok(GtkButton *b,gpointer data) {
  gdouble p[3];
  
  gtk_color_selection_get_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(csd)->colorsel), p);
  csd_color = triplet((int)(p[0]*255.0), (int)(p[1]*255.0), (int)(p[2]*255.0));
  gtk_widget_destroy(csd);
}

void util_csd_cancel(GtkButton *b,gpointer data) {
  gtk_widget_destroy(csd);
}

void util_csd_dead(GtkWidget *w,gpointer data) {
  csd_done = 1;
  csd = 0;
}

int util_get_color(GtkWidget *parent, int color, char *title) {
  gdouble p[3];

  csd_done  = 0;
  csd_color = color;

  csd = gtk_color_selection_dialog_new(title);
  gtk_window_set_modal(GTK_WINDOW(csd), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(csd), GTK_WINDOW(parent));
  //  gtk_window_set_position(GTK_WINDOW(csd), GTK_WIN_POS_CENTER);
  
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(csd)->ok_button),
		     "clicked",GTK_SIGNAL_FUNC(util_csd_ok),0);
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(csd)->cancel_button),
		     "clicked",GTK_SIGNAL_FUNC(util_csd_cancel),0);

  gtk_signal_connect(GTK_OBJECT(csd),
		     "destroy",GTK_SIGNAL_FUNC(util_csd_dead),0);

  gtk_color_selection_set_update_policy(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(csd)->colorsel), GTK_UPDATE_CONTINUOUS);

  p[0] = ((double)(t0(color))) / 255.0;
  p[1] = ((double)(t1(color))) / 255.0;
  p[2] = ((double)(t2(color))) / 255.0;

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(csd)->colorsel), p);
  gtk_widget_show(csd);
  gtk_widget_hide(GTK_COLOR_SELECTION_DIALOG(csd)->help_button);

  while(!csd_done) {
    if (gtk_events_pending())
      gtk_main_iteration();
  }

  return(csd_color);

}

void slider_action(Slider *s,int x,int y,int cb) {
  int px,pw,w;
  float val;

  w = s->widget->allocation.width;
  px = s->icon->W + 4 + 3;
  pw = w - s->icon->W - 6 - 6;

  if (x >= px && x <= px+pw) {
    val = ((float)(x-px))/((float)pw);
    s->value = val;
    if (s->callback && cb) s->callback(s);
    if (!s->grabbed) {
      gtk_grab_add(s->widget);
      s->grabbed = 1;
    }
    refresh(s->widget);
  }

}

gboolean slider_release(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  Slider *s = (Slider *) data;
  int x,y,b;

  x = (int) (eb->x);
  y = (int) (eb->y);
  b = eb->button;  
  if (!s->cont && b==1)
    slider_action(s,x,y,1);
  if (s->grabbed) {
    gtk_grab_remove(s->widget);
    s->grabbed = 0;
  }
  return FALSE;
}

gboolean slider_press(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  Slider *s = (Slider *) data;
  int x,y,b;

  x = (int) (eb->x);
  y = (int) (eb->y);
  b = eb->button;
  
  if (b==1) slider_action(s,x,y,s->cont);
  if (b==3) {
    s->value = s->defval;
    if (s->callback) s->callback(s);
    refresh(s->widget);
  }
  
  return FALSE;
}

gboolean slider_drag(GtkWidget *w, GdkEventMotion *em, gpointer data) {
  Slider *s = (Slider *) data;
  int x,y,b;


  if (em->window != w->window) return FALSE;

  x = (int) (em->x);
  y = (int) (em->y);
  b = 0;
  if (em->state & GDK_BUTTON1_MASK) b=1;
  if (em->state & GDK_BUTTON2_MASK) b=2;
  if (em->state & GDK_BUTTON3_MASK) b=3;

  if (b==1) slider_action(s,x,y,s->cont);

  return FALSE;
}

gboolean slider_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  Slider *s = (Slider *) data;
  int W,H,bx,bw,px;

  W = w->allocation.width;
  H = w->allocation.height;

  if (s->buf != 0) 
    if (s->buf->W != W || s->buf->H != H) {
      CImgDestroy(s->buf);
      s->buf=0;
    }
  if (s->buf == 0)
    s->buf = CImgNew(W,H);

  CImgFill(s->buf, s->bg);
  CImgDrawGlyph(s->buf, s->icon,2,(H - s->icon->H)/2,s->fg);
  
  bx = 4+s->icon->W;
  bw = W - 6 - s->icon->W;

  CImgDrawBevelBox(s->buf, bx, H/2-3, bw, 7, s->bg , 0, 0);
  px = bx + (s->value * (float)(bw-8));
  CImgDrawBevelBox(s->buf, bx, H/2-3, px-bx+2, 7, s->fill , 0, 0);

  CImgDrawBevelBox(s->buf, px, 2, 8, H-4, lighter(s->bg,5), 0, 1);


  gdk_draw_rgb_image(w->window, w->style->black_gc,
		     0,0,W,H,GDK_RGB_DITHER_NORMAL,
		     s->buf->val, s->buf->rowlen);
  return FALSE;
}

Slider *SliderNewFull(Glyph *icon, int w, int h) {
  Slider *x;

  x = (Slider *) MemAlloc(sizeof(Slider));
  if (!x) return 0;

  x->buf = 0;
  x->icon = icon;
  x->value = x->defval = 0.0;
  
  x->bg   = 0xc0c0c0;
  x->fg   = 0x000000;
  x->fill = 0xc0c0c0;
  x->callback = 0;
  x->grabbed = 0;
  x->cont = 1;

  x->widget = gtk_drawing_area_new();
  gtk_widget_set_events(x->widget,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_BUTTON1_MOTION_MASK|
			GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(x->widget), w,h);
  
  gtk_signal_connect(GTK_OBJECT(x->widget),"button_press_event",
		     GTK_SIGNAL_FUNC(slider_press), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"button_release_event",
		     GTK_SIGNAL_FUNC(slider_release), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"motion_notify_event",
		     GTK_SIGNAL_FUNC(slider_drag), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"expose_event",
		     GTK_SIGNAL_FUNC(slider_expose), (gpointer) x);

  gtk_widget_show(x->widget);
  return x;
}

Slider *SliderNew(Glyph *icon, int h) {
  Slider *x;

  x = (Slider *) MemAlloc(sizeof(Slider));
  if (!x) return 0;

  x->buf = 0;
  x->icon = icon;
  x->value = x->defval = 0.0;
  
  x->bg   = 0xc0c0c0;
  x->fg   = 0x000000;
  x->fill = 0xc0c0c0;
  x->callback = 0;
  x->grabbed = 0;
  x->cont = 1;

  x->widget = gtk_drawing_area_new();
  gtk_widget_set_events(x->widget,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_BUTTON1_MOTION_MASK|
			GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(x->widget), 100,h);
  

  gtk_signal_connect(GTK_OBJECT(x->widget),"button_press_event",
		     GTK_SIGNAL_FUNC(slider_press), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"button_release_event",
		     GTK_SIGNAL_FUNC(slider_release), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"motion_notify_event",
		     GTK_SIGNAL_FUNC(slider_drag), (gpointer) x);
  gtk_signal_connect(GTK_OBJECT(x->widget),"expose_event",
		     GTK_SIGNAL_FUNC(slider_expose), (gpointer) x);

  gtk_widget_show(x->widget);
  return x;
}

void    SliderSetCallback(Slider *s, void (*callback)(void *)) {
  s->callback = callback;
}

float   SliderGetValue(Slider *s) {
  return(s->value);
}

void    SliderSetValue(Slider *s, float value) {
  s->value = value;
  refresh(s->widget);
}

GdkCursor * util_cursor_new(GtkWidget *w,unsigned char *xbm,unsigned char *mask,int x,int y) {
  GdkBitmap *a,*b;
  GdkCursor *cur;
  GdkColor fg = { 0, 0xffff, 0xffff, 0xffff };
  GdkColor bg = { 0, 0, 0, 0 };
  a = gdk_bitmap_create_from_data (NULL, (char *) xbm,16,16);
  b = gdk_bitmap_create_from_data (NULL, (char *) mask,16,16);
  cur = gdk_cursor_new_from_pixmap(a,b,&fg,&bg,x,y);
  return cur;
}

GtkWidget * util_pixmap_new(GtkWidget *w, char **xpm) {
  GtkStyle *style;
  GtkWidget *z;
  GdkPixmap *a;
  GdkBitmap *b;
  style = w->style;
  
  a = gdk_pixmap_create_from_xpm_d(w->window, &b,
                                   &(style->bg[GTK_STATE_NORMAL]),
                                   (gchar **) xpm);
  z = gtk_pixmap_new(a,b);
  return z;
}

int sqdist(int x1,int y1,int x2,int y2) {
  int a,b;
  a = x1-x2;
  b = y1-y2;
  return(a*a+b*b); 
}

gboolean label_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data) {
  Label *l = (Label *) data;
  CImg *buf;
  int W,H,h;

  W = w->allocation.width;
  H = w->allocation.height;

  buf = CImgNew(W,H);
  h = SFontH(l->font,l->text);

  CImgFill(buf, l->bg);
  if (l->text)
    CImgDrawSText(buf, l->font, 4, (H-h)/2, l->fg, l->text);

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,buf->W,buf->H,
		     GDK_RGB_DITHER_NORMAL,
		     buf->val, buf->rowlen);
  CImgDestroy(buf);
  return FALSE;
}

Label * LabelNew(int bg,int fg,SFont *font) {
  Label *l;

  l = (Label *) MemAlloc (sizeof(Label));
  if (!l) return 0;

  l->bg = bg;
  l->fg = fg;
  l->font = font;

  l->text = 0;

  l->widget = gtk_drawing_area_new();
  gtk_widget_set_events(l->widget,GDK_EXPOSURE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(l->widget), 32, font->mh+2);
  gtk_widget_show(l->widget);

  gtk_signal_connect(GTK_OBJECT(l->widget), "expose_event",
		     GTK_SIGNAL_FUNC(label_expose), 
		     (gpointer) l);
  return l;
}

void    LabelSetText(Label *l, char * text) {
  if (l->text != 0) {
    MemFree(l->text);
    l->text = 0;
  }

  l->text=strdup(text);
  gtk_drawing_area_size(GTK_DRAWING_AREA(l->widget), 
			4 + SFontLen(l->font,text), SFontH(l->font,text));
  refresh(l->widget);
}

#define ICON_LB_LOCK "!000000000e0011002080208020807fc07fc07fc07fc07fc07fc07fc000000000"

#define ICON_LB_UNLOCK "!00000000003800440082008200807fc07fc07fc07fc07fc07fc07fc000000000"

Glyph *lbglyph[2] = {0,0};

LockBox * LockBoxNew(int bg,int fg,int state) {
  LockBox *b;

  b = (LockBox *) MemAlloc(sizeof(LockBox));
  if (!b) return 0;

  b->bg = bg;
  b->fg = fg;
  b->state = state;

  b->widget = gtk_drawing_area_new();
  gtk_widget_set_events(b->widget,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(b->widget), 20, 18);

  b->callback = 0;
  b->grabbed = 0;

  gtk_signal_connect(GTK_OBJECT(b->widget), "expose_event",
		     GTK_SIGNAL_FUNC(lb_expose), (gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget), "button_press_event",
		     GTK_SIGNAL_FUNC(lb_press), (gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget), "button_release_event",
		     GTK_SIGNAL_FUNC(lb_release), (gpointer) b);

  gtk_widget_show(b->widget);
  return b;
}

int       LockBoxGetState(LockBox *b) {
  return(b->state);
}

void      LockBoxSetState(LockBox *b, int state) {
  b->state = state;
  refresh(b->widget);
}

void      LockBoxSetCallback(LockBox *b,   void (*callback)(void *)) {
  b->callback = callback;
}

#define ICON_CB_CHECK "!00000003000e001c403820703ce01fe00fc00780078003000000000000000000"

Glyph *cbglyph = 0;

CheckBox * CheckBoxNew(int bg,int fg,int state) {
  CheckBox *cb;

  cb = (CheckBox *) MemAlloc(sizeof(CheckBox));
  if (!cb) return 0;

  cb->bg = bg;
  cb->fg = fg;
  cb->state = state;

  cb->widget = gtk_drawing_area_new();
  gtk_widget_set_events(cb->widget,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(cb->widget), 16, 16);

  cb->callback = 0;
  cb->grabbed = 0;

  gtk_signal_connect(GTK_OBJECT(cb->widget), "expose_event",
		     GTK_SIGNAL_FUNC(cb_expose), (gpointer) cb);
  gtk_signal_connect(GTK_OBJECT(cb->widget), "button_press_event",
		     GTK_SIGNAL_FUNC(cb_press), (gpointer) cb);
  gtk_signal_connect(GTK_OBJECT(cb->widget), "button_release_event",
		     GTK_SIGNAL_FUNC(cb_release), (gpointer) cb);

  gtk_widget_show(cb->widget);
  return cb;
}

int        CheckBoxGetState(CheckBox *cb) {
  return(cb->state);
}

void       CheckBoxSetState(CheckBox *cb, int state) {
  cb->state = state;
  refresh(cb->widget);
}

void       CheckBoxSetCallback(CheckBox *cb,   void (*callback)(void *)) {
  cb->callback = callback;
}

Button * ButtonNewFull(int bg,int fg, SFont *f, char *text, int w, int h) {
  Button *b;

  b = (Button *) MemAlloc(sizeof(Button));
  if (!b) return 0;

  b->bg = bg;
  b->fg = fg;
  b->text = strdup(text);
  b->callback = 0;
  b->grabbed = 0;
  b->font = f;

  b->widget = gtk_drawing_area_new();
  gtk_widget_set_events(b->widget,
			GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(b->widget),w,h);

  gtk_signal_connect(GTK_OBJECT(b->widget),"expose_event",
		     GTK_SIGNAL_FUNC(button_expose),(gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget),"button_press_event",
		     GTK_SIGNAL_FUNC(button_press),(gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget),"button_release_event",
		     GTK_SIGNAL_FUNC(button_release),(gpointer) b);

  gtk_widget_show(b->widget);
  return b;
}

Button * ButtonNew(int bg,int fg, SFont *f, char *text) {
  Button *b;
  int w;

  b = (Button *) MemAlloc(sizeof(Button));
  if (!b) return 0;

  b->bg = bg;
  b->fg = fg;
  b->text = strdup(text);
  b->callback = 0;
  b->grabbed = 0;
  b->font = f;

  b->widget = gtk_drawing_area_new();
  gtk_widget_set_events(b->widget,
			GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK);
  w = 6+SFontLen(f,text);
  gtk_drawing_area_size(GTK_DRAWING_AREA(b->widget),w,6+SFontH(f,text));

  gtk_signal_connect(GTK_OBJECT(b->widget),"expose_event",
		     GTK_SIGNAL_FUNC(button_expose),(gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget),"button_press_event",
		     GTK_SIGNAL_FUNC(button_press),(gpointer) b);
  gtk_signal_connect(GTK_OBJECT(b->widget),"button_release_event",
		     GTK_SIGNAL_FUNC(button_release),(gpointer) b);

  gtk_widget_show(b->widget);
  return b;
}

void     ButtonSetCallback(Button *b, void (*callback)(void *)) {
  b->callback = callback;
}

gboolean button_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  Button *b = (Button *) data;
  CImg *buf;
  int W,H;
  int tw,th;

  W = w->allocation.width;
  H = w->allocation.height;

  buf = CImgNew(W,H);
  CImgDrawBevelBox(buf,0,0,W,H,b->bg,0,b->grabbed ? 0 : 1);

  tw = SFontLen(b->font,b->text);
  th = SFontH(b->font,b->text);

  CImgDrawSText(buf,b->font,(W-tw)/2,(H-th)/2,b->fg,b->text);

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,buf->W,buf->H,
		     GDK_RGB_DITHER_NORMAL,
		     buf->val, buf->rowlen);
  
  CImgDestroy(buf);
  return FALSE;
}

gboolean button_press(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  Button *b = (Button *) data;

  if (eb->button == 1) {
    b->grabbed = 1;
    gtk_grab_add(w);
    refresh(b->widget);
  }

  return FALSE;
}

gboolean button_release(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  Button *b = (Button *) data;
  int x,y,W,H;

  x = (int) (eb->x);
  y = (int) (eb->y);
  W = b->widget->allocation.width;
  H = b->widget->allocation.height;

  if (b->grabbed && eb->button == 1) {
    b->grabbed = 0;
    gtk_grab_remove(w);
    refresh(b->widget);
    if (w->window == b->widget->window &&
	x>=0 && y>=0 && x<W && y<H)
      if (b->callback) b->callback((void *) b);
  }

  return FALSE;
}

gboolean lb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  LockBox *b = (LockBox *) data;
  CImg *buf;
  int W,H;

  W = w->allocation.width;
  H = w->allocation.height;

  buf = CImgNew(W,H);
  CImgFill(buf,b->bg);

  if (!lbglyph[0]) {
    lbglyph[0] = GlyphNewInit(16,16,ICON_LB_UNLOCK);
    lbglyph[1] = GlyphNewInit(16,16,ICON_LB_LOCK);
  }
  
  CImgDrawBevelBox(buf,(W-20)/2,(H-18)/2,20,18,
		   lighter(b->bg,3),0,b->grabbed ? 0 : 1);
  
  CImgDrawGlyph(buf,lbglyph[b->state?1:0],(W-16)/2,(H-16)/2,b->fg);

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,buf->W,buf->H,
		     GDK_RGB_DITHER_NORMAL,
		     buf->val, buf->rowlen);
  
  CImgDestroy(buf);

  return FALSE;
}

gboolean lb_press(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  LockBox *cb = (LockBox *) data;

  if (eb->button == 1) {
    cb->grabbed = 1;
    gtk_grab_add(w);
    refresh(cb->widget);
  }

  return FALSE;
}

gboolean lb_release(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  LockBox *cb = (LockBox *) data;

  if (cb->grabbed && eb->button == 1) {
    cb->state = !(cb->state);
    cb->grabbed = 0;
    gtk_grab_remove(w);
    refresh(cb->widget);
    if (cb->callback) cb->callback((void *) cb);
  }

  return FALSE;
}

gboolean cb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  CheckBox *cb = (CheckBox *) data;
  CImg *buf;
  int W,H;

  W = w->allocation.width;
  H = w->allocation.height;

  buf = CImgNew(W,H);
  CImgFill(buf,cb->bg);

  if (!cbglyph) cbglyph = GlyphNewInit(16,16,ICON_CB_CHECK);
  
  CImgDrawBevelBox(buf,(W-14)/2,(H-14)/2,14,14,
		   lighter(cb->bg,3),0,cb->grabbed ? 0 : 1);
  
  if (cb->state)
    CImgDrawGlyphAA(buf,cbglyph,(W-16)/2,(H-16)/2,cb->fg);

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,buf->W,buf->H,
		     GDK_RGB_DITHER_NORMAL,
		     buf->val, buf->rowlen);
  
  CImgDestroy(buf);

  return FALSE;
}

gboolean cb_press(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  CheckBox *cb = (CheckBox *) data;

  if (eb->button == 1) {
    cb->grabbed = 1;
    gtk_grab_add(w);
    refresh(cb->widget);
  }

  return FALSE;
}

gboolean cb_release(GtkWidget *w,GdkEventButton *eb,gpointer data) {
  CheckBox *cb = (CheckBox *) data;

  if (cb->grabbed && eb->button == 1) {
    cb->state = !(cb->state);
    cb->grabbed = 0;
    gtk_grab_remove(w);
    refresh(cb->widget);
    if (cb->callback) cb->callback((void *) cb);
  }

  return FALSE;
}

gboolean color_label_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  ColorLabel *cl = (ColorLabel *) data;
  GdkGC *gc;

  gc = gdk_gc_new(w->window);
  gc_color(gc, cl->color);
  gdk_draw_rectangle(w->window, gc, TRUE, 0,0, 
		     w->allocation.width,
		     w->allocation.height);
  gc_color(gc, cl->border);
  gdk_draw_rectangle(w->window, gc, FALSE, 0,0, 
		     w->allocation.width - 1,
		     w->allocation.height - 1);
  
  gdk_gc_destroy(gc);
  return FALSE;
}

ColorLabel * ColorLabelNew(int w,int h,int color,int border) {
  ColorLabel *cl;

  cl = (ColorLabel *) MemAlloc(sizeof(ColorLabel));
  if (!cl) return 0;

  cl->color = color;
  cl->border = border;
  cl->widget = gtk_drawing_area_new();
  gtk_widget_set_events(cl->widget,GDK_EXPOSURE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(cl->widget),w,h);

  gtk_signal_connect(GTK_OBJECT(cl->widget),"expose_event",
		     GTK_SIGNAL_FUNC(color_label_expose),
		     (gpointer) cl);

  gtk_widget_show(cl->widget);
  return cl;
}

void         ColorLabelSetColor(ColorLabel *cl, int color) {
  cl->color = color;
  refresh(cl->widget);
}


char * str2utf8(const char *src) {
  static char *t = 0;
  int tsize = 0;

  gchar *r;

  r = g_convert(src,strlen(src),"UTF-8","ISO-8859-1",0,0,0);

  if (t && tsize <= strlen(r)) {
    MemFree(t);
    t = 0;
  }

  if (!t) {
    tsize = strlen(r) + 1;
    t = (char *) MemAlloc(tsize);
    if (t!=0)
      strncpy(t,r,tsize);
  }
  g_free(r);
  return t;
}

char  *str2latin1(const char *src) {
  static char *t = 0;
  int tsize = 0;

  gchar *r;

  r = g_convert(src,strlen(src),"ISO-8859-1","UTF-8",0,0,0);

  if (t && tsize <= strlen(r)) {
    MemFree(t);
    t = 0;
  }

  if (!t) {
    tsize = strlen(r) + 1;
    t = (char *) MemAlloc(tsize);
    if (t!=0)
      strncpy(t,r,tsize);
  }
  g_free(r);
  return t;
}

gboolean filler_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  Filler *f = (Filler *) data;
  int W,H;

  W = w->allocation.width;
  H = w->allocation.height;

  if (!f->gc) {
    f->gc = gdk_gc_new(w->window);
    gc_color(f->gc, f->color);
  }
  gdk_draw_rectangle(w->window, f->gc, TRUE, 0,0,W,H);
  return FALSE;
}

Filler * FillerNew(int color) {
  Filler *f;
  f = (Filler *) MemAlloc(sizeof(Filler));
  if (!f) return 0;
  f->color = color;
  f->gc = 0;
  f->widget = gtk_drawing_area_new();
  gtk_widget_set_events(f->widget, GDK_EXPOSURE_MASK);
  gtk_signal_connect(GTK_OBJECT(f->widget),"expose_event",
		     GTK_SIGNAL_FUNC(filler_expose),(gpointer) f);
  gtk_widget_show(f->widget);
  return f;
}

void util_set_fg(GtkWidget *w, int fg) {
  GdkColor c;
  char z[16];
  snprintf(z,16,"#%.2x%.2x%.2x",t0(fg),t1(fg),t2(fg));
  gdk_color_parse(z,&c);
  gtk_widget_modify_fg(w,GTK_STATE_NORMAL,&c);
}

void util_set_bg(GtkWidget *w, int bg) {
  GdkColor c;
  char z[16];
  snprintf(z,16,"#%.2x%.2x%.2x",t0(bg),t1(bg),t2(bg));
  gdk_color_parse(z,&c);
  gtk_widget_modify_bg(w,GTK_STATE_NORMAL,&c);
}

Mutex *MutexNew() {
  Mutex *m;
  m = (Mutex *) MemAlloc(sizeof(Mutex));
  if (!m) return 0;
  pthread_mutex_init(m,NULL);
  return m;
}

void  MutexDestroy(Mutex *m) {
  if (m) {
    pthread_mutex_destroy(m);
    MemFree(m);
  }
}

void  MutexLock(Mutex *m) {
  pthread_mutex_lock(m);
}

void  MutexUnlock(Mutex *m) {
  pthread_mutex_unlock(m);
}

int   MutexTryLock(Mutex *m) {
  if (pthread_mutex_trylock(m)==EBUSY)
    return -1;
  else
    return 0;
}

DropBox * dropbox_new(char *s) {
  DropBox *d;
  GtkMenu *m;
  char sep[2], *p, *cp;

  d = (DropBox *) MemAlloc(sizeof(DropBox));
  if (!d) return 0;
  d->no = 0;
  d->value = 0;
  d->callback = 0;

  m = GTK_MENU(gtk_menu_new());

  cp = (char *) MemAlloc(strlen(s) + 1);
  if (!cp) { MemFree(d); return 0; }
  strncpy(cp, s, strlen(s) + 1);

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

void disable(GtkWidget *w) {
  gtk_widget_set_sensitive(w,FALSE);
}

void enable(GtkWidget *w) {
  gtk_widget_set_sensitive(w,TRUE);
}

GtkWidget * util_dialog_new(char *title, GtkWidget *parent, int modal,
			    GtkWidget **vbox, GtkWidget **bbox, 
			    int dw, int dh)
{
  GtkWidget *w,*v,*hb;
  
  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(w), modal?TRUE:FALSE);
  gtk_window_set_title(GTK_WINDOW(w), title);
  if (dw >= 0 && dh >= 0)
    gtk_window_set_default_size(GTK_WINDOW(w), dw, dh);
  if (parent!=0) 
    gtk_window_set_transient_for(GTK_WINDOW(w), GTK_WINDOW(parent));
  gtk_container_set_border_width(GTK_CONTAINER(w), 6);
  gtk_widget_realize(w);

  if (vbox != 0) {
    v = gtk_vbox_new(FALSE,2);
    gtk_container_add(GTK_CONTAINER(w), v);
    *vbox = v;

    if (bbox != 0) {
      hb = gtk_hbutton_box_new();
      gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
      gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
      gtk_box_pack_end(GTK_BOX(v), hb, FALSE, TRUE, 2);
      *bbox = hb;
    }
  }

  return w;
}

GtkWidget * util_rlabel(char *text, GtkWidget **L) {
  GtkWidget *h,*l;
  h = gtk_hbox_new(FALSE,0);
  l = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(l),GTK_JUSTIFY_RIGHT);
  gtk_box_pack_end(GTK_BOX(h),l,FALSE,FALSE,0);
  if (L!=0) *L = l;
  return h;
}

GtkWidget * util_llabel(char *text, GtkWidget **L) {
  GtkWidget *h,*l;
  h = gtk_hbox_new(FALSE,0);
  l = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(l),GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(h),l,FALSE,FALSE,0);
  if (L!=0) *L = l;
  return h;
}

struct formatdlg {
  GtkWidget *dlg;
  int        format;
  GtkWidget *rb[32];
  int nb;
  int done;
  int choice;
} fmdlg;

void fmdlg_destroy(GtkWidget *w, gpointer data) {
  fmdlg.done = 1;
}

void fmdlg_cancel(GtkWidget *w, gpointer data) {
  fmdlg.choice = -1;
  gtk_widget_destroy(fmdlg.dlg);
}

void fmdlg_ok(GtkWidget *w, gpointer data) {
  int i;
  for(i=0;i<fmdlg.nb;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fmdlg.rb[i])))
      fmdlg.choice = i;
  gtk_widget_destroy(fmdlg.dlg);
}

int util_get_format(GtkWidget *parent, char *title, char *formats) {
  char *cp,*p,sep[2];
  GtkWidget *v, *h, *f, *v2, *ok, *cancel;
  int j;

  cp = strdup(formats);
  if (!cp) return -1;

  fmdlg.dlg = util_dialog_new(title,parent,1,&v,&h,-1,-1);
  
  f = gtk_frame_new("Available Formats");
  gtk_box_pack_start(GTK_BOX(v), f, TRUE, TRUE, 2);
  v2 = gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(f), v2);

  j = 0;
  sep[0] = cp[0]; sep[1] = 0;
  p = strtok(&cp[1],sep);
  while(p) {
    if (j==0)
      fmdlg.rb[j] = gtk_radio_button_new_with_label(NULL,p);
    else
      fmdlg.rb[j] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fmdlg.rb[0]),p);

    gtk_box_pack_start(GTK_BOX(v2), fmdlg.rb[j], FALSE, TRUE, 2);
    ++j;
    p=strtok(0,sep);
  }

  fmdlg.nb = j;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fmdlg.rb[0]),TRUE);

  ok     = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");

  gtk_box_pack_start(GTK_BOX(h), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h), cancel, TRUE, TRUE, 0);

  fmdlg.choice = -1;
  fmdlg.done   = 0;

  gtk_signal_connect(GTK_OBJECT(fmdlg.dlg),"destroy",
		     GTK_SIGNAL_FUNC(fmdlg_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(fmdlg_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(fmdlg_cancel), 0);

  gtk_widget_show_all(fmdlg.dlg);
  gtk_window_present(GTK_WINDOW(fmdlg.dlg));

  while(!fmdlg.done)
    if (gtk_events_pending())
      gtk_main_iteration();    

  return(fmdlg.choice);
}

void util_set_icon(GtkWidget *w, const char **xpm, char *text) {
  GdkPixbuf *pb;

  pb = gdk_pixbuf_new_from_xpm_data(xpm);
  gtk_window_set_icon(GTK_WINDOW(w), pb);
  if (text)
    gdk_window_set_icon_name(w->window,text);
}

void resource_path(char *dest,const char *src, int ndest) {
  char paths[4][512];
  char tmp[1024];
  int i;
  FILE *f;

  snprintf(paths[0],512,"%s/.aftervoxel",res.Home);
  strncpy(paths[1],"/usr/share/aftervoxel",512);
  strncpy(paths[2],"/usr/local/share/aftervoxel",512);
  strncpy(paths[3],"./",512);

  for(i=0;i<4;i++) {
    snprintf(tmp,1024,"%s/%s",paths[i],src);
    f = fopen(tmp,"r");
    if (f!=NULL) {
      fclose(f);
      strncpy(dest,tmp,ndest);
      return;
    }
  }
  //  fprintf(stderr,"Resource not found: %s\n",src);
  dest[0] = 0;
}

GtkWidget * util_pixbutton_new(char **xpm) {
  GtkWidget *b,*p;
  b = gtk_button_new();
  p = util_pixmap_new(mw,xpm);
  gtk_container_add(GTK_CONTAINER(b),p);
  gtk_widget_show(p);
  gtk_widget_show(b);
  return b;
}

float angle(int x1,int y1,int x0,int y0) {
  
  float ang,dx,dy;

  dx = x1 - x0;
  dy = y0 - y1;

  if (x0!=x1) {
    ang = atan(dy/dx);
    if (x1 < x0) ang += M_PI;    
  } else {
    ang = M_PI / 2.0;
    if (y1 > y0) ang += M_PI;
  }
  ang *= 180.0 / M_PI;
  //  printf("ang = %.3f\n",ang);
  return(ang);
}
