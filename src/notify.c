/*
 * Keep notification lists so dependents can register to be
 * told when something changes.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "shmem.h"
#include "notify.h"

void notify_add( Notify **slot, void (*notefunc)(Notify*, void*src, void*arg), void *source, void *arg ) {
  Notify *knot;
  NotifyNode *node;
  if(slot == NULL || notefunc == NULL) return;
  if((knot = *slot) == NULL) {
    knot = NewN( Notify, 1 );
    knot->notifying = 0;
    knot->first = NULL;
    *slot = knot;
  }
  node = NewN( NotifyNode, 1 );
  node->notefunc = notefunc;
  node->source = source;
  node->arg = arg;
  node->next = knot->first;
  knot->first = node;
}

int notify_remove( Notify *knot, NotifyFunc notefunc, void *source, void *arg ) {
  NotifyNode *n, **np;
  int count = 0;
  if(knot == NULL) return 0;
  for(np = &knot->first; (n = *np) != NULL; np = &(*np)->next) {
    if(n->notefunc == notefunc &&
	    n->source == source &&
	    n->arg == arg) {
	*np = n->next;
	Free( n );
	count++;
    }
  }
  return count;
}

void notify_all( Notify *knot ) {
  NotifyNode *n;
  if(knot == NULL || knot->notifying) return;
  knot->notifying = 1;
  for(n = knot->first; n != NULL; n = n->next) {
    (*n->notefunc)(knot, n->source, n->arg);
  }
  knot->notifying = 0;
}
