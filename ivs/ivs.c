/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2010
   
   Felipe Paulo Guazzi Bergo 
   <fbergo@gmail.com>

   and
   
   Alexandre Xavier Falcao
   <afalcao@ic.unicamp.br>

   Distribution, trade, publication or bulk reproduction
   of this source code or any derivative works are
   strictly forbidden. Source code is provided to allow
   compilation on different architectures.

   ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libip.h>
#include "ivs.h"
#include "preproc.h"
#include "animate.h"
#include "util.h"
#include "be.h"
#include "filemgr.h"
#include "volumetry.h"
#include "tasklog.h"
#include "fuzzy.h"
#include "ct.h"
#include "onion.h"

#include "xpm/g12.xpm"
#include "xpm/r12.xpm"
#include "xpm/ldots.xpm"

#include "xpm/f5.xpm"
#include "xpm/f6.xpm"
#include "xpm/f7.xpm"
#include "xpm/f8.xpm"

#include "xpm/ivs.xpm"
#include "xpm/banner1.xpm"

#include "xpm/toolrotate.xpm"
#include "xpm/toolclip.xpm"
#include "xpm/toollight.xpm"
#include "xpm/toolcolors.xpm"
#include "xpm/tooladdseeds.xpm"
#include "xpm/tooldeltrees.xpm"
#include "xpm/tooloverlay.xpm"
#include "xpm/tool2d.xpm"

/* globals */
Scene     *scene;
GtkWidget *mainwindow, *notebook;
GdkCursor *crosshair, *arrow, *watch;
GtkTooltips *tips;

TaskQueue *qseg;
TaskLog   *tasklog;

char      filename[512];

/* sync_xxxx are incremented by background threads to notify
   the GUI thread about required updates */

int       sync_redraw   = 0;  /* forces redraw */
int       sync_newdata  = 0;  /* forces scene->pane update and redraw */
int       sync_ppchange = 0;  /* updates preprocessing pane,scene if needed */
int       sync_objmode  = 0;  /* get out of solid box mode, force redraw */

// "Scene" tab
struct SegPane   pane;
struct SegCanvas canvas;

// "Preprocessing" tab
extern struct PreProc preproc;

// "File Browser" tab
extern struct FileMgr mgr;

struct Status {
  GtkWidget *itself;
  GtkWidget *hself;
  GtkWidget *text;
  int        textout;
  GtkWidget *task[5];
  GtkWidget *activetask;
  int        ln;
} status;

struct PopupQueue {
  char title[4][32];
  char text[4][512];
  int  icon[4];
  int  n;
  pthread_mutex_t ex;
} pq;

struct StatusQueue {
  char text[4][256];
  int  timeout[4];
  int  n;
  pthread_mutex_t ex;
} sq;

fob FuzzyPars = { 2, 
		  {0, 800, [2 ... 9] = 10 },
		  {400, 400, [2 ... 9] = 10 } };

int FirstFuzzyDone = 0;		  
int fuzzyvariant = 0; // 0=normal, 1=strict

/* menu callbacks */

#define CBARG GtkWidget *w, gpointer data

void ivs_file_reset_annotation(CBARG);
void ivs_file_load_scene(CBARG);
void ivs_file_load_labels(CBARG);
void ivs_file_save_scene(CBARG);
void ivs_file_export_data(CBARG);
void ivs_file_export_masked_data(CBARG);
void ivs_file_export_view(CBARG);
void ivs_file_save_log(CBARG);
void ivs_file_info(CBARG);
void ivs_file_quit(CBARG);
void ivs_seed_save(CBARG);
void ivs_seed_load(CBARG);
void ivs_volumetry(CBARG);
void ivs_histogram(CBARG);
void ivs_cthres(CBARG);
void ivs_fpars(CBARG);
void ivs_onion(CBARG);
void ivs_mipload(CBARG);
void ivs_vspec_save(CBARG);
void ivs_vspec_load(CBARG);
void ivs_anim_sweep_full(CBARG);
void ivs_anim_sweep_quick(CBARG);
void ivs_anim_roty_full(CBARG);
void ivs_anim_roty_quick(CBARG);
void ivs_anim_wave_full(CBARG);
void ivs_anim_wave_quick(CBARG);
void ivs_anim_grow_full(CBARG);
void ivs_anim_grow_quick(CBARG);
void ivs_help_about(CBARG);
void ivs_keyhelp(CBARG);
void ivs_formathelp(CBARG);
void ivs_vsize(CBARG);

static GtkItemFactoryEntry ivs_menu[] = {
  { "/_File",                    NULL, NULL, 0, "<Branch>" },
  { "/File/_Reset Scene Annotation",NULL,GTK_SIGNAL_FUNC(ivs_file_reset_annotation),
    0,NULL},
  { "/File/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/File/_Load Scene or Volume...",NULL,GTK_SIGNAL_FUNC(ivs_file_load_scene),0,NULL},
  { "/File/Load La_bels...",NULL,GTK_SIGNAL_FUNC(ivs_file_load_labels),0,NULL},
  { "/File/_Save Scene...",NULL,GTK_SIGNAL_FUNC(ivs_file_save_scene),0,NULL},
  { "/File/Scene _Information",NULL,GTK_SIGNAL_FUNC(ivs_file_info),0,NULL},
  { "/File/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/File/Export Scene _Data...",NULL,GTK_SIGNAL_FUNC(ivs_file_export_data),0,NULL},
  { "/File/Export _Masked Scene Data...",NULL,GTK_SIGNAL_FUNC(ivs_file_export_masked_data),0,NULL},
  { "/File/_Export Scene View...",NULL,GTK_SIGNAL_FUNC(ivs_file_export_view),0,NULL},
  { "/File/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/File/Save _Task Log...",NULL,GTK_SIGNAL_FUNC(ivs_file_save_log),0,NULL},
  { "/File/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/File/_Quit",NULL,GTK_SIGNAL_FUNC(ivs_file_quit),0,NULL},
  { "/_Seeds",                    NULL, NULL, 0, "<Branch>" },
  { "/Seeds/Sa_ve Roots...",      NULL, GTK_SIGNAL_FUNC(ivs_seed_save),0,NULL },
  { "/Seeds/Lo_ad Seeds...",      NULL, GTK_SIGNAL_FUNC(ivs_seed_load),0,NULL },
  { "/_Tools", NULL,NULL,0,"<Branch>" },
  { "/Tools/_Volumetry...",NULL,GTK_SIGNAL_FUNC(ivs_volumetry),0,NULL },
  { "/Tools/_Histogram...",NULL,GTK_SIGNAL_FUNC(ivs_histogram),0,NULL },
  { "/Tools/_Cost Threshold...",NULL,GTK_SIGNAL_FUNC(ivs_cthres),0,NULL },
  { "/Tools/_Onion Peeling...",NULL,GTK_SIGNAL_FUNC(ivs_onion),0,NULL },
  { "/Tools/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Tools/_Fuzzy Parameters Dialog",NULL,GTK_SIGNAL_FUNC(ivs_fpars),0,NULL },
  { "/Tools/_Edit Voxel Size",NULL,GTK_SIGNAL_FUNC(ivs_vsize),0,NULL },
  { "/Tools/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Tools/Ove_rlay: Load New Volume...", NULL, GTK_SIGNAL_FUNC(ivs_mipload),0,NULL }, 
  { "/Tools/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Tools/_Save View Specification...",NULL,GTK_SIGNAL_FUNC(ivs_vspec_save),0,NULL },
  { "/Tools/_Load View Specification...",NULL,GTK_SIGNAL_FUNC(ivs_vspec_load),0,NULL },
  { "/_Animate",  NULL, NULL, 0, "<Branch>" },
  { "/Animate/Full XYZ Plane _Sweep",NULL, GTK_SIGNAL_FUNC(ivs_anim_sweep_full),0,NULL },
  { "/Animate/Quick XYZ Plane S_weep",NULL, GTK_SIGNAL_FUNC(ivs_anim_sweep_quick),0,NULL },
  { "/Animate/sep", NULL, NULL, 0, "<Separator>" },
  { "/Animate/Full _Y Rotation",NULL, GTK_SIGNAL_FUNC(ivs_anim_roty_full),0,NULL },
  { "/Animate/_Quick Y Rotation",NULL, GTK_SIGNAL_FUNC(ivs_anim_roty_quick),0,NULL },
  { "/Animate/sep", NULL, NULL, 0, "<Separator>" },
  { "/Animate/_Cost-based Full Wavefront",NULL, GTK_SIGNAL_FUNC(ivs_anim_wave_full),0,NULL },
  { "/Animate/Cost-based Quick _Wavefront",NULL, GTK_SIGNAL_FUNC(ivs_anim_wave_quick),0,NULL },
  { "/Animate/Cost-based _Full _Growth",NULL, GTK_SIGNAL_FUNC(ivs_anim_grow_full),0,NULL },
  { "/Animate/Cost-based Q_uick Growth",NULL, GTK_SIGNAL_FUNC(ivs_anim_grow_quick),0,NULL },
  { "/_Help", NULL, NULL,0,"<Branch>" },
  { "/Help/Keyboard and Mouse _Input...",NULL,GTK_SIGNAL_FUNC(ivs_keyhelp),0,NULL},
  { "/Help/_File Formats...",NULL,GTK_SIGNAL_FUNC(ivs_formathelp),0,NULL},
  { "/Help/sep", NULL,NULL,0,"<Separator>" },
  { "/Help/_About IVS...",NULL,GTK_SIGNAL_FUNC(ivs_help_about),0,NULL},
};

/* local functions */
void ivs_create_gui();

gboolean  ivs_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void      ivs_destroy(GtkWidget *w, gpointer data);

gboolean  ivs_view_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  ivs_view_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data);
gboolean  ivs_view_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);
gboolean  ivs_view_release(GtkWidget *widget,GdkEventButton *eb,gpointer data);

gboolean  mipcurve_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  mipcurve_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data);
gboolean  mipcurve_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);
void      mipcurve_mouse(int x,int y,int w,int h);

void      ivs_view_press_2D(int x,int y, int b);
void      ivs_view_drag_2D(int x,int y,int b);

gboolean  ivs_rot_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  ivs_ball_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean  ivs_ball_press(GtkWidget *widget, GdkEventButton *eb, gpointer data);
void      ivs_rot_preset(GtkWidget *w, gpointer data);
void      ivs_light_preset(GtkWidget *w, gpointer data);
void      ivs_clip_preset(GtkWidget *w, gpointer data);
void      ivs_color_preset(GtkWidget *w, gpointer data);
void      ivs_clear_addseeds(GtkWidget *w, gpointer data);
void      ivs_unmark_trees(GtkWidget *w, gpointer data);
void      ivs_segment(GtkWidget *w, gpointer data);
void      ivs_tool(GtkToggleButton *w, gpointer data);
gboolean  ivs_timed_update(gpointer data);
void      ivs_update(void *obj);
void      ivs_vrange_change(void *obj);
void      ivs_tonalization(void *obj);
gboolean  ivs_scan(gpointer data);
void      ivs_make_cube();
void      ivs_draw_cube();
void      ivs_update_status();
gboolean  ivs_status_clear(gpointer data);
void      ivs_check_popup_queue();
void      ivs_check_status_queue();
gboolean  ivs_key (GtkWidget * w, GdkEventKey * evt, gpointer data);
void	  ivs_draw_zinfo();

void      ivs_draw_tvoxel();

void      compose_vista();
void      compose_vista_2D();
void      compose_vista_2D_overlay();
void      draw_vista_2D();

void      ivs_pane_to_scene(int flags);
void      ivs_scene_to_pane(int flags);

void      ivs_viewspec_save(char *name);
void      ivs_viewspec_load(char *name);

void      ivs_mode2d_update(void *obj);
void      ivs_mip_src(void *obj);

