#ifndef ___NET_CONNECTION_H__
#define ___NET_CONNECTION_H__

#include <map>
#include <memory>
#include <stdio.h>
#include <cstring>
#include <stdarg.h>
#include "log/logger.h"
#include "utils/rc.h"
#include "utils/utility.h"
#include "utils/bytebuffer.h"

#include "selector.h"
#include "future.h"

namespace net
{
struct IConnection;

//连接状态事件触发
struct ILinkCtrlHandler
{
    virtual ~ILinkCtrlHandler() {}
    virtual void onConnected(IConnection* conn) = 0; //	连接成功
    virtual void onClose(const char* reason, IConnection* conn) = 0; //	连接失败、对方主动关闭、超时、网络异常连接断开、input buffer无空间
    virtual void onInitiativeClose(const char* reason, IConnection* conn) = 0; // app主动要求关闭连接
};

//接受到数据时触发
struct ILinkInputHandler
{
    virtual ~ILinkInputHandler() {}

    virtual int onData(const char* data, const uint32_t size, IConnection* conn) = 0; //	return -1 : close this connection
};

//输出缓冲区可以写或需要发心跳时触发
struct ILinkOutputHandler
{
    virtual ~ILinkOutputHandler() {}
    virtual void onWrite(IConnection* conn) {}
    virtual void onHeartbeat(IConnection* conn) {}
};

class IClientHandler : public ILinkInputHandler, public ILinkOutputHandler, public ILinkCtrlHandler, public lin_io::LockedRefCount
{
public:
	virtual ~IClientHandler() {}

	//注意以下回调都是在IO线程执行, 不建议在回调中执行大任务, 否则会卡IO线程

	//连接状态回调
    virtual void onConnected(IConnection* conn) = 0;
    virtual void onClose(const char* reason, IConnection* conn) = 0;
    virtual void onInitiativeClose(const char* reason, IConnection* conn) = 0;

	//接收到数据时回调
	virtual int onData(const char* data, const uint32_t size, IConnection* conn) = 0;
	//需要发心跳时回调
	virtual void onHeartbeat(IConnection* conn) = 0;
};
typedef lin_io::RcVar<IClientHandler> IClientHandler_var;

class ISocketManager : public lin_io::LockedRefCount
{
public:
    SOCKET find(const u_long peerIp, const int peerPort)
    {
    	uint64_t key = ((uint64_t)peerIp<<32) | peerPort;
    	lin_io::ScopLock<lin_io::SpinLock> sync(_lock);
    	auto it = _sockets.find(key);
    	return it != _sockets.end() ? it->second.first : 0;
    }
    void add(const u_long peerIp, const int peerPort, const SOCKET s)
    {
    	uint64_t key = ((uint64_t)peerIp<<32) | peerPort;
    	lin_io::ScopLock<lin_io::SpinLock> sync(_lock);
    	_sockets[key] = std::make_pair(s, 0);
    }
    void remove(const u_long peerIp, const int peerPort)
    {
    	uint64_t key = ((uint64_t)peerIp<<32) | peerPort;
    	lin_io::ScopLock<lin_io::SpinLock> sync(_lock);
    	_sockets.erase(key);
    }
public:
    bool setConnId(const u_long peerIp, const int peerPort, const uint32_t connId)
    {
    	uint64_t key = ((uint64_t)peerIp<<32) | peerPort;
    	lin_io::ScopLock<lin_io::SpinLock> sync(_lock);
    	auto it = _sockets.find(key);
    	if(it == _sockets.end())
    		return false;
    	it->second.second = connId;
    	return true;
    }
    uint32_t getConnId(const u_long peerIp, const int peerPort)
    {
    	uint64_t key = ((uint64_t)peerIp<<32) | peerPort;
    	lin_io::ScopLock<lin_io::SpinLock> sync(_lock);
    	auto it = _sockets.find(key);
    	return it != _sockets.end() ? it->second.second : 0;
    }
private:
    lin_io::SpinLock _lock;
	std::map<uint64_t, std::pair<SOCKET,uint32_t> > _sockets;
};
typedef lin_io::RcVar<ISocketManager> ISocketManager_var;

struct IConnection : public lin_io::LockedRefCount
{
    enum Type {TCP,UDP};
    enum Side {Server,Client};
    enum Status {CONNECTTING,ESTABLISHED,DISCONNECTED,SOCKETERROR};

