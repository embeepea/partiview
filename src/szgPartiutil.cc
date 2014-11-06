/*
 * Utility functions for szgPartiview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include "winjunk.h"
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>    /* for GLuint */
#endif

#include "geometry.h"
#include "shmem.h"
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#include "specks.h"
#include "partiviewc.h"
#include "szgPartiview.h"

#include <ctype.h>
#undef isspace		/* hack for irix 6.5 */
#undef isdigit


/* Handle pick results */
void specks_picker( void *view, int nhits, int nents, GLuint *hitbuf, void *vview )
{
#if 0
  struct stuff *st;
  unsigned int bestz = ~0u;
  struct stuff *bestst = NULL;
  struct specklist *bestsl;
  int bestspeckno;
  int id, bestid = -1;
  Point bestpos, wpos, cpos;
  int centerit = Fl::event_state(FL_SHIFT);

  for(id = 0; id < MAXSTUFF; id++) {
    if((st = stuffs[id]) == NULL) continue;
    if(specks_partial_pick_decode( st, id, nhits, nents, hitbuf,
					&bestz, &bestsl, &bestspeckno, &bestpos )) {
	bestst = st;
	bestid = id;
    }
  }
  if(bestst) {
    char fmt[1024], wpostr[80];
    char attrs[1024];
    vtfmpoint( &wpos, &bestpos, view->To2w( bestid ) );
    if(memcmp( &wpos, &bestpos, sizeof(wpos) )) {
	sprintf(wpostr, " (w%g %g %g)", wpos.x[0],wpos.x[1],wpos.x[2]);
    } else {
	wpostr[0] = '\0';
    }
    vtfmpoint( &cpos, &wpos, view->Tw2c() );
    strcpy(fmt, bestsl->bytesperspeck <= SMALLSPECKSIZE(MAXVAL)
		? "[g%d]%sPicked %g %g %g%s%.0s @%g (of %d)%s"
		: "[g%d]%sPicked %g %g %g%s \"%s\" @%g (of %d)%s");
    attrs[0] = '\0';
    struct speck *sp = NextSpeck( bestsl->specks, bestsl, bestspeckno);
    if(bestsl->text == NULL && bestst->vdesc[bestst->curdata][0].name[0] != '\0') {
	strcpy(attrs, ";");
	for(int i = 0; i < MAXVAL && bestst->vdesc[bestst->curdata][i].name[0]; i++) {
	    int pos = strlen(attrs);
	    snprintf(attrs+pos, sizeof(attrs)-pos, " %s %g",
		    bestst->vdesc[bestst->curdata][i].name, sp->val[i]);
	}
    }
    if(bestsl->text != NULL) {
	snprintf(attrs+strlen(attrs), sizeof(attrs)-strlen(attrs),
		" \"%s\"", bestsl->text);
    }
    msg(fmt, bestid, centerit?"*":"",
	bestpos.x[0],bestpos.x[1],bestpos.x[2], wpostr,
	bestsl->text ? bestsl->text : sp->title,
	vlength( &cpos ), nhits,
	attrs );

    if(centerit) {
	view->center( &wpos );
	if(ppui.censize > 0) view->redraw();
    }
  } else {

    msg("Picked nothing (%d hits)", nhits);
  }
#endif
}

void parti_set_alias( struct stuff *st, const char *alias ) {

  if(st == NULL) return;

  if(alias == NULL) alias = "";
  if(st->alias) free(st->alias);
  st->alias = strdup(alias);
}


void parti_allobjs( int argc, char *argv[], int verbose ) {
    struct stuff *st = ppszg.st;
    if(parti_parse_args( &st, argc, argv, NULL ))
	return;
    PvScene *scene = ppszg.scene;
    if(scene == 0)
	return;
    for(int i = 0; i < scene->nobjs(); i++) {
	st = scene->objstuff(i);
	if(st) {
	    const char *onoff = st->useme ? "on" : "off";
	    if(verbose) {
		if(st->alias)  msg("g%d=%s: (%s)", i, st->alias, onoff);
		else msg("g%d: (%s)", i, onoff);
	    }
	    specks_parse_args( &st, argc, argv );
	}
    }
}

