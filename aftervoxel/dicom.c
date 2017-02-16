#define SOURCE_DICOM_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <libip.h>
#include "aftervoxel.h"

#undef VERBOSEDEBUG
//#define VERBOSEDEBUG

static int endianflip = 0;

typedef struct _vr {
  char id[3];
  int  maxl;
  int  fixed;
  int  longlen;
} VR;

/* tipos de dados dos campos aceitos pelo DICOM 3 */
static VR DicomVR[26] = {
  { "AE",    16, 0 , 0 }, // 0
  { "AS",     4, 1 , 0 },
  { "AT",     4, 1 , 0 }, // 2
  { "CS",    16, 0 , 0 },
  { "DA",     8, 1 , 0 }, // 4
  { "DS",    16, 0 , 0 },
  { "DT",    26, 0 , 0 }, // 6
  { "FL",     4, 1 , 0 },
  { "FD",     8, 1 , 0 }, // 8
  { "IS",    12, 0 , 0 },
  { "LO",    64, 0 , 0 }, // 10
  { "LT", 10240, 0 , 0 },
  { "OB",     0, 0 , 1 }, // 12
  { "OW",     0, 0 , 1 },
  { "PN",    64, 0 , 0 }, // 14
  { "SH",    16, 0 , 0 },
  { "SL",     4, 1 , 0 }, // 16
  { "SQ",     0, 0 , 1 },
  { "SS",     2, 1 , 0 }, // 18
  { "ST",  1024, 0 , 0 },
  { "TM",    16, 0 , 0 }, // 20
  { "UI",    64, 0 , 0 },
  { "UL",     4, 1 , 0 }, // 22
  { "UN",     0, 0 , 1 },
  { "US",     2, 1 , 0 }, // 24
  { "UT",     0, 0 , 1 },
};

double robust_atof(char *p) {
  while( (*p) < 33)
    ++p;
  return(atof(p));
}

float DicomSliceDistance(DicomSlice *a, DicomSlice *b) {
  float d;
  if (!a || !b) return 0.0;
  if (! (a->fakezpos)) {
    return(fabs(b->zpos - a->zpos));
  } else if (a->slicedist != 0.0) {    
    d = (a->slicedist + b->slicedist) / 2.0;
    return d;
  } else {
    d = (a->thickness + b->thickness) / 2.0;
    return d;
  }
}

int DicomSliceIsDupe(DicomSlice *a,DicomSlice *b) {
  if (strcmp(a->field[DICOM_FIELD_STUDYID],b->field[DICOM_FIELD_STUDYID])) return 0;
  if (a->zpos!=0.0 && a->zpos != b->zpos) return 0;
  if (a->imgnumber!=0 && a->imgnumber != b->imgnumber) return 0;
  if (a->echonumber!=0 && a->echonumber != b->echonumber) return 0;
  return 1;
}

void DicomSliceCopy(DicomSlice *dest, DicomSlice *src) {
  int i;
  MemCpy(dest,src,sizeof(DicomSlice));
  dest->image = 0;
  for(i=0;i<9;i++)
    dest->field[i] = 0;
  for(i=0;i<9;i++)
    DicomSliceSet(dest,i,src->field[i]);
}

DicomSlice * DicomSliceNew() {
  DicomSlice *ds;
  int i;

  ds = (DicomSlice *) MemAlloc(sizeof(DicomSlice));
  if (!ds) return 0;

  for(i=0;i<9;i++) ds->field[i] = 0;
  ds->W = ds->H = ds->D = 0;
  ds->xsize = ds->ysize = ds->zsize = 0.0;
  ds->fakezpos = 0;
  ds->zpos = 0.0;
  ds->imgoffset = 0;
  ds->image = 0;
  ds->imgnumber = 0;
  ds->echonumber = 0;
  ds->thickness = 0.0;
  ds->slicedist = 0.0;
  ds->signedness = 1;
  for(i=0;i<3;i++)
    ds->origin[i] = 0.0;
  return ds;
}

