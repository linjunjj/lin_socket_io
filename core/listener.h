#ifndef _NET_LISTENER__
#define _NET_LISTENER__

#include "utils/rc.h"
#include "handler.h"
#include "connection.h"

namespace net
{
class IListener;
struct IAcceptHandler
{
    virtual ~IAcceptHandler() {}
    virtual void onAccept(IListener* listener, const SOCKET so, const uint32_t clientIp, const int clientPort, const std::string& data="") = 0;
};

struct IListenHandler
{
    virtual ~IListenHandler() {}
    virtual void onClose(IListener* listener) = 0;
    virtual void onListened(IListener* listener) = 0;
};

struct IServerHandler : public IListenHandler, public lin_io::LockedRefCount
{
	virtual ~IServerHandler() {}

	//监听状态回调
	virtual void onError(const int port, const int code, const char* reason) = 0;
    virtual void onClose(IListener* listener) = 0;
    virtual void onListened(IListener* listener) = 0;

	//获取连接终端处理句柄
	virtual IClientHandler* getClientHandler() = 0;
};
typedef lin_io::RcVar<IServerHandler> IServerHandler_var;

class IListener : public lin_io::LockedRefCount
{
public:
	enum
	{
		NO_ERROR = 0,
		BIND_ERROR = 1,
		REPEAT_ERROR = 2,
	};

    virtual ~IListener() {}
    virtual int getPort() const = 0;
    virtual int serverId() const {return getPort();}
    virtual SOCKET getSocket() = 0;
    virtual IServerHandler* getHandler() = 0;//获取处理句柄
};

typedef lin_io::RcVar<IListener> IListener_var;

enum
{
    SOCKOPT_REUSE = 1,
    SOCKOPT_NONBLOCK = 2,
    SOCKOPT_NODELAY = 4,
    SOCKOPT_DEFAULT = (SOCKOPT_REUSE | SOCKOPT_NONBLOCK | SOCKOPT_NODELAY),
};

struct IServerContext : public lin_io::LockedRefCount
{
	int port = 0;
	std::string ip;
	IServerHandler_var handler;

	//终端默认接收超时(单位毫秒)
	int timeoutMs = 0;

	//接受的连接的线程调度
	bool shareThread = false;//共享服务端口监听的线程
    std::shared_ptr<uint32_t> hashKey;
};

typedef lin_io::RcVar<IServerContext> IServerContext_var;

}

#endif
