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

// classes here: array<T> EllipseFit

#ifndef ELLIPSEFIT_H
#define ELLIPSEFIT_H 1

#include <stdio.h>
#include <math.h>
#include <vector>

using namespace std;

template <class T> class array {
 public:
  int rows,cols,n;
  T *value;

  array(int r, int c) {
    rows = r;
    cols = c;
    n = rows * cols;
    value = new T[r*c];
    zero();
  }

  ~array() {
    delete value;
  }

  void print() {
    int i,j;
    printf("array %dx%d:\n",rows,cols);
    for(i=0;i<rows;i++) {
      for(j=0;j<cols;j++)
	printf("   %.10g",e(i,j));
      printf("\n");
    }
    printf("---\n");
  }

  void print(const char *s) {
    int i,j;
    printf("array %s %dx%d:\n",s,rows,cols);
    for(i=0;i<rows;i++) {
      for(j=0;j<cols;j++)
	printf("   %.10g",e(i,j));
      printf("\n");
    }
    printf("---\n");
  }

  void zero() {
    int i;
    for(i=0;i<n;i++) value[i] = 0;
  }

  void fill(const T a) {
    int i;
    for(i=0;i<n;i++) value[i] = a;
  }
  
  // element (index start at 0)
  T &e(int r,int c) {
    return(value[r*cols+c]);
  }

  // math element (index start at 1)
  T &me(int r,int c) {
    return(value[(r-1)*cols+(c-1)]);
  }

  // element (index start at 0)
  T ce(int r,int c) const {
    return(value[r*cols+c]);
  }

  // math element (index start at 1)
  T cme(int r,int c) const {
    return(value[(r-1)*cols+(c-1)]);
  }

  void identity() {
    int i,j;
    for(i=0;i<cols;i++)
      for(j=0;j<rows;j++)
	e(j,i) = (j==i) ? 1 : 0;
  }

  array<T> &operator=(const array<T> &a) {
    int ec, er, i, j;
    ec = rows < a.rows ? rows : a.rows;
    er = cols < a.cols ? cols : a.cols;
    for(i=0;i<ec;i++)
      for(j=0;j<er;j++)
	e(j,i) = a.ce(j,i);
    return(*this);
  }

  void transpose(array<T> & r) {
    int i,j;
    if (r.cols != rows || r.rows != cols) return;
    for(i=0;i<cols;i++)
      for(j=0;j<rows;j++)
	r.e(i,j) = e(j,i);
  }

  // r := this x b
  void multiply(const array<T> &b, array<T> &r) {
    int i,j,k;
    r.zero();
    if (cols != b.rows || r.rows != rows || r.cols != b.cols)
      return;
    for(i=0;i<rows;i++)
      for(j=0;j<b.cols;j++)
	for(k=0;k<cols;k++)
	  r.e(i,j) += e(i,k) * b.ce(k,j);
  }

  // cholesky decomposition, r := lower triangular such that L * L' = this
  void cholesky(array<T> &r) {
    int i,j,k;
    T sum, *p;

    r.zero();
    if (cols!=rows) return;
    if (r.cols != cols || r.rows != rows) return;

    p = new T[cols+1];
    for(i=0;i<cols+1;i++) p[i] = 0;

    for(i=1;i<=cols;i++) {
      for(j=i;j<=cols;j++) {
	for(sum=me(i,j),k=i-1;k>=1;k--)
	  sum -= me(i,k) * me(j,k);
	if (i==j) {
	  if (sum <= 0) {

	  } else 
	    p[i] = (T) sqrt(sum);
	} else {
	  me(j,i) = sum / p[i];
	}
      }
    }
    for(i=1;i<=cols;i++)
      for(j=i;j<=cols;j++)
	if (i==j)
	  r.me(i,i) = p[i];
	else {
	  r.me(j,i) = me(j,i);
	  r.me(i,j) = 0;
	}
    delete p;
  }

  // inverts matrix by Gauss-Jordan. returns 0 on success.
  int invert(array<T> &r) {
    int k,i,j,p,q,npivot;
    T mult, D, temp, maxpivot;
    double eps = 10e-20;

    r.zero();
    if (rows!=cols) return -2;

    array<T> A(rows,2*rows+1), B(rows,rows+1);

    for(k=1;k<=rows;k++)
      for(j=1;j<=rows;j++)
	B.me(k,j) = me(k,j);

    for(k=1;k<=rows;k++) {
      for(j=1;j<=rows+1;j++)
	A.me(k,j) = B.me(k,j);
      for(j=rows+2;j<=2*rows+1;j++)
	A.me(k,j) = 0;
      A.me(k,k-1+rows+2) = 1;
    }

    for(k=1;k<=rows;k++) {
      maxpivot = fabs((double) A.me(k,k));
      npivot = k;
      for(i=k;i<=rows;i++)
	if (maxpivot<fabs((double) A.me(i,k))) {
	  maxpivot = fabs((double) A.me(i,k));
	  npivot = i;
	}
      if (maxpivot >= eps) {
	if (npivot!=k)
	  for(j=k;j<=2*rows+1;j++) {
	    temp = A.me(npivot,j);
	    A.me(npivot,j) = A.me(k,j);
	    A.me(k,j) = temp;
	  }
	D = A.me(k,k);
	for(j=2*rows+1;j>=k;j--)
	  A.me(k,j) /= D;
	for(i=1;i<=rows;i++) {
	  if (i!=k) {
	    mult = A.me(i,k);
	    for(j=2*rows+1;j>=k;j--)
	      A.me(i,j) -= mult*A.me(k,j);
	  }
	}
      } else
	return -1; // singular matrix
    }
    for(k=1,p=1;k<=rows;k++,p++)
      for(j=rows+2,q=1;j<=2*rows+1;j++,q++)
	r.me(p,q) = A.me(k,j);
    return 0;
  }

 private:
  void jacobi_rotate(int i, int j, int k, int l, T tau, T s) {
    T g,h;
    g = me(i,j);
    h = me(k,l);
    me(i,j) = g-s*(h+g*tau);
    me(k,l) = h+s*(g-h*tau);
  }

 public:
  void jacobi(array<T> & eval, array<T> & evec) {
    int j,iq,ip,i,nrot=0;
    T tresh,theta,tau,t,sm,s,h,g,c;

    T *b = new T[rows+1];
    T *z = new T[rows+1];

    evec.identity();
  
    for(ip=1;ip<=rows;ip++) {
      b[ip] = eval.me(ip,1) = me(ip,ip);
      z[ip] = 0;
    }

    for(i=1;i<=50;i++) {
      sm = 0;
      for(ip=1;ip<=rows-1;ip++)
	for(iq=ip+1;iq<=rows;iq++)
	  sm += fabs( (double) me(ip,iq) );
      if (sm == 0) {
	delete z;
	delete b;
	return;
      }
      tresh = (i<4) ? 0.2*sm/(rows*rows) : 0;

      for(ip=1;ip<=rows-1;ip++) {
	for(iq=ip+1;iq<=rows;iq++) {
	  g = 100.0*fabs(me(ip,iq));
	  if (i>4 && 
	      fabs(eval.me(ip,1))+g == fabs(eval.me(ip,1)) &&
	      fabs(eval.me(iq,1))+g == fabs(eval.me(iq,1)) )
	    me(ip,iq) = 0;
	  else if (fabs(me(ip,iq)) > tresh) {
	    h = eval.me(iq,1) - eval.me(ip,1);
	    if (fabs(h)+g == fabs(h))
	      t = me(ip,iq)/h;
	    else {
	      theta = 0.5*h/me(ip,iq);
	      t = 1.0/(fabs(theta)+sqrt(1.0+theta*theta));
	      if (theta < 0) t = -t;
	    }
	    c=1.0/sqrt(1+t*t);
	    s=t*c;
	    tau=s/(1.0+c);
	    h=t*me(ip,iq);
	    z[ip] -= h;
	    z[iq] += h;
	    eval.me(ip,1) -= h;
	    eval.me(iq,1) += h;
	    me(ip,iq) = 0;
	    for(j=1;j<=ip-1;j++)    jacobi_rotate(j,ip,j,iq,tau,s);
	    for(j=ip+1;j<=iq-1;j++) jacobi_rotate(ip,j,j,iq,tau,s);
	    for(j=iq+1;j<=rows;j++) jacobi_rotate(ip,j,iq,j,tau,s);
	    for(j=1;j<=rows;j++)    evec.jacobi_rotate(j,ip,j,iq,tau,s);
	    ++nrot;
	  }
	}
      }
      for(ip=1;ip<=rows;ip++) {
	b[ip] += z[ip];
	eval.me(ip,1) = b[ip];
	z[ip] = 0;
      }
    }
    delete b;
    delete z;
  }

};