void ivs_create_gui() {
  GtkWidget *mw, *v, *nb, *nbl[4], *nbh[4], *nbp[4], *nbc[4], *rp, *rrp;
  GtkWidget *tmp[80];
  GtkStyle  *black;
  GtkAccelGroup *mag;
  GtkItemFactory *gif;
  char z[128];

  int i,j,k;
  int nitems = sizeof(ivs_menu)/sizeof(ivs_menu[0]);

  pane.zinfo.active = 0;

  black = gtk_style_new();
  pane.active_tool=gtk_style_new();
  pane.inactive_tool=gtk_style_new();

  set_style_bg(black, 0x000000);
  set_style_bg(pane.active_tool, 0xff8000);
  set_style_bg(pane.inactive_tool, 0xd0c8b0);

  mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  mainwindow = mw;
  gtk_widget_set_events(mw,GDK_KEY_PRESS_MASK);
  gtk_window_set_title(GTK_WINDOW(mw),"IVS");
  gtk_window_set_default_size(GTK_WINDOW(mw),800,600);
  gtk_widget_realize(mw);

  set_icon(mw, ivs_xpm, "IVS");

  crosshair = gdk_cursor_new(GDK_TCROSS);
  arrow     = gdk_cursor_new(GDK_LEFT_PTR);
  watch     = gdk_cursor_new(GDK_WATCH);

  /* main vbox */
  v = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(mw), v);

  /* menu bar */
  mag=gtk_accel_group_new();
  gif=gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);

  gtk_item_factory_create_items (gif, nitems, ivs_menu, NULL);   
  gtk_window_add_accel_group(GTK_WINDOW(mw), mag);

  tmp[3] = gtk_item_factory_get_widget (gif, "<main>");
  gtk_box_pack_start(GTK_BOX(v), tmp[3], FALSE, TRUE, 0);

  /* notebook */
  notebook = nb = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_BOTTOM);
  gtk_box_pack_start(GTK_BOX(v), nb, TRUE, TRUE, 0);

  /* status bar */
  status.itself = gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(status.itself), GTK_SHADOW_OUT);
  status.hself = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(v), status.itself, FALSE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(status.itself), status.hself);
  gtk_widget_show(status.itself);
  gtk_widget_show(status.hself);

  sprintf(z,"IVS %s [%s] - Interactive Volume Segmentation.",
	  VERSION, IVS_TAG);

  status.textout = -1;
  status.text = gtk_label_new(z);

  tmp[70] = gtk_hbox_new(FALSE,0);
  tmp[63] = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(status.hself), status.text, FALSE, FALSE, 2);
  gtk_box_pack_end(GTK_BOX(status.hself), tmp[70], FALSE, TRUE, 2);
  gtk_box_pack_end(GTK_BOX(status.hself), tmp[63], FALSE, TRUE, 2);

  gtk_widget_show(status.text);

  status.task[0] = ivs_pixmap_new(mw, ldots_xpm);
  status.task[1] = ivs_pixmap_new(mw, r12_xpm);
  status.task[2] = ivs_pixmap_new(mw, r12_xpm);
  status.task[3] = ivs_pixmap_new(mw, r12_xpm);
  status.task[4] = ivs_pixmap_new(mw, g12_xpm);
  status.activetask = gtk_label_new(" ");
  status.ln = 0;

  for(i=0;i<5;i++) {
    gtk_box_pack_start(GTK_BOX(tmp[70]), status.task[i], FALSE, TRUE, 0);
    gtk_widget_hide(status.task[i]);
  }
  gtk_box_pack_start(GTK_BOX(tmp[70]), status.activetask, FALSE, TRUE, 2);

  gtk_widget_show(status.activetask);

  /* tool tips */
  tips = GTK_TOOLTIPS(gtk_tooltips_new());

  /* file manager pane */

  nbc[2] = gtk_hbox_new(FALSE, 0);

  nbh[2] = gtk_hbox_new(FALSE,2);
  nbl[2] = gtk_label_new("File Browser");
  nbp[2] = ivs_pixmap_new(mainwindow, f5_xpm);
  gtk_box_pack_start(GTK_BOX(nbh[2]), nbp[2], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(nbh[2]), nbl[2], FALSE, TRUE, 2);
  gtk_widget_show(nbl[2]);
  gtk_widget_show(nbp[2]);
  gtk_widget_show(nbh[2]);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb), nbc[2], nbh[2]);

  create_filemgr_gui();
  gtk_box_pack_start(GTK_BOX(nbc[2]), mgr.itself, TRUE, TRUE, 0);

  /* pre-processing pane */
  nbc[1] = gtk_hbox_new(FALSE,0);

  nbh[1] = gtk_hbox_new(FALSE,2);
  nbl[1] = gtk_label_new("Preprocessing");
  nbp[1] = ivs_pixmap_new(mainwindow, f6_xpm);
  gtk_box_pack_start(GTK_BOX(nbh[1]), nbp[1], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(nbh[1]), nbl[1], FALSE, TRUE, 2);
  gtk_widget_show(nbl[1]);
  gtk_widget_show(nbp[1]);
  gtk_widget_show(nbh[1]);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb), nbc[1], nbh[1]);

  create_preproc_gui(nbc[1]);

  /* view pane */
  nbc[0] = gtk_hbox_new(FALSE,0);

  nbh[0] = gtk_hbox_new(FALSE,2);
  nbl[0] = gtk_label_new("Scene");
  nbp[0] = ivs_pixmap_new(mainwindow, f7_xpm);
  gtk_box_pack_start(GTK_BOX(nbh[0]), nbp[0], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(nbh[0]), nbl[0], FALSE, TRUE, 2);
  gtk_widget_show(nbl[0]);
  gtk_widget_show(nbp[0]);
  gtk_widget_show(nbh[0]);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb), nbc[0], nbh[0]);

  /* log pane */
  nbc[3] = gtk_hbox_new(FALSE,0);

  nbh[3] = gtk_hbox_new(FALSE,2);
  nbl[3] = gtk_label_new("Log");
  nbp[3] = ivs_pixmap_new(mainwindow, f8_xpm);
  gtk_box_pack_start(GTK_BOX(nbh[3]), nbp[3], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(nbh[3]), nbl[3], FALSE, TRUE, 2);
  gtk_widget_show(nbl[3]);
  gtk_widget_show(nbp[3]);
  gtk_widget_show(nbh[3]);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb), nbc[3], nbh[3]);
  
  tmp[79] = create_tasklog_pane(tasklog);
  gtk_box_pack_start(GTK_BOX(nbc[3]), tmp[79], TRUE, TRUE, 0);

  canvas.itself = gtk_drawing_area_new();
  canvas.pfd = NULL;
  canvas.pl  = NULL;
  gtk_widget_set_events(canvas.itself,GDK_EXPOSURE_MASK|
			GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|
                        GDK_BUTTON1_MOTION_MASK|
                        GDK_BUTTON2_MOTION_MASK|
			GDK_BUTTON3_MOTION_MASK|
			GDK_BUTTON_RELEASE_MASK|
                        GDK_POINTER_MOTION_MASK);
  tmp[4] = gtk_vbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(nbc[0]), tmp[4], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tmp[4]), canvas.itself, TRUE, TRUE, 0);

  rp = gtk_vbox_new(FALSE,0);
  gtk_widget_set_style(rp, black);
  gtk_widget_ensure_style(rp);
  
  pane.itself = rp;

  tmp[5] = gtk_frame_new("Tools");
  tmp[6] = gtk_table_new(2,4,TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[5]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[6]), 2);

  gtk_container_add(GTK_CONTAINER(tmp[5]),tmp[6]);

  /* view tools */

  pane.tools[0] = icon_button(mainwindow, toolrotate_xpm, 0);
  pane.tools[1] = icon_button(mainwindow, toolclip_xpm, 0);
  pane.tools[2] = icon_button(mainwindow, toollight_xpm, 0);
  pane.tools[3] = icon_button(mainwindow, toolcolors_xpm, 0);
  pane.tools[4] = icon_button(mainwindow, tooladdseeds_xpm, 0);
  pane.tools[5] = icon_button(mainwindow, tooldeltrees_xpm, 0);
  pane.tools[6] = icon_button(mainwindow, tool2d_xpm, 0);
  pane.tools[7] = icon_button(mainwindow, tooloverlay_xpm, 0);

  gtk_tooltips_set_tip(tips, pane.tools[0], "Rotate Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[1], "Clip Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[2], "Light Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[3], "Color Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[4], "Add Seeds Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[5], "Remove Trees Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[6], "2D View Options Tool",0);
  gtk_tooltips_set_tip(tips, pane.tools[7], "Overlay Tool",0);

  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[0], 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[1], 1,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[2], 2,3, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[3], 3,4, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[4], 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[5], 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[7], 2,3, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[6]), pane.tools[6], 3,4, 1,2);

  /* overlay options */

  pane.tcontrol[7] = gtk_frame_new("Overlay Options");
  tmp[74] = gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[7]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[74]), 2);  
  gtk_container_add(GTK_CONTAINER(pane.tcontrol[7]), tmp[74]);

  pane.overlay.mipenable=gtk_check_button_new_with_label("Enable MIP Overlay");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.overlay.mipenable),FALSE);
  gtk_box_pack_start(GTK_BOX(tmp[74]), pane.overlay.mipenable, FALSE,FALSE,2);
  gtk_widget_show(pane.overlay.mipenable);  

  pane.overlay.miplighten=gtk_check_button_new_with_label("Lighten Only");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.overlay.miplighten),FALSE);
  gtk_box_pack_start(GTK_BOX(tmp[74]), pane.overlay.miplighten, FALSE,FALSE,2);
  gtk_widget_show(pane.overlay.miplighten);  

  pane.overlay.mipcurve=gtk_drawing_area_new();
  gtk_widget_set_events(pane.overlay.mipcurve,GDK_EXPOSURE_MASK|
                        GDK_BUTTON_PRESS_MASK|GDK_BUTTON1_MOTION_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(pane.overlay.mipcurve), 150, 32);

  gtk_box_pack_start(GTK_BOX(tmp[74]), pane.overlay.mipcurve, FALSE,FALSE,2);
  gtk_widget_show(pane.overlay.mipcurve);  

  pane.overlay.mipsource = dropbox_new(";Original;Current;Cost;Overlay");
  tmp[75] = gtk_hbox_new(FALSE,2);
  tmp[76] = gtk_label_new("Overlay Data:");

  gtk_box_pack_start(GTK_BOX(tmp[75]), tmp[76], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tmp[75]), pane.overlay.mipsource->widget, 
		     FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tmp[74]), tmp[75], FALSE,FALSE,2);
  gtk_widget_show(pane.overlay.mipsource->widget);  

  gtk_signal_connect(GTK_OBJECT(pane.overlay.mipenable),"toggled",
                     GTK_SIGNAL_FUNC(ivs_tog), 0);
  gtk_signal_connect(GTK_OBJECT(pane.overlay.miplighten),"toggled",
                     GTK_SIGNAL_FUNC(ivs_tog), 0);

  dropbox_set_changed_callback(pane.overlay.mipsource, ivs_mip_src);

  gtk_signal_connect(GTK_OBJECT(pane.overlay.mipcurve),"expose_event",
		     GTK_SIGNAL_FUNC(mipcurve_expose), 0);
  gtk_signal_connect(GTK_OBJECT(pane.overlay.mipcurve),"button_press_event",
		     GTK_SIGNAL_FUNC(mipcurve_press), 0);
  gtk_signal_connect(GTK_OBJECT(pane.overlay.mipcurve),"motion_notify_event",
		     GTK_SIGNAL_FUNC(mipcurve_drag), 0);

  /* 2d view options */

  pane.tcontrol[6] = gtk_frame_new("2D View Options");
  tmp[0] = gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[6]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[0]), 2);  
  gtk_container_add(GTK_CONTAINER(pane.tcontrol[6]), tmp[0]);

  tmp[72] = gtk_hbox_new(FALSE, 0);
  tmp[73] = gtk_label_new("Mode");
  pane.view2d.mode = dropbox_new(";same as 3D view;solid box;objects;object borders");
  gtk_box_pack_start(GTK_BOX(tmp[0]),tmp[72],FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[72]),tmp[73],FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(tmp[72]),pane.view2d.mode->widget,FALSE,FALSE,2);

  pane.view2d.label = gtk_label_new(" ");
  gtk_box_pack_start(GTK_BOX(tmp[0]), pane.view2d.label, FALSE, FALSE, 2);
  gtk_widget_show(pane.view2d.label);

  dropbox_set_changed_callback(pane.view2d.mode, ivs_mode2d_update);

  /* rotate options */
  
  pane.tcontrol[0] = gtk_frame_new("Rotate");
  tmp[7] = gtk_vbox_new(FALSE,2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[0]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[7]), 2);

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[0]), tmp[7]);

  tmp[8] = gtk_table_new(3,12,FALSE);
  gtk_box_pack_start(GTK_BOX(tmp[7]),tmp[8],FALSE,TRUE,2);

  pane.rot.xa = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,360.0,1.0,10.0,0.0);
  pane.rot.ya = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,360.0,1.0,10.0,0.0);
  pane.rot.za = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,360.0,1.0,10.0,0.0);
  
  pane.rot.xr = gtk_spin_button_new(pane.rot.xa, 2.0, 0);
  pane.rot.yr = gtk_spin_button_new(pane.rot.ya, 2.0, 0);
  pane.rot.zr = gtk_spin_button_new(pane.rot.za, 2.0, 0);

  tmp[9] = gtk_label_new("X Axis");
  tmp[10] = gtk_label_new("Y Axis");
  tmp[11] = gtk_label_new("Z Axis");

  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[9],       0,4,  0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[10],      4,8,  0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[11],      8,12, 0,1);

  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), pane.rot.xr, 0,4,  1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), pane.rot.yr, 4,8,  1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), pane.rot.zr, 8,12, 1,2);
  
  tmp[35] = gtk_label_new("Presets ");
  tmp[36] = gtk_button_new_with_label("XY");
  tmp[37] = gtk_button_new_with_label("YZ");
  tmp[38] = gtk_button_new_with_label("XZ");

  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[35],     0,6,  2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[36],     6,8,  2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[37],     8,10,  2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[8]), tmp[38],     10,12, 2,3);

  tmp[12] = gtk_label_new("drag scene to rotate.");
  gtk_box_pack_start(GTK_BOX(tmp[7]),tmp[12],FALSE,TRUE,2);  

  pane.rot.preview = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(pane.rot.preview), 64, 64);
  gtk_box_pack_start(GTK_BOX(tmp[7]),pane.rot.preview,FALSE,TRUE,2);

  pane.toid = -1;

  /* clip options */
  pane.tcontrol[1] = gtk_frame_new("Clip");
  tmp[13] = gtk_vbox_new(FALSE,2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[1]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[13]), 2);  

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[1]), tmp[13]);

  tmp[14] = gtk_table_new(4,3,FALSE);
  gtk_box_pack_start(GTK_BOX(tmp[13]),tmp[14],FALSE,TRUE,2);

  pane.clip.xa[0] = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,256.0,1.0,10.0,0.0);
  pane.clip.ya[0] = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,256.0,1.0,10.0,0.0);
  pane.clip.za[0] = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,256.0,1.0,10.0,0.0);

  pane.clip.xa[1] = (GtkAdjustment *) gtk_adjustment_new(256.0,0.0,256.0,1.0,10.0,0.0);
  pane.clip.ya[1] = (GtkAdjustment *) gtk_adjustment_new(256.0,0.0,256.0,1.0,10.0,0.0);
  pane.clip.za[1] = (GtkAdjustment *) gtk_adjustment_new(256.0,0.0,256.0,1.0,10.0,0.0);
  
  pane.clip.xr[0] = gtk_spin_button_new(pane.clip.xa[0], 2.0, 0);
  pane.clip.yr[0] = gtk_spin_button_new(pane.clip.ya[0], 2.0, 0);
  pane.clip.zr[0] = gtk_spin_button_new(pane.clip.za[0], 2.0, 0);

  pane.clip.xr[1] = gtk_spin_button_new(pane.clip.xa[1], 2.0, 0);
  pane.clip.yr[1] = gtk_spin_button_new(pane.clip.ya[1], 2.0, 0);
  pane.clip.zr[1] = gtk_spin_button_new(pane.clip.za[1], 2.0, 0);

  tmp[15] = gtk_label_new("X Clip");
  tmp[16] = gtk_label_new("Y Clip");
  tmp[17] = gtk_label_new("Z Clip");

  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), tmp[15], 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.xr[0], 1,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.xr[1], 2,3, 0,1);

  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), tmp[16], 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.yr[0], 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.yr[1], 2,3, 1,2);

  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), tmp[17], 0,1, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.zr[0], 1,2, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), pane.clip.zr[1], 2,3, 2,3);

  tmp[39] = gtk_button_new_with_label(" Full Volume ");
  gtk_table_attach_defaults(GTK_TABLE(tmp[14]), tmp[39], 1,3, 3,4);

  tmp[18] = gtk_label_new("use the on-scene\ncontrols to clip it.");
  gtk_box_pack_start(GTK_BOX(tmp[13]),tmp[18],FALSE,TRUE,2);  

  /* light */

  pane.tcontrol[2] = gtk_frame_new("Light/Shading");
  tmp[40] = gtk_vbox_new(FALSE,2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[2]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[40]), 2);    

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[2]), tmp[40]);

  pane.light.preview = gtk_drawing_area_new();
  gtk_widget_set_events(pane.light.preview, GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK);
  gtk_drawing_area_size(GTK_DRAWING_AREA(pane.light.preview), 48, 48);

  tmp[48] = gtk_hbox_new(FALSE,2);
  tmp[49] = gtk_label_new("click preview\nto set\nlight direction.");

  gtk_box_pack_start(GTK_BOX(tmp[40]),tmp[48], FALSE, TRUE, 2);

  gtk_box_pack_start(GTK_BOX(tmp[48]),pane.light.preview, FALSE, TRUE, 2);
  gtk_widget_show(pane.light.preview);

  gtk_box_pack_start(GTK_BOX(tmp[48]),tmp[49], FALSE, TRUE, 2);
  
  pane.light.ps = SceneNew();
  pane.light.ps->colors.bg = 0xc0c0c0;
  pane.light.ps->colors.label[1] = 0xe0e060;
  pane.light.ps->colors.label[2] = 0xe0c070;
  pane.light.ps->avol = XAVolNew(48,48,24);

  pane.light.ps->options.vmode   = vmode_label;
  pane.light.ps->options.shading = shading_phong;
  pane.light.ps->options.zoom    = zoom_100;

  pane.light.ps->hollow[0] = 1;
  pane.light.ps->hollow[1] = 0;
  pane.light.ps->hollow[2] = 0;

  pane.light.ps->clip.v[0][0] = pane.light.ps->clip.v[1][0] =
    pane.light.ps->clip.v[2][0] = 0;

  pane.light.ps->clip.v[0][1] = 47;
  pane.light.ps->clip.v[1][1] = 47;
  pane.light.ps->clip.v[2][1] = 23;

  for(i=0;i<48;i++)
    for(j=0;j<48;j++)
      for(k=0;k<24;k++)
	XASetLabel(pane.light.ps->avol, 
		   i + pane.light.ps->avol->tbrow[j] + pane.light.ps->avol->tbframe[k],
		   (((i-23)*(i-23) + (j-23)*(j-23) + (k-23)*(k-23)) <= 400)?
		   ( ((i/8)+(j/8))%2 ?  1 : 2 ):0);
		     
  tmp[41] = gtk_table_new(3,4,TRUE);
  gtk_box_pack_start(GTK_BOX(tmp[40]),tmp[41],FALSE,TRUE,2);

  pane.light.ka[0] = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.ambient, 0.0,1.0,0.01,0.05,0.0);
  pane.light.ka[1] = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.diffuse, 0.0,1.0,0.01,0.05,0.0);
  pane.light.ka[2] = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.specular,0.0,1.0,0.01,0.05,0.0);

  pane.light.za[0] = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.zgamma,  0.0,1.0,0.01,0.05,0.0);
  pane.light.za[1] = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.zstretch,0.0,3.0,0.01,0.05,0.0);

  pane.light.na = (GtkAdjustment *) gtk_adjustment_new(
       scene->options.phong.sre,     1.0,100.0,1.0,10.0,0.0);

  for(i=0;i<3;i++)
    pane.light.kr[i] = gtk_spin_button_new(pane.light.ka[i], 2.0, 2);
  for(i=0;i<2;i++)
    pane.light.zr[i] = gtk_spin_button_new(pane.light.za[i], 2.0, 2);
  pane.light.nr = gtk_spin_button_new(pane.light.na, 2.0, 0);

  tmp[42] = gtk_label_new("amb");
  tmp[43] = gtk_label_new("diff");
  tmp[44] = gtk_label_new("spec");
  tmp[45] = gtk_label_new("gam");
  tmp[46] = gtk_label_new("dep");
  tmp[47] = gtk_label_new("mat");

  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[42], 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[43], 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[44], 0,1, 2,3);

  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.kr[0], 1,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.kr[1], 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.kr[2], 1,2, 2,3);
  
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[47], 2,3, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[45], 2,3, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), tmp[46], 2,3, 2,3);

  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.nr,    3,4, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.zr[0], 3,4, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[41]), pane.light.zr[1], 3,4, 2,3);

  tmp[50] = gtk_hbox_new(FALSE, 0);
  tmp[51] = gtk_label_new("presets");
  tmp[52] = gtk_button_new_with_label(" 1 ");
  tmp[53] = gtk_button_new_with_label(" 2 ");
  tmp[54] = gtk_button_new_with_label(" 3 ");

  gtk_box_pack_start(GTK_BOX(tmp[40]), tmp[50], FALSE, TRUE,2);
  gtk_box_pack_start(GTK_BOX(tmp[50]), tmp[51], FALSE, TRUE,2);
  gtk_box_pack_start(GTK_BOX(tmp[50]), tmp[52], FALSE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(tmp[50]), tmp[53], FALSE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(tmp[50]), tmp[54], FALSE, TRUE,0);

  /* colors */

  pane.tcontrol[3] = gtk_frame_new("Colors");
  tmp[55] = gtk_vbox_new(FALSE,2);
  
  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[3]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[55]), 2);    

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[3]), tmp[55]);

  tmp[56] = gtk_table_new(4,2,TRUE);
  
  gtk_box_pack_start(GTK_BOX(tmp[55]), tmp[56], FALSE, TRUE, 2);
  
  tmp[57] = gtk_label_new("presets");
  tmp[58] = gtk_button_new_with_label("hi-sat");
  tmp[59] = gtk_button_new_with_label("bone/flesh");
  tmp[60] = gtk_button_new_with_label("mono 1");
  tmp[61] = gtk_button_new_with_label("mono 2");
  tmp[71] = gtk_button_new_with_label("random");

  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[57], 0,2, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[58], 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[60], 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[59], 0,1, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[61], 1,2, 2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[56]), tmp[71], 0,1, 3,4);

  /* add seeds */
  
  pane.tcontrol[4] = gtk_frame_new("Add Seeds");
  tmp[31] = gtk_vbox_new(FALSE,2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[4]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[31]), 2);  

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[4]), tmp[31]);

  pane.add.label = picker_new(1,PICKER_STYLE_LABEL);

  for(i=0;i<10;i++)
    picker_set_color(pane.add.label, i, scene->colors.label[i]);
  picker_set_down(pane.add.label, 0, 1);

  gtk_box_pack_start(GTK_BOX(tmp[31]), pane.add.label->widget, FALSE, TRUE, 0);

  tmp[34] = gtk_label_new("click or drag over the\nscene to mark seeds.");
  gtk_box_pack_start(GTK_BOX(tmp[31]),tmp[34], FALSE, FALSE, 2);

  tmp[64] = gtk_label_new("Current selection: ");
  pane.add.count = gtk_label_new("0 seeds");
  pane.add.ln = 0;

  gtk_box_pack_start(GTK_BOX(tmp[31]), tmp[64], FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(tmp[31]), pane.add.count, FALSE, TRUE, 2);

  tmp[32] = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[31]), tmp[32], FALSE, TRUE, 2);

  tmp[33] = gtk_button_new_with_label(" clear seed set ");
  gtk_box_pack_end(GTK_BOX(tmp[32]),tmp[33], FALSE, FALSE, 2);
  
  /* del trees */

  pane.tcontrol[5] = gtk_frame_new("Remove Trees");

  tmp[65] = gtk_vbox_new(FALSE,2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.tcontrol[5]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[65]), 2);  

  gtk_container_add(GTK_CONTAINER(pane.tcontrol[5]), tmp[65]);

  tmp[66] = gtk_label_new("click scene to\nmark/unmark trees.");
  tmp[67] = gtk_label_new("Current selection: ");
  pane.del.count = gtk_label_new("0 trees");
  pane.del.ln = 0;

  gtk_box_pack_start(GTK_BOX(tmp[65]), tmp[66], FALSE, TRUE, 2);

  gtk_box_pack_start(GTK_BOX(tmp[65]), tmp[67], FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(tmp[65]), pane.del.count, FALSE, TRUE, 2);

  tmp[68] = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[65]), tmp[68], FALSE, TRUE, 2);

  tmp[69] = gtk_button_new_with_label(" unmark all ");
  gtk_box_pack_end(GTK_BOX(tmp[68]),tmp[69], FALSE, FALSE, 2);

  /* solid/hollow controls */

  pane.solid = picker_new(0,PICKER_STYLE_EYE);

  for(i=0;i<10;i++) {
    picker_set_down(pane.solid, i, !(scene->hollow[i]));
    picker_set_color(pane.solid, i, scene->colors.label[i]);
  }

  picker_set_changed_callback(pane.solid, &ivs_update);

  tmp[19] = gtk_frame_new("Object Visibility");
  tmp[20] = gtk_vbox_new(FALSE,0);

  gtk_container_set_border_width(GTK_CONTAINER(tmp[19]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[20]), 4);  

  gtk_container_add(GTK_CONTAINER(tmp[19]), tmp[20]);
  gtk_box_pack_start(GTK_BOX(tmp[20]), pane.solid->widget, FALSE, TRUE, 0);

  /* drop boxes */

  tmp[21] = gtk_frame_new("Options");
  tmp[22] = gtk_table_new(5,2,FALSE);

  gtk_container_set_border_width(GTK_CONTAINER(tmp[21]), 2);
  gtk_container_set_border_width(GTK_CONTAINER(tmp[22]), 2);  

  gtk_container_add(GTK_CONTAINER(tmp[21]), tmp[22]);

  tmp[23] = gtk_label_new("Mode");
  tmp[24] = gtk_label_new("Zoom");
  tmp[25] = gtk_label_new("Data");
  tmp[26] = gtk_label_new("Shading");
  tmp[1]  = gtk_label_new("Views");

  for(i=0;i<4;i++) {
    tmp[27+i] = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(tmp[27+i]),tmp[23+i],FALSE,FALSE,0);
  }
  tmp[2] = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(tmp[2]),tmp[1],FALSE,FALSE,0);
  
  pane.views = dropbox_new(";3D;3D + 2D");
  pane.mode  = dropbox_new(VIEW_MODES);
  pane.zoom  = dropbox_new(";50%;100%;150%;200%;300%;400%");
  pane.src   = dropbox_new(";original;current;cost");
  pane.shading = dropbox_new(SHADING_OPTIONS);

  dropbox_set_changed_callback(pane.views,   &ivs_update);
  dropbox_set_changed_callback(pane.mode,    &ivs_update);
  dropbox_set_changed_callback(pane.zoom,    &ivs_update);
  dropbox_set_changed_callback(pane.src,     &ivs_update);
  dropbox_set_changed_callback(pane.shading, &ivs_update);

  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),tmp[27],0,1,0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),tmp[28],0,1,1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),tmp[29],0,1,2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),tmp[30],0,1,3,4);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),tmp[2],0,1,4,5);

  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),pane.mode->widget,1,2,0,1);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),pane.zoom->widget,1,2,1,2);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),pane.src->widget,1,2,2,3);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),pane.shading->widget,1,2,3,4);
  gtk_table_attach_defaults(GTK_TABLE(tmp[22]),pane.views->widget,1,2,4,5);

  /* vertical slides */

  rrp = gtk_vbox_new(FALSE, 0);
  gtk_widget_set_style(rrp, black);
  gtk_widget_ensure_style(rrp);

  pane.vrange       = vmapctl_new("  VIEW RANGE  ",  26, 256, scene->vmap);
  pane.tonalization = vslide_new("TONALIZATION", 26, 200);

  pane.tonalization->value = 1.0 - scene->options.srcmix;

  vmapctl_set_changed_callback(pane.vrange, &ivs_vrange_change);
  vslide_set_changed_callback(pane.tonalization, &ivs_tonalization);

  gtk_box_pack_start(GTK_BOX(rrp), pane.vrange->widget, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(rrp), pane.tonalization->widget, FALSE, TRUE, 0);

  /* put them together */

  gtk_box_pack_start(GTK_BOX(rp), tmp[5], FALSE, TRUE, 0); /* clip / rotate */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[0], FALSE, TRUE, 0); /* rotate */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[1], FALSE, TRUE, 0); /* clip */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[2], FALSE, TRUE, 0); /* light */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[3], FALSE, TRUE, 0); /* color */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[4], FALSE, TRUE, 0); /* add */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[5], FALSE, TRUE, 0); /* del */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[6], FALSE, TRUE, 0); /* 2d */
  gtk_box_pack_start(GTK_BOX(rp), pane.tcontrol[7], FALSE, TRUE, 0); /* overlay */

  gtk_box_pack_end(GTK_BOX(rp), tmp[21], FALSE, TRUE, 0); /* options */
  gtk_box_pack_end(GTK_BOX(rp), tmp[19], FALSE, TRUE, 2); /* solid */

  gtk_box_pack_start(GTK_BOX(nbc[0]), rp, FALSE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(nbc[0]), rrp, FALSE, TRUE, 0);

  /* segmentation bar */

  pane.seg.widget = gtk_hbox_new(FALSE, 2);

  gtk_container_set_border_width(GTK_CONTAINER(pane.seg.widget), 4);

  tmp[62] = gtk_label_new("Segmentation Method: ");
  pane.seg.method = dropbox_new(SEG_OPERATORS);
  pane.seg.go     = gtk_button_new_with_label(" Calculate Segmentation ");
  gtk_box_pack_start(GTK_BOX(tmp[4]), pane.seg.widget,FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pane.seg.widget), tmp[62],FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(pane.seg.widget), pane.seg.method->widget,FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(pane.seg.widget), pane.seg.go,FALSE, TRUE, 0);

  gtk_widget_show(pane.seg.widget);
  gtk_widget_show(pane.seg.go);

  /* file manager pane */

  /* attach signals */

  gtk_signal_connect(GTK_OBJECT(mw),"destroy",
                     GTK_SIGNAL_FUNC(ivs_destroy), 0);

  gtk_signal_connect(GTK_OBJECT(mw),"delete_event",
                     GTK_SIGNAL_FUNC(ivs_delete), 0);

  gtk_signal_connect (GTK_OBJECT (mw), "key_press_event",
                      GTK_SIGNAL_FUNC (ivs_key), 0);

  gtk_signal_connect(GTK_OBJECT(canvas.itself),"expose_event",
                     GTK_SIGNAL_FUNC(ivs_view_expose), 0);

  gtk_signal_connect(GTK_OBJECT(canvas.itself),"button_press_event",
                     GTK_SIGNAL_FUNC(ivs_view_press), 0);

  gtk_signal_connect(GTK_OBJECT(canvas.itself),"button_release_event",
                     GTK_SIGNAL_FUNC(ivs_view_release), 0);

  gtk_signal_connect(GTK_OBJECT(canvas.itself),"motion_notify_event",
                     GTK_SIGNAL_FUNC(ivs_view_drag), 0);

  gtk_signal_connect(GTK_OBJECT(tmp[36]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_rot_preset), (gpointer) 0);
  gtk_signal_connect(GTK_OBJECT(tmp[37]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_rot_preset), (gpointer) 1);
  gtk_signal_connect(GTK_OBJECT(tmp[38]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_rot_preset), (gpointer) 2);
  gtk_signal_connect(GTK_OBJECT(tmp[39]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_clip_preset), (gpointer) 0);

  for(i=0;i<NTOOLS;i++)
    gtk_signal_connect(GTK_OBJECT(pane.tools[i]),"toggled",
		       GTK_SIGNAL_FUNC(ivs_tool), (gpointer) i);

  gtk_signal_connect(GTK_OBJECT(pane.rot.preview),"expose_event",
                     GTK_SIGNAL_FUNC(ivs_rot_expose), 0);

  gtk_signal_connect(GTK_OBJECT(pane.rot.xa),"value_changed",
                     GTK_SIGNAL_FUNC(ivs_rot), 0);
  gtk_signal_connect(GTK_OBJECT(pane.rot.ya),"value_changed",
                     GTK_SIGNAL_FUNC(ivs_rot), 0);
  gtk_signal_connect(GTK_OBJECT(pane.rot.za),"value_changed",
                     GTK_SIGNAL_FUNC(ivs_rot), 0);

  for(i=0;i<2;i++)
    gtk_signal_connect(GTK_OBJECT(pane.clip.xa[i]),"value_changed",
		       GTK_SIGNAL_FUNC(ivs_rot), 0);
  for(i=0;i<2;i++)
    gtk_signal_connect(GTK_OBJECT(pane.clip.ya[i]),"value_changed",
		       GTK_SIGNAL_FUNC(ivs_rot), 0);
  for(i=0;i<2;i++)
    gtk_signal_connect(GTK_OBJECT(pane.clip.za[i]),"value_changed",
		       GTK_SIGNAL_FUNC(ivs_rot), 0);

  gtk_signal_connect(GTK_OBJECT(pane.light.preview),"expose_event",
                     GTK_SIGNAL_FUNC(ivs_ball_expose), 0);
  gtk_signal_connect(GTK_OBJECT(pane.light.preview),"button_press_event",
                     GTK_SIGNAL_FUNC(ivs_ball_press), 0);

  for(i=0;i<3;i++)
    gtk_signal_connect(GTK_OBJECT(pane.light.ka[i]),"value_changed",
		       GTK_SIGNAL_FUNC(ivs_rot), 0);
  for(i=0;i<2;i++)
    gtk_signal_connect(GTK_OBJECT(pane.light.za[i]),"value_changed",
		       GTK_SIGNAL_FUNC(ivs_rot), 0);
  gtk_signal_connect(GTK_OBJECT(pane.light.na),"value_changed",
		     GTK_SIGNAL_FUNC(ivs_rot), 0);

  gtk_signal_connect(GTK_OBJECT(tmp[52]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_light_preset), (gpointer) 0);
  gtk_signal_connect(GTK_OBJECT(tmp[53]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_light_preset), (gpointer) 1);
  gtk_signal_connect(GTK_OBJECT(tmp[54]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_light_preset), (gpointer) 2);

  gtk_signal_connect(GTK_OBJECT(tmp[58]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_color_preset), (gpointer) 0);
  gtk_signal_connect(GTK_OBJECT(tmp[59]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_color_preset), (gpointer) 1);
  gtk_signal_connect(GTK_OBJECT(tmp[60]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_color_preset), (gpointer) 2);
  gtk_signal_connect(GTK_OBJECT(tmp[61]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_color_preset), (gpointer) 3);
  gtk_signal_connect(GTK_OBJECT(tmp[71]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_color_preset), (gpointer) 4);

  gtk_signal_connect(GTK_OBJECT(tmp[33]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_clear_addseeds), (gpointer) 0);

  gtk_signal_connect(GTK_OBJECT(tmp[69]),"clicked",
                     GTK_SIGNAL_FUNC(ivs_unmark_trees), (gpointer) 0);

  gtk_signal_connect(GTK_OBJECT(pane.seg.go),"clicked",
                     GTK_SIGNAL_FUNC(ivs_segment), (gpointer) 0);

  /* call show on everything */

  for(i=0;i<2;i++) {
    gtk_widget_show(pane.clip.xr[i]);
    gtk_widget_show(pane.clip.yr[i]);
    gtk_widget_show(pane.clip.zr[i]);
  }

  for(i=0;i<2;i++)
    gtk_widget_show(pane.light.zr[i]);
  for(i=0;i<3;i++)
    gtk_widget_show(pane.light.kr[i]);
  gtk_widget_show(pane.light.nr);

  gtk_widget_show(pane.rot.xr);
  gtk_widget_show(pane.rot.yr);
  gtk_widget_show(pane.rot.zr);
  gtk_widget_show(pane.rot.preview);
  gtk_widget_show(pane.tcontrol[0]);
  gtk_widget_show(pane.add.count);
  gtk_widget_show(pane.del.count);

  for(i=0;i<NTOOLS;i++)
    gtk_widget_show(pane.tools[i]);

  // HERE
  for(i=0;i<77;i++)
    gtk_widget_show(tmp[i]);

  gtk_widget_show(rp);
  gtk_widget_show(rrp);
  gtk_widget_show(canvas.itself);

  gdk_window_set_cursor(canvas.itself->window, crosshair);
  gdk_window_set_cursor(preproc.canvas->window, crosshair);

  for(i=0;i<4;i++) {
    gtk_widget_show(nbc[i]);
    gtk_widget_show(nbl[i]);
  }
  gtk_widget_show(nb);
  gtk_widget_show(v);
  gtk_widget_show(mw);

  pane.tool = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[0]),1);
  dropbox_set_selection(pane.zoom, 1);

  gtk_timeout_add(300, ivs_scan, 0);
}

