#define NEWSTDIO

/*
 * Network server offering particle snapshots of kira/Starlab data
 * (www.manybody.org).
 * 
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

/* 
 * $Log: kiraserver.cc,v $
 * Revision 1.19  2003/07/29 21:15:37  steve
 * Changed Starlab vectors to vec.  Defined NEWSTDIO to allow compilatin
 * under gcc 3.2 (not sure if 2.9x is broken...).
 *
 * Revision 1.18  2002/06/18 21:25:25  slevy
 * Add reference to LICENSE.partiview file.
 * Where appropriate, update references to geomview license too,
 * now called COPYING.geomview.
 *
 * Revision 1.17  2002/05/23 22:38:04  slevy
 * Allow respecifying logT -> color map, and particle-id base number.
 *
 * Revision 1.16  2002/05/12 20:24:51  slevy
 * Er, um, preload_pdyn takes *two* booleans.  Ask possibly for
 * debug, always for vel.
 *
 * Revision 1.15  2002/05/12 19:42:47  slevy
 * Don't preload_pdyn() by default; only do if -P command-line option given.
 * And, ask for velocities (vel=true) when we do preload.
 *
 * Revision 1.14  2002/05/12 19:26:13  slevy
 * Add \n's to continued quoted lines.
 *
 * Revision 1.13  2002/05/12 19:17:59  slevy
 * Add interactive mode (kiraserver -i) to simplify debugging.
 *
 * Revision 1.12  2002/05/12 09:16:22  slevy
 * More debugging code.
 *
 * Revision 1.11  2002/04/29 00:11:25  slevy
 * Propagate velocity to sdb's, too.  Allow delta-t arg to time: t=<time>,<dt>
 * Need to pass vel=true arg to create_interpolated_tree*.
 * Hack: pass velocity back to specks in SEPVAL argument for leaf nodes.
 *
 * Revision 1.10  2002/04/26 20:15:50  slevy
 * Add debug mode (-v).  Split status dump into report() function.
 * Allow multiple requests per connection -- leave connection open
 * until we get EOF.
 *
 * Revision 1.9  2002/04/24 01:21:37  slevy
 * Allow -o option (o=1, o=2): specify using old tree-code.
 *
 * Revision 1.8  2002/04/23 03:21:22  slevy
 * Add rgb565 blackbody-color encoding from Mitchell Charity's code.
 *
 * Revision 1.7  2002/04/22 23:16:50  slevy
 * Ignore SIGPIPEs in case the caller disconnects early.
 * Accept T=<scalefactor> as well as T=<16 numbers>.
 * Report density center both in original and transformed coords.
 *
 * Revision 1.6  2002/04/17 20:46:07  slevy
 * Pass out just the leaf nodes (stars), ignore center-of-mass nodes.
 *
 * Revision 1.5  2002/04/17 15:51:11  slevy
 * Print starlab-computed density-center position too.
 * isalpha for Irix 6.5 back-compat.
 *
 * Revision 1.4  2002/04/16 19:10:45  slevy
 * Don't use curstate.T0 unless it's initialized!
 *
 * Revision 1.3  2002/04/16 18:54:33  slevy
 * Hack around inconsisent prototypes for accept().
 * Return more complete information -- transform etc. -- when
 * asked for neither speck nor sdb data.
 *
 * Revision 1.2  2002/04/16 15:25:34  slevy
 * Mollify gcc under Linux: sockaddr, etc.
 *
 * Revision 1.1  2002/04/16 15:17:01  slevy
 * Simple network server to cough up a list of particles at a given time.
 *
 */

#ifdef NEWSTDIO
#include <ostream.h>
#include <istream.h>
#endif /*NEWSTDIO*/

#include <stdio.h>
#include <stdarg.h>

#include "worldline.h"

#include "shmem.h"
#include "findfile.h"
#include "specks.h"

#include "kira_parti.h"

#include "stardef.h"

#include <unistd.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <signal.h>

#include <errno.h>

#include <ctype.h>
#undef isdigit		/* irix 6.5 back-compat hack */
#undef isalpha
static char local_id[] = "$Id: kiraserver.cc,v 1.19 2003/07/29 21:15:37 steve Exp $";

extern struct specklist *kira_get_parti( struct stuff *st, double realtime );
void msg( CONST char *fmt, ... );

typedef worldbundle *worldbundleptr;

enum speckfields {
	SPECK_ID = 0,		// worldline index (= kira index for single stars,
				//	unique small negative int for others)
	SPECK_MASS = 1,		// mass/Msun
	SPECK_NCLUMP = 2,	// number of stars in clump
	SPECK_TLOG = 3,		// log10(Teff)
	SPECK_LUM = 4,		// L/Lsun?
	SPECK_STYPE = 5,	// star type: 1ms 2cd 3gs 4bh 5sg 6hb
	SPECK_ISMEMBER = 6,	// is member of cluster?
	SPECK_ROOTID = 7,	// worldline index of root of clump
	SPECK_TREEADDR = 8,	// bit-encoded tree address within our clump (0 for isolated stars)
	SPECK_RINGSIZE = 9,	// size of ring marker
	SPECK_SQRTMASS = 10,	// square root of mass, for handy brightness factor
	SPECK_MU = 11,		// mass ratio; = 0 for leaf nodes
	SPECK_SEPVEC = 12,	// separation vec[3]
	SPECK_NDATAFIELDS = 15
};

static char *fieldnames[] = {  // Must match ``enum speckfields'' !!
	"id",		// worldline index (=? kira index for singles), unique <0 for others
	"mass",		// log10(mass/Msun)
	"nclump",	// number of stars in clump, = 1 for singles
	"Tlog",		// log10( Teff )
	"Lum",		// L/Lsun?
	"stype",	// stellar type index
	"ismember",	// is member of cluster? (0/1)
	"rootid",	// id of root of clump, = our id for singles
	"treeaddr",	// binary-coded address in clump
	"ringsize",	// size of ring
	"sqrtmass",	// sqrt(mass/Msun)
	"mu",		// mass ratio for nonleaf nodes
	NULL
};

struct vald {
	float min, max, sum;
};

struct trailhead {
    int maxtrail, ntrails;
    int next;		/* ring buffer next-slot-to-use */
    real lasttime;
    struct speck *specks;
};

