
// multi-band gaussian stretch filter (GUI)
// this file is #included from preproc.c

typedef struct _mbgsfilter {
  GtkAdjustment *ma[5], *da[5], *nba;
  GtkWidget     *mw[5], *dw[5], *nbw;
  GtkWidget     *preview;
  RCurve        *curve;
  i16_t         *func;
  i32_t         *hist;
  int            histsync;
  int            toid;
} mbgs_filter;

/* called when the bands, mean or deviation controls change */
void filter_mbgs_parchange(GtkAdjustment *a, gpointer data);

/* called when the preview checkbox changes */
void filter_mbgs_previewchange(GtkToggleButton *w, gpointer data);

/* timeout-called preview calculation */
gboolean filter_mbgs_preview(gpointer data);

/* final application of the filter */
void filter_mbgs_apply(GtkWidget *w, gpointer data);

/* preview update callback (called when cursor position changes) */
void filter_mbgs_previewchange_cb(PreProcFilter *ppf);

/* activation/update callback */
void filter_mbgs_activate_cb(PreProcFilter *ppf);
void filter_mbgs_update_cb(PreProcFilter *ppf);

PreProcFilter * filter_mbgs_create(int id) {
  PreProcFilter *pp;
  mbgs_filter *mbgs;
  GtkWidget *apply;
  int i;
  char z[3];

  pp = preproc_filter_new();
  mbgs = CREATE(mbgs_filter);

  pp->id = id;
  pp->data = mbgs;

  pp->updatepreview = filter_mbgs_previewchange_cb;
  pp->activate      = filter_mbgs_activate_cb;
  pp->update        = filter_mbgs_update_cb;

  pp->control =
    guip_create("frame[title='Multi-Band Gaussian Stretch',border=2]("\
		"vbox[gap=2,border=2]("\
		"hbox[gap=2]{gap=2}(label[text='Bands:'],"\
		"bands:spinbutton[val=5,min=2,max=5,step=1,page=2,pagelen=0]),"\
		"table[w=2,h=6]{gap=2}("\
		"label[text='mean']{l=0,t=0},"\
		"label[text='std.dev.']{l=1,t=0},"\
		"m1:spinbutton[val=0,min=0,max=32768,step=1,page=10,pagelen=0,"\
		"accel=1.5]{l=0,t=1},"\
		"s1:spinbutton[val=32,min=0,max=16384,step=1,page=5,pagelen=0,"\
		"accel=1.5]{l=1,t=1},"\
		"m2:spinbutton[val=64,min=0,max=32768,step=1,page=10,pagelen=0,"\
		"accel=1.5]{l=0,t=2},"\
		"s2:spinbutton[val=32,min=0,max=16384,step=1,page=5,pagelen=0,"\
		"accel=1.5]{l=1,t=2},"\
		"m3:spinbutton[val=128,min=0,max=32768,step=1,page=10,pagelen=0,"\
		"accel=1.5]{l=0,t=3},"\
		"s3:spinbutton[val=32,min=0,max=16384,step=1,page=5,pagelen=0,"\
		"accel=1.5]{l=1,t=3},"\
		"m4:spinbutton[val=192,min=0,max=32768,step=1,page=10,pagelen=0,"\
		"accel=1.5]{l=0,t=4},"\
		"s4:spinbutton[val=32,min=0,max=16384,step=1,page=5,pagelen=0,"\
		"accel=1.5]{l=1,t=4},"\
		"m5:spinbutton[val=255,min=0,max=32768,step=1,page=10,pagelen=0,"\
		"accel=1.5]{l=0,t=5},"\
		"s5:spinbutton[val=32,min=0,max=16384,step=1,page=5,pagelen=0,"\
		"accel=1.5]{l=1,t=5}),"\
		"func:curve{expand,fill},"\
		"preview:checkbox[text='Preview'],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");


  z[2] = 0;
  for(i=0;i<5;i++) {
    z[0] = 'm'; z[1] = '1' + i;
    mbgs->ma[i] = guip_get_adj(z);
    mbgs->mw[i] = guip_get(z);
    z[0] = 's';
    mbgs->da[i] = guip_get_adj(z);
    mbgs->dw[i] = guip_get(z);
  }

  mbgs->nba = guip_get_adj("bands");
  mbgs->nbw = guip_get("bands");

  mbgs->curve = guip_get_ext("func");
  mbgs->preview = guip_get("preview");
  mbgs->toid = -1;
  mbgs->hist = (i32_t *) malloc(SDOM * sizeof(i32_t));
  mbgs->histsync = -1;

  apply = guip_get("apply");

  mbgs->func = 0;

  gtk_signal_connect(GTK_OBJECT(mbgs->nba), "value_changed",
		     GTK_SIGNAL_FUNC(filter_mbgs_parchange), (gpointer) pp);
  for(i=0;i<5;i++) {
    gtk_signal_connect(GTK_OBJECT(mbgs->ma[i]), "value_changed",
		       GTK_SIGNAL_FUNC(filter_mbgs_parchange), (gpointer) pp);
    gtk_signal_connect(GTK_OBJECT(mbgs->da[i]), "value_changed",
		       GTK_SIGNAL_FUNC(filter_mbgs_parchange), (gpointer) pp);
  }

  gtk_signal_connect(GTK_OBJECT(mbgs->preview), "toggled",
		     GTK_SIGNAL_FUNC(filter_mbgs_previewchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_mbgs_apply), (gpointer) pp);
  return pp;
}

