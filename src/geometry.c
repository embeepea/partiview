/*
 * 3-D geometry (matrix, vector, quaternion) functions for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef sgi
#include <sys/endian.h>
#endif

#ifdef WIN32
# include "winjunk.h"
#endif

#include "geometry.h"

Matrix Tidentity = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

void vrotxy( register Point *dst, CONST Point *src, CONST float cs[2] )
{
   dst->x[0] = src->x[0]*cs[0] - src->x[1]*cs[1];
   dst->x[1] = src->x[0]*cs[1] + src->x[1]*cs[0];
   dst->x[2] = src->x[2];
}

#define	VDOT(a, b) \
	((a)->x[0]*(b)->x[0] + (a)->x[1]*(b)->x[1] + (a)->x[2]*(b)->x[2])

#define VROTXY(dst, src, cs) \
	(dst)->x[0] = (src)->x[0]*cs[0] - (src)->x[1]*cs[1], \
	(dst)->x[1] = (src)->x[0]*cs[1] + (src)->x[1]*cs[0], \
	(dst)->x[2] = (src)->x[2]

#define	VMID(dst, a, b) \
	(dst)->x[0] = .5*((a)->x[0] + (b)->x[0]), \
	(dst)->x[1] = .5*((a)->x[1] + (b)->x[1]), \
	(dst)->x[2] = .5*((a)->x[2] + (b)->x[2])

void vadd( register Point *dst, CONST Point *a, CONST Point *b )
{
   dst->x[0] = a->x[0] + b->x[0];
   dst->x[1] = a->x[1] + b->x[1];
   dst->x[2] = a->x[2] + b->x[2];
}
void vsub( register Point *dst, CONST Point *a, CONST Point *b )
{
   dst->x[0] = a->x[0] - b->x[0];
   dst->x[1] = a->x[1] - b->x[1];
   dst->x[2] = a->x[2] - b->x[2];
}

void vcross( register Point *dst, register CONST Point *a, register CONST Point *b )
{
   dst->x[0] = a->x[1]*b->x[2] - a->x[2]*b->x[1];
   dst->x[1] = a->x[2]*b->x[0] - a->x[0]*b->x[2];
   dst->x[2] = a->x[0]*b->x[1] - a->x[1]*b->x[0];
}


float vdot( CONST Point *a, CONST Point *b ) {
   return a->x[0]*b->x[0] + a->x[1]*b->x[1] + a->x[2]*b->x[2];
}

void vscale( register Point *dst, float s, register CONST Point *src )
{
   dst->x[0] = s*src->x[0];
   dst->x[1] = s*src->x[1];
   dst->x[2] = s*src->x[2];
}

void vsadd( Point *dst, CONST Point *a, float sb, CONST Point *b )
{
   dst->x[0] = a->x[0] + sb * b->x[0];
   dst->x[1] = a->x[1] + sb * b->x[1];
   dst->x[2] = a->x[2] + sb * b->x[2];
}

float vdist( CONST Point *p1, CONST Point *p2 ) {
   Point d;
   vsub(&d, p1, p2);
   return vlength(&d);
}

float vlength( CONST Point *v ) {
   return (float)sqrtf(VDOT(v, v));
}

float vunit( register Point *dst, register CONST Point *src ) {
   float s = (float)sqrtf(VDOT(src, src));
   float scl = s>0 ? 1.0f / s : 0;
   vscale( dst, scl, src );
   return s;
}

/* along = onto * (vec . onto / onto . onto)
 * perp  = vec - along
 */
void vproj( Point *along, Point *perp, CONST Point *vec, CONST Point *onto ) {
    float mag2 = VDOT(onto, onto);
    float dot = VDOT(vec, onto);
    float s = (mag2 > 0) ? dot / mag2 : 0;
    Point talong;
    if(along == NULL) along = &talong;
    vscale( along, s, onto );
    if(perp != NULL)
	vsadd( perp, vec, -1, along );
}

