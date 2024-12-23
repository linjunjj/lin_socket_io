#ifndef _NET_FUTURE_H__
#define _NET_FUTURE_H__

#include <vector>
#include <string>

#include "utils/rc.h"
#include "utils/mutex.h"
#include "utils/atomic.h"
#include "utils/utility.h"
#include "utils/exception.h"
#include "thread.h"
#include "coroutine.h"
#include "worker.h"
#include "handler.h"

namespace net
{
template<class T>
class SimpleTimer : Handler
{
protected:
	typedef	void (T::*Callback)() ;
public:
	SimpleTimer(T * obj, Callback cb) :_obj(obj),_cb(cb) {}
	virtual ~SimpleTimer()
	{
	}
	virtual void start(const int tm)
	{
		select_timeout(tm) ;
	}
	virtual void stop()
	{
		close_timeout();
	}
private:
	virtual void handle(const int ev)
	{
		try
		{
			if(_obj)
			{
				(_obj->*_cb)();
			}
		}
		catch(const std::exception & e)
		{
		}
	}
private:
	T *		 _obj;
	Callback _cb;
};

struct Cond
{
	enum status
	{
		ready,
		wait,
	};
	virtual ~Cond() {}
	virtual void init() = 0;
	virtual void yield() = 0;
	virtual bool resume() = 0;
};

struct ConditionWrapper : public Cond
{
public:
	ConditionWrapper():_status(wait) {}
	virtual ~ConditionWrapper() {}

	virtual void init() {_status=wait;}

	virtual void yield()
	{
		lin_io::ScopLock<lin_io::Condition> sync(_cond);
		while(_status == wait)
		{
			_cond.wait();
		}
	}
	virtual bool resume()
	{
		lin_io::ScopLock<lin_io::Condition> sync(_cond);
		if(_status != wait)
		{
			return false;
		}
		_status = ready;
		_cond.notify();
		return true ;
	}
private:
	lin_io::Condition _cond;
	status 	        _status;
};

struct CoroutineWrapper : public Cond
{
public:
	CoroutineWrapper():_caller(0),_status(wait){}
	virtual ~CoroutineWrapper(){}

	virtual void init() {_status=wait;}

	virtual void yield()
	{
		_caller = lin_io::Coroutine::running();
		_status = wait;
		lin_io::Coroutine::yield();
	}
	virtual bool resume()
	{
		if (_status != wait)
			return false;

		_status = ready;
		if(!lin_io::Coroutine::resume(_caller))
			return false ;

		return true ;
	}
private:
	lin_io::Coroutine* _caller;
	status		     _status;
};

struct Promise
{
	class Response
	{
	public:
		virtual ~Response() {}
		virtual void copy(Response* rsp) = 0;
	};

	template<class T>
	class VariedResponse : public Response
	{
	public:
		VariedResponse(T& data) : _data(data)
		{
		}
		virtual ~VariedResponse() {}
		virtual void copy(Response* rsp)
		{
			VariedResponse<T>* varRsp = (VariedResponse<T>*)rsp;
			_data = varRsp->_data;
		}
	private:
		T& _data;
	};

	class StatusCode
	{
	public:
		StatusCode(int& code) : _code(code)
		{
		}
		void set(const int code)
		{
			_code = code;
		}
	private:
		int& _code;
	};

public:
	Promise():val(0),code(0),ex(0),connid(0),done(false) {}
	virtual ~Promise()
	{
		if(val) {delete val;}
		if(code) {delete code;}
	}

	template<class Reply>
	void set_value(const Reply& msg)
	{
		VariedResponse<Reply> relay(const_cast<Reply&>(msg));
		done = true;
		if(this->val)
		{
			this->val->copy(&relay);
		}
	}

	void set_exception(const RuntimeException& ex)
	{
		done = true;
		this->ex = ex;
		if(this->code)
		{
			this->code->set(ex.getCode());
		}
	}

public:
	Promise& set_sn(const uint64_t sn) {return set_sn(std::to_string(sn));}
	Promise& set_sn(const uint32_t sn) {return set_sn(std::to_string(sn));}
	Promise& set_sn(const std::string& sn)
	{
		this->sn = sn;
		return *this;
	}

	Promise& set_codec(Response* val)
	{
		if(this->val)
		{
			delete this->val;
		}
		this->val = val;
		return *this;
	}

	Promise& set_codec(StatusCode* code)
	{
		if(this->code)
		{
			delete this->code;
		}
		this->code = code;
		return *this;
	}

	Promise& set_connection(const uint32_t connid)
	{
		this->connid = connid;
		return *this;
	}

	Promise& set_extra(const std::string& extra)
	{
		this->extra = extra;
		return *this;
	}

