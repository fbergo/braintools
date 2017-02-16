
#ifndef VISUALIZATION_H
#define VISUALIZATION_H 1

#include <ip_task.h>
#include <ip_scene.h>

void  RenderScene(Scene *scene, int width,int height, TaskLog *tlog);

void  RenderFrames(Scene *scene, VModeType mode,
		   CImg **xy, CImg **zy, CImg **xz,
		   int fx, int fy, int fz);

/* used by the animations only -- should be called right after RenderScene */
void  RenderClipCube(Scene *scene);

/* return !=0 if scene->amesh.tmpt has a valid triangle mesh
   z-sorted according to scene->amesh.tsort */
int   CalcAMesh(Scene *scene, float rx, float ry, float rz);
float AMeshCos(Scene *scene, int i);

// *zbuffer layers
#define ZBUFFER    0
#define NPROJ      1
#define SEED       2

#define VLARGE    15000000
#define VLARGE_D  60000.0

#endif
