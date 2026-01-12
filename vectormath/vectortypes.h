#ifndef VECTORMATH_VECTORTYPES_H
#define VECTORMATH_VECTORTYPES_H

#include "channel.h"
#include "vectorconfig.h"
#include "data/struct.h"
#include "system/typeinfo.h"

#ifndef VECTORMATH_NO_VALIDATION
#define VecAssert(x)		mthAssertf(x, "VectorMath validation failed")
#define VecAssertMsg(x,y)	mthAssertf(x,y)
#else
#define VecAssert(x)
#define VecAssertMsg(x,y)
#endif

// Vector types.
//
// Note: These are the four options for use:
//
// -- Vector_2 (Scalar pipeline)
// -- Vector_3 (Scalar pipeline)
// -- Vector_4 (Scalar pipeline)
// -- Vector_4V (Vector pipeline)
//
// Thus, the developer determines what is vectorized and what isn't. Vector_4V will be vectorized where available.
//













//#if !UNIQUE_VECTORIZED_TYPE
//// In order for the scalar fallback to work with legacy RAGE code, we need to be able to convert
//// from a Vector3Param or Vector4Param to a Vector_4. Unfortunately, on Win32, Vector3Param and Vector4Param are not intrinsic types,
//// but are RAGE classes. Hence these #includes are needed.
//#if __SPU
//#include "vector/vector3_consts_spu.cpp"
//#endif
//#include "vector/vector4.h"
//#endif // !UNIQUE_VECTORIZED_TYPE


namespace rage
{
namespace Vec
{
	//=======================================================================================================================================
	//
	//
	//   PERMUTE CONSTANTS
	//
	//
	//=======================================================================================================================================

	// These constant values are needed by XBox360 and PS3, but they are converted to 0,1,2,3 values as needed
	// with a macro MASK_IT(x). See V4Permute<>() and V4PermuteTwo<>() implementations (PS3, XBox360, Win32PC, C++ scalar).

	// Non-extern'd consts.
	// http://www.ezdefinition.com/cgi-bin/showsubject.cgi?sid=296
	// Only used instead of #defines so they may be inside a namespace.

	// For rage::Vec::V4Permute<>() and rage::Vec*V::Get<>().
	const u32 X = 0x00010203;
	const u32 Y = 0x04050607;
	const u32 Z = 0x08090A0B;
	const u32 W = 0x0C0D0E0F;

	// For rage::Vec::V4PermuteTwo<>() and rage::GetFromTwo<>().
	const u32 X1 = 0x00010203;
	const u32 Y1 = 0x04050607;
	const u32 Z1 = 0x08090A0B;
	const u32 W1 = 0x0C0D0E0F;
	const u32 X2 = 0x10111213;
	const u32 Y2 = 0x14151617;
	const u32 Z2 = 0x18191A1B;
	const u32 W2 = 0x1C1D1E1F;
} // namespace Vec

	//=======================================================================================================================================
	//
	//
	//   FLOAT-AS-INT CONSTANTS
	//
	//
	//=======================================================================================================================================

	// Used as template parameters to V*Constant<...>() functions.
	const u32 U32_ZERO					= 0x0;
	const u32 U32_NEGZERO				= 0x80000000;
	const u32 U32_FLT_LARGE_2			= 0x42C80000;	// 1e2
	const u32 U32_FLT_LARGE_4			= 0x461C4000;	// 1e4
	const u32 U32_FLT_LARGE_6			= 0x49742400;	// 1e2
	const u32 U32_FLT_LARGE_8			= 0x4CBEBC20;	// 1e8
	const u32 U32_FLT_EPSILON			= 0x34000000;	// 1.19209e-7 = FLT_EPSILON
	const u32 U32_FLT_SMALL_12			= 0x2B8CBCCC;	// 1e-12
	const u32 U32_QUARTER				= 0x3E800000;
	const u32 U32_HALF					= 0x3F000000;
	const u32 U32_ONE					= 0x3F800000;
	const u32 U32_NEGONE				= 0xBF800000;
	const u32 U32_TWO					= 0x40000000;
	const u32 U32_ALLF					= 0xFFFFFFFF;
	const u32 U32_INF					= 0x7F800000;
	const u32 U32_NEGINF				= 0xFF800000;
	const u32 U32_NAN					= 0x7FC00000;
	const u32 U32_LOG2_TO_LOG10			= 0x3E9A209B;
	const u32 U32_ONE_OVER_PI			= 0x3EA2F983;
	const u32 U32_TWO_OVER_PI			= 0x3F22F983;
	const u32 U32_TO_RADIANS			= 0x3C8EFA35;
	const u32 U32_ONE_PLUS_EPSILON		= 0x3F800001;
	const u32 U32_THREE_PI_OVER_FOUR	= 0x4016CBE4;
	const u32 U32_ONE_OVER_1024			= 0x3A800000;
	const u32 U32_ALMOST_ONE			= 0x3F7FF972;
	const u32 U32_FLT_SMALL_9			= 0x3089705F;
	const u32 U32_FLT_SMALL_6			= 0x358637BD;
	const u32 U32_FLT_SMALL_5			= 0x3727C5AC;
	const u32 U32_FLT_SMALL_4			= 0x38D1B717;
	const u32 U32_FLT_SMALL_2			= 0x3C23D70A;
	const u32 U32_FLT_SMALL_1			= 0x3DCCCCCD;
	const u32 U32_ONE_MINUS_FLT_SMALL_3	= 0x3F7FBE77;
	const u32 U32_NEGHALF				= 0xBF000000;
	const u32 U32_POINT_SEVEN			= 0x3F333333;
	const u32 U32_FLT_MIN				= 0x00800000;

