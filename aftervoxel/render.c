
#define SOURCE_RENDER_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libip.h>
#include "aftervoxel.h"

struct {
  int nx[128], ny[128];
  int npoints;
  int radius;
} smooth;

void render_calc_normal(int x,int y,int rw,
			float *xb,float *yb,float *zb,float *out);

void render_prepare_smoothing(int level);
void render_calc_smooth_normal(int x,int y,int rw,
			       float *xb,float *yb,float *zb,float *out);

void render_calc_quick_normal(int x,int y,int rw,
			      float *xb,float *yb,float *zb,float *out);

void render_threshold(CImg *dest, Transform *rot, float panx, float pany,
		      int W,int H,int D, int *map, int mapcount,
		      float *depth, float *depthdual, int color, 
		      int smoothness)
{
  int w,h,w2,h2,W2,H2,D2,WH;
  int i,j,k,n,p,q;
  float *xb,*yb,*normals;
  float fa,fb;
  Point A,B;
  float diag,dz,kz,z,Y;
  int ca;
  int *pm;

  int px,py,pz;

  int spx[3] = {1,0,1};
  int spy[3] = {0,1,1};

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  WH = W*H;

  VectorNormalize(&(v3d.light));

  /* init z-buffer and float (x,y) buffers (for normals) */
  xb = (float *) MemAlloc( w * h * sizeof(float) );
  yb = (float *) MemAlloc( w * h * sizeof(float) );
  normals = (float *) MemAlloc( w * h * sizeof(float) );

  if (!xb || !yb || !normals) {
    error_memory();
    return;
  }

  FloatFill(depth, MAGIC, w*h);
  FloatFill(depthdual, -MAGIC, w*h);
  FloatFill(xb, 0.0, w*h);
  FloatFill(yb, 0.0, w*h);
  FloatFill(normals, 0.0, w*h);

  /* make projection */

  pm = map;
  for(i=0;i<mapcount;i++,pm++) {

    n = *pm;
    pz = n / WH;
    py = (n % WH) / W;
    px = (n % WH) % W;

    A.x = px - W2;
    A.y = py - H2;
    A.z = pz - D2;

    PointTransform(&B,&A,rot);
    px = w2 + B.x + panx;
    py = h2 + B.y + pany;

    if (px < 0 || py < 0 || px >= w || py >= h) continue;

    p = px + py*w;

    if (B.z < depth[p]) {
      depth[p] = B.z;
      xb[p] = B.x;
      yb[p] = B.y;
    }

    if (B.z > depthdual[p])
      depthdual[p] = B.z;
  }

  /* forward-only 2x2 splatting */

  for(j=h-2;j>=0;j--)
    for(i=w-2;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(k=0;k<3;k++) {
	  fb = depth[q = p + spx[k] + w*spy[k]];
	  if (fa < fb) {
	    depth[q] = fa;
	    xb[q] = xb[p] + spx[k];
	    yb[q] = yb[p] + spy[k];
	  }
	}
      }
    }

  /* compute normals */

  if (smoothness <= 1) {
    for(j=2;j<h-2;j++)
      for(i=2;i<w-2;i++)
	if (depth[i+j*w] != MAGIC)
	  render_calc_normal(i,j,w,xb,yb,depth,normals);
  } else {
    render_prepare_smoothing(smoothness);
    for(j=smooth.radius;j<h-smooth.radius;j++)
      for(i=smooth.radius;i<w-smooth.radius;i++)
	if (depth[i+j*w] != MAGIC)
	  render_calc_smooth_normal(i,j,w,xb,yb,depth,normals);
  }

  /* render the monster */

  A.x = W;
  A.y = H;
  A.z = D;
  diag = VectorLength(&A) / 2.0;

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;

      if (depth[p] != MAGIC) {
	z = depth[p];
	dz = (diag - z);
	kz = (dz*dz) / (4.0*diag*diag);

	kz = v3d.zgamma + v3d.zstretch * kz;

	fa = normals[p];
	fb = phong_specular(fa, v3d.sre);
	
	ca=RGB2YCbCr(color);
	Y = (float) t0(ca);
	Y /= 255.0;
	
	Y = v3d.ka + kz * ( v3d.kd * Y * fa + v3d.ks * Y * fb);
	Y *= 255.0;
	if (Y > 255.0) Y=255.0;
	if (Y < 0.0) Y=0.0;
	ca = triplet((int) Y, t1(ca), t2(ca));
	ca = YCbCr2RGB(ca);
	CImgSet(dest,i,j,ca);
      }
    }
      
  MemFree(normals);
  MemFree(xb);
  MemFree(yb);
}

