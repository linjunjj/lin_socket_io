/**
 *  coroutine implement
 *  . 使用 libc的 makecontext/swapcontext 实现,在移植性方面,这几个调用被很多系统废弃
 *    但gcc目前还支持，也能正常使用
 *  . 原语：
 *    co create(function) throws exceptions   --  create a child coroutine
 *    co runing()                             --  return current coroutine
 *    Arg yeild() throws exceptions           --  switch child coroutine to main coroutine
 *    bool resume(co,Arg) throws exceptions   --  switch main coroutine to child coroutine
 *    . 创建协程，堆栈默认8K
 *    . 参数传递, 只支持单向( main -> child )
 *    . 参数类型: c/c++对动态参数支持不好，简单使用 void *
 *  . use split stack: 堆栈根据需要动态分配，这样创建每个协程初始只需要最小堆栈(如8K)
 *    1. #define  USE_SPLIT_STACKS
 *    2. gcc -fsplit-stack , gcc version > 4.6 (ubuntu 12.04默认gcc版本为4.6)
 *
 *  . 典型用法:
      in coroutine : 
        result = rpc(args){
                  send request(args)
                  addtimer(timeout)
                  return yeild()               <-- main coroutine trigger by i/o:   resume(co,result)   / 
                                                   main coroutine trigger by timer: resume(co,"timeout")
                 }

 */

#ifndef __EASY_COROUTINE_H__
#define __EASY_COROUTINE_H__

#include <stdio.h>  
#include <ucontext.h>  
#include <unistd.h>  
#include <exception>
#include <tr1/functional>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

#include "thread.h"
#include "utils/rc.h"
#include "handler.h"
#include "log/logger.h"

//#define USE_SPLIT_STACKS      //  use split stack:  gcc -fsplit-stack

#ifdef  USE_SPLIT_STACKS
#define SPLIT_STACK_CTXOFF    10
extern "C"
{
void *__splitstack_makecontext( std::size_t,
		void * [SPLIT_STACK_CTXOFF],
		std::size_t *);

void __splitstack_releasecontext( void * [SPLIT_STACK_CTXOFF]);

void __splitstack_resetcontext( void * [SPLIT_STACK_CTXOFF]);

void __splitstack_block_signals_context( void * [SPLIT_STACK_CTXOFF],
		int * new_value, int * old_value);
void __splitstack_getcontext( void * [SPLIT_STACK_CTXOFF]);

void __splitstack_setcontext( void * [SPLIT_STACK_CTXOFF]);
}
#endif

namespace lin_io
{

class CoroutineException : public std::runtime_error
{
public:
	CoroutineException(const std::string& str) : std::runtime_error(str){}
	virtual ~CoroutineException() throw() {}
};

class Coroutine : public RefCount
{
public:
	class Timer : Handler
	{
	public:
		Timer(Coroutine* caller) : _caller(caller) {}
		virtual ~Timer()
		{
			stop();
		}
		virtual void start(int tm)
		{
			select_timeout(tm);
			Coroutine::yield();
		}
		virtual void stop()
		{
			select_timeout();
		}
	private:
		virtual void handle(int ev)
		{
			try
			{
				if(_caller)
				{
					Coroutine::resume(_caller);
				}
			}
			catch(const std::exception& e)
			{
				GLERROR << "exception throw from callback! " << e.what();
			}
		}
	private:
		Coroutine* _caller;
	};

	enum Stat
	{
		SUSPENDED = 0,
		RUNNING = 1,
		NORMAL = 2,
		DEAD = 3
	};
	typedef void*  Arg ;
	typedef std::tr1::function<void()> Proc;

	static void msleep(int tm)
	{
		Timer timer(Coroutine::running());
		timer.start(tm);
	}

private:

#   ifdef USE_SPLIT_STACKS
	typedef void* segments_context[SPLIT_STACK_CTXOFF];
#   endif    

	struct Stack
	{
		size_t                      size;        //  stack
		void*                       boundary;    //  stack boundary
#     ifdef USE_SPLIT_STACKS
		segments_context            ssctx;        //  split stack context
#     endif

		Stack(size_t sz=8*1024)
		{
#       ifdef USE_SPLIT_STACKS
			void* limit = __splitstack_makecontext(sz, ssctx, &size);
			if(!limit) throw CoroutineException("allocate stack failed");
			boundary = limit;
			int off = 0;
			__splitstack_block_signals_context( ssctx, & off, 0);
#       else
			int total(sz);
			int pagesize = getpagesize();
			if((pagesize%sz) != 0)
			{
				sz  = (sz/pagesize +1) * pagesize;
			}
			boundary = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
			if(!boundary) throw CoroutineException("allocate stack failed");
			size = sz;
			//printf("--------------total: %u boundary size: %u page size: %d\n", total, size, pagesize);
#       endif        
		}
		virtual ~Stack()
		{
#       ifdef USE_SPLIT_STACKS        
			__splitstack_releasecontext(ssctx);
#       else
			munmap(boundary, size);
#       endif      
		}
	};

