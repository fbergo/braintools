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
#include <time.h>
#include <math.h>
#include "ip_adjacency.h"
#include "ip_avfilter.h"
#include "ip_optlib.h"
#include "ip_linsolve.h"
#include "ip_mem.h"

/*
   0 1 2     9 10 11
   3 4 5    12 13 14 
   6 7 8    15 16 17

*/

int vclip(int v,int min,int max) {
  if (v<min) return min;
  if (v>max) return max;
  return v;
}

// 3D Sobel gradient
void SobelGradient(XAnnVolume *vol) {
  int i,m,maxN,N;
  Volume *tmp;
  int dx,dy,dz;
  int acc[3],loc;

  static int n0[9]={-1, 0, 1,-1, 0, 1,-1, 0, 1};
  static int n1[9]={-1,-1,-1, 0, 0, 0, 1, 1, 1};
  static int we[9]={ 1, 2, 1, 2, 4, 2, 1, 2, 1};

  N = vol->N;
  maxN = N -1;

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);

  dx = 1;
  dy = vol->W;
  dz = vol->W * vol->H;

  for(i=0;i<N;i++) {
	
    // xy plane
    acc[0] = 0;
    for(m=0;m<9;m++) {
      loc = we[m] * (vol->vd[ vclip(i+dz+dx*n0[m]+dy*n1[m],0,maxN) ].value -
		     vol->vd[ vclip(i-dz+dx*n0[m]+dy*n1[m],0,maxN) ].value);
      acc[0] += loc;
    }
    acc[0] /= 16;
    acc[0] *= acc[0];

    // yz plane
    acc[1] = 0;
    for(m=0;m<9;m++) {
      loc = we[m] * (vol->vd[ vclip(i+dx+dz*n0[m]+dy*n1[m],0,maxN) ].value -
		     vol->vd[ vclip(i-dx+dz*n0[m]+dy*n1[m],0,maxN) ].value);
      acc[1] += loc;
    }
    acc[1] /= 16;
    acc[1] *= acc[1];

    // xz plane
    acc[2] = 0;
    for(m=0;m<9;m++) {
      loc = we[m] * (vol->vd[ vclip(i+dy+dx*n0[m]+dz*n1[m],0,maxN) ].value -
		     vol->vd[ vclip(i-dy+dx*n0[m]+dz*n1[m],0,maxN) ].value);
      acc[2] += loc;
    }
    acc[2] /= 16;
    acc[2] *= acc[2];

    tmp->u.i16[i] = (short int) vclip( (int) (sqrt(acc[0]+acc[1]+acc[2])), 0, 32766 );
  }

  for(i=0;i<N;i++)
    vol->vd[i].value = tmp->u.i16[i];

  VolumeDestroy(tmp);
}

/*
  morphological gradient
  calculates in one pass (dilation - erosion) of vol->vd[].value
  and places output in vol->vd[].value

  it'll be slightly wrong at the bounds of the image, for faster
  calculation
*/

void GenericFastMorphologicalGradient(XAnnVolume *vol, float R) {
  AdjRel *A;
  int i,j,k;
  int dil, ero, value;
  Volume *tmp;

  A=Spherical(R);
  AdjCalcDN(A,vol->W,vol->H,vol->D);

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  for(i=0;i<vol->N;i++)
    tmp->u.i16[i] = vol->vd[i].value;
  
  for(i=0; i< vol->N; i++) {
    dil = ero = tmp->u.i16[i];

    value = 0;
    for(j=0; j< A->n ; j++) {
      k = i + A->dn[j];

      if (k<0 || k>=vol->N) continue;

      value = tmp->u.i16[k];

      if (value > dil)
	dil = value; 
      else if (value < ero) 
	ero = value;
    }

    value = dil - ero;
    if (value > FMAX) value = FMAX; else if (value < 0) value = 0;

    vol->vd[i].value = (vtype) value;
  }

  VolumeDestroy(tmp);
  DestroyAdjRel(&A);
}

/* N_7 */
void FastMorphologicalGradient(XAnnVolume *vol) {
  GenericFastMorphologicalGradient(vol, 1.0000);
}

/* N_27 */
void FastBoxMorphologicalGradient(XAnnVolume *vol) {
  GenericFastMorphologicalGradient(vol, 1.7400);
}

void FastDirectionalMaxGradient(XAnnVolume *vol) {
  int i,j,ny,nz;
  Volume *dx;
  i16_t s0;

  dx = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  VolumeFill(dx, 0);

  ny = vol->W;
  nz = vol->W * vol->H;

  /* dx */
  j = vol->N - 1;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+1].value;
    dx->u.i16[i] = s0 > 0 ? s0 : -s0;
  }

  /* dy */
  j = vol->N - ny;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+ny].value;
    if (s0 < 0) s0 = -s0;
    if (s0 > dx->u.i16[i]) dx->u.i16[i] = s0;
  }

  /* dz */
  j = vol->N - nz;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+nz].value;
    if (s0 < 0) s0 = -s0;
    if (s0 > dx->u.i16[i]) dx->u.i16[i] = s0;
  }

  j = vol->N;
  for(i=0;i<j;i++)
    vol->vd[i].value = dx->u.i16[i];

  VolumeDestroy(dx);
}

