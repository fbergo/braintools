 
// bpp ssr filter (GUI)
// this file is #included from preproc.c

typedef struct _ssrfilter {
  GtkAdjustment *a;
} SSRFilter;

/* final application of the filter */
void filter_ssr_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_ssr_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;
  SSRFilter *f;

  pp = preproc_filter_new();

  pp->id = id;

  f = CREATE(SSRFilter);
  pp->data = (void *) f;

  pp->control = 
    guip_create("frame[title='Brain Gradient (SSR)',border=2](vbox[gap=2,border=2]("\
		"hbox[gap=2]{gap=2}(label[text='Inclusiveness (%)']{gap=2},"\
		"inc:spinbutton[val=17.0,min=10.0,max=30.0,step=1.0,page=10.0,pagelen=0.0,accel=1.5]{gap=2}),"\
		"label[text='This is an experimental filter.\nThis filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");
  f->a = guip_get_adj("inc");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_ssr_apply), (gpointer) pp);
  return pp;
}

void filter_ssr_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  SSRFilter *f = (SSRFilter *) (ppf->data);
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->fa[0] = f->a->value / 100.0;

  TaskNew(qseg, "Filter: SSR Gradient", (Callback) &be_filter_bpp_ssr,
	  (void *) args);
}
