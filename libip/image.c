/*

   LIBIP - Image Processing Library
   (C) 2002-2013
   
   Felipe P.G. Bergo <fbergo at gmail dot com>
   Alexandre X. Falcao <afalcao at ic dot unicamp dot br>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include "ip_image.h"
#include "ip_color.h"
#include "ip_optlib.h"
#include "ip_common.h"
#include "ip_geom.h"
#include "ip_mem.h"
#include "ip_volume.h"

CImg *CImgNew(int width, int height) {
  CImg *x;
  x=(CImg *)MemAlloc(sizeof(CImg));
  x->W = width;
  x->H = height;
  x->rowlen = 3 * width;
  x->val=(component *)MemAlloc(3*width*height*sizeof(component));
  MemSet(x->val,0,x->H * x->rowlen);
  return x;
}

CImg *CImgDup(CImg *src) {
  CImg *d;
  d = CImgNew(src->W,src->H);
  if (!d) return 0;
  MemCpy(d->val,src->val,d->H * d->rowlen);
  return d;
}

void  CImgDestroy(CImg *x) {
  if (x) {
    if (x->val)
      MemFree(x->val);
    MemFree(x);
  }
}

void CImgFill(CImg *x, int color) {

  RgbSet(x->val, color, x->W * x->H);

}

void CImgSet(CImg *z, int x,int y, int color) {
  component r,g,b, *p;

  r=t0(color);
  g=t1(color);
  b=t2(color);
  
  p = z->val + (z->rowlen * y + 3 * x);
  *(p++) = r;
  *(p++) = g;
  *p = b;
}

int CImgGet(CImg *z, int x,int y) {
  component r,g,b, *p;

  p = z->val + (z->rowlen * y + 3 * x);
  r = *(p++);
  g = *(p++);
  b = *p;

  return(triplet(r,g,b));
}

void CImgFullCopy(CImg *dest, CImg *src) {
  if (dest->W != src->W || dest->H != src->H)
    return;
  MemCpy(dest->val,src->val,src->rowlen * src->H);
}

int CImgMinSize(CImg *x, int w, int h) {
  if (!x) return 0;
  return (x->W >= w && x->H >= h);
}

int CImgSizeEq(CImg *x,int w,int h) {
  if (!x) return 0;
  return(x->W == w && x->H == h);
}

int CImgCompatible(CImg *a, CImg *b) {
  if (a==0 || b==0) return 0;
  return (a->W == b->W && a->H == b->H);
}

void CImgDrawRect(CImg *z, int x, int y, int w, int h, int color) {
  CImgDrawLine(z,x,y,x+w-1,y,color);
  CImgDrawLine(z,x,y+h-1,x+w-1,y+h-1,color);
  CImgDrawLine(z,x,y,x,y+h-1,color);
  CImgDrawLine(z,x+w-1,y,x+w-1,y+h-1,color);
}

void CImgFillRect(CImg *z, int x, int y, int w, int h, int color) {
  if (x >= z->W) return;
  if (y >= z->H) return;

  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x+w >= z->W) w=z->W-x-1;
  if (y+h >= z->H) h=z->H-y-1;
  if (w<0 || h<0) return;

  RgbRect(z->val, x, y, w, h, color, z->rowlen);
}

void        CImgDrawBevelBox(CImg *z, int x,int y,int w,int h, int color, 
			     int border, int up)
{
  int c[4];

  if (w<4 || h<4) return;

  if (up) {
    c[0] = darker(color, 4);
    c[1] = darker(color, 2);
    c[2] = lighter(color, 6);
    c[3] = lighter(color, 3);
  } else {
    c[2] = darker(color, 4);
    c[3] = darker(color, 2);
    c[0] = lighter(color, 6);
    c[1] = lighter(color, 3);
  }

  CImgFillRect(z,x,y,w,h,color);
  CImgDrawRect(z,x,y,w,h,border);

  CImgDrawLine(z,x+1,y+1,x+w-2,y+1, c[2]);
  CImgDrawLine(z,x+1,y+2,x+w-3,y+2, c[3]);
  CImgDrawLine(z,x+1,y+1,x+1,y+h-2, c[2]);
  CImgDrawLine(z,x+2,y+2,x+2,y+h-3, c[3]);
  CImgDrawLine(z,x+1,y+h-2,x+w-2,y+h-2, c[0]);
  CImgDrawLine(z,x+2,y+h-3,x+w-3,y+h-3, c[1]);
  CImgDrawLine(z,x+w-2,y+1,x+w-2,y+h-2, c[0]);
  CImgDrawLine(z,x+w-3,y+2,x+w-3,y+h-3, c[1]);
}

void        CImgDrawSButton(CImg *z, int x,int y,int w,int h, int bg,
			    int border,int textcolor, char *text,
			    SFont *font, int up)
{
  int tx,ty;
  CImgDrawBevelBox(z,x,y,w,h,bg,border,up);
  tx = SFontLen(font,text);
  tx = x + ((w-tx)/2);
  ty = SFontH(font,text);
  ty = y + ((h-ty)/2);
  CImgDrawSText(z,font,tx,ty,textcolor,text);
}

void        CImgDrawButton(CImg *z, int x,int y,int w,int h, int bg,
			   int border,int textcolor, char *text,
			   Font *font, int up)
{
  int tx,ty;
  CImgDrawBevelBox(z,x,y,w,h,bg,border,up);
  tx = strlen(text) * font->cw;
  tx = x + ((w-tx)/2);
  ty = y + ((h-font->H)/2);
  CImgDrawText(z,font,tx,ty,textcolor,text);
}

// http://trident.mcs.kent.edu/~rduckwor/SV/ASSGN/HW2/functions.txt
// (a versao do Foley nao trata todos os casos)
void CImgDrawLine(CImg *z, int x1, int y1, int x2, int y2, int color) 
{
  component triplet[3], *p=0;
  int x, y;
  int dy = y2 - y1;
  int dx = x2 - x1;
  int G, DeltaG1, DeltaG2, minG, maxG;	
  int swap;
  int inc = 1;

  triplet[0] = t0(color);
  triplet[1] = t1(color);
  triplet[2] = t2(color);

#define SetPixel(a,b,c) { p=a->val+a->rowlen*c+3*b; *(p++)=triplet[0]; *(p++)=triplet[1]; *p=triplet[2]; }

  if (x1>=0 && y1>=0 && x1<z->W && y1<z->H)
    SetPixel(z, x1, y1);
  
  if (abs(dy) < abs(dx)) {
    /* -1 < slope < 1 */
    if (dx < 0) {
      dx = -dx;
      dy = -dy;
      
      swap = y2;
      y2 = y1;
      y1 = swap;
      
      swap = x2;
      x2 = x1;
      x1 = swap;
    }
      
    if (dy < 0) {
      dy = -dy;
      inc = -1;
    }
      
    y = y1;
    x = x1 + 1;
      
    G = 2 * dy - dx;
    DeltaG1 = 2 * (dy - dx);
    DeltaG2 = 2 * dy;
    
    while (x <= x2) {
      if (G > 0) {
	G += DeltaG1;
	y += inc;
      } else
	G += DeltaG2;

      if (x>=0 && y>=0 && x<z->W && y<z->H)
	SetPixel(z, x, y);
      x++;
    }
  } else {
    /* slope < -1 or slope > 1 */
    if (dy < 0) {
      dx = -dx;
      dy = -dy;
      
      swap = y2;
      y2 = y1;
      y1 = swap;
      
      swap = x2;
      x2 = x1;
      x1 = swap;
    }
      
    if (dx < 0) {
      dx = -dx;
      inc = -1;
    }
      
    x = x1;
    y = y1 + 1;
      
    G = 2 * dx - dy;
    minG = maxG = G;
    DeltaG1 = 2 * (dx - dy);
    DeltaG2 = 2 * dx;
      
    while (y <= y2) {
      if (G > 0) {
	G += DeltaG1;
	x += inc;
      } else
	G += DeltaG2;
	  
      if (x>=0 && y>=0 && x<z->W && y<z->H)
	SetPixel(z, x, y);
      y++;
    }
  }
}
#undef SetPixel


