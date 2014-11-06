#ifdef USE_MODEL

/*
 * Read and display 3-D models from .obj/.ma files
 * (wavefront .obj geometry, Maya 2.x/3.x .ma Maya ASCII material properties).
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef WIN32
# include "winjunk.h"
#else
# include <unistd.h>
#endif

#include <ctype.h>
#undef isspace

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>    /* for GLuint */
#endif

#include "cat_model.h"
#include "findfile.h"
#include "partiviewc.h"

#include "specks.h"	/* just for tokenize() -- ugh */

/* Instantiate the templates we'll need */
#include "cat_modelutil.cc"

template class vvec<int>;
template class vvec<Point>;

void CAT_model_preinit()
{
  Model::preinit();
}


/*
 * linked list of all Models.
 * We don't expect there'll be a huge number of these.
 */
Model **Model::allModels = NULL;
Model **Model::trashModels = NULL;

char **Model::pathdirs = NULL;

void Model::preinit()
{
  allModels = NewN( Model *, 1 );
  *allModels = NULL;
  trashModels = NewN( Model *, 1 );
  *trashModels = NULL;
}

Model::Model()
{
  init();
}

void Model::init()
{
  name = fname = NULL;
  next = NULL;
  defined = 0;
}

void Model::add()
{
  Model *alias, **aliasp;

  if(this->name == NULL) {
    msg("Model::add(): Can't add a nameless Model!");
    return;
  }
  for(aliasp = allModels; (alias = *aliasp) != NULL; aliasp = &alias->next) {
    if(!strcmp(this->name, alias->name)) {
	/* We already have a model named that. */
	/* Remove it from main list, and add to our "trash" list. */
	Model *succ = alias->next;
	*aliasp = succ;
	/* Add it to our "trash" list. */
	alias->discard();
	break;
    }
  }
  this->next = *allModels;
  *allModels = this;
}

void Model::discard()
{
  this->next = *trashModels;
  *trashModels = this;
  this->unused = 0;	/* set purge clock */
}

void Model::purge()
{
  Model *m, **mp = trashModels;
  for(mp = trashModels; (m = *mp) != NULL; ) {
    if(m->unused > 2) {
	*mp = m->next;
	delete m;
    } else {
	m->unused++;
	mp = &m->next;
    }
  }
}


/* purge the model, but leave it in the list */
Model::~Model()
{
  name = fname = NULL;	/* XXX leak memory */
  defined = 0;
}

Model *Model::find( char *name )
{
  Model *m;
  if(name == NULL) return NULL;
  int len = strlen(name);
  Model *maybe = NULL;

  for(m = *allModels; m != NULL; m = m->next) {
    if(m->name != NULL) {
	if(!strcmp(m->name, name))
	    break;
	if(!memcmp(m->name, name, len) && m->name[len] == ':')
	    maybe = m;
    }
  }
  return m ? m : maybe;
}

time_t fmodtime(char *fname)
{
  struct stat st;

  if(stat(fname, &st) < 0)
    return 0;
  return st.st_mtime;
}

/*
 * Do we have an up-to-date copy of "file"?
 * Returns 1 if yes, 0 if not seen, -1 if out-of-date.
 */
enum Model::fstatus Model::seenFile( char *fname )
{
  Model *m;

  if(fname == NULL)
    return NONESUCH;

  time_t mtime = fmodtime(fname);
  if(mtime == 0)
    return NONESUCH;

  for(m = *allModels; m != NULL; m = m->next) {
    if(m->fname && strcmp(m->fname, fname) == 0) {
	return m->fmtime < mtime ? OUTDATED : READIT;
    }
  }
  return UNREAD;
}


/*
 * .obj file reader
 */

Appearance::Appearance() {
    init();
}

void Appearance::init() {
    memset(this, 0, sizeof(*this));
    defined = 0;
    use = NULL;
    txfname = NULL;
    name = NULL;
    txloaded = 0;
    hookfunc = NULL;
    hookdata = NULL;
}

Appearance::~Appearance() {
    if(txfname) Free(txfname);
    if(txdir) free(txdir);
    if(name) Free(name);
    // if(txloaded) IMAGEbuff::schedule_expunge(txim);
}

