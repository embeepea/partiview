/*
 * OpenGL texture (etc.) handling.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */
#ifdef WIN32
# include "winjunk.h"
#else /*unix?*/
#include <unistd.h>
#endif /*!WIN32*/
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>	/* for GLuint */
#include <GL/glext.h>	/* for glTexImage3D() */
#include <GL/glu.h>
#endif

#include <string.h>
#include "textures.h"
#include "shmem.h"
#include "findfile.h"

#if sgi && mips
# define glBindTexture  glBindTextureEXT
# define glGenTextures  glGenTexturesEXT
#endif

#if CAVE
#include "vd_util.h"
#endif

extern int mg_inhaletexture( Texture *tx, int rgba );

static int dspcontext = 0;

void set_dsp_context( int ctxno )
{
    if(ctxno < 0 || ctxno >= MAXDSPCTX) {
	fprintf(stderr, "textures: set_dsp_context(%d): context out of range!\n", ctxno);
	ctxno = MAXDSPCTX - 1;
    }
    dspcontext = ctxno;
}

int get_dsp_context(void)
{
#if CAVE
    /* assume virdir */
    int id = CAT_get_display_slot();
    if(id < 0 || id >= MAXDSPCTX) id = MAXDSPCTX-1;
    return id;
#else /* stand-alone -- single GLX context */
    return dspcontext;
#endif
}

#define TEXTURENO(enab)  ((enab) & 0xFFFFFF)
#define	BLEND_ON	 0x40000000
#define	BLEND_OFF	 0x20000000

int txbind( Texture *tx, int *enabled )
{
  static GLint format[5] =
	{ 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
  static GLfloat minfilts[] = {
	GL_NEAREST, GL_LINEAR,
	GL_NEAREST, GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, 
	GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR
  };
  static int ctx = -1;
  static int txenv[MAXDSPCTX];
  int wantblend = 0;
  int ena = -1;
  int txtarget;
  static int any3d = 0;
  int mustload;
  int wanted, wantenv;

  if(tx == NULL || txload(tx) == 0
#ifndef GL_TEXTURE_3D
	|| tx->flags & TXF_3D
#endif
		) {
    /* reset state, invalidate cache */
    if(enabled == NULL || *enabled != 0) {
	glBindTexture( GL_TEXTURE_2D, 0 );
	glDisable( GL_TEXTURE_2D );

#ifdef GL_TEXTURE_3D
	if(any3d) {
	    glDisable( GL_TEXTURE_3D );
	    glBindTexture( GL_TEXTURE_3D, 0 );
	}
#endif

	glDisable( GL_ALPHA_TEST );
    }
    txenv[ctx] = -1;
    if(enabled) *enabled = 0;
    return 0;
  }

#ifdef GL_TEXTURE_3D
  txtarget = (tx->flags & TXF_3D) ? GL_TEXTURE_3D : GL_TEXTURE_2D;

#else
  txtarget = GL_TEXTURE_2D;
#endif

  if(tx->flags & TXF_3D)
      any3d = 1;

  if(ctx < 0) {
    int i;
    for(i = 0; i < COUNT(txenv); i++)	/* first time -- initialize txenv[] state cache */
	txenv[i] = -1;
  }
  ctx = get_dsp_context();
  mustload = (tx->txid[ctx] == 0);

#if unix
  if(tx->report & (1<<ctx)) {
    fprintf(stderr, "tx %p pid %d ctx %d txid[ctx] %d data %p loaded %d isTexture %d\n",
	tx, getpid(), ctx, tx->txid[ctx], tx->data, tx->loaded,
	glIsTexture(tx->txid[ctx]));
    tx->report &= ~(1<<ctx);
  }
#endif
  wantblend = (tx->flags & (TXF_ALPHA|TXF_INTENSITY|TXF_ADD)) ? BLEND_ON : BLEND_OFF;

  wantenv =
	  tx->apply == TXF_DECAL ? GL_DECAL
	: tx->apply == TXF_BLEND ? GL_BLEND
				 : GL_MODULATE;

  if(txenv[ctx] != wantenv) {
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, wantenv);
    txenv[ctx] = wantenv;
  }

  if(!mustload && TEXTURENO(*enabled) == tx->txid[ctx])
    return *enabled;		/* short-circuit! */

  if(mustload) {
    GLuint id;
    glGenTextures( 1, &id );
    tx->txid[ctx] = id;
  }


  wanted = tx->txid[ctx] | wantblend;

  glBindTexture( txtarget, TEXTURENO(wanted) );

  if(mustload) {
    if(tx->data == NULL) {
	fprintf(stderr,
	    "Hey! Why are we having to reload the data for texture \"%s\"?\n",
	    tx->filename);
	tx->loaded = 0;
	txload(tx);
	wanted = (tx->data != NULL);
    }


    if(wanted) {
	static float black[4] = {0,0,0,0};
	unsigned char *txdata = (unsigned char *)tx->data;
	int channels = tx->channels;

	if((tx->flags&TXF_INTENSITY) && (channels == 1 || channels == 3)) {
	    int npix = tx->xsize * tx->ysize;
	    int k;
	    unsigned char *ip, *op;
	    channels++;
	    txdata = (unsigned char *)malloc( npix * channels );
	    k = npix;
	    ip = (unsigned char *)tx->data;
	    op = txdata;
	    switch(channels) {
	    case 2:
		do {
		    op[1] = *ip++;
		    op[0] = 255;
		    op += 2;
		} while(--k > 0);
		break;
	    case 4:
		do {
		    op[3] = 77*ip[0] + 150*ip[1] + 28*ip[2];
		    op[0] = ip[0]>=op[3] ? 255 : ip[0]/op[3];
		    op[1] = ip[1]>=op[3] ? 255 : ip[0]/op[3];
		    op[2] = ip[2]>=op[3] ? 255 : ip[2]/op[3];
		    ip += 3;
		    op += 4;
		} while(--k > 0);
		break;
	    }
	}

	if(tx->xsize % 4 != 0)
	    glPixelStorei( GL_PACK_ALIGNMENT, 1 );

	if(tx->flags & TXF_3D) {
#ifdef USE_GL_TEXTURE_3D

	    tx->qualflags &= ~TXQ_MIPMAP;
	    glTexImage3D( GL_TEXTURE_3D, 0, channels,
		    tx->xsize, tx->ysize, tx->zsize, 
		    0, /* border */
		    channels==1 && (tx->flags&TXF_ALPHA)
			? GL_ALPHA : format[channels],
		    GL_UNSIGNED_BYTE,
		    txdata );
#endif

	} else {

	    gluBuild2DMipmaps( GL_TEXTURE_2D, channels,
		    tx->xsize, tx->ysize,
		    channels==1 && (tx->flags&TXF_ALPHA)
			? GL_ALPHA : format[channels],
		    GL_UNSIGNED_BYTE,
		    txdata);

	}

	if(tx->xsize % 4 != 0)
	    glPixelStorei( GL_PACK_ALIGNMENT, 4 ); /* restore default */

	if(txdata != (unsigned char *)tx->data)
	    free(txdata);

	glTexParameterfv(txtarget, GL_TEXTURE_BORDER_COLOR,
	    black);
	glTexParameterf(txtarget, GL_TEXTURE_WRAP_S,
	    (tx->flags & TXF_SCLAMP) ? GL_CLAMP : GL_REPEAT);
	glTexParameterf(txtarget, GL_TEXTURE_WRAP_T,
	    (tx->flags & TXF_TCLAMP) ? GL_CLAMP : GL_REPEAT);
	glTexParameterf(txtarget, GL_TEXTURE_MIN_FILTER,
	    minfilts[tx->qualflags & 0x07]);
	glTexParameterf(txtarget, GL_TEXTURE_MAG_FILTER,
	    (tx->qualflags&0x07) ? GL_LINEAR : GL_NEAREST);


#if 0
	/* yes, we might very well need texture data again, in case window gets re-generated */
#if !CAVE
	/* We won't need texture data again -- opengl has it now */
	Free(tx->data);
	tx->data = NULL;
#endif
#endif

    }
  }

  if((wanted ^ *enabled) & (BLEND_ON|BLEND_OFF)) {
    if(wanted & BLEND_ON) {
	glEnable( GL_BLEND );
	if(tx->flags & TXF_ADD) {
	    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	    glDisable( GL_ALPHA_TEST );
	} else {
	    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	    glEnable( GL_ALPHA_TEST );
	    glAlphaFunc( GL_GREATER, 0.03 );
	}
    } else if(wanted & BLEND_OFF) {
	glDisable( GL_BLEND );
	glDisable(GL_ALPHA_TEST);
    }

    if(TEXTURENO(wanted)) {
	glEnable(txtarget);
    } else {
	glDisable(GL_TEXTURE_2D);
#ifdef GL_TEXTURE_3D
	if(any3d) glDisable(GL_TEXTURE_3D);
#endif
    }
    *enabled = wanted = tx->txid[ctx] | wantblend;
  }
  return wanted;
}

