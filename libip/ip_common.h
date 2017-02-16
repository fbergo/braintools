#ifndef _COMMON_H_
#define _COMMON_H_

#include <time.h>
#include <sys/time.h>

/* Error messages */

#define MSG1  "Cannot allocate memory space in"
#define MSG2  "Cannot open file in"

/* Common data types to all programs */ 

#ifndef __cplusplus
typedef enum{false,true} bool; 
#endif

/* Common definitions */

#define PI          3.1415927
#define INTERIOR    0
#define EXTERIOR    1
#define BOTH        2
#define WHITE       0 
#define GRAY        1
#define BLACK       2
#define NIL        -1
#define INCREASING  1
#define DECREASING  0

/* Common operations */

#undef MAX
#undef MIN
#define MAX(x,y) (((x) > (y))?(x):(y))
#define MIN(x,y) (((x) < (y))?(x):(y))

char   *AllocCharArray(int n);  /* It allocates 1D array of n characters */
int    *AllocIntArray(int n);   /* It allocates 1D array of n integers */
float  *AllocFloatArray(int n); /* It allocates 1D array of n floats */
double *AllocDoubleArray(int n);/* It allocates 1D array of n doubles */

void Error(char *msg,char *func); /* It prints error message and exits
                                     the program. */
void Warning(char *msg,char *func); /* It prints warning message and
                                       leaves the routine. */
void Change(int *a, int *b); /* It changes content between a and b */

typedef struct _FullTimer {
  struct timeval T0, T1;
  time_t  S0, S1;
  double  elapsed;
} FullTimer;

void   StartTimer(FullTimer *ft);
void   StopTimer(FullTimer *ft);
double TimeElapsed(FullTimer *ft);
void   PrintDiff(FullTimer *ft);
double CalcTimevalDiff(struct timeval *a, struct timeval *b);

// return the number of cpus in this computer
int CPUCount();

#endif
