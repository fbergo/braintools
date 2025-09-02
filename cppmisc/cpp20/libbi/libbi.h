// lib brain imaging, C++ 20-ish
#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <concepts>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <numbers>
#include <format>
#include <initializer_list>

#include "libbi_algebra.h"

namespace libbi {
  
  //! 3-component color (RGB, YCbCr)
  class Color {
    public:
    uint8_t R, G, B;
    
    //! Constructor, creates black color
    Color() { R = G = B = 0; }
    
    //! Constructor, creates color with (R,G,B) = (a,b,c)
    Color(int a, int b, int c) {
      R = static_cast<uint8_t>(a);
      G = static_cast<uint8_t>(b);
      B = static_cast<uint8_t>(c);
    }
    
    //! Constructor, creates color with (R,G,B) = (a,b,c)
    Color(uint8_t a, uint8_t b, uint8_t c) {
      R=a; G=b; B=c;
    }
    
    //! Copy constructor
    Color(const Color &c) {
      R=c.R; G=c.G; B=c.B;
    }
    
    //! Constructor, creates color from integer representation.
    //! Integer representation: bits 0-7: blue; bits 8-15: green;
    //! bits 16-23: red; bits 24-31: ignored.
    Color(int c) {
      R = ((c >> 16) & 0xff);
      G = ((c >> 8) & 0xff);
      B = (c & 0xff);
    }
    
    //! Comparison operator
    bool operator==(const Color &c) const {
      return (c.R == R && c.G == G && c.B == B);
    }
    
    //! Comparison operator
    bool operator!=(const Color &c) const {
      return (c.R != R || c.G != G || c.B != B);
    }
    
    //! Assignment operator
    Color &operator=(const Color &c) {
      R=c.R; G=c.G; B=c.B;
      return (*this);
    }
    
    //! Assignment operator, from integer representation. See \ref Color(int c)
    template<std::integral T> Color &operator=(const T c) {
      R = static_cast<uint8_t>((c >> 16) & 0xff);
      G = static_cast<uint8_t>((c >> 8) & 0xff);
      B = static_cast<uint8_t>(c & 0xff);
      return (*this);
    }
    
    //! Returns the integer representation of this color. See \ref Color(int c)
    int toInt() const { return ((((int)R) << 16) | (((int)G) << 8) | ((int)B)); }
    
    //! Lightens or darkens this color by factor x. The factor multiplies the
    //! luminance component in the YCbCr representation of this color.
    Color &operator*=(float x) {
      rgb2ycbcr();
      R = static_cast<uint8_t>(std::min(R * x, 255.0f));
      ycbcr2rgb();
      return (*this);
    }
    
    //! Luminance multiplication operator
    Color operator*(float x) const {
      Color c(*this);
      c *= x;
      return c;
    }
    
    //! sets a gray color, with (R,G,B) = (c,c,c), returns a reference
    //! to this color.
    Color &gray(int c);
    
    //! Mixes color src in this color, with proportion srcamount of src
    //! and (1-srcamount) of this color. Overwrites this color with
    //! the result and returns a reference to this color.
    Color &mix(const Color &src, float srcamount);
    
    //! Converts an RGB color triplet to YCbCr
    void rgb2ycbcr();
    
    //! Converts an YCbCr color triplet to RGB
    void ycbcr2rgb();
    
  };
  
  class Font {
    public:
    Font() { }
    // TODO
  };
  
  // 2D RGB image
  class Image {
    public:
      int W,H,N;
    private:
      std::vector<uint8_t> data;
    
    public:
      Image(); // creates empty image
      Image(int w, int h); //creates black-filled image
      Image(const std::string &filename); // reads from P5/P6 file
      Image(const char **xpm, const Color &transp); // reads from inline XPM
      
      bool readP6(const std::string &filename); // reads from P5/P6
      
      bool writeP6(const std::string &filename);  // writes to P6
      bool writePNG(const std::string &filename); // writes to PNG
      
      bool empty() const;
      void fill(const Color &c); // solid fill
      bool valid(int x,int y) const; // tests valid coordinates
      void set(int x, int y, const Color &c); // sets pixel
      void set(int x, int y, int c); // sets pixel
      int  get(int x, int y) const;  // gets pixel value
      
      void text(Font &f, const std::string &text, int x,int y, const Color &c, float opacity=1.0f);
      
      //! Flood fills from point (x,y) with \ref Color c. Flood fill is performed
      //! with 4-neighbors adjacency. Any color different from the previous color
      //! of (x,y) halts the flood. It does nothing if (x,y) has \ref Color c.
      void floodFill(int x,int y,const Color &c);
      
