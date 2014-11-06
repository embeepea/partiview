#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>


#include <ctype.h>
#undef isspace

#include "CMedit.H"
#include "colorpatch.H"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

CMedit :: CMedit(int x, int y, int w, int h, const char *label)
		: Fl_Gl_Window(x,y,w,h,label) {
  init();
  end();
}

int CMedit::fload( FILE *inf ) {
  char line[256];
  char *cp, *sp;
  int count = -1;
  int i, ix, ox, nix, prevox;
  float rgba[4], hsba[4], phsba[4];
  int lno;
  static enum CMfield flds[4] = { HUE, SAT, BRIGHT, ALPHA };
  int f;
  char tc[2];

  lno = 0;
  this->postscale_ = 1;
  this->postexpon_ = 1;
  ox = 0;
  while(fgets(line, sizeof(line), inf) != NULL) {
    lno++;
    for(cp = line; *cp && isspace(*cp); cp++)
	;
    if(*cp == '\0' || *cp == '\n')
	continue;

    sp = strstr(cp, "#Ascale");
    if(sp != NULL) {
	if(0==strncmp(sp, "#Ascale", 7)) {
	    *sp = '\0';
	    sp += 7;
	    if(*sp == '=' || *sp == ':')
		sp++;
	    sscanf(sp, "%f%f", &this->postscale_, &this->postexpon_);
	    cp = sp;	/* don't record this one as a comment */
	}
    }

    if(*cp == '#') {
	if(ncomments >= maxcomments) {
	    maxcomments *= 2;
	    comments = (char **)realloc( comments, maxcomments * sizeof(char *) );
	}
	comments[ncomments++] = strdup( line );
	continue;
    }

    /* ``nnn:'' entries set the colormap pointer ... */
    if(sscanf(line, "%d%1[:]", &nix, tc) == 2) {
	ix = nix;
	cp = strchr(line, ':') + 1;
	while(*cp && isspace(*cp)) cp++;
	if(*cp == '\0' || *cp == '\n' || *cp == '#')
	    continue;
    }

    if(count == -1) {
	if(!sscanf(line, "%d", &count) || count < 1) {
	    fprintf(stderr, "Not a .cmap file?  Doesn't begin with a number.\n");
	    return 0;
	}
	cment( count );
	ix = 0, prevox = 0;
	continue;
    } else {

	if(ix >= count)
	    break;
	    
	rgba[3] = 1;
	if(sscanf(cp, "%f%f%f%f", &rgba[0],&rgba[1],&rgba[2],&rgba[3]) < 3) {
	    fprintf(stderr, "Couldn't read colormap line %d (cmap entry %d of 0..%d)\n",
		lno, ix, count-1);
	    return 0;
	}
	rgb2hsb( rgba[0],rgba[1],rgba[2], &hsba[0],&hsba[1],&hsba[2] );
	hsba[3] = rgba[3];
	if(ox == 0)
	    memcpy(phsba, hsba, sizeof(phsba));
	else
	    phsba[0] = huenear(phsba[0], hsba[0]);
	ox = (cment_-1) * ix / count + 1;
	for(f = 0; f < 4; f++) {
	    dragrange( prevox, ox, flds[f], phsba[f], hsba[f], 1.0 );
	    phsba[f] = hsba[f];
	}
	prevox = ox;
	ix++;
    }

  }
  if(count <= 0) {
    fprintf(stderr, "Empty colormap file?\n");
    return 0;
  }
  if(ix < count)
    fprintf(stderr, "Only got %d colormap entries, expected %d\n", ix, count);

  for(f = 0; f < 4; f++)
    dragrange( prevox, cment_, flds[f], phsba[f], phsba[f], 1.0 );

  if(postscale_ != 1 || postexpon_ != 1) {
      for(i = 0; i < count; i++)
	alpha[i] = (alpha[i] > 0) ? pow(alpha[i] / postscale_, 1 / postexpon_) : 0;
  }

  return ix > 0;
}

int CMedit::fsave( FILE *outf ) {
  float r,g,b;
  int i, ok = 1;

  fprintf(outf, "%d\n", cment());
  for(i = 0; i < cment_; i++) {
    hsb2rgb( vh[i], vs[i], vb[i], &r, &g, &b );
    if(fprintf(outf, "%f %f %f %f\n", r, g, b, Aout( alpha[i] )) <= 0)
	ok = 0;
  }
  if(postscale_ != 1 || postexpon_ != 1)
    fprintf(outf, "#Ascale %g %g\n", postscale_, postexpon_);
  if(ncomments > 0)
    fputs("\n", outf);
  for(i = 0; i < ncomments; i++)
    fputs(comments[i], outf);
  return ok;
}
    
