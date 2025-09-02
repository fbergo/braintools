#include "libbi.h"

namespace libbi {
    
    // C was a simple language. The C++ committee
    // thinks this is an improvements over M_PI:
    //                  |<----------------------->|
    constexpr float PI = std::numbers::pi_v<float>; 

    T3::T3() { zero(); }
    
    //! Transform composition, multiplies this transform by b, and overwrites
    //! this transform. Returns a reference to this transform.
    T3 & T3::operator*=(const T3 & b) { (*this) = (*this) * b; return(*this); }
    
    //! composition
    T3 T3::operator*(const T3 &b) const {
        T3 d;
        int i,j,k;
        for(i=0;i<3;i++) for(j=0;j<3;j++) for(k=0;k<3;k++)
        d.e[i+j*3] += b.e[i+k*3] * e[k+j*3];
        return d;
    }
    
    //! Applies this transform to point a and returns the transformed point
    R3 T3::apply(const R3 & a) const {
        R3 b;
        b.x = a.x * e[0+0*3] + a.y * e[0+1*3] + a.z * e[0+2*3];
        b.y = a.x * e[1+0*3] + a.y * e[1+1*3] + a.z * e[1+2*3];
        b.z = a.x * e[2+0*3] + a.y * e[2+1*3] + a.z * e[2+2*3];
        return b;
    }
    
    //! Sets all coefficients to zero (null transform)
    void T3::zero() { std::fill(e, e+9, 0.0f); }
    
    //! Sets an identity transform
    void T3::identity() { zero(); e[0+0*3] = e[1+1*3] = e[2+2*3] = 1.0f; }
    
