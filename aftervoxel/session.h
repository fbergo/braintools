
#ifndef SESSION_H
#define SESSION_H 1

#include <libip.h>
#include <zlib.h>

typedef struct _aftervoxel_session {

  char filename[512];

} AftervoxelSession;

void session_new();
void session_new_inner();
void session_open();
void session_save();
void session_save_as();

void session_save_to(char *path);
void session_load_from(char *path);
void session_bgsave(void *args);
void session_bgload(void *args);

void av_write_tag(gzFile f, char *tag);
void av_write_int(gzFile f, int val);
void av_write_bool(gzFile f, int val);
void av_write_float(gzFile f, float val);
void av_write_string(gzFile f, char *s);
void av_write_transform(gzFile f, Transform *t);
void av_write_vector(gzFile f, Vector *v);

int   av_read_tag(gzFile f, char *tag);
int   av_read_int(gzFile f, int *err);
int   av_read_bool(gzFile f, int *err);
float av_read_float(gzFile f, int *err);
void  av_read_string(gzFile f, char *s, int maxlen, int *err);
char *av_read_string_alloc(gzFile f, int *err);
void  av_read_transform(gzFile f, Transform *t, int *err);
void  av_read_vector(gzFile f, Vector *v, int *err);

int VolCmp(Volume *a, Volume *b);

#endif
