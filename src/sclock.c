/*
 * Clocks, real-time and otherwise.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#if WIN32
# include <sys/timeb.h>
# include <memory.h>
# include "winjunk.h"
#else
# include <time.h>
# include <sys/types.h>
# include <sys/time.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <string.h>

#if CAVE
# include <cave_ogl.h>
#endif

#include "sclock.h"

double wallclock_time(void)
{
#if SZG_MAJOR_VERSION
  extern double syzygy_time(); /* in szgPartiview.cc */
  return syzygy_time();
#endif
#if CAVE
  return *CAVETime;
#else
#if WIN32
  static struct _timeb tm0;
  struct _timeb tm;
  _ftime( &tm );
  if(tm0.time == 0)
	  tm0 = tm;
  return tm.time - tm0.time + .001*(tm.millitm - tm0.millitm);
#else /*unix*/
  struct timeval now;
  static struct timeval tv0;
  gettimeofday(&now, NULL);
  if(tv0.tv_sec == 0)
    tv0 = now;
  return (now.tv_sec - tv0.tv_sec) + 1e-6*(now.tv_usec - tv0.tv_usec);
#endif
#endif
}

void clock_init( SClock *clk ) {
    memset(clk, 0, sizeof(*clk));
    clk->fwd = 1;
}

void clock_tick( SClock *clk ) {
    if(clk == NULL) return;
    if(!clk->running) return;

    if(clk->parent) {
	clk->curtime = clock_time(clk->parent)*clk->speed*clk->fwd + clk->basetime;
	/* ... or maybe some other function of parent's clock ... */
	clk->seqno = clk->parent->seqno;
    } else {
	clk->seqno++;
	if(clk->walltimed) {
	    double wallnow = (clk->wallfunc!=NULL)
				? (*clk->wallfunc)(clk) : wallclock_time();
	    if(clk->waswalltimed)
	    	clk->curtime += clk->fwd * clk->speed * (wallnow - clk->walllasttick);
	    clk->walllasttick = wallnow;
	} else {
	    clock_step(clk, clk->fwd);
	}
	clk->waswalltimed = clk->walltimed;
    }
}

double clock_time( SClock *clk ) {
    double curtime;

    if(clk == NULL) return 0;

    curtime = clk->curtime;
    if(curtime < clk->tmin) {
	double delta = fmod( clk->tmin - curtime,
				clk->tmax - clk->tmin + 2*clk->wrapband )
			- clk->wrapband;
	if(delta > 0) clock_set_time( clk, curtime = clk->tmax - delta );
	else curtime = clk->tmin;

    } else if(clk->curtime > clk->tmax) {
	double delta = fmod( clk->curtime - clk->tmax,
				clk->tmax - clk->tmin + 2*clk->wrapband )
			- clk->wrapband;
	if(delta > 0) clock_set_time( clk, curtime = clk->tmin + delta );
	else curtime = clk->tmax;
    }
    return clk->continuous ? curtime : rint(curtime);
}

void clock_set_fwd( SClock *clk, int fwd ) {
    fwd = (fwd>0) ? 1 : -1;
    if(clk && clk->fwd != fwd) {
	clk->basetime += 2*(clk->curtime - clk->basetime);
	clk->fwd = fwd;
    }
}

int clock_fwd( SClock *clk ) {
    return clk ? (clk->fwd>0 ? 1 : -1) : 0;
}

void clock_set_time( SClock *clk, double newtime ) {
    if(clk) {
	clk->basetime += newtime - clk->curtime;
	clk->curtime = newtime;
    }
}

void clock_set_speed( SClock *clk, double speed ) {
    /* Set speed while adjusting base to preserve value */
    if(clk && clk->speed != speed) {
	if(clk->speed != 0)
	    clk->basetime += (1 - speed/clk->speed) * clk->fwd
				* (clk->curtime - clk->basetime);
	clk->speed = speed;
    }
}

double clock_speed( SClock *clk ) {
    return clk ? clk->speed : 0;
}

void clock_set_timebase( SClock *clk, double timebase ) {
    if(clk)
	clk->basetime = timebase;
}

double clock_timebase( SClock *clk ) {
    return clk ? clk->basetime : 0;
}

void clock_set_step( SClock *clk, double step ) {
    if(clk) clk->deltatime = step;
}

void clock_step( SClock *clk, double sign ) {
    clock_add( clk, sign * clk->deltatime );
}

void clock_add( SClock *clk, double incr ) {
    if(clk) {
	clk->curtime += incr;
	clk->basetime += incr;
	clk->seqno++;
    }
}

void clock_set_running( SClock *clk, int running ) {
    if(clk) {
	clk->running = running;
	clk->waswalltimed = 0;
    }
}

int clock_running( SClock *clk ) {
    return clk ? clk->running : 0;
}

void clock_set_range( SClock *clk, double tmin, double tmax, double wrapband ) {
    /* if(tmin == tmax) tmax = tmin + 1; */
    clk->tmin = tmin;
    clk->tmax = tmax;
    clk->wrapband = wrapband>0 ? wrapband : 0;
}
