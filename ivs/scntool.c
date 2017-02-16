
/* 
   scntool - (C) 2003 Felipe Bergo fbergo at gmail.com
   manual available in man/scntool.1 

   (C) 2003-2017 Felipe P.G. Bergo fbergo at gmail dot com

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef struct _scn {
  int W,H,D,WxH,N;
  float dx,dy,dz;
  int bpv;
  short int *data;
} SCN;

typedef struct _trans {
  float c[3][3];
} Transform;

typedef struct _point {
  float x,y,z;
} Point;

char *input  = 0;
char *output = 0;
SCN  *scene  = 0;
int  verbose = 0;

/* low-level ops [no stderr reporting] */
SCN *      scn_new();
SCN *      scn_dup(SCN *scn);
int        scn_alloc(SCN *scn, int x, int y, int z);
int        scn_realloc(SCN *scn, int x, int y, int z);
int        scn_read_header(SCN *scn, FILE *f);
int        scn_read_data(SCN *scn, FILE *f);
void       scn_destroy(SCN *scn);
void       scn_set(SCN *scn, int x,int y,int z, short int value);
short int  scn_get(SCN *scn, int x,int y,int z);
short int  scn_fget(SCN *scn, float x,float y,float z);
short int  scn_max(SCN *scn);
short int  scn_min(SCN *scn);

short int  swab_16(short int val);
void       point_transform(Point *p, Transform *t);
void       point_xlate(Point *p, float dx, float dy, float dz);
void       point_assign(Point *p, float x, float y, float z);
void       trans_ident(Transform *t);
void       trans_rotx(Transform *t, float angle);
void       trans_roty(Transform *t, float angle);
void       trans_rotz(Transform *t, float angle);

/* high-level ops [decent error reporting] */
SCN *      scn_read(char *path);
int        scn_write(SCN *scn, char *path);

#define scn__min(a,b) (((a)<(b))?(a):(b))
#define scn__max(a,b) (((a)>(b))?(a):(b))

/* operations */
void oper_flipx();
void oper_flipy();
void oper_flipz();
void oper_rotz90();
void oper_roty90();
void oper_rotx90();
void oper_bpv(int v);
void oper_info();
void oper_normalize(int nmin, int nmax);
void oper_resizex(float ndx);
void oper_resizey(float ndy);
void oper_resizez(float ndz);
void oper_cropx(int xmin, int xmax);
void oper_cropy(int ymin, int ymax);
void oper_cropz(int zmin, int zmax);
void oper_arotx(float angle);
void oper_aroty(float angle);
void oper_arotz(float angle);
void oper_swap();
void oper_autopad();
void oper_autocrop();
void fatal();

void verbose_apply(char *s);
void verbose_ok();
void verbose_start();
void verbose_progress(int done, int total);
void verbose_done();

SCN * scn_new() {
  SCN *x;  x = (SCN *) malloc(sizeof(SCN));
  x->data = 0;
  return x;
}

SCN * scn_dup(SCN *scn) {
  SCN *x;

  x = scn_new();
  if (!x) return 0;
  if (scn_alloc(x, scn->W, scn->H, scn->D) < 0) return 0;
  x->dx  = scn->dx;
  x->dy  = scn->dy;
  x->dz  = scn->dz;
  x->bpv = scn->bpv;

  return x;
}

int   scn_alloc(SCN *scn, int x, int y, int z) {
  if (!scn) return -1;
  scn->W = x;
  scn->H = y;
  scn->D = z;
  scn->N = x*y*z;
  scn->WxH = x*y;
  scn->data = (short int *) malloc(2 * scn->N);
  if (!scn->data) return -1;
  memset(scn->data, 0, 2 * scn->N);
  return 0;
}

int scn_realloc(SCN *scn, int x, int y, int z) {
  if (!scn) return -1;
  if (scn->data) { free(scn->data); scn->data = 0; }
  return(scn_alloc(scn,x,y,z));
}

int   scn_read_header(SCN *scn, FILE *f) {
  if (!scn) return -1;
  if (!f) return -1;
  if (fscanf(f,"SCN \n %d %d %d \n %f %f %f \n %d \n",
	     &(scn->W), &(scn->H), &(scn->D),
	     &(scn->dx), &(scn->dy), &(scn->dz),
	     &(scn->bpv)) != 7)
    return -1;
  return 0;
}

