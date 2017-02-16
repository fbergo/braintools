
#define SOURCE_SESSION_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <libip.h>
#include <zlib.h>
#include "aftervoxel.h"

AftervoxelSession session;

void session_new() {
  int i,j;
  time_t t1,t2;
  
  // wait until the background thread queues are empty
  for(;;) {
    TaskQueueLock(res.bgqueue);
    i = GetTaskCount(res.bgqueue);
    j = res.bgqueue->state;
    TaskQueueUnlock(res.bgqueue);
    if (i > 0 || j != S_WAIT) {
      app_set_status("New Session: waiting pending tasks...",10);
      t1 = time(0);
      while(gtk_events_pending()) {
	gtk_main_iteration();
	t2 = time(0);
	if (t2 - t1 > 1)
	  break;
      }      
    } else break;
  }

  session_new_inner();
}

void session_new_inner() {
  int i;

  MutexLock(voldata.masterlock);
  session.filename[0] = 0;
  app_set_title(0);

  // clear image buffers

  for(i=0;i<3;i++) {
    if (v2d.base[i]) {
      CImgDestroy(v2d.base[i]);
      v2d.base[i] = 0;
    }
    if (v2d.scaled[i]) {
      CImgDestroy(v2d.scaled[i]);
      v2d.scaled[i] = 0;
    }
    if (v3d.sbase[i]) {
      CImgDestroy(v3d.sbase[i]);
      v3d.sbase[i] = 0;
    }
    if (v3d.sscaled[i]) {
      CImgDestroy(v3d.sscaled[i]);
      v3d.sscaled[i] = 0;
    }
  }
  if (v3d.base!=0) {
    CImgDestroy(v3d.base);
    v3d.base = 0;
  }
  if (v3d.scaled!=0) {
    CImgDestroy(v3d.scaled);
    v3d.scaled = 0;
  }

  // remove volume
  MutexLock(voldata.masterlock);
  if (voldata.original) {
    VolumeDestroy(voldata.original);
    voldata.original = NULL;
  }

  // remove labels
  if (voldata.label) {
    VolumeDestroy(voldata.label);
    voldata.label = NULL;
  }

  orient_init();
  undo_reset();

  MutexUnlock(voldata.masterlock);

  // measures
  measure_kill_all();

  // clear patient info
  info_reset();

  // load default object
  labels_init();
  labels_changed();

  // update right side (composition and object visibility)
  view3d_reset();
  view3d_set_gui();

  // update tonalization
  v2d.cpal = 0;
  v2d.pal = v2d.paldata[0];
  SliderSetValue(gui.ton, gui.ton->defval);
  SliderSetValue(gui.bri, gui.bri->defval);
  SliderSetValue(gui.con, gui.con->defval);
  refresh(gui.palcanvas);

  // force 4 views
  view.maximized = -1;

  ToolbarSetSelection(gui.t2d,0);
  ToolbarSetSelection(gui.tpen,0);
  ToolbarSetSelection(gui.t3d,0);

  MutexUnlock(voldata.masterlock);

  notify.sessionloaded++;
  view3d_invalidate(1,1);
  app_set_status("Session Started.",15);
}

void session_open() {
  char name[512];
  if (util_get_filename(mw,"Open Session",name,0)) {
    session_load_from(name);
  }
}

void session_save() {
  if (session.filename[0] != 0)
    session_save_to(session.filename);
  else
    session_save_as();
}

void session_save_as() {
  char name[512];

  if (util_get_filename(mw,"Save Session",name,0)) {
    strncpy(session.filename,name,512);
    session_save_to(session.filename);
  }

}

void session_save_to(char *path) {
  TaskNew(res.bgqueue,"Saving Session...",(Callback) &session_bgsave,
	  (void *) strdup(path));
}

void session_load_from(char *path) {
  TaskNew(res.bgqueue,"Loading Session...",(Callback) &session_bgload,
	  (void *) strdup(path));
}

