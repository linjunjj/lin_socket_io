#include "tcp_connection.h"
#include <stdio.h>
#include <string.h>
#include "log/logger.h"

#include "common.h"
#include "handler.h"
using namespace net;

void TcpClientSocket::select(int remove, int add)
{
    if(add & SEL_CONNECTING)
    {
        if (socket().isConnected())
        {
            onConnected("connect ok immediately");
            return;
        }
        add = SEL_RW;
    }
    Socket::select(remove, add);
}

void TcpClientSocket::handle(const int ev)
{
    // handle event. notify one by one
    if(socket().isConnected())
    {
        switch (ev)
        {
        case SEL_READ:
            onRead();
            return;
        case SEL_WRITE:
            onWrite();
            return;
        case SEL_TIMEOUT:
            onTimeout();
            return;
        }
    }
    else
    {
        switch(ev)
        {
        case SEL_TIMEOUT:
            onConnected("connect timeout");
            return;
        case SEL_READ:
        case SEL_WRITE:
            onConnected(socket().complete_nonblocking_connect());
            return;
        }
    }
    assert(false);
}

void TcpClientSocket::onTimeout()
{
	GLERROR << "time out";
}

void TcpClientSocket::onConnected(const std::string& desc)
{
    //nonblock connect, must handle this
	GLERROR << desc;
    assert(false);
}

//|ip| is net bitorder !!
TcpConnection::TcpConnection(const SOCKET so, const uint32_t ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager)
: _side(Server)
, _peerIp(ip)
, _peerPort(port)
, _localIp(0)
, _localPort(0)
, _timeout(msec)//ms
, _connId(id)
, _status(ESTABLISHED)
, _lastRecvTs(time(0))
, _lastSendTs(0)
, _sendBytes(0)
, _sentBytes(0)
, _recvBytes(0)
, _manager(manager)
, _handler(handler)
{
	try
	{
		doAccept(so);
		_localIp = socket().getlocal(&_localPort);
		socket().setnodelay();
		select(0, SEL_READ);
		if(_timeout > 0)//对于被动连接, 超时大于时才会启动定时器检测
		{
			select_timeout(_timeout);
		}
	}
    catch (const std::exception& e)
    {
    	socket().detach();
    	_status = SOCKETERROR;
    	GLERROR << "accept connection: " << addr_ntoa(ip) << ":" << port << "-" << so << " failed, " << e.what();
    }
}

// 必须要让timeout>0 ( TcpSocket在实现中如果timeout<=0,会用同步模式 )
TcpConnection::TcpConnection(const std::string& host, const int port, uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager)
: _side(Client)
, _peerIp(getHostByName(host.c_str()))
, _peerPort(port)
, _localIp(0)
, _localPort(0)
, _timeout(0)
, _connId(id)
, _status(CONNECTTING)
, _lastRecvTs(0)
, _lastSendTs(0)
, _sendBytes(0)
, _sentBytes(0)
, _recvBytes(0)
, _manager(manager)
, _handler(handler)
{
	try
	{
		doConnect(_peerIp, _peerPort, msec <= 0 ? DEFAULT_CONNECT_TIMEOUT : msec);
		socket().setnodelay();
		select(0, SEL_CONNECTING);
	}
    catch (const std::exception& e)
    {
    	_status = SOCKETERROR;
    	select_timeout();
    	Socket::remove();
    	GLERROR << "connect " << host << ":" << port << " failed, " << e.what();
    }
}

//|ip| is net bitorder !!
TcpConnection::TcpConnection(const uint32_t ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager)
: _side(Client)
, _peerIp(ip)
, _peerPort(port)
, _localIp(0)
, _localPort(0)
, _timeout(0)
, _connId(id)
, _status(CONNECTTING)
, _lastRecvTs(0)
, _lastSendTs(0)
, _sendBytes(0)
, _sentBytes(0)
, _recvBytes(0)
, _manager(manager)
, _handler(handler)
{
	try
	{
		doConnect(_peerIp, _peerPort, msec <= 0 ? DEFAULT_CONNECT_TIMEOUT : msec);
		socket().setnodelay();
		select(0, SEL_CONNECTING);
	}
    catch (const std::exception& e)
    {
    	_status = SOCKETERROR;
    	select_timeout();
    	Socket::remove();
    	GLERROR << "connect " <<  addr_ntoa(ip) << ":" << port << " failed, " << e.what();
    }
}

