/*
 * Particle format for "sdb" data: Loren Carpenter's Star Renderer.
 * Used in partiview.
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */
typedef enum {ST_POINT, ST_BRIGHT_CLOUD ,ST_DARK_CLOUD, ST_BOTH_CLOUD, ST_SPIKE, ST_OFF} stype;

#define IS_POINT(t)  ((1<<(t)) & ((1<<ST_POINT) | (1<<ST_SPIKE)))

typedef struct {
        float  x, y, z;
        float  dx, dy, dz;
        float  magnitude, radius;
        float  opacity;
        int  num;
        unsigned short  color;
	unsigned char	group;
        unsigned char	type;
}  db_star;

typedef  struct  hrec { float  t;  int  num;}  hrec_t;
typedef  struct  mrec { float  mass, x, y, z, vx, vy, vz, rho, temp, sfr, gasmass;
                 int  id, token;}  mrec_t;

