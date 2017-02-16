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
 $Id: stack.c,v 1.5 2013/11/11 22:34:13 bergo Exp $

 stack.c : integer stack data structure, fifo queue
           of integers

*****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libip.h"

Stack * StackNew(int n) {
  Stack *s;

  s = (Stack *) MemAlloc(sizeof(Stack));
  if (!s) return 0;

  s->n   = n;
  s->top = -1;
  s->data = (int *) MemAlloc(sizeof(int) * n);
  if (!s->data) {
    MemFree(s);
    return 0;
  }
  return s;
}

void    StackDestroy(Stack *s) {
  if (s) {
    if (s->data) MemFree(s->data);
    MemFree(s);
  }
}

void    StackPush(Stack *s, int n) {
  s->data[++(s->top)] = n;
}

int     StackPop(Stack *s) {
  if (s->top == -1) return -1;
  return(s->data[s->top--]);
}

int     StackEmpty(Stack *s) {
  return(s->top == -1);
}

FIFOQ * FIFOQNew(int n) {
  FIFOQ *q;

  q = (FIFOQ *) MemAlloc(sizeof(FIFOQ));
  if (!q) return 0;

  q->n   = n;
  q->put = q->get = 0;
  q->data = (int *) MemAlloc(sizeof(int) * n);
  if (!q->data) {
    MemFree(q);
    return 0;
  }
  return q;
}

void    FIFOQDestroy(FIFOQ *q) {
  if (q) {
    if (q->data) MemFree(q->data);
    MemFree(q);
  }  
}

void    FIFOQPush(FIFOQ *q, int n) {
  q->data[q->put] = n;
  q->put = (q->put + 1) % q->n;
}

int     FIFOQPop(FIFOQ *q) {
  int v;
  if (q->get == q->put) return -1;
  v = q->data[q->get];
  q->get = (q->get + 1) % q->n;
  return v;
}

int     FIFOQEmpty(FIFOQ *q) {
  return(q->get == q->put);
}
