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

/* $Id: iftalgo.c,v 1.15 2013/11/11 22:34:13 bergo Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libip.h"

#define DISCMAP_SRC_ELEM(m,a,b,c) m[((a)<<5)+(3*(b))+(c)]
#define DISCMAP_DST_ELEM(m,a,b,c) m[((a)<<5)+9+(3*(b))+(c)]

#define DISCMAP_SRC_ELEM_LIN(m,a,b) m[((a)<<5)+(b)]
#define DISCMAP_DST_ELEM_LIN(m,a,b) m[((a)<<5)+9+(b)]

void ComputeDiscMap(int *discmap, AdjRel *a, int W, int H, int D) {
  int i,j,k,dx,dy,dz;

  for(i=0;i<a->n;i++) {
    dx = a->dx[i];
    dy = a->dy[i];
    dz = a->dz[i];

    if (dz!=0) {
      for(k=-1;k<=1;k++)
	for(j=-1;j<=1;j++) {
	  DISCMAP_SRC_ELEM(discmap,i,k+1,j+1) = j + W*k;
	  DISCMAP_DST_ELEM(discmap,i,k+1,j+1) = j + W*k + W*H*dz;
	}
    } else if (dy!=0) {
      for(k=-1;k<=1;k++)
	for(j=-1;j<=1;j++) {
	  DISCMAP_SRC_ELEM(discmap,i,k+1,j+1) = j + W*H*k;
	  DISCMAP_DST_ELEM(discmap,i,k+1,j+1) = j + W*H*k + W*dy;
	}
    } else if (dx!=0) {
      for(k=-1;k<=1;k++)
	for(j=-1;j<=1;j++) {
	  DISCMAP_SRC_ELEM(discmap,i,k+1,j+1) = W*H*k + W*j;
	  DISCMAP_DST_ELEM(discmap,i,k+1,j+1) = dx + W*H*k + W*j;
	}
    }
  }
}

void DiscShedCore(XAnnVolume *vol, int gradmode) {
  Queue *Q;
  unsigned int i;
  int x,w;
  short int c1,c2;
  unsigned char adjcode;
  char wcolor;
  AdjRel *a;
  evoxel *vd = vol->vd;
  int mcost = SDOM, W = vol->W, H = vol->H, D = vol->D, N=vol->N;

  int *discmap;
  int discweight[16], dw;

  unsigned int l1;
  int n1,n2;
  int grad,gt;

  Voxel p,q;

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  switch(gradmode) {
  case 0:
    discweight[0]=1;  discweight[1]=2;  discweight[2]=1;
    discweight[3]=2;  discweight[4]=4;  discweight[5]=2;
    discweight[6]=1;  discweight[7]=2;  discweight[8]=1;
    dw = 16;
    break;
  case 1:
  default:
    discweight[0]=1;  discweight[1]=3;  discweight[2]=1;
    discweight[3]=3;  discweight[4]=9;  discweight[5]=3;
    discweight[6]=1;  discweight[7]=3;  discweight[8]=1;
    dw = 25;
    break;
  }

  /* compute discmap lookup */

  discmap = (int *) MemAlloc(sizeof(int) * 32 * a->n);
  MemSet(discmap,0,sizeof(int) * 32 * a->n);
  ComputeDiscMap(discmap, a, W, H, D);

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vd[i].cost = 0;
    vd[i].pred = 0;
    vd[i].label = (unsigned char) x;
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vd[i].cost, i);
    if (vd[i].cost < mcost)
      mcost = vd[i].cost;
    vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0)
    SetQueueCurrentBucket(Q, mcost);

  while( (x=RemoveQueue(Q)) != NIL ) {

    XAVolumeVoxelOf(vol,x,&p);
  
    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.y < 0 || q.z < 0 ||
	  q.x >= W || q.y >= H || q.z >= D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];
      wcolor = Q->L.elem[w].color;

      if (wcolor != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c2 = vd[w].cost; // save old cost for update

	/* online gradient calc */

	grad = 0;
	for(l1=0;l1<9;l1++) {
	  n1 = x + DISCMAP_SRC_ELEM_LIN(discmap,i,l1);
	  n2 = x + DISCMAP_DST_ELEM_LIN(discmap,i,l1);
	  if (n1 <0 || n2<0 || n1>=N || n2>=N) continue;
	  gt = discweight[l1] * (vd[n2].value - vd[n1].value);
	  grad += ((gt < 0) ? -gt : gt);
	}
	c1 = (short int) (grad / dw);
	c1 = MAX(vd[x].cost, c1);

	if (c1 < c2 || vd[w].pred == adjcode) {

	  vd[w].label = vd[x].label;
	  vd[w].cost  = c1;
	  vd[w].pred  = adjcode;

	  if (wcolor == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }

	}

      } // != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
  MemFree(discmap);
}

