#ifndef __UTIL_ATOMIC_H__
#define __UTIL_ATOMIC_H__

#include "int_types.h"

namespace lin_io
{
#ifdef _MSC_VER
static inline int32_t increment32(volatile int32_t* ptr, int increment)
{
    return InterlockedExchangeAdd(reinterpret_cast<volatile unsigned long*>(ptr), static_cast<unsigned long>(increment)) + increment;
}
//  @return 返回旧值
static inline int32_t compare_and_swap32(volatile int32_t* ptr, int32_t old_value, int32_t new_value)
{
    unsigned long result = InterlockedCompareExchange(reinterpret_cast<volatile unsigned long*>(ptr), static_cast<unsigned long>(new_value), static_cast<unsigned long>(old_value));
    return static_cast<int32_t>(result);
}
static inline void* compare_and_swap_pointer(volatile PVOID* ptr, void* old_value, void* new_value)
{
    return ::InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(ptr), new_value, old_value);
}
//  sample use: spinlock
//  lock: while( exchange32(&used,true)==true ) wait ;
//  unlock: exchange32(&used,false) ;
static inline int32_t exchange32(volatile int32_t* ptr, int32_t new_value)
{
    LONG result = InterlockedExchange(reinterpret_cast<volatile LONG*>(ptr), static_cast<LONG>(new_value));
    return static_cast<int32_t>(result);
}

#ifdef _WIN64
#if (sizeof(int64_t) != sizeof(void*))
#error "not a 64bit platform"
#endif
static inline int64_t increment64(volatile int64_t* ptr, int increment)
{
    return InterlockedExchangeAdd64(reinterpret_cast<volatile LONGLONG*>(ptr), static_cast<LONGLONG>(increment)) + increment;
}
static inline int64_t compare_and_swap64(volatile int64_t* ptr, int64_t old_value, int64_t new_value)
{
    PVOID result = InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(ptr), reinterpret_cast<PVOID>(new_value), reinterpret_cast<PVOID>(old_value));
    return reinterpret_cast<int64_t>(result);
}
//  sample use: spinlock
static inline int64_t exchange64(volatile int64_t* ptr, int64_t new_value)
{
    PVOID result = InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(ptr), reinterpret_cast<PVOID>(new_value));
    return reinterpret_cast<int64_t>(result);
}
#endif //  _WIN64

#if _MSC_VER >= 1400
//  full barrier
static inline void memory_barrier()
{
#if defined(ARCH_CPU_64_BITS)
    // See #undef and note at the top of this file.
    __faststorefence();
#else
    // We use MemoryBarrier from WinNT.h
    ::MemoryBarrier();
#endif
}
#else
#error "We require at least vs2005 for MemoryBarrier"
#endif

#else //  with linux gcc
//成功返回新值
static inline int32_t increment_int32(volatile int32_t* ptr, int increment)
{
    for (;;)
    {
        // Atomic exchange the old value with an incremented one.
        int32_t old_value = *ptr;
        int32_t new_value = old_value + increment;
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        {
            return new_value;
        }
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
    return 0;//加这个词句没意义,只为不告警
}

//成功返回旧值
static inline int32_t compare_and_swap_int32(volatile int32_t* ptr, int32_t old_value, int32_t new_value)
{
    int32_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    }while (prev_value == old_value);//值未被改变
    return prev_value;//值已经改变
}

//只交换一次,成功为true
static inline bool try_compare_and_swap_int32(volatile int32_t* ptr, int32_t old_value, int32_t new_value)
{
	if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
		return true;
	return false;
}

//  sample use: spinlock
//  lock: while( exchange32(&used,true)==true ) wait ;
//  unlock: exchange32(&used,false) ;
//  返回旧值
static inline int32_t exchange_int32(volatile int32_t* ptr, int32_t new_value)
{
    int32_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}
/*********************************************************************/
static inline uint32_t increment_uint32(volatile uint32_t* ptr, int increment)
{
    for (;;)
    {
        // Atomic exchange the old value with an incremented one.
        int32_t old_value = *ptr;
        int32_t new_value = old_value + increment;
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        {
            return new_value;
        }
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
    return 0;//加这个词句没意义,只为不告警
}

static inline int32_t compare_and_swap_uint32(volatile uint32_t* ptr, uint32_t old_value, uint32_t new_value)
{
	uint32_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    }while (prev_value == old_value);
    return prev_value;
}

