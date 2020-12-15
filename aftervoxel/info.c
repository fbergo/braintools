
#define SOURCE_INFO_C 1

#pragma GCC diagnostic ignored "-Wstringop-overflow"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include <math.h>
#include "aftervoxel.h"

PatientInfo patinfo;

void info_destroy(GtkWidget *w,gpointer data);
void info_ok(GtkWidget *w,gpointer data);
void info_cancel(GtkWidget *w,gpointer data);

static struct _idlg {
  GtkWidget *w;
  GtkEntry *e[6];  
  DropBox *gender;
  GtkWidget *notes;
} idlg;

void info_init() {
  patinfo.name[0] = 0;
  patinfo.age[0]  = 0;
  patinfo.gender  = GENDER_U;
  patinfo.scandate[0] = 0;
  patinfo.institution[0] = 0;
  patinfo.equipment[0] = 0;
  patinfo.protocol[0] = 0;
  patinfo.notes = 0;
  patinfo.noteslen = 0;
}

void info_reset() {
  if (patinfo.notes) {
    MemFree(patinfo.notes);
    patinfo.notes = 0;
    patinfo.noteslen = 0;
  }
  info_init();
}

void info_destroy(GtkWidget *w,gpointer data) {
  idlg.w = 0;
}

void info_ok(GtkWidget *w, gpointer data) {
  GtkTextBuffer *tb;
  GtkTextIter i1,i2;
  char *t;

  app_tdebug(5000);

  strncpy(patinfo.name, gtk_entry_get_text(idlg.e[0]), 127);
  strncpy(patinfo.age,  gtk_entry_get_text(idlg.e[1]), 15);
  patinfo.gender = (char) dropbox_get_selection(idlg.gender);
  strncpy(patinfo.scandate,  gtk_entry_get_text(idlg.e[2]), 31);
  strncpy(patinfo.institution,  gtk_entry_get_text(idlg.e[3]), 127);
  strncpy(patinfo.equipment,  gtk_entry_get_text(idlg.e[4]), 127);
  strncpy(patinfo.protocol,  gtk_entry_get_text(idlg.e[5]), 127);

  tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(idlg.notes));
  gtk_text_buffer_get_bounds(tb,&i1,&i2);
  t = gtk_text_buffer_get_text(tb,&i1,&i2,TRUE);

  if (strlen(t) >= patinfo.noteslen) {
    if (patinfo.notes != 0) {
      MemFree(patinfo.notes);
      patinfo.notes = 0;
      patinfo.noteslen = 0;
    }
    if (!patinfo.notes) {
      patinfo.noteslen = strlen(t) + 1;
      patinfo.notes = (char *) MemAlloc(patinfo.noteslen);
    }
  }
  strncpy(patinfo.notes, t, patinfo.noteslen);
  g_free(t);

  patinfo.name[127]=0;
  patinfo.age[15]=0;
  patinfo.scandate[31]=0;
  patinfo.institution[127]=0;
  patinfo.equipment[127]=0;
  patinfo.protocol[127]=0;

  gtk_widget_destroy(idlg.w);
  force_redraw_2D();
}

void info_cancel(GtkWidget *w, gpointer data) {
  app_tdebug(5001);
  gtk_widget_destroy(idlg.w);
}

