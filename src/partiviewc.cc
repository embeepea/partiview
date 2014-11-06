/*
 * UI-related glue functions for partiview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */
#include "config.h"
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
#include <png.h>

#include "specks.h"
#include "partiview.H"
#include "partiviewc.h"

#ifndef FLHACK
# include <FL/glut.H>	/* for GLUT_STEREO if FLTK knows it */
#endif

#ifndef GLUT_STEREO
# if unix
#  include <FL/x.H>	/* otherwise, need GLX symbols for X-specific stereo */
#  include <GL/glx.h>
# endif
#endif


#include <ctype.h>
#undef isspace		/* hack for irix 6.5 */
#undef isdigit

#ifdef FLHACK
extern void _pview_pickmsg( const char *fmt, ... );
#endif

/* Handle pick results */
void specks_picker( Fl_Gl_Window *junk, int nhits, int nents, GLuint *hitbuf, void *vview )
{
  Fl_Gview *view = (Fl_Gview *)vview;
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
#ifdef FLHACK
    _pview_pickmsg(fmt, bestid, centerit?"*":"",
	bestpos.x[0],bestpos.x[1],bestpos.x[2], wpostr,
	bestsl->text ? bestsl->text : sp->title,
	vlength( &cpos ), nhits,
	attrs );
#else
    msg(fmt, bestid, centerit?"*":"",
	bestpos.x[0],bestpos.x[1],bestpos.x[2], wpostr,
	bestsl->text ? bestsl->text : sp->title,
	vlength( &cpos ), nhits,
	attrs );
#endif

    if(centerit) {
	view->center( &wpos );
	if(ppui.censize > 0) view->redraw();
    }
  } else {

#ifdef FLHACK
    _pview_pickmsg("Picked nothing (%d hits)", nhits);
#else
    msg("Picked nothing (%d hits)", nhits);
#endif

  }
}

void draw_specks( Fl_Gview *view, void *vst, void *junk ) {
  struct stuff *st = (struct stuff *)vst;
  if(st->useme) {
    specks_set_timestep( st );
    specks_current_frame( st, st->sl );
    st->inpick = view->inpick();
    drawspecks( st );
    st->inpick = 0;
  }
}