void        CImgBrightnessContrast(CImg *dest, CImg *src, 
				   float bri,float con, int bgcolor)
{
  float center, scale, Y;
  int i,v,uc;
  component *p, *q;
  unsigned char lookup[256];
  

  if (bri==0.0 && con==0.0) {
    CImgFullCopy(dest,src);
    return;
  }

  center = 128 + (bri * 128.0) / 100.0;
  scale  = 1.0 + (con / 100.0);

  for(uc=0;uc<256;uc++) {
    Y = (float) uc;
    Y = center + (Y-128)*scale;
    if (Y<0.0) Y=0.0; if (Y>255.0) Y=255.0;
    lookup[uc] = (unsigned char) Y;
  }

  i = src->W * src->H;
  p = src->val;
  q = dest->val;    

  while(i) {
    v  = *(p++) << 16;
    v |= *(p++) << 8;
    v |= *(p++);

    if (v!=bgcolor) {
      v=RGB2YCbCr(v);
      v=triplet(lookup[t0(v)], t1(v), t2(v));
      v=YCbCr2RGB(v);
    }
    
    *(q++) = t0(v);
    *(q++) = t1(v);
    *(q++) = t2(v);
    --i;
  }

}

void CImgXOR(CImg *dest, CImg *src) {
  int i,n;

  if (!CImgCompatible(dest,src)) return;

  n = dest->W * dest->H * 3;
  for(i=0;i<n;i++)
    dest->val[i] ^= src->val[i];
}

CImg *CImgReadP6(char *path) {
  gzFile f;
  int i,w,h;
  CImg *img;
  char buf[512];

  f = gzopen(path,"r");
  if (!f) return 0;

  if (gzgets(f,buf,512) == 0)      return 0;
  if (buf[0]!='P' || buf[1]!='6') return 0;

  if (nc_gzgets(buf,512,f) == 0) return 0;
  if (sscanf(buf," %d %d ",&w,&h)!=2) return 0;
  if (nc_gzgets(buf,512,f) == 0) return 0;

  img = CImgNew(w,h);
  if (!img) return 0;
  
  for(i=0;i<h;i++)
    gzread(f,&(img->val[3*i*w]),w*3);
  gzclose(f);
  return(img);
}

void CImgSubst(CImg *dest, int dc, int sc) {
  component *p;
  int i,N;
  component a,b,c,d,e,f;

  a = t0(sc);
  b = t1(sc);
  c = t2(sc);
  d = t0(dc);
  e = t1(dc);
  f = t2(dc);
  
  N = dest->W * dest->H;
  for(i=0,p=dest->val;i<N;i++,p+=3) {
    if (p[0] == a && p[1] == b && p[2] == c) {
      p[0] = d;
      p[1] = e;
      p[2] = f;
    }
  }
}

void CImgGray(CImg *dest) {
  component *p;
  int i,N;
  component a;
  N = dest->W * dest->H;
  for(i=0,p=dest->val;i<N;i++,p+=3) {
    a = (component) (0.257*(float)p[0]+0.504*(float)p[1]+
		     0.098*(float)p[2]+16.0);
    p[0]=p[1]=p[2]=a;
  }
}

int CImgWriteP5(CImg *src, char *path) {
  FILE *f;
  int i,j;
  component *p,a;
  
  f = fopen(path,"w");
  if (!f) return -1;

  fprintf(f,"P5\n%d %d\n255\n",src->W,src->H);

  j = src->W * src->H;
  p = src->val;
  for(i=0;i<j;i++) {
    a = (component) (0.257*(float)p[0]+0.504*(float)p[1]+
		     0.098*(float)p[2]+16.0);
    fputc(a,f);
    p+=3;
  }

  fclose(f);
  return 0;
}

int CImgWriteP6(CImg *src, char *path) {
  FILE *f;
  int i,j;
  component *p;
  
  f = fopen(path,"w");
  if (!f) return -1;

  fprintf(f,"P6\n%d %d\n255\n",src->W,src->H);

  j = 3 * src->W * src->H;
  p = src->val;
  for(i=0;i<j;i++)
    fputc(*(p++),f);

  fclose(f);
  return 0;
}

CImg  *CImgIntegerZoom(CImg *src, int zoom) {
  CImg *dest;
  int i,j;

  dest = CImgNew(src->W * zoom, src->H * zoom);
  if (zoom == 1) {
    CImgFullCopy(dest,src);
    return dest;
  }

  for(i=0;i<src->H;i++)
    for(j=0;j<src->W;j++)
      RgbRect(dest->val, j*zoom, i*zoom, zoom, zoom, 
	      CImgGet(src,j,i), dest->rowlen);

  return dest;
}

CImg  *CImgHalfScale(CImg *src) {
  CImg *dest;
  int i,j,k,w,h;
  int r[4],g[4],b[4];

  w = src->W / 2;
  h = src->H / 2;

  if (w<1) w=1;
  if (h<1) h=1;

  dest = CImgNew(w,h);

  for(j=0;j<src->H-1;j+=2) 
    for(i=0;i<src->W-1;i+=2) {

      r[0] = CImgGet(src,i,  j);
      r[1] = CImgGet(src,i+1,j);
      r[2] = CImgGet(src,i,  j+1);
      r[3] = CImgGet(src,i+1,j+1);

      for(k=0;k<4;k++) {
	g[k] = t1(r[k]);
	b[k] = t2(r[k]);
	r[k] = t0(r[k]);	
      }
      for(k=1;k<4;k++) {
	r[0] += r[k];
	g[0] += g[k];
	b[0] += b[k];
      }
      CImgSet(dest,i/2,j/2,triplet( r[0]/4, g[0]/4, b[0]/4 ));
    }

  return dest;
}

