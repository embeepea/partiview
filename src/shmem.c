/*
 * Memory allocation.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
# include "winjunk.h"
#else
# include <unistd.h>
#endif

#include "shmem.h"

#if CAVE

#include <cave_ogl.h>

static int (*shm_purge_func)( void *data, int nbytes, void *arena ) = NULL;
static void *shm_purge_data;

void *aarena;

void shminit( int nbytes ) {
  aarena = CAVEUserSharedMemory( (unsigned int) nbytes );
  if(aarena == NULL) {
    fprintf(stderr, "Couldn't create CAVEUserSharedMemory of %dMbytes!\n",
	((unsigned int)nbytes) >> 20);
    exit(1);
  }
  amallopt(M_DEBUG, 1, aarena);
}

void shmusearena( void *a ) {
  aarena = a;
}

int shm_malloc_zero;

void *shmalloc( int nbytes ) {
  void *p;
  if(aarena == NULL) {
    fprintf(stderr, "shmalloc: Never called shminit( arenasize )!\n");
    exit(1);
  }
  if(nbytes <= 0) {
    shm_malloc_zero++;
    nbytes = 1;
  }

  do {
      p = amalloc( nbytes, aarena );
      if(p != NULL)
	return p;
  } while (shm_purge_func != NULL
		&& 0 != (*shm_purge_func)( shm_purge_data, nbytes, aarena ));
  fprintf(stderr, "Couldn't shmalloc %d bytes!  Pausing for debugging (pid %d)\n", nbytes, getpid());
  sleep(10);
  exit(1);
  return NULL;
}

void *shmrealloc( void *oldp, int nbytes ) {
  void *p;
  if(aarena == NULL) {
    fprintf(stderr, "shmrealloc: Never called shminit( arenasize )!\n");
    exit(1);
  }
  do {
      p = arealloc( oldp, nbytes, aarena );
      if(p != NULL)
	return p;
  } while (shm_purge_func != NULL
		&& 0 != (*shm_purge_func)( shm_purge_data, nbytes, aarena ));
  fprintf(stderr, "Couldn't shmrealloc %d bytes!  Pausing for debugging (pid %d)\n", nbytes, getpid());
  sleep(10);
  exit(1);
  return NULL;
}

void shmfree( void *p ) {
  if(aarena && p)
    afree( p, aarena );
}

char *shmstrdup( char *str ) {
  char *s;
  if(str == NULL) return NULL;
  s = shmalloc(strlen(str)+1);
  if(s == NULL) return NULL;
  strcpy(s,str);
  return s;
}
  
void shmrecycler( int (*func)(void *, int, void *), void *data )
{
  shm_purge_func = func;
  shm_purge_data = data;
}

#else

  /* non-CAVE -- ordinary single process address space? */

char *shmstrdup( CONST char *str ) {
  return str ? strdup(str) : NULL;
}

#endif /*CAVEMENU*/
