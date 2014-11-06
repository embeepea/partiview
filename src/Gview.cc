/*
 * Geomview-style 3-D view widget for FLTK.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef _WIN32
# include "winjunk.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "geometry.h"
#include "textures.h"	/* for set_dsp_context() */
#include <memory.h>

#include "Gview.H"


#ifndef wallclock_time
 extern "C" { extern double wallclock_time(void); }	// from sclock.c
#endif

static void translation(Matrix *T, float x, float y, float z);
static void rotation(Matrix *T, float x, float y, float z);

Fl_Gview::Fl_Gview(int x, int y, int w, int h, const char *label)
		: Fl_Gl_Window(x,y,w,h,label) {
  init();
  end();
}

void Fl_Gview::glmode( int bits )
{
  if(this->dspcontext() >= 0 && this->shown() && bits != this->mode()) {
    /* It'll probably require a new window, and all our
     * opengl context will get lost.
     * Let textures.c know about this so it can reinit.
     */
    this->dspcontext( Fl_Gview::next_dspcontext() );
  }
  this->mode( bits );
}

int Fl_Gview::next_dspcontext_ = 0;

int Fl_Gview::next_dspcontext()
{
   return next_dspcontext_++;
}

void Fl_Gview::dspcontext( int dspctx )
{
  if(next_dspcontext_ <= dspctx)
    next_dspcontext_ = dspctx + 1;
  this->dspcontext_ = dspctx;
}

void Fl_Gview::lookvec(int axis, const Point *vec) {
  Point v = *vec;
  if(axis < 0 || axis > 3) return;
  if(axis == 2) vscale(&v, -1,&v);	/* cam looks toward -Z */
  /* XXX fill in orthogonalization, invert, load into Tw2c */
  /* Don't need this right now */
  Fl::warning("Fl_Gview::lookvec() not implemented yet");
  redraw();
}

const Matrix *Fl_Gview::Tc2w() const {
  return &Tc2w_;
}

const Matrix *Fl_Gview::Tw2c() const {
  return &Tw2c_;
}

void Fl_Gview::Tc2w( const Matrix *newTc2w ) {
  Tc2w_ = *newTc2w;
  eucinv(&Tw2c_, &Tc2w_);
  notify();
  redraw();
}

void Fl_Gview::Tw2c( const Matrix *newTw2c ) {
  Tw2c_ = *newTw2c;
  eucinv(&Tc2w_, &Tw2c_);
  notify();
  redraw();
}

void Fl_Gview::reset( int id ) {
  if(id == GV_ID_CAMERA) {
    translation( &Tc2w_, 0, 0, focallen_ );
    Tc2w( &Tc2w_ );
  } else {
    const Matrix *o2w = To2w( id );
    if(o2w && o2w != &Tidentity) {
	*(Matrix *)o2w = Tidentity;	/* Reset! */
    }
  }
  notify();
  redraw();
}

void Fl_Gview::focallen(float flen) {
  if(flen == 0) {
    Fl::warning("Can't set Fl_Gview::focallen() to zero");
  } else {
    if(persp_)
	halfyfov_ *= flen / focallen_;
    focallen_ = flen;
  }
  notify();
  redraw();
}

float Fl_Gview::angyfov() const {
  return 2*atan(halfyfov_ / focallen_)*180/M_PI;
}

void Fl_Gview::angyfov(float deg) {
  if(deg == 0) Fl::warning("Can't set Fl_Gview::angyfov() to zero");
  else if(deg <= -180 || deg >= 180)
	Fl::warning("Can't set Fl_Gview::angyfov() to >= 180");
  else halfyfov_ = focallen_ * tan((deg/2)*M_PI/180);
  notify();
  redraw();
}

void Fl_Gview::halfyfov( float hyfov ) {
  if(hyfov == 0) Fl::warning("Can't set Fl_Gview::halfyfov() to zero");
  else halfyfov_ = hyfov;
  redraw();
}

void Fl_Gview::perspective( int bepersp ) {
  if((bepersp!=0) == persp_)
    return;
  persp_ = (bepersp != 0);
  redraw();
  notify();
}

void Fl_Gview::farclip( float cfar ) {
  if(cfar == far_) return;
  if(cfar != 0 || !persp_) {
    far_ = cfar;
    notify();
    redraw();
  }
}

void Fl_Gview::nearclip( float cnear ) {
  if(cnear == near_) return;
  if(cnear != 0 || !persp_) {
    near_ = cnear;
    notify();
    redraw();
  }
}

void Fl_Gview::stereooffset( int newoff ) {
  if(newoff != stereooff_) {
    stereooff_ = newoff;
    notify();
    redraw();
  }
}

