#ifndef _NET_SOCKETHELPER_H__
#define _NET_SOCKETHELPER_H__
#include <string.h>
#include <vector>
#include "socket_inc.h"
#include "utils/exception.h"

namespace net
{
enum
{
    SEL_NONE = 0,
    SEL_ALL = -1,

    SEL_READ = 1,
    SEL_WRITE = 2,
    SEL_RW = 3, // SEL_ERROR = 4,

    // notify only
    SEL_TIMEOUT = 8,

    // setup only, never notify
    // SEL_R_ONESHOT = 32, SEL_W_ONESHOT = 64, SEL_RW_ONESHOT = 96,
    SEL_CONNECTING = 128,
};

inline bool valid_addr(u_long ip) { return ip != INADDR_NONE; }
inline u_long aton_addr(const char* ip) { return ::inet_addr(ip); }
inline u_long aton_addr(const std::string& ip) { return aton_addr(ip.c_str()); }

// XXX thread ?
inline std::string addr_ntoa(u_long ip)
{
    struct in_addr addr;
    memcpy(&addr, &ip, 4);
    return std::string(::inet_ntoa(addr));
}

typedef RuntimeException socket_error;
typedef struct sockaddr_in ipaddr_type;

class SocketHelper {
public:
    virtual ~SocketHelper() { close(); }
    SOCKET getsocket() const { return m_socket; }
    SocketHelper() { m_socket = INVALID_SOCKET; }
    void attach(SOCKET so)
    {
        assert(!isValid());
        m_socket = so;
        m_sock_flags.connected = 1;
    }
    bool isValid() const { return (getsocket() != INVALID_SOCKET); }
    bool isBlocking() const { return m_sock_flags.nonblocking == 0; }
    bool isConnected() const { return m_sock_flags.connected == 1; }

    enum { Receives = 1,
        Sends = 2,
        Both = 3 };
    void shutdown(int nHow = Sends);
    bool isShutdownSends() const { return m_sock_flags.shutdown_send == 1; }
    bool isShutdownReceives() const { return m_sock_flags.shutdown_recv == 1; }

    bool checkSendTag() const { return m_sock_flags.send_tag == 1; }
    bool checkRecvTag() const { return m_sock_flags.recv_tag == 1; }
    // clear and return old tag
    bool clearSendTag()
    {
        bool b = checkSendTag();
        m_sock_flags.send_tag = 0;
        return b;
    }
    bool clearRecvTag()
    {
        bool b = checkRecvTag();
        m_sock_flags.recv_tag = 0;
        return b;
    }

    // create
    void socket(int type = SOCK_STREAM);

    void setnodelay();
    void setreuse();
    void setreuseport();
    void setblocking(bool blocking);
    void setsndbuf(int size);
    void setrcvbuf(int size);
    int getsndbuf() const;
    int getrcvbuf() const;
    int getavailbytes() const;

    std::string getpeerip(int* port = NULL) const { return addr_ntoa(getpeer(port)); }
    std::string getlocalip(int* port = NULL) const { return addr_ntoa(getlocal(port)); }
    u_long getpeer(int* port = NULL) const;
    u_long getlocal(int* port = NULL) const;

    // >0 : bytes recv
    // 0  : peer close normal
    // <0 : error (reserve)
    //    : throw socket-error
    int recv(char* buf, const int len);
    int recv(char* buf, const int len, const uint32_t sec);

    // >=0 : bytes send;
    // <0  : error (reserve)
    //     : throw socket_error
    //int send(const char* buf, const int len);

    // >=0 : bytes send;
    // <0  : error (reserve)
    // nonblocking
    int send(const char* buf, const int len);
    int send(const char* buf, const int len, const uint32_t sec);

    // true  : connect completed
    // false : nonblocking-connect inprocess
    //       : throw socket_error
    bool connect(const std::string& ip, const uint16_t port) { return connect(aton_addr(ip), port); }
    bool connect(const u_long ip, const uint16_t port);
    bool connect(const std::string& ip, const uint16_t port, const uint32_t sec) { return connect(aton_addr(ip), port, sec); }
    bool connect(const u_long ip, const uint16_t port, const uint32_t sec);
    std::string complete_nonblocking_connect();


