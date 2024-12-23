#ifndef _NET_SELECTOR_EPOLL__
#define _NET_SELECTOR_EPOLL__

#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <set>
#include "selector.h"

namespace net
{
class Selector_epoll : public Selector
{
    int _epfd;
    typedef std::set<Socket*> SocketSet_t;
    SocketSet_t _sockets;
public:
    Selector_epoll();
    virtual ~Selector_epoll();

    virtual void select(Socket* s, int remove, int add);
    virtual void loop_once(uint32_t msec);

    virtual void remove(Socket* s);

    virtual std::ostream& trace(std::ostream& os) const { return os; }
};
}
#endif
