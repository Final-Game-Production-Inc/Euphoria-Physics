// 
// system/interlocked.inl 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#if __PS3
#include <cell/atomic.h>
#elif __PSP2 || RSG_ORBIS
#include <sce_atomic.h>
#endif

#if __WIN32
extern "C"
{
// Avoid conflict with _config.h from stlport
#ifdef _STLP_IMPORT_DECLSPEC
_STLP_IMPORT_DECLSPEC long _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comperand);
#else 
long _InterlockedCompareExchange(long volatile *Destination, long Exchange, long Comperand);
#endif

// Functions declaration to not include windows.h
long _InterlockedExchange(long volatile*, long);
long _InterlockedIncrement(long volatile*);
long _InterlockedDecrement(long volatile*);
long _InterlockedExchangeAdd(long volatile*, long);
long _InterlockedOr(long volatile*, long);
long _InterlockedAnd(long volatile*, long);
long long _InterlockedExchange64(long long volatile*, long long);
long long _InterlockedOr64(long long volatile*, long long);
long long _InterlockedCompareExchange64(long long volatile*, long long, long long);
long long _InterlockedExchangeAdd64(long long volatile*, long long);
}
#endif

namespace rage
{

#if __WIN32

// The interlocked instructions seem to reboot the 360 if you call them with a bad pointer
// so this function is used to perform a normal read to validate the pointer first
template<class T> inline void AssertReadable(volatile const T* ASSERT_ONLY(destination)) { ASSERT_ONLY(*destination); }

CompileTimeAssert(sizeof(unsigned) == sizeof(long));

inline u32 sysInterlockedExchange(volatile u32* destination, u32 exchange)
{
	AssertReadable(destination);
    return (u32)_InterlockedExchange((volatile long*)destination, (long)exchange);
}

inline u64 sysInterlockedExchange(volatile u64* destination, u64 exchange)
{
	AssertReadable(destination);
	return (u64)_InterlockedExchange64((volatile long long*)destination, (long long)exchange);
}

inline u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand)
{
	AssertReadable(destination);
    return (u32)_InterlockedCompareExchange((volatile long*)destination, (long)exchange, (long)comparand);
}

inline u64 sysInterlockedCompareExchange(volatile u64 *destination, u64 exchange, u64 comparand)
{
	AssertReadable(destination);
	return (u64)_InterlockedCompareExchange64((volatile long long*)destination, (long long)exchange, (long long)comparand);
}

inline void* sysInterlockedExchangePointer(void* volatile* destination, void* exchange)
{
	AssertReadable(destination);
#if __64BIT
    return (void*)_InterlockedExchange64((long long volatile*)destination, (long long)exchange);
#else
	return (void*)_InterlockedExchange((long volatile*)destination, (long)exchange);
#endif
}

inline void* sysInterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand)
{
	AssertReadable(destination);
#if __64BIT
    return (void*)_InterlockedCompareExchange64((long long volatile*)(destination), (long long)exchange, (long long)comparand);
#else
	return (void*)_InterlockedCompareExchange((long volatile*)destination, (long)exchange, (long)comparand);
#endif
}

inline u32 sysInterlockedIncrement(volatile u32* destination)
{
	AssertReadable(destination);
    return (u32)_InterlockedIncrement((volatile long*)destination);
}

inline u32 sysInterlockedDecrement(volatile u32* destination)
{
	AssertReadable(destination);
    return (u32)_InterlockedDecrement((volatile long*)destination);
}

inline u32 sysInterlockedAdd(volatile u32* destination, s32 value) 
{
	AssertReadable(destination);
	return (u32)_InterlockedExchangeAdd((volatile long*)destination, (long)value);
}

inline u64 sysInterlockedAdd(volatile u64* destination, s64 value) 
{
	AssertReadable(destination);
	return (u64)_InterlockedExchangeAdd64((volatile long long*)destination, (long long)value);
}

inline void* sysInterlockedAddPointer(volatile void** destination, ptrdiff_t value)
{
	AssertReadable(destination);
#if __64BIT
	return (void*)sysInterlockedAdd((volatile unsigned long long *)destination, value);
#else
	return (void*)sysInterlockedAdd((volatile unsigned *)destination, value);
#endif
}

