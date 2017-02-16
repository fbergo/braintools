 
// morphological operations filter (GUI)
// this file is #included from preproc.c

typedef struct _morphflt {
  DropBox *operation;
  DropBox *shape;
  GtkAdjustment *size;
} morphflt;

/* final application of the filter */
void filter_morph_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_morph_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply;
  morphflt *mf;

  pp = preproc_filter_new();
  mf = CREATE(morphflt);

  pp->id = id;
  pp->data = mf;

  pp->control = 
    guip_create("frame[title='Morphology',border=2](vbox[gap=2,border=2]("\
		"hbox(label[text='Operation:'],op:dropbox[options=';Dilate;Erode']),"\
		"hbox(label[text='Struct. Element:'],se:dropbox[options=';Cross;Box']),"\
		"hbox(label[text='Size'],sz:spinbutton[val=3.0,min=3.0,max=11.0,step=2.0,page=4.0,pagelen=0.0,accel=1.5]),"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");

  mf->operation = (DropBox *) guip_get_ext("op");
  mf->shape     = (DropBox *) guip_get_ext("se");
  mf->size      = guip_get_adj("sz");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_morph_apply), (gpointer) pp);
  return pp;
}

void filter_morph_apply(GtkWidget *w, gpointer data) {
  ArgBlock *args;
  PreProcFilter *ppf = (PreProcFilter *) data;
  morphflt *mf = (morphflt *) (ppf->data);

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  args->ia[0] = dropbox_get_selection(mf->operation);
  args->ia[1] = dropbox_get_selection(mf->shape);
  args->ia[2] = (int) (mf->size->value);

  TaskNew(qseg, "Filter: Morphology", (Callback) &be_filter_morph, (void *) args);
}