/*
 * vtfmvector() transforms a vector (in homog coords, [x,y,z,0]) by a matrix.
 * vtfmpoint() transforms a point  [x,y,z,1].
 * The difference is that vtfmpoint() includes the matrix's translation part
 * and vtfmvector() doesn't.
 */

void vuntfmvector( Point *dst, register CONST Point *src, register CONST Matrix *T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T->m[4*i] + src->x[1]*T->m[4*i+1] + src->x[2]*T->m[4*i+2];
}
void vtfmvector( Point *dst, register CONST Point *src, register CONST Matrix *T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T->m[i] + src->x[1]*T->m[i+4] + src->x[2]*T->m[i+8];
}
void vtfmpoint( Point *dst, register CONST Point *src, register CONST Matrix *T )
{
  int i;
  for(i = 0; i < 3; i++)
    dst->x[i] = src->x[0]*T->m[i] + src->x[1]*T->m[i+4] + src->x[2]*T->m[i+8] + T->m[i+12];
}

void vgettranslation( Point *dst, CONST Matrix *T )
{
  memcpy(dst->x, &T->m[4*3+0], 3*sizeof(float));
}

void vsettranslation( Matrix *T, CONST Point *src )
{
  memcpy(&T->m[4*3+0], src->x, 3*sizeof(float));
}

/* Invert a matrix, assuming it's a Euclidean isometry
 * plus possibly uniform scaling.
 */
void eucinv( Matrix *dst, CONST Matrix *src )
{
  int i, j;
  float s = VDOT((Point *)src, (Point *)src);
  Point trans;
  Matrix T;
  if(src == dst) {
    T = *src;
    src = &T;
  }
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 3; j++)
	dst->m[i*4+j] = src->m[j*4+i] / s;
    dst->m[i*4+3] = 0;
  }
  vtfmvector( &trans, (Point *)&src->m[4*3+0], dst );
  vscale( (Point *)&dst->m[3*4+0], -1, &trans );
  dst->m[3*4+3] = 1;
}

void mcopy( Matrix *dst, CONST Matrix *src )
{
  memcpy( dst, src, sizeof(Matrix) );
}

void mmmul( Matrix *dst, CONST Matrix *a, CONST Matrix *b )
{
  int i, irow, j;
  Matrix tmp;
  if(dst == a || dst == b) {
    mmmul( &tmp, a, b );
    *dst = tmp;
    return;
  }
  for(i = 0; i < 4; i++) {
    irow = i*4;
    for(j = 0; j < 4; j++)
	dst->m[irow+j] = a->m[irow]*b->m[j] + a->m[irow+1]*b->m[1*4+j]
		       + a->m[irow+2]*b->m[2*4+j] + a->m[irow+3]*b->m[3*4+j];
  }
}

/* Construct matrix for geodesic rotation from "a" to "b".
 */
void grotation( Matrix *Trot, CONST Point *va, CONST Point *vb )
{
  Point a, b, aperp;
  float ab_1, apb;
  int i, j;

  mcopy( Trot, &Tidentity );
  if(vunit(&a, va) == 0 || vunit(&b, vb) == 0)
    return;
  ab_1 = VDOT(&a,&b) - 1;
  vproj( NULL, &aperp, &b, &a );
  if(vunit(&aperp, &aperp) == 0) {
    if(ab_1 >= -1)
	return;		/* Vectors are identical: no rotation */

    /* Otherwise, vectors are oppositely directed.
     * Rotate in an arbitrary plane which includes them.
     */
    aperp.x[ fabs(a.x[0]) < .7 ? 0 : 1 ] = 1;
    vproj( NULL, &aperp, &aperp, &a );
    vunit(&aperp, &aperp);
  }
  apb = VDOT(&aperp, &b);
  for(i = 0; i < 3; i++) {
    float acoef = a.x[i]*ab_1 - aperp.x[i]*apb;
    float apcoef = aperp.x[i]*ab_1 + a.x[i]*apb;
    for(j = 0; j < 3; j++)
	Trot->m[i*4+j] += a.x[j]*acoef + aperp.x[j]*apcoef;
  }
}

