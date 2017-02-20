#include "brain_imaging.h"

void usage();

vector<int> te;
Volume<int> *vin[128], *t2map, *m0map;

// t2comp te,te,te... output input-files
int main(int argc, char **argv) {

  int i,j,n;
  char tmp[128],out[512],*p;

  if (argc<4) { usage(); return 1; }
  
  strcpy(tmp,argv[1]);
  p = strtok(tmp,",\n\r\t ");
  while(p!=NULL) {
    te.push_back(atoi(p));
    if (te[te.size()-1] <= 0) {
      cerr << "** invalid echo time.\n";
      return 2;
    }
    p = strtok(NULL,",\n\r\t ");
  }

  n = te.size();
  if (n==0) { cerr << "** no echo times provided.\n"; return 2; }
  
  if (argc != 3+n) {
    usage();
    return 2;
  }

  cout << "using echo times:";
  for(i=0;i<n;i++) cout << " " << te[i];
  cout << endl;

  strcpy(out,argv[2]);
  cout << "output prefix: " << out << endl;

  for(i=0;i<n;i++) {
    cout << "reading " << argv[3+i] << endl;
    vin[i] = new Volume<int>(argv[3+i]);
    if (!vin[i]->ok()) {
      cerr << "** unable to read " << argv[3+i] << endl;
      return 2;
    }
  }

  t2map = new Volume<int>(vin[0]);
  m0map = new Volume<int>(vin[0]);
  t2map->fill(0);
  t2map->fill(0);

  Volume<double> *lnsi[128];
  Volume<int> *chi2;
  double sx=0.0,sx2=0.0,den;
  int N,W,H,D;
  double sy,M,r,num,oe;

  W = t2map->W;
  H = t2map->H;
  D = t2map->D;
  N = t2map->N;

  for(i=0;i<n;i++) {
    sx  += te[i];
    sx2 += te[i]*te[i];

    lnsi[i] = new Volume<double>(W,H,D);
    for(j=0;j<N;j++)
      if (vin[i]->voxel(j) > 0)
	lnsi[i]->voxel(j) = log(vin[i]->voxel(j));
      else
	vin[0]->voxel(j) = -2;
  }
  den = sx2 - sx*sx/n;
  chi2 = new Volume<int>(W,H,D);

  /*
  // block increasing voxels
  for(i=0;i<N;i++)
    if (vin[0]->voxel(i) > 0)
      for(j=0;j<n-1;j++)
	if (vin[j]->voxel(i) < vin[j+1]->voxel(i)) {
	  vin[0]->voxel(i) = -1;
	  break;
	}
  */

  for(i=0;i<N;i++) {

    if (vin[0]->voxel(i) == -1) {
      t2map->voxel(i) = 2047;
      m0map->voxel(i) = 2047;
      continue;
    }

    if (vin[0]->voxel(i) == -2) {
      t2map->voxel(i) = 0;
      m0map->voxel(i) = 0;
      continue;
    }

    chi2->voxel(i) = 0.0;
    sy = M = 0.0;
    for(j=0;j<n;j++) {
      sy += lnsi[j]->voxel(i);
      M  += lnsi[j]->voxel(i) * te[j];
    }
    num = M - sy*sx/n;

    if (den==0.0) {
      t2map->voxel(i) = 0;
      m0map->voxel(i) = 0;
    } else {
      M = num/den;
      t2map->voxel(i) = -1.0/M;
      r = (sy - sx*M) / n;
      m0map->voxel(i) = exp(r);
      for(j=0;j<n;j++) {
	oe = lnsi[j]->voxel(i) - M*te[j] - r;
	chi2->voxel(i) += oe*oe / (M*te[j] + r);
      }
    }
  }
    
  for(i=0;i<n;i++) {
    delete lnsi[i];
    delete vin[i];
  }

  for(i=0;i<N;i++)
    if (t2map->voxel(i) > 2047 || t2map->voxel(i) < 0)
      t2map->voxel(i) = 2047;

  // write results
  sprintf(tmp,"%s_t2.scn",out);
  t2map->writeSCN(tmp,16,true);
  cout << "wrote " << tmp << endl;

  sprintf(tmp,"%s_m0.scn",out);
  m0map->writeSCN(tmp,16,true);
  cout << "wrote " << tmp << endl;

  /*
  sprintf(tmp,"%s_X2.scn",out);
  chi2->writeSCN(tmp,16,true);
  cout << "wrote " << tmp << endl;
  */

  delete t2map;
  delete m0map;
  delete chi2;

  return 0;
}

void usage() {
  fprintf(stderr,"t2comp - computes T2 relaxation from multi-echo data\nusage: t2comp te,te,te... output input-files\n\n");
}

