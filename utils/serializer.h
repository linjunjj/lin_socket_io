/*
 * 通用的序列化工具, 提供通用的二进制数字、字符串等序列化／反序列化操作
 * 1. 定长(8bit,16bit,32bit,64bit)整型，浮点
 * 2. 指定长度(16bit,32bit)标识的字符串和二进制块
 * 3. google protocol buffer中变长数字和字符串
 *


   example:

		struct Obj : public NioMarshallable{
			int		a ;
			string	b ;
			void marshal(Serializer & s) const { 	s << a << b ;		}
			void unmarshal(Deserializer & ds ) { ds >> a >> b ; }
		}

		Obj o ;
		ByteBuffer bb ;
		Serializer s(&bb) ;
		s << o ;

		io.write(bb.data(),bb.size()) ;
		..

		Deserializer s(data,size) ;
		s >> o ;
		..

 */

#ifndef __NIO_COMM_SERIALIZE_H__
#define __NIO_COMM_SERIALIZE_H__

#include <stdexcept>

#include "bytebuffer.h"
#include "byteorder.h"
#include "exception.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <map>
#include <vector>
#include <cstring>


namespace lin_io
{
	enum TMessageType
	{
		T_CALL       = 1,
		T_REPLY      = 2,
		T_EXCEPTION  = 3,
		T_ONEWAY     = 4,
	};

	class Serializer;
	class Deserializer;

	//  interface NioMarshallable
	struct NioMarshallable
	{
		virtual void marshal(Serializer&) const = 0;
		virtual void unmarshal(const Deserializer&) = 0;
		NioMarshallable() {}
		virtual ~NioMarshallable() {}
	};


	typedef ByteBuffer ByteBuffer;

	struct BO {
		inline static uint16_t hton16(uint16_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le16(i) : h2be16(i); }
		inline static uint32_t hton32(uint32_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le32(i) : h2be32(i); }
		inline static uint64_t hton64(uint64_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le64(i) : h2be64(i); }
		inline static uint16_t ntoh16(uint16_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le16(i) : h2be16(i); }
		inline static uint32_t ntoh32(uint32_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le32(i) : h2be32(i); }
		inline static uint64_t ntoh64(uint64_t i, int bo) { return bo == LITTLE_ENDIAN ? h2le64(i) : h2be64(i); }
	};

	//template<typename Encoding >
	class Serializer
	{
	public:

		typedef  char Ch;

		//  byteOrder: LITTLE_ENDIAN / BIG_ENDIAN
		inline Serializer(ByteBuffer* bb = NULL, int byteOrder = LITTLE_ENDIAN)
		: buffer_(bb ? bb : &bb_)
		, bo_(byteOrder)
		{
			offset_ = buffer_->size();
		}
		inline virtual ~Serializer() {}

		inline const char* data() const
		{
			if (buffer_->size() < offset_) {
				assert(false);
				throw RuntimeException("offset out of range");
			}
			return (const char*)(buffer_->data() + offset_);
		}
		inline size_t size() const
		{
			if (buffer_->size() < offset_) {
				assert(false);
				return 0;
			}
			return buffer_->size() - offset_;
		}

		//	raw (这里要判断rj_buf_)
		inline void write(const void* s, size_t n) { buffer_->write((const char*)s, n); }
		inline void write(const std::string& raw) { buffer_->write(raw.data(), raw.size()); }
		//	fixed size integer : byte, fixed16 , fixed32 , fixed64
		inline void write_bool(bool b) { buffer_->write_u8(b ? 1 : 0); }
		inline void write_byte(uint8_t u) { buffer_->write_u8(u); }
		inline void write_int16(int16_t i) { buffer_->write_u16(BO::hton16((uint16_t)i, bo_)); }
		inline void write_int32(int32_t i) { buffer_->write_u32(BO::hton32((uint32_t)i, bo_)); }
		inline void write_int64(int64_t i) { buffer_->write_u64(BO::hton64((uint64_t)i, bo_)); }
		inline void write_uint16(uint16_t u) { buffer_->write_u16(BO::hton16(u, bo_)); }
		inline void write_uint32(uint32_t u) { buffer_->write_u32(BO::hton32(u, bo_)); }
		inline void write_uint64(uint64_t u) { buffer_->write_u64(BO::hton64(u, bo_)); }
		//	real
		inline void write_float(float f) { buffer_->write_u32(BO::hton32(*((uint32_t*)&f), bo_)); }
		inline void write_double(double d) { buffer_->write_u64(BO::hton64(*((uint64_t*)&d), bo_)); }
		//	varint
		inline void write_varint32(int32_t v) { buffer_->write_varint32(v); }
		inline void write_varint(int64_t v) { buffer_->write_varint(v); }

