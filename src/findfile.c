/* Copyright (c) 1992 The Geometry Center; University of Minnesota
   1300 South Second Street;  Minneapolis, MN  55454, USA;
   
This file is part of geomview/OOGL. geomview/OOGL is free software;
you can redistribute it and/or modify it only under the terms given in
the file COPYING.geomview, which you should have received along with this file.
This and other related software may be obtained via anonymous ftp from
geom.umn.edu; email: software@geom.umn.edu. */
/* Copyright (C) 1992 The Geometry Center */

/* Authors: Charlie Gunn, Stuart Levy, Tamara Munzner, Mark Phillips */

/* $Header: /home/cvsroot/partiview/src/findfile.c,v 1.11 2013/11/30 20:33:55 slevy Exp $ */

/*
 * Utility functions: file search path; command tokenizing.
 * Adapted for partiview by...
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <ctype.h>
#undef isspace		/* Hacks for Irix 6.5.x */
#undef isascii
#undef isalnum

#include "shmem.h"
#include "findfile.h"

#if defined(unix) || defined(__unix)
# include <unistd.h>		/* needed for access() */
#else	/* Win32 */
# include "winjunk.h"
#endif

#include <stdlib.h>


static CONST char **dirlist = NULL;
static void dirprefix(CONST char *file, char *dir);
char *envexpand(char *s);

/*-----------------------------------------------------------------------
 * Function:	filedirs
 * Description:	set the list of directories to search for files
 * Args:	dirs: NULL-terminated array of pointers to directory
 *		  strings
 * Author:	mbp
 * Date:	Wed Feb 12 14:09:48 1992
 * Notes:	This function sets the list of directories searched by
 *		findfile().   It makes an internal copy of these directories
 *		and expands all environment variables in them.
 */
void
filedirs(CONST char *dirs[])
{
  char buf[1024];
  int i,ndirs;
  CONST char **odirlist = dirlist;

  for (ndirs=0; dirs[ndirs]!=NULL; ++ndirs);
  dirlist = OOGLNewNE(CONST char *,ndirs+1, "filedirs: dirlist");
  for (i=0; i<ndirs; ++i) {
    strcpy(buf, dirs[i]);
    envexpand(buf);
    dirlist[i] = strdup(buf);
  }
  dirlist[ndirs] = NULL;
  if (odirlist) {
    CONST char **p;
    for (p=odirlist; *p!=NULL; ++p) free(*p);
    OOGLFree(odirlist);
  }
}


/*-----------------------------------------------------------------------
 * Function:	getfiledirs
 * Description:	return the list of dirs set by the last call to filedirs()
 * Author:	mbp
 * Date:	Wed Feb 12 14:09:48 1992
 */
CONST char **
getfiledirs()
{
  return dirlist;
}


/*-----------------------------------------------------------------------
 * Function:	findfile
 * Description:	resolve a filename into a pathname
 * Args:	*superfile: containing file
 *		*file: file to look for
 * Returns:	pointer to resolved pathname, or NULL if not found
 * Author:	mbp
 * Date:	Wed Feb 12 14:11:47 1992
 * Notes:
 *
 * findfile() tries to locate a (readable) file in the following way.
 *
 *    If file begins with a '/' it is assumed to be an absolute path.  In
 *    this case we expand any environment variables in file and test for
 *    existence, returning a pointer to the expanded path if the file is
 *    readable, NULL otherwise.
 *
 *    Now assume file does not begin with a '/'.
 *
 *    If superfile is non-NULL, we assume it is the pathname of a file
 *    (not a directory), and we look for file in the directory of that
 *    path.  Environment variables are expanded in file but not in
 *    superfile.
 *
 *    If superfile is NULL, or if file isn't found superfile directory,
 *    we look in each of the directories in the array last passed to
 *    filedirs().  Environment variables are expanded in file and in
 *    each of the directories last passed to filedirs().
 *
 *    We return a pointer to a string containing the entire pathname of
 *    the first location where file is found, or NULL if it is not found.
 *
 *    In all cases the returned pointer points to dynamically allocated
 *    space which will be freed on the next call to findfile().
 *
 *    File existence is tested with a call to access(), checking for read
 *    permission.
 */
char *
findfile(CONST char *superfile, CONST char *file)
{
  static char *path = NULL;
  CONST char **dirp;
  char pbuf[1024];
  int trydot = 1;

  if (path) {
    free(path);
    path = NULL;
  }
  if (file == NULL) return NULL;
  if (file[0] == '/' || file[0] == '$' || file[0] == '~'
		|| (file[0] == '.' && file[1] == '/') ) {
    strcpy(pbuf, file);
    envexpand(pbuf);
    if (access(pbuf,R_OK)==0)
      return (path = strdup(pbuf));
    else
      return NULL;
  }
  if (superfile) {
    dirprefix(superfile, pbuf);
    if(pbuf[0] == '\0' || !strcmp(pbuf, "."))
	trydot = 0;
    strcat(pbuf, file);
    envexpand(pbuf);
    if (access(pbuf,R_OK)==0)
      return (path = strdup(pbuf));
  }
  if(dirlist != NULL) {
      for (dirp = dirlist; *dirp != NULL; dirp++) {
	if(!strcmp(*dirp, ".")) trydot = 0;
	sprintf(pbuf,"%s/%s", *dirp, file);
	envexpand(pbuf);
	if (access(pbuf,R_OK)==0)
	  return (path = strdup(pbuf));
      }
  }
  /* Implicitly add "." at end of search path */
  if(trydot && access(file,R_OK) == 0)
    return (path = strdup(file));

  return (path = NULL);
}
    
