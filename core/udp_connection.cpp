#include "udp_connection.h"
#include <stdio.h>
#include <string.h>
#include "log/logger.h"

#include "common.h"
#include "handler.h"

using namespace net;

void UdpClientSocket::select(int remove, int add)
{
    if(add & SEL_CONNECTING)
    {
        if(socket().isConnected())
        {
            onConnected("connect ok immediately");
            return;
        }
        add = SEL_RW;
    }
    Socket::select(remove, add);
}

void UdpClientSocket::handle(const int ev)
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

void UdpClientSocket::onTimeout()
{
	GLERROR << "time out";
}

void UdpClientSocket::onConnected(const std::string& desc)
{
    //nonblock connect, must handle this
	GLERROR << desc;
    assert(false);
}

//|ip| is net bitorder !!
UdpConnection::UdpConnection(const SOCKET so, const uint32_t ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager, ISocketManager* listener)
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
, _listener(listener)
{
	try
	{
		doAccept(so);
		_localIp = socket().getlocal(&_localPort);
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
    	GLERROR << "accept connection: " <<  addr_ntoa(ip) << ":" << port << "-" << so << " failed, " << e.what();
    }
}

// 必须要让timeout>0 ( TcpSocket在实现中如果timeout<=0,会用同步模式 )
UdpConnection::UdpConnection(const std::string& host, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager)
: _side(Client)
, _peerIp(getHostByName(host.c_str()))
, _peerPort(port)
, _localIp(0)
, _localPort(0)
, _timeout(-1)
, _connId(id)
, _status(CONNECTTING)
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
		doConnect(_peerIp, _peerPort);
		select(0, SEL_CONNECTING);
	}
    catch (const std::exception& e)
    {
    	_status = SOCKETERROR;
    	GLERROR << "connect " << host << ":" << port << " failed, " << e.what();
    }
}

//|ip| is net bitorder !!
UdpConnection::UdpConnection(const uint32_t ip, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager)
: _side(Client)
, _peerIp(ip)
, _peerPort(port)
, _localIp(0)
, _localPort(0)
, _timeout(-1)
, _connId(id)
, _status(CONNECTTING)
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
		doConnect(_peerIp, _peerPort);
		select(0, SEL_CONNECTING);
	}
    catch (const std::exception& e)
    {
    	_status = SOCKETERROR;
    	GLERROR << "connect " << addr_ntoa(ip) << ":" << port << " failed, " << e.what();
    }
}

UdpConnection::~UdpConnection()
{
    if(_listener.ptr())
    {
    	GLINFO << "listener remove " << addr_ntoa(_peerIp) << ":" << _peerPort;
    	_listener->remove(_peerIp, _peerPort);
    }
}

std::string UdpConnection::dump() const
{
	if(!_info.empty())
		return _info;
    if(_info.empty())
    {
        char buf[256] = {0};
        snprintf(buf, sizeof(buf) - 1, "UDP{<%u>%s:%d%s%s:%d}"
        		,_connId, addr_ntoa(_localIp).c_str(), _localPort, _side == Server ? "<-" : "->", addr_ntoa(_peerIp).c_str(), _peerPort);
        _info = buf;
        return _info;
    }
    return "UDP{<"+ std::to_string(_connId) + ">}";
}

uint64_t UdpConnection::getSendBytes() const
{
	return _sendBytes;
}

uint64_t UdpConnection::getSentBytes() const
{
	return _sentBytes;
}

uint64_t UdpConnection::getRecvBytes() const
{
	return _recvBytes;
}

uint32_t UdpConnection::getPeerIp() const
{
    return _peerIp;
}

int UdpConnection::getPeerPort() const
{
    return _peerPort;
}

uint32_t UdpConnection::getLocalIp() const
{
    return _localIp;
}

int UdpConnection::getLocalPort() const
{
    return _localPort;
}

uint32_t UdpConnection::getConnId() const
{
    return _connId;
}

SOCKET UdpConnection::getSocket()
{
    return socket().getsocket();
}

void UdpConnection::close()
{
	handleOnInitiativeClose("close");
}

//  linux会设置成wbuf*2 , rbuf*2
void UdpConnection::setBufferSize(int wbuf, int rbuf)
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

uint32_t UdpConnection::send(const char* data, const uint32_t size) throw_exceptions
{ 
	try
	{
    if(socket().isConnected() && _output.empty())
    {
    	//if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) return UV_EAGAIN;//return en == EAGAIN || en == EINTR || EINPROGRESS == en;
    	int n = socket().send(data, (int)size);
        if(n == 0)
        {
            _output.push_back(std::string(data, size));
            select(0, SEL_WRITE);
        }
        else if(n < (int)size)
        {
        	GLERROR << "just " << n << " of " << size << " bytes were sent";
        }
        _sentBytes += n>0?n:0;
    }
    else
    {
        if(data && size > 0)
        	_output.push_back(std::string(data, size));
    }
    _lastSendTs = time(NULL);
    _sendBytes += size;

	}
    catch(const std::exception& e)
    {
    	GLERROR << "happen error: " << e.what() << " " << dump();
    }

    return (uint32_t)_output.size();
}

bool UdpConnection::flush() throw_exceptions
{
    while(!_output.empty())
    {
    	std::string data = _output.front();
        int n = socket().send(data.data(), (int)data.size());

        if(n == 0)
            break;
        else if(n < (int)data.size())
        	GLERROR << "just " << n << " of " << data.size() << " bytes were sent";

        _sentBytes += n>0?n:0;

        _output.pop_front();
        if(_output.empty())
        {
            select(SEL_WRITE, 0);
            break;
        }
    }
    return _output.empty();
}

void UdpConnection::onConnected(const std::string& desc)
{
    if(socket().isConnected())
    {
    	_localIp = socket().getlocal(&_localPort);
        _status = ESTABLISHED;
        try
        {
            int add = SEL_READ;
            if(!flush())
            {
                add |= SEL_WRITE;
            }
            select(SEL_ALL, add);
            //handleOnConnected();
        }
        catch (socket_error& e)
        {
        	GLWARN << "connect " << e.what() << " on connection " << dump();
            return;
        }
    }
    else
    {
        GLWARN << "connect " << desc << " on connection " << dump();
    }
}

void UdpConnection::onRead()
{
	lin_io::RcVar<UdpConnection> ref(this);
    _lastRecvTs = time(NULL);

    try
    {
        SOCKET s = socket().getsocket();

        int sz = std::max(static_cast<int>(_input.space()), 4 * 1024);
        char* w = _input.reserve(sz);
        int readBytes = socket().recv(w, sz - 1);
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
    	//handleOnClose(e.what());
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

void UdpConnection::onWrite()
{
	lin_io::RcVar<UdpConnection> ref(this);
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

void UdpConnection::setKeepaliveTimeout(const int msec)
{
	if(msec > 0)
	{
		_timeout = msec;
		select_timeout(_timeout);
	}
}

void UdpConnection::onTimeout()
{
	//超时判断
	lin_io::RcVar<UdpConnection> ref(this);
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

int UdpConnection::handleOnData() throw()
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

bool UdpConnection::handleOnConnected() throw()
{
    try
    {
        _handler->onConnected(this);
        return true;
    }
    catch (const std::exception& e)
    {
        GLERROR << "happen error: " << e.what() << " code: " << socket_error::getLastError();
        handleOnClose(e.what());//must close connection!
        return false;
    }
}

bool UdpConnection::handleOnClose(const char* reason) throw()
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

bool UdpConnection::handleOnInitiativeClose(const char* reason) throw()
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
