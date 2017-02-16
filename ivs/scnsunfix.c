
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int nc_fgets(char *s, int m, FILE *f) {
  while(fgets(s,m,f)!=NULL)
    if (s[0]!='#') return 1;
  return 0;
}

int read8(FILE *f) {
  int i;
  i = fgetc(f);
  return(i);
}

int read16(FILE *f) {
  int i,j,k;
  i = read8(f);
  j = read8(f);
  k = 256*j + i;
  return(k);
}

int read32(FILE *f) {
  int i,j,k;
  i = read16(f);
  j = read16(f);
  k = 65536*j + i;
  return(k);
}

void put8(FILE *f, int c) {
  fputc((c&0xff), f);
}

void put16(FILE *f, int c) {
  put8(f, c&0xff);
  put8(f, c>>8);
}

void put32(FILE *f, int c) {
  put16(f, c&0xffff);
  put16(f, c>>16);
}

int main(int argc, char ** argv) {
  FILE *f;
  int w,h,d,N;
  float dx, dy, dz;
  int bits;
  int *data;
  char z[4096];
  char *buffer;

  int i, j;

  if (argc != 2) {
    fprintf(stderr,"usage: scnsunfix scn-file\n\n");
    return 1;
  }

  f = fopen(argv[1],"r");

  buffer = (char *) malloc(65536);
  if (!buffer) goto kaput;

  setvbuf(f, buffer, _IOFBF, 65536);

  if (!f) {
    fprintf(stderr, "unable to open file %s\n",argv[1]);
    return 2;
  }

  if (!nc_fgets(z,4095,f)) goto kaput;
  if (z[0]!='S' || z[1]!='C' || z[2]!='N') goto kaput;
  if (!nc_fgets(z,4095,f)) goto kaput;
  if (sscanf(z," %d %d %d \n",&w,&h,&d)!=3) goto kaput;
  if (!nc_fgets(z,4095,f)) goto kaput;
  if (sscanf(z," %f %f %f \n",&dx,&dy,&dz)!=3) goto kaput;
  if (!nc_fgets(z,4095,f)) goto kaput;
  if (sscanf(z," %d \n",&bits)!=1) goto kaput;
  
  N = w * h *d;

  if (bits == 8) {
    printf("8-bit data, there's nothign to do.\n");
    return 0;
  }

  if (bits != 16)
    goto kaput;

  data = (int *) malloc (sizeof(int) * N);
  if (!data) goto kaput;

  for(i=0;i<N;i++) {
    j = read16(f);
    data[i] = (j>>8) | ((j&0xff)<<8);
  }
  fclose(f);

  f = fopen(argv[1],"w");
  if (!f) goto kaput;

  fprintf(f,"SCN\n%d %d %d\n%f %f %f\n16\n",
	  w,h,d,dx,dy,dz);
  
  for(i=0;i<N;i++)
    put16(f, data[i]);

  fclose(f);
  return 0;

 kaput:
  fprintf(stderr,"error!\n");
  return 2;
}

