#ifndef _FLHACK_H_
#define _FLHACK_H_

#ifdef WIN32
#include "winjunk.h"
#endif

#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

enum Fl_Event { // events
    FL_NO_EVENT = 0,
    FL_PUSH     = 1,
    FL_RELEASE  = 2,
    FL_ENTER        = 3,
    FL_LEAVE        = 4,
    FL_DRAG     = 5,
    FL_FOCUS        = 6,
    FL_UNFOCUS  = 7,
    FL_KEYBOARD = 8,
    FL_CLOSE        = 9,
    FL_MOVE     = 10,
    FL_SHORTCUT = 11,
    FL_DEACTIVATE   = 13,
    FL_ACTIVATE = 14,
    FL_HIDE     = 15,
    FL_SHOW     = 16,
    FL_PASTE        = 17,
    FL_SELECTIONCLEAR   = 18
};

enum Fl_Mode { // visual types and Fl_Gl_Window::mode() (values match Glut)
  FL_RGB	= 0,
  FL_INDEX	= 1,
  FL_SINGLE	= 0,
  FL_DOUBLE	= 2,
  FL_ACCUM	= 4,
  FL_ALPHA	= 8,
  FL_DEPTH	= 16,
  FL_STENCIL	= 32,
  FL_RGB8	= 64,
  FL_MULTISAMPLE= 128
};

// Fl::event_button():
#define FL_LEFT_MOUSE   1
#define FL_MIDDLE_MOUSE 2
#define FL_RIGHT_MOUSE  3

// Fl::event_state():
#define FL_SHIFT       0x00010000
#define FL_CAPS_LOCK   0x00020000
#define FL_CTRL        0x00040000
#define FL_ALT         0x00080000
#define FL_NUM_LOCK    0x00100000 // most X servers do this?
#define FL_META        0x00400000 // correct for XFree86
#define FL_SCROLL_LOCK 0x00800000 // correct for XFree86
#define FL_BUTTON1     0x01000000
#define FL_BUTTON2     0x02000000
#define FL_BUTTON3     0x04000000

typedef void (*FL_CB_FUNC)(void *);
typedef void (*FL_SWAP_FUNC)(void);

class Fl
{
  public:

    Fl(void);
    ~Fl(void);

    static int event_x(void);
    static int event_y(void);
    static int event_x_root(void);
    static int event_y_root(void);

    static int check(void);

    static unsigned int event_state(unsigned long _state);
    static unsigned int event_button(void);

    static char *event_text(void);

    static void add_idle(void (*_cb)(void *),void *_cb_data);
    static void remove_idle(FL_CB_FUNC _cb,void *_cb_data);
    static void add_timeout( double , void (*)(void *), void * );
    static void remove_timeout( void (*)(void *), void * );

    static void idle(void);
    static void nextEvent(void);

    static void warning(const char *, ...);

    static void wait(double _time) { }

    static int w() { return 1024; }
    static int h() { return 768; }


  public:

    static int x;
    static int y;
    static int x_root;
    static int y_root;
    static long state;
    static long button;
    static char keys[256];
    static FL_CB_FUNC cb;
    static void *cb_data;
};

class Fl_Gl_Window
{
  public:

    Fl_Gl_Window(int _x,int _y,int _w,int _h,const char *_label);

    virtual void draw(void) { }
    virtual void redraw(int _force=FALSE) { if (_force) draw(); if (swap_func) swap_func(); }
    virtual void end(void) { }
    virtual void resize(int _x,int _y,int _w,int _h);
    virtual void size(int _w,int _h);
    virtual void position(int _x,int _y);
    virtual int x_root(void);
    virtual int y_root(void);
    virtual int x(void);
    virtual int y(void);
    virtual int w(void);
    virtual int h(void);
    virtual void mode(int _mode);
    virtual void mode(int *_mode);
    virtual int mode(void);
    virtual void valid(int _valid);
    virtual int  valid(void);
    virtual int damage(void) { return 1; }
    virtual int children(void) { return 0; }
    virtual void make_current(void) { }
    virtual void take_focus(void) { }
    virtual bool visible_r(void) { return true; }
    virtual void set_swap_func(FL_SWAP_FUNC _swap_func);

    virtual Fl_Gl_Window *parent(void) { return 0; }
    virtual void show(void) { }

  protected:

    int posx;
    int posy;
    int width;
    int height;
    int valid_flag;
    Fl_Mode mode_flag;
    FL_SWAP_FUNC swap_func;
};

#endif /* _FLHACK_H_ */