int scn_read_data(SCN *scn, FILE *f) {
  int i,j,k,p;
  unsigned char *buf8;
  short int *buf16;
  int host_be = 0;
  
  buf8 = (unsigned char *) malloc(2 * scn->W);
  if (!buf8) return -1;

  buf16 = (short int *) buf8;

  buf16[0] = 0x1100;
  if (buf8[0] == 0x11) host_be = 1;

  j = scn->H * scn->D;

  switch(scn->bpv) {
  case 8:
    p = 0;
    for(i=0;i<j;i++) {
      if (fread(buf8,1,scn->W,f) != scn->W) return -1;
      for(k=0;k<scn->W;k++)
	scn->data[p++] = (short int) (buf8[k]);
    }
    break;
  case 16:
    p = 0;
    for(i=0;i<j;i++) {
      if (fread(buf8,2,scn->W,f) != scn->W) return -1;
      for(k=0;k<scn->W;k++)
	scn->data[p++] = (short int) (buf16[k]);
    }
    if (host_be)
      for(i=0;i<scn->N;i++)
	scn->data[i] = swab_16(scn->data[i]);
    break;
  default:
    free(buf8);
    return -1;
  }

  free(buf8);
  return 0;
}

int scn_write(SCN *scn, char *path) {
  FILE *f;
  unsigned char *buf8;
  short int *buf16;
  int i,j,k;
  int host_be = 0;

  if (!scn) return -1;
  f = fopen(path,"w");
  if (!f) {
    fprintf(stderr,"** unable to open %s for writing.\n",path);
    return -1;
  }

  buf8 = (unsigned char *) malloc(2 * scn->W);
  if (!buf8) {
    fprintf(stderr,"** malloc error.\n");
    fclose(f);
    return -1;
  }

  buf16 = (short int *) buf8;

  buf16[0] = 0x1100;
  if (buf8[0] == 0x11) host_be = 1;

  fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n%d\n",
	  scn->W, scn->H, scn->D,
	  scn->dx, scn->dy, scn->dz, scn->bpv);

  switch(scn->bpv) {
  case 8:
    j = scn->H * scn->D;
    for(i=0;i<j;i++) {
      for(k=0;k<scn->W;k++)
	buf8[k] = (unsigned char) (scn->data[i*scn->W + k]);
      if (fwrite(buf8, 1, scn->W, f) != scn->W) {
	fprintf(stderr,"** write error.\n");
	return -1;
      }
    }
    break;
  case 16:
    j = scn->H * scn->D;

    if (host_be)
      for(i=0;i<scn->N;i++)
	scn->data[i] = swab_16(scn->data[i]);

    for(i=0;i<j;i++) {
      for(k=0;k<scn->W;k++)
	buf16[k] = scn->data[i*scn->W + k];
      if (fwrite(buf8, 2, scn->W, f) != scn->W) {
	fprintf(stderr,"** write error.\n");
	return -1;
      }
    }
    break;
  default:
    fprintf(stderr,"** unsupported bpv, and this code should be unreachable.\n");
    return -1;

  }
  fclose(f);
  return 0;
}

SCN * scn_read(char *path) {
  FILE *f;
  SCN  *scn;

  f = fopen(path,"r");
  if (!f) {
    fprintf(stderr,"** unable to open %s for reading.\n",path);
    return 0;
  }

  scn = scn_new();
  if (!scn) {
    fprintf(stderr,"** cannot allocate structure.\n");
    fclose(f);
    return 0;
  }
  if (scn_read_header(scn, f) < 0) {
    fprintf(stderr,"** error reading scn header.\n");
    fclose(f);
    return 0;
  }
  if (scn->bpv != 8 && scn->bpv != 16) {
    fprintf(stderr,"** unsupported bpv value: %d\n",scn->bpv);
    fclose(f);
    return 0;
  }
  if (scn_alloc(scn, scn->W, scn->H, scn->D) < 0) {
    fprintf(stderr,"** cannot allocate structure.\n");
    fclose(f);
    return 0;
  }
  if (scn_read_data(scn, f) < 0) {
    fprintf(stderr,"** error reading volume data.\n");
    fclose(f);
    return 0;
  }
  fclose(f);
  return scn;
}

void  scn_destroy(SCN *scn) {
  if (!scn) return;
  if (scn->data) free(scn->data);
  free(scn);
}

void scn_set(SCN *scn, int x,int y,int z, short int value) {
  int n;
  n = x + (scn->W * y) + (scn->WxH * z);
  if (x < 0 || x >= scn->W ||
      y < 0 || y >= scn->H ||
      z < 0 || z >= scn->D)
    return;
  scn->data[n] = value;
}

short int scn_get(SCN *scn, int x,int y,int z) {
  int n;
  n = x + (scn->W * y) + (scn->WxH * z);
  if (x < 0 || x >= scn->W ||
      y < 0 || y >= scn->H ||
      z < 0 || z >= scn->D)
    return 0;
  return(scn->data[n]);
}