void ivs_fpars(CBARG) {
  if (scene->avol)
    ivs_get_fuzzy_pars(0);
  else
    ivs_warn_no_volume();
}

void ivs_anim_sweep_full(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_SWEEP, 1);
  else
    ivs_warn_no_volume();
}

void ivs_anim_sweep_quick(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_SWEEP, 4);
  else
    ivs_warn_no_volume();
}

void ivs_anim_roty_full(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_ROT_Y, 3);
  else
    ivs_warn_no_volume();
}

void ivs_anim_roty_quick(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_ROT_Y, 10);
  else
    ivs_warn_no_volume();
}

void ivs_anim_wave_full(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_WAVEFRONT, 256);
  else
    ivs_warn_no_volume();
}

void ivs_anim_wave_quick(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_WAVEFRONT, 1024);
  else
    ivs_warn_no_volume();
}

void ivs_anim_grow_full(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_GROWTH, 256);
  else
    ivs_warn_no_volume();
}

void ivs_anim_grow_quick(CBARG) {
  if (scene->avol)
    animate(mainwindow, scene, ANIM_GROWTH, 1024);
  else
    ivs_warn_no_volume();
}


void ivs_histogram(CBARG) {
  if (scene->avol)
    open_histogram(mainwindow);
  else
    ivs_warn_no_volume();
}

void ivs_vspec_save(CBARG) {
  char name[512];

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  if (ivs_get_filename(mainwindow,
		       "Save View Specification to... (.ivs)", 
		       name, 
		       GF_ENSURE_WRITEABLE)) 
    {
      ivs_viewspec_save(name);
    }
}

void ivs_vspec_load(CBARG) {
  char name[512];

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  if (ivs_get_filename(mainwindow,
		       "Load View Specification from... (.ivs)", 
		       name, 
		       GF_ENSURE_READABLE)) 
    {
      ivs_viewspec_load(name);
    }
}

void ivs_make_cube() {
  Transform a,b,c,d;
  Cuboid *f, *g;
  float vtx, vty, vtz, tx, ty, ta;
  int i;
  Point p[12];

  if (scene->avol) {
    
    f = &(canvas.fullcube);
    g = &(canvas.clipcube);

    CuboidOrthogonalInit(f,
			 0.0, scene->avol->W-1.0, 
			 0.0, scene->avol->H-1.0, 
			 0.0, scene->avol->D-1.0);
    
    CuboidOrthogonalInit(g,
			 scene->clip.v[0][0], scene->clip.v[0][1],
			 scene->clip.v[1][0], scene->clip.v[1][1],
			 scene->clip.v[2][0], scene->clip.v[2][1]);
    
    vtx = -(scene->avol->W / 2.0);
    vty = -(scene->avol->H / 2.0);
    vtz = -(scene->avol->D / 2.0);
    CuboidTranslate(f, vtx, vty, vtz);
    CuboidTranslate(g, vtx, vty, vtz);

    TransformIsoScale(&a, SceneScale(scene));
    TransformZRot(&b, scene->rotation.z);
    TransformYRot(&c, scene->rotation.y);
    TransformXRot(&d, scene->rotation.x);

    TransformCompose(&b,&a);
    TransformCompose(&c,&b);
    TransformCompose(&d,&c); // d <= ((a x b) x c) x d

    CuboidTransform(f, &d);
    CuboidTransform(g, &d);

    if (canvas.fourview) {
      tx = canvas.SW + canvas.SW / 2.0;
      ty = canvas.SH + canvas.SH / 2.0;
    } else {
      tx = canvas.W / 2.0; 
      ty = canvas.H / 2.0;
    }
    CuboidTranslate(f, tx, ty, 0.0);
    CuboidTranslate(g, tx, ty, 0.0);

    CuboidRemoveHiddenFaces(f);
    CuboidRemoveHiddenFaces(g);

    // calculate clipping controls

    // x clip
    PointAssign(&p[0], 0.0, -5.0, 0.0);
    PointAssign(&p[1], 0.0, -10.0, 0.0);
    PointAssign(&p[2], scene->avol->W, -10.0, 0.0);
    PointAssign(&p[3], scene->avol->W,  -5.0, 0.0);
    PointAssign(&p[4], scene->clip.v[0][0], -10.0,  0.0);
    PointAssign(&p[5], scene->clip.v[0][0], -20.0,  5.0);
    PointAssign(&p[6], scene->clip.v[0][0], -20.0, -5.0);
    PointAssign(&p[7], scene->clip.v[0][1], -10.0,  0.0);
    PointAssign(&p[8], scene->clip.v[0][1], -20.0,  5.0);
    PointAssign(&p[9], scene->clip.v[0][1], -20.0, -5.0);
    PointAssign(&p[10], scene->clip.v[0][0], -5.0,  0.0);
    PointAssign(&p[11], scene->clip.v[0][1], -5.0,  0.0);

    if (f->hidden[0] != f->hidden[2]) ta=0.0; else ta=scene->avol->D;

    for(i=0;i<12;i++) {
      PointTranslate(&p[i], vtx, vty, vtz + ta);
      PointTransform(&canvas.cc[0][i], &p[i], &d);
      PointTranslate(&canvas.cc[0][i], tx, ty, 0.0);
    }

    // y clip
    PointAssign(&p[0], scene->avol->W +  5.0, 0.0, 0.0);
    PointAssign(&p[1], scene->avol->W + 10.0, 0.0, 0.0);
    PointAssign(&p[2], scene->avol->W + 10.0, scene->avol->H, 0.0);
    PointAssign(&p[3], scene->avol->W +  5.0, scene->avol->H, 0.0);
    PointAssign(&p[4], scene->avol->W + 10.0, scene->clip.v[1][0],  0.0);
    PointAssign(&p[5], scene->avol->W + 20.0, scene->clip.v[1][0],  5.0);
    PointAssign(&p[6], scene->avol->W + 20.0, scene->clip.v[1][0], -5.0);
    PointAssign(&p[7], scene->avol->W + 10.0, scene->clip.v[1][1],  0.0);
    PointAssign(&p[8], scene->avol->W + 20.0, scene->clip.v[1][1],  5.0);
    PointAssign(&p[9], scene->avol->W + 20.0, scene->clip.v[1][1], -5.0);
    PointAssign(&p[10], scene->avol->W + 5.0, scene->clip.v[1][0],  0.0);
    PointAssign(&p[11], scene->avol->W + 5.0, scene->clip.v[1][1],  0.0);

    if (f->hidden[0] != f->hidden[4]) ta=0.0; else ta=scene->avol->D;

    for(i=0;i<12;i++) {
      PointTranslate(&p[i], vtx, vty, vtz + ta);
      PointTransform(&canvas.cc[1][i], &p[i], &d);
      PointTranslate(&canvas.cc[1][i], tx, ty, 0.0);
    }

    // z clip
    PointAssign(&p[0], 0.0, scene->avol->H +  5.0, 0.0);
    PointAssign(&p[1], 0.0, scene->avol->H + 10.0, 0.0);
    PointAssign(&p[2], 0.0, scene->avol->H + 10.0, scene->avol->D);
    PointAssign(&p[3], 0.0, scene->avol->H +  5.0, scene->avol->D);
    PointAssign(&p[4],  0.0, scene->avol->H + 10.0, scene->clip.v[2][0]);
    PointAssign(&p[5], -5.0, scene->avol->H + 20.0, scene->clip.v[2][0]);
    PointAssign(&p[6],  5.0, scene->avol->H + 20.0, scene->clip.v[2][0]);
    PointAssign(&p[7],  0.0, scene->avol->H + 10.0, scene->clip.v[2][1]);
    PointAssign(&p[8], -5.0, scene->avol->H + 20.0, scene->clip.v[2][1]);
    PointAssign(&p[9],  5.0, scene->avol->H + 20.0, scene->clip.v[2][1]);
    PointAssign(&p[10],  0.0, scene->avol->H + 5.0, scene->clip.v[2][0]);
    PointAssign(&p[11],  0.0, scene->avol->H + 5.0, scene->clip.v[2][1]);

    if (f->hidden[3] != f->hidden[5]) ta=0.0; else ta=scene->avol->W;

    for(i=0;i<12;i++) {
      PointTranslate(&p[i], vtx + ta, vty, vtz);
      PointTransform(&canvas.cc[2][i], &p[i], &d);
      PointTranslate(&canvas.cc[2][i], tx, ty, 0.0);
    }

  }
}

void ivs_draw_amesh() {
  GdkPoint T[3];
  int i,j,k,c;
  Triangle *t;

  for(i=0;i<scene->amesh.ntriangles;i++) {

    j = scene->amesh.tsort[i];
    for(k=0;k<3;k++) {
      T[k].x = (int) scene->amesh.tmpt[j].p[k].x;
      T[k].y = (int) scene->amesh.tmpt[j].p[k].y;
    }

    t = &(scene->amesh.srct[j]);
    
    if (((int)(t->p[0].x)) < scene->clip.v[0][0] ||
	((int)(t->p[0].x)) > scene->clip.v[0][1] ||
	((int)(t->p[0].y)) < scene->clip.v[1][0] ||
	((int)(t->p[0].y)) > scene->clip.v[1][1] ||
	((int)(t->p[0].z)) < scene->clip.v[2][0] ||
	((int)(t->p[0].z)) > scene->clip.v[2][1]) {

      c = mergeColorsRGB(0, 
			 scene->colors.bg,
			 0.9-0.6*scene->amesh.tz[j]);
      gc_color(canvas.gc, c);
      gdk_draw_polygon(canvas.vista, canvas.gc, TRUE, T, 3);

      c = mergeColorsRGB(0, 
			 scene->colors.label[scene->amesh.tl[j]], 
			 1.0-0.6*scene->amesh.tz[j]);
      gc_color(canvas.gc, c);
      gdk_draw_polygon(canvas.vista, canvas.gc, FALSE, T, 3);

    } else {
      c = mergeColorsRGB(0, 
			 scene->colors.label[scene->amesh.tl[j]], 
			 0.2+0.8*AMeshCos(scene, j));
      c = mergeColorsRGB(0, 
			 c,
			 1.0-0.6*scene->amesh.tz[j]);

      gc_color(canvas.gc, c);
      gdk_draw_polygon(canvas.vista, canvas.gc, TRUE, T, 3);
    }
  }
}

void ivs_draw_cube() {
  int i,hc,c1,c2,c3,c4;
  GdkRectangle r;

  if (canvas.fourview) {
    r.x = r.width = canvas.SW;
    r.y = r.height = canvas.SH;
    gdk_gc_set_clip_rectangle(canvas.gc, &r);
  }

  if (canvas.wiremode) {
    draw_cuboid_fill(&(canvas.fullcube), canvas.vista, canvas.gc,
		     darker(scene->colors.bg,1));
    draw_cuboid_fill(&(canvas.clipcube), canvas.vista, canvas.gc,
		     darker(scene->colors.bg,3));

    if (CalcAMesh(scene,
		  scene->rotation.x, 
		  scene->rotation.y,
		  scene->rotation.z)) 
      ivs_draw_amesh();

    hc = darker(scene->colors.bg, 4);
    draw_cuboid_frame(&(canvas.fullcube), canvas.vista, canvas.gc,
		      hc, scene->colors.fullcube, 1);
    draw_cuboid_frame(&(canvas.clipcube), canvas.vista, canvas.gc,
		      hc, scene->colors.clipcube, 1);
  } else {
    draw_cuboid_frame(&(canvas.fullcube), canvas.vista, canvas.gc,
		      0, scene->colors.fullcube, 0);
    draw_cuboid_frame(&(canvas.clipcube), canvas.vista, canvas.gc,
		      0, scene->colors.clipcube, 0);
  }

  c1 = darker(scene->colors.bg,3);
  c2 = lighter(scene->colors.bg,2);
  c3 = mergeColorsRGB(c2,0xff0000,0.66);
  c4 = mergeColorsRGB(c1,0xff0000,0.66);

  for(i=0;i<3;i++)
    draw_clip_control(canvas.cc[i], canvas.vista, canvas.gc,
		      canvas.keysel==i?c4:c1, 
		      scene->colors.fullcube,
		      canvas.closer == 2*i ? c3 : c2, 
		      scene->colors.clipcube,
		      canvas.closer == 2*i+1 ? c3 : c2, 
		      scene->colors.clipcube);

  if (canvas.fourview) {
    r.x = r.y = 0;
    r.width = canvas.W;
    r.height = canvas.H;
    gdk_gc_set_clip_rectangle(canvas.gc, &r);

    gc_color(canvas.gc, c1);
    gdk_draw_line(canvas.vista, canvas.gc, 0, canvas.SH, 
		  canvas.W,canvas.SH);
    gdk_draw_line(canvas.vista, canvas.gc, canvas.SW, 0, 
		  canvas.SW,canvas.H);
  }



}

void ivs_update_status() {
  static int cursor_state = 0;
  int i,n;
  Task *x;
  struct timeval now;
  char z[128];
  float val;
  int min, sec, dec;

  TaskQueueLock(qseg);
  
  n = GetTaskCount(qseg);
  
  if (!n) {
    for(i=0;i<5;i++) gtk_widget_hide(status.task[i]);
    gtk_label_set_text(GTK_LABEL(status.activetask), " ");
    refresh(status.activetask);

    if (cursor_state == 1) {
      gdk_window_set_cursor(mainwindow->window, arrow);
      gdk_window_set_cursor(canvas.itself->window, crosshair);
      gdk_window_set_cursor(preproc.canvas->window, crosshair);
      gdk_window_set_cursor(mgr.canvas->window, arrow);
      cursor_state = 0;
    }

  } else {
    for(i=0;i<5;i++)
      ivs_condshow(n>i, status.task[4-i]);
    x = GetNthTask(qseg, 0);
    gettimeofday(&now,0);

    val = (float) CalcTimevalDiff(&(x->start), &now);
    sec = (int) val;
    dec = (int) ((val - floor(val)) * 10.0);
    min = sec / 60;
    sec %= 60;

    snprintf(z,127,"(%.1d:%.2d.%.1d) %s",min,sec,dec,x->desc);
    z[127] = 0;
    gtk_label_set_text(GTK_LABEL(status.activetask), z);
    refresh(status.activetask);

    if (cursor_state == 0) {
      gdk_window_set_cursor(mainwindow->window, watch);
      gdk_window_set_cursor(canvas.itself->window, watch);
      gdk_window_set_cursor(preproc.canvas->window, watch);
      gdk_window_set_cursor(mgr.canvas->window, watch);
      cursor_state = 1;
    }

  }

  TaskQueueUnlock(qseg);
  status.ln = n;
}

/* signal handlers */

gboolean 
ivs_delete(GtkWidget *w, GdkEvent *e, gpointer data) { return FALSE; }

void 
ivs_destroy(GtkWidget *w, gpointer data) { gtk_main_quit(); }

gboolean  
ivs_timed_update(gpointer data) {
  pane.toid = -1;
  ivs_pane_to_scene(M_ALL);
  refresh(canvas.itself);
  if (pane.tool == 0)
    refresh(pane.rot.preview);
  if (pane.tool == 2)
    refresh(pane.light.preview);
  return FALSE;
}

void      
ivs_update(void *obj) {
  ivs_pane_to_scene(M_ALL);
  refresh(canvas.itself);
}

void
ivs_mode2d_update(void *obj) {
  canvas.fvdirty = 1;
  refresh(canvas.itself);
}

void
ivs_vrange_change(void *obj) {
  scene->invalid = 1;
  ivs_rot(0,0);
}

void ivs_tonalization(void *obj) {
  scene->options.srcmix = 1.0 - pane.tonalization->value;
  ivs_rot(0,0);
}

void
ivs_tog(GtkToggleButton *tog, gpointer data) {
  scene->invalid = 1;
  if (pane.toid >= 0)
    gtk_timeout_remove(pane.toid);
  pane.toid = gtk_timeout_add(500, ivs_timed_update, 0);
}

void
ivs_mip_src(void *obj) {
  scene->invalid = 1;
  if (pane.toid >= 0)
    gtk_timeout_remove(pane.toid);
  pane.toid = gtk_timeout_add(500, ivs_timed_update, 0);
}

void
ivs_rot(GtkAdjustment *adj, gpointer data) {
  if (pane.toid >= 0)
    gtk_timeout_remove(pane.toid);
  
  pane.toid = gtk_timeout_add(500, ivs_timed_update, 0);
}