    virtual ~IConnection() {}

    virtual std::string dump() const = 0;
    virtual uint64_t getSendBytes() const = 0;
    virtual uint64_t getSentBytes() const = 0;
    virtual uint64_t getRecvBytes() const = 0;

    virtual SOCKET getSocket() = 0;
    virtual uint32_t getConnId() const = 0;
    virtual uint32_t clientId() {return getConnId();}

    virtual Type type() const = 0;
    virtual Side side() const = 0;
    virtual Status status() const = 0;

    virtual int getPeerPort() const = 0;
    virtual uint32_t getPeerIp() const = 0;
    virtual int getLocalPort() const = 0;
    virtual uint32_t getLocalIp() const = 0;

    //关闭连接
    virtual void close() = 0;
    // 发送数据
    // 返回Connection当前output buffer size,调用层可以以此作流控,返回值>0时，会触发onWrite事件
    // 可用send(0,0)来返回当前output buffer size
    // 网络连接断开或Connection的输出缓冲满，抛出异常
    virtual uint32_t send(const char* data, const uint32_t sz) throw_exceptions = 0;

    //获取和设置接收超时时间
    virtual int getKeepaliveTimeout() = 0;
    virtual void setKeepaliveTimeout(const int msec) = 0;

    //获取处理句柄
    virtual IClientHandler* getHandler() = 0;
};
typedef lin_io::RcVar<IConnection> IConnection_var;

struct Connection : public IConnection
{
	virtual ~Connection()
	{
		clear();
	}

	bool pop(const uint32_t sn, IFuturevar& future) {return pop(std::to_string(sn), future);}
	bool pop(const uint64_t sn, IFuturevar& future) {return pop(std::to_string(sn), future);}
	bool pop(const std::string& sn, IFuturevar& future)
	{
		Futures::iterator it = _futures.find(sn);
		if (it != _futures.end())
		{
			future = it->second;
			_futures.erase(it);
			return true;
		}
		return false;
	}

	bool push(const uint32_t sn, IFuture* future) {return push(std::to_string(sn), future);}
	bool push(const uint64_t sn, IFuture* future) {return push(std::to_string(sn), future);}
	bool push(const std::string& sn, IFuture* future)
	{
		Futures::iterator it = _futures.find(sn);
		if(it != _futures.end())
		{
			it->second->set_exception(RuntimeException(RpcException::INTERNAL_ERROR, "repeat sequence"));
			_futures.erase(it);
		}

		IFuturevar ref(future);
		_futures[sn] = ref;
		return true;
	}
	void remove(const std::string& sn)
	{
		_futures.erase(sn);
	}

private:
	void clear()
	{
		while(!_futures.empty())
		{
			Futures::iterator it = _futures.begin();
			it->second->set_exception(RuntimeException(RpcException::CONNECTION_CLOSED, "connection closed"));
			_futures.erase(it);
		}
	}

private:
	typedef std::map<std::string,IFuturevar> Futures;
	Futures _futures;
};
typedef lin_io::RcVar<Connection> Connection_var;

//客户端信息
struct IClientContext : public lin_io::LockedRefCount
{
	SOCKET fd;
	int peerPort = 0;
	std::string peerIP;
	int timeoutMs = 0;
	IClientHandler_var handler;

	//UDP
	std::string data;
	ISocketManager_var listener;
};
typedef lin_io::RcVar<IClientContext> IClientContext_var;
}

#endif
