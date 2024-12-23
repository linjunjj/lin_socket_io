#include "selector_epoll.h"
#include <sys/epoll.h>
#include "log/logger.h"
#include "handler.h"

#define EPOLL_SIZE 100
using namespace net;

Selector_epoll::Selector_epoll()
: _epfd(-1)
{
	int maxevents(65535);
	char* buffer = getenv("MAX_EVENTS");
	if (buffer != NULL)
	{
		maxevents = atoi(buffer);
	}
	//GLINFO << "epoll max event size is " << maxevents;

    _epfd = epoll_create(maxevents);
    if (-1 == _epfd)
    {
        throw socket_error("epoll");
    }
}

Selector_epoll::~Selector_epoll()
{
    if (-1 != _epfd)
    {
        close(_epfd);
    }
}

inline void setupEpoll(int efd, int op, int sfd, epoll_event& ev)
{
    int ret = epoll_ctl(efd, op, sfd, &ev);
    if (ret != 0)
    {
        switch (errno)
        {
        case EBADF:
            GLERROR << "epfd or fd is not a valid file descriptor.";
            return;
        case EEXIST:
        	GLERROR << "was EPOLL_CTL_ADD, and the supplied file descriptor fd is already in epfd.";
            return;
        case EINVAL:
        	GLERROR << "epfd is not an epoll file descriptor, or fd is the same as epfd, or the requested operation op is not supported by this interface.";
            return;
        case ENOENT:
        	GLERROR << "op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not in epfd. ";
            return;
        case ENOMEM:
        	GLERROR << "There was insufficient memory to handle the requested op control operation.";
            return;
        case EPERM:
        	GLERROR << "The target file fd does not support epoll.";
            return;
        }
    }
}

void Selector_epoll::select(Socket* s, int remove, int add)
{
  epoll_event ev;
  ev.data.ptr = s;
  ev.events = EPOLLIN;

  std::pair<SocketSet_t::iterator, bool> p = _sockets.insert(s);
  if (p.second)
  {
    if (SEL_WRITE & add)
    {
        ev.events |= EPOLLOUT;
    }
    setupEpoll(_epfd, EPOLL_CTL_ADD, s->socket().getsocket(), ev);
  }
  else
  {
    unsigned int currstate = (s->socket().m_sock_flags.selevent & ~remove) | add;
    ev.events = 0;
    if (currstate & SEL_READ)
        ev.events |= EPOLLIN;
    if (currstate & SEL_WRITE)
        ev.events |= EPOLLOUT;
    setupEpoll(_epfd, EPOLL_CTL_MOD, s->socket().getsocket(), ev);
  }
}

void Selector_epoll::loop_once(uint32_t msec)
{
  epoll_event events[EPOLL_SIZE];
  int64_t lasttick = _tick;
  update_time();

  //dispatch timer events
  timout_run((int)(_tick - lasttick));

  uint32_t to = (uint32_t)countdown().next_timeout() ;
  msec = msec < to ? msec : to ;
  
  int waits = 0;
  for (;;)
  {
      waits = epoll_wait(_epfd, events, EPOLL_SIZE, msec);
      if (waits < 0)
      {
          if (EINTR == errno)
              continue;
          else
              throw socket_error("epoll");
      }
      else
      {
          break;
      }
  }

  if (waits == EPOLL_SIZE)
  {
      GLDEBUG << "epoll reach the max size: " << EPOLL_SIZE;
  }

  //dispatch io events
  for (int i = 0; i < waits; ++i)
  {
      Socket* sk = (Socket*)events[i].data.ptr;
      if (events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP))
      {
          notify_event(sk, SEL_READ);
      }
      if (events[i].events & EPOLLOUT)
      {
          notify_event(sk, SEL_WRITE);
      }
  }

  interrupt();
  clearRemoved();
  ++_loop_count;
}

void Selector_epoll::remove(Socket* s)
{
    epoll_event ev;
    setupEpoll(_epfd, EPOLL_CTL_DEL, s->socket().getsocket(), ev);
    _sockets.erase(s);
}
