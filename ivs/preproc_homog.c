 
// homogenization filter (GUI)
// this file is #included from preproc.c

/* final application of the filter */
void filter_homog_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_homog_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;

  pp = preproc_filter_new();

  pp->id = id;
  pp->data = 0;

  pp->control = 
    guip_create("frame[title='Slice Homogenization',border=2](vbox[gap=2,border=2]("\
		"label[text='This filter has no parameters.\nThis filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_homog_apply), (gpointer) pp);
  return pp;
}

void filter_homog_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);
  args->ia[0] = preproc.slice[0];

  TaskNew(qseg, "Filter: slice homogenization", (Callback) &be_filter_homog, (void *) args);
}
