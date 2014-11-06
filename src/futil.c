/* Copyright (c) 1992 The Geometry Center; University of Minnesota
   1300 South Second Street;  Minneapolis, MN  55454, USA;
   
This file is part of geomview/OOGL. geomview/OOGL is free software;
you can redistribute it and/or modify it only under the terms given in
the file COPYING.geomview, which you should have received along with this file.
This and other related software may be obtained via anonymous ftp from
geom.umn.edu; email: software@geom.umn.edu. */
/* Copyright (C) 1992 The Geometry Center */
/* Authors: Charlie Gunn, Stuart Levy, Tamara Munzner, Mark Phillips */


/* $Header: /home/cvsroot/partiview/src/futil.c,v 1.15 2009/06/26 08:56:47 slevy Exp $ */

/*
 * File I/O functions.
 * Adapted for partiview by ...
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

/*
 * Geometry object routines
 *
 * Utility routines, useful for many objects
 *
 * int
 * fgetnf(file, nfloats, floatp, binary)
 *	Read an array of floats from a file in "ascii" or "binary" format.
 *	Returns number of floats successfully read, should = nfloats.
 *	"Binary" means "IEEE 32-bit floating-point" format.
 *
 * int
 * fgetni(FILE *file, int nints, int *intsp, int binary)
 *	Read an array of ints from a file in "ascii" or "binary" format.
 *	Returns number of ints successfully read, should = nints.
 *	"Binary" means "32-bit big-endian" integer format.
 *
 * int
 * fgetns(FILE *file, int nshorts, short *intsp, int binary)
 *	Read an array of shorts from a file in "ascii" or "binary" format.
 *	Returns number of shorts successfully read, should = nints.
 *	"Binary" means "16-bit big-endian" integer format.
 *
 * int
 * fexpectstr(FILE *file, char *string)
 *	Expect the given string to appear immediately on file.
 *	Return 0 if the complete string is found,
 *	else the offset+1 of the last matched char within string.
 *	The first unmatched char is ungetc'd.
 *
 * int
 * fexpecttoken(FILE *file, char *string)
 *	Expect the given string to appear on the file, possibly after
 *	skipping some white space and comments.
 *	Return 0 if found, else the offset+1 of last matched char in string.
 *	The first unmatched char is ungetc'd.
 *
 * int fnextc(FILE *f, int flags)
 *	Advances f to the next "interesting" character and
 *	returns it.  The returned char is ungetc'ed so the next getc()
 *	will yield the same value.
 *	Interesting depends on flags:
 *	  0 : Skip blanks, tabs, newlines, and comments (#...\n).
 *	  1 : Skip blanks, tabs, and comments, but newlines are interesting
 *		(including the \n that terminates a comment).
 *	  2 : Skip blanks, tabs, and newlines, but stop at #.
 *	  3 : Skip blanks and tabs but stop at # or \n.
 *
 * int async_fnextc(FILE *f, int flags)
 *	Like fnextc() above, but guarantees not to block if no data is
 *	immediately available.  It returns either an interesting character,
 *	EOF, or the special code NODATA (== -2).
 *
 * int async_getc(FILE *f)
 *	Like getc(), but guarantees not to block.  Returns NODATA if
 *	nothing is immediately available.
 *
 * char *ftoken(FILE *f, int flags)
 *	Skips uninteresting characters with fnextc(f, flags),
 *	then returns a "token" - string of consecutive interesting characters.
 *	Returns NULL if EOF is reached with no token, or if
 *	flags specifies stopping at end-of-line and this is encountered with
 *	no token found.
 *	The token is effectively statically allocated and will be
 *	overwritten by the next ftoken() call.
 *
 * char *fdelimtok(char *delims, FILE *f, int flags)
 *	Like ftoken(), but specially handles the characters in delims[].
 *	If one appears at the beginning of the token, it's returned as 
 *	a single-character token.
 *	If a member of delims[] appears after other characters have been
 *	accumulated, it's considered to terminate the token.
 *	So successive calls to fdelimtok("()", f, 0)
 *	on a file containing  (fred smith)
 *	would return "(", "fred", "smith", and ")".
 *	Behavior is undefined if one of the predefined delimiters
 *	(white space or #) appears in delims[].  Explicit quoting
 *	(with ", ' or \) overrides detection of delimiters.
 *
 * int fgettransform(FILE *f, int ntransforms, float *transforms, int binary)
 *	Reads 4x4 matrices from FILE.  Returns the number of matrices found,
 *	up to ntransforms.  Returns 0 if no numbers are found.
 *	On finding incomplete matrices (not a multiple of 16 floats)
 *	returns -1, regardless of whether any whole matrices were found.
 *	Matrices are expected in the form used to transform a row vector
 *	multiplied on the left, i.e.  p * T -> p'; thus Euclidean translations
 *	appear in the last row.
 *
 * int fputtransform(FILE *f, int ntransforms, float *transforms, int binary)
 *	Writes 4x4 matrices to FILE.  Returns the number written, i.e.
 *	ntransforms unless an error occurs.
 *
 * int fputnf(FILE *f, int nfloats, float *fv, int binary)
 *	Writes 'nfloats' floats to the given file.  ASCII is in %g format,
 *	separated by blanks.
 *
 * FILE *fstropen(str, len, mode)
 *	Opens a string (buffer) as a "file".
 *	Mode is "r" or "w" as usual.
 *	Reads should return EOF on encountering end-of-string,
 *	writes past end-of-string should also yield an error return.
 *	fclose() should be used to free the FILE after use.
 */


