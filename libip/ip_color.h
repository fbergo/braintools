
#ifndef COLOR_H
#define COLOR_H

/* color-related functions */

int triplet(int a,int b,int c);

#define TRIPLET(a,b,c) ((((a)&0xff)<<16)|(((b)&0xff)<<8)|((c)&0xff))

#define t0(a) ((a>>16)&0xff)
#define t1(a) ((a>>8)&0xff)
#define t2(a) (a&0xff)

int RGB2YCbCr(int v);
int YCbCr2RGB(int v);

int RGB2HSV(int vi);
int HSV2RGB(int vi);

/* lightens a color by (1.10 ^ n) */
int          lighter(int c, int n);

/* darkens a color by (0.90 ^ n) */
int          darker(int c, int n);

/* returns the inverse in RGB-space (255-R,255-G,255-B) */
int          inverseColor(int c);

/* saturates the color */
int          superSat(int c);

/* merges two colors (1-ratio) of a and ratio of b, in RGB-space */
int          mergeColorsRGB(int a,int b,float ratio);

/* merges two colors (1-ratio) of a and ratio of b, in YCbCr-space */
int          mergeColorsYCbCr(int a,int b,float ratio);

/* returns RGB triplet with R=c, G=c, B=c */
int          gray(int c);

/* returns a random color */
int          randomColor();

#endif
