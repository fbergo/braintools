 
// rigid registration filter (GUI)
// this file is #included from preproc.c

typedef struct _regfilter {
  GtkWidget *infolabel;
  DropBox   *ptsel;

  int point[8][3];
} regfilter;

/* final application of the filter */
void filter_reg_apply(GtkWidget *w, gpointer data);
void filter_reg_go(GtkWidget *w, gpointer data);
void filter_reg_set_point(PreProcFilter *pp, int overlay, int x,int y,int z);
int  filter_reg_whatever(PreProcFilter *pp,int op,int ab,int x,int y,int z);
void filter_reg_bounds_check(regfilter *reg);

PreProcFilter * filter_reg_create(int id) {
  PreProcFilter *pp;
  GtkWidget *apply, *go;
  regfilter *reg;

  pp = preproc_filter_new();

  pp->id = id;
  reg = CREATE(regfilter);
  pp->data = reg;

  pp->whatever = filter_reg_whatever;

  pp->control = 
    guip_create("frame[title='Overlay Registration',border=2](vbox[gap=2,border=2]("\
		"pt:dropbox[options=';Edit Point #1;Edit Point #2;Edit Point #3;Edit Point #4'],"\
		"info:label[text='n/a',left],"\
		"label[text='You can use Ctrl+Click to navigate\nwithout marking points.',left]."\
		"hbox[gap=2]{gap=2}(go:button[text=' Go To Current Point '],apply:button[text=' Apply ']{packend})))");

  apply = guip_get("apply");
  go = guip_get("go");
  reg->ptsel     = (DropBox *) guip_get_ext("pt");
  reg->infolabel = guip_get("info");

  gtk_signal_connect(GTK_OBJECT(apply), "clicked",
		     GTK_SIGNAL_FUNC(filter_reg_apply), (gpointer) pp);

  gtk_signal_connect(GTK_OBJECT(go), "clicked",
		     GTK_SIGNAL_FUNC(filter_reg_go), (gpointer) pp);
  return pp;
}

void filter_reg_apply(GtkWidget *w, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  regfilter *reg = (regfilter *) (ppf->data);
  ArgBlock *args;

  int i,j,err=0;

  // check for stupid errors

  for(i=0;i<3;i++)
    for(j=i+1;j<4;j++) {
      if (reg->point[i][0] == reg->point[j][0] &&
	  reg->point[i][1] == reg->point[j][1] &&
	  reg->point[i][2] == reg->point[j][2])
	++err;
      if (reg->point[i+4][0] == reg->point[j+4][0] &&
	  reg->point[i+4][1] == reg->point[j+4][1] &&
	  reg->point[i+4][2] == reg->point[j+4][2])
	++err;
    }

  if (err > 0) {
    PopMessageBox(mainwindow,"Unable to Proceed",
		  "Registration requires 4 distinct points\n"\
		  "in the source volume and 4 distinct points\n"\
		  "in the overlay volume.",MSG_ICON_WARN);
    return;
  }

  if (!scene->mipvol) {
    PopMessageBox(mainwindow,"Error",
		  "There is no overlay volume loaded,\n"\
		  "use the Tools menu to load one.",
		  MSG_ICON_ERROR);
    return;
  }

  args=ArgBlockNew();
  args->scene = scene;
  args->ack   = &(sync_newdata);

  for(i=0;i<8;i++) {
    args->ia[3*i]   = reg->point[i][0];
    args->ia[3*i+1] = reg->point[i][1];
    args->ia[3*i+2] = reg->point[i][2];
  }

  TaskNew(qseg, "Filter: registration", (Callback) &be_filter_registration, (void *) args);
}

int filter_reg_whatever(PreProcFilter *pp, int op, int ab,int x,int y,int z)
{
  regfilter *reg = (regfilter *) (pp->data);
  int i;
  if (op==0) {
    filter_reg_set_point(pp,ab,x,y,z);
    return 0;
  } else {
    /* op=1: read
       ab=0 src, ab=1 overlay
       x: 0..3, y = 0..2 (x,y,z) */
    i = 4*ab + x;
    return(reg->point[i][y]);
  }
}

void filter_reg_set_point(PreProcFilter *pp, int overlay, int x,int y,int z) {
  regfilter *reg = (regfilter *) (pp->data);
  int i,j,k;
  char text[1024];

  i = dropbox_get_selection(reg->ptsel);
  j = overlay ? 1 : 0;

  k = 4*j + i;

  if (x >= 0) reg->point[k][0] = x;
  if (y >= 0) reg->point[k][1] = y;
  if (z >= 0) reg->point[k][2] = z;

  sprintf(text,
	  "Point #1 src=(%d,%d,%d) overlay=(%d,%d,%d)\n"\
	  "Point #2 src=(%d,%d,%d) overlay=(%d,%d,%d)\n"\
	  "Point #3 src=(%d,%d,%d) overlay=(%d,%d,%d)\n"\
	  "Point #4 src=(%d,%d,%d) overlay=(%d,%d,%d)",
	  reg->point[0][0],reg->point[0][1],reg->point[0][2],
	  reg->point[4][0],reg->point[4][1],reg->point[4][2],
	  reg->point[1][0],reg->point[1][1],reg->point[1][2],
	  reg->point[5][0],reg->point[5][1],reg->point[5][2],
	  reg->point[2][0],reg->point[2][1],reg->point[2][2],
	  reg->point[6][0],reg->point[6][1],reg->point[6][2],
	  reg->point[3][0],reg->point[3][1],reg->point[3][2],
	  reg->point[7][0],reg->point[7][1],reg->point[7][2]);
  gtk_label_set_text(GTK_LABEL(reg->infolabel), text);
  gtk_widget_queue_resize(reg->infolabel);
  gtk_widget_queue_draw(reg->infolabel);

  filter_reg_bounds_check(reg);

  if (pp->repaint)
    pp->repaint(pp);
}

void filter_reg_go(GtkWidget *w, gpointer data) {
  PreProcFilter *ppf = (PreProcFilter *) data;
  regfilter *reg = (regfilter *) (ppf->data);
  int i,j,k;

  j = preproc.show_overlay ? 1 : 0;
  i = dropbox_get_selection(reg->ptsel);

  k = 4*j + i;

  preproc.slice[0] = reg->point[k][2];
  preproc.slice[1] = reg->point[k][0];
  preproc.slice[2] = reg->point[k][1];

  if (ppf->repaint)
    ppf->repaint(ppf);
}

void filter_reg_bounds_check(regfilter *reg) {
  int W,H,D;
  int i;

  if (!scene->avol) return;
  W = scene->avol->W;
  H = scene->avol->H;
  D = scene->avol->D;

  for(i=0;i<8;i++) {
    if (reg->point[i][0] < 0) reg->point[i][0] = 0;
    if (reg->point[i][1] < 0) reg->point[i][1] = 0;
    if (reg->point[i][2] < 0) reg->point[i][2] = 0;
    if (reg->point[i][0] >= W) reg->point[i][0] = W-1;
    if (reg->point[i][1] >= H) reg->point[i][1] = H-1;
    if (reg->point[i][2] >= D) reg->point[i][2] = D-1;
  }
}
