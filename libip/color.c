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

/***********************************************
 $Id: color.c,v 1.5 2013/11/11 22:34:12 bergo Exp $

 color.c - color manipulation

************************************************/

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "ip_color.h"

int triplet(int a,int b,int c) {
  return(((a&0xff)<<16)|((b&0xff)<<8)|(c&0xff));
}

int RGB2YCbCr(int v) {
  float y,cb,cr;

  y=0.257*(float)t0(v)+
    0.504*(float)t1(v)+
    0.098*(float)t2(v)+16.0;

  cb=-0.148*(float)t0(v)-
      0.291*(float)t1(v)+
      0.439*(float)t2(v)+
      128.0;

  cr=0.439*(float)t0(v)-
     0.368*(float)t1(v)-
     0.071*(float)t2(v)+
     128.0;

  return(triplet((int)y,(int)cb,(int)cr));
}

int YCbCr2RGB(int v) {
  float r,g,b;

  r=1.164*((float)t0(v)-16.0)+
    1.596*((float)t2(v)-128.0);

  g=1.164*((float)t0(v)-16.0)-
    0.813*((float)t2(v)-128.0)-
    0.392*((float)t1(v)-128.0);
  
  b=1.164*((float)t0(v)-16.0)+
    2.017*((float)t1(v)-128.0);

  if (r<0.0) r=0.0;
  if (g<0.0) g=0.0;
  if (b<0.0) b=0.0;
  if (r>255.0) r=255.0;
  if (g>255.0) g=255.0;
  if (b>255.0) b=255.0;

  return(triplet((int)r,(int)g,(int)b));
}

#define GMAX(a,b,c) (((a>=b)&&(a>=c))?a:((b>c)?b:c))
#define GMIN(a,b,c) (((a<=b)&&(a<=c))?a:((b<c)?b:c))

int RGB2HSV(int vi) {
  float r = (float) t0(vi), 
        g = (float) t1(vi), 
        b = (float) t2(vi), v, x, f; 
  float a[3];

  int i; 

  r/=255.0;
  g/=255.0;
  b/=255.0;

  // RGB are each on [0, 1]. S and V are returned on [0, 1] and H is 
  // returned on [0, 6]. 

  x = GMIN(r, g, b); 
  v = GMAX(r, g, b); 
  if (v == x) {
    a[0]=0.0;
    a[1]=0.0;
    a[2]=v;
  } else {
    f = (r == x) ? g - b : ((g == x) ? b - r : r - g); 
    i = (r == x) ? 3 : ((g == x) ? 5 : 1);
    a[0]=((float)i)-f/(v-x);
    a[1]=(v-x)/v;
    a[2]=v;
  }

  // (un)normalize

  a[0]*=255.0/6.0;
  a[1]*=255.0;
  a[2]*=255.0;
  
  return(triplet((int)a[0],(int)a[1],(int)a[2]));
}

int HSV2RGB(int vi) {
  // H is given on [0, 6]. S and V are given on [0, 1]. 
  // RGB are each returned on [0, 1]. 
  float h = (float)t0(vi), 
        s = (float)t1(vi), 
        v = (float)t2(vi), m, n, f; 
  float a[3] = {0,0,0};
  int i; 

  h/=255.0/6.0;
  s/=255.0;
  v/=255.0;

  if (s==0.0) {
    a[0]=a[1]=a[2]=v;
  } else {
    i = (int) floor(h); 
    f = h - (float)i; 
    if(!(i & 1)) f = 1 - f; // if i is even 
    m = v * (1 - s); 
    n = v * (1 - s * f); 
    switch (i) { 
    case 6: 
    case 0: a[0]=v; a[1]=n; a[2]=m; break;
    case 1: a[0]=n; a[1]=v; a[2]=m; break;
    case 2: a[0]=m; a[1]=v; a[2]=n; break;
    case 3: a[0]=m; a[1]=n; a[2]=v; break;
    case 4: a[0]=n; a[1]=m; a[2]=v; break;
    case 5: a[0]=v; a[1]=m; a[2]=n; break;
    } 
  }

  // (un)normalize
  for(i=0;i<3;i++)
    a[i]*=255;

  return(triplet((int)a[0],(int)a[1],(int)a[2]));
}

