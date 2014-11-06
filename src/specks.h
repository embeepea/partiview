#ifndef SPECKS_H
#define	SPECKS_H
/*
 * Brains of partiview: core functions and data.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>    /* for GLuint */
#endif

#ifndef CONST
# ifdef __cplusplus
# define CONST const
# else
# define CONST
# endif
#endif

#include "geometry.h"
#include "textures.h"
#include "traj.h"
#include "sclock.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /*HAVE_PTHREAD_H*/

#include <stdio.h>

#define MAXVAL  29
#define CONSTVAL  MAXVAL

struct stuff;

struct speck {
  Point p;
  int rgba;
  float size;
  float val[MAXVAL];	/* other data values, from which we determine size/threshold/etc. */
  char title[28];
};

#define  SMALLSPECKSIZE(maxval)  ( (char *)&(((struct speck *)0)->val[maxval]) - (char *)0 )
#define  SPECKMAXVAL(sl)	 ( (float *)(((char *)0) + sl->bytesperspeck) - (float *)0 )
#define  NewNSpeck(sl, nspecks)  ( (struct speck *)NewN( char, (sl)->bytesperspeck*nspecks ) )
#define  NextSpeck(sp, sl, skip)	 ( (struct speck *) (((char *)sp) + (skip)*(sl)->bytesperspeck ) )

enum Lop { L_LIN, L_ABS, L_LOG, L_EXP, L_POW };

struct valdesc {
  float min, mean, max, sum;
  int nsamples;		/* over which mean is computed */
  float cmin, cmax;	/* range actually used for coloring (may differ from min..max) */
  float lmin, lmax;	/* range actually used for sizing (lum) */
  int cexact;		/* "use exact (unscaled) value as colortable index" */
  int call;		/* "use entire color range for color" */
  int lall;		/* "use entire data range for lum" */
  float lum;		/* luminosity scale factor */
  enum Lop lop;		/* L_LIN, L_ABS, L_LOG, L_EXP, L_POW */
  float lbase, lexp;	/* L_ABS: |v|+lbase; L_LOG: log(v/lbase + lexp); */
			/* L_EXP: pow(lexp, v/lbase); L_POW: pow(v/lbase, lexp) */
  char name[20];
  struct cment *vcmap;	/* per-data-variable colormap (may be NULL); rgba format */
  int vncmap;		/* number of entries in cmap */
};

struct cment {
  int raw;
  int cooked;
};

struct AMRbox {
  Point p0, p1;		/* opposite corners of box */
  int level;
  int boxno;
};

enum SurfStyle {
  S_SOLID, S_LINE, S_PLANE, S_POINT, S_HALO, S_OFF
};

enum MeshType {
  QUADMESH, POLYMESH, MODEL
};

typedef struct {	/* for glDrawElements() */
  GLenum prim;		/* GL primitive (GL_TRIANGLES, GL_QUADS, GL_TRIANGLE_STRIP, etc.) */
  GLenum eltype;	/* glInterleavedArrays() type code (GL_V3F, GL_C4UB_V3F, GL_T2F_V3F, GL_N3F_V3F, etc. */
  int base;		/* Starting index in elindices[].  Each is an index into elarrays[] */
  int count;		/* Number of indices */
  int ixmin, ixmax;	/* range of indices -- range of elarrays[] referenced by elindices[base .. base+count-1] */
} Elements;

