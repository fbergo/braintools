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

/************************************************
 $Id: adjacency.c,v 1.5 2013/11/11 22:34:12 bergo Exp $

 adjacency.c - adjacency relations

************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ip_common.h"  
#include "ip_adjacency.h"

AdjRel *CreateAdjRel(int n)
{
  AdjRel *A=NULL;

  A = (AdjRel *) calloc(1,sizeof(AdjRel));
  if (A != NULL){
    A->dx = AllocIntArray(n);
    A->dy = AllocIntArray(n);
    A->dz = AllocIntArray(n);
    A->dn = AllocIntArray(n);
    A->n  = n;
  } else {
    Error(MSG1,"CreateAdjRel");
  }

  return(A);
}

void DestroyAdjRel(AdjRel **A)
{
  AdjRel *aux;

  aux = *A;
  if (aux != NULL){
    if (aux->dx != NULL) free(aux->dx);
    if (aux->dy != NULL) free(aux->dy);
    if (aux->dz != NULL) free(aux->dz);
    if (aux->dn != NULL) free(aux->dn);
    free(aux);
    *A = NULL;
  }   
}

AdjRel *Circular(float r)
{
  AdjRel *A=NULL;
  int i,j,k,n,dx,dy,r0,r2,d,i0=0;
  float *dr,aux;

  n=0;

  r0 = (int)r;
  r2  = (int)(r*r);
  for(dy=-r0;dy<=r0;dy++)
    for(dx=-r0;dx<=r0;dx++)
      if ( ((dx*dx)+(dy*dy)) <= r2 )
	n++;

  A = CreateAdjRel(n);
  i=0;
  for(dy=-r0;dy<=r0;dy++)
    for(dx=-r0;dx<=r0;dx++)
      if(((dx*dx)+(dy*dy)) <= r2){
	A->dx[i]=dx;
	A->dy[i]=dy;
	A->dz[i]=0;
	A->dn[i]=0;
	if ((dx==0)&&(dy==0))
	  i0 = i;
	i++;
      }

  /* Set clockwise */
  
  dr = AllocFloatArray(A->n);
  for (i=0; i < A->n; i++) {
    dx = A->dx[i];
    dy = A->dy[i];
    dr[i] = (float)sqrt((dx*dx) + (dy*dy));
  }
  dr[i0] = 0.0;

  /* place central pixel at first */
  
  aux    = dr[i0];
  dr[i0] = dr[0];
  dr[0]  = aux;
  d         = A->dx[i0];
  A->dx[i0] = A->dx[0];
  A->dx[0]  = d;
  d         = A->dy[i0];
  A->dy[i0] = A->dy[0];
  A->dy[0]  = d;

  /* sort by radius */
  
  for (i=1; i < A->n-1; i++){
    k = i;
    for (j=i+1; j < A->n; j++)
      if (dr[j] < dr[k]){
	k = j;
      }
    aux   = dr[i];
    dr[i] = dr[k];
    dr[k] = aux;
    d        = A->dx[i];
    A->dx[i] = A->dx[k];
    A->dx[k] = d;
    d        = A->dy[i];
    A->dy[i] = A->dy[k];
    A->dy[k] = d;
  }

  free(dr);
  return(A);
}

AdjRel *Spherical(float r)
{
  AdjRel *A=NULL;
  int i,j,k,n,dx,dy,dz,r0,r2,d,i0=0;
  float *dr,aux;

  n=0;

  r0 = (int)r;
  r2  = (int)(r*r);
  for(dz=-r0;dz<=r0;dz++)
    for(dy=-r0;dy<=r0;dy++)
      for(dx=-r0;dx<=r0;dx++)
	if(((dx*dx)+(dy*dy)+(dz*dz)) <= r2)
	  n++;

  A = CreateAdjRel(n);
  i=0;
  for(dz=-r0;dz<=r0;dz++)
    for(dy=-r0;dy<=r0;dy++)
      for(dx=-r0;dx<=r0;dx++)
	if(((dx*dx)+(dy*dy)+(dz*dz)) <= r2){
	  A->dx[i]=dx;
	  A->dy[i]=dy;
	  A->dz[i]=dz;
	  A->dn[i]=0;
	  if ((dx==0)&&(dy==0)&&(dz==0))
	    i0 = i;
	  i++;
	}

  /* Set clockwise */
  
  dr = AllocFloatArray(A->n);
  for (i=0; i < A->n; i++) {
    dx = A->dx[i];
    dy = A->dy[i];
    dz = A->dz[i];
    dr[i] = (float)sqrt((dx*dx) + (dy*dy) + (dz*dz));
  }
  dr[i0] = 0.0;

  /* place central pixel at first */
  
  aux    = dr[i0];
  dr[i0] = dr[0];
  dr[0]  = aux;
  d         = A->dx[i0];
  A->dx[i0] = A->dx[0];
  A->dx[0]  = d;
  d         = A->dy[i0];
  A->dy[i0] = A->dy[0];
  A->dy[0]  = d;
  d         = A->dz[i0];
  A->dz[i0] = A->dz[0];
  A->dz[0]  = d;

  /* sort by radius */
  
  for (i=1; i < A->n-1; i++){
    k = i;
    for (j=i+1; j < A->n; j++)
      if (dr[j] < dr[k]){
	k = j;
      }
    aux   = dr[i];
    dr[i] = dr[k];
    dr[k] = aux;
    d        = A->dx[i];
    A->dx[i] = A->dx[k];
    A->dx[k] = d;
    d        = A->dy[i];
    A->dy[i] = A->dy[k];
    A->dy[k] = d;
    d        = A->dz[i];
    A->dz[i] = A->dz[k];
    A->dz[k] = d;
  }

  free(dr);
  return(A);
}

void    AdjCalcDN(AdjRel *A, int W,int H, int D) 
{
  int i;
  int fx, fy, fz;

  /* this avoids bugs (bad traversals) in 2D images */
  fx = 1;
  fy = W;
  fz = W*H;
  if (W == 1) fx = W*H*D;
  if (H == 1) fy = W*H*D;
  if (D == 1) fz = W*H*D;

  for(i=0;i<A->n;i++)
    A->dn[i] = (fx * A->dx[i]) + (fy * A->dy[i]) + (fz * A->dz[i]);
}

unsigned char AdjEncodeDN(AdjRel *A, int Nsrc, int Ndest) 
{
  unsigned int i;
  int d = Ndest - Nsrc;

  for(i=0;i<A->n;i++)
    if (A->dn[i]==d) return ((unsigned char)i);
  
  return 0;
}

/* doesn't check array bounds */
int AdjDecodeDN(AdjRel *A, int Nsrc, int edge) {
  return(Nsrc + A->dn[edge]);
}
