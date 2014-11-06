//steven marx: generic slider introduced version 0.7.02
#include "genericslider.H"

// #include <unistd.h>

static char *sldtypetitles[] = {"alpha", "fov", "censize", "chromarange", "focallength", "lblminpix", "labelsize", "polysides", "polysize", "slum"};
static char  sidtitles[NUMSLD][30];
static float  minrg[NUMSLD][2], maxrg[NUMSLD][2], steprg[NUMSLD][2]; //generic slider's min, max, step, by  [slider type][log/linear][data group]
static int  loglinsld[NUMSLD];                                       //toggle = 1   always log = 2   always linear =3 by slider
static int  loglinstate[NUMSLD];                                     //0 = off  1 = on  by slider

void pp_sldtype_init(Fl_Menu_Button * m) { 
  m->add("&alpha|&fov|&censize|c&hromarange|&focallength|labelminpi&xels|labelsi&ze|&polysides|pol&ysize|&slum");
  for(int i = m->size(); --i >= 0; ) 
    (((Fl_Menu_Item *)(m->menu()))[i]).labelcolor( m->labelcolor() );
}

void pp_sldtype_cb(Fl_Menu_Button* m, void *) {                      
  if(m->value() == SLUM) {
    ppui.genericslider->hide();
    ppui.slum->show();
    ppui.linlog->value(0);
    ppui.linlog->label("log");
    ppui.slidergroup->redraw();
    m->label(sldtypetitles[m->value()]);
    ppui.view->redraw();
    gensliderchg = 0;
    return;
  }
  genericslider_setparam();     //re-initialize slider 
  m->label(sldtypetitles[m->value()]); //change the lable under the menu to reflect chosen menu itme
  ppui.slum->hide();
  ppui.genericslider->show();
  if(Fl::event_button() == 2 || Fl::event_button() == 3) {  //first draft of code to support use of dialog box to change min, max, or step parameters
    int x = ppui.mainwin->x();
    int y = ppui.mainwin->y();
    int w = ppui.mainwin->w();
    int h = ppui.mainwin->h();
    static char wtitle[50];
    sprintf(wtitle, "%s slider parameters", sldtypetitles[m->value()]);
    if((w/2) > 300 )
      w = 600;
    if((h/2) > 300 )
      h = 600;      
    Fl_Window dialg(x,y, w/2+ 5, h/3, wtitle); //put the dialog to the left of the sliders
    Fl_Float_Input minrange(10, 7, w/4, 25, "minimum");
    Fl_Float_Input maxrange(10, 57, w/4, 25, "maximum");
    Fl_Float_Input steprange(10,107,w/4, 25, "step");
    Fl_Button b(w/2-65, 7, 60, 30, "OK");
    Fl_Button c(w/2-65, 45, 60, 30, "Cancel");
    minrange.labelsize(12);
    maxrange.labelsize(12);
    steprange.labelsize(12);
    minrange.align( FL_ALIGN_BOTTOM );
    maxrange.align( FL_ALIGN_BOTTOM );
    steprange.align(FL_ALIGN_BOTTOM);
    //now we supply the min, max, and step values to the respective input widgets
    char buf[40];
    sprintf(buf, "%g", minrg[m->value()][1]);
    minrange.value(buf);
    sprintf(buf, "%g", maxrg[m->value()][1]);
    maxrange.value(buf);
    sprintf(buf, "%g", steprg[m->value()][1]);
    steprange.value(buf);
    steprange.tooltip("The number of steps for the log slider = number of steps for the linear slider; therefore their step sizes are not equal");
    minrange.tooltip("Both log and linear sliders output the same value. Both slider types have the same minimum and maximum range.");
    maxrange.tooltip("Both log and linear sliders output the same value. Both slider types have the same minimum and maximum range.");
    dialg.add(b);
    dialg.add(c);
    dialg.add(minrange);
    dialg.add(maxrange);
    dialg.add(steprange);
    dialg.set_modal();
    dialg.show();
    b.value(0);
    c.value(0);
    while(1) {
      if(!dialg.visible()) {
	c.value(1); //software simulated user cancel
	break;      //dialog was closed so lets exit this code block
      }
      Fl::check();
      if(b.value() == 1 || c.value() == 1) {
	if( (atof(steprange.value()) <= 0)  && (b.value() == 1) ) {
	  fl_alert("step size must be > 0");
	  continue;
	}
	//first we check that values are within bounds for the slider type. so far we only check out FOV
        int mx = m->value();
	int loop = 0;
	switch(mx) {
	case FOV:
          if(atof(minrange.value()) <= 0 || atof(maxrange.value()) >= 179.45) {
	    fl_alert("the min for FOV is <= 0 or the max >= 179.45 please correct.");
	    loop = 1;
	    break;
	  }
	default:
	  break;
	}
	if(loop == 1)
	  continue;
        //end of check on values within bounds
	break;
      }
    }//end of while(1)
    if(c.value() == 0) {
      int answ1 = 1;
      int answ2 = 1;
      if( atof(minrange.value()) >= atof(maxrange.value())  )
	answ1 = fl_ask("The minimum is greater than or equal to maximum. is this what you want ?");
      double rx = fabs(atof(minrange.value()) - atof(maxrange.value()) );
      if( ( rx/atof(steprange.value())) < 10)
	answ2 = fl_ask("The step size is large compared to the range. is this what you wnat ?");
      if(!answ1 || !answ2) {
	fl_alert("Please make your correction(s).");
	while(1) {
	  Fl::check();
	  if(b.value() == 1 || c.value() == 1)
	    break;   
	}
      }
      if(c.value() == 0) {
        int mx = m->value();
	// now we update the linear slider with dialog values
	minrg[mx][1] =  atof(minrange.value());
	maxrg[mx][1] =  atof(maxrange.value());
	steprg[mx][1] = atof(steprange.value());
	// next we synchronize the log variant with the linear
	minrg[mx][0] =   (minrg[mx][1] < .001) ? -3 : log10(minrg[mx][1]);
	maxrg[mx][0] =   log10(maxrg[mx][1]);	  
	// we set the step size for log mode such that the number of steps over the range = the number of steps in the linear mode
	double numsteps = fabs((maxrg[mx][1] - minrg[mx][1])/steprg[mx][1]);
	if(numsteps == 0)
	  numsteps = 1; //don't want to divide by zero in the log step size calculation
	steprg[mx][0] = fabs((maxrg[mx][0] - minrg[mx][0]))/numsteps;
	//printf("linear: min = %g  max = %g  step %g\n",  minrg[mx][1], maxrg[mx][1],  steprg[mx][1]); //debug use only
	//printf("log   : min = %g  max = %g  step %g\n",  minrg[mx][0], maxrg[mx][0],  steprg[mx][0]); //debug use only
	//fflush(stdout); //debug use only
      }
    }
  } // end of first draft of code to support use of dialog box to change min, max, or step generic slider parameters
  genericslider_setparam();     //re-initialize slider  
  ppui.slum->hide();
  m->label(sldtypetitles[m->value()]);
  ppui.view->redraw();
  gensliderchg = 0;
}