#include <stdio.h>
#include <sys/types.h>

#include "futil.h"

#include "config.h"	/* for WORDS_BIGENDIAN */


#if defined(unix) || defined(__unix)
# include <sys/time.h>
# include <arpa/inet.h>		/* for htonl() */
# ifndef NeXT
#  include <unistd.h>
# endif
# ifdef AIX
#  include <sys/select.h>
# endif
#else
# include "winjunk.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#undef isascii		/* Hacks for Irix 6.5.x */
#undef isspace
#undef isalnum


#if WORDS_BIGENDIAN
# define AM_BIG_ENDIAN  WORDS_BIGENDIAN
#else
   /* Speedy binary I/O if the machine is known to be big-endian */
# if m68k || mc68000 || mips || sparc || __hpux || AIX || ppc || __ppc__
#  define AM_BIG_ENDIAN 1
# elif defined(WIN32) || defined(i386) || defined(__i386__) || defined(__ia64__) || defined(__x86_64__)
#  define AM_BIG_ENDIAN 0
# endif
#endif

#ifndef AM_BIG_ENDIAN
# error "Can't determine endianness of this machine -- define AM_BIG_ENDIAN to 1  or 0!"
#endif

static void swap32(int n, register unsigned int *vp) {
    while(n-- > 0) {
	*vp = ((*vp>>24)&0xFF | (*vp>>8)&0xFF00 | (*vp<<8)&0xFF0000 | (*vp&0xFF)<<24);
	vp++;
    }
}
static void swap16(int n, register unsigned short *sp) {
    while(n-- > 0) {
	*sp = ((*sp>>8)&0xFF | (*sp&0xFF)<<8);
	sp++;
    }
}

int
fnextc(FILE *f, int flags)
{
	register int c;

	c = getc(f);
	for(;;) {
	    switch(c) {
	    case EOF:
		return(EOF);

	    case ' ':
	    case '\t':
		break;			/* Always skip blanks and tabs */

	    case '#':
		if(flags & 2)		/* 2: stop on comments, else skip */
		    goto fim;

		while((c = getc(f)) != '\n' && c != EOF)
		    ;
		continue;		/* Rescan this c */

	    case '\n':
		if(!(flags & 1))	/* 1: stop on \n's, else skip them */
		    break;
					/* flags&1 => fall into default */

	    default:
	     fim:
		ungetc(c, f);
		return(c);
	    }

	    c = getc(f);
	}
}


float f_pow10[11] = { 1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10 };

#define fPow10(n)  ((unsigned)(n) > 10 ? fpow10(n) : f_pow10[n])

float
fpow10(int n)
{
	register int k;
	register float v;

	if((k = n) < 0)
		k = -k;
	v = f_pow10[k & 7];
	if(k >= 8) {
		float unit = 1e8;

		k >>= 3;
		for(;;) {
			if(k & 1)
				v *= unit;
			if((k >>= 1) == 0)
				break;
			unit *= unit;
		}
	}
	return(n < 0 ? 1.0/v : v);
}

/*
 * Read an array of white-space-separated floats from file 'f' in ASCII, fast.
 * Returns the number successfully read.
 */
