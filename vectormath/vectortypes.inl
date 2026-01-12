// TODO: Maybe reserve some regs for constants.

#ifdef __SNC__
#pragma diag_suppress 69	// This scares me but I can't see what's going wrong
#endif

#include "system/endian.h"

namespace rage
{
namespace Vec
{
	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	__forceinline const Vector_4V V4VConstant()
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE
		static const baVector_4 s_vect = { byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15 };
		return *(Vector_4V*)s_vect;
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
		static const Vector_4V s_vect = ((Vector_4V)((Vector_4V_uchar){ byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15 }));
		return s_vect;
#elif RSG_CPU_INTEL && UNIQUE_VECTORIZED_TYPE
		static const baVector_4 s_vect = { byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15 };
		return *(Vector_4V*)s_vect;
#else // !UNIQUE_VECTORIZED_TYPE
		return V4Constant<byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15>();
#endif
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	__forceinline const Vector_4V V4VConstant()
	{
#if __XENON && UNIQUE_VECTORIZED_TYPE
		static const uaVector_4 s_vect = { floatAsIntX, floatAsIntY, floatAsIntZ, floatAsIntW };
		return *(Vector_4V*)s_vect;
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
		static const Vector_4V s_vect = ((Vector_4V)((Vector_4V_uint){ floatAsIntX, floatAsIntY, floatAsIntZ, floatAsIntW }));
		return s_vect;
#elif RSG_CPU_INTEL && UNIQUE_VECTORIZED_TYPE
		static const uaVector_4 s_vect = { floatAsIntX, floatAsIntY, floatAsIntZ, floatAsIntW };
		return *(Vector_4V*)s_vect;
#elif __PSP2 && UNIQUE_VECTORIZED_TYPE
		static const Vector_4V s_vect = ((Vector_4V)((Vector_4V_uint){ floatAsIntX, floatAsIntY, floatAsIntZ, floatAsIntW }));
		return s_vect;
#else // !UNIQUE_VECTORIZED_TYPE
		return V4Constant<floatAsIntX,floatAsIntY,floatAsIntZ,floatAsIntW>();
#endif
	}

	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	__forceinline const Vector_4 V4Constant()
	{
		static const bVector_4 s_vect = { byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15 };
		return *union_cast<const Vector_4*>( &s_vect );
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	__forceinline const Vector_4 V4Constant()
	{
		static const uVector_4 s_vect = { floatAsIntX, floatAsIntY, floatAsIntZ, floatAsIntW };
		return *union_cast<const Vector_4*>( &s_vect );
	}

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ>
	__forceinline const Vector_3 V3Constant()
	{
		static const uVector_3 s_vect = { floatAsIntX, floatAsIntY, floatAsIntZ };
		return *union_cast<const Vector_3*>( &s_vect );
	}

	template <u32 floatAsIntX, u32 floatAsIntY>
	__forceinline const Vector_2 V2Constant()
	{
		static const uVector_2 s_vect = { floatAsIntX, floatAsIntY };
		return *union_cast<const Vector_2*>( &s_vect );
	}

	template <typename _T>
	__forceinline const Vector_4V V4VConstant()
	{
		VecAssertMsg( 0, "This is not a predefined constant!" );
		return V4VConstant(V_ZERO);
	}

	template <typename _T>
	__forceinline const Vector_4 V4Constant()
	{
		VecAssertMsg( 0, "This is not a predefined constant!" );
		return V4Constant(V_ZERO);
	}

	template <typename _T>
	__forceinline const Vector_3 V3Constant()
	{
		VecAssertMsg( 0, "This is not a predefined constant!" );
		return V3Constant(V_ZERO);
	}

	template <typename _T>
	__forceinline const Vector_2 V2Constant()
	{
		VecAssertMsg( 0, "This is not a predefined constant!" );
		return V2Constant(V_ZERO);
	}

	//================================================
	// Predefined Vector_4V Constants
	//================================================

	__forceinline const Vector_4V V4VConstant(eZEROInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(0);
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_splat_s32(0);
		//return (Vector_4V)(0.0f); // Another option. Sometimes generates a register xor w/iteself, sometimes a splatimmed (PSN). Dunno about SPU.
#elif UNIQUE_VECTORIZED_TYPE && RSG_CPU_INTEL
		return _mm_setzero_ps();
#else
		return V4VConstant<0,0,0,0>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_MAXInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_MAX );
#else
		return V4VConstant<U32_FLT_MAX,U32_FLT_MAX,U32_FLT_MAX,U32_FLT_MAX>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEG_FLT_MAXInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEG_FLT_MAX );
#else
		return V4VConstant<U32_NEG_FLT_MAX,U32_NEG_FLT_MAX,U32_NEG_FLT_MAX,U32_NEG_FLT_MAX>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_MINInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_MIN );
#else
		return V4VConstant<U32_FLT_MIN,U32_FLT_MIN,U32_FLT_MIN,U32_FLT_MIN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_LARGE_2Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_LARGE_2 );
#else
		// TODO: Can probably replace these with a similar call to 
		// __vcfux( __vspltisw(2), 31 ) or something...
		return V4VConstant<U32_FLT_LARGE_2,U32_FLT_LARGE_2,U32_FLT_LARGE_2,U32_FLT_LARGE_2>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_LARGE_4Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_LARGE_4 );
#else
		// TODO: Can probably replace these with a similar call to 
		// __vcfux( __vspltisw(2), 31 ) or something...
		return V4VConstant<U32_FLT_LARGE_4,U32_FLT_LARGE_4,U32_FLT_LARGE_4,U32_FLT_LARGE_4>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_LARGE_6Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_LARGE_6 );
