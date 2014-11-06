/*
 * Syzygy-related functions for szgPartiview, including main program.
 */

static char local_id[] = "$Id: szgPartiview.cc,v 1.18 2010/04/14 21:49:41 slevy Exp $";

/*
 * $Log: szgPartiview.cc,v $
 * Revision 1.18  2010/04/14 21:49:41  slevy
 * Get the head transformation right so that "where" is the inverse of "jump".
 *
 * Revision 1.17  2009/03/28 22:48:47  slevy
 * Add some debug code (matstr()).
 * Re-orthogonalize the matrix in FlyNav::navUpdate().
 * Otherwise it'll blow up within a few dozen iterations.
 *
 * Revision 1.16  2009/03/27 16:30:29  slevy
 * Add first attempt at flyer navigation.
 * New "-fly" config option, and "fly <buttonpov>,<buttoncenter>,<tspeed>,<rspeed>"
 * control command.  I don't know which button is which, so it's configurable on the fly.
 *
 * Revision 1.15  2009/03/27 03:26:51  slevy
 * When "jump"ing, align the target point with the head's position,
 * rather than the CAVE origin.  Use position only -- keep the orientation
 * of the CAVE.
 *
 * This still might not be right.  Might be better to put the
 * target point just behind the head, so an origin crosshair doesn't bug us.
 *
 * Revision 1.14  2009/03/26 20:47:13  slevy
 * Add -plog option to control logging.  If present, each szg process logs all msg()/warn() messages
 * to "parti.NNNN.log" for ProcessID NNNN.
 *
 * Revision 1.13  2009/03/25 07:47:39  slevy
 * ar_log_XXX logs don't seem to go anywhere, so also append to
 * parti.NNNN.log, where NNNN is the SZGClient ProcessID.
 * Maybe this too should be changed -- should we just try to log
 * to one big file, to limit clutter?  Every line is already labelled with [PID] now.
 *
 * Don't use exit(0) in command parser.  Set a shared quit flag instead,
 * and have everybody (?) exit later.
 *
 * <string>.append( C_string, len ) never copies in the string's trailing \000,
 * so add it ourselves.
 *
 * Only create a listenSocket if we're the master -- avoid races to grab the port.
 * I'm not sure when is best to determine master-hood, so out of superstition
 * we do it in the first preExchange callback.
 *
 * Yes, we *do* need to initialize cmdbuf_ to be *empty*.
 *
 * Revision 1.12  2009/03/14 21:32:02  slevy
 * Initialize the cmdbuf transfer-buffer size to 1 byte.
 * szg gets mad if we try to select size 0.
 *
 * Revision 1.11  2008/07/28 09:22:58  slevy
 * Add CMDTEST hook for testing PpCmd packing/unpacking.  Looks like it might work.
 *
 * Revision 1.10  2008/07/28 08:49:48  slevy
 * Scrap pp_ui_postinit().  It just overwrites settings that (a) were initialized
 * properly earlier and (b) might have been changed by command-line I/O.
 * I.e. let the "run" command work if included in a .cf file.
 *
 * Revision 1.9  2008/07/28 08:39:41  slevy
 * onPreExchange: Point sockp at a new bufferedSocket before passing to ar_accept()!
 * Tidy list insert/deletion.
 * Call clock_tick() periodically in master to allow time to pass.
 * onPostExchange: Add more error checking.
 * main: Hack to allow running under Linux+freeglut in standalone mode.
 *
 * Revision 1.8  2008/07/25 16:10:16  slevy
 * Lots of changes from Will Davis: new socket stuff
 * for listening for commands from network;
 * transfer fields; attempts to get "play" to play, etc.
 * Some from Stuart: PpCmd uses string object;
 * passing PpCmd commands by transfer-field
 * from master to slaves.
 *
 * Revision 1.7  2008/07/22 23:20:07  slevy
 * Need to resize() when adding new elements, not just reserve().
 *
 * Revision 1.6  2008/07/22 18:46:14  slevy
 * Use ar_log_<whatever>() for msg() logging.  Send to either _remark or _warning
 * according to whether msg() or warn() was called.
 *
 * Revision 1.5  2008/07/22 17:49:58  slevy
 * From Will Davis: globalize argc/argv for use in callback later.
 * onWindowStartGL() returns void.
 *
 * Implement PpCmd interface for serializing commands for network transport.
 *
 * Revision 1.4  2008/07/15 08:40:17  slevy
 * Don't mention "virtual" on *implementation* methods.
 *
 * Revision 1.3  2008/07/14 17:17:48  slevy
 * Attempt to glue in some arMasterSlaveFramework methods -- maybe enough to draw,
 * though not to process any commands after command-line startup yet.
 *
 */

#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"

#include <stdio.h>
#include <stdlib.h>
#include "config.h"

#ifdef AR_USE_WIN_32
# include "winjunk.h"
#endif

#include "plugins.h"
 
#include "specks.h"
#include "szgPartiview.h"
#include "shmem.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include "partiviewc.h"
#include "findfile.h"


struct Ppszg  ppszg;

static int specks_commandstr( struct stuff **stp, const char *str ) {
  int result;
  if(stp == NULL || *stp == NULL || str == NULL)
    return 0;

#define MAXARGS 128
  char *av[MAXARGS];
  int ac;
  char *txt = (char *)malloc( strlen(str) + 1 );
  char *s = txt;
  strcpy(txt, str);
  for(ac = 0; ac < MAXARGS-1; ac++) {
    av[ac] = strtok(s, " \t\n");
    if(av[ac] == NULL) break;
    s = NULL;
  }
  av[ac] = NULL;
  result = specks_parse_args( stp, ac,av);
  free(txt);
  return result;
}

int specks_commandfmt( struct stuff **stp, const char *fmt, ... ) {
  char cmd[1024], *cp;
  int ok;
  va_list args;
  va_start(args, fmt);
  vsprintf(cmd, fmt, args);
  va_end(args);
  for(cp = cmd; isspace(*cp); cp++)
      ;
  if(*cp == '\0') return 1;	// accept null command implicitly
  ok = specks_commandstr( stp, cmd );
  if(ok) parti_redraw();
  else msg("Unrecognized control command: %s", cmd);
  return ok;
}

/*
 * These two are a quick fix to using argc and argv[] in the
 * onStart() function. I don't believe you can pass parameters
 * into, but I could be wrong.
 *
 */
int globalArgc;
char** globalArgv;

// partiview <-> syzygy navigation.

void copyfrom_arMatrix( Matrix *dstT, const arMatrix4 *srcarT )
{
    int i;
    for(i = 0; i < 16; i++)
	dstT->m[i] = srcarT->v[i];
}

void copyto_arMatrix( arMatrix4 *dstarT, const Matrix *srcT )
{
    int i;
    for(i = 0; i < 16; i++)
	dstarT->v[i] = srcT->m[i];
}
/* =================================================================== */

