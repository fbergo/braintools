#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>
#include <unistd.h>
#include <list>
#include <queue>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "brain_imaging.h"
#include "braintag_ui.h"
#include "icon_cinapce.xpm"

#define VERSION "1.8c"

using namespace std;

#define MSG_NO_VOLUME "* No volume loaded."
#define MSG_NO_BRAIN  "* Brain not segmented."
#define MSG_NO_EDT    "* Planarization requires layering."
#define MSG_BUSY      "* Processing queue busy, retry later."
#define MSG_NO_TAGS   "* No tags loaded."

char appcfg[1024];
GtkWidget *mw, *canvas;
GdkGC *gc=NULL;
PangoLayout *pl=NULL;
PangoFontDescription *sans10 = NULL, *sans10b = NULL;
int bgcolor = 0xd0c5a0, errcolor = 0xffc0c0;

int cmd_prepare = 0;
int cmd_quit = 0;

bool Checkers = false;
int  CheckerSize = 16;

list<Window *> wlist;
Window *hover = NULL;
MessageWindow  *msg;
OrthogonalView *v2d = NULL;
ObjectView *vbrain = NULL;
EDTView    *vedt1  = NULL;
PlanarView *vtex   = NULL;
BrainToolbar *tools = NULL;
FeatureView *fv = NULL;

/* volume data */
// orig, binbrain, skin, edt, ptex, pmap, rtag, ptag
char *inputname = NULL;
char *names[8];

// orig, binbrain, skin, edt, ptex+pmap, rtag+ptag
volatile bool vloaded[6] = { false, false, false, false, false, false };
// v2d, v2d(seg,skin), v2d(orig, seg rotated), edt, ptex+pmap, rtag+ptag
volatile bool rupdate[6] = { false,false,false,false,false,false };
Volume<int>  *orig = NULL;
Volume<char> *binbrain = NULL;
Volume<char> *binskin  = NULL;
Volume<int>  *edt  = NULL;
Volume<int>  *ptex = NULL;
Volume<int>  *pmap = NULL;
Volume<char> *rtag  = NULL;
Volume<char> *ptag  = NULL;
T4  align_trans;
int align_slice;
bool skip_align = false;

/* multi-thread handling */
queue<Task *> qtask;
queue<char *> qmsg;
queue<int>    qmsgc;
Mutex tmutex, msgmutex;
pthread_t tid;
gint      mid=-1;
volatile bool quitting = false;