struct worldstuff {
    int nh, maxnh;
    worldbundleptr *wh;
    ifstream *ins;
    int ih;			// current worldbundle index
    real tmin, tmax;
    real tcur;			// current time
    real treq;			// requested time
    int readflags;		// KIRA_VERBOSE | KIRA_READLATER
    int treenodes;		// KIRA_{OFF|ON|ROOTS}
    int treerings;		// KIRA_{OFF|ON|ROOTS}
    int treearcs;		// KIRA_{OFF|ON|CROSS|TICK}
    float tickscale;		// size of treearc cross mark (frac of sep)
    int ringsizer;		// KIRA_RINGSEP, KIRA_RINGA
    float ringscale;		// multiplier for above
    float ringmin, ringmax;	// range of pixel sizes for ring markers
    int tracking, wastracking;	// id of particle we're tracking, or zero
    Point trackpos;		// last known position of tracked particle
    float massscale;		// scale-factor for masses (conv to Msun)
    int truemassscale;		// Did massscale come from kira itself?

    int maxstars, maxmarks;	// room allocated for each
    int maxleaves;
    struct specklist *sl;
    struct specklist *marksl;
    int slvalid;
    struct specklist *bufsl[2], *bufmarksl[2];  // double-buffers
    int bufno;
    int oldtree;
    vec center_pos;
    vec center_vel;
    int centered;
    int which_center;

    int myselseq;		// sequence number of ww->sel
    int nleafsel;
    SelMask *bufleafsel[2];

				// selection mapping
    SelOp intsrc;		// for all particles matching intsrc,
    SelOp intdest;		//   then turn on intdest bit(s).

    struct trailhead *trails;	// per-star specklist of recent history
    int maxtrail;
    int maxtrailno;
    SelOp trailsel;
    float trailalpha;
    float trailpsize;
    int trailonly;
    real maxtrailgap;

    SelOp picksel;		// what to do when a star is picked

    struct speck *marksp;	// current pointer, updated by add_speck
    int leafcount;		// temp, used in recursion only
    int interactsel, unionsel;	// ditto
    int pickcount;		// temp, used in kira_picked
    SelMask *leafsel;
    struct vald vd[SPECK_NDATAFIELDS];

    double curtime;

    worldstuff( struct stuff *st ) {
	this->init( st );
    }

    void init( struct stuff *st ) {
	nh = 0;
	wh = NULL;
	ih = 0;
	tmin = 0, tmax = 1;
	tcur = treq = 0;

	treenodes = KIRA_ON;
	treerings = KIRA_OFF;
	treearcs = KIRA_ON;
	ringsizer = KIRA_RINGA;
	ringscale = 1.5;
	massscale = 1.0;
	truemassscale = 0;

	ringmin = 2;
	ringmax = 50;

	tickscale = 0.25;
	tracking = wastracking = 0;
	centered = 0;  /* NOT auto-centered by default! */
	which_center = 0;
	oldtree = 0;
	center_pos[0] = center_pos[1] = center_pos[2] = 0;
	center_vel[0] = center_vel[1] = center_vel[2] = 0;

	sl = marksl = NULL;
	bufsl[0] = bufsl[1] = NULL;
	bufmarksl[0] = bufmarksl[1] = NULL;
	bufno = 0;
	maxstars = maxmarks = 0;
	slvalid = 0;

	trails = NULL;
	maxtrail = 50;
	maxtrailno = 0;
	trailonly = 0;
	trailalpha = 0.6;
	trailpsize = 1.0;
	maxtrailgap = 0.1;

	marksp = NULL;
	leafcount = 0;

	myselseq = 0;
	leafsel = NULL;
	bufleafsel[0] = bufleafsel[1] = NULL;
	nleafsel = 0;
    }
};

struct state {
  int type;
  int group;
  double realtime;
  double dt;
  char axis;
  float turnrate;
  double turntime0;
  Matrix T0;
  int has_T0;
  float mag0;
  float colorscale;
  float masslum;	/* if set, ignore SPECK_LUM; use MASS**3 * masslum */
  int oldtree;
  struct stuff *st;
  int port;
  int lsock;
  int verbose;
  int use_stdin;
  int preload;
  int idbase;
} curstate = { ST_POINT, 0, 0.0 };

void kira_invalidate( struct dyndata *dd, struct stuff *st ) {
    if(st->sl) st->sl->used = -1000000;
    worldstuff *ww = (worldstuff *)(dd->data);
    if(ww) ww->slvalid = 0;
}

int kira_read( struct stuff **stp, int argc, char *argv[], char *fname, void * ) {

    if(argc < 1 || strcmp(argv[0], "kira") != 0)
	return 0;

    struct stuff *st = *stp;
    char *realfile = findfile(fname, argv[1]);
    kira_open( &st->dyn, st, realfile ? realfile : argv[1], argc>2 ? atoi(argv[2]) : 0 );
    return 1;
}

int kira_set( struct dyndata *dd, struct stuff *st, int what, double val )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    if(ww == NULL) return 0;

    if(kira_get(dd, st, what) == val)
	return 2;

    switch(what) {
    case KIRA_NODES: ww->treenodes = (int)val; break;
    case KIRA_RINGS: ww->treerings = (int)val; break;
    case KIRA_TREE: ww->treearcs = (int)val; break;
    case KIRA_TICKSCALE: ww->tickscale = val; break;
    case KIRA_RINGSIZE: ww->ringsizer = (int)val; break;
    case KIRA_RINGSCALE: ww->ringscale = val; break;
    case KIRA_RINGMIN: ww->ringmin = val; break;
    case KIRA_RINGMAX: ww->ringmax = val; break;
    case KIRA_TRACK: if(ww->tracking != (int)val) ww->wastracking = 0;
		     ww->tracking = (int)val; break;
    default: return 0;
    }
    kira_invalidate( dd, st );
    return 1;
}

double kira_get( struct dyndata *dd, struct stuff *st, int what )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    if(ww == NULL) return 0;

    switch(what) {
    case KIRA_NODES: return ww->treenodes;
    case KIRA_RINGS: return ww->treerings;
    case KIRA_TREE:  return ww->treearcs;
    case KIRA_TICKSCALE: return ww->tickscale;
    case KIRA_RINGSIZE: return ww->ringsizer;
    case KIRA_RINGSCALE: return ww->ringscale;
    case KIRA_RINGMIN: return ww->ringmin;
    case KIRA_RINGMAX: return ww->ringmax;
    case KIRA_TRACK: return ww->tracking;
    }
    return 0;
}

int kira_get_center( Point *p, struct dyndata *dd, struct stuff *st ) {
    worldstuff *ww = (worldstuff *)dd->data;
    p->x[0] = ww->center_pos[0];
    p->x[1] = ww->center_pos[1];
    p->x[2] = ww->center_pos[2];
    return ww->which_center;
}

