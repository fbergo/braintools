
#define SOURCE_ORIENT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libip.h>
#include <gtk/gtk.h>
#include "aftervoxel.h"

#include "xpm/hflip.xpm"
#include "xpm/vflip.xpm"
#include "xpm/orot.xpm"

const char OriKeys[10] = "SCAUBLRAP";

OrientData orient;

void orient_init() {
  orient.main = ORIENT_UNKNOWN;
  orient.zlo  = ORIENT_UNKNOWN;
  orient.zhi  = ORIENT_UNKNOWN;
  orient.mainorder[0] = ORIENT_UNKNOWN;
  orient.mainorder[1] = ORIENT_UNKNOWN;
  orient.mainorder[2] = ORIENT_UNKNOWN;
  orient.mainorder[3] = ORIENT_UNKNOWN;
}

static struct _odlg {
  GtkWidget *dlg;
  GtkWidget *c[3],*b[9];
  CImg *buf[3];
  DropBox *mo;
  OrientData old;
  int nops, ops[128];
} ori;

gboolean ori_expose(GtkWidget *w,GdkEventExpose *ee,gpointer data) {
  int i=-1;
  int W,H,w2,h2;
  CImg *tmp;
  char buf[2];

  if (w==ori.c[0]) i = 0;
  if (w==ori.c[1]) i = 1;
  if (w==ori.c[2]) i = 2;
  if (i<0) return FALSE;

  tmp = CImgDup(ori.buf[i]);

  if (orient.main != ORIENT_UNKNOWN) {
    W = ori.buf[i]->W;
    H = ori.buf[i]->H;
    w2 = W/2;
    h2 = H/2;
    
    buf[1] = 0;
    switch(i) {
    case 0:
      buf[0] = OriKeys[orient.mainorder[0]];
      CImgDrawShieldedSText(tmp,res.font[5],w2,5,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[2]];
      CImgDrawShieldedSText(tmp,res.font[5],w2,H-20,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[3]];
      CImgDrawShieldedSText(tmp,res.font[5],5,h2,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[1]];
      CImgDrawShieldedSText(tmp,res.font[5],W-20,h2,0xffff00,0,buf);
      break;
    case 1:
      buf[0] = OriKeys[orient.mainorder[0]];
      CImgDrawShieldedSText(tmp,res.font[5],w2,5,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[2]];
      CImgDrawShieldedSText(tmp,res.font[5],w2,H-20,0xffff00,0,buf);
      buf[0] = OriKeys[orient.zlo];
      CImgDrawShieldedSText(tmp,res.font[5],5,h2,0xffff00,0,buf);
      buf[0] = OriKeys[orient.zhi];
      CImgDrawShieldedSText(tmp,res.font[5],W-20,h2,0xffff00,0,buf);
      break;      
    case 2:
      buf[0] = OriKeys[orient.zlo];
      CImgDrawShieldedSText(tmp,res.font[5],w2,5,0xffff00,0,buf);
      buf[0] = OriKeys[orient.zhi];
      CImgDrawShieldedSText(tmp,res.font[5],w2,H-20,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[3]];
      CImgDrawShieldedSText(tmp,res.font[5],5,h2,0xffff00,0,buf);
      buf[0] = OriKeys[orient.mainorder[1]];
      CImgDrawShieldedSText(tmp,res.font[5],W-20,h2,0xffff00,0,buf);
      break;
    }
  }

  gdk_draw_rgb_image(w->window,w->style->black_gc,
		     0,0,tmp->W,tmp->H,
		     GDK_RGB_DITHER_NORMAL,
		     tmp->val,tmp->rowlen);
  CImgDestroy(tmp);
  return FALSE;
}

void ori_destroy(GtkWidget *w,gpointer data) {
  CImgDestroy(ori.buf[0]);
  CImgDestroy(ori.buf[1]);
  CImgDestroy(ori.buf[2]);
  MemCpy(&orient,&(ori.old),sizeof(OrientData));
}

