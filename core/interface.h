#ifndef _NET_INTERFACE__
#define _NET_INTERFACE__

#include <string>
#include <stdio.h>
#include <functional>
#include "utils/atomic.h"
#include "utils/public.h"
#include "utils/packet.h"
#include "utils/exception.h"
#include "log/logger.h"
#include "future.h"
#include "manager.h"
#include "scheduler.h"

namespace net
{
//注意:接口类不能在当前IO线程中调用
/*接口列表:
 * -------------------------------
 *get         同步获取连接信息
 *async_get   异步获取连接信息
 *call        发送数据同频等待响应
 *async_call  发送数据异步等待响应
 *oneway      发送数据没有响应
 *close       同步关闭连接
 *async_close 异步关闭连接
*--------------------------------
*/

class Interface
{
public:
	Interface(const uint32_t connid) : _connid(connid)
	{
	}

	class Function
	{
	public:
		virtual ~Function() {}
		virtual void execute(IConnection* conn) = 0;
	};

	template<typename OBJ,typename RSP>
	class SyncFunc : public Function
	{
	public:
		SyncFunc(OBJ* obj, void (OBJ::*fun)(IConnection*, RSP&), RSP& rsp)
		:_obj(obj), _fun(fun), _rsp(rsp)
	    {
	    }
		virtual ~SyncFunc() {}
		virtual void execute(IConnection* conn)
		{
			(_obj->*_fun)(conn, _rsp);
		}

	private:
		OBJ* _obj;
		void (OBJ::*_fun)(IConnection*, RSP&);
		RSP&  _rsp;
	};

	template<typename RSP>
	class SyncFunc<void,RSP> : public Function
	{
	public:
		SyncFunc(void (*fun)(IConnection*, RSP&), RSP& rsp)
		:_fun(fun),_rsp(rsp)
	    {
	    }
		virtual ~SyncFunc() {}
		virtual void execute(IConnection* conn)
		{
			(*_fun)(conn, _rsp);
		}
	private:
		void (*_fun)(IConnection*, RSP&);
		RSP&  _rsp;
	};

	template<typename OBJ, typename CTX>
	class AsyncFunc : public Function
	{
	public:
		AsyncFunc(OBJ* obj, void (OBJ::*fun)(IConnection*, const CTX&), const CTX& ctx)
		:_obj(obj),_fun(fun),_ctx(ctx)
	    {
	    }
		virtual ~AsyncFunc() {}
		virtual void execute(IConnection* conn)
		{
			(_obj->*_fun)(conn, _ctx);
		}

	private:
		OBJ* _obj;
		void (OBJ::*_fun)(IConnection*, const CTX&);
		CTX _ctx;
	};

	template<typename CTX>
	class AsyncFunc<void,CTX> : public Function
	{
	public:
		AsyncFunc(void (*fun)(IConnection*, const CTX&), const CTX& ctx)
		:_fun(fun),_ctx(ctx)
	    {
	    }
		virtual ~AsyncFunc() {}
		virtual void execute(IConnection* conn)
		{
			(*_fun)(conn, _ctx);
		}
	private:
		void (*_fun)(IConnection*, const CTX&);
		CTX _ctx;
	};

	class Executor : public Task
	{
	public:
		Executor(const std::string& name, const uint32_t connid, Function* fun, IFuture* future=NULL)
		:_name(name),_connid(connid),_method(fun),_future(future)
		{
		}
		virtual ~Executor() {delete _method;}

		virtual bool run()
		{
			IConnection* conn = Manager::get()->getConnection(_connid);
			if(!conn)
			{
				GLWARN << "connid:" << _connid << " closed";
				_method->execute(0);
				if(_future.ptr()) _future->set_exception(RuntimeException(RpcException::CONNECTION_CLOSED));
			}
			else
			{
				_method->execute(conn);
				if(_future.ptr()) _future->set_exception(RuntimeException(0, "success"));
			}
			return false;
		}
		virtual const string& name() {return _name;}