void
ivs_clip_preset(GtkWidget *w, gpointer data) {
  if (scene->avol) {
    scene->clip.v[0][0] = scene->clip.v[1][0] = scene->clip.v[2][0] = 0;
    scene->clip.v[0][1] = scene->avol->W - 1;
    scene->clip.v[1][1] = scene->avol->H - 1;
    scene->clip.v[2][1] = scene->avol->D - 1;
	
    gtk_adjustment_set_value(pane.clip.xa[0], scene->clip.v[0][0]);
    gtk_adjustment_set_value(pane.clip.xa[1], scene->clip.v[0][1]);
    gtk_adjustment_set_value(pane.clip.ya[0], scene->clip.v[1][0]);
    gtk_adjustment_set_value(pane.clip.ya[1], scene->clip.v[1][1]);
    gtk_adjustment_set_value(pane.clip.za[0], scene->clip.v[2][0]);
    gtk_adjustment_set_value(pane.clip.za[1], scene->clip.v[2][1]);

    refresh(canvas.itself);
  }
}

void
ivs_clear_addseeds(GtkWidget *w, gpointer data) {
  if (scene->avol) {
    MapClear(scene->avol->addseeds);
    scene->invalid = 1;
    ivs_scene_to_pane(M_ADD);
    refresh(canvas.itself);
  }
}

void
ivs_unmark_trees(GtkWidget *w, gpointer data) {
  int i;
  if (scene->avol) {
    MapClear(scene->avol->delseeds);
    scene->invalid = 1;
    for(i=0;i<scene->avol->N;i++)
      scene->avol->vd[i].label &= LABELMASK;
    ivs_scene_to_pane(M_DEL);
    refresh(canvas.itself);
  }
}

void
ivs_light_preset(GtkWidget *w, gpointer data) {
  int i = (int) data;
  PhongParameters *p;

  p = &(pane.light.ps->options.phong);

  switch(i) {
  case 0: 
    VectorAssign(&(p->light), 0.0, 0.0, -1.0);
    p->ambient = 0.20; p->diffuse = 0.60; p->specular = 0.40;
    p->zgamma  = 0.25;  p->zstretch = 1.25; p->sre = 10;
    break;
  case 1: 
    VectorAssign(&(p->light), -0.8, -0.8, -1.0);
    VectorNormalize(&(p->light));
    p->ambient = 0.20; p->diffuse = 0.60; p->specular = 0.70;
    p->zgamma  = 0.25;  p->zstretch = 1.25; p->sre = 10;
    break;
  case 2: 
    VectorAssign(&(p->light), -0.5, -0.5, -1.0);
    VectorNormalize(&(p->light));
    p->ambient = 0.20; p->diffuse = 0.70; p->specular = 0.00;
    p->zgamma  = 0.25;  p->zstretch = 1.25; p->sre = 10;
    break;
  }

  MemCpy(&(scene->options.phong), p, sizeof(PhongParameters));
  refresh(pane.light.preview);

  gtk_adjustment_set_value(pane.light.ka[0], scene->options.phong.ambient);
  gtk_adjustment_set_value(pane.light.ka[1], scene->options.phong.diffuse);
  gtk_adjustment_set_value(pane.light.ka[2], scene->options.phong.specular);
  gtk_adjustment_set_value(pane.light.za[0], scene->options.phong.zgamma);
  gtk_adjustment_set_value(pane.light.za[1], scene->options.phong.zstretch);
  gtk_adjustment_set_value(pane.light.na, scene->options.phong.sre);

  refresh(canvas.itself);
}

void
ivs_color_preset(GtkWidget *w, gpointer data) {
  int i = (int) data;
  int j;

  switch(i) {
  case 0: SceneLoadColorScheme(scene, SCHEME_DEFAULT);   break;
  case 1: SceneLoadColorScheme(scene, SCHEME_BONEFLESH); break;
  case 2: SceneLoadColorScheme(scene, SCHEME_GSBLACK);   break;
  case 3: SceneLoadColorScheme(scene, SCHEME_GSWHITE);   break;
  case 4: SceneRandomColorScheme(scene); break;
  }

  for(j=0;j<10;j++) {
    picker_set_color(pane.solid,j,scene->colors.label[j]);
    picker_set_color(pane.add.label,j,scene->colors.label[j]);
  }

  refresh(pane.solid->widget);
  refresh(pane.add.label->widget);

  scene->invalid = 1;
  refresh(canvas.itself);
}

void
ivs_rot_preset(GtkWidget *w, gpointer data) {
  int i = (int) data;

  switch(i) {
  case 0: VectorAssign(&(scene->rotation),   0.0,  0.0, 0.0); break;
  case 1: VectorAssign(&(scene->rotation), 270.0, 90.0, 0.0); break;
  case 2: VectorAssign(&(scene->rotation), 270.0,  0.0, 0.0); break;
  default: return;
  }

  gtk_adjustment_set_value(pane.rot.xa, scene->rotation.x);
  gtk_adjustment_set_value(pane.rot.ya, scene->rotation.y);
  gtk_adjustment_set_value(pane.rot.za, scene->rotation.z);
  refresh(pane.rot.preview);

  refresh(canvas.itself);
}

gboolean
ivs_rot_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) {
  GdkWindow *w = widget->window;
  GdkPoint arrow[3];

  int ww, wh, i, cx, cy, sx, sy;
  float s, h, v;
  Point x,y,z, nx, ny, nz, a[12], b[12];
  Transform tx, ty, tz;

  gdk_window_get_size(w, &ww, &wh);

  if (ww < wh) s = ww / 2.0; else s = wh / 2.0;

  PointAssign(&x, s  , 0.0, 0.0);
  PointAssign(&y, 0.0, s  , 0.0);
  PointAssign(&z, 0.0, 0.0, s  );

  h = 0.2;
  v = 0.8;

  PointAssign(&a[0],  v*s  ,  h*s, 0.0);
  PointAssign(&a[1],  v*s  , -h*s, 0.0);
  PointAssign(&a[2],  h*s  ,  v*s, 0.0);
  PointAssign(&a[3], -h*s  ,  v*s, 0.0);
  PointAssign(&a[4],  0.0  ,  h*s, v*s);
  PointAssign(&a[5],  0.0  , -h*s, v*s);

  PointAssign(&a[6],   v*s  ,  0.0,  h*s);
  PointAssign(&a[7],   v*s  ,  0.0, -h*s);
  PointAssign(&a[8],   0.0  ,  v*s,  h*s);
  PointAssign(&a[9],   0.0  ,  v*s, -h*s);
  PointAssign(&a[10],  h*s  ,  0.0,  v*s);
  PointAssign(&a[11], -h*s  ,  0.0,  v*s);

  TransformXRot(&tx, pane.rot.xa->value);
  TransformYRot(&ty, pane.rot.ya->value);
  TransformZRot(&tz, pane.rot.za->value);
  TransformCompose(&ty, &tz);
  TransformCompose(&tx, &ty);

  PointTransform(&nx, &x, &tx);
  PointTransform(&ny, &y, &tx);
  PointTransform(&nz, &z, &tx);

  for(i=0;i<12;i++)
    PointTransform(&b[i], &a[i], &tx);

  gtk_draw_flat_box(widget->style, w, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
		    0,0,ww,wh);

  cx = ww / 2;
  cy = wh / 2;

  PointTranslate(&nx, cx, cy, 0);
  PointTranslate(&ny, cx, cy, 0);
  PointTranslate(&nz, cx, cy, 0);
  for(i=0;i<12;i++) PointTranslate(&b[i],cx,cy,0);

  gdk_draw_line(w, widget->style->fg_gc[0], cx,cy,nx.x,nx.y);
  gdk_draw_line(w, widget->style->fg_gc[0], cx,cy,ny.x,ny.y);
  gdk_draw_line(w, widget->style->fg_gc[0], cx,cy,nz.x,nz.y);

  arrow[0].x = nx.x;   arrow[0].y = nx.y;
  arrow[1].x = b[0].x; arrow[1].y = b[0].y;
  arrow[2].x = b[1].x; arrow[2].y = b[1].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);
  arrow[0].x = ny.x;   arrow[0].y = ny.y;
  arrow[1].x = b[2].x; arrow[1].y = b[2].y;
  arrow[2].x = b[3].x; arrow[2].y = b[3].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);
  arrow[0].x = nz.x;   arrow[0].y = nz.y;
  arrow[1].x = b[4].x; arrow[1].y = b[4].y;
  arrow[2].x = b[5].x; arrow[2].y = b[5].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);

  arrow[0].x = nx.x;   arrow[0].y = nx.y;
  arrow[1].x = b[6].x; arrow[1].y = b[6].y;
  arrow[2].x = b[7].x; arrow[2].y = b[7].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);
  arrow[0].x = ny.x;   arrow[0].y = ny.y;
  arrow[1].x = b[8].x; arrow[1].y = b[8].y;
  arrow[2].x = b[9].x; arrow[2].y = b[9].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);
  arrow[0].x = nz.x;   arrow[0].y = nz.y;
  arrow[1].x = b[10].x; arrow[1].y = b[10].y;
  arrow[2].x = b[11].x; arrow[2].y = b[11].y;
  gdk_draw_polygon(w, widget->style->fg_gc[0], TRUE, arrow, 3);

  sx = nx.x; sy = nx.y; 
  if (sy > cy) sy-=5; else sy+=10;
  if (sx < cx) sx-=10; else sx+=5;
  gtk_draw_string(widget->style, w, GTK_STATE_NORMAL, sx, sy, "x");

  sx = ny.x; sy = ny.y; 
  if (sy > cy) sy-=5; else sy+=10;
  if (sx < cx) sx-=10; else sx+=5;
  gtk_draw_string(widget->style, w, GTK_STATE_NORMAL, sx, sy, "y");

  sx = nz.x; sy = nz.y; 
  if (sy > cy) sy-=5; else sy+=10;
  if (sx < cx) sx-=10; else sx+=5;
  gtk_draw_string(widget->style, w, GTK_STATE_NORMAL, sx, sy, "z");

  return FALSE;
}

// MOUSE ACTION (DRAG, 2D)
void ivs_view_drag_2D(int x,int y,int b) {
  int px,py,pz,q=-1;

  px = canvas.fvx;
  py = canvas.fvy;
  pz = canvas.fvz;

  if (!scene->avol) return;

  // XY image
  if (inbox(x,y,0,0,canvas.SW,canvas.SH)) {
    q = 0;
    px = x - (canvas.fox[0] + canvas.fvtx[0]);
    py = y - (canvas.foy[0] + canvas.fvty[0]);
    px = (int) floor(((double) px) / SceneScale(scene));
    py = (int) floor(((double) py) / SceneScale(scene));
  } else if (inbox(x,y,canvas.SW,0,canvas.SW,canvas.SH)) {
    q = 1;
    pz = x - (canvas.fox[1] + canvas.fvtx[1]);
    py = y - (canvas.foy[1] + canvas.fvty[1]);
    pz = (int) floor(((double) pz) / SceneScale(scene));
    py = (int) floor(((double) py) / SceneScale(scene));
  } else if (inbox(x,y,0,canvas.SH,canvas.SW,canvas.SH)) {
    q = 2;
    px = x - (canvas.fox[2] + canvas.fvtx[2]);
    pz = y - (canvas.foy[2] + canvas.fvty[2]);
    px = (int) floor(((double) px) / SceneScale(scene));
    pz = (int) floor(((double) pz) / SceneScale(scene));
  }

  // ignore clicks out of bounds (except for middle button pan)
  if (b!=2)
    if (px < 0 || px >= scene->avol->W ||
	py < 0 || py >= scene->avol->H ||
	pz < 0 || pz >= scene->avol->D ||
	q < 0)
      return;

  // LEFT button
  if (b==1) {
    switch(pane.tool) {
    case 0: // rot
    case 1: // clip
    case 2: // light
    case 3: // color
    case 6: // 2d view options
    case 7: // overlay
      canvas.fvx = px;
      canvas.fvy = py;
      canvas.fvz = pz;
      canvas.fvdirty = 1;
      ivs_scene_to_pane(M_VIEW2D);
      refresh(canvas.itself);
      break;
    case 4: // add seeds
      if (q == canvas.fvasd[3]) {
	if (SceneMarkSeedLine(scene, 
			      canvas.fvasd[0],canvas.fvasd[1],canvas.fvasd[2],
			      px,py,pz, picker_get_selection(pane.add.label)))
	  {
	    ivs_scene_to_pane(M_ADD);
	    canvas.fvdirty = 1;
	    refresh(canvas.itself);
	  }
      }
      canvas.fvasd[0] = px;
      canvas.fvasd[1] = py;
      canvas.fvasd[2] = pz;
      canvas.fvasd[3] = q;
      break;
    }
  }

  // panning
  if (b==2 && canvas.fvpan[2]) {
    canvas.fvtx[0] += (x - canvas.fvpan[0]);
    canvas.fvty[0] += (y - canvas.fvpan[1]);

    canvas.fvtx[1] += (x - canvas.fvpan[0]);
    canvas.fvty[1] += (y - canvas.fvpan[1]);

    canvas.fvtx[2] += (x - canvas.fvpan[0]);
    canvas.fvty[2] += (y - canvas.fvpan[1]);

    canvas.fvpan[0] = x;
    canvas.fvpan[1] = y;

    refresh(canvas.itself);
  }
}

// MOUSE ACTION (DRAG)
gboolean ivs_view_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data) 
{
  int x,y,px,py,b, dx, dy, i, j, k,l, pc, lm[3];
  Vector AP,BP,d,d2;
  float ad,ac;

  x = (int) em->x; y = (int) em->y; 
  b = 0;
  if (em->state & GDK_BUTTON1_MASK) b=1;
  if (em->state & GDK_BUTTON2_MASK) b=2;
  if (em->state & GDK_BUTTON3_MASK) b=3;
  px = x; py = y;

  if (canvas.fourview) {
    if (!canvas.wiremode)
      if (!inbox(x,y,canvas.SW,canvas.SH,canvas.SW,canvas.SH) || 
	  canvas.fvpan[2]) {
	ivs_view_drag_2D(x,y,b);
	return FALSE;
      }
    px -= canvas.SW;
    py -= canvas.SH;
  }

  px += canvas.odelta[0];
  py += canvas.odelta[1];

  if (b==0) {
    pc = canvas.closer;
    canvas.closer = -1;

    switch(pane.tool) {
    case 1: // clip
      j = 500000;
      for(i=0;i<3;i++) {
	k = (x - canvas.cc[i][4].x);
	l = (y - canvas.cc[i][4].y);
	k = k*k + l*l;
	if (k < j) { j=k; canvas.closer = 2*i; }
	k = (x - canvas.cc[i][7].x);
	l = (y - canvas.cc[i][7].y);
	k = k*k + l*l;
	if (k < j) { j=k; canvas.closer = 2*i+1; }
      }

      if (canvas.closer != pc)
	refresh(canvas.itself);

      break;
    }

  }
  
  // LEFT mouse button
  if (b==1) {
    switch(pane.tool) {
    case 0: // rotation
      if (canvas.wiremode) {
	
	dx = x - canvas.t[0];
	dy = y - canvas.t[1];
	canvas.t[0] = x;
	canvas.t[1] = y;

	scene->rotation.x -= dy;
	scene->rotation.y -= dx;

	while(scene->rotation.x < 0.0) scene->rotation.x += 360.0;
	while(scene->rotation.y < 0.0) scene->rotation.y += 360.0;
	while(scene->rotation.x > 360.0) scene->rotation.x -= 360.0;
	while(scene->rotation.y > 360.0) scene->rotation.y -= 360.0;

	gtk_adjustment_set_value(pane.rot.xa, scene->rotation.x);
	gtk_adjustment_set_value(pane.rot.ya, scene->rotation.y);

	refresh(pane.rot.preview);
	refresh(canvas.itself);

      }
      break;
    case 1: // clip
      if (canvas.wiremode) {
	i = canvas.closer / 2;
	AP.x = x - canvas.cc[i][1].x;
	AP.y = y - canvas.cc[i][1].y;
	AP.z = 0;
	VectorCrossProduct(&d, &AP, &(canvas.vfrom));
	ad = VectorLength(&d);

	BP.x = x - canvas.cc[i][2].x;
	BP.y = y - canvas.cc[i][2].y;
	BP.z = 0;
	VectorCrossProduct(&d2, &BP, &(canvas.vfrom));
	ac = VectorLength(&d2);
	ac /= canvas.xysin;

	if (i==0) j=scene->avol->W; else
	  if (i==1) j=scene->avol->H; else
	    j=scene->avol->D;
	ad *= (float) j;
	ad /= canvas.xysin;
	ad /= canvas.fullnorm;
	k = (int) ad;
	if (k > j) k = j; if (k < 0) k = 0;

	if (ac > canvas.fullnorm) k=0;

	switch(canvas.closer) {
	case 0: 
	  scene->clip.v[0][0] = k;
	  if (scene->clip.v[0][1] < k) scene->clip.v[0][1] = k;
	  break;
	case 1:
	  scene->clip.v[0][1] = k;
	  if (scene->clip.v[0][0] > k) scene->clip.v[0][0] = k;
	  break;
	case 2: 
	  scene->clip.v[1][0] = k;
	  if (scene->clip.v[1][1] < k) scene->clip.v[1][1] = k;
	  break;
	case 3:
	  scene->clip.v[1][1] = k;
	  if (scene->clip.v[1][0] > k) scene->clip.v[1][0] = k;
	  break;
	case 4: 
	  scene->clip.v[2][0] = k;
	  if (scene->clip.v[2][1] < k) scene->clip.v[2][1] = k;
	  break;
	case 5:
	  scene->clip.v[2][1] = k;
	  if (scene->clip.v[2][0] > k) scene->clip.v[2][0] = k;
	  break;
	}

	lm[0] = scene->avol->W - 1;
	lm[1] = scene->avol->H - 1;
	lm[2] = scene->avol->D - 1;

	for(i=0;i<3;i++)
	  for(j=0;j<2;j++) {
	    if (scene->clip.v[i][j] > lm[i]) scene->clip.v[i][j] = lm[i];
	    if (scene->clip.v[i][j] < 0) scene->clip.v[i][j] = 0;
	  }
	
	gtk_adjustment_set_value(pane.clip.xa[0], scene->clip.v[0][0]);
	gtk_adjustment_set_value(pane.clip.xa[1], scene->clip.v[0][1]);
	gtk_adjustment_set_value(pane.clip.ya[0], scene->clip.v[1][0]);
	gtk_adjustment_set_value(pane.clip.ya[1], scene->clip.v[1][1]);
	gtk_adjustment_set_value(pane.clip.za[0], scene->clip.v[2][0]);
	gtk_adjustment_set_value(pane.clip.za[1], scene->clip.v[2][1]);

	refresh(canvas.itself);
      }
      break;
    case 4: // add seeds
      if (SceneMarkSeedLineOnProjection(scene, pane.add.lx, pane.add.ly, 
					px, py, 
					picker_get_selection(pane.add.label))) 
	{
	  canvas.fvdirty = 1;
	  ivs_scene_to_pane(M_ADD);
	  refresh(canvas.itself);
	}
      pane.add.lx = px;
      pane.add.ly = py;
      break;
    }
    
  } // b==1

  if (b==3) {
    pane.zinfo.x = px;
    pane.zinfo.y = py;
    pane.zinfo.active = 1;
    refresh(canvas.itself);
  }

  return FALSE;
}

// MOUSE ACTION (PRESS, 2D VIEW)
void ivs_view_press_2D(int x,int y, int b) {
  int px,py,pz,q=-1;

  px = canvas.fvx;
  py = canvas.fvy;
  pz = canvas.fvz;

  if (!scene->avol) return;
  canvas.fvasd[3] = -1;

  // XY image
  if (inbox(x,y,0,0,canvas.SW,canvas.SH)) {
    q = 0;
    px = x - (canvas.fox[0] + canvas.fvtx[0]);
    py = y - (canvas.foy[0] + canvas.fvty[0]);
    px = (int) floor(((double) px) / SceneScale(scene));
    py = (int) floor(((double) py) / SceneScale(scene));
  } else if (inbox(x,y,canvas.SW,0,canvas.SW,canvas.SH)) {
    q = 1;
    pz = x - (canvas.fox[1] + canvas.fvtx[1]);
    py = y - (canvas.foy[1] + canvas.fvty[1]);
    pz = (int) floor(((double) pz) / SceneScale(scene));
    py = (int) floor(((double) py) / SceneScale(scene));
  } else if (inbox(x,y,0,canvas.SH,canvas.SW,canvas.SH)) {
    q = 2;
    px = x - (canvas.fox[2] + canvas.fvtx[2]);
    pz = y - (canvas.foy[2] + canvas.fvty[2]);
    px = (int) floor(((double) px) / SceneScale(scene));
    pz = (int) floor(((double) pz) / SceneScale(scene));
  }

  // ignore clicks out of bounds (except for middle button pan)
  if (b!=2)
    if (px < 0 || px >= scene->avol->W ||
	py < 0 || py >= scene->avol->H ||
	pz < 0 || pz >= scene->avol->D ||
	q < 0)
      return;

  // LEFT mouse button
  if (b==1) {
    switch(pane.tool) {
    case 0: // rot
    case 1: // clip
    case 2: // light
    case 3: // color
    case 6: // 2d view options
    case 7: // overlay
      canvas.fvx = px;
      canvas.fvy = py;
      canvas.fvz = pz;
      canvas.fvdirty = 1;
      ivs_scene_to_pane(M_VIEW2D);
      refresh(canvas.itself);
      break;
    case 4: // add seeds
      if (SceneMarkSeed(scene, px,py,pz, picker_get_selection(pane.add.label)))
	{
	  canvas.fvdirty = 1;
	  ivs_scene_to_pane(M_ADD);
	  refresh(canvas.itself);
	  canvas.fvasd[0] = px;
	  canvas.fvasd[1] = py;
	  canvas.fvasd[2] = pz;
	  canvas.fvasd[3] = q;
	}
      break;
    case 5: // del trees
      if (SceneMarkTree(scene, px, py, pz)) {
	scene->invalid = 1;
	ivs_scene_to_pane(M_DEL);
	refresh(canvas.itself);
      }
      break;
    }
  }

  // pan view with MIDDLE button
  if (b==2) {
    canvas.fvpan[0] = x;
    canvas.fvpan[1] = y;
    canvas.fvpan[2] = 1;
    gtk_grab_add(canvas.itself);
    refresh(canvas.itself);
  }
}

