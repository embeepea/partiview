#define NEWSTDIO

/*
 * Interface to Kira Starlab (www.manybody.org) library for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

/* 
 * $Log: kira_parti.cc,v $
 * Revision 1.54  2006/01/21 05:14:32  slevy
 * Remove some debugging code.
 * Make kira_open read plain files again.
 *
 * Revision 1.53  2006/01/20 19:22:37  slevy
 * Add TCPsocket hooks.  (They don't work yet.)  Includes debugging code to prove
 * that they don't work.  Use an istream rather than ifstream -- we should
 * be able to create an istream from a socket, and that's all the starlab libs need.
 *
 * Revision 1.52  2006/01/17 07:39:50  slevy
 * Lots of changes.  Add threading.
 *
 * Revision 1.51  2004/04/19 21:04:12  slevy
 * Un-CONST-ify most of what had been constified.  It's just too messy.
 *
 * Revision 1.50  2004/04/19 17:42:41  slevy
 * Use AC_FUNC_ALLOCA and the autoconf-standard alloca boilerplate
 * in each of the files that uses alloca.
 *
 * Revision 1.49  2003/07/29 21:15:37  steve
 * Changed Starlab vectors to vec.  Defined NEWSTDIO to allow compilatin
 * under gcc 3.2 (not sure if 2.9x is broken...).
 *
 * Revision 1.48  2003/02/15 06:57:45  slevy
 * Initialize new <dyndata>.slvalid field.
 *
 * Revision 1.47  2002/06/18 21:25:25  slevy
 * Add reference to LICENSE.partiview file.
 * Where appropriate, update references to geomview license too,
 * now called COPYING.geomview.
 *
 * Revision 1.46  2002/06/12 23:30:26  slevy
 * Need winjunk.h here too.
 * Scrap unix-specific debug code.
 *
 * Revision 1.45  2002/05/06 19:44:34  slevy
 * Avoid Fl:: references if -DUSE_PLOT=0.
 *
 * Revision 1.44  2002/04/17 20:47:58  slevy
 * Add a not-quite-proprietary notice to all source files.
 * Once we pick a license this might change, but
 * in the mean time, at least the NCSA UIUC origin is noted.
 *
 * Revision 1.43  2002/03/11 22:28:21  slevy
 * Allow -DUSE_PLOT=0 to disable FLTK H-R plot widget.
 *
 * Revision 1.42  2002/01/27 00:37:48  slevy
 * No need to bogusly import getfloats() any more.
 *
 * Revision 1.41  2001/12/28 07:18:36  slevy
 * Convert to new plugin style.
 *
 * Revision 1.40  2001/08/29 20:58:14  slevy
 * Try (?) to handle selection (leafsel/bufsl->sel) properly -- it seems
 * to interact with double-buffering.
 * Use preload_pdyn() to precompute trees for each worldbundle.
 *
 * Revision 1.39  2001/08/28 18:42:07  slevy
 * Yeow, refer to the proper ww->bufleafsel[]!
 *
 * Revision 1.38  2001/08/28 18:28:18  slevy
 * Switch (I hope) to double-buffered graphics data, flagged by ww->bufno.
 *
 * Revision 1.37  2001/08/26 17:36:46  slevy
 * Parenthesize ambiguous expressions...
 *
 * Revision 1.36  2001/08/26 02:11:07  slevy
 * Oops, toss extra glBegin(GL_POINTS).
 *
 * Revision 1.35  2001/08/21 03:11:05  slevy
 * Use Steve's new family of center-points -- "kira center" command.
 * Add new per-particle field, "ismember", to show escapees.
 *
 * Revision 1.34  2001/07/19 20:12:47  slevy
 * Reprocess particles after changing parameters.
 * (Some "kira" commands use kira_set(), which already does this.
 * Just set "changed" for those which don't.)
 * Impose a max trail gap (delta time).  Break up trails at time-gaps
 * exceeding this threshold.
 *
 * Revision 1.33  2001/07/18 22:41:45  slevy
 * Add "kira center" command, to report all positions relative to
 * the kira-computed smoothed cluster center.
 *
 * Revision 1.32  2001/07/18 19:45:34  slevy
 * Picked stars now report their type and spectral/luminosity classes too.
 * New "kira hrdiag" command allows turning it on/off, setting plot ranges.
 * "kira int x" is a synonym for "kira int x=x", the common case.
 * New "sqrtmass" attribute.  "mass" is again in linear, not log units.
 * Disable kira display if we don't get a valid input file.
 * XXX Bogusly import getfloats() from partibrains.c; should go in a separate
 * utility .c/.h file.
 *
 * Revision 1.31 2001/07/17 22:19:37  slevy
 * Use type_string(stellar_type) to get readable names for stellar types
 * of picked stars.
 * 
 * Revision 1.30 2001/07/17 17:29:36  slevy
 * Use new mass_scale_factor() to get starlab's idea of
 * dynamical->solar mass unit conversion.
 * If it knows, then "kira massscale" only allows user
 * to override if user-supplied value ends with "!",
 * e.g. "kira mscale 600!".
 * 
 * Revision 1.29 2001/07/16 19:08:30  slevy
 * #if(def)'s for CAVE, where we have no plot widget.
 * 
 * Revision 1.28 2001/07/16 17:55:55  slevy
 * Report log10(mass*massscale) -- log is handier than linear units.
 * 
 * Revision 1.27 2001/07/12 21:37:02  slevy
 * Lots of changes.  Add trails.  Move tracking code so we'll be
 * able to track collections rather than single particles (not yet though).
 * Trails are relative to tracked-point position, so can be non-inertial.
 * 
 * Revision 1.26 2001/07/10 17:15:50  slevy
 * Add massscale (mscale) for convenient mass presentation.
 * Compute fake aggregate temp/luminosity for CM nodes.
 * Oops, marks need their own sel[] array!
 * Allow adjusting H-R diagram alpha values too.
 * 
 * Revision 1.25 2001/07/09 19:57:22  slevy
 * Add H-R diagram zoom (b/B keys).
 * Pay attention to emph/see settings.
 * Get selection working properly.
 * 
 * Revision 1.24 2001/07/07 15:27:57  slevy
 * Save/restore positions of subparticles, so we can reuse interpolated_tree safely.
 * Only use SEL_DEST-initialized pick etc. info.
 * 
 * Revision 1.23 2001/07/04 03:28:03  slevy
 * Add picking support: kira_picked() gets all pick callbacks.
 * Add set-selection features, including interaction tracing.
 * 
 * Revision 1.22 2001/07/02 16:17:24  slevy
 * Cast kira_HRplot to avoid complaints.
 * Hack for gcc 3.0's stdio (#ifdef NEWSTDIO).
 * 
 * Revision 1.21 2001/07/01 14:30:43  slevy
 * Do more plot setup here in kira_parti.  Set axis titles too.
 * 
 * Revision 1.20 2001/07/01 07:31:45  slevy
 * Hmm, make default logT range a bit smaller.
 * 
 * Revision 1.19 2001/07/01 06:42:01  slevy
 * Oops, get the sense of the log-T scale correct!
 * 
 * Revision 1.18 2001/06/30 18:09:15  slevy
 * Add kira_HRplot for drawing in H-R diagram.
 * 
 * Revision 1.17 2001/06/30 07:01:47  slevy
 * Yes, call create_interpolated_tree2() if not OLDTREE!
 * 
 * Revision 1.16 2001/06/06 17:22:55  slevy
 * get_stellar_type() now returns int (enum from starlab/inc/star_support.h),
 * rather than a string.
 * 
 * Revision 1.15 2001/05/30 14:31:16  slevy
 * Add Tlog, Lum, and stellar-type fields.
 * Star type is number-encoded, sigh:
 * 	0 = unknown
 * 	1 = ms	Main Sequence
 * 	2 = cd	Compact Dwarf??
 * 	3 = gs	??
 * 	4 = bh	Black Hole
 * 	5 = sg	Supergiant?
 * 	6 = hb	Horizontal Branch
 * 
 * Revision 1.14 2001/05/15 12:18:57  slevy
 * New interface to dynamic-data routines.
 * Now, the only #ifdef KIRA/WARP needed in partibrains.c are
 * the data-command initialization routines.  All others,
 * including control-command parsing and specialized drawing,
 * is now via a function table.
 * 
 * Revision 1.13 2001/05/14 15:47:59  slevy
 * Invalidate color/size/threshold sequence numbers so main code will recalc.
 * 
 * Revision 1.12 2001/05/14 08:40:06  slevy
 * kira_draw(): don't do anything if the dynamic data isn't ours.
 * 
 * Revision 1.11 2001/05/12 07:23:05  slevy
 * New generic get-time-range function for dynamic data.
 * 
 * Revision 1.10 2001/04/04 19:23:05  slevy
 * Use portable form of is_open().  Other tidiness.
 * 
 * Revision 1.9 2001/03/30 13:56:07  slevy
 * Rename functions to kira_*.
 * Add framework for incrementally reading paragraphs of input:
 * kira_open(), kira_read_more().
 * Add scraps for maintaining trails, but not enough to work yet.
 * 
 * Revision 1.8 2001/02/04 16:52:38  slevy
 * Use default speck-field names if user doesn't override them
 * (either before or after "kira" reader).
 * 
 * Revision 1.7 2001/02/03 23:37:51  slevy
 * 
 * Oops, now kira_draw needs to ensure we really have something to draw!
 * 
 * Revision 1.6 2001/02/03 15:26:16  slevy
 * 
 * OK, toss those extra 'break's.
 * 
 * Revision 1.5 2001/02/03 15:20:34  slevy
 * Add "kira tree" support -- tree arcs, with crosswise tick-marks
 * at center-of-mass points.  Tickmarks lie in screen plane, with
 * length of <tickscale> times true separation.
 * 
 * Revision 1.4 2000/12/31 00:25:05  slevy
 * Re-extract whenever kira_set changes something.
 * Add particle tracking.
 * Compute ring sizes correctly, and draw rounder rings.
 * 
 * Revision 1.3 2000/12/30 02:19:30  slevy
 * Add ring markers for non-isolated stars.  Not finished yet, but seems usable.
 * No GUI yet, but new commands:
 *    kiractl nodes  {on|off|roots}
 *    kiractl rings  {on|off|roots}
 *    kiractl ringsize {sep|semi}   (ring size from curr. separation or semimajor axis)
 *    kiractl scale  <scalefactor>
 *    kiractl span   <minpixels> <maxpixels>   (allowed range of ring radii)
 * 
 * Change encoding of nclump: n for leaves, -n for center-of-mass nodes,
 *    so colormap can make them distinguishable.
 * 
 * Bug: after kiractl command, must change timestep before it takes effect.
 * Bug: ring markers aren't colored yet for some reason.
 * 
 * Revision 1.2 2000/12/22 01:11:09  slevy
 * Recode to be speedier (use recursion) and to provide more information to partiview:
 * 	[0] worldline index (= kira index for singles, - and unique for others)
 * 	[1] mass
 * 	[2] number of stars (leaf nodes) in clump
 * 		+ 100 if this is a member of an unperturbed binary
 * 	[3] top-level name (small integer, = worldline index for single stars)
 * 	[4] tree address in clump (0=single, 1=root, children of <i> are <2i>,<2i+1>)
 * 
 * Revision 1.1 2000/12/20 16:50:54  slevy
 * Glue code to read starlab (kira) files and interpolate particle positions
 * into partiview internal form.
 */

