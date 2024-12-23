/*
 *	@file		iobuffer.h
 *	@location	nio
 *	@intro		实现一个同时支持读写的buffer
 *				. 能够尾部追加数据
 *			  . 能够头部读取数据、删除数据
 *				. 能够自动扩张(2倍增长)
 *        .   [todo]回收算法可参照tmalloc中Garbage Collection of Thread Caches ?
 *				. [!] iobuffer是不带网络编码的(只提供原始bytes和简单数据io,数据序列化应该由上层支持)
 */

#ifndef __NIO_BYTEBUFFER_H__
#define __NIO_BYTEBUFFER_H__

#include "int_types.h"
#include "varint.h"

#include <assert.h>
#include <stdexcept>
#include <string.h>

namespace lin_io
{
#define xmin(v1, v2) ((v1) < (v2) ? (v1) : (v2))
#define xmax(v1, v2) ((v1) > (v2) ? (v1) : (v2))

class ResourceLimitException : public std::runtime_error
{
public:
	ResourceLimitException(const std::string& _w): std::runtime_error(_w)
	{
	}
	virtual ~ResourceLimitException() {}
};

//	  hole                      space
//	[        ****************             ]
//			     |<---- size--->|
//
//	       data            tail
//		     head			       tail
//
//	 <--------------- capacity ---------->
//
//	usage:
//	1. for write:
//		ByteBuffer bb ;
//		bb.reserve(n) ;  n = io.read(bb.tail(),n) ;  bb.advance(n)	/
//		bb.append(data,n)
//	2. for read:
//		buf = bb.data() ; n = bb.size();
//		process data in [buf,buf+n) ;
//		bb.erase(n)
//  3.  for read / write
//      char * w = bb.reserve(n) ; n = io.read(w,n) ; char * r = bb.advance(n) ;
//      [w,w+n) is current data read from io
class ByteBuffer
{
public:
	// grow2x  内存分配是按2倍增长还是线性增长
	inline ByteBuffer(bool grow2x = true)
	: _cap(1024)
	, _limit(64 * 1024 * 1024)
	, _x2(grow2x)
	, _high_water_mark(0)
	{
		_data = (char*)_stackbuff;
		_head = _data;
		_tail = _data;
	}
	inline virtual ~ByteBuffer()
	{
		if (_data != (char*)_stackbuff)
			delete[] _data;
	}

	inline size_t limit() const { return _limit; }
	inline void limit(size_t n)
	{
		assert(n < 0x40000000); //	2G
		_limit = xmax(n, size());
	}

	//	return buffer capability
	inline size_t capacity() const { return _cap; }

	//	for buffer access
	inline const char& operator[](size_t pos) const
	{
		assert(pos < size());
		return _head[pos];
	}
	inline char& operator[](size_t pos)
	{
		assert(pos < size());
		return _head[pos];
	}

	//	valid data
	inline const char* data() const { return _head; }
	//	valid data size
	inline size_t size() const { return (size_t)(_tail - _head); }
	inline bool empty() const { return 0 == size(); }

	//	返回可写区开始位置
	inline char* tail() const { return _tail; }
	//	返回当前可追加的空间(连续空间)
	inline size_t space() const { return (size_t)(_data + _cap - _tail); }

	//	从头部删除数据,返回实际删除的数据大小
	//				if sz==-1 , clear all
	//	[!] to do : 适当的时候回收空间
	inline size_t erase(size_t sz = -1)
	{
		if (sz == (size_t)-1)
		{
			_head = _data;
			_tail = _head;
		}
		else
		{
			sz = xmin(size(), sz);
			_head += sz;
			//	当没有数据时，head,tail复位(不用搬移内存)
			if (_head >= _tail)
			{ //	empty
				_head = _data;
				_tail = _head;
			}
		}
		return sz;
	}

	//	ensures that size() henceforth returns n
	//	[!] 如果 n>size() , 会保证增加 n-size()的连续空间
	inline void resize(size_t n)
	{
		size_t sz = size();
		if (n > sz)
			reserve(n - sz);
		_tail = _head + n;
		_high_water_mark = xmax(_high_water_mark, size());
	}

	//	保证|n| byte连续可写空间
	inline char* reserve(size_t n)
	{
		if (space() >= n)
			return _tail;
		alloc(n);
		return _tail;
	}
	//	修正当前大小为size()+|n|
	//  返回修正前的tail(读点)
	inline char* advance(size_t n)
	{
		char* p = _tail;
		_tail = xmin(_tail + n, _data + _cap);
		_high_water_mark = xmax(_high_water_mark, size());
		return p;
	}

