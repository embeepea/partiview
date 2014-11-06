#ifndef USE_NETHACK
void nethack_init() { }
#else /* do USE_NETHACK */

/*
 * Tacky code for over-the-network control of partiview.
 * Use multi- or unicast UDP.
 *
 * Stuart Levy, slevy@ncsa.uiuc.edu
 * National Center for Supercomputing Applications,
 * University of Illinois 2001.
 * This file is part of partiview, released under the
 * Illinois Open Source License; see the file LICENSE.partiview for details.
 */
#include "config.h"

#include "plugins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "findfile.h"

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
  #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
extern char *alloca ();
#   endif
#  endif
# endif
#endif

#include <FL/Fl.H>

#include "specks.h"
#include "partiviewc.h"
#include "partiview.H"

struct sockaddr_in netsin;

int netfd = -1;

struct netops {
    int jumps;
    int cmds;
};

static struct netops listening = { 1, 1 };
static struct netops broadcasting = { 0, 0 };

static int net_parse_args( struct stuff **, int argc, char *argv[], char *fname, void * );
static void dorecv(int fd, void *junk);
static int setnet(struct sockaddr_in *sinp, int ttl);
static int sendjump( float txyz[3], float rxyz[3] );
static int sendcmd( int argc, char *argv[] );
static void netupdate();
static void navtrace();
static void cmdtrace( struct stuff **, int argc, char *argv[] );
void aer2rxyz( float rxyz[3], const float aer[3] );
void rxyz2aer( float aer[3], const float rxyz[3] );

static int netupdates;

void nethack_init() {
    parti_add_commands( net_parse_args, "net", NULL );
}

int net_parse_args( struct stuff **, int argc, char *argv[], char *fname, void * ) {

  int i;
  struct sockaddr_in sin;
  struct hostent *hp;
  struct netops *ops;
  static char Usage[] = "net addr IPADDR/PORT | listen [[-]nav] [[-]cmd] | broadcast [[-]nav] [[-]cmd]";

  if(argc < 1 || strcmp(argv[0], "net"))
      return 0;

  if(argc <= 1) {
      msg(Usage);
      return 1;
  }

  switch(argv[1][0]) {
    case 'a':
	if(argc > 2) {
	    int port = 7490, ttl = 2;
	    char *cp, *ep;
	    cp = strchr(argv[2], '/');
	    ttl = 2;
	    if(cp) {
		*cp = '\0';
		port = strtol(cp+1, &ep, 0);
		if(ep != cp && *ep == '/')
		    ttl = strtol(ep+1, NULL, 0);
	    }
	    sin.sin_port = htons( port );
	    if(inet_aton(argv[2], &sin.sin_addr)) {
		sin.sin_family = AF_INET;
	    } else if((hp = gethostbyname(argv[2])) != NULL) {
		memcpy(&sin.sin_addr, hp->h_addr_list[0], sizeof(sin.sin_addr));
		sin.sin_family = hp->h_addrtype;
	    } else {
		msg("%s: net addrs: %s: unknown host\n", fname, argv[2]);
		break;
	    }
	    setnet( &sin, ttl );
	}
	msg("net addr %s/%d", inet_ntoa( netsin.sin_addr ), htons( netsin.sin_port ));
	break;
    
    case 'l':
    case 'b':
	ops = (argv[1][0] == 'l') ? &listening : &broadcasting;
	for(i = 2; i < argc; i++) {
	    char *cp = &argv[i][0];
	    int on = 1;
	    if(*cp == '-') on = 0, cp++;
	    if(*cp == '+') on = 1, cp++;
	    switch(*cp) {
		case 'j': ops->jumps = on; break;
		case 'c': ops->cmds = on; break;
		case 'o':
			  if(!strcmp(cp, "on")) ops->jumps = ops->cmds = 1;
			  else if(!strcmp(cp, "off")) ops->jumps = ops->cmds = 0;
			  break;
	    }
	}
	msg("net %s %cjumps %ccmds",
		argv[1][0]=='l' ? "listen" : "broadcast",
		"-+"[ops->jumps], "-+"[ops->cmds]);
	break;

    case 'j': {
	Point xyz;
	float aer[3], rxyz[3];
	Matrix c2w;
	int sendit = 0;
	parti_getc2w( &c2w );
	tfm2xyzaer( &xyz, aer, &c2w );
	aer2rxyz( rxyz, aer );

	if(argc > 2 && argv[2][0] == 'h') {
	    sendit = 1;
	} else if(argc > 5) {
	    sendit = getfloats( &xyz.x[0], 3, 2, argc, argv );
	    sendit += getfloats( &rxyz[0], 3, 5, argc, argv );
	}
	if(sendit)
	    sendjump( &xyz.x[0], &rxyz[0] );
	break;
      }

    case 'c':
	sendcmd( argc-2, argv+2 );
	break;

    default:
	msg(Usage);
	break;
  }
  netupdate();
  return 1;
}

