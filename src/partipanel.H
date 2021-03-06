// generated by Fast Light User Interface Designer (fluid) version 1.0300

#ifndef partipanel_H
#define partipanel_H
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Button.H>
extern void pp_inertia_on_cb(Fl_Menu_*, void*);
extern void pp_hrdiag_on_cb(Fl_Menu_*, void*);
extern void pp_obj_cb(Fl_Menu_Button*, void*);
extern void pp_nav_cb(Fl_Menu_Button*, struct stuff **);
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Button.H>
extern void pp_sldtype_cb(Fl_Menu_Button*, void*);
#include <FL/Fl_Value_Slider.H>
extern void pp_slum_cb(Fl_Value_Slider*, struct stuff **);
extern void pp_genericslider_cb(Fl_Value_Slider*, void*);
extern void pp_linlog_cb(Fl_Button*, void*);
extern void pp_objtog_cb(Fl_Button*, void*);
extern void pp_step_cb(Fl_Button*, void*);
#include <FL/Fl_Float_Input.H>
extern void pp_timeinput_cb(Fl_Float_Input*, void*);
#include <FL/Fl_Roller.H>
extern void pp_jog_cb(Fl_Roller*, void*);
extern void pp_settrip_cb(Fl_Button*, void*);
extern void pp_backtrip_cb(Fl_Button*, void*);
extern void pp_timebaseinput_cb(Fl_Float_Input*, void*);
extern void pp_stepspeed_cb(class Fl_Log_Slider*, void*);
extern void pp_run_cb(Fl_Button*, void*);
extern void pp_feed_cb(Fl_Light_Button*, void*);
extern void pp_rdata_cb(Fl_Button*, struct stuff **);
extern void pp_play_cb(Fl_Button*, struct stuff **);
extern Fl_Menu_Button *playmenu;
#include <FL/Fl_Counter.H>
extern void pp_playframe_cb(Fl_Counter*, struct stuff **);
extern void pp_playtime_cb(Fl_Value_Slider*, struct stuff **);
extern Fl_Button *editbtn;
#include <FL/Fl_Tile.H>
#include "Hist.H"
#include <FL/Fl_Box.H>
extern void pp_cmd_cb(HistInput*, void*);
Fl_Double_Window* make_window();
extern Fl_Menu_Item menu_ppui[];
#endif
