
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "ivs.h"
#include "util.h"
#include "filemgr.h"

#include "xpm/folder.xpm"
#include "xpm/upfolder.xpm"
#include "xpm/symlink.xpm"
#include "xpm/file.xpm"
#include "xpm/file-text.xpm"
#include "xpm/pfolder.xpm"

#include "xpm/frame-scn.xpm"
#include "xpm/frame-mvf.xpm"
#include "xpm/frame-vla.xpm"
#include "xpm/frame-ana.xpm"
#include "xpm/frame-pgm.xpm"
#include "xpm/frame-ppm.xpm"
#include "xpm/frame-minc.xpm"
#include "xpm/nopreview.xpm"

struct FileMgr mgr;

gboolean  mgr_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
void      mgr_readdir();
int       is_folder(char *path, char *name);
int       is_file(char *path, char *name);
int       is_symlink(char *path, char *name);
int       guess_type(char *path, char *name);
void      get_thumb(Clickable *c, char *path, char *name);
int       ncmp(char *subject, char *pat, int n);
gboolean  mgr_scan(gpointer data);
void *    mgr_scan_content(void *args);
gboolean  mgr_click(GtkWidget *widget, GdkEventButton *eb, gpointer data);
void      mgr_action(Clickable *c);
void      mgr_scroll(GtkAdjustment *a, gpointer data);

void create_filemgr_gui() {
  GtkStyle *mgrstyle;

  MemSet(&mgr,0, sizeof(mgr));
  getcwd(mgr.curdir, 511);
  mgr.curdir[511] = 0;

  mgr.itself = gtk_hbox_new(FALSE,0);
  mgr.canvas = gtk_drawing_area_new();  
  gtk_widget_set_events(mgr.canvas,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK);
  gtk_box_pack_start(GTK_BOX(mgr.itself),mgr.canvas,TRUE,TRUE,0);

  mgr.pl =  gtk_widget_create_pango_layout(mgr.canvas, NULL);
  mgr.pfd = pango_font_description_from_string("Sans 10");
  if (mgr.pl != NULL && mgr.pfd != NULL)
    pango_layout_set_font_description(mgr.pl, mgr.pfd);

  mgr.vadj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1.0, 
						  32.0, 320.0, 320.0);
  mgr.vscroll = gtk_vscrollbar_new(mgr.vadj);
  gtk_box_pack_start(GTK_BOX(mgr.itself),mgr.vscroll,FALSE,TRUE,0);

  sem_init(&(mgr.scan_needed), 0, 0);
  pthread_mutex_init(&(mgr.list_mutex),0);
  pthread_create(&(mgr.ft_tid),NULL,mgr_scan_content,0);

  mgrstyle = gtk_style_new();
  set_style_bg(mgrstyle, 0xbbbbbb);
  gtk_widget_set_style(mgr.canvas, mgrstyle);

  gtk_signal_connect(GTK_OBJECT(mgr.canvas), "expose_event",
		     GTK_SIGNAL_FUNC(mgr_expose), 0);
  gtk_signal_connect(GTK_OBJECT(mgr.canvas), "button_press_event",
		     GTK_SIGNAL_FUNC(mgr_click), 0);
  gtk_signal_connect(GTK_OBJECT(mgr.vadj), "value_changed",
		     GTK_SIGNAL_FUNC(mgr_scroll), 0);

  gtk_widget_show(mgr.canvas);
  gtk_widget_show(mgr.vscroll);
  gtk_widget_show(mgr.itself);

  gtk_timeout_add(800, mgr_scan, 0);
  mgr_readdir();
}

gboolean  mgr_scan(gpointer data) {
  static int last_sync = 0;
  struct stat dstat;  

  if (stat(mgr.curdir,&dstat)==0) {
    if (dstat.st_mtime > mgr.dir_mtime) {
      mgr_readdir();
      return TRUE;
    }
  }

  if (mgr.sync_req > last_sync) {
    last_sync = mgr.sync_req;
    refresh(mgr.canvas);
  }
  
  return TRUE;
}

