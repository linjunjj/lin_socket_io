#include "logging.h"
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

#include <map>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include "utils/atomic.h"
#include "utils/thread_local.h"
#include "log_util.h"
#include "log_time.h"

namespace elog
{
#define LOG_LIST_MAX_SIZE 0x4000//-1=0x3FFF
#define LOG_DATA_MAX_SIZE 1024*1024*16
#define LOG_DATA_SIZE_1K 1024*1
#define LOG_DATA_SIZE_2K 1024*2
#define LOG_DATA_SIZE_4K 1024*4
#define LOG_DATA_SIZE_8K 1024*8
#define LOG_DATA_SIZE_16K 1024*16

/// 日志缓冲区节点
struct LogDataNode
{
	LogData* head = NULL;
	LogData* tail = NULL;
};
//thread_local LogDataNode t_node;

// LogSink constructor
LogFile::LogFile()
:LogFile(10)
{
}

LogFile::LogFile(uint32_t rollSize)
:_fileLimit(10)
,_fileCount(0)
,_rollSize(rollSize)
,_writtenBytes(0)
,_fileName(kDefaultLogFile)
,_date(util::Timestamp::now().date())
,_fp(nullptr)
{
}

LogFile::~LogFile()
{
    if (_fp)
        fclose(_fp);
}

void LogFile::setLogPath(const char* path)
{
#define POWER 0755
	_filePath = path;
	for(int i = 0; i < (int)_filePath.size(); i++)
	{
		if(_filePath[i] == '\\')
			_filePath[i] = '/';
	}
	//开始找第一个目录名称
	std::string::size_type pos = _filePath.find('/');
	if(pos == std::string::npos)
	{
		if(mkdir(_filePath.c_str(), POWER) != 0)
		{
			if(errno != EEXIST)
			{
				std::cout << _filePath << " mkdir error: " << strerror(errno) << std::endl;
			}
			else//存在才扫描
			{
				scanFiles();
			}
		}
		return;
	}
	//历遍所有目录名称
	while(pos != std::string::npos)
	{
		//创建目录
		std::string dirPath = _filePath.substr(0, pos);
		if(dirPath.empty() || dirPath=="." || dirPath == "..")
		{
			//继续找下一个目录名称
			pos = _filePath.find('/', pos+1);
			continue;
		}
		if(mkdir(dirPath.c_str(), POWER) != 0)
		{
			if(errno != EEXIST)
			{
				std::cout << dirPath << " mkdir error: " << strerror(errno) << std::endl;
				return;
			}
		}
		//继续找下一个目录名称
		pos = _filePath.find('/', pos+1);
	}

	//创建完整目录
	if(mkdir(_filePath.c_str(), POWER) != 0)
	{
		if(errno != EEXIST)
		{
			std::cout << _filePath << " mkdir error: " << strerror(errno) << std::endl;
		}
		else//存在才扫描
		{
			scanFiles();
		}
	}
}

void LogFile::scanFiles()
{
	DIR* dir = opendir(_filePath.c_str());
	if(dir == NULL)
	{
		std::cout << _filePath << " opendir error: " << strerror(errno) << std::endl;
		closedir(dir);
		return;
	}

	std::map<time_t, std::list<std::string> > hisFiles;
	struct dirent* de = NULL;
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_ino == 0 || de->d_name[0] == '.')
			continue;

		std::string fileName = de->d_name;
		std::string fileFullName = _filePath + "/" + fileName;

		struct stat fileStat;
		if(stat(fileFullName.c_str(), &fileStat) != 0)
		{
			std::cout << fileFullName << " stat error: " << strerror(errno) << std::endl;
			continue;
		}
		if(S_ISREG(fileStat.st_mode))//是否是一个常规文件
		{
			hisFiles[fileStat.st_mtime].push_back(fileFullName);
			std::cout << "file:" << fileFullName << " mtime: " << fileStat.st_mtime << std::endl;
		}
	}
	closedir(dir);

	//放在文件队列中
	for(auto it = hisFiles.begin(); it != hisFiles.end(); it++)
	{
		std::copy(it->second.begin(), it->second.end(), std::back_inserter(_hisFiles));
	}
	std::cout << _filePath << " file size: " << _hisFiles.size() << std::endl;

	//检查文件数是否超过限制值
	while(_hisFiles.size() > _fileLimit)
	{
		std::string fileFullName = _hisFiles.front();
		_hisFiles.pop_front();
		if(remove(fileFullName.c_str()) != 0)
		{
			std::cout << fileFullName << " remove error: " << strerror(errno) << std::endl;
		}
	}
}

