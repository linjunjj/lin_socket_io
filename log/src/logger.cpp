#include "logger.h"
#include <chrono>

#ifdef __linux
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h> // gettid().
typedef pid_t thread_id_t;
#else
#include <sstream>
typedef unsigned int thread_id_t; // MSVC
#endif

#include <iostream>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include "log_util.h"
#include "log_time.h"
#include "logging.h"

namespace elog
{
thread_id_t gettid()
{
	static thread_local thread_id_t t_tid = 0;
    if (t_tid == 0)
    {
#ifdef __linux
        t_tid = syscall(__NR_gettid);
#else
        std::stringstream ss;
        ss << std::this_thread::get_id();
        ss >> t_tid;
#endif
    }
    return t_tid;
}

LogLine::LogLine(LogLevel level, const char *file, const char *function, uint32_t line)
{
	int pos(0);
	for(int i = 0; file[i]; i++)
	{
		if(file[i] == '/' || file[i] == '\\')
			pos = i;
	}

	static std::string szLogLevelName[5] = {"TRACE","DEBUG","INFO","WARN","ERROR"};
    *this << '[' << szLogLevelName[level] << ' ' << util::Timestamp::now().formatTimestamp()
    	  << ' ' << gettid() << ' ' << (file+pos+1) << " (" << line << ") " << function << "] ";
}

LogLine::~LogLine()
{
   *this << '\n';
   Logging::instance().finish();
}

//这个函数是最终调用的----------
void LogLine::append(const char *data, const uint32_t n)
{
    Logging::instance().produce(data, n);//增加日志内容到缓冲区
}

void LogLine::append(const char *data)
{
	if(data)//这里必须判断是否空指针
	{
		append(data, strlen(data));
	}
}

LogLine &LogLine::operator<<(bool arg)
{
    if (arg)
        append("true", 4);
    else
        append("false", 5);
    return *this;
}

LogLine &LogLine::operator<<(char arg)
{
    append(&arg, 1);
    return *this;
}

LogLine &LogLine::operator<<(short arg)
{
    char tmp[8];
    int len = util::i16toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(unsigned short arg)
{
    char tmp[8];
    int len = util::u16toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(int arg)
{
    char tmp[12];
    int len = util::i32toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(unsigned int arg)
{
    char tmp[12];
    int len = util::u32toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(long arg)
{
    char tmp[24];
    int len = util::i64toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(unsigned long arg)
{
    char tmp[24];
    int len = util::u64toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(long long arg)
{
    char tmp[24];
    int len = util::i64toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(unsigned long long arg)
{
    char tmp[24];
    int len = util::u64toa(arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(float arg)
{
    append(std::to_string(arg).c_str());
    return *this;
}

LogLine &LogLine::operator<<(double arg)
{
    append(std::to_string(arg).c_str());
    return *this;
}

LogLine &LogLine::operator<<(long double arg)
{
	append(std::to_string(arg).c_str());
    return *this;
}

LogLine &LogLine::operator<<(void* arg)
{
    char tmp[12];
    int len = util::i64toa((long long)arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(const void* arg)
{
    char tmp[12];
    int len = util::i64toa((long long)arg, tmp);
    append(tmp, len);
    return *this;
}

LogLine &LogLine::operator<<(const char *arg)
{
    append(arg);
    return *this;
}

LogLine &LogLine::operator<<(const std::string &arg)
{
    append(arg.c_str(), arg.length());
    return *this;
}

void InitLogger(const std::string& name, const std::string& path, const uint32_t nMax, const uint32_t nMb)
{
	//注意顺序
	Logging::instance().setRollSize(nMb);//设置每个文件的大小
	Logging::instance().setFileLimit(nMax);//最大文件数
	Logging::instance().setLogPath(path.c_str());
	Logging::instance().setLogFile(name.c_str());
}

void SetLogLevel(LogLevel level)
{
	Logging::instance().setLogLevel(level);
}

LogLevel GetLogLevel()
{
	return Logging::instance().getLogLevel();
}

void SetLogLimit(const int limit)
{
	Logging::instance().setFileLimit(limit);
}

std::string GetLogStatistic()
{
	return Logging::instance().statistic();
}
} // namespace log