class EllipseFit {

 private:
  vector<double> vx,vy;

 public:
  // Ax^2 + Bxy + Cy^2 + Dx + Ey + F = 0, solution[0..5] == {A,B,C,D,E,F}
  double solution[6];
  bool fitted;

  EllipseFit() {
    fitted = false;
  }

  void clear() {
    vx.clear();
    vy.clear();
    fitted = false;
  }
  
  void append(double x, double y) {
    vx.push_back(x);
    vy.push_back(y);
  }


  // http://mathworld.wolfram.com/Ellipse.html
  double major_semiaxis() {
    double r,a,b,c,d,f,g;
    a = solution[0];
    b = solution[1] / 2.0;
    c = solution[2];
    d = solution[3] / 2.0;
    f = solution[4] / 2.0;
    g = solution[5];
    r = sqrt( (2*(a*f*f+c*d*d+g*b*b-2*b*d*f-a*c*g)) / 
	      ( (b*b-a*c) * (sqrt((a-c)*(a-c)+4*b*b) - (a+c) )) );
    return r;
  }

  double minor_semiaxis() {
    double r,a,b,c,d,f,g;
    a = solution[0];
    b = solution[1] / 2.0;
    c = solution[2];
    d = solution[3] / 2.0;
    f = solution[4] / 2.0;
    g = solution[5];
    r = sqrt( (2*(a*f*f+c*d*d+g*b*b-2*b*d*f-a*c*g)) / 
	      ( (b*b-a*c) * (-sqrt((a-c)*(a-c)+4*b*b) - (a+c) )) );
    return r;
  }

