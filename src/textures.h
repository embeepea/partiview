#ifndef _TEXTURES_H
#define _TEXTURES_H
/*
 * OpenGL texture (etc.) handling.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Texture Texture;
typedef struct { float r,g,b,a; } ColorA;

Texture * txmake( char *fname, int apply, int txflags, int qualflags );
int       txload( Texture *tx );
int	  txbind( Texture *tx, int *enabled );

int	  txaddentry( Texture ***tp, int *ntextures, char *fromfile,
		int txno, char *txfname, int apply, int txflags, int qualflags );

void	  txpurge( Texture *tx );

int	  get_dsp_context(void);
void	  set_dsp_context(int ctxno);

#define MAXDSPCTX 18

struct Texture {
    char *filename;		/* ppm or pgm (.Z) file */
    char *alphafilename;	/* If present, this is a .pgm (.Z) file */
    char *data;			/* Raw data, top to bottom, read from file */
    int xsize, ysize, zsize;
    int channels;
    unsigned int flags;		/* clamp, etc. */
    int apply;			/* Application style (TXF_DECAL, TXF_MODULATE, TXF_BLEND) */
    int loaded;			/* 0: not yet; 1: yes; -1: error */
    int coords;			/* Texture-coord auto generation (not used) */
    int qualflags;		/* APF_TX{MIPMAP,MIPINTERP,LINEAR}: if loaded, how? */
    int report;
    ColorA background;		/* background color: outside of clamped texture */
    Matrix tfm;			/* texture-coord transformation */
    int txid[MAXDSPCTX];	/* OpenGL texture-object id's */
    struct Texture *next;	/* Link in list of all loaded textures */
};

extern Texture *AllLoadedTextures;	/* List of em */


#define	TX_DOCLAMP	450
#define	  TXF_SCLAMP	  0x1	/* Clamp if s outside 0..1 (else wrap) */
#define	  TXF_TCLAMP	  0x2	/* Clamp if t outside 0..1 (else wrap) */

#define	  TXF_LOADED	  0x4	/* Has this texture been loaded?
				 * (tried to read those files yet?)
				 */
#define	  TXF_RGBA	  0x8	/* In loaded data, is R first byte? (else ABGR) */
#define	  TXF_USED	  0x10	/* "Recently rendered a geom containing this texture" */
#define	  TXF_ALPHA	  0x20	/* "use alpha channel" (if 1-chan, it's 000A) */
#define	  TXF_INTENSITY	  0x40	/* synthesize alpha = intensity */
#define	  TXF_ADD	  0x80	/* synthesize alpha = 0 (so "over" == "add") */
#define	  TXF_OVER	  0x100	/* always use "over" compositing */

#define	  TXF_3D	  0x200	/* 3-D texture */

#define	TX_APPLY	451	/* Interpret texture values to... */
#define	  TXF_MODULATE	  0
#define	  TXF_BLEND	  1
#define	  TXF_DECAL	  2

/* texture quality flags */
#define	  TXQ_MIPMAP	 0x4
#define   TXQ_LINEAR	 0x2
#define	  TXQ_NEAREST	 0x1

#define	TX_FILE		452
#define	TX_ALPHAFILE	453
#define	TX_DATA		454
#define	TX_XSIZE	455
#define	TX_YSIZE	456
#define	TX_CHANNELS	457
#define	TX_COORDS	458 /* Texture coordinates come from... */
#define	  TXF_COORD_GIVEN	0   /* given as part of object (default) */
				    /* In fact, only TXF_COORD_GIVEN works now. */
#define	  TXF_COORD_LOCAL	1   /* In coord system of texture map */
#define	  TXF_COORD_CAMERA	2   /* In camera coords */
#define	  TXF_COORD_NORMAL	3   /* Taken from surface-normal, for env map */
#define	TX_BACKGROUND	459
#define	TX_HANDLE_TRANSFORM	460

#define	TX_ABLOCK	464
#define	TX_END		465


#ifdef __cplusplus
}
#endif

#endif /* _TEXTURES_H */