	//  context for main coroutine
	struct MainContext
	{
		ucontext_t ctx;
#     ifdef USE_SPLIT_STACKS
		segments_context ssctx;
#     endif
		MainContext()
		{
			getcontext(&ctx);
#       ifdef USE_SPLIT_STACKS
			__splitstack_getcontext(ssctx);
#       endif
		}

		virtual ~MainContext()
		{
		}
	};

	static ThreadLocal<Coroutine>    _running;
	static ThreadLocal<MainContext>  _main;

	ucontext_t                  _context;
	Stack                       _stack;
	Arg                         _arg;     /// arguments btween coroutines
	Proc                        _routine;
	Stat                        _stat;
	std::map<pthread_key_t, void *> _tls;

	static void _release_main_context(void * p)
	{
		delete ((MainContext*)p);
	}
	static MainContext* _get_main_context()
	{
		MainContext* ctx = _main.get();
		if(!ctx)
		{
			ctx = new MainContext();
			_main.set(ctx);
		}
		return ctx;
	}

	static void _proc(Coroutine* self)
	{
		assert(self);
		RcVar<Coroutine> ref(self);   /// save a reference in stack
		//GLINFO << "coroutine: " << self << "--------------------size: " << self->_refcount();

		assert(self->_stat == SUSPENDED);
		self->_stat = RUNNING;   /// SUSPENDED -> RUNNING
		_running.set(self);

		try
		{
			//GLINFO << "coroutine: " << self << "--------------------stat RUNNING: begin coroutine task";
			(self->_routine)();
		}
		catch(std::exception& e)
		{
			GLERROR << "exception throw in coroutine " << e.what();
		}
		catch(...)
		{
			GLERROR << "exception throw in coroutine";
		}

		assert(_running.get()==self);

		//GLINFO << "coroutine: " << self << "--------------------stat DEAD: end coroutine task";
		self->_stat = DEAD;
		_running.set(0);
		/// jump to last resume caller
	}

	Coroutine(Proc proc, size_t stacksize=8*1024)
	:_stack(stacksize),_arg(0),_routine(proc),_stat(SUSPENDED)
	{
		assert(proc);

		if(_running.get()!=0)
		{
			/// only allow create coroutine in main coroutine
			throw CoroutineException("only allow create coroutine in main coroutine");
		}

		MainContext* main = _get_main_context();

		getcontext(&_context);
		_context.uc_stack.ss_sp = _stack.boundary;
		_context.uc_stack.ss_size = _stack.size;
		_context.uc_link = &main->ctx;
		makecontext(&_context,reinterpret_cast<void (*)(void)>(_proc), 1, this);
	}

public:
	virtual ~Coroutine()
	{
		assert(_stat==DEAD);
		assert(_running.get() != this);
	}

	/// @brief  返回当前状态
	///         'suspended'   协程刚创建还未执行
	///         'running'     执行中(当前协程是该协程)
	///         'normal'      挂起中(当前协程是该协程)
	///         'dead'        协程过程执行结束
	const char* status() const
	{
		switch(_stat)
		{
		case SUSPENDED : return "suspended";
		case RUNNING : return "running";
		case NORMAL : return "normal";
		case DEAD : return "dead";
		default : return "unknown";
		}
	}

	/// @brief  创建一个挂起的协程，可以通过resume()唤醒执行它
	/// @param  proc    协程过程
	/// @return 返回创建成功的协程指针.
	///          [!] 在调用resume()之前,调用方需要持有它的引用
	static Coroutine* create(Proc proc, size_t stackSize=8*1024)
	{
		assert(stackSize>=4*1024 && stackSize<=8*1024*1024);
		Coroutine* co = new Coroutine(proc, stackSize);
		return co;
	}

	/// @brief  返回当前执行的coroutine, 如果返回null为主协程
	static Coroutine* running()
	{
		return _running.get();
	}