// MOUSE ACTION (PRESS)
gboolean ivs_view_press(GtkWidget *widget, GdkEventButton *eb, gpointer data) 
{
  int x,y,px,py,b,i;
  Vector w,v,r;
  float t;

  x = (int) eb->x; y = (int) eb->y; b = eb->button;
  px = x; py = y;

  if (canvas.fourview) {
    if (!inbox(x,y,canvas.SW,canvas.SH,canvas.SW,canvas.SH)) {
      ivs_view_press_2D(x,y,b);
      return FALSE;
    }
    px -= canvas.SW;
    py -= canvas.SH;
  }

  px += canvas.odelta[0];
  py += canvas.odelta[1];

  // LEFT mouse button
  if (b==1) {
    
    switch(pane.tool) {
    case 0: // rotation
      canvas.t[0] = x;
      canvas.t[1] = y;
      canvas.wiremode = 1;
      gtk_grab_add(canvas.itself);
      refresh(canvas.itself);
      break;
    case 1: // clip
      if (!scene->avol) break;

      canvas.wiremode = 1;
      gtk_grab_add(canvas.itself);

      i = canvas.closer / 2;

      w.x = canvas.cc[i][3].x - canvas.cc[i][0].x;
      w.y = canvas.cc[i][3].y - canvas.cc[i][0].y;
      w.z = 0.0;
      canvas.fullnorm = VectorLength(&w);

      canvas.vfrom.x = canvas.cc[i][0].x - canvas.cc[i][1].x;
      canvas.vfrom.y = canvas.cc[i][0].y - canvas.cc[i][1].y;
      canvas.vfrom.z = 0.0;
      VectorNormalize(&(canvas.vfrom));

      VectorNormalize(&w);
      t = VectorInnerProduct(&w, &(canvas.vfrom));
      canvas.xysin = sqrt(1.0 - t*t);

      VectorAssign(&v, 0.0, 0.0, -1.0);
      VectorCrossProduct(&r, &v, &(canvas.vfrom));
      VectorNormalize(&r);

      refresh(canvas.itself);
      break;

    case 2: // light
      if (!scene->avol) break;

      canvas.tvoxel = SceneVoxelAt(scene, px, py);
      if (canvas.tvoxel == VLARGE)
	canvas.tvoxel = -1;

      refresh(canvas.itself);
      break;
      
    case 4: // add seeds
      if (SceneMarkSeedOnProjection(scene, px, py, 
				    picker_get_selection(pane.add.label))) 
	{
	  canvas.fvdirty = 1;
	  ivs_scene_to_pane(M_ADD);
	  refresh(canvas.itself);
	}
      pane.add.lx = px;
      pane.add.ly = py;
      break;
    case 5: // del trees
      if (SceneMarkTreeOnProjection(scene, px, py)) {
	scene->invalid = 1;
	ivs_scene_to_pane(M_DEL);
	refresh(canvas.itself);
      }
      break;
    }

  } // b==1

  if (b==3) {
    pane.zinfo.active = 1;
    pane.zinfo.x = px;
    pane.zinfo.y = py;
    refresh(canvas.itself);
  }

  return FALSE;
}

// MOUSE ACTION (RELEASE)
gboolean ivs_view_release(GtkWidget *widget,GdkEventButton *eb,gpointer data) 
{
  int x,y,b;
  x = (int) eb->x; y = (int) eb->y; b = eb->button;

  // LEFT mouse button
  if (b==1) {

    switch(pane.tool) {
    case 0: // rotation
      if (canvas.wiremode) {
	canvas.wiremode = 0;
	gtk_grab_remove(canvas.itself);
	refresh(canvas.itself);
      }
      break;
    case 1: // clip
      if (canvas.wiremode) {
	canvas.wiremode = 0;
	gtk_grab_remove(canvas.itself);
	refresh(canvas.itself);
      }
      break;
    }
  }

  if (b==2 && canvas.fvpan[2]) {
    canvas.fvpan[2] = 0;
    gtk_grab_remove(canvas.itself);
    refresh(canvas.itself);
  }

  if (b==3) {
    pane.zinfo.active = 0;
    refresh(canvas.itself);
  }

  return FALSE;
}

gboolean 
ivs_view_expose(GtkWidget *widget,GdkEventExpose *ee,
		gpointer data) {
  int ww,wh;
  
  gdk_window_get_size(widget->window, &ww, &wh);

  if (canvas.vista == 0 || ww != canvas.W || wh != canvas.H) {
    if (canvas.vista) {
      gdk_pixmap_unref(canvas.vista);
      canvas.vista = 0;
    }
    if (canvas.gc) {
      gdk_gc_destroy(canvas.gc);
      canvas.gc = 0;
    }
    canvas.W = ww;
    canvas.H = wh;

    canvas.vista = gdk_pixmap_new(widget->window, canvas.W, canvas.H, -1);
    scene->invalid = 1;
  }
  if (canvas.pfd == NULL) {
    canvas.pfd = pango_font_description_from_string(IVS_CANVAS_FONT);
    if (canvas.pfd == NULL) {
      fprintf(stderr,"** unable to load font %s\n",IVS_CANVAS_FONT);
      canvas.pl = NULL;
    } else {
      canvas.pl = gtk_widget_create_pango_layout(widget, NULL);
    }
  }

  compose_vista();
  
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  canvas.vista,
                  0,0,
                  0,0,
		  canvas.W,canvas.H);

  return FALSE;
}

gboolean  ivs_ball_press(GtkWidget *widget, GdkEventButton *eb, gpointer data) {
  int x,y,N;
  Voxel v;
  Vector l;

  x = eb->x; y = eb->y;

  N = SceneVoxelAt(pane.light.ps, x, y);

  if (N!=VLARGE) {
    XAVolumeVoxelOf(pane.light.ps->avol, N, &v);
    VectorAssign(&l, v.x - 23.0, v.y - 23.0, v.z - 23.0);
    VectorNormalize(&l);
    VectorCopy(&(pane.light.ps->options.phong.light), &l);
    refresh(pane.light.preview);
    ivs_rot(0,0);
  }

  return FALSE;
}

gboolean ivs_ball_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) {
  static GdkGC *gc=0;
  Vector L;
  int x,y,a,b;
  
  if (!gc) gc = gdk_gc_new(widget->window);

  pane.light.ps->colors.bg = triplet( widget->style->bg[0].red >> 8,
                                      widget->style->bg[0].green >> 8,
				      widget->style->bg[0].blue >> 8 );

  SceneCheckValidity(pane.light.ps);
  if (pane.light.ps->invalid) {
    ScenePrepareImage(pane.light.ps, 48, 48);
    CImgFill(pane.light.ps->vout, pane.light.ps->colors.bg);
    RenderScene(pane.light.ps,48,48,0);
  }

  gdk_draw_rgb_image(widget->window,widget->style->black_gc,0,0,
		     48, 48,
		     GDK_RGB_DITHER_NORMAL,
		     pane.light.ps->vout->val,
		     pane.light.ps->vout->rowlen);  

  // draw light
  VectorCopy(&L, &(pane.light.ps->options.phong.light));
  VectorNormalize(&L);
  VectorScalarProduct(&L, 19.50);

  x = 23 + L.x;
  y = 23 + L.y;

  gc_color(gc, 0);
  gdk_draw_arc(widget->window, gc, TRUE, x - 4, y - 4, 8, 8, 0 , 360*64);
  gc_color(gc, 0x00ff00);
  gdk_draw_arc(widget->window, gc, FALSE, x - 4, y - 4, 8, 8, 0 , 360*64);

  VectorScalarProduct(&L, 2.0);
  a = 23 + L.x;
  b = 23 + L.y;

  gdk_draw_line(widget->window, gc, x, y, a, b);

  return FALSE;
}

void ivs_tool(GtkToggleButton *w, gpointer data) {
  int n = (int) data;
  int i,ia;

  ia=gtk_toggle_button_get_active(w);

  if (ia) {
    pane.tool = n;
    for(i=0;i<NTOOLS;i++)
      if (i!=n) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[i]),0);
	gtk_widget_set_style(pane.tools[i], pane.inactive_tool);
	gtk_widget_ensure_style(pane.tools[i]);
	if (pane.tcontrol[i])
	  gtk_widget_hide(pane.tcontrol[i]);
      } else {
	gtk_widget_set_style(pane.tools[i], pane.active_tool);
	gtk_widget_ensure_style(pane.tools[i]);
	if (pane.tcontrol[i])
	  gtk_widget_show(pane.tcontrol[i]);
      }
  } else {
    if (pane.tool == n)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[n]),1);
  }
}

void compose_vista_2D() {
  int i;
  VModeType mode;

  if (!(scene->avol)) {
    for(i=0;i<3;i++)
      canvas.flat[i] = 0;
    return;
  }

  switch(dropbox_get_selection(pane.view2d.mode)) {
  case 0:
    mode = scene->options.vmode;
    break;
  case 1:
    mode = vmode_src;
    break;
  case 2:
    mode = vmode_srclabel;
    break;
  case 3:
    mode = vmode_srcborder;
    break;
  default:
    mode = vmode_src;
  }

  if (mode == vmode_seethru || mode == vmode_srctrees || mode == vmode_paths)
    mode = vmode_srcborder;

  RenderFrames(scene, mode,
	       &(canvas.flat[0]),
	       &(canvas.flat[1]),
	       &(canvas.flat[2]),
	       canvas.fvx, canvas.fvy, canvas.fvz);

  // merge plane images to 3D output
  compose_vista_2D_overlay();
  
}

void compose_vista_2D_overlay() {
  float vtx, vty, vtz, ptx, pty;
  Transform b,c,d;
  Point p,q;
  int i,j,k,rx,ry,ps,hisz;
  int W,H,D,iw,ih;
  int bounds[4];
  char *hitmap;
  float ratio[5] = {0.0, 0.33, 0.44, 0.55, 0.60 };
  float epsilon = 0.1;

  W = scene->avol->W;
  H = scene->avol->H;
  D = scene->avol->D;

  if (canvas.optvout != 0) {
    if (canvas.optvout->W != scene->vout->W ||
	canvas.optvout->H != scene->vout->H) {
      CImgDestroy(canvas.optvout);
      canvas.optvout = 0;
    }
  }
  if (canvas.optvout == 0)
    canvas.optvout = CImgNew(scene->vout->W,scene->vout->H);
  CImgFullCopy(canvas.optvout, scene->vout);

  if (scene->options.vmode == vmode_src)
    return;

  iw = scene->pvt.vinfo->W;
  ih = scene->pvt.vinfo->H;

  // build view transform
    
  vtx = -(scene->avol->W / 2.0);
  vty = -(scene->avol->H / 2.0);
  vtz = -(scene->avol->D / 2.0);
  
  TransformZRot(&b, scene->rotation.z);
  TransformYRot(&c, scene->rotation.y);
  TransformXRot(&d, scene->rotation.x);

  TransformCompose(&c,&b);
  TransformCompose(&d,&c); // d <= (b x c) x d

  ptx = iw / 2;
  pty = ih / 2;

  bounds[0] = bounds[1] = 0;
  bounds[2] = iw-1;
  bounds[3] = ih-1;

  ps = ScenePixel(scene);

  hitmap = (char *) malloc(iw*ih*sizeof(char));
  if (!hitmap) {
    CImgDestroy(canvas.optvout);
    canvas.optvout = 0;
    return;
  }
  MemSet(hitmap, 0, iw*ih);

  // y plane
  for(i=0;i<W;i++)
    for(j=0;j<D;j++) {
      p.x = epsilon + i + vtx;
      p.y = epsilon + canvas.fvy + vty;
      p.z = epsilon + j + vtz;
      PointTransform(&q,&p,&d);
      rx = q.x + ptx;
      ry = q.y + pty;

      if (rx < bounds[0] || rx > bounds[2] ||
	  ry < bounds[1] || ry > bounds[3])
	continue;
      hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
      if (q.z < hisz)
	hitmap[rx+ry*iw] = 1;
    }
  // z plane
  for(i=0;i<W;i++)
    for(j=0;j<H;j++) {
      p.x = epsilon + i + vtx;
      p.y = epsilon + j + vty;
      p.z = epsilon + canvas.fvz + vtz;
      PointTransform(&q,&p,&d);
      rx = q.x + ptx;
      ry = q.y + pty;

      if (rx < bounds[0] || rx > bounds[2] ||
	  ry < bounds[1] || ry > bounds[3])
	continue;
      hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
      if (q.z < hisz)
	hitmap[rx+ry*iw] |= 2;
    }
  // x plane
  for(i=0;i<H;i++)
    for(j=0;j<D;j++) {
      p.x = epsilon + canvas.fvx + vtx;
      p.y = epsilon + i + vty;
      p.z = epsilon + j + vtz;
      PointTransform(&q,&p,&d);
      rx = q.x + ptx;
      ry = q.y + pty;

      if (rx < bounds[0] || rx > bounds[2] ||
	  ry < bounds[1] || ry > bounds[3])
	continue;
      hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
      if (q.z < hisz)
	hitmap[rx+ry*iw] |= 4;
    }
  // intersections
  // x+y
  for(i=0;i<D;i++) {
    p.x = epsilon + canvas.fvx + vtx; p.y = epsilon + canvas.fvy + vty; 
    p.z = epsilon + i + vtz;
    PointTransform(&q,&p,&d); rx = q.x + ptx; ry = q.y + pty;

    if (rx < bounds[0] || rx > bounds[2] || ry < bounds[1] || ry > bounds[3])
      continue;
    hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
    if (q.z < hisz)
	hitmap[rx+ry*iw] |= 8;
  }
  // x+z
  for(i=0;i<H;i++) {
    p.x = epsilon + canvas.fvx + vtx; p.y = epsilon + i + vty; 
    p.z = epsilon + canvas.fvz + vtz;
    PointTransform(&q,&p,&d); rx = q.x + ptx; ry = q.y + pty;

    if (rx < bounds[0] || rx > bounds[2] || ry < bounds[1] || ry > bounds[3])
      continue;
    hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
    if (q.z < hisz)
	hitmap[rx+ry*iw] |= 8;
  }
  // y+z
  for(i=0;i<W;i++) {
    p.x = epsilon + i + vtx; p.y = epsilon + canvas.fvy + vty; 
    p.z = epsilon + canvas.fvz + vtz;
    PointTransform(&q,&p,&d); rx = q.x + ptx; ry = q.y + pty;

    if (rx < bounds[0] || rx > bounds[2] || ry < bounds[1] || ry > bounds[3])
      continue;
    hisz = VolumeGetF32(scene->pvt.xyzbuffer,rx,ry,2);
    if (q.z < hisz)
      hitmap[rx+ry*iw] |= 8;
  }

  k = iw*ih;
  for(i=0;i<k;i++) {
    j = 0;
    if (hitmap[i]&1) ++j;
    if (hitmap[i]&2) ++j;
    if (hitmap[i]&4) ++j;
    if (hitmap[i]&8) j = 4;
    hitmap[i] = j;
  }

  for(j=0;j<ih;j++)
    for(i=0;i<iw;i++) {
      rx = SceneInvX(scene,i);
      ry = SceneInvY(scene,j);
      if (inbox(rx,ry,0,0,iw-1,ih-1))
	CImgSet(canvas.optvout,i,j,
		mergeColorsRGB(CImgGet(canvas.optvout,i,j),0,
			       ratio[(int) hitmap[rx+ry*iw]]));      
    }

  free(hitmap);
}

void draw_vista_2D() {
  int x,y,bx,by,a,b,ch;
  GdkRectangle r;
  
  if (!scene->avol) return;
  if (!canvas.fourview) return;

  ch = superSat(scene->colors.bg);

  x = canvas.SW; y = canvas.SH;


  if (canvas.flat[0]) {
    r.x = 0;
    r.y = 0;
    r.width  = canvas.SW;
    r.height = canvas.SH;
    
    canvas.fox[0] = (canvas.SW / 2) - (canvas.flat[0]->W / 2);
    canvas.foy[0] = (canvas.SH / 2) - (canvas.flat[0]->H / 2);
    bx = canvas.fox[0] + canvas.fvtx[0];
    by = canvas.foy[0] + canvas.fvty[0];
    
    gdk_gc_set_clip_rectangle(canvas.gc, &r);

    if (!canvas.wiremode)
      gdk_draw_rgb_image(canvas.vista,canvas.gc,bx,by,
			 canvas.flat[0]->W, canvas.flat[0]->H,
			 GDK_RGB_DITHER_NORMAL,
			 canvas.flat[0]->val,
			 canvas.flat[0]->rowlen);
    else {
      gc_color(canvas.gc, darker(scene->colors.bg,1));
      gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			 bx,by,canvas.flat[0]->W, canvas.flat[0]->H);
    }

    // border
    gc_color(canvas.gc, scene->colors.fullcube);
    gdk_draw_rectangle(canvas.vista, canvas.gc, FALSE, bx-1,by-1,
		       canvas.flat[0]->W, canvas.flat[0]->H);

    // crosshair
    gc_color(canvas.gc, ch);
    a = bx + (int) (((double) canvas.fvx) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, a, by - 5, 
		  a, by + canvas.flat[0]->H + 5);
    b = by + (int) (((double) canvas.fvy) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, bx - 5, b,
		  bx + canvas.flat[0]->W + 5, b);
  }

  if (canvas.flat[1]) {
    r.x = canvas.SW;
    r.y = 0;
    r.width  = canvas.SW;
    r.height = canvas.SH;

    canvas.fox[1] = canvas.SW + (canvas.SW / 2) - (canvas.flat[1]->W / 2);
    canvas.foy[1] = (canvas.SH / 2) - (canvas.flat[1]->H / 2);
    bx = canvas.fox[1] + canvas.fvtx[1];
    by = canvas.foy[1] + canvas.fvty[1];

    gdk_gc_set_clip_rectangle(canvas.gc, &r);
    if (!canvas.wiremode)
      gdk_draw_rgb_image(canvas.vista,canvas.gc,bx,by,
			 canvas.flat[1]->W, canvas.flat[1]->H,
			 GDK_RGB_DITHER_NORMAL,
			 canvas.flat[1]->val,
			 canvas.flat[1]->rowlen);
    else {
      gc_color(canvas.gc, darker(scene->colors.bg,1));
      gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			 bx,by,canvas.flat[1]->W, canvas.flat[1]->H);
    }

    // border
    gc_color(canvas.gc, scene->colors.fullcube);
    gdk_draw_rectangle(canvas.vista, canvas.gc, FALSE, bx-1,by-1,
		       canvas.flat[1]->W, canvas.flat[1]->H);

    // crosshair
    gc_color(canvas.gc, ch);
    a = bx + (int) (((double) canvas.fvz) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, a, by - 5, 
		  a, by + canvas.flat[1]->H + 5);
    b = by + (int) (((double) canvas.fvy) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, bx - 5, b,
		  bx + canvas.flat[1]->W + 5, b);
  }
  
  if (canvas.flat[2]) {
    r.x = 0;
    r.y = canvas.SH;
    r.width  = canvas.SW;
    r.height = canvas.SH;

    canvas.fox[2] = (canvas.SW / 2) - (canvas.flat[2]->W / 2);
    canvas.foy[2] = canvas.SH + (canvas.SH / 2) - (canvas.flat[2]->H / 2);
    bx = canvas.fox[2] + canvas.fvtx[2];
    by = canvas.foy[2] + canvas.fvty[2];

    gdk_gc_set_clip_rectangle(canvas.gc, &r);
    if (!canvas.wiremode)
      gdk_draw_rgb_image(canvas.vista,canvas.gc,bx,by,
			 canvas.flat[2]->W, canvas.flat[2]->H,
			 GDK_RGB_DITHER_NORMAL,
			 canvas.flat[2]->val,
			 canvas.flat[2]->rowlen);
    else {
      gc_color(canvas.gc, darker(scene->colors.bg,1));
      gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			 bx,by,canvas.flat[2]->W, canvas.flat[2]->H);
    }

    // border
    gc_color(canvas.gc, scene->colors.fullcube);
    gdk_draw_rectangle(canvas.vista, canvas.gc, FALSE, bx-1,by-1,
		       canvas.flat[2]->W, canvas.flat[2]->H);

    // crosshair
    gc_color(canvas.gc, ch);
    a = bx + (int) (((double) canvas.fvx) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, a, by - 5, 
		  a, by + canvas.flat[2]->H + 5);
    b = by + (int) (((double) canvas.fvz) * SceneScale(scene));
    gdk_draw_line(canvas.vista, canvas.gc, bx - 5, b,
		  bx + canvas.flat[2]->W + 5, b);
  }
  
  r.x = 0;
  r.y = 0;
  r.width  = canvas.W;
  r.height = canvas.H;
  gdk_gc_set_clip_rectangle(canvas.gc, &r);
}

