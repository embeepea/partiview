#ifdef USE_IEEEIO

/*
 * Reader for John Shalf's FlexIO data for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <IEEEIO.hh>

#include "shmem.h"
#include <GL/glu.h>
#include "specks.h"

#include <unistd.h>
#include <getopt.h>
#include <math.h>

#include "partiviewc.h"
#include "findfile.h"

static int elements(int rank, int dims[]) {
  int i, n;
  for(i=0, n=1; i < rank; i++)
    n *= dims[i];
  return n;
}

extern "C" {

extern void strncpyt( char *dst, char *src, int dstsize );

int parti_ieee_read( struct stuff **stp, int argc, char *argv[], char *fname, void * ) {
    static char Usage[] = "ieee file.amr";

    if(argc < 1 || strcmp(argv[0], "ieee"))
	return 0;

    if(argc <= 1) {
	msg(Usage);
	return 1;
    }

    struct stuff *st = *stp;
    int timestepno = st->datatime;
    char *realfile;
    int i = 1;

    if(argc>2 && !strcmp(argv[1], "-t")) {
	if((timestepno = (int) getfloat(argv[2], st->datatime)) < 0) {
	    msg("ieee -t: expected timestepnumber(0-based), not %s", argv[2]);
	    return 1;
	}
	i = 3;
    }
    realfile = findfile( fname, argv[i] );
    if(realfile == NULL) {
	msg("%s: ieee: can't find file %s", fname, argv[i]);
    } else {
	/* Load IEEE dataset into time slot */
	specks_ieee_open( st, realfile, st->curdata, timestepno );
    }
    return 1;
}

void parti_ieee_init(void) {
    parti_add_reader( parti_ieee_read, "ieee", NULL );
}

void specks_ieee_open( struct stuff *st, char *fname, int dataset, int starttime )
{

  if(st->ndata >= MAXFILES) {
    fprintf(stderr, "Oops, already read limit of %d data files, ignoring %s\n",
	MAXFILES, fname);
    return;
  }

  IObase *infile = new IEEEIO( fname, IObase::Read );

  if(!infile->isValid()) {
    msg("Couldn't open IEEEIO file %s", fname);
    return;
  }

  if(starttime < 0) starttime = 0;

  int ntimes = infile->nDatasets();		/* time steps */

  int nd = (dataset < 0) ? st->curdata : dataset;

  char *dupfname = NewN( char, strlen(fname)+1 );
  strcpy(dupfname, fname);

  /* Preallocate room for timesteps */
  (void) specks_timespecksptr( st, nd, starttime + ntimes - 1 );

  for(int i = 0; i < ntimes; i++) {

    specks_discard( st, &st->anima[nd][i+starttime] );	/* free any scrap */
    /* st->anima[nd][i+starttime] = NULL; */
    st->datafile[nd][i+starttime] = (void *)infile;
    st->fname[nd][i+starttime] = dupfname;
  }
if(getenv("DBG")) fprintf(stderr, "<p%d>d%d t%d..%d <= %lx %s\n", getpid(), nd, starttime, starttime+ntimes-1, infile, fname );
}

