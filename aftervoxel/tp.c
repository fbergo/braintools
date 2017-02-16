
#define SOURCE_TP_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "aftervoxel.h"

typedef struct _tp_adjacency {
  int *dx,*dy,*dz;
  int n,sz;
} adjacency;

typedef struct _tp_ift_queue {
  int nb, ne, current, *first, *last, *prev, *next;
  char *color;
} ift_queue;

typedef struct _fifoq {
  int *data;
  int getq, putq, qsz, count, growstep;
} fifoq;

#define IFT_WHITE 0
#define IFT_GRAY  1
#define IFT_BLACK 2
#define IFT_NIL   -1

fifoq *fifoq_new(int initialsz, int growstep);
void   fifoq_destroy(fifoq *q);
void   fifoq_push(fifoq *q, int x);
int    fifoq_pop(fifoq *q);
int    fifoq_empty(fifoq *q);
void   fifoq_reset(fifoq *q);
void   fifoq_grow(fifoq *q);

ift_queue * ift_queue_new(int nelem, int nbucket);
void        ift_queue_reset(ift_queue *q);
void        ift_queue_destroy(ift_queue *q);
int         ift_queue_empty(ift_queue *q);
void        ift_queue_insert(ift_queue *q, int elem, int bucket);
int         ift_queue_remove(ift_queue *q);
void        ift_queue_update(ift_queue *q, int elem, int from, int to);
void        ift_queue_remove_elem(ift_queue *q, int elem, int bucket);

adjacency * adjacency_new();
void        adjacency_grow(adjacency *a, int newsz);
adjacency * adjacency_spherical_new(float radius, int self);
void        adjacency_destroy(adjacency *a);

int       compute_otsu(Volume16 *v);
void      means_above_below_t(Volume16 *v, int t, int *t1, int *t2);
Volume16 *apply_s_shape(Volume16 *v,int a,int b,int c);
Volume8  *bin_erode(Volume8 *v, float radius);
void      select_largest_comp(Volume8 *v, adjacency *a);
float     feature_distance(float *a, float *b, int n);
Volume8  *volume_border(int w, int h, int d);


void fifoq_reset(fifoq *q) {
  q->count = q->getq = q->putq = 0;
}

fifoq *fifoq_new(int initialsz, int growstep) {
  fifoq *q;
  q = (fifoq *) malloc(sizeof(fifoq));
  if (q==NULL) return NULL;
  q->qsz      = initialsz;
  q->growstep = growstep;
  q->count    = 0;
  q->getq     = 0;
  q->putq     = 0;
  q->data     = (int *) malloc(q->qsz * sizeof(int));
  if (q->data == NULL) { free(q); return NULL; }
  return q;
}

void fifoq_destroy(fifoq *q) {
  if (q!=NULL) {
    if (q->data != NULL)
      free(q->data);
    free(q);
  }
}

void fifoq_push(fifoq *q, int x) {
  if (q->count == q->qsz) fifoq_grow(q);
  q->data[q->putq] = x;
  q->putq = (q->putq + 1) % q->qsz;
  q->count++;
}

int fifoq_pop(fifoq *q) {
  int x;
  x = q->data[q->getq];
  q->getq = (q->getq + 1) % q->qsz;
  q->count--;
  return x;
}

int fifoq_empty(fifoq *q) {
  return(q->count == 0);
}

void fifoq_grow(fifoq *q) {
  printf("growing!\n");
  q->data = (int *) realloc(q->data, (q->qsz + q->growstep) * sizeof(int));
  if (q->data == NULL) return; // oops
  if (q->putq <= q->getq) {
    // fifo is wrapped on qsz, shift [getq,qsz) to [getq+growstep,qsz+growstep)
    memmove(&(q->data[q->getq + q->growstep]), &(q->data[q->getq]), 
	    (q->qsz - q->getq) * sizeof(int));
    q->getq += q->growstep;
  }
  q->qsz += q->growstep;
}

ift_queue * ift_queue_new(int nelem, int nbucket) {
  ift_queue *q;
  q = (ift_queue *) malloc(sizeof(ift_queue));
  if (q==NULL) return NULL;
  q->nb = nbucket;
  q->ne = nelem;
  q->first = (int *) malloc(q->nb * sizeof(int));
  q->last  = (int *) malloc(q->nb * sizeof(int));
  q->prev  = (int *) malloc(q->ne * sizeof(int));
  q->next  = (int *) malloc(q->ne * sizeof(int));
  q->color = (char *) malloc(q->ne * sizeof(char));
  ift_queue_reset(q);
  return q;
}

