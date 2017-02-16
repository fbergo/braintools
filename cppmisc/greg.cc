#include "brain_imaging.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "mygtk.h"
#include "greg.xpm"
#include <list>

#define GREG_CC 1
#include "greg.h"
#include "greg_msgd.h"
#include "greg_chs.h"

#define static static const
#include "xpm/greg_open1.xpm"
#include "xpm/greg_open2.xpm"
#include "xpm/greg_hnorm.xpm"
#include "xpm/greg_hmatch.xpm"
#include "xpm/greg_reg_msgd.xpm"
#include "xpm/greg_mask.xpm"
#include "xpm/greg_gray.xpm"
#include "xpm/greg_rg.xpm"
#include "xpm/greg_yp.xpm"
#include "xpm/greg_checkers.xpm"
#include "xpm/greg_redmask.xpm"
#include "xpm/greg_maskonly.xpm"
#include "xpm/greg_3d.xpm"
#include "xpm/greg_brain.xpm"
#include "xpm/greg_skin.xpm"
#include "xpm/greg_mask3d.xpm"
#undef static

#define VERSION "1.8"

GtkWidget *mw=NULL, *canvas, *sbar, *mth, *mor;
DropBox *cksz, *regalgo=NULL, *maskalgo=NULL;
GdkGC *gc = NULL;
PangoLayout *pl=NULL;
PangoFontDescription *sans10 = NULL, *sans10b = NULL;
int rpmode=0, maskmode=1, guimode=1;
GtkWidget *rpb[4], *mb[3], *t3[3];

Volume<int> *vol1 = NULL, *vol2 = NULL;
Volume<char> *mask = NULL;
char file1[512], file2[512];
int nmax[3] = {0,0,0}, wmax = 0, wmax2 = 0;
float wfac = 1.0;
int cursor[3] = {0,0,0}, pan[2] = {0,0};
float zoom = 1.0;
int zs[4] = {0,0,0,0}, ys[4] = {0,0,0,0}, ks[4] = {0,0,0,0};
double mvol = 0.0;

int renderok = 0;
Volume<char> *skin=NULL, *brain=NULL;
T3 rot;

struct _pane {
  int x,y,w,h;
} panes[8];

class ExposeState {

public:
  ExposeState() {
    int i;
    for(i=0;i<10;i++) {
      img[i] = NULL;
      zimg[i] = NULL;
      zf[i] = 1.0;
      valid[i] = false;
    }
  }
  ~ExposeState() {
    int i;
    for(i=0;i<10;i++) {
      if (img[i] != NULL) delete img[i];
      if (zimg[i] != NULL) delete zimg[i];
    }
  }

  void set(int i, Image *src) {
    if (i<0 || i>=10) return;
    if (img[i] != NULL) delete img[i];
    img[i] = new Image(*src);
    valid[i] = true;
    zf[i] = -1.0;
  }

  void scale(int i, float z) {
    if (i<0 || i>=10) return;
    if (img[i] == NULL) return;
    if (zf[i] != z || zimg[i] == NULL) {
      if (zimg[i] != NULL) delete zimg[i];
      zf[i] = z;
      zimg[i] = img[i]->scale(zf[i]);
    }
  }

  void invalidate(int i, int j) {
    int k;
    if (j<i) return;
    for(k=i;k<=j;k++)
      if (k>=0 && k<10) valid[k] = false;
  }

  void invalidate(int i) {
    if (i>=0 && i<10) valid[i] = false;
  }

  void invalidate() {
    int i;
    for(i=0;i<10;i++) valid[i] = false;
  }

  // 0,1,2 : v1 sag,cor,axi 3: v1 hist, 4,5,6: v2 sag,cor,axi, 7: v2 hist, 8: 3d 9: joint hist
  bool valid[10];
  Image *img[10], *zimg[10];
  float zf[10];
};

ExposeState xs;