inline u32 sysInterlockedOr(volatile u32* destination, u32 value) 
{
	AssertReadable(destination);
	return (u32)_InterlockedOr((volatile long*)destination, (long)value);
}

inline u64 sysInterlockedOr(volatile u64* destination, u64 value) 
{
	AssertReadable(destination);
	return (u64)_InterlockedOr64((volatile __int64*)destination, (__int64)value);
}

inline u32 sysInterlockedAnd(volatile u32* destination, u32 value) 
{
	AssertReadable(destination);
	return (u32)_InterlockedAnd((volatile long*)destination, (long)value);
}

inline u32 sysInterlockedRead(const volatile u32* destination) 
{
	return *destination;
}

inline u64 sysInterlockedRead(const volatile u64* destination) 
{
	return *destination;
}

inline void* sysInterlockedReadPointer(void* const volatile* target)
{
	return *target;
}

#elif __PPU

CompileTimeAssert(sizeof(unsigned) == sizeof(uint32_t));
CompileTimeAssert(sizeof(void*) == sizeof(uint32_t));

inline u32 sysInterlockedExchange(volatile u32* destination, u32 exchange)
{
    return cellAtomicStore32((uint32_t*)destination, exchange);
}

inline u64 sysInterlockedExchange(volatile u64* destination, u64 exchange)
{
	return cellAtomicStore64((uint64_t*)destination, exchange);
}

inline u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand)
{
    return cellAtomicCompareAndSwap32((uint32_t*)destination, comparand, exchange);
}

inline u64 sysInterlockedCompareExchange(volatile u64* destination, u64 exchange, u64 comparand)
{
	return cellAtomicCompareAndSwap64((uint64_t*)destination, comparand, exchange);
}

inline void* sysInterlockedExchangePointer(void* volatile* destination, void* exchange)
{
    return (void*)cellAtomicStore32((uint32_t*)destination, (uint32_t)exchange);
}

inline void* sysInterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand)
{
    return (void*)cellAtomicCompareAndSwap32((uint32_t*)destination, (uint32_t)comparand, (uint32_t)exchange);
}

inline u32 sysInterlockedIncrement(volatile u32* destination)
{
    return cellAtomicIncr32((uint32_t*)destination) + 1;
}

inline u32 sysInterlockedDecrement(volatile u32* destination)
{
    return cellAtomicDecr32((uint32_t*)destination) - 1;
}

inline u32 sysInterlockedAdd(volatile u32* destination, s32 value) 
{
	return cellAtomicAdd32((uint32_t*)destination, value);
}

inline u64 sysInterlockedAdd(volatile u64* destination, s64 value) 
{
	return cellAtomicAdd64((uint64_t*)destination, value);
}

inline u32 sysInterlockedOr(volatile u32* destination, u32 value) 
{
	return cellAtomicOr32((uint32_t*)destination, value);
}

inline u64 sysInterlockedOr(volatile u64* destination, u64 value) 
{
	return cellAtomicOr64((uint64_t*)destination, value);
}

inline u32 sysInterlockedAnd(volatile u32* destination, u32 value) 
{
	return cellAtomicAnd32((uint32_t*)destination, value);
}

inline u32 sysInterlockedRead(const volatile u32* destination) 
{
	return cellAtomicNop32((uint32_t*)destination);
}

inline u64 sysInterlockedRead(const volatile u64* destination) 
{
	return cellAtomicNop64((uint64_t*)destination);
}

inline void* sysInterlockedReadPointer(void* const volatile* target)
{
	return (void*)sysInterlockedRead((uint32_t*)target);
}

#elif __PSP2 || RSG_ORBIS

inline u32 sysInterlockedAdd(volatile u32* destination, s32 value)
{
	return sceAtomicAdd32((volatile int32_t*)destination, value);
}

inline u64 sysInterlockedAdd(volatile u64* destination, s64 value)
{
	return sceAtomicAdd64((volatile int64_t*)destination, value);
}

inline void* sysInterlockedAddPointer(volatile void ** destination, ptrdiff_t value) {
	return (void*) sysInterlockedAdd((uint64_t*)destination, value);
}

