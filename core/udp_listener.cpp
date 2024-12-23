#include "udp_listener.h"
#include "log/logger.h"
using namespace net;

UdpServerSocket::UdpServerSocket(const int port, const char* lpszip, const unsigned int ops)
:_sockets(new ISocketManager)
{
    socket().socket(SOCK_DGRAM);

    if(ops & SOCKOPT_REUSE)
    {
        socket().setreuse();
        socket().setreuseport();//REUSEPORT是必要的，因为在connect之前，你必须为新建的socket bind跟listener一样的IP地址和端口，因此就需要这个socket选项。
    }
    if(ops & SOCKOPT_NONBLOCK)
        socket().setblocking(false);

    socket().bind(port, lpszip);
}

UdpServerSocket::UdpServerSocket(const SOCKET s, const unsigned int ops)
:_sockets(new ISocketManager)
{
    socket().attach(s);

    if(ops & SOCKOPT_REUSE)
    {
        socket().setreuse();
        socket().setreuseport();
    }

    if(ops & SOCKOPT_NONBLOCK)
        socket().setblocking(false);
}

void UdpServerSocket::handle(const int ev)
{
    try
    { // XXX 忽略所有 accept 错误，只记录
        switch(ev)
        {
        case SEL_READ:
        {
            u_long ip;
            int port;
            std::string data;
            SOCKET s = accept(ip, port, data);
            if(s > 0)
            {
                onAccept(s, ip, port, data);
            }
            break;
        }
        case SEL_TIMEOUT:
            onTimeout();
            break;
        default:
        	GLERROR << "listen socket: " << socket().getsocket() << " unknow event";
            assert(false);
            break;
        }
    }
    catch (std::exception& ex)
    {
        GLERROR << "listen socket: " << socket().getsocket() << " error: " << ex.what();
    }
}

void UdpServerSocket::onTimeout()
{
	GLERROR << "listen socket: " << socket().getsocket() << " timeout";
    assert(false); // must
}

SOCKET UdpServerSocket::accept(u_long& ip, int& port, std::string& data)
{
	ipaddr_type peer;
	socklen_t addrLen = sizeof(peer);

	//接收远端数据
	char buf[1024*4];
	int ret = socket().recvfrom(buf, 1024*4, &peer, &addrLen);//void* buf, size_t len, ipaddr_type* from, socklen_t* fromlen
	if(ret > 0)
	{
		data.assign(buf, (std::string::size_type)ret);
	}
	peer.sin_family = AF_INET;

	//获取远端地址
	ip = peer.sin_addr.s_addr;
	port = ntohs(peer.sin_port);

	//创建UDP套接字
	SOCKET s = _sockets->find(ip, port);
	if(s)
	{
		return s;
	}

    ipaddr_type local;
    memset(&local, 0, (uint32_t)sizeof(local));
    int localPort;
    u_long localIp = socket().getlocal(&localPort);

    //设置本端地址
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = localIp;
    local.sin_port = htons(localPort);

	SocketHelper newSock;
	newSock.socket(SOCK_DGRAM);
	newSock.setreuse();
	newSock.setreuseport();

	if(::bind(newSock.getsocket(), (struct sockaddr *)&local, (uint32_t)sizeof(struct sockaddr)))
	{
		throw socket_error();
	}
	if(::connect(newSock.getsocket(), (struct sockaddr *)&peer, (uint32_t)sizeof(struct sockaddr)) == -1)
	{
		throw socket_error();
	}

	s = newSock.detach();
	_sockets->add(ip, port, s);

	GLINFO << "===========clone socket: " << s << " bind " << addr_ntoa(localIp) << ":" << localPort << " connect " << addr_ntoa(ip) << ":" << port;

	return s;
}

UdpListener::UdpListener(IAcceptHandler* handler, const IServerContext_var& context)
: UdpServerSocket(context->port, context->ip.c_str(), SOCKOPT_DEFAULT)
, _accept(handler)
, _context(context)
{
	ipaddr_type addr;
    socklen_t len = sizeof(ipaddr_type);
    if(getsockname(socket().getsocket(), reinterpret_cast<sockaddr*>(&addr), &len))
    {
        GLERROR << "socket: " << socket().getsocket() << " getsockname failed";
    }

    GLINFO << "socket: " << socket().getsocket() << " address: " << addr_ntoa(u_long(addr.sin_addr.s_addr)) << ":" << context->port;

    select(0, SEL_READ);
}

int UdpListener::getPort() const
{
	return _context->port;
}

SOCKET UdpListener::getSocket()
{
    return socket().getsocket();
}

IServerHandler* UdpListener::getHandler()
{
	return _context->handler.ptr();
}

void UdpListener::onAccept(const SOCKET so, const u_long ip, const int port, const std::string& data)
{
    if(!_accept)
    {
    	getSockets()->remove(ip, port);
    	SocketHelper::soclose(so);

    	GLERROR << "close incoming connection from " << addr_ntoa(ip) << ":" << port << " for no accept handler";
    	return;
    }
    _accept->onAccept(this, so, ip, port, data);
}

