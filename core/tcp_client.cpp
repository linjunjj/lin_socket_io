#include "tcp_client.h"
#include "log/logger.h"
#include "manager.h"
#include "scheduler.h"

using namespace net;

TcpClient::TcpClient(const std::string& ip, const int port, const uint32_t id, const int msec, IClientHandler* handler, ILinkCtrlHandler* manager)
:TcpConnection(ip, port, id, msec, handler, manager)
{
}

TcpClient::~TcpClient()
{
}

bool TcpClient::create(const IClientContext_var& context)
{
	return create(context->peerIP, context->peerPort, context->timeoutMs, context->handler.ptr());
}

bool TcpClient::create(const std::string& host, const int port, const int timeout, IClientHandler* handler)
{
	if(!handler)
	{
		GLERROR << "no handler, connect " << host << ":" << port << " failed";
		return false;
	}

	if(0 == net::Manager::get()->createTcpClient(host, port, timeout, handler))
	{
		return false;
	}
	return true;
}

bool TcpClient::remove(const uint32_t connId)
{
	auto conn = Manager::get()->getConnection(connId);
	if(conn)
	{
		conn->close();
		return true;
	}
	return false;
}

void TcpClient::onTimeout()
{
	int timeout = getKeepaliveTimeout();
//	//压缩空间
//	size_t isize = _input.gc();
//	size_t osize = _output.gc();
//	if(isize > 0 || osize > 0)
//	{
//		GLWARN << dump() << " recycle input size: " << isize << " output size: " << osize;
//	}
//
//	//打印缓冲区大小
//	size_t icap = _input.capacity();
//	size_t ocap = _output.capacity();
//	if(icap > 1024*100 || ocap > 1024*100)
//	{
//		GLWARN << dump() << " cache input capacity: " << icap << " output capacity: " << ocap;
//	}

	//超时判断
	lin_io::RcVar<TcpClient> ref(this);
	time_t now = time(0);

	//接收超时时间为2*timeout
	if((now - getLastRecvTime())*1000 < timeout*2)
	{
		heartbeat();
		select_timeout(timeout);
		return;
	}
	GLWARN << dump() << " recv timeout for " << 2*timeout;
	handleOnInitiativeClose("keepalive timeout");
}

void TcpClient::heartbeat()
{
	getHandler()->onHeartbeat(this);
}
