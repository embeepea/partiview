/*
 * Main program for FLTK-based partiview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"

#ifdef WIN32
# include "winjunk.h"
#endif
 
#include "specks.h"
#include "Gview.H"
#include "partiview.H"
#include "shmem.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include "glshader.h"

#include "partiviewc.h"
#include "findfile.h"

#ifdef FLHACK
# include "flhack.H"
#else
# include <FL/Fl.H>
# include <FL/fl_draw.H>
# include "genericslider.H"  //steven marx: generic slider introduced version 0.7.02
# include <FL/Fl_Tooltip.H>  //steven marx: version 0.7.02 now using fltk-1.1.2

# if HAVE_OLD_FILE_CHOOSER_H
#  include <FL/fl_file_chooser.H>	/* fltk 1.0.* and 1.1.rcX for X<6 */
# else
#  include <FL/Fl_File_Chooser.H>	/* fltk 1.1.0rc6 and later */
# endif
#include <FL/glut.H>		// for GLUT_MULTISAMPLE if fltk knows it
#endif /* !FLHACK */

static char local_id[] = "$Id: partiview.cc,v 1.70 2013/11/30 21:31:44 slevy Exp $";

struct _ppui  ppui;

#ifdef EMBEDHACK
# include "pview_funcs.h"
#endif

extern "C" { extern void plugin_init(); }

struct stuff *stuffs[MAXSTUFF];