void CImgRot180(CImg *src) {
  component *f, *b;
  component x,y,z;
  int i;

  f = src->val;
  b = src->val + (src->rowlen * (src->H-1) + 3 * (src->W-1));

  for(i = (src->W * src->H) / 2;i;i--) {
    x = *b;
    y = *(b+1);
    z = *(b+2);

    *b = *f;
    *(b+1) = *(f+1);
    *(b+2) = *(f+2);

    *f     = x;
    *(f+1) = y;
    *(f+2) = z;

    f+=3;
    b-=3;
  }
}

void CImgBitBlm(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
		int w, int h, float ratio)
{
  int lines_left;
  int line_len, djump, sjump;
  component *dpt, *spt;

  static component *tmpbuffer = 0;
  static int tmpsz = 0;
  
  if (ratio==1.0) {
    CImgBitBlt(dest,dx,dy,src,sx,sy,w,h);
    return;
  }
  if (ratio==0.0)
    return;

  /* fix bounds if needed */

  if (dx < 0) { w += dx; sx -= dx; dx = 0; }
  if (dy < 0) { h += dy; sy -= dy; dy = 0; }
  if (sx < 0) { w += sx; dx -= sx; sx = 0; }
  if (sy < 0) { h += sy; dy -= sy; sy = 0; }

  if (dx+w >= dest->W) { w = dest->W - dx; }
  if (dy+h >= dest->H) { h = dest->H - dy; }
  if (sx+w >= src->W)  { w = src->W - sx;  }
  if (sy+h >= src->H)  { h = src->H - sy;  }    

  if (w < 1 || h < 1) return;

  if (dy < sy) { /* top-down line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      RGBMerge(dpt,spt,w,ratio);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
    return;
  }

  if (dy > sy) { /* bottom-up line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + ((dy+h-1) * dest->rowlen) +  3*dx;
    spt = src->val + ((sy+h-1) * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      RGBMerge(dpt,spt,w,ratio);
      dpt -= djump;
      spt -= sjump;
      --lines_left;
    }
    return;
  }

  /* dest and src lines overlap, use a temporary buffer */
  if (dy == sy) {
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    if (tmpsz < line_len) {
      if (tmpbuffer != 0) MemFree(tmpbuffer);
      tmpsz = line_len;
      tmpbuffer = (component *) MemAlloc(tmpsz);
      if (tmpbuffer == 0) {
	tmpsz = 0;
	printf("malloc error in CImgBitBlt\n");
	return;
      }
    }

    while(lines_left>0) {
      MemCpy(tmpbuffer,spt,line_len);
      RGBMerge(dpt,tmpbuffer,w,ratio);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
  }

}

void CImgSetTransparentColor(CImg *dest, int color) {
  int i,n;
  component *p, s0,s1,s2;
  n = dest->W * dest->H;


  s0 = t0(color) & 0xfe;
  s1 = t1(color);
  s2 = t2(color);

  p = dest->val;
  for(i=0;i<n;i++,p+=3) {
    if ( ((p[0] & 0xfe) == s0) && (p[1] == s1) && (p[2] == s2) )
      p[0] |= 0x01;
    else
      p[0] &= 0xfe;
  }
}

void RGBOverlay(component *d, component *s, int w) {
  int i;
  for(i=0;i<w;i++) {
    if ( (s[0] & 0x01) == 0) {
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
    }
    d+=3;
    s+=3;
  }
}

void RGBMerge(component *d, component *s, int w, float ratio) {
  int i;
  float comp;

  comp = 1.0 - ratio;

  for(i=0;i<w;i++) {
    d[0] = (component) ((ratio * (float)(s[0])) + (comp * (float)(d[0])));
    d[1] = (component) ((ratio * (float)(s[1])) + (comp * (float)(d[1])));
    d[2] = (component) ((ratio * (float)(s[2])) + (comp * (float)(d[2])));
    d+=3;
    s+=3;
  }
}

// uses transparent color information in bit 0 of red component of src
void CImgBitBlto(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
		 int w, int h) 
{
  int lines_left;
  int line_len, djump, sjump;
  component *dpt, *spt;

  static component *tmpbuffer = 0;
  static int tmpsz = 0;
  
  /* fix bounds if needed */

  if (dx < 0) { w += dx; sx -= dx; dx = 0; }
  if (dy < 0) { h += dy; sy -= dy; dy = 0; }
  if (sx < 0) { w += sx; dx -= sx; sx = 0; }
  if (sy < 0) { h += sy; dy -= sy; sy = 0; }

  if (dx+w >= dest->W) { w = dest->W - dx; }
  if (dy+h >= dest->H) { h = dest->H - dy; }
  if (sx+w >= src->W)  { w = src->W - sx;  }
  if (sy+h >= src->H)  { h = src->H - sy;  }    

  if (w < 1 || h < 1) return;

  if (dy < sy) { /* top-down line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      RGBOverlay(dpt,spt,w);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
    return;
  }

  if (dy > sy) { /* bottom-up line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + ((dy+h-1) * dest->rowlen) +  3*dx;
    spt = src->val + ((sy+h-1) * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      RGBOverlay(dpt,spt,w);
      dpt -= djump;
      spt -= sjump;
      --lines_left;
    }
    return;
  }

  /* dest and src lines overlap, use a temporary buffer */
  if (dy == sy) {
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    if (tmpsz < line_len) {
      if (tmpbuffer != 0) MemFree(tmpbuffer);
      tmpsz = line_len;
      tmpbuffer = (component *) MemAlloc(tmpsz);
      if (tmpbuffer == 0) {
	tmpsz = 0;
	printf("malloc error in CImgBitBlt\n");
	return;
      }
    }

    while(lines_left>0) {
      MemCpy(tmpbuffer,spt,line_len);
      RGBOverlay(dpt,tmpbuffer,w);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
  }
}

void CImgBitBlt(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
		int w, int h)
{
  int lines_left;
  int line_len, djump, sjump;
  component *dpt, *spt;

  static component *tmpbuffer = 0;
  static int tmpsz = 0;
  
  /* fix bounds if needed */

  if (dx < 0) { w += dx; sx -= dx; dx = 0; }
  if (dy < 0) { h += dy; sy -= dy; dy = 0; }
  if (sx < 0) { w += sx; dx -= sx; sx = 0; }
  if (sy < 0) { h += sy; dy -= sy; sy = 0; }

  if (dx+w >= dest->W) { w = dest->W - dx; }
  if (dy+h >= dest->H) { h = dest->H - dy; }
  if (sx+w >= src->W)  { w = src->W - sx;  }
  if (sy+h >= src->H)  { h = src->H - sy;  }    

  if (w < 1 || h < 1) return;

  if (dy < sy) { /* top-down line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      MemCpy(dpt,spt,line_len);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
    return;
  }

  if (dy > sy) { /* bottom-up line copy */
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + ((dy+h-1) * dest->rowlen) +  3*dx;
    spt = src->val + ((sy+h-1) * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    while(lines_left>0) {
      MemCpy(dpt,spt,line_len);
      dpt -= djump;
      spt -= sjump;
      --lines_left;
    }
    return;
  }

  /* dest and src lines overlap, use a temporary buffer */
  if (dy == sy) {
    lines_left = h;
    line_len   = 3 * w;
    dpt = dest->val + (dy * dest->rowlen) +  3*dx;
    spt = src->val + (sy * src->rowlen) + 3*sx;
    djump = dest->rowlen;
    sjump = src->rowlen;

    if (tmpsz < line_len) {
      if (tmpbuffer != 0) MemFree(tmpbuffer);
      tmpsz = line_len;
      tmpbuffer = (component *) MemAlloc(tmpsz);
      if (tmpbuffer == 0) {
	tmpsz = 0;
	printf("malloc error in CImgBitBlt\n");
	return;
      }
    }

    while(lines_left>0) {
      MemCpy(tmpbuffer,spt,line_len);
      MemCpy(dpt,tmpbuffer,line_len);
      dpt += djump;
      spt += sjump;
      --lines_left;
    }
  }

}

void CImgDrawCircle(CImg *z, int cx,int cy,int r,int color) {
  int i,j,px,py,ix,iy,x,y,bestdist,bestindex;
  int sqdist,d;
  component c1,c2,c3;

  int n8x[8] = {-1,-1, 0, 1,1,1,0,-1};
  int n8y[8] = { 0,-1,-1,-1,0,1,1,1};

  int fx[5] = {-1,-1,-1,-1,-1};
  int fy[5] = {-1,-1,-1,-1,-1};
  int fn=0;
  
  sqdist = r*r;

  c1 = t0(color);
  c2 = t1(color);
  c3 = t2(color);

  ix = px = cx-r;
  iy = py = cy;
  fx[4] = px; 
  fy[4] = py;

  do {

    // paint current pixel
    if (px >=0 && px< z->W && py>=0 && py < z->H) {
      i = 3*(py*(z->W) + px);
      z->val[i++] = c1;
      z->val[i++] = c2;
      z->val[i] = c3;
    }

    // walk along the circle border until the circle closes
    bestindex = -1;
    bestdist  = 1000000;
    for(i=0;i<8;i++) {
      x = px + n8x[i];
      y = py + n8y[i];
      d = 0;
      for(j=0;j<5;j++)
	if (x==fx[j] && y==fy[j]) { d=1; break; }
      if (d) continue;
      d = (x-cx)*(x-cx) + (y-cy)*(y-cy) - sqdist;
      if (d < 0) d = -d;
      if (d < bestdist) {
	bestdist  = d;
	bestindex = i;
      }
    }
    fx[fn] = px;
    fy[fn] = py;
    fn = (fn+1) % 5;
    px += n8x[bestindex];
    py += n8y[bestindex];
  } while(px!=ix || py!=iy);
}

void CImgFillCircle(CImg *z, int cx,int cy,int r,int color) {
  int i,j,sqdist,dy,dx,row,pos;
  int xmin,xmax,ymin,ymax;
  component c1,c2,c3;

  xmin = MAX(0,cx-r);
  xmax = MIN(z->W-1,cx+r);
  ymin = MAX(0,cy-r);
  ymax = MIN(z->H-1,cy+r);
  
  sqdist = r*r;
  c1 = t0(color);
  c2 = t1(color);
  c3 = t2(color);

  for(i=ymin;i<=ymax;i++) {
    dy = (i-cy) * (i-cy);
    row = 3*(i*z->W);
    for(j=xmin;j<=xmax;j++) {
      dx = ((j-cx) * (j-cx)) + dy;
      if (dx <= sqdist) {
	pos = row + 3*j;
	z->val[pos++] = c1;
	z->val[pos++] = c2;
	z->val[pos] = c3;
      }
    }
  }
}

void CImgDrawTriangle(CImg *z, int x1,int y1,int x2,int y2,
		      int x3,int y3, int color)
{
  CImgDrawLine(z,x1,y1,x2,y2,color);
  CImgDrawLine(z,x2,y2,x3,y3,color);
  CImgDrawLine(z,x3,y3,x1,y1,color);
}

void CImgFillTriangle(CImg *z, int x1,int y1,int x2,int y2,int x3,int y3,
		      int color)
{
  int xmin,xmax,ymin,ymax;
  int i,j,k,v[3],w;
  component c1,c2,c3;

  xmin = MIN(x1,MIN(x2,x3));
  ymin = MIN(y1,MIN(y2,y3));
  xmax = MAX(x1,MAX(x2,x3));
  ymax = MAX(y1,MAX(y2,y3));
  w = z->W;

  if (ymin < 0) ymin = 0;
  if (xmin < 0) xmin = 0;
  if (ymax < 0) ymax = 0;
  if (xmax < 0) xmax = 0;

  if (ymax >= z->H) ymax = z->H - 1;
  if (xmax >= z->W) xmax = z->W - 1;
  if (ymin >= z->H) ymin = z->H - 1;
  if (xmin >= z->W) xmin = z->W - 1;

  c1 = t0(color);
  c2 = t1(color);
  c3 = t2(color);

  for(j=ymin;j<=ymax;j++) {
    for(i=xmin;i<=xmax;i++) {      
      v[0] = det3i(x1,y1,1, x2,y2,1, i,j,1);
      v[1] = det3i(x2,y2,1, x3,y3,1, i,j,1);
      v[2] = det3i(x3,y3,1, x1,y1,1, i,j,1);
      if (v[0]*v[1]>=0 && v[1]*v[2]>=0 && v[0]*v[2]>=0) {
	k = 3*((w*j)+i);
	z->val[k++] = c1;
	z->val[k++] = c2;
	z->val[k] = c3;
      }
    }
  }
}

/* positive len: scale factor relative to (x1,y1) - (x2,y2)
   negative len: absolute value used as fixed arrowhead length. */
void  CImgDrawArrow(CImg *z, int x1,int y1,int x2,int y2,
		    int angle, float len, int color)
{
  Vector a,b,c;
  Transform r1,r2;
  float fang = (float) angle;

  CImgDrawLine(z,x1,y1,x2,y2,color);

  VectorAssign(&a, x1-x2, y1-y2, 0.0);

  if (len >= 0.0) {
    VectorScalarProduct(&a, len);
  } else {
    VectorNormalize(&a);
    VectorScalarProduct(&a, -len);
  }

  TransformZRot(&r1, -fang/2.0);
  TransformZRot(&r2, fang/2.0);
  PointTransform(&b,&a,&r1);
  PointTransform(&c,&a,&r2);

  CImgFillTriangle(z,x2,y2,x2+b.x,y2+b.y,x2+c.x,y2+c.y,color);
}

void        CImgDrawBoldText(CImg *z, Font *f, int x, int y, 
			     int color, char *text) {
  int i, sl;
  sl = strlen(text);
  for(i=0;i<sl;i++) {
    CImgDrawChar(z,f,x+i*(1+f->cw),y,color,(unsigned char) (text[i]));
    CImgDrawChar(z,f,1+x+i*(1+f->cw),y,color,(unsigned char) (text[i]));
  }
}

void        CImgDrawShieldedText(CImg *z, Font *f, int x, int y, 
			     int color, int border, char *text)
{
  int i,j;
  for(i=-1;i<=1;i++)
    for(j=-1;j<=1;j++)
      CImgDrawText(z,f,x+i,y+j,border,text);
  CImgDrawText(z,f,x,y,color,text);  
}

void        CImgDrawText(CImg *z, Font *f, int x, int y, 
			 int color, char *text)
{
  int i, sl;
  sl = strlen(text);
  for(i=0;i<sl;i++) 
    CImgDrawChar(z,f,x+i*(f->cw),y,color,(unsigned char)(text[i]));
}

void        SFontDrawCharDepth(int iw,int ih,SFont *f, int x, int y,
			       unsigned char letter,
			       float *depth,float dval)
{
  int w,h,i,j,q;
  component *pat, *lpat;

  w = f->cw[(int) letter];
  h = f->ch[(int) letter];
  pat = f->data + (f->bw * ((int)letter));

  for(j=0;j<h;j++) {
    if (y+j < 0) continue;
    if (y+j >= ih) break;
    lpat = pat + f->W * j;
    for(i=0;i<w;i++) {
      if (x+i < 0) continue;
      if (x+i >= iw) break;
      if (lpat[i]!=255) {
	q=(x+i)+(y+j)*iw;
	if (depth[q] > dval)
	  depth[q] = dval;
      }
    }
  }
}

void        FontDrawCharDepth(int iw,int ih,Font *f, int x, int y,
			      unsigned char letter,
			      float *depth,float dval)
{
  int i,j,w,h,p,q;
  char *pat;

  w = f->cw;
  h = f->H;
  pat = f->pat;

  for(j=0;j<h;j++) {
    if (y+j < 0) continue;
    if (y+j >= ih) break;
    p = j * (f->W) +  (((int)letter) * f->cw);
    for(i=0;i<w;i++) {
      if (x+i < 0) continue;
      if (x+i >= iw) break;
      if (pat[p])
	if (depth[q=(x+i)+(y+j)*iw] > dval)
	  depth[q] = dval;
      ++p;
    }
  }
}

void        CImgDrawChar(CImg *z, Font *f, int x, int y,
			 int color, unsigned char letter)
{
  int i,j,w,h,iw,ih,p;
  char *pat;
  component cr,cg,cb,*d,*base,*line;

  //  printf("draw %c at %d,%d\n",letter,x,y);

  cr = t0(color);
  cg = t1(color);
  cb = t2(color);

  iw = z->W;
  ih = z->H;
  w = f->cw;
  h = f->H;
  pat = f->pat;
  base = z->val;

  for(j=0;j<h;j++) {
    if (y+j < 0) continue;
    if (y+j >= ih) break;
    line = base + (z->rowlen) * (y+j);
    p = j * (f->W) +  (((int)letter) * f->cw);
    for(i=0;i<w;i++) {
      if (x+i < 0) continue;
      if (x+i >= iw) break;
      if (pat[p]) {
	d = line + 3*(x+i);
	*(d++) = cr;
	*(d++) = cg;
	*d = cb;
      }
      ++p;
    }
  } 
}

void CImgDrawGlyphAA(CImg *z, Glyph *g, int x, int y, int color) {
  int i,j,m,w,h,iw,ih,p;
  char *pat;
  component cr,cg,cb,*d,*base,*line;
  static char *aa = 0;
  static int   aasz = 0;

  cr = t0(color);
  cg = t1(color);
  cb = t2(color);

  iw = z->W;
  ih = z->H;
  w = g->W;
  h = g->H;
  pat = g->pat;
  base = z->val;

  if (aa != 0 && aasz < w*h) {
    free(aa);
    aa = 0;
    aasz = 0;
  }
  if (aa == 0) {
    aa = (char *) malloc(w*h);
    aasz = w*h;
  }
  memset(aa,0,w*h);
  
  for(j=0;j<h;j++)
    for(i=0;i<w;i++)
      if (pat[i+j*w]) {
	if (i-1 >= 0) aa[(i-1)+(j)*w]++;
	if (i+1 <  w) aa[(i+1)+(j)*w]++;
	if (j-1 >= 0) aa[(i)+(j-1)*w]++;
	if (j+1 <  h) aa[(i)+(j+1)*w]++;
	if (i-1 >= 0 && j-1 >=0) aa[(i-1)+(j-1)*w]++;
	if (i-1 >= 0 && j+1 < h) aa[(i-1)+(j+1)*w]++;
	if (i+1 <  w && j-1 >=0) aa[(i+1)+(j-1)*w]++;
	if (i+1 <  w && j+1 < h) aa[(i+1)+(j+1)*w]++;
      }
  for(i=0;i<w*h;i++)
    if (pat[i]) aa[i] = 0;

  p = 0;
  for(j=0;j<h;j++) {
    if (y+j < 0) { p+=w; continue; }
    if (y+j >= ih) break;
    line = base + (z->rowlen) * (y+j);
    for(i=0;i<w;i++) {
      if (x+i < 0) { ++p; continue; }
      if (x+i >= iw) { p+=(w-i); break; }
      if (pat[p]) {
	d = line + 3*(x+i);
	*(d++) = cr;
	*(d++) = cg;
	*d = cb;
      }
      ++p;
    }
  }

  // draw aa mask

  for(j=0;j<h;j++)
    for(i=0;i<w;i++)
      if (aa[i+j*w] > 1) {
	d = base + (z->rowlen) * (y+j);
	d += 3*(x+i);
	m = triplet(*d,*(d+1),*(d+2));
	m = mergeColorsRGB(m,color,0.100 * ((float)(aa[i+j*w])));
	*d = t0(m);
	*(d+1) = t1(m);
	*(d+2) = t2(m);
      }
}

void CImgDrawGlyph(CImg *z, Glyph *g, int x, int y, int color) {
  int i,j,w,h,iw,ih,p;
  char *pat;
  component cr,cg,cb,*d,*base,*line;

  cr = t0(color);
  cg = t1(color);
  cb = t2(color);

  iw = z->W;
  ih = z->H;
  w = g->W;
  h = g->H;
  pat = g->pat;
  base = z->val;

  p = 0;
  for(j=0;j<h;j++) {
    if (y+j < 0) { p+=w; continue; }
    if (y+j >= ih) break;
    line = base + (z->rowlen) * (y+j);
    for(i=0;i<w;i++) {
      if (x+i < 0) { ++p; continue; }
      if (x+i >= iw) { p+=(w-i); break; }
      if (pat[p]) {
	d = line + 3*(x+i);
	*(d++) = cr;
	*(d++) = cg;
	*d = cb;
      }
      ++p;
    }
  }

}

RLEImg * RLENewFromBRV(char *filename) {
  gzFile f;
  RLEImg *rle;
  int sz,left,d;
  int w,h,nf;
  char buf[512];
  char sig[32];
  component *pt;

  rle = (RLEImg *) MemAlloc(sizeof(RLEImg));
  if (!rle) return 0;

  f = gzopen(filename,"r");
  if (!f) return 0;
  if (!gzgets(f,buf,511)) return 0;

  if (sscanf(buf,"%s %d %d %d %d ",sig,&w,&h,&nf,&sz)!=5) return 0;
  
  rle->data = (component *) malloc(sz);
  rle->streamlen = sz;

  pt = rle->data;
  left = sz;

  while(left > 0) {
    gzread(f,pt,d = ((left > 8192) ? 8192 : left));
    left -= d;
    pt += d;
  }

  rle->W = w;
  rle->H = h*nf;

  gzclose(f);
  return(rle);
}

RLEImg * RLENewFromStream(char *stream, int W, int H) {
  RLEImg *rle;
  int sz=0;
  rle = (RLEImg *) MemAlloc(sizeof(RLEImg));
  if (!rle) return 0;
  rle->W = W;
  rle->H = H;
  rle->data = (component *) Unstream(stream, &sz);
  rle->streamlen = sz;
  return(rle);
}

CImg * DecodeLossy(LossyImg *src) {
  CImg *dest;
  component *sv,*dv;
  int i,n,c1,c2;

  dest = CImgNew(src->W,src->H);
  n = src->W * src->H;

  sv = src->data;
  dv = dest->val;

  for(i=0;i<n/2;i++) {
    c1=YCbCr2RGB(TRIPLET(sv[0],sv[2],sv[3]));
    c2=YCbCr2RGB(TRIPLET(sv[1],sv[2],sv[3]));
    dv[0] = t0(c1);
    dv[1] = t1(c1);
    dv[2] = t2(c1);
    dv[3] = t0(c2);
    dv[4] = t1(c2);
    dv[5] = t2(c2);
    sv+=4;
    dv+=6;
  }

  return dest;
}

LossyImg * EncodeLossy(CImg *src) {
  LossyImg *li;
  int i,n,c1,c2;
  component *sv, *dv;

  n = src->W * src->H;

  li = (LossyImg *) MemAlloc(sizeof(LossyImg));
  li->W = src->W;
  li->H = src->H;
  li->data = (component *) MemAlloc(2*n);
  li->streamlen = 2*n;

  sv = src->val;
  dv = li->data;

  for(i=0;i<n/2;i++) {
    c1 = RGB2YCbCr(triplet(sv[0],sv[1],sv[2]));
    c2 = RGB2YCbCr(triplet(sv[3],sv[4],sv[5]));

    dv[0] = t0(c1); // Y1
    dv[1] = t0(c2); // Y2
    dv[2] = (t1(c1)>>1) + (t1(c2)>>1); // Cb avg
    dv[3] = (t2(c1)>>1) + (t2(c2)>>1); // Cr avg

    sv+=6;
    dv+=4;
  }

  return li;
}


RLEImg * EncodeRLE(CImg *src) {
  RLEImg *rle;
  int i,j,k,x, N, mr;
  int rl = 0;
  int sz = 4096;

  //FILE *f;

  component *sv, *dv;

  //f = fopen("rlesrc.img","w");
  //  for(i=0;i<src->W * src->H;i++)
  //    fputc(src->val[i],f);
  //  fclose(f);

  rle = (RLEImg *) MemAlloc(sizeof(RLEImg));
  rle->W = src->W;
  rle->H = src->H;
  rle->data = (component *) MemAlloc(sz);

  sv = src->val;
  dv = rle->data;

  N = src->W * src->H;
  j = 0; /* position in the rle stream */

  for(i=0;i<N;) {

    /* grow stream if needed */
    if (sz - rl < 8) {
      sz += 4096;
      dv = rle->data = (component *) MemRealloc(rle->data, sz);
    }

    mr = N - i;
    if (mr > 255) mr = 255;
    /* count n repetitions from this position */
    x = mr;
    for(k=0;k<mr;k++) {
      if ( (sv[3*(i+k)]   != sv[(3*i)])   ||
	   (sv[1+3*(i+k)] != sv[1+(3*i)]) ||
	   (sv[2+3*(i+k)] != sv[2+(3*i)]) ) {
	// printf("differ at k=%d\n",k);
	x = k;
	break;
      }
    }    
    
    dv[rl]   = (unsigned char) x;
    dv[rl+1] = sv[3*i];
    dv[rl+2] = sv[3*i+1];
    dv[rl+3] = sv[3*i+2];

    //    printf("i=%d rl=%d rle=<%d>%.2x%.2x%.2x\n",
    //	   i, rl, dv[rl], dv[rl+1], dv[rl+2], dv[rl+3]);

    rl += 4;
    i  += x;
  }

  rle->data = (component *) MemRealloc(rle->data, rl);
  rle->streamlen = rl;

  //f = fopen("rleout.img","w");
  //for(i=0;i<rl;i++)
  //  fputc(rle->data[i],f);
  //fclose(f);

  //  printf("RLE encoding: WxH=%dx%d N=%d RLE=%d ratio=%.4f\n",
  //	 src->W, src->H, N*3, rl, ((float)rl)/((float)(3*N)));
  

  return(rle);
}

void     RLEDestroy(RLEImg *rle) {
  MemFree(rle->data);
  MemFree(rle);
}

CImg   * DecodeRLE(RLEImg *src) {
  CImg *dest;
  component *sv, *dv;
  int n,N;
  unsigned int k;
  //  FILE *f;

  dest = CImgNew(src->W,src->H);
  N = src->W * src->H;
  
  sv = src->data;
  dv = dest->val;

  for(n=0;n<N;sv+=4) {
    for(k=0;k<sv[0];k++) {
      *(dv++) = sv[1];
      *(dv++) = sv[2];
      *(dv++) = sv[3];
    }
    n += sv[0];
  }

  // f = fopen("rledec.img","w");
  // for(i=0;i<dest->W * dest->H;i++)
  //  fputc(dest->val[i],f);
  // fclose(f);

  return dest;
}

Glyph *GlyphNew(int w,int h) {
  Glyph *g;
  g = (Glyph *) MemAlloc(sizeof(Glyph));
  if (!g) return 0;
  g->W = w;
  g->H = h;
  g->pat = (char *) MemAlloc(w*h*sizeof(char));
  if (!g->pat) { MemFree(g); return 0; }
  MemSet(g->pat,0,w*h);
  return g;
}

void   GlyphDestroy(Glyph *g) {
  if (g) {
    if (g->pat) MemFree(g->pat);
    MemFree(g);
  }
}

void   GlyphInit(Glyph *g,char *s) {
  int i,j,n,rest;
  char *p;

  n = g->W * g->H;
  MemSet(g->pat,0,n);

  if (s[0] != '!') {
    for(i=0;i<n;i++)
      if (s[i] != '.' && s[i] !=' ' && s[i] != '0')
	g->pat[i] = 1;
  } else {
    // hexadecimal representation
    p = s+1;
    i = 0;
    rest = n;
    while(rest && (*p)!=0) {
      if (*p >= '0' && *p <= '9') j = (*p) - '0';
      else if (*p >= 'A' && *p <= 'F') j = 10 + (*p) - 'A';
      else if (*p >= 'a' && *p <= 'f') j = 10 + (*p) - 'a';
      else j = 0;

      if (rest) { g->pat[i++] = j&8?1:0; --rest; }
      if (rest) { g->pat[i++] = j&4?1:0; --rest; }
      if (rest) { g->pat[i++] = j&2?1:0; --rest; }
      if (rest) { g->pat[i++] = j&1?1:0; --rest; }
      ++p;
    }
  }
}

Glyph *GlyphNewInit(int w,int h,char *s) {
  Glyph *g;
  g = GlyphNew(w,h);
  if (g) GlyphInit(g,s);
  return g;
}

void   GlyphSet(Glyph *g, int x,int y,int on) {
  g->pat[x+y*(g->W)] = on ? 1 : 0;
}

int    GlyphGet(Glyph *g, int x,int y) {
  return(g->pat[x+y*(g->W)]!=0);
}

Font *FontNew(int w,int h,char *s) {
  Font *f;
  int i,j,n,rest;
  char *p;

  f = (Font *) MemAlloc(sizeof(Font));
  if (!f) return 0;

  f->cw = w;
  f->W = w * 256;
  f->H = h;

  f->pat = (char *) MemAlloc(256*w*h*sizeof(char));
  if (!f->pat) { MemFree(f); return 0; }
  MemSet(f->pat,0,256*w*h);

  n = f->W * f->H;
  MemSet(f->pat,0,n);

  if (s[0] != '!') {
    for(i=0;i<n;i++)
      if (s[i] != '.' && s[i] !=' ' && s[i] != '0')
	f->pat[i] = 1;
  } else {
    // hexadecimal representation
    p = s+1;
    i = 0;
    rest = n;
    while(rest && (*p)!=0) {
      if (*p >= '0' && *p <= '9') j = (*p) - '0';
      else if (*p >= 'A' && *p <= 'F') j = 10 + (*p) - 'A';
      else if (*p >= 'a' && *p <= 'f') j = 10 + (*p) - 'a';
      else j = 0;

      if (rest) { f->pat[i++] = j&8?1:0; --rest; }
      if (rest) { f->pat[i++] = j&4?1:0; --rest; }
      if (rest) { f->pat[i++] = j&2?1:0; --rest; }
      if (rest) { f->pat[i++] = j&1?1:0; --rest; }
      ++p;
    }
  }

  return f;
}

void  FontDestroy(Font *f) {
  if (f) {
    if (f->pat) MemFree(f->pat);
    MemFree(f);
  }
}

CImg *CImgFastScale(CImg *src, float factor) {
  int i,j,nw,nh,nm,ow,oh;
  int *lu;
  CImg *dest;
  component *d,*s;

  if (!src) return 0;

  nw = (int) ((float)(src->W) * factor);
  nh = (int) ((float)(src->H) * factor);
  ow = src->W;
  oh = src->H;

  if (nw<=0 || nh<=0) return 0;

  dest = CImgNew(nw,nh);
  if (!dest) return 0;

  nm = MAX(nw,nh);
  lu = (int *) MemAlloc(nm * sizeof(int));
  if (!lu) return 0;

  for(i=0;i<nm;i++)
    lu[i] = (int) (((float)i) / factor);

  d = dest->val;
  for(j=0;j<nh;j++)
    for(i=0;i<nw;i++) {
      s = src->val + 3*(lu[i] + ow*lu[j]);
      *(d++) = *(s++);
      *(d++) = *(s++);
      *(d++) = *(s++);
    }

  MemFree(lu);
  return dest;
}

CImg *CImgScaleUp(CImg *src, float factor) {
  int nw,nh,ow,oh;
  float fi,fj;
  int i,j,x,y;
  component *s,*d;
  CImg *dest;

  if (!src) return 0;

  nw = (int) ((float)(src->W) * factor);
  nh = (int) ((float)(src->H) * factor);
  ow = src->W;
  oh = src->H;

  if (nw<=0 || nh<=0) return 0;

  dest = CImgNew(nw,nh);
  if (!dest) return 0;

  for(j=0;j<nh;j++)
    for(i=0;i<nw;i++) {

      fi = (float) i;
      fj = (float) j;

      x = (int) (fi / factor);
      y = (int) (fj / factor);

      if (x>=0 && x<ow && y>=0 && y<oh) {
	d = &(dest->val[3*(i+nw*j)]);
	s = &(src->val[3*(x+ow*y)]);
	*(d++) = *(s++);
	*(d++) = *(s++);
	*d = *s;
      }

    }
  return dest;
}

// slow but (probably) correct
CImg *CImgScale(CImg *src, float factor) {
  int nw,nh,ow,oh;
  CImg *dest;
  float x1,x2,y1,y2,fi,fj,dx,dy,fr,fg,fb,di;
  int i,j,k,a,b;
  component R,G,B;

  int *lookup;
  int *area;
  int  count;

  if (factor > 1.0) {
    return(CImgScaleUp(src,factor));
  }

  if (!src) return 0;

  nw = (int) ((float)(src->W) * factor);
  nh = (int) ((float)(src->H) * factor);
  ow = src->W;
  oh = src->H;

  if (nw<=0 || nh<=0) return 0;

  dest = CImgNew(nw,nh);
  if (!dest) return 0;

  lookup = (int *) MemAlloc(a=MAX(nw,ow)*MAX(nh,oh)*sizeof(int));
  area   = (int *) MemAlloc(a);
  if (!lookup || !area) {
    CImgDestroy(dest);
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
      for(b=y1;b<=y2;b++)
	if (b>=0 && b<oh)
	  for(a=x1;a<=x2;a++)
	    if (a>=0 && a<ow) {
	      k = 3*(a+b*ow);
	      lookup[count]=triplet(src->val[k],src->val[k+1],src->val[k+2]);
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
	fr += ((float)(area[b])) * ((float)t0(lookup[b]));
	fg += ((float)(area[b])) * ((float)t1(lookup[b]));
	fb += ((float)(area[b])) * ((float)t2(lookup[b]));
      }
      fr /= (float) a;
      fg /= (float) a;
      fb /= (float) a;

      R = (component) fr;
      G = (component) fg;
      B = (component) fb;

      k = 3*(i+j*nw);
      dest->val[k] = R;
      dest->val[k+1] = G;
      dest->val[k+2] = B;
    }

  MemFree(area);
  MemFree(lookup);
  return dest;
}

CImg *CImgScaleToWidth(CImg *src, int width) {
  float factor;
  factor = ((float)width) / ((float)(src->W));
  return(CImgScale(src, factor));
}

CImg *CImgScaleToHeight(CImg *src, int height) {
  float factor;
  factor = ((float)height) / ((float)(src->H));
  return(CImgScale(src, factor));
}

CImg *CImgScaleToFit(CImg *src, int maxdim) {
  if (src->W > src->H)
    return(CImgScaleToWidth(src,maxdim));
  else
    return(CImgScaleToHeight(src,maxdim));
}

char *   Stream(char *data, int size) {
  int i,j,slen;
  char *r;
  static char hexa[17] = "abcdefghijklmnop";

  slen = 10+size*2+1;
  r = (char *) malloc(slen);
  if (!r) return 0;
  MemSet(r,0,slen);

  sprintf(r,"%.10d",size);
  j = 10;
  for(i=0;i<size;i++) {
    r[j++] = hexa[ ((unsigned char) data[i]) / 16 ];
    r[j++] = hexa[ ((unsigned char) data[i]) % 16 ];
  }

  return r;
}

char * Unstream(char *stream, int *sz) {
  char *r;
  char t;
  int size,i,j;

  t = stream[10];
  stream[10] = 0;
  
  size = atoi(stream);
  stream[10] = t;

  r = (char *) malloc(size);
  if (!r) return 0;
  MemSet(r,0,size);

  j = 10;
  for(i=0;i<size;i++) {
    r[i] += 16 * (stream[j++] - 'a');
    r[i] += (stream[j++] - 'a');
  }
  
  if (sz != 0)
    *sz = size;
  return r;
}

SFont * SFontLoad(char *file) {
  FILE *f;
  SFont *s;
  char buf[4096],*p;
  int i,j;

  f = fopen(file,"r");
  if (!f) return 0;

  s = (SFont *) MemAlloc(sizeof(SFont));
  if (!s) { fclose(f); return 0; }

  if (fgets(buf,4095,f)==NULL) {
    fclose(f);
    return 0;
  }
   
  if (sscanf(buf,"%d %d\n",&i,&j)!=2) {
    fclose(f);
    return 0;
  }

  s->bw = i;
  s->bh = j;
  s->W = i*256;
  s->H = j;
  s->mh = 0;

  for(i=0;i<256;i++) {
    s->cw[i] = s->bw;
    s->ch[i] = s->bh;
  }

  s->data = (component *) MemAlloc(s->W * s->H);
  if (!s->data) {
    fclose(f);    
    return 0;
  }

  if (fgets(buf,4095,f)==NULL) {
    fclose(f);
    return 0;
  }
  p = strtok(buf," \t\r\n");
  i = 0;
  while(p && i<256) {
    s->cw[i] = MIN(atoi(p)+1,s->bw);
    p = strtok(0," \t\r\n");
    ++i;
  }
  if (fgets(buf,4095,f)==NULL) {
    fclose(f);
    return 0;
  }
  p = strtok(buf," \t\r\n");
  i = 0;
  while(p && i<256) {
    s->ch[i] = MIN(atoi(p),s->bh);
    s->mh = MAX(s->mh, s->ch[i]);
    p = strtok(0," \t\r\n");
    ++i;
  }
  for(i=0;i<s->bh;i++)
    fread(s->data+i*(s->W),1,s->W,f);  
  fclose(f);
  return s;
}

SFont * SFontFromFont(Font *f) {
  SFont *s;
  int i,n;

  //FILE *F;

  s = (SFont *) MemAlloc(sizeof(SFont));
  if (!s) return 0;
  s->W = 256 * f->cw;
  s->H  = f->H;
  s->mh = f->H;
  s->bw = f->cw;
  s->bh = f->H;

  for(i=0;i<256;i++) {
    s->cw[i] = f->cw;
    s->ch[i] = f->H;
  }

  s->data = (component *) MemAlloc(s->W * s->H);  
  if (!s->data)
    return 0;
  n = s->W * s->H;
  for(i=0;i<n;i++)
    s->data[i] = f->pat[i] ? 0 : 255;

  /*
  F = fopen("argh.pgm","w");
  fprintf(F,"P5\n%d %d\n255\n",s->W,s->H);
  for(i=0;i<n;i++)
    fputc(s->data[i],F);
  fclose(F);
  */

  return s;
}

int SFontLen(SFont *sf, char *text) {
  int i,l,w;
  w = 0;
  l = strlen(text);
  for(i=0;i<l;i++)
    w += sf->cw[ (int)(unsigned char) text[i] ];
  return w;
}

int SFontH(SFont *sf, char *text) {
  int i,l,w;
  w = 0;
  l = strlen(text);
  for(i=0;i<l;i++)
    w = MAX(w,sf->ch[ (int)(unsigned char) text[i] ]);
  return w;
}

void    SFontDestroy(SFont *sf) {
  if (sf) {
    if (sf->data)
      MemFree(sf->data);
    MemFree(sf);
  }
}

void        CImgDrawSChar(CImg *z, SFont *f, int x, int y,
			  int color, unsigned char letter)
{
  static float ratio[256];
  static int rinit = 0;
  int i,j,w,h,iw,ih;
  component cr,cg,cb,*d,*base,*line,*pat,*lpat;
  int sc;

  if (!rinit) {
    for(i=0;i<256;i++)
      ratio[i] = ((float) i)/255.0;
    rinit = 1;
  }

  cr = t0(color);
  cg = t1(color);
  cb = t2(color);

  iw = z->W;
  ih = z->H;
  w = f->cw[(int) letter];
  h = f->ch[(int) letter];
  pat = f->data + (f->bw * ((int)letter));
  base = z->val;

  for(j=0;j<h;j++) {
    if (y+j < 0) continue;
    if (y+j >= ih) break;
    line = base + (z->rowlen) * (y+j);
    lpat = pat + f->W * j;
    for(i=0;i<w;i++) {
      if (x+i < 0) continue;
      if (x+i >= iw) break;
      switch(lpat[i]) {
      case 0: // full color
	d = line + 3*(x+i);
	d[0] = cr;
	d[1] = cg;
	d[2] = cb;
	break;
      case 255: // no color
	break;
      default: // merge required
	d = line + 3*(x+i);
	sc = TRIPLET(d[0],d[1],d[2]);
	sc = mergeColorsRGB(color,sc,ratio[(int)lpat[i]]);
	d[0] = t0(sc);
	d[1] = t1(sc);
	d[2] = t2(sc);
      }
    }
  }  
}

void        CImgDrawSText(CImg *z, SFont *f, int x, int y, 
			  int color, char *text) 
{
  int i, sl;
  sl = strlen(text);
  for(i=0;i<sl;i++) {
    CImgDrawSChar(z,f,x,y,color,(unsigned char)(text[i]));
    x+=f->cw[(int)(unsigned char)(text[i])];
  }
}

void        CImgDrawBoldSText(CImg *z, SFont *f, int x, int y, 
			      int color, char *text) {
  int i, sl;
  sl = strlen(text);
  for(i=0;i<sl;i++) {
    CImgDrawSChar(z,f,x,y,color,(unsigned char)(text[i]));
    CImgDrawSChar(z,f,x+1,y,color,(unsigned char)(text[i]));
    x+=f->cw[(int)(unsigned char)(text[i])];
  }
}

void        CImgDrawShieldedSText(CImg *z, SFont *f, int x, int y, 
				  int color, int border, char *text) {
  int i,j;
  for(i=-1;i<=1;i++)
    for(j=-1;j<=1;j++)
      CImgDrawSText(z,f,x+i,y+j,border,text);
  CImgDrawSText(z,f,x,y,color,text);
}

