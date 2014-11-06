#ifndef _FUTIL_H
#define _FUTIL_H
/* Copyright (c) 1992 The Geometry Center; University of Minnesota
   1300 South Second Street;  Minneapolis, MN  55454, USA;
   
This file is part of geomview/OOGL. geomview/OOGL is free software;
you can redistribute it and/or modify it only under the terms given in
the file COPYING.geomview, which you should have received along with this file.
This and other related software may be obtained via anonymous ftp from
geom.umn.edu; email: software@geom.umn.edu. */
/* Copyright (C) 1992 The Geometry Center */
/* Authors: Charlie Gunn, Stuart Levy, Tamara Munzner, Mark Phillips */

/*
 * File I/O utilities for partiview, adapted from geomview by...
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONST
# ifdef __cplusplus
#  define CONST const
# else
#  define CONST
# endif
#endif


#define NODATA	(-2)		/* for async_getc(), async_fnextc() */

			/* "binary" arg to fgetnf/fgetni */
#define F_ASCII	    0
#define F_BINARY_BE 1
#define F_BINARY_LE 2


extern int fexpectstr(FILE *, CONST char *str);
extern int fexpecttoken(FILE *, CONST char *);
extern int fgetnf(FILE *, int nfloats, float *, int binary);
extern int fgetni(FILE *, int nints, int *, int binary);
extern int fgetns(FILE *, int nshorts, short *, int binary);
extern int fgettransform(FILE *, int ntrans, float *, int binary);
extern int fnextc(FILE *, int flags);
extern int async_getc(FILE *);
extern int async_fnextc(FILE *, int flags);
extern int fputnf(FILE *, int nfloats, CONST float *, int binary);
extern int fputtransform(FILE *, int ntrans, float *, int binary);
extern char *ftoken(FILE *, int flags);
extern char *fdelimtok(char *delims, FILE *, int flags);

#ifdef __cplusplus
}
#endif

#endif /*_FUTIL_H*/
