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

/*******************************************
 $Id: queue.c,v 1.5 2013/11/11 22:34:13 bergo Exp $

 queue.c - priority queue

********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ip_queue.h"
#include "ip_mem.h"

/* A priority queue Q consists of two data structures: a circular queue C and a table L that encodes all possible doubly-linked lists.

   Q requires that the maximum cost of an edge in the graph be a non-negative integer less than the number of buckets in C.
   Q->C.first[i] gives the first element that is in bucket i.
   Q->C.last[i]  gives the last  element that is in bucket i.
   Q->C.nbuckets gives the number of buckets in C.
   Q->current gives the current bucket being processed.

   All possible doubly-linked lists are represented in L. Each bucket contains a doubly-linked list that is treated as a FIFO.
   Q->L.elem[i].next: the next element to i 
   Q->L.elem[i].prev: the previous element to i
   Q->L.elem[i].color:  the color of an element with respect to Q: WHITE (never inserted), GRAY (inserted), BLACK (removed)
   Q->L.nelems: gives the total number of elements that can be inserted in Q (It is usually the number of pixels in a given image or the number of nodes in a graph)

   Insertions and deletions are done in O(1).
   Updates may take O(n), where n is the number of buckets.
*/

Queue *CreateQueue(int nbuckets, int nelems)
{
  Queue *Q=NULL;

  Q = (Queue *) MemAlloc(1*sizeof(Queue));
  
  if (Q != NULL) {
    Q->C.first = (int *)MemAlloc(nbuckets * sizeof(int));
    Q->C.last  = (int *)MemAlloc(nbuckets * sizeof(int));
    Q->C.nbuckets = nbuckets;
    if ( (Q->C.first != NULL) && (Q->C.last != NULL) ){
      Q->L.elem = (Node *)MemAlloc(nelems*sizeof(Node));
      Q->L.nelems = nelems;
      if (Q->L.elem != NULL){
	ResetQueue(Q);
      } else
	Error(MSG1,"CreateQueue");	
    } else
      Error(MSG1,"CreateQueue");
  } else 
    Error(MSG1,"CreateQueue");

  return(Q);
}

void ResetQueue(Queue *Q)
{
  int i;

  Q->C.current = 0;
  
  for (i=0; i < Q->C.nbuckets; i++)
    Q->C.first[i]=Q->C.last[i]=NIL;
	
  for (i=0; i < Q->L.nelems; i++){
    Q->L.elem[i].next =  Q->L.elem[i].prev = NIL;
    Q->L.elem[i].color = WHITE;
  }

}

void DestroyQueue(Queue **Q)
{
  Queue *aux;

  aux = *Q;
  if (aux != NULL) {
    if (aux->C.first != NULL) MemFree(aux->C.first);
    if (aux->C.last  != NULL) MemFree(aux->C.last);
    if (aux->L.elem  != NULL) MemFree(aux->L.elem);
    MemFree(aux);
    *Q = NULL;
  }
}

void SetQueueCurrentBucket(Queue *Q, int bucket) {
    Q->C.current = bucket % (Q->C.nbuckets);
}

void InsertQueue(Queue *Q, int bucket, int elem)
{
  if (Q->C.first[bucket] == NIL){ 
    Q->C.first[bucket]   = elem;  
    Q->L.elem[elem].prev = NIL;
  }else {
    Q->L.elem[Q->C.last[bucket]].next = elem;
    Q->L.elem[elem].prev = Q->C.last[bucket];
  }
  
  Q->C.last[bucket]     = elem;
  Q->L.elem[elem].next  = NIL;
  Q->L.elem[elem].color = GRAY;
}

int RemoveQueue(Queue *Q)
{
  int elem=NIL, next;
  int last;

  /** moves to next element or returns EMPTY queue **/
  if (Q->C.first[Q->C.current] == NIL) {
    last = Q->C.current;
    
    Q->C.current = (Q->C.current + 1) % (Q->C.nbuckets);
    
    while ((Q->C.first[Q->C.current] == NIL) && (Q->C.current != last)) {
      Q->C.current = (Q->C.current + 1) % (Q->C.nbuckets);
    }
    
    if (Q->C.first[Q->C.current] == NIL)
      return NIL;
  }

  elem = Q->C.first[Q->C.current];
  
  next = Q->L.elem[elem].next;
  if (next == NIL) {         /* there is a single element in the list */
    Q->C.first[Q->C.current] = Q->C.last[Q->C.current]  = NIL;    
  }
  else {
    Q->C.first[Q->C.current] = next;
    Q->L.elem[next].prev = NIL;
  }

  Q->L.elem[elem].color = BLACK;

  return elem;
}

