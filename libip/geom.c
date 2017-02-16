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

/****************************************************
 $Id: geom.c,v 1.13 2013/11/11 22:34:12 bergo Exp $

 geom.c - vectors, points, transforms, vector math

*****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ip_geom.h"
#include "ip_linsolve.h"
#include "ip_mem.h"
#include "ip_optlib.h"

Point * PointNew() {
  Point *p;
  p=(Point *)MemAlloc(sizeof(Point));
  if (!p) return 0;
  p->x = p->y = p->z = 0.0;
  return p;
}

void    PointCopy(Point *dest,Point *src) {
  dest->x = src->x;
  dest->y = src->y;
  dest->z = src->z;
}

void    PointTranslate(Point *p, float dx,float dy,float dz) {
  p->x += dx;
  p->y += dy;
  p->z += dz;
}

void    PointTransform(Point *dest,Point *src, Transform *t) {
  dest->x = src->x * t->e[0][0] + src->y * t->e[0][1] + src->z * t->e[0][2];
  dest->y = src->x * t->e[1][0] + src->y * t->e[1][1] + src->z * t->e[1][2];
  dest->z = src->x * t->e[2][0] + src->y * t->e[2][1] + src->z * t->e[2][2];
}

void PointHTrans(Point *dest,Point *src, HTrans *t) {
  dest->x = src->x * t->e[0][0] + src->y * t->e[0][1] + src->z * t->e[0][2] + t->e[0][3];
  dest->y = src->x * t->e[1][0] + src->y * t->e[1][1] + src->z * t->e[1][2] + t->e[1][3];
  dest->z = src->x * t->e[2][0] + src->y * t->e[2][1] + src->z * t->e[2][2] + t->e[2][3];
}

void    PointDestroy(Point *p) {
  if (p) MemFree(p);
}

void    PointAssign(Point *p, float x, float y, float z) {
  p->x = x;
  p->y = y;
  p->z = z;
}

HTrans *HTransNew() {
  HTrans *t;
  t=(HTrans *)MemAlloc(sizeof(HTrans));
  if (!t) return 0;
  HTransZero(t);
  return t;
}

void HTransCopy(HTrans *dest, HTrans *src) {
  MemCpy(dest,src,sizeof(HTrans));
}

void HTransDestroy(HTrans *t) {
  if (t) MemFree(t);
}

void HTransIdentity(HTrans *t) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<4;j++)
      if (i==j)
        t->e[i][j] = 1.0;
      else
        t->e[i][j] = 0.0;
}

void HTransZero(HTrans *t) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<4;j++)
      t->e[i][j] = 0.0;
}

void HTransXRot(HTrans *t, float angle) {
  HTransIdentity(t);
  angle *= M_PI / 180.0;
  t->e[1][1] = cos(angle);
  t->e[2][1] = sin(angle);
  t->e[1][2] = -sin(angle);
  t->e[2][2] = cos(angle);
}

void HTransYRot(HTrans *t, float angle) {
  HTransIdentity(t);
  angle *= M_PI / 180.0;
  t->e[0][0] = cos(angle);
  t->e[2][0] = sin(angle);
  t->e[0][2] = -sin(angle);
  t->e[2][2] = cos(angle);
}

void HTransZRot(HTrans *t, float angle) {
  HTransIdentity(t);
  angle *= M_PI / 180.0;
  t->e[0][0] = cos(angle);
  t->e[1][0] = sin(angle);
  t->e[0][1] = -sin(angle);
  t->e[1][1] = cos(angle);
}

void HTransIsoScale(HTrans *t, float factor) {
  HTransZero(t);
  t->e[0][0] = factor;
  t->e[1][1] = factor;
  t->e[2][2] = factor;
}

void HTransScale(HTrans *t, float sx,float sy,float sz) {
  HTransZero(t);
  t->e[0][0] = sx;
  t->e[1][1] = sy;
  t->e[2][2] = sz;
}

void HTransTrans(HTrans *t, float dx,float dy,float dz) {
  HTransIdentity(t);
  t->e[0][3] = dx;
  t->e[1][3] = dy;
  t->e[2][3] = dz;
}

void HTransCompose(HTrans *dest, HTrans *src) {
  HTrans tmp;
  int i,j,k;

  HTransZero(&tmp);

  for(i=0;i<3;i++)
    for(j=0;j<4;j++)
      for(k=0;k<3;k++)
        tmp.e[i][j] += src->e[i][k] * dest->e[k][j];

  HTransCopy(dest, &tmp);
}

int HTransInvert(HTrans *dest, HTrans *src) {
  int i;
  double M[16];

  for(i=0;i<12;i++)
    M[i] = src->e[i/4][i%4];
  M[12] = M[13] = M[14] = 0.0;
  M[15] = 1.0;
  if (minv(M,4)!=0) return -1;
  for(i=0;i<12;i++)
    dest->e[i/4][i%4] = M[i];
  return 0;
}

Transform * TransformNew() { 
  Transform *t;
  int i,j;
  t=(Transform *)MemAlloc(sizeof(Transform));
  if (!t) return 0;

  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      t->e[i][j] = 0.0;
  return t;
}

void        TransformCopy(Transform *dest,Transform *src) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      dest->e[i][j] = src->e[i][j];
}

void        TransformDestroy(Transform *t) {
  if (t) MemFree(t);
}

void        TransformIdentity(Transform *t) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      if (i==j)
	t->e[i][j] = 1.0;
      else
	t->e[i][j] = 0.0;
}

void        TransformZero(Transform *t) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      t->e[i][j]=0.0;
}

void        TransformXRot(Transform *t, float angle) {
  TransformIdentity(t);
  angle *= M_PI / 180.0;
  t->e[1][1] = cos(angle);
  t->e[2][1] = sin(angle);
  t->e[1][2] = -sin(angle);
  t->e[2][2] = cos(angle);
}

void        TransformYRot(Transform *t, float angle) {
  TransformIdentity(t);
  angle *= M_PI / 180.0;
  t->e[0][0] = cos(angle);
  t->e[2][0] = sin(angle);
  t->e[0][2] = -sin(angle);
  t->e[2][2] = cos(angle);
}

void        TransformZRot(Transform *t, float angle) {
  TransformIdentity(t);
  angle *= M_PI / 180.0;
  t->e[0][0] = cos(angle);
  t->e[1][0] = sin(angle);
  t->e[0][1] = -sin(angle);
  t->e[1][1] = cos(angle);  
}

void        TransformIsoScale(Transform *t, float factor) {
  TransformZero(t);
  t->e[0][0] = factor;
  t->e[1][1] = factor;
  t->e[2][2] = factor;
}

void  TransformScale(Transform *t, float sx, float sy, float sz) {
  TransformZero(t);
  t->e[0][0] = sx;
  t->e[1][1] = sy;
  t->e[2][2] = sz;
}

void        TransformCompose(Transform *dest, Transform *src) {
  Transform tmp;
  int i,j,k;

  TransformZero(&tmp);

  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      for(k=0;k<3;k++)
	tmp.e[i][j] += src->e[i][k] * dest->e[k][j];

  TransformCopy(dest, &tmp);
}

int TransformInvert(Transform *dest, Transform *src) {
  int i;
  double M[9];

  for(i=0;i<9;i++)
    M[i] = src->e[i/3][i%3];
  if (minv(M,3)!=0) return -1;
  for(i=0;i<9;i++)
    dest->e[i/3][i%3] = M[i];
  return 0;
}

float      sgn(float v) {
  if (v==0.0) return 0.0;
  if (v<0.0) return -1.0; else return 1.0;
}

int        isgn(int v) {
  if (v==0) return 0;
  if (v<0) return -1; else return 1;
}

float      det3(float a,float b,float c,float d,float e,float f,
		 float g,float h,float i) {
  return(a*e*i + b*f*g + c*d*h - c*e*g - a*f*h - b*d*i);
}

int        det3i(int a,int b,int c,int d,int e,int f,int g,int h,int i) 
{
  return(a*e*i + b*f*g + c*d*h - c*e*g - a*f*h - b*d*i);
}

int         FacePointTest(Face *f, Point *p) {
  float s=0.0,a;
  int i;

  for(i=0;i<4;i++) {
    a=sgn( det3 ( 1.0, f->p[i]->x,       f->p[i]->y,
		  1.0, f->p[(i+1)%4]->x, f->p[(i+1)%4]->y,
		  1.0, p->x,             p->y) );
    if (s!=0.0 && a!=0.0 && a!=s) return 0;
    if (a!=0.0) s=a;
  }
  return 1;

}

void FaceAssign(Face *f, Point *p0, Point *p1, Point *p2, Point *p3) {
  f->p[0] = p0;
  f->p[1] = p1;
  f->p[2] = p2;
  f->p[3] = p3;
}

float      FaceMaxZ(Face *f) {
  int i;
  float mz = -50000.0;
  for (i=0;i<4;i++)
    if (f->p[i]->z > mz)
      mz=f->p[i]->z;
  return mz;
}

// vectorial math

float VectorLength(Vector *v) {
  return(sqrt(v->x * v->x + v->y * v->y + v->z * v->z));
}

void VectorAdd(Vector *dest,Vector *src) {
  dest->x += src->x;
  dest->y += src->y;
  dest->z += src->z;
}

void VectorCrossProduct(Vector *dest,Vector *a, Vector *b) {
  dest->x = a->y * b->z  -  a->z * b->y;
  dest->y = a->z * b->x  -  a->x * b->z;
  dest->z = a->x * b->y  -  a->y * b->x;
}

float VectorInnerProduct(Vector *a, Vector *b) {
  float i;
  i=a->x * b->x  +  a->y * b->y  +  a->z * b->z;
  return i;
}

void VectorScalarProduct(Vector *dest, float factor) {
  dest->x *= factor;
  dest->y *= factor;
  dest->z *= factor;
}

void VectorNormalize(Vector *a) {
  float l;
  l=VectorLength(a);
  if (l != 0.0)
    VectorScalarProduct(a,1/l);
}

int VectorEqual(Vector *a,Vector *b) {
  return(a->x == b->x && a->y == b->y && a->z == b->z);
}

int inbox(int x,int y,int x0,int y0,int w,int h) {
  return( (x>=x0)&&(y>=y0)&&(x<x0+w)&&(y<y0+h) );
}

int box_intersection(int x1,int y1,int w1,int h1,
		     int x2,int y2,int w2,int h2)
{
  w1 += x1 - 1;
  w2 += x2 - 1;
  h1 += y1 - 1;
  h2 += y2 - 1;
  if (! ((x1 >= x2 && x1 <= w2) || (w1 >= x2 && w1 <= w2)) )
    return 0;
  if (! ((y1 >= y2 && y1 <= h2) || (h1 >= y2 && h1 <= h2)) )
    return 0;
  return 1;
}

/* cuboid */