void LazyShedL1Core(XAnnVolume *vol) {
  Queue *Q;
  unsigned int i;
  int x,w;
  short int c1,c2;
  unsigned char adjcode;
  char wcolor;
  AdjRel *a;
  evoxel *vd = vol->vd;
  int mcost = SDOM, W = vol->W, H = vol->H, D = vol->D;

  Voxel p,q;

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vd[i].cost = 0;
    vd[i].pred = 0;
    vd[i].label = (unsigned char) x;
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vd[i].cost, i);
    if (vd[i].cost < mcost)
      mcost = vd[i].cost;
    vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0)
    SetQueueCurrentBucket(Q, mcost);

  while( (x=RemoveQueue(Q)) != NIL ) {

    XAVolumeVoxelOf(vol,x,&p);
  
    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.y < 0 || q.z < 0 ||
	  q.x >= W || q.y >= H || q.z >= D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];
      wcolor = Q->L.elem[w].color;

      if (wcolor != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c2 = vd[w].cost; // save old cost for update
	c1 = MIN(1 + MAX(vd[x].cost, vd[w].value), 32767);

	if (c1 < c2 || vd[w].pred == adjcode) {

	  vd[w].label = vd[x].label;
	  vd[w].cost  = c1;
	  vd[w].pred  = adjcode;

	  if (wcolor == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }

	}

      } // != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
}

void WaterShedM(XAnnVolume *vol) {
  Queue *Q;
  unsigned int i;
  int x,w;
  short int c1,c2;
  unsigned char adjcode;
  char wcolor;
  AdjRel *a;
  evoxel *vd = vol->vd;
  int mcost = SDOM, W = vol->W, H = vol->H, D = vol->D;

  Voxel p,q;

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vd[i].cost = 0;
    vd[i].pred = 0;
    vd[i].label = (unsigned char) x;
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vd[i].cost, i);
    if (vd[i].cost < mcost)
      mcost = vd[i].cost;
    vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0)
    SetQueueCurrentBucket(Q, mcost);

  while( (x=RemoveQueue(Q)) != NIL ) {

    XAVolumeVoxelOf(vol,x,&p);
  
    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.y < 0 || q.z < 0 ||
	  q.x >= W || q.y >= H || q.z >= D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];
      wcolor = Q->L.elem[w].color;

      if (wcolor != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c2 = vd[w].cost; // save old cost for update
	c1 = MAX(vd[x].cost, vd[w].value);

	if (c1 < c2 || vd[w].pred == adjcode) {

	  vd[w].label = vd[x].label;
	  vd[w].cost  = c1;
	  vd[w].pred  = adjcode;

	  if (wcolor == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }

	}

      } // != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
}

void WaterShedP(XAnnVolume *vol) {
  FQueue *Q;
  unsigned int i;
  int x,w;
  short int c0,c1,tmp;
  unsigned char adjcode;
  AdjRel *a;

  Voxel p,q;

  Q=CreateFQueue(SDOM, vol->N);
  a=vol->predadj;

  // these are user-selected addition seeds
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertFQueue(Q, 0, i);
    vol->vd[i].cost = 0;
    vol->vd[i].pred = 0;
    vol->vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->addseeds);

  // these are removed/non-removed borders detected by
  // ClearTrees. delseeds and addseeds are disjoint (guaranteed by
  // by ClearTrees)
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertFQueue(Q, 0, i);
    vol->vd[i].cost = 0;
    vol->vd[i].pred = 0;
    vol->vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  while( (x=RemoveFQueue(Q)) != NIL ) {

    c0 = vol->vd[x].cost;

    XAVolumeVoxelOf(vol,x,&p);
  
    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.x >= vol->W || q.y < 0 || q.y >= vol->H ||
	  q.z < 0 || q.z >= vol->D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];

      adjcode = AdjEncodeDN(a,w,x);

      c1 = vol->vd[w].cost;
      if ( c0 < c1 || vol->vd[w].pred == adjcode ) {
	tmp=MAX(c0,(short int) vol->vd[w].value);
	  
	if (tmp < c1 || vol->vd[w].pred == adjcode) {

	  vol->vd[w].cost  = tmp;
	  vol->vd[w].label = vol->vd[x].label;
	  vol->vd[w].pred  = adjcode;
	  InsertFQueue(Q,tmp,w);
	} 
      }
    }

  }

  DestroyFQueue(&Q);
}