/* callbacks */
gboolean bt_del(GtkWidget *w, GdkEvent *e, gpointer data);
void     bt_destroy(GtkWidget *w, gpointer data);
gboolean bt_key(GtkWidget *w, GdkEventKey *e,gpointer data);
gboolean bt_expose(GtkWidget *w, GdkEventExpose *e, gpointer data);
gboolean bt_press(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean bt_release(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean bt_drag(GtkWidget *w, GdkEventMotion *e, gpointer data);

void bt_f_load(GtkWidget *w, gpointer data);
void bt_f_avopen(GtkWidget *w, gpointer data);
void bt_f_about(GtkWidget *w, gpointer data);
void bt_f_quit(GtkWidget *w, gpointer data);

void bt_p_segbrain(GtkWidget *w, gpointer data);
void bt_p_alignbrain(GtkWidget *w, gpointer data);
void bt_p_edt(GtkWidget *w, gpointer data);
void bt_p_planarize(GtkWidget *w, gpointer data);
void bt_p_prepare(GtkWidget *w, gpointer data);
void bt_p_clearcache(GtkWidget *w, gpointer data);

void bt_t_clear(GtkWidget *w, gpointer data);
void bt_t_save(GtkWidget *w, gpointer data);
void bt_t_revert(GtkWidget *w, gpointer data);

void bt_update_title();

/* bacgkround thread and message queue */
void   * bt_thread(void *p);
void     bt_process(Task *t);
void     bt_qmsg(const char *msg, int color);
gboolean bt_waitmsg(gpointer data);

void about_destroy(GtkWidget *w, gpointer data);
void about_ok(GtkWidget *w, gpointer data);


/* utility */
int  get_filename(GtkWidget *parent, const char *title, char *dest);
void set_icon(GtkWidget *widget, const char **xpm, char *text);

void bt_names(const char *src);
void bt_head();
void bt_head2();

static GtkItemFactoryEntry bt_menu[] = {
  { "/_File",                    NULL, NULL, 0,"<Branch>" },
  { "/File/_Load Volume...",     NULL, GTK_SIGNAL_FUNC(bt_f_load),0,NULL},
  { "/File/_Open Volume in External Viewer", NULL, GTK_SIGNAL_FUNC(bt_f_avopen),0,NULL},
  { "/File/sep",                 NULL, NULL, 0,"<Separator>" },
  { "/File/_About BrainTag...",     NULL, GTK_SIGNAL_FUNC(bt_f_about),0,NULL},
  { "/File/_Quit",               NULL, GTK_SIGNAL_FUNC(bt_f_quit),0,NULL},
  { "/_Processing",                    NULL, NULL, 0,"<Branch>" },
  { "/Processing/Segment _Brain",      NULL, GTK_SIGNAL_FUNC(bt_p_segbrain), 0, NULL },
  { "/Processing/_Align Brain",        NULL, GTK_SIGNAL_FUNC(bt_p_alignbrain), 0, NULL },
  { "/Processing/_Compute Layers",     NULL, GTK_SIGNAL_FUNC(bt_p_edt), 0, NULL },
  { "/Processing/Compute _Planarization", NULL, GTK_SIGNAL_FUNC(bt_p_planarize), 0, NULL },
  { "/Processing/sep",                 NULL, NULL, 0,"<Separator>" },
  { "/Processing/Prepare Volume for _Tagging", NULL, GTK_SIGNAL_FUNC(bt_p_prepare), 0, NULL },
  { "/Processing/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Processing/Clear Processing Cac_he", NULL, GTK_SIGNAL_FUNC(bt_p_clearcache), 0, NULL },
  { "/_Tagging",                       NULL, NULL, 0, "<Branch>" },
  { "/Tagging/_Clear All Tags",        NULL, GTK_SIGNAL_FUNC(bt_t_clear), 0, NULL },
  { "/Tagging/_Save Tags",             NULL, GTK_SIGNAL_FUNC(bt_t_save),  0, NULL },
  { "/Tagging/_Revert to Saved Tags",  NULL, GTK_SIGNAL_FUNC(bt_t_revert),  0, NULL },
};

void bt_update_title() {
  char tt[1024];
  if (inputname==NULL)
    sprintf(tt,"BrainTag %s",VERSION);
  else
    snprintf(tt,1023,"BrainTag %s - %s",VERSION,inputname);
  gtk_window_set_title(GTK_WINDOW(mw),tt);
}

void gui() {
  GtkWidget *v,*h,*mb;
  GtkItemFactory *gif;
  GtkAccelGroup *mag;
  int nitems = sizeof(bt_menu)/sizeof(bt_menu[0]);

  mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  bt_update_title();
  gtk_window_set_default_size(GTK_WINDOW(mw),960,768);
  gtk_widget_set_events(mw, GDK_EXPOSURE_MASK|GDK_KEY_PRESS_MASK);
  gtk_widget_realize(mw);

  set_icon(mw, icon_cinapce_xpm, (char *) "BrainTag");

  v = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(mw),v);

  /* menu bar */
  mag=gtk_accel_group_new();
  gif=gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);
  gtk_item_factory_create_items (gif, nitems, bt_menu, NULL);   
  gtk_window_add_accel_group(GTK_WINDOW(mw), mag);
  mb = gtk_item_factory_get_widget (gif, "<main>");
  gtk_box_pack_start(GTK_BOX(v),mb,FALSE,TRUE,0);

  h = gtk_hbox_new(FALSE,0);
  canvas = gtk_drawing_area_new();
  gtk_widget_set_events(canvas,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK|GDK_BUTTON_MOTION_MASK|
  			GDK_POINTER_MOTION_MASK);

  gtk_box_pack_start(GTK_BOX(v),h,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(h),canvas,TRUE,TRUE,0);

  gtk_signal_connect(GTK_OBJECT(mw),"delete_event",GTK_SIGNAL_FUNC(bt_del),NULL);
  gtk_signal_connect(GTK_OBJECT(mw),"destroy",GTK_SIGNAL_FUNC(bt_destroy),NULL);
  gtk_signal_connect(GTK_OBJECT(mw),"key_press_event",GTK_SIGNAL_FUNC(bt_key),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"expose_event",GTK_SIGNAL_FUNC(bt_expose),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"button_press_event",GTK_SIGNAL_FUNC(bt_press),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"button_release_event",GTK_SIGNAL_FUNC(bt_release),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"motion_notify_event",GTK_SIGNAL_FUNC(bt_drag),NULL);

  gtk_widget_show_all(mw);
  if (pl==NULL)
    pl = gtk_widget_create_pango_layout(canvas,NULL);
  if (sans10==NULL)
    sans10 = pango_font_description_from_string("Sans 10");
  if (sans10b==NULL)
    sans10b = pango_font_description_from_string("Sans Bold 10");
  if (gc==NULL)
    gc = gdk_gc_new(mw->window);

  Widget::defwidget = canvas;
  Widget::defgc = NULL;
  Widget::defpl = pl;
  Widget::defpfd = sans10b;
  Widget::defhfd = pango_font_description_from_string("Sans Bold 10");

  wlist.push_back(msg = new MessageWindow("Messages",10,500,sans10b));
  wlist.push_front(tools = new BrainToolbar("Tools",10,10));

  //msg->loadGeometry("msg",appcfg);
  //tools->loadGeometry("tools",appcfg);
}

int main(int argc, char **argv) {
  char loadimg[512];

  loadimg[0] = 0;

  setenv("LC_ALL","POSIX",1);
  setenv("LC_NUMERIC","POSIX",1);
  gtk_init(&argc,&argv);
  gdk_rgb_init();

  char *p = getenv("HOME");
  if (p!=NULL) {
    sprintf(appcfg,"%s/.braintag",p);
  } else {
    cerr << "** Fatal Error: HOME not set.\n\n";
    return 99;
  }

  for(int i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-checkers")) {      
      Checkers = true;
      continue;
    }
    if (!strcmp(argv[i],"-csz")) {
      CheckerSize = atoi(argv[(i+1)%argc]);
      ++i;
      continue;
    }
    if (!strcmp(argv[i],"-prepare")) {
      cmd_prepare = 1;
      continue;
    }
    if (!strcmp(argv[i],"-quit")) {
      cmd_quit = 1;
      continue;
    }
    if (argv[i][0] != '-') {
      strcpy(loadimg, argv[i]);
      continue;
    }
  }

  pthread_create(&tid,NULL,bt_thread,NULL);

  gui();

  char z[100];
  sprintf(z,"Braintag v%s started.",VERSION);
  msg->append(z);
  msg->append("(C) 2007-2017 Felipe Bergo <fbergo\x40gmail.com>");

  mid = gtk_timeout_add(150, bt_waitmsg, NULL);

  if (strlen(loadimg)) {
    Task *t, *p;
    snprintf(z,512,"Read volume from %s",loadimg);
    t = new Task(z);
    t->ival[0] = 0;
    t->File = strdup(loadimg);
    
    tmutex.lock();
    qtask.push(t);
    tmutex.unlock();

    if (cmd_prepare) {
      tmutex.lock();
      p = new Task("Segmenting Brain");
      p->ival[0] = 1;
      qtask.push(p);
      p = new Task("Aligning Brain");
      p->ival[0] = 2;
      qtask.push(p);
      p = new Task("Computing EDT");
      p->ival[0] = 6;
      qtask.push(p);
      p = new Task("Computing Planarization");
      p->ival[0] = 8;
      qtask.push(p);  
      tmutex.unlock();
    }
  }

  gtk_main();
  quitting = true;
  pthread_join(tid,NULL);

  return 0;
}

