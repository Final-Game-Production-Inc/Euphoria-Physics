//
// system/memops.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MEMOPS_H
#define SYSTEM_MEMOPS_H

#if __PPU
#include <sys/types.h>
#include "system/cache.h"
#elif __PSP2 || RSG_ORBIS
#include <stddef.h>
#endif

#include <string.h>

// The /Wp64 compiler option to Microsoft compilers causes the completely useless
//  warning C4311: 'type cast' : pointer truncation from 'void *' to 'rage::uptr'
// Because the compiler doesn't realise that uptr will be 64-bits when compiling 64-bit code.
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable:4311)
#endif

#if __XENON
extern "C" void *XMemCpy(void *pDest, const void *pSrc, unsigned long count);
extern "C" void *XMemSet(void *pDest, int c, unsigned long count);
extern "C" void *XMemCpy128(void *pDest, const void *pSrc, unsigned long count);
extern "C" void *XMemCpyStreaming(void *pDest, const void *pSrc, unsigned long count);
extern "C" void *XMemSet128(void *pDest, int c, unsigned long count);
#endif

namespace rage {

#if __PPU
void *sysMemCpySelect(void *pDest, const void *pSrc, size_t count);
void *sysMemCpyNonCachable(void *pDest, const void *pSrc, size_t count);
void *sysMemCpyImpl(void *pDest, const void *pSrc, size_t count);
void *sysMemCpy128Impl(void *pDest, const void *pSrc, size_t count);
void *sysMemCpyStreamingImpl(void *pDest, const void *pSrc, size_t count);
void *sysMemSetSelect(void *pDest, int c, size_t count);
void *sysMemSetImpl(void *pDest, int c, size_t count);
void *sysMemSet128Impl(void *pDest, int c, size_t count);
void *sysMemZeroImpl(void *pDest, size_t count);
#endif


// While all this code looks like madness, "why not just use memcpy instead of
// sysMemCpy when the size is a small compile time constant ???", there is
// actually a benefit to all of this.  With our default compiler options, SNC
// will not inline memcpy, ever.  Here, we have some control over things.  Also,
// higher level code can blindly just use sysMemCpy everywhere without ever
// thinking about things (though allowing people to not think about what the
// compiler is doing with code they write is a somewhat dubious benefit).


// PURPOSE: Check if a value is a compile time constant, and less than or equal
//          to a specified power of two value.  Generates a compile time constant
//          result.
// PARAMS:  VAL      - value to be tested
//          CMP_POW2 - compile time constant integer that is a power of 2
// RETURNS: non-zero iff VAL is a compile time constant and VAL <= CMP_POW2
// NOTES:   Based on code from
//          http://stackoverflow.com/questions/3299834/c-compile-time-constant-detection
struct sysMem_CheckConst0
{
	// A compile time constant 0 will resolve to this function overload,
	// since no implicit conversion is required.
	static u8 Check(void*) { return 0; }

