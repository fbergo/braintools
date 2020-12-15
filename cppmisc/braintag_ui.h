
#ifndef BRAINTAG_UI_H
#define BRAINTAG_UI_H 1

#include <gtk/gtk.h>
#include "brain_imaging.h"
#include <list>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
using namespace std;

gboolean window_timeout(gpointer data);

class UI {
public:
  Color  BG, Border, Title[4];
  string TitleFont, HelpFont;

  UI() {
    BG            = 0x706860;
    Border        = 0xe0e0a0;
    Title[0]      = 0x000000;
    Title[1]      = 0x909090;
    Title[2]      = 0xffffff;
    Title[3]      = 0xffffff;
    TitleFont     = "Sans Bold 10";
    HelpFont      = "Sans 8";
  }
} ui;

class Widget {
public:
  Widget() { widget = defwidget; gc = defgc; pl = defpl; pfd = defpfd; hfd = defhfd; }  
 
  static GtkWidget *defwidget;
  static GdkGC *defgc;
  static PangoLayout *defpl;
  static PangoFontDescription *defpfd;
  static PangoFontDescription *defhfd;

  void setWidget(GtkWidget *gw, GdkGC *ggc, PangoLayout *gpl) {
    widget = gw;
    gc     = gc;
    if (gpl!=NULL) 
      pl = gpl; 
    else 
      pl = gtk_widget_create_pango_layout(widget,NULL);
    pfd = pango_font_description_from_string(ui.TitleFont.c_str());
    hfd = pango_font_description_from_string(ui.HelpFont.c_str());
  }

  GtkWidget            * getWidget() { return widget; }
  GdkWindow            * getWindow() { if (widget!=NULL) return widget->window; else return NULL; }
  GdkGC                * getGC() { return gc; }
  PangoLayout          * getLayout() { return pl; }
  PangoFontDescription * getTitlePFD() { return pfd; }
  PangoFontDescription * getHelpPFD() { return hfd; }

  void drawString(int x,int y,Color color,const char *text, PangoFontDescription *fd=NULL) {
    ensureGC();
    if (fd==NULL) fd=getTitlePFD();
    pango_layout_set_font_description(pl, fd);
    pango_layout_set_text(pl, text, -1);
    gdk_rgb_gc_set_foreground(gc, color.toInt());
    gdk_draw_layout(widget->window,gc,x,y,pl);
  }

  void rect(int x,int y,int w,int h,Color c,bool fill) {
    ensureGC();
    gdk_rgb_gc_set_foreground(gc, c.toInt());
    gdk_draw_rectangle(widget->window,gc,fill?TRUE:FALSE,x,y,w,h);
  }

  void line(int x1,int y1,int x2,int y2,Color c) {
    gdk_rgb_gc_set_foreground(gc, c.toInt());
    gdk_draw_line(widget->window,gc,x1,y1,x2,y2);
  }

  void drawImage(Image *img, int x,int y) {
    ensureGC();
    gdk_draw_rgb_image(widget->window,gc,x,y,img->W,img->H,
		       GDK_RGB_DITHER_NORMAL, img->getBuffer(), 3*(img->W));
  }

  void setClip(GdkRegion *reg) {
    ensureGC();
    gdk_gc_set_clip_region(gc,reg);
  }

  void setClip(GdkRectangle *rec) {
    ensureGC();
    gdk_gc_set_clip_rectangle(gc,rec);
  }

  void getWindowSize(int &w,int &h) {
    if (widget->window!=NULL)
      gdk_window_get_size(widget->window,&w,&h);
    else w=h=0;
  }

  int stringWidth(const char *text, PangoFontDescription *fd=NULL) {
    PangoRectangle prl,pri;
    if (fd==NULL) fd=getTitlePFD();
    pango_layout_set_font_description(pl, fd);
    pango_layout_set_text(pl, text, -1);
    pango_layout_get_pixel_extents(pl,&prl,&pri);
    return(prl.width);
  }

  int stringHeight(const char *text, PangoFontDescription *fd=NULL) {
    PangoRectangle prl,pri;
    if (fd==NULL) fd=getTitlePFD();
    pango_layout_set_font_description(pl, fd);
    pango_layout_set_text(pl, text, -1);
    pango_layout_get_pixel_extents(pl,&prl,&pri);
    return(prl.height);
  }

  void drawSlider(int x,int y,int w,int h,float val, int sliderw=-1, bool label=true) {
    int sx;
    char z[64];

    sx = (int) (val*w);

    rect(x,y+h/2-2,w,4,Color(0xffffff),true);
    rect(x,y+h/2-2,w,4,ui.Border,false);

    if (sliderw < 0) {
      rect(x,y+2,sx,h-4,Color(0),true);
      rect(x,y+2,sx,h-4,ui.Border,false);
    } else {
      rect(x+sx-sliderw/2,y+2,sliderw,h-4,Color(0),true);
      rect(x+sx-sliderw/2,y+2,sliderw,h-4,ui.Border,false);
    }

    if (label) {
      snprintf(z,64,"%d %%",(int)(val*100.0));
      drawString(x+w+10,y-2,Color(0xffff80),z);
    }
  }

  void drawCheckBox(int x,int y,bool active) {
    rect(x,y+4,10,10,Color(0xffffff),true);
    rect(x,y+4,10,10,ui.Border,false);
    if (active) {
      line(x+2-2,y+8,x+6-2,y+12,0);
      line(x+3-2,y+8,x+7-2,y+12,0);
      line(x+6-2,y+12,x+14-2,y+4,0);
      line(x+7-2,y+12,x+15-2,y+4,0);
    }
  }

  void repaint() {    
    gtk_widget_queue_resize(widget);
  }

private:
  GtkWidget *widget;
  GdkGC     *gc;
  PangoLayout *pl;
  PangoFontDescription *pfd, *hfd;

  void ensureGC() {
    if (gc==NULL && widget->window != NULL)
      gc = gdk_gc_new(widget->window);
  }
};

GtkWidget * Widget::defwidget;
GdkGC * Widget::defgc;
PangoLayout * Widget::defpl;
PangoFontDescription * Widget::defpfd;
PangoFontDescription * Widget::defhfd;

class Window : public Widget {
public:
  int X,Y,W,H;
  char *Title;
  bool resizable;

  Window(const char *title, int x,int y,int w,int h) : Widget() {
    Title = strdup(title);
    X=x; Y=y;
    W=w; H=h;
    tw = 0;
    ma = 0;
    th = 24;
    grab=false;
    minw = 64;
    minh = 20;
    resizable = true;
  }

  virtual ~Window() {
    free(Title);
  }

  virtual vector<string> composeHelp() {
    vector<string> help;
    help.push_back(string("Default help message."));
    return help;
  }

  vector<string> keyHelp() {
    vector<string> help;
    string s,sep;
    sep = "    "; 
    s = "F1-F2: Tool"; s+=sep;
    s += "F5-F7: Label";
    s += sep;
    s += alienKey(0x2190);
    s += alienKey(0x2191);
    s += alienKey(0x2192);
    s += alienKey(0x2193);
    s += ": Depth";
    s += sep;
    s += alienKey(0x2212);
    s += "/+: Range";
    help.push_back(s);
    return help;
  }

  string alienKey(int code) {
    char tmp[16];
    int n;
    string s;
    n = g_unichar_to_utf8(code,tmp);
    tmp[n] = 0;
    s = tmp;
    return s;
  }

  int saveGeometry(const char *tag, const char *appfile) {
    ifstream f;
    vector<string> cfg;
    char tmp[1024], *p;

    f.open(appfile);
    if (f.good()) {

      while(!f.eof()) {
	f.getline(tmp,1024);
	Tokenizer t(tmp,", \t\n\r");
	p = t.nextToken();
	if (p==NULL) continue;
	if (strcmp(p,tag)!=0) 
	  cfg.push_back(string(tmp));
      }

      f.close();
    }

    sprintf(tmp,"%s,%d,%d,%d,%d",tag,X,Y,W,H);
    cfg.push_back(string(tmp));

    ofstream g;
    vector<string>::iterator i;
    g.open(appfile);
    if (!g.good()) return -1;
    for(i=cfg.begin();i!=cfg.end();i++)
      g << (*i) << endl;
    g.close();
    return 0;
  }

  void loadGeometry(const char *tag, const char *appfile) {
    ifstream f;
    char tmp[1024], *p;

    f.open(appfile);
    if (!f.good()) return;

    while(!f.eof()) {
      f.getline(tmp,1024);
      Tokenizer t(tmp,", \t\n\r");
      p = t.nextToken();
      if (p==NULL) continue;
      if (!strcmp(p,tag)) {
	X = t.nextInt();
	Y = t.nextInt();
	W = t.nextInt();
	H = t.nextInt();
      }
    }
    f.close();
    repaint();
  }


  void setMinimumSize(int mw,int mh) {
    minw = mw;
    minh = mh;
    if (W < minw) { W = minw; repaint(); }
    if (H < minh + th) { H = minh + th; repaint(); }
  }

  void paintHelp() {
    unsigned int i;
    int mw,cw,lh,by;
    vector<string> help;

    help = composeHelp();
    if (help.empty()) return;

    mw = 0;
    for(i=0;i<help.size();i++) {
      cw = stringWidth(help[i].c_str(),getHelpPFD());
      if (cw > mw) mw = cw;
    }

    lh = stringHeight(help[0].c_str(),getHelpPFD());
    lh+=2;

    by = Y+H+16;
    rect(X,Y+H+8,mw+16,16+lh*help.size(),ui.BG * 1.25,true);
    rect(X,Y+H+8,mw+16,16+lh*help.size(),ui.Border,false);

    rect(X+10,Y+H,10,9,ui.BG * 1.25,true);
    line(X+10,Y+H,X+10,Y+H+8,ui.Border);
    line(X+20,Y+H,X+20,Y+H+8,ui.Border);

    for(i=0;i<help.size();i++) {
      drawString(X+8+1,by+i*lh+1,Color(0),help[i].c_str(),getHelpPFD());
      drawString(X+8,by+i*lh,Color(0xffffff),help[i].c_str(),getHelpPFD());
    }
  }