int hasdata( ifstream *s, double maxwait ) {
#ifdef NEWSTDIO
    return 1;
#else /*old stdio, where filebuf::fd() still existed*/
    struct timeval tv;
    fd_set fds;
    if(s == NULL) return 0;
    if(s->rdbuf()->in_avail() || s->eof()) return 1;
    if(maxwait < 0) maxwait = 0;
    tv.tv_sec = (int)maxwait;
    tv.tv_usec = (int) (1000000 * (maxwait - tv.tv_sec));
    FD_ZERO(&fds);
    FD_SET(s->rdbuf()->fd(), &fds);
    return(select(s->rdbuf()->fd() + 1, &fds, NULL, NULL, &tv) > 0);
#endif
}

int kira_help( struct dyndata *dd, struct stuff *st, int verbose )
{
    msg(" kira {node|ring|size|scale|span|track|trail}  starlab controls; try \"kira ?\"");
    return 1;
}

int kira_read_more( struct dyndata *dd, struct stuff *st, int nmax, double tmax, double realdtmax )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    if(ww == NULL) return -1;
    ifstream *ins = ww->ins;
    if(ins == NULL || !ins->rdbuf()->is_open()) return -1;

    int paras = 0;
    while( paras < nmax && (ww->nh == 0 || ww->wh[ww->nh-1]->get_t_max() < tmax) ) {

	if(ww->nh >= ww->maxnh) {
	    ww->maxnh = (ww->nh > ww->maxnh*2) ? ww->nh + 1 : ww->maxnh*2;
	    ww->wh = RenewN( ww->wh, worldbundleptr, ww->maxnh );
	}

	worldbundle *wb;

	wb = read_bundle(*ins, ww->readflags & KIRA_VERBOSE);
	if(wb == NULL) {
	    ins->close();
	    if(paras == 0) paras = -1;
	    break;
	}
	ww->wh[ww->nh++] = wb;
	paras++;
    }

    if(paras > 0) {
	ww->tmin = ww->wh[0]->get_t_min();
	ww->tmax = ww->wh[ww->nh-1]->get_t_max();
    }
    return paras;
}

int kira_get_trange( struct dyndata *dd, struct stuff *st, double *tminp, double *tmaxp )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    if(ww == NULL) {
	*tminp = *tmaxp = 0;
	return 0;
    }
    *tminp = ww->tmin;
    *tmaxp = ww->tmax;
    return 1;
}

int kira_open( struct dyndata *dd, struct stuff *st, char *filename, int readflags)
{
    // If data field names aren't already assigned, set them now.
    for(int i = 0; fieldnames[i] != NULL; i++) {
	if(st->vdesc[st->curdata][i].name[0] == '\0')
	    strcpy(st->vdesc[st->curdata][i].name, fieldnames[i]);
    }

    if(filename == NULL || !strcmp(filename, "-init")) {
	return 0;	// "kira -init" sets up datafield names w/o complaint
    }

    ifstream *ins = new ifstream(filename);
    if (!ins || !*ins) {
	msg("Kira/starlab data file %s not found.", filename);
	delete ins;
	return 0;
    }

    struct worldstuff *ww = NewN( worldstuff, 1 );	// from shmem -- avoid "new"
    ww->init( st );
    ww->maxnh = 1024;
    ww->wh = NewN( worldbundleptr, ww->maxnh );
    ww->ins = ins;
    ww->tmin = ww->tmax = 0;
    ww->tcur = ww->treq = -1.0;		/* invalidate */
    ww->readflags = readflags;

    memset( dd, 0, sizeof(*dd) );
    dd->data = ww;
    dd->getspecks = kira_get_parti;
    dd->trange = kira_get_trange;
    dd->ctlcmd = NULL; /*kira_parse_args;*/
    dd->draw = NULL; /*kira_draw;*/
    dd->help = kira_help;
    dd->free = NULL;			/* XXX create a 'free' function someday */
    dd->enabled = 1;

    ww->nh = 0;
    if(! (readflags & KIRA_READLATER)) {
	kira_read_more( dd, st, ww->maxnh, 1e38, 1e38 );


	if (ww->nh == 0) {
	    msg("Data file %s not in worldbundle format.", filename);
	    dd->enabled = 0;
	    return 0;
	}
	if(curstate.preload)
	    preload_pdyn(ww->wh, ww->nh, readflags & KIRA_VERBOSE != 0, true/*vel*/);
    }

    float mscale = mass_scale_factor();
    if(mscale > 0) {
	ww->massscale = mscale;
	ww->truemassscale = 1;
    }

    ww->ih = 0;
    ww->sl = NULL;

    return 1;
}

static float log10_of( float v ) {
    return v <= 0 ? 0 : log10f( v );
}

static float netTL( pdyn *b, float *totlum ) {	// temp-weighted lum
    if(b == NULL) return 0;
    if(b->is_leaf()) {
	*totlum += b->get_luminosity();
	return b->get_temperature() * b->get_luminosity();
    }
    float totTL = 0;
    for_all_daughters(pdyn, b, bb)
	totTL += netTL( bb, totlum );
    return totTL;
}

