#ifndef _NET_MANGER_H__
#define _NET_MANGER_H__

#include <fstream>
#include <iostream>
#include <stdexcept>
#include "utils/rc.h"
#include "connection.h"

namespace net
{
//连接管理类
class Manager : public ILinkCtrlHandler
{
public:
	Manager(const uint32_t id)
    : _seed(id%Manager::_step)
    {
    }

    virtual ~Manager() {}

    static Manager* get(const uint32_t id=0);

public:
    void removeConnection(const uint32_t connid);
    IConnection* getConnection(const uint32_t connid);

    //主动连接客户端:在IO主线程中调用
    uint32_t createTcpClient(const std::string& host, const int port, const int timeout, IClientHandler* handler);

    //被动连接客户端:在IO主线程中调用
    uint32_t createTcpConnection(const SOCKET s, const uint32_t ip, const int port, const int timeout, IClientHandler* handler);

    //主动连接客户端:在IO主线程中调用
    uint32_t createUdpClient(const std::string& host, const int port, IClientHandler* handler);

    //被动连接客户端:在IO主线程中调用
    uint32_t createUdpConnection(const SOCKET s, const uint32_t ip, const int port, const int timeout, const std::string& data, IClientHandler* handler, ISocketManager* listener);

    //获取连接数量
    uint32_t getConnectionSize() {return _connections.size();}
    std::string dump(const std::string& filter);
public:
    //继承ILinkCtrlHandler
    virtual void onConnected(IConnection* conn);
    virtual void onClose(const char* reason, IConnection* conn);
    virtual void onInitiativeClose(const char* reason, IConnection* conn);

protected:
    bool addConnection(IConnection* conn);
    uint32_t getConnectionId();

public:
    static void init(const int step)
    {
    	Manager::_step = step;
    }

private:
    static void destroy(void* ptr)
    {
        if(ptr)
            delete ((Manager*)ptr);
    }

private:
    typedef std::map<uint32_t, IConnection_var> Connections;
    Connections _connections;
    uint32_t 	_seed;
    static uint32_t _step;
};

}

#endif
