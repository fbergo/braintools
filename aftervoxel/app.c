
/*
   Aftervoxel
   (C) 2004-2011 Felipe Bergo
   Programmed by Felipe P. G. Bergo
*/

#define SOURCE_APP_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libip.h>
#include <pthread.h>
#include "aftervoxel.h"

#include "xpm/mouse.xpm"

#include "xpm/panhand.xbm"
#include "xpm/panhand_mask.xbm"
#include "xpm/zoomcur.xbm"
#include "xpm/zoomcur_mask.xbm"
#include "xpm/pencur.xbm"
#include "xpm/pencur_mask.xbm"
#include "xpm/icon48.xpm"

GtkWidget *mw,*cw;

GdkPixmap *canvas = 0;
GdkGC     *gc = 0;

static char *vname[4] = {"2D(1)", "2D(2)", "2D(3)", "3D"};

// old bg: 39437c
ViewState view = { .W = -1, .H = -1, .bg = APPBG, .fg = APPFG,
		   .maximized = -1, .buf = {0,0,0,0}, .crosshair = 0x00ff00,
                   .measures = 0xffffff, .focus = 0, .zopane = -1,
                   .zobuf = 0, .pfg = APPCFG, .pbg = APPCBG,
                   .bcpane = -1 };

Resources res;

VolData voldata;

GUI gui;
gint apptoid = -1;

volatile NotifyData notify = {0,0,0,0,0,0,0};

struct _busydata {
  int  active;
  char msg[64];
  int  done;
  int  clock;

  CImg *buf, *ticker;
} busy = { .active=0, .clock = 0, .buf = 0, .ticker = 0 };

LabelControl labels;

pthread_t good_thread = 0;
int       tdebug      = 0;

/* app window events */

void      app_gui();
gboolean  app_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void      app_destroy(GtkWidget *w, gpointer data);
gboolean  app_timeout(gpointer data);
void      app_pop_consumer();

gboolean  app_key_down(GtkWidget *w,GdkEventKey *ek,gpointer data);

/* canvas events */
gboolean  canvas_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  canvas_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);
gboolean  canvas_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data);
gboolean  canvas_release(GtkWidget *widget, GdkEventButton *eb, gpointer data);
void      canvas_draw_2D(int i);
void      canvas_draw_3D();

void      canvas_calc();
void      update_controls();

void      overlay_changed(void *args);

/* key focus */
gboolean app_kf_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data);
gboolean app_kf_press(GtkWidget *w, GdkEventButton *eb, gpointer data);

/* view control */

void      v2d_palette_changed(Slider *s);
gboolean  palette_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
void      pal_up(void *args);
void      pal_down(void *args);

void      app_compose3d_changed(void *t);
void      app_vis_changed(void *t);
void      app_cmdline_open(const char *path);
void      app_cmdline_openseg(const char *path);

#define CBARG GtkWidget *w, gpointer data

void      app_menu_open(CBARG);
void      app_menu_close(CBARG);
void      app_menu_dicom(CBARG);
void      app_menu_about(CBARG);

void      app_measure_load(CBARG);
void      app_measure_save(CBARG);

void      app_label_load(CBARG);
void      app_label_save(CBARG);

void      app_menu_export_scn(CBARG);
void      app_menu_export_analyze(CBARG);

void      app_menu_seg_erase(CBARG);
void      app_menu_seg_load(CBARG);
void      app_menu_seg_save(CBARG);
void      app_menu_seg_auto(CBARG);
void      app_menu_seg_thres(CBARG);
void      app_menu_seg_tp2007(CBARG);

void      app_menu_undo(CBARG);
void      app_menu_kill_measures(CBARG);

void      app_menu_view_options3d(CBARG);
void      app_menu_view_export(CBARG);
void      app_menu_view_export2(CBARG);
void      app_menu_tool_info(CBARG);
void      app_menu_tool_volinfo(CBARG);
void      app_menu_tool_volumetry(CBARG);
void      app_menu_tool_histogram(CBARG);
void      app_menu_tool_roi(CBARG);
void      app_menu_tool_orient(CBARG);

void      app_session_new(CBARG);
void      app_session_open(CBARG);
void      app_session_save(CBARG);
void      app_session_save_as(CBARG);

void      app_title_check();

gboolean app_status_clear(gpointer data);

// zoom overlay
void zo_start(int x,int y,int pane);
void zo_drag(int x,int y);
void zo_end(int x,int y);

// bri/con overlay
void bc_start(int x,int y,int pane);
void bc_drag(int x,int y);
void bc_end(int x,int y);

