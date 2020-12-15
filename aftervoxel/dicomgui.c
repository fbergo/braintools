
#define SOURCE_DICOMGUI_C 1

#pragma GCC diagnostic ignored "-Wformat-truncation"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <libip.h>
#include <gtk/gtk.h>

#include "aftervoxel.h"

#include "xpm/dev_home_64.xpm"
#include "xpm/dev_hd_64.xpm"
#include "xpm/dev_cd_64.xpm"
#include "xpm/dev_net_64.xpm"
#include "xpm/dev_pendrive_64.xpm"
#include "xpm/dev_zip_64.xpm"
#include "xpm/dev_mo_64.xpm"

static GtkWidget *dlg = 0;

static struct _devlist {
  int n;
  DicomDev *devs;
} devlist;

static struct _didlg {
  GtkWidget *cwd;
  GtkWidget *dirs;
  GtkWidget *patients;
  GtkWidget *studies;
  GtkWidget *rescan;
  GtkWidget *preview;
  GtkWidget *ok;
  GtkWidget *interpolate;
  GtkWidget *isosize;
  GtkWidget *isoinfo;

  GtkListStore *ls1;
  GtkListStore *ls2;
  GtkListStore *ls3;
  char curpath[1024];
  char prevpath[1024];

  DicomSlice **slices;
  int slicecount;

  char **names;
  int namecount;

  char **studyids;
  int studycount;

  TaskQueue *bg;

  volatile int busy;
  volatile int total;
  volatile int complete;
  volatile int sync;

  GtkWidget *prog;
  gint toid;
} ctl = { .bg = 0, .sync = 0 };

void     di_dir_refresh();
void     di_patient_refresh();
void     di_patient_refresh_bg(void *args);
void     di_patient_update();
void     di_study_update();
void     di_add_patient(char *name);
void     di_add_study(char *study);
void     di_add_slice(DicomSlice *ds);
void     di_clear_slices();
void     di_clear_patients();
void     di_clear_studies();
gboolean di_timeout(gpointer data);

void     di_destroy(GtkWidget *w, gpointer data);
void     di_cancel(GtkWidget *w, gpointer data);
void     di_ok(GtkWidget *w, gpointer data);
void     di_rescan(GtkWidget *w, gpointer data);
void     di_preview(GtkWidget *w, gpointer data);
void     di_preview_dialog();
void     di_import();
void     di_import2(void *args);

void     di_dir_change(GtkTreeView        *treeview,
		       GtkTreePath        *path,
		       GtkTreeViewColumn  *col,
		       gpointer            userdata);
gint     di_dir_cmp(GtkTreeModel *model,
		    GtkTreeIter  *a,
		    GtkTreeIter  *b,
		    gpointer      userdata);
void     di_patient_changed(GtkTreeSelection *gts, gpointer data);
void     di_study_changed(GtkTreeSelection *gts, gpointer data);
int      slice_ptr_zcmp(const void *a, const void *b);

void     di_size(GtkSpinButton *sb, gpointer data);
void     di_interp(GtkToggleButton *tb, gpointer data);
void     di_update_isoinfo(int selchanged);
void     di_study_size(char *id, int *W,int *H,int *D,
		       float *dx,float *dy,float *dz);

gboolean di_timeout(gpointer data) {
  static int last = 0;
  int par,tot;
  float f;
  char msg[128];

  app_tdebug(3000);

  if (!dlg)
    return FALSE;

  if (ctl.sync > last) {
    last = ctl.sync;

    if (!ctl.busy) {
      gtk_widget_hide(ctl.prog);
      enable(ctl.dirs);
      enable(ctl.patients);
      enable(ctl.studies);
      enable(ctl.rescan);
      di_patient_update();
      di_study_update();
    } else {
      gtk_widget_show(ctl.prog);
    }
  }

  if (ctl.busy) {    
    par = ctl.complete;
    tot = ctl.total;

    if (tot < 0) {
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(ctl.prog));
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctl.prog),"Reading Directory");
    } else {
      f = (float) par;
      f /= (float) tot;
      if (f > 1.0) f=1.0;
      if (f < 0.0) f=0.0;
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctl.prog), (gdouble) f);
      snprintf(msg,128,"Analyzing Files... %d %%",(int)(f*100.0));
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctl.prog),msg);
    }
    refresh(ctl.prog);
  }

  return TRUE;
}

void di_destroy(GtkWidget *w, gpointer data) {
  app_tdebug(3001);
  dlg = 0;
  di_clear_slices();
  di_clear_patients();
  di_clear_studies();
  gtk_timeout_remove(ctl.toid);
  chdir(ctl.prevpath);
}

void di_cancel(GtkWidget *w, gpointer data) {
  app_tdebug(3002);
  gtk_widget_destroy(dlg);
}

void di_ok(GtkWidget *w, gpointer data) {
  app_tdebug(3003);
  di_import();
  gtk_widget_destroy(dlg);
}

void di_rescan(GtkWidget *w, gpointer data) {
  di_dir_refresh();
  di_patient_refresh();
}

void di_add_patient(char *name) {
  int i;
  char *d;

  for(i=0;i<ctl.namecount;i++)
    if (!strcmp(name, ctl.names[i]))
      return;

  ctl.namecount++;
  ctl.names = (char **) MemRealloc(ctl.names,
				   ctl.namecount*sizeof(char *));
  d = ctl.names[ctl.namecount-1] = MemAlloc(strlen(name)+1);
  strncpy(d,name,strlen(name) + 1);
}

void di_add_study(char *study) {
  int i;
  char *d;

  for(i=0;i<ctl.studycount;i++)
    if (!strcmp(study, ctl.studyids[i]))
      return;

  ctl.studycount++;
  ctl.studyids = (char **) MemRealloc(ctl.studyids,
				   ctl.studycount*sizeof(char *));
  d = ctl.studyids[ctl.studycount-1] = MemAlloc(strlen(study)+1);
  strncpy(d,study,strlen(study)+1);
}

void di_add_slice(DicomSlice *ds) {
  int i;

  for(i=0;i<ctl.slicecount;i++)
    if (DicomSliceIsDupe(ctl.slices[i],ds)) {
      return;
    }

  ctl.slicecount++;
  ctl.slices = (DicomSlice **) MemRealloc(ctl.slices, 
			 ctl.slicecount*sizeof(DicomSlice *));
  ctl.slices[ctl.slicecount-1] = ds;
}

void     di_clear_slices() {
  int i;
  if (ctl.slicecount > 0) {
    for(i=0;i<ctl.slicecount;i++)
      DicomSliceDestroy(ctl.slices[i]);
    MemFree(ctl.slices);
    ctl.slices = 0;
    ctl.slicecount = 0;
  }
}

void     di_clear_patients() {
  int i;
  if (ctl.namecount > 0) {
    for(i=0;i<ctl.namecount;i++)
      MemFree(ctl.names[i]);
    MemFree(ctl.names);
    ctl.names = 0;
    ctl.namecount = 0;
  }
}