TcpConnection::~TcpConnection()
{
	//GLINFO << "connection release----------------this: " << this << " connid: " << this->getConnId() << " this2: " << (uint64_t)this;
}

std::string TcpConnection::dump() const
{
	if(!_info.empty())
		return _info;
    if (_status == ESTABLISHED)
    {
        char buf[256] = {0};
        snprintf(buf, sizeof(buf) - 1, "TCP{<%u>%s:%d%s%s:%d}"
        		,_connId, addr_ntoa(_localIp).c_str(), _localPort, _side == Server ? "<-" : "->", addr_ntoa(_peerIp).c_str(), _peerPort);
        _info = (const char*)buf;
        return _info;
    }
	return "TCP{<"+ std::to_string(_connId) + ">}";
}

uint64_t TcpConnection::getSendBytes() const
{
	return _sendBytes;
}

uint64_t TcpConnection::getSentBytes() const
{
	return _sentBytes;
}

uint64_t TcpConnection::getRecvBytes() const
{
	return _recvBytes;
}

uint32_t TcpConnection::getPeerIp() const
{
    return _peerIp;
}

int TcpConnection::getPeerPort() const
{
    return _peerPort;
}

uint32_t TcpConnection::getLocalIp() const
{
    return _localIp;
}

int TcpConnection::getLocalPort() const
{
    return _localPort;
}

uint32_t TcpConnection::getConnId() const
{
    return _connId;
}

SOCKET TcpConnection::getSocket()
{
    return socket().getsocket();
}

void TcpConnection::close()
{
	handleOnInitiativeClose("close");
}

//  linux会设置成wbuf*2 , rbuf*2
void TcpConnection::setBufferSize(int wbuf, int rbuf)
{
    try
    {
    	if(wbuf>0) socket().setsndbuf(wbuf);
    	if(rbuf>0) socket().setrcvbuf(rbuf);
    }
    catch (const std::exception& e)
    {
    }
}

uint32_t TcpConnection::send(const char* data, const uint32_t size) throw_exceptions
{
	int n(0);
    if (socket().isConnected() && _output.empty())
    {
    	n = socket().send(data, (int)size);
        if (n < (int)size)
        {
            _output.write(data + n, size - n);
            select(0, SEL_WRITE);
        }
    }
    else
    {
        if (data && size > 0)
            _output.write(data, size);
    }

    _lastSendTs = time(NULL);
    _sendBytes += size;
    _sentBytes += n>0?n:0;

    return (uint32_t)_output.size();
}

bool TcpConnection::flush() throw_exceptions
{
    if(_output.empty())
        return true;
    int n = socket().send(_output.data(), (int)_output.size());
    if(n > 0)
        _output.erase(n);
    if(_output.empty())
        select(SEL_WRITE, 0);
    _sentBytes += n>0?n:0;
    return _output.empty();
}

void TcpConnection::onConnected(const std::string& desc)
{
    lin_io::RcVar<TcpConnection> ref(this);
    select_timeout();

    if(socket().isConnected())
    {
    	_status = ESTABLISHED;
    	_localIp = socket().getlocal(&_localPort);
    	_lastRecvTs = time(NULL);
        GLINFO << dump() << " connected";

        try
        {
            int add = SEL_READ;
            if(!flush())
                add |= SEL_WRITE;
            select(SEL_ALL, add);
            handleOnConnected();
        }
        catch(socket_error& e)
        {
        	GLWARN << "connect " << e.what() << " on connection " << dump();
        	handleOnClose(e.what()); //broken when send(S)
            return;
        }
    }
    else
    {
        GLWARN << "connect " << desc << " on connection " << dump();
    	handleOnClose(desc.c_str()); //connect timeout
    }
}