#ifdef USE_KIRA

#ifdef _WIN32
# include "winjunk.h"
#endif

#include "tcpsocket.h"
#include "threader.h"

#include <iosfwd>
#include <fstream>
#include <iostream>

#include "worldline.h"

#include "shmem.h"
#include "specks.h"
#include "partiviewc.h"
#include "findfile.h"

#include "kira_parti.h"		// declare following as extern "C"

#include <unistd.h>

#if !defined(CAVE) && !defined(USE_PLOT)
# define USE_PLOT 1
#endif

#if USE_PLOT
# include "Plot.H"
#endif

#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#undef isdigit		/* irix 6.5 back-compat hack */

static char local_id[] = "$Id: kira_parti.cc,v 1.54 2006/01/21 05:14:32 slevy Exp $";

extern struct specklist *kira_get_parti( struct stuff *st, double realtime );
extern int kira_parse_args( struct dyndata *dd, struct stuff *st, int argc, char **argv );
extern int kira_draw( struct dyndata *dd, struct stuff *st, struct specklist *slhead,
		Matrix *Tc2w, float radperpix );
extern int kira_open( struct dyndata *dd, struct stuff *st, istream *ins, const char *filename, int readflags );
extern void kira_HRplot( Fl_Plot *, struct stuff *st, struct dyndata *dd );
extern int kira_HRplot_events( Fl_Plot *plot, int ev );
extern int kira_picked( struct stuff *st, GLuint *hit, struct specklist *sl, int speckno );
extern void kira_maxtrail( struct dyndata *dd, struct stuff *st, int newmax );



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

struct kirastuff {
  public:
    Threader<kirastuff> thr;

    volatile int nh, maxnh; 
    worldbundle * volatile * volatile wh;

  private:
    real tmin_, tmax_;
    std::istream *ins_;		// source stream
    TCPsocket *ts_;
    int readflags_;

  public:

    kirastuff() {
	init();
    }

    pthread_mutex_t & mutex() { return thr.mutex(); }

    void init() {
	nh = 0;
	maxnh = 128;
	wh = NewN( worldbundleptr, maxnh );
	tmin_ = tmax_ = 0;
	ins_ = NULL;
	ts_ = NULL;
	readflags_ = 0;

	stopnh = 1<<30;
	stoptmax = 1e100;
	stoprealdt = 1e100;
    }

    // reader stop conditions
    int stopnh;
    double stoptmax;
    double stoprealdt;

    void readflags( int flags ) { readflags_ = flags; }

    void start() {
	thr.start( this, &kirastuff::kira_read_some );
    }

    void start( istream *ins ) {
	ins_ = ins;
	start();
    }

    void start( TCPsocket *ts ) {
	ts_ = ts;
	start();
    }

    // Methods which expect to be called with mutex locked:

    int wmax() {
	int w, wmax = 0;
	for(int k = 0; k < nh; k++) {
	    if(wh[k]!=NULL && wmax < (w = wh[k]->get_nw()))
		wmax = w;
	}
	return wmax;
    }

