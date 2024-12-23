#ifndef __NET_UDP_CONNECTION_H__
#define __NET_UDP_CONNECTION_H__

#include <stdio.h>
#include <list>
#include <cstring>
#include <stdarg.h>
#include "utils/rc.h"
#include "utils/utility.h"
#include "utils/bytebuffer.h"

#include "selector.h"
#include "connection.h"

namespace net
{
class UdpClientSocket : public Socket
{
public:
	UdpClientSocket(const SOCKET so) { doAccept(so); }
	UdpClientSocket(const u_long ip, const int port) { doConnect(ip, port); }
	UdpClientSocket(const std::string& host, const int port, const int timo) { doConnect(host, port); }

	UdpClientSocket() {}
	virtual ~UdpClientSocket() {}

    void doAccept(const SOCKET so)
    {
        socket().attach(so);
        socket().setblocking(false);
    }
    void doConnect(const std::string& ip, const int port)
    {
        doConnect(lin_io::aton_addr(ip), port);
    }
    void doConnect(const u_long ip, const int port)
    {
        socket().socket(SOCK_DGRAM);
        socket().setblocking(false);
        socket().connect(ip, port);
    }

    virtual void select(int remove, int add);

protected:
    virtual void handle(const int ev);

    virtual void onRead() = 0;
    virtual void onWrite() = 0;
    virtual void onTimeout();
    virtual void onConnected(const std::string& desc);
};

class UdpConnection : public Connection, public UdpClientSocket
{
public:
    //	default connecting timeout 5 sec
    static const uint32_t DEFAULT_CONNECT_TIMEOUT = 5 * 1000;

    typedef lin_io::ByteBuffer InputBuffer;
    typedef lin_io::ByteBuffer OutputBuffer;

    //被动连接
    UdpConnection(const SOCKET so, const uint32_t ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager, ISocketManager* listener);
    //主动连接
    UdpConnection(const uint32_t ip, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager);
    UdpConnection(const std::string& host, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager);
    virtual ~UdpConnection();

    //继承IConnection
    virtual std::string dump() const;
    virtual uint64_t getSendBytes() const;
    virtual uint64_t getSentBytes() const;
    virtual uint64_t getRecvBytes() const;

    virtual SOCKET getSocket();
    virtual uint32_t getConnId() const;

    virtual Type type() const { return UDP; }
    virtual Side side() const { return _side; }
    virtual Status status() const { return _status; }

    virtual int getPeerPort() const;
    virtual uint32_t getPeerIp() const;
    virtual int getLocalPort() const;
    virtual uint32_t getLocalIp() const;

    //关闭连接
    virtual void close();
    // 发送数据
    // 返回Connection当前output buffer size size,调用层可以以此作流控,返回值>0时，会触发onWrite事件
    // 可用send(0,0)来返回当前output buffer size
    // 网络连接断开或Connection的输出缓冲满，抛出异常
    virtual uint32_t send(const char* data, const uint32_t sz) throw_exceptions;

    //获取和设置接收超时时间
    virtual int getKeepaliveTimeout() {return _timeout;}
    virtual void setKeepaliveTimeout(const int msec);

    virtual IClientHandler* getHandler() {return _handler.ptr();}

    void setBufferSize(const int wbuf, const int rbuf);

    time_t getLastSendTime() {return _lastSendTs;}
    time_t getLastRecvTime() {return _lastRecvTs;}
protected:
    bool flush() throw_exceptions;

    //继承ClientSocket
    virtual void onTimeout();
    virtual void onConnected(const std::string& desc);
    virtual void onRead();
    virtual void onWrite();

protected:
    int  handleOnData() throw();
    bool handleOnClose(const char* reason) throw();
    bool handleOnInitiativeClose(const char* reason) throw();
public:
    bool handleOnConnected() throw();

protected:
    const Side _side;
    uint32_t _peerIp;
    int _peerPort;
    uint32_t _localIp;
    int _localPort;
    int _timeout;

    const uint32_t _connId;
    Status _status;
    time_t _lastRecvTs;
    time_t _lastSendTs;

    uint64_t _sendBytes;
    uint64_t _sentBytes;
public:
    uint64_t _recvBytes;

protected:
    //框架管理，例如用作路由
    ILinkCtrlHandler* _manager;
    //上层逻辑使用
    lin_io::RcVar<IClientHandler> _handler;
    //监听者的句柄
    lin_io::RcVar<ISocketManager> _listener;

    InputBuffer _input;
    std::list<std::string> _output;

    mutable std::string _info;
};

typedef lin_io::RcVar<UdpConnection> UdpConnection_var;
}

#endif