void render_quick_threshold(CImg *dest, Transform *rot, float panx, float pany,
			    int W,int H,int D, int *map, int mapcount,
			    float *depth, float *depthdual,int color)
{
  int w,h,w2,h2,W2,H2,D2,WH;
  int i,j,n,p,q;
  float fa,fb,*xb,*yb,*normals;
  Point A,B;
  float diag,dz,kz,z,Y;
  int ca;
  int *pm;

  int px,py,pz;

  int kx,ky;

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  WH = W*H;

  VectorNormalize(&(v3d.light));

  /* init z-buffer and float (x,y) buffers (for normals) */
  xb = (float *) MemAlloc( w * h * sizeof(float) );
  yb = (float *) MemAlloc( w * h * sizeof(float) );
  normals = (float *) MemAlloc( w * h * sizeof(float) );

  if (!xb || !yb || !normals) {
    error_memory();
    return;
  }

  FloatFill(depth, MAGIC, w*h);
  FloatFill(depthdual, -MAGIC, w*h);
  FloatFill(xb, 0.0, w*h);
  FloatFill(yb, 0.0, w*h);
  FloatFill(normals, 0.0, w*h);

  /* make projection */

  pm = map;
  for(i=0;i<mapcount;i+=30,pm+=30) {

    n = *pm;
    pz = n / WH;
    py = (n % WH) / W;
    px = (n % WH) % W;

    A.x = px - W2;
    A.y = py - H2;
    A.z = pz - D2;

    PointTransform(&B,&A,rot);
    px = w2 + B.x + panx;
    py = h2 + B.y + pany;

    if (px < 0 || py < 0 || px >= w || py >= h) continue;

    p = px + py*w;

    if (B.z < depth[p]) {
      depth[p] = B.z;
      xb[p] = B.x;
      yb[p] = B.y;
    }

    if (B.z > depthdual[p])
      depthdual[p] = B.z;
  }

  /* forward-only 12x12 splatting */

  for(j=h-12;j>=0;j--)
    for(i=w-12;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(ky=0;ky<12;ky++) {
	  for(kx=0;kx<12;kx++) {
	    fb = depth[q = p + kx + w*ky];
	    if (fa < fb) {
	      depth[q] = fa;
	      xb[q] = i+kx;
	      yb[q] = j+ky;
	    }
	  }
	}
      }
    }

  /* compute normals */

  for(j=12;j<h-12;j++)
    for(i=12;i<w-12;i++)
      if (depth[i+j*w] != MAGIC)
	render_calc_quick_normal(i,j,w,xb,yb,depth,normals);

  /* render the monster */

  A.x = W;
  A.y = H;
  A.z = D;
  diag = VectorLength(&A) / 2.0;

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;

      if (depth[p] != MAGIC) {
	z = depth[p];
	dz = (diag - z);
	kz = (dz*dz) / (4.0*diag*diag);

	kz = v3d.zgamma + v3d.zstretch * kz;

	fa = normals[p];
	fb = phong_specular(fa, v3d.sre);
	
	ca=RGB2YCbCr(color);
	Y = (float) t0(ca);
	Y /= 255.0;
	
	Y = v3d.ka + kz * ( v3d.kd * Y * fa + v3d.ks * Y * fb);
	Y *= 255.0;
	if (Y > 255.0) Y=255.0;
	if (Y < 0.0) Y=0.0;
	ca = triplet((int) Y, t1(ca), t2(ca));
	ca = YCbCr2RGB(ca);
	CImgSet(dest,i,j,ca);
      }
    }
  MemFree(normals);
  MemFree(xb);
  MemFree(yb);
}

void render_calc_quick_normal(int x,int y,int rw,
			      float *xb,float *yb,float *zb,float *out)
{
  int nx[8] = {  0,  12,  12,  12,  0, -12, -12, -12 };
  int ny[8] = { -12, -12,  0,  12,  12,  12,  0, -12 };

  unsigned int i;
  int k, nv, a;
  float est;
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  nv=0;
  k = 1;
  for(i=0;i<8;i++) {

    a = x + y*rw;
    if (zb[a] == MAGIC) continue;
    p[0].x = xb[a];   p[0].y = yb[a];   p[0].z = zb[a];

    a = (x+nx[(i+k)%8]) + (y+ny[(i+k)%8])*rw;
    if (zb[a] == MAGIC) continue;
    p[1].x = xb[a];   p[1].y = yb[a];   p[1].z = zb[a];

    a = (x+nx[i]) + (y+ny[i])*rw;
    if (zb[a] == MAGIC) continue;
    p[2].x = xb[a];   p[2].y = yb[a];   p[2].z = zb[a];

    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) {
    est = 1.0;
  } else {
    VectorNormalize(&normal);
    est = VectorInnerProduct(&(v3d.light), &normal);
    if (est < 0.0) est=0.0;
  }

  a = x + rw*y;
  out[a] = est;
}

void render_prepare_smoothing(int level) {
  float r,ang;
  int i;

  r = (float) level;

  smooth.npoints = 8;

  ang = 360.0 / ((float) (smooth.npoints));

  for(i=0;i<smooth.npoints;i++) {
    smooth.nx[i] = (int) (r * cos( i*ang*M_PI / 180.0 ));
    smooth.ny[i] = (int) (r * sin( i*ang*M_PI / 180.0 ));
  }

  smooth.radius = MAX(2,level);
}