#else
		// TODO: Can probably replace these with a similar call to 
		// __vcfux( __vspltisw(2), 31 ) or something...
		return V4VConstant<U32_FLT_LARGE_6,U32_FLT_LARGE_6,U32_FLT_LARGE_6,U32_FLT_LARGE_6>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_LARGE_8Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_LARGE_8 );
#else
		// TODO: Can probably replace these with a similar call to 
		// __vcfux( __vspltisw(2), 31 ) or something...
		return V4VConstant<U32_FLT_LARGE_8,U32_FLT_LARGE_8,U32_FLT_LARGE_8,U32_FLT_LARGE_8>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_EPSILONInitializer)
	{
		// 1.19209290e-007
		// Not a nice round number, but fast to generate!
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vslw( __vspltisw(13), __vspltisw(-6) );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_sl( (Vector_4V_uint)vec_splat_s32(13), (Vector_4V_uint)vec_splat_s32(-6) );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_EPSILON );
#else
		return V4VConstant<U32_FLT_EPSILON,U32_FLT_EPSILON,U32_FLT_EPSILON,U32_FLT_EPSILON>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_6Initializer)
	{
		// 1.0e-006
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_6 );
#else
		return V4VConstant<U32_FLT_SMALL_6,U32_FLT_SMALL_6,U32_FLT_SMALL_6,U32_FLT_SMALL_6>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_5Initializer)
	{
		// 1.0e-005
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_5 );
#else
		return V4VConstant<U32_FLT_SMALL_5,U32_FLT_SMALL_5,U32_FLT_SMALL_5,U32_FLT_SMALL_5>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_4Initializer)
	{
		// 1.0e-004
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_4 );
#else
		return V4VConstant<U32_FLT_SMALL_4,U32_FLT_SMALL_4,U32_FLT_SMALL_4,U32_FLT_SMALL_4>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_3Initializer)
	{
		// 1.0e-003
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_3 );
#else
		return V4VConstant<U32_FLT_SMALL_3,U32_FLT_SMALL_3,U32_FLT_SMALL_3,U32_FLT_SMALL_3>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_2Initializer)
	{
		// 1.0e-002
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_2 );
#else
		return V4VConstant<U32_FLT_SMALL_2,U32_FLT_SMALL_2,U32_FLT_SMALL_2,U32_FLT_SMALL_2>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_1Initializer)
	{
		// 1.0e-001
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_1 );
#else
		return V4VConstant<U32_FLT_SMALL_1,U32_FLT_SMALL_1,U32_FLT_SMALL_1,U32_FLT_SMALL_1>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONE_MINUS_FLT_SMALL_3Initializer)
	{
		// 0.999f
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE_MINUS_FLT_SMALL_3 );
#else
		return V4VConstant<U32_ONE_MINUS_FLT_SMALL_3,U32_ONE_MINUS_FLT_SMALL_3,U32_ONE_MINUS_FLT_SMALL_3,U32_ONE_MINUS_FLT_SMALL_3>();
#endif
	}


	__forceinline const Vector_4V V4VConstant(eONE_PLUS_EPSILONInitializer)
	{
		// A little over 1.0f
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vor( __vspltw(__vupkd3d(__vspltisw(0), VPACK_NORMSHORT2), 3), __vspltisw(1) );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		Vector_4V_uint INT_1 = (Vector_4V_uint)vec_splat_s32(1);
		return (Vector_4V)vec_or( (Vector_4V_uint)vec_ctf( INT_1, 0 ), INT_1 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE_PLUS_EPSILON );
#else
		return V4VConstant<U32_ONE_PLUS_EPSILON,U32_ONE_PLUS_EPSILON,U32_ONE_PLUS_EPSILON,U32_ONE_PLUS_EPSILON>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFLT_SMALL_12Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FLT_SMALL_12 );
#else
		return V4VConstant<U32_FLT_SMALL_12,U32_FLT_SMALL_12,U32_FLT_SMALL_12,U32_FLT_SMALL_12>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eZERO_WONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xAB );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(0), (Vector_4V_int)vec_ctf( vec_splat_s32(1), 0 ), 0x4 );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ZERO,U32_ONE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(2), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(2), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TWO );
#elif 0//UNIQUE_VECTORIZED_TYPE && RSG_CPU_INTEL
		return _mm_set_ps1( 2.0f );
#else
		return V4VConstant<U32_TWO,U32_TWO,U32_TWO,U32_TWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTHREEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(3), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(3), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_THREE );
#elif 0//UNIQUE_VECTORIZED_TYPE && RSG_CPU_INTEL
		return _mm_set_ps1( 3.0f );
#else
		return V4VConstant<U32_THREE,U32_THREE,U32_THREE,U32_THREE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFOURInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(4), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(4), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FOUR );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 4.0f );
#else
		return V4VConstant<U32_FOUR,U32_FOUR,U32_FOUR,U32_FOUR>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFIVEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(5), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(5), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FIVE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 5.0f );
#else
		return V4VConstant<U32_FIVE,U32_FIVE,U32_FIVE,U32_FIVE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eSIXInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(6), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(6), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_SIX );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 6.0f );
#else
		return V4VConstant<U32_SIX,U32_SIX,U32_SIX,U32_SIX>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eSEVENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(7), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(7), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_SEVEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 7.0f );
#else
		return V4VConstant<U32_SEVEN,U32_SEVEN,U32_SEVEN,U32_SEVEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eEIGHTInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(8), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(8), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_EIGHT );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 8.0f );
