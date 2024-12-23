#include "handler.h"
#include <sstream>
using namespace net;

Handler::~Handler()
{
	close_timeout();
}

void Handler::close_timeout()
{
    if (Selector* sel = Selector::get())
    {
        select_timeout(); //remove
        sel->record_removed(this);
    }
}

void Socket::select(int remove, int add)
{
    if(!socket().isValid())
        throw socket_error("select with invalid socket");

    // clear invalid add
    if(socket().isShutdownSends())
        add &= ~SEL_WRITE;
    if(socket().isShutdownReceives())
        add &= ~SEL_READ;

    remove = remove & ~add; // do not remove that will add

    // clear not need
    remove &= socket().m_sock_flags.selevent;
    add &= ~socket().m_sock_flags.selevent;

    if((remove + add) != SEL_NONE)
    {
    	Selector::me()->select(this, remove, add); // MUST HAVE selector

        // save event after select ok.
        socket().m_sock_flags.selevent = (socket().m_sock_flags.selevent & ~remove) | add;
        socket().m_sock_flags.selected = 1;
    }
}

void Socket::remove()
{
    if(socket().m_sock_flags.selected == 1)
    {
    	if(Selector* sel = Selector::get())
    	{
    		sel->remove(this);
    	}
        socket().m_sock_flags.selected = 0;
    }
}

Socket::~Socket()
{
    remove(); // close();
}
