#ifndef SCLOCK_H
#define SCLOCK_H
/*
 * Clocks, real-time and otherwise.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "notify.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct sclock_ SClock;

struct sclock_ {
  int seqno;			/* incremented for each update */
  int running;
  int fwd;
  int walltimed;
  int continuous;
  int pertick;
  double speed;			/* per real second, or per parent-clock unit */
  double fspeed;		/* per clock_tick() */
  double deltatime;		/* scales arg to clock_step() */
  double curtime;		/* now */
  double basetime;
  double (*wallfunc)(struct sclock_ *);

  int clamped, wrapped;
  double tmin, tmax;		/* valid range of times */
  double wrapband;

  double walllasttick;		/* wall-clock time of last tick */
  int waswalltimed;		/* was previous tick in walltimed mode? */
  SClock *parent;
  Notify *notify;		/* list of functions to call when we change */
};

/*
 * If parent is set, our time is
 *	curtime = clock_read(parent)*speed + basetime
 *	  (or maybe eventually some more complicated function)
 * otherwise it's
 *	curtime
 * How to change a (non-parented) clock:
 *   clock_tick(clk)
 *	if running & walltimed, adds speed * time-since-last-tick
 *	if running & !walltimed, does clock_step(clk, 1)
 *   clock_step(clk, N)
 *	adds N*deltatime to curtime.  ignores speed/fspeed.
 *   clock_set(clk, T)
 *	sets curtime.
 */
double wallclock_time( void );
void   clock_init( SClock * );
double clock_time( SClock * );
int    clock_fwd( SClock * );		/* returns +1 (fwd) or -1 (back) */
int    clock_running( SClock * );
double clock_speed( SClock * );
double clock_timebase( SClock * );
void   clock_set_time( SClock *, double newtime );
void   clock_set_running( SClock *, int running );
void   clock_set_fwd( SClock *, int fwd );
void   clock_set_step( SClock *, double deltatime );
void   clock_set_speed( SClock *, double speed );
void   clock_set_timebase( SClock *, double timebase );
void   clock_set_range( SClock *, double tmin, double tmax, double wrapband );
void   clock_range_include( SClock *, double t0, double t1 );
void   clock_add( SClock *, double incr );
void   clock_step( SClock *, double sign );
void   clock_tick( SClock * );
void   clock_add_notify( SClock *, NotifyFunc func, void *arg );
void   clock_remove_notify( SClock *, NotifyFunc func, void *arg );
void   clock_notify( SClock * );

#ifdef __cplusplus
}  /* end extern "C" */
#endif
#endif /* SCLOCK_H */