gboolean gg_del(GtkWidget *w, GdkEvent *e, gpointer data);
void     gg_destroy(GtkWidget *w, gpointer data);
gboolean gg_expose(GtkWidget *w, GdkEventExpose *e, gpointer data);
gboolean gg_press(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean gg_release(GtkWidget *w, GdkEventButton *e, gpointer data);
gboolean gg_drag(GtkWidget *w, GdkEventMotion *e, gpointer data);
void status(const char *msg);
int  inbox(int x,int y,int x0,int y0,int w,int h);
int lut(int value,int side);
int envint(const char *var);
double envdouble(const char *var);

void gg_cmds(vector<string> &cmds);
void gg_title();
void gg_load1(GtkWidget *w, gpointer data);
void gg_load2(GtkWidget *w, gpointer data);
void gg_save2(GtkWidget *w, gpointer data);
void gg_savemask(GtkWidget *w, gpointer data);
void gg_about(GtkWidget *w, gpointer data);
void gg_quit(GtkWidget *w, gpointer data);

void gg_reg_tool(GtkWidget *w, gpointer data);
void gg_reg_msgd(GtkWidget *w, gpointer data);
void gg_reg_ch1(GtkWidget *w, gpointer data);
void gg_reg_ch1x(GtkWidget *w, gpointer data);
void gg_reg_ch2(GtkWidget *w, gpointer data);
void gg_reg_ch2c(GtkWidget *w, gpointer data);

void gg_regm_msgd(GtkWidget *w, gpointer data);
void gg_regm_ch1(GtkWidget *w, gpointer data);
void gg_regm_ch1x(GtkWidget *w, gpointer data);
void gg_regm_ch2(GtkWidget *w, gpointer data);
void gg_regm_ch2c(GtkWidget *w, gpointer data);

void gg_mat_load(GtkWidget *w, gpointer data);
void gg_mat_save(GtkWidget *w, gpointer data);

void gg_reg0(GtkWidget *w, gpointer data);
void gg_rot(GtkWidget *w, gpointer data);
void gg_wait();
gboolean gg_timer(gpointer data);
void gg_cmask(GtkWidget *w, gpointer data);
void gg_norm1(GtkWidget *w, gpointer data);
void gg_match1(GtkWidget *w, gpointer data);
void gg_close(GtkWidget *w, gpointer data);
void gg_init(const char *a, const char *b);
void cmd_orient(const char *a);
void cmd_regtoo(const char *src, const char *dest);

void gg_rpb(GtkToggleButton *b, gpointer data);
void gg_mb(GtkToggleButton *b, gpointer data);
void gg_toggle3d(GtkToggleButton *b, gpointer data);
void gg_toggle(GtkWidget *w);
void gg_comp3d(GtkWidget *w, gpointer data);

void left_label(int x,int y,int w,int h,const char *label, int color, int shadow);
void center_label(int x,int y,int w,int h,const char *label, int color, int shadow);

float comp_mask(Volume<int> *a, Volume<int> *b,Volume<char> *c,int algo,int x,int y,int z,int threshold, int open_radius);
float comp_mask_tp(Volume<int> *a, Volume<int> *b,Volume<char> *c,int algo,int x,int y,int z);
template<typename T> double vec_mean(vector<T> &v);
template<typename T> double vec_stdev(vector<T> &v);

void rot_fix_orientation(int v, int or1, int or2);

void nmax_comp();
void comp_surfaces();
void cpu_count();

void *bgtask_main(void *data);

void print_reg_summary();

#pragma GCC diagnostic ignored "-Wwrite-strings"
static GtkItemFactoryEntry gg_menu[] = {
  { "/_File",                    NULL, NULL, 0, "<Branch>" },
  { "/File/Load Volume _1...",     NULL, GTK_SIGNAL_FUNC(gg_load1),0,NULL},
  { "/File/Load Volume _2...",     NULL, GTK_SIGNAL_FUNC(gg_load2),0,NULL},
  { "/File/_Save Volume 2 As...",     NULL, GTK_SIGNAL_FUNC(gg_save2),0,NULL},
  { "/File/Save _Mask As...",     NULL, GTK_SIGNAL_FUNC(gg_savemask),0,NULL},
  { "/File/_Close All",         NULL, GTK_SIGNAL_FUNC(gg_close),0,NULL},
  { "/File/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/File/_About Greg...",     NULL, GTK_SIGNAL_FUNC(gg_about),0,NULL},
  { "/File/_Quit",               NULL, GTK_SIGNAL_FUNC(gg_quit),0,NULL},
  { "/_Intensity",                    NULL, NULL, 0, "<Branch>" },
  { "/Intensity/_Match Histograms", NULL, GTK_SIGNAL_FUNC(gg_match1), 0, NULL },
  { "/Intensity/_Normalize Histograms", NULL, GTK_SIGNAL_FUNC(gg_norm1), 0, NULL },
  { "/_Registration",                    NULL, NULL, 0, "<Branch>" },
  { "/Registration/_Assume Registrated",        NULL, GTK_SIGNAL_FUNC(gg_reg0), 0, NULL },
  { "/Registration/Change _Orientation...",        NULL, GTK_SIGNAL_FUNC(gg_rot), 0, NULL },
  { "/Registration/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Registration/Register (_MSGD-WS-FF)",        NULL, GTK_SIGNAL_FUNC(gg_reg_msgd), 0, NULL },
  { "/Registration/Register (CHS-WS-L_1)",        NULL, GTK_SIGNAL_FUNC(gg_reg_ch1), 0, NULL },
  { "/Registration/Register (CHS-WS-L1_X)",       NULL, GTK_SIGNAL_FUNC(gg_reg_ch1x), 0, NULL },
  { "/Registration/Register (CHS-WS-L2_C)",        NULL, GTK_SIGNAL_FUNC(gg_reg_ch2c), 0, NULL },
  { "/Registration/Register (CHS-WS-L_2)",        NULL, GTK_SIGNAL_FUNC(gg_reg_ch2), 0, NULL },
  { "/Registration/sep",                 NULL, NULL, 0, "<Separator>" },
  { "/Registration/Register (MSGD-MI-FF)",        NULL, GTK_SIGNAL_FUNC(gg_regm_msgd), 0, NULL },
  { "/Registration/Register (CHS-MI-L1)",        NULL, GTK_SIGNAL_FUNC(gg_regm_ch1), 0, NULL },
  { "/Registration/Register (CHS-MI-L1X)",       NULL, GTK_SIGNAL_FUNC(gg_regm_ch1x), 0, NULL },
  { "/Registration/Register (CHS-MI-L2C)",        NULL, GTK_SIGNAL_FUNC(gg_regm_ch2c), 0, NULL },
  { "/Registration/Register (CHS-MI-L2)",        NULL, GTK_SIGNAL_FUNC(gg_regm_ch2), 0, NULL },
  { "/_Matrix",                    NULL, NULL, 0, "<Branch>" },
  { "/Matrix/_Load Matrix and Register...",        NULL, GTK_SIGNAL_FUNC(gg_mat_load), 0, NULL },
  { "/Matrix/_Save Current Registration Matrix As...",        NULL, GTK_SIGNAL_FUNC(gg_mat_save), 0, NULL },

};

void gg_title() {
  /*
    char msg[1024];
    if (vol!=NULL)
    sprintf(msg,"GREG - %s",volfile);
    else
    strcpy(msg,"SpineSeg");
    gtk_window_set_title(GTK_WINDOW(mw), msg);
  */
}

void cksz_changed(void *data) {
  xs.invalidate();
  redraw(mw);
}

void gg_toggle3d(GtkToggleButton *b, gpointer data) {
  xs.invalidate(8);
  redraw(mw);
}

float comp_mask_tp(Volume<int> *a, Volume<int> *b,Volume<char> *c,int algo,int x,int y,int z) {
  Volume<int> *grad, *src;
  Volume<char> *seed, *tp;
  int i,j,k;
  float maskvol;

  lok.lock();
  bgtask.nsteps = 5;
  bgtask.step = 1;
  strcpy(bgtask.minor,"Preparing Data");
  lok.unlock();

  src  = new Volume<int>(a->W,a->H,a->D,a->dx,a->dy,a->dz);
  seed = new Volume<char>(a->W,a->H,a->D,a->dx,a->dy,a->dz);
  src->fill(0);
  seed->fill(0);
  switch(algo) {
  case 3: for(i=0;i<src->N;i++) src->voxel(i) = abs(a->voxel(i) - b->voxel(i)); break;
  case 4: for(i=0;i<src->N;i++) src->voxel(i) = a->voxel(i); break; 
  case 5: for(i=0;i<src->N;i++) src->voxel(i) = b->voxel(i); break;
  }

  lok.lock();
  bgtask.step = 2;
  strcpy(bgtask.minor,"Gradient Computation");
  lok.unlock();

  grad = src->morphGradient3D(1.0);
  //grad->writeSCN("grad.scn",16,true);
  
  for(i=x-1;i<=x+1;i++)
    for(j=y-1;j<=y+1;j++)
      for(k=z-1;k<=z+1;k++)
	if (seed->valid(i,j,k))
	  seed->voxel(i,j,k) = 1;

  lok.lock();
  bgtask.step = 3;
  strcpy(bgtask.minor,"Tree Pruning");
  lok.unlock();

  tp = src->treePruning(seed);
  for(i=0;i<c->N;i++)
    c->voxel(i) = tp->voxel(i);
  delete tp;
  delete grad;
  delete src;
  delete seed;  

  lok.lock();
  bgtask.step = 4;
  strcpy(bgtask.minor,"Calculating Volume");
  lok.unlock();

  j = 0;
  for(i=0;i<c->N;i++)
    if (c->voxel(i)) j++;
  maskvol = (j * a->dx * a->dy * a->dz);

  lok.lock();
  bgtask.step = 5;
  strcpy(bgtask.minor,"Computing Mask Border");
  lok.unlock();

  c->incrementBorder(1.0);
  return maskvol;
}

template<typename T> double vec_mean(vector<T> &v) {
  T sum = (T) 0.0;
  unsigned int i;
  for(i=0;i<v.size();i++)
    sum += v[i];
  return(sum / v.size());
}

template<typename T> double vec_stdev(vector<T> &v) {
  T mean, sum = (T) 0.0;
  unsigned int i;
  mean = vec_mean(v);
  for(i=0;i<v.size();i++)
    sum += (v[i]-mean)*(v[i]-mean);
  return( (T) sqrt(sum / v.size()));
}

// algo: 0=diff thres, 1=vol 1 homog, 2=vol 2 homog
float comp_mask(Volume<int> *a, Volume<int> *b,Volume<char> *c,int algo,int x,int y,int z,int threshold, int open_radius) {
  int s,i,j,d,p;
  list<int> Q;
  Location lp,lq;
  SphericalAdjacency A(1.0), B(1.5);
  float maskvol = 0.0;
  vector<int> sample;
  int mean=0, dev=0, nsd=2;

  if (c!=NULL)
    c->fill(0);
  if (a==NULL || b==NULL) return 0.0;

  if (algo >= 3) return(comp_mask_tp(a,b,c,algo,x,y,z));

  if (a->W != b->W || a->H != b->H || a->D != b->D) return 0.0;

  s = (a->voxel(x,y,z) - b->voxel(x,y,z) < 0) ? 1 : 0;  

  lok.lock();
  bgtask.nsteps = 5;
  bgtask.step = 1;
  strcpy(bgtask.minor,"Growing region");
  lok.unlock();
  
  Q.push_back(a->address(x,y,z));
  c->voxel(x,y,z) = 1;
  if (algo == 1) sample.push_back(a->voxel(x,y,z));
  if (algo == 2) sample.push_back(b->voxel(x,y,z));

  while(!Q.empty()) {
    p = Q.front();
    Q.pop_front();
    lp.set(a->xOf(p), a->yOf(p), a->zOf(p));

    for(i=0;i<A.size();i++) {
      lq = A.neighbor(lp, i);
      if (a->valid(lq))
	if (c->voxel(lq) == 0) {

	  switch(algo) {
	  case 0: // diff threshold
	    d = a->voxel(lq) - b->voxel(lq);
	    if ((d <= -threshold && s==1) || (d >= threshold && s==0)) {
	      c->voxel(lq) = 1;
	      Q.push_back(a->address(lq));
	    }
	  case 1: // v1 homogeneity
	    if (sample.size() < (unsigned int) threshold) {
	      c->voxel(lq) = 1;
	      Q.push_back(a->address(lq));
	      sample.push_back(a->voxel(lq));
	    } else {
	      if (mean==0 && dev==0) {
		mean = (int) vec_mean(sample);
		dev  = (int) vec_stdev(sample);
		printf("homogeneity 1: sample size=%d mean=%d stdev=%d nsd=%d\n",
		       (int) sample.size(),mean,dev,nsd);
	      }
	      d = abs(a->voxel(lq) - mean);
	      if (d <= nsd*dev) {
		c->voxel(lq) = 1;
		Q.push_back(a->address(lq));
	      }
	    }
	    break;
	  case 2: // v2 threshold
	    if (sample.size() < (unsigned int) threshold) {
	      c->voxel(lq) = 1;
	      Q.push_back(b->address(lq));
	      sample.push_back(b->voxel(lq));
	    } else {
	      if (mean==0 && dev==0) {
		mean = (int) vec_mean(sample);
		dev  = (int) vec_stdev(sample);
		printf("homogeneity 2: sample size=%d mean=%d stdev=%d, nsd=%d\n",
		       (int) sample.size(),mean,dev,nsd);
	      }
	      d = abs(b->voxel(lq) - mean);
	      if (d <= nsd*dev) {
		c->voxel(lq) = 1;
		Q.push_back(b->address(lq));
	      }
	    }
	    break;
	  } // switch

	}
    }
  }

  lok.lock();
  bgtask.step = 2;
  strcpy(bgtask.minor,"Morphological Open");
  lok.unlock();

  if (open_radius > 0 && algo <= 2) {
    Volume<char> *tmp;

    lok.lock();
    bgtask.step = 2;
    strcpy(bgtask.minor,"Morphological Open");
    lok.unlock();
    
    tmp = c->binaryOpen(open_radius / a->dx);
    c->fill(0);

    lok.lock();
    bgtask.step = 3;
    strcpy(bgtask.minor,"Selecting Component");
    lok.unlock();


    Q.push_back(a->address(x,y,z));
    c->voxel(x,y,z) = 1;
    while(!Q.empty()) {
      p = Q.front();
      Q.pop_front();
      lp.set(a->xOf(p), a->yOf(p), a->zOf(p));
      
      for(i=0;i<B.size();i++) {
	lq = B.neighbor(lp, i);
	if (a->valid(lq))
	  if (c->voxel(lq) == 0 && tmp->voxel(lq)!=0) {
	    c->voxel(lq) = 1;
	    Q.push_back(a->address(lq));
	  }
      }
    }
    delete tmp;
  }

  lok.lock();
  bgtask.step = 4;
  strcpy(bgtask.minor,"Calculating Volume");
  lok.unlock();

  j = 0;
  for(i=0;i<c->N;i++)
    if (c->voxel(i)) j++;
  maskvol = (j * a->dx * a->dy * a->dz);

  lok.lock();
  bgtask.step = 5;
  strcpy(bgtask.minor,"Computing Mask Border");
  lok.unlock();

  c->incrementBorder(1.0);
  return maskvol;
}


void gg_init(const char *a, const char *b) {
  Volume<int> *tmp;

  printf("cmd line load volume 1=%s\n",a);
  printf("cmd line load volume 2=%s\n",b);

  tmp = new Volume<int>(a);
  if (! tmp->ok()) {
    printf("** load volume 1 failed, will not try to open volume 2.\n");
    return;
  }
  vol1 = tmp->isometricInterpolation();
  delete tmp;
  mask = new Volume<char>(vol1->W,vol1->H,vol1->D,vol1->dx,vol1->dy,vol1->dz);
  strcpy(file1, a);

  cursor[0] = vol1->W / 2;
  cursor[1] = vol1->H / 2;
  cursor[2] = vol1->D / 2;

  tmp = new Volume<int>(b);
  if (tmp->ok()) {
    vol2 = tmp->interpolate(vol1->dx,vol1->dy,vol1->dz);
    delete tmp;
    strcpy(file2, b);
  } else {
    printf("** load volume 2 failed.\n");
  }

  nmax_comp();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
}

void cmd_regtoo(const char *src, const char *dest) {
  Volume<int> *a, *b;
  T4 mat;

  if (vol1 == NULL) {
    printf("** error: volume1 not loaded.\n");
    return;
  }
  a = new Volume<int>(src);
  mat = rbest.t;

  if (!a->ok()) {
    printf("** error reading %s\n",src);
    return;
  }
  b = a->interpolate(vol1->dx,vol1->dy,vol1->dz);
  delete a;
  a = mat_register(vol1, b, mat);
  printf("Writing %s...\n",dest);
  a->writeSCN(dest,16,true);
  delete a;
  delete b;
}

void cmd_orient(const char *a) {
  int i,v,o1,o2;
  i = atoi(a);
  v = i/100;
  o1 = (i%100)/10;
  o2 = i%10;
  printf("cmd orient %d (vol=%d from=%d to=%d)\n",i,v,o1,o2);
  rot_fix_orientation(v,o1,o2);
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
}

void gg_load1(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];
  Volume<int> *tmp;

  if (get_filename(mw, "Load Volume 1", name)) {
    
    tmp = new Volume<int>(name);
    if (! tmp->ok()) {
      sprintf(msg,"Failed to read volume from %s",name);
      status(msg);
      delete tmp;
      redraw(mw);
      return;
    }
    if (vol1 != NULL) {
      delete vol1;
      if (skin!=NULL) { delete skin; skin=NULL; renderok = 0; }
      if (brain!=NULL) { delete brain; brain=NULL; renderok = 0; }
    }
    if (mask != NULL) {
      delete mask;
      mvol = 0.0;
    }
    if (vol2 != NULL) {
      vol1 = tmp->interpolate(vol2->dx,vol2->dy,vol2->dz);
    } else {
      vol1 = tmp->isometricInterpolation();
    }
    mask = new Volume<char>(vol1->W,vol1->H,vol1->D,vol1->dx,vol1->dy,vol1->dz);
    delete tmp;
    nmax_comp();

    if (vol2 == NULL) {
      cursor[0] = vol1->W / 2;
      cursor[1] = vol1->H / 2;
      cursor[2] = vol1->D / 2;
    }

    strcpy(file1, name);    
    gg_toggle(rpb[0]);
    xs.invalidate();
    redraw(mw);
    sprintf(msg,"Finished loading volume 1 from %s",name);
    status(msg); 
  }
}

void gg_mat_load(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];
  T4 t;

  if (vol2 == NULL) return;
  if (get_filename(mw, "Load Registration Matrix", name)) {
    ifstream f;
    f.open(name);
    if (!f.good()) {
      sprintf(msg,"Error opening %s",name);
      status(msg);
    } else {
      f >> t;
      f.close();
      bgtask.algo = 15;
      gg_wait();
      xs.invalidate();
      redraw(mw);
      status("matrix Registration finished.");
    }
  }
}

void gg_mat_save(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];

  if (vol2 == NULL) return;
  if (get_filename(mw, "Save Current Registration Matrix", name)) {
    ofstream f;
    f.open(name);
    if (!f.good()) {
      sprintf(msg,"Error opening %s for writing",name);
      status(msg);
    } else {
      f << rbest.t;
      f.close();
      sprintf(msg,"Registration matrix saved to %s",name);
      status(msg);
    }
  }
}