void Appearance::add( Appearance **list )
{
    if(list) {
	next = *list;
	*list = this;
    }
}

Appearance *Appearance::find( Appearance **list, char *name )
{
    Appearance *ap;
    if(list == NULL || name == NULL) return NULL;
    for(ap = *list; ap != NULL; ap = ap->next)
	if(!strcmp(ap->name, name))
	    break;
    return ap;
}

Appearance *Appearance::findCreate( Appearance **list, char *name )
{
    Appearance *ap;
    if(list == NULL || name == NULL) return NULL;
    if((ap = find(list, name)) != NULL)
	return ap;
    ap = new Appearance;
    ap->name = shmstrdup(name);
    ap->defined = 0;
    ap->add( list );
    return ap;
}

int Appearance::HookTextures( TextureHookFunc hook, void *data, const char *txnametag ) {

    int any = 0;
    if(txnametag == NULL ||
	  (this->txfname != NULL &&
	    strstr(this->txfname, txnametag) != NULL) ) {
	this->hookfunc = hook;
	this->hookdata = data;
	any = 1;
    }
    if(this->use != NULL && this->use != this &&
				(any || use->HookTextures(hook,data,txnametag))) {
	this->use->hookfunc = hook;
	this->use->hookdata = data;
	any = 1;
    }
    return any;
}

WavObj::WavObj() {
  init();
}

void WavObj::init() {
  pt.init();
  tx.init();
  norm.init();
  scene = NULL;
  faces = NULL;
}

// If you say
//     wavobj->HookTextures( NULL, NULL, "" )
// it will remove any hooks on any textures in that model.
int WavObj::HookTextures( TextureHookFunc hook, void *data, const char *txnametag ) {
    int total = 0;
    if(this->scene == NULL) return 0;
    for(Appearance *ap = this->scene->aps; ap != NULL; ap = ap->next)
		total += ap->HookTextures( hook, data, txnametag );
    return total;
}

WavObj::~WavObj() {
}

WavObj *WavObj::create() {
  WavObj *obj = NewN( WavObj, 1 );
  obj->init();
  return obj;
}

WavFaces *WavFaces::create() {
  WavFaces *wf = NewN( WavFaces, 1 );
  wf->init();
  return wf;
}

void WavFaces::init()
{
  nfv.init();
  fv.init();
  ap = NULL;
  obj = NULL;
  next = NULL;
}


int agetfloats( float *v, int max, int a0, int ac, char **av )
{
  int i;
  char *cp;
  float tv;

  for(i = 0; i < max && a0+i < ac; i++) {
    tv = strtod(av[a0+i], &cp);
    if(*cp != '\0')
	break;
    v[i] = tv;
  }
  return i;
}

WavObj *WavObj::addObj( char *name, char *group )
{
    WavObj *obj;
    char fullname[256];

    obj = new WavObj();
    *obj = *this;

    sprintf(fullname, group ? "%.127s:%.127s" : "%.127s", name, group);
    obj->name = shmstrdup(fullname);

    obj->pt.trim(0);
    obj->tx.trim(0);
    obj->norm.trim(0);

    obj->add();		/* Add new object to public list */

    for(WavFaces *wf = obj->faces; wf != NULL; wf = wf->next) {
	wf->nfv.trim(0);
	wf->fv.trim(0);
	wf->obj = obj;

	/*
	 * Check that we found all the material-types we need.
	 * Also load any textures that we're actually using.
	 */
	if(!wf->ap) {
	    msg("Warning: %s: no material for group of %d faces?",
		    fname, wf->nfv.count);
	} else if(!wf->ap->defined) {
	    msg("Warning: %s: found no def'n for material %s",
		    fname, wf->ap->name );
	    wf->ap->defined = 1;
	    wf->ap->lighted = 0;
	    wf->ap->Kd = 1;
	    wf->ap->Cs[0] = .3;	/* puke green! */
	    wf->ap->Cs[1] = .5;
	    wf->ap->Cs[2] = .15;
	} else {
	    wf->ap->loadTextures();
	}
    }
    return obj;
}

