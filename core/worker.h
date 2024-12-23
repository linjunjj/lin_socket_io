#ifndef _NET_WORKER__
#define _NET_WORKER__

#include <vector>
#include <pthread.h>
#include "thread.h"
#include "queue.h"

namespace net
{
class Task
{
public:
	virtual ~Task() {}
	virtual bool run() = 0;
	virtual const std::string& name() = 0;
};

template<typename OBJ,typename ARG>
class Executor : public Task
{
public:
	virtual ~Executor() {}
	Executor(OBJ* obj, void(OBJ::*fun)(const ARG& req), const ARG& req, const std::string& name="task")
    :_obj(obj),_fun(fun),_req(req),_name(name)
	{
	}
	virtual bool run()
    {
    	(_obj->*_fun)(_req);
    	return false;
    }
	virtual const std::string& name() {return _name;}
private:
    OBJ* _obj;
	void(OBJ::*_fun)(const ARG& req);
	ARG _req;
	std::string _name;
};

template<typename ARG>
class Executor<void,ARG> : public Task
{
public:
	virtual ~Executor() {}
	Executor(void(*fun)(const ARG& req), const ARG& req, const std::string& name="task")
    :_fun(fun),_req(req),_name(name)
	{
	}
	virtual bool run()
    {
    	(*_fun)(_req);
    	return false;
    }
	virtual const std::string& name() {return _name;}
private:
	void(*_fun)(const ARG& req);
	ARG _req;
	std::string _name;
};

//====================
struct Condition
{
public:
	enum status {ready,wait,};
	Condition():_status(wait) {}
	virtual ~Condition() {}
	void yield()
	{
		lin_io::ScopLock<lin_io::Condition> sync(_cond);
		while(_status == wait)
		{
			_cond.wait();
		}
	}
	bool resume()
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

template<typename OBJ,typename REQ,typename RSP>
class ExecutorEx : public Task
{
public:
	virtual ~ExecutorEx() {_cond.resume();}
	ExecutorEx(OBJ* obj, void(OBJ::*fun)(const REQ& req, RSP& rsp), const REQ& req, RSP& rsp, net::Condition& cond, const std::string& name="task")
    :_obj(obj),_fun(fun),_req(req),_rsp(rsp),_cond(cond),_name(name)
	{
	}
	virtual bool run()
    {
    	(_obj->*_fun)(_req, _rsp);
    	return false;
    }
	virtual const std::string& name() {return _name;}
private:
    OBJ* _obj;
	void(OBJ::*_fun)(const REQ& req, RSP& rsp);
	const REQ& _req;
	RSP& _rsp;
	net::Condition& _cond;
	std::string _name;
};

template<typename REQ,typename RSP>
class ExecutorEx<void,REQ,RSP> : public Task
{
public:
	virtual ~ExecutorEx() {_cond.resume();}
	ExecutorEx(void(*fun)(const REQ& req, RSP& rsp), const REQ& req, RSP& rsp, net::Condition& cond, const std::string& name="task")
    :_fun(fun),_req(req),_rsp(rsp),_cond(cond),_name(name)
	{
	}
	virtual bool run()
    {
    	(*_fun)(_req, _rsp);
    	return false;
    }
	virtual const std::string& name() {return _name;}
private:
	void(*_fun)(const REQ& req, RSP& rsp);
	const REQ& _req;
	RSP& _rsp;
	net::Condition& _cond;
	std::string _name;
};

class Worker : public Notifier::SignalHandler
{
public:
	Worker(const int id, const int limit=1000000)
	:_tid(0), _workerId(id), _queueLimit(limit)
	{
	}

	virtual ~Worker()
	{
	}
	virtual void onSignals(uint8_t e);

	void run();
	void start();
	bool addTask(Task *c);
	uint32_t getWorkerId() {return _workerId;}
	uint32_t getQueueSize() {return _queue.size();}
	pthread_t getThreadId() {return _tid;}

private:
	pthread_t _tid;
	uint32_t _workerId;
	uint32_t _queueLimit;
	SingleConsumerQueue<Task> _queue;
};

}

#endif
