#ifdef STANDALONE
# define _FILE_OFFSET_BITS  64
#endif

#define _GNU_SOURCE 1	/* for sincosf() from <math.h> if GNU libc */

#ifdef USE_WARP
/*
 * Time-dependent warp of a particle field,
 * for fake differentially-rotating galaxies.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "config.h"

#ifdef WIN32
# include "winjunk.h"
#endif

#if unix
# include <unistd.h>
#endif

#ifndef STANDALONE
#include "plugins.h"
#endif

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
# undef isspace
# undef isdigit

#ifdef HAVE_VALUES_H
# include <values.h>  /* for MAXFLOAT */
#endif

/* In case values.h doesn't define MAXFLOAT, e.g. on mingw32, ... */
#ifndef MAXFLOAT
# define MAXFLOAT  3e38
#endif

#include "shmem.h"
#include "specks.h"
#include "partiviewc.h"
#include "galaxyorbit.h"

#include "textures.h"		/* for get_dsp_context() */

#ifndef __linux__
/* MacOSX and MinGW don't have this */
static void my_sincosf(float x, float *sinp, float *cosp)
{
    *sinp = sinf(x);
    *cosp = cosf(x);
}
#define sincosf(x,s,p)  my_sincosf(x,s,p)
#endif

static char local_id[] = "$Id: warp.c,v 1.56 2010/12/17 03:41:22 slevy Exp $";

struct speckcache {
    float tfrac;
    double realtime;
    struct specklist *sl;
};

#define COORD_DISK   0
#define COORD_WORLD  (-1)
#define COORD_OBJECT (-2)

typedef enum { DIFFROT, EXTRAPOLATE, PROJECT, GALAXYRIDE, SHEETWARP } warpstyle_t;

struct cf {
    int c;
    float f;
};

struct svvec {
    struct cf *v;
    int len, room, maxc;
};

struct warpstuff {
    struct stuff *st;

    warpstyle_t style;

    /* for style == DIFFROT (differential rotation in XZ plane) */

    float tfrom, tto;
    float zerotime;
    int has_trange, has_zerotime;
    float period0;
    float tunit;
    float rcore, routercore;
    int core_coordsys;
    float rcoredisk, routercoredisk;
    double fixedp[3];
    float rfixed;
    int has_fixed, fixed_coordsys;
    float fixedrdisk;
    float rigidrot;
    float drigidrot;
    double d2o[16], o2d[16];
    int has_d2o, has_o2d;

    /* style == EXTRAPOLATE ("-extrap coef0[,degree]") */
    /* Do polynomial extrapolation. */
    int degree;
    int coef0;	/* first fieldnumber of 3*degree terms: x',y',z', x'',y'',z'',... */

    /* style == PROJECT (from N dimensions to 3-D) */
    struct svvec sref;
    struct svvec sproj[3];
    struct svvec sadd;
    float sfadd[3];

    /* style == GALAXYRIDE */
    int gorb0;
    int rideWith;	/* default -1; if >=0, ride along with N'th star from source list */
    Matrix Tride;	/* p*Tride => p' -- to ride along with given speck */
    int has_Tride;	/* is Tride valid? */

    /* style == WARPSHEET */
      /* dy = sheetampl * x * exp( - (x/rcore)^2 ) * exp( - (z/routercore)^2 ) */
    float sheetampl;

    int valid;
    int locked;

    float tfrac, dtfrac;
    double realtime;
    float fixomega;

    struct speckcache slc[MAXDSPCTX];
};

static float fastsin2pi( float v );
static void inittables(void);
static struct specklist *warp_get_parti( struct dyndata *dd, struct stuff *st, double realtime );
static int warp_get_trange( struct dyndata *dd, struct stuff *st, double *tminp, double *tmaxp, int ready );
static int warp_parse_args( struct dyndata *dd, struct stuff *st, int argc, char **argv );
struct warpstuff *warp_setup( struct stuff *st, struct warpstuff *ws, int argc, char **argv, int *optindp );

static int warp_coordsys( const char *str ) {
    if(strchr(str, 'w')) return COORD_WORLD;
    if(strchr(str, 'o')) return COORD_OBJECT;
    if(strchr(str, 'd')) return COORD_DISK;
    return COORD_DISK;
}

int parsematrix( double tfm[16], char *str, const char *whose )
{
    double m[16];
    Matrix T;
    Point xyz;
    float aer[3];
    float scl = 1.0;
    int k = sscanf(str, 
      "%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf%*c%lf",
      &m[0],&m[1],&m[2],&m[3],
      &m[4],&m[5],&m[6],&m[7],
      &m[8],&m[9],&m[10],&m[11],
      &m[12],&m[13],&m[14],&m[15]);

    switch(k) {

    case 1:	/* simple scaling */
	memset(tfm, 0, 16*sizeof(double));
	tfm[0*4+0] = tfm[1*4+1] = tfm[2*4+2] = m[0];
	tfm[3*4+3] = 1;
	return 1;

    case 7:	/* virdir style Tx Ty Tz Rx Ry Rz Scl */
	scl = m[6];	/* and fall into ... */

    case 6:	/* virdir style Tx Ty Tz Rx Ry Rz */
	xyz.x[0] = m[0];
	xyz.x[1] = m[1];
	xyz.x[2] = m[2];
	aer[1] = m[3];
	aer[0] = m[4];
	aer[2] = m[5];
	xyzaer2tfm( &T, &xyz, aer );
	for(k = 0; k < 3*4; k++)
	    tfm[k] = T.m[k] * scl;
	for(k = 3*4; k < 4*4; k++)
	    tfm[k] = T.m[k];
	return 1;

    case 9:	/* 3x3 linear */
	memset(tfm, 0, 16*sizeof(double));
	for(k = 0; k < 3; k++) {
	    tfm[k*4+0] = m[k*3+0];
	    tfm[k*4+1] = m[k*3+1];
	    tfm[k*4+2] = m[k*3+2];
	}
	tfm[3*4+3] = 1;
	return 1;

    case 16:
	memcpy(tfm, m, 16*sizeof(double));
	break; /* great */

    default:
	if(whose)
	    msg("%s %s: expected 1 or 6 or 7 or 9 or 16 numbers", whose, str);
	return 0;
    }
    return 1;
}

static void svneed( struct svvec *sv, int need )
{
    if(need < sv->room)
	return;
    sv->room = 3 + 2 * (need > sv->room ? need : sv->room);
    sv->v = (struct cf *) (sv->v ? malloc( sv->room * sizeof(*sv->v) )
	    		     : realloc( sv->v, sv->room * sizeof(*sv->v) ));
    memset( &sv->v[sv->len], 0, (sv->room - sv->len) * sizeof(*sv->v) );
}

static void svinit( struct svvec *sv )
{
    memset(sv, 0, sizeof(*sv));
}

static void svfree( struct svvec *sv )
{
    if(sv && sv->v)
	free(sv->v);
}

struct cf *svvref( struct svvec *sv, int index )
{
    if(index < 0)
	return NULL;
    if(index >= sv->room)
	svneed( sv, index+1 );
    if(sv->len <= index)
	sv->len = index+1;
    return &sv->v[index];
}

