#ifndef CAT_MODEL_H
#define CAT_MODEL_H
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

	/*
	 * Library of renderable 3-D models,
	 */

#include "geometry.h"
#include "cat_modelutil.h"
#include "textures.h"

/* A TextureHookFunc is called (from Appearance?::ApplyOGL()) while textures are
 * being bound.  It's called (possibly several times, if several objects have video textures) as:
 * int  hookfunc( objnoptr, data-passed-when-hook-was-registered, Appearance *ap, WavFaces * )
 * It should
 *   Look at ap->txim->filename to find out which texture this is.
 *   return 0 if there's no special processing desired  (e.g. video disabled);
 *     in that case, ApplyOGL will apply whatever texture it would have if un-hooked.
 * or
 *   if(*objnoptr != desired_video_texture) {
 *     // make like txbind( video_texture, objnoptr );
 *     glBindTexture( desired_video_texture );
 *     *objno = desired_video_texture;
 *     maybe apply other opengl settings, e.g. to material properties;
 *       you could use the Appearance pointer to see what other settings apply.
 *   }
 *   return 1:
 */
typedef int (*TextureHookFunc)( int *txobjno, void *hookdata, class Appearance *, class WavFaces * );

	/* 3-D model, in shared memory */

class Model : public Shmem {
  public:
	char *name;	/* model name */
	char *fname;	/* name of model file */
	long fmtime;
	int defined;
	Model *next;	/* link in list of all Models */

	Model();
	void init();
	virtual ~Model();

	static Model *find( char *name );

	enum fstatus {
		UNREAD = -2,	/* file exists, but we haven't loaded it */
		OUTDATED = -1,	/* we've read a copy, but file has changed */
		NONESUCH = 0,	/* there is no such file */
		READIT = 1	/* we have a current copy */
	}; /* So, > 0 means "yes", < 0 means "try it", = 0 "don't bother" */


	virtual void render() { }

	static void preinit();
	static void purge();
	void discard();

	virtual int HookTextures( TextureHookFunc hook, void *data, const char *txnametag ) { return 0; }

	static enum fstatus seenFile( char *fname );	/* Seen this model file?*/

	static char **pathdirs;			/* Search path for findFile() */
	static char *findFile( char *name );	/* returns salloc()ed string */

  protected:
	static Model **allModels;	/* List of all Models */
	static Model **trashModels;

	int unused;
	void add();			/* add this Model to list */
	
};


	/* Generic appearance parameters.
	 * Should have subclasses for the various appearance types.
	 * Too bad.
	 */
class Appearance : public Shmem {
  public:
	char *name;	/* Wavefront/Maya name of this appearance (shading group) */
	Appearance *next;
	Appearance *use;
	int defined;	/* Have we filled the rest of this structure in yet? */
	int lighted;	/* or not */
	int smooth;	/* or flat */
	int shiny;	/* or matte */
	int textured;	/* or not */
	float Kd;	/* ambient, diffuse, specular coefs */
	float Cs[3];	/* surface color */
	float Ca[3];	/* ambient color */
	float shininess; /* specular exponent */
	float Cspec[3];	/* specular color */

	/* in case we're textured... */
	char *txdir;	/* directory in which to search for textures */
	char *txfname;
	Texture *txim;
	int txloaded;

	TextureHookFunc hookfunc;
	void *hookdata;

	void applyOGL( class WavFaces * ); /* apply appearance to OpenGL state */

	Appearance();
	void init();
	~Appearance();

	void add( Appearance **list );

	static Appearance *find( Appearance **listhead, char *name );
	static Appearance *findCreate( Appearance **listhead, char *name );
	static Appearance *create();

	void loadTextures();
	int HookTextures( TextureHookFunc hook, void *data, const char *txnametag );
	static void purge( Appearance **listhead );

};

class MayaScene : public Shmem {
	MayaScene();

  protected:
	char *fname;
	Appearance *aps;

	int readScene( char *scenefname, int complain );

	char *parseFileNode( char *name, FILE *f, char *line, int *lno );
	char *parseMatNode( char *name, FILE *f, char *line, int *lno, char *kind );
	char *parseEngineNode( char *name, FILE *f, char *line, int *lno );
	char *parseScrapNode( char *name, FILE *f, char *line, int *lno );
	char *parseConnect( char *from, char *to );

	Appearance *partials;	/* Used while parsing only */

	friend class WavObj;
};

class WavObj;		/* Forward */

class WavFaces : public Shmem {
  public:
	vvec<int> nfv;	/* Numbers of verts per face; nfv.count = nfaces */
	vvec<int> fv;	/* Table of all verts */
	Appearance *ap;	/* with this appearance */
	WavObj *obj;	/* reference back to our parent object */
	WavFaces *next; /* next group of faces */

	WavFaces();
	~WavFaces();

	static WavFaces *create();
	void init();
};

/* Wavefront object, read from .obj file,
 * with appearance read from Maya scene file.
 */
class WavObj : public Model {
  public:
	vvec<Point> pt;	/* 3-D Points */
	vvec<Point> tx;	/* texture coords */
	vvec<Point> norm;	/* surface normals */

	MayaScene *scene; /* basis for appearances */
	WavFaces *faces;

	WavObj();
	void init();
	~WavObj();
	static WavObj *create();
	static int readFile( char *name, char *fname, char *scenename, int complain );
	void render();

	WavObj & operator=( const WavObj &src ) {
	    pt = src.pt; tx = src.tx; norm = src.norm;
	    scene = src.scene; faces = src.faces;
	    return *this;
	}

	    // If you say
	    //     wavobj->HookTextures( NULL, NULL, "" )
	    // it will remove any hooks on any textures in that model.
	virtual int HookTextures( TextureHookFunc hook, void *data, const char *txnametag );

  private:
	WavObj *addObj( char *name, char *group );
};

class HappyFace : public Model {
  public:
	static void initialize();
};

#endif /*CAT_MODEL_H*/
