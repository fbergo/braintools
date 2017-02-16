#define NO_STATIC 1
#include "brain_imaging.h"
#include "greg_msgd.h"
#include "greg.h"

void msgd(double **delta, double *theta, Volume<int> *B, vector<int> & S, R3 &cg, Volume<int> *mov, double *best) {
  double db[6];
  double tb[6];
  double vfb, vf, vf0, vf1, vf2;
  int i,j;
  int it = 1;
  char msg[256];
  Timestamp tic, toc;

  tic = Timestamp::now();

  vfb = cfrg(B,S,cg,mov,theta, NULL, -1, -1);

  for(i=0;i<6;i++) tb[i] = theta[i];

  do {
    vf0 = vfb;
    sprintf(msg,"MSGD optimization (iteration #%d, score %.2f)",it,vfb);
    lok.lock();
    strcpy(bgtask.minor, msg);
    lok.unlock();

    for(i=0;i<6;i++) theta[i] = tb[i];
    for(j=0;j<5;j++) {
      for(i=0;i<6;i++) {
	vf = vf0;

	db[i] = 0.0;
	
	vf1 = cfrg(B,S,cg,mov,theta,delta[j],i,1);
	vf2 = cfrg(B,S,cg,mov,theta,delta[j],i,-1);

	if (vf1>vf) { vf=vf1; db[i] = delta[j][i]; }
	if (vf2>vf) { db[i] = -delta[j][i]; }
      }

      vf = cfrg(B,S,cg,mov,theta,db,-1,-1);
      if (vf>vfb) {
	vfb = vf;
	for(i=0;i<6;i++)
	  tb[i] = theta[i] + db[i];
      }
    }

    if (logging) {
      toc = Timestamp::now();
      printf("msgd-ws %d %.3f %.2f\n",it,(toc-tic),vfb);
    }

    it++;
  } while(vfb>vf0);

  rbest.score = vfb;
  rbest.iterations = it-1;

  for(i=0;i<6;i++)
    best[i] = tb[i];
}