	// TODO: Check to make sure the following vals are the same as on SPU!
	const u32 U32_THREE					= 0x40400000;
	const u32 U32_FOUR					= 0x40800000;
	const u32 U32_FIVE					= 0x40a00000;
	const u32 U32_SIX					= 0x40c00000;
	const u32 U32_SEVEN					= 0x40e00000;
	const u32 U32_EIGHT					= 0x41000000;
	const u32 U32_NINE					= 0x41100000;
	const u32 U32_TEN					= 0x41200000;
	const u32 U32_ELEVEN				= 0x41300000;
	const u32 U32_TWELVE				= 0x41400000;
	const u32 U32_THIRTEEN				= 0x41500000;
	const u32 U32_FOURTEEN				= 0x41600000;
	const u32 U32_FIFTEEN				= 0x41700000;
	const u32 U32_NEGTWO				= 0xC0000000;
	const u32 U32_NEGTHREE				= 0xC0400000;
	const u32 U32_NEGFOUR				= 0xC0800000;
	const u32 U32_NEGFIVE				= 0xC0A00000;
	const u32 U32_NEGSIX				= 0xC0C00000;
	const u32 U32_NEGSEVEN				= 0xC0E00000;
	const u32 U32_NEGEIGHT				= 0xC1000000;
	const u32 U32_NEGNINE				= 0xC1100000;
	const u32 U32_NEGTEN				= 0xC1200000;
	const u32 U32_NEGELEVEN				= 0xC1300000;
	const u32 U32_NEGTWELVE				= 0xC1400000;
	const u32 U32_NEGTHIRTEEN			= 0xc1500000;
	const u32 U32_NEGFOURTEEN			= 0xC1600000;
	const u32 U32_NEGFIFTEEN			= 0xC1700000;
	const u32 U32_NEGSIXTEEN			= 0xC1800000;
	const u32 U32_NEG_FLT_MAX			= 0xFF7FFFFF;
	const u32 U32_ONE_OVER_255			= 0x3B808081;
	const u32 U32_255					= 0x437F0000;
	const u32 U32_SQRT_TWO				= 0x3FB504F3;
	const u32 U32_ONE_OVER_SQRT_TWO		= 0x3F3504F3;
	const u32 U32_SQRT_THREE			= 0x3FDDB3D7;
	const u32 U32_E						= 0x402DF854;

#if !__SPU
	const u32 U32_FLT_MAX				= 0x7F7FFFFF;
	const u32 U32_PI					= 0x40490FDB;
	const u32 U32_TWO_PI				= 0x40C90FDB;
	const u32 U32_PI_OVER_TWO			= 0x3FC90FDB;
	const u32 U32_NEG_PI				= 0xC0490FDB;
	const u32 U32_NEG_PI_OVER_TWO		= 0xBFC90FDB;
	const u32 U32_TO_DEGREES			= 0x42652EE1;	
	const u32 U32_PI_OVER_FOUR			= 0x3F490FDB;
	const u32 U32_FLT_SMALL_3			= 0x3A83126F; 	// 1e-3
	const u32 U32_THIRD					= 0x3EAAAAAB; 	// 0.333333333333333333333333333333333333333333333333333333333f
#else
	// For a given float constant, the hex is slightly differrent for some values on SPU.
	const u32 U32_FLT_MAX				= 0x7F7FFFFE;
	const u32 U32_PI					= 0x40490FDA;
	const u32 U32_TWO_PI				= 0x40C90FDA;
	const u32 U32_PI_OVER_TWO			= 0x3FC90FDA;
	const u32 U32_NEG_PI				= 0xC0490FDA;
	const u32 U32_NEG_PI_OVER_TWO		= 0xBFC90FDA;
	const u32 U32_TO_DEGREES			= 0x42652EE0;
	const u32 U32_PI_OVER_FOUR			= 0x3F490FDA;
	const u32 U32_FLT_SMALL_3			= 0x3A83126E; 	// 1e-3
	const u32 U32_THIRD					= 0x3EAAAAAA; 	// 0.333333333333333333333333333333333333333333333333333333333f
#endif

} // namespace rage

//=======================================================================================================================================
//
//
//   XBOX 360
//
//
//=======================================================================================================================================

