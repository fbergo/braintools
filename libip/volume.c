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

/*****************************************************************
 $Id: volume.c,v 1.23 2013/11/11 22:34:13 bergo Exp $

 volume.c - volume data structures

******************************************************************/

#if defined SOLARIS
#ifdef SPARC
#define __BIG_ENDIAN    4321
#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#elif defined LINUX || defined BSD
#include <endian.h>
#elif defined __APPLE__
#include <machine/endian.h>
#else
#error edit volume.c and add proper endian detection
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#if !defined SOLARIS
#include <stdint.h>
#else
#include <pthread.h>
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#endif

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ip_optlib.h"
#include "ip_volume.h"
#include "mvf.h"
#include "ip_task.h"
#include "ip_mem.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
 #define conv_le16(a) (a)
 #define conv_le32(a) (a)
 #define conv_be16(a) (((0xff00&(a))>>8)|((0x00ff&(a))<<8))
 #define conv_be32(a) (0xffffffff&((((a)&0xff000000)>>24)|(((a)&0x00ff0000)>>8)|(((a)&0x0000ff00)<<8)|(((a)&0x000000ff)<<24)))
#else
 #define conv_be16(a) (a)
 #define conv_be32(a) (a)
 #define conv_le16(a) (((0xff00&(a))>>8)|((0x00ff&(a))<<8))
 #define conv_le32(a) (0xffffffff&((((a)&0xff000000)>>24)|(((a)&0x00ff0000)>>8)|(((a)&0x0000ff00)<<8)|(((a)&0x000000ff)<<24)))
#endif

#define swab_16(a) (((0xff00&(a))>>8)|((0x00ff&(a))<<8))
#define swab_32(a) (0xffffffff&((((a)&0xff000000)>>24)|(((a)&0x00ff0000)>>8)|(((a)&0x0000ff00)<<8)|(((a)&0x000000ff)<<24)))

/* private calls */
void         pvt_vgmread(Volume *v, FILE *f);
void         pvt_scnread(int bpv, Volume *v, FILE *f);
Volume *     pvt_hdrread(char * hdrpath);
void         pvt_mvf_write(XAnnVolume *v, char *name);
XAnnVolume * pvt_mvf_read(char *name);
XAnnVolume * pvt_vla_read(char *name);


/* pula comentarios com # */
int nc_fgets(char *s, int m, FILE *f) {
  while(fgets(s,m,f)!=NULL)
    if (s[0]!='#') return 1;
  return 0;
}

/* pula comentarios com # */
int nc_gzgets(char *s, int m, gzFile f) {
  while(gzgets(f,s,m)!=NULL)
    if (s[0]!='#') return 1;
  return 0;
}

Volume *VolumeNew(int W,int H,int D, voltype type) {
  Volume *v;
  unsigned int i;

  v=(Volume *) MemAlloc(sizeof(Volume));
  if (!v)
    goto out_nomem;
    
  v->W=W;
  v->H=H;
  v->D=D;
  v->WxH = W * H;
  v->N = W * H * D;
  v->Type = type;

  v->dx = v->dy = v->dz = 1.0;

  switch(type) {
  case vt_integer_8:
    v->u.i8 = (i8_t *) MemAlloc(v->N * sizeof(i8_t));
    if (v->u.i8 == 0) goto out_nomem_free;
    MemSet(v->u.i8,0,v->N * sizeof(i8_t));
    break;
  case vt_integer_16:
    v->u.i16 = (i16_t *) MemAlloc(v->N * sizeof(i16_t));
    if (!v->u.i16) goto out_nomem_free;
    MemSet(v->u.i16,0,v->N * sizeof(i16_t));
    break;
  case vt_integer_32:
    v->u.i32 = (i32_t *) MemAlloc(v->N * sizeof(i32_t));
    if (v->u.i32 == 0) goto out_nomem_free;
    MemSet(v->u.i32,0,v->N * sizeof(i32_t));
    break;
  case vt_float_32:
    v->u.f32 = (f32_t *) MemAlloc(v->N * sizeof(f32_t));
    if (!v->u.f32) goto out_nomem_free;
    FloatFill(v->u.f32,0.0,v->N);
    break;
  case vt_float_64:
    v->u.f64 = (f64_t *) MemAlloc(v->N * sizeof(f64_t));
    if (!v->u.f64) goto out_nomem_free;
    DoubleFill(v->u.f64,0.0,v->N);
    break;
  }

  v->tbrow  =(int *)MemAlloc(H * sizeof(int));
  if (!v->tbrow)
    goto out_nomem_free;

  v->tbframe=(int *)MemAlloc(D * sizeof(int));

  if (!v->tbframe)
    goto out_nomem_free2;

  for(i=0;i<H;i++) v->tbrow[i]   = W * i;
  for(i=0;i<D;i++) v->tbframe[i] = W * H * i;

  return v;

 out_nomem_free2:
  MemFree(v->tbrow);
 out_nomem_free:
  MemFree(v);
 out_nomem:
  errno = ENOMEM;
  return 0;
}

void VolumeDestroy(Volume *v) {
  if (v) {
    MemFree(v->tbframe);
    MemFree(v->tbrow);
    switch(v->Type) {
    case vt_integer_8:  MemFree(v->u.i8); break;
    case vt_integer_16: MemFree(v->u.i16); break;
    case vt_integer_32: MemFree(v->u.i32); break;
    case vt_float_32:   MemFree(v->u.f32); break;
    case vt_float_64:   MemFree(v->u.f64); break;
    }
    MemFree(v);
  }
}

void VolumeFillF(Volume *v, double c) {
  switch(v->Type) {
  case vt_float_32:
    FloatFill(v->u.f32,(float) c, v->N);
    break;
  case vt_float_64:
    DoubleFill(v->u.f64,c, v->N);
    break;
  default:
    VolumeFill(v,(int) c);
  }
}

void VolumeFill(Volume *v, int c) {
  unsigned int i;
  i16_t x0;
  i32_t x1;

  switch(v->Type) {
    case vt_integer_8:
      MemSet(v->u.i8, (i8_t) c, v->N);
      break;
    case vt_integer_16:
      x0 = (i16_t) c;
      for(i=0;i<v->N;i++) v->u.i16[i] = x0;
      break;
    case vt_integer_32:
      x1= (i32_t) c;
      for(i=0;i<v->N;i++) v->u.i32[i] = x1;
      break;
    case vt_float_32:
      FloatFill(v->u.f32, (float) c, v->N);
      break;
    case vt_float_64:
      DoubleFill(v->u.f64, (double) c, v->N);
      break;
  }
}

i8_t VolumeGetI8(Volume *v, int x,int y, int z) {
  return( v->u.i8[ x + v->tbrow[y] + v->tbframe[z] ] );
}

i16_t VolumeGetI16(Volume *v, int x,int y, int z) {
  return( v->u.i16[ x + v->tbrow[y] + v->tbframe[z] ] );
}

i32_t VolumeGetI32(Volume *v, int x,int y, int z) {
  return( v->u.i32[ x + v->tbrow[y] + v->tbframe[z] ] );
}

f32_t VolumeGetF32(Volume *v, int x,int y, int z) {
  return( v->u.f32[ x + v->tbrow[y] + v->tbframe[z] ] );
}

f64_t VolumeGetF64(Volume *v, int x,int y, int z) {
  return( v->u.f64[ x + v->tbrow[y] + v->tbframe[z] ] );
}

void VolumeSetI8(Volume *v, int x,int y, int z, i8_t c) {
  v->u.i8[ x + v->tbrow[y] + v->tbframe[z] ] = c;
}

void VolumeSetI16(Volume *v, int x,int y, int z, i16_t c) {
  v->u.i16[ x + v->tbrow[y] + v->tbframe[z] ] = c;
}

void VolumeSetI32(Volume *v, int x,int y, int z, i32_t c) {
  v->u.i32[ x + v->tbrow[y] + v->tbframe[z] ] = c;
}

void VolumeSetF32(Volume *v, int x,int y, int z, f32_t c) {
  v->u.f32[ x + v->tbrow[y] + v->tbframe[z] ] = c;
}

void VolumeSetF64(Volume *v, int x,int y, int z, f64_t c) {
  v->u.f64[ x + v->tbrow[y] + v->tbframe[z] ] = c;
}