class FlyNav {
  public:
    bool   flying;	// Flying enabled?  If not, ignore all the below.
    int    bpov;	// While this button# pressed, fly, with point-of-view as rotation center
    int    bcen;	// While this button# pressed, fly, with "center" as rot. center
    float  tspeed;	// translation speed scale factor
    float  rspeed;	// rotation speed scale factor
    int    bstarted;	// has flying started?  If so, on which button?  Else -1.
    Matrix TstartW2C;	// wand-to-cave tfm at time of whichever button press
    Matrix TstartC2W;	// cave-to-wand, ditto

    FlyNav();
    void setPovButton( int b ) { bpov = b; }
    void setCenterButton( int b ) { bcen = b; }
    void navUpdate();
    void config( const char * s );
};

FlyNav::FlyNav() {
    flying = false;
    bpov = 0;
    bcen = 1;
    tspeed = 2;
    rspeed = 2;

    bstarted = -1;
    TstartW2C = Tidentity;	// initial Wand-to-Cave matrix
    TstartC2W = Tidentity;	// initial Cave-to-Wand matrix (inverse of Wand-to-Cave)
}

void FlyNav::config( const char *str ) {
    flying = true;
    if(0 == strcmp(str, "off")) {
	flying = false;
	return;
    } else if(0 == strcmp(str, "on")) {
	flying = true;
	return;
    }
    sscanf( str, "%d%*c%d%*c%f%*c%f%*c", &bpov, &bcen, &tspeed, &rspeed );
}

char *matstr( Matrix *T )
{
    static char s[128];
    sprintf(s, "%g %g %g %g  %g %g %g %g  %g %g %g %g  %g %g %g %g",
	    T->m[0],T->m[1],T->m[2],T->m[3],
	    T->m[4],T->m[5],T->m[6],T->m[7],
	    T->m[8],T->m[9],T->m[10],T->m[11],
	    T->m[12],T->m[13],T->m[14],T->m[15]);
    return s;
}

void FlyNav::navUpdate() {
    if(!flying)
	return;

    arMatrix4 wand2C = ppszg.getMatrix( 1 );	// wand-to-CAVE tfm

    if(this->bstarted < 0) {
	if(this->bpov >= 0 && ppszg.getOnButton( bpov )) {
	    this->bstarted = this->bpov;
	} else if(this->bcen >= 0 && ppszg.getOnButton( this->bcen )) {
	    this->bstarted = this->bcen;
	}
	if(this->bstarted >= 0) {
	    copyfrom_arMatrix( &this->TstartW2C, &wand2C );
	    eucinv( &this->TstartC2W, &this->TstartW2C );
	}

    } else {
	if(ppszg.getButton( this->bstarted )) {
	    // How much has wand moved since button was pressed?

	    arMatrix4 arC2world = ar_getNavMatrix();

	    Matrix TC2world;
	    copyfrom_arMatrix( &TC2world, &arC2world );
	    // TC2world is CAVE-to-world transform

	    float deltat = 0.001 * ppszg.getLastFrameTime();
	    

	    Matrix TnowW2C;	// wand-to-CAVE
	    copyfrom_arMatrix( &TnowW2C, &wand2C );
	    Matrix TdeltaC;
	    mmmul( &TdeltaC,  &this->TstartC2W,&TnowW2C );

	    Point pstartC, pnowC, dpC;
	    vgettranslation( &pstartC, &this->TstartW2C );
	    vgettranslation( &pnowC, &TnowW2C );
	    vsub( &dpC, &pnowC, &pstartC );
	    float mag = vlength( &dpC );

	    vscale( &dpC,  sqrt(mag) * this->tspeed * deltat, &dpC );

	    // now dpC is the incremental translation in CAVE-oriented coords

	    Quat dqC;
	    
	    tfm2quat( &dqC, &TdeltaC );

	    Quat qidentity = { 1, 0, 0, 0 };
	    if(dqC.q[0] < 0)
		qidentity.q[0] = -1;	/* interpolate in same quat hemisphere */

	    float qfrac = this->rspeed * deltat;
	    qcomb( &dqC,  qfrac,&dqC, 1-qfrac,&qidentity );
	    qnorm( &dqC, &dqC );
	    Matrix TincrC;
	    quat2tfm( &TincrC, &dqC );

	    // now TincrC is the incremental rotation in CAVE-oriented coordinates
	    // stuff the translation in too
	    vsettranslation( &TincrC, &dpC );


	    // where is the center-of-rotation? in either CAVE or world coordinates
	    const Point *pcenC, *pcenW;
	    if(this->bstarted == this->bpov) {
		pcenC = &pnowC;		// rotate about the wand
		pcenW = NULL;
	    } else {
		pcenC = NULL;
		pcenW = ppszg.scene->center();
	    }

	    Matrix TnewC2world;
	    mconjugate( &TnewC2world, &TC2world, &TincrC,
                &TC2world, NULL/*world2C auto-computed*/,
                pcenW, pcenC );


	    /* re-orthogonalize */
	    float scaling = vlength( (Point *)&TC2world.m[0] );
	    Point p;
	    Quat q;
	    vgettranslation( &p, &TnewC2world );
	    tfm2quat( &q, &TnewC2world );
	    quat2tfm( &TnewC2world, &q );
	    for(int i = 0; i < 12; i++)
		TnewC2world.m[i] *= scaling;
	    vsettranslation( &TnewC2world, &p );


	    arMatrix4 arnewC2world;
	    copyto_arMatrix( &arnewC2world, &TnewC2world );
	    ar_setNavMatrix( arnewC2world );
	} else {
	    bstarted = -1;
	}
    }
}

// Global instance
FlyNav flyer;


////////////////////////////////////////////////////////////////////////

int pp_read( struct stuff **, int argc, char *argv[], char *fromfname, void * ) {

    /* this module defines only one command at present ... */
    if(!strcmp( argv[0], "subcam")) {
	if(argc >= 9) {
	    parti_make_subcam( argv[1], argc-2, argv+2 );
	} else {
	    msg("%s: subcam: expected name az el rol L R B T, not %s",
		fromfname, rejoinargs( 1, argc, argv ));
	}
	return 1;
    }
    return 0;
}