int
fgetnf(register FILE *f, int maxf, float *fv, int binary)
{
	int ngot;
	float v;
	register int c = EOF;
	int n;
	int s, es, nd, any;

	switch(binary) {
#if AM_BIG_ENDIAN
	case F_BINARY_BE:
#else
	case F_BINARY_LE:
#endif
		/* Easy -- same as our native format */
		return fread((char *)fv, sizeof(*fv), maxf, f);

#if AM_BIG_ENDIAN
	case F_BINARY_LE:
#else
	case F_BINARY_BE:
#endif
		/* Target format is opposite of ours -- swap */
		n = fread((char *)fv, sizeof(*fv), maxf, f);
		swap32( n, (unsigned int *)fv );
		return n;

	case F_ASCII:

	    /* Read ASCII format floats */
	    v = 0;
	    for(ngot = 0; ngot < maxf; ngot++) {
		if(fnextc(f, 0) == EOF)
			return(ngot);
		n = 0;
		s = 0;
		nd = 0;
		any = 0;
		if((c = getc(f)) == '-') {
			s = 1;
			c = getc(f);
		}
		while(c >= '0' && c <= '9') {
			n = n*10 + c - '0';
			nd++;
			if(n >= 214748364) {	/* 2^31 / 10 */
				v = any ? v*fPow10(nd) + (float)n : (float)n;
				nd = n = 0;
				any = 1;
			}
			c = getc(f);
		}
		v = any ? v*fPow10(nd) + (float)n : (float)n;
		any += nd;
		if(c == '.') {
			nd = n = 0;
			while((c = getc(f)) >= '0' && c <= '9') {
				n = n*10 + c - '0';
				nd++;
				if(n >= 214748364) {
					v += (float)n / fPow10(nd);
					n = 0;
				}
			}
			v += (float)n / fPow10(nd);
		}
		if(any == 0 && nd == 0)
			break;
		if(c == 'e' || c == 'E') {
			es = nd = 0;
			switch(c = getc(f)) {
			case '-':
				es = 1;	/* And fall through */
			case '+':
				c = getc(f);
			}
			n = 0;
			while(c >= '0' && c <= '9') {
				n = n*10 + c - '0';
				nd++;
				c = getc(f);
			}
			if(nd == 0)
				break;
			if(es) v /= fPow10(n);
			else v *= fPow10(n);
		}
		fv[ngot] = s ? -v : v;
		if(c!=EOF)
		    ungetc(c, f);
		return(ngot);
	    }
	}
	return -1;
}


int
fgetni(register FILE *f, int maxi, int *iv, int binary)
{
	int ngot;
	int c = EOF;
	int n;
	int s, any;

	switch(binary) {
#if AM_BIG_ENDIAN
	case F_BINARY_BE:
#else
	case F_BINARY_LE:
#endif
		/* Easy -- same as our native format */
		return fread((char *)iv, sizeof(int), maxi, f);

#if AM_BIG_ENDIAN
	case F_BINARY_LE:
#else
	case F_BINARY_BE:
#endif
		/* Target format is opposite of ours -- swap */
		n = fread((char *)iv, sizeof(int), maxi, f);
		swap32( n, (unsigned int *)iv );
		return n;

	case F_ASCII:

	    /* Read ASCII format ints */
	    for(ngot = 0; ngot < maxi; ngot++) {
		if(fnextc(f, 0) == EOF)
			return(ngot);
		n = 0;
		s = 0;
		any = 0;
		if((c = getc(f)) == '-') {
			s = 1;
			c = getc(f);
		}
		while(c >= '0' && c <= '9') {
			n = n*10 + c - '0';
			any = 1;
			c = getc(f);
		}
		if(!any)
			break;
		iv[ngot] = s ? -n : n;
	    }
	    if(c!=EOF) ungetc(c, f);
	    return(ngot);
	}
	return -1;
}

int
fgetns(register FILE *f, int maxs, short *sv, int binary)
{
	int ngot;
	register int c = EOF;
	int n;
	int s, any;

	switch(binary) {
#if AM_BIG_ENDIAN
	case F_BINARY_BE:
#else
	case F_BINARY_LE:
#endif
		/* Easy -- same as our native format */
		return fread((char *)sv, sizeof(*sv), maxs, f);

#if AM_BIG_ENDIAN
	case F_BINARY_LE:
#else
	case F_BINARY_BE:
#endif
		/* Target format is opposite of ours -- swap */
		n = fread((char *)sv, sizeof(*sv), maxs, f);
		swap16( n, (unsigned short *)sv );
		return n;

	case F_ASCII:
	    /* Read ASCII format shorts */
	    for(ngot = 0; ngot < maxs; ngot++) {
		if(fnextc(f, 0) == EOF)
			return(ngot);
		n = s = any = 0;
		if((c = getc(f)) == '-') {
			s = 1;
			c = getc(f);
		}
		while(c >= '0' && c <= '9') {
			n = n*10 + c - '0';
			any = 1;
			c = getc(f);
		}
		if(!any)
			break;
		sv[ngot] = s ? -n : n;
	    }
	    if(c!=EOF) ungetc(c, f);
	    return(ngot);
	}
	return -1;
}

