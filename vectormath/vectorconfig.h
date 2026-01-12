
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTORMATH_VECTORCONFIG_H
#define VECTORMATH_VECTORCONFIG_H


// PURPOSE: Define SCALAR_TYPES_ONLY to be 1 to use the scalar versions ONLY. Good for debugging vector types. But NOT good for performance!
// (Not simply b/c scalar types are used, but b/c the fallback itself isn't as efficient as re-coding using scalar structures/functions.)
// This will be good for __TOOL builds that have alignment problems, as it will disable SSE vectors (and thus most alignment requirements in
// RAGE... since even /vector/ doesn't use SSE vectors!)
#define SCALAR_TYPES_ONLY			__PSP2	// TODO: Fix. This won't compile for all solutions when == 1 on __PS3. Also, need to fix runtime warnings (try phys sample_articulated Win32 Debug).

// PURPOSE: Define USE_INTRINSICS to be 1 to use intrinsics (both scalar and vector). Set to zero to use pure C++.
// (This differs from SCALAR_TYPES_ONLY in that even if SCALAR_TYPES_ONLY==1, scalar intrinics like __fsel() will still be used.
// So, USE_INTRINSICS==0 short-circuits even more than SCALAR_TYPES_ONLY==1.)
#define USE_INTRINSICS				1

// PURPOSE: If VECTORIZE is non-zero, the vectorized implementations will be used if possible.
#define VECTORIZE					(RSG_CPU_PPC || RSG_CPU_SPU || RSG_CPU_INTEL)

// PURPOSE: If LEGACYCONVERT_REINTERPRETCAST_DEBUG is non-zero, we'll make reinterpret_cast macros do type-checking at compile-time by thunking
// down to functions instead of straight to reinterpret_cast's. (See legacyconvert.h.) As for whether or not there is some performance overhead
// to the inlined function wrappers, I don't know, so set LEGACYCONVERT_REINTERPRETCAST_DEBUG to 0 to be sure to be most performant.
#define LEGACYCONVERT_REINTERPRETCAST_DEBUG			__DEV

// PURPOSE: If we don't want vectorized, we typedef the vectorized types to be normal types.
// This is useful for:
// 1) Testing a fallback version for correctness, if the optimized version produces dubious effects.
// 2) In case a non-vectorized (or a badly-vectorized) platform comes along... although, some hand-optimization might be needed in that case.
//		e.g. the SIMD V4Sin(Vector_4V) translates to four scalar sinf() calls, which is expensive (probably don't need 4 sine results)!
// 3) In case we want to throw away our vector alignment requirements because there are problems with getting aligned memory from the heap when
//		not using our own custom allocators (i.e. __TOOL builds!)
#if !(SCALAR_TYPES_ONLY) && VECTORIZE && USE_INTRINSICS
	#define UNIQUE_VECTORIZED_TYPE	1
#else
	#define UNIQUE_VECTORIZED_TYPE	0
#endif

// PURPOSE: Set equal to 1 to use the latter everywhere in place of the former (vectorized AoS implementation).
// The former has less instructions but more memory references, which the latter has more instructions but no memory references.
// This is useful since it's hard to tell on a small scale which one is better - it depends on situation, cache misses, etc.
//
// Original:
//merged0 = Vec::V4MergeXY( inCol0, inCol2 );
//merged1 = Vec::V4MergeZW( inCol0, inCol2 );
//outVec0 = Vec::V4MergeXY( merged0, inCol1 );
//outVec1 = Vec::V4PermuteTwo<Vec::Z1,Vec::Y2,Vec::W1,Vec::X1>( merged0, inCol1 ); // May fetch from memory, depending on platform
//outVec2 = Vec::V4PermuteTwo<Vec::X1,Vec::Z2,Vec::Y1,Vec::X1>( merged1, inCol1 ); // May fetch from memory, depending on platform
//inoutMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
//
// Alternate:
//Vec::Vector_4V _zero = Vec::V4VConstant(V_ZERO);
//Vec::Vector_4V temp0 = Vec::V4MergeXY(inCol0, inCol2);
//Vec::Vector_4V temp1 = Vec::V4MergeXY(inCol1, _zero);
//Vec::Vector_4V temp2 = Vec::V4MergeZW(inCol0, inCol2);
//Vec::Vector_4V temp3 = Vec::V4MergeZW(inCol1, _zero);
//outVec0 = Vec::V4MergeXY(temp0, temp1);
//outVec1 = Vec::V4MergeZW(temp0, temp1);
//outVec2 = Vec::V4MergeXY(temp2, temp3);
//inoutMat.SetColsIntrin128( outVec0, outVec1, outVec2 );
#define USE_ALTERNATE_3X3_TRANSPOSE			1

// PURPOSE: Set equal to 1 to use an alternative vactorized V4Negate() inplementation.
// This is useful since it's hard to tell on a small scale which one is better - it depends on situation. (Note that this option
// won't affect __WIN32PC.)
#define USE_ALTERNATE_NEGATE				0

// PURPOSE: Set equal to 1 to use an alternative vactorized V4SetWZero() inplementation.
// This is useful since it's hard to tell on a small scale which one is better - it depends on situation.
#define USE_ALTERNATE_SETWZERO				0

// NOTE: THE BELOW IS NOT YET IN USE!
// PURPOSE: Many constants are generated using instructions. In some cases this is a win. In other cases (tight loops), this may be a lose;
// we might rather incur cache hits. This parameter allows us to tradeoff how many instructions we will allow in constant generation before
// we'd rather just load from memory. This parameter is used as a default parameter in all templated vectorized constant generation functions.
// Since it's an optional parameter, you can tweak it for specific constants as desired.
// This variable is absolutely necessary, as "the point at which generating a const by instructions is worth it" is case-specific, and very
// hard to determine without large-app runtime profiling.
// Example: If the below is "2", then any const generations that take >2 instructions will fetch from memory instead.
#define CONSTGEN_NUM_INSTRUCTIONS_ALLOWED	1 // TODO: Use


// PURPOSE: So we can compile out some code on Managed (C++/CLI) builds
// NOTE: Move this to a force-include?
#ifdef _MANAGED
#define RSG_MANAGED _MANAGED
#else
#define RSG_MANAGED 0
#endif


#endif // VECTORMATH_VECTORCONFIG_H