int VolumeValid(Volume *v, int x, int y, int z) {
  if (x < 0 || y < 0 || z < 0 || 
      x >= v->W || y >= v->H || z>= v->D) return 0;
  return 1;
}

int ValidVoxel(Volume *v, Voxel *e) {
  return(VolumeValid(v,e->x,e->y,e->z));
}

void VolumeVoxelOf(Volume *v, int n, Voxel *out) {
  out->z = n / v->WxH;
  out->y = (n % v->WxH) / v->W;
  out->x = (n % v->WxH) % v->W;
}

Volume *VolumeNewAsClip(Volume *src, 
			int x0,int x1,int y0,int y1,int z0,int z1)
{
  Volume *dest;
  int i,j,k, nsrc, ndest;

  dest=VolumeNew(x1-x0+1, y1-y0+1, z1-z0+1, src->Type);
  if (!dest) return 0;

  dest->dx = src->dx;
  dest->dy = src->dy;
  dest->dz = src->dz;

  /* e', existe um motivo para se usar C++ para polimorfismo... */
#define VNAC_HEAD for(k=z0;k<=z1;k++) for(j=y0;j<=y1;j++) for(i=x0;i<=x1;i++) { nsrc=i+src->tbrow[j]+src->tbframe[k]; ndest=(i-x0)+dest->tbrow[j-y0]+dest->tbframe[k-z0];

#define VNAC_TAIL }

  switch(src->Type) {
  case vt_integer_8:
    { VNAC_HEAD dest->u.i8[ndest] = src->u.i8[nsrc]; VNAC_TAIL }
    break;
  case vt_integer_16:
    { VNAC_HEAD dest->u.i16[ndest] = src->u.i16[nsrc]; VNAC_TAIL }
    break;
  case vt_integer_32:
    { VNAC_HEAD dest->u.i32[ndest] = src->u.i32[nsrc]; VNAC_TAIL }
    break;
  case vt_float_32:
    { VNAC_HEAD dest->u.f32[ndest] = src->u.f32[nsrc]; VNAC_TAIL }
    break;
  case vt_float_64:
    { VNAC_HEAD dest->u.f64[ndest] = src->u.f64[nsrc]; VNAC_TAIL }
    break;
  }
#undef VNAC_HEAD
#undef VNAC_TAIL

  return dest;
}

Volume *VolumeNewFromFile(char *path) {
  Volume *v=0;
  int fmt,i,w,h,d,bpv=8;
  int V,vmax,ShiftAmount=0;
  float X[3];
  FILE *f=0;
  char z[4096];

  SetProgress(0,1000);

  fmt = GuessFileFormat(path);

  switch(fmt) {
  case FMT_UNKNOWN:
    fprintf(stderr,"** unrecognized file format **\n");
    break;
  case FMT_VGM:
    f=fopen(path,"r");
    if (f) {
      if (!nc_fgets(z,4095,f)) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z,"%d %d %d \n",&w,&h,&d)!=3) break;
      if (!w || !h || !d) break;
      v=VolumeNew(w,h,d,vt_integer_16);
      pvt_vgmread(v,f);
      fclose(f);
      f=0;
    }
    break;
  case FMT_SCN:
    f=fopen(path,"r");
    if (f) {
      if (!nc_fgets(z,4095,f)) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z," %d %d %d \n",&w,&h,&d)!=3) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z," %f %f %f \n",&X[0],&X[1],&X[2])!=3) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z," %d \n",&bpv)!=1) break;
      if (bpv<=16)
	v=VolumeNew(w,h,d,vt_integer_16);
      else
	v=VolumeNew(w,h,d,vt_integer_32);
      v->dx = X[0];
      v->dy = X[1];
      v->dz = X[2];
      pvt_scnread(bpv,v,f);
      fclose(f);
      f=0;
    }
    break;
  case FMT_P2:
    f=fopen(path,"r");
    if (f) {
      if (!nc_fgets(z,4095,f)) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z," %d %d \n",&w,&h)!=2) break;
      d=1;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z," %d \n",&vmax)!=1) break;
      ShiftAmount=0;
      if (vmax < 256)
	while(vmax > SMAX || vmax > 32767) { vmax>>=1; ++ShiftAmount; }
      v=VolumeNew(w,h,d,vt_integer_16);      
      MemSet(z,0,256);
      i=0;
      vmax=0;
      while(i!=v->N) {
	V=fgetc(f);
	if (V<'0') {
	  if (vmax) {
	    V=atoi(z);
	    V>>=ShiftAmount;
	    v->u.i16[i++] = (i16_t) V;
	    MemSet(z,0,256);
	    vmax=0;
	  }
	} else {
	  z[vmax++]=(char) V;	  
	}
      }
      fclose(f);
      f=0;
    }
    break;

  case FMT_P5:
    f=fopen(path,"r");
    if (f) {
      if (!nc_fgets(z,4095,f)) break;
      if (!nc_fgets(z,4095,f)) break;
      if (sscanf(z,"%d %d \n",&w,&h)!=2) break;
      d = 1;
      if (!w || !h ) break;
      if (!nc_fgets(z,4095,f)) break;
      v=VolumeNew(w,h,d,vt_integer_16);
      pvt_vgmread(v,f);
      fclose(f);
      f=0;
    }
    break;

  case FMT_HDR:
    v = pvt_hdrread(path);
    break;
  }

  if (f) fclose(f);

  EndProgress();
  return v;
}

int VolumeSaveSCN32(Volume *v, char *path) {
  FILE *f;
  i32_t blk[8192];
  int gone, left, x, i;

  if (v->Type != vt_integer_32)
    return 0;

  f=fopen(path,"w");
  if (!f) return 0;

  fprintf(f,"SCN\n%d %d %d\n%.2f %.2f %.2f\n32\n",
	  v->W,v->H,v->D,
	  v->dx, v->dy, v->dz);

  gone=0; left=v->N;
  while(left) {
    x = 8192; if (left < x) x = left;
    MemCpy(blk,&(v->u.i32[gone]),4*x);
    for(i=0;i<x;i++)
      blk[i] = conv_le32(blk[i]);
    i = fwrite(blk,4,x,f);
    if (i<=0) {
      fclose(f);
      return 0;
    }
    gone+=x;
    left-=x;
  }

  fclose(f);
  return 1;
}

int VolumeSaveSCN8(Volume *v, char *path) {
  FILE *f;
  i8_t blk[8192];
  int gone, left, x, i, p;

  if (v->Type != vt_integer_8)
    return 0;

  f=fopen(path,"w");
  if (!f) return 0;

  fprintf(f,"SCN\n%d %d %d\n%.2f %.2f %.2f\n8\n",
	  v->W,v->H,v->D,
	  v->dx, v->dy, v->dz);

  gone=0; left=v->N; p = 0;
  SetProgress(gone,v->N);
  while(left) {
    x = 8192; if (left < x) x = left;
    MemCpy(blk,&(v->u.i8[gone]),x);
    i = fwrite(blk,1,x,f);
    if (i<=0) {
      fclose(f);
      return 0;
    }
    gone+=x;
    left-=x;
    if (p%20 == 0)
      SetProgress(gone,v->N);
    ++p;
  }

  fclose(f);
  EndProgress();
  return 1;
}

int VolumeSaveSCN(Volume *v, char *path) {
  FILE *f;
  i16_t blk[8192];
  int gone, left, x, i;

  if (v->Type != vt_integer_16)
    return 0;

  f=fopen(path,"w");
  if (!f) return 0;

  fprintf(f,"SCN\n%d %d %d\n%.2f %.2f %.2f\n16\n",
	  v->W,v->H,v->D,
	  v->dx, v->dy, v->dz);

  gone=0; left=v->N;
  while(left) {
    x = 8192; if (left < x) x = left;
    MemCpy(blk,&(v->u.i16[gone]),2*x);
    for(i=0;i<x;i++)
      blk[i] = conv_le16(blk[i]);
    i = fwrite(blk,2,x,f);
    if (i<=0) {
      fclose(f);
      return 0;
    }
    gone+=x;
    left-=x;
  }

  fclose(f);
  return 1;
}

int VolumeViewShift(Volume *v) {
  int maxv,sa;
  if (v->Type == vt_integer_8) return 0;
  maxv = VolumeMax(v);
  sa = 0;
  while (maxv > VMAX) { maxv >>= 1; ++sa; }
  return sa;
}

