
#ifndef POLY_H
#define POLY_H 1

typedef struct _polynomial {
  int maxdegree;
  int degree;
  float * coef;
} Polynomial;

Polynomial * PolyNew(int maxdeg);
void         PolyDestroy(Polynomial *p);

void         PolyZero(Polynomial *p);
void         PolyCopy(Polynomial *dest, Polynomial *src);
void         PolyMultiplyV(Polynomial *dest, float v);
void         PolyMultiplyP(Polynomial *dest, Polynomial *src);
void         PolySumP(Polynomial *dest, Polynomial *src);

void         PolyDerivative(Polynomial *p);

void         PolySet(Polynomial *dest, int deg, float coef);
float        PolyGet(Polynomial *src, int deg);

float        PolyValue(Polynomial *src, float x);

void         LagrangeInterpolation(Polynomial *dest, 
				   float *x, float *y, int np);

#endif