static int specks_commandstr( struct stuff **stp, const char *str ) {
  if(stp == NULL || *stp == NULL || str == NULL)
    return 0;

#define MAXARGS 128
  char *av[MAXARGS];
  int ac;
  char *txt = (char *)alloca( strlen(str) + 1 );
  char *s = txt;
  strcpy(txt, str);
  for(ac = 0; ac < MAXARGS-1; ac++) {
    av[ac] = strtok(s, " \t\n");
    if(av[ac] == NULL) break;
    s = NULL;
  }
  av[ac] = NULL;
  return specks_parse_args( stp, ac,av);
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


/* =================================================================== */

void pp_clk_init() {
  ppui.clk = NewN( SClock, 1 );
  clock_init( ppui.clk );
  clock_set_speed( ppui.clk, 1.0 );
  ppui.clk->continuous = 1;	// is this used?
  ppui.clk->walltimed = 1;

}

void pp_ui_postinit() {
  parti_set_timebase( ppui.st, 0.0 );
  parti_set_timestep( ppui.st, 0.0 );
  parti_set_running( ppui.st, 0 );
  parti_set_fwd( ppui.st, 1 );

  ppui.stepspeed->logstyle( FL_LOG_LINEAR );
  ppui.stepspeed->truebounds( 1.0e-6, 10.0 );
  ppui.stepspeed->truevalue( clock_speed( ppui.clk ) );
  clock_set_step( ppui.clk, 0.1 * clock_speed( ppui.clk ) );

  ppui.steprow->hide();


  if(ppui.detached) {
    char code[2] = { ppui.detached, '\0' };
    parti_detachview(code);
  }

}


#ifdef FLHACK

void ppui_refresh()
{ }

#else /* !FLHACK */

void pp_nav_init(Fl_Menu_Button *);
int  pp_viewevent_cb( Fl_Gview *, int ev );

int gensliderchg = 0; //steven marx: generic slider introduced version 0.7.02

void pp_ui_init() {

  pp_nav_init(ppui.nav);

  pp_sldtype_init(ppui.sldtype); //steven marx: version 0.7.02
  genericslider_initparam();     //steven marx: version 0.7.02

  /* ppui.cmd->handle_cb = pp_cmd_handle_cb;  No, this doesn't work yet */

  ppui.view->eventhook = pp_viewevent_cb;

  parti_inertia( 1 );

  if(getenv("CMDFONT"))
    ppui.cmdhist->scaletext( atoi( getenv("CMDFONT") ) );
  ppui.freewin = NULL;
  ppui.boundwin = ppui.view;
  ppui.detached = 0;

  ppui.jpegqual = 97;

#ifdef DEFAULT_ELUMENS
  ppui.detached = 'f';
#endif
}

void pp_cmd_cb( HistInput* inp, void * ) {
  if(inp->hist()) {
    inp->hist()->addline( inp->value(), 0 );
    if(ppui.cmd)
	ppui.cmd->redraw();
  }
  const char *cp;
  for(cp = inp->value(); isspace(*cp); cp++)
      ;
  if(*cp == '\0')
      return;	// accept null command implicitly

  int ok = specks_commandstr( &ppui.st, inp->value() );
  if(!ok)
    msg("Unrecognized control command: %s", inp->value());
  ppui_refresh( ppui.st );
  parti_redraw();        
  genericslider_setparam(); //steven marx version 0.7.02 update generic slider
}


static enum Gv_Nav codes[] = { GV_FLY, GV_ORBIT, GV_ROTATE, GV_TRANSLATE };
static const char *navtitles[] =   {"[f]ly","[o]rbit","[r]ot","[t]ran"};

void pp_nav_init(Fl_Menu_Button *m) {
  m->add("[f]ly|[o]rbit|[r]ot|[t]ran");

  for(int i = m->size(); --i >= 0; ) 
    (((Fl_Menu_Item *)(m->menu()))[i]).labelcolor( m->labelcolor() );
}

void pp_nav_cb(Fl_Menu_Button* m, struct stuff **vst) {
  int v = m->value();
  if(v >= 0 && v < COUNT(codes))
    ppui.view->nav( codes[v] );
}   

void pp_obj_cb(Fl_Menu_Button* m, void *) {
  parti_object( m->text(), &ppui.st, 0 );
}

void pp_objtog_cb(Fl_Button* b, void *) {
  int objno = 1;
  sscanf(b->label(), "g%d", &objno);
  if(objno<0||objno>=MAXSTUFF||stuffs[objno]==NULL)
    return;

  //================ marx added version 0.7.04 to support 1 button mac mouse ======================
#ifndef __APPLE__
  if(Fl::event_button() == FL_RIGHT_MOUSE) {
#else
  if( (Fl::event_button() == FL_RIGHT_MOUSE) ||  ((Fl::event_button() == FL_LEFT_MOUSE) && Fl::event_alt())  ) {
#endif
 //================  end 1 button mac mouse =======================================================

    stuffs[objno]->useme = 1;
    parti_object( b->label(), &ppui.st, 0 );
    b->value(1);
  } else {
    stuffs[objno]->useme = !stuffs[objno]->useme;
  }
  ppui_refresh( ppui.st );
  parti_redraw();
}

void pp_genericslider_cb(Fl_Value_Slider* sl, void*) { //steven marx: version 0.7.02
  gensliderchg = 1;
  genericslider_setparam();
  gensliderchg = 0;
}

void pp_slum_cb(Fl_Value_Slider* sl, struct stuff ** ) {
  struct stuff *st = ppui.st;
  int cd = st->curdata, by = st->sizedby;
  if((unsigned int)cd >= MAXFILES) cd = 0;
  if((unsigned int)by >= MAXVAL+1) by = 0;
  struct valdesc *vd = &st->vdesc[cd][by];
  vd->lum = pow(10., sl->value());
  ppui.view->redraw();
}

void pp_step_cb( Fl_Button * , void *stepsign ) {
  double sign = reinterpret_cast<long>(stepsign);
  parti_set_running( ppui.st, 0 );
  switch(Fl::event_button()) {
  case FL_RIGHT_MOUSE: sign *= 10; break;
  case FL_MIDDLE_MOUSE: sign *= 0.1; break;
  }
  clock_step( ppui.st->clk, sign );
  specks_set_timestep( ppui.st );
  parti_redraw();
}

void pp_run_cb( Fl_Button *runbtn, void *stepsign ) {
  clock_set_fwd( ppui.st->clk, reinterpret_cast<long>(stepsign) );
  parti_set_running( ppui.st, runbtn->value() );
}

void pp_timeinput_cb( Fl_Float_Input *inp, void * ) {
    double v;

    parti_set_running( ppui.st, 0 );
    if(sscanf(inp->value(), "%lf", &v)) {
	clock_set_time( ppui.st->clk, v + ppui.timebasetime );
	parti_set_timestep( ppui.st, v + ppui.timebasetime );
	parti_redraw();
    }
}

void pp_timebaseinput_cb( Fl_Float_Input *inp, void * ) {
    double newbase;

    parti_set_running( ppui.st, 0 );
    if(sscanf(inp->value(), "%lf", &newbase)) {
	parti_set_timebase( ppui.st, newbase );
	ppui.timebasetime = newbase;
	parti_redraw();
    }
}

void pp_jog_cb( Fl_Roller *rol, void * ) {
    static double lastrol;
    double v = rol->value();
    clock_step( ppui.st->clk, v - lastrol );
    lastrol = v;

    parti_set_running( ppui.st, 0 );
    specks_set_timestep( ppui.st );
    parti_set_timestep( ppui.st, clock_time( ppui.st->clk ) );
    parti_redraw();
}

void pp_feed_cb( Fl_Light_Button *, void * ) {
    msg("can't feed yet...");
}

void pp_settrip_cb( Fl_Button *, void * ) {
    char str[32];
    ppui.timebasetime = clock_time( ppui.st->clk );
    ppui.timestep->value( "0" );
    sprintf(str, "%.16lg", ppui.timebasetime);
    ppui.timebase->value(str);
}

void pp_backtrip_cb( Fl_Button *, void * ) {
    clock_set_time( ppui.st->clk, ppui.timebasetime );
    ppui.timestep->value( "0" );
    parti_redraw();
}

void pp_stepspeed_cb( Fl_Log_Slider *spd, void * ) {
    double speed = spd->truevalue();
    clock_set_speed( ppui.st->clk, speed );
    clock_set_step( ppui.st->clk, 0.1 * speed );
}

void pp_rdata_cb( Fl_Button *, struct stuff ** ) {
  const char *result;
  result = fl_file_chooser("Choose virdir .wf path", "*.wf", NULL);
  if (result)
    parti_readpath( result );
}

void pp_playframe_cb( Fl_Counter *counter, struct stuff ** ) {
  if(ppui.playing)
    parti_play( "stop" );
  parti_setframe( (int)counter->value() );
}

void pp_playtime_cb( Fl_Value_Slider *slider, struct stuff ** ) {
  struct wfpath *path = &ppui.path;
  if(ppui.playing)
    parti_play( "stop" );
  parti_setframe( (int) (slider->value() * path->fps + path->frame0) );
}

void pp_play_cb( Fl_Button *play, struct stuff ** ) {
  if(Fl::event_state(FL_BUTTON3)) {
    ppui.playmenu->popup();
  } else {
    parti_play( 
	play->value() ? NULL /*play at default speed*/ 
		      : "stop"
	);
  }
}

static const char *viewhelp[] = { 
    "[?] this help",
    "<Tab> focus on command-input box",
    "[f]ly [o]rbit [r]ot [t]ranslate - left-mouse-drag motion modes",
    "Ctrl-left-drag: alternate (\"pan\" in orbit mode; axis-constrained in rot/trans)",
    "Middle-drag (or Option-left): forward/back (orbit/fly/trans), rotate in screen plane (rot)",
    "Right-click: pick object   Shift-right-click: pick object, set new center",
    "[p] pick object under cursor  [P] pick object and set new center",
    "[v] zoom out    [V] zoom in (optional fovy prefix, e.g. \"42.5v\")",
    "[{] animate backward  [}] animate forward [~] toggle fwd/back",
    "[z] 2x slower   [Z] 2x faster",
    "[<] step backward [>] step forward (optional delta-time prefix)",
    "[s] stereo on/off (optional stereosep prefix, e.g. \"-.02s\")",
    "[ctrl-s] toggle left/right eye of stereo view",
    "[ctrl-t] report frame rate",
    "[PrtSc] take image snapshot",
};

int pp_viewevent_cb( Fl_Gview *view, int ev ) {
  char snapinfo[1024];
  int fno;
  double bump;
  struct stuff *st = ppui.st;

#define CTRL(x) ((x) & 0x1F)

  if(ev == FL_KEYBOARD || ev == FL_SHORTCUT) {

    if(ppui.cmdhist && ppui.cmdhist->handle_nav(ev))
	return 1;

    switch(Fl::event_key()) {
    case FL_Print:
	if(view->num.has) {
	    ppui.snapfno = view->num.value();
	    view->num.has = 0;
	}
	fno = parti_snapshot(snapinfo);
	if(fno >= 0)
	    msg("Snapped %s", snapinfo);
	return 1;
    }
    int key = Fl::event_text()[0];
    switch(key) {

    case '?':
	for(int i = 0; i < COUNT(viewhelp); i++)
	    msg(" %s", viewhelp[i]);
	return 1;

    case '\t':
	if(!ppui.cmdhist || !ppui.cmdhist->input())
	    return 0;
	ppui.cmdhist->input()->take_focus();
	return 1;

    case 'S':
	if(view->num.has) {
	    float v = view->num.fvalue();
	    if(v == 0) {
		parti_stereo("off");
	    } else {
		if(fabs(v) < 1)		/* plausible? */
		    view->stereosep( v );
		parti_stereo("on");
	    }
	    view->num.has = 0;
	} else {
	    parti_stereo( view->stereo()==GV_MONO ? "on" : "off" );
	}
	msg("stereo %s (focallen %g)", parti_stereo(NULL), view->focallen());
	return 1;

    case CTRL('S'):
	if(strstr(parti_stereo(NULL),"left"))
	    parti_stereo("right");
	else
	    parti_stereo("left");
	msg("stereo %s", parti_stereo(NULL));
	return 1;

    case '>': bump = 1; goto step;
    case '<': bump = -1;
     step:
	if(view->num.has)
	    clock_set_step( st->clk, view->num.fvalue() );
	view->num.has = 0;
	clock_step( st->clk, bump );
	parti_set_running( st, 0 );
	/* clock_notify( st->clk );  NOTYET NOTIFY */
	parti_redraw();
	return 1;

    case '{':
    case '}':
	clock_set_fwd( st->clk, key=='}' );
	parti_set_running( st, 1 );
	return 1;

    case '~':
	if(clock_running(st->clk)) {
	    parti_set_running( st, 0 );
	} else {
	    int fwd = -clock_fwd( st->clk );
	    clock_set_fwd( st->clk, fwd );
	    parti_set_fwd( st, fwd );
	    parti_set_running( st, 1 );
	}
	return 1;

    case 'z':
    case 'Z':
	if(view->num.has) {
	    specks_set_speed( st, view->num.fvalue() );
	    view->num.has = 0;
	} else {
	    specks_set_speed( st,
			clock_speed(st->clk) * (key=='z' ? .5 : 2) );
	}
	clock_set_step( st->clk, 0.1 * clock_speed(st->clk) );
	return 1;
    
    case CTRL('K'):
	if(view->num.has) {
	    ppui.cmdhist->scaletext( view->num.fvalue() );
	    view->num.has = 0;
	}
	return 1;

    case CTRL('T'):
	msg("%.4g fps", ppui.view->fps());
	return 1;
    }
  } else if(ev == FL_SHOW) {
    ppui.view->redraw();	// XXX slevy DEBUG 2013.07.09
  }

  return 0;
}

static void fitlabel( Fl_Widget *w, int excess = 4 ) {
#ifdef __APPLE__
  int width = (int)fl_width( w->label(), strlen(w->label()) ) + excess; //marx added the strlen param
  w->hide();
  w->size( width, w->h() );
  w->show();
#else
  int ofont = fl_font(), osize = fl_size();
  fl_font( w->labelfont(), w->labelsize() );
  int width = (int)fl_width( w->label()) + excess; //marx added the strlen param
  w->hide();
  w->size( width, w->h() );
  w->show();
  fl_font(ofont, osize);
#endif
}

void ppui_refresh( struct stuff *st ) {

  if(ppui.view == NULL) return;

  /* Possibly switch selected object */
  int targ = ppui.view->target();

  if(targ >= 0 && targ < MAXSTUFF && stuffs[targ] != NULL && stuffs[targ] != ppui.st) {
    st = ppui.st = stuffs[targ];
  }

  enum Gv_Nav nav = ppui.view->nav();
  for(int navcode = 0; navcode < COUNT(codes); navcode++) {
    if(codes[navcode] == nav) {
	ppui.nav->value( navcode );
	if(strcmp(ppui.nav->label(), navtitles[navcode])) {
	    ppui.nav->label( navtitles[navcode] );
	    ppui.toprow->parent()->redraw();
	}
    }
  }

  if(ppui.view->inertia()) ppui.inertiaon->set();
  else ppui.inertiaon->clear();

  int tno, maxobject = 0;
  int timeshown = ppui.steprow->visible();
  for(tno = 0; tno < MAXSTUFF; tno++) {
    struct stuff *tst = stuffs[tno];
    if(tst == NULL) continue;

    maxobject = tno;
    if(!timeshown && tst->clk->tmin != tst->clk->tmax) {
	ppui.steprow->show();
	ppui.steprow->parent()->hide();	// force top-level Pack to be redraw()n
	ppui.steprow->parent()->show(); // for some reason just ->redraw() doesn't.
	ppui.mainwin->redraw();		// ... try harder.
	Fl::wait(.01);			// delay ... to handle events?
	ppui.cmdhist->redraw();		// ... and even harder.  This shouldn't be necessary, but trying to evade MacOS trouble.
	ppui.cmd->redraw();
	timeshown = 1;
    }

    if(ppui.st == stuffs[tno]) {
	char oname[12];
	Fl_Font lfont = ppui.view->movingtarget()
					? FL_HELVETICA_BOLD_ITALIC
					: FL_HELVETICA;
	sprintf(oname, "[g%d]", tno);

	if(strcmp(ppui.obj->label(), oname) || lfont != ppui.obj->labelfont()) {
	    ppui.obj->label( strdup( oname ) );
		// Iff we might actually move the target object,
		// display its [gN] title in BOLD ITALICs.
	    ppui.obj->labelfont( lfont );
	    ppui.obj->parent()->redraw();
	}
    }
  }

  if(st == NULL)
    return;

  if(ppui.objgroup->active() != st->useme) {
    if(st->useme) ppui.objgroup->activate();
    else ppui.objgroup->deactivate();
  }

  // objtogs is wrapped in a Group which is invisible by default.
  if(!ppui.objtogs->parent()->visible() && maxobject > 1) {
    ppui.objtogs->parent()->show();
    ppui.steprow->parent()->hide(); // force top-level Pack to be redraw()n
    ppui.steprow->parent()->show(); // for some reason just ->redraw() doesn't.
    ppui.mainwin->redraw();
  }

  if(ppui.objtogs->visible()) {
    Fl_Button *b, *b0 = (Fl_Button *)ppui.objtogs->child(0);
    for(tno = 0; tno <= maxobject; tno++) {
	if(tno >= ppui.objtogs->children()) {
	    char name[8];
	    sprintf(name, "g%d", tno);
	    b = new Fl_Button( b0->x(),b0->y(),b0->w(),b0->h(), strdup(name) );
	    b->labelcolor( b0->labelcolor() );
	    b->labelfont( b0->labelfont() );
	    b->labelsize( b0->labelsize() );
	    b->color( b0->color() );
	    b->selection_color( b0->selection_color() );
	    b->down_box( b0->down_box() );
	    b->type( b0->type() );
	    b->callback( (Fl_Callback*)pp_objtog_cb );
	    b->when(FL_WHEN_RELEASE_ALWAYS); //marx: release 0.7.06   - allows data group buttons callback to respond when inflight 
	    ppui.objtogs->add( b );
	}
	b = (Fl_Button *)ppui.objtogs->child(tno);
	
	if(stuffs[tno] == NULL) {
	    if(b->visible()) b->hide();
	} else {
	    if((b->value() != 0) != stuffs[tno]->useme)
		b->value( stuffs[tno]->useme );
	    if(!b->visible()) b->show();
	}
    }
  }
    

  ppui.point->value( st->usepoint );
  ppui.poly->value( st->usepoly );
  ppui.label->value( st->usetext );
  ppui.texture->value( st->usetextures );
  ppui.box->value( st->useboxes!=0 );
  int boxcolor = ( st->useboxes==2 ? 1/*red*/ : 2/*green*/ );
  if(boxcolor != ppui.box->color2()) {
    ppui.box->color2( boxcolor );
    ppui.box->redraw();
  }


  int cd = st->curdata, by = st->sizedby;
  if((unsigned int)cd >= MAXFILES) cd = 0;
  if((unsigned int)by >= MAXVAL+1) by = 0;
  struct valdesc *vd = &st->vdesc[cd][by];
  float lum = vd->lum;
  if(lum <= 0) lum = 1;
  ppui.slum->value( log10( lum ) );
  char slumlabel[50], *ep = slumlabel;
  if(parti_get_alias(st)[0]) {
    ep = slumlabel + sprintf(slumlabel, "[%.17s] ", parti_get_alias(st));
  }
  if(by == CONSTVAL) {
    sprintf( ep, "logslum %g", vd->lmin );
  } else if(st->vdesc[cd][by].name[0]) {
    sprintf( ep, "logslum %.10s", vd->name );
  } else {
    sprintf( ep, "logslum(field%d)", by );
  }
  if(strcmp(ppui.slum->label(), slumlabel)) {
    static char slumlab[50];
    strcpy(slumlab, slumlabel);
    ppui.slum->label(slumlab);
    ppui.slum->parent()->redraw();

    //steve marx version 0.7.02 support for generic slider. provides update to generic slider label upon data group change
    int menunum = ppui.sldtype->value();
    if(menunum >= 0 && menunum < NUMSLD && menunum != SLUM)
      genericslider_setparam();
  }
}


void pp_viewchanged( Fl_Gview *gview, void *st ) {
  ppui_refresh( ppui.st );
#if MATT_VIRDIR
  vd_ffn();
#endif
}

void pp_hrdiag_on_cb( Fl_Menu_ *menu, void * ) {
  const Fl_Menu_Item *item = menu->mvalue();
  if(ppui.hrdiag == NULL || ppui.hrdiagwin == NULL) return;
  parti_hrdiag_on( item->value() );
}

void pp_inertia_on_cb( Fl_Menu_ *menu, void * ) {
  const Fl_Menu_Item *item = menu->mvalue();
  ppui.view->inertia( item->value() );
  msg("inertia %s", ppui.view->inertia() ? "on" : "off");
}
#endif /*!FLHACK*/

int pp_read( struct stuff **, int argc, char *argv[], char *fromfname, void * ) {
    if(!strcmp( argv[0], "subcam")) {
	if(argc == 3 && 0==strcmp(argv[1],"-tilt")) {
	    ppui.sctilt = atof(argv[2]);
	    return 1;
	}
	if(argc >= 9) {
	    parti_make_subcam( argv[1], argc-2, argv+2 );
	} else {
	    msg("subcam: expected <name> <az> <el> <rol> <L> <R> <B> <T>, or -tilt <degrees>.  What's \"%s\"",
		rejoinargs( 1, argc, argv ));
	}
	return 1;
    }
    return 0;
}


static int whichobjname( const char *str ) {
  if(str == NULL) return -1;
  if(str[0] == '[') str++;
  int objno = -1;
  sscanf(str, "g%d", &objno);
  return objno;
}

void parti_set_alias( struct stuff *st, CONST char *alia ) {

  if(st == NULL) return;

  if(alia == NULL) alia = "";
  char *alias = strdup(alia);
  if(st->alias) free(st->alias);
  st->alias = alias;

#ifndef FLHACK
  int slotno, menuno, btno;
  for(slotno = 0; slotno < MAXSTUFF; slotno++) {
    if(stuffs[slotno] == st) {
	char lbl[32];
	sprintf(lbl, "[g%d]%.24s", slotno, alias);
	for(menuno = ppui.obj->size()-1; --menuno >= 0; ) {
	    if(whichobjname( ppui.obj->text(menuno) ) == slotno) {
		ppui.obj->replace( menuno, strdup(lbl) );
		break;
	    }
	}

	sprintf(lbl, alias[0]=='\0' ? "g%d" : "g%d=%.24s", slotno, alias);

	for(btno = ppui.objtogs->children(); --btno >= 0; ) {
	    Fl_Button *btn = (Fl_Button *)(ppui.objtogs->child(btno));
	    if(btn && whichobjname( btn->label() ) == slotno) {
		btn->label( strdup(lbl) );
		fitlabel( btn, 8 );
		break;
	    }
	}
    }
  }
#endif /*!FLHACK*/
}

int vmsg( const char *fmt, va_list args ) {

  char str[10240];

#ifdef HAVE_SNPRINTF
  vsnprintf(str, sizeof(str), fmt, args);
#else
  vsprintf(str, fmt, args);
#endif


#ifndef FLHACK
  if(gensliderchg) //steven marx: version 0.7.02 - we don't want voluminous generic slider output
    return 0;
  if(ppui.cmdhist) {
    ppui.cmdhist->addline( str, 1 );
    ppui.cmdhist->redraw();
    ppui.cmd->redraw();
  }

  return printf("%s\n", str);

#elif defined(EMBEDHACK)
  _pviewmsg = _pviewmsg + str + "\n";
  return 1;
#else
  return 1;	/* FLHACK but not EMBEDHACK */
#endif /*FLHACK*/
}

int msg( const char *fmt, ... ) {
  int val;

  va_list args;
  va_start(args, fmt);
  val = vmsg( fmt, args );
  va_end(args);
  return val;
}

void warn( const char *fmt, ... ) {

  va_list args;
  static char avoid[] = "X_ChangeProperty: "; /* ignore spurious X errors */
  if(0==strncmp(fmt, avoid, sizeof(avoid)-1))
     return;

  va_start(args, fmt);
  vmsg(fmt, args);
  va_end(args);
}

void parti_lighting() {
    static GLuint lightlist = 0;

    if(lightlist > 0) {
	glCallList( lightlist );
	return;
    }
    lightlist = glGenLists( 1 );
    glNewList( lightlist, GL_COMPILE_AND_EXECUTE );

    static GLfloat lmambient[] = { 0.1, 0.1, 0.1, 1.0 };
    static GLfloat amblight[] = { 0.1,	0.1, 0.1, 1.0 };
    static GLfloat whitelight[] = { 1, 1, 1, 1 };
    static GLfloat Ce[] = { 0, 0, 0, 1 };
    static GLfloat Ca[] = { 1, 1, 1, 1 };
    static GLfloat Cd[] = { 1, 1, 1, 1 };
    static GLfloat Cs[] = { 1, 1, 1, 1 };
    static GLfloat lightpos[3][4] = {
		{ 0, 0, -1, 0 },
		{ 1, 0, -.5, 0 },
		{ 0, 1, 0, 0 },
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

    glEndList();
}

void drawjunk(Fl_Gview *view, void *obj, void *arg) {
  static Point xyz[3] = {{1,0,0}, {0,1,0}, {0,0,1}};
  static Point nxyz[3] = {{-1,0,0}, {0,-1,0}, {0,0,-1}};
  static Point zero = {0,0,0};
  static float white[3] = {1,1,1};
  float censize = parti_getcensize();
  int i;

  if(ppui.drawtrace)
      (*ppui.drawtrace)();

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
    const Point *cen = view->center();
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

void readrc( struct stuff **stp ) {
  char *rc = findfile( NULL, "~/.partiviewrc" );
  if(rc) specks_read( stp, rc );
  rc = findfile( NULL, "./.partiviewrc" );
  if(rc) specks_read( stp, rc );
}

static char **latecmds = 0;
static int nlatecmds = 0;

static int cmdargs(int argc, char **argv, int & optind) {
  char *arg = argv[optind];

  if(!strcmp("-C", arg)) {
    // commands to be run after startup, once window has opened
    if(latecmds == 0)
	latecmds = new char *[argc];
    latecmds[nlatecmds++] = argv[optind+1];
    optind += 2;
    return 1;
  }
	
  if(!strcmp("-c", arg)) {
    arg = argv[optind+1];
    char *t = NewA(char, strlen(arg)+1);
    char *av[128];
    int ac = tokenize(arg, t, 128, av, NULL);
    specks_parse_args( &ppui.st, ac, av );
    optind += 2;
    return 1;
  }

  if(!strcmp("-hideui", arg)) {
    ppui.detached = 'h';
    optind++;
    return 1;
  }
  if(!strcmp("-detach", arg)) {
    ppui.detached = 'd';
    optind++;
    return 1;
  }

  if(!strcmp("-geom", arg) || !strcmp("-geometry", arg)) {
    ppui.reqwinsize = NULL;	/* override any initial "winsize" */
    			/* but let fltk parse it anyway */
  }
  return 0;
}

int pp_parse_args( struct stuff **, int argc, char *argv[], char *fromfname, void * ) {
  if(argc < 1) return 0;

  int i;
  if(!strcmp( argv[0], "exit" )) {
      	exit(0);

  } else if(!strcmp( argv[0], "stereo" )) {
	for(i = 1; i < argc; i++)
	    parti_stereo( argv[i] );

	msg("stereo %s", parti_stereo(NULL));

  } else if(!strcmp( argv[0], "pixelaspect" )) {
	if(argc > 1) {
	    float pixasp = atof(argv[1]);
	    if(pixasp != 0)
		parti_setpixelaspect( pixasp );
	}
	msg("pixelaspect %g", parti_getpixelaspect());

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
	    } else if(!strncmp(argv[1], "-q", 2)) {
		sscanf(argv[2], "%d", &ppui.jpegqual);
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

  } else if(!strcmp( argv[0], "ortho" )) {
#ifdef NOTYET
	int ortho = parti_ortho( argc>1 ? argv[1] : NULL);
	float fov = parti_fov(NULL);
	msg(ortho ? "ortho on (fov %g units)" : "ortho off (fov %g degrees)");
#endif

  } else if(!strcmp( argv[0], "pickrange" )) {
	float pickrange = parti_pickrange( argv[1] );
	msg("pickrange %g", pickrange);

  } else if(!strcmp( argv[0], "fov" ) || !strcmp( argv[0], "fovy" )) {
	float fovy = parti_fovy( (argc>1) ? argv[1] : NULL );
	msg("fovy %g", fovy);

  } else if(!strcmp( argv[0], "subcam")) {
	int index;
	if(argc > 1) {
	    if(!strcmp(argv[1], "-") || !strcmp(argv[1], "off")) {
		parti_select_subcam(0);
	    } else if(0==strcmp(argv[1],"-tilt")) {
		if(argc > 2)
		    sscanf(argv[2], "%f", &ppui.sctilt);
		msg("subcam -tilt %g", ppui.sctilt);
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
	    const char *name = parti_get_subcam( index, params );
	    msg("subcam %.30s  %.40s # az el rol  L R B T // subcam -tilt %g", name, params, ppui.sctilt);
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
      ppui.home[0] = xyz.x[0]; ppui.home[1] = xyz.x[1];  ppui.home[2]  = xyz.x[2];

    //ppui.home[3] = aer[0];   ppui.home[4] = aer[1];    ppui.home[5]  = aer[2]; bug in releases >= 0.7.03 
      ppui.home[3] = aer[1];   ppui.home[4] = aer[0];    ppui.home[5]  = aer[2]; //version 0.7.05 fix for the line above

    }
    msg("home %g %g %g  %g %g %g  (XYZ RxRyRz)", ppui.home[0], ppui.home[1], ppui.home[2], ppui.home[3], ppui.home[4], ppui.home[5]);  

  } else if(!strncmp(argv[0], "inertia", 5)) {
	if(argc > 1)
	    ppui.view->inertia( getbool( argv[1], ppui.view->inertia() ) );
	msg("inertia %s", ppui.view->inertia() ? "on":"off");

  } else if((!strcmp( argv[0], "rdata" ) || !strcmp(argv[0], "readpath"))
		&& argc>1) {
	char *tfname = argv[1];
	char *realfile = findfile( fromfname, tfname );
	if(realfile == NULL) {
	    tfname = (char *)alloca(strlen(argv[1]) + 32);
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

#ifndef FLHACK
  } else if(!strcmp( argv[0], "detach" )) {  /* "detach" or "detach f" or "detach f +50+100" */
	parti_detachview( rejoinargs( (argc>1 ? 1 : 0), argc, argv ));
	msg("detached");
#endif /*FLHACK*/

  } else {
	return 0;
  }

  return 1;
}

int main(int argc, char *argv[])
{
  static GLuint pickbuffer[20480];
  extern char partiview_version[];

  Fl::warning = warn;

  pp_clk_init();
  make_window();

  static Point black = {0,0,0};
  ppui.view->bgcolor( &black );
  ppui.view->dspcontext( 0 );	/* assign a display context, so texture code can track window changes */

  char title[64];
  sprintf(title, "partiview %.10s", partiview_version);
  ppui.mainwin->label( title );

  /* make_window() sets ppui.view, etc. */

  parti_add_commands( pp_parse_args, "partiview", NULL );
  parti_add_reader( pp_read, "partiview", NULL );
  plugin_init();
  pp_ui_init();
  ppui.view->add_drawer( drawjunk, NULL, NULL, NULL, 0 );
  ppui.view->pickbuffer( COUNT(pickbuffer), pickbuffer );

  ppui.view->zspeed = 5;
  ppui.view->farclip( 2500 );
  ppui.censize = 1.0;
  ppui.pickrange = 3.5;

  ppui.view->movingtarget( 0 );
  ppui.view->msg = msg;

  if(ppui.hrdiag) {
    ppui.hrdiag->msg = msg;
    ppui.hrdiag->bgcolor( &black );
  }

  ppui.playspeed = 1;
  ppui.playframe->lstep(10);

  initShaderStuff( false );		// prepare to load shader programs (later)

  parti_object( "g1", NULL, 1 );
  readrc( &ppui.st );

  int i = 0;
  if(Fl::args(argc, argv, i, cmdargs) == 0) {
    fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
    exit(1);
  }
  for( ; i < argc; i++) {
    specks_read( &ppui.st, argv[i] );
  }

  pp_ui_postinit();

  ppui.view->notifier( pp_viewchanged, ppui.st );
  ppui_refresh( ppui.st );

  if(ppui.detached == 'h')
      ppui.mainwin->hide();
  else
      ppui.mainwin->show(argc, argv);

#ifdef GLUT_MULTISAMPLE			// if multisampling known to FLTK
  parti_stereo( parti_stereo(NULL) );	// side effect: enables multisampling
#endif

  ppui.view->show();

  ppui.view->make_current();		// to allow shader stuff to work
  initShaderStuff( true );
   
  //marx version 0.7.03 process any pending events followed by simulation user pressing enter key
  //this appears to properly initialize the steprow group
  /*  
  for(i = 1; i < 11; i++) //11 is arbitrary probably only need 5
     Fl::wait(.1);
  pp_cmd_cb( ppui.cmd, NULL );
  */

  //replaces the above in release because the above seemed to stop working in 0.7.06
  //in effect i am replacing a software emulation of pressing the enter key with software simulation of a mouse click
  int cnt = 0; 
  while(double v = Fl::wait(.1)){
    if(v < 0)      //error
      break;
    if(cnt++ > 5) //because in windows wait(time) will not return 0 if time > 0
      break;
  }
  ppui_refresh(NULL);
  //end of replaces the above ...

#if 0 /* was: #ifdef __APPLE__ */
  Fl_Window* mw = ppui.mainwin;
  Fl::wait(.1);
  ppui.mainwin->resize(mw->x()+1, mw->y()+1, mw->w()+1, mw->h()+1); //marx: version 0.7.04
  //the resize compensates for bug that appears under os x only - damage to widgets does not cause redraw but resize seems to cause the needed redraw
  // this bug appears to be fixed (by fltk 1.1.10, probably earlier),
  // and doing this causes the main window to be non-resizable,
  // so let's toss it.  Thanks to Jonathan Strawn <jonnyflash@gmail.com>,
  // UNM ARTS Lab, for figuring this out. -slevy
#endif

  if(ppui.reqwinsize != NULL) {
    parti_update();
    parti_winsize( ppui.reqwinsize );
    ppui.reqwinsize = NULL;
  }

  Fl::visible_focus(0);            //marx: version 0.7.02 - keep the pre fltk-1.1.x old style of only text widgets to get keyboard focus

  Fl_Tooltip::delay(.5);               //marx: version 0.7.02
  Fl_Tooltip::font(FL_HELVETICA_BOLD); //marx: version 0.7.02
  Fl_Tooltip::color(68);               //marx: version 0.7.02

  // run "late" command files -- -C <file>
  for(i = 0; i < nlatecmds; i++)
    specks_commandstr( &ppui.st, latecmds[i] );

  // this shouldn't be necessary, but does it help on MacOS X?? slevy 2013.07.11
  ppui.view->redraw();

  return Fl::run();
}
