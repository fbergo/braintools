/*

   LIBIP - Image Processing Library
   (C) 2002-2017
   
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

/**************************************************************
 $Id: linsolve.c,v 1.9 2013/11/11 22:34:13 bergo Exp $

 linsolve.c - volume registration, linear system solving

***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libip.h"

void homog_trans(double *dest, double *src, double transform[4][4]);

int SolveRegistration(double *result,
		      int ax,int ay,int az,
		      int bx,int by,int bz,
		      int cx,int cy,int cz,
		      int dx,int dy,int dz,
		      int ex,int ey,int ez,
		      int fx,int fy,int fz,
		      int gx,int gy,int gz,
		      int hx,int hy,int hz)
{
  double A[12][12], B[12], ARGH[12*12];
  int i,j;

  for(i=0;i<12;i++)
    for(j=0;j<12;j++)
      A[i][j] = 0.0;

  for(i=0;i<12;i++)
    result[i] = 0.0;

  // a-b
  A[0][0] = ax;     A[1][0]  = ay;     A[2][0]  = az;
  A[3][1] = ax;     A[4][1]  = ay;     A[5][1]  = az;
  A[6][2] = ax;     A[7][2]  = ay;     A[8][2]  = az;
  A[9][0] = 1.0;    A[10][1] = 1.0;    A[11][2] = 1.0;

  // c-d
  A[0][3] = cx;     A[1][3]  = cy;     A[2][3]  = cz;
  A[3][4] = cx;     A[4][4]  = cy;     A[5][4]  = cz;
  A[6][5] = cx;     A[7][5]  = cy;     A[8][5]  = cz;
  A[9][3] = 1.0;    A[10][4] = 1.0;    A[11][5] = 1.0;

  // e-f
  A[0][6] = ex;     A[1][6]  = ey;     A[2][6]  = ez;
  A[3][7] = ex;     A[4][7]  = ey;     A[5][7]  = ez;
  A[6][8] = ex;     A[7][8]  = ey;     A[8][8]  = ez;
  A[9][6] = 1.0;    A[10][7] = 1.0;    A[11][8] = 1.0;

  // g-h
  A[0][9]  = gx;     A[1][9]   = gy;     A[2][9]   = gz;
  A[3][10] = gx;     A[4][10]  = gy;     A[5][10]  = gz;
  A[6][11] = gx;     A[7][11]  = gy;     A[8][11]  = gz;
  A[9][9] = 1.0;     A[10][10] = 1.0;    A[11][11] = 1.0;

  for(i=0;i<12;i++)
    for(j=0;j<12;j++)
      ARGH[i*12+j] = A[j][i];
  
  i = minv(ARGH,12);
  if (i < 0) {
    printf("matrix not invertible\n");
    return -1;
  }

  B[0] = bx; B[1]  = by; B[2]  = bz;
  B[3] = dx; B[4]  = dy; B[5]  = dz;
  B[6] = fx; B[7]  = fy; B[8]  = fz;
  B[9] = hx; B[10] = hy; B[11] = hz;

  mmul(ARGH,B,12);
  for(i=0;i<12;i++)
    result[i] = B[i];

  return 0;
}

void htrans(double *dest, double *src, double *transform) {
  int i;

  for(i=0;i<4;i++) {
    dest[i] = 
      src[0]*transform[i*4+0] + src[1]*transform[i*4+1] +
      src[2]*transform[i*4+2] + src[3]*transform[i*4+3];
  }
}

void RTransform(double *dest, double *src) {
  dest[0+4*0] = src[0];
  dest[1+4*0] = src[1];
  dest[2+4*0] = src[2];
  dest[0+4*1] = src[3];
  dest[1+4*1] = src[4];
  dest[2+4*1] = src[5];
  dest[0+4*2] = src[6];
  dest[1+4*2] = src[7];
  dest[2+4*2] = src[8];
  dest[3+4*0] = src[9];
  dest[3+4*1] = src[10];
  dest[3+4*2] = src[11];
  dest[0+4*3] = dest[1+4*3] = dest[2+4*3] = 0.0;
  dest[3+4*3] = 1.0; 
}

Volume *VolumeRegistrationTransform(Volume *src, double *coeffs)
{
  Volume *dest;
  int i,j,k,a,b,c;
  int W,H,D;
  double T[16], P[4], Q[4];

  RTransform(T,coeffs);

  W = src->W;
  H = src->H;
  D = src->D;
  dest = VolumeNew(W,H,D,vt_integer_16);
  if (!dest) return 0;

  for(k=0;k<D;k++)
    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	P[0] = i;
	P[1] = j;
	P[2] = k;
	P[3] = 1.0;
	htrans(Q,P,T);
	a = (int) Q[0];
	b = (int) Q[1];
	c = (int) Q[2];
	if (a < 0 || b < 0 || c < 0 ||
	    a >= W || b >= H || c >= D)
	  continue;
	VolumeSetI16(dest,i,j,k,VolumeGetI16(src,a,b,c));
      }

  return dest;
}

/* a = {n x n}, b = {1 x n}, result a*b in b */
void mmul(double *a,double *b,int n) {
  int i,j;
  static double *r = 0;
  static int rsize = 0;

  if (rsize < n) {
    if (r) MemFree(r);
    rsize = n;
    r = (double *) MemAlloc(sizeof(double) * rsize);
  }

  for(i=0;i<n;i++) {
    r[i] = 0.0;
    for(j=0;j<n;j++)
      r[i] += a[i*n+j] * b[j];
  }
  for(i=0;i<n;i++)
    b[i] = r[i];
}

