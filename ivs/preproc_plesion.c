 
// phantom lesion filter (GUI)
// this file is #included from preproc.c

typedef struct _plfilter {
  GtkAdjustment *a[4];
  int toid;
} PLFilter;

void filter_plesion_parchange(GtkAdjustment *a, gpointer data);
void filter_plesion_activate(PreProcFilter *ppf);
int  filter_plesion_whatever(PreProcFilter *ppf, int op, 
			     int a,int b,int c,int d);
gboolean filter_plesion_timeout(gpointer data);

/* final application of the filter */
void filter_plesion_apply(GtkWidget *w, gpointer data);

PreProcFilter * filter_plesion_create(int id) {
  PreProcFilter *pp;
  PLFilter *f;
  GtkWidget *apply;

  pp = preproc_filter_new();

  f = CREATE(PLFilter);

  pp->id = id;
  pp->data = (void *) f;

  pp->activate   = filter_plesion_activate;
  pp->deactivate = filter_plesion_activate;
  pp->whatever   = filter_plesion_whatever;

  pp->control = 
    guip_create("frame[title='Phantom Lesion',border=2](vbox[gap=2,border=2]("\
		"table[w=2,h=4]{gap=2}("\
		"label[text='Radius (mm)']{l=0,t=0},"\
		"label[text='Mean']{l=0,t=1},"\
		"label[text='Noise']{l=0,t=2},"\
		"label[text='Border (mm)']{l=0,t=3},"\
		"radius:spinbutton[val=5.0,min=3.0,max=300.0,step=1.0,page=10.0,"\
		"pagelen=0.0,accel=1.5]{l=1,t=0},"\
		"mean:spinbutton[val=300.0,min=0.0,max=4095.0,step=1.0,page=10.0,"\
		"pagelen=0.0,accel=1.5]{l=1,t=1},"\
		"noise:spinbutton[val=100.0,min=0.0,max=1000.0,step=1.0,page=10.0,"\
		"pagelen=0.0,accel=1.5]{l=1,t=2},"\
		"border:spinbutton[val=2.0,min=0.0,max=20.0,step=1.0,page=5.0,"\
		"pagelen=0.0,accel=1.5]{l=1,t=3}),"\
		"label[text='This filter does not provide previewing.',left],"\
		"hbox[gap=2]{gap=2}(apply:button[text=' Apply ']{packend})))");

  f->a[0] = guip_get_adj("radius");
  f->a[1] = guip_get_adj("mean");
  f->a[2] = guip_get_adj("noise");
  f->a[3] = guip_get_adj("border");

  f->toid = -1;

  apply = guip_get("apply");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_plesion_apply), (gpointer) pp);
  gtk_signal_connect(GTK_OBJECT(f->a[0]), "value_changed",
		     GTK_SIGNAL_FUNC(filter_plesion_parchange),
		     (gpointer) pp);

  return pp;
}

void filter_plesion_activate(PreProcFilter *ppf) {
  if (ppf->repaint)
    ppf->repaint(ppf);
}

gboolean filter_plesion_timeout(gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  PLFilter *f = (PLFilter *) (ppf->data);

  f->toid = -1;
  if (ppf->repaint)
    ppf->repaint(ppf);

  return FALSE;
}

void filter_plesion_parchange(GtkAdjustment *a, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  PLFilter *f = (PLFilter *) (ppf->data);

  if (f->toid >= 0) {
    gtk_timeout_remove(f->toid);
    f->toid = -1;
  }

  f->toid = gtk_timeout_add(400, filter_plesion_timeout, data);
}

int  filter_plesion_whatever(PreProcFilter *ppf, int op, 
			     int a,int b,int c,int d)
{
  PLFilter *f = (PLFilter *) (ppf->data);
  return( (int) (f->a[0]->value) ); /* return current radius */
}

void filter_plesion_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *ppf= (PreProcFilter *) data;
  PLFilter *f = (PLFilter *) (ppf->data);
  ArgBlock *args;

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_ppchange);

  args->ia[0] = (int) (f->a[0]->value); /* radius */
  args->ia[1] = (int) (f->a[1]->value); /* mean */
  args->ia[2] = (int) (f->a[2]->value); /* noise */
  args->ia[3] = (int) (f->a[3]->value); /* border */

  args->ia[4] = preproc.slice[1];
  args->ia[5] = preproc.slice[2];
  args->ia[6] = preproc.slice[0];

  TaskNew(qseg, "Filter: phantom lesion", (Callback) &be_filter_lesion, 
	  (void *) args);  
}
