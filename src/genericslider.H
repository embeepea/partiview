#ifndef GENERICSLIDER_H
#define GENERICSLIDER_H

//steven marx: generic slider introduced version 0.7.02

#include<math.h>
#include "partiview.H"

#include <FL/Fl_Float_Input.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Output.H>

void reset_loglin(int menunum);
void pp_sldtype_init(Fl_Menu_Button *);
void genericslider_setparam(void);      
void genericslider_initparam(void); 
  
void pp_sldtype_cb(Fl_Menu_Button* m, void *); 
void pp_linlog_cb(Fl_Light_Button* o, void *);
void slider_update(float* p);
void slider_update(int* p);
void genericslider_setparam(void);
void pp_genericslider_cb(Fl_Value_Slider* sl, void*);

extern int gensliderchg;  //flag that indicates if == 1 that value of current generic slider changed
enum sldtypes  {ALPHA, FOV, CENSIZE, CHROMADEPTHRANGE, FOCALLEN, LABELMINPIXELS, LABELSIZE, POLYSIDES, POLYSIZE, SLUM,
    NUMSLD};

void gensldtimeout(void* o);

#endif
