#ifndef _NET_UDP_LISTENER__
#define _NET_UDP_LISTENER__

#include "listener.h"

namespace net
{
class UdpServerSocket : public Socket
{
public:
	UdpServerSocket(const SOCKET s, const unsigned int ops = SOCKOPT_DEFAULT);
	UdpServerSocket(const int port, const char* lpszip, const unsigned int ops = SOCKOPT_DEFAULT);
	virtual ~UdpServerSocket() {}

protected:
    virtual void onAccept(const SOCKET so, const u_long ip, const int port, const std::string& data) = 0;
    virtual void handle(const int ev);
    virtual void onTimeout();

private:
    SOCKET accept(u_long& ip, int& port, std::string& data);

public:
    ISocketManager* getSockets() {return _sockets.ptr();}

private:
    ISocketManager_var _sockets;
};

class UdpListener : public IListener, public UdpServerSocket
{
public:
	UdpListener(IAcceptHandler* handler, const IServerContext_var& context);
    virtual ~UdpListener() {}

    virtual int getPort() const;
    virtual SOCKET getSocket();
    virtual IServerHandler* getHandler();//获取处理句柄

    const IServerContext* getContext() {return _context.ptr();}

protected:
    virtual void onAccept(const SOCKET so, const u_long ip, const int port, const std::string& data);

private:
    IAcceptHandler* _accept;
    IServerContext_var _context;
};
typedef lin_io::RcVar<UdpListener> UdpListener_var;

}

#endif
