
#ifndef IVS_FILEMGR_H
#define IVS_FILEMGR_H 1

#include <pthread.h>
#include <semaphore.h>
#include <gtk/gtk.h>
#include <libip.h>
#include "util.h"

typedef struct _clickable {
  int n,x,y;
  int action;
  int icon;
  int ignore;
  char caption[3][64];
  CImg *thumb;
  struct _clickable * next;
} Clickable;

struct FileMgr {
  
  GtkWidget *itself;
  GtkWidget *canvas;
  Graphics  *g;

  PangoLayout *pl;
  PangoFontDescription *pfd;

  char   curdir[512];
  time_t dir_mtime;
  
  GdkPixmap *icons[14];
  GdkBitmap *masks[14];
  int        icw[14], ich[14];

  Clickable *head, *tail;
  Clickable *chead, *ctail;
  int        errcode;

  int iw,ih;

  GtkWidget     *vscroll;
  GtkAdjustment *vadj;
  int            vadj_ignore;
  int            vadj_vignore;

  /* control for background thread that checks
     file contents */
  int              sync_req;
  pthread_t        ft_tid;
  sem_t            scan_needed;
  pthread_mutex_t  list_mutex; /* head->...->tail access */

};

#define T_FOLDER   0
#define T_UP       1
#define T_LINK     2
#define T_BLANK    3
#define T_VOL_SCN  4
#define T_VOL_VLA  5
#define T_VOL_MINC 6
#define T_VOL_HDR  7
#define T_TEXT     8
#define T_SFOLDER  9
#define T_VOL_PGM  10
#define T_VOL_P6   11
#define T_VOL_MVF  12
#define ICO_NOTHUMB 13

void create_filemgr_gui();

Clickable * MakeClickable(int n,int action, int icon,char *caption);
void        DestroyClickables(Clickable *c);
void        DestroyClickable(Clickable *c);

#endif