void RemoveQueueElem(Queue *Q, int elem, int bucket)
{
  int prev,next;

  prev = Q->L.elem[elem].prev;
  next = Q->L.elem[elem].next;
  Q->L.elem[elem].color = BLACK;

  /* if elem is the first element */
  if (Q->C.first[bucket] == elem) {
    Q->C.first[bucket] = next;
    if (next == NIL) /* elem is also the last one */
      Q->C.last[bucket] = NIL;
    else
      Q->L.elem[next].prev = NIL;
  }
  else{   /* elem is in the middle or it is the last */
    Q->L.elem[prev].next = next;
    if (next == NIL) /* if it is the last */
      Q->C.last[bucket] = prev;
    else 
      Q->L.elem[next].prev = prev;
  }
}

void UpdateQueue(Queue *Q, int elem, int from, int to)
{
  RemoveQueueElem(Q, elem, from);
  InsertQueue(Q, to, elem);
}

int EmptyQueue(Queue *Q)
{
  int last;
  if (Q->C.first[Q->C.current] != NIL)
    return 0;
  
  last = Q->C.current;
  
  Q->C.current = (Q->C.current + 1) % (Q->C.nbuckets);
  
  while ((Q->C.first[Q->C.current] == NIL) && (Q->C.current != last)) {
    Q->C.current = (Q->C.current + 1) % (Q->C.nbuckets); 
  }
  
  return (Q->C.first[Q->C.current] == NIL);
}


/* --------------------- FQUEUE ------------------------ */

FQueue *CreateFQueue(int nbuckets, int nelems) {
  FQueue *Q=NULL;

  Q = (FQueue *) MemAlloc(1*sizeof(FQueue));
  
  if (Q != NULL) {
    Q->first = (int *)MemAlloc(nbuckets * sizeof(int));
    Q->last  = (int *)MemAlloc(nbuckets * sizeof(int));
    Q->nbuckets = nbuckets;
    if ( (Q->first != NULL) && (Q->last != NULL) ){
      Q->elem = (XNode *)MemAlloc(nelems*sizeof(XNode));
      Q->nelems = nelems;
      if (Q->elem != NULL){
	ResetFQueue(Q);
      } else
	Error(MSG1,"CreateFQueue");	
    } else
      Error(MSG1,"CreateFQueue");
  } else 
    Error(MSG1,"CreateFQueue");

  return(Q);
}

void    DestroyFQueue(FQueue **Q) {
  FQueue *aux;

  aux = *Q;
  if (aux != NULL) {
    if (aux->first != NULL) MemFree(aux->first);
    if (aux->last  != NULL) MemFree(aux->last);
    if (aux->elem  != NULL) MemFree(aux->elem);
    MemFree(aux);
    *Q = NULL;
  }
}

void    ResetFQueue(FQueue *Q) {
  unsigned int i;

  Q->current = 0;
  
  for (i=0; i < Q->nbuckets; i++)
    Q->first[i]=Q->last[i]=NIL;
	
  for (i=0; i < Q->nelems; i++)
    Q->elem[i].next =  Q->elem[i].prev = NIL;

}

int     EmptyFQueue(FQueue *Q) {
  int last;
  if (Q->first[Q->current] != NIL)
    return 0;
  
  last = Q->current;
  
  Q->current = (Q->current + 1) % (Q->nbuckets);
  
  while ((Q->first[Q->current] == NIL) && (Q->current != last)) {
    Q->current = (Q->current + 1) % (Q->nbuckets); 
  }
  
  return (Q->first[Q->current] == NIL);
}

void    InsertFQueue(FQueue *Q, int bucket, int elem) {
  if (Q->first[bucket] == NIL){ 
    Q->first[bucket]   = elem;  
    Q->elem[elem].prev = NIL;
  }else {
    Q->elem[Q->last[bucket]].next = elem;
    Q->elem[elem].prev = Q->last[bucket];
  }
  
  Q->last[bucket]     = elem;
  Q->elem[elem].next  = NIL;
}

int     RemoveFQueue(FQueue *Q) {
  int elem=NIL, next;
  int last;

  /** moves to next element or returns EMPTY queue **/
  if (Q->first[Q->current] == NIL) {
    last = Q->current;
    
    Q->current = (Q->current + 1) % (Q->nbuckets);
    
    while ((Q->first[Q->current] == NIL) && (Q->current != last)) {
      Q->current = (Q->current + 1) % (Q->nbuckets);
    }
    
    if (Q->first[Q->current] == NIL)
      return NIL;
  }

  elem = Q->first[Q->current];
  
  next = Q->elem[elem].next;
  if (next == NIL) {         /* there is a single element in the list */
    Q->first[Q->current] = Q->last[Q->current]  = NIL;    
  }
  else {
    Q->first[Q->current] = next;
    Q->elem[next].prev = NIL;
  }

  return elem;
}