#if __XENON

	#include <vectorintrinsics.h>

	namespace rage
	{
	namespace Vec
	{
#if UNIQUE_VECTORIZED_TYPE
		typedef __declspec(passinreg) __vector4 Vector_4V;
#else
		typedef __declspec(passinreg) __vector4 Vector_4V_Persistent;
#endif // UNIQUE_VECTORIZED_TYPE

		// typeless
		typedef Vector_4V Vector_4V_uint;
		typedef Vector_4V Vector_4V_int;
		typedef Vector_4V Vector_4V_ushort;
		typedef Vector_4V Vector_4V_short;
		typedef Vector_4V Vector_4V_char;
		typedef Vector_4V Vector_4V_uchar;
	}
	}

	#if UNIQUE_VECTORIZED_TYPE

	// Standard way to declare initializers.
	// NOTE: Use V4VConstant<>() instead, if you know the floats at compile-time!
	#define VECTOR4V_LITERAL(fX, fY, fZ, fW)		{ (fX), (fY), (fZ), (fW) }

	#endif // UNIQUE_VECTORIZED_TYPE

//=======================================================================================================================================
//
//
//   PS3
//
//
//=======================================================================================================================================

#elif (__PS3)

#if __PPU
	#include <altivec.h>
#elif __SPU
	#include <vmx2spu.h>
#endif

	

	namespace rage
	{
	namespace Vec
	{
#if UNIQUE_VECTORIZED_TYPE
#	if __SPU || defined(__SNC__)
#		define ATTRIBUTE_VECTOR vector
#	else
#		define ATTRIBUTE_VECTOR __attribute__((altivec(vector__)))
#	endif
		typedef ATTRIBUTE_VECTOR float Vector_4V;

		// For convenience. Only used internally.
		typedef ATTRIBUTE_VECTOR unsigned int Vector_4V_uint;
		typedef ATTRIBUTE_VECTOR signed int Vector_4V_int;
		typedef ATTRIBUTE_VECTOR unsigned short int Vector_4V_ushort;
		typedef ATTRIBUTE_VECTOR signed short int Vector_4V_short;
		typedef ATTRIBUTE_VECTOR signed char Vector_4V_char;
		typedef ATTRIBUTE_VECTOR unsigned char Vector_4V_uchar;
#else
#	if __SPU || defined(__SNC__)
#		define ATTRIBUTE_VECTOR vector
#	else
#		define ATTRIBUTE_VECTOR __attribute__((altivec(vector__)))
#	endif
		typedef ATTRIBUTE_VECTOR float Vector_4V_Persistent;
#endif // UNIQUE_VECTORIZED_TYPE
	}
	}

	#if UNIQUE_VECTORIZED_TYPE

	// Standard way to declare initializers.
	// NOTE: Use V4VConstant<>() instead, if you know the floats at compile-time!
#define VECTOR4V_LITERAL(fX, fY, fZ, fW)		(::rage::Vec::Vector_4V){ (fX), (fY), (fZ), (fW) }

	#endif // UNIQUE_VECTORIZED_TYPE



//=======================================================================================================================================
//
//
//   WIN32 PC
//
//
//=======================================================================================================================================

#elif RSG_CPU_INTEL

	// mmintrin.h for MMX
	// xmmintrin.h for SSE		<-- Doesn't collide with x87 FPU, unlike MMX.
	// emmintrin.h for SSE2		<-- Like SSE, but supports 128-bit MMX. Widely supported (P4 and up).
	// pmmintrin.h for SSE3		<-- Not widely supported, but potentially useful for horizontal instructions.
	// smmintrin.h for SSE4.1	<-- Not widely supported.

#if RSG_DURANGO || RSG_ORBIS
	#include <smmintrin.h> // Assume SSE4.1
#else
	#include <emmintrin.h> // Assume SSE2.
#endif

	namespace rage
	{
	namespace Vec
	{
#if UNIQUE_VECTORIZED_TYPE
		typedef __m128 Vector_4V;
#else
		typedef __m128 Vector_4V_Persistent;
#endif // UNIQUE_VECTORIZED_TYPE
	}
	}

	#if UNIQUE_VECTORIZED_TYPE

	// Standard way to declare initializers.
	// NOTE: Use V4VConstant<>() instead, if you know the floats at compile-time!
	#define VECTOR4V_LITERAL(fX, fY, fZ, fW)		{ (fX), (fY), (fZ), (fW) }
	// TODO: Maybe: __m128 _mm_setr_ps( fW , fZ , fY , fX ) ?

	#endif // UNIQUE_VECTORIZED_TYPE

#elif __PSP2

	#include <arm_neon.h>
	namespace rage
	{
	namespace Vec
	{
#if UNIQUE_VECTORIZED_TYPE
		typedef float32x4_t Vector_4V;
		typedef uint32x4_t Vector_4V_uint;
		typedef int32x4_t Vector_4V_int;
		typedef uint16x8_t Vector_4V_ushort;
		typedef int16x8_t Vector_4V_short;
		typedef int8x8_t Vector_4V_char;
		typedef uint8x8_t Vector_4V_uchar;
#else
		typedef float32x4_t Vector_4V_Persistent;
#endif

#if UNIQUE_VECTORIZED_TYPE
	#define VECTOR4V_LITERAL(fX, fY, fZ, fW)		{ (fX), (fY), (fZ), (fW) }
#endif

	}
	}

