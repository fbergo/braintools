#define NO_STATIC 1
#include "brain_imaging.h"
#include "greg_msgd.h"
#include "greg_chs.h"
#include "greg.h"

// build vectors (rotations and translations) with ply 0 transforms
void chs_ply0(vector<T4> &rvec, vector<T4> &tvec, Volume<int> *a) {
  double angles[6] = { 16.0,  9.0, 5.0, 3.0, 1.0, 0.5 };
  double disps[5]  = { 16.0,  9.0, 5.0, 3.0, 1.0 };
  int i;
  double cx,cy,cz;
  T4 t;
  
  cx = a->W / 2.0;
  cy = a->H / 2.0;
  cz = a->D / 2.0;

  rvec.clear();
  tvec.clear();
  
  for(i=0;i<6;i++) {
    t.xrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.xrot(-angles[i],cx,cy,cz); rvec.push_back(t);
    t.yrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.yrot(-angles[i],cx,cy,cz); rvec.push_back(t);
    t.zrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.zrot(-angles[i],cx,cy,cz); rvec.push_back(t);
  }
  
  for(i=0;i<5;i++) {
    t.translate( disps[i], 0.0, 0.0); tvec.push_back(t);
    t.translate(-disps[i], 0.0, 0.0); tvec.push_back(t);
    t.translate(0.0,  disps[i], 0.0); tvec.push_back(t);
    t.translate(0.0, -disps[i], 0.0); tvec.push_back(t);
    t.translate(0.0, 0.0,  disps[i]); tvec.push_back(t);
    t.translate(0.0, 0.0, -disps[i]); tvec.push_back(t);
  }

}

// build vectors (rotations and translations) with ply 0x transforms
void chs_ply0x(vector<T4> &rvec, vector<T4> &tvec, Volume<int> *a) {
  double angles[12] = { 16.0, 12.0, 9.0, 7.0, 5.0, 4.0, 3.0, 2.0, 1.5, 1.0, 0.7, 0.5 };
  double disps[10]  = { 16.0, 12.0, 9.0, 7.0, 5.0, 4.0, 3.0, 2.0, 1.5, 1.0 };
  int i;
  double cx,cy,cz;
  T4 t;
  
  cx = a->W / 2.0;
  cy = a->H / 2.0;
  cz = a->D / 2.0;

  rvec.clear();
  tvec.clear();
  
  for(i=0;i<12;i++) {
    t.xrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.xrot(-angles[i],cx,cy,cz); rvec.push_back(t);
    t.yrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.yrot(-angles[i],cx,cy,cz); rvec.push_back(t);
    t.zrot(angles[i],cx,cy,cz);  rvec.push_back(t);
    t.zrot(-angles[i],cx,cy,cz); rvec.push_back(t);
  }
  
  for(i=0;i<10;i++) {
    t.translate( disps[i], 0.0, 0.0); tvec.push_back(t);
    t.translate(-disps[i], 0.0, 0.0); tvec.push_back(t);
    t.translate(0.0,  disps[i], 0.0); tvec.push_back(t);
    t.translate(0.0, -disps[i], 0.0); tvec.push_back(t);
    t.translate(0.0, 0.0,  disps[i]); tvec.push_back(t);
    t.translate(0.0, 0.0, -disps[i]); tvec.push_back(t);
  }

}

Volume<int> * chs_register1(Volume<int> *a, Volume<int> *b) {
  vector<T4> tvec, rvec, dvec;
  chs_ply0(rvec, tvec, a);
  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());
  strcpy(rbest.algo,"chs-ws-L1");
  return(chs_register(a,b,dvec));
}

Volume<int> * chs_register1x(Volume<int> *a, Volume<int> *b) {
  vector<T4> tvec, rvec, dvec;
  chs_ply0x(rvec, tvec, a);
  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());
  strcpy(rbest.algo,"chs-ws-L1X");
  return(chs_register(a,b,dvec));
}


