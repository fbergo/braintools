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

/************************************************************
 $Id: set.c,v 1.5 2013/11/11 22:34:13 bergo Exp $

 set.c - set data structure

*************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ip_set.h"
#include "ip_optlib.h"
#include "ip_mem.h"

void InsertSet(Set **S, int elem)
{
  Set *p=NULL;

  p = (Set *) calloc(1,sizeof(Set));
  if (p == NULL) Error(MSG1,"InsertSet");
  if (*S == NULL){
    p->elem = elem;
    p->next = NULL;
  }else{
    p->elem = elem;
    p->next = *S;
  }
  *S = p;
}

void RemoveFromSet(Set **S, int elem) {
  Set *x,*y;
  if ( (*S)->elem == elem ) {
    x = (*S);
    (*S) = (*S)->next;
    MemFree(x);
  } else {
    for(x=*S;x->next;x=x->next) {
      if (x->next->elem == elem) {
	y=x->next;
	x->next = y->next;
	MemFree(y);
	break;
      }
    }
  }
}

int RemoveSet(Set **S)
{
  Set *p;
  int elem=NIL;
  
  if (*S != NULL){
    p    =  *S;
    elem = p->elem;
    *S   = p->next;
    MemFree(p);
  }

  return(elem);
}

void DestroySet(Set **S)
{
  Set *p;
  while(*S != NULL){
    p = *S;
    *S = p->next;
    MemFree(p);
  }
}


int  FindSet(Set **S, int elem) {
  Set *x;
  if (S==NULL) return 0;
  x=*S;
  while(x!=NULL) {
    if (x->elem == elem) return 1;
    x=x->next;
  }
  return 0;
}

int  EmptySet(Set **S) {
  if (S==NULL)  return 1;
  if (*S == NULL) return 1;
  return 0;
}

int  SizeOfSet(Set **S) {
  if (S==NULL) return 0;
  if (*S == NULL) return 0;
  return(1 + SizeOfSet( &((*S)->next) ));
}

/* map (for seed sets and histograms) */

#define MAP_GROWTH 512

Map * MapNew() {
  Map *x;

  x = (Map *) MemAlloc(sizeof(Map));
  x->data = 0;
  x->n  = 0;
  x->sz = 0;

  return x;
}

void  MapDestroy(Map *m) {
  if (m->data != 0)
    MemFree(m->data);
  MemFree(m);
}

void  MapAdd(Map *m, int a, int q) {
  int p;
  if (m->data == 0) {
    m->sz = MAP_GROWTH;
    m->data = (Pair *) MemAlloc(sizeof(Pair) * m->sz);
    m->n = 1;
    m->data[0].a = a;
    m->data[0].b = q;
  } else {
    p = MapFind(m,a);
    if (p>=0) {
      m->data[p].b += q;
    } else {
      if (m->sz == m->n) {
	m->sz += MAP_GROWTH;
	m->data = (Pair *) MemRealloc(m->data, sizeof(Pair) * m->sz);
      }
      m->data[m->n].a = a;
      m->data[m->n].b = q;
      ++(m->n);
    }
  }
}

void  MapSetUnique(Map *m, int a, int q) {
  if (m->data == 0) {
    m->sz = MAP_GROWTH;
    m->data = (Pair *) MemAlloc(sizeof(Pair) * m->sz);
    m->n = 1;
    m->data[0].a = a;
    m->data[0].b = q;
  } else {
    if (m->sz == m->n) {
      m->sz += MAP_GROWTH;
      m->data = (Pair *) MemRealloc(m->data, sizeof(Pair) * m->sz);
    }
    m->data[m->n].a = a;
    m->data[m->n].b = q;
    ++(m->n);
  }
}