#endif // (platform)

	// Useful for initializing global const Vector_*'s with u32's at compile-time.
	namespace rage
	{
	namespace Vec
	{
		typedef ALIGNAS(16) u8 baVector_4[16] ;
		typedef ALIGNAS(16) u32 uaVector_4[4] ;
		typedef ALIGNAS(16) u32 uaVector_3[3] ;
		typedef ALIGNAS(8) u32 uaVector_2[2] ;
		typedef ALIGNAS(16) u32 ua32 ;
		typedef ALIGNAS(16) u16 ua16 ;
		typedef u8 bVector_4[16];
		typedef u32 uVector_4[4];
		typedef u32 uVector_3[3];
		typedef u32 uVector_2[2];
	}
	}



//=======================================================================================================================================
//
//
//   ALL
//
//
//=======================================================================================================================================

#if __DECLARESTRUCT
	namespace rage
	{
		// datSwapper() for 128-bit cross-platform vector-4 intrinsic typedef
#if UNIQUE_VECTORIZED_TYPE
		inline void datSwapper(Vec::Vector_4V& v)
		{
			float* p = reinterpret_cast<float*>(&v);
			datSwapper( p[0] );
			datSwapper( p[1] );
			datSwapper( p[2] );
			datSwapper( p[3] );
		}
#endif
	} // namespace rage
#endif // __DECLARESTRUCT

namespace rage
{
namespace Vec
{

	// Standard way to declare initializers.
	// NOTE: Use V*Constant<>() instead, if you know the floats at compile-time!
	#define VEC2_LITERAL(fX, fY)					::rage::Vec::Vector_2((fX), (fY))
	#define VEC3_LITERAL(fX, fY, fZ)				::rage::Vec::Vector_3((fX), (fY), (fZ))
	#define VEC4_LITERAL(fX, fY, fZ, fW)			::rage::Vec::Vector_4((fX), (fY), (fZ), (fW))

	// Scalar, as well as default.

	struct Vector_2
	{
		__forceinline Vector_2()
		{}
		__forceinline Vector_2(float _x, float _y)
			:x(_x),y(_y)
		{}
		__forceinline Vector_2& operator=( Vector_2 const& rhs )
		{
			x = rhs.x;
			y = rhs.y;
			return *this;
		}
		__forceinline Vector_2( Vector_2 const& v )
			:x(v.x),y(v.y)
		{}

		__forceinline operator float() const
		{
			return x;
		}
		float x, y;

		Vector_2(class datResource&) {}
		DECLARE_DUMMY_PLACE(Vector_2);
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vector_2);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_END();
		}
#endif
	};

	struct Vector_3	
	{
		__forceinline Vector_3()
		{}
		__forceinline Vector_3(float _x, float _y, float _z)
			:x(_x),y(_y),z(_z)
		{}
		__forceinline Vector_3& operator=( Vector_3 const& rhs )
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			return *this;
		}
		__forceinline Vector_3( Vector_3 const& v )
			:x(v.x),y(v.y),z(v.z)
		{}

		__forceinline operator float() const
		{
			return x;
		}
		float x, y, z;

		Vector_3(class datResource&) {}
		DECLARE_DUMMY_PLACE(Vector_3);
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vector_3);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_END();
		}
#endif
	};

	struct ALIGNAS(16) Vector_4
	{
		__forceinline Vector_4()
		{}
		__forceinline Vector_4(float _x, float _y, float _z, float _w)
			:x(_x),y(_y),z(_z),w(_w)
		{}
		__forceinline Vector_4& operator=( Vector_4 const& rhs )
		{
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			w = rhs.w;
			return *this;
		}
		__forceinline Vector_4( Vector_4 const& v )
			:x(v.x),y(v.y),z(v.z),w(v.w)
		{}
#if !UNIQUE_VECTORIZED_TYPE
		__forceinline Vector_4( Vector_4V_Persistent const& vec )
		{
			*this = *(Vec::Vector_4*)(&vec);
		}
#endif // !UNIQUE_VECTORIZED_TYPE

		__forceinline operator float() const
		{
			ASSERT_ONLY( union { float f; u32 u; } Temp1; Temp1.f = x; );
			ASSERT_ONLY( union { float f; u32 u; } Temp2; Temp2.f = y; );
			ASSERT_ONLY( union { float f; u32 u; } Temp3; Temp3.f = z; );
			ASSERT_ONLY( union { float f; u32 u; } Temp4; Temp4.f = w; );
			mthAssertf(	Temp1.u == Temp2.u && Temp1.u == Temp3.u && Temp1.u == Temp4.u,
						"Warning: Scalar rage::Vec::Vector_4V fallback may not work correctly since original vector was not splatted. This means we probably need to add a new fallback func that takes Vector_4_In instead of float." );
			return x;
		}

		float x, y, z, w;

		Vector_4(class datResource&) {}
		DECLARE_DUMMY_PLACE(Vector_4);
#if __DECLARESTRUCT
		void DeclareStruct(class datTypeStruct &s)
		{
			STRUCT_BEGIN(Vector_4);
			STRUCT_FIELD( x );
			STRUCT_FIELD( y );
			STRUCT_FIELD( z );
			STRUCT_FIELD( w );
			STRUCT_END();
		}
#endif
	} ;

	// This allows the scalar type to work in place of the vectorized type.