int WavObj::readFile( char *name, char *fname, char *scenename, int complain )
{
    FILE *inf = fopen(fname, "r");
    char line[1024], tline[1024], *s;
    int ac;
    char *av[32];
    Appearance *ap = NULL;
    int k;
    int lno = 0;
    char *err = NULL;
    char errbuf[80];
    char *group = NULL;
    WavFaces *curfaces = NULL;
    Point tpt[4000], ttx[4000], tnorm[4000];
    int tnfv[250], tfv[1000];
    int ok = 1;
    Point scaleby;
    WavObj me;

    if(inf == NULL) {
	if(complain)
	    msg( "Couldn't open model file %s", fname );
	return 0;
    }

    me.fname = shmstrdup(fname);
    me.scene = new MayaScene;
    me.scene->fname = shmstrdup(scenename);
    me.scene->aps = NULL;

	/*
	 * Inhale corresponding ".scene" file to read all those appearances
	 */

    me.scene->readScene( scenename, complain );

    /*
     * Use auto vars for default space.
     */
    me.pt.use( tpt, COUNT(tpt) );
    me.tx.use( ttx, COUNT(ttx) );
    me.norm.use( tnorm, COUNT(tnorm) );
    me.pt.count = me.tx.count = me.norm.count = 0;

    scaleby.x[0] = scaleby.x[1] = scaleby.x[2] = 1;

    while(fgets(line, sizeof(line), inf) != NULL) {
	lno++;
	ac = tokenize( line, tline, COUNT(av), av, NULL );
	if(ac <= 0) continue;
	s = av[0];

	if(!strcmp(s, "v")) {
	    Point *cv = me.pt.append();
	    if(agetfloats( &cv->x[0], 3, 1, ac, av ) < 3)
		err = "v: expected 3 floats";
	    cv->x[0] *= scaleby.x[0];
	    cv->x[1] *= scaleby.x[1];
	    cv->x[2] *= scaleby.x[2];

	} else if(!strcmp(s, "vt")) {
	    if(agetfloats( &me.tx.append()->x[0], 2, 1, ac, av ) < 2)
		err = "vt: expected 2 floats";

	} else if(!strcmp(s, "vn")) {
	    if(agetfloats( &me.norm.append()->x[0], 3, 1, ac, av ) < 3)
		err = "vn: expected 3 floats";

	} else if(!strcmp(s, "s")) {
	    /* Not sure what this new "s" directive means.
	     * Could it be related to smoothing?
	     * Anyway, let's ignore it.
	     */

	} else if(!strcmp(s, "scale")) {
	    int ns = agetfloats( &scaleby.x[0], 3, 1, ac, av );
	    switch(ns) {
	    case 1: scaleby.x[1] = scaleby.x[2] = scaleby.x[0]; break;
	    case 3: break;
	    default: err = "scale: expected 1 or 3 floats"; break;
	    }

	} else if(!strcmp(s, "g")) {
	    /* ignore "g" directives.  Hmm, wonder if they're
	     * related to Maya layers?
	     */

	} else if(!strcmp(s, "group")) {
	    if(me.pt.count > 0) {
		me.addObj( name, group );

		me.faces = NULL;
		me.pt.count = me.tx.count = me.norm.count = 0;
		me.pt.use( tpt, COUNT(tpt) );
		me.tx.use( ttx, COUNT(ttx) );
		me.norm.use( tnorm, COUNT(tnorm) );
	    }
	    if(group != NULL) Free(group);
	    group = shmstrdup(av[1]);
	    curfaces = NULL;

	} else if(!strcmp(s, "usemtl")) {
	    if(ac < 2)
		err = "usemtl: expected material name";
	    else {
		Appearance *nap = NULL;
		for(k = ac; --k > 0; ) {
		    nap = Appearance::find( &me.scene->aps, av[k] );
		    if(nap != NULL && nap->defined)
			break;
		}
		if(nap == NULL || !nap->defined) {
		    err = "usemtl: no known appearances listed";
		} else if(nap->defined < 0) {
		    err = "usemtl: no layered shaders; try multilister's Convert Solid Texture";
		    nap->defined = 1;	/* emit above message only once */
		} else if(ap != nap) {
		    if(curfaces) {
			curfaces->nfv.trim( curfaces->nfv.room );
			curfaces->fv.trim( curfaces->fv.room );
		    }
		    /* Do we already have some faces with this appearance?
		     * Add more to the same group.
		     */
		    for(curfaces = me.faces; curfaces != NULL;
						curfaces = curfaces->next) {
			if(curfaces->ap == nap)
			    break;
		    }
		    /* If not, curfaces is now NULL, so we'll create a new
		     * clump of faces when needed.
		     */
		    ap = nap;
		}
	    }

	} else if(!strcmp(s, "f")) {
	    if(curfaces == NULL) {
		curfaces = WavFaces::create();
		curfaces->obj = NULL;
		curfaces->ap = ap;
		curfaces->next = me.faces;
		me.faces = curfaces;
		curfaces->nfv.use( tnfv, COUNT(tnfv) );
		curfaces->fv.use( tfv, COUNT(tfv) );
	    }

	    /* How many verts on this face?  One per arg after the "f" */

	    int mynfv = (ac - 1);
		/* face description in fv[] begins at current pos'n */
	    int fv0 = curfaces->fv.count;

		/* preallocate enough space in fv[], and adjust count. */
	    curfaces->fv.needs( fv0 + mynfv*3 );
	    curfaces->fv.count = fv0 + mynfv*3;

		/* each face takes two entries in nfv[]: */
	    *curfaces->nfv.append() = mynfv;	/* count */
	    *curfaces->nfv.append() = fv0;	/* fv[] offset */
		/* and 3*N entries in fv[]: */
	    int *fvp = curfaces->fv.val(fv0);
	    for(k = 0; k < mynfv; k++, fvp += 3) {

		/* Parse the NN/NN/NN vertex description */
		char *cp = av[k+1];
		fvp[0] = strtol(cp, &cp, 0) - 1;
		if(*cp == '/') cp++;
		fvp[1] = strtol(cp, &cp, 0) - 1;
		if(*cp == '/') cp++;
		fvp[2] = strtol(cp, &cp, 0) - 1;
		if(*cp != '\0') {
		    err = "f: expected series of NN/NN/NN (or just NN) fields for each vertex";
		} else {
		    if(fvp[0] >= me.pt.count) {
			sprintf(err = errbuf,
			    "There's no vertex number %d! Only have %d",
			    fvp[0]+1, me.pt.count);
			ok = 0;
		    } else if(fvp[1] >= me.tx.count) {
			sprintf(err = errbuf,
			    "There's no texture-vertex number %d!  Only have %d",
			    fvp[1]+1, me.tx.count);
			ok = 0;
		    } else if(fvp[2] >= me.norm.count) {
			sprintf(err = errbuf,
			    "There's no surface-normal number %d!  Only have %d",
			    fvp[2]+1, me.norm.count);
			ok = 0;
		    }
		}
	    }

	} else {
	    err = "Surprise tag: expected \"f\" or \"v\" or \"vt\" or \"vn\" or \"usemtl\"";
	}

	if(err != NULL) {
	    msg("Reading .obj file %s line %d: %s", me.fname, lno, err);
	    msg(" %s", line);
	    // ok = 0;	// Let's make all errors non-fatal for now.
	    err = NULL;
	}
    }   
    
    fclose(inf);

    if(ok) {
	me.fmtime = fmodtime(me.fname);
	me.defined = 1;

	me.addObj( name, group );
    }
    return ok;
}