void     di_clear_studies() {
  int i;
  if (ctl.studycount > 0) {
    for(i=0;i<ctl.studycount;i++)
      MemFree(ctl.studyids[i]);
    MemFree(ctl.studyids);
    ctl.studyids = 0;
    ctl.studycount = 0;
  }
}

int floatcmp(const void *a, const void *b) {
  const float *c,*d;

  c = (const float *) a;
  d = (const float *) b;

  if ( (*c) < (*d) ) return -1;
  if ( (*c) > (*d) ) return 1;
  return 0;
}

void di_patient_changed(GtkTreeSelection *gts, gpointer data) {
  di_study_update();
}

void di_study_changed(GtkTreeSelection *gts, gpointer data) {
  GtkTreeSelection *sel;
  app_tdebug(3004);
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctl.studies));
  if (gtk_tree_selection_get_selected(sel,NULL,NULL)==FALSE) {
    disable(ctl.preview);
    disable(ctl.ok);
  } else {
    enable(ctl.preview);
    enable(ctl.ok);
  }
  di_update_isoinfo(1);
}

void di_study_update() {
  int i,j;
  GtkTreeIter iter;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  DicomSlice *sample=0;
  int zcount,rzcount;
  char tmp[256];
  char *col[6];
  char *who;

  float *zp = 0;

  app_tdebug(3005);

  gtk_list_store_clear(ctl.ls3);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctl.patients));
  if (gtk_tree_selection_get_selected(sel,&model,&iter)==FALSE)
    return; // no patient selected

  gtk_tree_model_get(model,&iter,0,&who,-1);
  di_clear_studies();

  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i],DICOM_FIELD_PATIENT),who))
      di_add_study(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID));

  if (ctl.slicecount)
    zp = (float *) MemAlloc(ctl.slicecount * sizeof(float));

  for(i=0;i<ctl.studycount;i++) {

    zcount = 0;
    rzcount = 0;
    for(j=0;j<ctl.slicecount;j++) {
      if (!strcmp(DicomSliceGet(ctl.slices[j], DICOM_FIELD_STUDYID),
		  ctl.studyids[i])) {
	sample = ctl.slices[j];
	zp[zcount] = ctl.slices[j]->zpos;
	zcount++;
	rzcount += ctl.slices[j]->D;
      }
    }

    qsort(zp,zcount,sizeof(float),&floatcmp);

    col[5] = strdup(ctl.studyids[i]);
    snprintf(tmp,256,"%s",DicomSliceGet(sample,DICOM_FIELD_INSTITUTION));
    col[4] = strdup(tmp);
    snprintf(tmp,256,"%s",DicomSliceGet(sample,DICOM_FIELD_EQUIP));
    col[3] = strdup(tmp);
    snprintf(tmp,256,"%s",DicomSliceGet(sample,DICOM_FIELD_MODALITY));
    col[2] = strdup(tmp);

    if (sample->zsize >= 0.0)
      snprintf(tmp,256,"%dx%dx%d (%.2fx%.2fx%.2f mm)",sample->W,sample->H,rzcount,
	       sample->xsize,sample->ysize,sample->zsize);
    else
      snprintf(tmp,256,"%dx%dx%d (%.2fx%.2f mm)",
	       sample->W,sample->H,rzcount,
	       sample->xsize,sample->ysize);
    col[1] = strdup(tmp);
    snprintf(tmp,256,"%s %.5s",
	     DicomSliceGet(sample,DICOM_FIELD_DATE),
	     DicomSliceGet(sample,DICOM_FIELD_TIME));
    col[0] = strdup(tmp);

    gtk_list_store_append(ctl.ls3, &iter);
    gtk_list_store_set(ctl.ls3, &iter, 
		       0, col[0],
		       1, col[1],
		       2, col[2],
		       3, col[3],
		       4, col[4],
		       5, col[5],
		       -1);
    for(j=0;j<6;j++) {
      free(col[j]);
    }
  }

  if (zp)
    MemFree(zp);
  g_free(who);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(ctl.studies));
  refresh(ctl.studies);
}

void di_patient_update() {
  int i;
  GtkTreeIter iter;

  app_tdebug(3006);
  gtk_list_store_clear(ctl.ls2);

  for(i=0;i<ctl.slicecount;i++)
    di_add_patient(DicomSliceGet(ctl.slices[i], DICOM_FIELD_PATIENT));

  for(i=0;i<ctl.namecount;i++) {
    gtk_list_store_append(ctl.ls2, &iter);
    gtk_list_store_set(ctl.ls2, &iter, 0, ctl.names[i],-1);
  }

  refresh(ctl.patients);
}

void di_patient_refresh() {
  app_tdebug(3007);
  disable(ctl.dirs);
  disable(ctl.patients);
  disable(ctl.studies);
  disable(ctl.rescan);
  gtk_list_store_clear(ctl.ls2);
  gtk_list_store_clear(ctl.ls3);
  TaskNew(ctl.bg, "read dicom dir",di_patient_refresh_bg,0);
}

void di_patient_refresh_bg(void *args) {
  DIR *d;
  struct dirent *de;
  struct stat s;
  DicomSlice *ds;
  int i,nfiles,scandebug=0;
  char tmp[1024],*var;

  var = getenv("AV_SCANDEBUG");
  if (var!=NULL)
    scandebug = atoi(var);

  ctl.total = -1;
  ctl.complete = 0;
  ctl.busy = 1;
  ctl.sync++;

  di_clear_slices();
  di_clear_patients();
  di_clear_studies();

  d = opendir(ctl.curpath);
  if (!d) {
    ctl.busy = 0;
    return;
  }

  i = 0;
  nfiles = 0;
  while( (de=readdir(d)) != NULL ) {
    snprintf(tmp,1024,"%s/%s",ctl.curpath, de->d_name);
    if (stat(tmp, &s) == 0)
      if (S_ISREG(s.st_mode))
	++nfiles;
  }
  rewinddir(d);
  ctl.total = nfiles;

  while( (de=readdir(d)) != NULL ) {
    
    snprintf(tmp,1024,"%s/%s",ctl.curpath, de->d_name);
    
    if (scandebug)
      printf("next scan: %s\n",tmp);

    if (stat(tmp, &s) == 0) {
      if (S_ISREG(s.st_mode)) {
	ds = ReadDicomSlice(tmp);

	if (scandebug) {
	  if (ds==NULL)
	    printf("  not dicom\n");
	  else
	    printf("  dicom\n");
	}

	if (ds!=NULL)
	  di_add_slice(ds);
      }
    }

    ++i;
    ctl.complete = i;
  }
  closedir(d);

  ctl.busy = 0;
  ctl.sync++;
  return;
}

void di_dir_change(GtkTreeView        *treeview,
		   GtkTreePath        *path,
		   GtkTreeViewColumn  *col,
		   gpointer            userdata)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar *name;
  char tmp[1024],t2[256];
  DIR *d;

  app_tdebug(3008);

  model = gtk_tree_view_get_model(treeview);

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_model_get(model, &iter, 0, &name, -1);
    
    snprintf(tmp,1024,"%s/%s", ctl.curpath, name);
    if (chdir(tmp)==0) {
      if ((d=opendir(".")) == NULL) {
	chdir(ctl.curpath);
	g_free(name);
	di_dir_refresh();
	return;
      }
      closedir(d);
      getcwd(ctl.curpath, 1024);
      if (strlen(ctl.curpath) > 48)
	snprintf(t2,256,"(...)%s",ctl.curpath+strlen(ctl.curpath)-48);
      else
	strncpy(t2,ctl.curpath,256);
      gtk_label_set_text(GTK_LABEL(ctl.cwd), t2);
      refresh(ctl.cwd);
      di_dir_refresh();
      di_patient_refresh();
    }
    
    g_free(name);
  }
}

