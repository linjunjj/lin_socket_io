/*
 * @Author: linjun linjun@cvte.com
 * @Date: 2024-07-05 01:23:06
 * @LastEditors: linjun linjun@cvte.com
 * @LastEditTime: 2024-12-23 22:50:44
 * @FilePath: /cxx/easy_socket/network/core/worker.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "worker.h"
#include "log/logger.h"
#include "common.h"
#include "manager.h"
#include "selector.h"

namespace net
{
void* work_loop_routine(void *ptr)
{
	Worker *w = (Worker *)ptr;

	w->run();

    return NULL;
}

void Worker::start()
{
	//启动主线程
	int ret = pthread_create(&_tid, NULL, work_loop_routine, this);
	if (ret != 0)
	{
		GLERROR << "create thread error: " << ret;
	}
}

void Worker::run()
{
	//设置线程ID
	_tid = net::getThreadId();

	GLINFO << "io worker thread start, id: " << _workerId << " thread id: " << _tid;
	_queue.connect(this);

	//分配IO内存(0号主线线不分配)
	if(_workerId)
		net::Manager::get(_workerId-1);

	//进行IO事件循环
	net::Selector::me()->mainloop();

	_queue.disconnect(this);
	GLINFO << "io worker thread stop, id: " << _workerId << " thread id: " << _tid;
}

void Worker::onSignals(uint8_t e)
{
	std::deque<Task*> cs;
	_queue.get(cs);
	if (cs.empty()) {return;}
	for(std::deque<Task*>::iterator c = cs.begin(); c != cs.end(); c++)
	{
		if (*c == 0) {continue;}
		try
		{
			if(!(*c)->run())
			{
				delete *c;
			}
		}
		catch(const std::exception& e)
		{
			GLERROR << "task name: " << (*c)->name() << " happen error: " << e.what();
			delete *c;
		}
		catch(...)
		{
			GLERROR << "task name: " << (*c)->name() << " happen error";
			delete *c;
		}
	}
}

bool Worker::addTask(Task *c)
{
	if(_queue.size() > _queueLimit)
	{
		//控制频率
		time_t currTime = time(0);
		static time_t lastTime = 0;
		if(currTime - lastTime > 1)
		{
			lastTime = currTime;
			GLERROR << "worker" << _workerId << " add task failed, limit " << _queueLimit;
			//上报监控
		}
		return false;
	}
	_queue.push(c);
	return true;
}
}