static GtkItemFactoryEntry app_menu[] = {
  { "/_File",NULL, NULL, 0, "<Branch>" },
  { "/File/_Open Image...",NULL, GTK_SIGNAL_FUNC(app_menu_open),0,NULL},
  { "/File/Import _DICOM...",NULL,GTK_SIGNAL_FUNC(app_menu_dicom),0,NULL},
  { "/File/sep",NULL, NULL, 0, "<Separator>" },
  { "/File/_Load Measurements...",NULL,GTK_SIGNAL_FUNC(app_measure_load),0,NULL},
  { "/File/_Save Measurements As...",NULL,GTK_SIGNAL_FUNC(app_measure_save),0,NULL},
  { "/File/sep",NULL, NULL, 0, "<Separator>" },
  { "/File/Load Object _Names and Colors...",NULL,GTK_SIGNAL_FUNC(app_label_load),0,NULL},
  { "/File/Save Object Names and _Colors As...",NULL,GTK_SIGNAL_FUNC(app_label_save),0,NULL},
  //{ "/File/sep",NULL, NULL, 0, "<Separator>" },
  //{ "/File/_New Session",NULL,GTK_SIGNAL_FUNC(app_session_new),0,NULL},
  //{ "/File/Open _Session...",NULL,GTK_SIGNAL_FUNC(app_session_open),0,NULL},
  //{ "/File/Sa_ve Session",NULL,GTK_SIGNAL_FUNC(app_session_save),0,NULL},
  //{ "/File/Save Session _As...",NULL,GTK_SIGNAL_FUNC(app_session_save_as),0,NULL},
  { "/File/sep",NULL, NULL, 0, "<Separator>" },
  { "/File/E_xport Volume as SCN...",NULL, GTK_SIGNAL_FUNC(app_menu_export_scn),0,NULL},
  { "/File/Export Volume as Analy_ze...",NULL, GTK_SIGNAL_FUNC(app_menu_export_analyze),0,NULL},
  { "/File/sep",NULL, NULL, 0, "<Separator>" },
  { "/File/A_bout Aftervoxel...",NULL, GTK_SIGNAL_FUNC(app_menu_about),0,NULL},
  { "/File/_Quit",NULL, GTK_SIGNAL_FUNC(app_menu_close),0,NULL},
  { "/_Edit",NULL,NULL,0,"<Branch>" },
  { "/Edit/_Undo Brush or Contour",NULL,GTK_SIGNAL_FUNC(app_menu_undo),201,NULL},
  { "/Edit/_Remove All Measurements",NULL,GTK_SIGNAL_FUNC(app_menu_kill_measures),0,NULL},
  { "/_Visualization",NULL,NULL,0,"<Branch>" },
  { "/Visualization/E_xport View...",NULL,GTK_SIGNAL_FUNC(app_menu_view_export),0,NULL},
  { "/Visualization/Export _Slices...",NULL,GTK_SIGNAL_FUNC(app_menu_view_export2),0,NULL},
  { "/Visualization/sep",NULL,NULL,0,"<Separator>" },
  { "/Visualization/3D _Options...",NULL,GTK_SIGNAL_FUNC(app_menu_view_options3d),0,NULL},
  { "/_Segmentation",NULL,NULL,0,"<Branch>" },
  { "/Segmentation/_Remove Segmentation",NULL,GTK_SIGNAL_FUNC(app_menu_seg_erase),0,NULL },
  { "/Segmentation/sep",NULL,NULL,0,"<Separator>" },
  { "/Segmentation/_Load Segmentation from SCN...",NULL,GTK_SIGNAL_FUNC(app_menu_seg_load),0,NULL },
  { "/Segmentation/_Save Segmentation to SCN...",NULL,GTK_SIGNAL_FUNC(app_menu_seg_save),0,NULL },
  { "/Segmentation/sep",NULL,NULL,0,"<Separator>" },
  //  { "/Segmentation/Segmentation by _Connectivity...",NULL,GTK_SIGNAL_FUNC(app_menu_seg_auto),0,NULL },
  { "/Segmentation/Segmentation by _Thresholding...",NULL,GTK_SIGNAL_FUNC(app_menu_seg_thres),0,NULL },
  { "/Segmentation/Automatic _Brain Segmentation by Tree Pruning",NULL,GTK_SIGNAL_FUNC(app_menu_seg_tp2007),0,NULL },
  { "/_Tools",NULL,NULL,0,"<Branch>" },
  //  { "/Tools/410",NULL,NULL,0,"<Separator>" },
  { "/Tools/_Patient Info...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_info),0,NULL},
  { "/Tools/_Volume Properties...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_volinfo),0,NULL},
  { "/Tools/_Histogram...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_histogram),0,NULL},
  { "/Tools/Volumetr_y...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_volumetry),0,NULL},
  { "/Tools/sep",NULL,NULL,0,"<Separator>" },
  { "/Tools/_Region of Interest...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_roi),0,NULL},
  { "/Tools/View Ori_entation...",NULL,GTK_SIGNAL_FUNC(app_menu_tool_orient),0,NULL},
};

void app_gui() {
  GtkWidget *v,*h,*tmp,*vleft,*vright,*l1,*l2,*l3,*l4,*l5,*l6;
  GtkWidget *t1,*eb[7],*t2;
  GtkWidget *ov,*ol,*ot;
  GtkWidget *roiv,*roil;
  Button *pal1,*pal2;
  GtkWidget *ph,*pc;
  Label *xl[4];
  GtkAccelGroup *mag;
  GtkItemFactory *gif;
  GdkPixbuf *pb;
  int nitems = sizeof(app_menu)/sizeof(app_menu[0]);

  Filler *fr,*fl;

  GtkWidget *tas[10];
  int i;

  app_tdebug(2000);

  mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_events(mw,GDK_KEY_PRESS_MASK);
  gtk_window_set_title(GTK_WINDOW(mw),APPNAME);
  gtk_window_set_default_size(GTK_WINDOW(mw),800,600);
  gtk_widget_realize(mw);

  pb = gdk_pixbuf_new_from_xpm_data((const char **)icon48_xpm);
  gtk_window_set_icon(GTK_WINDOW(mw), pb);
  gdk_window_set_icon_name(mw->window,APPNAME);

  /* main vbox */
  v = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(mw), v);

  /* menu bar */
  mag=gtk_accel_group_new();
  gif=gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);
  gtk_item_factory_create_items (gif, nitems, app_menu, NULL);
  gtk_window_add_accel_group(GTK_WINDOW(mw), mag);

  tmp = gtk_item_factory_get_widget (gif, "<main>");
  gtk_box_pack_start(GTK_BOX(v), tmp, FALSE, TRUE, 0);
  gtk_widget_show(tmp);

  /* big hbox */
  h = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 0);
  gtk_widget_show(h);

  vleft = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(h), vleft, FALSE, FALSE, 0);
  gtk_widget_show(vleft);

  gui.t2d = ToolbarNew(9,3,36,36);

  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_ARROW),"2D Cursor [Q]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_PAN),"2D Pan [W]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_ZOOM),"2D Zoom [E]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_PEN),"2D Brush [A]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_ERASER),"2D Eraser [S]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_CONTOUR1),"2D Contour [D]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_CONTOUR2),"2D Point Contour [Z]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_DIST),"2D Ruler [X]");
  ToolbarAddButton(gui.t2d,GlyphNewInit(24,24,ICON_TOOL_ANGLE),"2D Angles [C]");

  ToolbarSetCallback(gui.t2d, (Callback) app_t2d_changed);

  gui.t2d->bgi = view.pbg;
  gui.t2d->bga = darker(view.pfg, 4);
  gui.t2d->fgi = 0x000000;
  gui.t2d->fga = 0xffff00;

  eb[0] = gtk_event_box_new();
  l1 = gtk_label_new("2D Tools");
  util_set_fg(l1, 0xffffff);
  util_set_bg(eb[0],0x303030);

  gtk_container_add(GTK_CONTAINER(eb[0]),l1);
  gtk_box_pack_start(GTK_BOX(vleft), eb[0], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vleft), gui.t2d->widget, FALSE, FALSE, 0);
  gtk_widget_show(l1);

  eb[1] = gtk_event_box_new();
  gui.penlabel = gtk_label_new("Brush");
  util_set_fg(gui.penlabel, 0xffffff);
  util_set_bg(eb[1], 0x303030);
  gui.tpen = ToolbarNew(8,4,27,27);

  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_1),"Brush 1 [F1]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_3),"Brush 3 [F2]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_5),"Brush 5 [F3]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_7),"Brush 7 [F4]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_9),"Brush 9 [F5]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_11),"Brush 11 [F6]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_13),"Brush 13 [F7]");
  ToolbarAddButton(gui.tpen,GlyphNewInit(24,24,ICON_PEN_15),"Brush 15 [F8]");

  gui.tpen->bgi = view.pbg;
  gui.tpen->bga = darker(view.pfg, 4);
  gui.tpen->fgi = 0x000000;
  gui.tpen->fga = 0xffff00;

  gtk_container_add(GTK_CONTAINER(eb[1]),gui.penlabel);
  gtk_box_pack_start(GTK_BOX(vleft), eb[1], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vleft), gui.tpen->widget, FALSE, FALSE, 0);
  gtk_widget_show(gui.penlabel);

  /* label picker */

  eb[2] = gtk_event_box_new();
  l2 = gtk_label_new("Objects");
  util_set_fg(l2, 0xffffff);
  util_set_bg(eb[2], 0x303030);
  gtk_container_add(GTK_CONTAINER(eb[2]), l2);
  gtk_box_pack_start(GTK_BOX(vleft), eb[2], FALSE, FALSE, 0);
  labels.widget = gtk_drawing_area_new();
  gtk_widget_set_events(labels.widget,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
  gtk_box_pack_start(GTK_BOX(vleft), labels.widget, FALSE, FALSE, 0);
  gtk_widget_show(l2);
  gtk_widget_show(labels.widget);  

  fl = FillerNew(view.pbg);
  gtk_box_pack_start(GTK_BOX(vleft), fl->widget, TRUE, TRUE, 0);

  /* roi control */

  gui.roipanel = gtk_event_box_new();
  roiv = gtk_vbox_new(FALSE,0);
  roil = gtk_label_new("ROI");
  util_set_fg(roil, 0xffffff);
  util_set_bg(gui.roipanel, 0x603030);
  gtk_container_add(GTK_CONTAINER(gui.roipanel), roiv);
  gtk_box_pack_start(GTK_BOX(roiv), roil, FALSE, FALSE, 0);
  for(i=0;i<4;i++) {
    gui.roil[i] = LabelNew(0xddcccc,0x800000,res.font[3]);
    gtk_box_pack_start(GTK_BOX(roiv),gui.roil[i]->widget,FALSE,TRUE,0);
  }
  gui.roib[0] = ButtonNew(0xddcccc,0x800000,res.font[3],"Crop");
  gui.roib[1] = ButtonNew(0xddcccc,0x800000,res.font[3],"Cancel");
  gtk_box_pack_start(GTK_BOX(roiv),gui.roib[0]->widget,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(roiv),gui.roib[1]->widget,FALSE,TRUE,0);
  gtk_widget_show(roil);
  gtk_widget_show(roiv);
  gtk_box_pack_start(GTK_BOX(vleft), gui.roipanel, FALSE, FALSE, 0);

  ButtonSetCallback(gui.roib[0],&roi_clip);
  ButtonSetCallback(gui.roib[1],&roi_cancel);

  eb[6] = gtk_event_box_new();
  l6 = gtk_label_new("Tonalization");
  util_set_fg(l6, 0xffffff);
  util_set_bg(eb[6], 0x303030);
  gtk_container_add(GTK_CONTAINER(eb[6]), l6);
  gtk_box_pack_start(GTK_BOX(vleft), eb[6], FALSE, FALSE, 0);
  gtk_widget_show(l6);

  ph = gtk_hbox_new(FALSE,0);
  pal1 = ButtonNewFull(view.pbg,0,res.font[2],"<",14,16);
  pal2 = ButtonNewFull(view.pbg,0,res.font[2],">",14,16);
  gui.palcanvas = pc = gtk_drawing_area_new();
  gtk_widget_set_events(pc,GDK_EXPOSURE_MASK);
  gtk_box_pack_start(GTK_BOX(ph), pal1->widget, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ph), pc, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ph), pal2->widget, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vleft), ph, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(pc), "expose_event",
		     GTK_SIGNAL_FUNC(palette_expose), 0);
  ButtonSetCallback(pal1,&pal_down);
  ButtonSetCallback(pal2,&pal_up);
  gtk_widget_show(ph);
  gtk_widget_show(pc);

  gui.ovl[0] = LabelNew(view.pbg,0x404040,res.font[3]);
  gui.ovl[1] = LabelNew(view.pbg,0x404040,res.font[3]);
  LabelSetText(gui.ovl[0],"Identity");
  LabelSetText(gui.ovl[1],"Orientation");
  gui.overlay_info  = CheckBoxNew(view.pbg,0,0);
  gui.overlay_orient= CheckBoxNew(view.pbg,0,0);
  ot = gtk_table_new(2,2,FALSE);
  gtk_table_attach_defaults(GTK_TABLE(ot), gui.overlay_info->widget,0,1,0,1);
  gtk_table_attach_defaults(GTK_TABLE(ot), gui.overlay_orient->widget,0,1,1,2);
  gtk_table_attach_defaults(GTK_TABLE(ot), gui.ovl[0]->widget,1,2,0,1);
  gtk_table_attach_defaults(GTK_TABLE(ot), gui.ovl[1]->widget,1,2,1,2);
  gtk_box_pack_end(GTK_BOX(vleft), ot, FALSE, FALSE, 0);
  gtk_widget_show(ot);
  CheckBoxSetCallback(gui.overlay_info,(Callback) &overlay_changed);
  CheckBoxSetCallback(gui.overlay_orient,(Callback) &overlay_changed);

  gui.ton = SliderNew(GlyphNewInit(16,16,ICON_TON),16);
  gui.con = SliderNew(GlyphNewInit(16,16,ICON_CON),16);
  gui.bri = SliderNew(GlyphNewInit(16,16,ICON_BRI),16);

  gui.con->defval = 0.0;
  gui.bri->defval = 0.5;
  gui.ton->defval = 0.7;

  SliderSetCallback(gui.con, (Callback) v2d_palette_changed);
  SliderSetCallback(gui.bri, (Callback) v2d_palette_changed);
  SliderSetCallback(gui.ton, (Callback) v2d_palette_changed);

  gui.con->bg   = view.pbg;
  gui.con->fg   = 0x000000;
  gui.con->fill = view.pfg;

  gui.bri->bg   = view.pbg;
  gui.bri->fg   = 0x000000;
  gui.bri->fill = view.pfg;

  gui.ton->bg   = view.pbg;
  gui.ton->fg   = 0x000000;
  gui.ton->fill = view.pfg;

  gui.con->value = 0.0;
  gui.bri->value = 0.5;
  gui.ton->value = 0.7;

  gtk_box_pack_end(GTK_BOX(vleft), gui.bri->widget, FALSE, TRUE,0);
  gtk_box_pack_end(GTK_BOX(vleft), gui.con->widget, FALSE, TRUE,0);
  gtk_box_pack_end(GTK_BOX(vleft), gui.ton->widget, FALSE, TRUE,0);

  cw = gtk_drawing_area_new();
  gtk_widget_set_events(cw,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|
                        GDK_BUTTON1_MOTION_MASK|
                        GDK_BUTTON2_MOTION_MASK|
                        GDK_BUTTON3_MOTION_MASK|
                        GDK_BUTTON_RELEASE_MASK);
  gtk_box_pack_start(GTK_BOX(h), cw, TRUE, TRUE, 0);
  util_set_bg(cw,view.bg);
  gtk_widget_show(cw);

  /* 3d area */

  vright = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(h), vright, FALSE, FALSE, 0);
  gtk_widget_show(vright);

  gui.t3d = ToolbarNew(4,3,36,36);
  
  ToolbarAddButton(gui.t3d, GlyphNewInit(24,24,ICON_TOOL_ROT),"3D Rotate [R]");
  ToolbarAddButton(gui.t3d, GlyphNewInit(24,24,ICON_TOOL_PAN),"3D Pan [T]");
  ToolbarAddButton(gui.t3d, GlyphNewInit(24,24,ICON_TOOL_ZOOM),"3D Zoom [Y]");
  ToolbarAddButton(gui.t3d, GlyphNewInit(24,24,ICON_TOOL_DIST),"3D Geodesic Ruler [G]");

  ToolbarSetCallback(gui.t3d, (Callback) app_t3d_changed);

  gui.t3d->bgi = view.pbg;
  gui.t3d->bga = darker(view.pfg, 4);
  gui.t3d->fgi = 0x000000;
  gui.t3d->fga = 0xffff00;

  eb[3] = gtk_event_box_new();
  l3 = gtk_label_new("3D Tools");
  util_set_fg(l3, 0xffffff);
  util_set_bg(eb[3], 0x303030);
  gtk_container_add(GTK_CONTAINER(eb[3]), l3);
  gtk_box_pack_start(GTK_BOX(vright), eb[3], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vright), gui.t3d->widget, FALSE, FALSE, 0);
  gtk_widget_show(l3);

  eb[4] = gtk_event_box_new();
  l4 = gtk_label_new("Composition");
  util_set_fg(l4, 0xffffff);
  util_set_bg(eb[4], 0x303030);
  gtk_container_add(GTK_CONTAINER(eb[4]), l4);
  gtk_box_pack_start(GTK_BOX(vright), eb[4], FALSE, FALSE, 0);
  gtk_widget_show(l4);

  t1 = gtk_table_new(8,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vright), t1, FALSE, FALSE, 0);
  gtk_widget_show(t1);

  gui.skin  = CheckBoxNew(view.pbg,0,1);
  gui.oct   = CheckBoxNew(view.pbg,0,0);
  gui.obj   = CheckBoxNew(view.pbg,0,1);
  gui.meas  = CheckBoxNew(view.pbg,0,1);

  gui.null = GlyphNewInit(1,1,"!0");

  gui.oskin = SliderNew(gui.null,16);
  gui.oobj  = SliderNew(gui.null,16);

  gui.oskin->value  = 1.0;
  gui.oskin->defval = 1.0;
  gui.oskin->bg = view.pbg;
  gui.oskin->fg = 0x000000;
  gui.oskin->fill = view.pfg;

  gui.oobj->value  = 1.0;
  gui.oobj->defval = 1.0;
  gui.oobj->bg = view.pbg;
  gui.oobj->fg = 0x000000;
  gui.oobj->fill = view.pfg;

  xl[0] = LabelNew(view.pbg,0x404040,res.font[3]);
  xl[1] = LabelNew(view.pbg,0x404040,res.font[3]);
  xl[2] = LabelNew(view.pbg,0x404040,res.font[3]);
  xl[3] = LabelNew(view.pbg,0x404040,res.font[3]);

  LabelSetText(xl[0],"Skin");
  LabelSetText(xl[1],"Octant");
  LabelSetText(xl[2],"Objects");
  LabelSetText(xl[3],"Measures");

  gtk_table_attach_defaults(GTK_TABLE(t1), xl[0]->widget, 1,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(t1), xl[1]->widget, 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(t1), gui.skin->widget, 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(t1), gui.oct->widget,  0,1, 1,2);

  gtk_table_attach_defaults(GTK_TABLE(t1), xl[2]->widget, 1,2, 3,4);
  gtk_table_attach_defaults(GTK_TABLE(t1), gui.obj->widget,  0,1, 3,4);

  gtk_table_attach_defaults(GTK_TABLE(t1), gui.oskin->widget, 0,2, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(t1), gui.oobj->widget,  0,2, 4,5);

  gtk_table_attach_defaults(GTK_TABLE(t1), xl[3]->widget, 1,2, 5,6);
  gtk_table_attach_defaults(GTK_TABLE(t1), gui.meas->widget,  0,1, 5,6);


  CheckBoxSetCallback(gui.skin, &app_compose3d_changed);
  CheckBoxSetCallback(gui.oct,  &app_compose3d_changed);
  CheckBoxSetCallback(gui.obj,  &app_compose3d_changed);
  CheckBoxSetCallback(gui.meas, &app_compose3d_changed);

  SliderSetCallback(gui.oskin, &app_compose3d_changed);
  SliderSetCallback(gui.oobj,  &app_compose3d_changed);

  eb[5] = gtk_event_box_new();
  l5 = gtk_label_new("Visibility");
  util_set_fg(l5,0xffffff);
  util_set_bg(eb[5],0x303030);
  gtk_container_add(GTK_CONTAINER(eb[5]), l5);
  gtk_box_pack_start(GTK_BOX(vright), eb[5], FALSE, FALSE, 0);
  gtk_widget_show(l5);

  t2 = gtk_table_new(MAXLABELS,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vright), t2, FALSE, FALSE, 0);
  gtk_widget_show(t2);

  for(i=0;i<MAXLABELS;i++) {
    gui.vis_mark[i] = CheckBoxNew(view.pbg,0,1);
    gui.vis_name[i] = LabelNew(view.pbg,0x404040,res.font[3]);
    gtk_widget_hide(gui.vis_mark[i]->widget);
    gtk_widget_hide(gui.vis_name[i]->widget);
    gtk_table_attach_defaults(GTK_TABLE(t2),gui.vis_mark[i]->widget,0,1,i,i+1);
    gtk_table_attach_defaults(GTK_TABLE(t2),gui.vis_name[i]->widget,1,2,i,i+1);
    CheckBoxSetCallback(gui.vis_mark[i], &app_vis_changed);
  }

  fr = FillerNew(view.pbg);
  gtk_box_pack_start(GTK_BOX(vright), fr->widget, TRUE, TRUE, 0);

  /* octant control */
  gui.octpanel = gtk_event_box_new();
  ov = gtk_vbox_new(FALSE, 0);
  ol = gtk_label_new("Octant");
  util_set_fg(ol, 0xffffff);
  util_set_bg(gui.octpanel, 0x303030);
  gtk_container_add(GTK_CONTAINER(gui.octpanel), ov);
  gtk_box_pack_start(GTK_BOX(ov), ol, FALSE, FALSE, 0);
  gui.octcw = gtk_drawing_area_new();
  util_set_bg(gui.octcw, view.pbg);
  gtk_drawing_area_size(GTK_DRAWING_AREA(gui.octcw), 64, 64);
  gtk_widget_set_events(gui.octcw,GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK|
			GDK_BUTTON1_MOTION_MASK);
  gtk_box_pack_start(GTK_BOX(ov), gui.octcw, FALSE, FALSE, 0);
  gtk_widget_show(gui.octcw);
  gtk_widget_show(ol);
  gtk_widget_show(ov);
  
  gtk_signal_connect(GTK_OBJECT(gui.octcw), "expose_event",
		     GTK_SIGNAL_FUNC(view3d_oct_expose), 0);

  gtk_signal_connect(GTK_OBJECT(gui.octcw), "button_press_event",
		     GTK_SIGNAL_FUNC(view3d_oct_press), 0);
  gtk_signal_connect(GTK_OBJECT(gui.octcw), "button_release_event",
		     GTK_SIGNAL_FUNC(view3d_oct_release), 0);
  gtk_signal_connect(GTK_OBJECT(gui.octcw), "motion_notify_event",
		     GTK_SIGNAL_FUNC(view3d_oct_drag), 0);

  gtk_box_pack_end(GTK_BOX(vright), gui.octpanel, FALSE, FALSE, 0);

  /* first status bar (tool help) */

  gui.f2d = gtk_label_new(" ");
  gui.f3d = gtk_label_new(" ");

  tas[0] = gtk_vbox_new(FALSE,0);
  tas[1] = gtk_frame_new(0);
  tas[2] = gtk_hbox_new(FALSE,0);
  tas[4] = gtk_hbox_new(FALSE,0);
  tas[5] = gtk_hbox_new(FALSE,0);

  gtk_frame_set_shadow_type(GTK_FRAME(tas[1]), GTK_SHADOW_IN);

  gtk_box_pack_start(GTK_BOX(v), tas[1], FALSE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(tas[1]), tas[2]);
  tas[3] = util_pixmap_new(mw, mouse_xpm);
  gtk_box_pack_start(GTK_BOX(tas[2]), tas[3], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(tas[2]), tas[0], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tas[0]), tas[4], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tas[0]), tas[5], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tas[4]), gui.f2d, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(tas[5]), gui.f3d, FALSE, FALSE, 0);

  gui.kl = gtk_label_new("Key Focus:");
  gui.kf = gtk_drawing_area_new();
  gtk_widget_set_events(gui.kf, GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(gui.kf), 48, 32);

  gtk_box_pack_start(GTK_BOX(tas[2]), gui.kl, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(tas[2]), gui.kf, FALSE, FALSE, 0);
  gtk_widget_show(gui.kl);
  gtk_widget_show(gui.kf);

  gtk_signal_connect(GTK_OBJECT(gui.kf), "expose_event",
		     GTK_SIGNAL_FUNC(app_kf_expose), 0);
  gtk_signal_connect(GTK_OBJECT(gui.kf), "button_press_event",
		     GTK_SIGNAL_FUNC(app_kf_press), 0);


  for(i=0;i<=5;i++) gtk_widget_show(tas[i]);
  gtk_widget_show(gui.f2d);
  gtk_widget_show(gui.f3d);

  /* status bar */

  gui.status = gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(v), gui.status, FALSE, TRUE, 0);
  gtk_widget_show(gui.status);

  /* attach signals */

  gtk_signal_connect(GTK_OBJECT(mw),"destroy",
                     GTK_SIGNAL_FUNC(app_destroy), 0);

  gtk_signal_connect(GTK_OBJECT(mw),"delete_event",
                     GTK_SIGNAL_FUNC(app_delete), 0);

  gtk_signal_connect(GTK_OBJECT(mw),"key_press_event",
                     GTK_SIGNAL_FUNC(app_key_down), 0);

  gtk_signal_connect(GTK_OBJECT(cw),"expose_event",
                     GTK_SIGNAL_FUNC(canvas_expose), 0);

  gtk_signal_connect(GTK_OBJECT(cw),"button_press_event",
                     GTK_SIGNAL_FUNC(canvas_press), 0);

  gtk_signal_connect(GTK_OBJECT(cw),"button_release_event",
                     GTK_SIGNAL_FUNC(canvas_release), 0);

  gtk_signal_connect(GTK_OBJECT(cw),"motion_notify_event",
                     GTK_SIGNAL_FUNC(canvas_drag), 0);

  gtk_signal_connect(GTK_OBJECT(labels.widget),"expose_event",
                     GTK_SIGNAL_FUNC(labels_expose), 0);

  gtk_signal_connect(GTK_OBJECT(labels.widget),"configure_event",
                     GTK_SIGNAL_FUNC(labels_configure), 0);

  gtk_signal_connect(GTK_OBJECT(labels.widget),"button_press_event",
                     GTK_SIGNAL_FUNC(labels_press), 0);

  gtk_signal_connect(GTK_OBJECT(labels.widget),"button_release_event",
                     GTK_SIGNAL_FUNC(labels_release), 0);

  /* show stuff */
  for(i=0;i<7;i++) gtk_widget_show(eb[i]);

  gtk_widget_show(v);
  gtk_widget_show(mw);  

  apptoid = gtk_timeout_add(300, app_timeout, 0);
  app_t2d_changed(0);
  app_t3d_changed(0);
  labels_changed();


  /* init cursors */
  
  gui.cursor2[0] = gdk_cursor_new(GDK_TCROSS);
  gui.cursor2[1] = util_cursor_new(mw,panhand_bits,panhand_mask_bits,8,8);
  gui.cursor2[2] = util_cursor_new(mw,zoomcur_bits,zoomcur_mask_bits,5,5);
  gui.cursor2[3] = util_cursor_new(mw,pencur_bits,pencur_mask_bits,0,15);
  gui.cursor2[4] = util_cursor_new(mw,pencur_bits,pencur_mask_bits,0,15);
  gui.cursor2[5] = util_cursor_new(mw,pencur_bits,pencur_mask_bits,0,15);
  gui.cursor2[6] = util_cursor_new(mw,pencur_bits,pencur_mask_bits,0,15);
  gui.cursor2[7] = gdk_cursor_new(GDK_TCROSS);
  gui.cursor2[8] = gdk_cursor_new(GDK_TCROSS);

  gui.cursor3[0] = gui.cursor2[1];
  gui.cursor3[1] = gui.cursor2[1];
  gui.cursor3[2] = gui.cursor2[2];
  gui.cursor3[3] = gui.cursor2[7];
  gui.cursor3[4] = gui.cursor2[7];
  gui.cstate = -1;

  gui.edit_undo = gtk_item_factory_get_widget_by_action(gif,201);
  gtk_widget_set_sensitive(gui.edit_undo,FALSE);
}

