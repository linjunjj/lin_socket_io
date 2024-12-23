#include "tcp_server.h"
#include "log/logger.h"
#include "common.h"
#include "manager.h"
#include "scheduler.h"
using namespace net;

void TcpServer::doAccept(const IClientContext_var& context)
{
	net::Manager::get()->createTcpConnection(context->fd, getHostByName(context->peerIP.c_str()), context->peerPort, context->timeoutMs, context->handler.ptr());
}

//主线程中执行
void TcpServer::onAccept(IListener* listener, const SOCKET s, const uint32_t clientIp, const int clientPort, const std::string& data)
{
    auto li = (TcpListener*)listener;

    auto serverContext = li->getContext();
    if(serverContext->shareThread)
    {
    	net::Manager::get()->createTcpConnection(s, clientIp, clientPort, serverContext->timeoutMs, li->getHandler()->getClientHandler());
    	return;
    }

    IClientContext_var context(new IClientContext);
    context->fd = s;
    context->peerIP = net::addr_ntoa(clientIp);
    context->peerPort = clientPort;
    context->handler = serverContext->handler->getClientHandler();
    context->timeoutMs = serverContext->timeoutMs;//ms

    uint32_t hashKey = s;
    if(serverContext->hashKey.get())
    {
    	hashKey = *serverContext->hashKey.get();
    }

	Scheduler::instance().schedule(hashKey, TcpServer::doAccept, context);
}

bool TcpServer::addListener(const TcpListener_var& listener)
{
	_listeners[listener->getPort()] = listener;
	return true;
}

bool TcpServer::removeListener(const int port, TcpListener_var& listener)
{
	auto it = _listeners.find(port);
	if(it == _listeners.end())
		return false;

	listener = it->second;
	_listeners.erase(it);
	return true;
}

TcpListener* TcpServer::getListener(const int port)
{
	auto it = _listeners.find(port);
	return it != _listeners.end() ? it->second.ptr() : 0;
}

bool TcpServer::create(const int port, const int timeoutMs, IServerHandler* handler)
{
	if(!handler)
	{
		GLERROR << "no handler, listen port: " << port << " failed";
		return false;
	}

	IServerContext_var context(new IServerContext);
	context->port = port;
	context->handler = handler;
	context->timeoutMs = timeoutMs;
	context->shareThread = true;

	return create(context);
}

bool TcpServer::create(const IServerContext_var& context)
{
	auto listener = TcpServer::instance().getListener(context->port);
	if(listener)
	{
		GLWARN << "port: " << context->port << " is exist, listen failed";
		context->handler->onError(context->port, IListener::REPEAT_ERROR, "repeat bind");
		return false;
	}

	try
	{
		//开始绑定端口
		TcpListener_var s(new TcpListener(&TcpServer::instance(), context));
		TcpServer::instance().addListener(s);

		context->handler->onListened(s.ptr());
		GLINFO << "bind port: " << context->port << " success";
		std::cout << "tcp server bind port: " << context->port << " success" << std::endl;
		return true;
	}
	catch(const socket_error& e)
	{
		GLERROR << "bind port: " << context->port << " happen error: " << e.getCode() << "|" << e.what();
		context->handler->onError(context->port, IListener::BIND_ERROR, e.what());
	}
	catch(const std::exception& e)
	{
		GLERROR << "bind port: " << context->port << " happen error: " << e.what();
		context->handler->onError(context->port, IListener::BIND_ERROR, e.what());
	}
	return false;
}

bool TcpServer::remove(const int port)
{
	TcpListener_var listener;
	if(!TcpServer::instance().removeListener(port, listener))
	{
		GLERROR << "not find port: " << port;
		return false;
	}
	listener->getHandler()->onClose(listener.ptr());
	GLINFO << "close listen port: " << port;
	return true;
}