void session_bgload(void *args) {
  char *path = (char *) args;
  char msg[512],buf[512];
  int i,j,k,err=0;
  gzFile f;  
  Volume *v;

  int tot,cur;

  SubTaskInit(8);
  SubTaskSetText(0,"Opening File");
  SubTaskSetText(1,"Read 1/7");
  SubTaskSetText(2,"Read 2/7");
  SubTaskSetText(3,"Read 3/7");
  SubTaskSetText(4,"Read 4/7");
  SubTaskSetText(5,"Read 5/7");
  SubTaskSetText(6,"Read 6/7");
  SubTaskSetText(7,"Read 7/7");

  SetProgress(0,100);
  SubTaskSet(0);
  f = gzopen(path,"r");
  if (!f) {
    snprintf(msg,512,"Session Error: %s (%d)",strerror(errno), errno);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    return;
  }

  SubTaskSet(1);
  i = gzread(f,buf,8);
  if (i==8)
    if (strcmp(buf,"AFTERVOX\n")!=0) // HERE
      i = 0;
  if (i!=8) {
    app_pop_message("Error","Not a valid session file.",MSG_ICON_ERROR);
    gzclose(f);
    return;
  }

  session_new_inner();
  MutexLock(voldata.masterlock);

  for(;;) {

    i = av_read_tag(f,buf);

    if (i<0) {
      app_pop_message("Error","Damaged File.",MSG_ICON_ERROR);
      gzclose(f);
      return;
    }

    if (!strcmp(buf,"END")) break;

    if (!strcmp(buf,"OBJ")) {
      labels.count   = av_read_int(f,&err);
      labels.current = av_read_int(f,&err);
      for(i=0;i<labels.count;i++) {
	labels.colors[i] = av_read_int(f,&err);
	av_read_string(f,labels.names[i],128,&err);
	labels.visible[i] = av_read_bool(f,&err);
      }      
      continue;
    }

    if (!strcmp(buf,"3D")) {
      av_read_transform(f,&(v3d.rotation),&err);
      v3d.panx = av_read_float(f,&err);
      v3d.pany = av_read_float(f,&err);
      v3d.zoom = av_read_float(f,&err);
      continue;
    }

    if (!strcmp(buf,"3D.SKIN")) {
      v3d.skin.value     = av_read_int(f,&err);
      v3d.skin.smoothing = av_read_int(f,&err);
      v3d.skin.color     = av_read_int(f,&err);
      v3d.skin.enable    = av_read_bool(f,&err);
      v3d.skin.opacity   = av_read_float(f,&err);
      continue;
    }

    if (!strcmp(buf,"3D.OCTANT")) {
      v3d.octant.enable   = av_read_bool(f,&err);
      v3d.octant.usedepth = av_read_bool(f,&err);
      continue;
    }

    if (!strcmp(buf,"3D.OBJECTS")) {
      v3d.obj.enable   = av_read_bool(f,&err);
      v3d.obj.opacity  = av_read_float(f,&err);
      continue;
    }

    if (!strcmp(buf,"3D.LIGHT")) {
      av_read_vector(f,&(v3d.light),&err);
      v3d.ka = av_read_float(f,&err);
      v3d.kd = av_read_float(f,&err);
      v3d.ks = av_read_float(f,&err);
      v3d.zgamma   = av_read_float(f,&err);
      v3d.zstretch = av_read_float(f,&err);
      v3d.sre      = av_read_int(f,&err);
    }

    if (!strcmp(buf,"2D")) {
      v2d.cx = av_read_int(f,&err);
      v2d.cy = av_read_int(f,&err);
      v2d.cz = av_read_int(f,&err);
      for(i=0;i<3;i++) {
	v2d.panx[i] = av_read_int(f,&err);
	v2d.pany[i] = av_read_int(f,&err);
      }
      v2d.zoom = av_read_float(f,&err);
      v2d.cpal = av_read_int(f,&err);
      view.maximized = av_read_int(f,&err);
      view.focus     = av_read_int(f,&err);
      v2d.pal = v2d.paldata[v2d.cpal];
      continue;
    }

    if (!strcmp(buf,"2D.2")) {
      gui.ton->value = av_read_float(f,&err);
      gui.bri->value = av_read_float(f,&err);
      gui.con->value = av_read_float(f,&err);     
      gui.t2d->selected = av_read_int(f,&err);
      gui.tpen->selected = av_read_int(f,&err);
      gui.t3d->selected = av_read_int(f,&err);
      continue;
    }

    if (!strcmp(buf,"2D.3")) {
      v2d.overlay[0] = av_read_bool(f,&err);
      v2d.overlay[1] = av_read_bool(f,&err);
      orient.main = av_read_int(f,&err);
      orient.mainorder[0] = av_read_int(f,&err);
      orient.mainorder[1] = av_read_int(f,&err);
      orient.mainorder[2] = av_read_int(f,&err);
      orient.mainorder[3] = av_read_int(f,&err);
      orient.zlo = av_read_int(f,&err);
      orient.zhi = av_read_int(f,&err);
      continue;
    }

    if (!strcmp(buf,"PATIENT")) {
      SubTaskSet(2);
      av_read_string(f,patinfo.name,128,&err);
      av_read_string(f,patinfo.age,16,&err);
      patinfo.gender = (char) av_read_int(f,&err);
      av_read_string(f,patinfo.scandate,32,&err);
      av_read_string(f,patinfo.institution,128,&err);
      av_read_string(f,patinfo.equipment,128,&err);
      av_read_string(f,patinfo.protocol,128,&err);
      patinfo.notes = av_read_string_alloc(f,&err);
      if (patinfo.notes)
	patinfo.noteslen = strlen(patinfo.notes) + 1;
      continue;
    }

    if (!strcmp(buf,"MEASURE")) {
      j = av_read_int(f,&err);
      for(i=0;i<j;i++) {
	av_read_int(f,&err);
	av_read_int(f,&err);
	av_read_int(f,&err);
	av_read_bool(f,&err);
	av_read_float(f,&err);
	for(k=0;k<3;k++) {
	  av_read_int(f,&err);
	  av_read_int(f,&err);
	}
      }
      continue;
    }

    if (!strcmp(buf,"VOLUME")) {
      SubTaskSet(3);
      i = av_read_int(f,&err);
      j = av_read_int(f,&err);
      k = av_read_int(f,&err);
      v = VolumeNew(i,j,k,vt_integer_16);
      if (!v) {
	error_memory();
	gzclose(f);
	return;
      }
      v->dx = av_read_float(f,&err);
      v->dy = av_read_float(f,&err);
      v->dz = av_read_float(f,&err);

      // FIXME: treat big endian machines
      j = v->H * v->D;
      k = v->W;
      for(i=0;i<j;i++) {
	gzread(f,v->u.i16+(i*k),2*k);
	cur=i;
	tot=j;
	if (i%32==0) SetProgress(cur,tot);
      }
      voldata.original = v;
      v = 0;
      v2d.vmax = VolumeMax(voldata.original);
      continue;
    }

    if (!strcmp(buf,"LABEL")) {
      SubTaskSet(4);
      i = av_read_int(f,&err);
      j = av_read_int(f,&err);
      k = av_read_int(f,&err);
      v = VolumeNew(i,j,k,vt_integer_8);
      if (!v) {
	error_memory();
	gzclose(f);
	return;
      }
      v->dx = av_read_float(f,&err);
      v->dy = av_read_float(f,&err);
      v->dz = av_read_float(f,&err);
      // FIXME: treat big endian machines
      j = v->H * v->D;
      k = v->W;
      for(i=0;i<j;i++) {
	gzread(f,v->u.i8+i*k,k);
	cur=i;
	tot=j;
	if (i%32==0) SetProgress(cur,tot);
      }
      voldata.label = v;
      v = 0;
      continue;
    }

  }

  SubTaskSet(7);
  gzclose(f);

  MutexUnlock(voldata.masterlock);  
  notify.sessionloaded++;
  EndProgress();
  SubTaskDestroy();
  strncpy(session.filename,path,512);
  app_set_title_file(path);
}

