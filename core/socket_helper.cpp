#include "socket_helper.h"

#include <stdio.h>
#include "log/logger.h"

namespace net
{
SOCKET SocketHelper::detach()
{
    SOCKET so = m_socket;
    m_socket = INVALID_SOCKET;
    m_sock_flags.reset();
    return so;
}

void SocketHelper::shutdown(int nHow)
{
    if (nHow & Receives)
    {
        if (m_sock_flags.selevent & SEL_READ)
            throw socket_error(-1, "shutdown recv, but SEL_READ setup");
        m_sock_flags.shutdown_recv = 1;
    }
    if (nHow & Sends)
    {
        if (m_sock_flags.selevent & SEL_WRITE)
            throw socket_error(-1, "shutdown send, but SEL_WRITE setup");
        m_sock_flags.shutdown_send = 1;
    }
    //int ret =
    ::shutdown(getsocket(), how_shutdown(nHow));
}

void SocketHelper::close()
{
    if (isValid())
    {
        //int ret =
        soclose(m_socket);
        // XXX if (socket_error == ret) {}
        m_sock_flags.reset();
        m_socket = INVALID_SOCKET;
    }
}

void SocketHelper::setnodelay()
{
    int op = 1;
    if (!setsockopt(IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op)))
        throw socket_error("setnodelay");
}

void SocketHelper::setreuse()
{
    int op = 1;
    if (!setsockopt(SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)))
        throw socket_error("setreuse");
}

void SocketHelper::setreuseport()
{
    int op = 1;
    if (!setsockopt(SOL_SOCKET, SO_REUSEPORT, &op, sizeof(op)))
        throw socket_error("setreuseport");
}

int SocketHelper::getsndbuf() const
{
    int size = 0;
    socklen_t len = sizeof(size);
    if (!getsockopt(SOL_SOCKET, SO_SNDBUF, &size, &len))
        throw socket_error("getsndbuf");
    return size;
}

int SocketHelper::getrcvbuf() const
{
    int size = 0;
    socklen_t len = sizeof(size);
    if (!getsockopt(SOL_SOCKET, SO_RCVBUF, &size, &len))
        throw socket_error("getrcvbuf");
    return size;
}

void SocketHelper::setsndbuf(int size)
{
    if (!setsockopt(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)))
        throw socket_error("setsndbuf");
}

void SocketHelper::setrcvbuf(int size)
{
    if (!setsockopt(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)))
        throw socket_error("setrcvbuf");
}

int SocketHelper::getavailbytes() const
{
    if (m_sock_flags.tcpserver)
        return -1; // is tcp-server

    u_long ret;
    if (!IOCtl(FIONREAD, &ret))
        throw socket_error("getavailbytes");
    return ret;
}

void SocketHelper::listen()
{
    if (SOCKET_ERROR == ::listen(getsocket(), 4096))//128  SOMAXCONN
    {
        throw socket_error("listen error");
    }
    m_sock_flags.tcpserver = 1;
}

void SocketHelper::bind(int port, const std::string& ip)
{
    u_long addr = INADDR_ANY;
    if (!ip.empty())
    {
        addr = ::inet_addr(ip.c_str());
        if (addr == INADDR_NONE)
            throw socket_error(-1, "bind: invalid ip");
    }
    bind(port, addr);
}

void SocketHelper::bind(int port, const char* lpszip)
{
    u_long addr = INADDR_ANY;
    if (lpszip && strlen(lpszip) > 0)
    {
        addr = ::inet_addr(lpszip);
        if (addr == INADDR_NONE)
            throw socket_error(-1, "bind: invalid ip");
    }
    bind(port, addr);
}

void SocketHelper::bind(int port, u_long ip)
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    sa.sin_port = htons((u_short)port);

    setreuse();

    if (SOCKET_ERROR == ::bind(getsocket(), (struct sockaddr*)&sa, sizeof(sa)))
    {
        char buf[256] = {0};
        snprintf(buf, sizeof(buf), "bind at %d", port);
        throw socket_error((const char*)buf);
    }
}

void SocketHelper::bind(ipaddr_type* addr)
{
    setreuse();

    if (SOCKET_ERROR == ::bind(getsocket(), (struct sockaddr*)addr, sizeof(ipaddr_type)))
    {
        char buf[256] = {0};
        snprintf(buf, sizeof(buf), "bind at %d", ntohs(addr->sin_port));
        throw socket_error((const char*)buf);
    }
}

