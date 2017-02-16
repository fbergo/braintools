
#ifndef UTIL_H
#define UTIL_H 1

#include <libip.h>
#include <gtk/gtk.h>
#include <pthread.h>

#define CREATE(t)  ((t *)malloc(sizeof(t)))
#define DESTROY(a) free(a)

void gc_color(GdkGC *gc, int color);
void refresh(GtkWidget *w);
void disable(GtkWidget *w);
void enable(GtkWidget *w);

typedef struct _box {
  int x,y,w,h;
} Box;

void BoxAssign(Box *b, int x,int y,int w,int h);
int  InBox(int x,int y, Box *b);
int  InBoxXYWH(int x,int y, int x0,int y0,int w,int h);

void util_set_fg(GtkWidget *w, int fg);
void util_set_bg(GtkWidget *w, int bg);
void util_set_icon(GtkWidget *w, const char **xpm, char *text);

typedef struct _toolbar {
  int n;
  int cols,rows;
  int w,h;

  int tmpcount;

  Glyph **img;
  char  **tip;
  CImg  *buf;
  GtkWidget *widget;
  int bgi,bga,fgi,fga;

  int selected;
  int *enabled;

  GtkWidget *tips, *tipc;
  PangoLayout *pl;
  int hover;
  int thide;

  void (*callback)(void *);
} Toolbar;

Toolbar * ToolbarNew(int nitems,int cols,int w,int h);
void      ToolbarAddButton(Toolbar *t,Glyph *g,const char *tip);
void      ToolbarSetSelection(Toolbar *t, int i);
int       ToolbarGetSelection(Toolbar *t);
void      ToolbarSetCallback(Toolbar *t,  void (*callback)(void *));
void      ToolbarDisable(Toolbar *t, int i,int ns);
void      ToolbarEnable(Toolbar *t, int i);

typedef struct _slider {
  Glyph *icon;
  CImg *buf;
  GtkWidget *widget;
  float value, defval;

  int bg,fg,fill;

  int grabbed;
  int cont;

  void (*callback)(void *);
} Slider;

Slider *SliderNew(Glyph *icon, int h);
Slider *SliderNewFull(Glyph *icon, int w, int h);
void    SliderSetCallback(Slider *s, void (*callback)(void *));
float   SliderGetValue(Slider *s);
void    SliderSetValue(Slider *s, float value);

typedef struct _xlabel {
  SFont *font;
  int bg,fg;
  char *text;
  GtkWidget *widget;
} Label;

Label * LabelNew(int bg,int fg,SFont *font);
void    LabelSetText(Label *l, char * text);

typedef struct _xcheck {
  int bg, fg;
  int state;
  GtkWidget *widget;
  int grabbed;
  void (*callback)(void *);
} CheckBox;

CheckBox * CheckBoxNew(int bg,int fg,int state);
int        CheckBoxGetState(CheckBox *cb);
void       CheckBoxSetState(CheckBox *cb, int state);
void       CheckBoxSetCallback(CheckBox *cb,   void (*callback)(void *));

typedef struct _xlock {
  int bg, fg;
  int state;
  GtkWidget *widget;
  int grabbed;
  void (*callback)(void *);
} LockBox;

LockBox * LockBoxNew(int bg,int fg,int state);
int       LockBoxGetState(LockBox *b);
void      LockBoxSetState(LockBox *b, int state);
void      LockBoxSetCallback(LockBox *c, void (*callback)(void *));

typedef struct _xbutton {
  int bg, fg;
  char *text;
  SFont *font;
  GtkWidget *widget;
  int grabbed;
  void (*callback)(void *);
} Button;

Button * ButtonNew(int bg,int fg, SFont *f, char *text);
Button * ButtonNewFull(int bg,int fg, SFont *f, char *text, int w, int h);
void     ButtonSetCallback(Button *b, void (*callback)(void *));

typedef struct _colorlabel {
  int color,border;
  GtkWidget *widget;
} ColorLabel;

ColorLabel * ColorLabelNew(int w,int h,int color,int border);
void         ColorLabelSetColor(ColorLabel *cl, int color);

typedef struct _filler {
  int color;
  GtkWidget *widget;
  GdkGC *gc;
} Filler;

Filler * FillerNew(int color);

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

#define GF_ENSURE_WRITEABLE 0x01
#define GF_ENSURE_READABLE  0x02
#define GF_NONE             0
int  util_get_filename(GtkWidget *parent, char *title, char *dest, int flags);
int  util_get_filename2(GtkWidget *parent, char *title, char *dest, int flags,
			char *asktext, int *askresult);
int  util_get_dir(GtkWidget *parent, char *title, char *dest);

int util_get_color(GtkWidget *parent, int color, char *title);
int util_get_format(GtkWidget *parent, char *title, char *formats);

GtkWidget * util_pixbutton_new(char **xpm);
GtkWidget * util_pixmap_new(GtkWidget *w, char **xpm);
GtkWidget * util_dialog_new(char *title, GtkWidget *parent, int modal,
			    GtkWidget **vbox, GtkWidget **bbox,
			    int dw,int dh);
GtkWidget * util_rlabel(char *text, GtkWidget **L);
GtkWidget * util_llabel(char *text, GtkWidget **L);

GdkCursor * util_cursor_new(GtkWidget *w,unsigned char *xbm,unsigned char *mask,int x,int y);

int sqdist(int x1,int y1,int x2,int y2);

char  *str2utf8(const char *src);
char  *str2latin1(const char *src);

typedef pthread_mutex_t Mutex;

Mutex  *MutexNew();
void    MutexDestroy(Mutex *m);
void    MutexLock(Mutex *m); /* may block */
void    MutexUnlock(Mutex *m);
int     MutexTryLock(Mutex *m); /* never blocks, 0 on success, <0 on failure */

#define IS_IN_RANGE(v,min,max) ( (v)>=(min) && (v)<=(max) )

void    resource_path(char *dest,const char *src, int ndest);

float   angle(int x1,int y1,int x0,int y0);

#define AV_PATHLEN 512

#endif
