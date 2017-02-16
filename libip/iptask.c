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

/*********************************************************
 $Id: iptask.c,v 1.8 2013/11/11 22:34:13 bergo Exp $

 task.c - implementation of task queue on a POSIX thread

**********************************************************/

#include "ip_task.h"
#include "ip_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

void *ThreadMain(void *p);

volatile struct libip_task_subtask {
  int  im,n,current;
  char **ptr;
  pthread_mutex_t lock;
} subtask = { .im=0, .n=0 , .current = 0, .ptr = 0};

TaskQueue *TaskQueueNew() {
  TaskQueue *tq;
  
  tq = (TaskQueue *) MemAlloc(sizeof(TaskQueue));
  if (!tq) return 0;

  tq->q0           = 0;
  tq->n            = 0;
  tq->hn           = 0;
  tq->tid          = 0;
  tq->gone_fishing = 0;
  tq->state        = S_NEW;
  tq->logp         = 0;
  pthread_mutex_init(&(tq->tasklock), NULL);
  return(tq);
}

void TaskQueueDestroy(TaskQueue *tq) {
  if (tq!=0)
    MemFree(tq);
}

void TaskQueueSetLog(TaskQueue *tq, TaskLog *tl) {
  TaskQueueLock(tq);
  tq->logp = tl;
  TaskQueueUnlock(tq);
}

int TaskNew(TaskQueue *tq, char *desc, Callback func, void *args) {
  Task *x;
  int id;

  pthread_mutex_lock(&(tq->tasklock));

  if (tq->gone_fishing) return -1; /* thread is dying */
  if (tq->n==TQLEN) return -1;     /* thread can't take any more */

  x=& (tq->tasks[id=((tq->q0+tq->n)%TQLEN)]);

  strncpy(x->desc,desc,128);
  x->desc[127]=0;
  x->call  = func;
  x->args  = args;
  x->state = S_WAIT;
  x->done  = 0;

  tq->n++;

  pthread_mutex_unlock(&(tq->tasklock));
  return id;
}

int TaskState(TaskQueue *tq,int taskid) {
  return(tq->tasks[taskid].state);
}

int  GetTaskCount(TaskQueue *tq) {
  return tq->n;
}

int  GetTaskDoneCount(TaskQueue *tq) {
  return tq->hn;
}

Task *GetNthTask(TaskQueue *tq,int n) {
  return(& (tq->tasks[(tq->q0+n)%TQLEN]) );
}

void  StartThread(TaskQueue *tq) {
  if (tq->tid == 0)
    pthread_create(&(tq->tid),NULL,&ThreadMain,(void *) tq);
}

void  StopThread(TaskQueue *tq, int safe) {
  if (tq->tid) {
    if (safe) {
      TaskQueueLock(tq);
      tq->gone_fishing = 1;
      if (tq->state == S_RUN) tq->n = 1; else tq->n = 0;
      TaskQueueUnlock(tq);
      while(tq->state == S_RUN) usleep(1000);
    }      
    pthread_kill(tq->tid,SIGKILL);
    tq->tid = 0;
  }
}

void *ThreadMain(void *p) {
  TaskQueue *tq = (TaskQueue *) p;
  Task *x;
  FullTimer T;

  TaskQueueLock(tq);
  tq->state = S_WAIT;
  TaskQueueUnlock(tq);

  for(;;) {
    if (!tq->n) {
      usleep(50000);
      continue;
    }

    TaskQueueLock(tq);

    StartTimer(&T);
    x=GetNthTask(tq,0);
    gettimeofday(&(x->start),0);
    x->state = S_RUN;
    tq->state = S_RUN;

    TaskQueueUnlock(tq);

    if (subtask.im)
      SubTaskDestroy();
    (*(x->call))(x->args);

    TaskQueueLock(tq);

    StopTimer(&T);
    x->state=S_OVER;
    x->seconds=TimeElapsed(&T);
    x->done=1;
    tq->n--;
    tq->q0++;
    tq->q0 %= TQLEN;
    tq->hn++;

    tq->state = S_WAIT;
    
    TaskQueueUnlock(tq);

    if (tq->logp)
      TaskLogAdd(tq->logp,x->desc, x->seconds, T.S0, T.S1);
  }
  return NULL;
}

void TaskQueueLock(TaskQueue *tq) {
  pthread_mutex_lock(&(tq->tasklock));
}

void TaskQueueUnlock(TaskQueue *tq) {
  pthread_mutex_unlock(&(tq->tasklock));
}

TaskLog *TaskLogNew() {
  TaskLog *tl;
  
  tl = (TaskLog *) MemAlloc(sizeof(TaskLog));
  if (!tl) return 0;

  tl->N = 0;
  tl->head = tl->tail = 0;
  pthread_mutex_init(&(tl->logmutex), NULL);
  return tl;
}