	private:
		std::string _name;
		uint32_t    _connid;
		Function*   _method;
		IFuturevar 	_future;
	};

	//获取连接信息
	template<typename Reply>
	bool get(Reply& rsp, void(*fun)(IConnection* conn, Reply& rsp), const int ms=1000)
	{
		typedef SyncFunc<void,Reply> SyncCallback;
		SyncCallback* cb = new SyncCallback(fun, rsp);

		Futurevar future = Future::create(ms);
		future->promise().set_connection(_connid);
		future->setup(new Executor("get", _connid, cb, future.ptr()));
		future->wait();

		return true;
	}

	template<typename Object,typename Reply>
	bool get(Reply& rsp, Object* obj, void(Object::*fun)(IConnection* conn, Reply& rsp), const int ms=1000)
	{
		typedef SyncFunc<Object,Reply> SyncCallback;
		SyncCallback* cb = new SyncCallback(obj, fun, rsp);

		Futurevar future = Future::create(ms);
		future->promise().set_connection(_connid);
		future->setup(new Executor("get", _connid, cb, future.ptr()));
		future->wait();

		return true;
	}

	//异步获取连接信息
	template<typename Context>
	bool async_get(const Context& ctx, void(*fun)(IConnection* conn, const Context& ctx))
	{
		typedef AsyncFunc<void,Context> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback(fun, ctx);

		Executor* task = new Executor("async_get", _connid, cb);
		return Scheduler::instance().schedule(_connid, task);
	}

	template<typename Object,typename Context>
	bool async_get(const Context& ctx, Object* obj, void(Object::*fun)(IConnection* conn, const Context& ctx))
	{
		typedef AsyncFunc<Object,Context> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback(obj, fun, ctx);

		Executor* task = new Executor("async_get", _connid, cb);
		return Scheduler::instance().schedule(_connid, task);
	}

	//transport
	template<typename Request>
	class Transport : public Task
	{
	public:
		Transport(const std::string& name, const uint32_t connid, const Request& request, const string& sn="", IFuture* future=0)
		:_name(name), _connid(connid), _request(request), _sn(sn), _future(future)
		{
		}
		virtual ~Transport() {}
		virtual bool run()
		{
			try
			{
				execute();
			}
			catch(RuntimeException& e)
			{
				GLERROR << _name << " call exception: " << e.getCode() << " " << e.what();
				if(_future.ptr()) _future->set_exception(e);
			}
			catch(std::exception& e)
			{
				GLERROR << _name << " call exception: " << e.what();
				if(_future.ptr()) _future->set_exception(RuntimeException(RpcException::INTERNAL_ERROR, e.what()));
			}
			catch(...)
			{
				GLERROR << _name << " call exception: unknown error";
				if(_future.ptr()) _future->set_exception(RuntimeException(RpcException::INTERNAL_ERROR));
			}

			return false;
		}
		virtual const string& name() {return _name;}

	private:
		void execute()
		{
			//根据连接ID获取连接
			int32_t tmo = 0;//连接超时时间
			if(_future.ptr()) tmo = _future->tick();//启动定时器
			if(_future.ptr() && (tmo <= 0))
			{
				GLWARN << "connid " << _connid << " io busy";
				throw RuntimeException(RpcException::IO_BUSY);
				return;
			}
			Connection* conn = dynamic_cast<Connection*>(Manager::get()->getConnection(_connid));
			if(!conn)
			{
				GLWARN << "connid:" << _connid << " closed";
				throw RuntimeException(RpcException::CONNECTION_CLOSED);
				return;
			}

			//序列化并发送数据
			lin_io::Pack pack;
			if(!_request.serialize(pack))
			{
				GLWARN << "connid:" << _connid << " closed";
				throw RuntimeException(RpcException::PROTOCOL_ERROR);
				return;
			}
			conn->send(pack.data(), (uint32_t)pack.size());

			//把future放在连接中
			if(_future.ptr())
			{
				conn->push(_sn, _future);
			}
		}
	private:
		std::string	_name;
		uint32_t	_connid;
		Request     _request;
		string      _sn;
		IFuturevar 	_future;
	};

