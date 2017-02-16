/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2017
   
   Felipe P. G. Bergo 
   fbergo at gmail dot com

   and
   
   Alexandre X. Falcao
   afalcao at ic.unicamp.br 

   ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <libip.h>
#include "animate.h"
#include "util.h"

#include "xpm/playpause.xpm"
#include "xpm/ff.xpm"
#include "xpm/forw.xpm"
#include "xpm/backw.xpm"

#include "xpm/dialog.xpm"

#define ST_PAUSE    0
#define ST_PLAY     1
#define ST_FF       2

volatile struct _animate_data {
  GtkWidget *dlg, *canvas;
  Scene     *scn;
  int       nimg;
  RLEImg    **frames;
  int       rendering_done;
  int       frame;
  int       toid;
  int       w,h;
  GdkPixmap *buf;
  GdkGC     *bufgc;
  int       state;
  int       step;
  int       op;
  pthread_t tid;

  PangoLayout *pl;
  PangoFontDescription *pfd;
  
  GtkWidget *playpause, *ff, *stepf, *stepb;
} Anim = { 0,0, 0, 0, 0, 0, 0, 0, 0,0, 0,0,  ST_PLAY, 0, ANIM_SWEEP, 0,
	   0,0,0,0, NULL, NULL };

FullTimer ETA;

gboolean anim_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void     anim_destroy(GtkWidget *w, gpointer data);
gboolean anim_expose(GtkWidget *widget,GdkEventExpose *ee,
			   gpointer data);
gboolean anim_update(gpointer data);
void     anim_frame_advance();
void     anim_playpause(GtkWidget *w, gpointer data);
void     anim_ff(GtkWidget *w, gpointer data);
void     anim_stepf(GtkWidget *w, gpointer data);
void     anim_stepb(GtkWidget *w, gpointer data);
int      anim_frames(int domain, int step);

void *   anim_sweep_render(void *p);
void *   anim_rot_y_render(void *p);

void *   anim_growth_render(void *p);
void *   anim_wavefront_render(void *p);

int anim_frames(int domain, int step) {
  if (domain % step)
    return (1 + (domain / step));
  else
    return (domain / step);
}

