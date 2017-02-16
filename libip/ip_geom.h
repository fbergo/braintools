
#ifndef GEOM_H
#define GEOM_H 1

#define FORCE_RANGE(a,b,c) (((a)<(b))?(b):(((a)>(c))?(c):(a)))

typedef struct _point {
  float x,y,z;
} Point;
// -- dt end

typedef struct _transform {
  float e[3][3];  
} Transform;

typedef struct _htransform {
  float e[3][4];
} HTrans;

typedef struct _point     Vector;

typedef struct _face {
  Point *p[4];
} Face;

typedef struct _cuboid {
  Point p[8];
  Face  f[6];
  int   hidden[6]; /* 1 if face is hidden */
} Cuboid;

typedef struct _triangle {
  Point p[3];
} Triangle;

Point * PointNew();
void    PointCopy(Point *dest,Point *src);
void    PointAssign(Point *p, float x, float y, float z);
void    PointTransform(Point *dest,Point *src, Transform *t);
void    PointTranslate(Point *p, float dx,float dy,float dz);
void    PointDestroy(Point *p);
void    PointHTrans(Point *dest,Point *src, HTrans *t);

Transform * TransformNew();
void        TransformCopy(Transform *dest,Transform *src);
void        TransformDestroy(Transform *t);
void        TransformIdentity(Transform *t);
void        TransformZero(Transform *t);
void        TransformXRot(Transform *t, float angle);
void        TransformYRot(Transform *t, float angle);
void        TransformZRot(Transform *t, float angle);
void        TransformIsoScale(Transform *t, float factor);
void        TransformScale(Transform *t, float sx, float sy, float sz);
void        TransformCompose(Transform *dest, Transform *src);
int         TransformInvert(Transform *dest, Transform *src); // 0==success

void        TriangleTransform(Triangle *dest, Triangle *src, Transform *T);
void        TriangleTranslate(Triangle *src,  float dx, float dy, float dz);

HTrans     *HTransNew();
void        HTransCopy(HTrans *dest, HTrans *src);
void        HTransDestroy(HTrans *t);
void        HTransIdentity(HTrans *t);
void        HTransZero(HTrans *t);
void        HTransXRot(HTrans *t, float angle);
void        HTransYRot(HTrans *t, float angle);
void        HTransZRot(HTrans *t, float angle);
void        HTransIsoScale(HTrans *t, float factor);
void        HTransScale(HTrans *t, float sx,float sy,float sz);
void        HTransTrans(HTrans *t, float dx,float dy,float dz);
void        HTransCompose(HTrans *dest, HTrans *src);
int         HTransInvert(HTrans *dest, HTrans *src); // 0==success

float      sgn(float v);
int        isgn(int v);
float      det3(float a,float b,float c,float d,float e,float f,
		 float g,float h,float i);
int        det3i(int a,int b,int c,int d,int e,int f,int g,int h,int i);

// tests 2D point, 0 if out, 1 if in
int         FacePointTest(Face *f, Point *p);
float       FaceMaxZ(Face *f);
void        FaceAssign(Face *f, Point *p0, Point *p1, Point *p2, Point *p3);

Cuboid     *CuboidNew();
void        CuboidDestroy(Cuboid *c);
void        CuboidCopy(Cuboid *dest, Cuboid *src);
void        CuboidOrthogonalInit(Cuboid *c,
				 float x0,float x1,
				 float y0,float y1,
				 float z0,float z1);
void        CuboidTransform(Cuboid *c, Transform *t);
void        CuboidTranslate(Cuboid *c, float dx, float dy, float dz);

// assumes orthogonal observer at <0,0,-Inf>
void        CuboidRemoveHiddenFaces(Cuboid *c);

// vectorial math

float   VectorLength(Vector *v);
void    VectorAdd(Vector *dest,Vector *src);
void    VectorCrossProduct(Vector *dest,Vector *a,Vector *b);
void    VectorScalarProduct(Vector *dest, float factor);

#define VectorAssign PointAssign
#define VectorCopy   PointCopy

float   VectorInnerProduct(Vector *a, Vector *b);
void    VectorNormalize(Vector *a);
int     VectorEqual(Vector *a,Vector *b);

/* checks whether <x,y> is inside rectangle <x0,y0,w,h> */
int inbox(int x,int y,int x0,int y0,int w,int h);
int box_intersection(int x1,int y1,int w1,int h1,
		     int x2,int y2,int w2,int h2);

/* normalization */

int   IntNormalize(int value,int omin,int omax,int nmin,int nmax);
float FloatNormalize(float value,float omin,float omax,float nmin, float nmax);

/* wiremodel of an abacus */

typedef struct _wire {
  int npoints; // 2-4
  int fill;
  Point p[8];  
} Wire;

typedef struct _wiremodel {
  int nfaces;
  Wire *faces;
} WireModel;

WireModel *Abacus();

#endif