/*
 * Conjugate a transformation as needed for interactive positioning,
 * applying it in a given coordinate frame, and conjugating rotations
 * to fix a given point.
 * Coord abbreviations:  "o" (object) "w" (world) "f" (frame) "cen" center.
 * Tf2w and Tw2f are frame-to-world transform and its inverse.
 *  (Either may be NULL, in which case it's computed from the other.)
 *  (If both are NULL, we assume frame == world.)
 * Rotation-fixing point is given by either:
 *  pcenw -- the point in world coordinates, if non-NULL, or
 *  pcenf -- the point in frame coordinates, if non-NULL.
 * If both are NULL, the point is taken to be (0,0,0) in frame coordinates.
 */
void mconjugate( Matrix *To2wout, CONST Matrix *To2win, CONST Matrix *Tincrf,
		CONST Matrix *Tf2w, CONST Matrix *Tw2f,
		CONST Point *pcenw, CONST Point *pcenf )
{
  Matrix t_incrf, t_f2w, t_w2f;
  Matrix t1, t2;
  Point pt1, pt2, tp_cenf;

  if(Tf2w == NULL && Tw2f != NULL) {
    Tf2w = &t_f2w;
    eucinv((Matrix *)Tf2w, Tw2f);
  } else if(Tw2f == NULL && Tf2w != NULL) {
    Tw2f = &t_w2f;
    eucinv((Matrix *)Tw2f, Tf2w);
  }
  if(pcenf == NULL && pcenw != NULL) {
    if(Tw2f != NULL) {
	vtfmpoint(&tp_cenf, pcenw, Tw2f);
	pcenf = &tp_cenf;
    } else {
	pcenf = pcenw;
    }
  }
  if(pcenf != NULL) {
    t_incrf = *Tincrf;
    vtfmvector( &pt1, pcenf, &t_incrf );
    vsub( &pt1, pcenf, &pt1 );
    vgettranslation( &pt2, &t_incrf );
    vadd( &pt1, &pt1, &pt2 );
    vsettranslation( &t_incrf, &pt1 );
    Tincrf = &t_incrf;
  }

  if(Tf2w != NULL) {
    mmmul( &t1, Tw2f, Tincrf );
    mmmul( &t2, &t1,         Tf2w );
    mmmul( To2wout, To2win, &t2 );
    //fprintf(stderr,"mconj dets: Tw2f %g Tincrf %g Tf2w %g To2win %g To2wout %g\n",
    //	    mdet3(Tw2f),mdet3(Tincrf),mdet3(Tf2w),mdet3(To2win),mdet3(Tf2w),mdet3(To2wout));
  } else {
    mmmul( To2wout, To2win, Tincrf );
  }
}

void mrotation( Matrix *rot, float degrees, char xyz ) {
  float s = sinf(degrees * (M_PI/180));
  float c = cosf(degrees * (M_PI/180));
  int a = (xyz - 'x' + 1) % 3;
  int b = (xyz - 'x' + 2) % 3;

  *rot = Tidentity;
  rot->m[a*4+a] = c;
  rot->m[a*4+b] = s;
  rot->m[b*4+a] = -s;
  rot->m[b*4+b] = c;
}

void mscaling( Matrix *scale, float sx, float sy, float sz ) {
  *scale = Tidentity;
  scale->m[0*4+0] = sx;
  scale->m[1*4+1] = sy;
  scale->m[2*4+2] = sz;
}

void mtranslation( Matrix *tran, float tx, float ty, float tz ) {
  *tran = Tidentity;
  tran->m[3*4+0] = tx;
  tran->m[3*4+1] = ty;
  tran->m[3*4+2] = tz;
}


/*
 * Convert the rotation part of a Euclidean isometry+uniform-scaling matrix
 * into a unit quaternion, with non-negative real part.
 * Returns the real part of the quaternion, with the three imaginary components
 * left in iquat->x[0,1,2].
 */