static int projvec( struct warpstuff *ws, struct svvec *sv, int arg, int argc, char **argv )
{
    int ix, ok, a;
    char *cp, *ep;
    int err = 0;
    int i = 0;
    int coef = 0;
    int xindex = &(((struct speck *)0)->p.x[0]) - &(((struct speck *)0)->val[0]);

    svinit( sv );
    ws->style = PROJECT;
    sv->maxc = -1;
    a = arg;
    /*
     * expect:  [integer-or-fieldname ":"] float float ... {EOF | -<non-number>}
     */
    cp = argv[a];
    do {
	double v;
	char *colon;
	struct cf *cfp;

	while(*cp == ',' || *cp == ':' || isspace(*cp))
	    cp++;

	if(*cp == '\0') {
	    if(++a >= argc)
		break;
	    if(argv[a][0] == '-' && !(argv[a][1] == '.' || isdigit(argv[a][1])))
		break;
	    cp = argv[a];
	    continue;
	}

	colon = strpbrk(cp, ":, \t");
	if(colon && *colon == ':') {
	    *colon = '\0';
	    if(colon == cp+1 && *cp>='x' && *cp<='z') {
		coef = xindex + (*cp - 'x');
	    } else {
		err = !specks_set_byvariable( ws->st, cp, &coef );
	    }
	    *colon = ':';
	    if(err) {
		msg("warp %s: expected {<integer>|<fieldname>|x|y|z}: coef, coef, ..., not %s",
			argv[arg-1], cp);
		break;
	    }
	    cp = colon+1;
	    continue;
	}

        v = strtod(cp, &ep);

	if(ep == cp) {
	    err = 1;
	    break;
	}
	cp = ep;

	cfp = svvref( sv, i );
	cfp->f = v;
	cfp->c = coef;
	if(sv->maxc < coef)
	    sv->maxc = coef;
	i++;
	coef++;
	if(coef == xindex+3) coef = 0;

    } while(!err);

    return err ? argc+1 : a;
}

static void projadd( struct warpstuff *ws )
{
    int i, k, r;

    for(k = 0; k < 3; k++) {
	float v = ws->sadd.len>k ? ws->sadd.v[k].f : 0;
	for(r = 0; r < ws->sref.len; r++) {
	    int cref = ws->sref.v[r].c;
	    float fref = ws->sref.v[r].f;
	    struct svvec *sproj = &ws->sproj[k];
	    for(i = 0; i < sproj->len; i++) {
		if(sproj->v[i].c == cref)
		    v -= sproj->v[i].f * fref;
	    }
	}
	ws->sfadd[k] = v;
    }
}

static void warp_invalidate( struct stuff *st, struct warpstuff *ws )
{
    int i;
    st->dyn.slvalid = 0;
    ws->valid = 0;
    for(i=0; i<MAXDSPCTX; i++)
	ws->slc[i].tfrac = -MAXFLOAT;
}

void ws_init( struct warpstuff *ws )
{
  memset( ws, 0, sizeof(*ws) );
  ws->rideWith = -1;
}

struct warpstuff *warp_setup( struct stuff *st, struct warpstuff *oldws, int argc, char **argv, int *optindp )
{
  struct warpstuff *ws;
  struct warpstuff tws;
  int i, c;
  char key;
  float v;

  if(optindp) *optindp = 0;

  inittables();

  if(oldws)
    tws = *oldws;
  else
    ws_init( &tws );
  tws.st = st;

  for(i = 1; i < argc-1 && argv[i][0] == '-' && argv[i][1] != '\0'; i += 2) {
    char *optarg = argv[i+1];
    if(!strncmp(argv[i], "-w", 2) && argv[i][2] >= 'x' && argv[i][2] <= 'z') {
	i = projvec( &tws, &tws.sproj[ argv[i][2] - 'x' ], i+1, argc, argv ) - 2;

    } else if(!strcmp(argv[i], "-ref")) {
	i = projvec( &tws, &tws.sref, i+1, argc, argv ) - 2;

    } else if(!strcmp(argv[i], "-add")) {
	i = projvec( &tws, &tws.sadd, i+1, argc, argv ) - 2;
	if(tws.sadd.len != 3 || tws.sadd.v[0].c != 0 || tws.sadd.v[1].c != 1 || tws.sadd.v[2].c != 2) {
	    msg("warp -add: expected dx,dy,dz");
	    tws.sadd.len = 0;
	}

    } else if(!strncmp(argv[i], "-extrapolate", 3)) {
	char coef0name[32];
	tws.style = EXTRAPOLATE;
	tws.degree = 1;
	coef0name[0] = '\0';
	sscanf(optarg, "%31[^,],%d", coef0name, &tws.degree);
	if(!specks_set_byvariable( st, coef0name, &tws.coef0 )) {
	    tws.degree = 0;
	    if(0!=strcmp(coef0name, "off")) {
		msg("warp -extrap: expected fieldnumber or name, not \"%s\"", coef0name);
	    }
	}

    } else if(!strncmp(argv[i], "-sheetwarp", 6)) {
	int k = sscanf(optarg, "%f%*c%f%*c%f", &tws.sheetampl, &tws.rcore, &tws.routercore);
	if(k == 2)
	    tws.routercore = tws.rcore;
	if(k<2 || tws.rcore == 0 || tws.routercore == 0) {
	    msg("warp -sheetwarp: expected Yamplitude,Xscalelength,Zscalelength -- not \"%s\"", optarg);
	} else {
	    tws.style = SHEETWARP;
	}
    } else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "-trate")) {
	sscanf(optarg, "%f", &tws.period0);
    } else if(!strncmp(argv[i], "-seconds", 4)) {
	tws.tunit = 1.0;
    } else if(!strncmp(argv[i], "-frames", 3)) {
	tws.tunit = 30.0;
    } else if(!strcmp(argv[i], "-f")) {
	tws.has_trange = sscanf(optarg,"%f%*c%f",&tws.tfrom,&tws.tto);
    } else if(!strcmp(argv[i], "-z")) {
	tws.has_zerotime = sscanf(optarg, "%f", &tws.zerotime);
    } else if(!strcmp(argv[i], "-t")) {
	sscanf(optarg, "%f%*c%f", &tws.tfrac, &tws.dtfrac);
    } else if(!strncmp(argv[i], "-fix", 4)) {
	tws.has_fixed = 0;
	tws.fixed_coordsys = warp_coordsys( optarg );
	switch(sscanf(optarg, "%lf%*c%lf%*c%lf",
			&tws.fixedp[0], &tws.fixedp[1], &tws.fixedp[2])) {
	case 1: tws.has_fixed = 1; break;
	case 3: tws.has_fixed = 3; break;
	default:
	    msg("warp -fix {x,y,z|radius}[w] not %s", optarg);
	}
	
    } else if(!strcmp(argv[i], "-r")) {
	tws.core_coordsys = warp_coordsys( optarg );
	switch(sscanf(optarg, "%f%c%f", &tws.rcore, &key, &v)) {
	case 3:
	   if(key == '-') tws.routercore = v;
	   else tws.routercore = tws.rcore*(1+v);
	   break;
	case 1:
	   tws.routercore = tws.rcore * 1.1;
	   break;
	default:
	   argc = 0;
	}
    } else if(!strcmp(argv[i], "-T")) {
	tws.has_o2d = parsematrix( tws.o2d, optarg, "warp -T" );

    } else if(!strcmp(argv[i], "-F")) {
	tws.has_d2o = parsematrix( tws.d2o, optarg, "warp -F" );

    } else if(!strcmp(argv[i], "-rigidrot") || !strcmp(argv[i], "-R")) {
	tws.drigidrot = 0;
	sscanf(optarg, "%g%*c%g", &tws.rigidrot, &tws.drigidrot);	/* given as degrees */
	tws.rigidrot /= 360;	/* internally, kept as fraction of 1 turn */
	tws.drigidrot /= 360;

    } else if(!strncmp(argv[i], "-ride", 5)) {
	tws.rideWith = -1;
	tws.has_Tride = 0;
	sscanf(optarg, "%d", &tws.rideWith);

    } else if(!strncmp(argv[i], "-galaxy", 4)) {
	char gorb0name[32];

	tws.style = GALAXYRIDE;
	gorb0name[0] = '\0';
	sscanf(optarg, "%31s", gorb0name);
	if(!specks_set_byvariable( st, gorb0name, &tws.gorb0 )) {
	    tws.gorb0 = -1;
	    if(0!=strcmp(gorb0name, "off")) {
		msg("warp -extrap: expected fieldnumber or name or \"off\", not \"%s\"", gorb0name);
	    }
	}
    } else {
	msg("warp: unrecognized option %s", argv[i]);
	argc = -1;
	break;
    }
  }

  if(optindp)
    *optindp = i;

  if(argc < 3) {
    msg("Usage: %s  [-sheet ampl,xlength,zlength] [-extrap coef0[,degree]] [-f fin,fout][-p period0[f|s]][-z zerotime][-R rot[,drot]][-T o2d][-F d2o] [-r rcore[,transition][w]]] [-galaxy gorbcoef0] [-fix x,y,z[w]|radius[w]]", argv[0]);
    msg("-r : core rad. w/optional width of transition region (fraction of rcore)");
    msg("   : or (\"-\" separator) -r innercore-outercore");
    msg("-R(or -rigidrot) : add rigid rotation by \"rot\" degrees to entire body.");
    msg("-extrap coef0[,degree] : instead of above, extrapolate position by:");
    msg(" p=p0+(t/period0)*field[coef0..coef0+2]+(t/period0)^2*[coef0+3..coef0+5]+...");
    msg("-galaxy coef0   animated galaxy-like rotation.  coef0 is first of 8 fields -- see galaxyorbit.h");
    msg(" -ride <speckno>  ride along with i'th speck (0..N-1) in first loaded group");
    msg("-T, -F  sixteen numbers.  If only -T or -F is given, other is computed.");
    msg("  \"o2d\" = object-to-disk tfm, where disk center=0,0,0, in XZ plane.");
    msg("-fix x,y,z[w]  or  -fix r[w] (fix point or radius); \"w\" = \"world coords\".");
    msg(" sense: p' = p * o2d * warp(X,Z plane) * d2o");
    return NULL;
  }

  if(tws.style == PROJECT)
      projadd( &tws );		/* fill in sfadd[] terms: sum of sadd[] and sref dot sproj */

  ws = oldws ? oldws : NewN( struct warpstuff, 1 );
  if(tws.tunit == 0)
      tws.tunit = (tws.style == DIFFROT) ? 30.0 : 1.0;	/* ugly backward compat. */
  *ws = tws;
  warp_invalidate( st, ws );		/* Force recomputing parameters */
  return ws;
}