Volume<int> *msgd_register(Volume<int> *a, Volume<int> *b) {

  double theta[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  double bt[6];
  int i,j,k;
  T4 T;
  R3 cg;
  Timestamp tic, tac;

  Volume<int> *anorm, *bnorm, *B, *wsl, *out, *out2;
  Volume<char> *wsb;
  vector<int> S;
  
  double *delta[5];
  delta[0] = new double[6];
  delta[0][0] = delta[0][1] = delta[0][2] = 15.0;
  delta[0][3] = delta[0][4] = delta[0][5] = 15.0;
  delta[1] = new double[6];
  delta[1][0] = delta[1][1] = delta[1][2] = 10.0;
  delta[1][3] = delta[1][4] = delta[1][5] = 10.0;
  delta[2] = new double[6];
  delta[2][0] = delta[2][1] = delta[2][2] = 1.0;
  delta[2][3] = delta[2][4] = delta[2][5] = 3.0;
  delta[3] = new double[6];
  delta[3][0] = delta[3][1] = delta[3][2] = 0.5;
  delta[3][3] = delta[3][4] = delta[3][5] = 1.0;
  delta[4] = new double[6];
  delta[4][0] = delta[4][1] = delta[4][2] = 0.2;
  delta[4][3] = delta[4][4] = delta[4][5] = 0.2;
    
  lok.lock();
  bgtask.nsteps = 8;
  bgtask.step = 1;
  strcpy(bgtask.minor, "Histogram normalization (A)");
  lok.unlock();

  anorm = new Volume<int>(a);
  acc_hist_norm(anorm);

  lok.lock();
  bgtask.step = 2;
  strcpy(bgtask.minor, "Morphological Gradient (A)");
  lok.unlock();

  B = anorm->morphGradient3D(1.0);
  delete anorm;

  lok.lock();
  bgtask.step = 3;
  strcpy(bgtask.minor, "Histogram normalization (B)");
  lok.unlock();

  bnorm = new Volume<int>(b);
  acc_hist_norm(bnorm);

  lok.lock();
  bgtask.step = 4;
  strcpy(bgtask.minor, "Watershed transform (B)");
  lok.unlock();

  wsl = watershed(bnorm, 1.8, 0.07);
  delete bnorm;

  lok.lock();
  bgtask.step = 5;
  strcpy(bgtask.minor, "Border computation (B)");
  lok.unlock();

  wsb = borders(wsl);
  delete wsl;
  for(i=0;i<wsb->N;i++)
    if (wsb->voxel(i))
      S.push_back(i);

  lok.lock();
  bgtask.step = 6;
  strcpy(bgtask.minor, "Gravity center computation (B)");
  lok.unlock();

  calcCG(wsb, cg);
  delete wsb;  

  lok.lock();
  bgtask.step = 7;
  strcpy(bgtask.minor, "MSGD optimization");
  lok.unlock();

  msgd(delta,theta,B,S,cg,b, bt);

  // transform b
  printf("best = R(%.2f,%.2f,%.2f), T(%.2f,%.2f,%.2f)\n",
	 bt[0],bt[1],bt[2],bt[3],bt[4],bt[5]);
  mktrans(T, bt, cg);

  delete B;
  for(i=0;i<5;i++) delete delta[i];

  lok.lock();
  bgtask.step = 8;
  strcpy(bgtask.minor, "Interpolating result");
  lok.unlock();

  out = new Volume<int>( Max(a->W,b->W), Max(a->H,b->H), Max(a->D,b->D), a->dx, a->dy, a->dz);

  for(k=0;k<out->D;k++)
    for(j=0;j<out->H;j++)
      for(i=0;i<out->W;i++)
	if (b->valid(i,j,k))
	  out->voxel(i,j,k)  = b->voxel(i,j,k);

  out->transformLI(T);

  out2 = new Volume<int>(a);
  out2->fill(0);
  for(k=0;k<out2->D;k++)
    for(j=0;j<out2->H;j++)
      for(i=0;i<out2->W;i++)
	if (out->valid(i,j,k))
	  out2->voxel(i,j,k)  = out->voxel(i,j,k);

  delete out;

  tac = Timestamp::now();
  cout << "MSGD-WS-FF completed in " << (tac-tic) << " seconds.\n";

  strcpy(rbest.algo,"msgd-ws-ff");
  rbest.elapsed = tac-tic;
  rbest.t = T;

  return out2;
}

void mktrans(T4 &T, double *par, R3 &cg) {
  T4 rx,ry,rz,trans,x,y;

  trans.translate(par[3],par[4],par[5]);
  rx.xrot(par[0], cg.X, cg.Y, cg.Z);
  ry.yrot(par[1], cg.X, cg.Y, cg.Z);
  rz.zrot(par[2], cg.X, cg.Y, cg.Z);

  x = trans * rx;
  y = x * ry;
  x = y * rz;
  T = x;
}

double cfrg(Volume<int> *B, vector<int> &S, R3 &cg, Volume<int> *mov, double *theta, double *delta, int i, int dir) {
  double taux[6], D=0.0;
  int k;
  T4 T;
  R3 p,q;

  unsigned int s;
  int x1,y1,z1;

  if (delta==NULL) {
    mktrans(T, theta, cg);
  } else {
    if (i==-1) { // all pars
      for(k=0;k<6;k++) taux[k] = theta[k] + delta[k];
    } else {
      for(k=0;k<6;k++) taux[k] = theta[k];
      taux[i] = theta[i] + dir*delta[i];
    }
    mktrans(T,taux,cg);
  }

  for(s=0;s<S.size();s++) {
    p.set(mov->xOf(S[s]), mov->yOf(S[s]), mov->zOf(S[s]));
    q = T.apply(p);
    x1 = (int) q.X; y1 = (int) q.Y; z1 = (int) q.Z;
    if (B->valid(x1,y1,z1))
      D += B->voxel(x1,y1,z1);
  }

  return D;
}

void acc_hist_norm(Volume<int> *v) {
  int i, *ah, lo, hi;

  ah = new int[65536];
  for(i=0;i<65536;i++) ah[i] = 0;

  for(i=0;i<v->N;i++)
    if (v->voxel(i) >= 0 && v->voxel(i) < 65536)
      ah[v->voxel(i)]++;

  for(i=1;i<65536;i++)
    ah[i] += ah[i-1];

  lo = 0;
  while(lo<65536 && ah[lo] < (v->N * 0.10)) lo++;
  hi = 65535;
  while(hi>0 && ah[hi] > (v->N * 0.991)) hi--;

  //printf("clamp lo=%d hi=%d\n",lo,hi);

  for(i=0;i<v->N;i++)
    if (v->voxel(i) < lo)
      v->voxel(i) = 0;
    else if (v->voxel(i) > hi)
      v->voxel(i) = 4095;
    else
      v->voxel(i) = (int) (((v->voxel(i) - lo)*4095.0) / (hi - lo));

  delete ah;
}

Volume<int> *watershed(Volume<int> *v, double radius, double hfac) {
  int i,j,h,r=1,p,c1,c2,aq;
  SphericalAdjacency A(radius);
  BasicPQ *Q;
  Location lp,lq;

  Volume<int> *cost;
  Volume<int> *label;
  Volume<int> *grad;

  grad = v->morphGradient3D(1.0);

  h = (int) (grad->maximum() * hfac);

  cost  = new Volume<int>(v->W,v->H,v->D);
  label = new Volume<int>(v->W,v->H,v->D);

  Q = new BasicPQ(v->N, grad->maximum() + h + 2);

  for(i=0;i<v->N;i++) {
    cost->voxel(i) = grad->voxel(i) + h + 1;
    label->voxel(i) = 0;
    Q->insert(i,cost->voxel(i));
  }

  while(!Q->empty()) {
    p = Q->remove();

    if (label->voxel(p) == 0) {
      cost->voxel(p) = grad->voxel(p);
      label->voxel(p) = r;
      //printf("label %d to %d\n",r,p);
      r++;
    }

    lp.set( v->xOf(p), v->yOf(p), v->zOf(p) );
    for(j=0;j<A.size();j++) {
      lq = A.neighbor(lp,j);
      if (v->valid(lq)) {
	c1 = cost->voxel(lq);
	c2 = Max( cost->voxel(lp), grad->voxel(lq) );
	if (c2 < c1) {
	  aq = v->address(lq);
	  Q->update(aq,c1,c2);
	  cost->voxel(lq) = c2;
	  label->voxel(lq) = label->voxel(lp);
	}
      }
    }
  }

  delete Q;
  delete cost;
  delete grad;

  //label->writeSCN("label.scn",16,false);
  return label;
}

Volume<char> *borders(Volume<int> *v) {
  SphericalAdjacency A(1.0);
  Volume<char> *b;
  int i,j;
  Location lp, lq;

  b = new Volume<char>(v->W,v->H,v->D);

  for(i=0;i<v->N;i++) {
    lp.set( v->xOf(i), v->yOf(i), v->zOf(i) );
    for(j=0;j<A.size();j++) {
      lq = A.neighbor(lp,j);
      if (v->valid(lq))
	if (v->voxel(lp) < v->voxel(lq)) {
	  b->voxel(lp) = 1;
	  break;
	}
    }
  }

  return b;
}

void calcCG(Volume<char> *v, R3 &cg) {
  int x=0,y=0,z=0,i,j,k,n=0;

  for(k=0;k<v->D;k++)
    for(j=0;j<v->H;j++)
      for(i=0;i<v->W;i++)
	if (v->voxel(i,j,k)) {
	  x += i; y += j; z += k; n++;
	}

  if (n!=0) {
    cg.X = x/n;
    cg.Y = y/n;
    cg.Z = z/n;
  }
}

// MI

Volume<int>  *msgd_miregister(Volume<int> *a, Volume<int> *b) {
  double theta[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  double bt[6];
  int i,j,k;
  T4 T;
  R3 cg;
  Timestamp tic, tac;

  Volume<int> *anorm, *bnorm, *out, *out2;
  
  double *delta[5];
  delta[0] = new double[6];
  delta[0][0] = delta[0][1] = delta[0][2] = 15.0;
  delta[0][3] = delta[0][4] = delta[0][5] = 15.0;
  delta[1] = new double[6];
  delta[1][0] = delta[1][1] = delta[1][2] = 10.0;
  delta[1][3] = delta[1][4] = delta[1][5] = 10.0;
  delta[2] = new double[6];
  delta[2][0] = delta[2][1] = delta[2][2] = 1.0;
  delta[2][3] = delta[2][4] = delta[2][5] = 3.0;
  delta[3] = new double[6];
  delta[3][0] = delta[3][1] = delta[3][2] = 0.5;
  delta[3][3] = delta[3][4] = delta[3][5] = 1.0;
  delta[4] = new double[6];
  delta[4][0] = delta[4][1] = delta[4][2] = 0.2;
  delta[4][3] = delta[4][4] = delta[4][5] = 0.2;
    
  lok.lock();
  bgtask.nsteps = 4;
  bgtask.step = 1;
  strcpy(bgtask.minor, "Histogram normalization (A)");
  lok.unlock();

  anorm = new Volume<int>(a);
  acc_hist_norm(anorm);

  lok.lock();
  bgtask.step = 2;
  strcpy(bgtask.minor, "Histogram normalization (B)");
  lok.unlock();

  bnorm = new Volume<int>(b);
  acc_hist_norm(bnorm);

  cg.set(bnorm->W/2.0,bnorm->H/2.0,bnorm->D/2.0);

  lok.lock();
  bgtask.step = 3;
  strcpy(bgtask.minor, "MSGD optimization");
  lok.unlock();

  mi_msgd(delta,theta,anorm,bnorm,cg,bt);

  // transform b
  printf("best = R(%.2f,%.2f,%.2f), T(%.2f,%.2f,%.2f)\n",
	 bt[0],bt[1],bt[2],bt[3],bt[4],bt[5]);
  mktrans(T, bt, cg);

  for(i=0;i<5;i++) delete delta[i];
  delete anorm;
  delete bnorm;

  lok.lock();
  bgtask.step = 4;
  strcpy(bgtask.minor, "Interpolating result");
  lok.unlock();

  out = new Volume<int>( Max(a->W,b->W), Max(a->H,b->H), Max(a->D,b->D), a->dx, a->dy, a->dz);

  for(k=0;k<out->D;k++)
    for(j=0;j<out->H;j++)
      for(i=0;i<out->W;i++)
	if (b->valid(i,j,k))
	  out->voxel(i,j,k)  = b->voxel(i,j,k);

  out->transformLI(T);

  out2 = new Volume<int>(a);
  out2->fill(0);
  for(k=0;k<out2->D;k++)
    for(j=0;j<out2->H;j++)
      for(i=0;i<out2->W;i++)
	if (out->valid(i,j,k))
	  out2->voxel(i,j,k)  = out->voxel(i,j,k);

  delete out;

  tac = Timestamp::now();
  cout << "MSGD-MI-FF completed in " << (tac-tic) << " seconds.\n";

  strcpy(rbest.algo,"msgd-mi-ff");
  rbest.elapsed = tac-tic;
  rbest.t = T;

  return out2;
}

void mi_msgd(double **delta, double *theta, Volume<int> *a, Volume<int> *b, R3 &cg, double *best) {
  double db[6];
  double tb[6];
  double vfb, vf, vf0, vf1, vf2;
  int i,j;
  int it = 1;
  char msg[256];
  Timestamp tic, toc;

  tic = Timestamp::now();

  vfb = mi_cfrg(a,b,cg,theta, NULL, -1, -1);

  for(i=0;i<6;i++) tb[i] = theta[i];

  do {
    vf0 = vfb;
    sprintf(msg,"MSGD optimization (iteration #%d, score %.2f)",it,vfb);
    lok.lock();
    strcpy(bgtask.minor, msg);
    lok.unlock();

    for(i=0;i<6;i++) theta[i] = tb[i];
    for(j=0;j<5;j++) {
      for(i=0;i<6;i++) {
	vf = vf0;

	db[i] = 0.0;
	
	vf1 = mi_cfrg(a,b,cg,theta,delta[j],i,1);
	vf2 = mi_cfrg(a,b,cg,theta,delta[j],i,-1);

	if (vf1>vf) { vf=vf1; db[i] = delta[j][i]; }
	if (vf2>vf) { db[i] = -delta[j][i]; }
      }

      vf = mi_cfrg(a,b,cg,theta,db,-1,-1);
      if (vf>vfb) {
	vfb = vf;
	for(i=0;i<6;i++)
	  tb[i] = theta[i] + db[i];
      }
    }
    if (logging) {
      toc = Timestamp::now();
      printf("msgd-mi %d %.3f %.2f\n",it,(toc-tic),vfb);
    }
    it++;
  } while(vfb>vf0);

  rbest.score = vfb;
  rbest.iterations = it-1;

  for(i=0;i<6;i++)
    best[i] = tb[i];

}

double mi_cfrg(Volume<int> *a, Volume<int> *b, R3 &cg, double *theta, double *delta, int ii, int dir) {
  double taux[6], D=0.0;
  int k;
  T4 T;
  R3 p,q;

  int x1,y1,z1,x,y,z;

  int sba = 0, sbb = 0, va, vb;
  int i, j, *jh;
  const int misize = 64;
  int n = misize * misize;
  int ha[256], hb[256];
  double pj, pa, pb, count;

  va = a->maximum();
  vb = b->maximum();
  while(va > misize - 1) { va >>= 1; sba++; }
  while(vb > misize - 1) { vb >>= 1; sbb++; }
  
  jh = new int[n];
  for(i=0;i<n;i++) jh[i] = 0;

  if (delta==NULL) {
    mktrans(T, theta, cg);
  } else {
    if (ii==-1) { // all pars
      for(k=0;k<6;k++) taux[k] = theta[k] + delta[k];
    } else {
      for(k=0;k<6;k++) taux[k] = theta[k];
      taux[ii] = theta[ii] + dir*delta[ii];
    }
    mktrans(T,taux,cg);
  }

  for(z=0;z<b->D;z+=2)
    for(y=0;y<b->H;y+=2)
      for(x=0;x<b->W;x+=2) {
	p.set(x,y,z);
	q = T.apply(p);
	x1 = (int) q.X; y1 = (int) q.Y; z1 = (int) q.Z;
	if (a->valid(x1,y1,z1)) {
	  va = a->voxel(x1,y1,z1) >> sba;
	  vb = b->voxel(x,y,z) >> sbb;
	  jh[va + misize*vb]++;
	}
      }

  // compute disjoint histograms
  for(i=0;i<misize;i++) {
    ha[i] = hb[i] = 0;
    for(j=0;j<misize;j++) {
      ha[i] += jh[i + misize*j];
      hb[i] += jh[j + misize*i];
    }
  }

  count = 0.0;
  for(i=0;i<n;i++)
    count += jh[i];

  if (count == 0.0) count = 1.0;

  // Thevenaz 2000, eq 7
  for(i=0;i<misize;i++)
    for(j=0;j<misize;j++) {
      
      pj = jh[i + misize*j] / count;
      pa = ha[i] / count;
      pb = hb[j] / count;

      if (pj == 0.0 || pa == 0.0 || pb == 0.0) continue;
      D += pj * log( pj / (pa*pb) );
    }

  D *= 10000.0;

  delete jh;
  return D;
}