struct mesh {
  struct mesh *next;
  enum MeshType type;
  enum SurfStyle style;
  void *wavobj;		/* class WavObj, used iff type == MODEL */
  void (*objrender)(struct stuff *,struct mesh *);  /* draw func, if not built-in */
  int cindex;		/* colormap index, or -1 for white */
  int txno;		/* texture index, or -1 if none */
  int levelno;		/* level number, for show/hide selection */
  int nu, nv;		/* for QUADMESH type */
  int nfaces;		/* face count */
  int nfv;		/* number of face-vertices (sum of abs(fvn[i])) */
  int nverts;		/* size of pts array, = nu*nv for QUADMESHes */
  int ntx;		/* size of tx array */
  int nvnorms;		/* size of vnorms array */
  int *fvn;		/* fvn[nfaces] verts on this face (neg for tstrips) */
  int *fv0;		/* fv0[nfaces] starting index in fvs[] */
  int *fvs;		/* fvs[nfv] face vertex indices, each 0..nverts-1 */
#define FVSTEP	3
#define  FVS_VERT 0
#define  FVS_TX   1
#define  FVS_VNORM 2
  Point *pts;		/* pts[nverts] vertex coords */
  Point *tx;		/* tx[ntx] texture coords, or NULL if absent */
  Point *vnorms;	/* vnorms[nvnorms] vertex normals, or NULL */
  Point *fnorms;	/* fnorms[nfnorms] facet normals, or NULL if absent */
  Point *fcolors;	/* fcolors[nfaces] RGB colors stored in Point's, or NULL if absent */
  float haloth;		/* halo threshold */
  float linewidth;	/* 0->default */
  int usettfm;		/* non-identity ttfm */
  Matrix ttfm;		/* texture transform */

		/* for glDrawElements() */
  Elements *elem;
  int nelem;
  int *elindices;
  float *elarrays;
};

struct ellipsoid {
  struct ellipsoid *next;
  int nu, nv;
  enum SurfStyle style;
  int cindex;
  int level;
  Point pos;
  Point size;
  int hasori;
  Matrix ori;
  char *title;
  float linewidth;
};

struct straj {
  struct straj *next;
  float ttime0;		/* traj-time that maps to anim time 0 */
  float ttimerate;	/* traj-time increment per unit anim time */
  float tspan0;		/* traj-time back to ... */
  float tspan1;		/* traj-time back to ... */
};

enum SpecialSpeck { SPECKS=0, POLYLINES, TMESHES, MARKERS, ELLIPSOIDS };

#define MAXSELNAME 16
struct selitem {
  char name[MAXSELNAME];
};

typedef unsigned int SelMask;

enum SelUse {
    SEL_NONE,	/* disabled */
    SEL_USE,	/* evaluate given function */
    SEL_DEST
};


typedef struct _selfunc {
  SelMask wanted;		/* mask of interesting bits, either on or off */
  SelMask wanton;		/* mask of bits which are interesting if on */
  enum SelUse use;
} SelOp;

  /*
   * Selection scheme is:
   *  User specifies a set of bit-numbers, like
   *    3  -5  7  -10
   *  which refers to all specks with attributes 3 or 7 on, AND attributes 5 and 10 off.
   *  To compute this, consider two masks:
   *    wanton = (1<<3) | (1<<7)
   *    wantoff = (1<<5) | (1<<10)
   *  Then construct
   *    wanted = (wanton | wantoff)
   *  Now the speck with attribute-mask "attributes" is selected iff
   *    (attributes ^ wanton) & (wanton | wantoff) == 0
   */
#define	SELECTED(attrs, selp)	(0==( ((attrs)^(selp)->wanton) & (selp)->wanted))
#define	SELBYDEST(attrs, selp)  (0==( ((attrs)^(selp)->wanton) & ~(selp)->wanted))
#define	SELSET(attrs, selp)	((attrs) = ((attrs)&(selp)->wanted) ^ (selp)->wanton)
#define SELUNSET(attrs, selp)	((attrs) = ((attrs)|~(selp)->wanted) ^ (selp)->wanton)
	/* SET:   want=1 => attrs' = attrs ^ wanton
	 * 	  want=0 => attrs' = wanton
	 * UNSET: want=1 => attrs' = attrs ^ wanton
	 * 	  want=0 => attrs' = ~wanton
	 */

#define SELMASK(bitno)		(((SelMask)1) << ((bitno)-1))

struct specklist {
  struct specklist *next;
  int nspecks;
  int bytesperspeck;	/* allow for shortened specks */
  struct speck *specks;
  float scaledby;
  int coloredby;	/* val[] index used for colormapping */
  int sizedby;		/* val[] index used for sizing */
  int colorseq, sizeseq, threshseq;
  Point center, radius;	/* of bounding box, after scaling to world space */
  Point interest;	/* point-of-interest, if any */
  char *text;
  int subsampled;	/* subsampling ("every") factor applied already */
  int used;		/* "used" clock, for mem purges */
  int speckseq;		/* sequence number, for tracking derived specklists */
  int nsel;
  unsigned int *sel;	/* selection bitmasks per particle */
  int selseq;
  enum SpecialSpeck special;
  struct specklist *freelink; /* link on free/scrap list */
};