void DicomSliceDestroy(DicomSlice *ds) {
  int i;
  if (ds) {
    for(i=0;i<9;i++)
      if (ds->field[i]) MemFree(ds->field[i]);
    if (ds->image)
      MemFree(ds->image);
    MemFree(ds);
  }
}

void DicomSliceSet(DicomSlice *ds, int i, const char *s) {
  if (ds->field[i] != NULL)
    MemFree(ds->field[i]);
  ds->field[i] = (char *) MemAlloc(strlen(s) + 1);
  if (!ds->field[i]) return;
  strncpy(ds->field[i], s, strlen(s)+1);
}

char * DicomSliceGet(DicomSlice *ds, int i) {
  return(ds->field[i]);
}

void   DicomSliceLoadImage(DicomSlice *ds) {
  FILE *f;
  int i,j,k,N,WH,W;
  i8_t *tmp;

  f = fopen(ds->field[DICOM_FIELD_FILENAME],"r");
  if (!f) return;

  if (fseek(f,ds->imgoffset,SEEK_SET)!=0) {
    fclose(f);
    return;
  }

  if (ds->image) {
    MemFree(ds->image);
    ds->image = 0;
  }
  N = ds->W * ds->H * ds->D;
  ds->image = (i16_t *) MemAlloc(N * sizeof(i16_t));
  if (!ds->image) {
    fclose(f);
    return;
  }

  WH = ds->W * ds->H;
  W = ds->W;

  switch(ds->bpp) {
  case 8:
    tmp = (i8_t *) MemAlloc(ds->W);
    if (!tmp) break;
    for(k=0;k<ds->D;k++) {
      for(i=0;i<ds->H;i++) {
	fread(tmp, 1, ds->W, f);
	for(j=0;j<ds->W;j++)
	  ds->image[WH*k + W*i + j] = (i16_t) tmp[j];
      }
    }
    MemFree(tmp);
    break;
  case 16:
    for(k=0;k<ds->D;k++)
      for(i=0;i<ds->H;i++)
	fread(&(ds->image[WH*k + W*i]), 2, ds->W, f);
    break;
  }

  // convert from big endian data if needed
  if (ds->bpp == 16 && endianflip)
    for(i=0;i<N;i++)
      ds->image[i] = ((ds->image[i] & 0xff) << 8) | ((ds->image[i] & 0xff00) >> 8);
  
  // apply scaling
  for(i=0;i<N;i++)
    ds->image[i] = (i16_t) (ds->scale[0] + ds->scale[1] * ds->image[i]);

  ds->maxval = ds->minval = ds->image[0];
  for(i=1;i<N;i++) {
    ds->maxval = MAX(ds->maxval, ds->image[i]);
    ds->minval = MIN(ds->minval, ds->image[i]);
  }

  fclose(f);
}

void   DicomSliceDiscardImage(DicomSlice *ds) {
  if (ds->image) {
    MemFree(ds->image);
    ds->image = 0;
  }
}

void DicomFloatParse(const char *data, float *dest, int n) {
  char buf[256];
  char *p;
  int i;

  strncpy(buf,data,256);
  p = strtok(buf,"\\");
  i = 0;
  while(n) {
    if (!p) return;
    dest[i++] = robust_atof(p);
    --n;
    p = strtok(0,"\\");
  }
}

// fix 20041217 to 2004.12.17
void DicomFixDate(DicomField *vdate) {
  char buf[128];
  char *p;
  p = vdate->data;
  if (strlen(p)>=8)
    if (p[4] >= '0' && p[4] <= '9') {
      snprintf(buf,128,"%c%c%c%c.%c%c.%c%c",
	       p[0],p[1],p[2],p[3],
	       p[4],p[5],p[6],p[7]);
      strncpy(vdate->data, buf, 256);
      vdate->len = strlen(buf);
    }
}