void parti_lighting() {
    static GLuint lightlist[MAXDSPCTX];

    int ctx = get_dsp_context();	/* this would come from Syzygy wall number, say */
    int listmaking = 0;

    if((unsigned int)ctx < MAXDSPCTX) {
	if(lightlist[ctx] > 0) {
	    glCallList( lightlist[ctx] );
	    return;
	} else {
	    lightlist[ctx] = glGenLists( 1 );
	    glNewList( lightlist[ctx], GL_COMPILE_AND_EXECUTE );
	    listmaking = 1;
	}
    } else {
	listmaking = 0;
    }

    static GLfloat lmambient[] = { 0.1, 0.1, 0.1, 1.0 };
    static GLfloat amblight[] = { 0.1,	0.1, 0.1, 1.0 };
    static GLfloat whitelight[] = { 1, 1, 1, 1 };
    static GLfloat Ce[] = { 0, 0, 0, 1 };
    static GLfloat Ca[] = { 1, 1, 1, 1 };
    static GLfloat Cd[] = { 1, 1, 1, 1 };
    static GLfloat Cs[] = { 1, 1, 1, 1 };
    static GLfloat lightpos[3][4] = {
		0, 0, -1, 0,
		1, 0, -.5, 0,
		0, 1, 0, 0,
    };

    glShadeModel( GL_SMOOTH );

    glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmambient );
    glDisable( GL_LIGHTING );

    int i;

    for(i = 0; i < 8; i++)
	glDisable( GL_LIGHT0+i );

    for(i = 0; i < 3; i++) {
	glLightfv( GL_LIGHT0+i, GL_AMBIENT, amblight );
	glLightfv( GL_LIGHT0+i, GL_SPECULAR, whitelight );
	glLightfv( GL_LIGHT0+i, GL_DIFFUSE, whitelight );
	glLightfv( GL_LIGHT0+i, GL_POSITION, lightpos[i] );
	glEnable( GL_LIGHT0+i );
    }


    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, Ce );
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, Ca );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, Cd );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, Cs );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 32.0 );

    glColorMaterial( GL_FRONT_AND_BACK, GL_DIFFUSE );
    glDisable( GL_COLOR_MATERIAL );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_BLEND );

    if(listmaking)
	glEndList();
}

PvObject::PvObject() : st_(0), name_(0), id_(-1), parent_(0) {
}

PvObject::PvObject( const char *name, int id ) {
    this->init( name, id );
}

void PvObject::init( const char *name, int id ) {
    struct stuff *st = specks_init( 0, NULL );
    st->clk = ppszg.clk;
    this->st_ = st;
    this->To2w( &Tidentity );
    this->name_ = name ? strdup(name) : strdup("");
    this->id_ = id;
    this->parent_ = 0;
}

void PvObject::draw( bool inpick ) {

    if(st_ == NULL)
	return;

    specks_set_timestep( st_ );
    specks_current_frame( st_, st_->sl );

    glPushMatrix();
    glMultMatrixf( & To2w_.m[0] );

    st_->inpick = (int)inpick;
    drawspecks( st_ );
    st_->inpick = 0;

    glPopMatrix();
}



void drawetc( const Point *cen, float censize ) {
  static Point xyz[3] = {{{1,0,0}}, {{0,1,0}}, {{0,0,1}}};
  static Point nxyz[3] = {{{-1,0,0}}, {{0,-1,0}}, {{0,0,-1}}};
  static Point zero = {{0,0,0}};
  static float white[3] = {1,1,1};
  int i;

  // if(ppszg.drawtrace)
  //   (*ppszg.drawtrace)();

  parti_lighting();
  glDisable(GL_LIGHTING);
  if(censize > 0) {
    glPushMatrix();
    glScalef(censize,censize,censize);
    glBegin(GL_LINES);
    for(i = 0; i < 3; i++) {
	glColor3fv(xyz[i].x);
	glVertex3fv(xyz[i].x);
	glVertex3f(0,0,0);
    }
    glEnd();
    glPopMatrix();
  }

  if(censize != 0) {
    censize = fabs(censize);
    glPushMatrix();
    glTranslatef(cen->x[0],cen->x[1],cen->x[2]);
    glScalef(censize, censize, censize);
    glBegin(GL_LINES);
    for(i = 0; i < 3; i++) {
	glColor3fv(white);
	glVertex3fv(nxyz[i].x);
	glVertex3fv(zero.x);

	glColor3fv(xyz[i].x);
	glVertex3fv(zero.x);
	glVertex3fv(xyz[i].x);
    }
    glEnd();
    glPopMatrix();
  }
}

// Draw entire partiview scene.
// XXX How can we handle background color, ppszg.bgcolor?   Do we glClear() here ??
//
// YYY setting "inpick" is part of what's needed to go into OpenGL "selection" mode,
// YYY but we don't need that immediately if at all.
//  Just call draw() and let inpick default to false.
void PvScene::draw( bool inpick ) {

    drawetc( center(), censize() );

    for(int i = 0; i < nobjs(); i++) {
	obj(i)->draw( inpick );
    }
}

PvObject *PvScene::addobj( const char *objname, int id ) {
    if(id < 0)
	id = objs_.size();
    if(objstuff(id) == 0) {
	if(id >= objs_.capacity())
	    objs_.reserve( id + 15 );
	if(id >= objs_.size())
	    objs_.resize( id+1 );
	objs_[id].init( objname, id );
    } else {
	msg("Warning: Reusing object g%d (named %s)", id, objname);
    }
    return &objs_[id];
}


// watching file descriptors for external input, e.g. from network.
// XXX good to implement these someday.
void parti_asyncfd( int fd ) {
    msg("IGNORING parti_asyncfd(%d)", fd);
}
void parti_unasyncfd( int fd ) {
    // XXX
}

typedef enum { REMARK, WARN } MsgLevel;

static MsgLevel msglevel = REMARK;
static int processid = 42;
static int partiLog = 0;

int vmsg( const char *fmt, va_list args ) {

  char str[10240];
#ifdef HAVE_SNPRINTF
  vsnprintf(str, sizeof(str), fmt, args);
#else
  vsprintf(str, fmt, args);
#endif
  // XXX Now do something with str!
  fputs(str, stderr);
  fputc('\n', stderr);
  if(msglevel == REMARK) {
    ar_log_remark() << str;
  } else {
    ar_log_warning() << str;
  }

  if(partiLog) {
      char logfname[128];
      sprintf(logfname, partiLog > 1 ? "parti.%04d.log" : "parti.log", processid);
      FILE *f = fopen(logfname, "a");
      if(f != 0) {
	  fprintf(f, "[%d]: %s\n", processid, str);
	  fclose(f);
      }
  }

  return 0;
}

int msg( const char *fmt, ... ) {
  int val;

  va_list args;

  msglevel = REMARK;

  va_start(args, fmt);
  val = vmsg( fmt, args );
  va_end(args);
  return val;
}

void warn( const char *fmt, ... ) {

  va_list args;

  msglevel = WARN;

  va_start(args, fmt);
  vmsg(fmt, args);
  va_end(args);
}

Point Pstandard_head = { 0, 5.0, 3.5 };


void parti_getc2w( Matrix *c2w ) {
  /*
   * Modified July 23, 2008 William Davis
   * We needed the non-inverted matrix,
   * changed from ar_getNavInvMatrix to
   * ar_getNavMatrix();
   */
  arMatrix4 navmat = ar_getNavMatrix();
  Matrix cave2w;
  copyfrom_arMatrix( &cave2w, &navmat );
  Matrix Thead2cave = Tidentity;
  Thead2cave.m[3*4+0] = Pstandard_head.x[0];
  Thead2cave.m[3*4+1] = Pstandard_head.x[1];
  Thead2cave.m[3*4+2] = Pstandard_head.x[2];
  mmmul( c2w, &Thead2cave, &cave2w );
}