void render_calc_s6_normal(int x,int y,int rw,float *zb,float *out,
			   Vector *light) {
  unsigned int i;
  int k, nv, a;
  float est;
  int ax,ay;
  int nx[8] = {  0,  4,  6,  4,  0,  -4,  -6,  -4};
  int ny[8] = { -6, -4,  0,  4,  6,   4,   0,  -4};
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  nv=0;
  k = 1;
  for(i=0;i<8;i++) {

    a = x + y*rw;
    if (zb[a] == MAGIC) continue;
    p[0].x = x;   p[0].y = y;   p[0].z = zb[a];

    ax = nx[(i+k)%8];
    ay = ny[(i+k)%8];
    do {
      a = x+ax + (y+ay)*rw;

      if (zb[a]==MAGIC) {
	ax /= 2;
	ay /= 2;
      }
    } while(zb[a]==MAGIC && (ax!=0 || ay!=0));
    if (zb[a] == MAGIC) continue;

    p[1].x = x+ax;   p[1].y = y+ay;   p[1].z = zb[a];

    ax = nx[i];
    ay = ny[i];
    do {
      a = x+ax + (y+ay)*rw;
      
      if (zb[a] == MAGIC) {
	ax /= 2;
	ay /= 2;
      }

    } while(zb[a]==MAGIC && (ax!=0 || ay!=0));
    if (zb[a] == MAGIC) continue;

    p[2].x = x+ax;   p[2].y = y+ay;   p[2].z = zb[a];

    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) {
    est = 1.0;
  } else {
    VectorNormalize(&normal);
    est = VectorInnerProduct(light, &normal);
    if (est < 0.0) est=0.0;
  }

  a = x + rw*y;
  out[a] = est;
}

void render_calc_smooth_normal(int x,int y,int rw,
			       float *xb,float *yb,float *zb,float *out)
{
  unsigned int i;
  int k, nv, a;
  float est;
  int ax,ay;
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  nv=0;
  k = 1;
  for(i=0;i<smooth.npoints;i++) {

    a = x + y*rw;
    if (zb[a] == MAGIC) continue;
    p[0].x = xb[a];   p[0].y = yb[a];   p[0].z = zb[a];

    ax = smooth.nx[(i+k)%(smooth.npoints)];
    ay = smooth.ny[(i+k)%(smooth.npoints)];
    do {
      a = x+ax + (y+ay)*rw;

      if (zb[a]==MAGIC) {
	ax /= 2;
	ay /= 2;
      }
    } while(zb[a]==MAGIC && (ax!=0 || ay!=0));
    if (zb[a] == MAGIC) continue;

    p[1].x = xb[a];   p[1].y = yb[a];   p[1].z = zb[a];

    ax = smooth.nx[i];
    ay = smooth.ny[i];
    do {
      a = x+ax + (y+ay)*rw;
      
      if (zb[a] == MAGIC) {
	ax /= 2;
	ay /= 2;
      }

    } while(zb[a]==MAGIC && (ax!=0 || ay!=0));
    if (zb[a] == MAGIC) continue;

    p[2].x = xb[a];   p[2].y = yb[a];   p[2].z = zb[a];

    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) {
    est = 1.0;
  } else {
    VectorNormalize(&normal);
    est = VectorInnerProduct(&(v3d.light), &normal);
    if (est < 0.0) est=0.0;
  }

  a = x + rw*y;
  out[a] = est;
}

