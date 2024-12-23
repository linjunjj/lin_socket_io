#ifndef _NET_TCP_LISTENER__
#define _NET_TCP_LISTENER__

#include "listener.h"

namespace net
{
class TcpServerSocket : public Socket
{
public:
	TcpServerSocket(const SOCKET s, const unsigned int ops = SOCKOPT_DEFAULT);
	TcpServerSocket(const int port, const char* lpszip, const unsigned int ops = SOCKOPT_DEFAULT);
	virtual ~TcpServerSocket() {}

protected:
    virtual void onAccept(const SOCKET so, const u_long ip, const int port) = 0;
    virtual void handle(const int ev);
    virtual void onTimeout();
};

class TcpListener : public IListener, public TcpServerSocket
{
public:
    TcpListener(IAcceptHandler* handler, const IServerContext_var& context);
    virtual ~TcpListener() {}

    virtual int getPort() const;
    virtual SOCKET getSocket();
    virtual IServerHandler* getHandler();//获取处理句柄

    const IServerContext* getContext() {return _context.ptr();}

protected:
    virtual void onAccept(const SOCKET s, const u_long ip, const int port);

private:
    IAcceptHandler* _accept;
	IServerContext_var _context;
};
typedef lin_io::RcVar<TcpListener> TcpListener_var;

}

#endif
