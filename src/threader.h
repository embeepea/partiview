#ifndef THREADABLE_H
#define THREADABLE_H 1

/*
 * Generic class for something readable by a threaded assistant
 */

#include <stdio.h>
#include <pthread.h>
#include <errno.h>

template <class T> class Threader {

  private:
    bool threaded_;		// is a separate reader thread running?
    pthread_t th_;
    pthread_mutex_t mut_;	// lock on shared data
    char *name_;

  public:

    Threader() : threaded_(false), name_(NULL)
    {
	pthread_mutex_init( &mut_, NULL );
    }

    class Thunk {
	void *(T::*func_)(void);
	T *inst_;

	// private constructor -- thanks to Matt Hall
	Thunk( T *inst, void *(T::*func)() ) : inst_(inst), func_(func)
	{ }

      public:
	static Thunk *make( T *inst, void *(T::*func)(void) ) {
	    return new Thunk( inst, func );
	}

	static void *call( Thunk *deleteme ) {
	    Thunk t = *deleteme;
	    delete deleteme;
	    return ((t.inst_)->*(t.func_))();
	}
    };

  public:

    void start( T *inst, void *(T::*func)(void) ) {

	threaded_ = true;

	Thunk *g = Thunk::make( inst, func );
	pthread_create( &th_, NULL, (void *(*)(void *))&g->call, (void *)g );
    }

    pthread_mutex_t &mutex() { return mut_; }

    bool threading() const { return threaded_; }

    void lock() {
	if(threaded_) {
	    if(pthread_mutex_lock( &mut_ ) != 0) {
		fprintf(stderr, "%s->lock(): %s\n",
			name_ ? name_ : "Threader", strerror(errno));
	    }
	}
    }

    void unlock() {
	if(threaded_) {
	    if(pthread_mutex_unlock( &mut_ ) != 0) {
		fprintf(stderr, "%s->unlock(): %s\n",
			name_ ? name_ : "Threader", strerror(errno));
	    }
	}
    }
    void testcancel() {
	if(threaded_) pthread_testcancel();
    }

    void cancel() {
	if(threaded_) {
	    if(pthread_cancel( th_ ) != 0) {
		fprintf(stderr, "%s->cancel(0x%lx): %s\n",
			name_ ? name_ : "Threadable",
			(long int)th_, strerror(errno));
	    }
	}
    }

};

// Maybe convenient for above?
//  { scoped_lock lock(threader.mutex());
//    mess with threader's contents
//  }

class scoped_lock {
    pthread_mutex_t *m_;

  public:
    scoped_lock( pthread_mutex_t &mut ) {
	m_ = &mut;
	pthread_mutex_lock( m_ );
    }

    ~scoped_lock() {
	pthread_mutex_unlock( m_ );
    }
};

#endif /*THREADABLE_H*/
