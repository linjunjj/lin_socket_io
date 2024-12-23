#include "tcp_listener.h"
#include "log/logger.h"
using namespace net;

TcpServerSocket::TcpServerSocket(const int port, const char* lpszip, const unsigned int ops)
{
    socket().socket();

    if (ops & SOCKOPT_REUSE)
        socket().setreuse();
    if (ops & SOCKOPT_NONBLOCK)
        socket().setblocking(false);
    if (ops & SOCKOPT_NODELAY)
        socket().setnodelay();

    socket().bind(port, lpszip);
    socket().listen();
}

TcpServerSocket::TcpServerSocket(const SOCKET s, const unsigned int ops)
{
    socket().attach(s);

    if (ops & SOCKOPT_REUSE)
        socket().setreuse();
    if (ops & SOCKOPT_NONBLOCK)
        socket().setblocking(false);
    if (ops & SOCKOPT_NODELAY)
        socket().setnodelay();

    socket().listen();
}
void TcpServerSocket::handle(const int ev)
{
    try
    { // XXX 忽略所有 accept 错误，只记录
        switch(ev)
        {
        case SEL_READ:
        {
            // 可以忽略的错误直接返回 SOCKET_ERROR
            u_long ip;
            int port;
            SOCKET s;
            while ((s = socket().accept(&ip, &port)) != SOCKET_ERROR)
            {
                onAccept(s, ip, port);
            } // while
        } // case SEL_READ
        break;
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

void TcpServerSocket::onTimeout()
{
	GLERROR << "listen socket: " << socket().getsocket() << " timeout";
    assert(false); // must
}

TcpListener::TcpListener(IAcceptHandler* handler, const IServerContext_var& context)
: TcpServerSocket(context->port, NULL, SOCKOPT_DEFAULT)
, _accept(handler)
, _context(context)
{
	ipaddr_type addr;
    socklen_t len = sizeof(ipaddr_type);
    if (getsockname(socket().getsocket(), reinterpret_cast<sockaddr*>(&addr), &len))
    {
        GLERROR << "socket: " << socket().getsocket() << " getsockname failed";
    }

    GLINFO << "socket: " << socket().getsocket() << " address: " << addr_ntoa(u_long(addr.sin_addr.s_addr)) << ":" << context->port;

    select(0, SEL_READ);
}

int TcpListener::getPort() const
{
	return _context->port;
}

SOCKET TcpListener::getSocket()
{
    return socket().getsocket();
}

IServerHandler* TcpListener::getHandler()
{
	return _context->handler.ptr();
}

void TcpListener::onAccept(const SOCKET s, const u_long ip, const int port)
{
    if(!_accept)
    {
    	GLWARN << "close incoming connection from " << addr_ntoa(ip) << ":" << port << " for no accept handler";
        SocketHelper::soclose(s);
    	return;
    }
    _accept->onAccept(this, s, ip, port);
}