    //! Sets a rotation around X-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T3::xrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[1+1*3] =  std::cos(angle); e[2+1*3] = std::sin(angle);
        e[1+2*3] = -std::sin(angle); e[2+2*3] = std::cos(angle);
    }
    
    //! Sets a rotation around Y-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T3::yrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[0+0*3] =  std::cos(angle); e[2+0*3] = std::sin(angle);
        e[0+2*3] = -std::sin(angle); e[2+2*3] = std::cos(angle);
    }
    
    //! Sets a rotation around Z-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T3::zrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[0+0*3] =  std::cos(angle); e[1+0*3] = std::sin(angle);
        e[0+1*3] = -std::sin(angle); e[1+1*3] = std::cos(angle);
    }
    
    //! Sets a scaling transform, by factor on all dimensions
    void T3::scale(float factor) {
        zero(); e[0+0*3]=e[1+1*3]=e[2+2*3]=factor;
    }
    
    //! Sets a scaling transform, with different scaling factors
    //! for each dimension.
    void T3::scale(float fx, float fy, float fz) {
        zero(); e[0+0*3]=fx; e[1+1*3]=fy; e[2+2*3]=fz;
    }
    
    //! string representation
    std::string T3::to_string() const {
        std::string s;
        for(int i=0;i<3;i++)
            s += std::format("{:>6.2f} {:>6.2f} {:>6.2f}{}",e[3*i],e[3*i+1],e[3*i+2],i==2?"":"\n");
        return s;
    }

    T4::T4() { zero(); }
        
    //! Transform composition, multiplies this transform by b, overwrites this
    //! transform with the result, and returns a reference to this transform.
    T4 & T4::operator*=(const T4 & b) { (*this) = (*this) * b; return(*this); }
    
    //! Transform multiplication. Multiplies this transform by b, and returns
    //! a reference to the result without modifying this transform
    T4 T4::operator*(const T4 &b) const {
        T4 d;
        int i,j,k;
        d.zero();
        for(i=0;i<4;i++) for(j=0;j<4;j++) for(k=0;k<4;k++)
            d.e[i+j*4] += b.e[i+k*4] * e[k+j*4];
        return d;
    }
    
    bool T4::equals(const T4 &b, float epsilon) const {
        for(int i=0;i<4*4;i++)
            if (std::fabs(e[i] - b.e[i]) >= epsilon)
                return false;
        return true;
    }
    
    //! Applies this transform to point a and returns the transformed point
    R3 T4::apply(const R3 & a) const {
        R3 b;
        float w;
        b.x = a.x * e[0+0*4] + a.y * e[0+1*4] + a.z * e[0+2]*4 + e[0+3*4];
        b.y = a.x * e[1+0*4] + a.y * e[1+1*4] + a.z * e[1+2*4] + e[1+3*4];
        b.z = a.x * e[2+0*4] + a.y * e[2+1*4] + a.z * e[2+2*4] + e[2+3*4];
        w = e[3+0*4] + e[3+1*4] + e[3+2*4] + e[3+3*4];
        b /= w;
        return b;
    }
    
    //! Sets all coefficients to zero (null transform)
    void T4::zero() { std::fill(e,e+16,0.0f); }
    
    //! Sets an identity transform
    void T4::identity() { zero(); e[0+0*4] = e[1+1*4] = e[2+2*4] = e[3+3*4] = 1.0f; }
    
    //! Sets a rotation around X-axis transform, by angle degrees.
    //! Rotation center is given by (cx,cy,cz)
    void T4::xrot(float angle, float cx, float cy, float cz) {
        T4 &a = *this, b, c;
        a.translate(-cx,-cy,-cz);
        b.xrot(angle);
        c.translate(cx,cy,cz);
        a *= b;
        a *= c;
    }
    
    //! Sets a rotation around Y-axis transform, by angle degrees.
    //! Rotation center is given by (cx,cy,cz)
    void T4::yrot(float angle, float cx, float cy, float cz) {
        T4 &a = *this, b, c;
        a.translate(-cx,-cy,-cz);
        b.yrot(angle);
        c.translate(cx,cy,cz);
        a *= b;
        a *= c;
    }
    
    //! Sets a rotation around Z-axis transform, by angle degrees.
    //! Rotation center is given by (cx,cy,cz)
    void T4::zrot(float angle, float cx, float cy, float cz) {
        T4 &a = *this, b, c;
        a.translate(-cx,-cy,-cz);
        b.zrot(angle);
        c.translate(cx,cy,cz);
        a *= b;
        a *= c;
    }
    
    //! Sets a rotation around X-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T4::xrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[1+1*4] =  std::cos(angle); e[2+1*4] = std::sin(angle);
        e[1+2*4] = -std::sin(angle); e[2+2*4] = std::cos(angle);
    }
    
    //! Sets a rotation around Y-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T4::yrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[0+0*4] =  std::cos(angle); e[2+0*4] = std::sin(angle);
        e[0+2*4] = -std::sin(angle); e[2+2*4] = std::cos(angle);
    }
    
    //! Sets a rotation around Z-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T4::zrot(float angle) {
        identity();
        angle *= PI / 180.0f;
        e[0+0*4] =  std::cos(angle); e[1+0*4] = std::sin(angle);
        e[0+1*4] = -std::sin(angle); e[1+1*4] = std::cos(angle);
    }
    
    // rotation about arbitrary axis, around (cx,cy,cz)
    void T4::axisrot(const R3 &axis, float angle, float cx, float cy, float cz) {
        T4 &t = *this, r, ti;
        t.translate(-cx,-cy,-cz);
        r.axisrot(axis,angle);
        ti.translate(cx,cy,cz);
        t *= r;
        t *= ti;
    }
    
    // rotation about arbitrary axis, around the origin
    // Hearn et al, 1986, Computer Graphics, p. 224 (ISBN 0131653822)
    void T4::axisrot(const R3 &axis, float angle) {
        R3 au;
        T4 &rx = *this, ry, rz, ryi, rxi;
        float a,b,c,d;
    
        au = axis;
        au.normalize();
    
        a = au.x;
        b = au.y;
        c = au.z;
        d = std::sqrt(b*b+c*c);
        if (d == 0.0f) {
            xrot( a > 0.0f ? angle : -angle );
            return;
        }
    
        rx.set({1, 0,   0,   0,
               0, c/d, b/d, 0,
               0,-b/d, c/d, 0,
               0, 0,   0,   1});
        
        ry.set({d, 0, a, 0,
                0, 1, 0, 0,
               -a, 0, d, 0,
                0, 0, 0, 1});
        
        rz.zrot(angle);
        
        ryi.set({d, 0,-a, 0,
                 0, 1, 0, 0,
                 a, 0, d, 0,
                 0, 0, 0, 1});
        
        rxi.set({1, 0,    0,   0,
                 0, c/d, -b/d, 0,
                 0, b/d,  c/d, 0,
                 0, 0,    0,   1});
        
        rx *= ry;
        rx *= rz;
        rx *= ryi;
        rx *= rxi;
    }
    
    //! Sets a shearing transform along the X axis
    void T4::xshear(float yf, float zf) {
        identity(); e[0+1*4] = yf; e[0+2*4] = zf;
    }
    
    //! Sets a shearing transform along the Y axis
    void T4::yshear(float xf, float zf) {
        identity(); e[1+0*4] = xf; e[1+2*4] = zf;
    }
    
    //! Sets a shearing transform along the Z axis
    void T4::zshear(float xf, float yf) {
        identity(); e[2+0*4] = xf; e[2+1*4] = yf;
    }
    
    //! Sets a scaling transform, by factor on all dimensions
    void T4::scale(float factor) {
        zero(); e[0+0*4]=e[1+1*4]=e[2+2*4]=factor; e[3+3*4] = 1.0f;
    }
    
    //! Sets a scaling transform, by factors (fx,fy,fz)
    void T4::scale(float fx, float fy, float fz) {
        zero(); e[0+0*4]=fx; e[1+1*4]=fy; e[2+2*4]=fz; e[3+3*4] = 1.0f;
    }
    
    //! Sets a translation transform, by (dx,dy,dz)
    void T4::translate(float dx, float dy, float dz) {
        identity(); e[0+3*4] = dx; e[1+3*4] = dy; e[2+3*4] = dz;
    }
    
    void T4::set(std::initializer_list<float> coefs) {
        int i=0;
        for(auto it=coefs.begin();it!=coefs.end() && i<16; it++, i++) e[i] = *it;
    }
    
    //! Computes the inverse transform
    void T4::invert() { minv4(); }
    
    std::string T4::to_string() const {
        std::string s;
        for(int i=0;i<4;i++)
            s += std::format("{:>6.2f} {:>6.2f} {:>6.2f} {:>6.2f}{}",e[4*i],e[4*i+1],e[4*i+2],e[4*i+3],i==3?"":"\n");
        return s;
    }
    
    bool T4::minv4() {
        constexpr int n=4;
        int sle[n];
        float sq0[n], a[n*n];
        int lc,*le;
        float s,t,tq=0.0f,zr=1.e-15f;
        float *pa,*pd,*ps,*p,*q,*q0;
        int i,j,k,m;
                
        for(i=0;i<n*n;i++) a[i] = e[(i%n)+(i/n)*4];
        
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
            
            s=std::fabs(*pd); lc=j;
            for(k=j+1,ps=pd; k<n ;++k){
                if((t=std::fabs(*(ps+=n)))>s){ s=t; lc=k;}
            }
            tq=tq>s?tq:s; if(s<zr*tq){ return false; }
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
        for(i=0;i<n*n;i++) e[(i%n)+(i/n)*4] = a[i];
        return true;
    } // minv4

    
} // namespace
    