#pragma once

#include <stdexcept>
#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <atomic>

namespace lin_io
{
class SpinLock
{
public:
	SpinLock()
	{
		_lock.clear();
	}
	virtual ~SpinLock()
	{
	}
	inline
	void lock()
	{
		while(_lock.test_and_set());
	}
	inline
	void unlock()
	{
		_lock.clear();
	}
private:
	std::atomic_flag _lock;
};

inline uint64_t now()
{
    timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

class Mutex
{
public:
	Mutex()
	{
		pthread_mutex_init(&_mutex, 0);
	}
	virtual ~Mutex()
	{
		pthread_mutex_destroy(&_mutex);
	}

	inline
	void lock()
	{
		pthread_mutex_lock(&_mutex);
	}

	inline
	bool trylock()
	{
		return pthread_mutex_trylock(&_mutex)==0 ? true : false;
	}

	inline
	bool trylock(long milliseconds)
	{
		timespec ts ;
		ts.tv_sec = milliseconds / 1000 ;
		ts.tv_nsec = milliseconds * 1000 * 1000 ;
		return pthread_mutex_timedlock(&_mutex,&ts)==0 ? true : false;
	}

	inline
	void unlock()
	{
		pthread_mutex_unlock(&_mutex);
	}

private:
	Mutex(const Mutex&);
	void operator=(const Mutex&);
public:
	pthread_mutex_t _mutex;
};

//	scope lock/unlock
template <class M>
class ScopLock
{
	ScopLock(const ScopLock&);
	void operator=(const ScopLock&);

	M &	_mtx ;
public:
	ScopLock(M& m) : _mtx(m)
	{
		_mtx.lock();
	}
	virtual ~ScopLock()
	{
		_mtx.unlock();
	}
	void lock()
	{
		_mtx.lock();
	}
	void unlock()
	{
		_mtx.unlock();
	}
};
typedef	ScopLock<Mutex>	Synchronized;

}

