#ifndef __EZ_VECTOR_H_
#define __EZ_VECTOR_H_

#include <vector>
#include "mutex.h"

namespace lin_io
{
template<typename value_type>
class Vector
{
	typedef typename std::vector<value_type>::iterator iterator;
public:
	Vector();
	virtual ~Vector();

	bool empty();
	void clear();
	unsigned int size();
	
	void push_back(const value_type&);
	void push_front(const value_type&);

	int get(const int i,value_type&);//返回个数
	int find(const value_type&);//返回个数
	int erase(const value_type&);//返回个数

	int erase(std::vector<value_type>&);
	int getall(std::vector<value_type>&);

private:
	SpinLock _lock;
	std::vector<value_type> _vector;
};

template<typename value_type>
Vector<value_type>::Vector()
{
}

template<typename value_type>
Vector<value_type>::~Vector()
{
}

template<typename value_type>
void Vector<value_type>::push_back(const value_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	_vector.push_back(value);
}

template<typename value_type>
void Vector<value_type>::clear()
{
	ScopLock<SpinLock> sync(_lock);
	_vector.clear();
}

template<typename value_type>
unsigned int Vector<value_type>::size()
{
	ScopLock<SpinLock> sync(_lock);
	return (unsigned int)_vector.size();
}

template<typename value_type>
bool Vector<value_type>::empty()
{
	ScopLock<SpinLock> sync(_lock);
	return _vector.empty();
}

template<typename value_type>
int Vector<value_type>::get(const int i, value_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	
	if (i >= 0 && i < (int)_vector.size())
	{
		value = _vector[i];
		return 1;
	}

	return 0;
}

template<typename value_type>
int Vector<value_type>::erase(const value_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	int size(0);
	for (iterator it = _vector.begin(); it != _vector.end(); it++)
	{
		if (*it == value)
		{
			size++;
			_vector.erase(it);
			continue;
		}
	}
	return size;
}

template<typename value_type>
int Vector<value_type>::find(const value_type& value)
{
	ScopLock<SpinLock> sync(_lock);
	int size(0);
	for (iterator it = _vector.begin(); it != _vector.end(); it++)
	{
		if (*it == value)
		{
			size++;
			continue;
		}
	}
	return size;
}

template<typename value_type>
int Vector<value_type>::erase(std::vector<value_type>& values)
{
	ScopLock<SpinLock> sync(_lock);
	values = _vector;
	_vector.clear();
	return (int)values.size();
}

template<typename value_type>
int Vector<value_type>::getall(std::vector<value_type>& values)
{
	ScopLock<SpinLock> sync(_lock);
	values = _vector;
	return (int)values.size();
}
}

#endif