int VolumeMax(Volume *v) {
  unsigned int i;
  i8_t  m8  = 0;
  i16_t m16 = 0;
  i32_t m32 = 0;
  f32_t x32 = 0.0;
  f64_t x64 = 0.0;

  switch(v->Type) {
  case vt_integer_8:
    for(i=0;i<v->N;i++) if (v->u.i8[i] > m8) m8 = v->u.i8[i];
    return ((int) m8);
  case vt_integer_16:
    for(i=0;i<v->N;i++) if (v->u.i16[i] > m16) m16 = v->u.i16[i];
    return ((int) m16);
  case vt_integer_32:
    for(i=0;i<v->N;i++) if (v->u.i32[i] > m32) m32 = v->u.i32[i];
    return ((int) m32);
  case vt_float_32:
    for(i=0;i<v->N;i++) if (v->u.f32[i] > x32) x32 = v->u.f32[i];
    return((int) x32);
  case vt_float_64:
    for(i=0;i<v->N;i++) if (v->u.f64[i] > x64) x64 = v->u.f64[i];
    return((int) x64);
  }
  return 0;
}

/* xa */

XAnnVolume * XAVolNew(int W,int H, int D) {
  XAnnVolume *a;
  int i;

  a=(XAnnVolume *)MemAlloc(sizeof(XAnnVolume));
  
  a->W   = W;
  a->H   = H;
  a->D   = D;
  a->WxH = W*H;
  a->N   = W*H*D;

  a->dx = a->dy = a->dz = 1.0;

  a->vd = (evoxel *) MemAlloc(sizeof(evoxel) * a->N);
  MemSet(a->vd, 0, sizeof(evoxel) * a->N);

  a->tbrow  =(int *)MemAlloc(H * sizeof(int));
  a->tbframe=(int *)MemAlloc(D * sizeof(int));
  for(i=0;i<H;i++) a->tbrow[i]   = W * i;
  for(i=0;i<D;i++) a->tbframe[i] = W * H * i;

  a->addseeds = MapNew();
  a->delseeds = MapNew();
  a->predadj  = Spherical(1.0);
  AdjCalcDN(a->predadj, W, H, D);

  a->extra = 0;
  a->extra_is_freeable = 1;

  XAVolReset(a);
  return a;
}

XAnnVolume * XAVolNewWithVolume(Volume *v) {
  XAnnVolume *a;
  int i;

  a=XAVolNew(v->W,v->H,v->D);
  a->dx = v->dx;
  a->dy = v->dy;
  a->dz = v->dz;

  switch(v->Type) {
  case vt_integer_8:
    for(i=0;i<a->N;i++) a->vd[i].orig = (vtype) v->u.i8[i];
    break;
  case vt_integer_16:
    for(i=0;i<a->N;i++) a->vd[i].orig = (vtype) v->u.i16[i];
    break;
  case vt_integer_32:
    for(i=0;i<a->N;i++) a->vd[i].orig = (vtype) v->u.i32[i];
    break;
  case vt_float_32:
    for(i=0;i<a->N;i++) a->vd[i].orig = (vtype) v->u.f32[i];
    break;
  case vt_float_64:
    for(i=0;i<a->N;i++) a->vd[i].orig = (vtype) v->u.f64[i];
    break;
  }
  for(i=0;i<a->N;i++) a->vd[i].value = a->vd[i].orig;
  return a;
}

void         XAVolDestroy(XAnnVolume *v) {
  if (v) {
    MemFree(v->vd);
    MemFree(v->tbrow);
    MemFree(v->tbframe);
    DestroyAdjRel(&(v->predadj));
    MapDestroy(v->addseeds);
    MapDestroy(v->delseeds);
    MemFree(v);
  }
}

void         XAVolReset(XAnnVolume *v) {
  int i;

  MapClear(v->addseeds);
  MapClear(v->delseeds);

  for(i=0;i<v->N;i++) {
    v->vd[i].label = 0;
    v->vd[i].cost  = HIGH_S16;
    v->vd[i].pred  = 0;
  }

  if (v->extra && v->extra_is_freeable) {
    MemFree(v->extra);
    v->extra = 0;
  }

}

int         XAVolGetRootXYZ(XAnnVolume *v, int x,int y,int z) {
  int n;
  if (!XAVolumeValid(v,x,y,z)) return -1;
  n = x + v->tbrow[y] + v->tbframe[z];
  return(XAVolGetRootN(v,n));
}

int         XAVolGetRootN(XAnnVolume *v, int n) {
  if (n<0 || n>= v->N)
    return -1;
  
  while(v->vd[n].pred != 0)
    n=AdjDecodeDN(v->predadj, n, v->vd[n].pred);

  return n;
}

void        XAVolTagRemoval(XAnnVolume *v, int n) {
  int *fifo;
  int fifosz, getpt, putpt;
  int i,j,x,y;

  fifosz = v->N;
  fifo=(int *)MemAlloc(sizeof(int) * fifosz);
  getpt = 0;
  putpt = 1;

  fifo[0] = n;

  v->vd[n].label |= REMOVEMASK;
  while(getpt!=putpt) {
    x=fifo[getpt];
    getpt = (getpt + 1) % fifosz;
    
    for(i=1;i<v->predadj->n;i++) {
      j= x + v->predadj->dn[i];
      if (j<0 || j>= v->N)
	continue;
      y = AdjDecodeDN(v->predadj, j, v->vd[j].pred);
      if (y==x) {
	v->vd[j].label |= REMOVEMASK;
	fifo[putpt] = j;
	putpt = (putpt + 1) % fifosz;
      }
    }
  }

  MemFree(fifo);
}

void        XAVolUntagRemoval(XAnnVolume *v, int n) {
  int *fifo;
  int fifosz, getpt, putpt;
  int i,j,x,y;

  fifosz = v->N;
  fifo=(int *)MemAlloc(sizeof(int) * fifosz);
  getpt = 0;
  putpt = 1;

  fifo[0] = n;

  v->vd[n].label &= LABELMASK;
  while(getpt!=putpt) {
    x=fifo[getpt];
    getpt = (getpt + 1) % fifosz;
    for(i=1;i<v->predadj->n;i++) {
      j= x + v->predadj->dn[i];
      if (j<0 || j>= v->N)
	continue;
      y = AdjDecodeDN(v->predadj, j, v->vd[j].pred);
      if (y==x) {
	v->vd[j].label &= LABELMASK;
	fifo[putpt] = j;
	putpt = (putpt + 1) % fifosz;
      }          
    }
  }

  MemFree(fifo);
}

void XAVolToggleRemoval(XAnnVolume *v, int n) {
  int *fifo;
  int fifosz, getpt, putpt;
  int i,j,x,y;

  fifosz = v->N;
  fifo=(int *)MemAlloc(sizeof(int) * fifosz);
  getpt = 0;
  putpt = 1;

  fifo[0] = n;

  v->vd[n].label ^= REMOVEMASK;
  while(getpt!=putpt) {
    x=fifo[getpt];
    getpt = (getpt + 1) % fifosz;
    for(i=1;i<v->predadj->n;i++) {
      j= x + v->predadj->dn[i];
      if (j<0 || j>= v->N)
	continue;
      y = AdjDecodeDN(v->predadj, j, v->vd[j].pred);
      if (y==x) {
	v->vd[j].label ^= REMOVEMASK;
	fifo[putpt] = j;
	putpt = (putpt + 1) % fifosz;
      }          
    }
  }

  MemFree(fifo);  
}

int XAValidVoxel(XAnnVolume *v, Voxel *p) {
  if (p->x < 0 || p->y < 0 || p->z < 0) return 0;
  if (p->x >= v->W || p->y >= v->H || p->z >= v->D) return 0;
  return 1;
}

int         XAVolumeValid(XAnnVolume *v, int x, int y, int z) {
  if (x < 0 || y < 0 || z < 0) return 0;
  if (x >= v->W || y >= v->H || z>= v->D) return 0;
  return 1;
}

void        XAVolumeVoxelOf(XAnnVolume *v, int n, Voxel *out) {
  out->z = n / v->WxH;
  out->y = (n % v->WxH) / v->W;
  out->x = (n % v->WxH) % v->W;
}

