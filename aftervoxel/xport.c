
#define SOURCE_XPORT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libip.h>
#include <jpeglib.h>
#include <png.h>
#include <errno.h>
#include "aftervoxel.h"

static struct _xpdlg {
  GtkWidget *dlg;
  GtkWidget *pw;
  GtkWidget *fmt[3],*quality,*qtext;
  GtkWidget *tx,*ty,*togray;
  DropBox   *what;
  ColorLabel *bg;

  CImg *img[4],*buf[2];
  gint toid,jid;

  int preview_size;
  
  int v_fmt;
  int v_quality;
  int v_tx;
  int v_ty;
  int v_what;
  int v_bg;
  int v_gray;
} xp = { .v_fmt = 0, .v_quality = 100, .v_tx = 512, .v_ty = 512, 
	 .v_what = 3, .v_bg = 1, .img = {0,0,0,0}, .v_gray = 0, 
	 .toid = -1, .jid = -1, .preview_size = 0 } ;

void xp_destroy(GtkWidget *w, gpointer data);
void xp_ok(GtkWidget *w, gpointer data);
void xp_cancel(GtkWidget *w, gpointer data);
void xp_src(void *args);
void xp_fmt(GtkToggleButton *w,gpointer data);
void xp_gray(GtkToggleButton *w,gpointer data);
void xp_c0(GtkWidget *w, gpointer data);
void xp_c1(GtkWidget *w, gpointer data);
void xp_c2(GtkWidget *w, gpointer data);
void xp_c3(GtkWidget *w, gpointer data);
void xp_width(GtkSpinButton *sb, gpointer data);
void xp_quality(GtkRange *w,gpointer data);
gboolean xp_repaint(gpointer data);
gboolean xp_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
CImg * xport_fake_2D(int i);
gboolean xp_jpreview(gpointer data);
void     xp_queue_jpeg_preview();

int xp_save_jpeg(char *path);
int xp_save_png(char *path);
int xp_save_ppm(char *path);
int xp_save_pgm(char *path);

gboolean xp_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  
  int W,H;
  float rw,rh;
  CImg *tmp,*src;
  char t[256];

  W = w->allocation.width;
  H = w->allocation.height;

  src = xp.img[xp.v_what];
  if (xp.buf[1] && xp.v_fmt==0)
    src = xp.buf[1];

  rw = ((float)W) / ((float)(src->W));
  rh = ((float)H) / ((float)(src->H));

  if (xp.v_tx <= W && xp.v_ty <= H) {
    tmp = CImgScaleToWidth(src,xp.v_tx);
  } else if (rw < rh) 
    tmp = CImgScaleToWidth(src,W);
  else
    tmp = CImgScaleToHeight(src,H);

  if (xp.v_gray)
    CImgGray(tmp);

  gdk_draw_rgb_image(w->window, w->style->black_gc, 
		     0,0,tmp->W,tmp->H,GDK_RGB_DITHER_NORMAL,
		     tmp->val, tmp->rowlen);
  CImgDestroy(tmp);

  if (xp.preview_size < 0)
    gtk_label_set_text(GTK_LABEL(xp.qtext)," ");
  else {   
    snprintf(t,256,"<span size='small'><i>%s</i>\n%d Bytes (%.2f KB)</span>",
	     "Estimated Size:"
	     ,xp.preview_size,((float)xp.preview_size)/1024.0);
    gtk_label_set_markup(GTK_LABEL(xp.qtext),str2utf8(t));
  }

  return FALSE;
}

void xp_width(GtkSpinButton *sb, gpointer data) {
  float r,h,w;
  int ih;
  char z[32];

  w = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(xp.tx));
  xp.v_tx = (int) w;
  r = ((float) (xp.img[xp.v_what]->W)) / ((float) (xp.img[xp.v_what]->H));
  h = w / r;
  ih = (int) h;
  xp.v_ty = ih;
  snprintf(z,32,"%d",ih);
  gtk_label_set_text(GTK_LABEL(xp.ty),z);

  if (xp.toid >= 0)
    gtk_timeout_remove(xp.toid);
  xp.toid = gtk_timeout_add(250,xp_repaint,0);
}

void xp_gray(GtkToggleButton *w,gpointer data) {
  xp.v_gray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xp.togray));
  xp_queue_jpeg_preview();
  refresh(xp.pw);  
}

void xp_bg_change() {
  int i;
  for(i=0;i<3;i++) {
    CImgDestroy(xp.img[i]);
    xp.img[i] = xport_fake_2D(i);
  }
  CImgFullCopy(xp.img[3],xp.buf[0]);
  if (xp.v_bg != view.bg)
    CImgSubst(xp.img[3],xp.v_bg,view.bg);
  ColorLabelSetColor(xp.bg, xp.v_bg);
  xp_queue_jpeg_preview();
  refresh(xp.pw);
}

void xp_c0(GtkWidget *w, gpointer data) {
  xp.v_bg = view.bg;
  xp_bg_change();
}

void xp_c1(GtkWidget *w, gpointer data) {
  xp.v_bg = 0xffffff;
  xp_bg_change();
}

void xp_c2(GtkWidget *w, gpointer data) {
  xp.v_bg = 0x000000;
  xp_bg_change();
}

void xp_c3(GtkWidget *w, gpointer data) {
  xp.v_bg = util_get_color(xp.dlg, xp.v_bg,"Background Color:");
  xp_bg_change();
}

void xp_fmt(GtkToggleButton *w,gpointer data) {
  int i;
  for(i=0;i<3;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xp.fmt[i])))
      xp.v_fmt = i;
  gtk_widget_set_sensitive(xp.quality, xp.v_fmt == 0 ? TRUE : FALSE);
  xp_queue_jpeg_preview();
  refresh(xp.pw);
}

void xp_src(void *args) {
  xp.v_what = dropbox_get_selection(xp.what);
  xp_width(0,0);
  xp_queue_jpeg_preview();
  refresh(xp.pw);
}

void xp_destroy(GtkWidget *w, gpointer data) {
  int i;
  if (xp.img[0] != 0) {
    for(i=0;i<4;i++) {
      CImgDestroy(xp.img[i]);
      xp.img[i] = 0;
    }
    CImgDestroy(xp.buf[0]);
  }
  if (xp.toid >= 0) {
    gtk_timeout_remove(xp.toid);
    xp.toid = -1;
  }
  if (xp.jid >= 0) {
    gtk_timeout_remove(xp.jid);
    xp.jid = -1;
  }
  if (xp.buf[1]) {
    CImgDestroy(xp.buf[1]);
    xp.buf[1] = 0;
  }

}