void animate(GtkWidget *mainwindow, Scene *scene, int operation, int step) {
  int w,h,i;
  GtkWidget *v, *hb;
  GtkWidget *xpm[4];
  pthread_t tid;

  if (scene->avol == 0 || Anim.dlg!=0)
    return;

  Anim.step = step;
  Anim.op   = operation;
  Anim.tid  = 0;

  Anim.scn = SceneNew();
  SceneCopy(Anim.scn, scene);
  Anim.scn->wantmesh = 0;

  switch(Anim.op) {
  case ANIM_SWEEP:
    Anim.nimg   = anim_frames(scene->avol->W,step) + 
      anim_frames(scene->avol->H,step) + anim_frames(scene->avol->D, step);
    break;
  case ANIM_ROT_Y:
    Anim.nimg   = anim_frames(360, step);
    break;
  case ANIM_GROWTH:
  case ANIM_WAVEFRONT:
    Anim.nimg   = anim_frames(32768, step);
    break;
  }

  Anim.frames = (RLEImg **) malloc(sizeof(RLEImg *) * Anim.nimg);

  for(i=0;i<Anim.nimg;i++)
    Anim.frames[i] = 0;

  Anim.w = w = scene->vout->W;
  Anim.h = h = scene->vout->H;
  
  Anim.dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(Anim.dlg), "IVS: Animate");
  gtk_container_set_border_width(GTK_CONTAINER(Anim.dlg),2);
  gtk_widget_realize(Anim.dlg);
  set_icon(Anim.dlg, dialog_xpm, "Animate");

  v = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(Anim.dlg), v);  

  Anim.canvas = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(Anim.canvas), w, h);
  
  Anim.pl =  gtk_widget_create_pango_layout(Anim.canvas, NULL);
  Anim.pfd = pango_font_description_from_string("Sans 10");
  if (Anim.pl != NULL && Anim.pfd != NULL)
    pango_layout_set_font_description(Anim.pl, Anim.pfd);

  gtk_box_pack_start(GTK_BOX(v), Anim.canvas, TRUE, TRUE, 0);
  
  hb = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(v), hb, FALSE, TRUE, 2);

  Anim.playpause = gtk_button_new();
  Anim.ff        = gtk_toggle_button_new();
  Anim.stepb     = gtk_button_new();
  Anim.stepf     = gtk_button_new();

  gtk_container_set_border_width(GTK_CONTAINER(Anim.playpause), 2);
  gtk_container_set_border_width(GTK_CONTAINER(Anim.ff), 2);
  gtk_container_set_border_width(GTK_CONTAINER(Anim.stepf), 2);
  gtk_container_set_border_width(GTK_CONTAINER(Anim.stepb), 2);

  xpm[0] = ivs_pixmap_new(mainwindow, playpause_xpm);
  xpm[1] = ivs_pixmap_new(mainwindow, ff_xpm);
  xpm[2] = ivs_pixmap_new(mainwindow, forw_xpm);
  xpm[3] = ivs_pixmap_new(mainwindow, backw_xpm);

  gtk_box_pack_start(GTK_BOX(hb), Anim.playpause, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hb), Anim.ff, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hb), Anim.stepb, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hb), Anim.stepf, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(Anim.playpause), xpm[0]);
  gtk_container_add(GTK_CONTAINER(Anim.ff), xpm[1]);
  gtk_container_add(GTK_CONTAINER(Anim.stepf), xpm[2]);
  gtk_container_add(GTK_CONTAINER(Anim.stepb), xpm[3]);

  gtk_widget_set_sensitive(Anim.stepf, FALSE);
  gtk_widget_set_sensitive(Anim.stepb, FALSE);

  /* signals */

  gtk_signal_connect(GTK_OBJECT(Anim.canvas),"expose_event",
                     GTK_SIGNAL_FUNC(anim_expose), 0);
  gtk_signal_connect(GTK_OBJECT(Anim.dlg),"delete_event",
                     GTK_SIGNAL_FUNC(anim_delete), 0);
  gtk_signal_connect(GTK_OBJECT(Anim.dlg),"destroy",
                     GTK_SIGNAL_FUNC(anim_destroy), 0);

  gtk_signal_connect(GTK_OBJECT(Anim.playpause), "clicked",
		     GTK_SIGNAL_FUNC(anim_playpause), 0);
  gtk_signal_connect(GTK_OBJECT(Anim.ff), "toggled",
		     GTK_SIGNAL_FUNC(anim_ff), 0);
  gtk_signal_connect(GTK_OBJECT(Anim.stepb), "clicked",
		     GTK_SIGNAL_FUNC(anim_stepb), 0);
  gtk_signal_connect(GTK_OBJECT(Anim.stepf), "clicked",
		     GTK_SIGNAL_FUNC(anim_stepf), 0);

  gtk_widget_show(Anim.canvas);
  gtk_widget_show(Anim.playpause);
  gtk_widget_show(Anim.ff);
  gtk_widget_show(Anim.stepf);
  gtk_widget_show(Anim.stepb);
  for(i=0;i<4;i++)
    gtk_widget_show(xpm[i]);
  gtk_widget_show(hb);
  gtk_widget_show(v);
  gtk_widget_show(Anim.dlg);

  gtk_timeout_add(100, anim_update, 0);

  switch(Anim.op) {
  case ANIM_SWEEP:
    pthread_create(&tid, NULL, &anim_sweep_render, NULL);
    Anim.tid = tid;
    StartTimer(&ETA);
    break;
  case ANIM_ROT_Y:
    pthread_create(&tid, NULL, &anim_rot_y_render, NULL);
    Anim.tid = tid;
    StartTimer(&ETA);
    break;
  case ANIM_WAVEFRONT:
    pthread_create(&tid, NULL, &anim_wavefront_render, NULL);
    Anim.tid = tid;
    StartTimer(&ETA);
    break;
  case ANIM_GROWTH:
    pthread_create(&tid, NULL, &anim_growth_render, NULL);
    Anim.tid = tid;
    StartTimer(&ETA);
    break;
  }
}

void anim_playpause(GtkWidget *w, gpointer data) {

  if (Anim.state == ST_PAUSE) {
    
    Anim.state = ST_PLAY;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Anim.ff), FALSE);
    gtk_widget_set_sensitive(Anim.stepb, FALSE);
    gtk_widget_set_sensitive(Anim.stepf, FALSE);
    gtk_widget_set_sensitive(Anim.ff, TRUE);

  } else {

    Anim.state = ST_PAUSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Anim.ff), FALSE);
    gtk_widget_set_sensitive(Anim.stepb, TRUE);
    gtk_widget_set_sensitive(Anim.stepf, TRUE);
    gtk_widget_set_sensitive(Anim.ff, FALSE);

  }

}

