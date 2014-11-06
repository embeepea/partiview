//Fl_Scroll_Thin.cxx is identical to Fl_Scroll.cxx except we made
//the scroll bar thinner by changing SLIDER_WIDTH from 16 to 8.
//and we altered test for FL_DAMAGE_ALL in draw() to acoomodate
//damage children on scroll problem in windows (see comments
//in draw()

// "$Id: Fl_Scroll_Thin.cxx,v 1.2 2003/10/26 16:20:22 slevy Exp $"
//
// Scroll widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2002 by Bill Spitzak and others.
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
#include "Fl_Scroll_Thin.H"
#include <FL/fl_draw.H>

// Insure the scrollbars are the last children:
void Fl_Scroll_Thin::fix_scrollbar_order() {
  Fl_Widget** a = (Fl_Widget**)array();
  if (a[children()-1] != &scrollbar) {
    int i,j; for (i = j = 0; j < children(); j++)
      if (a[j] != &hscrollbar && a[j] != &scrollbar) a[i++] = a[j];
    a[i++] = &hscrollbar;
    a[i++] = &scrollbar;
  }
}

void Fl_Scroll_Thin::draw_clip(void* v,int X, int Y, int W, int H) {
  fl_clip(X,Y,W,H);
  Fl_Scroll_Thin* s = (Fl_Scroll_Thin*)v;
  // erase background if there is a boxtype:
  if (s->box()) {
    fl_color(s->color());
    fl_rectf(X,Y,W,H);
  }
  Fl_Widget*const* a = s->array();
  int R = X; int B = Y; // track bottom & right edge of all children
  for (int i=s->children()-2; i--;) {
    Fl_Widget& o = **a++;
    s->draw_child(o);
    s->draw_outside_label(o);
    if (o.x()+o.w() > R) R = o.x()+o.w();
    if (o.y()+o.h() > B) B = o.y()+o.h();
  }
  // fill any area to right & bottom of widgets:
  if (R < X+W && B > Y) {
    fl_color(s->color());
    fl_rectf(R,Y,X+W-R,B-Y);
  }
  if (B < Y+H) {
    fl_color(s->color());
    fl_rectf(X,B,W,Y+H-B);
  }
  fl_pop_clip();
}

void Fl_Scroll_Thin::bbox(int& X, int& Y, int& W, int& H) {
  X = x()+Fl::box_dx(box());
  Y = y()+Fl::box_dy(box());
  W = w()-Fl::box_dw(box());
  H = h()-Fl::box_dh(box());
  if (scrollbar.visible()) {
    W -= scrollbar.w();
    if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
  }
  if (hscrollbar.visible()) {
    H -= hscrollbar.h();
    if (scrollbar.align() & FL_ALIGN_TOP) Y += hscrollbar.h();
  }
}