void info_dialog() {
  GtkWidget *v,*bh,*ok,*cancel,*hs,*f,*sw;
  GtkWidget *h[7],*l[8];
  GtkTextBuffer *tb;
  int i;

  app_tdebug(5002);

  idlg.w = util_dialog_new("Patient Info",mw,1,&v,&bh,450,400);
  for(i=0;i<7;i++)
    h[i] = gtk_hbox_new(FALSE,4);

  l[0] = gtk_label_new("Name:");
  l[1] = gtk_label_new("Age:");
  l[2] = gtk_label_new("Gender:");
  l[3] = gtk_label_new("Scan Date:");
  l[4] = gtk_label_new("Institution:");
  l[5] = gtk_label_new("Equipment:");
  l[6] = gtk_label_new("Protocol:");
  l[7] = gtk_label_new("Comments:");

  for(i=0;i<6;i++)
    idlg.e[i] = GTK_ENTRY(gtk_entry_new());
   gtk_entry_set_max_length(idlg.e[0], 128);
   gtk_entry_set_max_length(idlg.e[1], 16);
   gtk_entry_set_max_length(idlg.e[2], 32);
   gtk_entry_set_max_length(idlg.e[3], 128);
   gtk_entry_set_max_length(idlg.e[4], 128);
   gtk_entry_set_max_length(idlg.e[5], 128);

   idlg.gender = dropbox_new(";Male;Female;None");

   // line 1 name
   gtk_box_pack_start(GTK_BOX(v), h[0], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[0]), l[0], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[0]), GTK_WIDGET(idlg.e[0]), TRUE, TRUE, 2);

   // line 2 age and gender
   gtk_box_pack_start(GTK_BOX(v), h[1], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[1]), l[1], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[1]), GTK_WIDGET(idlg.e[1]), TRUE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[1]), l[2], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[1]), idlg.gender->widget, FALSE, TRUE, 2);

   // line 3 scandate
   gtk_box_pack_start(GTK_BOX(v), h[2], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[2]), l[3], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[2]), GTK_WIDGET(idlg.e[2]), TRUE, TRUE, 2);

   // line 4 institution
   gtk_box_pack_start(GTK_BOX(v), h[3], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[3]), l[4], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[3]), GTK_WIDGET(idlg.e[3]), TRUE, TRUE, 2);

   // line 5 equip
   gtk_box_pack_start(GTK_BOX(v), h[4], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[4]), l[5], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[4]), GTK_WIDGET(idlg.e[4]), TRUE, TRUE, 2);

   // line 6 protocol/sequence
   gtk_box_pack_start(GTK_BOX(v), h[5], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[5]), l[6], FALSE, TRUE, 2);
   gtk_box_pack_start(GTK_BOX(h[5]), GTK_WIDGET(idlg.e[5]), TRUE, TRUE, 2);

   // line 7 notes title
   gtk_box_pack_start(GTK_BOX(v), h[6], FALSE, TRUE, 4);
   gtk_box_pack_start(GTK_BOX(h[6]), l[7], FALSE, TRUE, 2);

   // line 8 notes content
   f = gtk_frame_new(0);
   gtk_frame_set_shadow_type(GTK_FRAME(f), GTK_SHADOW_IN);
   sw = gtk_scrolled_window_new(0,0);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
   idlg.notes = gtk_text_view_new();
   gtk_box_pack_start(GTK_BOX(v), f, TRUE, TRUE, 2);
   gtk_container_add(GTK_CONTAINER(f), sw);
   gtk_container_add(GTK_CONTAINER(sw), idlg.notes);

   hs = gtk_hseparator_new();
   gtk_box_pack_start(GTK_BOX(v), hs, FALSE, TRUE, 4);

   ok     = gtk_button_new_with_label("Ok");
   cancel = gtk_button_new_with_label("Cancel");
   gtk_box_pack_start(GTK_BOX(bh), ok, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(bh), cancel, TRUE, TRUE, 0);

   // fill contents

   gtk_entry_set_text(idlg.e[0], patinfo.name);
   gtk_entry_set_text(idlg.e[1], patinfo.age);
   gtk_entry_set_text(idlg.e[2], patinfo.scandate);
   gtk_entry_set_text(idlg.e[3], patinfo.institution);
   gtk_entry_set_text(idlg.e[4], patinfo.equipment);
   gtk_entry_set_text(idlg.e[5], patinfo.protocol);
   dropbox_set_selection(idlg.gender, patinfo.gender);

   if (patinfo.notes != 0) {
     tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(idlg.notes));
     gtk_text_buffer_set_text(tb, patinfo.notes, -1);
   }

   gtk_signal_connect(GTK_OBJECT(idlg.w), "destroy",
		      GTK_SIGNAL_FUNC(info_destroy), 0);
   gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		      GTK_SIGNAL_FUNC(info_ok), 0);
   gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		      GTK_SIGNAL_FUNC(info_cancel), 0);

   gtk_widget_show_all(idlg.w);
}

GtkWidget *vidlg;

void volinfo_ok(GtkWidget *w, gpointer data) {
  app_tdebug(5003);
  gtk_widget_destroy(vidlg);
}