void pp_linlog_cb(Fl_Button* o, void *) { 
  int menunum;
  menunum  = ppui.sldtype->value();

  if(ppui.slum->visible() == 1)  {
    o->value(0); //slum slider is log never linear
    o->label("log");
    ppui.slidergroup->redraw();
    return;
  }

    if(o->value() == 1 && loglinsld[menunum] == 2) { //always log so cannot get to 1 = linear state
      o->value(0); //reset to log state
      o->label("log");
      gensliderchg = 0;
      genericslider_setparam(); 
      return;
    }

    if(o->value() == 0  && loglinsld[menunum] == 3) { //always linear so cannot get to 0 = log state
      o->value(1); //reset to linear state
      o->label("lin");
      gensliderchg = 0;
      genericslider_setparam(); 
      return;
    }

    if(loglinsld[menunum] == 1) { //this slider can be toggled between log and linear
      if( o->value() == 1) {
	o->label("lin");
	loglinstate[menunum] = 1;
      }
      else {
	o->label("log");
	loglinstate[menunum] = 0;
      }
      gensliderchg = 0;
      genericslider_setparam(); 
      return;
    }


}

void reset_loglin(int menunum) {
  if(loglinstate[menunum] != ppui.linlog->value()) {
    if( loglinstate[menunum]  ) {
      ppui.linlog->value(1);
      ppui.linlog->label("lin");
    }
    else {
      ppui.linlog->value(0);
      ppui.linlog->label("log"); 
    }
  }
}