void ift_queue_reset(ift_queue *q) {
  int i;
  if (q==NULL) return;
  q->current = 0;
  for(i=0;i<q->nb;i++)
    q->first[i] = q->last[i] = IFT_NIL;

  for(i=0;i<q->ne;i++) {
    q->prev[i] = q->next[i] = IFT_NIL;
    q->color[i] = IFT_WHITE;
  }
}

void ift_queue_destroy(ift_queue *q) {
  if (q!=NULL) {
    if (q->first != NULL) free(q->first);
    if (q->last != NULL) free(q->last);
    if (q->next != NULL) free(q->next);
    if (q->prev != NULL) free(q->prev);
    if (q->color != NULL) free(q->color);
    free(q);
  }
}

int ift_queue_empty(ift_queue *q) {
  int i;
  for(i=q->current;i<q->nb;i++)
    if (q->first[i] != IFT_NIL)
      return 0;
  return 1;
}

void ift_queue_insert(ift_queue *q, int elem, int bucket) {
  if (q->first[bucket] == IFT_NIL) {
    q->first[bucket] = elem;
    q->prev[elem] = IFT_NIL;
  } else {
    q->next[q->last[bucket]] = elem;
    q->prev[elem] = q->last[bucket];
  }
  q->last[bucket] = elem;
  q->next[elem] = IFT_NIL;
  q->color[elem] = IFT_GRAY;
  if (bucket < q->current) q->current = bucket;
}

int ift_queue_remove(ift_queue *q) {
  int elem,n;
  
  while(q->first[q->current]==IFT_NIL && q->current < q->nb)
    ++(q->current);
  if (q->current == q->nb) return IFT_NIL;
  elem = q->first[q->current];
  
  n = q->next[elem];
  if (n==IFT_NIL) {
    q->first[q->current] = q->last[q->current] = IFT_NIL;
  } else {
    q->first[q->current] = n;
    q->prev[n] = IFT_NIL;
  }
  q->color[elem] = IFT_BLACK;
  return elem;
}

void ift_queue_update(ift_queue *q, int elem, int from, int to) {
  ift_queue_remove_elem(q,elem,from);
  ift_queue_insert(q,elem,to);
}

void ift_queue_remove_elem(ift_queue *q, int elem, int bucket) {
  int p,n;

  p = q->prev[elem];
  n = q->next[elem];
  
  q->color[elem] = IFT_BLACK;
  
  if (q->first[bucket] == elem) {
    q->first[bucket] = n;
    if (n==IFT_NIL)
      q->last[bucket] = IFT_NIL;
    else
      q->prev[n] = IFT_NIL;
  } else {
    q->next[p] = n;
    if (n==IFT_NIL)
      q->last[bucket] = p;
    else
      q->prev[n] = p;
  }
}

Volume8 *bin_erode(Volume8 *v, float radius) {
  Volume32 *edt, *root;
  Volume8 *dest;
  int rlimit;
  
  Voxel q,r,rr;
  adjacency *A;

  ift_queue *Q;
  int ar,c1,c2,n,i,j,p;

  A = adjacency_spherical_new(1.5,0);
  
  rlimit = 1 + (int) (radius * radius);
  
  edt  = VolumeNew(v->W,v->H,v->D,vt_integer_32);
  root = VolumeNew(v->W,v->H,v->D,vt_integer_32);

  Q = ift_queue_new(v->N,(v->W)*(v->W)+(v->H)*(v->H)+(v->D)*(v->D)+1);
  
  for(p=0;p<v->N;p++) {
    if (v->u.i8[p] == 0) {
      edt->u.i32[p]  = 0;
      root->u.i32[p] = p;
      ift_queue_insert(Q,p,0);
    } else {
      edt->u.i32[p] = rlimit;
    }
  }
  
  while(!ift_queue_empty(Q)) {
    n = ift_queue_remove(Q);
    
    VolumeVoxelOf(edt, n, &q);
    j = root->u.i32[n];
    VolumeVoxelOf(edt, j, &rr);
    for(i=0;i<A->n;i++) {
      r.x = q.x + A->dx[i];
      r.y = q.y + A->dy[i];
      r.z = q.z + A->dz[i];

      if (VolumeValid(edt, r.x, r.y, r.z)) {
	ar = r.x + edt->tbrow[r.y] + edt->tbframe[r.z];
	c2 = (r.x - rr.x)*(r.x - rr.x) + (r.y - rr.y)*(r.y - rr.y) + (r.z - rr.z)*(r.z - rr.z);
	c1 = edt->u.i32[ar];
	if (c2 < c1) {
	  edt->u.i32[ar] = c2;
	  root->u.i32[ar] = j;
	  if (Q->color[ar] == IFT_GRAY)
	    ift_queue_update(Q,ar,c1,c2);
	  else
	    ift_queue_insert(Q,ar,c2);
	}
      }
    }
  }
  ift_queue_destroy(Q);
  adjacency_destroy(A);
  VolumeDestroy(root);
  
  dest = VolumeNew(edt->W,edt->H,edt->D,vt_integer_8);
  for(i=0;i<edt->N;i++)
    dest->u.i8[i] = edt->u.i32[i] < rlimit ? 0 : 1;
  VolumeDestroy(edt);
  return dest;
}