	const std::string& get_extra() {return extra;}

private:
	Response*         val;
	StatusCode*       code;
public:
	RuntimeException  ex;
	uint32_t          connid;
	std::string       sn;
	bool 		      done;
	std::string       extra;//扩展字段
};

class IFuture : public lin_io::LockedRefCount
{
public:
	IFuture() {}
	virtual ~IFuture() {}

protected:
	virtual void done() = 0;
public:
	virtual long tick() = 0;

	void set_exception(const RuntimeException& e)
	{
		if(!_promise.done);
		{
			_promise.set_exception(e);
			done();
		}
	}

	template<class Reply>
	void set_value(const Reply& msg)
	{
		if(!_promise.done)
		{
			_promise.set_value(msg);
			done();
		}
	}

	Promise& promise()
	{
		return _promise;
	}

private:
	Promise _promise;
};
typedef lin_io::RcVar<IFuture> IFuturevar;

class Future : public IFuture
{
public:
	typedef SimpleTimer<Future> Timer;

	Future(const int tm);
	virtual ~Future();
	static lin_io::RcVar<Future> create(const int ms)
	{
		return lin_io::RcVar<Future>(new Future(ms));
	}

	//IO线程执行
	virtual long tick();

public:
	bool setup(Task* c);
	void wait() throw_exceptions;
	void timeout();
private:
	void setCond(Cond *i) {_sync = i;}

protected:
	void yield() {_sync->yield();}
	bool resume() { return _sync->resume();}

	virtual void done();//在IO线程执行
private:
	bool				_enable;
	int64_t				_t0;
	int64_t 	        _tms;
	Timer 				_timer;
	Task*			    _executor;
	lin_io::VarVar<Cond>  _sync;
};
typedef lin_io::RcVar<Future> Futurevar;


class AsyncFuture : public IFuture
{
public:
	typedef SimpleTimer<AsyncFuture> Timer;

	AsyncFuture(const int tm, Task* cb);
	virtual ~AsyncFuture();

	static lin_io::RcVar<AsyncFuture> create(const int tm, Task* cb)
	{
		return lin_io::RcVar<AsyncFuture>(new AsyncFuture(tm, cb));
	}

	template<typename OBJ,typename REQ,typename RSP>
	class Executor : public Task
	{
	public:
		Executor(const std::string& name, OBJ* obj, void(OBJ::*fun)(const int code, const REQ& req, const RSP& rsp), const REQ& req)
		:_name(name), _obj(obj), _fun(fun), _req(req),_code(0)
	    {
	    }
		virtual ~Executor() {}
		virtual bool run()
		{
			(_obj->*_fun)(_code, _req, _rsp);
			return false;
		}
		virtual const std::string& name() {return _name;}

		REQ& req() {return _req;}
		RSP& rsp() {return _rsp;}
		int& code() {return _code;}
	private:
		std::string _name;
		OBJ* _obj;
		void(OBJ::*_fun)(const int code, const REQ& req, const RSP& rsp);
		REQ  _req;
		RSP  _rsp;
		int  _code;
	};

	template<typename REQ,typename RSP>
	class Executor<void,REQ,RSP> : public Task
	{
	public:
		Executor(const std::string& name, void(*fun)(const int code, const REQ& req, const RSP& rsp), const REQ& req)
		:_name(name), _fun(fun), _req(req),_code(0)
	    {
	    }
		virtual ~Executor() {}
		virtual bool run()
		{
			(*_fun)(_code, _req, _rsp);
			return false;
		}
		virtual const std::string& name() {return _name;}

		REQ& req() {return _req;}
		RSP& rsp() {return _rsp;}
		int& code() {return _code;}
	private:
		std::string _name;
		void(*_fun)(const int code, const REQ& req, const RSP& rsp);
		REQ  _req;
		RSP  _rsp;
		int  _code;
	};

	//-------------------------------------
	template<typename OBJ,typename CTX,typename REQ,typename RSP>
	class ExecutorEx : public Task
	{
	public:
		ExecutorEx(const std::string& name, OBJ* obj, void(OBJ::*fun)(const int code, const CTX& ctx, const REQ& req, const RSP& rsp), const CTX& ctx, const REQ& req)
		:_name(name), _obj(obj), _fun(fun), _req(req), _ctx(ctx), _code(0)
	    {
	    }
		virtual ~ExecutorEx() {}
		virtual bool run()
		{
			(_obj->*_fun)(_code, _ctx, _req, _rsp);
			return false;
		}
		virtual const std::string& name() {return _name;}

		REQ& req() {return _req;}
		RSP& rsp() {return _rsp;}
		int& code() {return _code;}
	private:
		std::string _name;
		OBJ* _obj;
		void(OBJ::*_fun)(const int code, const CTX& ctx, const REQ& req, const RSP& rsp);
		REQ  _req;
		RSP  _rsp;
		CTX  _ctx;
		int  _code;
	};