#if !(UNIQUE_VECTORIZED_TYPE)
	typedef Vector_4							Vector_4V;
	#define VECTOR4V_LITERAL(fX, fY, fZ, fW)		VEC4_LITERAL(fX, fY, fZ, fW)
	// Note: Even if we want all scalar types, we still have to handle the case of constructing a scalar type from a vectorized type.
	// 'Vector_4V_Persistent' (see far above) is a typedef that lets us do that.
#endif

	//================================================
	// Vector_4 (scalar) typedefs.
	//================================================

	// Parameter typedefs
	typedef Vector_4					Vector_4_Val;
	typedef Vector_4*					Vector_4_Ptr;
	typedef Vector_4&					Vector_4_Ref;
	typedef Vector_4 const&				Vector_4_ConstRef;
	typedef Vector_4 const*				Vector_4_ConstPtr;

#if __WIN32PC
	typedef Vector_4_ConstRef			Vector_4_In;
#else
	typedef Vector_4_Val				Vector_4_In;
#endif
	typedef Vector_4_Val				Vector_4_Out;
	typedef Vector_4_Ref				Vector_4_InOut;

	//================================================
	// Vector_4V (vector) typedefs.
	//================================================

	// Parameter typedefs
	typedef Vector_4V					Vector_4V_Val;
	typedef Vector_4V*					Vector_4V_Ptr;
	typedef Vector_4V&					Vector_4V_Ref;
	typedef Vector_4V const&			Vector_4V_ConstRef;
	typedef Vector_4V const*			Vector_4V_ConstPtr;

	// Win32-Specific: On Win32, vector arguments after the 3rd on not passed by register. And it won't even compile.
	// So use the following declaration for the 4th+ vector arguments in a given declaration.
	// (Note: you can rearrange the 3 allowed "Vector_4V_In" args however you want... the 3 vectors used in the earliest
	// calculations in a routine should be the 3 passed by register, ideally.)
#if __WIN32PC
	typedef Vector_4V_ConstRef			Vector_4V_In_After3Args;
#else
#	if UNIQUE_VECTORIZED_TYPE
	typedef Vector_4V_Val				Vector_4V_In_After3Args;
#	else
	typedef Vector_4V_ConstRef			Vector_4V_In_After3Args;
#	endif
#endif

#if UNIQUE_VECTORIZED_TYPE
	typedef Vector_4V_Val				Vector_4V_In;
#else
	typedef Vector_4V_ConstRef			Vector_4V_In;
#endif
	typedef Vector_4V_Val				Vector_4V_Out;
	typedef Vector_4V_Ref				Vector_4V_InOut;

	// These are for when you want to tell everyone your intrinsic parameters are meant to be interpreted as 2-tuples.
	typedef Vector_4V_In				V2Param128;
	typedef Vector_4V_In_After3Args		V2Param128_After3Args; // (Win32-specific - see above #if)
	typedef Vector_4V_Out				V2Return128;
	typedef Vector_4V_InOut				V2Ref128;
	// These are for when you want to tell everyone your intrinsic parameters are meant to be interpreted as 3-tuples. (A la the old rage::Vector3::Param.)
	typedef Vector_4V_In				V3Param128;
	typedef Vector_4V_In_After3Args		V3Param128_After3Args; // (Win32-specific - see above #if)
	typedef Vector_4V_Out				V3Return128;
	typedef Vector_4V_InOut				V3Ref128;
	// These are for when you want to tell everyone your intrinsic parameters are meant to be interpreted as 4-tuples.
	typedef Vector_4V_In				V4Param128;
	typedef Vector_4V_In_After3Args		V4Param128_After3Args; // (Win32-specific - see above #if)
	typedef Vector_4V_Out				V4Return128;
	typedef Vector_4V_InOut				V4Ref128;
	// These are for when you want to tell everyone your intrinsic parameters are meant to be interpreted as splatted 4-tuples.
	typedef Vector_4V_In				V4ParamSplatted128;
	typedef Vector_4V_In_After3Args		V4ParamSplatted128_After3Args; // (Win32-specific - see above #if)
	typedef Vector_4V_Out				V4ReturnSplatted128;
	typedef Vector_4V_InOut				V4RefSplatted128;

	//================================================
	// Vector_3 (scalar) typedefs.
	//================================================

	// Parameter typedefs
	typedef Vector_3					Vector_3_Val;
	typedef Vector_3*					Vector_3_Ptr;
	typedef Vector_3&					Vector_3_Ref;
	typedef Vector_3 const&				Vector_3_ConstRef;
	typedef Vector_3 const*				Vector_3_ConstPtr;

#if __WIN32PC
	typedef Vector_3_ConstRef			Vector_3_In;
#else
	typedef Vector_3_Val				Vector_3_In;
#endif
	typedef Vector_3_Val				Vector_3_Out;
	typedef Vector_3_Ref				Vector_3_InOut;

	//================================================
	// Vector_2 (scalar) typedefs.
	//================================================

	// Parameter typedefs
	typedef Vector_2					Vector_2_Val;
	typedef Vector_2*					Vector_2_Ptr;
	typedef Vector_2&					Vector_2_Ref;
	typedef Vector_2 const&				Vector_2_ConstRef;
	typedef Vector_2 const*				Vector_2_ConstPtr;

#if __WIN32PC
	typedef Vector_2_ConstRef			Vector_2_In;
#else
	typedef Vector_2_Val				Vector_2_In;