gint di_dir_cmp(GtkTreeModel *model,
		GtkTreeIter  *a,
		GtkTreeIter  *b,
		gpointer      userdata)
{
  gint ret = 0;  
  gchar *name1, *name2, *name3, *name4;

  app_tdebug(3009);

  gtk_tree_model_get(model, a, 0, &name1, -1);
  gtk_tree_model_get(model, b, 0, &name2, -1);

  if (name1 == NULL || name2 == NULL)
    {
      if (name1 == NULL && name2 == NULL)
	ret = 0;
      else
	ret = (name1 == NULL) ? -1 : 1;
    }
  else
    {
      name3 = g_utf8_strup(name1,-1);
      name4 = g_utf8_strup(name2,-1);
      ret = g_utf8_collate(name3,name4);
      g_free(name3);
      g_free(name4);
    }

  g_free(name1);
  g_free(name2);
  return ret;
}

void di_dir_refresh() {
  DIR *d;
  char tmp[1024];
  struct dirent *de;
  struct stat s;
  GtkTreeIter iter;

  d = opendir(ctl.curpath);
  if (!d) return;

  app_tdebug(3010);
  gtk_list_store_clear(ctl.ls1);

  while( (de=readdir(d)) != NULL ) {
    
    if (!strcmp(de->d_name,".")) continue;

    snprintf(tmp,1024,"%s/%s",ctl.curpath, de->d_name);
    
    if (stat(tmp, &s) == 0) {
      if (S_ISDIR(s.st_mode)) {
	gtk_list_store_append(ctl.ls1, &iter);
	gtk_list_store_set(ctl.ls1, &iter, 0, de->d_name,-1);
	//printf("%s\n",de->d_name);
      }
    }

  }
  closedir(d);
  refresh(ctl.dirs);
}

void di_goto(GtkWidget *w,gpointer data) {
  DicomDev *dev = (DicomDev *) data;
  DIR *d;

  app_tdebug(3011);

  if (chdir(dev->path)==0) {
    if ((d=opendir(".")) == NULL) {
      chdir(ctl.curpath);
      di_dir_refresh();
      return;
    }
    closedir(d);
    getcwd(ctl.curpath, 1024);
    gtk_label_set_text(GTK_LABEL(ctl.cwd), ctl.curpath);
    refresh(ctl.cwd);
    di_dir_refresh();
    di_patient_refresh();
  }
}

