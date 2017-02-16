

#ifndef IFTALGO_H
#define IFTALGO_H 1

#include "ip_volume.h"

void WaterShedP(XAnnVolume *vol);
void WaterShedM(XAnnVolume *vol);
void ClearTrees(XAnnVolume *vol);

// ClearTress + WaterShedP
void WaterShedPlus(XAnnVolume *vol);

// ClearTress + WaterShedM
void WaterShedMinus(XAnnVolume *vol);

// main Fuzzy connectedness entry point
void DFuzzyConn(XAnnVolume *vol, int nobjs, float *objdata);
void DStrictFuzzyConn(XAnnVolume *vol, int nobjs, float *objdata);

/* can be used for previewing - var is stddev^2 !!!*/
short int FuzzyCost(short int v, float mean, float var,float topval);

/* extended fuzzy voxel data */
typedef struct _fob {
  int   n;
  float mean[10];
  float stddev[10];
  float var[10];
} fob;

/* DMT - Delta Minimization Transform */
void DeltaMinimization(XAnnVolume *vol);

/* LazyShed L1 - Watershed with distance cost */
void LazyShedL1(XAnnVolume *vol);

/* DiscShed - Whatershed with built-in gradient computation */
void DiscShed(XAnnVolume *vol, int gradmode);

/* Kappa Connected Segmentation (but with infinite kappa) */
void KappaSeg(XAnnVolume *vol);

#endif