  void paint(bool active=true) {
    int pw,ph;

    pw = stringWidth(Title);
    ph = stringHeight(Title);

    rect(X,Y+th,W,H-th,ui.BG,true);
    rect(X,Y,pw+20,th,ui.Title[active?0:1],true);
    if (resizable)
      rect(X+W-16,Y+H-16,16,16,ui.Title[active?0:1],true);
    rect(X,Y+th,W,H-th,ui.Border,false);
    rect(X,Y,pw+20,th,ui.Border,false);
    if (resizable) {
      int x,y;
      x = X+W-16; y = Y+H-16;
      rect(x,y,16,16,ui.Border,false);
      line(x+3,y+13,x+13,y+13,ui.Border);
      line(x+13,y+13,x+13,y+3,ui.Border);
      line(x+13,y+3,x+3,y+13,ui.Border);
    }
    drawString(X+10,Y+(th-ph)/4,ui.Title[active?2:3],Title);
    tw = pw+20;

    GdkRectangle r,s;
    GdkRegion *rr, *ss;
    r.x = X+1;
    r.y = Y+th+1;
    r.width = W-2;
    r.height = H-th-2;

    s.x = X+W-16;
    s.y = Y+H-16;
    s.width = s.height = 16;

    rr = gdk_region_rectangle(&r);
    ss = NULL;
    if (resizable) {
      ss = gdk_region_rectangle(&s);
      gdk_region_subtract(rr,ss);   
    }
    setClip(rr);
    paintClient();
    r.x = 0;
    r.y = 0;
    getWindowSize(r.width,r.height);
    setClip(&r);
    gdk_region_destroy(rr);
    if (ss!=NULL)
      gdk_region_destroy(ss);
  }

  virtual void paintClient() { }
  virtual void pressClient(int x,int y,int button) { }
  virtual void releaseClient(int x,int y,int button) { }
  virtual void dragClient(int x,int y,int button) { }

  void press(int x,int y,int button) {
    if (button==1 && insideExtents(x,y,X,Y,tw,th)) {
      ma = 1; px=x; py=y; grab=true; return;
    }
    if (resizable && button==1 && insideExtents(x,y,X+W-16,Y+H-16,16,16)) {
      ma = 2; px=x; py=y; grab=true; return;
    }
    if (insideExtents(x,y,X,Y+th,W,H-th)) {
      grab = true;
      pressClient(x-X,y-th-Y,button);
    }
  }

  void release(int x,int y,int button) {
    if (ma!=0) {
      drag(x,y,button);
      ma = 0;
      grab = false;
      return;
    }
    if (insideExtents(x,y,X,Y+th,W,H-th)) {
      grab = false;
      releaseClient(x-X,y-th-Y,button);
    }
  }

  void drag(int x,int y,int button) {
    if (ma==1 && button==1) {
      X += x-px;
      Y += y-py;
      px = x;
      py = y;
      repaint();
      return;
    }
    if (ma==2 && button==1) {
      W += x-px;
      H += y-py;
      if (W < minw) W = minw;
      if (H < minh + th) H = minh + th;
      px = x;
      py = y;
      repaint();
      return;      
    }
    dragClient(x-X,y-th-Y,button);
  }

  bool inside(int x,int y) { return(insideExtents(x,y,X,Y,tw,th)||insideExtents(x,y,X,Y+th,W,H-th)); }

  bool hasGrab() { return grab; }

protected:
  int tw,th,ma,px,py;
  bool grab;

  bool insideCoords(int x,int y,int x1,int y1,int x2,int y2) {
    return((x>=x1)&&(x<=x2)&&(y>=y1)&&(y<=y2));
  }

  bool insideExtents(int x,int y,int x1,int y1,int w,int h) {
    return(insideCoords(x,y,x1,y1,x1+w-1,y1+h-1));
  }

private:
  int minw, minh;
};

class MessageWindow : public Window {
  
 public:
  MessageWindow(const char *title, int x,int y,
		PangoFontDescription *mfont, int scolor=0) : 
    Window(title,x,y,450,200) 
  {
    pfd = mfont;
    shadowcolor = scolor;
    setMinimumSize(400,64);
    yscroll = 0;
  }

  virtual ~MessageWindow() {
    vector<char *>::iterator i;
    for(i=msgs.begin();i!=msgs.end();i++)
      free(*i);
    msgs.clear();
    clr.clear();
  }

  vector<string> composeHelp() {
    return(keyHelp());
  }

  void append(const char *text, int color=0xffffff) {
    msgs.push_back(strdup(text));
    clr.push_back(color);
    repaint();
  }

  virtual void paintClient() {
    int i,y,ph,boxh, gap, minl, maxl;
    Color black, blue, shade;

    ph = stringHeight("Sample");
    y = H - (ph+4) + yscroll;

    black = 0;
    blue  = 0x5555ff;
    shade = ui.BG * 0.80; 

    gap = (H - 60) / msgs.size();
    if (gap < 2) gap = 2;
    boxh = gap - 1;
    if (boxh < 2) boxh = 2;

    minl = maxl = -1;

    for(i=msgs.size()-1;i>=0;i--) {

      if (y>0 && y<H) {

	if (i < minl || minl<0) minl = i;
	if (i > maxl || maxl<0) maxl = i;

	drawString(X+10+1,Y+y+1,Color(shadowcolor),msgs[i],pfd);
	drawString(X+10,Y+y,Color(clr[i]),msgs[i],pfd);

	rect(X+W-20,Y+30+i*gap,15,boxh,blue,true);
	//rect(X+W-20,Y+30+i*gap,15,boxh,black,false);

      } else {

	rect(X+W-20,Y+30+i*gap,15,boxh,shade,true);
      }

      y -= (ph + 4);
    }

    sbvar[0] = ph+4; // text line height
    sbvar[1] = 6;   // y top
    sbvar[2] = gap;  // scroll line height
    sbvar[3] = minl; // top text line
    sbvar[4] = maxl; // bottom text line

  }
    
  virtual void pressClient(int x,int y,int b) {
    int l;

    if (b==1 && insideExtents(x,y,W-20,sbvar[1],15,sbvar[2]*msgs.size())) {
      gx = x;
      gy = y;
      
      l = (y - sbvar[1]) / sbvar[2];

      if (sbvar[3] < 0) {
	yscroll = 0;
	repaint();
      } else if (l < sbvar[3]) {
	yscroll += sbvar[0] * (sbvar[3] - l);
	repaint();
      } else if (l > sbvar[4]) {
	yscroll -= sbvar[0] * (l - sbvar[4] + 1);
	repaint();
      }
    }

  }

  virtual void dragClient(int x,int y, int b) {
    int dy;
    if (b==1 && gx>=0 && gy>=0) {
      dy = y - gy;
      gx = x;
      gy = y;

      dy *= sbvar[0];
      dy /= sbvar[2];
      yscroll -= dy;
      repaint();
    }
  }

  virtual void releaseClient(int x,int y, int b) {
    gx=gy=-1;
  }

 private:
  PangoFontDescription *pfd;
  int msgcolor, shadowcolor;
  vector<char *> msgs;
  vector<int> clr;
  int yscroll, sbvar[5], gx,gy;
};

class LUT {
public:
  LUT() { lut = NULL; lutsz = 0; pmax=-1; pmaxs=0.0; }
  virtual ~LUT() { if (lut!=NULL) delete lut; }

  void prepareLUT(int maxval, float maxvals) {
    int i,emv;
    Color c;

    if (lut!=NULL && pmax == maxval && pmaxs == maxvals) return;

    alloc(maxval+1);
    emv = (int) (maxval * maxvals);
    for(i=0;i<lutsz;i++) {      
      c.gray( (i>emv) ? 255 : (int) (255.0 * i / emv) );
      lut[i] = c.toInt();
    }
    pmax  = maxval;
    pmaxs = maxvals;
  }

  int *getLUT() { return lut; }

private:
  int   lutsz;
  int  *lut;
  int   pmax;
  float pmaxs;

  void alloc(int sz) {
    if (lutsz==sz && lut!=NULL) return;
    if (lut!=NULL) delete lut;
    lut = new int[sz];
    lutsz = sz;
  }
};

class AdjustableDepth {
public:
  AdjustableDepth() { depth = 0; px=py=pz=-1; maxdepth = 0; }
  
  virtual ~AdjustableDepth() { 
    set<AdjustableDepth *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++)
      (*i)->removeDepthSync(this);
    sync.clear();
  }
  
  int getDepth() { return depth; }
  
  void setDepth(int val) {
    depth = val;    
    //cout << "depth = " << val << ", max = " << maxdepth << endl;
    if (depth < 0) depth = 0;
    if (depth > maxdepth) depth = maxdepth;
    invalidateDepth();
    syncAll();
  }

  void setCursor(int x,int y, int z) {
    px = x;
    py = y;
    pz = z;
    invalidateDepth();
    syncAll();
  }

  void clearCursor() {
    px=py=pz=-1;
    invalidateDepth();
    syncAll();
  }

  bool hasCursor() {
    return(px>=0);
  }

  void getCursor(int &x,int &y, int &z) {
    x = px;
    y = py;
    z = pz;
  }

  void setMaxDepth(int val) {
    //cerr << "max = " << val << endl;
    maxdepth = val;
    if (depth > maxdepth) {
      depth = maxdepth;
      invalidateDepth();
      syncAll();
    }
  }
  
  virtual void invalidateDepth() = 0;
  
  void syncDepth(AdjustableDepth *other) {
    set<AdjustableDepth *>::iterator i;
    if (other != this) {
      for(i=other->sync.begin();i!=other->sync.end();i++)
	sync.insert(*i);
      sync.insert(other);
      for(i=sync.begin();i!=sync.end();i++)
	(*i)->sync.insert(this);
    }
  }      

  void clearDepthSync() {
    sync.clear();
  }

  void dumpDepthGroup() {
    set<AdjustableDepth *>::iterator i;
    cerr << "DEPTH SYNC:\n";
    for(i=sync.begin();i!=sync.end();i++)
      cerr << (*i) << endl;
    cerr << "==END==:\n";
  }
  
private:
  int depth, maxdepth;
  int px,py,pz;
  
  set<AdjustableDepth *> sync;
  
  void syncAll() {
    set<AdjustableDepth *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++) {
      (*i)->depth = depth;
      (*i)->maxdepth = maxdepth;
      (*i)->px = px;
      (*i)->py = py;
      (*i)->pz = pz;
      (*i)->invalidateDepth();
    }
  }

  void removeDepthSync(AdjustableDepth *who) {
    set<AdjustableDepth *>::iterator i;
    i = find(sync.begin(),sync.end(),who);
    if (i!=sync.end()) sync.erase(i);
  }

};