gboolean xp_repaint(gpointer data) {
  xp_queue_jpeg_preview();
  refresh(xp.pw);
  xp.toid = -1;
  return FALSE;
}

void xp_ok(GtkWidget *w, gpointer data) {
  char name[512];

  if (util_get_filename(xp.dlg, "Save Image", name, 0)) {
    switch(xp.v_fmt) {
    case 0:
      if (xp_save_jpeg(name)!=0) return;
      break;
    case 1:
      if (xp_save_png(name)!=0) return;
      break;
    case 2:
      if (xp.v_gray) {
	if (xp_save_pgm(name)!=0) return;
      } else {
	if (xp_save_ppm(name)!=0) return;
      }
      break;
    }
  }

  gtk_widget_destroy(xp.dlg);
}

void xp_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(xp.dlg);
}

gboolean xp_jpreview(gpointer data) {
  if (xp.buf[1] != 0) {
    CImgDestroy(xp.buf[1]);
    xp.buf[1] = 0;
  }
  xp.buf[1] = CImgPreviewJPEG(xp.img[xp.v_what],xp.v_quality,
			      xp.v_gray,xp.v_tx,&(xp.preview_size));
  refresh(xp.pw);
  xp.jid = -1;
  return FALSE;
}

void xp_queue_jpeg_preview() {
  if (xp.buf[1] != 0) {
    CImgDestroy(xp.buf[1]);
    xp.buf[1] = 0;
  }
  xp.preview_size = -1;
  if (xp.jid >= 0)
    gtk_timeout_remove(xp.jid);
  if (xp.v_fmt == 0)
    xp.jid = gtk_timeout_add(150,xp_jpreview,0);

  // PPM / PGM file size
  if (xp.v_fmt == 2) {    
    if (xp.v_gray)
      xp.preview_size = 16 + xp.v_tx * xp.v_ty;
    else
      xp.preview_size = 16 + 3 * xp.v_tx * xp.v_ty;
  }

}

void xp_quality(GtkRange *w,gpointer data) {
  xp.v_quality = (int) gtk_range_get_value(GTK_RANGE(xp.quality));
  xp_queue_jpeg_preview();
}