float slerp( float frac, float from, float to )
{
  if(frac<=0) return from;
  if(frac>=1) return to;
  return from + frac*frac*(3-2*frac)*(to-from);
}

void windup( struct warpstuff *ws, Point *out, CONST Point *in )
{
  /*
   * v(r) =  (vmax/rcore)*r  r in 0..rcore
   *         vmax	      r > rcore
   * period(r) = 2pi/(vmax/rcore)  r<rcore  [Call this T0.]
   *		 2pi/(vmax/r) = (r/rcore)*T0
   * theta(t/T0,r) = (t/T0)*T0*(r < rcore ? 1 : rcore/r)
   * So rotate in the XZ plane about (0,0,0) by theta(t/T0,r)
   */
    float x = in->x[0];
    float z = in->x[2];
    float r = hypotf( x, z );
    float omega =  (r<=ws->rcoredisk) ? 1
		: (r>=ws->routercoredisk) ? ws->routercoredisk/r
		: slerp( 1+(r-ws->routercoredisk)/(ws->routercoredisk-ws->rcoredisk),
			1, ws->routercoredisk/r );
    float theta, s, c;

    theta = (omega - ws->fixomega) * ws->tfrac + ws->rigidrot;
    s = fastsin2pi( theta );
    c = fastsin2pi( theta + 0.25 );

    out->x[0] = x*c - z*s;
    out->x[1] = in->x[1];
    out->x[2] = x*s + z*c;
}

void dvtfmpoint( Point *dst, CONST Point *src, CONST double T[16] )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T[i] + src->x[1]*T[i+4] + src->x[2]*T[i+8] + T[i+12];
}

void windup_o2d( struct warpstuff *ws, Point *out, CONST Point *in ) {
    Point tin, tout;

    dvtfmpoint( &tin, in, ws->o2d );
    windup( ws, &tout, &tin );
    dvtfmpoint( out, &tout, ws->d2o );
}

void sheetwarp( struct warpstuff *ws, Point *xyzout, CONST Point *xyz )
{
  /*
   * v(r) =  (vmax/rcore)*r  r in 0..rcore
   *         vmax             r > rcore
   * period(r) = 2pi/(vmax/rcore)  r<rcore  [Call this T0.]
   *             2pi/(vmax/r) = (r/rcore)*T0
   * theta(t/T0,r) = (t/T0)*T0*(r < rcore ? 1 : rcore/r)
   * So rotate in the XZ plane about (0,0,0) by theta(t/T0,r)
   */
   float sx = xyz->x[0] / ws->rcore;
   float sz = xyz->x[2] / ws->routercore;

   xyzout->x[0] = xyz->x[0];
   xyzout->x[1] = xyz->x[1] + sx * ws->sheetampl * expf( -sx*sx ) * expf( -sz*sz );
   xyzout->x[2] = xyz->x[2];
}

void sheetwarp_o2d( struct warpstuff *ws, Point *out, CONST Point *in ) {
    Point tin, tout;

    dvtfmpoint( &tin, in, ws->o2d );
    sheetwarp( ws, &tout, &tin );
    dvtfmpoint( out, &tout, ws->d2o );
}

void extrap( struct warpstuff *ws, Point *out, CONST Point *in, CONST struct speck *sp, int deg )
{
    int i;
    float t = ws->tfrac;
    CONST float *coefval = &sp->val[ws->coef0];
    if(deg == 1) {
	out->x[0] = in->x[0] + coefval[0]*t;
	out->x[1] = in->x[1] + coefval[1]*t;
	out->x[2] = in->x[2] + coefval[2]*t;
    } else {
	*out = *in;
	for(i = 0; i < deg; i++, coefval += 3, t *= ws->tfrac) {
	    out->x[0] += coefval[0]*t;
	    out->x[1] += coefval[1]*t;
	    out->x[2] += coefval[2]*t;
	}
    }
}