inline u32 sysInterlockedIncrement(volatile u32* destination)
{
	return sceAtomicIncrement32((volatile int32_t*)destination) + 1;
}

inline u32 sysInterlockedDecrement(volatile u32* destination)
{
	return sceAtomicDecrement32((volatile int32_t*)destination) - 1;
}

inline u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand)
{
	return sceAtomicCompareAndSwap32((int32_t*)destination, comparand, exchange);
}

inline u64 sysInterlockedCompareExchange(volatile u64* destination, u64 exchange, u64 comparand)
{
	return sceAtomicCompareAndSwap64((int64_t*)destination, comparand, exchange);
}

inline u32 sysInterlockedExchange(volatile u32* destination, u32 exchange)
{   
	return sceAtomicExchange32((int32_t*)destination,(int32_t)exchange);
}

inline void *sysInterlockedExchangePointer(void* volatile * destination, void* exchange)
{   
#if __64BIT
	return (void*)sceAtomicExchange64((int64_t*)destination,(int64_t)exchange);
#else
	return (void*)sceAtomicExchange32((int32_t*)destination, (int32_t)exchange);
#endif
}

inline u32 sysInterlockedOr(volatile u32* destination, u32 value) 
{
	return sceAtomicOr32((int32_t*)destination, value);
}

inline u64 sysInterlockedOr(volatile u64* destination, u64 value) 
{
	return sceAtomicOr64((int64_t*)destination, (int64_t) value);
}

inline u32 sysInterlockedAnd(volatile u32* destination, u32 value) 
{
	return sceAtomicAnd32((int32_t*)destination, value);
}

inline u32 sysInterlockedRead(const volatile u32* destination) 
{
	return *destination;
}

inline u64 sysInterlockedRead(const volatile u64* destination) 
{
	return *destination;
}

inline void* sysInterlockedReadPointer(void* const volatile* target)
{
	return *target;
}

inline void* sysInterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand)
{
	return (void*)sceAtomicCompareAndSwap64((volatile int64_t*)(destination), (int64_t)comparand, (int64_t)exchange);
}

#elif __SPU

CompileTimeAssert(sizeof(unsigned) == sizeof(uint32_t));
CompileTimeAssert(sizeof(void*) == sizeof(uint32_t));

#if __ASSERT
#define TRed		"\033[31m"  // Red Text
#define TNorm		"\033[0m"	// Normal Text
inline bool sysValidateInterlocked(const char* file, u32 line, u32 ppu)
{
    if (ppu < 256*1024) 
    {
        Displayf(TRed"HALT: SPU sysInterlocked Assertion failed %s(%d) : Invalid PPU address 0x%x"TNorm, file, line, ppu);
        return false;
    }
    return true;
}

#define sysInterlockedAssert(destination) (sysValidateInterlocked(__FILE__,__LINE__,(u32)destination)||(__debugbreak(),0))
#else
#define sysInterlockedAssert(destination)
#endif

inline u32 sysInterlockedExchange(volatile u32* destination, u32 exchange)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return cellAtomicStore32(buf, (uint64_t)destination, exchange);
}

inline u64 sysInterlockedExchange(volatile u64* destination, u64 exchange)
{
	uint64_t buf[16] __attribute__((aligned(128)));
	sysInterlockedAssert(destination);
	return cellAtomicStore64(buf, (uint64_t)destination, exchange);
}

inline u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return cellAtomicCompareAndSwap32(buf, (uint64_t)destination, comparand, exchange);
}

inline u64 sysInterlockedCompareExchange(volatile u64* destination, u64 exchange, u64 comparand)
{
	uint64_t buf[16] __attribute__((aligned(128)));
	sysInterlockedAssert(destination);
    return cellAtomicCompareAndSwap64(buf, (uint64_t)destination, comparand, exchange);
}

inline void* sysInterlockedExchangePointer(void* volatile* destination, void* exchange)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return (void*)cellAtomicStore32(buf, (uint64_t)destination, (uint32_t)exchange);
}

inline void* sysInterlockedCompareExchangePointer(void* volatile* destination, void* exchange, void* comparand)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return (void*)cellAtomicCompareAndSwap32(buf, (uint64_t)destination, (uint32_t)comparand, (uint32_t)exchange);
}