		//	string/bytes:
		//  bytes with a 32 bits integer limited it's length
		inline void write_bytes32(const char* data, size_t size)
		{
			write_uint32(size);
			write(data, size);
		}
		//  string with a 16 bits integer limited it's length
		inline void write_string16(const std::string& str)
		{
			write_uint16(str.size());
			write(str.data(), str.size());
		}
		//  string with a 32 bits integer limited it's length
		inline void write_string32(const std::string& str)
		{
			write_uint32(str.size());
			write(str.data(), str.size());
		}

		//  bytes with a varint limited it's length
		inline void write_bytes(const char* data, size_t size)
		{
			write_varint(size);
			write(data, size);
		}
		//  string with a varint limited it's length
		inline void write_string(const std::string& str) { write_bytes(str.data(), str.size()); }
		//	for replace ( [!] all position based bytebuffer,not this serializer )
		//	@brief  更改指定位置的数据,返回新的位置(原位置+更改数据长度)
		//			用在fixed block
		inline size_t position() const { return size(); }
		inline size_t replace(size_t pos, const void* data, size_t n)
		{
			if (n == 0)
				return 0;
			if (pos + n > size())
				throw RuntimeException("Serializer.replace() : position out of range");
			memcpy(&(*buffer_)[pos], (const uint8_t*)data, n);
			return pos + n;
		}
		inline size_t replace_uint8(size_t pos, uint8_t u8) { return replace(pos, &u8, 1); }
		inline size_t replace_uint16(size_t pos, uint16_t u16)
		{
			u16 = BO::hton16(u16, bo_);
			return replace(pos, &u16, 2);
		}
		inline size_t replace_uint32(size_t pos, uint32_t u32)
		{
			u32 = BO::hton32(u32, bo_);
			return replace(pos, &u32, 4);
		}
		inline size_t replace_uint64(size_t pos, uint64_t u64)
		{
			u64 = BO::hton64(u64, bo_);
			return replace(pos, &u64, 8);
		}

		//rapid json
		inline void Put(Ch c) {
			char *p = buffer_->forward(sizeof(c));
			*(reinterpret_cast<Ch*>(p)) = c;
		}
		inline void PutUnsafe(Ch c) { Put(c); }
		inline void Flush() {}

		inline void Clear() { buffer_->erase(); }
		inline void ShrinkToFit() {
			Ch* c = Push(1);
			*c = '\0';
			Pop(1);
		}
		inline void Reserve(size_t count) { buffer_->reserve(sizeof(Ch)*count); }
		inline Ch* Push(size_t count) {
			size_t sz = sizeof(Ch)*count;
			return reinterpret_cast<Ch*>(buffer_->forward(sz));
		}
		inline Ch* PushUnsafe(size_t count) {
			return Push(count);
		}
		inline void Pop(size_t count) {
			buffer_->back(sizeof(Ch)*count);
		}

		inline void memset(size_t count,char c)
		{
			Push(count);
			std::memset(buffer_->tail(), c, count);
		}
		inline const Ch* GetString() {
			Ch* c = Push(1);
			*c = '\0';
			Pop(1);
			return (reinterpret_cast<const Ch*>(data()));
		}
		size_t GetSize() const { return size(); }

	private:
		Serializer(const Serializer& o);
		Serializer& operator=(const Serializer& o);

	private:
		ByteBuffer bb_;
		ByteBuffer* buffer_;
		size_t offset_;
		const int bo_;
	};

	//	只保存数据的引用，不改变数据
	class Deserializer
	{
	public:
		//  byteOrder: LITTLE_ENDIAN / BIG_ENDIAN
		inline virtual ~Deserializer() {}
		inline Deserializer(const void* data, size_t size, int byteOrder = LITTLE_ENDIAN)
		:data_(0),size_(0), bo_(byteOrder)
		{
			reset(data, size);
		}

		inline const char* data() const { return data_; }
		inline size_t size() const { return size_; }
		inline bool empty() const { return 0 == size_; }