speck *add_speck(pdyn *b, pdyn *top, int addr, speck *sp, specklist *sl, worldstuff *ww)
{
    sp->p.x[0] = b->get_pos()[0];
    sp->p.x[1] = b->get_pos()[1];
    sp->p.x[2] = b->get_pos()[2];

    if(ww->centered) {
	sp->p.x[0] -= ww->center_pos[0];
	sp->p.x[1] -= ww->center_pos[1];
	sp->p.x[2] -= ww->center_pos[2];
    }

    int id = b->get_index();
    if(b->is_leaf()) {
	if(id < ww->nleafsel) {
	    ww->unionsel |= ww->leafsel[id];
	    if(SELECTED(ww->leafsel[id], &ww->intsrc))
		ww->interactsel = ww->intdest.wanton;
	}
	sp->val[SPECK_ID] = id;
	sp->val[SPECK_TLOG] = log10_of( b->get_temperature() );
	sp->val[SPECK_LUM] = b->get_luminosity();
    } else {
	id = -b->get_worldline_index();
	if(id >= 0 || id + ww->maxstars < ww->maxleaves) {
	    static int yikes = 1;
	    if(yikes++ % 256 == 0) printf("OOPS: worldline_index %d not in range 1..%d\n",
		    -id, 2*ww->maxleaves);
	} else {
	    int slotno = ww->maxstars + id;
	    ww->unionsel |= ww->leafsel[slotno];
	}
	sp->val[SPECK_ID] = id;		// distinguish CM nodes

	float totL = 0, totTL;
	totTL = netTL( b, &totL );
	sp->val[SPECK_LUM] = totL;
	sp->val[SPECK_TLOG] = log10_of( totL == 0 ? 0 : totTL / totL );
    }

    sp->val[SPECK_MASS] = b->get_mass() * ww->massscale;
    sp->val[SPECK_SQRTMASS] = sqrtf( sp->val[SPECK_MASS] );
    sp->val[SPECK_STYPE] = b->get_stellar_type();	/* see starlab/inc/star_support.h */

    sp->val[SPECK_ISMEMBER] = is_member( ww->wh[ww->ih], b );
    sp->val[SPECK_NCLUMP] = 0;		// complete (add n_leaves) in kira_to_parti

    sp->val[SPECK_ROOTID] = (top->is_leaf())
			? top->get_index()
			: -top->get_worldline_index();

    // Addr is 0 for top-level leaf, 1 for top-level CM,
    // constructed from parent addr otherwise.

    sp->val[SPECK_TREEADDR] = addr;

    sp->val[SPECK_RINGSIZE] = 0;

    sl->nspecks++;
    if(sl->nspecks < ww->maxstars) {
	sp = NextSpeck(sp, sl, 1);
    } else {
	msg("add_speck overflow! %d >= %d", sl->nspecks, ww->maxstars);
    }
    return sp;
}
    
speck *assign_specks(pdyn *b, pdyn *top, int addr, speck *sp, specklist *sl, worldstuff *ww)
{
    // Convert relative coordinates to absolute.
    vec oldpos = b->get_pos();

    if (b != top)
	b->inc_pos(b->get_parent()->get_pos());


    if(b->is_leaf()
		|| ww->treenodes == KIRA_ON
		|| (ww->treenodes == KIRA_ROOTS && b == top)) {
	sp = add_speck(b, top, addr, sp, sl, ww);

	vec vel = b->get_vel();
	sp->val[SPECK_SEPVEC] = vel[0];		// for leaf nodes,
	sp->val[SPECK_SEPVEC+1] = vel[1];	// use SEPVEC to hold velocity
	sp->val[SPECK_SEPVEC+2] = vel[2];

	if(b->is_leaf())
	    ww->leafcount++;
    }

    if(!b->is_leaf() &&
		(ww->treerings == KIRA_ON || ww->treearcs != KIRA_OFF
		 || (ww->treerings == KIRA_ROOTS && b == top))) {

	struct speck *marksp = ww->marksp;
	ww->marksp = add_speck(b, top, addr, marksp, ww->marksl, ww);

	// find our two children

	pdyn *b1 = b->get_oldest_daughter();
	pdyn *b2 = b1 ? b1->get_younger_sister() : NULL;
	if(b2) {
	    vec sep = b1->get_pos() - b2->get_pos();
	    real dist = sqrt(sep * sep);
	    real mass = b->get_mass();	// = sum of b1&b2 masses.

	    real size = 0;
	    switch(ww->ringsizer) {
	    case KIRA_RINGSEP:
		size = 0.5 * dist;
		break;
	    case KIRA_RINGA:
	      {
		vec vel = b1->get_vel() - b2->get_vel();
		real speed2 = vel * vel;
		real E = 0.5 * speed2 - mass / dist;
		size = -mass / (2*E);	// semimajor axis
	      }
	    }
	    marksp->val[SPECK_RINGSIZE] = size;
	    marksp->val[SPECK_MU] = b1->get_mass() / mass;
	    marksp->val[SPECK_SEPVEC] = sep[0];
	    marksp->val[SPECK_SEPVEC+1] = sep[1];
	    marksp->val[SPECK_SEPVEC+2] = sep[2];
	}
    }

    // Recursion.

    addr *= 2;
    for_all_daughters(pdyn, b, bb)
	sp = assign_specks(bb, top, addr++, sp, sl, ww);

    if(b != top)
	b->set_pos( oldpos );

    return sp;
}

struct specklist *kira_to_parti(pdyn *root, struct dyndata *dd, struct stuff *st, struct worldstuff *ww)
{

    if(ww == NULL || ww->nh == 0) return NULL;


    struct specklist *sl = ww->bufsl[ww->bufno];
    struct specklist *marksl = ww->bufmarksl[ww->bufno];

    if(sl == NULL) {
	// Use NewN to allocate these in (potentially) shared memory,
	// rather than new which will allocate in local malloc pool.
	// Needed for virdir/cave version.

	int w, wmax = 0;
	for(int k = 0; k < ww->nh; k++) {
	    if(ww->wh[k] && wmax < (w = ww->wh[k]->get_nw()))
		wmax = w;
	}
	int smax = root->n_leaves();
	ww->maxstars = smax + wmax + smax;	// leave a bit of room for growth
	ww->maxmarks = 1+smax;
	ww->maxleaves = smax;

        sl = NewN( struct specklist, 1 );
	memset(sl, 0, sizeof(*sl));
	sl->bytesperspeck = SMALLSPECKSIZE( SPECK_NDATAFIELDS );
	sl->specks = NewNSpeck(sl, ww->maxstars);
	sl->scaledby = 1;

	marksl = NewN( struct specklist, 1 );
	*marksl = *sl;
	marksl->specks = NewNSpeck(marksl, ww->maxmarks);
	marksl->special = MARKERS;

	sl->nsel = ww->maxstars;
	marksl->nsel = ww->maxmarks;
	if(ww->bufsl[1 - ww->bufno]) {
	    sl->sel = ww->bufsl[1-ww->bufno]->sel;
	    marksl->sel = ww->bufmarksl[1-ww->bufno]->sel;
	} else {
	    sl->sel = NewN( SelMask, ww->maxstars );
	    memset(sl->sel, 0, sl->nsel*sizeof(SelMask));
	    marksl->sel = NewN( SelMask, ww->maxmarks );
	    memset(marksl->sel, 0, marksl->nsel*sizeof(SelMask));
	}

	sl->next = marksl;
	marksl->next = NULL;

	if(ww->bufleafsel[1 - ww->bufno] != NULL) {
	    ww->bufleafsel[ww->bufno] = ww->bufleafsel[1 - ww->bufno];
	} else {		// Only one bufleafsel
	    ww->bufleafsel[ww->bufno] = NewN( SelMask, ww->maxstars );
	    ww->nleafsel = ww->maxstars;
	    memset(ww->bufleafsel[ww->bufno], 0, ww->maxstars*sizeof(SelMask));
	}

	ww->trails = NewN( trailhead, ww->maxstars );
	memset(ww->trails, 0, ww->maxstars*sizeof(trailhead));

	ww->bufsl[ww->bufno] = sl;
	ww->bufmarksl[ww->bufno] = marksl;

    }
    ww->sl = sl;
    ww->marksl = marksl;


