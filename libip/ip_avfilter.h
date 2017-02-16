
#ifndef AVFILTER_H
#define AVFILTER_H 1

#include "ip_volume.h"

// slightly incorrect at the borders, but faster

/* cross element */
void FastMorphologicalGradient(XAnnVolume *vol);

/* box element (thicker) */
void FastBoxMorphologicalGradient(XAnnVolume *vol);

void FastDirectionalMaxGradient(XAnnVolume *vol);
void FastDirectionalAvgGradient(XAnnVolume *vol);

void SobelGradient(XAnnVolume *vol);

// slower but correct
void MorphologicalGradient(XAnnVolume *vol);

/* lookup-table filter, used by LS, GS and MBGS */

void XA_Lookup_Filter(XAnnVolume *vol, i16_t *lookup);
void XA_Lookup_Filter_Preview(Volume *vol, i16_t *lookup,
			      int x,int y,int z);

/* Linear Stretch (brightness/contrast) filter */

i16_t *  XA_LS_Function(float B, float C,int maxval);
void     XA_LS_Filter(XAnnVolume *vol, float B, float C);
void     XA_LS_Filter_Preview(Volume *vol, float B, float C,
			      int x,int y,int z);

/* Gaussian Stretch */

i16_t *  XA_GS_Function(float mean, float dev);
void     XA_GS_Filter(XAnnVolume *vol, float mean, float dev);
void     XA_GS_Filter_Preview(Volume *vol, float mean, float dev,
			      int x,int y,int z);

/* Multi-Band GS */

i16_t *  XA_MBGS_Function(int bands, float *means, float *devs);
void     XA_MBGS_Filter(XAnnVolume *vol, int bands, float *means, float *devs);
void     XA_MBGS_Filter_Preview(Volume *vol, int bands, float *means, float *devs,
				int x,int y,int z);


/* Threshold */
i16_t *  XA_Threshold_Function(int lower,int upper);
void     XA_Threshold_Filter(XAnnVolume *vol, int lower, int upper);
void     XA_Threshold_Filter_Preview(Volume *vol, int lower, int upper,
				     int x,int y,int z);


void XABlur3x3(XAnnVolume *vol);

/* Moda/Median */

void XAModa(XAnnVolume *vol);
void XAMedian(XAnnVolume *vol);

/* BPP-SSR */
void XABrainGaussianEstimate(XAnnVolume *vol, float perc, 
			     float *mean, float *stddev);
void XABrainGaussianAutoEstimate(XAnnVolume *vol, float *mean, float *stddev);

/* Affinity */
void XAMaxAffinity(XAnnVolume *vol);
void XAMinAffinity(XAnnVolume *vol);

/* Cost Threshold */

void XACostThreshold(XAnnVolume *vol, int min, int max, 
		     int onvalue, int inverted);

/* Phantom Lesion */

void XAPhantomLesion(XAnnVolume *vol, int radius, int mean, int noise, int border,
		     int x,int y,int z);

/* Homogenization */
void XAHomog(XAnnVolume *vol, int zslice);

/* Morphology */

void XADilate(XAnnVolume *vol, Volume *se); 
void XAErode(XAnnVolume *vol, Volume *se); 

/* Arithmetic 

   operations: 0 : orig - current
               1 : current - orig
               2 : orig + current
	       3 : current / orig
	       4 : orig / current
	       5 : orig * current
	       6 : orig := current

*/

void XAArithmetic(XAnnVolume *vol, int op);

#endif