void XAVolSave(XAnnVolume *v, char *name) {
  pvt_mvf_write(v, name);
}

/* -------------- old VLA format , for reference
void         XAVolSave(XAnnVolume *v, char *name) {
  FILE *f;
  int i,j,k,m,n;
  int nblk, bsz;
  char *blk;

  f=fopen(name,"w");
  if (!f) return;

  fprintf(f,"VLA\n3\n");
  fprintf(f,"%d %d %d\n",v->W,v->H,v->D);

  // ADD SEEDS

  j=MapSize(v->addseeds);
  fprintf(f,"aseeds = %d\n",j);
  
  for(i=0;i<j;i++) {
    MapGet(v->addseeds,i,&m,&n);
    fprintf(f,"%d %d\n",m,n);
  }

  // DEL SEEDS

  j=MapSize(v->delseeds);
  fprintf(f,"dseeds = %d\n",j);

  for(i=0;i<j;i++) {
    MapGet(v->delseeds,i,&m,&n);
    fprintf(f,"%d %d\n",m,n);
  }

  // PREDECESSOR ADJACENCY
  j=v->predadj->n;

  fprintf(f,"adj = %d\n",j);

  for(i=0;i<j;i++) {
    fprintf(f,"%d %d %d %d\n",
	    v->predadj->dx[i],
	    v->predadj->dy[i],
	    v->predadj->dz[i],
	    v->predadj->dn[i]);
  }

  // IMAGES: ORIG, VALUE, LABEL, PRED, COST [16bit]

  blk = (char *) MemAlloc(9 * 8192);
  if (!blk) {
    fprintf(stderr,"** unable to allocate memory\n");
    exit(5);
  }

  nblk = v->N / 8192;
  if (v->N % 8192) ++nblk;

  for(i=0;i<nblk;i++) {
    k = i * 8192;
    bsz = 8192; // voxels in this block
    if (k+bsz > v->N)
      bsz = v->N - k;
    // prepare 8K voxels on buffer
    MemSet(blk,0,9*8192);
    for(j=0;j<bsz;j++) {
      *((vtype *)((void *)(blk+j*9+0))) = v->vd[k+j].orig;
      *((vtype *)((void *)(blk+j*9+2))) = v->vd[k+j].value;
      *((ctype *)((void *)(blk+j*9+4))) = v->vd[k+j].cost;
      *((ltype *)((void *)(blk+j*9+6))) = 0; // pertinence
      *((ltype *)((void *)(blk+j*9+7))) = v->vd[k+j].label;
      *((ltype *)((void *)(blk+j*9+8))) = v->vd[k+j].pred;
    }
    fwrite(blk, 9, bsz, f);
    //printf("W block %d of %d (%d%%)\n",i,nblk-1,(100*i)/(nblk-1));
  }

  fclose(f);
  MemFree(blk);
}
--------------------------------------------------------- */

XAnnVolume * XAVolLoad(char *name) {

  if (mvf_check_format(name))
    return(pvt_mvf_read(name));
  if (IsVLAFile(name))
    return(pvt_vla_read(name));

  fprintf(stderr,"XAVolLoad ** unrecognized file format.\n\n");
  return 0;
}
 
int IsMVFFile(char *path) {
  return(mvf_check_format(path));
}

int IsVLAFile(char *path) {
  FILE *f;
  char z[256];

  f=fopen(path,"r");
  if (!f) return 0;

  if (!fgets(z,255,f)) {
    fclose(f);
    return 0;
  }
  fclose(f);
  z[4] = 0;
  if (!strcmp(z,"VLA\n"))
    return 1;
  else
    return 0;
}

void VGMFileStat(char *path, VGMStat *out) {
  FILE *f;
  char z[4096];
  struct stat s;

  out->W = out->H = out->D = 0;
  out->valid   = 0;
  out->success = 0;
  out->error   = 0;
  out->size    = 0;

  f=fopen(path,"r");
  if (!f) {
    out->error = errno;
    return;
  }
  
  out->success = 1;

  if (!nc_fgets(z,4095,f)) { fclose(f); return; }
  if (strstr(z,"VGM")!=z)  { fclose(f); return; }
  if (!nc_fgets(z,4095,f)) { fclose(f); return; }
  out->W=atoi(strtok(z," \t\n\r"));
  out->H=atoi(strtok(0," \t\n\r"));
  out->D=atoi(strtok(0," \t\n\r"));
  
  fclose(f);
  out->valid=1;

  if (stat(path,&s)==0)
    out->size = s.st_size;
  
}

 Volume * XAVolToVol(XAnnVolume *v, 
		     int x0,int x1,
		     int y0,int y1,
		     int z0,int z1) 
{
  Volume *sv;
  int i,j,k;
  sv=VolumeNew(x1-x0+1,y1-y0+1,z1-z0+1,vt_integer_16);

  for(i=z0;i<=z1;i++)
    for(j=y0;j<=y1;j++)
      for(k=x0;k<=x1;k++)
	sv->u.i16[ (k-x0) + sv->tbrow[j-y0] + sv->tbframe[i-z0] ] =
	  v->vd[ k + v->tbrow[j] + v->tbframe[i] ].value;

  return sv;
}

/*
  returns FMT_UNKNOWN, FMT_VGM, FMT_SCN, FMT_P2, FMT_P5 or FMT_HDR
*/
int GuessFileFormat(char *path) {
  int r = FMT_UNKNOWN;
  FILE *f;
  int i,nlen,a,b;
  char z[256];
  struct stat s;

  f=fopen(path,"r");
  if (!f) {
    fprintf(stderr,"** couldn't open %s\n",path);
    return r;
  }

  MemSet(z,0,256);

  /*
    check for analyze format, requirements for detection:
    
    + name ends in .hdr
    + sizeof_hdr must match file size
    + regular field must be 'r'

    header is written in big-endian order
  */
  nlen = strlen(path);
  if (nlen>3) {
    if (!strcasecmp(&path[nlen-4],".hdr")) {
      if (fread(z,1,40,f)==40) {
	if (stat(path,&s)==0) {
	  a = *((uint32_t *)(&z[0]));
	  b = conv_be32(a);
	  if (a == s.st_size || b == s.st_size) {
	    if (z[38] == 'r') {
	      fclose(f);
	      return FMT_HDR;
	    }
	  }
	}
      }
    }
  }

  MemSet(z,0,256);
  fseek(f,0,SEEK_SET);

  for(;;) {
    for(i=0;i<3;i++) {
      z[i] = fgetc(f);
      if (z[i]==EOF) { fclose(f); return r; }
    }
    if (z[0]!='#') break;
    if (!fgets(z,255,f)) { 
      fclose(f);
      return r; 
    }   
  }

  fclose(f);

  if (z[0]=='V' && z[1]=='G' && z[2]=='M') r = FMT_VGM;
  if (z[0]=='S' && z[1]=='C' && z[2]=='N') r = FMT_SCN;
  if (z[0]=='P' && z[1]=='2') r = FMT_P2;
  if (z[0]=='P' && z[1]=='5') r = FMT_P5;
  return r;
}

ltype XAGetLabel(XAnnVolume *v, int n) {
  return(v->vd[n].label);
}

vtype XAGetOrig(XAnnVolume *v, int n) {
  return(v->vd[n].orig);
}

vtype XAGetValue(XAnnVolume *v, int n) {
  return(v->vd[n].value);
}

ctype XAGetCost(XAnnVolume *v, int n) {
  return(v->vd[n].cost);
}

void XASetLabel(XAnnVolume *v, int n, ltype value) {
  v->vd[n].label = value;
}

int XAGetDecPredecessor(XAnnVolume *v, int n) {
  return(AdjDecodeDN(v->predadj,n,v->vd[n].pred));
}

int XAGetOrigMax(XAnnVolume *v) {
  int i,j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].orig > j)
      j = v->vd[i].orig;
  return j;
}

int XAGetValueMax(XAnnVolume *v) {
  int i,j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].value > j)
      j = v->vd[i].value;
  return j;
}

int XAGetCostMax(XAnnVolume *v) {
  int i,j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].cost > j)
      j = v->vd[i].cost;
  return j;
}