      //! Alpha bit block transfer: copies rectangle (0,0)-(w-1,h-1) of Image
      //! src to position (x,y) of this image. Pixels of color trans in src are
      //! considered transparent and are not copied. If w or h are negative,
      //! the width and/or height of src are used.
      void ablit(const Image &src, const Color &trans, int x, int y, int w=-1, int h=-1);
      
      //! Alpha bit block increment: similar to \ref ablit, but instead of
      //! copying the src image, uses it as a mask, and increments R,G and B
      //! of the pixels on this image where a non-transparent pixel of src
      //! would be painted.
      void ablinc(const Image &src, const Color &trans,int x,int y,int w=-1,int h=-1);
      
      //! Bit block transfer: copies rectangle (sx,sy)-(sx+w-1,sy+h-1) of Image
      //! src to position (dx,dy) of this image. If w or h are negative,
      //! the width and/or height of src are used.
      void blit(const Image &src, int sx, int sy, int dx,int dy,int w=-1,int h=-1);
      
      unsigned char *getBuffer();
      
      //! Draws a filled translucent rectangle with top left corner (x,y), 
      //! size (w,h), \ref Color src and opacity srcamount
      void blendbox(int x, int y, int w, int h, const Color &src, float srcamount);
      
      //! Shades a filled rectangle with top left corner (x,y) and size
      //! (w,h) by multiplying the YCbCr luminance of each pixel by factor
      void shadebox(int x, int y, int w, int h, float factor);
      
      //! Draws a rectangle with top left corner (x,y), size (w,h) and
      //! \ref Color c. If fill is true, the rectangle is filled.
      void rect(int x, int y, int w, int h, const Color &c, bool fill=true);
      
      //! Draws a line segment between points (x1,y1) and (x2,y2), with
      //! \ref Color c.
      void line(int x1,int y1,int x2,int y2, const Color &c);
      
      //! Scales this image by factor, and returns the new scaled image result. 
      //! It does not modify this image.
      Image scale(float factor) const;
    
    private:
      Image scaleUp(float factor) const;
      void  readXPM(const char **xpm, const Color &transp);
  };
  
  //! Point in discrete 3D space
  class P3 {
    public:
    int x,y,z;
    
    //! Translates this location by src, returns a reference to this
    //! location.
    P3 & operator+=(const P3 &src) {  x+=src.x; y+=src.y; z+=src.z; return(*this); }
    
    //! Returns true if this location is closer to the origin than b.
    bool operator<(const P3 &b) const { return(sqlen()<b.sqlen()); }
    
    //! Returns true if this and b are the same location
    bool operator==(const P3 &b) const { return(x==b.x && y==b.y && z==b.z); }
    
    //! Returns true if this and b are different locations
    bool operator!=(const P3 &b) const { return(x!=b.x || y!=b.y || z!=b.z); }
    
    //! Sets (X,Y,Z) to (x,y,z), return a reference to this location.
    P3 & set(int _x,int _y,int _z) { x=_x; y=_y; z=_z; return(*this); }
    
    //! Prints this location to stdout.
    std::string to_string() { return(std::format("({},{},{}) : {}",x,y,z,sqlen())); }
    
    //! Returns the square of the distance from the origin
    int sqlen() const { return( (x*x)+(y*y)+(z*z) ); }
    
    //! Returns the square of the distance between this and b
    int sqdist(const P3 &b) const { return( (x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z) ); }
  };
    
  //! Abstract discrete voxel adjacency
  class Adjacency {
    public:
    //! Returns the number of elements in this adjacency
    int size() { return(static_cast<int>(data.size())); }
    
    //! Returns the index-th (0-based) neighbor displacement, as a \ref Location
    P3 & operator[](int index) { return(data[index]); }
    
    //! Returns the i-th (0-based) neighbor of src, as a \ref Location
    P3 neighbor(const P3 &src, int i) {
      P3 tmp(src);
      tmp += data[i];
      return(tmp);
    }
    
    protected:
    //! Vector of neighbors
    std::vector<P3> data;
  };
  
  //! Discrete spherical voxel adjacency
  class SphericalAdjacency : public Adjacency {    
    public:  
    //! Constructor, creates adjacency of given radius. If self is true,
    //! the center voxel is considered its own neighbor.
    SphericalAdjacency(float radius,bool self=false) { resize(radius,self); }
    
    //! Recreates this adjacency, with given radius. If self is true,
    //! the center voxel is considered its own neighbor.
    void resize(float radius, bool self=false);
  };
  
  //! Discrete circular adjacency on the XY plane
  class DiscAdjacency : public Adjacency {
    public:    
    //! Constructor, creates adjacency of given radius. If self is true,
    //! the center voxel is considered its own neighbor.
    DiscAdjacency(float radius,bool self=false) { resize(radius,self); }
    
