#ifndef TCPSOCK_H
#define TCPSOCK_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus

#include <istream>
#include <ostream>
#include <streambuf>
#include <iosfwd>
#include <iostream>
#include <ext/stdio_filebuf.h>	// GNU C++ extension -- for making a filebuf from a unix file descriptor

class fdistream : public std::istream {
    int fd_;
    FILE *f_;
    typedef __gnu_cxx::stdio_filebuf<char>  stdio_filebuf;
    stdio_filebuf fdsb_;

  public:

    fdistream( int fd );
    ~fdistream();
};

class TCPsocket {
    int fd_;
    enum sockstate { INVALID, LISTENING, CONNECTING, CONNECTED } state_;
    int backlog_;
    bool nonblock_;
    struct sockaddr_in sin_;

    bool mksock( const char *name, bool nonblock );
    bool do_accept();
    void connectwith( const char *name, int port, bool nonblock );
    void listenwith( int listenport, bool await_first_contact, int backlog, bool nonblock );


  public:
    TCPsocket( const char *name, int port, bool nonblock = false );	// connect()
    TCPsocket( int listenport, bool await_first_contact = true, int backlog = 5, bool nonblock = false ); // listen()
    TCPsocket( TCPsocket &listener, bool nonblock = false );
    ~TCPsocket();

    bool valid() const { return state_ != INVALID; }
    enum sockstate state() const { return state_; }
    bool connected() const { return state_ == CONNECTED; }
    bool listening() const { return state_ == LISTENING; }

    void nonblock( bool nb );

    bool be_connected();				// wait for connection; false on failure

    int fd() const { return fd_; }			// suitable for select()ing

    bool ready();
    bool await( int usecs = -1 );

    fdistream *new_istream() {
	if(!be_connected())
	    return 0;
	return new fdistream( fd() );
    }
};

#define EXTERNC  extern "C"

#else /* not C++, just plain C */

struct TCPsocket;	/* opaque structure */

#define EXTERNC  extern

#endif

/* C interface */
EXTERNC struct TCPsocket *tcpconnect( char *name, int port, int nonblock );
EXTERNC struct TCPsocket *tcplisten( int port, int await_first_contact, int backlog, int nonblock );
EXTERNC struct TCPsocket *tcpaccept( struct TCPsocket *listener, int nonblock );

EXTERNC void  tcpclose( struct TCPsocket *ts );
EXTERNC int   tcpvalid( struct TCPsocket *ts );
EXTERNC int   tcpconnected( struct TCPsocket *ts );
EXTERNC int   tcplistening( struct TCPsocket *ts );
EXTERNC void  tcpnonblock( struct TCPsocket *ts, int nonblock );
EXTERNC int   tcpbe_connected( struct TCPsocket *ts );
EXTERNC int   tcpfd( struct TCPsocket *ts );
EXTERNC int   tcpawait( struct TCPsocket *ts, int maxusecs );	/* any data ready? wait up to that long, -1 => forever */

#undef EXTERNC


#endif /*TCPSOCK_H*/