void gg_savemask(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];
  int i;

  if (mask == NULL) return;
  if (get_filename(mw, "Save Volume mask", name)) {
    Volume<char> *mc = new Volume<char>(mask->W,mask->H,mask->D,mask->dx,mask->dy,mask->dz);
    mc->fill(0);
    for(i=0;i<mc->N;i++) if (mask->voxel(i) != 0) mc->voxel(i) = 1;
    mc->writeSCN(name, 8, false);
    delete mc;
    sprintf(msg,"Finished saving volume mask to %s",name);
    status(msg);
  }
}

void gg_save2(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];

  if (vol2 == NULL) return;
  if (get_filename(mw, "Save Volume 2", name)) {
    vol2->writeSCN(name, 16, true);
    sprintf(msg,"Finished saving volume 2 to %s",name);
    status(msg);
  }
}

void gg_load2(GtkWidget *w, gpointer data) {
  char name[512], msg[1024];
  Volume<int> *tmp;

  if (get_filename(mw, "Load Volume 2", name)) {
    
    tmp = new Volume<int>(name);
    if (! tmp->ok()) {
      sprintf(msg,"Failed to read volume from %s",name);
      status(msg);
      delete tmp;
      redraw(mw);
      return;
    }
    if (vol2 != NULL) {
      delete vol2;      
    }
    if (vol1 != NULL) {
      vol2 = tmp->interpolate(vol1->dx,vol1->dy,vol1->dz);
    } else {
      vol2 = tmp->isometricInterpolation();
    }
    delete tmp;
    nmax_comp();

    if (vol1 == NULL) {
      cursor[0] = vol2->W / 2;
      cursor[1] = vol2->H / 2;
      cursor[2] = vol2->D / 2;
    }

    strcpy(file2, name);
    gg_toggle(rpb[0]);
    xs.invalidate();
    redraw(mw);
    sprintf(msg,"Finished loading volume 2 from %s",name);
    status(msg); 
  }
}

void gg_reg0(GtkWidget *w, gpointer data) {

  Volume<int> *r;
  int i,j,k;

  if (vol1 == NULL || vol2 == NULL) return;

  r = new Volume<int>(vol1);
  r->fill(0);
  for(k=0;k<vol1->D;k++)
    for(j=0;j<vol1->H;j++)
      for(i=0;i<vol1->W;i++)
	if (vol2->valid(i,j,k))
	  r->voxel(i,j,k) = vol2->voxel(i,j,k);

  delete vol2;
  vol2 = r;
  nmax_comp();

  xs.invalidate();
  redraw(mw);
  status("Registration (Assume) Finished.");
}

void gg_reg_ch1(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 4;
  gg_wait();
  xs.invalidate();
  redraw(mw);
  status("Registration (CHS-WS-L1) finished.");
}

void gg_reg_ch1x(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 6;
  gg_wait();
  xs.invalidate();
  redraw(mw);
  status("Registration (CHS-WS-L1X) finished.");
}


void gg_reg_ch2(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 5;
  gg_wait();
  xs.invalidate();
  redraw(mw);
  status("Registration (CHS-WS-L2) finished.");
}

void gg_reg_ch2c(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 7;
  gg_wait();
  xs.invalidate();
  redraw(mw);
  status("Registration (CHS-WS-L2C) finished.");
}

void gg_reg_tool(GtkWidget *w, gpointer data) {

  int algo = dropbox_get_selection(regalgo);

  switch(algo) {
  case 0: gg_reg_msgd(w,data); break;
  case 1: gg_reg_ch1(w,data); break;
  case 2: gg_reg_ch1x(w,data); break;
  case 3: gg_reg_ch2c(w,data); break;
  case 4: gg_reg_ch2(w,data); break;

  case 5: gg_regm_msgd(w,data); break;
  case 6: gg_regm_ch1(w,data); break;
  case 7: gg_regm_ch1x(w,data); break;
  case 8: gg_regm_ch2c(w,data); break;
  case 9: gg_regm_ch2(w,data); break;
  }
}

void gg_reg_msgd(GtkWidget *w, gpointer data) {

  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 1;
  gg_wait();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (MSGD-WS-FF) finished.");
}

void gg_regm_ch1(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 11;
  gg_wait();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (CHS-MI-L1) finished.");
}

void gg_regm_ch1x(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 13;
  gg_wait();
  if (guimode) {    
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (CHS-MI-L1X) finished.");
}


void gg_regm_ch2(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 12;
  gg_wait();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (CHS-MI-L2) finished.");
}

void gg_regm_ch2c(GtkWidget *w, gpointer data) {
  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 14;
  gg_wait();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (CHS-MI-L2C) finished.");
}


void gg_regm_msgd(GtkWidget *w, gpointer data) {

  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  bgtask.algo = 10;
  gg_wait();
  if (guimode) {
    xs.invalidate();
    redraw(mw);
  }
  status("Registration (MSGD-MI-FF) finished.");
}


void gg_cmask(GtkWidget *w, gpointer data) {

  if (bgtask.algo >= 0) {
    status("Program busy.");
    return;
  }

  if (vol1==NULL || vol2==NULL) {
    status("Mask failed: No data.");
    return;
  }
  if (vol1->W != vol2->W || vol1->H != vol2->H || vol1->D != vol2->D) {
    status("Mask failed: Volumes must be registrated.");
    return;
  }

  bgtask.ipar[0] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mth));
  bgtask.ipar[1] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mor));
  bgtask.ipar[2] = cursor[0];
  bgtask.ipar[3] = cursor[1];
  bgtask.ipar[4] = cursor[2];
  bgtask.ipar[5] = dropbox_get_selection(maskalgo);

  bgtask.algo = 2;
  gg_wait();
  xs.invalidate(0,2);
  xs.invalidate(4,6);
  xs.invalidate(8);
  redraw(mw);
  status("Mask computation finished.");
}

void gg_quit(GtkWidget *w, gpointer data) {
  if (guimode)
    gtk_main_quit();
}

int inbox(int x,int y,int x0,int y0,int w,int h) {
  return(x>=x0 && y>=y0 && x<=(x0+w) && y<=(y0+h));
}

gboolean gg_del(GtkWidget *w, GdkEvent *e, gpointer data) {
  return FALSE;
}

void gg_destroy(GtkWidget *w, gpointer data) {
  gtk_main_quit();
}

void gg_wait() {
  gint id;

  if (guimode) {

    id = gtk_timeout_add(100, gg_timer, NULL);
    while(bgtask.algo > 0) {    
      if (gtk_events_pending())
	gtk_main_iteration();
      else
	usleep(50000);
    }
    gtk_timeout_remove(id);

  } else {

    while(bgtask.algo > 0)
      usleep(50000);

  }
}

gboolean gg_timer(gpointer data) {
  redraw(mw);
  return TRUE;
}

