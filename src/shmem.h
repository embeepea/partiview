#ifndef _SHMEM_H
#define _SHMEM_H
/*
 * Memory allocation.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONST
# ifdef __cplusplus
# define CONST const
# else
# define CONST
# endif
#endif

#if CAVEMENU
# define CAVE 1
#endif

#if CAVE
	/* Shared memory definitions */

#ifdef HAVE_MALLOC_H
# include <malloc.h>	/* for amalloc() */
#endif

#ifdef __cplusplus
# define NewN(type, count)     (static_cast <type *>(shmalloc((count) * sizeof(type))))
# define RenewN(p,type,count)  (static_cast <type *>(shmrealloc(p, (count) * sizeof(type))))
#else
# define NewN(type, count)     (type *)shmalloc((count) * sizeof(type))
# define RenewN(p,type,count)  (type *)shmrealloc(p, (count) * sizeof(type))
#endif

#define Free(p)		      shmfree( p )

extern void *aarena;
extern void shminit( int bytes );
extern void shmusearena( void *arena );

extern void *shmalloc(int nbytes);
extern void  shmfree(void *ptr);
extern void *shmrealloc(void *ptr, int nbytes);
extern char *shmstrdup(char *str);

	/* If we run out of memory, ask (*func)(data, nbytes, arena)
	 * to free some.
	 * func should return 1 if it freed something (i.e. if it's
	 *   worth retrying the allocation), 0 if no improvement.
	 * We should really have a list of these functions,
	 * for multiple separately-written clients of our shmem pool.
	 */
extern void shmrecycler( int (*func)(void *, int, void *), void *data );

#else /* non-CAVE */

#ifdef __cplusplus
# define NewN(type, count)	(static_cast <type *>(malloc((count) * sizeof(type))))
# define RenewN(p,type,count)	(static_cast <type *>(realloc(p, (count) * sizeof(type))))
#else /* plain C */
# define NewN(type, count)	((type *)malloc((count) * sizeof(type)))
# define RenewN(p,type,count)	((type *)realloc(p, (count) * sizeof(type)))
#endif

#define Free(p)			free( p )

extern char *shmstrdup( CONST char * );

#endif /*!CAVEMENU*/


#define OOGLNewNE(type, count, str)    NewN(type, count)
#define OOGLReNewNE(p,type,count,str)  RenewN(p,type,count)
#define OOGLFree(p)		       Free(p)


#ifdef __cplusplus
}
#endif
#endif