inline u32 sysInterlockedIncrement(volatile u32* destination)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return cellAtomicIncr32(buf, (uint64_t)destination) + 1;
}

inline u32 sysInterlockedDecrement(volatile u32* destination)
{
    uint32_t buf[32] __attribute__((aligned(128)));
    sysInterlockedAssert(destination);
    return cellAtomicDecr32(buf, (uint64_t)destination) - 1;
}

template<class T>
inline T sysInterlockedIncrement_Imp(volatile T* destination)
{
    sysInterlockedAssert(destination);
    T result;
    u32 ea = (u32)destination;
    u32 offs = ea&127;
    ea &= ~127;
    do
    {
        u8 ls[128] ALIGNED(128);
		spu_writech(MFC_LSA, (u32)ls);
		spu_writech(MFC_EAL, ea);
		spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
		spu_readch(MFC_RdAtomicStat);
        result = ++*(T*)&ls[offs];
		spu_writech(MFC_LSA, (u32)ls);
		spu_writech(MFC_EAL, ea);
		spu_writech(MFC_Cmd, MFC_PUTLLC_CMD);
    }
    while (Unlikely(spu_readch(MFC_RdAtomicStat)));
    return result;
}

inline u16 sysInterlockedIncrement(volatile u16* destination)
{
	return sysInterlockedIncrement_Imp(destination);
}

inline u8 sysInterlockedIncrement(volatile u8* destination)
{
	return sysInterlockedIncrement_Imp(destination);
}

template<class T>
inline T sysInterlockedDecrement_Imp(volatile T* destination)
{
    sysInterlockedAssert(destination);
    T result;
    u32 ea = (u32)destination;
    u32 offs = ea&127;
    ea &= ~127;
    do
    {
        u8 ls[128] ALIGNED(128);
		spu_writech(MFC_LSA, (u32)ls);
		spu_writech(MFC_EAL, ea);
		spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
		spu_readch(MFC_RdAtomicStat);
        result = --*(T*)&ls[offs];
		spu_writech(MFC_LSA, (u32)ls);
		spu_writech(MFC_EAL, ea);
		spu_writech(MFC_Cmd, MFC_PUTLLC_CMD);
    }
    while (Unlikely(spu_readch(MFC_RdAtomicStat)));
    return result;
}

inline u16 sysInterlockedDecrement(volatile u16* destination)
{
	return sysInterlockedDecrement_Imp(destination);
}

inline u8 sysInterlockedDecrement(volatile u8* destination)
{
	return sysInterlockedDecrement_Imp(destination);
}

inline u16 sysInterlockedIncrement_NoWrapping(volatile u16* destination)
{
	const u16 ret = sysInterlockedIncrement(destination);
	Assert(ret);
	return ret;
}

inline u8 sysInterlockedIncrement_NoWrapping(volatile u8* destination)
{
	const u8 ret = sysInterlockedIncrement(destination);
	Assert(ret);
	return ret;
}

inline u16 sysInterlockedDecrement_NoWrapping(volatile u16* destination)
{
	const u16 ret = sysInterlockedDecrement(destination);
	Assert((u16)(ret+1));
	return ret;
}

inline u8 sysInterlockedDecrement_NoWrapping(volatile u8* destination)
{
	const u8 ret = sysInterlockedDecrement(destination);
	Assert((u8)(ret+1));
	return ret;
}

inline u32 sysInterlockedAdd(volatile u32* destination, s32 value) 
{
    sysInterlockedAssert(destination);
	uint32_t buf[32] __attribute__((aligned(128)));
	return cellAtomicAdd32(buf, (uint64_t)destination, value);
}

inline u64 sysInterlockedAdd(volatile u64* destination, s64 value) 
{
	sysInterlockedAssert(destination);
	uint64_t buf[16] __attribute__((aligned(128)));
	return cellAtomicAdd64(buf, (uint64_t)destination, value);
}

inline void* sysInterlockedAddPointer(volatile void ** destination, ptrdiff_t value) {
    return (void*) sysInterlockedAdd((uint32_t*)destination, value);
}