static inline bool try_compare_and_swap_uint32(volatile uint32_t* ptr, uint32_t old_value, uint32_t new_value)
{
	if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
		return true;
	return false;
}

static inline uint32_t exchange_uint32(volatile uint32_t* ptr, uint32_t new_value)
{
	uint32_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}

static inline void* compare_and_swap_pointer(volatile void** ptr, void* old_value, void* new_value)
{
    void* prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = (void*)(*ptr);
    } while (prev_value == old_value);
    return prev_value;
}

//只交换一次,成功为true
static inline bool try_compare_swap_pointer(volatile void** ptr, void* old_value, void* new_value)
{
	if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
		return true;
	return false;
}

/*********************************************************************/
static inline int64_t increment_int64(volatile int64_t* ptr, int increment)
{
    for (;;)
    {
        // Atomic exchange the old value with an incremented one.
        int64_t old_value = *ptr;
        int64_t new_value = old_value + increment;
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        {
            return new_value;
        }
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
    return 0;//加这个词句没意义,只为不告警
}

static inline int64_t compare_and_swap_int64(volatile int64_t* ptr, int64_t old_value, int64_t new_value)
{
    int64_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    } while (prev_value == old_value);
    return prev_value;
}

//只交换一次,成功为true
static inline bool try_compare_and_swap_int64(volatile int64_t* ptr, int64_t old_value, int64_t new_value)
{
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        return true;
    return false;
}

//  sample use: spinlock
static inline int64_t exchange_int64(volatile int64_t* ptr, int64_t new_value)
{
    int64_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}
/*********************************************************************/
static inline uint64_t increment_uint64(volatile uint64_t* ptr, int increment)
{
    for (;;)
    {
        // Atomic exchange the old value with an incremented one.
        uint64_t old_value = *ptr;
        uint64_t new_value = old_value + increment;
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        {
            return new_value;
        }
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
    return 0;//加这个词句没意义,只为不告警
}

static inline uint64_t compare_and_swap_uint64(volatile uint64_t* ptr, uint64_t old_value, uint64_t new_value)
{
	uint64_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    } while (prev_value == old_value);
    return prev_value;
}


static inline bool try_compare_and_swap_uint64(volatile uint64_t* ptr, uint64_t old_value, uint64_t new_value)
{
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        return true;
    return false;
}

static inline uint64_t exchange_uint64(volatile uint64_t* ptr, uint64_t new_value)
{
	uint64_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}

/*********************************************************************/
static inline uint8_t compare_and_swap_int8(volatile int8_t* ptr, int8_t old_value, int8_t new_value)
{
	int8_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    } while (prev_value == old_value);
    return prev_value;
}


static inline bool try_compare_and_swap_uint8(volatile int8_t* ptr, int8_t old_value, int8_t new_value)
{
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        return true;
    return false;
}

static inline int8_t exchange_uint8(volatile int8_t* ptr, int8_t new_value)
{
	int8_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}

/*********************************************************************/
static inline uint8_t compare_and_swap_uint8(volatile uint8_t* ptr, uint8_t old_value, uint8_t new_value)
{
	uint8_t prev_value;
    do
    {
        if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
            return old_value;
        prev_value = *ptr;
    } while (prev_value == old_value);
    return prev_value;
}


static inline bool try_compare_and_swap_uint8(volatile uint8_t* ptr, uint8_t old_value, uint8_t new_value)
{
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value))
        return true;
    return false;
}

static inline uint8_t exchange_uint8(volatile uint8_t* ptr, uint8_t new_value)
{
	uint8_t old_value;
    do
    {
        old_value = *ptr;
    } while (!__sync_bool_compare_and_swap(ptr, old_value, new_value));
    return old_value;
}

//  full barrier
static inline void memory_barrier()
{
    __sync_synchronize();
}
#endif
}

#endif //  __UTIL_ATOMIC_H__