short int  scn_fget(SCN *scn, float x,float y,float z) {
  float da,db,va,vb,v;
  Point a,b;
  short int i,j;

  a.x = floor(x);
  a.y = floor(y);
  a.z = floor(z);
  b.x = ceil(x);
  b.y = ceil(y);
  b.z = ceil(z);

  i = scn_get(scn, (int)(a.x), (int)(a.y), (int)(a.z));
  j = scn_get(scn, (int)(b.x), (int)(b.y), (int)(b.z));

  if (i == j) return(i);

  va = (float) i;
  vb = (float) j;

  da = (x - a.x)*(x - a.x) + (y - a.y)*(y - a.y) + (z - a.z)*(z - a.z);
  db = (x - b.x)*(x - b.x) + (y - b.y)*(y - b.y) + (z - b.z)*(z - b.z);

  if (da == 0.0 && db == 0.0) return( i );

  v = ((da*vb)+(db*va))/(da+db);
  return((short int)v);
}

short int  scn_max(SCN *scn) {
  short int x = -32768;
  int i;
  for(i=0;i<scn->N;i++)
    if (scn->data[i] > x) x = scn->data[i];
  return x;
}

short int  scn_min(SCN *scn) {
  short int x = 32767;
  int i;
  for(i=0;i<scn->N;i++)
    if (scn->data[i] < x) x = scn->data[i];
  return x;
}

short int swab_16(short int val) {
  short int x;
  val &= 0xffff;
  x  = (val & 0xff) << 8;
  x |= (val >> 8);
  return x;
}

void point_transform(Point *p, Transform *t) {
  Point q;
  q.x = p->x * t->c[0][0] + p->y * t->c[0][1] + p->z * t->c[0][2];
  q.y = p->x * t->c[1][0] + p->y * t->c[1][1] + p->z * t->c[1][2];
  q.z = p->x * t->c[2][0] + p->y * t->c[2][1] + p->z * t->c[2][2];
  p->x = q.x;
  p->y = q.y;
  p->z = q.z;
}

void point_xlate(Point *p, float dx, float dy, float dz) {
  p->x += dx;
  p->y += dy;
  p->z += dz;
}

void point_assign(Point *p, float x, float y, float z) {
  p->x = x;
  p->y = y;
  p->z = z;
}

void trans_ident(Transform *t) {
  int i,j;
  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      t->c[i][j] = (i==j) ? 1.0 : 0.0;
}

void trans_rotx(Transform *t, float angle) {
  trans_ident(t);
  angle *= M_PI / 180.0;
  t->c[1][1] = cos(angle);
  t->c[2][1] = sin(angle);
  t->c[1][2] = -sin(angle);
  t->c[2][2] = cos(angle);
}

void trans_roty(Transform *t, float angle) {
  trans_ident(t);
  angle *= M_PI / 180.0;
  t->c[0][0] = cos(angle);
  t->c[2][0] = sin(angle);
  t->c[0][2] = -sin(angle);
  t->c[2][2] = cos(angle);
}

void trans_rotz(Transform *t, float angle) {
  trans_ident(t);
  angle *= M_PI / 180.0;
  t->c[0][0] = cos(angle);
  t->c[1][0] = sin(angle);
  t->c[0][1] = -sin(angle);
  t->c[1][1] = cos(angle);
}

int verbose_state = 0;

void fatal() {
  if (verbose_state == 1)
    verbose_done();
  if (verbose_state != 0)
    fprintf(stderr,"\n");
  fprintf(stderr,"** fatal error (probably out of memory).\n");
  exit(3);
}

void oper_flipx() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i, Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, k, j, i, scn_get(P, Q->W - k - 1, j, i));
  }
  
  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_flipy() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i, Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, k, j, i, scn_get(P, k, Q->H - j - 1, i));
  }

  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_flipz() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i, Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, k, j, i, scn_get(P, k, j, Q->D - i - 1));
  }

  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_rotz90() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  if (scn_realloc(Q, P->H, P->W, P->D) < 0) fatal();
  Q->dx = P->dy;
  Q->dy = P->dx;

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i, Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, Q->W - k - 1, j, i, scn_get(P, j, k, i));
  }

  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_roty90() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  if (scn_realloc(Q, P->D, P->H, P->W) < 0) fatal();
  Q->dx = P->dz;
  Q->dz = P->dx;

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i, Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, k, j, Q->D - i - 1, scn_get(P, i, j, k));
  }

  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_rotx90() {
  SCN *Q,*P;
  int i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  if (scn_realloc(Q, P->W, P->D, P->H) < 0) fatal();
  Q->dy = P->dz;
  Q->dz = P->dy;

  verbose_start();

  for(i=0;i<Q->D;i++) {
    verbose_progress(i,Q->D);
    for(j=0;j<Q->H;j++)
      for(k=0;k<Q->W;k++)
	scn_set(Q, k, Q->H - j - 1, i, scn_get(P, k, i, j));
  }

  verbose_done();
  scn_destroy(scene);
  scene = Q;
}