		inline void skip(size_t k) const
		{
			if (size_ < k)
				throw RuntimeException("Deserializer.skip(): not enough data");
			data_ += k;
			size_ -= k;
		}
		//	raw
		inline const char* read(size_t k) const
		{
			if (size_ < k)
				throw RuntimeException("Deserializer.read(): not enough data");
			const char* p = data_;
			data_ += k;
			size_ -= k;
			return p;
		}
		inline std::string read_raw(size_t n) const
		{
			std::string raw;
			const char* p = read(n);
			raw.assign(p, n);
			return raw;
		}

		//	fixed sized integer
		inline uint8_t read_byte() const
		{
			if (size_ < 1u)
				throw RuntimeException("Deserializer.read_byte(): not enough data");
			uint8_t u = *((uint8_t*)data_);
			data_ += 1u;
			size_ -= 1u;
			return u;
		}
		inline bool read_bool() const
		{
			uint8_t u = read_byte();
			return u != 0;
		}
		inline int16_t read_int16() const
		{
			if (size_ < 2u)
				throw RuntimeException("Deserializer.read_int16(): not enough data");
			int16_t u = *((int16_t*)data_);
			u = (int16_t)BO::ntoh16((uint16_t)u, bo_);
			data_ += 2u;
			size_ -= 2u;
			return u;
		}
		inline int32_t read_int32() const
		{
			if (size_ < 4u)
				throw RuntimeException("Deserializer.read_int32(): not enough data");
			int32_t u = *((int32_t*)data_);
			u = (int32_t)BO::ntoh32((uint32_t)u, bo_);
			data_ += 4u;
			size_ -= 4u;
			return u;
		}
		inline int64_t read_int64() const
		{
			if (size_ < 8u)
				throw RuntimeException("Deserializer.read_int64(): not enough data");
			int64_t u = *((int64_t*)data_);
			u = (int64_t)BO::ntoh64((uint64_t)u, bo_);
			data_ += 8u;
			size_ -= 8u;
			return u;
		}
		inline uint16_t read_uint16() const
		{
			if (size_ < 2u)
				throw RuntimeException("Deserializer.read_uint16(): not enough data");
			uint16_t u = *((uint16_t*)data_);
			u = BO::ntoh16(u, bo_);
			data_ += 2u;
			size_ -= 2u;
			return u;
		}
		inline uint32_t read_uint32() const
		{
			if (size_ < 4u)
				throw RuntimeException("Deserializer.read_uint32(): not enough data");
			uint32_t u = *((uint32_t*)data_);
			u = BO::ntoh32(u, bo_);
			data_ += 4u;
			size_ -= 4u;
			return u;
		}
		inline uint64_t read_uint64() const
		{
			if (size_ < 8u)
				throw RuntimeException("Deserializer.read_uint64(): not enough data");
			uint64_t u = *((uint64_t*)data_);
			u = BO::ntoh64(u, bo_);
			data_ += 8u;
			size_ -= 8u;
			return u;
		}
		inline float read_float() const
		{
			if (size_ < 4u)
				throw RuntimeException("Deserializer.read_float(): not enough data");
			uint32_t u32 = *((uint32_t*)data_);
			u32 = BO::ntoh32(u32, bo_);
			float f = *((float*)&u32);
			data_ += 4u;
			size_ -= 4u;
			return f;
		}
		inline double read_double() const
		{
			if (size_ < 8u)
				throw RuntimeException("Deserializer.read_double(): not enough data");
			uint64_t u64 = *((uint64_t*)data_);
			u64 = BO::ntoh64(u64, bo_);
			double d = *((double*)&u64);
			data_ += 8u;
			size_ -= 8u;
			return d;
		}

		//	google protocol bufffer's varint
		inline int64_t read_varint() const
		{
			int64_t i64;
			int n = Varint::read((const uint8_t*)data_, size_, i64);
			if (n <= 0)
				throw RuntimeException("Deserializer.read_varint(): not enough data");
			data_ += n;
			size_ -= n;
			return i64;
		}
		//	string / bytes
		//  bytes with a 32bit integer length-delimited
		inline const char* read_bytes32(size_t& size) const
		{
			size = read_uint32();
			return read(size);
		}
		inline std::string read_bytes32() const
		{
			std::string raw;
			size_t sz;
			const char* p = read_bytes32(sz);
			raw.assign(p, sz);
			return raw;
		}
		//  bytes with a 16bit integer length-delimited
		inline void read_string16(std::string& str) const
		{
			size_t len = read_uint16();
			str.assign((const char*)read(len), len);
		}
		inline std::string read_string16() const
		{
			std::string s;
			read_string16(s);
			return s;
		}
		//  bytes with a 32bit integer length-delimited
		inline void read_string32(std::string& str) const
		{
			size_t len = read_uint32();
			str.assign((const char*)read(len), len);
		}
		inline std::string read_string32() const
		{
			std::string s;
			read_string32(s);
			return s;
		}

