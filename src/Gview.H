#ifndef _GVIEW_H
#define _GVIEW_H
/*
 * Somewhat geomview-style OpenGL viewer for FLTK
 * Analogous to vtk's RenderWindowInteractor.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>    /* for GLuint */
#include <GL/glu.h>
#endif

#include <stdio.h>

#define  GV_ID_CAMERA    (-1)

#ifndef __cplusplus

typedef struct _Fl_Gview Fl_Gview;
int Gview_add_drawer( Fl_Gview *gview,
			void (*func)( Fl_Gview *, void *obj, void *arg ),
			void *obj, void *arg, int id );
void Gview_get_o2w( Fl_Gview *gview, Matrix *pos, int drawer );	/* returns parent */
void Gview_set_o2w( Fl_Gview *gview, CONST Matrix *pos, int drawer );
int Gview_inpick( Fl_Gview *gview );	/* for plain-C drawers */
void Gview_setpick( Fl_Gview *gview, void (*pickcb)(Fl_Gview *, int hits, int ents, GLuint *buf) );

#else /* C++ */

#include <stdlib.h>

#ifdef FLHACK
# include "flhack.H"
#else
# include <FL/Fl.H>
# include <FL/Fl_Gl_Window.H>
#endif

#include "geometry.h"

enum Gv_Nav {
  GV_FLY,
  GV_ORBIT,
  GV_ROTATE,
  GV_TRANSLATE,
  GV_LASTENUM
};

enum Gv_Stereo {
  GV_MONO,
  GV_REDCYAN,
  GV_CROSSEYED,
  GV_QUADBUFFERED,
  GV_LEFTEYE,
  GV_RIGHTEYE
};

struct GNumber {
  int has;	/* 0 if not, -1 or +1 (sign) if present */
  int val;
  int expon;	/* digits after decimal point */
  int prefix;	/* prefix character (e.g. 'c' or 'g') */

  void clear() { has = val = expon = prefix = 0; }
  int addchar(int c) {
    switch(c) {
	case '.': expon = 1; if(!has) has = 1, val = 0; return 1;
	case '-': has = -1; val = expon = 0; return 1;
	case 'c':
	case 'g': prefix = c; has = 1;  val = expon = 0; return 1;
    }
    if(c >= '0' && c <= '9') {
	if(!has) { has = 1; val = expon = 0; }
	if(expon) expon++;
	val = 10*val + c - '0';
	return 1;
    }
    return 0;
  }
  int tvalue( int defval = 0 ) {
	return has && prefix=='c' ? ~value(defval)
				  :  value(defval);
  }
  int value( int defval = 0 ) { return has ? has*val : defval; }
  float fvalue( float defval = 0 ) {
    if(!has) return defval;
    float v = has * val;
    while(--expon > 0) v *= 0.1;
    return v;
  }
};
 
struct MausSample {
  float x, y;
  float t;
};

class Fl_Gview : public Fl_Gl_Window {

 public:

  Fl_Gview(int x, int y, int h, int w, const char *label = 0);

  int add_drawer( void (*func)( Fl_Gview *, void *obj, void *arg ), void *obj, void *arg, char *name = NULL, int id = -1 );
  int  next_id() const;
  void notifier( void (*func)( Fl_Gview *, void * ), void * );
  int  (*msg)( const char *fmt, ... );
  int  (*eventhook)( Fl_Gview *, int ev );
  void (*predraw)( Fl_Gview *, int again );
  int  (*postdraw)( Fl_Gview *, int again );

  void glmode( int modebits );			/* wrapper for Fl_Gl_Window::mode( ... ) */

  const Matrix *Tc2w() const;
  const Matrix *Tw2c() const;
  void Tc2w( const Matrix * );
  void Tw2c( const Matrix * );
  void lookvec( int row, const Point *vec );
  void reset( int obj = GV_ID_CAMERA );

  void usesubcam( int on ) { use_subc_ = on; }
  int usesubcam() const { return use_subc_; };
  void Tc2subc( Matrix *newT ) { Tc2subc_ = *newT; }
  const Matrix *Tc2subc() const { return &Tc2subc_; }
  void subc_lrbt( float lrbt[4] );
  const float *subc_lrbt() const { return subclrbt_; }