int XAGetOrigViewShift(XAnnVolume *v) {
#ifdef VL8BIT
  return 0;
#else
  int i;
  vtype j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].orig > j)
      j = v->vd[i].orig;
  i=0;
  while(j>VMAX) { j>>=1; ++i; }
  return i;
#endif
}

int XAGetValueViewShift(XAnnVolume *v) {
#ifdef VL8BIT
  return 0;
#else
  int i;
  vtype j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].value > j)
      j = v->vd[i].value;
  i=0;
  while(j>VMAX) { j>>=1; ++i; }
  return i;
#endif
}

int XAGetCostViewShift(XAnnVolume *v) {
#ifdef VL8BIT
  return 0;
#else
  int i;
  ctype j;
  j=0;
  for(i=0;i<v->N;i++)
    if (v->vd[i].cost > j)
      j = v->vd[i].cost;
  i=0;
  while(j>VMAX) { j>>=1; ++i; }
  return i;
#endif
}

/* private I/O calls */

void pvt_vgmread(Volume *v, FILE *f) {
  int i,j,k,bsz,nblk;
  unsigned char *blk;

  blk = (unsigned char *) MemAlloc(32768);
  if (!blk) {
    fprintf(stderr,"Unable to allocate memory.\n\n");
    exit(5);
  }
  nblk = v->N / 32768;
  if (v->N % 32768) ++nblk;

  for(i=0;i<nblk;i++) {
    k = i*32768;
    bsz = 32768;
    if (k+bsz > v->N) bsz = v->N - k;

    if (fread(blk, 1, bsz, f)!=bsz)
      fprintf(stderr,"*** I/O error\n\n");

    for(j=0;j<bsz;j++)
      v->u.i16[k+j] = (i16_t) blk[j];
  }
  MemFree(blk);
}

void pvt_scnread(int bpv, Volume *v, FILE *f) {
  long streampos;
  int i,j,k,bsz,nblk;
  unsigned char *blk;
  short int *blk2;
  int *blk4;

  unsigned char cmax = 0;
  short int     smax = 0;
  int           ShiftAmount = 0;

  streampos = ftell(f);

  blk = (unsigned char *) MemAlloc(32768);
  if (!blk) {
    fprintf(stderr,"Unable to allocate memory.\n\n");
    exit(5);
  }
  blk2 = (short int *) blk;
  blk4 = (int *) blk;

  switch(bpv) {
    
  case 8:

    nblk = v->N / 32768;
    if (v->N % 32768) ++nblk;

    // find out max value
    for(i=0;i<nblk;i++) {
      k = i*32768;
      bsz = 32768; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 1, bsz, f);
      
      for(j=0;j<bsz;j++)
	if (blk[j] > cmax) cmax = blk[j];

      if (i%10 == 0)
	SetProgress(i,2*nblk);
    }

    // find out shift amount
    if (cmax != 0)
      while((cmax&0x80) == 0) { cmax<<=1; ++ShiftAmount; }

    // actually read out scene data
    fseek(f,streampos,SEEK_SET);

    for(i=0;i<nblk;i++) {
      k = i*32768;
      bsz = 32768; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 1, bsz, f);

      for(j=0;j<bsz;j++)
	v->u.i16[k+j] = (i16_t) (blk[j]);

      if (i%10 == 0)
	SetProgress(nblk+i,2*nblk);
    }
    // apply shift amount

    /*
    if (ShiftAmount)
	for(j=0;j<v->N;j++)
	  v->u.i16[j] <<= ShiftAmount;
    */
    break;
  case 16:
    nblk = v->N / 16384;
    if (v->N % 16384) ++nblk;

    // find out max value
    for(i=0;i<nblk;i++) {
      k = i*16384;
      bsz = 16384; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 2, bsz, f);

      for(j=0;j<bsz;j++) {
	blk2[j] = conv_le16(blk2[j]);
	if (blk2[j] > smax) smax = blk2[j];
      }

      if (i%10 == 0)
	SetProgress(i,2*nblk);
    }

    // find out shift amount (if image is already 16-bit, keep
    // untouched)
    if (smax < 256 && smax > 32)
      while((smax&0xc000) == 0) { smax<<=1; ++ShiftAmount; }

    // actually read out scene data, applying shift amount
    fseek(f,streampos,SEEK_SET);

    for(i=0;i<nblk;i++) {
      k = i*16384;
      bsz = 16384; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 2, bsz, f);

      for(j=0;j<bsz;j++) {
	blk2[j] = conv_le16(blk2[j]);
	if (ShiftAmount != 0) blk2[j] <<= ShiftAmount;

	v->u.i16[k+j] = (i16_t) blk2[j];
      }

      if (i%10 == 0)
	SetProgress(nblk+i,2*nblk);
    }

    for(i=0;i<v->N;i++)
      if (v->u.i16[i] < -1) {
	for(j=0;j<v->N;j++) {
	  k = (int) ((unsigned short int) (v->u.i16[j]));
	  k >>= 1;
	  v->u.i16[j] = (i16_t) k;
	}
	break;
      }

    break;
  case 32:
    nblk = v->N / 8192;
    if (v->N % 8192) ++nblk;

    for(i=0;i<nblk;i++) {
      k = i*8192;
      bsz = 8192; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 4, bsz, f);

      for(j=0;j<bsz;j++) {
	blk4[j] = conv_le32(blk4[j]);
	v->u.i32[k+j] = (i32_t) blk4[j];
      }

      if (i%10 == 0)
	SetProgress(i,nblk);
    }
    break;   
  default:
    fprintf(stderr,"Unsupported bpv: %d\n",bpv);
  }
  
  MemFree(blk);
}

Volume * pvt_hdrread(char * hdrpath) {
  Volume *v;
  int w,h,d, dt, bp, x, itmp;
  float fvoff,dx,dy,dz;
  char imgpath[512], header[148];
  int  nlen;
  FILE *f;
  int be = 0, ambe = 0;

  int i,j,k,bsz,nblk;
  unsigned char *blk;
  short int *blk2;

  int dim;

  nlen = strlen(hdrpath);
  if (nlen < 5) return 0;
  strcpy(imgpath, hdrpath);
  imgpath[nlen-3] = 'i';
  imgpath[nlen-2] = 'm';
  imgpath[nlen-1] = 'g';

  f = fopen(hdrpath,"r");
  if (!f) return 0;

  if (fread(header, 1, 148, f)!=148) {
    fclose(f);
    return 0;
  }
  fclose(f);

  if (header[0] == 0) be = 1; else be = 0;
  ambe = 0x11110000;
  if ( ((char *)(&ambe))[0] == 0x11 ) ambe = 1; else ambe = 0;

  dim = *((uint16_t *)(&header[40]));
  if (be!=ambe) dim = swab_16(dim); 

  switch(dim) {
  case 2: // 2D image
    w = *((uint16_t *)(&header[42]));
    h = *((uint16_t *)(&header[44]));
    d = 1;

    if (ambe != be) { w = swab_16(w); h = swab_16(h); }

    itmp = *((uint32_t *)(&header[76]));
    if (ambe != be) itmp = swab_32(itmp);
    dx = * ((float *) (&itmp));
    itmp = *((uint32_t *)(&header[80]));
    if (ambe != be) itmp = swab_32(itmp);
    dy = * ((float *) (&itmp));
    dz = 1.0;

    break;
  case 3: // 3D image
  case 4: // 4D (3D, time-series), use first "frame" only
    w = *((uint16_t *)(&header[42]));
    h = *((uint16_t *)(&header[44]));
    d = *((uint16_t *)(&header[46]));

    if (ambe != be) { w = swab_16(w); h = swab_16(h); d = swab_16(d); }

    itmp = *((uint32_t *)(&header[80]));
    if (ambe != be) itmp = swab_32(itmp);
    dx = * ((float *) (&itmp));
    itmp = *((uint32_t *)(&header[84]));
    if (ambe != be) itmp = swab_32(itmp);
    dy = * ((float *) (&itmp));
    itmp = *((uint32_t *)(&header[88]));
    if (ambe != be) itmp = swab_32(itmp);
    dz = * ((float *) (&itmp));

    break;
  default:
    fprintf(stderr,"** ANALYZE header has weird dimension settings.\n");
    return 0;
  }

  dt = *((uint16_t *)(&header[70]));
  bp = *((uint16_t *)(&header[72]));

  if (ambe != be) { dt = swab_16(dt); bp = swab_16(bp); }

  if ((dt!=2 && dt!=4) || (bp!=8 && bp!=16)) {
    fprintf(stderr,"** this library can only read 8- and 16-bit integer data. DT signature=%d, bits/voxel=%d\n",dt,bp);
    return 0;
  }

  x = *((uint32_t *)(&header[108]));
  if (be != ambe) x = swab_32(x);
  // "plug-n-pray" float conversion
  fvoff = * ((float *) (&x));
  x = (int) fvoff;
  
  f = fopen(imgpath, "r");
  if (!f) {
    fprintf(stderr,"** unable to open %s\n",imgpath);
    return 0;
  }

  fseek(f,x,SEEK_SET);
  
  v=VolumeNew(w,h,d,vt_integer_16);
  if (dx != 0.0 && dy != 0.0 && dz != 0.0) {
    v->dx = dx;
    v->dy = dy;
    v->dz = dz;
  }

  if (!v) {
    fprintf(stderr,"** Unable to allocate volume.\n\n");
    return 0;
  }

  blk = (unsigned char *) MemAlloc(32768);
  if (!blk) {
    fprintf(stderr,"Unable to allocate memory.\n\n");
    return 0;
  }
  blk2 = (short int *) blk;

  switch(bp) {
    
  case 8:
    nblk = v->N / 32768;
    if (v->N % 32768) ++nblk;

    for(i=0;i<nblk;i++) {
      k = i*32768;
      bsz = 32768; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 1, bsz, f);

      for(j=0;j<bsz;j++)
	v->u.i16[k+j] = (i16_t) (blk[j]);

      if (i%10 == 0)
	SetProgress(i,nblk);

    }

    break;
  case 16:
    nblk = v->N / 16384;
    if (v->N % 16384) ++nblk;

    for(i=0;i<nblk;i++) {
      k = i*16384;
      bsz = 16384; if (k+bsz > v->N) bsz = v->N - k;

      fread(blk, 2, bsz, f);

      for(j=0;j<bsz;j++)
	v->u.i16[k+j] = (i16_t) conv_le16(blk2[j]);

      if (i%10 == 0)
	SetProgress(i,nblk);
    }
    break;
  default:
    fprintf(stderr,"Unsupported bpv: %d\n",bp);
    return 0;
  }
  
  MemFree(blk);
  VolumeYFlip(v);
  return v;
}