		//  protocol buffer's var bytes / string
		//  bytes with a var integer length-delimited
		inline const char* read_bytes(size_t& size) const
		{
			size = read_varint();
			return read(size);
		}
		inline std::string read_bytes() const
		{
			std::string raw;
			size_t sz;
			const char* p = read_bytes(sz);
			raw.assign(p, sz);
			return raw;
		}
		//  string with a var integer length-delimited
		inline void read_string(std::string& str) const
		{
			size_t len = (size_t)read_varint();
			str.assign((const char*)read(len), len);
		}
		inline std::string read_string() const
		{
			std::string s;
			read_string(s);
			return s;
		}

		//	access the forward data , but not pop out
		inline bool peek_uint8(uint8_t& u8) const
		{
			if (size_ < 1u)
				return false;
			u8 = *((uint8_t*)&data_[0]);
			return true;
		}
		inline bool peek_uint8(size_t pos, uint8_t& u8) const
		{
			if (int(size_ - pos) < 1 )
				return false;
			u8 = *((uint8_t*)&data_[pos]);
			return true;
		}

		inline bool peek_uint16(uint16_t& u16) const
		{
			if (size_ < 2u)
				return false;
			u16 = *((uint16_t*)&data_[0]);
			u16 = BO::ntoh16(u16, bo_);
			return true;
		}
		inline bool peek_uint16(size_t pos, uint16_t& u16) const
		{
			if (int(size_ - pos) < 2 )
				return false;
			u16 = *((uint16_t*)&data_[pos]);
			u16 = BO::ntoh16(u16, bo_);
			return true;
		}

		inline bool peek_uint32(uint32_t& u32) const
		{
			if (size_ < 4u)
				return false;
			u32 = *((uint32_t*)&data_[0]);
			u32 = BO::ntoh32(u32, bo_);
			return true;
		}
		inline bool peek_uint32(size_t pos, uint32_t& u32) const {
			if (int(size_ - pos) < 4 )
				return false;
			u32 = *((uint32_t*)&data_[pos]);
			u32 = BO::ntoh32(u32, bo_);
			return true;
		}

		inline bool peek_uint64(uint64_t& u64) const
		{
			if (size_ < 8u)
				return false;
			u64 = *((uint64_t*)&data_[0]);
			u64 = BO::ntoh64(u64, bo_);
			return true;
		}
		inline bool peek_uint64(size_t pos, uint64_t& u64) const
		{
			if (int(size_ - pos) < 8 )
				return false;
			u64 = *((uint64_t*)&data_[pos]);
			u64 = BO::ntoh64(u64, bo_);
			return true;
		}

		inline bool peek_varint(size_t pos, int64_t& i64) const
		{
			int n = Varint::read((const uint8_t*)&data_[0], size_, i64);
			return n > 0;
		}
		inline void reset(const void* data, size_t size) const
		{
			data_ = (const char*)data;
			size_ = size;
		}
	private:
		mutable const char* data_;
		mutable size_t size_;
		const int bo_;
	};

	//  protocol buffer special
	namespace pb
	{
		//  !LITTLE_ENDIAN
		struct Tag {
			//	wire types define
			static const int32_t VARINT = 0; //	1~10 byte    ( int32, int64, uint32, uint64, sint32, sint64, bool, enum )
			static const int32_t FIXED64 = 1; //	8 byte       ( fixed64, sfixed64, double )
			static const int32_t VARBLOCK = 2; //	varint + data( string, bytes, embedded messages, packed repeated fields )
			//  Length-delimited
			static const int32_t GROUPSTART = 3; //  group begin , deprecated
			static const int32_t GROUPEND = 4; //  group end ,  deprecated
			static const int32_t FIXED32 = 5; //	4 byte       ( fixed32, sfixed32, float )

			static const int32_t BLOCK = 6; //	fixed32 + data  ,  self extend , not in goole defines!
			//  Length-delimited  , 不兼容google protocol buffer
			//  目的打包优化

