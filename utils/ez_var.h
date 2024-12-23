#ifndef __EZ_VAR_H_
#define __EZ_VAR_H_

#include <vector>
#include "mutex.h"

namespace lin_io
{
template<typename value_type>
class Var
{
public:
	Var()
	{
	}
	Var(const value_type& v):_var(v)
	{
	}
	Var(const Var<value_type>& v):_var(v)
	{
	}

	value_type get() const
	{
		ScopLock<SpinLock> sync(const_cast<SpinLock&>(_lock));
		return _var;
	}

	void operator=(const value_type& src)
	{
		ScopLock<SpinLock> sync(_lock);
		_var = src;
	}
	void operator=(const Var<value_type>& src)
	{
		ScopLock<SpinLock> sync(_lock);
		_var = src.get();
	}

	bool operator==(const value_type& src) const
	{
		ScopLock<SpinLock> sync(const_cast<SpinLock&>(_lock));
		return this->_var==src ? true : false;
	}
	bool operator==(const Var<value_type>& src) const
	{
		ScopLock<SpinLock> sync(const_cast<SpinLock&>(_lock));
		return this->_var==src.get() ? true : false;
	}

private:
	SpinLock _lock;
	value_type _var;
};

}

#endif
