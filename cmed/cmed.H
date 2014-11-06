#ifndef _CMED_H
#define _CMED_H

#ifdef _WIN32
# include "../src/winjunk.h"
#endif

#include "cmedpanel.H"
#include "CMedit.H"
#include "colorpatch.H"

extern struct _cshow {
    colorpatch* color;
    Fl_Output* hsba;
    Fl_Output* rgba;
    Fl_Value_Input* cindex;
    Fl_Box *hsblbls[3];
    Fl_Value_Input *postscalein;
    Fl_Value_Input *postexponin;
    Fl_Value_Input *cmentin;
    Fl_Output* scaleout;
} cshow;

#endif /*_CMED_H*/

