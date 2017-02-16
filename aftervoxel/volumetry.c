
#define SOURCE_VOLUMETRY_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <zlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

gboolean volumetry_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void     volumetry_destroy(GtkWidget *w, gpointer data);
void     volumetry_close(GtkWidget *w, gpointer data);
void     volumetry_recalc();
void     volumetry_mark(GtkToggleButton *w, gpointer data);
void     volumetry_save();
gboolean volumetry_save2(gpointer data);
void     volumetry_save_error();
void     volumetry_save_success(char *filename);
void     volumetry_export_csv(char *filename);
void     volumetry_export_html(char *filename);

static struct _vd {
  GtkWidget *w;
  int histogram[MAXLABELS+1];
  int index[MAXLABELS+1];
  GtkWidget *mark[MAXLABELS+1];
  GtkWidget *perc1[MAXLABELS+1];
  GtkWidget *perc2[MAXLABELS+1];
  GtkWidget *tot3, *perc3;
  float voxel;
  int n;

  float d1[MAXLABELS+1];
  float d2[MAXLABELS+1];
  float d3[MAXLABELS+1];
  float davg[MAXLABELS+1];

  float d4[3];
  float d5[3];

  int format;

} vdlg;

gboolean volumetry_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  return TRUE;
}

void volumetry_destroy(GtkWidget *w, gpointer data) {
  vdlg.w = 0;
}

void volumetry_close(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(vdlg.w);
}

void volumetry_recalc() {
  int i;
  int mtot = 0, tot = 0;
  float r;
  char z[128];

  for(i=0;i<vdlg.n;i++) {
    tot += vdlg.histogram[ vdlg.index[i] ];
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vdlg.mark[i])))
      mtot += vdlg.histogram[ vdlg.index[i] ];
  }

  for(i=0;i<vdlg.n;i++) {
    r = 100.0 * ((float)(vdlg.histogram[vdlg.index[i]])) / ((float)tot);
    vdlg.d2[i] = r;
    snprintf(z,128,"%.1f %%",r);    
    gtk_label_set_text(GTK_LABEL(vdlg.perc1[i]),z);

    if (mtot > 0 && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vdlg.mark[i]))) {
      r = 100.0 * ((float)(vdlg.histogram[vdlg.index[i]])) / ((float)mtot);
      vdlg.d3[i] = r;
      snprintf(z,128,"%.1f %%",r);
    } else {
      vdlg.d3[i] = -1.0;
      strncpy(z," - ",128);
    }
    gtk_label_set_text(GTK_LABEL(vdlg.perc2[i]),z);
  }

  r = ((float)mtot) * vdlg.voxel;
  vdlg.d4[0] = r;
  snprintf(z,128,"%.2f mm³",r);
  gtk_label_set_text(GTK_LABEL(vdlg.tot3),z);

  r = 100.0 * (((float)mtot) / ((float)tot));
  vdlg.d4[1] = r;
  vdlg.d4[2] = 100.0;
  snprintf(z,128,"%.1f %%",r);
  gtk_label_set_text(GTK_LABEL(vdlg.perc3),z);

  vdlg.d5[0] = ((float)tot) * vdlg.voxel;
  vdlg.d5[1] = 100.0;
  vdlg.d5[2] = -1.0;
}

void volumetry_mark(GtkToggleButton *w, gpointer data) {
  volumetry_recalc();
}

