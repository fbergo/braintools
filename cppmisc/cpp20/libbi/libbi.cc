#include <string>
#include <queue>
#include "libbi.h"


namespace libbi {
    
    const std::string version() {
        static const std::string v = "1.0.0";
        return v;
    }
    
    //! sets a gray color, with (R,G,B) = (c,c,c), returns a reference
    //! to this color.
    Color & Color::gray(int c) {
        R = G = B = static_cast<uint8_t>(c);
        return (*this);
    }
    
    //! Mixes color src in this color, with proportion srcamount of src
    //! and (1-srcamount) of this color. Overwrites this color with
    //! the result and returns a reference to this color.
    Color & Color::mix(const Color &src, float srcamount) {
        R = static_cast<uint8_t>((1.0f - srcamount) * R + (srcamount * (src.R)));
        G = static_cast<uint8_t>((1.0f - srcamount) * G + (srcamount * (src.G)));
        B = static_cast<uint8_t>((1.0f - srcamount) * B + (srcamount * (src.B)));
        return (*this);
    }
    
    //! Converts an RGB color triplet to YCbCr
    void Color::rgb2ycbcr() {
        float y, cb, cr;
        float fr = static_cast<float>(R);
        float fg = static_cast<float>(G);
        float fb = static_cast<float>(B);
        
        y  =  0.257f * fr + 0.504f * fg + 0.098f * fb + 16.0f;
        cb = -0.148f * fr - 0.291f * fg + 0.439f * fb + 128.0f;
        cr =  0.439f * fr - 0.368f * fg - 0.071f * fb + 128.0f;
        
        R = static_cast<uint8_t>(y);
        G = static_cast<uint8_t>(cb);
        B = static_cast<uint8_t>(cr);
    }
    
    //! Converts an YCbCr color triplet to RGB
    void Color::ycbcr2rgb() {
        float r, g, b;
        float fr = static_cast<float>(R);
        float fg = static_cast<float>(G);
        float fb = static_cast<float>(B);
        
        r = 1.164 * (fr - 16.0f) + 1.596f * (fb - 128.0f);
        g = 1.164 * (fr - 16.0f) - 0.813f * (fb - 128.0f) - 0.392 * (fg - 128.0f);
        b = 1.164 * (fr - 16.0f) + 2.017f * (fg - 128.0f);
        r = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
        g = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
        b = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
    }
    
    Image::Image() {
        W=H=N=0;
    }
    
    Image::Image(int w, int h) {
        W=w;
        H=h;
        N=W*H;
        data.resize(N*3);
        std::fill(data.begin(), data.end(), 0);
    }
    
    Image::Image(const std::string &filename) {
        W=H=N=0;
        readP6(filename);
    }
    
    Image::Image(const char **xpm, const Color &transp) {
        
        
    }

    bool Image::readP6(const std::string &filename) {
        return false;    
    }
    
    bool Image::writeP6(const std::string &filename) {
        return false;
    }
    
    bool Image::writePNG(const std::string &filename) {
        return false;
    }

    bool Image::empty() const { 
        return(N==0);
    }
    
    void Image::fill(const Color &c) {
        const int rowlen = 3*W;
        // fill first row
        for(int i=0;i<3*W;) {
            data[i++] = c.R;
            data[i++] = c.G;
            data[i++] = c.B;
        }
        // copy remaining rows
        for(int i=1;i<H;i++) {
            std::ranges::copy_n(data.begin(), rowlen, data.begin()+rowlen*i);
        }
    }
    
    bool Image::valid(int x,int y) const {
        return(x>=0 && x<W && y>=0 && y<H); 
    }
    
    void Image::set(int x, int y, const Color &c) {
        int pos = 3*(x+y*W);
        data[pos]   = c.R;
        data[pos+1] = c.G;
        data[pos+2] = c.B;
    }
    
    void Image::set(int x, int y, int c) {
        Color _c(c);
        int pos = 3*(x+y*W);
        data[pos]   = _c.R;
        data[pos+1] = _c.G;
        data[pos+2] = _c.B;
    }
    
    int  Image::get(int x, int y) const {
        int pos = 3*(x+y*W);
        return(Color(data[pos], data[pos+1], data[pos+2]).toInt());
    }
    
    void Image::text(Font &f, const std::string &text, int x,int y, const Color &c, float opacity) {
        // TODO
    }
    
