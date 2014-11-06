#ifndef _SZGPARTIVIEW_H
#define _SZGPARTIVIEW_H

/*
 * Main scene data for szgPartiview.
 */
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#include "arSocket.h"

#ifdef WIN32
# include "winjunk.h"
#endif

#include "specks.h"
#include "partiviewc.h"

#include <vector>

typedef struct subcam {
  char name[8];
  float azim, elev, roll;
  float nleft, right, ndown, up;
} Subcam;

struct PvObject {
  struct stuff *st_;
  Matrix To2w_;
  char	*name_;
  int	id_;			// id in parent's list of objects
  int	parent_;		/* 0=world, GV_ID_CAMERA=attach-to-camera */


  PvObject();
  PvObject( const char *name, int id );
  void init( const char *name, int id );

  Matrix *To2w() { return &To2w_; }
  void To2w( const Matrix *T ) { To2w_ = *T; }
  int parent() const { return parent_; } // 0 if ordinary, -1 if parent is camera
  void parent(int p) { parent_ = p; }
  int id() const { return id_; }
  

  void draw( bool inpick = false );
  struct stuff *objstuff() { return st_; }

};

struct PvScene {
  Point center_;
  float censize_;

  std::vector <class PvObject> objs_;


  PvScene() {
     center_.x[0] = center_.x[1] = center_.x[2] = 0;
     censize_ = 1.0;
  }

  const Point *center() const { return &center_; }
  void center( const Point *p ) { center_ = *p; }
  float censize() const { return censize_; }
  void censize( float s ) { censize_ = s; }

  PvObject *obj( int id ) { return ((unsigned int)id < objs_.size()) ? &objs_[id] : 0; }
  struct stuff *objstuff( int id ) {
	PvObject *ob = obj(id);
	return ob ? ob->objstuff() : 0;
  }
  int nobjs() const { return objs_.size(); }

  PvObject *addobj( const char *name, int id = -1 );

  void draw( bool inpick = false );
};

// For queueing commands for network transmission
typedef enum { CMD_NONE=0, CMD_DATA=1, CMD_CONTROL=2 } CmdType;

class bufferedSocket : public arSocket {
   public:
       string buf;
       bool seeneof;
       int linelen;
       bufferedSocket *next;

       bufferedSocket() : arSocket(AR_STANDARD_SOCKET), buf(), seeneof(false), linelen(-1), next(NULL)
       {
           buf.reserve(100);
       }

       // Time to give up and close this socket?
       // Yes if seeneof *and* there's no data in the buffer.
       bool expired() const {
           return seeneof && linelen < 0;
       }

       // Check for data and append to string.
       // Returns length of available data, or -1 if none ready yet.
       // If we return -1 AND seeneof is true, it's time to toss this socket.
       //
       // If poll() return >= 0, you should use str() as a returned string,
       // and then call consume() to say you've finished using the data.
       int poll() {
           char tbuf[512];
           while(!seeneof && readable()) {
               int got = ar_read(tbuf, sizeof(tbuf));
               if(got < 0) {
                   seeneof = true;
                   if(!buf.empty()) {
                       linelen = buf.size();           // yes EOF, but we have data to hand off first.
                       buf.append( 1, '\0' );          // add terminating NUL
                   }
                   break;
               }
               buf.append( tbuf, got );                // append it to previous data
               if(linelen < 0) {
                   char *newline = (char *)memchr(tbuf, '\n', got);
                   if(newline != 0) {                  // Yep.
                       linelen = buf.size() - got + (newline - tbuf);
                       buf[linelen] = '\0';            // Replace final \n or \r\n with \0 or \0\0
                       if(linelen > 0 && buf[linelen] == '\r')
                           buf[linelen - 1] = '\0';

                   }
               }
               if(got < sizeof(tbuf))
                   break;
           }
           return linelen;
       }

       char *str() {
           return &buf[0];     // Return pointer to line, which should be a \000-terminated string.
                               // Only valid if linelen >= 0.
       }

       // Consume current line -- indicate that we no longer need the data returned by str().
       // Return value is just like poll():
       // If there's (another) line in the buffer (after the consumed one),
       // then return its length, else -1.
       int consume() {
           if(linelen >= 0) {
               // have any data following the \n?
               if(buf.size() > linelen+1) {
                   int newsize = buf.size() - (linelen+1);
                   memmove(&buf[0], &buf[linelen+1], newsize);

                   buf.resize( newsize );

                   char *newline = (char *)memchr(&buf[0], '\n', newsize);
                   if(newline) {
                       linelen = newline - &buf[0];
                       buf[linelen] = '\0';
                       if(linelen > 0 && buf[linelen-1] == '\r')
                           buf[linelen-1] = '\0';
                   } else {
                       linelen = -1;
                   }
               } else {
                   buf.resize(0);
                   linelen = -1;
               }
           }
           return linelen;
       }
};

class PpCmd {
  public:
    CmdType type;   
    int argc;
    char **argv;

    PpCmd() { type = CMD_DATA; argc = -1; argv = 0; }
    PpCmd( CmdType, int argc, char **argv );
    ~PpCmd();
    int frozenLen() const;	// length of serialized data including terminating 000
    int freezeInto( string &str ) const;
    int thawFrom( char *buf, int &offset, bool copystrings = true );
};

struct Ppszg : public arMasterSlaveFramework {
  PvScene *scene;

  // XXX  Could have a draw callback kinda like
  // XXX  void draw() {
  //		glClearColor( bgcolor.x[0],bgcolor.x[1],bgcolor.x[2], 1 );
  //		glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );	// or however you do background color in szg
  //		scene->draw();
  //	  }

  struct stuff *st;			// ref for currently selected Object

  Point bgcolor;
  float clipnear_, clipfar_;
  float focallen_;

  string cmdbuf_;			// master->slave command transfer buf
  int   quitnow_;


  Ppszg();
  ~Ppszg();

  void nearclip( float s ) { clipnear_ = s; }
  void farclip( float s ) { clipfar_ = s; }
  float nearclip() const { return clipnear_; }
  float farclip() const { return clipfar_; }

  void focallen(float dist) { focallen_ = dist; }
  float focallen() const { return focallen_; }

  char *snapfmt;
  int snapfno;
  float pickrange;

  float home[6];        //marx marx version 0.7.03 home is equivalent to a jump to a named point of view

  SClock *clk;		// master data clock

  vector <PpCmd> cmdqueue;

  /* camera animation stuff */
  struct wfpath path;
    int playing;
    int playevery;
    int playidling;
    float playspeed;
    int framebase;
    float playtimebase;
    int playloop;

  double timebasetime;

  std::vector <Subcam>  sc;
  int subcam;

  void queueCmd( bool datacommand, int argc, char *argv[] );

 // implementing arMasterSlaveFramework methods:

  virtual bool onStart( arSZGClient& SZGClient );
  virtual void onWindowStartGL( arGUIWindowInfo * );
  virtual void onPreExchange( void );	// called on master only
  virtual void onPostExchange( void );	// called on master + slaves
  virtual void onWindowInit( void );	// clear to background... ar_defaultWindowInitCallback()
  virtual void onDraw( arGraphicsWindow& win, arViewport& vp );
  virtual void onDisconnectDraw( void );
  virtual void onCleanup( void );
  virtual void onUserMessage( const string& messageBody );

  // arSocket stuff

  arSocket* listenSocket;
  bufferedSocket* socketList;
};

extern struct Ppszg  ppszg;

extern void ppszg_refresh( struct stuff * );
extern int specks_commandfmt( struct stuff **, const char *fmt, ... );

#endif /*_SZGPARTIVIEW_H*/
