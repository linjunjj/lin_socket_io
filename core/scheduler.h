#pragma once

#include <vector>
#include "worker.h"

//用于调度消息的分发
namespace net
{
class Scheduler
{
	Scheduler();
public:
	static Scheduler& instance()
	{
		static Scheduler ins;
		return ins;
	}

	virtual ~Scheduler ();

	void stop();
	void start(const int num);

	//主线程任务调度
	bool schedule(Task* c);
	template<typename ARG>
	bool schedule(void(*fun)(const ARG& req), const ARG& req, const std::string& name="task")
	{
		Executor<void,ARG>* c = new Executor<void,ARG>(fun, req, name);
		return schedule(c);
	}

	template<typename OBJ,typename ARG>
	bool schedule(OBJ* obj, void(OBJ::*fun)(const ARG& req), const ARG& req, const std::string& name="task")
	{
		Executor<OBJ,ARG>* c = new Executor<OBJ,ARG>(obj, fun, req, name);
		return schedule(c);
	}

	//子线程任务调度
	bool schedule(const uint32_t hashkey, Task* c);
	template<typename ARG>
	bool schedule(const uint32_t hashkey, void(*fun)(const ARG& req), const ARG& req, const std::string& name="task")
	{
		Executor<void,ARG>* c = new Executor<void,ARG>(fun, req, name);
		return schedule(hashkey, c);
	}

	template<typename OBJ,typename ARG>
	bool schedule(const uint32_t hashkey, OBJ* obj, void(OBJ::*fun)(const ARG& req), const ARG& req, const std::string& name="task")
	{
		Executor<OBJ,ARG>* c = new Executor<OBJ,ARG>(obj, fun, req, name);
		return schedule(hashkey, c);
	}

	//以下两个函数为同步调度
	template<typename OBJ,typename REQ, typename RSP>
	bool schedule(const uint32_t hashkey, OBJ* obj, void(OBJ::*fun)(const REQ& req, RSP& rsp), const REQ& req, RSP& rsp, const std::string& name="task")
	{
		net::Condition cond;
		ExecutorEx<OBJ,REQ,RSP>* c = new ExecutorEx<OBJ,REQ,RSP>(obj, fun, req, rsp, cond, name);
		if(!schedule(hashkey, c))
			return false;
		cond.yield();
		return true;
	}
	template<typename REQ, typename RSP>
	bool schedule(const uint32_t hashkey, void(*fun)(const REQ& req, RSP& rsp), const REQ& req, RSP& rsp, const std::string& name="task")
	{
		net::Condition cond;
		ExecutorEx<void,REQ,RSP>* c = new ExecutorEx<void,REQ,RSP>(fun, req, rsp, cond, name);
		if(!schedule(hashkey, c))
			return false;
		cond.yield();
		return true;
	}

	bool empty() {return _workers.empty();}

	uint32_t getWorkerSize() {return _workers.size();}
	pthread_t getWorkerThreadId(const uint32_t hashkey)
	{
		if(_workers.empty())
			return 0;
		uint32_t index = hashkey % (_workers.size()-1) + 1;
		Worker* worker = _workers[index];
		return worker->getThreadId();
	}

	//for quit
	bool _quit;
private:
	std::vector<Worker*> _workers;
};
}

