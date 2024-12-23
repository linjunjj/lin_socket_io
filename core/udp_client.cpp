#include "udp_client.h"
#include "log/logger.h"
#include "manager.h"
#include "scheduler.h"

using namespace net;

UdpClient::UdpClient(const std::string& ip, const int port, const uint32_t id, IClientHandler* handler, ILinkCtrlHandler* manager)
:UdpConnection(ip, port, id, handler, manager)
{
}

UdpClient::~UdpClient()
{
}

bool UdpClient::create(const IClientContext_var& context)
{
	return create(context->peerIP, context->peerPort, context->handler.ptr());
}

bool UdpClient::create(const std::string& host, const int port, IClientHandler* handler)
{
	if(!handler)
	{
		GLERROR << "no handler, connect " << host << ":" << port << " failed";
		return false;
	}

	if(0 == net::Manager::get()->createUdpClient(host, port, handler))
	{
		return false;
	}
	return true;
}

bool UdpClient::remove(const uint32_t connId)
{
	auto conn = Manager::get()->getConnection(connId);
	if(conn)
	{
		conn->close();
		return true;
	}
	return false;
}

void UdpClient::onTimeout()
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
	lin_io::RcVar<UdpClient> ref(this);
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

void UdpClient::heartbeat()
{
	getHandler()->onHeartbeat(this);
}