void parti_allobjs( int argc, char *argv[], int verbose ) {
  struct stuff *st = stuffs[0];
  if(parti_parse_args( &st, argc, argv, NULL ))
    return;
  for(int i = 0; i < MAXSTUFF; i++) {
    st = stuffs[i];
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
  if(ppui.view) ppui.view->redraw();
#ifndef FLHACK
  if(ppui.hrdiag->visible_r()) ppui.hrdiag->redraw();
#endif
}


void parti_update() {
  if(ppui.view)
    while(ppui.view->damage())
	Fl::wait(.1);
}

void parti_censize( float newsize ) {
  if(newsize != ppui.censize) ppui.view->redraw();
  ppui.censize = newsize;
}

float parti_getcensize() {
  return ppui.censize;
}

char *parti_bgcolor( const char *rgb ) {
  static char bgc[20];
  Point bgcolor;

  bgcolor = *ppui.view->bgcolor();
  if(rgb) {
    if(1 == sscanf(rgb, "%f%*c%f%*c%f", &bgcolor.x[0], &bgcolor.x[1], &bgcolor.x[2]))
	bgcolor.x[1] = bgcolor.x[2] = bgcolor.x[0];
    ppui.view->bgcolor( &bgcolor );
  }

  sprintf(bgc, "%.3f %.3f %.3f", bgcolor.x[0],bgcolor.x[1],bgcolor.x[2]);
  return bgc;
}

static void set_wanted_dspcontext( Fl_Gview *view, int detached )
{
  view->dspcontext( (detached ? 2 : 0) + (view->stereo() == GV_QUADBUFFERED ? 1 : 0) );
}

#ifdef GLUT_STEREO

static int stereo_mode = FL_STEREO | FL_RGB | FL_DOUBLE | FL_DEPTH | FL_MULTISAMPLE;

int try_quad_stereo()
{
    return(Fl_Gl_Window::can_do( stereo_mode ));
}

#else  /* backward compatibility -- quadbuffered stereo under Unix/X only */

static int *stereo_mode = NULL;

#if defined(unix) && !defined(FLHACK)

static int attrs[] = {
#ifdef GLX_SAMPLE_BUFFERS_SGIS
    GLX_SAMPLE_BUFFERS_SGIS, 1,
    GLX_SAMPLES_SGIS, 4,
#endif
    GLX_STEREO,
    GLX_RGBA,
    GLX_DOUBLEBUFFER,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_DEPTH_SIZE, 1,
    None
};

int try_quad_stereo()
{
    stereo_mode = attrs;
#ifdef GLX_SAMPLE_BUFFERS_SGIS
    if(Fl_Gl_Window::can_do( stereo_mode ))
	return 1;
    stereo_mode += 4;
#endif
    if(Fl_Gl_Window::can_do( stereo_mode ))
	return 1;
    return 0;
}

#else  /* win32 or FLHACK */
int try_quad_stereo() { return 0; }
#endif

#endif


char *parti_stereo( const char *ster )
{
  static char rslt[28];
  static enum Gv_Stereo stereotype = GV_MONO;
  static int quadmode = -1;
  static const char *sternames[] = { "mono", "redcyan", "crosseyed", "glasses", "left", "right" };
  int smode = -1;

  if(stereotype == GV_MONO) {
    quadmode = try_quad_stereo();
    stereotype = (quadmode!=0) ? GV_QUADBUFFERED : GV_REDCYAN;
  }

  if(ster) {
    float v;
    int pixoff;
    const char *cp = strchr(ster, '@');
    if(cp && sscanf(cp+1, "%d", &pixoff)) {
	ppui.view->stereooffset(pixoff);
	smode = stereotype;
    }
    const char *sp = strpbrk(ster, "-.0123456789");
    if(sp && (cp==NULL || sp<cp) && sscanf(sp, "%f", &v)) {
	ppui.view->stereosep(v);
	smode = stereotype;
    }
    if(strstr(ster,"on"))
	smode = stereotype;
    if(strstr(ster,"off"))
	smode = GV_MONO;
    if(strstr(ster, "red"))
	smode = stereotype = GV_REDCYAN;
    if(strstr(ster, "quad") || strstr(ster, "gla") ||
				strstr(ster, "hard") || strstr(ster, "hw")) {
	if(quadmode > 0) {
	    smode = stereotype = GV_QUADBUFFERED;
	} else {
	    msg("Can't do quadbuffered/hardware stereo");
	    smode = GV_MONO;
	}
    }
    else if(strstr(ster, "cross"))
	smode = stereotype = GV_CROSSEYED;
    else if(strstr(ster, "left"))
	smode = stereotype = GV_LEFTEYE;
    else if(strstr(ster, "right"))
	smode = stereotype = GV_RIGHTEYE;
    if(smode < 0) {
	msg("stereo: [on|off|redcyan|glasses|crosseyed|left|right] [stereosep[@pixeloffset]]");
	msg(" Try stereosep values from 0.02 to 0.1, or -.02 .. -.1 to swap eyes");
    }
  }

  if(smode >= 0) {
    if(smode == (int)GV_QUADBUFFERED)
	ppui.view->glmode( stereo_mode );
    else
        ppui.view->glmode( FL_RGB | FL_DOUBLE | FL_DEPTH | FL_MULTISAMPLE );
    ppui.view->stereo( (enum Gv_Stereo)smode );
    set_wanted_dspcontext( ppui.view, (ppui.view == ppui.freewin)/*detached*/ );
  }

  smode = ppui.view->stereo();
  sprintf(rslt, "%g %s",
	ppui.view->stereosep(),
	(smode<=0||smode>=COUNT(sternames)) ? "off" : sternames[smode]);
  int pixoff = ppui.view->stereooffset();
  if(pixoff)
	sprintf(rslt+strlen(rslt), "@%d", pixoff);
  
  return rslt;
}

int y_root( Fl_Widget *w ) {
  if(w == NULL) return -1;
  int y = w->y();
  Fl_Window *pa;
  for(pa = w->window(); pa != NULL; pa = pa->window())
    y += pa->y();
  return y;
}

Fl_Gview *make_view_window() {
  if(ppui.freewin == NULL) {
    Fl_Group::current( NULL );
    Fl_Double_Window *o = new Fl_Double_Window(512, 512);
    ppui.freemain = o;
    ppui.freewin = new Fl_Gview(0, 0, 512, 512);
    o->add( ppui.freewin );
    Fl_Group::current( NULL );
  }
  return ppui.freewin;
}

void parti_detachview( const char *how ) {
  int seeui = 1;
  if(how == NULL) how = "fullscreen";
  Fl_Gview *w = ppui.view;

//fprintf(stderr, "detach(%s)\n", how);
  switch(how[0]) {
  case 'h': /* full screen, hidden UI */
	seeui = 0;
  case 'f': /* fullscreen detached window */
	w = make_view_window();
	ppui.freemain->fullscreen();
	parti_update();
	ppui.freemain->show();
	w->show();
	parti_update();
	w->resize( 0, 0, ppui.freemain->w(), ppui.freemain->h() );
	ppui.freemain->init_sizes();
	break;

  case 'd': /* ordinary detached window */
	w = make_view_window();
	if(ppui.view && ppui.view->w() * ppui.view->h() > 0) {
	    ppui.freemain->fullscreen_off(50, 50, ppui.view->w(), ppui.view->h());
	    w->size( ppui.view->w(), ppui.view->h() );
	} else {
	    ppui.freemain->fullscreen_off(50, 50, 512, 512);
	}
	ppui.freemain->show();
	if(w->window() && w->window()->visible()) w->show();
	break;

  default:
	msg("detach: huh?  expected \"detach fullscreen\" or \"detach hide\" or just \"detach\"");
	break;
  }
  if(w != NULL) {
    int mainy0 = y_root( ppui.mainwin );
    int viewy0 = y_root( ppui.cmd ) + ppui.cmd->h();
    int mainheight = viewy0 - mainy0;
    Fl_Widget *resiz = ppui.maintile->resizable();
    if(resiz && resiz != ppui.maintile) {
	ppui.maintile->resizable(ppui.maintile);
	ppui.maintile->remove( resiz );
    }
    ppui.mainwin->size( ppui.mainwin->w(), mainheight );
    ppui.mainwin->init_sizes();
    ppui.maintile->init_sizes();
    ppui.maintile->redraw();
    ppui.mainwin->redraw();
    // parti_update();
    if(w != ppui.view) {
	w->initfrom( ppui.view );
	ppui.view = w;
	set_wanted_dspcontext( ppui.view, 1 );
	/* Fix up dangling references to old view */
	ppui.view->picker( specks_picker, ppui.view );
	if(ppui.hrdiag) ppui.hrdiag->picker( specks_picker, ppui.view );
    }
    ppui.detached = how[0];	// 'h' or 'f' or 'd'
    parti_redraw();
    ppui.boundwin->hide();
    if(seeui) {
	ppui.mainwin->set_non_modal();
	ppui.mainwin->show();		// pop control strip to top
	ppui.mainwin->redraw();
    } else {
	ppui.view->show();
	ppui.mainwin->hide();
    }
    ppui.view->redraw();
  }

  /* Is there a main-window-position spec included in our param? */
  const char *cp;
  for(cp = how; *cp; cp++) {
      if(*cp == '-' || *cp == '+') {
	  char cx[2], cy[2];
	  int xpos, ypos;
	  if(4 == sscanf(cp, "%1[-+]%d%1[-+]%d", cx, &xpos, cy, &ypos)) {
	      Fl_Window *mw = ppui.mainwin;
	      mw->position( cx[0]=='+' ? xpos : Fl::w() - xpos - mw->w(),
		      	    cy[0]=='+' ? ypos : Fl::h() - ypos - mw->h() );
	      break;
	  }
      }
  }

}

char *parti_winsize( CONST char *newsize ) {
  static char cursize[24];
#ifdef FLHACK
  sprintf(cursize, "-1x-1");
#else
  int posx = 0, posy = 0, has_pos = 0;

//fprintf(stderr, "winsize(%s)\n", newsize);
  Fl_Window *top = (ppui.freewin != NULL) ?
		ppui.freemain : ppui.mainwin;

  int ox = ppui.view->w(), oy = ppui.view->h();
  if(!top->shown()) {
    ox = oy = -1;
  }

  if(newsize) {
    int nx = ox, ny = oy;
    const char *posp = strpbrk(newsize, "-+");
    char cx[2], cy[2];
    posx = top->x();
    posy = top->y();
    if(posp != newsize && sscanf(newsize, "%d%*c%d", &nx, &ny) == 1)
	ny = nx * oy / ox;
    if(posp && 4 == sscanf(posp, "%1[-+]%d%1[-+]%d", cx, &posx, cy, &posy)) {
	if(cx[0] == '-') posx = Fl::w() - nx - posx;
	if(cy[0] == '-') posy = Fl::h() - ny - posy;
	has_pos = 1;
    }
    if(ox < 0) {		/* Not yet available -- remember for later */
	strncpy(cursize, newsize, sizeof(cursize)-1);
	ppui.reqwinsize = cursize;
	return cursize;
    }
    if(nx != ox || ny != oy || has_pos) {
	if(ppui.freewin != NULL) {
	    parti_detachview( ppui.detached=='h' ? "hide" : "detach" );
	    top->border( 1 );
	    if(has_pos) top->position( posx, posy );
	    top->size( nx, ny );
	    ppui.view->size( nx, ny );
	} else {
	    if(has_pos)
		top->position( posx, posy );
	    top->size( top->w() + nx - ox,
		       top->h() + ny - oy );
	    ppui.view->size( nx, ny );
	}
	if(top->visible())
	    ppui.view->show();
	parti_update();
	return parti_winsize(NULL);
    }
  }
  sprintf(cursize, has_pos ? "%dx%d+%d+%d" : "%dx%d", ox, oy, posx, posy);
#endif
  return cursize;
}

char *parti_clip( const char *nearclip, const char *farclip ) {
  float n, f;
  int changed = 0;
  if(nearclip != NULL && sscanf(nearclip, "%f", &n) > 0) {
    ppui.view->nearclip( n );
    changed = 1;
  }
  if(farclip != NULL && sscanf(farclip, "%f", &f) > 0) {
    ppui.view->farclip( f );
    changed = 1;
  }
  if(changed && ppui.subcam != 0)
    parti_select_subcam( ppui.subcam );	/* recompute subcam projection */

  static char why[16];
  sprintf(why, "clip %.4g %.4g", ppui.view->nearclip(), ppui.view->farclip());
  return why;
}

int parti_move( const char *onoffobj, struct stuff **newst ) {
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
}

float parti_pickrange( char *newrange ) {
  if(newrange) {
    if(sscanf(newrange, "%f", &ppui.pickrange))
	ppui.view->picksize( 4*ppui.pickrange, 4*ppui.pickrange );
  }

  return ppui.pickrange;
}

static int endswith(const char *str, const char *suf) {
  if(str==NULL || suf==NULL) return 0;
  int len = strlen(str);
  int suflen = strlen(suf);
  return suflen <= len && memcmp(suf, str+len-suflen, suflen)==0;
}

int parti_snapset( char *fname, char *frameno, char *imgsize )
{
  int len;
#ifdef HAVE_PNG_H
  static char suf[] = ".%04d.png";
#elif unix
  static char suf[] = ".%04d.ppm.gz";
#else /* WIN32 */
  static char suf[] = ".%04d.ppm";
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
    if(ppui.snapfmt)  Free(ppui.snapfmt);
    ppui.snapfmt = NewN(char, len+needsuf+1);
    sprintf(ppui.snapfmt, needsuf ? "%s%s" : "%s", fname, suf);
  }
  if(frameno)
    sscanf(frameno, "%d", &ppui.snapfno);
  return ppui.snapfno;
}