void pvt_mvf_write(XAnnVolume *v, char *name) {
  MVF_File *mvf;
  char *buf;
  int  i,j,m,n,q;
  int  nS, nM;

  nS = MapSize(v->addseeds);
  nM = MapSize(v->delseeds);

  mvf = mvf_create(name, v->W,v->H,v->D,v->dx,v->dy,v->dz);
  if (!mvf) return;

  mvf_add_chunk(mvf, "ORIG" ,       -1, MVF_CHUNK_16BIT);
  mvf_add_chunk(mvf, "VALUE",       -1, MVF_CHUNK_16BIT);
  mvf_add_chunk(mvf, "COST" ,       -1, MVF_CHUNK_16BIT);
  mvf_add_chunk(mvf, "LABEL",       -1, MVF_CHUNK_8BIT);
  mvf_add_chunk(mvf, "PRED" ,       -1, MVF_CHUNK_8BIT);
  mvf_add_chunk(mvf, "ADJ"  ,     2048, MVF_CHUNK_TEXT);
  mvf_add_chunk(mvf, "SEEDS", 16+24*nS, MVF_CHUNK_TEXT);
  mvf_add_chunk(mvf, "MARKS", 16+24*nM, MVF_CHUNK_TEXT);
  mvf_add_chunk(mvf, "VIEW" ,    16384, MVF_CHUNK_TEXT);
  mvf_deploy_chunks(mvf);

  /* write chunks */

  buf = (char *) MemAlloc(16384);
  if (!buf)
    goto close_bye;

  m=0;
  j = mvf_lookup_chunk(mvf,"ORIG");
  mvf_goto(mvf,j,0);

  for(i=0;i<v->N;i++) {
    if (m > 16382) {
      if (mvf_write_data(mvf,buf,m)!=m) goto free_close_bye;
      m=0;
    }
    * ((i16_t *) (&buf[m])) = conv_le16( v->vd[i].orig );
    m+=2;
  }
  if (m) { mvf_write_data(mvf,buf,m); m=0; }

  j = mvf_lookup_chunk(mvf,"VALUE");
  mvf_goto(mvf,j,0);

  for(i=0;i<v->N;i++) {
    if (m > 16382) {
      if (mvf_write_data(mvf,buf,m)!=m) goto free_close_bye;
      m=0;
    }
    * ((i16_t *) (&buf[m])) = conv_le16( v->vd[i].value );
    m+=2;
  }
  if (m) { mvf_write_data(mvf,buf,m); m=0; }

  j = mvf_lookup_chunk(mvf,"COST");
  mvf_goto(mvf,j,0);

  for(i=0;i<v->N;i++) {
    if (m > 16382) {
      if (mvf_write_data(mvf,buf,m)!=m) goto free_close_bye;
      m=0;
    }
    * ((i16_t *) (&buf[m])) = conv_le16( v->vd[i].cost );
    m+=2;
  }
  if (m) { mvf_write_data(mvf,buf,m); m=0; }

  j = mvf_lookup_chunk(mvf,"LABEL");
  mvf_goto(mvf,j,0);

  for(i=0;i<v->N;i++) {
    if (m > 16383) {
      if (mvf_write_data(mvf,buf,m)!=m) goto free_close_bye;
      m=0;
    }
    buf[m] = v->vd[i].label;
    ++m;
  }
  if (m) { mvf_write_data(mvf,buf,m); m=0; }

  j = mvf_lookup_chunk(mvf,"PRED");
  mvf_goto(mvf,j,0);

  for(i=0;i<v->N;i++) {
    if (m > 16383) {
      if (mvf_write_data(mvf,buf,m)!=m) goto free_close_bye;
      m=0;
    }
    buf[m] = v->vd[i].pred;
    ++m;
  }
  if (m) { mvf_write_data(mvf,buf,m); m=0; }

  j = mvf_lookup_chunk(mvf,"ADJ");
  mvf_fill_chunk(mvf,j,0);
  mvf_goto(mvf,j,0);

  mvf_write_int_string(mvf,16,v->predadj->n);
  for(i=0;i<v->predadj->n;i++) {
    mvf_write_int_string(mvf,8,v->predadj->dx[i]);
    mvf_write_int_string(mvf,8,v->predadj->dy[i]);
    mvf_write_int_string(mvf,8,v->predadj->dz[i]);
    mvf_write_int_string(mvf,16,v->predadj->dn[i]);
  }

  j = mvf_lookup_chunk(mvf,"SEEDS");
  mvf_goto(mvf,j,0);
  mvf_write_int_string(mvf,16,nS);
  for(i=0;i<nS;i++) {
    MapGet(v->addseeds, i, &n, &q);
    mvf_write_int_string(mvf,16,n);
    mvf_write_int_string(mvf,8,q);
  }

  j = mvf_lookup_chunk(mvf,"MARKS");
  mvf_goto(mvf,j,0);
  mvf_write_int_string(mvf,16,nM);
  for(i=0;i<nM;i++) {
    MapGet(v->delseeds, i, &n, &q);
    mvf_write_int_string(mvf,16,n);
    mvf_write_int_string(mvf,8,q);
  }

  // VIEW (empty)
  j = mvf_lookup_chunk(mvf,"VIEW");
  mvf_fill_chunk(mvf,j,0);

 free_close_bye:
  MemFree(buf);
 close_bye:
  mvf_close(mvf);
}