/*
 * Check for a string on a file.
 * If found, return 0.
 * If not, return the offset of the last matched char +1
 * and ungetc the failed char so the caller can see it.
 */
int
fexpectstr(register FILE *file, char *str)
{
	register char *p = str;
	register int c;

	while(*p != '\0') {
	    if((c = getc(file)) != *p++) {
		if(c != EOF)
		    ungetc(c, file);
		return(p - str);
	    }
	}
	return 0;
}

/*
 * Check for a string on a file, skipping leading blanks.
 */
int
fexpecttoken(register FILE *file, char *str)
{
	(void) fnextc(file, 0);
	return fexpectstr(file, str);
}

int fescape(FILE *f)
{
    int n, k, c = fgetc(f);

    switch(c) {
	case 'n': return '\n';
	case 'b': return '\b';
	case 't': return '\t';
	case 'r': return '\r';
    }
    if(c < '0' || c > '7')
	return c;
    
    n = c-'0';  k = 2;
    while((c = fgetc(f)) >= '0' && c <= '7') {
	n = n*8 | c-'0';
	if(--k <= 0)
	    return n;
    }
    if(c != EOF) ungetc(c, f);
    return n;
}

/*
 * Get a token, return a string or NULL.
 * Tokens may be "quoted" or 'quoted'; backslashes accepted.
 * The string is statically allocated and should be copied if
 * needed before the next call to ftoken().
 */
char *
ftoken(FILE *file, int flags)
{
	static char *token = NULL;
	static int troom = 0;
	register int c;
	register char *p;
	register int term;

	if((term = fnextc(file, flags)) == EOF)
	    return NULL;

	if(token == NULL) {
	    troom = 50;
	    token = (char *)malloc(troom * sizeof(char));
	    if(token == NULL)
		return NULL;
	}

	p = token;
	switch(term) {
	case '"':
	case '\'':
	    (void) fgetc(file);
	    for(;;) { 
		if((c = getc(file)) == EOF || c == term)
		    break;
		else if(c == '\\')
		    c = fescape(file);
		*p++ = c;
		if(p == &token[troom]) {
		    token = (char *)realloc(token, troom * 2);
		    if(token == NULL)
			return NULL;
		    p = &token[troom];
		    troom *= 2;
		}
	    }
	    break;

	default:
	    if(isspace(term))
		return NULL;
	    while((c = getc(file)) != EOF && !isspace(c)) {
		if(c == '\\')
		    c = fescape(file);
		*p++ = c;
		if(p == &token[troom]) {
		    token = (char *)realloc(token, troom * 2);
		    if(token == NULL)
			return NULL;
		    p = &token[troom];
		    troom *= 2;
		}
	    }
	    break;
	}
	*p = '\0';
	return token;
}


/*
 * Get a token, return a string or NULL.
 * Tokens may be "quoted" or 'quoted'; backslashes accepted.
 * The string is statically allocated and should be copied if
 * needed before the next call to ftoken().
 */
char *
fdelimtok(char *delims, FILE *file, int flags)
{
	static char *token = NULL;
	static int troom = 0;
	register int c;
	register char *p;
	register char *q;
	register int term;

	if((term = fnextc(file, flags)) == EOF)
	    return NULL;

	if(token == NULL) {
	    troom = 50;
	    token = (char *)malloc(troom * sizeof(char));
	    if(token == NULL)
		return NULL;
	}

	p = token;
	switch(term) {
	case '"':
	case '\'':
	    (void) fgetc(file);
	    for(;;) { 
		if((c = getc(file)) == EOF || c == term)
		    break;
		else if(c == '\\')
		    c = fescape(file);
		*p++ = c;
		if(p == &token[troom]) {
		    token = (char *)realloc(token, troom * 2);
		    if(token == NULL)
			return NULL;
		    p = &token[troom];
		    troom *= 2;
		}
	    }
	    break;

	default:
	    if(isspace(term))
		return NULL;
	    while((c = getc(file)) != EOF && !isspace(c)) {
		if(c == '\\')
		    c = fescape(file);
		*p = c;
		if(++p == &token[troom]) {
		    token = (char *)realloc(token, troom * 2);
		    if(token == NULL)
			return NULL;
		    p = &token[troom];
		    troom *= 2;
		}
		for(q = delims; *q && c != *q; q++)
		    ;
		if(*q) {
		    if(p > token+1) {
			p--;
			ungetc(c, file);
		    }
		    break;
		}
	    }
	    break;
	}
	*p = '\0';
	return token;
}