// Set log file should reopen file.
void LogFile::setLogFile(const char *file)
{
    _fileName = _filePath + std::string("/") + file;
    rollFile();
}

void LogFile::rollFile(const uint8_t flag)
{
    if (_fp)
        fclose(_fp);

    switch(flag)
    {
    case FILE_NAME://文件名改变
    	break;
    case FILE_DATE://文件日期改变
    case FILE_SIZE://文件大小改变
    	renameFile();
    	break;
    default:break;
    }

    std::string file(_fileName + ".log");
    _fp = fopen(file.c_str(), "a+");
    if (!_fp)
    {
        fprintf(stderr, "Faild to open file:%s\n", file.c_str());
        exit(-1);
    }

    _fileCount++;
}

void LogFile::renameFile()
{
	//最大偿试1000次,意思就是如果重启时已经有1000个日志文件名称被使用,那么切换改名失败
	for(uint32_t i = 0; i < 1000; i++)
	{
		std::string defautFile = _fileName + ".log";
		std::string targetFile  = _fileName + '-' + _date + '-' + std::to_string(_fileCount) + ".log";
		if(access(targetFile.c_str(), 0) != 0)//0 exist
		{
			//不存在,文件名可用, 改名
			rename(defautFile.c_str(), targetFile.c_str());
			_hisFiles.push_back(targetFile);
			break;
		}
		_fileCount++;
	}
	//检查文件数是否超过限制值
	while(_hisFiles.size() > _fileLimit)
	{
		std::string fileFullName = _hisFiles.front();
		_hisFiles.pop_front();
		if(remove(fileFullName.c_str()) != 0)
		{
			std::cout << fileFullName << " remove error: " << strerror(errno) << std::endl;
		}
	}
}

uint32_t LogFile::write(const char *data, const uint32_t len)
{
	if(!_fp)//文件未打开前,标准输出
	{
		std::string log(data,len);
		printf("%s", log.c_str());
		return len;
	}

    std::string today(util::Timestamp::now().date());
    if (_date.compare(today))
    {
        rollFile(FILE_DATE);
        _date.assign(today);
        _fileCount = 1;//计数从1开始
    }

    uint64_t rollBytes = _rollSize * kBytesPerMb;
    if (_writtenBytes % rollBytes + len > rollBytes)
        rollFile(FILE_SIZE);

    uint32_t n = fwrite(data, 1, len, _fp);
    uint32_t remain = len - n;
    while (remain > 0)
    {
    	uint32_t x = fwrite(data + n, 1, remain, _fp);
        if (x == 0)
        {
            int err = ferror(_fp);
            if (err)
                fprintf(stderr, "File write error: %s\n", strerror(err));
            break;
        }
        n += x;
        remain -= x;
    }
    fflush(_fp);
    _writtenBytes += len - remain;

    return len - remain;
}

LogData::LogData()
:_size(0),_data(NULL),_capacity(0),_recycle(true),_used(0),_next(0)
{
}

LogData::LogData(const uint32_t capacity, const bool recycle)
:_size(0),_data(new char[capacity]),_capacity(capacity),_recycle(recycle),_used(0),_next(0)
{
}

LogData::~LogData()
{
	delete[] _data;
}

//返回写入的大小
uint32_t LogData::write(const char* buf, const uint32_t len)
{
	uint32_t writed(len);
	if(writed > _capacity - _size)
	{
		writed = _capacity - _size;
	}

	memcpy(_data + _size, buf, writed);
	_size += writed;
	return writed;
}