void FastDirectionalAvgGradient(XAnnVolume *vol) {
  int i,j,k,ny,nz;
  Volume *dx;
  i16_t s0;

  dx = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  VolumeFill(dx, 0);

  ny = vol->W;
  nz = vol->W * vol->H;

  /* dx */
  j = vol->N - 1;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+1].value;
    dx->u.i16[i] = s0 > 0 ? s0 : -s0;
  }

  /* dy */
  j = vol->N - ny;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+ny].value;
    if (s0 < 0) s0 = -s0;
    k = dx->u.i16[i];
    k += s0;
    dx->u.i16[i] = (i16_t) (k/2);
  }

  /* dz */
  j = vol->N - nz;
  for(i=0;i<j;i++) {
    s0 = vol->vd[i].value - vol->vd[i+nz].value;
    if (s0 < 0) s0 = -s0;
    k = 2 * ((int)(dx->u.i16[i]));
    k += s0;
    dx->u.i16[i] = (i16_t) (k/3);
  }

  j = vol->N;
  for(i=0;i<j;i++)
    vol->vd[i].value = dx->u.i16[i];

  VolumeDestroy(dx);
}

void MorphologicalGradient(XAnnVolume *vol) {
  AdjRel *A;
  int i,j;
  int dil, ero, value;
  Voxel p,q;
  Volume *tmp;

  A=Spherical(1.0);

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  for(i=0;i<vol->N;i++)
    tmp->u.i16[i] = vol->vd[i].value;
  
  for(i=0; i< vol->N; i++) {
    XAVolumeVoxelOf(vol, i, &p);

    dil = ero = tmp->u.i16[i];
    
    value = 0;
    for(j=0; j< A->n ; j++) {
      q.x = p.x + A->dx[j];
      q.y = p.y + A->dy[j];
      q.z = p.z + A->dz[j];

      if (!XAValidVoxel(vol, &q))
        continue;

      value = tmp->u.i16[q.x + tmp->tbrow[q.y] + tmp->tbframe[q.z]];

      if (value > dil)
	dil = value;
      else if (value < ero) 
	ero = value;
    }

    value = dil - ero;
    if (value > FMAX) value = FMAX; else if (value < 0)   value = 0;

    vol->vd[i].value = (vtype) value;
  }

  VolumeDestroy(tmp);
  DestroyAdjRel(&A);
}

i16_t * XA_LS_Function(float B, float C,int maxval) {
  i16_t *lookup;
  int i,dcenter;
  float center, scale, Y;

  lookup = (i16_t *) MemAlloc(SDOM * sizeof(i16_t));
  if (!lookup) return 0;

  if (B==0.0 && C==0.0) {
    for(i=0;i<SDOM;i++) lookup[i] = i;
    return lookup;
  }

  dcenter = (maxval + 1) / 2;
  center  = dcenter + (B * dcenter) / 100.0;
  scale   = 1.0 + (C / 100.0);

  MemSet(lookup,0,sizeof(i16_t)*SDOM);

  for(i=0;i<maxval;i++) {
    Y = (float) i;
    Y = center + (Y-dcenter)*scale;
    if (Y<0.0) Y=0.0; if (Y>maxval) Y=maxval;
    lookup[i] = (i16_t) Y;
  }
  
  return lookup;
}


void XA_LS_Filter(XAnnVolume *vol, float B, float C) {
  i16_t *lookup;

  if (B==0.0 && C==0.0)
    return;

  lookup = XA_LS_Function(B,C,XAGetValueMax(vol));
  if (!lookup) return;
  XA_Lookup_Filter(vol,lookup);
  MemFree(lookup);
}

void XA_LS_Filter_Preview(Volume *vol, float B, float C,
			  int x,int y,int z)
{
  i16_t *lookup;

  if (B==0.0 && C==0.0)
    return;

  lookup = XA_LS_Function(B,C,VolumeMax(vol));
  if (!lookup) return;
  XA_Lookup_Filter_Preview(vol,lookup,x,y,z);
  MemFree(lookup);
}

void XA_Lookup_Filter(XAnnVolume *vol, i16_t *lookup) 
{
  int i;
  for(i=0;i<vol->N;i++)
    vol->vd[i].value = lookup[ vol->vd[i].value ];
}

void XA_Lookup_Filter_Preview(Volume *vol, i16_t *lookup,
			      int x,int y,int z)
{
  int i,j,k;

  // 20040602 FIXME: voxel (x,y,z) gets transformed 3 times

  i = x;
  for(j=0;j<vol->H;j++)
    for(k=0;k<vol->D;k++)
      vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ] =
	lookup[vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ]];

  j = y;
  for(i=0;i<vol->W;i++)
    for(k=0;k<vol->D;k++)
      vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ] =
	lookup[vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ]];

  k = z;
  for(j=0;j<vol->H;j++)
    for(i=0;i<vol->W;i++)
      vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ] =
	lookup[vol->u.i16[ i + vol->tbrow[j] + vol->tbframe[k] ]];
}

i16_t * XA_GS_Function(float mean, float dev) {
  i16_t *lookup;
  float t, var2;
  int i;

  lookup = (i16_t *) MemAlloc(SDOM * sizeof(i16_t));
  if (!lookup) return 0;
   
  var2 = 2 * dev * dev;
  for(i=0;i<SDOM;i++) {
    t = ((float)i-mean) * ((float)i-mean);
    t = (float)(FMAX*exp(-t/var2));
    if (t > FMAX) t = FMAX;
    if (t < 0.0)  t = 0.0;
    lookup[i] = (i16_t) t;
  }

  return(lookup);
}

void XA_GS_Filter(XAnnVolume *vol, float mean, float dev) {
  i16_t *lookup;
  lookup = XA_GS_Function(mean, dev);
  if (!lookup) return;
  XA_Lookup_Filter(vol,lookup);
  MemFree(lookup);
}

