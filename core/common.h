#ifndef _NET_COMMON__
#define _NET_COMMON__

#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <netdb.h>
#include <ifaddrs.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

namespace net
{
//判断地址格式是否是IP(xxx.xxx.xxx.xxx格式)
inline bool isip(const char* s)
{
    const char* t;
    int dots = 0;
    int ndigit = 0;

    for (t = s; *t != '\0'; t++) {
        switch (*t) {
        case '.':
            if (ndigit == 0)
                return false;
            dots++;
            ndigit = 0;
            continue;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            ++ndigit;
            continue;
        default:
            return false;
        }
    }
    if (dots != 3)
        return false;
    return true;
}

//  return ip is bigendian!!
inline uint32_t getHostByName(const char* name)
{
    if(!name)
    {
        return 0;
    }

    if(isip(name))
    {
        return ::inet_addr(name);
    }

    struct hostent* he = ::gethostbyname(name);
    if(!he) {
        return 0;
    }
    
    return (reinterpret_cast<in_addr*>(he->h_addr))->s_addr;
}

inline pthread_t getThreadId()
{
	static thread_local pthread_t t_tid = 0;
    if (t_tid == 0)
    {
        t_tid = syscall(__NR_gettid);
    }
    return t_tid;
}

inline int getLocalIp(std::vector<std::string>& ips)
{
	struct ifaddrs *ifaddr, *ifa;

	// 获取本地所有接口地址
	if (getifaddrs(&ifaddr) == -1)
	{
		std::cout << "Failed to get interface addresses." << std::endl;
		return -1;
	}

	// 遍历接口列表
	for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
			continue;

		// 获取地址家族
		int family = ifa->ifa_addr->sa_family;

		// 只处理IPv4地址
		if (family == AF_INET)
		{
			char host[NI_MAXHOST] = {0};
			int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
			if (s != 0)
			{
				std::cout << "Failed to get IP address." << std::endl;
				continue;
			}
			std::cout << ifa->ifa_name << ": " << host << std::endl;
			ips.push_back(std::string(host));
		}
	}

	// 释放内存
	freeifaddrs(ifaddr);
	return 0;
}

}

#endif