void dicom_import_dialog() {
  GtkWidget *hdev,*bdev,*hf;
  GtkWidget *v,*v1,*hp,*dirs,*pat,*stu,*v2,*h2,*rescan,*bh,*cancel;
  GtkWidget *ds,*ps,*ss,*h3;
  GtkWidget *isof, *isoh, *isol;
  GtkCellRenderer     *renderer;
  GtkTreeSelection    *sel;
  GList *cl, *li;
  int i;

  app_tdebug(3012);
  
  if (dlg!=0) {
    gtk_window_present(GTK_WINDOW(dlg));
    return;
  }

  ctl.slices = 0;
  ctl.slicecount = 0;
  ctl.names = 0;
  ctl.namecount = 0;
  ctl.studyids = 0;
  ctl.studycount = 0;

  if (!ctl.bg) {
    ctl.bg = TaskQueueNew();
    StartThread(ctl.bg);
  }
  ctl.busy = 0;

  getcwd(ctl.curpath, 1024);
  getcwd(ctl.prevpath, 1024);
  // ERROR CHECK ?

  dlg = util_dialog_new("DICOM Import",mw,1,&v,&bh,900,600);

  /* top */
  hf = gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(hf),GTK_SHADOW_IN);
  hdev = gtk_hbox_new(FALSE,0);
  for(i=0;i<devlist.n;i++) {
    bdev = dev_button_new(&(devlist.devs[i]));
    gtk_box_pack_start(GTK_BOX(hdev), bdev, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(bdev),"clicked",
		       GTK_SIGNAL_FUNC(di_goto),
		       (gpointer)(&(devlist.devs[i])));
  }
  gtk_box_pack_start(GTK_BOX(v),hf,FALSE,TRUE,0);
  gtk_container_add(GTK_CONTAINER(hf),hdev);

  /* middle */
  hp = gtk_hpaned_new();
  gtk_box_pack_start(GTK_BOX(v), hp, TRUE, TRUE, 2);

  v1 = gtk_vbox_new(FALSE,2);
  ctl.cwd = gtk_label_new(ctl.curpath);
  gtk_box_pack_start(GTK_BOX(v1), ctl.cwd, FALSE, TRUE, 2);

  dirs = ctl.dirs     = gtk_tree_view_new();
  pat  = ctl.patients = gtk_tree_view_new();
  stu  = ctl.studies  = gtk_tree_view_new();

  ds = gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ds), 
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(ds), dirs);

  ps = gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ps), 
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(ps), pat);

  ss = gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ss), 
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(ss), stu);

  gtk_box_pack_start(GTK_BOX(v1), ds, TRUE, TRUE, 2);

  v2 = gtk_vbox_new(FALSE,2);
  h2 = gtk_hbox_new(FALSE,4);
  ctl.rescan = rescan = gtk_button_new_with_label("Update");

  ctl.prog = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(ctl.prog), 0.50);  

  gtk_paned_pack1(GTK_PANED(hp), v1, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(hp), v2, TRUE, TRUE);

  gtk_box_pack_start(GTK_BOX(v2), h2, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(h2), rescan, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(h2), ctl.prog, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(v2), ps, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(v2), ss, TRUE, TRUE, 2);

  ctl.interpolate = gtk_check_button_new_with_label("Isotropic Interpolation");
  gtk_box_pack_start(GTK_BOX(v2), ctl.interpolate, FALSE, TRUE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctl.interpolate),TRUE);

  isof = gtk_frame_new("Isotropic Voxel Size");
  gtk_frame_set_shadow_type(GTK_FRAME(isof),GTK_SHADOW_ETCHED_IN);
  isoh = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(isoh), 4);
  ctl.isosize = gtk_spin_button_new_with_range(0.10,5.00,0.05);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ctl.isosize), 2);
  isol = gtk_label_new("mm.");
  ctl.isoinfo = gtk_label_new(" ");
  util_set_fg(ctl.isoinfo, 0x000080);

  gtk_box_pack_start(GTK_BOX(v2), isof, FALSE, TRUE, 2);
  gtk_container_add(GTK_CONTAINER(isof), isoh);
  gtk_box_pack_start(GTK_BOX(isoh), ctl.isosize, FALSE,TRUE,2);
  gtk_box_pack_start(GTK_BOX(isoh), isol, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(isoh), ctl.isoinfo, FALSE, TRUE, 6);

  gtk_widget_show(isof);
  gtk_widget_show(isoh);
  gtk_widget_show(ctl.isosize);
  gtk_widget_show(isol);
  gtk_widget_show(ctl.isoinfo);
  disable(ctl.isosize);

  h3 = gtk_hbox_new(FALSE,2);
  ctl.preview = gtk_button_new_with_label("View");
  ctl.ok = gtk_button_new_with_label("Import");

  gtk_box_pack_start(GTK_BOX(h3), ctl.preview, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(h3), ctl.ok, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(v2), h3, FALSE, TRUE, 2);

  disable(ctl.preview);
  disable(ctl.ok);

  gtk_paned_set_position(GTK_PANED(hp), 300);

  /* populate dirs */
  ctl.ls1 = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(dirs), GTK_TREE_MODEL(ctl.ls1));  
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (dirs), -1,
					       "Directories",
                                               renderer, "text", 0,
                                               NULL);
  
  gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ctl.ls1), 0, 
				  di_dir_cmp, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ctl.ls1), 0, 
				       GTK_SORT_ASCENDING);
  
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(dirs));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  gtk_signal_connect(GTK_OBJECT(dirs), "row_activated",
		     GTK_SIGNAL_FUNC(di_dir_change), 0);

  /* patients list */
  ctl.ls2 = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(pat), GTK_TREE_MODEL(ctl.ls2));  
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (pat), -1,
					       "Patients",
                                               renderer, "text", 0,
                                               NULL);
  
  gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ctl.ls2), 0, 
				  di_dir_cmp, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ctl.ls2), 0, 
				       GTK_SORT_ASCENDING);
  
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pat));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  g_signal_connect(G_OBJECT (sel),"changed",G_CALLBACK(di_patient_changed),0);

  /* studies list */

  ctl.ls3 = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(stu), GTK_TREE_MODEL(ctl.ls3));
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 0,
					       "Date/Time",
                                               renderer, "text", 0,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 1,
					       "Dimension",
                                               renderer, "text", 1,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 2,
					       "Protocol",
                                               renderer, "text", 2,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 3,
					       "Scanner",
                                               renderer, "text", 3,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 4,
					       "Institution",
                                               renderer, "text", 4,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (stu), 5,
					       "Series ID",
                                               renderer, "text", 5,
                                               NULL);

  cl = gtk_tree_view_get_columns(GTK_TREE_VIEW(stu));
  li = cl;
  for(li=cl;li!=0;li=li->next)
    gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(li->data), TRUE);
  g_list_free(cl);

  gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ctl.ls3), 0, 
				  di_dir_cmp, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ctl.ls3), 0, 
				       GTK_SORT_ASCENDING);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(stu));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  g_signal_connect(G_OBJECT (sel),"changed",G_CALLBACK(di_study_changed),0);

  /* bottom */
  cancel = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(bh), cancel, TRUE, TRUE, 0);

  /* signals */

  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     GTK_SIGNAL_FUNC(di_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(di_cancel), 0);
  gtk_signal_connect(GTK_OBJECT(ctl.ok), "clicked",
		     GTK_SIGNAL_FUNC(di_ok), 0);
  gtk_signal_connect(GTK_OBJECT(ctl.preview), "clicked",
		     GTK_SIGNAL_FUNC(di_preview), 0);
  gtk_signal_connect(GTK_OBJECT(rescan), "clicked",
		     GTK_SIGNAL_FUNC(di_rescan), 0);
  gtk_signal_connect(GTK_OBJECT(ctl.interpolate),"toggled",
		     GTK_SIGNAL_FUNC(di_interp), 0);
  gtk_signal_connect(GTK_OBJECT(ctl.isosize),"value_changed",
		     GTK_SIGNAL_FUNC(di_size), 0);

  gtk_widget_show(hp);
  gtk_widget_show(dirs);
  gtk_widget_show(hdev);
  gtk_widget_show(hf);
  gtk_widget_show(ds);
  gtk_widget_show(ps);
  gtk_widget_show(ss);
  gtk_widget_show(v2);
  gtk_widget_show(v1);
  gtk_widget_show(ctl.cwd);
  gtk_widget_show(h2);
  gtk_widget_show(rescan);
  gtk_widget_show(h3);
  gtk_widget_show(ctl.preview);
  gtk_widget_show(ctl.interpolate);
  gtk_widget_show(ctl.ok);
  gtk_widget_show(pat);
  gtk_widget_show(stu);

  gtk_widget_show(bh);
  gtk_widget_show(cancel);
  gtk_widget_show(v);
  gtk_widget_show(dlg);

  ctl.toid = gtk_timeout_add(300, di_timeout, 0);

  di_dir_refresh();
  di_patient_refresh();
}

static struct _di_preview {
  DicomSlice **mosaic;
  CImg **images;
  CImg **thumbs;
  char  *sel;
  GdkGC *gc1,*gc2;
  GtkWidget *dl,*dr;
  int nf, ns;
  gint toid;
  int sync;
  volatile int done;

  int sx,sy;
  int grabbed;
  int func; // 0 zoom, 1 bri/con
  float zoom, pzoom;
  float bri, pbri, con, pcon;
} ctl2;

int slice_ptr_zcmp(const void *a, const void *b) {
  const DicomSlice **da, **db;

  da = (const DicomSlice **) a;
  db = (const DicomSlice **) b;

  if ((*da)->zpos < (*db)->zpos) return -1;
  if ((*da)->zpos > (*db)->zpos) return  1;
  if ((*da)->imgnumber < (*db)->imgnumber) return -1;
  if ((*da)->imgnumber > (*db)->imgnumber) return  1;
  if ((*da)->echonumber < (*db)->echonumber) return -1;
  if ((*da)->echonumber > (*db)->echonumber) return  1;
  return 0;
}

void     di_preview(GtkWidget *w, gpointer data) {
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *id;
  int i,j;

  app_tdebug(3013);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctl.studies));
  if (gtk_tree_selection_get_selected(sel,&model,&iter)==FALSE)
    return;

  gtk_tree_model_get(model,&iter,5,&id,-1);

  ctl2.nf = 0;
  ctl2.ns = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id)) {
      ctl2.nf++;
      ctl2.ns += ctl.slices[i]->D;
    }

  ctl2.mosaic = (DicomSlice **) MemAlloc(ctl2.nf * sizeof(DicomSlice *));
  ctl2.images = (CImg **) MemAlloc(ctl2.ns * sizeof(CImg *));
  ctl2.thumbs = (CImg **) MemAlloc(ctl2.ns * sizeof(CImg *));
  ctl2.sel    = (char *) MemAlloc(ctl2.ns * sizeof(char));
  if (!ctl2.mosaic || !ctl2.images || !ctl2.thumbs || !ctl2.sel) {
    g_free(id);
    return;
  }
  MemSet(ctl2.sel, 0, ctl2.ns);
  ctl2.sel[0] = 1;

  j = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id))
      ctl2.mosaic[j++] = ctl.slices[i];

  g_free(id);

  qsort(ctl2.mosaic, ctl2.nf, sizeof(DicomSlice *), &slice_ptr_zcmp);
  ctl2.done = 0;

  di_preview_dialog();
}