XAnnVolume * pvt_mvf_read(char *name)
{
  XAnnVolume *v;
  MVF_File *mvf;
  int i,j,ur,ib,r,s,t;
  char *buf = 0;

  mvf = mvf_open(name);
  if (!mvf) return 0;

  v = XAVolNew(mvf->header.W,mvf->header.H,mvf->header.D);
  if (!v) {
    mvf_close(mvf);
    return 0;
  }

  v->dx = mvf->header.dx;
  v->dy = mvf->header.dy;
  v->dz = mvf->header.dz;

  buf = (char *) MemAlloc(16384);
  if (!buf) {
    mvf_close(mvf);
    XAVolDestroy(v);
    return 0;
  }

  j = mvf_lookup_chunk(mvf,"ORIG");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);

  i=0; ur=v->N;
  while(ur) {
    ib = (ur > 8192) ? 8192 : ur;
    mvf_read_data(mvf,buf,ib*2);
    for(t=0;t<ib;t++)
      v->vd[i+t].orig = conv_le16( * ((i16_t *)(&buf[2*t])) );
    i +=ib;
    ur-=ib;
  }

  j = mvf_lookup_chunk(mvf,"VALUE");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);
  i=0; ur=v->N;
  while(ur) {
    ib = (ur > 8192) ? 8192 : ur;
    mvf_read_data(mvf,buf,ib*2);
    for(t=0;t<ib;t++)
      v->vd[i+t].value = conv_le16( * ((i16_t *)(&buf[2*t])) );
    i +=ib;
    ur-=ib;
  }

  j = mvf_lookup_chunk(mvf,"COST");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);
  i=0; ur=v->N;
  while(ur) {
    ib = (ur > 8192) ? 8192 : ur;
    mvf_read_data(mvf,buf,ib*2);
    for(t=0;t<ib;t++)
      v->vd[i+t].cost = conv_le16( * ((i16_t *)(&buf[2*t])) );
    i +=ib;
    ur-=ib;
  }

  j = mvf_lookup_chunk(mvf,"LABEL");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);
  i=0; ur=v->N;
  while(ur) {
    ib = (ur > 16384) ? 16384 : ur;
    mvf_read_data(mvf,buf,ib);
    for(t=0;t<ib;t++)
      v->vd[i+t].label = buf[t];
    i +=ib;
    ur-=ib;
  }

  j = mvf_lookup_chunk(mvf,"PRED");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);
  i=0; ur=v->N;
  while(ur) {
    ib = (ur > 16384) ? 16384 : ur;
    mvf_read_data(mvf,buf,ib);
    for(t=0;t<ib;t++)
      v->vd[i+t].pred = buf[t];
    i +=ib;
    ur-=ib;
  }

  j = mvf_lookup_chunk(mvf,"ADJ");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);

  t = mvf_read_int_string(mvf,16);
  if (v->predadj)
    DestroyAdjRel(&(v->predadj));

  v->predadj=CreateAdjRel(t);

  for(i=0;i<t;i++) {
    v->predadj->dx[i] = mvf_read_int_string(mvf,8);
    v->predadj->dy[i] = mvf_read_int_string(mvf,8);
    v->predadj->dz[i] = mvf_read_int_string(mvf,8);
    v->predadj->dn[i] = mvf_read_int_string(mvf,16);
  }

  j = mvf_lookup_chunk(mvf,"SEEDS");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);

  t = mvf_read_int_string(mvf,16);
  for(i=0;i<t;i++) {
    r = mvf_read_int_string(mvf,16);
    s = mvf_read_int_string(mvf,8);
    MapSetUnique(v->addseeds,r,s);
  }

  j = mvf_lookup_chunk(mvf,"MARKS");
  if (j<0) goto bad_format;
  mvf_goto(mvf,j,0);

  t = mvf_read_int_string(mvf,16);
  for(i=0;i<t;i++) {
    r = mvf_read_int_string(mvf,16);
    s = mvf_read_int_string(mvf,8);
    MapSetUnique(v->delseeds,r,s);
  }

  mvf_close(mvf);
  MemFree(buf);
  return v;

 bad_format:
  fprintf(stderr,"** bad MVF file\n");
  mvf_close(mvf);
  XAVolDestroy(v);
  if (buf)
    MemFree(buf);
  return 0;
}

/* old format */
XAnnVolume * pvt_vla_read(char *name) 
{
  XAnnVolume *v;
  FILE *f;
  int version;
  int a,b,c,d,i,j,k;
  int bsz, nblk;
  char *blk;

  f=fopen(name,"r");
  if (!f) return 0;

  // HEADER

  if (fscanf(f,"VLA\n%d\n",&version)!=1) {
    fprintf(stderr,"** not a VLA file\n");
    return 0;
  }

  if (version < 3) {
    fprintf(stderr,"sorry, support for VLA version 1 and 2 was removed.\n");
    return 0;
  }

  // DIMENSIONS

  if (fscanf(f," %d %d %d \n",&a,&b,&c)!=3)
    return 0;

  v=XAVolNew(a,b,c);

  // ADD SEEDS

  if (fscanf(f,"aseeds = %d \n",&j)!=1) {
    XAVolDestroy(v);
    return 0;
  }

  for(i=0;i<j;i++) {
    if (fscanf(f," %d %d \n",&a,&b)!=2) {
      XAVolDestroy(v);
      return 0;
    }
    MapSetUnique(v->addseeds,a,b);
  }

  // DEL SEEDS

  if (fscanf(f,"dseeds = %d \n",&j)!=1) {
    XAVolDestroy(v);
    return 0;
  }

  for(i=0;i<j;i++) {
    if (fscanf(f," %d %d \n",&a,&b)!=2) {
      XAVolDestroy(v);
      return 0;
    }
    MapSetUnique(v->delseeds,a,b);
  }

  // PREDECESSOR ADJACENCY

  if (fscanf(f,"adj = %d \n",&j)!=1) {
    XAVolDestroy(v);
    return 0;
  }
  if (v->predadj)
    DestroyAdjRel(&(v->predadj));

  v->predadj=CreateAdjRel(j);

  for(i=0;i<j;i++) {
    if (fscanf(f," %d %d %d %d \n",&a,&b,&c,&d)!=4) {
      XAVolDestroy(v);
      return 0;
    }
    v->predadj->dx[i] = a;
    v->predadj->dy[i] = b;
    v->predadj->dz[i] = c;
    v->predadj->dn[i] = d;
  }

  // IMAGES: ORIG, VALUE, LABEL, PRED, COST [16bit, little endian]

  blk = (char *) MemAlloc(9 * 8192);
  if (!blk) {
    fprintf(stderr,"** unable to allocate memory\n");
    exit(5);
  }

  nblk = v->N / 8192;
  if (v->N % 8192) ++nblk;

  for(i=0;i<nblk;i++) {
    k = i * 8192;
    bsz = 8192; /* voxels in this block */
    if (k+bsz > v->N)
      bsz = v->N - k;
    // prepare 8K voxels on buffer
    MemSet(blk,0,9*8192);
    fread(blk, 9, bsz, f);

    for(j=0;j<bsz;j++) {
      v->vd[k+j].orig  = *((vtype *)((void *)(blk+j*9+0)));
      v->vd[k+j].value = *((vtype *)((void *)(blk+j*9+2)));
      v->vd[k+j].cost  = *((ctype *)((void *)(blk+j*9+4)));
      v->vd[k+j].label = *((ltype *)((void *)(blk+j*9+7)));
      v->vd[k+j].pred  = *((ltype *)((void *)(blk+j*9+8)));
    }
    //printf("W block %d of %d (%d%%)\n",i,nblk-1,(100*i)/(nblk-1));
  }

  fclose(f);
  MemFree(blk);
  return v;
}