void session_bgsave(void *args) {
  char *path = (char *) args;
  char msg[512],*p;
  gzFile f;
  int i,j,k;
  i16_t *p16;
  i8_t *p8;

  int cur,tot;

  SubTaskInit(8);
  SubTaskSetText(0,"Opening File");
  SubTaskSetText(1,"Save 1/7");
  SubTaskSetText(2,"Save 2/7");
  SubTaskSetText(3,"Save 3/7");
  SubTaskSetText(4,"Save 4/7");
  SubTaskSetText(5,"Save 5/7");
  SubTaskSetText(6,"Save 6/7");
  SubTaskSetText(7,"Save 7/7");

  SubTaskSet(0);
  f = gzopen(path,"w2");
  if (!f) {
    snprintf(msg,512,"Session Error: %s (%d)",strerror(errno), errno);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    return;
  }

  SubTaskSet(1);
  cur=0;
  tot=4;

  gzprintf(f,"AFTERVOX\n");

  // object names and colors

  av_write_tag(f,"OBJ");
  av_write_int(f,labels.count);
  av_write_int(f,labels.current);
  for(i=0;i<labels.count;i++) {
    av_write_int(f,labels.colors[i]);
    av_write_string(f,labels.names[i]);
    av_write_bool(f,labels.visible[i]);
  }
  ++cur;
  SetProgress(cur,tot);

  // 3d vars

  av_write_tag(f,"3D");
  av_write_transform(f,&(v3d.rotation));
  av_write_float(f,v3d.panx);
  av_write_float(f,v3d.pany);
  av_write_float(f,v3d.zoom);

  av_write_tag(f,"3D.SKIN");
  av_write_int(f,v3d.skin.value);
  av_write_int(f,v3d.skin.smoothing);
  av_write_int(f,v3d.skin.color);
  av_write_bool(f,v3d.skin.enable);
  av_write_float(f,v3d.skin.opacity);
  
  av_write_tag(f,"3D.OCTANT");
  av_write_bool(f,v3d.octant.enable);
  av_write_bool(f,v3d.octant.usedepth);

  av_write_tag(f,"3D.OBJECTS");
  av_write_bool(f,v3d.obj.enable);
  av_write_float(f,v3d.obj.opacity);

  av_write_tag(f,"3D.LIGHT");
  av_write_vector(f,&(v3d.light));
  av_write_float(f,v3d.ka);
  av_write_float(f,v3d.kd);
  av_write_float(f,v3d.ks);
  av_write_float(f,v3d.zgamma);
  av_write_float(f,v3d.zstretch);
  av_write_int(f,v3d.sre);
  ++cur;
  SetProgress(cur,tot);

  // 2d vars

  av_write_tag(f,"2D");
  av_write_int(f,v2d.cx);
  av_write_int(f,v2d.cy);
  av_write_int(f,v2d.cz);
  for(i=0;i<3;i++) {
    av_write_int(f,v2d.panx[i]);
    av_write_int(f,v2d.pany[i]);
  }
  av_write_float(f,v2d.zoom);
  av_write_int(f,v2d.cpal);
  av_write_int(f,view.maximized);
  av_write_int(f,view.focus);

  av_write_tag(f,"2D.2");
  av_write_float(f,gui.ton->value);
  av_write_float(f,gui.bri->value);
  av_write_float(f,gui.con->value);
  av_write_int(f,gui.t2d->selected);
  av_write_int(f,gui.tpen->selected);
  av_write_int(f,gui.t3d->selected);

  av_write_tag(f,"2D.3");
  av_write_bool(f,v2d.overlay[0]); // patient info
  av_write_bool(f,v2d.overlay[1]); // orientation
  av_write_int(f,orient.main);
  av_write_int(f,orient.mainorder[0]);
  av_write_int(f,orient.mainorder[1]);
  av_write_int(f,orient.mainorder[2]);
  av_write_int(f,orient.mainorder[3]);
  av_write_int(f,orient.zlo);
  av_write_int(f,orient.zhi);

  ++cur;
  SetProgress(cur,tot);

  // patient info

  SubTaskSet(2);
  av_write_tag(f,"PATIENT");
  av_write_string(f,patinfo.name);
  av_write_string(f,patinfo.age);
  av_write_int(f,patinfo.gender);
  av_write_string(f,patinfo.scandate);
  av_write_string(f,patinfo.institution);
  av_write_string(f,patinfo.equipment);
  av_write_string(f,patinfo.protocol);
  if (patinfo.notes != 0)
    av_write_string(f,patinfo.notes);
  else
    av_write_string(f,"");
  SetProgress(1,2);

  // measurements

  // todo
  SetProgress(2,2);

  // original volume

  SubTaskSet(3);
  SetProgress(0,100);
  if (voldata.original) {
    av_write_tag(f,"VOLUME");
    av_write_int(f,voldata.original->W);
    av_write_int(f,voldata.original->H);
    av_write_int(f,voldata.original->D);
    av_write_float(f,voldata.original->dx);
    av_write_float(f,voldata.original->dy);
    av_write_float(f,voldata.original->dz);
    j = voldata.original->H * voldata.original->D;
    k = voldata.original->W;
    p16 = voldata.original->u.i16;
    // FIXME: treat big endian machines
    for(i=0;i<j;i++) {
      gzwrite(f,p16,2*k);
      p16 += k;
      if (i%32==0) SetProgress(i,j);
    }
  }
  
  // labels volume

  SubTaskSet(4);
  SetProgress(0,100);
  if (voldata.label) {
    av_write_tag(f,"LABEL");
    av_write_int(f,voldata.label->W);
    av_write_int(f,voldata.label->H);
    av_write_int(f,voldata.label->D);
    av_write_float(f,voldata.label->dx);
    av_write_float(f,voldata.label->dy);
    av_write_float(f,voldata.label->dz);
    j = voldata.label->H * voldata.label->D;
    k = voldata.label->W;
    p8 = voldata.label->u.i8;
    // FIXME: treat big endian machines
    for(i=0;i<j;i++) {
      gzwrite(f,p8,k);
      p8 += k;
      if (i%32==0) SetProgress(i,j);
    }
  }
  
  // enough
  SetProgress(cur,tot);
  av_write_tag(f,"END");

  SubTaskSet(7);
  gzclose(f);

  p = &path[strlen(path)];
  while(p!=path && *p != '/') --p;
  if (*p=='/') ++p;

  snprintf(msg,512,"Session successfully saved to %s",p);
  app_set_status(msg,20);
  free(args);
  EndProgress();
  SubTaskDestroy();
  app_set_title_file(path);
}

