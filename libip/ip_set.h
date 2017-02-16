#ifndef _SET_H_
#define _SET_H_

#include <ip_common.h>

typedef struct _set {
  int elem;
  struct _set *next;
} Set;

void InsertSet(Set **S, int elem);
int  RemoveSet(Set **S);
void RemoveFromSet(Set **S, int elem);
void DestroySet(Set **S);
int  FindSet(Set **S, int elem);
int  EmptySet(Set **S);
int  SizeOfSet(Set **S);

typedef struct _pair {
  int a,b;
} Pair;

typedef struct _map {
  Pair *data;
  int  n, sz;
} Map;

/*
  mapa: conjunto de pares (A,B) onde A ocorre no maximo
        uma vez no conjunto.

        usado para histogramas e conjuntos de sementes
        (A e' o voxel da semente, B o seu label)
*/

Map * MapNew();
void  MapDestroy(Map *m);
void  MapAdd(Map *m, int a, int q);
void  MapSet(Map *m, int a, int q);
void  MapSetUnique(Map *m, int a, int q);
int   MapFind(Map *m, int a);
int   MapOf(Map *m, int a);
int   MapSize(Map *m);
int   MapGet(Map *m, int index, int *a, int *b);
int   MapEmpty(Map *m);
int   MapRemoveAny(Map *m,int *a,int *b);
int   MapRemove(Map *m,int a);
void  MapShrink(Map *m);
void  MapClear(Map *m);

/*
  a bitmap structure
*/

typedef struct _bmap {
  char *data;
  int N, VN;
} BMap;

BMap * BMapNew(int n);
void   BMapDestroy(BMap *b);
void   BMapFill(BMap *b, int value);
int    BMapGet(BMap *b, int n);
void   BMapSet(BMap *b, int n, int value);
void   BMapToggle(BMap *b, int n);
void   BMapCopy(BMap *dest, BMap *src);

/* fast inline versions of the above */

#define _fast_BMapGet(b,n) ((b->data[n>>3]&(1<<(n&0x07)))!=0)
#define _fast_BMapSet(b,n,v) if (v) b->data[n>>3]|=(1<<(n&0x07)); else b->data[n>>3]&=((~0)^(1<<(n&0x07)));

#define _fast_BMapSet0(b,n) b->data[n>>3]&=((~0)^(1<<(n&0x07)));
#define _fast_BMapSet1(b,n) b->data[n>>3]|=(1<<(n&0x07));
#define _fast_BMapToggle(b,n) b->data[n>>3]^=(1<<(n&0x07));

#endif