void compose_vista() {
  int x,y,si;
  CImg *o;
  int csx,csy;

  if (!canvas.gc) canvas.gc = gdk_gc_new(canvas.vista);
  gc_color(canvas.gc, scene->colors.bg);
  gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE, 0,0, canvas.W, canvas.H);

  if (scene->avol) {

    if (canvas.fourview) {
      canvas.SW = canvas.W / 2;
      canvas.SH = canvas.H / 2;
      x = canvas.SW;
      y = canvas.SH;
    } else {
      canvas.SW = canvas.W;
      canvas.SH = canvas.H;
      x = y = 0;
    }

    if (!canvas.wiremode) {
      SceneCheckValidity(scene);
      si = scene->invalid;
      if (si) {
	canvas.odelta[0] = canvas.odelta[1] = 0;
	csx = canvas.SW;
	csy = canvas.SH;

	/* buffer must be twice its size when 
	   rendering in 50% zoom mode */
	if (scene->options.zoom == zoom_50) {
	  csx *= 2; csy *= 2;
	  canvas.odelta[0] = csx / 4; 
	  canvas.odelta[1] = csy / 4;
	}

	ScenePrepareImage(scene, csx, csy);
	CImgFill(scene->vout, scene->colors.bg);
	RenderScene(scene,csx,csy,tasklog);

	ivs_make_cube();
      }

      if (canvas.fourview)
	if (si || canvas.fvdirty) {
	  compose_vista_2D();
	  canvas.fvdirty = 0;
	}
      o = scene->vout;
      if (!canvas.fourview) {
	if (canvas.optvout) CImgDestroy(canvas.optvout);
	canvas.optvout = 0;
      } else if (canvas.optvout!=0)
	o = canvas.optvout;

      gdk_draw_rgb_image(canvas.vista,canvas.gc,x,y,
			 canvas.SW, canvas.SH,
			 GDK_RGB_DITHER_NORMAL,
			 o->val + 3*canvas.odelta[0] +
			 o->rowlen * canvas.odelta[1],
			 o->rowlen);      
    } else {
      ivs_make_cube();
    }

    if (canvas.fourview) 
      draw_vista_2D();
    ivs_draw_cube();
    
    if (canvas.tvoxel >= 0)
      ivs_draw_tvoxel();

    if (pane.zinfo.active)
      ivs_draw_zinfo();

  }
}

void ivs_check_popup_queue() {
  pthread_mutex_lock(&(pq.ex));

  if (pq.n) {
    pq.n--;
    PopMessageBox(mainwindow, pq.title[pq.n], pq.text[pq.n], pq.icon[pq.n]);
  }

  pthread_mutex_unlock(&(pq.ex));
}

void ivs_queue_popup(char *title, char *text, int icon) {
  pthread_mutex_lock(&(pq.ex));

  if (pq.n<4) {
    strncpy(pq.title[pq.n], title, 32);
    strncpy(pq.text[pq.n], text, 512);
    pq.icon[pq.n] = icon;
    pq.title[pq.n][31] = 0;
    pq.text[pq.n][511] = 0;
    pq.n++;
  }

  pthread_mutex_unlock(&(pq.ex));
}

void ivs_check_status_queue() {
  pthread_mutex_lock(&(sq.ex));

  if (sq.n) {
    sq.n--;
    ivs_set_status(sq.text[sq.n], sq.timeout[sq.n]);
  }

  pthread_mutex_unlock(&(sq.ex));
}

void ivs_queue_status(char *text, int timeout) {
  pthread_mutex_lock(&(sq.ex));

  if (sq.n < 4) {
    strncpy(sq.text[sq.n], text, 256);
    sq.text[sq.n][255] = 0;
    sq.timeout[sq.n] = timeout;
    sq.n++;
  }

  pthread_mutex_unlock(&(sq.ex));
}

void ivs_set_status(char *text, int timeout) {
  if (status.textout > 0)
    gtk_timeout_remove(status.textout);
  gtk_label_set_text(GTK_LABEL(status.text), text);
  refresh(status.text);
  status.textout=gtk_timeout_add(timeout*1000, ivs_status_clear, 0);
}

gboolean  ivs_status_clear(gpointer data) {
  gtk_label_set_text(GTK_LABEL(status.text), " ");
  refresh(status.text);
  status.textout = -1;
  return FALSE;
}

gboolean  ivs_scan(gpointer data) {
  static int last_redraw   = 0;
  static int last_newdata  = 0;
  static int last_ppchange = 0;
  static int last_objmode  = 0;
  static int last_log      = -1;
  int wref=0, wstp=0, wsta=0, wrpp=0, wlog=0;
  int i;

  if (sync_redraw > last_redraw) {
    last_redraw = sync_redraw;
    wref = 1;
  }

  if (sync_newdata> last_newdata) {
    last_newdata = sync_newdata;
    last_redraw  = sync_redraw;
    wref = 1;
    wstp = 1;
    wrpp = 1;
    scene->invalid = 1;
  }

  if (sync_ppchange> last_ppchange) {
    last_ppchange = sync_ppchange;
    wrpp = 1;
    if (scene->options.src == src_value)
      wref = 1;
  }

  if (sync_objmode > last_objmode) {
    last_objmode = sync_objmode;
    if (scene->options.vmode == vmode_src) {
      scene->options.vmode = vmode_srclabel;
      ivs_scene_to_pane(M_MODE);
    }
    wref = 1;
  }

  if (!wstp) {
    if (scene->avol) {
      i = MapSize(scene->avol->addseeds);
      if (i!=pane.add.ln)
	wstp = 1;
      i = MapSize(scene->avol->delseeds);
      if (i!=pane.del.ln)
	wstp = 1;
    }
  }

  if (!wsta) {
    TaskQueueLock(qseg);
    i=GetTaskCount(qseg);
    TaskQueueUnlock(qseg);
    if (i > 0 || i!=status.ln) wsta = 1;
  }

  if (!wlog) {
    TaskLogLock(tasklog);
    i=TaskLogGetCount(tasklog);
    if (i!=last_log) {
      last_log = i;
      wlog = 1;
    }
    TaskLogUnlock(tasklog);
  }
    
  if (wstp) ivs_scene_to_pane(M_ALL^M_REPAINT);
  if (wref) {
    pane.zinfo.svalid = 0;
    refresh(canvas.itself);
  }
  if (wrpp) { 
    preproc.invalid=1; 
    preproc.bounds.omin = -1;
    preproc.bounds.vmin = -1;
    pane.zinfo.svalid = 0;
    preproc.ppspin++;
    refresh(preproc.canvas); 
    preproc_update_ppdialog();
  }

  if (wsta)
    ivs_update_status();

  if (wlog)
    update_tasklog_pane();

  ivs_check_status_queue();
  ivs_check_popup_queue();

  return TRUE;
}

void      ivs_pane_to_scene(int flags) {
  int i;

  if (flags & M_VIEWS) {
    i = canvas.fourview;
    canvas.fourview = dropbox_get_selection(pane.views);
    if (canvas.fourview != i) {
      scene->invalid = 1;
      canvas.fvdirty = 1;
    }
  }

  if (flags & M_ROT) {
    scene->rotation.x = pane.rot.xa->value;
    scene->rotation.y = pane.rot.ya->value;
    scene->rotation.z = pane.rot.za->value;
  }

  if (flags & M_CLIP) {
    scene->clip.v[0][0] = pane.clip.xa[0]->value;
    scene->clip.v[0][1] = pane.clip.xa[1]->value;
    scene->clip.v[1][0] = pane.clip.ya[0]->value;
    scene->clip.v[1][1] = pane.clip.ya[1]->value;
    scene->clip.v[2][0] = pane.clip.za[0]->value;
    scene->clip.v[2][1] = pane.clip.za[1]->value;
  }

  if (flags & M_LIGHT) {
    scene->options.phong.ambient  = pane.light.ka[0]->value;
    scene->options.phong.diffuse  = pane.light.ka[1]->value;
    scene->options.phong.specular = pane.light.ka[2]->value;
    scene->options.phong.zgamma   = pane.light.za[0]->value;
    scene->options.phong.zstretch = pane.light.za[1]->value;
    scene->options.phong.sre      = (int) (pane.light.na->value);

    VectorCopy(&(scene->options.phong.light), &(pane.light.ps->options.phong.light));

    MemCpy(&(pane.light.ps->options.phong), &(scene->options.phong), 
	   sizeof(scene->options.phong));
  }

  if (flags & M_SOLID) {
    for(i=0;i<10;i++)
      scene->hollow[i] = !picker_get_down(pane.solid, i);
  }

  if (flags & M_MODE) {
    scene->enable_mip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pane.overlay.mipenable));
    scene->mip_lighten_only = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pane.overlay.miplighten));

    switch(dropbox_get_selection(pane.overlay.mipsource)) {
    case 0: scene->mip_source = MIPSRC_ORIG;    break;
    case 1: scene->mip_source = MIPSRC_CURR;    break;
    case 2: scene->mip_source = MIPSRC_COST;    break;
    case 3: scene->mip_source = MIPSRC_OVERLAY; break;
    }

    switch(dropbox_get_selection(pane.mode)) {
    case VM_SBOX:  scene->options.vmode = vmode_src;       break;
    case VM_OBJ:   scene->options.vmode = vmode_srclabel;  break;
    case VM_OBOR:  scene->options.vmode = vmode_srcborder; break;
    case VM_TBOR:  scene->options.vmode = vmode_srctrees;  break;
    case VM_TRANS: scene->options.vmode = vmode_seethru;   break;
    case VM_PATHS: scene->options.vmode = vmode_paths;     break;
    }
    
    switch(dropbox_get_selection(pane.zoom)) {
    case 0: scene->options.zoom = zoom_50; break;
    case 1: scene->options.zoom = zoom_100; break;
    case 2: scene->options.zoom = zoom_150; break;
    case 3: scene->options.zoom = zoom_200; break;
    case 4: scene->options.zoom = zoom_300; break;
    case 5: scene->options.zoom = zoom_400; break;
    }
    
    switch(dropbox_get_selection(pane.src)) {
    case 0: scene->options.src = src_orig;  break;
    case 1: scene->options.src = src_value; break;
    case 2: scene->options.src = src_cost;  break;
    }
    
    switch(dropbox_get_selection(pane.shading)) {
    case SH_OP_AUTO:  scene->options.shading = shading_auto;        break;
    case SH_OP_NONE:  scene->options.shading = shading_none;        break;
    case SH_OP_DEPTH: scene->options.shading = shading_quad_z_dist; break;
    case SH_OP_SURFACE: scene->options.shading = shading_phong;     break;
    case SH_OP_GRAD:    scene->options.shading = shading_grad_phong;  break;
    case SH_OP_N2:      scene->options.shading = shading_phong_sq;    break;
    case SH_OP_N3:      scene->options.shading = shading_phong_cube;  break;
    case SH_OP_CARTOON: scene->options.shading = shading_phong_cartoon; break;
    }
  }

  if (flags & M_REPAINT) refresh(canvas.itself);
}

void      ivs_scene_to_pane(int flags) {
  int i, a[4];
  char z[256];

  if (flags & M_VIEW2D) {
    if (scene->avol) {
      i = canvas.fvx + 
	scene->avol->tbrow[canvas.fvy] + scene->avol->tbframe[canvas.fvz];
      sprintf(z,"Cursor X=%d Y=%d Z=%d\norig = %d\nvalue = %d\ncost = %d\nlabel = %d", canvas.fvx,canvas.fvy,canvas.fvz, 
	      scene->avol->vd[i].orig,
	      scene->avol->vd[i].value,
	      scene->avol->vd[i].cost,
	      (scene->avol->vd[i].label & LABELMASK));
    } else {
      strcpy(z," ");
    }
    gtk_label_set_text(GTK_LABEL(pane.view2d.label), z);
    refresh(pane.view2d.label);
  }

  if (flags & M_ROT) {
    gtk_adjustment_set_value(pane.rot.xa, scene->rotation.x);
    gtk_adjustment_set_value(pane.rot.ya, scene->rotation.y);
    gtk_adjustment_set_value(pane.rot.za, scene->rotation.z);
  }

  if (flags & M_CLIP) {
    if (scene->avol) {
      pane.clip.xa[0]->lower = pane.clip.xa[1]->lower = 0;
      pane.clip.xa[0]->upper = pane.clip.xa[1]->upper = scene->avol->W - 1;
      pane.clip.ya[0]->lower = pane.clip.ya[1]->lower = 0;
      pane.clip.ya[0]->upper = pane.clip.ya[1]->upper = scene->avol->H - 1;
      pane.clip.za[0]->lower = pane.clip.za[1]->lower = 0;
      pane.clip.za[0]->upper = pane.clip.za[1]->upper = scene->avol->D - 1;
    }

    gtk_adjustment_set_value(pane.clip.xa[0], scene->clip.v[0][0]);
    gtk_adjustment_set_value(pane.clip.xa[1], scene->clip.v[0][1]);
    gtk_adjustment_set_value(pane.clip.ya[0], scene->clip.v[1][0]);
    gtk_adjustment_set_value(pane.clip.ya[1], scene->clip.v[1][1]);
    gtk_adjustment_set_value(pane.clip.za[0], scene->clip.v[2][0]);
    gtk_adjustment_set_value(pane.clip.za[1], scene->clip.v[2][1]);
  }

  if (flags & M_LIGHT) {
    gtk_adjustment_set_value(pane.light.ka[0], scene->options.phong.ambient);
    gtk_adjustment_set_value(pane.light.ka[1], scene->options.phong.diffuse);
    gtk_adjustment_set_value(pane.light.ka[2], scene->options.phong.specular);
    gtk_adjustment_set_value(pane.light.za[0], scene->options.phong.zgamma);
    gtk_adjustment_set_value(pane.light.za[1], scene->options.phong.zstretch);
    gtk_adjustment_set_value(pane.light.na, scene->options.phong.sre);
    
    VectorCopy(&(pane.light.ps->options.phong.light), &(scene->options.phong.light));
    
    MemCpy(&(scene->options.phong), &(pane.light.ps->options.phong),
	   sizeof(scene->options.phong));
  }

  if (flags & M_SOLID) {
    for(i=0;i<10;i++) {
      picker_set_down(pane.solid, i, !(scene->hollow[i]));
      picker_set_color(pane.solid, i, scene->colors.label[i]);
    }
  }

  if (flags & M_MODE) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.overlay.mipenable), (scene->enable_mip) ? TRUE : FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.overlay.miplighten), (scene->mip_lighten_only) ? TRUE : FALSE);

    switch(scene->mip_source) {
    case MIPSRC_ORIG: dropbox_set_selection(pane.overlay.mipsource, 0); break;
    case MIPSRC_CURR: dropbox_set_selection(pane.overlay.mipsource, 1); break;
    case MIPSRC_COST: dropbox_set_selection(pane.overlay.mipsource, 2); break;
    case MIPSRC_OVERLAY: dropbox_set_selection(pane.overlay.mipsource, 3); break;
    }

    switch(scene->options.vmode) {
    case vmode_src:       a[0]=VM_SBOX;  break;
    case vmode_srclabel:  a[0]=VM_OBJ;   break;
    case vmode_srcborder: a[0]=VM_OBOR;  break;
    case vmode_srctrees:  a[0]=VM_TBOR;  break;
    case vmode_seethru:   a[0]=VM_TRANS; break;
    case vmode_paths:     a[0]=VM_PATHS; break;
    default: a[0]=0;
    }
    
    switch(scene->options.zoom) {
    case zoom_50:  a[1]=0; break;
    case zoom_100: a[1]=1; break;
    case zoom_150: a[1]=2; break;
    case zoom_200: a[1]=3; break;
    case zoom_300: a[1]=4; break;
    case zoom_400: a[1]=5; break;
    default: a[1]=0;
    }
    
    switch(scene->options.src) {
    case src_orig:  a[2]=0; break;
    case src_value: a[2]=1; break;
    case src_cost:  a[2]=2; break;
    default: a[2]=0;
    }

    switch(scene->options.shading) {
    case shading_auto:         a[3]=SH_OP_AUTO; break;
    case shading_none:         a[3]=SH_OP_NONE; break;
    case shading_quad_z_dist: 
    case shading_lin_z_dist:   a[3]=SH_OP_DEPTH; break;
    case shading_normal:       
    case shading_phong:
    case shading_smooth_phong:  a[3]=SH_OP_SURFACE; break;
    case shading_grad_phong:    a[3]=SH_OP_GRAD; break;
    case shading_phong_sq:      a[3]=SH_OP_N2; break;
    case shading_phong_cube:    a[3]=SH_OP_N3; break;
    case shading_phong_cartoon: a[3]=SH_OP_CARTOON; break;
    default: a[3]=0;
    }

    dropbox_set_selection(pane.mode,    a[0]);
    dropbox_set_selection(pane.zoom,    a[1]);
    dropbox_set_selection(pane.src,     a[2]);
    dropbox_set_selection(pane.shading, a[3]);
  }

  if (flags & M_VIEWS) {
    dropbox_set_selection(pane.views, canvas.fourview ? 1 : 0);
  }

  if (flags & M_ADD) {
    if (scene->avol)
      i = MapSize(scene->avol->addseeds);
    else
      i = 0;
    pane.add.ln = i;
    sprintf(z,"%d seed%s",i,i==1?"\0":"s");
    gtk_label_set_text(GTK_LABEL(pane.add.count), z);
  }

  if (flags & M_DEL) {
    if (scene->avol)
      i = MapSize(scene->avol->delseeds);
    else
      i = 0;
    pane.del.ln = i;
    sprintf(z,"%d tree%s",i,i==1?"\0":"s");
    gtk_label_set_text(GTK_LABEL(pane.del.count), z);
  }

  if (flags & M_ADD)   refresh(pane.add.count);
  if (flags & M_DEL)   refresh(pane.del.count);
  if (flags & M_ROT)   refresh(pane.rot.preview);
  if (flags & M_LIGHT) refresh(pane.light.preview);

  if (flags & M_REPAINT) refresh(pane.itself);
}

void ivs_segment(GtkWidget *w, gpointer data) {
  char z[256];
  int i;
  ArgBlock *args;

  i = dropbox_get_selection(pane.seg.method);
  
  switch(i) {
  case SEG_OP_WATERSHED:          strcpy(z,"DIFT/watershed"); break;
  case SEG_OP_IFT_PLUS_WATERSHED: strcpy(z,"IFT+ watershed"); break;
  case SEG_OP_LAZYSHED_L1:        strcpy(z,"DIFT/lasyshed-l1"); break;
  case SEG_OP_DISCSHED_1:         strcpy(z,"DIFT/discshed(4-2-1)"); break;
  case SEG_OP_DISCSHED_2:         strcpy(z,"DIFT/kappa"); break;
  case SEG_OP_FUZZY: 
    fuzzyvariant = 0;
    if (!FirstFuzzyDone)
      ivs_get_fuzzy_pars(1);
    else
      ivs_do_fuzzy();
    return;
  case SEG_OP_STRICT_FUZZY:
    fuzzyvariant = 1;
    if (!FirstFuzzyDone)
      ivs_get_fuzzy_pars(1);
    else
      ivs_do_fuzzy();
    return;
  case SEG_OP_DMT:
    strcpy(z,"DIFT/DMT"); break;

  }

  args = ArgBlockNew();
  args->scene  = scene;
  args->ack    = &sync_objmode;
  args->ia[0]  = i;

  TaskNew(qseg, z, (Callback) &be_segment, (void *) args);
}

void ivs_load_convert_minc(char *path) {
  char z[256],rname[256];
  int nlen;
  ArgBlock *args;

  strcpy(filename, path);

  if (scene->avol) {
    XAVolDestroy(scene->avol);
    scene->avol = 0;
  }

  nlen=strlen(filename);
  if (nlen < 14) {
    strcpy(rname, filename);    
  } else {
    sprintf(rname,"...%s",&filename[nlen-14]);
  }
  
  sprintf(z,"Load MINC - %s",rname);

  args = ArgBlockNew();
  strcpy(args->sa, filename);
  args->scene = scene;
  args->ack   = &sync_newdata;

  TaskNew(qseg,z,(Callback) &be_load_minc, args);
}

void ivs_load_new(char *path) {
  int nlen;
  char z[256],rname[256];
  ArgBlock *args;

  strcpy(filename, path);

  if (scene->avol) {
    XAVolDestroy(scene->avol);
    scene->avol = 0;
  }

  nlen=strlen(filename);
  if (nlen < 14) {
    strcpy(rname, filename);    
  } else {
    sprintf(rname,"...%s",&filename[nlen-14]);
  }

  sprintf(z,"Load Volume - %s",rname);

  args = ArgBlockNew();
  strcpy(args->sa, filename);
  args->scene = scene;
  args->ack   = &sync_newdata;

  TaskNew(qseg,z,(Callback) &be_load_volume, args);
}

int main(int argc, char **argv) {
  int i, hasvol=0;

  gtk_disable_setlocale();
  setlocale(LC_ALL,"en_US.UTF-8");

  gtk_init(&argc, &argv);
  gdk_rgb_init();
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());

  MemSet(&canvas, 0, sizeof(canvas));
  canvas.keysel = -1;
  canvas.tvoxel = -1;
  canvas.fourview = 0;
  canvas.optvout = 0;
  canvas.fvpan[2] = 0;

  for(i=0;i<3;i++) {
    canvas.flat[i] = 0;
    canvas.fvr[i] = canvas.fvtx[i] = canvas.fvty[i] = 0;
  }
  canvas.fvx = canvas.fvy = canvas.fvz = 0;

  filename[0] = 0;
  scene = SceneNew();

  SceneLoadColorScheme(scene, SCHEME_BONEFLESH);

  tasklog  = TaskLogNew();
  qseg     = TaskQueueNew();
  TaskQueueSetLog(qseg, tasklog);
  StartThread(qseg);

  pthread_mutex_init(&(pq.ex), NULL);
  pq.n = 0;

  pthread_mutex_init(&(sq.ex), NULL);
  sq.n = 0;

  ivs_create_gui();

  for(i=1;i<argc;i++)
    if (argv[i][0]!='-') {
      ivs_load_new(argv[i]);
      hasvol=1;
      break;
    }

  gtk_notebook_set_page(GTK_NOTEBOOK(notebook), hasvol?2:0);

  gtk_main();

  return 0;
}

// menu callbacks

void ivs_file_reset_annotation(CBARG) {
  ArgBlock *args;

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  args = ArgBlockNew();
  args->scene = scene;
  args->ack   = &sync_redraw;
  TaskNew(qseg, "Clear Annotation", (Callback) &be_clear_annotation, (void *) args);
  FirstFuzzyDone = 0;
}