#else
		return V4VConstant<U32_EIGHT,U32_EIGHT,U32_EIGHT,U32_EIGHT>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNINEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(9), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(9), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NINE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 9.0f );
#else
		return V4VConstant<U32_NINE,U32_NINE,U32_NINE,U32_NINE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(10), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(10), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 10.0f );
#else
		return V4VConstant<U32_TEN,U32_TEN,U32_TEN,U32_TEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eELEVENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(11), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(11), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ELEVEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 11.0f );
#else
		return V4VConstant<U32_ELEVEN,U32_ELEVEN,U32_ELEVEN,U32_ELEVEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTWELVEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(12), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(12), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TWELVE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 12.0f );
#else
		return V4VConstant<U32_TWELVE,U32_TWELVE,U32_TWELVE,U32_TWELVE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTHIRTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(13), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(13), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_THIRTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 13.0f );
#else
		return V4VConstant<U32_THIRTEEN,U32_THIRTEEN,U32_THIRTEEN,U32_THIRTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFOURTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(14), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(14), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FOURTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 14.0f );
#else
		return V4VConstant<U32_FOURTEEN,U32_FOURTEEN,U32_FOURTEEN,U32_FOURTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eFIFTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(15), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(15), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_FIFTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 15.0f );
#else
		return V4VConstant<U32_FIFTEEN,U32_FIFTEEN,U32_FIFTEEN,U32_FIFTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eX_AXIS_WONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xEB );
#else
		return V4VConstant<U32_ONE,U32_ZERO,U32_ZERO,U32_ONE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eY_AXIS_WONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xBB );
#else
		return V4VConstant<U32_ZERO,U32_ONE,U32_ZERO,U32_ONE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eZ_AXIS_WONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xAF );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(0), (Vector_4V_int)vec_ctf( vec_splat_s32(1), 0 ), 0x8 );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ONE,U32_ONE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eX_AXIS_WZEROInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xEA );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( (Vector_4V_int)vec_ctf( vec_splat_s32(1), 0 ), vec_splat_s32(0), 0xC );
#else
		return V4VConstant<U32_ONE,U32_ZERO,U32_ZERO,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eY_AXIS_WZEROInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xBA );
#else
		return V4VConstant<U32_ZERO,U32_ONE,U32_ZERO,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eZ_AXIS_WZEROInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V zeroInZ_oneInW = __vupkd3d(__vspltisw(0), VPACK_NORMSHORT2);
		return __vpermwi( zeroInZ_oneInW, 0xAE );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ONE,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKYZWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_32, 0x3 );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(0), vec_splat_s32(-1), 0xC );
#else
		return V4VConstant<U32_ZERO,U32_ALLF,U32_ALLF,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXZWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_32, 0x2 );
#else
		return V4VConstant<U32_ALLF,U32_ZERO,U32_ALLF,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXYWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_32, 0x1 );
#else
		return V4VConstant<U32_ALLF,U32_ALLF,U32_ZERO,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXYZInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_32, 0x0 );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(-1), vec_splat_s32(0), 0x4 );
#else
		return V4VConstant<U32_ALLF,U32_ALLF,U32_ALLF,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vsldoi( __vspltisw(-1), __vspltisw(0), 0xC );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(-1), vec_splat_s32(0), 0xC );
#else
		return V4VConstant<U32_ALLF,U32_ZERO,U32_ZERO,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKYInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vrlimi( __vspltisw(0), __vspltisw(-1), 0x4, 0x0 );
#else
		return V4VConstant<U32_ZERO,U32_ALLF,U32_ZERO,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKZInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vrlimi( __vspltisw(0), __vspltisw(-1), 0x2, 0x0 );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ALLF,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vsldoi( __vspltisw(0), __vspltisw(-1), 0x4 );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(0), vec_splat_s32(-1), 0x4 );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ZERO,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXYInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_64HI, 0x0 );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(-1), vec_splat_s32(0), 0x8 );
#else
		return V4VConstant<U32_ALLF,U32_ALLF,U32_ZERO,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXZInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vrlimi( __vspltisw(0), __vspltisw(-1), 0xA, 0x0 );
#else
		return V4VConstant<U32_ALLF,U32_ZERO,U32_ALLF,0>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_64LO, 0x1 );
#else
		return V4VConstant<U32_ALLF,U32_ZERO,U32_ZERO,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKYZInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vrlimi( __vspltisw(0), __vspltisw(-1), 0x6, 0x0 );
#else
		return V4VConstant<U32_ZERO,U32_ALLF,U32_ALLF,U32_ZERO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKYWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vrlimi( __vspltisw(0), __vspltisw(-1), 0x5, 0x0 );
#else
		return V4VConstant<U32_ZERO,U32_ALLF,U32_ZERO,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKZWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		Vector_4V _ffffffff = __vspltisw(-1);
		return __vpkd3d( _ffffffff, _ffffffff, VPACK_D3DCOLOR, VPACK_64LO, 0x2 );
#elif UNIQUE_VECTORIZED_TYPE && __PS3
		return (Vector_4V)vec_sld( vec_splat_s32(0), vec_splat_s32(-1), 0x8 );
#else
		return V4VConstant<U32_ZERO,U32_ZERO,U32_ALLF,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltw(__vupkd3d(__vspltisw(0), VPACK_NORMSHORT2), 3);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(1), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		// movss followed by "splat_x" on Win32... at least saves some space in the data segment over the next option
		return _mm_set_ps1( 1.0f );
#else
		// movaps on Win32
		return V4VConstant<U32_ONE,U32_ONE,U32_ONE,U32_ONE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGONEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-1), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-1), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGONE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -1.0f );