void render_calc_normal(int x,int y,int rw,
			float *xb,float *yb,float *zb,float *out)
{
  int nx[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
  int ny[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

  int qx[8] = {  1,  2,  2,  1, -1, -2, -2, -1 };
  int qy[8] = { -2, -1,  1,  2,  2,  1, -1, -2 };

  unsigned int i;
  int k, nv, a;
  float est;
  
  Vector p[3], w[3], normal = {0.0, 0.0, 0.0};

  nv=0;
  k = 1;
  for(i=0;i<8;i++) {

    a = x + y*rw;
    if (zb[a] == MAGIC) continue;
    p[0].x = xb[a];   p[0].y = yb[a];   p[0].z = zb[a];

    a = (x+nx[(i+k)%8]) + (y+ny[(i+k)%8])*rw;
    if (zb[a] == MAGIC) continue;
    p[1].x = xb[a];   p[1].y = yb[a];   p[1].z = zb[a];

    a = (x+nx[i]) + (y+ny[i])*rw;
    if (zb[a] == MAGIC) continue;
    p[2].x = xb[a];   p[2].y = yb[a];   p[2].z = zb[a];

    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  for(i=0;i<8;i++) {

    a = x + y*rw;
    if (zb[a] == MAGIC) continue;
    p[0].x = xb[a];   p[0].y = yb[a];   p[0].z = zb[a];

    a = (x+qx[(i+k)%8]) + (y+qy[(i+k)%8])*rw;
    if (zb[a] == MAGIC) continue;
    p[1].x = xb[a];   p[1].y = yb[a];   p[1].z = zb[a];

    a = (x+qx[i]) + (y+qy[i])*rw;
    if (zb[a] == MAGIC) continue;
    p[2].x = xb[a];   p[2].y = yb[a];   p[2].z = zb[a];
    
    w[0].x = p[1].x - p[0].x;
    w[0].y = p[1].y - p[0].y;
    w[0].z = p[1].z - p[0].z;
    
    w[1].x = p[2].x - p[0].x;
    w[1].y = p[2].y - p[0].y;
    w[1].z = p[2].z - p[0].z;
    
    VectorCrossProduct(&w[2], &w[0], &w[1]);
    VectorNormalize(&w[2]);
    
    ++nv;
    VectorAdd(&normal, &w[2]);
  }

  if (!nv) {
    est = 1.0;
  } else {
    VectorNormalize(&normal);
    est = VectorInnerProduct(&(v3d.light), &normal);
    if (est < 0.0) est=0.0;
  }

  a = x + rw*y;
  out[a] = est;
}

float phong_specular(float angcos, int n) {
  float a,r;

  a=acos(angcos);
  if (a > M_PI / 4.0)
    return 0.0;

  a=cos(2.0 * a);
  r=a;
  while(n!=1) { r*=a; --n; }
  return r;
}

/* compute v3d.skin.tmap if needed */
/* call render_threshold */
void render_skin(void *args) {
  int mapsize;
  int count;

  Volume *vol;
  int W,H,D,WH;
  int i,j,k, n, *map;
  i16_t tval;

  v3d.skin.ready = 0;
  vol = voldata.original;
  if (vol==NULL) return;
  
  /* compute pseudo-shell */
  if (!v3d.skin.tmap) {
    mapsize=8192;
    map = v3d.skin.tmap = (int *) MemAlloc(sizeof(int) * mapsize);

    if (!map) {
      error_memory();
      return;
    }

    count = 0;

    W = vol->W;
    H = vol->H;
    D = vol->D;
    WH = W*H;

    tval = v3d.skin.value;

    for(k=1;k<D-1;k++)
      for(j=1;j<H-1;j++)
	for(i=1;i<W-1;i++) {
	  n = i + j*W + k*WH;
	  if (vol->u.i16[n] >= tval) {
	    if (vol->u.i16[n-1] < tval ||
		vol->u.i16[n+1] < tval ||
		vol->u.i16[n-W] < tval ||
		vol->u.i16[n+W] < tval ||
		vol->u.i16[n-WH] < tval ||
		vol->u.i16[n+WH] < tval) {
	      
	      map[count] = n;
	      ++count;
	      if (count == mapsize) {
		mapsize += 8192;
		map = v3d.skin.tmap = (int *) MemRealloc(map, 
						      sizeof(int) * mapsize);
		if (!map) {
		  error_memory();
		  return;
		}
	      }
	    }
	  }
	}
    v3d.skin.tcount = count;
  }
  
  MutexLock(v3d.skin.lock);
  FloatFill(v3d.skin.zbuf,MAGIC,v3d.skin.zlen);
  FloatFill(v3d.skin.ebuf,-MAGIC,v3d.skin.zlen);
  CImgFill(v3d.skin.out, view.bg);

  /* call the rendering */
  render_threshold(v3d.skin.out,&(v3d.rotation),
		   v3d.panx / v3d.zoom,v3d.pany / v3d.zoom,
		   vol->W,vol->H,vol->D,
		   v3d.skin.tmap,v3d.skin.tcount,
		   v3d.skin.zbuf,v3d.skin.ebuf,
		   v3d.skin.color, v3d.skin.smoothing);
  MutexUnlock(v3d.skin.lock);

  v3d.skin.invalid = 0;
  v3d.skin.ready = 1;
  notify.renderstep++;
}

void render_octant(void *args) {
  int *lookup;
  Volume *vol;
  int ox,oy,oz;

  v3d.octant.ready = 0;
  vol = voldata.original;

  MutexLock(v3d.octant.lock);
  FloatFill(v3d.octant.zbuf,MAGIC,v3d.octant.zlen);

  lookup = app_calc_lookup();
  if (!lookup) {
    MutexUnlock(v3d.octant.lock);
    return;
  }

  CImgFill(v3d.octant.out, view.bg);

  ox = v2d.cx;
  oy = v2d.cy;
  oz = v2d.cz;

  if (!v3d.skin.enable) 
    render_full_octant(v3d.octant.out,&(v3d.rotation),
		       v3d.panx / v3d.zoom, v3d.pany / v3d.zoom,
		       vol, ox, oy, oz, lookup, v3d.octant.zbuf);
  else
    render_real_octant(v3d.octant.out,&(v3d.rotation),
		       v3d.panx / v3d.zoom, v3d.pany / v3d.zoom,
		       vol, ox, oy, oz, lookup, v3d.octant.zbuf);
  MutexUnlock(v3d.octant.lock);
  MemFree(lookup);
  v3d.octant.invalid = 0;
  v3d.octant.ready = 1;
  notify.renderstep++;

}

void render_quick_full_octant(CImg *dest, Transform *rot, float panx,
			      float pany, Volume *vol,int cx,int cy,int cz,
			      int *lookup,float *depth)
{
  int w,h,w2,h2,W2,H2,D2,W,H,D;
  int i,j,k,l,p,q,px,py,ii,jj;
  float fa,fb;
  Point A,B;
  i16_t *val;
 
  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  W = vol->W;
  H = vol->H;
  D = vol->D;

  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  /* init z-buffer */
  FloatFill(depth, MAGIC, w*h);
  val = (i16_t *) MemAlloc(w*h*sizeof(i16_t));
  if (!val) {
    error_memory();
    return;
  }

  MemSet(val,0,w*h*sizeof(i16_t));

  /* make projection */

  // XY-plane
  for(j=0;j<H;j+=4)
    for(i=0;i<W;i+=4) {
      A.x = i - W2;
      A.y = j - H2;
      A.z = cz - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx; 
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[j] + vol->tbframe[cz]];
      }
    }

  // YZ-plane
  for(i=0;i<D;i+=4) 
    for(j=0;j<H;j+=4) {

      A.x = cx - W2;
      A.y = j - H2;
      A.z = i - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cz; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==D-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[cx + vol->tbrow[j] + vol->tbframe[i]];
      }
    }

  // XZ-plane
  for(j=0;j<D;j+=4)
    for(i=0;i<W;i+=4) {
      A.x = i - W2;
      A.y = cy - H2;
      A.z = j - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cz; if (l<0) l = -l;
	if (j==0 || i==0 || j==D-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[cy] + vol->tbframe[j]];
      }
    }

  /* forward-only 4x4 splatting */

  for(j=h-5;j>=0;j--)
    for(i=w-5;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(jj=0;jj<5;jj++)
	  for(ii=0;ii<5;ii++) {
	    fb = depth[q = p + ii + w*jj];
	    if (fa < fb) {
	      depth[q] = fa;
	      val[q] = val[p];
	    }
	  }
      }
    }


  /* render it */

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;
      if (depth[p] != MAGIC)
	CImgSet(dest,i,j, val[p] == 32767 ? view.fg : v2d.pal[lookup[val[p]]]);
    }

  MemFree(val);
}

