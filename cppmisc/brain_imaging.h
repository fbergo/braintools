
/*

  monolithic header with brain imaging classes.

  originally this was called everything.h

  (C) 2009-2017 Felipe Bergo, fbergo at gmail.com

 */

#ifndef BRAIN_IMAGING_H
#define BRAIN_IMAGING_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <bzlib.h>
#include <zlib.h>
#include <png.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __APPLE__
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#endif

using namespace std;

//! String Tokenizer
class Tokenizer {
public:

  //! Constructor, creates a tokenizer with no string and blanks as separator.
  //! See \ref setSeparator(const char *s).
  Tokenizer() {
    src = NULL;
    ptr = NULL;
    sep = NULL;
    token = NULL;
    setSeparator();
  }
  
  //! Constructor, creates a tokenizer with string s and the given
  //! set of separators. If separator is NULL, blanks are the separators.
  //! See \ref setSeparator(const char *s).
  Tokenizer(const char *s, const char *separator=NULL) {
    src = NULL;
    ptr = NULL;
    sep = NULL;
    token = NULL;
    setString(s);
    setSeparator(separator);
  }


  //! Destructor
  ~Tokenizer() {
    if (src!=NULL) free(src); 
    if (sep!=NULL) free(sep);
    if (token!=NULL) free(token);
  }

  //! Returns the next token, or NULL if there are no more tokens.
  char * nextToken() {
    char *start;
    int i;

    while(*ptr != 0 && strchr(sep,*ptr)!=NULL)
      ++ptr;

    if (*ptr == 0) return NULL; // no more tokens
    start = ptr;

    while(*ptr != 0 && strchr(sep,*ptr)==NULL)
      ++ptr;

    if (token!=NULL) { free(token); token = NULL; }
    token = (char *) malloc(1+ptr-start);
    if (token==NULL) return NULL;
    
    for(i=0;i<ptr-start;i++)
      token[i] = start[i];
    token[ptr-start] = 0;
    return token;
  }

  //! Returns the next token interpreted as an integer, or 0 if there
  //! are no more tokens left.
  int nextInt() {
    char *x;
    x = nextToken();
    if (x==NULL) return 0; else return(atoi(x));
  }

  //! Returns the next token interpreted as a double, or 0 if there
  //! are no more tokens left.
  double nextDouble() {
    char *x;
    x = nextToken();
    if (x==NULL) return 0; else return(atof(x));
  }

  //! Returns the next token interpreted as a float, or 0 if there
  //! are no more tokens left.
  float  nextFloat() {
    return((float)nextDouble());
  }

  //! Sets the string to be tokenized to s, and places the tokenization
  //! cursor at the start of s.
  void setString(const char *s) {
    if (src!=NULL) { free(src); src=NULL; }
    src = strdup(s);
    ptr = src;
  }

  //! Sets the set of token separator characters. If set to NULL, the blanks
  //! string (" \t\n\r") is used.
  void setSeparator(const char *separator=NULL) {
    if (sep!=NULL) { free(sep); sep=NULL; }
    if (separator!=NULL) sep = strdup(separator); else sep = strdup(" \t\n\r");
  }

  //! Rewinds the tokenization cursor to the start of the current subject
  //! string.
  void rewind() {
    ptr = src;
  }

  //! Returns the number of tokens left
  int countTokens() {
    int n;
    char *ptrcopy;
    ptrcopy=ptr;
    n = 0;
    while(nextToken()!=NULL) ++n;
    ptr = ptrcopy;
    return n;
  }

private:
  char *src,*ptr,*sep,*token;
};

//! Point or Vector in 3D space.
//! Point/Vector coordinates are stored as 32-bit floating point.
class R3 {
 public:  
  float X,Y,Z;

  //! creates a point at the origin
  R3() { X=Y=Z=0.0; }
  //! Constructor, creates a point at the given coordinates
  R3(float x,float y,float z) { X=x; Y=y; Z=z; }
  //! Copy constructor
  R3(const R3 &p) { X=p.X; Y=p.Y; Z=p.Z; }
  //! Copy constructor
  R3(R3 *p) { X=p->X; Y=p->Y; Z=p->Z; }

  //! point assignment
  R3 & operator=(const R3 &b) { X=b.X; Y=b.Y; Z=b.Z; return(*this); }

  //! adds b to the point
  R3 & operator+=(const R3 &b) { X+=b.X; Y+=b.Y; Z+=b.Z; return(*this); }

  //! subtracts b from the point
  R3 & operator-=(const R3 &b) { X-=b.X; Y-=b.Y; Z-=b.Z; return(*this); }

  //! multiplies the point by a scalar
  R3 & operator*=(const float &b) { X*=b; Y*=b; Z*=b; return(*this); }

  //! divides the point by a scalar
  R3 & operator/=(const float &b) { X/=b; Y/=b; Z/=b; return(*this); }

  //! vector addition
  R3 operator+(const R3 &b) const { 
    R3 t; t.X = X+b.X; t.Y = Y+b.Y; t.Z = Z+b.Z; return(t); }

  //! vector subtraction
  R3 operator-(const R3 &b) const { 
    R3 t; t.X = X-b.X; t.Y = Y-b.Y; t.Z = Z-b.Z; return(t); }

  //! vector multiplication by a scalar
  R3 operator*(const float &b) const { 
    R3 t; t.X = X*b; t.Y = Y*b; t.Z = Z*b; return(t); }

  //! vector division by a scalar
  R3 operator/(const float &b) const { 
    R3 t; t.X = X/b; t.Y = Y/b; t.Z = Z/b; return(t); }
 
  //! vector inner product
  float inner(const R3 &b) const { return(X*b.X+Y*b.Y+Z*b.Z); }

  //! angle between two vectors
  float angle(const R3 &b) const { 
    float ip; 
    ip = inner(b) / (length() * b.length()); 
    if (ip < -1.0f) ip = -1.0f;
    if (ip > 1.0f) ip = 1.0f;
    return(acos(ip));
  }

  //! Returns the vector cross product of this x b. Does not modify this point.
  R3 cross(const R3 &b) const {
    R3 r;
    r.X = Y*b.Z - Z*b.Y;
    r.Y = Z*b.X - X*b.Z;
    r.Z = X*b.Y - Y*b.X;
    return r;
  }

  //! sets the coordinates of the point
  void set(float x, float y, float z) { X=x; Y=y; Z=z; }

  //! returns the length of the vector
  float length() const { return(sqrt(X*X+Y*Y+Z*Z)); }

  //! normalizes this vector to unit-length
  void  normalize() { float l; l = length(); if (l!=0.0) (*this)/=l; }

  //! prints this vector to stdout
  void print() const { cout << "R3=(" << X << "," << Y << "," << Z << ")\n"; }
  void print(const char *s) const { cout << s << "=(" << X << "," << Y << "," << Z << ")\n"; }
  void print(int dummy) const { cout << "R3=(" << X << "," << Y << "," << Z << ")"; }

  //! applies psqrt to all components
  void usqrt() { X = R3::usqrt(X); Y = R3::usqrt(Y); Z = R3::usqrt(Z); }

 private:
  static double usqrt(const double a) {
    if (a<0) return(-sqrt(-a)); else return(sqrt(a));
  }
};

//! 3x3 Linear Transformation.
//! 3x3 Linear Transformation, which does not allow translations.
class T3 {
 public:
  //! Constructor, creates a null transform
  T3() { zero(); }

  //! Copy constructor
  T3(const T3 &b) { 
    int i,j; for(i=0;i<3;i++) for(j=0;j<3;j++) e[i][j] = b.e[i][j]; }

  //! Transform assignment
  T3 & operator=(const T3 &b) {
    int i,j; for(i=0;i<3;i++) for(j=0;j<3;j++) e[i][j] = b.e[i][j]; 
    return(*this); }

  //! Transform composition, multiplies this transform by b, and overwrites
  //! this transform. Returns a reference to this transform.
  T3 & operator*=(const T3 & b) { (*this) = (*this) * b; return(*this); }

  //! Multiplication by a scalar
  T3 operator*(const T3 &b) {
    T3 d;
    int i,j,k;
    d.zero();
    for(i=0;i<3;i++) for(j=0;j<3;j++) for(k=0;k<3;k++)
      d.e[i][j] += b.e[i][k] * this->e[k][j];
    return d;
  }
  
  //! Applies this transform to point a and returns the transformed point
  R3 apply(const R3 & a) {
    R3 b;
    b.X = a.X * e[0][0] + a.Y * e[0][1] + a.Z * e[0][2];
    b.Y = a.X * e[1][0] + a.Y * e[1][1] + a.Z * e[1][2];
    b.Z = a.X * e[2][0] + a.Y * e[2][1] + a.Z * e[2][2];
    return b;
  }

  //! Sets all coefficients to zero (null transform)
  void zero() {
    int i,j; for(i=0;i<3;i++) for(j=0;j<3;j++) e[i][j]=0.0; }

  //! Sets an identity transform
  void identity() { zero(); e[0][0] = e[1][1] = e[2][2] = 1.0; }

  //! Sets a rotation around X-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void xrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[1][1] = cos(angle);  e[2][1] = sin(angle);
    e[1][2] = -sin(angle); e[2][2] = cos(angle);
  }

  //! Sets a rotation around Y-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void yrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[0][0] = cos(angle);  e[2][0] = sin(angle);
    e[0][2] = -sin(angle); e[2][2] = cos(angle);
  }

  //! Sets a rotation around Z-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void zrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[0][0] = cos(angle);  e[1][0] = sin(angle);
    e[0][1] = -sin(angle); e[1][1] = cos(angle);
  }

  //! Sets a scaling transform, by factor on all dimensions
  void scale(float factor) {
    zero(); e[0][0]=e[1][1]=e[2][2]=factor;
  }

  //! Sets a scaling transform, with different scaling factors
  //! for each dimension.
  void scale(float fx, float fy, float fz) {
    zero(); e[0][0]=fx; e[1][1]=fy; e[2][2]=fz;
  }

  //! Prints the transform matrix on stdout
  void print() {
    int i,j;
    for(i=0;i<3;i++) {
      for(j=0;j<3;j++) {
	printf("%.2f ",e[j][i]);
      }
      printf("\n");
    }
    printf("\n");
  }

 private:
  float e[3][3];
};

//! 4x4 Linear Transformation.
//! 4x4 Linear Transformation.
class T4 {
 public:
  //! Constructor, creates a null transform
  T4() { zero(); }

  //! Copy constructor
  T4(const T4 &b) { 
    int i,j; for(i=0;i<4;i++) for(j=0;j<4;j++) e[i][j] = b.e[i][j]; }

  //! Transform assignment
  T4 & operator=(const T4 &b) {
    int i,j; for(i=0;i<4;i++) for(j=0;j<4;j++) e[i][j] = b.e[i][j]; 
    return(*this); }

  //! Transform composition, multiplies this transform by b, overwrites this
  //! transform with the result, and returns a reference to this transform.
  T4 & operator*=(const T4 & b) { (*this) = (*this) * b; return(*this); }

  //! Transform multiplication. Multiplies this transform by b, and returns
  //! a reference to the result without modifying this transform
  T4 operator*(const T4 &b) {
    T4 d;
    int i,j,k;
    d.zero();
    for(i=0;i<4;i++) for(j=0;j<4;j++) for(k=0;k<4;k++)
      d.e[i][j] += b.e[i][k] * this->e[k][j];
    return d;
  }

  int equals(const T4 &b, float epsilon=0.0001) const {
    int i,j;
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	if (fabs(e[i][j] - b.e[i][j]) >= epsilon)
	  return 0;
    return 1;
  }

  //! Applies this transform to point a and returns the transformed point
  R3 apply(const R3 & a) {
    R3 b;
    float w;
    b.X = a.X * e[0][0] + a.Y * e[0][1] + a.Z * e[0][2] + e[0][3];
    b.Y = a.X * e[1][0] + a.Y * e[1][1] + a.Z * e[1][2] + e[1][3];
    b.Z = a.X * e[2][0] + a.Y * e[2][1] + a.Z * e[2][2] + e[2][3];
    w = e[3][0] + e[3][1] + e[3][2] + e[3][3];
    b /= w;
    return b;
  }

  //! Sets all coefficients to zero (null transform)
  void zero() {
    int i,j; for(i=0;i<4;i++) for(j=0;j<4;j++) e[i][j]=0.0; }

  //! Sets an identity transform
  void identity() { zero(); e[0][0] = e[1][1] = e[2][2] = e[3][3] = 1.0; }

  //! Sets a rotation around X-axis transform, by angle degrees.
  //! Rotation center is given by (cx,cy,cz)
  void xrot(float angle, float cx, float cy, float cz) {
    T4 a,b,c;
    a.translate(-cx,-cy,-cz);
    b.xrot(angle);
    c.translate(cx,cy,cz);
    a *= b;
    a *= c;
    (*this) = a;
  }

  //! Sets a rotation around Y-axis transform, by angle degrees.
  //! Rotation center is given by (cx,cy,cz)
  void yrot(float angle, float cx, float cy, float cz) {
    T4 a,b,c;
    a.translate(-cx,-cy,-cz);
    b.yrot(angle);
    c.translate(cx,cy,cz);
    a *= b;
    a *= c;
    (*this) = a;
  }

  //! Sets a rotation around Z-axis transform, by angle degrees.
  //! Rotation center is given by (cx,cy,cz)
  void zrot(float angle, float cx, float cy, float cz) {
    T4 a,b,c;
    a.translate(-cx,-cy,-cz);
    b.zrot(angle);
    c.translate(cx,cy,cz);
    a *= b;
    a *= c;
    (*this) = a;
  }

  //! Sets a rotation around X-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void xrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[1][1] = cos(angle);  e[2][1] = sin(angle);
    e[1][2] = -sin(angle); e[2][2] = cos(angle);
  }

  //! Sets a rotation around Y-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void yrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[0][0] = cos(angle);  e[2][0] = sin(angle);
    e[0][2] = -sin(angle); e[2][2] = cos(angle);
  }

  //! Sets a rotation around Z-axis transform, by angle degrees.
  //! Rotation center is the origin.
  void zrot(float angle) {
    identity();
    angle *= M_PI / 180.0;
    e[0][0] = cos(angle);  e[1][0] = sin(angle);
    e[0][1] = -sin(angle); e[1][1] = cos(angle);
  }

  // rotation about arbitrary axis, around (cx,cy,cz)
  void axisrot(const R3 &axis, float angle, float cx, float cy, float cz) {
    T4 t,r,ti;
    t.translate(-cx,-cy,-cz);
    r.axisrot(axis,angle);
    ti.translate(cx,cy,cz);
    t *= r;
    t *= ti;
    (*this) = t;
  }

  // rotation about arbitrary axis, around the origin
  // Hearn et al, 1986, Computer Graphics, p. 224 (ISBN 0131653822)
  void axisrot(const R3 &axis, float angle) {
    R3 au;
    T4 rx, ry, rz, ryi, rxi;
    double a,b,c,d;

    au = axis;
    au.normalize();
    
    a = au.X;
    b = au.Y;
    c = au.Z;
    d = sqrt(b*b+c*c);
    if (d == 0.0) {
      xrot( a > 0 ? angle : -angle );
      return;
    }

    rx.set(1, 0,   0,   0,
	   0, c/d, b/d, 0,
	   0,-b/d, c/d, 0,
	   0, 0,   0,   1);

    ry.set(  d, 0, a, 0,
	     0, 1, 0, 0,
	    -a, 0, d, 0,
	     0, 0, 0, 1);

    rz.zrot(angle);

    ryi.set(  d, 0,-a, 0,
	      0, 1, 0, 0,
	      a, 0, d, 0,
	      0, 0, 0, 1);

    rxi.set(1, 0,    0,   0,
	    0, c/d, -b/d, 0,
	    0, b/d,  c/d, 0,
	    0, 0,    0,   1);

    rx *= ry;
    rx *= rz;
    rx *= ryi;
    rx *= rxi;
    (*this) = rx;
  }

  //! Sets a shearing transform along the X axis
  void xshear(float yf, float zf) {
    identity(); e[0][1] = yf; e[0][2] = zf;
  }

  //! Sets a shearing transform along the Y axis
  void yshear(float xf, float zf) {
    identity(); e[1][0] = xf; e[1][2] = zf;
  }

  //! Sets a shearing transform along the Z axis
  void zshear(float xf, float yf) {
    identity(); e[2][0] = xf; e[2][1] = yf;
  }

  //! Sets a scaling transform, by factor on all dimensions
  void scale(float factor) {
    zero(); e[0][0]=e[1][1]=e[2][2]=factor; e[3][3] = 1.0;
  }

  //! Sets a scaling transform, by factors (fx,fy,fz)
  void scale(float fx, float fy, float fz) {
    zero(); e[0][0]=fx; e[1][1]=fy; e[2][2]=fz; e[3][3] = 1.0;
  }

  //! Sets a translation transform, by (dx,dy,dz)
  void translate(float dx, float dy, float dz) {
    identity(); e[0][3] = dx; e[1][3] = dy; e[2][3] = dz;
  }

  void set(float c11,float c21,float c31,float c41,
	   float c12,float c22,float c32,float c42,
	   float c13,float c23,float c33,float c43,
	   float c14,float c24,float c34,float c44) {
    e[0][0] = c11; e[1][0] = c21; e[2][0] = c31; e[3][0] = c41;
    e[0][1] = c12; e[1][1] = c22; e[2][1] = c32; e[3][1] = c42;
    e[0][2] = c13; e[1][2] = c23; e[2][2] = c33; e[3][2] = c43;
    e[0][3] = c14; e[1][3] = c24; e[2][3] = c34; e[3][3] = c44;
  }

  //! Computes the inverse transform
  void invert() { minv4(); }

  //! Prints the transform matrix on stdout
  void print() {
    int i,j;
    for(i=0;i<4;i++) {
      for(j=0;j<4;j++) {
	printf("%.2f ",e[j][i]);
      }
      printf("\n");
    }
    printf("\n");
  }

  friend ostream & operator<<(ostream& s, T4 &t) {
    int i,j;
    s << "T4 [ ";
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	s << t.e[j][i] << " ";
    s << "]\n";
    return s;
  }

  friend istream & operator>>(istream& s, T4 &t) {
    char l[128];
    Tokenizer k;
    int i,j;
    s.getline(l,128);
    k.setString(l);
    k.setSeparator(" \r\n\t");
    if (k.countTokens() != 19) goto t4inputerror;
    if (strcmp(k.nextToken(),"T4")!=0) goto t4inputerror;
    if (strcmp(k.nextToken(),"[")!=0) goto t4inputerror;
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	t.e[j][i] = atof(k.nextToken());
    if (strcmp(k.nextToken(),"]")!=0) goto t4inputerror;
    return s;
 t4inputerror:
    cerr << "T4::operator>> : bad syntax reading matrix data.\n";
    t.identity();
    return s;
  }

 private:
  float e[4][4];

  bool minv4() {
    int *sle;
    float *sq0, *a;
    int n = 4; // matrix size

    int lc,*le; float s,t,tq=0.,zr=1.e-15;
    float *pa,*pd,*ps,*p,*q,*q0;
    int i,j,k,m;
    
    sle = new int[n];
    sq0 = new float[n];
    a = new float[n*n];

    for(i=0;i<n*n;i++) a[i] = e[i%n][i/n];

    le = sle;
    q0 = sq0;
    
    for(j=0,pa=pd=a; j<n ;++j,++pa,pd+=n+1){
      if(j>0){
	for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
	for(i=1; i<n ;++i){ lc=i<j?i:j;
	for(k=0,p=pa+i*n-j,q=q0,t=0.; k<lc ;++k) t+= *p++ * *q++;
	q0[i]-=t;
	}
	for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
      }
      
      s=fabs(*pd); lc=j;
      for(k=j+1,ps=pd; k<n ;++k){
	if((t=fabs(*(ps+=n)))>s){ s=t; lc=k;}
      }
      tq=tq>s?tq:s; if(s<zr*tq){ return false;}
      *le++ =lc;
      if(lc!=j){
	for(k=0,p=a+n*j,q=a+n*lc; k<n ;++k){
	  t= *p; *p++ = *q; *q++ =t;
	}
      }
      for(k=j+1,ps=pd,t=1./ *pd; k<n ;++k) *(ps+=n)*=t;
      *pd=t;
    }
    for(j=1,pd=ps=a; j<n ;++j){
      for(k=0,pd+=n+1,q= ++ps; k<j ;++k,q+=n) *q*= *pd;
    }
    for(j=1,pa=a; j<n ;++j){ ++pa;
    for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *q++ = *p;
    for(k=0; k<j ;++k){ t=0.;
    for(i=k,p=pa+k*n+k-j,q=q0+k; i<j ;++i) t-= *p++ * *q++;
    q0[k]=t;
    }
    for(i=0,q=q0,p=pa; i<j ;++i,p+=n) *p= *q++;
    }
    for(j=n-2,pd=pa=a+n*n-1; j>=0 ;--j){ --pa; pd-=n+1;
    for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *q++ = *p;
    for(k=n-1,ps=pa; k>j ;--k,ps-=n){ t= -(*ps);
    for(i=j+1,p=ps,q=q0; i<k ;++i) t-= *++p * *q++;
    q0[--m]=t;
    }
    for(i=0,m=n-j-1,q=q0,p=pd+n; i<m ;++i,p+=n) *p= *q++;
    }
    for(k=0,pa=a; k<n-1 ;++k,++pa){
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *q++ = *p;
      for(j=0,ps=a; j<n ;++j,ps+=n){
	if(j>k){ t=0.; p=ps+j; i=j;}
	else{ t=q0[j]; p=ps+k+1; i=k+1;}
	for(; i<n ;) t+= *p++ *q0[i++];
	q0[j]=t;
      }
      for(i=0,q=q0,p=pa; i<n ;++i,p+=n) *p= *q++;
    }
    for(j=n-2,le--; j>=0 ;--j){
      for(k=0,p=a+j,q=a+ *(--le); k<n ;++k,p+=n,q+=n){
	t=*p; *p=*q; *q=t;
      }
    }
    delete sq0;
    delete sle;
    for(i=0;i<n*n;i++) e[i%n][i/n] = a[i];
    delete a;
    return true;
  } // minv4
};

//! Spherical coordinate mapping for 3D space
class Sphere {
 public:
  //! Constructor, creates a sphere with the given center
  Sphere(const R3 &center) { Center = center; }

  //! Constructor, creates a sphere centered ad (x,y,z)
  Sphere(int x, int y, int z) { Center.set(x,y,z); }
  
  //! Converts spherical cooordinates src to rectangular coordinates dest.
  //! (src.X,src.Y,src.Z) = (theta,phi,rho). Theta and phi are given in degrees.
  void sph2rect(R3 &dest, R3 &src) {
    R3 t;
    t = src;
    t.X *= M_PI/180.0;
    t.Y *= M_PI/180.0;
    dest.set( cos(t.X) * sin(t.Y),
	      sin(t.X) * sin(t.Y),
	      cos(t.Y) );
    dest.normalize();
    dest *= t.Z;
    dest += Center;
  }

  //! Converts rectangular coordinates src to spherical coordinates dest.
  //! (dest.X,dest.Y,dest.Z) = (theta,phi,rho). Theta and phi are given in degrees.
  void rect2sph(R3 &dest, R3 &src) {
    R3 t;
    t = src;
    t -= Center;
    dest.Z = t.length();
    t.normalize();
    dest.Y = acos(t.Z);
    if (t.X < 0)
      dest.X = M_PI - asin(t.Y / sqrt(t.X*t.X + t.Y*t.Y));
    else
      dest.X = asin(t.Y / sqrt(t.X*t.X + t.Y*t.Y));
    dest.X *= 180.0/M_PI;
    dest.Y *= 180.0/M_PI;
  }

  //! Converts spherical coordinates src to cylindrical projection
  //! coordinates dest, such that (theta,phi) span a square plane patch of
  //! side texsz. dest.Z is always 0.
  void sph2tex(R3 &dest, R3 &src, int texsz) {
    R3 t;
    t = src;
    while(t.X < 0.0) t.X += 360.0;
    while(t.Y < 0.0) t.Y += 360.0;
    dest.X = (t.X * texsz) / 360.0;
    dest.Y = (t.Y * texsz) / 180.0;
    dest.Z = 0.0;
  }

  //! Converts rectangular coordinates src to cylindrical projection
  //! coordinates dest, such that (theta,phi) span a square plane patch of
  //! side texsz. dest.Z is always 0.
  void rect2tex(R3 &dest, R3 &src, int texsz) {
    R3 s;
    rect2sph(s,src);
    sph2tex(dest,s,texsz);
  }

 private:
  R3 Center;
};

//! A timestamp for measuring time intervals
class Timestamp {
 public:
  //! Constructor, creates a timestamp with values (sec,usec,big) for
  //! seconds and microseconds elapsed since last midnight
  Timestamp(int sec, int usec) { set(sec,usec); }

  //! Constructor, creates a timestamp with the current time
  Timestamp() { (*this) = Timestamp::now(); }

  //! Assignment operator
  Timestamp & operator=(const Timestamp &t) { S = t.S; U=t.U; return(*this); }

  //! Subtraction, returns the number of seconds elapsed between this
  //! timestamp and t.
  double operator-(Timestamp &t) {
    int msec;
    double sec;
    msec = 1000*(S-t.S);
    msec += (U/1000 - t.U/1000);
    sec = (double) (msec / 1000.0);
    return sec;
  }

  //! Sets the current timestamp
  void set(int sec, int usec) { S=sec; U=usec; }

  //! Returns the current timestamp
  static Timestamp now() { 
    Timestamp t(0,0);
    struct timeval tv;
    gettimeofday(&tv,0);
    t.S = (int) tv.tv_sec;
    t.U = (int) tv.tv_usec;
    return(t);
  }

 private:
  int S,U;
};

//! Mutual exclusion
class Mutex {
 public:
  //! Constructor, creates a new mutex object
  Mutex()  { pthread_mutex_init(&m,0); }

  //! Acquires the mutex. Beware of deadlocks.
  void lock()   { pthread_mutex_lock(&m);   }

  //! Releases the mutex. Beware of deadlocks.
  void unlock() { pthread_mutex_unlock(&m); }
 private:
  pthread_mutex_t m;
};

//! 3-component color (RGB, YCbCr)
class Color {
 public:
  uint8_t R,G,B;

  //! Constructor, creates black color
  Color() { R=G=B=0; }