float tfm2iquat( Point *iquat, CONST Matrix *T )
{
  float mag, sinhalf, trace;
  float scl = vlength((Point *)T);	/* gauge scaling from 1st row */
  Point axis;

#define Tij(i,j) T->m[(i)*4+(j)]

  trace = scl==0 ? 3 : (Tij(0,0) + Tij(1,1) + Tij(2,2))/scl; /*1 + 2*cos(ang)*/
  if(trace<-1) trace = -1; else if(trace > 3) trace = 3;
  sinhalf = sqrtf(3 - trace) / 2;		/* sin(angle/2) */

  axis.x[0] = Tij(1,2) - Tij(2,1);
  axis.x[1] = Tij(2,0) - Tij(0,2);
  axis.x[2] = Tij(0,1) - Tij(1,0);
  if(trace < -.25) {
    /* Angle near pi; sin(angle) is small, so use cos-related elements */
    float c = (trace-1)/2;	/* cos(angle) */
    float v = 1-c;		/* versine(angle) */

    if(Tij(0,0) > c+.5) {		/* large x component */
	axis.x[0] = sqrtf((Tij(0,0)-c)/v) * (axis.x[0]<0 ? -1 : 1);
	axis.x[1] = (Tij(0,1)+Tij(1,0)) / (2*v*axis.x[0]);
	axis.x[2] = (Tij(0,2)+Tij(2,0)) / (2*v*axis.x[0]);

    } else if(Tij(1,1) > c+.5) {	/* large Y component */
	axis.x[1] = sqrtf((Tij(1,1)-c)/v) * (axis.x[1]<0 ? -1 : 1);
	axis.x[0] = (Tij(0,1)+Tij(1,0)) / (2*v*axis.x[1]);
	axis.x[2] = (Tij(2,1)+Tij(1,2)) / (2*v*axis.x[1]);

    } else if(Tij(2,2) > c+.5) {	/* large Z component */
	axis.x[2] = sqrtf((Tij(2,2)-c)/v) * (axis.x[2]<0 ? -1 : 1);
	axis.x[0] = (Tij(0,2)+Tij(2,0)) / (2*v*axis.x[2]);
	axis.x[1] = (Tij(2,1)+Tij(1,2)) / (2*v*axis.x[2]);
    } else {
	int i;
	fprintf(stderr, "Hey, tfm2quat() got a non-rotation matrix!\n");
	fprintf(stderr, "Check this out:\n");
	for(i=0;i<4;i++)
	  fprintf(stderr, "%12.8g %12.8g %12.8g %12.8g\n", Tij(i,0), Tij(i,1), Tij(i,2), Tij(i,3));
    }
  }
  mag = vlength(&axis);
  if(!finite(mag)) {
    fprintf(stderr, "Yikes, tfm2quat() yields NaN?\n");
  }
  /* The imaginary part is a vector pointing along the axis of rotation,
   * of magnitude sin(angle/2).  So normalize & scale the axis,
   * but don't fail if its magnitude was zero (i.e. no rotation).
   */
  vscale( iquat, mag==0 ? 0 : sinhalf/mag, &axis );
  return (float)sqrtf(1 + trace) * .5f;
}

/* quickie determinant of 3x3 submatrix */
float mdet3( CONST Matrix *T )
{
  return vlength((CONST Point *)&T->m[0]);
  Point v01;
  vcross( &v01, (CONST Point *)&T->m[0], (CONST Point *)&T->m[4] );
  return vdot( &v01, (CONST Point *)&T->m[8] );
}
   