    struct speck *sp = sl->specks;

    sl->nspecks = 0;
    sl->colorseq = sl->sizeseq = sl->threshseq = 0;
    marksl->colorseq = marksl->sizeseq = marksl->threshseq = 0;
    marksl->nspecks = 0;
    ww->marksp = marksl->specks;
    ww->leafcount = 0;
    ww->leafsel = ww->bufleafsel[ww->bufno];

    int i;
    struct vald *vd = ww->vd;
    for(i = 0, vd = ww->vd; i < SPECK_NDATAFIELDS; i++, vd++) {
	vd->min = 1e9;
	vd->max = -1e9;
	vd->sum = 0;
    }
    int ntotal = 0;

    if(ww->wastracking) ww->wastracking = -1;

    for_all_daughters(pdyn, root, b) {
	int ns = sl->nspecks;
	int mns = marksl->nspecks;
	int nl = ww->leafcount;
	speck *tsp = sp;
	speck *msp = ww->marksp;

	ww->interactsel = ww->unionsel = 0;
	sp = assign_specks(b, b, !b->is_leaf(), sp, sl, ww);

	int k;
	int nleaves = ww->leafcount - nl;
	int count = sl->nspecks - ns;

	ww->unionsel |= ww->interactsel;

	/* For all nodes in this subtree, ... */
	for(k = 0; k < count; k++) {
	    tsp->val[SPECK_NCLUMP] += nleaves;
	    int id = (int)tsp->val[SPECK_ID];
	    if(id < 0) {
		/* For CM nodes, negate nclump value. */
		tsp->val[SPECK_NCLUMP] = -tsp->val[SPECK_NCLUMP];
		/* Also, propagate all leaves' set membership to CM nodes */
		sl->sel[ntotal] = ww->unionsel;

	    } else if(ww->interactsel && id < ww->nleafsel) {
		/* For leaf nodes, if at least one star in this
		 * group is in the interaction set,
		 * then add all other group members to it.
		 */
		sl->sel[ntotal] = ww->leafsel[id] |= ww->interactsel;
		/* Making trails? */
	    }
	    for(i = 0, vd = ww->vd; i <= SPECK_STYPE; i++, vd++) {
		float v = tsp->val[i];
		if(vd->min > v) vd->min = v;
		if(vd->max < v) vd->max = v;
		vd->sum += v;
	    }
	    ntotal++;
	    tsp = NextSpeck(tsp, sl, 1);
	}

	/* For all marks (rings, etc.) in this subtree */
	count = marksl->nspecks - mns;
	for(k = 0; k < count; k++) {
	    msp->val[SPECK_NCLUMP] += nleaves;
	    if(msp->val[0] < 0)
		msp->val[SPECK_NCLUMP] = -msp->val[SPECK_NCLUMP]; // negate nclump for CM nodes
	    marksl->sel[mns+k] = ww->unionsel;
	    msp = NextSpeck(msp, sl, 1);
	}
    }

    if(ww->wastracking < 0) ww->wastracking = 0;	// Detach if tracked pcle not found now

    for(i = 0, vd = ww->vd; i < SPECK_NDATAFIELDS && i < MAXVAL; i++, vd++) {
	struct valdesc *tvd = &st->vdesc[ st->curdata ][ i ];

	tvd->nsamples = ntotal;
	if(vd->min > vd->max) {
	    vd->min = vd->max = 0;
	    tvd->nsamples = 0;
	}
	tvd->min = vd->min;
	tvd->max = vd->max;
	tvd->sum = vd->sum;
	tvd->mean = tvd->nsamples > 0 ? vd->sum / tvd->nsamples : 0;
    }

    ww->bufno = 1 - ww->bufno;
    return sl;
}

void kira_set_tree( struct dyndata *dd, struct stuff *st, int oldtree ) {
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    ww->oldtree = oldtree;
    ww->slvalid = 0;
}


struct specklist *kira_get_parti( struct dyndata *dd, struct stuff *st, double realtime )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;

    if (ww == NULL || ww->nh == 0)
	return NULL;

    ww->treq = realtime;

    if (realtime < ww->tmin) realtime = ww->tmin;
    if (realtime > ww->tmax) realtime = ww->tmax;

    if (ww->tcur == realtime && ww->slvalid)
	return ww->sl;

    int ih = ww->ih;
    int nh = ww->nh;

    if(ih < 0 || ih >= nh) ih = 0;
    worldbundle *wb = ww->wh[ih];
    for(; realtime > wb->get_t_max() && ih < nh-1; ih++, wb = ww->wh[ih])
	;
    for(; realtime < wb->get_t_min() && ih > 0; ih--, wb = ww->wh[ih])
	;

    /* Sanity check */
    if(realtime < wb->get_t_min() || realtime > wb->get_t_max()) {
	msg("ERROR: realtime %lg out of range %lg .. %lg of worldbundle %d of 0..%d",
		realtime, wb->get_t_min(), wb->get_t_max(),
		ih, nh-1);
	ww->slvalid = 0;
	return NULL;
    }

    if(ww->oldtree == 1) {
	pdyn *root = create_interpolated_tree(wb, realtime, 1);
	ww->center_pos = get_center_pos();	// but probably invalid here
	ww->center_vel = get_center_vel();
	ww->sl = kira_to_parti(root, dd, st, ww);
	rmtree(root);
    } else {
	pdyn *root = create_interpolated_tree2(wb, realtime, 1);
	ww->center_pos = get_center_pos();
	ww->center_vel = get_center_vel();
	if(curstate.verbose>=3)
	    msg("realtime %lg pre-n_daughters %d n_leaves %d root %x wb %x timerange %lg .. %lg ih %d of 0..%d",
		    realtime, root->n_daughters(), root->n_leaves(), root,
		    wb, wb->get_t_min(), wb->get_t_max(),
		    ih, nh-1);

	ww->sl = kira_to_parti(root, dd, st, ww);

	if(curstate.verbose>=2)
	    msg("realtime %lg n_daughters %d nspecks %d sl %x root %x",
		    realtime, root->n_daughters(), ww->sl->nspecks, ww->sl, root);

    }

    ww->ih = ih;
    ww->tcur = realtime;

    ww->slvalid = 1;
    return ww->sl;
}