void DicomFixTime(DicomField *vtime) {
  char buf[128];
  char *p;
  p = vtime->data;
  if (strlen(p)>=6)
    if (!strchr(p, ':')) {
      snprintf(buf,128,"%c%c:%c%c",p[0],p[1],p[2],p[3]);
      strncpy(vtime->data, buf, 256);
      vtime->len = strlen(buf);
    }
}

DicomSlice * ReadDicomSlice(const char *filename) {
  FILE *f;
  char buf[4096], *p;
  DicomField *df, *col, *row, *zpos, *pixeldata, *bits, *spacing, *mfnum;
  DicomField *thickness, *manuf, *model, *institution, *modality, *seqname, *sopuid, *echonum;
  DicomField *vdate, *vtime, *studyid,*name,*imgpos,*imgnum,*slicedist,*sc1,*sc2,*signedness;
  DicomSlice *ds;
  int dicomdebug = 0;
  long seekstart = 132;

  f = fopen(filename,"r");
  if (!f) {
    return NULL;
  }

  p = getenv("AV_DICOM_DEBUG");
  if (p!=0)
    dicomdebug = atoi(p);

  if (dicomdebug) printf("ReadDicomSlice %s\n",filename);

  /* check if NEMA header is present (DICM signature at offset 128) */
  if (fseek(f,128,SEEK_SET)!=0) return 0;
  if (fread(buf,1,4,f)!=4) return 0;
  buf[4]=0;
  if (strcmp(buf,"DICM")!=0) {
    fseek(f,0,SEEK_SET); /* no signature */
    seekstart = 0;
  }
  
  /* read all fields */
  endianflip = 0;
  df = NULL;
  while( ReadDicomField(f, &df) != 0 ) ;

  // deal with big endian files, see dicom2scn.c for better comments
  if (df!=NULL && df->next==NULL && (df->len & 0xff) == 0) {
    DFKill(df);
    fseek(f,seekstart,SEEK_SET);
    endianflip = 1;
    df = NULL;
    while( ReadDicomField(f, &df) != 0 ) ;
  }

  fclose(f);
  if (!df) {
    return NULL;
  }

  sopuid      = DicomGetField(df, 0x0008, 0x0018);
  echonum     = DicomGetField(df, 0x0018, 0x0086);
  col         = DicomGetField(df, 0x0028, 0x0011);
  row         = DicomGetField(df, 0x0028, 0x0010);
  mfnum       = DicomGetField(df, 0x0028, 0x0008);
  zpos        = DicomGetField(df, 0x0020, 0x1041);
  pixeldata   = DicomGetField(df, 0x7fe0, 0x0010);
  bits        = DicomGetField(df, 0x0028, 0x0100);
  spacing     = DicomGetField(df, 0x0028, 0x0030);
  thickness   = DicomGetField(df, 0x0018, 0x0050);
  manuf       = DicomGetField(df, 0x0008, 0x0070);
  model       = DicomGetField(df, 0x0008, 0x1090);
  institution = DicomGetField(df, 0x0008, 0x0080);
  modality    = DicomGetField(df, 0x0008, 0x0060);
  vdate       = DicomGetField(df, 0x0008, 0x0020);
  vtime       = DicomGetField(df, 0x0008, 0x0030);
  studyid     = DicomGetField(df, 0x0020, 0x000E);
  name        = DicomGetField(df, 0x0010, 0x0010);
  imgpos      = DicomGetField(df, 0x0020, 0x0032);
  imgnum      = DicomGetField(df, 0x0020, 0x0013);
  slicedist   = DicomGetField(df, 0x0018, 0x0088);
  signedness  = DicomGetField(df, 0x0028, 0x0103);

  seqname = DicomGetField(df, 0x0008, 0x103e);
  if (!seqname) seqname = DicomGetField(df, 0x0018, 0x1030);
  if (!seqname) seqname = DicomGetField(df, 0x0018, 0x0020);

  /*
  if (!col) printf("missing 1\n");
  if (!row) printf("missing 2\n");
  if (!pixeldata) printf("missing 3\n");
  if (!manuf) printf("missing 4\n");
  if (!model) printf("missing 5\n");
  if (!institution) printf("missing 6\n");
  if (!modality) printf("missing 7\n");
  if (!vdate) printf("missing 8\n");
  if (!vtime) printf("missing 9\n");
  if (!studyid) printf("missing 10\n");
  if (!name) printf("missing 11\n");
  if (!spacing) printf("missing 12\n");
  */

  if (col && row && 
      (zpos || (imgpos && imgnum) || (imgnum && thickness) || (imgnum && slicedist) || (mfnum && slicedist)) && 
      pixeldata && manuf && model &&
      institution && modality && vdate && vtime && 
      studyid && name && spacing) {

    // fix date and time if needed
    DicomFixDate(vdate);
    DicomFixTime(vtime);

    ds = DicomSliceNew();
    DicomSliceSet(ds, DICOM_FIELD_PATIENT, name->data);
    DicomSliceSet(ds, DICOM_FIELD_INSTITUTION, institution->data);
    snprintf(buf,4096,"%s / %s",manuf->data,model->data);
    DicomSliceSet(ds, DICOM_FIELD_EQUIP, buf);

    seqname = DicomGetField(df, 0x0008,0x103e);
    if (seqname == NULL)
      seqname = DicomGetField(df, 0x0018,0x1030);
    if (seqname == NULL)
      seqname = DicomGetField(df, 0x0018,0x0024);

    if (seqname!=NULL)
      snprintf(buf,4096,"%s (%s)",seqname->data, modality->data);
    else
      snprintf(buf,4096,"(%s)",modality->data);

    DicomSliceSet(ds, DICOM_FIELD_MODALITY, buf);
    DicomSliceSet(ds, DICOM_FIELD_STUDYID, studyid->data);
    DicomSliceSet(ds, DICOM_FIELD_FILENAME, filename);
    DicomSliceSet(ds, DICOM_FIELD_DATE, vdate->data);
    DicomSliceSet(ds, DICOM_FIELD_TIME, vtime->data);
    if (sopuid)
      DicomSliceSet(ds, DICOM_FIELD_SOPUID, sopuid->data);
    if (echonum)
      ds->echonumber = atoi(echonum->data);
    ds->W = atoi(col->data);
    ds->H = atoi(row->data);
    
    if (mfnum == NULL)
      ds->D = 1;
    else
      ds->D = atoi(mfnum->data);

    if (ds->D == 1) {
      if (zpos) {
	ds->zpos = atof(zpos->data);
	if (!imgnum)
	  ds->imgnumber = (int) (1000.0*ds->zpos);
	ds->origin[0] = ds->origin[1] = 0.0;
	ds->origin[2] = ds->zpos;
      }
    }

    if (imgnum) {
      ds->imgnumber = atoi(imgnum->data);
      //printf("file=%s setting imgnumber = %d (%x)\n",filename,ds->imgnumber,ds->imgnumber);
    }

    if (imgpos) {
      DicomFloatParse(imgpos->data, ds->origin, 3);
      if (!zpos) {
	ds->zpos = (float) (ds->imgnumber);
	ds->fakezpos = 1;
      }
    }

    if (thickness)
      ds->thickness = robust_atof(thickness->data);

    if (slicedist)
      ds->slicedist = robust_atof(slicedist->data);

    if (signedness) {
      ds->signedness = !(atoi(signedness->data));
      //printf("file=%s s=%d\n",filename,ds->signedness);
    }

    ds->imgoffset = (unsigned int) atoi(pixeldata->data);
    ds->bpp = (bits ? atoi(bits->data) : 16);
    strncpy(buf,spacing->data, 4096);
    p=strtok(buf,"\\");
    if (p) {
      ds->xsize = robust_atof(p);
      p = strtok(0,"\\");
      if (p) ds->ysize = robust_atof(p);
    }

    ds->zsize = ds->slicedist;
    if (ds->slicedist == 0.0 && ds->thickness != 0.0)
      ds->zsize = ds->thickness;
      
    // intensity scaling
    ds->scale[0] = 0.0; ds->scale[1] = 1.0;
    sc1 = DicomGetField(df, 0x0028, 0x1052);
    if (sc1 != NULL) ds->scale[0] = robust_atof(sc1->data);
    sc2 = DicomGetField(df, 0x0028, 0x1053);
    if (sc2 != NULL) ds->scale[1] = robust_atof(sc2->data);

    DFKill(df);
    //if (dicomdebug)
    //  printf("success on %s\n",filename);
    return ds;
  } else {
    if (pixeldata!=0 && dicomdebug>0) {
      printf("ReadDicomSlice failed on %s\n",filename);
      PrintDicomField("col",col);
      PrintDicomField("row",row);
      PrintDicomField("zpos",zpos);
      PrintDicomField("bits",bits);
      PrintDicomField("spacing",spacing);
      PrintDicomField("thickness",thickness);
      PrintDicomField("manuf",manuf);
      PrintDicomField("model",model);
      PrintDicomField("institution",institution);
      PrintDicomField("modality",modality);
      PrintDicomField("seqname",seqname);
      PrintDicomField("vdate",vdate);
      PrintDicomField("vtime",vtime);
      PrintDicomField("studyid",studyid);
      PrintDicomField("name",name);
      PrintDicomField("imgpos",imgpos);
      PrintDicomField("imgnum",imgnum);
      PrintDicomField("slicedist",slicedist);
      printf("** end of debug report\n");
    }
  }

  if (df)
    DFKill(df);
  return NULL;
}