void orient_bg(void *args) {
  int h=0,v=0,r=0;
  int i,j,k;
  i16_t t16;
  i8_t t8;
  Volume *v16,*v8;

  int W,H,D;

  for(i=0;i<ori.nops;i++)
    switch(ori.ops[i]) {
    case 1: h++; break;
    case 2: v++; break;
    case 3: r++; break;
    }

  h %= 2;
  v %= 2;
  r %= 4;

  MutexLock(voldata.masterlock);

  W= voldata.original->W;
  H= voldata.original->H;
  D= voldata.original->D;

  if (h) {
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W/2;i++) {
	  t16 = VolumeGetI16(voldata.original,i,j,k);
	  VolumeSetI16(voldata.original,i,j,k,
		       VolumeGetI16(voldata.original,W-i-1,j,k));
	  VolumeSetI16(voldata.original,W-i-1,j,k,t16);

	  t8 = VolumeGetI8(voldata.label,i,j,k);
	  VolumeSetI8(voldata.label,i,j,k,
		       VolumeGetI8(voldata.label,W-i-1,j,k));
	  VolumeSetI8(voldata.label,W-i-1,j,k,t8);	  
	}
  }

  if (v) {
    for(k=0;k<D;k++)
      for(j=0;j<H/2;j++)
	for(i=0;i<W;i++) {
	  t16 = VolumeGetI16(voldata.original,i,j,k);
	  VolumeSetI16(voldata.original,i,j,k,
		       VolumeGetI16(voldata.original,i,H-j-1,k));
	  VolumeSetI16(voldata.original,i,H-j-1,k,t16);

	  t8 = VolumeGetI8(voldata.label,i,j,k);
	  VolumeSetI8(voldata.label,i,j,k,
		      VolumeGetI8(voldata.label,i,H-j-1,k));
	  VolumeSetI8(voldata.label,i,H-j-1,k,t8);	  
	}
  }

  while(r) {
    W = voldata.original->W;
    H = voldata.original->H;
    D = voldata.original->D;
    v16 = VolumeNew(H,W,D,vt_integer_16);
    for(k=0;k<D;k++)
      for(j=0;j<W;j++)
	for(i=0;i<H;i++)
	  VolumeSetI16(v16,i,j,k,
		       VolumeGetI16(voldata.original,j,H-i-1,k));
    v8 = voldata.original;
    voldata.original = v16;
    VolumeDestroy(v8);

    v8 = VolumeNew(H,W,D,vt_integer_8);
    for(k=0;k<D;k++)
      for(j=0;j<W;j++)
	for(i=0;i<H;i++)
	  VolumeSetI8(v8,i,j,k,
		       VolumeGetI8(voldata.label,j,H-i-1,k));
    v16 = voldata.label;
    voldata.label = v8;
    VolumeDestroy(v16);
    r--;
  }

  notify.segchanged++;
  MutexUnlock(voldata.masterlock);
  view3d_invalidate(1,1);
}

void ori_ok(GtkWidget *w,gpointer data) {
  MemCpy(&(ori.old),&orient,sizeof(OrientData));
  gtk_widget_destroy(ori.dlg);

  if (ori.nops > 0) {
    undo_reset();
    undo_update();
    measure_kill_all();
    TaskNew(res.bgqueue,"Applying New Orientation...",(Callback) &orient_bg, 0);
  }
  force_redraw_2D();
}

void ori_cancel(GtkWidget *w,gpointer data) {
  gtk_widget_destroy(ori.dlg);
}

void ori_change(GtkWidget *w,gpointer data) {
  int i = *((int *)data);
  int t;

  switch(i) {
  case 0: // hflip 0
  case 6: // hflip 2
    t = orient.mainorder[1];
    orient.mainorder[1] = orient.mainorder[3];
    orient.mainorder[3] = t;
    break;
  case 1: // vflip 0
  case 4: // vflip 1
    t = orient.mainorder[0];
    orient.mainorder[0] = orient.mainorder[2];
    orient.mainorder[2] = t;
    break;
  case 3: // hflip 1
  case 7: // vflip 2
    t = orient.zlo;
    orient.zlo = orient.zhi;
    orient.zhi = t;
    break;
  case 2: // rot 0
    t = orient.mainorder[0];
    orient.mainorder[0] = orient.mainorder[3];
    orient.mainorder[3] = orient.mainorder[2];
    orient.mainorder[2] = orient.mainorder[1];
    orient.mainorder[1] = t;
    break;
  }
  refresh(ori.c[0]);
  refresh(ori.c[1]);
  refresh(ori.c[2]);
}

