#ifndef _EASY_UTILITY_TEMPLATE_H_
#define _EASY_UTILITY_TEMPLATE_H_

#include <cstddef>

///避免和std tr1\c++0x的冲突
namespace lin_io
{
class noncopyable
{
 protected:
    noncopyable() {}
    virtual ~noncopyable() {}
 private:  // emphasize the following members are private
    noncopyable( const noncopyable& );
    const noncopyable& operator=( const noncopyable& );
};

template<class T, T v>
struct integral_constant {
	static const T value = v;
	typedef T value_type;
	typedef integral_constant<T, v> type;
};


template <class T, T v> const T integral_constant<T, v>::value;

typedef char (&yes)[2];
typedef char (&no)[1];

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template <class T> struct is_pointer : false_type {};
template <class T> struct is_pointer<T*> : true_type {};

template <class T, class U> struct is_same : public false_type {};
template <class T> struct is_same<T,T> : true_type {};

template<class> struct is_array : public false_type {};
template<class T, size_t n> struct is_array<T[n]> : public true_type {};
template<class T> struct is_array<T[]> : public true_type {};

template <class T> struct is_non_const_reference : false_type {};
template <class T> struct is_non_const_reference<T&> : true_type {};
template <class T> struct is_non_const_reference<const T&> : false_type {};

template <class T> struct is_const : false_type {};
template <class T> struct is_const<const T> : true_type {};

template <class T> struct is_void : false_type {};
template <> struct is_void<void> : true_type {};

template< class T > struct remove_pointer                    {typedef T type;};
template< class T > struct remove_pointer<T*>                {typedef T type;};
template< class T > struct remove_pointer<T* const>          {typedef T type;};
template< class T > struct remove_pointer<T* volatile>       {typedef T type;};
template< class T > struct remove_pointer<T* const volatile> {typedef T type;};

template<typename T> struct remove_const { typedef T     type; };
template<typename T> struct remove_const<T const> { typedef T     type; };


template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> { typedef T type; };

template<typename B,typename D>
class is_base_of {
	static yes check(const B*);
	static no check(const void*);
public:
	enum {
		value = sizeof(check(static_cast<const D*>(0))) == sizeof(yes),
	};
};

template<typename T>
typename enable_if<is_pointer<T>::value, T>::type*
destroy(T& t) {
	if (t) {delete t ;t = 0;};
	return 0;
}
template<typename T>
typename enable_if<!is_pointer<T>::value>::type*
destroy(T& t) {
	return 0;
}

template<typename T>
typename enable_if<is_pointer<T>::value, typename remove_pointer<T>::type >::type*
copy(T& t) {
	return t;
}

template<typename T>
typename enable_if<!is_pointer<T>::value, typename remove_pointer<T>::type >::type*
copy(T& t) {
	return new T(t);
}

template<typename T>
typename enable_if<is_pointer<T>::value, typename remove_pointer<T>::type >::type
ref(T& t) {
	return *t;
}

template<typename T>
typename enable_if<!is_pointer<T>::value, typename remove_pointer<T>::type >::type
ref(T& t) {
	return t;
}

template<typename T>
typename enable_if<is_pointer<T>::value, typename remove_pointer<T>::type >::type*
ptr(T& t) {
	return t;
}

template<typename T>
typename enable_if<!is_pointer<T>::value, typename remove_pointer<T>::type >::type*
ptr(T& t) {
	return &t;
}

template<typename F>
class Executor;

template<typename T, typename Arg1, typename Arg2>
class Executor<void(T::*)(const Arg1&, Arg2&)> {
public:
	typedef Arg1	A1;
	typedef Arg2    A2;
	Executor(void(T::*method)(const Arg1&, Arg2&)):method_(method)
	{
	}
	void operator () (T* obj, const A1* p1, A2* p2,...) {
		(obj->*method_)(*p1, *p2);
	}
	virtual ~Executor()
	{
	}
private:
	void(T::*method_)(const Arg1&, Arg2&);
};

template<typename T, typename Arg1>
class Executor<void(T::*)(const Arg1&)> {
public:
	typedef Arg1		A1;
	typedef void        A2;
	Executor(void(T::*method)(const Arg1& )):method_(method)
	{
	}
	void operator () (T* obj, const A1* p1,...) {
		(obj->*method_)(*p1);
	}
	virtual ~Executor()
	{
	}
private:
	void(T::*method_)(const Arg1&);
};

template<typename T>
struct SmallObject {
	static const bool value = (sizeof(T) < 1024);
};
template<>
struct SmallObject<void> {
	static const bool value = false;
};
template<typename T, bool b = SmallObject<T>::value >
struct Stack : public noncopyable {};
template<typename T>
struct Stack<T,true> : public noncopyable{
	Stack() {}
	virtual ~Stack() {}
	T* ptr() {return &s_;}
	T s_;
private:
};
template<typename T>
struct Stack<T,false> : public noncopyable{
	Stack():s_(new T()) {}
	virtual ~Stack() {if (s_) {delete s_;}}
	T* ptr() {return s_;}
	T *s_;
};
template<>
struct Stack<void> : public noncopyable {
	void* ptr() {
		return 0;
	}
};

}

#endif /* YAF_UTILITY_TEMPLATE_H_ */
