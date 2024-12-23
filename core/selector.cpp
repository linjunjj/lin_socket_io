#include "selector.h"

#include <stdio.h>
#include <time.h>
#include "log/logger.h"
#include "handler.h"

#define HAVE_EPOLL 1

#if defined(HAVE_SELECT) || defined(WIN32)
#include "selector_sel.h"
#elif defined(HAVE_KQUEUE)
#error unsupport kqueue!
#elif defined(HAVE_POLL)
#error unsupport pull!
#elif defined(HAVE_EPOLL)
#include "selector_epoll.h"
#else
#error ERROR, NO SELECTOR
#endif

namespace net
{
TSS<Selector> Selector::gTls(Selector::destructor);

///return the selector of current thread
Selector* Selector::me()
{
	Selector* sel = gTls.get();
    if (!sel)
    {
#if defined(HAVE_SELECT) || defined(WIN32)
        sel = new net::Selector_sel();
#elif defined(HAVE_KQUEUE)
        sel = new net::Selector_kq();
#elif defined(HAVE_POLL)
        sel = new net::Selector_poll();
#elif defined(HAVE_EPOLL)
        sel = new net::Selector_epoll();
#elif defined(HAVE_NU_EPOLL)
        sel = new net::NuSelectorEpoll();
#else
#error ERROR, NO SELECTOR
#endif
        gTls.set(sel);
    }
    return sel;
}

Selector* Selector::get()
{
	return gTls.get();
}

void Selector::finalize()
{
	Selector* sel = gTls.get();
    if (sel)
    {
    	gTls.set(0);
        delete sel;
    }
    SocketHelper::finalize();
}

Selector::Selector()
: _now(0)
, _tick(0)
, _loop_count(0)
, _running(false)
, _interrupt_handler(0)
, _elapsed(0)
{
    update_time();
    // initialize net library when  first access socket in current thread
    SocketHelper::initialize();

    //	直接ignore掉
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        throw socket_error("signal");
    }
}

Selector::~Selector()
{
    if (_interrupt_handler)
    {
    	delete _interrupt_handler;
    	_interrupt_handler = 0;
    }
}

void Selector::mainloop(uint32_t ms)
{
	_running = true;
    while (_running)
    {
        loop_once(ms);
    }
}

void Selector::set_interrupt_handler(Handler* handler)
{
    if (_interrupt_handler)
    {
    	delete _interrupt_handler;
    }
    _interrupt_handler = handler;
}

void Selector::update_time()
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

    //use clock for timer
	struct timespec	ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

    _now = tv.tv_sec;
    _tick = ((int64_t)ts.tv_sec) * 1000 + ((int64_t)ts.tv_nsec) / (1000 * 1000) ;
}

void Selector::notify_event(Handler* s, int event)
{
    // assert(s);
    if (isRemoved(s))
    {
        return;
    }
    try
    {
        s->handle(event);
    }
    catch (std::exception& ex)
    {
    	GLERROR << "event: " << event << " error: " << ex.what();
    }
}

}