void PrintDicomField(char *label, DicomField *df) {
  if (df) {
    printf("[%.4x:%.4x] %-16s value(%d)=%s\n",
	   df->group,df->key,label,df->len,df->data);
  } else {
    printf("Field %s not present\n",label);
  }
}

int ReadDicomField(FILE *f, DicomField **df) {
  DicomField *x;
  unsigned char b[256];
  int i,clen;

  x = DFNew();
  if (fread(b,1,4,f)!=4) {
    DFKill(x);
    return 0;
  }

  x->group = ConvInt16(b);
  x->key   = ConvInt16(&b[2]);

  if (fread(b,1,4,f)!=4) {
    DFKill(x);
    return 0;
  }

  for(i=0;i<26;i++) {
    if (b[0] == DicomVR[i].id[0] && b[1] == DicomVR[i].id[1]) {

      //      printf("VR match: %c%c\n",b[0],b[1]);
      if (DicomVR[i].longlen) {
        if (fread(b,1,4,f)!=4) {
          DFKill(x);
          return 0;
        }
        x->len = ConvInt32(b);
        //      printf("longlen=%d (0x%.8x) %d %d %d %d\n",x->len, x->len, b[0],b[1],b[2],b[3]);
      } else {
        x->len = ConvInt16(&b[2]);
        //      printf("shortlen=%d (0x%.4x)\n",x->len,x->len);
      }
      if (x->len > 127) {
        snprintf(x->data,256,"%d",(int)(ftell(f)));
        if (fseek(f,x->len,SEEK_CUR)!=0) {
          DFKill(x);
          return 0;
        }
      } 
      if (x->len <= 0) {
	// ignore SQs for now
	memset(x->data, 0, 256);
	x->len = 0;
      } 
      if (x->len > 0 && x->len < 128) {
        memset(b,0,256);
        if (fread(b,1,x->len,f)!=x->len) {
          DFKill(x);
          return 0;
        }
        if (i==16 || i==22) {
          snprintf(x->data,256,"%d",(int)(ConvInt32(b)));
        } else if (i==18 || i==24) {
          snprintf(x->data,256,"%d",(int)(ConvInt16(b)));
        } else {
          memset(x->data,0,256);
          memcpy(x->data,b,x->len);
        }
      }
      x->next = *df;
      *df     = x;
      return 1;
    }
  }

  // implicit VLs
  clen = ConvInt32(&b[0]);

  if (clen > 255) {
    snprintf(x->data,256,"%d",(int)(ftell(f)));
    if (fseek(f, clen, SEEK_CUR)!=0) {
      DFKill(x);
      return 0;
    }
    x->len = clen;
  }

  if (clen <= 0) {
    memset(x->data,0,256);
    x->len = 0;
    // empty field
  } 

  if (clen > 0 && clen < 256) {
    if (fread(b,1,clen,f)!=clen) {
      DFKill(x);
      return 0;
    }
    x->len = clen;
    
    // slice thickness: always DS type
    // modality: always CS
    // seq name / scanning seq: always SH
    // institution: LO (?)
    if ((x->group == 0x18 && x->key == 0x50) ||
        (x->group == 0x08 && x->key == 0x60) ||
        (x->group == 0x08 && x->key == 0x80) ||
        (x->group == 0x18 && x->key == 0x24) ||
        (x->group == 0x18 && x->key == 0x20) ||
        (x->group == 0x20 && x->key == 0x1041))
    {
      memset(x->data,0,256);
      memcpy(x->data,b,clen);
    } else {

      if ((clen!=2 && clen!=4) || DicomLooksAlphanumeric(b,clen)) {
        memset(x->data,0,64);
        memcpy(x->data,b,clen);
      } else {
	if (clen==2) snprintf(x->data,256,"%d",ConvInt16(b));
	if (clen==4) snprintf(x->data,256,"%d",ConvInt32(b));
      }
    }
  }
  
  x->next = *df;
  *df     = x;

  return 1;
}