void extrap_o2d( struct warpstuff *ws, Point *out, CONST struct speck *sp, int deg )
{
    Point tin, tout;

    dvtfmpoint( &tin, &sp->p, ws->o2d );
    extrap( ws, &tout, &tin, sp, deg );
    dvtfmpoint( out, &tout, ws->d2o );
}

void animgalaxy( struct warpstuff *ws, Point *out, CONST Point *in, CONST struct speck *sp );

void prewarpspecklist( struct warpstuff *ws, struct stuff *st, CONST struct specklist *osl )
{
     ws->has_Tride = 0;
     if(ws->style == GALAXYRIDE && ws->rideWith >= 0) {
	CONST struct specklist *sl, *final;
	int ntotal = 0;
	final = NULL;
	for(sl = osl; sl; sl = sl->next) {
	    if(sl->text == 0 && sl->nspecks > 0 && ws->gorb0 + 8 <= SPECKMAXVAL(sl)) {
		final = sl;
		ntotal += sl->nspecks;
	    }
	}
	/* It needs to be part of the last nonempty speck bundle or we're screwed */
	if(final != NULL && ws->rideWith < final->nspecks) {
	    /* OK, compute its position */
	    struct speck *sp = NextSpeck( final->specks, final, ws->rideWith );
	    Point tfrom, tto;
	    Point ttoxy;
	    Point alignxy;
	    Point shiftby;

	    if(ws->has_o2d) {
		dvtfmpoint( &tfrom, &sp->p, ws->o2d );
	    } else {
		tfrom = sp->p;
	    }
	    animgalaxy( ws, &tto, &tfrom, sp );
	    /* Compute riding-along transformation.
	     * Rotate tto.{x,y,0} to align with unwarped in-disk-plane vector,
	     * and translate tto to its original un-warped position.
	     */
	    ttoxy = tto;  ttoxy.x[2] = 0;
	    alignxy = sp->p; alignxy.x[2] = 0;
	    grotation( &ws->Tride, &ttoxy, &alignxy );
	    vtfmpoint( &shiftby, &tto, &ws->Tride );
	    vsub( &shiftby, &sp->p, &shiftby );
	    vsettranslation( &ws->Tride, &shiftby );
	    ws->has_Tride = 1;
	}
    }
}
  
void animgalaxy( struct warpstuff *ws, Point *out, CONST Point *in, CONST struct speck *sp )
{
  /* Uses ws globals:
   *	tfrac (Myr)
   *	dtfrac (delta tfrac)
   *	zomega (angular freq of vertical oscillations, radians/Myr)
   *	Romegapc (angular freq of longitudinal orbit):
   *	    Romega = Romegapc / Rg
   *	    Having Romegapc as a constant implies vLSR = const (flat rotation curve)
   */
   GalOrbit *gorb = (GalOrbit *)&sp->val[ ws->gorb0 ];
   float zc, zs;        /* altitude sincos */
   float ec, es;        /* epicycle sincos */
   float gc, gs;	/* Rg circle sincos */
   float xEp, yEp;      /* epicycle offsets from Rg generating circle */
   float Romega = gorb->Romegapc / gorb->Rg;
   float xEpRg;
   float Rge;

   sincosf( gorb->zphase + ws->tfrac * gorb->zomega, &zs, &zc );
   sincosf( gorb->Rphase0 + ws->tfrac * Romega, &es, &ec );
   sincosf( gorb->Rgphase + ws->tfrac * Romega, &gs, &gc );

   Rge = gorb->Rg * gorb->ecc;
   xEp = ec * Rge;
   yEp = 2.0f * es * Rge;

  /*
   x_generatingcircle = -Rg * cos( phi_g )
   y_generatingcircle =  Rg * sin( phi_g )
   
   To that, add [ xEp, yEp ], rotating just as 
   the generating basis does: xEp is positive
   toward the Milky Way center, and yEp is positive
   toward the generating circle's forward tangent.
   Result:

     x = x_circle + (xEp * gc - yEp * gs)
     y = y_circle + (yEp * gc + xEp * gs)
   */
   
   out->x[0] = (xEp - gorb->Rg) * gc + yEp * gs;
   out->x[1] = yEp * gc - (xEp - gorb->Rg) * gs;
   out->x[2] = gorb->zmax * zs;

   if(ws->has_Tride) {	/* apply riding-along transform */
     Point o = *out;
     vtfmpoint( out, &o, &ws->Tride );
   }

#if USE_DELTA
  /*
    xEp' = -Romega Rge es
    yEp' = 2 Romega Rge ec
    gs' = Romega gc
    gc' = -Romega gs

    x'	= Romega * (Rge ec gs + Rge gc es + Rg gs)
    y'	= Romega * (Rge ec gc - Rge es gs + Rg gc)
   */
    dout->x[0] = dtfrac * Romega * (Rge * (ec*gs + gc*es) + gorb->Rg * gs);
    dout->x[1] = dtfrac * Romega * (Rge * (ec*gc - es*gs) + gorb->Rg * gc);
    dout->x[2] = dtfrac * gorb->zomega * zc * gorb->zmax;
#endif
}


void animgalaxy_o2d( struct warpstuff *ws, Point *out, struct speck *sp )
{
    Point tin, tout;
    int i;

    dvtfmpoint( &tin, &sp->p, ws->o2d );
    animgalaxy( ws, &tout, &tin, sp );
    dvtfmpoint( out, &tout, ws->d2o );
}

void warpspecks( struct warpstuff *ws,
		 struct stuff *st,
		 struct specklist *osl,
		 struct specklist *sl )
{
  struct speck *osp, *sp;
  int i, n;