void ori_mainchange(void *args) {
  int i,j;
  j = dropbox_get_selection(ori.mo);
  for(i=0;i<9;i++)
    gtk_widget_set_sensitive(ori.b[i],j==3?FALSE:TRUE);
  if (j==3) j=99;
  orient.main = j;
  switch(j) {
  case ORIENT_SAGGITAL:
    orient.mainorder[0] = ORIENT_UPPER;
    orient.mainorder[1] = ORIENT_POSTERIOR;
    orient.mainorder[2] = ORIENT_LOWER;
    orient.mainorder[3] = ORIENT_ANTERIOR;
    orient.zlo = ORIENT_LEFT;
    orient.zhi = ORIENT_RIGHT;
    break;
  case ORIENT_CORONAL:
    orient.mainorder[0] = ORIENT_UPPER;
    orient.mainorder[1] = ORIENT_RIGHT;
    orient.mainorder[2] = ORIENT_LOWER;
    orient.mainorder[3] = ORIENT_LEFT;
    orient.zlo = ORIENT_ANTERIOR;
    orient.zhi = ORIENT_POSTERIOR;
    break;
  case ORIENT_AXIAL:
    orient.mainorder[0] = ORIENT_ANTERIOR;
    orient.mainorder[1] = ORIENT_RIGHT;
    orient.mainorder[2] = ORIENT_POSTERIOR;
    orient.mainorder[3] = ORIENT_LEFT;
    orient.zlo = ORIENT_UPPER;
    orient.zhi = ORIENT_LOWER;
    break;
  }
  refresh(ori.c[0]);
  refresh(ori.c[1]);
  refresh(ori.c[2]);
}

void ori_hflip(GtkWidget *w,gpointer data) {
  int i,j,W,H,t;

  t = orient.mainorder[1];
  orient.mainorder[1] = orient.mainorder[3];
  orient.mainorder[3] = t;

  W = ori.buf[0]->W;
  H = ori.buf[0]->H;
  for(i=0;i<W/2;i++) {
    for(j=0;j<H;j++) {
      t = CImgGet(ori.buf[0],i,j);
      CImgSet(ori.buf[0],i,j,CImgGet(ori.buf[0],W-1-i,j));
      CImgSet(ori.buf[0],W-1-i,j,t);
    }
  }

  H = ori.buf[2]->H;
  for(i=0;i<W/2;i++) {
    for(j=0;j<H;j++) {
      t = CImgGet(ori.buf[2],i,j);
      CImgSet(ori.buf[2],i,j,CImgGet(ori.buf[2],W-1-i,j));
      CImgSet(ori.buf[2],W-1-i,j,t);
    }
  }

  ori.ops[ori.nops++] = 1;

  refresh(ori.c[0]);
  refresh(ori.c[1]);
  refresh(ori.c[2]);
}

void ori_vflip(GtkWidget *w,gpointer data) {
  int i,j,W,H,t;

  t = orient.mainorder[0];
  orient.mainorder[0] = orient.mainorder[2];
  orient.mainorder[2] = t;

  W = ori.buf[0]->W;
  H = ori.buf[0]->H;
  for(i=0;i<H/2;i++) {
    for(j=0;j<W;j++) {
      t = CImgGet(ori.buf[0],j,i);
      CImgSet(ori.buf[0],j,i,CImgGet(ori.buf[0],j,H-1-i));
      CImgSet(ori.buf[0],j,H-1-i,t);
    }
  }

  W = ori.buf[1]->W;
  for(i=0;i<H/2;i++) {
    for(j=0;j<W;j++) {
      t = CImgGet(ori.buf[1],j,i);
      CImgSet(ori.buf[1],j,i,CImgGet(ori.buf[1],j,H-1-i));
      CImgSet(ori.buf[1],j,H-1-i,t);
    }
  }

  ori.ops[ori.nops++] = 2;

  refresh(ori.c[0]);
  refresh(ori.c[1]);
  refresh(ori.c[2]);
}

