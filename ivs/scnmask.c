#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libip.h>

int main(int argc, char **argv) {
  Volume *v;
  int i,j,k,lv;

  if (argc<3 || argc>4) {
    fprintf(stderr,"usage: scnmask input.scn level [output.scn]\n\n");
    return 1;
  }

  v = VolumeNewFromFile(argv[1]);
  if (!v) {
    fprintf(stderr,"error reading %s\n",argv[1]);
    return 2;
  }

  lv = atoi(argv[2]);

  for(i=0;i<v->D;i++)
    for(j=0;j<v->H;j++)
      for(k=0;k<v->W;k++)
	if (VolumeGetI16(v,k,j,i) >= lv)
	  VolumeSetI16(v,k,j,i,1);
	else
	  VolumeSetI16(v,k,j,i,0);

  if (argc==4)
    VolumeSaveSCN(v,argv[3]);
  else
    VolumeSaveSCN(v,argv[1]);

  return 0;
}
