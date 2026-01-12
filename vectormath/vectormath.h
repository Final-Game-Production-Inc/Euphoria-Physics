#ifndef VECTORMATH_VECTORMATH_H
#define VECTORMATH_VECTORMATH_H

//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//


	//=======================================================================================================================================
	//
	//
	//   INTERFACE
	//
	//
	//=======================================================================================================================================

#include "math/constants.h"
#include "channel.h"
#include "vectortypes.h"
#include "vectorutility.h"

#if UNIQUE_VECTORIZED_TYPE

	#include "v1vector4v.h"
	#include "v2vector4v.h"
	#include "v3vector4v.h"
	#include "v4vector4v.h"
	#include "v4vector4vutility.h"

#endif // UNIQUE_VECTORIZED_TYPE

	#include "v2vector2.h"
	#include "v3vector3.h"
	#include "v1vector4.h"
	#include "v2vector4.h"
	#include "v3vector4.h"
	#include "v4vector4.h"


	//=======================================================================================================================================
	//
	//
	//   IMPLEMENTATIONS
	//
	//
	//=======================================================================================================================================

	#include "mathops.h" // Yes, there is a bit of duplicated code in here from amath.h, but I'm trying to avoid dependence on amath.h.
	#include "vector/allbitsf.h"

	// TEMP
//	#include "system/memory.h"


	// Platform-specific implementations.
	//
	// Write fast VECTOR versions in the *vector*vcore_*.inl's. These are all essential!
	// The cross-platform *vector*v.inl's make calls to those platform-specific core functions.
	//
	// If a faster approach for a function not in "core" is devised for a specific platform,
	// then bring that function into core, and write versions for each platform.
	
	// PS3
#if __PS3 && UNIQUE_VECTORIZED_TYPE
	#include "v4vector4vcore_ps3.inl"
	#include "v3vector4vcore_ps3.inl"
	#include "v2vector4vcore_ps3.inl"
	#include "v1vector4vcore_ps3.inl"	

	// XBox 360
#elif __XENON && UNIQUE_VECTORIZED_TYPE
	#include "v4vector4vcore_xbox360.inl"
	#include "v3vector4vcore_xbox360.inl"
	#include "v2vector4vcore_xbox360.inl"
	#include "v1vector4vcore_xbox360.inl"

	// Windows
#elif RSG_CPU_INTEL && UNIQUE_VECTORIZED_TYPE
	#include "v4vector4vcore_win32pc.inl"
	#include "v3vector4vcore_win32pc.inl"
	#include "v2vector4vcore_win32pc.inl"
	#include "v1vector4vcore_win32pc.inl"
#endif

	// Cross-platform vector functions that rely on "core" (platform-specific) functionality.
#if UNIQUE_VECTORIZED_TYPE
	#include "v4vector4vutility.inl"
	#include "v4vector4v.inl"
	#include "v3vector4v.inl"
	#include "v2vector4v.inl"
	#include "v1vector4v.inl"
#endif

	// Provides:
	//	-- Scalar vector functions.
	//	-- Fallbacks for vectorized vector functions (for nonvectorized platforms).
	#include "v4vector4.inl"
	#include "v3vector4.inl"
	#include "v2vector4.inl"
	#include "v1vector4.inl"
	#include "v3vector3.inl"
	#include "v2vector2.inl"


//=======================================================================================================================================
//
//
//   SCALAR <-> VECTOR CONVERSION
//
// Use these to use either float or vec load/store instructions, regardless of the data type.
// If just converting between local variables, use Load*(). This will incur one LHS (necessarily).
//
//
//=======================================================================================================================================

namespace rage
{
namespace Vec
{

	

	// Scalar float instructions only.
	void V4LoadAsScalar( Vector_4& a, Vector_4V const& loc );
	void V4StoreAsScalar( Vector_4V& loc, Vector_4 const& a );
	void V3LoadAsScalar( Vector_3& a, Vector_4V const& loc );
	void V3StoreAsScalar( Vector_4V& loc, Vector_3 const& a );
	void V2LoadAsScalar( Vector_2& a, Vector_4V const& loc );
	void V2StoreAsScalar( Vector_4V& loc, Vector_2 const& a );

	// Vector instructions only.
	void V4LoadAsVec( Vector_4V& a, Vector_4 const& loc );
	void V4StoreAsVec( Vector_4& loc, Vector_4V const& a );
	void V3LoadAsVec( Vector_4V& a, Vector_3 const& loc );
	void V3StoreAsVec( Vector_3& loc, Vector_4V const& a );
	void V2LoadAsVec( Vector_4V& a, Vector_2 const& loc );
	void V2StoreAsVec( Vector_2& loc, Vector_4V const& a );



	//-----------------------------------------------------
	// Vector_4V <--> Vector_4
	//-----------------------------------------------------

	// If in memory, this will load using float instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V4LoadAsScalar( Vector_4& a, Vector_4V const& loc )
	{
		a = *(Vector_4*)(&loc);
	}

	// If in memory, this will load using vector instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V4LoadAsVec( Vector_4V& a, Vector_4 const& loc )
	{
		a = *(Vector_4V*)(&loc);
	}

	// This will store to memory using float instructions.
	__forceinline
	void V4StoreAsScalar( Vector_4V& loc, Vector_4 const& a )
	{
		*(Vector_4*)(&loc) = a;
	}

	// This will store to memory using vector instructions.
	__forceinline
	void V4StoreAsVec( Vector_4& loc, Vector_4V const& a )
	{
		*(Vector_4V*)(&loc) = a;
	}

	//-----------------------------------------------------
	// Vector_4V <--> Vector_3
	//-----------------------------------------------------

	// If in memory, this will load using float instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V3LoadAsScalar( Vector_3& a, Vector_4V const& loc )
	{
		a = *(Vector_3*)(&loc);
	}

	// If in memory, this will load using vector instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V3LoadAsVec( Vector_4V& a, Vector_3 const& loc )
	{
		a = *(Vector_4V*)(&loc);
	}

	// This will store to memory using float instructions.
	__forceinline
	void V3StoreAsScalar( Vector_4V& loc, Vector_3 const& a )
	{
		*(Vector_3*)(&loc) = a;
	}

	// This will store to memory using Vector instructions.
	//__forceinline
	//void V3StoreAsVec( Vector_3& loc, Vector_4V const& a )
	//{
	//  // This would overwrite too much...
	//	*(Vector_4V*)(&loc) = a;
	//}

	//-----------------------------------------------------
	// Vector_4V <--> Vector_2
	//-----------------------------------------------------

	// If in memory, this will load using float instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V2LoadAsScalar( Vector_2& a, Vector_4V const& loc )
	{
		a = *(Vector_2*)(&loc);
	}

	// If in memory, this will load using vector instructions.
	// If in registers, this will incur one LHS.
	__forceinline
	void V2LoadAsVec( Vector_4V& a, Vector_2 const& loc )
	{
		a = *(Vector_4V*)(&loc);
	}

	// This will store to memory using float instructions.
	__forceinline
	void V2StoreAsScalar( Vector_4V& loc, Vector_2 const& a )
	{
		*(Vector_2*)(&loc) = a;
	}

	// This will store to memory using Vector instructions.
	//__forceinline
	//void V2StoreAsVec( Vector_2& loc, Vector_4V const& a )
	//{
	//  // This would overwrite too much...
	//	*(Vector_4V*)(&loc) = a;
	//}


}
}

#endif // VECTORMATH_VECTORMATH_H
