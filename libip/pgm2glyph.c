#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// input: stdin, output: stdout
int main(int argc, char **argv) {
  int i,j,w,h;
  unsigned char *data;
  char *p;
  char line[512];

  if (fgets(line,511,stdin)) {
    if (line[0]!='P' || line[1]!='5') {
      fprintf(stderr,"input must be a P5 image.\n");
      return 1;
    }
  }

  do { fgets(line,511,stdin); } while(line[0]=='#');

  p=strtok(line," \t\n\r");
  w = atoi(p);
  p=strtok(0," \t\n\r");
  h = atoi(p);

  do { fgets(line,511,stdin); } while(line[0]=='#');

  data = (unsigned char *) malloc(w*h+10);
  if (!data) return 1;
  memset(data,0,w*h+10);

  for(i=0;i<w*h;i++) {
    data[i] = fgetc(stdin);
    if (data[i] < 255) data[i] = 0;
  }

  // print out result
  printf("!");
  for(i=0;i<w*h;i+=4) {
    j = 0;
    if (data[i])   j|=8;
    if (data[i+1]) j|=4;
    if (data[i+2]) j|=2;
    if (data[i+3]) j|=1;
    printf("%x",j);
  }
  printf("\n");
  return 0;
}