void xport_view_dialog() {
  GtkWidget *v,*hb,*ok,*cancel,*h,*vl,*vr,*pl;
  GtkWidget *f[5],*vv[5],*xl,*yl,*wl,*c[4],*hh;
  int i;

  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (xp.v_bg == 1)
    xp.v_bg = view.bg;
  xp.buf[1] = 0;

  for(i=0;i<3;i++)
    xp.img[i] = xport_fake_2D(i);
  xp.img[3] = CImgDup(v3d.scaled);

  xp.buf[0] = CImgDup(xp.img[3]);
  xp.v_tx = xp.img[0]->W;
  xp.v_ty = xp.img[0]->H;

  xp.dlg = util_dialog_new("Export Image",mw,1,&v,&hb,640,480);

  h = gtk_hbox_new(FALSE,4);
  vl = gtk_vbox_new(FALSE,2);
  vr = gtk_vbox_new(FALSE,2);

  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h), vl, FALSE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(h), vr, TRUE, TRUE, 4);

  /* left */

  // what to export
  vv[1] = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vl), vv[1], FALSE, TRUE, 4);
  wl = gtk_label_new("View to Export:");
  xp.what = dropbox_new(";2D View (1);2D View (2);2D View (3);3D View");
  dropbox_set_selection(xp.what, xp.v_what);
  gtk_box_pack_start(GTK_BOX(vv[1]), wl, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vv[1]), xp.what->widget, FALSE, TRUE, 2);

  // frame 0
  f[0] = gtk_frame_new("Format");
  gtk_frame_set_shadow_type(GTK_FRAME(f[0]), GTK_SHADOW_ETCHED_IN);
  vv[0] = gtk_vbox_new(FALSE,2);

  #define RBFW gtk_radio_button_new_with_label_from_widget
  xp.fmt[0] = gtk_radio_button_new_with_label(NULL,"JPEG");
  xp.fmt[1] = RBFW(GTK_RADIO_BUTTON(xp.fmt[0]),"PNG");
  xp.fmt[2] = RBFW(GTK_RADIO_BUTTON(xp.fmt[0]),"PPM / PGM");
  gtk_container_add(GTK_CONTAINER(f[0]),vv[0]);
  for(i=0;i<3;i++)
    gtk_box_pack_start(GTK_BOX(vv[0]), xp.fmt[i], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vl), f[0], FALSE, TRUE, 4);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xp.fmt[xp.v_fmt]),TRUE);
  #undef RBFW

  // frame 1
  f[1] = gtk_frame_new("JPEG Quality (%)");
  gtk_frame_set_shadow_type(GTK_FRAME(f[1]), GTK_SHADOW_ETCHED_IN);
  xp.quality = gtk_hscale_new_with_range(5.0,100.0,1.0);
  gtk_scale_set_digits(GTK_SCALE(xp.quality),0);
  gtk_range_set_value(GTK_RANGE(xp.quality), (gdouble)(xp.v_quality));
  gtk_container_add(GTK_CONTAINER(f[1]), xp.quality);
  gtk_box_pack_start(GTK_BOX(vl), f[1], FALSE, TRUE, 4);

  // frame 2
  f[2] = gtk_frame_new("Size");
  gtk_frame_set_shadow_type(GTK_FRAME(f[2]), GTK_SHADOW_ETCHED_IN);
  vv[2] = gtk_hbox_new(FALSE,2);
  xl = gtk_label_new("Width:");
  yl = gtk_label_new("Height:");
  xp.tx = gtk_spin_button_new_with_range(16.0,2048.0,1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(xp.tx), (gdouble)(xp.v_tx));
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(xp.tx),0);
  xp.ty = gtk_label_new("xxx");
  gtk_box_pack_start(GTK_BOX(vv[2]),xl,FALSE, FALSE,2);
  gtk_box_pack_start(GTK_BOX(vv[2]),xp.tx,FALSE, TRUE,2);
  gtk_box_pack_start(GTK_BOX(vv[2]),yl,FALSE, FALSE,2);
  gtk_box_pack_start(GTK_BOX(vv[2]),xp.ty,FALSE, TRUE,2);
  gtk_container_add(GTK_CONTAINER(f[2]),vv[2]);
  gtk_box_pack_start(GTK_BOX(vl), f[2], FALSE, TRUE, 4);

  // frame 3
  f[3] = gtk_frame_new("Background Color:");
  gtk_frame_set_shadow_type(GTK_FRAME(f[3]), GTK_SHADOW_ETCHED_IN);
  vv[3] = gtk_table_new(2,3,TRUE);
  gtk_container_add(GTK_CONTAINER(f[3]),vv[3]);
  xp.bg = ColorLabelNew(32,16,xp.v_bg,0);
  c[0] = gtk_button_new_with_label("Default");
  c[1] = gtk_button_new_with_label("White");
  c[2] = gtk_button_new_with_label("Black");
  c[3] = gtk_button_new_with_label("Other...");
  gtk_table_set_col_spacings(GTK_TABLE(vv[3]),4);
  gtk_table_set_row_spacings(GTK_TABLE(vv[3]),4);

  gtk_table_attach_defaults(GTK_TABLE(vv[3]),xp.bg->widget,0,1,0,2);
  for(i=0;i<4;i++)
    gtk_table_attach_defaults(GTK_TABLE(vv[3]),c[i],
			      1+(i%2),2+(i%2),0+(i/2),1+(i/2));

  gtk_box_pack_start(GTK_BOX(vl), f[3], FALSE, TRUE, 4);

  gtk_container_set_border_width(GTK_CONTAINER(vv[0]),4);
  gtk_container_set_border_width(GTK_CONTAINER(vv[2]),4);
  gtk_container_set_border_width(GTK_CONTAINER(vv[3]),4);

  xp.togray = gtk_check_button_new_with_label("Generate grayscale image");
  gtk_box_pack_start(GTK_BOX(vl), xp.togray, FALSE, TRUE, 4);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xp.togray),xp.v_gray);

  hh = gtk_hbox_new(FALSE,2);
  xp.qtext = gtk_label_new(" ");
  gtk_box_pack_start(GTK_BOX(hh),xp.qtext,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(vl),hh,FALSE,TRUE,0);
  
  /* right */
  vv[4] = gtk_hbox_new(FALSE, 0);
  pl = gtk_label_new("Sample:");
  gtk_box_pack_start(GTK_BOX(vv[4]),pl,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vr), vv[4], FALSE, FALSE, 2);
  xp.pw = gtk_drawing_area_new();
  gtk_box_pack_start(GTK_BOX(vr), xp.pw, TRUE, TRUE, 2);

  /* bottom */

  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hb),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),cancel,TRUE,TRUE,0);

  gtk_widget_show_all(xp.dlg);

  gtk_signal_connect(GTK_OBJECT(xp.dlg),"destroy",
		     GTK_SIGNAL_FUNC(xp_destroy),0);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(xp_ok),0);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(xp_cancel),0);
  gtk_signal_connect(GTK_OBJECT(xp.pw),"expose_event",
		     GTK_SIGNAL_FUNC(xp_expose),0);
  gtk_signal_connect(GTK_OBJECT(c[0]),"clicked",
		     GTK_SIGNAL_FUNC(xp_c0),0);
  gtk_signal_connect(GTK_OBJECT(c[1]),"clicked",
		     GTK_SIGNAL_FUNC(xp_c1),0);
  gtk_signal_connect(GTK_OBJECT(c[2]),"clicked",
		     GTK_SIGNAL_FUNC(xp_c2),0);
  gtk_signal_connect(GTK_OBJECT(c[3]),"clicked",
		     GTK_SIGNAL_FUNC(xp_c3),0);

  for(i=0;i<3;i++)
    gtk_signal_connect(GTK_OBJECT(xp.fmt[i]),"toggled",
		       GTK_SIGNAL_FUNC(xp_fmt),0);
  gtk_signal_connect(GTK_OBJECT(xp.togray),"toggled",
		     GTK_SIGNAL_FUNC(xp_gray),0);
  gtk_signal_connect(GTK_OBJECT(xp.tx),"value_changed",
		     GTK_SIGNAL_FUNC(xp_width),0);
  gtk_signal_connect(GTK_OBJECT(xp.quality),"value_changed",
		     GTK_SIGNAL_FUNC(xp_quality),0);

  dropbox_set_changed_callback(xp.what,&xp_src);
  gtk_widget_set_sensitive(xp.quality, xp.v_fmt == 0 ? TRUE : FALSE);
  xp_width(0,0);
}

CImg * xport_fake_2D(int i) {
  int bg,fg;
  CImg *dest;
  int w,h,x=0,y=0,rx,ry;
  char buf[2];
  int W,H,w2,h2,ovy;

  v2d_compose();
  bg = xp.v_bg;
  fg = view.fg;
  w = view.bw[i];
  h = view.bh[i];
  dest = CImgNew(w,h);
  
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
  }

  // overlays

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
    ovy = 5;
    if (strlen(patinfo.name)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.name));
      ovy += 12;
    }
    if (strlen(patinfo.age)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.age));
      ovy += 12;
    }
    if (patinfo.gender!=GENDER_U) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    patinfo.gender==GENDER_M?"M":"F");
      ovy += 12;
    }
    if (strlen(patinfo.scandate)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.scandate));
      ovy += 12;
    }
    if (strlen(patinfo.institution)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.institution));
      ovy += 12;
    }
    if (strlen(patinfo.equipment)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.equipment));
      ovy += 12;
    }
    if (strlen(patinfo.protocol)) {
      CImgDrawShieldedSText(dest,res.font[3],8,ovy,0xffffff,0,
			    str2latin1(patinfo.protocol));
      ovy += 12;
    }
  }

  return dest;
}

// png stuff

int CImgWritePNG(CImg *src, char *path) {
  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;
  component **rows;
  int i;

  f = fopen(path, "w");
  if (!f) return -1;
  
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
  if (!png_ptr) {
    fclose(f);
    errno = ENOMEM;
    return -1;
  }
  info_ptr = png_create_info_struct(png_ptr);
  if (!png_ptr) {
    fclose(f);
    errno = ENOMEM;
    return -1;
  }
  setjmp(png_jmpbuf(png_ptr));
  png_init_io(png_ptr, f);

  png_set_IHDR(png_ptr,info_ptr,src->W,src->H,
	       8,PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);

  rows = (component **) MemAlloc(sizeof(component *) * src->H);
  if (!rows) {
    fclose(f);
    errno = ENOMEM;
    return -1;
  }
  for(i=0;i<src->H;i++)
    rows[i] = &(src->val[i*src->rowlen]);

  png_set_rows(png_ptr, info_ptr, rows);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(f);
  return 0;
}

