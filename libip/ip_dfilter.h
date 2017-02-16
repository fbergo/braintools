
#ifndef DFILTER_H
#define DFILTER_H 1

#include "ip_volume.h"

/* all operations assume 16-bit volumes */

/* B,C=(0,0) = no change; (100,100) = +100% on both */
void BrightnessContrast(Volume *dest, Volume *src, float B, float C);


void Threshold(Volume *dest, Volume *src, int val, 
	       int blackbelow, int whiteabove);

void Blur3x3(Volume *dest, Volume *src);

void GaussianStretch(Volume *dest, Volume *src, float mean, float dev);

void Erode3x3(Volume *dest, Volume *src);
void Dilate3x3(Volume *dest, Volume *src);

double * Histogram16(Volume *src);
double * HultHistogram16(Volume *src);

/* kernel density estimation */
double * KDE(double *h16);
double * derivate(double *h);
double * SplineInterp(double *h16);
double * HistAvg(double *h, int t);

void     HistNormalize(double *h16);
int      MinBucket(double *h16);
int      MaxBucket(double *h16);
double * CreateHistogram();
void     DestroyHistogram(double *h16);

#endif
