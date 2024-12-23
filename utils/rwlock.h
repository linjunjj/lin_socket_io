#pragma once

#include <pthread.h>

namespace rwlock
{
class RWLock
{
public:
	RWLock()
	{
		pthread_rwlock_init(&_rwLock, 0);
	}
	virtual ~RWLock()
	{
		pthread_rwlock_destroy(&_rwLock);
	}

	inline
	void rdlock()
	{
		pthread_rwlock_rdlock(&_rwLock);
	}

	inline
	void wrlock()
	{
		pthread_rwlock_wrlock(&_rwLock);
	}

	inline
	void unlock()
	{
		pthread_rwlock_unlock(&_rwLock);
	}

private:
	RWLock(const RWLock&);
	void operator=(const RWLock&);
public:
	pthread_rwlock_t _rwLock;
};

class ReadLock
{
	ReadLock(const ReadLock&);
	void operator = (const ReadLock&);

private:
	RWLock& _rwLock;

public:
	ReadLock(RWLock& rwLock): _rwLock(rwLock)
	{
		_rwLock.rdlock();
	}

	virtual ~ReadLock()
	{
		_rwLock.unlock();
	}
};

class WriteLock
{
	WriteLock(const WriteLock&);
	void operator = (const WriteLock&);

private:
	RWLock& _rwLock;

public:
	WriteLock(RWLock& rwLock): _rwLock(rwLock)
	{
		_rwLock.wrlock();
	}

	virtual ~WriteLock()
	{
		_rwLock.unlock();
	}
};
}
