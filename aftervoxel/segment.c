
#define SOURCE_SEGMENT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "aftervoxel.h"

struct segcontrol {
  int label;
  Volume *oldlabel;
  Volume *cost;
} segdata; 

static struct segdlgctl {
  int hist[MAXLABELS+1];
  int assoc[MAXLABELS];
  GtkWidget *radio[MAXLABELS];
  int nradio;

  GtkWidget *ct;
  gint toid;
} dlgctl;

GtkWidget *sdlg = 0;

void seg_init() {
  segdata.oldlabel = 0;
  segdata.cost     = 0;  
}

gboolean seg_dlg_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
  return TRUE;
}

void seg_dlg_destroy(GtkWidget *w, gpointer data) {
  sdlg = 0;
}

void seg_dlg_ok(GtkWidget *w, gpointer data) {
  int i;
  segdata.label = 1;
  for(i=0;i<dlgctl.nradio;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dlgctl.radio[i]))) {
      segdata.label = dlgctl.assoc[i];
      break;
    }
  gtk_widget_destroy(sdlg);

  TaskNew(res.bgqueue,"Segmenting by Connectivity...",
	  (Callback) &seg_computation3, 0);
}

void seg_dlg_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(sdlg);
}

void seg_dialog() {
  int i,j;
  GtkWidget *v,*bh,*ok,*cancel,*l;

  if (sdlg != 0) {
    gtk_window_present(GTK_WINDOW(sdlg));
    return;
  }

  // find used labels
  if (MutexTryLock(voldata.masterlock) != 0) return;
  for (i=0;i<MAXLABELS+1;i++)
    dlgctl.hist[i] = 0;
  for (i=0;i<voldata.label->N;i++)
    dlgctl.hist[ (int) (voldata.label->u.i8[i] & 0x3f) ]++;
  MutexUnlock(voldata.masterlock);

  j = dlgctl.hist[1];
  for(i=2;i<MAXLABELS+1;i++)
    j = MAX(dlgctl.hist[i],j);

  if (j==0) {
    PopMessageBox(mw,"Error","No base segmentation.",MSG_ICON_WARN);
    return;
  }

  sdlg = util_dialog_new("Segmentation by Connectivity",mw,1,&v,&bh,-1,-1);

  l = gtk_label_new("Choose the Object");
  gtk_box_pack_start(GTK_BOX(v), l, FALSE, FALSE, 2);

  j = 0;
  for(i=1;i<MAXLABELS+1;i++) {

    if (dlgctl.hist[i] == 0) continue;

    if (j==0)
      dlgctl.radio[j]=gtk_radio_button_new_with_label(NULL,
						      labels.names[i-1]);
    else
      dlgctl.radio[j]=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(dlgctl.radio[0]),
								  labels.names[i-1]);
    dlgctl.assoc[j] = i;
    gtk_box_pack_start(GTK_BOX(v), dlgctl.radio[j], FALSE, TRUE, 2);
    gtk_widget_show(dlgctl.radio[j]);
    ++j;
  }
  dlgctl.nradio = j;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlgctl.radio[0]),TRUE);

  ok     = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");

  gtk_box_pack_start(GTK_BOX(bh), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(bh), cancel, TRUE, TRUE, 0);

  /* signals */

  gtk_signal_connect(GTK_OBJECT(sdlg), "destroy",
		     GTK_SIGNAL_FUNC(seg_dlg_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(sdlg), "delete_event",
		     GTK_SIGNAL_FUNC(seg_dlg_delete), 0);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(seg_dlg_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(seg_dlg_cancel), 0);

  gtk_widget_show_all(sdlg);
}

void seg_dlg2_destroy(GtkWidget *w, gpointer data) {
  sdlg = 0;
  if (segdata.cost) {
    VolumeDestroy(segdata.cost);
    segdata.cost = 0;
  }
  if (segdata.oldlabel) {
    VolumeDestroy(segdata.oldlabel);
    segdata.oldlabel = 0;
  }

  ToolbarEnable(gui.t2d, 3);
  ToolbarEnable(gui.t2d, 4);
  ToolbarEnable(gui.t2d, 5);
  ToolbarEnable(gui.t2d, 6);
  ToolbarEnable(gui.t2d, 7);
  ToolbarEnable(gui.t2d, 8);
  ToolbarEnable(gui.t3d, 3);

}