#else
		return V4VConstant<U32_NEGONE,U32_NEGONE,U32_NEGONE,U32_NEGONE>();
#endif
	}


	__forceinline const Vector_4V V4VConstant(eNEGTWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-2), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-2), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGTWO );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -2.0f );
#else
		return V4VConstant<U32_NEGTWO,U32_NEGTWO,U32_NEGTWO,U32_NEGTWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGTHREEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-3), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-3), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGTHREE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -3.0f );
#else
		return V4VConstant<U32_NEGTHREE,U32_NEGTHREE,U32_NEGTHREE,U32_NEGTHREE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGFOURInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-4), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-4), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGFOUR );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -4.0f );
#else
		return V4VConstant<U32_NEGFOUR,U32_NEGFOUR,U32_NEGFOUR,U32_NEGFOUR>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGFIVEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-5), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-5), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGFIVE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -5.0f );
#else
		return V4VConstant<U32_NEGFIVE,U32_NEGFIVE,U32_NEGFIVE,U32_NEGFIVE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGSIXInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-6), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-6), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGSIX );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -6.0f );
#else
		return V4VConstant<U32_NEGSIX,U32_NEGSIX,U32_NEGSIX,U32_NEGSIX>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGSEVENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-7), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-7), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGSEVEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -7.0f );
#else
		return V4VConstant<U32_NEGSEVEN,U32_NEGSEVEN,U32_NEGSEVEN,U32_NEGSEVEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGEIGHTInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-8), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-8), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGEIGHT );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -8.0f );
#else
		return V4VConstant<U32_NEGEIGHT,U32_NEGEIGHT,U32_NEGEIGHT,U32_NEGEIGHT>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGNINEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-9), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-9), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGNINE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -9.0f );
#else
		return V4VConstant<U32_NEGNINE,U32_NEGNINE,U32_NEGNINE,U32_NEGNINE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGTENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-10), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-10), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGTEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -10.0f );
#else
		return V4VConstant<U32_NEGTEN,U32_NEGTEN,U32_NEGTEN,U32_NEGTEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGELEVENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-11), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-11), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGELEVEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -11.0f );
#else
		return V4VConstant<U32_NEGELEVEN,U32_NEGELEVEN,U32_NEGELEVEN,U32_NEGELEVEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGTWELVEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-12), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-12), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGTWELVE );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -12.0f );
#else
		return V4VConstant<U32_NEGTWELVE,U32_NEGTWELVE,U32_NEGTWELVE,U32_NEGTWELVE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGTHIRTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-13), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-13), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGTHIRTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -13.0f );
#else
		return V4VConstant<U32_NEGTHIRTEEN,U32_NEGTHIRTEEN,U32_NEGTHIRTEEN,U32_NEGTHIRTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGFOURTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-14), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-14), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGFOURTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -14.0f );
#else
		return V4VConstant<U32_NEGFOURTEEN,U32_NEGFOURTEEN,U32_NEGFOURTEEN,U32_NEGFOURTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGFIFTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-15), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-15), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGFIFTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -15.0f );
#else
		return V4VConstant<U32_NEGFIFTEEN,U32_NEGFIFTEEN,U32_NEGFIFTEEN,U32_NEGFIFTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGSIXTEENInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-16), 0 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-16), 0 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGSIXTEEN );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -16.0f );
#else
		return V4VConstant<U32_NEGSIXTEEN,U32_NEGSIXTEEN,U32_NEGSIXTEEN,U32_NEGSIXTEEN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGHALFInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(-1), 1 ) ;
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(-1), 1 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGHALF );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( -0.5f );
#else
		return V4VConstant<U32_NEGHALF,U32_NEGHALF,U32_NEGHALF,U32_NEGHALF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONE_WZEROInitializer)
	{
		return V4VConstant<U32_ONE,U32_ONE,U32_ONE,U32_ZERO>();
	}

	__forceinline const Vector_4V V4VConstant(eHALFInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(1), 1 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(1), 1 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_HALF );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 0.5f );
#else
		return V4VConstant<U32_HALF,U32_HALF,U32_HALF,U32_HALF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTHIRDInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_THIRD );
#else
		return V4VConstant<U32_THIRD,U32_THIRD,U32_THIRD,U32_THIRD>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eQUARTERInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfux( __vspltisw(1), 2 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(1), 2 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_QUARTER );
#elif 0//UNIQUE_VECTORIZED_TYPE && __WIN32PC
		return _mm_set_ps1( 0.25f );
#else
		return V4VConstant<U32_QUARTER,U32_QUARTER,U32_QUARTER,U32_QUARTER>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINFInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_INF );
#else
		return V4VConstant<U32_INF,U32_INF,U32_INF,U32_INF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEGINFInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEGINF );
#else
		return V4VConstant<U32_NEGINF,U32_NEGINF,U32_NEGINF,U32_NEGINF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNANInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NAN );
#else
		return V4VConstant<U32_NAN,U32_NAN,U32_NAN,U32_NAN>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eLOG2_TO_LOG10Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_LOG2_TO_LOG10 );
