#ifdef USE_ELUMENS

/*
 * Elumens spiClops immersive display support for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef WIN32
# include "winjunk.h"
#endif

#include "Gview.H"
#include "partiview.H"
#include "partiviewc.h"
#include <math.h>

#include<string.h> //marx added to 0.7.06 to support elumens/spiclops build

#define SPILIB_API 1
#include "spiclopsExt.h"

#if !defined(GLX_VERSION_1_2) && defined(unix)
  /* Hack -- elumens spiclops library needs glXGetCurrentDisplay() */
#include <FL/x.H>
extern "C" {
 Display *glXGetCurrentDisplay() {
  if(fl_display == 0) fl_open_display();
  return fl_display;
 }
}
#endif /*!GLX_VERSION_1_2*/

extern "C" {
  extern void pp_spi_init();
}

struct spichan {
  int code;		/* SPI_1C_FRONT, etc. */
  int pfcode;		/* SPI_PF_1_CHAN, etc. */
  float fovx, fovy;	/* angular field of view */
  int tess;		/* mesh tessellation */
};

struct spistuff {
  void *ctx;
  int on;	// if not, just show normal view
  int fast;	// fast, pbuffered graphics?
  Point Eye;	// offAxisEye
  Point Lens;	// offAxisLens
  int haschanxy, haschanwh;
  float chanxy[2], chanwh[2];
  struct spichan chan[4];
  int nchan;
  int refresh;
};

static struct spistuff spi;

unsigned int nextpow2( int v ) {
  if(v <= 0) return 1;
  unsigned int i;
  for(i = 2; i < v && i != 0; i <<= 1)
    ;
  return i;
}

void pp_spi_predraw( Fl_Gview *view, int passno ) {
  if(!spi.on) return;

  int w = view->w(), h = view->h(), x = view->x(), y = view->y();

  if(!spi.haschanwh && (w != spi.chanwh[0] || h != spi.chanwh[1])) {
    spi.refresh = 1;
    spi.chanwh[0] = w, spi.chanwh[1] = h;
  }

  if(!spi.haschanxy && (x != spi.chanxy[0] || y != spi.chanxy[1])) {
    spi.refresh = 1;
    spi.chanxy[0] = x, spi.chanxy[1] = y;
  }

  if(spi.refresh) {
    int i, allpfchans = 0, allchans = 0;
    for(i = 0; i < spi.nchan; i++) {
	allpfchans |= spi.chan[i].pfcode;
	allchans |= spi.chan[i].code;
    }
    if(spi.ctx == NULL) {
	spi.ctx = spiInitialize( NULL, allpfchans |
			(spi.fast ? SPI_PF_AUTO : SPI_PF_BACKBUFFER) );
    }
    spi.refresh = 0;
    if(spi.fast)
	spiSetPBufferSize( spi.ctx, nextpow2(spi.chanwh[0]), nextpow2(spi.chanwh[1]) );

    spiSetChanOrigin( spi.ctx, allchans, spi.chanxy[0], spi.chanxy[1] );
    spiSetChanSize( spi.ctx, allchans, spi.chanwh[0], spi.chanwh[1] );

    for(i = 0; i < spi.nchan; i++) {
	spiSetChanFOV( spi.ctx, spi.chan[i].code,
				spi.chan[i].fovx, spi.chan[i].fovy );
	spiSetChanTessLevel( spi.ctx, spi.chan[i].code,
				spi.chan[i].tess );
    }
    spiSetChanEyePosition ( spi.ctx, allchans,
	spi.Eye.x[0], spi.Eye.x[1], spi.Eye.x[2] );
    spiSetChanLensPosition( spi.ctx, allchans,
	spi.Lens.x[0], spi.Lens.x[1], spi.Lens.x[2] );
  }

  spiBegin( spi.ctx );
  spiPreRender( spi.ctx, spi.chan[passno].code );
}

int pp_spi_postdraw( Fl_Gview *view, int passno ) {

  if(!spi.on) return 0;

  spiPostRender( spi.ctx, spi.chan[passno].code );
  spiEnd( spi.ctx );

  if(passno < spi.nchan-1)
    return passno+1;

  /* OK, we've rendered all passes, now let spi library make real image */

  glClearColor( 0, 0, .1, 1 );
  glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
  spiFlush( spi.ctx, SPI_ALL_CHAN );
  /* tell caller that we want no more rendering passes */
  return 0;
}

