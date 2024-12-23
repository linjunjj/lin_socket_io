#ifndef __NIO_REFCOUNT_H__
#define __NIO_REFCOUNT_H__


#include <assert.h>
#include <iostream>

#include "atomic.h"
#include "int_types.h"

namespace lin_io
{
using namespace std;
/**
 *	where is the counter 	
 *	1. intrusive		
 *		class Counter{ int _count ; }  , 
 *		Foo : public Counter{...}
 *		shared_ptr<Foo> { Foo * _ptr ; }
 *  2. external		
 *		class Counter{ int _count ; }	
 *		shared_ptr<Foo> { Counter * _count = new Counter() ; Foo * _ptr ; }
 *	3. object memory
 *		new Foo() =>  alloc(sizeof(Foo)+sizeof(int))  [count| foo instance]
 *													        ^
 *															Foo *
 *	4. implicit
 *		managed all reference into a link list
 *
 * 						intrusive 	external 	object memory 	implicit
 *	existing practices 	poor 		best 		good 			best
 *	ease of use 		best 		good 		best 			good
 *	performance 		best 		poor 		best 			best
 *	concurrency 		best 		best 		best 			poor
 *
 */

///	缺点: 入侵，不支持多继承
///	优点: 对象本身能访问自己的计数
///	like shared_ptr<T> of stl , RcVar<T>采样继承模式，入侵强，多继承支持不好;　shared_ptr<T>采样组合方式，兼容性好，性能差一些
///	shared_ptr的问题:
///	shared_ptr对指针只能初始化一次!!一旦初始化，对象生命完全交予该shared_ptr，以后只能通过shared_ptr<T>对象赋值，所有参数传递和存储都只能是shared_ptr<T>/weak_ptr<T>(
///	从某种意义上来说，这也是一种入侵，并且更严重)
///	更严重的是，对象本身对自身的引用计数一无所知，很难做到在对象本身的方法中回调外部函数(可能会释放该对象)时，通过堆栈持有引用：
///	Foo::fireEvent(){
///		RcVar<Foo>  ref(this) ; 	//	save a reference in stack
///		callback(...) ;			//	callback would release Foo
///	}
///	无法这样: T * p = new T(); shared_ptr<T> a(p) ; shard_ptr<T> b(p) ; 不支持 shared_ptr<T> a = T *
///	要想做到对象能获取到自身的refcount, 对此boost提供了一个enable_shared_from_this<T>,要求T继承它(又回到继承模式的入侵了)
/// 因此，简单资源对象用shared_ptr方便，控制对象特别是产生事件/回调的对象宁愿用继承方式
///
class RefCount {
public:
    RefCount()
        : ref_(0)
    {
    }
    virtual ~RefCount() { assert(ref_ == 0); }
    //	get the current count of the object reference, just for debug
    int32_t _refcount() const { return ref_; }
protected:
    // increase the count of the object reference
    inline void _retain() const { ++ref_; }
    //decrease the count of the object reference
    inline void _release(bool destroy = true) const
    {
        assert(ref_ > 0);
        bool alive = (--ref_) > 0;
        if (!alive && destroy)
            delete this;
    }
    //	只允许RcVar<T>调用_retain(),_release()
    template <class T>
    friend class RcVar;

private:
    mutable int32_t ref_;

    // forbid copy and assignment of the RC object
    RefCount(const RefCount&);
    void operator=(const RefCount&);
};

class LockedRefCount {
public:
    LockedRefCount()
        : ref_(0)
    {
    }
    virtual ~LockedRefCount() { assert(ref_ == 0); }
    //	get the current count of the object reference, just for debug
    int32_t _refcount() const { return ref_; }
protected:
    // increase the count of the object reference
    inline void _retain() const
    {
        increment_int32(reinterpret_cast<volatile int32_t*>(&ref_), 1);
    }
    //decrease the count of the object reference
    inline void _release(bool destroy = true) const
    {
        assert(ref_ > 0);
        bool alive = increment_int32(reinterpret_cast<volatile int32_t*>(&ref_), -1) > 0;
        if (!alive && destroy)
            delete this;
    }

