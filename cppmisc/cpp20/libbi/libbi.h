// lib brain imaging, C++ 20-ish
#pragma once

#include <cstdint>
#include <algorithm>
#include <concepts>

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

} // namespace libbi