void ivs_file_load_labels(CBARG) {
  char name[512], z[256];
  ArgBlock *args;

  if (ivs_get_filename(mainwindow, 
		       "Load labels... (scn)", 
		       name,
		       GF_ENSURE_READABLE)) {

    sprintf(z,"Load Labels - %s",ivs_short_filename(name));

    args = ArgBlockNew();
    strncpy(args->sa, name, 512);
    args->sa[511] = 0;
    args->scene = scene;
    args->ack   = &sync_newdata;
    
    TaskNew(qseg,z,(Callback) &be_load_labels, args);
  }
}

void ivs_file_load_scene(CBARG) {
  char name[512], z[256];
  ArgBlock *args;

  if (ivs_get_filename(mainwindow, 
		       "Load new volume... (mvf/vla/scn/minc/hdr/pgm)", 
		       name,
		       GF_ENSURE_READABLE)) {

    sprintf(z,"Load Volume - %s",ivs_short_filename(name));

    args = ArgBlockNew();
    strncpy(args->sa, name, 512);
    args->sa[511] = 0;
    args->scene = scene;
    args->ack   = &sync_newdata;
    
    TaskNew(qseg,z,(Callback) &be_load_volume, args);
  }
}

void ivs_file_save_scene(CBARG) {
  char name[512], z[128];
  ArgBlock *args;
  if (!scene->avol) {
    ivs_warn_no_volume();
  } else {
    if (ivs_get_filename(mainwindow, 
			 "Save scene to... (.mvf)", 
			 name,
			 GF_ENSURE_WRITEABLE))
      {
	snprintf(z, 127, "Save scene state to %s", name);
	z[127]=0;
	args = ArgBlockNew();
	args->scene = scene;
	strcpy(args->sa, name);
	TaskNew(qseg, z, (Callback) &be_save_state, (void *) args);
      }
  }
}

void ivs_file_export_view(CBARG) {
  char name[512];
  if (!scene->avol) {
    ivs_warn_no_volume();
  } else {
    if (ivs_get_filename(mainwindow, 
			 "Export View to... (.ppm)", 
			 name,
			 GF_ENSURE_WRITEABLE))
      {
	CImgWriteP6(scene->vout, name);
      }
  }
}

void ivs_file_save_log(CBARG) {
  char name[512];
  if (ivs_get_filename(mainwindow,
		       "Save Task Log to... (.txt)", 
		       name, 
		       GF_ENSURE_WRITEABLE)) 
    {
      TaskLogSave(tasklog, name);
    }
}

void ivs_file_quit(CBARG) {
  gtk_main_quit();
}

void ivs_seed_save(CBARG) {
  char name[512],z[128];
  ArgBlock *args;

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  if (ivs_get_filename(mainwindow,
		       "Save Roots to... (.txt)",
		       name,
		       GF_ENSURE_WRITEABLE)) {

    args=ArgBlockNew();
    args->scene = scene;
    strncpy(args->sa, name, 511);
    args->sa[511] = 0;
    strcpy(z,"Save forest roots to file.");
    TaskNew(qseg, z, (Callback) &be_save_roots, (void *) args);    
  }
}

void ivs_seed_load(CBARG) {
  char name[512], z[64];
  ArgBlock *args;

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  if (ivs_get_filename(mainwindow,
		       "Load Seeds from... (.txt)",
		       name,
		       GF_ENSURE_READABLE)) {
    args=ArgBlockNew();
    args->scene = scene;
    strncpy(args->sa, name, 511);
    args->sa[511] = 0;
    args->ack = &sync_newdata;
    strcpy(z,"Load seeds from file.");
    TaskNew(qseg, z, (Callback) &be_load_seeds, (void *) args);
  }
}

// ---------- export scene data

struct _export_data_dialog {
  GtkWidget *dlg;
  GtkWidget *what[4];
  GtkWidget *scnheader;

} ed_dialog;

void ivs_ed_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(ed_dialog.dlg);
  ed_dialog.dlg = 0;
}

void ivs_ed_ok(GtkWidget *widget, gpointer data) {
  int i,w,h;
  char name[512], z[512];
  ArgBlock *args;

  w=0;
  for(i=0;i<4;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ed_dialog.what[i])))
      w=i;
  h=0;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ed_dialog.scnheader)))
    h=1;

  ivs_ed_cancel(0,0); // destroy dialog

  if (ivs_get_filename(mainwindow,
		       "Save data to... (.scn/.raw)",
		       name,
		       GF_ENSURE_WRITEABLE)) {
    args = ArgBlockNew();
    args->scene      = scene;
    args->ia[0]      = w;
    args->ia[1]      = h;
    strncpy(args->sa,name,511);
    args->sa[511]=0;

    sprintf(z,"Export data to %s", ivs_short_filename(name));
    TaskNew(qseg, z, (Callback) &be_export_data, (void *) args);
  }

}

void ivs_file_export_data(CBARG) {
  GtkWidget *v, *f1, *v1, *hs, *hb, *ok, *cancel;
  GSList *l;
  int i;

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  ed_dialog.dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ed_dialog.dlg), "Export Scene Data...");
  gtk_window_set_modal(GTK_WINDOW(ed_dialog.dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(ed_dialog.dlg), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(ed_dialog.dlg),6);

  v=gtk_vbox_new(FALSE,4);

  gtk_container_add(GTK_CONTAINER(ed_dialog.dlg), v);
  
  f1 = gtk_frame_new("Data to export");
  gtk_box_pack_start(GTK_BOX(v), f1, FALSE, TRUE, 4);
  v1 = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(f1), v1);

  ed_dialog.what[0] = gtk_radio_button_new_with_label(0,"Label (8-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(ed_dialog.what[0]));
  ed_dialog.what[1] = gtk_radio_button_new_with_label(l,"Original volume data (16-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(ed_dialog.what[1]));
  ed_dialog.what[2] = gtk_radio_button_new_with_label(l,"Preprocessed volume data (16-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(ed_dialog.what[2]));
  ed_dialog.what[3] = gtk_radio_button_new_with_label(l,"Path costs (16-bit)");

  for(i=0;i<4;i++) {
    gtk_box_pack_start(GTK_BOX(v1), ed_dialog.what[i], FALSE, TRUE, 2);
    gtk_widget_show(ed_dialog.what[i]);
  }

  ed_dialog.scnheader = gtk_check_button_new_with_label("Write SCN header");

  gtk_box_pack_start(GTK_BOX(v), ed_dialog.scnheader, FALSE, TRUE, 2);
  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  hb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
  gtk_box_pack_start(GTK_BOX(v),hb,FALSE,TRUE,2);

  ok     = gtk_button_new_with_label("OK");
  cancel = gtk_button_new_with_label("Cancel");

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hb), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb), cancel, TRUE, TRUE, 0);

  gtk_widget_show(ok);
  gtk_widget_show(cancel);
  gtk_widget_show(hb);
  gtk_widget_show(hs);
  gtk_widget_show(ed_dialog.scnheader);
  gtk_widget_show(v1);
  gtk_widget_show(f1);
  gtk_widget_show(v);
  gtk_widget_show(ed_dialog.dlg);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ed_dialog.what[0]), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ed_dialog.scnheader), TRUE);

  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(ivs_ed_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(ivs_ed_cancel), 0);
}

// export masked data

struct _export_mask_data_dialog {
  GtkWidget *dlg;
  GtkWidget *what[4];
  GtkWidget *scnheader;
  GtkWidget *mask[10];
} edx_dialog;

void ivs_edx_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(edx_dialog.dlg);
  edx_dialog.dlg = 0;
}

void ivs_edx_ok(GtkWidget *widget, gpointer data) {
  int i,w,h;
  char name[512], z[512];
  ArgBlock *args;
  int mask[10];

  w=0;
  for(i=0;i<4;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edx_dialog.what[i])))
      w=i;
  h=0;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edx_dialog.scnheader)))
    h=1;

  for(i=0;i<10;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edx_dialog.mask[i])))
      mask[i] = 1;
    else
      mask[i] = 0;

  ivs_edx_cancel(0,0); // destroy dialog

  if (ivs_get_filename(mainwindow,
		       "Save data to... (.scn/.raw)",
		       name,
		       GF_ENSURE_WRITEABLE)) {
    args = ArgBlockNew();
    args->scene      = scene;
    args->ia[0]      = w;
    args->ia[1]      = h;

    for(i=0;i<10;i++)
      args->ia[10+i] = mask[i];

    strncpy(args->sa,name,511);
    args->sa[511]=0;

    sprintf(z,"Export Masked data to %s", ivs_short_filename(name));
    TaskNew(qseg, z, (Callback) &be_export_masked_data, (void *) args);
  }

}

void ivs_file_export_masked_data(CBARG) {
  GtkWidget *v, *f1, *v1, *hs, *hb, *ok, *cancel;
  GtkWidget *f2, *v2, *h, *L;
  ColorTag *tag;
  GSList *l;
  int i;
  char z[64];

  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  }

  edx_dialog.dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(edx_dialog.dlg), "Export Masked Scene Data...");
  gtk_window_set_modal(GTK_WINDOW(edx_dialog.dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(edx_dialog.dlg), 
			       GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(edx_dialog.dlg),6);

  v=gtk_vbox_new(FALSE,4);

  gtk_container_add(GTK_CONTAINER(edx_dialog.dlg), v);
  
  f1 = gtk_frame_new("Data to export");
  gtk_box_pack_start(GTK_BOX(v), f1, FALSE, TRUE, 4);
  v1 = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(f1), v1);

  edx_dialog.what[0] = gtk_radio_button_new_with_label(0,"Label (8-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(edx_dialog.what[0]));
  edx_dialog.what[1] = gtk_radio_button_new_with_label(l,"Original volume data (16-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(edx_dialog.what[1]));
  edx_dialog.what[2] = gtk_radio_button_new_with_label(l,"Preprocessed volume data (16-bit)");
  l = gtk_radio_button_group(GTK_RADIO_BUTTON(edx_dialog.what[2]));
  edx_dialog.what[3] = gtk_radio_button_new_with_label(l,"Path costs (16-bit)");

  for(i=0;i<4;i++) {
    gtk_box_pack_start(GTK_BOX(v1), edx_dialog.what[i], FALSE, TRUE, 2);
    gtk_widget_show(edx_dialog.what[i]);
  }

  edx_dialog.scnheader = gtk_check_button_new_with_label("Write SCN header");

  gtk_box_pack_start(GTK_BOX(v), edx_dialog.scnheader, FALSE, TRUE, 2);

  // ------------

  f2 = gtk_frame_new("Objects to Include:");
  gtk_box_pack_start(GTK_BOX(v), f2, FALSE, TRUE, 4);

  v2 = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(f2), v2);

  for(i=0;i<10;i++) {
    tag = colortag_new(scene->colors.label[i], 24, 16);

    if (i==0)
      strcpy(z,"Background");
    else      
      sprintf(z,"Object #%d", i);

    edx_dialog.mask[i] = gtk_check_button_new();

    h = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(edx_dialog.mask[i]), h);
    
    L = gtk_label_new(z);
    
    gtk_box_pack_start(GTK_BOX(h), tag->widget, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(h), L, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(v2), edx_dialog.mask[i], FALSE, TRUE, 0);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edx_dialog.mask[i]),
				 i==0?FALSE:TRUE);
    gtk_widget_show(edx_dialog.mask[i]);
    gtk_widget_show(L);
    gtk_widget_show(h);
  }


  // ------------

  hs = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 2);

  hb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
  gtk_box_pack_start(GTK_BOX(v),hb,FALSE,TRUE,2);

  ok     = gtk_button_new_with_label("OK");
  cancel = gtk_button_new_with_label("Cancel");

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hb), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hb), cancel, TRUE, TRUE, 0);

  gtk_widget_show(f2);
  gtk_widget_show(v2);

  gtk_widget_show(ok);
  gtk_widget_show(cancel);
  gtk_widget_show(hb);
  gtk_widget_show(hs);
  gtk_widget_show(edx_dialog.scnheader);
  gtk_widget_show(v1);
  gtk_widget_show(f1);
  gtk_widget_show(v);
  gtk_widget_show(edx_dialog.dlg);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edx_dialog.what[0]), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edx_dialog.scnheader), TRUE);

  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(ivs_edx_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(ivs_edx_cancel), 0);
}

// keyboard shortcuts

/*

  TOP LEVEL:

      F5    = file browser pane
      F6    = preprocessing pane
      F7    = scene pane
      F8    = log pane

  SCENE PANE ONLY:
  
     arrows  = rotation
     Home    = reset rotation
     -,=     = -/+ zoom

     X,Y,Z   = select/unselect clip axis
     W,Q,O,P = adjust clip axis
     End     = reset clip

     R,C     = rotate/clip tools
     Ins     = add seeds
     Del     = del trees
     [,]     = current label

     F9      = solid mode
     F10     = object mode
     F11     = border mode
*/

#define PG_FB    0
#define PG_PP    1
#define PG_SCENE 2
#define PG_LOG   3

gboolean  ivs_key (GtkWidget * w, GdkEventKey * evt, gpointer data) {
  guint k;
  int i,vmax[3], page, GotShift=0;

  if (evt->state&GDK_MOD1_MASK || evt->state&GDK_CONTROL_MASK)
    return TRUE;

  k = evt->keyval;
  if (evt->state&GDK_SHIFT_MASK) GotShift=1;

  switch(k) {
  case GDK_F5: gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 0);
    goto caught_key;
  case GDK_F6: gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 1);
    goto caught_key;
  case GDK_F7: gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 2);
    goto caught_key;
  case GDK_F8: gtk_notebook_set_page(GTK_NOTEBOOK(notebook), 3);
    goto caught_key;
  }

  page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));

  if (page==PG_SCENE) {

    switch(k) {
      /* rotation */
    case GDK_Left: 
      scene->rotation.y -= GotShift ? 1.0 : 10.0;
      while(scene->rotation.y < 0.0) scene->rotation.y += 360.0;
      ivs_scene_to_pane(M_ROT);
      sync_redraw++;
      goto caught_key;
    case GDK_Right: 
      scene->rotation.y += GotShift ? 1.0 : 10.0;
      while(scene->rotation.y > 360.0) scene->rotation.y -= 360.0;
      ivs_scene_to_pane(M_ROT);
      sync_redraw++;
      goto caught_key;
    case GDK_Up: 
      scene->rotation.x -= GotShift ? 1.0 : 10.0;
      while(scene->rotation.x < 0.0) scene->rotation.x += 360.0;
      ivs_scene_to_pane(M_ROT);
      sync_redraw++;
      goto caught_key;
    case GDK_Down: 
      scene->rotation.x += GotShift ? 1.0 : 10.0;
      while(scene->rotation.x > 360.0) scene->rotation.x -= 360.0;
      ivs_scene_to_pane(M_ROT);
      sync_redraw++;
      goto caught_key;
    case GDK_Home:
      scene->rotation.x = 0.0;
      scene->rotation.y = 0.0;
      scene->rotation.z = 0.0;
      ivs_scene_to_pane(M_ROT);
      sync_redraw++;
      goto caught_key;

      /* zoom */
    case GDK_minus:
      if (scene->options.zoom != zoom_50) {
	scene->options.zoom--;
	ivs_scene_to_pane(M_MODE);
	sync_redraw++;
      }
      goto caught_key;
    case GDK_equal:
      if (scene->options.zoom != zoom_400) {
	scene->options.zoom++;
	ivs_scene_to_pane(M_MODE);
	sync_redraw++;
      }
      goto caught_key;
    

      /* clipping */
    case GDK_X: case GDK_x:
      if (canvas.keysel == 0) canvas.keysel=-1; else canvas.keysel=0;
      sync_redraw++;
      goto caught_key;
    case GDK_Y: case GDK_y:
      if (canvas.keysel == 1) canvas.keysel=-1; else canvas.keysel=1;
      sync_redraw++;
      goto caught_key;
    case GDK_Z: case GDK_z:
      if (canvas.keysel == 2) canvas.keysel=-1; else canvas.keysel=2;
      sync_redraw++;
      goto caught_key;
    case GDK_Q: case GDK_q:
      if (canvas.keysel>=0) {
	scene->clip.v[canvas.keysel][0] -= GotShift ? 1 : 10;
	if (scene->clip.v[canvas.keysel][0] < 0)
	  scene->clip.v[canvas.keysel][0] = 0;
	ivs_scene_to_pane(M_CLIP);
	sync_redraw++;
      }
      goto caught_key;
    case GDK_W: case GDK_w:
      if (scene->avol!=0 && canvas.keysel>=0) {
	scene->clip.v[canvas.keysel][0] += GotShift ? 1 : 10;
	vmax[0] = scene->avol->W-1; vmax[1] = scene->avol->H-1;
	vmax[2] = scene->avol->D-1;
	if (scene->clip.v[canvas.keysel][0] > vmax[canvas.keysel])
	  scene->clip.v[canvas.keysel][0] = vmax[canvas.keysel];
	if (scene->clip.v[canvas.keysel][0] > scene->clip.v[canvas.keysel][1])
	  scene->clip.v[canvas.keysel][1] = scene->clip.v[canvas.keysel][0];
	ivs_scene_to_pane(M_CLIP);
	sync_redraw++;
      }
      goto caught_key;
    case GDK_O: case GDK_o:
      if (canvas.keysel>=0) {
	scene->clip.v[canvas.keysel][1] -= GotShift ? 1 : 10;
	if (scene->clip.v[canvas.keysel][1] < 0)
	  scene->clip.v[canvas.keysel][1] = 0;
	if (scene->clip.v[canvas.keysel][1] < scene->clip.v[canvas.keysel][0])
	  scene->clip.v[canvas.keysel][0] = scene->clip.v[canvas.keysel][1];
	ivs_scene_to_pane(M_CLIP);
	sync_redraw++;
      }
      goto caught_key;
    case GDK_P: case GDK_p:
      if (scene->avol!=0 && canvas.keysel>=0) {
	scene->clip.v[canvas.keysel][1] += GotShift ? 1 : 10;
	vmax[0] = scene->avol->W-1; vmax[1] = scene->avol->H-1;
	vmax[2] = scene->avol->D-1;
	if (scene->clip.v[canvas.keysel][1] > vmax[canvas.keysel])
	  scene->clip.v[canvas.keysel][1] = vmax[canvas.keysel];
	if (scene->clip.v[canvas.keysel][1] < scene->clip.v[canvas.keysel][0])
	  scene->clip.v[canvas.keysel][0] = scene->clip.v[canvas.keysel][1];
	ivs_scene_to_pane(M_CLIP);
	sync_redraw++;
      }
      goto caught_key;
    case GDK_End:
      if (scene->avol!=0) {
	scene->clip.v[0][0] = scene->clip.v[1][0] = scene->clip.v[2][0] = 0;
	scene->clip.v[0][1] = scene->avol->W-1;
	scene->clip.v[1][1] = scene->avol->H-1;
	scene->clip.v[2][1] = scene->avol->D-1;
	ivs_scene_to_pane(M_CLIP);
	sync_redraw++;
      }
      goto caught_key;

      /* tool bar */
    case GDK_R: case GDK_r:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[0]),TRUE);
      goto caught_key;
    case GDK_C: case GDK_c:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[1]),TRUE);
      goto caught_key;
    case GDK_Insert:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[4]),TRUE);
      goto caught_key;
    case GDK_Delete:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pane.tools[5]),TRUE);
      goto caught_key;
    case GDK_bracketleft:
      i=picker_get_selection(pane.add.label)-1;
      if (i<0) i+=10;
      picker_set_down(pane.add.label,i,1);
      goto caught_key;
    case GDK_bracketright:
      i=picker_get_selection(pane.add.label)+1;
      if (i>=10) i-=10;
      picker_set_down(pane.add.label,i,1);
      goto caught_key;
      
      /* view mode */
    case GDK_F9:
      scene->options.vmode = vmode_src;
      ivs_scene_to_pane(M_MODE);
      ++sync_redraw;
      goto caught_key;
    case GDK_F10:
      scene->options.vmode = vmode_srclabel;
      ivs_scene_to_pane(M_MODE);
      ++sync_redraw;
      goto caught_key;
    case GDK_F11:
      scene->options.vmode = vmode_srcborder;
      ivs_scene_to_pane(M_MODE);
      ++sync_redraw;
      goto caught_key;

    } // switch k
  } // page=SCENE

  return FALSE;

 caught_key:
  gtk_signal_emit_stop_by_name(GTK_OBJECT(w), "key_press_event");
  return TRUE;
}

void ivs_go_to_pane(int i) {
  gtk_notebook_set_page(GTK_NOTEBOOK(notebook), i); 
}

// About Dialog

#if AUDIENCE==2

static char *lic_web = "For educational and research use only. COMMERCIAL\n"\
                       "REDISTRIBUTION AND MEDICAL USAGE OF THIS\n"\
                       "SOFTWARE ARE STRICTLY FORBIDDEN.\n\n"\
         "THIS SOFTWARE COMES WITH NO WARRANTY. The authors\n"\
         "are not liable to you for any damages arising from\n"\
         "its usage or mis-usage.\n\n"\
         "This version of IVS is available for free download at the website\n"\
         "listed above, and can be copied and redistributed for free.\n"\
         "A quick usage guide is also available from the website.";

#else

static char *lic_unicamp = "RESTRICTED FOR RESEARCH AND EDUCATIONAL USAGE\n"\
	                   "IN UNICAMP FACILITIES.\n"\
	 "Terms of use: this release of IVS is available for research\n"\
	 "and educational usage inside Unicamp only. The authors grant\n"\
	 "permission to copy and install this software in computers\n"\
	 "inside the campus. Users are not authorized to copy, sell, trade\n"\
	 "or redistribute this software to/with third parties. The source\n"\
	 "code, if made available, is for the convenience of compiling on\n"\
	 "multiple platforms only. Derivative works and redistributions of\n"\
         "the source are not allowed at all.";

#endif

