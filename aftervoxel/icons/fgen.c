
#include <stdio.h>
#include <gtk/gtk.h>
#include <libip.h>

char x[257];
int cw[256],ch[256];

#define TAM "large"

void util_set_bg(GtkWidget *w, int bg);
char * str2utf8(const char *src);

gboolean expose(GtkWidget *w, GdkEventExpose *ee,gpointer data) {
  int i,j,W,H;
  PangoLayout *pl;
  char msg[32];
  GdkPixbuf *pb;
  GdkPixmap *pm;
  FILE *f;
  guchar *p;
  int n;
  int pw,ph;

  W = 16;
  H = 20;

  pl = gtk_widget_create_pango_layout(w," ");

  pm = gdk_pixmap_new(w->window,W*256,H,-1);
  gdk_draw_rectangle(pm,w->style->white_gc,TRUE,0,0,W*256,H);

  for(i=0;i<256;i++) {
    switch(i) {
    case 0:
      sprintf(msg,"<span size='%s'> </span>",TAM);
      break;
    case '<':
      sprintf(msg,"<span size='%s'>&lt;</span>",TAM);
      break;
    case '>':
      sprintf(msg,"<span size='%s'>&gt;</span>",TAM);
      break;
    case '&':
      sprintf(msg,"<span size='%s'>&amp;</span>",TAM);
      break;
    default:
      sprintf(msg,"<span size='%s'>%c</span>",TAM,x[i]);
      break;
    }
    gdk_draw_rectangle(pm,w->style->white_gc,TRUE,W*i,0,W,H);
    pango_layout_set_markup(pl, str2utf8(msg), -1);
    gdk_draw_layout(pm,w->style->black_gc,W*i,0,pl);
    pango_layout_get_pixel_size(pl,&pw,&ph);
    cw[i] = pw;
    ch[i] = ph;
  }

  pb = gdk_pixbuf_get_from_drawable(NULL,pm,
				    gtk_widget_get_colormap(w),
				    0,0,0,0,W*256,H);

  if (pb)
    gdk_pixbuf_save(pb,"fgen.png","png",NULL,0);

  f = fopen("font.af","w");
  fprintf(f,"%d %d\n",W,H);
  for(i=0;i<256;i++)
    fprintf(f,"%d%c",cw[i],i==255?'\n':' ');
  for(i=0;i<256;i++)
    fprintf(f,"%d%c",ch[i],i==255?'\n':' ');
  n = gdk_pixbuf_get_n_channels(pb);
  p = gdk_pixbuf_get_pixels(pb);

  for(i=0;i<W*H*256;i++) {
    fputc(*p,f);
    p+=n;
  }
  fclose(f);

  gtk_main_quit();
  return FALSE;
}

int main(int argc, char **argv) {
  int i;
  GtkWidget *w,*c;

  x[256]=0;
  for(i=0;i<256;i++)
    x[i] = i;

  gtk_init(&argc,&argv);
  
  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  c = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(c),256,256);
  gtk_container_add(GTK_CONTAINER(w),c);
  util_set_bg(c,0xffffff);
  gtk_widget_show_all(w);
  gtk_signal_connect(GTK_OBJECT(c),"expose_event",
		     GTK_SIGNAL_FUNC(expose),0);
  gtk_main();
  return 0;
}


void util_set_bg(GtkWidget *w, int bg) {
  GdkColor c;
  char z[16];
  sprintf(z,"#%.2x%.2x%.2x",bg>>16,0xff&(bg>>8),bg&0xff);
  gdk_color_parse(z,&c);
  gtk_widget_modify_bg(w,GTK_STATE_NORMAL,&c);
}

char * str2utf8(const char *src) {
  static char *t = 0;
  int tsize = 0;

  gchar *r;

  r = g_convert(src,strlen(src),"UTF-8","ISO-8859-1",0,0,0);

  if (t && tsize <= strlen(r)) {
    free(t);
    t = 0;
  }

  if (!t) {
    tsize = strlen(r) + 1;
    t = (char *) malloc(tsize);
    if (t!=0)
      strcpy(t,r);
  }
  g_free(r);
  return t;
}
