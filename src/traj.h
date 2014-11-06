#ifndef TRAJ_H
#define TRAJ_J 1

typedef struct {
    int atype;		/* 'i' or 'f' */
    char *aname;
} TAttr;

typedef union {
    int i;
    float f;
} TVal;

typedef struct {
    int nsamp;
    TVal *tattr;
    TVal *sattr;
} TrajArc;

typedef struct {
    char magic[4];	/* 'l' 'T' xxx xxx */
    int reqno;
    int groupval;
    int groupmask;
    float startdatatime;
    float duration;
    float velrate;
    float datarate;
    float sampledt;
    int reportIOformat;
    int reportstyle;
    int reportinterval;
    int ntrajattrs;
    TAttr *trajattr;	/* getc32 => 'i' or 'f', followed by name-bytecount, followed by \000-padded name.  Bytecount includes padding. */
			/* might include "ttime0", "startdatatime" */
    int nsampattrs;
    TAttr *sampattr;
    int ntrajectories;
    TrajArc *arc;
} TrajHead;

#endif