// jpeg stuff

int CImgWriteJPEG(CImg *src, char *path, int quality) {

  /*
    recipe from jpeglib docs

    1. Allocate and initialize a JPEG compression object
    2. Specify the destination for the compressed data (eg, a file)
    3. Set parameters for compression, including image size & colorspace
    4. jpeg_start_compress(...);
    5. while (scan lines remain to be written)
    6. jpeg_write_scanlines(...);
    7. jpeg_finish_compress(...);
    8. Release the JPEG compression object
  */

  int i;
  FILE *f;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  JSAMPROW row_pointer[1];
  int row_stride;

  /* 1 */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  /* 2 */
  f = fopen(path,"w");
  if (!f) return -1;
  jpeg_stdio_dest(&cinfo, f);

  /* 3 */
  cinfo.image_width  = src->W;
  cinfo.image_height = src->H;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&cinfo);

  if (quality < 1)   quality = 1;
  if (quality > 100) quality = 100;
  jpeg_set_quality(&cinfo, quality, TRUE);
  cinfo.dct_method = JDCT_FLOAT;

  /* 4 */
  jpeg_start_compress(&cinfo,TRUE);

  /* 5-6 */
  row_stride = 3 * src->W;
  for(i=0;i<src->H;i++) {
    row_pointer[0] = &(src->val[row_stride * i]);
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* 7 */
  jpeg_finish_compress(&cinfo);

  /* 8 */
  jpeg_destroy_compress(&cinfo);
  fclose(f);
  return 0;
}

CImg * CImgReadJPEG(char *path) {

  /*
    recipe from jpeglib docs

    1. Allocate and initialize a JPEG decompression object
    2. Specify the source of the compressed data (eg, a file)
    3. Call jpeg_read_header() to obtain image info.
    4. Set parameters for decompression.
    5. jpeg_start_decompress(...);
    6. read scanlines
    7. stop decompress
    8. clean up

  */

  CImg *out;
  int i;
  FILE *f;

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  JSAMPROW row_pointer[1];
  int row_stride;

  /* 1 */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  /* 2 */
  f = fopen(path, "r");
  if (!f) return 0;
  jpeg_stdio_src(&cinfo, f);

  /* 3 */
  jpeg_read_header(&cinfo, TRUE);
     
  /* 4 */
  // we're happy with the defaults, thanks

  /* 5 */
  jpeg_start_decompress(&cinfo);
  
  out = CImgNew(cinfo.output_width, cinfo.output_height);
  if (!out) return 0;

  /* 6 */
  row_stride = 3 * out->W;
  for(i=0;i<out->H;i++) {
    row_pointer[0] = &(out->val[row_stride * i]);
    jpeg_read_scanlines(&cinfo, row_pointer, 1);
  }

  /* 7-8 */
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(f);
  return out;
}

CImg *CImgPreviewJPEG(CImg *src,int quality,int gray,int width,int *psize) {
  char nam[512];
  CImg *out,*tmp;
  FILE *f;

  if (tmpnam(nam)==NULL) return 0;
  
  if (width > 0)
    tmp = CImgScaleToWidth(src,width);
  else
    tmp = CImgDup(src);
  if (gray) CImgGray(tmp);

  if (CImgWriteJPEG(tmp,nam,quality)!=0) return 0;
  CImgDestroy(tmp);

  if (psize!=0) {
    f = fopen(nam,"r");
    if (f!=NULL) {
      fseek(f,0L,SEEK_END);
      *psize = (int) ftell(f);
      fclose(f);
    }
  }

  out = CImgReadJPEG(nam);
  unlink(nam);
  return out;
}

int xp_save_jpeg(char *path) {
  CImg *out;
  char msg[512];
  char *p;

  out = CImgScaleToWidth(xp.img[xp.v_what],xp.v_tx);
  if (xp.v_gray) CImgGray(out);

  if (CImgWriteJPEG(out,path,xp.v_quality)!=0) {
    CImgDestroy(out);
    snprintf(msg,512,"Saving image failed:\n%s (%d)",strerror(errno), errno);
    PopMessageBox(xp.dlg,"Error",msg,MSG_ICON_ERROR);
    return -1;
  }

  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"JPEG save to %s successful.",p);
  app_set_status(msg,20);
  CImgDestroy(out);
  return 0;
}

int xp_save_png(char *path) {
  CImg *out;
  char msg[512];
  char *p;

  out = CImgScaleToWidth(xp.img[xp.v_what],xp.v_tx);
  if (xp.v_gray) CImgGray(out);

  if (CImgWritePNG(out,path)!=0) {
    CImgDestroy(out);
    snprintf(msg,512,"Saving image failed:\n%s (%d)",strerror(errno), errno);
    PopMessageBox(xp.dlg,"Error",msg,MSG_ICON_ERROR);
    return -1;
  }

  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"PNG save to %s successful.",p);
  app_set_status(msg,20);
  CImgDestroy(out);
  return 0;
}

int xp_save_ppm(char *path) {
  CImg *out;
  char msg[512];
  char *p;

  out = CImgScaleToWidth(xp.img[xp.v_what],xp.v_tx);

  if (CImgWriteP6(out,path)!=0) {
    CImgDestroy(out);
    snprintf(msg,512,"Saving image failed:\n%s (%d)",strerror(errno), errno);
    PopMessageBox(xp.dlg,"Error",msg,MSG_ICON_ERROR);
    return -1;
  }

  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"PPM save to %s successful.",p);
  app_set_status(msg,20);
  CImgDestroy(out);
  return 0;
}

int xp_save_pgm(char *path) {
  CImg *out;
  char msg[512];
  char *p;

  out = CImgScaleToWidth(xp.img[xp.v_what],xp.v_tx);

  if (CImgWriteP5(out,path)!=0) {
    CImgDestroy(out);
    snprintf(msg,512,"Saving image failed:\n%s (%d)",strerror(errno), errno);
    PopMessageBox(xp.dlg,"Error",msg,MSG_ICON_ERROR);
    return -1;
  }

  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"PGM save to %s successful.",p);
  app_set_status(msg,20);
  CImgDestroy(out);
  return 0;
}

