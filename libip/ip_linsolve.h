#ifndef LIBIP_LINSOLVE_H
#define LIBIP_LINSOLVE_H 1

#include <ip_volume.h>

/* 
   a~b , c~d, e~f, g~h,
   result is a 12-position array for the 12 coefficients
   A..L of the homogeneous affine transform
 
   [ A B C J ]
   [ D E F K ]
   [ G H I L ]
   [ 0 0 0 1 ]

   that (hopefully) takes b to a, d to c, f to e and h to g.

*/
int SolveRegistration(double *result,
		      int ax,int ay,int az,
		      int bx,int by,int bz,
		      int cx,int cy,int cz,
		      int dx,int dy,int dz,
		      int ex,int ey,int ez,
		      int fx,int fy,int fz,
		      int gx,int gy,int gz,
		      int hx,int hy,int hz);

/* coeffs is the result from SolveRegistration */
Volume *VolumeRegistrationTransform(Volume *src, double *coeffs);

int   minv(double *a, int n);
void  mmul(double *a,double *b,int n);

int   minvf(float *a, int n);
void  mmulf(float *a,float *b,int n);

void  htrans(double *dest, double *src, double *transform);
void  RTransform(double *dest, double *src);

void  least_squares(i16_t *varx, i16_t *vary, int n, float *a, float *b);
void  pseudo_least_squares(i16_t *varx, i16_t *vary, int n, float *a, float *b);

#endif
