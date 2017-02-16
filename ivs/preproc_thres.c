 
// threshold filter (GUI)
// this file is #included from preproc.c

typedef struct _tfilter {
  GtkAdjustment *la, *ua;
  GtkWidget     *lw, *uw;
  GtkWidget     *preview;
  RCurve        *curve;
  i16_t         *func;
  i32_t         *hist;
  int            histsync;
  int            toid;
} t_filter;

/* called when the threshold controls change */
void filter_thres_parchange(GtkAdjustment *a, gpointer data);

/* called when the preview checkbox changes */
void filter_thres_previewchange(GtkToggleButton *w, gpointer data);

/* timeout-called preview calculation */
gboolean filter_thres_preview(gpointer data);

/* final application of the filter */
void filter_thres_apply(GtkWidget *w, gpointer data);

/* preview update callback (called when cursor position changes) */
void filter_thres_previewchange_cb(PreProcFilter *ppf);

/* activation/update callback */
void filter_thres_activate_cb(PreProcFilter *ppf);
void filter_thres_update_cb(PreProcFilter *ppf);

PreProcFilter * filter_thres_create(int id) {
  PreProcFilter *pp;
  t_filter *ls;
  GtkWidget *apply;

  pp = preproc_filter_new();
  ls = CREATE(t_filter);

  pp->id = id;
  pp->data = ls;

  pp->updatepreview = filter_thres_previewchange_cb;
  pp->activate      = filter_thres_activate_cb;
  pp->update        = filter_thres_update_cb;

  pp->control = 
    guip_create("frame[title='Threshold',border=2](vbox[gap=2,border=2]("\
		"table[w=2,h=2]{gap=2}("\
		"label[text='lower threshold']{l=0,r=1,t=0,b=1},"\
		"label[text='upper threshold']{l=0,r=1,t=1,b=2},"\
		"lo:spinbutton[val=0.0,min=0.0,max=32768.0,"\
		"step=1.0,page=10.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=0,b=1},"\
		"hi:spinbutton[val=0.0,min=0.0,max=32768.0,"\
		"step=1.0,page=10.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=1,b=2}),"\
		"func:curve{expand,fill},"\
		"preview:checkbox[text='Preview'],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  ls->la = guip_get_adj("lo");
  ls->ua = guip_get_adj("hi");

  ls->lw = guip_get("lo");
  ls->uw = guip_get("hi");
  ls->curve = guip_get_ext("func");
  ls->preview = guip_get("preview");
  ls->toid = -1;
  ls->hist = (i32_t *) malloc(SDOM * sizeof(i32_t));
  ls->histsync = -1;

  apply = guip_get("apply");

  ls->func = 0;

  gtk_signal_connect(GTK_OBJECT(ls->la), "value_changed",
		     GTK_SIGNAL_FUNC(filter_thres_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(ls->ua), "value_changed",
		     GTK_SIGNAL_FUNC(filter_thres_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(ls->preview), "toggled",
		     GTK_SIGNAL_FUNC(filter_thres_previewchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_thres_apply), (gpointer) pp);
  return pp;
}

void filter_thres_activate_cb(PreProcFilter *ppf) {
  t_filter *ls = (t_filter *) (ppf->data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ls->preview), FALSE);
  filter_thres_update_cb(ppf);
}

void filter_thres_update_cb(PreProcFilter *ppf) {
  t_filter *ls = (t_filter *) (ppf->data);
  int i;

  if (ls->func != 0) {
    rcurve_set_function(ls->curve,0,0,0);
    free(ls->func);
    ls->func = 0;
  }

  if (scene->avol != 0) {
    ls->func = XA_Threshold_Function(ls->la->value,ls->ua->value);
    rcurve_set_function(ls->curve, ls->func, 0, preproc.bounds.vmax);

    if (preproc.ppspin > ls->histsync) {
      MemSet(ls->hist,0,sizeof(i32_t) * SDOM);
      for(i=0;i<scene->avol->N;i++)
	++(ls->hist[scene->avol->vd[i].value]);

      rcurve_set_histogram(ls->curve, ls->hist);
      ls->histsync = preproc.ppspin;
    }
  }
  refresh(ls->curve->widget);
}

gboolean filter_thres_preview(gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  t_filter *ls = (t_filter *) (pp->data);
  Volume *preview;

  ls->toid = -1;

  // apply filter on these slices

  preview = preproc_prepare_preview();
  XA_Threshold_Filter_Preview(preview, 
			      ls->la->value,
			      ls->ua->value,
			      *(pp->xslice),
			      *(pp->yslice),
			      *(pp->zslice));
  preproc.gotpreview = 1;
  if (pp->repaint)
    pp->repaint(pp);  
  return FALSE;
}

void filter_thres_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  t_filter *ls = (t_filter *) (pp->data);

  if (ls->la->value > ls->ua->value) {
    if (a!=ls->la)
      gtk_adjustment_set_value(ls->la, ls->ua->value);
    if (a!=ls->ua)
      gtk_adjustment_set_value(ls->ua, ls->la->value);
  }

  filter_thres_update_cb(pp);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ls->preview))) {
    if (ls->toid >= 0)
      gtk_timeout_remove(ls->toid);
    ls->toid = gtk_timeout_add(500, filter_thres_preview, data);
  }
}

void filter_thres_previewchange_cb(PreProcFilter *ppf) {
  t_filter *ls = (t_filter *) (ppf->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ls->preview));

  if (s) {
    if (ls->toid >= 0)
      gtk_timeout_remove(ls->toid);
    ls->toid = gtk_timeout_add(500, filter_thres_preview, (gpointer)(ppf));
  }

}

void filter_thres_previewchange(GtkToggleButton *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  t_filter *ls = (t_filter *) (pp->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ls->preview));

  if (!s) {
    if (ls->toid >=0 ) {
      gtk_timeout_remove(ls->toid);
      ls->toid = -1;
    }
    preproc.gotpreview = 0;
    if (pp->repaint)
      pp->repaint(pp);
  } else {
    if (ls->toid >= 0)
      gtk_timeout_remove(ls->toid);
    ls->toid = gtk_timeout_add(500, filter_thres_preview, data);
  }
}

void filter_thres_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  t_filter *ls = (t_filter *) (pp->data);
  ArgBlock *args;

  args = ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->ia[0] = (int) (ls->la->value);
  args->ia[1] = (int) (ls->ua->value);

  preproc.gotpreview = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ls->preview), FALSE);
  TaskNew(qseg, "Filter: threshold", 
	  (Callback) &be_filter_threshold, (void *) args);
}