DicomField * DicomGetField(DicomField *H, int group, int key) {
  for(;H;H=H->next)
    if (H->group == group && H->key == key)
      return H;
  return 0;
}

int ConvInt32(unsigned char *x) {
  int a,b,c,d;
  if (endianflip) {
    a = (int) (x[3]);
    b = (int) (x[2]);
    c = (int) (x[1]);
    d = (int) (x[0]);
  } else {
    a = (int) (x[0]);
    b = (int) (x[1]);
    c = (int) (x[2]);
    d = (int) (x[3]);
  }
  a = a + 256*b + 65536*c + 16777216*d;
  return a;
}

int ConvInt16(unsigned char *x) {
  int a,b;
  if (endianflip) {
    a = (int) (x[1]);
    b = (int) (x[0]);
  } else {
    a = (int) (x[0]);
    b = (int) (x[1]);
  }
  a = a + 256*b;
  return a;
}

DicomField * DFNew() {
  DicomField *df;
  df = (DicomField *) MemAlloc(sizeof(DicomField));
  df->group = df->key = 0;
  df->data[0] = 0;
  df->len = 0;
  df->next = 0;
  return df;
}

void DFKill(DicomField *df) {
  int i,n;
  DicomField *p,**v;

  n=1;
  for(p=df->next;p!=NULL;p=p->next)
    n++;

  v = MemAlloc(sizeof(DicomField *) * n);

  i=0; p=df;
  while(i<n && p!=NULL) {
    v[i] = p;
    p=p->next;
    i++;
  }

  n=i;
  for(i=0;i<n;i++)
    MemFree(v[i]);
  MemFree(v);
}

int DicomLooksAlphanumeric(unsigned char *s, int len) {
  int i;
  for(i=0;i<len;i++)
    if (s[i] < 32 || s[i] > 126)
      return 0;
  return 1;
}