static struct _xbdialog {
  GtkWidget *dlg,*pw;
  DropBox   *what;
  GtkWidget *fmt[3],*quality,*qtext;
  GtkWidget *html;
  GtkWidget *preview;
  gint jid;

  CImg *img[4], *buf;
  char savedir[512], prefix[128];
  int lock;

  int v_what, v_fmt, v_quality, v_html;
} xb = { .v_what = 0, .v_fmt = 0, .v_quality = 100, .v_html = 1,
         .jid = -1, .lock = 0 };

void xb_destroy(GtkWidget *w, gpointer data);
void xb_ok(GtkWidget *w, gpointer data);
void xb_cancel(GtkWidget *w, gpointer data);
void xb_src(void *args);
void xb_fmt(GtkToggleButton *w,gpointer data);
void xb_html(GtkToggleButton *w,gpointer data);
void xb_quality(GtkRange *w,gpointer data);
gboolean xb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data);
gboolean xb_jpreview(gpointer data);
void     xb_queue_jpeg_preview();
void     xb_save(void *args);

void xport_bulk_dialog() {
  GtkWidget *v,*hb,*ok,*cancel;
  GtkWidget *h,*vl,*vr,*vv[3],*wl,*f[2],*pl;
  int i;

  while (xb.lock)
    usleep(1000);

  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  v2d_compose();
  for(i=0;i<4;i++)
    xb.img[i] = CImgDup(v2d.base[i%3]);
  xb.buf = 0;

  xb.dlg = util_dialog_new("Export Slices",mw,1,&v,&hb,600,300);

  strncpy(xb.prefix,"img",128);

  h = gtk_hbox_new(FALSE,4);
  vl = gtk_vbox_new(FALSE,2);
  vr = gtk_vbox_new(FALSE,2);

  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h), vl, FALSE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(h), vr, TRUE, TRUE, 4);

  /* left */

  // what to export
  vv[1] = gtk_hbox_new(FALSE,2);
  gtk_box_pack_start(GTK_BOX(vl), vv[1], FALSE, TRUE, 4);
  wl = gtk_label_new("Orientation to Export:");
  xb.what = dropbox_new(";2D View (1);2D View (2);2D View (3);All");
  dropbox_set_selection(xb.what, xb.v_what);
  gtk_box_pack_start(GTK_BOX(vv[1]), wl, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vv[1]), xb.what->widget, FALSE, TRUE, 2);

  // frame 0
  f[0] = gtk_frame_new("Format");
  gtk_frame_set_shadow_type(GTK_FRAME(f[0]), GTK_SHADOW_ETCHED_IN);
  vv[0] = gtk_vbox_new(FALSE,2);

  #define RBFW gtk_radio_button_new_with_label_from_widget
  xb.fmt[0] = gtk_radio_button_new_with_label(NULL,"JPEG");
  xb.fmt[1] = RBFW(GTK_RADIO_BUTTON(xb.fmt[0]),"PNG");
  xb.fmt[2] = RBFW(GTK_RADIO_BUTTON(xb.fmt[0]),"PPM");
  gtk_container_add(GTK_CONTAINER(f[0]),vv[0]);
  for(i=0;i<3;i++)
    gtk_box_pack_start(GTK_BOX(vv[0]), xb.fmt[i], FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vl), f[0], FALSE, TRUE, 4);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xb.fmt[xb.v_fmt]),TRUE);
  #undef RBFW

  // frame 1
  f[1] = gtk_frame_new("JPEG Quality (%)");
  gtk_frame_set_shadow_type(GTK_FRAME(f[1]), GTK_SHADOW_ETCHED_IN);
  xb.quality = gtk_hscale_new_with_range(5.0,100.0,1.0);
  gtk_scale_set_digits(GTK_SCALE(xb.quality),0);
  gtk_range_set_value(GTK_RANGE(xb.quality), (gdouble)(xb.v_quality));
  gtk_container_add(GTK_CONTAINER(f[1]), xb.quality);
  gtk_box_pack_start(GTK_BOX(vl), f[1], FALSE, TRUE, 4);

  gtk_container_set_border_width(GTK_CONTAINER(vv[0]),4);

  xb.html = gtk_check_button_new_with_label("Generate HTML Index");
  gtk_box_pack_start(GTK_BOX(vl), xb.html, FALSE, TRUE, 4);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xb.html),xb.v_html);
  
  /* right */
  vv[2] = gtk_hbox_new(FALSE, 0);
  pl = gtk_label_new("Sample:");
  gtk_box_pack_start(GTK_BOX(vv[2]),pl,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vr), vv[2], FALSE, FALSE, 2);
  xb.pw = gtk_drawing_area_new();
  gtk_box_pack_start(GTK_BOX(vr), xb.pw, TRUE, TRUE, 2);

  /* bottom */

  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hb),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),cancel,TRUE,TRUE,0);

  gtk_widget_show_all(xb.dlg);

  gtk_signal_connect(GTK_OBJECT(xb.dlg),"destroy",
		     GTK_SIGNAL_FUNC(xb_destroy),0);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(xb_ok),0);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(xb_cancel),0);
  gtk_signal_connect(GTK_OBJECT(xb.pw),"expose_event",
		     GTK_SIGNAL_FUNC(xb_expose),0);

  for(i=0;i<3;i++)
    gtk_signal_connect(GTK_OBJECT(xb.fmt[i]),"toggled",
		       GTK_SIGNAL_FUNC(xb_fmt),0);
  gtk_signal_connect(GTK_OBJECT(xb.html),"toggled",
		     GTK_SIGNAL_FUNC(xb_html),0);
  gtk_signal_connect(GTK_OBJECT(xb.quality),"value_changed",
		     GTK_SIGNAL_FUNC(xb_quality),0);

  dropbox_set_changed_callback(xb.what,&xb_src);
  gtk_widget_set_sensitive(xb.quality, xb.v_fmt == 0 ? TRUE : FALSE);
  xb_queue_jpeg_preview();
}

void xb_destroy(GtkWidget *w, gpointer data) {
  int i;

  if (xb.img[0] != 0) {
    for(i=0;i<4;i++) {
      CImgDestroy(xb.img[i]);
      xb.img[i] = 0;
    }
    if (xb.buf) {
      CImgDestroy(xb.buf);
      xb.buf = 0;
    }
  }
  if (xb.jid >= 0) {
    gtk_timeout_remove(xb.jid);
    xb.jid = -1;
  }
}