    // use poll or select(WIN32) to wait
    int waitevent(int event, int timeout);

    // >0 : bytes sendto
    // 0  : isOk or isIgnoreError
    // <0 : error (reserve)
    //    : throw socket_error
    int sendto(const void* msg, size_t len, const ipaddr_type* to, socklen_t tolen);
    int sendto(const void* msg, size_t len, u_long ip, int port);
    int sendto(const void* msg, size_t len, const std::string& ip, int port)
    {
        return sendto(msg, len, aton_addr(ip), port);
    }
    // >0 : bytes recvfrom
    // 0  : isOk or isIgnoreError
    // <0 : error (reserve)
    //    : throw socket_error
    int recvfrom(void* buf, size_t len, ipaddr_type* from, socklen_t* fromlen);

    // >=0 : accepted socket
    // <0  : some error can ignore. for accept more than once
    //     : throe socket_error
    SOCKET accept(u_long* addr = NULL, int* port = NULL);
    SOCKET accept(std::string* ip, int* port = NULL);

    void bind(int port, const std::string& ip);
    void bind(int port, const char* lpszip);
    void bind(int port, u_long ip = INADDR_ANY);
    void bind(ipaddr_type* addr);
    void listen();

    // init
    static void initialize();
    static void finalize();

    static int soclose(SOCKET so);

    SOCKET detach();

    struct SockFlags {
        // used by Socket
        unsigned int selevent : 8;
        unsigned int selected : 1;
        // inner
        unsigned int connected : 1;
        unsigned int nonblocking : 1;
        unsigned int tcpserver : 1;
        unsigned int shutdown_send : 1;
        unsigned int shutdown_recv : 1;

        unsigned int send_tag : 1;
        unsigned int recv_tag : 1;

        SockFlags() { reset(); }
        void reset() { *((unsigned int*)this) = 0; } /* XXX clear all */
    } m_sock_flags;

protected:
    bool getsockopt(int level, int optname, void* optval, socklen_t* optlen) const
    {
        return (SOCKET_ERROR != ::getsockopt(getsocket(), level, optname, (char*)optval, optlen));
    }
    bool setsockopt(int level, int optname, const void* optval, socklen_t optlen)
    {
        return (SOCKET_ERROR != ::setsockopt(getsocket(), level, optname, (const char*)optval, optlen));
    }

