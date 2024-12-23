#include "udp_server.h"
#include "log/logger.h"
#include "common.h"
#include "manager.h"
#include "scheduler.h"
using namespace net;

void UdpServer::doAccept(const IClientContext_var& context)
{
	net::Manager::get()->createUdpConnection(context->fd, getHostByName(context->peerIP.c_str()), context->peerPort,
			context->timeoutMs, context->data, context->handler.ptr(), context->listener.ptr());
}

//主线程中执行
void UdpServer::onAccept(IListener* listener, const SOCKET s, const uint32_t clientIp, const int clientPort, const std::string& data)
{
    auto li = (UdpListener*)listener;

    auto serverContext = li->getContext();
    if(serverContext->shareThread)
    {
    	net::Manager::get()->createUdpConnection(s, clientIp, clientPort, serverContext->timeoutMs, data, li->getHandler()->getClientHandler(), li->getSockets());
    	return;
    }

    IClientContext_var context(new IClientContext);
    context->fd = s;
    context->peerIP = net::addr_ntoa(clientIp);
    context->peerPort = clientPort;
    context->handler = serverContext->handler->getClientHandler();
    context->timeoutMs = serverContext->timeoutMs;//ms

    context->data = data;
    context->listener = li->getSockets();

    uint32_t hashKey = s;
    if(serverContext->hashKey.get())
    {
    	hashKey = *serverContext->hashKey.get();
    }

	Scheduler::instance().schedule(hashKey, UdpServer::doAccept, context);
}

bool UdpServer::addListener(const UdpListener_var& listener)
{
	_listeners[listener->getPort()] = listener;
	return true;
}

bool UdpServer::removeListener(const int port, UdpListener_var& listener)
{
	auto it = _listeners.find(port);
	if(it == _listeners.end())
		return false;

	listener = it->second;
	_listeners.erase(it);
	return true;
}

UdpListener* UdpServer::getListener(const int port)
{
	auto it = _listeners.find(port);
	return it != _listeners.end() ? it->second.ptr() : 0;
}

bool UdpServer::create(const int port, const std::string& ip, const int timeoutMs, IServerHandler* handler)
{
	if(!handler)
	{
		GLERROR << "no handler, listen port: " << port << " failed";
		return false;
	}

	IServerContext_var context(new IServerContext);
	context->ip = ip;
	context->port = port;
	context->handler = handler;
	context->timeoutMs = timeoutMs;
	context->shareThread = true;

	return create(context);
}

bool UdpServer::create(const IServerContext_var& context)
{
	auto listener = UdpServer::instance().getListener(context->port);
	if(listener)
	{
		GLWARN << "port: " << context->port << " is exist, listen failed";
		context->handler->onError(context->port, IListener::REPEAT_ERROR, "repeat bind");
		return false;
	}

	try
	{
		//开始绑定端口
		UdpListener_var s(new UdpListener(&UdpServer::instance(), context));
		UdpServer::instance().addListener(s);

		context->handler->onListened(s.ptr());

		GLINFO << "bind port: " << context->port << " success";
		std::cout << "udp server bind port: " << context->port << " success" << std::endl;
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

bool UdpServer::remove(const int port)
{
	UdpListener_var listener;
	if(!UdpServer::instance().removeListener(port, listener))
	{
		GLERROR << "not find port: " << port;
		return false;
	}

	listener->getHandler()->onClose(listener.ptr());
	GLINFO << "close listen port: " << port;
	return true;
}




