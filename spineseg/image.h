/*

    SpineSeg
    http://www.lni.hc.unicamp.br/app/spineseg
    https://github.com/fbergo/braintools
    Copyright (C) 2009-2017 Felipe P.G. Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

// classes here: Color Image

#ifndef IMAGE_H
#define IMAGE_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <queue>
#include <map>
#include "tokenizer.h"

using namespace std;

//! 3-component color (RGB, YCbCr)
class Color {
 public:
  uint8_t R,G,B;

  //! Constructor, creates black color
  Color() { R=G=B=0; }

  //! Constructor, creates color with (R,G,B) = (a,b,c)
  Color(int a,int b,int c) { R=(uint8_t)a; G=(uint8_t)b; B=(uint8_t)c; }

  //! Constructor, creates color with (R,G,B) = (a,b,c)
  Color(uint8_t a,uint8_t b,uint8_t c) { R=a; G=b; B=c; }

  //! Copy constructor
  Color(const Color &c) { R=c.R; G=c.G; B=c.B; }

  //! Constructor, creates color from integer representation.
  //! Integer representation: bits 0-7: blue; bits 8-15: green;
  //! bits 16-23: red; bits 24-31: ignored.
  Color(int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); }

  //! Comparison operator
  bool operator==(Color &c) { return(c.R==this->R && c.G==this->G && c.B==this->B); }

  //! Comparison operator
  bool operator!=(Color &c) { return(c.R!=this->R || c.G!=this->G || c.B!=this->B); }

  //! Assignment operator
  Color & operator=(const Color &c) { R=c.R; G=c.G; B=c.B; return(*this); }

  //! Assignment operator, from integer representation. See \ref Color(int c)
  Color & operator=(const int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); return(*this); }

  //! Assignment operator, from integer representation. See \ref Color(int c)
  Color & operator=(const unsigned int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); return(*this); }

  //! Returns the integer representation of this color. See \ref Color(int c)
  int toInt() { return( ( ((int)R)<<16)|(((int)G)<<8)|((int)B) ); }

  //! Lightens or darkens this color by factor x. The factor multiplies the
  //! luminance component in the YCbCr representation of this color.
  Color & operator*=(float x) {
    float fr;
    rgb2ycbcr();
    fr = R*x; if (fr > 255.0) fr=255.0;
    R = (uint8_t) fr;
    ycbcr2rgb();
    return(*this);
  }

  //! Luminance multiplication operator
  Color operator*(float x) {
    Color c;
    c = (*this);
    c *= x;
    return(c);
  }

  //! sets a gray color, with (R,G,B) = (c,c,c), returns a reference
  //! to this color.
  Color & gray(int c) { R = G = B = (uint8_t) c; return(*this); }

  //! Mixes color src in this color, with proportion srcamount of src
  //! and (1-srcamount) of this color. Overwrites this color with
  //! the result and returns a reference to this color.
  Color & mix(Color &src, float srcamount) {
    R = (uint8_t) ((1.0 - srcamount)*(this->R) + (srcamount*(src.R)));
    G = (uint8_t) ((1.0 - srcamount)*(this->G) + (srcamount*(src.G)));
    B = (uint8_t) ((1.0 - srcamount)*(this->B) + (srcamount*(src.B)));
    return(*this);
  }

  //! Converts an RGB color triplet to YCbCr
  void rgb2ycbcr() {
    float y,cb,cr;
    
    y=0.257*(float)R+0.504*(float)G+0.098*(float)B+16.0;
    cb=-0.148*(float)R-0.291*(float)G+0.439*(float)B+128.0;
    cr=0.439*(float)R-0.368*(float)G-0.071*(float)B+128.0;
    R = (uint8_t) y; G = (uint8_t) cb; B = (uint8_t) cr;
  }

  //! Converts an YCbCr color triplet to RGB
  void ycbcr2rgb() {
    float r,g,b;

    r=1.164*((float)R-16.0)+1.596*((float)B-128.0);
    g=1.164*((float)R-16.0)-0.813*((float)B-128.0)-0.392*((float)G-128.0);
    b=1.164*((float)R-16.0)+2.017*((float)G-128.0);

    if (r<0.0) r=0.0; if (g<0.0) g=0.0; if (b<0.0) b=0.0;
    if (r>255.0) r=255.0; if (g>255.0) g=255.0; if (b>255.0) b=255.0;
    R=(uint8_t)r; G=(uint8_t)g; B=(uint8_t)b;
  }
};

//! 2D RGB Image
class Image {
 public:
  int W /** Image width */,H /** Image height */,N /** Pixel count */;

  //! Constructor, creates a w x h image filled with black
  Image(int w,int h) {
    Color black;
    data = new uint8_t[w*h*3];
    W=w;
    H=h;
    N = w*h;
    fill(black);
  }

  //! Copy constructor
  Image(Image &img) {
    W=img.W;
    H=img.H;
    N=img.N;
    data = new uint8_t[W*H*3];
    memcpy(this->data, img.data, 3*N);
  }

  //! Construtor, creates an image from a P5/P6 PPM file
  Image(const char *filename) {
    ifstream f;
    char tmp[61],*p;
    int i;
    data = 0;
    W=H=N=0;
    f.open(filename);
    if (f.good()) {
      f.getline(tmp,60);
      if (!strcmp(tmp,"P6")) {
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	p = strchr(tmp,' '); if (!p) p = strchr(tmp,'\t');
	*p = 0; W = atoi(tmp); H = atoi(p+1);
	N = W*H;
	data = new uint8_t[W*H*3];
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	for(i=0;i<H;i++)
	  f.read((char *) (&data[3*W*i]),3*W);
      }
      if (!strcmp(tmp,"P5")) {
	uint8_t *tgray;
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	p = strchr(tmp,' '); if (!p) p = strchr(tmp,'\t');
	*p = 0; W = atoi(tmp); H = atoi(p+1);
	N = W*H;
	tgray = new uint8_t[W*H];
	data = new uint8_t[W*H*3];
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	for(i=0;i<H;i++)
	  f.read((char *) (&tgray[W*i]),W);
	for(i=0;i<N;i++)
	  data[3*i] = data[3*i+1] = data[3*i+2] = tgray[i];
	delete tgray;
      }
      f.close();
    }
  }

  //! Constructor, creates an image from an included XPM image (xpm).
  //! Transparent pixels are replaced by the \ref Color transp.
  Image(char **xpm, Color &transp) {
    readXPM(xpm,transp);
  }

  //! Destructor
  virtual ~Image() {
    if (data!=0) delete data;
  }

  //! Saves this image to a P6 PPM file
  void writeP6(const char *filename) {
    ofstream f;
    int i;
    f.open(filename,ofstream::binary);
    if (!f.good()) return;

    f << "P6\n" << W << " " << H << "\n255\n";
    for(i=0;i<H;i++)
      f.write( (char *) (&data[i*W*3]), W*3);
    f.close();
  }

  //! Fills all pixels with \ref Color c
  void fill(Color &c) {
    int i,p;
    for(i=0,p=0;i<N;i++) {
      data[p++] = c.R;
      data[p++] = c.G;
      data[p++] = c.B;
    }
  }

  //! Sets pixel (x,y) to \ref Color c
  virtual void set(const int x,const int y,Color &c) {
    if (valid(x,y)) {
      data[3*(x+y*W) ] = c.R;
      data[3*(x+y*W)+1 ] = c.G;
      data[3*(x+y*W)+2 ] = c.B;
    }
  }

  //! Sets pixel (x,y) to color c, given as an integer representation
  //! (see \ref Color::Color(int c))
  void set(const int x,const int y,const int c) {
    if (valid(x,y)) {
      Color d(c);
      data[3*(x+y*W) ] = d.R;
      data[3*(x+y*W)+1 ] = d.G;
      data[3*(x+y*W)+2 ] = d.B;
    }
  }

  //! Returns the color of pixel (x,y) in integer representation
  //! (see \ref Color::Color(int c))
  virtual int get(const int x,const int y) {
    if (valid(x,y)) {
      Color c;
      c.R = data[3*(x+y*W) ];
      c.G = data[3*(x+y*W) + 1];
      c.B = data[3*(x+y*W) + 2];
      return(c.toInt());
    } else return 0;
  }

  //! Returns true if (x,y) is in the image domain, false otherwise
  bool valid(const int x,const int y) { return(x>=0 && x<W && y>=0 && y<H); }

  //! Flood fills from point (x,y) with \ref Color c. Flood fill is performed
  //! with 4-neighbors adjacency. Any color different from the previous color
  //! of (x,y) halts the flood. It does nothing if (x,y) has \ref Color c.
  void floodFill(int x,int y,Color &c) {
    queue<int> fifo;
    bool *done;
    int bg,px,py,pi;
    
    bg = get(x,y);
    if (c.toInt() == bg) return;

    done = new bool[W*H];
    for(pi=0;pi<W*H;pi++) done[pi] = false;

    fifo.push(x+y*W);    
    done[x+y*W] = true;
    while(!fifo.empty()) {
      pi = fifo.front();
      done[pi] = true;
      fifo.pop();

      px = pi%W;
      py = pi/W;
      if (valid(px,py)) {
	set(px,py,c);

	if (valid(px-1,py) && get(px-1,py) == bg && !done[(px-1)+(py)*W]) {
	  fifo.push((px-1)+(py)*W);
	  done[(px-1)+(py)*W] = true;
	}
	if (valid(px+1,py) && get(px+1,py) == bg && !done[(px+1)+(py)*W]) {
	  fifo.push((px+1)+(py)*W);
	  done[(px+1)+(py)*W] = true;
	}
	if (valid(px,py-1) && get(px,py-1) == bg && !done[(px)+(py-1)*W]) {
	  fifo.push((px)+(py-1)*W);
	  done[(px)+(py-1)*W] = true;
	}
	if (valid(px,py+1) && get(px,py+1) == bg && !done[(px)+(py+1)*W]) {
	  fifo.push((px)+(py+1)*W);
	  done[(px)+(py+1)*W] = true;
	}
      }
    }
  }
  
  //! Alpha bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
  //! src to position (x,y) of this image. Pixels of color trans in src are
  //! considered transparent and are not copied. If w or h are negative,
  //! the width and/or height of src are used.
  void ablit(Image &src, Color &trans,int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;
    uint8_t r,g,b,ar,ag,ab;

    if (w<0) w=src.W;
    if (h<0) h=src.H;

    ar = trans.R;
    ag = trans.G;
    ab = trans.B;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+src.W*j);
	  r = src.data[s];
	  g = src.data[s+1];
	  b = src.data[s+2];
	  if (r!=ar || g!=ag || b!=ab) {
	    this->data[d] = r;
	    this->data[d+1] = g;
	    this->data[d+2] = b;
	  }
	} 
  }

  //! Alpha bit block increment: similar to \ref ablit, but instead of
  //! copying the src image, uses it as a mask, and increments R,G and B
  //! of the pixels on this image where a non-transparent pixel of src
  //! would be painted.
  void ablinc(Image &src, Color &trans,int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;
    uint8_t r,g,b,ar,ag,ab;

    if (w<0) w=src.W;
    if (h<0) h=src.H;

    ar = trans.R;
    ag = trans.G;
    ab = trans.B;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+src.W*j);
	  r = src.data[s];
	  g = src.data[s+1];
	  b = src.data[s+2];
	  if (r!=ar || g!=ag || b!=ab) {
	    this->data[d]++;
	    this->data[d+1]++;
	    this->data[d+2]++;
	  }
	} 
  }

  //! Bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
  //! src to position (x,y) of this image. If w or h are negative,
  //! the width and/or height of src are used.
  void blit(Image &src, int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;

    if (w<0) w=src.W;
    if (h<0) h=src.H;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j) && src.valid(i,j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+(src.W)*j);
	  this->data[d] = src.data[s];
	  this->data[d+1] = src.data[s+1];
	  this->data[d+2] = src.data[s+2];
	} 
  }

  //! Bit block transfer: copies rectangle (sx,sy)-(sx+w-1,sy+h-1) of Image
  //! src to position (dx,dy) of this image. If w or h are negative,
  //! the width and/or height of src are used.
  void blit(Image &src, int sx, int sy, int dx,int dy,int w=-1,int h=-1) {

    int i,j,s,d;

    if (sx + w > src.W) w = src.W-sx;
    if (sy + h > src.H) h = src.H-sy;
    if (w<0) w=src.W;
    if (h<0) h=src.H;
    if (dx + w > W) w = W-dx;
    if (dy + h > H) h = H-dy;
    if (w < 0 || h < 0) return;

    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(dx+i,dy+j) && src.valid(sx+i,sy+j)) {
	  d = 3*(dx+i+W*(dy+j));
	  s = 3*(sx+i+(src.W)*(sy+j));
	  this->data[d] = src.data[s];
	  this->data[d+1] = src.data[s+1];
	  this->data[d+2] = src.data[s+2];
	} 
  }
  
  //! Returns a pointer to the RGB buffer
  unsigned char *getBuffer() {
    return((unsigned char *)data);
  }

  //! Draws a filled translucent rectangle with top left corner (x,y), 
  //! size (w,h), \ref Color src and opacity srcamount
  void blendbox(int x, int y, int w, int h, Color &src, float srcamount) {
    int i,j,p;
    Color c;
    if (x >= W) return; if (y >= H) return;
    if (x<0) { w+=x; x=0; } if (y<0) { h+=y; y=0; }
    if (x+w >= W) w=W-x-1; if (y+h >= H) h=H-y-1;
    if (w<0 || h<0) return;
    p=3*(x+y*W);
    for(i=0;i<h;i++) {
      for(j=0;j<w;j++) {
	c.R = data[p+3*j ];
	c.G = data[p+3*j+1 ];
	c.B = data[p+3*j+2 ];
	c.mix(src,srcamount);
	data[p+3*j ] = c.R;
	data[p+3*j+1 ] = c.G;
	data[p+3*j+2 ] = c.B;
      }
      p += 3*W;
    }    
  }

  //! Shades a filled rectangle with top left corner (x,y) and size
  //! (w,h) by multiplying the YCbCr luminance of each pixel by factor
  void shadebox(int x, int y, int w, int h, float factor) {
    int i,j,p;
    Color c;
    if (x >= W) return; if (y >= H) return;
    if (x<0) { w+=x; x=0; } if (y<0) { h+=y; y=0; }
    if (x+w >= W) w=W-x-1; if (y+h >= H) h=H-y-1;
    if (w<0 || h<0) return;
    p=3*(x+y*W);
    for(i=0;i<h;i++) {
      for(j=0;j<w;j++) {
	c.R = data[p+3*j ];
	c.G = data[p+3*j+1 ];
	c.B = data[p+3*j+2 ];
	c *= factor;
	data[p+3*j ] = c.R;
	data[p+3*j+1 ] = c.G;
	data[p+3*j+2 ] = c.B;
      }
      p += 3*W;
    }    
  }

  //! Draws a rectangle with top left corner (x,y), size (w,h) and
  //! \ref Color c. If fill is true, the rectangle is filled.
  void rect(int x, int y, int w, int h, Color &c, bool fill=true) {
    int i,j,p;
    if (!fill) {
      line(x,y,x+w-1,y,c);
      line(x,y+h-1,x+w-1,y+h-1,c);
      line(x,y,x,y+h-1,c);
      line(x+w-1,y,x+w-1,y+h-1,c);
    } else {
      if (x >= W) return; if (y >= H) return;
      if (x<0) { w+=x; x=0; } if (y<0) { h+=y; y=0; }
      if (x+w >= W) w=W-x-1; if (y+h >= H) h=H-y-1;
      if (w<0 || h<0) return;
      p=3*(x+y*W);
      for(i=0;i<h;i++) {
	for(j=0;j<w;j++) {
	  data[p+3*j ] = c.R;
	  data[p+3*j+1 ] = c.G;
	  data[p+3*j+2 ] = c.B;
	}
	p += 3*W;
      }
    }
  }

  //! Draws a line segment between points (x1,y1) and (x2,y2), with
  //! \ref Color c.
  void line(int x1,int y1,int x2,int y2,Color &c) {
    int x, y;
    int dy = y2 - y1;
    int dx = x2 - x1;
    int G, DeltaG1, DeltaG2;
    int swap;
    int inc = 1;

    if (x1>=0 && y1>=0 && x1<W && y1<H)
      set(x1,y1,c);
  
    if (abs(dy) < abs(dx)) {
      /* -1 < slope < 1 */
      if (dx < 0) {
	dx = -dx; dy = -dy;      
	swap = y2; y2 = y1; y1 = swap;	
	swap = x2; x2 = x1; x1 = swap;
      }
      
      if (dy < 0) { dy = -dy; inc = -1; }
      
      y = y1; x = x1 + 1;      
      G = 2 * dy - dx; DeltaG1 = 2 * (dy - dx); DeltaG2 = 2 * dy;
    
      while (x <= x2) {
	if (G > 0) { G += DeltaG1; y += inc; } 
	else G += DeltaG2;	
	if (x>=0 && y>=0 && x<W && y<H) set(x,y,c);
	x++;
      }
    } else {
      /* slope < -1 or slope > 1 */
      if (dy < 0) { 
	dx = -dx; dy = -dy;
	swap = y2; y2 = y1; y1 = swap;	
	swap = x2; x2 = x1; x1 = swap;
      }      
      if (dx < 0) { dx = -dx; inc = -1; }
      
      x = x1; y = y1 + 1;      
      G = 2 * dx - dy; DeltaG1 = 2 * (dx - dy);
      DeltaG2 = 2 * dx;
      
      while (y <= y2) {
	if (G > 0) { G += DeltaG1; x += inc; } 
	else G += DeltaG2;
	
	if (x>=0 && y>=0 && x<W && y<H) set(x,y,c);
	y++;
      }
    }
  }

  //! Scales this image by factor, and returns the new scaled image result. 
  //! It does not modify this image.
  Image *scale(float factor) {
      int nw,nh,ow,oh;
      Image *dest;
      float x1,x2,y1,y2,fi,fj,dx,dy,fr,fg,fb,di;
      int i,j,k,a,b;
      uint8_t R,G,B;
      
      uint8_t *lookup[3];
      int *area;
      int  count;

      if (factor > 1.0) return(scaleUp(factor));

      nw = (int) ((float)(W) * factor);
      nh = (int) ((float)(H) * factor);
      ow = W;
      oh = H;
      if (nw<=0 || nh<=0) return 0;

      dest = new Image(nw,nh);
      lookup[0] = new uint8_t[ow*oh];
      lookup[1] = new uint8_t[ow*oh];
      lookup[2] = new uint8_t[ow*oh];
      area   = new int[ow*oh];
      if (!lookup[0] || !area) {
	delete dest;
	return 0;
      }

      for(j=0;j<nh;j++)
	for(i=0;i<nw;i++) {
	  
	  fi = (float) i;
	  fj = (float) j;
	  x1 = fi / factor;
	  x2 = (fi+1.0) / factor;
	  y1 = fj / factor;
	  y2 = (fj+1.0) / factor;
	  
	  di = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	  
	  count = 0;
	  for(b=(int)y1;b<=(int)y2;b++)
	    if (b>=0 && b<oh)
	      for(a=(int)x1;a<=(int)x2;a++)
		if (a>=0 && a<ow) {
		  k = 3*(a+b*ow);
		  lookup[0][count]=data[k];
		  lookup[1][count]=data[k+1];
		  lookup[2][count]=data[k+2];
		  dx = (a-x1);
		  dy = (b-y1);
		  area[count] = (int) (100.0*(di - sqrt(dx*dx+dy*dy)));
		  count++;
		}
	  
	  a = 0;
	  for(b=0;b<count;b++)
	    a+=area[b];
	  
	  fb=fg=fr=0.0;
	  for(b=0;b<count;b++) {
	    fr += ((float)(area[b])) * ((float)(lookup[0][b]));
	    fg += ((float)(area[b])) * ((float)(lookup[1][b]));
	    fb += ((float)(area[b])) * ((float)(lookup[2][b]));
	  }
	  fr /= (float) a;
	  fg /= (float) a;
	  fb /= (float) a;

	  R = (uint8_t) fr;
	  G = (uint8_t) fg;
	  B = (uint8_t) fb;

	  k = 3*(i+j*nw);
	  dest->data[k] = R;
	  dest->data[k+1] = G;
	  dest->data[k+2] = B;
	}

      delete area;
      delete lookup[0];
      delete lookup[1];
      delete lookup[2];
      return dest;
  }

 private:
  uint8_t *data;

  Image *scaleUp(float factor) {
    int nw,nh,ow,oh;
    float fi,fj;
    int i,j,x,y;
    uint8_t *s,*d;
    Image *dest;

    nw = (int) ((float)(W) * factor);
    nh = (int) ((float)(H) * factor);
    ow = W;
    oh = H;

    dest = new Image(nw,nh);
    if (!dest) return 0;

    for(j=0;j<nh;j++)
      for(i=0;i<nw;i++) {

	fi = (float) i;
	fj = (float) j;

	x = (int) (fi / factor);
	y = (int) (fj / factor);

	if (x>=0 && x<ow && y>=0 && y<oh) {
	  d = &(dest->data[3*(i+nw*j)]);
	  s = &(this->data[3*(x+ow*y)]);
	  *(d++) = *(s++);
	  *(d++) = *(s++);
	  *d = *s;
	}
      }
    return dest;
  }

  void readXPM(char **xpm,Color &transp) {
    int i,j,k,line,npix;
    Tokenizer t;

    map<string, Color> colortable;
    map<string, Color>::iterator cti;

    char *key, *p;
    Color c;

    int ncolors, cpp;

    W=H=N=0;
    data=NULL;

    t.setString(xpm[0]);
    if (t.countTokens()!=4) return;
    
    W       = t.nextInt();
    H       = t.nextInt();
    ncolors = t.nextInt();
    cpp     = t.nextInt();

    N = W*H;
    data = new uint8_t[W*H*3];
    c = 0;
    fill(c);

    line = 1;
    key = (char *) malloc(cpp+1);
    key[cpp]=0;

    for(i=0;i<ncolors;i++,line++) {

      for(j=0;j<cpp;j++) key[j] = xpm[line][j];

      t.setString(xpm[line]+cpp);
      p = t.nextToken();
      if (p==NULL) continue;
      c = 0;
      if (!strcasecmp(p,"c")) {
	p = t.nextToken();
	if (!strcasecmp(p,"none"))
	  c = transp;
	else if (p[0]=='#') {	  
	  c = (unsigned int) strtol(&p[1],NULL,16);
	}
      }
      string s(key);
      colortable.insert( make_pair(s,c) );
    }

    for(j=0;j<H;j++) {
      npix = strlen(xpm[line+j]) / cpp;
      if (npix > W) npix = W;
      for(i=0;i<npix;i++) {
	
	for(k=0;k<cpp;k++)
	  key[k] = xpm[line+j][cpp*i+k];

	cti = colortable.find(key);
	if (cti == colortable.end()) {
	  c = 0;
	} else
	  c = cti->second;

	set(i,j,c);
      }
    }

    free(key);
    colortable.clear();
  }

};

#endif