void Fl_Gview::pixelaspect( float pixasp ) {
  if(pixasp != 0 && pixasp != pixelaspect_) {
    pixelaspect_ = pixasp;
    notify();
    redraw();
  }
}


void Fl_Gview::center( const Point *cenw ) {
  if(cenw) pcenw_ = *cenw;
  else pcenw_.x[0] = pcenw_.x[1] = pcenw_.x[2] = 0;
  notify();
}

int Fl_Gview::next_id() const {
  int id;
  for(id = 1; withid(id) >= 0; id++)
    ;
  return id;
}

int Fl_Gview::add_drawer( void (*func)( Fl_Gview *, void *obj, void *arg ),
	void *obj, void *arg, char *name, int id )
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
  dp->To2w = Tidentity;
  dp->name = name;
  dp->id = id;
  dp->parent = 0;
  dp->objclip = 0;
  dp->objnear = 0;
  dp->objfar = 0;
  notify();
  redraw();
  return id;
}

int Fl_Gview::withid( int id ) const {	// Which drawer[] slot is id in?
  for(int dno = 0; dno < ndrawers_; dno++)
    if(drawers_[dno].id == id)
	return dno;
  return -1;
}

const Matrix *Fl_Gview::To2w( int id ) const {
  if(id == GV_ID_CAMERA)
    return Tc2w();
  int dno = withid( id );
  return (dno < 0) ? &Tidentity : &drawers_[dno].To2w;
}

int Fl_Gview::To2w( int id, const Matrix *newT ) {
  if(id == GV_ID_CAMERA) {
    Tc2w( newT );
    return -1;
  }
  int drawerno = withid( id );
  if(drawerno < 0) return 0;
  drawers_[drawerno].To2w = newT ? *newT : Tidentity;
  notify();
  redraw();
  return 1;
}

void Fl_Gview::objparent( int id, int parent ) {
  int drawerno = withid( id );
  if(drawerno >= 0)
    drawers_[drawerno].parent = parent;
}

int Fl_Gview::objparent( int id ) const {
  int drawerno = withid( id );
  return (drawerno >= 0) ? drawers_[drawerno].parent : 0;
}

void Fl_Gview::subc_lrbt( float subclrbt[4] ) {
  for(int k = 0; k < 4; k++)
    subclrbt_[k] = subclrbt[k];
}

#define VIEW_CLEAR  0x1
#define	VIEW_RED    0x2
#define	VIEW_CYAN   0x4

void Fl_Gview::glprojection( float nearclip, float farclip, const Matrix *postproj )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    if(inpick()) {
	GLint vp[4] = {0, 0, w(), h()};
	if(stereo_ == GV_CROSSEYED) {
	    /* Jigger viewport -- choose whichever half this pick came from */
	    int myw = (w() - stereooff_)/2;
	    vp[2] = myw;
	    if(pickx_ > myw) vp[0] = w() - myw;
	}
	gluPickMatrix( pickx_, picky_, pickwidth_, pickheight_, vp );
    }
    if(use_subc_) {
	glFrustum( nearclip * subclrbt_[0], nearclip * subclrbt_[1],
		   nearclip * subclrbt_[2], nearclip * subclrbt_[3],
		   nearclip, farclip );

    } else if(persp_) {
	float nthf = nearclip * halfyfov_ / focallen_;
	glFrustum( -nthf * aspect_, nthf * aspect_,
		   -nthf, nthf,
		   nearclip, farclip );
    } else {
	glOrtho( -aspect_*halfyfov_, aspect_*halfyfov_,
		 -halfyfov_, halfyfov_,
		  nearclip, farclip );
    }
    if(postproj)
	glMultMatrixf( postproj->m );
    glMatrixMode( GL_MODELVIEW );
}