void tfm2quat( Quat *quat, CONST Matrix *T )
{
  float ww, xx, yy, zz;  /* i.e. w^2, x^2, etc. */
  float w, x, y, z, max;
  float s = vlength( (Point *)&T->m[0*4+0] );
  int best = 0;

	/* A rotation matrix is
	 *  ww+xx-yy-zz    2(xy-wz)  2(xz+wy)
	 *  2(xy+wz)    ww-xx+yy-zz  2(yz-wx)
	 *  2(xz-wy)       2(yz+wx)  ww-xx-yy+zz
	 * and
	 * ww+xx+yy+zz = ss
	 */
  if(s > 0) {
    /* OK */
  } else {
    quat->q[0] = 1;
    quat->q[1] = quat->q[2] = quat->q[3] = 0;
    return;
  }
  ww = (s + T->m[0*4+0] + T->m[1*4+1] + T->m[2*4+2]);       /* 4 * w^2 */
  xx = (s + T->m[0*4+0] - T->m[1*4+1] - T->m[2*4+2]);
  yy = (s - T->m[0*4+0] + T->m[1*4+1] - T->m[2*4+2]);
  zz = (s - T->m[0*4+0] - T->m[1*4+1] + T->m[2*4+2]);

  max = ww;
  if(max < xx) max = xx, best = 1;
  if(max < yy) max = yy, best = 2;
  if(max < zz) max = zz, best = 3;

  switch(best) {
  case 0:				/* ww == max */
    w = sqrtf(ww) * 2;			/* 4w */
    x = (T->m[2*4+1] - T->m[1*4+2]) / w;  /* 4wx/4w */
    y = (T->m[0*4+2] - T->m[2*4+0]) / w;  /* 4wy/4w */
    z = (T->m[1*4+0] - T->m[0*4+1]) / w;  /* 4wz/4w */
    w *= .25f;				/* w */
    break;

  case 1:				/* xx == max */
    x = sqrtf(xx) * 2;			/* 4x */
    w = (T->m[2*4+1] - T->m[1*4+2]) / x;  /* 4wx/4x */
    y = (T->m[1*4+0] + T->m[0*4+1]) / x;  /* 4xy/4x */
    z = (T->m[0*4+2] + T->m[2*4+0]) / x;  /* 4xz/4x */
    x *= .25f;				/* x */
    break;

  case 2:				/* yy == max */
    y = sqrtf(yy) * 2;			/* 4y */
    w = (T->m[0*4+2] - T->m[2*4+0]) / y;  /* 4wy/4y */
    x = (T->m[1*4+0] + T->m[0*4+1]) / y;  /* 4xy/4y */
    z = (T->m[2*4+1] + T->m[1*4+2]) / y;  /* 4yz/4y */
    y *= .25f;				/* y */
    break;

  default:				/* zz == max */
    z = sqrtf(zz) * 2;			/* 4z */
    w = (T->m[1*4+0] - T->m[0*4+1]) / z;  /* 4wz/4z */
    x = (T->m[0*4+2] + T->m[2*4+0]) / z;  /* 4xz/4z */
    y = (T->m[2*4+1] + T->m[1*4+2]) / z;  /* 4yz/4z */
    z *= .25f;				/* z */
    break;
  }

  s = sqrtf(s);

  quat->q[0] = -w/s;
  quat->q[1] = x/s;
  quat->q[2] = y/s;
  quat->q[3] = z/s;
}

void quat2tfm( register Matrix *dst, CONST Quat *quat )
{
  float txx, txy, txz, txw, tyy, tyz, tyw, tzz, tzw;
  float ss = quat->q[0]*quat->q[0] + quat->q[1]*quat->q[1]
	   + quat->q[2]*quat->q[2] + quat->q[3]*quat->q[3];
  float sscl;
  if(ss == 0) {
    *dst = Tidentity;
    return;
  }
  sscl = 2/ss;
    
  txx = sscl*quat->q[1]*quat->q[1];
  txy = sscl*quat->q[1]*quat->q[2];
  txz = sscl*quat->q[1]*quat->q[3];
  txw = sscl*quat->q[1]*quat->q[0];
  tyy = sscl*quat->q[2]*quat->q[2];
  tyz = sscl*quat->q[2]*quat->q[3];
  tyw = sscl*quat->q[2]*quat->q[0];
  tzz = sscl*quat->q[3]*quat->q[3];
  tzw = sscl*quat->q[3]*quat->q[0];

  dst->m[0*4+0] = 1 - (tyy+tzz);	/* 1 - 2*(y^2 + z^2), etc. */
  dst->m[0*4+1] =     (txy+tzw);
  dst->m[0*4+2] =     (txz-tyw);

  dst->m[1*4+0] =     (txy-tzw);
  dst->m[1*4+1] = 1 - (txx+tzz);
  dst->m[1*4+2] =     (tyz+txw);

  dst->m[2*4+0] =     (txz+tyw);
  dst->m[2*4+1] =     (tyz-txw);
  dst->m[2*4+2] = 1 - (txx+tyy);

  dst->m[0*4+3] = dst->m[1*4+3] = dst->m[2*4+3] =
		  dst->m[3*4+0] = dst->m[3*4+1] = dst->m[3*4+2] = 0;
  dst->m[3*4+3] = 1;
}

