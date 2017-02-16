#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "ip_common.h"

/* Defined in common.h */
/* WHITE     0   the node was never inserted in the priority queue */
/* GRAY      1   the node has been inserted in the priority queue */
/* BLACK     2   the node has been removed from the priority queue */
/* NIL      -1   Bucket is empty in the priority queue */

typedef struct _node { 
  int next;  /* next node */
  int prev;  /* prev node */
  char color; /* color of a node: WHITE=0, GRAY=1, or BLACK=2 */
} Node;

typedef struct _doublylinkedlists {
  Node *elem;  /* all possible doubly-linked lists of the circular queue */
  int nelems;  /* total number of elements */
} DoublyLinkedLists; 

typedef struct _circularqueue { 
  int *first;  /* list of the first elements of each doubly-linked list */
  int *last;   /* list of the last  elements of each doubly-linked list  */
  int nbuckets;  /* number of buckets in the circular queue */
  int current;   /* current bucket */
} CircularQueue;

typedef struct _queue { /* Priority queue by Dial implemented as
                           suggested by A. Falcao */
  CircularQueue C;
  DoublyLinkedLists L;
} Queue;


Queue *CreateQueue(int nbuckets, int nelems);
void   DestroyQueue(Queue **Q);
void   ResetQueue(Queue *Q);
int    EmptyQueue(Queue *Q);
void   InsertQueue(Queue *Q, int bucket, int elem);
int    RemoveQueue(Queue *Q);
void   RemoveQueueElem(Queue *Q, int elem, int bucket);
void   UpdateQueue(Queue *Q, int elem, int from, int to);
void   SetQueueCurrentBucket(Queue *Q, int bucket);

/* variation on the above, without the color field,
   might be slightly faster due to better element
   alignment (XNode struct is 8 bytes long, Node was 9) and
   the absence of color updates - F. Bergo */

typedef struct _xnode { 
  int next;  /* next node */
  int prev;  /* prev node */
} XNode;

typedef struct _fqueue {

  /*  circular queue */

  int *first;  /* list of the first elements of each doubly-linked list */
  int *last;   /* list of the last  elements of each doubly-linked list  */
  int nbuckets;  /* number of buckets in the circular queue */
  int current;   /* current bucket */

  /* doubly linked list */

  XNode *elem; /* all possible doubly-linked lists of the circular queue */
  int nelems;  /* total number of elements */

} FQueue; 

FQueue *CreateFQueue(int nbuckets, int nelems);
void    DestroyFQueue(FQueue **Q);
void    ResetFQueue(FQueue *Q);
int     EmptyFQueue(FQueue *Q);
void    InsertFQueue(FQueue *Q, int bucket, int elem);
int     RemoveFQueue(FQueue *Q);
void    RemoveFQueueElem(FQueue *Q, int elem, int bucket);
void    UpdateFQueue(FQueue *Q, int elem, int from, int to);


typedef struct _wqbucket {
  int head, tail;
} WQBucket;

typedef struct _wqelement {
  int  prev, next;
  char present;
} WQElement;

typedef struct _wqueue {
  int  N, nB, Count;
  int  B0, C0, HB;
  WQBucket  *B;
  WQElement *L;
} WQueue;


WQueue * WQCreate(int elements,int buckets);
void     WQDestroy(WQueue *Q);
void     WQInsert(WQueue *Q,int v, int cost);
int      WQRemove(WQueue *Q);
void     WQUpdate(WQueue *Q,int v, int oldcost, int newcost);
void     WQInsertOrUpdate(WQueue *Q,int v, int oldcost, int newcost);
int      WQIsEmpty(WQueue *Q);
int      WQIsQueued(WQueue *Q,int v);
void     WQRemoveElement(WQueue *Q, int v, int cost);

#endif
