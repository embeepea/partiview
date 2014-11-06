#ifndef GLSHADER_H
#define GLSHADER_H 1

typedef struct {
    enum { UNKNOWN, SUPPORTED, UNSUPPORTED } supported;
    int enabled;
    GLuint prog;			/* program object */
    GLuint vshad;			/* vertex shader */
    const char *shadername;
    const char *shadersource;
    bool started;
} ShaderStuff;
    
extern ShaderStuff shaderstuff;

extern void initShaderStuff( bool GLready );
extern const char *useShader( const char *fname );

#endif /* GLSHADER_H */
