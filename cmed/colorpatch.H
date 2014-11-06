#ifndef _COLORPATCH_H
#define _COLORPATCH_H

#include "cmedpanel.H"

class colorpatch : public Fl_Gl_Window {
  public:

  float vr,vg,vb,va;

  void rgba(float r,float g,float b,float a) { vr=r,vg=g,vb=b,va=a; redraw(); }

  colorpatch(int x,int y,int w,int h,char *label=0) :
	Fl_Gl_Window(x,y,w,h,label) {
    vr = vg = vb = va = 0;
  }

 protected:

  virtual void draw();
};

#endif
