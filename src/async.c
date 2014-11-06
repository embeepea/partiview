#include <stdio.h>
#include <stdlib.h>
/*
 * Async communication with external modules (subprocesses) for partiview.
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

#include "specks.h"
#include "partiviewc.h"
#include "findfile.h"	/* for tokenize() */
#include "futil.h"

#include <ctype.h>
#undef isspace		/* for irix 6.5 backward compat, sigh */

#if unix
# include <unistd.h>
#endif

#define MAX_ASYNC 8

FILE *asyncmd[MAX_ASYNC];

int specks_add_async( struct stuff *st, char *cmdstr, int replytoo )
{
  int i;

#if unix  /* not WIN32 */
  if(cmdstr == NULL)
    return -1;
  for(i = 0; i < MAX_ASYNC; i++) {
    if(asyncmd[i] == NULL) {
	asyncmd[i] = popen(cmdstr, "r");
#if !CAVEMENU
	parti_asyncfd( fileno( asyncmd[i] ) );
#endif
	return i;
    }
  }
#endif /*unix*/
  msg("Sorry, all %d async-command slots full: can't run %s", MAX_ASYNC, cmdstr);
  return -1;
}

int specks_check_async( struct stuff **stp )
{
  int i, any = 0;
  char *av[128];
  int ac;
  char buf[5120], tbuf[5120];
  static int reentered = 0;	/* Don't allow recursion! */

#if unix  /* not WIN32 */

  if(stp == NULL || *stp == NULL) return 0;
  if(reentered>0) return 0;
  reentered = 1;

  for(i = 0; i < MAX_ASYNC; i++) {
    int brackets = 0;
    int none = 1;
    int nlines = 0;

    while(asyncmd[i] && (none || brackets > 0)) {
	int c = (brackets>0) ? fnextc(asyncmd[i], 1)
			     : async_fnextc(asyncmd[i], 1);
	switch(c) {
	case EOF:
	    if(getenv("DBG")) msg("Closing async %d", i);
#if !CAVEMENU
	    parti_unasyncfd( fileno( asyncmd[i] ) );
#endif
	    pclose(asyncmd[i]);
	    asyncmd[i] = NULL;
	    break;
	case -2: /* NODATA */
	    none = 0;
	    break;
	default:
	    /* Got something.  Assume that we can read a whole line quickly. */
	    if(fgets(buf, sizeof(buf), asyncmd[i]) == NULL) {
		if(getenv("DBG")) msg("fgets: Closing async %d", i);
		pclose(asyncmd[i]);
		asyncmd[i] = NULL;
		break;
	    }
	    nlines++;
	    any++;

	    for(c = 0; isspace(buf[c]) || buf[c] == '{' || buf[c] == '}'; c++) {
		if(buf[c] == '{') brackets++;
		else if(buf[c] == '}' && brackets > 0) brackets--;
		buf[c] = ' ';
	    }
	    if(getenv("DBG")) msg("async %d[%d]{%d}: %s", i, nlines, brackets, buf);
	    ac = tokenize( buf, tbuf, COUNT(av), av, 0 );
	    if(ac > 0) {
		none = 0;
#if CAVEMENU
		if(!specks_parse_args( stp, ac, av ))
		    VIDI_queue_commandstr( buf );

#else /* non-virdir version */
		parti_redraw();
		specks_parse_args( stp, ac, av );
#endif
	    }

	}
    }
  }
  reentered = 0;
#endif /*unix not WIN32*/
  return any;
}

