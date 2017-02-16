 
// interpolation filter (GUI)
// this file is #included from preproc.c

typedef struct _ifilter {
  GtkAdjustment *va[3];
  GtkWidget     *vw[3];
  GtkWidget     *label[2];
  RadioBox      *mode;
  int            ignore;
} IFilter;

PreProcFilter *ugly_global_ippf = 0;

/* activation/update callback */
void filter_interp_activate_cb(PreProcFilter *ppf);
/* radiobox callback */
void filter_interp_mode_cb(void *x);

/* recalcs all the text */
void filter_interp_recalc_labels(PreProcFilter *ppf);

/* voxel size changes */
void filter_interp_parchange(GtkAdjustment *a, gpointer data);

/* final application of the filter */
void filter_interp_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_interp_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;
  IFilter *f;
  int i;

  pp = preproc_filter_new();

  ugly_global_ippf = pp;

  pp->id = id;

  f = CREATE(IFilter);
  pp->data = (void *) f;

  pp->activate = filter_interp_activate_cb;
  pp->update   = filter_interp_activate_cb;

  pp->control = 
    guip_create("frame[title='Linear Interpolation',border=2](vbox[gap=2,border=2]("\
		"rb:radiobox[title='Dimension',options=';Isotropic, interpolate to smaller voxel dimension;Isotropic, interpolate to largest voxel dimension;User-defined'],"\
		"l1:label[text='current voxel size:',center],"\
		"hbox[gap=1,border=2]{expand}("\
		"label[text='new voxel size:'],"\
		"vx:spinbutton[val=0.01,min=0.01,max=20.0,step=0.10,page=1.0,pagelen=0.0,accel=1.5,digits=4]{fill,expand},"\
		"vy:spinbutton[val=0.01,min=0.01,max=20.0,step=0.10,page=1.0,pagelen=0.0,accel=1.5,digits=4]{fill,expand},"\
		"vz:spinbutton[val=0.01,min=0.01,max=20.0,step=0.10,page=1.0,pagelen=0.0,accel=1.5,digits=4]{fill,expand}),"\
		"frame[border=2](l2:label[text=' ',left]),"\
		"label[text='This filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  f->va[0] = guip_get_adj("vx");
  f->va[1] = guip_get_adj("vy");
  f->va[2] = guip_get_adj("vz");

  f->vw[0] = guip_get("vx");
  f->vw[1] = guip_get("vy");
  f->vw[2] = guip_get("vz");

  f->label[0] = guip_get("l1");
  f->label[1] = guip_get("l2");

  f->mode = (RadioBox *) guip_get_ext("rb");
  f->ignore = 0;

  radiobox_set_changed_callback(f->mode, filter_interp_mode_cb);

  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_interp_apply), (gpointer) pp);

  for(i=0;i<3;i++)
    gtk_signal_connect(GTK_OBJECT(f->va[i]), "value_changed",
		       GTK_SIGNAL_FUNC(filter_interp_parchange),
		       (gpointer) pp);

  return pp;
}

void filter_interp_activate_cb(PreProcFilter *ppf) {
  IFilter *f = (IFilter *) (ppf->data);
  int mode,i;
  float vx,vy,vz,G,L;

  mode = radiobox_get_selection(f->mode);

  if (mode == 2) {
    // free form controls enabled
    gtk_widget_set_sensitive(f->vw[0], TRUE);
    gtk_widget_set_sensitive(f->vw[1], TRUE);
    gtk_widget_set_sensitive(f->vw[2], TRUE);
  } else {
    gtk_widget_set_sensitive(f->vw[0], FALSE);
    gtk_widget_set_sensitive(f->vw[1], FALSE);
    gtk_widget_set_sensitive(f->vw[2], FALSE);
  }

  if (scene->avol) {
    vx = scene->avol->dx;
    vy = scene->avol->dy;
    vz = scene->avol->dz;

    G = vx; if (vy>G) G=vy; if (vz>G) G=vz;
    L = vx; if (vy<L) L=vy; if (vz<L) L=vz;

    if (mode == 0) {
      for(i=0;i<3;i++) {
	++(f->ignore);
	gtk_adjustment_set_value(f->va[i], L);
      }
    }
    if (mode == 1) {
      for(i=0;i<3;i++) {
	++(f->ignore);
	gtk_adjustment_set_value(f->va[i], G);
      }
    }
  }

  filter_interp_recalc_labels(ppf);
}

void filter_interp_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  IFilter *f = (IFilter *) pp->data;

  if (f->ignore > 0) {
    --(f->ignore);
    return;
  }

  filter_interp_activate_cb(pp);
}

void filter_interp_recalc_labels(PreProcFilter *ppf) {
  IFilter *f = (IFilter *) (ppf->data);
  float vx,vy,vz;
  float nx,ny,nz;
  int W,H,D, nW, nH, nD;
  float sN, dN, ratio;
  
  char z[1024];

  if (!scene->avol) {
    gtk_label_set_text(GTK_LABEL(f->label[0]),
		       "No volume loaded.");
    gtk_label_set_text(GTK_LABEL(f->label[1]),
		       "No volume loaded.");
  } else {

    vx = scene->avol->dx;
    vy = scene->avol->dy;
    vz = scene->avol->dz;

    sprintf(z,"Current voxel size: %.4f x %.4f x %.4f",
	    vx,vy,vz);
    
    gtk_label_set_text(GTK_LABEL(f->label[0]),z);

    nx = f->va[0]->value;
    ny = f->va[1]->value;
    nz = f->va[2]->value;

    W = scene->avol->W;
    H = scene->avol->H;
    D = scene->avol->D;

    nW = (int)((float)(W-1)*vx/nx)+1;
    nH = (int)((float)(H-1)*vy/ny)+1;
    nD = (int)((float)(D-1)*vz/nz)+1;

    sN = (float) (2*W*H*D);
    dN = (float) (2*nW*nH*nD);

    sN /= 1024.0 * 1024.0;
    dN /= 1024.0 * 1024.0;

    ratio = (dN / sN) * 100.0;

    sprintf(z,"Est. file size for current volume: %.1f MB\n"\
	    "Est. file size for new volume: %.1f MB\n"\
	    "Volume Ratio: %.1f %%\n"\
	    "Scaling in each dimension: %.2f x %.2f x %.2f\n"\
	    "New volume dimensions: %d x %d x %d",
	    sN,dN,ratio,vx/nx,vy/ny,vz/nz,nW,nH,nD);

    gtk_label_set_text(GTK_LABEL(f->label[1]),z);

  }
}

void filter_interp_mode_cb(void *x) {
  filter_interp_activate_cb(ugly_global_ippf);
}

void filter_interp_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  IFilter *f = (IFilter *) (pp->data);
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_newdata);
  args->fa[0] = f->va[0]->value;
  args->fa[1] = f->va[1]->value;
  args->fa[2] = f->va[2]->value;

  TaskNew(qseg, "Filter: linear interpolation", (Callback) &be_interpolate_volume, (void *) args);
}
