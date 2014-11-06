#ifndef HIST_H
#define HIST_H
/*
 * FLTK History widget: list browser plus command input box.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>

class Hist;

class HistInput : public Fl_Input {
 public:
  HistInput(int x,int y,int w,int h,const char *label=0) :
			    Fl_Input(x,y,w,h,label) {
    hist_ = 0;
  }
  virtual int handle(int ev);
  void hist( Hist *h );
  Hist *hist() { return hist_; }
  int scaletext( float amount );

 protected:
  Hist *hist_;
};

class HistBrowser : public Fl_Browser {
 public:
  HistBrowser(int x, int y, int w, int h, const char *l=0) :
				Fl_Browser(x,y,w,h,l) {
    hist_ = 0;
    callback( (void (*)(Fl_Widget *,void *))picked_cb );
  }
  virtual int handle(int ev);
  void hist( Hist *h );
  Hist *hist() const { return hist_; }
  int min, max;		// range of selection ... ?

  int is_cmd( int line );
  int find_cmd( int fromline, int incr, int takeany = 0 );
  void addline( const char *str, int is_cmd=0 );
  int handle_nav( int ev );
  void tighten_histrange();
  void scaletext( float amount );

  static void picked_cb( HistBrowser *, void * );

 protected:
  Hist *hist_;
  int selscanup(int fromline);
  int selscandown( int fromline);
};

class Hist : public Fl_Group {
 public:
  Hist(int x, int y, int w, int h, const char *l=0) :
				Fl_Group(x,y,w,h,l) {
    inp = 0;
    brow = 0;
  }
  virtual int handle(int ev);

  HistInput *input() const { return inp; }
  void input(HistInput *i) { inp = i; }
  HistBrowser *browser() const { return brow; }
  void browser(HistBrowser *b) { brow = b; }

  void scaletext( float amount );

  int handle_nav( int ev ) {
    return brow ? brow->handle_nav(ev) : 0;
  }

  void addline( const char *str, int is_cmd=0 ) {
    if(brow) brow->addline(str, is_cmd);
  }

 protected:
  HistInput *inp;
  HistBrowser *brow;
};


#endif
