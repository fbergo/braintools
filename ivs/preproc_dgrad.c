 
// directional gradient filter (GUI)
// this file is #included from preproc.c

/* final application of the filter */
void filter_dgrad_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_dgrad_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;

  pp = preproc_filter_new();

  pp->id = id;

  pp->control = 
    guip_create("frame[title='Directional Gradient',border=2](vbox[gap=2,border=2]("\
		"label[text='Mode:',center]{gap=2},"\
		"se:dropbox[options=';Maximum;Average']{gap=2},"\
		"label[text='This filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  pp->data = guip_get_ext("se");
  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_dgrad_apply), (gpointer) pp);
  return pp;
}

void filter_dgrad_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;
  PreProcFilter *pp = (PreProcFilter *) data;
  DropBox *db = (DropBox *) (pp->data);
  int a;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  a = dropbox_get_selection(db);
  args->ia[0] = a+2;

  TaskNew(qseg, "Filter: gradient", (Callback) &be_filter_grad, (void *) args);
}