/* 
   |= REMOVEMASK makes the visualization render voxels
   scheduled for removal in a different color. Even though
   it is not needed by the algorithm, setting the high bit
   (|= REMOVEMASK) makes it easy to spot algorithm errors
   in the visualization, because all removed voxels must be
   reconquered by "clean" ancestors
*/

// on output, vol->addseeds will contain the user-selected addition seeds
// and vol->delseeds will contain the seeds in the removed/non-removed
// border. This was changed for the implementation of the IFT- , but
// the IFT+ code has been changed to work correctly with the new situation
void ClearTrees(XAnnVolume *vol) {
  int *fifo;
  int fifosz, getpt, putpt;
  int i,j,x,y;

  unsigned char *newseeds;

  fifosz = vol->N;
  fifo=(int *)MemAlloc(sizeof(int) * fifosz);
  getpt = 0;
  putpt = 0;

  /* prepare tree roots and insert them in the FIFO */

  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &j);
    fifo[putpt]=i;
    vol->vd[i].label |= REMOVEMASK;

    if (vol->vd[i].pred != 0)
      printf("cleartrees: seed %d isn't root ?!?\n",i);
    vol->vd[i].pred   = 0;
    vol->vd[i].cost   = HIGH_S16;
    ++putpt;
  }
  MapShrink(vol->delseeds);

  /* propagate fifo to find children */
  while(getpt!=putpt) {
    x=fifo[getpt];
    getpt = (getpt + 1) % fifosz;

    for(i=1;i<vol->predadj->n;i++) {
      j= x + vol->predadj->dn[i];
      if (j<0 || j>= vol->N)
	continue;
      y = AdjDecodeDN(vol->predadj, j, vol->vd[j].pred);
      if (y==x) {
	vol->vd[j].label |= REMOVEMASK;
	vol->vd[j].pred   = 0;
	vol->vd[j].cost   = HIGH_S16;
	fifo[putpt] = j;
	putpt = (putpt + 1) % fifosz;
      }          
    }
  }

  MemFree(fifo);

  /* now do a raster pass over the volume inserting removed/non-removed
     borders into the seed set */

  newseeds = (unsigned char *) MemAlloc(vol->N * sizeof(unsigned char));
  MemSet(newseeds, 255, vol->N);

  for(i=0;i<vol->N;i++) {

    /* unconquered (removed) voxels are skipped */
    if (vol->vd[i].pred == 0)
      continue;

    for(j=1;j<vol->predadj->n;j++) {
      x = i + vol->predadj->dn[j];
      if (x<0 || x>= vol->N)
	continue;
      
      /* if this is a valid voxel that has an unconquered 
	 neighbour voxel, add it as seed */
      if (vol->vd[x].label & REMOVEMASK) {
	newseeds[i] = vol->vd[i].label;
	break;
      }
    } // for j (neighbours)
  } // for i (raster pass)

  // unmark removal seeds if they're user addition seeds too
  j = MapSize(vol->addseeds);
  for(i=0;i<j;i++) {
    MapGet(vol->addseeds,i,&x,&y);
    newseeds[i] = 255;
    vol->vd[i].label &= LABELMASK;
  }

  for(i=0;i<vol->N;i++)
    if (newseeds[i] != 255)
      MapSetUnique(vol->delseeds, i, newseeds[i]);
  MemFree(newseeds);
}


void WaterShedPlus(XAnnVolume *vol) {
  ClearTrees(vol);
  WaterShedP(vol);
}

void WaterShedMinus(XAnnVolume *vol) {
  ClearTrees(vol);
  WaterShedM(vol);
}

void LazyShedL1(XAnnVolume *vol) {
  ClearTrees(vol);
  LazyShedL1Core(vol);
}

void DiscShed(XAnnVolume *vol, int gradmode) {
  ClearTrees(vol);
  DiscShedCore(vol, gradmode);
}