void Fl_Gview::draw_scene( int how, const Matrix *postproj ) {
  /* draw scene */
  float then = wallclock_time();

  if(this->dspcontext() >= 0)
    set_dsp_context( this->dspcontext() );	/* alert texture layer */

  if(focalpointing_) {
      /* Compute distance from camera point to focal point.
       * Or, should we do this as a focal-plane distance instead?
       * If we just compute a point distance, it'll make sense even
       * if the point is behind the camera plane.
       * Note that the focalpoint is specified in *world* coordinates.
       */
    Point fpcam;
    vtfmpoint( &fpcam, &focalpoint_, Tw2c() );
    float dist = vlength( &fpcam );
    focallen( (dist > minfocallen_) ? dist : minfocallen_ );
  }

  glClearDepth( 1.0 );
  if(how & VIEW_CLEAR) {
    glColorMask( 1, 1, 1, 1 );
    glClearColor( bgcolor_.x[0], bgcolor_.x[1], bgcolor_.x[2], 0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  } else {
    glClear( GL_DEPTH_BUFFER_BIT );
  }

  if(how & VIEW_RED)
    glColorMask( 1, 0, 0, 1 );
  else if(how & VIEW_CYAN)
    glColorMask( 0, 1, 1, 1 );

  glEnable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_COLOR_MATERIAL );

  if(how || postproj)
    glprojection( near_, far_, postproj );


  int curclip = 0;
  int again = 0;

  do {
    if(predraw)
      (*predraw)( this, again );

    if(how || postproj) {
      glMatrixMode( GL_MODELVIEW );
      if(use_subc_) {
	glLoadMatrixf( Tc2subc()->m );
	glMultMatrixf( Tw2c()->m );
      } else {
	glLoadMatrixf( Tw2c()->m );
      }
    }

    for(int i = 0; i < ndrawers_; i++) {
      struct drawer *dp = &drawers_[i];
      if(dp->func != NULL) {
	int wantclip = dp->objclip;
	if(wantclip)
	  glprojection( dp->objnear, dp->objfar, postproj );
	else if(curclip)
	  glprojection( near_, far_, postproj );
	curclip = wantclip;

	glPushMatrix();
	if(dp->parent == GV_ID_CAMERA) {
	  if(use_subc_)
	    glLoadMatrixf( Tc2subc()->m );
	  else
	    glLoadIdentity();
	}
	glMultMatrixf( dp->To2w.m );
	if(inpick_) {
	  glLoadName( dp->id );
	  glPushName(0);
	}
	(*dp->func)(this, dp->obj, dp->arg);
	if(inpick_)
	  glPopName();
	glPopMatrix();
      }
    }
    if(postdraw)
      again = (*postdraw)( this, again );

    if(again) {
      //marx added to 0.7.06  to address 'duplicate' image when using multiple channels
      glClearDepth( 1.0 );
      if(how & VIEW_CLEAR) {
	glColorMask( 1, 1, 1, 1 );
	glClearColor( bgcolor_.x[0], bgcolor_.x[1], bgcolor_.x[2], 0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
      } else {
	glClear( GL_DEPTH_BUFFER_BIT );
      }
      //end marx added to 0.7.06 to address 'duplicate' image when using multiple channels
    }

  } while(again);

  rateaccum( wallclock_time() - then );
  framecount++;
}

static void stereoeye( Matrix *dst, float stereosep, float focallen ) {
  *dst = Tidentity;
  dst->m[4*2+0] = stereosep;
  dst->m[4*3+0] = stereosep * focallen;
}

void Fl_Gview::draw() {

  if(!valid() || damage() || inpick() || (stereo_ != GV_MONO)) {
    /* Assume reshaped */
    valid(1);
    glViewport( 0, 0, w(), h() );

#if defined(FL_MULTISAMPLE) && defined(GL_MULTISAMPLE_SGIS)
    if(this->mode() & FL_MULTISAMPLE)
	glEnable(GL_MULTISAMPLE_SGIS);
#endif

    aspect_ = h() > 0 ? pixelaspect_ * (float)w() / (float)h() : 1.0;

    Matrix postproj;

    switch(stereo_) {
    case GV_MONO:
    default:
	draw_scene( VIEW_CLEAR, NULL );
	break;

    case GV_REDCYAN:
	stereoeye( &postproj, stereosep_, focallen_ );
	draw_scene( VIEW_CLEAR|VIEW_RED, &postproj );
	stereoeye( &postproj, -stereosep_, focallen_ );
	draw_scene( VIEW_CYAN, &postproj );
	glColorMask( 1, 1, 1, 1 );
	break;

    case GV_QUADBUFFERED:
	stereoeye( &postproj, -stereosep_, focallen_ );
	glDrawBuffer( GL_BACK_RIGHT );
	draw_scene( VIEW_CLEAR, &postproj );

	stereoeye( &postproj, stereosep_, focallen_ );
	glDrawBuffer( GL_BACK_LEFT );
	draw_scene( VIEW_CLEAR, &postproj );
	break;

    case GV_LEFTEYE:
	stereoeye( &postproj, stereosep_, focallen_ );
	draw_scene( VIEW_CLEAR, &postproj );
	break;

    case GV_RIGHTEYE:
	stereoeye( &postproj, -stereosep_, focallen_ );
	draw_scene( VIEW_CLEAR, &postproj );
	break;

    case GV_CROSSEYED:
	int myw, myh;
	myw = (w() - stereooff_)/2;	/* or w()/2 - halfgap */
	myh = h();
	aspect_ = myh > 0 ? pixelaspect_ * myw / (float)myh : 1.0;
	
	stereoeye( &postproj, -stereosep_, focallen_ );
	glViewport( 0, 0, myw, myh );	/* right-eye view drawn on left side */
	draw_scene( VIEW_CLEAR, &postproj );

	stereoeye( &postproj, stereosep_, focallen_ );
	glViewport( w()-myw, 0, myw, myh );
	draw_scene( 0, &postproj );
	break;
    }
  } else {
    draw_scene( VIEW_CLEAR, NULL );
  }

  /* draw (I hope) any children lying on top of us */
  if(children() > 0) Fl_Gl_Window::draw();
}

int Fl_Gview::snapshot( int x, int y, int w, int h,  void *packedrgb )
{
  if(!visible_r())
    return 0;
  make_current();
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadBuffer( GL_FRONT );
  glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, packedrgb);
  return 1; // Might return whether this window was properly uncovered?
}