static char *progname;

void msg( CONST char *fmt, ... ) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "%s: ", progname);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
  va_end(args);
}

int readTmap( char *fname );

int scanopt(char *opt, char *arg) {
    float *tp;
    char *ep;
    int k;
    switch(opt[0]) {
    case 'T':
	tp = &curstate.T0.m[0];
	k = sscanf(arg,
		"%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f",
	    	tp+0,tp+1,tp+2,tp+3,    tp+4,tp+5,tp+6,tp+7,
	    	tp+8,tp+9,tp+10,tp+11,  tp+12,tp+13,tp+14,tp+15);
	if(k == 1) {
	    memset(tp, 0, 16*sizeof(float));
	    tp[5] = tp[10] = tp[0];
	    tp[15] = 1;
	    curstate.has_T0 = (tp[0] != 1.0);
	} else if(k == 16) {
	    curstate.has_T0 = 1;
	} else {
	    msg("T: expected 16 numbers");
	    return 0;
	}
	return 1;

    case 't':
	if(opt[1] == 'y') { /* "type" */
	    curstate.type = atoi(arg);
	} else {	    /* "time" */
	    curstate.realtime = strtod( arg, &ep );
	    if(arg == ep) {
		msg("%s: expected time", opt);
		return 0;
	    }
	    curstate.dt = (*ep == '\0') ? 0 : strtod( ep+1, NULL );
	}
	return 1;

    case 'r':
	switch(arg[0]) {
	case 'x': case 'y': case 'z':  curstate.axis = arg[0]; break;
	default: msg("%s: expected <axis><degrees/sec>[@time0]", opt); return 0;
	}
	curstate.turnrate = strtod( arg+1, NULL );
	if((ep = strchr(arg, '@')) != NULL)
	    curstate.turntime0 = strtod(ep+1, NULL);
	return 1;

    case 'g':
	curstate.group = atoi(arg);
	return 1;

    case 'm':
	curstate.mag0 = atof(arg);
	return 1;

    case 'M':
	curstate.masslum = atof(arg);
	return 1;

    case 'c':
	curstate.colorscale = atof(arg);
	return 1;

    case 'C':
	readTmap( arg );
	return 1;

    case 'o':
	curstate.oldtree = atoi(arg);
	return 1;

    case 'p':
	curstate.port = atoi(arg);
	return 1;

    case 'v':
	curstate.verbose = atoi(arg);
	return 1;

    case 'i':
	curstate.use_stdin = 1;
	return 1;

    case 'n':
	curstate.idbase = atoi(arg);
	return 1;

    case 'P':
	curstate.preload = 1;
	return 1;

    case 'y':
	curstate.type = atoi(arg);
	return 1;

    default:
	msg("unknown option %c %s", opt[0], arg);
	return 0;
    }
}

#if !WORDS_BIGENDIAN
void starswap(db_star *st) {
  int i, *wp;
  static int one = 1;
  /* byte-swap x,y,z, dx,dy,dz, magnitude,radius,opacity fields (32-bit float),
   *		num (32-bit int),
   *		color (16-bit short).
   * group and type fields shouldn't need swapping,
   * assuming the compiler packs bytes into a word in increasing
   * address order.  Seems safe.
   */
  if(*(char *)&one == 1) {
    for(i = 0, wp = (int *)st; i < 10; i++)
	wp[i] = htonl(wp[i]);
    st->color = htons(st->color);
  }
}
#else
#define  starswap(x)  /*nothing*/
#endif /*!WORDS_BIGENDIAN*/

static struct logTmap {
	float logT;
	unsigned short rgb565;
} defaultTmap[] = {
//  Adapted from Mitchell Charity's web page on black body RGB colors,
//   http://www.vendian.org/mncharity/dir3/blackbody/
  3.000, 0xf9c0,  3.079, 0xfa80,  3.204, 0xfb80,  3.301, 0xfc42,
  3.380, 0xfce7,  3.477, 0xfdad,  3.556, 0xfe31,  3.643, 0xfed6,
  3.716, 0xff5a,  3.792, 0xffbe,  3.869, 0xef7f,  3.944, 0xdf1f,
  4.017, 0xcedf,  4.093, 0xbe9f,  4.164, 0xb67f,  4.236, 0xae5f,
  4.310, 0xae3f,  4.380, 0xa61f,  4.450, 0xa5ff,
};

struct logTmap *myTmap = defaultTmap;
int nmyTmap = COUNT(defaultTmap);

static unsigned short logT2rgb565(float logT) {
    int i;
    for(i = 0; i < nmyTmap-1 && logT > myTmap[i].logT; i++)
	;
    return myTmap[i].rgb565;
}

int readTmap( char *fname ) {
    FILE *inf;
    char line[80];
#define MAXTMAP 300
    struct logTmap tm[MAXTMAP];
    int ntmap;
    char *cp;

    for(cp = fname; *cp; cp++) {
	if(*cp == '&' || isspace(*cp)) {
	    *cp = '\0';
	    break;
	}
    }
    inf = fopen(fname, "r");
    if(inf == NULL) {
	fprintf(stderr, "Can't open temp-to-rgb565-color map %s\n", fname);
	return 0;
    }
    ntmap = 0;
    while(fgets(line, sizeof(line), inf) != NULL) {
	static char delim[] = " \t\r\n";
	cp = line;
	while(isspace(*cp)) cp++;
	if(*cp == '\0' || *cp == '#') continue;
	if(ntmap >= MAXTMAP) {
	    fprintf(stderr,
		    "%s: only room for %d temp-to-rgb565-color entries, raise MAXTMAP\n",
		    fname);
	    goto fail;
	}
	if(sscanf(cp, "%f 0x%x", &tm[ntmap].logT, &tm[ntmap].rgb565) < 2
	    && sscanf(cp, "%f %x", &tm[ntmap].logT, &tm[ntmap].rgb565) < 2) {
	        fprintf(stderr, "%s: expected <logT> 0x<hex-rgb565-color> not %s",
			fname, line);
		goto fail;
	}
	ntmap++;
    }
    if(ntmap > 0) {
	nmyTmap = ntmap;
	myTmap = NewN( struct logTmap, ntmap );
	memcpy( myTmap, tm, ntmap*sizeof(struct logTmap) );
    }
    fclose(inf);
    return 1;

  fail:
    fclose(inf);
    return 0;
}