gboolean bt_key(GtkWidget *w, GdkEventKey *e,gpointer data) {

  switch(e->keyval) {
  case GDK_Left:
  case GDK_Up:
    if (vedt1!=NULL)
      vedt1->setDepth(vedt1->getDepth() - 1);
    return TRUE;
  case GDK_Right:
  case GDK_Down:
    if (vedt1!=NULL)
      vedt1->setDepth(vedt1->getDepth() + 1);
    return TRUE;
  case GDK_KP_Subtract:
  case GDK_minus:
    if (v2d!=NULL)
      v2d->setViewRange(v2d->getViewRange() - 0.05);
    return TRUE;
  case GDK_KP_Add:
  case GDK_plus:
  case GDK_equal:
    if (v2d!=NULL)
      v2d->setViewRange(v2d->getViewRange() + 0.05);
    return TRUE;
  case GDK_F1:
    tools->action(0); tools->repaint(); return TRUE;
  case GDK_F2:
    tools->action(1); tools->repaint(); return TRUE;
  case GDK_F5:
    tools->action(2); tools->repaint(); return TRUE;
  case GDK_F6:
    tools->action(3); tools->repaint(); return TRUE;
  case GDK_F7:
    tools->action(4); tools->repaint(); return TRUE;
  default:
    return FALSE;
  }
}

void bt_p_segbrain(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  tmutex.lock();

  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Segmenting Brain");
  p->ival[0] = 1;
  qtask.push(p);
  tmutex.unlock();
}

void bt_p_alignbrain(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  if (binbrain==NULL) {
    bt_qmsg(MSG_NO_BRAIN,errcolor);
    return;
  }

  tmutex.lock();

  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Aligning Brain");
  p->ival[0] = 2;
  qtask.push(p);
  tmutex.unlock();

}

void bt_p_edt(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  if (binbrain==NULL) {
    bt_qmsg(MSG_NO_BRAIN,errcolor);
    return;
  }

  tmutex.lock();

  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Computing EDT");
  p->ival[0] = 6;
  qtask.push(p);
  tmutex.unlock();
}

void bt_p_planarize(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  if (binbrain==NULL) {
    bt_qmsg(MSG_NO_BRAIN,errcolor);
    return;
  }

  if (edt==NULL) {
    bt_qmsg(MSG_NO_EDT,errcolor);
    return;
  }

  tmutex.lock();
  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }
  
  p = new Task("Computing Planarization");
  p->ival[0] = 8;
  qtask.push(p);
  tmutex.unlock();
}

void bt_p_prepare(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  tmutex.lock();
  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Segmenting Brain");
  p->ival[0] = 1;
  qtask.push(p);
  p = new Task("Aligning Brain");
  p->ival[0] = 2;
  qtask.push(p);
  p = new Task("Computing EDT");
  p->ival[0] = 6;
  qtask.push(p);
  p = new Task("Computing Planarization");
  p->ival[0] = 8;
  qtask.push(p);  
  tmutex.unlock();
}

void bt_p_clearcache(GtkWidget *w, gpointer data) {
  Task *p;

  if (orig==NULL) {
    bt_qmsg(MSG_NO_VOLUME,errcolor);
    return;
  }

  tmutex.lock();
  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }
  
  p = new Task("Clearing Disk Cache");
  p->ival[0]=11;
  qtask.push(p);
  tmutex.unlock();
}


void bt_t_clear(GtkWidget *w, gpointer data) {

  if (rtag==NULL || ptag==NULL) {
    bt_qmsg(MSG_NO_TAGS,errcolor);
    return;
  }

  // HERE: warn lost tags

  rtag->fill(0);
  ptag->fill(0);

  if (vbrain!=NULL) vbrain->tagsChanged();
  if (vedt1!=NULL) vedt1->tagsChanged();
  if (vtex!=NULL) vtex->tagsChanged();
  bt_qmsg("Tags cleared.",0xffffff);
}

void bt_t_save(GtkWidget *w, gpointer data) {
  Task *p;

  if (rtag==NULL || ptag==NULL) {
    bt_qmsg(MSG_NO_TAGS,errcolor);
    return;
  }

  tmutex.lock();

  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Saving Tags");
  p->ival[0] = 12;
  qtask.push(p);
  tmutex.unlock();
}

void bt_t_revert(GtkWidget *w, gpointer data) {
  Task *p;

  if (rtag==NULL || ptag==NULL) {
    bt_qmsg(MSG_NO_TAGS,errcolor);
    return;
  }

  // HERE: warn lost tags

  tmutex.lock();

  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }

  p = new Task("Reverting Tags");
  p->ival[0] = 10;
  p->ival[1] = 1; // don't reset if no file on disk
  qtask.push(p);
  tmutex.unlock();
}