void seg_dlg2_cancel(GtkWidget *w, gpointer data) {
  if (dlgctl.toid >= 0) {
    gtk_timeout_remove(dlgctl.toid);
    dlgctl.toid = -1;
  }
  MemCpy(voldata.label->u.i8, segdata.oldlabel->u.i8, segdata.oldlabel->N);
  notify.segchanged++;
  v2d.invalid = 1;
  view3d_objects_trash();
  v3d.obj.invalid = 1;
  force_redraw_2D();
  if (v3d.obj.enable)
    force_redraw_3D();
  gtk_widget_destroy(sdlg);
}

void seg_dlg2_apply() {
  int i,t;
  Volume *label,*old;

  MutexLock(voldata.masterlock);

  t = (int) gtk_range_get_value(GTK_RANGE(dlgctl.ct));

  label = voldata.label;
  old = segdata.oldlabel;

  MemCpy(label->u.i8, old->u.i8, old->N);
  for(i=0;i<old->N;i++)
    if (segdata.cost->u.i16[i] >= 0 && segdata.cost->u.i16[i] <= t)
      label->u.i8[i] = (i8_t) (segdata.label);

  MutexUnlock(voldata.masterlock);

  notify.segchanged++;
  v2d.invalid = 1;
  v3d.obj.invalid = 1;
  force_redraw_2D();
  if (v3d.obj.enable)
    force_redraw_3D();
}

void seg_dlg2_ok(GtkWidget *w, gpointer data) {
  if (dlgctl.toid >= 0) {
    gtk_timeout_remove(dlgctl.toid);
    dlgctl.toid = -1;
  }
  seg_dlg2_apply();
  gtk_widget_destroy(sdlg);
}

gboolean seg_dlg2_timeout(gpointer data) {
  dlgctl.toid = -1;
  seg_dlg2_apply();
  return FALSE;
}

void seg_dlg2_update(GtkRange *w, gpointer data) {
  if (dlgctl.toid >= 0) {
    gtk_timeout_remove(dlgctl.toid);
    dlgctl.toid = -1;
  }
  dlgctl.toid = gtk_timeout_add(200,seg_dlg2_timeout,0);
}