Volume *VolumeInterpolate(Volume *src, float dx, float dy, float dz)
{
  Volume *dest, *tmp;
  int value;
  int i;
  Voxel P,Q,R; /* previous, current, and next voxel */
  float min;
  float walked_dist,dist_PQ;

  /* The default voxel sizes of the output scene should be dx=dy=dz=min(dx,dy,dz) */

  if ((dx == 0.0) || (dy == 0.0) || (dz == 0.0)) {
    min = src->dx;
    if (src->dy < min)
      min = src->dy;
    if (src->dz < min)
      min = src->dz;
    dx = min; dy = min; dz = min;
    if (min <= 0)
      return 0;
  }

  /* If there is no need for resampling then returns input scene */
  if ((dx == src->dx) && (dy == src->dy) && (dz == src->dz)) {
    dest = VolumeNew(src->W,src->H,src->D,vt_integer_16);
    dest->dx = dx;
    dest->dy = dy;
    dest->dz = dz;
    for(i=0;i<src->N;i++)
      dest->u.i16[i] = src->u.i16[i];
    return (dest);
  } else {
  /* Else the working image is the input image */
    dest = src;
  }

  /* Resample in x */

  if (dx != src->dx) {
    tmp = VolumeNew( (int)((float)(dest->W-1)*dest->dx/dx)+1,
		     dest->H,
		     dest->D, vt_integer_16);
    for(Q.x=0; Q.x < tmp->W; Q.x++)
      for(Q.z=0; Q.z < tmp->D; Q.z++)
        for(Q.y=0; Q.y < tmp->H; Q.y++) {

	  walked_dist = (float)Q.x * dx; /* the walked distance so far */
	  
	  P.x = (int)(walked_dist/src->dx); /* P is the previous pixel in the
					       original scene */
	  P.y = Q.y;
	  P.z = Q.z;
	  
	  R.x = P.x + 1; /* R is the next pixel in the original
			    image. Observe that Q is in between P
			    and R. */
	  R.y = P.y;
	  R.z = P.z;
	  
	  dist_PQ =  walked_dist - (float)P.x * src->dx;  /* the distance between P and Q */

	  /* interpolation: P --- dPQ --- Q ---- dPR-dPQ ---- R

	     I(Q) = (I(P)*(dPR-dPQ) + I(R)*dPQ) / dPR
	     
	  */

	  value = (int)((( src->dx - dist_PQ)*
			 (float)VolumeGetI16(dest,P.x, P.y, P.z) + 
			 dist_PQ * (float)VolumeGetI16(dest,R.x,R.y,R.z)) /
			src->dx);
	  VolumeSetI16(tmp,Q.x,Q.y,Q.z,value);
	}
    dest=tmp;
  }

  /* Resample in y */

  if (dy != src->dy) {
    tmp = VolumeNew(dest->W, 
		    (int)(((float)dest->H-1.0) * src->dy / dy)+1,
		    dest->D, vt_integer_16);
    for(Q.y=0; Q.y < tmp->H; Q.y++)
      for(Q.z=0; Q.z < tmp->D; Q.z++)
          for(Q.x=0; Q.x < tmp->W; Q.x++) {

            walked_dist = (float)Q.y * dy;

            P.x = Q.x;
            P.y = (int)(walked_dist/src->dy);
            P.z = Q.z;

            R.x = P.x;
            R.y = P.y + 1;
            R.z = P.z;

            dist_PQ =  walked_dist - (float)P.y * src->dy;

            value = (int)(((src->dy - dist_PQ)*
			   (float)VolumeGetI16(dest,P.x,P.y,P.z) + 
			   dist_PQ * (float)VolumeGetI16(dest,R.x,R.y,R.z)) / 
			  src->dy);
	    VolumeSetI16(tmp,Q.x,Q.y,Q.z,value);
          }
    if (dest != src)
      VolumeDestroy(dest);
    dest=tmp;
  }

  /* Resample in z */

  if (dz != src->dz) {
    tmp = VolumeNew(dest->W,
		    dest->H,
		    (int)(((float)dest->D-1.0) * src->dz / dz)+1,
		    vt_integer_16);
    for(Q.z=0; Q.z < tmp->D; Q.z++)
        for(Q.y=0; Q.y < tmp->H; Q.y++)
          for(Q.x=0; Q.x < tmp->W; Q.x++) {

            walked_dist = (float)Q.z * dz;

            P.x = Q.x;
            P.y = Q.y;
            P.z = (int)(walked_dist/src->dz);

            R.x = P.x;
            R.y = P.y;
            R.z = P.z + 1;

            dist_PQ =  walked_dist - (float)P.z * src->dz;

	    value = (int)(((src->dz - dist_PQ)*
			   (float)VolumeGetI16(dest,P.x,P.y,P.z) + 
			   dist_PQ * (float)VolumeGetI16(dest,R.x,R.y,R.z)) / 
			  src->dz) ;
	    VolumeSetI16(tmp,Q.x,Q.y,Q.z,value);
	  }
    if (dest != src)
      VolumeDestroy(dest);
    dest=tmp;
  }
 
  dest->dx=dx;
  dest->dy=dy;
  dest->dz=dz;
  return(dest);
}

Volume *SENew(int w,int h,int d) {
  Volume *se;
  se = VolumeNew(w,h,d,vt_integer_8);
  return(se);
}

void    SEDestroy(Volume *se) {
  VolumeDestroy(se);
}

Volume *SECross(int size) {
  Volume *se;
  int i;
  se = SENew(size,size,size);

  for(i=0;i<size;i++) {
    VolumeSetI8(se, i, size/2, size/2, 1);
    VolumeSetI8(se, size/2, i, size/2, 1);
    VolumeSetI8(se, size/2, size/2, i, 1);
  }

  return se;
}

Volume *SEBox(int size) {
  Volume *se;
  int i;
  se = SENew(size,size,size);

  for(i=0;i<se->N;i++)
    se->u.i8[i] = 1;

  return se;
}

Volume *SELine(int size,int direction) {
  Volume *se;
  int i;
  int sx,sy,sz,dx,dy,dz,x,y,z;
  se = SENew(size,size,size);
  
  switch(direction) {

    // x,y,z lines
  case 0: sx=0; sy=size/2; sz=size/2; dx=1; dy=0; dz=0; break; 
  case 1: sx=size/2; sy=0; sz=size/2; dx=0; dy=1; dz=0; break;
  case 2: sx=size/2; sy=size/2; sz=0; dx=0; dy=0; dz=1; break;
    
    // xy lines
  case 3: sx=0; sy=size-1; sz=size/2; dx=1; dy=-1; dz=0; break;
  case 4: sx=0; sy=0; sz=size/2; dx=1; dy=1; dz=0; break;
    
    // xz lines
  case 5: sx=0; sy=size/2; sz=size-1; dx=1; dy=0; dz=-1; break;
  case 6: sx=0; sy=size/2; sz=0; dx=1; dy=0; dz=1; break;

    // yz lines
  case 7: sx=size/2; sy=0; sz=size-1; dx=0; dy=1; dz=-1; break;
  case 8: sx=size/2; sy=0; sz=0; dx=0; dy=1; dz=1; break;

    // xyz lines
  case 9:  sx=0; sy=0; sz=0; dx=1; dy=1; dz=1; break;
  case 10: sx=size-1; sy=size-1; sz=0; dx=-1; dy=-1; dz=1; break;
  case 11: sx=0; sy=size-1; sz=0; dx=1; dy=-1; dz=1; break;
  case 12: sx=size-1; sy=0; sz=0; dx=-1; dy=1; dz=1; break;

  default:
    fprintf(stderr,"bad direction for SELine: %d\n",direction);
    return 0;
  }
  
  x = sx;
  y = sy;
  z = sz;

  for(i=0;i<size;i++) {
    VolumeSetI8(se,x,y,z,1);
    x+=dx;
    y+=dy;
    z+=dz;
  }

  return se;
}

void SEOr(Volume *dest, Volume *src) {
  int i;
  if (dest->N != src->N) return;
  for(i=0;i<dest->N;i++)
    if (src->u.i8[i] != 0)
      dest->u.i8[i] = 1;
}

void SEAnd(Volume *dest, Volume *src) {
  int i;
  if (dest->N != src->N) return;
  for(i=0;i<dest->N;i++)
    if (src->u.i8[i] == 0)
      dest->u.i8[i] = 0;  
}

void    VolumeYFlip(Volume *v) {
  int i,j,H2,Wlen,H;
  i16_t *tmp;
  int *tby,*tbz;

  if (v->Type != vt_integer_16) return;

  tmp = (i16_t *) MemAlloc(v->W * 2);

  H = v->H - 1;
  H2 = v->H / 2;
  tby = v->tbrow;
  tbz = v->tbframe;
  Wlen = v->W * 2;

  for(i=0;i<v->D;i++) {
    for(j=0;j<H2;j++) {
      MemCpy(tmp, &(v->u.i16[tbz[i]+tby[j]]),Wlen);
      MemCpy(&(v->u.i16[tbz[i]+tby[j]]),
	     &(v->u.i16[tbz[i]+tby[H-j]]),
	     Wlen);
      MemCpy(&(v->u.i16[tbz[i]+tby[H-j]]),tmp,Wlen);
    }
  }

  MemFree(tmp);
}
