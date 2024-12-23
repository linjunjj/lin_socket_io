#include "log_time.h"

#include <chrono>
#include <algorithm>

#ifdef __linux
#include <sys/time.h>
#endif

#include <stdio.h>

namespace util
{
Timestamp Timestamp::now()
{
    uint64_t timestamp = 0;

#ifdef __linux
    // use gettimeofday(2) is 15% faster then std::chrono in linux.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timestamp = tv.tv_sec * kMicrosecondOneSecond + tv.tv_usec;
#else
    timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
#endif

    return Timestamp(timestamp);
}

std::string Timestamp::datetime() const
{
	static thread_local time_t t_second = 0;
	static thread_local char t_datetime[24]; // 20190816 15:32:25

    time_t nowsec = _timestamp / kMicrosecondOneSecond;
    if (t_second != nowsec)
    {
    	t_second = nowsec;
    	struct tm st_time;
    	localtime_r(&t_second, &st_time);
        strftime(t_datetime, sizeof(t_datetime), "%Y%m%d %H:%M:%S", &st_time);
    }

    return t_datetime;
}

std::string Timestamp::datetime2() const
{
	static thread_local time_t t_second2 = 0;
	static thread_local char t_datetime2[26]; // 2019-08-16 15:32:25

    time_t nowsec = _timestamp / kMicrosecondOneSecond;
    if (t_second2 != nowsec)//t_second < nowsec 时间支持回退
    {
    	t_second2 = nowsec;
        struct tm st_time;
        localtime_r(&t_second2, &st_time);
        strftime(t_datetime2, sizeof(t_datetime2), "%Y-%m-%d %H:%M:%S", &st_time);
    }

    return t_datetime2;
}

std::string Timestamp::time() const
{
    std::string time(datetime(), 9, 16);
    time.erase(std::remove(time.begin(), time.end(), ':'), time.end());
    return time;
}

std::string Timestamp::date() const
{
	return std::string(datetime(), 0, 8);
}

std::string Timestamp::formatTimestamp() const
{
    char format[28];
    unsigned int microseconds = static_cast<unsigned int>(_timestamp % kMicrosecondOneSecond);
    snprintf(format, sizeof(format), "%s.%06u", datetime2().c_str(),microseconds);
    return format;
}

} // namespace util