gboolean gg_expose(GtkWidget *w, GdkEventExpose *e, gpointer data) {
  int W,H;
  Image *img, *tmp, *unz;
  int ew=0,eh=0,ed=0,m,i,j,k,v1,v2,lx = -1,ly = -1, ms;
  char msg[1024];
  Volume<int> *src;
  Color c, green(0x00ff00), yellow(0xffff00), red(0xff0000), orange(0xff8000), 
    white(0xffffff), blue(0x000080), black(0);

  static int h1[256], h2[256], mh1=0, mh2=0, mhj=0;
  static int *jh=NULL;
  
  static int cszs[9] = {2,4,8,16,24,32,48,64,128};
  int csz;

  csz = cszs[ dropbox_get_selection(cksz) % 9 ];

  gdk_window_get_size(GDK_WINDOW(w->window),&W,&H);
  gdk_rgb_gc_set_foreground(gc,0x005500);
  gdk_draw_rectangle(w->window,gc,TRUE,0,0,W,H);

  pango_layout_set_font_description(pl, sans10b);

  m = rpmode;
  ms = maskmode;

  for(i=0;i<8;i++) memset(&panes[i],0,sizeof(panes[i]));

  lok.lock();

  if (!xs.valid[3]) {
    for(i=0;i<256;i++) h1[i]=h2[i]=1;
    if (vol1!=NULL) {
      for(i=0;i<vol1->N;i++) 
	if (vol1->voxel(i) >=0 && vol1->voxel(i) < 4096)
	  h1[vol1->voxel(i)/16]++;
      mh1 = 0;
      for(i=0;i<256;i++) {
	h1[i] = (int) (50.0 * log(h1[i]));
	mh1 = Max(mh1, h1[i]);
      }
    }
  }
  if (!xs.valid[7]) {    
    if (vol2!=NULL) {
      for(i=0;i<vol2->N;i++) 
	if (vol2->voxel(i) >=0 && vol2->voxel(i) < 4096)
	  h2[vol2->voxel(i)/16]++;
      mh2 = 0;
      for(i=0;i<256;i++) {
	h2[i] = (int) (50.0 * log(h2[i]));
	mh2 = Max(mh2, h2[i]);
      }
    }
  }

  // left panes
  
  if (vol1 != NULL) {    
   
    // sag

    if (!xs.valid[0]) {
      img = new Image(vol1->W,vol1->H);
      for(j=0;j<vol1->H;j++)
	for(i=0;i<vol1->W;i++) {
	  if (ms==2 && mask->voxel(i,j,cursor[2])==0)
	    c.gray(0);
	  else
	    c.gray(lut(vol1->voxel(i,j,cursor[2]),0));
	  if (i==cursor[0] || j==cursor[1])
	    c.mix(green, 0.33);
	  if (ms==1) {
	    if (mask->voxel(i,j,cursor[2])==2)
	      c.mix(red, 0.90);
	    else if (mask->voxel(i,j,cursor[2])==1)
	      c.mix(red, 0.25);
	  }
	  img->set(i,j,c);
	}

      xs.set(0,img);
      delete img;
    }

    xs.scale(0,zoom);
    tmp = xs.zimg[0];

    panes[0].x = 10+pan[0];
    panes[0].y = 10+pan[1];
    panes[0].w = tmp->W;
    panes[0].h = tmp->H;

    panes[1].x = panes[0].x + panes[0].w + 10;
    panes[1].y = panes[0].y;
    
    panes[2].x = panes[0].x;
    panes[2].y = panes[0].y + panes[0].h + 10;

    gdk_draw_rgb_image(w->window,gc,panes[0].x,panes[0].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));

    // cor

    if (!xs.valid[1]) {
      img = new Image(vol1->D,vol1->H);
      for(j=0;j<vol1->H;j++)
	for(i=0;i<vol1->D;i++) {
	  if (ms==2 && mask->voxel(cursor[0],j,i)==0)
	    c.gray(0);
	  else
	    c.gray(lut(vol1->voxel(cursor[0],j,i),0));
	  if (i==cursor[2] || j==cursor[1])
	    c.mix(green, 0.33);
	  if (ms==1) {
	    if (mask->voxel(cursor[0],j,i)==2)
	      c.mix(red, 0.90);
	    else if (mask->voxel(cursor[0],j,i)==1)
	      c.mix(red, 0.25);
	  }
	  img->set(i,j,c);
	}
      
      xs.set(1,img);
      delete img;
    }

    xs.scale(1,zoom);
    tmp = xs.zimg[1];
    panes[1].w = tmp->W;
    panes[1].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[1].x,panes[1].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));

    // axi

    if (!xs.valid[2]) {
      img = new Image(vol1->W,vol1->D);
      for(j=0;j<vol1->D;j++)
	for(i=0;i<vol1->W;i++) {
	  if (ms==2 && mask->voxel(i,cursor[1],j)==0)
	    c.gray(0);
	  else
	    c.gray(lut(vol1->voxel(i,cursor[1],j),0));
	  if (i==cursor[0] || j==cursor[2])
	    c.mix(green, 0.33);
	  if (ms==1) {
	    if (mask->voxel(i,cursor[1],j)==2)
	      c.mix(red, 0.90);
	    else if (mask->voxel(i,cursor[1],j)==1)
	      c.mix(red, 0.25);
	  }
	  img->set(i,j,c);
	}

      xs.set(2,img);
      delete img;
    }

    xs.scale(2,zoom);
    tmp = xs.zimg[2];
    panes[2].w = tmp->W;
    panes[2].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[2].x,panes[2].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));

    lx = panes[0].x;
    ly = panes[2].y + panes[2].h + 20;

    { //histogram 1
      int hx, hy, hw;
      hx = panes[1].x;
      hy = panes[2].y;
      hw = Min(256,panes[1].w);

      if (!xs.valid[3]) {
	tmp = new Image(hw,50);
	tmp->fill(white);

	for(i=0;i<256;i++)
	  tmp->line((i*hw)/256,50,
		    (i*hw)/256,50-((h1[i]*50)/mh1),blue);
	tmp->rect(0,0,hw,50,black,false);
	xs.set(3,tmp);
	delete tmp;
      }

      tmp = xs.img[3];
      gdk_draw_rgb_image(w->window,gc,hx,hy,
			 tmp->W,tmp->H,
			 GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));
    }
  }

  // right panes
  if (vol2 != NULL) {

    if (vol1 != NULL) {
      ew = Min(vol1->W,vol2->W);
      eh = Min(vol1->H,vol2->H);
      ed = Min(vol1->D,vol2->D);

      if (cursor[0] >= ew) cursor[0] = ew-1;
      if (cursor[1] >= eh) cursor[1] = eh-1;
      if (cursor[2] >= ed) cursor[2] = ed-1;

    } else {
      ew = vol2->W;
      eh = vol2->H;
      ed = vol2->D;
      m = 0;
    }

    panes[4].x = panes[1].x + panes[1].w + 10;
    panes[4].y = 10 + pan[1];

    // sag

    if (!xs.valid[4]) {
      img = new Image(ew,eh);
      switch(m) {
      case 0: // plain
	for(j=0;j<eh;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,j,cursor[2])==0)
	      c.gray(0);
	    else
	      c.gray(lut(vol2->voxel(i,j,cursor[2]),1));
	    if (i==cursor[0] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,j,cursor[2])==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,j,cursor[2])==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 1: // checkers
	for(j=0;j<eh;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,j,cursor[2])==0)
	      c.gray(0);
	    else {
	      if ((i/csz + j/csz + cursor[2]/csz)%2) src=vol1; else src=vol2;
	      c.gray(lut(src->voxel(i,j,cursor[2]),src==vol2));
	    }
	    if (i==cursor[0] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,j,cursor[2])==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,j,cursor[2])==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 2: // red/green
	for(j=0;j<eh;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,j,cursor[2])==0)
	      c.gray(0);
	    else {
	      c.R = lut(vol1->voxel(i,j,cursor[2]),0);
	      c.G = lut(vol2->voxel(i,j,cursor[2]),1);
	      c.B = (c.R+c.G)/2;
	    }
	    if (i==cursor[0] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,j,cursor[2])==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,j,cursor[2])==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 3: // difference
	for(j=0;j<eh;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,j,cursor[2])==0)
	      c.gray(0);
	    else {
	      v1 = vol1->voxel(i,j,cursor[2]);
	      v2 = (vol2->voxel(i,j,cursor[2]) * wmax) / wmax2;
	      c.gray(lut(v1-v2,0));
	      if (v1 > v2) c.B /= 2; else c.G /= 2;
	    }
	    if (i==cursor[0] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,j,cursor[2])==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,j,cursor[2])==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      }
      
      xs.set(4,img);
      delete img;
    }

    xs.scale(4,zoom);
    tmp = xs.zimg[4];
    panes[4].w = tmp->W;
    panes[4].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[4].x,panes[4].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));

    // cor

    if (!xs.valid[5]) {
      img = new Image(ed,eh);
      switch(m) {
      case 0: // plain
	for(j=0;j<eh;j++)
	  for(i=0;i<ed;i++) {
	    if (ms==2 && mask->voxel(cursor[0],j,i)==0)
	      c.gray(0);
	    else
	      c.gray(lut(vol2->voxel(cursor[0],j,i),1));
	    if (i==cursor[2] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(cursor[0],j,i)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(cursor[0],j,i)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 1: // checkers
	for(j=0;j<eh;j++)
	  for(i=0;i<ed;i++) {
	    if (ms==2 && mask->voxel(cursor[0],j,i)==0)
	      c.gray(0);
	    else {
	      if ((i/csz + j/csz + cursor[0]/csz)%2) src=vol1; else src=vol2;
	      c.gray(lut(src->voxel(cursor[0],j,i),src==vol2));
	    }
	    if (i==cursor[2] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(cursor[0],j,i)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(cursor[0],j,i)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 2: // red/green
	for(j=0;j<eh;j++)
	  for(i=0;i<ed;i++) {
	    if (ms==2 && mask->voxel(cursor[0],j,i)==0)
	      c.gray(0);
	    else {
	      c.R = lut(vol1->voxel(cursor[0],j,i),0);
	      c.G = lut(vol2->voxel(cursor[0],j,i),1);
	      c.B = (c.R+c.G)/2;
	    }
	    if (i==cursor[2] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(cursor[0],j,i)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(cursor[0],j,i)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 3: // difference
	for(j=0;j<eh;j++)
	  for(i=0;i<ed;i++) {
	    if (ms==2 && mask->voxel(cursor[0],j,i)==0)
	      c.gray(0);
	    else {
	      v1 = vol1->voxel(cursor[0],j,i);
	      v2 = (vol2->voxel(cursor[0],j,i) * wmax) / wmax2;
	      c.gray(lut(v1-v2,0));
	      if (v1 > v2) c.B /= 2; else c.G /= 2;
	    }
	    if (i==cursor[2] || j==cursor[1]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(cursor[0],j,i)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(cursor[0],j,i)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      }
           
      xs.set(5,img);
      delete img;
    }

    xs.scale(5,zoom);
    tmp = xs.zimg[5];
    panes[5].x = panes[4].x + panes[4].w + 10;
    panes[5].y = panes[4].y;
    panes[5].w = tmp->W;
    panes[5].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[5].x,panes[5].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));
    
    // axi
    if (!xs.valid[6]) {
      img = new Image(ew,ed);
      switch(m) {
      case 0: // plain
	for(j=0;j<ed;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,cursor[1],j)==0)
	      c.gray(0);
	    else
	      c.gray(lut(vol2->voxel(i,cursor[1],j),1));
	    if (i==cursor[0] || j==cursor[2]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,cursor[1],j)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,cursor[1],j)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 1: // checkers
	for(j=0;j<ed;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,cursor[1],j)==0)
	      c.gray(0);
	    else {
	      if ((i/csz + j/csz + cursor[1]/csz)%2) src=vol1; else src=vol2;
	      c.gray(lut(src->voxel(i,cursor[1],j),src==vol2));
	    }
	    if (i==cursor[0] || j==cursor[2]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,cursor[1],j)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,cursor[1],j)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 2: // red/green
	for(j=0;j<ed;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,cursor[1],j)==0)
	      c.gray(0);
	    else {
	      c.R = lut(vol1->voxel(i,cursor[1],j),0);
	      c.G = lut(vol2->voxel(i,cursor[1],j),1);
	      c.B = (c.R+c.G)/2;
	    }
	    if (i==cursor[0] || j==cursor[2]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,cursor[1],j)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,cursor[1],j)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      case 3: // difference
	for(j=0;j<ed;j++)
	  for(i=0;i<ew;i++) {
	    if (ms==2 && mask->voxel(i,cursor[1],j)==0)
	      c.gray(0);
	    else {
	      v1 = vol1->voxel(i,cursor[1],j);
	      v2 = (vol2->voxel(i,cursor[1],j) * wmax) / wmax2;
	      c.gray(lut(v1-v2,1));
	      if (v1 > v2) c.B /= 2; else c.G /= 2;
	    }
	    if (i==cursor[0] || j==cursor[2]) c.mix(green, 0.33);
	    if (ms==1) {
	      if (mask->voxel(i,cursor[1],j)==2)
		c.mix(red, 0.90);
	      else if (mask->voxel(i,cursor[1],j)==1)
		c.mix(red, 0.25);
	    }
	    img->set(i,j,c);
	  }
	break;
      }
            
      xs.set(6,img);
      delete img;
    }

    xs.scale(6,zoom);
    tmp = xs.zimg[6];
    panes[6].x = panes[4].x;
    panes[6].y = panes[4].y + panes[4].h + 10;
    panes[6].w = tmp->W;
    panes[6].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[6].x,panes[6].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));

    if (lx < 0) {
      lx = panes[4].x;
      ly = panes[6].y + panes[6].h + 20;
    }

    { // histogram 2
      int hx, hy, hw;
      hx = panes[5].x;
      hy = panes[6].y;
      hw = Min(256,panes[5].w);
      
      if (!xs.valid[7]) {
	tmp = new Image(hw,50);
	tmp->fill(white);

	for(i=0;i<256;i++)
	  tmp->line((i*hw)/256,50,
		    (i*hw)/256,50-((h2[i]*50)/mh2),blue);
	tmp->rect(0,0,hw,50,black,false);
	xs.set(7,tmp);
	delete tmp;
      }

      tmp = xs.img[7];
      gdk_draw_rgb_image(w->window,gc,hx,hy,
			 tmp->W,tmp->H,
			 GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));
    }
    
  }

  // joint histogram
  if (jh==NULL) jh = (int *) malloc(256*256*sizeof(int));

  if (vol1!=NULL && vol2!=NULL) {
    int hw,hx,hy,ma,mb,sa=0,sb=0,jhs=64;

    hx = panes[5].x;
    hy = panes[6].y + 60;
    hw = Min(256,panes[5].w);

    if (!xs.valid[9]) {
      ma = vol1->maximum();
      mb = vol2->maximum();
      while(ma > jhs-1) { ma >>= 1; sa++; } 
      while(mb > jhs-1) { mb >>= 1; sb++; } 

      for(i=0;i<jhs*jhs;i++) jh[i] = 0;
      
      for(k=0;k<ed;k++)
	for(j=0;j<eh;j++)
	  for(i=0;i<ew;i++) {
	    v1 = vol1->voxel(i,j,k) >> sa;
	    v2 = vol2->voxel(i,j,k) >> sb;
	    if (v1 < 0) v1 = 0;
	    if (v2 < 0) v2 = 0;
	    jh[ v1*jhs + v2 ]++;
	  }
      
      // max
      mhj = 0;
      for(i=0;i<jhs*jhs;i++) {
	jh[i] = (int) (100.0 * log(Max(1,jh[i])));
	mhj = Max(mhj, jh[i]);
      }
	    
      tmp = new Image(hw,hw);
      tmp->fill(white);
      
      for(i=0;i<jhs;i++)
	for(j=0;j<jhs;j++) {
	  c.B = 255;
	  c.R = c.G = 255 - ((255 * jh[j*jhs + i]) / mhj);
	  tmp->rect((i*hw*(256/jhs)) / 256, (j*hw*(256/jhs)) / 256, 256/jhs, 256/jhs, c, true);
	}

      tmp->rect(0,0,hw,hw,black,false);
      xs.set(9,tmp);
      delete tmp;
    }
    
    tmp = xs.img[9];
    gdk_draw_rgb_image(w->window,gc,hx,hy,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));    
  }


  // RENDER
  if (vol1 != NULL && renderok) {

    if (!xs.valid[8]) {
      Color cs(0xe6cca9), cb(0xdfcb7d), cm(0xcc1111), dbg(0x003300);
      int rs[3];
      RenderingContext *rc;
      //cout << "new render\n";

      unz = new Image(vol1->diagonalLength(),vol1->diagonalLength());
      unz->fill(dbg);
      rc = new RenderingContext(*unz);
      rc->prepareFirst();
      
      for(i=0;i<3;i++)
	rs[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(t3[i]));

      if (rs[2] && mask!=NULL && mvol > 0.0)
	mask->render(*unz,rot,*rc,cm,1.0,RenderType_EQ,2);
      if (rs[1] && brain!=NULL)
	brain->render(*unz,rot,*rc,cb,0.7,RenderType_EQ,2);
      if (rs[0] && skin!=NULL)
	skin->render(*unz,rot,*rc,cs,0.5,RenderType_EQ,2);

      delete rc;
      xs.set(8,unz);
      delete unz;
    }

    xs.scale(8,zoom);
    tmp = xs.zimg[8];

    if (vol2!=NULL)
      panes[3].x = panes[5].x + panes[5].w + 10;
    else
      panes[3].x = panes[1].x + panes[1].w + 10;
    panes[3].y = panes[1].y;
    panes[3].w = tmp->W;
    panes[3].h = tmp->H;

    gdk_draw_rgb_image(w->window,gc,panes[3].x,panes[3].y,
		       tmp->W,tmp->H,
		       GDK_RGB_DITHER_NORMAL, tmp->getBuffer(), 3*(tmp->W));    
  }


  if (vol1 != NULL) {
    sprintf(msg,"Volume #1: %s",file1);
    left_label(lx,ly, 400, 14, msg, 0xffffff, 1);
    sprintf(msg,"%d x %d x %d (%.2f mm³)", vol1->W, vol1->H, vol1->D, vol1->dx);
    left_label(lx,ly+20, 400, 14, msg, 0xffffff, 1);
    ly += 40;
  }
  if (vol2 != NULL) {
    sprintf(msg,"Volume #2: %s",file2);
    left_label(lx,ly, 400, 14, msg, 0xffffff, 1);
    sprintf(msg,"%d x %d x %d (%.2f mm³)", vol2->W, vol2->H, vol2->D, vol2->dx);
    left_label(lx,ly+20, 400, 14, msg, 0xffffff, 1);
    ly += 40;
  }
  if (vol1 != NULL && vol2 != NULL) {
    sprintf(msg,"Voxel (%d,%d,%d) (1) %d (2) %d Diff %d",
	    cursor[0],cursor[1],cursor[2], 
	    vol1->voxel(cursor[0],cursor[1],cursor[2]),
	    vol2->voxel(cursor[0],cursor[1],cursor[2]),
	    vol1->voxel(cursor[0],cursor[1],cursor[2])-vol2->voxel(cursor[0],cursor[1],cursor[2]));
    left_label(lx,ly, 400, 14, msg, 0xffffff, 1);
    ly += 20;
    if (mvol > 0.0) {
      sprintf(msg,"Mask volume: %.1f mm³",mvol);
      left_label(lx,ly, 400, 14, msg, 0xffffff, 1);
      ly += 20;
    }
  }

  gc_color(gc,0xffffff);
  for(i=0;i<7;i++)
    if (panes[i].w != 0)
      gdk_draw_rectangle(w->window,gc,FALSE,panes[i].x,panes[i].y,panes[i].w,panes[i].h);

  // bg overlay
  if (bgtask.algo > 0) {
    Timestamp b;
    int s,nt;
    int bx,by,bw,bh;

    bw = 600;
    bh = 90;
    bx = (W-bw)/2;
    by = (H-bh)/2;

    gc_color(gc,0xc0c0c0);
    gdk_draw_rectangle(w->window,gc,TRUE, bx, by, bw, bh);
    gc_color(gc,0);
    gdk_draw_rectangle(w->window,gc,FALSE, bx, by, bw, bh);
    s = (int) (b-bgtask.start);
    sprintf(msg,"%s (%.2d:%.2d)",bgtask.major, s/60, s%60);
    left_label(bx+20,by+10,560,14, msg, 0, 0);
    left_label(bx+20,by+30,560,14, bgtask.minor, 0, 0);

    if (bgtask.nsteps > 0) {
      gc_color(gc,0x00ffff);
      gdk_draw_rectangle(w->window,gc,TRUE, bx+20, by+50, 560, 10);
      gc_color(gc,0x000088);
      gdk_draw_rectangle(w->window,gc,TRUE, bx+20, by+50, 
			 (560*bgtask.step)/bgtask.nsteps, 10);
      gc_color(gc,0);
      gdk_draw_rectangle(w->window,gc,FALSE, bx+20, by+50, 560, 10);
    }

    nt = busythreads;
    for(s=0;s<ncpus;s++) {
      gc_color(gc, s < nt ? 0x00ee00 : 0x446644);
      gdk_draw_rectangle(w->window,gc,TRUE, bx + 20 + 12*s, by + 64, 10,10);
      gc_color(gc, 0);
      gdk_draw_rectangle(w->window,gc,FALSE, bx + 20 + 12*s, by + 64, 10,10);
    }

    // score graph
    if (rbest.nhist > 1) {
      double mv = 5000.0;
      double nv = rbest.scorehist[0];
      int xi = 15, yv1, yv2;

      by = by + bh + 10;
      bw = 250;
      bh = 100;
      gc_color(gc, 0xffffff);
      gdk_draw_rectangle(w->window,gc,TRUE,bx,by,bw,bh);

      for(s=0;s<rbest.nhist;s++) {
	if (rbest.scorehist[s] > mv)
	  mv = rbest.scorehist[s];
	if (rbest.scorehist[s] < nv)
	  nv = rbest.scorehist[s];
      }
      mv += 1000.0;
      nv = 0.0;
      if (mv - nv < 1.0) mv += 1.0;

      if (bw / rbest.nhist < xi)
	xi = bw / rbest.nhist;

      gc_color(gc,0xc0c0c0);
      for(s=1000;s<mv;s+=1000) {
	if (s >= nv) {
	  yv1 = (by+bh) - ( bh * ((s-nv) / (mv-nv)) );
	  gdk_draw_line(w->window,gc,bx,yv1,bx+bw,yv1);
	}
      }
      for(s=xi;s<bw;s+=xi)
	gdk_draw_line(w->window,gc,bx+s,by,bx+s,by+bh);

      gc_color(gc,0x0000cc);
      for(s=0;s<rbest.nhist-1;s++) {
	yv1 = (by+bh) - ( bh * ((rbest.scorehist[s]-nv) / (mv-nv)) );
	yv2 = (by+bh) - ( bh * ((rbest.scorehist[s+1]-nv) / (mv-nv)) );
	gdk_draw_line(w->window,gc,bx+s*xi,yv1,bx+(s+1)*xi,yv2);
	gdk_draw_rectangle(w->window,gc,TRUE,bx+(s+1)*xi-1,yv2-1,3,3);
      }
      gc_color(gc, 0);
      gdk_draw_rectangle(w->window,gc,FALSE,bx,by,bw,bh);

    }
      
  }

  // zoom overlay

  if (zs[0]) {
    int zw;

    gc_color(gc, 0);
    gdk_draw_rectangle(w->window,gc,TRUE,zs[1],zs[2]-4,200,10);

    zw = zs[3]-zs[1]; if (zw < 0) zw = 0; if (zw > 200) zw=200;
    gc_color(gc,0xdeff00);
    gdk_draw_rectangle(w->window,gc,TRUE,zs[1],zs[2]-4,zw,10);

    gc_color(gc, 0xffffff);
    gdk_draw_rectangle(w->window,gc,FALSE,zs[1],zs[2]-4,200,10);

    sprintf(msg,"Zoom: %d%%",(int)(100.0*zoom));
    center_label(zs[1],zs[2]-20,200,20,msg,0xdeff00,1);

  }

  // window overlay
  if (ys[0]) {
    int ww;

    gc_color(gc, 0);
    gdk_draw_rectangle(w->window,gc,TRUE,ys[1],ys[2]-4,200,10);

    ww = ys[3]-ys[1]; if (ww < 0) ww = 0; if (ww > 200) ww=200;
    gc_color(gc,0xdeff00);
    gdk_draw_rectangle(w->window,gc,TRUE,ys[1],ys[2]-4,ww,10);

    gc_color(gc, 0xffffff);
    gdk_draw_rectangle(w->window,gc,FALSE,ys[1],ys[2]-4,200,10);

    sprintf(msg,"Window: %d%% (L/R: %d/%d)",(int)(100.0*wfac),wmax,wmax2);
    center_label(ys[1],ys[2]-20,200,20,msg,0xdeff00,1);
  }

  lok.unlock();
  return TRUE;
}

