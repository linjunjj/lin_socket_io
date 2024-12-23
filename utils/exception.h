#ifndef _EASY_EXCEPTION__
#define _EASY_EXCEPTION__

#include <string>
#include <errno.h>

class RuntimeException : public std::exception
{
public:
	RuntimeException()
    {
		_code = errno;
		_massage = strerror(errno);
    }
	RuntimeException(const int code) : _code(code)
    {
		_massage = strerror(_code);
    }
	RuntimeException(const std::string& massage): _massage(massage)
    {
		_code = errno;
    }
	RuntimeException(const int code, const std::string& massage) : _code(code), _massage(massage)
    {
    }

	virtual ~RuntimeException()
	{
	}

	int getCode() const {return _code;}
	std::string getMessage() {return _massage;}
	virtual const char* what() const throw() { return _massage.c_str(); }

	static int getLastError();
private:
	int _code;
	std::string _massage;
};

inline int RuntimeException::getLastError()
{
	return errno;
}

//Rpc 异常
struct RpcException : public std::exception
{
	enum Type
	{
		//fb thrift  throw exception
		UNKNOWN = 0,
		UNKNOWN_METHOD = 1,
		INVALID_MESSAGE_TYPE = 2,
		WRONG_METHOD_NAME = 3,
		BAD_SEQUENCE_ID = 4,
		MISSING_RESULT = 5,
		INTERNAL_ERROR = 6,
		PROTOCOL_ERROR = 7,
		INVALID_TRANSFORM = 8,
		INVALID_PROTOCOL = 9,
		UNSUPPORTED_CLIENT_TYPE = 10,

		//framework throw exceptin
		IO_MAYBE_BLOCK = 1001,
		IO_BUSY = 1002,
		SERVER_UNAVAILABLE = 1003,
		CONNECT_TIMEOUT = 1004,
		CONNECT_FAILED = 1005,
		TIMEOUT = 1006,
		CONNECTION_CLOSED = 1007,
		WORKER_QUEUE_FULL = 1008,
		CONNECTION_LOST = 1009,
		//app user throw exception
		APP_ERR_CODE = 2000,
	};

	RpcException() :_type(UNKNOWN)
	{
	}
	RpcException(int type) :_type(type)
	{
	}
	RpcException(const std::string& message) :_type(UNKNOWN),_message(message)
	{
	}
	RpcException(int type,const std::string& message) :_type(type),_message(message)
	{
	}

	virtual ~RpcException() throw()
	{
	}

	const int getType() const
	{
		return _type;
	}

	virtual const char* what() const throw()
	{
		if (_message.empty())
		{
			switch (_type)
			{
			case UNKNOWN                 : return "TApplicationException: Unknown application exception";
			case UNKNOWN_METHOD          : return "TApplicationException: Unknown method";
			case INVALID_MESSAGE_TYPE    : return "TApplicationException: Invalid message type";
			case WRONG_METHOD_NAME       : return "TApplicationException: Wrong method name";
			case BAD_SEQUENCE_ID         : return "TApplicationException: Bad sequence identifier";
			case MISSING_RESULT          : return "TApplicationException: Missing result";
			case INTERNAL_ERROR          : return "TApplicationException: Internal error";
			case PROTOCOL_ERROR          : return "TApplicationException: Protocol error";
			case INVALID_TRANSFORM       : return "TApplicationException: Invalid transform";
			case INVALID_PROTOCOL        : return "TApplicationException: Invalid protocol";
			case UNSUPPORTED_CLIENT_TYPE : return "TApplicationException: Unsupported client type";

			case IO_MAYBE_BLOCK          : return "RpcException: io thread maybe block,because rpc.call not in thread/coroutine";
			case IO_BUSY          		 : return "RpcException: io thread busy,when io thread do rpc.call but no time remain";
			case SERVER_UNAVAILABLE    	 : return "RpcException: rpc.call name not found in s2s";
			case CONNECT_TIMEOUT      	 : return "RpcException: rpc.call connect server timeout";
			case CONNECT_FAILED          : return "RpcException: rpc.call connect server failed";
			case TIMEOUT        		 : return "RpcException: rpc.call timeout out";
			case CONNECTION_CLOSED       : return "RpcException: rpc.call connection closed";
			case WORKER_QUEUE_FULL       : return "RpcException: rpc.call worker queue full";

			case APP_ERR_CODE 			 : return "RpcException: app throw exception no reason";

			default                      : return "RpcException: (Invalid exception type)";
			};
		}
		return _message.c_str();
	}
	int 		_type;
	std::string _message;
};

struct AppException : public RpcException
{
	static const Type err_code = APP_ERR_CODE;
	AppException(const std::string& msg):RpcException(err_code, msg)
	{
	}
	virtual ~AppException() {}
};

#endif
