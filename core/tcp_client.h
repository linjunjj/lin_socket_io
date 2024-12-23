#ifndef _NET_TCP_CLIENT__
#define _NET_TCP_CLIENT__

#include <stdio.h>
#include <memory>
#include "common.h"
#include "tcp_connection.h"

namespace net
{
//增加心跳处理
class TcpClient : public TcpConnection
{
public:
	TcpClient(const std::string& host, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager);
	virtual ~TcpClient();

public:
	//创建客户端
	static bool create(const IClientContext_var& context);
    static bool create(const std::string& host, const int port, const int timeoutMs, IClientHandler* handler);
    static bool remove(const uint32_t connId);

public:
	virtual void onTimeout();

private:
	void heartbeat();
};
}

#endif