#endif
	typedef Vector_2_Val				Vector_2_Out;
	typedef Vector_2_Ref				Vector_2_InOut;

} // namespace Vec
} // namespace rage

//=======================================================================================================================================
//
//
//   VEC CONSTANT TYPES (Usage: Vec::V4VConstant(V_ZERO), e.g.)
//
//
//=======================================================================================================================================

namespace rage
{
	//	Decimal (or hex) values. Expanded decimal values from http://babbage.cs.qc.cuny.edu/IEEE-754/index.xhtml
	//	If only one value is listed, it gets splatted to all channels in the vector
	enum eZEROInitializer			{	V_ZERO, V_F_F_F_F, V_FALSE };	//	0.0
	enum eONEInitializer			{	V_ONE	};						//	1.0	
	enum eTWOInitializer			{	V_TWO	};						//	2.0	
	enum eTHREEInitializer			{	V_THREE	};						//	3.0	
	enum eFOURInitializer			{	V_FOUR	};						//	4.0	
	enum eFIVEInitializer			{	V_FIVE	};						//	5.0	
	enum eSIXInitializer			{	V_SIX	};						//	6.0	
	enum eSEVENInitializer			{	V_SEVEN	};						//	7.0	
	enum eEIGHTInitializer			{	V_EIGHT	};						//	8.0	
	enum eNINEInitializer			{	V_NINE	};						//	9.0	
	enum eTENInitializer			{	V_TEN	};						//	10.0
	enum eELEVENInitializer			{	V_ELEVEN	};					//	11.0
	enum eTWELVEInitializer			{	V_TWELVE	};					//	12.0
	enum eTHIRTEENInitializer		{	V_THIRTEEN	};					//	13.0
	enum eFOURTEENInitializer		{	V_FOURTEEN	};					//	14.0
	enum eFIFTEENInitializer		{	V_FIFTEEN	};					//	15.0
	enum eNEGONEInitializer			{	V_NEGONE	};					//	-1.0
	enum eNEGTWOInitializer			{	V_NEGTWO	};					//	-2.0
	enum eNEGTHREEInitializer		{	V_NEGTHREE	};					//	-3.0
	enum eNEGFOURInitializer		{	V_NEGFOUR	};					//	-4.0
	enum eNEGFIVEInitializer		{	V_NEGFIVE	};					//	-5.0
	enum eNEGSIXInitializer			{	V_NEGSIX	};					//	-6.0
	enum eNEGSEVENInitializer		{	V_NEGSEVEN	};					//	-7.0
	enum eNEGEIGHTInitializer		{	V_NEGEIGHT	};					//	-8.0
	enum eNEGNINEInitializer		{	V_NEGNINE	};					//	-9.0
	enum eNEGTENInitializer			{	V_NEGTEN	};					//	-10.0
	enum eNEGELEVENInitializer		{	V_NEGELEVEN	};					//	-11.0
	enum eNEGTWELVEInitializer		{	V_NEGTWELVE	};					//	-12.0
	enum eNEGTHIRTEENInitializer	{	V_NEGTHIRTEEN	};				//	-13.0
	enum eNEGFOURTEENInitializer	{	V_NEGFOURTEEN	};				//	-14.0
	enum eNEGFIFTEENInitializer		{	V_NEGFIFTEEN	};				//	-15.0
	enum eNEGSIXTEENInitializer		{	V_NEGSIXTEEN	};				//  -16.0

	enum eNEG_FLT_MAXInitializer			{	V_NEG_FLT_MAX			};	//	-3.4028234663852886e+38
	enum eFLT_MAXInitializer				{	V_FLT_MAX				};	//  3.4028234663852886e+38				(3.4028232635611926e+38 on SPU)
	enum eFLT_MINInitializer				{	V_FLT_MIN				};	//  1.1754943508222875e-38
	enum eFLT_LARGE_2Initializer			{	V_FLT_LARGE_2			};	//	1e2	
	enum eFLT_LARGE_4Initializer			{	V_FLT_LARGE_4			};	//	1e4	
	enum eFLT_LARGE_6Initializer			{	V_FLT_LARGE_6			};	//	1e6	
	enum eFLT_LARGE_8Initializer			{	V_FLT_LARGE_8			};	//	1e8	
	enum eFLT_EPSILONInitializer			{	V_FLT_EPSILON			};	//  1.1920928955078125e-7
	enum eFLT_SMALL_6Initializer			{	V_FLT_SMALL_6			};	//	1e-6	=	0.9999999974752427e-6
	enum eFLT_SMALL_5Initializer			{	V_FLT_SMALL_5			};	//	1e-5	=	0.000009999999747378752
	enum eFLT_SMALL_4Initializer			{	V_FLT_SMALL_4			};	//  1e-4	=	0.00009999999747378752
	enum eFLT_SMALL_3Initializer			{	V_FLT_SMALL_3			};	//	1e-3	=	0.0010000000474974513    (0.0009999999310821295 on SPU)
	enum eFLT_SMALL_2Initializer			{	V_FLT_SMALL_2			};	//  1e-2	=	0.009999999776482582
	enum eFLT_SMALL_1Initializer			{	V_FLT_SMALL_1			};	//  1e-1	=	0.10000000149011612
	enum eFLT_SMALL_12Initializer			{	V_FLT_SMALL_12			};	//	1e-12	=	0.9999999960041972e-12
	enum eONE_PLUS_EPSILONInitializer		{	V_ONE_PLUS_EPSILON		};	//	1.0000001192092896
	enum eONE_MINUS_FLT_SMALL_3Initializer	{   V_ONE_MINUS_FLT_SMALL_3	};	//	1.0 - 1e-3