	//------------------------------------------------------------------------------------------------------------
	//发送数据，同步响应
	template<typename Request,typename Reply>
	bool call(const string& sn, const Request& req, Reply& rsp, const int ms=1000) throw_exceptions
	{
		GLINFO << "connid: " << _connid << " sync wait timeout(ms): " << ms;

		Futurevar future = Future::create(ms);
		future->promise().set_sn(sn).set_connection(_connid).set_codec(new Promise::VariedResponse<Reply>(rsp));
		future->setup(new Transport<Request>("call", _connid, req, sn, future.ptr()));
		future->wait();

		return true;
	}

	//发送数据，异步响应
	template<typename Request,typename Reply>
	bool async_call(const string& sn, const Request& req,
			void(*fun)(const int, const Request&, const Reply&), const uint32_t ms=1000) throw_exceptions
	{
		//必须要有SN
		if(sn.empty())
		{
			GLERROR << "connid: " << _connid << " sn is empty";
			return false;
		}
		GLINFO << "connid: " << _connid << " sn: " << sn << " async wait timeout(ms): " << ms;

		//构建回调执行者
		typedef AsyncFuture::Executor<void,Request,Reply> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback("", fun, req);

		//创建异步future
		AsyncFuturevar future = AsyncFuture::create(ms, cb);
		future->promise().set_sn(sn).set_connection(_connid).set_codec(new Promise::VariedResponse<Reply>(cb->rsp())).set_codec(new net::Promise::StatusCode(cb->code()));
		return future->setup(new Transport<Request>("async_call", _connid, req, sn, future.ptr()));
	}

	template<typename Object,typename Request,typename Reply>
	bool async_call(const string& sn, const Request& req,Object* obj,
			void(Object::*fun)(const int, const Request&, const Reply&), const uint32_t ms=1000) throw_exceptions
	{
		//必须要有SN
		if(sn.empty())
		{
			GLERROR << "connid: " << _connid << " sn is empty";
			return false;
		}
		GLINFO << "connid: " << _connid << " sn: " << sn << " async wait timeout(ms): " << ms;

		//构建回调执行者
		typedef AsyncFuture::Executor<Object,Request,Reply> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback("", obj, fun, req);

		//创建异步future
		AsyncFuturevar future = AsyncFuture::create(ms, cb);
		future->promise().set_sn(sn).set_connection(_connid).set_codec(new Promise::VariedResponse<Reply>(cb->rsp())).set_codec(new net::Promise::StatusCode(cb->code()));
		return future->setup(new Transport<Request>("async_call", _connid, req, sn, future.ptr()));
	}

	//发送数据，异步响应
	template<typename Request,typename Reply,typename Extra>
	bool async_call(const std::string& sn, const Extra& ext, const Request& req,
			void(*fun)(const int, const Extra&, const Request&, const Reply&), const int ms=1000) throw_exceptions
	{
		//必须要有SN
		if(sn.empty())
		{
			GLERROR << "connid: " << _connid << " sn is empty";
			return false;
		}
		GLINFO << "connid: " << _connid << " sn: " << sn << " async wait timeout(ms): " << ms;

		//构建回调执行者
		typedef AsyncFuture::ExecutorEx<void,Extra,Request,Reply> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback("", fun, ext, req);

		//创建异步future
		AsyncFuturevar future = AsyncFuture::create(ms, cb);
		future->promise().set_sn(sn).set_connection(_connid).set_codec(new Promise::VariedResponse<Reply>(cb->rsp())).set_codec(new net::Promise::StatusCode(cb->code()));
		return future->setup(new Transport<Request>("async_call", _connid, req, sn, future.ptr()));
	}