int minv(double *a, int n) {
  static int *sle = 0;
  static double *sq0 = 0;
  static int rsize = 0;

  int lc,*le; double s,t,tq=0.,zr=1.e-15;
  double *pa,*pd,*ps,*p,*q,*q0;
  int i,j,k,m;

  if (rsize < n) {
    if (sle) MemFree(sle);
    if (sq0) MemFree(sq0);
    rsize = n;
    sle=(int *)MemAlloc(rsize*sizeof(int));
    sq0=(double *)MemAlloc(rsize*sizeof(double));
  }
  le = sle;
  q0 = sq0;

  for(j=0,pa=pd=a; j<n ;++j,++pa,pd+=n+1){
    if(j>0){
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
      for(i=1; i<n ;++i){ lc=i<j?i:j;
      for(k=0,p=pa+i*n-j,q=q0,t=0.; k<lc ;++k) t+= *p++ * *q++;
      q0[i]-=t;
      }
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
    }
    
    s=fabs(*pd); lc=j;
    for(k=j+1,ps=pd; k<n ;++k){
      if((t=fabs(*(ps+=n)))>s){ s=t; lc=k;}
    }
    tq=tq>s?tq:s; if(s<zr*tq){ return -1;}
    *le++ =lc;
    if(lc!=j){
      for(k=0,p=a+n*j,q=a+n*lc; k<n ;++k){
        t= *p; *p++ = *q; *q++ =t;
      }
    }
    for(k=j+1,ps=pd,t=1./ *pd; k<n ;++k) *(ps+=n)*=t;
    *pd=t;
  }
  for(j=1,pd=ps=a; j<n ;++j){
    for(k=0,pd+=n+1,q= ++ps; k<j ;++k,q+=n) *q*= *pd;
  }
  for(j=1,pa=a; j<n ;++j){ ++pa;
  for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *q++ = *p;
  for(k=0; k<j ;++k){ t=0.;
  for(i=k,p=pa+k*n+k-j,q=q0+k; i<j ;++i) t-= *p++ * *q++;
  q0[k]=t;
  }
  for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *p= *q++;
  }
  for(j=n-2,pd=pa=a+n*n-1; j>=0 ;--j){ --pa; pd-=n+1;
  for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *q++ = *p;
  for(k=n-1,ps=pa; k>j ;--k,ps-=n){ t= -(*ps);
  for(i=j+1,p=ps,q=q0; i<k ;++i) t-= *++p * *q++;
  q0[--m]=t;
  }
  for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *p= *q++;
  }
  for(k=0,pa=a; k<n-1 ;++k,++pa){
    for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
    for(j=0,ps=a; j<n ;++j,ps+=n){
      if(j>k){ t=0.; p=ps+j; i=j;}
      else{ t=q0[j]; p=ps+k+1; i=k+1;}
      for(; i<n ;) t+= *p++ *q0[i++];
      q0[j]=t;
    }
    for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
  }
  for(j=n-2,le--; j>=0 ;--j){
    for(k=0,p=a+j,q=a+ *(--le); k<n ;++k,p+=n,q+=n){
      t=*p; *p=*q; *q=t;
    }
   }
  return 0;
}

