
#define SOURCE_MEASURE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libip.h>
#include "aftervoxel.h"

Distance * dist_data = NULL;
int dist_size = 0, dist_count = 0;
int dist_edit[2]; // index, point

int dist_sane(Distance *d, Volume *v) {
  if (d==NULL || v==NULL) return 0;
  if (d->x[0] < 0 || d->x[0] >= v->W ||
      d->x[1] < 0 || d->x[1] >= v->W ||
      d->y[0] < 0 || d->y[0] >= v->H ||
      d->y[1] < 0 || d->y[1] >= v->H ||
      d->z[0] < 0 || d->z[0] >= v->D ||
      d->z[1] < 0 || d->z[1] >= v->D)
    return 0;
  return 1;
}

void dist_add_point(int x,int y,int z,Volume *v) {
  int d;

  if (x<0 || y<0 || z<0 || x>=v->W || y>=v->H || z>=v->D) return;
  if (dist_count == 0) dist_new();
  if (dist_data[dist_count-1].done == 2) dist_new();

  d = dist_data[dist_count-1].done;
  dist_data[dist_count-1].x[d] = x;
  dist_data[dist_count-1].y[d] = y;
  dist_data[dist_count-1].z[d] = z;
  dist_data[dist_count-1].done++;

  if (dist_data[dist_count-1].done == 2)
    dist_compute(dist_count-1,v);
}

int dist_n() {
  return dist_count;
}

Distance * dist_get(int index) {
  if (index < 0 || index >= dist_count) return NULL;
  return(&dist_data[index]);
}

int sqdist3(int a,int b,int c,int d,int e,int f) {
  return( (a-d)*(a-d) + (b-e)*(b-e) + (c-f)*(c-f) );
}

void dist_edit_init(int x,int y,int z) {
  int i, j, bsd = -1, cd;

  dist_edit[0] = -1;
  dist_edit[1] = -1;

  for(i=0;i<dist_count;i++)
    for(j=0;j<dist_data[i].done;j++) {
      cd = sqdist3(x,y,z,
		   dist_data[i].x[j],dist_data[i].y[j],dist_data[i].z[j]);
      if (bsd < 0 || cd < bsd) {
	bsd = cd;
	dist_edit[0] = i;
	dist_edit[1] = j;
      }
    }  
}

void dist_kill_closer(int x,int y,int z) {
  int i, j, bsd = -1, cd, ci = -1;
  for(i=0;i<dist_count;i++) {
    for(j=0;j<dist_data[i].done;j++) {
      cd = sqdist3(x,y,z,
		   dist_data[i].x[j],dist_data[i].y[j],dist_data[i].z[j]);
      if (bsd < 0 || cd < bsd) {
	bsd = cd;
	ci = i;
      }
    }
  }
  if (ci >= 0)
    dist_kill(ci);
}

void dist_edit_set(int x,int y,int z,Volume *v) {
  if (dist_edit[0] >= 0 && dist_edit[0] < dist_count) {
    dist_data[ dist_edit[0] ].x[dist_edit[1]] = x;
    dist_data[ dist_edit[0] ].y[dist_edit[1]] = y;
    dist_data[ dist_edit[0] ].z[dist_edit[1]] = z;
    dist_compute(dist_edit[0],v);
  }
}

void dist_compute(int index, Volume *v) {
  float a,b,c;
  if (index < 0 || index >= dist_count) return;
  if (dist_data[index].done != 2) return;

  a = (v->dx * (dist_data[index].x[0] - dist_data[index].x[1]));
  b = (v->dy * (dist_data[index].y[0] - dist_data[index].y[1]));
  c = (v->dz * (dist_data[index].z[0] - dist_data[index].z[1]));
  dist_data[index].value = (float) (sqrt(a*a + b*b + c*c));
}

void dist_kill(int index) {
  int i;

  if (index < 0 || index >= dist_count) return;
  if (dist_edit[0]==index) dist_edit[0]=-1;
  if (dist_edit[0]>index) dist_edit[0]--;

  for(i=index;i<dist_count-1;i++) 
    memcpy(&dist_data[i],&dist_data[i+1],sizeof(Distance));
  
  dist_count--;
}

void dist_kill_all() {
  dist_count = 0;
  dist_edit[0] = -1;
}

