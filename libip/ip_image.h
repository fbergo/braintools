
#ifndef IMAGE_H
#define IMAGE_H 1

/* color image, meant to be used to render the visualization */

typedef unsigned char component;

// 1-bit transparent/opaque mask for drawing icons, letters
// and such
typedef struct _Glyph {
  int W,H;
  char *pat;
} Glyph;

Glyph *GlyphNew(int w,int h);
Glyph *GlyphNewInit(int w,int h,char *s);
void   GlyphDestroy(Glyph *g);
void   GlyphInit(Glyph *g,char *s);
void   GlyphSet(Glyph *g, int x,int y,int on);
int    GlyphGet(Glyph *g, int x,int y);

typedef struct _Font {
  int W,H,cw;
  char *pat;
} Font;

Font *FontNew(int w,int h,char *s);
void  FontDestroy(Font *f);

typedef struct _SFont {
  int W,H,bw,bh,mh;
  int cw[256],ch[256];
  component *data;
} SFont;

SFont * SFontLoad(char *file);
SFont * SFontFromFont(Font *f);
void    SFontDestroy(SFont *sf);
int     SFontLen(SFont *sf, char *text);
int     SFontH(SFont *sf, char *text);

typedef struct _ColorImage {
  int W,H,rowlen;
  component *val;
} CImg;

CImg       *CImgNew(int width, int height);
CImg       *CImgDup(CImg *src);
void        CImgDestroy(CImg *x);
void        CImgFill(CImg *x, int color);
int         CImgCompatible(CImg *a, CImg *b);
int         CImgMinSize(CImg *x, int w, int h);
int         CImgSizeEq(CImg *x,int w,int h);

void        CImgSet(CImg *z, int x,int y, int color);
int         CImgGet(CImg *z, int x,int y);

/* drawing primitives */
void        CImgFillRect(CImg *z, int x, int y, int w, int h, int color);
void        CImgDrawRect(CImg *z, int x, int y, int w, int h, int color);
void        CImgDrawLine(CImg *z, int x1, int y1, int x2, int y2, int color);
void        CImgDrawCircle(CImg *z, int cx,int cy,int r,int color);
void        CImgFillCircle(CImg *z, int cx,int cy,int r,int color);
void        CImgDrawTriangle(CImg *z, int x1,int y1,int x2,int y2,
			     int x3,int y3, int color);
void        CImgFillTriangle(CImg *z, int x1,int y1,int x2,int y2,
			     int x3,int y3, int color);

/* positive len: scale factor relative to (x1,y1) - (x2,y2)
   negative len: absolute value used as fixed arrowhead length. */
void        CImgDrawArrow(CImg *z, int x1,int y1,int x2,int y2,
			  int angle, float len, int color);


void        CImgDrawGlyph(CImg *z, Glyph *g, int x, int y, int color);
void        CImgDrawGlyphAA(CImg *z, Glyph *g, int x, int y, int color);
void        CImgDrawText(CImg *z, Font *f, int x, int y, 
			 int color, char *text);
void        CImgDrawBoldText(CImg *z, Font *f, int x, int y, 
			     int color, char *text);
void        CImgDrawShieldedText(CImg *z, Font *f, int x, int y, 
			     int color, int border, char *text);
void        CImgDrawChar(CImg *z, Font *f, int x, int y,
			 int color, unsigned char letter);
void        FontDrawCharDepth(int iw,int ih,Font *f, int x, int y,
			      unsigned char letter,
			      float *depth,float dval);

void        CImgDrawSText(CImg *z, SFont *f, int x, int y, 
			 int color, char *text);
void        CImgDrawBoldSText(CImg *z, SFont *f, int x, int y, 
			     int color, char *text);
void        CImgDrawShieldedSText(CImg *z, SFont *f, int x, int y, 
			     int color, int border, char *text);
void        CImgDrawSChar(CImg *z, SFont *f, int x, int y,
			  int color, unsigned char letter);
void        SFontDrawCharDepth(int iw,int ih,SFont *f, int x, int y,
			       unsigned char letter,
			       float *depth,float dval);

void        CImgDrawBevelBox(CImg *z, int x,int y,int w,int h, int color, 
			     int border, int up);
void        CImgDrawButton(CImg *z, int x,int y,int w,int h, int bg,
			   int border,int textcolor, char *text,
			   Font *font, int up);
void        CImgDrawSButton(CImg *z, int x,int y,int w,int h, int bg,
			    int border,int textcolor, char *text,
			    SFont *font, int up);

void        CImgFullCopy(CImg *dest, CImg *src);
void        CImgBitBlt(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
		       int w, int h);
void        CImgBitBlto(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
			int w, int h);

// ratio = 1.0 src only (BitBlt), 0.0 dest only (nop)
void        CImgBitBlm(CImg *dest, int dx, int dy, CImg *src, int sx, int sy,
		       int w, int h, float ratio);


void        CImgSetTransparentColor(CImg *dest, int color);
void        CImgSubst(CImg *dest, int dc, int sc);
void        CImgGray(CImg *dest);

void        CImgBrightnessContrast(CImg *dest, CImg *src, 
				   float bri,float con, int bgcolor);

CImg       *CImgIntegerZoom(CImg *src, int zoom);
CImg       *CImgHalfScale(CImg *src);
CImg       *CImgScale(CImg *src, float factor);
CImg       *CImgFastScale(CImg *src, float factor);
CImg       *CImgScaleToWidth(CImg *src, int width);
CImg       *CImgScaleToHeight(CImg *src, int height);
CImg       *CImgScaleToFit(CImg *src, int maxdim);

CImg       *CImgReadP6(char *path);
int         CImgWriteP6(CImg *src, char *path);
int         CImgWriteP5(CImg *src, char *path);
void        CImgXOR(CImg *dest, CImg *src);


/* 180 deg rotation, in place */
void        CImgRot180(CImg *src);

/* simple run-length compression for keeping animation sequences in
   memory - array of 32-bit ints of the form <count><r><g><b> (8 bit each) 
*/

typedef struct _RLEImage {
  int W,H;
  component *data;
  int streamlen;
} RLEImg;

RLEImg * RLENewFromStream(char *stream, int W, int H);
RLEImg * RLENewFromBRV(char *filename);
RLEImg * EncodeRLE(CImg *src);
void     RLEDestroy(RLEImg *rle);
CImg   * DecodeRLE(RLEImg *src);

/* lossy compression (fixed, 66%): 
   converts 24-bit RGB to 16-bit YCbCr (8-4-4) */

typedef RLEImg LossyImg;

#define LossyDestroy    RLEDestroy
#define LossyNewFromBRV RLENewFromBRV
CImg     * DecodeLossy(LossyImg *src);
LossyImg * EncodeLossy(CImg *src);

void RGBMerge(component *d, component *s, int w, float ratio);
void RGBOverlay(component *d, component *s, int w);

char *   Stream(char *data, int size);
char *   Unstream(char *stream, int *sz);

#endif