void LogData::alloc(const uint32_t capacity)
{
	if(_data)
	{
		delete[] _data;
	}
	//重新分配空间
	_capacity = capacity;
	_data = new char[_capacity];
	_size = 0;
	_next = 0;
}

bool LogData::own()
{
	//如果从0到1成功说明可用
	return lin_io::try_compare_and_swap_uint8(&_used, 0, 1);
}

bool LogData::recycle()
{
	if(_recycle)
	{
		lin_io::exchange_uint8(&_used,0);
		_size = 0;
		_next = 0;
	}
	return _recycle;
}

void LogData::set_next(LogData* data)
{
	_next = data;
}

LogData* LogData::get_next()
{
	return _next;
}

BufferBucket::BufferBucket(const int level,const int capacity)
:_level(level),_capacity(capacity),_pos(0)
{
	_bucket = new LogData[capacity];
	for(int i = 0; i < capacity; i++)
	{
		_bucket[i].alloc(level);
	}
}

BufferBucket::~BufferBucket()
{
	delete[] _bucket;
}

void BufferBucket::release(LogData* data)
{
	if(data->recycle())
	{
		return;
	}
	delete data;
}

LogData* BufferBucket::alloc()
{
	uint32_t pos = lin_io::increment_uint32(&_pos,1) & (_capacity-1);

	if(_bucket[pos].own())
	{
		return &_bucket[pos];
	}
	return (new LogData(_level));
}

BlockingBuffer::BlockingBuffer()
:_queueSize(0)
,_consumePos(1)//从1开始消费
,_producePos(0)
,_logQueue(new volatile void*[LOG_LIST_MAX_SIZE])
,_bufferBucket1K(new BufferBucket(LOG_DATA_SIZE_1K,16384))//16M
,_bufferBucket2K(new BufferBucket(LOG_DATA_SIZE_2K,4096))//8M
,_bufferBucket4K(new BufferBucket(LOG_DATA_SIZE_4K,2048))//8M
,_bufferBucket8K(new BufferBucket(LOG_DATA_SIZE_8K,512))//4M
,_bufferBucket16K(new BufferBucket(LOG_DATA_SIZE_16K,256))//4M
,_tryCount(0)
,_maxWaitTime(0)
,_addFailedNum(0)
{
	for(int i = 0; i < LOG_LIST_MAX_SIZE; i++)
	{
		_logQueue[i] = 0;
	}
}

BlockingBuffer::~BlockingBuffer()
{
	delete[] _logQueue;//指针里数据还要释放
	delete _bufferBucket1K;
	delete _bufferBucket2K;
	delete _bufferBucket4K;
	delete _bufferBucket8K;
	delete _bufferBucket16K;
}

std::string BlockingBuffer::dump()
{
	std::ostringstream text;
	text << "try count: " << _tryCount << " max wait time: " << _maxWaitTime << " us, add failed count: " << _addFailedNum
		 << ", {size=" << _queueSize << " produce pos=" << _producePos << " consume pos=" << _consumePos << "}";
	return text.str();
}

// 从链表缓冲区获取日志
uint32_t BlockingBuffer::consume(char *to, uint32_t n)
{
	uint32_t readBytes(0);
	while(_queueSize > 0)
	{
		uint32_t pos = _consumePos & (LOG_LIST_MAX_SIZE-1);

		//从队列取出数据
		LogData* data = (LogData*)_logQueue[pos];
		if(!data)
		{
			break;
		}

		//循环读取所有日志块
		LogData* next = NULL;
		while(data)
		{
			if(readBytes + data->size() > n)
			{
				_logQueue[pos] = data;//还有数据未处理完
				return readBytes;
			}
			memcpy(to + readBytes, data->data(), data->size());
			readBytes += data->size();

			next = data->get_next();
			release(data);
			data = next;
		}

		//所有日志块读完才算是处理完一条日志
		_logQueue[pos] = NULL;
		_consumePos++;
		lin_io::increment_int32(&_queueSize,-1);
		//std::cout << pos << "==========>consume pos: " << _consumePos << " produce pos: " << _producePos << " queue size: " << _queueSize << std::endl;
	}
    return readBytes;
}