void XA_GS_Filter_Preview(Volume *vol, float mean, float dev,
			  int x,int y,int z) 
{
  i16_t *lookup;
  lookup = XA_GS_Function(mean, dev);
  if (!lookup) return;
  XA_Lookup_Filter_Preview(vol,lookup,x,y,z);
  MemFree(lookup);
}

i16_t *  XA_MBGS_Function(int bands, float *means, float *devs) {
  i16_t *lookup;
  float G, sq, var2[10];
  float offset,range,cmax;  
  int i,j,k;

  lookup = (i16_t *) MemAlloc(SDOM * sizeof(i16_t));
  if (!lookup) return 0;
   
  if (bands < 1 || bands > 5)
    return 0;

  offset = (float) (SDOM / bands);
  range  = offset - 1.0;

  for(i=0;i<bands;i++)
    var2[i] = 2 * devs[i] * devs[i];

  for(i=0;i<SDOM;i++) {
    k=0;
    cmax = -1.0;
    for(j=0;j<bands;j++) {
      sq = ((float)i-means[j]);
      sq *= sq;
      G = (float) (range * exp(-sq/var2[j]));
      if (G > range) G = range;
      if (G < 0.0)   G = 0.0;
      if (G > cmax) {
	cmax = G;
	k    = j;
      }
    }
    lookup[i] = (i16_t) ((offset * k) + cmax);
  }

  return(lookup);
}

void XA_MBGS_Filter(XAnnVolume *vol, int bands, float *means, float *devs) {
  i16_t *lookup;
  lookup = XA_MBGS_Function(bands, means, devs);
  if (!lookup) return;
  XA_Lookup_Filter(vol,lookup);
  MemFree(lookup);
}

void XA_MBGS_Filter_Preview(Volume *vol, int bands, float *means, float *devs,
			    int x,int y,int z)
{
  i16_t *lookup;
  lookup = XA_MBGS_Function(bands, means, devs);
  if (!lookup) return;
  XA_Lookup_Filter_Preview(vol,lookup,x,y,z);
  MemFree(lookup);
}

i16_t *  XA_Threshold_Function(int lower,int upper) 
{
  i16_t *lookup;
  int i;

  if (lower > upper) {
    i = lower;
    lower = upper;
    upper = i;
  }

  if (lower < 0) lower = 0;
  if (upper >= SDOM) upper = SDOM - 1;

  lookup = (i16_t *) MemAlloc(SDOM * sizeof(i16_t));
  if (!lookup) return 0;

  MemSet(lookup, 0, SDOM * sizeof(i16_t));

  for(i=lower;i<=upper;i++)
    lookup[i] = (i16_t) i;

  return(lookup);
}

void     XA_Threshold_Filter(XAnnVolume *vol, int lower, int upper) 
{
  i16_t *lookup;
  lookup = XA_Threshold_Function(lower, upper);
  if (!lookup) return;
  XA_Lookup_Filter(vol,lookup);
  MemFree(lookup);
}

void     XA_Threshold_Filter_Preview(Volume *vol, int lower, int upper,
				 int x,int y,int z)
{
  i16_t *lookup;
  lookup = XA_Threshold_Function(lower, upper);
  if (!lookup) return;
  XA_Lookup_Filter_Preview(vol,lookup,x,y,z);
  MemFree(lookup);
}


void XABlur3x3(XAnnVolume *vol) {

  static int K[3][3][3] = 
    { { {0,1,0}, {1,2,1}, {0,1,0} },
      { {1,2,1}, {2,3,2}, {1,2,1} },
      { {0,1,0}, {1,2,1}, {0,1,0} } };
  int dx[3],dy[3],dz[3];
  int i,j,k,m,a,b,c;
  Volume *tmp;

  dx[0] = -1;          dx[1] = 0; dx[2] = 1;
  dy[0] = -(vol->W);   dy[1] = 0; dy[2] = vol->W;
  dz[0] = -(vol->WxH); dz[1] = 0; dz[2] = vol->WxH;

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  for(i=0;i<vol->N;i++)
    tmp->u.i16[i] = vol->vd[i].value;

  for(i=0;i<vol->N;i++) {
    a = 0;
    c = 0;
    for(j=0;j<3;j++)
      for(k=0;k<3;k++)
	for(m=0;m<3;m++) {
	  b = i + dx[j]+dy[k]+dz[m];
	  if (b>=0 && b<vol->N) {
	    a += tmp->u.i16[b] << K[j][k][m];
	    c += 1 << K[j][k][m];
	  }
	}
    if (!c) c=1;
    a /= c;
    vol->vd[i].value = (vtype) a;
  }

  VolumeDestroy(tmp);

}

void XAModa(XAnnVolume *vol) {
  int i,j,b;

  int block[128];
  int n;
  int bc,bd;
  char *ct;

  Volume *tmp;
  AdjRel *A;

  A = Spherical(1.74);
  AdjCalcDN(A, vol->W, vol->H, vol->D);

  ct = (char *) MemAlloc(32768);
  MemSet(ct,0,32768);

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  for(i=0;i<vol->N;i++)
    tmp->u.i16[i] = vol->vd[i].value;

  for(i=0;i<vol->N;i++) {

    n = 0;

    for(j=0;j<A->n;j++) {
      b = i + A->dn[j];
      if (b>=0 && b<vol->N)
	block[n++] = tmp->u.i16[b];
    }

    bc = 1;
    bd = 0;
    for(j=0;j<n;j++)
      if (++ct[block[j]] > bc) {
	bc = ct[block[j]];
	bd = block[j];
      }

    for(j=0;j<n;j++)
      ct[block[j]] = 0;

    if (bc>2)
      vol->vd[i].value = (vtype) bd;
  }
  
  VolumeDestroy(tmp);
  DestroyAdjRel(&A);
  MemFree(ct);
}