static double sx=0, sy=0, sz=0, smass=0;
static int nspecks;
static int has_tfm;
static Point pmin, pmax;
static Matrix pT;

static void report( FILE *outf, struct dyndata *dyn, struct stuff *st )
{
    double tmin, tmax;
    kira_get_trange( &st->dyn, st, &tmin, &tmax );
    fprintf(outf, "time %lg  in  %lg .. %lg\n",
	curstate.realtime, tmin, tmax);
    fprintf(outf, "nspecks %d\n", nspecks);
    if(curstate.idbase != 0)
	fprintf(outf, "idbase %d\n", curstate.idbase);
    if(smass > 0) {
	fprintf(outf, "CM %lg %lg %lg\n", sx/smass, sy/smass, sz/smass);
	fprintf(outf, "bbox center %g %g %g  radius %g %g %g\n",
	    .5*(pmax.x[0]+pmin.x[0]), .5*(pmax.x[1]+pmin.x[1]), .5*(pmax.x[2]+pmin.x[2]),
	    .5*(pmax.x[0]-pmin.x[0]), .5*(pmax.x[1]-pmin.x[1]), .5*(pmax.x[2]-pmin.x[2]));
	Point cen;
	kira_get_center( &cen, dyn, st );
	fprintf(outf, "Center %g %g %g", cen.x[0],cen.x[1],cen.x[2]);
	if(has_tfm) {
	    Point wcen;
	    vtfmpoint( &wcen, &cen, &pT );
	    fprintf(outf, " -> %g %g %g", wcen.x[0],wcen.x[1],wcen.x[2]);
	}
	fprintf(outf, "\n");
    }
    fprintf(outf, "transformation: out = in");
    if(curstate.turnrate != 0)
	fprintf(outf, " * %cRotation(%g*(time-%g))",
		curstate.axis, curstate.turnrate, curstate.turntime0);
    if(curstate.has_T0) {
	float *fp = curstate.T0.m;
	fprintf(outf, " * [%g %g %g %g  %g %g %g %g  %g %g %g %g  %g %g %g %g]",
		fp[0],fp[1],fp[2],fp[3],
		fp[4],fp[5],fp[6],fp[7],
		fp[8],fp[9],fp[10],fp[11],
		fp[12],fp[13],fp[14],fp[15]);
    }
    else if(curstate.turnrate == 0)
	fprintf(outf, " (identity transform)");
    fprintf(outf, "\n");
    fprintf(outf, "group %d  type %d\n", curstate.group, curstate.type);
}

float specklum( struct speck *sp ) {
    return (curstate.masslum != 0) ? sp->val[SPECK_MASS]*sp->val[SPECK_MASS]*sp->val[SPECK_MASS]*curstate.masslum :
				     sp->val[SPECK_LUM];
}

int serveonce(char *req, FILE *outf)
{
    int as_sdb = 0, as_speck = 0;
    Point p;
    db_star star;
    char *cp;
    struct stuff *st = curstate.st;

    nspecks=0;
    sx = sy = sz = smass = 0;

    pmin.x[0]=pmin.x[1]=pmin.x[2] = 1e20;
    pmax.x[0]=pmax.x[1]=pmax.x[2] = -1e20;

    if(curstate.verbose)
	fprintf(stdout, "REQ: %s\n", req);

    if(outf == NULL || req == NULL) {
	msg("kira server: get lost!");
	if(outf) fclose(outf);
	return 0;
    }

    if(strstr(req, "sdb"))  as_sdb = 1;
    else if(strstr(req, "speck")) as_speck = 1;

    char *eqp;
    for(eqp = req; (eqp = strchr(eqp, '=')) != NULL; ) {
	char *optp, *argp;
	for(optp = eqp; isalpha(optp[-1]); optp--)
	    ;
	argp = eqp+1;
	cp = strpbrk(argp, ";&");
	if(cp) {
	    eqp = cp+1;
	    *cp = '\0';
	}
	scanopt( optp, argp );
	if(cp == NULL) break;
    }

    memset(&star, 0, sizeof(star));

    star.group = curstate.group;
    star.type = curstate.type;

    struct specklist *sl;
    sl = kira_get_parti( &st->dyn, st, curstate.realtime );

    if(sl == NULL)
	return 0;

    has_tfm = curstate.has_T0 || curstate.turnrate != 0;
    if(curstate.turnrate != 0) {
	Matrix Trot;
	mrotation( &Trot, curstate.turnrate * (curstate.realtime - curstate.turntime0), curstate.axis );
	if(curstate.has_T0) {
	    mmmul( &pT, &Trot, &curstate.T0 );
	} else {
	    pT = Trot;
	}
    } else {
	pT = curstate.T0;
    }

    nspecks = 0;
    for(int i = 0; i < sl->nspecks; i++) {
	struct speck *sp = NextSpeck(sl->specks, sl, i);
	int wrote = 0;
	if(sp->val[SPECK_NCLUMP] < 0)
	    continue;
	nspecks++;
	Point vel, tvel;
	vel.x[0] = sp->val[SPECK_SEPVEC] * curstate.dt;
	vel.x[1] = sp->val[SPECK_SEPVEC+1] * curstate.dt;
	vel.x[2] = sp->val[SPECK_SEPVEC+2] * curstate.dt;
	if(has_tfm) {
	    vtfmpoint( &p, &sp->p, &pT );
	    vtfmpoint( &tvel, &vel, &pT );
	    vel = tvel;
	} else {
	    p = sp->p;
	}
	if(as_sdb) {
	    star.x = p.x[0];
	    star.y = p.x[1];
	    star.z = p.x[2];
	    star.dx = vel.x[0];			// Use velocity,
	    star.dy = vel.x[1];			// held in SEPVEC
	    star.dz = vel.x[2];			// for leaf nodes.
	    star.magnitude = curstate.mag0 - .921 * logf( specklum(sp) );
	    if(curstate.colorscale >= 0) {
		star.color = (unsigned short) (curstate.colorscale * sp->val[SPECK_TLOG]);
	    } else {
		/* packed rgb565 */
		star.color = logT2rgb565( sp->val[SPECK_TLOG] );
	    }
	    star.num = (int) sp->val[SPECK_ID] + curstate.idbase;
	    starswap(&star);
	    wrote = fwrite(&star, sizeof(star), 1, outf);
	} else if(as_speck) {
	    wrote = fprintf(outf, "%g %g %g %g %g %g\n",
		p.x[0],p.x[1],p.x[2], 
		specklum(sp), sp->val[SPECK_TLOG], sp->val[SPECK_ID] + curstate.idbase);
	} else {
	    double m = sp->val[SPECK_MASS];
	    sx += p.x[0]*m;
	    sy += p.x[1]*m;
	    sz += p.x[2]*m;
	    smass += m;
	    if(pmin.x[0] > p.x[0]) pmin.x[0] = p.x[0];
	    if(pmin.x[1] > p.x[1]) pmin.x[1] = p.x[1];
	    if(pmin.x[2] > p.x[2]) pmin.x[2] = p.x[2];
	    if(pmax.x[0] < p.x[0]) pmax.x[0] = p.x[0];
	    if(pmax.x[1] < p.x[1]) pmax.x[1] = p.x[1];
	    if(pmax.x[2] < p.x[2]) pmax.x[2] = p.x[2];
	}
	if(wrote < 0)			/* SIGPIPE? Anyway, quit writing. */
	    break;
    }
    if(!as_sdb && !as_speck) {
	report( outf, &st->dyn, st );
    }
    if(curstate.verbose) {
	char *cp = strchr(req, '\n');
	if(cp) *cp = '\0';
	fprintf(stdout, "REQ: %s\n", req);
	report( stdout, &st->dyn, st );
	fflush(stdout);
    }
    return nspecks;
}