/* FUZZY CONNECTEDNESS */

void DFuzzyMain(XAnnVolume *vol, int nobjs, float *objdata);
char FuzzyCloser(short int v, float *mean, float *var, int nobjs);


void DFuzzyConn(XAnnVolume *vol, int nobjs, float *objdata) {
  /*
  if (vol->extra == 0) {
    vol->extra = MemAlloc(vol->N);
    vol->extra_is_freeable = 1;
    if (!vol->extra)
      fprintf(stderr,"** not enough memory to allocate fuzzyconn block.\n");
  }
  */

  ClearTrees(vol);
  DFuzzyMain(vol, nobjs, objdata);
}

short int FuzzyCost(short int v, float mean, float var,float topval) {
  float x;

  x = (((float)v)-mean);
  x = x * x;
  x = topval * (1.0 - exp(-x/(2*var)));
  return((short int) x);
}

/* private function, returns index of best fit object for value v */
char FuzzyCloser(short int v, float *mean, float *var, int nobjs) {
  int i,besti=0;
  short int c, best=32767;

  for(i=0;i<nobjs;i++) {
    c = FuzzyCost(v, mean[i], var[i], 32000.0);
    if (c < best) {
      best  = c;
      besti = i;
    }
  }
  return((char)(besti));
}

/*
   the affinity of a value v to a set (m,s) is given by

   exp[ - (v-m)^2 / 2*(s^2) ] in the [0,1] interval
*/

void DFuzzyMain(XAnnVolume *vol, int nobjs, float *objdata) {
  Queue *Q;
  unsigned int i,j;
  int x,w;
  short int c0,c1,c2;
  unsigned char adjcode;
  AdjRel *a;
  int mcost = SDOM;
  fob f;

  Voxel p,q;

  f.n = nobjs;
  for(i=0;i<f.n;i++) {
    f.mean[i] = objdata[2*i];
    f.stddev[i] = objdata[2*i+1];
    f.var[i] = f.stddev[i] * f.stddev[i];
  }

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vol->vd[i].cost = 0;
    vol->vd[i].pred = 0;
    vol->vd[i].label = (unsigned char) x;
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vol->vd[i].cost, i);
    if (vol->vd[i].cost < mcost)
      mcost = vol->vd[i].cost;
    vol->vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0) {
    SetQueueCurrentBucket(Q, mcost);
    //    printf("mcost is %d\n",mcost);
  }

  while( (x=RemoveQueue(Q)) != NIL ) {

    c0 = vol->vd[x].cost;

    XAVolumeVoxelOf(vol,x,&p);

    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.x >= vol->W || q.y < 0 || q.y >= vol->H ||
	  q.z < 0 || q.z >= vol->D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];

      if (Q->L.elem[w].color != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c1 = 32767;
	for(j=0;j<f.n;j++) {
	  c2 = FuzzyCost((vol->vd[w].value + vol->vd[x].value)/2,
			 f.mean[j],
			 f.var[j],
			 32000.0);
	  if (c2 < c1)
	    c1 = c2;
	}
	c1 = MAX(c1, c0); /* c1 is the max between best affinity and current
			     cost of x */

	if (vol->vd[w].pred == adjcode || c1 < vol->vd[w].cost) {

	  c2 = vol->vd[w].cost; /* save old cost for queue update */
	  vol->vd[w].cost  = c1;
	  vol->vd[w].label = vol->vd[x].label;
	  vol->vd[w].pred  = adjcode;
	      
	  if (Q->L.elem[w].color == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }
	} // if predtest true or cost offered is less than current cost
	  
      } // if != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
}

void DMTMain(XAnnVolume *vol);

void DeltaMinimization(XAnnVolume *vol) {
  if (vol->extra == 0) {
    vol->extra = MemAlloc(sizeof(short int) * 2 * vol->N);
    vol->extra_is_freeable = 1;
    if (!vol->extra)
      fprintf(stderr,"** not enough memory to allocate DMT block.\n");
  }
  
  ClearTrees(vol);
  DMTMain(vol);
}

