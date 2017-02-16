/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2017
   
   Felipe P. G. Bergo 
   fbergo at gmail.com

   and
   
   Alexandre X. Falcao
   afalcao at ic.unicamp.br

   ----------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <libip.h>
#include <math.h>
#include "util.h"

gboolean picker_expose(GtkWidget *widget,GdkEventExpose *ee,
			gpointer data);
gboolean picker_down(GtkWidget *widget,GdkEventButton *be, gpointer data);
gboolean picker_up(GtkWidget *widget,GdkEventButton *be, gpointer data);

void     dropbox_select(GtkWidget *w,gpointer data);
void     radiobox_toggled(GtkToggleButton *t, gpointer data);

gboolean vslide_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data);
gboolean  vslide_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data);
gboolean  vslide_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);

gboolean vmapctl_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data);
gboolean  vmapctl_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data);
gboolean  vmapctl_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);
gboolean  vmapctl_release(GtkWidget *widget, GdkEventButton *eb, gpointer data);
gboolean  vmapctl_leave(GtkWidget *widget, GdkEventCrossing *ec, gpointer data);
void vmapctl_p0(GtkWidget *w, gpointer data);
void vmapctl_p1(GtkWidget *w, gpointer data);
void vmapctl_p2(GtkWidget *w, gpointer data);
void vmapctl_p3(GtkWidget *w, gpointer data);
void vmapctl_p4(GtkWidget *w, gpointer data);
void vmapctl_p5(GtkWidget *w, gpointer data);
void vmapctl_reset(GtkWidget *w, gpointer data);

gboolean colortag_expose(GtkWidget *widget,GdkEventExpose *ee,
			 gpointer data);

void set_style_bg(GtkStyle *s, int color) {
  int R,G,B,i;

  R = t0(color) << 8;
  G = t1(color) << 8;
  B = t2(color) << 8;

  for(i=0;i<5;i++) {
    s->bg[i].red   = R;
    s->bg[i].green = G;
    s->bg[i].blue  = B;
  }
}

void gc_color(GdkGC *gc, int color) {
  gdk_rgb_gc_set_foreground(gc, color);
}

void  gc_clip(GdkGC *gc, int x, int y, int w, int h) {
  GdkRectangle r;
  r.x = x;
  r.y = y;
  r.width = w;
  r.height = h;
  gdk_gc_set_clip_rectangle(gc, &r);
}

void refresh(GtkWidget *w) {
  gtk_widget_queue_draw(w);
}

GtkWidget * icon_button(GtkWidget *parent, char **xpm, char *text) {
  GtkWidget *b, *v, *l=0, *x;
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkStyle *style;
  b = gtk_toggle_button_new();
  v = gtk_vbox_new(FALSE,0);

  if (text!=0)
    l = gtk_label_new(text);
  
  style = gtk_widget_get_style(parent);
  icon =   gdk_pixmap_create_from_xpm_d (parent->window, &mask,
					 &style->bg[GTK_STATE_NORMAL],
					 (gchar **) xpm);
  x = gtk_pixmap_new(icon, mask);

  gtk_container_add(GTK_CONTAINER(b), v);
  gtk_box_pack_start(GTK_BOX(v), x, FALSE, TRUE, 0);

  if (l) {
    gtk_box_pack_start(GTK_BOX(v), l, FALSE, TRUE, 0);
    gtk_widget_show(l);
  }

  gtk_widget_show(x);
  gtk_widget_show(v);
  return b;
}

void set_icon(GtkWidget *widget, char **xpm, char *text) {
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

Graphics * graphics_validate_buffer(GdkWindow *w, Graphics *prev) {
  int ww,wh;
  Graphics *g;
  gdk_window_get_size(w,&ww,&wh);
  if (!prev) {
    g = CREATE(Graphics);
    g->w  = gdk_pixmap_new(w, ww, wh, -1);
    g->gc = gdk_gc_new(g->w);
    g->W  = ww;
    g->H  = wh;
    return g;
  } else {
    if (prev->W < ww || prev->H < wh) {
      gdk_gc_destroy(prev->gc);
      gdk_pixmap_unref(prev->w);
      free(prev);
      g = CREATE(Graphics);
      g->w  = gdk_pixmap_new(w, ww, wh, -1);
      g->gc = gdk_gc_new(g->w);
      g->W  = ww;
      g->H  = wh;
      return g;
    } else {
      return prev;
    }
  }
}

void graphics_commit_buffer(Graphics *g, GdkWindow *w, GdkGC *gc) {
  if (g) gdk_draw_pixmap(w,gc,g->w,0,0,0,0,g->W,g->H);
}

void ivs_condshow(int condition, GtkWidget *w) {
  if (condition)
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);
}