gboolean gg_press(GtkWidget *w, GdkEventButton *e, gpointer data) {

  int x,y,b,ctrl;

  x = (int) e->x;
  y = (int) e->y;
  b = (int) e->button;
  ctrl = (e->state & GDK_CONTROL_MASK) ? 1 : 0;

  // cursor
  if (b==1) {
    
    if (inbox(x,y,panes[0].x,panes[0].y,panes[0].w,panes[0].h)) {      
      cursor[0] = (int) ((x - panes[0].x) / zoom);
      cursor[1] = (int) ((y - panes[0].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }
    if (inbox(x,y,panes[1].x,panes[1].y,panes[1].w,panes[1].h)) {      
      cursor[2] = (int) ((x - panes[1].x) / zoom);
      cursor[1] = (int) ((y - panes[1].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }
    if (inbox(x,y,panes[2].x,panes[2].y,panes[2].w,panes[2].h)) {      
      cursor[0] = (int) ((x - panes[2].x) / zoom);
      cursor[2] = (int) ((y - panes[2].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }

    if (inbox(x,y,panes[4].x,panes[4].y,panes[4].w,panes[4].h)) {      
      cursor[0] = (int) ((x - panes[4].x) / zoom);
      cursor[1] = (int) ((y - panes[4].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }
    if (inbox(x,y,panes[5].x,panes[5].y,panes[5].w,panes[5].h)) {      
      cursor[2] = (int) ((x - panes[5].x) / zoom);
      cursor[1] = (int) ((y - panes[5].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }
    if (inbox(x,y,panes[6].x,panes[6].y,panes[6].w,panes[6].h)) {      
      cursor[0] = (int) ((x - panes[6].x) / zoom);
      cursor[2] = (int) ((y - panes[6].y) / zoom);
      xs.invalidate(0,2);
      xs.invalidate(4,6);
      redraw(canvas);
      return TRUE;
    }

    if (panes[3].w && (inbox(x,y,panes[3].x,panes[3].y,panes[3].w,panes[3].h) || ks[0])) {
      if (!ks[0]) {
	ks[0] = x;
	ks[1] = y;
	return TRUE;
      } else {
	T3 rx,ry;
	ry.yrot(x-ks[0]);
	rx.xrot(y-ks[1]);
	rot *= rx;
	rot *= ry;
	ks[0] = x;
	ks[1] = y;
	xs.invalidate(8);
	redraw(canvas);
	return TRUE;
      }
    }

    if (!ks[2]) {
      ks[2] = x;
      ks[3] = y;
    } else {
      pan[0] += x - ks[2];
      pan[1] += y - ks[3];
      ks[2] = x;
      ks[3] = y;
      redraw(canvas);
      return(TRUE);
    }

  }

  // window
  if (b==2) {
    if (!ys[0]) {
      ys[0] = 1;
      ys[1] = x - (int) (200.0 * wfac);
      ys[2] = y;
      ys[3] = x;
    } else {
      wfac = (x-ys[1])/200.0;
      if (wfac < 0.01) wfac = 0.01;
      if (wfac > 1.0)  wfac = 1.0;
      ys[3] = x;
      wmax2 = (int) (nmax[2] * wfac);
      if (!ctrl) wmax = wmax2;
    }
    xs.invalidate(0,2);
    xs.invalidate(4,6);
    redraw(canvas);
    return TRUE;
  }

  // zoom
  if (b==3) {

    if (!zs[0]) {
      zs[0] = 1;
      zs[1] = x - (int) (200.0 * log(10.0*zoom) / log(160.0) );
      zs[2] = y;
      zs[3] = x;      
    } else {
      zoom = exp( (log(160.0)*(x-zs[1])/200.0) - log(10.0) );
      if (zoom < 0.10) zoom = 0.10;
      if (zoom > 16.0) zoom = 16.0;
      zs[3] = x;
    }
    redraw(canvas);
    return TRUE;

  }


  return TRUE;
}

gboolean gg_release(GtkWidget *w, GdkEventButton *e, gpointer data) {
  int b;

  b = (int) e->button;
  
  if (b==3 && zs[0]) { zs[0]=0; redraw(canvas); return TRUE; }
  if (b==2 && ys[0]) { ys[0]=0; redraw(canvas); return TRUE; }
  if (b==1 && ks[0]) { ks[0]=0; redraw(canvas); return TRUE; }
  if (b==1 && ks[2]) { ks[2]=0; redraw(canvas); return TRUE; }

  return TRUE;
}

gboolean gg_drag(GtkWidget *w, GdkEventMotion *e, gpointer data) {

  int x,y,b;
  GdkEventButton eb;

  x = (int) e->x;
  y = (int) e->y;
  b = 0;

  if (e->state & GDK_BUTTON3_MASK) b=3;
  if (e->state & GDK_BUTTON2_MASK) b=2;
  if (e->state & GDK_BUTTON1_MASK) b=1;

  if (b!=0) {
    eb.x = x;
    eb.y = y;
    eb.button = b;
    eb.state = e->state;
    gg_press(w,&eb,data);
  }

  return TRUE;
}


void status(const char *msg) {
  if (guimode) {
    gtk_statusbar_push(GTK_STATUSBAR(sbar), 0, msg);
    redraw(sbar);
  } else {
    printf(".. status: %s\n",msg);
  }
}

void gui() {
  GtkWidget *v, *h, *h2, *mbar, *l, *cmask, *t, *lb;
  GtkItemFactory *gif;
  GtkAccelGroup *mag;
  GtkTooltips *tips;
  int nitems = sizeof(gg_menu) / sizeof(gg_menu[0]);
  int i;

  mw = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(mw),"GREG");
  gtk_window_set_default_size(GTK_WINDOW(mw),960,768);
  gtk_widget_set_events(mw, GDK_EXPOSURE_MASK);
  gtk_widget_realize(mw);

  set_icon(mw, greg_xpm, "GREG");
  tips = gtk_tooltips_new();

  v = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(mw), v);

  // menu
  mag=gtk_accel_group_new();
  gif=gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);
  gtk_item_factory_create_items (gif, nitems, gg_menu, NULL);   
  gtk_window_add_accel_group(GTK_WINDOW(mw), mag);
  mbar = gtk_item_factory_get_widget (gif, "<main>");
  gtk_box_pack_start(GTK_BOX(v),mbar,FALSE,TRUE,0);

  // toolbars
  h = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(v),h,FALSE,TRUE,0);
  h2 = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(v),h2,FALSE,TRUE,0);
  
  // toolbuttons
  t = image_button(mw, greg_open1_xpm);
  gtk_tooltips_set_tip(tips,t,"Load Volume 1",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_load1),NULL);

  t = image_button(mw, greg_open2_xpm);
  gtk_tooltips_set_tip(tips,t,"Load Volume 2",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_load2),NULL);

  t = image_button(mw, greg_hnorm_xpm);
  gtk_tooltips_set_tip(tips,t,"Normalize Histograms",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_norm1),NULL);

  t = image_button(mw, greg_hmatch_xpm);
  gtk_tooltips_set_tip(tips,t,"Match Histograms",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_match1),NULL);

  t = image_button(mw, greg_reg_msgd_xpm);
  gtk_tooltips_set_tip(tips,t,"Registrate",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_reg_tool),NULL);

  regalgo = dropbox_new(";MSGD-WS-FF;CHS-WS-L1;CHS-WS-L1X;CHS-WS-L2C;CHS-WS-L2;MSGD-MI-FF;CHS-MI-L1;CHS-MI-L1X;CHS-MI-L2C;CHS-MI-L2");
  dropbox_set_selection(regalgo, 9);
  gtk_box_pack_start(GTK_BOX(h),regalgo->widget,FALSE,FALSE,2);

  t = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,4);

  rpb[0] = image_toggle(mw, greg_gray_xpm);
  rpb[1] = image_toggle(mw, greg_checkers_xpm);
  rpb[2] = image_toggle(mw, greg_rg_xpm);
  rpb[3] = image_toggle(mw, greg_yp_xpm);

  gtk_tooltips_set_tip(tips,rpb[0],"Volume 2: Original data",NULL);
  gtk_tooltips_set_tip(tips,rpb[1],"Volume 2: Checkers",NULL);
  gtk_tooltips_set_tip(tips,rpb[2],"Volume 2: Red/green overlay",NULL);
  gtk_tooltips_set_tip(tips,rpb[3],"Volume 2: Yellow/purple difference",NULL);

  for(i=0;i<4;i++) {
    gtk_box_pack_start(GTK_BOX(h),rpb[i],FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(rpb[i]),"toggled",GTK_SIGNAL_FUNC(gg_rpb),NULL);
  }

  rpmode = 0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rpb[rpmode]),TRUE);

  lb = gtk_label_new("Checker Size:");
  cksz = dropbox_new(",2,4,8,16,24,32,48,64,128");
  dropbox_set_selection(cksz,3);

  gtk_box_pack_start(GTK_BOX(h),lb,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),cksz->widget,FALSE,FALSE,0);

  t = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,4);

  t = image_button(mw, greg_3d_xpm);
  gtk_tooltips_set_tip(tips,t,"Compute 3D View",NULL);
  gtk_box_pack_start(GTK_BOX(h),t,FALSE,FALSE,4);
  gtk_signal_connect(GTK_OBJECT(t),"clicked",GTK_SIGNAL_FUNC(gg_comp3d),NULL);

  t3[0] = image_toggle(mw, greg_skin_xpm);
  t3[1] = image_toggle(mw, greg_brain_xpm);
  t3[2] = image_toggle(mw, greg_mask3d_xpm);
  
  gtk_tooltips_set_tip(tips,t3[0],"Toggle 3D Skin On/Off",NULL);
  gtk_tooltips_set_tip(tips,t3[1],"Toggle 3D Brain On/Off",NULL);
  gtk_tooltips_set_tip(tips,t3[2],"Toggle 3D Mask On/Off",NULL);
  
  for(i=0;i<3;i++) {
    gtk_box_pack_start(GTK_BOX(h),t3[i],FALSE,FALSE,0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t3[i]),TRUE);
    gtk_signal_connect(GTK_OBJECT(t3[i]),"toggled",GTK_SIGNAL_FUNC(gg_toggle3d),NULL);
  }

  // second hbar

  cmask = image_button(mw,greg_mask_xpm);
  gtk_tooltips_set_tip(tips,cmask,"Compute Mask",NULL);
  gtk_box_pack_start(GTK_BOX(h2),cmask,FALSE,FALSE,4);
  
  maskalgo = dropbox_new(";Difference Threshold;Local Homogeneity: Volume 1;Local Homogeneity: Volume 2;Tree Pruning: Difference;Tree Pruning: Volume 1;Tree Pruning: Volume 2");
  dropbox_set_selection(maskalgo, 0);
  gtk_box_pack_start(GTK_BOX(h2),maskalgo->widget,FALSE,FALSE,2);

  l = gtk_label_new("Mask Threshold:");
  gtk_box_pack_start(GTK_BOX(h2),l,FALSE,FALSE,4);
  
  mth = gtk_spin_button_new_with_range(0.0,3000.0,10.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mth), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(mth), 500);
  gtk_box_pack_start(GTK_BOX(h2),mth,FALSE,FALSE,4);
  
  l = gtk_label_new("Open Radius:");
  gtk_box_pack_start(GTK_BOX(h2),l,FALSE,FALSE,4);
  mor = gtk_spin_button_new_with_range(0.0,10.0,1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(mor), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(mor), 2.0);
  gtk_box_pack_start(GTK_BOX(h2),mor,FALSE,FALSE,4);

  l = gtk_label_new("Mask display:");
  gtk_box_pack_start(GTK_BOX(h2),l,FALSE,FALSE,4);

  mb[0] = image_toggle(mw, greg_gray_xpm);
  mb[1] = image_toggle(mw, greg_redmask_xpm);
  mb[2] = image_toggle(mw, greg_maskonly_xpm);

  gtk_tooltips_set_tip(tips,mb[0],"Hide mask",NULL);
  gtk_tooltips_set_tip(tips,mb[1],"Red mask overlay",NULL);
  gtk_tooltips_set_tip(tips,mb[2],"Mask only",NULL);

  for(i=0;i<3;i++) {
    gtk_box_pack_start(GTK_BOX(h2),mb[i],FALSE,FALSE,0);
    gtk_signal_connect(GTK_OBJECT(mb[i]),"toggled",GTK_SIGNAL_FUNC(gg_mb),NULL);
  }

  maskmode = 1;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb[maskmode]),TRUE);

  // canvas
  canvas = gtk_drawing_area_new();
  gtk_widget_set_events(canvas,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK|GDK_BUTTON_MOTION_MASK);
  gtk_box_pack_start(GTK_BOX(v), canvas, TRUE, TRUE, 0);

  // status

  sbar = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(v), sbar, FALSE, TRUE, 0);

  if (pl==NULL)
    pl = gtk_widget_create_pango_layout(canvas,NULL);
  if (sans10==NULL)
    sans10 = pango_font_description_from_string("Sans 10");
  if (sans10b==NULL)
    sans10b = pango_font_description_from_string("Sans Bold 10");
  if (gc==NULL)
    gc = gdk_gc_new(mw->window);

  dropbox_set_changed_callback(cksz, cksz_changed);

  gtk_signal_connect(GTK_OBJECT(cmask),"clicked",GTK_SIGNAL_FUNC(gg_cmask),NULL);
  gtk_signal_connect(GTK_OBJECT(mw),"delete_event",GTK_SIGNAL_FUNC(gg_del),NULL);
  gtk_signal_connect(GTK_OBJECT(mw),"destroy",GTK_SIGNAL_FUNC(gg_destroy),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"expose_event",GTK_SIGNAL_FUNC(gg_expose),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"button_press_event",GTK_SIGNAL_FUNC(gg_press),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"button_release_event",GTK_SIGNAL_FUNC(gg_release),NULL);
  gtk_signal_connect(GTK_OBJECT(canvas),"motion_notify_event",GTK_SIGNAL_FUNC(gg_drag),NULL);
  //gtk_signal_connect(GTK_OBJECT(mw),"key_press_event",GTK_SIGNAL_FUNC(gg_key),NULL);

  gtk_widget_show_all(mw);
}