void iquat2tfm( Matrix *dst, CONST Point *iquat )
{
  Point axis;
  float s, c, v, coshalf;
  int i, j;
  float sinhalf = vlength(iquat);

  mcopy( dst, &Tidentity );
  if(sinhalf == 0)
    return;
  if(sinhalf > 1) {
    fprintf(stderr, "quat2tfm: Yikes, clamping quat to length 1 (was 1+%g)\n", sinhalf-1);
    sinhalf = 1;
  }

  vscale(&axis, 1/sinhalf, iquat);

  coshalf = sqrtf(1 - sinhalf*sinhalf);
  s = 2*sinhalf*coshalf;	/* sin(angle) */
  v = 2*sinhalf*sinhalf;	/* versine: 1 - cos(angle) */
  c = 1 - v;			/* cos(angle) */

  for(i = 0; i < 3; i++) {
    for(j = 0; j < i; j++)
	dst->m[4*i+j] = dst->m[4*j+i] = axis.x[i]*axis.x[j]*v;
    dst->m[4*i+i] = axis.x[i]*axis.x[i]*v + c;
  }
  dst->m[4*0+1] += axis.x[2]*s;  dst->m[4*1+0] -= axis.x[2]*s;
  dst->m[4*2+0] += axis.x[1]*s;  dst->m[4*0+2] -= axis.x[1]*s;
  dst->m[4*1+2] += axis.x[0]*s;  dst->m[4*2+1] -= axis.x[0]*s;
}

float qdot( CONST Quat *qa, CONST Quat *qb ) {
  return qa->q[0]*qb->q[0] + qa->q[1]*qb->q[1]
	+ qa->q[2]*qb->q[2] + qa->q[3]*qb->q[3];
}

float qdist( CONST Quat *qa, CONST Quat *qb ) {
  float dw, dx, dy, dz;
  if(qdot(qa, qb) < 0) {
    dw = qa->q[0] + qb->q[0];
    dx = qa->q[1] + qb->q[1];
    dy = qa->q[2] + qb->q[2];
    dz = qa->q[3] + qb->q[3];
  } else {
    dw = qa->q[0] - qb->q[0];
    dx = qa->q[1] - qb->q[1];
    dy = qa->q[2] - qb->q[2];
    dz = qa->q[3] - qb->q[3];
  }
  return sqrtf(dw*dw + dx*dx + dy*dy + dz*dz);
}

float iqdist( CONST Point *q1, CONST Point *q2 ) {
  Point nq2;
  float s, sneg;
  vscale(&nq2, -1, q2);
  s = vdist(q1,q2);
  sneg = vdist(q1, &nq2);
  return (s < sneg) ? s : sneg;
}

float tdist( CONST Matrix *t1, CONST Matrix *t2 ) {
  float s = 0;
  int i;
  for(i=0; i<12; i++)
    s += (t1->m[i]-t2->m[i])*(t1->m[i]-t2->m[i]);
  return (float)sqrtf(s);
}

void rot2tfm( Matrix *dst, float degrees, CONST Point *gaxis )
{
  Point axis;
  int i, j;
  float c, v, s;

  *dst = Tidentity;
  if(degrees == 0 || gaxis == NULL || vunit( &axis, gaxis ) == 0) {
    return;
  }
  c = (float)cosf( degrees * (M_PI/180) );
  s = (float)sinf( degrees * (M_PI/180) );
  v = 1 - c;

  for(i = 0; i < 3; i++) {
    for(j = 0; j < i; j++)
	dst->m[4*i+j] = dst->m[4*j+i] = axis.x[i]*axis.x[j]*v;
    dst->m[4*i+i] = axis.x[i]*axis.x[i]*v + c;
  }
  dst->m[4*0+1] += axis.x[2]*s;  dst->m[4*1+0] -= axis.x[2]*s;
  dst->m[4*2+0] += axis.x[1]*s;  dst->m[4*0+2] -= axis.x[1]*s;
  dst->m[4*1+2] += axis.x[0]*s;  dst->m[4*2+1] -= axis.x[0]*s;
}

