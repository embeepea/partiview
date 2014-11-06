#ifndef SFONT_H
#define SFONT_H
/*
 * Hershey vector fonts.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include "geometry.h"	/* for Point, CONST, COUNT() */

#ifdef __cplusplus
extern "C" {
#endif

extern float sfStrWidth( CONST char *str );	/* width assuming height=1.0 */

extern float sfStrDraw( CONST char *str, float height, CONST Point *base );
extern float sfStrDrawTJ( CONST char *str, float height, CONST Point *base,
					CONST Matrix *tfm, CONST char *just );

#ifdef __cplusplus
}
#endif

#endif /*SFONT_H*/