  double eccentricity() {
    double a,b;
    a = major_semiaxis();
    b = minor_semiaxis();
    return(sqrt(1-((b*b)/(a*a))));
  }

  double flatness() {
    return(major_semiaxis() / minor_semiaxis());
  }

  double perimeter() {
    double a,b;
    a = major_semiaxis();
    b = minor_semiaxis();
    return( M_PI * sqrt( 2 * (a*a + b*b)) );
  }

  double area() {
    double a,b;
    a = major_semiaxis();
    b = minor_semiaxis();
    return(M_PI * a * b);
  }

  void plot(array<double> & pts) {
    int npts = pts.rows / 2;
    int i,j;

    double Ao,Ax,Ay,Axx,Ayy,Axy,theta,kk;
    double *lambda;

    pts.zero();
    if (!fitted) return;

    Axx = solution[0];
    Axy = solution[1];
    Ayy = solution[2];
    Ax  = solution[3];
    Ay  = solution[4];
    Ao  = solution[5];

    array<double> A(2,2), Ai(2,2), b(2,1), u(2,npts), Aib(2,1), r1(1,1);
    array<double> bt(1,2), Aiu(2,npts), uAiu(2,npts), L(2,npts), B(2,npts);
    array<double> ss1(2,npts), ss2(2,npts), Xpos(2,npts), Xneg(2,npts);
    
    A.me(1,1) = Axx;    A.me(1,2) = Axy/2;
    A.me(2,1) = Axy/2;  A.me(2,2) = Ayy;
    b.me(1,1) = Ax;     b.me(2,1) = Ay;

    for(i=1, theta=0.0; i<=npts; i++, theta+=(M_PI/npts)) {
      u.me(1,i) = cos(theta);
      u.me(2,i) = sin(theta);
    }

    A.invert(Ai);
    Ai.multiply(b,Aib);
    b.transpose(bt);
    bt.multiply(Aib,r1);

    r1.me(1,1) -= 4*Ao;
    Ai.multiply(u,Aiu);

    for(i=1;i<=2;i++)
      for(j=1;j<=npts;j++)
	uAiu.me(i,j) = u.me(i,j) * Aiu.me(i,j);

    lambda=new double[npts+1];
    for(j=1;j<=npts;j++) {
      if ( (kk=(r1.me(1,1) / (uAiu.me(1,j)+uAiu.me(2,j)))) >= 0.0 )
	lambda[j] = sqrt(kk);
      else
	lambda[j] = -1.0;
    }

    for(j=1;j<=npts;j++)
      L.me(1,j) = L.me(2,j) = lambda[j];
    for(j=1;j<=npts;j++) {
      B.me(1,j) = b.me(1,1);
      B.me(2,j) = b.me(2,1);
    }

    for(j=1;j<=npts;j++) {
      ss1.me(1,j) = 0.5 * ( L.me(1,j) * u.me(1,j) - B.me(1,j) );
      ss1.me(2,j) = 0.5 * ( L.me(2,j) * u.me(2,j) - B.me(2,j) );
      ss2.me(1,j) = 0.5 * ( -L.me(1,j) * u.me(1,j) - B.me(1,j) );
      ss2.me(2,j) = 0.5 * ( -L.me(2,j) * u.me(2,j) - B.me(2,j) );
    }

    Ai.multiply(ss1,Xpos);
    Ai.multiply(ss2,Xneg);

    for(j=1;j<=npts;j++) {
      if (lambda[j]==-1.0) {
	pts.me(j,1) = -1;
	pts.me(j,2) = -1;
	pts.me(j+npts,1) = -1;
	pts.me(j+npts,2) = -1;
      } else {
	pts.me(j,1) = Xpos.me(1,j);
	pts.me(j,2) = Xpos.me(2,j);
	pts.me(j+npts,1) = Xneg.me(1,j);
	pts.me(j+npts,2) = Xneg.me(2,j);
      }
    }

    delete lambda;
  }