gboolean  mgr_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data) {
  int ww,wh,cw,ch,ncol,x,y,tw,ty;
  Clickable *p;
  char z[512];
  int i,qx,qy;
  PangoRectangle pri, prl;

  if (!mgr.icons[0]) {

#define LDICON(a,b) mgr.icons[a]=gdk_pixmap_create_from_xpm_d(widget->window, &(mgr.masks[a]),&(widget->style->bg[GTK_STATE_NORMAL]),(gchar **)b)

    LDICON(0,folder_xpm);
    LDICON(1,upfolder_xpm);
    LDICON(2,symlink_xpm);
    LDICON(3,file_xpm);
    LDICON(4,frame_scn_xpm);
    LDICON(5,frame_vla_xpm);
    LDICON(6,frame_minc_xpm);
    LDICON(7,frame_ana_xpm);
    LDICON(8,file_text_xpm);
    LDICON(9,pfolder_xpm);
    LDICON(10,frame_pgm_xpm);
    LDICON(11,frame_ppm_xpm);
    LDICON(12,frame_mvf_xpm);
    LDICON(13,nopreview_xpm);

#undef LDICON

    for(i=0;i<13;i++) { mgr.icw[i]=52; mgr.ich[i]=45; }
    mgr.icw[4]=mgr.ich[4]=94;
    mgr.icw[5]=mgr.ich[5]=94;
    mgr.icw[6]=mgr.ich[6]=94;
    mgr.icw[7]=mgr.ich[7]=94;
    mgr.icw[10]=mgr.ich[10]=94;
    mgr.icw[12]=mgr.ich[12]=94;

  }

  mgr.g = graphics_validate_buffer(widget->window, mgr.g);
  gdk_window_get_size(widget->window, &ww, &wh);

  gc_color(mgr.g->gc, 0xaaaaaa);
  gdk_draw_rectangle(mgr.g->w, mgr.g->gc, TRUE, 0,0,ww,150);

  gc_color(mgr.g->gc, 0xbbbbbb);
  gdk_draw_rectangle(mgr.g->w, mgr.g->gc, TRUE, 0,150,ww,wh-150);

  gc_color(mgr.g->gc, 0);
  gdk_draw_line(mgr.g->w, mgr.g->gc, 0,149,ww,149);
  gc_color(mgr.g->gc, 0x404040);
  gdk_draw_line(mgr.g->w, mgr.g->gc, 0,150,ww,150);

  /* draw current dir */
  gc_color(mgr.g->gc,0);

  sprintf(z,"Current Path: %s",mgr.curdir);
  pango_layout_set_text(mgr.pl, z, -1);
  gdk_draw_layout(mgr.g->w, mgr.g->gc,10,2,mgr.pl);

  x=5; cw = 72;

  for(p=mgr.chead;p!=NULL;p=p->next) {
    p->x = x;
    p->y = 32;

    gc_clip(mgr.g->gc,p->x,0,72-5,150);
    gdk_draw_pixmap(mgr.g->w, mgr.g->gc, mgr.icons[p->icon], 
		    0,0, 
		    p->x+(72-52)/2,
		    p->y,
		    52,45);

    pango_layout_set_text(mgr.pl, p->caption[0], -1);
    pango_layout_get_pixel_extents(mgr.pl, &pri, &prl);
    tw = prl.width;
    if (tw < cw) tw = (cw - tw) / 2; else tw = 0;    
    gdk_draw_layout(mgr.g->w, mgr.g->gc, p->x+tw, p->y + 45 + 2, mgr.pl);

    if (p->next != NULL) {
      gc_clip(mgr.g->gc,0,0,mgr.g->W,150);
      gdk_draw_line(mgr.g->w, mgr.g->gc, 
		    p->x+((72-52)/2)+47, 
		    p->y+22,
		    p->x+80+((72-52)/2), 
		    p->y+22);
    }

    if (p != mgr.chead) {
      gc_clip(mgr.g->gc,0,0,mgr.g->W,150);
      gdk_draw_line(mgr.g->w, mgr.g->gc, 
		    p->x+((72-52)/2)+2, 
		    p->y+22,
		    p->x+((72-52)/2)-4, 
		    p->y+22);
    }

    x+=80;
  }

  /* draw files */

  cw = mgr.iw = 100;
  ch = mgr.ih = 140;
  ncol = (ww-10) / cw;
  x=5; y=155;
  ty = (int) (mgr.vadj->value);

  if (mgr.errcode) {
    gc_clip(mgr.g->gc,0,150,mgr.g->W,mgr.g->H-150);
    gc_color(mgr.g->gc, 0xff0000);
    if (mgr.errcode == 1)
      pango_layout_set_text(mgr.pl,"Folder contents cannot be read.",-1);
      gdk_draw_layout(mgr.g->w,mgr.g->gc,x,y+15,mgr.pl);
  }

  if (mgr.vadj_vignore) {
    --mgr.vadj_vignore;
  } else {
    mgr.vadj->lower = 0.0;
    if (mgr.tail)
      mgr.vadj->upper = (float) ((ch * mgr.tail->n) / ncol);
    else
      mgr.vadj->upper = 1.0;
    mgr.vadj->step_increment = ch;
    mgr.vadj->page_increment = wh-150;
    mgr.vadj->page_size      = ch;

    if (mgr.vadj->value > mgr.vadj->upper) {
      mgr.vadj->value = mgr.vadj->upper;
      ++mgr.vadj_ignore;
      gtk_adjustment_value_changed(mgr.vadj);
    }
    gtk_adjustment_changed(mgr.vadj);
    //gtk_range_slider_update(GTK_RANGE(mgr.vscroll)); TODO
  }

  pthread_mutex_lock(&(mgr.list_mutex));

  gc_color(mgr.g->gc, 0);
  for(p=mgr.head;p!=NULL;p=p->next) {
    if (p->ignore) continue;

    p->x = x + cw * (p->n % ncol);
    p->y = y + ch * (p->n / ncol) - ty;

    if (p->y > wh)
      break;

    gc_clip(mgr.g->gc,p->x,151,cw-5,wh-151);

    qx = p->x+(100-mgr.icw[p->icon])/2;
    qy = p->y+(100-mgr.ich[p->icon])/2;

    gdk_draw_pixmap(mgr.g->w, mgr.g->gc, mgr.icons[p->icon], 
		    0,0, qx, qy,
		    mgr.icw[p->icon], mgr.ich[p->icon]);

    if (p->thumb != 0) {
      gdk_draw_rgb_image(mgr.g->w, mgr.g->gc,
			 qx+7, qy+7, p->thumb->W, p->thumb->H,
			 GDK_RGB_DITHER_NORMAL,
			 p->thumb->val,
			 p->thumb->rowlen);
    } else {
      if ( (p->icon >=4 && p->icon <=7) || (p->icon >= 10 && p->icon <=12) )
	gdk_draw_pixmap(mgr.g->w, mgr.g->gc, mgr.icons[ICO_NOTHUMB], 
			0,0, qx+7, qy+7, 76, 64);
    }

    pango_layout_set_text(mgr.pl, p->caption[0], -1);
    pango_layout_get_pixel_extents(mgr.pl, &pri, &prl);
    tw = prl.width;
    if (tw < cw) tw = (cw - tw) / 2; else tw = 0;
    gdk_draw_layout(mgr.g->w, mgr.g->gc, p->x+tw, p->y + 98 + 2, mgr.pl);

    if (p->caption[1][0]) {
      gc_color(mgr.g->gc, 0x000080);
      pango_layout_set_text(mgr.pl, p->caption[1], -1);
      pango_layout_get_pixel_extents(mgr.pl, &pri, &prl);
      tw = prl.width;
      if (tw < cw) tw = (cw - tw) / 2; else tw = 0;
      gdk_draw_layout(mgr.g->w, mgr.g->gc, p->x+tw, p->y + 98 + 14, mgr.pl);
      gc_color(mgr.g->gc, 0);
    }

  }

  gc_clip(mgr.g->gc,0,0,mgr.g->W,mgr.g->H);
  pthread_mutex_unlock(&(mgr.list_mutex));

  graphics_commit_buffer(mgr.g, widget->window, widget->style->black_gc);    
  return FALSE;
}