	inline char* forward(size_t n)
	{
		reserve(n);
		char *p = _tail;
		_tail += n;
		return p;
	}
	inline char* back(size_t n)
	{
		char *p = _tail;
		if (size() >= n)
		{
			_tail -= n;
		}
		return p;
	}
	//	从尾部追加数据,返回实际追加的数据大小
	inline size_t write(const char* dat, size_t sz)
	{
		assert(dat);
		reserve(sz);
		memcpy(_tail, dat, sz);
		_tail += sz;
		_high_water_mark = xmax(_high_water_mark, size());
		return sz;
	}
	inline void write_u8(char v)
	{
		reserve(sizeof(char));
		*((char*)_tail) = v;
		_tail += sizeof(char);
		_high_water_mark = xmax(_high_water_mark, size());
	}
	inline void write_u16(uint16_t v)
	{
		reserve(sizeof(uint16_t));
		*((uint16_t*)_tail) = v;
		_tail += sizeof(uint16_t);
		_high_water_mark = xmax(_high_water_mark, size());
	}
	inline void write_u32(uint32_t v)
	{
		reserve(sizeof(uint32_t));
		*((uint32_t*)_tail) = v;
		_tail += sizeof(uint32_t);
		_high_water_mark = xmax(_high_water_mark, size());
	}
	inline void write_u64(uint64_t v)
	{
		reserve(sizeof(uint64_t));
		*((uint64_t*)_tail) = v;
		_tail += sizeof(uint64_t);
		_high_water_mark = xmax(_high_water_mark, size());
	}

	//	varint read/write, 带zigzag编码
	inline void write_varint32(int32_t value)
	{
		reserve(Varint::max_varint_bytes);
		size_t n = Varint::write_u32(zigzag32(value), (uint8_t*)_tail);
		_tail += n;
		_high_water_mark = xmax(_high_water_mark, size());
	}
	inline void write_varint(int64_t value)
	{
		reserve(Varint::max_varint_bytes);
		size_t n = Varint::write_u64(zigzag(value), (uint8_t*)_tail);
		_tail += n;
		_high_water_mark = xmax(_high_water_mark, size());
	}
	//	如果读出数据，返回true; 否则返回false
	inline bool read_varint(int64_t& value)
	{
		int n = Varint::read((const uint8_t*)_head, size(), value);
		if (n > 0)
		{
			_head += n;
			return true;
		}
		return false;
	}

	//	压缩buffer空间,释放多余的内存
	//	minCapacity [in] 保留最小的容量大小.
	//	(当实际的cap大于minCapacity才尝试压缩)
	inline size_t compact(size_t minCapacity = 4 * 1024)
	{
		if (_cap <= minCapacity)
			return 0;

		//	如果空余空间小于1k,不用整理
		size_t sz = size();
		if (_cap - sz <= 1024)
			return 0;

		size_t newCap = xmax(sz, minCapacity);
		newCap = blocked_size(newCap);

		size_t diff = _cap - newCap;

		char* ptr = new char[newCap];
		memcpy(ptr, _head, sz);
		if (_data != (char*)_stackbuff)
			delete[] _data;

		_data = ptr;
		_head = _data;
		_tail = _head + sz;
		_cap = newCap;

		//降水位
		_high_water_mark = sz;
		return diff;
	}

	//  garbage collect
	//  记录上次gc以来使用大小的最高水位,回收空闲区的一半
	inline size_t gc()
	{
		size_t gcsize = (_cap - _high_water_mark) / 2;
		if ((int)gcsize < 1024)
			return 0;
		return compact(size() + gcsize);
	}

public:
	enum
	{
		BLOCK_SIZE = 4 * 1024, //	4K
		BLOCK_BITS = 12
	};

private:
	ByteBuffer(const ByteBuffer&);
	ByteBuffer& operator=(const ByteBuffer&);

	static inline size_t blocked_size(size_t sz) throw() { return (sz + BLOCK_SIZE - 1) >> BLOCK_BITS << BLOCK_BITS; }
	inline size_t alloc_size(size_t sz)
	{
		if (sz > _limit)
		{
			throw ResourceLimitException("buffer overflow");
		}
		if (_x2)
		{
			size_t n;
			for (n = _cap; n < sz; n <<= 1);
			return blocked_size(n);
		}
		else
		{
			return blocked_size(sz);
		}
	}
	//	返回data前的空洞
	inline size_t hole() const { return (size_t)(_head - _data); }
	//	回收hole:  move [head,tail] => [0,tail-head]
	inline void recycle()
	{
		size_t n = (size_t)(_tail - _head);
		if (n > 0)
			memmove(_data, _head, n);
		_head = _data;
		_tail = _head + n;
	}
	//	分配至少 |n| byte 连续可写空间
	inline void alloc(size_t n)
	{
		//	当前空间足够，整理成连续空间
		if ((size_t)(_head + _cap - _tail) >= n)
		{ //	hole() + space() >= n
			recycle();
			assert(space() >= n);
			return;
		}

		//	reserve new space
		size_t sz = size();
		size_t newCap = alloc_size(sz + n);

		char* ptr = new char[newCap];
		memcpy(ptr, _head, sz);
		if (_data != (char*)_stackbuff)
			delete[] _data;

		_data = ptr;
		_head = _data;
		_tail = _head + sz;
		_cap = newCap;

		assert(space() >= n);
	}

private:
	char _stackbuff[1024];
	char* _data;
	char* _head; //	read pointer
	char* _tail; //	write pointer
	uint32_t _cap; //	capacity(bytes)
	uint32_t _limit; //	max capability(bytes)
	bool _x2; //	内存分配是否按2倍增长(或按线性增长)
	uint32_t  _high_water_mark ;  //  记录上一次回收以后最高水位( max(size) )
};

}

#endif	//	__NIO_BYTEBUFFER_H__

