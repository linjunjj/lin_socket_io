#ifndef __UTIL_BYTEORDER_H__
#define __UTIL_BYTEORDER_H__

/**
 * for byte order 
 * export: BYTE_ORDER , BIG_ENDIAN , LITTLE_ENDIAN
 * h2be16(x),h2be32(x),h2be64(x)
 * h2le16(x),h2le16(x),h2le16(x)
 */

//#if defined (__GLIBC__)
// GNU libc offers the helpful header <endian.h> which defines __BYTE_ORDER
#if defined(linux) || defined(__linux) || defined(__linux__)
#include <endian.h>
#elif defined(__sparc) || defined(__sparc__)   \
    || defined(_POWER) || defined(__powerpc__) \
    || defined(__ppc__) || defined(__hpux)     \
    || defined(_MIPSEB) || defined(_POWER)     \
    || defined(__s390__)
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define PDP_ENDIAN 3412
#define BYTE_ORDER BIG_ENDIAN
#elif defined(__i386__) || defined(__alpha__)   \
    || defined(__ia64) || defined(__ia64__)     \
    || defined(_M_IX86) || defined(_M_IA64)     \
    || defined(_M_ALPHA) || defined(__amd64)    \
    || defined(__amd64__) || defined(_M_AMD64)  \
    || defined(__x86_64) || defined(__x86_64__) \
    || defined(_M_X64)
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#define PDP_ENDIAN 3412
#define BYTE_ORDER LITTLE_ENDIAN
#else
#error The file platform.h needs to be set up for your CPU type.
#endif

#ifndef __bswap_16
#define __bswap_16(x) ((((x) >> 8) & 0xff) | (((x)&0xff) << 8))
#endif

#ifndef __bswap_32
#define __bswap_32(x) \
    ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))
#endif

#ifndef __bswap_64
#define __bswap_64(x)                         \
    ((((x)&0xff00000000000000ull) >> 56)      \
        | (((x)&0x00ff000000000000ull) >> 40) \
        | (((x)&0x0000ff0000000000ull) >> 24) \
        | (((x)&0x000000ff00000000ull) >> 8)  \
        | (((x)&0x00000000ff000000ull) << 8)  \
        | (((x)&0x0000000000ff0000ull) << 24) \
        | (((x)&0x000000000000ff00ull) << 40) \
        | (((x)&0x00000000000000ffull) << 56))
#endif

#ifndef LITTLE_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN
#endif
#ifndef BYTE_ORDER
#define __BYTE_ORDER BYTE_ORDER
#endif

#if BYTE_ORDER == LITTLE_ENDIAN

// host => big endian
#define h2be16(x) __bswap_16(x)
#define h2be32(x) __bswap_32(x)
#define h2be64(x) __bswap_64(x)

// host => little endian
#define h2le16(x) (x)
#define h2le32(x) (x)
#define h2le64(x) (x)

#else // BIG_ENDIAN

// host => big endian
#define h2be16(x) (x)
#define h2be32(x) (x)
#define h2be64(x) (x)

// host => little endian
#define h2le16(x) __bswap_16(x)
#define h2le32(x) __bswap_32(x)
#define h2le64(x) __bswap_64(x)

#endif

// host => special byte order
#define h2bo16(bo, x) (BIG_ENDIAN == (bo) ? h2be16(x) : h2le16(x))
#define h2bo32(bo, x) (BIG_ENDIAN == (bo) ? h2be32(x) : h2le32(x))
#define h2bo64(bo, x) (BIG_ENDIAN == (bo) ? h2be64(x) : h2le64(x))

// @brief   return if byte order of current host is big endian
inline bool isBigEndian() { return BYTE_ORDER == BIG_ENDIAN; }

#endif //   __UTIL_BYTEORDER_H__