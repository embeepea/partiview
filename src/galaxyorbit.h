#ifndef GALAXYORBIT_H
#define GALAXYORBIT_H

typedef struct {
    float zmax;		/* max altitude, pc */
    float zphase;	/* phase of z oscillations, radians */
    float Rg;		/* radius of generating-circle orbit */
    float ecc;		/* eccentricity */
    float Rphase0;	/* phase of epicycle ellipse */
    float Rgphase;	/* phase of generating-circle */
    float zomega;	/* angular freq of vert oscillations, radians/Myr */
    float Romegapc;	/* angular freq of longitudinal orbit, radians-pc/Myr */
} GalOrbit;

#endif