gboolean  app_delete(GtkWidget *w, GdkEvent *e, gpointer data)
{
  return FALSE;
}

void      app_destroy(GtkWidget *w, gpointer data)
{
  app_tdebug(2001);
  gtk_main_quit();
}

void      app_session_new(CBARG) {
  session_new();
}

void      app_session_open(CBARG) {
  session_open();
}

void      app_session_save(CBARG) {
  session_save();
}

void      app_session_save_as(CBARG) {
  session_save_as();
}

void      app_menu_close(CBARG) {
  app_tdebug(2002);
  gtk_main_quit();
}

void      app_menu_dicom(CBARG) {
  dicom_import_dialog();
}

void      app_menu_about(CBARG) {
  about_dialog();
}

void      app_menu_open(CBARG) {
  char name[512];
  ArgBlock *args;
  int interp;
  
  if (util_get_filename2(mw, "Load Volume", name, GF_ENSURE_READABLE, "Isotropic Interpolation",&interp)) {
    args = ArgBlockNew();
    args->vols[0] = &(voldata.original);
    args->ia[0] = interp ? 1 : 0;
    strncpy(args->sa, name, 512);
    args->ack = &(notify.datachanged);
    TaskNew(res.bgqueue, "Loading Volume", (Callback) &bg_load_volume, 
	    (void *)args);
  }

}

void app_cmdline_openseg(const char *path) {
  int len;
  len = strlen(path);
  if (strcasecmp(&path[len-3],"scn")==0 ||
      strcasecmp(&path[len-3],"hdr")==0 ||
      strcasecmp(&path[len-3],"img")==0 )
    {
      TaskNew(res.bgqueue, "Loading Segmentation", (Callback) &bg_load_segmentation, 
	      (void *) (strdup(path)));
    }
  else {
    fprintf(stderr, "** unrecognized extension in %s\n",path);
  }
}

void app_cmdline_open(const char *path) {
  
  int len;
  ArgBlock *args;
  len = strlen(path);
  if (strcasecmp(&path[len-3],"scn")==0 ||
      strcasecmp(&path[len-3],"hdr")==0 ||
      strcasecmp(&path[len-3],"img")==0 )
    {
      args = ArgBlockNew();
      args->vols[0] = &(voldata.original);
      args->ia[0] = 2;
      strncpy(args->sa, path,512);
      args->ack = &(notify.datachanged);
      TaskNew(res.bgqueue, "Loading Volume", (Callback) &bg_load_volume, 
	      (void *)args);
    }
  else {
    fprintf(stderr, "** unrecognized extension in %s\n",path);
  }
}

gboolean  canvas_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data)
{
  int i,w,h;
  char *st;
  char buf[512];
  int si,sn;
  
  gdk_window_get_size(widget->window, &w, &h);

  if (view.W != w || view.H != h || canvas==0 || gc==0) {
    if (gc) {
      gdk_gc_destroy(gc);
      gc = 0;
    }
    if (canvas) {
      gdk_pixmap_unref(canvas);
      canvas = 0;
    }
    view.invalid = 1;
    view.W = w;
    view.H = h;
    canvas = gdk_pixmap_new(widget->window,w,h,-1);
    gc = gdk_gc_new(canvas);
  }

  if (view.invalid)
    canvas_calc();

  for(i=0;i<4;i++)
    if (view.enable[i]) {
      if (view.buf[i]) {
	if (view.buf[i]->W != view.bw[i] || view.buf[i]->H != view.bh[i]) {
	  CImgDestroy(view.buf[i]);
	  view.buf[i] = 0;
	}
      }
      if (!view.buf[i])
	view.buf[i] = CImgNew(view.bw[i], view.bh[i]);
    }

  for(i=0;i<3;i++)
    if (view.enable[i]) {
      canvas_draw_2D(i);
      gdk_draw_rgb_image(canvas,gc,
			 view.bx[i],view.by[i],
			 view.bw[i],view.bh[i],
			 GDK_RGB_DITHER_NORMAL,
			 view.buf[i]->val,
			 view.buf[i]->rowlen);

      if (v2d.mpane == i && (gui.t2d->selected == 3 || gui.t2d->selected == 4)) {
	v2d_pen_cursor(canvas,gc);
      }

    }

  if (view.enable[3]) {
    canvas_draw_3D();
    gdk_draw_rgb_image(canvas,gc,
		       view.bx[3],view.by[3],
		       view.bw[3],view.bh[3],
		       GDK_RGB_DITHER_NORMAL,
		       view.buf[3]->val,
		       view.buf[3]->rowlen);
  }

  view.invalid = 0;

  if (busy.active) {
    if (!busy.buf)
      busy.buf = CImgNew(416,80);

    CImgDrawBevelBox(busy.buf,0,0,416,80,view.pbg,0,1);
    CImgDrawSText(busy.buf,res.font[4],16,14,0,busy.msg);

    CImgDrawBevelBox(busy.buf,16,48,384,16, 0xffffff, 1, 0);

    if (!busy.ticker) {
      busy.ticker = CImgNew(480,16);
      CImgFill(busy.ticker, 0xffffff);

      for(i=0;i<16;i++) {
	CImgDrawLine(busy.ticker,i,15,i+16,-1,0xff0000);
      }
      for(i=1;i<15;i++)
	CImgBitBlt(busy.ticker,i*32,0,busy.ticker,0,0,32,16);
    }

    i = GetProgress();
    if (i < 0)
      CImgBitBlt(busy.buf, 17,49, busy.ticker, 32-(busy.clock%32), 0, 382, 14);
    else if (i > 0)
      CImgDrawBevelBox(busy.buf,16,48,(int)(((float)i)*3.84),16,0xff0000,1,1);

    st = SubTaskGet(&si,&sn);
    if (st!=0) {
      snprintf(buf,512,"Step %d of %d: %s",si,sn,st);
      free(st);
      CImgDrawSText(busy.buf,res.font[4],16,30,0x800000,buf);
    }

    gdk_draw_rgb_image(canvas,gc,
		       (view.W - 416)/2,
		       (view.H - 80)/2, 416,80,
		       GDK_RGB_DITHER_NORMAL,
		       busy.buf->val,
		       busy.buf->rowlen);    
  }

  // commit result
  gdk_draw_pixmap(widget->window, widget->style->black_gc,
		  canvas, 0,0, 0,0, view.W, view.H);
  return FALSE;
}