void about_destroy(GtkWidget *w, gpointer data) {
  gtk_grab_remove(w);
}

void about_ok(GtkWidget *w, gpointer data) {
  gtk_widget_destroy( (GtkWidget *) data );
}

void bt_f_about(GtkWidget *w, gpointer data) {
  GtkWidget *ad,*vb,*hb,*hb2,*l,*ok,*i;
  char z[1024];
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkStyle *style;
  
  style=gtk_widget_get_style(mw);
  icon = gdk_pixmap_create_from_xpm_d (mw->window, &mask,
                                       &style->bg[GTK_STATE_NORMAL],
                                       (gchar **) icon_cinapce_xpm);
  

  ad = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ad),"About BrainTag");
  gtk_window_set_position(GTK_WINDOW(ad),GTK_WIN_POS_CENTER);
  gtk_window_set_transient_for(GTK_WINDOW(ad),GTK_WINDOW(mw));
  gtk_container_set_border_width(GTK_CONTAINER(ad),6);
  gtk_widget_realize(ad);

  i = gtk_image_new_from_pixmap(icon, mask);

  vb = gtk_vbox_new(FALSE,4);
  hb = gtk_hbox_new(FALSE,0);
  snprintf(z,1024,"BrainTag version %s\n(C) 2007-2017 Felipe Bergo\n<fbergo\x40gmail.com>\n\nThis is free software and comes with no warranty.",VERSION);
  l = gtk_label_new(z);
  hb2 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb2), 5);
  ok = gtk_button_new_with_label("Close");
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);

  gtk_container_add(GTK_CONTAINER(ad), vb);
  gtk_box_pack_start(GTK_BOX(vb),hb,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),i,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(hb),l,FALSE,FALSE,8);
  gtk_box_pack_start(GTK_BOX(vb),hb2,FALSE,TRUE,10);
  gtk_box_pack_start(GTK_BOX(hb2),ok,FALSE,FALSE,2);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ad),"delete_event",
                     GTK_SIGNAL_FUNC(bt_del),NULL);
  gtk_signal_connect(GTK_OBJECT(ad),"destroy",
                     GTK_SIGNAL_FUNC(about_destroy),NULL);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
                     GTK_SIGNAL_FUNC(about_ok),(gpointer)ad);
  
  gtk_widget_show_all(ad);
  gtk_grab_add(ad);

}

void bt_f_avopen(GtkWidget *w, gpointer data) {
  
  if (names[0]==NULL) {
    msg->append("* Open in AfterVoxel failed: No volume loaded.",errcolor);
    return;
  }

  ifstream f;
  f.open(names[0]);
  if (!f.good()) {
    msg->append("* Open in AfterVoxel failed: Aligned volume file is unreadable.",errcolor);
    return;
  }
  f.close();

  char *p = getenv("PATH");
  if (p==NULL) {
    msg->append("* Open in AfterVoxel failed: PATH not set.",errcolor);
    return;
  }
  Tokenizer t(p,":");
  char *q, avpath[1024];
  for(;;) {
    q = t.nextToken();
    if (q==NULL) break;
    sprintf(avpath,"%s/aftervoxel",q);
    ifstream g;
    g.open(avpath);
    if (g.good()) {
      g.close();
      break;
    }
    avpath[0] = 0;
  }

  if (strlen(avpath)==0) {
    msg->append("* Open in AfterVoxel failed: aftervoxel not found.",errcolor);
    return;
  }

  if (fork()==0) {
    execlp(avpath,"aftervoxel",names[0],NULL);
    exit(1);
  }
}

void bt_f_load(GtkWidget *w, gpointer data) {
  char filename[512], z[512];
  
  tmutex.lock();
  if (!qtask.empty()) {
    bt_qmsg(MSG_BUSY,errcolor);
    tmutex.unlock();
    return;
  }
  tmutex.unlock();

  if (get_filename(mw, "Load Volume", filename)) {
    
    if (strstr(filename,"_bt_")!=NULL) {
      msg->append("* Volumes with _bt_ in the name should not be loaded directly.",errcolor);
      return;
    }

    // HERE: warn lost tags

    Task *t;
    snprintf(z,512,"Read volume from %s",filename);
    t = new Task(z);
    t->ival[0] = 0;
    t->File = strdup(filename);
    
    tmutex.lock();
    qtask.push(t);
    tmutex.unlock();
  }
}

void bt_f_quit(GtkWidget *w, gpointer data) {
  if (mid >= 0) gtk_timeout_remove(mid);
  mid = -1;
  gtk_main_quit();
}

gboolean bt_del(GtkWidget *w, GdkEvent *e, gpointer data) {
  return FALSE;
}

void bt_destroy(GtkWidget *w, gpointer data) {

  /*
  if (v2d!=NULL)    v2d->saveGeometry("v2d",appcfg);
  if (msg!=NULL)    msg->saveGeometry("msg",appcfg);
  if (tools!=NULL)  tools->saveGeometry("tools",appcfg);
  if (vbrain!=NULL) vbrain->saveGeometry("vbrain",appcfg);
  if (vedt1!=NULL)  vedt1->saveGeometry("vedt1",appcfg);
  if (vtex!=NULL)   vtex->saveGeometry("vtex",appcfg);
  */

  if (mid >= 0) gtk_timeout_remove(mid);
  mid = -1;
  gtk_main_quit();
}

