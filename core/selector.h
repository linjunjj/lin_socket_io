#ifndef __NET_SELECTOR__
#define __NET_SELECTOR__

#include <set>
#include <algorithm>
#include <stdio.h>
#include "socket_helper.h"
#include "countdown.h"

namespace net
{
class Handler;
class Socket;

template <typename T>
class TSS
{
public:
    typedef void (*Destructor)(void* ptr);
    TSS(Destructor destructor = 0) { pthread_key_create(&_key, (Destructor)destructor); }
    ~TSS() { pthread_key_delete(_key); }
    inline T* get() { return static_cast<T*>(pthread_getspecific(_key)); }
    inline void set(T* p) { pthread_setspecific(_key, p); }
    inline void finalize() {} //do nothing , pthread api support auto destructor callback when thread exit
private:
    pthread_key_t _key;
};

class Selector
{
public:
    static Selector* me();
    static Selector* get();
    static void finalize();

public:
    Selector();
    virtual ~Selector();
    ///	loop control
    void stop() { _running = false; }
    bool isRunning() const { return _running; }
    void mainloop(uint32_t ms = 50);

    virtual void loop_once(uint32_t msec = 50) = 0;

    ///	time cache
    ///	@return current time for sec ( wall clock! )
    time_t now() { return _now; }
    ///	@return current time for msec( jiffies clock! )
    int64_t tick() { return _tick; }
    /// @return total loop times
    int64_t loop_times() { return _loop_count; }

    void set_interrupt_handler(Handler* handler);
    ///	for io event
    virtual void remove(Socket* s) {}
    virtual void select(Socket* s, int remove, int add) = 0;
    ///	for timer event
    void select_timeout(Handler* s, int timeout) { _countdown.select_timeout(s, timeout); }

    virtual std::ostream& trace(std::ostream& os) const = 0;
protected:
    // XXX
    void interrupt()
    {
        if (_interrupt_handler)
            notify_event(_interrupt_handler, 0);
    }

    // check : destroy in loop
    bool isRemoved(void* s) const { return _removed.find(s) != _removed.end(); }
    void clearRemoved() { _removed.clear(); }
    void record_removed(void* s) { if(_running) _removed.insert(s); }

    void notify_event(Handler* s, int event);

    void update_time();

    /// check and fire timers
    void timout_run(int elapsed)
    {
      /// select_XXX调用timeout_run的频率并不按tick来
      _elapsed += elapsed;
      if (_elapsed < Countdown::TIME_CLICK)
          return;
      elapsed = _elapsed;
      _elapsed = 0;

      Countdown::timeout_result result;
      _countdown.click_elapse(result, elapsed);
      for (Countdown::iterator i = result.begin(); i != result.end(); ++i)
      {
          notify_event(i->handle(), SEL_TIMEOUT);
      }
    }

    //friend class Handler;
    friend class Handler;
    Countdown& countdown() { return _countdown; }
protected:
    time_t _now; //	current time of sec ( wall clock )
    int64_t _tick; //	current time of msec( jiffies clock! )
    int64_t _loop_count; // total loop times
private:
    static void destructor(void* sel)
    {
        if (sel)
            delete ((Selector*)sel);
    }

    std::set<void*> _removed;
    Countdown _countdown;

    bool _running;
    Handler* _interrupt_handler;

    int _elapsed; //  累计到 >= Countdown::TIME_CLICK
    static TSS<Selector> gTls;
};

}

#endif
