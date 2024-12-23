#ifndef __NET_QUEUE_H__
#define __NET_QUEUE_H__

#include <errno.h>
#include <stdio.h>
#include <deque>
#include <vector>
#include "utils/mutex.h"
#include "selector.h"
#include "handler.h"

namespace net
{
typedef int pipefd_t;

/// multi-producer single-consumer
class Notifier : public Socket
{
public:
    //  signals define:
    static const uint8_t input = 0x01;   //  has input
    static const uint8_t quit = 0x02;   //  exit
    static const uint8_t timer_task = 0x04; //  0x04 ~ 0x80 user self define

    /// <events>
    struct SignalHandler
    { //  --  > current thread
    	virtual ~SignalHandler() {}
        virtual void onSignals(uint8_t signals) = 0;
    };

public:
    Notifier();
    ~Notifier();
    ///  @brief  consumer方: 连上/断开当前线程的event looper(监听通知事件)
    ///  call by consumer thread
    void connect(SignalHandler* handler);
    void disconnect(SignalHandler* handler);
    ///  call by producer thread
    bool notify(uint8_t e = input);
    ///  call by consumer thread
    uint8_t wait(int32_t ms = -1);

    inline pipefd_t inputfd() const { return _fds[0]; }
    inline pipefd_t ouputfd() const { return _fds[1]; }

    inline void destroy() {}
private:
    uint8_t read();
    void handle(const int e);

private:
    SOCKET _fds[2];
    SignalHandler* _handler;
};

/// queue with nofity
/// 不限队列最大长度(app自己去限)
template <typename MESSAGE, typename NOTIFIER = Notifier>
class MessageQueue
{
public:
    typedef std::deque<MESSAGE*> Messages;
    typedef Notifier::SignalHandler SignalHandler;

public:
    MessageQueue(): _notify(1)
    {
    }
    virtual ~MessageQueue() { _clear(); }

    /// @brief  连接队列事件监听
    ///          call by consumer thread
    void connect(SignalHandler* handler)
    {
        notifier_.connect(handler);
    }
    /// @brief  断开队列事件监听
    void disconnect(SignalHandler* handler)
    {
        notifier_.disconnect(handler);
    }
    /// @brief  append message into queue
    ///         call by producer thread
    void push(MESSAGE* msg)
    {
        assert(msg);
        lin_io::ScopLock<lin_io::SpinLock> sync(mutex_);
        bool notify = queue_.empty() || _notify;
        queue_.push_back(msg);
        if (notify)
        {
            _notify = 0;
            notifier_.notify();
        }
    }
    /// @brief  pop message from queue
    /// @return return the message in queue head
    ///         if queue is empty,return NULL
    ///         call by consumer thread
    MESSAGE* pop()
    {
    	lin_io::ScopLock<lin_io::SpinLock> sync(mutex_);
        if (queue_.empty())
        {
            _notify = 1;
            return 0;
        }
        MESSAGE* msg = queue_.front();
        queue_.pop_front();
        return msg;
    }

    /// @brief  pop all messages in queue
    void get(std::deque<MESSAGE*>& msgs)
    {
        msgs.clear();
        lin_io::ScopLock<lin_io::SpinLock> sync(mutex_);
        queue_.swap(msgs);
        _notify = 1;
    }
    /// @brief  return current messages num in queue
    size_t size()
    {
    	lin_io::ScopLock<lin_io::SpinLock> sync(mutex_);
        return queue_.size();
    }

private:
    void _clear()
    {
        Messages msgs;
        get(msgs);
        for (typename Messages::iterator it = msgs.begin(); it != msgs.end(); ++it)
        {
            delete (*it);
        }
    }

private:
    lin_io::SpinLock mutex_;
    Messages queue_;
    NOTIFIER notifier_;
    uint8_t _notify; /// when queue was empty,force notify next time
};

template <typename MESSAGE>
class SingleConsumerQueue : public MessageQueue<MESSAGE, Notifier>
{
public:
    SingleConsumerQueue() {}
    virtual ~SingleConsumerQueue() {}
};

}

#endif