void anim_ff(GtkWidget *w, gpointer data) {

  if (Anim.state == ST_PAUSE) return;
  
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Anim.ff))) {
    Anim.state = ST_FF;
  } else {
    Anim.state = ST_PLAY;
  }

}

void anim_stepf(GtkWidget *w, gpointer data) {
  int ready, current;

  ready   = Anim.rendering_done;
  current = (Anim.frame + 1) % Anim.nimg;

  if (current < ready)
    Anim.frame = current;  
}

void anim_stepb(GtkWidget *w, gpointer data) {
  int ready, current;

  ready   = Anim.rendering_done;
  current = (Anim.frame + Anim.nimg - 1) % Anim.nimg;

  if (current < ready)
    Anim.frame = current;
}

gboolean 
anim_delete(GtkWidget *w, GdkEvent *e, gpointer data) { 
  pthread_t tid;

  if (Anim.rendering_done != Anim.nimg) {
    
    tid = Anim.tid;

    if (tid != 0) {
      pthread_cancel(tid);
      pthread_join(tid, 0);
      Anim.tid = 0;
    }

  } 
  return FALSE; 
}

void anim_destroy(GtkWidget *w, gpointer data) {
  int i;
  if (Anim.rendering_done) {
    for(i=0;i<Anim.nimg;i++)
      if (Anim.frames[i] != 0)
	RLEDestroy(Anim.frames[i]);
    free(Anim.frames);
    SceneDestroy(Anim.scn);

    gdk_gc_destroy(Anim.bufgc);
    gdk_pixmap_unref(Anim.buf);

    Anim.dlg = 0;
    Anim.canvas = 0;
    Anim.rendering_done = 0;
    Anim.frame = 0;
    Anim.buf = 0;
    Anim.bufgc = 0;
    Anim.state = ST_PLAY;
    
    if (Anim.toid)
      gtk_timeout_remove(Anim.toid);
    Anim.toid = 0;
  }
}

void anim_frame_advance() {
  int ready, current;
  if (Anim.state == ST_PAUSE)
    return;
  ready   = Anim.rendering_done;
  current = Anim.frame;

  if (Anim.state == ST_FF)
    current += 3;
  else
    current += 1;

  current %= Anim.nimg;

  if (current >= ready) current = ready - 1;
  Anim.frame = current;
}