void parti_redraw() {
   // nop -- szg is always redrawing.
}


void parti_update() {
   // nop too
}

void parti_censize( float newsize ) {
  ppszg.scene->censize( newsize );
}

float parti_getcensize() {
  return ppszg.scene->censize();
}

char *parti_bgcolor( const char *rgb ) {
  static char bgc[16];
  Point bgcolor;

  bgcolor = ppszg.bgcolor;
  if(rgb) {
    if(1 == sscanf(rgb, "%f%*c%f%*c%f", &bgcolor.x[0], &bgcolor.x[1], &bgcolor.x[2]))
	bgcolor.x[1] = bgcolor.x[2] = bgcolor.x[0];
    ppszg.bgcolor = bgcolor;
  }

  sprintf(bgc, "%.3f %.3f %.3f", bgcolor.x[0],bgcolor.x[1],bgcolor.x[2]);
  return bgc;
}



char *parti_stereo( const char *ster )
{
  return "nevermind";
}

void parti_detachview( const char *how ) {
  // nop.
}

char *parti_winsize( CONST char *newsize ) {
  static char cursize[24];

  sprintf(cursize, "314x159");
  return cursize;
}

char *parti_clip( const char *nearclip, const char *farclip ) {
  float n = 0.1, f = 2500;
  int changed = 0;
  if(nearclip != NULL && sscanf(nearclip, "%f", &n) > 0) {
    ppszg.nearclip( n );
    changed = 1;
  }
  if(farclip != NULL && sscanf(farclip, "%f", &f) > 0) {
    ppszg.farclip( f );
    changed = 1;
  }
  if(changed) {
    ppszg.setClipPlanes( ppszg.nearclip(), ppszg.farclip() );
  }

  static char why[16];
  sprintf(why, "clip %.4g %.4g", ppszg.nearclip(), ppszg.farclip());
  return why;
}

int parti_move( const char *onoffobj, struct stuff **newst ) {
#if 0
  XXX this might be nice to implement -- let navigation change an object's placement
  rather than moving the camera

  int objno = -1;
  if(onoffobj) {
    if(!strcmp(onoffobj, "off"))
	ppui.view->movingtarget(0);
    else if(!strcmp(onoffobj, "on"))
	ppui.view->movingtarget(1);
    else if(sscanf(onoffobj, "g%d", &objno) > 0 ||
			sscanf(onoffobj, "%d", &objno) > 0) {
	ppui.view->movingtarget(1);
	objno = parti_object( onoffobj, newst, 0 );
    } else {
	msg("move {on|off|g<N>} -- select whether navigation can move sub-objects");
    }
  }
  return ppui.view->movingtarget() ? ppui.view->target() : -1;
#endif
  return -1;
}

float parti_pickrange( char *newrange ) {
#if 0
  XXX determines accuracy needed when picking
  if(newrange) {
    if(sscanf(newrange, "%f", &ppui.pickrange))
	ppszg.view->picksize( 4*ppui.pickrange, 4*ppui.pickrange );
  }

  return ppui.pickrange;
#endif
  return 0;
}

static int endswith(char *str, char *suf) {
  if(str==NULL || suf==NULL) return 0;
  int len = strlen(str);
  int suflen = strlen(suf);
  return suflen <= len && memcmp(suf, str+len-suflen, suflen)==0;
}

int parti_snapset( char *fname, char *frameno, char *imgsize )
{
  int len;
#if unix
  static char suf[] = ".%03d.ppm.gz";
#else
  static char suf[] = ".%03d.ppm";
#endif
  int needsuf;

  if(fname) {
    len = strlen(fname);
    needsuf = strchr(fname, '%')!=NULL || fname[0] == '|'
		  || endswith(fname, ".tif") || endswith(fname, ".tiff")
		  || endswith(fname, ".sgi") || endswith(fname, ".png")
		  || endswith(fname, ".jpeg") || endswith(fname, ".jpg")
		  || endswith(fname, ".ppm") || endswith(fname, ".gz")
		? 0 : sizeof(suf)-1;
    if(ppszg.snapfmt)  Free(ppszg.snapfmt);
    ppszg.snapfmt = NewN(char, len+needsuf+1);
    sprintf(ppszg.snapfmt, needsuf ? "%s%s" : "%s", fname, suf);
  }
  if(frameno)
    sscanf(frameno, "%d", &ppszg.snapfno);
  return ppszg.snapfno;
}