// 把日志写到缓冲区
void BlockingBuffer::produce(const char *from, uint32_t n)
{
	//限制一次写入的日志大小不能超过16M
	if(n > LOG_DATA_MAX_SIZE)
	{
		n = LOG_DATA_MAX_SIZE;
	}

	//先分配内存
	LogDataNode& t_node = com::ThreadLocal<LogDataNode>::get();
	if(t_node.head == NULL)
	{
		t_node.head = this->alloc(n);
		t_node.tail = t_node.head;
	}

	//写入数据
	uint32_t bytes = t_node.tail->write(from, n);
	if(bytes < n)
	{
		//重新分配内存
		LogData* data = this->alloc(n-bytes);
		data->write(from + bytes, n - bytes);

		t_node.tail->set_next(data);
		t_node.tail = data;
	}
}

void BlockingBuffer::finish()
{
	//blocking, 队列已满
	uint64_t bt = util::Timestamp::now().timestamp();
	while(lin_io::increment_int32(&_queueSize,1) >= LOG_LIST_MAX_SIZE)
	{
		lin_io::increment_uint32(&_tryCount,1);
		lin_io::increment_int32(&_queueSize,-1);
		usleep(5);
	}

	uint64_t et = util::Timestamp::now().timestamp();
	if(et - bt > _maxWaitTime)
	{
		_maxWaitTime = et - bt;
	}

	uint32_t pos = lin_io::increment_uint32(&_producePos,1) & (LOG_LIST_MAX_SIZE-1);
	//std::cout << pos << "----------------------------->produce pos: " << _producePos << std::endl;

	LogDataNode& t_node = com::ThreadLocal<LogDataNode>::get();
	if(!lin_io::try_compare_swap_pointer(&_logQueue[pos],0,t_node.head))
	{
		_addFailedNum++;
		std::cout << "error: " << _producePos << " not null, add log data failed" << std::endl;
		lin_io::increment_int32(&_queueSize,-1);
	}
	t_node.head = NULL;
	t_node.tail = NULL;
}

bool BlockingBuffer::consumeable()
{
	return _queueSize > 0 ? true : false;
}

void BlockingBuffer::release(LogData* data)
{
	if(data->capacity() <= LOG_DATA_SIZE_1K)
	{
		_bufferBucket1K->release(data);
		return;
	}
	if(data->capacity() <= LOG_DATA_SIZE_2K)
	{
		_bufferBucket2K->release(data);
		return;
	}
	if(data->capacity() <= LOG_DATA_SIZE_4K)
	{
		_bufferBucket4K->release(data);
		return;
	}
	if(data->capacity() <= LOG_DATA_SIZE_8K)
	{
		_bufferBucket8K->release(data);
		return;
	}
	if(data->capacity() <= LOG_DATA_SIZE_16K)
	{
		_bufferBucket16K->release(data);
		return;
	}

	std::cout << "--------------------delete log data size: " << data->capacity() << std::endl;
	delete data;
}

LogData* BlockingBuffer::alloc(const uint32_t n)
{
	if(n <= LOG_DATA_SIZE_1K)
	{
		return _bufferBucket1K->alloc();
	}
	if(n <= LOG_DATA_SIZE_2K)
	{
		return _bufferBucket2K->alloc();
	}
	if(n <= LOG_DATA_SIZE_4K)
	{
		return _bufferBucket4K->alloc();
	}
	if(n <= LOG_DATA_SIZE_8K)
	{
		return _bufferBucket8K->alloc();
	}
	if(n <= LOG_DATA_SIZE_16K)
	{
		return _bufferBucket16K->alloc();
	}
	std::cout << "--------------------new log data size: " << n << std::endl;
	return (new LogData(n));
}