MayaScene::MayaScene()
{
  aps = NULL;
  partials = NULL;
  fname = NULL;
}

#define	LINESIZE	1024

char *MayaScene::parseFileNode( char *name, FILE *f, char *reline, int *lno )
{
  char line[LINESIZE+1], tline[LINESIZE+1];
  char *av[8];
  Appearance *ap = Appearance::findCreate( &this->partials, name );

  while(fgets(line, LINESIZE, f) != NULL) {
    if(!isspace(line[0])) {	/* Must not be our kind of "file" node -- rescan */
	// delete ap;  Not if we add with findCreate()!
	strcpy(reline, line);
	return NULL;		/* we don't consider this an error */
    }
    ++*lno;

    int ac = tokenize( line, tline, COUNT(av), av, NULL );
    if(ac <= 4 || strcmp(av[0], "setAttr") != 0)
	return "createNode file: expected setAttr";
    if(strcmp(av[1], ".ftn") != 0 || strcmp(av[2], "-type") != 0)
	continue;
    ap->txfname = shmstrdup( av[4] );

    if(this->fname != NULL) {
	char *tail = strrchr( this->fname, '/' );
	if(tail) {
	    int len = tail - this->fname + 1;
	    ap->txdir = static_cast<char *>(malloc( len+1 ));
	    memcpy( ap->txdir, this->fname, len );
	    ap->txdir[len] = '\0';
	} else {
	    ap->txdir = strdup("./");
	}
    }
    return NULL;
  }

  // delete ap;  Not if we add with findCreate()!
  reline[0] = '\0';
  return "Surprise EOF";
}