    //! Recreates this adjacency, with given radius. If self is true,
    //! the center voxel is considered its own neighbor.
    void resize(float radius, bool self=false);
    
  };
  
  class VolumeDomain {
    public:
    int W,H,D,WxH,N;
    
    VolumeDomain();
    VolumeDomain(int _w, int _h, int _d);
    void resize(int _w, int _h, int _d);
    
    int  address(int x, int y, int z) const;
    bool valid(int x, int y, int z) const;
    bool valid(const P3 &p) const;
    bool valid_address(int a) const;
    
    template<std::integral T> bool valid(T x, T y, T z) const {
      return(valid( static_cast<int>(x), static_cast<int>(y), static_cast<int>(z) ));
    }
    
    int xOf(int a) const { return( W==0 ? 0 : (a%WxH) % W ); }
    int yOf(int a) const { return( W==0 ? 0 : (a%WxH) / W ); }
    int zOf(int a) const { return( WxH==0 ? 0 : a / WxH ); }
    
    P3 xyzOf(int a) const { return(P3(xOf(a),yOf(a),zOf(a))); }
    
    int yOffset(int y) const { return( y>=0 && y<H ? tby[y] : 0 ); }
    int zOffset(int z) const { return( z>=0 && z<D ? tbz[z] : 0 ); }
    
    //! Returns the integer length of the diagonal segment (0,0,0)-(W,H,D)
    int diagonalLength() const;
    
    private:
    std::vector<int> tby, tbz;
    
    void recalc_tables();
  };
  
  template<typename T> class Volume : public VolumeDomain {
    public:
    float dx,dy,dz; // voxel dimensions
    
    // creates null volume
    Volume() : VolumeDomain() {
      dx = dy = dz = 1.0f;
    }
    
    // creates in-memory blank volume
    Volume(int w,int h,int d, float _dx=1.0f, float _dy=1.0f, float _dz=1.0f) : VolumeDomain(w,h,d) {
      dx = _dx;
      dy = _dy;
      dz = _dz;
    }
    
    // reads volume from file
    Volume(const std::string &filename) : VolumeDomain() {
      // not implemented
    }
    
    // copy constructor
    Volume(const Volume<T> &src) : VolumeDomain(src) {
      dx = src.dx;
      dy = src.dy;
      dz = src.dz;
      data = src.data;
    }
    
    void resize(int w,int h,int d, bool preserve=true) {
      if (preserve) {
        VolumeDomain ndom(w,h,d);
        std::vector<T> ndata(ndom.N);
        int cw = std::min(W, w);
        int ch = std::min(H, h);
        int cd = std::min(D, d);
        for(int z=0;z<cd;z++) {
          int src_zoff  = zOffset(z);
          int dest_zoff = ndom.zOffset(z);
          for(int y=0;y<ch;y++) {
            int src_yoff = yOffset(y);
            int dest_yoff = ndom.yOffset(z);
            std::ranges::copy_n(data.begin()+src_zoff+src_yoff, cw, ndata.begin()+dest_zoff+dest_yoff);
          }
        }
        VolumeDomain::resize(w,h,d); // update domain size
        std::swap(data, ndata); // move-swap data
      } else {
        VolumeDomain::resize(w,h,d);
        data.resize(N);
        std::fill(data.begin(), data.end(), static_cast<T>(0));
      }
    }
    
    void clear() {
      resize(0,0,0);
      data.clear();
    }
    
    void fill(const T& val) {
      std::fill(data.begin(), data.end(), val);
    }
    
    bool empty() const { return(N==0); }
    
    T maximum() const { return(* std::max_element(data.begin(), data.end())); }
    T minimum() const { return(* std::min_element(data.begin(), data.end())); }
    
    // voxel access without bounds checking
    T& voxel(int a)                        { return(data[a]); }
    T& voxel(int x, int y, int z)          { return(data[address(x,y,z)]); }
    T& voxel(const P3 &p)                  { return(data[address(p.x,p.y,p.z)]); }
    T& voxel(float x, float y, float z)    { return(data[address((int)x,(int)y,(int)z)]); }
    T& voxel(double x, double y, double z) { return(data[address((int)x,(int)y,(int)z)]); }
    
    double mean() const {
      double sum=0.0;
      if (empty()) return 0.0;
      for(auto &x : data) sum += x;
      return(sum / (double) N);
    }
    
    double stdev(double _mean) const {
      double sum=0.0;
      for(auto &x : data) sum += (_mean-x)*(_mean-x);
      return(std::sqrt(sum / (double) N));  
    }
    
    private:
    std::vector<T> data;
    
    
    
  };
  
} // namespace libbi