GtkWidget * ivs_pixmap_new(GtkWidget *w, char **xpm) {
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

void shielded_text(GdkDrawable *w, PangoLayout *pl, GdkGC *gc, int x, int y,
		   int c0, int c1, char *text)
{
  int i,j;
  if (pl==NULL) return;

  gc_color(gc,c1);
  pango_layout_set_text(pl, text, -1); 
  for(i=-1;i<2;i++)
    for(j=-1;j<2;j++)
      if (i!=0 || j!=0)
	gdk_draw_layout(w,gc,x+i,y+j,pl);
  gc_color(gc,c0);
  gdk_draw_layout(w,gc,x,y,pl);
}

void shadow_text(GdkDrawable *w, PangoLayout *pl, GdkGC *gc, int x, int y,
		 int c0, int c1, char *text)
{
  if (pl==NULL) return;
  pango_layout_set_text(pl, text, -1); 
  gc_color(gc,c1);
  gdk_draw_layout(w,gc,x+1,y+1,pl);
  gc_color(gc,c0);
  gdk_draw_layout(w,gc,x,y,pl);
}

void bevel_box(GdkDrawable *d, GdkGC *gc, int color, int x, int y,
	       int w, int h) {
  int cd, cl;

  cl  = lighter(color, 4);
  cd  = darker(color, 4);

  gc_color(gc, color);
  gdk_draw_rectangle(d, gc, TRUE, x,y, w, h);

  gc_color(gc, cd);
  gdk_draw_rectangle(d, gc, FALSE, x+1,y+1, w-2, h-2);

  gc_color(gc, cl);
  gdk_draw_line(d, gc, x+1, y+1, x+w-2, y+1);
  gdk_draw_line(d, gc, x+1, y+1, x+1, y+h-2);

  gc_color(gc, 0);
  gdk_draw_rectangle(d, gc, FALSE, x,y, w,h);
}

void ivs_safe_short_filename(char *x, char *dest,int destsize) {
  int l,p;

  l = strlen(x);
  p = l-1;

  while(p>0 && x[p]!='/') --p;
  if (x[p] == '/') ++p;

  l = l-p+1;
  if (l>=destsize) l=destsize-1;

  strncpy(dest,&x[p],l);
  dest[destsize-1] = 0;
}

char * ivs_short_filename(char *x) {
  static char rdata[256];
  ivs_safe_short_filename(x,rdata,256);
  return rdata;
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

int  ivs_get_filename(GtkWidget *parent, char *title, char *dest, int flags) {
  GtkWidget *fdlg;
  FILE *f;
  char z[1024];

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

  while(!fdlg_data.done)
    if (gtk_events_pending())
      gtk_main_iteration();

  if (fdlg_data.ok) {

    if (flags & GF_ENSURE_WRITEABLE) {
      f = fopen(dest, "w");
      if (!f) {
	sprintf(z,"Unable to open\n%s\nfor writing.",dest);
	PopMessageBox(parent,"Error",z,MSG_ICON_ERROR);
	return 0;
      }
      fclose(f);
    }
    if (flags & GF_ENSURE_READABLE) {
      f = fopen(dest, "r");
      if (!f) {
	sprintf(z,"Unable to open\n%s\nfor reading.",dest);
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

void draw_cuboid_fill(Cuboid *c, GdkDrawable *dest, GdkGC *gc,
		      int fillcolor)
{
  int i,j;
  GdkPoint p[4];

  gc_color(gc, fillcolor);

  for(i=0;i<6;i++)
    if (!c->hidden[i]) {
      for(j=0;j<4;j++) {
	p[j].x = c->f[i].p[j]->x;
	p[j].y = c->f[i].p[j]->y;
      }
      gdk_draw_polygon(dest,gc,TRUE,p,4);       
    }
}

void draw_cuboid_frame(Cuboid *c, GdkDrawable *dest, GdkGC *gc,
		       int hiddencolor, int edgecolor, int drawhidden) {
  int i,j;

  if (drawhidden) {
    gc_color(gc, hiddencolor);
    for(i=0;i<6;i++)
      if (c->hidden[i]) {
	for(j=0;j<4;j++)
	  gdk_draw_line(dest,gc, 
			c->f[i].p[j]->x, 
			c->f[i].p[j]->y,
			c->f[i].p[(j+1)%4]->x, 
			c->f[i].p[(j+1)%4]->y);
      }
  }
  
  gc_color(gc, edgecolor);
  for(i=0;i<6;i++)
    if (!c->hidden[i]) {
      for(j=0;j<4;j++)
	gdk_draw_line(dest,gc, 
		      c->f[i].p[j]->x, 
		      c->f[i].p[j]->y,
		      c->f[i].p[(j+1)%4]->x, 
		      c->f[i].p[(j+1)%4]->y);
    }
}

void draw_clip_arrow(Point *p,  GdkDrawable *dest, GdkGC *gc,
		     int fill, int edge, int disp) {
  int i;
  GdkPoint q[3];

  for(i=0;i<3;i++) {
    q[i].x = p[i+disp].x;
    q[i].y = p[i+disp].y;
  }

  gc_color(gc, fill);
  gdk_draw_polygon(dest, gc, TRUE, q, 3);
  gc_color(gc, edge);
  gdk_draw_polygon(dest, gc, FALSE, q, 3);
}

void draw_clip_arrows(Point *p,  GdkDrawable *dest, GdkGC *gc,
		      int h1fill, int h1edge,
		      int h2fill, int h2edge) 
{
  if (p[4].z < p[7].z) {
    draw_clip_arrow(p, dest, gc, h2fill, h2edge, 7);
    draw_clip_arrow(p, dest, gc, h1fill, h1edge, 4);
  } else {
    draw_clip_arrow(p, dest, gc, h1fill, h1edge, 4);
    draw_clip_arrow(p, dest, gc, h2fill, h2edge, 7);
  }
}

void draw_clip_track(Point *p,  GdkDrawable *dest, GdkGC *gc,
		     int bgcolor, int edgecolor, int h1, int h2)
{
  GdkPoint q[4];
  int i;

  for(i=0;i<4;i++) {
    q[i].x = p[i].x;
    q[i].y = p[i].y;
  }
  
  gc_color(gc, bgcolor);
  gdk_draw_polygon(dest, gc, TRUE, q, 4);

  gc_color(gc, h1);
  gdk_draw_line(dest, gc, p[4].x, p[4].y, p[10].x, p[10].y);
  gc_color(gc, h2);
  gdk_draw_line(dest, gc, p[7].x, p[7].y, p[11].x, p[11].y);

  gc_color(gc, edgecolor);
  gdk_draw_polygon(dest, gc, FALSE, q, 4);
}

void draw_clip_control(Point *p, GdkDrawable *dest, GdkGC *gc,
		       int bgcolor, int edgecolor, 
		       int h1fill, int h1edge,
		       int h2fill, int h2edge)
{
  if (p[5].z < p[0].z || p[6].z < p[0].z) {
    draw_clip_track(p, dest, gc, bgcolor, edgecolor, h1edge, h2edge);
    draw_clip_arrows(p, dest, gc, h1fill, h1edge, h2fill, h2edge);
  } else {
    draw_clip_arrows(p, dest, gc, h1fill, h1edge, h2fill, h2edge);
    draw_clip_track(p, dest, gc, bgcolor, edgecolor, h1edge, h2edge);
  }
}

Picker * picker_new(int exclusive, int style) {
  Picker *p;

  p = CREATE(Picker);
  if (!p) return 0;
  MemSet(p,0,sizeof(Picker));

  p->exclusive = exclusive;
  p->value = 0;
  p->style = style;
  
  p->widget = gtk_drawing_area_new();
  gtk_widget_set_events(p->widget,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(p->widget), 161, 33);
  
  gtk_signal_connect(GTK_OBJECT(p->widget),"expose_event",
                     GTK_SIGNAL_FUNC(picker_expose), (gpointer) p);

  gtk_signal_connect(GTK_OBJECT(p->widget),"button_press_event",
                     GTK_SIGNAL_FUNC(picker_down), (gpointer) p);

  gtk_signal_connect(GTK_OBJECT(p->widget),"button_release_event",
                     GTK_SIGNAL_FUNC(picker_up), (gpointer) p);
  p->gc = 0;
  p->de = -1;

  p->pl =  gtk_widget_create_pango_layout(p->widget, NULL);
  p->pfd = pango_font_description_from_string("Sans 10");
  if (p->pl != NULL && p->pfd != NULL)
    pango_layout_set_font_description(p->pl, p->pfd);

  gtk_widget_show(p->widget);

  return p;
}

void picker_repaint(Picker *p) {
  gtk_widget_queue_draw(p->widget);
}

void picker_set_down(Picker *p, int i, int down) {
  int j;
  if (p->down[i] != down) {
    p->down[i] = down;

    if (p->exclusive && down) {
      for(j=0;j<10;j++)
	if (j!=i) p->down[j] = 0;
      p->value = i;
    }

    picker_repaint(p);
  }
}

int picker_get_down(Picker *p, int i) {
  return(p->down[i]);
}

int picker_get_selection(Picker *p) {
  return(p->value);
}

void picker_set_color(Picker *p, int i, int color) {
  p->color[i] = color;
}

void picker_set_changed_callback(Picker *p, void (*callback)(void*)) {
  p->callback = callback;
}

gboolean 
picker_down(GtkWidget *widget,GdkEventButton *be, gpointer data) {
  Picker *me = (Picker *) data;
  int x,y;

  x = (int) (be->x); y = (int) (be->y);
  
  x/=32; y/=16;
  me->de = -1;

  if (x<5 && x>=0 && y<2 && y>=0)
    me->de = 5*y + x;

  return FALSE;
}

gboolean 
picker_up(GtkWidget *widget,GdkEventButton *be, gpointer data) {
  Picker *me = (Picker *) data;
  int i,x,y;

  x = (int) (be->x); y = (int) (be->y);
  
  x/=32; y/=16;

  if (me->de >= 0) {
    
    x = 5*y + x;
    if (x == me->de) {

      if (me->exclusive && me->down[x]) goto nada;
      me->down[x] = !me->down[x];
      me->value = x;

      if (me->exclusive)
	for(i=0;i<10;i++)
	  if (i!=x) me->down[i] = 0;

      picker_repaint(me);
      if (me->callback != 0) me->callback( (void *) me );
    }
  }

 nada:
  me->de = -1;
  return FALSE;
}

gboolean 
picker_expose(GtkWidget *widget,GdkEventExpose *ee,
	      gpointer data) 
{
  Picker *me = (Picker *) data;
  GdkWindow  *w  = widget->window;
  int i,j,k,x,y,W,H,l,rc,d,ll,dd;
  char z[8];
  PangoRectangle pri, prl;

  if (!me->gc)
    me->gc = gdk_gc_new(w);

  for(i=0;i<10;i++) {
    x = 32*(i%5);
    y = 16*(i/5);

    rc = me->color[i];

    if (!me->down[i]) rc = mergeColorsRGB(rc,0xc0c0c0,0.50);

    l = mergeColorsRGB(rc,0xffffff,0.66);
    d = mergeColorsRGB(rc,0x000000,0.66);

    ll = mergeColorsRGB(rc,0xffffff,0.33);
    dd = mergeColorsRGB(rc,0x000000,0.33);

    gdk_rgb_gc_set_foreground(me->gc, me->down[i]?l:d);
    gdk_draw_rectangle(w,me->gc,TRUE,x,y,32,16);
    gdk_rgb_gc_set_foreground(me->gc, me->down[i]?ll:dd);
    gdk_draw_rectangle(w,me->gc,TRUE,x+2,y+2,32-3,16-3);
    gdk_rgb_gc_set_foreground(me->gc, rc);
    gdk_draw_rectangle(w,me->gc,TRUE,x+3,y+3,32-5,16-5);

    gdk_rgb_gc_set_foreground(me->gc, me->down[i]?d:l);
    gdk_draw_line(w,me->gc,x+1,y+15,x+1,y+1);
    gdk_draw_line(w,me->gc,x+1,y+1,x+31,y+1);
    gdk_rgb_gc_set_foreground(me->gc, me->down[i]?dd:ll);
    gdk_draw_line(w,me->gc,x+2,y+14,x+2,y+2);
    gdk_draw_line(w,me->gc,x+2,y+2,x+30,y+2);

    switch(me->style) {

    case PICKER_STYLE_NOTHING: break;
    case PICKER_STYLE_BOX:
      if (me->down[i]) {
	gdk_rgb_gc_set_foreground(me->gc, 0x000000);
	gdk_draw_rectangle(w,me->gc,TRUE,x+16-4+1,y+8-2+1,9,5);
	gdk_rgb_gc_set_foreground(me->gc, ll);
	gdk_draw_rectangle(w,me->gc,TRUE,x+16-4,y+8-2,8,4);
      }
      
      gdk_rgb_gc_set_foreground(me->gc, me->down[i]?0:dd);
      gdk_draw_rectangle(w,me->gc,FALSE,x+16-4,y+8-2,8,4);
      break;

    case PICKER_STYLE_NUMBER:
    case PICKER_STYLE_LABEL:
      sprintf(z,"%d",i);
      if (i==0 && me->style == PICKER_STYLE_LABEL)
	strcpy(z,"BG");

      pango_layout_set_text(me->pl, z, -1);
      pango_layout_get_pixel_extents(me->pl, &pri, &prl);

      W = prl.width;
      H = prl.height;

      gdk_rgb_gc_set_foreground(me->gc, dd);

      for(j=-2;j<=2;j++)
	for(k=-1;k<=1;k++)
	  gdk_draw_layout(w, me->gc, x+(32-W)/2+j,y+(16-H)/2+k, me->pl);

      gdk_draw_layout(w, widget->style->white_gc, x+(32-W)/2,y+(16-H)/2, me->pl);
      
      break;
    case PICKER_STYLE_EYE:
      gc_color(me->gc, 0xffffff);
      gdk_draw_arc(w,me->gc,TRUE,x+16-9,y+8-4,18,8,0,360*64);
      gc_color(me->gc, 0xc0c0c0);
      gdk_draw_arc(w,me->gc,TRUE,x+16-9,y+8-4,18,8,0,180*64);
      gc_color(me->gc, 0x000000);
      gdk_draw_arc(w,me->gc,FALSE,x+16-9,y+8-4,18,8,0,360*64);
      gdk_draw_arc(w,me->gc,FALSE,x+16-10,y+8-1,20,3,0,180*64);
      gc_color(me->gc, 0x603000);
      gdk_draw_arc(w,me->gc,TRUE,x+16-3,y+8-3,6,6,0,360*64);
      gc_color(me->gc, 0xc0c0c0);
      gdk_draw_rectangle(w,me->gc,TRUE,x+16-4,y+8-3,8,2);

      if (!me->down[i]) {
	gc_color(me->gc, 0x000000);
	for(j=-2;j<=2;j+=4) {
	  gdk_draw_line(w,me->gc,x+16-5+j,y+8-5,x+16+5+j,y+8+5);
	  gdk_draw_line(w,me->gc,x+16-5+j,y+8+5,x+16+5+j,y+8-5);
	}
	gc_color(me->gc, 0xff0000);
	for(j=-1;j<=1;j++) {
	  gdk_draw_line(w,me->gc,x+16-5+j,y+8-5,x+16+5+j,y+8+5);
	  gdk_draw_line(w,me->gc,x+16-5+j,y+8+5,x+16+5+j,y+8-5);
	}
      }
      break;

    }
  }

  gdk_draw_rectangle(w,widget->style->black_gc, FALSE, 0,0, 160,16);
  gdk_draw_rectangle(w,widget->style->black_gc, FALSE, 0,16, 160,16);

  for(i=0;i<5;i++)
    gdk_draw_line(w,widget->style->black_gc, i*32, 0, i*32, 32);

  return FALSE;
}

RadioBox * radiobox_new(char *title, char *s) 
{
  RadioBox *r;
  GtkWidget *v;
  char sep[2], *p, *cp;
  GSList *rg;
  
  r = CREATE(RadioBox);
  if (!r) return 0;

  r->no       = 0;
  r->value    = 0;
  r->callback = 0;

  r->widget = gtk_frame_new(title);
  v = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(r->widget), v);
  gtk_container_set_border_width(GTK_CONTAINER(v), 4);

  cp = (char *) malloc(strlen(s) + 1);
  if (!cp) { free(r); return 0; }
  strcpy(cp, s);
  sep[0] = s[0]; sep[1] = 0;
  p = strtok(&cp[1],sep);
  rg = 0;
  while(p) {
    r->o[r->no] = gtk_radio_button_new_with_label(rg, p);
    gtk_box_pack_start(GTK_BOX(v), r->o[r->no], FALSE, TRUE, 0);
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(r->o[r->no]));
    gtk_widget_show(r->o[r->no]);
    gtk_signal_connect(GTK_OBJECT(r->o[r->no]), "toggled",
		       GTK_SIGNAL_FUNC(radiobox_toggled),
		       (gpointer) r);
    ++(r->no);
    p = strtok(0, sep);
  }

  gtk_widget_show(v);
  gtk_widget_show(r->widget);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->o[0]),TRUE);
  return r;
}

int radiobox_get_selection(RadioBox *d) 
{
  return(d->value);
}

void radiobox_set_selection(RadioBox *r, int i) 
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->o[i]),TRUE);
  r->value = i;
}