	template<typename CTX,typename REQ,typename RSP>
	class ExecutorEx<void,CTX,REQ,RSP> : public Task
	{
	public:
		ExecutorEx(const std::string& name, void(*fun)(const int code, const CTX& ctx, const REQ& req, const RSP& rsp), const CTX& ctx, const REQ& req)
		:_name(name), _fun(fun), _req(req), _ctx(ctx), _code(0)
	    {
	    }
		virtual ~ExecutorEx() {}
		virtual bool run()
		{
			(*_fun)(_code, _ctx, _req, _rsp);
			return false;
		}
		virtual const std::string& name() {return _name;}

		REQ& req() {return _req;}
		RSP& rsp() {return _rsp;}
		int& code() {return _code;}
	private:
		std::string _name;
		void(*_fun)(const int code, const CTX& ctx, const REQ& req, const RSP& rsp);
		REQ  _req;
		RSP  _rsp;
		CTX  _ctx;
		int  _code;
	};

	//需要在IO任务内执行
	virtual long tick();
protected:
	virtual	void done();

public:
	bool setup(Task* c);
	void timeout();

private:
	bool		_enable;
	int64_t		_t0;
	int64_t 	_tms;
	Timer 		_timer;
	Task*		_callback;
};

typedef lin_io::RcVar<AsyncFuture> AsyncFuturevar;

//******************************************************
class ISyncFuture : public lin_io::LockedRefCount
{
public:
	ISyncFuture() {}
	virtual ~ISyncFuture() {}

	class IAdapter
	{
	public:
		virtual ~IAdapter() {}
		virtual void copy(IAdapter* adapter) = 0;
	};

	template<typename V>
	class Adapter : public IAdapter
	{
	public:
		Adapter(V& value) : _value(value)
		{
		}
		virtual ~Adapter() {}
		virtual void copy(IAdapter* adapter)
		{
			Adapter<V>* src = (Adapter<V>*)adapter;
			_value = src->_value;
		}
	private:
		V& _value;
	};

	class Promise
	{
	public:
		Promise(ISyncFuture* f): _done(false),_e(0,"success"),_adapter(0),_future(f)
		{
		}
		virtual ~Promise()
		{
			if(_adapter) {delete _adapter;}
		}

		template<class Reply>
		void set_value(const Reply& val)
		{
			if(_adapter)
			{
				Adapter<Reply> relay(const_cast<Reply&>(val));
				_adapter->copy(&relay);
			}
			if(!_done)
			{
				_done = true;
				_future->done();
			}
		}

		void set_exception(const RuntimeException& e)
		{
			_e = e;
			if(!_done)
			{
				_done = true;
				_future->done();
			}
		}

	public:
		Promise& set_adapter(IAdapter* adapter)
		{
			if(_adapter)
				delete _adapter;
			_adapter = adapter;
			return *this;
		}

		const RuntimeException& get_exception() {return _e;}

	public:
		Promise(const Promise&);
		void operator=(const Promise&);

	private:
		bool _done;
		RuntimeException _e;
		IAdapter*    _adapter;
		ISyncFuture* _future;
	};

	virtual void done() = 0;
	virtual Promise* promise() = 0;
};

template<typename T>
class SyncFuture : public ISyncFuture
{
public:
	static lin_io::RcVar< SyncFuture<T> > create()
	{
		lin_io::RcVar<SyncFuture<T>> p(new SyncFuture<T>);
		return p;
	}

public:
	const std::vector< std::pair<T,RuntimeException>* >& get()
	{
		this->wait();
		for(int i = 0; i < (int)_promises.size(); i++)
		{
			_results[i]->second = ((Promise*)_promises[i])->get_exception();
		}
		return _results;
	}

	SyncFuture(): _count(0),_sync(new ConditionWrapper())
	{
	}
	virtual ~SyncFuture()
	{
		for(auto e : _results) {delete e;}
		for(auto e : _promises) {delete e;}
	}

	Promise* promise()
	{
		auto r = new std::pair<T,RuntimeException>();
		auto p = new Promise(this);
		_results.push_back(r);//创建结果
		_promises.push_back(p);//创建promise
		p->set_adapter(new Adapter<T>(r->first));

		if(1 == lin_io::increment_int32(&_count,1))
		{
			_sync->init();
		}
		return p;
	}

protected:
	void done()
	{
		if(0 == lin_io::increment_int32(&_count,-1))
		{
			_sync->resume();
		}
	}

private:
	void wait()
	{
		if(_count)
		{
			_sync->yield();
		}
	}

private:
	volatile int32_t _count;
	lin_io::VarVar<Cond> _sync;
	std::vector<Promise*> _promises;
	std::vector< std::pair<T, RuntimeException>* > _results;
};
typedef lin_io::RcVar<ISyncFuture> ISyncFuturevar;
}

#endif
