#ifndef GREG_H
#define GREG_H 1

struct _bgtask {
  int algo;
  int nsteps;
  int step;
  char major[256];
  char minor[256];
  int ipar[10];
  Timestamp start;
};

#define MAXSCOREHIST 200

struct _rbest {
  char algo[128];
  T4 t;
  double score;
  double elapsed;
  int iterations;

  // score history, for plotting
  double scorehist[MAXSCOREHIST];
  int nhist;
};

// greg.ccexternals/globals
#ifdef GREG_CC
struct _bgtask bgtask;
Mutex lok;
struct _rbest rbest;
T4 best;
int ncpus = 0;
int logging = 0;
volatile int busythreads = 0;

#else
extern struct _bgtask bgtask;
extern Mutex lok;
extern struct _rbest rbest;
extern int ncpus;
extern int logging;
extern int busythreads;
#endif

// expose whatever is called from greg_*.cc
int envint(const char *var);

#define Max(a,b) ((a)>(b)?(a):(b))
#define Min(a,b) ((a)<(b)?(a):(b))

#endif