void parti_setc2w( const Matrix *Tc2w ) {
  //msg("IGNORING parti_setc2w() (\"jump\")");
  //XXX handle the "jump" command.  Something like
  /*
   * Modified July 23, 2008 William Davis
   * Sets the navigation matrix to the coordinates
   * specified by the "jump" command
   * Removed eucinv( &w2c, c2w )
   * Modified copyto_arMatrix() to match
   */
  arMatrix4 h2C = ppszg.getMatrix( 0 );	// get head-to-Cave transform
  // Ignore head orientation, just use its position,
  // and take shortcut rather than eucinv for inverse.
  Matrix invTh2C = Tidentity;
#if 0
  invTh2C.m[12] = -h2C.v[12];
  invTh2C.m[13] = -h2C.v[13];
  invTh2C.m[14] = -h2C.v[14];
#else
  invTh2C.m[12] = -Pstandard_head.x[0];
  invTh2C.m[13] = -Pstandard_head.x[1];
  invTh2C.m[14] = -Pstandard_head.x[2];
#endif

  Matrix Thead2world;
  mmmul( &Thead2world, &invTh2C, Tc2w );
  arMatrix4 jumpto;
  copyto_arMatrix( &jumpto, &Thead2world );

  const arHead *hd = ppszg.getHead();
  arVector3 eyeL = hd->getEyePosition( -1 );
  arVector3 eyeR = hd->getEyePosition( +1 );
  arMatrix4 wasat = ar_getNavMatrix();
  ar_setNavMatrix( jumpto );
  arMatrix4 nowat = ar_getNavMatrix();
  arMatrix4 hdmat = ppszg.getMatrix( 0 );

  msg("eyes:  %.3f %.3f %.3f  %.3f %.3f %.3f", eyeL.v[0],eyeL.v[1],eyeL.v[2], eyeR.v[0],eyeR.v[1],eyeR.v[2]);
  msg("head:   %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f",
	  hdmat.v[0],hdmat.v[1],hdmat.v[2],hdmat.v[3],
	  hdmat.v[4],hdmat.v[5],hdmat.v[6],hdmat.v[7],
	  hdmat.v[8],hdmat.v[9],hdmat.v[10],hdmat.v[11],
	  hdmat.v[12],hdmat.v[13],hdmat.v[14],hdmat.v[15]);
  msg("navwas: %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f",
	  wasat.v[0],wasat.v[1],wasat.v[2],wasat.v[3],
	  wasat.v[4],wasat.v[5],wasat.v[6],wasat.v[7],
	  wasat.v[8],wasat.v[9],wasat.v[10],wasat.v[11],
	  wasat.v[12],wasat.v[13],wasat.v[14],wasat.v[15]);
  msg("setc2w: %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f",
	  Tc2w->m[0],Tc2w->m[1],Tc2w->m[2],Tc2w->m[3],
	  Tc2w->m[4],Tc2w->m[5],Tc2w->m[6],Tc2w->m[7],
	  Tc2w->m[8],Tc2w->m[9],Tc2w->m[10],Tc2w->m[11],
	  Tc2w->m[12],Tc2w->m[13],Tc2w->m[14],Tc2w->m[15]);
  msg("navnow: %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f  %.3f %.3f %.3f %.3f",
	  nowat.v[0],nowat.v[1],nowat.v[2],nowat.v[3],
	  nowat.v[4],nowat.v[5],nowat.v[6],nowat.v[7],
	  nowat.v[8],nowat.v[9],nowat.v[10],nowat.v[11],
	  nowat.v[12],nowat.v[13],nowat.v[14],nowat.v[15]);
}

double syzygy_time() {	// called from sclock.c
    return ppszg.getTime() * 0.001;
}

// Camera Animation (from .wf path)
static void playidle( void * ) {
  struct wfpath *path = &ppszg.path;

  if(!ppszg.playing) {
    // XXX disable calling playidle() in this case. // Fl::remove_idle( playidle, NULL );
    ppszg.playidling = 0;
    return;
  }

  double now;
#if TESTCAVE
  now = ppszg.getTime();
#else 
  now = wallclock_time();
#endif
  
  if(ppszg.playspeed == 0)
    ppszg.playspeed = 1;

  int frame = ppszg.framebase;

  if(ppszg.playevery) {
    frame += (int) ((ppszg.playspeed < 0)	/* round away from zero */
		    ? ppszg.playspeed - .999f
		    : ppszg.playspeed + .999f);
    ppszg.framebase = frame;
    ppszg.playtimebase = now;
  } else {
    frame += (int) ((now - ppszg.playtimebase) * ppszg.playspeed * path->fps);
  }

  if(frame < path->frame0 || frame >= path->frame0 + path->nframes) {

    frame = (frame < path->frame0)
	  ? path->frame0
	  : path->frame0 + path->nframes-1;

    switch(ppszg.playloop) {
    case 0:	/* Not looping; just stop at end. */
	// XXX disable calling playidle() here   // Fl::remove_idle( playidle, NULL );
	ppszg.playidling = 0;
	//parti_play( "stop" );
	break;

    case -1:	/* Bounce: stick at same end of range, change direction */
	ppszg.playspeed = -ppszg.playspeed;
	break;

    case 1:	/* Loop: jump to other end of range */
	frame = (frame == path->frame0)
		? path->frame0 + path->nframes-1
		: path->frame0;
    }
    ppszg.framebase = frame;
    ppszg.playtimebase = now;
    
    parti_setframe( frame );
    return;
  }

  /*
   * Cases:
   *  - we're displaying frames slower than real time.
   *	=> use an idle function.
   *  - we're displaying frames faster than real time.
   *    => use a timeout.
   */

  int needidle = ppszg.playidling;
  float dt = 0;

  if(frame != path->curframe) {
    parti_setframe( frame );
    needidle = 1;
  } else {
    frame += (ppszg.playspeed > 0) ? 1 : -1;
    dt = (frame - ppszg.framebase) / (ppszg.playspeed * path->fps)
	+ ppszg.playtimebase - now;
    needidle = (dt < .05);
  }

  if(needidle && !ppszg.playidling) {
    // XXX enable calling playidle() here  // Fl::add_idle( playidle, NULL );
    ppszg.playidling = 1;
  }
  if(!needidle && ppszg.playidling) {
    // XXX disable calling playidle() here // Fl::remove_idle( playidle, NULL );
    ppszg.playidling = 0;
  }
  ppszg.playidling = needidle;

  if(!needidle) {
    // XXX schedule calling playidle once, "dt" seconds from now // Fl::add_timeout( dt, playidle, NULL );
  }
}