Volume<int> * chs_register2(Volume<int> *a, Volume<int> *b) {
  unsigned int i,j,nr, nt;
  vector<T4> tvec, rvec, dvec;
  T4 t;
  chs_ply0(rvec, tvec, a);
  nt = tvec.size();
  nr = rvec.size();

  // build ply 1

  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());

  // TxR
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = tvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // RxT
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = rvec[j];
      t *= tvec[i];
      dvec.push_back(t);
    }
  // RxR avoiding identities and redundant permutations
  for(i=0;i<nr;i++)
    for(j=0;j<nr;j++) {
      // exclude null combinations (e.g. xrot(-5) + xrot(+5))
      if ( (i%2 == 0 && j-i == 1) || (j%2 == 0 && i-j == 1) ) continue;
      // exclude redundant permutations of rotations about the same axis
      if ( ( (i%6)/2 == (j%6)/2 ) && (j<i)) continue; 

      t = rvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // TxT avoiding identities and redundant permutations 
  for(i=0;i<nt;i++)
    for(j=i;j<nt;j++) {
      if ( (i%2 == 0 && j-i == 1) ) continue;
      t = tvec[i];
      t *= tvec[j];
      dvec.push_back(t);
    }

  strcpy(rbest.algo,"chs-ws-L2");
  return(chs_register(a,b,dvec));
}

Volume<int> * chs_register2c(Volume<int> *a, Volume<int> *b) {
  unsigned int i,j,nr, nt;
  vector<T4> tvec, rvec, dvec;
  T4 t;
  chs_ply0(rvec, tvec, a);
  nt = tvec.size();
  nr = rvec.size();

  // build ply 1

  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());

  // TxR
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = tvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // RxT
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = rvec[j];
      t *= tvec[i];
      dvec.push_back(t);
    }

  strcpy(rbest.algo,"chs-ws-L2C");
  return(chs_register(a,b,dvec));
}

Volume<int> * chs_register(Volume<int> *a, Volume<int> *b,vector<T4> &tvec) {
  Volume<int> *anorm, *bnorm, *B, *wsl, *out, *out2;
  Volume<char> *wsb;
  vector<int> S;
  int i,j,k;
  T4 T;
  Timestamp tic, tac;

  lok.lock();
  busythreads++;
  bgtask.nsteps = 7;
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

  //B->writeSCN("B.scn",16,true);
  //wsb->writeSCN("wsb.scn",8,false);
  delete wsb;

  lok.lock();
  bgtask.step = 6;
  strcpy(bgtask.minor, "CHS optimization");
  busythreads--;
  lok.unlock();

  T.identity();
  chs(B,S,T,tvec,b);

  delete B;

  lok.lock();
  busythreads++;
  bgtask.step = 7;
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

  lok.lock();
  busythreads--;
  lok.unlock();

  tac = Timestamp::now();
  cout << "CHS completed in " << (tac-tic) << " seconds.\n";

  rbest.elapsed = tac-tic;
  rbest.t = T;

  return out2;
}

Volume<int> * mat_register(Volume<int> *a, Volume<int> *b,T4 &mat) {
  Volume<int> *out, *out2;
  int i,j,k;
  Timestamp tic, tac;

  strcpy(rbest.algo,"loaded-matrix");

  lok.lock();
  busythreads++;
  bgtask.nsteps = 1;
  bgtask.step = 1;
  strcpy(bgtask.minor, "Registration");
  lok.unlock();
  
  out = new Volume<int>( Max(a->W,b->W), Max(a->H,b->H), Max(a->D,b->D), a->dx, a->dy, a->dz);
  
  for(k=0;k<out->D;k++)
    for(j=0;j<out->H;j++)
      for(i=0;i<out->W;i++)
	if (b->valid(i,j,k))
	  out->voxel(i,j,k)  = b->voxel(i,j,k);

  out->transformLI(mat);
  
  out2 = new Volume<int>(a);
  out2->fill(0);
  for(k=0;k<out2->D;k++)
    for(j=0;j<out2->H;j++)
      for(i=0;i<out2->W;i++)
	if (out->valid(i,j,k))
	  out2->voxel(i,j,k)  = out->voxel(i,j,k);

  delete out;

  lok.lock();
  busythreads--;
  lok.unlock();

  tac = Timestamp::now();
  cout << "Registration completed in " << (tac-tic) << " seconds.\n";

  rbest.elapsed = tac-tic;
  rbest.t = mat;

  return out2;
}

