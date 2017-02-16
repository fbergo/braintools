

#ifndef FIO_H
#define FIO_H 1

#include <stdio.h>

int fio_swab_16(int val);
int fio_swab_32(int val);

int   fio_abs_read_8(FILE *f, long offset);
int   fio_abs_read_16(FILE *f, long offset, int is_big_endian);
int   fio_abs_read_32(FILE *f, long offset, int is_big_endian);
float fio_abs_read_float32(FILE *f, long offset, int is_big_endian);

int   fio_read_8(FILE *f);
int   fio_read_16(FILE *f, int is_big_endian);
int   fio_read_32(FILE *f, int is_big_endian);
float fio_read_float32(FILE *f, int is_big_endian);

int  fio_abs_write_8(FILE *f, long offset, int val);
int  fio_abs_write_16(FILE *f, long offset, int is_big_endian, int val);
int  fio_abs_write_32(FILE *f, long offset, int is_big_endian, int val);
int  fio_abs_write_float32(FILE *f, long offset, int is_big_endian, float val);

int  fio_abs_write_zeroes(FILE *f, long offset, int n);

int  fio_write_8(FILE *f, int val);
int  fio_write_16(FILE *f, int is_big_endian, int val);
int  fio_write_32(FILE *f, int is_big_endian, int val);
int  fio_write_float32(FILE *f, int is_big_endian, float val);

int  fio_write_zeroes(FILE *f, int n);


#endif