GtkWidget *pdlg = 0;

void di_pdestroy(GtkWidget *w, gpointer data);
void di_pclose(GtkWidget *w, gpointer data);
gboolean di_pexpose1(GtkWidget *w, GdkEventExpose *ee, gpointer data);
gboolean di_pclick1(GtkWidget *w, GdkEventButton *eb, gpointer data);
gboolean di_pexpose2(GtkWidget *w, GdkEventExpose *ee, gpointer data);
void     di_pload(void *args);
gboolean di_ptimeout(gpointer data);

gboolean di_ppress2(GtkWidget *w, GdkEventButton *eb, gpointer data);
gboolean di_prelease2(GtkWidget *w, GdkEventButton *eb, gpointer data);
gboolean di_pdrag2(GtkWidget *w, GdkEventMotion *em, gpointer data);

void di_preview_dialog() {
  GtkWidget *v,*hp,*sl,*sr,*dl,*dr,*bh,*close;
  int i,MW,MH;

  app_tdebug(3014);

  MW = ctl2.mosaic[0]->W;
  MH = ctl2.mosaic[0]->H;
  for(i=0;i<ctl2.nf;i++) {
    MW = MAX(MW,ctl2.mosaic[i]->W);
    MH = MAX(MH,ctl2.mosaic[i]->H);
  }

  ctl2.gc1 = ctl2.gc2 = 0;
  ctl2.toid = -1;
  ctl2.zoom = 1.0;
  ctl2.bri = 0.0;
  ctl2.con = 0.0;
  ctl2.grabbed = 0;

  pdlg = util_dialog_new("DICOM View",dlg,1,&v,&bh,640,480);

  hp = gtk_hpaned_new();
  sl = gtk_scrolled_window_new(0,0);
  sr = gtk_scrolled_window_new(0,0);
  ctl2.dl = dl = gtk_drawing_area_new();
  ctl2.dr = dr = gtk_drawing_area_new();
  gtk_widget_set_events(dl,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_events(dr,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON1_MOTION_MASK|GDK_BUTTON3_MOTION_MASK|
			GDK_BUTTON_RELEASE_MASK);

  gtk_box_pack_start(GTK_BOX(v), hp, TRUE, TRUE, 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sl), 
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sr), 
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

  gtk_drawing_area_size(GTK_DRAWING_AREA(dr), MW, MH);
  gtk_drawing_area_size(GTK_DRAWING_AREA(dl), 72*4, 72*(1+(ctl2.ns/4)));
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sl), dl);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sr), dr);

  gtk_paned_pack1(GTK_PANED(hp), sl, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(hp), sr, TRUE, TRUE);

  close = gtk_button_new_with_label("Close");
  gtk_box_pack_start(GTK_BOX(bh), close, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(pdlg), "destroy",
		     GTK_SIGNAL_FUNC(di_pdestroy), 0);
  gtk_signal_connect(GTK_OBJECT(close), "clicked",
		     GTK_SIGNAL_FUNC(di_pclose), 0);
  gtk_signal_connect(GTK_OBJECT(dl), "expose_event",
		     GTK_SIGNAL_FUNC(di_pexpose1), 0);
  gtk_signal_connect(GTK_OBJECT(dl), "button_press_event",
		     GTK_SIGNAL_FUNC(di_pclick1), 0);
  gtk_signal_connect(GTK_OBJECT(dr), "expose_event",
		     GTK_SIGNAL_FUNC(di_pexpose2), 0);

  gtk_signal_connect(GTK_OBJECT(dr), "button_press_event",
		     GTK_SIGNAL_FUNC(di_ppress2), 0);
  gtk_signal_connect(GTK_OBJECT(dr), "button_release_event",
		     GTK_SIGNAL_FUNC(di_prelease2), 0);
  gtk_signal_connect(GTK_OBJECT(dr), "motion_notify_event",
		     GTK_SIGNAL_FUNC(di_pdrag2), 0);

  gtk_widget_show_all(pdlg);

  TaskNew(ctl.bg, "going to magrathea", di_pload, 0);
  ctl2.sync = 0;
  ctl2.toid = gtk_timeout_add(400, di_ptimeout, 0);
}

gboolean di_ptimeout(gpointer data) {

  if (ctl2.sync < ctl2.done) {
    ctl2.sync = ctl2.done;
    refresh(ctl2.dl);
    refresh(ctl2.dr);
  }

  if (ctl2.done == ctl2.ns) {
    ctl2.toid = -1;
    return FALSE;
  }

  return TRUE;
}

void di_pclose(GtkWidget *w, gpointer data) {
  app_tdebug(3015);
  gtk_widget_destroy(pdlg);
}

void di_pdestroy(GtkWidget *w, gpointer data) {
  int i;
  while(ctl2.done < ctl2.ns) usleep(500);
  for(i=0;i<ctl2.ns;i++) {
    CImgDestroy(ctl2.thumbs[i]);
    CImgDestroy(ctl2.images[i]);
  }
  MemFree(ctl2.thumbs);
  MemFree(ctl2.images);
  MemFree(ctl2.mosaic);
  MemFree(ctl2.sel);
  ctl2.thumbs = 0;
  ctl2.images = 0;
  ctl2.mosaic = 0;

  if (ctl2.gc1) {
    gdk_gc_destroy(ctl2.gc1);
    ctl2.gc1 = 0;
  }
  if (ctl2.gc2) {
    gdk_gc_destroy(ctl2.gc2);
    ctl2.gc2 = 0;
  }
  pdlg = 0;
}

gboolean di_pclick1(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  int ctrl,shift,x,y;
  int i,j,k,f,l;

  x = (int) (eb->x);
  y = (int) (eb->y);
  ctrl  = ((eb->state & GDK_CONTROL_MASK) != 0);
  shift = ((eb->state & GDK_SHIFT_MASK) != 0);

  if (x > 72*4) return FALSE;

  j = (x/72) + (y/72)*4;
  if (j<0 || j >= ctl2.done) return FALSE;

  if (!ctrl && !shift) {
    for(i=0;i<ctl2.ns;i++)
      ctl2.sel[i] = (i==j);
  }
  if (ctrl && !shift)
    ctl2.sel[j] = !ctl2.sel[j];
  if (shift && !ctrl) {
    k = -1;
    for(i=0;i<ctl2.ns;i++)
      if (i!=j && ctl2.sel[i]) {
	k = i;
	break;
      }

    if (k>=0) {
      f = MIN(k,j);
      l = MAX(k,j);
      for(i=f;i<=l;i++)
	ctl2.sel[i] = 1;
    }
  }

  refresh(ctl2.dl);
  refresh(ctl2.dr);
  return FALSE;
}