void gg_cmds(vector<string> &cmds) {
  unsigned int i,n;

  n = cmds.size();
  for(i=0;i<n;i++) {

    if (cmds[i] == "log") {
      printf("cmd log\n");
      logging = 1;
      continue;
    }

    if (cmds[i] == "nolog") {
      printf("cmd nolog\n");
      logging = 0;
      continue;
    }

    if (cmds[i] == "cpus" && n-i > 1) {
      ncpus = atoi(cmds[i+1].c_str());
      if (ncpus < 1) ncpus = 1;
      if (ncpus > 32) ncpus = 32;
      printf("cmd cpus %d\n",ncpus);
      i++;
      continue;
    }

    if (cmds[i] == "orient" && n-i > 1) {
      cmd_orient(cmds[i+1].c_str());
      i++;
      continue;
    }
    
    if (cmds[i] == "msgdw") {
      printf("cmd register msgdw\n");
      gg_reg_msgd(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsw1") {
      printf("cmd register chsw1\n");
      gg_reg_ch1(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsw1x") {
      printf("cmd register chsw1x\n");
      gg_reg_ch1x(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsw2") {
      printf("cmd register chsw2\n");
      gg_reg_ch2(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsw2c") {
      printf("cmd register chsw2c\n");
      gg_reg_ch2c(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "msgdm") {
      printf("cmd register msgdm\n");
      gg_regm_msgd(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsm1") {
      printf("cmd register chsm1\n");
      gg_regm_ch1(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsm1x") {
      printf("cmd register chsm1x\n");
      gg_regm_ch1x(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsm2") {
      printf("cmd register chsm2\n");
      gg_regm_ch2(NULL,NULL);
      gg_wait();
      continue;
    }
    if (cmds[i] == "chsm2c") {
      printf("cmd register chsm2c\n");
      gg_regm_ch2c(NULL,NULL);
      gg_wait();
      continue;
    }
    // save file.scn
    if (cmds[i] == "save" && vol2!=NULL && n-i > 1) {
      printf("cmd save %s\n",cmds[i+1].c_str());
      if (vol2->writeSCN(cmds[i+1].c_str(),16,true) < 0)
	printf("** save failed.\n");
      i++;
      continue;
    }
    // savemat file.mat
    if (cmds[i] == "savemat" && n-1 > 1) {
      printf("cmd savemat %s\n",cmds[i+1].c_str());
      ofstream f;
      f.open(cmds[i+1].c_str());
      if (f.good()) {
	f << rbest.t;
	f.close();
      } else {
	printf("** error writing matrix to %s\n",cmds[i+1].c_str());
      }	
      i++;
      continue;
    }
    // loadmat file.mat
    if (cmds[i] == "loadmat" && n-1 > 1) {
      printf("cmd loadmat %s\n",cmds[i+1].c_str());
      ifstream f;
      f.open(cmds[i+1].c_str());
      if (f.good()) {
	f >> rbest.t;
	f.close();
      } else {
	printf("** error reading matrix from %s\n",cmds[i+1].c_str());
      }
      i++;
      continue;
    }
    // regtoo src.scn dest.scn
    if (cmds[i] == "regtoo" && n-1 > 2) {
      printf("cmd regtoo %s => %s\n",cmds[i+1].c_str(),cmds[i+2].c_str());
      cmd_regtoo(cmds[i+1].c_str(), cmds[i+2].c_str());
      i+=2;
      continue;
    }
    if (cmds[i] == "quit") {
      printf("cmd quit\n");
      exit(0);
    }
    printf("** unrecognized cmd [%s] -- skipping\n",cmds[i].c_str());
  }

}

int main(int argc, char **argv) {
  pthread_t tid;
  int i;
  vector<string> cmds;

  rbest.nhist = 0;
  rot.identity();

  for(i=3;i<argc;i++)
    if (!strcmp(argv[i],"nogui"))
      guimode = 0;

  if (guimode) {
    gtk_init(&argc,&argv);
    gdk_rgb_init();
  }

  bgtask.algo = -1;
  pthread_create(&tid, NULL, bgtask_main, NULL);

  if (guimode)
    gui();

  if (argc>=3) {
    gg_init(argv[1],argv[2]);
  }

  for(i=3;i<argc;i++) {
    cmds.push_back(string(argv[i]));
  }

  cpu_count();

  if (!cmds.empty())
    gg_cmds(cmds);

  if (guimode)
    gtk_main();
  return 0;
}

DropBox *rimg=NULL, *rs=NULL, *rd=NULL;

void rot_destroy(GtkWidget *w, gpointer data) {
  dropbox_destroy(rimg);
  dropbox_destroy(rs);
  dropbox_destroy(rd);
  rimg = rs = rd = NULL;
  gtk_grab_remove(w);
}

void rot_fix_orientation(int v, int or1, int or2) {
  // 0: sag, LR (base)
  // 1: sag, RL
  // 2: cor, AP
  // 3: cor, PA
  // 4: tra, BT/FH
  // 5: tra, TB/HF
  // forward = whatever to base
  // reverse = base to whatever

  Volume<int> *dv, *tv;
  Volume<int>::Orientation o1 = Volume<int>::sagittal_lr, o2 = Volume<int>::sagittal_lr;

  if (vol1==NULL || vol2==NULL) {
    fprintf(stderr,"rot_fix_orientation(%d,%d,%d): volume missing.\n",v,or1,or2);
    return;
  }
  if (or1 < 0 || or1 > 5 || or2 < 0 || or2 > 5 || v < 1 || v > 2) {
    fprintf(stderr,"rot_fix_orientation(%d,%d,%d): bad parameters.\n",v,or1,or2);
    return;
  }

  switch(v) {
  case 1: tv = vol1; break;
  case 2: tv = vol2; break;
  default: return;
  }

  switch(or1) {
  case 0: o1 = Volume<int>::sagittal_lr; break;
  case 1: o1 = Volume<int>::sagittal_rl; break;
  case 2: o1 = Volume<int>::coronal_ap; break;
  case 3: o1 = Volume<int>::coronal_pa; break;
  case 4: o1 = Volume<int>::axial_fh; break;
  case 5: o1 = Volume<int>::axial_hf; break;
  }

  switch(or2) {
  case 0: o2 = Volume<int>::sagittal_lr; break;
  case 1: o2 = Volume<int>::sagittal_rl; break;
  case 2: o2 = Volume<int>::coronal_ap; break;
  case 3: o2 = Volume<int>::coronal_pa; break;
  case 4: o2 = Volume<int>::axial_fh; break;
  case 5: o2 = Volume<int>::axial_hf; break;
  }

  dv = tv->changeOrientation(o1,o2);

  if (v==1) {
    delete vol1;
    vol1 = dv;
  } else {
    delete vol2;
    vol2 = dv;
  }
}

void rot_ok(GtkWidget *ww, gpointer data) {
  int v,or1,or2;

  if (dropbox_get_selection(rimg) == 0)
    v = 1;
  else
    v = 2;

  or1 = dropbox_get_selection(rs);
  or2 = dropbox_get_selection(rd);

  rot_fix_orientation(v,or1,or2);

  xs.invalidate();
  redraw(mw);
  gtk_widget_destroy( (GtkWidget *) data );
}

void rot_cancel(GtkWidget *w, gpointer data) {
  gtk_widget_destroy( (GtkWidget *) data );
}

void gg_rot(GtkWidget *w, gpointer data) {
  GtkWidget *ad,*vb,*hb,*l[3],*h[3],*ok,*cancel;

  if (vol1 == NULL || vol2 == NULL) return;

  ad = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ad),"Change Orientation");
  gtk_window_set_position(GTK_WINDOW(ad),GTK_WIN_POS_CENTER);
  gtk_window_set_transient_for(GTK_WINDOW(ad),GTK_WINDOW(mw));
  gtk_container_set_border_width(GTK_CONTAINER(ad),6);
  gtk_widget_realize(ad);
  set_icon(GTK_WIDGET(ad), greg_xpm, "GREG");

  l[0] = gtk_label_new("Image to modify:");
  l[1] = gtk_label_new("Current orientation:");
  l[2] = gtk_label_new("New orientation:");

  rimg = dropbox_new(",Image 1,Image 2");
  dropbox_set_selection(rimg, 1);
  rs =   dropbox_new(",Sagittal (left to right),Sagittal (right to left),Coronal (anterior to posterior),Coronal (posterior to anterior),Transversal (bottom to top),Transversal (top to bottom)");
  rd =   dropbox_new(",Sagittal (left to right),Sagittal (right to left),Coronal (anterior to posterior),Coronal (posterior to anterior),Transversal (bottom to top),Transversal (top to bottom)");

  vb = gtk_vbox_new(FALSE,4);
  h[0] = gtk_hbox_new(FALSE,4);
  h[1] = gtk_hbox_new(FALSE,4);
  h[2] = gtk_hbox_new(FALSE,4);

  gtk_container_add(GTK_CONTAINER(ad), vb);
  gtk_box_pack_start(GTK_BOX(vb), h[0], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vb), h[1], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vb), h[2], FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(h[0]), l[0], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h[0]), rimg->widget, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h[1]), l[1], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h[1]), rs->widget, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h[2]), l[2], FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h[2]), rd->widget, FALSE, FALSE, 0);

  hb = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  ok = gtk_button_new_with_label("Ok");
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  cancel = gtk_button_new_with_label("Cancel");

  gtk_box_pack_start(GTK_BOX(vb),hb,FALSE,TRUE,10);
  gtk_box_pack_start(GTK_BOX(hb),ok,FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(hb),cancel,FALSE,FALSE,2);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ad),"delete_event",
                     GTK_SIGNAL_FUNC(gg_del),NULL);
  gtk_signal_connect(GTK_OBJECT(ad),"destroy",
                     GTK_SIGNAL_FUNC(rot_destroy),NULL);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
                     GTK_SIGNAL_FUNC(rot_ok),(gpointer)ad);
  gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
                     GTK_SIGNAL_FUNC(rot_cancel),(gpointer)ad);
  
  gtk_widget_show_all(ad);
  gtk_grab_add(ad);
}

