/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2010
   
   Felipe Paulo Guazzi Bergo 
   <fbergo@gmail.com>

   and
   
   Alexandre Xavier Falcao
   <afalcao@ic.unicamp.br>

   Distribution, trade, publication or bulk reproduction
   of this source code or any derivative works are
   strictly forbidden. Source code is provided to allow
   compilation on different architectures.

   ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libip.h>
#include <math.h>
#include "ivs.h"
#include "be.h"

extern Scene     *scene;
extern GtkWidget *mainwindow;
extern TaskQueue *qseg;
extern struct SegCanvas canvas;

GtkWidget *ownd = 0;
GtkAdjustment *oa = 0;
GtkWidget *olabel = 0;

int *tde_cost = 0;
volatile int  tde_done = 0;
BMap *omask = 0;

int aid = -1;
int toid = -1;
int oignore = 0;

void onion_edt(void *unused);
gboolean onion_watch(gpointer data);
void onion_update(GtkAdjustment *a, gpointer data);
gboolean onion_refresh(gpointer data);
void onion_animate(GtkWidget *w, gpointer data);

gboolean onion_delete(GtkWidget *w, GdkEvent *e, gpointer data);
void onion_destroy(GtkWidget *w, gpointer data);

void ivs_onion_open() {
  GtkWidget *v,*s,*b;

  if (ownd) {
    gtk_widget_show(ownd);
    return;
  }

  ownd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ownd), "Tools: Onion Peeling");
  gtk_window_set_modal(GTK_WINDOW(ownd), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(ownd), GTK_WINDOW(mainwindow));
  gtk_container_set_border_width(GTK_CONTAINER(ownd),6);

  v = gtk_vbox_new(FALSE,0);

  olabel = gtk_label_new("Computing EDT...");

  oa = (GtkAdjustment *) gtk_adjustment_new(0.0,0.0,1.0,0.01,0.10,0.0);
  s = gtk_hscale_new(oa);

  b = gtk_button_new_with_label("Start/Stop Animation");

  gtk_container_add(GTK_CONTAINER(ownd), v);
  gtk_box_pack_start(GTK_BOX(v), olabel, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(v), s, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(v), b, FALSE, TRUE, 4);

  gtk_widget_show(s);
  gtk_widget_show(olabel);
  gtk_widget_show(b);
  gtk_widget_show(v);
  gtk_widget_show(ownd);

  gtk_signal_connect(GTK_OBJECT(oa), "value_changed",
		     GTK_SIGNAL_FUNC(onion_update), 0);
  gtk_signal_connect(GTK_OBJECT(b), "clicked",
		     GTK_SIGNAL_FUNC(onion_animate), 0);

  gtk_signal_connect(GTK_OBJECT(ownd), "destroy",
		     GTK_SIGNAL_FUNC(onion_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(ownd), "delete_event",
		     GTK_SIGNAL_FUNC(onion_delete), 0);


  tde_done = 0;
  TaskNew(qseg, "Onion: IFT/EDT", (Callback) &onion_edt, 0);

  gtk_timeout_add(300, onion_watch, 0);
}

gboolean 
onion_delete(GtkWidget *w, GdkEvent *e, gpointer data) { 
  if (!tde_done) return TRUE;
  return FALSE; 
}

void onion_destroy(GtkWidget *w, gpointer data) {
  scene->with_restriction = 0;
  scene->restriction = 0;
  if (omask) BMapDestroy(omask);
  omask = 0;
  if (tde_cost) {
    free(tde_cost);
    tde_cost = 0;
  }
  ownd = 0;
  scene->invalid = 1;
  ivs_rot(0,0);
}

gboolean onion_animate2(gpointer data) {
  static int dir = 1;
  float nv;

  nv = oa->value + (float) dir;

  if (nv < oa->lower) {
    nv = oa->lower;
    dir = -dir;
  }

  if (nv > oa->upper) {
    nv = oa->upper;
    dir = -dir;
  }

  oignore++;
  gtk_adjustment_set_value(oa, nv);
  onion_refresh(0);
  return TRUE;
}

void onion_animate(GtkWidget *w, gpointer data) {
  if (!tde_done) return;
  if (aid >= 0) {
    gtk_timeout_remove(aid);
    aid = -1;
  } else {
    aid = gtk_timeout_add(200,onion_animate2,0);
  }
}

