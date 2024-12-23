#include "udp_socket.h"
#include <stdio.h>
#include <string.h>
#include <memory>
#include "log/logger.h"
#include "scheduler.h"
#include "common.h"

namespace net
{
UdpSocket::UdpSocket(Listener* listener, const std::string& host, const int port)
: _listener(listener)
, _timeout(-1)
, _lastRecvTs(time(0))
, _lastSendTs(0)
{
	this->init();
	if(port > 0)
	{
		this->bind(getHostByName(host.c_str()), port);
	}
}

UdpSocket::UdpSocket(Listener* listener, const uint32_t ip, const int port)
: _listener(listener)
, _timeout(-1)
, _lastRecvTs(time(0))
, _lastSendTs(0)
{
	this->init();
	if(port > 0)
	{
		this->bind(ip, port);
	}
}

bool UdpSocket::init()
{
	try
	{
		memset(&_localAddr, 0, (int)sizeof(_localAddr));
	    socket().socket(SOCK_DGRAM);
	    socket().setblocking(false);
	    socket().setrcvbuf(1024*1024);//1M
	    return true;
	}
    catch (const std::exception& e)
    {
    	GLERROR << "socket error: " << e.what();
    	return false;
    }
}

void UdpSocket::bind(const uint32_t ip, const int port)
{
	try
	{
	    _localAddr.sin_family = AF_INET;
	    _localAddr.sin_addr.s_addr = ip;
	    _localAddr.sin_port = htons(port);
	    GLINFO << "socket bind " << addr_ntoa(ip) << ":" << port << " on " << getSocket();
	    socket().bind(&_localAddr);
	    select(0, SEL_READ);
	}
    catch (const std::exception& e)
    {
    	GLERROR << "socket bind " << addr_ntoa(ip) << ":" << port << " failed, " << e.what() << " on " << getSocket();
    }
}

UdpSocket::~UdpSocket()
{
	select_timeout();
    Socket::remove();
}

ipaddr_type* UdpSocket::getLocalAddr()
{
	if(_localAddr.sin_port==0)
	{
		int port(0);
		u_long ip = socket().getlocal(&port);
	    _localAddr.sin_family = AF_INET;
	    _localAddr.sin_addr.s_addr = ip;
	    _localAddr.sin_port = htons(port);
	}
	return &_localAddr;
}

SOCKET UdpSocket::getSocket()
{
    return socket().getsocket();
}

int UdpSocket::send(const char* data, const uint32_t size, const ipaddr_type* to)
{
	try
	{
    if(_queue.empty())
    {
    	int n = socket().sendto(data, (int)size, to, (int)sizeof(ipaddr_type));
        if(n == 0)
        {
        	GLWARN << "0 bytes were sent";
        	_queue.push_back(std::shared_ptr<UdpData>(new UdpData(data, size, to)));
            select(0, SEL_WRITE);
            return (int)size;
        }
        if(n < (int)size)
        {
        	GLERROR << "just " << n << " of " << size << " bytes were sent";
        	return -1;
        }
    }
    else
    {
        if(data && size > 0)
        	_queue.push_back(std::shared_ptr<UdpData>(new UdpData(data, size, to)));
    }
    _lastSendTs = time(NULL);
	}
    catch(const std::exception& e)
    {
    	GLERROR << "happen error: " << e.what() << " on " << getSocket();
    	return -1;
    }
    return 0;
}

int UdpSocket::send(const char* data, const uint32_t size, const uint32_t ip, const uint16_t port)
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    sa.sin_port = htons((u_short)port);

    return send(data, size, &sa);
}

bool UdpSocket::flush() throw_exceptions
{
    while(!_queue.empty())
    {
    	auto pk = _queue.front();
    	int n = socket().sendto(pk->data.data(), (int)pk->data.size(), &pk->addr, (int)sizeof(ipaddr_type));
        if(n == 0)
        {
            break;
        }
        else if(n < (int)pk->data.size())
        {
        	GLERROR << "just " << n << " of " << pk->data.size() << " bytes were sent";
        }

        _queue.pop_front();
        if(_queue.empty())
        {
            select(SEL_WRITE, 0);
            break;
        }
    }
    return _queue.empty();
}

void UdpSocket::handle(const int ev)
{
    // handle event. notify one by one
	switch (ev)
	{
	case SEL_READ:
		onRead();
		return;
	case SEL_WRITE:
		onWrite();
		return;
	case SEL_TIMEOUT:
		onTimeout();
		return;
	}
}