#define  YMAX  (1.0)
#define  YMIN  (-.25)
#define  YBAR0  (-.01)
#define  YBAR1  (-.06)

#define  XMIN  (-cment_ / 16.f)
#define  XMAX  cment_

int CMedit::wx2x( int wx ) {
  return (int) (XMIN + wx * (XMAX-XMIN) / w());
}

float CMedit::wy2y( int wy ) {
  return (YMAX - wy * (YMAX-YMIN) / h());
}

void CMedit::draw() {
  int i;
  float v;
  int coarse = (w() >= 2*cment());

  if(!valid() || damage()/* == FL_DAMAGE_ALL*/) {
    /* Assume reshaped */
    valid(1);
    remin = 0;  remax = cment()-1;
    glViewport( 0, 0, w(), h() );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( XMIN, XMAX,  YMIN, 1,  -1, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
  }

  if(remax-remin < cment()-1) {
    glEnable( GL_SCISSOR_TEST );
    glScissor( remin*w()/cment(), (remax+1)*w()/cment(), 0, h() );
  } else {
    glDisable( GL_SCISSOR_TEST );
  }

  glClearColor( 0,0,0,0 );
  glClear( GL_COLOR_BUFFER_BIT );

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_LIGHTING );
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_COLOR_MATERIAL );

  glBegin( GL_QUADS );
  glColor3f( 0,0,0 );
  glVertex2f( remin, -.05 );
  glVertex2f( remax+1, -.05 );
  glColor3f( 1,1,1 );
  glVertex2f( remax+1, YMIN );
  glVertex2f( remin, YMIN );
  glEnd();

  glLineWidth( 1 );
  glDisable( GL_BLEND );

  if(hsbmode) {
    glColor3f( 1,1,0 ); /* Hue: yellow */
    float midhue = y2hue(.5);
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, v = hue2y( huenear( vh[i], midhue ) ) );
	if(coarse) glVertex2f( i+1, v );
    }
    glEnd();

    glColor3f( .3,1,.25 );  /* Saturation: teal */
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, vs[i] );
	if(coarse) glVertex2f( i+1, vs[i] );
    }
    glEnd();

    glColor3f( .5,.2,1 );  /* Brightness: purple */
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, vb[i] );
	if(coarse) glVertex2f( i+1, vb[i] );
    }
    glEnd();
  } else {

    float r[CMENTMAX], g[CMENTMAX], b[CMENTMAX];

    for(i = remin; i <= remax; i++)
	hsb2rgb( vh[i], vs[i], vb[i], &r[i], &g[i], &b[i] );

    glColor3f( 1,0,0 ); /* red */
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, r[i] );
	if(coarse) glVertex2f( i+1, r[i] );
    }
    glEnd();

    glColor3f( 0,1,0 );  /* green */
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, g[i] );
	if(coarse) glVertex2f( i+1, g[i] );
    }
    glEnd();

    glColor3f( 0,0,1 );  /* blue */
    glBegin( GL_LINE_STRIP );
    for(i = remin; i <= remax; i++) {
	glVertex2f( i, b[i] );
	if(coarse) glVertex2f( i+1, b[i] );
    }
    glEnd();
  }   

  glColor3f( .7,.7,.7 );  /* alpha: gray */
  glBegin( GL_LINE_STRIP );
  for(i = remin; i <= remax-coarse; i++) {
    glVertex2f( i, alpha[i] );
    if(coarse) glVertex2f( i+1, alpha[i] );
  }
  glEnd();

  glDisable( GL_BLEND );
  glShadeModel( GL_SMOOTH );
  glBegin( GL_QUAD_STRIP );
  for(i = remin; i <= remax; i++) {
    float rgb[3];
    hsb2rgb( vh[i], vs[i], vb[i], &rgb[0],&rgb[1],&rgb[2] );
    glColor3fv( rgb );
    glVertex2f( i, YBAR0 );
    glVertex2f( i, YBAR1 );
    if(coarse) {
	glVertex2f( i+1, YBAR0 );
	glVertex2f( i+1, YBAR1 );
    }
  }
  glEnd();

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glShadeModel( GL_SMOOTH );
  glBegin( GL_QUAD_STRIP );
  for(i = remin; i <= remax; i++) {
    float rgba[4];
    hsb2rgb( vh[i], vs[i], vb[i], &rgba[0],&rgba[1],&rgba[2] );
    rgba[3] = alpha[i];
    glColor4fv( rgba );
    glVertex2f( i, YBAR1 );
    glVertex2f( i, YMIN );
    if(coarse) {
	glVertex2f( i+1, YBAR1 );
	glVertex2f( i+1, YMIN );
    }
  }
  glEnd();
  glDisable( GL_BLEND );

  if(remin <= 0) {
    glBegin( GL_QUAD_STRIP );
    for(i = 0; i < 128; i++) {
	float rgb[3];
	float y = i / 127.;
	hsb2rgb( y2hue(y), 1, 1, &rgb[0],&rgb[1],&rgb[2] );
	glColor3fv( rgb );
	glVertex2f( XMIN, y );
	glVertex2f( XMIN/4, y );
    }
    glEnd();
  }

  glFinish();

  /* draw (I hope) any children lying on top of us */
  if(children()) Fl_Gl_Window::draw();
}