  const Matrix *To2w( int drawerno ) const;	/* returns &Tidentity if invalid */
  int To2w( int drawerno, const Matrix * );	/* returns zero if invalid */
  int objparent( int drawerno ) const;		/* parent object id (0=world, -1=camera) */
  void objparent( int drawerno, int id );		

  int perspective() { return persp_; }
  void perspective(int persp);
  float focallen() const { return focallen_; }
  void focallen( float dist );
  int focalpoint( Point *fpt, float *minfocallen ) const {
      if(fpt) *fpt = focalpoint_;
      if(minfocallen) *minfocallen = minfocallen_;
      return focalpointing_;
  }
  void focalpoint( Point *fpt, float minfocallen, int on ) {
      focalpoint_ = *fpt;
      minfocallen_ = minfocallen;
      focalpointing_ = on;
  }
  void focalpoint( int on ) {
      focalpointing_ = on;
  }
  float halfyfov() const { return halfyfov_; } ;
  void halfyfov( float half_dist_at_focallen );
  float angyfov() const;
  void angyfov( float degrees );
  float nearclip() const { return near_; };
  void nearclip(float newnear);
  float farclip() const { return far_; };
  void farclip(float newfar);
  float aspect() const { return aspect_; }
  void aspect( float newasp );  /* sets window aspect ratio (xsize/ysize) */
  float pixelaspect() const { return pixelaspect_; }
  void pixelaspect( float newasp );
  int stereooffset() const { return stereooff_; }
  void stereooffset( int pixels );

  enum Gv_Stereo stereo() const { return stereo_; }
  void stereo( enum Gv_Stereo setting ) { stereo_ = setting; redraw(); }

  float stereosep() const { return stereosep_; }
  void stereosep( float tanhalfangle ) { stereosep_ = tanhalfangle; redraw(); }

  int inpick() const { return inpick_; } // draw()ers: am I in GL_SELECT mode?
  void picksize( float width, float height );
  void pickbuffer( int words, GLuint *buf );
  void picker( void (*pickcb)(Fl_Gl_Window *, int, int, GLuint *, void *arg), void *arg ); 
  void (*picker( void **argp = 0 ))(Fl_Gl_Window *, int, int, GLuint *, void *arg);
  int pickresults( int *wordsp, GLuint **bufp );  /* returns num hits, too */
  int do_pick( float pickx, float picky ); /* Calls back function given by picker() */

  int snapshot( int x, int y, int w, int h, void *packedrgb );
	// Take snapshot into caller-supplied buffer, w*h*3 bytes long

  const Point *center() const { return &pcenw_; }
  void center( const Point *pcenw );
  int owncoords() { return owncoords_; }
  void owncoords(int useown);


  void nav( enum Gv_Nav navmode );
  enum Gv_Nav nav() const { return nav_; }

  float fps() const;	/* frames per second (smoothed) */
  int framecount;

	/* Do nav ops apply to the target?  !movingtarget() means
	 * that all nav applies to the camera, even r and t.
	 */
  int movingtarget() const { return movingtarget_; }
  void movingtarget(int v) { movingtarget_ = v;  notify(); }

  int inertia() const { return inertia_; }
  void inertia( int on );
  void driftoff( );
  void drifton( );
  void drifton( MausSample *rate );
  int drifting() const { return drifting_; }

  void target( int drawerno ) {
    if(drawerno >= GV_ID_CAMERA && drawerno < ndrawers_
				 && drawerno != target_) {
	target_ = drawerno;
	notify();
    }
  }

  int target() const { return target_; }

  int ndrawers() const { return ndrawers_; }

  float zspeed;		/* for fly, translate modes */

  GNumber num;		/* accumulates numeric keyboard input */

  int retarget() {
	if(num.has) target( num.tvalue() );
	return target();
  }

  char *dname( int drawerno );

  void bgcolor( Point *rgb ) { bgcolor_ = *rgb; redraw(); }
  const Point *bgcolor() const { return &bgcolor_; }

  void initfrom( Fl_Gview * );

  int dspcontext() const { return dspcontext_; }
  void dspcontext( int dspctx );


  virtual void draw();
  virtual int  handle(int ev);

  static void idlepick( void *me );
  static void idledrift( void *me );
  static int next_dspcontext();

 protected:

  static int next_dspcontext_;

  void glprojection( float nearclip, float farclip, const Matrix *postproj );

