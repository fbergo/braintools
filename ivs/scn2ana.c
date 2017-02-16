/*
   SCN -> Analyze conversion

   (C) 2003 Felipe P.G. Bergo <bergo@liv.ic.unicamp.br>
   2003.11.03

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
#include <errno.h>
#include "fio.h"

struct _scn {
  int   W,H,D;
  float dx,dy,dz;
  int   bpp;
  int   maxval;
} scn;

struct _hdr1 {
  int        hdrlen;
  char       data_type[10];
  char       db_name[18];
  int        extents;
  short int  error;
  char       regular;
  char       hkey0;
} hdr1;

struct _hdr2 {
  short int  dim[8];
  short int  unused[7];
  short int  data_type;
  short int  bpp;
  short int  dim_un0;
  float      pixdim[8];
  float      zeroes[8];
  int        maxval;
  int        minval;
} hdr2;

char * scn_name = 0;
char * hdr_name = 0;
char * img_name = 0;

void fgoto(int x,int y,int z,FILE *f, int bpp) {
  long off;
  off = bpp * (x + (scn.W*(scn.H-1-y)) + (scn.W * scn.H) * z);
  fseek(f,off,SEEK_SET);  
}

void write_hdr() {
  int i;
  FILE *f;

  f = fopen(hdr_name,"w");
  if (!f) {
    fprintf(stderr,"** unable to open %s for writing.\n", hdr_name);
    exit(1);
  }

  memset(&hdr1, 0, sizeof(hdr1));
  memset(&hdr2, 0, sizeof(hdr2));

  for(i=0;i<8;i++) {
    hdr2.pixdim[i] = 0.0;
    hdr2.zeroes[i] = 0.0;
  }

  /* -- first header segment -- */

  hdr1.hdrlen  = 348;
  hdr1.regular = 'r';

  fio_abs_write_32(f, 0, 0, hdr1.hdrlen);
  fio_abs_write_zeroes(f, 4, 34);
  fio_abs_write_8(f, 38, hdr1.regular);
  fio_abs_write_8(f, 39, hdr1.hkey0);

  /* -- second header segment -- */

  hdr2.dim[0] = 4;
  hdr2.dim[1] = scn.W;
  hdr2.dim[2] = scn.H;
  hdr2.dim[3] = scn.D;
  hdr2.dim[4] = 1;

  hdr2.data_type = (scn.bpp == 16) ? 4 : 2;
  hdr2.bpp       = scn.bpp;
  hdr2.pixdim[0] = 1.0;
  hdr2.pixdim[1] = scn.dx;
  hdr2.pixdim[2] = scn.dy;
  hdr2.pixdim[3] = scn.dz;
  hdr2.maxval    = scn.maxval;
  hdr2.minval    = 0;

  for(i=0;i<8;i++)
    fio_abs_write_16(f, 40 + 0  + 2*i, 0, hdr2.dim[i]);
  for(i=0;i<7;i++)
    fio_abs_write_16(f, 40 + 16 + 2*i, 0, hdr2.unused[i]);

  fio_abs_write_16(f, 40 + 30, 0, hdr2.data_type);
  fio_abs_write_16(f, 40 + 32, 0, hdr2.bpp);
  fio_abs_write_16(f, 40 + 34, 0, hdr2.dim_un0);

  for(i=0;i<8;i++)
    fio_abs_write_float32(f, 40 + 36 + 4*i, 0, hdr2.pixdim[i]);
  for(i=0;i<8;i++)
    fio_abs_write_float32(f, 40 + 68 + 4*i, 0, hdr2.zeroes[i]);
 
  fio_abs_write_32(f, 40 + 100, 0, hdr2.maxval);
  fio_abs_write_32(f, 40 + 104, 0, hdr2.minval);

  /* -- third header segment (patient info) --- */

  fio_abs_write_zeroes(f, 148, 200);
  fclose(f);
}