void volumetry_dialog() {
  GtkWidget *v, *hb, *t, *l[7], *hs2, *hs, *close, *save, *p3, *t3;
  ColorLabel *cl;
  int i,j;
  char z[128];
  int64_t sum[MAXLABELS+1];
  int n[MAXLABELS+1];

  if (voldata.original==NULL) {
    warn_no_volume();
    return;
  }

  for(i=0;i<MAXLABELS+1;i++)
    vdlg.histogram[i] = 0;

  MutexLock(voldata.masterlock);

  for(i=1;i<MAXLABELS+1;i++) {
    sum[i] = 0;
    n[i] = 0;
  }
  for(i=0;i<voldata.label->N;i++) {
    vdlg.histogram[ (int) (voldata.label->u.i8[i] & 0x3f) ]++;
    sum[ (int) (voldata.label->u.i8[i] & 0x3f) ] += voldata.original->u.i16[i];
    n[(int) (voldata.label->u.i8[i] & 0x3f) ]++;
  }

  vdlg.voxel = voldata.original->dx * voldata.original->dy * voldata.original->dz;
  MutexUnlock(voldata.masterlock);

  vdlg.w = util_dialog_new("Volumetry",mw,1,&v,&hb,-1,-1);
  t = gtk_table_new(MAXLABELS+5,7,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(t), 4);
  gtk_table_set_col_spacings(GTK_TABLE(t), 6);
  gtk_box_pack_start(GTK_BOX(v), t, TRUE, TRUE, 2);

  l[0] = gtk_label_new(" ");
  l[1] = gtk_label_new(" ");
  l[2] = gtk_label_new(" ");
  l[3] = gtk_label_new(" ");
  l[4] = gtk_label_new(" ");
  l[5] = gtk_label_new(" ");
  l[6] = gtk_label_new(" ");

  gtk_label_set_markup(GTK_LABEL(l[0]),"<b><i>Mark</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[1]),"<b><i>Color</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[2]),"<b><i>Object</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[3]),"<b><i>Volume</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[4]),"<b><i>% Total Volume</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[5]),"<b><i>% Marked Volume</i></b>");
  gtk_label_set_markup(GTK_LABEL(l[6]),"<b><i>Avg. Intensity</i></b>");

  for(i=0;i<7;i++)
    gtk_table_attach_defaults(GTK_TABLE(t),l[i],i,i+1,0,1);

  j = 0;
  for(i=0;i<MAXLABELS+1;i++)
    if (vdlg.histogram[i] > 0) {
      vdlg.index[j] = i;
      vdlg.mark[j] = gtk_check_button_new();
      gtk_signal_connect(GTK_OBJECT(vdlg.mark[j]),"toggled",
			 GTK_SIGNAL_FUNC(volumetry_mark),0);
      if (i>0)
	cl = ColorLabelNew(32,16,labels.colors[i-1],0);
      else
	cl = 0;
      l[0] = util_llabel(i>0?labels.names[i-1]:"Background",0);

      vdlg.d1[j] = vdlg.voxel * ((float)(vdlg.histogram[i]));
      snprintf(z,128,"%.2f mm³",vdlg.d1[j]);
      l[1] = util_rlabel(z,0);
      l[2] = util_rlabel(" - ",&(vdlg.perc1[j]));
      l[3] = util_rlabel(" - ",&(vdlg.perc2[j]));

      vdlg.davg[j] = (n[i] > 0) ? ((float) (((double)sum[i]) / ((double)n[i]))) : 0.0f;

      snprintf(z,128,"%.2f",vdlg.davg[j]);
      l[4] = util_rlabel(z,0);

      gtk_table_attach_defaults(GTK_TABLE(t),vdlg.mark[j],0,1,j+2,j+3);
      if (cl)
	gtk_table_attach_defaults(GTK_TABLE(t),cl->widget,1,2,j+2,j+3);
      gtk_table_attach_defaults(GTK_TABLE(t),l[0],2,3,j+2,j+3);
      gtk_table_attach_defaults(GTK_TABLE(t),l[1],3,4,j+2,j+3);
      gtk_table_attach_defaults(GTK_TABLE(t),l[2],4,5,j+2,j+3);
      gtk_table_attach_defaults(GTK_TABLE(t),l[3],5,6,j+2,j+3);
      gtk_table_attach_defaults(GTK_TABLE(t),l[4],6,7,j+2,j+3);
      ++j;
    }

  vdlg.n = j;

  hs = gtk_hseparator_new();
  hs2 = gtk_hseparator_new();
  gtk_table_attach_defaults(GTK_TABLE(t),hs,0,7,j+2,j+3);
  gtk_table_attach_defaults(GTK_TABLE(t),hs2,0,7,1,2);

  l[0] = util_llabel("Marked",0);
  l[1] = util_llabel("Global",0);
  snprintf(z,128,"%.2f mm³", vdlg.voxel * ((float)(voldata.original->N)));
  l[2] = util_rlabel(z,0);
  l[3] = util_rlabel("100.0 %",0);
  l[4] = gtk_label_new(" - ");
  l[5] = util_rlabel("100.0 %",0);

  t3 = util_rlabel(" - ",&(vdlg.tot3));
  p3 = util_rlabel(" - ",&(vdlg.perc3));

  /* total marcado */
  gtk_table_attach_defaults(GTK_TABLE(t),l[0],0,3,j+3,j+4);
  gtk_table_attach_defaults(GTK_TABLE(t),t3,3,4,j+3,j+4);
  gtk_table_attach_defaults(GTK_TABLE(t),p3,4,5,j+3,j+4);  
  gtk_table_attach_defaults(GTK_TABLE(t),l[3],5,6,j+3,j+4);

  /* total geral */
  gtk_table_attach_defaults(GTK_TABLE(t),l[1],0,3,j+4,j+5);
  gtk_table_attach_defaults(GTK_TABLE(t),l[2],3,4,j+4,j+5);
  gtk_table_attach_defaults(GTK_TABLE(t),l[4],5,6,j+4,j+5);
  gtk_table_attach_defaults(GTK_TABLE(t),l[5],4,5,j+4,j+5);

  close = gtk_button_new_with_label("Close");
  save  = gtk_button_new_with_label("Save...");
  gtk_box_pack_start(GTK_BOX(hb),save,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),close,TRUE,TRUE,0);

  gtk_signal_connect(GTK_OBJECT(vdlg.w), "destroy",
		     GTK_SIGNAL_FUNC(volumetry_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(vdlg.w), "delete_event",
		     GTK_SIGNAL_FUNC(volumetry_delete), 0);
  gtk_signal_connect(GTK_OBJECT(close), "clicked",
		     GTK_SIGNAL_FUNC(volumetry_close), 0);
  gtk_signal_connect(GTK_OBJECT(save), "clicked",
		     GTK_SIGNAL_FUNC(volumetry_save), 0);

  gtk_widget_show_all(vdlg.w);
  volumetry_recalc();
}