gboolean di_pexpose1(GtkWidget *w, GdkEventExpose *ee, gpointer data) {
  int W,H;
  GdkGC *gc;
  int i;
  int x,y,iw,ih;
  CImg *img;

  W = w->allocation.width;
  H = w->allocation.height;

  if (!ctl2.gc1)
    ctl2.gc1 = gdk_gc_new(w->window);
  gc = ctl2.gc1;

  gc_color(gc,0x9090d0);
  gdk_draw_rectangle(w->window,gc,TRUE,0,0,W,H);
  
  for(i=0;i<ctl2.done;i++) {
    x = (i%4)*72;
    y = (i/4)*72;
    img = ctl2.thumbs[i];
    iw = img->W;
    ih = img->H;

    if (ctl2.sel[i]) {
      gc_color(gc,0xffff80);
      gdk_draw_rectangle(w->window,gc,TRUE,x,y,71,71);
    }
    gdk_draw_rgb_image(w->window,w->style->black_gc,
		       x+(72-iw)/2,y+(72-ih)/2,iw,ih,
		       GDK_RGB_DITHER_NORMAL,
		       img->val, img->rowlen);
  }

  for(i=0;i<ctl2.done;i++) {
    x = (i%4)*72;
    y = (i/4)*72;
    gc_color(gc,ctl2.sel[i]?0xffff00:0x0000ff);
    gdk_draw_rectangle(w->window,gc,FALSE,x,y,71,71);
  }

  return FALSE;
}

gboolean di_pexpose2(GtkWidget *w, GdkEventExpose *ee, gpointer data) {
  int W,H;
  int i;
  GdkGC *gc;
  CImg *z,*t;
  char msg[32];

  W = w->allocation.width;
  H = w->allocation.height;

  if (!ctl2.gc2)
    ctl2.gc2 = gdk_gc_new(w->window);
  gc = ctl2.gc2;

  gc_color(gc,0x9090d0);
  gdk_draw_rectangle(w->window,gc,TRUE,0,0,W,H);

  for(i=0;i<ctl2.ns;i++)
    if (ctl2.sel[i])
      break;

  if (i<ctl2.done) {

    if (i<ctl2.ns) {      

      t = CImgNew(ctl2.images[i]->W,ctl2.images[i]->H);
      CImgBrightnessContrast(t,ctl2.images[i],ctl2.bri,ctl2.con,0xff0000);

      if (ctl2.zoom == 1.0 && !ctl2.grabbed)
	z = t;
      else
	z = CImgScale(t, ctl2.zoom);

      if (ctl2.grabbed) {

	switch(ctl2.func) {
	case 0:
	  snprintf(msg,32,"ZOOM: %d %%",(int)(ctl2.zoom*100.0));
	  CImgDrawShieldedSText(z,res.font[2],5,5,0x00ff00,0,msg);
	  break;
	case 1:
	  snprintf(msg,32,"BRIGHTNESS: %+d %%",(int)(ctl2.bri));
	  CImgDrawShieldedSText(z,res.font[2],5,5,0x00ff00,0,msg);
	  snprintf(msg,32,"CONTRAST: %+d %%",(int)(ctl2.con));
	  CImgDrawShieldedSText(z,res.font[2],5,20,0x00ff00,0,msg);
	  break;
	}

      }

      gdk_draw_rgb_image(w->window,w->style->black_gc,
			 0,0,z->W,z->H,
			 GDK_RGB_DITHER_NORMAL,
			 z->val, z->rowlen);
      gc_color(gc,0x0000ff);
      gdk_draw_rectangle(w->window,gc,FALSE,0,0,z->W,z->H);


      if (z!=t)
	CImgDestroy(z);
      CImgDestroy(t);
    }

  }

  return FALSE;
}

void di_pload(void *args) {
  int i,j,k,ni,W,H,WH;
  DicomSlice *ds;
  float f,fmax;

  ni = 0;
  for(i=0;i<ctl2.nf;i++) {

    ds = ctl2.mosaic[i];

    DicomSliceLoadImage(ds);
    if (!ds->image)
      return;
    
    W = ds->W;
    H = ds->H;
    WH = W*H;

    for(k=0;k<ds->D;k++) {
      ctl2.images[ni] = CImgNew(W,H);
      if (!ctl2.images[ni])
	return;

      fmax = (float) (ds->maxval - ds->minval);
      if (fmax == 0.0) ++fmax;

      for(j=0;j<W*H;j++) {
	f = (float) ds->image[k*WH+j] - ds->minval;
	f *= 255.0;
	f /= (fmax - ds->minval);
	CImgSet(ctl2.images[ni], j%W, j/W, gray( (int) f ));
      }

      // build thumbnail
      ctl2.thumbs[ni] = CImgScaleToFit(ctl2.images[ni], 64);
      ctl2.done++;
      ni++;
    }
  }
}

gboolean di_ppress2(GtkWidget *w, GdkEventButton *eb, gpointer data) {

  app_tdebug(3016);
  if (eb->button == 1) {
    ctl2.sx = (int) (eb->x);
    ctl2.sy = (int) (eb->y);
    ctl2.grabbed = 1;
    ctl2.func = 0;
    ctl2.pzoom = ctl2.zoom;
    gtk_grab_add(w);
    refresh(ctl2.dr);
  }
  if (eb->button == 2) {
    ctl2.zoom = 1.0;
    ctl2.bri = 0.0;
    ctl2.con = 0.0;
    refresh(ctl2.dr);
    gtk_drawing_area_size(GTK_DRAWING_AREA(ctl2.dr),
			  (int) (ctl2.mosaic[0]->W * ctl2.zoom),
			  (int) (ctl2.mosaic[0]->H * ctl2.zoom));
  }
  if (eb->button == 3) {
    ctl2.sx = (int) (eb->x);
    ctl2.sy = (int) (eb->y);
    ctl2.grabbed = 1;
    ctl2.func = 1;
    ctl2.pbri = ctl2.bri;
    ctl2.pcon = ctl2.con;
    gtk_grab_add(w);
    refresh(ctl2.dr);
  }

  return FALSE;
}

gboolean di_prelease2(GtkWidget *w, GdkEventButton *eb, gpointer data) {
  float dx,dy;

  app_tdebug(3017);

  if (ctl2.grabbed) {
    switch(ctl2.func) {
    case 0:
      dx = eb->x - ctl2.sx;
      ctl2.zoom = ctl2.pzoom * (1.0 + (dx/40.0));
      if (ctl2.zoom < 0.15) ctl2.zoom = 0.15;
      if (ctl2.zoom > 16.0) ctl2.zoom = 16.0;
      break;
    case 1:
      dx = eb->x - ctl2.sx;
      dy = eb->y - ctl2.sy;
      ctl2.con = ctl2.pcon + dx/1.75;
      ctl2.bri = ctl2.pbri - dy/1.75;
      if (ctl2.con < -100.0) ctl2.con = -100.0;
      if (ctl2.bri < -100.0) ctl2.bri = -100.0;
      if (ctl2.con > 100.0) ctl2.con = 100.0;
      if (ctl2.bri > 100.0) ctl2.bri = 100.0;
      break;
    }
    ctl2.grabbed = 0;
    gtk_grab_remove(w);
    if (ctl2.func == 0)
      gtk_drawing_area_size(GTK_DRAWING_AREA(ctl2.dr),
			    (int) (ctl2.mosaic[0]->W * ctl2.zoom),
			    (int) (ctl2.mosaic[0]->H * ctl2.zoom));
    refresh(ctl2.dr);
  }

  return FALSE;
}