	/// @brief  切换到主协程, 直到主协程调用resume()时切换回来(返回)
	///          yield()只允许在子协程中调用
	/// @return 返回主协程调用resume()时传入的参数
	static Arg yield()
	{
		MainContext* main = _main.get();
		if(!main)
		{
			throw CoroutineException("coroutine happen error, main routine not created!");
		}

		Coroutine* self = _running.get();
		if(!self)
		{
			throw CoroutineException("coroutine happen error, only allow call yield in child coroutine");
		}
		if(self->_stat != RUNNING)
		{
			throw CoroutineException("coroutine happen error, illegal state");
		}

		self->_stat = NORMAL;

#     ifdef USE_SPLIT_STACKS
		__splitstack_getcontext( self->_stack.ssctx );
		__splitstack_setcontext( main->ssctx );
#     endif

		//GLINFO << "coroutine: " << self << "--------------------stat NORMAL: swapcontext to main";
		int ret = swapcontext((ucontext_t *)&self->_context, &main->ctx);

#     ifdef USE_SPLIT_STACKS
		__splitstack_setcontext( self->_stack.ssctx);
#     endif

		//GLINFO << "coroutine: " << self << "--------------------stat RUNNING: return child to run";
		self->_stat = RUNNING;
		_running.set((Coroutine *)self);      /// jump back or jump falied

		if(ret!=0)
		{
			throw CoroutineException("coroutine happen error, dead coroutine");
		}
		Arg arg = self->_arg;

		//self->_arg = 0 ;
		return arg ;
	}

	/// only allow call resume in main coroutine
		/// @brief  切换到目标协程|co| , 直到co过程调用yeild()或co过程执行完时切换回来(返回)
		///          resume()只允许在主协程中调用
		/// @param  co    目标协程
		/// @param  arg   传递给目标协程的参数
		/// @return true : switch成功并成功跳回(co过程还未结束)
		///          false : co过程在resume()返回前已经结束,两种情况都存在:
		///                  1. 调用resume前co过程已经结束.
		///                  2. 调用resume前co过程还在执行，但本次跳转过去后co过程执行完(跳回之前co过程结束)
	static bool resume(Coroutine* co, Arg arg = 0)
	{
		assert(co);
		RcVar<Coroutine> ref(co);   /// save a reference,insure co not destroy when jump back
		//GLINFO << "coroutine: " << co << "--------------------size: " << co->_refcount();

		MainContext* main = _main.get();
		if(!main)
		{
			throw CoroutineException("coroutine happen error, main routine not created!");
		}

		if(_running.get()!=0)
		{
			throw CoroutineException("coroutine happen error, only allow call resume in main coroutine");
		}

		assert(co->_stat != RUNNING);  //  stat == SUSPENDED / NORMAL / DEAD
		if(co->_stat == DEAD)
			return false;

		//co->_stat = RUNNING ;
		co->_arg = arg;

#     ifdef USE_SPLIT_STACKS
		__splitstack_getcontext(main->ssctx);
		__splitstack_setcontext(co->_stack.ssctx);
#     endif

		//GLINFO << "coroutine: " << co << "--------------------swapcontext main->child[_proc]";
		int ret = swapcontext(&main->ctx, &co->_context);

#     ifdef USE_SPLIT_STACKS
		__splitstack_setcontext(main->ssctx);
#     endif

		//GLINFO << "coroutine: " << co << "--------------------resume main to run";
		_running.set(0);
		if(ret!=0)
			co->_stat = DEAD;

		return ret==0;
	}

	inline
	int setspecific(pthread_key_t __key, void* __pointer)
	{
		_tls[__key] = __pointer;
		return 0;
	}
	inline void *getspecific(pthread_key_t __key)
	{
		return _tls[__key];
	}
	inline void deletespecific(pthread_key_t __key)
	{
		_tls.erase(__key);
	}
};

//执行协程的类
struct __co
{
	template<typename Fn>
	void operator,(Fn fn)
	{
		Coroutine *co = Coroutine::create(fn);
		//GLINFO << "coroutine: " << co << "--------------------create and resume";
		Coroutine::resume(co);
	}
	__co() {}
private:
	__co(const __co&) {}
};

template<typename T>
class LocalStorage
{
public:
	typedef void (*Destructor)(void* ptr);
	LocalStorage(Destructor destructor = 0) { pthread_key_create(&_key, (Destructor)destructor); }
	virtual ~LocalStorage() { pthread_key_delete(_key);}
	inline T* get()
	{
		if(Coroutine::running())
		{
			return static_cast<T*>(Coroutine::running()->getspecific(_key));
		}
		return static_cast<T*>(pthread_getspecific(_key));
	}
	inline void set(T *p)
	{
		if(Coroutine::running())
		{
			Coroutine::running()->setspecific(_key, p);
		}
		pthread_setspecific(_key, p);
	}
private:
	pthread_key_t _key;
};

template<typename T>
struct tss
{
	static void set(T *t)
	{
		_tls.set(t);
	}
	static T* get()
	{
		return static_cast<T*>(_tls.get());
	}
	static LocalStorage<T> _tls;
};

template<typename T> LocalStorage<T> tss<T>::_tls;
}

#define go lin_io::__co(),

#endif
