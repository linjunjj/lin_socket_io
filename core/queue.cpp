#include "queue.h"
#include "log/logger.h"

using namespace net;
using namespace lin_io;

Notifier::Notifier() : _handler(0)
{
    _fds[0] = _fds[1] = -1;
    if (::pipe2(_fds, O_NONBLOCK))
        throw socket_error("create pipe");
    socket().attach(_fds[0]);
}

Notifier::~Notifier()
{
    remove();
    socket().detach();
    if (_fds[0] != -1)
        ::close(_fds[0]);
    if (_fds[1] != -1)
        ::close(_fds[1]);
}

/// @brief fire signal
bool Notifier::notify(uint8_t signal)
{
    int ret = ::write(_fds[1], (const char*)&signal, 1);
    int	err = errno;
    if (ret == -1 && !(err == EWOULDBLOCK || err == EINPROGRESS || err == EAGAIN || err == EINTR))
    {
    	GLERROR << "write pipe happen error: " << err;
        return false;
    }

    return true;
}

/// @brief 连接信号监听(selector会添加该fd的事件监听)
void Notifier::connect(SignalHandler* handler)
{
    if (!handler)
        return;
    _handler = handler;
    select(0, SEL_READ);
}

/// @brief 断开信号监听(selector也会移除该fd的事件监听)
void Notifier::disconnect(SignalHandler* handler)
{
    _handler = 0;
    remove();
}

/// @brief    blocking wait for signals
/// @param    ms   timeout value(ms). if <0 , wait for ever
/// @return   if timeout, return 0 ; else return signals
uint8_t Notifier::wait(int32_t ms)
{
    try
    {
        SocketHelper so;
        so.attach(_fds[0]);
        so.waitevent(SEL_READ, ms);
        so.detach();
        return read();
    }
    catch (const std::exception& e)
    {
        //logth(Warn, "Notifier.wait: %s", e.what());
        return 0;
    }
}

uint8_t Notifier::read()
{
    uint8_t signals = 0;
    uint8_t buf[1024];
    for (;;)
    {
        int ret = ::read(_fds[0], (char*)buf, sizeof(buf));
        int	err = errno;
        if (ret == 0 || (ret == -1 && !(err == EWOULDBLOCK || err == EINPROGRESS || err == EAGAIN || err == EINTR)))
        {
        	GLERROR << "read pipe happen error: " << err;
            remove();
        }
        if (ret > 0)
        {
            for (int i = 0; i < ret; ++i)
            {
                signals |= buf[i];
            }
        }
        else
        {
            break;
        }
        if (ret < (int)(sizeof(buf)))
            break;
    }
    return signals;
}

void Notifier::handle(const int ev)
{
    try
    {
        uint8_t signals = read();
        if (_handler)
            _handler->onSignals(signals);
    }
    catch (const std::exception& e)
    {
    	GLERROR << "read pipe happen error: " << e.what();
    }
}

