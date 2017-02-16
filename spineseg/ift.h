/*

    SpineSeg
    http://www.liv.ic.unicamp.br/~bergo/spineseg
    Copyright (C) 2009-2011 Felipe Paulo Guazzi Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

// classes here: Location PriorityQueue BasicPQ Adjacency SphericalAdjacency DiscAdjacency
      
#ifndef IFT_H
#define IFT_H 1

#include <assert.h>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

//! Point in discrete 3D space
class Location {
 public:
  int X /** X coordinate */,Y /** Y coordinate */,Z /** Z coordinate */;

  //! Constructor
  Location() { X=Y=Z=0; }

  //! Constructor, sets (X,Y,Z)=(x,y,z)
  Location(int x,int y,int z) { X=x; Y=y; Z=z; }

  //! Copy constructor
  Location(const Location &src) { X=src.X; Y=src.Y; Z=src.Z; }

  //! Copy constructor
  Location(const Location *src) { X=src->X; Y=src->Y; Z=src->Z; }

  //! Translates this location by src, returns a reference to this
  //! location.
  Location & operator+=(const Location &src) { 
    X+=src.X; Y+=src.Y; Z+=src.Z;
    return(*this);
  }

  //! Returns true if this location is closer to the origin than b.
  bool operator<(const Location &b) const { 
    return(sqlen()<b.sqlen()); 
  }

  //! Returns true if this and b are the same location
  bool operator==(const Location &b) const { 
    return(X==b.X && Y==b.Y && Z==b.Z); 
  }

  //! Returns true if this and b are different locations
  bool operator!=(const Location &b) const { 
    return(X!=b.X || Y!=b.Y || Z!=b.Z); 
  }

  //! Assignment operator
  Location & operator=(const Location &b) { X=b.X; Y=b.Y; Z=b.Z; return(*this); }

  //! Sets (X,Y,Z) to (x,y,z), return a reference to this location.
  Location & set(int x,int y,int z) { X=x; Y=y; Z=z; return(*this); }
  
  //! Prints this location to stdout.
  void print() { cout << "(" << X << "," << Y << "," << Z << ") : " << sqlen() << "\n"; }

  //! Returns the square of the distance from the origin
  int sqlen() const { return( (X*X)+(Y*Y)+(Z*Z) ); }

  //! Returns the square of the distance between this and b
  int sqdist(const Location &b) {
    return( (X-b.X)*(X-b.X) + (Y-b.Y)*(Y-b.Y) + (Z-b.Z)*(Z-b.Z) );
  }
};

//! Abstract priority queue interface
class PriorityQueue {
 public:
  /** Tag: element was never queued */
  static const int White = 0;
  /** Tag: element is queued */ 
  static const int Gray  = 1;
  /** Tag: element was queued and dequeued */
  static const int Black = 2; 
  /** Internal list tag */ 
  static const int Nil   = -1;

  virtual ~PriorityQueue() { }

  //! Returns true if there are no elements queued
  virtual bool empty()=0;

  //! Empties the queue and turn all elements White
  virtual void reset()=0;

  //! Inserts element elem into the given bucket
  virtual void insert(int elem, int bucket)=0;

  //! Removes and returns the next element to be dequeued
  virtual int  remove()=0;

  //! Moves element elem from bucket <b>from</b> to bucket <b>to</b>
  virtual void update(int elem, int from, int to)=0;

  //! Returns the color tag of an element
  virtual int colorOf(int elem)=0;
};

//! Basic linear-time priority queue
class BasicPQ : public PriorityQueue {
 public:

  //! Constructor, creates a priority queue for elements 0..nelem-1 and
  //! up to nbucket priority buckets
  BasicPQ(int nelem, int nbucket) {
    NB = nbucket;
    NE = nelem;
    first = new int[NB];
    last  = new int[NB];
    prev = new int[NE];
    next = new int[NE];
    color = new char[NE];

    reset();
  }

  //! Destructor
  virtual ~BasicPQ() {
    if (first) delete first;
    if (last) delete last;
    if (prev) delete prev;
    if (next) delete next;
    if (color) delete color;
  }

  //! Returns true if the queue is empty
  bool empty() {
    int i;
    for(i=current;i<NB;i++)
      if (first[i] != Nil)
	return false;
    return true;
  }

  //! Empties the queue and turn all elements White
  void reset() { 
    int i;
    current = 0;

    for(i=0;i<NB;i++)
      first[i] = last[i] = Nil;

    for(i=0;i<NE;i++) {
      prev[i] = next[i] = Nil;
      color[i] = White;
    }
  }

