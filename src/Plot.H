#ifndef _PLOT_H
#define _PLOT_H
/*
 * FLTK OpenGL 2-D plot widget.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#include <stdio.h>

#ifndef __cplusplus

typedef struct _Fl_Plot Fl_Plot;
int Plot_add_drawer( Fl_Plot *,
			void (*func)( Fl_Plot *, void *obj, void *arg ),
			void *obj, void *arg, CONST char *name, int id );
int Plot_inpick( Fl_Plot * );	/* for plain-C drawers */
void Plot_setpick( Fl_Plot *, void (*pickcb)(Fl_Plot *, int hits, int ents, GLuint *buf) );

#else /* C++ */

#include <stdlib.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include "geometry.h"

class Fl_Plot : public Fl_Gl_Window {

 public:

  Fl_Plot(int x, int y, int w, int h, const char *label = 0);

  int add_drawer( void (*func)( Fl_Plot *, void *obj, void *arg ), void *obj, void *arg, const char *name = NULL, int id = -1 );
  int  next_id() const;
  void notifier( void (*func)( Fl_Plot *, void * ), void * );
  int  (*msg)( const char *fmt, ... );
  int  (*eventhook)( Fl_Plot *, int ev );

  void xrange( float xmin, float xmax );
  void yrange( float ymin, float ymax );
  float x0() const { return x0_; }
  float x1() const { return x1_; }
  float y0() const { return y0_; }
  float y1() const { return y1_; }

  void xtitle( const char * );
  void ytitle( const char * );
  char *xtitle() const { return xtitle_; }
  char *ytitle() const { return ytitle_; }

  float zoomview( float zoomin, int xev = 0, int yev = 0 );

  int inpick() const { return inpick_; } // draw()ers: am I in GL_SELECT mode?
  void picksize( float width, float height );
  void pickbuffer( int words, GLuint *buf );
  void picker( void (*pickcb)(Fl_Gl_Window *, int, int, GLuint *, void *arg), void *arg ); 
  void (*picker( void **argp = 0 ))(Fl_Gl_Window *, int, int, GLuint *, void *arg);
  int pickresults( int *wordsp, GLuint **bufp );  /* returns num hits, too */
  int do_pick( float pickx, float picky ); /* Calls back function given by picker() */

  int snapshot( int x, int y, int w, int h, void *packedrgb );
	// Take snapshot into caller-supplied buffer, w*h*3 bytes long

  int ndrawers() const { return ndrawers_; }

  char *dname( int drawerno );

  void bgcolor( Point *rgb ) { bgcolor_ = *rgb; redraw(); }
  const Point *bgcolor() const { return &bgcolor_; }

  virtual void draw();
  virtual int  handle(int ev);

  static void idlepick( void *me );

 protected:
  float x0_, x1_, y0_, y1_;
  float initx0_, inity0_, initx1_, inity1_;
  char *xtitle_, *ytitle_;

  float orthox0_, orthox1_, orthoy0_, orthoy1_;
  void (*pickcb_)( Fl_Gl_Window *, int hits, int nents, GLuint *buf, void *arg );
  int inpick_;
  float pickx_, picky_, pickwidth_, pickheight_;
  int picknents_, pickhits_;
  GLuint *pickbuf_;
  GLuint tpickbuf[1024];
  void *pickarg_;

	/* We handle mouse-driven picking in an idle-callback, to avoid
	 * getting behind if there are many events.
	 */
  float dpickx_, dpicky_;
  int pickneeded_;
  int hasfocus_;

  void (*notify_)( Fl_Plot *, void *);
  void *notifyarg_;
  void notify();
				/* navigation settings */
  /* Sweep status -- where current mouse drag started */
  int evx_, evy_, evzaxis_, evconstrain_;

  struct drawer {
    void (*func)(Fl_Plot *, void *obj, void *arg);
    void *obj, *arg;
    char *name;
    int id;
  };
  int ndrawers_, maxdrawers_;
  struct drawer *drawers_;
  int withid( int id ) const;
  void draw_scene( int needproj, const Matrix *postproj );

  Point bgcolor_;	/* OK, it's not a Point, we just want 3 components */

  void do_nav(int ev, int slow, int zaxis, int constrained);

  void start_nav( int navtarget ); /* Save event-position, camera, target-position */

  void init() {
    msg = printf;
    eventhook = NULL;
    ndrawers_ = maxdrawers_ = 0;
    drawers_ = NULL;
    inpick_ = 0;
    pickwidth_ = 2, pickheight_ = 2;

    orthox0_ = -.15;
    orthox1_ = 1.05;
    orthoy0_ = -.15;
    orthoy1_ = 1.05;

    picknents_ = sizeof(tpickbuf) / sizeof(tpickbuf[0]);
    pickbuf_ = tpickbuf;
    pickarg_ = NULL;
    notify_ = NULL;
    notifyarg_ = NULL;
    bgcolor_.x[0] = bgcolor_.x[1] = bgcolor_.x[2] = .2;
    hasfocus_ = 0;
  }
};
#endif /* C++ */
#endif /*_GVIEW_H*/
