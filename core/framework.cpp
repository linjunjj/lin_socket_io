#include "framework.h"
#include <sys/sysinfo.h>
#include "log/logger.h"

#include "manager.h"
#include "scheduler.h"
#include "interface.h"
#include "tcp_client.h"
#include "udp_client.h"
#include "tcp_server.h"
#include "udp_server.h"

namespace net
{
Framework::Framework()
{
}
Framework::~Framework()
{
}

void Framework::async_start(const int procs)
{
	static bool start(false);
	if(start)
	{
		GLINFO << "net frame is running";
		return;
	}

	start = true;
	int size(procs);
	if(size == -1)
	{
		size = get_nprocs();
		GLINFO << "no set pool size, cpu core size: " << size;
	}
	size = size > 16 ? 16 : size;
	if(size <= 0)
	{
		GLERROR << "pool size is 0, exit!";
		exit(0);
		return;
	}

	//设置步长(步长为工作线程数量)
	Manager::init(size);

	//启动N个线程，其中0号为主线程
	Scheduler::instance().start(size);
}

void Framework::stop()
{
	Scheduler::instance().stop();
}

void io_thread_create_tcp_server(const IServerContext_var& context)
{
	TcpServer::create(context);
}

void io_thread_create_udp_server(const IServerContext_var& context)
{
	UdpServer::create(context);
}

void io_thread_delete_tcp_server(const int& serverId)
{
	TcpServer::remove(serverId);
}

void io_thread_delete_udp_server(const int& serverId)
{
	UdpServer::remove(serverId);
}

void io_thread_create_tcp_client(const IClientContext_var& context)
{
	TcpClient::create(context);
}

void io_thread_create_udp_client(const IClientContext_var& context)
{
	UdpClient::create(context);
}

bool Framework::createTcpServer(IServerHandler* handler, const int port, const int timeout)
{
	IServerContext_var context(new IServerContext);
	context->port = port;
	context->handler = handler;
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_tcp_server, context);
}

bool Framework::createTcpServer(IServerHandler* handler, const int port, const int timeout, const uint32_t hashKey)
{
	IServerContext_var context(new IServerContext);
	context->port = port;
	context->handler = handler;
	context->hashKey = std::shared_ptr<uint32_t>(new uint32_t(hashKey));
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_tcp_server, context);
}

bool Framework::createTcpClient(IClientHandler* handler, const std::string& host, const int port, const int timeout)
{
	static uint32_t _id(0);
	IClientContext_var context(new IClientContext);
	context->peerIP = host;
	context->peerPort = port;
	context->handler = handler;
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(_id++, io_thread_create_tcp_client, context);
}

bool Framework::createTcpClient(IClientHandler* handler, const std::string& host, const int port, const int timeout, const uint32_t hashKey)
{
	IClientContext_var context(new IClientContext);
	context->peerIP = host;
	context->peerPort = port;
	context->handler = handler;
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(hashKey, io_thread_create_tcp_client, context);
}

bool Framework::createUdpServer(IServerHandler* handler, const int port, const int timeout)
{
	IServerContext_var context(new IServerContext);
	context->port = port;
	context->handler = handler;
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_udp_server, context);
}

bool Framework::createUdpServer(IServerHandler* handler, const int port, const int timeout, const uint32_t hashKey)
{
	IServerContext_var context(new IServerContext);
	context->port = port;
	context->handler = handler;
	context->hashKey = std::shared_ptr<uint32_t>(new uint32_t(hashKey));
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_udp_server, context);
}

bool Framework::createUdpServer(IServerHandler* handler, const std::string& ip, const int port, const int timeout)
{
	IServerContext_var context(new IServerContext);
	context->ip = ip;
	context->port = port;
	context->handler = handler;
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_udp_server, context);
}

bool Framework::createUdpServer(IServerHandler* handler, const std::string& ip, const int port, const int timeout, const uint32_t hashKey)
{
	IServerContext_var context(new IServerContext);
	context->ip = ip;
	context->port = port;
	context->handler = handler;
	context->hashKey = std::shared_ptr<uint32_t>(new uint32_t(hashKey));
	context->timeoutMs = timeout;
	return Scheduler::instance().schedule(io_thread_create_udp_server, context);
}

bool Framework::createUdpClient(IClientHandler* handler, const std::string& host, const int port)
{
	static uint32_t _id(0);
	IClientContext_var context(new IClientContext);
	context->peerIP = host;
	context->peerPort = port;
	context->handler = handler;
	return Scheduler::instance().schedule(_id++, io_thread_create_udp_client, context);
}

bool Framework::createUdpClient(IClientHandler* handler, const std::string& host, const int port, const uint32_t hashKey)
{
	IClientContext_var context(new IClientContext);
	context->peerIP = host;
	context->peerPort = port;
	context->handler = handler;
	return Scheduler::instance().schedule(hashKey, io_thread_create_udp_client, context);
}

bool Framework::deleteTcpServer(const int serverId)
{
	return Scheduler::instance().schedule(io_thread_delete_tcp_server, serverId);
}

bool Framework::deleteUdpServer(const int serverId)
{
	return Scheduler::instance().schedule(io_thread_delete_udp_server, serverId);
}

bool Framework::deleteTcpClient(const int clientId)
{
	return Interface(clientId).async_close();
}

bool Framework::deleteUdpClient(const int clientId)
{
	return Interface(clientId).async_close();
}
}