void av_write_tag(gzFile f, char *tag) {
  char x[32];
  MemSet(x,0,32);
  strncpy(x,tag,32);
  gzwrite(f,x,32);
}

int av_read_tag(gzFile f, char *tag) {
  int i;
  i = gzread(f,tag,32);
  return(i==32);
}

void av_write_int(gzFile f, int val) {
  char x[16];
  MemSet(x,0,16);
  snprintf(x,16,"%d",val);
  gzwrite(f,x,16);
}

int  av_read_int(gzFile f, int *err) {
  char x[16];
  int i;
  i = gzread(f,x,16);
  if (i!=16) { (*err)++; return 0; }
  i = atoi(x);
  return i;
}

void av_write_bool(gzFile f, int val) {
  gzputc(f,val?1:0);
}

int   av_read_bool(gzFile f, int *err) {
  int c;
  c = gzgetc(f);
  if (c < 0) { (*err)++; return 0; }
  return(c!=0);
}

void av_write_float(gzFile f, float val) {
  char x[64];
  MemSet(x,0,64);
  snprintf(x,63,"%.10f",val);  
  x[63] = 0;
  gzwrite(f,x,64);
}

float av_read_float(gzFile f, int *err) {
  char x[64];
  int i;
  float v;
  i = gzread(f,x,64);
  if (i!=64) { (*err)++; return 0.0; }
  v = atof(x);
  return v;
}

