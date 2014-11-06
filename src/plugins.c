/*
 * Statically-compiled "plugin" initialization for partiview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef WIN32
# include "winjunk.h"
#endif

#include "plugins.h"

#include <stdlib.h>
#include "specks.h"
#include "partiviewc.h"
#include "shmem.h"
#include "config.h"

void plugin_init() {
#ifdef USE_KIRA
  kira_parti_init();
#endif
#ifdef USE_ELUMENS
  pp_spi_init();
#endif
#ifdef USE_NETHACK
  nethack_init();
#endif
#ifdef USE_SIXDOF	/* spaceball, etc. */
  sixdof_init();
#endif
#ifdef USE_MODEL
  parti_model_init();
#endif
#ifdef USE_WARP
  warp_init();
#endif
#ifdef USE_IEEEIO
  parti_ieee_init();
#endif
#ifdef USE_CONSTE
  conste_init();
#endif
}

static struct parser *parsers = NULL;   /* PJT: double declared ?? */
static struct parser *reader_parsers = NULL;
static void (*cmdtracefunc)( struct stuff **, int argc, char *argv[] );

void parti_cmdtrace( void (*func)( struct stuff **, int argc, char *argv[] ) ) {
  cmdtracefunc = func;
}

int parti_parse_args( struct stuff **stp, int argc, char *argv[], char *fname ) {
  struct parser *pp;
  int val;

  if(cmdtracefunc)
      (*cmdtracefunc)( stp, argc, argv );
  for(pp = parsers; pp; pp = pp->next)
    if(pp->parsefunc && (val = (*pp->parsefunc)( stp, argc, argv, fname, pp->etc )) != 0)
	return val;
  return 0;
}

void parti_add_commands( int (*parsefunc)(struct stuff **, int, char *[], char *fname, void *), char *whose, void *etc ) {
  struct parser *pp = NewN( struct parser, 1 );
  pp->next = parsers;
  pp->whose = whose;
  pp->etc = etc;
  pp->parsefunc = parsefunc;
  parsers = pp;
}

int parti_read( struct stuff **stp, int argc, char *argv[], char *fname ) {
  struct parser *pp;
  int val;
  for(pp = reader_parsers; pp; pp = pp->next)
    if(pp->parsefunc && (val = (*pp->parsefunc)( stp, argc, argv, fname, pp->etc )) != 0)
	return val;
  return 0;
}

void parti_add_reader( int (*parsefunc)(struct stuff **, int, char *[], char *, void *), char *whose, void *etc ) {
  struct parser *pp = NewN( struct parser, 1 );
  pp->next = reader_parsers;
  pp->whose = whose;
  pp->etc = etc;
  pp->parsefunc = parsefunc;
  reader_parsers = pp;
}