void onion_update(GtkAdjustment *a, gpointer data) {
  if (!tde_done) return;
  if (toid >= 0) {
    gtk_timeout_remove(toid);
    toid = -1;
  }

  if (oignore > 0) { --oignore; return; }
  toid = gtk_timeout_add(200, onion_refresh, 0);
}

gboolean onion_refresh(gpointer data) {
  int i, N= scene->avol->N;
  int v;

  toid = -1;

  if (!omask)
    omask = BMapNew(N);

  BMapFill(omask, 0);

  v = (int) (oa->value);
  for(i=0;i<N;i++)
    if (tde_cost[i] >= v && tde_cost[i] < v+5)
      _fast_BMapSet1(omask, i);

  scene->with_restriction = 2;
  scene->restriction = omask;
  scene->invalid = 1;
  refresh(canvas.itself);
  return FALSE;
}

gboolean onion_watch(gpointer data) {
  int i;
  int max = 0;
  int N = scene->avol->N;

  if (!tde_done) return TRUE;

  for(i=0;i<N;i++)
    if (tde_cost[i] > max) max = tde_cost[i];
  
  gtk_widget_hide(olabel);

  oa->lower = 0.0;
  oa->upper = (float) max;
  oa->step_increment = 1.0;
  oa->page_increment = 1.0;
  gtk_adjustment_changed(oa);
  gtk_adjustment_set_value(oa,0.0);

  return FALSE;
}

void onion_edt(void *unused) {
  WQueue *Q;
  AdjRel *A;
  XAnnVolume *avol = scene->avol;
  int i,j,N,p,q,c;
  int *RootMap;

  Voxel a,b,r;

  if (!avol) return;

  tde_cost = (int *) malloc(sizeof(int) * avol->N);
  if (!tde_cost) {
    printf("failed to allocate tde_cost\n");
    return;
  }

  N = avol->N;

  for(i=0;i<N;i++) {
    if (avol->vd[i].label == 0)
      tde_cost[i] = -1;
    else
      tde_cost[i] = 2000000000;
  }

  Q = WQCreate(N, 20);
  A = Spherical(1.8);
  RootMap = (int *) malloc(sizeof(int) * N);
  if (!RootMap) {
    printf("failed to allocate RootMap\n");
    return;
  }

  for(i=0;i<N;i++) {
    if (avol->vd[i].label != 0) {
      XAVolumeVoxelOf(avol, i, &a);
      for(j=1;j<A->n;j++) {
	b.x = a.x + A->dx[j];
	b.y = a.y + A->dy[j];
	b.z = a.z + A->dz[j];
	if (XAValidVoxel(avol, &b))
	  if (avol->vd[b.x + avol->tbrow[b.y] + avol->tbframe[b.z]].label == 0) {
	    tde_cost[i] = 0;
	    RootMap[i] = i;
	    WQInsert(Q,i,0);
	    break;
	  }      
      }
    }
  }

  while((p=WQRemove(Q))>=0) {
    XAVolumeVoxelOf(avol, p, &a);
    XAVolumeVoxelOf(avol, RootMap[p], &r);

    for(j=1;j<A->n;j++) {
      b.x = a.x + A->dx[j];
      b.y = a.y + A->dy[j];
      b.z = a.z + A->dz[j];
      if (XAValidVoxel(avol, &b)) {
	q = b.x + avol->tbrow[b.y] + avol->tbframe[b.z];
	if (tde_cost[q] > tde_cost[p]) {
	  c = (r.x-b.x)*(r.x-b.x);
	  c += (r.y-b.y)*(r.y-b.y);
	  c += (r.z-b.z)*(r.z-b.z);
	  if (c < tde_cost[q]) {
	    WQInsertOrUpdate(Q,q,tde_cost[q],c);
	    tde_cost[q] = c;
	    RootMap[q] = RootMap[p];
	  }
	}
      }
    }
  }
  
  WQDestroy(Q);
  free(RootMap);
  DestroyAdjRel(&A);

  for(i=0;i<N;i++)
    tde_cost[i] = (int) (2*sqrt(tde_cost[i]));

  tde_done = 1;
}