struct specktree {	/* Not used yet, if ever */
  struct specklist self;
  struct specklist *kid[8];
};

struct coordsys {
  char name[16];
  Matrix w2coord;
};

struct wfframe {
  float tx, ty, tz;
  float rx, ry, rz;
  float fovy;
};

struct wfpath {
  struct wfpath *next;
  int nframes, frame0;
  int curframe;
  float fps;		/* frames per second */
  struct wfframe *frames;
  float *focallens;
};

enum FadeType {
     F_SPHERICAL, F_PLANAR, F_CONSTANT, F_LINEAR, F_LREGION, F_KNEE2, F_KNEE12
};

/* async reader ops:
 *  lock, unlock shared data (by reader and main)
 *  pause, resume thread (wait on condition variable) (by main)
 *  cancel read (by main)
 *  set memory budget (by main)
 *  notify of change of timespan or whatever (by reader)
 *  import data (by reader)
 *  purge data (by reader)
 */  

typedef struct dyndata {
    int enabled;
    int slvalid;
    void *data;
    int (*ctlcmd)( struct dyndata *, struct stuff *, int argc, char **argv );
    int (*trange)( struct dyndata *, struct stuff *, double *tminp, double *tmaxp, int ready );
    struct specklist *(*getspecks)( struct dyndata *, struct stuff *, double realtimestep );
    int (*draw)( struct dyndata *, struct stuff *, struct specklist *head, Matrix *Tc2w, float radperpix );
    int (*help)( struct dyndata *, struct stuff *, int verbose );
    void (*free)( struct dyndata *, struct stuff * );
} DynData;


    
typedef int (*SpecksPickFunc)( struct stuff *, GLuint *hit, struct specklist *, int speckno );



#define MAXFILES 8

struct stuff {
	/* owned by display thread: */
  struct specklist *sl;	/* Current display = anima[curdata][curtime] */

	/* shared with possible reader threads: */
  struct specklist **anima[MAXFILES];	/* anima[ndata][ntimes]: All data.  Shared with reader threads. */
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t smut;
#endif

  char dataname[MAXFILES][12];
  int ntimes, ndata;
  int timeroom;			/* number of slots allocated for:
				 * anima[0..ndata-1][0..timeroom-1]
				 * datafile[0..ndata-1][0..timeroom-1]
				 */
  
	/* rest owned by display thread. */
  int curtime, curdata;
  double currealtime;
  SClock *clk;			/* provider of time */
  int datatime;			/* timestep at which to add newly-read specks */
  int datasync;			/* read .raw data synchronously? */
    /* Clones of sl, curtime, curdata, snapped once per frame for consistency */
	/* If st->dyndata,
	 * then specks_set_timestep(st, time)
	 * calls (*st->dyndatafunc)(st, time) to find new st->sl.
	 * st->dyndatadata can hold an opaque pointer used by dyndatafunc.
	 */
  DynData dyn;

  int conste;			/* Takahei's constellation figures */
  void *constedata;

  struct specklist *frame_sl;
  int frame_time, frame_data;
  struct specklist **annot[MAXFILES];  /* annotation strings */
  void **datafile[MAXFILES];	/* datafile[ndata][ntimes] -- open-file handles for each dataset */
  int datatimes[MAXFILES];	/* number of timesteps for each dataset */
  char **fname[MAXFILES];
#define CURDATATIME(field)  (((unsigned int)st->curtime < st->ntimes) ? st->field[st->curdata][st->curtime] : NULL)
  struct valdesc vdesc[MAXFILES][MAXVAL+1];
  char *annotation;		/* annotation string */
  char *frame_annotation;	/* ... snapped for current frame */
  char *alias;			/* alt name for this object besides g<N> */

  int ntextures;		/* extensible array of Textures */
  Texture **textures;


  Matrix objTo2w;		/* object-to-world transform, used in cave only so far */
  float spacescale;
  int sizedby, coloredby;		
  int sizeseq, colorseq;

