
#ifndef VOLUME_H
#define VOLUME_H 1

#include <stdio.h>
#include <zlib.h>
#include <ip_set.h>
#include <ip_adjacency.h>

#define HIGH_S16      32767
#define HIGH_U16      65535
#define HIGH_S32 2000000000
#define HIGH_U8         255
#define HIGH_S8         127

#define SMAX 32767 /* max intensity for processing */
#define FMAX 32766 /* max intensity for filter output */
#define SDOM 32768 /* lookup table sizes */
#define VMAX 255   /* max intensity for visualization */

#define REMOVEMASK 0x80
#define LABELMASK  0x7F

#define FMT_UNKNOWN 0
#define FMT_VGM     1
#define FMT_SCN     2
#define FMT_P2      3 
#define FMT_P5      4
#define FMT_HDR     5 /* analyze */

typedef enum {
  vt_integer_8,
  vt_integer_16,
  vt_integer_32,
  vt_float_32,
  vt_float_64
} voltype;

typedef unsigned char      i8_t;
typedef signed short int   i16_t;
typedef unsigned short int u16_t;
typedef signed int         i32_t;
typedef float              f32_t;
typedef double             f64_t;

typedef struct _volume {
  int     N, W,H,D, WxH;
  int     *tbrow;
  int     *tbframe;
  voltype Type;

  /* this is what val[i] would be, use u.i16[i] for
     16-bit volumes, for example */
  union {
    i8_t      *i8;
    i16_t     *i16;
    u16_t     *u16;
    i32_t     *i32;
    f32_t     *f32;
    f64_t     *f64;
  } u;

  float   dx,dy,dz;
} Volume;

/* for clarity, whenever needed */
typedef Volume Volume8;
typedef Volume Volume16;
typedef Volume Volume32;
typedef Volume VolumeF32;
typedef Volume VolumeF64;

#define val32(v,i) ((v)->u.i32[(i)])
#define val16(v,i) ((v)->u.i16[(i)])
#define val8(v,i) ((v)->u.i8[(i)])

typedef struct _vgmstat {
  int W,H,D;
  int valid;
  int success;
  int error;
  int size;
} VGMStat;

typedef short int     ctype;
typedef short int     vtype;
typedef unsigned char ltype;

typedef struct _evoxel {
  ctype     cost;
  vtype     orig;
  vtype     value;
  ltype     label;
  ltype     pred;
} evoxel;

struct _xannvolume {
  int W,H,D,N,WxH;

  evoxel *vd;

  int    *tbrow;
  int    *tbframe;

  Map    *addseeds;
  Map    *delseeds;
  AdjRel *predadj;

  float   dx,dy,dz;

  void   *extra; /* pointer to special optional data,
		    such as fuzzy connectedness membership data */
  char    extra_is_freeable;
};

typedef struct _xannvolume XAnnVolume;

struct _voxel {
  int x,y,z;
};

typedef struct _voxel Voxel;

/* File Format Guess */
int     GuessFileFormat(char *path);

/* Volume Creation */
Volume *VolumeNew(int W,int H,int D, voltype type);
void    VolumeDestroy(Volume *v);

void    VolumeFill(Volume *v, int c);
void    VolumeFillF(Volume *v, double c);

i8_t    VolumeGetI8(Volume *v, int x,int y, int z);
i16_t   VolumeGetI16(Volume *v, int x,int y, int z);
i32_t   VolumeGetI32(Volume *v, int x,int y, int z);
f32_t   VolumeGetF32(Volume *v, int x,int y, int z);
f64_t   VolumeGetF64(Volume *v, int x,int y, int z);

void    VolumeSetI8(Volume *v, int x,int y, int z, i8_t c);
void    VolumeSetI16(Volume *v, int x,int y, int z, i16_t c);
void    VolumeSetI32(Volume *v, int x,int y, int z, i32_t c);
void    VolumeSetF32(Volume *v, int x,int y, int z, f32_t c);
void    VolumeSetF64(Volume *v, int x,int y, int z, f64_t c);

int     VolumeValid(Volume *v, int x, int y, int z);
int     ValidVoxel(Volume *v, Voxel *e);
void    VolumeVoxelOf(Volume *v, int n, Voxel *out);

#define VOXELX(N,W,WH) (((N) % (WH)) % (W))
#define VOXELY(N,W,WH) (((N) % (WH)) / (W))
#define VOXELZ(N,W,WH) ((N) / (WH))

int     VolumeSaveSCN(Volume *v, char *path);
int     VolumeSaveSCN8(Volume *v, char *path);
int     VolumeSaveSCN32(Volume *v, char *path);

Volume *VolumeNewFromFile(char *path);
Volume *VolumeNewAsClip(Volume *src, 
			int x0,int x1,int y0,int y1,
			int z0,int z1);

int     VolumeViewShift(Volume *v);
int     VolumeMax(Volume *v);

Volume *VolumeInterpolate(Volume *src,float dx, float dy, float dz);

void    VolumeYFlip(Volume *v);

void VGMFileStat(char *path, VGMStat *out);

int IsVLAFile(char *path);
int IsMVFFile(char *path);

/* XAnnVolume (annotated volume) operations */

XAnnVolume * XAVolNew(int W,int H, int D);
XAnnVolume * XAVolNewWithVolume(Volume *v);
void         XAVolDestroy(XAnnVolume *v);
void         XAVolReset(XAnnVolume *v);

void         XAVolSave(XAnnVolume *v, char *name);
XAnnVolume * XAVolLoad(char *name);

Volume     * XAVolToVol(XAnnVolume *v, 
			int x0,int x1,
			int y0,int y1,
			int z0,int z1);

int         XAVolGetRootXYZ(XAnnVolume *v, int x,int y,int z);
int         XAVolGetRootN(XAnnVolume *v, int n);
void        XAVolTagRemoval(XAnnVolume *v, int n);
void        XAVolUntagRemoval(XAnnVolume *v, int n);
void        XAVolToggleRemoval(XAnnVolume *v, int n);

int         XAValidVoxel(XAnnVolume *v, Voxel *p);
int         XAVolumeValid(XAnnVolume *v, int x, int y, int z);
void        XAVolumeVoxelOf(XAnnVolume *v, int n, Voxel *out);

ltype       XAGetLabel(XAnnVolume *v, int n);
vtype       XAGetOrig(XAnnVolume *v, int n);
vtype       XAGetValue(XAnnVolume *v, int n);
ctype       XAGetCost(XAnnVolume *v, int n);

int         XAGetDecPredecessor(XAnnVolume *v, int n);

void        XASetLabel(XAnnVolume *v, int n, ltype value);

int         XAGetOrigViewShift(XAnnVolume *v);
int         XAGetValueViewShift(XAnnVolume *v);
int         XAGetCostViewShift(XAnnVolume *v);

int         XAGetOrigMax(XAnnVolume *v);
int         XAGetValueMax(XAnnVolume *v);
int         XAGetCostMax(XAnnVolume *v);

/* structuring elements (8-bit volumes) */

Volume *SENew(int w,int h,int d);
void    SEDestroy(Volume *se);

Volume *SECross(int size);
Volume *SEBox(int size);
Volume *SELine(int size,int direction);

void SEOr(Volume *dest, Volume *src);
void SEAnd(Volume *dest, Volume *src);

int nc_fgets(char *s, int m, FILE *f);
int nc_gzgets(char *s, int m, gzFile f);

#endif