			//	for field tag
			static inline int32_t type(uint32_t tag) { return tag & 0x07; }
			static inline int32_t key(uint32_t tag) { return tag >> 3; }
			static inline int32_t make(int32_t type, int32_t key) { return (key << 3) | (type & 0x07); }
			static inline int32_t sizeoftag(int32_t type, int32_t key)
			{
				if (key < 0x10)
					return 1; //	4 bits  [1,15]
				else if (key < 0x800)
					return 2; //	11 bits [16,2047]
				else
					return Varint::size(make(type, key));
			}
		};

		//	protocol buffer's field tag
		inline Serializer& write_tag(Serializer& pk, uint32_t type, uint32_t index)
		{
			pk.write_varint32(Tag::make(type, index));
			return pk;
		}
		//	protocol buffer's block
		inline Serializer& write_block(Serializer& pk, const void* dat, size_t n, int type = Tag::VARBLOCK)
		{
			if (type == Tag::VARBLOCK) {
				pk.write_varint32(n);
				pk.write(dat, n);
			} else {
				pk.write_uint32(n);
				pk.write(dat, n);
			}
			return pk;
		}
		//  protocol buffer's object
		inline Serializer& write_object(Serializer& pk, const NioMarshallable& object, int type = Tag::VARBLOCK)
		{
			if (type == pb::Tag::VARBLOCK) {
				ByteBuffer bb;
				Serializer pk1(&bb, LITTLE_ENDIAN);
				object.marshal(pk1);
				write_block(pk, pk1.data(), pk1.size(), type);
			} else if (type == pb::Tag::BLOCK) {
				size_t pos = pk.position();
				pk.write_uint32(0);
				object.marshal(pk);
				pk.replace_uint32(pos, pk.position() - 4 - pos);
			}
			return pk;
		}

		//	@brief	read a protocol buffer's field tag
		inline uint32_t read_tag(Deserializer& upk)
		{
			return (uint32_t)upk.read_varint();
		}

		//  skip a protocol buffer's field
		//	@brief	discard current field
		//  @return skiped size
		inline int skip(Deserializer& upk, uint32_t type)
		{
			const int presize = (int)upk.size();
			switch (type) {
			case pb::Tag::VARINT:
				upk.read_varint();
				break;
			case pb::Tag::FIXED32:
				upk.read(4u);
				break;
			case pb::Tag::FIXED64:
				upk.read(8u);
				break;
			case pb::Tag::BLOCK: {
				uint32_t sz = upk.read_uint32();
				upk.read(sz);
				break;
			}
			case pb::Tag::VARBLOCK: {
				uint32_t sz = (uint32_t)upk.read_varint();
				upk.read(sz);
				break;
			}
			default:
				throw RuntimeException("Deserializer.skip(): unrecgnized type");
			}
			return presize - upk.size();
		}

		//	@brief	read a message
		inline Deserializer& read_object(Deserializer& upk, NioMarshallable& obj)
		{
			obj.unmarshal(upk);
			return upk;
		}

		//	@param  type  Tag.BLOCK / Tag.VARBLOCK
		inline Deserializer read_block(Deserializer& upk, int32_t type)
		{
			uint32_t sz;
			if (type == Tag::BLOCK) {
				sz = upk.read_uint32();
			} else if (type == Tag::VARBLOCK) {
				sz = (uint32_t)upk.read_varint();
			} else {
				throw RuntimeException("Deserializer.read_block(): unrecgnized block type");
			}
			const char* ptr = upk.read(sz);
			return Deserializer(ptr, sz, LITTLE_ENDIAN);
		}
	} //  pb
}//easy


//namespace rapidjson
//{
//	//! Implement specialized version of PutN() with memset() for better performance.
//	template<>
//	inline void PutN(lin_io::Serializer& buf, char c, size_t n)
//	{
//		buf.memset(sizeof(c)*n, c);
//	}
//}

//  thrift special
namespace thrift
{
	//  !BIG_ENDIAN
	//  field type
	enum TType
	{
		T_STOP       = 0,
		T_VOID       = 1,
		T_BOOL       = 2,
		T_BYTE       = 3,
		T_I08        = 3,
		T_I16        = 6,
		T_I32        = 8,
		T_U64        = 9,
		T_I64        = 10,
		T_DOUBLE     = 4,
		T_STRING     = 11,
		T_UTF7       = 11,
		T_STRUCT     = 12,
		T_MAP        = 13,
		T_SET        = 14,
		T_LIST       = 15,
		T_UTF8       = 16,
		T_UTF16      = 17
	};

