 
// crop/clip filter (GUI)
// this file is #included from preproc.c

typedef struct _cropf {
  GtkAdjustment *va[6];
  GtkWidget     *vw[6];
  GtkWidget     *label;
  int            toid;
} CropFilter;

/* activation/update callback */
void filter_crop_activate_cb(PreProcFilter *ppf);
void filter_crop_update_cb(PreProcFilter *ppf);

/* deactivation callback */
void filter_crop_deactivate_cb(PreProcFilter *ppf);

/* spinbutton changes */
void filter_crop_parchange(GtkAdjustment *a, gpointer data);

/* composes the label text */
void filter_crop_calc_label(PreProcFilter *ppf);

/* magic function */
int filter_crop_whatever(PreProcFilter *ppf, int op, int ab, 
			 int x, int y, int z);

/* final application of the filter */
void filter_crop_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_crop_create(int id) {
  PreProcFilter *pp;
  CropFilter *f;
  GtkWidget *apply;
  int i;

  pp = preproc_filter_new();

  pp->id = id;
  f = CREATE(CropFilter);
  pp->data = (void *) f;

  pp->activate   = filter_crop_activate_cb;
  pp->deactivate = filter_crop_deactivate_cb;
  pp->update     = filter_crop_update_cb;
  pp->whatever   = filter_crop_whatever;

  pp->control = 
    guip_create("frame[title='Region Clip',border=2](vbox[gap=2,border=2]("\
		"table[w=4,h=2]{gap=2}("\
		"label[text='corner A']{l=0,t=0},"\
		"label[text='corner B']{l=0,t=1},"\
		"ax:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=1,t=0},"\
		"ay:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=2,t=0},"\
		"az:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=3,t=0},"\
		"bx:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=1,t=1},"\
		"by:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=2,t=1},"\
		"bz:spinbutton[val=0.0,min=0.0,max=4096.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=3,t=1}),"\
		"label[text='Use the left and right mouse\nbuttons to select corners on\nthe slice images.',left],"\
		"frame[border=4](hbox[gap=0,border=4](l1:label[text=' ',left])),"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  f->va[0] = guip_get_adj("ax");
  f->va[1] = guip_get_adj("ay");
  f->va[2] = guip_get_adj("az");
  f->va[3] = guip_get_adj("bx");
  f->va[4] = guip_get_adj("by");
  f->va[5] = guip_get_adj("bz");

  f->vw[0] = guip_get("ax");
  f->vw[1] = guip_get("ay");
  f->vw[2] = guip_get("az");
  f->vw[3] = guip_get("bx");
  f->vw[4] = guip_get("by");
  f->vw[5] = guip_get("bz");

  f->label = guip_get("l1");

  f->toid = -1;

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_crop_apply), (gpointer) pp);

  for(i=0;i<6;i++)
    gtk_signal_connect(GTK_OBJECT(f->va[i]), "value_changed",
		       GTK_SIGNAL_FUNC(filter_crop_parchange),
		       (gpointer) pp);

  return pp;
}

void filter_crop_activate_cb(PreProcFilter *ppf) {
  filter_crop_update_cb(ppf);
  if (ppf->repaint)
    ppf->repaint(ppf);
}