void seg_dialog2() {
  GtkWidget *v,*l,*bh,*ok,*cancel,*l2,*h;
  int vmax;

  if (sdlg != 0) {
    gtk_window_present(GTK_WINDOW(sdlg));
    return;
  }

  dlgctl.toid = -1;

  sdlg = util_dialog_new("Segmentation by Connectivity",mw,0,&v,&bh,-1,-1);

  l = gtk_label_new("Adust the threshold."); // HERE

  gtk_box_pack_start(GTK_BOX(v), l, FALSE, FALSE, 2);

  vmax = VolumeMax(segdata.cost);

  h = gtk_hbox_new(FALSE, 2);
  l2 = gtk_label_new("Threshold:");
  dlgctl.ct = gtk_hscale_new_with_range(0.0,vmax,1.0);
  gtk_scale_set_digits(GTK_SCALE(dlgctl.ct), 0);
  gtk_scale_set_draw_value(GTK_SCALE(dlgctl.ct), TRUE);
  gtk_range_set_value(GTK_RANGE(dlgctl.ct), 0.0);

  gtk_box_pack_start(GTK_BOX(h), l2, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(h), dlgctl.ct, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(v), h, TRUE, TRUE, 2);

  ok     = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Ok");

  gtk_box_pack_start(GTK_BOX(bh), ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(bh), cancel, TRUE, TRUE, 0);

  /* signals */

  gtk_signal_connect(GTK_OBJECT(sdlg), "destroy",
		     GTK_SIGNAL_FUNC(seg_dlg2_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(sdlg), "delete_event",
		     GTK_SIGNAL_FUNC(seg_dlg_delete), 0);
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     GTK_SIGNAL_FUNC(seg_dlg2_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     GTK_SIGNAL_FUNC(seg_dlg2_cancel), 0);
  gtk_signal_connect(GTK_OBJECT(dlgctl.ct), "value_changed",
		     GTK_SIGNAL_FUNC(seg_dlg2_update), 0);

  gtk_widget_show_all(sdlg);

  ToolbarDisable(gui.t2d, 3,0);
  ToolbarDisable(gui.t2d, 4,0);
  ToolbarDisable(gui.t2d, 5,0);
  ToolbarDisable(gui.t2d, 6,0);
  ToolbarDisable(gui.t2d, 7,0);
  ToolbarDisable(gui.t2d, 8,0);
  ToolbarDisable(gui.t3d, 3,0);
}

static struct _thr {
  GtkWidget *dlg;
  GtkWidget *lower,*upper;
  Volume *oldlabel;
  int nradio;
  int apply;
  gint toid;
  int label;
  int lval, uval;
  GtkWidget *radio[MAXLABELS];
} thr;

void thr_ok(GtkWidget *w,gpointer data);
void thr_cancel(GtkWidget *w,gpointer data);
void thr_destroy(GtkWidget *w,gpointer data);
void thr_toggle(GtkToggleButton *w,gpointer data);
void thr_change(GtkSpinButton *w,gpointer data);
void thr_update();
void thr_apply();
gboolean thr_update_timeout(gpointer data);

void thr_dialog() {
  GtkWidget *v,*bh,*h,*of,*ov,*t,*l1,*l2,*ok,*cancel;
  GtkWidget *tf,*tb,*tl;
  int i,j;

  thr.oldlabel = VolumeNew(voldata.label->W,
			   voldata.label->H,
			   voldata.label->D, vt_integer_8);
  if (!thr.oldlabel) {
    error_memory();
    return;
  }
  MutexLock(voldata.masterlock);
  MemCpy(thr.oldlabel->u.i8,voldata.label->u.i8,voldata.label->N);
  MutexUnlock(voldata.masterlock);

  thr.dlg = util_dialog_new("Threshold Segmentation",mw,0,&v,&bh,-1,-1);
  thr.apply = 0;
  thr.toid = -1;

  for(i=3;i<9;i++)
    ToolbarDisable(gui.t2d,i,0);
  ToolbarDisable(gui.t3d,3,0);

  h = gtk_hbox_new(FALSE,2);
  of = gtk_frame_new("Object");
  gtk_frame_set_shadow_type(GTK_FRAME(of),GTK_SHADOW_ETCHED_IN);
  ov = gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(ov),6);

  gtk_box_pack_start(GTK_BOX(v), h, TRUE,TRUE, 2);
  gtk_box_pack_start(GTK_BOX(h), of, FALSE, TRUE, 2);
  gtk_container_add(GTK_CONTAINER(of), ov);

  thr.nradio = labels.count;
  for(i=0;i<labels.count;i++) {

    if (i==0)
      thr.radio[i]=gtk_radio_button_new_with_label(NULL,
						   labels.names[i]);
    else
      thr.radio[i]=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(thr.radio[0]),
							       labels.names[i]);
    gtk_box_pack_start(GTK_BOX(ov), thr.radio[i], FALSE, TRUE, 2);
    gtk_signal_connect(GTK_OBJECT(thr.radio[i]),"toggled",
		       GTK_SIGNAL_FUNC(thr_toggle), 0);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(thr.radio[0]),TRUE);

  t = gtk_table_new(3,2,FALSE);
  gtk_box_pack_start(GTK_BOX(h), t, FALSE, TRUE,0);

  j = VolumeMax(voldata.original);
  thr.lower = gtk_spin_button_new_with_range(0.0,j,1.0);
  thr.upper = gtk_spin_button_new_with_range(0.0,j,1.0);

  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(thr.lower),0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(thr.upper),0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(thr.lower),j/10.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(thr.upper),(9.0*j)/10.0);

  l1 = gtk_label_new("Lower Threshold:");
  l2 = gtk_label_new("Upper Threshold:");

  gtk_table_attach_defaults(GTK_TABLE(t),l1,0,1,1,2);
  gtk_table_attach_defaults(GTK_TABLE(t),l2,0,1,2,3);
  gtk_table_attach_defaults(GTK_TABLE(t),thr.lower,1,2,1,2);
  gtk_table_attach_defaults(GTK_TABLE(t),thr.upper,1,2,2,3);

  tf = gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(tf),GTK_SHADOW_ETCHED_IN);
  tb = gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(tb), 6);
  tl = gtk_label_new("All voxels..."); // HERE
  gtk_container_add(GTK_CONTAINER(tf), tb);
  gtk_box_pack_start(GTK_BOX(tb), tl, FALSE, TRUE, 0);
  gtk_table_attach_defaults(GTK_TABLE(t),tf,0,2,0,1);

  ok     = gtk_button_new_with_label("Ok");
  cancel = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(bh),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(bh),cancel,TRUE,TRUE,0);

  gtk_signal_connect(GTK_OBJECT(thr.dlg), "destroy",
		     GTK_SIGNAL_FUNC(thr_destroy), 0);
  gtk_signal_connect(GTK_OBJECT(thr.lower), "value_changed",
		     GTK_SIGNAL_FUNC(thr_change), 0);
  gtk_signal_connect(GTK_OBJECT(thr.upper), "value_changed",
		     GTK_SIGNAL_FUNC(thr_change), 0);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(thr_ok), 0);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
		     GTK_SIGNAL_FUNC(thr_cancel), 0);

  gtk_widget_show_all(thr.dlg);
  thr_toggle(0,0);
  thr_change(0,0);
}