void DMTMain(XAnnVolume *vol) {
  Queue *Q;
  unsigned int i;
  int x,w;
  short int c0,c1,c2,xmin,xmax,wmin,wmax;
  unsigned char adjcode;
  AdjRel *a;
  int mcost = SDOM;
  short int *edata = (short int *) vol->extra;

  Voxel p,q;

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vol->vd[i].cost = 0;
    vol->vd[i].pred = 0;
    vol->vd[i].label = (unsigned char) x;

    edata[2*i]   = vol->vd[i].value;
    edata[2*i+1] = vol->vd[i].value;

    // printf("seed %d, value=%d, closer=%d\n",i,vol->vd[i].value,objmap[i]);
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vol->vd[i].cost, i);
    if (vol->vd[i].cost < mcost)
      mcost = vol->vd[i].cost;
    vol->vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0) {
    SetQueueCurrentBucket(Q, mcost);
    //    printf("mcost is %d\n",mcost);
  }

  while( (x=RemoveQueue(Q)) != NIL ) {

    c0 = vol->vd[x].cost;

    XAVolumeVoxelOf(vol,x,&p);

    xmin = edata[2*x];
    xmax = edata[2*x+1];

    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.x >= vol->W || q.y < 0 || q.y >= vol->H ||
	  q.z < 0 || q.z >= vol->D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];

      if (Q->L.elem[w].color != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	wmax = MAX(xmax, vol->vd[w].value);
	wmin = MIN(xmin, vol->vd[w].value);

	c1 = wmax - wmin;
	if (c0 > c1) c1 = c0; // c1 = MAX(c1,c0)

	if (vol->vd[w].pred == adjcode || c1 < vol->vd[w].cost) {

	  c2 = vol->vd[w].cost; /* save old cost for queue update */
	  vol->vd[w].cost  = c1;
	  vol->vd[w].label = vol->vd[x].label;
	  vol->vd[w].pred  = adjcode;

	  edata[2*w]   = wmin;
	  edata[2*w+1] = wmax;
	      
	  if (Q->L.elem[w].color == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }
	} // if predtest true or cost offered is less than current cost
	  
      } // if != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
}

/* STRICT FUZZY CONNECTEDNESS: add penalty to go thru
   an edge the fits better other object than this one */

void DStrictFuzzyMain(XAnnVolume *vol, int nobjs, float *objdata);

void DStrictFuzzyConn(XAnnVolume *vol, int nobjs, float *objdata) {
  if (vol->extra == 0) {
    vol->extra = MemAlloc(vol->N);
    vol->extra_is_freeable = 1;
    if (!vol->extra)
      fprintf(stderr,"** not enough memory to allocate fuzzyconn block.\n");
  }

  ClearTrees(vol);
  DStrictFuzzyMain(vol, nobjs, objdata);
}

void DStrictFuzzyMain(XAnnVolume *vol, int nobjs, float *objdata) {
  Queue *Q;
  unsigned int i,j;
  int x,w;
  short int c0,c1,c2;
  unsigned char adjcode;
  AdjRel *a;
  int mcost = SDOM;
  fob f;
  char *objmap = (char *) vol->extra;

  Voxel p,q;

  f.n = nobjs;
  for(i=0;i<f.n;i++) {
    f.mean[i] = objdata[2*i];
    f.stddev[i] = objdata[2*i+1];
    f.var[i] = f.stddev[i] * f.stddev[i];
  }

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  // user-added seeds (inserted as roots, with cost 0)
  while(MapSize(vol->addseeds)) {
    MapRemoveAny(vol->addseeds, (int *) &i, &x);
    InsertQueue(Q, 0, i);
    vol->vd[i].cost = 0;
    vol->vd[i].pred = 0;
    vol->vd[i].label = (unsigned char) x;
    objmap[i] = FuzzyCloser(vol->vd[i].value, f.mean, f.var, f.n);
    // printf("seed %d, value=%d, closer=%d\n",i,vol->vd[i].value,objmap[i]);
    mcost = 0;
  }
  MapShrink(vol->addseeds);

  // seeds in the removed/non-removed border
  while(MapSize(vol->delseeds)) {
    MapRemoveAny(vol->delseeds, (int *) &i, &x);
    InsertQueue(Q, vol->vd[i].cost, i);
    if (vol->vd[i].cost < mcost)
      mcost = vol->vd[i].cost;
    vol->vd[i].label = (unsigned char) x;
  }
  MapShrink(vol->delseeds);

  if (mcost != 0) {
    SetQueueCurrentBucket(Q, mcost);
    //    printf("mcost is %d\n",mcost);
  }

  while( (x=RemoveQueue(Q)) != NIL ) {

    c0 = vol->vd[x].cost;

    XAVolumeVoxelOf(vol,x,&p);

    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.x >= vol->W || q.y < 0 || q.y >= vol->H ||
	  q.z < 0 || q.z >= vol->D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];

      if (Q->L.elem[w].color != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c1 = 32767;
	for(j=0;j<f.n;j++) {
	  c2 = FuzzyCost((vol->vd[w].value + vol->vd[x].value)/2,
			 f.mean[j],
			 f.var[j],
			 32000.0);
	  if (j != objmap[x]) { /* penalty for changing objects */
	    c2 += 8000;
	    if (c2 < 0) c2 = 32767; /* overflow, c2 goes negative */
	  }
	  if (c2 < c1)
	    c1 = c2;
	}
	c1 = MAX(c1, c0); /* c1 is the max between best affinity and current
			     cost of x */

	if (vol->vd[w].pred == adjcode || c1 < vol->vd[w].cost) {

	  c2 = vol->vd[w].cost; /* save old cost for queue update */
	  vol->vd[w].cost  = c1;
	  vol->vd[w].label = vol->vd[x].label;
	  vol->vd[w].pred  = adjcode;
	  objmap[w] = objmap[x];
	      
	  if (Q->L.elem[w].color == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }
	} // if predtest true or cost offered is less than current cost
	  
      } // if != BLACK
      
    } // for i
    
  } // while RemoveQueue

  DestroyQueue(&Q);
}