float CMedit::drag( int x, enum CMfield field, float y, float lerp ) {
  float ny, oy, *vp;
  float rgb[3];
  int usergb = 0;
  if(x < 0 || x >= cment())
    return 0;
  if(locked && (x < lockmin || x > lockmax))
    return 0;
  switch(field) {
  case HUE: vp = &vh[x]; break;
  case SAT: vp = &vs[x]; break;
  case BRIGHT: vp = &vb[x]; break;
  case ALPHA: vp = &alpha[x]; break;
  default:
	hsb2rgb( vh[x],vs[x],vb[x], &rgb[0],&rgb[1],&rgb[2] );
	vp = &rgb[ field - RED ];
	usergb = 1;
  }
  if(field == HUE) {
    oy = hue2y(*vp);
    y = hue2y( huenear( y2hue(y), *vp ) );
    ny = (1-lerp) * oy + lerp * y;
    *vp = y2hue(ny);
  } else {
    oy = *vp;
    ny = (1-lerp) * oy + lerp * y;
    if(field != ALPHA) {
	if(ny < 0) ny = 0;
	else if(ny > 1) ny = 1;
    }
    *vp = ny;
    if(usergb)
	rgb2hsb( rgb[0],rgb[1],rgb[2], &vh[x],&vs[x],&vb[x] );
  }
  return ny;
}

void CMedit::dragrange( int x0, int x1, enum CMfield field, float y0, float y1, float lerp ) {
  int x;
  float dy;
  if(x0 == x1  || x0 < 0 && x1 < 0  ||  x0 >= cment() && x1 >= cment())
    return;

  dy = (y1 - y0) / (x1 - x0);
  if(x0 < x1)
    for(x = x0; x < x1; x++)
	drag( x, field, y0 + dy*(x-x0), lerp );
  else
    for(x = x0; x > x1; x--)
	drag( x, field, y0 + dy*(x-x0), lerp );
}


int CMedit::handle(int ev) {
  int x = wx2x( Fl::event_x() );
  float y = wy2y( Fl::event_y() );
  int xmin, xmax;
 
  switch(ev) {

  case FL_SHORTCUT:
    if(Fl::event_key() == 'u') {
	undo();
	return 1;
    }
    return 0;

  case FL_PUSH:
    dragfrom = x, dragval = y, dragamount = 0;
    draghue = ( (x - XMIN) * (x - (XMIN/4)) < 0 );	/* If dragging on hue strip */

    if(Fl::event_state(FL_SHIFT)) {
	dragfield = ALPHA;
    } else {
	// Which button?  Take account of alt/meta modifiers too.
	
	int btn = 1;

	if(Fl::event_state(FL_BUTTON2 | FL_ALT))
	    btn = 2;
	else if(Fl::event_state(FL_BUTTON3 | FL_META))
	    btn = 3;

	static CMfield btn2field[2][3] = {
	    { RED, GREEN, BLUE },
	    { HUE, SAT, BRIGHT }
	};
	dragfield = btn2field[ hsbmode ][ btn-1 ];
    }

    if(Fl::event_key('l')) {
	/*...*/
    } else if(Fl::event_key('r')) {
	/*...*/
    } else {
	snapshot();
    }
    /* Fall into ... */
    
  case FL_DRAG:
  case FL_RELEASE:

#ifdef NOTYET
    if(draghue) {
	float h0 = y2hue( y );
	hueshift += (y - dragval) / huezoom;
	huezoom *= (x - dragfrom)
    } else { ... }
#endif
    dragrange( dragfrom, x, dragfield, dragval, y,
		    Fl::event_state(FL_CTRL) ? 1.0 : lerpval );
    xmin = (dragfrom<x) ? dragfrom : x;
    xmax = dragfrom+x-xmin;
    if(damage() == 0) {
	remin = xmin, remax = xmax;
    } else {
	if(remin > xmin) remin = xmin;
	if(remax < xmax) remax = xmax;
    }
    damage(1);
    if(dragfrom != x)
	dragamount = 1;
    if(ev == FL_RELEASE && dragamount == 0)
	drag( x, dragfield, y, Fl::event_state(FL_CTRL) ? 1.0 : lerpval );

    dragfrom = x, dragval = y;
    report( x );
    return 1;

  case FL_ENTER:
  case FL_LEAVE:
    dragfrom = x;
    report( x );
    return 1;

  case FL_MOVE:
    dragfrom = x;
    report( x );
    return 1;
    
  }
  return 0;
}

