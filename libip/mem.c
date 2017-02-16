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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "ip_mem.h"

struct libip_memconfig {
  int             initted;
  int             threaded;
  void            (*ehandler)(unsigned int);
} memconf = {0, 1};

pthread_mutex_t mutex;

void  MemInit() {
  if (memconf.initted) return;
  pthread_mutex_init(&mutex,NULL);
  memconf.initted = 1;
}

void  MemConfig(int threaded) {
  memconf.threaded = threaded;
}

void *MemAlloc(unsigned int size) {
  void *x;

  MemInit();
  if (memconf.threaded) pthread_mutex_lock(&mutex);
  x = malloc((size_t) size);
  if (memconf.threaded) pthread_mutex_unlock(&mutex);

  if (!x) {

    if (memconf.ehandler != 0)
      memconf.ehandler(size);
    else
      fprintf(stderr,"[libip::MemAlloc] failed to allocate %d bytes and no exception handler is registered.\n",(int) size);
  }

  return x;
}

void *MemRealloc(void *ptr, unsigned int size) {
  void *x;

  MemInit();
  if (memconf.threaded) pthread_mutex_lock(&mutex);
  x = realloc(ptr, (size_t) size);
  if (memconf.threaded) pthread_mutex_unlock(&mutex);

  if (!x) {

    if (memconf.ehandler != 0)
      memconf.ehandler(size);
    else
      fprintf(stderr,"[libip::MemRealloc] failed to allocate %d bytes and no exception handler is registered.\n",(int) size);
  }

  return x;
}

void  MemFree(void *ptr) {
  MemInit();
  if (memconf.threaded) pthread_mutex_lock(&mutex);
  free(ptr);
  if (memconf.threaded) pthread_mutex_unlock(&mutex);
}

void  MemExceptionHandler(void (*callback)(unsigned int)) {
  MemInit();
  memconf.ehandler = callback;
}
