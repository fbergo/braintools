#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libip.h>

int main(int argc, char **argv) {
  Volume *T,*S;
  int W,H,D,N,i,j,k;

  int O1=0,O2=0,FP=0,FN=0,FA=0; // obj1, obj2, false positives, false negative, false anythings
  float F[10];

  if (argc!=3) {
    fprintf(stderr,"usage: scncomp truth-scene subject-scene\n\n");
    return 1;
  }

  T = VolumeNewFromFile(argv[1]);
  S = VolumeNewFromFile(argv[2]);

  if (!T || !S) {
    fprintf(stderr,"error reading inputs.\n\n");
    return 2;
  }

  W = T->W;
  H = T->H;
  D = T->D;
  if (S->W != W || S->H != H || S->D != D) {
    fprintf(stderr,"image dimensions do not match, bye.\n\n");
    return 2;
  }
  N = W*H*D;

  for(i=0;i<N;i++) {
    j = (T->u.i16[i] != 0);
    k = (S->u.i16[i] != 0);

    if (j) O1++;
    if (k) O2++;
    if (j && (!k)) FN++;
    if ((!j) && k) FP++;
    if (j!=k) FA++;
  }

  VolumeDestroy(S);
  VolumeDestroy(T);

  F[0] = ((float)(N-FA)) / ((float)N);
  F[1] = ((float)FN) / ((float)N);
  F[2] = ((float)FP) / ((float)N);

  F[3] = ((float)(O1-FA)) / ((float)O1);
  F[4] = ((float)FN) / ((float)O1);
  F[5] = ((float)FP) / ((float)O1);

  for(i=0;i<6;i++) F[i]*= 100.0;

  printf("                      Accuracy\tF.Pos\tF.Neg\n");
  printf("Pct over scene size:  %.4f\t%.4f\t%.4f\n",F[0],F[2],F[1]);
  printf("Pct over truth size:  %.4f\t%.4f\t%.4f\n",F[3],F[5],F[4]);

  return 0;
}