int   minvf(float *a, int n) {
  static int *sle = 0;
  static float *sq0 = 0;
  static int rsize = 0;

  int lc,*le; float s,t,tq=0.,zr=1.e-15;
  float *pa,*pd,*ps,*p,*q,*q0;
  int i,j,k,m;

  if (rsize < n) {
    if (sle) MemFree(sle);
    if (sq0) MemFree(sq0);
    rsize = n;
    sle=(int *)MemAlloc(rsize*sizeof(int));
    sq0=(float *)MemAlloc(rsize*sizeof(float));
  }
  le = sle;
  q0 = sq0;

  for(j=0,pa=pd=a; j<n ;++j,++pa,pd+=n+1){
    if(j>0){
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
      for(i=1; i<n ;++i){ lc=i<j?i:j;
      for(k=0,p=pa+i*n-j,q=q0,t=0.; k<lc ;++k) t+= *p++ * *q++;
      q0[i]-=t;
      }
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
    }
    
    s=fabs(*pd); lc=j;
    for(k=j+1,ps=pd; k<n ;++k){
      if((t=fabs(*(ps+=n)))>s){ s=t; lc=k;}
    }
    tq=tq>s?tq:s; if(s<zr*tq){ return -1;}
    *le++ =lc;
    if(lc!=j){
      for(k=0,p=a+n*j,q=a+n*lc; k<n ;++k){
        t= *p; *p++ = *q; *q++ =t;
      }
    }
    for(k=j+1,ps=pd,t=1./ *pd; k<n ;++k) *(ps+=n)*=t;
    *pd=t;
  }
  for(j=1,pd=ps=a; j<n ;++j){
    for(k=0,pd+=n+1,q= ++ps; k<j ;++k,q+=n) *q*= *pd;
  }
  for(j=1,pa=a; j<n ;++j){ ++pa;
  for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *q++ = *p;
  for(k=0; k<j ;++k){ t=0.;
  for(i=k,p=pa+k*n+k-j,q=q0+k; i<j ;++i) t-= *p++ * *q++;
  q0[k]=t;
  }
  for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *p= *q++;
  }
  for(j=n-2,pd=pa=a+n*n-1; j>=0 ;--j){ --pa; pd-=n+1;
  for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *q++ = *p;
  for(k=n-1,ps=pa; k>j ;--k,ps-=n){ t= -(*ps);
  for(i=j+1,p=ps,q=q0; i<k ;++i) t-= *++p * *q++;
  q0[--m]=t;
  }
  for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *p= *q++;
  }
  for(k=0,pa=a; k<n-1 ;++k,++pa){
    for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
    for(j=0,ps=a; j<n ;++j,ps+=n){
      if(j>k){ t=0.; p=ps+j; i=j;}
      else{ t=q0[j]; p=ps+k+1; i=k+1;}
      for(; i<n ;) t+= *p++ *q0[i++];
      q0[j]=t;
    }
    for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
  }
  for(j=n-2,le--; j>=0 ;--j){
    for(k=0,p=a+j,q=a+ *(--le); k<n ;++k,p+=n,q+=n){
      t=*p; *p=*q; *q=t;
    }
   }
  return 0;
}

void  mmulf(float *a,float *b,int n) {
  int i,j;
  static float *r = 0;
  static int rsize = 0;

  if (rsize < n) {
    if (r) MemFree(r);
    rsize = n;
    r = (float *) MemAlloc(sizeof(float) * rsize);
  }

  for(i=0;i<n;i++) {
    r[i] = 0.0;
    for(j=0;j<n;j++)
      r[i] += a[i*n+j] * b[j];
  }
  for(i=0;i<n;i++)
    b[i] = r[i];
}

/* 
   J.H. Mathews, 
   Numerical Methods for Mathematics, Science and Engineering,
   Prentice Hall, 2nd Edition, pg. 264
   519.4 / M424n / IMECC
*/

void least_squares(i16_t *varx, i16_t *vary, int n, float *a, float *b) {
  float Xmean, Ymean, SumX, SumXY, tmp, A, B;
  int Xsum, Ysum;
  int i;

  Xsum = Ysum = 0;
  for(i=0;i<n;i++) {
    Xsum += (int) varx[i];
    Ysum += (int) vary[i];
  }
  Xmean = ((float) Xsum) / ((float) n);
  Ymean = ((float) Ysum) / ((float) n);

  SumX  = 0.0;
  SumXY = 0.0;
  for(i=0;i<n;i++) {
    tmp = ((float)(varx[i])) - Xmean;
    SumX += tmp*tmp;
    tmp = (((float)(varx[i])) - Xmean) *
      (((float)(vary[i])) - Ymean);
    SumXY += tmp;
  }

  A = SumXY / SumX;
  B = Ymean - A*Xmean;

  *a = A;
  *b = B;
}

/* force B=0 */
void  pseudo_least_squares(i16_t *varx, i16_t *vary, int n, float *a, float *b)
{
  int i;
  float s,t,w;

  t = 0.0;
  w = 0.0;
  for(i=0;i<n;i++) {
    if (varx[i] > 0) {
      s = ((float)((vary[i]))) / ((float)((varx[i])));
      t += s;
      w += 1.0;
    }
  }
  
  if (w>0.0)
    t /= (float) w;
  *a = t;
  *b = 0.0;
}
