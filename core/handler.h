#ifndef _NET_HANDLER__
#define _NET_HANDLER__

#include <time.h>
#include "selector.h"

namespace net
{
class Handler
{
public:
    // handle event. notify one by one
    virtual void handle(const int ev) = 0;

    enum { INFTIMO = -1,MAXTIMO = (size_t(-1) >> 1) };
    void select_timeout(const int timeout = INFTIMO)
    {
    	Selector::me()->countdown().select_timeout(this, timeout);
    }
    Handler() {}
    virtual ~Handler();

public:
    void close_timeout();
protected:
    friend class Countdown;
    Countdown::iterator m_iter;// manage by Countdown

    // non-copyable
    Handler(const Handler&) {}
    void operator=(const Handler&);
};

class Socket : public Handler
{
public:
    virtual void select(int remove, int add);
    virtual void remove();

    Socket() {}

    SocketHelper& socket() { return m_socket; }
protected:
    virtual ~Socket();
    SocketHelper m_socket;
};
}

#endif
