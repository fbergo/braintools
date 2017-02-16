
#ifndef RENDER_H
#define RENDER_H 1

#include <libip.h>

#define MAGIC 1000000.0

void render_threshold(CImg *dest, Transform *rot, float panx, float pany,
		      int W,int H,int D, int *map, int mapcount,
		      float *depth, float *depthdual, int color,
		      int smoothness);

void render_quick_threshold(CImg *dest, Transform *rot, float panx, float pany,
			    int W,int H,int D, int *map, int mapcount,
			    float *depth, float *depthdual, int color);

void render_full_octant(CImg *dest, Transform *rot, float panx, float pany,
			Volume *vol,int cx,int cy,int cz,int *lookup,
			float *depth);

void render_real_octant(CImg *dest, Transform *rot, float panx, float pany,
			Volume *vol,int cx,int cy,int cz,int *lookup,
			float *depth);

void render_quick_full_octant(CImg *dest, Transform *rot, float panx,
			      float pany, Volume *vol,int cx,int cy,int cz,
			      int *lookup,float *depth);

void render_labels(CImg *dest, Transform *rot, float panx,float pany,
		   Volume *vol,float *depth);

void render_quick_labels(CImg *dest, Transform *rot, float panx, float pany,
			 Volume *vol, float *depth);


void render_skin(void *args);

void render_octant(void *args);

void render_objects(void *args);

void render_calc_s6_normal(int x,int y,int rw,float *zb,float *out,
			   Vector *light);
float phong_specular(float angcos, int n);

#endif