float CMedit::huenear( float hue, float hueref ) {
  float h = hue;
  if(h - hueref > .5f)
    do { h -= 1.0f; } while (h - hueref > .5);
  else
    while(h - hueref < -.5f) h += 1.0f;
  return h;
}

float CMedit::hue2y( float hue ) {
  /* Find closest multiple of given hue to reference y-position y0 */
  return (hue - hueshift) * huezoom;
}

float CMedit::y2hue( float y ) {
  return y / huezoom + hueshift;
}

static float sample( float *a, int ents, float at, int smooth ) {
  if(at <= 0 || ents <= 1) return a[0];
  if(at >= 1) return a[ents-1];
  float eat = at * ents;
  int iat = (int)eat;
  return smooth ? a[iat]*(1 - (eat-iat)) + a[iat+1]*(eat-iat)
		: a[iat];
}

void CMedit::postscale( float s ) {
  postscale_ = s;
}
void CMedit::postexpon( float e ) {
  postexpon_ = e;
}

void CMedit::cment( int newcment ) {
  if(newcment < 1) return;
  if(newcment > CMENTMAX) {
    fprintf(stderr, "Oops, can't ask for more than %d colormap entries -- using %d\n",
	CMENTMAX,CMENTMAX);
    newcment = CMENTMAX;
  }
  if(cment_ == newcment)
    return;

  /* Resample */

  snapshot();

  int smooth = 0; // (cment_ < newcment);
  for(int o = 0; o < newcment; o++) {
    float at = newcment > 1 ? (float)o / (newcment-1) : 0;
    vh[o] = sample( &snap[0][0], cment_, at, smooth );
    vs[o] = sample( &snap[1][0], cment_, at, smooth );
    vb[o] = sample( &snap[2][0], cment_, at, smooth );
    alpha[o] = sample( &snap[3][0], cment_, at, smooth );
  }
  cment_ = newcment;
  remin = 0;
  remax = cment_ - 1;
  lockmin = 0;
  lockmax = cment_ - 1;

  if(cmentcb_ != 0)
    (*cmentcb_)( this );
  report( dragfrom );

  redraw();
}

void CMedit::getrgba( int index, float rgba[4] ) const
{
  if(index < 0 || index >= cment()-1) {
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
  } else {
    hsb2rgb( vh[index], vs[index], vb[index], &rgba[0],&rgba[1],&rgba[2] );
    rgba[3] = alpha[index];
  }
}

void CMedit::gethsba( int index, float hsba[4] ) const
{
  if(index < 0 || index >= cment()-1) {
    hsba[0] = hsba[1] = hsba[2] = hsba[3] = 0;
  } else {
    hsba[0] = vh[index];
    hsba[1] = vs[index];
    hsba[2] = vb[index];
    hsba[3] = alpha[index];
  }
}
  

void CMedit::reportto( void (*func)( CMedit *cm, int index ) ) {
  reportfunc = func;
}

void CMedit::report( int index )
{
  if(index < 0 || index >= cment() || reportfunc == NULL) 
    return;
  lastx_ = index;

  (*reportfunc)( this, index );
}

void CMedit::report()
{
  CMedit::report( lastx_ );
}

float CMedit::Aout( float Ain ) const
{
  return Ain > 0 ? postscale_ * pow( Ain, postexpon_ ) : 0;
}

void colorpatch::draw() {
    glClearColor( vr,vg,vb,va );
    glClear( GL_COLOR_BUFFER_BIT );
}
