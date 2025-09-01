// lib brain imaging, C++ 20-ish
#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <concepts>
#include <string>
#include <memory>
#include <vector>

namespace libbi {

  //! 3-component color (RGB, YCbCr)
  class Color {
  public:
    uint8_t R, G, B;

    //! Constructor, creates black color
    Color() { R = G = B = 0; }

    //! Constructor, creates color with (R,G,B) = (a,b,c)
    Color(int a, int b, int c) {
      (R,G,B) = (static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(c));
    }

    //! Constructor, creates color with (R,G,B) = (a,b,c)
    Color(uint8_t a, uint8_t b, uint8_t c) {
      (R,G,B) = (a,b,c);
    }

    //! Copy constructor
    Color(const Color &c) {
      (R,G,B) = (c.R, c.G, c.B);
    }

    //! Constructor, creates color from integer representation.
    //! Integer representation: bits 0-7: blue; bits 8-15: green;
    //! bits 16-23: red; bits 24-31: ignored.
    Color(int c) {
      (R,G,B) = ( ((c >> 16) & 0xff), ((c >> 8) & 0xff), (c & 0xff));
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
      (R,G,B) = (c.R, c.G, c.B);
      return (*this);
    }

    //! Assignment operator, from integer representation. See \ref Color(int c)
    template<std::integral T> Color &operator=(const T c) {
      (R,G,B) = ( static_cast<uint8_t>((c >> 16) & 0xff), static_cast<uint8_t>((c >> 8) & 0xff), static_cast<uint8_t>(c & 0xff));
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

  class VolumeDomain {
    public:
      int W,H,D,WxH,N;

      VolumeDomain();
      VolumeDomain(int _w, int _h, int _d);
      void resize(int _w, int _h, int _d);

      int  address(int x, int y, int z) const;
      bool valid(int x, int y, int z) const;

      template<std::integral T> bool valid(T x, T y, T z) const {
        return(valid( static_cast<int>(x), static_cast<int>(y), static_cast<int>(z) ));
      }

      int xOf(int a) const { return( W==0 ? 0 : (a%WxH) % W ); }
      int yOf(int a) const { return( W==0 ? 0 : (a%WxH) / W ); }
      int zOf(int a) const { return( WxH==0 ? 0 : a / WxH ); }

      //! Returns the integer length of the diagonal segment (0,0,0)-(W,H,D)
      int diagonalLength() const;
      
      private:
      std::vector<int> tby, tbz;

      void recalc_tables();
  };

  template<typename T> class Volume : public VolumeDomain {
    public:
      float dx,dy,dz; // voxel dimensions
      T lastmax;

      // creates null volume
      Volume() : VolumeDomain() {
        dx = dy = dz = 1.0f;
      }

      // creates in-memory blank volume
      Volume(int w,int h,int d, float _dx=1.0f, float _dy=1.0f, float _dz=1.0f) : VolumeDomain(w,h,d) {
        (dx,dy,dz) = (_dx,_dy,_dz);
      }

      // reads volume from file
      Volume(const std::string &filename) : VolumeDomain() {
        // not implemented
      }

      // copy constructor
      Volume(const Volume<T> &src) : VolumeDomain(src) {
        // not implemented
      }

    private:
      std::vector<T> data;



  };

} // namespace libbi