void Fl_Gview::takeMausSample( int ev, float mintime ) {
    float now = wallclock_time();
    if(ev == FL_PUSH)
	nms_ = 0;
    if(nms_ > 0 && now - ms_[nms_-1].t <= mintime)
	return;

    if(nms_ >= COUNT(ms_)-1) {
	for(int i = 1; i < COUNT(ms_); i++)
	    ms_[i-1] = ms_[i];
	nms_ = COUNT(ms_)-1;
    } else {
    }
    ms_[nms_].x = Fl::event_x();
    ms_[nms_].y = Fl::event_y();
    ms_[nms_].t = now;
    ++nms_;
}

int Fl_Gview::pastMaus( MausSample *tms, float then ) {
    if(nms_ == 0) return 0;
    int i;
    for(i = nms_; --i > 0 && ms_[i].t >= then; )
	;
    tms->x = ms_[i].x;
    tms->y = ms_[i].y;
    tms->t = ms_[i].t;
    return 1;
}   

float Fl_Gview::fps() const {
    return (sumw_ == 0) ? 0 : sumw_ / sumwdt_;
}

void Fl_Gview::rateaccum( float dt ) {
    float w = dt / 0.5;		/* half-second smoothing scale */
    sumwdt_ = w * dt + sumwdt_/(1 + w);
    sumw_ = w + sumw_/(1 + w);
}

static int wasESC;	/* double-ESC counter */

static int verbose;	/* for debugging! */

#define	XYBUTTON	FL_BUTTON1
#define	PICKBUTTON	FL_BUTTON2
#define	ZBUTTON		FL_BUTTON3

#define XYBUTTONVAL	FL_LEFT_MOUSE
#define	PICKBUTTONVAL	FL_MIDDLE_MOUSE
#define ZBUTTONVAL	FL_RIGHT_MOUSE

#define	SLOWKEY		FL_SHIFT
#define	CONSTRAINKEY	FL_CTRL

