#include "manager.h"
#include <sstream>
#include "log/logger.h"
#include "tcp_connection.h"
#include "tcp_client.h"
#include "udp_connection.h"
#include "udp_client.h"

using namespace net;

uint32_t Manager::_step = 1;

Manager* Manager::get(const uint32_t id)
{
	static TSS<Manager> inst(Manager::destroy);
	Manager* r = inst.get();
    if(!r)
    {
    	GLINFO << "create router in thread local memory, id: " << id;
    	std::cout << "create router in thread local memory, id: " << id << " max: " << Manager::_step << std::endl;
    	r = new Manager(id);
    	inst.set(r);
    }
    return r;
}

uint32_t Manager::getConnectionId()
{
	//type : socket id hash on this connection,there use 4bit (0-8)
    uint32_t connid;
    do
    {
    	_seed += Manager::_step;
        if(_seed == 0)
        {
        	_seed += Manager::_step;
        }
        connid = _seed;
    }while(_connections.find(connid) != _connections.end());
    return connid;
}

bool Manager::addConnection(IConnection* conn)
{
    Connections::iterator it = _connections.find(conn->getConnId());
    if(it != _connections.end())
    {
        if(conn == it->second.ptr())
        {
            return true;
        }
        GLWARN << "connection: duplicated connid " << conn->getConnId() << " " << conn->dump();
        return false;
    }
    IConnection_var ref(conn);
    _connections[conn->getConnId()] = ref;

    return true;
}

void Manager::removeConnection(const uint32_t connid)
{
    Connections::iterator it = _connections.find(connid);
    if(it != _connections.end())
    {
    	auto conn = it->second;
    	GLINFO << "remove connection " << conn->dump();
    	//断开处理事件
        _connections.erase(it);
    }
}

IConnection* Manager::getConnection(const uint32_t connid)
{
    Connections::iterator it = _connections.find(connid);
    if(it == _connections.end())
    {
        return 0;
    }
    return it->second.ptr();
}

uint32_t Manager::createTcpClient(const std::string& host, const int port, const int timeout, IClientHandler* handler)
{
	uint32_t newConnId = getConnectionId();
	IConnection_var conn(new TcpClient(host, port, newConnId, timeout, handler, this));

	if(conn->status() == IConnection::SOCKETERROR)
	{
		GLERROR << "connid: " << newConnId << " happen socket error";
		handler->onClose("socket error", conn.ptr());
		return 0;
	}

	//连接状态异步返回
	if(!addConnection(conn.ptr()))
	{
		GLERROR << "add connid: " << newConnId << " to " << host << ":" << port << " failed!";
		return 0;
	}

	GLINFO << "connect connid: " << newConnId << "|" << Manager::_step << " (" << (newConnId%Manager::_step) << ") to " << host << ":" << port;
	return newConnId;
}

//只能在IO worker线程中执行
uint32_t Manager::createTcpConnection(const SOCKET s, const uint32_t ip, const int port, const int timeout, IClientHandler* handler)
{
	uint32_t newConnId = getConnectionId();
	IConnection_var conn(new TcpConnection(s, ip, port, newConnId, timeout, handler, this));
	if(conn->status() != IConnection::ESTABLISHED)
	{
	     GLERROR << "create tcp connection " << addr_ntoa(ip) << ":" << port << " failed!";
	     SocketHelper::soclose(s);
	     return 0;
	}
	if(!addConnection(conn.ptr()))
	{
		GLERROR << "add connid: " << newConnId << " from " << addr_ntoa(ip) << ":" << port << " failed!";
	    return 0;
	}

	GLINFO << "accept connid: " << newConnId << "|" << Manager::_step << " (" << (newConnId%Manager::_step) << ") " << conn->dump();

	//连接回调
	handler->onConnected(conn.ptr());

	return newConnId;
}