#ifdef HAVE_JPEGLIB_H
/* jpeg image snapshotting */


#ifdef WIN32
typedef unsigned char  boolean;
#endif
extern "C" {
#include <jpeglib.h>
};

static struct jpeg_compress_struct cinfo;
static struct jpeg_error_mgr jerr;


struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

static struct my_error_mgr myjerr;

static void snapjpegerr(j_common_ptr cinfo)
{
  my_error_ptr myerr = reinterpret_cast<my_error_ptr>(cinfo->err);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


static int snapjpeg( char *outfname, int xsize, int ysize, char *rgbbuf )
{
    FILE *outf = fopen(outfname, "wb");
    if(outf == 0) {
	msg("Can't open output image %s", outfname);
	return 1;
    }

    cinfo.err = jpeg_std_error(reinterpret_cast<jpeg_error_mgr*>(&myjerr));
    jpeg_create_compress(&cinfo);

    cinfo.image_width = xsize;      /* image width and height, in pixels */
    cinfo.image_height = ysize;
    cinfo.input_components = 3;     /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, ppui.jpegqual, TRUE/*force_baseline*/);

    cinfo.err = jpeg_std_error(&myjerr.pub);
    myjerr.pub.error_exit = snapjpegerr;

    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(myjerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file, and return.
	 */

	/* Display the error message too. */
	(*cinfo.err->output_message) (reinterpret_cast<jpeg_common_struct*>(&cinfo));

	jpeg_destroy_compress(&cinfo);
	fclose(outf);
	return 1;
    }

    jpeg_stdio_dest(&cinfo, outf);

    jpeg_start_compress(&cinfo, TRUE);

    while(cinfo.next_scanline < cinfo.image_height) {
	JSAMPROW rowptr[1];
	rowptr[0] = reinterpret_cast<JSAMPROW>(&rgbbuf[(cinfo.image_height - 1 - cinfo.next_scanline) * xsize*3]);
	jpeg_write_scanlines(&cinfo, rowptr, 1);
    }
    jpeg_finish_compress(&cinfo);
    
    int iook;
    iook = fclose(outf);

    jpeg_destroy_compress(&cinfo);

    return iook ? 1 : 0;	/* return 1 if fclose() reported error, 0 if success */
}
#endif /*HAVE_JPEGLIB_H*/


