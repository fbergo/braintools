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

// $Id: dfilter.c,v 1.7 2013/11/11 22:34:12 bergo Exp $

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ip_optlib.h"
#include "ip_volume.h"
#include "ip_dfilter.h"
#include "ip_mem.h"

void BrightnessContrast(Volume *dest, Volume *src, float B, float C) {
  i16_t lookup[SDOM];
  int i,imax,dcenter;
  float center, scale, Y;

  if (B==0.0 && C==0.0)
    return;

  imax = VolumeMax(src);

  dcenter = (imax+1) / 2;

  center = dcenter + (B * dcenter) / 100.0;

  scale  = 1.0 + (C / 100.0);

  for(i=0;i<SDOM;i++) {
    Y = (float) i;
    Y = center + (Y-dcenter)*scale;
    if (Y<0.0) Y=0.0;
    if (Y>imax) Y=imax;
    lookup[i] = (i16_t) Y;
  }

  for(i=0;i<dest->N;i++)
    dest->u.i16[i] = lookup[ src->u.i16[i] ];
}

void GaussianStretch(Volume *dest, Volume *src, float mean, float dev) {
  float fgauss[SDOM];
  i16_t gauss[SDOM];
  float sq, var2;
  int i;  
  
  var2 = 2 * dev * dev;
  for(i=0;i<SDOM;i++) {
    sq = ((float)i-mean) * ((float)i-mean);
    fgauss[i]=(float)(SMAX*exp(-sq/var2));
    if (fgauss[i] > SMAX) fgauss[i] = SMAX;
    if (fgauss[i] < 0.0) fgauss[i] = 0.0;
    gauss[i] = (i16_t) fgauss[i];
  }

  for(i=0;i<src->N;i++)
    dest->u.i16[i] = gauss[src->u.i16[i]];
}

void Threshold(Volume *dest, Volume *src, int val, 
	       int blackbelow, int whiteabove)
{
  i16_t x,v,b,w;
  i16_t *below, *above;
  int i;

  v = (i16_t) val;
  x = 0;
  b = 0;
  w = (i16_t) VolumeMax(src);
  if (w < 255) w=255;

  if (blackbelow) below = &b; else below = &x;
  if (whiteabove) above = &w; else above = &x;

  for(i=0;i<src->N;i++) {
    x = src->u.i16[i];
    dest->u.i16[i] = (x < v) ? (*below) : (*above);
  }
}

/*
    [1  2  1] [2  4  2] [1  2  1] 
    [2  4  2] [4  8  4] [2  4  2] sum = 64
    [1  2  1] [2  4  2] [1  2  1] 
*/
void Blur3x3(Volume *dest, Volume *src) {

  static int K[3][3][3] = 
    { { {0,1,0}, {1,2,1}, {0,1,0} },
      { {1,2,1}, {2,3,2}, {1,2,1} },
      { {0,1,0}, {1,2,1}, {0,1,0} } };
  int dx[3],dy[3],dz[3];
  int i,j,k,m,a,b,c;

  dx[0] = -1;          dx[1] = 0; dx[2] = 1;
  dy[0] = -(src->W);   dy[1] = 0; dy[2] = src->W;
  dz[0] = -(src->WxH); dz[1] = 0; dz[2] = src->WxH;

  for(i=0;i<src->N;i++) {
    a = 0;
    c = 0;
    for(j=0;j<3;j++)
      for(k=0;k<3;k++)
	for(m=0;m<3;m++) {
	  b = i + dx[j]+dy[k]+dz[m];
	  if (b>=0 && b<src->N) {
	    a += src->u.i16[b] << K[j][k][m];
	    c += 1 << K[j][k][m];
	  }
	}
    if (!c) c=1;
    a /= c;
    dest->u.i16[i] = (i16_t) a;
  }
}

void Erode3x3(Volume *dest, Volume *src) {
  int i,j,k;
  int a,b,c;
  int p,q,dy,dz,dy1,dz1;
  i16_t minval;

  dy1 = src->tbrow[1];
  dz1 = src->tbframe[1];

  for(k=1;k<src->D-1;k++) {
    dz = src->tbframe[k];
    for(j=1;j<src->H-1;j++) {
      dy = src->tbrow[j];
      for(i=1;i<src->W-1;i++) {
	minval = 32767;

	p = i + dy + dz;

	for(c=-1;c<=1;c++)
	  for(b=-1;b<=1;b++)
	    for(a=-1;a<=1;a++) {
	      q = p + a + dy1*b + dz1*c;
	      if (src->u.i16[q] < minval)
		minval = src->u.i16[q];
	    }

	dest->u.i16[p] = minval;
      }
    }
  }
}

void Dilate3x3(Volume *dest, Volume *src) {
  int i,j,k;
  int a,b,c;
  int p,q,dy,dz,dy1,dz1;
  i16_t maxval;

  dy1 = src->tbrow[1];
  dz1 = src->tbframe[1];

  for(k=1;k<src->D-1;k++) {
    dz = src->tbframe[k];
    for(j=1;j<src->H-1;j++) {
      dy = src->tbrow[j];
      for(i=1;i<src->W-1;i++) {
	maxval = -32768;

	p = i + dy + dz;

	for(c=-1;c<=1;c++)
	  for(b=-1;b<=1;b++)
	    for(a=-1;a<=1;a++) {
	      q = p + a + dy1*b + dz1*c;
	      if (src->u.i16[q] > maxval)
		maxval = src->u.i16[q];
	    }

	dest->u.i16[p] = maxval;
      }
    }
  }
}