void about_destroy(GtkWidget *w, gpointer data) {
  gtk_grab_remove(w);
}

void about_ok(GtkWidget *w, gpointer data) {
  gtk_widget_destroy( (GtkWidget *) data );
}

void gg_about(GtkWidget *w, gpointer data) {
  GtkWidget *ad,*vb,*hb,*hb2,*l,*ok,*i;
  char z[1024];
  GdkPixmap *icon;
  GdkBitmap *mask;
  GtkStyle *style;

  style=gtk_widget_get_style(mw);
  icon = gdk_pixmap_create_from_xpm_d (mw->window, &mask,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) greg_xpm);

  ad = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ad),"About GREG");
  gtk_window_set_position(GTK_WINDOW(ad),GTK_WIN_POS_CENTER);
  gtk_window_set_transient_for(GTK_WINDOW(ad),GTK_WINDOW(mw));
  gtk_container_set_border_width(GTK_CONTAINER(ad),6);
  gtk_widget_realize(ad);

  i = gtk_image_new_from_pixmap(icon, mask);

  vb = gtk_vbox_new(FALSE,4);
  hb = gtk_hbox_new(FALSE,0);
  snprintf(z,1024,"GREG version %s\n(C) 2009-2012 Felipe Bergo\n<fbergo\x40gmail.com>\n\nProgram settings:\n\tUse up to %d CPUs\n\tLogging %s\n\nThis is free software and comes with no warranty.",
	   VERSION,ncpus,logging?"enabled":"disabled");

  l = gtk_label_new(z);
  hb2 = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb2), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb2), 5);
  ok = gtk_button_new_with_label("Close");
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);

  gtk_container_add(GTK_CONTAINER(ad), vb);
  gtk_box_pack_start(GTK_BOX(vb),hb,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),i,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(hb),l,FALSE,FALSE,8);
  gtk_box_pack_start(GTK_BOX(vb),hb2,FALSE,TRUE,10);
  gtk_box_pack_start(GTK_BOX(hb2),ok,FALSE,FALSE,2);
  gtk_widget_grab_default(ok);

  gtk_signal_connect(GTK_OBJECT(ad),"delete_event",
		     GTK_SIGNAL_FUNC(gg_del),NULL);
  gtk_signal_connect(GTK_OBJECT(ad),"destroy",
		     GTK_SIGNAL_FUNC(about_destroy),NULL);
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(about_ok),(gpointer)ad);
  
  gtk_widget_show_all(ad);
  gtk_grab_add(ad);
}