void Fl_Scroll_Thin::draw() {

  fix_scrollbar_order();
  int X,Y,W,H; bbox(X,Y,W,H);
  uchar d = damage();

  //if (d & FL_DAMAGE_ALL) { // full redraw - original fltk code replaced by if(d) { on next line
  if (d) {  //marx 0.7.02 any damage causes full redraw   
    // marx - version 0.7.02 - under windows redraw of children on scroll is not handled without this change
    //with or without this change linux redraws some scroll children unnecessarily on generic slider movement
    //this linux behaivior causes mild flicker of some scroll children on slider movement
    //this linux problem happens only when gview is updated and is not directly related to the generic slider
    draw_box(box(),x(),y(),w(),h(),color());
    draw_clip(this, X, Y, W, H);
  } else {
    if (d & FL_DAMAGE_SCROLL) { // scroll the contents:
      fl_scroll(X, Y, W, H, oldx-xposition_, oldy-yposition_, draw_clip, this);
    }
    if (d & FL_DAMAGE_CHILD) { // draw damaged children
      fl_clip(X, Y, W, H);
      Fl_Widget*const* a = array();
      for (int i=children()-2; i--;) update_child(**a++);
      fl_pop_clip();
    }
  }


  // accumulate bounding box of children:
  int l = X; int r = X; int t = Y; int b = Y;
  Fl_Widget*const* a = array();
  for (int i=children()-2; i--;) {
    Fl_Object* o = *a++;
    if (o->x() < l) l = o->x();
    if (o->y() < t) t = o->y();
    if (o->x()+o->w() > r) r = o->x()+o->w();
    if (o->y()+o->h() > b) b = o->y()+o->h();
  }

  // turn the scrollbars on and off as necessary:
  // See if children would fit if we had no scrollbars...
  X = x()+Fl::box_dx(box());
  Y = y()+Fl::box_dy(box());
  W = w()-Fl::box_dw(box());
  H = h()-Fl::box_dh(box());
  int vneeded = 0;
  int hneeded = 0;
  if (type() & VERTICAL) {
    if ((type() & ALWAYS_ON) || t < Y || b > Y+H) {
      vneeded = 1;
      W -= scrollbar.w();
      if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
    }
  }
  if (type() & HORIZONTAL) {
    if ((type() & ALWAYS_ON) || l < X || r > X+W) {
      hneeded = 1;
      H -= hscrollbar.h();
      if (scrollbar.align() & FL_ALIGN_TOP) Y += hscrollbar.h();
      // recheck vertical since we added a horizontal scrollbar
      if (!vneeded && (type() & VERTICAL)) {
	if ((type() & ALWAYS_ON) || t < Y || b > Y+H) {
	  vneeded = 1;
	  W -= scrollbar.w();
	  if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
	}
      }
    }
  }
  // Now that we know what's needed, make it so.
  if (vneeded && !scrollbar.visible()) {
    scrollbar.set_visible();
    d = FL_DAMAGE_ALL;
  }
  else if (!vneeded && scrollbar.visible()) {
    scrollbar.clear_visible();
    draw_clip(this,
	      scrollbar.align()&FL_ALIGN_LEFT ? X : X+W-scrollbar.w(),
	      Y, scrollbar.w(), H);
    d = FL_DAMAGE_ALL;
  }
  if (hneeded && !hscrollbar.visible()) {
    hscrollbar.set_visible();
    d = FL_DAMAGE_ALL;
  }
  else if (!hneeded && hscrollbar.visible()) {
    hscrollbar.clear_visible();
    draw_clip(this,
	      X, scrollbar.align()&FL_ALIGN_TOP ? Y : Y+H-hscrollbar.h(),
	      W, hscrollbar.h());
    d = FL_DAMAGE_ALL;
  }

  scrollbar.resize(scrollbar.align()&FL_ALIGN_LEFT ? X-scrollbar.w() : X+W,
		   Y, scrollbar.w(), H);
  scrollbar.value(oldy = yposition_ = (Y-t), H, 0, b-t);

  hscrollbar.resize(X,
		    scrollbar.align()&FL_ALIGN_TOP ? Y-hscrollbar.h() : Y+H,
		    W, hscrollbar.h());
  hscrollbar.value(oldx = xposition_ = (X-l), W, 0, r-l);

  // draw the scrollbars:
  if (d & FL_DAMAGE_ALL) {
    draw_child(scrollbar);
    draw_child(hscrollbar);
    if (scrollbar.visible() && hscrollbar.visible()) {
      // fill in the little box in the corner
      fl_color(color());
      fl_rectf(scrollbar.x(), hscrollbar.y(), scrollbar.w(), hscrollbar.h());
    }
  } else {
    update_child(scrollbar);
    update_child(hscrollbar);
  }
}

void Fl_Scroll_Thin::resize(int X, int Y, int W, int H) {
  fix_scrollbar_order();
  // move all the children:
  Fl_Widget*const* a = array();
  for (int i=children()-2; i--;) {
    Fl_Object* o = *a++;
    o->position(o->x()+X-x(), o->y()+Y-y());
  }
  Fl_Widget::resize(X,Y,W,H);
}

void Fl_Scroll_Thin::position(int X, int Y) {
  int dx = xposition_-X;
  int dy = yposition_-Y;
  if (!dx && !dy) return;
  xposition_ = X;
  yposition_ = Y;
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o == &hscrollbar || o == &scrollbar) continue;
    o->position(o->x()+dx, o->y()+dy);
  }
  damage(FL_DAMAGE_SCROLL);
}

void Fl_Scroll_Thin::hscrollbar_cb(Fl_Widget* o, void*) {
  Fl_Scroll_Thin* s = (Fl_Scroll_Thin*)(o->parent());
  s->position(int(((Fl_Scrollbar*)o)->value()), s->yposition());
}

void Fl_Scroll_Thin::scrollbar_cb(Fl_Widget* o, void*) {
  Fl_Scroll_Thin* s = (Fl_Scroll_Thin*)(o->parent());
  s->position(s->xposition(), int(((Fl_Scrollbar*)o)->value()));
}

//#define SLIDER_WIDTH 16
//created Fl_Scroll_Thin.cxx and Fl_Scroll_Thin.H just to change this #define
#define SLIDER_WIDTH 8

Fl_Scroll_Thin::Fl_Scroll_Thin(int X,int Y,int W,int H,const char* L)
  : Fl_Group(X,Y,W,H,L), 
    scrollbar(X+W-SLIDER_WIDTH,Y,SLIDER_WIDTH,H-SLIDER_WIDTH),
    hscrollbar(X,Y+H-SLIDER_WIDTH,W-SLIDER_WIDTH,SLIDER_WIDTH) {
  type(BOTH);
  xposition_ = 0;
  yposition_ = 0;
  hscrollbar.type(FL_HORIZONTAL);
  hscrollbar.callback(hscrollbar_cb);
  scrollbar.callback(scrollbar_cb);
}

int Fl_Scroll_Thin::handle(int event) {
  fix_scrollbar_order();
  return Fl_Group::handle(event);
}

//
// End of "$Id: Fl_Scroll_Thin.cxx,v 1.2 2003/10/26 16:20:22 slevy Exp $".
//