int parti_snapshot( char *snapinfo )
{
  char tfcmd[10240], *tftail;
  int fail;
#if !WIN32  /* if unix */
  static char defsnap[] = "snap.%03d.sgi";
  static char prefix[] = "|convert ppm:- ";
  static char gzprefix[] = "|gzip >";
#else
  static char defsnap[] = "snap.%03d.ppm";
  static char prefix[] = "";
#endif

  if(snapinfo) snapinfo[0] = '\0';
  if(ppszg.snapfmt == NULL) {
    ppszg.snapfmt = NewN(char, sizeof(defsnap)+1);
    strcpy(ppszg.snapfmt, defsnap);
  }
  
  if(ppszg.snapfmt[0] == '|' || endswith(ppszg.snapfmt, ".ppm")) {
    tfcmd[0] = '\0';
#if unix
  } else if(endswith(ppszg.snapfmt, ".ppm.gz")) {
    strcpy(tfcmd, gzprefix);
#endif
  } else {
    strcpy(tfcmd, prefix);
  }
  tftail = tfcmd+strlen(tfcmd);
  sprintf(tftail, ppszg.snapfmt, ppszg.snapfno);

  // Ensure window's image is up-to-date
  // parti_update();

#if 0
  XXX might be nice to implement this someday, somehow.
  Probably would need to be a flag read at the end of PvScene::draw()

  int y, h = ppui.view->h(), w = ppui.view->w();
  char *buf = (char *)malloc(w*h*3);
  if(!ppui.view->snapshot( 0, 0, w, h, buf )) {
    free(buf);
    msg("snapshot: couldn't read from graphics window?");
    return -2;
  }

  FILE *p;
#if unix
  void (*oldpipe)(int);
  int popened = tfcmd[0] == '|';
  if(popened) {
    oldpipe = signal(SIGPIPE, SIG_IGN);
    p = popen(tfcmd+1, "w");
  } else
#endif
    p = fopen(tfcmd, "wb");

  fprintf(p, "P6\n%d %d\n255\n", w, h);
  for(y = h; --y >= 0 && fwrite(&buf[w*3*y], w*3, 1, p) > 0; )
    ;
  free(buf);
  fflush(p);
  fail = ferror(p);

#if unix
  if(popened) {
    pclose(p);
    signal(SIGPIPE, oldpipe);
  }
  else
#endif
    fclose(p);  /* win32 */


  if(y >= 0 || fail) {
    msg("snapshot: Error writing to %s", tfcmd);
    return -1;
  }
  if(snapinfo)
    sprintf(snapinfo, "%.1000s [%dx%d]", tftail, w, h);
#endif
  return ppszg.snapfno++;
}

const char *parti_get_alias( struct stuff *st ) {
  return st && st->alias ? st->alias : "";
}

/*
 * This is how new objects get created -- via "object g<N>" command.
 */
int parti_object( const char *objname, struct stuff **newst, int create ) {
    int i = -1;
    int gname = 0;
    char c;
    PvObject *ob = NULL;
    bool useme = false;

    if(newst)
	*newst = ppszg.st;

    if(objname == NULL) {
	return parti_idof( ppszg.st );
    }

    if(!strcmp(objname, "c") || !strcmp(objname, "c0")) {
    	return OBJNO_NONE;
    }

    c = '=';
    if((		/* accept gN or gN=... or N or N=... or [gN] */
	    ((sscanf(objname, "g%d%c", &i, &c) > 0 || sscanf(objname, "%d%c", &i, &c) > 0) && c == '=')
	    || sscanf(objname, "[g%d]", &i) > 0)) {
	    gname = 1;
	    ob = ppszg.scene->obj( i );
	    if(ob==0 || ob->objstuff() == 0) {
		if(create) {
		    ob = ppszg.scene->addobj( objname, i );
		    useme = true;
		}
	    } else {
		// Need to create this one
		useme = true;
	    }
    } else {
	for(i = ppszg.scene->nobjs(); --i >= 0; ) {
	    ob = ppszg.scene->obj(i);
	    struct stuff *st = ob->objstuff();
	    if(st && st->alias && 0==strcmp(st->alias, objname)) {
		useme = true;
		break;
	    }
	}
    }
    if(i == -1 && create && !gname) {
	ob = ppszg.scene->addobj( objname );
	if(!gname)
	    parti_set_alias(ob->objstuff(), objname);
	useme = true;
    }
    if(useme && ob->objstuff() != NULL) {
	ppszg.st = ob->objstuff();
	if(newst) *newst = ppszg.st;
	return ob->id();
    }
    msg("Don't understand object name \"%s\"", objname);
    return OBJNO_NONE;
}