    void makeroomfor( int i ) {
	if(i >= maxnh) {
	    maxnh = (i > maxnh*2) ? i+5 : maxnh*2;
	    worldbundle * volatile * volatile newwh = new worldbundle * volatile[maxnh];
	    for(int k = 0; k < nh; k++)
		newwh[k] = wh[k];
	    delete [] wh;
	    wh = newwh;
	}
    }

    void trange( double *tminp, double *tmaxp ) {
	if(tminp) *tminp = tmin_;
	if(tmaxp) *tmaxp = tmax_;
    }

    void *kira_read_some();

    void kira_read_some( std::istream *ins ) {
	ins_ = ins;
	kira_read_some();
    }

};

struct worldstuff {
    struct stuff *st;
    struct dyndata *dd;

    kirastuff ks;		// if we do asynchronous reading, shared data lives here

    int ih;			// current displayed worldbundle index
    worldbundleptr curwb;	// current worldbundle chunk (owned by display thread)

    real tmin, tmax;
    real tcur;			// current time
    real treq;			// requested time
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

#if USE_PLOT
    struct Fl_Plot *plot;
#endif

    worldstuff( struct stuff *inst ) {
	this->init( inst );
    }

    void init( struct stuff *inst ) {
	ih = 0;
	tmin = 0, tmax = 1;
	tcur = treq = 0;
	st = inst;
	dd = NULL;

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
	centered = 1;
	which_center = 0;
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
	parse_selexpr( inst, "pick", &picksel, NULL, NULL );
	selinit( &trailsel );
	selinit( &intsrc );
	selinit( &intdest );
    }
};

extern void kira_add_trail( struct stuff *st, struct worldstuff *, int id, struct speck * );
extern void kira_erase_trail( struct stuff *st, struct worldstuff *, int id );
extern void kira_track_break( struct worldstuff *ww, Point *newtrack );

void kira_invalidate( struct dyndata *dd, struct stuff *st ) {
    if(st->sl) st->sl->used = -1000000;
    worldstuff *ww = (worldstuff *)dd->data;
    if(ww) ww->slvalid = 0;
}

int kira_read( struct stuff **stp, int argc, char *argv[], char *fname, void * ) {

    if(argc < 1 || strcmp(argv[0], "kira") != 0)
	return 0;

    struct stuff *st = *stp;
    st->clk->continuous = 1;
    char *realfile = findfile(fname, argv[1]);
    kira_open( &st->dyn, st, NULL, realfile ? realfile : argv[1], argc>2 ? atoi(argv[2]) : 0 );
    return 1;
}

void *kirastuff::kira_read_some()
{
    fprintf(stderr, "kira_read_some(): ins %p ts %p threading %d\n", ins_,ts_, thr.threading());
    if(ins_ == NULL && ts_ != NULL && ts_->valid()) {
	if(thr.threading())	// if we're running in a separate thread, be synchronous
	    ts_->nonblock( false );
	ins_ = ts_->new_istream();
	if(ins_ == 0) {
	    fprintf(stderr, "kirastuff::kira_read_some(): Rats.\n");
	    return 0;
	}
    }

    if(ins_ == NULL) {
	fprintf(stderr, "kira_read_some: neither stream nor socket?\n");
	return 0;
    }

    int paras = 0;
    int timelimit = (stoprealdt < 1e6);
    double wallstop = stoprealdt + wallclock_time();
    while( (nh == 0 || wh[nh-1]->get_t_max() < stoptmax) ) {

	if(timelimit && wallstop < wallclock_time())
	    break;

	worldbundle *wb;

	thr.testcancel();
fprintf(stderr, "kira_read_some: pre-read_bundle\n");
	wb = read_bundle(*ins_, readflags_ & KIRA_VERBOSE);
fprintf(stderr, "kira_read_some: read_bundle() -> %p\n", wb);
	thr.testcancel();

	if(wb == NULL) {
	    if(paras == 0) paras = -1;
	    break;
	}

	{
	    scoped_lock lk( thr.mutex() );
	    makeroomfor( nh );
	    wh[nh++] = wb;
	    paras++;
	}
    }
fprintf(stderr, "kira_read_some: %d paras, nh %d\n", paras, nh);

    if(paras > 0) {
	tmin_ = wh[0]->get_t_min();
	tmax_ = wh[nh-1]->get_t_max();
    }
    return (void *)&nh;
}

#if 0
int kira_read_more( struct dyndata *dd, struct stuff *st, int nmax, double tmax, double realdtmax )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    if(ww == NULL) return -1;
    istream *ins = ww->ins;
    if(ins == NULL || !ins->rdbuf()->is_open()) return -1;

    int paras = 0;
    int timelimit = (realdtmax < 1e6);
    double wallstop = realdtmax + wallclock_time();
    while( paras < nmax && (ww->nh == 0 || ww->wh[ww->nh-1]->get_t_max() < tmax) ) {

	if(timelimit) {
	    double dt = wallstop - wallclock_time();
	    if(!hasdata(ins, dt))
		break;
	    if(dt <= 0)
		break;
	}

	if(ww->nh >= ww->maxnh) {
	    ww->lock();
	    ww->maxnh = (ww->nh > ww->maxnh*2) ? ww->nh + 1 : ww->maxnh*2;
	    ww->wh = RenewN( ww->wh, worldbundleptr, ww->maxnh );
	    ww->unlock();
	}

	worldbundle *wb;

	ww->testcancel();
	wb = read_bundle(*ins, ww->readflags & KIRA_VERBOSE);
	ww->testcancel();
	if(wb == NULL) {
	    ins->close();
	    if(paras == 0) paras = -1;
	    break;
	}

	ww->lock();
	ww->wh[ww->nh++] = wb;
	paras++;
	ww->unlock();
    }

    if(paras > 0) {
	ww->tmin = ww->wh[0]->get_t_min();
	ww->tmax = ww->wh[ww->nh-1]->get_t_max();
    }
    return paras;
}
#endif

