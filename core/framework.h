#ifndef __NET_FRAMEWORK_H__
#define __NET_FRAMEWORK_H__

#include "listener.h"
namespace net
{
class Framework
{
private:
	Framework();
public:
	~Framework();
	static Framework& instance()
	{
		static Framework ins;
		return ins;
	}

public:
	//异步启动
	void async_start(const int procs = -1);
	void stop();

	//*协议层接口(协议层的handler已经继承了智能指针，必须要使用new, 框架负责释放内存)
	//hash值为0为无效, hash值是用来调度服务器接受的连接到哪个线程处理; timeout为连接超时时间, 单位为毫秒
	bool createTcpServer(IServerHandler* handler, const int port, const int timeoutMs);
	bool createTcpServer(IServerHandler* handler, const int port, const int timeoutMs, const uint32_t hashKey);

	bool createTcpClient(IClientHandler* handler, const std::string& host, const int port, const int timeoutMs);
	bool createTcpClient(IClientHandler* handler, const std::string& host, const int port, const int timeoutMs, const uint32_t hashKey);

	//hash值为0为无效, hash值是用来调度服务器接受的连接到哪个线程处理; timeout为连接接收超时时间, 单位为毫秒
	bool createUdpServer(IServerHandler* handler, const int port, const int timeoutMs);
	bool createUdpServer(IServerHandler* handler, const int port, const int timeoutMs, const uint32_t hashKey);
	bool createUdpServer(IServerHandler* handler, const std::string& ip, const int port, const int timeoutMs);
    bool createUdpServer(IServerHandler* handler, const std::string& ip, const int port, const int timeoutMs, const uint32_t hashKey);

    bool createUdpClient(IClientHandler* handler, const std::string& ip, const int port);
	bool createUdpClient(IClientHandler* handler, const std::string& ip, const int port, const uint32_t hashKey);

	//*应用层接口(应用层的handler, 不建议用智能指针管理, 必须要使用new, 框架负责释放内存)
	template<typename T>
	bool createTcpServer(typename T::Handler* handler, const int port, const int timeoutMs, const uint32_t hashKey)
	{
		return createTcpServer(new T(handler), port, timeoutMs, hashKey);
	}
	template<typename T>
	bool createTcpServer(typename T::Handler* handler, const int port, const int timeoutMs)
	{
		return createTcpServer(new T(handler), port, timeoutMs);
	}
	template<typename T>
	bool createUdpServer(typename T::Handler* handler, const int port, const int timeoutMs)
	{
		return createUdpServer(new T(handler), port, timeoutMs);
	}
	template<typename T>
	bool createUdpServer(typename T::Handler* handler, const int port, const int timeoutMs, const uint32_t hashKey)
	{
		return createUdpServer(new T(handler), port, timeoutMs, hashKey);
	}
    template<typename T>
	bool createUdpServer(typename T::Handler* handler, const std::string& ip, const int port, const int timeoutMs)
	{
		return createUdpServer(new T(handler), ip, port, timeoutMs);
	}
    template<typename T>
	bool createUdpServer(typename T::Handler* handler, const std::string& ip, const int port, const int timeoutMs, const uint32_t hashKey)
	{
		return createUdpServer(new T(handler), ip, port, timeoutMs, hashKey);
	}
	template<typename T>
	bool createTcpClient(typename T::Listener* handler, const std::string& host, const int port, const int timeoutMs)
	{
		return createTcpClient(new T(handler), host, port, timeoutMs);
	}
	template<typename T>
	bool createTcpClient(typename T::Listener* handler, const std::string& host, const int port, const int timeoutMs, const uint32_t hashKey)
	{
		return createTcpClient(new T(handler), host, port, timeoutMs, hashKey);
	}
	template<typename T>
	bool createUdpClient(typename T::Listener* handler, const std::string& host, const int port, const int timeoutMs)
	{
		return createUdpClient(new T(handler), host, port, timeoutMs);
	}
	template<typename T>
	bool createUdpClient(typename T::Listener* handler, const std::string& host, const int port, const int timeoutMs, const uint32_t hashKey)
	{
		return createUdpClient(new T(handler), host, port, timeoutMs, hashKey);
	}

	//删除接口
	bool deleteTcpServer(const int serverId);
	bool deleteUdpServer(const int serverId);
	bool deleteTcpClient(const int clientId);
	bool deleteUdpClient(const int clientId);
};
}
#endif