class AdjustableViewRange {
public:
  AdjustableViewRange() { range = 1.0; }

  virtual ~AdjustableViewRange() { 
    set<AdjustableViewRange *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++)
      (*i)->removeViewRangeSync(this);
    sync.clear();
  }

  float getViewRange() { return range; }

  void  setViewRange(float val) { 
    range = val;
    if (range < 0.05) range = 0.05;
    if (range > 1.0) range = 1.0;
    invalidateViewRange();
    syncAll();
  }
  
  virtual void invalidateViewRange() = 0;

  void syncViewRange(AdjustableViewRange *other) {
    set<AdjustableViewRange *>::iterator i;
    if (other != this) {
      for(i=other->sync.begin();i!=other->sync.end();i++)
	sync.insert(*i);
      sync.insert(other);
      for(i=sync.begin();i!=sync.end();i++)
	(*i)->sync.insert(this);
    }
  }

  void clearViewRangeSync() {
    sync.clear();
  }


private:
  float range;
  set<AdjustableViewRange *> sync;

  void syncAll() {
    set<AdjustableViewRange *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++) {
      (*i)->range = range;
      (*i)->invalidateViewRange();
    }
  }

  void removeViewRangeSync(AdjustableViewRange *who) {
    set<AdjustableViewRange *>::iterator i;
    i = find(sync.begin(),sync.end(),who);
    if (i!=sync.end()) sync.erase(i);
  }

};

class Button {
public:
  int X,Y,W,H;

  Button(int x,int y,int w,int h,char **xpm,
	 const Color &act, const Color &inac, const Color &border) {
    X = x;
    Y = y;
    W = w;
    H = h;
    c[0] = border;
    c[1] = act;
    c[2] = inac;

    icon[0] = xpm != NULL ? new Image(xpm,c[1]) : NULL;
    icon[1] = xpm != NULL ? new Image(xpm,c[2]) : NULL;
  }

  virtual ~Button() {
    if (icon[0]!=NULL) delete icon[0];
    if (icon[1]!=NULL) delete icon[1];
  }

  bool inside(int x,int y) {
    return(x>=X && x<X+W && y>=Y && y<Y+H);
  }

  void draw(GdkWindow *w,GdkGC *gc,int bx,int by,bool active,bool down) {
    gdk_rgb_gc_set_foreground(gc, 0);
    gdk_draw_rectangle(w,gc,TRUE,bx+X+4,by+Y+4,W,H);

    gdk_rgb_gc_set_foreground(gc, c[ active ? 1 : 2].toInt());
    gdk_draw_rectangle(w,gc,TRUE,bx+X+(down?3:0),by+Y+(down?3:0),W,H);
    gdk_rgb_gc_set_foreground(gc, c[0].toInt() );
    gdk_draw_rectangle(w,gc,FALSE,bx+X+(down?3:0),by+Y+(down?3:0),W,H);

    if (icon[0]!=NULL) {
      Image *img;
      img = active ? icon[0] : icon[1];
      if (c!=NULL)
	gdk_draw_rgb_image(w,gc,bx+X+(W-img->W)/2+(down?3:0),by+Y+(H-img->H)/2+(down?3:0), 
			   img->W,img->H, GDK_RGB_DITHER_NORMAL, img->getBuffer(), 3*(img->W));
    }
  }

private:
  Color c[3];
  Image *icon[2];

};

#include "arrow.xpm"
#include "poly.xpm"

#include "fcd.xpm"
#include "ok.xpm"
#include "void.xpm"

class BrainToolbar : public Window {
public:
  BrainToolbar(const char *title,int x,int y) :
    Window(title,x,y,250,24+32+20)
  {
    int i;
    mode  = 0;
    label = 0;
    mdown = ndown = -1;
    setMinimumSize(0,0);
    resizable = false;

    bc1 = 0xffe000;
    bc2 = 0xc0c0c0;

    buttons.push_back(new Button(10+0*40, 10,32,32,arrow_xpm,  bc1,bc2,ui.Border));
    buttons.push_back(new Button(10+1*40, 10,32,32,poly_xpm,   bc1,bc2,ui.Border));

    buttons.push_back(new Button(120+0*40,10,32,32,fcd_xpm, bc1,bc2,ui.Border));
    buttons.push_back(new Button(120+1*40,10,32,32,ok_xpm,  bc1,bc2,ui.Border));
    buttons.push_back(new Button(120+2*40,10,32,32,void_xpm, bc1,bc2,ui.Border));

    for(i=0;i<5;i++)
      active[i] = 0;
    active[0] = 1;
    active[2] = 1;
  }

  virtual ~BrainToolbar() {
    unsigned int i;
    for(i=0;i<buttons.size();i++)
      delete(buttons[i]);
    buttons.clear();
  }

  vector<string> composeHelp() {
    return(keyHelp());
  }

  virtual void paintClient() {
    unsigned int i;

    for(i=0;i<buttons.size();i++)
      buttons[i]->draw(getWindow(),getGC(),X,Y+th,active[i],ndown==(int)i);
  }

  virtual void pressClient(int x,int y,int b) {
    unsigned int i;
    if (b==1) {
      for(i=0;i<buttons.size();i++)
	if (buttons[i]->inside(x,y)) {
	  mdown = ndown = (int) i;
	  repaint();
	  return;
	}
    }
  }

  virtual void dragClient(int x,int y,int b) {
    if (b==1 && mdown>=0) {
      int pn = ndown;
      ndown = buttons[mdown]->inside(x,y) ? mdown : -1;
      if (ndown != pn) repaint();
    }
  }

  virtual void releaseClient(int x,int y,int b) {
    unsigned int i;
    if (b==1 && mdown>=0) {
      for(i=0;i<buttons.size();i++)
	if (buttons[i]->inside(x,y)) {
	  if (mdown == (int) i) {
	    action((int) i);
	    break;
	  }
	}
      mdown = -1;
      ndown = -1;
      repaint();
    }    
  }

  void action(int b) {
    int i;
    if (b<2) {
      mode = b;
      for(i=0;i<2;i++) active[i] = (b==i);
    } else if (b<5) {
      label = b-2;
      for(i=2;i<5;i++) active[i] = (b==i);      
    }
  }

  int getMode()  { return mode; }
  int getLabel() { return((label+1)%3); }

private:
  int mode, label;
  int mdown, ndown;
  vector<Button *> buttons;
  int active[11];
  Color bc1,bc2;

};

class BrainToolbarClient {
public:
  BrainToolbarClient() { toolbar = NULL; }
  
  int getToolbarMode()  { return(toolbar==NULL ? 0 : toolbar->getMode()); }
  int getToolbarLabel() { return(toolbar==NULL ? 0 : toolbar->getLabel()); }
  void setBrainToolbar(BrainToolbar *btb) { toolbar = btb; }

private:
  BrainToolbar *toolbar;
};