class ws_job {
public:
  // input
  vector<T4> candidates;
  Volume<int> *B, *mov;
  vector<int> *S;
  // output
  double best_score;
  T4 best_candidate;
  // aux
  pthread_t tid;
};

void *chs_thread(void *arg) {
  ws_job *job = (ws_job *) arg;
  unsigned int i;
  double score;

  lok.lock();
  busythreads++;
  lok.unlock();

  for(i=0;i<job->candidates.size();i++) {
    score = chs_eval(job->B,*(job->S),job->mov,job->candidates[i]);
    if (score > job->best_score) {
      job->best_score = score;
      job->best_candidate = job->candidates[i];
    }
  }

  lok.lock();
  busythreads--;
  lok.unlock();

  return NULL;
}

void chs(Volume<int> *B, vector<int> &S, T4 &T, vector<T4> &tvec, Volume<int> *mov) {
  double bscore, pbscore;
  int it = 0;
  T4 cur, tmp, best;
  unsigned int k;
  int i;
  char msg[256];
  Timestamp tic, toc;
  ws_job jobs[32];

  tic = Timestamp::now();

  cur = T;
  best = cur;
  bscore = chs_eval(B,S,mov,cur);

  rbest.nhist = 1;
  rbest.scorehist[0] = bscore;

  do {
    it++;
    pbscore = bscore; // previous best score

    /* original code, before parallelization:
    for(k=0;k<tvec.size();k++) {
      tmp = cur;
      tmp *= tvec[k];
      score = chs_eval(B,S,mov,tmp);
      if (score > bscore) {
	bscore = score;
	best = tmp;
      }
    }
    */

    for(i=0;i<ncpus;i++)
      jobs[i].candidates.clear();

    // parallel version    
    for(k=0;k<tvec.size();k++) {
      tmp = cur;
      tmp *= tvec[k];
      jobs[ k % ncpus ].candidates.push_back(tmp);
      jobs[ k % ncpus ].B   = B;
      jobs[ k % ncpus ].S   = &S;
      jobs[ k % ncpus ].mov = mov;
      jobs[ k % ncpus ].best_score     = bscore;
      jobs[ k % ncpus ].best_candidate = cur;
    }

    for(i=0;i<ncpus;i++)
      pthread_create(&(jobs[i].tid), NULL, chs_thread, &(jobs[i]));

    for(i=0;i<ncpus;i++)
      pthread_join(jobs[i].tid, NULL);

    for(i=0;i<ncpus;i++)
      if (!jobs[i].candidates.empty())
	if (jobs[i].best_score > bscore) {
	  bscore = jobs[i].best_score;
	  best   = jobs[i].best_candidate;
	}

    /* end parallel part */

    if (rbest.nhist < MAXSCOREHIST) {
      rbest.scorehist[rbest.nhist] = bscore;
      rbest.nhist++;
    }

    cur = best;

    if (logging) {
      toc = Timestamp::now();
      printf("chs-ws %d %.3f %.2f\n",it,(toc-tic),bscore);
    }

    sprintf(msg,"CHS optimization (iteration #%d, score %.2f)",it,bscore);
    lok.lock();
    strcpy(bgtask.minor, msg);
    lok.unlock();


  } while(bscore > pbscore);

  rbest.score = bscore;
  rbest.iterations = it;

  T = best;
}

// the score must be comparable to cfrg(...) in greg_msgd.cc
double chs_eval(Volume<int> *B, vector<int> &S, Volume<int> *mov, T4 &T) {
  unsigned int s;
  double D=0.0;
  int x1,y1,z1;
  R3 p,q;

  for(s=0;s<S.size();s++) {
    p.set(mov->xOf(S[s]), mov->yOf(S[s]), mov->zOf(S[s]));
    q = T.apply(p);
    x1 = (int) q.X; y1 = (int) q.Y; z1 = (int) q.Z;
    if (B->valid(x1,y1,z1))
      D += B->voxel(x1,y1,z1);
  }

  return D;
}