  int usesee;
  SelOp seesel;		/* bitmask of desired features in sl->sel[] */
  int selseq;			/* sel[] change sequence number */

  SpecksPickFunc picked;
  void *pickinfo;

  int trueradius;
  char *sdbvars;
  int maxcomment;

  struct coordsys altcoord[1];

  float speed;			/* time steps per CAT_time second */
  float fspeed;			/* time steps per CAVETime second */
  int skipblanktimes;		/* skip time-slots with nothing in them */
  double time0;			/* base timestep, added to time*speed */
  int timeplay;
  int timefwd;
  int timestepping;
  double timedelta;		/* default delta timestep amount */
  float playnext;		/* time at which we take our next step.
				 * (= CAVETime of last step plus 1/fspeed,
				 * if in timeplay mode).
				 */
  int hidemenu;
  float menudemandfps;		/* If frame rate < demanded then hide scene */

  float alpha;
  float gamma;			/* to compensate for display gamma */
  int fast;
  int fog;			/* unused */
  float pfaint, plarge;		/* min/max point brightness */
  float polymin;		/* min size (pixels) to render polygons */
  float polymax;		/* max size (pixels) to render polygons */
  float polyfademax;		/* if polymin < size < polyfademax, then fade */

  float psize;
  float polysize;
  float textmin;		/* min height (pixels) to render labels */
  float textsize;
  int useme;			/* global enable/disable display flag */
  int usepoly;
  int polysizevar;
  int polyarea;			/* scale polygon area with data? else radius */
  int polyorivar0;
  int usepoint;
  int usetext;
  int usetextaxes;
  int usetextures;
  int useblobs;
  float txscale;
  int texturevar;
  float fogscale;
  int npolygon;
  enum { VEC_OFF, VEC_ON, VEC_ARROW } usevec;
  int vecvar0;
  float vecscale;
  float vecarrowscale;
  float vecalpha;

  enum FadeType fade;

  float fadeknee1, fadeknee2;	/* near and far distance knees in fade curve */
  float knee2steep;		/* steepness of far knee */
  Point fadecen;
  
#define P_THRESHMIN 0x1
#define P_THRESHMAX 0x2
#define P_USETHRESH 0x4		/* supplanted by "see thresh" */
  SelOp threshsel;
  int usethresh;		/* bit-encoded */
  int threshvar;		/* datavar index to threshold on */
  float thresh[2];		/* data range */
  int threshseq;
#define	SEL_OFF	   34
#define SEL_ALL	   33
#define SEL_THRESH 32
#define SEL_PICK   31		/*XXX shouldn't be hard-wired!*/

  int useemph;
  SelOp emphsel;
  float emphfactor;		/* increase lum by this factor */


#define MAXSEL 30
  struct selitem selitems[MAXSEL];

  int inpick;

  float gscale;
  Point gtrans;

  int useboxes;
  int boxtimes;
  int boxlevels;
  int boxlevelmask;
  int boxlabels;
  int boxaxes;
  float boxlabelscale;
  float boxlinewidth;
  float goboxscale;
#define MAXBOXLEV 28
  float boxscale[MAXBOXLEV];
  struct AMRbox **boxes;	/* An array per time step */

  int staticboxroom;
  struct AMRbox *staticboxes;	/* another array of permanent boxes */

  int usemeshes;
  struct mesh *staticmeshes;
  struct mesh **meshes[MAXFILES]; /* meshes[curdata][curtime] list of meshes */
  float mullions;
  int useellipsoids;
  struct ellipsoid *staticellipsoids;

  char vdcmd[128];

  int subsample;	/* show every Nth data point */
  int everycomp;	/* compensate for subsampling by brightening? */
  struct cment *cmap;	/* rgba format */
  int ncmap;		/* number of entries in cmap, 1..256 (> 0) */

  struct cment *boxcmap;
  int boxncmap;

  struct cment *textcmap;
  int textncmap;

  struct AMRbox clipbox; /* clipping region.  clipbox.level > 0 if active. */

  int depthsort;	/* sort particles by camera distance? */

  int fetchpid;
  volatile int fetching; /* busy fetching data in subprocess (don't start another fetch) */
  volatile int fetchdata, fetchtime; /* data and timestep being fetched */

