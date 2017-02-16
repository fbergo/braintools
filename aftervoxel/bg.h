
#ifndef BG_H
#define BG_H 1

#include <libip.h>

/* generic argument block */
typedef struct _argblock {
  Volume       **vols[4]; /* up to 4 volume pointers */
  volatile int  *ack;     /* will be incremented upon completion of task */
  int            ia[32];  /* up to 32 integer arguments */
  float          fa[32];  /* up to 32 float arguments */
  char           sa[512]; /* 1 string argument */
} ArgBlock;

ArgBlock * ArgBlockNew();
void       ArgBlockDestroy(ArgBlock *args);
void       ArgBlockSendAck(ArgBlock *args);

void bg_load_volume(void *args);
void bg_erase_segmentation(void *args);
void bg_load_segmentation(void *args);
void bg_save_segmentation(void *args);
void bg_seg_tp2007(void *args);

void error_volume_load();
void error_volume_save();
void error_memory();
void warn_no_volume();

#endif