void radiobox_set_changed_callback(RadioBox *r, void (*callback)(void*))
{
  r->callback = callback;
}

void radiobox_toggled(GtkToggleButton *t, gpointer data) {
  GtkWidget *w  = GTK_WIDGET(t);
  RadioBox  *r  = (RadioBox *) data;
  int i,j=0;

  for(i=0;i<r->no;i++)
    if (w == r->o[i])
      if (gtk_toggle_button_get_active(t)) {
	r->value = i;
	j = 1;
	break;
      }

  if (j) {
    if (r->callback)
      r->callback((void *) r);
  }
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

ViewMapCtl * vmapctl_new(char *caption, int w, int h, ViewMap *map) 
{
  ViewMapCtl *ctl;
  GtkWidget *mi;

  ctl = CREATE(ViewMapCtl);
  if (!ctl) return 0;

  strncpy(ctl->caption, caption, 31);
  ctl->caption[31] = 0;

  ctl->vmap = map;
  ctl->callback = 0;
  ctl->gc       = 0;
  ctl->pb       = 0;
  ctl->pending  = 0;

  ctl->c1 = gtk_drawing_area_new();
  gtk_widget_set_events(ctl->c1, GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON1_MOTION_MASK|
			GDK_LEAVE_NOTIFY_MASK|
			GDK_BUTTON_RELEASE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(ctl->c1), w, h);

  ctl->pl =  gtk_widget_create_pango_layout(ctl->c1, NULL);
  ctl->pfd = pango_font_description_from_string("Sans 10");
  if (ctl->pl != NULL && ctl->pfd != NULL)
    pango_layout_set_font_description(ctl->pl, ctl->pfd);

  gtk_signal_connect(GTK_OBJECT(ctl->c1), "expose_event",
		     GTK_SIGNAL_FUNC(vmapctl_expose), (gpointer) ctl);
  gtk_signal_connect(GTK_OBJECT(ctl->c1), "button_press_event",
		     GTK_SIGNAL_FUNC(vmapctl_press), (gpointer) ctl);
  gtk_signal_connect(GTK_OBJECT(ctl->c1), "motion_notify_event",
		     GTK_SIGNAL_FUNC(vmapctl_drag), (gpointer) ctl);
  gtk_signal_connect(GTK_OBJECT(ctl->c1), "button_release_event",
		     GTK_SIGNAL_FUNC(vmapctl_release), (gpointer) ctl);
  gtk_signal_connect(GTK_OBJECT(ctl->c1), "leave_notify_event",
		     GTK_SIGNAL_FUNC(vmapctl_leave), (gpointer) ctl);

  /* the popup menu */
  ctl->popup = GTK_MENU(gtk_menu_new());

#define MENUSIG(obj,func,ptr) gtk_signal_connect(GTK_OBJECT(obj),"activate", GTK_SIGNAL_FUNC(func), (gpointer) ptr)

  mi = gtk_menu_item_new_with_label("Palette Gray");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p0,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Palette Hotmetal");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p1,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Palette Spectrum");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p2,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Palette Spectrum B");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p3,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Palette Yellow-Red");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p4,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Palette Neon");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_p5,ctl); gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label("Default Range");
  gtk_menu_append(ctl->popup, mi);
  MENUSIG(mi,vmapctl_reset,ctl); gtk_widget_show(mi);

  gtk_widget_show(ctl->c1);
  ctl->widget = ctl->c1;
  return ctl;
}