void dist_new() {

  if (dist_data==NULL || dist_size==0) {
    dist_size = 32;
    dist_data = (Distance *) MemAlloc(sizeof(Distance) * dist_size);
  }

  if (dist_size - dist_count < 1) {
    dist_size += 32;
    dist_data = (Distance *) MemRealloc(dist_data,sizeof(Distance) * dist_size);
  }

  dist_data[dist_count].x[0] = 0;
  dist_data[dist_count].y[0] = 0;
  dist_data[dist_count].z[0] = 0;

  dist_data[dist_count].x[1] = 0;
  dist_data[dist_count].y[1] = 0;
  dist_data[dist_count].z[1] = 0;

  dist_data[dist_count].value = 0.0f;
  dist_data[dist_count].done = 0;

  dist_count++;
}

void dist_add_full(int x1, int y1, int z1, int x2,int y2, int z2, Volume *v) {
  if (dist_count == 0) dist_new();
  if (dist_data[dist_count-1].done == 2) dist_new();
  
  dist_data[dist_count-1].x[0] = x1;
  dist_data[dist_count-1].y[0] = y1;
  dist_data[dist_count-1].z[0] = z1;

  dist_data[dist_count-1].x[1] = x2;
  dist_data[dist_count-1].y[1] = y2;
  dist_data[dist_count-1].z[1] = z2;

  dist_data[dist_count-1].done = 2;
  dist_compute(dist_count-1,v);
}

// angles


Angle * angle_data = NULL;
int angle_size = 0, angle_count = 0;
int angle_edit[2]; // index, point

int angle_sane(Angle *a, Volume *v) {
  if (a==NULL || v==NULL) return 0;
  if (a->x[0] < 0 || a->x[0] >= v->W ||
      a->x[1] < 0 || a->x[1] >= v->W ||
      a->x[2] < 0 || a->x[2] >= v->W ||
      a->y[0] < 0 || a->y[0] >= v->H ||
      a->y[1] < 0 || a->y[1] >= v->H ||
      a->y[2] < 0 || a->y[2] >= v->H ||
      a->z[0] < 0 || a->z[0] >= v->D ||
      a->z[1] < 0 || a->z[1] >= v->D ||
      a->z[2] < 0 || a->z[2] >= v->D)
    return 0;
  return 1;
}

void angle_add_point(int x,int y,int z,Volume *v) {
  int d;

  if (x<0 || y<0 || z<0 || x>=v->W || y>=v->H || z>=v->D) return;
  if (angle_count == 0) angle_new();
  if (angle_data[angle_count-1].done == 3) angle_new();

  d = angle_data[angle_count-1].done;
  angle_data[angle_count-1].x[d] = x;
  angle_data[angle_count-1].y[d] = y;
  angle_data[angle_count-1].z[d] = z;
  angle_data[angle_count-1].done++;

  if (angle_data[angle_count-1].done == 3)
    angle_compute(angle_count-1,v);
}

int angle_n() {
  return(angle_count);
}

Angle * angle_get(int index) {
  if (index < 0 || index >= angle_count) return NULL;
  return(&angle_data[index]);
}

void angle_edit_init(int x,int y,int z) {
  int i, j, bsd = -1, cd;

  angle_edit[0] = -1;
  angle_edit[1] = -1;

  for(i=0;i<angle_count;i++)
    for(j=0;j<angle_data[i].done;j++) {
      cd = sqdist3(x,y,z,
		   angle_data[i].x[j],angle_data[i].y[j],angle_data[i].z[j]);
      if (bsd < 0 || cd < bsd) {
	bsd = cd;
	angle_edit[0] = i;
	angle_edit[1] = j;
      }
    }  
}

void angle_kill_closer(int x,int y,int z) {
  int i, j, bsd = -1, cd, ci = -1;
  for(i=0;i<angle_count;i++) {
    for(j=0;j<angle_data[i].done;j++) {
      cd = sqdist3(x,y,z,
		   angle_data[i].x[j],angle_data[i].y[j],angle_data[i].z[j]);
      if (bsd < 0 || cd < bsd) {
	bsd = cd;
	ci = i;
      }
    }
  }
  if (ci >= 0)
    angle_kill(ci);
}

void angle_edit_set(int x,int y,int z, Volume *v) {
  if (angle_edit[0] >= 0 && angle_edit[0] < angle_count) {
    angle_data[ angle_edit[0] ].x[angle_edit[1]] = x;
    angle_data[ angle_edit[0] ].y[angle_edit[1]] = y;
    angle_data[ angle_edit[0] ].z[angle_edit[1]] = z;
    angle_compute(angle_edit[0],v);
  }
}