void KappaSeg(XAnnVolume *vol) {
  Queue *Q;
  unsigned int i;
  int x,w;
  short int c1,c2;
  unsigned char adjcode;
  char wcolor;
  AdjRel *a;
  evoxel *vd = vol->vd;
  int mcost = SDOM, W = vol->W, H = vol->H, D = vol->D;

  Voxel p,q;

  int imax, mean;
  i16_t *lookup;
  float fa,fb,fc;
  
  imax = 0;
  for(i=0;i<vol->N;i++)
    if (vol->vd[i].value > imax) imax = vol->vd[i].value;

  Q=CreateQueue(SDOM, vol->N);
  a=vol->predadj;

  MapRemoveAny(vol->addseeds, (int *) &i, &x);
  mean = vol->vd[i].value;
  InsertQueue(Q,0,i);
  vd[i].cost = 0;
  vd[i].pred = 0;
  vd[i].label = (unsigned char) x;
  mcost = 0;

  MapClear(vol->addseeds);
  MapClear(vol->delseeds);

  lookup = (i16_t *) MemAlloc(2*32768);
  fa = (float) mean;
  fb = (float) (20*imax);
  for(i=0;i<32768;i++) {
    fc = ((float)(i)) - fa;
    fc = 1.0-exp((-fc*fc) / fb);
    lookup[i] = (i16_t) (32767.0 * fc);
  }
  
  if (mcost != 0)
    SetQueueCurrentBucket(Q, mcost);

  while( (x=RemoveQueue(Q)) != NIL ) {

    XAVolumeVoxelOf(vol,x,&p);
  
    for(i=1;i<a->n;i++) {
      q.x = p.x + a->dx[i];
      q.y = p.y + a->dy[i];
      q.z = p.z + a->dz[i];

      if (q.x < 0 || q.y < 0 || q.z < 0 ||
	  q.x >= W || q.y >= H || q.z >= D)
	continue;

      w=q.x + vol->tbrow[q.y] + vol->tbframe[q.z];
      wcolor = Q->L.elem[w].color;

      if (wcolor != BLACK) {

	adjcode = AdjEncodeDN(a,w,x);

	c2 = vd[w].cost; // save old cost for update
	c1 = MAX(vd[x].cost, lookup[vd[w].value]);

	if (c1 < c2 || vd[w].pred == adjcode) {

	  vd[w].label = vd[x].label;
	  vd[w].cost  = c1;
	  vd[w].pred  = adjcode;

	  if (wcolor == GRAY) {
	    UpdateQueue(Q,w,c2,c1);
	  } else {
	    InsertQueue(Q,c1,w);
	  }

	}

      } // != BLACK
      
    } // for i
    
  } // while RemoveQueue

  MemFree(lookup);
  DestroyQueue(&Q);
}