	//  message type
	enum MsgType {
		CALL = 1,
		REPLAY = 2,
		EXCEPTION = 3,
		ONEWAY = 4
	};
	//  message header
	static const uint32_t VERSION_MASK = 0xffff0000;
	static const uint32_t VERSION_1 = 0x80010000;

	//  check and skip the value specialed |ftype|
	//  @return   >=0   skip ok, skipped size
	//            <0    data not enough in (data,size)
	inline int skip(const lin_io::Deserializer& upk, uint32_t ftype)
	{
		size_t size = upk.size();
		switch (ftype) {
		case T_STOP:
		case T_VOID:
			break;
		case T_BOOL:
		case T_BYTE:
			if (upk.size() < 1)
				return -1;
			else
				upk.read(1);
			break;
		case T_I16:
			if (upk.size() < 2)
				return -1;
			else
				upk.read(2);
			break;
		case T_I32:
			if (upk.size() < 4)
				return -1;
			else
				upk.read(4);
			break;
		case T_I64:
		case T_U64:
		case T_DOUBLE:
			if (upk.size() < 8)
				return -1;
			else
				upk.read(8);
			break;
		case T_STRING:
		case T_UTF8:
		case T_UTF16: {
			if (upk.size() < 4)
				return -1;
			uint32_t sz = upk.read_uint32();
			if (upk.size() < sz)
				return -1;
			upk.read(sz);
			break;
		}
		case T_MAP: {
			if (upk.size() < 1 + 1 + 2)
				return -1;
			uint32_t ktype = upk.read_byte();
			uint32_t vtype = upk.read_byte();
			uint32_t num = upk.read_uint32();
			for (uint32_t i = 0; i < num; ++i) {
				if (!skip(upk, ktype))
					return -1;
				if (!skip(upk, vtype))
					return -1;
			}
			break;
		}
		case T_SET:
		case T_LIST: {
			if (upk.size() < 1 + 4)
				return -1;
			uint32_t vtype = upk.read_byte();
			uint32_t num = upk.read_uint32();
			for (uint32_t i = 0; i < num; ++i) {
				if (!skip(upk, vtype))
					return -1;
			}
			break;
		}
		case T_STRUCT: {
			while (true) {
				if (upk.size() < 1)
					return -1;
				uint32_t ftype = upk.read_byte();
				if (ftype == T_STOP)
					break;
				if (upk.size() < 2)
					return -1;
				upk.read(2); //  field id( 2 byte )
				if (!skip(upk, ftype))
					return -1;
			}
			break;
		}
		default:
			throw RuntimeException("unrecgnized thrift field type");
		}
		return (int)(size - upk.size());
	}

	inline uint32_t readFieldBegin(const lin_io::Deserializer& up, TType& fieldType,int16_t& fieldId){
		uint32_t s = up.size();
		int8_t type;
		type = up.read_byte();
		fieldType = (TType)type;
		if (fieldType == T_STOP) {
			fieldId = 0;
			return s-up.size();
		}
		fieldId = up.read_int16();
		return s-up.size();
	}
	inline void readMapBegin(const lin_io::Deserializer& up, TType& keyType,TType& valType,uint32_t& size){
		int8_t k, v;
		k = up.read_byte();
		v = up.read_byte();
		size = up.read_uint32();
		keyType = (TType)k;
		valType = (TType)v;
	}
	inline void readListBegin(const lin_io::Deserializer& up, TType& elemType,uint32_t& size)  {
		int8_t e;
		e  = up.read_byte();
		elemType = (TType)e;
		size = up.read_uint32();
	}
	inline void readSetBegin(const lin_io::Deserializer& up, TType& elemType,uint32_t& size)  {
		int8_t e;
		e  = up.read_byte();
		elemType = (TType)e;
		size = up.read_uint32();
	}

	inline uint32_t readMessageBegin(const lin_io::Deserializer& up,std::string& name,int32_t& messageType,uint32_t& seqid)
	{
		uint32_t s = up.size();
		int sz = up.read_int32();

		// Check for correct version number
		uint32_t version = sz & VERSION_MASK;
		if (version != VERSION_1)
		{
			throw RuntimeException("Bad version identifier");
		}
		messageType = (int32_t)(sz & 0x000000ff);
		up.read_string32(name);
		seqid = up.read_int32();

		return s - up.size();
	}