gboolean bt_expose(GtkWidget *w, GdkEventExpose *e, gpointer data) {
  int W,H;  
  GdkRectangle r;
  int k,j=0;

  for(k=0;k<6;k++)
    if (rupdate[k]) ++j;
  if (j) bt_waitmsg(NULL);

  gdk_window_get_size(GDK_WINDOW(w->window),&W,&H);
  r.x=r.y=0;
  r.width = W;
  r.height = H;
  gdk_gc_set_clip_rectangle(gc, &r);

  gdk_rgb_gc_set_foreground(gc,bgcolor);
  gdk_draw_rectangle(w->window,gc,TRUE,0,0,W,H);

  list<Window *>::reverse_iterator i;
  for(i=wlist.rbegin();i!=wlist.rend();i++) {
    (*i)->paint( (*i)==wlist.front() );
    if (*i == hover)
      (*i)->paintHelp();
  }

  return TRUE;
}

gboolean bt_press(GtkWidget *w, GdkEventButton *e, gpointer data) {
  list<Window *>::iterator i;
  Window *iw;
  int x,y,b;

  x = (int) e->x;
  y = (int) e->y;
  b = (int) e->button;

  for(i=wlist.begin();i!=wlist.end();i++)
    if ( (*i)->inside(x,y) ) {
      if (i != wlist.begin() ) {
	iw = *i;
	wlist.erase(i);
	wlist.push_front(iw);
	i = wlist.begin();
	hover = wlist.front();
	gtk_widget_queue_resize(canvas);
      }
      (*i)->press(x,y,b);
      break;
    }
  return TRUE;
}

gboolean bt_release(GtkWidget *w, GdkEventButton *e, gpointer data) {
  list<Window *>::iterator i;
  int x,y,b;

  x = (int) e->x;
  y = (int) e->y;
  b = (int) e->button;

  for(i=wlist.begin();i!=wlist.end();i++)
    if ( (*i)->hasGrab() ) {
      (*i)->release(x,y,b);
      return TRUE;
    }
  for(i=wlist.begin();i!=wlist.end();i++)
    if ( (*i)->inside(x,y) ) {
      (*i)->release(x,y,b);
      return TRUE;
    }
  return TRUE;
}

gboolean bt_drag(GtkWidget *w, GdkEventMotion *e, gpointer data) {
  list<Window *>::iterator i;
  int x,y,b;

  x = (int) e->x;
  y = (int) e->y;
  b = 0;
  if (e->state & GDK_BUTTON3_MASK) b=3;
  if (e->state & GDK_BUTTON2_MASK) b=2;
  if (e->state & GDK_BUTTON1_MASK) b=1;

  if (b==0) {
    Window *ph;
    ph = hover;
    for(i=wlist.begin();i!=wlist.end();i++)
      if ( (*i)->hasGrab() ) return TRUE;

    hover = NULL;
    for(i=wlist.begin();i!=wlist.end();i++)
      if ( (*i)->inside(x,y) ) {
	hover = *i;
	break;
      }
    if (hover != ph)
      gtk_widget_queue_resize(canvas);
    return TRUE;
  }

  for(i=wlist.begin();i!=wlist.end();i++)
    if ( (*i)->hasGrab() ) {
      (*i)->drag(x,y,b);
      return TRUE;
    }
  for(i=wlist.begin();i!=wlist.end();i++)
    if ( (*i)->inside(x,y) ) {
      (*i)->drag(x,y,b);
      return TRUE;
    }
  return TRUE;
}

void   *bt_thread(void *p) {
  Task *t;

  while(!quitting) {
    tmutex.lock();
    if (qtask.empty()) {
      tmutex.unlock();
      usleep(20000);
      continue;
    }
    
    t = qtask.front();
    tmutex.unlock();   
    bt_process(t);

    tmutex.lock();
    qtask.pop();
    tmutex.unlock();

    delete t;
  }
  return FALSE;
}