#ifdef HAVE_PNG_H

/* png image snapshotting */
static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

static void failpng() {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    msg("snapshot: error writing png header");
}

/* returns 0 on success, nonzero on failure */
static int snappng( char *outfname, int xsize, int ysize, char *rgbbuf )
{
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr)
	info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
	msg("snapshot: can't init png library");
	return 1;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
	failpng();
	return 1;
    }

    FILE *outf = fopen( outfname, "wb" );
    if(outf == 0) {
	return 1;
    }

    png_init_io( png_ptr, outf );

    png_set_IHDR( png_ptr, info_ptr,
		xsize, ysize, 8,/*bit depth*/
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

    /* Don't really know that we're in sRGB color space, but let's say so anyway. */
    png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr, PNG_INFO_sRGB);

    png_write_info( png_ptr, info_ptr );

    png_bytep *rowps = new png_bytep[ysize];
    for(int k = 0; k < ysize; k++)
	rowps[ysize-k-1] = (png_bytep) &rgbbuf[k*xsize*3];
    png_write_image( png_ptr, rowps );
    delete rowps;

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(outf);
    return 0;
}

#endif /* HAVE_PNG_H */


int parti_snapshot( char *snapinfo )
{
  char tfcmd1[10240], tfcmd2[10240], *tftail;
  int fail;
  enum imtype { AS_PNG, AS_JPEG, AS_OTHER } astype = AS_OTHER;

#if defined(HAVE_PNG_H) || !WIN32
  static char defsnap[] = "snap.%04d.png";
#else
  static char defsnap[] = "snap.%04d.ppm";
#endif

#if !WIN32  /* if unix */
  static char prefix[] = "|convert ppm:- ";
  static char gzprefix[] = "|gzip >";
#else
  static char prefix[] = "";
#endif

  if(snapinfo) snapinfo[0] = '\0';
  if(ppui.snapfmt == NULL) {
    ppui.snapfmt = NewN(char, sizeof(defsnap)+1);
    strcpy(ppui.snapfmt, defsnap);
  }
  
  if(ppui.snapfmt[0] == '|' || endswith(ppui.snapfmt, ".ppm")) {
    tfcmd1[0] = '\0';

#ifdef HAVE_PNG_H
  } else if(endswith(ppui.snapfmt, ".png")) {
    /* use built-in libpng writer if available */
    astype = AS_PNG;
    tfcmd1[0] = '\0';
#endif

#ifdef HAVE_JPEGLIB_H
  } else if(endswith(ppui.snapfmt, ".jpeg") || endswith(ppui.snapfmt, ".jpg")) {
    /* use built-in libjpeg writer if available */
    astype = AS_JPEG;
    tfcmd1[0] = '\0';
#endif

#if unix
  } else if(endswith(ppui.snapfmt, ".ppm.gz")) {
    strcpy(tfcmd1, gzprefix);
#endif
  } else {
    strcpy(tfcmd1, prefix);
  }
  tftail = tfcmd1+strlen(tfcmd1);
  sprintf(tftail, ppui.snapfmt, ppui.snapfno);
  if(!ppui.view || !ppui.view->visible_r()) {
    msg("snapshot: no visible graphics window?");
    return -2;
  }

  Fl_Widget *pa;
  for(pa = ppui.view; pa->parent(); pa = pa->parent())
    ;
  pa->show();	// raise window

  bool snapstereo = (strchr(tfcmd1, '@') != NULL);
  enum Gv_Stereo stereowas = ppui.view->stereo();
  char *tfcmd = snapstereo ? tfcmd2 : tfcmd1;

  int y, h = ppui.view->h(), w = ppui.view->w();
  char *buf = (char *)malloc(w*h*3);

  for(int eye = 0; eye < (snapstereo ? 2 : 1); eye++) {
    char eyech = 'L';
    if(snapstereo) {
	ppui.view->stereo( eye==0 ? GV_LEFTEYE : GV_RIGHTEYE );
	strcpy(tfcmd, tfcmd1);
	// replace all instances of "@" with "L" or "R"
	char eyech = eye==0 ? 'L' : 'R';
	for(char *s = tfcmd; (s = strchr(s, '@')) != NULL; s++)
	    *s = eyech;
    }

    // Ensure window's image is up-to-date
    parti_update();

    if(!ppui.view->snapshot( 0, 0, w, h, buf )) {
	free(buf);
	msg("snapshot: couldn't read from graphics window?");
	fail = -2;
	break;
    }

    switch(astype) {
    case AS_PNG:
#ifdef HAVE_PNG_H
	fail = snappng( tfcmd, w, h, buf );
#endif
	break;

#ifdef HAVE_JPEGLIB_H
    case AS_JPEG:
	fail = snapjpeg( tfcmd, w, h, buf );
	break;
#endif

    default:
	 /* write ppm stream/file */
	FILE *p;
#if unix
	void (*oldpipe)(int) = 0;
	int popened = tfcmd[0] == '|';
	if(popened) {
	    oldpipe = signal(SIGPIPE, SIG_IGN);
	    p = popen(tfcmd+1, "w");
	} else {
	    p = fopen(tfcmd, "wb");
	}

	fprintf(p, "P6\n%d %d\n255\n", w, h);
	for(y = h; --y >= 0 && fwrite(&buf[w*3*y], w*3, 1, p) > 0; )
	    ;
	fflush(p);
	fail = ferror(p) || y >= 0;

	if(popened) {
	    pclose(p);
	    signal(SIGPIPE, oldpipe);
	}
	else
	    fclose(p);

#else  /* win32 */
	p = fopen(tfcmd, "wb");

	fprintf(p, "P6\n%d %d\n255\n", w, h);
	for(y = h; --y >= 0 && fwrite(&buf[w*3*y], w*3, 1, p) > 0; )
	    ;
	fflush(p);
	fail = ferror(p) || y >= 0;
	fclose(p);
#endif
	/* end of "write ppm" case */
	break;
    }

    if(fail) {
	msg("snapshot: Error writing to %s", tfcmd);
	break;
    }
  }
  free(buf);

  if(snapstereo)	// restore changed stereo setting
    ppui.view->stereo( stereowas );

  if(snapinfo) {
    if(fail)
	sprintf(snapinfo, "failed snapping %.900s", tftail);
    else if(snapstereo)
	sprintf(snapinfo, "%.1000s [%dx%d] @ => stereo L,R", tftail, w, h);
    else
	sprintf(snapinfo, "%.1000s [%dx%d]", tftail, w, h);
  }

  return fail ? -1 : ppui.snapfno++;
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

  if(newst)
    *newst = ppui.st;

  if(objname == NULL) {
    return parti_idof( ppui.st );
  }
  if(ppui.view == NULL)
    return OBJNO_NONE;
  if(!strcmp(objname, "c") || !strcmp(objname, "c0")) {
    ppui.view->target( GV_ID_CAMERA );
    return OBJNO_NONE;
  }
  c = '=';
  if((		/* accept gN or gN=... or N or N=... or [gN] */
	((sscanf(objname, "g%d%c", &i, &c) > 0 || sscanf(objname, "%d%c", &i, &c) > 0) && c == '=')
	|| sscanf(objname, "[g%d]", &i) > 0)) {
	gname = 1;
  } else {
    for(i = MAXSTUFF; --i >= 0; ) {
	if(stuffs[i] && stuffs[i]->alias && !strcmp(stuffs[i]->alias, objname))
	    break;
    }
  }
  if(i == -1 && create && !gname) {
    for(i = 1; i < MAXSTUFF && stuffs[i]; i++)
	;
  }
  if(i >= MAXSTUFF) {
    msg("Oops, can only have objects g1..g%d", MAXSTUFF-1);
    return OBJNO_NONE;
  }
  if(i > 0) {
    if(stuffs[i] == NULL) {
	stuffs[i] = specks_init( 0, NULL );
	stuffs[i]->clk = ppui.clk;
	ppui.view->add_drawer( draw_specks, stuffs[i], NULL, NULL, i );
	ppui.view->picker( specks_picker, ppui.view );
	ppui.view->picksize( 4*ppui.pickrange, 4*ppui.pickrange );
	char tname[12];
	sprintf(tname, "[g%d]", i);
	int me = ppui.obj->add( strdup(tname) );

	((Fl_Menu_Item *)ppui.obj->menu())[me].labelcolor( ppui.obj->labelcolor() );
	if(!gname)
	    parti_set_alias(stuffs[i], (char *)objname);
    }
    ppui.st = stuffs[i];
    ppui.view->target( i );
    ppui_refresh( ppui.st );
    if(newst) *newst = stuffs[i];
    return i;
  }
  msg("Don't understand object name \"%s\"", objname);
  return OBJNO_NONE;
}

