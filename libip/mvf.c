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
#include <unistd.h>
#include "mvf.h"

/* internal functions */
void mvf_write_header(MVF_File *mvf);
void mvf_write_chunk_headers(MVF_File *mvf);
void mvf_write_chunk_header(MVF_File *mvf, int index);

void mvf_read_header(MVF_File *mvf);
void mvf_read_chunk_headers(MVF_File *mvf);
void mvf_read_chunk_header(MVF_File *mvf, int index);

MVF_File * mvf_create(char *path, int w,int h,int d,
		      float dx,float dy,float dz) 
{
  MVF_File *mvf;

  mvf = (MVF_File *) malloc(sizeof(MVF_File));
  if (!mvf) return 0;

  mvf->path = (char *) malloc(strlen(path)+1);
  if (!mvf->path) { 
    free(mvf); 
    return 0; 
  }
  strcpy(mvf->path,path);

  mvf->rwmode = 1;
  mvf->f = fopen(mvf->path, "w");
  if (!mvf->f) {
    free(mvf->path);
    free(mvf);
    return 0;
  }

  mvf->chunks = 0;
  mvf->header.W  = w;
  mvf->header.H  = h;
  mvf->header.D  = d;
  mvf->header.dx = dx;
  mvf->header.dy = dy;
  mvf->header.dz = dz;
  mvf->header.nchunks = 0;

  mvf_write_header(mvf);
  return(mvf);
}


void       mvf_add_chunk(MVF_File *mvf, char *chunk_name, 
			 int length, int chunk_type)
{
  int n;
  MVF_Chunk_Header *h;

  n = mvf->header.nchunks + 1;

  if (n==1)
    mvf->chunks = (MVF_Chunk_Header *) malloc(sizeof(MVF_Chunk_Header)*n);
  else
    mvf->chunks = (MVF_Chunk_Header *) realloc(mvf->chunks,
					       sizeof(MVF_Chunk_Header)*n);
  if (!mvf->chunks)
    return;
  
  mvf->header.nchunks++;
  h = &(mvf->chunks[n-1]);
  memset(h,0,sizeof(MVF_Chunk_Header));

  if (strlen(chunk_name) > 31) {
    memcpy(h->name, chunk_name, 32);
    h->name[31] = 0;
  } else {
    strcpy(h->name, chunk_name);
  }

  if (length == -1) {
    if (chunk_type == MVF_CHUNK_8BIT)
      length = mvf->header.W * mvf->header.H * mvf->header.D;
    if (chunk_type == MVF_CHUNK_16BIT)
      length = 2 * mvf->header.W * mvf->header.H * mvf->header.D;
  }

  h->length     = length;
  h->chunk_type = chunk_type;
  h->offset     = -1;
}

void mvf_deploy_chunks(MVF_File *mvf) 
{
  int i,offset;

  if (mvf->rwmode != 1)
    return;

  offset = 88+(mvf->header.nchunks*104);
  for(i=0;i<mvf->header.nchunks;i++) {
    mvf->chunks[i].offset = offset;
    offset += mvf->chunks[i].length;
  }

  mvf_write_header(mvf);
  mvf_write_chunk_headers(mvf);
}

int mvf_lookup_chunk(MVF_File *mvf, char *chunk_name)
{
  int i;
  for(i=0;i<mvf->header.nchunks;i++)
    if (!strcmp(chunk_name,mvf->chunks[i].name))
      return i;
  return -1;
}

int mvf_write_chunk_data(MVF_File *mvf, int chunk_index,
			 int offset, void *ptr, int bytes)
{
  if (mvf_goto(mvf,chunk_index,offset)!=0)
    return 0;
  return(mvf_write_data(mvf,ptr,bytes));
}

int mvf_write_data(MVF_File *mvf, void *ptr, int bytes) {
  char *p;
  int q,r,t;
  q=bytes;
  p=(char *)ptr;
  t=0;

  if (mvf->rwmode == 0)
    return 0; /* FIXME */

  while(q) {
    r = fwrite(p, 1, q, mvf->f);
    if (r <= 0)
      return 0;
    q -= r;
    t += r;
    p += r;
  }
  return t;
}

void mvf_fill_chunk(MVF_File *mvf, int chunk_index, char byte) {
  char  p[1024];
  int   q,r;

  if (mvf->rwmode == 0)
    return; /* FIXME */

  fseek(mvf->f, mvf->chunks[chunk_index].offset, SEEK_SET);
  memset(p,byte,1024);

  q = mvf->chunks[chunk_index].length;
  while(q) {
    r = fwrite(p, 1, (q>1024)?1024:q, mvf->f);
    q -= r;
  }
}

int mvf_read_chunk_data(MVF_File *mvf, int chunk_index,
			int offset, void *ptr, int bytes)
{
  if (mvf_goto(mvf,chunk_index,offset)!=0)
    return 0;
  return(mvf_read_data(mvf,ptr,bytes));
}