void volinfo_dialog() {
  GtkWidget *dlg,*v,*bh,*l,*ok;
  char tmp[2048],aux[256];
  int i;
  i16_t L,U;

  app_tdebug(5004);

  if (voldata.original == NULL) {
    warn_no_volume();
    return;
  }

  vidlg = dlg = util_dialog_new("Volume Properties",mw,1,&v,&bh,-1,-1);

  snprintf(aux,256,"Original Volume Dimension: %d x %d x %d voxels",
	   voldata.original->W,
	   voldata.original->H,
	   voldata.original->D);
  strncpy(tmp,aux,2048); strncat(tmp,"\n",2048);
  snprintf(aux,256,"Original Voxel Size: %.2f x %.2f x %.2f mm",
	   voldata.original->dx,
	   voldata.original->dy,
	   voldata.original->dz);
  strncat(tmp,aux,2048); strncat(tmp,"\n",2048);

  L=U=voldata.original->u.i16[0];
  for(i=1;i<voldata.original->N;i++) {
    L = MIN(L,voldata.original->u.i16[i]);
    U = MAX(U,voldata.original->u.i16[i]);
  }

  snprintf(aux,256,"Intensity Range: %d - %d",L,U);
  strncat(tmp,aux,2048); strncat(tmp,"\n\n",2048);

  l = gtk_label_new(tmp);
  gtk_box_pack_start(GTK_BOX(v),l,TRUE,TRUE,4);
  ok = gtk_button_new_with_label("Ok");
  gtk_box_pack_start(GTK_BOX(bh), ok, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(volinfo_ok),0);
  gtk_widget_show_all(dlg);
}

struct _hdlg {
  GtkWidget *dlg, *canvas, *logscale, *objects, *outline, *xrg, *yrg;
  int *histdata;
  int maxhist, hsz;
  int user_range[4], full_range[4];
  int *objdata[MAXLABELS+1];
} hdlg;

void     histogram_destroy(GtkWidget *w,gpointer data);
void     histogram_close(GtkWidget *w,gpointer data);
gboolean histogram_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
void     histogram_toggle(GtkToggleButton *w,gpointer data);
void     histogram_update();

void histogram_destroy(GtkWidget *w,gpointer data) {
  int i;
  if (hdlg.histdata != NULL) {
    free(hdlg.histdata);
    for(i=0;i<=MAXLABELS;i++)
      free(hdlg.objdata[i]);
  }
  memset(&hdlg,0,sizeof(hdlg));  
}

void histogram_close(GtkWidget *w,gpointer data) {
  gtk_widget_destroy(hdlg.dlg);
}

void histogram_range_edited(GtkWidget *w,gpointer data) {
  char s[128], *p;
  int t;

  strncpy(s,gtk_entry_get_text(GTK_ENTRY(hdlg.xrg)),127);
  s[127] = 0;
  p = strtok(s," -\r\n\t.;,");
  if (p==NULL) return;
  hdlg.user_range[0] = atoi(p);
  p = strtok(NULL," -\r\n\t.;,");  
  hdlg.user_range[1] = atoi(p);

  strncpy(s,gtk_entry_get_text(GTK_ENTRY(hdlg.yrg)),127);
  s[127] = 0;
  p = strtok(s," -\r\n\t.;,");
  if (p==NULL) return;
  hdlg.user_range[2] = atoi(p);
  p = strtok(NULL," -\r\n\t.;,");  
  hdlg.user_range[3] = atoi(p);

  if (hdlg.user_range[0] < 0) hdlg.user_range[0] = 0;
  if (hdlg.user_range[0] > hdlg.maxhist) hdlg.user_range[0] = hdlg.maxhist;
  if (hdlg.user_range[1] < 0) hdlg.user_range[1] = 0;
  if (hdlg.user_range[1] > hdlg.maxhist) hdlg.user_range[1] = hdlg.maxhist;

  if (hdlg.user_range[2] < 0) hdlg.user_range[2] = 0;
  if (hdlg.user_range[2] > hdlg.full_range[3]) hdlg.user_range[2] = hdlg.full_range[3];
  if (hdlg.user_range[3] < 0) hdlg.user_range[3] = 0;
  if (hdlg.user_range[3] > hdlg.full_range[3]) hdlg.user_range[3] = hdlg.full_range[3];

  if (hdlg.user_range[1] < hdlg.user_range[0]) {
    t = hdlg.user_range[1];
    hdlg.user_range[1] = hdlg.user_range[0];
    hdlg.user_range[0] = t;
  }

  if (hdlg.user_range[3] < hdlg.user_range[2]) {
    t = hdlg.user_range[3];
    hdlg.user_range[3] = hdlg.user_range[2];
    hdlg.user_range[2] = t;
  }

  sprintf(s,"%d-%d",hdlg.user_range[0],hdlg.user_range[1]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.xrg), s);
  sprintf(s,"%d-%d",hdlg.user_range[2],hdlg.user_range[3]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.yrg), s);
  refresh(hdlg.canvas);
}