gboolean di_pdrag2(GtkWidget *w, GdkEventMotion *em, gpointer data) {
  float dx,dy;

  if (ctl2.grabbed && (em->state & GDK_BUTTON1_MASK) && ctl2.func==0) {
    dx = em->x - ctl2.sx;
    ctl2.zoom = ctl2.pzoom * (1.0 + (dx/40.0));
    if (ctl2.zoom < 0.15) ctl2.zoom = 0.15;
    if (ctl2.zoom > 16.0) ctl2.zoom = 16.0;
    refresh(ctl2.dr);
  }
  if (ctl2.grabbed && (em->state & GDK_BUTTON3_MASK) && ctl2.func==1) {
    dx = em->x - ctl2.sx;
    dy = em->y - ctl2.sy;
    ctl2.con = ctl2.pcon + dx/1.75;
    ctl2.bri = ctl2.pbri - dy/1.75;
    if (ctl2.con < -100.0) ctl2.con = -100.0;
    if (ctl2.bri < -100.0) ctl2.bri = -100.0;
    if (ctl2.con > 100.0) ctl2.con = 100.0;
    if (ctl2.bri > 100.0) ctl2.bri = 100.0;
    refresh(ctl2.dr);
  }

  return FALSE;
}

static struct _dimp {
  DicomSlice **mosaic;
  int n;
  int   interpolate;
  float voxelsize;
} di;

void di_import(void *args) {
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *id;
  int i,j;

  app_tdebug(3018);

  di.interpolate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctl.interpolate));
  di.voxelsize = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctl.isosize));

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctl.studies));
  if (gtk_tree_selection_get_selected(sel,&model,&iter)==FALSE)
    return;

  gtk_tree_model_get(model,&iter,5,&id,-1);

  di.n = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id))
      di.n++;

  di.mosaic = (DicomSlice **) MemAlloc(di.n * sizeof(DicomSlice *));
  if (!di.mosaic) {
    g_free(id);
    return;
  }

  j = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id)) {
      di.mosaic[j] = DicomSliceNew();
      DicomSliceCopy(di.mosaic[j],ctl.slices[i]);
      j++;
    }

  g_free(id);

  qsort(di.mosaic, di.n, sizeof(DicomSlice *), &slice_ptr_zcmp);

  TaskNew(res.bgqueue,"Importing DICOM volume...",(Callback) &di_import2, 0);
}

void di_import2(void *args) {
  int i,W,H,D,n,mval,Mval,pd, signedness=-1, psign=-1;
  Volume *v,*vv;
  float dx,dy,dz;

  n = di.n;
  W = di.mosaic[0]->W;
  H = di.mosaic[0]->H;
  D = 0;
  for(i=0;i<n;i++)
    D += di.mosaic[i]->D;

  info_reset();
  strncpy(patinfo.name, DicomSliceGet(di.mosaic[0],DICOM_FIELD_PATIENT), 127);
  strncpy(patinfo.institution, DicomSliceGet(di.mosaic[0],DICOM_FIELD_INSTITUTION), 127);
  strncpy(patinfo.equipment, DicomSliceGet(di.mosaic[0],DICOM_FIELD_EQUIP), 127);
  strncpy(patinfo.scandate, DicomSliceGet(di.mosaic[0],DICOM_FIELD_DATE), 31);
  strncpy(patinfo.protocol, DicomSliceGet(di.mosaic[0],DICOM_FIELD_MODALITY), 127);

  SetProgress(0,2*D);
  v = VolumeNew(W,H,D,vt_integer_16);
  if (!v) return;
  
  dx = di.mosaic[0]->xsize;
  dy = di.mosaic[0]->ysize;
  dz = di.mosaic[0]->zsize;
  if (dz == 0.0) dz = 1.0;

  v->dx = dx;
  v->dy = dy;
  v->dz = dz;

  pd = 0;
  for(i=0;i<di.n;i++) {
    /*
    printf("i=%d zpos=%f imgnumber=%d echonumber=%d\n",
    	   i,di.mosaic[i]->zpos,di.mosaic[i]->imgnumber,di.mosaic[i]->echonumber);
    */
    DicomSliceLoadImage(di.mosaic[i]);
    if (di.mosaic[i]->image) {
      MemCpy(&(v->u.i16[pd*W*H]),di.mosaic[i]->image,2 * W * H * di.mosaic[i]->D);
      psign = signedness;
      signedness = di.mosaic[i]->signedness;
      //printf("gs=%d\n",signedness);
      if (psign >= 0 && signedness >= 0 && psign != signedness)
	fprintf(stderr,"** aftervoxel warning: signedness flag changes between slices of the same study.\n");
    }

    pd += di.mosaic[i]->D;

    DicomSliceDiscardImage(di.mosaic[i]);
    DicomSliceDestroy(di.mosaic[i]);
    SetProgress(pd,2*D);
  }
  MemFree(di.mosaic);
  di.mosaic = 0;
  
  if (di.interpolate && di.voxelsize > 0.0) {
    vv = VolumeInterpolate(v,di.voxelsize,di.voxelsize,di.voxelsize);
    if (vv) {
      VolumeDestroy(v);
      v = vv;
    }
  }
  
  // fix negative offset

  if (signedness) {
    for(;;) {
      mval = Mval = 0;
      for(i=0;i<v->N;i++) {
	if (v->u.i16[i] < mval)
	  mval = v->u.i16[i];
	if (v->u.i16[i] > Mval)
	  Mval = v->u.i16[i];
      }

      if (Mval - mval < 32768) break;
      fprintf(stderr,"** aftervoxel 16-bit range workaround warning: dividing original intensities by 2 (original range %d - %d).\n",mval,Mval);
      for(i=0;i<v->N;i++) {
	v->u.i16[i] /= 2;
      }
    } 
    if (mval < 0)
      for(i=0;i<v->N;i++)
	v->u.i16[i] -= mval;
  } else {
    // unsigned data
    for(;;) {
      mval = Mval = 0;
      for(i=0;i<v->N;i++) {
	if (v->u.u16[i] < mval)
	  mval = v->u.u16[i];
	if (v->u.u16[i] > Mval)
	  Mval = v->u.u16[i];
      }

      if (Mval < 32768) break;
      fprintf(stderr,"** aftervoxel 16-bit range workaround warning: dividing original intensities by 2, to handle signedness properly (original range %d - %d).\n",mval,Mval);
      for(i=0;i<v->N;i++) {
	v->u.u16[i] /= 2;
      }
    } 
  }
    
  //  printf("%d %d %d %f %f %f\n",v->W,v->H,v->D,v->dx,v->dy,v->dz);

  MutexLock(voldata.masterlock);
  if (voldata.original)
    VolumeDestroy(voldata.original);
  voldata.original = v;
  notify.datachanged++;
  MutexUnlock(voldata.masterlock);

  EndProgress();
}