    bool IOCtl(long cmd, u_long* arg) const;
    int how_shutdown(int nHow);
    bool isIgnoreConnect(int en) const;
    bool isIgnoreError(int en) const;
    bool isIgnoreAcceptError(int en) const;
    void close(); // careful if it's selected. hide now
public:
    std::vector<std::string> getlocalips();
private:
    SOCKET m_socket;
};

#if defined(WIN32) && !defined(__CYGWIN32__)
#include "socket_helper_win.inc"
#else
#include "socket_helper_unx.inc"
#endif

inline int SocketHelper::recv(char* buf, const int len)
{
	m_sock_flags.recv_tag = 1;

	int ret = ::recv(m_socket, buf, len, 0);
	if(ret < 0)
	{
		int err = socket_error::getLastError();
		if (isIgnoreError(err))
		{
			return 0;
		}
		throw socket_error();
	}
	else if(ret == 0)
	{
		// 这里表示对端的socket已正常关闭.发送过FIN了。
		throw socket_error("recv failed, counterpart has shut off");
	}
	return ret;
}

inline int SocketHelper::recv(char* buf, const int len, const uint32_t sec)
{
	m_sock_flags.recv_tag = 1;

	int count = 0;
	while(count < len)
	{
		int ret = ::recv(m_socket, buf + count, len-count, 0);
		if(ret > 0)
		{
			count += ret;
			continue;
		}

		if(ret == 0)
		{
			// 这里表示对端的socket已正常关闭.发送过FIN了。
			throw socket_error("recv failed, counterpart has shut off");
			break;
		}

		int err = socket_error::getLastError();
		if(err == EAGAIN)
		{
			break;// 由于是非阻塞的模式,表示当前缓冲区已无数据可读
		}
		if(err == EINTR)
		{
			continue;// 被信号中断
		}
		if(SEL_READ == waitevent(SEL_READ, sec))
		{
			continue;
		}
		throw socket_error();//其他不可弥补的错误
	}

	return count;
}

inline int SocketHelper::send(const char* buf, const int len)
{
    m_sock_flags.send_tag = 1;

	int ret = ::send(m_socket, buf, len, 0);
	if (0 > ret)
	{
		int err = socket_error::getLastError();
		if (isIgnoreError(err))
		{
			return 0;
		}
		throw socket_error();
	}
	return ret;
}

inline int SocketHelper::send(const char* buf, const int len, const uint32_t sec)
{
    m_sock_flags.send_tag = 1;

    int count = 0;
	while(count < len)
	{
		int ret = ::send(m_socket,buf+count,len-count,0);
		if(ret > 0)
		{
			count += ret;
			continue;
		}

		int err = socket_error::getLastError();
		if (err == EINTR)
		{
			continue;// 被信号中断
		}
		if(err == EAGAIN)
		{
			if(SEL_WRITE == waitevent(SEL_WRITE, sec))
			{
				continue;
			}
		}

		throw socket_error();
	}
	return count;
}

inline int SocketHelper::sendto(const void* msg, size_t len, u_long ip, int port)
{
    ipaddr_type sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip;
    sa.sin_port = htons((u_short)port);

    return sendto(msg, len, &sa, sizeof(sa));
}

inline int SocketHelper::sendto(const void* msg, size_t len, const ipaddr_type* to, socklen_t tolen)
{
    m_sock_flags.send_tag = 1;
    int ret = ::sendto(getsocket(), (const char*)msg, SOX_INT_CAST(len), 0, (struct sockaddr*)to, tolen);

    if (SOCKET_ERROR == ret)
    {
        int en = socket_error::getLastError();
        if (isIgnoreError(en))
            return 0;
        throw socket_error(en, "sendto");
    }
    return ret;
}

inline int SocketHelper::recvfrom(void* buf, size_t len, ipaddr_type* from, socklen_t* fromlen)
{
    m_sock_flags.recv_tag = 1;
    int ret = ::recvfrom(getsocket(), (char*)buf, SOX_INT_CAST(len), 0, (struct sockaddr*)from, fromlen);

    if (SOCKET_ERROR == ret) {
        int en = socket_error::getLastError();
        if (isIgnoreError(en))
            return 0; // igore again ant interrupt
        throw socket_error(en, "recvfrom");
    }
    return ret;
}

inline void SocketHelper::socket(int type)
{
    m_socket = ::socket(AF_INET, type, 0);
    if (SOCKET_ERROR == m_socket)
        throw socket_error("create socket");
}

inline SOCKET SocketHelper::accept(std::string* ip, int* port)
{
    u_long addr;
    int pot;
    SOCKET newso = accept(&addr, &pot);
    if (ip)
        *ip = addr_ntoa(addr);
    if (port)
        *port = pot;
    return newso;
}

inline SOCKET SocketHelper::accept(u_long* addr, int* port)
{
    ipaddr_type sa;
    socklen_t len = sizeof(sa);

    SOCKET ret = ::accept(getsocket(), (struct sockaddr*)&sa, &len);
    if (SOCKET_ERROR == ret) {
        int en = socket_error::getLastError();
        if (isIgnoreAcceptError(en))
            return SOCKET_ERROR;
        throw socket_error(en, "accept");
    }

    if (addr)
        *addr = sa.sin_addr.s_addr;
    if (port)
        *port = ntohs(sa.sin_port);
    return ret;
}
}

#endif