CImg * img_r90(CImg *src) {
  CImg *dest;
  int i,j;
  dest = CImgNew(src->H,src->W);
  for(i=0;i<src->H;i++)
    for(j=0;j<src->W;j++)
      CImgSet(dest,i,j,CImgGet(src,j,src->H-i-1));
  return dest;
}

void ori_rot(GtkWidget *w,gpointer data) {
  int i,t;
  CImg *r90, *t90;

  t = orient.mainorder[0];
  orient.mainorder[0] = orient.mainorder[3];
  orient.mainorder[3] = orient.mainorder[2];
  orient.mainorder[2] = orient.mainorder[1];
  orient.mainorder[1] = t;

  r90 = img_r90(ori.buf[0]);
  CImgDestroy(ori.buf[0]);
  ori.buf[0] = r90;

  r90 = img_r90(ori.buf[1]);
  t90 = img_r90(ori.buf[2]);
  CImgDestroy(ori.buf[1]);
  CImgDestroy(ori.buf[2]);
  ori.buf[1] = t90;
  ori.buf[2] = r90;

  for(i=0;i<3;i++)
    gtk_drawing_area_size(GTK_DRAWING_AREA(ori.c[i]),
			  ori.buf[i]->W,ori.buf[i]->H);
  
  ori.ops[ori.nops++] = 3;

  refresh(ori.c[0]);
  refresh(ori.c[1]);
  refresh(ori.c[2]);
}