void mgr_scroll(GtkAdjustment *a, gpointer data) {
  if (mgr.vadj_ignore) {
    --mgr.vadj_ignore;
    return;
  }
  ++mgr.vadj_vignore;
  refresh(mgr.canvas);
}

gboolean  mgr_click(GtkWidget *widget, GdkEventButton *eb, gpointer data) {
  Clickable *p,q;
  int a,x,y;
  
  if (eb->button != 1) return FALSE;

  pthread_mutex_lock(&(mgr.list_mutex));

  x = (int) (eb->x);
  y = (int) (eb->y);

  for(p=mgr.chead;p!=NULL;p=p->next) {
    if (x >= p->x && y >= p->y && x < p->x+mgr.iw && y < p->y+mgr.ih) {
      for(a = mgr.ctail->n - p->n;a>0;a--) {
	if (chdir("..")!=0) {
	  ivs_set_status("Unable to change directory.",10);
	  goto out;
	}
      }
      getcwd(mgr.curdir, 511);
      mgr.curdir[511] = 0; 
      pthread_mutex_unlock(&(mgr.list_mutex));
      mgr_readdir();
      return FALSE;
    }
  }

  for(p=mgr.head;p!=NULL;p=p->next) {
    if (p->ignore) continue;
    if (x >= p->x && y >= p->y && x < p->x+mgr.iw && y < p->y+mgr.ih) {
      MemCpy(&q,p,sizeof(Clickable));
      q.thumb = 0;
      pthread_mutex_unlock(&(mgr.list_mutex));
      mgr_action(&q);
      return FALSE;
    }
  }

 out:
  pthread_mutex_unlock(&(mgr.list_mutex));
  return FALSE;
}