void read_scn() {
  FILE *f,*g;
  char line[256];
  char *buf,*buf2;
  int i,j,k,l,Bpp,cmax=0;
  int host_be = 0;
  short int *ibuf;

  k = 0x11110000;
  buf = (char *) (&k);
  if (buf[0] == 0x11) host_be = 1;

  f = fopen(scn_name,"r");
  if (!f) {
    fprintf(stderr,"** unable to open %s for reading.\n",scn_name);
    exit(1);
  }

  if (!fgets(line, 255, f))
    goto read_error;

  if (line[0]!='S' || line[1] !='C' || line[2] != 'N')
    goto format_error;

  if (fscanf(f,"%d %d %d \n",&(scn.W),&(scn.H),&(scn.D))!=3)
    goto format_error;

  if (fscanf(f,"%f %f %f \n",&(scn.dx),&(scn.dy),&(scn.dz))!=3)
    goto format_error;

  if (!fgets(line, 255, f))
    goto format_error;
  while(strlen(line)>0 && line[strlen(line)-1] < 32) 
    line[strlen(line)-1] = 0;
  scn.bpp = atoi(line);

  if (scn.bpp != 8 && scn.bpp != 16)
    goto format_error;

  printf("%d %d %d %.2f %.2f %.2f %d\n",scn.W,scn.H,scn.D,scn.dx,scn.dy,scn.dz,scn.bpp);

  j = scn.H * scn.D;
  Bpp = scn.bpp / 8;

  buf = (char *) malloc( scn.W * Bpp );
  if (!buf)
    goto malloc_error;

  buf2 = (char *) malloc( scn.W * Bpp );
  if (!buf2)
    goto malloc_error;

  g = fopen(img_name,"w");
  if (!g) {
    fprintf(stderr,"** unable to open %s for writing.\n",
	    img_name);
    exit(1);
  }

  ibuf = (short int *) buf;

  /* copy pixel data */
  for(i=0;i<j;i++) {
    if (fread(buf, 1, scn.W * Bpp, f) != (scn.W * Bpp))
      goto read_error;
    fgoto(0,i%(scn.H),i/(scn.H),g,Bpp);
    if (fwrite(buf, 1, scn.W * Bpp, g) != (scn.W * Bpp))
      goto write_error;

    if (scn.maxval < 0) {
      if (Bpp == 1) {
	for (k=0;k<scn.W;k++) {
	  l = (int) ((unsigned char) (buf[k]));
	  if (l > cmax) cmax = l;
	}
      } else {
	for (k=0;k<scn.W;k++) {
	  l = (int) (ibuf[k]);
	  if (host_be) l = fio_swab_16(l);
	  if (l > cmax) cmax = l;
	}
      }
    }
  }

  fclose(g);
  fclose(f);

  if (scn.maxval < 0)
    scn.maxval = cmax;

  return;

 format_error:
  fprintf(stderr,"** input file is not in SCN format.\n");
  exit(2);
 read_error:
  fprintf(stderr,"** read error - %s.\n",strerror(errno));
  exit(2);
 write_error:
  fprintf(stderr,"** write error.\n");
  exit(2);
 malloc_error:
  fprintf(stderr,"** malloc error.\n");
  exit(3);

}

int main(int argc, char **argv) {
  int i, got_scn_name=0;

  scn.maxval = -1;

  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      if ( (i < (argc-1)) && !strcmp(argv[i],"-o") ) {
	if (hdr_name) free(hdr_name);
	hdr_name = (char *) malloc(strlen(argv[i+1])+8);
	strcpy(hdr_name, argv[i+1]);
	if (!strstr(hdr_name,".hdr"))
	  strcat(hdr_name,".hdr");
	++i;
	continue;
      }
      if ( (i < (argc-1)) && !strcmp(argv[i],"-max") ) {
	scn.maxval = atoi(argv[i+1]);
	++i;
	continue;
      }

      fprintf(stderr,"unknown command line option: %s\n\n",argv[i]);
      return 1;
    }
    
    if (scn_name) free(scn_name);
    scn_name = (char *) malloc(strlen(argv[i])+1);
    strcpy(scn_name, argv[i]);
    got_scn_name = 1;

  }

  if (!got_scn_name) {
    fprintf(stderr,"usage: scn2ana [-o outfile.hdr] scnfile\n\n");
    return 1;
  }

  if (!hdr_name)
    hdr_name = "out.hdr";

  img_name = (char *) malloc(strlen(hdr_name)+1);
  strcpy(img_name, hdr_name);
  strcpy(img_name + strlen(hdr_name) - 3, "img");

  read_scn();
  write_hdr();

  return 0;
}