void rot2iquat( Point *iquat, float degrees, CONST Point *axis )
{
  vunit( iquat, axis );
  vscale( iquat, sinf( degrees * (M_PI/360.) ), iquat );
}

void rot2quat( Quat *quat, float degrees, CONST Point *axis )
{
  float half = degrees * (M_PI/360);
  float len = vlength(axis);
  float s;
  if(len == 0 || half == 0) {
    quat->q[0] = 1;
    quat->q[1] = quat->q[2] = quat->q[3] = 0;
    return;
  }
  s = sinf(half) / vlength(axis);
  quat->q[0] = cosf(half);
  vscale( (Point *)&quat->q[1], s, axis );
}

void quat_lerp( Quat *qdst, float frac, CONST Quat *qfrom, CONST Quat *qto ) {
  ;
  qcomb( qdst,
	frac, qfrom,
	(qdot( qfrom, qto ) < 0) ? frac-1 : 1-frac, qto );
  qnorm( qdst, qdst );
}

void iquat_lerp( Point *qdst, float frac, CONST Point *qfrom, CONST Point *qto ) {
  Point dst;
  Point tto = *qto;
  float s, rdst;
  float ifrom2 = VDOT(qfrom,qfrom);
  float rfrom = ifrom2>1 ? 0 : (float)sqrtf(1 - ifrom2);	/* Real part */
  float ito2 = VDOT(&tto,&tto);
  float rto = ito2>1 ? 0 : sqrtf(1 - ito2);
  float dot = VDOT(qfrom,&tto);
  if(dot < 0) {
	/* quaternions are in opposite hemispheres: negate "tto" */
	rto = -rto;
	vscale( &tto, -1, &tto );
  }
  /* Use linear interpolation between the quaternions.  This isn't right,
   * but shouldn't be far off if they don't differ by much.
   */
  rdst = (1-frac)*rfrom + frac*rto;
  vlerp( &dst, frac, qfrom, &tto );
  s = 1/sqrtf(rdst*rdst + VDOT(&dst,&dst));
  if(!finite(s)) {
    fprintf(stderr, "Yeow!\n");
  }
  if(rdst < 0) s = -s;
  vscale( qdst, s, &dst );
}

/*
 * Decompose the rotation & translation part of a camera-to-world matrix
 * (which may also include some *uniform* scaling)
 * into a product of three rotations:
 *
 *  Rotation(c2w) = rot('z', aer[2]) * rot('x', aer[1]) * rot('y', aer[0])
 *
 * Angles are returned in *degrees*, not radians!
 * aer[] stands for Azimuth, Elevation, and Roll, in that order.
 * aer[0] ~ y-rotation (closest to world)
 * aer[1] ~ x-rotation
 * aer[2] ~ z-rotation (closest to camera)
 */
float tfm2xyzaer( Point *xyz, float aer[3], CONST Matrix *c2w )
{
  float sx, cx, scl;
  Point xrow = *(Point *)&c2w->m[2*4+0];
  if(xyz != NULL)
    vgettranslation( xyz, c2w );
  scl = vunit( &xrow, &xrow );
  if(aer) {
    sx = -xrow.x[1];				/* normalized -m[2][1] */
    cx = (sx<-1 || sx>1) ? 0 : sqrtf(1 - sx*sx);
    aer[1] = atan2f( sx, cx ) * 180/M_PI;	/* xrot */
    if(cx < .001) {
	aer[0] = atan2f( c2w->m[1*4+0], c2w->m[0*4+0] )
		* (aer[1] < 0 ? -180/M_PI : 180/M_PI);
	aer[2] = 0;
    } else {
	aer[0] = atan2f( c2w->m[2*4+0], c2w->m[2*4+2] ) * 180/M_PI; /* yrot */
	aer[2] = atan2f( c2w->m[0*4+1], c2w->m[1*4+1] ) * 180/M_PI; /* zrot */
    }
  }
  return scl;
}

