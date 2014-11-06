#ifndef CAT_MODELUTIL_H
#define CAT_MODELUTIL_H
/*
 * Utility functions for cat_model.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "shmem.h"

class Shmem {
	/* Shared-memory allocator.
	 */
  public:
	void *operator new( size_t s ) { totalloc += s; return NewN( char, s ); }
	// void *operator new[]( size_t s );
	void operator delete( void *p ) { Free(p); }
	// void operator delete[]( void *p );

	   /* Copy a string to shared memory */
	static char *strdup( char *str ) { return shmstrdup(str); }

	static void *salloc( size_t s ) { totalloc += s; return NewN( char, s ); }
	static void sfree( void *p ) { Free(p); }

	static int totalloc;
};

	/* Extendable array of vect's, stored in CAVEMalloc'ed memory.
	 * Should be a template.
	 */
template <class T>
class vvec : public Shmem {

  public:
	int count;	/* Number of items defined in *v */
	int room;	/* Total space available in *v */
	int ours;	/* "May we sfree(v)?" */
	T *v;		/* Data */

	vvec();
	~vvec();
	void init();
	vvec( int space );		/* initial estimate */
	void init( int space );		/* ditto */
	void use( T *buf, int space );	/* "Use this buffer" */
	void trim( int excess = 0 );	/* trim to size, and ensure CAVEMalloc()ed */
	T *needs( int nitems );

	T *append();	/* Extend by 1, and return address of new item */
	T *val( int index );	/* index'th item (extending if needed) */

	vvec<T> & operator= ( const vvec<T> & src );
};

#endif /*CAT_MODELUTIL_H*/
