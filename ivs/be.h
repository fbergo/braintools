
#ifndef IVS_BE_H
#define IVS_BE_H 1

#include <libip.h>

/* generic argument block */
typedef struct _argblock {
  Scene *scene;  /* scene to work on */
  int   *ack;    /* will be incremented upon completion of task */
  int   ia[32];  /* up to 32 integer arguments */
  float fa[32];  /* up to 32 float arguments */
  char  sa[512]; /* 1 string argument */
} ArgBlock;

ArgBlock * ArgBlockNew();
void       ArgBlockDestroy(ArgBlock *args);
void       ArgBlockSendAck(ArgBlock *args);

/* comments after/above declarations tell how ArgBlock are
   used by the call. Scene and Ack are (almost) always used,
   so they are not mentioned. */

void be_load_volume(void *args);        /* sa <=> name */
void be_load_mip_volume(void *args);    /* sa <=> name */
void be_load_labels(void *args);        /* sa <=> name */
void be_load_minc(void *args);          /* sa <=> name */
void be_segment(void *args);            /* ia[0] <=> method */
void be_save_state(void *args);         /* sa <=> name */
void be_clear_annotation(void *args);   /* -- */

void be_filter_ls(void *args);          /* fa[0] <=> B, fa[1] <=> C */
void be_filter_gs(void *args);          /* fa[0] <=> mean, fa[1] <=> stddev */
void be_filter_mbgs(void *args);        /* ia[0]=bands, fa[0..4] <=> means, 
                                                        fa[5..9] <=> stddevs */
void be_filter_threshold(void *args);   /* ia[0..1] <=> window range */

void be_filter_conv(void *args);        /* -- */
void be_filter_modal(void *args);       /* -- */
void be_filter_median(void *args);      /* -- */
void be_filter_grad(void *args);        /* ia[0] <=> algorithm */
void be_filter_revert(void *args);      /* -- */
void be_filter_swap(void *args);        /* -- */
void be_filter_bpp_ssr(void *args);     /* fa[0] <=> bpp-ssr openness perc. */
void be_filter_lesion(void *args);      /* ia[0..6]: radius,mean,noise,border,x,y,z */
void be_filter_registration(void *args); /* ia[0..23]: {x,y,z} of 4 points in
                                                source, followed by 4 points
                                                in the overlay */
void be_filter_homog(void *args);        /* -- */
void be_filter_morph(void *args);        /* ia[0]=operation, ia[1]=shape,
                                               ia[2]=size */
void be_filter_arith(void *args);        /* ia[0]=operation */

void be_export_data(void *args);        /* ia[0] <=> what, ia[1] <=> SCNhdr,
                                           sa <=> name */
void be_export_masked_data(void *args); /* ia[0] <=> what, ia[1] <=> SCNhdr,
					   ia[10]..ia[19] <=> object mask
                                           sa <=> name */

void be_save_roots(void *args);         /* sa <=> name */
void be_load_seeds(void *args);         /* sa <=> name */

void be_interpolate_volume(void *args); /* fa[0..2] <=> dx,dy,dz */
void be_clip_volume(void *args);        /* ia[0..5] <=> ax,ay,ax,bx,by,bz */
void be_filter_aff(void *args);         /* ia[0] <=> algorithm */

void be_cost_threshold(void *args);     /* ia[0]: min, ia[1]: max, 
                                           ia[2]: orig(0), current (1) 
                                           ia[3]: invert sense */

#endif
