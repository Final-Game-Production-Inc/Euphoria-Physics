//
// system/membarrier.h
//
// Copyright (C) 2011-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MEMBARRIER_H
#define SYSTEM_MEMBARRIER_H

#if __PPU
#	include <ppu_intrinsics.h>
#elif __WIN32PC
#	pragma warning(push)
#	pragma warning(disable:4668)    // 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
#	pragma warning(disable:4995)    // 'function': name was marked as #pragma deprecated
	// math.h must be included before intrin.h on x64 debug builds
#	include <math.h>
#	include <intrin.h>
#	include <emmintrin.h>
#	pragma warning(pop)
#elif RSG_DURANGO
#	include <emmintrin.h>
#elif RSG_ORBIS
#	include <emmintrin.h>
#elif RSG_XENON
#	include <PPCIntrinsics.h>
#endif


namespace rage
{

// PURPOSE:	Force barrier ordering (compiler and cpu) between loads
__forceinline void sysMemReadBarrier()
{
#	if __PPU
		__lwsync();
#	elif __SPU
		// nop
#	elif __WIN32PC || RSG_DURANGO || RSG_ORBIS
		_mm_lfence();
#	elif __XENON
		__lwsync();
#	else
#		error "not yet implemented for this platform"
#	endif
}

// PURPOSE:	Force barrier ordering (compiler and cpu) between stores
__forceinline void sysMemWriteBarrier()
{
#	if __PPU
		__lwsync();
#	elif __SPU
		// nop
#   elif RSG_DURANGO || RSG_ORBIS
		_mm_sfence();
#	elif __WIN32PC
#		if __64BIT
			__faststorefence();
#		else
			_mm_sfence();
#		endif
#	elif __XENON
		__lwsync();
#	else
#		error "not yet implemented for this platform"
#	endif
}

// PURPOSE:	Force barrier ordering (compiler and cpu) between all memory accesses
__forceinline void sysMemReadWriteBarrier()
{
#	if __PPU
		__lwsync();
#	elif __SPU
		// nop
#	elif __WIN32PC || RSG_DURANGO || RSG_ORBIS
		_mm_mfence();
#	elif __XENON
		__lwsync();
#	else
#		error "not yet implemented for this platform"
#	endif
}

} // namespace rage

#endif // SYSTEM_MEMBARRIER_H