void canvas_draw_2D(int i) {
  int dbg,bg,fg,bbg;
  CImg *dest;
  int w,h,ox,oy,x=0,y=0,j,k,rx,ry,W,H,w2,h2;
  int ovy;
  char z[128],buf[2];

  if (v2d.invalid) {
    //    printf("v2dcompose\n");
    v2d_compose();
  }
  
  bg = view.bg;
  dbg = darker(bg, DTITLE);
  bbg = BFUNC(bg,BVAL);
  fg = view.fg;
  w = view.bw[i];
  h = view.bh[i];
  dest = view.buf[i];
  
  ox = view.bx[i];
  oy = view.by[i];

  // bg
  CImgFill(dest, bg);

  if (v2d.base[i] != 0) {
    x = (w / 2) - v2d.scaled[i]->W / 2 + v2d.panx[i];
    y = (h / 2) - v2d.scaled[i]->H / 2 + v2d.pany[i];
    CImgBitBlt(dest, x,y, v2d.scaled[i], 0,0, 
	       v2d.scaled[i]->W, v2d.scaled[i]->H);
    // border and crosshair
    CImgDrawRect(dest, x,y, v2d.scaled[i]->W, v2d.scaled[i]->H,fg);
    v2d_v2s(v2d.cx,v2d.cy,v2d.cz,i,&rx,&ry);
    CImgDrawLine(dest, rx, y, rx, y+v2d.scaled[i]->H-1, view.crosshair);
    CImgDrawLine(dest, x, ry, x+v2d.scaled[i]->W-1, ry, view.crosshair);

    if (v2d.info[i][0] != 0)
      CImgDrawShieldedSText(dest,res.font[4],4,view.bh[i]-16,0x00ff00,0,v2d.info[i]);

    if (v2d.overlay[1]) {

      if (orient.main != ORIENT_UNKNOWN) {
	W = dest->W;
	H = dest->H;
	w2 = W/2;
	h2 = H/2;
	
	buf[1] = 0;
	switch(i) {
	case 0:
	  buf[0] = OriKeys[orient.mainorder[0]];
	  CImgDrawShieldedSText(dest,res.font[5],w2,20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[2]];
	  CImgDrawShieldedSText(dest,res.font[5],w2,H-20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[3]];
	  CImgDrawShieldedSText(dest,res.font[5],5,h2,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[1]];
	  CImgDrawShieldedSText(dest,res.font[5],W-20,h2,0xffff00,0,buf);
	  break;
	case 1:
	  buf[0] = OriKeys[orient.mainorder[0]];
	  CImgDrawShieldedSText(dest,res.font[5],w2,20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[2]];
	  CImgDrawShieldedSText(dest,res.font[5],w2,H-20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.zlo];
	  CImgDrawShieldedSText(dest,res.font[5],5,h2,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.zhi];
	  CImgDrawShieldedSText(dest,res.font[5],W-20,h2,0xffff00,0,buf);
	  break;      
	case 2:
	  buf[0] = OriKeys[orient.zlo];
	  CImgDrawShieldedSText(dest,res.font[5],w2,20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.zhi];
	  CImgDrawShieldedSText(dest,res.font[5],w2,H-20,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[3]];
	  CImgDrawShieldedSText(dest,res.font[5],5,h2,0xffff00,0,buf);
	  buf[0] = OriKeys[orient.mainorder[1]];
	  CImgDrawShieldedSText(dest,res.font[5],W-20,h2,0xffff00,0,buf);
	  break;
	}
      } 
    }

    if (v2d.overlay[0]) {
      ovy = 20;
      if (strlen(patinfo.name)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.name);
	ovy += 12;
      }
      if (strlen(patinfo.age)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.age);
	ovy += 12;
      }
      if (patinfo.gender!=GENDER_U) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			      patinfo.gender==GENDER_M?"M":"F");
	ovy += 12;
      }
      if (strlen(patinfo.scandate)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.scandate);
	ovy += 12;
      }
      if (strlen(patinfo.institution)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.institution);
	ovy += 12;
      }
      if (strlen(patinfo.equipment)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.equipment);
	ovy += 12;
      }
      if (strlen(patinfo.protocol)) {
	CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,patinfo.protocol);
	ovy += 12;
      }
    }
  }

  // draw zoom overlay, if any
  if (view.zopane == i) {
    j = view.zolen * ((v2d.zoom - 0.50) / 7.5);

    CImgFill(view.zobuf,0);
    CImgFillRect(view.zobuf,0,0,j,8,0xffcc80);
    CImgDrawRect(view.zobuf,0,0,view.zolen,8,0xff8000);
    CImgBitBlm(dest,view.zox,view.zoy-4,view.zobuf,0,0,view.zolen,8,0.45);

    CImgDrawShieldedSText(dest,res.font[3],view.zox, view.zoy-18,0xffcc80,0,"50%");
    CImgDrawShieldedSText(dest,res.font[3],view.zox+view.zolen,view.zoy-18,0xffcc80,0,"800%");
    snprintf(z,128,"%d%%",(int)(v2d.zoom * 100.0));
    CImgDrawShieldedSText(dest,res.font[3],view.zox+j, view.zoy-18,0xffcc80,0,z);
  }

  if (view.bcpane == i) {
    CImgFillRect(dest,0,0,4,view.bch,0xff0000);
    j = (int) (gui.bri->value * ((float)(view.bch)));
    j = view.bch - j;
    CImgFillTriangle(dest,4,j,8,j-4,8,j+4,0xff0000);
    CImgDrawShieldedSText(dest,res.font[3],12,j-5,0xff0000,0x000000,"Brightness");
    CImgDrawGlyph(dest,gui.bri->icon,22,j-21,0xff0000);

    j = (int) (view.bcpx * ((float)(view.bcw)));
    k = (int) (view.bcpy * ((float)(view.bch)));
    k = view.bch - k;
    CImgFillCircle(dest,j,k,10,0);
    CImgDrawLine(dest,j-4,k-4,j+4,k+4,0xffff00);
    CImgDrawLine(dest,j+4,k-4,j-4,k+4,0xffff00);
    CImgDrawCircle(dest,j,k,8,0xffff00);

    CImgFillRect(dest,0,view.bch-4,view.bcw,4,0xffff00);
    j = (int) (gui.con->value * ((float)(view.bcw)));
    CImgFillTriangle(dest,j,view.bch-4,j-4,view.bch-8,j+4,view.bch-8,0xffff00);
    CImgDrawShieldedSText(dest,res.font[3],j-27,view.bch-22,0xffff00,0x000000,"Contrast");
    CImgDrawGlyph(dest,gui.con->icon,j-8,view.bch-38,0xffff00);
  }

  // header
  CImgFillRect(dest, 0,0, w, 16, dbg);
  CImgDrawLine(dest, 0,16,w,16, fg);
  
  CImgDrawBevelBox(dest, w-16,0,16,17, bbg, fg, 1);
  CImgDrawGlyph(dest, (view.maximized==i) ? res.gmin : res.gmax,
		view.bmax[i].x - ox, view.bmax[i].y - oy, fg);
  
  CImgDrawBoldSText(dest, res.font[2], 4, 2, fg, vname[i]);
  CImgDrawRect(dest, 0,0,w,h, fg);
  
  if (v2d.base[i] != 0) {
    switch(i) {
    case 0: j = v2d.cz + 1; k = voldata.original->D; break;
    case 1: j = v2d.cx + 1; k = voldata.original->W; break;
    case 2: j = v2d.cy + 1; k = voldata.original->H; break;
    default: j = -1; k = -1;
    }
    snprintf(z,128,"Slice %d of %d",j,k);
    CImgDrawSText(dest, res.font[1], 64, 2, fg, z);

    CImgDrawBevelBox(dest, 200,0,16,17, bbg, fg, 1);
    CImgDrawBevelBox(dest, 200+16,0,16,17, bbg, fg, 1);
    CImgDrawGlyph(dest, res.gleft,200, 0, fg);
    CImgDrawGlyph(dest, res.gright,200+16, 0, fg);

  }
  
}

void canvas_draw_3D() {
  int dbg;
  int w,h,ox,oy,dw,dh,j;
  CImg *dest;
  char z[64];

  dbg = darker(view.bg, DTITLE);
  w = view.bw[3];
  h = view.bh[3];
  dest = view.buf[3];

  dw = 2 + (int) (((float)w) / v3d.zoom);
  dh = 2 + (int) (((float)h) / v3d.zoom);

  if (v3d.base)
    if (v3d.base->W != dw || v3d.base->H != dh)
      v3d.invalid = 1;

  if (!v3d.base)
    v3d.invalid = 1;

  if (v3d.invalid)
    view3d_compose(w,h,view.bg);

  ox = view.bx[3];
  oy = view.by[3];

  CImgFill(dest, view.bg);

  if (v3d.scaled != 0)
    CImgBitBlt(dest, w/2 - (v3d.scaled->W / 2), h/2 - (v3d.scaled->H / 2), 
	       v3d.scaled, 0,0, v3d.scaled->W, v3d.scaled->H);

  // draw zoom overlay, if any
  if (view.zopane == 3) {
    j = view.zolen * ((v3d.zoom - 0.50) / 7.5);

    CImgFill(view.zobuf,0);
    CImgFillRect(view.zobuf,0,0,j,8,0xffcc80);
    CImgDrawRect(view.zobuf,0,0,view.zolen,8,0xff8000);
    CImgBitBlm(dest,view.zox,view.zoy-4,view.zobuf,0,0,view.zolen,8,0.45);

    CImgDrawShieldedSText(dest,res.font[3],view.zox, view.zoy-18,0xffcc80,0,"50%");
    CImgDrawShieldedSText(dest,res.font[3],view.zox+view.zolen,view.zoy-18,0xffcc80,0,"800%");
    snprintf(z,64,"%d%%",(int)(v3d.zoom * 100.0));
    CImgDrawShieldedSText(dest,res.font[3],view.zox+j, view.zoy-18,0xffcc80,0,z);
  }

  CImgFillRect(dest, 0,0, w, 16, dbg);
  CImgDrawLine(dest, 0,16,w,16, view.fg);

  CImgDrawBevelBox(dest, w-16,0,16,17, BFUNC(view.bg,BVAL), view.fg, 1);
  CImgDrawGlyph(dest, (view.maximized==3) ? res.gmin : res.gmax,
		view.bmax[3].x - ox, view.bmax[3].y - oy, view.fg);

  CImgDrawBoldSText(dest, res.font[2], 4, 2, view.fg, vname[3]);
  CImgDrawRect(dest, 0,0,w,h, view.fg);

  // whatever
  if (v3d.octant.enable)
    refresh(gui.octcw);
}

void canvas_calc() {
  int i;
  switch(view.maximized) {
  case 0:
    view.bx[0] = view.by[0] = 0;
    view.bw[0] = view.W; 
    view.bh[0] = view.H;
    view.enable[0] = 1;
    view.enable[1] = view.enable[2] = view.enable[3] = 0;
    break;
  case 1:
    view.bx[1] = view.by[1] = 0;
    view.bw[1] = view.W; 
    view.bh[1] = view.H;
    view.enable[1] = 1;
    view.enable[0] = view.enable[2] = view.enable[3] = 0;
    break;
  case 2:
    view.bx[2] = view.by[2] = 0;
    view.bw[2] = view.W; 
    view.bh[2] = view.H;
    view.enable[2] = 1;
    view.enable[0] = view.enable[1] = view.enable[3] = 0;
    break;
  case 3:
    view.bx[3] = view.by[3] = 0;
    view.bw[3] = view.W; 
    view.bh[3] = view.H;
    view.enable[3] = 1;
    view.enable[0] = view.enable[1] = view.enable[2] = 0;
    break;
  default:
    view.bx[0] = view.by[0] = 0;
    view.bx[1] = view.W / 2;
    view.by[1] = 0;
    view.bx[2] = 0;
    view.by[2] = view.H / 2;
    view.bx[3] = view.W / 2;
    view.by[3] = view.H / 2;
    for(i=0;i<4;i++) {
      view.enable[i] = 1;
      view.bw[i] = view.W / 2;
      view.bh[i] = view.H / 2;
    }
    break;
  }

  for(i=0;i<4;i++)
    if (view.enable[i]) {
      BoxAssign(&(view.bmax[i]), 
		view.bx[i] + view.bw[i] - 16,
		view.by[i], 16, 16);
      BoxAssign(&(view.blef[i]), view.bx[i] + 200, view.by[i], 16,16);
      BoxAssign(&(view.brig[i]), view.bx[i] + 216, view.by[i], 16,16);
    }
}

struct {
  int lx,ly;
  int pane;
  int active;
} drag = {0,0,-1,0} ;
  