class OrthogonalView : public Window, 
		       public LUT, 
		       public AdjustableViewRange,
		       public AdjustableDepth
{
 public:
  OrthogonalView(const char *title, int x, int y, int w, int h) : Window(title,x,y,w,h) {
    vol = NULL;
    seg = NULL;
    edt = NULL;
    xy = zy = xz = NULL;
    p=q=r=NULL;
    invalid[0] = invalid[1] = invalid[2] = true;
    showseg=true;
    setMinimumSize(64,72);
  }

  virtual ~OrthogonalView() {
    if (xy!=NULL) delete xy;
    if (zy!=NULL) delete zy;
    if (xz!=NULL) delete xz;
    if (p!=NULL) delete p;
    if (q!=NULL) delete q;
    if (r!=NULL) delete r;
  }
  
  void setVolume(Volume<int> *v) {
    vol = v;

    if (p!=NULL) {
      delete p;
      delete q;
      delete r;
      p=q=r=NULL;
    }

    maxval = vol->maximum();
    cx = vol->W / 2;
    cy = vol->H / 2;
    cz = vol->D / 2;
    PW = PH = -1;
    invalidate();
  }

  void setEDT(Volume<int> *v) {
    edt = v;
    if (edt!=NULL) {
      int maxdepth = (int) ceil(sqrt(edt->maximum()));
      setMaxDepth(maxdepth);
    }
    invalidate();
  }

  void setSegmentation(Volume<char> *v) {
    seg = v;
    invalidate();
  }

  vector<string> composeHelp() {
    vector<string> h;
    h = keyHelp();
    h.push_back(string("[Mouse] Left:Passive 2D Cursor, Right:Active 2D Cursor"));
    return(h);
  }

  virtual void invalidateViewRange() {
    invalidate();
  }

  virtual void invalidateDepth() {
    invalidate();
  }

  void invalidate() {
    invalid[0]=invalid[1]=invalid[2]=true;
    repaint();
  }

  virtual void paintClient() {

    if (vol==NULL) return;
    if (PW!=W || PH!=H) { invalid[0]=invalid[1]=invalid[2]=true; }

    if (invalid[0] || invalid[1] || invalid[2])
      render(invalid[0],invalid[1],invalid[2]);

    int a,b,c,bx,by;

    bx = X+10;
    by = Y+th+10;

    /* the images */
    drawImage(xy,bx,by);
    drawImage(zy,bx+10+xy->W,by);
    drawImage(xz,bx,by+10+xy->H);

    a = (int) (zoom * cx);
    b = (int) (zoom * cy);
    c = (int) (zoom * cz);

    /* the cursor */
    line(bx,by+b,bx+xy->W,by+b,Color(0x00ff00));
    line(bx+a,by,bx+a,by+xy->H,Color(0x00ff00));
    line(bx+10+xy->W,by+b,bx+10+xy->W+zy->W,by+b,Color(0x00ff00));
    line(bx+10+xy->W+c,by,bx+10+xy->W+c,by+zy->H,Color(0x00ff00));
    line(bx,by+10+xy->H+c,bx+xz->W,by+10+xy->H+c,Color(0x00ff00));
    line(bx+a,by+10+xy->H,bx+a,by+10+xy->H+xz->H,Color(0x00ff00));

    /* border */
    rect(bx,by,xy->W,xy->H,ui.Border,false);
    rect(bx+10+xy->W,by,zy->W,zy->H,ui.Border,false);
    rect(bx,by+10+xy->H,xz->W,xz->H,ui.Border,false);

    /* text on bottom */
    char z[256];
    snprintf(z,256,"Size = %d x %d x %d. (%d, %d, %d) = %d. Zoom %d %%",
	     vol->W,vol->H,vol->D,cx,cy,cz, vol->voxel(cx,cy,cz), (int) (100.0*zoom));
    drawString(X+10,Y+H-24,Color(0xffff80),z);

    /* segmentation check box */
    if(seg!=NULL) {
      drawCheckBox(bx+10+xy->W,by+10+xy->H,showseg);
      drawString(bx+10+xy->W + 16, by+10+xy->H, Color(0xffff80), "Show Segmentation");
    }

    drawString(bx+10+xy->W,by+10+xy->H+20,Color(0xffff80),"View Range: ");
    drawSlider(bx+10+xy->W+10,by+10+xy->H+40,100,16,getViewRange(),4,false);

  }

  virtual void pressClient(int x,int y,int button) {
    int bx,by;
    if (vol==NULL) return;

    bx = 10;
    by = 10;

    if ((button==1 || button==3) && insideExtents(x,y,bx,by,xy->W,xy->H)) {
      float nx,ny;
      nx = (x-bx) / zoom;
      ny = (y-by) / zoom;
      cx = (int) nx;
      cy = (int) ny;
      if (cx < 0) cx = 0;
      if (cx >= vol->W) cx = vol->W-1;
      if (cy < 0) cy = 0;
      if (cy >= vol->H) cy = vol->H-1;
      invalid[1] = true;
      invalid[2] = true;

      if (button==3 && edt!=NULL) {
	int d = edt->voxel(cx,cy,cz);
	if (d < 0)
	  d=0; 
	else {
	  d=(int) sqrt(d);
	  setCursor(cx,cy,cz);
	}
	setDepth(d);
      } else {
	repaint();
      }
      return;
    }

    if ((button==1 || button==3) && insideExtents(x,y,bx+10+xy->W,by,zy->W,zy->H)) {
      float nz,ny;
      nz = (x-(bx+10+xy->W)) / zoom;
      ny = (y-by) / zoom;
      cz = (int) nz;
      cy = (int) ny;
      if (cz < 0) cz = 0;
      if (cz >= vol->D) cz = vol->D-1;
      if (cy < 0) cy = 0;
      if (cy >= vol->H) cy = vol->H-1;
      invalid[0] = true;
      invalid[2] = true;
      if (button==3 && edt!=NULL) {
	int d = edt->voxel(cx,cy,cz);
	if (d < 0) 
	  d=0; 
	else {
	  d=(int) sqrt(d);
	  setCursor(cx,cy,cz);
	}
	setDepth(d);
      } else {
	repaint();
      }
      return;
    }

    if ((button==1 || button==3) && insideExtents(x,y,bx,by+10+xy->H,xz->W,xz->H)) {
      float nx,nz;
      nx = (x-bx) / zoom;
      nz = (y-(by+10+xy->H)) / zoom;
      cx = (int) nx;
      cz = (int) nz;
      if (cx < 0) cx = 0;
      if (cx >= vol->W) cx = vol->W-1;
      if (cz < 0) cz = 0;
      if (cz >= vol->D) cz = vol->D-1;
      invalid[0] = true;
      invalid[1] = true;
      if (button==3 && edt!=NULL) {
	int d = edt->voxel(cx,cy,cz);
	if (d < 0) 
	  d=0; 
	else {
	  d=(int) sqrt(d);
	  setCursor(cx,cy,cz);
	}
	setDepth(d);
      } else {
	repaint();
      }
      return;
    }

    if (seg!=NULL && button==1 && insideExtents(x,y,bx+10+xy->W,by+10+xy->H,zy->W,16)) {
      showseg = !showseg;
      invalid[0]=invalid[1]=invalid[2]=true;
      repaint();
    }

    if (button==1 && insideExtents(x,y,bx+10+xy->W+10,by+10+xy->H+40,100,16))
      setViewRange( (x-(bx+10+xy->W+10)) / 100.0 );
  }

  virtual void dragClient(int x,int y,int b) {
    if (b==1 || b==3) pressClient(x,y,b);
  }

  virtual void releaseClient(int x,int y,int b) {
    if (hasCursor())
      clearCursor();
  }

 private:
  Volume<int>  *vol, *edt;
  Volume<char> *seg;
  int maxval,cx,cy,cz, PW,PH;
  float zoom;
  Image *xy, *zy, *xz, *p, *q, *r;
  bool invalid[3], showseg;

  void render(bool rxy=true, bool rzy=true, bool rxz=true) {
    int i,j,sqd=0,nsqd=0,v;
    Color c,d,yellow,red;
    float caw, cah, fsw, fsh, z0, z1;
    int *lut;

    if (!vol) return;

    if (p==NULL) p = new Image(vol->W,vol->H);
    if (q==NULL) q = new Image(vol->D,vol->H);
    if (r==NULL) r = new Image(vol->W,vol->D);

    prepareLUT(maxval, getViewRange());
    lut = getLUT();

    yellow = 0xffff00;
    red = 0xff0000;

    if (edt!=NULL) {
      sqd = getDepth() * getDepth();
      nsqd = (getDepth()+1) * (getDepth()+1);
    }

    if (rxy) {
      for(j=0;j<vol->H;j++)
	for(i=0;i<vol->W;i++) {
	  c = lut[ vol->voxel(i,j,cz) ];
	  if (seg!=NULL && showseg) {
	    if (seg->voxel(i,j,cz)==1) c.mix(yellow,0.25);
	    else if (seg->voxel(i,j,cz)==2) c.mix(yellow,0.75);
	  }
	  if (edt!=NULL) {
	    v = edt->voxel(i,j,cz);
	    if (v >= sqd && v <= nsqd)
	      c.mix(red,0.75);
	  }
	  p->set(i,j,c);
	}
    }
    if (rzy) {
      for(j=0;j<vol->H;j++)
	for(i=0;i<vol->D;i++) {
	  c = lut[ vol->voxel(cx,j,i) ];
	  if (seg!=NULL && showseg) {
	    if (seg->voxel(cx,j,i)==1) c.mix(yellow,0.25);
	    else if (seg->voxel(cx,j,i)==2) c.mix(yellow,0.75);
	  }
	  if (edt!=NULL) {
	    v = edt->voxel(cx,j,i);
	    if (v >= sqd && v <= nsqd)
	      c.mix(red,0.75);
	  }
	  q->set(i,j,c);
	}
    }
    if (rxz) {
      for(j=0;j<vol->D;j++)
	for(i=0;i<vol->W;i++) {
	  c = lut[ vol->voxel(i,cy,j) ];
	  if (seg!=NULL && showseg) {
	    if (seg->voxel(i,cy,j)==1) c.mix(yellow,0.25);
	    else if (seg->voxel(i,cy,j)==2) c.mix(yellow,0.75);
	  }
	  if (edt!=NULL) {
	    v = edt->voxel(i,cy,j);
	    if (v >= sqd && v <= nsqd)
	      c.mix(red,0.75);
	  }
	  r->set(i,j,c);
	}
    }

    caw = W - 20.0;
    cah = H - th - 20.0 - 24.0;
    fsw = vol->W + vol->D + 10.0;
    fsh = vol->H + vol->D + 10.0;
    z0 = caw / fsw;
    z1 = cah / fsh;
    if (z0 < z1) zoom=z0; else zoom=z1;

    if (rxy) {
      if (xy!=NULL) { delete xy; xy=NULL; }
      xy = p->scale(zoom);
    }

    if (rzy) {
      if (zy!=NULL) { delete zy; zy=NULL; }
      zy = q->scale(zoom);
    }

    if (rxz) {
      if (xz!=NULL) { delete xz; xz=NULL; }
      xz = r->scale(zoom);
    }

    PW = W;
    PH = H;
    invalid[0] = invalid[1] = invalid[2] = false;
  }

};

class RenderObject {
public:
  char         *name;
  Volume<char> *mask;
  Color         color;
  float         opacity;
  bool          active;
  RenderType    rendertype;
  int           rendervalue;

  RenderObject() {
    mask    = NULL;
    color   = 0;
    opacity = 1.0;
    active  = true;
    name    = NULL;
    rendertype  = RenderType_EQ;
    rendervalue = 2;
  }

  RenderObject(const char *n, Volume<char> *m, Color &c, 
	       float op=1.0, bool act=true,
	       RenderType rt = RenderType_EQ, int rv = 2) {
    name = strdup(n);
    mask = m;
    color = c;
    opacity = op;
    active = act;
    rendertype  = rt;
    rendervalue = rv;
  }

  int operator==(const char *s) { if (name==NULL) return 0; else return(strcmp(name,s)==0); }

  virtual ~RenderObject() {
    if (name!=NULL) free(name);
  }

};

class Rotatable {
public:
  Rotatable() {
    rot.identity();
  }

  virtual ~Rotatable() { sync.clear(); }

  virtual void setRotation(T3 &t) {
    rot = t;
    invalidateRotation();
    syncAll();
  }

  T3 & getRotation() {
    return rot;
  }

  void pressRotate(int x,int y,int b) {
    switch(b) {
    case 1:
      lx = x;
      ly = y;
      break;
    case 3:
      rot.identity();
      invalidateRotation();
      syncAll();
      break;
    }
  }

  void releaseRotate(int x,int y,int b) {
    if (b==1) dragRotate(x,y,b);
  }

  void dragRotate(int x,int y,int b) {
    if (b==1) {
      T3 rx,ry;
      ry.yrot(x - lx);
      rx.xrot(y - ly);
      rot *= rx;
      rot *= ry;
      lx = x;
      ly = y;
      invalidateRotation();
      syncAll();
    }
  }

  virtual void invalidateRotation() = 0;

  void syncRotation(Rotatable *other) {
    set<Rotatable *>::iterator i;
    if (other != this) {
      for(i=other->sync.begin();i!=other->sync.end();i++)
	sync.insert(*i);
      sync.insert(other);
      for(i=sync.begin();i!=sync.end();i++)
	(*i)->sync.insert(this);
    }
  }
  
protected:
  T3 rot;
private:
  int lx,ly;
  set<Rotatable *> sync;

  void syncAll() {
    set<Rotatable *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++) {
      (*i)->rot = rot;
      (*i)->invalidateRotation();
    }
  }
};

class BrainTagger {
public:
  BrainTagger() {
    rtag = NULL;
    ptag = NULL;
    pmap = NULL;
    rdepth = NULL;
    frozen = 0;
    pend_diam  = 1;
    pend_label = 1;
  }