int parti_idof( struct stuff *st ) {
  for(int i = 0; i < MAXSTUFF; i++)
    if(stuffs[i] == st) return i;
  return OBJNO_NONE;
}

float parti_focal( CONST char *newfocal ) {
  float focallen;
  if(newfocal && sscanf(newfocal, "%f", &focallen) > 0)
    ppui.view->focallen( focallen );
  return ppui.view->focallen();
}

int parti_focalpoint( int argc, char **argv, Point *focalpoint, float *minfocallen )
{
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
}


float parti_fovy( CONST char *newfovy ) {
  float fovy;
  if(newfovy && sscanf(newfovy, "%f", &fovy) > 0)
    if(ppui.view->perspective())
	ppui.view->angyfov( fovy );
    else
	ppui.view->halfyfov( 2*fovy );

  return ppui.view->perspective() ? ppui.view->angyfov()
				  : 2 * ppui.view->halfyfov();
}

float parti_getpixelaspect() {
  return ppui.view ? ppui.view->pixelaspect() : 1.0;
}

void parti_setpixelaspect( float pixasp ) {
  if(ppui.view)
    ppui.view->pixelaspect( pixasp );
}


void parti_getc2w( Matrix *c2w ) {
  if(ppui.view && c2w)
    *c2w = *ppui.view->Tc2w();
}