// DRAG
gboolean  canvas_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data)
{
  int x,y,b,i,ax,ay,az;
  int vk,v[4],vi,label;
  int dx,dy;
  Transform tx,ty;
  int nc, ctrl;

  static int *ddabuffer = 0;
  static int ddasize = 0;
  int ddacount;

  x = (int) em->x; y = (int) em->y; 
  ctrl = (em->state & GDK_CONTROL_MASK);
  b = 0;
  if (em->state & GDK_BUTTON1_MASK) b=1;
  if (em->state & GDK_BUTTON2_MASK) b=2;
  if (em->state & GDK_BUTTON3_MASK) b=3;

  if (b==0) {

    switch(view.maximized) {
    case -1:
      nc = (x >= view.bx[3] && y >= view.by[3]) ? 3 : 2;
      if (inbox(x,y,view.bx[0],view.by[0],view.bw[0],16)||
	  inbox(x,y,view.bx[1],view.by[1],view.bw[1],16)||
	  inbox(x,y,view.bx[2],view.by[2],view.bw[2],16)||
	  inbox(x,y,view.bx[3],view.by[3],view.bw[3],16))
	nc = 1;
      break;
    case 3:
      nc = 3;
      if (inbox(x,y,view.bx[view.maximized],
		view.by[view.maximized],
		view.bw[view.maximized],16))
	nc = 1;
      break;
    default:
      nc = 2;
      if (inbox(x,y,view.bx[view.maximized],
		view.by[view.maximized],
		view.bw[view.maximized],16))
	nc = 1;
      break;
    }

    if (nc != gui.cstate) {
      gui.cstate = nc;
      switch(nc) {
      case 1:
	//	printf("1\n");
	gdk_window_set_cursor(cw->window, NULL);
	break;
      case 2:
	//      printf("2\n");
	gdk_window_set_cursor(cw->window, gui.cursor2[ gui.t2d->selected ]);
	break;
      case 3:
	//      printf("3\n");
	gdk_window_set_cursor(cw->window, gui.cursor3[ gui.t3d->selected ]);
	break;
      }
    }

  }

  if (gui.t2d->selected == 3 || gui.t2d->selected == 4 || v2d.mpane >= 0) {
    v2d.mx = x;
    v2d.my = y;
    v2d.mpane = -1;
    for(i=0;i<4;i++)
      if (view.enable[i] && InBoxXYWH(x,y,view.bx[i],view.by[i]+16,view.bw[i],view.bh[i]-16)) {
	v2d.mpane = i;
	break;
      }
    refresh(cw);
  }

  if (drag.pane < 0 || !drag.active) return FALSE;
  i = drag.pane;

  if (view.enable[i]) {
    if (voldata.original!=0) {

      // crosshair position tool
      if (i<3 && gui.t2d->selected==0 && b==1) {
	v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
	    ay<voldata.original->H && az<voldata.original->D) {


	  if (!roi.active || ctrl!=0) {
	    v2d.cx = ax;
	    v2d.cy = ay;
	    v2d.cz = az;
	    force_redraw_2D();
	    if (v3d.octant.enable)
	      app_queue_octant_redraw();
	  } else {
	    roi_edit(ax,ay,az,i);
	    force_redraw_2D();
	  }
	  return FALSE;
	}
      }

      // bri/con control
      if (i<3 && gui.t2d->selected==0 && b==3) {
	bc_drag(x,y);
	return FALSE;
      }

      // pan tool
      if (i<3 && gui.t2d->selected==1 && b==1) {
	v2d.panx[i] += (x-v2d.tpan[0]);
	v2d.pany[i] += (y-v2d.tpan[1]);
	v2d.tpan[0] = x;
	v2d.tpan[1] = y;
	force_redraw_2D();
	return FALSE;
      }

      // zoom tool
      if (i<3 && gui.t2d->selected==2 && b==1) {
	zo_drag(x,y);
	return FALSE;
      }

      // pencil and eraser
      if (i<3 && (gui.t2d->selected==3 || gui.t2d->selected==4) && (b==1 || b==3)) {

	label = labels.current + 1;
	if (gui.t2d->selected == 4) label = 0; // eraser tool
	
	vk = abs(x-drag.lx)+abs(y-drag.ly)+2;
	if (ddasize < vk) {
	  if (ddabuffer == 0)
	    ddabuffer = (int *) MemAlloc(sizeof(int) * 4 * vk);
	  else
	    ddabuffer = (int *) MemRealloc(ddabuffer, sizeof(int) * 4 * vk);
	  if (!ddabuffer) {
	    error_memory();
	    return FALSE;
	  }
	  ddasize = vk;
	}
	ddacount = 1;
	ddabuffer[0] = x;
	ddabuffer[1] = y;
	ddabuffer[2] = drag.lx;
	ddabuffer[3] = drag.ly;
	
	while(ddacount) {
	  
	  for(vi=0;vi<4;vi++)
	    v[vi] = ddabuffer[(ddacount-1)*4 + vi];
	  ddacount--;
	  
	  vk = sqdist(v[0],v[1],v[2],v[3]);
	  
	  switch(vk) {
	  case 0: // same point
	    v2d_s2v(v[0]-view.bx[i],v[1]-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D)
	      v2d_trace(ax,ay,az,i,1,label,(b==3));
	    break;
	  case 1: 
	  case 2: // two adjacent points
	    v2d_s2v(v[0]-view.bx[i],v[1]-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D)
	      v2d_trace(ax,ay,az,i,1,label,(b==3));
	    v2d_s2v(v[2]-view.bx[i],v[3]-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D)
	      v2d_trace(ax,ay,az,i,1,label,(b==3));
	    break;		
	  default: // need dda
	    ddabuffer[ddacount*4 + 0] = v[0];
	    ddabuffer[ddacount*4 + 1] = v[1];
	    ddabuffer[ddacount*4 + 2] = (v[0]+v[2])/2;
	    ddabuffer[ddacount*4 + 3] = (v[1]+v[3])/2;
	    ddacount++;
	    ddabuffer[ddacount*4 + 0] = v[2];
	    ddabuffer[ddacount*4 + 1] = v[3];
	    ddabuffer[ddacount*4 + 2] = (v[0]+v[2])/2;
	    ddabuffer[ddacount*4 + 3] = (v[1]+v[3])/2;
	    ddacount++;
	  }
	}
	
	drag.lx = x;
	drag.ly = y;
	return FALSE;
      }

      // contour 1 (drag)
      if (i<3 && (gui.t2d->selected==5) && b==1) {
	v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	
	switch(i) {
	case 0: 
	  contour_locate(az,i);
	  contour_add_point(ax,ay);
	  force_redraw_2D();
	  break;
	case 1:
	  contour_locate(ax,i);
	  contour_add_point(az,ay);
	  force_redraw_2D();
	  break;
	case 2:
	  contour_locate(ay,i);
	  contour_add_point(ax,az);
	  force_redraw_2D();
	  break;
	}
	
	return FALSE;
      }

      // editing distance
      if (i<3 && gui.t2d->selected==7 && b==3) {
	v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);	  
	dist_edit_set(ax,ay,az,voldata.original);
	force_redraw_2D();
	force_redraw_3D();
	return FALSE;
      }

      // editing angle
      if (i<3 && gui.t2d->selected==8 && b==3) {
	v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	angle_edit_set(ax,ay,az,voldata.original);
	force_redraw_2D();
	force_redraw_3D();
	return FALSE;
      }
      
      // 3D: rotation
      if (i==3 && (gui.t3d->selected==0) && b==1) {
	if (drag.lx < 0) return FALSE;
	dx = x - drag.lx;
	dy = y - drag.ly;
	while (dx < 360) dx+=360;
	while (dx > 360) dx-=360;
	while (dy > 360) dy-=360;
	while (dy < 360) dy+=360;
	TransformYRot(&ty, dx);
	TransformXRot(&tx, dy);
	TransformCompose(&ty,&tx);
	TransformCopy(&(v3d.wirerotation),&(v3d.rotation));
	TransformCompose(&(v3d.wirerotation),&ty);
	view3d_invalidate(0,1);
	force_redraw_3D();
	return FALSE;
      }

      // 3D: pan
      if (i==3 && (gui.t3d->selected==1) && b==1) {
	if (drag.lx < 0) return FALSE;
	dx = x - drag.lx;
	dy = y - drag.ly;
	v3d.panx = v3d.savepanx + dx;
	v3d.pany = v3d.savepany + dy;
	TransformCopy(&(v3d.wirerotation),&(v3d.rotation));
	view3d_invalidate(0,1);
	force_redraw_3D();
	return FALSE;
      }

      if (i==3 && (gui.t3d->selected==2) && b==1) {
	zo_drag(x,y);
	return FALSE;
      }

    } // original!=0
  } // enable[i]

  return FALSE;
}

// CANVAS_PRESS
gboolean  canvas_press(GtkWidget *widget, GdkEventButton *eb, gpointer data)
{
  int i,x,y,b;
  int ax,ay,az,label;
  int ctrl = 0;

  app_tdebug(2003);

  x = (int) (eb->x);
  y = (int) (eb->y);
  b = eb->button;
  ctrl = (eb->state & GDK_CONTROL_MASK);

  for(i=0;i<4;i++)
    if (view.enable[i]) {
      /* maximize buttons */
      if (b==1 && InBox(x,y,&(view.bmax[i]))) {
	if (view.maximized < 0) view.maximized = i; else view.maximized = -1;
	view.invalid = 1;
	drag.pane = -1;
	drag.active = 0;
	if (view.maximized < 0) {
	  gtk_widget_show(gui.kf);
	  gtk_widget_show(gui.kl);
	} else {
	  gtk_widget_hide(gui.kf);
	  gtk_widget_hide(gui.kl);
	}
	refresh(cw);
	return FALSE;
      }
      /* left/right buttons */
      if (b==1 && voldata.original!=NULL && InBox(x,y,&(view.blef[i]))) {
	v2d_left(i);
	drag.pane = -1;
	drag.active = 0;
	return FALSE;
      }
      if (b==1 && voldata.original!=NULL && InBox(x,y,&(view.brig[i]))) {
	v2d_right(i);
	drag.pane = -1;
	drag.active = 0;
	return FALSE;
      }

      if (voldata.original!=NULL) {
	if (InBoxXYWH(x,y,view.bx[i],view.by[i],view.bw[i],view.bh[i])) {
	  
	  // zoom tool
	  if (i<3 && gui.t2d->selected==2) {
	    
	    if (b==1) {
	      zo_start(x,y,i);
	      drag.pane = i;
	      drag.active = 1;
	      return FALSE;
	    }

	    if (b==3) {
	      v2d.zoom = 1.0;
	      force_redraw_2D();
	      drag.pane = -1;
	      drag.active = 0;
	      return FALSE;
	    }

	  }
	  
	  // crosshair position tool
	  if (i<3 && gui.t2d->selected==0 && b==1) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D) {

	      if (!roi.active || ctrl!=0) {
		v2d.cx = ax;
		v2d.cy = ay;
		v2d.cz = az;
		force_redraw_2D();
		if (v3d.octant.enable)
		  app_queue_octant_redraw();
		drag.pane = i;
		drag.active = 1;
	      } else {
		roi_edit(ax,ay,az,i);
		force_redraw_2D();
		drag.pane = i;
		drag.active = 1;
	      }

	      return FALSE;
	    }
	  }

	  // bri/con control
	  if (i<3 && gui.t2d->selected==0 && b==3) {
	    bc_start(x,y,i);
	    drag.pane = i;
	    drag.active = 1;
	    return FALSE;
	  }

	  // pan tool
	  if (i<3 && gui.t2d->selected==1 && b==1) {
	    v2d.tpan[0] = x;
	    v2d.tpan[1] = y;
	    drag.pane = i;
	    drag.active = 1;
	    return FALSE;
	  }
	  if (i<3 && gui.t2d->selected==1 && b==3) {
	    v2d.panx[i] = v2d.pany[i] = 0;
	    force_redraw_2D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // eraser: kill slice
	  if (i<3 && gui.t2d->selected==4 && b==3 && ctrl) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D)
	      {
		undo_push(i);
		v2d_purge(ax,ay,az,i,1);
	      }
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // pencil disk/sphere
	  if (i<3 && (gui.t2d->selected==3 || gui.t2d->selected==4) && (b==1 || b==3)) {
	    label = labels.current + 1;
	    if (gui.t2d->selected == 4) label = 0; // eraser tool

	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    if (ax>=0 && ay>=0 && az>=0 && ax<voldata.original->W &&
		ay<voldata.original->H && az<voldata.original->D) 
	      {
		undo_push(i);
		if (gui.t2d->selected==3 && b==3 && ctrl) view3d_find_skin(&ax,&ay,&az);
		v2d_trace(ax,ay,az,i,1,label,(b==3));
	      }
	    drag.lx = x;
	    drag.ly = y;
	    drag.pane = i;
	    drag.active = 1;
	    return FALSE;
	  }

	  // contour 1 (drag)
	  if (i<3 && (gui.t2d->selected==5) && b==1) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);

	    contour_reset();
	    switch(i) {
	    case 0: 
	      contour_locate(az,i);
	      contour_add_point(ax,ay);
	      force_redraw_2D();
	      break;
	    case 1:
	      contour_locate(ax,i);
	      contour_add_point(az,ay);
	      force_redraw_2D();
	      break;
	    case 2:
	      contour_locate(ay,i);
	      contour_add_point(ax,az);
	      force_redraw_2D();
	      break;
	    }
	    drag.pane = i;
	    drag.active = 1;
	    return FALSE;
	  }
	  
	  // contour 1 (undo)
	  if (i<3 && (gui.t2d->selected==5) && b==2) {
	    contour_undo(voldata.label);
	    force_redraw_2D();

	    if (v3d.obj.enable)
	      app_queue_obj_redraw();

	    drag.pane = -1;
	    drag.active = 0;

	    return FALSE;
	  }

	  // contour 2 (discrete points)
	  if (i<3 && (gui.t2d->selected==6) && b==1) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);

	    switch(i) {
	    case 0: 
	      contour_locate(az,i);
	      contour_add_point(ax,ay);
	      force_redraw_2D();
	      break;
	    case 1:
	      contour_locate(ax,i);
	      contour_add_point(az,ay);
	      force_redraw_2D();
	      break;
	    case 2:
	      contour_locate(ay,i);
	      contour_add_point(ax,az);
	      force_redraw_2D();
	      break;
	    }
	    
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // contour 2 (discrete points, reset)
	  if (i<3 && (gui.t2d->selected==6) && b==2) {
	    contour_reset();
	    force_redraw_2D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // contour 2 (discrete points, close)
	  if (i<3 && (gui.t2d->selected==6) && b==3) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);

	    switch(i) {
	    case 0: 
	      contour_locate(az,i);
	      contour_add_point(ax,ay);
	      break;
	    case 1:
	      contour_locate(ax,i);
	      contour_add_point(az,ay);
	      break;
	    case 2:
	      contour_locate(ay,i);
	      contour_add_point(ax,az);
	      break;
	    }

	    undo_push(i);
	    contour_close(voldata.label, labels.current+1);
	    force_redraw_2D();

	    if (v3d.obj.enable)
	      app_queue_obj_redraw();

	    drag.pane = -1;
	    drag.active = 0;

	    return FALSE;
	  }

	  // measurement, line
	  if (i<3 && (gui.t2d->selected==7) && b==1 && !ctrl) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    dist_add_point(ax,ay,az,voldata.original);
	    force_redraw_2D();
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  if (i<3 && (gui.t2d->selected==7) && b==1 && ctrl) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    dist_kill_closer(ax,ay,az);
	    force_redraw_2D();
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // measurement, angle 
	  if (i<3 && (gui.t2d->selected==8) && b==1 && !ctrl) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    angle_add_point(ax,ay,az,voldata.original);
	    force_redraw_2D();
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  if (i<3 && (gui.t2d->selected==8) && b==1 && ctrl) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    angle_kill_closer(ax,ay,az);
	    force_redraw_2D();
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // distance edit 
	  if (i<3 && (gui.t2d->selected==7) && b==3) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);

	    dist_edit_init(ax,ay,az);
	    drag.pane = i;
	    drag.active = 1;

	    return FALSE;
	  }

	  // angle edit 
	  if (i<3 && (gui.t2d->selected==8) && b==3) {
	    v2d_s2v(x-view.bx[i],y-view.by[i],i,&ax,&ay,&az);
	    
	    angle_edit_init(ax,ay,az);
	    drag.pane = i;
	    drag.active = 1;

	    return FALSE;
	  }

	  // 3D: rotation
	  if (i==3 && (gui.t3d->selected==0) && b==1) {
	    drag.lx = x;
	    drag.ly = y;
	    v3d.wireframe = 1;
	    drag.pane = i;
	    drag.active = 1;
	    force_redraw_3D();
	    return FALSE;
	  }

	  // 3D: rotation
	  if (i==3 && (gui.t3d->selected==0) && b==3) {
	    TransformIdentity(&(v3d.rotation));
	    TransformIdentity(&(v3d.wirerotation));
	    view3d_invalidate(0,1);
	    v3d.wireframe = 0;
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // 3D: pan
	  if (i==3 && (gui.t3d->selected==1) && b==1) {
	    drag.lx = x;
	    drag.ly = y;
	    v3d.savepanx = v3d.panx;
	    v3d.savepany = v3d.pany;
	    v3d.wireframe = 1;
	    drag.pane = i;
	    drag.active = 1;
	    return FALSE;
	  }

	  // 3D: pan
	  if (i==3 && (gui.t3d->selected==1) && b==3) {
	    v3d.panx = 0.0;
	    v3d.pany = 0.0;
	    view3d_invalidate(0,1);
	    v3d.wireframe = 0;
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	  // 3D: zoom
	  if (i==3 && gui.t3d->selected==2) {

	    if (b==1) {
	      v3d.wireframe = 1;
	      TransformCopy(&(v3d.wirerotation),&(v3d.rotation));
	      zo_start(x,y,i);
	      drag.pane = i;
	      drag.active = 1;
	      return FALSE;
	    }
	    
	    if (b==3) {
	      v3d.zoom = 1.0;
	      view3d_invalidate(0,1);
	      force_redraw_3D();
	      drag.pane = -1;
	      drag.active = 0;
	      return FALSE;
	    }
	    
	  }

	  // 3D: geodesic ruler
	  if (i==3 && (gui.t3d->selected==3) && (b==1 || b==3)) {
	    georuler_set_point((x-view.bx[3])/v3d.zoom,(y-view.by[3])/v3d.zoom,(b==1));
	    //view3d_invalidate(0,1);
	    force_redraw_3D();
	    drag.pane = -1;
	    drag.active = 0;
	    return FALSE;
	  }

	} // XYWH
      } // original!=0
    }

  return FALSE;
}