int mvf_read_data(MVF_File *mvf, void *ptr, int bytes) {
  char *p;
  int   q,r,t;

  if (mvf->rwmode == 1)
    return 0; /* FIXME */

  q=bytes;
  t=0;
  p=(char *)ptr;

  while(q) {
    r = fread(p, 1, q, mvf->f);
    if (r <= 0)
      return 0;
    q -= r;
    t += r;
    p += r;
  }
  return t;
}

void mvf_write_int_string(MVF_File *mvf, int fieldlen, int value) 
{
  char tmp[64],*aux=0,*p;
  
  if (mvf->rwmode == 0)
    return; /* FIXME */

  if (fieldlen<=64) {
    p = tmp;
  } else {
    aux = (char *) malloc(fieldlen);
    if (!aux) return;

    p = aux;
  }
  memset(p, 0, fieldlen);
  sprintf(p, "%d", value);
  *(p+fieldlen-1) = 0;
  fwrite(p,1,fieldlen,mvf->f);
  if (aux)
    free(aux);
}

void mvf_write_float_string(MVF_File *mvf, int fieldlen, float value)
{
  char tmp[64],*aux=0,*p;
  
  if (mvf->rwmode == 0)
    return; /* FIXME */

  if (fieldlen<=64) {
    p = tmp;
  } else {
    aux = (char *) malloc(fieldlen);
    if (!aux) return;

    p = aux;
  }
  memset(p, 0, fieldlen);
  sprintf(p, "%.6f", value);
  *(p+fieldlen-1) = 0;
  fwrite(p,1,fieldlen,mvf->f);
  if (aux)
    free(aux);
}

void mvf_write_string(MVF_File *mvf, int fieldlen, char *value)
{
  char tmp[64],*aux=0,*p;
  
  if (mvf->rwmode == 0)
    return; /* FIXME */

  if (fieldlen<=64) {
    p = tmp;
  } else {
    aux = (char *) malloc(fieldlen);
    if (!aux) return;

    p = aux;
  }
  memset(p, 0, fieldlen);
  if (strlen(value) < fieldlen)
    strcpy(p,value);
  else {
    memcpy(p,value,fieldlen);
    *(p+fieldlen-1) = 0;
  }
  fwrite(p,1,fieldlen,mvf->f);
  if (aux)
    free(aux);
}

int        mvf_read_int_string(MVF_File *mvf, int fieldlen)
{
  int v;
  int i;
  char tmp[64],*aux=0,*p;

  if (mvf->rwmode == 1)
    return -1; /* FIXME */

  if (fieldlen <= 64) {
    p = tmp;
  } else {
    aux = (char *) malloc(fieldlen);
    if (!aux) return -1;

    p = aux;
  }

  i = fread(p,1,fieldlen,mvf->f);
  if (i != fieldlen) return -1;
  v = atoi(p);
  if (aux)
    free(aux);
  return v;
}

float      mvf_read_float_string(MVF_File *mvf, int fieldlen)
{
  float v;
  int i;
  char tmp[64],*aux=0,*p;

  if (mvf->rwmode == 1)
    return 0.0; /* FIXME */

  if (fieldlen <= 64) {
    p = tmp;
  } else {
    aux = (char *) malloc(fieldlen);
    if (!aux) return 0.0;

    p = aux;
  }

  i = fread(p,1,fieldlen,mvf->f);
  if (i != fieldlen) return -1;
  v = atof(p);
  if (aux)
    free(aux);
  return v;
}

void       mvf_read_string(MVF_File *mvf, int fieldlen, char *dest)
{
  if (mvf->rwmode == 1)
    return; /* FIXME */

  fread(dest,1,fieldlen,mvf->f);
}

int mvf_goto(MVF_File *mvf, int chunk_index, int offset) {
  return(fseek(mvf->f, 
	       mvf->chunks[chunk_index].offset + offset, 
	       SEEK_SET));
}

int mvf_skip(MVF_File *mvf, int offset) {
  return(fseek(mvf->f, offset, SEEK_CUR));
}

void       mvf_close(MVF_File *mvf)
{
  fclose(mvf->f);
  if (mvf->chunks)
    free(mvf->chunks);
  free(mvf->path);
  free(mvf);
}

int        mvf_check_format(char *path) {
  FILE *f;
  char buf[5];
  f = fopen(path,"r");
  if (!f) return 0;
  if (fread(buf,1,5,f)!=5) {
    fclose(f);
    return 0;
  }
  fclose(f);
  
  buf[4] = 0;
  return(strcmp(buf,"MVF\n")==0);
}