// MI

Volume<int> * chs_miregister1(Volume<int> *a, Volume<int> *b) {
  vector<T4> tvec, rvec, dvec;
  chs_ply0(rvec, tvec, a);
  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());
  strcpy(rbest.algo,"chs-mi-L1");
  return(chs_miregister(a,b,dvec));
}

Volume<int> * chs_miregister1x(Volume<int> *a, Volume<int> *b) {
  vector<T4> tvec, rvec, dvec;
  chs_ply0x(rvec, tvec, a);
  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());
  strcpy(rbest.algo,"chs-mi-L1X");
  return(chs_miregister(a,b,dvec));
}

Volume<int> * chs_miregister2(Volume<int> *a, Volume<int> *b) {
  unsigned int i,j,nr, nt;
  vector<T4> tvec, rvec, dvec;
  T4 t;
  chs_ply0(rvec, tvec, a);
  nt = tvec.size();
  nr = rvec.size();

  // build ply 1

  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());

  // TxR
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = tvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // RxT
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = rvec[j];
      t *= tvec[i];
      dvec.push_back(t);
    }
  // RxR avoiding identities and redundant permutations
  for(i=0;i<nr;i++)
    for(j=0;j<nr;j++) {
      // exclude null combinations (e.g. xrot(-5) + xrot(+5))
      if ( (i%2 == 0 && j-i == 1) || (j%2 == 0 && i-j == 1) ) continue;
      // exclude redundant permutations of rotations about the same axis
      if ( ( (i%6)/2 == (j%6)/2 ) && (j<i)) continue; 

      t = rvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // TxT avoiding identities and redundant permutations 
  for(i=0;i<nt;i++)
    for(j=i;j<nt;j++) {
      if ( (i%2 == 0 && j-i == 1) ) continue;
      t = tvec[i];
      t *= tvec[j];
      dvec.push_back(t);
    }

  strcpy(rbest.algo,"chs-mi-L2");
  return(chs_miregister(a,b,dvec));
}

Volume<int> * chs_miregister2c(Volume<int> *a, Volume<int> *b) {
  unsigned int i,j,nr, nt;
  vector<T4> tvec, rvec, dvec;
  T4 t;
  chs_ply0(rvec, tvec, a);
  nt = tvec.size();
  nr = rvec.size();

  // build ply 1

  // dvec = rvec . tvec
  dvec.insert(dvec.end(), rvec.begin(), rvec.end());
  dvec.insert(dvec.end(), tvec.begin(), tvec.end());

  // TxR
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = tvec[i];
      t *= rvec[j];
      dvec.push_back(t);
    }
  // RxT
  for(i=0;i<nt;i++)
    for(j=0;j<nr;j++) {
      t = rvec[j];
      t *= tvec[i];
      dvec.push_back(t);
    }

  strcpy(rbest.algo,"chs-mi-L2C");
  return(chs_miregister(a,b,dvec));
}

Volume<int> * chs_miregister(Volume<int> *a, Volume<int> *b,vector<T4> &tvec) {
  Volume<int> *anorm, *bnorm, *out, *out2;
  int i,j,k;
  T4 T;
  Timestamp tic, tac;

  lok.lock();
  busythreads++;
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

  lok.lock();
  bgtask.step = 3;
  strcpy(bgtask.minor, "CHS optimization");
  busythreads--;
  lok.unlock();

  T.identity();
  mi_chs(anorm,bnorm,T,tvec);

  delete anorm;
  delete bnorm;

  lok.lock();
  busythreads++;
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

  lok.lock();
  busythreads--;
  lok.unlock();

  tac = Timestamp::now();
  cout << "CHS completed in " << (tac-tic) << " seconds.\n";

  rbest.elapsed = tac-tic;
  rbest.t = T;

  return out2;
}