void volumetry_save() {
  int format;

  format = util_get_format(vdlg.w,"File Format",";CSV Text;HTML");
  if (format < 0) return;
  vdlg.format = format;

  gtk_timeout_add(90,volumetry_save2,0);

}

gboolean volumetry_save2(gpointer data) {
  char filename[512];
  if (util_get_filename(vdlg.w, "Save Volumetry", filename, 0))
    {
      switch(vdlg.format) {
      case 0: volumetry_export_csv(filename); break;
      case 1: volumetry_export_html(filename); break;
      }
    }
  return FALSE;
}

void volumetry_save_error() {
  char z[512];
  snprintf(z,512,"Error: %s (%d)",strerror(errno), errno);
  PopMessageBox(vdlg.w,"Error",z,MSG_ICON_ERROR);
}

void volumetry_save_success(char *filename) {
  char *p;
  char msg[256];
  p = &filename[strlen(filename)];
  while(p!=filename && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,256,"Volumetry successfully saved as %s (%s)",
	   p,vdlg.format==0?"CSV":"HTML");
  app_set_status(msg,20);
}

void volumetry_export_csv(char *filename) {
  FILE *f;
  int i,j,k;
  char z[256];

  f = fopen(filename,"w");
  if (!f) {
    volumetry_save_error();
    return;
  }

  fprintf(f,"Marked,Object,Volume (mm^3),%% Total Volume,%% Marked Volume,Average Intensity\n");
  
  for(i=0;i<vdlg.n;i++) {    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vdlg.mark[i])))
      fprintf(f,"Yes,");
    else
      fprintf(f,"No,");

    j = vdlg.index[i];

    if (!j) strncpy(z,"Background",256); else strncpy(z,labels.names[j-1],256);
    for(k=0;k<strlen(z);k++) {
      if (z[k]==',') z[k]='_';
      if (z[k]&0x80) z[k]='_';
    }

    fprintf(f,"%s,%.2f,%.1f %%,",z,vdlg.d1[i],vdlg.d2[i]);
    if (vdlg.d3[i] < 0.0) 
      fprintf(f,"n/a,");
    else
      fprintf(f,"%.1f %%,",vdlg.d3[i]);
    fprintf(f,"%.2f\n",vdlg.davg[i]);
  }

  fprintf(f,"%s,,%.2f,%.1f %%,n/a\n","Total",vdlg.d5[0],vdlg.d5[1]);
  fprintf(f,"%s,,%.2f,%.1f %%,%.1f %%\n","Marked",vdlg.d4[0],
	  vdlg.d4[1],vdlg.d4[2]);
  fclose(f);
  volumetry_save_success(filename);
}