/* tests:

    03andrea-ci.scn :  

                       selection sort               20.7 secs (kept)
                       heapsort                     21.5 secs
                       bubble sort                  26.5 secs
                       quicksort (with extern call) 45.3 secs

*/

int intcmp(const void *a, const void *b) {
  const int *c = (const int *) a;
  const int *d = (const int *) b;
  if (*c < *d) return -1;
  if (*c == *d) return 0;
  return 1;
}

void XAMedian(XAnnVolume *vol) {
  int i,j,k,m,b,z;

  int block[128];
  int n;

  //  int l,r,hsz;

  Volume *tmp;
  AdjRel *A;

  A = Spherical(1.74);
  AdjCalcDN(A, vol->W, vol->H, vol->D);

  tmp = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);
  for(i=0;i<vol->N;i++)
    tmp->u.i16[i] = vol->vd[i].value;

  for(i=0;i<vol->N;i++) {

    n = 0;

    for(j=0;j<A->n;j++) {
      b = i + A->dn[j];
      if (b>=0 && b<vol->N)
	block[n++] = tmp->u.i16[b];
    }

    /*
    // heapsort
    hsz = n;
    for(j=1+hsz/2;j>=0;j--) {

      /// heapify j
      k = j;
      for(;;) {
	l = (2*(k+1)) - 1;
	r = l + 1;

	if (l < hsz && block[l] > block[k])
	  m = l;
	else
	  m = k;

	if (r < hsz && block[r] > block[m])
	  m = r;

	if (m != k) {
	  z = block[k];
	  block[k] = block[m];
	  block[m] = z; // swap k,m
	  k = m;
	} else break;
      } // heapify
    } // for j (build-heap)

    for(j=hsz-1;j>=1;j--) {
      z = block[j];
      block[j] = block[0];
      block[0] = z; // swap 0,j
      hsz--;
      
      k = 0;
      for(;;) {

	l = (2*(k+1)) - 1;
	r = l + 1;

	if (l < hsz && block[l] > block[k])
	  m = l;
	else
	  m = k;

	if (r < hsz && block[r] > block[m])
	  m = r;

	if (m!=k) {
	  z = block[k];
	  block[k] = block[m];
	  block[m] = z; // swap k,m
	  k = m;
	} else break;
      } // heapify
    } // for j (sort)
    */

    /*
    // bubble sort
    for(j=0;j<n-1;j++) {
      for(k=j+1;k<n;k++) {
	if (block[j] > block[k]) {
	  b = block[j];
	  block[j] = block[k];
	  block[k] = b;
	}
      }
    }
    */

    // selection sort
    for(j=0;j<n;j++) {
      z = block[j];
      m = j;
      for(k=j+1;k<n;k++)
	if (block[k] < z) { z = block[k]; m = k; }
      z = block[j];
      block[j] = block[m];
      block[m] = z;
    }

    /*
    // quicksort with library call
    qsort(block,n,sizeof(int),intcmp);
    */

    vol->vd[i].value = block[n/2];

  }
  
  VolumeDestroy(tmp);
  DestroyAdjRel(&A);
}

int bpp_walk_left(int *H,int x0, int tol) {
  int downcount = 0;
  int x;
  int peakx, cmax, mt=0;

  cmax = H[x0];
  peakx = x0;
  for(x=x0-1;x>=0;x--) {
    if (H[x] > cmax) { 
      cmax = H[x];
      peakx = x;
      mt = (8*cmax) / 10;
    }
    if (H[x] < mt) ++downcount; else downcount=0;
    if (downcount > tol) return peakx;
  }
  return 0;
}

float ratio(int num, int denom) {
  return( ((float)num) / ((float)denom) );
}

int xacc(int *A, int x0,int x1) {
  if (x1 > 32767) x1=32767;
  if (x0<=0) {
    return (A[x1]);
  } else {
    return (A[x1] - A[x0-1]);
  }  
}

void XABrainGaussianAutoEstimate(XAnnVolume *vol, float *mean, float *stddev)
{
  XABrainGaussianEstimate(vol, 0.1700, mean, stddev);
}

