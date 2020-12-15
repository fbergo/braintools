#include "brain_imaging.h"

#pragma GCC diagnostic ignored "-Wformat-overflow"

#define VERSION "2017.1"

void usage();
void copy_slice(Volume<int> *src, int zsrc, Volume<int> *dest, int zdest);
void scn_namecat(char *dest, const char *src, const char *cat, const char *prefix);
void scn_namecat_ext(char *dest, const char *src, const char *cat, const char *prefix, const char *ext);
void scn_basename(char *dest, const char *src);

// splitmri -p PATTERN [-t] [-o prefix] [input] 
// PATTERN = n[,n...]
int main(int argc, char **argv) {

  int i,j,k,bz,bi,nd;
  char pattern[128], input[512], prefix[128], oname[512], tmp[512], *p;
  int patarray[128];
  int patn=0;
  int total=0;
  int fix_thickness = 0;
  Volume<int> *vin, *vout[128];

  pattern[0] = 0;
  input[0] = 0;
  strcpy(prefix,"s");

  for(i=1;i<argc;i++) {
    if (!strcmp(argv[i],"-t")) {
      fix_thickness = 1;
      continue;
    }
    if (!strcmp(argv[i],"-p") && i<(argc-1)) {
      strcpy(pattern,argv[i+1]);
      i++;
      continue;
    }
    if (!strcmp(argv[i],"-o") && i<(argc-1)) {
      strcpy(prefix,argv[i+1]);
      i++;
      continue;
    }
    if (argv[i][0] != '-')
      strcpy(input,argv[i]);
    else {
      usage();
      return 1;
    }
  }

  if (strlen(pattern)==0 || strlen(input)==0) {
    usage();
    return 1;
  }

  p = strtok(pattern,",\r\n\t ");
  if (p==NULL) { fprintf(stderr,"** Invalid Pattern!\n"); return 2; }
  while(p!=NULL) {
    patarray[patn] = atoi(p);
    total += patarray[patn];
    if (patarray[patn] == 0) { fprintf(stderr,"** Invalid Pattern!\n"); return 2; }
    patn++;
    p = strtok(NULL,",\r\n\t ");    
  }

  vin = new Volume<int>(input);
  if (! vin->ok()) {
    fprintf(stderr,"** Unable to read %s\n",input); return 2;
  }

  if (vin->D % total != 0) {
    fprintf(stderr,"** Volume dimensions do not fit the pattern.\n"); return 2;
  }

  nd = vin->D / total;
  for(i=0;i<total;i++) {
    vout[i] = new Volume<int>(vin->W,vin->H,nd,vin->dx,vin->dy,vin->dz);
    if (fix_thickness) vout[i]->dz *= total;
  }

  for(i=0,bz=0,bi=0;i<patn;i++) {
    for(j=0;j<patarray[i]*nd;j++) {
      k = j % patarray[i];
      //      printf("i=%d bz=%d bi=%d total=%d k=%d copying %d to %d(%d)\n",i,bz,bi,total,k,bz+j,bi+k,j/patarray[i]);
      copy_slice(vin,bz+j,vout[bi+k],j/patarray[i]);
    }
    bi+=patarray[i];
    bz+=patarray[i]*nd;
  }

  delete vin;

  p = strrchr(input,'/');
  if (p==NULL)
    strcpy(tmp,input);
  else
    strcpy(tmp,p+1);

  for(i=0;i<total;i++) {    
    if (total >= 10)
      sprintf(oname,"%s%02d_%s",prefix,i+1,tmp);
    else
      sprintf(oname,"%s%d_%s",prefix,i+1,tmp);

    printf("writing %s...\n",oname);
    vout[i]->writeSCN(oname,16,true);
    delete vout[i];
  }

  printf("done.\n\n");

  return 0;
}

void usage() {
  fprintf(stderr,"splitmri - version %s - (C) 2011-2017 Felipe Bergo <fbergo\x40gmail.com>\n",VERSION);
  fprintf(stderr,"Splits interleaved SCN files.\n\n");
  fprintf(stderr,"Usage: splitmri -p PATTERN [-t] [-o prefix] input\n\n");
  fprintf(stderr,"-o set the output prefix (default \"s\"), PATTERN must be a\n");
  fprintf(stderr,"sequence of comma-separated numbers.\n\n");
}

void copy_slice(Volume<int> *src, int zsrc, Volume<int> *dest, int zdest) {
  int i,j;
  if (src->W != dest->W || src->H != dest->H) return;
  if (zsrc >= src->D || zdest >= dest->D) return;
  for(j=0;j<src->H;j++)
    for(i=0;i<src->W;i++)
      dest->voxel(i,j,zdest) = src->voxel(i,j,zsrc);
}

void scn_namecat(char *dest, const char *src, const char *cat, const char *prefix) {
  scn_namecat_ext(dest,src,cat,prefix,"scn");
}

void scn_namecat_ext(char *dest, const char *src, const char *cat, const char *prefix, const char *ext) {
  char tmp[1024], *p;
  scn_basename(tmp, src);
  sprintf(dest,"%s_%s.%s",tmp,cat,ext);
  p = strrchr(dest,'/');
  if (p==NULL) {
    sprintf(tmp, "%s_%s", prefix, dest);
    strcpy(dest, tmp);    
  } else {
    sprintf(tmp,"%s_%s",prefix,p+1);
    strcpy(p+1,tmp);    
  }
}

void scn_basename(char *dest, const char *src) {
  if (!strcasecmp(&src[strlen(src)-4],".scn")) {
    strcpy(dest,src);
    dest[strlen(dest)-4] = 0;
  } else {
    strcpy(dest, src);
  }
}