void mgr_action(Clickable *c) {
  char catname[512];

  switch(c->icon) {
  case T_FOLDER:
  case T_LINK:
    if (chdir(c->caption[0])!=0) {
      ivs_set_status("Unable to change directory.",10);
    } else {
      pthread_mutex_lock(&(mgr.list_mutex));
      getcwd(mgr.curdir, 511);
      mgr.curdir[511] = 0; 
      pthread_mutex_unlock(&(mgr.list_mutex));
      mgr_readdir();
    }
    break;
  case T_UP:
    chdir("..");
    pthread_mutex_lock(&(mgr.list_mutex));
    getcwd(mgr.curdir, 511);
    mgr.curdir[511] = 0; 
    pthread_mutex_unlock(&(mgr.list_mutex));
    mgr_readdir();
    break;
  case T_VOL_SCN: // volumes that can be loaded directly
  case T_VOL_VLA:
  case T_VOL_MVF:
  case T_VOL_PGM:
  case T_VOL_HDR:
    sprintf(catname,"%s/%s",mgr.curdir,c->caption[0]);
    ivs_load_new(catname);
    ivs_go_to_pane(
		   ((c->icon==T_VOL_VLA)||
		    (c->icon==T_VOL_MVF))
		   ?2:1);
    break;
  case T_VOL_MINC:
    sprintf(catname,"%s/%s",mgr.curdir,c->caption[0]);
    ivs_load_convert_minc(catname);
    ivs_go_to_pane(1);
    break;
  case T_VOL_P6: // unsupported file
    ivs_set_status("IVS cannot load color pictures.",10);
    break;
  case T_BLANK:
    ivs_set_status("IVS was unable to detect this file's format.",10);
    break;
  default:
    printf("unknown type %d\n",c->icon);
  }
}

int folder_cmp(const void *a, const void *b) {
  return(strcasecmp((const char *) a, (const char *) b));
}