#else
		return V4VConstant<U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(e7FFFFFFFInitializer)
	{
		// TODO: POSSIBILY SLOWER THAN LOADING FROM MEMORY..
//#if UNIQUE_VECTORIZED_TYPE && __XENON
//		// U32_ALLF FFFFFFFF FFFFFFFF FFFFFFFF
//		Vector_4V signBitVect = __vspltisw(-1);
//		// 0x80000000 80000000 80000000 80000000
//		signBitVect = __vslw( signBitVect, signBitVect );
//		// 0x7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF.
//		return __vnor( signBitVect, signBitVect );
//#elif UNIQUE_VECTORIZED_TYPE && __PS3
//		// U32_ALLF FFFFFFFF FFFFFFFF FFFFFFFF
//		Vector_4V_uint signBitVect = (Vector_4V_uint)vec_splat_s32(-1);
//		// 0x80000000 80000000 80000000 80000000
//		signBitVect = vec_sl( signBitVect, signBitVect );
//		// 0x7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF.
//		return (Vector_4V)vec_nor( signBitVect, signBitVect );
//#else
//#endif

#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x7FFFFFFF );
#else
		return V4VConstant<0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(e80000000Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		// 0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
		Vector_4V signBitVect = __vspltisw(-1);
		// 0x80000000 80000000 80000000 80000000
		return __vslw( signBitVect, signBitVect );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		// 0xFFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
		Vector_4V_uint signBitVect = (Vector_4V_uint)vec_splat_s32(-1);
		// 0x80000000 80000000 80000000 80000000
		return (Vector_4V)vec_sl( signBitVect, signBitVect );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x80000000 );
#else
		return V4VConstant<0x80000000,0x80000000,0x80000000,0x80000000>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eMASKXYZWInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(-1);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(-1);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ALLF );
#elif UNIQUE_VECTORIZED_TYPE && RSG_CPU_INTEL
		// This first initialization shouldn't be necessary, but the
		// compiler won't ignore initilization, and inline asm won't
		// optimize to one instruction...
		Vector_4V a = _mm_setzero_ps();
		return _mm_cmpeq_ps( a, a );
#else
		return V4VConstant<U32_ALLF,U32_ALLF,U32_ALLF,U32_ALLF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_1Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(1);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(1);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x1 );
#elif UNIQUE_VECTORIZED_TYPE && RSG_CPU_INTEL
		Vector_4V zero = _mm_setzero_ps();
		Vector_4V neg1 = _mm_cmpeq_ps(zero,zero);
		__m128i tempA = *(__m128i*)(&zero);
		__m128i tempB = *(__m128i*)(&neg1);
		__m128i tempDiff = _mm_sub_epi32( tempA, tempB );
		return *(Vector_4V*)(&tempDiff);
#else
		return V4VConstant<0x1,0x1,0x1,0x1>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_2Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(2);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(2);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x2 );
#else
		return V4VConstant<0x2,0x2,0x2,0x2>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_3Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(3);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(3);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x3 );
#else
		return V4VConstant<0x3,0x3,0x3,0x3>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_4Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(4);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(4);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x4 );
#else
		return V4VConstant<0x4,0x4,0x4,0x4>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_5Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(5);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(5);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x5 );
#else
		return V4VConstant<0x5,0x5,0x5,0x5>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_6Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(6);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(6);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x6 );
#else
		return V4VConstant<0x6,0x6,0x6,0x6>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_7Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(7);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(7);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x7 );
#else
		return V4VConstant<0x7,0x7,0x7,0x7>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_8Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(8);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(8);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x8 );
#else
		return V4VConstant<0x8,0x8,0x8,0x8>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_9Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(9);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(9);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0x9 );
#else
		return V4VConstant<0x9,0x9,0x9,0x9>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_10Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(10);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(10);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xA );
#else
		return V4VConstant<0xA,0xA,0xA,0xA>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_11Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(11);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(11);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xB );
#else
		return V4VConstant<0xB,0xB,0xB,0xB>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_12Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(12);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(12);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xC );
#else
		return V4VConstant<0xC,0xC,0xC,0xC>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_13Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(13);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(13);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xD );
#else
		return V4VConstant<0xD,0xD,0xD,0xD>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_14Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(14);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(14);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xE );
#else
		return V4VConstant<0xE,0xE,0xE,0xE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eINT_15Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vspltisw(15);
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return (Vector_4V)vec_splat_s32(15);
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)0xF );
#else
		return V4VConstant<0xF,0xF,0xF,0xF>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONE_OVER_1024Initializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __XENON
		return __vcfsx( __vspltisw(1), 10 );
#elif UNIQUE_VECTORIZED_TYPE && __PPU
		return vec_ctf( vec_splat_s32(1), 10 );
#elif UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE_OVER_1024 );
#else
		return V4VConstant<U32_ONE_OVER_1024,U32_ONE_OVER_1024,U32_ONE_OVER_1024,U32_ONE_OVER_1024>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONE_OVER_PIInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE_OVER_PI );
#else
		return V4VConstant<U32_ONE_OVER_PI,U32_ONE_OVER_PI,U32_ONE_OVER_PI,U32_ONE_OVER_PI>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTWO_OVER_PIInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TWO_OVER_PI );
#else
		return V4VConstant<U32_TWO_OVER_PI,U32_TWO_OVER_PI,U32_TWO_OVER_PI,U32_TWO_OVER_PI>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(ePIInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_PI );
#else
		return V4VConstant<U32_PI,U32_PI,U32_PI,U32_PI>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTWO_PIInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TWO_PI );
#else
		return V4VConstant<U32_TWO_PI,U32_TWO_PI,U32_TWO_PI,U32_TWO_PI>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(ePI_OVER_TWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_PI_OVER_TWO );
#else
		return V4VConstant<U32_PI_OVER_TWO,U32_PI_OVER_TWO,U32_PI_OVER_TWO,U32_PI_OVER_TWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEG_PIInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEG_PI );
#else
		return V4VConstant<U32_NEG_PI,U32_NEG_PI,U32_NEG_PI,U32_NEG_PI>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eNEG_PI_OVER_TWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_NEG_PI_OVER_TWO );
