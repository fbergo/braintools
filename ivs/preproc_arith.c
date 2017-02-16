 
// arithmetic operations filter (GUI)
// this file is #included from preproc.c

typedef struct _arithflt {
  DropBox *operation;
} arithflt;

/* final application of the filter */
void filter_arith_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_arith_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;
  arithflt *af;

  pp = preproc_filter_new();
  af = CREATE(arithflt);

  pp->id = id;
  pp->data = af;

  pp->control = 
    guip_create("frame[title='Arithmetic',border=2](vbox[gap=2,border=2]("\
		"hbox(label[text='Operation:'],op:dropbox[options=';orig - current;current - orig;orig + current;current / orig;orig / current;orig * current;copy current to orig']),"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  af->operation = (DropBox *) guip_get_ext("op");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_arith_apply), (gpointer) pp);
  return pp;
}

void filter_arith_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;
  PreProcFilter *ppf = (PreProcFilter *) data;
  arithflt *af = (arithflt *) (ppf->data);

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  args->ia[0] = dropbox_get_selection(af->operation);

  TaskNew(qseg, "Filter: Arithmetic", (Callback) &be_filter_arith, (void *) args);
}