adjacency * adjacency_new() {
  adjacency *a;
  a = (adjacency *) malloc(sizeof(adjacency));
  a->n = 0;
  a->sz = 0;
  a->dx = a->dy = a->dz = NULL;
  return a;
}

void adjacency_grow(adjacency *a, int newsz) {
  if (a == NULL) return;
  if (a->dx == NULL || a->sz == 0) {
    a->sz = newsz;
    a->dx = (int *) malloc(newsz * sizeof(int));
    a->dy = (int *) malloc(newsz * sizeof(int));
    a->dz = (int *) malloc(newsz * sizeof(int));
  } else {
    a->sz = newsz;
    a->dx = (int *) realloc(a->dx, newsz * sizeof(int));
    a->dy = (int *) realloc(a->dy, newsz * sizeof(int));
    a->dz = (int *) realloc(a->dz, newsz * sizeof(int));
  }
}

adjacency * adjacency_spherical_new(float radius, int self) {
  int dx,dy,dz,r0,r2,rs;
  adjacency *a;

  r0  = (int) radius;
  r2  = (int) (radius*radius);

  a = adjacency_new();  
  adjacency_grow(a,32);

  for(dz=-r0;dz<=r0;dz++)
    for(dy=-r0;dy<=r0;dy++)
      for(dx=-r0;dx<=r0;dx++) {
	rs = dx*dx + dy*dy + dz*dz;
	if ((rs > 0  || self) && rs <= r2) {
	  if (a->n == a->sz) adjacency_grow(a, a->sz+32);
	  a->dx[a->n] = dx;
	  a->dy[a->n] = dy;
	  a->dz[a->n] = dz;
	  a->n++;
	}
      }
  return a;
}

void adjacency_destroy(adjacency *a) {
  if (a!=NULL) {
    if (a->dx != NULL) free(a->dx);
    if (a->dy != NULL) free(a->dy);
    if (a->dz != NULL) free(a->dz);
    free(a);
  }
}

int compute_otsu(Volume16 *v) {
  double *hist;
  int i,t,hn,imax,topt=0;
  double p1,p2,m1,m2,s1,s2,j,jmax=-1.0;
  
  imax = VolumeMax(v);
  hn = imax + 1;
  hist = (double *) malloc(hn * sizeof(double));
  for(i=0;i<hn;i++) hist[i] = 0;
  for(i=0;i<v->N;i++) ++hist[v->u.i16[i]];
  for(i=0;i<hn;i++) hist[i] /= ((double) (v->N));
  
  for(t=1;t<imax;t++) {
    for(i=0,p1=0.0;i<=t;i++) p1 += hist[i];
    p2 = 1.0 - p1;
    if ((p1>0.0)&&(p2>0.0)) {
      for(i=0,m1=0.0;i<=t;i++) m1 += hist[i] * i;
      m1 /= p1;
      for(i=t+1,m2=0.0;i<=imax;i++) m2 += hist[i] * i;
      m2 /= p2;
      for(i=0,s1=0.0;i<=t;i++) s1 += hist[i] * (i-m1) * (i-m1);
      s1 /= p1;
      for(i=t+1,s2=0.0;i<=imax;i++) s2 += hist[i] * (i-m2) * (i-m2);
      s2 /= p2;
      j = (p1*p2*(m1-m2)*(m1-m2))/(p1*s1+p2*s2);
    } else {
      j = 0.0;
    }
    if (j > jmax) { jmax = j; topt = t; }
  }
  free(hist);
  return topt;
}