void genericslider_initparam() {
  //sets slider ranges, stepsize, and allowable linear and log types
  minrg[ALPHA][1] = 0.0;
  maxrg[ALPHA][1] = 1.0;
  steprg[ALPHA][1] = .01;   

  minrg[FOV][1] = 0.00001;   //fov must be > 0
  maxrg[FOV][1] = 179.449;   //fov must be < 179.45
  steprg[FOV][1] = .1;
  
  minrg[CENSIZE][0] =  -3;
  maxrg[CENSIZE][0] =  log10(10000);
  steprg[CENSIZE][0] = .10;
  minrg[CENSIZE][1] =  0.0;
  maxrg[CENSIZE][1] =  10000.0;
  steprg[CENSIZE][1] = .10;

  minrg[FOCALLEN][0] = -3;
  maxrg[FOCALLEN][0] = 5;
  steprg[FOCALLEN][0] = .1;
  minrg[FOCALLEN][1] = 1;
  maxrg[FOCALLEN][1] = 1000;
  steprg[FOCALLEN][1] = .1;

  minrg[CHROMADEPTHRANGE][0] = -1;
  maxrg[CHROMADEPTHRANGE][0] = 5;
  steprg[CHROMADEPTHRANGE][0] = .01;
  minrg[CHROMADEPTHRANGE][1] = .1;
  maxrg[CHROMADEPTHRANGE][1] = 1000;
  steprg[CHROMADEPTHRANGE][1] = .1;

   
  minrg[LABELMINPIXELS][1]  =  0;
  maxrg[LABELMINPIXELS][1]  =  20;
  steprg[LABELMINPIXELS][1] =  1;

  minrg[LABELSIZE][0] = -3;
  maxrg[LABELSIZE][0] =  log10(1000);
  steprg[LABELSIZE][0] = .01;
  minrg[LABELSIZE][1] = 0.01;
  maxrg[LABELSIZE][1] = 1000.0;
  steprg[LABELSIZE][1] = .01;

  minrg[POLYSIDES][1] =  3;
  maxrg[POLYSIDES][1] =  16;
  steprg[POLYSIDES][1] = 1;

  minrg[POLYSIZE][0] = -3;
  maxrg[POLYSIZE][0] =  log10(10);
  steprg[POLYSIZE][0] = .01;
  minrg[POLYSIZE][1] =  0;
  maxrg[POLYSIZE][1] =  10;
  steprg[POLYSIZE][1] = .01;
 
  for(int i = 0; i < NUMSLD; i++) {
    //loglinsld[i] :  toggle == 1   or   always log == 2  or  always linear ==  3
    if(i == ALPHA)
      loglinsld[i] = 3; //alpha slider always linear
    else if(i == FOV)
      loglinsld[i] = 3; //fov slider always linear
    else if(i == LABELMINPIXELS)
      loglinsld[i] = 3; //labelminpixels slider always linear
    else if(i == POLYSIDES)
      loglinsld[i] = 3; //polysides slider always linear
    else
      loglinsld[i] = 1; //default toggle type

    if(loglinsld[i] == 1 || loglinsld[i] == 2)
      loglinstate[i] = 0;
    else
      loglinstate[i] = 1;
  }     
}
void slider_update(float* p) {
  float v;
  Fl_Value_Slider* o = ppui.genericslider;
  int menunum;
  menunum  = ppui.sldtype->value();
  int log_lin = ppui.linlog->value();

  if(gensliderchg == 1) {
    v =  o->value();
    if(log_lin == 1) 
      *p = o->value();
    else
      *p = pow(10., o->value());
  }
  else  {
    if(log_lin == 1) 
      o->value(*p);
    else {
      if(*p <= 0) 
	o->value(-3);
      else {
	v = log10(*p);
	o->value(v);
      }
    }
  }
}

void slider_update(int* p) {
  float v;
  Fl_Value_Slider* o = ppui.genericslider;
  int menunum;
  menunum  = ppui.sldtype->value();
  int log_lin = ppui.linlog->value();

  if(gensliderchg == 1) {
    v =  o->value();
    if(log_lin == 1) 
      *p = (int)(o->value());
    else
      *p = (int)(pow(10., o->value()));
  }
  else  {
    if(log_lin == 1) 
      o->value(*p);
    else {
      if(*p <= 0) 
	o->value(-3);
      else {
	v = log10(*p);
	o->value(v);
      }
    }
  }
}