void orient_dialog() {
  GtkWidget *v,*hb,*t,*ok,*cancel;
  GtkWidget *h[3],*f1[3],*f2[3],*r[3],*l,*pl,*imf1,*imf2,*imr,*lf,*lh;
  GtkTooltips *tt;
  GtkWidget *vo,*voh;
  int i;

  static int values[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  while(!v2d.base[0]) {
    usleep(5000);
  }

  MemCpy(&(ori.old),&orient,sizeof(OrientData));

  ori.nops=0;
  ori.buf[0] = CImgDup(v2d.base[0]);
  ori.buf[1] = CImgDup(v2d.base[1]);
  ori.buf[2] = CImgDup(v2d.base[2]);

  ori.dlg = util_dialog_new("Orientation",mw,1,&v,&hb,-1,-1);
  tt = gtk_tooltips_new();

  lf = gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(lf),GTK_SHADOW_ETCHED_IN);
  lh = gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(lh),6);

  pl = gtk_label_new("Use the controls to adjust L/R/T/B."); // HERE
  gtk_box_pack_start(GTK_BOX(v),lf,FALSE,TRUE,2);
  gtk_container_add(GTK_CONTAINER(lf),lh);
  gtk_box_pack_start(GTK_BOX(lh),pl,FALSE,FALSE,0);

  t = gtk_table_new(4,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(t),6);
  gtk_table_set_col_spacings(GTK_TABLE(t),6);
  gtk_box_pack_start(GTK_BOX(v),t,TRUE,TRUE,0);

  for(i=0;i<3;i++) {
    ori.c[i] = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(ori.c[i]),
			  ori.buf[i]->W,ori.buf[i]->H);
    h[i] = gtk_hbox_new(FALSE,0);
    f1[i] = util_pixbutton_new(hflip_xpm);
    f2[i] = util_pixbutton_new(vflip_xpm);
    r[i] = util_pixbutton_new(orot_xpm);
    gtk_box_pack_start(GTK_BOX(h[i]),f1[i],FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(h[i]),f2[i],FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(h[i]),r[i],FALSE,TRUE,0);

    gtk_tooltips_set_tip(tt, f1[i], "Horizontal Flip", 0);
    gtk_tooltips_set_tip(tt, f2[i], "Vertical Flip", 0);
    gtk_tooltips_set_tip(tt, r[i],  "Rotate", 0);

    gtk_signal_connect(GTK_OBJECT(f1[i]),"clicked",
		       GTK_SIGNAL_FUNC(ori_change),
		       &(values[3*i+0]));
    gtk_signal_connect(GTK_OBJECT(f2[i]),"clicked",
		       GTK_SIGNAL_FUNC(ori_change),
		       &(values[3*i+1]));
    gtk_signal_connect(GTK_OBJECT(r[i]),"clicked",
		       GTK_SIGNAL_FUNC(ori_change),
		       &(values[3*i+2]));
  }

  ori.b[0] = f1[0];
  ori.b[1] = f2[0];
  ori.b[2] = r[0];
  ori.b[3] = f1[1];
  ori.b[4] = f2[1];
  ori.b[5] = r[1];
  ori.b[6] = f1[2];
  ori.b[7] = f2[2];
  ori.b[8] = r[2];

  l = gtk_label_new("View:");
  ori.mo = dropbox_new(";Saggittal;Coronal;Axial;No Info"); // HERE
  gtk_box_pack_start(GTK_BOX(h[0]),ori.mo->widget,FALSE,TRUE,4);
  dropbox_set_selection(ori.mo,MIN(3,orient.main));

  dropbox_set_changed_callback(ori.mo,&ori_mainchange);

  gtk_table_attach_defaults(GTK_TABLE(t),ori.c[0],0,1,0,1);
  gtk_table_attach_defaults(GTK_TABLE(t),ori.c[1],1,2,0,1);
  gtk_table_attach_defaults(GTK_TABLE(t),ori.c[2],0,1,2,3);
  gtk_table_attach_defaults(GTK_TABLE(t),h[0],0,1,1,2);
  gtk_table_attach_defaults(GTK_TABLE(t),h[1],1,2,1,2);
  gtk_table_attach_defaults(GTK_TABLE(t),h[2],0,1,3,4);

  vo = gtk_frame_new("Volume Operations:");
  gtk_frame_set_shadow_type(GTK_FRAME(vo),GTK_SHADOW_ETCHED_IN);
  voh = gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(voh),8);
  imf1 = util_pixbutton_new(hflip_xpm);
  imf2 = util_pixbutton_new(vflip_xpm);
  imr = util_pixbutton_new(orot_xpm);

  gtk_tooltips_set_tip(tt, imf1, "Horizontal Flip", 0);
  gtk_tooltips_set_tip(tt, imf2, "Vertical Flip", 0);
  gtk_tooltips_set_tip(tt, imr,  "Rotate", 0);

  gtk_table_attach(GTK_TABLE(t),vo,1,2,2,4,TRUE,TRUE,0,0);
  gtk_container_add(GTK_CONTAINER(vo),voh);
  gtk_box_pack_start(GTK_BOX(voh),imf1,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(voh),imf2,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(voh),imr,FALSE,FALSE,0);
  
  gtk_signal_connect(GTK_OBJECT(imf1),"clicked",
		     GTK_SIGNAL_FUNC(ori_hflip),0);
  gtk_signal_connect(GTK_OBJECT(imf2),"clicked",
		     GTK_SIGNAL_FUNC(ori_vflip),0);
  gtk_signal_connect(GTK_OBJECT(imr),"clicked",
		     GTK_SIGNAL_FUNC(ori_rot),0);

  ok = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");

  gtk_box_pack_start(GTK_BOX(hb),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),cancel,TRUE,TRUE,0);

  gtk_widget_show_all(ori.dlg);
  gtk_widget_hide(ori.b[5]);
  gtk_widget_hide(ori.b[8]);

  gtk_signal_connect(GTK_OBJECT(ori.dlg),"destroy",
		     GTK_SIGNAL_FUNC(ori_destroy),0);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(ori_ok),0);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(ori_cancel),0);
  gtk_signal_connect(GTK_OBJECT(ori.c[0]),"expose_event",
		     GTK_SIGNAL_FUNC(ori_expose),0);
  gtk_signal_connect(GTK_OBJECT(ori.c[1]),"expose_event",
		     GTK_SIGNAL_FUNC(ori_expose),0);
  gtk_signal_connect(GTK_OBJECT(ori.c[2]),"expose_event",
		     GTK_SIGNAL_FUNC(ori_expose),0);
  ori_mainchange(0);
}