void parti_play( const char *rate ) {
  struct wfpath *path = &ppszg.path;
  int playnow = 1;
  int awaitdone = 0;
  msg("Inside the parti_play!");
  msg(rate);
  if(rate != NULL) {
    char *ep;
    float sp = strtod(rate, &ep);
    printf("Rate: %f\n", sp);
    if(strchr(ep, 's')) playnow = 0;
    if(strchr(ep, 'l')) ppszg.playloop = 1;
    else if(strchr(ep, 'k')) ppszg.playloop = -1; /* rock */
    else if(strchr(ep, 'f') || strchr(ep, 'e')) ppszg.playevery = 1;
    else if(strchr(ep, 'r') || strchr(ep, 't')) ppszg.playevery = 0;
    if(strstr(rate, "wait"))
	awaitdone = 1;

    if(ep == rate) {
	/* No leading number -- did we get just "-" or "-?" or "-help"? */
	if(0==strcmp(rate, "-") || strchr(rate, '?') || strchr(rate, 'h')) {
	    msg("Usage: play [rate][s][l|k][f|r][wait]  e.g.  \"play\" or \"play 30\" or \"play 10kf wait\"");
	    msg("   rate  frames/sec; [s]et speed, don't play now; [l]oop/roc[k]");
	    msg("         play every [f]rame/skip to maintain [r]ate;  [wait] until done");
	    return;
	}
	if(sp == 0) playnow = 0;
	else ppszg.playspeed = sp;
    }
  }

  // Disable playidle() idle-function
  // Fl::remove_idle( playidle, NULL );
  // Fl::remove_timeout( playidle, NULL );
  ppszg.playing = 0;
  ppszg.playidling = 0;

  if(playnow) {
#if CAVE
    ppszg.playtimebase = .001 * ppszg.getTime();
#else
    ppszg.playtimebase = wallclock_time();
#endif
    printf("playtimebase: %f", ppszg.playtimebase);
    ppszg.framebase = path->curframe;
    if(ppszg.playspeed > 0 && path->curframe >= path->frame0+path->nframes-1)
	ppszg.framebase = path->frame0;
    else if(ppszg.playspeed < 0 && path->curframe <= path->frame0)
	ppszg.framebase = path->frame0 + path->nframes - 1;

    parti_setframe( ppszg.framebase );

    /* Switch into "play" mode */
    // XXX start calling playidle() idle-function 
    ppszg.playidling = 1;
    ppszg.playing = 1;
  }

}


// Idle function -- enabled when data-animation clock is running
void pp_stepper() {
  if(ppszg.st) {
    clock_tick( ppszg.st->clk );
    specks_set_timestep( ppszg.st );
  }
}

// Data animation
void parti_set_running( struct stuff *st, int on ) {
    if(clock_running(st->clk) != on) {
	if(on) {
	    // XXX enable pp_stepper idle function
	} else {
	    // XXX disable pp_stepper idle function, no longer needed (?)
	}
	if(st->clk) st->clk->walltimed = 1;
	clock_set_running(st->clk, on);
    }
}

void readrc( struct stuff **stp ) {
  char *rc = findfile( NULL, "~/.partiviewrc" );
  if(rc) specks_read( stp, rc );
  rc = findfile( NULL, "./.partiviewrc" );
  if(rc) specks_read( stp, rc );
}

static int cmdargs(int argc, char **argv, int & optind) {
  char *arg = argv[optind];
  if(!strcmp("-c", arg)) {
    arg = argv[optind+1];
    char *t = NewN(char, strlen(arg)+1);
    char *av[128];
    int ac = tokenize(arg, t, 128, av, NULL);
    specks_parse_args( &ppszg.st, ac, av );
    Free(t);
    optind += 2;
    return 1;
  }

  return 0;
}

