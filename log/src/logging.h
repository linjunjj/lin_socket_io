#pragma once
#include <list>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <stdint.h> // uint32_t
#include <stdio.h>
#include <string.h>
#include "logger.h"

namespace elog
{
enum RollType : uint8_t { FILE_NAME, FILE_DATE, FILE_SIZE };

/// 从缓冲区写入文件, and the file automatic roll with a setting size.
class LogFile
{
  public:
	LogFile();
	LogFile(uint32_t rollSize);
    ~LogFile();

    void scanFiles();
    void setLogPath(const char *path);
    void setLogFile(const char *file);

    void setRollSize(uint32_t size) {_rollSize = size;}
    void setFileLimit(uint32_t limit) {_fileLimit = (limit > 0 && limit < 1000) ? limit : 100;}
    void rollFile(const uint8_t flag = FILE_NAME);
    void renameFile();
    uint32_t write(const char *data, const uint32_t len);

  private:
    uint32_t _fileLimit;
    uint32_t _fileCount;    // file roll count.
    uint32_t _rollSize;     // size of MB.
    uint64_t _writtenBytes; // total written bytes.
    std::string _fileName;
    std::string _filePath;
    std::string _date;
    FILE *_fp;
    std::list<std::string> _hisFiles;
    static const uint32_t kBytesPerMb = 1 << 20;
    static constexpr const char *kDefaultLogFile = "./log";
};

class BufferBucket;
class LogData
{
public:
	LogData();
	LogData(const uint32_t capacity, const bool recycle=false);
	~LogData();

	uint32_t size() {return _size;}
	const char* data() {return _data;}
	bool own();
	bool recycle();
	uint32_t write(const char* buf, const uint32_t len);
	uint32_t capacity() {return _capacity;}

	void set_next(LogData* data);
	LogData* get_next();
protected:
    friend class BufferBucket;
	void alloc(const uint32_t capacity);

private:
	uint32_t _size;
	char* _data;
	uint32_t _capacity;
	bool _recycle;
	volatile uint8_t _used;//0未使用 1使用中
	LogData* _next;
};

class BufferBucket
{
public:
	BufferBucket(const int level,const int capacity);
	~BufferBucket();

	LogData* alloc();
	void release(LogData* data);
private:
	uint32_t _level;
	uint32_t _capacity;
	LogData* _bucket;
	volatile uint32_t _pos;
};

class BlockingBuffer
{
public:
    BlockingBuffer();
    ~BlockingBuffer();

    /// consume n bytes data to
    uint32_t consume(char *to, uint32_t n);

    /// 生产日志先写到缓冲区
    void produce(const char *from, uint32_t n);
    /// 把日志缓存区的数据块放到消费队列中,数组不够会阻塞
    void finish();

    /// 是否有数据可消费
    bool consumeable();

    std::string dump();
private:
    void release(LogData* data);
	LogData* alloc(const uint32_t n);

private:
	volatile int32_t _queueSize;//队列可用数据
	volatile uint32_t _consumePos;//已经消费的进度
	volatile uint32_t _producePos;//已经生产的进度
	volatile void** _logQueue;//日志数据消费队列
	BufferBucket* _bufferBucket1K;
	BufferBucket* _bufferBucket2K;
	BufferBucket* _bufferBucket4K;
	BufferBucket* _bufferBucket8K;
	BufferBucket* _bufferBucket16K;

private:
	volatile uint32_t _tryCount;
	volatile uint64_t _maxWaitTime;
	volatile uint64_t _addFailedNum;
};

class Logging
{
public:
    LogLevel getLogLevel() const { return _level;}
    void setLogLevel(LogLevel level) {_level = level;}
    void setLogFile(const char *file) {_logFile.setLogFile(file);}
    void setLogPath(const char *path) {_logFile.setLogPath(path);}
    void setRollSize(uint32_t size) {_logFile.setRollSize(size);}
    void setFileLimit(uint32_t limit) {_logFile.setFileLimit(limit);}

	static Logging& instance()
	{
		static Logging ins;
		return ins;
	}

    void produce(const char *data, const uint32_t n);
    void finish();

    /// 日志统计
    std::string statistic() const;

private:
    Logging();
    Logging(const Logging &) = delete;
    Logging &operator=(const Logging &) = delete;
    ~Logging();

    void sinkThreadFunc();

private:
    LogFile _logFile;
    bool _threadSync; // front-back-end sync.
    bool _threadExit; // background thread exit flag.
    LogLevel _level;
    volatile uint64_t _logCount; // count of produced logs.
    uint32_t _sinkCount;         // count of sinking to file.
    uint64_t _remainBytes;
    uint64_t _totalSinkTimes;   // total time takes of sinking to file.
    uint64_t _totalConsumeBytes;// total consume bytes.
    uint32_t _perConsumeBytes; // bytes of consume first-end data per loop.
    char *_outputBuffer;       // first internal buffer.
    std::thread _sinkThread;
    std::mutex  _bufferMutex; // internel buffer mutex.
    std::mutex  _condMutex;
    std::condition_variable _proceedCond;  // for background thread to proceed.
    std::condition_variable _hitEmptyCond; // for no data to consume.
    BlockingBuffer *_blockingBuffer;
};
}