  virtual ~BrainTagger() { sync.clear(); pendingtags.clear(); }

  void setTags(Volume<char> *rt, Volume<char> *pt, Volume<int> *pm, Volume<int> *sqd) {
    rtag = rt;
    ptag = pt;
    pmap = pm;
    rdepth = sqd;
  }

  virtual void tagsChanged() = 0;

  void pushRoundTag(int x,int y,int z,int bdiam,char label) {
    int n;
    if (ptag == NULL || pmap == NULL || rtag == NULL || rdepth == NULL) return;

    n = rtag->address(x,y,z);
    rtag->voxel(n) = label;
    pendingtags.push_back(n);
    pend_diam  = bdiam;    
    pend_label = label;
  }

  void flushRoundTags() {
    int i,j,k,z0,z1;
    unsigned int ui;

    if (pendingtags.empty())
      return;

    sort(pendingtags.begin(),pendingtags.end());

    freezeTagger();

    z0 = minz(pendingtags[0]);
    z1 = maxz(pendingtags[0]);

    for(ui=1;ui<pendingtags.size();ui++) {
      if (minz(pendingtags[ui]) < z0) z0 = minz(pendingtags[ui]);
      if (maxz(pendingtags[ui]) > z1) z1 = maxz(pendingtags[ui]);
    }

    if (z0 < 0) z0 = 0;
    if (z1 < 0) z1 = 0;
    if (z0 >= ptag->D) z0 = ptag->D - 1;
    if (z1 >= ptag->D) z1 = ptag->D - 1;

    for(k=z0;k<=z1;k++)
      for(j=0;j<ptag->H;j++)
	for(i=0;i<ptag->W;i++)
	  if (binary_search(pendingtags.begin(),pendingtags.end(), pmap->voxel(i,j,k)))
	    planarTag(i,j,k,pend_diam,pend_label);

    thawTagger();
    pendingtags.clear();
  }

  void getXY(int rx,int ry,int rz,int pz,int &px,int &py) {
    int i,j,k,mink,maxk,r;
    r = rtag->address(rx,ry,rz);

    mink = pz - 3; if (mink < 0) mink=0;
    maxk = pz + 3; if (maxk >= ptag->D) maxk = ptag->D - 1;

    for(k=mink;k<=maxk;k++)
      for(j=0;j<ptag->H;j++)
	for(i=0;i<ptag->W;i++)
	  if (pmap->voxel(i,j,k) == r) {
	    px = i;
	    py = j;
	    return;
	  }
    px = -1;
    py = -1;
  }

  void roundTag(int x,int y,int z,int bdiam, char label) {
    int i,j,k,z0,z1,n;

    if (ptag == NULL || pmap == NULL || rtag == NULL || rdepth == NULL) return;

    freezeTagger();

    z0 = (int) floor(sqrt(rdepth->voxel(x,y,z)));
    z1 = (int) ceil(sqrt(rdepth->voxel(x,y,z)));
    if (z0 < 0) z0 = 0;
    if (z1 < 0) z1 = 0;
    if (z0 >= ptag->D) z0 = ptag->D - 1;
    if (z1 >= ptag->D) z1 = ptag->D - 1;
    n = rtag->address(x,y,z);

    for(k=z0;k<=z1;k++)
      for(j=0;j<ptag->H;j++)
	for(i=0;i<ptag->W;i++)
	  if (pmap->voxel(i,j,k) == n)
	    planarTag(i,j,k,bdiam,label);

    thawTagger();
  }

  void roundTag(int px,int py,int pz,int x,int y,int z,int bdiam, char label) {
    int hx,hy,hz;

    freezeTagger();
    roundTag(x,y,z,bdiam,label);
    roundTag(px,py,pz,bdiam,label);

    hx = (px+x)/2;
    hy = (py+y)/2;
    hz = (pz+z)/2;

    if ( (hx==x && hy==y && hz==z) || (hx==px && hy==py && hz==pz) ) {
      thawTagger();
      return;
    }
    
    roundTag(x,y,z,hx,hy,hz,bdiam,label);
    roundTag(hx,hy,hz,px,py,pz,bdiam,label);
    thawTagger();
  }

  void planarTag(int x,int y,int z, int bdiam, char label) {
    Location a,b;
    int i,j;

    if (ptag == NULL || pmap == NULL || rtag == NULL) return;
    a.set(x,y,z);
    if (!ptag->valid(a)) return;

    if (bdiam > 1) {
      DiscAdjacency da(bdiam/2.0,true);
      for(i=0;i<da.size();i++) {
	b = da.neighbor(a,i);
	if (ptag->valid(b)) {
	  ptag->voxel(b) = label;
	  j = pmap->voxel(b);
	  if (j < rtag->N && j>=0)
	    rtag->voxel(j) = label;
	}
      }
    } else {

      ptag->voxel(a) = label;
      j = pmap->voxel(a);
      if (j < rtag->N && j >= 0)
	rtag->voxel(j) = label;

    }

    updateTagger();
  }

  void planarTag(int px,int py,int pz,int x,int y,int z, 
		 int bdiam, char label) {
    int hx,hy,hz;

    freezeTagger();
    planarTag(x,y,z,bdiam,label);
    planarTag(px,py,pz,bdiam,label);

    hx = (px+x)/2;
    hy = (py+y)/2;
    hz = (pz+z)/2;

    if ( (hx==x && hy==y && hz==z) || (hx==px && hy==py && hz==pz) ) {
      thawTagger();
      return;
    }
    
    planarTag(x,y,z,hx,hy,hz,bdiam,label);
    planarTag(hx,hy,hz,px,py,pz,bdiam,label);

    thawTagger();
  }

  void syncTags(BrainTagger *other) {
    set<BrainTagger *>::iterator i;
    if (other != this) {
      for(i=other->sync.begin();i!=other->sync.end();i++)
        sync.insert(*i);
      sync.insert(other);
      for(i=sync.begin();i!=sync.end();i++)
        (*i)->sync.insert(this);
    }
  }

protected:
  Volume<char> * getRTag() { return rtag; }
  Volume<char> * getPTag() { return ptag; }

  void freezeTagger() { ++frozen; }

  void thawTagger() {
    --frozen;
    if (frozen < 0) frozen = 0;
    updateTagger();
  }

  void updateTagger() {
    if (!frozen) {
      tagsChanged();
      syncAll();
    }
  }

private:
  Volume<char> *rtag, *ptag;
  Volume<int>  *pmap, *rdepth;
  set<BrainTagger *> sync;
  vector<int> pendingtags;
  int frozen;
  int  pend_diam;
  char pend_label;

  void syncAll() {
    set<BrainTagger *>::iterator i;
    for(i=sync.begin();i!=sync.end();i++) {
      (*i)->tagsChanged();
    }
  }

  int minz(int n) {
    if (rdepth==NULL) return 0;
    if (n<0 || n>=rdepth->N) return 0;
    return((int) floor(sqrt(rdepth->voxel(n))));
  }

  int maxz(int n) {
    if (rdepth==NULL) return 0;
    if (n<0 || n>=rdepth->N) return 0;
    return((int) ceil(sqrt(rdepth->voxel(n))));
  }

};

class ObjectView : public Window, 
		   public Rotatable,
		   public BrainTagger
{
public:

  ObjectView(const char *title, int x,int y,int w,int h) :
    Window(title,x,y,w,h) {
    buf = NULL;
    unz = NULL;
    rc = NULL;
    invalid = 2;
    PW=PH=-1;
    zoom=1.0;
    maxdiag=-1;
    fsx=fsy=10;
    setMinimumSize(72,72);
  }

  virtual ~ObjectView() {
    if (buf!=NULL) delete buf;
    if (unz!=NULL) delete unz;
    if (rc!=NULL) delete rc;
    clearObjects();
  }

  vector<string> composeHelp() {
    vector<string> h;
    h = keyHelp();
    h.push_back(string("[Mouse] Left:Rotate, Right:Reset Rotation"));
    return(h);
  }


  void clearObjects() {
    unsigned int i;
    for(i=0;i<obj.size();i++)
      delete(obj[i]);
    obj.clear();
    invalid = 2;
  }  

  void appendObject(RenderObject *ro)  { addObject(ro,true); }
  void prependObject(RenderObject *ro) { addObject(ro,false); }

  RenderObject * getObjectByName(const char *name) {
    unsigned int i;
    for(i=0;i<obj.size();i++) if ( (*(obj[i])) == name ) return(obj[i]);
    return NULL;
  }

  virtual void tagsChanged() {
    invalid = 2;
    repaint();
  }

  virtual void invalidateRotation() {
    invalid = 2;
    repaint();
  }

  virtual void paintClient() {
    int x,y,sw,msw;
    unsigned int i;
    if (obj.empty()) return;
    if (W!=PW || H!=PH) invalid=1;
    if (invalid) render();

    x = X+W/2;
    y = Y+th+(H-th)/2;
    x -= buf->W / 2;
    y -= buf->H / 2 + 8*obj.size();

    drawImage(buf,x,y);

    msw = 0;
    for(i=0;i<obj.size();i++) {
      sw = stringWidth(obj[i]->name);
      if (sw > msw) msw = sw;
    }

    fsx = X+20+msw;
    fsy = Y+H-16*obj.size()-10;

    for(i=0;i<obj.size();i++)
      if (obj[i]->name != NULL) {
	drawString(X+10+1,fsy+16*i+1-2,Color(0),obj[i]->name);
	drawString(X+10,fsy+16*i-2,obj[i]->color,obj[i]->name);
	drawSlider(fsx,fsy+16*i,100,14,obj[i]->opacity,4,true);
      }

    fsx -= X;
    fsy -= Y+th;

    rect(X+5,Y+th+fsy-5,msw+20+150,16*obj.size()+10,ui.Border,false);
  }

  virtual void pressClient(int x,int y,int b) {

    if ((b==1 || b==3) && insideExtents(x,y,5,fsy-5,fsx+150,16*obj.size()+10)) {
      int o;
      float ns;
      o =  (y - fsy) / 16;
      ns = (x-fsx) / 100.0;
      if (ns > 1.0) ns=1.0;
      if (ns < 0.0) ns=0.0;
      if (b==3) ns = 1.0;
      if (o < (int) (obj.size())) {
	obj[o]->opacity = ns;
	invalid = 2;
	repaint();
      }
      return;
    }

    pressRotate(x,y,b);
  }

  virtual void dragClient(int x,int y,int b) {
    if ((b==1 || b==3) && insideExtents(x,y,5,fsy-5,fsx+150,16*obj.size()+10)) {
      pressClient(x,y,b);
      return;
    }

    dragRotate(x,y,b);
  }

  virtual void releaseClient(int x,int y,int b) {
    if (insideExtents(x,y,5,fsy-5,fsx+150,16*obj.size()+10)) return;
    releaseRotate(x,y,b);
  }

private:
  vector<RenderObject *> obj;
  int maxdiag,lx,ly;
  int PW,PH;
  Image *buf, *unz;
  RenderingContext *rc;
  int invalid;
  float zoom;

  int fsx,fsy;

  void render() {
    Color c1;
    float rw,rh,z0,z1;
    unsigned int i;

    if (obj.empty()) return;

    if (unz==NULL) {
      unz = new Image(maxdiag,maxdiag);
      rc = new RenderingContext(*unz);
      invalid = 2;
    }

    if (invalid==2) {
      c1 = ui.BG;
      unz->fill(c1);
      rc->prepareFirst();
      
      for(i=0;i<obj.size();i++)
	if (obj[i]->active && obj[i]->opacity > 0.0)
	  obj[i]->mask->render(*unz,rot,*rc,obj[i]->color,obj[i]->opacity,
			       obj[i]->rendertype, obj[i]->rendervalue);
    }

    if (buf==NULL) invalid = 1;

    if (invalid) {
      rw = W - 20.0;
      rh = H - 20.0 - th - 16.0 * obj.size();
      z0 = rw / unz->W;
      z1 = rh / unz->H;
      if (z0 < z1) zoom=z0; else zoom=z1;
      zoom *= 1.33;
    
      if (buf!=NULL) { delete buf; buf=NULL; }
      buf = unz->scale(zoom);
    }

    invalid = 0;
    PW = W;
    PH = H;
  }

  void addObject(RenderObject *ro, bool top) {
    unsigned int i;
    maxdiag = -1;

    if (unz!=NULL) { delete unz; unz = NULL; }
    if (rc!=NULL)  { delete rc; rc = NULL; }

    if (top)
      obj.push_back(ro);
    else
      obj.insert(obj.begin(), ro);

    for(i=0;i<obj.size();i++)
      if (obj[i]->mask->diagonalLength() > maxdiag)
	maxdiag = obj[i]->mask->diagonalLength();

    invalid = 2;
    repaint();

  }

};

