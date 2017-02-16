#ifndef _ADJACENCY_H_
#define _ADJACENCY_H_

typedef struct _adjrel {
  int *dx;
  int *dy;
  int *dz;
  int *dn;
  int n;
} AdjRel;

AdjRel *CreateAdjRel(int n);

void    DestroyAdjRel(AdjRel **A);

AdjRel *Spherical(float r); // 3D
AdjRel *Circular(float r);  // 2D

/* calculates the dn array for a WxHxD volume */
void    AdjCalcDN(AdjRel *A, int W,int H, int D);

/* returns the index that encodes the Nsrc-Ndest edge */
unsigned char  AdjEncodeDN(AdjRel *A, int Nsrc, int Ndest);

/* returns the Ndest of the encoded Nsrc-Ndest edge */
int AdjDecodeDN(AdjRel *A, int Nsrc, int edge);

#endif