void         vmapctl_set_changed_callback(ViewMapCtl *s, 
					  void (*callback)(void *))
{
  s->callback = callback;
}

gboolean vmapctl_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data)
{
  ViewMapCtl *me = (ViewMapCtl *) data;
  GtkStyle *style;
  int w,h;
  int i, j, l, x, x0, y1;
  GdkPoint t[3];
  PangoRectangle pri, prl;

  style = widget->style;
  gdk_window_get_size(widget->window, &w, &h);

  if (!me->pb) {
    me->pb = gdk_pixmap_new(widget->window, w, h, -1);
    me->gc = gdk_gc_new(me->pb);
  }

  pango_layout_set_text(me->pl, "X", -1);
  pango_layout_get_pixel_extents(me->pl, &pri, &prl);
  x0 = prl.width;
  x0 = x0 + x0/2;

  gc_color(me->gc, 0);
  gdk_draw_rectangle(me->pb, me->gc, TRUE, 0, 0, w, h);
  gc_color(me->gc, 0xffffff);

  l = strlen(me->caption);

  for(i=0;i<l;i++) {
    pango_layout_set_text(me->pl, &(me->caption[i]), 1);
    pango_layout_get_pixel_extents(me->pl, &pri, &prl);
    x = (x0 - prl.width) / 2;
    gdk_draw_layout(me->pb, me->gc, x, i*(h-4)/l, me->pl);
  }

  /* draw color range */

  for(i=1;i<h-1;i++) {
    j = ((float)(h-i)) * (me->vmap->maxval / ((float)h) );
    gc_color(me->gc, ViewMapColor(me->vmap,j));
    gdk_draw_line(me->pb, me->gc, x0, i, w, i);
  }

  for(i=0;i<16;i++) {
    gc_color(me->gc, 0xffffff);
    gdk_draw_line(me->pb, me->gc, (x0+2*w)/3, i*h/16, w, i*h/16);
    gc_color(me->gc, 0);
    gdk_draw_line(me->pb, me->gc, (x0+2*w)/3, i*h/16+1, w, i*h/16+1);
  }

  for(i=0;i<4;i++) {
    gc_color(me->gc, 0xffffff);
    gdk_draw_line(me->pb, me->gc, (x0+w)/2, i*h/4, w, i*h/4);
    gc_color(me->gc, 0);
    gdk_draw_line(me->pb, me->gc, (x0+w)/2, i*h/4+1, w, i*h/4+1);
  }

  gc_color(me->gc, 0xffffff);
  gdk_draw_line(me->pb, me->gc, x0, h/2, w, h/2);
  gc_color(me->gc, 0);
  gdk_draw_line(me->pb, me->gc, x0, h/2+1, w, h/2+1);

  /* lower limit */

  y1 = (1.0 - me->vmap->minlevel) * h;
  me->y1 = y1;

  t[0].x = (x0+w)/2;  t[0].y = y1;
  t[1].x = x0/2; t[1].y = y1-6;
  t[2].x = x0/2; t[2].y = y1+6;

  gc_color(me->gc, 0xff0000);
  gdk_draw_polygon(me->pb, me->gc, TRUE, t, 3);
  gc_color(me->gc, 0xffffff);
  gdk_draw_polygon(me->pb, me->gc, FALSE, t, 3);

  /* upper limit */

  y1 = (1.0 - me->vmap->maxlevel) * h;
  me->y2 = y1;

  t[0].x = (x0+w)/2;  t[0].y = y1;
  t[1].x = x0/2; t[1].y = y1-6;
  t[2].x = x0/2; t[2].y = y1+6;

  gc_color(me->gc, 0xff0000);
  gdk_draw_polygon(me->pb, me->gc, TRUE, t, 3);
  gc_color(me->gc, 0xffffff);
  gdk_draw_polygon(me->pb, me->gc, FALSE, t, 3);

  gdk_draw_rectangle(me->pb, me->gc, FALSE, 0, 0, w-1, h-1);
  gdk_draw_pixmap(widget->window, style->black_gc, me->pb, 0,0, 0,0, w,h);
  
  return FALSE;
}