Cuboid     *CuboidNew() {
  Cuboid *c;
  int i;

  c=(Cuboid *)MemAlloc(sizeof(Cuboid));
  for(i=0;i<8;i++)
    c->p[i].x = c->p[i].y = c->p[i].z = 0.0;

  c->f[0].p[0] = & (c->p[0]);
  c->f[0].p[1] = & (c->p[1]);
  c->f[0].p[2] = & (c->p[2]);
  c->f[0].p[3] = & (c->p[3]);

  c->f[1].p[0] = & (c->p[4]);
  c->f[1].p[1] = & (c->p[5]);
  c->f[1].p[2] = & (c->p[6]);
  c->f[1].p[3] = & (c->p[7]);

  c->f[2].p[0] = & (c->p[0]);
  c->f[2].p[1] = & (c->p[1]);
  c->f[2].p[2] = & (c->p[4]);
  c->f[2].p[3] = & (c->p[5]);

  c->f[3].p[0] = & (c->p[2]);
  c->f[3].p[1] = & (c->p[3]);
  c->f[3].p[2] = & (c->p[6]);
  c->f[3].p[3] = & (c->p[7]);

  for(i=0;i<6;i++)
    c->hidden[i] = 0;

  return c;
}

void        CuboidDestroy(Cuboid *c) {
  if (c) MemFree(c);
}

