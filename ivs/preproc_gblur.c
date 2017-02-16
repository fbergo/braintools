 
// gaussian blur filter (GUI)
// this file is #included from preproc.c
//
// if you need a reference, this is the shortest filter interface code 
// (since it has no previewing and no parameters).

/* final application of the filter */
void filter_gblur_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_gblur_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;

  pp = preproc_filter_new();

  pp->id = id;
  pp->data = 0;

  pp->control = 
    guip_create("frame[title='Gaussian Blur',border=2](vbox[gap=2,border=2]("\
		"label[text='This filter has no parameters.\nThis filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_gblur_apply), (gpointer) pp);
  return pp;
}

void filter_gblur_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  TaskNew(qseg, "Filter: blur", (Callback) &be_filter_conv, (void *) args);
}