void angle_compute(int index, Volume *v) {
  Vector a,b;
  float ang;

  if (index < 0 || index >= angle_count) return;
  if (angle_data[index].done != 3) return;

  a.x = v->dx * (angle_data[index].x[1] - angle_data[index].x[0]);
  a.y = v->dy * (angle_data[index].y[1] - angle_data[index].y[0]);
  a.z = v->dz * (angle_data[index].z[1] - angle_data[index].z[0]);

  b.x = v->dx * (angle_data[index].x[2] - angle_data[index].x[0]);
  b.y = v->dy * (angle_data[index].y[2] - angle_data[index].y[0]);
  b.z = v->dz * (angle_data[index].z[2] - angle_data[index].z[0]);

  VectorNormalize(&a);
  VectorNormalize(&b);
  ang = acos(VectorInnerProduct(&a,&b));
  ang *= 180.0 / M_PI;
  angle_data[index].value = ang;
}

void angle_kill(int index) {
  int i;

  if (index < 0 || index >= angle_count) return;
  if (angle_edit[0]==index) angle_edit[0]=-1;
  if (angle_edit[0]>index) angle_edit[0]--;

  for(i=index;i<angle_count-1;i++) 
    memcpy(&angle_data[i],&angle_data[i+1],sizeof(Angle));
  
  angle_count--;
}

void angle_kill_all() {
  angle_count = 0;
  angle_edit[0] = -1;
}

void angle_new() {
  int i;

  if (angle_data==NULL || angle_size==0) {
    angle_size = 32;
    angle_data = (Angle *) MemAlloc(sizeof(Angle) * angle_size);
  }

  if (angle_size - angle_count < 1) {
    angle_size += 32;
    angle_data = (Angle *) MemRealloc(angle_data,sizeof(Angle) * angle_size);
  }

  for(i=0;i<3;i++) {
    angle_data[angle_count].x[i] = 0;
    angle_data[angle_count].y[i] = 0;
    angle_data[angle_count].z[i] = 0;
  }

  angle_data[angle_count].value = 0.0f;
  angle_data[angle_count].done = 0;

  angle_count++;
}


// geodesic

GeoRuler *gr = NULL;
int grsz = 0;
int grn  = 0;

void georuler_add() {

  if (gr==NULL) {
    grsz = 32;
    gr = (GeoRuler *) MemAlloc(sizeof(GeoRuler) * grsz);
    if (!gr) { error_memory(); return; }   
  } else if (grn == grsz) {
    grsz += 32;
    gr = (GeoRuler *) MemRealloc(gr, sizeof(GeoRuler) * grsz);
    if (!gr) { error_memory(); return; }
  }

  MemSet(&gr[grn],0,sizeof(GeoRuler));
  grn++;
}

