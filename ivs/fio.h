/*
   dicom2scn and associated tools

   (C) 2003-2010 Felipe P.G. Bergo <bergo@liv.ic.unicamp.br>

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
