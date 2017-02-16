
/* simple file i/o functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fio.h"

static int fio_big_endian = -1;

void fio_init() {
  short int x;
  char *p;

  x = 0x0001;
  p = (char *) (&x);

  if ((*p) == 0)
    fio_big_endian = 1;
  else
    fio_big_endian = 0;
}

int fio_swab_16(int val) {
  int x;
  val &= 0xffff;
  x  = (val & 0xff) << 8;
  x |= (val >> 8);
  return x;
}

int fio_swab_32(int val) {
  int x;
  val &= 0xffffffff;
  x  = (val & 0xff) << 24;
  x |= ((val >> 8) & 0xff) << 16;
  x |= ((val >> 16) & 0xff) << 8;
  x |= ((val >> 24) & 0xff);
  return x;
}

int   fio_abs_read_8(FILE *f, long offset) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_read_8(f));
}

int fio_abs_read_16(FILE *f, long offset, int is_big_endian) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_read_16(f,is_big_endian));
}

int fio_abs_read_32(FILE *f, long offset, int is_big_endian) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_read_32(f,is_big_endian));
}

float fio_abs_read_float32(FILE *f, long offset, int is_big_endian) {
  int val;
  float *p, q;
  val = fio_abs_read_32(f,offset,is_big_endian);
  p = (float *) (&val);
  q = *p;
  return q;
}

int   fio_read_8(FILE *f) {
  char val;

  if (fio_big_endian < 0) fio_init();
  if (fread(&val,1,1,f)!=1) return -1;

  return((int)val);
}

int   fio_read_16(FILE *f, int is_big_endian) {
  short int val;

  if (fio_big_endian < 0) fio_init();
  if (fread(&val,1,2,f)!=2) return -1;

  if ( (is_big_endian!=0) != (fio_big_endian!=0) )
    val = (short int) fio_swab_16(val);
  return ((int)val);
}

int   fio_read_32(FILE *f, int is_big_endian) {
  int val;

  if (fio_big_endian < 0) fio_init();
  if (fread(&val,1,4,f)!=4) return -1;

  if ( (is_big_endian!=0) != (fio_big_endian!=0) )
    val = (short int) fio_swab_32(val);
  return val;
}

float fio_read_float32(FILE *f, int is_big_endian) {
  int val;
  float *p, q;
  val = fio_read_32(f,is_big_endian);
  p = (float *) (&val);
  q = *p;
  return q;
}

int fio_abs_write_8(FILE *f, long offset, int val) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_write_8(f,val));
}

int fio_abs_write_16(FILE *f, long offset, int is_big_endian, int val) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_write_16(f,is_big_endian,val));
}

int fio_abs_write_32(FILE *f, long offset, int is_big_endian, int val) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_write_32(f,is_big_endian,val));
}

int fio_abs_write_float32(FILE *f, long offset, int is_big_endian, float val) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_write_float32(f,is_big_endian,val));
}

int   fio_write_8(FILE *f, int val) {
  char v;
  if (fio_big_endian < 0) fio_init();
  v = (char) (val & 0xff);
  if (fwrite(&v,1,1,f)!=1) return -1;
  return 0;
}

int   fio_write_16(FILE *f, int is_big_endian, int val) {
  short int v;
  if (fio_big_endian < 0) fio_init();
  v = (short int) (val & 0xffff);
  if ( (is_big_endian!=0) != (fio_big_endian!=0) )
    v = (short int) fio_swab_16(v);
  if (fwrite(&v,1,2,f)!=2) return -1;
  return 0;
}

int   fio_write_32(FILE *f, int is_big_endian, int val) {
  int v = val;
  if (fio_big_endian < 0) fio_init();
  if ( (is_big_endian!=0) != (fio_big_endian!=0) )
    v = fio_swab_32(v);
  if (fwrite(&v,1,4,f)!=4) return -1;
  return 0;
}

int fio_write_float32(FILE *f, int is_big_endian, float val) {
  float v, *w;
  int i, *j;
  if (fio_big_endian < 0) fio_init();
  v = val;
  if ( (is_big_endian!=0) != (fio_big_endian!=0) ) {
    j = (int *) (&v);
    i = *j;
    i = fio_swab_32(i);
    w = (float *) j;
    v = *w;
  }
  if (fwrite(&v,1,4,f)!=4) return -1;
  return 0;
}

int  fio_abs_write_zeroes(FILE *f, long offset, int n) {
  if (fseek(f,offset,SEEK_SET)!=0) return -1;
  return(fio_write_zeroes(f,n));
}

int  fio_write_zeroes(FILE *f, int n) {
  while (n>0) {
    if (n>=4) {
      if (fio_write_32(f, 0, 0) != 0) return -1;
      n -= 4;
    } else if (n>=2) {
      if (fio_write_16(f, 0, 0) != 0) return -1;
      n -= 2;
    } else {
      if (fio_write_8(f, 0) != 0) return -1;
      n--;
    }
  }
  return 0;
}