void parti_setc2w( const Matrix *c2w ) {
  ppui.view->Tc2w( c2w );
}

void parti_seto2w( struct stuff *, int objno, const Matrix *o2w ) {
  ppui.view->To2w( objno, o2w );
}

void parti_geto2w( struct stuff *, int objno, Matrix *o2w ) {
  *o2w = *ppui.view->To2w( objno );
}

void parti_parent( struct stuff *st, int parent ) {
  ppui.view->objparent( parti_idof(st), parent );
}

void parti_nudge_camera( Point *disp ) {
  Matrix Tc2w, Tdisp, Tnewc2w;

  if(ppui.view) {
    Tc2w = *ppui.view->Tc2w();
    mtranslation( &Tdisp, disp->x[0], disp->x[1], disp->x[2] );
    mmmul( &Tnewc2w, &Tc2w,&Tdisp );
    ppui.view->Tc2w( &Tnewc2w );
  }
}

void subcam_matrices( Subcam *sc, float sctilt, Matrix *Tc2subc, float subclrbt[4] )
{
  Matrix Ry, Rx, Rz, Rflop, Tfy, Tfyx;
  mrotation( &Rflop, 90-sctilt, 'x' );
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
    tsc = ppui.sc[index-1];

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
    for(i = 0; i < ppui.scroom && ppui.sc[i].name[0] != '\0'; i++)
	;
    if(i >= ppui.scroom) {  /* enlarge */ 
	int newroom = ppui.scroom>0 ? 2*ppui.scroom+5 : 30;
	ppui.sc = RenewN( ppui.sc, Subcam, newroom );
	if(ppui.sc == 0) {
	    perror("Ran out of memory allocating subcam");
	    ppui.scroom = 0;
	    ppui.sc = 0;
	    return 0;
	}
	memset( &ppui.sc[ppui.scroom], 0, (newroom - ppui.scroom) * sizeof(Subcam) );
	i = ppui.scroom;
	ppui.scroom = newroom;
    }
    index = i;
  }

  sprintf(tsc.name, "%.7s", name);
  ppui.sc[index] = tsc;
  return index+1;
}

int parti_subcam_named( CONST char *name )
{
  for(int i = 0; i < ppui.scroom && ppui.sc[i].name[0] != '\0'; i++)
    if(0==strcmp(ppui.sc[i].name, name))
	return i+1;
  return 0;
}

int parti_select_subcam( int index )
{
  if(index < 0 || index > ppui.scroom)
    return ppui.subcam;

  ppui.subcam = index;
  if(index == 0) {
    ppui.view->usesubcam(0);
    parti_redraw();
    return 0;
  }

  Subcam *tsc = &ppui.sc[index-1];

  Matrix Tc2subc;
  float subclrbt[4];
  subcam_matrices( tsc, ppui.sctilt, &Tc2subc, subclrbt );
  ppui.view->Tc2subc( &Tc2subc );
  ppui.view->subc_lrbt( subclrbt );
  ppui.view->usesubcam(1);
  parti_redraw();
  return index;
}

const char *parti_get_subcam( int index, char *paramsp )
{
  if(paramsp) paramsp[0] = '\0';
  if(index == 0 || index > ppui.scroom) return "";
  Subcam *sc = &ppui.sc[index-1];
  if(sc->name[0] == '\0') return "";
  if(paramsp)
    sprintf(paramsp, "%g %g %g %g %g %g %g",
	sc->azim, sc->elev, sc->roll,
	sc->nleft, sc->right, sc->ndown, sc->up);
  return sc->name;
}

int parti_current_subcam( void )
{
  return ppui.subcam;
}

char *parti_subcam_list()
{
  static char *what = 0;
  static char blank[2] = "";
  static int whatroom = 0;
  char *tail;
  int i, len;

  what[0] = '\0';

  for(i = 0, len = 2; i < ppui.scroom && ppui.sc[i].name[0] != '\0'; i++) {
    len += strlen(ppui.sc[i].name) + 1;
  }
  if(len > whatroom) {
    whatroom = len*2 + 1;
    what = (char *)realloc(what, whatroom);
  }
  
  for(i = 0, tail = what; i < ppui.scroom && ppui.sc[i].name[0] != '\0'; i++) {
    sprintf(tail, " %s", ppui.sc[i].name);
    tail += strlen(tail);
  }
  return((tail==what) ? blank : what+1);
}