void xb_ok(GtkWidget *w, gpointer data) {
  char name[512];

  if (util_get_dir(xb.dlg, "Export Directory", name)) {
    strncpy(xb.savedir, name, 512);
    xb.lock = 1;
    TaskNew(res.bgqueue,"Exporting slices...",(Callback) &xb_save, 0);
  }

  gtk_widget_destroy(xb.dlg);
}

void xb_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(xb.dlg);
}

void xb_src(void *args) {
  xb.v_what = dropbox_get_selection(xb.what);
  xb_queue_jpeg_preview();
  refresh(xb.pw);
}

void xb_fmt(GtkToggleButton *w,gpointer data) {
  int i;
  for(i=0;i<3;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xb.fmt[i])))
      xb.v_fmt = i;
  gtk_widget_set_sensitive(xb.quality, xb.v_fmt == 0 ? TRUE : FALSE);
  xb_queue_jpeg_preview();
  refresh(xb.pw);
}

void xb_html(GtkToggleButton *w,gpointer data) {
  xb.v_html = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xb.html));
}

void xb_quality(GtkRange *w,gpointer data) {
  xb.v_quality = (int) gtk_range_get_value(GTK_RANGE(xb.quality));
  xb_queue_jpeg_preview();
}

gboolean xb_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  CImg *src;

  if (xb.buf) 
    src = xb.buf;
  else
    src = xb.img[xb.v_what];

  gdk_draw_rgb_image(w->window, w->style->black_gc, 
		     0,0,src->W,src->H,GDK_RGB_DITHER_NORMAL,
		     src->val, src->rowlen);
  return FALSE;
}

gboolean xb_jpreview(gpointer data) {
  if (xb.buf != 0) {
    CImgDestroy(xb.buf);
    xb.buf = 0;
  }
  xb.buf = CImgPreviewJPEG(xb.img[xb.v_what],xb.v_quality,0,-1,0);
  refresh(xb.pw);
  xb.jid = -1;
  return FALSE;
}

void     xb_queue_jpeg_preview() {
  if (xb.buf != 0) {
    CImgDestroy(xb.buf);
    xb.buf = 0;
  }
  if (xb.jid >= 0)
    gtk_timeout_remove(xb.jid);
  if (xb.v_fmt == 0)
    xb.jid = gtk_timeout_add(150,xb_jpreview,0);
}