void mgr_readdir() {
  int i,j,k,nf,base;
  int sblen;
  char *sortbuf;
  DIR *d;
  struct dirent *de;
  Clickable *c;
  char z[64];
  struct stat dstat;

  mgr.errcode = 0;

  if (stat(mgr.curdir,&dstat)==0)
    mgr.dir_mtime = dstat.st_mtime;
  else
    mgr.dir_mtime = time(0);

  // build the curpath list

  if (mgr.chead != 0) {
    DestroyClickables(mgr.chead);
    mgr.chead = mgr.ctail = NULL;
  }

  mgr.chead = MakeClickable(0,0,T_SFOLDER,"/");
  mgr.ctail = mgr.chead;

  i=0;
  j=1;

  for(;;) {
    while(mgr.curdir[j] != 0 && mgr.curdir[j] != '/') 
      ++j;

    k=j-i-1;
    if (k<=0) break;
    if (k>64) k=64;
    MemSet(z,0,64);
    strncpy(z,&(mgr.curdir[i+1]),k);
    z[63]=0;

    c=MakeClickable(mgr.ctail->n+1, 0, T_SFOLDER, z);
    mgr.ctail->next = c;
    mgr.ctail = c;

    if (mgr.curdir[j]==0)
      break;
    i=j;
    ++j;
  }

  //

  pthread_mutex_lock(&(mgr.list_mutex));

  if (mgr.head != 0) {
    DestroyClickables(mgr.head);
    mgr.head = mgr.tail = 0;
    mgr.vadj->value = 0.0;
  }
  
  d = opendir(mgr.curdir);
  if (!d) {
    mgr.errcode = 1;
    ivs_set_status("Folder contents cannot be read.",10);
    goto mrd_refresh;
  }

  nf = 0;
  sblen = 64;
  sortbuf = (char *) malloc(sblen * 64);
  if (!sortbuf) goto mrd_fdir;

  // first directories
  while( (de=readdir(d)) != NULL ) {
    if (is_folder(mgr.curdir, de->d_name)) {
      if (!strcmp(de->d_name,"."))
	continue;
      if (nf == sblen) {
	sblen *= 2;
	sortbuf = realloc(sortbuf, sblen * 64);
	if (!sortbuf) goto mrd_fdir;
      }
      strncpy(&sortbuf[64*nf], de->d_name, 64);
      sortbuf[64*nf+63]=0;
      ++nf;
    }
  }

  qsort(sortbuf,nf,64,folder_cmp);
  base = 0;

  for(i=0;i<nf;i++) {
    if (!strcmp(&sortbuf[i*64],".."))
      c=MakeClickable(base+i,0,T_UP,"up");
    else
      c=MakeClickable(base+i,0,T_FOLDER,&sortbuf[i*64]);
    if (mgr.head == 0) {
      mgr.head = mgr.tail = c;
    } else {
      mgr.tail->next = c;
      mgr.tail = c;
    }
  }

  base+=nf;

  // then regular files
  nf = 0;
  rewinddir(d);
  
  while( (de=readdir(d)) != NULL ) {
    if (is_file(mgr.curdir, de->d_name)) {
      if (nf == sblen) {
	sblen *= 2;
	sortbuf = realloc(sortbuf, sblen * 64);
	if (!sortbuf) goto mrd_fdir;
      }
      strncpy(&sortbuf[64*nf], de->d_name, 64);
      sortbuf[64*nf+63]=0;
      ++nf;
    }
  }

  closedir(d);
  qsort(sortbuf,nf,64,folder_cmp);

  for(i=0;i<nf;i++) {
    c=MakeClickable(base+i,0,T_BLANK,&sortbuf[i*64]);
    if (mgr.head == 0) {
      mgr.head = mgr.tail = c;
    } else {
      mgr.tail->next = c;
      mgr.tail = c;
    }
  }
  base+=nf;

  free(sortbuf);

  sem_post(&(mgr.scan_needed)); /* request file content scan by
				   the other thread */
  goto mrd_refresh;

 mrd_fdir:
  closedir(d);
  
 mrd_refresh:
  pthread_mutex_unlock(&(mgr.list_mutex));
  mgr.vadj_ignore = 0;
  mgr.vadj_vignore = 0;
  gtk_adjustment_value_changed(mgr.vadj);
  refresh(mgr.canvas);
}

int is_folder(char *path, char *name) {
  char catname[512];
  struct stat s;
  sprintf(catname,"%s/%s",path,name);
  if (stat(catname, &s)==0)
    return(S_ISDIR(s.st_mode));
  return 0;
}

int is_file(char *path, char *name) {
  char catname[512];
  struct stat s;
  sprintf(catname,"%s/%s",path,name);
  if (stat(catname, &s)==0)
    return(S_ISREG(s.st_mode));
  return 0;
}

int is_symlink(char *path, char *name) {
  char catname[512];
  struct stat s;
  sprintf(catname,"%s/%s",path,name);
  if (lstat(catname, &s)==0)
    return(S_IFLNK & s.st_mode);
  return 0;
}

int ncmp(char *subject, char *pat, int n) {
  char *p, *q;
  p = subject;
  q = pat;
  while(n) {
    if (!*p || !*q) return 0;
    if (*p != *q) return 0;
    ++p; ++q; --n;
  }
  return 1;
}

