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

#include <stdlib.h>
#include <math.h>
#include "ip_poly.h"
#include "ip_optlib.h"
#include "ip_mem.h"

Polynomial * PolyNew(int maxdeg) {
  Polynomial *p;
  
  p = (Polynomial *) MemAlloc(sizeof(Polynomial));
  if (!p) return 0;
  p->coef = (float *) MemAlloc((maxdeg+1)*sizeof(float));
  if (!p->coef) return 0;

  p->maxdegree = maxdeg;
  p->degree    = 0;
  FloatFill(p->coef, 0.0, maxdeg+1);

  return p;
}

void PolyDestroy(Polynomial *p) {
  MemFree(p->coef);
  MemFree(p);
}

void PolyZero(Polynomial *p) {
  FloatFill(p->coef, 0.0, p->maxdegree+1);
}

void PolyCopy(Polynomial *dest, Polynomial *src) {
  int i;
  PolyZero(dest);
  for(i=0;i<=src->maxdegree && i<=dest->maxdegree;i++)
    dest->coef[i] = src->coef[i];
}

void PolyMultiplyV(Polynomial *dest, float v) {
  int i;
  for(i=0;i<=dest->degree;i++)
    dest->coef[i] *= v;
}

void PolyMultiplyP(Polynomial *dest, Polynomial *src) {
  int i,j,outsz;
  float *out;

  outsz = dest->degree + src->degree;
  out = (float *)MemAlloc( (outsz+1) * sizeof(float));  
  FloatFill(out, 0.0, outsz+1);

  for(i=0;i<=src->degree;i++)
    for(j=0;j<=dest->degree;j++)
      out[i+j] += dest->coef[j] * src->coef[i];

  for(i=0;i<=outsz && i<=dest->maxdegree;i++) {
    dest->coef[i] = out[i];
    dest->degree = i;
  }
  MemFree(out);
}

void PolySumP(Polynomial *dest, Polynomial *src) {
  int i;
  for(i=0;i<=src->degree;i++) {
    if (i>dest->maxdegree) return;
    dest->coef[i] += src->coef[i];
    if (i > dest->degree) dest->degree = i;
  }
}

void PolyDerivative(Polynomial *p) {
  int i;

  for(i=0;i<p->degree;i++)
    p->coef[i] = (i+1.0) * p->coef[i+1];
  
  p->coef[p->degree] = 0.0;
  p->degree--;
}

void         PolySet(Polynomial *dest, int deg, float coef) {
  if (deg <= dest->maxdegree) {
    dest->coef[deg] = coef;
    if (dest->degree < deg) dest->degree = deg;
  }
}

float        PolyGet(Polynomial *src, int deg) {
  return(src->coef[deg]);
}

float        PolyValue(Polynomial *src, float x) {
  float v;
  int i;
  v = src->coef[0];
  
  for(i=1;i<=src->degree;i++)
    v += src->coef[i] * ((float)pow(x, i));

  return v;
}

void LagrangeInterpolation(Polynomial *dest, 
			   float *x, float *y, int np)
{
  int i,j;
  Polynomial T, P;
  float Tc[2], Pc[32];
  float div;

  PolyZero(dest);

  T.maxdegree = 1;
  T.degree = 1;
  T.coef = Tc;

  P.maxdegree = (dest->maxdegree > 31) ? 31 : dest->maxdegree;
  P.degree = 0;
  P.coef = Pc;

  for(i=0;i<np;i++) {
    
    FloatFill(Pc, 0.0, P.maxdegree+1);
    P.degree = 1;
    P.coef[0] = y[i];
    div = 1.0;

    for(j=0;j<np;j++)
      if (j!=i) {
	T.coef[0] = -x[j];
	T.coef[1] = 1.0;

	PolyMultiplyP(&P,&T);
	div *= x[i] - x[j];
      }

    if (div == 0.0) div = 1.0; else div = 1.0 / div;
    PolyMultiplyV(&P, div);
    PolySumP(dest, &P);
  }
}
