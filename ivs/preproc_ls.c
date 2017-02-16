 
// linear stretch filter (GUI)
// this file is #included from preproc.c

typedef struct _lsfilter {
  GtkAdjustment *ba, *ca;
  GtkWidget     *bw, *cw;
  GtkWidget     *preview;
  RCurve        *curve;
  i16_t         *func;
  i32_t         *hist;
  int            histsync;
  int            toid;
} ls_filter;

/* called when the brightness or contrast controls change */
void filter_ls_parchange(GtkAdjustment *a, gpointer data);

/* called when the preview checkbox changes */
void filter_ls_previewchange(GtkToggleButton *w, gpointer data);

/* timeout-called preview calculation */
gboolean filter_ls_preview(gpointer data);

/* final application of the filter */
void filter_ls_apply(GtkWidget *w, gpointer data);

/* preview update callback (called when cursor position changes) */
void filter_ls_previewchange_cb(PreProcFilter *ppf);

/* activation/update callback */
void filter_ls_activate_cb(PreProcFilter *ppf);
void filter_ls_update_cb(PreProcFilter *ppf);

PreProcFilter * filter_ls_create(int id) {
  PreProcFilter *pp;
  ls_filter *ls;
  GtkWidget *apply;

  pp = preproc_filter_new();
  ls = CREATE(ls_filter);

  pp->id = id;
  pp->data = ls;

  pp->updatepreview = filter_ls_previewchange_cb;
  pp->activate      = filter_ls_activate_cb;
  pp->update        = filter_ls_update_cb;

  pp->control = 
    guip_create("frame[title='Linear Stretch',border=2](vbox[gap=2,border=2]("\
		"table[w=2,h=2]{gap=2}("\
		"label[text='brightness %']{l=0,r=1,t=0,b=1},"\
		"label[text='contrast %']{l=0,r=1,t=1,b=2},"\
		"bri:spinbutton[val=0.0,min=-150.0,max=150.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=0,b=1},"\
		"con:spinbutton[val=0.0,min=-150.0,max=150.0,"\
		"step=1.0,page=5.0,pagelen=0.0,accel=1.5]{l=1,r=2,t=1,b=2}),"\
		"func:curve{expand,fill},"\
		"preview:checkbox[text='Preview'],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  ls->ba = guip_get_adj("bri");
  ls->ca = guip_get_adj("con");

  ls->bw = guip_get("bri");
  ls->cw = guip_get("con");
  ls->curve = guip_get_ext("func");
  ls->preview = guip_get("preview");
  ls->toid = -1;
  ls->hist = (i32_t *) malloc(SDOM * sizeof(i32_t));
  ls->histsync = -1;

  apply = guip_get("apply");

  ls->func = 0;

  gtk_signal_connect(GTK_OBJECT(ls->ba), "value_changed",
		     GTK_SIGNAL_FUNC(filter_ls_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(ls->ca), "value_changed",
		     GTK_SIGNAL_FUNC(filter_ls_parchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(ls->preview), "toggled",
		     GTK_SIGNAL_FUNC(filter_ls_previewchange), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_ls_apply), (gpointer) pp);
  return pp;
}

void filter_ls_activate_cb(PreProcFilter *ppf) {
  ls_filter *ls = (ls_filter *) (ppf->data);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ls->preview), FALSE);
  filter_ls_update_cb(ppf);
}

void filter_ls_update_cb(PreProcFilter *ppf) {
  ls_filter *ls = (ls_filter *) (ppf->data);
  int i;

  if (ls->func != 0) {
    rcurve_set_function(ls->curve,0,0,0);
    free(ls->func);
    ls->func = 0;
  }

  if (scene->avol != 0) {
    ls->func = XA_LS_Function(ls->ba->value,ls->ca->value,
			      preproc.bounds.vmax);
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

gboolean filter_ls_preview(gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  ls_filter *ls = (ls_filter *) (pp->data);
  Volume *preview;

  ls->toid = -1;
  preview = preproc_prepare_preview();

  // apply filter on these slices

  XA_LS_Filter_Preview(preview, 
		       ls->ba->value,
		       ls->ca->value,
		       *(pp->xslice),
		       *(pp->yslice),
		       *(pp->zslice));
  preproc.gotpreview = 1;

  if (pp->repaint)
    pp->repaint(pp);
  return FALSE;
}

void filter_ls_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  ls_filter *ls = (ls_filter *) (pp->data);

  filter_ls_update_cb(pp);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ls->preview))) {
    if (ls->toid >= 0)
      gtk_timeout_remove(ls->toid);
    ls->toid = gtk_timeout_add(500, filter_ls_preview, data);
  }
}

void filter_ls_previewchange_cb(PreProcFilter *ppf) {
  ls_filter *ls = (ls_filter *) (ppf->data);

  int s;

  s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ls->preview));

  if (s) {
    if (ls->toid >= 0)
      gtk_timeout_remove(ls->toid);
    ls->toid = gtk_timeout_add(500, filter_ls_preview, (gpointer)(ppf));
  } else {
    preproc.gotpreview = 0;
    if (ppf->repaint)
      ppf->repaint(ppf);  
  }

}

void filter_ls_previewchange(GtkToggleButton *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  ls_filter *ls = (ls_filter *) (pp->data);

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
    ls->toid = gtk_timeout_add(500, filter_ls_preview, data);
  }
}

void filter_ls_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *pp = (PreProcFilter *) data;
  ls_filter *ls = (ls_filter *) (pp->data);
  ArgBlock *args;

  args = ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->fa[0] = ls->ba->value;
  args->fa[1] = ls->ca->value;

  preproc.gotpreview = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ls->preview), FALSE);
  TaskNew(qseg, "Filter: linear stretch", 
	  (Callback) &be_filter_ls, (void *) args);
}