/*
 * Tcam2world = RotZ(aer[2]) * RotX(aer[1]) * RotY(aer[0]) * Translate(xyz)
 */
void xyzaer2tfm( Matrix *c2w, CONST Point *xyz, CONST float aer[3] )
{
  Matrix rx, ry, rz, t;
  if(aer == NULL) {
    *c2w = Tidentity;
  } else {
    mrotation( &ry, aer[0], 'y' );
    mrotation( &rx, aer[1], 'x' );
    mrotation( &rz, aer[2], 'z' );
    mmmul( &t, &rz, &rx );
    mmmul( c2w, &t,      &ry );
  }
  if(xyz != NULL)
    vsettranslation( c2w, xyz );
}

/*
 * Like glFrustum( left, right, bottom, top, near, far ):
 * ( 2*$n/($r-$l),       0,              0,                      0,
 *       0,          2*$n/($t-$b),       0,                      0,
 *  ($r+$l)/($r-$l), ($t+$b)/($t-$b),    -($n+$f)/($f-$n),       -1,
 *       0,              0,              -2*$f*$n/($f-$n),       0 );
 */
void mfrustum( Matrix *Tproj, float l, float r, float b, float t, float n, float f )
{
  float dx = r - l;	/* right - left */
  float dy = t - b;	/* top - bottom */
  float dz = f - n;	/* far - near */

  if(dx == 0 || dy == 0 || dz == 0 || f == 0 || n == 0) {
    *Tproj = Tidentity;
    return;
  }
  memset(Tproj, 0, sizeof(Matrix));
  Tproj->m[0*4+0] = 2*n / dx;
  Tproj->m[1*4+1] = 2*n / dy;
  Tproj->m[2*4+0] = (l+r) / dx;
  Tproj->m[2*4+1] = (t+b) / dy;
  Tproj->m[2*4+2] = -(n+f) / dz;
  Tproj->m[2*4+3] = -1;
  Tproj->m[3*4+2] = -2*n*f / dz;
}

void vlerp( Point *dst, float frac, CONST Point *vfrom, CONST Point *vto )
{
  dst->x[0] = (1-frac)*vfrom->x[0] + frac*vto->x[0];
  dst->x[1] = (1-frac)*vfrom->x[1] + frac*vto->x[1];
  dst->x[2] = (1-frac)*vfrom->x[2] + frac*vto->x[2];
}

/* Linear combination: dst = sa*a + sb*b */
void vcomb( Point *dst, float sa, CONST Point *a, float sb, CONST Point *b )
{
  dst->x[0] = sa*a->x[0] + sb*b->x[0];
  dst->x[1] = sa*a->x[1] + sb*b->x[1];
  dst->x[2] = sa*a->x[2] + sb*b->x[2];
}

void qcomb( Quat *dst, float sa, CONST Quat *qa, float sb, CONST Quat *qb )
{
  dst->q[0] = sa*qa->q[0] + sb*qb->q[0];
  dst->q[1] = sa*qa->q[1] + sb*qb->q[1];
  dst->q[2] = sa*qa->q[2] + sb*qb->q[2];
  dst->q[3] = sa*qa->q[3] + sb*qb->q[3];
}

void qnorm( Quat *dst, CONST Quat *src )
{
  float  s = src->q[0]*src->q[0] + src->q[1]*src->q[1]
	   + src->q[2]*src->q[2] + src->q[3]*src->q[3];
  if(s != 0)
    s = 1/sqrtf(s);
  dst->q[0] = src->q[0]*s;
  dst->q[1] = src->q[1]*s;
  dst->q[2] = src->q[2]*s;
  dst->q[3] = src->q[3]*s;
}