/*-----------------------------------------------------------------------
 * Function:	dirprefix
 * Description:	get the directory prefix from a pathname
 * Args:	*path: the pathname
 *		*dir: pointer to location where answer is to be stored
 * Author:	mbp
 * Date:	Wed Feb 12 14:17:36 1992
 * Notes:	Answer always ends with a '/' if path contains a '/',
 *		otherwise dir is set to "".
 */
static void
dirprefix(CONST char *path, char *dir)
{
  register char *end;

  strcpy(dir, path);
  end = dir + strlen(dir) - 1;
  while (end >= dir && *end != '/') --end;
  if (end >= dir) *(end+1) = '\0';
  else dir[0] = '\0';
}

/*-----------------------------------------------------------------------
 * Function:	envexpand
 * Description:	expand environment variables in a string
 * Args:	*s: the string
 * Returns:	s
 * Author:	mbp
 * Date:	Fri Feb 14 09:46:22 1992
 * Notes:	expansion is done inplace; there better be enough room!
 */
char *
envexpand(char *s)
{
  char *c, *env, *envend, *tail;

  c = s;
  if (*c == '~' && (env = getenv("HOME"))) {
    tail = strdup(c+1);
    strcpy(c, env);
    strcat(c, tail);
    c += strlen(env);
    free(tail);
  }
  while (*c != '\0') {
    if (*c == '$') {
      for(envend = c; isalnum(*++envend) || *envend == '_'; ) ;
      tail = strdup(envend);
      *envend = '\0';
      if((env = getenv(c+1)) == NULL) {
	fprintf(stderr, "%s : No %s environment variable",s,c+1);
	strcpy(c,tail);
      } else {
	strcpy(c,env);
	strcat(c,tail);
	c += strlen(env);
      }
      free(tail);
    }
    else ++c;
  }
  return s;
}   


int tokenize(char *str, char *tbuf, int maxargs, char **argv, char **commentp)
{
  register char *ip = str;
  register char *op = tbuf;
  int argc = 0;

  if(commentp) *commentp = NULL;

  while(argc < maxargs-1) {
    while(isspace(*ip)) ip++;
    if(*ip == '\0') goto eol;

    if(argc == 0 && *ip != '#' && (strchr(ip, '$') || strchr(ip, '~'))) {
	/*
	 * Hack: only envexpand() if the whole input isn't a comment.
	 * But we can still be fooled if there's a $ in an embedded comment.
	 */
	ip = envexpand(ip);
    }
    argv[argc++] = op;
    switch(*ip) {

    case '#':
	if(commentp != NULL) {
	    *commentp = op;
	    while(*ip && *ip != '\n' && *ip != '\r')
		*op++ = *ip++;
	    *op = '\0';
	}
	argc--;
	goto eol;

    case '"':
	ip++;
	while(*ip != '"') {
	    if(*ip == '\\') ip++;
	    if(*ip == '\0') goto eol;
	    *op++ = *ip++;
	}
	ip++;
	break;

    case '\'':
	ip++;
	while(*ip != '\'') {
	    if(*ip == '\\') ip++;
	    if(*ip == '\0') goto eol;
	    *op++ = *ip++;
	}
	ip++;
	break;

    default:
	while(!isspace(*ip)) {
	    if(*ip == '\\') ip++;
	    if(*ip == '\0') goto eol;
	    *op++ = *ip++;
	}
	break;
    }
    *op++ = '\0';
  }
  /* oh no, too many entries for argv[] array --
   * leave all remaining chars in last argument.
   */
  strcpy(op, ip);
  argv[argc++] = op;
 eol:
  *op = '\0';
  argv[argc] = NULL;
  return argc;
}

char *rejoinargs( int arg0, int argc, char **argv )
{
  int k;
  int room = 2;
  char *ep;
  static char *space = NULL;
  static int sroom = 0;
  for(k = arg0; k < argc; k++)
    room += strlen(argv[k]) + 1;
  if(room > sroom) {
    if(space) free(space);
    sroom = room + 50;
    space = (char *)malloc(sroom);
  }
  for(k = arg0, ep = space; k < argc; k++) {
    ep += sprintf(ep, "%s ", argv[k]);
  }
  if(ep > space) ep--;
  *ep = '\0';
  return space;
}
