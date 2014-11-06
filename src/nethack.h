#ifndef NETHACK_H
#define NETHACK_H

/*
 * Over-the-network control of partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

extern void nethack_init();
extern int net_parse_args( struct stuff **, int argc, char *argv[], void * );

#endif /*NETHACK_H*/
