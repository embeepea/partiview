//
// "$Id: Fl_Log_Slider.H,v 1.3 2002/06/18 21:25:24 slevy Exp $"
//
// Log-value slider header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2000 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//

// Adapted to make a logarithmic slider
// by Stuart Levy, slevy@ncsa.uiuc.edu,
// University of Illinois 2001
// This file is part of partiview, released under the
// Illinois Open Source License; see the file LICENSE.partiview for details.

#ifndef Fl_Log_Slider_H
#define Fl_Log_Slider_H

#include <FL/Fl_Slider.H>

enum Fl_Log_Style {
	FL_LINEAR_LINEAR, 	// Linear scale, linear text readout
	FL_LOG_LINEAR,		// Log(base10) scale, linear text readout
	FL_LOG_LOG		// Log scale, log text readout
};

class Fl_Log_Slider : public Fl_Slider {
    uchar textfont_, textsize_, textcolor_;
    enum Fl_Log_Style style_;
    int  textwidth_;		// width (in pixels) for text area for horiz
    double truevalue_, oldtruevalue_;
    double truemin_, truemax_;
    int truedirt_;
protected:
    virtual int format(char *buf);
    double printvalue(double trueval);
public:
    FL_EXPORT void draw();
    FL_EXPORT int handle(int);
    FL_EXPORT Fl_Log_Slider(int x,int y,int w,int h, const char *l = 0);

    double truevalue();
    void truevalue(double v);
    double truemin() const {return truemin_;}
    double truemax() const {return truemax_;}
    void truemin(double v) { truemin_ = v; truedirt_ = 1; }
    void truemax(double v) { truemax_ = v; truedirt_ = 1; }
    void truebounds(double vmin, double vmax) {truemin(vmin); truemax(vmax);}

    Fl_Font textfont() const {return (Fl_Font)textfont_;}
    void textfont(uchar s) {textfont_ = s;}
    uchar textsize() const {return textsize_;}
    void textsize(uchar s) {textsize_ = s;}
    Fl_Color textcolor() const {return (Fl_Color)textcolor_;}
    void textcolor(uchar s) {textcolor_ = s;}
    enum Fl_Log_Style logstyle() { return style_; }
    void logstyle( Fl_Log_Style style ) { style_ = style; redraw(); }
    int textwidth() { return textwidth_; }
    void textwidth( int pixels ) { textwidth_ = pixels; redraw(); }
};

#endif

//
// End of "$Id: Fl_Log_Slider.H,v 1.3 2002/06/18 21:25:24 slevy Exp $".
//
