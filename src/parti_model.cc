#ifdef USE_MODEL

/*
 * Interface to cat_model code for partiview.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include "winjunk.h"
#endif


#include "specks.h"
#include "cat_model.h"
#include "cat_modelutil.h"
#include "findfile.h"
#include "partiviewc.h"

#include "plugins.h"

void cat_model_render( struct stuff *st, struct mesh *m ) {

  /* Load default color, just in case it's needed */
  if(st->coloredby == CONSTVAL) {
    struct valdesc *vd = &st->vdesc[st->curdata][CONSTVAL];
    glColor4f( vd->cmin, vd->cmax, vd->mean, st->alpha );
  } else {
    glColor4f( 1,1,1,st->alpha );
  }

  (static_cast<WavObj *>(m->wavobj))->render();
}

int parti_model_read( struct stuff **stp, int argc, char *argv[], char *fromfname, void * ) {
  if(argc < 1 || strcmp(argv[0], "mayaobj"))
    return 0;


  struct stuff *st = *stp;
  int timestep = st->datatime;
  struct mesh *m = NewN( struct mesh, 1 );
  char *scenename = NULL;

  memset(m, 0, sizeof(*m));

  m->type = MODEL;
  m->style = S_SOLID;

  int i;
  for(i = 1; i+1 < argc; i += 2) {
    if(!strncmp(argv[i], "-time", 3)) {
	timestep = atoi(argv[i+1]);
    } else if(!strcmp(argv[i], "-static")) {
	timestep = -1;
	i--;
    } else if(!strncmp(argv[i], "-c", 2)) {
	sscanf(argv[i+1], "%d", &m->cindex);
    } else if(!strcmp(argv[i], "-scene")) {
	scenename = argv[i+1];
    } else {
	break;
    }
  }

  if(i+1 != argc) {
    msg("%s: expected [-scene scenename.ma] objmodel.obj", fromfname);
    Free(m);
    return 1;
  }

  char *modelfname = findfile( fromfname, argv[i] );

  if(modelfname == NULL) {
    msg("%s: %s: can't find model file", fromfname, argv[i]);
    Free(m);
    return 1;
  }

  char *modelname = argv[i];

  int namelen = strlen(modelfname);

  if(scenename == NULL) {
    
    scenename = NewA( char, namelen + 7 );
    strcpy(scenename, modelfname);
    strcpy( strcmp(modelfname+namelen-4, ".obj") == 0
		? scenename+namelen-4 : scenename+namelen,
		".scene" );
  }

  m->objrender = cat_model_render;

  if( WavObj::readFile( modelname, modelfname, scenename, 1 ) ) {
    m->wavobj = WavObj::find( modelname );
  }

  if(m->wavobj == NULL) {
    Free(m);
    return 1;
  }

  if(timestep >= 0) {
    if(timestep >= st->ntimes)
	specks_ensuretime( st, st->curdata, timestep );
    m->next = st->meshes[st->curdata][timestep];
    st->meshes[st->curdata][timestep] = m;
  } else {
    m->next = st->staticmeshes;
    st->staticmeshes = m;
  }
  return 1;
}

void parti_model_init() {

    Model::preinit();

    parti_add_reader( parti_model_read, "mayaobj", NULL );
}
#endif /*USE_MODEL*/
