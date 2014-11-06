#ifndef _PARTIVIEW_H
#define _PARTIVIEW_H

/*
 * User-interface-related data for partiview, using FLTK.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef WIN32
# include "winjunk.h"
#endif

#ifndef FLHACK
#include "Hist.H"
#include "Fl_Log_Slider.H"
#include "partipanel.H"
#include "Gview.H"
#include "Plot.H"
#include "specks.h"
#include "partiviewc.h"
#include "Fl_Scroll_Thin.H"
#else // FLHACK
#include "Gview.H"
#endif // FLHACK

typedef struct subcam {
  char name[8];
  float azim, elev, roll;
  float nleft, right, ndown, up;
} Subcam;

struct _ppui {
  struct stuff *st;

  struct wfpath path;
  char *snapfmt;
  int snapfno;
  int jpegqual;		// snapshot quality if making JPEGs
  float censize;
  float pickrange;
  char *reqwinsize;

  float home[6];        //marx marx version 0.7.03 home is equivalent to a jump to a named point of view

  SClock *clk;		/* master data clock */
  SClock *camclock;	/* camera animation clock */
    int playing;
    int playevery;
    int playidling;
    float playspeed;
    int framebase;
    float playtimebase;
    int playloop;

  double timebasetime;

  int scroom;
  Subcam *sc;
  int subcam;
  float sctilt;

#ifndef FLHACK
  Fl_Double_Window* mainwin;

  class Fl_Tile* maintile;
  Fl_Group* topclump;
  Fl_Group* toprow;
  Fl_Pack*  objtogs;
  Fl_Group* objgroup;
  Fl_Menu_Button* more;
  Fl_Menu_Button* obj;
  Fl_Menu_Button* nav;
  Fl_Menu_Item* inertiaon;
  Fl_Light_Button* point;
  Fl_Light_Button* poly;
  Fl_Light_Button* label;
  Fl_Light_Button* texture;
  Fl_Light_Button* box;

  Fl_Button* homejump;            //steven marx: version 0.7.03

  Fl_Value_Slider* slum;

  Fl_Group* slidergroup;          //steven marx: version 0.7.02 original slum slider overlaid by generic sliders
  Fl_Menu_Button* sldtype;        //steven marx: version 0.7.02  menu selectable sliders
  Fl_Value_Slider* genericslider; //steven marx: version 0.7.02  we change genericslider into the one we want (replaced by the following line)
  Fl_Button* linlog;             //steven marx: version 0.7.02 makes genericslider toggle linear <-> log 

  Fl_Light_Button* feed;
  Fl_Menu_Button* playmenu;

  Fl_Group* steprow;
  Fl_Button* step[2];
  Fl_Button* runstop[2];
  Fl_Float_Input* timestep;
  Fl_Roller* jog;
  Fl_Button* settrip;
  Fl_Button* backtrip;
  Fl_Float_Input* timebase;
  Fl_Log_Slider* stepspeed;

  Fl_Group* animrow;
  Fl_Button* rdata;
  Fl_Button* play;
  Fl_Counter* playframe;
  Fl_Value_Slider* playtime;

  Hist *cmdhist;
  HistInput *cmd;

  Fl_Double_Window* hrdiagwin;
  Fl_Menu_Item* hrdiagon;
  Fl_Plot*  hrdiag;

  Fl_Gview *freewin;
  Fl_Gview *boundwin;
  Fl_Double_Window *freemain;
#endif // !FLHACK

  Fl_Gview* view;

  int detached;

  void (*drawtrace)();
};

extern struct _ppui  ppui;

extern void ppui_refresh( struct stuff * );
extern int specks_commandfmt( struct stuff **, const char *fmt, ... );

#endif /*_PARTIVIEW_H*/