static void netupdate() {
    netupdates = 0;
    if(netsin.sin_addr.s_addr == INADDR_ANY) {
	ppui.drawtrace = NULL;
	parti_cmdtrace( NULL );
 	return;
    }
    ppui.drawtrace = broadcasting.jumps ? navtrace : NULL;
    parti_cmdtrace( broadcasting.cmds ? cmdtrace : NULL );
}

void aer2rxyz( float rxyz[3], const float aer[3] ) {
    rxyz[0] = aer[0];
    rxyz[2] = aer[1];
    rxyz[1] = aer[2];
}

void rxyz2aer( float aer[3], const float rxyz[3] ) {
    aer[0] = rxyz[0];
    aer[1] = rxyz[2];
    aer[2] = rxyz[1];
}

static void navtrace() {
    static Matrix oldc2w;
    Matrix c2w;

    if(!broadcasting.jumps)
	return;

    parti_getc2w( &c2w );
    if(netupdates == 0 || memcmp(&c2w, &oldc2w, sizeof(c2w)) != 0) {
	Point pos;
	float aer[3], rxyz[3];
	oldc2w = c2w;
	netupdates++;
	tfm2xyzaer( &pos, aer, &c2w );
	aer2rxyz( rxyz, aer );
	sendjump( pos.x, rxyz );
    }
}

static void cmdtrace( struct stuff **, int argc, char *argv[] ) {
    if(!broadcasting.cmds)
	return;
    if(argc>1 && !strcmp(argv[0], "net"))	/* unsafe to broadcast "net ..." cmds */
	return;
    sendcmd( argc, argv );
}