	template<typename Object,typename Request,typename Reply,typename Extra>
	bool async_call(const std::string& sn, const Extra& ext, const Request& req, Object* obj,
			void(*fun)(const int, const Extra&, const Request&, const Reply&), const int ms=1000) throw_exceptions
	{
		//必须要有SN
		if(sn.empty())
		{
			GLERROR << "connid: " << _connid << " sn is empty";
			return false;
		}
		GLINFO << "connid: " << _connid << " sn: " << sn << " async wait timeout(ms): " << ms;

		//构建回调执行者
		typedef AsyncFuture::ExecutorEx<Object,Extra,Request,Reply> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback("", obj, fun, ext, req);

		//创建异步future
		AsyncFuturevar future = AsyncFuture::create(ms, cb);
		future->promise().set_sn(sn).set_connection(_connid).set_codec(new Promise::VariedResponse<Reply>(cb->rsp())).set_codec(new net::Promise::StatusCode(cb->code()));
		return future->setup(new Transport<Request>("async_call", _connid, req, sn, future.ptr()));
	}

	//发送数据，没有响应
	template<typename Request>
	bool oneway(Request& req) throw_exceptions
	{
		Transport<Request>* task = new Transport<Request>("oneway", _connid, req);

		//生成IO任务
		return Scheduler::instance().schedule(_connid, task);
	}

	//-----------------------------------------------------------------------------------------------------------------
	class Closer : public Task
	{
	public:
		Closer(const std::string& name, const uint32_t connid, IFuture* future=NULL)
		:_name(name),_connid(connid),_future(future)
		{
		}
		virtual ~Closer() {}

		virtual bool run()
		{
			IConnection* conn = Manager::get()->getConnection(_connid);
			if(!conn)
			{
				GLWARN << "connid:" << _connid << " closed";
				if(_future.ptr()) _future->set_exception(RuntimeException(RpcException::CONNECTION_CLOSED));
			}
			else
			{
				conn->close();
				if(_future.ptr()) _future->set_exception(RuntimeException(0, "success"));
			}
			return false;
		}
		virtual const string& name() {return _name;}

	private:
		std::string _name;
		uint32_t    _connid;
		IFuturevar 	_future;
	};

	//同步关闭连接
	bool close(const int ms = 1000)
	{
		Futurevar future = Future::create(ms);
		future->promise().set_connection(_connid);
		future->setup(new Closer("close", _connid, future.ptr()));
		future->wait();
		return true;
	}

	template<typename Reply>
	bool close(Reply& rsp, void(*fun)(Connection*, Reply&), const int ms=1000)
	{
		typedef SyncFunc<void,Reply> SyncCallback;
		SyncCallback* cb = new SyncCallback(fun, rsp);

		Futurevar future = Future::create(ms);
		future->promise().set_connection(_connid);
		future->setup(new Closer("close", _connid, cb, future.ptr()));
		future->wait();

		return true;
	}

	template<typename Object,typename Reply>
	bool close(Reply& rsp, Object* obj, void(Object::*fun)(Connection*, Reply&), const int ms=1000)
	{
		typedef SyncFunc<Object,Reply> SyncCallback;
		SyncCallback* cb = new SyncCallback(obj, fun, rsp);

		Futurevar future = Future::create(ms);
		future->promise().set_connection(_connid);
		future->setup(new Closer("close", _connid, cb, future.ptr()));
		future->wait();

		return true;
	}

	//异步关闭连接
	bool async_close()
	{
		Closer* task = new Closer("async_close", _connid);
		return Scheduler::instance().schedule(_connid, task);
	}

	template<typename Context>
	bool async_close(const Context& ctx, void(*fun)(Connection*, const Context&))
	{
		typedef AsyncFunc<void,Context> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback(fun, ctx);

		Closer* task = new Closer("async_close", _connid, cb);
		return Scheduler::instance().schedule(_connid, task);
	}

	template<typename Object,typename Context>
	bool async_close(const Context& ctx, Object* obj, void(Object::*fun)(Connection*, const Context&))
	{
		typedef AsyncFunc<Object,Context> AsyncCallback;
		AsyncCallback* cb = new AsyncCallback(obj, fun, ctx);

		Closer* task = new Closer("async_close", _connid, cb);
		return Scheduler::instance().schedule(_connid, task);
	}

private:
	uint32_t _connid;
};

}

#endif