void  MapSet(Map *m, int a, int q) {
  int p;
  if (m->data == 0) {
    m->sz = MAP_GROWTH;
    m->data = (Pair *) MemAlloc(sizeof(Pair) * m->sz);
    m->n = 1;
    m->data[0].a = a;
    m->data[0].b = q;
  } else {
    p = MapFind(m,a);
    if (p>=0) {
      m->data[p].b = q;
    } else {
      if (m->sz == m->n) {
	m->sz += MAP_GROWTH;
	m->data = (Pair *) MemRealloc(m->data, sizeof(Pair) * m->sz);
      }
      m->data[m->n].a = a;
      m->data[m->n].b = q;
      ++(m->n);
    }
  }
}

int   MapFind(Map *m, int a) {
  int i;

  if (!m) return -1;
  
  for(i=0;i< m->n ;i++)
    if (m->data[i].a == a)
      return i;
  
  return -1;
}

int   MapOf(Map *m, int a) {
  int i;
  i = MapFind(m, a);
  if (i<0) return -1; else return(m->data[i].b);
}

int   MapSize(Map *m) {
  if (m) return(m->n); else return(0);
}

int   MapEmpty(Map *m) {
  if (m) return(m->n==0); else return(1);
}

int   MapRemoveAny(Map *m,int *a,int *b) {
  if (!m || m->n == 0)
    return 0;

  *a = m->data[m->n-1].a;
  *b = m->data[m->n-1].b;
  --m->n;
  return 1;
}

int   MapRemove(Map *m,int a) {
  int i,b;

  i = MapFind(m,a);
  if (i<0) return -1;

  b = m->data[i].b;
  if (i != m->n-1) {
    m->data[i].a = m->data[m->n - 1].a;
    m->data[i].b = m->data[m->n - 1].b;
  }
  --(m->n);
  return b;
}

void  MapShrink(Map *m) {
  if (!m) return;
  if (m->sz - m->n > 64) {
    m->sz = m->n;
    if (m->sz != 0) {
      m->data = (Pair *) MemRealloc(m->data, m->sz * sizeof(Pair));
    } else {
      MemFree(m->data);
      m->data = 0;
    }
  }
}

void  MapClear(Map *m) {
  if (m) {
    m->n=0;
    MapShrink(m);
  }
}

int   MapGet(Map *m, int index, int *a, int *b) {
  if (!m)
    return 0;
  if (index < 0 || index >= m->n)
    return 0;

  *a = m->data[index].a;
  *b = m->data[index].b;
  return 1;
}

/* BMap */

BMap * BMapNew(int n) {
  BMap *x;
  x= (BMap *) MemAlloc(sizeof(BMap));
  if (!x) return 0;
  x->N    = n;
  x->VN   = n/8;
  if (n%8) x->VN++;
  x->data = (char *) MemAlloc(sizeof(char) * x->VN);
  if (!x->data) return 0;
  BMapFill(x,0);
  return x;
}

void   BMapDestroy(BMap *b) {
  if (b) {
    if (b->data) {
      MemFree(b->data);
      b->data = 0;
    }
    MemFree(b);
  }
}
void   BMapFill(BMap *b, int value) {
  MemSet(b->data, value?0xff:0, b->VN);
}

void   BMapCopy(BMap *dest, BMap *src) {
  int n;
  BMapFill(dest, 0);
  n = dest->VN;
  if (n > src->VN) n = src->VN;
  memcpy(dest->data,src->data,sizeof(char) * src->VN);
}

static char bmap_set[8]   = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
static char bmap_reset[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };

int    BMapGet(BMap *b, int n) {
  int thebyte, thebit, value;

  thebyte = n >> 3;
  thebit  = n & 0x07;

  value = b->data[thebyte] & bmap_set[thebit];
  if (value) value=1;
  return value;
}

void BMapSet(BMap *b, int n, int value) {
  int thebyte, thebit;

  thebyte = n >> 3;
  thebit  = n & 0x07;

  if (value)
    b->data[thebyte] |= bmap_set[thebit];
  else
    b->data[thebyte] &= bmap_reset[thebit];  
}

void BMapToggle(BMap *b, int n) {
  int thebyte, thebit;

  thebyte = n >> 3;
  thebit  = n & 0x07;

  b->data[thebyte] ^= bmap_set[thebit];
}
