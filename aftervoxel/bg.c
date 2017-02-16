
#define SOURCE_BG_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aftervoxel.h"

void error_volume_load() {
  app_pop_message("Error","Unable to load image.\nThe file may be damaged, unsupported or unreadable.",MSG_ICON_ERROR);
}

void error_volume_save() {
  app_pop_message("Error","Unable to save image.",MSG_ICON_ERROR);
}

void error_memory() {
  app_pop_message("Error","Not enough memory to complete the operation.",MSG_ICON_ERROR);
}

void warn_no_volume() {
  app_pop_message("Error","No volume loaded.",MSG_ICON_WARN);
}

ArgBlock * ArgBlockNew() {
  ArgBlock *args;
  int i;
  args = CREATE(ArgBlock);
  if (!args) return 0;
  for(i=0;i<4;i++)
    args->vols[i] = 0;
  args->ack = 0;
  args->sa[0] = 0;
  return args;
}

void ArgBlockDestroy(ArgBlock *args) {
  if (args)
    DESTROY(args);
}

void ArgBlockSendAck(ArgBlock *args) {
  if (args->ack)
    ++ (*(args->ack));
}

#define APP_BG_PROLOGUE ArgBlock *data = (ArgBlock *) args
#define APP_BG_EPILOGUE ArgBlockDestroy(data)
#define APP_BG_ACK      ArgBlockSendAck(data)

void bg_load_volume(void *args) {
  APP_BG_PROLOGUE;
  Volume *v,*iv,*tmp;
  float vs,vt;

  v = VolumeNewFromFile(data->sa);
  if (!v) goto eout;

  if (data->ia[0] == 1) {
    vs = MIN(MIN(v->dx,v->dy),v->dz);
    iv = VolumeInterpolate(v,vs,vs,vs);
    if (iv) {
      tmp = v;
      v = iv;
      VolumeDestroy(tmp);
    }
  }

  // smart interpolation (only if MAX/MIN < 2)
  if (data->ia[0] == 2) {
    vs = MIN(MIN(v->dx,v->dy),v->dz);
    vt = MAX(MAX(v->dx,v->dy),v->dz);

    if (vs>0.01 && (vt/vs) < 2.0) {
      iv = VolumeInterpolate(v,vs,vs,vs);
      if (iv) {
	tmp = v;
	v = iv;
	VolumeDestroy(tmp);
      }
    }
  }
  
  info_reset();
  *(data->vols[0]) = v;
  app_set_title_file(data->sa);
  
  //  printf("size=%dx%dx%d\n",v->W,v->H,v->D);

  APP_BG_ACK;
  APP_BG_EPILOGUE;
  return;

 eout:
  error_volume_load();
  APP_BG_EPILOGUE;
}

void bg_erase_segmentation(void *args) {
  int i,N;

  if (voldata.label) {
    N = voldata.label->N;
    
    SetProgress(0,N);
    for(i=0;i<N;i++) {
      voldata.label->u.i8[i] = 0;
      if (i%10000 == 0)
	SetProgress(i,N);
    }
    
    EndProgress();
    notify.segchanged++;
  }

}

void bg_load_segmentation(void *args) {
  char *name;
  Volume *v,*L;
  int i,j,k;
  i8_t maxl,l;

  name = (char *) args;

  v = VolumeNewFromFile(name);

  if (!v) {
    error_volume_load();
    return;
  }

  maxl = 0;

  v2d_ensure_label();
  L = voldata.label;
  for(k=0;k<MIN(L->D,v->D);k++)
    for(j=0;j<MIN(L->H,v->H);j++)
      for(i=0;i<MIN(L->W,v->W);i++) {
	VolumeSetI8(L,i,j,k,l = (i8_t) (VolumeGetI16(v,i,j,k) % (MAXLABELS+1) ));
	if (l>maxl) maxl = l;
      }

  labels_ensure_count(maxl+1);

  free(name);
  VolumeDestroy(v);
  notify.segchanged++;
}

void bg_save_segmentation(void *args) {
  char *name;

  name = (char *) args;
  if (VolumeSaveSCN8(voldata.label, name)==0)
    error_volume_save();
  free(name);
}

void bg_seg_tp2007(void *args) {
  Volume *m=NULL, *e=NULL, *g=NULL, *s=NULL;
  int lb,i;
  
  SetProgress(0,5);
  m = tp_brain_marker_comp(voldata.original);
  //VolumeSaveSCN8(m,"tpm.scn");

  SetProgress(1,5);
  e = tp_brain_otsu_enhance(voldata.original);
  //VolumeSaveSCN(e,"tpe.scn");

  SetProgress(2,5);
  if (e!=NULL)
    g = tp_brain_feature_gradient(e);
  SetProgress(3,5);
  if (g!=NULL && m!=NULL) {
    //VolumeSaveSCN(g,"tpg.scn");
    s = tp_brain_tree_pruning(g,m);
    //VolumeSaveSCN8(s,"tps.scn");
  }
  SetProgress(4,5);

  if (e!=NULL) VolumeDestroy(e);
  if (m!=NULL) VolumeDestroy(m);
  if (g!=NULL) VolumeDestroy(g);

  if (voldata.label != NULL && s!=NULL) {
    lb = labels_new_named_object("Brain (TP)",0xffaa8c);
    for(i=0;i<voldata.label->N;i++)
      if (s->u.i8[i]) voldata.label->u.i8[i] = (i8_t) lb;
  }

  if (s!=NULL) VolumeDestroy(s);
  SetProgress(5,5);
  notify.segchanged++;
}

