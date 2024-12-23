#include "scheduler.h"
#include "log/logger.h"
using namespace net;

Scheduler::Scheduler():_quit(false)
{
}

Scheduler::~Scheduler()
{
}

void Scheduler::stop()
{
	_quit = true;
	for(auto it = _workers.begin(); it != _workers.end(); it++)
	{
		delete *it;
	}
	_workers.clear();
}

void Scheduler::start(const int num)
{
	//启动N个线程，其中0号为主线程
	for(int i = 0; i < num+1; i++)
	{
		auto w = new Worker(i);
		w->start();
		_workers.push_back(w);
	}
	GLINFO << "start thread size: " << num;
}

bool Scheduler::schedule(Task *c)
{
	if(_quit)
	{
		GLINFO << "server downing....worker cannot add anymore";
		delete c;
		return false;
	}

	if(_workers.empty())
	{
		GLERROR << "worker size: " << _workers.size() << " add task failed, name: " << c->name();
		delete c;
		return false;
	}

	uint32_t index = 0;
	Worker* worker = _workers[index];
	if(!worker->addTask(c))
	{
		GLERROR << "index: " << index << " size: " << worker->getQueueSize() << " add task failed, name: " << c->name();
		delete c;
		return false;
	}

	return true;
}

bool Scheduler::schedule(const uint32_t hashkey, Task *c)
{
	if(_quit)
	{
		GLINFO << "server downing....worker can not add anymore";
		delete c;
		return false;
	}

	if(_workers.size() < 2)
	{
		GLERROR << "worker size: " << _workers.size() << " add task failed, name: " << c->name();
		delete c;
		return false;
	}

	uint32_t index = hashkey % (_workers.size()-1) + 1;
	Worker* worker = _workers[index];
	if(!worker->addTask(c))
	{
		GLINFO << "index: " << index << " size: " << worker->getQueueSize() << " add task failed, name: " << c->name();
		delete c;
		return false;
	}

	return true;
}

