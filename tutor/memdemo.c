#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>		/* for mmap() */
#include <sys/mman.h>

#include <signal.h>

/*
 * This structure is declared "volatile" below
 * to tell the compiler that its contents may change, even if this
 * program doesn't change them.
 */
struct mappedmem {
  volatile struct mappedmem *here; /* Address at which we'd like to be mapped.
  				 * If you map this segment at a different
				 *  address, any pointers must be adjusted!
				 * But maybe that's OK.
				 */
  long  maplen;			/* length of complete mmapped segment */

#define MAPPEDMEM_MAGIC 0xfeedc001
  int magic;			/* Magic number, so other files can't be
				 * confused with ours.  This amounts to a
				 * version-number on this shared-memory area,
				 * so change it if the layout changes.
				 */

  /* Pass one message with this simple handshake sequence:
   *  Consumer increments wantseq to show that it's ready for data,
   *    then waits for gotseq to catch up.
   *  Producer waits for wantseq > gotseq, then updates message buffer,
   *    and sets gotseq = wantseq to show that it's caught up.
   */
  int wantseq;
  int gotseq;

  /* Who's using this shared memory anyway?
   * Since processes might attach or detach at any time,
   * let's keep track of who used to be here.
   */
  int producer_pid, consumer_pid;

  /* The message to be passed */
  int msglen;
  char msgdata[1];	/* actually let's say it's longer than this,
			 * as given by msglen, but not to exceed
			 * the 'length' at the beginning of this header.
			 */
};
 

/* Test whether process exists without harming it */
int is_alive(int pid) {
  return(pid > 0 && !(kill(pid, 0) < 0 && errno == ESRCH));
}

/*
 * Create new shared-memory segment and map it into memory.
 */
volatile struct mappedmem *
newshm(char *mapfile, long maplen)
{
  static char zero = '\0';
  volatile struct mappedmem *shm;
  int fd;

  fd = open(mapfile, O_RDWR|O_CREAT|O_TRUNC, 0666);

  if(fd < 0) {
    fprintf(stderr, "Can't create shared-memory file %s: ", mapfile);
    perror("");
    return 0;
  }

  /* Extend file to proper length */
  lseek(fd, maplen-1, SEEK_SET);
  write(fd, &zero, 1);

  /* Map the file into memory.  Anywhere will do. */
  shm = (volatile struct mappedmem *)
	mmap( NULL, maplen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );

  if( (void *)shm == (void *)MAP_FAILED ) {
    fprintf(stderr, "Can't map %d bytes of shared-memory file %s: ",
	    maplen, mapfile);
    perror("");
    return 0;
  }

  /* Fill in blanks. */

  shm->here = shm;
  shm->maplen = maplen;
  shm->wantseq = 0;
  shm->gotseq = 0;

  /* Mark as valid */
  shm->magic = MAPPEDMEM_MAGIC;

  return shm;
}

/*
 * Open existing shared-memory segment and map it.
 */
volatile struct mappedmem *
oldshm(char *mapfile)
{
  int fd = open(mapfile, O_RDWR, 0666);
  volatile struct mappedmem *shm;
  struct mappedmem mm;
  long maplen;

  if(fd < 0) {
    fprintf(stderr, "Can't open shared-memory file %s: ", mapfile);
    perror("");
    return 0;
  }

  maplen = lseek(fd, 0, SEEK_END);	/* find length of file */
  lseek(fd, 0, SEEK_SET);

  if(read(fd, &mm, sizeof(mm)) != sizeof(mm)) {
    fprintf(stderr, "Can't even read %d-byte header from %s\n",
	sizeof(mm), mapfile);
  }
  if(mm.magic != MAPPEDMEM_MAGIC) {
    fprintf(stderr, "%s doesn't look like a shmem file: magic 0x%08x not 0x%08x\n",
	    mapfile, mm.magic, MAPPEDMEM_MAGIC);
    return 0;
  }
  if(maplen < mm.maplen) {
    fprintf(stderr, "%s is shorter (%d bytes) than its header claims (%d)!\n",
	mapfile, maplen, mm.maplen);
    return NULL;
  }

  /* Map the file into memory.  Ask, but don't demand, that it be
   * in the same place as the original creator had it mapped.
   * If we actually used pointers inside this shared-mem segment,
   * would MAP_SHARED below to MAP_SHARED|MAP_FIXED.
   */
  shm = (volatile struct mappedmem *)
	mmap( (void *)mm.here, mm.maplen,
		PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );

  if( (void *)shm == (void *)MAP_FAILED ) {
    fprintf(stderr, "Can't map %d bytes of shared-memory file %s: ",
	    maplen, mapfile);
    perror("");
    return NULL;
  }

  return shm;
}


/* 
 * Handshake.
 * This is the weak point of using shared memory,
 * since the operating system doesn't offer any assistance.
 * Could use another mechanism -- e.g. pushing bytes through
 * a socket, or signalling, or a semaphore -- to
 * let the OS do synchronization.
 */

