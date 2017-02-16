/*

    SpineSeg
    http://www.lni.hc.unicamp.br/app/spineseg
    https://github.com/fbergo/braintools
    Copyright (C) 2009-2017 Felipe P.G. Bergo
    fbergo/at/gmail/dot/com

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

// classes here: Mutex

#ifndef MUTEX_H
#define MUTEX_H 1

#include <pthread.h>

//! Mutual exclusion
class Mutex {
 public:
  //! Constructor, creates a new mutex object
  Mutex()  { pthread_mutex_init(&m,0); }

  //! Acquires the mutex. Beware of deadlocks.
  void lock()   { pthread_mutex_lock(&m);   }

  //! Releases the mutex. Beware of deadlocks.
  void unlock() { pthread_mutex_unlock(&m); }
 private:
  pthread_mutex_t m;
};

#endif
