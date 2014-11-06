#include "config.h"

#ifdef HAVE_THREADS

#include "tcpsocket.h"
#include <sys/time.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>

#include <ext/stdio_filebuf.h>

using namespace std;

static const char *serr() {
    return strerror(errno);
}

bool TCPsocket::mksock( const char *name, bool nb ) {

    fd_ = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if(fd_ < 0) {
	fprintf(stderr, "TCPsocket(\"%s\"): %s\n", name, serr());
	return false;
    }
    nonblock( nb );
    return true;
}

void TCPsocket::nonblock( bool nb ) {
    nonblock_ = nb;
    if(nonblock_) {
	fcntl( fd_, F_SETFL, fcntl( fd_, F_GETFL ) | O_NONBLOCK );
    } else {
	fcntl( fd_, F_SETFL, fcntl( fd_, F_GETFL ) & ~O_NONBLOCK );
    }
}
    

TCPsocket::~TCPsocket() {
    if(fd_ >= 0)
	close(fd_);
}

void TCPsocket::connectwith( const char *name, int port, bool nonblock )
{
    state_ = INVALID;
    fd_ = -1;

    const char *s = strchr(name, ':');
    if(s == name) {
	listenwith( atoi(s+1), !nonblock, 1, nonblock );
	return;

    } else if(s != NULL) {
	char *ts = new char[ s - name + 1 ];
	memcpy( ts, s, s - name );
	ts[s-name] = '\0';
	connectwith( ts, atoi(s+1), nonblock );
	delete ts;
	return;
    }
	
    if(port <= 0 || port >= 65536) {
	::fprintf(stderr, "TCPsocket(): port %d out of range\n", port);
	return;
    }

    memset(&sin_, 0, sizeof(sin_));
    sin_.sin_family = AF_INET;
    sin_.sin_port = htons(port);   

    if(inet_aton( name, &sin_.sin_addr )) {
	/* ok */
    } else {
	struct hostent *hp = gethostbyname( name );
	if(hp == NULL) {
	    const char *s = hstrerror(h_errno);
	    fprintf(stderr, "TCPsocket(\"%s\") -- %s\n", name, s);
	    return;
	}
	memcpy(&sin_.sin_addr, &hp->h_addr, sizeof(struct in_addr));
    }

    if(!mksock( name, nonblock ))
	return;

    if(connect( fd_, (struct sockaddr *)&sin_, sizeof(sin_) ) == 0) {
	state_ = CONNECTED;
    } else if(errno == EAGAIN || errno == EINPROGRESS) {
	state_ = CONNECTING;
    } else {
	fprintf(stderr, "TCPsocket(\"%s\"[%s], %d): %s\n",
		name, inet_ntoa(sin_.sin_addr),
		port, serr());
	state_ = INVALID;
    }
}
    

TCPsocket::TCPsocket( const char *name, int port, bool nonblock )
{
    connectwith( name, port, nonblock );
}


TCPsocket::TCPsocket( int listenport, bool await_first_contact, int backlog, bool nonblock )
{
    listenwith( listenport, await_first_contact, backlog, nonblock );
}

void TCPsocket::listenwith( int listenport, bool await_first_contact, int backlog, bool nonblock )
{
    state_ = INVALID;
    fd_ = -1;

    if(listenport <= 0 || listenport >= 65536) {
	fprintf(stderr, "TCPsocket(%d): port out of range\n", listenport);
	return;
    }

    memset(&sin_, 0, sizeof(sin_));

    sin_.sin_family = AF_INET;
    sin_.sin_addr.s_addr = INADDR_ANY;
    sin_.sin_port = htons(listenport);

    if(!mksock( "(listen)", nonblock ))
	return;

    static int one = 1;
    if(setsockopt( fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) ) < 0) {
	fprintf(stderr, "TCPsocket(%d): setsockopt: %s\n", listenport, serr());
	/* non-fatal */
    }

    if(bind(fd_, (struct sockaddr *)&sin_, sizeof(sin_)) < 0) {
	fprintf(stderr, "TCPsocket(%d): bind: %s\n", listenport, serr());
	return;
    }

    backlog_ = backlog && !await_first_contact;
    if(listen(fd_, backlog_) < 0) {
	fprintf(stderr, "TCPsocket(%d): listen: %s\n", listenport, serr());
	return;
    }
    state_ = LISTENING;

    if(await_first_contact) {
	do_accept();
    }
}