void means_above_below_t(Volume16 *v, int t, int *t1, int *t2) {
  int64_t m1=0, m2=0, nv1=0, nv2=0;
  int p,delta;
  for(p=0;p<v->N;p++) {
    if (v->u.i16[p] < t) {
      m1 += v->u.i16[p];
      ++nv1;
    } else if (v->u.i16[p]>t) {
      m2 += v->u.i16[p];
      ++nv2;
    }
  }
  delta = (int) ((m2/nv2 - m1/nv1)/2);
  *t1 = t+delta;
  *t2 = t-delta;
}

Volume16 *apply_s_shape(Volume16 *v,int a,int b,int c) {

  Volume16 *dest;
  float x;
  int p;
  float weight,sqca;
 
  dest = VolumeNew(v->W,v->H,v->D,vt_integer_16);
  sqca = (float) ((c-a)*(c-a));
 
 for(p=0;p<v->N;p++) {
   x = (float) (v->u.i16[p]);
   if (x<=a) {
     weight = 0.0;
   } else {
     if (x>a && x<=b) {
       weight = (2.0* (x-a) * (x-a)) / sqca;
     } else if (x>b && x<=c) {
       weight = (1.0 - 2.0*(x-c)*(x-c)/sqca );
     } else {
       weight = 1.0;
     }
   }
   dest->u.i16[p] = (int) (weight * x);
 }
 return dest;
}

void select_largest_comp(Volume8 *v, adjacency *a) {
  Volume32 *label;
  fifoq *Q;
  int i,j,k,L=0,aq;
  Voxel p,q;
  int bestarea=0,bestcomp=0,area=0;
  
  label = VolumeNew(v->W,v->H,v->D,vt_integer_32);
  Q = fifoq_new(v->N, 4096);

  for(i=0;i<v->N;i++) {
    if (v->u.i8[i]==1 && label->u.i32[i]==0) {
      area = 1;
      L++;
      label->u.i32[i] = L;
      fifoq_reset(Q);
      fifoq_push(Q,i);

      while(!fifoq_empty(Q)) { 
	j = fifoq_pop(Q);
	
	VolumeVoxelOf(v, j, &p);
	for(k=0;k<a->n;k++) {
	  q.x = p.x + a->dx[k];
	  q.y = p.y + a->dy[k];
	  q.z = p.z + a->dz[k];
	  if (VolumeValid(v,q.x,q.y,q.z)) {
	    aq = q.x + v->tbrow[q.y] + v->tbframe[q.z];
	    if (v->u.i8[aq]==1 && label->u.i32[aq]==0) {
	      label->u.i32[aq] = L;
	      ++area;
	      fifoq_push(Q,aq);
	    }
	  }
	}
      }
    }
    if (area > bestarea) {
      //printf("L=%d area=%d total=%d\n",L,area,v->N);
      bestarea = area;
      bestcomp = L;
    }
  }
  
  for(i=0;i<v->N;i++)
    if (label->u.i32[i] != bestcomp)
      v->u.i8[i] = 0;
  
  VolumeDestroy(label);
  fifoq_destroy(Q);
}

Volume8 * tp_brain_marker_comp(Volume16 *orig) {

  int i,a=0,b,c=0,x;
  Volume8  *dest,*ero;
  Volume16 *tmp;
  adjacency *r1;

  r1 = adjacency_spherical_new(1.0, 0);
  
  b = compute_otsu(orig);
  means_above_below_t(orig,b,&c,&a);
  tmp = apply_s_shape(orig,a,b,c);

  dest = VolumeNew(orig->W,orig->H,orig->D,vt_integer_8);
  for(i=0;i<tmp->N;i++) {
    x = tmp->u.i16[i];
    if (x >= b && x < 4096)
      dest->u.i8[i] = 1;
  }
  VolumeDestroy(tmp);

  //VolumeSaveSCN8(dest,"tpm0.scn");
  //printf("starting erode\n");
  ero = bin_erode(dest, 5.0);
  //printf("ended erode\n");
  //VolumeSaveSCN8(ero,"tpm1.scn");
  VolumeDestroy(dest);
  select_largest_comp(ero, r1);
  //printf("ended largest comp\n");

  adjacency_destroy(r1);

  return ero;
}