int pp_spi_parse_args( struct stuff **, int argc, char *argv[], char *fname, void * ) {
  if(argc < 1 || (strncmp(argv[0], "elumens", 3)
			&& strcmp(argv[0], "spi")))
    return 0;

  //char *subcmd = argc>1 ? argv[1] : ""
   const char *subcmd = argc>1 ? argv[1] : ""; //marx modified to support elumens/spiclops build for 0.7.06
  
  if(!strcmp(subcmd, "on")) {
    spi.on = 1;

  } else if(!strcmp(subcmd, "off")) {
    spi.on = 0;

  } else if(!strcmp(subcmd, "fast")) {
    spi.fast = getbool( argv[2], 1 );
    msg("fast (pbuffered graphics) %s", spi.fast?"on":"off");

  } else if(!strcmp(subcmd, "eye")) {
    getfloats( &spi.Eye.x[0], 3, 2, argc, argv );
    msg("spi eye %g %g %g", spi.Eye.x[0], spi.Eye.x[1], spi.Eye.x[2]);

  } else if(!strcmp(subcmd, "lens")) {
    getfloats( &spi.Lens.x[0], 3, 2, argc, argv );
    msg("spi lens %g %g %g", spi.Lens.x[0], spi.Lens.x[1], spi.Lens.x[2]);

  } else if(!strcmp(subcmd, "chan")) {
    int cno = 0;
    struct spichan *cp;
    if(argc<=2) {
	for(cno = 0; cno < spi.nchan; cno++) {
	    cp = &spi.chan[cno];
	    msg("spi chan %d  %g(fovx) %g(fovy)  %d(tess)  %d(code) %d(pfcode)",
		cno, cp->fovx, cp->fovy, cp->tess, cp->code, cp->pfcode);
	}
    } else {
	cno = atoi(argv[2]);
	if(cno < 0 || cno > COUNT(spi.chan)) {
	    msg("spi chan: must be in range 0..3");
	    return 1;
	}
	while(spi.nchan <= cno) {
	    cp = &spi.chan[cno];
	    cp->fovx = cp->fovy = 90;  cp->tess = 21;
	    cp->code = 0; cp->pfcode = 0;
	}
	cp = &spi.chan[cno];
	if(argc>3) sscanf(argv[3], "%f", &cp->fovx);
	if(argc>4) sscanf(argv[4], "%f", &cp->fovy);
	if(argc>5) sscanf(argv[5], "%d", &cp->tess);
	if(argc>6) sscanf(argv[6], "%d", &cp->code);
	if(argc>7) sscanf(argv[7], "%d", &cp->pfcode);
	msg("spi chan %d  %g(fovx) %g(fovy)  %d(tess)  %d(code) %d(pfcode)",
	    cno, cp->fovx, cp->fovy, cp->tess, cp->code, cp->pfcode);
    }

  } else if(!strcmp(subcmd, "chans")) {
    int cno = argc>2 ? atoi(argv[2]) : spi.nchan;
    if(cno > COUNT(spi.chan) || cno < 1) {
	msg("spi chans %d: must be in range 1..%d", cno, spi.nchan);
    } else {
	spi.nchan = cno;
    }
  } else if(!strcmp(subcmd, "xy")) {
    if(argc>2&&!strcmp(argv[2],"off")) {
	spi.haschanxy = 0;
    } else if(getfloats( &spi.chanxy[0], 2, 2, argc, argv ) == 2) {
	spi.haschanxy = 1;
    }
    msg(spi.haschanxy ? "spi xy %.0f %.0f" : "spi xy off (%.0f %.0f)",
	spi.chanxy[0], spi.chanxy[1]);

  } else if(!strcmp(subcmd, "wh")) {
    if(argc>2&&!strcmp(argv[2],"off")) {
	spi.haschanwh = 0;
    } else if(getfloats( &spi.chanwh[0], 2, 2, argc, argv ) == 2) {
	spi.haschanwh = 1;
    }
    msg(spi.haschanwh ? "spi wh %.0f %.0f" : "spi wh off (%.0f %.0f)",
	spi.chanwh[0], spi.chanwh[1]);

  } else {
    msg("spi {on|off|eye|lens|fovy|xy|wh}");
    return 1;
  }

  spi.refresh = 1;
  parti_redraw();
  return 1;
}

void pp_spi_init() {

  spi.ctx = NULL;
  spi.on = 0;

  spi.haschanwh = 0;
  spi.haschanxy = 0;

  /* Set defaults for VisionSTATION */
  spi.nchan = 2;

  struct spichan *cp;

  cp = &spi.chan[0];
  cp->pfcode = SPI_PF_1_CHAN;
  cp->code = SPI_1C_FRONT;
  cp->fovx = cp->fovy = 150;
  cp->tess = 31;

  cp = &spi.chan[1];
  cp->pfcode = SPI_PF_O_CHAN;
  cp->code = SPI_OC_FRONT;
  cp->fovx = cp->fovy = 65;
  cp->tess = 21;

  spi.Eye.x[0] = 0; spi.Eye.x[1] = 0.42; spi.Eye.x[2] = 0.38;
  spi.Lens.x[0] = 0; spi.Lens.x[1] = -0.19; spi.Lens.x[2] = 0;

  parti_add_commands( pp_spi_parse_args, "spi", NULL );
  if(ppui.view) {
    ppui.view->predraw = pp_spi_predraw;
    ppui.view->postdraw = pp_spi_postdraw;
  }
}

#else /* no USE_ELUMENS */

void pp_spi_init() { }
  
#endif /*USE_ELUMENS*/
