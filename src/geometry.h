#ifndef GEOMETRY_H
#define GEOMETRY_H
/*
 * 3-D geometry (matrix, vector, quaternion) functions for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONST
# ifdef __cplusplus
#  define CONST const
# else
#  define CONST
# endif
#endif

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
extern void *alloca( int );
#   endif
#  endif
# endif
#endif

#ifdef __cplusplus
# define NewA(type, count)  (static_cast<type *>(alloca((count) * sizeof(type))))
#else
# define NewA(type, count)  ( (type *)alloca((count) * sizeof(type)) )
#endif

#ifndef HAVE_SQRTF
#  define sqrtf(x)	sqrt(x)
#  define tanf(x)	tan(x)
#  define sinf(x)	sin(x)
#  define cosf(x)	cos(x)
#  define atan2f(y,x)	atan2(y,x)
#  define hypotf(x,y)	hypot(x,y)
#endif

#ifndef COUNT
# define COUNT(array)  (sizeof(array) / sizeof((array)[0]))
#endif


typedef struct { float x[3]; } Point;
typedef struct { float q[4]; } Quat;  /* {r,i,j,k} => {0,1,2,3} */

typedef struct { float m[4*4]; } Matrix;

/*
 * Global Constants
 */

extern Matrix Tidentity;


/*
 * Global variables
 */

void arena_init( int nbytes );

void vsub( Point *dst, CONST Point *a, CONST Point *b );
void vadd( Point *dst, CONST Point *a, CONST Point *b );
void vcross( Point *dst, CONST Point *a, CONST Point *b );
float vdot( CONST Point *a, CONST Point *b );
void vscale( Point *dst, float s, CONST Point *src );
void vsadd( Point *dst, CONST Point *a, float sb, CONST Point *b );
void vlerp( Point *dst, float frac, CONST Point *vfrom, CONST Point *vto );
void vcomb( Point *dst, float sa, CONST Point *a, float sb, CONST Point *b );
float vunit( Point *dst, CONST Point *src );
void vproj( Point *along, Point *perp, CONST Point *vec, CONST Point *onto );

float qdot( CONST Quat *q1, CONST Quat *q2 );
void qnorm( Quat *qdst, CONST Quat *qsrc );
void qcomb( Quat *qdst, float sa, CONST Quat *qa, float sb, CONST Quat *qb );

	/* Distance between two 3-D points */
float vdist( CONST Point *p1, CONST Point *p2 );
	/* Distance between two quaternions */
float iqdist( CONST Point *q1, CONST Point *q2 );
float qdist( CONST Quat *q1, CONST Quat *q2 );
	/* Distance between two matrices */
float tdist( CONST Matrix *t1, CONST Matrix *t2 );

	/* vector magnitude */
float vlength( CONST Point *v );

	/* Transform a 3-D point by a matrix: dst = src*T. */
void vtfmpoint( Point *dst,  CONST Point *src, CONST Matrix *T );

	/* Transform a vector by a (Euclidean) matrix: dst = src*T */
void vtfmvector( Point *dst,  CONST Point *src, CONST Matrix *T );
void vuntfmvector( Point *dst,  CONST Point *src, CONST Matrix *T );

	/* Get translation part of matrix */
void vgettranslation( Point *dst, CONST Matrix *T );
	/* Set translation part of matrix, leave 3x3 submatrix alone */
void vsettranslation( Matrix *T, CONST Point *src );

void vrotxy( Point *dst, CONST Point *src, CONST float cs[2] );
void mcopy( Matrix *dst, CONST Matrix *src );

	/* 4x4 matrix multiply */
void mmmul( Matrix *dst, CONST Matrix *a, CONST Matrix *b );

	/* Matrix inverse for Euclidean similarities: matrices are
	 * a product of pure translation/rotation/uniform-scaling --
	 */
void eucinv( Matrix *dst, CONST Matrix *src );

	/* quickie determinant of 3x3 submatrix */
float mdet3( CONST Matrix *T );

	/* matrix conjugation: see its use in Gview.C */
void mconjugate( Matrix *To2wout, CONST Matrix *To2win, CONST Matrix *Tincrf,
		 CONST Matrix *Tf2w, CONST Matrix *Tw2f,
		 CONST Point *pcenw, CONST Point *pcenf );

void grotation( Matrix *Trot, CONST Point *fromaxis, CONST Point *toaxis );

	/* matrix to 3-component quaternion (real-part is sqrt(1-iquat.iquat),
	 * and real part (in range 0 .. +1) is also returned as func value.
	 */
float tfm2iquat( Point *iquat, CONST Matrix *src );	/* returns real part */
void tfm2quat( Quat *quat, CONST Matrix *src );
void iquat2tfm( Matrix *dst, CONST Point *iquat );
void quat2tfm( Matrix *dst, CONST Quat *quat );

	/* quaternion interpolation */
void iquat_lerp( Point *dquat, float frac, CONST Point *qfrom, CONST Point *qto );
void quat_lerp( Quat *dquat, float frac, CONST Quat *qfrom, CONST Quat *qto );

	/* axis/angle rotation -> quaternion */
void rot2quat( Quat *quat, float degrees, CONST Point *axis );
void rot2iquat( Point *iquat, float degrees, CONST Point *axis );

	/* construct matrix from axis/angle rotation */
void rot2tfm( Matrix *dst, float degrees, CONST Point *axis );

	/* Construct a matrix that's a pure X/Y/Z rotation/scaling/translation */
void mrotation( Matrix *dst, float degrees, char xyzaxis );
void mscaling( Matrix *dst, float sx, float sy, float sz );
void mtranslation( Matrix *dst, float tx, float ty, float tz );

	/* cam2world <-> az(Y) el(X) roll(Z) + translation */
float tfm2xyzaer( Point *xyz, float aer[3], CONST Matrix *c2w ); /* => scale */
void xyzaer2tfm( Matrix *c2w, CONST Point *xyz, CONST float aer[3] );

	/* perspective frustum */
void mfrustum( Matrix *Tproj, float nl, float nr, float nd, float nu, float cnear, float cfar );


#ifdef __cplusplus
}
#endif


#endif