  if(osl->specks == NULL) return;
  n = osl->nspecks;
  /* memcpy( sl->specks, osl->specks, osl->bytesperspeck * n ); */
  osp = osl->specks;
  sp = sl->specks;
  switch(ws->style) {
  case EXTRAPOLATE: {
	int deg = ws->degree;
	int excess = SMALLSPECKSIZE( ws->degree*3 + ws->coef0 ) - sl->bytesperspeck;
	if(excess > 0)
	    deg -= (excess+2) / 3;
	    
	for(i = 0; i < n; i++) {
	    if(ws->has_o2d)
		extrap_o2d( ws, &sp->p, osp, deg );
	    else
		extrap( ws, &sp->p, &osp->p, osp, deg );
	    osp = NextSpeck(osp, osl, 1);
	    sp = NextSpeck(sp, sl, 1);
	}
    }
    break;

  case DIFFROT:
    for(i = 0; i < n; i++) {
	if(ws->has_o2d)
	    windup_o2d( ws, &sp->p, &osp->p );
	else
	    windup( ws, &sp->p, &osp->p );
	osp = NextSpeck(osp, osl, 1);
	sp = NextSpeck(sp, sl, 1);
    }
    break;

  case SHEETWARP:
    for(i = 0; i < n; i++) {
	if(ws->has_o2d)
	    sheetwarp_o2d( ws, &sp->p, &osp->p );
	else
	    sheetwarp( ws, &sp->p, &osp->p );
	osp = NextSpeck(osp, osl, 1);
	sp = NextSpeck(sp, sl, 1);
    }
    break;

  case GALAXYRIDE:
    if(ws->coef0 >= 0 && ws->coef0 + 8 <= SPECKMAXVAL(osl)) {
	for(i = 0; i < n; i++) { 
	    if(ws->has_o2d)
		animgalaxy_o2d( ws, &sp->p, osp );
	    else
		animgalaxy( ws, &sp->p, &osp->p, osp );
	    osp = NextSpeck(osp, osl, 1);
	    sp = NextSpeck(sp, sl, 1);
	}
    }
    break;

  case PROJECT: {
	int k;
	int maxval = SPECKMAXVAL(osl);
	int maxcoef = ws->sproj[0].maxc > ws->sproj[1].maxc ? ws->sproj[0].maxc : ws->sproj[1].maxc;
	if(maxcoef < ws->sproj[2].maxc)
	    maxcoef = ws->sproj[2].maxc;

	if(maxcoef >= maxval) {
	    /* Can't transform, or identity tfm */
	    for(i = 0; i < n; i++) {
		sp->p = osp->p;
		osp = NextSpeck(osp, osl, 1);
		sp = NextSpeck(sp, sl, 1);
	    }
	} else {
	    for(i = 0; i < n; i++) {
		for(k = 0; k < 3; k++) {
		    struct svvec *sproj = &ws->sproj[k];
		    float *oval = &osp->val[0];
		    float v = ws->sfadd[k];
		    if(sproj->len == 0) {
			v += osp->p.x[k];
		    } else {
			int m;
			for(m = 0; m < sproj->len; m++)
			    v += osp->val[ sproj->v[m].c ] * sproj->v[m].f;
		    }
		    sp->p.x[k] = v;
		}
		osp = NextSpeck(osp, osl, 1);
		sp = NextSpeck(sp, sl, 1);
	    }
	}
    }
    break;

  default:
    msg("warp: warped style %d", ws->style);
  }
}

static float warpfrac( struct warpstuff *ws, double time )
{
  if(ws->has_trange == 2 && ws->tfrom != ws->tto) {
    return slerp( (time - ws->tfrom) / (ws->tto - ws->tfrom), -1, 1 );
  } else if(ws->period0 != 0) {
    return (time - ws->tfrom) / ws->period0;
  } else {
    return 0;
  }
}

static double dvlength( double v[3] ) {
  return sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
}

static void ddmmul( double dest[16], Matrix *a, double b[16] ) {
    int i,j;
    for(i = 0; i < 4; i++) {
	for(j = 0; j < 4; j++) {
	    dest[i*4+j]	= a->m[i*4+0]*b[0*4+j]
			+ a->m[i*4+1]*b[1*4+j]
			+ a->m[i*4+2]*b[2*4+j]
			+ a->m[i*4+3]*b[3*4+j];
	}
    }
}

static void dvmmul( double dest[3], double src[3], double T[16] ) {
    double w = src[0]*T[0*4+3] + src[1]*T[1*4+3] + src[2]*T[2*4+3] + T[3*4+3];
    w = 1/w;
    dest[0] = (src[0]*T[0*4+0] + src[1]*T[1*4+0] + src[2]*T[2*4+0] + T[3*4+0])*w;
    dest[1] = (src[0]*T[0*4+1] + src[1]*T[1*4+1] + src[2]*T[2*4+1] + T[3*4+1])*w;
    dest[2] = (src[0]*T[0*4+2] + src[1]*T[1*4+2] + src[2]*T[2*4+2] + T[3*4+2])*w;
}

static int getT2d( double T2d[16], struct stuff *st, struct warpstuff *ws, int coordsys )
{
    Matrix o2w, w2o;
    switch(coordsys) {
    case COORD_DISK:
	return 0;
    case COORD_OBJECT:
	if(!ws->has_o2d)
	    return 0;
	memcpy( T2d, ws->o2d, 16*sizeof(double) );
	return 1;
    case COORD_WORLD:
	parti_geto2w( st, parti_idof(st), &o2w );
	eucinv( &w2o, &o2w );
	if(ws->has_o2d) {
	    ddmmul( T2d, &w2o, ws->o2d );
	} else {
	    int i;
	    for(i = 0; i < 4*4; i++)
		T2d[i] = w2o.m[i];
	}
	return 1;
    }
    msg("warp getT2d %d", coordsys);
    return 0;
}

static double rtodisk( struct stuff *st, struct warpstuff *ws, double radius, int coordsys ) {
  double w2d[16];
  switch(coordsys) {
  case COORD_DISK:
	return radius;
  case COORD_OBJECT:
	return (ws->has_o2d ?  radius * dvlength( &ws->o2d[0] ) : radius);
  case COORD_WORLD:
	getT2d( w2d, st, ws, COORD_WORLD );
	return radius * dvlength( &w2d[0] );
  }
  msg("warp: rtodisk");
  return 0;
}

/* Delay computing transformable quantities until we really need them */
static void setup_coords( struct stuff *st, struct warpstuff *ws ) {
    double fixed2d[16];
    double fixedpdisk[3];

    if(!ws->valid) {
	ws->rcoredisk = rtodisk( st, ws, ws->rcore, ws->core_coordsys );
	ws->routercoredisk = rtodisk( st, ws, ws->routercore, ws->core_coordsys );
	switch(ws->has_fixed) {
	case 1:
	    ws->fixedrdisk = rtodisk( st, ws, ws->fixedp[0], ws->fixed_coordsys );
	    break;
	case 3:
	    if(getT2d( fixed2d, st, ws, ws->fixed_coordsys )) {
		dvmmul( fixedpdisk, ws->fixedp, fixed2d );
		ws->fixedrdisk = hypot( fixedpdisk[0], fixedpdisk[2] );
	    } else {
		ws->fixedrdisk = hypot( ws->fixedp[0], ws->fixedp[2] );
	    }
	    break;
	}
	ws->valid = 1;
    }
    if(ws->has_fixed) {
	ws->fixomega =
	    (ws->fixedrdisk <= ws->rcoredisk) ? 1
	    : (ws->fixedrdisk >= ws->routercoredisk) ?
			ws->routercoredisk / ws->fixedrdisk
	    : slerp( 1 + (ws->fixedrdisk - ws->routercoredisk)
			/ (ws->routercoredisk - ws->rcoredisk),
		 1, ws->routercoredisk / ws->fixedrdisk );
    } else {
	ws->fixomega = 0;
    }
}


#define MAXINTERP 1024

float sintbl[MAXINTERP+1];

static void inittables(void) {
  int i;

  for(i = 0; i <= MAXINTERP; i++)
    sintbl[i] = sinf(2*M_PI*i / MAXINTERP);
}

/* fastsin2pi( x ) returns approximately sin( 2*pi*x ),
 * i.e. sin(x) if x is given as a fraction of a full 360-degree turn.
 */
static float fastsin2pi( float v ) {
  int i;
  while(v < 0) v += 1;
  while(v > 1) v -= 1;
  v *= MAXINTERP;
  i = (int)v;
  return sintbl[i] + (v - i)*(sintbl[i+1]-sintbl[i]);
}