void bt_process(Task *t) {
  char z[256];
  Task *p;
  int i;

  t->start = Timestamp::now();
  bt_qmsg(t->Title,0xffffff);
  switch(t->ival[0]) {
  case 0:
    Volume<int> *v;

    bt_names(t->File);
    v = NULL;

    if (names[0]!=NULL) {
      v = new Volume<int>(names[0]);
      if (v->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[0]);
	bt_qmsg(z,0xffffff);
	skip_align = true;
      } else {
	delete v;
	v = NULL;
      }
    }
    
    if (v==NULL) {
      v = new Volume<int>(t->File);
      if (!v->ok()) {
	bt_qmsg("> * Unable to load volume",errcolor);
	if (inputname != NULL) { free(inputname); inputname=NULL; }
	break;
      }
      skip_align = false;
    }

    if (v->dx != v->dy || v->dy != v->dz) {
      Volume<int> *v2;
      bt_qmsg("> Interpolating volume",0xffffff);
      v2 = v->isometricInterpolation();
      delete v;
      v = v2;
    }

    if (orig!=NULL) { delete orig; orig = NULL; }
    orig = v;
    vloaded[0] = true;
    rupdate[0] = true;
    break;
  case 1:
    Volume<int> *ve, *vg;
    Volume<char> *vm,*vs,*vsb,*ts, *tsb;

    vsb = NULL;

    if (names[1]!=NULL) {
      vsb = new Volume<char>(names[1]);
      if (vsb->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[1]);
	bt_qmsg(z,0xffffff);
      } else {
	delete vsb;
	vsb = NULL;
	skip_align = false;
      }
    }

    if (vsb == NULL) {
      bt_qmsg("> Computing Markers",0xffffff);
      vm = orig->brainMarkerComp();
      bt_qmsg("> Enhancing Intensity",0xffffff);
      ve = orig->otsuEnhance(0.42);
      bt_qmsg("> Computing Gradient",0xffffff);
      vg = ve->featureGradient();
      delete ve;
      bt_qmsg("> Tree Pruning",0xffffff);
      vs = vg->treePruning(vm);
      bt_qmsg("> Morphological Close",0xffffff);
      vsb = vs->binaryClose(10.0);
      delete vs;
      vsb->incrementBorder();
      delete vm;
      delete vg;
    }

    if (binbrain!=NULL) {
      delete binbrain;
      binbrain = NULL;
    }

    binbrain = vsb;
    if (binskin!=NULL) { delete binskin; binskin = NULL; }

    vloaded[1] = true;
    rupdate[1] = true;

    tsb = NULL;
    if (names[2]!=NULL) {
      tsb = new Volume<char>(names[2]);
      if (tsb->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[2]);
	bt_qmsg(z,0xffffff);
      } else {
	delete tsb;
	tsb = NULL;
      }
    }

    if (tsb==NULL) {
      bt_qmsg("> Segmenting Skin",0xffffff);
      ts = orig->binaryThreshold( (int) (0.135 * orig->maximum()) );

      for(i=0;i<ts->N;i++)
	if (binbrain->voxel(i) != 0) ts->voxel(i) = 0;      

      bt_qmsg("> Closing Skin",0xffffff);
      tsb = ts->binaryClose(5.0);
      delete ts;
      tsb->incrementBorder();
    }

    binskin = tsb;
    vloaded[2] = true;
    rupdate[1] = true;
    break;

  case 2:
    align_trans.identity();
    align_slice = orig->darkerSlice(binbrain,
				    (int)(10000.0 / (orig->dx * orig->dx)));

    if (!skip_align) {
      int niter;
      orig->planeFit( align_slice, binbrain, align_trans, niter, NULL, false );

      // fix Z rotation
      {
	R3 v[3];
	T4 ar;
	float za;
	
	v[0].set(0,0,0);
	v[1].set(0,-1,0);
	v[0] = align_trans.apply(v[0]);
	v[1] = align_trans.apply(v[1]);
	v[1] -= v[0];
	v[1].Z = 0.0;
	v[1].normalize();
	v[2].set(0,-1,0);
	za = v[2].inner(v[1]);
	if (za > 1.0) za = 1.0;
	za = 180.0 * acos( za ) / M_PI;
	if (v[0].X > 0.0) za = -za;
	ar.zrot(za, orig->W/2, orig->H/2, orig->D/2);
	align_trans *= ar;
      }

      align_trans.invert();
      binbrain->transform(align_trans);
      orig->transform(align_trans);
      binskin->transform(align_trans);
      rupdate[2] = true;

      tmutex.lock();
      p = new Task("Saving Aligned Brain");
      p->ival[0] = 3;
      qtask.push(p);
      tmutex.unlock();
    }

    tmutex.lock();
    p = new Task("Saving Brain Segmentation");
    p->ival[0] = 4;
    qtask.push(p);
    p = new Task("Saving Skin Segmentation");
    p->ival[0] = 5;
    qtask.push(p);
    tmutex.unlock();

    break;

  case 3:
    if (names[0]!=NULL)
      orig->writeSCN(names[0],-1,true);
    break;

  case 4:
    if (names[1]!=NULL)
      binbrain->writeSCN(names[1],-1,true);
    break;

  case 5:
    if (names[2]!=NULL)
      binskin->writeSCN(names[2],-1,true);
    break;

  case 6:
    Volume<int> *vedt;
    vedt = NULL;

    if (names[3]!=NULL) {
      vedt = new Volume<int>(names[3]);
      if (vedt->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[3]);
	bt_qmsg(z,0xffffff);
      } else {
	delete vedt;
	vedt = NULL;
      }
    }

    if (vedt==NULL) {
      vedt = binbrain->edt();
      tmutex.lock();
      p = new Task("Saving EDT");
      p->ival[0] = 7;
      qtask.push(p);
      tmutex.unlock();
    }

    if (edt!=NULL) { delete edt; edt=NULL; }
    edt = vedt;
    vloaded[3] = true;
    rupdate[3] = true;
    break;

  case 7:
    if (names[3]!=NULL)
      edt->writeSCN(names[3],32,true);
    break;

  case 8:
    Volume<int> *vt,*vmap;
    vt = NULL;
    vmap = NULL;
    if (names[4]!=NULL && names[5]!=NULL) {
      vt = new Volume<int>(names[4]);
      vmap = new Volume<int>(names[5]);
      if (vt->ok() && vmap->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[4]);
	bt_qmsg(z,0xffffff);
	snprintf(z,256,"> Loaded cached file %s",names[5]);
	bt_qmsg(z,0xffffff);
      } else {
	delete vt;
	delete vmap;
	vt = NULL;
	vmap = NULL;
      }
    }

    if (Checkers) {
      if (vt!=NULL) { delete vt; vt=NULL; }
      if (vmap!=NULL) { delete vmap; vmap=NULL; }      
    }
    
    if (vt==NULL) {
      Location sc;
      T4 X;
      int darkslice;
      X.identity();
      darkslice = orig->darkerSlice(binbrain, 1000);
      sc = edt->planeCenter(darkslice,X);

      if (Checkers) {
	int ii,mv,ix,iy,iz;
	mv = orig->maximum();
	for(ii=0;ii<orig->N;ii++) {
	  ix = orig->xOf(ii);
	  iy = orig->yOf(ii);
	  iz = orig->zOf(ii);
	  orig->voxel(ii) = ((ix/CheckerSize) + (iy/CheckerSize) + (iz/CheckerSize)) % 2 ? mv/4 : (3*mv)/4;
	}
      }

      vt = edt->edtTexture(orig, sc, &vmap);

      //cout << sqrt(edt->voxel(sc)) << endl;

      tmutex.lock();
      p = new Task("Saving Planarization");
      p->ival[0] = 9;
      qtask.push(p);
      tmutex.unlock();
    }

    if (ptex!=NULL) { delete ptex; ptex = NULL; }
    ptex = vt;
    if (pmap!=NULL) { delete pmap; pmap = NULL; }
    pmap = vmap;

    vloaded[4] = true;
    rupdate[4] = true; 
    
    tmutex.lock();
    p = new Task("Loading FCD tags");
    p->ival[0] = 10;
    p->ival[1] = 0;
    qtask.push(p);
    tmutex.unlock();

    break;
  case 9:
    if (names[4]!=NULL && ptex!=NULL && !Checkers)
      ptex->writeSCN(names[4],-1,false);
    if (names[5]!=NULL && pmap!=NULL)
      pmap->writeSCN(names[5],32,true);
    break;

  case 10:
    Volume<char> *rt, *pt;

    rt = pt = NULL;
    if (names[6]!=NULL && names[7]!=NULL) {
      rt = new Volume<char>(names[6]);
      pt = new Volume<char>(names[7]);
      if (rt->ok() && pt->ok()) {
	snprintf(z,256,"> Loaded cached file %s",names[6]);
	bt_qmsg(z,0xffffff);
	snprintf(z,256,"> Loaded cached file %s",names[7]);
	bt_qmsg(z,0xffffff);
      } else {
	delete rt;
	rt = NULL;
	delete pt;
	pt = NULL;
      }
    }

    if ((rt==NULL || pt==NULL) && t->ival[1]==1) break; // don't clear if nothing saved

    if (rt==NULL) {
      rt = new Volume<char>(orig->W,orig->H,orig->D);
      rt->fill(0);
      pt = new Volume<char>(ptex->W,ptex->H,ptex->D);
      pt->fill(0);
    }

    if (rtag!=NULL) { delete rtag; rtag = NULL; }
    rtag = rt;
    if (ptag!=NULL) { delete ptag; ptag = NULL; }
    ptag = pt;
    rupdate[5] = true;
    vloaded[5] = true;
    break;
  case 11: // clear cache
    for(i=0;i<8;i++)
      if (names[i]!=NULL)
	if (strlen(names[i])>0) {
	  ifstream f;
	  f.open(names[i]);
	  if (f.good()) {
	    f.close();
	    snprintf(z,256,"> Removing cache file %s",names[i]);
	    bt_qmsg(z,0xffffff);
	    unlink(names[i]);
	  }
	}
    snprintf(z,256,"> Cache cleared.");
    bt_qmsg(z,0xffffff);
    break;
  case 12: // save tags
    if (names[6]!=NULL && rtag!=NULL)
      rtag->writeSCN(names[6],-1,false);
    if (names[7]!=NULL && ptag!=NULL)
      ptag->writeSCN(names[7],-1,false);
    break;
  }
  t->finish = Timestamp::now();
  snprintf(z,256,"# Task finished in %.3f seconds #",t->finish - t->start);
  bt_qmsg(z,0xffff00);
}

