#ifndef _PVIEW_FUNCS_H_
#define _PVIEW_FUNCS_H_

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include <GL/gl.h>
#include "flhack.h"
#include "Gview.H"
#include "partiview.H"
#include "geometry.h"
#include "shmem.h"
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "findfile.h"

void pviewInit(const char *_filename="");
void pviewEval(const char *_command);
bool pviewPicked(void);
extern char _pickmsg[1024];

#ifdef EMBEDHACK
bool pviewMessage(void);
#include <string>
extern std::string _pviewmsg;
#endif //EMBEDHACK

#endif
