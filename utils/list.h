#pragma once

#include <list>
#include "mutex.h"

namespace lin_io
{
template <typename T>
class LockFreeQueue
{
public:
	LockFreeQueue() {}
	virtual ~LockFreeQueue() {}
	void push_back(T* data);
	T* pop_front(void);
	bool empty(void);
	uint32_t size();

private:
    SpinLock _lock;
    std::list<T*> _list;
};

template <typename T>
void LockFreeQueue<T>::push_back(T* data)
{
	ScopLock<SpinLock> sync(_lock);
	_list.push_back(data);
}

template <typename T>
T* LockFreeQueue<T>::pop_front()
{
	ScopLock<SpinLock> sync(_lock);
	if(_list.empty())
	{
		return NULL;
	}
	auto data = _list.front();
	_list.pop_front();
	return data;
}

template <typename T>
bool LockFreeQueue<T>::empty()
{
	ScopLock<SpinLock> sync(_lock);
    return _list.empty();
}

template <typename T>
uint32_t LockFreeQueue<T>::size()
{
	ScopLock<SpinLock> sync(_lock);
    return _list.size();
}

}