	inline void writeMessageBegin(lin_io::Serializer& pk,const std::string& name, int32_t messageType,uint32_t seqid)
	{
		int32_t version = (VERSION_1) | ((int32_t)messageType);
		pk.write_int32(version);
		pk.write_string32(name);
		pk.write_int32(seqid);
	}

	inline bool writeFieldBegin(lin_io::Serializer& pk, const TType fieldType, const int16_t fieldId) {
		pk.write_byte((uint8_t)fieldType);
		pk.write_int16(fieldId);
		return true;
	}
	inline bool writeMapBegin(lin_io::Serializer& pk,const TType keyType, const TType valType,const uint32_t size) {
		pk.write_byte((uint8_t)keyType);
		pk.write_byte((uint8_t)valType);
		pk.write_uint32(size);
		return true;
	}
	inline bool writeListBegin(lin_io::Serializer& pk,const TType elemType,const uint32_t size) {
		pk.write_byte((uint8_t)elemType);
		pk.write_uint32(size);
		return true;
	}
	inline bool writeSetBegin(lin_io::Serializer& pk,const TType elemType,const uint32_t size) {
		pk.write_byte((uint8_t)elemType);
		pk.write_uint32(size);
		return true;
	}

	//  @brief   try read a whole message
	//  @return  true     there has a whole message in (data,size)
	//                    and body is (data+headerLength,bodyLength)
	//           false    data not enough
	inline bool read_message_begin(const char* data, size_t size, std::string& fname,
			uint32_t& mtype, uint32_t& sn,
			uint32_t& headerLength, uint32_t& bodyLength)
	{
		lin_io::Deserializer upk(data, size, BIG_ENDIAN);
		if (upk.size() < 4)
			return false;
		uint32_t sz = upk.read_uint32();
		if (sz & 0x80000000) { //	strict , 消息头中带版本号. new version
			mtype = sz & 0xff;
			if (upk.size() < 4)
				return false; //  fname
			sz = upk.read_uint32();
			if (upk.size() < sz)
				return false;
			fname.assign(upk.read(sz), sz);
			if (upk.size() < 4)
				return false;
			sn = upk.read_uint32();
		} else {
			if (upk.size() < 4)
				return false; //  fname
			sz = upk.read_uint32();
			fname.assign(upk.read(sz), sz);
			if (upk.size() < 1 + 4)
				return false; //  mtype,sn
			mtype = upk.read_byte();
			sn = upk.read_uint32();
		}
		headerLength = (uint32_t)(size - upk.size());
		int pos = skip(upk, T_STRUCT);
		if (pos < 0)
			return false;
		bodyLength = (uint32_t)pos;
		return true;
	}

	//  @return  if is STOP field( this struct end )
	inline bool read_field_begin(lin_io::Deserializer& upk, uint32_t& ftype, uint16_t& fid)
	{
		ftype = upk.read_byte();
		if (ftype == T_STOP)
			return true;
		fid = upk.read_uint16();
		return false;
	}

	//  @brief  create a whole message
	inline std::string build_message(const std::string& fname, MsgType mtype,
			uint32_t sn, const lin_io::NioMarshallable& object)
	{
		lin_io::Serializer pk(0, BIG_ENDIAN);
		pk.write_uint32(VERSION_1 | (mtype & 0xff));
		pk.write_string32(fname);
		pk.write_uint32(sn);
		object.marshal(pk);
		std::string raw;
		raw.assign(pk.data(), pk.size());
		return raw;
	}

	//  @brief  create a whole exception message
	inline std::string build_exception_message(const std::string& fname, uint32_t sn,
			const std::string& reason, uint32_t code)
	{
		lin_io::Serializer pk(0, BIG_ENDIAN);
		pk.write_uint32(VERSION_1 | EXCEPTION);
		pk.write_string32(fname);
		pk.write_uint32(sn);
		pk.write_byte(T_STRING);
		pk.write_uint16(1);
		pk.write_string32(reason);
		pk.write_byte(T_I32);
		pk.write_uint16(2);
		pk.write_uint32(code);
		pk.write_byte(T_STOP);
		std::string raw;
		raw.assign(pk.data(), pk.size());
		return raw;
	}
}
#endif // __NIO_COMM_SERIALIZE_H__