void render_full_octant(CImg *dest, Transform *rot, float panx, float pany,
			Volume *vol,int cx,int cy,int cz,int *lookup,
			float *depth)
{
  int w,h,w2,h2,W2,H2,D2,W,H,D;
  int i,j,k,l,p,q,px,py;
  float fa,fb;
  Point A,B;
  i16_t *val;
  float dz,z,kz,Y,diag;
  int ca,cb;

  int spx[3] = {1,0,1};
  int spy[3] = {0,1,1};

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  W = vol->W;
  H = vol->H;
  D = vol->D;

  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  /* init z-buffer */
  FloatFill(depth, MAGIC, w*h);
  val = (i16_t *) MemAlloc(w*h*sizeof(i16_t));
  if (!val) {
    error_memory();
    return;
  }

  MemSet(val,0,w*h*sizeof(i16_t));

  /* make projection */

  // XY-plane
  for(j=0;j<H;j++)
    for(i=0;i<W;i++) {
      A.x = i - W2;
      A.y = j - H2;
      A.z = cz - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx; 
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[j] + vol->tbframe[cz]];
      }
    }

  // YZ-plane
  for(i=0;i<D;i++) 
    for(j=0;j<H;j++) {

      A.x = cx - W2;
      A.y = j - H2;
      A.z = i - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cz; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==D-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[cx + vol->tbrow[j] + vol->tbframe[i]];
      }
    }

  // XZ-plane
  for(j=0;j<D;j++)
    for(i=0;i<W;i++) {
      A.x = i - W2;
      A.y = cy - H2;
      A.z = j - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cz; if (l<0) l = -l;
	if (j==0 || i==0 || j==D-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[cy] + vol->tbframe[j]];
      }
    }

  /* forward-only 2x2 splatting */

  for(j=h-2;j>=0;j--)
    for(i=w-2;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(k=0;k<3;k++) {
	  fb = depth[q = p + spx[k] + w*spy[k]];
	  if (fa < fb) {
	    depth[q] = fa;
	    val[q] = val[p];
	  }
	}
      }
    }

  /* render it */

  if (v3d.octant.usedepth) {

    A.x = W;
    A.y = H;
    A.z = D;
    diag = VectorLength(&A) / 2.0;

    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	p = i+j*w;
	if (depth[p] != MAGIC) {
	  z = depth[p];
	  dz = (diag - z);
	  kz = (dz*dz) / (4.0*diag*diag);
	  kz = v3d.zgamma + v3d.zstretch * kz;
	  ca =  val[p] == 32767 ? view.fg : v2d.pal[lookup[val[p]]];
	  cb = RGB2YCbCr(ca);
	  Y = (float) t0(cb);
	  Y *= kz;
	  if (Y<0.0) Y=0.0;
	  if (Y>255.0) Y=255.0;
	  ca = triplet((int)Y,t1(cb),t2(cb));
	  ca = YCbCr2RGB(ca);
	  CImgSet(dest,i,j,ca);
	}
      }

  } else {

    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	p = i+j*w;
	if (depth[p] != MAGIC)
	  CImgSet(dest,i,j, val[p] == 32767 ? view.fg : v2d.pal[lookup[val[p]]]);
      }

  }

  MemFree(val);
}