  Point qc2w_;
  Matrix Tc2w_, Tw2c_; 

  int persp_;
  enum Gv_Stereo stereo_;
  float focallen_, halfyfov_, near_, far_;
  float aspect_, pixelaspect_, stereosep_;
  int stereooff_;

  int dspcontext_;

  float minfocallen_;
  Point focalpoint_;
  int focalpointing_;

  Matrix Tc2subc_;
  float subclrbt_[4];  // subcam frustum tan(left),tan(right),tan(bot),tan(top)
  int use_subc_;

  int target_;
  int movingtarget_;

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

  void (*notify_)( Fl_Gview *, void *);
  void *notifyarg_;
  void notify();
				/* navigation settings */
  enum Gv_Nav nav_;		/*  nav mode */
  int owncoords_;		/*  own coordinates */
  Point pcenw_;			/* center pt (world coords) for orbit/rotate */
  int nullthresh_;		/* null-motion threshold in pixels */
  MausSample driftrate_;
  float drifttime_;		/* wall-clock time at previous drift cycle */
  int inertia_;
  int drifting_;

#define NMAUSAMP 4
  MausSample ms_[NMAUSAMP];
  int nms_;

  /* Nav status -- where current mouse drag started */
  int evx_, evy_, evslow_, evzaxis_, evconstrain_;
  Matrix evTobj2w_, evTc2w_, evTw2c_;

  /* smoothed frame rate */
  float sumwdt_, sumw_;

  struct drawer {
    void (*func)(Fl_Gview *, void *obj, void *arg);
    void *obj, *arg;
    Matrix To2w;
    char *name;
    int id;
    int parent;			/* 0=world, GV_ID_CAMERA=attach-to-camera */
    int objclip;		/* object has custom clip planes? */
    float objnear, objfar;
  };
  int ndrawers_, maxdrawers_;
  struct drawer *drawers_;
  int withid( int id ) const;
  void draw_scene( int needproj, const Matrix *postproj );

  Point bgcolor_;	/* OK, it's not a Point, we just want 3 components */

  void do_nav(int ev, int slow, int zaxis, int constrained);

  void start_nav( int navtarget ); /* Save event-position, camera, target-position */

  void takeMausSample( int ev, float timescale );
  int  pastMaus( MausSample *rate, float ago );
  void rateaccum( float dt );

  void init() {
    msg = printf;
    eventhook = NULL;
    predraw = NULL;
    postdraw = NULL;
    num.has = 0;
    ndrawers_ = maxdrawers_ = 0;
    drawers_ = NULL;
    dspcontext_ = -1;
    owncoords_ = 0;
    persp_ = 1;
    stereo_ = GV_MONO;
    pixelaspect_ = 1.0;
    stereosep_ = .05;
    stereooff_ = 0;
    target_ = GV_ID_CAMERA;
    movingtarget_ = 0;
    focallen_ = 3;  halfyfov_ = .5;	/* 60-degree default FOV */
    near_ = .1;    far_ = 100;

    minfocallen_ = 0.1;
    focalpoint_.x[0] = focalpoint_.x[1] = focalpoint_.x[2] = 0;
    focalpointing_ = 0;

    zspeed = 1;
    inpick_ = 0;
    nms_ = 0;
    pickwidth_ = 2, pickheight_ = 2;
    picknents_ = sizeof(tpickbuf) / sizeof(tpickbuf[0]);
    pickbuf_ = tpickbuf;
    pickarg_ = NULL;
    notify_ = NULL;
    notifyarg_ = NULL;
    pcenw_.x[0] = pcenw_.x[1] = pcenw_.x[2] = 0;
    bgcolor_.x[0] = bgcolor_.x[1] = bgcolor_.x[2] = .2;
    nullthresh_ = 2;
    hasfocus_ = 0;
    use_subc_ = 0;
    Tc2subc_ = Tidentity;
    subclrbt_[0] = subclrbt_[2] = -1;
    subclrbt_[1] = subclrbt_[3] = 1;
    inertia_ = 0;
    drifting_ = 0;
    driftrate_.x = driftrate_.y = driftrate_.t = 0;
    sumwdt_ = sumw_ = 0;
    framecount = 0;
    nav( GV_ORBIT );
    reset( GV_ID_CAMERA );
  }
};
#endif /* C++ */
#endif /*_GVIEW_H*/