    //! Flood fills from point (x,y) with \ref Color c. Flood fill is performed
    //! with 4-neighbors adjacency. Any color different from the previous color
    //! of (x,y) halts the flood. It does nothing if (x,y) has \ref Color c.
    void Image::floodFill(int x,int y,const Color &c) {
        std::queue<int> fifo;
        std::vector<bool> done(W*H, false);
        int bg,px,py,pi;
        
        bg = get(x,y);
        if (c.toInt() == bg) return;
        
        fifo.push(x+y*W);    
        done[x+y*W] = true;

        auto proc_neighbor = [&](int nx, int ny) {
            if (valid(nx,ny) && !done[nx+ny*W] && get(nx,ny)==bg) {
                fifo.push(nx+ny*W);
                done[nx+ny*W] = true;
            }
        };

        while(!fifo.empty()) {
            pi = fifo.front();
            done[pi] = true;
            fifo.pop();
            
            px = pi%W;
            py = pi/W;
            if (valid(px,py)) {
                set(px,py,c);                
                proc_neighbor(px-1, py);
                proc_neighbor(px+1, py);
                proc_neighbor(px, py-1);
                proc_neighbor(px, py+1);
            }
        }
    }
    
    //! Alpha bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
    //! src to position (x,y) of this image. Pixels of color trans in src are
    //! considered transparent and are not copied. If w or h are negative,
    //! the width and/or height of src are used.
    void Image::ablit(const Image &src, const Color &trans, int x, int y, int w, int h) {
        int i,j,s,d;
        uint8_t r,g,b,ar,ag,ab;
        
        if (w<0) w=src.W;
        if (h<0) h=src.H;
        
        ar = trans.R;
        ag = trans.G;
        ab = trans.B;
        
        for(j=0;j<h;j++) {
            for(i=0;i<w;i++) {
                if (valid(x+i,y+j)) {
                    d = 3*(x+i+W*(y+j));
                    s = 3*(i+src.W*j);
                    r = src.data[s];
                    g = src.data[s+1];
                    b = src.data[s+2];
                    if (r!=ar || g!=ag || b!=ab) {
                        data[d]   = r;
                        data[d+1] = g;
                        data[d+2] = b;
                    }
                }
            }
        }
    }
    
    //! Alpha bit block increment: similar to \ref ablit, but instead of
    //! copying the src image, uses it as a mask, and increments R,G and B
    //! of the pixels on this image where a non-transparent pixel of src
    //! would be painted.
    void Image::ablinc(const Image &src, const Color &trans,int x,int y,int w,int h) {
        int i,j,s,d;
        uint8_t r,g,b,ar,ag,ab;
        
        if (w<0) w=src.W;
        if (h<0) h=src.H;

        ar = trans.R;
        ag = trans.G;
        ab = trans.B;
        
        for(j=0;j<h;j++) {
            for(i=0;i<w;i++) {
                if (valid(x+i,y+j)) {
                    d = 3*(x+i+W*(y+j));
                    s = 3*(i+src.W*j);
                    r = src.data[s];
                    g = src.data[s+1];
                    b = src.data[s+2];
                    if (r!=ar || g!=ag || b!=ab) {
                        ++data[d];
                        ++data[d+1];
                        ++data[d+2];
                    }
                } 
            }
        }     
    }
    
    //! Bit block transfer: copies rectangle (sx,sy)-(sx+w-1,sy+h-1) of Image
    //! src to position (dx,dy) of this image. If w or h are negative,
    //! the width and/or height of src are used.
    void Image::blit(const Image &src, int sx, int sy, int dx,int dy,int w,int h) {
        
        
    }
    
    unsigned char *Image::getBuffer() {
        return(reinterpret_cast<unsigned char *>(data.data()));
    }
    
    //! Draws a filled translucent rectangle with top left corner (x,y), 
    //! size (w,h), \ref Color src and opacity srcamount
    void Image::blendbox(int x, int y, int w, int h, const Color &src, float srcamount) {
        
    }
    
    //! Shades a filled rectangle with top left corner (x,y) and size
    //! (w,h) by multiplying the YCbCr luminance of each pixel by factor
    void Image::shadebox(int x, int y, int w, int h, float factor) {
        
    }
    
    //! Draws a rectangle with top left corner (x,y), size (w,h) and
    //! \ref Color c. If fill is true, the rectangle is filled.
    void Image::rect(int x, int y, int w, int h, const Color &c, bool fill) {
        
    }
    
