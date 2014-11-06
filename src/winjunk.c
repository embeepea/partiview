#ifdef WIN32
/*
 * Assorted functions needed for Windows port.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "config.h"

#include "winjunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2) {
  int c1, c2;
  if(s1 == NULL) return 1;
  if(s2 == NULL) return -1;
  for(;;) {
    c1 = *s1++;
    c2 = *s2++;
    if(c1 == '\0')
	return (c2 == '\0' ? 0 : 1);
    if(c2 == '\0')
	return -1;
    if(c1 >= 'A' && c1 <= 'Z') c1 += 'a'-'A';
    if(c2 >= 'A' && c2 <= 'Z') c2 += 'a'-'A';
    if(c1 < c2) return -1;
    if(c1 > c2) return 1;
  }
}


int strncasecmp(const char *s1, const char *s2, int maxlen) {
  int c1, c2;
  int i;
  if(s1 == NULL) return 1;
  if(s2 == NULL) return -1;
  for(i = 0; i < maxlen; i++) {
    c1 = *s1++;
    c2 = *s2++;
    if(c1 == '\0')
	return (c2 == '\0' ? 0 : 1);
    if(c2 == '\0')
	return -1;
    if(c1 >= 'A' && c1 <= 'Z') c1 += 'a'-'A';
    if(c2 >= 'A' && c2 <= 'Z') c2 += 'a'-'A';
    if(c1 < c2) return -1;
    if(c1 > c2) return 1;
  }
  return 0;
}
#endif /*!HAVE_STRCASECMP*/

void srandom(int seed) {
  srand(seed);
}

int random() {
  return rand() << 15 | rand();
}

  /* Assuming x86 Windows, i.e. little-endian */
/* We already have this defined -- no, we don't */
unsigned int my_htonl(unsigned int v) {
    return (v&0xFF)<<24 | (v&0xFF00)<<8 | (v>>8)&0xFF00 | (v>>24)&0xFF;
}
unsigned int my_ntohl(unsigned int v) {
    return (v&0xFF)<<24 | (v&0xFF00)<<8 | (v>>8)&0xFF00 | (v>>24)&0xFF;
}

#include <math.h>
#include <float.h>

#if !defined(AR_USE_MINGW)  // was !defined(HAVE_RINT)
double rint(double v) {
    return floor(v + 0.5);
}

#include <sys/types.h>
#include <sys/stat.h>
int stat( char *fname, struct stat *st )
{
    return _stat(fname, st);
}

#endif /*!HAVE_RINT*/


#ifndef HAVE_SNPRINTF
/* Use FLTK's substitute vsnprintf since Win32 lacks any of its own */
extern int fl_vsnprintf(char* str, size_t size, const char* fmt, va_list ap);

int snprintf(char* str, size_t size, const char* fmt, ...) {
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = fl_vsnprintf(str, size, fmt, ap);
  va_end(ap);
  return ret;
}
#endif /*!HAVE_SNPRINTF*/

#endif /*WIN32*/
