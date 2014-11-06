#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GL_GLEXT_PROTOTYPES 1	/* yes, of course we want glUseProgram etc. declared. */

#ifdef HAVE_LIBGLEW
# ifdef _WIN32
#  define GLEW_STATIC 1
# endif
# include <GL/glew.h>
#endif

#if 0  /* better HAVE_LIBGLEW */
# include <GL/gl.h>
# include <GL/glext.h>
# include <GL/glu.h>
#endif

#include "glshader.h"
#include "geometry.h"
#include "partiviewc.h"


ShaderStuff shaderstuff;

int shader_parse_args( struct stuff **, int argc, char *argv[], char *fromfname, void * )
{
    if(argc>0 && !strcmp(argv[0], "shader")) {
	if(shaderstuff.supported == ShaderStuff::UNKNOWN)
	    initShaderStuff( false );
	msg("%s", useShader(argv[1]));
	return 1;
    } else {
	return 0;
    }
}

void initShaderStuff( bool GLready )
{
  if(!shaderstuff.started) {
    shaderstuff.started = true;
    parti_add_commands( shader_parse_args, "shader", (void *)&shaderstuff );
  }

#ifdef HAVE_LIBGLEW
  if(shaderstuff.supported == ShaderStuff::UNKNOWN && GLready) {
    GLenum ini;
    if( (ini = glewInit()) != GLEW_OK ) {
	fprintf(stderr, "initShaderStuff: glewInit(): %s\n", glewGetErrorString(ini));
	return;
    }
    shaderstuff.supported = (GLEW_ARB_vertex_shader) ? ShaderStuff::SUPPORTED : ShaderStuff::UNSUPPORTED;
    if(shaderstuff.shadername != 0 && shaderstuff.supported == ShaderStuff::SUPPORTED) {
	/* if a shader was requested before we had a chance to initialize, try installing it now,
	 * and complain if there's trouble.
	 */
	msg( "%s", useShader( shaderstuff.shadername ) );
    }
  }
#else
  shaderstuff.supported = ShaderStuff::UNSUPPORTED;
#endif

}


char *inhalefile(const char *fname, const char *forwhat)
{
    FILE *f;
    long flen;
    char *s;

    if(fname == 0 || 0==strcmp(fname,"none"))
	return 0;

    f = fopen(fname, "r");
    if(f == 0) {
	if(forwhat) {
	    fprintf(stderr, "partiview: can't open %s %s\n", fname, forwhat);
	    return 0;
	}
    }

    fseek(f, 0, SEEK_END);

    flen = ftell(f);
    if(flen < 0) {
	fclose(f);
	fprintf(stderr, "partiview: can't measure length of file %s: %s\n", fname, strerror(errno));
	return 0;
    }
    fseek(f, 0, SEEK_SET);

    s = (char *)malloc(flen+1);
    if(s==0 || fread(s, flen, 1, f) <= 0) {
	perror(fname);
	free(s);
	fclose(f);
	return 0;
    }
    s[flen] = '\0';
    return s;
}
	

static const char default_vertshader[] = "\
#version 110\n\
\n\
main() {\n\
    gl_Position = ftransform();\n\
}\n\
";


const char *reportShader(void)
{
  static char *oom = 0;

  if(shaderstuff.supported == ShaderStuff::UNKNOWN) {
    return "shaders not initialized (initShaderStuff() never called)";
  } else if(shaderstuff.supported == ShaderStuff::UNSUPPORTED) {
#ifdef HAVE_LIBGLEW
    return "GLSL vertex shaders not supported";
#else
    return "GLSL vertex shaders not available (not configured with GLEW library)";
#endif
  } else if(shaderstuff.shadername) {
    char rpt[200];
    sprintf(rpt, "vertex shader %.175s loaded%s", shaderstuff.shadername, shaderstuff.enabled ? "" : "(disabled)");
    if(oom) free(oom);
    oom = strdup(rpt);
    return oom;
  } else {
    return "No GLSL vertex shader loaded";
  }
}

const char *useShader( const char *fname )
{
  GLenum e;
  GLint plogroom, slogroom;
  GLchar *plog, *slog;
  GLint logbytes;
  GLint vsok;

  if(fname == 0)
    return reportShader();

  if(0==strcmp(fname, "on")) {
    shaderstuff.enabled = 1;
    if(shaderstuff.prog == 0)
	return "No shader loaded yet";
    glUseProgram( shaderstuff.prog );
    return reportShader();

  } else if(0==strcmp(fname, "off")) {
    shaderstuff.enabled = 0;
    glUseProgram(0);
    return reportShader();
  }

  const char *oldshader = shaderstuff.shadername;
  shaderstuff.shadername = strdup(fname);
  if(oldshader)
    free((void *)oldshader);

  if(shaderstuff.supported == ShaderStuff::UNKNOWN) {
    return "shaders not yet initialized, will try later";
  }
    
  if(shaderstuff.supported != ShaderStuff::SUPPORTED) {
    return reportShader();
  }

  if(shaderstuff.prog != 0) {
    glUseProgram(0);
    glDeleteProgram(shaderstuff.prog);
  }
  if(shaderstuff.vshad != 0) {
    glDeleteShader(shaderstuff.vshad);
  }
  shaderstuff.vshad = glCreateShader(GL_VERTEX_SHADER);
  

  const char *vf = inhalefile( shaderstuff.shadername, "partiview vertex shader" );
  const char *vsrc = vf ? vf : default_vertshader;

  glShaderSource( shaderstuff.vshad, 1, &vsrc, NULL );
  if(vf) free((void *)vf);
  
  glCompileShader( shaderstuff.vshad );
  while((e = glGetError()) != 0) {
    fprintf(stderr, "GLSL Vertex shader: %s (0x%x)\n", gluErrorString(e), e);
  }

  glGetShaderiv( shaderstuff.vshad, GL_INFO_LOG_LENGTH, &slogroom );
  slog = (GLchar *)malloc( slogroom+1 );
  glGetShaderInfoLog( shaderstuff.vshad, slogroom+1, &logbytes, slog );
  fprintf(stderr, "Vertex shader log:\n%s\n", slog);

  glGetShaderiv( shaderstuff.vshad, GL_COMPILE_STATUS, &vsok );

  if(vsok == GL_TRUE) {
      shaderstuff.prog = glCreateProgram();
      glAttachShader(shaderstuff.prog, shaderstuff.vshad);
      glLinkProgram( shaderstuff.prog );
      while((e = glGetError()) != 0) {
	fprintf(stderr, "GLSL shader program: %s (0x%x)\n", gluErrorString(e), e);
      }
      glGetProgramiv( shaderstuff.prog, GL_INFO_LOG_LENGTH, &plogroom );
      plog = (GLchar *)malloc( plogroom+1 );
      glGetProgramInfoLog( shaderstuff.prog, logbytes+1, &plogroom, plog );
      fprintf(stderr, "Shader program log:\n%s\n", plog);

      free(plog);
  } else {
      shaderstuff.vshad = 0;
      shaderstuff.prog = 0;
  }
  free(slog);

  if(shaderstuff.enabled && shaderstuff.prog != 0)
    glUseProgram( shaderstuff.prog );
  return reportShader();
}