void        CuboidOrthogonalInit(Cuboid *c,
				 float x0,float x1,
				 float y0,float y1,
				 float z0,float z1) {
  /*
                    _
      4 --- 5       /| (Z)
     /     /|      /
    0 --- 1 |     *--> (X)
    | 7   | 6     |
    |     |/     \|/
    3 --- 2      (Y)

  */
  int i;

  
  PointAssign(&(c->p[0]), x0, y0, z0);
  PointAssign(&(c->p[1]), x1, y0, z0);
  PointAssign(&(c->p[2]), x1, y1, z0);
  PointAssign(&(c->p[3]), x0, y1, z0);
  PointAssign(&(c->p[4]), x0, y0, z1);
  PointAssign(&(c->p[5]), x1, y0, z1);
  PointAssign(&(c->p[6]), x1, y1, z1);
  PointAssign(&(c->p[7]), x0, y1, z1);

  FaceAssign(&(c->f[0]), &(c->p[0]), &(c->p[1]), &(c->p[2]), &(c->p[3]));
  FaceAssign(&(c->f[1]), &(c->p[4]), &(c->p[5]), &(c->p[6]), &(c->p[7]));
  FaceAssign(&(c->f[2]), &(c->p[0]), &(c->p[1]), &(c->p[5]), &(c->p[4]));
  FaceAssign(&(c->f[3]), &(c->p[3]), &(c->p[2]), &(c->p[6]), &(c->p[7]));
  FaceAssign(&(c->f[4]), &(c->p[1]), &(c->p[2]), &(c->p[6]), &(c->p[5]));
  FaceAssign(&(c->f[5]), &(c->p[0]), &(c->p[3]), &(c->p[7]), &(c->p[4]));

  for(i=0;i<6;i++)
    c->hidden[i] = 0;
}

