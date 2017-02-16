
#ifndef SCENE_H
#define SCENE_H 1

#include <ip_geom.h>
#include <ip_volume.h>
#include <ip_set.h>

#define SCHEME_DEFAULT      0
#define SCHEME_BONEFLESH    1
#define SCHEME_GSBLACK      2
#define SCHEME_GSWHITE      3

#define VMAP_GRAY           0
#define VMAP_HOTMETAL       1
#define VMAP_SPECTRUM       2
#define VMAP_SPECTRUM_B     3
#define VMAP_YELLOW_RED     4
#define VMAP_NEON           5

#define MIPSRC_ORIG    0
#define MIPSRC_CURR    1
#define MIPSRC_COST    2
#define MIPSRC_OVERLAY 3

typedef struct _viewmap {
  
  float          maxval;     /* true maximum in the 16-bit range */
  float          maxlevel;   /* relative pseudo-maximum [0..1] */
  float          minlevel;   /* relative pseudo-minimum [0..1] */

  unsigned char *map;        /* 32768-position palette map */
  int            palette[256]; /* the screen color palette */
} ViewMap;

ViewMap *ViewMapNew();
void     ViewMapDestroy(ViewMap *vmap);
void     ViewMapLoadPalette(ViewMap *vmap, int p);
void     ViewMapUpdate(ViewMap *vmap);
void     ViewMapCopy(ViewMap *dest, ViewMap *src);

#define ViewMapColor(vmap, val) ((vmap)->palette[(vmap)->map[(val)]])

typedef struct _clipping {
  int v[3][2];  /* clipping bounds for X,Y,Z */
} Clipping;


typedef enum {
  shading_none          = 0,
  shading_lin_z_dist    = 1,
  shading_quad_z_dist   = 2,
  shading_normal        = 3,
  shading_quad_z_normal = 4,
  shading_phong         = 5,
  shading_smooth_phong  = 6,
  shading_grad_phong    = 7,
  shading_phong_sq      = 8, /* use cos^2 instead of cos */
  shading_phong_cube    = 9, /* use cos^3 instead of cos */
  shading_phong_cartoon = 10, /* quantize normals to 4 levels */
  shading_auto          = 99,
} ShadingType;

typedef enum {
  zoom_50  = -1,
  zoom_100 = 0,
  zoom_150 = 1,
  zoom_200 = 2,
  zoom_300 = 3,
  zoom_400 = 4,
} ZoomType;

typedef enum {
  vmode_src        = 0, /* no transparency */
  vmode_label      = 1, /* label color for non-hollow objects */
  vmode_srclabel   = 2, /* mix of src and label color for non-hollow objects */
  vmode_srcborder  = 3, /* src with border highlight, for non-hollow objects */
  vmode_srctrees   = 4, /* src with tree border highlight,for non-hollow objs*/
  vmode_seethru    = 5, /* srclabel with all non-hollow objects rendered */
  vmode_paths      = 6, /* ift path lines */
} VModeType;

typedef enum {
  src_orig       = 0,
  src_value      = 1,
  src_cost       = 2,
} SrcType;

typedef struct _phong_parameters {
  float  ambient;
  float  diffuse;
  float  specular;
  float  zgamma;
  float  zstretch;
  int    sre;      /* specular reflection exponent (Foley, pg.729) */
  Vector light;
} PhongParameters;

typedef struct _rendering_options {
  SrcType            src;
  VModeType          vmode;
  ZoomType           zoom;
  ShadingType        shading;
  PhongParameters    phong;
  float              srcmix; /* 0.0 = all label, 1.0 = all source/pert */
} RenderingOptions;

typedef struct _scenecolors {
  int  bg;
  int  clipcube, fullcube;
  int  label[256];
  char whiteborder;
} SceneColors;

typedef struct _scene_pvt {
  Vector           rotation;
  Clipping         clip;
  RenderingOptions popts;
  SceneColors      pcolors;
  char             hollow[256];
  int              minz, maxz;
  int              outw, outh;
  int              oxc,  oyc;
  Volume          *vinfo;
  Volume          *xyzbuffer;  
  Volume          *surface;
} ScenePvt;

/* new idea (May 2003) : create a polygon mesh of
   the last rendering, in background */
typedef struct _smesh {
  int maxtriangles, hgrid, vgrid;
  int ntriangles;
  int validmesh;

  Triangle *srct, *tmpt;
  int      *tsort;
  float    *tz;
  ltype    *tl;
} SceneMesh;

typedef struct _scene {
  XAnnVolume      *avol; /* the input volume */
  CImg            *vout; /* the output view */

  /* parameters that shape how the view is rendered */
  Vector           rotation;
  Clipping         clip;
  RenderingOptions options;
  SceneColors      colors;
  char             hollow[256];

  int              invalid; /* set to 1 to force re-rendering */

  /* private, please don't mess with it outside this lib */
  ScenePvt         pvt;

  /* new: polygon-mesh approximation of last rendering */
  SceneMesh        amesh;
  int              wantmesh;

  /* new: view range */
  ViewMap         *vmap;

  /* new: cost threshold / onion */
  int              with_restriction; /* 0=none, 1=cost, 2=mask */
  int              cost_threshold_min;
  int              cost_threshold_max;
  BMap            *restriction;

  ShadingType      eshade; /* effective shading */

  /* new: mip overlay */
  int              enable_mip;
  int              mip_lighten_only;
  float            mip_ramp_min;
  float            mip_ramp_max;
  Volume           *mipvol;
  int              mip_source;

} Scene;

typedef struct _roi {
  char labelroi[256]; /* 1 means voxels with that label are in the ROI */
  char clip_only;     /* 1 means voxels out of the clip are not in the ROI */
} ROI;

Scene  *SceneNew();
void    SceneCopy(Scene *dest, Scene *src);
void    SceneDestroy(Scene *s);

void    SceneLoadColorScheme(Scene *scene, int scheme);
void    SceneRandomColorScheme(Scene *scene);
void    SceneValidate(Scene *s);
void    SceneCheckValidity(Scene *s);

void    SceneSetVolume(Scene *scene, XAnnVolume *v);
void    ScenePrepareImage(Scene *scene, int w, int h);

int     SceneX(Scene *scene, int x);
int     SceneY(Scene *scene, int y);
int     SceneW(Scene *scene, int w);
int     SceneH(Scene *scene, int h);
int     SceneInvX(Scene *scene, int x);
int     SceneInvY(Scene *scene, int y);
int     ScenePixel(Scene *scene);
double  SceneScale(Scene *scene);

int     SceneVoxelAt(Scene *scene, int x, int y);
int     SceneVoxelAtT(Scene *scene, int x, int y);

int     SceneMarkSeedOnProjection(Scene *scene, int x,int y, int label);
int     SceneMarkSeedLineOnProjection(Scene *scene, int x0,int y0, 
				      int x1, int y1,int label);
int     SceneMarkTreeOnProjection(Scene *scene, int x,int y);

int     SceneMarkSeed(Scene *scene, int x, int y, int z, int label);
int     SceneMarkSeedLine(Scene *scene, int x0, int y0, int z0, 
			  int x1,int y1,int z1, int label);
int     SceneMarkTree(Scene *scene, int x,int y,int z);

void    SceneSaveView(Scene *scene, char *path);
void    SceneLoadView(Scene *scene, char *path);

void    SceneInitAMesh(Scene *scene, int hgrid, int vgrid);
void    SceneShrinkAMesh(Scene *scene);

#endif