int parti_idof( struct stuff *st ) {
    for(int i = 0; i < ppszg.scene->nobjs(); i++) {
	if(ppszg.scene->objstuff(i) == st)
	    return i;
    }
    return OBJNO_NONE;
}

float parti_focal( CONST char *newfocal ) {
  float focallen;
  if(newfocal && sscanf(newfocal, "%f", &focallen) > 0)
    ppszg.focallen( focallen );
  return ppszg.focallen();
}

int parti_focalpoint( int argc, char **argv, Point *focalpoint, float *minfocallen )
{

#if 0
    int ison;
    Point fpt;
    float mlen;
    int changed = 0;

    ison = ppui.view->focalpoint( &fpt, &mlen );

    if(argc > 0) {
	if(isalpha(argv[0][0])) {
	    int nowon = getbool(argv[0], -1);
	    if(nowon < 0)
		argc = 0;
	    else {
		ison = nowon;
		changed = 1;
		argc--;
		argv++;
	    }
	}
	if(getfloats( &fpt.x[0], 3, 0, argc, argv ) ||
		getfloats( &mlen, 1, 3, argc, argv ) ||
		changed)
	    ppui.view->focalpoint( &fpt, mlen, ison );
    }
    ison = ppui.view->focalpoint( &fpt, &mlen );
    if(focalpoint)
	*focalpoint = fpt;
    if(minfocallen)
	*minfocallen = mlen;
    return ison;
#endif
    return 0;
}


float parti_fovy( CONST char *newfovy ) {
  // stub
  return 42;
}

void parti_center( CONST Point *cen ) {
    ppszg.scene->center( cen );
}

void parti_seto2w( struct stuff *, int objno, const Matrix *o2w ) {
  PvObject *ob = ppszg.scene->obj( objno );
  if(ob)
      ob->To2w( o2w );
}

void parti_geto2w( struct stuff *, int objno, Matrix *o2w ) {
  PvObject *ob = ppszg.scene->obj( objno );
  if(ob)
    *o2w = *ob->To2w();
}

void parti_parent( struct stuff *st, int parent ) {
    PvObject *ob = ppszg.scene->obj( parti_idof( st ) );
    if(ob)
	ob->parent( parent );
}

void parti_nudge_camera( Point *disp ) {
  Matrix Tc2w, Tdisp, Tnewc2w;

  parti_getc2w( &Tc2w );
  mtranslation( &Tdisp, disp->x[0], disp->x[1], disp->x[2] );
  mmmul( &Tnewc2w, &Tc2w,&Tdisp );
  parti_setc2w( &Tnewc2w );
}

void subcam_matrices( Subcam *sc, Matrix *Tc2subc, float subclrbt[4] )
{
  Matrix Ry, Rx, Rz, Rflop, Tfy, Tfyx;
  mrotation( &Rflop, 90,     'x' );
  mrotation( &Ry, -sc->azim, 'y' );
  mrotation( &Rx, -sc->elev, 'x' );
  mrotation( &Rz, -sc->roll, 'z' );
  mmmul( &Tfy, &Rflop, &Ry );
  mmmul( &Tfyx, &Tfy, &Rx );
  mmmul( Tc2subc, &Tfyx, &Rz );

  subclrbt[0] = -tanf( sc->nleft * (M_PI/180) );
  subclrbt[1] =  tanf( sc->right * (M_PI/180) );
  subclrbt[2] = -tanf( sc->ndown * (M_PI/180) );
  subclrbt[3] =  tanf( sc->up    * (M_PI/180) );
}