int pp_parse_args( struct stuff **, int argc, char *argv[], char *fromfname, void * ) {
  if(argc < 1) return 0;

  int i;
  if(!strcmp( argv[0], "exit" )) {
        msg("exiting");
      	// ppszg.stop( 1 );
	ppszg.quitnow_ = 1;

  } else if(!strcmp( argv[0], "fly" )) {
      if(argc > 1)
	  flyer.config( argv[1] );
      msg("fly %s  povbutton %d centerbutton %d tspeed %g rspeed %g  (\"fly %d,%d,%g,%g\"",
	     flyer.flying ? "on" : "off",
	    flyer.bpov, flyer.bcen, flyer.tspeed, flyer.rspeed, 
	    flyer.bpov, flyer.bcen, flyer.tspeed, flyer.rspeed);

  } else if(!strcmp( argv[0], "stereo" )) {
	for(i = 1; i < argc; i++)
	    parti_stereo( argv[i] );

	msg("stereo %s", parti_stereo(NULL));

  } else if(!strcmp( argv[0], "winsize" )) {
	msg("winsize %s", parti_winsize( rejoinargs( 1, argc, argv ) ) );

  } else if(!strncmp( argv[0], "snapset", 7 ) || !strcmp( argv[0], "snapshot" )) {
	char *frameno = NULL, *basename = NULL;
	char *size = NULL;
	int now = (0 == strcmp(argv[0], "snapshot"));
	while(argc > 2) {
	    if(!strcmp(argv[1], "-w")) {
		size = argv[2];
	    } else if(!strcmp(argv[1], "-n")) {
		frameno = argv[2];
	    } else
		break;
	    argc -= 2, argv += 2;
	}
	if(argc > 1)
	    basename = rejoinargs( 1, argc, argv );
	parti_snapset( basename, frameno, size );
	if(now) {
	    char snapinfo[1024]; 
	    if(parti_snapshot(snapinfo) >= 0)
		msg("Snapped %s", snapinfo);
	}

  } else if(!strcmp( argv[0], "clip" )) {
	msg("%s", parti_clip( argv[1], argc>2?argv[2]:NULL ) );

  } else if(!strcmp( argv[0], "pickrange" )) {
	float pickrange = parti_pickrange( argv[1] );
	msg("pickrange %g", parti_pickrange( NULL ));

  } else if(!strcmp( argv[0], "fov" ) || !strcmp( argv[0], "fovy" )) {
	float fovy = parti_fovy( (argc>1) ? argv[1] : NULL );
	msg("fovy %g", fovy);

  } else if(!strcmp( argv[0], "subcam")) {
	int index;
	if(argc > 1) {
	    if(!strcmp(argv[1], "-") || !strcmp(argv[1], "off")) {
		parti_select_subcam(0);
	    } else {
		index = parti_make_subcam( argv[1], argc-2, argv+2 );
		if(index > 0) {
		    parti_select_subcam(index);
		} else {
		    char *what = parti_subcam_list();
		    msg(what[0]=='\0' ?  "No subcams defined yet" :
					"Known subcams: %s", what);
		}
	    }
	}
	index = parti_current_subcam();
	if(index > 0) {
	    char params[100];
	    char *name = parti_get_subcam( index, params );
	    msg("subcam %s  %s # az el rol  L R B T", name, params);
	} else {
	    int wx, wy;
	    float hx, hy = .5 * parti_fovy(NULL);
	    sscanf( parti_winsize(NULL), "%d%*c%d", &wx, &wy );
	    if(wy == 0) wy = 1;
	    hx = atan( tan(hy*(M_PI/180))*wx / wy ) * (180/M_PI);
	    msg("subcam off  0 0 0  %g %g  %g %g # a e r   L R B T (default)",
			hx, hx, hy, hy);
	}

  } else if(!strncmp( argv[0], "focallen", 8 )) {
	float focallen = parti_focal( (argc>1) ? argv[1] : NULL );
	msg("focallength %g", focallen);
	
  } else if(!strcmp( argv[0], "focalpoint" )) {
        /* args:
	 * on/off
	 * x y z [minfocallen]
	 */
        Point fpt;
	float minlen;
	int ison = parti_focalpoint( argc-1,  (argv+1), &fpt, &minlen );

	msg(ison ? "focalpoint %g %g %g  %g # pointxyz minfocallen"
		 : "focalpoint off # %g %g %g  %g # pointxyz minfocallen",
		 fpt.x[0],fpt.x[1],fpt.x[2], minlen);


  } else if(!strncmp( argv[0], "jump", 4 )) {
    Point xyz;
    float aer[3];
    static int stupid[3] = {1,0,2};	/* aer -> rx ry rz */
    Matrix c2w;
    parti_getc2w( &c2w );
    tfm2xyzaer( &xyz, aer, &c2w );
    if(argc>1) {
      for(i=1; i<argc && i<4+3; i++) {
	if(i-1<3) xyz.x[i-1] = getfloat(argv[i], xyz.x[i-1]);
	else aer[stupid[i-4]] = getfloat(argv[i], aer[stupid[i-4]]);
      }
      xyzaer2tfm( &c2w, &xyz, aer );
      parti_setc2w( &c2w );
    }
    msg("jump %g %g %g  %g %g %g  (XYZ RxRyRz)",
	xyz.x[0],xyz.x[1],xyz.x[2],
	aer[1],aer[0],aer[2]);
   
  } else if(!strncmp( argv[0], "home", 4 )) {//marx version 0.7.03
    Point xyz;
    float aer[3];
    static int stupid[3] = {1,0,2};	/* aer -> rx ry rz */
    Matrix c2w;
    parti_getc2w( &c2w );
    tfm2xyzaer( &xyz, aer, &c2w );
    if(argc>1) {
      for(i=1; i<argc && i<4+3; i++) {
	if(i-1<3) xyz.x[i-1] = getfloat(argv[i], xyz.x[i-1]);
	else aer[stupid[i-4]] = getfloat(argv[i], aer[stupid[i-4]]);
      }
      xyzaer2tfm( &c2w, &xyz, aer );
      parti_setc2w( &c2w );
      ppszg.home[0] = xyz.x[0]; ppszg.home[1] = xyz.x[1];  ppszg.home[2]  = xyz.x[2];

    //ppszg.home[3] = aer[0];   ppszg.home[4] = aer[1];    ppszg.home[5]  = aer[2]; bug in releases >= 0.7.03 
      ppszg.home[3] = aer[1];   ppszg.home[4] = aer[0];    ppszg.home[5]  = aer[2]; //version 0.7.05 fix for the line above

    }
    msg("home %g %g %g  %g %g %g  (XYZ RxRyRz)", ppszg.home[0], ppszg.home[1], ppszg.home[2], ppszg.home[3], ppszg.home[4], ppszg.home[5]);  

  } else if((!strcmp( argv[0], "rdata" ) || !strcmp(argv[0], "readpath"))
		&& argc>1) {
	bool freet = false;
	char *tfname = argv[1];
	char *realfile = findfile( fromfname, tfname );
	if(realfile == NULL) {
	    tfname = (char *)malloc(strlen(argv[1]) + 32);
	    freet = true;
	    sprintf(tfname, "data/record/%s%s", argv[1],
		strstr(argv[1], ".wf") ? "" : ".wf");
	    realfile = findfile( NULL, tfname+12 );
	}
	if(realfile == NULL)
	    realfile = findfile( fromfname, tfname );
	if(realfile)
	    parti_readpath( realfile );
	else
	    msg("%s: can't find \"%s\" nor data/record/... nor ...wf",
		argv[0],argv[1]);
	if(freet)
	    free(tfname);

  } else if(!strcmp( argv[0], "play" )) {
	parti_play( argc>1 ? rejoinargs(1, argc, argv) : NULL );

  } else if(!strcmp( argv[0], "frame" )) {
	CONST struct wfpath *pp;
	i = parti_frame( argv[1], &pp );
	msg("frame %d (of %d..%d)", pp->curframe,
		pp->frame0, pp->nframes + pp->frame0 - 1);

  } else if(!strcmp( argv[0], "interest") || !strcmp( argv[0], "int" ) ||
		!strcmp( argv[0], "center" ) ||
		!strcmp( argv[0], "cen" )) {
	Point cen;
	float censize = parti_getcensize();
	parti_getcenter( &cen );
	if(argc > 1) {
	    sscanf(argv[1], "%f%*c%f%*c%f%*c%f",
		&cen.x[0],&cen.x[1],&cen.x[2], &censize);
	    if(argc > 3)
		for(i=0;i<3;i++)
		    cen.x[i] = getfloat(argv[i+1], cen.x[i]);
	    if(argc == 3 || argc == 5)
		sscanf(argv[argc-1], "%f", &censize);
	    parti_center( &cen );
	    if(censize != parti_getcensize())
		parti_censize( censize );
	}
	msg("center %g %g %g  %g(radius)", cen.x[0],cen.x[1],cen.x[2], censize);

  } else if(!strcmp( argv[0], "censize" )) {
	if(argc>1)
	    parti_censize( getfloat(argv[1], parti_getcensize()));
	msg("censize %g  (interest-marker size)", parti_getcensize());

  } else {
	return 0;
  }

  return 1;
}

void pp_ui_postinit() {
  parti_set_timebase( ppszg.st, 0.0 );
  parti_set_timestep( ppszg.st, 0.0 );
  parti_set_running( ppszg.st, 0 );
  parti_set_fwd( ppszg.st, 1 );
}

void clearBuffer(char* buf, int bufsize)
{
  for(int i=0; i < bufsize; i++)
  {
    buf[i] = '\0';
  }
}

PpCmd::PpCmd( CmdType type, int argc, char **argv )
{
    this->type = type;
    this->argc = argc;
    this->argv = argv;
}

PpCmd::~PpCmd() {
}

int PpCmd::frozenLen() const {
    int len = 2*argc+2;
    for(int i = 0; i < argc; i++)
	len += strlen(argv[i]);
    return len;
}