gboolean 
anim_expose(GtkWidget *widget,GdkEventExpose *ee,
		  gpointer data) 
{
  int ww,wh;
  char z[64];
  CImg *decode;
  int ready, current;
  int inframe = -1;
  int W,H;

  double eta;
  int min, sec;

  W = Anim.scn->avol->W;
  H = Anim.scn->avol->H;
  
  gdk_window_get_size(widget->window, &ww, &wh);

  if (Anim.buf == 0) {
    Anim.buf = gdk_pixmap_new(widget->window, ww, wh, -1);
    Anim.bufgc = gdk_gc_new(Anim.buf);
  }

  ready = Anim.rendering_done;
  current = Anim.frame;

  if (Anim.rendering_done == Anim.nimg) {

    if (Anim.tid != 0) {
      pthread_join(Anim.tid, 0);
      Anim.tid = 0;
    }

    decode = DecodeRLE(Anim.frames[current]);
    gdk_draw_rgb_image(Anim.buf, Anim.bufgc,
		       0,0,
		       decode->W, decode->H,
		       GDK_RGB_DITHER_NORMAL,
		       decode->val,
		       decode->rowlen);
    CImgDestroy(decode);
    anim_frame_advance();
    inframe = current;

  } else {

    if (ready > 0) {

      current = Anim.frame;
      if (current >= ready)
	current = ready - 1;
      else
	anim_frame_advance();

      decode = DecodeRLE(Anim.frames[current]);
      gdk_draw_rgb_image(Anim.buf, Anim.bufgc,
			 0,0,
			 decode->W, decode->H,
			 GDK_RGB_DITHER_NORMAL,
			 decode->val,
			 decode->rowlen);
      CImgDestroy(decode);
      inframe = current;

    } else {
      gc_color(Anim.bufgc, 0);
      gdk_draw_rectangle(Anim.buf, Anim.bufgc,
			 TRUE, 0,0, ww, wh);
    }

    StopTimer(&ETA);
    eta = TimeElapsed(&ETA);
    if (Anim.rendering_done > 0) {
      eta = eta / ((double)(Anim.rendering_done));
    } else {
      eta = 5.0;
    }
    eta *= (double) (Anim.nimg - Anim.rendering_done);
    min = ((int) eta) / 60;
    sec = ((int) eta) % 60;

    gc_color(Anim.bufgc, 0xffffff);
    sprintf(z,"Rendering. Done %d of %d frames (%.2d:%.2d until completion)",
	    Anim.rendering_done, Anim.nimg,min, sec);
    pango_layout_set_text(Anim.pl, z, -1);
    gdk_draw_layout(Anim.buf, Anim.bufgc, 10, 60, Anim.pl);
  }

  if (inframe >= 0) {

    switch(Anim.state) {
    case ST_PAUSE: strcpy(z,"(paused)"); break;
    case ST_PLAY:  strcpy(z,"(playing)"); break;
    case ST_FF:    strcpy(z,"(fast forward)"); break;
    }

    gc_color(Anim.bufgc, 0xffdd00);
    pango_layout_set_text(Anim.pl, z, -1);
    gdk_draw_layout(Anim.buf, Anim.bufgc, 10, 20, Anim.pl);

    switch(Anim.op) {
    case ANIM_SWEEP:
      if (inframe < W / Anim.step) {
	sprintf(z,"Frame #%d : X = %d",
		inframe, Anim.step * inframe);
      } else if (inframe < (W + H) / Anim.step ) {
	sprintf(z,"Frame #%d : Y = %d",
		inframe, Anim.step * (inframe - W/Anim.step) );
      } else {
	sprintf(z,"Frame #%d : Z = %d",
		inframe, Anim.step * (inframe - ((W + H)/Anim.step) ) );
      }
      break;
    case ANIM_ROT_Y:
      sprintf(z,"Frame #%d : Y rotation = %d deg",
	      inframe, Anim.step * inframe);
      break;
    case ANIM_GROWTH:
    case ANIM_WAVEFRONT:
      sprintf(z,"Frame #%d",inframe);
      break;
    }

    gc_color(Anim.bufgc, 0xffff00);
    pango_layout_set_text(Anim.pl, z, -1);
    gdk_draw_layout(Anim.buf, Anim.bufgc, 10, 40, Anim.pl);
    
  }

  gdk_draw_pixmap(widget->window, widget->style->black_gc,
		  Anim.buf, 0,0, 0,0, Anim.w, Anim.h);

  return FALSE;
}

gboolean anim_update(gpointer data) {
  if (Anim.dlg) {
    gtk_widget_queue_draw(Anim.canvas);
    return TRUE;
  } else {
    Anim.toid = 0;
    return FALSE;
  }
}

void *anim_sweep_render(void *p) {
  int frame = 0;
  int i;

  for(i=0;i<Anim.scn->avol->W;i+=Anim.step) {
    Anim.scn->clip.v[0][0] = i;
    Anim.scn->clip.v[0][1] = i;
    Anim.scn->clip.v[1][0] = 0;
    Anim.scn->clip.v[1][1] = Anim.scn->avol->H - 1;
    Anim.scn->clip.v[2][0] = 0;
    Anim.scn->clip.v[2][1] = Anim.scn->avol->D - 1;
    Anim.scn->invalid = 1;
    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();
  }

  for(i=0;i<Anim.scn->avol->H;i+=Anim.step) {
    Anim.scn->clip.v[0][0] = 0;
    Anim.scn->clip.v[0][1] = Anim.scn->avol->W - 1;
    Anim.scn->clip.v[1][0] = i;
    Anim.scn->clip.v[1][1] = i;
    Anim.scn->clip.v[2][0] = 0;
    Anim.scn->clip.v[2][1] = Anim.scn->avol->D - 1;
    Anim.scn->invalid = 1;
    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();
  }

  for(i=0;i<Anim.scn->avol->D;i+=Anim.step) {
    Anim.scn->clip.v[0][0] = 0;
    Anim.scn->clip.v[0][1] = Anim.scn->avol->W - 1;
    Anim.scn->clip.v[1][0] = 0;
    Anim.scn->clip.v[1][1] = Anim.scn->avol->H - 1;
    Anim.scn->clip.v[2][0] = i;
    Anim.scn->clip.v[2][1] = i;
    Anim.scn->invalid = 1;
    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();
  }

  Anim.rendering_done = Anim.nimg;
  pthread_exit(0);
  return 0;
}