int parti_make_subcam( CONST char *name, int argc, char **params )
{
  Subcam tsc;
  int index;
  int i, any = 0;
  if(name == NULL || params == NULL)
    return 0;

  index = parti_subcam_named( name );
  if(index > 0)
    tsc = ppszg.sc[index-1];

  for(i = 0; i < 7 && i < argc; i++) {
    char junk;
    int ok = sscanf(params[i], "%f%c", ((float *)&tsc.azim) + i, &junk);
    if(ok == 1) any++;
    else if(0!=strcmp(params[i], ".") && 0!=strcmp(params[i], "-")) {
	msg("subcam %s: what's ``%s''?", name, params[i]);
	return 0;
    }
  }

  if(tsc.nleft + tsc.right == 0 || tsc.ndown + tsc.up == 0) {
    msg("subcam %s -- degenerate?", name);
    return 0;
  }
  if(any == 0)
    return index;

  if(index == 0) {
    if(any != 7) {
	msg("new subcam %s: expected 7 parameters", name);
	return 0;
    }
    index = ppszg.sc.size();
    ppszg.sc.reserve( index+1 );
  }

  sprintf(tsc.name, "%.7s", name);
  ppszg.sc[index] = tsc;
  return index+1;
}

int parti_subcam_named( CONST char *name )
{
  for(int i = 0; i < ppszg.sc.size(); i++)
    if(0==strncmp(ppszg.sc[i].name, name, 7))
	return i+1;
  return 0;
}

int parti_select_subcam( int index )
{
#if 0
  if(index <= 0 || index > ppszg.sc.size())
    return ppszg.subcam;

  ppszg.subcam = index;
  if(index == 0) {
    ppszg.view->usesubcam(0);
    parti_redraw();
    return 0;
  }

  Subcam *tsc = &ppszg.sc[index-1];

  Matrix Tc2subc;
  float subclrbt[4];
  subcam_matrices( tsc, &Tc2subc, subclrbt );
  ppui.view->Tc2subc( &Tc2subc );
  ppui.view->subc_lrbt( subclrbt );
  ppui.view->usesubcam(1);
  parti_redraw();
  return index;
#endif
  return 0;
}

char *parti_get_subcam( int index, char *paramsp )
{
  if(paramsp) paramsp[0] = '\0';
  if(index <= 0 || index > ppszg.sc.size()) return "";
  Subcam *sc = &ppszg.sc[index-1];
  if(sc->name[0] == '\0') return "";
  if(paramsp)
    sprintf(paramsp, "%g %g %g %g %g %g %g",
	sc->azim, sc->elev, sc->roll,
	sc->nleft, sc->right, sc->ndown, sc->up);
  return sc->name;
}

int parti_current_subcam( void )
{
  return 0;
}

char *parti_subcam_list()
{
  static char *what = 0;
  char *tail;
  static char blank[2] = "";
  what[0] = '\0';
  int i, len;
  for(i = 0, len = 1; i < ppszg.sc.size(); i++)
    len += 1 + strlen(ppszg.sc[i].name);
  delete what;
  what = new char [len];
  for(i = 0, tail = what; i < ppszg.sc.size(); i++) {
    if(ppszg.sc[i].name[0] != '\0') {
	sprintf(tail, " %s", ppszg.sc[i].name);
	tail += strlen(tail);
    }
  }
  return((tail==what) ? blank : what+1);
}


void parti_setpath( int nframes, struct wfframe *frames, float fps = 30, float *focallens = 0 ) {
  struct wfpath *path = &ppszg.path;
  if(path->frames != NULL)
    free(path->frames);
  if(path->focallens != NULL)
    free(path->focallens);
  path->frames = NULL;
  path->fps = (fps <= 0 ? 30.0 : fps);
  path->nframes = nframes;
  path->frame0 = 1;
  path->curframe = path->frame0;
  if(nframes > 0) {
    path->frames = NewN( struct wfframe, nframes );
    memcpy( path->frames, frames, nframes*sizeof(struct wfframe) );
    if(focallens != 0) {
	path->focallens = NewN( float, nframes );
	memcpy(path->focallens, focallens, nframes*sizeof(float) );
    }
  }
}