void CuboidTransform(Cuboid *c, Transform *t) {
  Point tmp;
  int i;

  for(i=0;i<8;i++) {
    PointTransform(&tmp, &(c->p[i]), t);
    PointCopy(&(c->p[i]), &tmp);    
  }

  for(i=0;i<6;i++)
    c->hidden[i] = 0;
}

void CuboidTranslate(Cuboid *c, float dx, float dy, float dz) {
  int i;
  for(i=0;i<8;i++)
    PointTranslate(&(c->p[i]), dx, dy, dz);
}

void CuboidCopy(Cuboid *dest, Cuboid *src) {
  int i,j,k;

  for(i=0;i<8;i++)
    PointCopy(&(dest->p[i]), &(src->p[i]));

  for(i=0;i<6;i++)
    dest->hidden[i] = src->hidden[i];

  for(i=0;i<6;i++)
    for(j=0;j<4;j++)
      for(k=0;k<8;k++)
	if (src->f[i].p[j] == &(src->p[k])) {
	  dest->f[i].p[j] = &(dest->p[k]);
	  break;
	}

}

void CuboidRemoveHiddenFaces(Cuboid *c) {
  float maxz = -10000.0;
  float fmaxz[6] = { [0 ... 5] = -10000.0 };
  int i,j,k;
  int pv[8] = { [0 ... 7] = 1 };

  for(i=0;i<6;i++)
    c->hidden[i] = 0;

  for(i=0;i<8;i++)
    if (c->p[i].z > maxz)
      maxz=c->p[i].z;

  for(i=0;i<6;i++)
    for(j=0;j<4;j++)
      if (c->f[i].p[j]->z > fmaxz[i])
	fmaxz[i]=c->f[i].p[j]->z;
  
  for(i=0;i<8;i++)
    for(j=0;j<6;j++)
      if (fmaxz[j] < c->p[i].z)
	if ( FacePointTest(&(c->f[j]), &(c->p[i])) ) {
	  pv[i]=0;
	  break;
	}

  for(i=0;i<8;i++)
    if (!pv[i])
      for(j=0;j<6;j++)
	for(k=0;k<4;k++)
	  if (c->f[j].p[k] == &(c->p[i])) {
	    c->hidden[j] = 1;
	    break;
	  }
}

