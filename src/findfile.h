#ifndef _FINDFILE_H
#define _FINDFILE_H
/*
 * Some file and command utilities for partiview, adapted from geomview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern CONST char **getfiledirs(void);
extern char *findfile( CONST char *superfile, CONST char *fname );
extern void filedirs( CONST char **dirs );

extern char *envexpand(char *str);

extern int tokenize( char *str, char *tbuf, int maxargs, char **argv, char **commentp);
extern char *rejoinargs( int arg0, int argc, char **argv );


#ifdef __cplusplus
}
#endif

#endif