void TcpConnection::onRead()
{
	lin_io::RcVar<TcpConnection> ref(this);
    _lastRecvTs = time(NULL);

    try
    {
        SOCKET s = socket().getsocket();
        int sz = std::max(static_cast<int>(_input.space()), 4 * 1024);
        char* w = _input.reserve(sz);
        int readBytes = socket().recv(w, sz - 1);//方便留一位设置为0用来截断字符串
        if(readBytes > 0)
        {
        	w[readBytes] = 0;
        	_input.advance(readBytes);
        	_recvBytes += readBytes;
        }
    }
    catch(const lin_io::ResourceLimitException&)
    {
    	GLWARN << "read input buffer no space on connection " << dump();
    	handleOnClose("input buffer no space");
    }
    catch(socket_error& e)
    {
    	GLWARN << "read " << e.what() << " on connection " << dump();
    	handleOnClose(e.what());
        return;
    }

    int ret = handleOnData();
    if(ret >= 0)
    {
        _input.erase(ret);
    }
    else
    {
    	handleOnInitiativeClose("handle data happen error");
    }
}

void TcpConnection::onWrite()
{
	lin_io::RcVar<TcpConnection> ref(this);
    try
    {
        bool b = flush();
        if(b)
        {
        	//回调给上层逻辑处理
            _handler->onWrite(this);
        }
    }
    catch (socket_error& e)
    {
    	GLWARN << "write " << e.what() << " on connection " << dump();
    	handleOnClose(e.what());
    }
}

void TcpConnection::setKeepaliveTimeout(const int msec)
{
	if(msec > 0)
	{
		_timeout = msec;
		select_timeout(_timeout);
	}
}

void TcpConnection::onTimeout()
{
	//超时判断
	lin_io::RcVar<TcpConnection> ref(this);
	if(_timeout > 0)
	{
		int ts = _timeout - (int)(time(0) - _lastRecvTs)*1000;
		if(ts > 0)
		{
			select_timeout(ts);
			return;
		}
		GLWARN << dump() << " recv timeout for " << _timeout << " ms diff: " << ts << " ms";
		handleOnInitiativeClose("recv timeout");
		return;
	}

	//	no timer added !!
	GLERROR << dump() << " timeout";
	select_timeout();
	assert(false);
}

int TcpConnection::handleOnData() throw()
{
    try
    {
        //回调给上层逻辑处理
        return _handler->onData(_input.data(), (uint32_t)_input.size(), this);
    }
    catch (const std::exception& e)
    {
    	GLERROR << "Unpack error " << e.what();
        return -1;
    }
}

bool TcpConnection::handleOnConnected() throw()
{
	lin_io::RcVar<TcpConnection> ref(this);
    try
    {
        _handler->onConnected(this);
        return true;
    }
    catch(const std::exception& e)
    {
        GLERROR << "happen error: " << e.what() << " code: " << socket_error::getLastError();
    	handleOnClose(e.what());
        return false;
    }
}

bool TcpConnection::handleOnClose(const char* reason) throw()
{
    try
    {
        _status = DISCONNECTED;

        select_timeout();
        Socket::remove();
        _manager->onClose(reason, this);
        return true;
    }
    catch (const std::exception& e)
    {
    	select_timeout();
        Socket::remove(); //	must close connection!
        GLERROR << "ignore exception from callback 'ILinkCtrlHandler.onClose': " << e.what();
        return false;
    }
}

bool TcpConnection::handleOnInitiativeClose(const char* reason) throw()
{
    try
    {
    	_status = DISCONNECTED;

    	select_timeout();
        Socket::remove();
        _manager->onInitiativeClose(reason, this);
        return true;
    }
    catch (const std::exception& e)
    {
    	select_timeout();
        Socket::remove(); //	must close connection!
        GLERROR << "ignore exception from callback 'ILinkCtrlHandler.onInitiativeClose': " << e.what();
        return false;
    }
}