char *MayaScene::parseMatNode( char *name, FILE *f, char *reline, int *lno, char *kind )
{
  char line[LINESIZE+1], tline[LINESIZE+1];
  char *av[16];
  Appearance *ap = Appearance::findCreate( &this->partials, name );

  ap->defined = 1;

/*
 * Fill in all our defaults
 */
  ap->shiny = (strcmp(kind, "lambert") != 0);
  ap->lighted = 1;
  ap->smooth = 0;
  ap->textured = 0;
  ap->Cs[0] = ap->Cs[1] = ap->Cs[2] = 0.5;
  ap->Ca[0] = ap->Ca[1] = ap->Ca[2] = 0;
  // ap->Ka = 0;
  ap->Kd = .8;
  // ap->Ks = .5;
  ap->Cspec[0] = ap->Cspec[1] = ap->Cspec[2] = .5;
  ap->shininess = 20.;

  while(fgets(line, LINESIZE, f) != NULL) {
    if(!isspace(line[0])) {	/* End of material node */
	strcpy(reline, line);
	return NULL;		/* we don't consider this an error */
    }
    ++*lno;

    int ac = tokenize( line, tline, COUNT(av), av, NULL );
    if(ac <= 2 || strcmp(av[0], "setAttr") != 0)
	return "material node: expected setAttr";
    if(!strcmp(av[1], ".dc"))
	ap->Kd = atof(av[2]);

    else if(!strcmp(av[1], ".sro"))
	ap->shininess = 1 / (1 - atof(av[2]));

    else if(!strcmp(av[1], ".cp"))
	ap->shininess = atof(av[2]);

    else if(ac >= 7 && !strcmp(av[2],"-type") && !strcmp(av[3],"float3")) {
	if(!strcmp(av[1], ".c"))
	    agetfloats( &ap->Cs[0], 3, 4, ac, av );
	if(!strcmp(av[1], ".ambc"))
	    agetfloats( &ap->Ca[0], 3, 4, ac, av );
	else if(!strcmp(av[1], ".sc"))
	    agetfloats( &ap->Cspec[0], 3, 4, ac, av );
    }
  }
  reline[0] = '\0';
  return NULL;
}

char *MayaScene::parseScrapNode( char *name, FILE *f, char *reline, int *lno ) {
  Appearance *ap = Appearance::findCreate( &this->partials, name );
  fgets(reline, sizeof(reline), f);
  ap->defined = -1;
  return NULL;
}

char *MayaScene::parseEngineNode( char *name, FILE *f, char *reline, int *lno )
{
  Appearance::findCreate( &this->aps, name );	/* just ensure node on list! */
  fgets(reline, sizeof(reline), f);
  return NULL;
}

/*
 * if str ends with suffix suf, return index in str where suf begins,
 * else -1.
 */
int endsWith( char *str, char *suf )
{
  if(str == NULL) return 0;
  int at = strlen(str) - strlen(suf);
  if(at < 0) return 0;
  return strcmp( str+at, suf ) == 0 ? at : -1;
}