	// All other integers will resolve to this second function overload.
	struct ImplicitConversion { /*implicit*/ ImplicitConversion(size_t) {} };
	static u16 Check(ImplicitConversion) { return 0; }
};
#define SYS_MEM_IS_COMPILE_TIME_CONST_ZERO(VAL) (sizeof(sysMem_CheckConst0::Check(VAL)) == 1)
#define SYS_MEM_IS_COMPILE_TIME_CONST_LEQ(VAL, CMP_POW2) \
	(SYS_MEM_IS_COMPILE_TIME_CONST_ZERO(VAL) || SYS_MEM_IS_COMPILE_TIME_CONST_ZERO(((VAL)-1)^(((VAL)-1)&((CMP_POW2)-1))))


// This function is templated on the pointer types since the Microsoft compiler
// (360 anyways), uses the pointer types to in it's heuristic when determining
// wether to inline or not.
//
// With our default compiler settings, SNC needs some coaxing to force inline
// memcpy.
//
#ifdef __SNC__
#	pragma control %push deflib
#	pragma control %push inlinemaxsize
#	pragma control %push inlinesize
#	pragma control deflib=2
#	pragma control inlinemaxsize=50000
#	pragma control inlinesize=50000
#endif
template<bool MEMCPY> struct sysMemCpy_
{
	template<class T, class U>
	static __forceinline void *Cpy(T *dst, const U *src, size_t size)
	{
#		if defined(__GNUC__) && !defined(__SNC__)
			return __builtin_memcpy(dst, src, size);
#		else
			return memcpy(dst, src, size);
#		endif
	}

	// SFINAE based function overloads for pointer like types.
	// The type needs to have two nested types,
	//      typedef int IS_PTR_LIKE_TYPE;
	//      typedef TO  CAN_CAST_TO_PTR_TYPE;
	// where TO is the raw pointer type that can be cast to.
	// SYS_MEM_OPS_PTR_LIKE_TYPE can be used to define these.
	template<class T, class U>
	static __forceinline void *Cpy(const T &dst, const U &src, size_t size, typename T::IS_PTR_LIKE_TYPE=0, typename U::IS_PTR_LIKE_TYPE=0)
	{
		return Cpy(static_cast<typename T::CAN_CAST_TO_PTR_TYPE*>(dst), static_cast<const typename U::CAN_CAST_TO_PTR_TYPE*>(src), size);
	}
	template<class T, class U>
	static __forceinline void *Cpy(const T &dst, const U *src, size_t size, typename T::IS_PTR_LIKE_TYPE=0)
	{
		return Cpy(static_cast<typename T::CAN_CAST_TO_PTR_TYPE*>(dst), src, size);
	}
	template<class T, class U>
	static __forceinline void *Cpy(T *dst, const U &src, size_t size, typename U::IS_PTR_LIKE_TYPE=0)
	{
		return Cpy(dst, static_cast<const typename U::CAN_CAST_TO_PTR_TYPE*>(src), size);
	}
};
#ifdef __SNC__
#	pragma control %pop inlinesize
#	pragma control %pop inlinemaxsize
#	pragma control %pop deflib
#endif


#if __PPU
	template<> struct sysMemCpy_<false>
	{
		static __forceinline void *Cpy(void *dst, const void *src, size_t size)
		{
			return sysMemCpySelect(dst, src, size);
		}
	};

#elif __XENON
	template<> struct sysMemCpy_<false>
	{
		static __forceinline void *Cpy(void *dst, const void *src, size_t size)
		{
			return XMemCpy(dst, src, size);
		}
	};

#endif


// PURPOSE: Copies a specified number of bytes from any area
//          of memory to a region of cached memory.
// PARAMS:  dest  - Pointer to a cached memory address to copy data into.
//          src   - Pointer to the source from which to copy.
//          count - Number of bytes to copy.
// RETURNS: The original dest pointer.
// NOTES:   The memory blocks pointed to by the src and dest arguments must not overlap.
#ifndef _MSC_VER
#	define sysMemCpy(DST, SRC, SIZE) \
		sysMemCpy_<SYS_MEM_IS_COMPILE_TIME_CONST_LEQ((SIZE), 32)>::Cpy((DST), (SRC), (SIZE))
#elif _MSC_VER < 1600
	// 2008 compiler fails to compile
	//      sysMemCpy(this, pOther, sizeof(*this));
	// Looks like a template evaluation order bug.  The sizeof(*this) passed
	// through to SYS_MEM_IS_COMPILE_TIME_CONST_LEQ doesn't work,
#	define sysMemCpy    memcpy
#else // _MSC_VER >= 1600
	// For Microsoft compilers, disable the warning
	//  'argument' : conversion from 'int' to 'size_t', signed/unsigned mismatch
#	define sysMemCpy(DST, SRC, SIZE)                                                            \
		__pragma(warning(push))                                                                 \
		__pragma(warning(disable:4245))                                                         \
		sysMemCpy_<SYS_MEM_IS_COMPILE_TIME_CONST_LEQ((SIZE), 32)>::Cpy((DST), (SRC), (SIZE))    \
		__pragma(warning(pop))
#endif


// PURPOSE: Helper macros for allowing templated pointer like types to work with sysMemCpy.
// PARAMS:  TYPE - Base type that the templated type can be used as a pointer to.
#define SYS_MEM_OPS_PTR_LIKE_TYPE(TYPE)     \
	typedef int IS_PTR_LIKE_TYPE;           \
	typedef _T  CAN_CAST_TO_PTR_TYPE


// PURPOSE: Copies a specified number of bytes from any area
//          of memory to a region of cached memory.
// PARAMS:  dest  - Pointer to a cached memory address to copy data into,
//                  must be 128 byte aligned.
//          src   - Pointer to the source from which to copy, must be 16 byte aligned.
//          count - Number of bytes to copy, must be a multiple of 128 bytes.
// RETURNS: The original dest pointer.
inline void *sysMemCpy128(void *dest, const void *src, size_t count)
{
	FastAssert(!((((uptr)dest))&127));	// ensure 128 byte aligned
	FastAssert(!((((uptr)src))&15));		// ensure 16 byte aligned
	FastAssert(!((((uptr)count))&127));	// ensure 128 byte aligned
#if __XENON
	return XMemCpy128(dest, src, count);
#elif __PPU
	return sysMemCpy128Impl(dest, src, count);
#else
	return memcpy(dest, src, count);
#endif
}


// PURPOSE: Copies a specified number of bytes from any area
//          of memory to a region of cached memory.  Memory is
//          freed from L1/L2 cache after copying to minimize
//          disruption of exiting cache contents.
// PARAMS:  dest  - Pointer to a cached memory address to copy data into.
//          src   - Pointer to the source from which to copy.
//          count - Number of bytes to copy.
// RETURNS: The original dest pointer.
inline void *sysMemCpyStreaming(void *dest, const void *src, size_t count)
{
#if __XENON
	return XMemCpyStreaming(dest, src, count);
#elif __PPU
	if (!(((u32)dest)&15) && !(((u32)src)&15) && !(count&15) && count >= 256)
	{
		return sysMemCpyStreamingImpl(dest, src, count);
	}
	else
	{
		return memcpy(dest, src, count);
	}
#else
	return memcpy(dest, src, count);
#endif
}


#ifdef __SNC__
#	pragma control %push deflib
#	pragma control %push inlinemaxsize
#	pragma control %push inlinesize
#	pragma control deflib=2
#	pragma control inlinemaxsize=50000
#	pragma control inlinesize=50000
#endif
template<bool MEMSET> struct sysMemSet_
{
	template<class T>
	static __forceinline void *Set(T *dst, int c, size_t size)
	{
#		if defined(__GNUC__) && !defined(__SNC__)
			return __builtin_memset(dst, c, size);
#		else
			return memset(dst, c, size);
#		endif
	}

	template<class T, class U>
	static __forceinline void *Set(const T &dst, int c, size_t size, typename T::IS_PTR_LIKE_TYPE=0)
	{
		return Set(static_cast<typename T::CAN_CAST_TO_PTR_TYPE*>(dst), c, size);
	}
};
#ifdef __SNC__
#	pragma control %pop inlinesize
#	pragma control %pop inlinemaxsize
#	pragma control %pop deflib
#endif


#if __PPU
	template<> struct sysMemSet_<false>
	{
		static __forceinline void *Set(void *dst, int c, size_t size)
		{
			return sysMemSetSelect(dst, c, size);
		}
	};

#elif __XENON
	template<> struct sysMemSet_<false>
	{
		static __forceinline void *Set(void *dst, int c, size_t size)
		{
			return XMemSet(dst, c, size);
		}
	};

#endif


// PURPOSE: Stores a character into a buffer repeatedly, for a specified
//          number of bytes.
// PARAMS:  dest  - Pointer to a cached memory address to copy data into.
//          c     - Character to set.
//          count - Number of times to store c in the buffer.
// RETURNS: The original dest pointer.
#ifndef _MSC_VER
#	define sysMemSet(DST, C, SIZE) \
		sysMemSet_<SYS_MEM_IS_COMPILE_TIME_CONST_LEQ((SIZE), 32)>::Set((DST), (C), (SIZE))
#elif _MSC_VER < 1600
	// 2008 compiler fails to compile
	//      sysMemSet(this, 0, sizeof(*this));
	// Looks like a template evaluation order bug.  The sizeof(*this) passed
	// through to SYS_MEM_IS_COMPILE_TIME_CONST_LEQ doesn't work,
#	define sysMemSet    memset
#else // _MSC_VER >= 1600
	// For Microsoft compilers, disable the warning
	//  'argument' : conversion from 'int' to 'size_t', signed/unsigned mismatch
#	define sysMemSet(DST, C, SIZE)                                                          \
		__pragma(warning(push))                                                             \
		__pragma(warning(disable:4245))                                                     \
		sysMemSet_<SYS_MEM_IS_COMPILE_TIME_CONST_LEQ((SIZE), 32)>::Set((DST), (C), (SIZE))  \
		__pragma(warning(pop))
#endif


// PURPOSE: Stores a character into a buffer repeatedly, for a specified
//          number of bytes.
// PARAMS:  dest  - Pointer to a cached memory address to copy data into,
//                  must be 128 byte aligned.
//          c     - Character to set.
//          count - Number of times to store c in the buffer, must be a
//                  multiple of 128.
// RETURNS: The original dest pointer.
inline void *sysMemSet128(void *dest, int c, size_t count)
{
	FastAssert(!((((uptr)dest))&127));	// ensure 128 byte aligned
	FastAssert(!((((uptr)count))&127));	// ensure 128 byte aligned
#if __XENON
	return XMemSet128(dest, c, count);
#elif __PPU
	if ((u8)c == 0)
	{
		for (size_t i = 0; i < count; i += 128)
			ZeroDC(dest, i);
		return dest;
	}
	else if (count >= 256)
	{
		return sysMemSet128Impl(dest, c, count);
	}
	else
	{
		return memset(dest, c, count);
	}
#else
	return memset(dest, c, count);
#endif
}


// PURPOSE: Zero out a block of memory of size known at compile time
// PARAMS:
//	_NumBytes - The number of bytes to zero, known at compile time
//  dest - The beginning of the memory where zeros should be written
// NOTES:
//    dest must be 4 byte aligned
//    Future improvement: use VMX instructions to speed it up even more
template <int _NumDWords>
__forceinline void sysMemZeroWords(void* dest)
{
#ifdef __SNC__
	memset(dest, 0, _NumDWords*4);
#else
	u32* u32dest = (u32*)dest;

	// Big bitsets get memsetted to reduce code size
	// All the branches get compiled out because _NumBytes is a constant
	if (_NumDWords > 16)
	{
		sysMemSet( dest, 0, _NumDWords*4 );
		return;
	}

	if (_NumDWords >= 1)  u32dest[0]  = 0;
	if (_NumDWords >= 2)  u32dest[1]  = 0;
	if (_NumDWords >= 3)  u32dest[2]  = 0;
	if (_NumDWords >= 4)  u32dest[3]  = 0;
	if (_NumDWords >= 5)  u32dest[4]  = 0;
	if (_NumDWords >= 6)  u32dest[5]  = 0;
	if (_NumDWords >= 7)  u32dest[6]  = 0;
	if (_NumDWords >= 8)  u32dest[7]  = 0;
	if (_NumDWords >= 9)  u32dest[8]  = 0;
	if (_NumDWords >= 10) u32dest[9]  = 0;
	if (_NumDWords >= 11) u32dest[10] = 0;
	if (_NumDWords >= 12) u32dest[11] = 0;
	if (_NumDWords >= 13) u32dest[12] = 0;
	if (_NumDWords >= 14) u32dest[13] = 0;
	if (_NumDWords >= 15) u32dest[14] = 0;
	if (_NumDWords >= 16) u32dest[15] = 0;
#endif
}


template <int _Bytes>
__forceinline void sysMemZeroBytes(void* dest)
{
	CompileTimeAssert(!(_Bytes & 3));
	sysMemZeroWords<_Bytes/4>(dest);
}

} // namespace rage

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // SYSTEM_MEMOPS_H
