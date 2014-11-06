#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <GL/glut.h>

#include "flhack.H"
#include "Gview.H"
#include "partiview.H"
#include "geometry.h"
#include "shmem.h"
#include "pview_funcs.h"

extern "C" {
#include "findfile.h"
};

int wwidth = 640;
int wheight = 480;
int wposx = 100;
int wposy = 100;
int isFullScreen = false;

extern char partiview_version[];

void display()
{
    ppui.view->redraw(true);

    glutSwapBuffers();
}

void reshape (int _w,int _h)
{
    if (!isFullScreen) {
        wwidth = _w;
        wheight = _h;
    }

    ppui.view->size(_w,_h);
}

void mouseFunc(int _button,int _state,int _x,int _y)
{
#ifdef MACINTOSH
    if (_button == GLUT_LEFT_BUTTON && glutGetModifiers())
        _button = GLUT_RIGHT_BUTTON;
#endif

    Fl::state = 0;
    if (_button == GLUT_LEFT_BUTTON) {
        Fl::state = FL_BUTTON1;
        Fl::button = FL_LEFT_MOUSE;
    }
    if (_button == GLUT_MIDDLE_BUTTON) {
        Fl::state = FL_BUTTON2;
        Fl::button = FL_MIDDLE_MOUSE;
    }
    if (_button == GLUT_RIGHT_BUTTON) {
        Fl::state = FL_BUTTON3;
        Fl::button = FL_RIGHT_MOUSE;
    }
    if (glutGetModifiers() == GLUT_ACTIVE_SHIFT)
        Fl::state += FL_SHIFT;
    if (glutGetModifiers() == GLUT_ACTIVE_CTRL)
        Fl::state += FL_CTRL;
    Fl::x = _x;
    Fl::y = _y;

    if (_state == GLUT_DOWN)
        ppui.view->handle(FL_PUSH);
    else {
        Fl::state = 0;
        ppui.view->handle(FL_RELEASE);
        Fl::button = 0;
    }
}

void mouseMotion(int _x,int _y)
{
    Fl::x = _x;
    Fl::y = _y;

    ppui.view->handle(FL_DRAG);
}

void special(int _key,int _x,int _y)
{
    Fl::keys[0] = _key;
    Fl::keys[1] = '\0';
    Fl::x = _x;
    Fl::y = _y;

    if (glutGetModifiers() == GLUT_ACTIVE_SHIFT)
        Fl::state += FL_SHIFT;
    if (glutGetModifiers() == GLUT_ACTIVE_CTRL)
        Fl::state += FL_CTRL;

    ppui.view->handle(FL_KEYBOARD);

    if (_key == GLUT_KEY_F1) {
        fprintf(stdout, "> ");
        char line[1024];
        gets(line);
        pviewEval(line);
    }
    if (_key == GLUT_KEY_F11) {
        if (isFullScreen) {
            isFullScreen = false;
            glutPositionWindow(wposx, wposy);
            glutReshapeWindow(wwidth, wheight);
        } else {
            isFullScreen = true;
            glutFullScreen();
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char _key,int _x,int _y)
{
    Fl::keys[0] = _key;
    Fl::keys[1] = '\0';
    Fl::x = _x;
    Fl::y = _y;

    if (glutGetModifiers() == GLUT_ACTIVE_SHIFT)
        Fl::state += FL_SHIFT;
    if (glutGetModifiers() == GLUT_ACTIVE_CTRL)
        Fl::state += FL_CTRL;

    ppui.view->handle(FL_KEYBOARD);

    glutPostRedisplay();
}

void idle(void)
{
    Fl::idle();

    if (pviewPicked()) {
        fprintf(stdout, "%s\n", _pickmsg);
        strcpy(_pickmsg, "");
    }

    glutPostRedisplay();
}

int main(int _argc,char **_argv)
{
    glutInit(&_argc,_argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(wwidth, wheight);
    glutInitWindowPosition(wposx, wposy);
    glutCreateWindow(_argv[0]);

    fprintf(stdout, "Welcome to Partiview ver.%s : glut version.\n", partiview_version);
    fprintf(stdout, "Hit [F1] key to enter a command.\n\n");

    // initialize partiview
    char* initarg = NULL;
    if (_argc>1)
        initarg = _argv[1];
    pviewInit(initarg);
    pviewEval("inertia on");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseFunc);
    glutMotionFunc(mouseMotion);

    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutIdleFunc(idle);

    glutMainLoop();

    return 0;
}
