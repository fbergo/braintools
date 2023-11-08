
#ifndef OPTLIB_H
#define OPTLIB_H 1

void MemCpy(void *dest,void *src,int nbytes);
void MemSet(void *dest,unsigned char c, int nbytes);
void RgbSet(void *dest,int rgb, int count);
void FloatFill(float *dest, float val, int count);
void DoubleFill(double *dest, double val, int count);
void RgbRect(void *dest,int x,int y,int w,int h,int rgb,int rowoffset);

#endif /* OPTLIB_H */