void xb_save(void *args) {
  char name[1024],tname[1024];
  CImg *img,*thumb;
  int i,j,k,p;
  int total=0, cur;
  int *lookup;
  Volume *vol;
  FILE *f;
  static char *ext[3] = { "jpg","png","ppm" };

  MutexLock(voldata.masterlock);
  vol = voldata.original;

  switch(xb.v_what) {
  case 0: total = vol->D; break;
  case 1: total = vol->W; break;
  case 2: total = vol->H; break;
  case 3: total = vol->W + vol->H + vol->D; break;
  }

  cur = 0;
  SetProgress(cur,total);
  lookup = app_calc_lookup();

  if (xb.v_what==0 || xb.v_what==3) {
    img = CImgNew(vol->W,vol->H);
    for(i=0;i<vol->D;i++) {
      for(j=0;j<vol->H;j++)
	for(k=0;k<vol->W;k++) {
	  p = vol->tbframe[i] + vol->tbrow[j] + k;
	  CImgSet(img, k,j, v2d.pal[lookup[vol->u.i16[p]]]);
	}
      snprintf(name,1024,"%s/%s_%d_%.5d",xb.savedir,xb.prefix,1,i+1);
      snprintf(tname,1024,"%s/thumb_%s_%d_%.5d",xb.savedir,xb.prefix,1,i+1);
      p = -1;
      switch(xb.v_fmt) {
      case 0:
	strncat(name,".jpg",1024);
	strncat(tname,".jpg",1024);
	p = CImgWriteJPEG(img,name,xb.v_quality);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWriteJPEG(thumb,tname,MIN(50,xb.v_quality));
	  CImgDestroy(thumb);
	}
	break;	
      case 1:
	strncat(name,".png",1024);
	strncat(tname,".png",1024);
	p = CImgWritePNG(img,name);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWritePNG(thumb,tname);
	  CImgDestroy(thumb);
	}
	break;
      case 2:
	strncat(name,".ppm",1024);
	p = CImgWriteP6(img,name);
	break;
      }
      if (p!=0) {
	snprintf(name,1024,"Saving image failed:\n%s (%d)",strerror(errno), errno);
	app_pop_message("Error",name,MSG_ICON_ERROR);
	CImgDestroy(img);
	xb.lock = 0;
	MutexUnlock(voldata.masterlock);
	return;
      }
      ++cur;
      SetProgress(cur,total);
    }
    CImgDestroy(img);
  }

  if (xb.v_what==1 || xb.v_what==3) {
    img = CImgNew(vol->D,vol->H);
    for(i=0;i<vol->W;i++) {
      for(j=0;j<vol->H;j++)
	for(k=0;k<vol->D;k++) {
	  p = vol->tbframe[k] + vol->tbrow[j] + i;
	  CImgSet(img, k,j, v2d.pal[lookup[vol->u.i16[p]]]);
	}
      snprintf(name,1024,"%s/%s_%d_%.5d",xb.savedir,xb.prefix,2,i+1);
      snprintf(tname,1024,"%s/thumb_%s_%d_%.5d",xb.savedir,xb.prefix,2,i+1);
      p = -1;
      switch(xb.v_fmt) {
      case 0:
	strncat(name,".jpg",1024);
	strncat(tname,".jpg",1024);
	p = CImgWriteJPEG(img,name,xb.v_quality);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWriteJPEG(thumb,tname,MIN(50,xb.v_quality));
	  CImgDestroy(thumb);
	}
	break;	
      case 1:
	strncat(name,".png",1024);
	strncat(tname,".png",1024);
	p = CImgWritePNG(img,name);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWritePNG(thumb,tname);
	  CImgDestroy(thumb);
	}
	break;
      case 2:
	strncat(name,".ppm",1024);
	p = CImgWriteP6(img,name);
	break;
      }
      if (p!=0) {
	snprintf(name,1024,"Saving image failed:\n%s (%d)",strerror(errno), errno);
	app_pop_message("Error",name,MSG_ICON_ERROR);
	CImgDestroy(img);
	xb.lock = 0;
	MutexUnlock(voldata.masterlock);
	return;
      }
      ++cur;
      SetProgress(cur,total);
    }
    CImgDestroy(img);
  }

  if (xb.v_what==2 || xb.v_what==3) {
    img = CImgNew(vol->W,vol->D);
    for(i=0;i<vol->H;i++) {
      for(j=0;j<vol->D;j++)
	for(k=0;k<vol->W;k++) {
	  p = vol->tbframe[j] + vol->tbrow[i] + k;
	  CImgSet(img, k,j, v2d.pal[lookup[vol->u.i16[p]]]);
	}
      snprintf(name,1024,"%s/%s_%d_%.5d",xb.savedir,xb.prefix,3,i+1);
      snprintf(tname,1024,"%s/thumb_%s_%d_%.5d",xb.savedir,xb.prefix,3,i+1);
      p = -1;
      switch(xb.v_fmt) {
      case 0:
	strncat(name,".jpg",1024);
	strncat(tname,".jpg",1024);
	p = CImgWriteJPEG(img,name,xb.v_quality);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWriteJPEG(thumb,tname,MIN(50,xb.v_quality));
	  CImgDestroy(thumb);
	}
	break;	
      case 1:
	strncat(name,".png",1024);
	strncat(tname,".png",1024);
	p = CImgWritePNG(img,name);
	if (xb.v_html) {
	  thumb = CImgScaleToWidth(img,64);
	  p += CImgWritePNG(thumb,tname);
	  CImgDestroy(thumb);
	}
	break;
      case 2:
	strncat(name,".ppm",1024);
	p = CImgWriteP6(img,name);
	break;
      }
      if (p!=0) {
	snprintf(name,1024,"Saving image failed:\n%s (%d)",strerror(errno), errno);
	app_pop_message("Error",name,MSG_ICON_ERROR);
	CImgDestroy(img);
	xb.lock = 0;
	MutexUnlock(voldata.masterlock);
	return;
      }
      ++cur;
      SetProgress(cur,total);
    }
    CImgDestroy(img);
  }

  if (xb.v_html) {
    snprintf(name,1024,"%s/index.html",xb.savedir);
    f = fopen(name,"w");
    if (!f) {
      snprintf(name,1024,"Saving image failed:\n%s (%d)",strerror(errno), errno);
      app_pop_message("Error",name,MSG_ICON_ERROR);
      xb.lock = 0;
      MutexUnlock(voldata.masterlock);
      return;
    }

    fprintf(f,"<html><title>%s</title><frameset cols=\"350,*\"><frame src=\"index_left.html\" name=\"left\"><frame name=\"right\"></frameset></html>\n","Image Index");
    fclose(f);

    snprintf(name,1024,"%s/index_left.html",xb.savedir);
    f = fopen(name,"w");
    if (!f) {
      snprintf(name,1024,"Saving image failed:\n%s (%d)",strerror(errno), errno);
      app_pop_message("Error",name,MSG_ICON_ERROR);
      xb.lock = 0;
      MutexUnlock(voldata.masterlock);
      return;
    }

    fprintf(f,"<html><title>%s</title><body bgcolor=white body=black>\n","Image Index");
    fprintf(f,"<table border=0 cellspacing=2 cellpadding=2>\n");
    
    if (xb.v_what == 0 || xb.v_what == 3) {
      for(i=0;i<vol->D;i++) {
	if (i%4 == 0) fprintf(f,"<tr>");

	fprintf(f,"<td><a href=\"%s_%d_%.5d.%s\" target=\"right\"><img src=\"thumb_%s_%d_%.5d.%s\"></a></td>",
		xb.prefix,1,i+1,ext[xb.v_fmt],
		xb.prefix,1,i+1,ext[xb.v_fmt]);

	if (i%4 == 3) fprintf(f,"</tr>");
      }

      if (vol->D % 4 != 0)
	fprintf(f,"</tr>\n");
    }

    if (xb.v_what == 1 || xb.v_what == 3) {
      for(i=0;i<vol->W;i++) {
	if (i%4 == 0) fprintf(f,"<tr>");

	fprintf(f,"<td><a href=\"%s_%d_%.5d.%s\" target=\"right\"><img src=\"thumb_%s_%d_%.5d.%s\"></a></td>",
		xb.prefix,2,i+1,ext[xb.v_fmt],
		xb.prefix,2,i+1,ext[xb.v_fmt]);

	if (i%4 == 3) fprintf(f,"</tr>");
      }

      if (vol->W % 4 != 0)
	fprintf(f,"</tr>\n");
    }

    if (xb.v_what == 2 || xb.v_what == 3) {
      for(i=0;i<vol->H;i++) {
	if (i%4 == 0) fprintf(f,"<tr>");

	fprintf(f,"<td><a href=\"%s_%d_%.5d.%s\" target=\"right\"><img src=\"thumb_%s_%d_%.5d.%s\"></a></td>",
		xb.prefix,3,i+1,ext[xb.v_fmt],
		xb.prefix,3,i+1,ext[xb.v_fmt]);

	if (i%4 == 3) fprintf(f,"</tr>");
      }

      if (vol->H % 4 != 0)
	fprintf(f,"</tr>\n");
    }

    fprintf(f,"</table>\n");
    fclose(f);
  }

  MutexUnlock(voldata.masterlock);
  EndProgress();
  xb.lock = 0;
}

static struct _volxport {
  char name[512];
} vxp;

void xb_scnsave(void *args);
void xb_anasave(void *args);

void xport_volume(int format) {
  char name[512];
  
  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  if (util_get_filename(mw, format==0?"Export volume as SCN":"Export volume as Analyze", name, 0)) {
    strncpy(vxp.name,name,512);
    switch(format) {
    case 0:
      TaskNew(res.bgqueue,"Exporting volume",(Callback) &xb_scnsave, 0);
      break;
    case 1:
      TaskNew(res.bgqueue,"Exporting volume",(Callback) &xb_anasave, 0);
      break;
    }
  }
}