int lut(int value, int side) {
  int emax;
  emax = (side == 0) ? wmax : wmax2;
  if (value<0) value=-value;
  if (value > emax) return 255;
  return( (int) ((255.0 * value) / emax) );
}

void left_label(int x,int y,int w,int h,const char *label, int color, int shadow) {
  int tw,th;

  pango_layout_set_text(pl, label, -1);
  pango_layout_get_pixel_size(pl,&tw,&th);
  if (shadow) {
    gc_color(gc,0);
    gdk_draw_layout(canvas->window, gc, x+1, y+1+(h-th)/2,pl);
  }
  gc_color(gc, color);
  gdk_draw_layout(canvas->window, gc, x, y+(h-th)/2,pl);
}

void center_label(int x,int y,int w,int h,const char *label, int color, int shadow) {
  int tw,th;
  pango_layout_set_text(pl, label, -1);
  pango_layout_get_pixel_size(pl,&tw,&th);
  if (shadow) {
    gc_color(gc, 0);
    gdk_draw_layout(canvas->window, gc, x+1+(w-tw)/2, y+1+(h-th)/2,pl);
  }
  gc_color(gc, color);
  gdk_draw_layout(canvas->window, gc, x+(w-tw)/2, y+(h-th)/2,pl);
}

// ===========================================

int envint(const char *var) {
  char *s;
  s = getenv(var);
  if (s==NULL) return 0; else return(atoi(s));
}

double envdouble(const char *var) {
  char *s;
  s = getenv(var);
  if (s==NULL) return 0.0; else return(atof(s));
}

void *bgtask_main(void *data) {
  Volume<int> * r;

  for(;;) {
    if (bgtask.algo < 0) {
      usleep(50000);
      continue;
    }
    
    switch(bgtask.algo) {
    case 1: // msgd-ws-ff

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"MSGD-WS-FF Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	busythreads++;
	lok.unlock();	
	r = msgd_register(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	busythreads--;
	lok.unlock();
      } else
	bgtask.algo = -1;      
      break;

    case 2: // mask computation
      if (vol1!=NULL && vol2!=NULL && mask!=NULL) {
	lok.lock();
	busythreads++;
	strcpy(bgtask.major,"Mask Computation");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();
	mvol = comp_mask(vol1,vol2,mask,bgtask.ipar[5],
			 bgtask.ipar[2],bgtask.ipar[3],bgtask.ipar[4],
			 bgtask.ipar[0],bgtask.ipar[1]);
	mask->invalidateRenderCache();
	lok.lock();
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	busythreads--;
	lok.unlock();
      } else
	bgtask.algo = -1;
      break;

    case 3: // skin+brain computation
      if (vol1!=NULL) {
	lok.lock();
	busythreads++;
	renderok = 0;
	strcpy(bgtask.major,"3D Surface Computation");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	if (skin!=NULL) { delete skin; skin=NULL; }
	if (brain!=NULL) { delete brain; brain=NULL; }
	lok.unlock();
	comp_surfaces();
	xs.invalidate();
	busythreads--;
	bgtask.algo = -1;
      } else
	bgtask.algo = -1;
      break;
    case 4: //CHS-WS-L1

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-WS-L1 Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_register1(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 5: //CHS-WS-L2
      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-WS-L2 Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_register2(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 6: //CHS-WS-L1X

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-WS-L1X Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_register1x(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 7: //CHS-WS-L2C
      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-WS-L2C Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_register2c(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;

    case 10: // msgd-mi-ff

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	busythreads++;
	strcpy(bgtask.major,"MSGD-MI-FF Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = msgd_miregister(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	busythreads--;
	lok.unlock();
      } else
	bgtask.algo = -1;      
      break;

    case 11: //CHS-MI-L1

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-MI-L1 Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_miregister1(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 12: //CHS-MI-L2
      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-MI-L2 Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_miregister2(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 13: //CHS-MI-L1X

      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-MI-L1X Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_miregister1x(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;
    case 14: //CHS-MI-L2C
      if (vol1!=NULL && vol2!=NULL) {
	lok.lock();
	strcpy(bgtask.major,"CHS-MI-L2C Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();	
	r = chs_miregister2c(vol1, vol2);
	print_reg_summary();
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;      

      break;

    case 15: // mat-load
      if (vol1!=NULL && vol2!=NULL) {
	T4 mat;
	lok.lock();
	strcpy(bgtask.major,"Matrix Registration");
	strcpy(bgtask.minor,"Initializing");
	bgtask.step = 0;
	bgtask.nsteps = 0;
	bgtask.start = Timestamp::now();
	lok.unlock();
	mat = rbest.t;
	r = mat_register(vol1, vol2, mat);
	lok.lock();
	delete vol2;
	vol2 = r;
	nmax_comp();
	xs.invalidate();
	bgtask.algo = -1;
	lok.unlock();
      } else
	bgtask.algo = -1;
      break;
    }

    if (bgtask.algo < 0 && rbest.nhist > 0)
      rbest.nhist = 0;

  }

  return NULL;
}

void gg_norm1(GtkWidget *w, gpointer data) {

  if (vol1!=NULL) acc_hist_norm(vol1);
  if (vol2!=NULL) acc_hist_norm(vol2);
  status("Histogram Normalization complete.");

  nmax_comp();
  xs.invalidate(0,7);
  xs.invalidate(9);
  redraw(mw);
}

void gg_close(GtkWidget *w, gpointer data) {
  if (vol1!=NULL) {
    delete vol1;
    vol1 = NULL;
  }
  if (vol2!=NULL) {
    delete vol2;
    vol2 = NULL;
  }
  if (mask!=NULL) {
    delete mask;
    mask = NULL;
    mvol = 0.0;
  }
  if (skin!=NULL) { delete skin; skin=NULL; renderok = 0; }
  if (brain!=NULL) { delete brain; brain=NULL; renderok = 0; }
  status("Volumes closed.");
  xs.invalidate(8);
  redraw(mw);
}

void gg_match1(GtkWidget *w, gpointer data) {
  if (vol1==NULL || vol2==NULL) return;

  vol2->histogramMatch(vol1);
  nmax_comp();

  status("Histogram match complete.");
  xs.invalidate(0,7);
  xs.invalidate(9);
  redraw(mw);
}

void nmax_comp() {
  if (vol1!=NULL) nmax[0] = vol1->maximum();
  if (vol2!=NULL) nmax[1] = vol2->maximum();
  nmax[2] = (int) (1.05 * Max(nmax[0], nmax[1]));
  wmax = wmax2 = (int) (nmax[2] * wfac);
}

void gg_rpb(GtkToggleButton *b, gpointer data) {
  int i,s;
  int om;
  static int ignore = 0;

  if (ignore > 0) {
    --ignore;
    return;
  }
  
  om = rpmode;
  for(i=0;i<4;i++) 
    if (GTK_TOGGLE_BUTTON(rpb[i])==b)
      rpmode = i;

  for(i=0;i<4;i++) {
    s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rpb[i]));
    if (i==rpmode && !s) {
      ignore++;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rpb[i]),TRUE);
    }
    if (i!=rpmode && s) {
      ignore++;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rpb[i]),FALSE);
    }
  }

  if (rpmode != om) {
    xs.invalidate(4,6);
    redraw(mw);
  }

}

void gg_mb(GtkToggleButton *b, gpointer data) {

  int i,s;
  int om;
  static int ignore = 0;

  if (ignore > 0) {
    --ignore;
    return;
  }
  
  om = maskmode;
  for(i=0;i<3;i++) 
    if (GTK_TOGGLE_BUTTON(mb[i])==b)
      maskmode = i;

  for(i=0;i<3;i++) {
    s = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mb[i]));
    if (i==maskmode && !s) {
      ignore++;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb[i]),TRUE);
    }
    if (i!=maskmode && s) {
      ignore++;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb[i]),FALSE);
    }
  }

  if (maskmode != om) {
    xs.invalidate(0,7);
    xs.invalidate(9);
    redraw(mw);
  }
}

void gg_toggle(GtkWidget *w) {
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
}

void gg_comp3d(GtkWidget *w, gpointer data) {
  
  if (bgtask.algo > 0) {
    status("Program busy.");
    return;
  }

  if (renderok && brain!=NULL && skin!=NULL) {
    status("3D surfaces already computed.");
    return;
  }

  if (vol1==NULL) {
    status("Volume 1 not loaded.");
  }

  bgtask.algo = 3;
  gg_wait();
  xs.invalidate(8);
  redraw(mw);
  status("Surface Computation finished.");
}

void comp_surfaces() {
  Volume<int> *ve, *vg;
  Volume<char> *vm, *tsb;
  int i;

  // brain
  lok.lock();
  bgtask.nsteps = 9;
  bgtask.step = 1;
  strcpy(bgtask.minor,"Brain: computing markers");
  lok.unlock();

  vm = vol1->brainMarkerComp();

  lok.lock();
  bgtask.step = 2;
  strcpy(bgtask.minor,"Brain: intensity enhancement");
  lok.unlock();

  ve = vol1->otsuEnhance();

  lok.lock();
  bgtask.step = 3;
  strcpy(bgtask.minor,"Brain: computing gradient");
  lok.unlock();

  vg = ve->featureGradient();
  delete ve;

  lok.lock();
  bgtask.step = 4;
  strcpy(bgtask.minor,"Brain: tree pruning");
  lok.unlock();

  brain = vg->treePruning(vm);
  delete vm;
  delete vg;

  lok.lock();
  bgtask.step = 5;
  strcpy(bgtask.minor,"Brain: selecting surface");
  lok.unlock();

  brain->incrementBorder();

  lok.lock();
  bgtask.step = 6;
  strcpy(bgtask.minor,"Skin: threshold");
  lok.unlock();

  tsb = vol1->binaryThreshold(400);

  lok.lock();
  bgtask.step = 7;
  strcpy(bgtask.minor,"Skin: excluding brain");
  lok.unlock();

  for(i=0;i<tsb->N;i++)
    if (brain->voxel(i)) tsb->voxel(i) = 0;

  lok.lock();
  bgtask.step = 8;
  strcpy(bgtask.minor,"Skin: morphological close");
  lok.unlock();

  skin = tsb->binaryClose(5.0 / vol1->dx);
  delete tsb;

  lok.lock();
  bgtask.step = 9;
  strcpy(bgtask.minor,"Skin: selecting surface");
  lok.unlock();

  skin->incrementBorder();
  renderok = 1;
}

void print_reg_summary() {
  if (logging) {
    printf("registration summary: file1=%s file2=%s algo=%s score=%.2f iterations=%d time=%.2f\n",
	   file1,file2, rbest.algo, rbest.score, rbest.iterations, rbest.elapsed);
  }
}

void cpu_count() {
  FILE *f;
  char s[256];
  int x = 0;
  f = fopen("/proc/cpuinfo","r");
  if (f == NULL) { ncpus = 1; return; }
  
  while(fgets(s,255,f)!=NULL)
    if (!strncmp(s,"processor",9))
      x++;

  fclose(f);
  if (x < 1) x = 1;
  if (x > 32) x = 32;
  ncpus = x;
}

