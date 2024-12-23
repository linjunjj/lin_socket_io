#ifndef _NET_UDP_CLIENT__
#define _NET_UDP_CLIENT__

#include <stdio.h>
#include <memory>
#include "common.h"
#include "udp_connection.h"

namespace net
{
//增加心跳处理
class UdpClient : public UdpConnection
{
public:
	UdpClient(const std::string& host, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager);
	virtual ~UdpClient();

public:
	static bool create(const IClientContext_var& context);
    static bool create(const std::string& host, const int port, IClientHandler* handler);
    static bool remove(const uint32_t connId);

public:
	virtual void onTimeout();

private:
	void heartbeat();
};
}

#endif