inline u32 sysInterlockedOr(volatile u32* destination, u32 value) 
{
    sysInterlockedAssert(destination);
	uint32_t buf[32] __attribute__((aligned(128)));
	return cellAtomicOr32(buf, (uint64_t)destination, value);
}

inline u32 sysInterlockedAnd(volatile u32* destination, u32 value) 
{
    sysInterlockedAssert(destination);
	uint32_t buf[32] __attribute__((aligned(128)));
	return cellAtomicAnd32(buf, (uint64_t)destination, value);
}

inline u32 sysInterlockedRead(const volatile u32* destination) 
{
    sysInterlockedAssert(destination);
	uint32_t buf[32] __attribute__((aligned(128)));
	return cellAtomicNop32(buf, (uint64_t)destination);
}

inline u64 sysInterlockedRead(const volatile u64* destination) 
{
	sysInterlockedAssert(destination);
	uint64_t buf[16] __attribute__((aligned(128)));
	return cellAtomicNop64(buf, (uint64_t)destination);
}

inline void* sysInterlockedReadPointer(void* const volatile* target)
{
	return (void*)sysInterlockedRead((uint32_t*)target);
}

#elif 0

//These functions should never be used (they're not atomic) but they document
//the intended behavior.

u32 sysInterlockedExchange(volatile u32* destination, u32 exchange)
{
    unsigned current = *destination;
    *destination = exchange;
    return current;
}

u32 sysInterlockedCompareExchange(volatile u32* destination, u32 exchange, u32 comparand)
{
    unsigned current = *destination;
    if(current == comparand)
        *destination = exchange;
    return current;
}

void* sysInterlockedExchangePointer(volatile void* destination, void* exchange)
{
    return (void*)sysInterlockedExchange((unsigned*)destination, (unsigned)exchange);
}

void* sysInterlockedCompareExchangePointer(volatile void* destination, void* exchange, void* comparand)
{
    return (void*)sysInterlockedCompareExchange((unsigned*)destination, (unsigned)exchange, (unsigned)comparand);
}

u32 sysInterlockedIncrement(volatile u32* destination)
{
    return ++(*destination);
}

u32 sysInterlockedDecrement(volatile u32* destination)
{
    return --(*destination);
}

#endif

#if !__SPU

inline u16 sysInterlockedIncrement(volatile u16* destination)
{
#	if RSG_ORBIS
		return sceAtomicIncrement16((volatile int16_t*)destination)+1;
#	else
		// Apparently Microsoft doesn't have any 16-bit intrinsics for x86/x64 platforms, despite just needing
		//      lock xadd (or lock inc if the return value is not used)
		// TODO: Check if this is worth implementing with inline assembler for Microsoft compilers
		//
		// PPC does not have native support for 16-bit atomics, so the additional complexity is required.
		//

		// Incrementing the most significant bytes is simpler, as there is no worry about corrupting the other 16-bytes
		volatile u32 *dst32 = (u32*)((uptr)destination&~3);
#		if __BE
			// Halfword at word-aligned address, two most-significant bytes on big-endian machine
			if (~(uptr)destination & 2)
				return (u16)(((sysInterlockedAdd((u32*)dst32, 65536)) >> 16) + 1);
#		else
			// Halfword at non-word-aligned address, two most-significant bytes on little-endian machine
			if ((uptr)destination & 2)
				return (u16)(((sysInterlockedAdd((u32*)dst32, 65536)) >> 16) + 1);
#		endif

		// Halfword at non-word-aligned address, two least-significant bytes on big-endian machine
		// Halfword at word-aligned address, two least-significant bytes on little-endian machine
		u32 cmp, wr;
		do
		{
			cmp = *dst32;
			wr = (cmp&0xffff0000) | ((cmp+1)&0xffff);
		}
		while (sysInterlockedCompareExchange(dst32, wr, cmp) != cmp);
		return (u16)wr;
#	endif
}