void bt_qmsg(const char *msg, int color) {
  msgmutex.lock();
  qmsg.push(strdup(msg));
  qmsgc.push(color);
  msgmutex.unlock();
}

gboolean bt_waitmsg(gpointer data) {
  char *m;
  Color c;
  list<Window *>::iterator i;

  msgmutex.lock();
  while(!qmsg.empty()) {
    m = qmsg.front();
    qmsg.pop();
    msg->append(m,qmsgc.front());
    qmsgc.pop();
    free(m);
  }
  msgmutex.unlock();

  if (rupdate[0] && vloaded[0]) {
    rupdate[0] = false;
    bt_update_title();

    if (v2d==NULL) {
      v2d = new OrthogonalView("2D View",10,100,400,400);
      wlist.push_front(v2d);
      //v2d->loadGeometry("v2d",appcfg);
    }
    v2d->setVolume(orig);
    v2d->setSegmentation(NULL);

    if (fv!=NULL) {
      i = find(wlist.begin(),wlist.end(),fv);
      if (i!=wlist.end()) wlist.erase(i);
      delete fv; fv = NULL;
    }

    if (vtex!=NULL) {
      vtex->saveGeometry("vtex",appcfg);
      i = find(wlist.begin(),wlist.end(),vtex);
      if (i!=wlist.end()) wlist.erase(i);
      delete vtex; vtex = NULL;
    }

    if (vedt1!=NULL) {
      vedt1->saveGeometry("vedt1",appcfg);
      i = find(wlist.begin(),wlist.end(),vedt1);
      if (i!=wlist.end()) wlist.erase(i);
      delete vedt1; vedt1 = NULL;
    }

    if (vbrain!=NULL) {
      vbrain->saveGeometry("vbrain",appcfg);
      i = find(wlist.begin(),wlist.end(),vbrain);
      if (i!=wlist.end()) wlist.erase(i);
      delete vbrain; vbrain = NULL;
    }

    if (binbrain!=NULL) { delete binbrain; binbrain=NULL; vloaded[1] = false; }
    if (binskin!=NULL)  { delete binskin; binskin=NULL;  vloaded[2] = false; }
    if (edt!=NULL)      { delete edt; edt=NULL; vloaded[3] = false; }
    if (ptex!=NULL)     { delete ptex; ptex=NULL; vloaded[4] = false; }
    if (pmap!=NULL)     { delete pmap; pmap=NULL; vloaded[4] = false; }
    if (rtag!=NULL)     { delete rtag; rtag=NULL; vloaded[5] = false; }
    if (ptag!=NULL)     { delete ptag; ptag=NULL; vloaded[5] = false; }

    v2d->repaint();
  }

  if (rupdate[1] && vloaded[1]) {
    rupdate[1] = false;
    if (v2d!=NULL) {
      v2d->setSegmentation(binbrain);
      v2d->repaint();
    }
    if (vbrain==NULL) {
      vbrain = new ObjectView("Head View",40,140,400,400);
      wlist.push_front(vbrain);
      //vbrain->loadGeometry("vbrain",appcfg);
    }
    bt_head();
  }

  if (rupdate[2] && vloaded[0] && vloaded[1]) {
    rupdate[2] = false;
    if (v2d!=NULL) {
      v2d->setVolume(orig);
      v2d->setSegmentation(binbrain);
      v2d->repaint();
    }
    if (vbrain!=NULL)
      bt_head();
  }

  if (rupdate[3] && vloaded[3]) {
    rupdate[3] = false;
    if (vedt1==NULL) {
      float range = v2d->getViewRange();
      vedt1 = new EDTView("Curvilinear View",450,140,400,400);
      //vedt1->loadGeometry("vedt1",appcfg);
      vedt1->setViewRange(range);

      wlist.push_front(vedt1);
      vedt1->setBrainToolbar(tools);
      vedt1->syncRotation(vbrain);
      vedt1->syncViewRange(v2d);
      vedt1->syncTags(vbrain);
      vedt1->syncDepth(v2d);
    }
    vedt1->setEDT(edt,orig);
    vedt1->repaint();
    v2d->setEDT(edt);
    v2d->repaint();
  }

  if (rupdate[4] && vloaded[4]) {
    rupdate[4] = false;
    if (vtex==NULL) {
      float range = vedt1->getViewRange();
      vtex = new PlanarView("Planar View",600,180,400,400);
      //vtex->loadGeometry("vtex",appcfg);
      vtex->setViewRange(range);
      wlist.push_front(vtex);
      vtex->setBrainToolbar(tools);
      vtex->syncViewRange(v2d);
      vtex->syncDepth(vedt1);
      vtex->syncTags(vbrain);
    }
    vtex->setVolume(ptex, orig->maximum(), 
		    (int) ceil(sqrt(edt->maximum())), orig->dx);
    vtex->setTags(rtag,ptag,pmap,edt);
    vtex->repaint();

    /*
    if (fv==NULL) {
      fv = new FeatureView("Feature View",700,200,400,400);
      wlist.push_front(fv);
      vtex->setFV(fv);
      fv->setVolume(ptex);      
    }
    */

  }

  if (rupdate[5] && vloaded[5]) {
    rupdate[5] = false;
    vedt1->setTags(rtag,ptag,pmap,edt);
    vtex->setTags(rtag,ptag,pmap,edt);
    bt_head2();
    vedt1->tagsChanged();
    vtex->tagsChanged();
    if (cmd_quit)
      gtk_main_quit();
  }

  return TRUE;
}