void render_real_octant(CImg *dest, Transform *rot, float panx, float pany,
			Volume *vol,int cx,int cy,int cz,int *lookup,
			float *depth)
{
  Point A,B,C[8],R[8];
  int i,j,k,l,W,H,D,W2,H2,D2,mz,p,px,py,q,w,h,w2,h2;
  float vz,fa,fb;
  i16_t *val;

  int x0,xf,xi;
  int y0,yf,yi;
  int z0,zf,zi;

  float dz,z,kz,Y,diag;
  int ca,cb;

  int spx[3] = {1,0,1};
  int spy[3] = {0,1,1};

  W = vol->W; W2 = W / 2;
  H = vol->H; H2 = H / 2;
  D = vol->D; D2 = D / 2;

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  // find the proper octant
  PointAssign(&C[0], 0,0,0);
  PointAssign(&C[1], W,0,0);
  PointAssign(&C[2], W,H,0);
  PointAssign(&C[3], 0,H,0);
  PointAssign(&C[4], 0,0,D);
  PointAssign(&C[5], W,0,D);
  PointAssign(&C[6], W,H,D);
  PointAssign(&C[7], 0,H,D);

  for(i=0;i<8;i++) {
    PointTranslate(&C[i],-W2,-H2,-D2);
    PointTransform(&R[i],&C[i],rot);
  }
  vz = R[0].z;
  mz = 0;
  for(i=0;i<8;i++)
    if (R[i].z < vz) {
      vz = R[i].z;
      mz = i;
    }

  x0 = cx; y0 = cy; z0 = cz;

  if (mz==0 || mz==3 || mz==4 || mz==7) {
    xf = 0; xi = -1;
  } else {
    xf = W-1; xi = 1;
  }

  if (mz==0 || mz==1 || mz==4 || mz==5) {
    yf = 0; yi = -1;
  } else {
    yf = H-1; yi = 1;
  }

  if (mz<=3) {
    zf = 0; zi = -1;
  } else {
    zf = D-1; zi = 1;
  }

  // init z-buffer
  FloatFill(depth, MAGIC, w*h);
  val = (i16_t *) MemAlloc(w*h*sizeof(i16_t));
  if (!val) {
    error_memory();
    return;
  }
  MemSet(val,0,w*h*sizeof(i16_t));

  /* make projection */

  // XY-plane
  for(j=y0;j!=yf;j+=yi)
    for(i=x0;i!=xf;i+=xi) {
      A.x = i - W2;
      A.y = j - H2;
      A.z = cz - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[j] + vol->tbframe[cz]];
      }
    }

  // YZ-plane
  for(i=z0;i!=zf;i+=zi) 
    for(j=y0;j!=yf;j+=yi) {

      A.x = cx - W2;
      A.y = j - H2;
      A.z = i - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cz; if (k<0) k = -k;
	l = j-cy; if (l<0) l = -l;
	if (j==0 || i==0 || j==H-1 || i==D-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[cx + vol->tbrow[j] + vol->tbframe[i]];
      }
    }

  // XZ-plane
  for(j=z0;j!=zf;j+=zi)
    for(i=x0;i!=xf;i+=xi) {
      A.x = i - W2;
      A.y = cy - H2;
      A.z = j - D2;

      PointTransform(&B,&A,rot);
      px = w2 + B.x + panx;
      py = h2 + B.y + pany;
      if (px < 0 || py < 0 || px >= w || py >= h) continue;
      p = px + py*w;      
      if (B.z < depth[p]) {
	depth[p] = B.z;
	k = i-cx; if (k<0) k = -k;
	l = j-cz; if (l<0) l = -l;
	if (j==0 || i==0 || j==D-1 || i==W-1 || k<3 || l<3)
	  val[p] = 32767;
	else
	  val[p] = vol->u.i16[i + vol->tbrow[cy] + vol->tbframe[j]];
      }
    }

  /* forward-only 2x2 splatting */

  for(j=h-2;j>=0;j--)
    for(i=w-2;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(k=0;k<3;k++) {
	  fb = depth[q = p + spx[k] + w*spy[k]];
	  if (fa < fb) {
	    depth[q] = fa;
	    val[q] = val[p];
	  }
	}
      }
    }

  /* render it */


  if (v3d.octant.usedepth) {

    A.x = W;
    A.y = H;
    A.z = D;
    diag = VectorLength(&A) / 2.0;

    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	p = i+j*w;
	if (depth[p] != MAGIC) {
	  z = depth[p];
	  dz = (diag - z);
	  kz = (dz*dz) / (4.0*diag*diag);
	  kz = v3d.zgamma + v3d.zstretch * kz;
	  ca =  val[p] == 32767 ? view.fg : v2d.pal[lookup[val[p]]];
	  cb = RGB2YCbCr(ca);
	  Y = (float) t0(cb);
	  Y *= kz;
	  if (Y<0.0) Y=0.0;
	  if (Y>255.0) Y=255.0;
	  ca = triplet((int)Y,t1(cb),t2(cb));
	  ca = YCbCr2RGB(ca);
	  CImgSet(dest,i,j,ca);
	}
      }

  } else {

    for(j=0;j<h;j++)
      for(i=0;i<w;i++) {
	p = i+j*w;
	if (depth[p] != MAGIC)
	  CImgSet(dest,i,j, val[p] == 32767 ? view.fg : v2d.pal[lookup[val[p]]]);
      }

  }

  MemFree(val);
}