// CANVAS_RELEASE
gboolean  canvas_release(GtkWidget *widget, GdkEventButton *eb, gpointer data)
{
  int x,y,b,dx,dy;
  int ax,ay,az,label;

  Transform tx,ty;

  app_tdebug(2004);

  x = (int) (eb->x);
  y = (int) (eb->y);
  b = eb->button;

  label = labels.current + 1;

  if (drag.pane < 0 || !drag.active)
    return FALSE;

  if (view.enable[drag.pane] && drag.active) {
    if (voldata.original!=NULL) {
	
      // bri/con control
      if (drag.pane<3 && (gui.t2d->selected==0) && b==3) {
	bc_end(x,y);
	drag.active = 0;
	drag.pane = -1;
	return FALSE;
      }

      // zoom 2D
      if (drag.pane<3 && (gui.t2d->selected==2) && b==1) {
	zo_end(x,y);
	drag.active = 0;
	drag.pane = -1;
	return FALSE;
      }

      // contour 1 (drag)
      if (drag.pane<3 && (gui.t2d->selected==5) && b==1) {
	  v2d_s2v(x-view.bx[drag.pane],y-view.by[drag.pane],drag.pane,&ax,&ay,&az);
	  
	  switch(drag.pane) {
	  case 0: 
	    contour_locate(az,drag.pane);
	    contour_add_point(ax,ay);
	    break;
	  case 1:
	    contour_locate(ax,drag.pane);
	    contour_add_point(az,ay);
	    break;
	  case 2:
	    contour_locate(ay,drag.pane);
	    contour_add_point(ax,az);
	    break;
	  }

	  undo_push(drag.pane);
	  contour_close(voldata.label, label);
	  force_redraw_2D();
	  
	  if (v3d.obj.enable)
	    app_queue_obj_redraw();
	  
	  return FALSE;
      }
      
      // 3D: rotation
      if (drag.pane==3 && (gui.t3d->selected==0) && b==1) {
	if (drag.lx < 0) return FALSE;
	dx = x - drag.lx;
	dy = y - drag.ly;
	while (dx < 360) dx+=360;
	while (dx > 360) dx-=360;
	while (dy > 360) dy-=360;
	while (dy < 360) dy+=360;
	TransformYRot(&ty, dx);
	TransformXRot(&tx, dy);
	TransformCompose(&ty,&tx);
	TransformCompose(&(v3d.rotation),&ty);
	v3d.wireframe = 0;
	view3d_invalidate(0,1);
	force_redraw_3D();
	drag.lx = -1;
	drag.active = 0;
	drag.pane = -1;
	return FALSE;
      }

      // 3D: pan
      if (drag.pane==3 && (gui.t3d->selected==1) && b==1) {
	if (drag.lx < 0) return FALSE;
	dx = x - drag.lx;
	dy = y - drag.ly;
	v3d.panx = v3d.savepanx + dx;
	v3d.pany = v3d.savepany + dy;
	v3d.wireframe = 0;
	view3d_invalidate(0,1);
	force_redraw_3D();
	drag.lx = -1;
	drag.active = 0;
	drag.pane = -1;
	return FALSE;
      }

      // zoom 3D
      if (drag.pane==3 && (gui.t3d->selected==2) && b==1) {
	v3d.wireframe = 0;
	zo_end(x,y);
	drag.active = 0;
	drag.pane = -1;
	return FALSE;
      }

    } // original!=0
  } // enable

  drag.active = 0;
  drag.pane = -1;
  force_redraw_2D();
  force_redraw_3D();
  return FALSE;
}

struct _status_data {
  char text[512];
  int  invalid;
  int  count;
  gint toid;
  Mutex *lock;
} status = { .count = 0, .toid = -1 };

void app_set_status(char *text, int timeout) {
  app_tdebug(2005);
  MutexLock(status.lock);
  strncpy(status.text, text, 511);
  status.text[511] = 0;
  status.invalid = 1;
  if (status.toid >= 0)
    gtk_timeout_remove(status.toid);
  status.toid=gtk_timeout_add(timeout*1000,app_status_clear,0);
  MutexUnlock(status.lock);
}

struct _title_data {
  int changed;
  char  *name;
  Mutex *lock;
} title;

void app_set_title_file(char *text) {
  char tmp[1024],*p;
  if (text) {
    strncpy(tmp,text,1024);
    p = basename(tmp);
    app_set_title(p);
  } else {
    app_set_title(NULL);
  }
}

void app_set_title(char *text) {
  MutexLock(title.lock);
  if (title.name) free(title.name);
  if (text)
    title.name = strdup(text);
  else
    title.name = 0;
  title.changed = 1;
  MutexUnlock(title.lock);
}

void app_title_check() {
  char tmp[512];
  app_tdebug(2006);
  if (title.changed) {
    MutexLock(title.lock);
    if (title.name)
      snprintf(tmp,512,"%s - %s",APPNAME,title.name);
    else
      strncpy(tmp,APPNAME,512);
    gtk_window_set_title(GTK_WINDOW(mw),tmp);
    title.changed = 0;
    MutexUnlock(title.lock);
  }
}

gboolean app_status_clear(gpointer data) {
  app_tdebug(2007);
  MutexLock(status.lock);
  status.toid = -1;
  if (status.count > 0) {
    gtk_statusbar_pop(GTK_STATUSBAR(gui.status),0);
    status.count--;
  }
  status.text[0] = 0;
  status.invalid = 0;
  MutexUnlock(status.lock);
  return FALSE;
}

gboolean app_timeout(gpointer data) {
  static int checkpoint[7] = {0,0,0,0,0,0,0};
  app_tdebug(2008);
  int rep2d = 0, rep3d = 0;
  int i;
  Task *t;

#ifdef TODEBUG
  FullTimer ft;
  printf("app_timeout in\n");
  StartTimer(&ft);
#endif

  /* update status line if needed */
  MutexLock(status.lock);
  if (status.invalid) {
    if (status.count > 0) {
      gtk_statusbar_pop(GTK_STATUSBAR(gui.status),0);
      status.count--;
    }
      
    gtk_statusbar_push(GTK_STATUSBAR(gui.status), 0, status.text);
    status.count++;
    status.invalid = 0;
    status.text[0] = 0;
  }
  MutexUnlock(status.lock);

  /* check title name change */
  app_title_check();

  /* show any queued message boxes */
  app_pop_consumer();

  /* verify counters */
  if (checkpoint[5] < notify.sessionloaded) {
    rep3d=1;
    rep2d=1;
    checkpoint[5] = notify.sessionloaded;
    update_controls();
  }

  // HERE
  if (checkpoint[0] < notify.datachanged) {
    if (voldata.original != NULL) {
      if (notify.labelsaver == 0 && voldata.label!=NULL) {
	VolumeDestroy(voldata.label);
	voldata.label = NULL;
      } else {
	notify.labelsaver--;
	if (notify.labelsaver < 0) notify.labelsaver=0;
      }
      view3d_invalidate(1,1);
    }
    measure_kill_all();

    undo_reset();
    v2d_reset();
    rep3d = 1;
    rep2d = 1;
    checkpoint[0] = notify.datachanged;
  }

  if (checkpoint[2] < notify.renderstep) {
    rep3d = 1;
    checkpoint[2] = notify.renderstep;
  }

  /* check if there is a bg task running */
  TaskQueueLock(res.bgqueue);
  i = GetTaskCount(res.bgqueue);

  if (i==0) {
    if (busy.active) {
      busy.active = 0;
      busy.clock = 0;
      rep2d = 1;
    }
  } else {
    t = GetNthTask(res.bgqueue,0);
    
    busy.active = 1;
    MemSet(busy.msg,0,64);
    strncpy(busy.msg, t->desc, 63);
    busy.done = -1;
    busy.clock+=3;
    rep2d = 1;
  }

  TaskQueueUnlock(res.bgqueue);

  if (view.invalid) rep2d = 1;

  if (checkpoint[1] < notify.viewchanged || rep2d) { 
    checkpoint[1] = notify.viewchanged;
    force_redraw_2D();
    undo_update();
  }

  if (checkpoint[3] < notify.segchanged) {
    view3d_objects_trash();
    rep3d = 1;
    force_redraw_2D();
    refresh(labels.widget);
    labels_changed();
    checkpoint[3] = notify.segchanged;
  }

  if (rep3d)
    force_redraw_3D();

  if (checkpoint[4] < notify.autoseg) {
    seg_dialog2();
    checkpoint[4] = notify.autoseg;
  }

#ifdef TODEBUG
  StopTimer(&ft);
  printf("app_timeout out, %.3f secs\n",TimeElapsed(&ft));
#endif

  return TRUE;
}

int *app_calc_lookup() {
  int *lookup;
  int i;
  float fi,bri;

  lookup = (int *) MemAlloc(sizeof(int) * 65536);
  if (!lookup) {
    error_memory();
    return 0;
  }

  if (!voldata.original) {
    MemSet(lookup,0,65536*sizeof(int));
    return lookup;
  }

  if (v2d.vmax == 0) v2d.vmax = 1;
  bri = gui.bri->value - 0.50;
  for(i=0;i<65536;i++) {
    fi = (((float) i)+bri*((float)v2d.vmax)) / ((float) v2d.vmax);
    fi *= (1.0 + 5.0 * gui.con->value);
    if (fi < 0.0) fi = 0.0;
    if (fi > 1.0) fi = 1.0;    
    lookup[i] = (int) (255.0 * fi);
  }
  
  return lookup;
}

void v2d_palette_changed(Slider *s) {
  force_redraw_2D();
  if (v3d.octant.enable)
    app_queue_octant_redraw();
}

int main(int argc, char **argv) {
  char *lang,*denv,*home;

  tdebug = 0;
  denv = getenv("BVTD"); // thread debugging
  if (denv)
    if (atoi(denv) > 0)
      tdebug = 1;
  good_thread = pthread_self();

  lang = getenv("LANG");
  if (lang) {
    if (strlen(lang) > 0) {
      setenv("LANG","",1);
      execvp(argv[0],argv);
    }
  }
  setenv("LC_NUMERIC","POSIX",1);

  setlocale(LC_ALL,"C");
  gtk_set_locale();
  gtk_init(&argc, &argv);
  gdk_rgb_init();
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());

  res.gmax   = GlyphNewInit(16,16,ICON_MAX);
  res.gmin   = GlyphNewInit(16,16,ICON_MIN);
  res.gleft  = GlyphNewInit(16,16,ICON_LEFT);
  res.gright = GlyphNewInit(16,16,ICON_RIGHT);
  
  res.font[0] = SFontFromFont(FontNew(8,13,FONT_8x13));
  res.font[1] = SFontFromFont(FontNew(8,13,FONT_8x13));
  res.font[2] = SFontFromFont(FontNew(8,13,FONT_8x13));
  res.font[3] = res.font[0];
  res.font[4] = res.font[1];
  res.font[5] = res.font[2];

  if (!res.font[3]) res.font[3] = res.font[0];
  if (!res.font[4]) res.font[4] = res.font[1];
  if (!res.font[5]) res.font[5] = res.font[2];

  home = getenv("HOME");
  if (home != NULL)
    strcpy(res.Home,home);
  else
    strcpy(res.Home,"/");

  status.invalid = 0;
  status.lock = MutexNew();
  title.lock = MutexNew();
  title.changed = 0;
  title.name    = 0;

  voldata.original = NULL;
  voldata.label = NULL;
  voldata.masterlock = MutexNew();

  app_start1(0);
  app_start2(0);

  if (argc>=2)
    app_cmdline_open(argv[1]);
  if (argc>=3) {
    notify.labelsaver++;
    app_cmdline_openseg(argv[2]);
  }

  gtk_main();

  return 0;
}

