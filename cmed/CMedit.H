#ifndef _GVIEW_H
#define _GVIEW_H
/*
 * Graphical colormap editor
 */

#include <stdlib.h>
#include <stdio.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>

#define CMENTMAX  16384

extern "C" {
 extern void hsb2rgb(float h, float s, float b, float *rp, float *gp, float *bp);
 extern void rgb2hsb(float r, float g, float b,  float *hp, float *sp, float *bp);
}


enum CMfield {
   HUE, SAT, BRIGHT,
   ALPHA,
   RED, GREEN, BLUE
};
  

class CMedit : public Fl_Gl_Window {

 public:

  CMedit(int x, int y, int h, int w, const char *label = 0);

  int cment() const { return cment_; }
  void cment( int newcment );

  int hsbmode;
  float lerpval;
  float vh[CMENTMAX], vs[CMENTMAX], vb[CMENTMAX], alpha[CMENTMAX];
  
  float drag( int index, enum CMfield field, float value, float lerp = 1.0 );
  void dragrange( int x0, int x1, enum CMfield field, float v0, float v1, float lerp = 1.0 );
  float get( int index, enum CMfield field ) const;
  void getrgba( int index, float rgba[4] ) const;
  void gethsba( int index, float hsba[4] ) const;

  int fload( FILE *inf );
  int fsave( FILE *outf );

  void report( int index );
  void report( );
  void reportto( void (*func)( CMedit *cm, int index ) );
  void cmentto( void (*cb)(const CMedit *) ) { cmentcb_ = cb; }

  void snapshot() {		/* for undo-ing */
	for(int k = 0; k < CMENTMAX; k++) {
	    snap[0][k] = vh[k];
	    snap[1][k] = vs[k];
	    snap[2][k] = vb[k];
	    snap[3][k] = alpha[k];
	}
	snapcment_ = cment_;
  }
  int undo() {		/* actually undo/redo */
	float t;
	for(int k = 0; k < CMENTMAX; k++) {
	    t = vh[k];       vh[k] = snap[0][k];  snap[0][k] = t;
	    t = vs[k];       vs[k] = snap[1][k];  snap[1][k] = t;
	    t = vb[k];       vb[k] = snap[2][k];  snap[2][k] = t;
	    t = alpha[k]; alpha[k] = snap[3][k];  snap[3][k] = t;
	}
	int i = cment_; cment_ = snapcment_; snapcment_ = i;
	redraw();
	return 1;
  }

  virtual void draw();
  virtual int  handle(int ev);
  virtual void resize(int nx, int ny, int nw, int nh) {
	w(nw); h(nh); hide(); show();
  }

  void postscale( float );
  void postexpon( float );
  float postscale() const { return postscale_; } 
  float postexpon() const { return postexpon_; }

  float Aout( float Ain ) const;


 protected:

  int lastx_;

  int cment_, snapcment_;
  float snap[4][CMENTMAX];

  float postscale_, postexpon_;

  void (*cmentcb_)( const CMedit * );
  float huenear(float hue, float hueref);
  float hue2y(float hue);
  float y2hue(float y);

  int wx2x( int wx );
  float wy2y( int wy );

  void (*reportfunc)( CMedit *, int index );

  float hueshift, huezoom;
  int remin, remax;	/* repair region */
  int dragfrom;
  int dragamount;
  enum CMfield dragfield;
  float dragval;
  int draghue;
  int locked, lockmin, lockmax;

  char **comments;
  int ncomments, maxcomments;

  void init() {

    cment_ = snapcment_ = 256;
    remin = 0;  remax = cment()-1;

    postscale_ = 1.0;
    postexpon_ = 1.0;

    lerpval = 0.5;
    hsbmode = 1;
    dragfrom = -1;  dragval = 0;  dragamount = 0;
    locked = 0;
    lockmin = 0, lockmax = cment()-1;

    hueshift = 0;
    huezoom = 1;
    draghue = 0;

    ncomments = 0;
    maxcomments = 8;
    comments = (char **)malloc( maxcomments * sizeof(char *) );

    for(int k = 0; k < cment_; k++) {
	vh[k] = 1 - .5*k / cment_;
	vs[k] = .5;
	vb[k] = .25 + .75*k / cment_;
	alpha[k] = .33 + .67 * k*k / (cment_*cment_);
    }
    snapshot();
  }
};
#endif /*_GVIEW_H*/
