#include <string>
#include <sstream>
#include <queue>
#include <map>
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
        W=H=N=0;
        readXPM(xpm, transp);
    }

    bool Image::readP6(const std::string &filename) {
        // TODO
        return false;    
    }
    
    bool Image::writeP6(const std::string &filename) {
        // TODO
        return false;
    }
    
    bool Image::writePNG(const std::string &filename) {
        // TODO
        return false;
    }
    
    void Image::readXPM(const char **xpm, const Color &transp) {
        using std::string;
        using std::map;
        using std::vector;
        using std::istringstream;
        
        map<string, Color> colortable;
        int ncolors, cpp;
        
        auto tokenize = [](const string &s) -> vector<string> {
            istringstream iss(s);
            vector<string> out;
            string t;
            while(iss >> t) out.push_back(t);
            return out;
        };

        auto lower = [](const string &s) -> string {
            string out(s);
            for(auto &c : out) c=std::tolower(c);
            return out;
        };

        // parse header
        {
            auto tokens = tokenize(xpm[0]);
            if (tokens.size()!=4) return;
            W =       std::stoi(tokens[0]);
            H =       std::stoi(tokens[1]);
            ncolors = std::stoi(tokens[2]);
            cpp     = std::stoi(tokens[3]);
        }
        
        // resize image
        N = W*H;
        data.resize(N*3);
        std::fill(data.begin(), data.end(), 0);        
        
        // read color table
        int line = 1;
        std::string key;
        
        for(int i=0;i<ncolors;i++,line++) {

            string xs(xpm[line]);
            string key = xs.substr(0,cpp);
            
            auto tokens = tokenize(xs.substr(cpp));
            if (tokens.size() < 2) continue;

            Color c(0);

            if (lower(tokens[0]) == "c") {
                if (lower(tokens[1]) == "none")
                    c = transp;
                else if (tokens[1][0] == '#') {
                    c = static_cast<unsigned int>( std::stol(tokens[1].substr(1), nullptr, 16 ) );
                }
                colortable[key] = c;
            }
        }
        
        // interpret image data
        Color c(0);
        for(int j=0;j<H;j++) {
            string xs(xpm[line+j]);
            int npix = xs.size() / cpp;
            npix = std::min(npix, W);
            for(int i=0;i<npix;i++) {
                string key = xs.substr(i*cpp, cpp);
                try { c = colortable.at(key); } catch(...) { c=0; }                
                set(i,j,c);
            }
        }
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
        if (w<0) w=src.W;
        if (h<0) h=src.H;
        
        w = std::min({w, src.W-sx, W-dx});  
        h = std::min({h, src.H-sy, H-dy});

        for(int j=0;j<h;j++) {
            int s = 3*(sx+(sy+j)*src.W);
            int d = 3*(dx+(dy+j)*W);
            std::copy_n(&src.data[s], 3*w, &data[d]);
        }
    }
    
    unsigned char *Image::getBuffer() {
        return(reinterpret_cast<unsigned char *>(data.data()));
    }
    
    //! Draws a filled translucent rectangle with top left corner (x,y), 
    //! size (w,h), \ref Color src and opacity srcamount
    void Image::blendbox(int x, int y, int w, int h, const Color &src, float srcamount) {
        int i,j,p;
        Color c;
        if (x >= W || y >= H) return;
        if (x<0) { w+=x; x=0; }
        if (y<0) { h+=y; y=0; }
        w = std::min(w, W-x);
        h = std::min(h, H-y);
        if (w<0 || h<0) return;
        p=3*(x+y*W);
        for(i=0;i<h;i++) {
          for(j=0;j<w;j++) {
            c.R = data[p+3*j ];
            c.G = data[p+3*j+1 ];
            c.B = data[p+3*j+2 ];
            c.mix(src,srcamount);
            data[p+3*j ]   = c.R;
            data[p+3*j+1 ] = c.G;
            data[p+3*j+2 ] = c.B;
          }
          p += 3*W;
        }    
    }
    
    //! Shades a filled rectangle with top left corner (x,y) and size
    //! (w,h) by multiplying the YCbCr luminance of each pixel by factor
    void Image::shadebox(int x, int y, int w, int h, float factor) {
        int i,j,p;
        Color c;
        if (x >= W || y >= H) return;
        if (x<0) { w+=x; x=0; }
        if (y<0) { h+=y; y=0; }
        w = std::min(w, W-x);
        h = std::min(h, H-y);
        if (w<0 || h<0) return;
        p=3*(x+y*W);
        for(i=0;i<h;i++) {
            for(j=0;j<w;j++) {
                c.R = data[p+3*j ];
                c.G = data[p+3*j+1 ];
                c.B = data[p+3*j+2 ];
                c *= factor;
                data[p+3*j ]   = c.R;
                data[p+3*j+1 ] = c.G;
                data[p+3*j+2 ] = c.B;
            }
            p += 3*W;
        }    
    }
    
    //! Draws a rectangle with top left corner (x,y), size (w,h) and
    //! \ref Color c. If fill is true, the rectangle is filled.
    void Image::rect(int x, int y, int w, int h, const Color &c, bool fill) {
        if (!fill) {
            line(x,y,x+w-1,y,c);
            line(x,y+h-1,x+w-1,y+h-1,c);
            line(x,y,x,y+h-1,c);
            line(x+w-1,y,x+w-1,y+h-1,c);
        } else {
            if (x >= W || y >= H) return;
            if (x<0) { w+=x; x=0; }
            if (y<0) { h+=y; y=0; }
            w = std::min(w, W-x);
            h = std::min(h, H-y);
            if (w<0 || h<0) return;

            int p=3*(x+y*W);
            for(int j=0;j<w;j++) {
                data[p+3*j ]   = c.R;
                data[p+3*j+1 ] = c.G;
                data[p+3*j+2 ] = c.B;
            }
            for(int i=1;i<h;i++) {
                std::copy_n(&data[p], 3*w, &data[p+3*W*i]);
            }
        }
    }
    
    //! Draws a line segment between points (x1,y1) and (x2,y2), with
    //! \ref Color c.
    void Image::line(int x1,int y1,int x2,int y2, const Color &c) {
        int x, y;
        int dy = y2 - y1;
        int dx = x2 - x1;
        int G, DeltaG1, DeltaG2;	
        int inc = 1;
        
        auto inbounds = [&](int _x, int _y) -> bool { return(_x>=0 && _y>=0 && _x<W && _y<H); };

        if (inbounds(x1,y1)) set(x1,y1,c);
        
        if (std::abs(dy) < std::abs(dx)) {
            /* -1 < slope < 1 */
            if (dx < 0) {
                dx = -dx; dy = -dy;
                std::swap(y1,y2);
                std::swap(x1,x2);      
            }
            
            if (dy < 0) { dy = -dy; inc = -1; }
            
            y = y1; x = x1 + 1;      
            G = 2 * dy - dx; DeltaG1 = 2 * (dy - dx); DeltaG2 = 2 * dy;
            
            while (x <= x2) {
                if (G > 0) { G += DeltaG1; y += inc; } 
                else G += DeltaG2;	
                if (inbounds(x,y)) set(x,y,c);
                x++;
            }
        } else {
            /* slope < -1 or slope > 1 */
            if (dy < 0) { 
                dx = -dx; dy = -dy;
                std::swap(y1,y2);
                std::swap(x1,x2);
            }      
            if (dx < 0) { dx = -dx; inc = -1; }
            
            x = x1; y = y1 + 1;      
            G = 2 * dx - dy; DeltaG1 = 2 * (dx - dy);
            DeltaG2 = 2 * dx;
            
            while (y <= y2) {
                if (G > 0) { G += DeltaG1; x += inc; } 
                else G += DeltaG2;
                
                if (inbounds(x,y)) set(x,y,c);
                y++;
            }
        }
    }
    
    //! Scales this image by factor, and returns the new scaled image result. 
    //! It does not modify this image.
    Image Image::scale(float factor) const {
        if (factor > 1.0f) return(scaleUp(factor));

        Image dst;
        int nw,nh,ow,oh;
        float x1,x2,y1,y2,fi,fj,dx,dy,fr,fg,fb,di;
        int i,j,k,a,b;
        uint8_t R,G,B;

        std::vector<uint8_t> lookup[3];
        std::vector<int> area;
        int  count;
    
        nw = (int) (static_cast<float>(W) * factor);
        nh = (int) (static_cast<float>(H) * factor);
        ow = W;
        oh = H;
        if (nw<=0 || nh<=0) return dst;
    
        dst = std::move(Image(nw,nh));
        lookup[0].resize(ow*oh);
        lookup[1].resize(ow*oh);
        lookup[2].resize(ow*oh);
        area.resize(ow*oh);
    
        for(j=0;j<nh;j++) {
          for(i=0;i<nw;i++) {
            
            fi = (float) i;
            fj = (float) j;
            x1 = fi / factor;
            x2 = (fi+1.0f) / factor;
            y1 = fj / factor;
            y2 = (fj+1.0f) / factor;
            
            di = std::sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
            
            count = 0;
            for(b=(int)y1;b<=(int)y2;b++) {
              if (b>=0 && b<oh) {
                for(a=(int)x1;a<=(int)x2;a++) {
                  if (a>=0 && a<ow) {
                    k = 3*(a+b*ow);
                    lookup[0][count]=data[k];
                    lookup[1][count]=data[k+1];
                    lookup[2][count]=data[k+2];
                    dx = (a-x1);
                    dy = (b-y1);
                    area[count] = (int) (100.0f*(di - sqrt(dx*dx+dy*dy)));
                    count++;
                  }
                }
              }
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
            dst.data[k] = R;
            dst.data[k+1] = G;
            dst.data[k+2] = B;
          }
        }
    
        return dst;
    }

    Image Image::scaleUp(float factor) const {
        Image dst;
        
        int nw,nh,ow,oh;
        
        nw = (int) ((float)(W) * factor);
        nh = (int) ((float)(H) * factor);
        ow = W;
        oh = H;
        
        dst = std::move(Image(nw,nh));
        
        for(int j=0;j<nh;j++) {
            for(int i=0;i<nw;i++) {
                
                float fi = (float) i;
                float fj = (float) j;
                
                int x = (int) (fi / factor);
                int y = (int) (fj / factor);
                
                if (x>=0 && x<ow && y>=0 && y<oh) {
                    int d = 3*(i+nw*j);
                    int s = 3*(x+ow*y);
                    dst.data[d]   = data[s];
                    dst.data[d+1] = data[s+1];
                    dst.data[d+2] = data[s+2];
                }
            }
        }

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

    RenderingContext::RenderingContext(int w,int h) {
        W = w;
        H = h;
        allocate();
        prepareFirst();
      }
    
      RenderingContext::RenderingContext(const Image &img) {
        W = img.W;
        H = img.H;
        allocate();
        prepareFirst();
      }
      
      void RenderingContext::prepareFirst() {
        std::fill(zbuf.begin(), zbuf.end(), InfZ);
        std::fill(nbuf.begin(), nbuf.end(), 0.0f);
        std::fill(rbuf.begin(), rbuf.end(), 0);
      }
    
      void RenderingContext::prepareNext() {
        std::fill(rbuf.begin(), rbuf.end(), 0);
      }
    
      void RenderingContext::clearI() {
        std::fill(ibuf.begin(), ibuf.end(), -1);
      }
    
      void RenderingContext::clearN() {
        std::fill(nbuf.begin(), nbuf.end(), 0.0f);
      }
          
      void RenderingContext::allocate() {
        zbuf.resize(W*H);
        nbuf.resize(W*H);
        xbuf.resize(W*H);
        ybuf.resize(W*H);
        rbuf.resize(W*H);
        ibuf.resize(W*H);
      }
  

} // namespace

