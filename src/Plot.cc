#ifdef WIN32
# include "winjunk.h"
#endif
/*
 * FLTK OpenGL 2-D plot widget.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#if unix && !__APPLE__
# include "FL/x.H"
#endif

#include <string.h>
#include <math.h>
#include "Plot.H"
#include "sfont.h"
#include "shmem.h"

int Plot_add_drawer( Fl_Plot *plot,
			void (*func)( Fl_Plot *, void *obj, void *arg ),
			void *obj, void *arg, char *name, int id ) {
  return plot->add_drawer( func, obj, arg, name, id );
}

int Plot_inpick( Fl_Plot *plot ) { return plot->inpick(); }

void Plot_setpick( Fl_Plot *, void (*pickcb)(Fl_Plot *, int hits, int ents, GLuint *buf) );

Fl_Plot::Fl_Plot(int x, int y, int w, int h, const char *label)
		: Fl_Gl_Window(x,y,w,h,label) {
  init();
  end();
}


int Fl_Plot::add_drawer( void (*func)( Fl_Plot *, void *obj, void *arg ),
				void *obj, void *arg, const char *name, int id )
{
  if(id < 0) id = next_id();
  int dno = withid( id );
  if(dno < 0) {
    if(ndrawers_ >= maxdrawers_) {
	maxdrawers_ = ndrawers_*2 + 15;
	int room = maxdrawers_ * sizeof(struct drawer);
	drawers_ = (struct drawer *)
		    (drawers_==NULL ? malloc(room) : realloc(drawers_, room));
    }
    dno = ndrawers_++;
  }
  struct drawer *dp = &drawers_[dno];
  dp->func = func;
  dp->obj = obj;
  dp->arg = arg;
  dp->name = shmstrdup(name);
  dp->id = id;
  notify();
  redraw();
  return id;
}

void Fl_Plot::notify() {
  if(notify_ != NULL)
    (*notify_)( this, notifyarg_ );
}


int Fl_Plot::withid( int id ) const {	// Which drawer[] slot is id in?
  for(int dno = 0; dno < ndrawers_; dno++)
    if(drawers_[dno].id == id)
	return dno;
  return -1;
}

int Fl_Plot::next_id() const {
  int id;
  for(id = 1; withid(id) >= 0; id++)
    ;
  return id;
}

void draw_axis( float v0, float v1, float ybase, float ytick, float htext, char *just, char *title ) {
  char ljust[4], rjust[4];
  sprintf(ljust, "%.2sw", just?just:"");
  sprintf(rjust, "%.2se", just?just:"");
  glColor3f( 1,1,1 );
  glBegin( GL_LINES );
  glVertex2f( 0,ybase );
  glVertex2f( 1,ybase );
  glVertex2f( 0,ybase );
  glVertex2f( 0,ytick );
  glVertex2f( 1,ybase );
  glVertex2f( 1,ytick );
  glEnd();
  char lbl[16];
  sprintf(lbl, "%.2g", v0);
  Point at = { 0, ytick, 0 };
  sfStrDrawTJ( lbl, htext*.8, &at, NULL, ljust );

  sprintf(lbl, "%.2g", v1);
  at.x[0] = 1;
  sfStrDrawTJ( lbl, htext*.8, &at, NULL, rjust );

  if(title) {
    at.x[0] = .5;
    at.x[1] = ytick + (ytick-ybase);
    sfStrDrawTJ( title, htext, &at, NULL, "c" );
  }
}

static float coordof( int pixel, int npix, float o0, float o1, float v0, float v1 ) {
  float o = pixel * (o1-o0) / npix + o0;
  return o * (v1-v0) + v0;
}

static float rounded( int prec, float v ) {
  char str[32];
  sprintf(str, "%.*g", prec, v);
  return atof(str);
}

static float choosezoom( float zoom, int pixel,
			int npix, float o0, float o1, float *v0p, float *v1p ) {
  float range = (*v1p - *v0p);
  if(range == 0) return 1;		/* unchanged */
  float newv0, newv1, error;

  float center = coordof( pixel, npix, o0, o1, *v0p, *v1p );
  for(int prec = 0; prec < 4; prec++) {
    newv0 = rounded( prec, center - .5*range/zoom );
    newv1 = rounded( prec, center + .5*range/zoom );
    error = 1 - zoom*(newv1 - newv0)/range;
    if(fabs(error) < .1) break;
  }
  if(newv1 == newv0) return 1;		/* degenerate after 4 digits?! */
  *v0p = newv0;
  *v1p = newv1;
  return range / (newv1 - newv0);
}