  //! Inserts element elem into the given bucket
  void insert(int elem, int bucket) { 

    #ifdef SAFE
    assert(elem>=0 && elem<NE);
    assert(bucket>=0 && bucket<NB);
    #endif

    if (first[bucket] == Nil) {
      first[bucket] = elem;
      prev[elem] = Nil;
    } else {
      #ifdef SAFE
        assert(last[bucket]>=0 && last[bucket]<NE);
      #endif
      next[last[bucket]] = elem;
      prev[elem] = last[bucket];
    }
    last[bucket] = elem;
    next[elem] = Nil;
    color[elem] = Gray;
    if (bucket < current) current = bucket;
  }

  //! Removes and returns the next element to be dequeued
  int remove() { 
    int elem,n;

    #ifdef SAFE
    assert(current>=0 && current < NB);
    #endif

    while(first[current]==Nil && current < NB)
      ++current;
    if (current == NB) return Nil;
    elem = first[current];

    n = next[elem];
    if (n==Nil) {
      first[current] = last[current] = Nil;
    } else {
      first[current] = n;
      prev[n] = Nil;
    }
    color[elem] = Black;
    return elem;
  }

  //! Moves element elem from bucket <b>from</b> to bucket <b>to</b>
  void update(int elem, int from, int to) { 
    removeElem(elem,from);
    insert(elem,to);
  }

  //! Returns the color tag of an element
  int colorOf(int elem) { return((int)(color[elem])); }

 private:
  int NB,NE,current,*first, *last,*prev,*next;
  char *color;

  void removeElem(int elem, int bucket) {
    int p,n;

    #ifdef SAFE
    assert(elem>=0 && elem<NE);
    assert(bucket>=0 && bucket<NB);
    #endif

    p = prev[elem];
    n = next[elem];

    color[elem] = Black;
    
    if (first[bucket] == elem) {
      first[bucket] = n;
      if (n==Nil)
	last[bucket] = Nil;
      else
	prev[n] = Nil;
    } else {
      next[p] = n;
      if (n==Nil)
	last[bucket] = p;
      else
	prev[n] = p;
    }
  }

};

//! Abstract discrete voxel adjacency
class Adjacency {
 public:
  //! Returns the number of elements in this adjacency
  int size() { return(data.size()); }

  //! Returns the index-th (0-based) neighbor displacement, as a \ref Location
  Location & operator[](int index) {
    return(data[index]);
  }

  //! Returns the i-th (0-based) neighbor of src, as a \ref Location
  Location neighbor(const Location &src, int i) {
    Location tmp;
    tmp = src;
    tmp += data[i];
    return(tmp);
  }

 protected:
  //! Vector of neighbors
  vector<Location> data;
};

//! Discrete spherical voxel adjacency
class SphericalAdjacency : public Adjacency {

 public:  
  //! Constructor
  SphericalAdjacency();

  //! Constructor, creates adjacency of given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  SphericalAdjacency(float radius,bool self=false) {
    resize(radius,self);
  }

  //! Recreates this adjacency, with given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  void resize(float radius, bool self=false) {
    int dx,dy,dz,r0,r2;

    data.clear();
    r0 = (int) radius;
    r2  = (int)(radius*radius);

    for(dz=-r0;dz<=r0;dz++)
      for(dy=-r0;dy<=r0;dy++)
	for(dx=-r0;dx<=r0;dx++) {
	  Location loc(dx,dy,dz);
	  if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
	    Location *nloc = new Location(loc);
	    data.push_back(*nloc);
	  }
	}
    sort(data.begin(),data.end());
  }

};

//! Discrete circular adjacency on the XY plane
class DiscAdjacency : public Adjacency {

 public:
  //! Constructor
  DiscAdjacency();

  //! Constructor, creates adjacency of given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  DiscAdjacency(float radius,bool self=false) {
    resize(radius,self);
  }

  //! Recreates this adjacency, with given radius. If self is true,
  //! the center voxel is considered its own neighbor.
  void resize(float radius, bool self=false) {
    int dx,dy,r0,r2;

    data.clear();
    r0 = (int) radius;
    r2  = (int)(radius*radius);

    for(dy=-r0;dy<=r0;dy++)
      for(dx=-r0;dx<=r0;dx++) {
	Location loc(dx,dy,0);
	if ((loc.sqlen() > 0  || self) && loc.sqlen() <= r2) {
	  Location *nloc = new Location(loc);
	  data.push_back(*nloc);
	}
      }
    sort(data.begin(),data.end());
  }

};

#endif


