#ifndef _NET_CLIENT_HANDLER__
#define _NET_CLIENT_HANDLER__

#include "../core/connection.h"

namespace net
{
class ClientHandler : public IClientHandler
{
public:
	ClientHandler() {}
	virtual ~ClientHandler() {}

	//注意以下回调都是在IO线程执行, 不建议在回调中执行大任务, 否则会卡IO线程

	//连接状态回调
	virtual void onConnected(IConnection* conn) = 0; //连接成功
	virtual void onClose(const char* reason, IConnection* conn) = 0; //连接失败、对方主动关闭、超时、网络异常连接断开、input buffer无空间
	virtual void onInitiativeClose(const char* reason, IConnection* conn) = 0; //app主动要求关闭连接

	//接收到数据时回调
	virtual int onData(const char* data, const uint32_t size, IConnection* conn) = 0;
	//需要发心跳时回调
	virtual void onHeartbeat(IConnection* conn) {}
};
}

#endif