class EDTView : public Window, 
		public LUT,
		public Rotatable, 
		public AdjustableViewRange,
		public AdjustableDepth,
		public BrainToolbarClient,
		public BrainTagger
{
public:
  EDTView(const char *title,int x,int y,int w,int h) :
    Window(title,x,y,w,h)
  {
    unz = NULL;
    buf = NULL;
    PW=PH=-1;
    edt  = NULL;
    orig = NULL;
    maxdepth = maxdiag = 0;
    zoom = 1.0;
    invalid = 2;
    rc = NULL;
    dsx = dsy = vrx = vry = rx = ry = ptx = pty = ptz = lcx = lcy = -1;
    setMinimumSize(72,72);
    useLight = false;
  }
  
  virtual ~EDTView() {
    if (unz!=NULL) delete unz;
    if (buf!=NULL) delete buf;
    if (rc!=NULL) delete rc;
  }

  vector<string> composeHelp() {
    vector<string> h;
    h = keyHelp();
    switch(getToolbarMode()) {
    case 0: h.push_back(string("[Mouse] Left:Rotate, Right:Reset Rotation")); break;
    case 1: h.push_back(string("[Mouse] Left:Tag (Contour)")); break;
    }
    return(h);
  }

  void setEDT(Volume<int> *vedt, Volume<int> *vorig) {
    orig = vorig;
    edt  = vedt;

    if (unz!=NULL) { delete unz; unz = NULL; }
    if (rc!=NULL)  { delete rc; rc = NULL; }

    maxdiag = vedt->diagonalLength();
    maxdepth = (int) ceil(sqrt(edt->maximum()));
    maxval = (orig!=NULL ? orig->maximum() : 256);

    setMaxDepth(maxdepth);
    
    invalid = 2;
    repaint();
  }

  virtual void tagsChanged() {
    invalid = 2;
    repaint();
  }

  virtual void invalidateDepth() {
    invalid = 2;
    repaint();
  }

  virtual void paintClient() {
    int x,y;
    unsigned int i;
    Color c;

    if (edt==NULL || orig==NULL) return;
    if (W!=PW || H!=PH) invalid = 1;
    if (invalid) render();

    x = X+W/2;
    y = Y+th+(H-th)/2;
    x -= buf->W / 2;
    y -= buf->H / 2;

    rx = x - X;
    ry = y - (Y+th);
    drawImage(buf,x,y);

    if (!poly.empty()) {

      switch(getToolbarLabel()) {
      case 1:  c = 0xff2020; break;
      case 2:  c = 0x2020ff; break;
      default: c = 0xffff20; break;
      }
      
      for(i=0;i<poly.size()-1;i++) {
	line( (int) (X + rx + poly[i].X * zoom),
	      (int) (Y+th+ry + poly[i].Y * zoom),
	      (int) (X+ rx + poly[i+1].X * zoom),
	      (int) (Y+th+ry + poly[i+1].Y * zoom), c);
      }

    }

    if (orig!=NULL) {
      char z[64];
      
      lcx = X+10;
      lcy = Y+H-65;
      drawCheckBox(lcx+5,lcy,useLight);
      drawString(lcx+20,lcy, Color(0xffff80),"Show Surface Depth");

      vrx = X+10;
      vry = Y+H-45;

      drawString(vrx+5,vry-2,Color(0xffff80),"View Range:");
      drawSlider(vrx+95,vry-1,100,16, getViewRange(), 4, false);

      dsx = X+10;
      dsy = Y+H-24;

      drawString(dsx+5,dsy-2,Color(0xffff80),"Depth:");
      drawSlider(dsx+55,dsy-1,100,16,((float)getDepth())/maxdepth, 4, false);
      sprintf(z,"%.1f mm", getDepth() * orig->dx);
      drawString(dsx+160,dsy-2,Color(0xffff80),z);

      rect(lcx,lcy-5,220,22+21+22,ui.Border,false);
      dsx -= X;
      dsy -= Y+th;
      vrx -= X;
      vry -= Y+th;
      lcx -= X;
      lcy -= Y+th;
    }

  }

  virtual void invalidateRotation() {
    invalid = 2;
    repaint();
  }

  virtual void pressClient(int x,int y,int b) {
    if (dsx>=0 && b==1 && insideExtents(x,y,lcx,lcy-5,220,22+21+22)) {
      float nd;
      if (y > dsy-5) {
	nd = maxdepth * (x - (dsx+55)) / 100.0;
	if (nd < 0.0) nd = 0.0;
        if (nd > maxdepth) nd = maxdepth;
	setDepth((int) rint(nd));
      } else if (y < lcy + 16) {
	useLight = !useLight;
	invalidateRotation();
      } else {
	nd = (x-(vrx+95)) / 100.0;
	if (nd < 0.0) nd = 0.0;
        if (nd > 1.0) nd = 1.0;
	setViewRange(nd);
      }
      return;
    }

    switch(getToolbarMode()) {
    case 0: pressRotate(x,y,b); break;
    case 1: pressPoly(x,y,b); break;
    }
  }

  virtual void releaseClient(int x,int y,int b) {
    if (dsx>=0 && b==1 && insideExtents(x,y,lcx,lcy-5,220,22+21+22)) return;

    switch(getToolbarMode()) {
    case 0: releaseRotate(x,y,b); break;
    case 1: releasePoly(x,y,b); break;
    }
  }

  virtual void dragClient(int x,int y,int b) {
    if (dsx>=0 && b==1 && insideExtents(x,y,lcx,lcy-5,220,22+21+22)) {
      pressClient(x,y,b);
      return;
    }

    switch(getToolbarMode()) {
    case 0: dragRotate(x,y,b); break;
    case 1: dragPoly(x,y,b); break;
    }
  }

  virtual void invalidateViewRange() {
    invalid = 2;
    repaint();
  }

private:
  Volume<int>  *edt, *orig;
  Volume<char> *rtag;
  int maxdepth, maxdiag;
  Image *unz,*buf;
  int PW,PH;
  float zoom;
  int invalid;
  RenderingContext *rc;
  int   maxval;
  int dsx, dsy, vrx, vry, lcx, lcy;
  int rx, ry, ptx, pty, ptz;
  vector<R3> poly;
  bool useLight;

  void pressPoly(int x,int y,int b) {
    if (b==1 && getToolbarMode()==1) {
      R3 p;
      p.set ( (x-rx)/zoom, (y-ry)/zoom, 0.0 );
      poly.push_back(p);
      repaint();
      return;
    }
    if (b==3 && getToolbarMode()==1) {
      poly.clear();
      repaint();
      return;
    }
  }

  void dragPoly(int x,int y,int b) {
    pressPoly(x,y,b);
  }

  void releasePoly(int x,int y,int b) {
    if (b==1) {
      pressPoly(x,y,b);
      tagPolygon();
    }
  }

  void tagPolygon() {
    if (!poly.empty()) {
      Image *tmp;
      unsigned int i,j;
      int w,h,x,y,vi;
      char label;
      Color c;
      
      w = unz->W + 2;
      h = unz->H + 2;
      tmp = new Image(w,h);
      tmp->fill(c = 0xffffff);
      c = 0x808080;

      for(i=0;i<poly.size();i++) {
        j = (i+1)%poly.size();
        tmp->line( 1 + (int) poly[i].X, 1 + (int) poly[i].Y,
                   1 + (int) poly[j].X, 1 + (int) poly[j].Y, c );
      }

      c = 0x000000;
      tmp->floodFill(0,0,c);
      if (tmp->get(w-1,h-1) != 0) tmp->floodFill(w-1,h-1,c);
      if (tmp->get(0,h-1) != 0) tmp->floodFill(0,h-1,c);
      if (tmp->get(w-1,0) != 0) tmp->floodFill(w-1,0,c);

      label = (char) getToolbarLabel();
      for(y=0;y<unz->H;y++)
        for(x=0;x<unz->W;x++)
          if (tmp->get(x+1,y+1) != 0) {
	    vi = rc->ibuf[x + y*rc->W];
	    if (vi>=0 && vi < edt->N) {
	      pushRoundTag(edt->xOf(vi),edt->yOf(vi),edt->zOf(vi),1,label);
	    }
	  }
      
      flushRoundTags();
      delete tmp;
    }
    
    poly.clear();
  }

  void render() {
    Color c1;
    float rw,rh,z0,z1;

    if (edt==NULL || orig==NULL) return;

    if (unz==NULL) {
      unz = new Image(maxdiag,maxdiag);
      rc = new RenderingContext(*unz);
      invalid = 2;
    }

    if (invalid==2) {
      Color tc[256];
      int i;
      for(i=0;i<256;i++) tc[i] = 0;
      tc[1] = 0xff2020;
      tc[2] = 0x2020ff;
      
      c1 = ui.BG;
      unz->fill(c1);
      rc->prepareFirst();
      prepareLUT(maxval,getViewRange());
      if (useLight)
	edt->edtRender2(*unz,rot,*rc,getLUT(),orig,(float) getDepth(),1.0,
		       getRTag(),0.33,tc);
      else
	edt->edtRender(*unz,rot,*rc,getLUT(),orig,(float) getDepth(),1.0,
		       getRTag(),0.33,tc);

      if (hasCursor()) {
	int cx,cy,cz,ix,iy;
	Color green = 0x00ff00;
	getCursor(cx,cy,cz);
	R3 cp,half;
	cp.set(cx,cy,cz);
	half.set(orig->W / 2.0,orig->H / 2.0,orig->D / 2.0);
	cp -= half;
	cp = rot.apply(cp);
	half.set(unz->W /2.0,unz->H/2.0,0.0);
	cp += half;

	ix = (int) cp.X;
	iy = (int) cp.Y;
	if (cp.Z <= rc->zbuf[ix + unz->W * iy] + 2.0)
	  unz->rect(ix-5,iy-5,10,10,green,true);
      }
    }

    if (buf==NULL) invalid = 1;

    if (invalid) {
      rw = W - 20.0;
      rh = H - 20.0 - th;
      z0 = rw / unz->W;
      z1 = rh / unz->H;
      if (z0 < z1) zoom=z0; else zoom=z1;
      zoom *= 1.50;
      
      if (buf!=NULL) { delete buf; buf=NULL; }
      buf = unz->scale(zoom);
    }
    
    invalid = 0;
    PW = W;
    PH = H;
  }

};