void UdpSocket::onRead()
{
    _lastRecvTs = time(NULL);

    try
    {
        ipaddr_type from;
        socklen_t fromlen = sizeof(ipaddr_type);
        char buf[1024*4] = {0};
        int size = socket().recvfrom(buf, (uint32_t)sizeof(buf), &from, &fromlen);
        _listener->onData(this, buf, size, &from);
    }
    catch(socket_error& e)
    {
    	GLWARN << "read " << e.what() << " on " << getSocket();
    }
}

void UdpSocket::onWrite()
{
    try
    {
        bool b = flush();
        if(b)
        {
        	//回调给上层逻辑处理
            //_handler->onWrite(this);
        }
    }
    catch (socket_error& e)
    {
    	GLWARN << "write " << e.what() << " on connection " << getSocket();
    }
}

void UdpSocket::onTimeout()
{
	//超时判断
	int ts = _timeout - (int)(time(0) - _lastRecvTs)*1000;
	if(ts > 0)
	{
		select_timeout(ts);
		return;
	}

	GLWARN << "recv timeout for " << _timeout << " ms diff: " << ts << " ms on " << getSocket();
	_listener->onTimeout(this);
	return;
}

void UdpSocket::setRecvTimeout(const int msec)
{
	_timeout = msec > 0 ? msec : -1;
	select_timeout(_timeout);
}

void UdpSocket::setBufferSize(const int size)
{
	try
	{
	    socket().setrcvbuf(size);
	}
    catch (const std::exception& e)
    {
    	GLERROR << "set buffer size error: " << e.what() << " on " << getSocket();
    }
}

int UdpSocket::getBufferSize()
{
	try
	{
	    return socket().getrcvbuf();
	}
    catch (const std::exception& e)
    {
    	GLERROR << "get buffer size error: " << e.what() << " on " << getSocket();
    }
    return -1;
}

//------------------
struct SocketContext
{
	uint16_t localPort;
	std::string localIp;
	uint32_t hashKey;
	UdpSocket::Listener* listener;
};

std::map<uint32_t, std::shared_ptr<UdpSocket>>& get_udp_socket_map()
{
	typedef std::map<uint32_t, std::shared_ptr<UdpSocket>> UdpSocketMap;
	static thread_local UdpSocketMap inst;
	return inst;
}

void get_udp_socket(const std::pair<uint32_t, UdpSocket::FindCallback>& ctx)
{
	UdpSocket* sock(0);
	std::map<uint32_t, std::shared_ptr<UdpSocket>>::iterator it = get_udp_socket_map().find(ctx.first);
	if (it != get_udp_socket_map().end())
	{
		sock = it->second.get();
	}
	(*ctx.second)(sock);
}

void create_udp_socket(const SocketContext& ctx)
{
	std::shared_ptr<UdpSocket> sock(new UdpSocket(ctx.listener,ctx.localIp, ctx.localPort));

	std::map<uint32_t, std::shared_ptr<UdpSocket>>::iterator it = get_udp_socket_map().find(sock->getSocket());
	if (it != get_udp_socket_map().end())
	{
		//destory
		it->second->getListener()->onDestroy(it->second.get(), -1, "socket id repeat");
		get_udp_socket_map().erase(it);
	}

	get_udp_socket_map()[sock->getSocket()] = sock;
	sock->getListener()->onCreate(sock.get());
}

void delete_udp_socket(const uint32_t& fd)
{
	std::map<uint32_t, std::shared_ptr<UdpSocket>>::iterator it = get_udp_socket_map().find(fd);
	if (it != get_udp_socket_map().end())
	{
		//destory
		it->second->getListener()->onDestroy(it->second.get(), -1, "destory");
		get_udp_socket_map().erase(it);
	}
}

void UdpSocket::find(const uint32_t fd, FindCallback cb)
{
	std::pair<uint32_t, UdpSocket::FindCallback> ctx(fd, cb);
	Scheduler::instance().schedule(1, get_udp_socket, ctx, "getUdpSocket");
}

bool UdpSocket::create(Listener* listener, const std::string& localIp, const int localPort)
{
	SocketContext ctx;
	ctx.listener = listener;
	ctx.localIp = localIp;
	if (ctx.localIp.empty())
	{
		std::vector<std::string> ips;
		if(net::getLocalIp(ips))
			return false;
		ctx.localIp = ips[0];
	}

	ctx.localPort = localPort;
	return Scheduler::instance().schedule(1, create_udp_socket, ctx, "createUdpSocket");
}

void UdpSocket::destroy(const uint32_t fd)
{
	Scheduler::instance().schedule(1, delete_udp_socket, fd, "deleteUdpSocket");
}
}

