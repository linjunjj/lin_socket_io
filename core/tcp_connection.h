#ifndef __NET_TCP_CONNECTION_H__
#define __NET_TCP_CONNECTION_H__

#include <stdio.h>
#include <cstring>
#include <stdarg.h>
#include "utils/rc.h"
#include "utils/utility.h"
#include "utils/bytebuffer.h"

#include "selector.h"
#include "connection.h"

namespace net
{
class TcpClientSocket : public Socket
{
public:
	TcpClientSocket(const SOCKET so) { doAccept(so); }
	TcpClientSocket(const u_long ip, const int port, const int timo) { doConnect(ip, port, timo); }
	TcpClientSocket(const std::string& host, const int port, const int timo) { doConnect(host, port, timo); }

	TcpClientSocket() {}
	virtual ~TcpClientSocket() {}

    void doAccept(const SOCKET so)
    {
        socket().attach(so);
        socket().setblocking(false);
    }
    void doConnect(const std::string& ip, const int port, const int timo)
    {
        doConnect(lin_io::aton_addr(ip), port, timo);
    }
    void doConnect(const u_long ip, const int port, const int timo)
    {
        socket().socket();
        socket().setblocking(false);
        if(!socket().connect(ip, port))
        {
            select_timeout(timo);
        }
    }

    virtual void select(int remove, int add);

protected:
    virtual void handle(const int);

    virtual void onRead() = 0;
    virtual void onWrite() = 0;
    virtual void onTimeout();
    virtual void onConnected(const std::string& desc);
};

class TcpConnection : public Connection, public TcpClientSocket
{
public:
    //	default connecting timeout 5 sec
    static const uint32_t DEFAULT_CONNECT_TIMEOUT = 5 * 1000;

    typedef lin_io::ByteBuffer InputBuffer;
    typedef lin_io::ByteBuffer OutputBuffer;

    //被动连接
    TcpConnection(const SOCKET so, const uint32_t ip, const int port,const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager);
    //主动连接
    TcpConnection(const uint32_t ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager);
    TcpConnection(const std::string& host, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager);
    virtual ~TcpConnection();

    //继承IConnection
    virtual std::string dump() const;
    virtual uint64_t getSendBytes() const;
    virtual uint64_t getSentBytes() const;
    virtual uint64_t getRecvBytes() const;

    virtual SOCKET getSocket();
    virtual uint32_t getConnId() const;

    virtual Type type() const { return TCP; }
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

    //for some performence optimizing, for example: can serialize message to output dirctly
    InputBuffer& input() { return _input; }
    OutputBuffer& output() { return _output; }

    time_t getLastSendTime() {return _lastSendTs;}
    time_t getLastRecvTime() {return _lastRecvTs;}
protected:
    bool flush() throw_exceptions;

    //继续ClientSocket
    virtual void onTimeout();
    virtual void onConnected(const std::string& desc);
    virtual void onRead();
    virtual void onWrite();

protected:
    int handleOnData() throw();
    bool handleOnConnected() throw();
    bool handleOnClose(const char* reason) throw();
    bool handleOnInitiativeClose(const char* reason) throw();

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
    uint64_t _recvBytes;

    //框架管理，例如用作路由
    ILinkCtrlHandler* _manager;

    //上层逻辑使用
    lin_io::RcVar<IClientHandler> _handler;

    InputBuffer _input;
    OutputBuffer _output;
    mutable std::string _info;
};

typedef lin_io::RcVar<TcpConnection> TcpConnection_var;
}

#endif