/* perc should be between 0.15 and 0.20 for optimal results */
void XABrainGaussianEstimate(XAnnVolume *vol, float perc, 
			     float *mean, float *stddev)
{
  int i;
  int xmax, n99, x99, m, s;
  int *H,*tmp,*A;
  float r;

  *mean   = 128.0;
  *stddev = 128.0;

  H   = (int *) MemAlloc(32768 * sizeof(int));
  if (!H) return;
  tmp = (int *) MemAlloc(32768 * sizeof(int));
  if (!tmp) { MemFree(H); return; }
  A   = (int *) MemAlloc(32768 * sizeof(int));
  if (!A) { MemFree(tmp); MemFree(H); return; }

  // calculate histogram
  MemSet(H,0,32768*sizeof(int));
  for(i=0;i<vol->N;i++)
    ++H[ vol->vd[i].orig ];

  // smooth histogram
  MemCpy(tmp,H,sizeof(int) * 32768);
  H[0] = (4*tmp[0] + 2*tmp[1] + tmp[2]) / 10;
  for(i=2;i<32766;i++)
    H[i] = (tmp[i-2] + 2*tmp[i-1] + 4*tmp[i] + 2*tmp[i+1] + tmp[i+2]) / 10;

  // accumulate histogram
  MemSet(A,0,32768*sizeof(int));
  A[0] = H[0];
  for(i=1;i<32768;i++)
    A[i] = A[i-1] + H[i];
  
  // calc limits
  n99 = (A[32767] * 99) / 100;

  xmax = 0;
  for(i=0;i<32768;i++)
    if (H[i]!=0) xmax=i;

  x99 = 0;
  for(i=0;i<xmax;i++) {
    if (A[i] < n99)
      x99 = i;
    else
      break;
  }

  m = bpp_walk_left(H, x99, 32);

  // grow stddev until we get perc of the voxels on the volume
  r = 0.0;
  s = 1;
  while( r < perc ) {
    ++s;
    r = ratio ( xacc(A, m-s, m+s) , A[32767] );
  }

  *mean   = (float) m;
  *stddev = (float) s;

  MemFree(A);
  MemFree(tmp);
  MemFree(H);
}

/* affinity filters */

void XAMaxAffinity(XAnnVolume *vol) {
  int npoints;
  int *P,*L;
  Volume *delta, *aff, *root;

  BMap   *gray;
  int    *fifo;
  int    getq, putq;

  AdjRel *A3,*A1;

  Voxel v,w;
  short int a,b,c;
  unsigned char u;

  int i,j,k,r,s,t;
  float f;

  npoints = MapSize(vol->addseeds);
  if (!npoints) return;

  P     = (int *)   MemAlloc(sizeof(int) * npoints);
  L     = (int *)   MemAlloc(sizeof(int) * npoints);

  /* read seeds without destroying the set */
  for(i=0;i<npoints;i++) {
    MapGet(vol->addseeds, i, &j, &k);
    P[i] = j; /* voxel address */
    L[i] = k; /* label */
  }

  delta = VolumeNew(vol->W,vol->H,vol->D,vt_float_32);
  VolumeFillF(delta, 2000.0);

  aff   = VolumeNew(vol->W,vol->H,vol->D,vt_integer_8);
  root  = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);

  /* initialize fifo */
  gray = BMapNew(vol->N);
  BMapFill(gray, 0);
  fifo = (int *) MemAlloc(sizeof(int) * vol->N);
  putq = getq = 0;

  /* calculate delta for the seeds */

  A3 = Spherical(3.0);
  AdjCalcDN(A3, vol->W, vol->H, vol->D);  

  for(i=0;i<npoints;i++) {
    XAVolumeVoxelOf(vol, P[i], &v);
    a = 32767;
    b = 0;
    for(j=0;j<A3->n;j++) {
      w.x = v.x + A3->dx[j];
      w.y = v.y + A3->dy[j];
      w.z = v.z + A3->dz[j];
      if (XAValidVoxel(vol, &w)) {
	c = vol->vd[ P[i]+A3->dn[j] ].value;
	if (c<a) a = c;
	if (c>b) b = c;
      }
    }
    if (b==a) ++b;
    delta->u.f32[ P[i] ] = 2.0 * ((float)(b-a)) * ((float)(b-a));
  }

  DestroyAdjRel(&A3);

  /* initialize roots and affinities, insert seeds in the fifo */
  VolumeFill(aff, 0);

  for(i=0;i<npoints;i++) {
    aff->u.i8[ P[i] ]   = 255;
    root->u.i16[ P[i] ] = i;

    fifo[putq++] = P[i];
    BMapSet(gray, P[i], 1);
  }

  A1 = Spherical(1.0);
  AdjCalcDN(A1, vol->W, vol->H, vol->D);  

  while(putq != getq) {
    s = fifo[getq];
    getq = (getq + 1) % vol->N;

    BMapSet(gray, s, 0);

    r = P [ root->u.i16[s] ];

    XAVolumeVoxelOf(vol, s, &v);
    for(i=1;i<A1->n;i++) {
      w.x = v.x + A1->dx[i];
      w.y = v.y + A1->dy[i];
      w.z = v.z + A1->dz[i];
      if (XAValidVoxel(vol, &w)) {
	t = s + A1->dn[i];
	
	f = ( vol->vd[r].value - vol->vd[t].value );
	f = 255.0 * exp( - f*f / delta->u.f32[r] );
	u = (unsigned char) f;
	if (u==0) u=1;
	
	if (u > aff->u.i8[t]) {
	  root->u.i16[t] = root->u.i16[s];
	  aff->u.i8[t]  = u;
	  if (!BMapGet(gray, t)) {
	    fifo[putq] = t;
	    putq = (putq + 1) % vol->N;
	    BMapSet(gray, t, 1);
	  }
	}

      }
    } // for i
  } // putq!=getq

  /* map result to annotated volume */

  MemFree(fifo);
  DestroyAdjRel(&A1);
  BMapDestroy(gray);

  for(i=0;i<vol->N;i++)
    vol->vd[i].value = 256 * L[root->u.i16[i]] + (int)(aff->u.i8[i]);

  VolumeDestroy(delta);
  VolumeDestroy(root);
  VolumeDestroy(aff);

  MemFree(P);
  MemFree(L);
}