int lighter(int c, int n) {
  int r,g,b,r0,g0,b0,i;
  r=c>>16;
  g=(c>>8)&0xff;
  b=c&0xff;

  for(i=0;i<n;i++) {
    r0=r+r/10; if (r0==r) r0=r+1;
    g0=g+g/10; if (g0==g) g0=g+1;
    b0=b+b/10; if (b0==b) b0=b+1;

    if (r0>255) r0=255;
    if (b0>255) b0=255;
    if (g0>255) g0=255;

    r=r0; g=g0; b=b0;
  }

  return( (r<<16)|(g<<8)|b );  
}

int darker(int c, int n) {
  int r,g,b,r0,g0,b0,i;
  r=c>>16;
  g=(c>>8)&0xff;
  b=c&0xff;

  for(i=0;i<n;i++) {
    r0=r-r/10; if (r0==r) r0=r-1;
    g0=g-g/10; if (g0==g) g0=g-1;
    b0=b-b/10; if (b0==b) b0=b-1;

    if (r0<0) r0=0;
    if (b0<0) b0=0;
    if (g0<0) g0=0;

    r=r0; g=g0; b=b0;
  }

  return( (r<<16)|(g<<8)|b );
}

/* 
   inverseColor
   Returns the inverse in RGB-space.

   input:  RGB triplet
   output: RGB triplet
   author: bergo
*/
int          inverseColor(int c) {
  int r,g,b;
  r=c>>16;
  g=(c>>8)&0xff;
  b=c&0xff;
  r=255-r; g=255-g; b=255-b;
  return( (r<<16)|(g<<8)|b );  
}

int superSat(int c) {
  int x;
  x = RGB2HSV(c);
  x = triplet(t0(x),255,t2(x));
  x = HSV2RGB(x);
  return x;
}

/*
  mergeColorsRGB
  Merges 2 colors, (1-ratio) of a with (ratio) of b.
  Works in RGB space.

  input:  2 RGB triplets, merge ratio
  output: RGB triplet
  author: bergo
*/
int mergeColorsRGB(int a,int b,float ratio) {
  float c[6];
  int r[3];

  c[0] = (float) (a >> 16);
  c[1] = (float) (0xff & (a >> 8));
  c[2] = (float) (0xff & a);

  c[3] = (float) (b >> 16);
  c[4] = (float) (0xff & (b >> 8));
  c[5] = (float) (0xff & b);

  c[0] = (1.0-ratio) * c[0] + ratio * c[3];
  c[1] = (1.0-ratio) * c[1] + ratio * c[4];
  c[2] = (1.0-ratio) * c[2] + ratio * c[5];

  r[0] = (int) c[0];
  r[1] = (int) c[1];
  r[2] = (int) c[2];

  return( (r[0]<<16) | (r[1]<<8) | r[2] );
}

/*
  mergeColorsYCbCr
  Merges 2 colors, (1-ratio) of a with (ratio) of b.
  Works in YCbCr space.

  input:  2 RGB triplets, merge ratio
  output: RGB triplet
  author: bergo
*/
int          mergeColorsYCbCr(int a,int b,float ratio) {
  float c[6];
  int ya,yb;
  int r[3];

  ya=RGB2YCbCr(a);
  yb=RGB2YCbCr(b);

  c[0] = (float) (a >> 16);
  c[1] = (float) (0xff & (a >> 8));
  c[2] = (float) (0xff & a);

  c[3] = (float) (b >> 16);
  c[4] = (float) (0xff & (b >> 8));
  c[5] = (float) (0xff & b);

  c[0] = (1.0-ratio) * c[0] + ratio * c[3];
  c[1] = (1.0-ratio) * c[1] + ratio * c[4];
  c[2] = (1.0-ratio) * c[2] + ratio * c[5];

  r[0] = (int) c[0];
  r[1] = (int) c[1];
  r[2] = (int) c[2];

  ya = (r[0]<<16) | (r[1]<<8) | r[2];
  return(YCbCr2RGB(ya));
}

/*
  gray
  builds the gray triplet with R=c, G=c, B=c

  input:  gray level (0-255)
  output: RGB triplet
  author: bergo
*/
int gray(int c) {
  return( (c<<16) | (c<<8) | c );
}

int randomColor() {
  static int seeded = 0;
  int a,b,c;

  if (!seeded) {
    srand(time(0));
    seeded = 1;
  }

  a = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
  b = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
  c = 1+(int) (255.0*rand()/(RAND_MAX+1.0));
  return(triplet(a,b,c));
}