	enum eZERO_WONEInitializer		{	V_ZERO_WONE							};	//	0.0		0.0		0.0		1.0
	enum eONE_WZEROInitializer		{	V_ONE_WZERO							};	//	1.0		1.0		1.0		0.0

	enum eX_AXIS_WONEInitializer	{	V_X_AXIS_WONE						};	//	1.0		0.0		0.0		1.0
	enum eY_AXIS_WONEInitializer	{	V_Y_AXIS_WONE						};	//	0.0		1.0		0.0		1.0
	enum eZ_AXIS_WONEInitializer	{	V_Z_AXIS_WONE, V_UP_AXIS_WONE		};	//	0.0		0.0		1.0		1.0

	enum eX_AXIS_WZEROInitializer	{	V_X_AXIS_WZERO						};	//	1.0		0.0		0.0		0.0
	enum eY_AXIS_WZEROInitializer	{	V_Y_AXIS_WZERO						};	//	0.0		1.0		0.0		0.0
	enum eZ_AXIS_WZEROInitializer	{	V_Z_AXIS_WZERO, V_UP_AXIS_WZERO		};	//	0.0		0.0		1.0		0.0

	enum eMASKXInitializer		{	V_MASKX, 	V_T_F_F_F					};	//  0xffffffff	0x00000000	0x00000000	0x00000000
	enum eMASKYInitializer		{	V_MASKY, 	V_F_T_F_F					};	//  0x00000000	0xffffffff	0x00000000	0x00000000
	enum eMASKZInitializer		{	V_MASKZ, 	V_F_F_T_F, V_MASK_UP_AXIS 	};	//	0x00000000	0x00000000	0xffffffff	0x00000000
	enum eMASKWInitializer		{	V_MASKW, 	V_F_F_F_T					};	//	0x00000000	0x00000000	0x00000000	0xffffffff
	enum eMASKXYInitializer		{	V_MASKXY, 	V_T_T_F_F, V_NOT_UP_AXIS_W	};	//  0xffffffff	0xffffffff	0x00000000	0x00000000
	enum eMASKXZInitializer		{	V_MASKXZ, 	V_T_F_T_F					};	//	0xffffffff	0x00000000	0xffffffff	0x00000000
	enum eMASKXWInitializer		{	V_MASKXW, 	V_T_F_F_T					};	//	0xffffffff	0x00000000	0x00000000	0xffffffff
	enum eMASKYZInitializer		{	V_MASKYZ, 	V_F_T_T_F					};	//	0x00000000	0xffffffff	0xffffffff	0x00000000
	enum eMASKYWInitializer		{	V_MASKYW, 	V_F_T_F_T					};	//	0x00000000	0xffffffff	0x00000000	0xffffffff
	enum eMASKZWInitializer		{	V_MASKZW, 	V_F_F_T_T, V_MASK_UP_AXIS_W	};	//	0x00000000	0x00000000	0xffffffff	0xffffffff
	enum eMASKYZWInitializer	{	V_MASKYZW, 	V_F_T_T_T					};	//	0x00000000	0xffffffff	0xffffffff	0xffffffff
	enum eMASKXZWInitializer	{	V_MASKXZW, 	V_T_F_T_T					};	//	0xffffffff	0x00000000	0xffffffff	0xffffffff
	enum eMASKXYWInitializer	{	V_MASKXYW, 	V_T_T_F_T, V_NOT_UP_AXIS	};	//	0xffffffff	0xffffffff	0x00000000	0xffffffff
	enum eMASKXYZInitializer	{	V_MASKXYZ, 	V_T_T_T_F					};	//	0xffffffff	0xffffffff	0xffffffff	0x00000000
	enum eMASKXYZWInitializer	{	V_MASKXYZW,	V_T_T_T_T, V_TRUE			};	//  0xffffffff	0xffffffff	0xffffffff	0xffffffff

	enum eQUARTERInitializer			{	V_QUARTER			};	//	1/4		=	0.25
	enum eTHIRDInitializer				{	V_THIRD				};	//	1/3		=	0.3333333432674408		(0.3333333134651184 on SPU)
	enum eHALFInitializer				{	V_HALF				};	//  1/2		=	0.5
	enum eNEGHALFInitializer			{	V_NEGHALF			};	//	-1/2	=	-0.5
	enum eINFInitializer				{	V_INF				};	//  +Inf
	enum eNEGINFInitializer				{	V_NEGINF			};	//  -Inf
	enum eNANInitializer				{	V_NAN				};	//  NaN
	enum eLOG2_TO_LOG10Initializer		{	V_LOG2_TO_LOG10		};	//	log(10.0)/log(2.0) = 0.3010300099849701