  int used;		/* global "used" clock, for LRU purging */
  struct specklist *scrap; /* stuff to be deleted when it's safe */
  int nghosts;		/* keep recent ghost snapshots of dynamic data */

  int usertrange;
  double utmin, utmax, utwrap;	/* user-supplied time range */

  int speckseq;		/* sequence-number seed */

  /* rgb565 colormaps & parameters */
  float rgbright[3], rgbgamma[3];
  unsigned char rgbmap[3][256];

  /* chromadepth stuff */
  int use_chromadepth;
  float chromaslidestart, chromaslidelength;
  int nchromacm;
  struct cment *chromacm;
};

/*
 * some globals for collaborating with Matt's AMR code.
 * We need to publicize where our menu lies.
 */
extern int parti_menuwall;
extern int parti_menubox[4];	/* xmin,ymin, xmax,ymax (pixels)
				 * with 0,0 at LOWER LEFT
				 */
extern int parti_datastep;
extern int vtkamr_datastep, vtkamr_datastep_ready;


extern void	     specks_display( struct stuff *st );

extern struct stuff *specks_init( int argc, char *argv[] );
extern void	     specks_read( struct stuff **stp, char *fname );

extern struct specklist *specks_ieee_read_timestep( struct stuff *st,
			int subsample, int dataset, int timestep );
extern void  specks_ieee_open( struct stuff *st, char *fname, int dataset, int starttime );
extern void  drawspecks( struct stuff *st );
int specks_partial_pick_decode( struct stuff *st, int id,
			int nhits, int nents, GLuint *hitbuf,
			unsigned int *bestzp,
			struct specklist **slp, int *speckno, Point *pos );
extern int   specks_parse_args( struct stuff **, int cat_argc, char *cat_argv[] );

extern void  specks_set_time( struct stuff *, double newtime );
extern void  specks_set_timestep( struct stuff * );	/*...from current time*/
extern int   specks_get_datastep( struct stuff * );
extern void  specks_set_speed( struct stuff *, double newspeed );
extern void  specks_set_timebase( struct stuff *, double newbase );
extern void  specks_discard( struct stuff *, struct specklist **slist );  /* Free later */

extern int   specks_check_async( struct stuff ** );
extern int   specks_add_async( struct stuff *st, char *cmdstr, int replytoo );

extern void specks_read_cmap( struct stuff *st, char *fname, int *ncmapp, struct cment **cmapp );

extern void  specks_set_annotation( struct stuff *, CONST char *str );

extern void  specks_current_frame( struct stuff *, struct specklist *sl );
extern void  specks_reupdate( struct stuff *, struct specklist *sl );
extern void  specks_datawait( struct stuff * );

extern void  specks_lock_init( struct stuff * );
extern void  specks_lock( struct stuff * );
extern void  specks_unlock( struct stuff * );

	/* uses locking: */
extern struct specklist * specks_timespecks( struct stuff *, int dataset, int timestep );
extern void specks_ensuretime( struct stuff *, int dataset, int timestep );
extern void specks_insertspecks( struct stuff *, int dataset, int timestep, struct specklist * );
extern void specks_clearspecks( struct stuff *, int dataset, int timestep );



extern float display_time(void);
extern char *rejoinargs( int arg0, int argc, char **argv );
extern double getfloat( char *str, double defval );
extern int getbool( char *str, int what );
extern int specks_set_byvariable( struct stuff *st, char *str, int *val );

extern int parse_selexpr( struct stuff *, CONST char *str, SelOp *dest, SelOp *src, CONST char *plaint );
extern char *show_selexpr( struct stuff *, SelOp *dest, SelOp *src );
extern void selinit( SelOp * );
extern void selsrc2dest( struct stuff *, CONST SelOp *src, SelOp *dest );
extern void seldest2src( struct stuff *, CONST SelOp *dest, SelOp *src );
CONST char *selcounts( struct stuff *st, struct specklist *sl, SelOp *selp );

extern SpecksPickFunc specks_all_picks( struct stuff *, SpecksPickFunc func, void *arg );

#ifdef __cplusplus
}
#endif

#endif /*SPECKS_H*/
