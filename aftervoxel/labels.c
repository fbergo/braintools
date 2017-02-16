
#define SOURCE_LABELS_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

int newob_bup = 1;
static int LC = 0;

void label_save(const char *name) {
  FILE *f;
  int i;
  f = fopen(name,"w");
  if (f==NULL) return;
  for(i=0;i<labels.count;i++) {
    fprintf(f,"%x,%s\n",labels.colors[i],labels.names[i]);
  }
  fclose(f);
}

void label_load(const char *name) {
  FILE *f;
  int i=0;
  char s[256], *p;
  f = fopen(name,"r");
  if (f==NULL) return;
  while(fgets(s,255,f)!=NULL) {
    if (s[0] == '#') continue;
    p = strtok(s,",\r\n");
    if (p==NULL) continue;
    labels.colors[i] = strtol(p, NULL, 16);
    p = strtok(NULL,"\r\n");
    if (p==NULL) continue;
    strcpy(labels.names[i], p);
    i++;
    if (i==10) break;
  }
  fclose(f);
  if (labels.count < i) labels.count = i;
  refresh(labels.widget);
  labels_changed();
}

void  labels_init() {
  int i;
  labels.img = 0;
  labels.count = 0;
  labels.current = 0;

  for(i=1;i<256;i++) {
    if (i&0x80) labels.colors[i-1] = 0xff0000;
    if (i&0x40) labels.colors[i-1] = 0x0000ff;
  }

  for(i=0;i<MAXLABELS;i++) {
    memset(labels.names[i],0,128);
    labels.colors[i] = 0xffff00;
    labels.visible[i] = 1;
  }

  LC = 0;
  labels_new_object();
}

void labels_ensure_count(int count) {
  if (count > MAXLABELS) count = MAXLABELS;
  while(labels.count < count)
    labels_new_object();
  notify.segchanged++;
}

gboolean  labels_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data)
{
  if (!labels.img)
    labels_configure(widget,0,0);

  labels_draw();  
  gdk_draw_rgb_image(widget->window, widget->style->black_gc,
		     0,0,
		     widget->allocation.width,
		     widget->allocation.height,
		     GDK_RGB_DITHER_NORMAL,
		     labels.img->val,
		     labels.img->rowlen);
  return FALSE;
}

gboolean  labels_configure(GtkWidget *widget,GdkEventConfigure *ec,gpointer data)
{
  int rx,ry;

  app_tdebug(6000);

  if (labels.img == 0)
    labels.img = CImgNew(256,(MAXLABELS*16)+20);
  
  rx = 72;
  ry = 20 + (labels.count) * 16;

  if (widget->allocation.width < rx || widget->allocation.height != ry)
    gtk_drawing_area_size(GTK_DRAWING_AREA(labels.widget), rx, ry);

  return FALSE;
}

void      labels_draw()
{
  int i,by;

  CImgFill(labels.img, view.pbg);

  CImgDrawSButton(labels.img, 4,1, 88,16, 
		  lighter(view.pbg,2), 0, 0x000000, "New Object",
		  res.font[3], newob_bup);

  for(i=0;i<labels.count;i++) {
    by = 20+16*i;

    if (labels.current == i) {
      CImgFillRect(labels.img,0,by,labels.img->W,16,lighter(view.pfg,5));
      CImgDrawRect(labels.img,-1,by,labels.img->W,16,0x000000);
    }

    CImgDrawBevelBox(labels.img,2,by+2,20,12,labels.colors[i],0,0);
    CImgDrawSText(labels.img, res.font[3], 24, by+(16-SFontH(res.font[3],labels.names[i]))/2, 0x000000, labels.names[i]);
  }
}

void labels_new_object() {
  int i;
  static int C[MAXLABELS] = { 0xffff00, 0xff0000, 0x00a000, 0xa000a0,
			      0x0000ff, 0xff8000, 0x00ffff, 0xffc0c0,
			      0xff00ff, 0x80ff00, 0x8080ff, 0x008080 };
  i = labels.count;
  snprintf(labels.names[i],32,"Object #%d",labels.count+1);
  labels.colors[i] = C[(LC++)%MAXLABELS];
  labels.count++;
}

int labels_new_named_object(const char *name, int color) {
  int i;
  i = labels.count;
  snprintf(labels.names[i],32,"%s",name);
  labels.colors[i] = color;
  labels.count++;
  return i+1;
}


gboolean  labels_release(GtkWidget *widget,GdkEventButton *eb,gpointer data) {
  app_tdebug(6001);
  if (!newob_bup) {
    newob_bup = 1;
    labels_new_object();
    gtk_grab_remove(labels.widget);
    refresh(labels.widget);
    labels_changed();
  }
  return FALSE;
}