int maxdelay = 50000;
int pollusec = 10000;

/* Wait for (*from - *to) >= thresh.
 * First time, spin for up to "maxdelay" loops.
 * After that, just poll every "pollusec" microseconds.
 */
int await(volatile int *from, volatile int *to, int thresh,
					volatile int *peerp,
					char *peername)
{
  int count = maxdelay;
  int peer = -1, deadpeer = -1;

  do {
    if(*from - *to >= thresh)
	return 1;
  } while(--count >= 0);

  for(;;) {
    usleep(pollusec);

    if(*from - *to >= thresh)
	return 2;

    /*
     * Try to detect whether original sender is still attached
     * to this segment.  Could exit if not, but maybe
     * someone else will attach later.
     */
    if(peerp && deadpeer != (peer = *peerp) && !is_alive(peer)) {
	deadpeer = peer;
	if(deadpeer > 0 && peername != NULL)
	    fprintf(stderr, "%s process %d is gone!\n",
		peername, deadpeer);
    }
  }
}

int
produce(volatile struct mappedmem *shm) {
  char line[512];
  int len;
  int gotany;

  fprintf(stderr,
	"Starting producer -- reading lines of text from standard input\n");

  /* Is another producer already using this shm segment? */
  if( is_alive(shm->producer_pid) ) {
    fprintf(stderr, "Producer process %d is already using same segment!\n",
	shm->producer_pid);
    return 0;
  }
  shm->producer_pid = getpid();

  do {
    /* Get something to say */
    gotany = (fgets(line, sizeof(line), stdin) != NULL);

    /* Wait until receiver is ready: until shm->wantseq - shm->gotseq >= 1. */
    await(&shm->wantseq, &shm->gotseq, 1, &shm->consumer_pid, "consumer");

    /* If we got an end-of-file on input,
     * pass EOF (msglen == -1) to consumer.
     */
    if(gotany) {
	shm->msglen = strlen(line);
	strcpy(shm->msgdata, line);
    } else {
	shm->msglen = -1;
    }
    shm->gotseq = shm->wantseq;

  } while(gotany);
  return 1;
}

int
consume(volatile struct mappedmem *shm) {
  fprintf(stderr, "Starting consumer -- passing received msgs to stdout\n");

  /* Is another consumer already using this shm segment? */
  if( is_alive(shm->consumer_pid) ) {
    fprintf(stderr, "Consumer process %d is already using same segment!\n",
	shm->consumer_pid);
    return 0;
  }
  shm->consumer_pid = getpid();

  /* Post initial request */
  shm->wantseq = shm->gotseq + 1;

  /* Wait until sender has filled our request: gotseq >= wantseq */
  while(await(&shm->gotseq, &shm->wantseq, 0, &shm->producer_pid, "producer")) {

    /* Consume our message */
    if(shm->msglen < 0) {
	printf("[EOF]\n");
	break;
    }
    printf("[%d] %s", shm->msglen, shm->msgdata);

    /* Ask sender for next message */
    shm->wantseq = shm->gotseq + 1;
  }
  return 1;
}

main(int argc, char *argv[])
{
  int maplen = 8*1024;
  char *mapfile = "/var/tmp/Mapfile";
  volatile struct mappedmem *shm = NULL;
  int be_producer = 0;
  int be_new = 0;
  int c;

  if(argc <= 1) {
    fprintf(stderr, "Usage: %s [-p | -c] [-n]  [nbytes] [filename]\n\
Demonstrate communicating via shared memory.\n\
  memdemo -p   starts a \"producer\" process, which reads\n\
		lines of text from its standard input and sends\n\
		them via shared memory to ...\n\
  memdemo -c   a \"consumer\" process, which takes those messages\n\
		and prints them to standard output.\n\
Other options:\n\
  -n   \"new\"  Create new shared-memory file, even if one\n\
		already exists.  (Default: only create if needed.)\n\
  nbytes	Length of shared-memory segment, default %d bytes.\n\
  filename	Name of shared-memory disk file, default %s\n",
	argv[0], maplen, mapfile);
    exit(1);
  }

  while((c = getopt(argc, argv, "cpn")) != EOF) {
    switch(c) {
    case 'c': be_producer = 0; break;
    case 'p': be_producer = 1; break;
    case 'n': be_new = 1; break;
    }
  }

  if(argc > optind) maplen = atoi(argv[optind]);
  if(argc > optind+1) mapfile = argv[optind+1];

  /* Try to open existing shared-memory segment.  If none, create it. */
  if(!be_new && access(mapfile, W_OK) == 0)
    shm = oldshm(mapfile);
  if(shm == NULL)
    shm = newshm(mapfile, maplen);
  if(shm == NULL)
    exit(1);

  if(be_producer) {
    produce(shm);
  } else {
    consume(shm);
  }
  return 0;
}