MVF_File * mvf_open(char *path) {
  MVF_File * mvf;

  if (!mvf_check_format(path))
    return 0;

  mvf = (MVF_File *) malloc(sizeof(MVF_File));
  if (!mvf) return 0;

  mvf->path = (char *) malloc(strlen(path)+1);
  if (!mvf->path) { 
    free(mvf); 
    return 0; 
  }
  strcpy(mvf->path,path);

  mvf->f = fopen(mvf->path, "r");
  mvf->rwmode = 0;

  if (!mvf->f) {
    free(mvf->path);
    free(mvf);
    return 0;
  }

  mvf->chunks = 0;
  mvf_read_header(mvf);
  mvf_read_chunk_headers(mvf);

  return mvf;
}

MVF_File * mvf_open_rw(char *path) {
  MVF_File * mvf;

  if (!mvf_check_format(path))
    return 0;

  mvf = (MVF_File *) malloc(sizeof(MVF_File));
  if (!mvf) return 0;

  mvf->path = (char *) malloc(strlen(path)+1);
  if (!mvf->path) { 
    free(mvf); 
    return 0; 
  }
  strcpy(mvf->path,path);

  mvf->f = fopen(mvf->path, "r+");
  mvf->rwmode = 2;

  if (!mvf->f) {
    free(mvf->path);
    free(mvf);
    return 0;
  }

  mvf->chunks = 0;
  mvf_read_header(mvf);
  mvf_read_chunk_headers(mvf);

  return mvf;
}

/* ---------- */

void mvf_write_header(MVF_File *mvf) {
  char hbuf[88];
  
  if (mvf->rwmode == 0)
    return; /* FIXME */

  memset(hbuf,0,88);

  strcpy(hbuf,"MVF\n");
  sprintf(&hbuf[8] ,"%d",mvf->header.W);
  sprintf(&hbuf[16],"%d",mvf->header.H);
  sprintf(&hbuf[24],"%d",mvf->header.D);
  sprintf(&hbuf[32],"%.6f",mvf->header.dx);
  sprintf(&hbuf[48],"%.6f",mvf->header.dy);
  sprintf(&hbuf[64],"%.6f",mvf->header.dz);
  sprintf(&hbuf[80],"%d",mvf->header.nchunks);
  fseek(mvf->f, 0, SEEK_SET);
  fwrite(hbuf,1,88,mvf->f);
}

void mvf_read_header(MVF_File *mvf) {
  char hbuf[88];

  if (mvf->rwmode == 1)
    return; /* FIXME */

  fseek(mvf->f, 0, SEEK_SET);
  if (fread(hbuf,1,88,mvf->f) != 88)
    return;

  mvf->header.W = atoi(&hbuf[8]);
  mvf->header.H = atoi(&hbuf[16]);
  mvf->header.D = atoi(&hbuf[24]);

  mvf->header.dx = atof(&hbuf[32]);
  mvf->header.dy = atof(&hbuf[48]);
  mvf->header.dz = atof(&hbuf[64]);

  mvf->header.nchunks = atoi(&hbuf[80]);
  mvf->chunks = 0;
}

void mvf_write_chunk_header(MVF_File *mvf, int index) {
  char hbuf[104];

  if (mvf->rwmode == 0)
    return; /* FIXME */
  if (index<0 || index >= mvf->header.nchunks)
    return;

  memset(hbuf,0,104);
  memcpy(hbuf,mvf->chunks[index].name,32);
  sprintf(&hbuf[32],"%d",mvf->chunks[index].chunk_type);
  sprintf(&hbuf[40],"%d",mvf->chunks[index].offset);
  sprintf(&hbuf[72],"%d",mvf->chunks[index].length);
  fseek(mvf->f, 88+(104*index), SEEK_SET);
  fwrite(hbuf,1,104,mvf->f);
}

void mvf_read_chunk_header(MVF_File *mvf, int index) {
  char hbuf[104];

  if (mvf->rwmode == 1)
    return; /* FIXME */
  if (index<0 || index >= mvf->header.nchunks)
    return;

  fseek(mvf->f, 88+(104*index), SEEK_SET);
  if (fread(hbuf,1,104,mvf->f)!=104) return;

  memcpy(mvf->chunks[index].name, hbuf, 32);
  mvf->chunks[index].chunk_type = atoi(&hbuf[32]);
  mvf->chunks[index].offset     = atoi(&hbuf[40]);
  mvf->chunks[index].length     = atoi(&hbuf[72]);
}

void mvf_write_chunk_headers(MVF_File *mvf) {
  int i;
  for(i=0;i<mvf->header.nchunks;i++)
    mvf_write_chunk_header(mvf,i);
}

void mvf_read_chunk_headers(MVF_File *mvf) {
  int i;
  if (mvf->chunks)
    free(mvf->chunks);
  i = sizeof(MVF_Chunk_Header) * mvf->header.nchunks;
  mvf->chunks = (MVF_Chunk_Header *) malloc(i);
  if (!mvf->chunks)
    return;
  memset(mvf->chunks,0,i);
  for(i=0;i<mvf->header.nchunks;i++)
    mvf_read_chunk_header(mvf,i);
}