    //! Draws a line segment between points (x1,y1) and (x2,y2), with
    //! \ref Color c.
    void Image::line(int x1,int y1,int x2,int y2, const Color &c) {
        
    }
    
    //! Scales this image by factor, and returns the new scaled image result. 
    //! It does not modify this image.
    Image Image::scale(float factor) const {
        Image dst;
        // TODO
        return dst;
    }

    void SphericalAdjacency::resize(float radius, bool self) {
        int dx,dy,dz,r0,r2;
        
        data.clear();
        r0 = (int) radius;
        r2 = (int) (radius*radius);
        
        for(dz=-r0;dz<=r0;dz++) {
          for(dy=-r0;dy<=r0;dy++) {
            for(dx=-r0;dx<=r0;dx++) {
              P3 loc(dx,dy,dz);
              if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
                data.push_back(loc);
              }
            }
          }
        }
        std::sort(data.begin(),data.end());
    }    

    //! Recreates this adjacency, with given radius. If self is true,
    //! the center voxel is considered its own neighbor.
    void DiscAdjacency::resize(float radius, bool self) {
      int dx,dy,r0,r2;
      
      data.clear();
      r0 = (int) radius;
      r2 = (int) (radius*radius);
      
      for(dy=-r0;dy<=r0;dy++) {
        for(dx=-r0;dx<=r0;dx++) {
          P3 loc(dx,dy,0);
          if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
            data.push_back(loc);
          }
        }
      }
      std::sort(data.begin(),data.end());
    }     

    VolumeDomain::VolumeDomain() { 
        W=H=D=WxH=N=0;
    }
    
    VolumeDomain::VolumeDomain(int _w, int _h, int _d) {
        W = _w; H = _h; D = _d;
        WxH = W*H;
        N = W*H*D;
        recalc_tables();
    }

    void VolumeDomain::resize(int _w, int _h, int _d) {
        if (W!=_w || H!=_h || D!=_d) {
            W = _w; H = _h; D = _d;
            WxH = W*H;
            N = W*H*D;
            recalc_tables();
        }        
    }

    int VolumeDomain::address(int x,int y,int z) const {
        return(x+tby[y]+tbz[z]);
    }

    bool VolumeDomain::valid(int x, int y, int z) const {
        return(x>=0 && x<W && y>=0 && y<H && z>=0 && z<D);
    }

    bool VolumeDomain::valid(const P3 &p) const {
        return(valid(p.x,p.y,p.z)); 
    }

    bool VolumeDomain::valid_address(int a) const {
        return(a>=0 && a<N);
    }

    int VolumeDomain::diagonalLength() const { 
        return( static_cast<int>(std::sqrt(static_cast<double>(W*W+H*H+D*D))) ); 
    }
 
    void VolumeDomain::recalc_tables() {
        if (N>0) {
            tby.resize(H);
            tbz.resize(D);
            for(int i=0;i<H;i++) tby[i] = W*i;
            for(int i=0;i<D;i++) tbz[i] = WxH*i;
        }
    }

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
        angle *= M_PI / 180.0f;
        e[1+1*3] =  std::cos(angle); e[2+1*3] = std::sin(angle);
        e[1+2*3] = -std::sin(angle); e[2+2*3] = std::cos(angle);
    }
    
    //! Sets a rotation around Y-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T3::yrot(float angle) {
        identity();
        angle *= M_PI / 180.0f;
        e[0+0*3] =  std::cos(angle); e[2+0*3] = std::sin(angle);
        e[0+2*3] = -std::sin(angle); e[2+2*3] = std::cos(angle);
    }
    
    //! Sets a rotation around Z-axis transform, by angle degrees.
    //! Rotation center is the origin.
    void T3::zrot(float angle) {
        identity();
        angle *= M_PI / 180.0f;
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
        int i,j;
        std::string s;
        s = std::format("{:>6.2f} {:>6.2f} {:>6.2f}\n{:>6.2f} {:>6.2f} {:>6.2f}\n{:>6.2f} {:>6.2f} {:>6.2f}",
            e[0],e[1],e[2],e[3],e[4],e[5],e[6],e[7],e[8]);
        return s;
    }
    
    /*
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
        delete[] sq0;
        delete[] sle;
        for(i=0;i<n*n;i++) e[i%n][i/n] = a[i];
        delete[] a;
        return true;
      } // minv4
    };
    */
    



} // namespace

