/*

    SpineSeg
    http://www.liv.ic.unicamp.br/~bergo/spineseg
    Copyright (C) 2009-2011 Felipe Paulo Guazzi Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

// classes here: R3 T4

#ifndef GEOMETRY_H
#define GEOMETRY_H 1

#include <stdio.h>
#include <math.h>
#include <iostream>

using namespace std;

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
  R3 operator+(const R3 &b) { 
    R3 t; t.X = X+b.X; t.Y = Y+b.Y; t.Z = Z+b.Z; return(t); }

  //! vector subtraction
  R3 operator-(const R3 &b) { 
    R3 t; t.X = X-b.X; t.Y = Y-b.Y; t.Z = Z-b.Z; return(t); }

  //! vector multiplication by a scalar
  R3 operator*(const float &b) { 
    R3 t; t.X = X*b; t.Y = Y*b; t.Z = Z*b; return(t); }

  //! vector division by a scalar
  R3 operator/(const float &b) { 
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

#endif