Volume16 * tp_brain_otsu_enhance(Volume16 *orig) {
  int a=0,b,c=0;
  b = compute_otsu(orig);
  means_above_below_t(orig,b,&c,&a);
  return(apply_s_shape(orig,a,b,c));
}

float feature_distance(float *a, float *b, int n) {
  int i;
  float dist=0.0;
  for (i=0;i<n;i++)
    dist += (b[i]-a[i])/2.0;
  dist /= n;
  return(exp(-(dist-0.5)*(dist-0.5)/2.0));
}

Volume16 * tp_brain_feature_gradient(Volume16 *orig) {
  Volume16 * dest;
  int i,ap,aq,W,H,D,N;
  float *f,dist,gx,gy,gz,v,imax,mg[6];
  adjacency *a6, *a7;
  Voxel p,q;

  W = orig->W;
  H = orig->H;
  D = orig->D;
  N = orig->N;
  
  imax = (float) VolumeMax(orig);
  f = (float *) malloc(N*7*sizeof(float));
  if (f==NULL) return NULL;
  
  a6 = adjacency_spherical_new(1.0,0);
  a7 = adjacency_spherical_new(1.0,1);
  dest = VolumeNew(W,H,D,vt_integer_16);
  
  for(p.z=0;p.z<D;p.z++)
    for(p.y=0;p.y<H;p.y++)
      for(p.x=0;p.x<W;p.x++) {
	ap = p.x + orig->tbrow[p.y] + orig->tbframe[p.z];
	for(i=0;i<a7->n;i++) {
	  q.x = p.x + a7->dx[i];
	  q.y = p.y + a7->dy[i];
	  q.z = p.z + a7->dz[i];
	  if (VolumeValid(orig,q.x,q.y,q.z)) {
	    f[(7*ap)+i] = orig->u.i16[ q.x + orig->tbrow[q.y] + orig->tbframe[q.z] ] / imax ;
	  } else
	    f[(7*ap)+i] = 0.0;
	}
      }
  
  p.x = p.y = p.z = 0;

  for(i=0;i<6;i++)
    mg[i] = sqrt( a6->dx[i] * a6->dx[i] + a6->dy[i] * a6->dy[i] + a6->dz[i] * a6->dz[i] ); 
  
  for(p.z=0;p.z<D;p.z++) {
    for(p.y=0;p.y<H;p.y++) {
      for(p.x=0;p.x<W;p.x++) {
	ap = p.x + orig->tbrow[p.y] + orig->tbframe[p.z];
	gx = gy = gz = 0.0;
	for(i=0;i<a6->n;i++) {
	  q.x = p.x + a6->dx[i];
	  q.y = p.y + a6->dy[i];
	  q.z = p.z + a6->dz[i];
	  if (VolumeValid(orig,q.x,q.y,q.z)) {
	    aq = q.x + orig->tbrow[q.y] + orig->tbframe[q.z];
	    dist = feature_distance(&f[7*ap],&f[7*aq],7);
	    gx += (dist * a6->dx[i]) / mg[i];
	    gy += (dist * a6->dy[i]) / mg[i];
	    gz += (dist * a6->dz[i]) / mg[i];
	  }
	}
	v = 100000.0 * sqrt(gx*gx + gy*gy + gz*gz);
	dest->u.i16[ap] = (i16_t) v;
      }
    }
  }
  
  free(f);  
  adjacency_destroy(a6);
  adjacency_destroy(a7);
  return dest;
}

Volume8 *volume_border(int w, int h, int d) {
  Volume8 *B;
  int i,j;
  B = VolumeNew(w,h,d,vt_integer_8);
  if (B==NULL) return NULL;
  if (d>=3)
    for(i=0;i<w;i++)
      for(j=0;j<h;j++)
	B->u.i8[i+B->tbrow[j]+B->tbframe[0]] = B->u.i8[i+B->tbrow[j]+B->tbframe[d-1]] = 1;
  if (h>=3)
    for(i=0;i<w;i++)
      for(j=0;j<d;j++)
	B->u.i8[i+B->tbrow[0]+B->tbframe[j]] = B->u.i8[i+B->tbrow[h-1]+B->tbframe[j]] = 1;
  if (w>=3)
    for(i=0;i<h;i++)
      for(j=0;j<d;j++)
	B->u.i8[0+B->tbrow[i]+B->tbframe[j]] = B->u.i8[w-1+B->tbrow[i]+B->tbframe[j]] = 1;
  return B;
}