void thr_ok(GtkWidget *w,gpointer data) {
  thr.apply = 1;
  gtk_widget_destroy(thr.dlg);
}

void thr_cancel(GtkWidget *w,gpointer data) {
  gtk_widget_destroy(thr.dlg);
}

void thr_destroy(GtkWidget *w,gpointer data) {
  int i;

  if (!thr.apply) {
    MutexLock(voldata.masterlock);
    MemCpy(voldata.label->u.i8,thr.oldlabel->u.i8,thr.oldlabel->N);
    MutexUnlock(voldata.masterlock);
  } else
    thr_apply();
  VolumeDestroy(thr.oldlabel);
  thr.apply = 0;
  if (thr.toid >= 0)
    gtk_timeout_remove(thr.toid);

  for(i=3;i<9;i++)
    ToolbarEnable(gui.t2d,i);
  ToolbarEnable(gui.t3d,3);
}

void thr_apply() {
  int i,n;
  i8_t label;
  i16_t lt,ut;

  n     = thr.oldlabel->N;
  lt    = (i16_t) thr.lval;
  ut    = (i16_t) thr.uval;
  label = (i8_t) thr.label;

  MutexLock(voldata.masterlock);
  MemCpy(voldata.label->u.i8,thr.oldlabel->u.i8,n);
  for(i=0;i<n;i++)
    if (voldata.original->u.i16[i] >= lt &&
	voldata.original->u.i16[i] <= ut && thr.oldlabel->u.i8[i]==0)
      voldata.label->u.i8[i] = label;
  MutexUnlock(voldata.masterlock);
}

void thr_toggle(GtkToggleButton *w,gpointer data) {
  int i;
  for(i=0;i<thr.nradio;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(thr.radio[i])))
      thr.label = i+1;
  thr_update();
}

void thr_change(GtkSpinButton *w,gpointer data) {
  thr.lval = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(thr.lower));
  thr.uval = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(thr.upper));

  if (thr.uval < thr.lval) {
    if (w == GTK_SPIN_BUTTON(thr.lower))
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(thr.upper), thr.lval);
    else
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(thr.lower), thr.uval);
  }

  thr_update();
}