float Fl_Plot::zoomview( float zoomfactor, int xev, int yev ) {
  if(zoomfactor == 0) {
    xrange( initx0_, initx1_ );
    yrange( inity0_, inity1_ );
    redraw();
    return 1.0;
  }
  choosezoom( zoomfactor, h()-yev, h(), orthoy0_, orthoy1_, &y0_, &y1_ );
  float got = choosezoom( zoomfactor, xev, w(), orthox0_, orthox1_, &x0_, &x1_ );
#if unix && !__APPLE__
  Window xwin = fl_xid(this);
  if(xwin != None)
      XWarpPointer(fl_display, xwin, xwin, 0, 0, 0, 0, 
	         (int)(w()*(.5 - orthox0_)/(orthox1_ - orthox0_)),
	         (int)(h()*(orthoy1_ - .5)/(orthoy1_ - orthoy0_)));
#endif
  redraw();
  return got;
}

void Fl_Plot::draw() {
  /* draw scene */

  if(!valid() || damage() || inpick()) {
    /* Assume reshaped */
    valid(1);
    glViewport( 0, 0, w(), h() );
  }

  glClearDepth( 1.0 );
  glColorMask( 1, 1, 1, 1 );
  glClearColor( bgcolor_.x[0], bgcolor_.x[1], bgcolor_.x[2], 0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_COLOR_MATERIAL );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  if(inpick()) {
    GLint vp[4] = {0, 0, w(), h()};
    gluPickMatrix( pickx_, picky_, pickwidth_, pickheight_, vp );
  }
  glOrtho( orthox0_, orthox1_,  orthoy0_, orthoy1_,  -1, 1 );
  glMatrixMode( GL_MODELVIEW );

  glPushMatrix();
  glScalef( x1_ == x0_ ? 1 : 1/(x1_-x0_),
	    y1_ == y0_ ? 1 : 1/(y1_-y0_), 1 );
  glTranslatef( -x0_, -y0_, 0 );
  for(int i = 0; i < ndrawers_; i++) {
    struct drawer *dp = &drawers_[i];
    if(dp->func != NULL) {
	if(inpick_) {
	    glLoadName( dp->id );
	    glPushName( 0 );
	}
	(*dp->func)( this, dp->obj, dp->arg );
	if(inpick_)
	    glPopName();
    }
  }
  glPopMatrix();

  /* Draw axes */

  draw_axis( x0_, x1_, -.01, -.04, .1, "n", xtitle() );
  glPushMatrix();
  glRotatef( 90, 0,0,1 );
  draw_axis( y0_, y1_, .01, .04, .1, "s", ytitle() );
  glPopMatrix();

  if(children() > 0)
    Fl_Gl_Window::draw();
}