void app_t3d_changed(void *t) {
  char z[256],*m;

  app_tdebug(2009);
  z[0] = 0;

  switch(gui.t3d->selected) {
  case 0: m="<span foreground='#800000'>3D Rotation</span> <b>[B1]</b> <span size='small'>Rotate Image</span> <b>[B3]</b> <span size='small'>Initial Rotation</span>";

    strncpy(z,m,256);
    break;
  case 1:
    m="<span foreground='#800000'>3D Pan</span> <b>[B1]</b> <span size='small'>Move Image</span> <b>[B3]</b> <span size='small'>Center Image</span>";

    strncpy(z,m,256);
    break;
  case 2:

    m="<span foreground='#800000'>3D Zoom</span> <b>[B1]</b> <span size='small'>Zoom</span> <b>[B3]</b> <span size='small'>Zoom 100%</span>";
    strncpy(z,m,256);
    break;

  case 3:
    m="<span foreground='#800000'>3D Geodesic Ruler</span> <b>[B1]</b> <span size='small'>Append Point</span> <b>[B3]</b> <span size='small'>Start New Path</span>";
    strncpy(z,m,256);
    break;

  }

  gui.cstate = -1; // update cursor  
  gtk_label_set_markup(GTK_LABEL(gui.f3d),z);
  force_redraw_3D();
}

void app_t2d_changed(void *t) {
  char z[512],*m=NULL;

  app_tdebug(2010);
  z[0] = 0;

  switch(gui.t2d->selected) {
  case 0: m="<span foreground='#800000'>2D Cursor</span> <b>[B1]</b> <span size='small'>Position Cursor</span> <b>[B3]</b> <span size='small'>Change Brightness/Contrast</span>"; 

    if (roi.active) {

      m = "<span foreground='#800000'>2D Cursor/Clip</span> <b>[B1]</b> <span size='small'>Position ROI</span> <b>[Ctrl+B1]</b><span size='small'>Position Cursor</span> <b>[B3]</b> <span size='small'>Change Brightness/Contrast</span>";

    }
    
    break;

  case 1: m="<span foreground='#800000'>2D Pan</span> <b>[B1]</b> <span size='small'>Move Image</span> <b>[B3]</b> <span size='small'>Center Image</span>"; break;

  case 2: m="<span foreground='#800000'>2D Zoom</span> <b>[B1]</b> <span size='small'>Zoom</span> <b>[B3]</b> <span size='small'>100% Zoom</span>"; break;

  case 3: m="<span foreground='#800000'>2D Brush</span> <b>[B1]</b> <span size='small'>Paint Disk</span> <b>[B3]</b> <span size='small'>Paint Sphere</span> <b>[Ctrl+B3]</b> <span size='small'>Skin Surface Sphere</span>"; break;

  case 4: m="<span foreground='#800000'>2D Eraser</span> <b>[B1]</b> <span size='small'>Erase Disk</span> <b>[B3]</b> <span size='small'>Erase Sphere</span> <b>[Ctrl+B3]</b> <span size='small'>Erase Slice</span>"; break;

  case 5: m="<span foreground='#800000'>2D Contour</span> <b>[B1]</b> <span size='small'>Trace Contour</span> <b>[B2]</b> <span size='small'>Undo Last Contour</span>"; break;

  case 6: m="<span foreground='#800000'>2D Point Contour</span> <b>[B1]</b> <span size='small'>Add Point</span> <b>[B2]</b> <span size='small'>Erase Points</span> <b>[B3]</b> <span size='small'>Add Last Point</span>"; break;
  case 7: m="<span foreground='#800000'>2D Ruler</span> <b>[B1]</b> <span size='small'>Mark Points</span> <b>[Ctrl+B1]</b> <span size='small'>Remove Measure</span> <b>[B3]</b> <span size='small'>Move Points</span>"; break;

  case 8: m="<span foreground='#800000'>2D Angles</span> <b>[B1]</b> <span size='small'>Mark Points</span> <b>[Ctrl+B1]</b> <span size='small'>Remove Measure</span> <b>[B3]</b> <span size='small'>Move Points</span>"; break;

  }

  strncpy(z,m,256);
  gtk_label_set_markup(GTK_LABEL(gui.f2d),z);
  gui.cstate = -1; // update cursor
  app_set_status("",10);
}

void app_vis_changed(void *t) {
  int i;

  for(i=0;i<MAXLABELS;i++)
    if (t == (void *)(gui.vis_mark[i]))
      labels.visible[i] = CheckBoxGetState(gui.vis_mark[i]);

  v3d.obj.invalid = 1;
  force_redraw_3D();
}

void app_update_oct_panel() {
  static int fp = 0;
  app_tdebug(2011);
  if (v3d.octant.enable) {
    gtk_widget_show(gui.octpanel);
    if (!fp) {
      gdk_window_set_cursor(gui.octcw->window, gui.cursor2[0]);
      fp = 1;
    }
  } else
    gtk_widget_hide(gui.octpanel);
}

void app_compose3d_changed(void *t) {


  /* opacity changes don't need hard invalidation */
  if (t == (void *)(gui.oskin))
    v3d.skin.opacity = SliderGetValue(gui.oskin);
  if (t == (void *)(gui.oobj))
    v3d.obj.opacity = SliderGetValue(gui.oobj);

  /* enabling/disabling do */

  if (t == (void *)(gui.skin)) {
    v3d.skin.enable = CheckBoxGetState(gui.skin);
    if (v3d.octant.enable) v3d.octant.invalid = 1;
  }
  if (t == (void *)(gui.oct)) {
    v3d.octant.enable = CheckBoxGetState(gui.oct);
    v3d.octant.invalid = 1;
    app_update_oct_panel();
  }
  if (t == (void *)(gui.obj)) {
    v3d.obj.enable = CheckBoxGetState(gui.obj);
    v3d.obj.invalid = 1;
  }
  if (t == (void *)(gui.meas)) {
    v3d.show_measures = CheckBoxGetState(gui.meas);
  }

  force_redraw_3D();
}

void force_redraw_2D() {
  // printf("force2d\n");
  v2d.invalid = 1;
  refresh(cw);
}

void force_redraw_3D() {
  //  printf("force3d\n");
  v3d.invalid = 1;
  refresh(cw);
}

gint oct_toid = -1, obj_toid = -1;

gboolean octant_redraw(gpointer data) {

  oct_toid = -1;

  if (v3d.octant.enable) {
    v3d.octant.invalid = 1;
    force_redraw_3D();
  }
    
  return FALSE;
}

void app_queue_octant_redraw() {
  app_tdebug(2012);
  if (oct_toid >= 0) {
    gtk_timeout_remove(oct_toid);
    oct_toid = -1;
  }

  oct_toid = gtk_timeout_add(150, octant_redraw, 0);
}

gboolean object_redraw(gpointer data) {

  obj_toid = -1;

  if (v3d.obj.enable) {
    v3d.obj.invalid = 1;
    force_redraw_3D();
  }
    
  return FALSE;
}

void app_queue_obj_redraw() {
  app_tdebug(2013);
  if (obj_toid >= 0) {
    gtk_timeout_remove(obj_toid);
    obj_toid = -1;
  }

  obj_toid = gtk_timeout_add(350, object_redraw, 0);
}

void      app_menu_undo(CBARG) {
  undo_pop();
}

void      app_menu_kill_measures(CBARG) {
  measure_kill_all();
  app_set_status("Measurements Removed",20);
  force_redraw_2D();
  force_redraw_3D();
}

void app_menu_seg_erase(CBARG) {

  if (!voldata.label) {
    warn_no_volume();
    return;
  }

  if (PopYesNoBox(mw,"Confirmation","All segmentation will be lost. Proceed ?",MSG_ICON_ASK)==MSG_YES) {
    TaskNew(res.bgqueue, "Removing Segmentation", (Callback) &bg_erase_segmentation, 0);
  }

}

void app_menu_seg_tp2007(CBARG) {

  if (voldata.original == NULL) {
    warn_no_volume();
    return;
  }

  TaskNew(res.bgqueue, "Automatic Brain Segmentation by Tree Pruning", (Callback) &bg_seg_tp2007, 0);
}

void app_menu_seg_thres(CBARG) {
  if (voldata.original == NULL) {
    warn_no_volume();
    return;
  }

  thr_dialog();
}

void app_menu_seg_auto(CBARG) {
  if (voldata.original == NULL) {
    warn_no_volume();
    return;
  }

  seg_dialog();
}

void app_label_load(CBARG) {
  char name[512];
  if (util_get_filename(mw, "Load Object Names and Colors", name, GF_ENSURE_READABLE)) {
    label_load(name);
    view3d_invalidate(1,0);
    force_redraw_2D();
    force_redraw_3D();
  }
}

void app_label_save(CBARG) {
  char name[512];
  
  if (util_get_filename(mw, "Save Object Names and Colors", name, GF_ENSURE_WRITEABLE)) {
    label_save(name);
  }
}

void app_measure_load(CBARG) {
  char name[512];
  
  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (util_get_filename(mw, "Load Measurements", name, GF_ENSURE_READABLE)) {
    measure_load(name);
    force_redraw_2D();
    force_redraw_3D();
  }

}

void app_measure_save(CBARG) {
  char name[512];
  
  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (util_get_filename(mw, "Save Measurements", name, GF_ENSURE_WRITEABLE)) {
    measure_save(name);
  }
}


void app_menu_seg_load(CBARG) {
  char name[512];
  
  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (util_get_filename(mw, "Load Segmentation", name, GF_ENSURE_READABLE))
    {
      TaskNew(res.bgqueue, "Loading Segmentation", (Callback) &bg_load_segmentation, (void *) (strdup(name)));
    }
}

void app_menu_seg_save(CBARG) {
  char name[512];

  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (util_get_filename(mw, "Save Segmentation", name, GF_ENSURE_WRITEABLE))
    {
      TaskNew(res.bgqueue, "Saving Segmentation", (Callback) &bg_save_segmentation, (void *) (strdup(name)));
    }

}

void app_menu_view_options3d(CBARG) {
  view3d_dialog_options();
}

void app_menu_view_export(CBARG) {
  xport_view_dialog();
}

void app_menu_view_export2(CBARG) {
  xport_bulk_dialog();
}

void MyXAHomog(XAnnVolume *vol,int zslice) {
  int setlen, minilen;
  int i,j,k,t,u,half,W,H, WW, HH;
  i16_t *varx, *vary, *lookup;
  int *tmpa, *tmpb;
  float a,b,tmp;

  int maxamp,imax;
  i16_t smin,smax,v;

  if (!vol) return;
  W = vol->W;
  H = vol->H;
  setlen = W * H;

  WW = 1 + W/4;
  HH = 1 + H/4;
  minilen = WW * HH;

  varx = (i16_t *) MemAlloc(sizeof(i16_t) * setlen);
  if (!varx) return;
  vary = (i16_t *) MemAlloc(sizeof(i16_t) * setlen);
  if (!vary) {
    MemFree(varx);
    return;
  }
  lookup = (i16_t *) MemAlloc(sizeof(i16_t) * 32768);
  if (!lookup) {
    MemFree(vary);
    MemFree(varx);
    return;
  }
  tmpa = (int *) MemAlloc(sizeof(int) * minilen);
  tmpb = (int *) MemAlloc(sizeof(int) * minilen);
  if (!tmpa || !tmpb) {
    MemFree(lookup);
    MemFree(vary);
    MemFree(varx);
    return;
  }
   

  // pick a half slice with largest amplitude

  if (zslice < 0) {
    maxamp = 0;
    imax = 0;
    
    for(i=0;i<vol->D;i++) {
      
      smin = smax = vol->vd[i*vol->WxH].value;
      
      for(j=0;j<vol->WxH;j++) {
	v = vol->vd[j + vol->WxH * i].value;
	if (v < smin) smin = v;
	if (v > smax) smax = v;
      }
      k = (int) (smax - smin);
      if (k > maxamp) {
	maxamp = k;
	imax = i;
      }
    }

    half = imax;
  } else {
    half = zslice;
  }

  for(i=half;i>0;i--) {
    t = 0;

    // scale down both slices
    MemSet(tmpa, 0, minilen*sizeof(int));
    MemSet(tmpb, 0, minilen*sizeof(int));

    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	u = vol->tbrow[j] + k;
	tmpa[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i] + u].value;
	tmpb[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i-1] + u].value;
      }
    
    for(j=0;j<minilen;j++) {
      tmpa[j] /= 16;
      tmpb[j] /= 16;
    }

    // collect data points for correlation
    for(j=0;j<HH;j++)
      for(k=0;k<WW;k++) {
	u = j*WW + k;
	varx[t] = (i16_t) tmpa[u];
	vary[t] = (i16_t) tmpb[u];
	t++;
      }

    // find linear regression
    least_squares(vary,varx,t,&a,&b);

    a *= 1.01;

    //    printf("z=%d LS1 a=%.4f b=%.4f n=%d\n",i-1,a,b,t);

    // calculate lookup table
    for(j=0;j<32768;j++) {
      tmp = (((float) j) * a + b);
      if (tmp < 0.0) tmp = 0.0;
      if (tmp > 32767.0) tmp = 32767.0;
      lookup[j] = (i16_t) tmp;
    }

    // apply transform to slice i-1
    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	t = vol->tbframe[i-1] + vol->tbrow[j] + k;
	vol->vd[t].value = lookup[ vol->vd[t].value ];
      }

  }

  for(i=half;i<vol->D-1;i++) {
    t = 0;

    // scale down both slices
    MemSet(tmpa, 0, minilen*sizeof(int));
    MemSet(tmpb, 0, minilen*sizeof(int));

    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	u = vol->tbrow[j] + k;
	tmpa[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i] + u].value;
	tmpb[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i+1] + u].value;
      }
    
    for(j=0;j<minilen;j++) {
      tmpa[j] /= 16;
      tmpb[j] /= 16;
    }

    // collect data points for correlation
    for(j=0;j<HH;j++)
      for(k=0;k<WW;k++) {
	u = j*WW + k;
	varx[t] = (i16_t) tmpa[u];
	vary[t] = (i16_t) tmpb[u];
	t++;
      }

    // find linear regression
    least_squares(vary,varx,t,&a,&b);
    a *= 1.01;

    //    printf("z=%d LS1 a=%.4f b=%.4f n=%d\n",i-1,a,b,t);

    // calculate lookup table
    for(j=0;j<32768;j++) {
      tmp = (((float) j) * a + b);
      if (tmp < 0.0) tmp = 0.0;
      if (tmp > 32767.0) tmp = 32767.0;
      lookup[j] = (i16_t) tmp;
    }

    // apply transform to slice i+1
    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	t = vol->tbframe[i+1] + vol->tbrow[j] + k;
	vol->vd[t].value = lookup[ vol->vd[t].value ];
      }

  }

  MemFree(tmpa);
  MemFree(tmpb);
  MemFree(lookup);
  MemFree(varx);
  MemFree(vary);
}

