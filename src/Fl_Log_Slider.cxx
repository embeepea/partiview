//
// "$Id: Fl_Log_Slider.cxx,v 1.5 2002/03/14 17:33:08 slevy Exp $"
//
// Value slider widget for the Fast Light Tool Kit (FLTK).
//  -- adapted (slevy@ncsa.uiuc.edu) for log scaling
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

#include <FL/Fl.H>
#include "Fl_Log_Slider.H"
#include <FL/fl_draw.H>
#include <stdio.h>
#include <math.h>

Fl_Log_Slider::Fl_Log_Slider(int x,int y,int w,int h, const char*l)
: Fl_Slider(x,y,w,h,l) {
  step(0,100);
  textfont_ = FL_HELVETICA;
  textsize_ = 10;
  textcolor_ = FL_BLACK;
  style_ = FL_LOG_LINEAR;
  textwidth_ = 45;
}

int Fl_Log_Slider::format(char *buf) {
  if(style_ == FL_LINEAR_LINEAR || style_ == FL_LOG_LINEAR)
    return Fl_Valuator::format(buf);
  else
    return sprintf(buf, "%g", truevalue());
}

double Fl_Log_Slider::truevalue() {
    if(style_ == FL_LINEAR_LINEAR)
	return value();
    double smin = minimum();
    if(truemin_ == 0) {
	truemin_ = (truemax_ == 0) ? 0.1 : 0.1 * truemax_;
    }
    if(truemax_ == 0) truemax_ = 100 * truemin_;
    double lograt = log(fabs(truemax_ / truemin_));
    return truemin_ * exp(lograt *
			fabs( (value() - smin) / (maximum() - smin) ) );
}

void Fl_Log_Slider::truevalue( double v ) {

    oldtruevalue_ = truevalue_;
    truevalue_ = v;

    if(style_ == FL_LINEAR_LINEAR) { 
	value(v);
	return;
    }
    if(truemin_ == 0) {
	truemin_ = (truemax_ == 0) ? 0.1 : 0.1 * truemax_;
    }
    if(truemax_ == 0) truemax_ = 100 * truemin_;
    double lograt = log(fabs(truemax_ / truemin_));
    double w = v / truemin_;
    if(w > 0) w = log(w) / lograt;
    value( minimum() + w * (maximum() - minimum()) );
}

double Fl_Log_Slider::printvalue( double truev ) {
    switch(style_) {
    case FL_LOG_LOG:
    case FL_LOG_LINEAR:
	return truev==0 ? -10 : log10( fabs(truev) );

    default:
    case FL_LINEAR_LINEAR:
	return truev;
    }
}


void Fl_Log_Slider::draw() {
  int sxx = x(), syy = y(), sww = w(), shh = h();
  int bxx = x(), byy = y(), bww = w(), bhh = h();
  if (horizontal()) {
    bww = textwidth_; sxx += textwidth_; sww -= textwidth_;
  } else {
    syy += 25; bhh = 25; shh -= 25;
  }
  if (damage()&FL_DAMAGE_ALL) draw_box(box(),sxx,syy,sww,shh,color());
  if(truedirt_) {
    minimum( printvalue(truemin()) );
    maximum( printvalue(truemax()) );
    truevalue( truevalue_ );		// sneaky, huh?
    truedirt_ = 0;
  }
  Fl_Slider::draw(sxx+Fl::box_dx(box()),
		  syy+Fl::box_dy(box()),
		  sww-Fl::box_dw(box()),
		  shh-Fl::box_dh(box()));
  draw_box(box(),bxx,byy,bww,bhh,color());
  char buf[128];
  format(buf);
  
  fl_font(textfont(), textsize());
#if FL_MAJOR_VERSION==1 && FL_MINOR_VERSION==0
#  define fl_inactive(c)  inactive(c)
#endif
  fl_color(active_r() ? textcolor() : fl_inactive(textcolor()));
  fl_draw(buf, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
}

int Fl_Log_Slider::handle(int event) {
  int sxx = x(), syy = y(), sww = w(), shh = h();
  if (horizontal()) {
    sxx += textwidth_; sww -= textwidth_;
  } else {
    syy += 25; shh -= 25;
  }
  return Fl_Slider::handle(event,
			   sxx+Fl::box_dx(box()),
			   syy+Fl::box_dy(box()),
			   sww-Fl::box_dw(box()),
			   shh-Fl::box_dh(box()));
}

//
// End of "$Id: Fl_Log_Slider.cxx,v 1.5 2002/03/14 17:33:08 slevy Exp $".
//