uint32_t Manager::createUdpClient(const std::string& host, const int port, IClientHandler* handler)
{
	uint32_t newConnId = getConnectionId();
	IConnection_var conn(new UdpClient(host, port, newConnId, handler, this));

	if(conn->status() == IConnection::SOCKETERROR)
	{
		GLERROR << "connid: " << newConnId << " happen socket error";
		handler->onClose("socket error", conn.ptr());
		return 0;
	}

	if(!addConnection(conn.ptr()))
	{
		GLERROR << "add connid: " << newConnId << " to " << host << ":" << port << " failed!";
		return 0;
	}

	GLINFO << "connect connid: " << newConnId << "|" << Manager::_step << " (" << (newConnId%Manager::_step) << ") to " << host << ":" << port;

	//必须在addConnection处理连接事件，否则发消息时可能找不到连接ID
	((UdpConnection*)conn.ptr())->handleOnConnected();

	return newConnId;
}

//只能在IO worker线程中执行
uint32_t Manager::createUdpConnection(const SOCKET s, const uint32_t ip, const int port, const int timeout, const std::string& data, IClientHandler* handler, ISocketManager* listener)
{
	uint32_t curConnId = listener->getConnId(ip, port);
	if(curConnId)
	{
		IConnection* conn = getConnection(curConnId);
		if(conn)
		{
			GLINFO << "accept socket: " << s << " data size: " << data.size() << " " << conn->dump();
			conn->getHandler()->onData(data.data(), (uint32_t)data.size(), conn);
			return curConnId;
		}
	}

	uint32_t newConnId = getConnectionId();
	IConnection_var conn(new UdpConnection(s, ip, port, newConnId, timeout, handler, this, listener));
	if(conn->status() != UdpConnection::ESTABLISHED)
	{
	     GLERROR << "create tcp connection " << addr_ntoa(ip) << ":" << port << " failed!";
	     SocketHelper::soclose(s);
	     return 0;
	}
	if(!listener->setConnId(ip, port, newConnId))
	{
		GLERROR << "set connid: " << newConnId << " from " << addr_ntoa(ip) << ":" << port << " failed!";
	}
	if(!addConnection(conn.ptr()))
	{
		GLERROR << "add connid: " << newConnId << " from " << addr_ntoa(ip) << ":" << port << " failed!";
	    return 0;
	}

	GLINFO << "accept socket: " << s << " connid: " << newConnId << "|" << Manager::_step << " (" << (newConnId%Manager::_step) << ") " << conn->dump();

	//连接回调
	handler->onConnected(conn.ptr());

	//马上处理数据
	handler->onData(data.data(), (uint32_t)data.size(), conn.ptr());

	((UdpConnection*)conn.ptr())->_recvBytes += (uint32_t)data.size();

	return newConnId;
}

void Manager::onConnected(IConnection* conn)
{
	GLINFO << "---connected " << conn->dump();
}

//只能在IO worker线程中执行
void Manager::onClose(const char* reason, IConnection* conn)
{
    //remove the connection
	GLINFO << "---size: " << getConnectionSize() << " broken for " << reason << " " << conn->dump();

	//回调给业务层处理
	conn->getHandler()->onClose(reason, conn);

    removeConnection(conn->getConnId());
}

//只能在IO worker线程中执行
void Manager::onInitiativeClose(const char* reason, IConnection* conn)
{
    //remove the connection
	GLWARN << "---size: " << getConnectionSize() << " close by app connection for " << reason << " " << conn->dump();

	//回调给业务层处理
	conn->getHandler()->onInitiativeClose(reason, conn);

	removeConnection(conn->getConnId());
}

std::string Manager::dump(const std::string& filter)
{
	uint32_t count(0);
    std::string info;
	for(Connections::iterator it = _connections.begin(); it != _connections.end(); it++)
    {
		std::stringstream ss;
        ss  << "connid: " << it->first << "," << it->second->dump() << "\n";
        if(filter.empty() || ss.str().find(filter) != std::string::npos)
        {
        	info += ss.str();
        	count++;
        }
    }
	info += "total size=" + std::to_string(count);
	info += "\n";
    return info;
}