void app_menu_tool_histogram(CBARG) {
  histogram_dialog();
}

void app_menu_tool_info(CBARG) {
  info_dialog();
}

void app_menu_tool_volinfo(CBARG) {
  volinfo_dialog();
}

void app_menu_tool_volumetry(CBARG) {
  volumetry_dialog();
}

void app_menu_tool_roi(CBARG) {
  roi_tool();
}

void app_menu_tool_orient(CBARG) {
  orient_dialog();
}

void app_menu_export_scn(CBARG) {
  xport_volume(0);
}

void app_menu_export_analyze(CBARG) {
  xport_volume(1);
}

typedef struct _popitem {
  char *title;
  char *text;
  int   icon;
  struct _popitem *prev;
  struct _popitem *next;
} PopItem;

PopItem *NewPopItem(char *title, char *text, int icon) {
  PopItem *x;
  x = (PopItem *) MemAlloc(sizeof(PopItem));
  if (!x) return 0;
  x->title = strdup(title);
  x->text  = strdup(text);
  x->icon  = icon;
  x->prev  = 0;
  x->next  = 0;
  return x;
}

void DestroyPopItem(PopItem *x) {
  if (x) {
    if (x->title) MemFree(x->title);
    if (x->text) MemFree(x->text);
    MemFree(x);
  }
}

struct popqueue {
  int nitems;
  PopItem *head,*tail;  
  Mutex   *lock;
} PopQ;

void   app_pop_init() {
  PopQ.nitems = 0;
  PopQ.head = PopQ.tail = 0;
  PopQ.lock = MutexNew();
}

void   app_pop_message(char *title, char *text, int icon) {
  PopItem *x;

  x = NewPopItem(title,text,icon);
  if (!x) return;
  MutexLock(PopQ.lock);
  if (PopQ.nitems == 0) {
    PopQ.head = PopQ.tail = x;
  } else {
    x->prev = PopQ.tail;
    PopQ.tail->next = x;
    PopQ.tail = x;    
  }
  PopQ.nitems++;
  MutexUnlock(PopQ.lock);
}

void app_pop_consumer() {
  PopItem *x;
  MutexLock(PopQ.lock);
  if (PopQ.nitems == 0) {
    MutexUnlock(PopQ.lock);
    return;
  }

  x = PopQ.head;

  if (x->next) {
    x->next->prev = 0;
    PopQ.head = x->next;
  } else {
    PopQ.head = PopQ.tail = 0;
  }

  PopQ.nitems--;
  MutexUnlock(PopQ.lock);

  PopMessageBox(mw,x->title,x->text,x->icon);
  DestroyPopItem(x);
}

gboolean app_kf_expose(GtkWidget *w, GdkEventExpose *ee, gpointer data) {
  int W,H,i;
  static CImg *buf = 0;

  W = w->allocation.width;
  H = w->allocation.height;

  if (buf!=0) {
    if (buf->W != W || buf->H != H) {
      CImgDestroy(buf);
      buf = 0;
    }
  }
  if (!buf) {
    buf = CImgNew(W,H);
  }

  for(i=0;i<4;i++) {
    CImgDrawBevelBox(buf, (i%2)*(W/2), (i/2)*(H/2),
		     W/2, H/2, 
		     view.focus==i ? view.pfg : view.pbg,
		     0, view.focus!=i);
  }

  gdk_draw_rgb_image(w->window, w->style->black_gc, 
		     0,0,W,H,GDK_RGB_DITHER_NORMAL,
		     buf->val, buf->rowlen);

  return FALSE;
}

gboolean app_kf_press(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  int i,x,y,W,H;

  if (eb->button == 1) {
    W = w->allocation.width;
    H = w->allocation.height;
    W /= 2;
    H /= 2;

    x = (int) (eb->x);
    y = (int) (eb->y);
    i = (x / W) + (2*(y/H));
    if (i<0) i=0;
    if (i>3) i=3;
    view.focus = i;
    refresh(gui.kf);
  }
  return FALSE;
}

void app_action_key(int i) {

  if (view.maximized < 0) {

    if (view.focus == 3) return;

    if (i==0)
      v2d_left(view.focus);
    else
      v2d_right(view.focus);
    refresh(cw);

  } else {
    if (view.maximized==3) return;
    if (i==0)
      v2d_left(view.maximized);
    else
      v2d_right(view.maximized);
    refresh(cw);
  }

}

gboolean  app_key_down(GtkWidget *w,GdkEventKey *ek,gpointer data) {
  
  if (ek->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK))
    return FALSE;

  switch(ek->keyval) {
    /* 2D tool shortcuts */
  case GDK_Q:
  case GDK_q:
    ToolbarSetSelection(gui.t2d,0);
    app_t2d_changed(0);
    return TRUE;
  case GDK_W:
  case GDK_w:
    ToolbarSetSelection(gui.t2d,1);
    app_t2d_changed(0);
    return TRUE;
  case GDK_E:
  case GDK_e:
    ToolbarSetSelection(gui.t2d,2);
    app_t2d_changed(0);
    return TRUE;
  case GDK_A:
  case GDK_a:
    ToolbarSetSelection(gui.t2d,3);
    app_t2d_changed(0);
    return TRUE;
  case GDK_S:
  case GDK_s:
    ToolbarSetSelection(gui.t2d,4);
    app_t2d_changed(0);
    return TRUE;
  case GDK_D:
  case GDK_d:
    ToolbarSetSelection(gui.t2d,5);
    app_t2d_changed(0);
    return TRUE;
  case GDK_Z:
  case GDK_z:
    ToolbarSetSelection(gui.t2d,6);
    app_t2d_changed(0);
    return TRUE;
  case GDK_X:
  case GDK_x:
    ToolbarSetSelection(gui.t2d,7);
    app_t2d_changed(0);
    return TRUE;
  case GDK_C:
  case GDK_c:
    ToolbarSetSelection(gui.t2d,8);
    app_t2d_changed(0);
    return TRUE;
    /* 3D tools */
  case GDK_R:
  case GDK_r:
    ToolbarSetSelection(gui.t3d,0);
    app_t3d_changed(0);
    return TRUE;
  case GDK_T:
  case GDK_t:
    ToolbarSetSelection(gui.t3d,1);
    app_t3d_changed(0);
    return TRUE;
  case GDK_Y:
  case GDK_y:
    ToolbarSetSelection(gui.t3d,2);
    app_t3d_changed(0);
    return TRUE;
  case GDK_F:
  case GDK_f:
    ToolbarSetSelection(gui.t3d,3);
    app_t3d_changed(0);
    return TRUE;
    /* brush keys */
  case GDK_F1:
    ToolbarSetSelection(gui.tpen,0);
    return TRUE;
  case GDK_F2:
    ToolbarSetSelection(gui.tpen,1);
    return TRUE;
  case GDK_F3:
    ToolbarSetSelection(gui.tpen,2);
    return TRUE;
  case GDK_F4:
    ToolbarSetSelection(gui.tpen,3);
    return TRUE;
  case GDK_F5:
    ToolbarSetSelection(gui.tpen,4);
    return TRUE;
  case GDK_F6:
    ToolbarSetSelection(gui.tpen,5);
    return TRUE;
  case GDK_F7:
    ToolbarSetSelection(gui.tpen,6);
    return TRUE;
  case GDK_F8:
    ToolbarSetSelection(gui.tpen,7);
    return TRUE;

  case GDK_Left:
    app_action_key(0);
    return TRUE;
  case GDK_Right:
    app_action_key(1);
     return TRUE;
  }
  return FALSE;
}

void zo_start(int x,int y,int pane) {
  float zoom, zolen;

  x -= view.bx[pane];
  y -= view.by[pane];
  view.zopane = pane;

  zoom = (pane == 3) ? v3d.zoom : v2d.zoom;

  view.zolen = (2*view.bw[pane])/3;
  view.zoy   = y;
  zolen = (float) view.zolen;
  view.zox   = x - (zolen * ((zoom - 0.5)/7.5));

  view.zobuf = CImgNew(view.zolen,8);
  
  if (pane == 3) force_redraw_3D(); else force_redraw_2D();
}

void zo_drag(int x,int y) {
  float zoom, zolen;

  x -= view.bx[view.zopane];
  zolen = (float) view.zolen;

  zoom = 0.5 + (7.5*(x-view.zox)) / zolen;
  if (zoom < 0.50) zoom=0.50;
  if (zoom > 8.00) zoom=8.00;

  if (view.zopane==3) {
    v3d.zoom = zoom; 
    view3d_invalidate(0,1);
    force_redraw_3D();
  } else {
    v2d.zoom = zoom;
    force_redraw_2D();
  }
}

void zo_end(int x,int y) {
  int i;
  zo_drag(x,y);
  view.zolen = 0;
  view.zox = view.zoy = 0;
  i = view.zopane;
  view.zopane = -1;
  if (i==3) {
    force_redraw_3D();
    view3d_invalidate(0,1);
  } else
    force_redraw_2D();
  CImgDestroy(view.zobuf);
}

void bc_start(int x,int y,int pane) {
  float b, c;

  x -= view.bx[pane];
  y -= view.by[pane];
  view.bcpane = pane;

  c = gui.con->value; // x
  b = gui.bri->value; // y

  view.bcpx = c;
  view.bcpy = b;

  view.bcw = view.bw[pane];
  view.bch = view.bh[pane];

  c = ((float)x) / ((float)(view.bcw));
  b = ((float)(view.bch-y)) / ((float)(view.bch));
  gui.con->value = c;
  gui.bri->value = b;
  refresh(gui.con->widget);
  refresh(gui.bri->widget);
  force_redraw_2D();
}

void bc_drag(int x,int y) {
  float b, c;

  x -= view.bx[view.bcpane];
  y -= view.by[view.bcpane];

  c = ((float)x) / ((float)(view.bcw));
  b = ((float)(view.bch-y)) / ((float)(view.bch));
  gui.con->value = c;
  gui.bri->value = b;
  refresh(gui.con->widget);
  refresh(gui.bri->widget);
  force_redraw_2D();
}

void bc_end(int x,int y) {
  bc_drag(x,y);
  view.bcpane = -1;
  force_redraw_2D();
}

gboolean  palette_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  static CImg *buf = 0;
  int i,j,W,H;
  float fa;
  char *text;

  W = w->allocation.width;
  H = w->allocation.height;
  
  if (buf!=0)
    if (buf->W!=W || buf->H!=H) {
      CImgDestroy(buf);
      buf = 0;
    }

  if (buf==0)
    buf=CImgNew(W,H);

  for(i=0;i<W;i++) {
    fa = ((float)i) / ((float)W);
    fa *= 255.0;
    j = (int) fa;
    CImgDrawLine(buf,i,0,i,H-1,v2d.pal[j]);
  }
  CImgDrawRect(buf,0,0,W,H,0);

  j = strlen(v2d.palname[v2d.cpal]);
  text = v2d.palname[v2d.cpal];
  CImgDrawShieldedSText(buf,res.font[3],
			(W-SFontLen(res.font[3],text))/2,
			(H-SFontH(res.font[3],text))/2,0xffffff,0,text);
  
  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,W,H,
		     GDK_RGB_DITHER_NORMAL,
		     buf->val,
		     buf->rowlen);
  return FALSE;
}

void pal_up(void *args) {
  v2d.cpal = (v2d.cpal + 1) % (v2d.npal);
  v2d.pal  = v2d.paldata[v2d.cpal];
  force_redraw_2D();
  view3d_octant_trash();
  force_redraw_3D();
  refresh(gui.palcanvas);
}

void pal_down(void *args) {
  v2d.cpal = (v2d.cpal + v2d.npal - 1) % (v2d.npal);
  v2d.pal  = v2d.paldata[v2d.cpal];
  force_redraw_2D();
  view3d_octant_trash();
  force_redraw_3D();
  refresh(gui.palcanvas);
}

void update_controls() {
  // opacity and composition
  SliderSetValue(gui.oskin,v3d.skin.opacity);
  SliderSetValue(gui.oobj,v3d.obj.opacity);

  CheckBoxSetState(gui.skin,v3d.skin.enable);
  CheckBoxSetState(gui.oct,v3d.octant.enable);
  CheckBoxSetState(gui.obj,v3d.obj.enable);

  CheckBoxSetState(gui.overlay_info,v2d.overlay[0]);
  CheckBoxSetState(gui.overlay_orient,v2d.overlay[1]);

  // label visibility and names, both sides
  labels_changed();

  // palette
  refresh(gui.palcanvas);
  refresh(gui.octpanel);

  refresh(gui.t2d->widget);
  refresh(gui.tpen->widget);
  refresh(gui.t3d->widget);
  app_t2d_changed(0);
  app_t3d_changed(0);
  app_update_oct_panel();

}

void overlay_changed(void *args) {
  v2d.overlay[0] = CheckBoxGetState(gui.overlay_info);
  v2d.overlay[1] = CheckBoxGetState(gui.overlay_orient);
  force_redraw_2D();
}

void app_tdebug(int id) {
  if (tdebug) {
    if (pthread_self() != good_thread) {
      fprintf(stderr,"BV thread debug: id=%d gt=%d bt=%d\n",
	      id,(int)(good_thread),(int)(pthread_self()));
    }
  }
}