void georuler_set_point(int x,int y,int append) {
  int w,h,p,wh,W,H,WH;
  int *rmap;
  Volume *vol;

  int i,j,r,getq,putq,qsz,ci,pnp;
  int pa,pb,pr,px,py,pz,ra,rb,rr,rx,ry,rz,si,di;
  int *q, *pm;
  float *dm, d;

  vol = voldata.original;
  W = vol->W;
  H = vol->H;
  WH = W*H;

  w = v3d.skin.out->W;
  h = v3d.skin.out->H;
  wh = w*h;

  //  printf("georuler_set_point %d %d (%d %d)\n",x,y,w,h);

  if (x<0 || y<0 || x>=w || y>=h) return;

  //  printf("ok\n");

  if (grn == 0 || gr==NULL || append==0) georuler_add();

  rmap = skin_rmap(w,h,&(v3d.rotation),
		   v3d.panx / v3d.zoom,v3d.pany / v3d.zoom,
		   vol->W,vol->H,vol->D,
		   v3d.skin.tmap,v3d.skin.tcount);

  p = rmap[x + y*w];
  ci = grn - 1;
  // printf("p=%d\n",p);
  if (p>=0) {
    if (gr[ci].marks == 0) {

      gr[ci].sv = gr[ci].dv = gr[ci].tv = p;
      gr[ci].si = gr[ci].di = gr[ci].ti = x+y*w;
      gr[ci].distance = 0.0;
      gr[ci].data = NULL;
      gr[ci].np = 0;

    } else {

      gr[ci].tv = gr[ci].dv;
      gr[ci].ti = gr[ci].di;

      gr[ci].dv = p;
      gr[ci].di = x+y*w;
    }
    gr[ci].marks++;

    // printf("grn=%d p=%d i=%d marks=%d\n",grn,p,x+y*w,gr[ci].marks);
    
    if (gr[ci].marks >= 2) {
      
      qsz = 8*wh;
      q = (int *) MemAlloc(sizeof(int) * qsz);
      dm = (float *) MemAlloc(sizeof(float) * wh);
      pm = (int *) MemAlloc(sizeof(int) * wh);
      
      for(i=0;i<wh;i++) {
	dm[i] = sqrt(2 * (w*w + h*h + w*w));
	pm[i] = -1;
      }

      si = gr[ci].ti;
      di = gr[ci].di;

      //printf("search is ci=%d si=%d di=%d\n",ci,si,di);

      dm[ si ] = 0.0;
      pm[ si ] = -1;
      q[0] = si;
      putq = 1;
      getq = 0;

      while(getq!=putq) {
	p = q[getq++];
	getq %= qsz;
	
	pa = p % w;
	pb = p / w;

	pr = rmap[p];
	if (pr < 0) continue;

	pz = pr / WH;
	py = (pr % WH) / W;
	px = (pr % WH) % W;

	for(i=-1;i<=1;i++)
	  for(j=-1;j<=1;j++) {
	    if (i==0 && j==0) continue;
	    if (pa+i < 0 || pa+i >= w || pb+j < 0 || pb+j >= h) continue;

	    ra = pa + i;
	    rb = pb + j;
	    r = ra + w*rb;

	    rr = rmap[r];
	    if (rr < 0) continue;
	    
	    rz = rr / WH;
	    ry = (rr % WH) / W;
	    rx = (rr % WH) % W;

	    d = dm[p] + sqrt((px-rx)*(px-rx) + (py-ry)*(py-ry) + (pz-rz)*(pz-rz));
	    if (d < dm[r]) {
	      dm[r] = d;
	      pm[r] = p;
	      q[putq++] = r;
	      putq %= qsz;
	    }
	  }
      }

      // find path backtracking from di
      j = 1;
      p = di;
      while(pm[p]>0) {
	j++;
	p = pm[p];
      }

      //printf("np=%d path j=%d\n",gr[ci].np, j);

      pnp = gr[ci].np;
      gr[ci].np += j;

      if (gr[ci].data == NULL)
	gr[ci].data = (int *) MemAlloc(sizeof(int) * gr[ci].np);
      else
	gr[ci].data = (int *) MemRealloc(gr[ci].data, sizeof(int) * gr[ci].np);

      for(i=j-1, p=di;i>=0 && p>0;i--) {
	gr[ci].data[pnp+i] = rmap[p];
	p = pm[p];
      }
      
      gr[ci].distance += dm[di] * vol->dx;

      MemFree(q);
      MemFree(dm);
      MemFree(pm);
    }
  }
  
  MemFree(rmap);
}

int georuler_count() {
  return grn;
}

GeoRuler * georuler_get(int index) {
  if (index < 0 || index >= grn) return NULL;
  return(&gr[index]);
}

void georuler_kill(int index) {
  if (index < 0 || index >= grn) return;
  if (gr[index].data != NULL) MemFree(gr[index].data);
  if (index < grn-1)
    memcpy(&gr[index],&gr[grn-1],sizeof(GeoRuler));
  grn--;
}

void georuler_kill_all() {
  while(grn > 0)
    georuler_kill(grn-1);
}

void measure_kill_all() {
  dist_kill_all();
  angle_kill_all();
  georuler_kill_all();
}

int georuler_sane(GeoRuler *g, Volume *v) {
  int i;
  if (g==NULL || v==NULL) return 0;
  if (g->data == NULL) return 0;
  
  for(i=0;i<g->np;i++)
    if (g->data[i] < 0 || g->data[i] >= v->N)
      return 0;

  return 1;
}