void histogram_reset_range(GtkWidget *w,gpointer data) {
  char s[64];
  sprintf(s,"%d-%d",hdlg.full_range[0],hdlg.full_range[1]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.xrg), s);
  sprintf(s,"%d-%d",hdlg.full_range[2],hdlg.full_range[3]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.yrg), s);
  histogram_range_edited(NULL,NULL);
}

void histogram_toggle(GtkToggleButton *w,gpointer data) {
  refresh(hdlg.canvas);
}

double xlog10(int x) {
  if (x==0) return 0; else return(log10(x));
}

gboolean histogram_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) {
  int i,j,k,w,h,x,uselog,showobjs,outline,x1,x2,y1,y2;
  double a,b,hw,hh,mh;
  static CImg *db = NULL;
  char s[64];
  double xmin, xmax, xd, ymin, ymax, yd;

  gdk_window_get_size(widget->window,&w,&h);

  if (db!=NULL) {
    if (db->W != w || db->H != h) {
      CImgDestroy(db);
      db = NULL;
    }
  }

  if (db == NULL)
    db = CImgNew(w,h);

  // axis
  CImgFill(db,0xffffff);
  CImgDrawLine(db,49,0,49,h-19,0);
  CImgDrawLine(db,49,h-19,w,h-19,0);
  
  xmin = (double) hdlg.user_range[0];
  xmax = (double) hdlg.user_range[1];
  xd = xmax - xmin;
  if (xd < 0.01) xd = 1.0;

  ymin = (double) hdlg.user_range[2];
  ymax = (double) hdlg.user_range[3];
  yd = ymax - ymin;
  if (yd < 0.01) yd = 1.0;
  
  // x ticks
  hw = w-50.0;
  hh = h-20.0;
  a = (1000.0 * hw) / xd;

  if (a > 32.0)  {
    x1 = ((int)(xmin / 1000.0)) * 1000;
    x2 = ((int)(xmax / 1000.0)) * 1000;
    for(i=x1;i<x2;i+=1000) {
      x = 50+((i-xmin)/1000)*a;
      sprintf(s,"%d",i);
      CImgDrawLine(db,x,h-21,x,h-17,0);
      CImgDrawSText(db,res.font[3],x,h-16,0x888888,s);
    }
  }

  a = (500.0 * hw) / xd;
  if (a > 32.0) {
    x1 = ((int)(xmin / 500.0)) * 500;
    x2 = ((int)(xmax / 500.0)) * 500;
    for(i=x1;i<x2;i+=500) {
      x = 50+((i-xmin)/500)*a;
      sprintf(s,"%d",i);
      CImgDrawLine(db,x,h-21,x,h-17,0);
      CImgDrawSText(db,res.font[3],x,h-16,0x888888,s);
    }
  }

  a = (100.0 * hw) / xd;
  if (a > 32.0) {
    x1 = ((int)(xmin / 100.0)) * 100;
    x2 = ((int)(xmax / 100.0)) * 100;
    for(i=x1;i<x2;i+=100) {
      x = 50+((i-xmin)/100)*a;
      sprintf(s,"%d",i);
      CImgDrawLine(db,x,h-21,x,h-17,0);
      CImgDrawSText(db,res.font[3],x,h-16,0x888888,s);
    }
  }

  // histograms
  mh = yd;

  uselog   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hdlg.logscale));
  showobjs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hdlg.objects));
  outline  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hdlg.outline));

  // y ticks
  y1 = (int) pow(10.0, ceil(xlog10(ymin)));
  y2 = (int) pow(10.0, floor(xlog10(ymax)));
  x2 = (int) ceil(xlog10(y1));

  //printf("y1=%d y2=%d x2=%d\n",y1,y2,x2);

  for(i=y1;i<=y2;i*=10,x2++) {
    if (uselog)
      x1 = (hh*(xlog10(i)-xlog10(ymin)))/log10(yd);
    else
      x1 = (hh*(i-ymin))/yd;
    x1 = h-20-x1;
    sprintf(s,"%d",x2);
    CImgDrawLine(db,47,x1,51,x1,0);
    CImgDrawSText(db,res.font[3],20,x1-6,0x888888,"10");
    CImgDrawSText(db,res.font[3],36,x1-10,0x888888,s);
  }

  if (uselog) mh = xlog10(mh);

  // main histogram

  if (outline) {

    x1 = -1; x2 = -1;
    y1 = -1; y2 = -1;

    for(i=xmin;i<=xmax;i++) {

      if (hdlg.histdata[i] == 0) continue;

      if (uselog)
	b = hh * (xlog10(hdlg.histdata[i])-xlog10(ymin)) / mh;
      else
	b = (hh * (hdlg.histdata[i] - ymin)) / mh;

      if (b > hh) b = hh;
      if (b < 0)  b = 0;
      j = 50 + (int) round(((i-xmin)*hw) / xd);
      if (x1 < 0) { 
	x1 = j; y1 = b; 
      } else {
	x2 = j; y2 = b;
	CImgDrawLine(db,x1,h-20-y1,x2,h-20-y2,0x882222);
	x1 = x2;
	y1 = y2;
      }
    }
    
  } else {

    for(i=xmin;i<=xmax;i++) {
      
      if (hdlg.histdata[i] < ymin) continue;
      
      if (uselog) {
	b = hh * (xlog10(hdlg.histdata[i])-xlog10(ymin)) / mh;
      } else
	b = (hh * (hdlg.histdata[i] - ymin)) / mh;
      
      x1 = 50 + (int) round(((i-xmin)*hw) / xd);
      x2 = 50 + (int) round(((i+1-xmin)*hw) / xd);
      
      if (x1 == x2)
	CImgDrawLine(db,x1,(int)(h-20-b),x1,h-20,0x882222);
      else 
	CImgFillRect(db,x1,(int)(h-20-b+1),x2-x1+1,1+(int)b,0x882222);
    }

  }

  // object histograms
  if (showobjs) {

    if (outline) {

      for(j=1;j<=MAXLABELS;j++)  {
	x1 = -1; x2 = -1;
	y1 = -1; y2 = -1;
	for(i=xmin;i<xmax;i++) {
	  
	  if (hdlg.objdata[j][i] == 0) continue;
	  
	  if (uselog)
	    b = hh * (xlog10(hdlg.objdata[j][i])-xlog10(ymin)) / mh;
	  else
	    b = (hh * (hdlg.objdata[j][i]-ymin)) / mh;

	  if (b > hh) b = hh;
	  if (b < 0)  b = 0;
	  k = 50 + (int) round(((i-xmin)*hw) / xd);
	  if (x1 < 0) { 
	    x1 = k; y1 = b; 
	  } else {
	    x2 = k; y2 = b;
	    CImgDrawLine(db,x1,h-20-y1,x2,h-20-y2,labels.colors[j-1]);
	    x1 = x2;
	    y1 = y2;
	  }
	}
      }


    } else {
      for(j=1;j<=MAXLABELS;j++) 
	for(i=xmin;i<xmax;i++) {
	  
	  if (hdlg.objdata[j][i] == 0) continue;
	  if (hdlg.objdata[j][i] < ymin) continue;
	  
	  if (uselog)
	    b = hh * (xlog10(hdlg.objdata[j][i])-xlog10(ymin)) / mh;
	  else
	    b = (hh * (hdlg.objdata[j][i]-ymin)) / mh;
	  
	  x1 = 50 + (int) round(((i-xmin)*hw) / xd);
	  x2 = 50 + (int) round(((i+1-xmin)*hw) / xd);
	  
	  if (x1 == x2)
	    CImgDrawLine(db,x1,(int)(h-20-b),x1,h-20,labels.colors[j-1]);
	  else 
	    CImgFillRect(db,x1,(int)(h-20-b+1),x2-x1+1,1+(int)b,labels.colors[j-1]);
	}
    }
  }

  gdk_draw_rgb_image(widget->window,widget->style->black_gc,
		     0,0,w,h,
		     GDK_RGB_DITHER_NORMAL,db->val,db->rowlen);
  return FALSE;
}


