#ifndef _NET_TCP_SERVER__
#define _NET_TCP_SERVER__

#include <pthread.h>
#include <deque>
#include <vector>
#include "scheduler.h"
#include "connection.h"
#include "tcp_listener.h"

namespace net
{
class TcpServer : public IAcceptHandler
{
private:
	static TcpServer& instance()
	{
		static thread_local TcpServer ins;
		return ins;
	}

public:
    static bool create(const int port, const int timeoutMs, IServerHandler* handler);
    static bool create(const IServerContext_var& context);
    static bool remove(const int port);

private:
    static void doAccept(const IClientContext_var& context);

    virtual void onAccept(IListener* listener, const SOCKET s, const uint32_t clientIp, const int clientPort, const std::string& data="");
    bool addListener(const TcpListener_var& listener);
    bool removeListener(const int port, TcpListener_var& listener);
    TcpListener* getListener(const int port);

private:
    typedef std::map<int, TcpListener_var> Listeners;
    Listeners _listeners;//只会在主线程中操作
};
}

#endif