static int setnet(struct sockaddr_in *sinp, int ttl) {
    int multi = IN_CLASSD( ntohl( sinp->sin_addr.s_addr ) );
    static int on = 1;

    if(netfd >= 0) {
	Fl::remove_fd( netfd );
	close(netfd);
	netfd = -1;
    }

    netfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(netfd < 0) {
	perror("socket");
	return -1;
    }

    fcntl(netfd, F_SETFL, fcntl(netfd, F_GETFL, 0) | O_NONBLOCK|O_NDELAY);


#ifdef SO_REUSEPORT
    if(setsockopt(netfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
	perror("setsockopt: SO_REUSEPORT");
    }
#else /* if (as in Linux) no SO_REUSEPORT, try SO_REUSEADDR at least */
    if(setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	perror("setsockopt: SO_REUSEADDR");
    }
#endif /*SO_REUSEPORT*/

    if(bind(netfd, (struct sockaddr *)sinp, sizeof(*sinp)) < 0) {
	struct sockaddr_in bsin = *sinp;
	bsin.sin_addr.s_addr = INADDR_ANY;
	if(bind(netfd, (struct sockaddr *)&bsin, sizeof(bsin)) < 0) {
	    fprintf(stderr, "bind to %s: ", inet_ntoa(sinp->sin_addr));
	    perror("");
	    return -1;
	}
    }

    if(multi) {
	unsigned char mttl = ttl;
	static int bufsize = 32768;
	static unsigned char nope = 0;
	struct ip_mreq mr;

	if(mttl < 2) mttl = 2;

	if(setsockopt(netfd, IPPROTO_IP, IP_MULTICAST_LOOP, &nope, 1) < 0) {
	    perror("disabling IP_MULTICAST_LOOP");
	}

	if(setsockopt(netfd, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, 1) < 0) {
	    perror("setting IP_MULTICAST_TTL");
	}

	if(setsockopt(netfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(int)) < 0) {
	    perror("multicast SO_SNDBUF");
	}


	if(setsockopt(netfd, IPPROTO_IP, IP_MULTICAST_TTL, &mttl, 1) < 0) {
	    perror("setting IP_MULTICAST_TTL");
	}

#ifdef IP_ADD_MEMBERSHIP
	mr.imr_interface.s_addr = INADDR_ANY;
	mr.imr_multiaddr.s_addr = sinp->sin_addr.s_addr;
	if (setsockopt(netfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
	    fprintf(stderr, "IP_ADD_MEMBERSHIP: can't join multicast group %s",
		    inet_ntoa(sinp->sin_addr));
	    perror("");
	}
#endif /*IP_ADD_MEMBERSHIP*/

    }

    netsin = *sinp;

    Fl::add_fd( netfd, dorecv, NULL );
    netupdate();
    return 1;
}

#define PV_MAGIC 0x19570118
#define PV_JUMP  ('j'<<24 | 'u'<<16 | 'm'<<8 | 'p')
#define PV_CMD   ('c'<<24 | 'm'<<16 | 'd'<<8 | 0)

struct jumper {
    int magic;
    int kind;
    float txyz[3];
    float rxyz[3];
};

void htonlfloats( float *dst, float *src, int count ) {
    int i;
    for(i = 0; i < count; i++)
	*(int *)&dst[i] = htonl( *(int *)&src[i] );
}

static int sendjump( float txyz[3], float rxyz[3] ) {
    struct jumper jump;

    if(netfd < 0)
	return 0;
    jump.magic = htonl(PV_MAGIC);
    jump.kind = htonl(PV_JUMP);
    htonlfloats( jump.txyz, txyz, 3 );
    htonlfloats( &jump.rxyz[0], &rxyz[0], 3 );
    if(sendto( netfd, &jump, sizeof(jump), 0,  (struct sockaddr *)&netsin, sizeof(netsin) ) < 0) {
	perror("sendjump: sendto");
	return -1;
    }
    return 1;
}

static int sendcmd( int argc, char *argv[] ) {
    if(netfd < 0)
	return 0;
    char *str = rejoinargs( 0, argc, argv );
    int len = strlen(str);
    int totlen = 3*sizeof(int) + len + 1;
    int *buf = (int *) alloca( 3*sizeof(int) + len + 1 );
    buf[0] = htonl(PV_MAGIC);
    buf[1] = htonl(PV_CMD);
    buf[2] = htonl( strlen(str) );
    memcpy( (char *)&buf[3], str, len+1 );
    if(sendto( netfd, buf, totlen, 0,  (struct sockaddr *)&netsin, sizeof(netsin) ) < 0) {
	perror("sendcmd: sendto");
	return -1;
    }
    return 1;
}

static void receivejump( struct jumper *jump, int len, struct sockaddr_in *srcsin ) {
    Matrix c2w;
    Point xyz;
    float aer[3], rxyz[3];

    if(len < sizeof(struct jumper)) {
	msg("short net jump from %s", inet_ntoa(srcsin->sin_addr));
	return;
    }
    if(!listening.jumps)
	return;
    if(broadcasting.jumps) {
	msg("Avoiding loop with %s: setting net broadcast -jump",
			inet_ntoa(srcsin->sin_addr));
	broadcasting.jumps = 0;
	netupdate();
    }
    htonlfloats( &xyz.x[0], jump->txyz, 3 );
    htonlfloats( &rxyz[0], jump->rxyz, 3 );
    rxyz2aer( aer, rxyz );
    xyzaer2tfm( &c2w, &xyz, aer );
    parti_setc2w( &c2w );
}

static void receivecmd( int *buf, int len, struct sockaddr_in *srcsin ) {
    int slen = ntohl(buf[2]);
    if(3*sizeof(int) + slen > len) {
	msg("short net cmd from %s (len = %d expected %d)",
			inet_ntoa(srcsin->sin_addr),
			len, 3*sizeof(int) + slen);
	return;
    }
    if(!listening.cmds)
	return;
    if(broadcasting.cmds) {
	msg("Avoiding loop with %s: setting net broadcast -cmd",
			inet_ntoa(srcsin->sin_addr));
	broadcasting.cmds = 0;
	netupdate();
    }

    char *tstr = (char *)alloca( slen+1 );
    char *argv[512];
    int argc = tokenize( (char *)&buf[3], tstr, COUNT(argv), argv, NULL );
    specks_parse_args( &ppui.st, argc, argv );
    parti_redraw();
}

#ifndef __BITS_SOCKET_H
# define  socklen_t  int
#endif

static void dorecv(int fd, void *junk) {
    struct sockaddr_in sfrom;
    socklen_t sfromlen = sizeof(sfrom);
    int len;
    int buf[16384];
    int kind;

    for(;;) {
	/* remember, we set this socket to non-blocking mode */
	/* loop to receive all that's available */
	len = recvfrom(fd, (char *)buf, sizeof(buf), 0,
	    		(struct sockaddr *)&sfrom, &sfromlen);
	if(len <= 0)
	    return;

	if(ntohl( buf[0] ) != PV_MAGIC) {
	    msg("net: bad magic 0x%x", ntohl( buf[0] ));
	    return;
	}
	kind = ntohl( buf[1] );
	switch(kind) {
	    case PV_JUMP:
		receivejump( (struct jumper *)buf, len, &sfrom );
		break;
	    case PV_CMD:
		receivecmd( buf, len, &sfrom );
		break;
	}
    }
}
#endif /*USE_NETHACK*/