bool TCPsocket::do_accept() {

    socklen_t len = sizeof(sin_);
    int newfd;

    fcntl( fd_, F_SETFL, fcntl( fd_, F_GETFL ) & ~O_NONBLOCK );
    do {
	newfd = accept( fd_, (struct sockaddr *)&sin_, &len );
    } while(newfd < 0 && errno == EINTR);

    if(newfd < 0) {
	fprintf(stderr, "TCPsocket(%d, true): accept: %s\n", ntohs(sin_.sin_port), serr());
	state_ = INVALID;
	return false;
    }
    close(fd_);
    fd_ = newfd;
    state_ = CONNECTED;

    nonblock( nonblock_ );
    return true;
}

bool TCPsocket::be_connected()
{
    nonblock( false );
    switch(state_) {
    case INVALID: return false;
    case CONNECTED: return true;
    case CONNECTING: return await();	// wait until connection succeeds or fails
    case LISTENING: return do_accept();
    default:	    return false;
    }
}

bool TCPsocket::ready()
{
    if(!valid())
	return false;

    fd_set fds;
    static struct timeval zero = { 0, 0 };

    FD_ZERO(&fds);
    FD_SET( fd_, &fds );
    return select( fd_+1, &fds, NULL, NULL, &zero ) > 0;
}

bool TCPsocket::await( int usecs )
{
    if(!valid())
	return false;

    fd_set fds;

    FD_ZERO(&fds);
    FD_SET( fd_, &fds );

    if(usecs < 0) {
	/* wait indefinitely */
	return select( fd_+1, &fds, NULL, NULL, NULL ) > 0;
    } else {
	struct timeval awhile = { usecs / 1000000, usecs % 1000000 };
	return select( fd_+1, &fds, NULL, NULL, &awhile ) > 0;
    }
}

TCPsocket::TCPsocket( TCPsocket & listener, bool nonblock )
{
    state_ = INVALID;
    fd_ = -1;

    if(!listener.valid())
	return;

    socklen_t sl = sizeof(sin_);

    do {
	fd_ = accept( listener.fd(), (struct sockaddr *)&sin_, &sl );
    } while(fd_ < 0 && errno == EINTR);

    if(fd_ < 0) {
	fprintf(stderr, "TCPsocket(listening(%d)): accept: %s\n",
		ntohs(listener.sin_.sin_port), serr());
	return;
    }

    if(nonblock)
	fcntl( fd_, F_SETFL, fcntl( fd_, F_GETFL ) | O_NONBLOCK );
    else
	fcntl( fd_, F_SETFL, fcntl( fd_, F_GETFL ) & ~O_NONBLOCK );
    state_ = CONNECTED;
}

fdistream::fdistream( int fd ) : 
		fd_(fd),
		f_( fdopen(fd, "rb") ),
		fdsb_( f_,  std::ios::in | std::ios::binary, 0 ),
	        istream( &fdsb_ )
{ }

fdistream::~fdistream()
{
    if(f_)
	fclose(f_);
}

/* plain C interface */

struct TCPsocket *tcpconnect( char *name, int port, int nonblock )
{
    return new TCPsocket( name, port, nonblock );
}

struct TCPsocket *tcplisten( int port, int await_first_contact, int backlog, int nonblock ) {
    return new TCPsocket( port, await_first_contact, backlog, nonblock );
}

struct TCPsocket *tcpaccept( struct TCPsocket *listener, int nonblock ) {
    return new TCPsocket( *listener, nonblock );
}

void  tcpclose( struct TCPsocket *ts ) {
    delete ts;
}

int   tcpvalid( struct TCPsocket *ts ) {
    return ts && ts->valid();
}

int   tcpconnected( struct TCPsocket *ts ) {
    return ts && ts->state_ == CONNECTED;
}

int   tcplistening( struct TCPsocket *ts ) {
    return ts && ts->state_ == LISTENING;
}

void  tcpnonblock( struct TCPsocket *ts, int nonblock ) {
    return ts && ts->nonblock_;
}

int   tcpbe_connected( struct TCPsocket *ts ) {
    return ts->be_connected();
}

int   tcpfd( struct TCPsocket *ts ) {
    return ts->fd();
}

int   tcpawait( struct TCPsocket *ts, int maxusecs ) {
    return ts->await( maxusecs );
}

#endif /*HAVE_THREADS*/