gboolean  vmapctl_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data)
{
  GdkEventButton eb;

  eb.y      = em->y;
  eb.button = 1;
  vmapctl_press(widget, &eb, data);
  return FALSE;
}

gboolean  vmapctl_press(GtkWidget *widget, GdkEventButton *eb, gpointer data)
{
  int y,b;
  int w,h;
  float v,d1,d2;
  ViewMapCtl *me = (ViewMapCtl *) data;

  y = (int) eb->y; b = eb->button;

  if (b==1) {
    gdk_window_get_size(widget->window,&w,&h);
    v = ((float)y) / ((float)h);
    v = 1.0 - v;

    d1 = y - me->y1;
    d2 = y - me->y2;
    d1 *= d1;
    d2 *= d2;

    if (d1 < d2) {
      me->vmap->minlevel = v;
      if (me->vmap->maxlevel < me->vmap->minlevel)
	me->vmap->maxlevel = me->vmap->minlevel + 0.01;
    } else {
      me->vmap->maxlevel = v;
      if (me->vmap->minlevel > me->vmap->maxlevel)
	me->vmap->minlevel = me->vmap->maxlevel - 0.01;
    }

    if (me->vmap->maxlevel > 1.0)
      me->vmap->maxlevel = 1.0;
    if (me->vmap->minlevel > 1.0)
      me->vmap->minlevel = 1.0;
    if (me->vmap->maxlevel < 0.0)
      me->vmap->maxlevel = 0.0;
    if (me->vmap->minlevel < 0.0)
      me->vmap->minlevel = 0.0;

    ViewMapUpdate(me->vmap);
    me->pending = 1;
    refresh(me->widget);
  }

  if (b==3) {
    gtk_menu_popup(me->popup,NULL,NULL,NULL,NULL,eb->button,eb->time);
  }

  return FALSE;
}

gboolean  vmapctl_release(GtkWidget *widget, GdkEventButton *eb, 
			  gpointer data)
{
  ViewMapCtl *me = (ViewMapCtl *) data;
  if (me->pending) {
    me->pending = 0;
    if (me->callback) me->callback(me);
  }
  return FALSE;
}

gboolean  vmapctl_leave(GtkWidget *widget, GdkEventCrossing *ec,
			gpointer data)
{
  ViewMapCtl *me = (ViewMapCtl *) data;
  if (me->pending) {
    me->pending = 0;
    if (me->callback) me->callback(me);
  }
  return FALSE;
}

void vmapctl_p0(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_GRAY);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_p1(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_HOTMETAL);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_p2(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_SPECTRUM);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_p3(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_SPECTRUM_B);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_p4(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_YELLOW_RED);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_p5(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  ViewMapLoadPalette(me->vmap, VMAP_NEON);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

void vmapctl_reset(GtkWidget *w, gpointer data) {
  ViewMapCtl *me = (ViewMapCtl *) data;
  me->vmap->minlevel = 0.0;
  me->vmap->maxlevel = 1.0;
  ViewMapUpdate(me->vmap);
  refresh(me->widget);
  if (me->callback) me->callback(me);
}

VSlide * vslide_new(char *caption, int w, int h) {
  VSlide *s;

  s = CREATE(VSlide);
  if (!s) return 0;

  strncpy(s->caption, caption, 31);
  s->caption[31] = 0;

  s->value = 0.0;
  s->callback = 0;
  s->gc       = 0;
  s->pb       = 0;

  s->widget = gtk_drawing_area_new();
  gtk_widget_set_events(s->widget, GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|GDK_BUTTON1_MOTION_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(s->widget), w, h);

  s->pl =  gtk_widget_create_pango_layout(s->widget, NULL);
  s->pfd = pango_font_description_from_string("Sans 10");
  if (s->pl != NULL && s->pfd != NULL)
    pango_layout_set_font_description(s->pl, s->pfd);

  gtk_signal_connect(GTK_OBJECT(s->widget), "expose_event",
		     GTK_SIGNAL_FUNC(vslide_expose), (gpointer) s);
  gtk_signal_connect(GTK_OBJECT(s->widget), "button_press_event",
		     GTK_SIGNAL_FUNC(vslide_press), (gpointer) s);
  gtk_signal_connect(GTK_OBJECT(s->widget), "motion_notify_event",
		     GTK_SIGNAL_FUNC(vslide_drag), (gpointer) s);

  gtk_widget_show(s->widget);
  return s;
}

void vslide_set_value(VSlide *s, float val) {
  if (val < 0.0) val = 0.0;
  if (val > 1.0) val = 1.0;
  s->value = val;
  refresh(s->widget);
}

float vslide_get_value(VSlide *s) {
  return(s->value);
}

void vslide_set_changed_callback(VSlide *s, void (*callback)(void *)) {
  s->callback = callback;
}

gboolean  vslide_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data) {
  int y;
  int w,h;
  float v;
  VSlide *me = (VSlide *) data;
  if (em->state & GDK_BUTTON1_MASK) {
    y = (int) em->y;
    gdk_window_get_size(widget->window,&w,&h);
    v = ((float)y) / ((float)h);
    me->value = 1.0 - v;
    if (me->callback) me->callback(me);
    refresh(me->widget);
  }
  return FALSE;
}

gboolean  vslide_press(GtkWidget *widget, GdkEventButton *eb, gpointer data) {
  int y,b;
  int w,h;
  float v;
  VSlide *me = (VSlide *) data;

  y = (int) eb->y; b = eb->button;

  if (b==1) {
    gdk_window_get_size(widget->window,&w,&h);
    v = ((float)y) / ((float)h);
    me->value = 1.0 - v;
    if (me->callback) me->callback(me);
    refresh(me->widget);
  }
  if (b==3) {
    me->value = 0.50;
    if (me->callback) me->callback(me);
    refresh(me->widget);
  }

  return FALSE;

}