void bt_head() {
  Color c;
  vbrain->clearObjects();

  if (binbrain!=NULL) {
    c = 0xc8b464;
    vbrain->appendObject(new RenderObject("Brain",binbrain,c,1.0));
  }

  if (binskin!=NULL) {
    c = 0xeec295;
    vbrain->appendObject(new RenderObject("Skin",binskin,c,0.5));
  }

  vbrain->repaint();
}

void bt_head2() {
  Color c;
  RenderObject *o1,*o2;

  if (rtag!=NULL) {
    o1 = vbrain->getObjectByName("Non-FCD");

    if (o1==NULL)
      vbrain->prependObject(new RenderObject("Non-FCD",rtag,c=0x2020ff,1.0,true,RenderType_EQ,2));
    else
      o1->mask = rtag;

    o2 = vbrain->getObjectByName("FCD");
    if (o2==NULL)
      vbrain->prependObject(new RenderObject("FCD",rtag,c=0xff2020,1.0,true,RenderType_EQ,1));
    else
      o2->mask = rtag;
  }
  vbrain->tagsChanged();
}


void bt_names(const char *src) {

  char *p;
  int i;
  char base[512], ext[64], tmp[512];

  for(i=0;i<8;i++)
    names[i] = NULL;

  if (inputname != NULL) {
    free(inputname);
    inputname = NULL;
  }
  p = (char *) strrchr(src,'/');
  if (p!=NULL)
    inputname = strdup(p+1);
  else
    inputname = strdup(src);

  p = (char *) strchr(src,'.');
  if (p!=NULL) {
    g_strlcpy(base,src,512);
    p = strchr(base,'.');
    *p = 0;    
    g_strlcpy(ext,p+1,64);

    snprintf(tmp,512,"%s_bt_aligned.%s",base,ext);
    names[0] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_seg.%s",base,ext);
    names[1] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_skin.%s",base,ext);
    names[2] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_edt.%s",base,ext);
    names[3] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_ptex.%s",base,ext);
    names[4] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_pmap.%s",base,ext);
    names[5] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_rtags.%s",base,ext);
    names[6] = strdup(tmp);

    snprintf(tmp,512,"%s_bt_ptags.%s",base,ext);
    names[7] = strdup(tmp);
  }


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
