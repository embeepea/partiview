#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <ctype.h>
#undef isspace

#include "cmed.H"

struct _cshow  cshow;


static int cleanup(char *buf, int room, const char *str) {
  char *p;
  const char *q;
  p = buf;
  for(q = str; *q && isspace(*q); q++) ;
  for( ; *q && !isspace(*q) && p < &buf[room-1]; q++)
    *p++ = *q;
  *p = '\0';
  return(p > buf);
}

void input_cb( Fl_Input *inp, void * ) {
  int nents = cmedit->cment();
  if(sscanf(inp->value(), "cments %d", &nents) > 0
     || sscanf(inp->value(), "cment %d", &nents) > 0) {

    cmedit->cment( nents );

  }
}

void fload_cb( Fl_Button *, void * ) {
  FILE *inf;
  char fname[256];

  fnamebox->position(strlen(fnamebox->value()), strlen(fnamebox->value()));
  Fl::focus(fnamebox);

  if(!cleanup(fname, sizeof(fname), fnamebox->value())) {
    fnamebox->value("Type input file name here");
    return;
  }

  if((inf = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "%s: can't open: ", fname);
    perror("");
    fnamebox->insert(" [?]");
    return;
  }

  cmedit->snapshot();

  if(!cmedit->fload( inf )) {
    fprintf(stderr, "%s: can't read colormap\n", fname);
    fnamebox->insert(" [?]");
  }
  cmedit->redraw();
  fclose(inf);
}

void fsave_cb( Fl_Button *savebutton, void * ) {
  FILE *outf;
  char fname[256];

  if(!cleanup(fname, sizeof(fname), fnamebox->value())) {
    fnamebox->value("Type output file name here");
    fnamebox->position(0, strlen(fnamebox->value()));
    Fl::focus(fnamebox);
    return;
  }

  if((outf = fopen(fname, "w")) == NULL) {
    fprintf(stderr, "%s: can't create: ", fname);
    perror("");
    fnamebox->position( strlen(fnamebox->value()) );
    fnamebox->insert(" [?]");
    return;
  }

  if( cmedit->fsave( outf ) ) {
    fprintf(stderr, "%s: saved\n", fname);
  } else {
    fprintf(stderr, "%s: write error: ", fname);
    perror("");
  }
  fclose(outf);
}

void undo_cb( Fl_Button *, void * ) {
  cmedit->undo();
}

void report_cb( Fl_Value_Input *cindex, void * ) {
  cmedit->report( (int) cindex->value() );
}

void lerp_cb( Fl_Slider *sl, void * ) {
  cmedit->lerpval = sl->value();
}

void rgbmode_cb( Fl_Button *btn, void * ) {
  static char *btnlbl[2] = { "RGB", "HSB" };
  static char *hsblbls[3][2] = {
		{ "Red(L)", "Hue(L)" },
		{ "Green(M)", "Sat(M)" },
		{ "Blue(R)", "Bright(R)" } };
  cmedit->hsbmode = !cmedit->hsbmode;
  cmedit->redraw();
  btn->label( btnlbl[ cmedit->hsbmode ] );
  btn->redraw();
  cshow.hsblbls[0]->labelcolor( cmedit->hsbmode ? 3/*yellow*/ : 1/*red*/ );

  for(int i = 0; i < 3; i++) {
    cshow.hsblbls[i]->label( hsblbls[i][cmedit->hsbmode] );
    cshow.hsblbls[i]->redraw();
  }
}

void reporter( CMedit *cm, int x )
{
  char msg[64];
  float hsba[4], rgba[4];

  if(cshow.cindex == NULL) return;
  cm->gethsba( x, hsba );
  cm->getrgba( x, rgba );
  cshow.cindex->value( x );
  sprintf(msg, "%.3f %.3f %.3f %.3f", hsba[0],hsba[1],hsba[2],hsba[3]);
  cshow.hsba->value( msg );
  sprintf(msg, "%.3f %.3f %.3f", rgba[0],rgba[1],rgba[2]);
  cshow.rgba->value( msg );
  cshow.color->rgba( rgba[0], rgba[1], rgba[2], rgba[3] );
  cshow.color->redraw();
  sprintf(msg, "%g", cm->Aout( rgba[3] ) );
  cshow.scaleout->value(msg);
  if(cshow.cmentin->value() != cm->cment())
    cshow.cmentin->value( cm->cment() );
  if(cshow.postscalein->value() != cm->postscale())
    cshow.postscalein->value( cm->postscale() );
  if(cshow.postexponin->value() != cm->postexpon())
    cshow.postexponin->value( cm->postexpon() );
}

void cmenter( const CMedit *cm )
{
  if(cshow.cmentin->value() != cm->cment())
    cshow.cmentin->value( cm->cment() );
}

void ascale_cb( Fl_Value_Input*, void* ) {

  if(cshow.postscalein->value() != cmedit->postscale() ||
     cshow.postexponin->value() != cmedit->postexpon()) {

    cmedit->postscale( cshow.postscalein->value() );
    cmedit->postexpon( cshow.postexponin->value() );
    cmedit->report();
  }
}

void ncment_cb( Fl_Value_Input *inp, void * )
{
  cmedit->cment( (int) (inp->value() + 0.5) );
}
  
void quietwarning( const char *fmt, ... ) {
  char msg[10240];
  static char avoid[] = "X_ChangeProperty: ";
  va_list args;
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  if(0!=strncmp(msg, avoid, sizeof(avoid)-1))
    fputs(msg, stderr);
}

int main(int argc, char *argv[]) {

  Fl::warning = quietwarning;

  Fl_Window *top = make_window();

  cmedit->reportto( reporter );
  cmedit->cmentto( cmenter );

  cshow.postscalein->value(1);
  cshow.postexponin->value(1);

  if(argc>2 && !strcmp(argv[1], "-e")) {
    cmedit->cment( atoi(argv[2]) );
    argc -= 2, argv += 2;
  }
  if(argc>1) {
    fnamebox->value( argv[argc-1] );
    fload_cb( loadbtn, NULL );
  }
  top->show(0, argv);
  cmedit->show();
  return Fl::run();
}