gboolean vslide_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data) 
{
  VSlide *me = (VSlide *) data;
  GtkStyle *style;
  int w,h;
  int i, l, x, x0, y1;
  GdkPoint t[3];
  PangoRectangle pri, prl;

  style = widget->style;
  gdk_window_get_size(widget->window, &w, &h);

  if (!me->pb) {
    me->pb = gdk_pixmap_new(widget->window, w, h, -1);
    me->gc = gdk_gc_new(me->pb);
  }

  pango_layout_set_text(me->pl, "X", -1);
  pango_layout_get_pixel_extents(me->pl, &pri, &prl);
  x0 = prl.width;
  x0 = x0 + x0/2;

  gc_color(me->gc, 0);
  gdk_draw_rectangle(me->pb, me->gc, TRUE, 0, 0, w, h);
  gc_color(me->gc, 0xffffff);

  l = strlen(me->caption);

  for(i=0;i<l;i++) {
    pango_layout_set_text(me->pl, &(me->caption[i]), 1);
    pango_layout_get_pixel_extents(me->pl, &pri, &prl);
    x = (x0 - prl.width) / 2;
    gdk_draw_layout(me->pb, me->gc, x, i*(h-4)/l, me->pl);
  }

  for(i=1;i<h-1;i++) {
    gc_color(me->gc, gray( (224 * (h-i)) / h ));
    gdk_draw_line(me->pb, me->gc, x0, i, w, i);
  }

  y1 = (1.0 - me->value) * h;

  t[0].x = (x0+w)/2;  t[0].y = y1;
  t[1].x = x0/2; t[1].y = y1-6;
  t[2].x = x0/2; t[2].y = y1+6;

  for(i=0;i<16;i++) {
    gc_color(me->gc, 0xffffff);
    gdk_draw_line(me->pb, me->gc, (x0+2*w)/3, i*h/16, w, i*h/16);
    gc_color(me->gc, 0);
    gdk_draw_line(me->pb, me->gc, (x0+2*w)/3, i*h/16+1, w, i*h/16+1);
  }

  for(i=0;i<4;i++) {
    gc_color(me->gc, 0xffffff);
    gdk_draw_line(me->pb, me->gc, (x0+w)/2, i*h/4, w, i*h/4);
    gc_color(me->gc, 0);
    gdk_draw_line(me->pb, me->gc, (x0+w)/2, i*h/4+1, w, i*h/4+1);
  }

  gc_color(me->gc, 0xffffff);
  gdk_draw_line(me->pb, me->gc, x0, h/2, w, h/2);
  gc_color(me->gc, 0);
  gdk_draw_line(me->pb, me->gc, x0, h/2+1, w, h/2+1);

  gc_color(me->gc, 0xff0000);
  gdk_draw_polygon(me->pb, me->gc, TRUE, t, 3);
  gc_color(me->gc, 0xffffff);
  gdk_draw_polygon(me->pb, me->gc, FALSE, t, 3);


  gdk_draw_rectangle(me->pb, me->gc, FALSE, 0, 0, w-1, h-1);
  gdk_draw_pixmap(widget->window, style->black_gc, me->pb, 0,0, 0,0, w,h);
  
  return FALSE;
}

void plain_dialog_ok(GtkWidget *w, gpointer data) {
  gtk_widget_destroy((GtkWidget *)data);
}

void plain_dialog(GtkWindow *parent, 
		  char *title, char *text, char *okcaption) 
{
  GtkWidget *dlg,*v,*l,*hs,*h,*ok;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);
  
  v=gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(dlg),v);

  l = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(l), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(v), l, FALSE, TRUE, 2);

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  h=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h), 5);
  gtk_box_pack_start(GTK_BOX(v),h,FALSE,TRUE,2);

  ok = gtk_button_new_with_label(okcaption);

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(h), ok, TRUE, TRUE, 0);

  gtk_widget_show(ok);
  gtk_widget_show(h);
  gtk_widget_show(hs);
  gtk_widget_show(l);
  gtk_widget_show(v);
  gtk_widget_show(dlg);

  gtk_widget_grab_default(ok);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(plain_dialog_ok), (gpointer) dlg);
}

void plain_dialog_with_icon(GtkWindow *parent,
			    char *title, char *text, char *okcaption,
			    char **xpm)
{
  GtkWidget *dlg,*v,*vh,*vhv,*l,*hs,*h,*ok;
  GtkWidget *icon;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
  gtk_container_set_border_width(GTK_CONTAINER(dlg),6);
  
  v=gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(dlg),v);

  vh = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(v), vh, TRUE, TRUE, 2);
  vhv = gtk_vbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vh), vhv, FALSE, TRUE, 2);

  icon = ivs_pixmap_new(GTK_WIDGET(parent), xpm);
  gtk_box_pack_start(GTK_BOX(vhv), icon, FALSE, TRUE, 2);

  l = gtk_label_new(text);
  gtk_label_set_justify(GTK_LABEL(l), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(vh), l, FALSE, TRUE, 2);

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  h=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(h), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(h), 5);
  gtk_box_pack_start(GTK_BOX(v),h,FALSE,TRUE,2);

  ok = gtk_button_new_with_label(okcaption);

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(h), ok, TRUE, TRUE, 0);

  gtk_widget_show(ok);
  gtk_widget_show(h);
  gtk_widget_show(hs);
  gtk_widget_show(l);
  gtk_widget_show(icon);
  gtk_widget_show(vhv);
  gtk_widget_show(vh);
  gtk_widget_show(v);
  gtk_widget_show(dlg);

  gtk_widget_grab_default(ok);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(plain_dialog_ok), (gpointer) dlg);
}

gboolean colortag_expose(GtkWidget *widget,GdkEventExpose *ee,
			 gpointer data)
{
  ColorTag *c = (ColorTag *) data;

  if (!c->gc) {
    c->gc = gdk_gc_new(widget->window);
  }
  gc_color(c->gc, c->color);
  gdk_draw_rectangle(widget->window,c->gc,TRUE,0,0,c->w,c->h);
  return FALSE;
}

ColorTag * colortag_new(int color, int w, int h) {
  ColorTag *c;

  c = CREATE(ColorTag);
  if (!c) return 0;

  MemSet(c,0,sizeof(ColorTag));

  c->w     = w;
  c->h     = h;
  c->color = color;
  c->gc    = 0;

  c->widget = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(c->widget),w,h);
  
  gtk_signal_connect(GTK_OBJECT(c->widget),"expose_event",
		     GTK_SIGNAL_FUNC(colortag_expose), (gpointer) c);

  gtk_widget_show(c->widget);
  return c;
}

/* smenu */

SMenuItem *smenuitem_new(char *caption, int sectitle, int id) {
  SMenuItem *mi;

  mi = CREATE(SMenuItem);
  if (!mi) return 0;

  mi->caption = malloc(strlen(caption) + 1);
  if (!mi->caption) {
    free(mi);
    return 0;
  }

  strcpy(mi->caption, caption);
  mi->sectitle = sectitle;
  mi->id       = id;

  return mi;
}

void smenuitem_destroy(SMenuItem *mi) {
  if (mi) {
    if (mi->caption) free(mi->caption);
    free(mi);
  }
}

gboolean smenu_expose(GtkWidget *widget,GdkEventExpose *ee,
		      gpointer data);
gboolean smenu_bdown(GtkWidget *widget,GdkEventButton *be, gpointer data);
gboolean smenu_hover(GtkWidget *widget,GdkEventMotion *be, gpointer data);
gboolean smenu_leave(GtkWidget *widget,GdkEventCrossing *be, gpointer data);