void histogram_update() {
  int i,j;
  Volume *vol, *lab;
  vol = voldata.original;
  lab = voldata.label;
  hdlg.maxhist = VolumeMax(vol);
  hdlg.hsz = hdlg.maxhist + 1;

  hdlg.histdata = (int *) malloc(sizeof(int) * hdlg.hsz);
  if (hdlg.histdata == NULL) return;
  memset(hdlg.histdata,0,sizeof(int) * hdlg.hsz);

  for(i=0;i<=MAXLABELS;i++) {
    hdlg.objdata[i] = (int *) malloc(sizeof(int) * hdlg.hsz);
    memset(hdlg.objdata[i],0,sizeof(int) * hdlg.hsz);
  }

  for(i=0;i<vol->N;i++) {
    if (vol->u.i16[i] >= 0 && vol->u.i16[i] <= hdlg.maxhist) {
      hdlg.histdata[ (int) (vol->u.i16[i]) ]++;
      j = (int) (lab->u.i8[i] & 0x3f);
      if (j>0 && j<=MAXLABELS) hdlg.objdata[j][(int) (vol->u.i16[i])]++;
    }
  }

  hdlg.full_range[0] = 0;
  hdlg.full_range[1] = hdlg.maxhist;
  hdlg.full_range[2] = 0;

  j = 0;
  for(i=0;i<hdlg.hsz;i++)
    if (hdlg.histdata[i] > j)
      j = hdlg.histdata[i];
  hdlg.full_range[3] = j;
  memcpy(hdlg.user_range,hdlg.full_range,4*sizeof(int));
}