void filter_crop_update_cb(PreProcFilter *ppf) {
  CropFilter *f = (CropFilter *) (ppf->data);

  int i,w,h,d;

  w = (int) (f->va[0]->upper);
  h = (int) (f->va[1]->upper);
  d = (int) (f->va[2]->upper);

  if (!scene->avol) {
    for(i=0;i<6;i++)
      gtk_widget_set_sensitive(f->vw[i],FALSE);
    filter_crop_calc_label(ppf);
    return;
  }

  for(i=0;i<6;i++)
    gtk_widget_set_sensitive(f->vw[i],TRUE);

  if (scene->avol->W != w) {
    f->va[0]->upper = scene->avol->W;
    f->va[3]->upper = scene->avol->W;
    gtk_adjustment_changed(f->va[0]);
    gtk_adjustment_changed(f->va[3]);
  }

  if (scene->avol->H != h) {
    f->va[1]->upper = scene->avol->H;
    f->va[4]->upper = scene->avol->H;
    gtk_adjustment_changed(f->va[1]);
    gtk_adjustment_changed(f->va[4]);
  }

  if (scene->avol->D != d) {
    f->va[2]->upper = scene->avol->D;
    f->va[5]->upper = scene->avol->D;
    gtk_adjustment_changed(f->va[2]);
    gtk_adjustment_changed(f->va[5]);
  }

  for(i=0;i<6;i++) {
    if(f->va[i]->value < f->va[i]->lower)
      gtk_adjustment_set_value(f->va[i],f->va[i]->lower);
    if(f->va[i]->value > f->va[i]->upper)
      gtk_adjustment_set_value(f->va[i],f->va[i]->upper);
  }

  filter_crop_calc_label(ppf);
}

void filter_crop_calc_label(PreProcFilter *ppf) {
  CropFilter *f = (CropFilter *) (ppf->data);
  char z[1024];

  int w,h,d,nw,nh,nd;
  float sN, dN;

  if (!scene->avol) {
    strcpy(z,"No volume loaded.");
    gtk_label_set_text(GTK_LABEL(f->label), z);
    return;
  }

  w = scene->avol->W;
  h = scene->avol->H;
  d = scene->avol->D;
  
  nw = (int)(f->va[0]->value) - (int)(f->va[3]->value);
  nh = (int)(f->va[1]->value) - (int)(f->va[4]->value);
  nd = (int)(f->va[2]->value) - (int)(f->va[5]->value);

  if (nw < 0) nw = -nw;
  if (nh < 0) nh = -nh;
  if (nd < 0) nd = -nd;

  sN = (float)(2*w*h*d);
  dN = (float)(2*nw*nh*nd);

  sN /= 1024.0 * 1024.0;
  dN /= 1024.0 * 1024.0;

  sprintf(z,"Est. current volume file size: %.1f MB\n"\
	  "Est. new volume file size: %.1f MB\n"\
	  "New volume dimensions: %d x %d x %d",
	  sN,dN,nw,nh,nd);
  gtk_label_set_text(GTK_LABEL(f->label), z);
}

gboolean filter_crop_timeout(gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  CropFilter *f = (CropFilter *) (ppf->data);

  f->toid = -1;
  ppf->repaint(ppf);

  return FALSE;
}

void filter_crop_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  CropFilter *f = (CropFilter *) (ppf->data);

  filter_crop_calc_label(ppf);

  if (f->toid >= 0) {
    gtk_timeout_remove(f->toid);
    f->toid = -1;
  }

  f->toid = gtk_timeout_add(500, filter_crop_timeout, data);
}

int filter_crop_whatever(PreProcFilter *ppf, int op, int ab, 
			 int x, int y, int z)
{
  CropFilter *f = (CropFilter *) (ppf->data);
  int v;

  if (op==0) {
    // WRITE

    if (x >= 0)
      gtk_adjustment_set_value(f->va[ab*3 + 0], (float) x);
    if (y >= 0)
      gtk_adjustment_set_value(f->va[ab*3 + 1], (float) y);
    if (z >= 0)
      gtk_adjustment_set_value(f->va[ab*3 + 2], (float) z);
    return 0;

  } else {
    // READ

    v = (int) (f->va[ab*3 + x]->value);
    return v;
  }
}

void filter_crop_deactivate_cb(PreProcFilter *ppf) {
  if (ppf->repaint)
    ppf->repaint(ppf);
}

void filter_crop_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  CropFilter *f = (CropFilter *) (ppf->data);
  ArgBlock *args;
  int i;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_newdata);

  for(i=0;i<6;i++)
    args->ia[i] = (int) (f->va[i]->value);

  TaskNew(qseg, "Filter: volume clip", (Callback) &be_clip_volume, (void *) args);
}