void deucinv( double dst[16], CONST double src[16] )
{
    int i, j;
    double s = src[0]*src[0] + src[1]*src[1] + src[2]*src[2];
    Point trans;
    double t[16];
    if(src == dst) {
	memcpy( t, src, sizeof(t) );
	src = t;
    }
    for(i = 0; i < 3; i++) {
	for(j = 0; j < 3; j++)
	    dst[i*4+j] = src[j*4+i] / s;
	dst[i*4+3] = 0;
    }
    for(i = 0; i < 3; i++)
	dst[3*4+i] = -(src[3*4+0]*dst[0*4+i] + src[3*4+1]*dst[1*4+i] + src[3*4+2]*dst[2*4+i]);
    dst[3*4+3] = 1;
}


#ifndef STANDALONE

static char *projvec2str( struct stuff *st, char *str, char *end, char *prefix, struct svvec *sv )
{
    int xindex = &(((struct speck *)0)->p.x[0]) - &(((struct speck *)0)->val[0]);
    int len = prefix ? strlen(prefix) : 0;
    char *pre = prefix;
    int i;

    if(sv == NULL || sv->len == 0)
	return str;
    if(str == NULL || str+len+4 >= end)
	return NULL;
    for(i = 0; i < sv->len; i++) {
	int c = sv->v[i].c;
        struct valdesc *vd;
    	if(c>=xindex && c<=xindex+2) {
	    str += snprintf(str, end-str-1, "%s %c:%g", pre, c-xindex+'x', sv->v[i].f);
	} else if(c >= 0 && c < MAXVAL && (vd = &st->vdesc[st->curdata][c])->name[0] != '\0') {
	    str += snprintf(str, end-str-1, "%s %s:%g", pre, vd->name, sv->v[i].f);
	} else {
	    str += snprintf(str, end-str-1, "%s %d:%g", pre, c, sv->v[i].f);
	}
	pre = ",";
    }
    *str = '\0';
    return str;
}


static int warp_parse_args( struct dyndata *dd, struct stuff *st, int argc, char **argv )
{
  struct warpstuff *ws;
  CONST char *op;
  if(argc < 1 || 0!=strcmp(argv[0], "warp"))
    return 0;
  if(dd == NULL || (ws = (struct warpstuff *)dd->data) == NULL) {
    msg("No warp enabled");
    return 1;
  }

  if(argc>1) {
      if(!strcmp(argv[1], "lock")) {
	st->dyn.enabled = 1;
	warp_invalidate( st, ws );	/* recompute just in case */
	ws->locked = 1;

      } else if(!strcmp(argv[1], "off")) {
	st->dyn.enabled = -1;	/* defined, but not active */
	warp_invalidate( st, ws );	/* force redisplay */

      } else if((!strcmp(argv[1], "unlock") || !strcmp(argv[1], "on"))) {
	st->dyn.enabled = 1;
	warp_invalidate( st, ws );	/* force recomputing */
	ws->locked = 0;
      }
  }

  op = ws->locked ? "locked " : st->dyn.enabled<0 ? "off " : "";

  switch(ws->style) {
     case EXTRAPOLATE:
	msg("warp %stfrac %g at time %g  extrapolate degree %d using fields %d..%d",
	    op,
	    ws->tfrac, ws->realtime,
	    ws->degree, ws->coef0, ws->coef0 + ws->degree*3 - 1);
	break;
    case DIFFROT:
	msg("warp %stfrac %g at time %g  fixp %g %g %g rfixd %g fixsys %d rcored %g,%g",
	    op,
	    ws->tfrac, ws->realtime,
	    ws->fixedp[0],ws->fixedp[1],ws->fixedp[2], ws->fixedrdisk, ws->fixed_coordsys, ws->rcoredisk, ws->routercoredisk);
	break;
    case GALAXYRIDE:
	msg("warp %stfrac %g at time %g  -galaxy %d",
	    op,
	    ws->tfrac, ws->realtime,
	    ws->coef0);
	break;
    case PROJECT: {
	    char str[200], *cp;
	    sprintf(str, "warp %s", op);
	    cp = str+strlen(str);

	    cp = projvec2str( st, cp, &str[sizeof(str)], " -wx", &ws->sproj[0] );
	    cp = projvec2str( st, cp, &str[sizeof(str)], " -wy", &ws->sproj[1] );
	    cp = projvec2str( st, cp, &str[sizeof(str)], " -wz", &ws->sproj[2] );
	    cp = projvec2str( st, cp, &str[sizeof(str)], " -ref", &ws->sref );
	    if(ws->sadd.len > 0) {
		cp += snprintf( cp, &str[sizeof(str)] - cp, " -add %g,%g,%g",
			ws->sadd.v[0].f, ws->sadd.v[1].f, ws->sadd.v[2].f);
	    }
	    msg(str);
        }
	break;
    case SHEETWARP:
	msg("warp %syampl %g xlength %g zlength %g", op, ws->sheetampl, ws->rcore, ws->routercore);
	break;
  }
  return 1;
}

int warp_get_trange( struct dyndata *dd, struct stuff *st, double *tminp, double *tmaxp, int ready )
{
  *tminp = -10000;
  *tmaxp = 10000;
  return 1;
}

static void scrapsl( struct specklist **slp ) {
  struct specklist *sl, *slnext;
  if(slp == NULL) return;
  sl = *slp;
  *slp = NULL;
  for( ; sl; sl = slnext) {
    slnext = sl->next;
    if(sl->specks) Free(sl->specks);
    Free(sl);
  }
}


int warp_read( struct stuff **stp, int argc, char *argv[], char *fname, void *etc ) {
    struct stuff *st = *stp;
    struct warpstuff *ws;

    if(argc < 1 || strcmp(argv[0], "warp") != 0)
	return 0;
    st->clk->continuous = 1;

    if(st->dyn.ctlcmd == warp_parse_args) {
	ws = (struct warpstuff *)st->dyn.data;
    } else {
	ws = NULL;
    }
    ws = warp_setup( st, ws, argc, argv, NULL );
    if(ws == NULL)
	return 0;

    memset(&st->dyn, 0, sizeof(st->dyn));
    st->dyn.data = ws;
    st->dyn.getspecks = warp_get_parti;
    st->dyn.trange = warp_get_trange;
    st->dyn.ctlcmd = warp_parse_args;
    st->dyn.draw = NULL;
    st->dyn.free = NULL;
    st->dyn.enabled = 1;
    warp_invalidate( st, ws );
    return 1;
}

void warp_init() {
    parti_add_reader( warp_read, "warp", NULL );
}

