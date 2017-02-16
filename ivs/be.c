/* -----------------------------------------------------

   IVS - Interactive Volume Segmentation
   (C) 2002-2010
   
   Felipe Paulo Guazzi Bergo 
   <fbergo@gmail.com>

   and
   
   Alexandre Xavier Falcao
   <afalcao@ic.unicamp.br>

   Distribution, trade, publication or bulk reproduction
   of this source code or any derivative works are
   strictly forbidden. Source code is provided to allow
   compilation on different architectures.

   ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "be.h"
#include "msgbox.h"
#include "util.h"
#include "ivs.h"

ArgBlock * ArgBlockNew() {
  ArgBlock *args;
  args = CREATE(ArgBlock);
  if (!args) return 0;
  args->ack = 0;
  args->scene = 0;
  args->sa[0] = 0;
  return args;
}

void ArgBlockDestroy(ArgBlock *args) {
  if (args)
    DESTROY(args);
}

void ArgBlockSendAck(ArgBlock *args) {
  if (args->ack)
    ++ (*(args->ack));
}

#define IVS_BE_PROLOGUE ArgBlock *data = (ArgBlock *) args
#define IVS_BE_EPILOGUE ArgBlockDestroy(data)
#define IVS_BE_ACK      ArgBlockSendAck(data)

void be_load_labels(void *args) {
  IVS_BE_PROLOGUE;
  Volume *v;
  XAnnVolume *xv;
  int i;

  v = VolumeNewFromFile(data->sa);
  if (!v) goto eout;

  if (data->scene->avol) {
    xv = data->scene->avol;
    if (xv->W != v->W ||
	xv->H != v->H ||
	xv->D != v->D)
      goto edim;
    for(i=0;i<v->N;i++)
      xv->vd[i].label = (i8_t) (v->u.i16[i]);
    VolumeDestroy(v);
  }

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 edim:
  ivs_queue_popup("Error","Dimension Mismatch, volumes must have the same size.\n",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
 eout:
  ivs_queue_popup("Error","Unable to load volume data.\n(Bad format ?)",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
}

void be_load_mip_volume(void *args) {
  IVS_BE_PROLOGUE;
  Volume *vol, *tmp;
  int fw,fh,fd;
  int i,j,k;

  vol = VolumeNewFromFile(data->sa);
  if (!vol) goto eout;

  // fix dimensions if needed
  if (data->scene->avol) {
    fw = data->scene->avol->W;
    fh = data->scene->avol->H;
    fd = data->scene->avol->D;

    if (fw != vol->W || fh != vol->H || fd != vol->D) {
      tmp = VolumeNew(fw,fh,fd,vt_integer_16);
      if (!tmp) goto eout;
      for(k=0;k<MIN(fd,vol->D);k++)
	for(j=0;j<MIN(fh,vol->H);j++)
	  for(i=0;i<MIN(fw,vol->W);i++)
	    VolumeSetI16(tmp,i,j,k,VolumeGetI16(vol,i,j,k));
      VolumeDestroy(vol);
      vol = tmp;
      tmp = 0;
    }
  }

  if (data->scene->mipvol != 0) {
    VolumeDestroy(data->scene->mipvol);
    data->scene->mipvol = 0;
  }

  data->scene->mipvol = vol;

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 eout:
  ivs_queue_popup("Error","Unable to load MIP volume data.\n(Bad format ?)",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
}

void be_load_volume(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *xa=0, *tmpxa;
  Volume *v;
  int mvf = 0;

  mvf = IsMVFFile(data->sa);

  if (IsVLAFile(data->sa) || mvf) {

    xa = XAVolLoad(data->sa);

    if (xa) {
      tmpxa = data->scene->avol;
      data->scene->avol = xa;
      if (tmpxa)
	XAVolDestroy(tmpxa);
    }

  } else {

    v = VolumeNewFromFile(data->sa);
    if (!v) goto eout;

    xa = XAVolNewWithVolume(v);

    if (xa) {
      tmpxa = data->scene->avol;
      data->scene->avol = xa;
      if (tmpxa)
	XAVolDestroy(tmpxa);
    }

    VolumeDestroy(v);
  }

  if (xa) {
    data->scene->clip.v[0][0] = 0;
    data->scene->clip.v[1][0] = 0;
    data->scene->clip.v[2][0] = 0;

    data->scene->clip.v[0][1] = xa->W - 1;
    data->scene->clip.v[1][1] = xa->H - 1;
    data->scene->clip.v[2][1] = xa->D - 1;

    if (mvf)
      SceneLoadView(data->scene, data->sa);

  } else
    goto eout;

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 eout:
  ivs_queue_popup("Error","Unable to load volume data.\n(Bad format ?)",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;  
}

void be_load_minc(void *args) {
  IVS_BE_PROLOGUE;
  Volume *v;
  XAnnVolume *xa;
  char tmpname[512], cmd[2048];
  FILE *f;

  sprintf(tmpname,"/tmp/mconv-%d-%d.scn",(int)(getpid()),(int)(time(0)));

  sprintf(cmd,"minctoscn %s %s >/dev/null",data->sa,tmpname);

  if (system(cmd)!=0)
    goto err;
    
  f = fopen(tmpname,"r");
  if (!f) goto err;

  v = VolumeNewFromFile(tmpname);
  if (!v) goto err;
  xa = XAVolNewWithVolume(v);

  if (xa) {
    if (data->scene->avol) {
      XAVolDestroy(data->scene->avol);
      data->scene->avol = 0;
    }
    data->scene->avol = xa;
  }

  VolumeDestroy(v);
  unlink(tmpname);

  if (xa) {
    data->scene->clip.v[0][0] = 0;
    data->scene->clip.v[1][0] = 0;
    data->scene->clip.v[2][0] = 0;

    data->scene->clip.v[0][1] = xa->W - 1;
    data->scene->clip.v[1][1] = xa->H - 1;
    data->scene->clip.v[2][1] = xa->D - 1;
  } else
    goto err;

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 err:
  ivs_queue_popup("Error","Unable to open MINC file. File must be MINC,\n"\
		  "minctoscn, mincinfo and mincextract must be on the\n"\
		  "path and /tmp must be writeable and have enough room\n"\
		  "for a temporary volume.",MSG_ICON_ERROR);  
  IVS_BE_EPILOGUE;
}

void be_segment(void *args) {
  IVS_BE_PROLOGUE;

  if (data->scene->avol) {
    switch(data->ia[0]) {
    case SEG_OP_WATERSHED: 
      //printf ("A\n");
      WaterShedMinus(data->scene->avol); 
      break;
    case SEG_OP_IFT_PLUS_WATERSHED: 
      //printf ("B\n");
      WaterShedPlus(data->scene->avol);  
      break;
    case SEG_OP_FUZZY: 
      //printf ("C\n");
      DFuzzyConn(data->scene->avol, data->ia[1], &(data->fa[0])); 
      break;
    case SEG_OP_STRICT_FUZZY: 
      //printf ("D\n");
      DStrictFuzzyConn(data->scene->avol, data->ia[1], &(data->fa[0])); 
      break;
    case SEG_OP_DMT:
      //printf ("E\n");
      DeltaMinimization(data->scene->avol); 
      break;
    case SEG_OP_LAZYSHED_L1:
      LazyShedL1(data->scene->avol);
      break;
    case SEG_OP_DISCSHED_1:
      DiscShed(data->scene->avol,0);
      break;
    case SEG_OP_DISCSHED_2:
      KappaSeg(data->scene->avol);
      break;
    }
    data->scene->invalid = 1;
    IVS_BE_ACK;
  }

  IVS_BE_EPILOGUE;
}

void be_save_state(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAVolSave(data->scene->avol, data->sa);
    SceneSaveView(data->scene, data->sa);
  }
  IVS_BE_EPILOGUE;
}

void be_clear_annotation(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAVolReset(data->scene->avol);
    data->scene->invalid = 1;
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_threshold(void *args) {
  IVS_BE_PROLOGUE;

  if (data->scene->avol) {
    XA_Threshold_Filter(data->scene->avol,
			data->ia[0],data->ia[1]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_ls(void *args) {
  IVS_BE_PROLOGUE;

  if (data->scene->avol) {
    XA_LS_Filter(data->scene->avol,
		 data->fa[0],data->fa[1]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_lesion(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAPhantomLesion(data->scene->avol,
		    data->ia[0],
		    data->ia[1],
		    data->ia[2],
		    data->ia[3],
		    data->ia[4],
		    data->ia[5],
		    data->ia[6]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_arith(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAArithmetic(data->scene->avol,data->ia[0]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_morph(void *args) {
  IVS_BE_PROLOGUE;
  Volume *se = 0;
  int sz;

  if (data->scene->avol) {
    sz = data->ia[2];
    switch(data->ia[1]) {
    case 0: se = SECross(sz); break;
    case 1: se = SEBox(sz); break;
    default: se = SECross(3);
    }

    switch(data->ia[0]) {
    case 0: XADilate(data->scene->avol, se); break;
    case 1: XAErode(data->scene->avol, se); break;
    }

    if (se)
      SEDestroy(se);
    IVS_BE_ACK;
  }


  IVS_BE_EPILOGUE;
}

void be_filter_gs(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XA_GS_Filter(data->scene->avol, 
		 data->fa[0], data->fa[1]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_mbgs(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XA_MBGS_Filter(data->scene->avol, 
		   data->ia[0],&(data->fa[0]), &(data->fa[5]));
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_conv(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XABlur3x3(data->scene->avol);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_modal(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAModa(data->scene->avol);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_homog(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAHomog(data->scene->avol, data->ia[0]);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}


void be_filter_median(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    XAMedian(data->scene->avol);
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_aff(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {
    switch(data->ia[0]) {
    case 0: /* max affinity */
      XAMaxAffinity(data->scene->avol);
      break;
    case 1: /* min affinity */
      XAMinAffinity(data->scene->avol);
      break;
    }
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_grad(void *args) {
  IVS_BE_PROLOGUE;
  if (data->scene->avol) {

    switch(data->ia[0]) {
    case 0:
      FastMorphologicalGradient(data->scene->avol);
      break;
    case 1:
      FastBoxMorphologicalGradient(data->scene->avol);
      break;
    case 2:
      FastDirectionalMaxGradient(data->scene->avol);
      break;
    case 3:
      FastDirectionalAvgGradient(data->scene->avol);
      break;
    case 4:
      SobelGradient(data->scene->avol);
      break;
    }
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_swap(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *avol;
  Volume *mipvol;
  int i;
  i16_t tmp;

  avol = data->scene->avol;
  mipvol = data->scene->mipvol;

  if (avol!=0 && mipvol!=0) {
    for(i=0;i<avol->N;i++) {
      tmp = avol->vd[i].value;
      avol->vd[i].value = mipvol->u.i16[i];
      mipvol->u.i16[i] = tmp;
    }
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_revert(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *avol;
  int i;

  avol = data->scene->avol;
  if (avol) {
    for(i=0;i<avol->N;i++)
      avol->vd[i].value = avol->vd[i].orig;
    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

void be_filter_bpp_ssr(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *avol;
  float a,b;
  char z[64];

  avol = data->scene->avol;
  if (avol) {
    XABrainGaussianEstimate(avol, data->fa[0], &a, &b);

    sprintf(z,"GSF estimation: mean=%d, stddev=%d",(int)a,(int)b);
    ivs_queue_status(z, 30);

    XA_GS_Filter(avol, a, b);
    XABlur3x3(avol);
    FastMorphologicalGradient(avol);

    IVS_BE_ACK;
  }
  IVS_BE_EPILOGUE;
}

#define BE_LSB(a) (a&0xff)
#define BE_MSB(a) ((a>>8)&0xff)

void be_export_data(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *avol;
  int i;
  FILE *f;
  char *buffer;
  int   bsize, bcount;

  avol = data->scene->avol;

  f=fopen(data->sa,"w");
  if (!f) {
    ivs_queue_popup("Error","Unable to open file for writing.",MSG_ICON_ERROR);
    goto bed_over;
  }

  bsize = 32768;
  buffer = (char *)malloc(bsize);
  bcount = 0;
  if (buffer) {

    if (data->ia[1]) {
      fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n%d\n",
	      avol->W,avol->H,avol->D,
	      avol->dx,avol->dy,avol->dz,
	      (data->ia[0]==0)?8:16);
    }

    switch(data->ia[0]) {
    case 0: // label
      for(i=0;i<avol->N;i++) {
	buffer[bcount++] = (char) (LABELMASK & (avol->vd[i].label));
	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 1: // orig
      for(i=0;i<avol->N;i++) {
	buffer[bcount++] = (char) (BE_LSB(avol->vd[i].orig));
	buffer[bcount++] = (char) (BE_MSB(avol->vd[i].orig));
	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 2: // value
      for(i=0;i<avol->N;i++) {
	buffer[bcount++] = (char) (BE_LSB(avol->vd[i].value));
	buffer[bcount++] = (char) (BE_MSB(avol->vd[i].value));
	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 3: // cost
      for(i=0;i<avol->N;i++) {
	buffer[bcount++] = (char) (BE_LSB(avol->vd[i].cost));
	buffer[bcount++] = (char) (BE_MSB(avol->vd[i].cost));
	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    }

    // flush local buffer
    if (bcount)
      fwrite(buffer, 1, bcount, f);
    fclose(f);
    free(buffer);
  }

 bed_over:
  IVS_BE_EPILOGUE;
}

void be_export_masked_data(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *avol;
  int i, lval;
  FILE *f;
  char *buffer;
  int   bsize, bcount;

  avol = data->scene->avol;

  f=fopen(data->sa,"w");
  if (!f) {
    ivs_queue_popup("Error","Unable to open file for writing.",MSG_ICON_ERROR);
    goto bed_over;
  }

  bsize = 32768;
  buffer = (char *)malloc(bsize);
  bcount = 0;
  if (buffer) {

    if (data->ia[1]) {
      fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n%d\n",
	      avol->W,avol->H,avol->D,
	      avol->dx,avol->dy,avol->dz,
	      (data->ia[0]==0)?8:16);
    }

    switch(data->ia[0]) {
    case 0: // label
      for(i=0;i<avol->N;i++) {
	lval = LABELMASK&(avol->vd[i].label);

	if (data->ia[10+lval])
	  buffer[bcount++] = (char) (LABELMASK & (avol->vd[i].label));
	else
	  buffer[bcount++] = 0;

	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 1: // orig
      for(i=0;i<avol->N;i++) {
	lval = LABELMASK&(avol->vd[i].label);
	
	if (data->ia[10+lval]) {
	  buffer[bcount++] = (char) (BE_LSB(avol->vd[i].orig));
	  buffer[bcount++] = (char) (BE_MSB(avol->vd[i].orig));
	} else {
	  buffer[bcount++] = 0;
	  buffer[bcount++] = 0;
	}

	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 2: // value
      for(i=0;i<avol->N;i++) {
	lval = LABELMASK&(avol->vd[i].label);

	if (data->ia[10+lval]) {
	  buffer[bcount++] = (char) (BE_LSB(avol->vd[i].value));
	  buffer[bcount++] = (char) (BE_MSB(avol->vd[i].value));
	} else {
	  buffer[bcount++] = 0;
	  buffer[bcount++] = 0;
	}

	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    case 3: // cost
      for(i=0;i<avol->N;i++) {
	lval = LABELMASK&(avol->vd[i].label);

	if (data->ia[10+lval]) {
	  buffer[bcount++] = (char) (BE_LSB(avol->vd[i].cost));
	  buffer[bcount++] = (char) (BE_MSB(avol->vd[i].cost));
	} else {
	  buffer[bcount++] = 0;
	  buffer[bcount++] = 0;
	}
	if (bcount==bsize) {
	  fwrite(buffer, 1, bcount, f);
	  bcount=0;
	}
      }
      break;
    }

    // flush local buffer
    if (bcount)
      fwrite(buffer, 1, bcount, f);
    fclose(f);
    free(buffer);
  }

 bed_over:
  IVS_BE_EPILOGUE;
}

void be_save_roots(void *args) {
  IVS_BE_PROLOGUE;
  FILE *f;
  char z[128],y[64];
  int i,c;
  XAnnVolume *avol;
  int n,a,b;

  f = fopen(data->sa, "w");
  if (!f) {
    ivs_queue_popup("Error","Unable to open file for writing.",MSG_ICON_ERROR);
    goto bsr_over;
  }
  
  avol=data->scene->avol;
  
  c=0;

  if (MapSize(avol->addseeds)==0) {
    for(i=0;i<avol->N;i++)
      if (avol->vd[i].pred == 0) {
	++c;
	fprintf(f,"%d\t%d\n",i,LABELMASK&(avol->vd[i].label));
      }
  } else {
    n = MapSize(avol->addseeds);
    for(i=0;i<n;i++) {
      MapGet(avol->addseeds, i, &a, &b);
      ++c;
      fprintf(f,"%d\t%d\n",a,b);
    }
  }
  fclose(f);
  
  ivs_safe_short_filename(data->sa, y, 64);
  sprintf(z,"Saved roots to %s (%d roots).", y, c);
  ivs_queue_status(z, 15);
  
 bsr_over:
  IVS_BE_EPILOGUE;
}

void be_load_seeds(void *args) {
  IVS_BE_PROLOGUE;
  FILE *f;
  XAnnVolume *avol;
  int a,b,c;
  char z[512],y[64];

  f = fopen(data->sa, "r");
  if (!f) {
    ivs_queue_popup("Error","Unable to open file for reading.",MSG_ICON_ERROR);
    goto bls_over;
  }
    
  avol = data->scene->avol;
  MapClear(avol->addseeds);

  c=0;
  while(fgets(z, 511, f)!=0) {
    if (z[0]=='#') continue;
    if (sscanf(z," %d %d \n",&a,&b)==2) {
      if (a < avol->N) {
	MapSet(avol->addseeds, a, b);
	avol->vd[a].label = b;
	++c;
      }
    }
  }

  fclose(f);

  ivs_safe_short_filename(data->sa, y, 64);
  sprintf(z,"%d seeds loaded from %s.",c,y);
  ivs_queue_status(z,15);

  IVS_BE_ACK;

 bls_over:
  IVS_BE_EPILOGUE;
}

void be_clip_volume(void *args) {
  IVS_BE_PROLOGUE;
  int K[6], w,h,d, i,j,k;
  XAnnVolume *xa=0, *tmpxa;
  Volume *dest;

  if (!data->scene->avol) goto ecout;

  K[0] = MIN(MIN(data->ia[0], data->ia[3]), data->scene->avol->W - 1);
  K[1] = MIN(MAX(data->ia[0], data->ia[3]), data->scene->avol->W - 1);

  K[2] = MIN(MIN(data->ia[1], data->ia[4]), data->scene->avol->H - 1);
  K[3] = MIN(MAX(data->ia[1], data->ia[4]), data->scene->avol->H - 1);

  K[4] = MIN(MIN(data->ia[2], data->ia[5]), data->scene->avol->D - 1);
  K[5] = MIN(MAX(data->ia[2], data->ia[5]), data->scene->avol->D - 1);

  w = K[1]-K[0]+1;
  h = K[3]-K[2]+1;
  d = K[5]-K[4]+1;

  dest = VolumeNew(w,h,d, vt_integer_16);
  if (!dest) goto ecout;
  dest->dx = data->scene->avol->dx;
  dest->dy = data->scene->avol->dy;
  dest->dz = data->scene->avol->dz;

  for(i=0;i<d;i++)
    for(j=0;j<h;j++)
      for(k=0;k<w;k++)
	VolumeSetI16(dest, k,j,i,
		     XAGetOrig(data->scene->avol,
			       K[0]+k +
			       data->scene->avol->tbrow[K[2]+j] +
			       data->scene->avol->tbframe[K[4]+i]));

  
  xa = XAVolNewWithVolume(dest);
  
  if (xa) {
    tmpxa = data->scene->avol;
    data->scene->avol = xa;
    if (tmpxa)
      XAVolDestroy(tmpxa);
  }

  VolumeDestroy(dest);

  if (xa) {
    data->scene->clip.v[0][0] = 0;
    data->scene->clip.v[1][0] = 0;
    data->scene->clip.v[2][0] = 0;
    
    data->scene->clip.v[0][1] = xa->W - 1;
    data->scene->clip.v[1][1] = xa->H - 1;
    data->scene->clip.v[2][1] = xa->D - 1;
  } else {
    goto ecout;
  }

  if (data->scene->mipvol) {
    VolumeDestroy(data->scene->mipvol);
    data->scene->mipvol = 0;
    ivs_queue_popup("Warning","Overlay volume unloaded after clipping,\nreload if necessary.",MSG_ICON_WARN);
  }

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 ecout:
  ivs_queue_popup("Error","Unable to clip volume.\n(Out of memory ?)",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
}

void be_filter_registration(void *args) {
  IVS_BE_PROLOGUE;
  Volume *tmp,*xtmp;
  int i;
  double result[12];

  if (data->scene->mipvol) {
    i = SolveRegistration(&result[0], 
			  data->ia[0],data->ia[1],data->ia[2],
			  data->ia[12],data->ia[13],data->ia[14],
			  data->ia[3],data->ia[4],data->ia[5],
			  data->ia[15],data->ia[16],data->ia[17],
			  data->ia[6],data->ia[7],data->ia[8],
			  data->ia[18],data->ia[19],data->ia[20],
			  data->ia[9],data->ia[10],data->ia[11],
			  data->ia[21],data->ia[22],data->ia[23]);
    if (i<0) goto singular;
    
    tmp = VolumeRegistrationTransform(data->scene->mipvol, &result[0]);
    if (!tmp) goto oops;

    xtmp = data->scene->mipvol;
    data->scene->mipvol = tmp;
    VolumeDestroy(xtmp);
  } 

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 singular:
  ivs_queue_popup("Error","Unable to perform registration,\nequation system is singular.",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
  return;
 oops:
  ivs_queue_popup("Error","Unable to perform registration,\nmaybe you ran out of memory.",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
  return;
}

void be_interpolate_volume(void *args) {
  IVS_BE_PROLOGUE;
  XAnnVolume *xa=0, *tmpxa;
  Volume *src, *dest;
  Volume *nmip, *tmp;
  int i,j,k,mw,mh,md;

  if (data->scene->avol) {
    src = VolumeNew(data->scene->avol->W,
		    data->scene->avol->H,
		    data->scene->avol->D,
		    vt_integer_16);
    if (!src) 
      goto eout;
    src->dx = data->scene->avol->dx;
    src->dy = data->scene->avol->dy;
    src->dz = data->scene->avol->dz;
    for(i=0;i<data->scene->avol->N;i++)
      src->u.i16[i] = data->scene->avol->vd[i].value;

    dest = VolumeInterpolate(src, data->fa[0], data->fa[1], data->fa[2]);
    VolumeDestroy(src);
    if (!dest)
      goto eout;

    xa = XAVolNewWithVolume(dest);

    if (xa) {
      tmpxa = data->scene->avol;
      data->scene->avol = xa;
      if (tmpxa)
	XAVolDestroy(tmpxa);
    }

    VolumeDestroy(dest);

    // fix overlay dimensions
    if (data->scene->mipvol) {
      nmip = VolumeNew(data->scene->avol->W,
		       data->scene->avol->H,
		       data->scene->avol->D,
		       vt_integer_16);
      if (nmip) {
	mw = MIN(data->scene->avol->W, data->scene->mipvol->W);
	mh = MIN(data->scene->avol->H, data->scene->mipvol->H);
	md = MIN(data->scene->avol->D, data->scene->mipvol->D);
	for(k=0;k<md;k++)
	  for(j=0;j<mh;j++)
	    for(i=0;i<mw;i++)
	      VolumeSetI16(nmip, i,j,k, 
			   VolumeGetI16(data->scene->mipvol,i,j,k));
	tmp = data->scene->mipvol;
	data->scene->mipvol = nmip;
	VolumeDestroy(tmp);
      }
    }

    if (xa) {
      data->scene->clip.v[0][0] = 0;
      data->scene->clip.v[1][0] = 0;
      data->scene->clip.v[2][0] = 0;
      
      data->scene->clip.v[0][1] = xa->W - 1;
      data->scene->clip.v[1][1] = xa->H - 1;
      data->scene->clip.v[2][1] = xa->D - 1;
    } else {
      goto eout;
    }

  }

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
  return;

 eout:
  ivs_queue_popup("Error","Unable to interpolate volume.\n(Out of memory ?)",MSG_ICON_ERROR);
  IVS_BE_EPILOGUE;
}

void be_cost_threshold(void *args) {
  IVS_BE_PROLOGUE;

  if (data->scene->avol) {
    XACostThreshold(data->scene->avol,
		    data->ia[0],data->ia[1],data->ia[2],data->ia[3]);
    IVS_BE_ACK;
  }

  IVS_BE_ACK;
  IVS_BE_EPILOGUE;
}