class FeatureView : public Window {

 public:
  FeatureView(const char *title,int x,int y,int w,int h) :
    Window(title,x,y,w,h)
  {
    ptex = NULL;
    px=py=pz=0;
    p[0] = new MagPatch(8,8);
    p[1] = new MagPatch(8,8);

    p[2] = new MagPatch(16,16);
    p[3] = new MagPatch(16,16);

    p[4] = new MagPatch(32,32);
    p[5] = new MagPatch(32,32);

    p[6] = new MagPatch(64,64);
    p[7] = new MagPatch(64,64);

    img[0] = new Image(8,8);
    img[1] = new Image(8,8);

    img[2] = new Image(16,16);
    img[3] = new Image(16,16);

    img[4] = new Image(32,32);
    img[5] = new Image(32,32);

    img[6] = new Image(64,64);
    img[7] = new Image(64,64);

    img[8] = new Image(8,8);
    img[9] = new Image(8,8);

    img[10] = new Image(16,16);
    img[11] = new Image(16,16);

    img[12] = new Image(32,32);
    img[13] = new Image(32,32);

    img[14] = new Image(64,64);
    img[15] = new Image(64,64);

  }

  virtual ~FeatureView() {
    int i;
    for(i=0;i<8;i++)
      delete(p[i]);
    for(i=0;i<16;i++)
      delete(img[i]);
  }

  void setPosition(int x,int y,int z) {
    px = x; py = y; pz = z;
    invalidate();
  }

  void setVolume(Volume<int> *pt) {
    ptex = pt;
    otsu = ptex->computeOtsu();
    invalidate();
  }

  void invalidate() {
    compute();
    repaint();
  }

  virtual void paintClient() {
    char z[200];

    if (ptex==NULL) {
      drawString(X+10,Y+th+10,Color(0xffffff),"No volume");
      return;
    }

    sprintf(z,"Position=(%d,%d,%d)",px,py,pz);
    drawString(X+10,Y+th+10,Color(0xffffff),z);

    drawImage(img[0],X+10,Y+th+30);
    drawImage(img[1],X+10+10,Y+th+30);
    drawImage(img[8],X+10+20,Y+th+30);
    drawImage(img[9],X+10+30,Y+th+30);

    drawImage(img[2],X+10,Y+th+40);
    drawImage(img[3],X+10+20,Y+th+40);
    drawImage(img[10],X+10+40,Y+th+40);
    drawImage(img[11],X+10+60,Y+th+40);

    drawImage(img[4],X+10,Y+th+60);
    drawImage(img[5],X+10+40,Y+th+60);
    drawImage(img[12],X+10+80,Y+th+60);
    drawImage(img[13],X+10+120,Y+th+60);

    drawImage(img[6],X+10,Y+th+100);
    drawImage(img[7],X+10+70,Y+th+100);
    drawImage(img[14],X+10+140,Y+th+100);
    drawImage(img[15],X+10+210,Y+th+100);
    
    sprintf(z,"M8: (%.2f / %.2f / %.2f) M16: (%.2f / %.2f / %.2f)",
	    m[0],m[1],m[2],m[3],m[4],m[5]);
    drawString(X+10,Y+th+170,Color(0xffffff),z);

    sprintf(z,"M32: (%.2f / %.2f / %.2f) M64: (%.2f / %.2f / %.2f)",
	    m[6],m[7],m[8],m[9],m[10],m[11]);
    drawString(X+10,Y+th+185,Color(0xffffff),z);

    sprintf(z,"SD8: (%.2f / %.2f / %.2f) SD16: (%.2f / %.2f / %.2f)",
	    m[12],m[13],m[14],m[15],m[16],m[17]);
    drawString(X+10,Y+th+200,Color(0xffffff),z);

    sprintf(z,"SD32: (%.2f / %.2f / %.2f) SD64: (%.2f / %.2f / %.2f)",
	    m[18],m[19],m[20],m[21],m[22],m[23]);
    drawString(X+10,Y+th+215,Color(0xffffff),z);

    sprintf(z,"A8: (%.2f / %.2f / %.2f) A16: (%.2f / %.2f / %.2f)",
	    m[24],m[25],m[26],m[27],m[28],m[29]);
    drawString(X+10,Y+th+230,Color(0xffffff),z);

    sprintf(z,"A32: (%.2f / %.2f / %.2f) A64: (%.2f / %.2f / %.2f)",
	    m[30],m[31],m[32],m[33],m[34],m[35]);
    drawString(X+10,Y+th+245,Color(0xffffff),z);


  }

 private:
  int px,py,pz,otsu;
  Volume<int> *ptex;
  MagPatch *p[8];
  Image *img[16];

  double m[100];

  void compute() {
    int ay,i,j;
    float m1,m2;
    Color c;

    if (ptex!=NULL) {
      // extract
      ay = ptex->H - py;
      p[0]->extract(ptex,px,py,pz);
      p[1]->extract(ptex,px,ay,pz);

      p[2]->extract(ptex,px,py,pz);
      p[3]->extract(ptex,px,ay,pz);

      p[4]->extract(ptex,px,py,pz);
      p[5]->extract(ptex,px,ay,pz);

      p[6]->extract(ptex,px,py,pz);
      p[7]->extract(ptex,px,ay,pz);

      m1 = p[6]->maximum();
      m2 = p[7]->maximum();
      if (m2 > m1) m1 = m2;

      // render
      for(i=0;i<8;i++)
	for(j=0;j<8;j++) {
	  c.gray( (int) (255.0*(p[0]->point(i,j) / m1)) );
	  img[0]->set(i,j,c);
	  c.gray( (int) (255.0*(p[1]->point(i,j) / m1)) );
	  img[1]->set(i,j,c);

	  c.gray( p[0]->point(i,j) > otsu ? 255 : 0 );
	  img[8]->set(i,j,c);
	  c.gray( p[1]->point(i,j) > otsu ? 255 : 0 );
	  img[9]->set(i,j,c);
	}

      for(i=0;i<16;i++)
	for(j=0;j<16;j++) {
	  c.gray( (int) (255.0*(p[2]->point(i,j) / m1)) );
	  img[2]->set(i,j,c);
	  c.gray( (int) (255.0*(p[3]->point(i,j) / m1)) );
	  img[3]->set(i,j,c);

	  c.gray( p[2]->point(i,j) > otsu ? 255 : 0 );
	  img[10]->set(i,j,c);
	  c.gray( p[3]->point(i,j) > otsu ? 255 : 0 );
	  img[11]->set(i,j,c);
	}

      for(i=0;i<32;i++)
	for(j=0;j<32;j++) {
	  c.gray( (int) (255.0*(p[4]->point(i,j) / m1)) );
	  img[4]->set(i,j,c);
	  c.gray( (int) (255.0*(p[5]->point(i,j) / m1)) );
	  img[5]->set(i,j,c);

	  c.gray( p[4]->point(i,j) > otsu ? 255 : 0 );
	  img[12]->set(i,j,c);
	  c.gray( p[5]->point(i,j) > otsu ? 255 : 0 );
	  img[13]->set(i,j,c);
	}

      for(i=0;i<64;i++)
	for(j=0;j<64;j++) {
	  c.gray( (int) (255.0*(p[6]->point(i,j) / m1)) );
	  img[6]->set(i,j,c);
	  c.gray( (int) (255.0*(p[7]->point(i,j) / m1)) );
	  img[7]->set(i,j,c);

	  c.gray( p[6]->point(i,j) > otsu ? 255 : 0 );
	  img[14]->set(i,j,c);
	  c.gray( p[7]->point(i,j) > otsu ? 255 : 0 );
	  img[15]->set(i,j,c);
	}

      m[0] = p[0]->mean();
      m[1] = p[1]->mean();
      m[2] = (m[0] + 1.0) / (m[1] + 1.0);

      m[3] = p[2]->mean();
      m[4] = p[3]->mean();
      m[5] = (m[3] + 1.0) / (m[4] + 1.0);

      m[6] = p[4]->mean();
      m[7] = p[5]->mean();
      m[8] = (m[6] + 1.0) / (m[7] + 1.0);

      m[9] = p[6]->mean();
      m[10] = p[7]->mean();
      m[11] = (m[9] + 1.0) / (m[10] + 1.0);


      m[12] = p[0]->stdev(p[0]->mean());
      m[13] = p[1]->stdev(p[1]->mean());
      m[14] = (m[12] + 1.0) / (m[13] + 1.0);

      m[15] = p[2]->stdev(p[2]->mean());
      m[16] = p[3]->stdev(p[3]->mean());
      m[17] = (m[15] + 1.0) / (m[16] + 1.0);

      m[18] = p[4]->stdev(p[4]->mean());
      m[19] = p[5]->stdev(p[5]->mean());
      m[20] = (m[18] + 1.0) / (m[19] + 1.0);

      m[21] = p[6]->stdev(p[6]->mean());
      m[22] = p[7]->stdev(p[7]->mean());
      m[23] = (m[21] + 1.0) / (m[22] + 1.0);

      m[24] = p[0]->aratio(otsu);
      m[25] = p[1]->aratio(otsu);
      m[26] = (m[24]+1)/(m[25]+1);

      m[27] = p[2]->aratio(otsu);
      m[28] = p[3]->aratio(otsu);
      m[29] = (m[27]+1)/(m[28]+1);

      m[30] = p[4]->aratio(otsu);
      m[31] = p[5]->aratio(otsu);
      m[32] = (m[30]+1)/(m[31]+1);

      m[33] = p[6]->aratio(otsu);
      m[34] = p[7]->aratio(otsu);
      m[35] = (m[33]+1)/(m[34]+1);
    }
  }

};