void    RemoveFQueueElem(FQueue *Q, int elem, int bucket) {
  int prev,next;

  prev = Q->elem[elem].prev;
  next = Q->elem[elem].next;
  
  /* if elem is the first element */
  if (Q->first[bucket] == elem) {
    Q->first[bucket] = next;
    if (next == NIL) /* elem is also the last one */
      Q->last[bucket] = NIL;
    else
      Q->elem[next].prev = NIL;
  }
  else{   /* elem is in the middle or it is the last */
    Q->elem[prev].next = next;
    if (next == NIL) /* if it is the last */
      Q->last[bucket] = prev;
    else 
      Q->elem[next].prev = prev;
  }
}

void    UpdateFQueue(FQueue *Q, int elem, int from, int to) {
  RemoveFQueueElem(Q, elem, from);
  InsertFQueue(Q, to, elem);
}

/* --- windowed queue */

WQueue * WQCreate(int elements,int buckets) {
  int i;
  WQueue *Q;

  Q = (WQueue *) MemAlloc(sizeof(WQueue));
  if (!Q) return 0;
  
  Q->nB     = buckets;
  Q->N      = elements;

  Q->Count  = 0;
  Q->B0     = 0;
  Q->C0     = 0;
  Q->HB     = 0;

  Q->B = (WQBucket *)  MemAlloc(sizeof(WQBucket) * Q->nB);
  Q->L = (WQElement *) MemAlloc(sizeof(WQElement) * Q->N);

  if (!Q->B) return 0;
  if (!Q->L) return 0;

  for(i=0;i<Q->nB;i++)
    Q->B[i].head = Q->B[i].tail = -1; /* nil */

  for(i=0;i<Q->N;i++)
    Q->L[i].present = 0;

  return Q;
}

void WQDestroy(WQueue *Q) {
  if (Q) {
    if (Q->B) MemFree(Q->B);
    if (Q->L) MemFree(Q->L);
    MemFree(Q);
  }
}

void WQInsert(WQueue *Q,int v, int cost) {
  int bucket;
  bucket = (Q->B0 + cost - Q->C0) % Q->nB;
  if (Q->B[bucket].head == -1) {
    Q->B[bucket].head = v;
    Q->L[v].prev = -1;
  } else {
    Q->L[Q->B[bucket].tail].next = v;
    Q->L[v].prev = Q->B[bucket].tail;
  }
  Q->L[v].next = -1;
  Q->L[v].present = 1;
  Q->B[bucket].tail = v;
  ++(Q->Count);
}

int WQRemove(WQueue *Q) {
  int bucket, v;
  if (Q->Count==0) return -1;
  bucket = Q->HB;
  while(Q->B[bucket].head == -1)
    bucket = (bucket + 1) % Q->nB;
  v = Q->B[bucket].head;
  if (Q->L[v].next == -1) {
    Q->B[bucket].head = -1;
    Q->B[bucket].tail = -1;
  } else {
    Q->B[bucket].head = Q->L[v].next;
    Q->L[Q->L[v].next].prev = -1;
  }
  Q->L[v].present = 0;
  --(Q->Count);
  if (bucket != Q->B0) {
    Q->C0 = Q->C0 + bucket - Q->B0;
    Q->B0 = bucket;
  }
  Q->HB = bucket;
  return v;
}

int WQIsEmpty(WQueue *Q) {
  return(Q->Count == 0);
}

int WQIsQueued(WQueue *Q,int v) {
  return((int)(Q->L[v].present));
}

void WQUpdate(WQueue *Q,int v, int oldcost, int newcost) {
  WQRemoveElement(Q, v, oldcost);
  WQInsert(Q, v, newcost);
}

void WQInsertOrUpdate(WQueue *Q,int v, int oldcost, int newcost) {
  if (Q->L[v].present)
    WQRemoveElement(Q, v, oldcost);
  WQInsert(Q, v, newcost);

}

void WQRemoveElement(WQueue *Q, int v, int cost) {
  int bucket;
  bucket = (Q->B0 + cost - Q->C0) % Q->nB;
  if (Q->L[v].prev == -1)
    Q->B[bucket].head = Q->L[v].next;
  else
    Q->L[Q->L[v].prev].next = Q->L[v].next;
  if (Q->L[v].next == -1)
    Q->B[bucket].tail = Q->L[v].prev;
  else
    Q->L[Q->L[v].next].prev = Q->L[v].prev;
  Q->L[v].present = 0;
  --(Q->Count);
}
