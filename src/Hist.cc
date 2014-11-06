/*
 * FLTK History widget: list browser plus command input box.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "config.h"

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
  #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
extern char *alloca ();
#   endif
#  endif
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FL/Fl.H"
#include "FL/Fl_Browser.H"
#include "FL/Fl_Input.H"
#include "Hist.H"

void HistInput::hist( Hist *h ) {
  hist_ = h;
  if(h) h->input(this);
}

int HistInput::handle( int ev ) {
  if(hist_ && hist_->handle_nav(ev))
    return 1;
  


return Fl_Input::handle(ev);
}

void HistBrowser::hist( Hist *h ) {
  hist_ = h;
  if(h) h->browser(this);
}

int HistBrowser::handle( int ev ) {
  if(hist_ && hist_->handle_nav(ev))
    return 1;
  //printf("*** FL_FOCUS = %d    Fl::event_key() = %d   FL_Tab = %d ***\n", FL_FOCUS, Fl::event_key(), FL_Tab);
  //marx version 0.7.02 - to insure tab but not arrow navigation result in focus going to command line
  if( (ev != FL_FOCUS)  && (Fl::event_key() == FL_Tab) ) {
    //printf("when (ev != FL_FOCUS)  && (Fl::event_key() == FL_Tab) is true the event is %d\n",ev);
    take_focus();
    return 1; //accept focus
  }
  //marx version 0.7.02 - end of tab logic
  //printf("when (ev != FL_FOCUS)  && (Fl::event_key() == FL_Tab) is NOT true the event is %d\n",ev);


  return Fl_Browser::handle(ev);
}

int Hist::handle( int ev ) {
  if(handle_nav(ev))
    return 1;
  return Fl_Group::handle(ev);
}

int HistBrowser::selscanup( int from ) {
  while(from < size() && !selected(from))
    from++;
  return from;
}

int HistBrowser::selscandown( int from ) {
  if(from > size()) from = size()-1;
  while(from > 0 && selected(from))
    from--;
  return from;
}

void HistBrowser::tighten_histrange() {
  int i;
  if(min <= 0) min = 1;
  if(max > size()) max = size();
  for(i = min; i <= max && !selected(i); i++)
	;
  min = i;
  for(i = max; i >= min && !selected(i); i--)
	;
  max = i;
}

int HistInput::scaletext( float amount ) {
    int newsize;
    if(amount < 0) newsize = (int) (-amount * textsize() + .5);
    else if(amount > 0) newsize = (int) amount;
    else newsize = 14;
    textsize(newsize);
    int dh = (int) (1.6*newsize + 2) - h();
    if(this->parent() == this->hist()) {
	resize( x(), y() - dh, w(), h() + dh );
    } else {
	Fl_Widget *subhist = this->parent();
	while(subhist->parent() && subhist->parent() != this->hist())
	    subhist = subhist->parent();
	((Fl_Group *)subhist)->resize( subhist->x(), subhist->y() - dh,
				       subhist->w(), subhist->h() + dh );
    }
    return dh;
}

void HistBrowser::scaletext( float amount ) {
    int newsize;
    if(amount < 0) newsize = (int) (-amount * textsize() + .5);
    else if(amount > 0) newsize = (int) amount;
    else newsize = 14;
    textsize(newsize);
}

void Hist::scaletext( float amount ) {
    int dh = 0;
    if(input()) {
	dh = input()->scaletext(amount);
    }
    if(browser()) {
	browser()->scaletext(amount);
	browser()->resize( browser()->x(), browser()->y(), browser()->w(),
						browser()->h() - dh );
    }
    redraw();
}

static char msgprefix[] = "@C2@.# ";
#define MSGPREFIXSKIP (sizeof(msgprefix) - 3)	/* retain "# " */

int HistBrowser::is_cmd( int line ) {
  if(line <= 0 || line > size())
    return 0;
  return 0!=strncmp(text(line), msgprefix, sizeof(msgprefix)-1);
}

#define MAXHISTORY	500

void HistBrowser::addline( const char *str, int as_cmd ) {
  char *what = (char *)str;
  if(as_cmd) {
    what = (char *)alloca( strlen(str) + sizeof(msgprefix) + 1 );
    strcpy(what, msgprefix);
    strcpy(what+sizeof(msgprefix)-1, str);
  }
  if(size() >= MAXHISTORY) {
    for(int hyst = 0; hyst < 5; hyst++) {
	remove( 1 );
	min--;
	max--;
    }
  }
  add( what, (void *)as_cmd );
  middleline( size() ); // bottomline() won't do
  redraw();
}

int HistBrowser::find_cmd( int fromline, int incr, int takeany )
{
  while(fromline > 0 && fromline <= size()) {
    if(takeany || is_cmd(fromline))
	return fromline;
    fromline += incr;
  }
  return 0;
}

int HistBrowser::handle_nav( int ev ) {
  int incr = 1;
  int line;
  HistInput *inp;
  if(hist_ == NULL || (inp = hist_->input()) == NULL)
    return 0;

  if(ev == FL_KEYBOARD || ev == FL_SHORTCUT) {
    switch(Fl::event_key()) {

    case FL_Up:
    case FL_Page_Up:
	incr = -1;		/* and fall into... */

    case FL_Down:
    case FL_Page_Down:
	tighten_histrange();
	line = selected(min) ? min : size()-incr;
	int takeany = Fl::event_state(FL_CTRL) ? 1 : 0;
	line = find_cmd( line+incr, incr, takeany );
	if(line) {
	    deselect(0);
	    select(line, 1);
	    middleline( line );
	    min = max = line;

	    const char *s, *str = text(line);
	    /* skip leading comment, color, etc. prefixes */
	    while(str) {
		if(str[0] == '#' || str[0] == ' ')
		    str++;
		else if(str[0] == '@' && (s = strstr(str, "@.")) != NULL)
		    str = s+2;
		else
		    break;
	    }
	    if(str)
		inp->value( str );
	    inp->position( inp->size(), inp->size() );
	}
	inp->take_focus();
	return 1;
    }
  }
  return 0;
}


void HistBrowser::picked_cb( HistBrowser *brow, void * ) {
  int i, which = brow->value();
  if(which <= 0) return;
  if(brow->min > which) brow->min = which;
  if(brow->max < which) brow->max = which;
  brow->tighten_histrange();

  if(!Fl::event_state(FL_BUTTON1|FL_BUTTON2|FL_BUTTON3)) {
    // All mouse-buttons released -- it's worth rebuilding our selection.
    int pos, len = 0;
    const char *s;
    for(i = brow->min; i <= brow->max; i++) {
	if(brow->selected(i)) {
	    s = brow->text(i);
	    if(!strncmp(s, msgprefix, sizeof(msgprefix)-1))
		s += MSGPREFIXSKIP;
	    len += strlen(s) + 1;
	}
    }
    if(len == 0) {
	Fl::selection_owner( NULL );
    } else {
	char *all = (char *)alloca( len + 1 );
	pos = 0;
	for(i = brow->min; i <= brow->max; i++) {
	    if(brow->selected(i)) {
		s = brow->text(i);
		if(!strncmp(s, msgprefix, sizeof(msgprefix)-1))
		    s += MSGPREFIXSKIP;
		len = strlen(s);
		memcpy(&all[pos], s, len);
		pos += len;
		all[pos++] = '\n';
	    }
	}
	Fl::selection( *brow, all, pos );
    }
  }
}