#else
		return V4VConstant<U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTO_DEGREESInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TO_DEGREES ); // Note the different pattern!
#else
		return V4VConstant<U32_TO_DEGREES,U32_TO_DEGREES,U32_TO_DEGREES,U32_TO_DEGREES>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eTO_RADIANSInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_TO_RADIANS );
#else
		return V4VConstant<U32_TO_RADIANS,U32_TO_RADIANS,U32_TO_RADIANS,U32_TO_RADIANS>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eSQRT_TWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_SQRT_TWO );
#else
		return V4VConstant<U32_SQRT_TWO,U32_SQRT_TWO,U32_SQRT_TWO,U32_SQRT_TWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eONE_OVER_SQRT_TWOInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_ONE_OVER_SQRT_TWO );
#else
		return V4VConstant<U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eSQRT_THREEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_SQRT_THREE );
#else
		return V4VConstant<U32_SQRT_THREE,U32_SQRT_THREE,U32_SQRT_THREE,U32_SQRT_THREE>();
#endif
	}

	__forceinline const Vector_4V V4VConstant(eEInitializer)
	{
#if UNIQUE_VECTORIZED_TYPE && __SPU
		return (Vector_4V)spu_splats( (int)U32_E );
#else
		return V4VConstant<U32_E,U32_E,U32_E,U32_E>();
#endif
	}


	//================================================
	// Predefined Vector_4 Constants
	//================================================

	__forceinline const Vector_4 V4Constant(eZEROInitializer)
	{
		return V4Constant<0,0,0,0>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_MAXInitializer)
	{
		return V4Constant<U32_FLT_MAX,U32_FLT_MAX,U32_FLT_MAX,U32_FLT_MAX>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_LARGE_2Initializer)
	{
		return V4Constant<U32_FLT_LARGE_2,U32_FLT_LARGE_2,U32_FLT_LARGE_2,U32_FLT_LARGE_2>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_LARGE_4Initializer)
	{
		return V4Constant<U32_FLT_LARGE_4,U32_FLT_LARGE_4,U32_FLT_LARGE_4,U32_FLT_LARGE_4>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_LARGE_6Initializer)
	{
		return V4Constant<U32_FLT_LARGE_6,U32_FLT_LARGE_6,U32_FLT_LARGE_6,U32_FLT_LARGE_6>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_LARGE_8Initializer)
	{
		return V4Constant<U32_FLT_LARGE_8,U32_FLT_LARGE_8,U32_FLT_LARGE_8,U32_FLT_LARGE_8>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_EPSILONInitializer)
	{
		return V4Constant<U32_FLT_EPSILON,U32_FLT_EPSILON,U32_FLT_EPSILON,U32_FLT_EPSILON>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_SMALL_6Initializer)
	{
		return V4Constant<U32_FLT_SMALL_6,U32_FLT_SMALL_6,U32_FLT_SMALL_6,U32_FLT_SMALL_6>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_SMALL_5Initializer)
	{
		return V4Constant<U32_FLT_SMALL_5,U32_FLT_SMALL_5,U32_FLT_SMALL_5,U32_FLT_SMALL_5>();
	}

	__forceinline const Vector_4 V4Constant(eFLT_SMALL_12Initializer)
	{
		return V4Constant<U32_FLT_SMALL_12,U32_FLT_SMALL_12,U32_FLT_SMALL_12,U32_FLT_SMALL_12>();
	}

	__forceinline const Vector_4 V4Constant(eZERO_WONEInitializer)
	{
		return V4Constant<U32_ZERO,U32_ZERO,U32_ZERO,U32_ONE>();
	}

	__forceinline const Vector_4 V4Constant(eX_AXIS_WONEInitializer)
	{
		return V4Constant<U32_ONE,U32_ZERO,U32_ZERO,U32_ONE>();
	}

	__forceinline const Vector_4 V4Constant(eY_AXIS_WONEInitializer)
	{
		return V4Constant<U32_ZERO,U32_ONE,U32_ZERO,U32_ONE>();
	}

	__forceinline const Vector_4 V4Constant(eZ_AXIS_WONEInitializer)
	{
		return V4Constant<U32_ZERO,U32_ZERO,U32_ONE,U32_ONE>();
	}

	__forceinline const Vector_4 V4Constant(eMASKYZWInitializer)
	{
		return V4Constant<U32_ZERO,U32_ALLF,U32_ALLF,U32_ALLF>();
	}

	__forceinline const Vector_4 V4Constant(eMASKXZWInitializer)
	{
		return V4Constant<U32_ALLF,U32_ZERO,U32_ALLF,U32_ALLF>();
	}

	__forceinline const Vector_4 V4Constant(eMASKXYWInitializer)
	{
		return V4Constant<U32_ALLF,U32_ALLF,U32_ZERO,U32_ALLF>();
	}

	__forceinline const Vector_4 V4Constant(eMASKXYZInitializer)
	{
		return V4Constant<U32_ALLF,U32_ALLF,U32_ALLF,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eMASKXInitializer)
	{
		return V4Constant<U32_ALLF,U32_ZERO,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eMASKYInitializer)
	{
		return V4Constant<U32_ZERO,U32_ALLF,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eMASKZInitializer)
	{
		return V4Constant<U32_ZERO,U32_ZERO,U32_ALLF,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eMASKWInitializer)
	{
		return V4Constant<U32_ZERO,U32_ZERO,U32_ZERO,U32_ALLF>();
	}

	__forceinline const Vector_4 V4Constant(eONEInitializer)
	{
		return V4Constant<U32_ONE,U32_ONE,U32_ONE,U32_ONE>();
	}

	__forceinline const Vector_4 V4Constant(eONE_WZEROInitializer)
	{
		return V4Constant<U32_ONE,U32_ONE,U32_ONE,0>();
	}

	__forceinline const Vector_4 V4Constant(eHALFInitializer)
	{
		return V4Constant<U32_HALF,U32_HALF,U32_HALF,U32_HALF>();
	}

	__forceinline const Vector_4 V4Constant(eINFInitializer)
	{
		return V4Constant<U32_INF,U32_INF,U32_INF,U32_INF>();
	}

	__forceinline const Vector_4 V4Constant(eNEGINFInitializer)
	{
		return V4Constant<U32_NEGINF,U32_NEGINF,U32_NEGINF,U32_NEGINF>();
	}

	__forceinline const Vector_4 V4Constant(eNEGONEInitializer)
	{
		return V4Constant<U32_NEGONE,U32_NEGONE,U32_NEGONE,U32_NEGONE>();
	}

	__forceinline const Vector_4 V4Constant(eNANInitializer)
	{
		return V4Constant<U32_NAN,U32_NAN,U32_NAN,U32_NAN>();
	}

	__forceinline const Vector_4 V4Constant(eLOG2_TO_LOG10Initializer)
	{
		return V4Constant<U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10>();
	}

	__forceinline const Vector_4 V4Constant(e7FFFFFFFInitializer)
	{
		return V4Constant<0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF>();
	}

	__forceinline const Vector_4 V4Constant(e80000000Initializer)
	{
		return V4Constant<0x80000000,0x80000000,0x80000000,0x80000000>();
	}

	__forceinline const Vector_4 V4Constant(eMASKXYZWInitializer)
	{
		return V4Constant<U32_ALLF,U32_ALLF,U32_ALLF,U32_ALLF>();
	}

	__forceinline const Vector_4 V4Constant(eX_AXIS_WZEROInitializer)
	{
		return V4Constant<U32_ONE,U32_ZERO,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eY_AXIS_WZEROInitializer)
	{
		return V4Constant<U32_ZERO,U32_ONE,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eZ_AXIS_WZEROInitializer)
	{
		return V4Constant<U32_ZERO,U32_ZERO,U32_ONE,U32_ZERO>();
	}

	__forceinline const Vector_4 V4Constant(eONE_OVER_PIInitializer)
	{
		return V4Constant<U32_ONE_OVER_PI,U32_ONE_OVER_PI,U32_ONE_OVER_PI,U32_ONE_OVER_PI>();
	}

	__forceinline const Vector_4 V4Constant(eTWO_OVER_PIInitializer)
	{
		return V4Constant<U32_TWO_OVER_PI,U32_TWO_OVER_PI,U32_TWO_OVER_PI,U32_TWO_OVER_PI>();
	}

	__forceinline const Vector_4 V4Constant(ePIInitializer)
	{
		return V4Constant<U32_PI,U32_PI,U32_PI,U32_PI>();
	}

	__forceinline const Vector_4 V4Constant(eTWO_PIInitializer)
	{
		return V4Constant<U32_TWO_PI,U32_TWO_PI,U32_TWO_PI,U32_TWO_PI>();
	}

	__forceinline const Vector_4 V4Constant(ePI_OVER_TWOInitializer)
	{
		return V4Constant<U32_PI_OVER_TWO,U32_PI_OVER_TWO,U32_PI_OVER_TWO,U32_PI_OVER_TWO>();
	}

	__forceinline const Vector_4 V4Constant(eNEG_PIInitializer)
	{
		return V4Constant<U32_NEG_PI,U32_NEG_PI,U32_NEG_PI,U32_NEG_PI>();
	}

	__forceinline const Vector_4 V4Constant(eNEG_PI_OVER_TWOInitializer)
	{
		return V4Constant<U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO,U32_NEG_PI_OVER_TWO>();
	}

	__forceinline const Vector_4 V4Constant(eTO_DEGREESInitializer)
	{
		return V4Constant<U32_TO_DEGREES,U32_TO_DEGREES,U32_TO_DEGREES,U32_TO_DEGREES>();
	}

	__forceinline const Vector_4 V4Constant(eTO_RADIANSInitializer)
	{
		return V4Constant<U32_TO_RADIANS,U32_TO_RADIANS,U32_TO_RADIANS,U32_TO_RADIANS>();
	}

	__forceinline const Vector_4 V4Constant(eSQRT_TWOInitializer)
	{
		return V4Constant<U32_SQRT_TWO,U32_SQRT_TWO,U32_SQRT_TWO,U32_SQRT_TWO>();
	}

	__forceinline const Vector_4 V4Constant(eONE_OVER_SQRT_TWOInitializer)
	{
		return V4Constant<U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO,U32_ONE_OVER_SQRT_TWO>();
	}

	__forceinline const Vector_4 V4Constant(eSQRT_THREEInitializer)
	{
		return V4Constant<U32_SQRT_THREE,U32_SQRT_THREE,U32_SQRT_THREE,U32_SQRT_THREE>();
	}

	__forceinline const Vector_4 V4Constant(eEInitializer)
	{
		return V4Constant<U32_E,U32_E,U32_E,U32_E>();
	}


	//================================================
	// Predefined Vector_3 Constants
	//================================================

	__forceinline const Vector_3 V3Constant(eZEROInitializer)
	{
		return V3Constant<U32_ZERO,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_MAXInitializer)
	{
		return V3Constant<U32_FLT_MAX,U32_FLT_MAX,U32_FLT_MAX>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_LARGE_2Initializer)
	{
		return V3Constant<U32_FLT_LARGE_2,U32_FLT_LARGE_2,U32_FLT_LARGE_2>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_LARGE_4Initializer)
	{
		return V3Constant<U32_FLT_LARGE_4,U32_FLT_LARGE_4,U32_FLT_LARGE_4>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_LARGE_6Initializer)
	{
		return V3Constant<U32_FLT_LARGE_6,U32_FLT_LARGE_6,U32_FLT_LARGE_6>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_LARGE_8Initializer)
	{
		return V3Constant<U32_FLT_LARGE_8,U32_FLT_LARGE_8,U32_FLT_LARGE_8>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_EPSILONInitializer)
	{
		return V3Constant<U32_FLT_EPSILON,U32_FLT_EPSILON,U32_FLT_EPSILON>();
	}

	__forceinline const Vector_3 V3Constant(eFLT_SMALL_12Initializer)
	{
		return V3Constant<U32_FLT_SMALL_12,U32_FLT_SMALL_12,U32_FLT_SMALL_12>();
	}

	__forceinline const Vector_3 V3Constant(eZERO_WONEInitializer)
	{
		return V3Constant<U32_ZERO,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eX_AXIS_WONEInitializer)
	{
		return V3Constant<U32_ONE,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eY_AXIS_WONEInitializer)
	{
		return V3Constant<U32_ZERO,U32_ONE,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eZ_AXIS_WONEInitializer)
	{
		return V3Constant<U32_ZERO,U32_ZERO,U32_ONE>();
	}

	__forceinline const Vector_3 V3Constant(eMASKYZWInitializer)
	{
		return V3Constant<0,U32_ALLF,U32_ALLF>();
	}

	__forceinline const Vector_3 V3Constant(eMASKXZWInitializer)
	{
		return V3Constant<U32_ALLF,U32_ZERO,U32_ALLF>();
	}

	__forceinline const Vector_3 V3Constant(eMASKXYWInitializer)
	{
		return V3Constant<U32_ALLF,U32_ALLF,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eMASKXInitializer)
	{
		return V3Constant<U32_ALLF,U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eMASKYInitializer)
	{
		return V3Constant<U32_ZERO,U32_ALLF,U32_ZERO>();
	}

	__forceinline const Vector_3 V3Constant(eMASKZInitializer)
	{
		return V3Constant<U32_ZERO,U32_ZERO,U32_ALLF>();
	}

	__forceinline const Vector_3 V3Constant(eMASKXYZInitializer)
	{
		return V3Constant<U32_ALLF,U32_ALLF,U32_ALLF>();
	}

	__forceinline const Vector_3 V3Constant(eONEInitializer)
	{
		return V3Constant<U32_ONE,U32_ONE,U32_ONE>();
	}

	__forceinline const Vector_3 V3Constant(eHALFInitializer)
	{
		return V3Constant<U32_HALF,U32_HALF,U32_HALF>();
	}

	__forceinline const Vector_3 V3Constant(eINFInitializer)
	{
		return V3Constant<U32_INF,U32_INF,U32_INF>();
	}

	__forceinline const Vector_3 V3Constant(eNEGINFInitializer)
	{
		return V3Constant<U32_NEGINF,U32_NEGINF,U32_NEGINF>();
	}

	__forceinline const Vector_3 V3Constant(eNANInitializer)
	{
		return V3Constant<U32_NAN,U32_NAN,U32_NAN>();
	}

	__forceinline const Vector_3 V3Constant(eLOG2_TO_LOG10Initializer)
	{
		return V3Constant<U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10,U32_LOG2_TO_LOG10>();
	}

	__forceinline const Vector_3 V3Constant(e7FFFFFFFInitializer)
	{
		return V3Constant<0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF>();
	}

	__forceinline const Vector_3 V3Constant(e80000000Initializer)
	{
		return V3Constant<0x80000000,0x80000000,0x80000000>();
	}

	//================================================
	// Predefined Vector_2 Constants
	//================================================

	__forceinline const Vector_2 V2Constant(eZEROInitializer)
	{
		return V2Constant<U32_ZERO,U32_ZERO>();
	}

	__forceinline const Vector_2 V2Constant(eONEInitializer)
	{
		return V2Constant<U32_ONE,U32_ONE>();
	}

	__forceinline const Vector_2 V2Constant(eFLT_LARGE_2Initializer)
	{
		return V2Constant<U32_FLT_LARGE_2,U32_FLT_LARGE_2>();
	}

	__forceinline const Vector_2 V2Constant(eFLT_LARGE_4Initializer)
	{
		return V2Constant<U32_FLT_LARGE_4,U32_FLT_LARGE_4>();
	}

	__forceinline const Vector_2 V2Constant(eFLT_LARGE_6Initializer)
	{
		return V2Constant<U32_FLT_LARGE_6,U32_FLT_LARGE_6>();
	}

	__forceinline const Vector_2 V2Constant(eFLT_LARGE_8Initializer)
	{
		return V2Constant<U32_FLT_LARGE_8,U32_FLT_LARGE_8>();
	}

	__forceinline const Vector_2 V2Constant(eFLT_MAXInitializer)
	{
		return V2Constant<U32_FLT_MAX,U32_FLT_MAX>();
	}

} // namespace Vec


namespace sysEndian
{
	template<> inline void SwapMe(Vec::Vector_4V& v) 
	{
		float* p = reinterpret_cast<float*>(&v);
		SwapMe(p[0]);
		SwapMe(p[1]);
		SwapMe(p[2]);
		SwapMe(p[3]);
	}
} // namespace sysEndian

} // namespace rage