int Fl_Plot::handle(int ev) {

  if(eventhook) {
    /* Allow clients to pre-screen our events without subclassing */
    switch((*eventhook)(this, ev)) {
    case 1: return 1;	/* pre-screener handled it */
    case -1: return 0;  /* pre-screener commands us to ignore it too */
    default: break;	/* Else just process event normally below */
    }
  }

  if(Fl_Gl_Window::handle(ev))
    return 1;

  switch(ev) {
  case FL_FOCUS:   hasfocus_ = 1; return 1;  // Yes, we want FL_KEYBOARD events
  case FL_UNFOCUS: hasfocus_ = 0; return 1;

  case FL_PUSH:
	{  // Besides navigating, grab keyboard focus if we're clicked-on
	    if(!hasfocus_) take_focus();
	}
  case FL_DRAG:
  case FL_RELEASE:
    if(pickcb_ != NULL) {

	dpickx_ = Fl::event_x();
	dpicky_ = h() - Fl::event_y();

	if(ev == FL_PUSH) {
	    do_pick( dpickx_, dpicky_ );
	    pickneeded_ = 0;
	} else {
	    if(!pickneeded_) {
		pickneeded_ = 1;
		Fl::add_idle( Fl_Plot::idlepick, (void *)this );
	    }
	}
	return 1;
    }
    /* Maybe do right-button pick? */
    return 0;

  case FL_ENTER:
    take_focus();
    return 1;

  case FL_KEYBOARD:

    if(Fl::event_text() == NULL || Fl::event_text()[0] == '\0')
	return 0;

    int c = Fl::event_text()[0];

    switch(c) {
    case 'p':
    case 'P':
		do_pick( Fl::event_x_root() - x_root(),
			 y_root() + h() - Fl::event_y_root() );
	      break;

    case 'V':	zoomview( 1.33, Fl::event_x(), Fl::event_y() ); break;
    case 'v': 	zoomview( 1/1.33, Fl::event_x(), Fl::event_y() ); break;
    case 'w':   zoomview( 0 ); break;

    default: c = 0;
    }

    if(Fl::event_key(FL_Home)) {
	zoomview( 0 );
	return 1;
    }

    return c==0 ? 0 : 1;

  }
  return 0;
}


void Fl_Plot::xrange( float x0, float x1 ) {
    initx0_ = x0_ = x0;
    initx1_ = x1_ = x1;
    notify(); redraw();
}
void Fl_Plot::yrange( float y0, float y1 ) {
    inity0_ = y0_ = y0;
    inity1_ = y1_ = y1;
    notify(); redraw();
}
void Fl_Plot::xtitle( const char *str ) {
	if(xtitle_) Free(xtitle_);
	xtitle_ = str ? shmstrdup(str) : NULL;
	notify();
	redraw();
}
void Fl_Plot::ytitle( const char *str ) {
	if(ytitle_) Free(ytitle_);
	ytitle_ = str ? shmstrdup(str) : NULL;
	notify();
	redraw();
}

void Fl_Plot::picksize( float width, float height )
{
  pickwidth_ = width;
  pickheight_ = height;
}

void Fl_Plot::pickbuffer( int words, GLuint *buf )
{
  picknents_ = words;
  pickbuf_ = buf;
}
void Fl_Plot::picker( void (*pickcb)(Fl_Gl_Window *, int, int, GLuint *, void *arg), void *arg )
{
  pickcb_ = pickcb;
  pickarg_ = arg;
} 

int Fl_Plot::pickresults( int *nentp, GLuint **bufp )
{
  if(nentp) *nentp = picknents_;
  if(bufp) *bufp = pickbuf_;
  return pickhits_;
}

int Fl_Plot::do_pick( float xpick, float ypick )
{
  make_current();
  glSelectBuffer( picknents_, pickbuf_ );
  int ok = glRenderMode( GL_SELECT );
  if(ok != 0) {
    fprintf(stderr, "Trouble in do_pick: glRenderMode( GL_SELECT ) = %d\n", ok);
    ok = glRenderMode( GL_SELECT );
    fprintf(stderr, "Retry: %d\n", ok);
  }
  glPushName(0);

  inpick_ = 1;
  pickx_ = xpick;
  picky_ = ypick;
  draw();
  inpick_ = 0;

  glPopName();
  pickhits_ = glRenderMode( GL_RENDER );
  if(pickcb_)
    (*pickcb_)( this, pickhits_, picknents_, pickbuf_, pickarg_ );
  valid(0);
  return pickhits_;
}

int Fl_Plot::snapshot( int x, int y, int w, int h,  void *packedrgb )
{
  make_current();
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadBuffer( GL_FRONT );
  glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, packedrgb);
  return 1; // Might return whether this window was properly uncovered?
}
    // Take snapshot into caller-supplied buffer, w*h*3 bytes long

void Fl_Plot::idlepick( void *vthis ) {
  Fl_Plot *me = (Fl_Plot *)vthis;

  if(me->pickneeded_) {
    me->do_pick( me->dpickx_, me->dpicky_ );
    me->pickneeded_ = 0;
    Fl::remove_idle( Fl_Plot::idlepick, vthis );
  }
}