/* this is private, you have no business calling this on
   your own */
void __DestroyLogNodeList(TaskLogNode *x) {
  if (!x) return;
  if (x->next)
    __DestroyLogNodeList(x->next);
  MemFree(x);
}

void TaskLogDestroy(TaskLog *tl) {
  if (tl) {
    if (tl->head)
      __DestroyLogNodeList(tl->head);
    MemFree(tl);
  }
}

void TaskLogAdd(TaskLog *tl,char *desc, double seconds, 
		time_t start, time_t finish)
{
  TaskLogNode *x;
  
  x=(TaskLogNode *) MemAlloc(sizeof(TaskLogNode));
  if (!x) return;

  TaskLogLock(tl);
  
  strcpy(x->task.desc, desc);
  x->task.tid = pthread_self();
  x->task.real_elapsed = seconds;
  x->task.start        = start;
  x->task.finish       = finish;
  x->next = 0;

  if (tl->head == 0) {
    tl->head = x;
    tl->tail = x;
  }  else {
    tl->tail->next = x;
    tl->tail = x;
  }  
 
  tl->N++;
  TaskLogUnlock(tl);
}

int      TaskLogGetCount(TaskLog *tl) {
  return(tl->N);
}

void     TaskLogSave(TaskLog *tl, char *name) {
  FILE *f;
  TaskLogNode *p;
  f=fopen(name,"w");
  if (!f)
    return;

  fprintf(f,"%-40s  %-4s  %-10s  %-10s\n",
	  "Description","T-Id","Seconds","Start/End");
  TaskLogLock(tl);
  for(p=tl->head;p!=0;p=p->next) {
    fprintf(f,"%-40s  %-4d  %-10.6f  %-8d->%-8d\n",
	    p->task.desc,
	    p->task.tid,
	    p->task.real_elapsed,
	    (int) p->task.start,
	    (int) p->task.finish);
  }
  TaskLogUnlock(tl);
  fclose(f);
}

void TaskLogLock(TaskLog *tl) {
  pthread_mutex_lock(&(tl->logmutex));
}

void TaskLogUnlock(TaskLog *tl) {
  pthread_mutex_unlock(&(tl->logmutex));
}

struct libip_task_progress {
  int active;
  int current;
  int total;
};

volatile struct libip_task_progress prog = {0,0,0};

void SetProgress(int current,int total) {
  prog.active = 1;
  prog.current = current;
  prog.total = total;

  if (current > total) {
    printf("warning: current=%d total=%d\n",current,total);
  }
}

int  GetProgress() {
  if (prog.active) {
    return( (100*prog.current) / prog.total );
  } else
    return -1;
}

void EndProgress() {
  prog.active = 0;
}

void  SubTaskLock() {
  pthread_mutex_lock((pthread_mutex_t *) &(subtask.lock));
}

void  SubTaskUnlock() {
  pthread_mutex_unlock((pthread_mutex_t *) &(subtask.lock));
}

void  SubTaskInit(int n) {
  int i;
  if (!subtask.im) {
    pthread_mutex_init((pthread_mutex_t *) &(subtask.lock),NULL);
    subtask.im = 1;
  }
  if (subtask.current)
    SubTaskDestroy();

  SubTaskLock();
  subtask.n       = n;
  subtask.current = 0;
  subtask.ptr = (char **) MemAlloc(sizeof(char *) * n);
  for(i=0;i<n;i++)
    subtask.ptr[i] = 0;
  SubTaskUnlock();
}

void  SubTaskSetText(int i,char *s) {
  SubTaskLock();
  if (!subtask.ptr) return;
  subtask.ptr[i%(subtask.n)] = strdup(s);
  SubTaskUnlock();
}

void  SubTaskSet(int i) {
  SubTaskLock();
  if (i >= subtask.n) i = subtask.n - 1;
  subtask.current = i;
  SubTaskUnlock();
}

void  SubTaskDestroy() {
  int i;
  SubTaskLock();
  if (subtask.ptr) {
    for(i=0;i<subtask.n;i++) {
      if (subtask.ptr[i])
	free(subtask.ptr[i]);
    }
    MemFree(subtask.ptr);
    subtask.ptr = 0;
    subtask.n = 0;
  }
  SubTaskUnlock();
}

char *SubTaskGet(int *current,int *total) {
  char *s=0;
  SubTaskLock();
  if (subtask.ptr)
    if (subtask.ptr[subtask.current]) {
      s = strdup(subtask.ptr[subtask.current]);
      if (current != 0) *current = subtask.current + 1;
      if (total != 0) *total = subtask.n;
    }
  SubTaskUnlock();
  return s;
}
