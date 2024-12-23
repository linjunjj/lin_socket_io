#ifndef __NET_UDP_CONNECTION_H__
#define __NET_UDP_CONNECTION_H__

#include <stdio.h>
#include <list>
#include <string>
#include <memory>
#include "utils/rc.h"
#include "utils/utility.h"
#include "handler.h"

namespace net
{
class UdpSocket : public Socket
{
public:
	struct Listener
	{
		virtual ~Listener() {}
		virtual void onData(UdpSocket* so, const char* data, const uint32_t size, const ipaddr_type* from) = 0;
		virtual void onTimeout(UdpSocket* so) = 0;

		//以下函数只有调用create或destroy才会触发回调
		virtual void onCreate(UdpSocket* so) {}
		virtual void onDestroy(UdpSocket* so, const int code, const char* reason) {}
	};

    UdpSocket(Listener* listener, const uint32_t ip, const int port);
    UdpSocket(Listener* listener, const std::string& host, const int port);
    virtual ~UdpSocket();

    SOCKET getSocket();
    ipaddr_type* getLocalAddr();
    virtual int send(const char* data, const uint32_t sz, const ipaddr_type* to);
    virtual int send(const char* data, const uint32_t sz, const uint32_t ip, const uint16_t port);

    int getBufferSize();
    void setBufferSize(const int size);
    int getRecvTimeout() {return _timeout;}
    void setRecvTimeout(const int msec);
    time_t getLastSendTime() {return _lastSendTs;}
    time_t getLastRecvTime() {return _lastRecvTs;}
	Listener* getListener() {return _listener;}

private:
    bool init();
    void bind(const uint32_t ip, const int port);

public:
	typedef void (*FindCallback)(UdpSocket* so);
	static void find(const uint32_t fd, FindCallback cb);
	static bool create(Listener* listener, const std::string& localIp, const int localPort);
	static void destroy(const uint32_t fd);

protected:
    bool flush() throw_exceptions;

    //继承Socket
    virtual void handle(const int ev);
    virtual void onTimeout();
    virtual void onRead();
    virtual void onWrite();

protected:
    Listener* _listener;
    int _timeout;
    time_t _lastRecvTs;
    time_t _lastSendTs;
    ipaddr_type _localAddr;

    struct UdpData
    {
    	std::string data;
    	ipaddr_type addr;
    	UdpData(const char* buf, const uint32_t len, const ipaddr_type* to):data(buf,len) {memcpy(&addr, &to, (size_t)sizeof(ipaddr_type));}
    };
    std::list<std::shared_ptr<UdpData> > _queue;
};

typedef lin_io::RcVar<UdpSocket> UdpSocket_var;
}

#endif