void render_objects(void *args) {
  Volume *vol;
  Transform T;

  v3d.obj.ready = 0;  
  MutexLock(v3d.obj.lock);
  FloatFill(v3d.obj.zbuf,MAGIC,v3d.obj.zlen);
  CImgFill(v3d.obj.out, view.bg);

  TransformCopy(&T,&(v3d.rotation));

  //MutexLock(voldata.masterlock);
  vol = voldata.label;
  if (vol!=NULL)
    render_labels(v3d.obj.out,&T,
		  v3d.panx / v3d.zoom, v3d.pany / v3d.zoom,
		  vol, v3d.obj.zbuf);
  //MutexUnlock(voldata.masterlock);
  MutexUnlock(v3d.obj.lock);

  v3d.obj.invalid = 0;
  v3d.obj.ready = 1;
  notify.renderstep++;
}

void render_quick_labels(CImg *dest, Transform *rot, float panx, float pany,
			 Volume *vol, float *depth)
{
  int W,H,D,WH,W2,H2,D2,w,h,w2,h2;
  int i,j,k,m,n,p,px,py,pz,ca;
  Transform T;
  Point A,B;
  char *val;
  float fa,fb;
  float *xb,*yb,*normals;
  float diag,dz,kz,z,Y;
  i8_t L;

  TransformCopy(&T,rot);

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;
  FloatFill(depth,MAGIC,w*h);

  val = (char *) MemAlloc(w*h);
  if (!val) return;
  MemSet(val,0,w*h);

  xb = (float *) MemAlloc(sizeof(float) * w * h * 3);
  if (!xb) return;
  yb      = &xb[w*h];
  normals = &xb[2*w*h];
  FloatFill(xb,0.0,w*h*2);

  W = vol->W;
  H = vol->H;
  D = vol->D;
  WH = W*H;

  W2 = W/2;
  H2 = H/2;
  D2 = D/2;

  for(i=0;i<vol->N;i+=19) {
    pz = i / WH;
    py = (i % WH) / W;
    px = (i % WH) % W;
    
    L = vol->u.i8[i];
    if (L != 0 && labels.visible[L-1]) {
      A.x = px - W2;
      A.y = py - H2;
      A.z = pz - D2;
      PointTransform(&B,&A,&T);

      px = B.x + w2 + panx;
      py = B.y + h2 + pany;

      if (px<0 || px>=w || py<0 || py>=h) continue;
      p = px + py*w;

      if (B.z < depth[p]) {
	xb[p] = B.x;
	yb[p] = B.y;
	depth[p] = B.z;
	val[p] = L;
      }
    }
  }

  // large splat  
  for(j=h-12;j>=0;j--)
    for(i=w-12;i>=0;i--) {
      k = i + j*w;
      if (depth[k] != MAGIC) {
        fa = depth[k];
        for(n=0;n<12;n++) {
          for(m=0;m<12;m++) {
            fb = depth[p = k + m + w*n];
            if (fa < fb) {
              depth[p] = fa;
	      val[p] = val[k];
	      xb[p] = i+m;
	      yb[p] = j+n;
	    }
          }
        }
      }
    }

  /* compute normals */

  for(j=12;j<h-12;j++)
    for(i=12;i<w-12;i++)
      if (depth[i+j*w] != MAGIC)
        render_calc_quick_normal(i,j,w,xb,yb,depth,normals);

  // render 

  A.x = W;
  A.y = H;
  A.z = D;
  diag = VectorLength(&A) / 2.0;

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;
      if (depth[p]!=MAGIC) {
        z = depth[p];
        dz = (diag - z);
        kz = (dz*dz) / (4.0*diag*diag);
        kz = v3d.zgamma + v3d.zstretch * kz;

        fa = normals[p];
        fb = phong_specular(fa, v3d.sre);
        
        ca=RGB2YCbCr(labels.colors[val[p]-1]);
        Y = (float) t0(ca);
        Y /= 255.0;
        
        Y = v3d.ka + kz * ( v3d.kd * Y * fa + v3d.ks * Y * fb);
        Y *= 255.0;
        if (Y > 255.0) Y=255.0;
        if (Y < 0.0) Y=0.0;
        ca = triplet((int) Y, t1(ca), t2(ca));
        ca = YCbCr2RGB(ca);
        CImgSet(dest,i,j,ca);
      }
    }

  MemFree(xb);
  MemFree(val);
}

