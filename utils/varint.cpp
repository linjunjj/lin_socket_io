#include "varint.h"
#include <stdio.h>

using namespace lin_io;

//	@brief	����|value|��varint�洢��С(bytes)
int Varint::varint_size32(uint32_t value)
{
	if (value < (1 << 7)) {
		return 1;
	} else if (value < (1 << 14)) {
		return 2;
	} else if (value < (1 << 21)) {
		return 3;
	} else if (value < (1 << 28)) {
		return 4;
	} else {
		return 5;
	}
}

//	@brief	����|value|��varint�洢��С(bytes)
int Varint::varint_size64(uint64_t value)
{
	if (value < (1ull << 35)) {
		if (value < (1ull << 7)) {
			return 1;
		} else if (value < (1ull << 14)) {
			return 2;
		} else if (value < (1ull << 21)) {
			return 3;
		} else if (value < (1ull << 28)) {
			return 4;
		} else {
			return 5;
		}
	} else {
		if (value < (1ull << 42)) {
			return 6;
		} else if (value < (1ull << 49)) {
			return 7;
		} else if (value < (1ull << 56)) {
			return 8;
		} else if (value < (1ull << 63)) {
			return 9;
		} else {
			return 10;
		}
	}
}

int Varint::write_u32(uint32_t value,uint8_t * target)
{
	target[0] = static_cast<uint8_t>(value | 0x80);
	if (value >= (1 << 7)) {
		target[1] = static_cast<uint8_t>((value >>  7) | 0x80);
		if (value >= (1 << 14)) {
			target[2] = static_cast<uint8_t>((value >> 14) | 0x80);
			if (value >= (1 << 21)) {
				target[3] = static_cast<uint8_t>((value >> 21) | 0x80);
				if (value >= (1 << 28)) {
					target[4] = static_cast<uint8_t>(value >> 28);
					return 5;
				} else {
					target[3] &= 0x7F;
					return 4;
				}
			} else {
				target[2] &= 0x7F;
				return 3;
			}
		} else {
			target[1] &= 0x7F;
			return 2;
		}
	} else {
		target[0] &= 0x7F;
		return 1;
	}
}

int Varint::write_u64(uint64_t value, uint8_t * target)
{
	// Splitting into 32-bit pieces gives better performance on 32-bit
	// processors.
	uint32_t part0 = static_cast<uint32_t>(value      );
	uint32_t part1 = static_cast<uint32_t>(value >> 28);
	uint32_t part2 = static_cast<uint32_t>(value >> 56);

	int size;

	// Here we can't really optimize for small numbers, since the value is
	// split into three parts.  Cheking for numbers < 128, for instance,
	// would require three comparisons, since you'd have to make sure part1
	// and part2 are zero.  However, if the caller is using 64-bit integers,
	// it is likely that they expect the numbers to often be very large, so
	// we probably don't want to optimize for small numbers anyway.  Thus,
	// we end up with a hardcoded binary search tree...
	if (part2 == 0) {
		if (part1 == 0) {
			if (part0 < (1 << 14)) {
				if (part0 < (1 << 7)) {
					size = 1; goto size1;
				} else {
					size = 2; goto size2;
				}
			} else {
				if (part0 < (1 << 21)) {
					size = 3; goto size3;
				} else {
					size = 4; goto size4;
				}
			}
		} else {
			if (part1 < (1 << 14)) {
				if (part1 < (1 << 7)) {
					size = 5; goto size5;
				} else {
					size = 6; goto size6;
				}
			} else {
				if (part1 < (1 << 21)) {
					size = 7; goto size7;
				} else {
					size = 8; goto size8;
				}
			}
		}
	} else {
		if (part2 < (1 << 7)) {
			size = 9; goto size9;
		} else {
			size = 10; goto size10;
		}
	}

	size10: target[9] = static_cast<uint8_t>((part2 >>  7) | 0x80);
	size9 : target[8] = static_cast<uint8_t>((part2      ) | 0x80);
	size8 : target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
	size7 : target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
	size6 : target[5] = static_cast<uint8_t>((part1 >>  7) | 0x80);
	size5 : target[4] = static_cast<uint8_t>((part1      ) | 0x80);
	size4 : target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
	size3 : target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
	size2 : target[1] = static_cast<uint8_t>((part0 >>  7) | 0x80);
	size1 : target[0] = static_cast<uint8_t>((part0      ) | 0x80);

	target[size-1] &= 0x7F;
	return size;
}


