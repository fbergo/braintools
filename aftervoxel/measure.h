
#ifndef MEASURE_H
#define MEASURE_H 1

#define MODE_DIST  0
#define MODE_ANGLE 1

typedef struct _distance {
  int x[2],y[2],z[2];
  int done;
  float value;
} Distance;

void measure_load(const char *filename);
void measure_save(const char *filename);

void       dist_add_point(int x,int y,int z,Volume *v);
void       dist_kill_closer(int x,int y,int z);
int        dist_n();
Distance * dist_get(int index);
void       dist_edit_init(int x,int y,int z);
void       dist_edit_set(int x,int y,int z, Volume *v);
void       dist_compute(int index, Volume *v);
void       dist_kill(int index);
void       dist_kill_all();
void       dist_new();
void       dist_add_full(int x1, int y1, int z1, int x2,int y2, int z2, Volume *v);
int        dist_sane(Distance *d, Volume *v);

typedef struct _angle {
  int x[3],y[3],z[3];
  int done;
  float value;
} Angle;

void       angle_add_point(int x,int y,int z,Volume *v);
void       angle_kill_closer(int x,int y,int z);
int        angle_n();
Angle *    angle_get(int index);
void       angle_edit_init(int x,int y,int z);
void       angle_edit_set(int x,int y,int z, Volume *v);
void       angle_compute(int index, Volume *v);
void       angle_kill(int index);
void       angle_kill_all();
void       angle_new();
int        angle_sane(Angle *a, Volume *v);

typedef struct _georuler {

  int marks, sv, dv, tv, si, di, ti;
  int np, *data;
  float distance;

} GeoRuler;

void georuler_add();
void georuler_set_point(int x,int y,int append);

int georuler_count();
GeoRuler * georuler_get(int index);
void georuler_kill(int index);
void georuler_kill_all();
int  georuler_sane(GeoRuler *g, Volume *v);

void measure_kill_all();

#endif