void render_labels(CImg *dest, Transform *rot, float panx,float pany,
		   Volume *vol,float *depth)
{
  int w,h,w2,h2,W,H,D,W2,H2,D2,WH,N;
  int i,j,k,p,q;
  float *xb,*yb,*normals;
  float fa,fb;
  Point A,B;
  float diag,dz,kz,z,Y;
  int ca;
  i8_t *val, l;

  int px,py,pz;

  int spx[3] = {1,0,1};
  int spy[3] = {0,1,1};

  w = dest->W;
  h = dest->H;
  w2 = w/2;
  h2 = h/2;

  W = vol->W;
  H = vol->H;
  D = vol->D;
  W2 = W / 2;
  H2 = H / 2;
  D2 = D / 2;

  WH = W*H;
  N = vol->N;

  VectorNormalize(&(v3d.light));

  /* init z-buffer and float (x,y) buffers (for normals) */
  xb = (float *) MemAlloc( w * h * sizeof(float) );
  yb = (float *) MemAlloc( w * h * sizeof(float) );
  normals = (float *) MemAlloc( w * h * sizeof(float) );
  val = (i8_t *) MemAlloc(w * h * sizeof(i8_t));

  if (!xb || !yb || !normals || !val) {
    error_memory();
    return;
  }

  FloatFill(depth, MAGIC, w*h);
  FloatFill(xb, 0.0, w*h);
  FloatFill(yb, 0.0, w*h);
  FloatFill(normals, 0.0, w*h);
  MemSet(val,0,w*h*sizeof(i8_t));

  /* make projection */

  for(i=0;i<N;i++) {

    l = vol->u.i8[i];
    if (l == 0) continue;
    if (!labels.visible[l-1]) continue;

    pz = i / WH;
    py = (i % WH) / W;
    px = (i % WH) % W;

    A.x = px - W2;
    A.y = py - H2;
    A.z = pz - D2;

    PointTransform(&B,&A,rot);
    px = w2 + B.x + panx;
    py = h2 + B.y + pany;

    if (px < 0 || py < 0 || px >= w || py >= h) continue;

    p = px + py*w;

    if (B.z < depth[p]) {
      depth[p] = B.z;
      xb[p] = B.x;
      yb[p] = B.y;
      val[p] = vol->u.i8[i];
    }

  }

  /* forward-only 2x2 splatting */

  for(j=h-2;j>=0;j--)
    for(i=w-2;i>=0;i--) {
      p = i + j*w;
      if (depth[p] != MAGIC) {
	fa = depth[p];
	for(k=0;k<3;k++) {
	  fb = depth[q = p + spx[k] + w*spy[k]];
	  if (fa < fb) {
	    depth[q] = fa;
	    val[q] = val[p];
	    xb[q] = xb[p] + spx[k];
	    yb[q] = yb[p] + spy[k];
	  }
	}
      }
    }

  /* compute normals */

  for(j=2;j<h-2;j++)
    for(i=2;i<w-2;i++)
      if (depth[i+j*w] != MAGIC)
	render_calc_normal(i,j,w,xb,yb,depth,normals);

  /* render the monster */

  A.x = W;
  A.y = H;
  A.z = D;
  diag = VectorLength(&A) / 2.0;

  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      p = i+j*w;

      if (depth[p] != MAGIC) {
	z = depth[p];
	dz = (diag - z);
	kz = (dz*dz) / (4.0*diag*diag);

	kz = v3d.zgamma + v3d.zstretch * kz;

	fa = normals[p];
	fb = phong_specular(fa, v3d.sre);
	
	ca=RGB2YCbCr( labels.colors[(int) (val[p])-1] );
	Y = (float) t0(ca);
	Y /= 255.0;
	
	Y = v3d.ka + kz * ( v3d.kd * Y * fa + v3d.ks * Y * fb);
	Y *= 255.0;
	if (Y > 255.0) Y=255.0;
	if (Y < 0.0) Y=0.0;
	ca = triplet((int) Y, t1(ca), t2(ca));
	ca = YCbCr2RGB(ca);
	CImgSet(dest,i,j,ca);
      }
    }
      
  MemFree(normals);
  MemFree(xb);
  MemFree(yb);
  MemFree(val);
}