void dicom_init() {
  FILE *f;
  char conf[512],buf[512],*p;
  static char *icons[7] = { "home","hd","cd","net","pendrive","zip","mo" };

  int i,j,s;

  resource_path(conf,"aftervoxel.conf",512);
  

  if (strlen(conf)>0)
    f = fopen(conf,"r");
  else
    f = NULL;
  j = 0;
  if (f!=NULL) {
    while(fgets(buf,511,f)!=NULL)
      if (strncmp(buf,"dev=",4)==0) ++j;
    fseek(f,0,SEEK_SET);
  }

  devlist.n = 2+j;

  devlist.devs = (DicomDev *) MemAlloc(sizeof(DicomDev) * devlist.n);

  devlist.devs[0].icon = ICON_HOME;
  strncpy(devlist.devs[0].path,res.Home,512);
  strncpy(devlist.devs[0].label,"Home",32);

  devlist.devs[1].icon = ICON_HD;
  strncpy(devlist.devs[1].path,"/",512);
  strncpy(devlist.devs[1].label,"Root",32);
  
  i = 2;
  if (f!=NULL) {
    for(;;) {
      if (fgets(buf,511,f)==NULL) break;
      if (strncmp(buf,"dev=",4)==0) {
	p = strtok(&buf[4],";\n");
	if (!p) continue;
	devlist.devs[i].icon = ICON_NET;
	for(s=0;s<7;s++)
	  if (!strcmp(p,icons[s])) {
	    devlist.devs[i].icon = s;
	    break;
	  }
	p = strtok(0,";\n");
	if (!p) continue;
	strncpy(devlist.devs[i].label,p,32);
	p = strtok(0,";\n");
	if (!p) continue;
	strncpy(devlist.devs[i].path,p,512);
	++i;
      }
    }
    fclose(f);
  }
  if (i < devlist.n)
    devlist.n = i;
}

GtkWidget * dev_button_new(DicomDev *dev) {
  GtkWidget *b,*v,*i,*l,*d;
  char **xpm=0;
  char tag[1024],tmp[512];

  app_tdebug(3019);

  b = gtk_button_new();
  v = gtk_vbox_new(FALSE,0);
  
  switch(dev->icon) {
  case ICON_HOME:     xpm = dev_home_64_xpm; break;
  case ICON_HD:       xpm = dev_hd_64_xpm; break;
  case ICON_CD:       xpm = dev_cd_64_xpm; break;
  case ICON_NET:      xpm = dev_net_64_xpm; break;
  case ICON_PENDRIVE: xpm = dev_pendrive_64_xpm; break;
  case ICON_ZIP:      xpm = dev_zip_64_xpm; break;
  case ICON_MO:       xpm = dev_mo_64_xpm; break;
  }
  i = util_pixmap_new(mw,xpm);

  if (strlen(dev->path)>16)
    snprintf(tmp,512,"...%s",&(dev->path[strlen(dev->path)-14]));
  else
    strncpy(tmp,dev->path,512);

  snprintf(tag,1024,"<b>%s</b>\n<span size='small'>%s</span>",dev->label,tmp);
  l = gtk_label_new(" ");
  gtk_label_set_justify(GTK_LABEL(l),GTK_JUSTIFY_CENTER);
  gtk_label_set_markup(GTK_LABEL(l),tag);
  gtk_box_pack_start(GTK_BOX(v),i,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(v),l,FALSE,TRUE,0);

  d = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(d),64,1);
  gtk_box_pack_start(GTK_BOX(v),d,FALSE,TRUE,0);

  gtk_container_add(GTK_CONTAINER(b),v);
  gtk_widget_show_all(b);
  return b;
}

void di_update_isoinfo(int selchanged) {
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  GtkTreeModel *model;
  int W,H,D;
  unsigned int NW,NH,ND,NN,bytes;
  float dx,dy,dz,val,sx,sy,sz,megabytes,fbytes;
  gchar *id;
  char buf[512];

  app_tdebug(3020);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ctl.studies));
  if (gtk_tree_selection_get_selected(sel,&model,&iter)==FALSE ||
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctl.interpolate))==FALSE)
    {
      disable(ctl.isosize);
      gtk_label_set_text(GTK_LABEL(ctl.isoinfo), " ");
      return;
    }

  enable(ctl.isosize);

  // find slices
  gtk_tree_model_get(model,&iter,5,&id,-1);
  di_study_size(id,&W,&H,&D,&dx,&dy,&dz);
  g_free(id);

  if (selchanged) {
    val = MIN(MIN(dx,dy),dz);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctl.isosize),val);
  }

  val = (float) gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctl.isosize));
  sx = ((float)W)*dx/val;
  sy = ((float)H)*dy/val;
  sz = ((float)D)*dz/val;
  NW = (unsigned int) sx;
  NH = (unsigned int) sy;
  ND = (unsigned int) sz;
  NN = NW*NH*ND;

  fbytes = ((float)NN) * 5.0;

  bytes = NN*3; /* 2 original, 1 label */
  megabytes = ((float)bytes) / (1024.0*1024.0);

  if (fbytes < (4.0*1024.0*1024.0*1024.0-100.0*1024.0*1024.0))
    snprintf(buf,512,"Size: %dx%dx%d.\nMemory Required: %.2f MB.",NW,NH,ND,megabytes);
  else
    snprintf(buf,512,"Size: %dx%dx%d.\nMemory Required: > 4GB (forbidden)",NW,NH,ND);
  gtk_label_set_text(GTK_LABEL(ctl.isoinfo), buf);
} 

void di_size(GtkSpinButton *sb, gpointer data) {
  di_update_isoinfo(0);
}

void di_interp(GtkToggleButton *tb, gpointer data) {
  di_update_isoinfo(0);
}

void di_study_size(char *id, int *W,int *H,int *D,
		   float *dx,float *dy,float *dz)
{
  int i,j,n,d;
  DicomSlice **ds;

  n = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id))
      n++;

  ds = (DicomSlice **) MemAlloc(n * sizeof(DicomSlice *));
  if (!ds) {
    *W = 1;
    *H = 1;
    *D = 1;
    *dx = 1.0;
    *dy = 1.0;
    *dz = 1.0;
    error_memory();
    return;
  }

  j = 0;
  d = 0;
  for(i=0;i<ctl.slicecount;i++)
    if (!strcmp(DicomSliceGet(ctl.slices[i], DICOM_FIELD_STUDYID),id)) {
      ds[j] = ctl.slices[i];
      d += ds[j]->D;
      j++;
    }
  qsort(ds, n, sizeof(DicomSlice *), &slice_ptr_zcmp);

  *W = ds[0]->W;
  *H = ds[0]->H;
  *D = d;

  *dx = ds[0]->xsize;
  *dy = ds[0]->ysize;

  if (n>1 && DicomSliceDistance(ds[0],ds[1]) > 0.0f) {
    *dz = DicomSliceDistance(ds[0],ds[1]);
    for(i=0;i<n;i++)
      ds[i]->zsize = *dz;
    //printf("dz1=%f\n",*dz);
  } else {
    *dz = ds[0]->zsize;
    //printf("dz2=%f\n",*dz);
  }

  MemFree(ds);
}
