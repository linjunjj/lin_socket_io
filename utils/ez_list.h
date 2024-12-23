#ifndef __EZ_LIST_H_
#define __EZ_LIST_H_

#include <list>
#include "mutex.h"

namespace lin_io
{
template<typename T>
class List
{
private:
	List(const List&);
	void operator=(const List&);
public:
    //typedef typename list<T>::iterator iterator;
    //typedef typename list<T>::value_type value_type;
    //typedef list<T> inner_type;

    List(const unsigned int max = 0);
    virtual ~List();

    bool push_back(const T&);
    bool push_front(const T&);
    bool pop_front(T&);
    bool pop_back(T&);

    void set_capacity(const unsigned int max);
    unsigned int capacity();
    unsigned int size();

    void clear();
	void getall(std::list<T>&);

protected:
	SpinLock _lock;
	std::list<T> _list;
    unsigned int _max;
};

template<typename T>
List<T>::List(const unsigned int max)
:_max(max==0 ? 0xFFFFFFFF : max)
{
}

template<typename T>
List<T>::~List()
{
}

template<typename T>
bool List<T>::push_back(const T& t)
{
    ScopLock<SpinLock> sync(_lock);
    if(_list.size() < _max)
    {
        _list.push_back(t);
        return true;
    }

    return false;
}

template<typename T>
bool List<T>::push_front(const T& t)
{
    ScopLock<SpinLock> sync(_lock);
    if(_list.size() < _max)
    {
        _list.push_front(t);
        return true;
    }

    return false;
}

template<typename T>
bool List<T>::pop_front(T& t)
{
    ScopLock<SpinLock> sync(_lock);
    if(!_list.empty())
    {
		t = _list.front();
		_list.pop_front();
        return true;
    }

    return false;
}

template<typename T>
bool List<T>::pop_back(T& t)
{
    ScopLock<SpinLock> sync(_lock);
    if(!_list.empty())
    {
        t = _list.back();
        _list.pop_back();
        return true;
    }

    return false;
}

template<typename T>
unsigned int List<T>::size()
{
    ScopLock<SpinLock> sync(_lock);
    return (unsigned int)_list.size();
}

template<typename T>
void List<T>::set_capacity(const unsigned int max)
{
    ScopLock<SpinLock> sync(_lock);
	_max = max == 0 ? 0xFFFFFFFF : max;
}

template<typename T>
unsigned int List<T>::capacity()
{
    ScopLock<SpinLock> sync(_lock);
    return _max;
}

template<typename T>
void List<T>::clear()
{
    ScopLock<SpinLock> sync(_lock);
    _list.clear();
}

template<typename T>
void List<T>::getall(std::list<T>& data)
{
	ScopLock<SpinLock> sync(_lock);
	data = _list;
}

}
#endif
