
#ifndef MVF_H
#define MVF_H 1

#include <stdio.h>

/* multi-volume file */
/* file format for the annotated scene */

typedef struct mvf_header {
  int   W,H,D;
  float dx,dy,dz;
  int   nchunks;
} MVF_Header;

typedef struct mvf_chunk_header {
  char  name[32];
  int   offset;
  int   length;
  int   chunk_type;
} MVF_Chunk_Header;

typedef struct mvf_file {
  MVF_Header        header;
  MVF_Chunk_Header *chunks;
  FILE             *f;  
  char             *path;
  int              rwmode; /* 0=read, 1=write, 2=r/w */
} MVF_File;

/* chunk_type */
#define MVF_CHUNK_TEXT    0
#define MVF_CHUNK_8BIT    8
#define MVF_CHUNK_16BIT  16

/* usable for creation */
MVF_File * mvf_create(char *path, int w,int h,int d,
		      float dx,float dy,float dz);

void       mvf_add_chunk(MVF_File *mvf, char *chunk_name, 
			 int length, int chunk_type);
void       mvf_deploy_chunks(MVF_File *mvf);

int        mvf_write_chunk_data(MVF_File *mvf, int chunk_index,
				int offset, void *ptr, int bytes);
void       mvf_fill_chunk(MVF_File *mvf, int chunk_index, char byte);

void       mvf_write_int_string(MVF_File *mvf, int fieldlen, int value);
void       mvf_write_float_string(MVF_File *mvf, int fieldlen, float value);
void       mvf_write_string(MVF_File *mvf, int fieldlen, char *value);
int        mvf_write_data(MVF_File *mvf, void *ptr, int bytes);

/* usable for reading */
int        mvf_check_format(char *path);
MVF_File * mvf_open(char *path);
MVF_File * mvf_open_rw(char *path);
int        mvf_read_chunk_data(MVF_File *mvf, int chunk_index,
			       int offset, void *ptr, int bytes);

int        mvf_read_int_string(MVF_File *mvf, int fieldlen);
float      mvf_read_float_string(MVF_File *mvf, int fieldlen);
void       mvf_read_string(MVF_File *mvf, int fieldlen, char *dest);
int        mvf_read_data(MVF_File *mvf, void *ptr, int bytes);

/* common (usable when reading / creating) */

void       mvf_close(MVF_File *mvf);
int        mvf_lookup_chunk(MVF_File *mvf, char *chunk_name);
int        mvf_goto(MVF_File *mvf, int chunk_index, int offset);
int        mvf_skip(MVF_File *mvf, int offset);

#endif
