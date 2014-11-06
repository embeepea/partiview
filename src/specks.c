#define _XOPEN_SOURCE 500	/* for UNIX98 pthread mutexes */

#include <errno.h>
#include <stdlib.h>
#include "specks.h"
#include "shmem.h"
#include <string.h>
#include "partiviewc.h"		/* for msg() */

	/* only safe if lock held */
static struct specklist **specks_timespecksptr( struct stuff *, int dataset, int timestep );

#ifdef HAVE_PTHREAD_H
void specks_lock_init( struct stuff *st )
{
    static pthread_mutexattr_t ma;
    pthread_mutexattr_init( &ma );
    pthread_mutexattr_settype( &ma, PTHREAD_MUTEX_ERRORCHECK );
    pthread_mutex_init( &st->smut, &ma );
    pthread_mutexattr_destroy( &ma );
}

void specks_lock( struct stuff *st )
{
    int e = pthread_mutex_lock( &st->smut );
    if(e != 0) {
	fprintf(stderr, "specks_lock(%p): %s\n", st, strerror(e));
    }
}

void specks_unlock( struct stuff *st )
{
    int e = pthread_mutex_unlock( &st->smut );
    if(e != 0) {
	fprintf(stderr, "specks_unlock(%p): %s\n", st, strerror(e));
    }
}
#else /* no pthreads */
void specks_lock_init( struct stuff *st ) {}
void specks_lock( struct stuff *st ) {}
void specks_unlock( struct stuff *st ) {}
#endif

struct specklist *specks_timespecks( struct stuff *st, int dataset, int timestep )
{
    struct specklist *sl;
    specks_lock( st );
    sl = (timestep >= 0 && timestep < st->ntimes &&
			dataset >= 0 && dataset < st->ndata)
	? st->anima[dataset][timestep] : NULL;
    specks_unlock( st );
    return sl;
}

void specks_insertspecks( struct stuff *st, int dataset, int timestep, struct specklist *sl )
{
    struct specklist **slp;

    if(dataset < 0 || timestep < 0)
	return;
    specks_ensuretime( st, dataset, timestep );

    specks_lock( st );
    slp = &st->anima[dataset][timestep];
    sl->next = *slp;
    *slp = sl;
    specks_unlock( st );
}


static void specks_freenow( struct specklist **slp )
{
  struct specklist *sl, **sprev;
  int any = 0;

  for(sprev = slp; (sl = *sprev) != NULL; ) {
    *sprev = sl->next;
    if(sl->specks != NULL)
	Free(sl->specks);
    Free(sl);
  }
}

static int specks_freeoldscrap( struct specklist **slp, int maxage )
{
  struct specklist *sl, **sprev;
  int any = 0;

  for(sprev = slp; (sl = *sprev) != NULL && sl->used <= maxage; ) {
    if(sl->used <= maxage) {
	*sprev = sl->freelink;
	if(sl->specks != NULL)
	    Free(sl->specks);
	Free(sl);
	any++;
    } else {
	/* too recent - might still be in use */
	sprev = &sl->freelink;
    }
  }
  return any;
}

void specks_discard( struct stuff *st, struct specklist **slp )
{
  struct specklist *sl, *slnext;

  for(sl = *slp; sl != NULL; sl = slnext) {
    slnext = sl->next;
    sl->freelink = st->scrap;
    st->scrap = sl;
  }
  *slp = NULL;
}

