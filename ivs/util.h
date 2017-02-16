

#ifndef IVS_UTIL_H
#define IVS_UTIL_H 1

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libip.h>

#include "msgbox.h"

#define CREATE(t)  ((t *)malloc(sizeof(t)))
#define DESTROY(a) free(a)

void set_style_bg(GtkStyle *s, int color);
void gc_color(GdkGC *gc, int color);
void gc_clip(GdkGC *gc, int x, int y, int w, int h);
void refresh(GtkWidget *w);

void set_icon(GtkWidget *widget, char **xpm, char *text);

GtkWidget * icon_button(GtkWidget *parent, char **xpm, char *text);

void plain_dialog(GtkWindow *parent, 
		  char *title, char *text, char *okcaption);

void plain_dialog_with_icon(GtkWindow *parent,
			    char *title, char *text, char *okcaption,
			    char **xpm);

typedef struct _graphics {
  GdkPixmap *w;
  GdkGC     *gc;
  int        W,H;
} Graphics;

Graphics * graphics_validate_buffer(GdkWindow *w, Graphics *prev);
void       graphics_commit_buffer(Graphics *g, GdkWindow *w, GdkGC *gc);

void shadow_text(GdkDrawable *w, PangoLayout *pl, GdkGC *gc, int x, int y,
		 int c0, int c1, char *text);
void shielded_text(GdkDrawable *w, PangoLayout *pl, GdkGC *gc, int x, int y,
		   int c0, int c1, char *text);

void bevel_box(GdkDrawable *d, GdkGC *gc, int color, int x, int y,
	       int w, int h);

GtkWidget * ivs_pixmap_new(GtkWidget *w, char **xpm);
void        ivs_condshow(int condition, GtkWidget *w);

char *      ivs_short_filename(char *x);
void        ivs_safe_short_filename(char *x, char *dest,int maxchar);

#define GF_ENSURE_WRITEABLE 0x01
#define GF_ENSURE_READABLE  0x02
#define GF_NONE             0
int  ivs_get_filename(GtkWidget *parent, char *title, char *dest, int flags);

void draw_cuboid_fill(Cuboid *c, GdkDrawable *dest, GdkGC *gc,
		      int fillcolor);
void draw_cuboid_frame(Cuboid *c, GdkDrawable *dest, GdkGC *gc,
		       int hiddencolor, int edgecolor, int drawhidden);

void draw_clip_control(Point *p, GdkDrawable *dest, GdkGC *gc,
		       int bgcolor, int edgecolor, 
		       int h1fill, int h1edge,
		       int h2fill, int h2edge);

#define PICKER_STYLE_NOTHING 0
#define PICKER_STYLE_BOX     1
#define PICKER_STYLE_NUMBER  2
#define PICKER_STYLE_LABEL   3
#define PICKER_STYLE_EYE     4

typedef struct _picker {
  GtkWidget *widget;
  GdkGC     *gc;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  int  down[10];
  int  color[10];
  int  de;
  int  exclusive;
  int  style; /* 0: nothing  1: little box  2: number */
  int  value;
  void (*callback)(void *);
} Picker;

Picker * picker_new(int exclusive, int style);
void     picker_repaint(Picker *p);
void     picker_set_color(Picker *p, int i, int color);
void     picker_set_down(Picker *p, int i, int down);
int      picker_get_down(Picker *p, int i);
int      picker_get_selection(Picker *p);
void     picker_set_changed_callback(Picker *p, void (*callback)(void*));

typedef struct _dropbox {
  GtkWidget *widget;
  int        value;
  GtkWidget *o[32];
  int        no;
  void (*callback)(void *);
} DropBox;

/* s[0] = separator, s[1]...s[n] = list of options separated by s[0]. */
DropBox * dropbox_new(char *s);
int       dropbox_get_selection(DropBox *d);
void      dropbox_set_selection(DropBox *d, int i);
void      dropbox_set_changed_callback(DropBox *d, void (*callback)(void*));

typedef struct _radiobox {
  GtkWidget *widget;
  int        value;
  GtkWidget *o[32];
  int        no;
  void (*callback)(void *);
} RadioBox;

/* s[0] = separator, s[1]...s[n] = list of options separated by s[0]. */
RadioBox * radiobox_new(char *title, char *s);
int        radiobox_get_selection(RadioBox *d);
void       radiobox_set_selection(RadioBox *d, int i);
void       radiobox_set_changed_callback(RadioBox *d, void (*callback)(void*));

typedef struct _vslide {
  GtkWidget *widget;
  GdkGC     *gc;
  GdkPixmap *pb;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  char       caption[32];
  float      value;
  void (*callback)(void *);
} VSlide;

VSlide * vslide_new(char *caption, int w, int h);
void     vslide_set_value(VSlide *s, float val);
float    vslide_get_value(VSlide *s);
void     vslide_set_changed_callback(VSlide *s, void (*callback)(void *));

typedef struct _vmapctl {
  GtkWidget *widget, *c1;
  GdkGC     *gc;
  GdkPixmap *pb;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  char       caption[32];
  ViewMap   *vmap;
  int        y1, y2;
  int        pending;
  GtkMenu   *popup;
  void (*callback)(void *);
} ViewMapCtl;

ViewMapCtl * vmapctl_new(char *caption, int w, int h, ViewMap *map);
void         vmapctl_set_changed_callback(ViewMapCtl *s, 
					  void (*callback)(void *));



typedef struct _ctag {
  GtkWidget *widget;
  GdkGC     *gc;
  int        color;
  int        w,h;
} ColorTag;

ColorTag * colortag_new(int color, int w, int h);

#define max2(a,b) (((a)>(b))?(a):(b))
#define max3(a,b,c) (((a)>(b))?(max2(a,c)):(max2(b,c)))

typedef struct _smenuitem {
  char *caption;
  int   sectitle;
  int   id;
} SMenuItem;

SMenuItem *smenuitem_new(char *caption, int sectitle, int id);
void       smenuitem_destroy(SMenuItem *mi);

typedef struct _smenu {
  GtkWidget *widget;
  GdkPixmap *db;
  GdkGC     *gc;
  PangoLayout *pl;
  PangoFontDescription *pfd;

  int        bg[4]; /* off hover on sectitle */
  int        fg[4]; /* off hover on sectitle */

  int nitems, cap;
  int selection;

  int lastw, lasth;
  int hover;
  int aw,ah;

  SMenuItem **items;

  void (*callback)(struct _smenu *, int);
} SMenu;

SMenu *     smenu_new();
void        smenu_set_callback(SMenu *sm, void (*callback)(SMenu *,int));
GtkWidget * smenu_get_widget(SMenu *sm);
void        smenu_set_colors(SMenu *sm, 
			     int bg_off,   int fg_off,
			     int bg_hover, int fg_hover,
			     int bg_on,    int fg_on,
			     int bg_title, int fg_title);
void        smenu_append_item(SMenu *sm, SMenuItem *mi);
int         smenu_get_selection_id(SMenu *sm);     
void        smenu_set_selection_id(SMenu *sm, int id);
void        smenu_repaint(SMenu *sm);

typedef struct _rcurve {
  GtkWidget *widget;
  PangoLayout *pl;
  PangoFontDescription *pfd;
  i16_t *func;
  i32_t *histogram;

  int    xmin,xmax;
  int    ymin,ymax;
  int    hmax, hemax;

  GdkGC *gc;
} RCurve;

RCurve * rcurve_new();
void     rcurve_set_function(RCurve *r, i16_t *f, int xmin, int xmax);
void     rcurve_set_histogram(RCurve *r, i32_t *h);

#endif
