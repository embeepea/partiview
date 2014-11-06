#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "flhack.H"

int Fl::x = 0;
int Fl::y = 0;
int Fl::x_root = 0;
int Fl::y_root = 0;
long Fl::state = 0;
long Fl::button = 0;
char Fl::keys[256];
FL_CB_FUNC Fl::cb = NULL;
void *Fl::cb_data = NULL;

Fl::Fl(void)
{
    x = 0;
    y = 0;
    x_root = 0;
    y_root = 0;
    state = 0;
    strcpy(keys,"");

    cb = NULL;
    cb_data = NULL;
}

Fl::~Fl(void)
{
}

int Fl::event_x(void)
{
    return x;
}

int Fl::event_y(void)
{
    return y;
}

int Fl::event_x_root(void)
{
    return x;
}

int Fl::event_y_root(void)
{
    return y;
}

int Fl::check(void)
{
    return 0;
}

unsigned int Fl::event_state(unsigned long _state)
{
    return ((state & _state) != 0);
}

unsigned int Fl::event_button(void)
{
    return button;
}

char *Fl::event_text(void)
{
    return keys;
}

void Fl::add_idle(FL_CB_FUNC _cb,void *_cb_data)
{
    cb = _cb;
    cb_data = _cb_data;
}

void Fl::remove_idle(FL_CB_FUNC _cb,void *_cb_data)
{
    cb = NULL;
    cb_data = NULL;
}

void Fl::idle(void)
{
    if (cb)
        cb(cb_data);
}

void Fl::add_timeout( double, FL_CB_FUNC, void * )
{
}

void Fl::remove_timeout( FL_CB_FUNC, void * )
{
}

void Fl::nextEvent(void)
{
    state = 0;
}

void Fl::warning(const char *_message, ...)
{
    char message[256];
    va_list args;
    va_start(args,_message);

    vsprintf(message,_message,args);
}

Fl_Gl_Window::Fl_Gl_Window(int _x,int _y,int _w,int _h,const char *_label)
{
    posx = _x;
    posy = _y;
    width = _w;
    height = _h;
    mode_flag = (Fl_Mode)(FL_RGB8|FL_DOUBLE|FL_ALPHA|FL_DEPTH);
    valid_flag = 0;
    swap_func = NULL;
}

void Fl_Gl_Window::resize(int _x,int _y,int _w,int _h)
{
    posx = _x;
    posy = _y;
    width = _w;
    height = _h;
}

void Fl_Gl_Window::position(int _x,int _y)
{
    posx = _x;
    posy = _y;
}

void Fl_Gl_Window::size(int _w,int _h)
{
    width = _w;
    height = _h;
}

int Fl_Gl_Window::x_root(void)
{
    return 0;
}

int Fl_Gl_Window::y_root(void)
{
    return 0;
}

int Fl_Gl_Window::x(void)
{
    return posx;
}

int Fl_Gl_Window::y(void)
{
    return posy;
}

int Fl_Gl_Window::w(void)
{
    return width;
}

int Fl_Gl_Window::h(void)
{
    return height;
}

void Fl_Gl_Window::mode(int _mode)
{
    mode_flag = (Fl_Mode)_mode;
}

void Fl_Gl_Window::mode(int *_mode)
{
    mode_flag = (Fl_Mode)_mode[0];
}

int Fl_Gl_Window::mode(void)
{
    return mode_flag;
}

void Fl_Gl_Window::valid(int _valid)
{
    valid_flag = _valid;
}

int Fl_Gl_Window::valid(void)
{
    return valid_flag;
}

void Fl_Gl_Window::set_swap_func(FL_SWAP_FUNC _swap_func)
{
    swap_func = _swap_func;
}

extern "C"
{
int fl_vsnprintf(char* str, size_t size, const char* fmt, va_list ap)
{
    const char* e = str+size-1;
    char* p = str;
    char copy[20];
    char* copy_p;
    char sprintf_out[100];

    while (*fmt && p < e) {
        if (*fmt != '%') {
            *p++ = *fmt++;
        } else {
            fmt++;
            copy[0] = '%';
            for (copy_p = copy+1; copy_p < copy+19;) {
                switch ((*copy_p++ = *fmt++)) {
                case 0:
                fmt--; goto CONTINUE;
                case '%':
                *p++ = '%'; goto CONTINUE;
                case 'c':
                *p++ = va_arg(ap, int);
                goto CONTINUE;
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                *copy_p = 0;
                sprintf(sprintf_out, copy, va_arg(ap, int));
                copy_p = sprintf_out;
                goto DUP;
                case 'e':
                case 'E':
                case 'f':
                case 'g':
                *copy_p = 0;
                sprintf(sprintf_out, copy, va_arg(ap, double));
                copy_p = sprintf_out;
                goto DUP;
                case 'p':
                *copy_p = 0;
                sprintf(sprintf_out, copy, va_arg(ap, void*));
                copy_p = sprintf_out;
                goto DUP;
                case 'n':
                *(va_arg(ap, int*)) = p-str;
                goto CONTINUE;
                case 's':
                copy_p = va_arg(ap, char*);
                if (!copy_p) copy_p = "NULL";
                DUP:
                while (*copy_p && p < e) *p++ = *copy_p++;
                goto CONTINUE;
                }
            }
        }
        CONTINUE:;
    }
    *p = 0;
    if (*fmt) return -1;
    return p-str;
}
};