void XAMinAffinity(XAnnVolume *vol) {
  int npoints;
  int *P,*L;
  Volume *delta, *aff, *root;

  BMap   *gray;
  int    *fifo;
  int    getq, putq;

  AdjRel *A3,*A1;

  Voxel v,w;
  short int a,b,c;
  unsigned char u;

  int i,j,k,r,s,t;
  float f;

  npoints = MapSize(vol->addseeds);
  if (!npoints) return;

  P     = (int *)   MemAlloc(sizeof(int) * npoints);
  L     = (int *)   MemAlloc(sizeof(int) * npoints);

  /* read seeds without destroying the set */
  for(i=0;i<npoints;i++) {
    MapGet(vol->addseeds, i, &j, &k);
    P[i] = j; /* voxel address */
    L[i] = k; /* label */
  }

  delta = VolumeNew(vol->W,vol->H,vol->D,vt_float_32);
  VolumeFillF(delta, 0.0);

  aff   = VolumeNew(vol->W,vol->H,vol->D,vt_integer_8);
  root  = VolumeNew(vol->W,vol->H,vol->D,vt_integer_16);

  /* initialize fifo */
  gray = BMapNew(vol->N);
  BMapFill(gray, 0);
  fifo = (int *) MemAlloc(sizeof(int) * vol->N);
  putq = getq = 0;

  /* calculate delta for the seeds */

  A3 = Spherical(3.0);
  AdjCalcDN(A3, vol->W, vol->H, vol->D);  

  for(i=0;i<npoints;i++) {
    XAVolumeVoxelOf(vol, P[i], &v);
    a = 32767;
    b = 0;
    for(j=0;j<A3->n;j++) {
      w.x = v.x + A3->dx[j];
      w.y = v.y + A3->dy[j];
      w.z = v.z + A3->dz[j];
      if (XAValidVoxel(vol, &w)) {
	c = vol->vd[ P[i]+A3->dn[j] ].value;
	if (c<a) a = c;
	if (c>b) b = c;
      }
    }
    if (b==a) ++b;
    delta->u.f32[ P[i] ] = 2.0 * ((float)(b-a)) * ((float)(b-a));
  }

  DestroyAdjRel(&A3);

  /* initialize roots and affinities, insert seeds in the fifo */
  VolumeFill(aff, 255);

  for(i=0;i<npoints;i++) {
    aff->u.i8[ P[i] ]   = 0;
    root->u.i16[ P[i] ] = i;

    fifo[putq++] = P[i];
    BMapSet(gray, P[i], 1);
  }

  A1 = Spherical(1.0);
  AdjCalcDN(A1, vol->W, vol->H, vol->D);  

  while(putq != getq) {
    s = fifo[getq];
    getq = (getq + 1) % vol->N;

    BMapSet(gray, s, 0);

    r = P [ root->u.i16[s] ];

    XAVolumeVoxelOf(vol, s, &v);
    for(i=1;i<A1->n;i++) {
      w.x = v.x + A1->dx[i];
      w.y = v.y + A1->dy[i];
      w.z = v.z + A1->dz[i];
      if (XAValidVoxel(vol, &w)) {
	t = s + A1->dn[i];
	
	f = ( vol->vd[r].value - vol->vd[t].value );
	f = 255.0 * exp( - f*f / delta->u.f32[r] );
	u = (unsigned char) f;
	if (u == 255) u = 254;

	if (u < aff->u.i8[t]) {
	  root->u.i16[t] = root->u.i16[s];
	  aff->u.i8[t]  = u;
	  if (!BMapGet(gray, t)) {
	    fifo[putq] = t;
	    putq = (putq + 1) % vol->N;
	    BMapSet(gray, t, 1);
	  }
	}

      }
    } // for i
  } // putq!=getq

  /* map result to annotated volume */

  MemFree(fifo);
  DestroyAdjRel(&A1);
  BMapDestroy(gray);

  for(i=0;i<vol->N;i++)
    vol->vd[i].value = 256 * L[root->u.i16[i]] + (int)(aff->u.i8[i]);

  VolumeDestroy(delta);
  VolumeDestroy(root);
  VolumeDestroy(aff);

  MemFree(P);
  MemFree(L);
}

void XACostThreshold(XAnnVolume *vol, int min, int max, 
		     int onvalue, int inverted)
{
  int i,N;

  N = vol->N;


  if (onvalue) {
    
    if (inverted) {
      for(i=0;i<N;i++)
	if (vol->vd[i].cost >= min && vol->vd[i].cost <= max)
	  vol->vd[i].value = 0;
    } else {
      for(i=0;i<N;i++)
	if (vol->vd[i].cost < min || vol->vd[i].cost > max)
	  vol->vd[i].value = 0;
    }

  } else {

    if (inverted) {
      for(i=0;i<N;i++)
	if (vol->vd[i].cost >= min && vol->vd[i].cost <= max)
	  vol->vd[i].orig = 0;
    } else {
      for(i=0;i<N;i++)
	if (vol->vd[i].cost < min || vol->vd[i].cost > max)
	  vol->vd[i].orig = 0;
    }

  }
}