int Varint::read_u32(const uint8_t * buffer,uint32_t & value)
{
	const uint8_t * ptr = buffer;
	uint32_t b;
	uint32_t result;

	b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |=  b         << 28; if (!(b & 0x80)) goto done;

	// If the input is larger than 32 bits, we still need to read it all
	// and discard the high-order bits.
	for (uint32_t i = 0; i < max_varint_bytes - max_varint32_bytes; ++i) {
		b = *(ptr++); if (!(b & 0x80)) goto done;
	}

	// We have overrun the maximum size of a varint (10 bytes).  Assume
	// the data is corrupt.
	return 0 ;

	done:
	value = result;
	return (int)(ptr-buffer);
}

int Varint::read_u64(const uint8_t * buffer,uint64_t & value)
{
	const uint8_t * ptr = buffer;
	uint32_t b;

	// Splitting into 32-bit pieces gives better performance on 32-bit processors.
	uint32_t part0 = 0, part1 = 0, part2 = 0;

	b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
	b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
	b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
	b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
	b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
	b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
	b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
	b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
	b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
	b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;

	// We have overrun the maximum size of a varint (10 bytes).  The data must be corrupt.
	return 0;

	done:
	value = (static_cast<uint64_t>(part0)      ) |
	 	    (static_cast<uint64_t>(part1) << 28) |
			(static_cast<uint64_t>(part2) << 56);
	return (int)(ptr - buffer) ;
}

//	@brief	read a varint from |bytes|
//	@return bytes for this varint
//			if failed , return 0
int	Varint::read(const uint8_t * bytes,size_t size,int64_t & value)
{
	uint64_t u64 ;
	if( size>0 ){
		if( *bytes < 0x80 ){
			u64 = * bytes ;
			value = zagzig(u64) ;
			return 1 ;
		}else{
			if( size >= Varint::max_varint_bytes || !(bytes[size-1] & 0x80) ){
				int n = Varint::read_u64(bytes,u64) ;
				if( n>0 ) value = zagzig(u64) ;
				return n ;
			}else{	//	slow mode
				u64 = 0;
				const uint8_t * ptr = bytes ;
				const uint8_t * tail = ptr + size ;
				for(uint32_t i=0,m7=0 ; i<Varint::max_varint_bytes && ptr<tail ;++i,++ptr,m7+=7){
					uint64_t u8 = *ptr ;
					u64 |= ( u8 & 0x7f ) << m7 ;
					if( !( u8 & 0x80 ) ){
						value = zagzig(u64) ;
						return i+1 ;
					}
				}
				return 0 ;
			}
		}
	}else{
		return 0 ;
	}
}


/////////////////////////////////////////////////////////////////////////////////////
//	for test

#ifdef	__TEST__

namespace test_varint{		

void testZigzag(int64_t v){	
	int64_t v1 = anka::zigzag(v) ;
	int64_t v2 = anka::zagzig(v1) ;
	dwAssert(v2==v) ;
	printf("%"PRId64" => %"PRIu64" => %"PRId64" \n",v,v1,v2) ;
}

void testSerialize(uint8_t * bytes,int64_t v){
	int n = anka::Varint::write(v,bytes) ;
	int64_t v1 ;
	int n1 = anka::Varint::read(bytes,10,v1) ;

	dwAssert(n==n1) ;
	dwAssert(v1==v) ;
	dwAssert(n==anka::Varint::size(v)) ;

	printf("[%d] %I64d = %I64d \n",n,v,v1) ;
}

void test(){
	//	test zigzag
	printf("test zigzag:\n") ;
	for(int32_t i=0;i<64;++i){
		testZigzag( (1LL<<i)-1 ) ;
		testZigzag( -( (1LL<<i)-1 ) ) ;
		
		testZigzag( 1LL<<i ) ;
		testZigzag( -( 1LL<<i) ) ;
		
		if( i!=63 ){
			testZigzag(  (1LL<<i) + 1 ) ;
			testZigzag( -( (1LL<<i) + 1) ) ;
		}
	}
	printf("test serialize:\n") ;
	
	uint8_t bytes[10] ;
	for(int32_t i=0;i<64;++i){
		printf("2 ^ %d\n",i ) ;
		testSerialize(bytes, (1LL<<i)-1 ) ;
		testSerialize(bytes, -( (1LL<<i)-1 ) ) ;
		
		testSerialize(bytes, 1LL<<i ) ;
		testSerialize(bytes, -( 1LL<<i) ) ;
		
		if( i!=63 ){
			testSerialize(bytes,  (1LL<<i) + 1 ) ;
			testSerialize(bytes, -( (1LL<<i) + 1) ) ;
		}
	}
}

}
#endif	//	__TEST__