void ivs_help_about(CBARG) {
  char z[1024];

  sprintf(z,"IVS version %s\nEdition/Distribution: %s\n\n",
	  VERSION, IVS_TAG);

  strcat(z,
	 "Interactive Volume Segmentation\n\n"\
	 "(C) 2002-2010 Felipe P.G. Bergo and Alexandre X. Falcao\n"\
	 "Developed by Felipe P.G. Bergo and Alexandre X. Falcao\n"\
	 "at the Institute of Computing, Unicamp, Campinas, Brazil.\n\n"\
	 "Website: http://www.liv.ic.unicamp.br/~bergo/ivs\n"\
         "Contact: fbergo@gmail.com or afalcao@ic.unicamp.br \n\n");

  strcat(z, TERMS_OF_USE);

  plain_dialog_with_icon(GTK_WINDOW(mainwindow),"About IVS",z,"OK",
			 banner1_xpm);
}

void ivs_file_info(CBARG) {
  char z[1024];
  float v1,v2;
  if (!scene->avol) {
    ivs_warn_no_volume();
    return;
  } else {
    v1 = scene->avol->dx * scene->avol->dy * scene->avol->dz;
    v2 = ((float)(scene->avol->N)) * v1;
    sprintf(z,
	    "Volume size: %d x %d x %d (%d voxels)\n"\
	    "Voxel size: %.2f x %.2f x %.2f units\n"\
	    "Scene volume: %.4f units^3\n"\
	    "Volume of one voxel: %.4f units^3",
	    scene->avol->W,
	    scene->avol->H,
	    scene->avol->D,
	    scene->avol->N,
	    scene->avol->dx,
	    scene->avol->dy,
	    scene->avol->dz,
	    v2, v1);
  }
  plain_dialog(GTK_WINDOW(mainwindow),"Scene Information",z,"OK");
}

void ivs_keyhelp(CBARG) {
  char z[2048];

  sprintf(z,
	  "Keyboard (anywhere):\n"\
	  "   F5,F6,F7,F8 change the current tabbed pane (file browser,\n"\
	  "               preprocessing, scene and log).\n\n"\
	  "Keyboard (scene pane only):\n"\
	  "   R - choose rotation tool\n"\
	  "   C - choose clip tool\n"\
	  "   Insert - choose Add Seeds tool\n"\
	  "   Delete - choose Remove Trees tool\n"\
	  "   Home   - reset rotation\n"\
	  "   End    - reset clipping\n"\
	  "   Arrows - rotate (Shift+Arrows for fine rotation)\n"\
	  "   -,= (minus, equal) - change zoom\n"\
	  "   X,Y,Z  - select/unselect clipping axis (highlighted in red)\n"\
	  "   W,Q,O,P - move selected (red) clipping axis limits. (Shift+{W,Q,O,P} for fine control)\n"\
	  "   [ ] (brackets) - move current label selection (add seeds tool)\n"\
	  "   F9,F10,F11  - change rendering mode (solid box, objects and object borders, respectively)\n\n"\
	  "Mouse (scene pane):\n"\
	  "   Left button: depends on selected tool.\n"\
	  "   Right button: inspect data.\n"\
	  "\n"\
	  "Mouse (preprocessing pane):\n"\
	  "   Left button: move crosshair selection.\n"\
	  "   Middle button: drag to pan viewing area.\n"\
	  "   Right button: toggle original/current data display.\n"\
	  "\n"\
	  "Mouse (file browser pane):\n"\
	  "   Left button: change directory or open volume/image file.");

  plain_dialog(GTK_WINDOW(mainwindow),"Keyboard/Mouse Input Help",z,"OK");
}

void ivs_formathelp(CBARG) {
  char z[2048];

  sprintf(z,
	  "Input Formats:\n"\
	  "  MVF - IVS's own scene format, which saves the whole session.\n"\
	  "  VLA - deprecated format used by previous beta versions of IVS.\n"\
	  "  SCN - Simple volume format used internally at IC-Unicamp.\n"\
	  "  Analyze - Analyze 7.5's .hdr+.img format.\n"\
	  "            Select the .hdr file to load the volume.\n"\
          "  MINC - Import supported through mincextract and mincinfo.\n"\
	  "  PGM (P2/P5) - 2D grayscale image formats.\n"\
	  "  Seeds - text files saved by Seeds -> Save Roots...\n\n"\
	  "Output Formats:\n"\
	  "  MVF - Saves the whole session, File -> Save Scene...\n"\
	  "  SCN - File -> Export Scene Data...\n"\
	  "  RAW data dump - File -> Export Scene Data...\n"\
	  "  PPM - (P6) 2D color image of current view, File -> Export Scene View...\n"\
	  "  Roots - text files with current influence zone roots.\n"\
	  "  Log   - text file with operation log, File -> Save Task Log..."
	  );
  
  plain_dialog(GTK_WINDOW(mainwindow),"File Formats Help",z,"OK");
}

void ivs_vsize(CBARG) {
  if (scene->avol)
    ivs_voxel_size_edit();
  else
    ivs_warn_no_volume();
}

void ivs_volumetry(CBARG) {
  if (scene->avol)
    ivs_volumetry_open();
  else
    ivs_warn_no_volume();
}

void ivs_cthres(CBARG) {
  if (scene->avol)
    ivs_ct_open();
  else
    ivs_warn_no_volume();
}

void ivs_onion(CBARG) {
  if (scene->avol)
    ivs_onion_open();
  else
    ivs_warn_no_volume();
}

void ivs_draw_zinfo() {
  int i,j,k;
  int n[7][7];
  int x[4],y[4];
  char z[64];
  Voxel v;

  for(i=-3;i<4;i++)
    for(j=-3;j<4;j++)
      n[i+3][j+3] = SceneVoxelAt(scene,pane.zinfo.x+i,pane.zinfo.y+j);

  x[0] = x[2] = 16;
  x[1] = x[3] = canvas.W - 16 - 7*16;

  y[0] = y[1] = 32;
  y[2] = y[3] = canvas.H - 32 - 7*16;

  gc_color(canvas.gc, darker(scene->colors.bg,3));
  for(j=0;j<4;j++)
    for(i=0;i<7*16;i+=2)
      gdk_draw_line(canvas.vista, canvas.gc, x[j], y[j]+i, x[j]+7*16, y[j]+i);

  if (!pane.zinfo.svalid) {
    pane.zinfo.s[0] = XAGetOrigViewShift(scene->avol);
    pane.zinfo.s[1] = XAGetValueViewShift(scene->avol);
    pane.zinfo.s[2] = XAGetCostViewShift(scene->avol);
    pane.zinfo.svalid = 1;
  }

  for(i=0;i<7;i++)
    for(j=0;j<7;j++) {
      if (n[i][j] != VLARGE) {

	// orig
	gc_color(canvas.gc, 
		 gray(scene->avol->vd[n[i][j]].orig >> pane.zinfo.s[0]));
	gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			   x[0]+i*16, y[0]+j*16, 16, 16);

	// value
	gc_color(canvas.gc, 
		 gray(scene->avol->vd[n[i][j]].value >> pane.zinfo.s[1]));
	gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			   x[1]+i*16, y[1]+j*16, 16, 16);

	// cost
	gc_color(canvas.gc,
		 gray(scene->avol->vd[n[i][j]].cost >> pane.zinfo.s[2]));
	gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			   x[2]+i*16, y[2]+j*16, 16, 16);

	// label
	gc_color(canvas.gc,
	 scene->colors.label[scene->avol->vd[n[i][j]].label&LABELMASK]);
	gdk_draw_rectangle(canvas.vista, canvas.gc, TRUE,
			   x[3]+i*16, y[3]+j*16, 16, 16);

	gc_color(canvas.gc, 0xffffff);
	for(k=0;k<4;k++)
	  gdk_draw_rectangle(canvas.vista, canvas.gc, FALSE,
			     x[k]+i*16, y[k]+j*16, 16, 16);

      }

    }


  for(i=0;i<4;i++) {
    gc_color(canvas.gc, 0xffff00);
    gdk_draw_rectangle(canvas.vista, canvas.gc, 
		       FALSE, x[i], y[i], 7*16, 7*16);
    gdk_draw_rectangle(canvas.vista, canvas.gc, 
		       FALSE, x[i]+3*16, y[i]+3*16, 16, 16);
    gc_color(canvas.gc, 0x404000);
    gdk_draw_rectangle(canvas.vista, canvas.gc, 
		       FALSE, x[i]+3*16-1, y[i]+3*16-1, 18, 18);

  }

  pango_layout_set_font_description(canvas.pl, canvas.pfd);

  if (n[3][3] != VLARGE) {
    XAVolumeVoxelOf(scene->avol, n[3][3], &v);
    
    sprintf(z,"position: (%d, %d, %d)",v.x,v.y,v.z);
    shielded_text(canvas.vista, canvas.pl, canvas.gc, x[0], y[0] + 8*16,
		  0xffff00, 0,z);

    sprintf(z,"value: %d", scene->avol->vd[n[3][3]].orig);
    shielded_text(canvas.vista, canvas.pl, canvas.gc, x[0], y[0] + 9*16,
		  0xffff00, 0,z);

    sprintf(z,"value: %d", scene->avol->vd[n[3][3]].value);
    shielded_text(canvas.vista, canvas.pl, canvas.gc, x[1], y[1] + 8*16,
		  0xffff00, 0,z);

    sprintf(z,"cost: %d", scene->avol->vd[n[3][3]].cost);
    shielded_text(canvas.vista, canvas.pl, canvas.gc, x[2], y[2] + 8*16,
		  0xffff00, 0,z);

    sprintf(z,"label: %d",
	    (scene->avol->vd[n[3][3]].label & LABELMASK));
    shielded_text(canvas.vista, canvas.pl, canvas.gc, x[3], y[3] + 8*16,
		  0xffff00, 0,z);

  } else {
    strcpy(z,"no projection");
    for(k=0;k<4;k++)
      shielded_text(canvas.vista, canvas.pl, canvas.gc, x[k], y[k] + 8*16,
		    0xffff00, 0, z);
  }

  shielded_text(canvas.vista, canvas.pl, canvas.gc, x[0], y[0] - 4,
		0xffff00, 0,"Original Data");
  shielded_text(canvas.vista, canvas.pl, canvas.gc, x[1], y[1] - 4,
	      0xffff00, 0,"Current Data");
  shielded_text(canvas.vista, canvas.pl, canvas.gc, x[2], y[2] - 4,
	      0xffff00, 0,"Path Costs");
  shielded_text(canvas.vista, canvas.pl, canvas.gc, x[3], y[3] - 4,
	      0xffff00, 0,"Labels");

}


void      ivs_viewspec_save(char *name)
{
  FILE *f;
  int i;

  f = fopen(name,"w");
  if (!f) {
    PopMessageBox(mainwindow,"Error",
		  "Could not open file for writing.",MSG_ICON_WARN);
    return;
  }

  fprintf(f,"%%IVS View Specification\n");
  fprintf(f,"rot=%d,%d,%d\n",
	  (int)(scene->rotation.x),
	  (int)(scene->rotation.y),
	  (int)(scene->rotation.z));
  fprintf(f,"clip=%d,%d,%d,%d,%d,%d\n",
	  scene->clip.v[0][0],
	  scene->clip.v[0][1],
	  scene->clip.v[1][0],
	  scene->clip.v[1][1],
	  scene->clip.v[2][0],
	  scene->clip.v[2][1]);
  fprintf(f,"light=%f,%f,%f,%f,%f,%d,%f,%f,%f\n",
	  scene->options.phong.ambient,
	  scene->options.phong.diffuse,
	  scene->options.phong.specular,
	  scene->options.phong.zgamma,
	  scene->options.phong.zstretch,
	  scene->options.phong.sre,
	  scene->options.phong.light.x,
	  scene->options.phong.light.y,
	  scene->options.phong.light.z);
  fprintf(f,"hollow=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
	  scene->hollow[0],scene->hollow[1],
	  scene->hollow[2],scene->hollow[3],
	  scene->hollow[4],scene->hollow[5],
	  scene->hollow[6],scene->hollow[7],
	  scene->hollow[8],scene->hollow[9]);
  fprintf(f,"modes=%d,%d,%d,%d\n",
	  (int) (scene->options.vmode),
	  (int) (scene->options.zoom),
	  (int) (scene->options.src),
	  (int) (scene->options.shading));
  fprintf(f,"colors=");
  for(i=0;i<10;i++)
    fprintf(f,"%.6x,",scene->colors.label[i]);
  fprintf(f,"%.6x,%.6x,%.6x\n",
	  scene->colors.bg,
	  scene->colors.clipcube,
	  scene->colors.fullcube);
  fclose(f);
  ivs_set_status("View specification saved.", 10);
}

void      ivs_viewspec_load(char *name)
{
  FILE *f;
  int i,x[20];
  float y[10];
  char line[512],tmp[512], aux[512];
  
  f = fopen(name,"r");
  if (!f) {
    PopMessageBox(mainwindow,"Error",
		  "Could not open file for reading.",MSG_ICON_WARN);
    return;
  }

  while(fgets(line,512,f)!=0) {
    if (strstr(line,"rot=")!=0) {
      sscanf(line,"rot= %d , %d , %d \n",
	     &x[0], &x[1], &x[2]);
      scene->rotation.x = x[0];
      scene->rotation.y = x[1];
      scene->rotation.z = x[2];
      continue;
    }
    if (strstr(line,"clip=")!=0) {
      sscanf(line,"clip= %d , %d , %d , %d , %d , %d \n",
	     &x[0], &x[1], &x[2], &x[3], &x[4], &x[5]);
      scene->clip.v[0][0] = x[0];
      scene->clip.v[0][1] = x[1];
      scene->clip.v[1][0] = x[2];
      scene->clip.v[1][1] = x[3];
      scene->clip.v[2][0] = x[4];
      scene->clip.v[2][1] = x[5];
      continue;
    }
    if (strstr(line,"light=")!=0) {
      sscanf(line,"light= %f , %f , %f , %f , %f , %d , %f , %f , %f \n",
	     &y[0], &y[1], &y[2], &y[3], &y[4], &x[0],
	     &y[5], &y[6], &y[7]);
      scene->options.phong.ambient  = y[0];
      scene->options.phong.diffuse  = y[1];
      scene->options.phong.specular = y[2];
      scene->options.phong.zgamma   = y[3];
      scene->options.phong.zstretch = y[4];
      scene->options.phong.sre      = x[0];
      scene->options.phong.light.x  = y[5];
      scene->options.phong.light.y  = y[6];
      scene->options.phong.light.z  = y[7];
      continue;
    }
    if (strstr(line,"hollow=")!=0) {
      sscanf(line,"hollow= %s\n", tmp);
      for(i=0;i<10;i++) {
	sscanf(tmp, "%d , %s", &x[0], aux);
	scene->hollow[i] = x[0];
	strcpy(tmp, aux);
      }
      continue;
    }
    if (strstr(line,"modes=")!=0) {
      sscanf(line,"modes= %d , %d , %d , %d \n",
	     &x[0], &x[1], &x[2], &x[3]);
      scene->options.vmode   = x[0];
      scene->options.zoom    = x[1];
      scene->options.src     = x[2];
      scene->options.shading = x[3];
      continue;
    }
    if (strstr(line,"colors=")!=0) {
      sscanf(line,"colors= %s\n", tmp);
      for(i=0;i<10;i++) {
	sscanf(tmp, "%x , %s", &x[0], aux);
	scene->colors.label[i] = x[0];
	strcpy(tmp, aux);
      }
      sscanf(tmp, " %x , %x , %x ",&x[0],&x[1],&x[2]);
      scene->colors.bg = x[0];
      scene->colors.clipcube = x[1];
      scene->colors.fullcube = x[2];
      continue;
    }
  }
  fclose(f);

  scene->clip.v[0][0] = MIN(scene->avol->W, scene->clip.v[0][0]);
  scene->clip.v[0][1] = MIN(scene->avol->W, scene->clip.v[0][1]);

  scene->clip.v[1][0] = MIN(scene->avol->H, scene->clip.v[1][0]);
  scene->clip.v[1][1] = MIN(scene->avol->H, scene->clip.v[1][1]);

  scene->clip.v[2][0] = MIN(scene->avol->D, scene->clip.v[2][0]);
  scene->clip.v[2][1] = MIN(scene->avol->D, scene->clip.v[2][1]);

  ivs_scene_to_pane(M_ALL);
  scene->invalid = 1;
  ivs_rot(0,0);
  ivs_set_status("View specification loaded.", 10);
}

void ivs_warn_no_volume() {
  PopMessageBox(mainwindow, "No Data",
		"There is no volume loaded.",
		MSG_ICON_WARN);
}

void ivs_draw_tvoxel() {
  XAnnVolume *avol = scene->avol;
  int c1;
  int p,q;
  Transform Ta,Tb,Tc,Td;
  int HW,HH,HD;
  Point a,b,c,d;
  Voxel va,vb;
  int s[4];
  GdkGC *gc = canvas.gc;

  c1 = scene->colors.label[ avol->vd[canvas.tvoxel].label & LABELMASK ];

  TransformXRot(&Ta, scene->rotation.x);
  TransformYRot(&Tb, scene->rotation.y);
  TransformZRot(&Tc, scene->rotation.z);
  TransformIsoScale(&Td, SceneScale(scene));
  TransformCompose(&Tb,&Tc);
  TransformCompose(&Ta,&Tb);
  TransformCompose(&Td,&Ta);

  HW = avol->W / 2;
  HH = avol->H / 2;
  HD = avol->D / 2;

  gc_color(gc, c1);

  p = canvas.tvoxel;
  q = XAGetDecPredecessor(avol, p);
  if (p==q) return;
  while(q!=p) {

    XAVolumeVoxelOf(avol, p, &va);
    XAVolumeVoxelOf(avol, q, &vb);
    PointAssign(&a, va.x, va.y, va.z);
    PointTranslate(&a, -HW, -HH, -HD);
    PointAssign(&b, vb.x, vb.y, vb.z);
    PointTranslate(&b, -HW, -HH, -HD);
    PointTransform(&c,&a,&Td);
    PointTransform(&d,&b,&Td);

    s[0] = c.x + scene->pvt.oxc;
    s[1] = c.y + scene->pvt.oyc;
    s[2] = d.x + scene->pvt.oxc;
    s[3] = d.y + scene->pvt.oyc;

    if (s[0] >=0 && s[1] >=0 && s[2]>=0 && s[3] >=0 &&
	s[0] <canvas.W && s[1]<canvas.H && s[2] <canvas.W &&
	s[3] <canvas.H) {
      gdk_draw_line(canvas.vista, gc, s[0],s[1],s[2],s[3]);
    }

    p = q;
    q = XAGetDecPredecessor(avol, p);
  }

  gdk_draw_rectangle(canvas.vista, gc, TRUE, s[2]-2, s[3]-2,4,4);

}

gboolean  mipcurve_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) {
  int w,h;
  GdkGC *gc;

  float x1, x2;
  GdkPoint P[3];

  gdk_window_get_size(widget->window, &w, &h);
  gc = gdk_gc_new(widget->window);

  x1 = 1 + scene->mip_ramp_min * (w-3);
  x2 = 1 + scene->mip_ramp_max * (w-3);

  gc_color(gc, 0xc0c0c0);
  gdk_draw_rectangle(widget->window,gc,TRUE,0,0,w,h);
  gc_color(gc, 0x8080ff);

  P[0].x = (int) x1;
  P[0].y = h-1;

  P[1].x = (int) x2;
  P[1].y = h-1;

  P[2].x = (int) x2;
  P[2].y = 0;

  gdk_draw_polygon(widget->window, gc, TRUE, &P[0], 3);
  gdk_draw_rectangle(widget->window, gc, TRUE, x2, 0, w-x2+1,h);

  gc_color(gc, 0);
  gdk_draw_line(widget->window, gc, 0, h-1, x1, h-1);
  gdk_draw_line(widget->window, gc, x1, h-1, x2, 0);
  gdk_draw_line(widget->window, gc, x2, 0, w, 0);

  gdk_draw_rectangle(widget->window, gc, FALSE, 0,0,w-1,h-1);

  gc_color(gc,0xff0000);
  gdk_draw_line(widget->window, gc, x1, 0, x1, h-1);
  gdk_draw_line(widget->window, gc, x2, 0, x2, h-1);

  gdk_gc_destroy(gc);
  return FALSE;
}

gboolean  mipcurve_drag(GtkWidget *widget, GdkEventMotion *em, gpointer data) {
  int w,h;
  gdk_window_get_size(widget->window, &w, &h);
  mipcurve_mouse(em->x,em->y,w,h);
  return FALSE;
}

gboolean  mipcurve_press(GtkWidget *widget, GdkEventButton *eb, gpointer data) {
  int w,h;
  gdk_window_get_size(widget->window, &w, &h);
  mipcurve_mouse(eb->x,eb->y,w,h);
  return FALSE;
}

void mipcurve_mouse(int x,int y,int w,int h) {
  float v;

  v = ((float)(x-1)) / ((float)(w-3));

  if (v < 0.0) v = 0.0;
  if (v > 1.0) v = 1.0;
  if (v < scene->mip_ramp_min) {
    scene->mip_ramp_min = v;
  } else if (v > scene->mip_ramp_max) {
    scene->mip_ramp_max = v;
  } else if (v - scene->mip_ramp_min < scene->mip_ramp_max - v) {
    scene->mip_ramp_min = v;
  } else {
    scene->mip_ramp_max = v;
  }

  refresh(pane.overlay.mipcurve);
  ivs_tog(0,0);
}

void ivs_mipload(GtkWidget *widget, gpointer data) {
  char name[512], z[256];
  ArgBlock *args;

  if (ivs_get_filename(mainwindow, 
		       "Load Bew Overlay Volume... (SCN)", 
		       name,
		       GF_ENSURE_READABLE)) {

    sprintf(z,"Load Overlay Volume - %s",ivs_short_filename(name));

    args = ArgBlockNew();
    strncpy(args->sa, name, 512);
    args->sa[511] = 0;
    args->scene = scene;
    args->ack   = &sync_newdata;
    
    TaskNew(qseg,z,(Callback) &be_load_mip_volume, args);
  }
}