/*

 -110                40 60       130
  +------------------------------+  -90
  | -90                      110 |
  |  +----------------+  +----+  |  -70
  |  |                |  |    |  |
  |  |                |  |    |  |
  |  |                |  |    |  |
  |  |                |  |    |  |
  |  |                |  |    |  |
  |  |                |  |    |  |
  |  +----------------+  +----+  |  70
  |                              |
  +------------------------------+  90

  BIG BOX : 0
  LEFT    : 1
  RIGHT   : 2

  behind: 3,4,5

*/

WireModel *Abacus() {
  WireModel *wm;
  int i,j,offset,x,y;
  float a,b;

  int donutx[] = {0,1,2,3,5,0,2,3,4,5,0,1,2,3,5,0,2,3,4,5,8,9,7,9,7,9,7,9};
  int donuty[] = {0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,0,0,1,1,2,2,3,3};

  wm=(WireModel *)MemAlloc(sizeof(WireModel));

  wm->nfaces = 12 + 6 + 4*7 + 4*7 + 28*16*2;
  wm->faces = (Wire *) MemAlloc(sizeof(Wire) * wm->nfaces);

  offset =  12 + 6 + 4*7 + 4*7;

  for(i=0;i<wm->nfaces;i++) {
    wm->faces[i].npoints = 4;
    wm->faces[i].fill = 1;
  }
  
  PointAssign(&(wm->faces[0].p[0]), -110, -90, -15);
  PointAssign(&(wm->faces[0].p[1]),  130, -90, -15);
  PointAssign(&(wm->faces[0].p[2]),  130,  90, -15);
  PointAssign(&(wm->faces[0].p[3]), -110,  90, -15);

  PointAssign(&(wm->faces[1].p[0]), -90, -70, -15);
  PointAssign(&(wm->faces[1].p[1]),  40, -70, -15);
  PointAssign(&(wm->faces[1].p[2]),  40,  70, -15);
  PointAssign(&(wm->faces[1].p[3]), -90,  70, -15);

  PointAssign(&(wm->faces[2].p[0]),  60, -70, -15);
  PointAssign(&(wm->faces[2].p[1]),  110, -70, -15);
  PointAssign(&(wm->faces[2].p[2]),  110,  70, -15);
  PointAssign(&(wm->faces[2].p[3]),  60,  70, -15);

  PointAssign(&(wm->faces[3].p[0]), -110, -90, 15);
  PointAssign(&(wm->faces[3].p[1]),  130, -90, 15);
  PointAssign(&(wm->faces[3].p[2]),  130,  90, 15);
  PointAssign(&(wm->faces[3].p[3]), -110,  90, 15);

  PointAssign(&(wm->faces[4].p[0]), -90, -70, 15);
  PointAssign(&(wm->faces[4].p[1]),  40, -70, 15);
  PointAssign(&(wm->faces[4].p[2]),  40,  70, 15);
  PointAssign(&(wm->faces[4].p[3]), -90,  70, 15);

  PointAssign(&(wm->faces[5].p[0]),  60, -70, 15);
  PointAssign(&(wm->faces[5].p[1]),  110, -70, 15);
  PointAssign(&(wm->faces[5].p[2]),  110,  70, 15);
  PointAssign(&(wm->faces[5].p[3]),  60,  70, 15);

  PointAssign(&(wm->faces[6].p[0]),  -90, -70, -15);
  PointAssign(&(wm->faces[6].p[1]),   40, -70, -15);
  PointAssign(&(wm->faces[6].p[2]),   40, -70, 15);
  PointAssign(&(wm->faces[6].p[3]),  -90, -70, 15);

  PointAssign(&(wm->faces[7].p[0]),   60, -70, -15);
  PointAssign(&(wm->faces[7].p[1]),  110, -70, -15);
  PointAssign(&(wm->faces[7].p[2]),  110, -70, 15);
  PointAssign(&(wm->faces[7].p[3]),   60, -70, 15);

  PointAssign(&(wm->faces[8].p[0]),  -90,  70, -15);
  PointAssign(&(wm->faces[8].p[1]),   40,  70, -15);
  PointAssign(&(wm->faces[8].p[2]),   40,  70, 15);
  PointAssign(&(wm->faces[8].p[3]),  -90,  70, 15);

  PointAssign(&(wm->faces[9].p[0]),   60,  70, -15);
  PointAssign(&(wm->faces[9].p[1]),  110,  70, -15);
  PointAssign(&(wm->faces[9].p[2]),  110,  70, 15);
  PointAssign(&(wm->faces[9].p[3]),   60,  70, 15);

  PointAssign(&(wm->faces[10].p[0]), -110, -90, -15);
  PointAssign(&(wm->faces[10].p[1]),  130, -90, -15);
  PointAssign(&(wm->faces[10].p[2]),  130, -90,  15);
  PointAssign(&(wm->faces[10].p[3]), -110, -90,  15);

  PointAssign(&(wm->faces[11].p[0]), -110,  90, -15);
  PointAssign(&(wm->faces[11].p[1]),  130,  90, -15);
  PointAssign(&(wm->faces[11].p[2]),  130,  90,  15);
  PointAssign(&(wm->faces[11].p[3]), -110,  90,  15);

  PointAssign(&(wm->faces[12].p[0]),  -90,  -70, -15);
  PointAssign(&(wm->faces[12].p[1]),  -90,  70,  -15);
  PointAssign(&(wm->faces[12].p[2]),  -90,  70,  15);
  PointAssign(&(wm->faces[12].p[3]),  -90,  -70,  15);

  PointAssign(&(wm->faces[13].p[0]),  40,  -70, -15);
  PointAssign(&(wm->faces[13].p[1]),  40,  70,  -15);
  PointAssign(&(wm->faces[13].p[2]),  40,  70,  15);
  PointAssign(&(wm->faces[13].p[3]),  40,  -70,  15);

  PointAssign(&(wm->faces[14].p[0]),  60,  -70, -15);
  PointAssign(&(wm->faces[14].p[1]),  60,  70,  -15);
  PointAssign(&(wm->faces[14].p[2]),  60,  70,  15);
  PointAssign(&(wm->faces[14].p[3]),  60,  -70,  15);

  PointAssign(&(wm->faces[15].p[0]),  110,  -70, -15);
  PointAssign(&(wm->faces[15].p[1]),  110,  70,  -15);
  PointAssign(&(wm->faces[15].p[2]),  110,  70,  15);
  PointAssign(&(wm->faces[15].p[3]),  110,  -70,  15);

  PointAssign(&(wm->faces[16].p[0]),  -110,  -90, -15);
  PointAssign(&(wm->faces[16].p[1]),  -110,  90,  -15);
  PointAssign(&(wm->faces[16].p[2]),  -110,  90,  15);
  PointAssign(&(wm->faces[16].p[3]),  -110,  -90,  15);

  PointAssign(&(wm->faces[17].p[0]),   130,  -90, -15);
  PointAssign(&(wm->faces[17].p[1]),   130,  90,  -15);
  PointAssign(&(wm->faces[17].p[2]),   130,  90,  15);
  PointAssign(&(wm->faces[17].p[3]),   130,  -90,  15);

  for(j=0;j<4;j++) {
    for(i=0;i<5;i++) {
      wm->faces[18+7*j+i].npoints = 2;
      wm->faces[18+4*7+7*j+i].npoints = 2;
      a= (-70+17+j*35) + 1.5 * cos(70.0 * i * M_PI/180.0);
      b= 0.0 + 1.5 * sin(60.0 * i * M_PI/180.0);
      PointAssign(&(wm->faces[18+i+7*j].p[0]), -90,  a, b);
      PointAssign(&(wm->faces[18+i+7*j].p[1]), 40,  a, b);

      PointAssign(&(wm->faces[18+4*7+i+7*j].p[0]), 60,  a, b);
      PointAssign(&(wm->faces[18+4*7+i+7*j].p[1]), 110,  a, b);
    }
    wm->faces[18+7*j+5].npoints = 5;
    wm->faces[18+7*j+6].npoints = 5;

    wm->faces[18+4*7+7*j+5].npoints = 5;
    wm->faces[18+4*7+7*j+6].npoints = 5;

    for(i=0;i<5;i++) {
      a= (-70+17+j*35) + 1.5 * cos(70.0 * i * M_PI/180.0);
      b= 0.0 + 1.5 * sin(60.0 * i * M_PI/180.0);
      PointAssign(&(wm->faces[18+7*j+5].p[i]), -90,  a, b);
      PointAssign(&(wm->faces[18+7*j+6].p[i]), 40,  a, b);

      PointAssign(&(wm->faces[18+4*7+7*j+5].p[i]), 60,  a, b);
      PointAssign(&(wm->faces[18+4*7+7*j+6].p[i]), 110,  a, b);
    }
  }

  for(j=0;j<28;j++) {
    for(i=0;i<16;i++) {
      wm->faces[offset+j*16+2*i].npoints = 3;
      wm->faces[offset+j*16+2*i+1].npoints = 3;

      if (donutx[j]>6)
	x=-90+15+7*20+5+16*(donutx[j]-7);
      else
	x=-90+15+20*donutx[j];
	
      y=-70+17+35*donuty[j];

      PointAssign(&(wm->faces[offset+j*16+2*i].p[0]),x-5,y,0);
      PointAssign(&(wm->faces[offset+j*16+2*i+1].p[0]),x+5,y,0);

      a=15.0*cos(i*45.0*M_PI/180.0);
      b=15.0*sin(i*45.0*M_PI/180.0);
      PointAssign(&(wm->faces[offset+j*16+2*i].p[1]),x,y+a,b);
      PointAssign(&(wm->faces[offset+j*16+2*i+1].p[1]),x,y+a,b);
      
      a=15.0*cos((i+1)*45.0*M_PI/180.0);
      b=15.0*sin((i+1)*45.0*M_PI/180.0);
      PointAssign(&(wm->faces[offset+j*16+2*i].p[2]),x,y+a,b);
      PointAssign(&(wm->faces[offset+j*16+2*i+1].p[2]),x,y+a,b);
    }
  }

  return wm;
}


void TriangleTransform(Triangle *dest, Triangle *src, Transform *T)
{
  PointTransform(&(dest->p[0]),&(src->p[0]),T);
  PointTransform(&(dest->p[1]),&(src->p[1]),T);
  PointTransform(&(dest->p[2]),&(src->p[2]),T);
}

void TriangleTranslate(Triangle *src, float dx, float dy, float dz)
{
  PointTranslate(&(src->p[0]),dx,dy,dz);
  PointTranslate(&(src->p[1]),dx,dy,dz);
  PointTranslate(&(src->p[2]),dx,dy,dz);
}

int   IntNormalize(int value,int omin,int omax,int nmin,int nmax)
{
  float tmp;
  int   i;
  if ( (omax - omin) == 0) return 0;
  tmp = ((float)(value - omin)) / ((float)(omax - omin));
  i = nmin + (int)(tmp * ((float)(nmax - nmin)));
  return i;
}

float FloatNormalize(float value,float omin,float omax,
		     float nmin, float nmax)
{
  float tmp,i;
  if ( (omax - omin) < 0.00001) return 0.0;
  tmp = (value - omin) / (omax - omin);
  i = nmin + tmp * (nmax - nmin);
  return i;
}