static void maketitle( char *str, int menunum, int linear, Fl_Value_Slider *o )
{
    char *p = str;
    if(menunum != CENSIZE && menunum != FOCALLEN)
	p += sprintf(str, "[%.8s] ", parti_get_alias(ppui.st));
    p += sprintf(p, linear ? "%s" : "log(%s)", sldtypetitles[menunum]);
    if(o)
	sprintf(p, " %.4g", linear ? o->value() : pow(10, o->value()));
}

void genericslider_setparam() {
  Fl_Value_Slider* o = ppui.genericslider;
  static int menunum;
  menunum  = ppui.sldtype->value();
  if( (menunum == SLUM) || (menunum < 0) || (menunum >= NUMSLD) || (menunum > ppui.sldtype->size()) ) 
    return;

  struct stuff *st = ppui.st;

  reset_loglin(menunum);
  int log_lin = ppui.linlog->value();
  float *p;
  int   *ip;

  float temp;
  char buf[40];
  
  switch (menunum) {
 
  case ALPHA:
    p = &(ppui.st->alpha);
    slider_update(p);
    break;

  case CENSIZE:
    p = &(ppui.censize);
    slider_update(p); 
    break;

  case CHROMADEPTHRANGE:
    p = &(ppui.st->chromaslidelength);
    slider_update(p);
    break;

  case FOCALLEN:
    {
	float tfocallen = ppui.view->focallen();
	float oldfocallen = tfocallen;
	p = &tfocallen;
	slider_update(p);
	if(tfocallen != oldfocallen)
	    ppui.view->focallen( tfocallen );
    }
    break;

  case FOV:
    temp = parti_fovy(NULL);
    p = &temp;
    slider_update(p);
    if(gensliderchg == 1) {
      sprintf(buf, "%f", *p);
      parti_fovy(buf);
    }
    break;

   case LABELMINPIXELS:
    p = &(ppui.st->textmin);
    slider_update(p); 
    break;

  case LABELSIZE:
    p = &(ppui.st->textsize);
    slider_update(p);
    break;

  case POLYSIDES:
    ip = &(ppui.st->npolygon);
    slider_update(ip);
    break;

  case POLYSIZE:
    p = &(ppui.st->polysize);
    slider_update(p);
    break;

  case SLUM:
    return; //we don't handle slum slider with the generic slider

  default:
    return;
  }//end of switch

  o->range(minrg[menunum][log_lin], maxrg[menunum][log_lin]);
  o->step(steprg[menunum][log_lin]);

  maketitle( sidtitles[menunum], menunum, log_lin, o );

  Fl::visible_focus(1); //marx: version 0.7.02 - don't use the pre fltk-1.1.x old style of only text widgets to get keyboard focus
  o->take_focus();      //so we can use the arrow keys to fine tune the slider

  //==========  start of code for dynamic label =============================
  //idea here is dynamically update label on discrete actions NOT on continuous motion. this will prevent flicker and still provide status info !
  static bool firstdrag = true;
  if( Fl::event_is_click()) {
    firstdrag = true;
  } 
  else if( Fl::event_key(FL_Enter)) {
    firstdrag = true;
  }
  else {
    //must be mouse drag  or keyboard arrow key drag since slider changed value and it wasn't keyboard entered command update or mouse click on slider
    maketitle( sidtitles[menunum], menunum, log_lin, o );
  }
  if(firstdrag) {
      o->label(sidtitles[menunum]);
      Fl::remove_timeout(gensldtimeout);
      Fl::add_timeout(.4, gensldtimeout, o ); //we do not want to draw too many labels when dragging
  }
  //=============  /end of code for dynamic label ===============================
  
  ppui.view->redraw();
  
  Fl::visible_focus(0); //marx: version 0.7.02 - revert to the pre fltk-1.1.x old style of only text widgets to get keyboard focus
}



void gensldtimeout(void* widg) {
  Fl_Value_Slider* o = (Fl_Value_Slider*)widg;
  int menunum =  ppui.sldtype->value(); 
  int log_lin = ppui.linlog->value();
  struct stuff *st = ppui.st;
  maketitle( sidtitles[menunum], menunum, log_lin, o );
  o->label(sidtitles[menunum]);
}