struct specklist *
warp_get_parti( struct dyndata *dd, struct stuff *st, double arealtime )
{
  struct warpstuff *ws = (struct warpstuff *)dd->data;
  int ctx = get_dsp_context();
  struct speckcache *sc = &ws->slc[ctx];
  struct specklist *sl, *osl, **slp;
  static int once = 1;
  int stepno;
  double realtime = arealtime;

#if unix
  if(once-- == 0 && getenv("DBG")) {
    fprintf(stderr, "warp_get_parti:   dbx -p %d\n", getpid());
    sleep(atoi(getenv("DBG")));
  }
#endif

  if(ws->locked && ws->valid && sc != NULL)
    return sc->sl;

  setup_coords( st, ws );	/* bind coordsys-related stuff now */

  stepno = realtime<0 ? 0 : (int)floorf( realtime );	/* In case there are multiple timesteps, find the nearest below current */
  if(stepno >= st->ntimes)
    stepno = st->ntimes-1;

  realtime -= stepno;

  ws->realtime = realtime;
  ws->tfrac = warpfrac(ws, realtime * ws->tunit);
  if(ws->has_zerotime)
    ws->tfrac -= warpfrac(ws, ws->zerotime);
  if(ws->has_trange && ws->period0 != 0)
    ws->tfrac *= (ws->tto - ws->tfrom) / ws->period0;

  if(ws->has_o2d && !ws->has_d2o) {
    deucinv( ws->d2o, ws->o2d );
    ws->has_d2o = -1;
  } else if(!ws->has_o2d && ws->has_d2o) {
    deucinv( ws->o2d, ws->d2o );
    ws->has_o2d = -1;
  }


  if(getenv("WDBG")) printf("%g %g %g # rt frac frac(rt)\n", realtime, ws->tfrac, warpfrac(ws, realtime*ws->tunit));

  if(sc->sl != NULL && sc->tfrac == ws->tfrac && sc->realtime == arealtime)
    return sc->sl;

  osl = specks_timespecks( st, 0, stepno );
  slp = &sc->sl;

  prewarpspecklist( ws, st, osl );
  for(sl = *slp; osl != NULL; osl = osl->next, slp = &sl->next, sl = *slp) {
    if(sl == NULL || sl->speckseq != osl->speckseq) {
	scrapsl(slp);
	sl = NewN( struct specklist, 1 );
	*sl = *osl;
	sl->specks = NULL;
	sl->next = NULL;
	if(osl->specks) {
	    int len = osl->bytesperspeck * osl->nspecks;
	    sl->specks = (struct speck *)NewN( char, len );
	    memcpy( sl->specks, osl->specks, len );
	}
	*slp = sl;
    }
    warpspecks( ws, st, osl, sl );
  }

  sc->tfrac = ws->tfrac;
  sc->realtime = arealtime;	/* remember original realtime so we
				 * can check validity.  tfrac isn't enough --
				 * we might use same tfrac with multiple
				 * source-data timesteps
				 */
  return sc->sl;
}

#else /*STANDALONE*/

#include <stdarg.h>
#include "stardef.h"

int msg( CONST char *fmt, ... ) {
  char str[10240];

  va_list args;
  va_start(args, fmt);
  vsprintf(str, fmt, args);
  va_end(args);

  return fprintf(stderr, "%s\n", str);
}

int parti_idof( struct stuff *stjunk ) {
  return 1;
}

void parti_geto2w( struct stuff *stjunk, int objid, Matrix *o2w ) {
  int k;
  char *tfm = getenv("tfm");
  Matrix T;
  Point xyz;
  float aer[3], scl;
  if(tfm == NULL) {
    msg("warpsdb: need object-to-world transform, but no \"tfm\" env variable set?");
    exit(1);
  }
  k = sscanf(tfm,
"%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c%f",
    &T.m[0], &T.m[1], &T.m[2], &T.m[3],
    &T.m[4], &T.m[5], &T.m[6], &T.m[7],
    &T.m[8], &T.m[9], &T.m[10], &T.m[11],
    &T.m[12], &T.m[13], &T.m[14], &T.m[15]);

  scl = 1;

  switch(k) {
  case 1:
    scl = T.m[0];
    mscaling( o2w, scl,scl,scl );
    break;

  case 7:
    scl = T.m[6];  /* and fall into... */
  case 6:
    xyz.x[0] = T.m[0]; xyz.x[1] = T.m[1]; xyz.x[2] = T.m[2];
    aer[1] = T.m[3];   aer[0] = T.m[4];   aer[2] = T.m[5];
    xyzaer2tfm( &T, &xyz, aer );
    if(scl == 1) {
	*o2w = T;
    } else {
	Matrix Ts;
	mscaling( &Ts, scl,scl,scl );
	mmmul( o2w, &Ts, &T );
    }
    break;
  case 16:
    *o2w = T;
    break;
  default:
    msg("warpsdb: \"tfm\"=%s; expected 1 or 6 or 7 or 16 numbers", tfm);
    exit(1);
  }
}


/* stub. */
int specks_set_byvariable( struct stuff *st, char *str, int *val )
{
  return 0;
}


void dwindup( struct warpstuff *ws, float *xyzout, float *dxyzout, CONST float *xyz )
{
  /*
   * v(r) =  (vmax/rcore)*r  r in 0..rcore
   *         vmax             r > rcore
   * period(r) = 2pi/(vmax/rcore)  r<rcore  [Call this T0.]
   *             2pi/(vmax/r) = (r/rcore)*T0
   * theta(t/T0,r) = (t/T0)*T0*(r < rcore ? 1 : rcore/r)
   * So rotate in the XZ plane about (0,0,0) by theta(t/T0,r)
   */
   float x = xyz[0];
   float z = xyz[2];
   float r = hypot( x, z );
   float omega = ( (r<=ws->rcoredisk) ? 1
		: (r>=ws->routercoredisk) ? ws->routercoredisk/r
		: slerp( 1 + (r-ws->routercoredisk)/(ws->routercoredisk-ws->rcoredisk),
			1, ws->routercoredisk/r )
		) - ws->fixomega;
 
   float theta = omega * ws->tfrac + ws->rigidrot;
   float dtheta = (omega * ws->dtfrac + ws->drigidrot) * 2*M_PI;
   float s = fastsin2pi( theta );
   float c = fastsin2pi( theta + 0.25 );

   xyzout[0] = x*c - z*s;
   xyzout[1] = xyz[1];
   xyzout[2] = x*s + z*c;

   dxyzout[0] = -dtheta * (x*s + z*c);
   dxyzout[1] = 0;
   dxyzout[2] = dtheta * (x*c - z*s);
}

void dwindup_o2d( struct warpstuff *ws, float *xyzout, float *dxyzout, CONST float *xyz )
{
    float v[3], w[3], dw[3];
    int i;

    for(i = 0; i < 3; i++)
	v[i] = xyz[0]*ws->o2d[0*4+i] + xyz[1]*ws->o2d[1*4+i]
	     + xyz[2]*ws->o2d[2*4+i] + ws->o2d[3*4+i];
    dwindup( ws, w, dw, v );
    for(i = 0; i < 3; i++) {
	xyzout[i] = w[0]*ws->d2o[0*4+i] + w[1]*ws->d2o[1*4+i]
		  + w[2]*ws->d2o[2*4+i] + ws->d2o[3*4+i];
	dxyzout[i] = dw[0]*ws->d2o[0*4+i] + dw[1]*ws->d2o[1*4+i]
		   + dw[2]*ws->d2o[2*4+i];
    }
}

