
// gaussian stretch filter (GUI)
// this file is #included from preproc.c

typedef struct _gsfilter {
  GtkAdjustment *ma, *da;
  GtkWidget     *mw, *dw;
  GtkWidget     *preview;
  RCurve        *curve;
  i16_t         *func;
  i32_t         *hist;
  int            histsync;
  int            toid;
} gs_filter;

/* called when the mean or deviation controls change */
void filter_gs_parchange(GtkAdjustment *a, gpointer data);

/* called when the preview checkbox changes */
void filter_gs_previewchange(GtkToggleButton *w, gpointer data);

/* timeout-called preview calculation */
gboolean filter_gs_preview(gpointer data);

/* final application of the filter */
void filter_gs_apply(GtkWidget *w, gpointer data);

/* preview update callback (called when cursor position changes) */
void filter_gs_previewchange_cb(PreProcFilter *ppf);

/* activation/update callback */
void filter_gs_activate_cb(PreProcFilter *ppf);
void filter_gs_update_cb(PreProcFilter *ppf);

PreProcFilter * filter_gs_create(int id) {
  PreProcFilter *pp;
  gs_filter *gs;
  GtkWidget *apply;

  pp = preproc_filter_new();
  gs = CREATE(gs_filter);

  pp->id = id;
  pp->data = gs;

  pp->updatepreview = filter_gs_previewchange_cb;
  pp->activate      = filter_gs_activate_cb;
  pp->update        = filter_gs_update_cb;

  pp->control = 
    guip_create("frame[title='Gaussian Stretch',border=2](vbox[gap=2,border=2]("\
		"table[w=2,h=2]{gap=2}("\
		"label[text='mean']{l=0,r=1,t=0,b=1},"\
		"label[text='std.deviation']{l=0,r=1,t=1,b=2},"\
		"mean:spinbutton[val=128.0,min=0.0,max=32768.0,"\
		"step=1.0,page=10.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=0,b=1},"\
		"stddev:spinbutton[val=128.0,min=0.0,max=16384.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=1,b=2}),"\
		"func:curve{expand,fill},"\
		"preview:checkbox[text='Preview'],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  gs->ma = guip_get_adj("mean");
  gs->da = guip_get_adj("stddev");

  gs->mw = guip_get("mean");
  gs->dw = guip_get("stddev");
  gs->curve = guip_get_ext("func");
  gs->preview = guip_get("preview");
  gs->toid = -1;
  gs->hist = (i32_t *) malloc(SDOM * sizeof(i32_t));
  gs->histsync = -1;

  apply = guip_get("apply");

  gs->func = 0;

  gtk_signal_connect(GTK_OBJECT(gs->ma), "value_changed",
		     GTK_SIGNAL_FUNC(filter_gs_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(gs->da), "value_changed",
		     GTK_SIGNAL_FUNC(filter_gs_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(gs->preview), "toggled",
		     GTK_SIGNAL_FUNC(filter_gs_previewchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_gs_apply), (gpointer) pp);
  return pp;
}

void filter_gs_activate_cb(PreProcFilter *ppf) {
  gs_filter *gs = (gs_filter *) (ppf->data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gs->preview), FALSE);
  filter_gs_update_cb(ppf);
}

void filter_gs_update_cb(PreProcFilter *ppf) {
  gs_filter *gs = (gs_filter *) (ppf->data);
  int i;

  if (gs->func != 0) {
    rcurve_set_function(gs->curve,0,0,0);
    free(gs->func);
    gs->func = 0;
  }

  if (scene->avol != 0) {
    gs->func = XA_GS_Function(gs->ma->value,gs->da->value);
    rcurve_set_function(gs->curve, gs->func, 0, 32767);
    gs->curve->ymax = SDOM;
    gs->curve->ymin = 0;

    if (preproc.ppspin > gs->histsync) {
      MemSet(gs->hist,0,sizeof(i32_t) * SDOM);
      for(i=0;i<scene->avol->N;i++)
	++(gs->hist[scene->avol->vd[i].value]);

      rcurve_set_histogram(gs->curve, gs->hist);
      gs->histsync = preproc.ppspin;
    }
  }
  refresh(gs->curve->widget);
}

gboolean filter_gs_preview(gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  gs_filter *gs = (gs_filter *) (pp->data);
  Volume *preview;

  gs->toid = -1;

  // apply filter on these slices

  preview = preproc_prepare_preview();
  XA_GS_Filter_Preview(preview, 
		       gs->ma->value,
		       gs->da->value,
		       *(pp->xslice),
		       *(pp->yslice),
		       *(pp->zslice));

  preproc.gotpreview = 1;

  if (pp->repaint)
    pp->repaint(pp);  
  return FALSE;
}

void filter_gs_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  gs_filter *gs = (gs_filter *) (pp->data);

  filter_gs_update_cb(pp);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gs->preview))) {
    if (gs->toid >= 0)
      gtk_timeout_remove(gs->toid);
    gs->toid = gtk_timeout_add(500, filter_gs_preview, data);
  }
}

void filter_gs_previewchange_cb(PreProcFilter *ppf) {
  gs_filter *gs = (gs_filter *) (ppf->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gs->preview));

  if (s) {
    if (gs->toid >= 0)
      gtk_timeout_remove(gs->toid);
    gs->toid = gtk_timeout_add(500, filter_gs_preview, (gpointer)(ppf));
  }

}

void filter_gs_previewchange(GtkToggleButton *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  gs_filter *gs = (gs_filter *) (pp->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gs->preview));

  if (!s) {
    if (gs->toid >=0 ) {
      gtk_timeout_remove(gs->toid);
      gs->toid = -1;
    }
    preproc.gotpreview = 0;
    if (pp->repaint)
      pp->repaint(pp);
  } else {
    if (gs->toid >= 0)
      gtk_timeout_remove(gs->toid);
    gs->toid = gtk_timeout_add(500, filter_gs_preview, data);
  }
}

void filter_gs_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  gs_filter *gs = (gs_filter *) (pp->data);
  ArgBlock *args;

  args = ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->fa[0] = gs->ma->value;
  args->fa[1] = gs->da->value;

  preproc.gotpreview = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gs->preview), FALSE);
  TaskNew(qseg, "Filter: gaussian stretch", (Callback) &be_filter_gs, (void *) args);
}