  //! Constructor, creates color with (R,G,B) = (a,b,c)
  Color(int a,int b,int c) { R=(uint8_t)a; G=(uint8_t)b; B=(uint8_t)c; }

  //! Constructor, creates color with (R,G,B) = (a,b,c)
  Color(uint8_t a,uint8_t b,uint8_t c) { R=a; G=b; B=c; }

  //! Copy constructor
  Color(const Color &c) { R=c.R; G=c.G; B=c.B; }

  //! Constructor, creates color from integer representation.
  //! Integer representation: bits 0-7: blue; bits 8-15: green;
  //! bits 16-23: red; bits 24-31: ignored.
  Color(int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); }

  //! Comparison operator
  bool operator==(Color &c) { return(c.R==this->R && c.G==this->G && c.B==this->B); }

  //! Comparison operator
  bool operator!=(Color &c) { return(c.R!=this->R || c.G!=this->G || c.B!=this->B); }

  //! Assignment operator
  Color & operator=(const Color &c) { R=c.R; G=c.G; B=c.B; return(*this); }

  //! Assignment operator, from integer representation. See \ref Color(int c)
  Color & operator=(const int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); return(*this); }

  //! Assignment operator, from integer representation. See \ref Color(int c)
  Color & operator=(const unsigned int c) { R=((c>>16)&0xff); G=((c>>8)&0xff); B=(c&0xff); return(*this); }

  //! Returns the integer representation of this color. See \ref Color(int c)
  int toInt() { return( ( ((int)R)<<16)|(((int)G)<<8)|((int)B) ); }

  //! Lightens or darkens this color by factor x. The factor multiplies the
  //! luminance component in the YCbCr representation of this color.
  Color & operator*=(float x) {
    float fr;
    rgb2ycbcr();
    fr = R*x; if (fr > 255.0) fr=255.0;
    R = (uint8_t) fr;
    ycbcr2rgb();
    return(*this);
  }

  //! Luminance multiplication operator
  Color operator*(float x) {
    Color c;
    c = (*this);
    c *= x;
    return(c);
  }

  //! sets a gray color, with (R,G,B) = (c,c,c), returns a reference
  //! to this color.
  Color & gray(int c) { R = G = B = (uint8_t) c; return(*this); }

  //! Mixes color src in this color, with proportion srcamount of src
  //! and (1-srcamount) of this color. Overwrites this color with
  //! the result and returns a reference to this color.
  Color & mix(Color &src, float srcamount) {
    R = (uint8_t) ((1.0 - srcamount)*(this->R) + (srcamount*(src.R)));
    G = (uint8_t) ((1.0 - srcamount)*(this->G) + (srcamount*(src.G)));
    B = (uint8_t) ((1.0 - srcamount)*(this->B) + (srcamount*(src.B)));
    return(*this);
  }

  //! Converts an RGB color triplet to YCbCr
  void rgb2ycbcr() {
    float y,cb,cr;
    
    y=0.257*(float)R+0.504*(float)G+0.098*(float)B+16.0;
    cb=-0.148*(float)R-0.291*(float)G+0.439*(float)B+128.0;
    cr=0.439*(float)R-0.368*(float)G-0.071*(float)B+128.0;
    R = (uint8_t) y; G = (uint8_t) cb; B = (uint8_t) cr;
  }

  //! Converts an YCbCr color triplet to RGB
  void ycbcr2rgb() {
    float r,g,b;

    r=1.164*((float)R-16.0)+1.596*((float)B-128.0);
    g=1.164*((float)R-16.0)-0.813*((float)B-128.0)-0.392*((float)G-128.0);
    b=1.164*((float)R-16.0)+2.017*((float)G-128.0);

    if (r<0.0) r=0.0;
    if (g<0.0) g=0.0;
    if (b<0.0) b=0.0;
    if (r>255.0) r=255.0;
    if (g>255.0) g=255.0;
    if (b>255.0) b=255.0;
    R=(uint8_t)r; G=(uint8_t)g; B=(uint8_t)b;
  }
};


//! Interface for 2D paintable classes
class Paintable {
 public:
  //! Sets the color of (x,y) to c
  virtual void set(const int x,const int y,Color &c)=0;

  //! Returns the integer representation of the color of pixel (x,y)
  virtual int  get(const int x,const int y)=0;

  virtual ~Paintable() { }
};


//! Font renderer based on FreeType2 
class Font {
 public:
  //! Constructor
  Font() { if (!inited) init(); }

  //! Constructor, loads font from filename at size ptsize
  Font(const char *filename, int ptsize) {
    if (!inited) init();
    load(filename,ptsize);
  }

  //! Loads a new font from filename, at size ptsize
  void load(const char *filename, int ptsize) {
    FT_New_Face(library,filename,0,&face);
    FT_Set_Char_Size(face, 0, ptsize*64, 0, 0);
    H = face->size->metrics.height / 64;
  }

  //! Returns the character height for the current font, in pixels
  int height() { return H; }

  //! Returns the width of the given text string with the current font,
  //! in pixels.
  int width(const char *text) {
    FT_GlyphSlot slot = face->glyph;
    int w = 0, len, n;
    len = strlen(text);
    for (n=0;n<len;n++) { 
      FT_Load_Char( face, text[n], FT_LOAD_RENDER );     
      w += slot->advance.x >> 6;
    }
    return w;
  }

  //! Renders text at (x,y) with \ref Color c on the \ref Paintable p with
  //! given opacity (0..1). (x,y) is the top left corner of the rendering
  //! rectangle.
  void render(const char *text, int x, int y, Color &c, Paintable *p, float opacity=1.0) {
    FT_GlyphSlot slot = face->glyph;
    int pen_x=x, pen_y=y+H, n, len, i, j, w, dx, dy;
    Color d;
    unsigned char g;

    len = strlen(text);
    for (n=0;n<len;n++) { 
      FT_Load_Char( face, text[n], FT_LOAD_RENDER );
      
      dx = slot->bitmap_left;
      dy = slot->bitmap_top;
      w = slot->bitmap.width;
      for(j=0;j<(int) slot->bitmap.rows;j++)
	for(i=0;i<(int) slot->bitmap.width;i++) {
	  g = slot->bitmap.buffer[i+j*w];
	  if (g > 10) {
	    d = p->get(pen_x+i+dx,pen_y+j-dy);
	    d.mix(c,opacity * ((float)g)/255.0);
	    p->set(pen_x+i+dx,pen_y+j-dy,d);
	  }
	}

      pen_x += slot->advance.x >> 6;
    }
  }

 private:
  FT_Face face;
  static bool inited;
  static FT_Library library;
  int H;

  void init() { FT_Init_FreeType(&library); inited=true; }
};

#ifndef NO_STATIC
FT_Library Font::library;
bool Font::inited = false;
#endif

//! 2D RGB Image
class Image : public Paintable {
 public:
  int W /** Image width */,H /** Image height */,N /** Pixel count */;

  //! Constructor, creates a w x h image filled with black
  Image(int w,int h) {
    Color black;
    data = new uint8_t[w*h*3];
    W=w;
    H=h;
    N = w*h;
    fill(black);
  }

  //! Copy constructor
  Image(Image &img) {
    W=img.W;
    H=img.H;
    N=img.N;
    data = new uint8_t[W*H*3];
    memcpy(this->data, img.data, 3*N);
  }

  //! Construtor, creates an image from a P5/P6 PPM file
  Image(const char *filename) {
    ifstream f;
    char tmp[61],*p;
    int i;
    data = 0;
    W=H=N=0;
    f.open(filename);
    if (f.good()) {
      f.getline(tmp,60);
      if (!strcmp(tmp,"P6")) {
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	p = strchr(tmp,' '); if (!p) p = strchr(tmp,'\t');
	*p = 0; W = atoi(tmp); H = atoi(p+1);
	N = W*H;
	data = new uint8_t[W*H*3];
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	for(i=0;i<H;i++)
	  f.read((char *) (&data[3*W*i]),3*W);
      }
      if (!strcmp(tmp,"P5")) {
	uint8_t *tgray;
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	p = strchr(tmp,' '); if (!p) p = strchr(tmp,'\t');
	*p = 0; W = atoi(tmp); H = atoi(p+1);
	N = W*H;
	tgray = new uint8_t[W*H];
	data = new uint8_t[W*H*3];
	do { f.getline(tmp,60); } while(tmp[0]=='#');
	for(i=0;i<H;i++)
	  f.read((char *) (&tgray[W*i]),W);
	for(i=0;i<N;i++)
	  data[3*i] = data[3*i+1] = data[3*i+2] = tgray[i];
	delete tgray;
      }
      f.close();
    }
  }

  //! Constructor, creates an image from an included XPM image (xpm).
  //! Transparent pixels are replaced by the \ref Color transp.
  Image(char **xpm, Color &transp) {
    readXPM(xpm,transp);
  }

  //! Destructor
  virtual ~Image() {
    if (data!=0) delete[] data;
  }

  //! Saves this image to a P6 PPM file
  void writeP6(const char *filename) {
    ofstream f;
    int i;
    f.open(filename,ofstream::binary);
    if (!f.good()) return;

    f << "P6\n" << W << " " << H << "\n255\n";
    for(i=0;i<H;i++)
      f.write( (char *) (&data[i*W*3]), W*3);
    f.close();
  }

  //! Saves this image to a PNG file via libpng
  void writePNG(const char *filename) {
    FILE *f;
    png_structp png_ptr;
    png_infop info_ptr;
    uint8_t **rows;
    int i;
    
    f = fopen(filename, "w");
    if (f==NULL) return;
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    if (png_ptr==NULL) {
      // todo: print error
      fclose(f);
      return;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (png_ptr==NULL) {
      // todo: print error
      fclose(f);
      return;
    }
    setjmp(png_jmpbuf(png_ptr));
    png_init_io(png_ptr, f);
    
    png_set_IHDR(png_ptr,info_ptr,W,H,
		 8,PNG_COLOR_TYPE_RGB,
		 PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);
    
    rows = (uint8_t **) malloc(sizeof(uint8_t *) * H);
    if (rows == NULL) {
      // todo: print error
      fclose(f);
      return;
    }
    for(i=0;i<H;i++)
      rows[i] = &(data[i*W*3]);
    
    png_set_rows(png_ptr, info_ptr, rows);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(rows);
    fclose(f);
  }
  
  //! Fills all pixels with \ref Color c
  void fill(Color &c) {
    int i,p;
    for(i=0,p=0;i<N;i++) {
      data[p++] = c.R;
      data[p++] = c.G;
      data[p++] = c.B;
    }
  }

  //! Sets pixel (x,y) to \ref Color c
  virtual void set(const int x,const int y,Color &c) {
    if (valid(x,y)) {
      data[3*(x+y*W) ] = c.R;
      data[3*(x+y*W)+1 ] = c.G;
      data[3*(x+y*W)+2 ] = c.B;
    }
  }

  //! Sets pixel (x,y) to color c, given as an integer representation
  //! (see \ref Color::Color(int c))
  void set(const int x,const int y,const int c) {
    if (valid(x,y)) {
      Color d(c);
      data[3*(x+y*W) ] = d.R;
      data[3*(x+y*W)+1 ] = d.G;
      data[3*(x+y*W)+2 ] = d.B;
    }
  }

  //! Returns the color of pixel (x,y) in integer representation
  //! (see \ref Color::Color(int c))
  virtual int get(const int x,const int y) {
    if (valid(x,y)) {
      Color c;
      c.R = data[3*(x+y*W) ];
      c.G = data[3*(x+y*W) + 1];
      c.B = data[3*(x+y*W) + 2];
      return(c.toInt());
    } else return 0;
  }

  //! Returns true if (x,y) is in the image domain, false otherwise
  bool valid(const int x,const int y) { return(x>=0 && x<W && y>=0 && y<H); }

  //! Renders text at position (x,y) with \ref Color c, \ref Font f and
  //! given opacity
  void text(Font &f, const char *text, int x,int y, Color &c, float opacity=1.0) {
    f.render(text,x,y,c,(Paintable *) this,opacity);
  }

  //! Flood fills from point (x,y) with \ref Color c. Flood fill is performed
  //! with 4-neighbors adjacency. Any color different from the previous color
  //! of (x,y) halts the flood. It does nothing if (x,y) has \ref Color c.
  void floodFill(int x,int y,Color &c) {
    queue<int> fifo;
    bool *done;
    int bg,px,py,pi;
    
    bg = get(x,y);
    if (c.toInt() == bg) return;

    done = new bool[W*H];
    for(pi=0;pi<W*H;pi++) done[pi] = false;

    fifo.push(x+y*W);    
    done[x+y*W] = true;
    while(!fifo.empty()) {
      pi = fifo.front();
      done[pi] = true;
      fifo.pop();

      px = pi%W;
      py = pi/W;
      if (valid(px,py)) {
	set(px,py,c);

	if (valid(px-1,py) && get(px-1,py) == bg && !done[(px-1)+(py)*W]) {
	  fifo.push((px-1)+(py)*W);
	  done[(px-1)+(py)*W] = true;
	}
	if (valid(px+1,py) && get(px+1,py) == bg && !done[(px+1)+(py)*W]) {
	  fifo.push((px+1)+(py)*W);
	  done[(px+1)+(py)*W] = true;
	}
	if (valid(px,py-1) && get(px,py-1) == bg && !done[(px)+(py-1)*W]) {
	  fifo.push((px)+(py-1)*W);
	  done[(px)+(py-1)*W] = true;
	}
	if (valid(px,py+1) && get(px,py+1) == bg && !done[(px)+(py+1)*W]) {
	  fifo.push((px)+(py+1)*W);
	  done[(px)+(py+1)*W] = true;
	}
      }
    }
  }
  
  //! Alpha bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
  //! src to position (x,y) of this image. Pixels of color trans in src are
  //! considered transparent and are not copied. If w or h are negative,
  //! the width and/or height of src are used.
  void ablit(Image &src, Color &trans,int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;
    uint8_t r,g,b,ar,ag,ab;

    if (w<0) w=src.W;
    if (h<0) h=src.H;

    ar = trans.R;
    ag = trans.G;
    ab = trans.B;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+src.W*j);
	  r = src.data[s];
	  g = src.data[s+1];
	  b = src.data[s+2];
	  if (r!=ar || g!=ag || b!=ab) {
	    this->data[d] = r;
	    this->data[d+1] = g;
	    this->data[d+2] = b;
	  }
	} 
  }

  //! Alpha bit block increment: similar to \ref ablit, but instead of
  //! copying the src image, uses it as a mask, and increments R,G and B
  //! of the pixels on this image where a non-transparent pixel of src
  //! would be painted.
  void ablinc(Image &src, Color &trans,int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;
    uint8_t r,g,b,ar,ag,ab;

    if (w<0) w=src.W;
    if (h<0) h=src.H;

    ar = trans.R;
    ag = trans.G;
    ab = trans.B;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+src.W*j);
	  r = src.data[s];
	  g = src.data[s+1];
	  b = src.data[s+2];
	  if (r!=ar || g!=ag || b!=ab) {
	    this->data[d]++;
	    this->data[d+1]++;
	    this->data[d+2]++;
	  }
	} 
  }

  //! Bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
  //! src to position (x,y) of this image. If w or h are negative,
  //! the width and/or height of src are used.
  void blit(Image &src, int x,int y,int w=-1,int h=-1) {
    int i,j,s,d;

    if (w<0) w=src.W;
    if (h<0) h=src.H;
    
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(x+i,y+j) && src.valid(i,j)) {
	  d = 3*(x+i+W*(y+j));
	  s = 3*(i+(src.W)*j);
	  this->data[d] = src.data[s];
	  this->data[d+1] = src.data[s+1];
	  this->data[d+2] = src.data[s+2];
	} 
  }

  //! Bit block transfer: copies rectangle (sx,sy)-(sx+w-1,sy+h-1) of Image
  //! src to position (dx,dy) of this image. If w or h are negative,
  //! the width and/or height of src are used.
  void blit(Image &src, int sx, int sy, int dx,int dy,int w=-1,int h=-1) {

    int i,j,s,d;

    if (sx + w > src.W) w = src.W-sx;
    if (sy + h > src.H) h = src.H-sy;
    if (w<0) w=src.W;
    if (h<0) h=src.H;
    if (dx + w > W) w = W-dx;
    if (dy + h > H) h = H-dy;
    if (w < 0 || h < 0) return;

    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	if (valid(dx+i,dy+j) && src.valid(sx+i,sy+j)) {
	  d = 3*(dx+i+W*(dy+j));
	  s = 3*(sx+i+(src.W)*(sy+j));
	  this->data[d] = src.data[s];
	  this->data[d+1] = src.data[s+1];
	  this->data[d+2] = src.data[s+2];
	} 
  }
  
  //! Returns a pointer to the RGB buffer
  unsigned char *getBuffer() {
    return((unsigned char *)data);
  }

  //! Draws a filled translucent rectangle with top left corner (x,y), 
  //! size (w,h), \ref Color src and opacity srcamount
  void blendbox(int x, int y, int w, int h, Color &src, float srcamount) {
    int i,j,p;
    Color c;
    if (x >= W) return;
    if (y >= H) return;
    if (x<0) { w+=x; x=0; }
    if (y<0) { h+=y; y=0; }
    if (x+w >= W) w=W-x-1;
    if (y+h >= H) h=H-y-1;
    if (w<0 || h<0) return;
    p=3*(x+y*W);
    for(i=0;i<h;i++) {
      for(j=0;j<w;j++) {
	c.R = data[p+3*j ];
	c.G = data[p+3*j+1 ];
	c.B = data[p+3*j+2 ];
	c.mix(src,srcamount);
	data[p+3*j ] = c.R;
	data[p+3*j+1 ] = c.G;
	data[p+3*j+2 ] = c.B;
      }
      p += 3*W;
    }    
  }

  //! Shades a filled rectangle with top left corner (x,y) and size
  //! (w,h) by multiplying the YCbCr luminance of each pixel by factor
  void shadebox(int x, int y, int w, int h, float factor) {
    int i,j,p;
    Color c;
    if (x >= W) return;
    if (y >= H) return;
    if (x<0) { w+=x; x=0; }
    if (y<0) { h+=y; y=0; }
    if (x+w >= W) w=W-x-1;
    if (y+h >= H) h=H-y-1;
    if (w<0 || h<0) return;
    p=3*(x+y*W);
    for(i=0;i<h;i++) {
      for(j=0;j<w;j++) {
	c.R = data[p+3*j ];
	c.G = data[p+3*j+1 ];
	c.B = data[p+3*j+2 ];
	c *= factor;
	data[p+3*j ] = c.R;
	data[p+3*j+1 ] = c.G;
	data[p+3*j+2 ] = c.B;
      }
      p += 3*W;
    }    
  }

  //! Draws a rectangle with top left corner (x,y), size (w,h) and
  //! \ref Color c. If fill is true, the rectangle is filled.
  void rect(int x, int y, int w, int h, Color &c, bool fill=true) {
    int i,j,p;
    if (!fill) {
      line(x,y,x+w-1,y,c);
      line(x,y+h-1,x+w-1,y+h-1,c);
      line(x,y,x,y+h-1,c);
      line(x+w-1,y,x+w-1,y+h-1,c);
    } else {
      if (x >= W) return;
      if (y >= H) return;
      if (x<0) { w+=x; x=0; }
      if (y<0) { h+=y; y=0; }
      if (x+w >= W) w=W-x-1;
      if (y+h >= H) h=H-y-1;
      if (w<0 || h<0) return;
      p=3*(x+y*W);
      for(i=0;i<h;i++) {
	for(j=0;j<w;j++) {
	  data[p+3*j ] = c.R;
	  data[p+3*j+1 ] = c.G;
	  data[p+3*j+2 ] = c.B;
	}
	p += 3*W;
      }
    }
  }

  //! Draws a line segment between points (x1,y1) and (x2,y2), with
  //! \ref Color c.
  void line(int x1,int y1,int x2,int y2,Color &c) {
    int x, y;
    int dy = y2 - y1;
    int dx = x2 - x1;
    int G, DeltaG1, DeltaG2;	
    int swap;
    int inc = 1;

    if (x1>=0 && y1>=0 && x1<W && y1<H)
      set(x1,y1,c);
  
    if (abs(dy) < abs(dx)) {
      /* -1 < slope < 1 */
      if (dx < 0) {
	dx = -dx; dy = -dy;      
	swap = y2; y2 = y1; y1 = swap;	
	swap = x2; x2 = x1; x1 = swap;
      }
      
      if (dy < 0) { dy = -dy; inc = -1; }
      
      y = y1; x = x1 + 1;      
      G = 2 * dy - dx; DeltaG1 = 2 * (dy - dx); DeltaG2 = 2 * dy;
    
      while (x <= x2) {
	if (G > 0) { G += DeltaG1; y += inc; } 
	else G += DeltaG2;	
	if (x>=0 && y>=0 && x<W && y<H) set(x,y,c);
	x++;
      }
    } else {
      /* slope < -1 or slope > 1 */
      if (dy < 0) { 
	dx = -dx; dy = -dy;
	swap = y2; y2 = y1; y1 = swap;	
	swap = x2; x2 = x1; x1 = swap;
      }      
      if (dx < 0) { dx = -dx; inc = -1; }
      
      x = x1; y = y1 + 1;      
      G = 2 * dx - dy; DeltaG1 = 2 * (dx - dy);
      DeltaG2 = 2 * dx;
      
      while (y <= y2) {
	if (G > 0) { G += DeltaG1; x += inc; } 
	else G += DeltaG2;
	
	if (x>=0 && y>=0 && x<W && y<H) set(x,y,c);
	y++;
      }
    }
  }

  //! Scales this image by factor, and returns the new scaled image result. 
  //! It does not modify this image.
  Image *scale(float factor) {
      int nw,nh,ow,oh;
      Image *dest;
      float x1,x2,y1,y2,fi,fj,dx,dy,fr,fg,fb,di;
      int i,j,k,a,b;
      uint8_t R,G,B;
      
      uint8_t *lookup[3];
      int *area;
      int  count;

      if (factor > 1.0) return(scaleUp(factor));

      nw = (int) ((float)(W) * factor);
      nh = (int) ((float)(H) * factor);
      ow = W;
      oh = H;
      if (nw<=0 || nh<=0) return 0;

      dest = new Image(nw,nh);
      lookup[0] = new uint8_t[ow*oh];
      lookup[1] = new uint8_t[ow*oh];
      lookup[2] = new uint8_t[ow*oh];
      area   = new int[ow*oh];
      if (!lookup[0] || !area) {
	delete dest;
	return 0;
      }

      for(j=0;j<nh;j++)
	for(i=0;i<nw;i++) {
	  
	  fi = (float) i;
	  fj = (float) j;
	  x1 = fi / factor;
	  x2 = (fi+1.0) / factor;
	  y1 = fj / factor;
	  y2 = (fj+1.0) / factor;
	  
	  di = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	  
	  count = 0;
	  for(b=(int)y1;b<=(int)y2;b++)
	    if (b>=0 && b<oh)
	      for(a=(int)x1;a<=(int)x2;a++)
		if (a>=0 && a<ow) {
		  k = 3*(a+b*ow);
		  lookup[0][count]=data[k];
		  lookup[1][count]=data[k+1];
		  lookup[2][count]=data[k+2];
		  dx = (a-x1);
		  dy = (b-y1);
		  area[count] = (int) (100.0*(di - sqrt(dx*dx+dy*dy)));
		  count++;
		}
	  
	  a = 0;
	  for(b=0;b<count;b++)
	    a+=area[b];
	  
	  fb=fg=fr=0.0;
	  for(b=0;b<count;b++) {
	    fr += ((float)(area[b])) * ((float)(lookup[0][b]));
	    fg += ((float)(area[b])) * ((float)(lookup[1][b]));
	    fb += ((float)(area[b])) * ((float)(lookup[2][b]));
	  }
	  fr /= (float) a;
	  fg /= (float) a;
	  fb /= (float) a;

	  R = (uint8_t) fr;
	  G = (uint8_t) fg;
	  B = (uint8_t) fb;

	  k = 3*(i+j*nw);
	  dest->data[k] = R;
	  dest->data[k+1] = G;
	  dest->data[k+2] = B;
	}

      delete area;
      delete lookup[0];
      delete lookup[1];
      delete lookup[2];
      return dest;
  }

 private:
  uint8_t *data;

  Image *scaleUp(float factor) {
    int nw,nh,ow,oh;
    float fi,fj;
    int i,j,x,y;
    uint8_t *s,*d;
    Image *dest;

    nw = (int) ((float)(W) * factor);
    nh = (int) ((float)(H) * factor);
    ow = W;
    oh = H;

    dest = new Image(nw,nh);
    if (!dest) return 0;

    for(j=0;j<nh;j++)
      for(i=0;i<nw;i++) {

	fi = (float) i;
	fj = (float) j;

	x = (int) (fi / factor);
	y = (int) (fj / factor);

	if (x>=0 && x<ow && y>=0 && y<oh) {
	  d = &(dest->data[3*(i+nw*j)]);
	  s = &(this->data[3*(x+ow*y)]);
	  *(d++) = *(s++);
	  *(d++) = *(s++);
	  *d = *s;
	}
      }
    return dest;
  }

  void readXPM(char **xpm,Color &transp) {
    int i,j,k,line,npix;
    Tokenizer t;

    map<string, Color> colortable;
    map<string, Color>::iterator cti;

    char *key, *p;
    Color c;

    int ncolors, cpp;

    W=H=N=0;
    data=NULL;

    t.setString(xpm[0]);
    if (t.countTokens()!=4) return;
    
    W       = t.nextInt();
    H       = t.nextInt();
    ncolors = t.nextInt();
    cpp     = t.nextInt();

    N = W*H;
    data = new uint8_t[W*H*3];
    c = 0;
    fill(c);

    line = 1;
    key = (char *) malloc(cpp+1);
    key[cpp]=0;

    for(i=0;i<ncolors;i++,line++) {

      for(j=0;j<cpp;j++) key[j] = xpm[line][j];

      t.setString(xpm[line]+cpp);
      p = t.nextToken();
      if (p==NULL) continue;
      c = 0;
      if (!strcasecmp(p,"c")) {
	p = t.nextToken();
	if (!strcasecmp(p,"none"))
	  c = transp;
	else if (p[0]=='#') {	  
	  c = (unsigned int) strtol(&p[1],NULL,16);
	}
      }
      string s(key);
      colortable.insert( make_pair(s,c) );
    }

    for(j=0;j<H;j++) {
      npix = strlen(xpm[line+j]) / cpp;
      if (npix > W) npix = W;
      for(i=0;i<npix;i++) {
	
	for(k=0;k<cpp;k++)
	  key[k] = xpm[line+j][cpp*i+k];

	cti = colortable.find(key);
	if (cti == colortable.end()) {
	  c = 0;
	} else
	  c = cti->second;

	set(i,j,c);
      }
    }

    free(key);
    colortable.clear();
  }

};

