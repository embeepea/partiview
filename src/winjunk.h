#ifndef WINJUNK_H
#define WINJUNK_H
#ifdef WIN32
/*
 * Assorted functions needed for Windows port.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#define  WINDOWS_LEAN_AND_MEAN	1	/* ugh */
#include <windows.h>
#include <malloc.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This was taken out because it was throwing compiler errors of
 * redefining htonl, even though we say #undef. Not sure if it's
 * important right now.
 *
 */
#undef htonl
#undef ntohl
#define htonl(x)  my_htonl(x)
#define ntohl(x)  my_ntohl(x)
extern unsigned int my_htonl(unsigned int v);
extern unsigned int my_ntohl(unsigned int v);

#ifndef HAVE_STRCASECMP
  extern int strcasecmp(const char *s1, const char *s2);
  extern int strncasecmp(const char *s1, const char *s2, int maxlen);
#endif

#ifndef HAVE_SNPRINTF
  /* we'll use FLTK's fl_vsnprintf since non-MINGW Win32 lacks it */
  extern int snprintf(char *dest, int maxlen, const char *fmt, ...);
#endif

#ifndef HUGE
  #define  HUGE		FLT_MAX
#endif
#ifndef M_LN10
  #define  M_LN10	2.30258509299404568402
#endif
#ifndef M_PI
  #define  M_PI		3.14159265358979323846
#endif

#define finite(x)	_finite(x)

extern void srandom(int seed);
extern int random();

#ifndef HAVE_RINT
extern double rint(double);
#endif


#ifndef HAVE_SQRTF
#define sqrtf(_) sqrt(_)
#define sinf(_)  sin(_)
#define cosf(_)  cos(_)
#define tanf(_)  tan(_)
#define expf(_)  exp(_)
#define log10f(_) log10(_)
#define atan2f(_,__) atan2(_,__)
#define hypotf(_,__) hypot(_,__)
#define	fabsf(_)  fabs(_)
#endif /*!HAVE_SQRTF*/

#define _NO_OLDNAMES
#include <io.h>		/* for access() */
#ifndef R_OK
# define R_OK 004	/* No Windows include-file defines this?! */
#endif

#define access(fname, mode)   _access(fname, mode)

#ifdef __cplusplus
}	/* end extern "C" */
#endif /*__cplusplus__*/

#endif /*WIN32*/
#endif /*WINJUNK_H*/
