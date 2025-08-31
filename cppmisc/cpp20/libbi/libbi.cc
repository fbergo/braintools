#include <string>
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
    
    
}