/* Serialize the command, and append result to 'buf' */
int PpCmd::freezeInto( string &buf ) const { 
    int ineed = buf.size() + frozenLen() + 1;

    if(buf.capacity() < ineed)
	buf.reserve( 2*ineed+1 );

    for(int i = 0; i < argc; i++) {
	buf.append( 1, (char)type );
	buf.append( argv[i], strlen(argv[i]) );
	buf.append( 1, '\000' );
    }
    buf.append( 1, '\000' );	// mark end of last arg
    return buf.size();
}

int PpCmd::thawFrom( char *buf, int &offset, bool copystrings ) {

    if(buf == 0)
	return 0;

    int p = offset;

    argc = 0;
    argv = 0;
    type = (CmdType) buf[p];
    switch(type) {
    case CMD_DATA:
    case CMD_CONTROL:
	break;		// OK.

    case CMD_NONE:
	offset = p+1;
	return -1;	// EOF

    default:
	msg("PpCmd::thawFrom(): Trouble decoding %p at offset %d (got 0x%x not 0/1/2)", buf, p, buf[p]);
	type = CMD_NONE;
	return -1;
    }

    // count args
    do {
	p++;
	p += strlen( &buf[p] ) + 1;
	argc++;
    } while(buf[p] != 0);
    argv = new char *[argc+1];
    // msg("Found %d args in thawing %d bytes from offset %d", argc, p-offset, offset);
    // copy args
    p = offset;
    for(int a = 0; a < argc; a++) {
	p++;
	argv[a] = copystrings ? strdup(&buf[p]) : &buf[p];
	p += strlen(&buf[p]) + 1;
    }
    argv[argc] = 0;
    offset = p+1;
    return argc;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

Ppszg::~Ppszg() {
    delete scene;
    // delete clk;  Don't delete the clock -- might be shared with other objects
}


Ppszg::Ppszg() {
    scene = new PvScene();
    st = NULL;
    clipnear_ = 0.1;
    clipfar_ = 2500;
    focallen_ = 3;
    quitnow_ = 0;

    snapfmt = NULL;
    snapfno = 0;
    pickrange = 3.5;


    memset(home, 0, sizeof(home));

    clk = new SClock();
    clock_init( clk );
    clock_set_speed( clk, 1.0 );
    clk->continuous = 1;
    clk->walltimed = 1;

    memset( &path, 0, sizeof(path) );
    playing = 0;
    playevery = 0;
    playidling = 0;
    playspeed = 20;
    framebase = 1;
    playtimebase = 0;
    playloop = 0;

    timebasetime = 0;

    subcam = 0;

    listenSocket = 0;
    /* List of connected arSockets */
    socketList = NULL;
}


bool Ppszg::onStart( arSZGClient& SZGClient ) {
  /*
   * Add Transfer Fields to Framework so data is
   * accessible and syncronized to all clients
   * Added July 23, 2008 William Davis
   */
  processid = SZGClient.getProcessID();	// for msg() log files

  addTransferField("curtime", &(ppszg.clk->curtime), AR_DOUBLE, 1);
  addTransferField("running", &(ppszg.clk->running), AR_INT, 1);
  addTransferField("continuous", &(ppszg.clk->continuous), AR_INT, 1);
  addTransferField("center_", &(ppszg.scene->center_), AR_FLOAT, 3);
  addTransferField("censize_", &(ppszg.scene->censize_), AR_FLOAT, 1);
  addTransferField("quitnow", &(ppszg.quitnow_), AR_INT, 1);
  addInternalTransferField( "commands", AR_CHAR, 1 );

  parti_bgcolor( "0 0 0" );

  parti_add_commands( pp_parse_args, "partiview", NULL );
  parti_add_reader( pp_read, "partiview", NULL );
  plugin_init();


  // pp_ui_init();
  // ppui.view->pickbuffer( COUNT(pickbuffer), pickbuffer );

  parti_clip( "0.1", "2500" );
  parti_censize( 1.0 );
  // parti_pickrange( 3.5 );

  parti_object( "g1", NULL, 1 );
  readrc( &ppszg.st );

  for(int i = 1; i < globalArgc; i++) {
    if(0==strcmp(globalArgv[i], "-c") && i+1 < globalArgc) {
	i++;
	int len = strlen(globalArgv[i]);
	int maxargs = len/2 + 2;    // max possible number of args
	char **av = new char *[maxargs];
	char tbuf[ 2*len + 1024 ];
	int ac = tokenize( globalArgv[i], tbuf, maxargs, av, NULL );
	if(! specks_parse_args( &ppszg.st, ac, av )) {
	    msg("-c: Unrecognized control command: %s", globalArgv[i]);
	}
	delete av;
    } else if(0 == strcmp(globalArgv[i], "-fly")) {
	flyer.config( globalArgv[++i] );
    } else if(0 == strcmp(globalArgv[i], "-plog")) {
	partiLog++;
    } else {
	specks_read( &ppszg.st, globalArgv[i] );
    }
  }

  // pp_ui_postinit();

  return true;
}

#if 0
bool arMasterSlaveFramework::addInternalTransferField( std::string fieldName,
                                                         arDataType dataType,
                                                         int numElements );

The pointer argument is omitted, and the numElements argument now denotes the initial size. The size can be changed by calling:

  bool arMasterSlaveFramework::setInternalTransferFieldSize( std::string fieldName,
                                                             arDataType dataType,
                                                             int newSize );
#endif


void Ppszg::onWindowStartGL( arGUIWindowInfo * ) {
  set_dsp_context( 0 );	/* assign a display context, so texture code can track window changes */
}

void Ppszg::onPreExchange( void ) {	// called on master only
  // do navigation...
  // process commands read from network...
  //   and package any with
  /* Here we want to do the listening on the sockets to interpret
   * any commands we might receive over them
   */
  static int socketid = 0;
  if(ppszg.listenSocket == 0) {
    // Create listener socket.  Do this only in a *live* master
    /* arSockets - originally from salimiman by Jim Crowell */
    msg("master on pid %d listening on port 8675", processid);
    listenSocket = new arSocket(AR_LISTENING_SOCKET);
    listenSocket->ar_create();
    listenSocket->setSendBufferSize(2048);
    listenSocket->setReceiveBufferSize(2048);
    listenSocket->reuseAddress(true);
    list<string> acceptMask;
    listenSocket->setAcceptMask(acceptMask);
    /* Bind to port 8675 */
    listenSocket->ar_bind(NULL, 8675);
    listenSocket->ar_listen(256);
  }

  if(ppszg.listenSocket->readable())
  {
    bufferedSocket* sockp = new bufferedSocket();
    arSocketAddress addr;
    ppszg.listenSocket->ar_accept(sockp, &addr);
    msg("process %d accepted connection", processid);

    sockp->setID(socketid++);
    sockp->next = ppszg.socketList;
    ppszg.socketList = sockp;
  }

  /* "fly" navigation if selected */
  flyer.navUpdate();

  cmdbuf_.clear();		// wipe master->slaves buffer for this cycle

  /* Now read data into the buffer */
  bufferedSocket* sock = ppszg.socketList;
  bufferedSocket** previousSocketp = &ppszg.socketList;
  while( sock != NULL ) {
	int len = sock->poll();
	while(len >= 0) {		// repeat for all the lines in buffer

            int maxargs = len/2 + 2;    // max possible number of args
            char **av = new char *[maxargs];
            char tbuf[ 2*len + 1024 ];
		// temp buffer.  Extra room allows for $expansion etc.
		// XXX should pass this length to tokenize!

            int ac = tokenize( sock->str(), tbuf, maxargs, av, NULL );

            if(ac > 0) {
		// Not a comment nor a blank line.  Try to parse it locally.
		if( ! specks_parse_args( &ppszg.st, ac, av ) ) {
		    msg("Unrecognized control command: %s", sock->str());
		}
		// In any case, pass it on to slaves
		PpCmd cmd( CMD_CONTROL, ac, av );
		cmd.freezeInto( cmdbuf_ );
            }
	    delete av;

            len = sock->consume();      // Done processing that line.  Got another on hand?
	}
	if(sock->expired()) {
	   *previousSocketp = sock->next;
	   delete sock;
	} else {
	    previousSocketp = &sock->next;
	}
	sock = *previousSocketp;
  }

  // Now ship bundled commands to slaves

  if(cmdbuf_.size() == 0)
      cmdbuf_.append( 1, '\000' );	// ensure nonzero size

  setInternalTransferFieldSize( "commands", AR_CHAR, cmdbuf_.size() );

  if(cmdbuf_.size() > 1 && getenv("CMDTEST")) {
	PpCmd tcmd;
	int offset = 0;
	int room = cmdbuf_.size();
	    msg("Got %d-bytes of command packet", room-1);	// XXX DEBUG
	while(offset+1 < room) {
	    int oof = offset;
	    if( tcmd.thawFrom( &cmdbuf_[0], offset, false ) < 0)
		break;
	    // false -> build argc/argv which point into cmdp string directly
	    msg("Offset %d..%d: command [%d] %s", oof,offset, tcmd.argc, rejoinargs( 0, tcmd.argc, tcmd.argv ));
            if(tcmd.argc <= 0 || oof == offset) {
                msg("Had enough: tcmd.argc=%d, offset %d, room %d", tcmd.argc, offset, room);
                break;
            }
        }
  }

  int room = 0;
  char *cmdp = (char *)getTransferField( "commands", AR_CHAR, room );
  if(cmdp == 0 || room != cmdbuf_.size()) {
    msg("panic: transfer field size %d != the size %d we told it to be!\n",
		room, cmdbuf_.size());
  } else {
       memcpy( cmdp, &cmdbuf_[0], room );
  }

  /*
   * Added July 23, 2008 William Davis
   * Forces the framework to listen to updates to navigation matrix
   * changed from a "jump" command
   */ 
  navUpdate();

  clock_tick( ppszg.clk );		// let time advance
}

void Ppszg::onPostExchange( void )	// called on master + slaves
{
    if(! this->getMaster() ) {
	// We're a slave.  Decode any commands passed by master this cycle.
	int room = 0;
	char *cmdp = (char *)getTransferField( "commands", AR_CHAR, room );
	if(cmdp == 0) {
	    static int once = 1;
	    if(once) msg("Ppszg::onPostExchange: can't get transfer field \"commands\"");
	    once = 0;
	    return;
	}

#if 0
	if(room > 1) {
	    msg("postExchange: GOT %d-bytes of cmd pkt", room-1);
	    char unk[512];
	    int u,uu;
	    for(u=uu=0; u<room; u++)
		uu += sprintf(&unk[uu], cmdp[u]<' ' ? "\\x%02x" : "%c", cmdp[u]);
	    unk[uu] = '\0';
	    msg("Received: \"%s\"", unk); // XXX debug
	}
#endif

	int offset = 0;
	PpCmd pp;

	while(offset+1 < room) {
            int oof = offset;
	    bool done;
	    done = (pp.thawFrom( cmdp, offset, false ) < 0);
	    // false -> build argc/argv which point into cmdp string directly
            if(done || oof >= offset || pp.argc <= 0) {
              msg("Had enough of cmd pkt: done %d offset %d was %d, room %d, argc %d, type 0x%x", done, offset, oof, room, pp.argc, pp.type);
              break;
            }
	    msg("postExchange: processing '%s'", rejoinargs(0,pp.argc,pp.argv));
	    if(pp.type == CMD_CONTROL) {
		if(! specks_parse_args( &ppszg.st, pp.argc, pp.argv ) ) {
		    warn("Unrecognized control command on slave: %s",
			rejoinargs( 0, pp.argc, pp.argv ));
		}
	    } else {
		warn("can only handle control cmds, not: %s",
		    rejoinargs(0, pp.argc, pp.argv));
	    }
	}
    }

    /* terminate if we're asked to... */
    /* is this the right way to do this? */
    if(ppszg.quitnow_)
	exit(0);
}

void Ppszg::onWindowInit( void ) {	// clear to background...
    glClearColor( bgcolor.x[0], bgcolor.x[1], bgcolor.x[2], 1 );
    ar_defaultWindowInitCallback();
}

void Ppszg::onDraw( arGraphicsWindow& win, arViewport& vp ) {
    /*
     * Added July 23, 2008 William Davis
     * Loads the navigation matrix into the framework, making
     * sure we listen to the changes enacted by the "jump" command
     * in parti_setc2w()
     */
    loadNavMatrix();

    scene->draw();
}

void Ppszg::onDisconnectDraw( void ) {
    glClearColor( 1, .1, .1, 1 );
    glClear( GL_COLOR_BUFFER_BIT );
}

void Ppszg::onCleanup( void ) {
}

void Ppszg::onUserMessage( const string& messageBody ) {
    // called in master on "dex <N> user <some message for this program>"
    // parse messageBody, add to command queue for next preExchange time
}

#ifdef __linux__
#include <GL/glut.h>	// hack: Syzygy uses glutWireSphere() in simulator mode, but freeglut insists on glutInit() first
#endif

int main(int argc, char *argv[])
{

#ifdef __linux__
    int targc = 1;		// rest of hack.
    glutInit( &targc, argv );
#endif

    globalArgc = argc; //Make these two variables accessible to the
    globalArgv = argv; // onStart callback

    if( ! ppszg.init( argc, argv ) ) {
	fprintf(stderr, "arMasterSlaveFramework::init() failed, giving up...\n");
	exit(1);
    }

    if( ! ppszg.start() ) {
	fprintf(stderr, "arMasterSlaveFramework::start() failed, giving up...\n");
	exit(1);
    }
    // Shouldn't reach here.
    fprintf(stderr, "Shouldn't reach here.\n");
    exit(2);
}