Texture *txmake( char *fname, int apply, int txflags, int qualflags )
{
  Texture *tx = NewN( Texture, 1 );
  memset(tx, 0, sizeof(*tx));
  tx->tfm = Tidentity;
  tx->filename = fname;
  tx->apply = apply;
  tx->flags = txflags;
  tx->qualflags = qualflags;
  return tx; 
}

int txload( Texture *tx )
{
  if(tx == NULL || tx->loaded < 0) return 0;
  if(tx->loaded == 1) return 1; /* already loaded */
  if(tx->loaded == 2) return 0;	/* busy loading now! */
  tx->loaded = 2;	/* set this to make multiprocess collisions unlikely */
  if(mg_inhaletexture( tx, TXF_RGBA ) <= 0) {
    tx->loaded = -1;
    return 0;
  }
  tx->loaded = 1;
  return 1;
}

int txaddentry( Texture ***tp, int *ntextures, char *fromfile,
		int txno, char *txfname, int apply, int txflags, int qualflags )
{
  char *realfname;
  if(txno < 0) {
    fprintf(stderr, "Texture indices must be >= 0 (texture %d %s)\n", txno, txfname);
    return 0;
  }

  realfname = findfile(fromfile, txfname);
  if(realfname == NULL) {
    fprintf(stderr, "Can't find texture file \"%s\", ignoring it.\n", txfname);
    return 0;
  }
  
  if(txno >= *ntextures) {
    int ontex = *ntextures;
    if(*ntextures == 0) {
	*ntextures = txno + 62;
	*tp = NewN( Texture *, *ntextures );
    } else {
	*ntextures = (txno > ontex*2+1 ? txno+ontex : ontex*2+1);
	*tp = RenewN( *tp, Texture *, *ntextures );
    }
    memset(&(*tp)[ontex], 0, (*ntextures - ontex) * sizeof(Texture *));
  }
  (*tp)[txno] = txmake( shmstrdup(realfname), apply, txflags, qualflags );
  return txno;
}