void xb_scnsave(void *args) {
  Volume *vol;
  FILE *f;
  int i,HD,W;
  char msg[512];
  char *p, *path;

  MutexLock(voldata.masterlock);
  vol = voldata.original;
  SetProgress(0,vol->N);

  f = fopen(vxp.name,"w");
  if (!f) {
    snprintf(msg,512,"Saving volume failed:\n%s (%d)",strerror(errno), errno);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    MutexUnlock(voldata.masterlock);
    return;
  }

  fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n16\n",
	  vol->W,vol->H,vol->D,vol->dx,vol->dy,vol->dz);

  W = vol->W;
  HD = vol->H * vol->D;

  for(i=0;i<HD;i++) {
    if (fwrite(&(vol->u.i16[i*W]),2,W,f)!=W) {
      snprintf(msg,512,"Saving volume failed:\n%s (%d)",strerror(errno), errno);
      app_pop_message("Error",msg,MSG_ICON_ERROR);
      MutexUnlock(voldata.masterlock);
      fclose(f);
      return;
    }
    SetProgress(i,HD);
  }
  
  fclose(f);
  EndProgress();
  MutexUnlock(voldata.masterlock);

  path = vxp.name;
  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"SCN save to %s successful.",p);
  app_set_status(msg,20);
}

void xb_anasave(void *args) {
  Volume *vol;
  FILE *f;
  int i,HD,W,nlen;
  char msg[512];
  char hdrname[512],imgname[512];
  char *p, *path;

  struct _hdr1 {
    int        hdrlen;
    char       data_type[10];
    char       db_name[18];
    int        extents;
    short int  error;
    char       regular;
    char       hkey0;
  } hdr1;

  struct _hdr2 {
    short int  dim[8];
    short int  unused[7];
    short int  data_type;
    short int  bpp;
    short int  dim_un0;
    float      pixdim[8];
    float      zeroes[8];
    int        maxval;
    int        minval;
  } hdr2;

  MutexLock(voldata.masterlock);
  vol = voldata.original;
  SetProgress(0,vol->N);

  nlen = strlen(vxp.name);

  if (nlen>=4 && !strcmp(&vxp.name[nlen-4],".hdr")) {
    strncpy(hdrname,vxp.name,512);
    strncpy(imgname,vxp.name,512);
    imgname[nlen-3] = 'i';
    imgname[nlen-2] = 'm';
    imgname[nlen-1] = 'g';
  } else if (nlen>=4 && !strcmp(&vxp.name[nlen-4],".img")) {
    strncpy(hdrname,vxp.name,512);
    strncpy(imgname,vxp.name,512);
    hdrname[nlen-3] = 'h';
    hdrname[nlen-2] = 'd';
    hdrname[nlen-1] = 'r';
  } else {
    strncpy(hdrname,vxp.name,512);
    strncpy(imgname,vxp.name,512);
    strncat(hdrname,".hdr",512);
    strncat(imgname,".img",512);
  }

  // write img
  f = fopen(imgname,"w");
  if (!f) {
    snprintf(msg,512,"Saving volume failed:\n%s (%d)",strerror(errno), errno);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    MutexUnlock(voldata.masterlock);
    return;
  }

  W = vol->W;
  HD = vol->H * vol->D;

  VolumeYFlip(vol);
  for(i=0;i<HD;i++) {
    if (fwrite(&(vol->u.i16[i*W]),2,W,f)!=W) {
      snprintf(msg,512,"Saving volume failed:\n%s (%d)",strerror(errno), errno);
      app_pop_message("Error",msg,MSG_ICON_ERROR);
      VolumeYFlip(vol);
      MutexUnlock(voldata.masterlock);
      fclose(f);
      return;
    }
    SetProgress(i,HD+1);
  }
  
  VolumeYFlip(vol);
  fclose(f);

  // write hdr
  f = fopen(hdrname,"w");
  if (!f) {
    snprintf(msg,512,"Saving volume failed:\n%s (%d)",strerror(errno), errno);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    MutexUnlock(voldata.masterlock);
    return;
  }

  memset(&hdr1, 0, sizeof(hdr1));
  memset(&hdr2, 0, sizeof(hdr2));

  for(i=0;i<8;i++) {
    hdr2.pixdim[i] = 0.0;
    hdr2.zeroes[i] = 0.0;
  }

  /* -- first header segment -- */

  hdr1.hdrlen  = 348;
  hdr1.regular = 'r';

  fio_abs_write_32(f, 0, 0, hdr1.hdrlen);
  fio_abs_write_zeroes(f, 4, 34);
  fio_abs_write_8(f, 38, hdr1.regular);
  fio_abs_write_8(f, 39, hdr1.hkey0);

  /* -- second header segment -- */

  hdr2.dim[0] = 4;
  hdr2.dim[1] = vol->W;
  hdr2.dim[2] = vol->H;
  hdr2.dim[3] = vol->D;
  hdr2.dim[4] = 1;

  hdr2.data_type = 4;
  hdr2.bpp       = 16;
  hdr2.pixdim[0] = 1.0;
  hdr2.pixdim[1] = vol->dx;
  hdr2.pixdim[2] = vol->dy;
  hdr2.pixdim[3] = vol->dz;
  hdr2.maxval    = VolumeMax(vol);
  hdr2.minval    = 0;

  for(i=0;i<8;i++)
    fio_abs_write_16(f, 40 + 0  + 2*i, 0, hdr2.dim[i]);
  for(i=0;i<7;i++)
    fio_abs_write_16(f, 40 + 16 + 2*i, 0, hdr2.unused[i]);

  fio_abs_write_16(f, 40 + 30, 0, hdr2.data_type);
  fio_abs_write_16(f, 40 + 32, 0, hdr2.bpp);
  fio_abs_write_16(f, 40 + 34, 0, hdr2.dim_un0);

  for(i=0;i<8;i++)
    fio_abs_write_float32(f, 40 + 36 + 4*i, 0, hdr2.pixdim[i]);
  for(i=0;i<8;i++)
    fio_abs_write_float32(f, 40 + 68 + 4*i, 0, hdr2.zeroes[i]);
 
  fio_abs_write_32(f, 40 + 100, 0, hdr2.maxval);
  fio_abs_write_32(f, 40 + 104, 0, hdr2.minval);

  /* -- third header segment (patient info) --- */

  fio_abs_write_zeroes(f, 148, 200);
  fclose(f);

  EndProgress();
  MutexUnlock(voldata.masterlock);

  path = hdrname;
  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"Analyze save to %s successful.",p);
  app_set_status(msg,20);
}
