#ifdef USE_MODEL
/*
 * Utility functions for cat_model.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdlib.h>
#include "cat_modelutil.h"

int Shmem::totalloc;

/*
 * Variable-length arrays.
 */

template <class T>
vvec<T>::vvec() {
  init();
}

template <class T>
void vvec<T>::init() {
  count = room = ours = 0;
  v = NULL;
}

template <class T>
vvec<T>::vvec( int space ) {
  init(space);
}

template <class T>
void vvec<T>::init( int space ) {
  count = 0;
  if(space <= 0) space = 15;
  room = space;
  v = NewN( T, space );
  ours = 1;
}

template <class T>
void vvec<T>::use( T *buf, int space ) {
  count = 0;
  if(ours) Free( v );
  room = space;
  v = buf;
  ours = 0;
}

template <class T>
void vvec<T>::trim( int excess ) {
  if(v != NULL) {
    if(ours) {
	v = RenewN( v, T, count+excess );
    } else {
	T *tv;
	tv = NewN( T, count+excess );
	memcpy( tv, v, count*sizeof(T) );
	v = tv;
    }
    room = count + excess;
    ours = 1;
  }
}

template <class T>
vvec<T> & vvec<T>::operator= ( const vvec<T> & src ) {
    room = src.room;
    count = src.count;
    v = src.v;
    ours = 0;
    trim( 0 );
    return *this;
}


template <class T>
T *vvec<T>::needs( int nitems ) {
  if(room < nitems) {
    int newroom = (2*room < nitems) ? nitems + (nitems/2) : 2*room;
    T *newv = NewN( T, newroom );
    if(count>0 && v != NULL)
	memcpy( newv, v, count*sizeof(T) );
    if(ours && v) Free( v );
    ours = 1;
    room = newroom;
    v = newv;
  }
  return v;
}

template <class T>
T *vvec<T>::append() {
  T *p = val(count);
  return p;
}

template <class T>
T *vvec<T>::val( int index ) {
  needs( index+1 );
  if(count <= index)
    count = index+1;
  return &v[index];
}

template <class T>
vvec<T>::~vvec() {
  if(ours && v)
	Free( v );
}
#endif /*USE_MODEL*/
