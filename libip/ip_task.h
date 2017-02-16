
#ifndef TASK_H
#define TASK_H 1

#include <time.h>
#include <pthread.h>

#include "ip_common.h"
#include "ip_adjacency.h"
#include "ip_volume.h"

#define S_WAIT 0
#define S_RUN  1
#define S_OVER 2
#define S_NEW  3 /* task queue state only */

typedef void (*Callback) (void *);


/* THE TASK LOG */

typedef struct _task_log_event {
  int    tid;           /* thread id of thread that ran it */
  char   desc[128];     /* textual description of task     */
  double real_elapsed;  /* elapsed time (seconds)          */
  time_t start, finish; /* start/finish times as UNIX time */
} TaskLogEvent;

typedef struct _task_log_node {
  TaskLogEvent task;
  struct _task_log_node *next;
} TaskLogNode;

typedef struct _task_log {
  TaskLogNode     *head;
  TaskLogNode     *tail;
  int             N;
  pthread_mutex_t logmutex;
} TaskLog;

TaskLog *TaskLogNew();
void     TaskLogDestroy(TaskLog *tl);
void     TaskLogAdd(TaskLog *tl,char *desc, double seconds, 
		    time_t start, time_t finish);
int      TaskLogGetCount(TaskLog *tl);
void     TaskLogSave(TaskLog *tl, char *name);
void     TaskLogLock(TaskLog *tl);
void     TaskLogUnlock(TaskLog *tl);

/* THE TASK STRUCTURES */

typedef struct _task {
  char           desc[128];
  double         seconds;  
  int            state;
  int            done;
  Callback       call;
  void          *args;
  struct timeval start;
} Task;

/* current maximum task queue length */
#define TQLEN 512

typedef struct _taskqueue {
  int             q0;
  int             n;
  int             hn;
  Task            tasks[TQLEN];
  pthread_t       tid;
  pthread_mutex_t tasklock;
  int             state;
  int             gone_fishing;
  TaskLog        *logp;
} TaskQueue;

TaskQueue *TaskQueueNew();
void       TaskQueueDestroy(TaskQueue *tq);
void       TaskQueueSetLog(TaskQueue *tq, TaskLog *tl);
void       TaskQueueLock(TaskQueue *tq);
void       TaskQueueUnlock(TaskQueue *tq);

/* call these within a Lock() / Unlock() pair */
int   TaskNew(TaskQueue *tq, char *desc, Callback func, void *args);
int   TaskState(TaskQueue *tq, int taskid);
int   GetTaskCount(TaskQueue *tq);
int   GetTaskDoneCount(TaskQueue *tq);
Task *GetNthTask(TaskQueue *tq,int n);
/* */

/*
  runs the given queue in a new POSIX thread
*/
void  StartThread(TaskQueue *tq);

/* 
   safe=1: waits current running task (if any) to finish
   safe=0: kills thread immediately, any data structures the
           current task is fiddling with are likely to be
           corrupted
*/
void  StopThread(TaskQueue *tq, int safe);


void SetProgress(int current,int total);
int  GetProgress();
void EndProgress();

void  SubTaskInit(int n);
void  SubTaskSetText(int i,char *s);
void  SubTaskSet(int i);
void  SubTaskDestroy();
char *SubTaskGet(int *current,int *total); // returns a strdup, free it on your own
void  SubTaskLock();
void  SubTaskUnlock();

#endif