gboolean  labels_press(GtkWidget *widget,GdkEventButton *eb,gpointer data)
{
  int x,y,b,i;

  app_tdebug(6002);

  b = eb->button;
  x = (int) (eb->x);
  y = (int) (eb->y);

  // new object
  if (b==1 && InBoxXYWH(x,y,4,1,88,16) && labels.count < MAXLABELS) {
    newob_bup = 0;
    gtk_grab_add(labels.widget);
    refresh(labels.widget);
    return FALSE;
  }

  // select a different label
  if (b==1 && y>=20) {
    i = (y-20) / 16;
    if (i>=0 && i<labels.count) {
      labels.current = i;
      refresh(labels.widget);
      return FALSE;
    }
  }

  // edit a label
  if (b==3 && y>=20) {
    i = (y-20) / 16;
    if (i>=0 && i<labels.count) {
      labels_edit(i);
      return FALSE;
    }
  }

  return FALSE;
}

struct {
  GtkWidget *dlg;
  GtkWidget *name;
  int color;
  int i;
} ld;

void labels_edit(int i) {
  GtkWidget *h,*h1,*v,*l,*h2,*ok,*cancel,*color,*kill,*vs;

  app_tdebug(6003);

  ld.i = i;

  ld.dlg = util_dialog_new("Object Properties",mw,1,&v,&h2,-1,-1);
  ld.color = labels.colors[i];
  
  h = gtk_hbox_new(FALSE,2);
  h1 = gtk_hbox_new(FALSE,2);
  l = gtk_label_new("Object Label:");
  ld.name = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(ld.name), labels.names[i]);

  ok     = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");
  color  = gtk_button_new_with_label("Change Color...");
  kill   = gtk_button_new_with_label("Remove Object");

  vs = gtk_hseparator_new();

  gtk_box_pack_start(GTK_BOX(v), h, FALSE, TRUE,4);
  gtk_box_pack_start(GTK_BOX(v), h1, FALSE, TRUE,4);
  gtk_box_pack_start(GTK_BOX(v), vs, FALSE, FALSE,4);
  gtk_box_pack_start(GTK_BOX(h), l, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h), ld.name, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(h1), color, TRUE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(h1), kill, TRUE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(h2), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h2), cancel, TRUE, TRUE, 0);

  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);

  gtk_widget_show_all(ld.dlg);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(labels_edit_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(labels_edit_cancel), 0);
  gtk_signal_connect(GTK_OBJECT(color), "clicked",
		     GTK_SIGNAL_FUNC(labels_edit_color), 0);
  gtk_signal_connect(GTK_OBJECT(kill), "clicked",
		     GTK_SIGNAL_FUNC(labels_edit_kill), 0);
}

void      labels_edit_ok(GtkButton *w, gpointer data) {
  app_tdebug(6004);
  strncpy(labels.names[ld.i], gtk_entry_get_text(GTK_ENTRY(ld.name)), 128);
  labels.names[ld.i][127] = 0;
  labels.colors[ld.i] = ld.color;
  gtk_widget_destroy(ld.dlg);
  refresh(labels.widget);
  labels_changed();
  v2d.invalid = 1;
  notify.segchanged++;
  refresh(cw);
}

void      labels_edit_cancel(GtkButton *w, gpointer data) {
  app_tdebug(6005);
  gtk_widget_destroy(ld.dlg);
}

void      labels_edit_color(GtkButton *w, gpointer data) {
  int ncolor;
  ncolor = util_get_color(ld.dlg, ld.color, "Object Color");
  ld.color = ncolor;
}

void      labels_edit_kill(GtkButton *w, gpointer data) {
  int i; 
  app_tdebug(6006);

  if (PopYesNoBox(ld.dlg,"Confirmation","The object will be lost. Proceed ?",MSG_ICON_ASK)==MSG_NO)
    return;

  for(i=ld.i;i<labels.count-1;i++) {
    labels.colors[i] = labels.colors[i+1];
    strncpy(labels.names[i], labels.names[i+1],128);
  }

  if (voldata.label) {
    for(i=0;i<voldata.label->N;i++) {
      if (voldata.label->u.i8[i] == (i8_t) (ld.i+1))
	voldata.label->u.i8[i] = 0;
      if (voldata.label->u.i8[i] > (i8_t) (ld.i+1))
	voldata.label->u.i8[i]--;
    }
    notify.segchanged++;
  }

  labels.count--;
  if (labels.current == labels.count)
    labels.current = 0;

  gtk_widget_destroy(ld.dlg);
  labels_changed();
}

// update visibility control
void labels_changed() {
  int i;
  app_tdebug(6007);

  for(i=0;i<MAXLABELS;i++) {
    if (i < labels.count) {
      CheckBoxSetState(gui.vis_mark[i], labels.visible[i]);
      LabelSetText(gui.vis_name[i], labels.names[i]);
      gtk_widget_show(gui.vis_mark[i]->widget);
      gtk_widget_show(gui.vis_name[i]->widget);
    } else {
      gtk_widget_hide(gui.vis_mark[i]->widget);
      gtk_widget_hide(gui.vis_name[i]->widget);
    }
  }
  refresh(labels.widget);
}
