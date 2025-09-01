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


}