char *MayaScene::parseConnect( char *from, char *to )
{
  int efrom, eto;
  Appearance *afrom, *ato;

  if((efrom = endsWith( from, ".oc" ))>0  && (eto = endsWith( to, ".ss" ))>0) {
    /* material -> engine */
    from[efrom] = '\0';  to[eto] = '\0';

    /* Expect to find "from" on the "partials" list (as a lamb/blinn/phong node)
     * and "to" on the "aps" list (as a final shadingEngine node).
     */
    afrom = Appearance::find( &this->partials, from );
    ato = Appearance::find( &this->aps, to );
    if(afrom && ato) {
	ato->use = afrom;
	ato->defined = 1;
    }
  }
  else if((efrom = endsWith( from, ".oc" ))>0 && (eto = endsWith( to, ".c" ))>0) {
    /* texture-file -> material */
    from[efrom] = '\0'; to[eto] = '\0';
    afrom = Appearance::find( &this->partials, from );
    ato = Appearance::find( &this->partials, to );
    if(afrom && afrom->txfname && ato)
	ato->use = afrom;
  }
  return NULL;
}


int MayaScene::readScene( char *scenefname, int complain )
{
  char line[LINESIZE+1], tline[LINESIZE+1];
  int ac;
  char *av[32];
  int lno = 0;
  int ok = 1;
  int rescan = 0;
  char *err = NULL;

  if(scenefname == NULL) return 0;
  FILE *f = fopen(scenefname, "r");
  if(f == NULL) {
    msg( "Can't open Maya scene file %s", scenefname);
    return 0;
  }

  while(rescan || fgets(line, sizeof(line), f) != NULL) {
    rescan = 0;
    lno++;

    if(strcmp(line, "FOR4") == 0) {
	msg( "Hmm, is %s a \"Maya binary\" file?  Needs to be saved as \"Maya ASCII\".",
		scenefname);
    }
    else if(strncmp(line, "createNode", 10) == 0) {

	ac = tokenize( line, tline, COUNT(av), av, NULL );
	if(ac <= 1)
	    continue;
	char *kind = av[1];
	char *name = "";
	int i;
	for(i = 2; i < ac-1; i++) {
	    if(!strcmp(av[i], "-n")) {
		name = av[i+1];
		break;
	    }
	}
	if(!strcmp(kind, "file"))
	    err = parseFileNode(name, f, line, &lno);
	else if(!strcmp(kind, "lambert") || !strcmp(kind, "blinn")
					 || !strcmp(kind, "phong"))
	    err = parseMatNode(name, f, line, &lno, kind);
	else if(!strcmp(kind, "shadingEngine"))
	    err = parseEngineNode(name, f, line, &lno);
	else if(!strcmp(kind, "layeredShader"))
	    err = parseScrapNode(name, f, line, &lno);
	else
	    continue;	/* Ignore other createNode types */
        rescan = 1;

    }
    else if(strncmp(line, "connectAttr", 11) == 0) {
	ac = tokenize( line, tline, COUNT(av), av, NULL );
	if(ac >= 3)
	    err = parseConnect(av[1], av[2]);
	continue;
    }
    else
	continue;		/* Ignore other stuff in file */

    if(err) {
	msg("Reading Maya file %s line %d: %s", fname, lno, err);
	msg("  %s", line);
	ok = 0;
	break;
    }
  }

  fclose(f);

  if(ok) {
    /* Tie partial information into top-level Appearance's */
    Appearance *ap, *mat;
    for(ap = this->aps; ap != NULL; ap = ap->next) {
	if((mat = ap->use) != NULL && mat->defined) {
	    ap->defined = 1;
	    if(mat->use && mat->use->txfname) {
		mat->txfname = mat->use->txfname;
		if(mat->use->txdir)
		    mat->txdir = mat->use->txdir;
	    }
	}
    }
  }
  return ok;
}

void Appearance::loadTextures()
{
  if(txfname != NULL && txloaded == 0) {
    char *cp = strrchr(txfname, '/');
    if(cp == NULL) cp = txfname;
    else cp++;
    char *fullname = findfile( txdir, cp );
    if(fullname == NULL) {
	msg( "Can't find texture image %s", cp);
	txloaded = -1;	/* Failed */
	return;
    }
    txim = txmake( fullname, TXF_DECAL, TXF_SCLAMP|TXF_TCLAMP, 7 );
    if(txload(txim) <= 0) {
	msg( "Can't load texture (must be SGI image) %s", fullname);
	txloaded = -1;	/* Failed */
	return;
    }
    txloaded = 1;
    textured = 1;

  } else if(this->use != NULL) {

    use->loadTextures();
  }
}

