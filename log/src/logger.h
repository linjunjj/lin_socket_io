#pragma once

#include <string>
#include <stdint.h> // uint32_t
#include <string.h>
#include <unistd.h>

namespace elog
{
// 日志等级
// Log level >= set level will be written to file.
enum LogLevel
{
	TRACE = 0,
	DEBUG = 1,
	INFO  = 2,
	WARN  = 3,
	ERROR = 4
};

// 日志格式
//  +------+-----------+-------+------+------+----------+------+
//  | level | time | thread id | file | line | function | logs |
//  +------+-----------+-------+------+------+----------+------+
class LogLine
{
public:
    LogLine(LogLevel level, const char *file, const char *function, uint32_t line);
    ~LogLine();

    /// Overloaded functions with various types of argument.
    LogLine &operator<<(bool arg);
    LogLine &operator<<(char arg);
    LogLine &operator<<(short arg);
    LogLine &operator<<(unsigned short arg);
    LogLine &operator<<(int arg);
    LogLine &operator<<(unsigned int arg);
    LogLine &operator<<(long arg);
    LogLine &operator<<(unsigned long arg);
    LogLine &operator<<(long long arg);
    LogLine &operator<<(unsigned long long arg);
    LogLine &operator<<(float arg);
    LogLine &operator<<(double arg);
    LogLine &operator<<(long double arg);
    LogLine &operator<<(void* arg);
    LogLine &operator<<(const void* arg);
    LogLine &operator<<(const char *arg);
    LogLine &operator<<(const std::string &arg);

private:
    void append(const char *data, const uint32_t len);
    void append(const char *data);
};

/// Log initialize
void InitLogger(const std::string& name, const std::string& path, const uint32_t nMax, const uint32_t nMb);
/// Set log level, default INFO.
void SetLogLevel(LogLevel level);
/// Set log file num limit
void SetLogLimit(const int limit);
/// Get log level.
LogLevel GetLogLevel();
/// Get log statistic
std::string GetLogStatistic();
}// name space log

/// Create a log_line with log level \p level .
/// Custom macro \p __REL_FILE__ is relative file path as filename.
#ifndef __REL_FILE__
#define __REL_FILE__ __FILE__
#endif
#define LOG(level) if (elog::GetLogLevel() <= level) elog::LogLine(level, __REL_FILE__, __FUNCTION__, __LINE__)

#define GLTRACE LOG(elog::TRACE)
#define GLDEBUG LOG(elog::DEBUG)
#define GLINFO  LOG(elog::INFO)
#define GLWARN  LOG(elog::WARN)
#define GLERROR LOG(elog::ERROR)

#define LOG_TRACE LOG(elog::TRACE)
#define LOG_DEBUG LOG(elog::DEBUG)
#define LOG_INFO  LOG(elog::INFO)
#define LOG_WARN  LOG(elog::WARN)
#define LOG_ERROR LOG(elog::ERROR)

// wrapper logger
#define WLOG(level, file, function, line) if (elog::GetLogLevel() <= level) elog::LogLine(level, file, function, line)

#define WLTRACE(file, function, line) WLOG(elog::TRACE, file, function, line)
#define WLDEBUG(file, function, line) WLOG(elog::DEBUG, file, function, line)
#define WLINFO(file, function, line)  WLOG(elog::INFO, file, function, line)
#define WLWARN(file, function, line)  WLOG(elog::WARN, file, function, line)
#define WLERROR(file, function, line) WLOG(elog::ERROR, file, function, line)