// LimLog Constructor.
Logging::Logging()
:_logFile()
,_threadSync(false)
,_threadExit(false)
,_level(LogLevel::INFO)
,_logCount(0)
,_sinkCount(0)
,_remainBytes(0)
,_totalSinkTimes(0)
,_totalConsumeBytes(0)
,_perConsumeBytes(0)
,_outputBuffer(nullptr)
,_sinkThread()
,_bufferMutex()
,_condMutex()
,_proceedCond()
,_hitEmptyCond()
,_blockingBuffer(nullptr)
{
    _outputBuffer = static_cast<char *>(malloc(LOG_DATA_MAX_SIZE));
    _blockingBuffer = new BlockingBuffer();

    //日志写入文件线程
    _sinkThread = std::thread(&Logging::sinkThreadFunc, this);

//    uint32_t aa=4294967000-100;
//    for(uint64_t i = aa; i < 4294967300+100; i++)
//    {
//    	uint32_t pos = ++aa;
//    	std::cout << i << "==============+++============pos: " << pos << " -> val: " << (aa & 0x3FFF) << std::endl;
//    }
}

Logging::~Logging()
{
    {
        std::unique_lock<std::mutex> lock(_condMutex);
        _threadSync = true;
        _proceedCond.notify_all();
        _hitEmptyCond.wait(lock);
    }
    {
        // stop sink thread.
        std::lock_guard<std::mutex> lock(_condMutex);
        _threadExit = true;
        _proceedCond.notify_all();
    }

    if (_sinkThread.joinable())
    	_sinkThread.join();

    free(_outputBuffer);
    delete _blockingBuffer;
}

// Sink log info to file with async.
void Logging::sinkThreadFunc()
{
    while (!_threadExit)
    {
        // 把日志缓冲区的数据移动到内部缓存区中
		_perConsumeBytes = _blockingBuffer->consume(_outputBuffer, LOG_DATA_MAX_SIZE);//统计该次消息的数据大小

        // not data to sink, go to sleep, 50us.
        if (_perConsumeBytes == 0)
        {
        	if(_blockingBuffer->consumeable())
        	{
        		continue;//还有数据可消费
        	}
            std::unique_lock<std::mutex> lock(_condMutex);

            // 如果前端产生一个同步操作，继续消费.
            if (_threadSync)
            {
                _threadSync = false;
                continue;
            }

            _hitEmptyCond.notify_one();//唤醒前端继续
            _proceedCond.wait_for(lock, std::chrono::microseconds(1000*1000));//由前端唤醒
        }
        else
        {
            uint64_t bTime = util::Timestamp::now().timestamp();
            //把缓冲区日志写入文件
            uint64_t bytes = _logFile.write(_outputBuffer, _perConsumeBytes);
            uint64_t eTime = util::Timestamp::now().timestamp();
            _remainBytes += _perConsumeBytes - bytes;
            _totalSinkTimes += eTime - bTime;//写入花费的时间
            _sinkCount++;
            _totalConsumeBytes += bytes;//统计总写入数据的大小
            _perConsumeBytes = 0;
        }
    }
}

void Logging::produce(const char *data, const uint32_t n)
{
    _blockingBuffer->produce(data, n);
}

void Logging::finish()
{
	_blockingBuffer->finish();
    lin_io::increment_uint64(&_logCount,1);
    //notify
    _proceedCond.notify_one();
}

// List related data of loggin system.
// \TODO write this info to file and stdout with stream.
std::string Logging::statistic() const
{
	std::ostringstream text;
	text << "\n"
	     << "============= logging system related data =============\n"
	     << "+log_stat total produced logs count: " << _logCount << "\n"
	     << "+log_stat total bytes of sinking to file: " << _totalConsumeBytes << " bytes\n"
	     << "+log_stat average bytes of sinking to file: " <<  (_sinkCount == 0 ? 0 : _totalConsumeBytes / _sinkCount) << " bytes\n"
	     << "+log_stat count of sinking to file: " <<  _sinkCount << "\n"
	     << "+log_stat total microseconds takes of sinking to file: " << _totalSinkTimes << " us\n"
	     << "+log_stat average microseconds takes of sinking to file: " << (_sinkCount == 0 ? 0 : _totalSinkTimes / _sinkCount)  << " us\n"
	     << "+log_stat total bytes of remain: " << _remainBytes << " bytes\n"
	     << "+log_stat " << _blockingBuffer->dump() << "\n"
	     << "=======================================================\n";
    return text.str();
}

} // namespace log
