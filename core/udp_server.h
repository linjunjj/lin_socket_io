#ifndef _NET_UDP_SERVER__
#define _NET_UDP_SERVER__

#include <pthread.h>
#include <deque>
#include <vector>
#include "scheduler.h"
#include "connection.h"
#include "udp_listener.h"

namespace net
{
class UdpServer : public IAcceptHandler
{
private:
	static UdpServer& instance()
	{
		static thread_local UdpServer ins;
		return ins;
	}

public:
    static bool create(const int port, const std::string& ip, const int timeoutMs, IServerHandler* handler);
    static bool create(const IServerContext_var& context);
    static bool remove(const int port);

private:
    static void doAccept(const IClientContext_var& context);
    virtual void onAccept(IListener* listener, const SOCKET s, const uint32_t clientIp, const int clientPort, const std::string& data);
    bool addListener(const UdpListener_var& listener);
    bool removeListener(const int port, UdpListener_var& listener);
    UdpListener* getListener(const int port);

private:
    typedef std::map<int, UdpListener_var> Listeners;
    Listeners _listeners;//只会在主线程中操作
};
}

#endif