Volume8 * tp_brain_tree_pruning(Volume16 *grad, Volume8 *markers) {
  ift_queue *Q;
  Volume16 *cost;
  Volume32 *pred, *dcount;
  Volume8 *seg, *B;
  adjacency *a6;
  int i,j,k,maxval,inf,p,c1,c2,dmax,pmax=0,gmax,W,H,D,N,ar;
  Voxel q,r;
  fifoq *fifo;

  W = grad->W;
  H = grad->H;
  D = grad->D;
  N = grad->N;

  a6 = adjacency_spherical_new(1.0,0);
  fifo = fifoq_new(N, 4096);
  
  /* compute IFT */
  cost = VolumeNew(W,H,D,vt_integer_16);
  pred = VolumeNew(W,H,D,vt_integer_32);

  VolumeFill(pred,-1);
  
  maxval = (int) VolumeMax(grad);
  inf = maxval + 1;
  
  Q = ift_queue_new(N,maxval+1);
  
  for(i=0;i<N;i++) {
    if (markers->u.i8[i] != 0) {
      cost->u.i16[i] = 0;
      ift_queue_insert(Q,i,0);
    } else {
      cost->u.i16[i] = inf;
    }
  }
  
  while(!ift_queue_empty(Q)) {
    p = ift_queue_remove(Q);
    VolumeVoxelOf(grad,p,&q);
    for(i=0;i<a6->n;i++) {
      r.x = q.x + a6->dx[i];
      r.y = q.y + a6->dy[i];
      r.z = q.z + a6->dz[i];
      if (VolumeValid(grad,r.x,r.y,r.z)) {
	ar = r.x + grad->tbrow[r.y] + grad->tbframe[r.z];
	c1 = cost->u.i16[ar];
	c2 = MAX( cost->u.i16[p], grad->u.i16[ar] );
	if (c2 < c1) {
	  cost->u.i16[ar] = c2;
	  pred->u.i32[ar] = p;
	  ift_queue_insert(Q,ar,c2);
	}
      }
    }
  }
  
  VolumeDestroy(cost);
  
  /* compute descendant count (old method) */
  dcount = VolumeNew(W,H,D,vt_integer_32);
  B = volume_border(W,H,D);
  for(i=0;i<N;i++) {
    if (B->u.i8[i] != 0) {
      j = i;
      while(pred->u.i32[j] != -1) {
	dcount->u.i32[pred->u.i32[j]]++;
	j = pred->u.i32[j];
      }
    }
  }
  
  /* find leaking points */
  for(i=0;i<N;i++) {
    if (B->u.i8[i] == 1) {
      j = i;
      dmax = 0;
      while(pred->u.i32[j] != -1) {
	if (dcount->u.i32[j] > dmax) {
	  dmax = dcount->u.i32[j];
	  pmax = j;
	}
	j = pred->u.i32[j];
      }
      k = pmax;
      gmax = grad->u.i16[k];
      while(pred->u.i32[k] != -1) {
	if ( (dcount->u.i32[k] == dmax) && (gmax < grad->u.i16[k]) ) {
	  pmax = k;
	  gmax = grad->u.i16[k];
	}
	k = pred->u.i32[k];
      }
      
      B->u.i8[pmax] = 2; // set leaking point
    }
  }
  
  /* prune trees */
  seg = VolumeNew(W,H,D,vt_integer_8);
  
  for(i=0;i<N;i++) {
    if (pred->u.i32[i] == -1) {
      seg->u.i8[i] = 1;

      fifoq_push(fifo,i);
      
      while(!fifoq_empty(fifo)) {
	j = fifoq_pop(fifo);
	VolumeVoxelOf(grad, j, &q);
	seg->u.i8[j] = 1;
	
	for(k=0;k<a6->n;k++) {
	  r.x = q.x + a6->dx[k];
	  r.y = q.y + a6->dy[k];
	  r.z = q.z + a6->dz[k];
	  if (VolumeValid(grad, r.x, r.y, r.z)) {
	    ar = r.x + grad->tbrow[r.y] + grad->tbframe[r.z];
	    if ( (pred->u.i32[ar] == j) && (B->u.i8[ar] != 2) )
	      fifoq_push(fifo,ar);
	  }
	}
      }
    }
  }
  
  VolumeDestroy(pred);
  VolumeDestroy(dcount);
  VolumeDestroy(B);
  fifoq_destroy(fifo);
  adjacency_destroy(a6);

  return seg;
}