//! Abstract priority queue interface
class PriorityQueue {
 public:
  /** Tag: element was never queued */
  static const int White = 0;
  /** Tag: element is queued */ 
  static const int Gray  = 1;
  /** Tag: element was queued and dequeued */
  static const int Black = 2; 
  /** Internal list tag */ 
  static const int Nil   = -1;

  virtual ~PriorityQueue() { }

  //! Returns true if there are no elements queued
  virtual bool empty()=0;

  //! Empties the queue and turn all elements White
  virtual void reset()=0;

  //! Inserts element elem into the given bucket
  virtual void insert(int elem, int bucket)=0;

  //! Removes and returns the next element to be dequeued
  virtual int  remove()=0;

  //! Moves element elem from bucket <b>from</b> to bucket <b>to</b>
  virtual void update(int elem, int from, int to)=0;

  //! Returns the color tag of an element
  virtual int colorOf(int elem)=0;
};

//! Basic linear-time priority queue
class BasicPQ : public PriorityQueue {
 public:

  //! Constructor, creates a priority queue for elements 0..nelem-1 and
  //! up to nbucket priority buckets
  BasicPQ(int nelem, int nbucket) {
    NB = nbucket;
    NE = nelem;
    first = new int[NB];
    last  = new int[NB];
    prev = new int[NE];
    next = new int[NE];
    color = new char[NE];

    reset();
  }

  //! Destructor
  virtual ~BasicPQ() {
    if (first) delete first;
    if (last) delete last;
    if (prev) delete prev;
    if (next) delete next;
    if (color) delete color;
  }

  //! Returns true if the queue is empty
  bool empty() {
    int i;
    for(i=current;i<NB;i++)
      if (first[i] != Nil)
	return false;
    return true;
  }

  //! Empties the queue and turn all elements White
  void reset() { 
    int i;
    current = 0;

    for(i=0;i<NB;i++)
      first[i] = last[i] = Nil;

    for(i=0;i<NE;i++) {
      prev[i] = next[i] = Nil;
      color[i] = White;
    }
  }

  //! Inserts element elem into the given bucket
  void insert(int elem, int bucket) { 

    #ifdef SAFE
    assert(elem>=0 && elem<NE);
    assert(bucket>=0 && bucket<NB);
    #endif

    if (first[bucket] == Nil) {
      first[bucket] = elem;
      prev[elem] = Nil;
    } else {
      #ifdef SAFE
        assert(last[bucket]>=0 && last[bucket]<NE);
      #endif
      next[last[bucket]] = elem;
      prev[elem] = last[bucket];
    }
    last[bucket] = elem;
    next[elem] = Nil;
    color[elem] = Gray;
    if (bucket < current) current = bucket;
  }

  //! Removes and returns the next element to be dequeued
  int remove() { 
    int elem,n;

    #ifdef SAFE
    assert(current>=0 && current < NB);
    #endif

    while(first[current]==Nil && current < NB)
      ++current;
    if (current == NB) return Nil;
    elem = first[current];

    n = next[elem];
    if (n==Nil) {
      first[current] = last[current] = Nil;
    } else {
      first[current] = n;
      prev[n] = Nil;
    }
    color[elem] = Black;
    return elem;
  }

  //! Moves element elem from bucket <b>from</b> to bucket <b>to</b>
  void update(int elem, int from, int to) { 
    removeElem(elem,from);
    insert(elem,to);
  }

  //! Returns the color tag of an element
  int colorOf(int elem) { return((int)(color[elem])); }

 private:
  int NB,NE,current,*first, *last,*prev,*next;
  char *color;

  void removeElem(int elem, int bucket) {
    int p,n;

    #ifdef SAFE
    assert(elem>=0 && elem<NE);
    assert(bucket>=0 && bucket<NB);
    #endif

    p = prev[elem];
    n = next[elem];

    color[elem] = Black;
    
    if (first[bucket] == elem) {
      first[bucket] = n;
      if (n==Nil)
	last[bucket] = Nil;
      else
	prev[n] = Nil;
    } else {
      next[p] = n;
      if (n==Nil)
	last[bucket] = p;
      else
	prev[n] = p;
    }
  }

};

//! Point in discrete 3D space
class Location {
 public:
  int X /** X coordinate */,Y /** Y coordinate */,Z /** Z coordinate */;

  //! Constructor
  Location() { X=Y=Z=0; }

  //! Constructor, sets (X,Y,Z)=(x,y,z)
  Location(int x,int y,int z) { X=x; Y=y; Z=z; }

  //! Copy constructor
  Location(const Location &src) { X=src.X; Y=src.Y; Z=src.Z; }

  //! Copy constructor
  Location(const Location *src) { X=src->X; Y=src->Y; Z=src->Z; }

  //! Translates this location by src, returns a reference to this
  //! location.
  Location & operator+=(const Location &src) { 
    X+=src.X; Y+=src.Y; Z+=src.Z;
    return(*this);
  }

  //! Returns true if this location is closer to the origin than b.
  bool operator<(const Location &b) const { 
    return(sqlen()<b.sqlen()); 
  }

  //! Returns true if this and b are the same location
  bool operator==(const Location &b) const { 
    return(X==b.X && Y==b.Y && Z==b.Z); 
  }

  //! Returns true if this and b are different locations
  bool operator!=(const Location &b) const { 
    return(X!=b.X || Y!=b.Y || Z!=b.Z); 
  }

  //! Assignment operator
  Location & operator=(const Location &b) { X=b.X; Y=b.Y; Z=b.Z; return(*this); }

  //! Sets (X,Y,Z) to (x,y,z), return a reference to this location.
  Location & set(int x,int y,int z) { X=x; Y=y; Z=z; return(*this); }
  
  //! Prints this location to stdout.
  void print() { cout << "(" << X << "," << Y << "," << Z << ") : " << sqlen() << "\n"; }

  //! Returns the square of the distance from the origin
  int sqlen() const { return( (X*X)+(Y*Y)+(Z*Z) ); }

  //! Returns the square of the distance between this and b
  int sqdist(const Location &b) {
    return( (X-b.X)*(X-b.X) + (Y-b.Y)*(Y-b.Y) + (Z-b.Z)*(Z-b.Z) );
  }
};

//! Abstract discrete voxel adjacency
class Adjacency {
 public:
  //! Returns the number of elements in this adjacency
  int size() { return(data.size()); }

  //! Returns the index-th (0-based) neighbor displacement, as a \ref Location
  Location & operator[](int index) {
    return(data[index]);
  }

  //! Returns the i-th (0-based) neighbor of src, as a \ref Location
  Location neighbor(const Location &src, int i) {
    Location tmp;
    tmp = src;
    tmp += data[i];
    return(tmp);
  }

 protected:
  //! Vector of neighbors
  vector<Location> data;
};

//! Discrete spherical voxel adjacency
class SphericalAdjacency : public Adjacency {

 public:  
  //! Constructor
  SphericalAdjacency();

  //! Constructor, creates adjacency of given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  SphericalAdjacency(float radius,bool self=false) {
    resize(radius,self);
  }

  //! Recreates this adjacency, with given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  void resize(float radius, bool self=false) {
    int dx,dy,dz,r0,r2;

    data.clear();
    r0 = (int) radius;
    r2  = (int)(radius*radius);

    for(dz=-r0;dz<=r0;dz++)
      for(dy=-r0;dy<=r0;dy++)
	for(dx=-r0;dx<=r0;dx++) {
	  Location loc(dx,dy,dz);
	  if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
	    Location *nloc = new Location(loc);
	    data.push_back(*nloc);
	  }
	}
    sort(data.begin(),data.end());
  }

};

//! Discrete circular adjacency on the XY plane
class DiscAdjacency : public Adjacency {

 public:
  //! Constructor
  DiscAdjacency();

  //! Constructor, creates adjacency of given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  DiscAdjacency(float radius,bool self=false) {
    resize(radius,self);
  }

  //! Recreates this adjacency, with given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  void resize(float radius, bool self=false) {
    int dx,dy,r0,r2;

    data.clear();
    r0 = (int) radius;
    r2  = (int)(radius*radius);

    for(dy=-r0;dy<=r0;dy++)
      for(dx=-r0;dx<=r0;dx++) {
	Location loc(dx,dy,0);
	if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
	  Location *nloc = new Location(loc);
	  data.push_back(*nloc);
	}
      }
    sort(data.begin(),data.end());
  }

};

//! A 3D discrete domain
class VolumeDomain {

 public:

  int W,H,D,N,WxH;

  //! Constructor
  VolumeDomain() {
    tby = NULL;
    tbz = NULL;
    W = H = D = WxH = N = 0;
  }
  
  //! Constructor, creates a domain with given width, height and depth
  VolumeDomain(int w,int h,int d) {
    tby = NULL;
    tbz = NULL;
    resize(w,h,d);
  }

  //! Destructor
  virtual ~VolumeDomain() {
    if (tby) delete tby;
    if (tbz) delete tbz;
  }

  //! Resizes this domain
  virtual void resize(int w,int h,int d) {
    int i;

    W = w;
    H = h;
    D = d;
    N = W*H*D;
    WxH = W*H;
    if (tby!=NULL) delete tby;
    if (tbz!=NULL) delete tbz;
    if (N==0) {
      tby = NULL;
      tbz = NULL;
      return;
    }
    tby = new int[H];
    tbz = new int[D];
    for(i=0;i<H;i++) tby[i] = W*i;
    for(i=0;i<D;i++) tbz[i] = WxH*i;
  }

  //! Returns the linear address of voxel (x,y,z)
  int  address(int x,int y,int z) {
    return(x+tby[y]+tbz[z]);
  }

  //! Returns the linear address of \ref Location loc
  int  address(const Location &loc) {
    return(loc.X+tby[loc.Y]+tbz[loc.Z]);
  }

  //! Returns true if voxel (x,y,z) is within this domain
  bool valid(int x,int y,int z) {
    return((x>=0)&&(y>=0)&&(z>=0)&&(x<W)&&(y<H)&&(z<D));
  }

  //! Returns true if voxel (x,y,z) is within this domain
  bool valid(float x,float y,float z) {
    return((x>=0)&&(y>=0)&&(z>=0)&&(x<W)&&(y<H)&&(z<D));
  }

  //! Returns true if \ref Location loc is within this domain
  bool valid(const Location &loc) {
    return(valid(loc.X,loc.Y,loc.Z));
  }
  
  //! Returns the X component of the linear address a
  int  xOf(int a) { return( (a%WxH) % W ); }

  //! Returns the Y component of the linear address a
  int  yOf(int a) { return( (a%WxH) / W ); }

  //! Returns the Z component of the linear address a
  int  zOf(int a) { return( a / WxH ); }

  //! Returns the length of the diagonal segment (0,0,0)-(W,H,D)
  int diagonalLength() { return((int) (sqrt(W*W+H*H+D*D))); }

 private:
  int *tby, *tbz;
};

//! Point in bound discrete 3D space
class BoundLocation : public Location {
 public:
  //! Constructor
  BoundLocation() : Location() { bind(0,0,0); }

  //! Constructor
  BoundLocation(VolumeDomain &vd) : Location() {
    bind(vd);
  }
    
  //! Constructor
  BoundLocation(int w,int h,int d) : Location() {
    bind(w,h,d);
  }

  //! Copy constructor
  BoundLocation(BoundLocation &b) {
    (*this) = b;
  }

  virtual ~BoundLocation() { }

  //! Binds this location to a \ref VolumeDomain
  void bind(VolumeDomain &vd) {
    W = vd.W; H = vd.H; D = vd.D;
    postBind();
  }

  //! Binds this location to a (w,h,d) domain
  void bind(int w,int h,int d) {
    W = w; H = h; D = d;
    postBind();
  }

  //! Advances to the next location within the bound domain
  BoundLocation & operator++(int a) {
    ++X; 
    if (X==W) { X=0; ++Y; }
    if (Y==H) { Y=0; Z++; } 
    if (Z>=D) { X=0; Y=0; Z=D; } 
    return(*this);
  }

  //! Assignment operator
  BoundLocation & operator=(const BoundLocation &b) { 
    X=b.X; Y=b.Y; Z=b.Z; bind(b.W,b.H,b.D); return(*this); }

  //! Assignment operator
  BoundLocation & operator=(const Location &b) { 
    X=b.X; Y=b.Y; Z=b.Z; return(*this); }

  //! Returns a reference to the first location in the bound domain
  Location & begin() { return pb; }

  //! Returns a reference to the location after the last one in the bound domain
  Location & end() { return pe; }

  //! Prints this location to stdout.
  virtual void print() { cout << "(" << X << "," << Y << "," << Z << "), (" << W << "," << H << "," << D << ")" << endl; }


 private:
  int W,H,D;
  Location pb, pe;

  void postBind() {
    pb.set(0,0,0);
    pe.set(0,0,D);
  }

};

class LocationIterator : public Location {
 public:
  LocationIterator(int w,int h,int d) : Location() { W=w; H=h; D=d; }
  LocationIterator(VolumeDomain *vd) : Location() { W=vd->W; H=vd->H; D=vd->D; }
  void begin() { X=Y=Z=0; }

  bool end() { return(Z==D); }

  LocationIterator & operator++(int a) { ++X; if (X==W) { X=0; ++Y; if (Y==H) { Y=0; ++Z; } } return(*this); }

 private:
  int W,H,D;
};

enum RenderType {
  RenderType_NONE,
  RenderType_EQ,
  RenderType_LT,
  RenderType_LE,
  RenderType_GT,
  RenderType_GE
};

class RenderingContext {
public:
  int W,H;
  float *xbuf, *ybuf, *zbuf, *nbuf;
  int   *ibuf;
  char  *rbuf;

  static constexpr float InfZ = 50000.0f;

  RenderingContext(int w,int h) {
    W = w;
    H = h;
    allbuf = NULL;
    allocate();
    prepareFirst();
  }

  RenderingContext(Image &img) {
    W = img.W;
    H = img.H;
    allocate();
    prepareFirst();
  }

  ~RenderingContext() {
    if (allbuf!=NULL) delete allbuf;
    if (rbuf!=NULL)   delete rbuf;
  }

  void prepareFirst() {
    int i,N;
    N = W*H;
    for(i=0;i<N;i++) zbuf[i] = InfZ;
    for(i=0;i<N;i++) nbuf[i] = 0.0;
    for(i=0;i<N;i++) rbuf[i] = 0;
  }

  void prepareNext() {
    int i,N;
    N = W*H;
    for(i=0;i<N;i++) rbuf[i] = 0;
  }

  void clearI() {
    int i,N;
    N = W*H;
    for(i=0;i<N;i++) ibuf[i] = -1;
  }

  void clearN() {
    int i,N;
    N = W*H;
    for(i=0;i<N;i++) nbuf[i] = 0.0;
  }
  
private:
  float *allbuf;

  void allocate() {
    allbuf = new float[4 * W * H];
    zbuf = allbuf;
    nbuf = &allbuf[W*H];
    xbuf = &allbuf[2*W*H];
    ybuf = &allbuf[3*W*H];
    rbuf = new char[W*H];
    ibuf = (int *) nbuf;
  }
  
};

class VoxelQuad {
public:
  int16_t x,y,z;
  unsigned int count;

  VoxelQuad() { x=y=z=0; count=0; }
  void set(int a,int b,int c,int d=0) { x = a; y = b; z = c; count = (unsigned int) d; }
  bool operator<(const VoxelQuad &b) const { return(count>b.count); }
};

