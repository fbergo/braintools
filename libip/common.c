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

/*
  $Id: common.c,v 1.3 2013/11/11 22:34:12 bergo Exp $

  common.c
  array allocation and time interval measurement
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ip_common.h"
#include "iconfig.h"

char *AllocCharArray(int n)
{
  char *v=NULL;
  v = (char *) calloc(n,sizeof(char));
  if (v == NULL)
    Error(MSG1,"AllocCharArray");
  return(v);
}

int *AllocIntArray(int n)
{
  int *v=NULL;
  v = (int *) calloc(n,sizeof(int));
  if (v == NULL)
    Error(MSG1,"AllocIntArray");
  return(v);
}

float *AllocFloatArray(int n)
{
  float *v=NULL;
  v = (float *) calloc(n,sizeof(float));
  if (v == NULL)
    Error(MSG1,"AllocFloatArray");
  return(v);
}

double *AllocDoubleArray(int n)
{
  double *v=NULL;
  v = (double *) calloc(n,sizeof(double));
  if (v == NULL)
    Error(MSG1,"AllocDoubleArray");
  return(v);
}

void Error(char *msg,char *func){ /* It prints error message and exits
                                    the program. */
  fprintf(stderr,"%s %s\n",msg,func);
  exit(-1);
}

void Warning(char *msg,char *func){ /* It prints warning message and
                                       leaves the routine. */
 fprintf(stdout,"%s %s\n",msg,func);

}

void Change(int *a, int *b){ /* It changes content between a and b */
  int c;    
  c  = *a;
  *a = *b;
  *b = c;
}

double TimeElapsed(FullTimer *ft) {
  return(CalcTimevalDiff(&(ft->T0),&(ft->T1)));
}

double CalcTimevalDiff(struct timeval *a, struct timeval *b) {
  double secs;

  // midnight wrapping
  if (a->tv_sec > b->tv_sec)
    b->tv_sec += 3600 * 24;

  secs=1000.0*(b->tv_sec - a->tv_sec);
  secs+=(b->tv_usec - a->tv_usec)/1000.0;
  secs/=1000.0;
  return secs;  
}

void PrintDiff(FullTimer *ft) {
  printf("time elapsed: %.4f seconds\n",TimeElapsed(ft));
}

void StartTimer(FullTimer *ft) {
  gettimeofday(&(ft->T0),0);
  ft->S0 = time(0);
}

void StopTimer(FullTimer *ft) {
  gettimeofday(&(ft->T1), 0);
  ft->S1      = time(0);
  ft->elapsed = TimeElapsed(ft);
}

int CPUCount() {
#ifdef OS_LINUX
  FILE *f;
  static int KnownCount = 0;
  int n,cpus=0;
  char x[512];

  if (KnownCount > 0)
    return KnownCount;

  f=fopen("/proc/cpuinfo","r");
  if (!f) return 1;

  while(fgets(x,511,f)!=0)
    if (sscanf(x,"processor : %d",&n)==1) ++cpus;

  fclose(f);
  
  if (cpus < 1) cpus = 1;
  KnownCount = cpus;

  return KnownCount;

#else
  return 1;
#endif
}