void * mgr_scan_content(void *args) {
  Clickable *p;
  int t,i,j;

  for(;;) {
    sem_wait(&(mgr.scan_needed));
    pthread_mutex_lock(&(mgr.list_mutex));
    i = 0;
    for(p=mgr.head;p!=NULL;p=p->next) {
      if (p->icon == T_BLANK) {
	t = guess_type(mgr.curdir, p->caption[0]);
	if (t>0) { p->icon = t; ++i; }
	if (p->icon == T_VOL_SCN || p->icon == T_VOL_PGM)
	  get_thumb(p, mgr.curdir, p->caption[0]);
      }
      if (p->icon == T_FOLDER) {
	if (is_symlink(mgr.curdir, p->caption[0])) {
	  p->icon = T_LINK; ++i;
	}
      }
      if (p->icon == T_BLANK || p->icon == T_TEXT)
	p->ignore = 1;
    }

    j=0;
    for(p=mgr.head;p!=NULL;p=p->next) {
      p->n = j;
      if (!p->ignore) ++j;
    }

    if (i) ++mgr.sync_req;    
    pthread_mutex_unlock(&(mgr.list_mutex));
  }

  return 0;
}

int xnc_fgets(char *s, int m, FILE *f) {
  while(fgets(s,m,f)!=NULL)
    if (s[0]!='#') return 1;
  return 0;
}

void get_thumb(Clickable *c, char *path, char *name) {
  char catname[512],x[1024],b[10];
  FILE *f=0;
  int i,j,k,w,h,d,bpv,n;
  long off;
  unsigned char *tmp,*p,*r;
  short int *sp;
  int bigendian = 0;
  short int mv = 0;
  int sb;
  CImg *t;
  char o;

  sprintf(catname,"%s/%s",path,name);

  if (c->icon == T_VOL_PGM) {
    f = fopen(catname,"r");
    if (!f) return;
    if (!xnc_fgets(x,511,f)) goto bailout;
    o = x[1];
    if (!xnc_fgets(x,511,f)) goto bailout;
    if (sscanf(x," %d %d \n",&w,&h)!=2) goto bailout;
    if (!xnc_fgets(x,511,f)) goto bailout;
    
    tmp = (unsigned char *) malloc(2 * w * h);
    r = (unsigned char *) malloc(w * h);
    if (!tmp || !r) goto bailout;

    sprintf(c->caption[1],"%dx%d",w,h);

    if (o=='2') {
      // P2
      memset(b,0,10);
      n = 0;
      p = tmp;
      d = w*h;
      sp = (short int *) tmp;
      for(i=0;i!=d;) {
	j = fgetc(f);
	if (j<=32) {
	  if (n>0) {
	    sp[i++] = (short int) atoi(b);
	    memset(b,0,10); 
	    n=0;
	  }
	} else {
	  b[n++] = (char) j;
	}
      }
      fclose(f);
      mv = 0;
      for(i=0;i<d;i++)
	if (sp[i] > mv) mv = sp[i];
      sb = 0;
      while(mv > 255) { mv>>=1; ++sb; }
      for(i=0;i<n;i++) tmp[i] = (unsigned char) (sp[i] >> sb);
    } else {
      // P5

      for(i=0;i<h;i++)
	fread(&tmp[w*i],1,w,f);
      fclose(f);
    }

    while(w>76 || h>64) {
      for(i=0;i<h-1;i+=2)
	for(j=0;j<w-1;j+=2) {
	  k = (tmp[w*i+j] + tmp[w*(i+1)+j] + 
	    tmp[w*(i+1)+j+1] + tmp[w*i+j+1]) / 4;
	  r[(w/2)*(i/2)+(j/2)] = (unsigned char) k;
	}
      w/=2;
      h/=2;
      memcpy(tmp,r,w*h);
    }
    free(tmp);

    t = CImgNew(76,64);
    CImgFill(t, 0);
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	CImgSet(t, ((76-w)/2) + i, ((64-h)/2) + j,
		gray(r[w*j+i]));
    free(r);
    c->thumb = t;
    return;
  }

  if (c->icon == T_VOL_SCN) {
    f = fopen(catname,"r");
    if (!f) return;
    if (!xnc_fgets(x,511,f)) goto bailout;
    if (!xnc_fgets(x,511,f)) goto bailout;
    if (sscanf(x," %d %d %d \n",&w,&h,&d)!=3) goto bailout;
    if (!xnc_fgets(x,511,f)) goto bailout;
    if (!xnc_fgets(x,511,f)) goto bailout;
    if (sscanf(x," %d \n",&bpv)!=1) goto bailout;

    sprintf(c->caption[1],"%dx%dx%d",w,h,d);

    tmp = (unsigned char *) malloc(4 * w * h);
    r = (unsigned char *) malloc(w * h);
    if (!tmp || !r) goto bailout;

    tmp[0] = 0x00;
    tmp[1] = 0x01;
    sp = (short int *) tmp;
    bigendian = (sp[0] == 0x0001);

    bpv /= 8;
    off = w*h*bpv*(d/4);
    fseek(f, off, SEEK_CUR);

    p = tmp;
    for(i=0;i<h;i++) {
      j=fread(p, bpv, w, f);
      p+=w*bpv;
    }
    fclose(f);

    n= w*h;
    if (bpv==2) {
      for(i=0;i<n;i++) {
	if (bigendian)
	  sp[i] = (0x00ff&(sp[i]>>8)) | (0xff00&(sp[i]<<8));
	if (sp[i] > mv) mv = sp[i];
      }
      sb = 0;
      while(mv > 255) { mv>>=1; ++sb; }
      for(i=0;i<n;i++) tmp[i] = (unsigned char) (sp[i] >> sb);
    }

    while(w>76 || h>64) {
      for(i=0;i<h-1;i+=2)
	for(j=0;j<w-1;j+=2) {
	  k = (tmp[w*i+j] + tmp[w*(i+1)+j] + 
	    tmp[w*(i+1)+j+1] + tmp[w*i+j+1]) / 4;
	  r[(w/2)*(i/2)+(j/2)] = (unsigned char) k;
	}
      w/=2;
      h/=2;
      memcpy(tmp,r,w*h);
    }
    free(tmp);

    t = CImgNew(76,64);
    CImgFill(t, 0);
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	CImgSet(t, ((76-w)/2) + i, ((64-h)/2) + j,
		gray(r[w*j+i]));
    free(r);
    c->thumb = t;
    return;
  }

  bailout: 
  if (f) fclose(f); 
  return;
}

