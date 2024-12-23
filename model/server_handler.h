#ifndef _NET_SERVER_HANDLER__
#define _NET_SERVER_HANDLER__

#include <vector>
#include "../core/listener.h"

namespace net
{
class ServerHandler : public IServerHandler
{
public:
	ServerHandler() {}
	virtual ~ServerHandler() {}

	//监听状态回调
	virtual void onError(const int port, const int code, const char* reason) {}
	virtual void onClose(IListener* listener) {}
	virtual void onListened(IListener* listener) {}

	//获取连接终端处理句柄
	virtual IClientHandler* getClientHandler() = 0;
};
}

#endif
