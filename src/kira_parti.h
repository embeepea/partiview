#ifdef USE_KIRA
/*
 * Interface to Kira Starlab (www.manybody.org) library for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#if __cplusplus
extern "C" {
#endif

void kira_parti_init();

#define KIRA_VERBOSE	0x01
#define KIRA_READLATER	0x02
// int kira_open(struct dyndata *dd, struct stuff *st, ifstream *ins, int flags);  /* VERBOSE|READLATER */
int kira_read_more( struct stuff *st, int nmax, double tmax, double maxdelay );

	/* read, stopping after first of:
	 *   nmax paragraphs
	 *   tmax dynamical-end-time,
	 *   delaymax wall-clock reading time
	 */
double get_parti_time( struct stuff *st );
void set_parti_time( struct stuff *st, double reqtime );
int get_parti_time_range( struct dyndata *, struct stuff *st, double *tmin, double *tmax, int ready );
struct specklist *kira_get_parti( struct dyndata *, struct stuff *st, double realtime );

#define KIRA_NODES	0	/* Display nonleaf (center-of-mass) nodes? */
#define	KIRA_RINGS	1	/* Display marker rings for multiple systems? */
#define	KIRA_TREE	2
# define  KIRA_OFF	  0	   /* no */
# define  KIRA_ON	  1	   /* yes, show all */
# define  KIRA_ROOTS	  2	   /* show only for root nodes (one per clump) */
# define  KIRA_CROSS	  2	   /* for "tree": show tick-cross */
# define  KIRA_TICK	  3	   /* for "tree": show tick-mark only */

#define	KIRA_TICKSCALE	3

#define KIRA_RINGSIZE	4	/* What determines size of marker rings? */
# define  KIRA_RINGSEP    0	/*   instantaneous separation */
# define  KIRA_RINGA	  1	/*   abs(semimajor axis) of Keplerian orbit */

#define	KIRA_RINGSCALE	5	/* Multiplier for ring size */
#define KIRA_RINGMIN	6	/* Min apparent size of rings (pixels) */
#define	KIRA_RINGMAX	7	/* Max apparent size of rings (pixels) */
#define KIRA_TRACK	8	/* make camera track motion of given particle */

double kira_get( struct dyndata *, struct stuff *st, int what );
int    kira_set( struct dyndata *, struct stuff *st, int what, double value );

void   kira_draw( struct stuff *st, struct specklist *slhead, Matrix *Tc2w, float radperpix );

#if __cplusplus
} /* end extern "C" */
#endif

#endif /*USE_KIRA*/