template <class T> class Volume : public VolumeDomain {

 public:
  float dx,dy,dz;
  T lastmax;

  typedef BoundLocation iterator;
  
  Volume() : VolumeDomain() {
    data = 0;
    dx = dy = dz = 1.0;
    vqbuf = NULL;
    rendercount = 0;
    lastmax = 0;
  }

  Volume(int w,int h,int d, 
	 float pdx=1.0, float pdy=1.0,float pdz=1.0) :  VolumeDomain(w,h,d) {
    int i;

    vqbuf = NULL;
    rendercount = 0;

    dx = pdx;
    dy = pdy;
    dz = pdz;
    data = new T[N];
    if (!data) return;
    for(i=0;i<N;i++) data[i] = 0;
    lastmax = 0;

    anyi.bind(*this);
    ibegin = anyi.begin(); ibegin.bind(*this);
    iend   = anyi.end();   iend.bind(*this);
  }


  Volume(const char *filename) : VolumeDomain() {
    data = 0;
    vqbuf = NULL;
    rendercount = 0;
    lastmax = 0;
    readAnything(filename);
  }

  void readAnything(const char *filename) {
    FILE *f;
    BZFILE *bf;
    char hdr[512];
    int n,berr;

    f = fopen(filename,"r");
    if (f==NULL) return;
    n = fread(hdr,1,348,f);
    fclose(f);

    if (n >= 4) {
      // plain SCN
      if (strncmp("SCN\n",hdr,4)==0) { readSCN(filename); return; }
      // SCN+bzip2
      if (strncmp("BZh",hdr,3)==0) {
	f = fopen(filename,"r");
	if (f==NULL) return;
	bf = BZ2_bzReadOpen(&berr,f,0,0,NULL,0);
	if (berr==BZ_OK) {
	  n = BZ2_bzRead(&berr,bf,hdr,348);
	}
	BZ2_bzReadClose(&berr,bf);
	fclose(f);
	if (strncmp("SCN\n",hdr,4)==0) { readBzSCN(filename); return; }
      }
    }
    if (n>=348) {
      // plain NIFTI
      if (hdr[0]==0x5c && hdr[1]==0x01 && hdr[2]==0 && hdr[3]==0 &&
	  strncmp("n+1\0",&hdr[344],4)==0) {
	readNIFTI(filename);
	return;
      }
    }
    if (strcasecmp(filename+strlen(filename)-4,".mgz")==0 ||
	strcasecmp(filename+strlen(filename)-4,".mgh")==0 ) {
      readMGZ(filename);
      return;
    }
    // TODO: .nii.gz
    cerr << "** Unrecognized volume format: " << filename << endl;
  }
  
  // copy constructor
  Volume(Volume<T> *src) {
    W = src->W;
    H = src->H;
    D = src->D;
    N = W*H*D;
    dx = src->dx;
    dy = src->dy;
    dz = src->dz;
    resize(W,H,D);
    data = new T[N];
    vqbuf = NULL;
    if (data != NULL)
      for(int i=0;i<N;i++)
	data[i] = src->data[i];
    lastmax = src->lastmax;
  }

  virtual ~Volume() {
    if (data!=NULL)  delete[] data;
    if (vqbuf!=NULL) delete[] vqbuf;
  }

  iterator & begin() { return(ibegin); }
  iterator & end()   { return(iend); }

  void resize(int w,int h,int d) {
    VolumeDomain::resize(w,h,d);
    anyi.bind(*this);
    ibegin = anyi.begin();
    iend   = anyi.end();
    ibegin.bind(*this);
    iend.bind(*this);
  }

  void clear() {
    if (data!=NULL) delete[] data;
    data = NULL;
    resize(0,0,0);
    szPartial = szTotal = szCompressed = 0;
    lastmax = 0;
  }

  int * getSizeTotalPtr() { return(&szTotal); }
  int * getSizePartialPtr() { return(&szPartial); }

  int getSizeTotal() {
    if (data == NULL) return 0;
    return szTotal;
  }

  int getSizePartial() {
    if (data == NULL) return 0;
    return szPartial;
  }

  int getSizeCompressed() {
    if (data == NULL) return 0;
    return szCompressed;
  }

  int writeSCN(const char *filename) {
    switch(sizeof(T)) {
    case 1: return(writeSCN(filename,8,false));
    case 2: return(writeSCN(filename,16,true));
    case 4: return(writeSCN(filename,32,true));
    default: return -1;
    }
  }

  int writeBzSCN(const char * filename, int bits, bool sign, int level) {
    if (bits < 0) {
      int64_t a = (int64_t) minimum();
      int64_t b = (int64_t) maximum();
      if (a>=0 && b<=255) { 
	bits = 8; sign = false;
      } else if (a>=-128 && b<=127) {
	bits = 8; sign = true;
      } else if (a>=0 && b<=65535) {
	bits= 16; sign = false;
      } else if (a>=-32768 && b<=32767) {
	bits = 16; sign = true;
      } else {
	bits = 32;
	sign = (a < 0);
      }
    }

    if (bits!=8 && bits!=16 && bits!=32) {
      cerr << "** writeSCN: invalid bits value\n\n";
      return -1;
    }

    if (level < 1) level = 1;
    if (level > 9) level = 9;

    FILE *f;
    BZFILE *bf;
    int berr, i, j;
    char out[256];

    f = fopen(filename, "w");
    if (f==NULL) return -1;

    bf = BZ2_bzWriteOpen(&berr, f, level, 0, 30);
    if (berr != BZ_OK) goto bzwfail;

    szTotal = N * (bits/8);
    szPartial = 0;
    szCompressed = 0;

    sprintf(out,"SCN\n%d %d %d\n%.4f %.4f %.4f\n%d\n",W,H,D,dx,dy,dz,bits);
    BZ2_bzWrite(&berr, bf, out, strlen(out));
    if (berr != BZ_OK) goto bzwfail;

    if (bits==8 && !sign) {      
      uint8_t *xd = new uint8_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 255 ? 255 : (uint8_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, W);
	szPartial += W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    } else if (bits==8 && sign) {
      int8_t *xd = new int8_t[W];
      int x;
      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -128 ? -128 : (x > 127 ? 127 : (int8_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, W);
	szPartial += W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    } else if (bits==16 && !sign) {
      uint16_t *xd = new uint16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 65535 ? 65535 : (uint16_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, 2*W);
	szPartial += 2*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    } else if (bits==16 && sign) {
      int16_t *xd = new int16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -32768 ? -32768 : (x > 32767 ? 32767 : (int16_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, 2*W);
	szPartial += 2*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    } else if (bits==32 && !sign) {
      uint32_t *xd = new uint32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (uint32_t) x );
	}
	BZ2_bzWrite(&berr, bf, xd, 4*W);
	szPartial += 4*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    } else if (bits==32 && sign) {
      int32_t *xd = new int32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (int32_t) x;
	}
	BZ2_bzWrite(&berr, bf, xd, 4*W);
	szPartial += 4*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete[] xd;
    }

    unsigned int bin, bout;
    BZ2_bzWriteClose(&berr, bf, 0, &bin, &bout);
    fclose(f);
    if (berr != BZ_OK) return -1;

    szTotal = szPartial = (int) bin;
    szCompressed = bout;

    return 0;

  bzwfail:
    BZ2_bzWriteClose(&berr, bf, 1, NULL, NULL);
    fclose(f);
    return -1;
  }

  int writeSCN(const char * filename, int bits, bool sign) {

    if (bits < 0) {
      int64_t a = (int64_t) minimum();
      int64_t b = (int64_t) maximum();
      if (a>=0 && b<=255) { 
	bits = 8; sign = false;
      } else if (a>=-128 && b<=127) {
	bits = 8; sign = true;
      } else if (a>=0 && b<=65535) {
	bits= 16; sign = false;
      } else if (a>=-32768 && b<=32767) {
	bits = 16; sign = true;
      } else {
	bits = 32;
	sign = (a < 0);
      }
    }

    if (bits!=8 && bits!=16 && bits!=32) {
      cerr << "** writeSCN: invalid bits value\n\n";
      return -1;
    }

    ofstream f;
    int i,j;

    szTotal = N * (bits/8);
    szPartial = szCompressed = 0;

    f.open(filename,ofstream::binary);
    if (!f.good()) return -1;
    
    f << "SCN\n" << W << " " << H << " " << D << "\n";
    f << dx << " " << dy << " " << dz << "\n";
    f << bits << "\n";

    if (bits==8 && !sign) {      
      uint8_t *xd = new uint8_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 255 ? 255 : (uint8_t) x ));
	}
	f.write( (char *) xd, 1 * W);
	szPartial += W;
      }
      delete[] xd;
    } else if (bits==8 && sign) {
      int8_t *xd = new int8_t[W];
      int x;
      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -128 ? -128 : (x > 127 ? 127 : (int8_t) x ));
	}
	f.write( (char *) xd, 1 * W);
	szPartial += W;
      }
      delete[] xd;
    } else if (bits==16 && !sign) {
      uint16_t *xd = new uint16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 65535 ? 65535 : (uint16_t) x ));
	}
	f.write( (char *) xd, 2 * W);
	szPartial += 2*W;
      }
      delete[] xd;
    } else if (bits==16 && sign) {
      int16_t *xd = new int16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -32768 ? -32768 : (x > 32767 ? 32767 : (int16_t) x ));
	}
	f.write( (char *) xd, 2 * W);
	szPartial += 2*W;
      }
      delete[] xd;
    } else if (bits==32 && !sign) {
      uint32_t *xd = new uint32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (uint32_t) x );
	}
	f.write( (char *) xd, 4 * W);
	szPartial += 4*W;
      }
      delete[] xd;
    } else if (bits==32 && sign) {
      int32_t *xd = new int32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (int32_t) x;
	}
	f.write( (char *) xd, 4 * W);
	szPartial += 4*W;
      }
      delete[] xd;
    }

    szCompressed = szPartial;

    f.close();
    return 0;
  }

  void readBzSCN(const char *filename) {
    FILE *f;
    BZFILE *bf;
    int i,w=0,h=0,d=0,state,bpp=0;
    float y[7];
    string x;
    char hdr[4],c;
    int berr,bn;

    clear();

    f = fopen(filename,"r");
    if (f==NULL) return;
    bf = BZ2_bzReadOpen(&berr, f, 0, 0, NULL, 0);
    if (berr != BZ_OK) goto bzrfail;

    bn = BZ2_bzRead(&berr,bf,hdr,4);
    if (bn!=4 || berr!=BZ_OK) goto bzrfail;

    if (strncmp("SCN\n",hdr,4)!=0) goto bzrfail;

    x.clear();    
    for(state=0;state!=7;) {
      bn=BZ2_bzRead(&berr,bf,&c,1);
      if (bn!=1 || berr!=BZ_OK) goto bzrfail;
      if (c==' ' || c=='\t' || c=='\n') {
	y[state] = atof(x.c_str());
	x.clear();
	state++;
      } else {
	x += c;
      }
      if (state==7 && c!='\n') { 
	do { bn=BZ2_bzRead(&berr,bf,&c,1); } while(c!='\n' && berr==BZ_OK); 
      }
    }
    if (berr != BZ_OK) goto bzrfail;
    w = (int) y[0];
    h = (int) y[1];
    d = (int) y[2];
    dx = y[3];
    dy = y[4];
    dz = y[5];
    bpp = (int) y[6];

    resize(w,h,d);
    data = new T[N];
    if (data==NULL) goto bzrfail;

    szTotal = N * (bpp/8);
    szPartial = 0;

    switch(bpp) {
    case 8:
      uint8_t *td8;
      td8 = new uint8_t[N];
      if (!td8) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td8[i*W],W);
	szPartial += W;
	if (bn!=W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td8[i]);
      delete td8;
      break;
    case 16:
      uint16_t *td16;
      td16 = new uint16_t[N];
      if (!td16) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td16[i*W],2*W);
	szPartial += 2*W;
	if (bn!=2*W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td16[i]);
      delete td16;
      break;
    case 32:
      int32_t *td32;
      td32 = new int32_t[N];
      if (!td32) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td32[i*W],4*W);
	szPartial += 4*W;
	if (bn!=4*W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td32[i]);
      delete td32;
      break;
    }

    BZ2_bzReadClose(&berr, bf);
    fclose(f);
    return;

  bzrfail:
    //printf("berr = %d\n",berr);
    clear();
    BZ2_bzReadClose(&berr, bf);
    fclose(f);
    return;
  }

  /* 
     yet another retarded file format, this one uses a 
     floating-point field to represent a file offset

     http://nifti.nimh.nih.gov/nifti-1/documentation/nifti1fields
 */
  void readNIFTI(const char *filename) {
    FILE *f;
    char hdr[348];
    int n,nd,i,offset;
    int *iptr;
    int16_t *sptr;
    float *fptr;
    float slope, intercept;
    int16_t dtype;

    clear();
    f = fopen(filename,"r");
    if (f==NULL) return;
    n = fread(hdr,1,348,f);
    if (n!=348) {
      fclose(f);
      return;
    }    

    // format verification
    iptr = (int *) (&hdr[0]);
    if (*iptr != 348) return; // bad header
    if (strncmp(&hdr[344],"n+1\0",4) != 0) return; // bad header
    sptr = (int16_t *) &hdr[40];
    nd = sptr[0];
    if (nd<2 || nd>3) { 
      cerr << "** readNIFTI: only 2- and 3-dimensional images supported. Dimensions in " << filename << "=" << nd << endl;
      fclose(f);
      return;
    }
    W = sptr[1]; H = sptr[2]; D = (nd==3 ? sptr[3] : 1);
    fptr = (float *) &hdr[76];
    dx = fptr[1]; dy = fptr[2]; dz = (nd == 3 ? fptr[3] : 1.0);
    // convert units if needed
    if ( (hdr[123] & 0x07) == 1) { dx/=1000.0; dy/=1000.0; dz/=1000.0; } // meters
    if ( (hdr[123] & 0x07) == 3) { dx*=1000.0; dy*=1000.0; dz*=1000.0; } // microns
    sptr = (int16_t *) &hdr[70];
    dtype = *sptr;
    fptr = (float *) &hdr[108];
    offset = (int) fptr[0];
    slope = fptr[1];
    intercept = fptr[2];
    // bail out on unsupported data types (complex, rgb, 128-bit float)
    if (dtype == 32 || dtype == 128 || dtype == 1536 || dtype == 1792 || dtype == 2048) {
      cerr << "** readNIFTI: file " << filename << " uses unsupported data type " << dtype << endl;
      fclose(f);
      return;
    }
    // sanity check
    if (W<=0 || H <=0 || D<=0) goto nifti_read_fail;
    N = W * H * D;
    //printf("readNIFTI dim=%d %d %d dtype=%d pixdim=%.4f %.4f %.4f slope=%f intercept=%f\n",W,H,D,dtype,dx,dy,dz,slope,intercept);

    resize(W,H,D);
    data = new T[N];
    if (data == NULL) goto nifti_read_fail;
    if (fseek(f,(long) offset, SEEK_SET) != 0) goto nifti_read_fail;
    switch(dtype) {
    case 2: { // uint8
      uint8_t *t8u = new uint8_t[N];
      if (t8u==NULL) goto nifti_read_fail;
      szTotal = N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t8u[i*W]), 1, W, f);
	if (n != W) goto nifti_read_fail;
	szPartial += W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t8u[i] = (uint8_t) (intercept + slope * t8u[i]);
      for(i=0;i<N;i++) data[i] = (T) (t8u[i]);
      delete[] t8u;
    } break;
    case 256: { // int8
      int8_t *t8s = new int8_t[N];
      if (t8s==NULL) goto nifti_read_fail;
      szTotal = N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t8s[i*W]), 1, W, f);
	if (n != W) goto nifti_read_fail;
	szPartial += W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t8s[i] = (int8_t) (intercept + slope * t8s[i]);
      for(i=0;i<N;i++) data[i] = (T) (t8s[i]);
      delete[] t8s;
    } break;
    case 4: { // int16
      int16_t *t16s = new int16_t[N];
      if (t16s==NULL) goto nifti_read_fail;
      szTotal = 2*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t16s[i*W]), 1, 2*W, f);
	if (n != 2*W) goto nifti_read_fail;
	szPartial += 2*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t16s[i] = (int16_t) (intercept + slope * t16s[i]);
      for(i=0;i<N;i++) data[i] = (T) (t16s[i]);
      delete[] t16s;
    } break;
    case 512: { // uint16
      uint16_t *t16u = new uint16_t[N];
      if (t16u==NULL) goto nifti_read_fail;
      szTotal = 2*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t16u[i*W]), 1, 2*W, f);
	if (n != 2*W) goto nifti_read_fail;
	szPartial += 2*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t16u[i] = (uint16_t) (intercept + slope * t16u[i]);
      for(i=0;i<N;i++) data[i] = (T) (t16u[i]);
      delete[] t16u;
    } break;
    case 8: { //int32
      int32_t *t32s = new int32_t[N];
      if (t32s==NULL) goto nifti_read_fail;
      szTotal = 4*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t32s[i*W]), 1, 4*W, f);
	if (n != 4*W) goto nifti_read_fail;
	szPartial += 4*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t32s[i] = (int32_t) (intercept + slope * t32s[i]);
      for(i=0;i<N;i++) data[i] = (T) (t32s[i]);
      delete[] t32s;
    } break;
    case 768: { //uint32
      uint32_t *t32u = new uint32_t[N];
      if (t32u==NULL) goto nifti_read_fail;
      szTotal = 4*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t32u[i*W]), 1, 4*W, f);
	if (n != 4*W) goto nifti_read_fail;
	szPartial += 4*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t32u[i] = (uint32_t) (intercept + slope * t32u[i]);
      for(i=0;i<N;i++) data[i] = (T) (t32u[i]);
      delete[] t32u;
    } break;
    case 1024: { //int64
      int64_t *t64s = new int64_t[N];
      if (t64s==NULL) goto nifti_read_fail;
      szTotal = 8*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t64s[i*W]), 1, 8*W, f);
	if (n != 8*W) goto nifti_read_fail;
	szPartial += 8*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t64s[i] = (int64_t) (intercept + slope * t64s[i]);
      for(i=0;i<N;i++) data[i] = (T) (t64s[i]);
      delete[] t64s;
    } break;
    case 1280: { //uint64
      uint64_t *t64u = new uint64_t[N];
      if (t64u==NULL) goto nifti_read_fail;
      szTotal = 8*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&t64u[i*W]), 1, 8*W, f);
	if (n != 8*W) goto nifti_read_fail;
	szPartial += 8*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) t64u[i] = (uint64_t) (intercept + slope * t64u[i]);
      for(i=0;i<N;i++) data[i] = (T) (t64u[i]);
      delete[] t64u;
    } break;
    case 16: { //float32
      float *f32 = new float[N];
      if (f32==NULL) goto nifti_read_fail;
      szTotal = 4*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&f32[i*W]), 1, 4*W, f);
	if (n != 4*W) goto nifti_read_fail;
	szPartial += 4*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) f32[i] = (float) (intercept + slope * f32[i]);
      for(i=0;i<N;i++) data[i] = (T) (f32[i]);
      delete[] f32;
    } break;
    case 64: { //float64
      double *f64 = new double[N];
      if (f64==NULL) goto nifti_read_fail;
      szTotal = 8*N; szPartial = 0;
      for(i=0;i<H*D;i++) {
	n = fread( (void *) (&f64[i*W]), 1, 8*W, f);
	if (n != 8*W) goto nifti_read_fail;
	szPartial += 8*W;
      }
      if (slope!=1.0f || intercept!=0.0f)
	for(i=0;i<N;i++) f64[i] = (double) (intercept + slope * f64[i]);
      for(i=0;i<N;i++) data[i] = (T) (f64[i]);
      delete[] f64;
    } break;
    }
    niftiFlip();
    fclose(f);
    return;
  nifti_read_fail:
    clear();
    fclose(f);
    return;
  }

  void readMGZ(const char *filename) {
    gzFile g;
    int i,j,k,w,h,d,nr,bpp,stride,iv[7], *fake;
    short int sv;
    float fv[15];

    clear();
    
    g = gzopen(filename, "r");
    if (g==NULL) return;

    gzread(g,&iv[0],7*sizeof(int));
    gzread(g,&sv,1*sizeof(short int));
    gzread(g,&fv[0],15*sizeof(float));

    for(i=0;i<7;i++) iv[i] = be32toh(iv[i]);
    sv = be16toh(sv);
    fake = (int *) (&fv[0]);
    for(i=0;i<15;i++) fake[i] = be32toh(fake[i]);

    w = iv[1];
    h = iv[2];
    d = iv[3];
    dx = fv[0];
    dy = fv[1];
    dz = fv[2];

    if (iv[4] != 1) {
      cerr << "** readMGZ: unsupported vectorial data.\n";
      return;
    }
    switch(iv[5]) {
    case 0: bpp=8; break;
    case 1: bpp=32; break;
    case 3: bpp=-1; break;
    case 4: bpp=16; break;
    default: bpp=-1;
    }

    if (bpp<0) {
      cerr << "** readMGZ: unsupported data type.\n";
      return;
    }

    resize(w,h,d);
    data = new T[N];
    
    stride = W*(bpp/8);
    nr = N / W;
    k = 0;
    uint8_t *buf8;
    int16_t *buf16;
    int32_t *buf32;
    switch(bpp) {
    case 8:
      buf8 = new uint8_t[W];
      if (buf8==NULL) goto mgzfail;
      for(i=0;i<nr;i++) {
	gzread(g,buf8,stride);
	for(j=0;j<W;j++)
	  data[k++] = (T) buf8[j];
      }
      delete buf8;
      break;
    case 16:
      buf16 = new int16_t[W];
      if (buf16==NULL) goto mgzfail;
      for(i=0;i<nr;i++) {
	gzread(g,buf16,stride);
	for(j=0;j<W;j++)
	  data[k++] = (T) buf16[j];
      }
      delete buf16;
      break;
    case 32:
      buf32 = new int32_t[W];
      if (buf32==NULL) goto mgzfail;
      for(i=0;i<nr;i++) {
	gzread(g,buf32,stride);
	for(j=0;j<W;j++)
	  data[k++] = (T) buf32[j];
      }
      delete buf32;
      break;
    }
    gzclose(g);
    return;
  mgzfail:
    clear();
    gzclose(g);
    return;
  }

  void readSCN(const char *filename) {
    FILE *f;
    int i,w=0,h=0,d=0,c,state,bpp=0;
    float y[7];
    string x;
    char hdr[4];

    clear();

    f = fopen(filename,"r");
    if (f==NULL) return;
    if (fread(hdr,1,4,f)!=4) goto rfail;

    if (strncmp("BZh",hdr,3)==0) {
      fclose(f);
      readBzSCN(filename);
      return;
    }

    if (strncmp("SCN\n",hdr,4)!=0) goto rfail;

    x.clear();    
    for(state=0;state!=7;) {
      c = fgetc(f);
      if (ferror(f)) goto rfail;
      if (c==' ' || c=='\t' || c=='\n') {
	y[state] = atof(x.c_str());
	x.clear();
	state++;
      } else {
	x += c;
      }
      if (state==7 && c!='\n') { do { c=fgetc(f); } while(c!='\n' && !ferror(f)); }
    }
    if (ferror(f)) goto rfail;
    w = (int) y[0];
    h = (int) y[1];
    d = (int) y[2];
    dx = y[3];
    dy = y[4];
    dz = y[5];
    bpp = (int) y[6];

    resize(w,h,d);
    data = new T[N];
    if (data==NULL) goto rfail;

    szTotal = N * (bpp/8);
    szPartial = 0;

    switch(bpp) {
    case 8:
      uint8_t *td8;
      td8 = new uint8_t[N];
      if (!td8) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td8[i*W]), 1, W, f);
	szPartial += W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td8[i]);
      delete td8;
      break;
    case 16:
      uint16_t *td16;
      td16 = new uint16_t[N];
      if (!td16) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td16[i*W]), 1, 2*W, f);
	szPartial += 2*W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td16[i]);
      delete td16;
      break;
    case 32:
      int32_t *td32;
      td32 = new int32_t[N];
      if (!td32) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td32[i*W]), 1, 4*W, f);
	szPartial += 4*W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td32[i]);
      delete td32;
      break;
    }
    fclose(f);
    return;

  rfail:
    clear();
    fclose(f);
    return;
  }

  bool ok() { return(data!=0); }

  T & voxel(int x,int y,int z) { 
    #ifdef SAFE
    assert(address(x,y,z)>=0 && address(x,y,z)<N);
    #endif
    return(data[address(x,y,z)]); 
  }
  T & voxel(float x,float y,float z) { 
    #ifdef SAFE
    return(voxel((int)x,(int)y,(int)z));
    #else
    return(data[address((int)x,(int)y,(int)z)]);
    #endif
  }
  T & voxel(int a) { 
    #ifdef SAFE
    assert(a>=0 && a<N);
    #endif
    return(data[a]);
  }
  T & voxel(Location & loc) { 
    #ifdef SAFE
    assert(address(loc)>=0 && address(loc)<N);
    #endif
    return(data[address(loc)]);
  }

  T interpolatedVoxel(double x, double y, double z) {
    int i,j,k,xx[2],yy[2],zz[2];
    double d,dsum,vsum,maxd;
    xx[0] = (int) floor(x); xx[1] = (int) ceil(x);
    yy[0] = (int) floor(y); yy[1] = (int) ceil(y);
    zz[0] = (int) floor(z); zz[1] = (int) ceil(z);

    if (xx[0] < 0 || xx[0] >= W || yy[0] < 0 || yy[0] >= H || zz[0] < 0 || zz[0] >= D) return 0;
    if (xx[1] < 0 || xx[1] >= W) xx[1] = xx[0];
    if (yy[1] < 0 || yy[1] >= H) yy[1] = yy[0];
    if (zz[1] < 0 || zz[1] >= D) zz[1] = zz[0];
    
    dsum = 0.0;
    vsum = 0.0;
    maxd = sqrt(3.0);
    for(i=0;i<2;i++)
      for(j=0;j<2;j++)
	for(k=0;k<2;k++) {
	  d = maxd - sqrt( (xx[i]-x)*(xx[i]-x) + (yy[j]-y)*(yy[j]-y) + (zz[k]-z)*(zz[k]-z) );
	  vsum += d * (double) voxel(xx[i],yy[j],zz[k]);
	  dsum += d;
	}
    return((T)(vsum/dsum));
  }

  T maximum(bool prevok=false) {
    if (prevok) return lastmax;
    T x = data[0];
    for(int i=0;i<N;i++) if (data[i] > x) x = data[i];
    lastmax = x;
    return x;
  }

  T maximum(double perc) {
    int i, pn = (int) (perc * N);
    T rmv;
    int *histo;
    rmv = maximum();
    histo = new int[rmv+1];
    for(i=0;i<=rmv;i++) histo[i] = 0;
    for(i=0;i<N;i++) histo[voxel(i)]++;
    for(i=1;i<=rmv;i++) histo[i] += histo[i-1];
    for(i=rmv;histo[i] > pn;i--) ;
    delete histo;
    return( (T) i );
  }

  T minimum() {
    T x = data[0];
    for(int i=0;i<N;i++) if (data[i] < x) x = data[i];
    return x;
  }

  double mean() {
    double sum=0.0, n;
    n = (double) N;
    for(int i=0;i<N;i++)
      sum += (data[i] / n);
    return sum;
  }

  double stdev() {
    return(stdev(mean()));
  }

  double stdev(double mean) {
    double sum=0.0, n;
    n = (double) N;
    for(int i=0;i<N;i++)
      sum += ( (data[i]-mean)*(data[i]-mean) ) / n;
    return(sqrt(sum));    
  }

  void gradient(int x,int y,int z,R3 &g) {
    int p[3],q[3];

    p[0] = x-1 < 0 ? x : x-1;
    p[1] = y-1 < 0 ? y : y-1;
    p[2] = z-1 < 0 ? z : z-1;

    q[0] = x+1 >= W ? x : x+1;
    q[1] = y+1 >= H ? y : y+1;
    q[2] = z+1 >= D ? z : z+1;

    g.X = (float) (voxel(q[0],y,z) - voxel(p[0],y,z));
    g.Y = (float) (voxel(x,q[1],z) - voxel(x,p[1],z));
    g.Z = (float) (voxel(x,y,q[2]) - voxel(x,y,p[2]));
  }

  void fill(T val) { for(int i=0;i<N;i++) data[i] = val; }

  /* algorithms */

  int max(const int a,const int b) { return((a>b)?a:b); }
  int min(const int a,const int b) { return((a<b)?a:b); }

  void normalize(int maxval) {

    int i;
    T cmax, cmin, camp, v;

    cmax = maximum();
    cmin = minimum();
    camp = cmax-cmin;

    for(i=0;i<N;i++) {
      v = voxel(i);
      v = ((v-cmin)*maxval)/camp;
      voxel(i) = v;
    }

  }

  void histogramMatch(Volume<T> *src) {
    int *h1, *h2, i, j, sj, mh;
    float *f1, *f2, p;
    
    mh = min(4096, (int) (maximum() + 1));
    mh = min(4096, (int) ((src->maximum())+1));

    if (mh < 1) return;

    h1 = new int[mh];
    h2 = new int[mh];
    f1 = new float[mh];
    f2 = new float[mh];

    for(i=0;i<mh;i++) h1[i] = h2[i] = 0;
    
    // compute histograms
    for(i=0;i<N;i++)
      if (voxel(i) >= 0 && voxel(i) < mh)
	h1[(int) voxel(i)]++;
    
    for(i=0;i<src->N;i++)
      if (src->voxel(i) >= 0 && src->voxel(i) < mh)
	h2[(int) (src->voxel(i))]++;
    
    // accumulate
    for(i=1;i<mh;i++) {
      h1[i] += h1[i-1];
      h2[i] += h2[i-1];
    }
    
    // normalize
    for(i=0;i<mh;i++) {
      f1[i] = ((float) h1[i]) / N;
      f2[i] = ((float) h2[i]) / src->N;
    }
    
    // build lut
    sj = 0;
    for(i=0;i<mh;i++) {
      p = f1[i];
      for(j=sj;j<mh-1;j++) if (f2[j] > p) break;
      h1[i] = sj = j;
    }

    // apply lut to this volume
    for(i=0;i<N;i++)
      if (voxel(i) >= 0 && voxel(i) < mh) 
	voxel(i) = (T) h1[(int) voxel(i)];

    delete h1;
    delete h2;
    delete f1;
    delete f2;
  }

  Volume<char> *binaryThreshold(T val) {
    int i;
    Volume<char> *r;
    r = new Volume<char>(W,H,D,dx,dy,dz);
    r->fill(0);
    for(i=0;i<N;i++)
      if (voxel(i) >= val) r->voxel(i) = 1;
    return r;
  }

  void incrementBorder(float radius=1.5) {
    SphericalAdjacency A(radius);
    iterator p;
    Location q;
    int i;

    for(p=begin();p!=end();p++) {
      if (voxel(p)!=0)
	for(i=0;i<A.size();i++) {
	  q = A.neighbor(p,i);
	  if (valid(q))
	    if (voxel(q)==0) {
	      voxel(p)++;
	      break;
	    }
	}
    }
  }

  void clamp(T minval, T maxval) {
    int i;
    for(i=0;i<N;i++)
      if (data[i] < minval) data[i] = minval;
      else if (data[i] > maxval) data[i] = maxval;
  }
  
  // input: 0=do not expand, 1=expand, 2=seed
  Volume<int> * edt() {
    Volume<int> *edt, *root;
    
    Location q,r,rr;
    iterator p;
    SphericalAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      switch(voxel(ap)) {
      case 0: edt->voxel(ap) = -1; break;
      case 1: edt->voxel(ap) = 2*(W*W+H*H+D*D); break;
      case 2: edt->voxel(ap) = 0; root->voxel(ap) = ap; Q->insert(ap,0); break;
      }
    }
 
    //cout << "initial state of 1,H-1,1 (this) = " << (int) (voxel(1,H-1,1)) << endl;
    //cout << "initial state of 1,H-1,1 (edt)  = " << edt->voxel(1,H-1,1) << endl;
   
    //cout << 2*(W*W+H*H+D*D);
    
    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
	r = A.neighbor(q,i);
	if (valid(r)) {
	  c2 = r.sqdist(rr);
	  c1 = edt->voxel(r);
	  if (c2 < c1) {
	    ar = address(r);
	    edt->voxel(ar) = c2;
	    root->voxel(ar) = j;
	    if (Q->colorOf(ar) == PriorityQueue::Gray)
	      Q->update(ar,c1,c2);
	    else
	      Q->insert(ar,c2);
	  }
	}
      }
    }
    delete Q;
    delete root;

    //cout << "final state of 1,H-1,1 (edt) = " << edt->voxel(1,H-1,1) << endl;

    return edt;
  }

  // input: 0=do not expand, 1=expand, 2=seed
  void edt(Volume<int> * &edmap, Volume<int> * &rootmap) {
    Volume<int> *edt, *root;
    
    Location q,r,rr;
    iterator p;
    SphericalAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      switch(voxel(ap)) {
      case 0: edt->voxel(ap) = -1; break;
      case 1: edt->voxel(ap) = 2*(W*W+H*H+D*D); break;
      case 2: edt->voxel(ap) = 0; root->voxel(ap) = ap; Q->insert(ap,0); break;
      }
    }
 
    //cout << "initial state of 1,H-1,1 (this) = " << (int) (voxel(1,H-1,1)) << endl;
    //cout << "initial state of 1,H-1,1 (edt)  = " << edt->voxel(1,H-1,1) << endl;
   
    //cout << 2*(W*W+H*H+D*D);
    
    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
	r = A.neighbor(q,i);
	if (valid(r)) {
	  c2 = r.sqdist(rr);
	  c1 = edt->voxel(r);
	  if (c2 < c1) {
	    ar = address(r);
	    edt->voxel(ar) = c2;
	    root->voxel(ar) = j;
	    if (Q->colorOf(ar) == PriorityQueue::Gray)
	      Q->update(ar,c1,c2);
	    else
	      Q->insert(ar,c2);
	  }
	}
      }
    }
    delete Q;

    //cout << "final state of 1,H-1,1 (edt) = " << edt->voxel(1,H-1,1) << endl;
    edmap   = edt;
    rootmap = root;
  }

  Volume<char> * treePruning(Volume<char> *seeds) {
    BasicPQ *Q;
    Volume<int> *cost, *pred, *dcount;
    Volume<char> *seg, *B;
    SphericalAdjacency A6(1.0);
    int i,j,k,maxval,inf,p,c1,c2,dmax,pmax=0,gmax;
    Location q,r;
    queue<int> fifo;

    /* compute IFT */
    cost = new Volume<int>(W,H,D);
    pred = new Volume<int>(W,H,D);

    pred->fill(-1);

    maxval = (int) maximum();
    inf = maxval + 1;

    Q = new BasicPQ(N,maxval+1);

    for(i=0;i<N;i++) {
      if (seeds->voxel(i) != 0) {
	cost->voxel(i) = 0;
	Q->insert(i,0);
      } else {
	cost->voxel(i) = inf;
      }
    }

    while(!Q->empty()) {
      p = Q->remove();
      q.set( xOf(p), yOf(p), zOf(p) );
      for(i=0;i<A6.size();i++) {
	r = A6.neighbor(q,i);
	if (valid(r)) {
	  c1 = cost->voxel(r);
	  c2 = max( cost->voxel(q), this->voxel(r) );	  
	  if (c2 < c1) {
	    cost->voxel(r) = c2;
	    pred->voxel(r) = p;
	    Q->insert(address(r), c2);
	  }
	}
      }
    }

    delete cost;

     /* compute descendant count (old method) */
    dcount = new Volume<int>(W,H,D);
    B = this->border();
    for(i=0;i<N;i++) {
      if (B->voxel(i) != 0) {
	j = i;
	while(pred->voxel(j) != -1) {
	  dcount->voxel(pred->voxel(j))++;
	  j = pred->voxel(j);
	}
      }
    }
    
    /* find leaking points */
    for(i=0;i<N;i++) {
      if (B->voxel(i) == 1) {
	j = i;
	dmax = 0;
	while(pred->voxel(j) != -1) {
	  if (dcount->voxel(j) > dmax) {
	    dmax = dcount->voxel(j);
	    pmax = j;
	  }
	  j = pred->voxel(j);
	}
	k = pmax;
	gmax = this->voxel(k);
	while(pred->voxel(k) != -1) {
	  if ( (dcount->voxel(k) == dmax) && (gmax < this->voxel(k)) ) {
	    pmax = k;
	    gmax = this->voxel(k);
	  }
	  k = pred->voxel(k);
	}

	/*
	if (B->voxel(pmax) != 2)
	  cout << "prune at " << pmax << endl;
	*/
	B->voxel(pmax) = 2; // set leaking point
      }
    }

    /* prune trees */
    seg = new Volume<char>(W,H,D);

    for(i=0;i<N;i++) {
      if (pred->voxel(i) == -1) {
	seg->voxel(i) = 1;
	fifo.push(i);

	while(!fifo.empty()) {
	  j = fifo.front();
	  fifo.pop();
	  q.set( xOf(j), yOf(j), zOf(j) );
	  seg->voxel(j) = 1;

	  for(k=0;k<A6.size();k++) {
	    r = A6.neighbor(q,k);
	    if (valid(r))
	      if ( (pred->voxel(r) == j) && (B->voxel(r) != 2) )
		fifo.push(address(r));
	  }
	}
      }
    }

    delete pred;
    delete dcount;
    delete B;
    return seg;
  }

  Volume<char> * border() {
    Volume<char> *B;
    int i,j;
    B = new Volume<char>(W,H,D);
    if (D>=3)
      for(i=0;i<W;i++)
	for(j=0;j<H;j++)
	  B->voxel(i,j,0) = B->voxel(i,j,D-1) = 1;
    if (H>=3)
      for(i=0;i<W;i++)
	for(j=0;j<D;j++)
	  B->voxel(i,0,j) = B->voxel(i,H-1,j) = 1;
    if (W>=3)
      for(i=0;i<H;i++)
	for(j=0;j<D;j++)
	  B->voxel(0,i,j) = B->voxel(W-1,i,j) = 1;
    return B;
  }

  Volume<T> * erode(Adjacency &A) {
    int j;
    T val, upper, lower;
    Volume<T> * dest;
    iterator p;
    Location q;

    dest = new Volume<T>(W,H,D);
    upper = maximum();
    lower = minimum();

    for(p=begin();p!=end();p++) {
      val = upper;
      for(j=0;j<A.size();j++) {
	q = A.neighbor(p,j);
	if (valid(q)) if (voxel(q) < val) val = voxel(q);
	if (val == lower) break;
      }
      dest->voxel(p) = val;
    }
    return dest;
  }

  Volume<T> * dilate(Adjacency &A) {
    int j;
    T val, upper, lower;
    Volume<T> * dest;
    iterator p;
    Location q;

    dest = new Volume<T>(W,H,D);
    upper = maximum();
    lower = minimum();

    for(p=begin();p!=end();p++) {
      val = lower;
      for(j=0;j<A.size();j++) {
	q = A.neighbor(p,j);
	if (valid(q)) if (voxel(q) > val) val = voxel(q);
	if (val == upper) break;
      }
      dest->voxel(p) = val;
    }
    return dest;
  }

  Volume<T> * binaryErode(float radius) {
    Volume<int> *edt, *root;
    Volume<T> *dest;
    int rlimit;

    Location q,r,rr;
    iterator p;
    SphericalAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    rlimit = 1 + (int) (radius * radius);

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      if (voxel(ap) == 0) {
	edt->voxel(ap) = 0;
	root->voxel(ap) = ap;
	Q->insert(ap, 0);
      } else {
	edt->voxel(ap) = rlimit;
      }
    }

    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
        r = A.neighbor(q,i);
        if (valid(r)) {
          c2 = r.sqdist(rr);
          c1 = edt->voxel(r);
          if (c2 < c1) {
            ar = address(r);
            edt->voxel(ar) = c2;
            root->voxel(ar) = j;
            if (Q->colorOf(ar) == PriorityQueue::Gray)
              Q->update(ar,c1,c2);
            else
              Q->insert(ar,c2);
          }
        }
      }
    }
    delete Q;
    delete root;

    dest = new Volume<T>(W,H,D);
    for(i=0;i<edt->N;i++)
      dest->voxel(i) = edt->voxel(i) < rlimit ? 0 : 1;
    delete edt;
    return dest;
  }

  Volume<T> * binaryErode2D(float radius) {
    Volume<int> *edt, *root;
    Volume<T> *dest;
    int rlimit;

    Location q,r,rr;
    iterator p;
    DiscAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    rlimit = 1 + (int) (radius * radius);

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      if (voxel(ap) == 0) {
	edt->voxel(ap) = 0;
	root->voxel(ap) = ap;
	Q->insert(ap, 0);
      } else {
	edt->voxel(ap) = rlimit;
      }
    }

    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
        r = A.neighbor(q,i);
        if (valid(r)) {
          c2 = r.sqdist(rr);
          c1 = edt->voxel(r);
          if (c2 < c1) {
            ar = address(r);
            edt->voxel(ar) = c2;
            root->voxel(ar) = j;
            if (Q->colorOf(ar) == PriorityQueue::Gray)
              Q->update(ar,c1,c2);
            else
              Q->insert(ar,c2);
          }
        }
      }
    }
    delete Q;
    delete root;

    dest = new Volume<T>(W,H,D);
    for(i=0;i<edt->N;i++)
      dest->voxel(i) = edt->voxel(i) < rlimit ? 0 : 1;
    delete edt;
    return dest;
  }

  Volume<T> * binaryDilate(float radius) {
    Volume<int> *edt, *root;
    Volume<T> *dest;
    int rlimit;

    Location q,r,rr;
    iterator p;
    SphericalAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    rlimit = 1 + (int) (radius * radius);

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      if (voxel(ap) == 1) {
	edt->voxel(ap) = 0;
	root->voxel(ap) = ap;
	Q->insert(ap, 0);
      } else {
	edt->voxel(ap) = rlimit;
      }
    }

    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
        r = A.neighbor(q,i);
        if (valid(r)) {
          c2 = r.sqdist(rr);
          c1 = edt->voxel(r);
          if (c2 < c1) {
            ar = address(r);
            edt->voxel(ar) = c2;
            root->voxel(ar) = j;
            if (Q->colorOf(ar) == PriorityQueue::Gray)
              Q->update(ar,c1,c2);
            else
              Q->insert(ar,c2);
          }
        }
      }
    }
    delete Q;
    delete root;

    dest = new Volume<T>(W,H,D);
    for(i=0;i<edt->N;i++)
      dest->voxel(i) = edt->voxel(i) < rlimit ? 1 : 0;
    delete edt;
    return dest;
  }

  Volume<T> * binaryDilate2D(float radius) {
    Volume<int> *edt, *root;
    Volume<T> *dest;
    int rlimit;

    Location q,r,rr;
    iterator p;
    DiscAdjacency A(1.5);
    BasicPQ *Q;
    int ap,ar,c1,c2,n,i,j;

    rlimit = 1 + (int) (radius * radius);

    edt = new Volume<int>(W,H,D);
    root = new Volume<int>(W,H,D);
    Q = new BasicPQ(N,W*W+H*H+D*D+1);

    for(p=begin();p!=end();p++) {
      ap = address(p);
      if (voxel(ap) == 1) {
	edt->voxel(ap) = 0;
	root->voxel(ap) = ap;
	Q->insert(ap, 0);
      } else {
	edt->voxel(ap) = rlimit;
      }
    }

    while(!Q->empty()) {
      n = Q->remove();
      q.set( xOf(n), yOf(n), zOf(n) );
      j = root->voxel(n);
      rr.set( xOf(j), yOf(j), zOf(j) );
      for(i=0;i<A.size();i++) {
        r = A.neighbor(q,i);
        if (valid(r)) {
          c2 = r.sqdist(rr);
          c1 = edt->voxel(r);
          if (c2 < c1) {
            ar = address(r);
            edt->voxel(ar) = c2;
            root->voxel(ar) = j;
            if (Q->colorOf(ar) == PriorityQueue::Gray)
              Q->update(ar,c1,c2);
            else
              Q->insert(ar,c2);
          }
        }
      }
    }
    delete Q;
    delete root;

    dest = new Volume<T>(W,H,D);
    for(i=0;i<edt->N;i++)
      dest->voxel(i) = edt->voxel(i) < rlimit ? 1 : 0;
    delete edt;
    return dest;
  }

  Volume<T> * binaryClose(float radius, bool op2d=false) {
    Volume<T> *tb,*x,*y;
    int border,i,j,k;

    border = (int) ceil(radius);
    tb = new Volume<T>(W+2*border,H+2*border,D+2*border);
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  tb->voxel(i+border,j+border,k+border) = voxel(i,j,k);

    x = op2d ? tb->binaryDilate2D(radius) : tb->binaryDilate(radius);
    y = op2d ? x->binaryErode2D(radius)   : x->binaryErode(radius);
    delete tb;

    tb = new Volume<T>(W,H,D);
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  tb->voxel(i,j,k) = y->voxel(i+border,j+border,k+border);
    
    delete x;
    delete y;
    return tb;
  }

  Volume<T> * binaryOpen(float radius, bool op2d=false) {
    Volume<T> *tb,*x,*y;
    int border,i,j,k;

    border = (int) ceil(radius);
    tb = new Volume<T>(W+2*border,H+2*border,D+2*border);
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  tb->voxel(i+border,j+border,k+border) = voxel(i,j,k);

    x = op2d ? tb->binaryErode2D(radius) : tb->binaryErode(radius);
    y = op2d ? x->binaryDilate2D(radius) : x->binaryDilate(radius);
    delete tb;

    tb = new Volume<T>(W,H,D);
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++)
	  tb->voxel(i,j,k) = y->voxel(i+border,j+border,k+border);
    
    delete x;
    delete y;
    return tb;
  }

  Volume<char> * brainMarkerComp() {
    int i,a=0,b,c=0,x;
    Volume<char> *dest,*ero;
    Volume<int> *tmp;
    SphericalAdjacency R5(5.0,true), R1(1.0,false);

    b = computeOtsu();
    meansAboveBelowT(b,c,a);
    tmp = applySShape(a,b,c);
    dest = new Volume<char>(W,H,D);
    for(i=0;i<N;i++) {
      x = tmp->voxel(i);
      if (x >= b && x < 4096)
	dest->voxel(i) = 1;
    }
    delete tmp;

    ero = dest->erode(R5);
    delete dest;
    ero->selectLargestComp(R1);
    return ero;
  }

  void selectLargestComp(Adjacency &A) {
    Volume<int> * label;
    queue<int> Q;
    int i,j,k,L=1;
    Location p,q;
    int bestarea=0,bestcomp=0,area=0;

    label = new Volume<int>(W,H,D);
       
    for(i=0;i<N;i++) {
      if (this->voxel(i)==1 && label->voxel(i)==0) {
	area = 1;
	label->voxel(i) = L;
	Q.push(i);
	while(!Q.empty()) {
	  j = Q.front();
	  Q.pop();

	  p.set( xOf(j), yOf(j), zOf(j) );
	  for(k=0;k<A.size();k++) {
	    q = A.neighbor(p,k);
	    if (valid(q)) {
	      if (this->voxel(q)==1 && label->voxel(q)==0) {
		label->voxel(q) = label->voxel(p);
		++area;
		Q.push(address(q));
	      }
	    }
	  }
	}
      }
      if (area > bestarea) {
	bestarea = area;
	bestcomp = L;
      }
      L++;
    }

    for(i=0;i<N;i++)
      if (label->voxel(i) != bestcomp)
	this->voxel(i) = 0;

    delete label;
  }

  Volume<int> * labelBinComp(Adjacency &A) {
    Volume<int> * label;
    queue<int> Q;
    int i,j,k,L=1;
    Location p,q;

    label = new Volume<int>(W,H,D);
    
    for(i=0;i<N;i++) {
      if (voxel(i)==1 && label->voxel(i)==0) {
	label->voxel(i) = L;
	Q.push(i);
	while(!Q.empty()) {
	  j = Q.front();
	  Q.pop();

	  p.set( xOf(j), yOf(j), zOf(j) );
	  for(k=0;k<A.size();k++) {
	    q = A.neighbor(p,k);
	    if (valid(q)) {
	      if (voxel(q)==1 && label->voxel(q)==0) {
		label->voxel(q) = label->voxel(p);
		Q.push(address(q));
	      }
	    }
	  }
	}
      }
      L++;
    }
    return(label);
  }

  Volume<int> * otsuEnhance() {
    int a=0,b,c=0;
    b = computeOtsu();
    meansAboveBelowT(b,c,a);
    //cout << "a=" << a << "  b=" << b << "  c=" << c << endl;
    return(applySShape(a,b,c));
  }

  Volume<int> * otsuEnhance(double cf) {
    int a=0,b,c=0;
    b = computeOtsu();
    b = (int) (b * cf);
    meansAboveBelowT(b,c,a);
    //cout << "a=" << a << "  b=" << b << "  c=" << c << endl;
    return(applySShape(a,b,c));
  }

  int computeOtsu() {
    double *hist;
    int i,t,hn,imax,topt=0;
    double p1,p2,m1,m2,s1,s2,j,jmax=-1.0;

    imax = (int) maximum();
    hn = imax + 1;
    hist = new double[hn];
    for(i=0;i<hn;i++) hist[i] = 0;
    for(i=0;i<N;i++) ++hist[data[i]];
    for(i=0;i<hn;i++) hist[i] /= ((double)N);

    for(t=1;t<imax;t++) {
      for(i=0,p1=0.0;i<=t;i++) p1 += hist[i];
      p2 = 1.0 - p1;
      if ((p1>0.0)&&(p2>0.0)) {
	for(i=0,m1=0.0;i<=t;i++) m1 += hist[i] * i;
	m1 /= p1;
	for(i=t+1,m2=0.0;i<=imax;i++) m2 += hist[i] * i;
	m2 /= p2;
	for(i=0,s1=0.0;i<=t;i++) s1 += hist[i] * (i-m1) * (i-m1);
	s1 /= p1;
	for(i=t+1,s2=0.0;i<=imax;i++) s2 += hist[i] * (i-m2) * (i-m2);
	s2 /= p2;
	j = (p1*p2*(m1-m2)*(m1-m2))/(p1*s1+p2*s2);
      } else {
	j = 0.0;
      }
      if (j > jmax) { jmax = j; topt = t; }
    }
    delete hist;
    return topt;
  }

  int computeOtsu(Volume<char> *mask) {
    double *hist;
    int i,t,hn,imax,topt=0, RN;
    double p1,p2,m1,m2,s1,s2,j,jmax=-1.0;

    imax = (int) maximum();
    hn = imax + 1;
    hist = new double[hn];
    for(i=0;i<hn;i++) hist[i] = 0;
    RN = 0;
    for(i=0;i<N;i++) if (mask->voxel(i)!=0) { ++hist[data[i]]; ++RN; }
    for(i=0;i<hn;i++) hist[i] /= ((double)RN);

    for(t=1;t<imax;t++) {
      for(i=0,p1=0.0;i<=t;i++) p1 += hist[i];
      p2 = 1.0 - p1;
      if ((p1>0.0)&&(p2>0.0)) {
	for(i=0,m1=0.0;i<=t;i++) m1 += hist[i] * i;
	m1 /= p1;
	for(i=t+1,m2=0.0;i<=imax;i++) m2 += hist[i] * i;
	m2 /= p2;
	for(i=0,s1=0.0;i<=t;i++) s1 += hist[i] * (i-m1) * (i-m1);
	s1 /= p1;
	for(i=t+1,s2=0.0;i<=imax;i++) s2 += hist[i] * (i-m2) * (i-m2);
	s2 /= p2;
	j = (p1*p2*(m1-m2)*(m1-m2))/(p1*s1+p2*s2);
      } else {
	j = 0.0;
      }
      if (j > jmax) { jmax = j; topt = t; }
    }
    delete hist;
    return topt;
  }

  void meansAboveBelowT(int t, int &t1, int &t2) {
    int64_t m1=0, m2=0, nv1=0, nv2=0;
    int p,delta;
    for(p=0;p<N;p++) {
      if (data[p]<t) {
	m1 += data[p];
	++nv1;
      } else if (data[p]>t) {
	m2 += data[p];
	++nv2;
      }
    }
    delta = (int) ((m2/nv2 - m1/nv1)/2);
    t1 = t+delta;
    t2 = t-delta;
  }

  Volume<int> * applySShape(int a,int b,int c) {
    Volume<int> * dest;
    float x;
    int p;
    float weight,sqca;

    dest = new Volume<int>(W,H,D);
    sqca = (float) ((c-a)*(c-a));

    for(p=0;p<N;p++) {
      x = voxel(p);
      if (x<=a) {
	weight = 0.0;
      } else {
	if (x>a && x<=b) {
	  weight = (2.0* (x-a) * (x-a)) / sqca;
	} else if (x>b && x<=c) {
	  weight = (1.0 - 2.0*(x-c)*(x-c)/sqca );
	} else {
	  weight = 1.0;
	}
      }
      dest->voxel(p) = (int) (weight * x);
    }
    return dest;
  }

  // Bergo et al, 2007
  Volume<char> * segmentBrainJMIV() {
    Volume<int> *e,*g,*iso;
    Volume<char> *m, *s;
    iso = isometricInterpolation();
    m = iso->brainMarkerComp();
    e = iso->otsuEnhance();
    delete iso;
    g = e->featureGradient();
    s = g->treePruning(m);
    delete e;
    delete m;
    delete g;
    return s;
  }

  Volume<T> * featureGradient() {
    Volume<T> * dest;
    int i,ap;
    float *f,*mg,dist,gx,gy,gz,v,imax;
    SphericalAdjacency A6(1.0,false), A7(1.0,true);
    Location p, q;

    imax = (float) maximum();
    f = new float[N * 7];
    if (!f) return 0;
    mg = new float[6];

    dest = new Volume<T>(W,H,D);
    
    for(p.Z=0;p.Z<D;p.Z++)
      for(p.Y=0;p.Y<H;p.Y++)
	for(p.X=0;p.X<W;p.X++) {
	  ap = address(p);
	  for(i=0;i<A7.size();i++) {
	    q = A7.neighbor(p,i);
	    if (valid(q)) {
	      f[(7*ap)+i] = voxel(q) / imax ;
	    } else
	      f[(7*ap)+i] = 0.0;
	  }
	}

    p.set(0,0,0);
    for(i=0;i<6;i++)
      mg[i] = sqrt(A6.neighbor(p,i).sqlen());

    for(p.Z=0;p.Z<D;p.Z++) {
      for(p.Y=0;p.Y<H;p.Y++) {
	for(p.X=0;p.X<W;p.X++) {
	  ap = address(p);
	  gx = gy = gz = 0.0;
	  for(i=0;i<A6.size();i++) {
	    q = A6.neighbor(p,i);
	    if (valid(q)) {
	      dist = featureDistance(&f[7*ap],&f[7*address(q)],7);
	      gx += (dist * A6[i].X) / mg[i];
	      gy += (dist * A6[i].Y) / mg[i];
	      gz += (dist * A6[i].Z) / mg[i];
	    }
	  }
	  v = 100000.0 * sqrt(gx*gx + gy*gy + gz*gz);
	  dest->voxel(ap) = (T) v;
	}
      }
    }

    delete f;
    delete mg;

    return dest;
  }

  float featureDistance(float *a,float *b,int n) {
      int i;
      float dist=0.0;
      for (i=0;i<n;i++)
	dist += (b[i]-a[i])/2.0;
      dist /= n;
      return(exp(-(dist-0.5)*(dist-0.5)/2.0));
  }

  Volume<T> * isometricInterpolation() {
    float md;

    if (dx==dy && dy==dz) {
      Volume<T> *v;
      int i;
      v = new Volume<T>(W,H,D);
      v->dx = dx;
      v->dy = dy;
      v->dz = dz;
      for(i=0;i<N;i++)
	v->voxel(i) = this->voxel(i);
      return v;
    }

    md = (dx < dy) ? dx : dy;
    md = (dz < md) ? dz : md;
    return(interpolate(md,md,md));
  }

  Volume<T> * interpolate(float ndx, float ndy, float ndz) {
     Volume<T> *dest, *tmp;
     int value;
     Location Q,P,R;
     float walked_dist,dist_PQ;

     if (ndx==dx && ndy==dy && ndz==dz) {
       return(new Volume<T>(this));
     }

     dest = this;

     if (ndx != dx) {
       tmp = new Volume<T>( (int)((float)(dest->W-1)*dest->dx/ndx)+1,
			    dest->H,
			    dest->D);
       for(Q.X=0; Q.X < tmp->W; Q.X++) {
	 for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	   for(Q.Y=0; Q.Y < tmp->H; Q.Y++) {
	     
	     walked_dist = (float)Q.X * ndx;
	     P.X = (int)(walked_dist/dx);
	     P.Y = Q.Y; P.Z = Q.Z;
	     R.X = P.X + 1;
	     if (R.X >= dest->W) R.X = dest->W - 1;
	     R.Y = P.Y; R.Z = P.Z;
	     dist_PQ =  walked_dist - (float)P.X * dx;
	     
	     value = (int)((( dx - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float) dest->voxel(R)) / dx);
	     tmp->voxel(Q) = (T) value;
	   }
       }
       dest = tmp;
     }

     if (ndy != dy) {
       tmp = new Volume<T>(dest->W,
			   (int)(((float)dest->H-1.0) * dy / ndy)+1,
			   dest->D);
       for(Q.Y=0; Q.Y < tmp->H; Q.Y++)
	 for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	   for(Q.X=0; Q.X < tmp->W; Q.X++) {
	     
	     walked_dist = (float)Q.Y * ndy;
	     
	     P.X = Q.X;
	     P.Y = (int)(walked_dist/dy);
	     P.Z = Q.Z;
	     R.X = P.X;
	     R.Y = P.Y + 1;
	     if (R.Y >= dest->H) R.Y = dest->H - 1;
	     R.Z = P.Z;
	     dist_PQ =  walked_dist - (float)P.Y * dy;
	     
	     value = (int)(((dy - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float)(dest->voxel(R))) / dy);
	     tmp->voxel(Q) = (T) value;
	   }
       if (dest != this)
	 delete dest;
       dest=tmp;
     }

     if (ndz != dz) {
       tmp = new Volume<T>(dest->W,
			   dest->H,
			   (int)(((float)dest->D-1.0) * dz / ndz)+1);
       for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	 for(Q.Y=0; Q.Y < tmp->H; Q.Y++)
	   for(Q.X=0; Q.X < tmp->W; Q.X++) {
	     
	     walked_dist = (float)Q.Z * ndz;
	     
	     P.X = Q.X; P.Y = Q.Y;
	     P.Z = (int)(walked_dist/dz);
	     
	     R.X = P.X; R.Y = P.Y;
	     R.Z = P.Z + 1;
	     if (R.Z >= dest->D) R.Z = dest->D - 1;	     
	     dist_PQ =  walked_dist - (float)P.Z * dz;
	     
	     value = (int)(((dz - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float)(dest->voxel(R))) / dz);
	     tmp->voxel(Q) = (T) value;
	   }
       if (dest != this)
	 delete dest;
       dest=tmp;
     }
          
     dest->dx=ndx;
     dest->dy=ndy;
     dest->dz=ndz;
     return(dest);
  }

  // left/right, anterior/posterior, feet/head
  enum Orientation { sagittal_lr=0, sagittal_rl=1, 
		     coronal_ap=2, coronal_pa=3, 
		     axial_fh=4, axial_hf=5 };

  Volume<T> *changeOrientation(Orientation from, Orientation to) {
    T4 forw[6], rev[6], tmp, t;
    R3 a,b,c,c2;
    int i,x,y,z,nw,nh,nd,x1,y1,z1;
    double ndx,ndy,ndz;
    Volume<T> *out;

    // basic transformations
    forw[0].identity();
    forw[1].yrot(180.0);
    forw[2].yrot(-90.0);   
    forw[3].yrot(90.0);
    forw[4].xrot(90.0);  tmp.yrot(-90.0); forw[4] *= tmp;
    forw[5].xrot(-90.0); tmp.yrot(-90.0); forw[5] *= tmp;
    
    // compute inverse transforms
    for(i=0;i<6;i++) {
      rev[i] = forw[i];
      rev[i].invert();
    }
    
    t = forw[from];
    t *= rev[to];

    // fix translation
    a.set(0,0,0);
    c.set(W,H,D);
    b = t.apply(a);
    c2 = t.apply(c);
    
    x = min((int) b.X,(int) c2.X);
    y = min((int) b.Y,(int) c2.Y);
    z = min((int) b.Z,(int) c2.Z);

    tmp.translate(-x,-y,-z);
    t *= tmp;

    a.set(W,H,D);
    c.set(0,0,0);
    
    b = t.apply(a);
    c = t.apply(c);
    b -= c;

    nw = (int) ceil(fabs(b.X));
    nh = (int) ceil(fabs(b.Y));
    nd = (int) ceil(fabs(b.Z));

    {    
      R3 org(0,0,0), ux(1,0,0), uy(0,1,0), uz(0,0,1);
      org = t.apply(org);
      ux  = t.apply(ux);
      uy  = t.apply(uy);
      uz  = t.apply(uz);
      ux -= org;
      uy -= org;
      uz -= org;
      ndx = dx;
      ndy = dy;
      ndz = dz;
      if (fabs(ux.X) > 0.90) ndx = dx; else if (fabs(ux.Y) > 0.90) ndy = dx; else ndz = dx;
      if (fabs(uy.X) > 0.90) ndx = dy; else if (fabs(uy.Y) > 0.90) ndy = dy; else ndz = dy;
      if (fabs(uz.X) > 0.90) ndx = dz; else if (fabs(uz.Y) > 0.90) ndy = dz; else ndz = dz;
    }
    
    out = new Volume<T>(nw, nh, nd, ndx, ndy, ndz);
    t.invert();
    
    for(z=0;z<nd;z++)
      for(y=0;y<nh;y++)
	for(x=0;x<nw;x++) {
	  a.set(x,y,z);
	  b = t.apply(a);	
	  x1 = (int) b.X;
	  y1 = (int) b.Y;
	  z1 = (int) b.Z;
	  if (valid(x1,y1,z1))
	    out->voxel(x,y,z) = voxel(x1,y1,z1);
	}

    return out;
  }

  Volume<int> * edtTexture(Volume<int> *mr, Location &center, Volume<int> **tmap=0) {
    Volume<int> *tex;
    Sphere *S=0;
    int i,j,k,sk,maxdepth,ac;
    float s,diag;
    int texsz = 512;
    R3 a,b;
    Location c;
    
    // find sphere center
    S = new Sphere(center.X,center.Y,center.Z);

    maxdepth = (int) sqrt( maximum() );
    diag = sqrt(W*W+H*H+D*D);

    tex = new Volume<int>(texsz,texsz,maxdepth);
    tex->fill(0);

    if (tmap != 0)
      *tmap = new Volume<int>(texsz,texsz,maxdepth);

    for(j=0;j<texsz;j++) {
      for(i=0;i<texsz;i++) {
	for(s=0.0;s<diag;s+=0.25) {
	  a.set( ((float)j)*360.0 / ((float)texsz),
		 ((float)i)*180.0 / ((float)texsz),
		 s );
	  S->sph2rect(b,a);
	  c.X = (int) (b.X);
	  c.Y = (int) (b.Y);
	  c.Z = (int) (b.Z);
	  if (valid(c)) {
	    ac = address(c);
	    k = voxel(ac);
	    if (k<0) break;
	    sk = (int) sqrt(k);
	    // macos crashes here
	    // printf("ac=%d c=%d,%d,%d k=%d sk=%d tex.dim=%d,%d,%d\n",ac,c.X,c.Y,c.Z,k,sk,tex->W,tex->H,tex->D);
	    if  (tex->valid(j,i,sk)) {
	      tex->voxel(j,i,sk) = mr->voxel(ac);
	      if (tmap != NULL) (*tmap)->voxel(j,i,sk) = ac;
	      if (sk>0) {
		tex->voxel(j,i,sk-1) = mr->voxel(ac);
		if (tmap != 0) {
		  (*tmap)->voxel(j,i,sk-1) = ac;
		}
	      }
	    }
	  }
	}
      }
    }

    if (*tmap != 0) {
      (*tmap)->voxel(0) = this->W;
      (*tmap)->voxel(1) = this->H;
      (*tmap)->voxel(2) = this->D;
    }

    delete S;
    return tex;
  }

  // find darker XY slice in this volume, constrained by
  // the given mask. Ignore slices with less than minArea
  // voxels within the mask
  int darkerSlice(Volume<char> *mask, int minArea) {
    int i,j,k,n;
    float acc;
    float minval;
    int minz;
    
    minval = (float) maximum();
    minz = 0;
    
    for(k=0;k<D;k++) {
      acc = 0.0;
      n = 0;

      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  if (mask->valid(i,j,k)) {
	    if (mask->voxel(i,j,k) != 0) {
	      acc += (float) voxel(i,j,k);
	      n++;
	    }
	  }
	}
      
      if (n < minArea) continue;
      acc /= (float) n;
      if (acc < minval) {
	minval = acc;
	minz = k;
      }
    }
    return minz;
  }

  // darkness score for finding the plane between hemispheres
  float planeScore(int z, Volume<char> *mask, T4 &X) {
    int i,j,n=0;
    R3 r;
    float acc=0.0;

    for(j=-H;j<2*H;j++)
      for(i=-W;i<2*W;i++) {
	r.set(i,j,z);
	r = X.apply(r);
	if (mask->valid(r.X,r.Y,r.Z) && valid(r.X,r.Y,r.Z))
	  if (mask->voxel(r.X,r.Y,r.Z) != 0) {
	    acc += voxel(r.X,r.Y,r.Z);
	    n++;
	  }
      }

    if (n!=0) acc /= n;
    return acc;
  }

  Location planeCenter(int z,T4 &X) {
    Location P(0,0,0);
    int i,j;
    R3 r;
    int mv=0;

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	r.set(i,j,z);
	r = X.apply(r);
	if (valid(r.X,r.Y,r.Z))
	  if (voxel(r.X,r.Y,r.Z) > mv) {
	    mv = voxel(r.X,r.Y,r.Z);
	    P.set( (int) (r.X), (int) (r.Y), (int) (r.Z) );
	  }
      }

    return(P);
  }

  void planeFit(int z, Volume<char> *mask, T4 &X,int &niter,const char *outfile=NULL,bool verbose=false) {
    T4 lib[42], at, bestat;
    float myscore, c, best, fscore;
    float cx,cy,cz;
    int i,j,whobest;

    cx = W/2.0;
    cy = H/2.0;
    cz = (float) z;

    const char *desc[42] = {
      "TX/-1","TX/+1",
      "TY/-1","TY/+1",
      "TZ/-1","TZ/+1",
      "RX/-1","RX/+1",
      "RY/-1","RY/+1",
      "RZ/-1","RZ/+1",
      "TX/-5","TX/+5",
      "TY/-5","TY/+5",
      "TZ/-5","TZ/+5",
      "RX/-5","RX/+5",
      "RY/-5","RY/+5",
      "RZ/-5","RZ/+5",
      "RX/-10","RX/+10",
      "RY/-10","RY/+10",
      "RZ/-10","RZ/+10",
      "TX/-10","TX/+10",
      "TY/-10","TY/+10",
      "TZ/-10","TZ/+10",
      "RX/-0.5","RX/+0.5",
      "RY/-0.5","RY/+0.5",
      "RZ/-0.5","RZ/+0.5",
    };

    lib[0].translate(-1.0, 0.0, 0.0);
    lib[1].translate( 1.0, 0.0, 0.0);
    lib[2].translate( 0.0,-1.0, 0.0);
    lib[3].translate( 0.0, 1.0, 0.0);
    lib[4].translate( 0.0, 0.0,-1.0);
    lib[5].translate( 0.0, 0.0, 1.0);

    lib[6].xrot(1.0, cx,cy,cz);
    lib[7].xrot(-1.0, cx,cy,cz);
    lib[8].yrot(1.0, cx,cy,cz);
    lib[9].yrot(-1.0, cx,cy,cz);
    lib[10].zrot(1.0, cx,cy,cz);
    lib[11].zrot(-1.0, cx,cy,cz);

    lib[12].translate(-5.0, 0.0, 0.0);
    lib[13].translate( 5.0, 0.0, 0.0);
    lib[14].translate( 0.0,-5.0, 0.0);
    lib[15].translate( 0.0, 5.0, 0.0);
    lib[16].translate( 0.0, 0.0,-5.0);
    lib[17].translate( 0.0, 0.0, 5.0);

    lib[18].xrot(5.0, cx,cy,cz);
    lib[19].xrot(-5.0, cx,cy,cz);
    lib[20].yrot(5.0, cx,cy,cz);
    lib[21].yrot(-5.0, cx,cy,cz);
    lib[22].zrot(5.0, cx,cy,cz);
    lib[23].zrot(-5.0, cx,cy,cz);

    lib[24].xrot(10.0, cx,cy,cz);
    lib[25].xrot(-10.0, cx,cy,cz);
    lib[26].yrot(10.0, cx,cy,cz);
    lib[27].yrot(-10.0, cx,cy,cz);
    lib[28].zrot(10.0, cx,cy,cz);
    lib[29].zrot(-10.0, cx,cy,cz);

    lib[30].translate(-10.0, 0.0, 0.0);
    lib[31].translate( 10.0, 0.0, 0.0);
    lib[32].translate( 0.0,-10.0, 0.0);
    lib[33].translate( 0.0, 10.0, 0.0);
    lib[34].translate( 0.0, 0.0,-10.0);
    lib[35].translate( 0.0, 0.0, 10.0);

    lib[36].xrot(0.5, cx,cy,cz);
    lib[37].xrot(-0.5, cx,cy,cz);
    lib[38].yrot(0.5, cx,cy,cz);
    lib[39].yrot(-0.5, cx,cy,cz);
    lib[40].zrot(0.5, cx,cy,cz);
    lib[41].zrot(-0.5, cx,cy,cz);

    fscore = myscore = planeScore(z,mask,X);

    j = 0;
    do {
      whobest = -1;
      best = myscore;
      bestat = X;
    
      for(i=0;i<42;i++) {
	at = X * lib[i];
	c = planeScore(z,mask,at);
	if (c < best && fabsf(c-best)>1.0E-4f) {
	  best = c;
	  whobest = i;
	  bestat = at;
	}
      }

      if (whobest >= 0) {
	X = bestat;
	j++;
	if (verbose) {
	  cout << "planeFit iteration " << j << ", " << desc[whobest] << ", score " << myscore << " => " << best << endl;
	}

	myscore = best;

      }

    } while(whobest >= 0);

    niter = j;
    if (verbose)
      cout << ">> " << j << " alignment iterations, optimization: " << fscore << " => " << best << endl; 

    if (outfile!=NULL)
      planeWrite(z,X,outfile);
  }

  void planeWrite(int z,T4 &X,const char *outfile) {
    
    int i,j;
    R3 r;
    Volume<short int> *tmp;

    tmp = new Volume<short int>(W,H,D,dx,dy,dz);
    for(i=0;i<N;i++)
      tmp->voxel(i) = (short int) (this->voxel(i));

    for(j=-H;j<2*H;j++)
      for(i=-W;i<2*W;i++) {
	r.set(i,j,z);
	r = X.apply(r);
	if (valid(r.X,r.Y,r.Z))
	  tmp->voxel(r.X,r.Y,r.Z) = 2000;
      }

    tmp->writeSCN(outfile);
    delete tmp;
  }

  void transform(T4 &X) {
    transformNN(X);
  }

  Volume<T> *paddedTransformLI(T4 &X) {
    R3 corner[8],a;
    Volume<T> *tmp;
    int x[2],y[2],z[2];
    int nw,nh,nd,hz;
    int i,j,k,ix,iy,iz,jx,jy,jz;
    float ws,vs,w;
    float sq3;
    T4 A,IX;

    corner[0].set(0,0,0);
    corner[1].set(W-1,0,0);
    corner[2].set(0,H-1,0);
    corner[3].set(0,0,D-1);
    corner[4].set(W-1,H-1,0);
    corner[5].set(W-1,0,D-1);
    corner[6].set(0,H-1,D-1);
    corner[7].set(W-1,H-1,D-1);

    for(i=0;i<8;i++)
      corner[i] = X.apply(corner[i]);

    x[0] = x[1] = (int) (corner[0].X);
    y[0] = y[1] = (int) (corner[0].Y);
    z[0] = z[1] = (int) (corner[0].Z);    

    for(i=0;i<8;i++) {
      if (corner[i].X < x[0]) x[0] = (int) (corner[i].X);
      if (corner[i].X > x[1]) x[1] = (int) (corner[i].X);
      if (corner[i].Y < y[0]) y[0] = (int) (corner[i].Y);
      if (corner[i].Y > y[1]) y[1] = (int) (corner[i].Y);
      if (corner[i].Z < z[0]) z[0] = (int) (corner[i].Z);
      if (corner[i].Z > z[1]) z[1] = (int) (corner[i].Z);
    }

    hz = D/2;
    if (abs(z[1] - hz) > abs(hz - z[0])) {
      z[0] = hz - abs(z[1] - hz);
    } else {
      z[1] = hz + abs(hz - z[0]);
    }

    nw = x[1] - x[0] + 1;
    nh = y[1] - y[0] + 1;
    nd = z[1] - z[0] + 1;

    IX = X;
    A.translate(-x[0],-y[0],-z[0]);
    IX *= A;    
    IX.invert();

    tmp = new Volume<T>(nw,nh,nd,dx,dy,dz);

    sq3 = sqrt(3.0);

    for(k=0;k<nd;k++)
      for(j=0;j<nh;j++)
	for(i=0;i<nw;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);

	  ws = 0.0f;
	  vs = 0.0f;

	  for(iz=0;iz<2;iz++)
	    for(iy=0;iy<2;iy++)
	      for(ix=0;ix<2;ix++) {
		jx = ((int)floor(a.X)) + ix;
		jy = ((int)floor(a.Y)) + iy;
		jz = ((int)floor(a.Z)) + iz;
		if (valid(jx,jy,jz)) {
		  w = sq3 - sqrt( (jx-a.X)*(jx-a.X) +
				  (jy-a.Y)*(jy-a.Y) +
				  (jz-a.Z)*(jz-a.Z) );
		  vs += w * (this->voxel(jx,jy,jz)) ;
		  ws += w;
		}
	      }
	  
	  if (ws != 0.0f) {
	    vs /= ws;
	    tmp->voxel(i,j,k) = (T) vs;
	  }
	}
    return tmp;
  }

  void transformLI(T4 &X) {
    R3 a;
    T4 IX;
    int i,j,k,ix,iy,iz,jx,jy,jz;
    float ws,vs,w;
    Volume<T> *tmp;
    float sq3;

    tmp = new Volume<T>(W,H,D,dx,dy,dz);

    IX = X;
    IX.invert();
    
    sq3 = sqrt(3.0);

    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);

	  ws = 0.0f;
	  vs = 0.0f;

	  for(iz=0;iz<2;iz++)
	    for(iy=0;iy<2;iy++)
	      for(ix=0;ix<2;ix++) {
		jx = ((int)floor(a.X)) + ix;
		jy = ((int)floor(a.Y)) + iy;
		jz = ((int)floor(a.Z)) + iz;
		if (valid(jx,jy,jz)) {
		  w = sq3 - sqrt( (jx-a.X)*(jx-a.X) +
				  (jy-a.Y)*(jy-a.Y) +
				  (jz-a.Z)*(jz-a.Z) );
		  vs += w * (this->voxel(jx,jy,jz)) ;
		  ws += w;
		}
	      }
	  
	  if (ws != 0.0f) {
	    vs /= ws;
	    tmp->voxel(i,j,k) = (T) vs;
	  }
	}

    for(i=0;i<N;i++)
      this->voxel(i) = tmp->voxel(i);
    delete tmp;   
  }

  void transformNN(T4 &X) {
    R3 a;
    T4 IX;
    int i,j,k;
    Volume<T> *tmp;

    tmp = new Volume<T>(W,H,D,dx,dy,dz);

    IX = X;
    IX.invert();
    
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);
	  if (valid(a.X,a.Y,a.Z))
	    tmp->voxel(i,j,k) = this->voxel(a.X,a.Y,a.Z);
	}

    for(i=0;i<N;i++)
      this->voxel(i) = tmp->voxel(i);
    delete tmp;
  }


  Volume<int> ** watershed2D() {
    Volume<int> **R, *pred, *label, *cost;
    DiscAdjacency A(1.0);
    BasicPQ *Q;
    int i,j,k,m1,a1,p,s,c1,c2,aq;
    Location lp,lq;

    R = new Volume<int> *[3];

    R[0] = pred  = new Volume<int>(W,H,D);
    R[1] = label = new Volume<int>(W,H,D);
    R[2] = cost  = new Volume<int>(W,H,D);

    m1 = (int) (maximum() + 1);
    Q = new BasicPQ(N,m1+2);

    for(k=0;k<D;k++) {

      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {

	  a1 = address(i,j,k);
	  cost->voxel(a1) = m1;
	  Q->insert(a1, this->voxel(a1));

	  label->voxel(a1) = a1;
	  pred->voxel(a1) = -1;

	}

      while(!Q->empty()) {
	p = Q->remove();
	if(cost->voxel(p) == m1)
	  cost->voxel(p) = this->voxel(p);
	lp.set( xOf(p), yOf(p), zOf(p) );
	for(s=0;s<A.size();s++) {
	  lq = A.neighbor(lp,s);
	  if (valid(lq)) {
	    c1 = cost->voxel(lq);
	    c2 = max( cost->voxel(lp), this->voxel(lq) );
	    if (c2 < c1) {
	      aq = address(lq);
	      if (Q->colorOf(aq) == PriorityQueue::Gray)
		Q->update(aq,c1,c2);
	      else
		Q->insert(aq,c2);
	      cost->voxel(lq) = c2;
	      pred->voxel(lq) = p;
	      label->voxel(lq) = label->voxel(lp);
	    }
	  }
	}
      }
      
    }
    delete Q;
    return(R);
  }

  Volume<T> * sobelGradient2D() {

    Volume<T> *dest;
    dest = new Volume<T>(W,H,D,dx,dy,dz);
    int i,j,k,p;
    T sx, sy;
    static const int da[6] = {-1,-1,-1,1,1,1};
    static const int db[6] = {-1,0,1,-1,0,1};
    static const T w[6] = {-1,-2,-1,1,2,1};
    
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  sx = sy = 0;
	  for(p=0;p<6;p++) {
	    if (valid(i+da[p],j+db[p],k)) sx += w[p] * voxel(i+da[p],j+db[p],k);
	    if (valid(i+db[p],j+da[p],k)) sy += w[p] * voxel(i+db[p],j+da[p],k);
	  }
	  dest->voxel(i,j,k) = (T) sqrt(sx*sx+sy*sy);
	}
      
    return dest;
  }

  Volume<T> * morphGradient2D(float radius) {

    Volume<T> *dest;
    dest = new Volume<T>(W,H,D,dx,dy,dz);
    int i,j,k,ii,jj;
    int rq;
    T vmin, vmax, c;

    rq = (int) (radius*radius);

    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  
	  vmin = vmax = voxel(i,j,k);

	  for(jj=-rq;jj<=rq;jj++)
	    for(ii=-rq;ii<=rq;ii++)
	      if (ii*ii + jj*jj <= rq)
		if (valid(i+ii,j+jj,k)) {
		  c = voxel(i+ii,j+jj,k);
		  if (c < vmin) vmin = c; else if (c > vmax) vmax = c;
		}
	  
	  dest->voxel(i,j,k) = vmax - vmin;
	}
      
    return dest;
  }

  Volume<T> * morphGradient3D(float radius) {

    Volume<T> *dest;
    dest = new Volume<T>(W,H,D,dx,dy,dz);
    int i,j,k,ii,jj,kk;
    int rq;
    T vmin, vmax, c;

    rq = (int) (radius*radius);

    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  
	  vmin = vmax = voxel(i,j,k);

	  for(kk=-rq;kk<=rq;kk++)
	    for(jj=-rq;jj<=rq;jj++)
	      for(ii=-rq;ii<=rq;ii++)
		if (ii*ii + jj*jj + kk*kk <= rq)
		  if (valid(i+ii,j+jj,k+kk)) {
		    c = voxel(i+ii,j+jj,k+kk);
		    if (c < vmin) vmin = c; else if (c > vmax) vmax = c;
		  }
	  
	  dest->voxel(i,j,k) = vmax - vmin;
	}
      
    return dest;
  }

  /* visualization */

  void invalidateRenderCache() {
    if (vqbuf!=NULL) delete[] vqbuf;
    vqbuf = NULL;
  }

  void render(Image &dest, T3 &rot, 
	      RenderingContext &rc, Color &objcolor, float opacity=1.0,
	      RenderType rendertype = RenderType_EQ, int rendervalue=2) 
  {
    float *zbuf, *nbuf, *xbuf, *ybuf, dw2, dh2;
    int  *ibuf;
    char *rbuf;
    int i,j,k,m,n,o,dw,dh;
    R3 a,b, light(0,0,-1);
    //    Timestamp t0,t4;
    
    //    t0 = Timestamp::now();

    dw = dest.W;
    dh = dest.H;
    dw2 = dw / 2.0;
    dh2 = dh / 2.0;

    zbuf = rc.zbuf;
    nbuf = rc.nbuf;
    xbuf = rc.xbuf;
    ybuf = rc.ybuf;
    rbuf = rc.rbuf;
    ibuf = rc.ibuf;
    rc.prepareNext();
    rc.clearI();

    // project
    comp_vqbuf(rendertype, rendervalue);
    for(i=0;i<vqlen;i++) {
      a.X = vqbuf[i].x; a.Y = vqbuf[i].y; a.Z = vqbuf[i].z;      
      b = rot.apply(a);
      b.X += dw2;
      b.Y += dh2;
      if (dest.valid((int)(b.X),(int)(b.Y))) {
	j = (int) (b.X) + dw * (int) (b.Y);
	if (b.Z < zbuf[j]) {
	  zbuf[j] = b.Z;
	  xbuf[j] = b.X;
	  ybuf[j] = b.Y;
	  rbuf[j] = 1;
	  ibuf[j] = i;
	}
      }
    }

    // t1 = Timestamp::now();

    // splat
    int sx[3] = {1,0,1};
    int sy[3] = {0,1,1};
    for(j=dh-2;j>0;j--)
      for(i=dw-2;i>0;i--) {
	n = i+dw*j;
	if (zbuf[n] != RenderingContext::InfZ) {
	  for(k=0;k<3;k++) {
	    m = i+sx[k]+dw*(j+sy[k]);
	    if (zbuf[m] > zbuf[n]) {
	      zbuf[m] = zbuf[n];
	      xbuf[m] = xbuf[n] + sx[k];
	      ybuf[m] = ybuf[n] + sy[k];
	      rbuf[m] = rbuf[n];
	      ibuf[m] = ibuf[n];
	    }
	  }
	}
      }

    // count renderings
    for(i=0;i<dw*dh;i++)
      if (ibuf[i] >= 0)
	vqbuf[ibuf[i]].count++;
    rendercount++;

    if (rendercount % 5 == 0)
      sort(vqbuf, vqbuf + vqlen);

    rc.clearN();

    // t2 = Timestamp::now();

    // find normals

    int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int qx[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
    int qy[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };

    R3 p[3], w[3], normal;
    int nv;

    for(j=2;j<dh-2;j++)
      for(i=2;i<dw-2;i++) {
	n = i+j*dw;
	if (rbuf[n] != 0) {
	  nv = 0;
	  normal.set(0,0,0);
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + nx[(k+1)%8]) + dw*(j+ny[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + nx[k]) + dw*(j+ny[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + qx[(k+1)%8]) + dw*(j+qy[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + qx[k]) + dw*(j+qy[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }

	  float est;
	  normal.normalize();
	  est = (nv==0) ? 0.0 : normal.inner(light);
	  if (est < 0.0) est = 0.0;
	  nbuf[n] = est;
	}
      }
    
    // t3 = Timestamp::now();

    // cast
    float kz=1.50,ka=0.20,ks=0.40,kd=0.60,pd,ps,Y;
    Color tmpc,c2,cmix;

    tmpc = objcolor;
    tmpc.rgb2ycbcr();
    
    for(j=0;j<dh;j++)
      for(i=0;i<dw;i++) {
	n = i+dw*j;
	if (rbuf[n]!=0) {
	  Y = tmpc.R / 255.0;
	  pd = nbuf[n];
	  if (acos(nbuf[n]) > M_PI / 4.0) 
	    ps = 0.0;
	  else
	    ps = pow(cos(2.0*nbuf[n]),10.0);
	  Y = ka + kz * Y * (kd*pd + ks*ps);
	  Y *= 255.0;
	  if (Y > 255.0) Y=255.0;
          if (Y < 0.0) Y = 0.0;
	  c2 = tmpc;
	  c2.R = (uint8_t) Y;
	  c2.ycbcr2rgb();
	  if (opacity==1.0)
	    dest.set(i,j,c2);
	  else {
	    cmix = dest.get(i,j);
	    cmix.mix(c2,opacity);
	    dest.set(i,j,cmix);
	  }
	}
      }

    //    t4 = Timestamp::now();

    /*
    cout << "r-project: " << (t1-t0) << endl;
    cout << "r-splat  : " << (t2-t1) << endl;
    cout << "r-normals: " << (t3-t2) << endl;
    cout << "r-cast   : " << (t4-t3) << endl;
    cout << "obj=" << (uint64_t)(this) << " / ";
    cout << "r-full   : " << (t4-t0) << endl;
    */
  }

  void render(Image &dest, T3 &rot, 
	      RenderingContext &rc, Color &objcolor, R3 &lightvec, float opacity=1.0,
	      RenderType rendertype = RenderType_EQ, int rendervalue=2,
	      float kz=1.50,float ka=0.20,float ks=0.40,float kd=0.60) 
  {
    float *zbuf, *nbuf, *xbuf, *ybuf, dw2, dh2;
    int  *ibuf;
    char *rbuf;
    int i,j,k,m,n,o,dw,dh;
    R3 a,b,light;
    //    Timestamp t0,t4;
    
    //    t0 = Timestamp::now();

    light = lightvec;
    light.normalize();

    dw = dest.W;
    dh = dest.H;
    dw2 = dw / 2.0;
    dh2 = dh / 2.0;

    zbuf = rc.zbuf;
    nbuf = rc.nbuf;
    xbuf = rc.xbuf;
    ybuf = rc.ybuf;
    rbuf = rc.rbuf;
    ibuf = rc.ibuf;
    rc.prepareNext();
    rc.clearI();

    // project
    comp_vqbuf(rendertype, rendervalue);
    for(i=0;i<vqlen;i++) {
      a.X = vqbuf[i].x; a.Y = vqbuf[i].y; a.Z = vqbuf[i].z;      
      b = rot.apply(a);
      b.X += dw2;
      b.Y += dh2;
      if (dest.valid((int)(b.X),(int)(b.Y))) {
	j = (int) (b.X) + dw * (int) (b.Y);
	if (b.Z < zbuf[j]) {
	  zbuf[j] = b.Z;
	  xbuf[j] = b.X;
	  ybuf[j] = b.Y;
	  rbuf[j] = 1;
	  ibuf[j] = i;
	}
      }
    }

    // t1 = Timestamp::now();

    // splat
    int sx[3] = {1,0,1};
    int sy[3] = {0,1,1};
    for(j=dh-2;j>0;j--)
      for(i=dw-2;i>0;i--) {
	n = i+dw*j;
	if (zbuf[n] != RenderingContext::InfZ) {
	  for(k=0;k<3;k++) {
	    m = i+sx[k]+dw*(j+sy[k]);
	    if (zbuf[m] > zbuf[n]) {
	      zbuf[m] = zbuf[n];
	      xbuf[m] = xbuf[n] + sx[k];
	      ybuf[m] = ybuf[n] + sy[k];
	      rbuf[m] = rbuf[n];
	      ibuf[m] = ibuf[n];
	    }
	  }
	}
      }

    // count renderings
    for(i=0;i<dw*dh;i++)
      if (ibuf[i] >= 0)
	vqbuf[ibuf[i]].count++;
    rendercount++;

    if (rendercount % 5 == 0)
      sort(vqbuf, vqbuf + vqlen);

    rc.clearN();

    // t2 = Timestamp::now();

    // find normals

    int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int qx[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
    int qy[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };

    R3 p[3], w[3], normal;
    int nv;

    for(j=2;j<dh-2;j++)
      for(i=2;i<dw-2;i++) {
	n = i+j*dw;
	if (rbuf[n] != 0) {
	  nv = 0;
	  normal.set(0,0,0);
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + nx[(k+1)%8]) + dw*(j+ny[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + nx[k]) + dw*(j+ny[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + qx[(k+1)%8]) + dw*(j+qy[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + qx[k]) + dw*(j+qy[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }

	  float est;
	  normal.normalize();
	  est = (nv==0) ? 0.0 : normal.inner(light);
	  if (est < 0.0) est = 0.0;
	  nbuf[n] = est;
	}
      }
    
    // t3 = Timestamp::now();

    // cast
    float pd,ps,Y;
    Color tmpc,c2,cmix;

    tmpc = objcolor;
    tmpc.rgb2ycbcr();
    
    for(j=0;j<dh;j++)
      for(i=0;i<dw;i++) {
	n = i+dw*j;
	if (rbuf[n]!=0) {
	  Y = tmpc.R / 255.0;
	  pd = nbuf[n];
	  if (acos(nbuf[n]) > M_PI / 4.0) 
	    ps = 0.0;
	  else
	    ps = pow(cos(2.0*nbuf[n]),10.0);
	  Y = ka + kz * Y * (kd*pd + ks*ps);
	  Y *= 255.0;
	  if (Y > 255.0) Y=255.0;
          if (Y < 0.0) Y = 0.0;
	  c2 = tmpc;
	  c2.R = (uint8_t) Y;
	  c2.ycbcr2rgb();
	  if (opacity==1.0)
	    dest.set(i,j,c2);
	  else {
	    cmix = dest.get(i,j);
	    cmix.mix(c2,opacity);
	    dest.set(i,j,cmix);
	  }
	}
      }

    //    t4 = Timestamp::now();

    /*
    cout << "r-project: " << (t1-t0) << endl;
    cout << "r-splat  : " << (t2-t1) << endl;
    cout << "r-normals: " << (t3-t2) << endl;
    cout << "r-cast   : " << (t4-t3) << endl;
    cout << "obj=" << (uint64_t)(this) << " / ";
    cout << "r-full   : " << (t4-t0) << endl;
    */
  }

  void comp_vqbuf(RenderType rt, int rval, bool force=false) {
    static RenderType prt = RenderType_NONE;
    static int        prv = -1;
    int i,c;

    // vqbuf is valid
    if (vqbuf!=NULL && rt==prt && rval==prv && !force)
      return;

    if (vqbuf!=NULL) delete[] vqbuf;
    vqbuf = NULL;

    prt = rt;
    prv = rval;

    switch(rt) {
    case RenderType_EQ:
      for(c=0,i=0;i<N;i++)
	if (data[i] == rval) ++c;
      break;
    case RenderType_LT:
      for(c=0,i=0;i<N;i++)
	if (data[i] < rval) ++c;
      break;
    case RenderType_LE:
      for(c=0,i=0;i<N;i++)
	if (data[i] <= rval) ++c;
      break;
    case RenderType_GT:
      for(c=0,i=0;i<N;i++)
	if (data[i] > rval) ++c;
      break;
    case RenderType_GE:
      for(c=0,i=0;i<N;i++)
	if (data[i] >= rval) ++c;
      break;
    default:
      c = 0;
      return;
    }

    vqbuf = new VoxelQuad[c];
    vqlen = c;

    switch(rt) {
    case RenderType_EQ:
      for(c=0,i=0;i<N;i++)
	if (data[i] == rval)
	  vqbuf[c++].set(xOf(i) - W/2, yOf(i) - H/2, zOf(i) - D/2);
      break;
    case RenderType_LT:
      for(c=0,i=0;i<N;i++)
	if (data[i] < rval)
	  vqbuf[c++].set(xOf(i) - W/2, yOf(i) - H/2, zOf(i) - D/2);
      break;
    case RenderType_LE:
      for(c=0,i=0;i<N;i++)
	if (data[i] <= rval)
	  vqbuf[c++].set(xOf(i) - W/2, yOf(i) - H/2, zOf(i) - D/2);
      break;
    case RenderType_GT:
      for(c=0,i=0;i<N;i++)
	if (data[i] > rval)
	  vqbuf[c++].set(xOf(i) - W/2, yOf(i) - H/2, zOf(i) - D/2);
      break;
    case RenderType_GE:
      for(c=0,i=0;i<N;i++)
	if (data[i] >= rval)
	  vqbuf[c++].set(xOf(i) - W/2, yOf(i) - H/2, zOf(i) - D/2);
      break;
    default:
      c=0;
    }
  }

  void edtRender(Image &dest, T3 &rot, RenderingContext &rc,int *lut, 
		 Volume<int> *orig, float depth, float opacity=1.0,
		 Volume<char> *tags=NULL, float tagopacity=1.0, Color *tagcolors=NULL) 
  {
    int i,j,k,dw,dh,sqd,usqd;
    float dw2,dh2;
    R3 a,b,c2;
    float *zbuf;
    int   *ibuf;
    Color c, cmix;

    sqd = (int) (depth*depth);
    usqd = (int) ((depth+3.0)*(depth+3.0));
    
    dw = dest.W;
    dh = dest.H;
    dw2 = dw / 2.0;
    dh2 = dh / 2.0;
    c2.set(W/2.0,H/2.0,D/2.0);

    rc.prepareNext();
    rc.clearI();

    zbuf = rc.zbuf;
    ibuf = rc.ibuf;

    // project
    for(i=0;i<N;i++) {
      if (data[i] >= sqd && data[i] < usqd) {
	a.X = xOf(i); a.Y = yOf(i); a.Z = zOf(i);
	a -= c2;
	b = rot.apply(a);
	b.X += dw2;
	b.Y += dh2;
	if (dest.valid((int)(b.X),(int)(b.Y))) {
	  j = (int) (b.X) + dw * (int) (b.Y);
	  if (b.Z < zbuf[j]) {
	    zbuf[j] = b.Z;
	    ibuf[j] = i;
	  }

	}
      }
    }

    // render
    for(j=0;j<dh;j++)
      for(i=0;i<dw;i++) {
	k = ibuf[i + j*dw];
	if (k>=0) {
	  c = lut[ orig->voxel(k) ];

	  if (tags!=NULL && tagopacity != 0.0 && tagcolors!=NULL)
	    if (tags->voxel(k) != 0)
	      c.mix(tagcolors[ (int) (tags->voxel(k)) ], tagopacity);

	  if (opacity==1.0)
	    dest.set(i,j,c);
	  else {
	    cmix = dest.get(i,j);
	    cmix.mix(c,opacity);
	    dest.set(i,j,cmix);
	  }
	}
      }
  }

  void edtRender2(Image &dest, T3 &rot, RenderingContext &rc,int *lut, 
		  Volume<int> *orig, float depth, float opacity=1.0,
		  Volume<char> *tags=NULL, float tagopacity=1.0, Color *tagcolors=NULL) 
  {
    int i,j,k,dw,dh,sqd,usqd,n,m,o;
    float dw2,dh2;
    R3 a,b,c2,light(0,0,-1);
    float *zbuf, *nbuf, *xbuf, *ybuf;
    char *rbuf;
    int   *ibuf, *icp;
    Color c, cmix;

    sqd = (int) (depth*depth);
    usqd = (int) ((depth+3.0)*(depth+3.0));
    
    dw = dest.W;
    dh = dest.H;
    dw2 = dw / 2.0;
    dh2 = dh / 2.0;
    c2.set(W/2.0,H/2.0,D/2.0);

    rc.prepareNext();
    rc.clearI();

    zbuf = rc.zbuf;
    nbuf = rc.nbuf;
    xbuf = rc.xbuf;
    ybuf = rc.ybuf;
    rbuf = rc.rbuf;
    ibuf = rc.ibuf;

    // project
    for(i=0;i<N;i++) {
      if (data[i] >= sqd && data[i] < usqd) {
	a.X = xOf(i); a.Y = yOf(i); a.Z = zOf(i);
	a -= c2;
	b = rot.apply(a);
	b.X += dw2;
	b.Y += dh2;
	if (dest.valid((int)(b.X),(int)(b.Y))) {
	  j = (int) (b.X) + dw * (int) (b.Y);
	  if (b.Z < zbuf[j]) {
	    zbuf[j] = b.Z;
	    xbuf[j] = b.X;
	    ybuf[j] = b.Y;
	    rbuf[j] = 1;
	    ibuf[j] = i;
	  }

	}
      }
    }

    // splat
    int sx[3] = {1,0,1};
    int sy[3] = {0,1,1};
    for(j=dh-2;j>0;j--)
      for(i=dw-2;i>0;i--) {
	n = i+dw*j;
	if (zbuf[n] != RenderingContext::InfZ) {
	  for(k=0;k<3;k++) {
	    m = i+sx[k]+dw*(j+sy[k]);
	    if (zbuf[m] > zbuf[n]) {
	      zbuf[m] = zbuf[n];
	      xbuf[m] = xbuf[n] + sx[k];
	      ybuf[m] = ybuf[n] + sy[k];
	      rbuf[m] = rbuf[n];
	      ibuf[m] = ibuf[n];
	    }
	  }
	}
      }

    icp = new int[dw*dh];
    memcpy(icp,ibuf,dw*dh*sizeof(int));

    // normals
    rc.clearN();

    int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
    int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

    int qx[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
    int qy[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };

    R3 p[3], w[3], normal;
    int nv;

    for(j=2;j<dh-2;j++)
      for(i=2;i<dw-2;i++) {
	n = i+j*dw;
	if (rbuf[n] != 0) {
	  nv = 0;
	  normal.set(0,0,0);
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + nx[(k+1)%8]) + dw*(j+ny[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + nx[k]) + dw*(j+ny[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }
	  for(k=0;k<8;k++) {
	    p[0].set(xbuf[n],ybuf[n],zbuf[n]);
	    m = (i + qx[(k+1)%8]) + dw*(j+qy[(k+1)%8]);
	    if (rbuf[m]==0) continue;
	    p[1].set(xbuf[m],ybuf[m],zbuf[m]);
	    o = (i + qx[k]) + dw*(j+qy[k]);
	    if (rbuf[o]==0) continue;
	    p[2].set(xbuf[o],ybuf[o],zbuf[o]);

	    w[0] = p[1]-p[0];
	    w[1] = p[2]-p[0];
	    w[2] = w[0].cross(w[1]);
	    w[2].normalize();
	    normal += w[2];
	    ++nv;
	  }

	  float est;
	  normal.normalize();
	  est = (nv==0) ? 0.0 : normal.inner(light);
	  if (est < 0.0) est = 0.0;
	  nbuf[n] = est;
	}
      }

    float kz=1.50,ka=0.20,ks=0.40,kd=0.60,pd,ps,Y;
    Color tmpc,c3;

    // render
    for(j=0;j<dh;j++)
      for(i=0;i<dw;i++) {
	n = i + dw*j;
	if (rbuf[n]!=0) {

	  k = icp[n];
	  tmpc = lut[ orig->voxel(k) ];

	  if (tags!=NULL && tagopacity != 0.0 && tagcolors!=NULL)
	    if (tags->voxel(k) != 0)
	      tmpc.mix(tagcolors[ (int) (tags->voxel(k)) ], tagopacity);

	  tmpc.rgb2ycbcr();
	  Y = tmpc.R / 255.0;
	  pd = nbuf[n];
	  if (acos(nbuf[n]) > M_PI / 4.0)
	    ps = 0.0;
	  else
	    ps = pow(cos(2.0*nbuf[n]),10.0);
	  Y = ka + kz * Y * (kd*pd + ks*ps);
	  Y *= 255.0;
	  if (Y > 255.0) Y=255.0;
          if (Y < 0.0) Y = 0.0;
	  c3 = tmpc;
	  c3.R = (uint8_t) Y;
	  c3.ycbcr2rgb();

	  if (opacity==1.0)
	    dest.set(i,j,c3);
	  else {
	    cmix = dest.get(i,j);
	    cmix.mix(c3,opacity);
	    dest.set(i,j,cmix);
	  }
	}
      }
    delete icp;
  }

 private:
  T *data;

  // rendering cache
  VoxelQuad *vqbuf;
  int        vqlen;
  unsigned int rendercount;
  iterator anyi,ibegin,iend;
  int szPartial, szTotal, szCompressed;

  // flip y axis
  void niftiFlip() {
    int z,i,j;
    T *tmp;
    tmp = new T[W];
    if (tmp == NULL) return;
    for(z=0;z<D;z++) {
      i = 0; j = H-1;
      for(i=0,j=H-1;i<j;i++,j--) {
	memcpy(tmp, &data[z*WxH + W*i], W*sizeof(T));
	memcpy(&data[z*WxH + W*i], &data[z*WxH + W*j], W*sizeof(T));
	memcpy(&data[z*WxH + W*j], tmp, W*sizeof(T));
      }
    }
    delete tmp;
  }

};

typedef enum { SMReal, SMImaginary, SMMagnitude, SMPhase, SMSpace } SpectrumMap;

class Spectrum {
 public:

  static const SpectrumMap Real = SMReal;
  static const SpectrumMap Imaginary = SMImaginary;
  static const SpectrumMap Magnitude = SMMagnitude;
  static const SpectrumMap Phase = SMPhase;
  static const SpectrumMap Space = SMSpace;

  Spectrum() {
    allocated = false; W=H=0;
  }
  
  Spectrum(float *data, int width, int height) {
    allocated = false; W=H=0;
    compute(data,width,height);
  }

  Spectrum(Spectrum *src) {
    W=src->W;
    H=src->H;    
    allocated=false;
    if (src->allocated)
      alloc();
    memcpy(v_real,src->v_real,W*H*sizeof(double));
    memcpy(v_imag,src->v_imag,W*H*sizeof(double));
    memcpy(v_magnitude,src->v_magnitude,W*H*sizeof(double));
    memcpy(v_phase,src->v_phase,W*H*sizeof(double));
    memcpy(v_data,src->v_data,W*H*sizeof(double));
  }


  ~Spectrum() {
    dealloc();
  }

  void compute(float *data, int width, int height) {
    fft2d(data,width,height);
    fftMagPhase();    
  }

  void inverse() { invfft2d(); }

  void lowpass(float t1,float t2) {
    float p1,p2,f,d;
    int i,j;

    p1 = (W/2.0) * t1;
    p2 = (W/2.0) * t2;

    //    cout << "W=" << W << endl;
    //    cout << "t1-t2=" << t1 << " " << t2 << endl;
    //    cout << "p1-p2=" << p1 << " " << p2 << endl;
    if (p1 > p2) {
      d = p1;
      p1 = p2;
      p2 = d;
    }
    
    for(j=0;j<H/2;j++)
      for(i=0;i<W/2;i++) {
	d = sqrt(i*i+j*j);
	if (d < p1)
	  f = 1.0;
	else if (d > p2)
	  f = 0.0;
	else
	  f = 1.0 - ((d-p1) / (p2-p1));

	real(i,j) *= f;
	imag(i,j) *= f;

	real(W-i-1,j) *= f;
	imag(W-i-1,j) *= f;

	real(i,H-j-1) *= f;
	imag(i,H-j-1) *= f;

	real(W-i-1,H-j-1) *= f;
	imag(W-i-1,H-j-1) *= f;

      }

    fftMagPhase();
  }

  void highpass(float t1,float t2) {
    float p1,p2,f,d;
    int i,j;

    p1 = (W/2) * t1;
    p2 = (W/2) * t2;
    if (p1 > p2) {
      d = p1;
      p1 = p2;
      p2 = d;
    }
    
    for(j=0;j<H/2;j++)
      for(i=0;i<W/2;i++) {
	d = sqrt(i*i+j*j);
	if (d < p1)
	  f = 0.0;
	else if (d > p2)
	  f = 1.0;
	else
	  f = ((d-p1) / (p2-p1));

	real(i,j) *= f;
	imag(i,j) *= f;

	real(W-i-1,j) *= f;
	imag(W-i-1,j) *= f;

	real(i,H-j-1) *= f;
	imag(i,H-j-1) *= f;

	real(W-i-1,H-j-1) *= f;
	imag(W-i-1,H-j-1) *= f;

      }

    fftMagPhase();
  }

  double & real(int x,int y) { return(v_real[x+y*W]); }
  double & imag(int x,int y) { return(v_imag[x+y*W]); }
  double & mag(int x,int y) { return(v_magnitude[x+y*W]); }
  double & magnitude(int x,int y) { return(v_magnitude[x+y*W]); }
  double & phase(int x,int y) { return(v_phase[x+y*W]); }
  double & space(int x,int y) { return(v_data[x+y*W]); }

  double & real(int i) { return(v_real[i]); }
  double & imag(int i) { return(v_imag[i]); }
  double & mag(int i) { return(v_magnitude[i]); }
  double & magnitude(int i) { return(v_magnitude[i]); }
  double & phase(int i) { return(v_phase[i]); }
  double & space(int i) { return(v_data[i]); }

  double fde() {
    double sum = 0, h = 0, norm;
    int i,N;
    N = W*H;
    for(i=0;i<N;i++)
      sum += v_magnitude[i];
    if (sum == 0.0) return 0.0;
    for(i=0;i<N;i++) {
      norm = v_magnitude[i] / sum;
      if (norm < 0.0) cout << "PQP!\n";
      if (norm != 0.0) 
	h += norm * log(norm);
    }
    return -h;
  }

  Image *view(int maxval=-1) {

    Image * img;
    float smin, smax;
    double *tmp;
    int i,j,k;
    Color c;

    img = new Image(W*5,H);
    tmp = new double[W*H];
    
    // real
    for(i=0;i<W*H;i++) tmp[i] = log(1+v_real[i]);
    smin = smax = tmp[0]; 
    for(i=0;i<W*H;i++) {
      if (tmp[i] < smin) smin = tmp[i];
      if (tmp[i] > smax) smax = tmp[i];
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	c.gray( (int) (255.0*(tmp[k] - smin)/(smax-smin)));
	img->set(W*1+i,j,c);
      }

    // imag
    for(i=0;i<W*H;i++) tmp[i] = log(1+v_imag[i]);
    smin = smax = tmp[0]; 
    for(i=0;i<W*H;i++) {
      if (tmp[i] < smin) smin = tmp[i];
      if (tmp[i] > smax) smax = tmp[i];
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	c.gray( (int) (255.0*(tmp[k] - smin)/(smax-smin)));
	img->set(W*2+i,j,c);
      }

    // mag
    for(i=0;i<W*H;i++) tmp[i] = log(1+v_magnitude[i]);
    smin = smax = tmp[0]; 
    for(i=0;i<W*H;i++) {
      if (tmp[i] < smin) smin = tmp[i];
      if (tmp[i] > smax) smax = tmp[i];
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	c.gray( (int) (255.0*(tmp[k] - smin)/(smax-smin)));
	img->set(W*3+i,j,c);
      }
    // phase
    for(i=0;i<W*H;i++) tmp[i] = v_phase[i];
    smin = smax = tmp[0]; 
    for(i=0;i<W*H;i++) {
      if (tmp[i] < smin) smin = tmp[i];
      if (tmp[i] > smax) smax = tmp[i];
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	c.gray( (int) (255.0*(tmp[k] - smin)/(smax-smin)));
	img->set(W*4+i,j,c);
      }
    // original
    for(i=0;i<W*H;i++) tmp[i] = v_data[i];

    if (maxval >= 0) {
      smin = 0;
      smax = maxval;
    } else {
      smin = smax = tmp[0]; 
      for(i=0;i<W*H;i++) {
	if (tmp[i] < smin) smin = tmp[i];
	if (tmp[i] > smax) smax = tmp[i];
      }
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	c.gray( (int) (255.0*(tmp[k] - smin)/(smax-smin)));
	img->set(W*0+i,j,c);
      }

    delete tmp;
    return img;
  }

  void save(SpectrumMap what, const char *name) {
    float smin, smax;
    double *tmp,val;
    FILE *f;
    int i;
    
    tmp = new double[W*H];
    
    for(i=0;i<W*H;i++) {
      switch(what) {
      case Real:      tmp[i] = log(1+v_real[i]); break;
      case Imaginary: tmp[i] = log(1+v_imag[i]); break;
      case Magnitude: tmp[i] = log(1+v_magnitude[i]); break;
      case Phase:     tmp[i] = v_phase[i]; break;
      case Space:     tmp[i] = v_data[i]; break;
      }
    }
    
    smin = smax = tmp[0];
    
    for(i=0;i<W*H;i++) {
      if (tmp[i] < smin) smin = tmp[i];
      if (tmp[i] > smax) smax = tmp[i];
    }
    
    f = fopen(name,"w");
    if (f==NULL) return;
    fprintf(f,"P5\n%d %d\n255\n",W,H);

    for(i=0;i<W*H;i++) {
      val = 255.0*(tmp[i] - smin)/(smax-smin);
      fputc( (int) val, f );
    }
    
    delete tmp;
    fclose(f);
  }

  void print() {
    int i;
    printf("real = ");
    for(i=0;i<W*H;i++)
      printf("%.2f ",v_real[i]);
    printf("\n");

    printf("imag = ");
    for(i=0;i<W*H;i++)
      printf("%.2f ",v_imag[i]);
    printf("\n");
  }

  
  int W,H;
 private:
  double *v_real,*v_imag,*v_magnitude,*v_phase,*v_data;
  bool allocated;

  void alloc() {
    v_real      = new double[W*H];
    v_imag      = new double[W*H];
    v_magnitude = new double[W*H];
    v_phase     = new double[W*H];
    v_data      = new double[W*H];
    allocated = true;
  }

  void dealloc() {
    if (allocated) {
      delete v_real;
      delete v_imag;
      delete v_magnitude;
      delete v_phase;
      delete v_data;
      allocated = false;
    }
  }

  void fftMagPhase() {
    int i,j,k;
    for(j=0;j<H;j++)
      for(i=0;i<W;i++) {
	k = i + j*W;
	v_magnitude[k] = sqrt( v_real[k]*v_real[k] + v_imag[k]*v_imag[k] );
	v_phase[k]     = atan( v_imag[k] / v_real[k]);
      }
  }

  // output placed in the space() vector
  void invfft2d() {
    int i, j;
    double *r1, *i1;

    if (!allocated) return;

    /* Transform the rows */
    r1 = new double[W];
    i1 = new double[W];

    for (i=0; i<H; i++) {
      for (j=0; j<W; j++) {
	  r1[j] = real(j,i);
	  i1[j] = imag(j,i);
      }
      fft(-1, (long)(W), r1, i1);
      for (j=0; j<W; j++) {
	real(j,i) = r1[j];
	imag(j,i) = i1[j];
      }
    }
    delete r1;
    delete i1;

    /* Transform the cols */
    r1 = new double[H];
    i1 = new double[H];
    
    for (i=0; i<W; i++) {
      for (j=0; j<H; j++) {
	r1[j] = real(i,j);
	i1[j] = imag(i,j);
      }
      fft(-1, (long)(H), r1, i1);
      for (j=0; j<H; j++)
	space(i,j) = r1[j];
    }
    delete r1;
    delete i1;
  }

  void fft2d(float *data, int width, int height) {
    int i, j, NW, NH;
    double *r1, *i1;

    /* Enlarge ncols and nrows to the nearest power of 2 */
    NW = (int)ceil(log(width)/log(2));
    NH = (int)ceil(log(height)/log(2));
    NW = 1 << NW;
    NH = 1 << NH;

    if (allocated && (NW != W || NH != H))
      dealloc();
    W = NW; H = NH;
    if (!allocated)
      alloc();

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	space(i,j) = (i<width && j<height) ? (double) (data[i+j*width]) : 0.0;

    /* Transform the rows */
    r1 = new double[W];
    i1 = new double[W];

    for (i=0; i<H; i++) {
      for (j=0; j<W; j++) {
	if (i<height && j<width)
	  r1[j] = space(j,i);
	else
	  r1[j] = 0.0;
	i1[j] = 0.0;
      }
      fft(1, (long)(W), r1, i1);
      for (j=0; j<W; j++) {
	real(j,i) = r1[j];
	imag(j,i) = i1[j];
      }
    }
    delete r1;
    delete i1;

    /* Transform the cols */
    r1 = new double[H];
    i1 = new double[H];
    
    for (i=0; i<W; i++) {
      for (j=0; j<H; j++) {
	r1[j] = real(i,j);
	i1[j] = imag(i,j);
      }
      fft(1, (long)(H), r1, i1);
      for (j=0; j<H; j++) {
	real(i,j) = r1[j];
	imag(i,j) = i1[j];
      }
    }
    delete r1;
    delete i1;
  }

  void fft(int dir, long nn, double *x, double *y) {
    /*
      This computes an in-place complex-to-complex FFT
      x and y are the real and imaginary arrays of nn points.
      dir =  1 gives forward transform
      dir = -1 gives reverse transform
    */

    int m;
    long i,i1,j,k,i2,l,l1,l2;
    double c1,c2,tx,ty,t1,t2,u1,u2,z;

    m = (int)(log(nn)/log(2)+.00001);
   
    /* Do the bit reversal */
    i2 = nn >> 1;
    j = 0;
    for (i=0;i<nn-1;i++) {
      if (i < j) {
	tx = x[i];
	ty = y[i];
	x[i] = x[j];
	y[i] = y[j];
	x[j] = tx;
	y[j] = ty;
      }
      k = i2;
      while (k <= j) {
	j -= k;
	k >>= 1;
      }
      j += k;
    }

    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
	for (i=j;i<nn;i+=l2) {
	  i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
	}
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1)
	c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
    }
    
    /* Scaling for reverse transform */
    if (dir == -1) {
      for (i=0;i<nn;i++) {
	x[i] /= (double)nn;
	y[i] /= (double)nn;
      }
    }
  }

};

class Patch {

 public:
  Patch(int width, int height=1) {
    unsigned int i;
    W = width;
    H = height;
    N = W*H;
    data = new float[N];
    for(i=0;i<N;i++) data[i] = 0.0;
  }

  Patch(float *v, int width, int height=1) {
    unsigned int i;
    W = width;
    H = height;
    N = W*H;
    data = new float[N];
    for(i=0;i<N;i++) data[i] = v[i];
  }

  Patch(const char *filename) {
    ifstream f;
    char tmp[512];
    unsigned char c;
    unsigned int i;

    data = NULL;
    W=H=N=0;
    f.open(filename);
    if (f.good()) {
      f.getline(tmp,512); if (f.bad()) return;
      if (strcmp(tmp,"P5")) {
	f.close();
	return;
      }
      f.getline(tmp,512); if (f.bad()) return;
      Tokenizer t(tmp," \t\r\n");
      if (t.countTokens()!=2) {
	f.close();
	return;
      }
      W = t.nextInt();
      H = t.nextInt();
      N = W*H;
      data = new float[N];
      f.getline(tmp,512); if (f.bad()) return;
      for(i=0;i<N;i++) {
	f.read((char *) &c,1);
	if (f.bad()) break;
	data[i] = (float) c;
      }
      f.close();
    }
  }

  virtual ~Patch() { delete[] data; }

  void fill(float val) {
    unsigned int i;
    for(i=0;i<N;i++) data[i] = val;
  }

  int saveP5(const char *filename) {
    ofstream o;
    o.open(filename);
    if (!o.good()) return -1;

    float mval, scale=1.0f;
    unsigned char u;
    unsigned int i;

    mval = maximum();
    if (mval > 255.0f)
      scale = 255.0f / mval;
    else if (mval <= 1.2)
      scale = 255.0f;     

    o << "P5\n" << W << ' ' << H << "\n255\n";
    for(i=0;i<N;i++) {
      u = (unsigned char) (data[i] * scale);
      o.write((char *) &u,1);
      if (!o.good()) { o.close(); return -1; }
    }
    o.close();
    return 0;
  }

  void print() {
    unsigned int i,j;
    cout << "Patch =\n";
    for(j=0;j<H;j++) {
      for(i=0;i<W;i++) {
	printf("%5.4f  ",point(i,j));
      }
      cout << endl;
    }
  }

  // extrai um patch normal ao vetor grad, centrado em (x,y,z)
  // mirror=true extrai o simétrico pelo plano Z=D/2.
  template<class T> void extractOblique(Volume<T> *src,
					R3 &grad,
					int x,int y,int z, 
					bool mirror=false) {
    R3 u,v,w;
    int w2,h2,i,j,px,py,pz;

    //grad.print();

    if (mirror) grad.Z = -grad.Z;

    if (fabs(grad.X) < fabs(grad.Y) && fabs(grad.X) < fabs(grad.Z))
      w.set(1.0f,0.0f,0.0f);
    else if (fabs(grad.Y) < fabs(grad.X) && fabs(grad.Y) < fabs(grad.Z))
      w.set(0.0f,1.0f,0.0f);
    else
      w.set(0.0f,0.0f,1.0f);
    
    u = grad.cross(w);
    v = grad.cross(u);
    u.normalize();
    v.normalize();

    //u.print();
    //v.print();

    w2 = W/2;
    h2 = H/2;
    
    if (mirror) z = src->D - z - 1;

    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++) {
	px = (int) (x + (i-w2)*u.X + (j-h2)*v.X);
	py = (int) (y + (i-w2)*u.Y + (j-h2)*v.Y);
	pz = (int) (z + (i-w2)*u.Z + (j-h2)*v.Z);
	if (src->valid(px,py,pz)) {
	  point(i,j) = (float) (src->voxel(px,py,pz));
	  //src->voxel(px,py,pz) = 5000;
	} else
	  point(i,j) = 0.0f;
      }
  }

  template<class T> void extractOblique(Volume<T> *src,
					Volume<int> *gradfield,
					int x,int y,int z, bool
					mirror=false) {
    R3 g;
    gradfield->gradient(x,y,z,g);
    g.usqrt();
    extractOblique(src,g,x,y,z,mirror);
  }

  template<class T> void extract(Volume<T> *src, int x,int y,int z) {
    unsigned int i,j,w2,h2;

    w2 = W/2;
    h2 = H/2;

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (src->valid(x+i-w2,y+j-h2,z))
	  data[i+W*j] = (float) (src->voxel(x+i-w2,y+j-h2,z));
	else
	  data[i+W*j] = 0.0;
  }


  template<class T> void extractXY(Volume<T> *src, int x,int y,int z) {
    return(extract(src,x,y,z));
  }

  template<class T> void extractXZ(Volume<T> *src, int x,int y,int z) {
    unsigned int i,j,w2,h2;

    w2 = W/2;
    h2 = H/2;

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (src->valid(x+i-w2,y,z+j-h2))
	  data[i+W*j] = (float) (src->voxel(x+i-w2,y,z+j-h2));
	else
	  data[i+W*j] = 0.0;
  }

  template<class T> void extractYZ(Volume<T> *src, int x,int y,int z) {
    unsigned int i,j,w2,h2;

    w2 = W/2;
    h2 = H/2;

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (src->valid(x,z+i-w2,y+j-h2))
	  data[i+W*j] = (float) (src->voxel(x,z+i-w2,y+j-h2));
	else
	  data[i+W*j] = 0.0;
  }

  template<class T> void paste(Volume<T> *src, int x,int y,int z) {
    int i,j,w2,h2;

    w2 = W/2;
    h2 = H/2;

    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
	if (src->valid(x+i-w2,y+j-h2,z))
	  src->voxel(x+i-w2,y+j-h2,z) = (T) data[i+W*j];
  }

  float & point(int x,int y) { return(data[x+y*W]); }
  float & point(int i) { return(data[i]); }

  void copyFrom(Patch *src) {
    unsigned int i;
    if (N == src->N)
      for(i=0;i<N;i++)
	data[i] = src->data[i];
    else
      cerr << "** Patch::copyFrom dimension mismatch.\n";
  }

  //! computes the gray-level cooccurrence matrix of src
  //! src patch must be normalized 0..1
  void glcm(Patch *src) {
    unsigned int i,j;
    int a,b,c,d;
    float s;

    for(i=0;i<N;i++) data[i] = 0.0f;

    for(j=0;j<src->H-1;j++)
      for(i=0;i<src->W-1;i++) {
	a = (int) (src->point(i,j) * W);
	b = (int) (src->point(i+1,j) * W);
	c = (int) (src->point(i,j+1) * W);
	d = (int) (src->point(i+1,j+1) * W);
	if (a<0) a=0; else if (a>=(int)W) a=W-1;
	if (b<0) b=0; else if (b>=(int)W) b=W-1;
	if (c<0) c=0; else if (c>=(int)W) c=W-1;
	if (d<0) d=0; else if (d>=(int)W) d=W-1;
	++point(a,b);
	++point(b,a);
	++point(a,c);
	++point(c,a);
	++point(a,d);
	++point(d,a);
	++point(b,c);
	++point(c,b);
      }

    // normalize glcm
    s = sum();
    if (s!=0.0f)
      for(i=0;i<N;i++)
	data[i] /= s;
  }

  //! GLCM: Haralick contrast
  float contrast() {
    int i,j;
    float x=0.0f;
    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
        x += (i-j)*(i-j)*point(i,j);
    x /= (W-1.0f)*(W-1.0f);
    return x;
  }

  //! GLCM: Haralick homogeneity
  float homogeneity() {
    int i,j;
    float x=0.0f;
    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
        x += point(i,j) / (1.0f+abs(i-j));
    return x;
  }

  //! GLCM: Haralick energy
  float energy() {
    int i,j;
    float x=0.0f;
    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
	x += point(i,j) * point(i,j);
    return x;
  }

  //! GLCM: Haralick entropy
  float entropy() {
    int i,j;
    float x=0.0f;
    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
	if (point(i,j)!=0.0f) 
	  x += point(i,j) * log10f(point(i,j));
    return x; 
  }

  //! GLCM: Haralick correlation
  float correlation() {
    unsigned int i,j;
    float mx=0.0f,my=0.0f,sx=0.0f,sy=0.0f,x=0.0f,tmp;

    // mx
    for(i=0;i<W;i++) {
      tmp = 0.0f;
      for(j=0;j<H;j++)
	tmp += point(i,j);
      mx +=  i * tmp;
    }
    
    // my
    for(i=0;i<H;i++) {
      tmp = 0.0f;
      for(j=0;j<W;j++)
	tmp += point(j,i);
      my += i * tmp;
    }

    // sx
    for(i=0;i<W;i++) {
      tmp = 0.0f;
      for(j=0;j<H;j++)
	tmp += point(i,j);
      sx +=  (i-mx)*(i-mx) * tmp;
    }

    // sy
    for(i=0;i<H;i++) {
      tmp = 0.0f;
      for(j=0;j<W;j++)
	tmp += point(j,i);
      sy += (i-my)*(i-my) * tmp;
    }

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	x += i*j*point(i,j) - mx*my;

    if (sx!=0.0f && sy!=0.0f)
      x /= sx * sy;

    /*
      cout << mx << ", ";
      cout << my << ", ";
      cout << sx << ", ";
      cout << sy << ": ";
      cout << x << "\n";
    */
    return x;
  }


  virtual double hfpower() {
    unsigned int i,j,rx,ry,w2,h2;
    double acc = 0.0, sqdist, p;

    Spectrum *s;
    s = fft();

    w2 = W/2;
    h2 = H/2;

    for(i=0;i<W;i++)
      for(j=0;j<H;j++) {
	rx = (i + w2) % W;
	ry = (j + h2) % H;
	p = log(s->magnitude(i,j) + 1);
	sqdist = (rx-w2)*(rx-w2)+(ry-h2)*(ry-h2);
	acc += p*sqdist;
      }
    
    delete s;
    return(acc);
  }

  double aratio(double th) {
    unsigned int i;
    double ca=1.0,cb=1.0;
    for(i=0;i<N;i++)
      if (data[i] < th) ++cb; else ++ca;

    if (ca/cb < 1E-10 || ca/cb > 1E10)
      cout << "ca=" << ca << ", cb=" << cb << ", N=" << N << ", ca+cb=" << (ca+cb) << ", ca/cb=" << (ca/cb) << endl;
    return(ca/cb);
  }

  float lratio(float l1, float l2) {
    unsigned int i,ca=0,cb=0;
    for(i=0;i<N;i++)
      if (data[i] == l1) ++ca; 
      else if (data[i] == l2) ++cb;
    return((ca+1.0)/(cb+1.0));
  }

  void yflip() {
    float t;
    unsigned int i,x,y,ay;

    for(i=0;i<N/2;i++) {
      x = i%W;
      y = i/W;
      ay = H - y - 1;
      t = data[i];
      data[i] = data[x+ay*W];
      data[x+ay*W] = t;
    }
  }  

  float absdiff(Patch *b) {
    float s = 0.0;
    unsigned int i;

    for(i=0;i<N;i++)
      s += fabs(data[i] - b->data[i]);

    return s;
  }

  float sum() {
    float s=0.0f;
    unsigned int i;
    for(i=0;i<N;i++) s+=data[i];
    return s;
  }

  float mean() {
    return(sum()/N);
  }
  
  float stdev(const float vmean) {
    unsigned int i;
    float sum=0.0;
    for(i=0;i<N;i++) sum += (data[i]-vmean)*(data[i]-vmean);
    sum = sqrt(sum/N);
    return sum;
  }

  float variance(const float vmean) {
    unsigned int i;
    float sum=0.0;
    for(i=0;i<N;i++) sum += (data[i]-vmean)*(data[i]-vmean);
    sum /= N;
    return sum;
  }

  float skewness(const float vmean) {
    unsigned int i;
    float sum=0.0, d;

    d = variance(vmean);
    if (d == 0.0) return 0.0;
    for(i=0;i<N;i++) 
      sum += (data[i]-vmean)*(data[i]-vmean)*(data[i]-vmean);
    sum /= (N * sqrt(d*d*d));
    return sum;
  }

  float kurtosis(const float vmean) {
    unsigned int i;
    float sum=0.0, d, t;

    d = variance(vmean);
    if (d == 0.0) return 0.0;
    for(i=0;i<N;i++) {
      t = (data[i]-vmean)*(data[i]-vmean);
      sum += t*t;
    }
    sum /= (N * d*d);
    sum -= 3.0;
    return sum;   
  }

  float amplitude() {
    unsigned int i;
    float vmin,vmax;
    vmin=vmax=data[0];
    for(i=1;i<N;i++) {
      if (data[0] < vmin) vmin = data[i];
      if (data[1] > vmax) vmax = data[i];
    }
    return(vmax-vmin);
  }

  float minimum() {
    unsigned int i;
    float vmin;
    vmin = data[0];
    for(i=1;i<N;i++) if (data[i] < vmin) vmin = data[i];
    return vmin;
  }

  float maximum() {
    unsigned int i;
    float vmax;
    vmax = data[0];
    for(i=1;i<N;i++) if (data[i] > vmax) vmax = data[i];
    return vmax;
  }

  // low-to-high freq ratio
  float lth(unsigned int bands) {
    Spectrum *s;
    float sumlow, sumhigh, nlow, nhigh;
    unsigned int i,j;

    s = new Spectrum();
    s->compute(data,W,H);

    sumlow = sumhigh = 0.0;
    nlow = nhigh = 0.0;

    for(j=0;j<H;j++)
      for(i=0;i<W;i++)
	if (i<bands && j<bands) {
	  sumlow += s->magnitude(i,j);
	  nlow++;
	} else {
	  sumhigh += s->magnitude(i,j);
	  nhigh++;
	}
    sumlow /= nlow;
    sumhigh /= nhigh;
    delete s;
    if (sumhigh == 0.0) return 0.0;
    return(sumlow/sumhigh);
  }

  Spectrum *fft() {
    Spectrum *s;
    s = new Spectrum();
    s->compute(data,W,H);
    return s;
  }

  // high-frequency power residue
  float hfres(float t1, float t2) {
    Spectrum *s;
    float acc = 0.0;
    unsigned int i;
    s = new Spectrum();
    s->compute(data,W,H);
    s->lowpass(t1,t2);
    s->inverse();
    
    for(i=0;i<N;i++)
      acc += fabs( data[i] - s->space(i) );
    acc /= N;
    delete s;
    return acc;
  }

  // high-frequency power residue
  float hfres(Spectrum *fft, float t1, float t2) {
    Spectrum *s;
    float acc = 0.0;
    unsigned int i;
    s = new Spectrum(fft);
    s->lowpass(t1,t2);
    s->inverse();    
    for(i=0;i<N;i++)
      acc += fabs( data[i] - s->space(i) );
    acc /= N;
    delete s;
    return acc;
  }

  float lfres(float t1, float t2) {
    Spectrum *s;
    float acc = 0.0;
    unsigned int i;
    s = new Spectrum();
    s->compute(data,W,H);
    s->highpass(t1,t2);
    s->inverse();
    
    for(i=0;i<N;i++)
      acc += fabs( data[i] - s->space(i) );
    acc /= N;
    delete s;
    return acc;
  }

  void discretize(int levels) {
    float vmax, vmin, x;
    unsigned int i;
    vmax = maximum();
    vmin = minimum();

    if ( (vmax - vmin) == 0.0) return;

    for(i=0;i<N;i++) {
      x = (data[i]-vmin) / (vmax-vmin);
      x *= levels;
      if (x == levels) --x;
      x = (float) ((int) x);
      x *= vmax / levels;
      data[i] = x;
    }
  }

  float * getData() { return data; }

  Spectrum *getSpectrum() {
    Spectrum *s;
    s = new Spectrum();
    s->compute(data,W,H);
    return s;
  }

  double gaussian(const double x, const double y, const double sigma) const {
    return ( exp(-(x*x + y*y)/(2.0*sigma*sigma)) / (2.0*M_PI*sigma*sigma) );
  }

  void gaussianKernel(double sigma) {
    int i,j,cx,cy;
    double sum;
    cx = W/2; cy = H/2;
    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++)
	point(i,j) = gaussian( (double)(i-cx), (double)(j-cy), sigma );
    for(i=0, sum=0.0;i<(int)N;i++)
      sum += point(i);
    if (sum!=0.0)
      for(i=0;i<(int)N;i++)
	point(i) /= sum;
  }

  void convolve(Patch *kernel, Patch *scrap=NULL) {
    int i,j,m,n,kx2,ky2,x,y;
    float tmp;
    Patch *res;

    if (scrap != NULL)
      res = scrap;
    else
      res = new Patch(W,H);
    kx2 = kernel->W / 2;
    ky2 = kernel->H / 2;

    for(j=0;j<(int)H;j++)
      for(i=0;i<(int)W;i++) {
        tmp = 0.0f;
        for(n=0;n<(int)(kernel->H);n++)
          for(m=0;m<(int)(kernel->W);m++) {
            x = i - kx2 + m;
            y = j - ky2 + n;
            if (x >= 0 && x < (int)W && y >= 0 && y < (int)H)
              tmp += (kernel->point(m,n)) * (this->point(x,y));
          }
        res->point(i,j) = tmp;
      }

    for(i=0;i<(int)N;i++)
      data[i] = res->data[i];
    if (scrap==NULL)
      delete res;
  }
    
  int countBelow(float value) {
    unsigned int i;
    int x=0;
    for(i=0;i<N;i++)
      if (data[i] < value) ++x;
    return x;
  }

  float gblur(Patch &convolved) {
    float sum;
    unsigned int i;

    if (convolved.W != W || convolved.H != H) {
      cerr << "** Patch::gblur(...): tmp/tmp2 dimension mismatch.\n";
      return 0.0f;
    }

    sum = 0.0;
    for(i=0;i<N;i++)
      sum += fabsf(convolved.data[i] - data[i]);
    sum /= (W * H);
    
    return(sum);
  }

  float gblur(Patch &kernel, Patch &tmp, Patch &tmp2) {
    float sum;
    unsigned int i;

    if (tmp.W != W || tmp.H != H || tmp2.W != W || tmp2.H != H) {
      cerr << "** Patch::gblur(...): tmp/tmp2 dimension mismatch.\n";
      return 0.0f;
    }

    tmp.copyFrom(this);
    tmp.convolve(&kernel,&tmp2);

    sum = 0.0;
    for(i=0;i<N;i++)
      sum += fabsf(tmp.data[i] - data[i]);
    sum /= (W * H);
    
    return(sum);
  }

  float gblur(int kw, int kh, double sigma) {
    Patch k(kw,kh);
    Patch b(W,H);
    unsigned int i;
    float norm, sum; 

    k.gaussianKernel(sigma);
    b.copyFrom(this);
    b.convolve(&k,NULL);

    norm = W * H;
    sum = 0.0;
    for(i=0;i<N;i++)
      sum += fabsf(b.data[i] - data[i]);
    sum /= norm;
    
    return(sum);
  }

  unsigned int W,H,N;
 protected:
  float *data;


};

class MagPatch : public Patch {

 public:
  MagPatch(int width,int height) : Patch(width,height) {
    int fftw,ffth;
    fftw = (int) rint(log(width)/log(2));
    ffth = (int) rint(log(height)/log(2));
    fftw = 1 << fftw;
    ffth = 1 << ffth;
    if (fftw!=width || ffth!=height) {
      cerr << "(" << fftw << "," << ffth << ")\n";
      cerr << "** fatal: class MagPatch requires powers of two as dimensions. bye.\n\n";
      exit(77);
    }
    
    ffta = new double[W*H];
    fftb = new double[W*H];
    rowr = new double[W];
    rowi = new double[W];
    colr = new double[H];
    coli = new double[H];
  }

  virtual ~MagPatch() {
    delete ffta;
    delete fftb;
    delete rowr;
    delete colr;
    delete rowi;
    delete coli;
  }

  virtual double hfpower() {
    unsigned int i,j,a,rx,ry,w2,h2;
    double acc = 0.0, p;

    fft2d();

    w2 = W/2;
    h2 = H/2;

    for(i=0;i<W;i++)
      for(j=0;j<H;j++) {
	rx = (i + w2) % W;
	ry = (j + h2) % H;
	a = i+j*W;
	p = log( sqrt(ffta[a]*ffta[a] + fftb[a]*fftb[a]) + 1.0 );
	acc += p * ((rx-w2)*(rx-w2)+(ry-h2)*(ry-h2));
      }
    return(acc);
  }


 private:
  double *ffta,*fftb,*rowr,*rowi,*colr,*coli;

  void fft2d() {
    unsigned int i,j;
    // transform the rows
    for(i=0;i<H;i++) {
      for(j=0;j<W;j++) {
	rowr[j] = data[j+W*i];
	rowi[j] = 0.0;
      }
      fft1d(rowr,rowi,W);
      for(j=0;j<W;j++) {
	ffta[j+i*W] = rowr[j];
	fftb[j+i*W] = rowi[j];
      }
    }
    // transform the columns
    for(i=0;i<W;i++) {
      for(j=0;j<H;j++) {
	colr[j] = ffta[i+j*W];
	coli[j] = fftb[i+j*W];
      }
      fft1d(colr,coli,H);
      for(j=0;j<H;j++) {
	ffta[i+j*W] = colr[j];
	fftb[i+j*W] = coli[j];
      }
    }
  }

  void fft1d(double *x, double *y,unsigned int nn) {
    unsigned int m,i,i1,j,k,i2,l,l1,l2;
    double c1,c2,tx,ty,t1,t2,u1,u2,z;
    
    m = (unsigned int)(log(nn)/log(2)+.00001);
    
    /* Do the bit reversal */
    i2 = nn >> 1;
    j = 0;
    for (i=0;i<nn-1;i++) {
      if (i < j) {
	tx = x[i];
	ty = y[i];
	x[i] = x[j];
	y[i] = y[j];
	x[j] = tx;
	y[j] = ty;
      }
      k = i2;
      while (k <= j) {
	j -= k;
	k >>= 1;
      }
      j += k;
    }
    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0;
      u2 = 0.0;
      for (j=0;j<l1;j++) {
	for (i=j;i<nn;i+=l2) {
	  i1 = i + l1;
	  t1 = u1 * x[i1] - u2 * y[i1];
	  t2 = u1 * y[i1] + u2 * x[i1];
	  x[i1] = x[i] - t1;
	  y[i1] = y[i] - t2;
	  x[i] += t1;
	  y[i] += t2;
	}
	z =  u1 * c1 - u2 * c2;
	u2 = u1 * c2 + u2 * c1;
	u1 = z;
      }
      c2 = -sqrt((1.0 - c1) / 2.0); /* forward-only */
      c1 = sqrt((1.0 + c1) / 2.0);
    }
  }

};

// co-occurrence
class COMatrix {
 public:
  COMatrix(int sz) {
    size = sz;
    N = size*size;
    data = new int[N];
    clear();
  }

  ~COMatrix() {
    delete[] data;
  }

  void clear() {
    int i;
    for(i=0;i<N;i++)
      data[i] = 0;  
  }

  void extract(Patch *src) {
    double srcmax;
    unsigned int i,j,m,n;
    int k;
    static int vx[8] = {-1,  0,  1, 1, 1, 0, -1, -1};
    static int vy[8] = {-1, -1, -1, 0, 1, 1,  1,  0};
    srcmax = 1.0 + src->maximum();
    
    for(j=0;j<src->H;j++)
      for(i=0;i<src->W;i++) 
	for(k=0;k<8;k++) {
	  m = i+vx[k];
	  n = j+vy[k];
	  if (m<0 || n<0 || m>=src->W || n>=src->H) continue;
	  increment( (int) (size * src->point(i,j) / srcmax),
		     (int) (size * src->point(m,n) / srcmax) );
	}
  }

  void increment(int i,int j) {
    if (i < 0) i = 0;
    if (i >= size) i = size-1;
    if (j < 0) j = 0;
    if (j >= size) j = size-1;

    data[i+j*size]++;
  }

  float energy() {
    int i;
    float x=0.0;
    for(i=0;i<N;i++)
      x += data[i]*data[i];
    return x;
  }

  float entropy() {
    int i;
    float x=0.0;
    for(i=0;i<N;i++)
      x += log(data[i])*data[i];
    return x;
  }

  float contrast() {
    int i,j;
    float x=0.0;
    for(i=0;i<size;i++)
      for(j=0;j<size;j++)
	x += (i-j)*(i-j)*data[i+j*size];
    return x;
  }

  float homogeneity() {
    int i,j;
    float x=0.0;
    for(i=0;i<size;i++)
      for(j=0;j<size;j++)
	x += ((float)(data[i+j*size])) / (1.0+abs(i-j));
    return x;
  }

 private:
      int *data,size,N;
};


class KNN {
 public:
  KNN() { fvsize=0; bias=NULL; K=1; knn = new int[1]; norm=NULL; }

  virtual ~KNN() {
    unsigned int i;
    for(i=0;i<data.size();i++)
      delete(data[i]);
    data.clear();
    if (bias!=NULL)
      delete bias;
    if (norm!=NULL)
      delete norm;
    delete knn;
  }

  // sz = 1 + actual fvsize (class is part of the vector)
  void setFeatureVectorSize(int sz) { 
    fvsize = sz; 
    bias = new float[fvsize];
    for(int i=0;i<fvsize;i++)
      bias[i] = 1.0f;
  }

  void setBias(int klass, float biasvalue) { bias[klass] = biasvalue; }

  void setK(int k) { K=k; delete knn; knn = new int[K]; }

  void addFeatureVector(float *fv) {
    float *myfv;
    int i;
    myfv = new float[fvsize];
    for(i=0;i<fvsize;i++) myfv[i] = fv[i];
    data.push_back(myfv);
  }

  void normalize() {
    int i,j;
    if (norm != NULL)
      delete norm;
    norm = new float[fvsize];

    for(i=0;i<fvsize;i++)
      norm[i] = data[0][i];

    for(i=1;i<(int)data.size();i++)
      for(j=0;j<fvsize;j++)
	if (fabs(data[i][j]) > norm[j])
	  norm[j] = fabs(data[i][j]);

    for(i=0;i<fvsize;i++)
      if (norm[i] == 0.0f)
	norm[i] = 1.0f;

    for(i=1;i<fvsize;i++) cout << "norm(" << i << ")=" << norm[i] << ",";
    cout << endl;

    for(i=0;i<(int)data.size();i++)
      for(j=1;j<fvsize;j++)
	data[i][j] /= norm[j];
  }

  int classify(float *fv) {
    int i,j,mc,mk,c,k;
    for(i=0;i<K;i++)
      knn[i] = closest(fv, knn, i);

    if (K==1)
      return((int)(data[knn[0]][0]));

    for(i=0;i<K;i++) knn[i] = (int) data[knn[i]][0];
    mc = 0;
    mk = -1;
    for(i=0;i<K;i++) {
      c = 1;
      k = knn[i];
      for(j=i+1;j<K;j++)
	if (knn[j] == k)
	  ++c;
      if (c > mc) {
	mc = c;
	mk = k;
      }
    }
    return(mk);
  }

 private:
  int K,*knn;
  int fvsize; // feature vector size = 1 (class) + (fvsize-1) features
  float *bias, *norm;
  vector<float *> data;

  int closest(float *fv, int *exc, int nexc) {
    float bd=0.0,d;
    int ibd=-1;
    unsigned int i;
    
    sort(exc, exc+nexc);

    for(i=0;i<data.size();i++) {
      if (binary_search(exc,exc+nexc,(int)i)) continue;
      if (ibd < 0) {
	ibd = i;
	bd = distance(fv,data[i]);
      } else {
	d = distance(fv,data[i]);
	if (d < bd) {
	  bd = d;
	  ibd = i;
	}
      }
    }
    return ibd;
  }

  float distance(float *a, float *b) {
    int i;
    float acc = 0.0f, bi;
    for(i=1;i<fvsize;i++)
      acc += (a[i]/norm[i]-b[i])*(a[i]/norm[i]-b[i]);
    bi = bias[(int)(b[0])];
    return(acc * bi * bi);
  }

};

#endif