void XAPhantomLesion(XAnnVolume *vol, int radius, int mean, 
		     int noise, int border,
		     int x,int y,int z)
{
  int i,j;
  Voxel v;
  float fa,fb,fc,fx,fy,fz,fr,fm,fn,d,r,dx,dy,dz;
  int mx,my,mz,nx,ny,nz;

  srand(time(0));

  fx = (float) x;
  fy = (float) y;
  fz = (float) z;

  fr = (float) radius;
  fm = (float) (radius - border);
  fn = (float) noise;

  mx = x - (int) (ceil(radius / vol->dx));
  my = y - (int) (ceil(radius / vol->dy));
  mz = z - (int) (ceil(radius / vol->dz));

  if (mx < 0) mx = 0;
  if (my < 0) my = 0;
  if (mz < 0) mz = 0;

  nx = x + (int) (ceil(radius / vol->dx));
  ny = y + (int) (ceil(radius / vol->dy));
  nz = z + (int) (ceil(radius / vol->dz));

  if (nx >= vol->W) nx = vol->W - 1;
  if (ny >= vol->H) ny = vol->H - 1;
  if (nz >= vol->D) nz = vol->D - 1;

  mx = mx + (vol->W * my) + (vol->W * vol->H * mz);
  nx = nx + (vol->W * ny) + (vol->W * vol->H * nz);
  if (mx < 0) mx = 0;
  if (nx >= vol->N) nx = vol->N - 1;

  for(i=mx;i<=nx;i++) {
    XAVolumeVoxelOf(vol, i, &v);
    fa = v.x;
    fb = v.y;
    fc = v.z;

    dx = (fa-fx) * vol->dx;
    dy = (fb-fy) * vol->dy;
    dz = (fc-fz) * vol->dz;
    d = sqrt( dx*dx + dy*dy + dz*dz );
    
    if (d <= fr) {

      j = mean - (fn/2.0) + (int) (fn*rand()/(RAND_MAX+1.0));
      if (j<0) j=0;
      if (j>32766) j=32766;

      if (d >= fm) {
	r = (d-fm) / (fr-fm);
	fa = (1.0-r) * ((float) j);
	fb = r * ((float) (vol->vd[i].value));
	j = (short int) (fa + fb);
      }
      vol->vd[i].value = (short int) j;
    }
  }
}

// homogenization stuff

void XAHomog(XAnnVolume *vol,int zslice) {
  int setlen, minilen;
  int i,j,k,t,u,half,W,H, WW, HH;
  i16_t *varx, *vary, *lookup;
  int *tmpa, *tmpb;
  float a,b,tmp;

  int maxamp,imax;
  i16_t smin,smax,v;

  if (!vol) return;
  W = vol->W;
  H = vol->H;
  setlen = W * H;

  WW = 1 + W/4;
  HH = 1 + H/4;
  minilen = WW * HH;

  varx = (i16_t *) MemAlloc(sizeof(i16_t) * setlen);
  if (!varx) return;
  vary = (i16_t *) MemAlloc(sizeof(i16_t) * setlen);
  if (!vary) {
    MemFree(varx);
    return;
  }
  lookup = (i16_t *) MemAlloc(sizeof(i16_t) * 32768);
  if (!lookup) {
    MemFree(vary);
    MemFree(varx);
    return;
  }
  tmpa = (int *) MemAlloc(sizeof(int) * minilen);
  tmpb = (int *) MemAlloc(sizeof(int) * minilen);
  if (!tmpa || !tmpb) {
    MemFree(lookup);
    MemFree(vary);
    MemFree(varx);
    return;
  }
   

  // pick a half slice with largest amplitude

  if (zslice < 0) {
    maxamp = 0;
    imax = 0;
    
    for(i=0;i<vol->D;i++) {
      
      smin = smax = vol->vd[i*vol->WxH].value;
      
      for(j=0;j<vol->WxH;j++) {
	v = vol->vd[j + vol->WxH * i].value;
	if (v < smin) smin = v;
	if (v > smax) smax = v;
      }
      k = (int) (smax - smin);
      if (k > maxamp) {
	maxamp = k;
	imax = i;
      }
    }

    half = imax;
  } else {
    half = zslice;
  }

  for(i=half;i>0;i--) {
    t = 0;

    // scale down both slices
    MemSet(tmpa, 0, minilen*sizeof(int));
    MemSet(tmpb, 0, minilen*sizeof(int));

    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	u = vol->tbrow[j] + k;
	tmpa[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i] + u].value;
	tmpb[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i-1] + u].value;
      }
    
    for(j=0;j<minilen;j++) {
      tmpa[j] /= 16;
      tmpb[j] /= 16;
    }

    // collect data points for correlation
    for(j=0;j<HH;j++)
      for(k=0;k<WW;k++) {
	u = j*WW + k;
	varx[t] = (i16_t) tmpa[u];
	vary[t] = (i16_t) tmpb[u];
	t++;
      }

    // find linear regression
    least_squares(vary,varx,t,&a,&b);

    //    printf("z=%d LS1 a=%.4f b=%.4f n=%d\n",i-1,a,b,t);

    // calculate lookup table
    for(j=0;j<32768;j++) {
      tmp = (((float) j) * a + b);
      if (tmp < 0.0) tmp = 0.0;
      if (tmp > 32767.0) tmp = 32767.0;
      lookup[j] = (i16_t) tmp;
    }

    // apply transform to slice i-1
    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	t = vol->tbframe[i-1] + vol->tbrow[j] + k;
	vol->vd[t].value = lookup[ vol->vd[t].value ];
      }

  }

  for(i=half;i<vol->D-1;i++) {
    t = 0;

    // scale down both slices
    MemSet(tmpa, 0, minilen*sizeof(int));
    MemSet(tmpb, 0, minilen*sizeof(int));

    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	u = vol->tbrow[j] + k;
	tmpa[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i] + u].value;
	tmpb[ (j>>2)*WW + (k>>2) ] += vol->vd[vol->tbframe[i+1] + u].value;
      }
    
    for(j=0;j<minilen;j++) {
      tmpa[j] /= 16;
      tmpb[j] /= 16;
    }

    // collect data points for correlation
    for(j=0;j<HH;j++)
      for(k=0;k<WW;k++) {
	u = j*WW + k;
	varx[t] = (i16_t) tmpa[u];
	vary[t] = (i16_t) tmpb[u];
	t++;
      }

    // find linear regression
    least_squares(vary,varx,t,&a,&b);

    //    printf("z=%d LS1 a=%.4f b=%.4f n=%d\n",i-1,a,b,t);

    // calculate lookup table
    for(j=0;j<32768;j++) {
      tmp = (((float) j) * a + b);
      if (tmp < 0.0) tmp = 0.0;
      if (tmp > 32767.0) tmp = 32767.0;
      lookup[j] = (i16_t) tmp;
    }

    // apply transform to slice i+1
    for(j=0;j<H;j++)
      for(k=0;k<W;k++) {
	t = vol->tbframe[i+1] + vol->tbrow[j] + k;
	vol->vd[t].value = lookup[ vol->vd[t].value ];
      }

  }

  MemFree(tmpa);
  MemFree(tmpb);
  MemFree(lookup);
  MemFree(varx);
  MemFree(vary);
}

