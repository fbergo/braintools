#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libip.h>

int main(int argc, char **argv) {
  Volume *A,*B,*C;
  int W,H,D,N,i,T;

  if (argc!=5) {
    fprintf(stderr,"usage: spm2mask img1 img2 out threshold\n");
    return 1;
  }

  A = VolumeNewFromFile(argv[1]);
  B = VolumeNewFromFile(argv[2]);
  T = atoi(argv[4]);

  if (A==0 || B==0) {
    fprintf(stderr,"error reading input.\n");
    return 2;
  }

  W = A->W;
  H = A->H;
  D = A->D;
  if (W!=B->W || H!=B->H || D!=B->D) {
    fprintf(stderr,"images must have the same dimensions.\n");
    return 2;
  }
  N = W*H*D;

  C = VolumeNew(W,H,D,vt_integer_8);
  VolumeFill(C,0);

  for(i=0;i<N;i++)
    if (A->u.i16[i] >= T || B->u.i16[i] >= T)
      C->u.i8[i] = 1;


  VolumeDestroy(A);
  VolumeDestroy(B);
  VolumeSaveSCN8(C,argv[3]);
  VolumeDestroy(C);
  return 0;
}