int guess_type(char *path, char *name) {
  char catname[512];
  unsigned char qk[1024];
  int  la,ua,nlen;
  int  i,n;
  FILE *f;

  sprintf(catname,"%s/%s",path,name);

  nlen = strlen(catname);
  if (!strcasecmp(&catname[nlen-4],".hdr"))
    if (GuessFileFormat(catname) == FMT_HDR) // all the dirty work
      return T_VOL_HDR;

  f=fopen(catname,"r");
  if (!f) return T_BLANK;
  n = fread(qk,1,1024,f);
  fclose(f);
  if (n<4) return T_BLANK;

  if (ncmp((char *) qk,"SCN\n",4)) return T_VOL_SCN;
  if (ncmp((char *) qk,"VLA\n",4)) return T_VOL_VLA;
  if (ncmp((char *) qk,"MVF\n",4)) return T_VOL_MVF;
  if (ncmp((char *) qk,"P2\n",3))  return T_VOL_PGM;
  if (ncmp((char *) qk,"P5\n",3))  return T_VOL_PGM;
  if (ncmp((char *) qk,"P6\n",3))  return T_VOL_P6;
  if (ncmp((char *) qk,"CDF",3))   return T_VOL_MINC;
  
  la=ua=0;
  for(i=0;i<n;i++)
    if (qk[i]>=32 && qk[i]<128) ++la; else ++ua;

  /* > 75% of file sample are printable chars */
  if (la > 3*ua) return T_TEXT;

  return T_BLANK;
}

Clickable * MakeClickable(int n,int action, int icon,char *caption) {
  Clickable *c;
  c = CREATE(Clickable);
  c->n = n;
  c->x = c->y = 0;
  c->action = action;
  c->icon   = icon;
  strncpy(c->caption[0],caption,64);
  c->caption[0][63] = 0;
  c->caption[1][0] = 0;
  c->next = 0;
  c->thumb = 0;
  c->ignore = 0;
  return c;
}

void DestroyClickables(Clickable *c) {
  if (c->next != 0)
    DestroyClickables(c->next);
  if (c->thumb)
    CImgDestroy(c->thumb);
  free(c);
}

void DestroyClickable(Clickable *c) {
  if (c->thumb)
    CImgDestroy(c->thumb);
  free(c);
}