inline u8 sysInterlockedIncrement(volatile u8* destination)
{
#	if RSG_ORBIS
		return sceAtomicIncrement8((volatile int8_t*)destination)+1;
#	else
		volatile u32 *dst32 = (u32*)((uptr)destination&~3);
		const uptr byteOffset = ((uptr)destination & 3);
		const uptr bitOffset = byteOffset << 3;
#		if __BE
			if (byteOffset == 0)
				return (u8)(((sysInterlockedAdd((u32*)dst32, 0x01000000)) >> 24) + 1);
			const u32 mask = (u32)0xff000000 >> bitOffset;
#		else
			if (byteOffset == 3)
				return (u8)(((sysInterlockedAdd((u32*)dst32, 0x01000000)) >> 24) + 1);
			const u32 mask = 0x000000ff << bitOffset;
#		endif
		const u32 sub = mask;
		u32 cmp, wr;
		do
		{
			cmp = *dst32;
			wr = (cmp&~mask) | ((cmp-sub)&mask);
		}
		while (sysInterlockedCompareExchange(dst32, wr, cmp) != cmp);
#		if __BE
 			return (u8)(wr >> (24-bitOffset));
#		else
 			return (u8)(wr >> bitOffset);
#		endif
#	endif
}

inline u16 sysInterlockedDecrement(volatile u16* destination)
{
#	if RSG_ORBIS
		return sceAtomicDecrement16((volatile int16_t*)destination)-1;
#	else
		// Decrementing the most significant bytes is simpler, as there is no worry about corrupting the other 16-bytes
		volatile u32 *dst32 = (u32*)((uptr)destination&~3);
#		if __BE
			// Halfword at word-aligned address, two most-significant bytes on big-endian machine
			if (~(uptr)destination & 2)
				return (u16)(((sysInterlockedAdd((u32*)dst32, -65536)) >> 16) - 1);
#		else
			// Halfword at non-word-aligned address, two most-significant bytes on little-endian machine
			if ((uptr)destination & 2)
				return (u16)(((sysInterlockedAdd((u32*)dst32, -65536)) >> 16) - 1);
#		endif

		// Halfword at non-word-aligned address, two least-significant bytes on big-endian machine
		// Halfword at word-aligned address, two least-significant bytes on little-endian machine
		u32 cmp, wr;
		do
		{
			cmp = *dst32;
			wr = (cmp&0xffff0000) | ((cmp-1)&0xffff);
		}
		while (sysInterlockedCompareExchange(dst32, wr, cmp) != cmp);
		return (u16)wr;
#	endif
}

inline u8 sysInterlockedDecrement(volatile u8* destination)
{
#	if RSG_ORBIS
		return sceAtomicDecrement8((volatile int8_t*)destination)-1;
#	else
		volatile u32 *dst32 = (u32*)((uptr)destination&~3);
		const uptr byteOffset = ((uptr)destination & 3);
		const uptr bitOffset = byteOffset << 3;
#		if __BE
			if (byteOffset == 0)
				return (u8)(((sysInterlockedAdd((u32*)dst32, 0xff000000)) >> 24) - 1);
			const u32 mask = (u32)0xff000000 >> bitOffset;
#		else
			if (byteOffset == 3)
				return (u8)(((sysInterlockedAdd((u32*)dst32, 0xff000000)) >> 24) - 1);
			const u32 mask = 0x000000ff << bitOffset;
#		endif
		const u32 add = mask;
		u32 cmp, wr;
		do
		{
			cmp = *dst32;
			wr = (cmp&~mask) | ((cmp+add)&mask);
		}
		while (sysInterlockedCompareExchange(dst32, wr, cmp) != cmp);
#		if __BE
 			return (u8)(wr >> (24-bitOffset));
#		else
 			return (u8)(wr >> bitOffset);
#		endif
#	endif
}

#if RSG_ORBIS

inline u16 sysInterlockedIncrement_NoWrapping(volatile u16* destination)
{
	const u16 ret = sysInterlockedIncrement(destination);
	Assert(ret);
	return ret;
}

inline u8 sysInterlockedIncrement_NoWrapping(volatile u8* destination)
{
	const u8 ret = sysInterlockedIncrement(destination);
	Assert(ret);
	return ret;
}

inline u16 sysInterlockedDecrement_NoWrapping(volatile u16* destination)
{
	const u16 ret = sysInterlockedDecrement(destination);
	Assert((u16)(ret+1));
	return ret;
}

inline u8 sysInterlockedDecrement_NoWrapping(volatile u8* destination)
{
	const u8 ret = sysInterlockedDecrement(destination);
	Assert((u8)(ret+1));
	return ret;
}