void WavObj::render()
{
    WavFaces *wf;
    int i;
    int *fvp, *nfvp;
    int token = 0;
    int prevtoken = 0;

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);

    for(wf = faces; wf != NULL; wf = wf->next) {

	if(wf->ap)
	    wf->ap->applyOGL(wf);

	nfvp = wf->nfv.v;
	for(i = 0, nfvp = wf->nfv.v; i < wf->nfv.count; i += 2, nfvp += 2) {
	    int nverts = nfvp[0];
	    int fvbase = nfvp[1];

	    switch(nfvp[0]) {
	    case 1: token = GL_POINTS; break;
	    case 2: token = GL_LINES; break;
	    case 3: token = GL_TRIANGLES; break;
	    case 4: token = GL_QUADS; break;
	    default: token = GL_POLYGON;
		     if(prevtoken != 0) prevtoken = -1;
		     break;
	    }
	    if(token != prevtoken) {
		if(prevtoken != 0) glEnd();
		glBegin( token );
	    }
	    for(fvp = &wf->fv.v[ fvbase ]; --nverts >= 0; fvp += 3) {
		if(fvp[2]>=0)
		    glNormal3fv( norm.v[ fvp[2] ].x );
		if(fvp[1]>=0)
		    glTexCoord2fv( tx.v[ fvp[1] ].x );
		glVertex3fv( pt.v[ fvp[0] ].x );
	    }
	    prevtoken = token;
	}
	if(prevtoken != 0) {
	    glEnd();
	    prevtoken = 0;
	}
    }
    txbind(NULL, NULL);
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void Appearance::applyOGL( WavFaces *wf )
{
    static GLfloat zero[3] = {0,0,0};
    GLfloat cd[3];

    if(!defined)
	return;
    Appearance *ap = this;
    if(this->use) ap = this->use;

    glDisable(GL_LIGHTING);

    glDisable(GL_COLOR_MATERIAL);
    if(!ap->textured) {
	// If this node is flagged as having no texture,
	// just apply all our other material properties (color, shininess, ...)
	glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, ap->shiny ? ap->shininess : 10 );
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, ap->Ca );
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,
	    ap->shiny ? ap->Cspec : zero );
	cd[0] = ap->Kd * ap->Cs[0];
	cd[1] = ap->Kd * ap->Cs[1];
	cd[2] = ap->Kd * ap->Cs[2];
	if(ap->lighted)
	    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, cd );
	else
	    glColor3fv( cd );
    } else {
	// We have a texture, or at least a pointer to an image file.
	// Select white material with appropriate shininess.
	static GLfloat white[] = {1,1,1,1};
	glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, ap->shiny ? 10 : 10 );
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &white[0] );
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, ap->shiny ? &white[0] : zero );
	glMatrixMode( GL_TEXTURE );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
    }

    if(ap->use && ap->use->textured && ap->use->txloaded > 0)
 	ap = ap->use;

    if(ap->lighted) glEnable(GL_LIGHTING);
    int enabled = -1;
	//jjm-hook
    if(ap->hookfunc != NULL) {
	if( (*ap->hookfunc)( &enabled, ap->hookdata, ap, wf ) )
	    return;
    }
    if(ap->textured && ap->txloaded > 0) {
	txbind( ap->txim, &enabled ); /* Bind tx object, enable texturing. */
	// glEnable( GL_ALPHA_TEST );  Something's wrong on the Octane?? XXX
	glAlphaFunc( GL_GREATER, 0.5 );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    } else {
	txbind( NULL, NULL );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_ALPHA_TEST );
    }
}

void survey( Appearance *ap ) {
  while(ap != NULL) {
    printf("%p: %s[%p]%d  ",
	ap, ap->name, ap->name, ap->defined);
    ap = ap->next;
  }
  printf("\n");
}
#endif /*USE_MODEL*/
