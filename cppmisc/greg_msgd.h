
#ifndef GREG_MSGD_H
#define GREG_MSGD_H 1

#include "brain_imaging.h"

void          msgd(double **delta, double *theta, Volume<int> *B, vector<int> & S, R3 &cg, Volume<int> *mov, double *best);
Volume<int>  *msgd_register(Volume<int> *a, Volume<int> *b);

void          mktrans(T4 &T, double *par, R3 &cg);
double        cfrg(Volume<int> *B, vector<int> &S, R3 &cg, Volume<int> *mov, double *theta, double *delta, int i, int dir);
void          acc_hist_norm(Volume<int> *v);
Volume<int>  *watershed(Volume<int> *v, double radius, double hfac);
Volume<char> *borders(Volume<int> *v);
void          calcCG(Volume<char> *v, R3 &cg);

void          mi_msgd(double **delta, double *theta, Volume<int> *a, Volume<int> *b, R3 &cg, double *best);
Volume<int>  *msgd_miregister(Volume<int> *a, Volume<int> *b);
double        mi_cfrg(Volume<int> *a, Volume<int> *b, R3 &cg, double *theta, double *delta, int ii, int dir);


#endif
