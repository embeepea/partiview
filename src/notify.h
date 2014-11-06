#ifndef NOTIFY_H
#define NOTIFY_H
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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct notify_ Notify;
typedef struct notifynode_ NotifyNode;

typedef void (*NotifyFunc)(Notify *by, void *source, void *arg);

struct notifynode_ {
  NotifyFunc notefunc;
  void *source;
  void *arg;
  NotifyNode *next;
};

struct notify_ {
  int notifying;
  NotifyNode *first;
};

extern void notify_add( Notify **slot, NotifyFunc notefunc, void *source, void *arg );
extern int  notify_remove( Notify *head, NotifyFunc notefunc, void *source, void *arg );
extern void notify_all( Notify *head );

#ifdef __cplusplus
}  /* end extern "C" */
#endif
#endif /* SCLOCK_H */
