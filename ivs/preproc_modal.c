 
// modal filter (GUI)
// this file is #included from preproc.c

/* final application of the filter */
void filter_modal_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_modal_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;

  pp = preproc_filter_new();

  pp->id = id;
  pp->data = 0;

  pp->control = 
    guip_create("frame[title='Modal',border=2](vbox[gap=2,border=2]("\
		"label[text='This filter has no parameters.\nThis filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_modal_apply), (gpointer) pp);
  return pp;
}

void filter_modal_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  TaskNew(qseg, "Filter: modal", (Callback) &be_filter_modal, (void *) args);
}