int serverlisten( int port ) {
    struct sockaddr_in asin;
    static int one = 1;
    int lsock;

    memset(&asin, 0, sizeof(asin));
    asin.sin_family = AF_INET;
    asin.sin_port = htons(port);
    asin.sin_addr.s_addr = INADDR_ANY;

    lsock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if(lsock < 0) {
	perror("socket");
	return -1;
    }
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(bind(lsock, (struct sockaddr *)&asin, sizeof(asin)) < 0) {
	perror("bind");
	return -1;
    }

    if(listen(lsock, 90) < 0) {
	perror("listen");
	return -1;
    }
    return lsock;
}

void serverloop( int lsock ) {
    for(;;) {
	struct sockaddr_in from;
	FILE *inf, *outf;
	char req[1280];
	
#if sgi
	int fromlen = sizeof(from);
#else
	unsigned int fromlen = sizeof(from);
#endif
	int s = accept(lsock, (struct sockaddr *)&from, &fromlen);
	if(s < 0) {
	    if(errno == EINTR)
		continue;
	    perror("accept");
	    return;
	}
	inf = fdopen(s, "r");
	if(fgets(req, sizeof(req), inf) == NULL) {
	    perror("fgets");
	} else {
	    outf = fdopen(s, "w");
	    do {
		serveonce( req, outf );
		fflush(outf);
	    } while(fgets(req, sizeof(req), inf) != NULL);
	    fclose(outf);
	}
	fclose(inf);
	close(s);
    }
}


main(int argc, char *argv[])
{
    static struct stuff tst;
    curstate.st = &tst;
    int c;
    char opt[8];

    progname = argv[0];

    curstate.colorscale = -1;
    curstate.has_T0 = 0;
    curstate.port = 3400;

    while((c = getopt(argc, argv, "T:r:t:y:g:m:c:o:p:v:in:PC:")) != EOF) {
	opt[0] = c;
	opt[1] = '\0';
	if(c == 'y') strcpy(opt, "type");
	scanopt( opt, optarg );
    }
    if(optind != argc-1) {
	fprintf(stderr, "Usage: %s [options] file.kira\n\
Options:\n\
   -t time\n\
   -r <axis><degreespertime>[@time0]  Timed rotation about axis 'x'/'y'/'z'\n\
   -T 16-numbers   outcoords = kira * rotation(time) * tfm\n\
   -y sdbtype\n\
   -g group\n\
   -m mag0         sdb mag = mag0 - log(lum)/(log(100)/5)\n\
   -M masslum	   if nonzero, ignore bogus lum; use mass**3 * masslum instead.\n\
   -c colorscale   sdb color = log10(T)*colorscale; -c -1 => RGB565 (default)\n\
   -o treestyle(1 vs 2) (default -o 2 for create_interpolated_tree2)\n\
   -p portno	   listen for HTTP connections on that TCP port\n\
   -v verbose(0 vs 1) server logs requests to its own stdout\n\
   -i		   read from stdin, write to stdout (else be a network server)\n\
   -n numbase	   add numbase to particle ids\n\
   -P		   call preload_pdyn()\n\
   -C mapfile	   use that colormap; each line has: log-temperature rgb565-color\n\
", progname);
	exit(1);
    }

    /* Ignore SIGPIPE signals -- don't crash if a caller disconnects. */
    signal(SIGPIPE, SIG_IGN);

    if(curstate.use_stdin) {
	msg("Reading %s", argv[optind]);
	kira_open( &curstate.st->dyn, curstate.st, argv[optind], curstate.verbose ? KIRA_VERBOSE : 0 );
	int eofs = 0;
	char req[512];
	fprintf(stderr, "Type  ?  for help\n");
	for(;;) {
	    fprintf(stderr, "\n>> ");
	    if(fgets(req, sizeof(req), stdin) == NULL) {
		if(++eofs >= 3) break;	/* quit if 3 successive EOFs */
		continue;
	    } else {
		eofs = 0;
	    }

	    if(req[0] == 'q')
		break;

	    if(req[0] == '?' || req[0] == 'h') {
		printf("Usage:  each line of input yields one starlab lookup,\n\
after applying all options given on that line.  Unchanged settings persist.\n\
Options take the form <keyletter>=<value> and may be separated by \"&\" or \";\"\n\
  t=<time>   simulation time\n\
  o=<treestyle>  o=1 => create_interpolated_tree, o=2 => ...tree2.  Default o=2.\n\
  T=<16-numbers> apply 4x4 transformation\n\
  speck	     dump ASCII specks (default just summary information)\n\
  q          quit\n\
");
	    } else {
		serveonce( req, stdout );
	    }
	}
	exit(1);

    } else {
	/* be a network server */

	msg("Listening on port %d", curstate.port);
	int lsock = serverlisten( curstate.port );
	if(lsock < 0)
	    exit(1);

	msg("Reading %s", argv[optind]);
	kira_open( &curstate.st->dyn, curstate.st, argv[optind], curstate.verbose ? KIRA_VERBOSE : 0 );

	msg("Ready.");
	serverloop( lsock );
    }
}