SMenu * smenu_new() {
  SMenu *sm;
  sm = CREATE(SMenu);
  if (!sm) return 0;

  sm->nitems    = 0;
  sm->cap       = 32;
  sm->items     = (SMenuItem **) malloc(sizeof(SMenuItem *) * 32);
  sm->selection = 0;

  sm->lastw = 1;
  sm->lasth = 1;
  sm->hover = -1;

  sm->widget = gtk_drawing_area_new();
  gtk_widget_set_events(sm->widget,GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|
                        GDK_POINTER_MOTION_MASK|GDK_LEAVE_NOTIFY_MASK);

  sm->db = NULL;
  sm->gc = NULL;

  sm->pl  = gtk_widget_create_pango_layout(sm->widget, NULL);
  sm->pfd = pango_font_description_from_string("Sans 10");
  if (sm->pl != NULL && sm->pfd != NULL)
    pango_layout_set_font_description(sm->pl, sm->pfd);

  sm->bg[0] = 0xc0c0c0;
  sm->bg[1] = 0xe0e0e0;
  sm->bg[2] = 0x606060;
  sm->bg[3] = 0x000000;

  sm->fg[0] = 0;
  sm->fg[1] = 0;
  sm->fg[2] = 0xffffff;
  sm->fg[3] = 0xffffff;

  sm->callback = 0;

  gtk_signal_connect(GTK_OBJECT(sm->widget),"expose_event",
		     GTK_SIGNAL_FUNC(smenu_expose), (gpointer) sm);
  gtk_signal_connect(GTK_OBJECT(sm->widget),"button_press_event",
		     GTK_SIGNAL_FUNC(smenu_bdown), (gpointer) sm);
  gtk_signal_connect(GTK_OBJECT(sm->widget),"motion_notify_event",
		     GTK_SIGNAL_FUNC(smenu_hover), (gpointer) sm);
  gtk_signal_connect(GTK_OBJECT(sm->widget),"leave_notify_event",
		     GTK_SIGNAL_FUNC(smenu_leave), (gpointer) sm);

  return sm;
}

void smenu_set_callback(SMenu *sm, void (*callback)(SMenu *,int)) {
  sm->callback = callback;
}

GtkWidget * smenu_get_widget(SMenu *sm) {
  return(sm->widget);
}

void  smenu_set_colors(SMenu *sm, 
		       int bg_off,   int fg_off,
		       int bg_hover, int fg_hover,
		       int bg_on,    int fg_on,
		       int bg_title, int fg_title) {
  sm->bg[0] = bg_off;
  sm->bg[1] = bg_hover;
  sm->bg[2] = bg_on;
  sm->bg[3] = bg_title;

  sm->fg[0] = fg_off;
  sm->fg[1] = fg_hover;
  sm->fg[2] = fg_on;
  sm->fg[3] = fg_title;
}

void smenu_append_item(SMenu *sm, SMenuItem *mi) {
  if (sm->cap == sm->nitems) {
    sm->cap += 64;
    sm->items = (SMenuItem **) realloc(sm->items, 
				       sm->cap * sizeof(SMenuItem *));    
  }
  sm->items[sm->nitems++] = mi;
}

int smenu_get_selection_id(SMenu *sm) {

  return(sm->items[sm->selection]->id);
}

void smenu_set_selection_id(SMenu *sm, int id) {
  int i;

  for(i=0;i<sm->nitems;i++)
    if (sm->items[i]->id == id) {
      sm->selection = i;
      break;
    }
}

void smenu_repaint(SMenu *sm) {
  gtk_widget_queue_draw(sm->widget);
}

/* private callbacks */

gboolean smenu_expose(GtkWidget *widget,GdkEventExpose *ee,
		      gpointer data) 
{
  SMenu *sm = (SMenu *) data;
  int i,mx,my,w,h;
  PangoRectangle pri, prl;

  /* allocate drawing buffer if necessary (first call only) */
  if (sm->db==NULL) {

    mx=my=w=h=0;
    for(i=0;i<sm->nitems;i++) {
      pango_layout_set_text(sm->pl, sm->items[i]->caption, -1);
      pango_layout_get_pixel_extents(sm->pl, &pri, &prl);
      w = prl.width;
      h = prl.height;
      if (w > mx) mx = w;
      if (h > my) my = h;
    }
    w = mx + 32;
    h = sm->nitems * (my+4);
    sm->lastw = mx + 32;
    sm->lasth = my+4;
    
    gtk_drawing_area_size(GTK_DRAWING_AREA(sm->widget), w, h);
    sm->db = gdk_pixmap_new(widget->window,w,h,-1);
    sm->gc = gdk_gc_new(sm->db);
    sm->aw = w;
    sm->ah = h;
  }

  /* if size allocation is larger than last size, 
     enlarge offscreen buffer */

  gdk_window_get_size(widget->window,&w,&h);

  if (w > sm->aw || h > sm->ah) {
    gdk_gc_destroy(sm->gc);
    gdk_pixmap_unref(sm->db);

    sm->aw = w;
    sm->ah = h;
    sm->db = gdk_pixmap_new(widget->window,w,h,-1);
    sm->gc = gdk_gc_new(sm->db);

    sm->lastw = w;
  } else {
    sm->lastw = w;
  }

  /* draw menu in offscreen buffer */

  w = sm->lastw;
  h = sm->lasth;

  for(i=0;i<sm->nitems;i++) {
    pango_layout_set_text(sm->pl, sm->items[i]->caption, -1);
    pango_layout_get_pixel_extents(sm->pl, &pri, &prl);
    mx = prl.width;
    my = prl.height;

    gc_color(sm->gc, (sm->hover == i) ? 
	     sm->bg[1] : sm->bg[0]);
    if (sm->selection == i)     gc_color(sm->gc, sm->bg[2]);
    if (sm->items[i]->sectitle) gc_color(sm->gc, sm->bg[3]);

    gdk_draw_rectangle(sm->db, sm->gc, TRUE, 0, h*i, w, h);

    gc_color(sm->gc, (sm->hover == i) ? 
	     sm->fg[1] : sm->fg[0]);
    if (sm->selection == i)     gc_color(sm->gc, sm->fg[2]);
    if (sm->items[i]->sectitle) gc_color(sm->gc, sm->fg[3]);

    if (sm->items[i]->sectitle) {
      gdk_draw_layout(sm->db, sm->gc, 16, (h*(i+0)) + (h-my)/2, sm->pl);
    } else {
      gdk_draw_layout(sm->db, sm->gc, (w-mx)/2, (h*(i+0)) + (h-my)/2, sm->pl);
    }
  }
  
  /* copy buffer to actual window */
  gdk_draw_pixmap(widget->window, widget->style->black_gc, 
		  sm->db,
		  0,0,0,0,sm->aw, sm->ah);

  return FALSE;
}

gboolean smenu_bdown(GtkWidget *widget,GdkEventButton *be, gpointer data)
{
  SMenu *sm = (SMenu *) data;

  int y;
  y = (int) (be->y);
  y /= sm->lasth;

  if (y>=0 && y<sm->nitems)
    if (!sm->items[y]->sectitle) {
      sm->selection = y;
      smenu_repaint(sm);
      if (sm->callback != 0) {
	(*(sm->callback))(sm, sm->items[y]->id);
      }
    }

  return FALSE;
}

gboolean smenu_hover(GtkWidget *widget,GdkEventMotion *be, gpointer data)
{
  SMenu *sm = (SMenu *) data;
  int y;
  y = (int) (be->y);
  y /= sm->lasth;
  sm->hover = y;
  smenu_repaint(sm);

  return FALSE;
}

gboolean smenu_leave(GtkWidget *widget,GdkEventCrossing *be, gpointer data)
{
  SMenu *sm = (SMenu *) data;
  sm->hover = -1;
  smenu_repaint(sm);
  return FALSE;
}

gboolean rcurve_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data);