void histogram_dialog() {
  GtkWidget *dlg, *v, *c, *h, *h2, *bh, *b1, *xl, *yl, *rb;
  char s[64];

  if (voldata.original == NULL) {
    warn_no_volume();
    return;
  }

  memset(&hdlg,0,sizeof(hdlg));
  histogram_update();

  hdlg.dlg = dlg = util_dialog_new("Histogram",mw,1,&v,&bh,600,400);
  
  hdlg.canvas = c = gtk_drawing_area_new();
  gtk_widget_set_events(c,GDK_EXPOSURE_MASK);
  h = gtk_hbox_new(FALSE,2);
  hdlg.logscale = gtk_check_button_new_with_label("Logarithmic scale");
  hdlg.objects  = gtk_check_button_new_with_label("Object Histograms");
  hdlg.outline  = gtk_check_button_new_with_label("Outline Mode");

  h2 = gtk_hbox_new(FALSE,2);
  xl = gtk_label_new("X Range:");
  hdlg.xrg = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(hdlg.xrg), 10);
  sprintf(s,"%d-%d",hdlg.full_range[0],hdlg.full_range[1]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.xrg), s);
  yl = gtk_label_new("Y Range:");
  hdlg.yrg = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(hdlg.yrg), 10);
  sprintf(s,"%d-%d",hdlg.full_range[2],hdlg.full_range[3]);
  gtk_entry_set_text(GTK_ENTRY(hdlg.yrg), s);
  rb = gtk_button_new_with_label("Full Range");

  b1 = gtk_button_new_with_label("Close");

  gtk_box_pack_start(GTK_BOX(v),c,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(v),h,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(v),h2,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),hdlg.logscale,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),hdlg.objects,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),hdlg.outline,FALSE,FALSE,0);

  gtk_box_pack_start(GTK_BOX(h2),xl,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h2),hdlg.xrg,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h2),yl,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h2),hdlg.yrg,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h2),rb,FALSE,FALSE,4);

  gtk_box_pack_start(GTK_BOX(bh),b1,TRUE,TRUE,0);

  gtk_signal_connect(GTK_OBJECT(dlg), "destroy", 
		     GTK_SIGNAL_FUNC(histogram_destroy), NULL);
  gtk_signal_connect(GTK_OBJECT(b1), "clicked", 
		     GTK_SIGNAL_FUNC(histogram_close), NULL);
  gtk_signal_connect(GTK_OBJECT(rb), "clicked", 
		     GTK_SIGNAL_FUNC(histogram_reset_range), NULL);
  gtk_signal_connect(GTK_OBJECT(c), "expose_event", 
		     GTK_SIGNAL_FUNC(histogram_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(hdlg.logscale), "toggled", 
		     GTK_SIGNAL_FUNC(histogram_toggle), NULL);
  gtk_signal_connect(GTK_OBJECT(hdlg.objects), "toggled", 
		     GTK_SIGNAL_FUNC(histogram_toggle), NULL);
  gtk_signal_connect(GTK_OBJECT(hdlg.outline), "toggled", 
		     GTK_SIGNAL_FUNC(histogram_toggle), NULL);
  gtk_signal_connect(GTK_OBJECT(hdlg.xrg), "activate", 
		     GTK_SIGNAL_FUNC(histogram_range_edited), NULL);
  gtk_signal_connect(GTK_OBJECT(hdlg.yrg), "activate", 
		     GTK_SIGNAL_FUNC(histogram_range_edited), NULL);

  gtk_widget_show_all(dlg);

}