class PlanarView : public Window, 
		   public LUT, 
		   public AdjustableViewRange,
		   public AdjustableDepth,
		   public BrainToolbarClient,
		   public BrainTagger
{
public:
  PlanarView(const char *title,int x,int y,int w,int h) :
    Window(title,x,y,w,h)
  {
    tex     = NULL;
    unz     = NULL;
    buf     = NULL;
    ptx=pty=ptz=-1;
    PW=PH=-1;
    fv = NULL;
    invalid = true;
    zoom = 1.0;
    setMinimumSize(72,72);
  }

  virtual ~PlanarView() {
    if (unz!=NULL) delete unz;
    if (buf!=NULL) delete buf;
  }

  vector<string> composeHelp() {
    vector<string> h;
    h = keyHelp();
    switch(getToolbarMode()) {
    case 0: h.push_back(string("[Mouse] Left:Feature Position")); break;
    case 1: h.push_back(string("[Mouse] Left:Tag (Contour)")); break;
    }
    return(h);
  }

  void setVolume(Volume<int> *t, int omax, int md, float dx) {
    tex = t;
    maxval = omax;
    maxdepth = md;
    voxelmm = dx;
    invalid = true;

    setMaxDepth(maxdepth);
    repaint();

    if (fv!=NULL)
      fv->setVolume(tex);
  }

  void setFV(FeatureView *fvp) {
    fv = fvp;
  }

  virtual void paintClient() {
    char z[64];
    unsigned int i;
    Color c;

    if (tex==NULL) return;
    if (PW!=W || PH!=H || unz==NULL || buf==NULL) invalid = 2;
    if (invalid) render();

    drawImage(buf,X+10,Y+th+10);

    if (!poly.empty()) {

      switch(getToolbarLabel()) {
      case 1:  c = 0xff2020; break;
      case 2:  c = 0x2020ff; break;
      default: c = 0xffff20; break;
      }

      for(i=0;i<poly.size()-1;i++) {
	line ( (int) (X + 10 + poly[i].X * zoom),
	       (int) (Y+th+10 + poly[i].Y * zoom),
	       (int) (X+ 10 + poly[i+1].X * zoom),
	       (int) (Y+th+10 + poly[i+1].Y * zoom), c);
      }

    }

    
    vrx = X+10;
    vry = Y+H-45;
    
    drawString(vrx+5,vry-2,Color(0xffff80),"View Range:");
    drawSlider(vrx+95,vry-1,100,16, getViewRange(), 4, false);
    
    dsx = X+10;
    dsy = Y+H-24;
    
    drawString(dsx+5,dsy-2,Color(0xffff80),"Depth:");
    drawSlider(dsx+55,dsy-1,100,16,((float)getDepth())/maxdepth, 4, false);
    sprintf(z,"%.1f mm", getDepth() * voxelmm);
    drawString(dsx+160,dsy-2,Color(0xffff80),z);
    
    rect(vrx,vry-5,220,22+21,ui.Border,false);
    dsx -= X;
    dsy -= Y+th;
    vrx -= X;
    vry -= Y+th;

    //rect(X+10,Y+th+vry-5,220,22+21,0xff0000,false);
    //rect(X+10,Y+th+10,buf->W,buf->H,0x00ff00,false);

  }

  virtual void tagsChanged() {
    invalid = 2;
    repaint();
  }

  virtual void invalidateViewRange() {
    invalid = 2;
    repaint();
  }

  virtual void invalidateDepth() {
    invalid = 2;
    repaint();
  }

  virtual void pressClient(int x,int y,int b) {

    if (dsx>=0 && b==1 && insideExtents(x,y,vrx,vry-5,220,22+21)) {
      float nd;
      if (y > dsy-5) {
	nd = maxdepth * (x - (dsx+55)) / 100.0;
	if (nd < 0.0) nd = 0.0;
        if (nd > maxdepth) nd = maxdepth;
	setDepth((int) rint(nd));
      } else {
	nd = (x-(vrx+95)) / 100.0;
	if (nd < 0.0) nd = 0.0;
        if (nd > 1.0) nd = 1.0;
	setViewRange(nd);
      }
      return;
    }
    
    if (unz==NULL || buf==NULL) return;

    if (b==1 && getToolbarMode()==1 && insideExtents(x,y,10,10,buf->W,buf->H)) {
      R3 p;
      p.set( (x-10.0)/zoom, (y-10.0)/zoom, 0.0 );
      poly.push_back(p);
      repaint();
      return;
    }

    if (b==1 && getToolbarMode()==0 && insideExtents(x,y,10,10,buf->W,buf->H)) {
      int rx,ry,rz;
      rx = (int) ((x - 10.0) / zoom);
      ry = (int) ((y - 10.0) / zoom);
      rz = getDepth();
      if (fv!=NULL)
	fv->setPosition(rx,ry,rz);
      return;
    }

    if (b==3 && getToolbarMode()==1) {
      poly.clear();
      repaint();
      return;
    }
  }

  virtual void dragClient(int x,int y,int b) {
    pressClient(x,y,b);
  }

  virtual void releaseClient(int x,int y,int b) {
    if (getToolbarMode()==1) {
      pressClient(x,y,b);
      tagPolygon();
    }
  }

private:
  Volume<int> *tex;
  Image *unz, *buf;
  int PW,PH,vrx,vry,dsx,dsy,ptx,pty,ptz;
  int invalid, maxval, maxdepth;
  float zoom, voxelmm;
  vector<R3> poly;
  FeatureView *fv;

  void tagPolygon() {

    if (!poly.empty()) {
      Image *tmp;
      unsigned int i,j;
      int w,h,x,y,z;
      char label;
      Color c;

      w = tex->W + 2;
      h = tex->H + 2;
      tmp = new Image(w,h);
      tmp->fill(c = 0xffffff);
      c = 0x808080;

      for(i=0;i<poly.size();i++) {
	j = (i+1)%poly.size();
	tmp->line( 1 + (int) poly[i].X, 1 + (int) poly[i].Y,
		   1 + (int) poly[j].X, 1 + (int) poly[j].Y, c );
      }
      
      c = 0x000000;
      tmp->floodFill(0,0,c);
      if (tmp->get(w-1,h-1) != 0) tmp->floodFill(w-1,h-1,c);
      if (tmp->get(0,h-1) != 0) tmp->floodFill(0,h-1,c);
      if (tmp->get(w-1,0) != 0) tmp->floodFill(w-1,0,c);

      freezeTagger();

      z = getDepth();
      label = (char) getToolbarLabel();
      for(y=0;y<tex->H;y++)
	for(x=0;x<tex->W;x++)
	  if (tmp->get(x+1,y+1) != 0)
	    planarTag(x,y,z,1,label);

      delete tmp;
      thawTagger();
    }    

    poly.clear();
  }

  void render() {
    int i,j,d;
    int *lut;
    Color c,cmix,blue,red;
    float caw,cah,uw,uh,z0,z1;
    Volume<char> *ptag;

    if (tex==NULL) return;
    if (unz==NULL) {
      unz = new Image(tex->W,tex->H);
      invalid = 2;
    }

    if (invalid == 2) {
     
      red  = 0xff2020;
      blue = 0x2020ff;
      ptag = getPTag();

      prepareLUT(maxval,getViewRange());
      lut = getLUT();
      d = getDepth();
      if (d >= tex->D) d = tex->D-1;
      for(j=0;j<tex->H;j++)
	for(i=0;i<tex->W;i++) {
	  c = lut[ tex->voxel(i,j,d) ];
	  if (ptag!=NULL) {
	    switch(ptag->voxel(i,j,d)) {
	    case 1: c.mix(red,0.25); break;
	    case 2: c.mix(blue,0.25); break;
	    }
	  }
	  unz->set(i,j,c);
	}


      if (hasCursor()) {
	int cx,cy,cz,ix,iy;
	Color green = 0x00ff00;
	getCursor(cx,cy,cz);

	getXY(cx,cy,cz,getDepth(),ix,iy);
	if (ix >= 0)
	  unz->rect(ix-5,iy-5,10,10,green,true);
      }

    }

    if (buf==NULL) invalid = 1;

    if (invalid) {
      caw = W - 20.0;
      cah = H - th - 65.0;
      uw = (float) (unz->W);
      uh = (float) (unz->H);
      z0 = caw / uw;
      z1 = cah / uh;
      if (z0 < z1) zoom = z0; else zoom = z1;
      if (buf!=NULL) { delete buf; buf = NULL; }
      buf = unz->scale(zoom);
    }

    invalid = 0;
    PW = W;
    PH = H;
  }
};

class Task {
 public:
  char *Title, *File;
  int   ival[10];
  float fval[10];
  Timestamp start, finish;

  Task(const char *title) { Title = strdup(title); File = NULL; }  
  virtual ~Task() { free(Title); if (File!=NULL) free(File); }
};

#endif