RCurve * rcurve_new() {
  RCurve *r;

  r = CREATE(RCurve);

  r->widget = gtk_drawing_area_new();
  gtk_widget_set_events(r->widget,GDK_EXPOSURE_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(r->widget), 256, 192);

  r->func      = 0;
  r->histogram = 0;
  r->gc = 0;

  r->pl =  gtk_widget_create_pango_layout(r->widget, NULL);
  r->pfd = pango_font_description_from_string("Sans 10");
  if (r->pl != NULL && r->pfd != NULL)
    pango_layout_set_font_description(r->pl, r->pfd);

  gtk_signal_connect(GTK_OBJECT(r->widget), "expose_event",
		     GTK_SIGNAL_FUNC(rcurve_expose),
		     (gpointer)(r));


  gtk_widget_show(r->widget);
  return r;
}

void rcurve_set_function(RCurve *r, i16_t *f, int xmin, int xmax) {
  int i,ymin,ymax;
  r->func = f;
  r->xmin = xmin;
  r->xmax = xmax;

  if (!f) return;

  ymax = f[xmin];
  ymin = f[xmin];

  for(i=0;i<=xmax;i++) {
    if (f[i] < ymin) ymin = f[i];
    if (f[i] > ymax) ymax = f[i];
  }

  r->ymin = ymin;
  r->ymax = ymax;

  refresh(r->widget);
}

void rcurve_set_histogram(RCurve *r, i32_t *h) {
  int i,j,k;
  r->histogram = h;
  if (!h) return;
  r->hmax = h[0];
  j = 0;
  for(i=1;i<SDOM;i++)
    if (h[i] > r->hmax) {
      r->hmax = h[i];
      j = i;
    }
  r->hemax = r->hmax;

  k = h[j==0?1:0];
  for(i=0;i<SDOM;i++)
    if (i!=j && h[i] > k)
      k = h[i];

  if (r->hmax / k > 3)
    r->hemax = (r->hmax + 3*k) / 4;
}

gboolean rcurve_expose(GtkWidget *widget,GdkEventExpose *ee,
		       gpointer data)
{
  RCurve *r = (RCurve *) data;
  int i,j,k,t,s;
  int w,h;
  float ly,cy,cx;
  float yamp, xamp;
  float fw,fh;
  PangoRectangle pri, prl;
  int cw;
  char z[64];

  gdk_window_get_size(widget->window, &w, &h);

  if (!r->gc)
    r->gc = gdk_gc_new(widget->window);

  gc_color(r->gc, 0xe0e0e0);
  gdk_draw_rectangle(widget->window, r->gc, TRUE, 0,0,w,h);
  gc_color(r->gc, 0);
  gdk_draw_rectangle(widget->window, r->gc, FALSE, 0,0,w-1,h-1);

  if (!r->func) return FALSE;

  xamp = (float) (r->xmax - r->xmin + 1);
  yamp = (float) (r->ymax - r->ymin + 1);

  fw = (float) w;
  fh = (float) h - 3;

  // draw histogram (if available)

  if (r->histogram) {
    gc_color(r->gc, 0xb0b0b0);

    for(i=0;i<w;i++) {
      cx = (float) i;
      cx /= fw;
      cx *= xamp;
      cx += r->xmin;
      j = (int) cx;

      cx = (float) (i+1);
      cx /= fw;
      cx *= xamp;
      cx += r->xmin;
      k = (int) cx;
      if (k!=j) --k;

      s = 0;
      for(t=j;t<=k;t++)
	s += r->histogram[t];

      cy = (float) s;
      cy /= (float) (k-j+1);
      cy /= (float) (r->hemax);

      if (cy > 1.0)
	gc_color(r->gc, 0xffb0b0);
      else
	gc_color(r->gc, 0xb0b0b0);

      cy *= (float) (h - 3);

      gdk_draw_line(widget->window,r->gc,i,h-1,i,h-((int)cy)-1);
    }
  }

  // draw grid

  
  gc_color(r->gc, 0x606060);

  gdk_draw_line(widget->window, r->gc, 0,h/2,w,h/2);
  gdk_draw_line(widget->window, r->gc, 0,h/4,w,h/4);
  gdk_draw_line(widget->window, r->gc, 0,3*h/4,w,3*h/4);

  pango_layout_set_text(r->pl, "0123456789", -1);
  pango_layout_get_pixel_extents(r->pl, &pri, &prl);

  sprintf(z,"%d",r->ymax);
  pango_layout_set_text(r->pl, z, -1);
  gdk_draw_layout(widget->window, r->gc, 5, 2, r->pl);

  sprintf(z,"%d",r->ymin+3*(r->ymax-r->ymin)/4);
  pango_layout_set_text(r->pl, z, -1);
  gdk_draw_layout(widget->window, r->gc, 5, 2+h/4, r->pl);

  sprintf(z,"%d",r->ymin+(r->ymax-r->ymin)/2);
  pango_layout_set_text(r->pl, z, -1);
  gdk_draw_layout(widget->window, r->gc, 5, 2+h/2, r->pl);

  sprintf(z,"%d",r->ymin+(r->ymax-r->ymin)/4);
  pango_layout_set_text(r->pl, z, -1);
  gdk_draw_layout(widget->window, r->gc, 5, 2+3*h/4, r->pl);

  gdk_draw_line(widget->window, r->gc, w/2,0,w/2,h);
  gdk_draw_line(widget->window, r->gc, w/4,0,w/4,h);
  gdk_draw_line(widget->window, r->gc, 3*w/4,0,3*w/4,h);

  sprintf(z,"%d",r->xmax);
  pango_layout_set_text(r->pl, z, -1);
  pango_layout_get_pixel_extents(r->pl, &pri, &prl);
  cw = prl.width;
  gdk_draw_layout(widget->window, r->gc, w-cw-5, h-3, r->pl);

  sprintf(z,"%d",r->xmin+(3*(r->xmax-r->xmin))/4);
  pango_layout_set_text(r->pl, z, -1);
  pango_layout_get_pixel_extents(r->pl, &pri, &prl);
  cw = prl.width;
  gdk_draw_layout(widget->window, r->gc, (3*w/4)-cw-5, h-3, r->pl);

  sprintf(z,"%d",r->xmin+(r->xmax-r->xmin)/2);
  pango_layout_set_text(r->pl, z, -1);
  pango_layout_get_pixel_extents(r->pl, &pri, &prl);
  cw = prl.width;
  gdk_draw_layout(widget->window, r->gc, (w/2)-cw-5, h-3, r->pl);

  sprintf(z,"%d",r->xmin+(r->xmax-r->xmin)/4);
  pango_layout_set_text(r->pl, z, -1);
  pango_layout_get_pixel_extents(r->pl, &pri, &prl);
  cw = prl.width;
  gdk_draw_layout(widget->window, r->gc, (w/4)-cw-5, h-3, r->pl);

  // plot function

  gc_color(r->gc, 0x000080);

  ly = (float) (r->func[r->xmin] - r->ymin);
  ly /= yamp;
  ly = fh * (1.0 - ly);

  for(i=1;i<w;i++) {

    cx = (float) i;
    cx /= fw;
    cx *= xamp;
    cx += r->xmin;

    j = (int) cx;
    if (j > r->xmax) j=r->xmax;

    cy = (float) (r->func[j] - r->ymin);
    cy /= yamp;
    cy = fh * (1.0 - cy);

    gdk_draw_line(widget->window,r->gc,i-1,(int)(ly)+1,i,(int)(cy)+1);
    ly = cy;
  }
  
  gc_color(r->gc, 0);
  gdk_draw_rectangle(widget->window, r->gc, FALSE, 0,0,w-1,h-1);

  return FALSE;
}