    //	只允许RcVar<T>调用_retain(),_release()
    template <class T>
    friend class RcVar;

private:
    mutable int32_t ref_;

    // forbid copy and assignment of the RC object
    LockedRefCount(const LockedRefCount&);
    void operator=(const LockedRefCount&);
};

template <class T>
class RcVar {
public:
    explicit RcVar()
        : ptr_(0)
    {
    }
    explicit RcVar(T* ptr)
        : ptr_(ptr)
    {
        if (ptr_)
            ptr_->_retain();
    }
    RcVar(const RcVar<T>& var)
    {
        ptr_ = var.ptr();
        if (ptr_)
            ptr_->_retain();
    }
    virtual ~RcVar()
    {
        if (ptr_)
            ptr_->_release();
    }

    // operator =() functions
    inline RcVar<T>& operator=(const RcVar<T>& var)
    {
        T* ptr = var.ptr();
        if (ptr != ptr_) {
            T* oldptr = ptr_;
            ptr_ = ptr;
            if (ptr_)
                ptr_->_retain();
            if (oldptr)
                oldptr->_release();
        }
        return *this;
    }

    inline RcVar<T>& operator=(T* ptr)
    {
        T* oldptr = ptr_;
        ptr_ = ptr;
        if (ptr_)
            ptr_->_retain();
        if (oldptr)
            oldptr->_release();
        return *this;
    }

    // operator ->() funcion
    inline T* operator->() const
    {
        assert(ptr_ != 0);
        return ptr_;
    }

    //	operator .() function
    inline operator T&()
    {
        assert(ptr_ != 0);
        return *ptr_;
    }
    inline operator T&() const
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline bool is_nil() const { return (ptr_ == (T*)0); }

    inline operator T*() const { return ptr_; }

    // conversion function
    inline T& ref()
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline T* ptr() const { return ptr_; }

    //	detach
    inline T* _retn()
    {
        T* ret = ptr_;
        ptr_ = 0;
        ret->_release(false);
        return ret;
    }

private:
    T* ptr_;
};

/// like auto_ptr<T> of stl , 这个完全可以用auto_ptr<T>替代
template <class T>
class VarVar {
public:
    explicit VarVar()
        : ptr_(0)
    {
    }
    explicit VarVar(T* ptr)
        : ptr_(ptr)
    {
        assert(ptr != 0);
    }

    virtual ~VarVar() { delete ptr_; }

    // operator = function
    VarVar<T>& operator=(T* v)
    {
        if (ptr_)
            delete ptr_;
        ptr_ = v;
        return *this;
    }

    inline T* operator->()
    {
        assert(ptr_ != 0);
        return ptr_;
    }
    inline const T* operator->() const
    {
        assert(ptr_ != 0);
        return ptr_;
    }

    // conversion function
    inline operator T&() const
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline operator const T&() const
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline operator T&()
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline operator const T*() const
    {
        return ptr_;
    }

    // conversion function
    inline T* ptr() const { return ptr_; }
    inline T& ref()
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline T* in() const
    {
        assert(ptr_ != 0);
        return ptr_;
    }

    inline T*& out()
    {
        delete ptr_;
        ptr_ = 0;
        return ptr_;
    }

    inline T& inout()
    {
        assert(ptr_ != 0);
        return *ptr_;
    }

    inline bool is_nil() const
    {
        return (ptr_ == (T*)0);
    }

    //	detach
    inline T* _retn()
    {
        T* ret = ptr_;
        ptr_ = 0;
        return ret;
    }
private:	
	VarVar(const VarVar<T>&) ;
	VarVar<T>& operator=(const VarVar<T>&);	

	T*		ptr_;
};

}	//	namespace core

#endif	//	__NIO_REFCOUNT_H__