struct specklist *
specks_ieee_read_timestep( struct stuff *st, int subsample, int dataset, int timestep )
{
  int curdata = dataset;
  int curtime = timestep;
  int timebase;
  float spacescale = st->spacescale;
  int skips[64], skinx, skip;
  int outnel;

  if(st->ndata == 0)
    return NULL;

  if(curdata >= st->ndata) curdata = st->ndata-1;
  if(curdata < 0) curdata = 0;

  if(curtime >= st->ntimes) curtime = st->ntimes-1;
  if(curtime < 0) curtime = 0;

  IObase *infile = (IObase *)st->datafile[curdata][curtime];

  if(infile == NULL)
    return NULL;

  if(!infile->isValid()) {
    fprintf(stderr, "<p%d>d%d t%d infile %x invalid!\n", getpid(), dataset, timestep, infile);
    fprintf(stderr, "sleeping 10 secs -- to look around,   dbx -p %d\n", getpid());
    sleep(10);
    return NULL;
  }

  for(timebase = timestep;
	timebase > 0 && (IObase *)st->datafile[curdata][timebase-1] == infile;
	timebase--) 
    ;


  IObase::DataType dt;
  int rank, dims[3];

  infile->seek( curtime - timebase );
  dims[1]=dims[2]=1;
  infile->readInfo(dt, rank, dims);

  int i, k, anel;
  int nel = dims[rank-1];

  if(dims[0] != 3 || rank != 2) {
    fprintf(stderr, "%s<%lx> dataset %d (timestep %d): expected 3xN data organization, not %dx%d!\n",
	st->fname[curdata][curtime], infile, curtime - timebase, curtime, dims[0],dims[1]);
    fprintf(stderr, "sleeping 10 secs -- to look around,   dbx -p %d\n", getpid());
    sleep(10);
    return NULL;
  }

  if(subsample < 1)
    subsample = 1;

  int sktotal = 0;
  for(skinx = 0; skinx < 64; skinx++) {
    skips[skinx] = subsample + (int) ((drand48() - .5)*sqrt(subsample-1));
    if(skips[skinx] <= 0) skips[skinx] = 1;
    sktotal += skips[skinx];
  }

  float *xyz = new float[ elements(rank,dims) ];
  infile->read(xyz);

  int nattr = infile->nAttributes();
  if(nattr > MAXVAL) nattr = MAXVAL;

  outnel = (nel / sktotal) * 64;
  for(i = 0, skinx = 0; i < nel % sktotal; ) {
    skip = skips[skinx++ & (64-1)];
    i += skip;
    outnel++;
  }
  struct specklist *sl = NewN( struct specklist, 1 );
  memset( sl, 0, sizeof(*sl) );
  sl->bytesperspeck = SMALLSPECKSIZE( nattr );

  struct speck *sp = (struct speck *)NewN( char, outnel*sl->bytesperspeck );
  sl->sel = NewN( SelMask, outnel );
  memset( sl->sel, 0, outnel*sizeof(SelMask) );


  sl->specks = sp;
  sl->nspecks = outnel;
  sl->scaledby = st->spacescale;
  sl->coloredby = -1;
  sl->sizedby = -1;

  sl->subsampled = subsample;

  register struct speck *tsp = sp;
  float *ap = xyz;

float x0=1e8,x1=-1e8,y0=1e8,y1=-1e8,z0=1e8,z1=-1e8;
  for(skinx = 0, i = 0; i < nel; ) {
    tsp->p.x[0] = ap[0] * spacescale;
    tsp->p.x[1] = ap[1] * spacescale;
    tsp->p.x[2] = ap[2] * spacescale;
if(x0>tsp->p.x[0]) x0 = tsp->p.x[0]; if(x1<tsp->p.x[0]) x1=tsp->p.x[0];
if(y0>tsp->p.x[1]) y0 = tsp->p.x[1]; if(y1<tsp->p.x[1]) y1=tsp->p.x[1];
if(z0>tsp->p.x[2]) z0 = tsp->p.x[2]; if(z1<tsp->p.x[2]) z1=tsp->p.x[2];
    tsp->size = 1;
    tsp->rgba = 0xFFFFFF00;
    skip = skips[skinx++ & (64-1)];
    tsp = NextSpeck(tsp, sl, 1);
    i += skip;
    ap += 3*skip;
  }

if(getenv("RANGE"))
  printf("range (scaled by %g): %g..%g %g..%g %g..%g\n",
	spacescale, x0,x1, y0,y1, z0,z1);

  delete xyz;

  float *attr = new float[nel];

  for(i = 0; i < nattr && i < MAXVAL; i++) {
    char aname[128];
    char *myname;
    float min = 1e20, max = -1e20, mean = 0;
    struct valdesc *vd = &st->vdesc[curdata][i];

    if(vd->nsamples > 0)
	min = vd->min, max = vd->max;

    infile->readAttributeInfo(i, aname, dt, anel);
    myname = aname;
    if(!strncmp(aname, "particle_", 9)) myname += 9;

    strncpyt(st->vdesc[curdata][i].name, myname, sizeof(st->vdesc[0][0].name));

    if(anel != nel) {
	fprintf(stderr, "Hey, attribute %d (%s) of file %s has %d elements, not %d!\n",
	    i, aname, st->fname[curdata][curtime], anel, nel);
	memset(attr, 0, nel*sizeof(*attr));
    } else {
	infile->readAttribute(i, attr);
    }

    struct speck *peak = NULL;
    for(k = 0, skinx = 0, tsp = sp, ap = attr; k < nel; ) {
	register float a = *ap;
	tsp->val[i] = a;
	if(min > a) min = a;
	if(max < a) {
	    max = a;
	    peak = tsp;
	}
	mean += a;
	skip = skips[skinx++ & (64-1)];
	tsp = NextSpeck( tsp, sl, 1 );
	ap += skip;
	k += skip;
    }
    if((char *)tsp - (char *)sp != outnel * sl->bytesperspeck) {
	fprintf(stderr, "panic: copying attr %d: subsampled into %ld, expected %d of %d\n",
		i, ((char *)tsp - (char *)sp) / sl->bytesperspeck, outnel, anel);
    }
    if(nel > 0) {
	vd->min = min;
	vd->max = max;
	vd->mean = (mean + vd->mean*vd->nsamples) / (outnel + vd->nsamples);
	vd->nsamples += outnel;
	vd->sum = vd->mean * vd->nsamples;
	if(aname[0] == 'd' && peak != NULL) {
	    sl->interest = peak->p;
	}
    }
  }
  delete attr;

  /* If we had any data in this slot before, scrap it now. */

  specks_discard( st, &st->anima[curdata][curtime] );
		/* Is this safe?  Do we need an interlock? */

  st->anima[curdata][curtime] = sl;

  return sl;
}

} // end extern "C"

#endif /*USE_IEEEIO*/