void     volumetry_export_html(char *filename) {
  FILE *f;
  int i,j;
  char z[256];

  f = fopen(filename,"w");
  if (!f) {
    volumetry_save_error();
    return;
  }

  fprintf(f,"<html><head><title>%s</title><META http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\"></head>\n","Volumetry");
  fprintf(f,"<body bgcolor=White text=Black>\n");
  fprintf(f,"<h4>%s</h4>\n","Volumetry");
  fprintf(f,"<table border=1 cellspacing=4 cellpadding=4>\n");
  fprintf(f,"<tr><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td><td><b><i>%s</i></b></td></tr>\n",
	  "Marked","Object","Color","Volume (mm&sup3;)","% of Volume","% of Marked","Avg.Intensity");
  
  for(i=0;i<vdlg.n;i++) {    
    fprintf(f,"<tr>");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vdlg.mark[i])))
      fprintf(f,"<td><font color=\"#008000\">%s</font></td>","Yes");
    else
      fprintf(f,"<td><font color=\"#800000\">%s</font></td>","No");

    j = vdlg.index[i];

    if (!j) strncpy(z,"Background",256); else strncpy(z,labels.names[j-1],256);

    fprintf(f,"<td>%s</td><td bgcolor=\"%.6x\">&nbsp;&nbsp;</td><td align=right>%.2f</td><td align=right>%.1f %%</td>",z,j==0?0xffffff:labels.colors[j-1],vdlg.d1[i],vdlg.d2[i]);
    if (vdlg.d3[i] < 0.0) 
      fprintf(f,"<td align=right><i>N/A</i></td>\n");
    else
      fprintf(f,"<td align=right>%.1f %%</td>\n",vdlg.d3[i]);

    fprintf(f,"<td align=right>%.2f</td></tr>\n",vdlg.davg[i]);
  }

  fprintf(f,"<tr><td colspan=3><b>%s</b></td><td align=right>%.2f</td><td align=right>%.1f %%</td><td align=right><i>N/A</i></td></tr>\n","Grand Total",vdlg.d5[0],vdlg.d5[1]);
  fprintf(f,"<tr><td colspan=3><b>%s</b></td><td align=right>%.2f</td><td align=right>%.1f %%</td><td align=right>%.1f %%</td>\n","Marked Total",vdlg.d4[0],vdlg.d4[1],vdlg.d4[2]);
  fprintf(f,"</body></html>\n");

  fclose(f);
  volumetry_save_success(filename);
}