void *anim_rot_y_render(void *p) {
  int frame = 0;
  int i;

  Anim.scn->clip.v[0][0] = 0;
  Anim.scn->clip.v[0][1] = Anim.scn->avol->W - 1;
  Anim.scn->clip.v[1][0] = 0;
  Anim.scn->clip.v[1][1] = Anim.scn->avol->H - 1 ;
  Anim.scn->clip.v[2][0] = 0;
  Anim.scn->clip.v[2][1] = Anim.scn->avol->D - 1;

  for(i=0;i<360;i+=Anim.step) {
    Anim.scn->rotation.y = (float) i;
    Anim.scn->invalid = 1;
    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();
  }

  Anim.rendering_done = Anim.nimg;
  pthread_exit(0);
  return 0;
}

void *   anim_growth_render(void *p) {
  int frame = 0;
  int i, j, rstep, upper, lastj=-1;

  int *hist;

  hist = (int *) malloc(sizeof(int) * 32768);

  MemSet(hist, 0, sizeof(int) * 32768);
  for(i=0;i<Anim.scn->avol->N;i++)
    ++hist[Anim.scn->avol->vd[i].cost];
  for(i=1;i<32768;i++)
    hist[i] += hist[i-1];

  rstep = Anim.scn->avol->N / Anim.nimg;

  Anim.scn->clip.v[0][0] = 0;
  Anim.scn->clip.v[0][1] = Anim.scn->avol->W - 1;
  Anim.scn->clip.v[1][0] = 0;
  Anim.scn->clip.v[1][1] = Anim.scn->avol->H - 1 ;
  Anim.scn->clip.v[2][0] = 0;
  Anim.scn->clip.v[2][1] = Anim.scn->avol->D - 1;

  for(i=0;i<Anim.nimg;i++) {

    upper = rstep * (i+1);

    for(j=1;j<32768;j++)
      if (hist[j] >= upper)
	break;

    if (j == lastj)
      ++j;

    Anim.scn->with_restriction = 1;
    Anim.scn->cost_threshold_min = 0;
    Anim.scn->cost_threshold_max = j;
    Anim.scn->invalid = 1;

    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();

    lastj = j;
  }

  free(hist);
  Anim.rendering_done = Anim.nimg;
  pthread_exit(0);
  return 0;
}

void *   anim_wavefront_render(void *p) {
  int frame = 0;
  int i, j, k, rstep, upper, lower;
  int lastj=-1, lastk=-1;

  int *hist;

  hist = (int *) malloc(sizeof(int) * 32768);

  MemSet(hist, 0, sizeof(int) * 32768);
  for(i=0;i<Anim.scn->avol->N;i++)
    ++hist[Anim.scn->avol->vd[i].cost];
  for(i=1;i<32768;i++)
    hist[i] += hist[i-1];

  rstep = Anim.scn->avol->N / Anim.nimg;

  Anim.scn->clip.v[0][0] = 0;
  Anim.scn->clip.v[0][1] = Anim.scn->avol->W - 1;
  Anim.scn->clip.v[1][0] = 0;
  Anim.scn->clip.v[1][1] = Anim.scn->avol->H - 1 ;
  Anim.scn->clip.v[2][0] = 0;
  Anim.scn->clip.v[2][1] = Anim.scn->avol->D - 1;

  for(i=0;i<Anim.nimg;i++) {

    lower = rstep * i;

    for(k=0;k<32768;k++)
      if (hist[k] >= lower)
	break;

    upper = rstep * (i+1);

    for(j=k+1;j<32768;j++)
      if (hist[j] >= upper)
	break;

    if (k == lastk && j == lastj)
      ++j;

    Anim.scn->with_restriction = 1;
    Anim.scn->cost_threshold_min = k;
    Anim.scn->cost_threshold_max = j;
    Anim.scn->invalid = 1;

    ScenePrepareImage(Anim.scn, Anim.w, Anim.h);
    CImgFill(Anim.scn->vout, Anim.scn->colors.bg);
    RenderScene(Anim.scn, Anim.w, Anim.h, 0);
    RenderClipCube(Anim.scn);
    Anim.frames[frame] = EncodeRLE(Anim.scn->vout);
    ++frame;
    Anim.rendering_done = frame;
    pthread_testcancel();

    lastj = j;
    lastk = k;
  }

  free(hist);
  Anim.rendering_done = Anim.nimg;
  pthread_exit(0);
  return 0;
}
