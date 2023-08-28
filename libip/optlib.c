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

/* fallbacks for the functions in optlib.s where MMX isn't
   available */

#include "iconfig.h"
#include "ip_optlib.h"

#if (defined (PROG_NASM) && defined(CPU_MMX)) || ( defined(PROG_YASM) && defined(ARCH_AMD64) )

#undef CMemCpy
#undef CMemSet
#undef CRgbSet
#undef CFloatFill
#undef CRgbRect

// haven't written an asm version yet
#define CDoubleFill 1

#else /* NASM && MMX */

#define CMemCpy    1
#define CMemSet    1
#define CRgbSet    1
#define CFloatFill 1
#define CRgbRect   1

#define CDoubleFill 1

#endif /* NASM && MMX */

#ifdef CMemCpy
#include <string.h>
void MemCpy(void *dest,void *src,int nbytes) {
  memcpy(dest,src,nbytes);
}
#endif

#ifdef CMemSet
#include <string.h>
void MemSet(void *dest,unsigned char c, int nbytes) {
  memset(dest,(int) c,nbytes);
}
#endif

#ifdef CRgbSet
void RgbSet(void *dest,int rgb, int count) {
  int i,r,g,b;
  unsigned char *p;
  r=(rgb>>16);
  g=(rgb>>8)&0xff;
  b=rgb&0xff;
  p=(unsigned char *)dest;
  for(i=count;i;i--) {
    *(p++) = r;
    *(p++) = g;
    *(p++) = b;
  }
}
#endif

#ifdef CFloatFill
void FloatFill(float *dest, float val, int count) {
  int i;
  for (i=0;i<count;i++)
    dest[i] = val;
}
#endif

#ifdef CDoubleFill
void DoubleFill(double *dest, double val, int count) {
  int i;
  for (i=0;i<count;i++)
    dest[i] = val;
}
#endif

#ifdef CRgbRect
void RgbRect(void *dest,int x,int y,int w,int h,int rgb,int rowoffset) {
  unsigned char r,g,b, *p, *q;
  int i;

  r=(rgb >> 16) & 0xff;
  g=(rgb >> 8) & 0xff;
  b=rgb & 0xff;
  
  p = ((unsigned char *)dest) + y * rowoffset + 3 * x;
  for(;h;--h) {
    q = p;
    for(i=w;i;--i) {
      *(p++) = r;
      *(p++) = g;
      *(p++) = b;
    }
    p = q + rowoffset;
  }  
}
#endif