void oper_bpv(int v) {
  SCN *P = scene;
  int i;

  if (!P) fatal();
  if (v != 8 && v != 16) fatal();

  /* 16 => 16 or 8 => 8 do nothing */
  if (P->bpv == v) return;

  /* 8 => 16 only changes the field */
  if (v == 16) {
    P->bpv = 16;
    return;
  }

  /* 16 => 8 thresholds the volume */

  verbose_start();

  for(i=0;i<P->N;i++) {
    if (i % P->WxH == 0)
      verbose_progress(i, P->N);
    if (P->data[i] < 0)   P->data[i] = 0;
    if (P->data[i] > 255) P->data[i] = 255;
  }
  P->bpv = v;

  verbose_done();
}

void oper_info() {
  short int vmin, vmax;

  if (!scene) return;

  vmin = scn_min(scene);
  vmax = scn_max(scene);

  printf("Dimension (voxels):     %d x %d x %d [total=%d]\n",
	 scene->W,scene->H,scene->D,scene->N);
  printf("Voxel size        :     %.4f x %.4f x %.4f [volume=%.4f]\n",
	 scene->dx,scene->dy,scene->dz,scene->dx*scene->dy*scene->dz);
  printf("Bits per voxel    :     %d\n",scene->bpv);
  printf("Range (Min, Max)  :     %d -- %d\n",vmin,vmax);
}

void oper_normalize(int nmin, int nmax) {
  SCN *P = scene;
  int i, j, cmin, cmax, dn, dc;
  float x,z, fcmin, fnmin;

  if (!P) fatal();

  cmin = scn_min(P);
  cmax = scn_max(P);

  dn = nmax - nmin;
  dc = cmax - cmin;
  if (dc == 0) dc = 1;
  x = ((float)(dn)) / ((float)(dc));
  fnmin = (float) nmin;
  fcmin = (float) cmin;

  verbose_start();

  for(i=0;i<P->N;i++) {
    if (i % P->WxH == 0)
      verbose_progress(i,P->N);

    z = (float) (P->data[i]);
    z = ((z - fcmin) * x) + fnmin;
    j = (int) z;
    if (j < -32768) j = -32768;
    if (j > 32767)  j = 32767;
    P->data[i] = (short int) j;
  }

  verbose_done();
}

