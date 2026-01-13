//
// system/llsc.h
//
// Copyright (C) 2012-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_LLSC_H
#define SYSTEM_LLSC_H

#if __PPU
#	include <ppu_intrinsics.h>
#	define LLSC_MSR()   (void)0

#elif __XENON

	// Note that these functions need to be extern "C"'d, because of the
	// different return value type in this header and in the .cpp.  Without the
	// extern "C", this causes link errors.  The extern "C", also prevents us
	// from placing these functions in namespace rage.

	// PURPOSE: Execute a LWARX
	// PARAMS:
	//  mem - memory to load and reserve
	//  msr - location to store previous MSR value
	// RETURNS: Value loaded from mem
	// NOTES:
	//  Disables interrupts.  Must be matched to a sys_360_stwcx call.
	extern "C" rage::u32 sys_360_lwarx(const volatile rage::u32 *mem, rage::u64 *msr);

	// PURPOSE: Execute a LDARX
	// PARAMS:
	//  mem - memory to load and reserve
	//  msr - location to store previous MSR value
	// RETURNS: Value loaded from mem
	// NOTES:
	//  Disables interrupts.  Must be matched to a sys_360_stwcx call.
	extern "C" rage::u64 sys_360_ldarx(const volatile rage::u64 *mem, rage::u64 *msr);

	// PURPOSE: Execute a STWCX.
	// PARAMS:
	//  mem - memory to conditionally store to
	//  msr - previous MSR value
	// RETURNS: true iff store was successful
	// NOTES:
	//  Restores previous interupt enable state to MSR register.
	extern "C" bool sys_360_stwcx(volatile rage::u32 *mem, rage::u32 val, rage::u64 msr);

	// PURPOSE: Execute a STDCX.
	// PARAMS:
	//  mem - memory to conditionally store to
	//  msr - previous MSR value
	// RETURNS: true iff store was successful
	// NOTES:
	//  Restores previous interupt enable state to MSR register.
	extern "C" bool sys_360_stdcx(volatile rage::u64 *mem, rage::u64 val, rage::u64 msr);

	// Macros to help make code compatable with both the PS3 PPU and the 360 Xenon.
	// LLSC_MSR macro for use at local scope only.  The variable it defines must be per thread.
#	define LLSC_MSR()           rage::u64 llsc_msr
#	define __lwarx(PTR)         sys_360_lwarx((PTR), &llsc_msr)
#	define __ldarx(PTR)         sys_360_ldarx((PTR), &llsc_msr)
#	define __stwcx(PTR, VAL)    sys_360_stwcx((PTR), (VAL), llsc_msr)
#	define __stdcx(PTR, VAL)    sys_360_stdcx((PTR), (VAL), llsc_msr)

#endif // __XENON


// Templated helper functions to allow the same code to work with both 32 and 64-bit pointers.
#if __PPU || __XENON
	namespace rage
	{
		template<class ATOMIC_TYPE> static inline ATOMIC_TYPE LoadAndReserve_Imp(volatile ATOMIC_TYPE *ptr XENON_ONLY(, u64 *msr));
#		if __PPU
			template<> static inline u32 LoadAndReserve_Imp(volatile u32 *ptr) { return __lwarx(ptr); }
			template<> static inline u64 LoadAndReserve_Imp(volatile u64 *ptr) { return __ldarx(ptr); }
#		elif __XENON
			template<> static inline u32 LoadAndReserve_Imp(volatile u32 *ptr, u64 *msr) { return sys_360_lwarx(ptr, msr); }
			template<> static inline u64 LoadAndReserve_Imp(volatile u64 *ptr, u64 *msr) { return sys_360_ldarx(ptr, msr); }
#		endif

		template<class ATOMIC_TYPE> static inline bool StoreConditional_Imp(volatile ATOMIC_TYPE *ptr, ATOMIC_TYPE val XENON_ONLY(, u64 msr));
#		if __PPU
			template<> static inline bool StoreConditional_Imp(volatile u32 *ptr, u32 val) { return __stwcx(ptr, val); }
			template<> static inline bool StoreConditional_Imp(volatile u64 *ptr, u64 val) { return __stdcx(ptr, val); }
#		elif __XENON
			template<> static inline bool StoreConditional_Imp(volatile u32 *ptr, u32 val, u64 msr) { return sys_360_stwcx(ptr, val, msr); }
			template<> static inline bool StoreConditional_Imp(volatile u64 *ptr, u64 val, u64 msr) { return sys_360_stdcx(ptr, val, msr); }
#		endif
	}

#	define LoadAndReserve(PTR)          rage::LoadAndReserve_Imp  ((PTR)        XENON_ONLY(, &llsc_msr))
#	define StoreConditional(PTR, VAL)   rage::StoreConditional_Imp((PTR), (VAL) XENON_ONLY(,  llsc_msr))

#endif // __PPU || __XENON

#endif // SYSTEM_LLSC_XBOX360_H