void filter_mbgs_activate_cb(PreProcFilter *ppf) {
  mbgs_filter *mbgs = (mbgs_filter *) (ppf->data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mbgs->preview), FALSE);
  filter_mbgs_update_cb(ppf);
}

void filter_mbgs_update_cb(PreProcFilter *ppf) {
  mbgs_filter *mbgs = (mbgs_filter *) (ppf->data);
  int i;
  float m[5], d[5];

  if (mbgs->func != 0) {
    rcurve_set_function(mbgs->curve,0,0,0);
    free(mbgs->func);
    mbgs->func = 0;
  }

  if (scene->avol != 0) {
    for(i=0;i<5;i++) {
      m[i] = mbgs->ma[i]->value;
      d[i] = mbgs->da[i]->value;
    }
    mbgs->func = XA_MBGS_Function((int)(mbgs->nba->value),m,d);
    rcurve_set_function(mbgs->curve, mbgs->func, 0, 32767);
    mbgs->curve->ymax = SDOM;
    mbgs->curve->ymin = 0;

    if (preproc.ppspin > mbgs->histsync) {
      MemSet(mbgs->hist,0,sizeof(i32_t) * SDOM);
      for(i=0;i<scene->avol->N;i++)
	++(mbgs->hist[scene->avol->vd[i].value]);

      rcurve_set_histogram(mbgs->curve, mbgs->hist);
      mbgs->histsync = preproc.ppspin;
    }
  }
  refresh(mbgs->curve->widget);
}

gboolean filter_mbgs_preview(gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  mbgs_filter *mbgs = (mbgs_filter *) (pp->data);
  int i;
  float m[5],d[5];
  Volume *preview;

  mbgs->toid = -1;
  // apply filter on these slices

  for(i=0;i<5;i++) {
    m[i] = mbgs->ma[i]->value;
    d[i] = mbgs->da[i]->value;
  }
  preview = preproc_prepare_preview();
  XA_MBGS_Filter_Preview(preview, 
			 (int)(mbgs->nba->value),
			 m,d,
			 *(pp->xslice),
			 *(pp->yslice),
			 *(pp->zslice));

  preproc.gotpreview = 1;
  if (pp->repaint)
    pp->repaint(pp);  
  return FALSE;
}

void filter_mbgs_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  mbgs_filter *mbgs = (mbgs_filter *) (pp->data);
  int i,j;

  filter_mbgs_update_cb(pp);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mbgs->preview))) {
    if (mbgs->toid >= 0)
      gtk_timeout_remove(mbgs->toid);
    mbgs->toid = gtk_timeout_add(500, filter_mbgs_preview, data);
  }

  if (a == mbgs->nba) {
    j = (int) (mbgs->nba->value);
    for(i=0;i<5;i++)
      if (i < j) {
	gtk_widget_show(mbgs->mw[i]);
	gtk_widget_show(mbgs->dw[i]);
      } else {
	gtk_widget_hide(mbgs->mw[i]);
	gtk_widget_hide(mbgs->dw[i]);
      }
  }
}

void filter_mbgs_previewchange_cb(PreProcFilter *ppf) {
  mbgs_filter *mbgs = (mbgs_filter *) (ppf->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mbgs->preview));

  if (s) {
    if (mbgs->toid >= 0)
      gtk_timeout_remove(mbgs->toid);
    mbgs->toid = gtk_timeout_add(500, filter_mbgs_preview, (gpointer)(ppf));
  }

}

void filter_mbgs_previewchange(GtkToggleButton *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  mbgs_filter *mbgs = (mbgs_filter *) (pp->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mbgs->preview));

  if (!s) {
    if (mbgs->toid >=0 ) {
      gtk_timeout_remove(mbgs->toid);
      mbgs->toid = -1;
    }
    preproc.gotpreview = 0;
    if (pp->repaint)
      pp->repaint(pp);
  } else {
    if (mbgs->toid >= 0)
      gtk_timeout_remove(mbgs->toid);
    mbgs->toid = gtk_timeout_add(500, filter_mbgs_preview, data);
  }
}

void filter_mbgs_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  mbgs_filter *mbgs = (mbgs_filter *) (pp->data);
  ArgBlock *args;
  int i;

  args = ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->ia[0] = (int) (mbgs->nba->value);

  for(i=0;i<args->ia[0];i++) {
    args->fa[i] = mbgs->ma[i]->value;
    args->fa[5+i] = mbgs->da[i]->value;
  }

  preproc.gotpreview = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mbgs->preview), FALSE);
  TaskNew(qseg, "Filter: multi-band GSF", (Callback) &be_filter_mbgs, (void *) args);
}