void measure_load(const char *filename) {
  FILE *f;
  char msg[1024];
  char line[8192], *p;
  Distance td;
  Angle ta;
  GeoRuler tg;
  static const char *sep = " \r\t\n";
  int i,di=0,ai=0,gi=0,j;

  f = fopen(filename, "r");
  if (f == NULL) {
    sprintf(msg,"Unable open %s for reading.",filename);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    return;
  }

  measure_kill_all();

  while(fgets(line,8191,f)!=NULL) {
    p = strtok(line,sep);
    if (p==NULL) break;

    //printf("D=%d A=%d G=%d\n",dist_count,angle_count,grn);

    if (!strcmp(p, "D")) {
      p = strtok(NULL, sep); if (p==NULL) continue; td.x[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.y[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.z[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.x[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.y[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.z[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; td.value = atof(p);
      
      td.done = 2;
      if (!dist_sane(&td,voldata.original)) { di++; continue; }

      dist_new();
      memcpy(&dist_data[dist_count-1],&td,sizeof(Distance));
      dist_compute(dist_count-1, voldata.original);
      continue;
    }
    if (!strcmp(p, "A")) {
      p = strtok(NULL, sep); if (p==NULL) continue; ta.x[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.y[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.z[0] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.x[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.y[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.z[1] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.x[2] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.y[2] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.z[2] = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; ta.value = atof(p);
      
      ta.done = 3;
      if (!angle_sane(&ta,voldata.original)) { ai++; continue; }

      angle_new();
      memcpy(&angle_data[angle_count-1],&ta,sizeof(Angle));
      angle_compute(angle_count-1, voldata.original);
      continue;
    }
    if (!strcmp(p, "G")) {
      p = strtok(NULL, sep); if (p==NULL) continue; tg.marks = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.sv = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.dv = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.tv = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.si = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.di = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.ti = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.np = atoi(p);
      p = strtok(NULL, sep); if (p==NULL) continue; tg.distance = atof(p);

      tg.data = (int *) MemAlloc(sizeof(int) * tg.np);
      if (tg.data == NULL) continue;
      for(i=0;i<tg.np;i++) {
	p = strtok(NULL, sep); if (p==NULL) break; tg.data[i] = atoi(p);
      }
      if (i!=tg.np || p==NULL) {
	MemFree(tg.data);
	continue;
      }
      if (!georuler_sane(&tg,voldata.original)) { gi++; continue; }

      georuler_add();
      memcpy(&gr[grn-1],&tg,sizeof(GeoRuler));
      continue;
    }
  }
  fclose(f);

  j = di+ai+gi;
  
  if (j==0) {
    sprintf(msg,"Loaded %d measurements from %s.",grn+dist_count+angle_count,filename);
  } else {
    sprintf(msg,"Loaded %d measurements from %s (%d inconsistent measurements ignored).",grn+dist_count+angle_count,filename,j);
  }
  app_set_status(msg,30);
}

void measure_save(const char *filename) {
  FILE *f;
  char msg[1024];
  int i,j;

  f = fopen(filename,"w");
  if (f == NULL) {
    sprintf(msg,"Unable to open %s for writing.",filename);
    app_pop_message("Error",msg,MSG_ICON_ERROR);
    return;
  }

  for(i=0;i<dist_count;i++)
    if (dist_data[i].done == 2) {
      fprintf(f,"D %d %d %d %d %d %d %.6f\n",
	      dist_data[i].x[0],
	      dist_data[i].y[0],
	      dist_data[i].z[0],
	      dist_data[i].x[1],
	      dist_data[i].y[1],
	      dist_data[i].z[1],
	      dist_data[i].value);
    }

  for(i=0;i<angle_count;i++)
    if (angle_data[i].done == 3) {
      fprintf(f,"A %d %d %d %d %d %d %d %d %d %.6f\n",
	      angle_data[i].x[0],
	      angle_data[i].y[0],
	      angle_data[i].z[0],
	      angle_data[i].x[1],
	      angle_data[i].y[1],
	      angle_data[i].z[1],
	      angle_data[i].x[2],
	      angle_data[i].y[2],
	      angle_data[i].z[2],
	      angle_data[i].value);
    }

  for(i=0;i<grn;i++) {
    fprintf(f,"G %d %d %d %d %d %d %d %d %.6f ",
	    gr[i].marks, gr[i].sv, gr[i].dv, gr[i].tv, gr[i].si, gr[i].di, gr[i].ti, gr[i].np, gr[i].distance);
    for(j=0;j<gr[i].np;j++)
      fprintf(f,"%d ",gr[i].data[j]);
    fprintf(f,"\n");
  }

  fclose(f);

  sprintf(msg,"Saved %d measurements to %s.",grn+dist_count+angle_count,filename);
  app_set_status(msg,20);
}
