#include "future.h"

#include "log/logger.h"
#include "scheduler.h"
#include "manager.h"

namespace net
{
Future::Future(const int ms)
:_enable(false)
,_t0(lin_io::nowUs()/1000)
,_tms(ms)
,_timer(this, &Future::timeout)
,_executor(0)
{
	if(lin_io::Coroutine::running())
	{
		this->setCond(new CoroutineWrapper());
	}
	else
	{
		this->setCond(new ConditionWrapper());
	}
}

Future::~Future()
{
	if(_executor) delete _executor;
}

long Future::tick()
{
	int64_t now(lin_io::nowUs()/1000);//ms
	if (now>_t0)
		_tms -= (now-_t0);
	if(!_enable)
	{
		if (_tms > 0)
			_timer.start(_tms);
		_enable = true;
	}
	return _tms;
}

bool Future::setup(Task* c)
{
	_executor = c;
	return true;
}

void Future::wait() throw_exceptions
{
	//RcVar<Future> ref(this);
	//Q&A (多线程条件变量的问题)
	//Q:为什么task要在wait这里delivery给io去run?
	//A:对于多个rpc.call在一个future上等待时,如果task
	//	在wait之前就被delivery到io去run.因为worker和io
	//	是同时执行的,可能存在这种情况io先于wait之前做完,这个时候
	//	因为wait还没调用,不清楚有多少个rpc.call这个时候条件变量
	//	的状态没法确定(如果有x个rpc.call y个刚好完成，这时候状态
	//	如果设成ready那么之后的wait将不会等待y之后rpc.call完成
	//	如果设成wait,在x=y的时候之后woker在wait之后将无法唤醒
	//  所以在每次rpc.call完成的之后一定要知道整个future的call
	// 	数量,虽然可能极大的概率是rpc.call完成落后于wait,但为了代码
	//	的严谨性,rpc.call的触发还是放在wait里面

	//从这里设置调用开始时间
	_t0 = lin_io::nowUs()/1000;
	//hash到同一个IO线程并发执行调用请求
	if(!promise().connid)
	{
		GLERROR << "no set connection id";
		throw RuntimeException(RpcException::APP_ERR_CODE);
	}

	//生成IO任务
	auto executor(_executor);
	_executor = NULL;
	if(!Scheduler::instance().schedule(promise().connid, executor))
	{
		throw RuntimeException(RpcException::WORKER_QUEUE_FULL);
	}

	//等待IO或定时器事情
	yield();

	//获取调用结果
	if(promise().ex.getCode())
	{
		throw promise().ex;
	}
}

void Future::timeout()
{
	if(promise().done)
		return;

	lin_io::RcVar<Future> ref(this);
	std::string desc("timeout");
	Connection* conn = dynamic_cast<Connection*>(Manager::get()->getConnection(promise().connid));
	if(conn)
	{
		conn->remove(promise().sn);//移除连接上的future
		desc += " " + conn->dump();
	}

	GLWARN << "sync future sn: " << promise().sn << " timeout for " << this->_tms << " ms";
	//设置超时返回
	this->set_exception(RuntimeException(RpcException::TIMEOUT, desc));
}

void Future::done()
{
	_timer.stop();
	resume();
}

//------------------
AsyncFuture::AsyncFuture(const int tm, Task* cb)
:_enable(false)
,_t0(lin_io::nowUs()/1000)
,_tms(tm)
,_timer(this, &AsyncFuture::timeout)
,_callback(cb)
{
}

AsyncFuture::~AsyncFuture()
{
	if(_callback)
	{
		delete _callback;
		_callback = 0;
	}
}

//需要在IO任务内执行
long AsyncFuture::tick()
{
	int64_t now(lin_io::nowUs()/1000);//ms
	if (now>_t0)
		_tms -= (now-_t0);

	GLINFO << "async future sn: " << promise().sn << " connid: " << promise().connid << " start timer for " << _tms << " ms";
	if(!_enable)
	{
		if (_tms > 0)
			_timer.start(_tms);
		_enable = true;
	}
	return _tms;
}

bool AsyncFuture::setup(Task* c)
{
	//生成IO任务
	if(promise().connid)
	{
		return Scheduler::instance().schedule(promise().connid, c);
	}

	GLERROR << "no set connection id";
	delete c;
	return false;
}

void AsyncFuture::done()
{
	try
	{
		_timer.stop();
		if(_callback)
			_callback->run();
	}
	catch(std::exception& e)
	{
		GLERROR << "async future sn: " << promise().sn << " connid: " << promise().connid << " callback happen error: " << e.what();
	}
	catch(...)
	{
		GLERROR << "async future sn: " << promise().sn << " connid: " << promise().connid << " callback happen error";
	}
}

void AsyncFuture::timeout()
{
	lin_io::RcVar<AsyncFuture> ref(this);
	std::string desc("timeout");
	Connection* conn = dynamic_cast<Connection*>(Manager::get()->getConnection(promise().connid));
	if(conn)
	{
		conn->remove(promise().sn);//移除连接上的future
		desc += " " + conn->dump();
	}

	GLWARN << "async future sn: " << promise().sn << " connid: " << promise().connid << " timeout for " << _tms << " ms " << desc;
	//设置超时返回
	this->set_exception(RuntimeException(RpcException::TIMEOUT, desc));
}

}