u_long SocketHelper::getpeer(int* port) const
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));
    socklen_t len = sizeof(sa);

    if (SOCKET_ERROR == ::getpeername(getsocket(), (struct sockaddr*)&sa, &len))
        throw socket_error("getpeer");

    if (port)
        *port = ntohs(sa.sin_port);
    return sa.sin_addr.s_addr;
}

u_long SocketHelper::getlocal(int* port) const
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));
    socklen_t len = sizeof(sa);

    if (SOCKET_ERROR == ::getsockname(getsocket(), (struct sockaddr*)&sa, &len))
        throw socket_error("getlocal");

    if (port)
        *port = ntohs(sa.sin_port);
    return sa.sin_addr.s_addr;
}

bool SocketHelper::connect(const u_long ip, const uint16_t port)
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    sa.sin_port = htons(port);

    if (SOCKET_ERROR == ::connect(getsocket(), (struct sockaddr*)&sa, sizeof(sa)))
    {
        int err = socket_error::getLastError();
        if (isIgnoreConnect(err))
            return false;

        throw socket_error(err);
    }

    m_sock_flags.connected = 1;
    return true;
}

bool SocketHelper::connect(const u_long ip, const uint16_t port, const uint32_t sec)
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    sa.sin_port = htons(port);

    if (SOCKET_ERROR == ::connect(getsocket(), (struct sockaddr*)&sa, sizeof(sa)))
    {
    	if(SEL_WRITE != waitevent(SEL_WRITE, sec))
    	{
    		return false;
    	}

		int err = 0;
		socklen_t l = sizeof(err);
		if (!getsockopt(SOL_SOCKET, SO_ERROR, &err, &l))
		     err = socket_error::getLastError();
		if (err)
		{
			throw socket_error(err);
		}
	}

	return true;
}

std::string SocketHelper::complete_nonblocking_connect()
{
    int err = 0;
    socklen_t l = sizeof(err);
    if (!getsockopt(SOL_SOCKET, SO_ERROR, &err, &l))
        err = socket_error::getLastError();
    if (err)
    {
    	socket_error e(err, "connect noneblock");
        return e.getMessage();
    }
    m_sock_flags.connected = 1;
    return std::string("connect ok");
}

#if defined(WIN32)
int SocketHelper::waitevent(int event, int timeout)
{
    if (!isValid())
        throw socket_error(-1, "poll invalid socket");

    fd_set rs;
    fd_set ws;
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    if (event & SEL_READ)
        FD_SET(getsocket(), &rs);
    if (event & SEL_WRITE)
        FD_SET(getsocket(), &ws);

    int ret;
    if (timeout >= 0) {
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = 1000 * (timeout % 1000);
        ret = ::select(int(getsocket()) + 1, &rs, &ws, NULL, &tv);
    } else
        ret = ::select(int(getsocket()) + 1, &rs, &ws, NULL, NULL);

    if (SOCKET_ERROR == ret)
        throw socket_error("select");
    if (0 == ret)
        return 0; // timeout

    // ok, return ready event
    ret = 0;
    if (FD_ISSET(getsocket(), &rs))
        ret |= SEL_READ;
    if (FD_ISSET(getsocket(), &ws))
        ret |= SEL_WRITE;
    return ret;
}

#else

int SocketHelper::waitevent(int event, int timeout)
{
    if (!isValid())
        throw socket_error(-1, "poll invalid socket");

    struct pollfd fds[1];
    fds[0].fd = getsocket();
    fds[0].events = 0;

    if (event & SEL_READ)
        fds[0].events |= POLLIN;
    if (event & SEL_WRITE)
        fds[0].events |= POLLOUT;

    int ret = ::poll(fds, 1, timeout);
    if (SOCKET_ERROR == ret) {
        if (errno == EINTR)
            return -1;
        throw socket_error("poll");
    }
    if (0 == ret)
        return 0; // timeout

    if (fds[0].revents & POLLERR)
        throw socket_error(-1, "POLLERR");
    if (fds[0].revents & POLLNVAL)
        throw socket_error(-1, "POLLNVAL");

    // ok, return ready event
    ret = 0;
    if (fds[0].revents & POLLIN)
        ret |= SEL_READ;
    if (fds[0].revents & POLLOUT)
        ret |= SEL_WRITE;
    return ret;
}

#endif

}