/* ignores negative values */
double * Histogram16(Volume *src) {
  int i;
  double *h;

  h = (double *) MemAlloc(32768 * sizeof(double));
  for(i=0;i<32768;i++)
    h[i] = 0.0;

  for(i=0;i<src->N;i++)
    if (src->u.i16[i] >= 0)
      h[(int) (src->u.i16[i])]++;

  return h;
}

double * HultHistogram16(Volume *src) {
  int i,j,x;
  double *h;
  i16_t v;

  h = (double *) MemAlloc(32768 * sizeof(double));
  for(i=0;i<32768;i++)
    h[i] = 0.0;

  for(x=src->W/2 - 2;x<=src->W/2 + 2;x++)
    for(j=0;j<src->D;j++)
      for(i=0;i<src->H;i++) {
	v = src->u.i16[x+src->tbrow[i]+src->tbframe[j]];
	if (v >= 0)
	  h[(int) v]++;
      }

  return h;
}

// kernel density estimation
double * KDE(double *h16) {
  double *tmp;
  int i,j,k;

  double kernel[31];
  double b,c;

  tmp = (double *) MemAlloc(32768 * sizeof(double));

  c= 2*(7.0 * 7.0);
  for(i=0;i<31;i++) {
    b = i-15.5;
    b *= b;
    kernel[i] = exp(-b/c);
  }
  c = 0.0;
  for(i=0;i<31;i++)
    c += kernel[i];
  for(i=0;i<31;i++)
    kernel[i] /= c;

  for(i=0;i<32768;i++) {
    tmp[i] = 0.0;
    for(j=0;j<31;j++) {
      k = i+j-15;
      if (k<0) k=0;
      if (k>32767) k=32767;
      tmp[i] += kernel[j] * h16[k];
    }
  }
  return tmp;
}

double * SplineInterp(double *h16) {
  double *d;
  double *t1;
  int i,j,n;
  double u,u3,u2,s;

  int *t2;
  d = (double *) MemAlloc(sizeof(double) * 32768);  
  for(i=0;i<32768;i++)
    d[i] = h16[i];

  t1 = (double *) MemAlloc(sizeof(double) * 32770);  
  t2 = (int *) MemAlloc(sizeof(int) * 32770);  

  t2[0] = 0;
  t1[0] = 0.0;
  n = 1;
  for(i=0;i<32768;i++)
    if (h16[i]!=0.0) {
      t1[n] = h16[i];
      t2[n] = i;
      ++n;
    }
  t1[n] = 0.0;
  t2[n] = 0;
  n++;

  //  printf("n=%d\n",n);

  /* interpolate spline from points t1[1] to t1[n-3] */

  s = 0.5;

  for(i=1;i<=n-3;i++) {

    for(j=t2[i]+1;j<t2[i+1];j++) {
      
      u = ((double) (j - t2[i])) / ((double)(t2[i+1]-t2[i]));

      //      printf("u=%.2f u0=%d u1=%d p=%d pk=%.2f,%.2f,%.2f,%.2f\n",u,t2[i],t2[i+1],j,t1[i-1],t1[i],t1[i+1],t1[i+2]);

      u2 = u * u;
      u3 = u2 * u;

      // Hearn, pp. 325, eq. 10-38
      d[j] = 
	t1[i-1] * (-s*u3+2*s*u2-s*u) +
	t1[i]   * ((2-s)*u3 + (s-3)*u2 + 1) +
	t1[i+1] * ((s-2)*u3 + (3-2*s)*u2 + s*u) +
	t1[i+2] * (s*u3 - s*u2);

    }

  }

  MemFree(t1);
  MemFree(t2);
  return d;
}

double * derivate(double *h) {
  double *d;
  int i;
  d = (double *) MemAlloc(sizeof(double) * 32768);
  d[0] = h[1] - h[0];
  d[32767] = h[32767] - h[32766];
  for(i=1;i<32767;i++)
    d[i] = (h[i+1] - h[i-1]) / 2.0;
  return d;
}

double * HistAvg(double *h, int t) {
  double *d;
  int i;
  double acc=0.0, dt;

  d = (double *) MemAlloc(sizeof(double) * 32768);
  for(i=0;i<32768;i++)
    d[i] = 0.0;

  dt = (double) t;

  for(i=0;i<32768;i++) {
    if (i>0 && i%t==0) {
      d[i-t] = acc / dt;
      acc = h[i];
    } else {
      acc += h[i];
    }
  }

  return d;

}

void  HistNormalize(double *h16) {
  int i;
  double max = -1.0;

  for(i=0;i<32768;i++)
    if (h16[i] > max) max = h16[i];
  if (max > 0.0)
    for(i=0;i<32768;i++)
      h16[i] /= max;
}

int MinBucket(double *h16) {
  int i;
  for(i=0;i<32767;i++)
    if (h16[i] > 0.0)
      return(i);
  return i;
}

int MaxBucket(double *h16) {
  int i;
  for(i=32767;i>0;i--)
    if (h16[i] > 0.0)
      return(i);
  return i;
}

double * CreateHistogram() {
  double *a;
  int i;
  a = (double *) MemAlloc(sizeof(double) * 32768);
  if (!a) return 0;
  for(i=0;i<32768;i++) a[i] = 0.0;
  return a;
}

void     DestroyHistogram(double *h16) {
  if (h16)
    MemFree(h16);
}