	enum eONE_OVER_1024Initializer		{	V_ONE_OVER_1024		};	//  1/1024	=	0.0009765625
	enum eONE_OVER_PIInitializer		{	V_ONE_OVER_PI		};	//  1/pi	=	0.31830987334251404
	enum eTWO_OVER_PIInitializer		{	V_TWO_OVER_PI		};	//	2/pi	=	0.6366197466850281
	enum ePIInitializer					{	V_PI				};	//	pi		=	3.1415927410125732		(3.141592502593994 on SPU)
	enum eTWO_PIInitializer				{	V_TWO_PI			};	//  2*pi	=	6.2831854820251465		(6.283185005187988 on SPU)
	enum ePI_OVER_TWOInitializer		{	V_PI_OVER_TWO		};	//	pi/2	=	1.5707963705062866		(1.570796251296997 on SPU)
	enum eNEG_PIInitializer				{	V_NEG_PI			};	//  -pi		=	-3.1415927410125732		(-3.141592502593994 on SPU)
	enum eNEG_PI_OVER_TWOInitializer	{	V_NEG_PI_OVER_TWO	};	//	-pi/2	=	-1.5707963705062866		(-1.570796251296997 on SPU)
	enum eTO_DEGREESInitializer			{	V_TO_DEGREES		};	//	180/pi	=	57.295780181884766		(57.2957763671875 on SPU)
	enum eTO_RADIANSInitializer			{	V_TO_RADIANS		};	//	pi/180	=	0.01745329238474369
	enum eSQRT_TWOInitializer			{	V_SQRT_TWO			};  // sqrt(2)	=	1.41421353816986083984375
	enum eONE_OVER_SQRT_TWOInitializer	{	V_ONE_OVER_SQRT_TWO	};	// 1/sqrt(2) =	0.7071067811865475
	enum eSQRT_THREEInitializer			{	V_SQRT_THREE		};	// sqrt(3)	=	1.73205077648162841796875
	enum eEInitializer					{	V_E					};	// e		=	2.71828174591064453125

	enum eINT_1Initializer		{	V_INT_1		};	//	0x00000001	
	enum eINT_2Initializer		{	V_INT_2		};	//	0x00000002	
	enum eINT_3Initializer		{	V_INT_3		};	//	0x00000003	
	enum eINT_4Initializer		{	V_INT_4		};	//	0x00000004	
	enum eINT_5Initializer		{	V_INT_5		};	//	0x00000005	
	enum eINT_6Initializer		{	V_INT_6		};	//	0x00000006	
	enum eINT_7Initializer		{	V_INT_7		};	//	0x00000007	
	enum eINT_8Initializer		{	V_INT_8		};	//	0x00000008	
	enum eINT_9Initializer		{	V_INT_9		};	//	0x00000009	
	enum eINT_10Initializer		{	V_INT_10	};	//	0x0000000a	
	enum eINT_11Initializer		{	V_INT_11	};	//	0x0000000b	
	enum eINT_12Initializer		{	V_INT_12	};	//	0x0000000c	
	enum eINT_13Initializer		{	V_INT_13	};	//	0x0000000d	
	enum eINT_14Initializer		{	V_INT_14	};	//	0x0000000e	
	enum eINT_15Initializer		{	V_INT_15	};	//	0x0000000f	

	enum e7FFFFFFFInitializer	{	V_7FFFFFFF	};	//  0x7fffffff
	enum e80000000Initializer	{	V_80000000	};	//	0x80000000

	// Special initializers for quaternions and matrices
	enum eIDENTITYInitializer	{	V_IDENTITY	};
	enum eROW_MAJORInitializer	{	V_ROW_MAJOR	};
	enum eCOL_MAJORInitializer	{	V_COL_MAJOR	};


namespace Vec
{
	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	const Vector_4V V4VConstant();

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	const Vector_4V V4VConstant();

	template <u32 x>
	__forceinline const Vector_4V V4VConstantSplat() { return V4VConstant<x,x,x,x>(); }

	// There is a V4VConstant override for all constant initializer types
	// (only eZEROInitializer shown here, but any eXXXInitializer will work)
	// Others are defined in vectortypes.inl
	const Vector_4V V4VConstant(eZEROInitializer);

	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	const Vector_4 V4Constant();

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	const Vector_4 V4Constant();

	// There is a V4Constant override for many constant initializer types
	// (only eZEROInitializer shown here, but many eXXXInitializers will work)
	// Others are defined in vectortypes.inl
	const Vector_4 V4Constant(eZEROInitializer);

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ>
	const Vector_3 V3Constant();

	// There is a V3Constant override for many constant initializer types
	// (only eZEROInitializer shown here, but many eXXXInitializers will work)
	// Others are defined in vectortypes.inl
	const Vector_3 V3Constant(eZEROInitializer);

	template <u32 floatAsIntX, u32 floatAsIntY>
	const Vector_2 V2Constant();

	// There is a V2Constant override for some constant initializer types
	// (only eZEROInitializer shown here, but some other eXXXInitializers will work)
	// Others are defined in vectortypes.inl
	const Vector_2 V2Constant(eZEROInitializer);


} // namespace Vec
} // namespace rage

#include "vectortypes.inl"

#endif // VECTORMATH_VECTORTYPES_H