int Fl_Gview::handle(int ev) {
  
  if(eventhook) {
    // Allow clients to pre-screen our events without subclassing
    switch((*eventhook)(this, ev)) {
    case 1: return 1;	// pre-screener handled it
    case -1: return 0;  // pre-screener commands us to ignore it too
    default: break;	// Else just process event normally below
    }
  }

  switch(ev) {
  case FL_FOCUS:   hasfocus_ = 1; return 1;  // Yes, we want FL_KEYBOARD events
  case FL_UNFOCUS: hasfocus_ = 0; return 1;

  case FL_PUSH:
	{  // Besides navigating, grab keyboard focus if we're clicked-on
	    if(!hasfocus_) take_focus();
	}
	/* and fall into ... */
  case FL_RELEASE:

//=========== T. Takahei code added by Marx version 0.7.04 ===========
#ifndef __APPLE__
    if(Fl::event_state(XYBUTTON|ZBUTTON|PICKBUTTON) == 0
       && (Fl::event_button() == XYBUTTONVAL ||
	   Fl::event_button() == ZBUTTONVAL)) {
#else
      //if(Fl::event_state(XYBUTTON) == 0  && (!Fl::event_state(CONSTRAINKEY))) { //original 
      
      //handle a real two or three button mac mouse
      if(Fl::event_state(XYBUTTON|ZBUTTON|PICKBUTTON) == 0 && (Fl::event_button() == XYBUTTONVAL || Fl::event_button() == ZBUTTONVAL)) {
#endif
//============ end T. Takahei code =====================================

	    /* If everything's released, ... */
	    do_nav( FL_RELEASE, evslow_, evzaxis_, evconstrain_ );
	    return 1;
	}
	/* else fall through into... */

  case FL_DRAG:
    takeMausSample( ev, 0.125 );


//=========== T. Takahei code modified by Marx version 0.7.04 ===========
#ifndef __APPLE__
    if(Fl::event_state(XYBUTTON|ZBUTTON) && !Fl::event_state(PICKBUTTON)) {
      do_nav(ev, Fl::event_state(FL_SHIFT), Fl::event_state(ZBUTTON),
	     Fl::event_state(CONSTRAINKEY));
      return 1;
    } else if(Fl::event_state(PICKBUTTON)
	      && !Fl::event_state(XYBUTTON|ZBUTTON)
	      && pickcb_ != NULL) {
#else
      /* 
      //debug only
      if(Fl::event_state(FL_ALT))
        printf("you pressed option key\n");
      else if(Fl::event_state(FL_META))
	printf("you pressed apple key\n");
      else if(Fl::event_state(FL_CTRL))
	printf("you pressed ctrl key\n");
      else if(Fl::event_state(FL_BUTTON1))
	printf("you pressed left mouse button\n");
      else
	printf("i don't know what key you pressed\n");
      //end debug only
      */

      if(!Fl::event_state(FL_ALT)) { //if option key is pressed skip ahead for the test for mouse button simulation. 
	if(Fl::event_state(XYBUTTON|ZBUTTON) && !Fl::event_state(PICKBUTTON)) {
	do_nav(ev, Fl::event_state(FL_SHIFT), Fl::event_state(ZBUTTON),
	       Fl::event_state(CONSTRAINKEY));
	return 1;
	}
      }
      //if(Fl::event_state(XYBUTTON) && !Fl::event_state(CONSTRAINKEY)) { original but now we ignore meta and use constrainkey (FL_CTRL) to rotate in orbit mode (hope do_nav knows this!) 
      if(Fl::event_state(XYBUTTON) && !Fl::event_state(FL_META) && !Fl::event_state(PICKBUTTON)  ) { //enter mouse button simulation if meta key and or real middle mouse button are NOT pressed!
	//do_nav(ev, Fl::event_state(FL_SHIFT), Fl::event_state(FL_ALT), Fl::event_state(FL_META)); //toshi's original
	do_nav(ev, Fl::event_state(FL_SHIFT), Fl::event_state(FL_ALT), Fl::event_state(FL_CTRL));
        return 1;
      } else if (Fl::event_state(PICKBUTTON) && pickcb_ != NULL) {
    //} else if (Fl::event_state(XYBUTTON) && Fl::event_state(CONSTRAINKEY) && pickcb_ != NULL) { commented out per Brian Abbott request not to support CTRL key to emulate middle mouse button pick on Apple
#endif
//=========== end T. Takahei code modified by Marx version 0.7.04 ======================================= 

	dpickx_ = Fl::event_x();
	dpicky_ = h() - Fl::event_y();

	if(ev == FL_PUSH) {
	    do_pick( dpickx_, dpicky_ );
	    pickneeded_ = 0;
	} else {
	    if(!pickneeded_) {
		pickneeded_ = 1;
		Fl::add_idle( Fl_Gview::idlepick, (void *)this );
	    }
	}
	return 1;
    }
    return 0;

  case FL_ENTER:
    take_focus();
    return 1;

  case FL_KEYBOARD:

    if(Fl::event_text() == NULL || Fl::event_text()[0] == '\0')
	return 0;

    int c = Fl::event_text()[0];

    if(c != '\033')
	wasESC = 0;

    if(num.addchar( c ))
	return 1;


    switch(c) {
    case 'w': reset( retarget() ); notify(); redraw(); break;
    case 'r': retarget(); nav( GV_ROTATE ); break;
    case 'p':
    case 'P': if(num.has) retarget();
	      else {
		do_pick( Fl::event_x_root() - x_root(),
			 y_root() + h() - Fl::event_y_root() );
	      }
	      break;
    case 'f': retarget(); nav( GV_FLY ); break;
    case 't': retarget(); nav( GV_TRANSLATE ); break;
    case 'o': retarget(); nav( GV_ORBIT ); break;
    case 'O': perspective( num.value( !perspective() )  );
	      if(msg) msg("Perspective %s", persp_?"on":"off");
	      notify();
	      break;

    case 'v':
    case 'V':	if(num.has) angyfov( num.fvalue() );
		else halfyfov( halfyfov() * (c=='v' ? 1.25 : 1/1.25) );
		if(msg) msg("angyfov %g", angyfov());
		notify();
		break;

    case 'v'&0x1F: verbose = !verbose; break;

    case '@':
	Point cpos;
	Quat cquat;
	vgettranslation( &cpos, To2w( retarget() ) );
	tfm2quat( &cquat, To2w( retarget() ) );
	if(msg) msg("%s at %.4g %.4g %.4g  quat %.4g %.4g %.4g %.4g",
		dname( retarget() ),
		cpos.x[0],cpos.x[1],cpos.x[2],
		cquat.q[0], cquat.q[1], cquat.q[2], cquat.q[3]);
	break;

    case '=':
	const float *fp;
	int me;
	me = retarget();
	fp = &To2w(me)->m[0];
	if(msg) msg("%s To2w():", dname(me));
	int i;
	for(i = 0; i < 16; i+=4)
	    if(msg) msg("\t%9.5g %9.5g %9.5g %9.5g", fp[i],fp[i+1],fp[i+2],fp[i+3]);
	Matrix w2o;
	eucinv( &w2o, To2w(me) );
	fp = &w2o.m[0];
	if(msg) msg("%s Tw2o():", dname(me));
	for(i = 0; i < 16; i+=4)
	    if(msg) msg("\t%9.5g %9.5g %9.5g %9.5g", fp[i],fp[i+1],fp[i+2],fp[i+3]);
	float aer[3];
	Point xyz;
	tfm2xyzaer( &xyz, aer, To2w(me) );
	if(msg) {
	    msg("%s o2w = XYZ Ry Rx Rz FOV:", dname(me));
	    msg("  %g %g %g  %g %g %g  %g",
		xyz.x[0],xyz.x[1],xyz.x[2],
		aer[1],aer[0],aer[2],
		perspective() ? angyfov() : -2*halfyfov() );
	}
	break;

    case '\033':	  /* ESC */
	if(wasESC++ > 0)
	    exit(0);
    // case FL_End: Fl::warning("End key!"); return 1; // test!

    default: c = 0;
    }

    num.clear();

    /* Maybe check Fl::event_key(FL_HOME), etc.? */
    return c==0 ? 0 : 1;

  }
  return 0;
}

void Fl_Gview::idlepick( void *vthis ) {
  Fl_Gview *me = (Fl_Gview *)vthis;

  if(me->pickneeded_) {
    me->do_pick( me->dpickx_, me->dpicky_ );
    me->pickneeded_ = 0;
    Fl::remove_idle( Fl_Gview::idlepick, vthis );
  }
}


void Fl_Gview::notifier( void (*func)(Fl_Gview*,void*), void *arg ) {
  notify_ = func;
  notifyarg_ = arg;
}

void Fl_Gview::notify() {
  if(notify_ != NULL)
    (*notify_)( this, notifyarg_ );
}

void Fl_Gview::nav( enum Gv_Nav newnav ) {
  if(nav_ != newnav) {
    nav_ = newnav;
    driftoff();
    notify();
  }
}

/*
 * rot through theta about vector {x,y,z}, where tan(theta/2) = length(xyz)
 */
static void rotation(Matrix *T, float x, float y, float z)
{

  float chalf = sqrtf(1 / (1 + x*x + y*y + z*z));
  Quat rq = { chalf, x*chalf, y*chalf, z*chalf };
  quat2tfm(T, &rq);
}

static void translation(Matrix *T, float x, float y, float z)
{
  Point p = {x,y,z};
  *T = Tidentity;
  vsettranslation( T, &p );
}

void Fl_Gview::idledrift( void *vthis ) {
  Fl_Gview *me = (Fl_Gview *)vthis;
  Fl::check();
  if(me->drifting_) {
    me->do_nav( FL_DRAG, me->evslow_, me->evzaxis_, me->evconstrain_);
  } else {
    Fl::remove_idle( idledrift, vthis );
  }
}

void Fl_Gview::driftoff() {
  if(drifting_) {
    Fl::remove_idle( idledrift, this );
    drifting_ = 0;
  }
}

void Fl_Gview::drifton( MausSample *rate ) {
  if((rate->x == 0 && rate->y == 0) || rate->t == 0) {
    driftoff();
    return;
  }
  driftrate_ = *rate;
  if(!drifting_) {
    Fl::add_idle( idledrift, this );
    drifting_ = 1;
    drifttime_ = wallclock_time();
  }
}

#define MAXDELAY 1.2
#define MINDELAY 0.15

void Fl_Gview::drifton() {
  MausSample rate;
  float now = wallclock_time();
  if(pastMaus(&rate, now - 0.33)) {
    rate.x = Fl::event_x() - rate.x;
    rate.y = Fl::event_y() - rate.y;
    rate.t = now - rate.t;
	/* assume extreme times to be measurement errors */
    if(rate.t > MAXDELAY) rate.t = MAXDELAY;
    else if(rate.t < MINDELAY) rate.t = MINDELAY;
    if(fabs(rate.x) > nullthresh_ || fabs(rate.y) > nullthresh_) {
	drifton(&rate);
    } else {
	driftoff();
    }
  } else {
    driftoff();
  }
}


void Fl_Gview::inertia( int on ) {
  inertia_ = on;
  if(!inertia_)
    driftoff();
}
    

void Fl_Gview::start_nav( int mytarget ) {
    evx_ = Fl::event_x();
    evy_ = Fl::event_y();
    evTc2w_ = Tc2w_, evTw2c_ = Tw2c_;
    evTobj2w_ = *To2w( mytarget );
}


void Fl_Gview::do_nav(int ev, int slow, int zaxis, int constrained) {

  if((slow != evslow_ || zaxis != evzaxis_ || constrained != evconstrain_)
	&& (ev == FL_DRAG)) {
    /* If conditions changed, pretend button was released
     * (so we commit to this nav update) and
     * pushed again.
     */
    do_nav( FL_RELEASE, evslow_, evzaxis_, evconstrain_ );
    ev = FL_PUSH;
  }

  Gv_Nav curnav = (constrained && nav_==GV_ORBIT)
			? (zaxis ? GV_ROTATE : GV_FLY)
			: nav_;

  int mytarget = (curnav == GV_ORBIT || curnav == GV_FLY || !movingtarget())
		? GV_ID_CAMERA
		: target();

  if(ev == FL_PUSH) {
    start_nav( mytarget );
    evslow_ = slow;
    evzaxis_ = zaxis;
    evconstrain_ = constrained;
    driftoff();
    return;
  }

  if((ev == FL_DRAG || ev == FL_RELEASE) && w() > 0) {
    float slowrate = slow ? 0.05 : 1.0;
    int field = w() > h() ? w() : h();
    float dx = -(Fl::event_x() - evx_);
    float dy =  (Fl::event_y() - evy_);
    float angfield = halfyfov_ / focallen_;


    if(drifting()) {
	start_nav(mytarget);
	float now = wallclock_time();
	float tscale = (now - drifttime_) / driftrate_.t;
	drifttime_ = now;
	dx = -driftrate_.x * tscale;
	dy = driftrate_.y * tscale;
    }

    dx *= slowrate / field;
    dy *= slowrate / field;

    if(constrained && curnav == nav_) {
	// CTRL key means "constrain to X/Y axis",
	// except in Orbit mode where it means "fly" or "twist"!
	if(fabs(dx) < fabs(dy)) dx = 0;
	else dy = 0;
    }

    Matrix Tincr;
    const Matrix *Tf2w, *Tw2f;
    const Point *pcenterw = NULL;

    if(owncoords_) {
	Tf2w = Tw2f = NULL;
    } else {
	Tf2w = &evTc2w_, Tw2f = &evTw2c_;
    }

    switch(curnav) {
    case GV_ROTATE:
	if(zaxis)
	    rotation(&Tincr, 0, 0, (dx+dy));
	else
	    rotation(&Tincr, -2*dy, 2*dx, 0);
	pcenterw = &pcenw_;
	break;
    case GV_ORBIT:
	if(zaxis) {
	    Point pcamw;
	    vgettranslation( &pcamw, Tc2w() );
	    translation(&Tincr, 0, 0, (dx+dy) * vdist(&pcenw_, &pcamw));
	    start_nav( mytarget );
	} else {
	    rotation(&Tincr, -2*dy, 2*dx, 0);
	}
	pcenterw = &pcenw_;
	break;
    case GV_FLY:
	if(zaxis) {
	    translation(&Tincr, 0, 0, (dx+dy) * zspeed * focallen_);
	} else {
	    rotation(&Tincr, dy*angfield, -dx*angfield, 0);
	}
	pcenterw = NULL;
	break;
    case GV_TRANSLATE:
	if(zaxis) {
	    translation(&Tincr, 0, 0, -(dx+dy) * zspeed * focallen_);
	} else {
	    translation(&Tincr, 2 * dx * halfyfov_ * focallen_, 2 * dy * halfyfov_ * focallen_, 0);
	}
	pcenterw = NULL;
	break;
    default:
	fprintf(stderr, "Fl_Gview::do_nav(): Unknown nav mode %d\n", nav_);
	Tincr = Tidentity;
    }

    if(ev == FL_RELEASE) {
	if(inertia())
	    drifton();
	else
	    driftoff();
    }

    if(verbose) {
	Point dp;
	Quat dq;
	vgettranslation( &dp, &Tincr );
	tfm2quat( &dq, &Tincr );
	if(msg) msg("dx %.3f dy %.3f  angfield %.3g  dp %.4g %.4g %.4g  dq %.4g %.4g %.4g %.4g",
		dx, dy, angfield,
		dp.x[0],dp.x[1],dp.x[2],
		dq.q[0],dq.q[1],dq.q[2],dq.q[3]);
    }

    Matrix newTobj2w;
    mconjugate( &newTobj2w, &evTobj2w_, &Tincr,
		Tf2w, Tw2f, pcenterw, NULL );
    float scaling = vlength( (Point *)&evTobj2w_.m[0] );
    if(fabs(scaling - 1) < .01) {
	Point p;
	Quat q;
	vgettranslation( &p, &newTobj2w );
	tfm2quat( &q, &newTobj2w );
	quat2tfm( &newTobj2w, &q );
	vscale( (Point *)&newTobj2w.m[0], scaling, (Point *)&newTobj2w.m[0] );
	vscale( (Point *)&newTobj2w.m[4], scaling, (Point *)&newTobj2w.m[4] );
	vscale( (Point *)&newTobj2w.m[8], scaling, (Point *)&newTobj2w.m[8] );
	vsettranslation( &newTobj2w, &p );
    }

    To2w( mytarget, &newTobj2w );
    redraw();
  }
}

void Fl_Gview::picksize( float width, float height )
{
  pickwidth_ = width;
  pickheight_ = height;
}

void Fl_Gview::pickbuffer( int nents, GLuint *buf )
{
  picknents_ = nents;
  pickbuf_ = buf;
}

void Fl_Gview::picker( void (*pickcb)(Fl_Gl_Window *, int, int, GLuint *, void *), void *arg )
{
  pickcb_ = pickcb;
  pickarg_ = arg;
}

void (*Fl_Gview::picker(void **argp))(Fl_Gl_Window *, int, int, GLuint *, void *)
{
  if(argp) *argp = pickarg_;
  return pickcb_;
}

int Fl_Gview::pickresults( int *nentp, GLuint **bufp )
{
  if(nentp) *nentp = picknents_;
  if(bufp) *bufp = pickbuf_;
  return pickhits_;
}

int Fl_Gview::do_pick( float xpick, float ypick )
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

char *Fl_Gview::dname( int id ) {
  static char name[32];
  if(id < 0) return "[c0]";
  int dno = withid( id );
  if(dno >= 0) {
    sprintf(name, drawers_[dno].name ? "[g%d %.22s]" : "[g%d]",
	id, drawers_[dno].name);
  } else {
    sprintf(name, "[g%d?]", id);
  }
  return name;
}

void Fl_Gview::initfrom( Fl_Gview *v ) {
    msg = v->msg;
    eventhook = v->eventhook;
    predraw = v->predraw;
    postdraw = v->postdraw;
    ndrawers_ = v->ndrawers_;
    maxdrawers_ = v->maxdrawers_;
    drawers_ = (drawer *)malloc(maxdrawers_ * sizeof(drawer));
    memcpy(drawers_, v->drawers_, ndrawers_ * sizeof(drawer));
    owncoords_ = v->owncoords_;
    persp_ = v->persp_;
    stereo_ = v->stereo_;
    pixelaspect_ = v->pixelaspect_;
    stereooff_ = v->stereooff_;
    aspect_ = v->aspect_;
    stereosep_ = v->stereosep_;
    target_ = v->target_;
    movingtarget_ = v->movingtarget_;
    focallen_ = v->focallen_;
    halfyfov_ = v->halfyfov_;
    near_ = v->near_;
    far_ = v->far_;
    zspeed = v->zspeed;
    inpick_ = v->inpick_;
    pickwidth_ = v->pickwidth_, pickheight_ = v->pickheight_;
    picknents_ = v->picknents_;
    pickbuf_ = v->pickbuf_;
    pickarg_ = v->pickarg_;
    pickcb_ = v->pickcb_;
    notify_ = v->notify_;
    notifyarg_ = v->notifyarg_;
    pcenw_ = v->pcenw_;
    bgcolor_ = v->bgcolor_;
    nullthresh_ = v->nullthresh_;
    hasfocus_ = 0;
    use_subc_ = v->use_subc_;
    Tc2subc_ = v->Tc2subc_;
    pickcb_ = v->pickcb_;
    for(int i = 0; i < 4; i++)
	subclrbt_[i] = v->subclrbt_[i];
    nav( v->nav() );
    inertia_ = v->inertia_;
    drifting_ = 0;
    driftrate_.x = driftrate_.y = driftrate_.t = 0;
    nms_ = 0;
    
    Tc2w( v->Tc2w() );
}