void XADilate(XAnnVolume *vol, Volume *se) {
  int i,j,k,W,H,D,a,b,c,sw,sh,sd,rw,rh,rd,x,y,z;
  i16_t vmax;
  Volume *tmp;

  W = vol->W;
  H = vol->H;
  D = vol->D;

  tmp = VolumeNew(W,H,D,vt_integer_16);
  if (!tmp) return;

  sw = se->W;
  sh = se->H;
  sd = se->D;

  rw = sw/2;
  rh = sh/2;
  rd = sd/2;

  for(k=rd;k<D-rd;k++)
    for(j=rh;j<H-rw;j++)
      for(i=rw;i<W-rh;i++) {

	vmax = 0;
	
	for(c=0;c<sd;c++)
	  for(b=0;b<sh;b++)
	    for(a=0;a<sw;a++)
	      if (se->u.i8[a+se->tbrow[b]+se->tbframe[c]] != 0) {
		x = i + a - rw;
		y = j + b - rh;
		z = k + c - rd;
		vmax=MAX(vmax,vol->vd[x+vol->tbrow[y]+vol->tbframe[z]].value);
	      }

	tmp->u.i16[i+tmp->tbrow[j]+tmp->tbframe[k]] = vmax;
      }
  
  j = W*H*D;
  for(i=0;i<j;i++)
    vol->vd[i].value = tmp->u.i16[i];
  MemFree(tmp);
}

void XAErode(XAnnVolume *vol, Volume *se) {
  int i,j,k,W,H,D,a,b,c,sw,sh,sd,rw,rh,rd,x,y,z;
  i16_t vmin;
  Volume *tmp;

  W = vol->W;
  H = vol->H;
  D = vol->D;

  tmp = VolumeNew(W,H,D,vt_integer_16);
  if (!tmp) return;

  sw = se->W;
  sh = se->H;
  sd = se->D;

  rw = sw/2;
  rh = sh/2;
  rd = sd/2;

  for(k=rd;k<D-rd;k++)
    for(j=rh;j<H-rw;j++)
      for(i=rw;i<W-rh;i++) {

	vmin = 32767;
	
	for(c=0;c<sd;c++)
	  for(b=0;b<sh;b++)
	    for(a=0;a<sw;a++)
	      if (se->u.i8[a+se->tbrow[b]+se->tbframe[c]] != 0) {
		x = i + a - rw;
		y = j + b - rh;
		z = k + c - rd;
		vmin=MIN(vmin,vol->vd[x+vol->tbrow[y]+vol->tbframe[z]].value);
	      }

	tmp->u.i16[i+tmp->tbrow[j]+tmp->tbframe[k]] = vmin;
      }
  
  j = W*H*D;
  for(i=0;i<j;i++)
    vol->vd[i].value = tmp->u.i16[i];
  MemFree(tmp);
}

void XAArithmetic(XAnnVolume *vol, int op) {
  int i,N;
  int val;
  
  N = vol->N;

  switch(op) {

  case 0: // o - c
    for(i=0;i<N;i++) {
      val = ((int) (vol->vd[i].orig)) - ((int) (vol->vd[i].value));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 1: // c - o
    for(i=0;i<N;i++) {
      val = ((int) (vol->vd[i].value)) - ((int) (vol->vd[i].orig));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 2: // o + c
    for(i=0;i<N;i++) {
      val = ((int) (vol->vd[i].orig)) + ((int) (vol->vd[i].value));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 3: // c / o
    for(i=0;i<N;i++) {
      val = (1+(int) (vol->vd[i].value)) / (1+(int) (vol->vd[i].orig));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 4: // o / c
    for(i=0;i<N;i++) {
      val = (1+(int) (vol->vd[i].orig)) / (1+(int) (vol->vd[i].value));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 5: // c * o
    for(i=0;i<N;i++) {
      val = ((int) (vol->vd[i].value)) * ((int) (vol->vd[i].orig));
      if (val < 0) val = 0; if (val>32767) val=32767;
      vol->vd[i].value = (i16_t) val;
    }
    break;

  case 6: // c / o
    for(i=0;i<N;i++)
      vol->vd[i].orig = vol->vd[i].value;
    break;
  }

}