class mi_job {
public:
  // input
  vector<T4> candidates;
  Volume<int> *a, *b;
  // output
  double best_score;
  T4 best_candidate;
  // aux
  pthread_t tid;
};

void *chs_mi_thread(void *arg) {
  mi_job *job = (mi_job *) arg;
  unsigned int i;
  double score;

  lok.lock();
  busythreads++;
  lok.unlock();

  for(i=0;i<job->candidates.size();i++) {
    //printf("thread id=%d cand=%d of %d\n",(int)(job->tid),i+1,(int)(job->candidates.size()));

    score = mi_chs_eval(job->a,job->b,job->candidates[i]);
    if (score > job->best_score) {
      job->best_score = score;
      job->best_candidate = job->candidates[i];
    }
  }

  lok.lock();
  busythreads--;
  lok.unlock();

  return NULL;
}

void mi_chs(Volume<int> *a, Volume<int> *b, T4 &T, vector<T4> &tvec) {
  double bscore, pbscore;
  int it = 0;
  T4 cur, tmp, best;
  unsigned int k;
  int i;
  char msg[256];
  Timestamp tic, toc;
  mi_job jobs[32];

  tic = Timestamp::now();

  a->maximum();
  b->maximum();

  cur = T;
  best = cur;
  bscore = mi_chs_eval(a,b,cur);

  rbest.nhist = 1;
  rbest.scorehist[0] = bscore;

  do {
    it++;
    pbscore = bscore; // previous best score

    /* original code before parallelization 
    for(k=0;k<tvec.size();k++) {
      tmp = cur;
      tmp *= tvec[k];
      score = mi_chs_eval(a,b,tmp);
      if (score > bscore) {
	bscore = score;
	best = tmp;
      }
    }
    */

    for(i=0;i<ncpus;i++)
      jobs[i].candidates.clear();

    // parallel version    
    for(k=0;k<tvec.size();k++) {
      tmp = cur;
      tmp *= tvec[k];
      jobs[ k % ncpus ].candidates.push_back(tmp);
      jobs[ k % ncpus ].a   = a;
      jobs[ k % ncpus ].b   = b;
      jobs[ k % ncpus ].best_score     = bscore;
      jobs[ k % ncpus ].best_candidate = cur;
    }

    for(i=0;i<ncpus;i++)
      pthread_create(&(jobs[i].tid), NULL, chs_mi_thread, &(jobs[i]));

    for(i=0;i<ncpus;i++)
      pthread_join(jobs[i].tid, NULL);

    for(i=0;i<ncpus;i++)
      if (!jobs[i].candidates.empty())
	if (jobs[i].best_score > bscore) {
	  bscore = jobs[i].best_score;
	  best   = jobs[i].best_candidate;
	}

    // end of parallelization changes

    if (rbest.nhist < MAXSCOREHIST) {
      rbest.scorehist[rbest.nhist] = bscore;
      rbest.nhist++;
    }

    cur = best;

    if (logging) {
      toc = Timestamp::now();
      printf("chs-mi %d %.3f %.2f\n",it,(toc-tic),bscore);
    }

    sprintf(msg,"CHS optimization (iteration #%d, score %.2f)",it,bscore);
    lok.lock();
    strcpy(bgtask.minor, msg);
    lok.unlock();

  } while(bscore > pbscore);

  rbest.score = bscore;
  rbest.iterations = it;

  T = best;
}

// IEEE TMI, Thevenaz 2000
double mi_chs_eval(Volume<int> *a, Volume<int> *b, T4 &T) {
  double D=0.0;
  int x1,y1,z1,x,y,z;
  R3 p,q;
  
  int sba = 0, sbb = 0, va, vb;
  int i, j, *jh;
  const int misize = 64;
  int n = misize * misize;
  int ha[256], hb[256];
  double pj, pa, pb, count;

  va = a->maximum(true);
  vb = b->maximum(true);
  while(va > misize - 1) { va >>= 1; sba++; }
  while(vb > misize - 1) { vb >>= 1; sbb++; }
  
  jh = new int[n];
  for(i=0;i<n;i++) jh[i] = 0;

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