void oper_resizex(float ndx) {
  SCN *P, *Q;
  int px,py,pz, qx,qy,qz, rx,ry,rz;
  float walked_dist, dpq;
  short int v;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();
  
  px = (int) ((float)(P->W-1)*(P->dx/ndx)) + 1;
  if (scn_realloc(Q, px , P->H, P->D) < 0) fatal();
  Q->dx = ndx;

  verbose_start();

  for(qx=0; qx < Q->W; qx++) {
    verbose_progress(qx, Q->W);
    for(qz=0; qz < Q->D; qz++)
      for(qy=0; qy < Q->H; qy++) {

	walked_dist = ((float) qx) * Q->dx;
	
	px = (int)(walked_dist/P->dx);
	py = qy;
	pz = qz;
	
	rx = px + 1; 
	ry = py;
	rz = pz;
          
	dpq =  walked_dist - ( ((float)px) * P->dx );
	
	v = (short int) (( ( P->dx - dpq) * ((float)scn_get(P,px,py,pz)) +
			   (     dpq    ) * ((float)scn_get(P,rx,ry,rz)) ) /
			 P->dx );
	scn_set(Q, qx,qy,qz, v);
      }
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_resizey(float ndy) {
  SCN *P, *Q;
  int px,py,pz, qx,qy,qz, rx,ry,rz;
  float walked_dist, dpq;
  short int v;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();
  
  py = (int) ((float)(P->H-1)*(P->dy/ndy)) + 1;
  if (scn_realloc(Q, P->W ,py, P->D) < 0) fatal();
  Q->dy = ndy;

  verbose_start();
  
  for(qy=0; qy < Q->H; qy++) {
    verbose_progress(qy, Q->H);
    for(qx=0; qx < Q->W; qx++)
      for(qz=0; qz < Q->D; qz++) {

	walked_dist = ((float) qy) * Q->dy;
	
	px = qx;
	py = (int)(walked_dist/P->dy);
	pz = qz;
	
	rx = px; 
	ry = py + 1;
	rz = pz;
          
	dpq =  walked_dist - ( ((float)py) * P->dy );
	
	v = (short int) (( ( P->dy - dpq) * ((float)scn_get(P,px,py,pz)) +
			   (     dpq    ) * ((float)scn_get(P,rx,ry,rz)) ) /
			 P->dy );
	scn_set(Q, qx,qy,qz, v);
      }
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_resizez(float ndz) {
  SCN *P, *Q;
  int px,py,pz, qx,qy,qz, rx,ry,rz;
  float walked_dist, dpq;
  short int v;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();
  
  pz = (int) ((float)(P->D-1)*(P->dz/ndz)) + 1;
  if (scn_realloc(Q, P->W, P->H, pz) < 0) fatal();
  Q->dz = ndz;

  verbose_start();
  
  for(qz=0; qz < Q->D; qz++) {
    verbose_progress(qz, Q->D);
    for(qy=0; qy < Q->H; qy++)
      for(qx=0; qx < Q->W; qx++) {

	walked_dist = ((float) qz) * Q->dz;
	
	px = qx;
	py = qy;
	pz = (int)(walked_dist/P->dz);

	
	rx = px; 
	ry = py;
	rz = pz + 1;
          
	dpq =  walked_dist - ( ((float)pz) * P->dz );
	
	v = (short int) (( ( P->dz - dpq) * ((float)scn_get(P,px,py,pz)) +
			   (     dpq    ) * ((float)scn_get(P,rx,ry,rz)) ) /
			 P->dz );
	scn_set(Q, qx,qy,qz, v);
      }
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_cropx(int xmin, int xmax) {
  SCN *P, *Q;
  int nw, i, j, k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  nw = xmax - xmin + 1;
  if (scn_realloc(Q, nw, Q->H, Q->D) < 0) fatal();

  verbose_start();

  for(k=0;k<Q->D;k++) {
    verbose_progress(k, Q->D);
    for(j=0;j<Q->H;j++)
      for(i=0;i<Q->W;i++)
	scn_set(Q, i, j, k, scn_get(P, i + xmin, j, k));
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_cropy(int ymin, int ymax) {
  SCN *P, *Q;
  int nh, i, j, k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  nh = ymax - ymin + 1;
  if (scn_realloc(Q, Q->W, nh, Q->D) < 0) fatal();

  verbose_start();

  for(k=0;k<Q->D;k++) {
    verbose_progress(k, Q->D);
    for(j=0;j<Q->H;j++)
      for(i=0;i<Q->W;i++)
	scn_set(Q, i, j, k, scn_get(P, i, j + ymin, k));
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_cropz(int zmin, int zmax) {
  SCN *P, *Q;
  int nd, i, j, k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  nd = zmax - zmin + 1;
  if (scn_realloc(Q, Q->W, Q->H, nd) < 0) fatal();

  verbose_start();

  for(k=0;k<Q->D;k++) {
    verbose_progress(k,Q->D);
    for(j=0;j<Q->H;j++)
      for(i=0;i<Q->W;i++)
	scn_set(Q, i, j, k, scn_get(P, i, j, k + zmin));
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_arotx(float angle) {
  SCN *P, *Q;
  Point a;
  Transform t;
  float cx,cy,cz, i, j, k;

  P = scene;
  if (!P) fatal();

  verbose_start();

  Q = scn_dup(P);
  if (!Q) fatal();

  trans_rotx(&t, -angle);
  cx = Q->W / 2.0;
  cy = Q->H / 2.0;
  cz = Q->D / 2.0;

  for(k=0.0;k<Q->D;k++) {
    verbose_progress( (int) k, Q->D );
    for(j=0.0;j<Q->H;j++)  
      for(i=0.0;i<Q->W;i++) {
	a.x = i - cx; a.y = j - cy; a.z = k - cz;
	point_transform(&a, &t);
	a.x += cx; a.y += cy; a.z += cz;

	scn_set(Q, (int) i, (int) j, (int) k,
		scn_fget(P, a.x, a.y, a.z));
      }
  }
  
  scn_destroy(P);
  scene = Q;
  verbose_done();
}

void oper_aroty(float angle) {
  SCN *P, *Q;
  Point a;
  Transform t;
  float cx,cy,cz, i, j, k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  verbose_start();

  trans_roty(&t, angle);
  cx = Q->W / 2.0;
  cy = Q->H / 2.0;
  cz = Q->D / 2.0;

  for(k=0.0;k<Q->D;k++) {
    verbose_progress( (int) k, Q->D );
    for(j=0.0;j<Q->H;j++)  
      for(i=0.0;i<Q->W;i++) {
	a.x = i - cx; a.y = j - cy; a.z = k - cz;
	point_transform(&a, &t);
	a.x += cx; a.y += cy; a.z += cz;

	scn_set(Q, (int) i, (int) j, (int) k,
		scn_fget(P, a.x, a.y, a.z));
      }
  }
  
  scn_destroy(P);
  scene = Q;
  verbose_done();
}

void oper_arotz(float angle) {
  SCN *P, *Q;
  Point a;
  Transform t;
  float cx,cy,cz, i, j, k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  verbose_start();

  trans_rotz(&t, -angle);
  cx = Q->W / 2.0;
  cy = Q->H / 2.0;
  cz = Q->D / 2.0;

  for(k=0.0;k<Q->D;k++) {
    verbose_progress( (int) k, Q->D );
    for(j=0.0;j<Q->H;j++)  
      for(i=0.0;i<Q->W;i++) {
	a.x = i - cx; a.y = j - cy; a.z = k - cz;
	point_transform(&a, &t);
	a.x += cx; a.y += cy; a.z += cz;

	scn_set(Q, (int) i, (int) j, (int) k,
		scn_fget(P, a.x, a.y, a.z));
      }
  }
  
  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_swap() {
  int i;

  if (!scene) fatal();
  if (scene->bpv != 16) return;

  verbose_start();
  for(i=0;i<scene->N;i++) {
    if (i % scene->WxH == 0)
      verbose_progress(i,scene->N);
    scene->data[i] = swab_16(scene->data[i]);
  }
  verbose_done();
}

void oper_autopad() {
  SCN *P, *Q;
  int diag, mx,my,mz, i,j,k;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  diag = (int) (ceil(sqrt( (P->W)*(P->W) + (P->H)*(P->H) + (P->D)*(P->D) )));
  mx = (diag - P->W) / 2;
  my = (diag - P->H) / 2;
  mz = (diag - P->D) / 2;

  if (scn_realloc(Q, diag, diag, diag) < 0) fatal();

  verbose_start();

  for(k=0;k<P->D;k++) {
    verbose_progress(k,P->D);
    for(j=0;j<P->H;j++)
      for(i=0;i<P->W;i++)
	scn_set(Q, i+mx, j+my, k+mz, scn_get(P,i,j,k));
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void oper_autocrop() {
  SCN *P, *Q;
  int i,j,k,mx,my,mz,nx,ny,nz,w,h,d;

  P = scene;
  if (!P) fatal();

  Q = scn_dup(P);
  if (!Q) fatal();

  mx = P->W - 1;
  my = P->H - 1;
  mz = P->D - 1;
  nx=ny=nz=0;

  verbose_start();

  for(k=0;k<P->D;k++) {
    verbose_progress(k,2 * P->D);
    for(j=0;j<P->H;j++)
      for(i=0;i<P->W;i++)
	if (scn_get(P,i,j,k) != 0) {
	  mx = scn__min(mx, i);
	  my = scn__min(my, j);
	  mz = scn__min(mz, k);
	  nx = scn__max(nx, i);
	  ny = scn__max(ny, j);
	  nz = scn__max(nz, k);
	}
  }

  w = nx - mx + 1;
  h = ny - my + 1;
  d = nz - mz + 1;
  
  if (scn_realloc(Q, w, h, d) < 0) fatal();

  for(k=0;k<d;k++) {
    verbose_progress(P->D + k,2 * P->D);
    for(j=0;j<h;j++)
      for(i=0;i<w;i++)
	scn_set(Q,i,j,k, scn_get(P, i+mx, j+my, k+mz));
  }

  verbose_done();
  scn_destroy(P);
  scene = Q;
}

void verbose_apply(char *s) {
  int i;
  if (verbose) {
    printf("applying %s... ",s);
    i = 12 - strlen(s);
    while(i>0) { printf(" "); --i; }
    fflush(stdout);
    verbose_state = 2;
  }
}

void verbose_ok() {
  if (verbose) {
    printf("ok\n");
    verbose_state = 0;
  }
}

void verbose_start() {
  char x[32];
  if (!verbose) return;
  
  memset(x,32,32);
  x[28] = 0;
  printf("%s",x);
  fflush(stdout);
  verbose_state = 1;
}

void verbose_progress(int done, int total) {
  char x[32];
  int i;
  if (!verbose) return;

  memset(x,8,28);
  x[28] = 0;
  printf("%s",x);

  memset(x,32,32);
  x[28] = 0;
  x[0] = '[';
  x[21] = ']';

  i = (20*done) / total;
  memset(x+1,'=',i);
  x[i+1] = '>';
  i = (100*done) / total;
  x[23] = i>99 ? '1' : ' ';
  x[24] = i>9 ? '0'+(i/10) : ' ';
  x[25] = '0' + i%10;
  x[26] = '%';
  printf("%s",x);
  fflush(stdout);
  verbose_state = 1;
}

void verbose_done() {
  char x[32];
  if (!verbose) return;
  memset(x,8,28);
  x[28] = 0;
  printf("%s",x);
  memset(x,32,28);
  printf("%s",x);
  memset(x,8,28);
  printf("%s",x);
  verbose_state = 2;
}

int main(int argc, char **argv) {
  int i,j,k,dashdo=8000;
  float f[3];
  
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i],"-do")) {
	dashdo = i;
	break;
      }
      if (!strcmp(argv[i],"-v")) {
	verbose = 1;
	continue;
      }
      if (i<(argc-1) && !strcmp(argv[i],"-o")) {
	output = strdup(argv[i+1]);
	++i;
      }
    } else {
      input = strdup(argv[i]);
    }
  }

  if (!input) {
    fprintf(stderr,"Usage: scntool input [-v] [-o output] -do [operations]\n\n");
    return 1;
  }

  if (!output) {
    output = strdup(input);
  }

  if (verbose) {
    printf("reading %s... ",input); fflush(stdout);
  }
  scene = scn_read(input);
  if (!scene) return 2;
  verbose_ok();

  /* perform operations */

  for(i=dashdo+1;i<argc;i++) {

    /* 0-argument filters */
    if (!strcmp(argv[i], "flipx")) {
      verbose_apply("flipx");
      oper_flipx();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "flipy")) {
      verbose_apply("flipy");
      oper_flipy();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "flipz")) {
      verbose_apply("flipz");
      oper_flipz();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "info")) {
      verbose_apply("info");
      verbose_ok();
      oper_info();
      continue;
    }

    if (!strcmp(argv[i], "swap")) {
      verbose_apply("swap");
      oper_swap();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "autopad")) {
      verbose_apply("autopad");
      oper_autopad();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "autocrop")) {
      verbose_apply("autocrop");
      oper_autocrop();
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "isoscale")) {
      verbose_apply("isoscale");
      f[0] = scene->dx;
      f[1] = scene->dy;
      f[2] = scene->dz;
      if (f[1] < f[0]) f[0] = f[1];
      if (f[2] < f[0]) f[0] = f[2];
      if (f[0] != scene->dx) oper_resizex(f[0]);
      if (f[0] != scene->dy) oper_resizey(f[0]);
      if (f[0] != scene->dz) oper_resizez(f[0]);
      verbose_ok();
      continue;
    }

    if (!strcmp(argv[i], "quit")) {
      if (verbose) printf("leaving (quit requested).\n");
      return 0;
    }

    /* 3-argument filters */
    if ( (argc - i) > 3 ) {
      if (!strcmp(argv[i], "voxelsize")) {
	verbose_apply("voxelsize");
	f[0] = atof(argv[++i]);
	f[1] = atof(argv[++i]);
	f[2] = atof(argv[++i]);
	if (f[0] == 0.0 || f[1] == 0.0 || f[2] == 0.0) {
	  fprintf(stderr,"** bad or missing voxelsize arguments.\n");
	  return 2;
	}
	if (f[0] > 0.0 && f[0] != scene->dx) oper_resizex(f[0]);
	if (f[1] > 0.0 && f[1] != scene->dy) oper_resizey(f[1]);
	if (f[2] > 0.0 && f[2] != scene->dz) oper_resizez(f[2]);
	verbose_ok();
	continue;
      }
      if (!strcmp(argv[i], "voxelscale")) {
	verbose_apply("voxelscale");
	f[0] = atof(argv[++i]);
	f[1] = atof(argv[++i]);
	f[2] = atof(argv[++i]);
	if (f[0] == 0.0 || f[1] == 0.0 || f[2] == 0.0) {
	  fprintf(stderr,"** bad or missing voxelscale arguments.\n");
	  return 2;
	}
	if (f[0] > 0.0) oper_resizex(f[0]*scene->dx);
	if (f[1] > 0.0) oper_resizey(f[1]*scene->dy);
	if (f[2] > 0.0) oper_resizez(f[2]*scene->dz);
	verbose_ok();
	continue;
      }

    }

    /* 2-argument filters */
    if ( (argc - i) > 2 ) {
      if (!strcmp(argv[i], "normalize")) {
	verbose_apply("normalize");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) j=90000;
	k = atoi(argv[++i]);
	if (k==0 && !isdigit(argv[i][0])) k=90000;
	if (j >= k || j > 32767 || k > 32767 || j < -32768 || k < -32768) {
	  fprintf(stderr,"** bad or missing range for normalize.\n");
	  return 2;
	}
	oper_normalize(j,k);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "cropx")) {
	verbose_apply("cropx");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropx.\n");
	  return 2;
	}
	k = atoi(argv[++i]);
	if (k==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropx.\n");
	  return 2;
	}
	if (j < 0) j = 0;
	if (k >= scene->W || k < 0) k = scene->W - 1;
	if (j >= k) {
	  fprintf(stderr,"** bad cropping range.\n");
	  return 2;
	}
	oper_cropx(j,k);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "cropy")) {
	verbose_apply("cropy");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropy.\n");
	  return 2;
	}
	k = atoi(argv[++i]);
	if (k==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropy.\n");
	  return 2;
	}
	if (j < 0) j = 0;
	if (k >= scene->H || k < 0) k = scene->H - 1;
	if (j >= k) {
	  fprintf(stderr,"** bad cropping range.\n");
	  return 2;
	}
	oper_cropy(j,k);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "cropz")) {
	verbose_apply("cropz");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropz.\n");
	  return 2;
	}
	k = atoi(argv[++i]);
	if (k==0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad or missing parameter for cropz.\n");
	  return 2;
	}
	if (j < 0) j = 0;
	if (k >= scene->D || k < 0) k = scene->D - 1;
	if (j >= k) {
	  fprintf(stderr,"** bad cropping range.\n");
	  return 2;
	}
	oper_cropz(j,k);
	verbose_ok();
	continue;
      }

    }

    /* 1-argument filters */
    if ( (argc - i) > 1 ) {
      
      if (!strcmp(argv[i], "bpv")) {
	verbose_apply("bpv");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) j=1;
	if (j!=8 && j!=16) {
	  fprintf(stderr,"** bpv must be 8 or 16.\n");
	  return 2;
	}
	oper_bpv(j);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "arotx")) {
	verbose_apply("arotx");
	f[0] = atof(argv[++i]);
	if (f[0]==0.0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad parameter for arotx.\n");
	  return 2;
	}
	oper_arotx(f[0]);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "aroty")) {
	verbose_apply("aroty");
	f[0] = atof(argv[++i]);
	if (f[0]==0.0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad parameter for aroty.\n");
	  return 2;
	}
	oper_aroty(f[0]);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "arotz")) {
	verbose_apply("arotz");
	f[0] = atof(argv[++i]);
	if (f[0]==0.0 && !isdigit(argv[i][0])) {
	  fprintf(stderr,"** bad parameter for arotz.\n");
	  return 2;
	}
	oper_arotz(f[0]);
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "rotz")) {
	verbose_apply("rotz");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) j=1;
	while(j<0)   j+=360;
	while(j>=360) j-=360;
	switch(j) {
	case 0:
	  break;
	case 90:
	  oper_rotz90();
	  break;
	case 180:
	  oper_flipx();
	  oper_flipy();
	  break;
	case 270:
	  oper_flipx();
	  oper_flipy();
	  oper_rotz90();
	  break;
	default:
	  fprintf(stderr,"** rotz rotates only by multiples of 90 degrees.\n");
	  return 2;
	}
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "roty")) {
	verbose_apply("roty");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) j=1;
	while(j<0)   j+=360;
	while(j>=360) j-=360;
	switch(j) {
	case 0:
	  break;
	case 90:
	  oper_roty90();
	  break;
	case 180:
	  oper_flipx();
	  oper_flipz();
	  break;
	case 270:
	  oper_flipx();
	  oper_flipz();
	  oper_roty90();
	  break;
	default:
	  fprintf(stderr,"** roty rotates only by multiples of 90 degrees.\n");
	  return 2;
	}
	verbose_ok();
	continue;
      }

      if (!strcmp(argv[i], "rotx")) {
	verbose_apply("rotx");
	j = atoi(argv[++i]);
	if (j==0 && !isdigit(argv[i][0])) j=1;
	while(j<0)   j+=360;
	while(j>=360) j-=360;
	switch(j) {
	case 0:
	  break;
	case 90:
	  oper_rotx90();
	  break;
	case 180:
	  oper_flipy();
	  oper_flipz();
	  break;
	case 270:
	  oper_flipy();
	  oper_flipz();
	  oper_rotx90();
	  break;
	default:
	  fprintf(stderr,"** rotx rotates only by multiples of 90 degrees.\n");
	  return 2;
	}
	verbose_ok();
	continue;
      }


    }

    fprintf(stderr,"** unknown operation: %s\n",argv[i]);
    return 2;

  }

  if (scene) {
    if (verbose) {
      printf("writing %s... ",output); fflush(stdout);
    }
    scn_write(scene, output);
    verbose_ok();
    scn_destroy(scene);
  }
  if (verbose)
    printf("leaving. (no errors)\n");
  return 0;
}