#else

// NOTE -- this will NOT handle overflow and underflow cases correctly!
inline u16 sysInterlockedIncrement_NoWrapping(volatile u16* destination)
{
	u16 ret;
#if __BE
	// Halfword at non-word-aligned address, two least-significant bytes on big-endian machine
	if ((uptr)destination & 2)
		ret = ((sysInterlockedAdd((u32*)((uptr)destination ^ 2), 1)) & 65535) + 1;
	// Halfword at word-aligned address, two most-significant bytes on big-endian machine
	else
		ret = ((sysInterlockedAdd((u32*)destination, 65536)) >> 16) + 1;
#else
	// Halfword at non-word-aligned address, two most-significant bytes on little-endian machine
	if ((uptr)destination & 2)
		ret = ((sysInterlockedAdd((u32*)((uptr)destination ^ 2), 65536)) >> 16) + 1;
	// Halfword at word-aligned address, two least-significant bytes on little-endian machine
	else
		ret = ((sysInterlockedAdd((u32*)destination, 1)) & 65535) + 1;
#endif
	Assert(ret);
	return ret;
}

// NOTE -- this will NOT handle overflow and underflow cases correctly!
inline u8 sysInterlockedIncrement_NoWrapping(volatile u8* destination)
{
	volatile u32 *dst32 = (u32*)((uptr)destination&~3);
	const uptr byteOffset = ((uptr)destination & 3);
	const uptr bitOffset = byteOffset << 3;
#	if __BE
		const u32 add = 0x01000000 >> bitOffset;
		const u8  ret = (u8)((sysInterlockedAdd(dst32, add) >> (24-bitOffset)) + 1);
#	else
		const u32 add = 0x00000001 << bitOffset;
		const u8  ret = (u8)((sysInterlockedAdd(dst32, add) >> bitOffset) + 1);
#	endif
	Assert(ret);
	return ret;
}

// NOTE -- this will NOT handle overflow and underflow cases correctly!
inline u16 sysInterlockedDecrement_NoWrapping(volatile u16* destination)
{
	u16 ret;
#if __BE
	// Halfword at non-word-aligned address, two least-significant bytes on big-endian machine
	if ((uptr)destination & 2)
		ret = ((sysInterlockedAdd((u32*)((uptr)destination ^ 2), -1)) & 65535) - 1;
	// Halfword at word-aligned address, two most-significant bytes on big-endian machine
	else
		ret = ((sysInterlockedAdd((u32*)destination, -65536)) >> 16) - 1;
#else
	// Halfword at non-word-aligned address, two most-significant bytes on little-endian machine
	if ((uptr)destination & 2)
		ret = ((sysInterlockedAdd((u32*)((uptr)destination ^ 2), -65536)) >> 16) - 1;
	// Halfword at word-aligned address, two least-significant bytes on little-endian machine
	else
		ret = ((sysInterlockedAdd((u32*)destination, -1)) & 65535) - 1;
#endif
	Assert((u16)(ret+1));
	return ret;
}

// NOTE -- this will NOT handle overflow and underflow cases correctly!
inline u8 sysInterlockedDecrement_NoWrapping(volatile u8* destination)
{
	volatile u32 *dst32 = (u32*)((uptr)destination&~3);
	const uptr byteOffset = ((uptr)destination & 3);
	const uptr bitOffset = byteOffset << 3;
#	if __BE
		const u32 add = (s32)0xff000000 >> bitOffset;
		const u8  ret = (u8)((sysInterlockedAdd(dst32, add) >> (24-bitOffset)) - 1);
#	else
		const u32 add = 0xffffffff << bitOffset;
		const u8  ret = (u8)((sysInterlockedAdd(dst32, add) >> bitOffset) - 1);
#	endif
	Assert((u8)(ret+1));
	return ret;
}

#endif

#endif

template<class T>
inline T* sysInterlockedCompareExchange(T* volatile* destination, T* exchange, T* comparand)
{
	return (T*)sysInterlockedCompareExchange((volatile uptr*)destination, (uptr)exchange, (uptr)comparand);
}

}   //namespace rage
