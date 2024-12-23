#ifndef __NIO_THREAD_H__
#define __NIO_THREAD_H__

#include <assert.h>
#include <stdexcept>
#include <stdio.h>
#include <cxxabi.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include "utils/mutex.h"
#include "utils/atomic.h"
#include "log/logger.h"

namespace lin_io
{
template <typename T>
class ThreadLocal
{
public:
	typedef void (*Destructor)(void* ptr);
	ThreadLocal(Destructor destructor = 0) { pthread_key_create(&_key, (Destructor)destructor); }
	virtual ~ThreadLocal() { pthread_key_delete(_key);}
	inline T* get() {return static_cast<T*>(pthread_getspecific(_key));}
	inline void set(T* p) {pthread_setspecific(_key, p); }
	inline void finalize() {} //	do nothing , pthread api support auto destructor callback when thread exit
private:
	pthread_key_t _key;
};

class Condition : public Mutex
{
 public:
   Condition()
   {
     pthread_cond_init(&_cond, 0);
   }
   virtual ~Condition()
   {
     pthread_cond_destroy(&_cond);
   }
   /// @return false : timeout or error
   bool  wait(int32_t ms=-1)
   {
     int r;
     if( ms<0 )
     {
       r = pthread_cond_wait(&_cond,&_mutex);
     }
     else
     {
       struct timeval  now;
       struct timespec ts;  //  绝对时间
       gettimeofday(&now, NULL);
       ts.tv_sec = now.tv_sec + ms / 1000;
       ts.tv_nsec = ( now.tv_usec + ms%1000 * 1000 ) * 1000;
       r = pthread_cond_timedwait(&_cond,&_mutex,&ts);
     }
     return !r ;
   }
   void notify()
   {
     pthread_cond_signal(&_cond);
   }
   void notifyAll()
   {
     pthread_cond_broadcast(&_cond);
   }
 private:
   pthread_cond_t  _cond;
 };

///	thread
typedef void* (*THREAD_ROUTINE)(void*);
class Thread
{
public:
    Thread(bool autoDelete = true)
        : _auto_delete(autoDelete)
        , _tid(0)
        , _quit(0)
        , _aliveGuard(0)
    {
    }

    virtual ~Thread()
    {
        if (_aliveGuard)
            *_aliveGuard = 0;
    }

    void start()
    {

    	int err = pthread_create(&_tid, NULL,thread_routine, this);
        if (err != 0)
            throw std::runtime_error("pthread_create()");
    }

    void stop()
    {
        exchange_int32(reinterpret_cast<volatile int32_t*>(&_quit), 1);
        //	send cancle signal , thread proc will call pthread_testcancel to check if in cancel state
        //  in glibc , pthread_cancel can raise a exception in thread rountine to break running! , and the exception
        //  must NOT ignored!(if catched it in your code , must rethrow it)
        pthread_cancel(_tid);
    }

    void join(int timeout = -1)
    {
        pthread_join(_tid, NULL);
    }

    bool quiting() { return compare_and_swap_int32(reinterpret_cast<volatile int32_t*>(&_quit), 1, 1) == 1; }

    pthread_t tid() const { return _tid; }

private:
    //	to implement the routine
    virtual void run() {};

private:
    bool _auto_delete; //  是否允许线程函数结束时自动销毁Thread实例
    pthread_t _tid;
    uint32_t _quit;
    uint8_t* _aliveGuard; //

    static void* thread_routine(void* arg)
    {
        sigset_t set;
        sigemptyset(&set);        
        sigaddset(&set, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &set, NULL);

        Thread* th = static_cast<Thread*>(arg);
        assert(th);

        uint8_t alive = 1;
        th->_aliveGuard = &alive;

        try
        {
            th->run();
        }
        catch (std::exception& ex)
        {
        	GLERROR << "thread routine happen error: " << ex.what();
        }
        catch (...)
        {
        	GLERROR << "thread routine happen unknown error";
        }

        if (alive)
        {
            th->_aliveGuard = 0;
            if (th->_auto_delete)
                delete th;
        }

        return 0;
    }

    Thread(const Thread&);
    const Thread& operator=(const Thread&);
};
}

#endif