void dsheetwarp( struct warpstuff *ws, float *xyzout, float *dxyzout, CONST float *xyz )
{
  /*
   * v(r) =  (vmax/rcore)*r  r in 0..rcore
   *         vmax             r > rcore
   * period(r) = 2pi/(vmax/rcore)  r<rcore  [Call this T0.]
   *             2pi/(vmax/r) = (r/rcore)*T0
   * theta(t/T0,r) = (t/T0)*T0*(r < rcore ? 1 : rcore/r)
   * So rotate in the XZ plane about (0,0,0) by theta(t/T0,r)
   */
   float sx = xyz[0] / ws->rcore;
   float sz = xyz[2] / ws->routercore;

   xyzout[0] = xyz[0];
   xyzout[1] = xyz[1] + sx * ws->sheetampl * expf( -sx*sx ) * expf( -sz*sz );
   xyzout[2] = xyz[2];

   dxyzout[0] = 0;
   dxyzout[1] = 0;
   dxyzout[2] = 0;
}

void dsheetwarp_o2d( struct warpstuff *ws, float *xyzout, float *dxyzout, CONST float *xyz )
{
    float v[3], w[3], dw[3];
    int i;

    for(i = 0; i < 3; i++)
	v[i] = xyz[0]*ws->o2d[0*4+i] + xyz[1]*ws->o2d[1*4+i]
	     + xyz[2]*ws->o2d[2*4+i] + ws->o2d[3*4+i];
    dsheetwarp( ws, w, dw, v );
    for(i = 0; i < 3; i++) {
	xyzout[i] = w[0]*ws->d2o[0*4+i] + w[1]*ws->d2o[1*4+i]
		  + w[2]*ws->d2o[2*4+i] + ws->d2o[3*4+i];
	dxyzout[i] = dw[0]*ws->d2o[0*4+i] + dw[1]*ws->d2o[1*4+i]
		   + dw[2]*ws->d2o[2*4+i];
    }
}

void dextrap( struct warpstuff *ws, float *xyzout, float *dxyzout,
		CONST float *xyz, CONST struct speck *sp, int deg )
{
    float *coefval = &sp->val[ws->coef0];
    if(deg == 1) {
	xyzout[0] = xyz[0] + coefval[0]*ws->tfrac;
	xyzout[1] = xyz[1] + coefval[1]*ws->tfrac;
	xyzout[2] = xyz[2] + coefval[2]*ws->tfrac;
	dxyzout[0] = coefval[0]*ws->dtfrac;
	dxyzout[1] = coefval[1]*ws->dtfrac;
	dxyzout[2] = coefval[2]*ws->dtfrac;
    } else {
	int i;
	float t = ws->tfrac;
	float tprime = ws->tfrac + ws->dtfrac;
	xyzout[0] = xyz[0];
	xyzout[1] = xyz[1];
	xyzout[2] = xyz[2];
	dxyzout[0] = dxyzout[1] = dxyzout[2] = 0;
	for(i = 0; i < deg; i++, coefval += 3) {
	    xyzout[0] += coefval[0]*t;
	    xyzout[1] += coefval[1]*t;
	    xyzout[2] += coefval[2]*t;
	    dxyzout[0] += coefval[0]*(tprime-t);
	    dxyzout[1] += coefval[1]*(tprime-t);
	    dxyzout[2] += coefval[2]*(tprime-t);
	    t *= ws->tfrac;
	    tprime *= (ws->tfrac + ws->dtfrac);
	}
    }
}

void dextrap_o2d( struct warpstuff *ws, float *xyzout, float *dxyzout,
		CONST struct speck *sp, int deg )
{
    float v[3], w[3], dw[3];
    int i;

    for(i = 0; i < 3; i++)
	v[i] = sp->p.x[0]*ws->o2d[0*4+i] + sp->p.x[1]*ws->o2d[1*4+i]
	     + sp->p.x[2]*ws->o2d[2*4+i] + ws->o2d[3*4+i];
    dextrap( ws, w, dw, v, sp, deg );
    for(i = 0; i < 3; i++) {
	xyzout[i] = w[0]*ws->d2o[0*4+i] + w[1]*ws->d2o[1*4+i]
		  + w[2]*ws->d2o[2*4+i] + ws->d2o[3*4+i];
	dxyzout[i] = dw[0]*ws->d2o[0*4+i] + dw[1]*ws->d2o[1*4+i]
		   + dw[2]*ws->d2o[2*4+i];
    }
}

static void swabsdb( db_star *sp ) {
    int i;
    for(i = 0; i < 9; i++)
	((int *)sp)[i] = htonl(((int *)sp)[i]);	/* x,y,z...,num */
    sp->color = htons(sp->color);
}

void warpsdb( struct warpstuff *ws, FILE *inf, FILE *outf ) {
    static int one = 1;
    int needswab = (*(char *)&one != 0);
    int nstars = 0;
    db_star star;

    if(ws->has_o2d && !ws->has_d2o) {
	deucinv( ws->d2o, ws->o2d );
	ws->has_d2o = -1;
    } else if(!ws->has_o2d && ws->has_d2o) {
	deucinv( ws->o2d, ws->d2o );
	ws->has_o2d = -1;
    }

    while(fread(&star, sizeof(star), 1, inf) > 0) {
	float xyz[3];
	nstars++;
	if(needswab)
	    swabsdb(&star);
	memcpy(xyz, &star.x, 3*sizeof(float));
	if(ws->has_o2d)
	    dwindup_o2d( ws, &star.x, &star.dx, xyz );
	else
	    dwindup( ws, &star.x, &star.dx, xyz );
	if(needswab)
	    swabsdb(&star);
	if(fwrite(&star, sizeof(star), 1, outf) <= 0) {
	    msg("Error writing star #%d", nstars);
	    break;
	}
    }
}

int main(int argc, char *argv[]) {
    struct stuff st[1];
    struct warpstuff *ws;
    char *infname = NULL;

    memset(&st, 0, sizeof(st));

#define BIGBUFSIZE 2097152
    char *obuf = (char *)malloc(BIGBUFSIZE);
    char *ibuf = (char *)malloc(BIGBUFSIZE);
    int opti = -1;

    ws = warp_setup( st, NULL, argc, argv, &opti );
    if(ws == NULL)
	exit(1);	/* warp_setup() must have already printed a message */

    setup_coords( NULL, ws );

    if(ws->has_fixed && getenv("WARPDBG")) {
	fprintf(stderr, "warp rcoredisk %g fixrdisk %g fixomega %g\n", ws->rcoredisk, ws->fixedrdisk, ws->fixomega);
    }

    setvbuf( stdout, obuf, _IOFBF, BIGBUFSIZE );

    if(opti < argc) {
	while(opti < argc) {
	    FILE *inf = (0==strcmp(argv[opti],"-")) ? stdin : fopen(argv[opti], "rb");
	    if(inf == NULL) {
		msg("%s: %s: cannot open input", argv[0], argv[opti]);
	    } else {
		setvbuf( inf, ibuf, _IOFBF, BIGBUFSIZE );
		warpsdb( ws, inf, stdout );
		if(inf != stdin)
		    fclose(inf);
	    }
	    opti++;
	}

    } else {
	setvbuf( stdin, ibuf, _IOFBF, BIGBUFSIZE );
	warpsdb( ws, stdin, stdout );
    }

    return 0;
}

#endif /*!STANDALONE*/

#endif /*USE_WARP*/