void kira_parti_init() {
    parti_add_reader( kira_read, "kira", NULL );
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
    specks_set_timestep( st );
    parti_redraw();
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

int hasdata( istream *s, double maxwait ) {
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


int kira_get_trange( struct dyndata *dd, struct stuff *st, double *tminp, double *tmaxp, int ready )
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


int kira_open( struct dyndata *dd, struct stuff *st, istream *ins, const char *filename, int readflags )
{
    // If data field names aren't already assigned, set them now.
    for(int i = 0; fieldnames[i] != NULL; i++) {
	if(st->vdesc[st->curdata][i].name[0] == '\0')
	    strcpy(st->vdesc[st->curdata][i].name, fieldnames[i]);
    }

    if(ins == NULL && (filename == NULL || !strcmp(filename, "-init"))) {
	return 0;	// "kira -init" sets up datafield names w/o complaint
    }

    TCPsocket *ts = NULL;

    if(ins == NULL) {
	if(0==strncmp(filename, "kira://", 7)) {
	    ts = new TCPsocket( filename+7, 7078/*M15*/, true );
	    if(ts->valid()) {
		readflags |= KIRA_READLATER;	// always read asynchronously from sockets
	    } else {
		msg("Can't connect to %s", filename);
		delete ts;
		return 0;
	    }
	} else {
	    ins = new ifstream(filename);
	    if (!ins || !*ins) {
		msg("Can't open Kira/starlab data file \"%s\".", filename);
		delete ins;
		return 0;
	    }
	}
    }

    struct worldstuff *ww = new worldstuff( st );
    // ww->init( st );
    ww->tmin = ww->tmax = 0;
    ww->tcur = ww->treq = -1.0;		/* invalidate */
    ww->ks.readflags( readflags );

    memset( dd, 0, sizeof(*dd) );
    dd->data = ww;
    dd->getspecks = kira_get_parti;
    dd->trange = kira_get_trange;
    dd->ctlcmd = kira_parse_args;
    dd->draw = kira_draw;
    dd->help = kira_help;
    dd->free = NULL;			/* XXX create a 'free' function someday */
    dd->enabled = 1;
    dd->slvalid = 0;

    if(ts != NULL) {
	/* always read asynchronously from sockets */
	ww->ks.start( ts );
    } else if((readflags & KIRA_READLATER)) {
	/* read asynchronously from file */
	ww->ks.start( ins );
    } else {
	/* read synchronously from file */
	ww->ks.kira_read_some( ins );
	if (ww->ks.nh == 0) {
	    msg("Data file %s not in worldbundle format.", filename);
	    dd->enabled = 0;
	    return 0;
	}
	{
	    scoped_lock lk( ww->ks.thr.mutex() );
	    preload_pdyn((worldbundle **)ww->ks.wh, ww->ks.nh, false);
	}
    }

    // XXX does this need to go into kira_read_more()?
    float mscale = mass_scale_factor();
    if(mscale > 0) {
	ww->massscale = mscale;
	ww->truemassscale = 1;
    }

    ww->ih = 0;
    ww->sl = NULL;


    // Register for H-R diagram display
    static float xrange[] = { 5, 3 };	// initial range of log(T)
    static float yrange[] = { -4, 6 };	// initial range of log(L)
    static char xtitle[] = "log T";
    static char ytitle[] = "log L";

#if USE_PLOT
    Fl_Plot *plot;
    plot = parti_register_plot( st, (void (*)(Fl_Plot*,void*,void*))kira_HRplot, dd );
    if(plot) {
	if(plot->x0() == plot->x1())
	    plot->xrange( xrange[0], xrange[1] );
	if(plot->y0() == plot->y1())
	    plot->yrange( yrange[0], yrange[1] );
	if(plot->xtitle() == NULL) plot->xtitle( xtitle );
	if(plot->ytitle() == NULL) plot->ytitle( ytitle );
	plot->eventhook = kira_HRplot_events;
    }
    ww->plot = plot;
#endif /*USE_PLOT*/
    return 1;
}

double get_parti_time( struct dyndata *dd, struct stuff *st ) {
    return dd->data ? ((struct worldstuff *)(dd->data))->tcur : 0.0;
}

void set_parti_time( struct dyndata *dd, struct stuff *st, double reqtime ) {
    if(dd->data == NULL)
	return;
    // otherwise please seek to requested time ...
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

    sp->val[SPECK_ISMEMBER] = is_member( ww->curwb, b );
    sp->val[SPECK_NCLUMP] = 0;		// complete (add n_leaves) in kira_to_parti

    // DON'T offset unperturbed binaries by 100 in "nclump" field.

    //if (b != top) {
    //	pdyn *bb = b;
    //	while (bb->get_elder_sister())
    //	    bb = bb->get_elder_sister();
    //	if (bb->get_kepler()) sp->val[SPECK_NCLUMP] += 100;
    //}

    sp->val[SPECK_ROOTID] = (top->is_leaf())
			? top->get_index()
			: -top->get_worldline_index();

    // Addr is 0 for top-level leaf, 1 for top-level CM,
    // constructed from parent addr otherwise.

    sp->val[SPECK_TREEADDR] = addr;

    sp->val[SPECK_RINGSIZE] = 0;

    if(id != 0 && ww->tracking == id) {
	Point newpos;
	newpos.x[0] = b->get_pos()[0];
	newpos.x[1] = b->get_pos()[1];
	newpos.x[2] = b->get_pos()[2];
	if(ww->wastracking) {
	    Point delta;
	    vsub( &delta, &newpos,&ww->trackpos );
	    parti_nudge_camera( &delta );
	} else {
	    kira_track_break( ww, &newpos );
	}
	ww->trackpos = newpos;
	ww->wastracking = 1;
    }

    sl->nspecks++;
    sp = NextSpeck(sp, sl, 1);
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

    if(ww == NULL || ww->ks.nh == 0) return NULL;


    struct specklist *sl = ww->bufsl[ww->bufno];
    struct specklist *marksl = ww->bufmarksl[ww->bufno];

    if(sl == NULL) {
	// Use NewN to allocate these in (potentially) shared memory,
	// rather than new which will allocate in local malloc pool.
	// Needed for virdir/cave version.

	int smax = root->n_leaves();
	int wmax;
	{
	    scoped_lock lk( ww->ks.mutex() );
	    wmax = ww->ks.wmax();		// find max number of stars over all current wb's
	}

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
	marksl->specks = NewNSpeck( marksl, ww->maxmarks );
	marksl->special = MARKERS;

	sl->nsel = ww->maxstars;
	marksl->nsel = ww->maxmarks;
	if(ww->bufsl[1 - ww->bufno]) {
	    sl->sel = ww->bufsl[1-ww->bufno]->sel;
	    marksl->sel = ww->bufmarksl[1-ww->bufno]->sel;
	} else {
	    sl->sel = NewN( SelMask, ww->maxstars );
	    memset( sl->sel, 0, sl->nsel*sizeof(SelMask) );
	    marksl->sel = NewN( SelMask, ww->maxmarks );
	    memset( marksl->sel, 0, marksl->nsel*sizeof(SelMask) );
	}

	sl->next = marksl;
	marksl->next = NULL;

	if(ww->bufleafsel[1 - ww->bufno] != NULL) {
	    ww->bufleafsel[ww->bufno] = ww->bufleafsel[1 - ww->bufno];
	} else {		// Only one bufleafsel
	    ww->bufleafsel[ww->bufno] = NewN( SelMask, ww->maxstars );
	    ww->nleafsel = ww->maxstars;
	    memset( ww->bufleafsel[ww->bufno], 0, ww->maxstars*sizeof(SelMask) );
	}

	ww->trails = NewN( trailhead, ww->maxstars );
	memset(ww->trails, 0, ww->maxstars*sizeof(trailhead));

	selinit( &ww->intdest );
	selinit( &ww->intsrc );

	ww->bufsl[ww->bufno] = sl;
	ww->bufmarksl[ww->bufno] = marksl;

	specks_all_picks( st, kira_picked, dd );

    }
    ww->sl = sl;
    ww->marksl = marksl;

    if(ww->myselseq < sl->selseq && sl->sel != NULL) {
	// somebody changed something in the global select bits, so
	// gather all stars' sel-bits back into our local copy.
	struct speck *tsp = sl->specks;
	SelMask *leafsel = ww->bufleafsel[1 - ww->bufno];
	for(int k = 0; k < sl->nsel; k++) {
	    int id = (int)tsp->val[SPECK_ID];
	    if(id == 0) break;
	    if(id > 0 && id < ww->nleafsel) {
		leafsel[id] = sl->sel[k];
	    } else if(id < 0 && id + ww->maxstars >= ww->maxleaves) {
		leafsel[ww->maxstars + id] = sl->sel[k];
	    }
	    tsp = NextSpeck(tsp, sl, 1);
	}
	ww->myselseq = sl->selseq;
    }


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
	    if(ww->trailsel.use != SEL_NONE &&
		    SELECTED( ww->leafsel[id], &ww->trailsel )) {
		kira_add_trail( st, ww, id, tsp );
	    } else if(ww->trailonly) {
		kira_erase_trail( st, ww, id );
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

    specks_reupdate( st, sl );

    ww->bufno = 1 - ww->bufno;
    return sl;
}

struct specklist *kira_get_parti( struct dyndata *dd, struct stuff *st, double realtime )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;

    if (ww == NULL || ww->ks.nh == 0)
	return NULL;

    ww->treq = realtime;

    {
	scoped_lock lk( ww->ks.mutex() );	// grab shared data for a moment
						// lock released when we leave this scope

	ww->ks.trange( &ww->tmin, &ww->tmax );

	if (realtime < ww->tmin) realtime = ww->tmin;
	if (realtime > ww->tmax) realtime = ww->tmax;

	if (ww->tcur == realtime && ww->slvalid)
	    return ww->sl;

	int ih = ww->ih;
	int nh = ww->ks.nh;

	worldbundle * volatile * volatile wh = ww->ks.wh;

	worldbundle *wb = wh[ih];
	for(; realtime > wb->get_t_max() && ih < nh-1; ih++, wb = wh[ih])
	    ;
	for(; realtime < wb->get_t_min() && ih > 0; ih--, wb = wh[ih])
	    ;

	ww->curwb = wb;
	ww->ih = ih;
	ww->tcur = realtime;
    }

#if OLDTREE
    pdyn *root = create_interpolated_tree(ww->curwb, realtime);
    ww->sl = kira_to_parti(root, dd, st, ww);
    rmtree(root);
#else
    pdyn *root = create_interpolated_tree2(ww->curwb, realtime);
    ww->center_pos = get_center_pos();
    ww->center_vel = get_center_vel();
    ww->sl = kira_to_parti(root, dd, st, ww);
#endif

    ww->slvalid = 1;
    return ww->sl;
}

#define TRAILGAP 1		/* bit in alpha byte of rgba -> "don't draw from prev pt to here" */

void kira_add_trail( struct stuff *st, worldstuff *ww, int id, struct speck *sp )
{
    if(id < 0) id = ww->maxstars + id;
    if(id <= 0 || id > ww->maxstars || ww->maxtrail <= 0) return;

    struct trailhead *th = &ww->trails[id];
    int bps = ww->sl->bytesperspeck;
    if(th->maxtrail <= 0) {
	th->maxtrail = ww->maxtrail;
	th->specks = (struct speck *)NewN( char, th->maxtrail*bps );
	th->ntrails = 0;
	th->next = 0;
    }
    if(th->ntrails > th->maxtrail)
	th->ntrails = th->maxtrail;
    if(th->next < 0 || th->next >= th->maxtrail)
	th->next = 0;
    struct speck *tsp = (struct speck *)(((char *)th->specks) + th->next*bps);
    memcpy( tsp, sp, bps );
    vsub( &tsp->p, &tsp->p, &ww->trackpos );
    if(fabs(th->lasttime - ww->treq) > ww->maxtrailgap) {
	((char *)&tsp->rgba)[3] = TRAILGAP;	/* alpha "1" bit is on for post-gap trail points */
    } else {
	((char *)&tsp->rgba)[3] = 0;
    }
    tsp->val[0] = ww->treq;
    th->lasttime = ww->treq;
    if(++th->next >= th->maxtrail)
	th->next = 0;
    if(++th->ntrails > th->maxtrail)
	th->ntrails = th->maxtrail;
    if(id >= ww->maxtrailno)
	ww->maxtrailno = id+1;
}

void kira_erase_trail( struct stuff *st, worldstuff *ww, int id )
{
    if(id < 0) id = ww->maxstars + id;
    if(id <= 0 || id > ww->maxstars || ww->maxtrail <= 0) return;
    struct trailhead *th = &ww->trails[id];
    th->next = th->ntrails = 0;
    if(id+1 == ww->maxtrailno) {
	while(ww->maxtrailno > 0 && ww->trails[ww->maxtrailno-1].ntrails == 0)
	    ww->maxtrailno--;
    }
}

void kira_track_break( struct worldstuff *ww, Point *newtrack )
{
    Point incr;
    vsub( &incr, newtrack, &ww->trackpos );
    for(int id = 0; id < ww->maxstars; id++) {
	struct trailhead *th = &ww->trails[id];
	if(th->ntrails > 0) {
	    struct speck *sp = th->specks;
	    for(int i = 0; i < th->maxtrail; i++) {
		vsub( &sp->p, &sp->p, &incr );
		sp = NextSpeck(sp, ww->sl, 1);
	    }
	}
    }
    // ww->trackpos = *newtrack;  no, let caller do that.
}

void kira_maxtrail( struct dyndata *dd, struct stuff *st, int newmax )
{
    int i;
    struct worldstuff *ww = (struct worldstuff *)dd->data;
    ww->maxtrail = newmax;
    if(ww->trails == NULL) return;
    int bps = ww->sl->bytesperspeck;
    char *spare = (char *)malloc( bps * newmax );

    for(i = 0; i < ww->maxstars; i++) {
	struct trailhead *th = &ww->trails[i];
	if(th->maxtrail == 0)
	    continue;
	if(th->ntrails > newmax) th->ntrails = newmax;
	int first = (th->next + th->maxtrail - th->ntrails) % th->maxtrail;
	char *base = (char *)th->specks;
	if(first < th->next) {
	    memmove( spare, base + first*bps, th->ntrails*bps ); 
	} else {
	    /* rearrange two pieces: 0..next-1  first..max-1
	     * into 0..keep-1  keep..keep+next-1
	     */
	    int keep = th->maxtrail - first;
	    memcpy( spare, base + first*bps, keep*bps );
	    memmove( spare + keep*bps, base, th->next*bps );
	}
	Free( th->specks );
	th->specks = (struct speck *)NewN( char, newmax*bps );
	memcpy( th->specks, spare, th->ntrails*bps );
	th->next = th->ntrails;
	th->maxtrail = newmax;
    }
    free(spare);
}


int kira_draw( struct dyndata *dd, struct stuff *st, struct specklist *slhead, Matrix *Tc2w, float radperpix )
{
    struct worldstuff *ww = (struct worldstuff *)dd->data;

    if(ww == NULL || !dd->enabled)
	return 0;

    struct specklist *sl;
    float halftickscale = 0.5 * ww->tickscale;
    int inpick = st->inpick;
    int slno;

    static Point zero = {0,0,0};
    Point fwdvec = {0,0,-radperpix};	// scaled by pixels-per-radian
    Point eyepoint, fwd, unitfwd;
    float fwdd;

    vtfmpoint( &eyepoint, &zero, Tc2w );
    vtfmvector( &fwd, &fwdvec, Tc2w );
    fwdd = -vdot( &eyepoint, &fwd );
    vunit( &unitfwd, &fwd );

#define MAXRING 32
    Point fan[MAXRING];
    int nring = 24;
    int i;
    for(i = 0; i < nring; i++) {
	float th = 2*M_PI*i / nring;
	vcomb( &fan[i],  cos(th),(Point *)&Tc2w->m[0*4+0],
			 sin(th),(Point *)&Tc2w->m[1*4+0] );
    }

    int specks_slno = 0;

    for(sl = slhead, slno = 1; sl != NULL; sl = sl->next, slno++) {
	if(sl == ww->sl) {
	    specks_slno = slno;

	} else if(sl->special == MARKERS) {
	    int ns = sl->nspecks;
	    struct speck *sp = sl->specks;

	    if(inpick) {
		glLoadName(slno);
		glPushName(0);
	    }
	    for(i = 0; i < ns; i++, sp = NextSpeck(sp, sl, 1)) {
		float unitperpix = vdot( &fwd, &sp->p ) + fwdd;
		if(unitperpix <= 0) continue;

		if( !SELECTED(sl->sel[i], &st->seesel) )
		    continue;
		if(ww->treerings == KIRA_ON ||
			(ww->treerings == KIRA_ROOTS && sp->val[SPECK_TREEADDR] == 1)) {
		    float ringpixels = fabs(sp->val[SPECK_RINGSIZE] * ww->ringscale) / unitperpix;
		    if(ringpixels < ww->ringmin) ringpixels = ww->ringmin;
		    if(ringpixels > ww->ringmax) ringpixels = ww->ringmax;
		    float rring = ringpixels * unitperpix;
		    int step = ringpixels>20 ? 1 : ringpixels>8 ? 2 : 3;

		    if(inpick) glLoadName(i);
		    else glColor3ubv( (GLubyte *)&sp->rgba );
		    glBegin( GL_LINE_LOOP );
		    for(int k = 0; k < nring; k+=step)
			glVertex3f( sp->p.x[0] + rring*fan[k].x[0],
				    sp->p.x[1] + rring*fan[k].x[1],
				    sp->p.x[2] + rring*fan[k].x[2] );
		    glEnd();
		}
		if(ww->treearcs != KIRA_OFF) {
		    float mu = sp->val[SPECK_MU];
		    float *sep = &sp->val[SPECK_SEPVEC];

		    if(inpick) glLoadName(i);
		    else glColor3ubv( (GLubyte *)&sp->rgba );
		    glBegin( GL_LINES );

		    if(ww->treearcs != KIRA_TICK) {
			glVertex3f(
			    sp->p.x[0] - mu*sep[0],
			    sp->p.x[1] - mu*sep[1],
			    sp->p.x[2] - mu*sep[2] );
			glVertex3f(
			    sp->p.x[0] + (1-mu)*sep[0],
			    sp->p.x[1] + (1-mu)*sep[1],
			    sp->p.x[2] + (1-mu)*sep[2] );
		    }

		    if(ww->treearcs == KIRA_CROSS || ww->treearcs == KIRA_TICK) {
			Point cr;
			vcross( &cr,  &unitfwd, (Point *)sep );
			float sepsep = vdot((Point *)sep, (Point *)sep);
			float sepfwd = vdot((Point *)sep, &unitfwd);
			float scaleby = halftickscale / sqrt(1 - sepfwd*sepfwd/sepsep);
			glVertex3f(
			   sp->p.x[0] - scaleby*cr.x[0],
			   sp->p.x[1] - scaleby*cr.x[1],
			   sp->p.x[2] - scaleby*cr.x[2] );
			glVertex3f(
			   sp->p.x[0] + scaleby*cr.x[0],
			   sp->p.x[1] + scaleby*cr.x[1],
			   sp->p.x[2] + scaleby*cr.x[2] );
		    }
		    glEnd();
		}
	    }
	    if(inpick)
		glPopName();
	}
    }

    glLineWidth( ww->trailpsize );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    int alpha = 0, rgbmask = ~0;
    ((GLubyte *)&alpha)[3] = (int) (255 * ww->trailalpha);
    ((GLubyte *)&rgbmask)[3] = 0;
    if(inpick) {
	glPushName( specks_slno );
	glPushName( 0 );
    }

    glPointSize( 1.5 );
    glPushMatrix();
    glTranslatef( ww->trackpos.x[0], ww->trackpos.x[1], ww->trackpos.x[2] );
    for(i = 0; i < ww->maxtrailno; i++) {
	struct trailhead *th = &ww->trails[i];
	if(th->ntrails == 0 || th->maxtrail <= 0)
	    continue;
	if(inpick)
	    glLoadName( i );
	glBegin( GL_LINE_STRIP );
	int first = (th->next + th->maxtrail - th->ntrails) % th->maxtrail;
	int k, rgb = 0, rgba = 0;
	struct speck *sp;
	char *base = (char *)th->specks;
	int bps = ww->sl->bytesperspeck;
	int from1 = first;
	int to1 = (first < th->next) ? th->next : th->maxtrail;
	int from2 = 0;
        int to2 = (first < th->next) ? 0 : th->next;
	int wasgap = 0, ingap = 0;
	Point *prev = NULL;
	for(k = from1; k < to1; k++) {
	    sp = (struct speck *)(base + k*bps);
	    ingap = ((char *)&sp->rgba)[3] & TRAILGAP;
	    if(wasgap != ingap) {
		glEnd();
		if(ingap) {
		    glBegin( GL_POINTS );
		} else {
		    glBegin( GL_LINE_STRIP );
		    glVertex3fv( prev->x );
		}
		wasgap = ingap;
	    }
	    if(rgb != (sp->rgba & rgbmask)) {
		rgb = sp->rgba & rgbmask;
		rgba = rgb | alpha;
		glColor4ubv( (GLubyte *)&rgba );
	    }
	    glVertex3fv( sp->p.x );
	    prev = &sp->p;
	}
	for(k = from2; k < to2; k++) {
	    sp = (struct speck *)(base + k*bps);
	    ingap = ((char *)&sp->rgba)[3] & TRAILGAP;
	    if(wasgap != ingap) {
		glEnd();
		if(ingap) {
		    glBegin( GL_POINTS );
		} else {
		    glBegin( GL_LINE_STRIP );
		    glVertex3fv( prev->x );
		}
		wasgap = ingap;
	    }
	    if(rgb != (sp->rgba & rgbmask)) {
		rgb = sp->rgba & rgbmask;
		rgba = rgb | alpha;
		glColor4ubv( (GLubyte *)&rgba );
	    }
	    glVertex3fv( sp->p.x );
	    prev = &sp->p;
	}
	glEnd();
    }
    glPopMatrix();
    if(inpick) {
	glPopName();
	glPopName();
    }
    return 1;
}

int kira_parse_args( struct dyndata *dd, struct stuff *st, int argc, char **argv )
{
    char *swhat = argv[1];
    char *sval = argc>2 ? argv[2] : NULL;
    int what;
    double val;
    worldstuff *ww = (worldstuff *)dd->data;
    int changed = 0;

    if(0!=strncmp(argv[0], "kira", 4))	/* accept "kira" or "kiractl" */
	return 0;

    if(swhat == NULL) swhat = "?";

    if(!strncmp(swhat, "sep", 3) || !strncmp(swhat, "semi", 4)) {
	kira_set( dd, st, KIRA_RINGSIZE, swhat[2]=='p' ? KIRA_RINGSEP : KIRA_RINGA );
	msg("kiractl ringsize %s",
	    kira_get( dd, st, KIRA_RINGSIZE ) == KIRA_RINGSEP ? "separation" : "semimajor");

    } else if(!strcmp(swhat, "ringsize") || !strcmp(swhat, "ringscale") || !strcmp(swhat, "size")) {
	if(sval) {
	    val = !strncmp(sval,"sep",3) ? KIRA_RINGSEP
		: !strncmp(sval,"semi",4) ? KIRA_RINGA
		: !strcmp(sval,"a") ? KIRA_RINGA
		: kira_get( dd, st, KIRA_RINGSIZE );
	    kira_set( dd, st, KIRA_RINGSIZE, val );
	    if(argc > 3)
		kira_set( dd, st, KIRA_RINGSCALE,
		    getfloat( argv[3], kira_get( dd, st, KIRA_RINGSCALE ) ) );
	}
	msg("kiractl ringsize %s %g",
	    kira_get( dd, st, KIRA_RINGSIZE ) == KIRA_RINGSEP ? "separation" : "semimajor",
	    kira_get( dd, st, KIRA_RINGSCALE ));

    } else if(!strcmp(swhat, "ringscale") || !strcmp(swhat, "scale")) {
	if(sval)
	    kira_set( dd, st, KIRA_RINGSCALE, getfloat( sval, kira_get(dd,st,KIRA_RINGSCALE) ) );
	msg("kiractl ringscale %g", kira_get(dd,st,KIRA_RINGSCALE));

    } else if(!strcmp(swhat, "span") || !strcmp(swhat, "ringspan")) {
	if(argc>2 && (val = getfloat( argv[2], -1 )) >= 0)
		kira_set( dd, st, KIRA_RINGMIN, val );
	if(argc>3 && (val = getfloat( argv[3], -1 )) >= 0)
		kira_set( dd, st, KIRA_RINGMAX, val );
	msg("kiractl ringspan %.0f %.0f",
	    kira_get( dd, st, KIRA_RINGMIN ), kira_get( dd, st, KIRA_RINGMAX ));

    } else if(!strncmp(swhat, "nod", 3) || !strncmp(swhat, "ring", 4)) {
	what = swhat[0]=='n' ? KIRA_NODES : KIRA_RINGS;
	if(sval)
	    kira_set( dd, st, what,
		 (0==strncmp(sval, "root", 4)) ? KIRA_ROOTS : getbool(sval, KIRA_ON) );
	val = kira_get( dd, st, what );
	msg("kiractl %s %s %g",
	    what==KIRA_NODES ? "nodes" : "rings",
	    val==2 ? "root" : val==1 ? "on" : "off",
	    kira_get( dd, st, KIRA_TICKSCALE ));

    } else if(!strncmp(swhat, "tree", 3) || !strcmp(swhat, "arc")) {
	if(sval)
	    kira_set( dd, st, KIRA_TREE,
		sval[0]=='c' ? KIRA_CROSS : sval[0]=='t' ? KIRA_TICK
		: getbool(sval, KIRA_ON) );
	if(argc>3 && sscanf(argv[3], "%lf", &val)>0)
	    kira_set( dd, st, KIRA_TICKSCALE, val );
	val = kira_get( dd, st, KIRA_TREE );
	msg("kiractl tree %s",
	    val==KIRA_CROSS ? "cross" : val==KIRA_TICK ? "tick"
	    : val ? "on" : "off");

    } else if(!strncmp(swhat, "mscale", 4) || !strcmp(swhat, "massscale")) {
	if(!ww->truemassscale || strchr(sval, '!')) {
	    ww->massscale = getfloat( sval, ww->massscale );
	    changed = 1;
	}
	msg("kiractl mscale %g (kira says %g)",
		ww->massscale, mass_scale_factor());

    } else if(!strncmp(swhat, "track", 4)) {
	if(sval)
	    kira_set( dd, st, KIRA_TRACK, getbool(sval, 0) );
	val = kira_get( dd, st, KIRA_TRACK );
	msg(val == 0 ? "kiractl track off" : "kiractl track %d", (int)val);

    } else if(!strncmp(swhat, "center", 6)) {
	if(sval) {
	    int just = 0;
	    if(!strncmp(sval,"off",3) || !strcmp(sval,"inertial")) {
		ww->centered = 0;
		just = 1;
	    } else if(!strcmp(sval,"on")) {
		/* fine */
	    } else if(!strcmp(sval,"next") || sval[0] == '+') {
		ww->which_center++;
	    } else if(isdigit(sval[0])) {
		ww->which_center = atoi(sval);
	    } else {
		just = 1;
	    }
	    if(ww->which_center >= get_n_center())
		ww->which_center %= get_n_center();
	    if(!just) ww->centered = 1;
	    if(ww->centered) {
		scoped_lock lk( ww->ks.mutex() );
		set_center( (worldbundle **)ww->ks.wh, ww->ks.nh, ww->which_center );
	    }
	    changed = 1;
	}

	msg("kira center %s%d(%s) (center pos %g %g %g vel %g %g %g)",
	  ww->centered ? "" : "off ",
	  ww->which_center, get_center_id(),
	  ww->center_pos[0], ww->center_pos[1], ww->center_pos[2],
	  ww->center_vel[0], ww->center_vel[1], ww->center_vel[2] );
    
    } else if(!strcmp(swhat, "maxtrail")) {
	if(sval)
	    kira_maxtrail( dd, st, getbool(sval, ww->maxtrail) );
	msg("kira maxtrail %d", ww->maxtrail);

    } else if(!strcmp(swhat, "trail")) {
	if(sval) {
	    if(!strcmp(sval, "clear")) {
		for(int i = 0; i < ww->maxstars; i++)
		    kira_erase_trail( st, ww, i );
	    } else {
		int a = 2;
		if(!strcmp(sval, "only")) {
		    ww->trailonly = 1;
		    a = 3;
		} else {
		    ww->trailonly = 0;
		}
		parse_selexpr( st, rejoinargs( a, argc, argv ), NULL, &ww->trailsel, "kira trail" );
	    }
	    changed = 1;
	}
	msg("kira trail%s %s (%s)", ww->trailonly ? " only":"",
		    show_selexpr( st, NULL, &ww->trailsel ),
		    selcounts( st, st->sl, &ww->trailsel ));

    } else if(!strncmp(swhat, "trailgap", 6)) {
	if(sval) {
	    ww->maxtrailgap = getfloat( sval, ww->maxtrailgap );
	    changed = 1;
	}
	msg("kira trailgaptime %g", ww->maxtrailgap);

    } else if(!strncmp(swhat, "pick", 2)) {
	int picking = getbool( sval, (st->picked == kira_picked) );
	if(picking) {
	    specks_all_picks( st, kira_picked, dd );
	} else {
	    specks_all_picks( st, NULL, NULL );
	}

    } else if(!strncmp(swhat, "int", 3)) {
	what = parse_selexpr( st, rejoinargs( 2, argc, argv ), &ww->intdest, &ww->intsrc, "kira intsel" );
	if(what != 3)
	    selsrc2dest( st, &ww->intsrc, &ww->intdest );
	ww->intdest.wanted &= ~ww->intdest.wanton;	/* interact always OR's into existing set */

	msg("kiractl interact %s", show_selexpr(st, &ww->intdest, &ww->intsrc));

    } else if(!strncasecmp(swhat, "hr", 2)) {
#if !USE_PLOT
	msg("kira hrdiag: H-R diagram not available");
#else
	Fl_Plot *plot = ww->plot;
	if(plot == NULL) {
	    msg("kira hrdiag: not initialized?");
	    return 1;
	}
	if(sval) {
	    if(!strcmp(sval, "on") || !strcmp(sval, "off")) {
		parti_hrdiag_on( getbool(sval, 1) );
		if(argc > 3) argc--, argv++, sval = argv[2];
	    }
	    if(!strcmp(sval, "range")) {
		float xxyyrange[4] = { plot->x0(), plot->x1(), plot->y0(), plot->y1() };
		getfloats( &xxyyrange[0], 4, 3, argc, argv );
		plot->xrange( xxyyrange[0], xxyyrange[1] );
		plot->yrange( xxyyrange[2], xxyyrange[3] );
	    }
	}
	msg( "kira hrdiag %s range %g %g(logT) %g %g(logL)",
		plot->visible_r() ? "on":"off",
		plot->x0(), plot->x1(), plot->y0(), plot->y1() );
#endif

    } else {
	msg("kiractl {node|ring} {on|off|root} | tree {on|off|cross|tick} [<tickscale>] | size {sep|semimaj} | scale <fac> | span <minpix> <maxpix> | track <id>| intsel <dest> = <src>");
    }
    if(changed) {
	kira_invalidate( dd, st );
	parti_redraw();
    }
    return 1;
}

void turnoff( struct specklist *sl, SelOp *dest ) {
    if(sl && sl->sel && sl->nsel >= sl->nspecks && dest && dest->use == SEL_DEST) {
	SelMask *sel = sl->sel;
	for(int i = 0; i < sl->nspecks; i++) {
	    SELUNSET( sel[i], dest );
	}
	sl->selseq++;
    }
}

int kira_picked( struct stuff *st, GLuint *hit, struct specklist *sl, int speckno )
{
    struct dyndata *dd = (struct dyndata *)st->pickinfo;
    worldstuff *ww = (worldstuff *)dd->data;
    int retain;

#if CAVE || !USE_PLOT
    retain = 1;
#else
    retain = Fl::event_state(FL_CTRL);
#endif

    if(hit == NULL) {
	if(speckno > 0 && !retain) {
	    turnoff( ww->sl, &ww->picksel );
	    turnoff( ww->marksl, &ww->picksel );
	    ww->pickcount = 0;
	}
	if(speckno == 0) {
	    if(ww->pickcount > 0 || !retain) {
		st->selseq++;
		kira_invalidate( dd, st );
		parti_redraw();
	    }
	}
	return 0;
    }

    if(ww->sl == sl && sl->sel && sl->nsel >= sl->nspecks) {
	if(speckno < 0 || speckno >= sl->nspecks) return 0;
	if(ww->picksel.use == SEL_DEST)
	    SELSET( sl->sel[speckno], &ww->picksel );

	struct speck *sp = NextSpeck( sl->specks, sl, speckno );
	float teff = expf( sp->val[SPECK_TLOG] * M_LN10 );
	enum spectral_class st = get_spectral_class( teff );
	enum luminosity_class lc = get_luminosity_class( teff, sp->val[SPECK_LUM] );
	msg("[id %g nc %g mass %.3g Tlog %.2f L %.3g root %g stype %d(%s %s%s) speck %d]",
		sp->val[SPECK_ID], sp->val[SPECK_NCLUMP],
		sp->val[SPECK_MASS], sp->val[SPECK_TLOG],
		sp->val[SPECK_LUM], sp->val[SPECK_ROOTID],
		(int)sp->val[SPECK_STYPE],
		type_string((enum stellar_type)sp->val[SPECK_STYPE]),
		type_string(st), type_string(lc),
		speckno);

	sl->selseq++;
    }
    return 0;
}

#if USE_PLOT
static float HRplot_dotsize = 2.5;
static float HRplot_alpha = 0.7;

int kira_HRplot_events( Fl_Plot *plot, int ev ) {
    if(ev == FL_KEYBOARD) {
	switch(Fl::event_text()[0]) {
	case 'b': HRplot_dotsize *= 1.33; break;
	case 'B': HRplot_dotsize /= 1.33; break;
	case 'a': HRplot_alpha = 1 - (1 - HRplot_alpha)*.75; break;
	case 'A': HRplot_alpha = 1 - (1 - HRplot_alpha)*1.33; break;
	case '\033': parti_hrdiag_on( 0 ); break;
	default: return 0;
	}
	plot->redraw();
	return 0;	/* let Fl_Plot::handle have at it too */
    }
    return 0;
}


void kira_HRplot( Fl_Plot *plot, struct stuff *st, struct dyndata *dd )
{
    if(dd == NULL) return;
    worldstuff *ww = (worldstuff *)dd->data;
    int inpick = plot->inpick();
    int wasemph = -1;
    int plainalpha = (int) (255 * HRplot_alpha);
    int emphalpha = (int) (255 * (1 - (1 - HRplot_alpha)*.5));
    int alpha = plainalpha;

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glDisable( GL_ALPHA_TEST );
    glEnable( GL_POINT_SMOOTH );
    glPointSize( HRplot_dotsize );

    float xmin = plot->x0();
    float xmax = plot->x1();
    if(xmin > xmax) xmin = xmax, xmax = plot->x0();
    float ymin = plot->y0();
    float ymax = plot->y1();
    if(ymin > ymax) ymin = ymax, ymax = plot->y0();

    int slno = 1;

    for(struct specklist *sl = ww->sl; sl != NULL; sl = sl->next, slno++) {
	if(sl->special != SPECKS)
	    continue;

	if(inpick) {
	    glLoadName(slno);
	    glPushName(0);
	}
	glBegin( GL_POINTS );
	int ns = sl->nspecks;
	struct speck *sp = sl->specks;
	for(int i = 0; i < ns; i++, sp = NextSpeck(sp, sl, 1)) {
	    if(sp->val[SPECK_MU] != 0 || sp->val[SPECK_LUM] <= 0)
		continue;	/* leaf nodes only */
	    int rgba = sp->rgba;
	    if( !SELECTED(sl->sel[i], &st->seesel) )
		continue;
	    if(inpick) {
		glEnd();
		glLoadName(i);
		glBegin( GL_POINTS );
	    } else {
		int isemph = st->emphsel.use != SEL_NONE &&
					SELECTED( sl->sel[i], &st->emphsel );
		if(isemph != wasemph) {
		    glPointSize( isemph ? HRplot_dotsize*1.5 : HRplot_dotsize );
		    alpha = isemph ? emphalpha : plainalpha;
		    wasemph = isemph;
		}
	    }
	    ((unsigned char *)&rgba)[3] = alpha;
	    glColor4ubv( (GLubyte *)&rgba );

	    float x = sp->val[SPECK_TLOG];
	    float y = sp->val[SPECK_LUM] > 0 ? log10f( sp->val[SPECK_LUM] ) : ymin;
	    glVertex2f( x<xmin ? xmin : x>xmax ? xmax : x,
			y<ymin ? ymin : y>ymax ? ymax : y );
	}
	glEnd();
	if(inpick) glPopName();
    }
}
#endif /*USE_PLOT*/

#endif /* USE_KIRA */