void thr_update() {
  if (thr.toid >= 0)
    gtk_timeout_remove(thr.toid);
  thr.toid = gtk_timeout_add(200,thr_update_timeout,0);
}

gboolean thr_update_timeout(gpointer data) {
  thr_apply();
  force_redraw_2D();
  view3d_invalidate(0,1);
  force_redraw_3D();
  thr.toid = -1;
  return FALSE;
}

void seg_computation3(void *args) {
  Volume *orig,*cost,*root;
  int i,W,H,D,WH,N;

  int count,total;
  i8_t L;

  Queue *Q;
  AdjRel *A;
  Voxel p,q;
  int x,w;
  i16_t c1,c2,c3;

  MutexLock(voldata.masterlock);
  orig = voldata.original;
  W = orig->W;
  H = orig->H;
  D = orig->D;
  N = orig->N;
  WH = W*H;

  if (segdata.cost != 0) {
    VolumeDestroy(segdata.cost);
    segdata.cost = 0;
  }

  cost = segdata.cost = VolumeNew(W,H,D,vt_integer_16);

  if (!cost) {
    error_memory();
    MutexUnlock(voldata.masterlock);
    return;
  }

  root = VolumeNew(W,H,D,vt_integer_32);
  
  if (!root) {
    error_memory();
    MutexUnlock(voldata.masterlock);
    return;
  }

  VolumeFill(cost, 32767);
  L = (i8_t) (segdata.label);

  SetProgress(0,N);

  /* compute IFT expansion*/

  Q = CreateQueue(32768, N);
  A = Spherical(1.0);

  if (!Q || !A) {
    error_memory();
    MutexUnlock(voldata.masterlock);
    return;
  }

  total = 0;
  for(i=0;i<N;i++) {
    if (voldata.label->u.i8[i] == 0) continue;
    if (voldata.label->u.i8[i] == L) {
      InsertQueue(Q,0,i);
      cost->u.i16[i] = 0;
      root->u.i32[i] = i;
    } else {
      ++total;
      cost->u.i16[i] = -1; // don't conquer other labels
    }
  }
  total = N-total;
  count = 0;

  while ( (x=RemoveQueue(Q)) != NIL ) {
    
    ++count;
    if (count%20000 == 0) SetProgress(count,total);
    
    p.z = x / WH;
    p.y = (x % WH) / W;
    p.x = (x % WH) % W;

    for(i=1;i<A->n;i++) {
      q.x = p.x + A->dx[i];
      q.y = p.y + A->dy[i];
      q.z = p.z + A->dz[i];

      if (q.x < 0 || q.y < 0 || q.z < 0 ||
	  q.z >= W || q.y >= H || q.z >= D) continue;

      w = q.x + W*q.y + WH*q.z;

      c2 = cost->u.i16[w];
      if (c2 < 0) continue;

      c3 = orig->u.i16[root->u.i32[x]] - orig->u.i16[w];
      if (c3 < 0) c3 = -c3;
      c1 = MAX(cost->u.i16[x], c3);

      if (c1 < c2) {
	cost->u.i16[w] = c1;
	root->u.i32[w] = root->u.i32[x];

	if (Q->L.elem[w].color == GRAY)
	  UpdateQueue(Q,w,c2,c1);
	else
	  InsertQueue(Q,c1,w);
      }
    } // for i
  } // removequeue

  VolumeDestroy(root);
  DestroyQueue(&Q);
  DestroyAdjRel(&A);

  if (segdata.oldlabel != 0) {
    VolumeDestroy(segdata.oldlabel);
    segdata.oldlabel = 0;
  }
  
  segdata.oldlabel = VolumeNew(W,H,D,vt_integer_8);
  if (!segdata.oldlabel) {
    error_memory();
    MutexUnlock(voldata.masterlock);
    return;
  }

  MemCpy(segdata.oldlabel->u.i8, voldata.label->u.i8, N);

  MutexUnlock(voldata.masterlock);
  notify.autoseg++;

  //  VolumeSaveSCN(segdata.cost,"cost.scn");
}