void av_write_string(gzFile f, char *s) {
  av_write_int(f,strlen(s));
  gzwrite(f,s,strlen(s));
}

char *av_read_string_alloc(gzFile f, int *err) {
  int i,len;
  char *s;
  len = av_read_int(f,err);
  if (!len) return 0;
  s = (char *) MemAlloc(len+1);
  if (!s) { (*err)++; return 0; }
  i = gzread(f,s,len);
  if (i!=len) (*err)++;
  return s;
}

void  av_read_string(gzFile f, char *s, int maxlen, int *err) {
  int i,len;

  s[0] = 0;
  len = av_read_int(f,err);
  if (!len) return;
  
  if (len > maxlen) {
    i = gzread(f,s,maxlen);
    s[maxlen-1] = 0;
    if (i!=maxlen) (*err)++;
    len -= maxlen;
    while(len) { 
      if (gzgetc(f)<0) { 
	(*err)++; 
	break; 
      } 
      --len; 
    }
  } else {
    i = gzread(f,s,len);
    if (i!=len) (*err)++;
  }
}

void av_write_transform(gzFile f, Transform *t) {
  int i,j;
  for(j=0;j<3;j++)
    for(i=0;i<3;i++)
      av_write_float(f,t->e[i][j]);
}

void av_read_transform(gzFile f, Transform *t, int *err) {
  int i,j;
  for(j=0;j<3;j++)
    for(i=0;i<3;i++)
      t->e[i][j] = av_read_float(f,err);
}

void av_write_vector(gzFile f, Vector *v) {
  av_write_float(f,v->x);
  av_write_float(f,v->y);
  av_write_float(f,v->z);
}

void  av_read_vector(gzFile f, Vector *v, int *err) {
  v->x = av_read_float(f,err);
  v->y = av_read_float(f,err);
  v->z = av_read_float(f,err);
}

int VolCmp(Volume *a, Volume *b) {
  int sz;

  if (a==0 && b==0) return 0;
  if (!a || !b) return 1;

  if (a->W != b->W ||
      a->H != b->H ||
      a->D != b->D) return 1;
  
  if (a->Type != b->Type) return 1;

  sz = a->N;
  
  switch(a->Type) {
  case vt_integer_8: break;
  case vt_integer_16: sz*=2; break;
  case vt_integer_32:
  case vt_float_32:   sz*=4; break;
  case vt_float_64:   sz*=8; break;
  }

  return(memcmp(a->u.i8,b->u.i8,(size_t) sz));
}