/*
 * Load one or more Transforms from a file.
 * Return 1 on success, 0 on failure.
 */
int
fgettransform(FILE *file, int ntrans, float *trans /* float trans[ntrans][4][4] */, int binary)
{
	register float *T;
	int nt;

	for(nt = 0; nt < ntrans; nt++) {
	    T = trans + 16*nt;
	    switch(fgetnf(file, 16, T, binary)) {
	    case 16:
		break;

	    case 0:
		return nt;

	    default:
		return -1;
	    }
	}
	return ntrans;
}

int
fputnf(FILE *file, int count, float *v, int binary)
{
	register int i;
	long w;
	if(binary) {
#if AM_BIG_ENDIAN
	  return fwrite(v, sizeof(float), count, file);
#else
	  for(i = 0; i < count; i++) {
	    w = htonl(*(long *)&v[i]);
	    fwrite(&w, sizeof(float), 1, file);
	  }
	  return count;
#endif
	}

	fprintf(file, "%g", v[0]);
	for(i = 1; i < count; i++)
	    fprintf(file, " %g", v[i]);
	return count;
}

int
fputtransform(FILE *file, int ntrans, float *trans, int binary)
{
	register int i, n;
	register float *p;

	if(binary) {
#if AM_BIG_ENDIAN
	    return fwrite(trans, 4*4*sizeof(float), ntrans, file);
#else
	fprintf(stderr, "fputtransform: need code to handle binary writes for this architecture.");
	return 0;
#endif
	}

	/* ASCII. */

	for(n = 0; n < ntrans; n++) {
	    p = trans + n*16;
	    for(i = 0; i < 4; i++, p += 4) {
		fprintf(file, "  %12.8g  %12.8g  %12.8g  %12.8g\n",
		    p[0], p[1], p[2], p[3]);
	    }
	    if(ferror(file))
		return n;
	    fprintf(file, "\n");
	}
	return ntrans;
}

/***************************************************************************/

/*
 * Non-blocking buffered I/O
 */

	/* Peer into a stdio buffer, check whether it has data available
	 * for reading.  Almost portable given the common stdio ancestry.
	 */
#if __GLIBC__
#  define F_HASDATA(f)  ((f)->_IO_read_ptr < (f)->_IO_read_end)
#elif __APPLE__  /* really? or is this MacOSX-specific? */
#  define F_HASDATA(f)  ((f)->_r > 0)
#else  /* SGI and many others */
#  define F_HASDATA(f)  ((f)->_cnt > 0)
#endif


int
fhasdata(FILE *f)
{
    return F_HASDATA(f);
}

int
async_getc(register FILE *f)
{

#if defined(unix) || defined(__unix)

    fd_set fds;
    static struct timeval notime = { 0, 0 };

    if(F_HASDATA(f))
	return getc(f);
    FD_ZERO(&fds);
    FD_SET(fileno(f), &fds);
    if(select(fileno(f)+1, &fds, NULL, NULL, &notime) == 1)
	return fgetc(f);
    return NODATA;
#else
    return getc(f);
#endif
}

int
async_fnextc(register FILE *f, register int flags)
{
    register int c;

    c = async_getc(f);
    for(;;) {
	switch(c) {
	case EOF:
	case NODATA:
	    return(c);

	case ' ':
	case '\t':
	    break;			/* Always skip blanks and tabs */

	case '#':
	    if(flags & 2)		/* 2: stop on comments, else skip */
		goto fim;

	    while((c = getc(f)) != '\n' && c != EOF)
		;
	    continue;			/* Rescan this c */

	case '\n':
	    if(!(flags & 1))		/* 1: stop on \n's, else skip them */
		break;
				    	/* flags&1 => fall into default */

	default:
	 fim:
	    ungetc(c, f);
	    return(c);
	}

	c = async_getc(f);
    }
}