int parti_readpath( const char *fname ) {
  int nframes, room;
  int first = 1;
  struct wfframe *fr, *cf;
  FILE *f;
  char line[256], *cp;
  int lno = 0;
  int got;
  float *focallens;
  int nfocallens = 0;

  if(fname == NULL || (f = fopen(fname, "r")) == NULL) {
    msg("readpath: %.200s: cannot open: %s", fname, strerror(errno));
    return 0;
  }

  nframes = 0;
  room = 2000;
  fr = NewN( struct wfframe, room );
  focallens = NewN( float, room );

  while(fgets(line, sizeof(line), f) != NULL) {
    lno++;
    for(cp = line; isspace(*cp); cp++)
	;
    if(*cp == '#' || *cp == '\0') continue;
    if(nframes >= room) {
	room *= 3;
	fr = RenewN( fr, struct wfframe, room );
	focallens = RenewN( focallens, float, room );
    }
    cf = &fr[nframes];
    got = sscanf(cp, "%f%f%f%f%f%f%f%f",
	    &cf->tx,&cf->ty,&cf->tz, &cf->rx,&cf->ry,&cf->rz, &cf->fovy, &focallens[nframes]);
    if(got < 7) {
	msg( "readpath: %.200s line %d: expected tx ty tz rx ry rz fovy [focallen], got: %.120s",
	    fname, lno, line);
	fclose(f);
	return 0;
    }
    if(got == 8)
	nfocallens++;
    nframes++;
  }
  fclose(f);
  parti_setpath( nframes, fr, 30, nfocallens==nframes ? focallens : 0 );
  Free(fr);
  Free(focallens);
  parti_setframe( 1 );
  msg("%d frames in %.200s", nframes, fname);
  return 1;
}


void wf2c2w( Matrix *Tcam, const struct wfframe *wf )
{
  Matrix Rx, Ry, Rz, T;

  mtranslation( &T, wf->tx, wf->ty, wf->tz );
  mrotation( &Ry,  wf->ry, 'y' );
  mrotation( &Rz, wf->rz, 'z' );
  mrotation( &Rx, wf->rx, 'x' );

  mmmul( Tcam, &Rz, &Rx );
  mmmul( Tcam, Tcam, &Ry );
  mmmul( Tcam, Tcam, &T );	/* so Tcam = Rz * Rx * Ry * T */
}

int parti_frame( CONST char *frameno, CONST struct wfpath **pp ) {
  if(pp != NULL)
    *pp = &ppszg.path;

  if(frameno != NULL) {
    parti_play( "stop" );
    return parti_setframe( atoi(frameno) );
  }

  return (ppszg.path.frames == NULL || ppszg.path.nframes <= 0)
	? -1 : ppszg.path.curframe;
}


int parti_setframe( int fno ) {

  struct wfpath *path = &ppszg.path;

  if(path->frames == NULL || path->nframes <= 0)
    return -1;
  if(fno < path->frame0)
    fno = path->frame0;
  else if(fno >= path->frame0 + path->nframes)
    fno = path->frame0 + path->nframes - 1;

  Matrix Tc2w;
  wf2c2w( &Tc2w, &path->frames[fno - path->frame0] );
  parti_setc2w( &Tc2w );
  // ... angyfov( path->frames[fno - path->frame0].fovy );
  path->curframe = fno;
  if(path->focallens != NULL)
    ppszg.focallen( path->focallens[fno - path->frame0] );

  return fno - path->frame0;
}

void parti_center( Point *cen )
{
  ppszg.scene->center( cen );
}

void parti_getcenter( Point *cen ) {
    *cen = *ppszg.scene->center();
}


void parti_set_speed( struct stuff *st, double speed ) {
    // nop
}

void parti_set_timebase( struct stuff *st, double timebase ) {
  char str[32];
  ppszg.timebasetime = timebase;
  sprintf(str, "%.16lg", timebase);
  parti_set_timestep( st, clock_time( st->clk ) );
}

void parti_set_timestep( struct stuff *st, double timestep ) {
  // nop
}

void parti_set_fwd( struct stuff *st, int fwd ) {
    // nop
}