int specks_purge( void *vst, int nbytes, void *aarena )
{
  struct stuff *st = (struct stuff *)vst;
  int oldused = st->used;
  int oldtime = -1, oldds = -1;
  int t, ds;
  struct specklist *sl;

#ifdef sgi
  static int first = 1;
  struct mallinfo mi;
  mi = amallinfo( aarena );

  if(first) {
    first = 0;
    msg("Purging %dKbyte shmem arena (currently %dK used in %d blks, %dK free)",
	mi.arena>>10, mi.uordblks>>10, mi.ordblks, mi.fordblks>>10);
  }
#endif

  /* Free any known scrap first */

#define OLD_ENOUGH  4

  if(specks_freeoldscrap( &st->scrap, st->used - OLD_ENOUGH ) > 0)
    return 1;

  for(t = 0; t < st->ntimes; t++) {
    if(t == st->curtime) continue;
    for(ds = 0; ds < st->ndata; ds++) {
	sl = st->anima[ds][t];
	if(sl != NULL && sl != st->sl && sl->used < oldused) {
	    oldused = st->used;
	    oldtime = t;
	    oldds = ds;
	}
    }
  }
  if(oldtime >= 0) {
    specks_discard( st, &st->anima[oldds][oldtime] );
    specks_freeoldscrap( &st->scrap, oldtime );
    return 1;	/* We freed something, so try allocating again */
  } else {
    msg("Ran out of shmem, couldn't find anything more to purge");
#ifdef sgi
    msg("%dKbyte shmem arena (currently %dK used in %d blks, %dK free)",
	mi.arena>>10, mi.uordblks>>10, mi.ordblks, mi.fordblks>>10);

#endif
    return 0;	/* No progress made -- give up */
  }
}

static struct specklist **specks_scraptail( struct stuff *st )
{
  struct specklist **slp = &st->scrap;
  while(*slp)
    slp = &(*slp)->freelink;
  return slp;
}

void specks_clearspecks( struct stuff *st, int dataset, int timestep )
{
    struct specklist **slp;

    if(st->anima == NULL || st->anima[dataset] == NULL)
	return;

    specks_lock(st);
    slp = &st->anima[dataset][timestep];
    if(*slp == st->frame_sl) {
	/* in use -- add to purge-list */
	specks_discard( st, slp );
    } else {
	specks_freenow( slp );
    }
    specks_unlock(st);
}

void specks_ensuretime( struct stuff *st, int dataset, int timestep )
{
  int d, needroom;
  struct specklist **na, **nan;
  void **ndf;
  char **nfn;
  struct mesh **nmesh;

  if(timestep < st->ntimes)
	return;

  specks_lock( st );

  if((timestep >= st->ntimes || dataset >= st->ndata)) {
    needroom = st->timeroom;
    if(timestep >= st->timeroom)
	needroom = 2*timestep + 15;

    for(d = 0; d < st->ndata || (dataset < MAXFILES && d <= dataset); d++) {

	if(needroom == st->timeroom && d < st->ndata)
	    continue;

	na = NewN( struct specklist *, needroom );
	nan = NewN( struct specklist *, needroom );
	ndf = NewN( void *, needroom );
	nfn = NewN( char *, needroom );
	nmesh = NewN( struct mesh *, needroom );
	memset(na, 0, needroom * sizeof(*na));
	memset(nan, 0, needroom * sizeof(*nan));
	memset(ndf, 0, needroom * sizeof(*ndf));
	memset(nfn, 0, needroom * sizeof(*nfn));
	memset(nmesh, 0, needroom * sizeof(*nmesh));
	if(d < st->ndata && st->anima[d])
	    memcpy( na, st->anima[d], st->ntimes * sizeof(*na) );

	if(d < st->ndata && st->annot[d])
	    memcpy( nan, st->annot[d], st->ntimes * sizeof(*nan) );

	if(d < st->ndata && st->datafile[d])
	    memcpy( ndf, st->datafile[d], st->ntimes * sizeof(*ndf) );

	if(d < st->ndata && st->fname[d])
	    memcpy( nfn, st->fname[d], st->ntimes * sizeof(*nfn) );

	if(d < st->ndata && st->meshes[d])
	    memcpy( nmesh, st->meshes[d], st->ntimes * sizeof(*nfn) );

	/* Don't free old pointers, just in case they're in use. */
	st->anima[d] = na;
	st->annot[d] = nan;
	st->datafile[d] = ndf;
	st->fname[d] = nfn;
	st->meshes[d] = nmesh;
    }
    st->timeroom = needroom;

    if(timestep >= st->ntimes)
	st->ntimes = timestep + 1;
    if(dataset >= st->ndata && dataset < MAXFILES)
	st->ndata = dataset + 1;
  }
  specks_unlock( st );
}