struct Fl_Plot *parti_register_plot( struct stuff *st, void (*draw)(struct Fl_Plot *, void *obj, void *arg), void *arg ) {
    if(ppui.hrdiag) {
	ppui.hrdiag->add_drawer( draw, st, arg, parti_get_alias(st), parti_idof(st) );
	ppui.hrdiag->picker( specks_picker, ppui.view );
	ppui.hrdiag->picksize( 2*ppui.pickrange, 2*ppui.pickrange );
    }
    return ppui.hrdiag;
}

void parti_hrdiag_on( int on ) {
  if(on) {
    ppui.hrdiagwin->show();
    ppui.hrdiag->show();
  } else {
    ppui.hrdiagwin->hide();
  }
}

int parti_inertia( int on ) {
  if(on >= 0)
    ppui.view->inertia( on );
  return ppui.view->inertia();
}

void parti_setpath( int nframes, struct wfframe *frames, float fps = 30, float *focallens = 0 ) {
  struct wfpath *path = &ppui.path;
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

  if(ppui.playframe) {
    ppui.playframe->minimum( path->frame0 );
    ppui.playframe->maximum( path->frame0 + path->nframes - 1 );
    ppui.playframe->value( path->frame0 );

    ppui.playtime->step( 1, 100 );
    ppui.playtime->minimum( 0.0 );
    ppui.playtime->maximum( path->nframes / path->fps );
    ppui.playtime->value( 0.0 );
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

static void fd_got_data( int fd, void * ) {
  specks_check_async( &ppui.st );
}

#ifndef FLHACK
void parti_asyncfd(int fd) { Fl::add_fd( fd, fd_got_data ); }
void parti_unasyncfd(int fd) { Fl::remove_fd(fd); }
#else
void parti_asyncfd(int fd) { }
void parti_unasyncfd(int fd) { }
#endif

static void playidle( void * ) {
  struct wfpath *path = &ppui.path;

  if(!ppui.playing) {
    Fl::remove_idle( playidle, NULL );
    ppui.playidling = 0;
    return;
  }

  double now = wallclock_time();
  
  if(ppui.playspeed == 0)
    ppui.playspeed = 1;

  int frame = ppui.framebase;

  if(ppui.playevery) {
    frame += (int) ((ppui.playspeed < 0)	/* round away from zero */
		    ? ppui.playspeed - .999f
		    : ppui.playspeed + .999f);
    ppui.framebase = frame;
    ppui.playtimebase = now;
  } else {
    frame += (int) ((now - ppui.playtimebase) * ppui.playspeed * path->fps);
  }

  if(frame < path->frame0 || frame >= path->frame0 + path->nframes) {

    frame = (frame < path->frame0)
	  ? path->frame0
	  : path->frame0 + path->nframes-1;

    switch(ppui.playloop) {
    case 0:	/* Not looping; just stop at end. */
	Fl::remove_idle( playidle, NULL );
	ppui.playidling = 0;
	parti_play( "stop" );
	break;

    case -1:	/* Bounce: stick at same end of range, change direction */
	ppui.playspeed = -ppui.playspeed;
	break;

    case 1:	/* Loop: jump to other end of range */
	frame = (frame == path->frame0)
		? path->frame0 + path->nframes-1
		: path->frame0;
    }
    ppui.framebase = frame;
    ppui.playtimebase = now;
    
    parti_setframe( frame );
    return;
  }

  /*
   * Cases:
   *  - we're displaying frames slower than real time.
   *	=> use an idle function.
   *  - we're displaying frames faster than real time.
   *    => use a timeout.
   */

  int needidle = ppui.playidling;
  float dt = 0;

  if(frame != path->curframe) {
    parti_setframe( frame );
    needidle = 1;
  } else {
    frame += (ppui.playspeed > 0) ? 1 : -1;
    dt = (frame - ppui.framebase) / (ppui.playspeed * path->fps)
	+ ppui.playtimebase - now;
    needidle = (dt < .05);
  }

  if(needidle && !ppui.playidling) {
    Fl::add_idle( playidle, NULL );
  }
  if(!needidle && ppui.playidling) {
    Fl::remove_idle( playidle, NULL );
  }
  ppui.playidling = needidle;

  if(!needidle) {
fprintf(stderr, "<%.3f>", dt);
    Fl::add_timeout( dt, playidle, NULL );
  }
}

void parti_play( const char *rate ) {

  struct wfpath *path = &ppui.path;
  int playnow = 1;
  int awaitdone = 0;

  if(rate != NULL) {
    char *ep;
    float sp = strtod(rate, &ep);
    if(strchr(ep, 's')) playnow = 0;
    if(strchr(ep, 'l')) ppui.playloop = 1;
    else if(strchr(ep, 'k')) ppui.playloop = -1; /* rock */
    else if(strchr(ep, 'f') || strchr(ep, 'e')) ppui.playevery = 1;
    else if(strchr(ep, 'r') || strchr(ep, 't')) ppui.playevery = 0;
    if(strstr(rate, "wait"))
	awaitdone = 1;

    if(ep == rate) {
	/* No leading number -- did we get just "-" or "-?" or "-help"? */
	if(0==strcmp(rate, "-") || strchr(rate, '?') || strchr(rate, 'h')) {
	    msg("Usage: play [rate][s][l|k][f|r][wait]  e.g.  \"play\" or \"play 30\" or \"play 10kf wait\"");
	    msg("   rate  frames/sec; [s]et speed, don't play now; [l]oop/roc[k]");
	    msg("         play every [f]rame/skip to maintain [r]ate;  [wait] until done");
	    return;
	}
	if(sp == 0) playnow = 0;
	else ppui.playspeed = sp;
    }
  }

  /* Always stop any ongoing timers */
  Fl::remove_idle( playidle, NULL );
  Fl::remove_timeout( playidle, NULL );
  ppui.playing = 0;
  ppui.playidling = 0;

  if(playnow) {
    ppui.playtimebase = wallclock_time();
    ppui.framebase = path->curframe;
    if(ppui.playspeed > 0 && path->curframe >= path->frame0+path->nframes-1)
	ppui.framebase = path->frame0;
    else if(ppui.playspeed < 0 && path->curframe <= path->frame0)
	ppui.framebase = path->frame0 + path->nframes - 1;

    parti_setframe( ppui.framebase );
    Fl::add_idle( playidle, NULL );
    ppui.playing = 1;
    ppui.playidling = 1;
  }

#ifndef FLHACK
  if(ppui.play != NULL) {
    ppui.play->label( ppui.playing ? "stop" : "play" );
    ppui.play->value( ppui.playing );
    ppui.play->redraw();
  }
#endif //FLHACK

  if(awaitdone) {
      /* Don't let this command return until the "play" is finished, or at least stopped.
       * But allow user input along the way.
       */
      while(ppui.playing)
	  Fl::wait(0.1);
  }
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
    *pp = &ppui.path;

  if(frameno != NULL) {
    parti_play( "stop" );
    return parti_setframe( atoi(frameno) );
  }

  return (ppui.path.frames == NULL || ppui.path.nframes <= 0)
	? -1 : ppui.path.curframe;
}


int parti_setframe( int fno ) {

  struct wfpath *path = &ppui.path;

  if(path->frames == NULL || path->nframes <= 0)
    return -1;
  if(fno < path->frame0)
    fno = path->frame0;
  else if(fno >= path->frame0 + path->nframes)
    fno = path->frame0 + path->nframes - 1;

  Matrix Tc2w;
  wf2c2w( &Tc2w, &path->frames[fno - path->frame0] );
  ppui.view->Tc2w( &Tc2w );
  ppui.view->angyfov( path->frames[fno - path->frame0].fovy );
  path->curframe = fno;
  if(path->focallens != NULL)
    ppui.view->focallen( path->focallens[fno - path->frame0] );

#ifndef FLHACK
  if(ppui.playframe) {
    ppui.playframe->value( fno );
    float t = (fno - path->frame0) / path->fps;
    if(fabs(t - ppui.playtime->value()) * path->fps > .5)
	ppui.playtime->value( t );
  }
#endif //FLHACK

  return fno - path->frame0;
}

void parti_center( const Point *cen )
{
  if(ppui.view)
	ppui.view->center( cen );
}

void parti_getcenter( Point *cen ) {
  if(ppui.view && cen)
    *cen = *ppui.view->center();
}


void parti_set_speed( struct stuff *st, double speed ) {
#ifndef FLHACK
  ppui.stepspeed->truevalue( speed );
#endif //FLHACK
}

void parti_set_timebase( struct stuff *st, double timebase ) {
  char str[32];
  ppui.timebasetime = timebase;
  sprintf(str, "%.16lg", timebase);
#ifndef FLHACK
  ppui.timebase->value( str );
#endif //FLHACK
  parti_set_timestep( st, clock_time( st->clk ) );
}

void parti_set_timestep( struct stuff *st, double timestep ) {
  char str[32];
  sprintf(str, "%.16lg", timestep - ppui.timebasetime);
#ifndef FLHACK
  ppui.timestep->value( str );

  if( ppui.timestep->visible_r() && ppui.timebase->visible_r() ) { //steven marx: version 0.7.0 reduce cmdhist flicker
    ppui.timestep->redraw();
    ppui.timebase->redraw();
  }
#endif //FLHACK

}

void pp_stepper( void *vst ) {
  struct stuff *st = (struct stuff *)vst;
  if(st) {
    clock_tick( st->clk );
    specks_set_timestep( ppui.st );
    parti_redraw();
    Fl::check();
  }
}

void parti_set_running( struct stuff *st, int on ) {
  if(clock_running(st->clk) != on) {
    if(on) Fl::add_idle( pp_stepper, st );
    else Fl::remove_idle( pp_stepper, st );
    if(st->clk) st->clk->walltimed = 1;
    clock_set_running(st->clk, on);
  }
#ifndef FLHACK
  ppui.runstop[0]->value( on && clock_fwd(st->clk) < 0 );
  ppui.runstop[1]->value( on && clock_fwd(st->clk) > 0 );
#endif //FLHACK
}

void parti_set_fwd( struct stuff *st, int fwd ) {
  int fwdbtn = (fwd > 0);
#ifndef FLHACK
  ppui.runstop[!fwdbtn]->value(0);
  ppui.runstop[fwdbtn]->value( clock_running( st->clk ) );
#endif //FLHACK
}