  double distance(double x, double y) {
    double a,b,c,delta,y0,y1,d0,d1;

    a = solution[2]; // C
    b = solution[4] + solution[1]*x; // E+Bx
    c = solution[0]*x*x + solution[3]*x + solution[5]; // Ax^2+Dx+F

    delta = b*b - 4*a*c;
    if (delta < 0) return 10000.0;
    y0 = (-b + sqrt(b*b-4*a*c)) / 2*a;
    y1 = (-b - sqrt(b*b-4*a*c)) / 2*a;

    d0 = fabs(y0-y);
    d1 = fabs(y1-y);
    if (d0 < d1) return d0; else return d1;
  }
  
  // AW Fitzgibbon, 1999
  void fit() {
    int np, i, j, solind;
    double mod;
    array<double> S(6,6), Const(6,6), tmp(6,6), L(6,6), C(6,6);
    array<double> invL(6,6), sol(6,6), V(6,6), d(6,1);

    fitted = false;

    // constraints
    Const.me(1,3) = -2.0;
    Const.me(2,2) =  1.0;
    Const.me(3,1) = -2.0;    

    np = vx.size();
    if (np<6) return;

    array<double> D(np,6);
    
    // design matrix
    for(i=1;i<=np;i++) {
      //printf("(%d) (%f,%f)\n",i,vx[i-1],vy[i-1]);

      D.me(i,1) = vx[i-1] * vx[i-1];
      D.me(i,2) = vx[i-1] * vy[i-1];
      D.me(i,3) = vy[i-1] * vy[i-1];
      D.me(i,4) = vx[i-1];
      D.me(i,5) = vy[i-1];
      D.me(i,6) = 1.0;
    }

    //Const.print("Const");

    // scatter mattrix
    array<double> Dt(6,np);
    D.transpose(Dt);
    Dt.multiply(D,S);
    //S.print("Scatter");

    S.cholesky(L);
    //L.print("Cholesky");
    L.invert(invL);
    //invL.print("inverse");

    array<double> invLt(6,6);
    invL.transpose(invLt);
    Const.multiply(invLt,tmp);
    invL.multiply(tmp,C);
    //C.print("The C matrix");
    C.jacobi(d,V);
    //V.print("eigenvectors");
    //d.print("eigenvalues");

    invLt.multiply(V,sol);
    //sol.print("GEV solution unnormalized");
    
    // normalize solution
    for(j=1;j<=6;j++) {
      mod = 0.0;
      for(i=1;i<=6;i++)
	mod += sol.me(i,j) * sol.me(i,j);
      for(i=1;i<=6;i++)
	sol.me(i,j) /= sqrt(mod);
    }

    //sol.print("GEV solution");

    double zt    = 10e-20;
    solind = 1;

    for(i=1;i<=6;i++)
      if (d.me(i,1)<0 && fabs(d.me(i,1))>zt)
	solind = i;

    //printf("solind=%d\n",solind);

    // fetch the solution (eigenvector associated to the single negative eigenvalue)
    for(j=1;j<=6;j++)
      solution[j-1] = sol.me(j,solind);
    fitted = true;

    /*
    printf("npts=%d A=%.10g B=%.10g C=%.10g D=%.10g E=%.10g F=%.10g\n",
	   (int) (vx.size()),solution[0],
	   solution[1],
	   solution[2],
	   solution[3],
	   solution[4],
	   solution[5]);
    */
  }

};

#endif
