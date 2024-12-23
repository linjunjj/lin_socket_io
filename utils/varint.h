#ifndef	__NIO_VARINT_H__
#define	__NIO_VARINT_H__

/*
 *  变长integer. 存储方式: 
 *  1. 每个字节使用0~7 bit,bit8作为是否还有后继数据的判断
 *  2. 低字节在前，高字节在后
 *  3. 采用zigzag编码来压缩负数
 *  最大10byte. 值域范围为(-2^63,2^63) 
 *  * 参见google protocol buffer
 *  
 */

#include "int_types.h"

#include <string>


//////////////////////////////////////////////////////////////////////////////////

namespace lin_io
{

	//	ZigZags编码: 把 [-N,N]的数字映射到[0,2N]的空间
	//  int32 		->     uint32_t
	// -------------------------
	//	          0 ->          0
	//	         -1 ->          1
	//	          1 ->          2
	//	         -2 ->          3
	//	        ... ->        ...
	//  2147483647  -> 4294967294
	// -2147483648  -> 4294967295
	//
	//	        >> encode >>
	//	        << decode <<
	//	最大表示范围(-2^63,2^63)

	static inline uint32_t zigzag32(int32_t i){	return (i<<1) ^ (i>>31) ;	}
	static inline int32_t zagzig32(uint32_t i){	return (i>>1) ^ -static_cast<int32_t>(i&1) ; }
	static inline uint64_t zigzag(int64_t i){ return (i<<1) ^ (i>>63) ; }
	static inline int64_t zagzig(uint64_t i){ return (i>>1) ^ -static_cast<int64_t>(i&1) ; }

	//	变长存储数字: 每字节用7bit，最高bit(bit8)标识是否还有后继数据
	struct	Varint{
		static const uint32_t max_varint_bytes = 10 ;	//	最大能存储64 bits整数
		static const uint32_t max_varint32_bytes = 5 ;	//	最大能存储32 bits整数

		//	@brief	计算|value|的varint存储大小(bytes)
		static int varint_size32(uint32_t value) ;
		//	@brief	计算|value|的varint存储大小(bytes)
		static int varint_size64(uint64_t value) ;

		//	@brief	写入varint数据到|target|
		//			[!] target必须保证有足够空间(至少10 byte)
		//	@return 返回实际写入的字节数
		static int write_u32(uint32_t value,uint8_t * target) ;
		static int write_u64(uint64_t value, uint8_t * target) ;

		//	@brief	从|buffer|读取一个varint数据至|value|
		//	[!] buffer空间始终是足够的
		//	@return 实际读出的字节数. 如果读取失败(|buffer|当前数据并不是一个varint),返回0
		static int read_u32(const uint8_t * buffer,uint32_t & value) ;
		static int read_u64(const uint8_t * buffer,uint64_t & value) ;

		//	计算|v|的varint长度(encoded)
		static inline int size(int64_t value){	return varint_size64( zigzag(value) ) ;	}

		//	@brief	写入一个varint数字|v|到|bytes|中, 带zigzag编码
		//			必须保证bytes空间足够!
		//	@return 返回实际写入的字节数
		static inline int write(int64_t v,uint8_t * bytes){		return write_u64(zigzag(v),bytes) ;		}

		//	@brief	从|bytes|中读出一个varint数字到|value|, 带zigzag编码
		//	@return 返回读取的字节数. 如果读取失败(数据不够或当前数据不是varint)，返回0
		static int	read(const uint8_t * bytes,size_t size,int64_t & value) ;

		//	判断buffer中是否有一个varint
		//	@return 如果存在，返回该varint的byte数; 否则返回0
		static inline int	has(uint8_t * bytes,size_t size) {
			uint32_t n = static_cast<uint32_t>( size<max_varint_bytes ? size : max_varint_bytes ) ;
			for(uint32_t i=0;i<n;++i){
				if( !( bytes[i] & 0x80 ) ) return i+1 ;
			}
			return 0 ;
		}
	} ;
}	//	nio

#ifdef	__TEST__

/**
 *	for test
 */
namespace test_varint{		
	extern void testZigzag(int64_t v) ;
	extern void testSerialize(uint8_t * bytes,int64_t v) ;
	extern void test() ;
}

#endif	//	__TEST__


#endif	//	__NIO_VARINT_H__
