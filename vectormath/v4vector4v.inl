// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 


#if __XENON
// don't want to include xnamath.h (and xtl.h) here
extern __vector4        XMVectorPow(__vector4 V1, __vector4 V2);
#endif

namespace rage
{
namespace Vec
{
	__forceinline Vector_4V_Out V4LoadUnaligned( const void* src )
	{
#if __XENON
		// Due to a compiler bug, we need to use __lvx_volatile instead of
		// __lvx, otherwise the lvx128 instruction can be moved before stores to
		// the same memory address.  GameDS has been informed of this problem,
		// and hopefully this bug can get fixed in the compiler
		return __vperm(__lvx_volatile(src, 0), __lvx_volatile(src, 15), __lvsl(src, 0)); // this is safer ..
		//return __vor(__lvlx(src, 0), __lvrx(src, 16));
#elif __PPU
		return (vec_float4)vec_perm(vec_lvx(0, (const unsigned char*)src), vec_lvx(15, (const unsigned char*)src), vec_lvsl(0, (const unsigned char*)src)); // this is safer ..
		//return vec_or(vec_lvlx(0, src), vec_lvrx(16, src));
#elif RSG_CPU_INTEL
		return _mm_loadu_ps((const float*)src);
#elif __SPU // this code comes directly from \cell\target\spu\include\vectormath\cpp\vec_aos.h: inline void loadXYZW( Vector4 & vec, const float * fptr )
#if 0
		vec_float4 vec0 = (vec_float4)si_lqd(src, 0);
		vec_float4 vec1 = (vec_float4)si_lqd(src, 16);
		uint32_t offset = (uint32_t)src & 0xf;
		vec0 = spu_slqwbyte(vec0, offset);
		vec1 = spu_rlmaskqwbyte(vec1, offset - 16);
		return spu_or(vec0, vec1);
#else
		const vec_float4  vfsrc0 = *((vec_float4*)(((uintptr_t)src) + 0));
		const vec_float4  vfsrc1 = *((vec_float4*)(((uintptr_t)src) + 16));
		const vec_uchar16 vucpat = (vec_uchar16)spu_add(
			(vec_ushort8)(spu_splats((unsigned char)(((int)src) & 0xf))),
			(vec_ushort8)(0x0001, 0x0203, 0x0405, 0x0607, 0x0809, 0x0A0B, 0x0C0D, 0x0E0F)
			);
		return spu_shuffle(vfsrc0, vfsrc1, vucpat);
#endif
#endif
	}

	__forceinline void V4StoreUnaligned( void* dst, Vector_4V_In v )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)dst & 3) == 0);
#if __XENON
		__stvlx(v, dst, 0);
		__stvrx(v, dst, 16);
#elif __PPU
		vec_stvlx((vec_uchar16)v, 0,  (unsigned char*)dst);
		vec_stvrx((vec_uchar16)v, 16, (unsigned char*)dst);
#elif RSG_CPU_INTEL
		return _mm_storeu_ps((float*)dst, v);
#elif __SPU // this code comes directly from \cell\target\spu\include\vectormath\cpp\vec_aos.h: inline void storeXYZW( Vector4 vec, float * fptr )
#if 0
		vec_float4 dstVec0 = (vec_float4)si_lqd(dst, 0);
		vec_float4 dstVec1 = (vec_float4)si_lqd(dst, 16);
#else
		vec_float4 * vptr0 = (vec_float4*)(((uintptr_t)dst) + 0);
		vec_float4 * vptr1 = (vec_float4*)(((uintptr_t)dst) + 16);
		vec_float4 dstVec0 = *vptr0;
		vec_float4 dstVec1 = *vptr1;
#endif
		uint32_t   offset = (uint32_t)dst & 0xf;
		vec_uint4  mask   = (vec_uint4)spu_splats(0xffffffff);
		vec_uint4  mask0  = (vec_uint4)spu_rlmaskqwbyte(mask, -offset);
		vec_uint4  mask1  = (vec_uint4)spu_slqwbyte(mask, 16 - offset);
		vec_float4 vec0   = spu_rlmaskqwbyte(v, -offset);
		vec_float4 vec1   = spu_slqwbyte(v, 16 - offset);
		dstVec0 = spu_sel(dstVec0, vec0, mask0);
		dstVec1 = spu_sel(dstVec1, vec1, mask1);
#if 0
		si_stqd(dstVec0, dst, 0);
		si_stqd(dstVec1, dst, 16);
#else
		*vptr0 = dstVec0;
		*vptr1 = dstVec1;
#endif
#endif
	}

	template<unsigned NUM_BYTES>
	__forceinline Vector_4V_Out V4LoadUnalignedSafe( const void* src )
	{
#		if __XENON
			// See V4LoadUnaligned for explanation of why __lvx_volatile is used.
			__vector4 left  = __lvx_volatile(src, 0);
			__vector4 right = __lvx_volatile(src, NUM_BYTES-1);
			__vector4 ret = __vperm(left, right, __lvsl(src, 0));
#			if __DEV
				// Trash the undefined bytes to help catch cases were not enough
				// were requested.
				__vector4 mask = __vsldoi(__vspltisw(0), __vspltisw(-1), 16-NUM_BYTES);
				ret = __vor(ret, mask);
#			endif
			return (Vector_4V)ret;
#		elif __PPU
			vec_uchar16 left  = vec_lvx(0,           (const unsigned char*)src);
			vec_uchar16 right = vec_lvx(NUM_BYTES-1, (const unsigned char*)src);
			vec_uchar16 ret = vec_vperm(left, right, vec_lvsl(0, (const unsigned char*)src));
#			if __DEV
				vec_uchar16 mask = vec_vsldoi((vec_uchar16)vec_vspltisw(0), (vec_uchar16)vec_vspltisw(-1), 16-NUM_BYTES);
				ret = vec_vor(ret, mask);
#			endif
			return (Vector_4V)ret;
#		elif RSG_CPU_INTEL
			__m128 v[2];
			v[0] = _mm_load_ps((float*)((uptr)src&~15));
			v[1] = _mm_load_ps((float*)(((uptr)src+NUM_BYTES-1)&~15));
			__m128 ret = _mm_loadu_ps((float*)((uptr)v+((uptr)src&15)));
#			if __DEV
				__m128i mask = _mm_setzero_si128();
				mask = _mm_cmpeq_epi32(mask, mask);
				mask = _mm_slli_si128(mask, NUM_BYTES);
				ret = _mm_or_ps(ret, _mm_castsi128_ps(mask));
#			endif
			return (Vector_4V)ret;
#		elif __SPU
			Vector_4V ret = V4LoadUnaligned(src);
#			if __DEV
				qword mask = si_fsmbi((1<<(16-NUM_BYTES))-1);
				ret = (Vector_4V)si_or((qword)ret, mask);
#			endif
			return ret;
#		endif
	}

	__forceinline Vector_4V_Out V4LoadLeftUnsafe( const void* src )
	{
#if __XENON
		return __lvlx(src, 0);
#elif __PPU
		return (vec_float4)vec_lvlx(0, (const unsigned char*)src);
#elif RSG_CPU_INTEL
		return _mm_loadu_ps((const float*)src);
#elif __SPU
		qword q = si_lqd(si_from_ptr(src), 0);
		q = si_rotqby(q, si_from_ptr(src));
		return (vec_float4)q;
#endif
	}

	__forceinline Vector_4V_Out V4LoadLeft( const void* src )
	{
#if RSG_CPU_INTEL
		__m128 v[2];
		v[0] = _mm_load_ps((float*)((uptr)src&~15));
		return _mm_loadu_ps((float*)((uptr)v+((uptr)src&15)));
#else
		return V4LoadLeftUnsafe(src);
#endif
	}

	// Returns float as a splatted Vector_4V.
	__forceinline Vector_4V_Out V4LoadScalar32IntoSplatted( const float& scalar )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)&scalar & 3) == 0);
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V){scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return vec_splat((Vector_4V)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return spu_splats( scalar );
#endif
		}
#elif __XENON
		return __vspltw(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_set_ps1( scalar );
#endif
	}

	// Returns u32 as a splatted Vector_4V.
	__forceinline Vector_4V_Out V4LoadScalar32IntoSplatted( const u32& scalar )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)&scalar & 3) == 0);
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_uint){scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return vec_splat((Vector_4V)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		}
#elif __XENON
		return __vspltw(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi32( scalar ));
#endif
	}

	// Returns s32 as a splatted Vector_4V.
	__forceinline Vector_4V_Out V4LoadScalar32IntoSplatted( const s32& scalar )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)&scalar & 3) == 0);
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_int){scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return vec_splat((Vector_4V)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		}
#elif __XENON
		return __vspltw(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi32( scalar ));
#endif
	}

	__forceinline void V4StoreScalar32FromSplatted( float& fLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvewx( splattedVec, &fLoc, 0 );
#elif __PPU
		vec_ste( splattedVec, 0, &fLoc );
#elif __SPU
		fLoc = spu_extract( splattedVec, 0 );
#elif RSG_CPU_INTEL
		_mm_store_ss( &fLoc, splattedVec );
#endif
	}

	__forceinline void V4StoreScalar32FromSplatted( u32& uLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvewx( splattedVec, &uLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_uint)splattedVec, 0, &uLoc );
#elif __SPU
		uLoc = spu_extract( (Vector_4V_uint)splattedVec, 0 );
#elif RSG_CPU_INTEL
		_mm_store_ss( (float*)(&uLoc), splattedVec );
#endif
	}

	__forceinline void V4StoreScalar32FromSplatted( s32& sLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvewx( splattedVec, &sLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_int)splattedVec, 0, &sLoc );
#elif __SPU
		sLoc = spu_extract( (Vector_4V_int)splattedVec, 0 );
#elif RSG_CPU_INTEL
		_mm_store_ss( (float*)(&sLoc), splattedVec );
#endif
	}

	__forceinline Vector_4V_Out V4LoadScalar16IntoSplatted( const u16& scalar )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)&scalar & 1) == 0);
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_ushort){scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return (Vector_4V)vec_splat((Vector_4V_ushort)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		} 
#elif __XENON
		return __vsplth(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi16( scalar ));
#endif
	}

	__forceinline Vector_4V_Out V4LoadScalar16IntoSplatted( const s16& scalar )
	{
		// Check for misaligned val.
//		FastAssert(((uptr)&scalar & 1) == 0);
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_short){scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return (Vector_4V)vec_splat((Vector_4V_short)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		} 
#elif __XENON
		return __vsplth(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi16( scalar ));
#endif
	}

	__forceinline void V4StoreScalar16FromSplatted( u16& uLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvehx( splattedVec, &uLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_ushort)splattedVec, 0, &uLoc );
#elif __SPU
		uLoc = spu_extract( (Vector_4V_ushort)splattedVec, 0 );
#elif RSG_CPU_INTEL
 #if RSG_ORBIS
		uLoc = _mm_extract_epi16(splattedVec, 0);
 #else
		// No SSE/SSE2 intrinsic for this.
		uLoc = splattedVec.m128_u16[0];
 #endif
#endif
	}

	__forceinline void V4StoreScalar16FromSplatted( s16& sLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvehx( splattedVec, &sLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_short)splattedVec, 0, &sLoc );
#elif __SPU
		sLoc = spu_extract( (Vector_4V_short)splattedVec, 0 );
#elif RSG_CPU_INTEL
 #if RSG_ORBIS
		sLoc = _mm_extract_epi16(splattedVec, 0);
 #else
		// No SSE/SSE2 intrinsic for this.
		sLoc = splattedVec.m128_i16[0];
 #endif
#endif
	}

	__forceinline Vector_4V_Out V4LoadScalar8IntoSplatted( const u8& scalar )
	{
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_uchar){scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return (Vector_4V)vec_splat((Vector_4V_uchar)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		}
#elif __XENON
		return __vspltb(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi8( scalar ));
#endif
	}

	__forceinline Vector_4V_Out V4LoadScalar8IntoSplatted( const s8& scalar )
	{
#if __PS3
		if(__builtin_constant_p(scalar))
		{
			return (Vector_4V)(Vector_4V_char){scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar, scalar};
		}
		else
		{
#if __PPU
			return (Vector_4V)vec_splat((Vector_4V_char)vec_lvlx(0, &scalar), 0);
#elif __SPU
			return (Vector_4V)spu_splats( scalar );
#endif
		}
#elif __XENON
		return __vspltb(__lvlx(&scalar, 0), 0);
#elif RSG_CPU_INTEL
		return _mm_castsi128_ps(_mm_set1_epi8( scalar ));
#endif
	}

	__forceinline void V4StoreScalar8FromSplatted( u8& uLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvebx( splattedVec, &uLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_uchar)splattedVec, 0, &uLoc );
#elif __SPU
		uLoc = spu_extract( (Vector_4V_uchar)splattedVec, 0 );
#elif RSG_CPU_INTEL
 #if RSG_ORBIS
		uLoc = _mm_extract_epi8(splattedVec, 0);
 #else
		// No SSE/SSE2 intrinsic for this.
		uLoc = splattedVec.m128_u8[0];
 #endif
#endif
	}

	__forceinline void V4StoreScalar8FromSplatted( s8& sLoc, Vector_4V_In splattedVec )
	{
#if __XENON
		__stvebx( splattedVec, &sLoc, 0 );
#elif __PPU
		vec_ste( (Vector_4V_char)splattedVec, 0, &sLoc );
#elif __SPU
		sLoc = spu_extract( (Vector_4V_char)splattedVec, 0 );
#elif RSG_CPU_INTEL
 #if RSG_ORBIS
		sLoc = _mm_extract_epi8(splattedVec, 0);
 #else
		// No SSE/SSE2 intrinsic for this.
		sLoc = splattedVec.m128_i8[0];
 #endif
#endif
	}

	__forceinline void SetX( Vector_4V_InOut inoutVector, const float& floatVal )
	{
		Vector_4V vX = V4LoadScalar32IntoSplatted( floatVal );
		inoutVector = V4PermuteTwo<X1,Y2,Z2,W2>(vX, inoutVector);
	}

	__forceinline void SetY( Vector_4V_InOut inoutVector, const float& floatVal )
	{
		Vector_4V vY = V4LoadScalar32IntoSplatted( floatVal );
		inoutVector = V4PermuteTwo<X2,Y1,Z2,W2>(vY, inoutVector);
	}

	__forceinline void SetZ( Vector_4V_InOut inoutVector, const float& floatVal )
	{
		Vector_4V vZ = V4LoadScalar32IntoSplatted( floatVal );
		inoutVector = V4PermuteTwo<X2,Y2,Z1,W2>(vZ, inoutVector);
	}

	__forceinline void SetW( Vector_4V_InOut inoutVector, const float& floatVal )
	{
		Vector_4V vW = V4LoadScalar32IntoSplatted( floatVal );
		inoutVector = V4PermuteTwo<X2,Y2,Z2,W1>(vW, inoutVector);
	}

	/* __forceinline void SetX( Vector_4V_InOut inoutVector, const u32& uintVal )
	{
		SetX( inoutVector, *(reinterpret_cast<const float*>(&uintVal)) );
	}

	__forceinline void SetY( Vector_4V_InOut inoutVector, const u32& uintVal )
	{
		SetY( inoutVector, *(reinterpret_cast<const float*>(&uintVal)) );
	}

	__forceinline void SetZ( Vector_4V_InOut inoutVector, const u32& uintVal )
	{
		SetZ( inoutVector, *(reinterpret_cast<const float*>(&uintVal)) );
	}

	__forceinline void SetW( Vector_4V_InOut inoutVector, const u32& uintVal )
	{
		SetW( inoutVector, *(reinterpret_cast<const float*>(&uintVal)) );
	} */

	__forceinline void SetXInMemory( Vector_4V_InOut inoutVector, float floatVal )
	{
		(reinterpret_cast<float*>(&inoutVector))[0] = floatVal;
	}

	__forceinline void SetYInMemory( Vector_4V_InOut inoutVector, float floatVal )
	{
		(reinterpret_cast<float*>(&inoutVector))[1] = floatVal;
	}

	__forceinline void SetZInMemory( Vector_4V_InOut inoutVector, float floatVal )
	{
		(reinterpret_cast<float*>(&inoutVector))[2] = floatVal;
	}

	__forceinline void SetWInMemory( Vector_4V_InOut inoutVector, float floatVal )
	{
		(reinterpret_cast<float*>(&inoutVector))[3] = floatVal;
	}

	__forceinline void SetXYZWInMemory( Vector_4V_InOut inoutVector, float x, float y, float z, float w )
	{
		float *asFloats = reinterpret_cast<float*>(&inoutVector);
		asFloats[0] = x;
		asFloats[1] = y;
		asFloats[2] = z;
		asFloats[3] = w;
	}

	__forceinline void SetXInMemory( Vector_4V_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[0] = intVal;
	}

	__forceinline void SetYInMemory( Vector_4V_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[1] = intVal;
	}

	__forceinline void SetZInMemory( Vector_4V_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[2] = intVal;
	}

	__forceinline void SetWInMemory( Vector_4V_InOut inoutVector, int intVal )
	{
		(reinterpret_cast<int*>(&inoutVector))[3] = intVal;
	}

	__forceinline void SetXInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal )
	{
		V4StoreScalar32FromSplatted( (reinterpret_cast<float*>(&inoutVector))[0], splattedVal );
	}

	__forceinline void SetYInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal )
	{
		V4StoreScalar32FromSplatted( (reinterpret_cast<float*>(&inoutVector))[1], splattedVal );
	}

	__forceinline void SetZInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal )
	{
		V4StoreScalar32FromSplatted( (reinterpret_cast<float*>(&inoutVector))[2], splattedVal );
	}

	__forceinline void SetWInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal )
	{
		V4StoreScalar32FromSplatted( (reinterpret_cast<float*>(&inoutVector))[3], splattedVal );
	}

	__forceinline float GetElem( Vector_4V_ConstRef inVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index %d out of range [0,3]", elem );
		return ((float*)(&inVector))[elem];
	}

	__forceinline float& GetElemRef( Vector_4V_Ptr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index %d out of range [0,3]", elem );
		return ((float*)(pInVector))[elem];
	}

	__forceinline const float& GetElemRef( Vector_4V_ConstPtr pInVector, unsigned int elem )
	{
		mthAssertf( elem <= 3 , "Invalid element index %d out of range [0,3]", elem );
		return ((const float*)(pInVector))[elem];
	}

	__forceinline Vector_4V_Out GetXV( Vector_4V_In inVector )
	{
		return V4SplatX(inVector);
	}

	__forceinline Vector_4V_Out GetYV( Vector_4V_In inVector )
	{
		return V4SplatY(inVector);
	}

	__forceinline Vector_4V_Out GetZV( Vector_4V_In inVector )
	{
		return V4SplatZ(inVector);
	}

	__forceinline Vector_4V_Out GetWV( Vector_4V_In inVector )
	{
		return V4SplatW(inVector);
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const float& s )
	{
		inoutVector = V4LoadScalar32IntoSplatted( s );
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const u32& s )
	{
		inoutVector = V4LoadScalar32IntoSplatted( s );
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const int& s )
	{
		inoutVector = V4LoadScalar32IntoSplatted( s );
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, Vector_4V_In inVector )
	{
		inoutVector = inVector;
	}

	__forceinline void V4ZeroComponents( Vector_4V_InOut inoutVector )
	{
		inoutVector = V4VConstant(V_ZERO);
	}

	__forceinline void V4SetWZero( Vector_4V_InOut inoutVector )
	{
#if !USE_ALTERNATE_SETWZERO
		Vector_4V zero = V4VConstant(V_ZERO);

		// 1 instruction on Xenon. [NOTE -- __vrlimi can be used to mask w-component, constant zero is not required]
		// 2 dependent instructions on PS3 (if v4permtwo2instructions_ps3.inl is #included in v4vector4vcore_ps3.inl).
		inoutVector = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( zero, inoutVector );
#else
		inoutVector = V4And(inoutVector, V4VConstant(V_MASKXYZ));
#endif // !USE_ALTERNATE_SETWZERO
	}

	//============================================================================
	// Standard Algebra

	__forceinline Vector_4V_Out V4Invert_NewtonRaphsonRefine(Vector_4V_In x, Vector_4V_In y0)
	{
		// y1 = y0 + y0 * (1.0 - x * y0)

		const Vector_4V one = V4VConstant(V_ONE);
		return V4AddScaled(y0, y0, V4SubtractScaled(one, x, y0));
	}

	__forceinline Vector_4V_Out V4InvSqrt_NewtonRaphsonRefine(Vector_4V_In x, Vector_4V_In y0)
	{
		// y1 = y0 + 0.5 * y0 * (1.0 - x * y0 * y0)
		// .. = y0 + y0 * (0.5 - 0.5 * x * y0 * y0)

		const Vector_4V half = V4VConstant(V_HALF);
		return V4AddScaled(y0, y0, V4SubtractScaled(half, half, V4Scale(x, V4Scale(y0, y0))));
	}

#define __VECTOR_SQRT_INTRINSIC    (RSG_CPU_INTEL && 1) // platform has vector sqrt intrinsic (can be disabled on PC for testing)
#define __VECTOR_DENORMAL_HANDLING (RSG_CPU_INTEL) // platform handles denormal values uniquely instead of flushing to zero

#if __XENON
	__forceinline Vector_4V_Out V4InvertFast         (Vector_4V_In a) { return __vrefp(a); }
	__forceinline Vector_4V_Out V4InvSqrtFast        (Vector_4V_In a) { return __vrsqrtefp(a); }
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return __vrefp(__vrsqrtefp(a)); } // correctly returns 0 for 0 <= x < FLT_MIN
#elif __PPU
	__forceinline Vector_4V_Out V4InvertFast         (Vector_4V_In a) { return vec_re(a); }
	__forceinline Vector_4V_Out V4InvSqrtFast        (Vector_4V_In a) { return vec_rsqrte(a); }
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return vec_re(vec_rsqrte(a)); } // correctly returns 0 for 0 <= x < FLT_MIN
#elif __SPU
#if 1 // these call si_fi after si_frest/si_frsqest, so they are more accurate but slightly slower
	__forceinline Vector_4V_Out V4InvertFast         (Vector_4V_In a) { return spu_re(a); }
	__forceinline Vector_4V_Out V4InvSqrtFast        (Vector_4V_In a) { return spu_rsqrte(a); }
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return spu_re(spu_rsqrte(a)); } // correctly returns 0 for 0 <= x < FLT_MIN
#else // consider these for faster estimates ..
	__forceinline Vector_4V_Out V4InvertFast         (Vector_4V_In a) { return si_frest(a); }
	__forceinline Vector_4V_Out V4InvSqrtFast        (Vector_4V_In a) { return si_frsqest(a); }
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return si_frest(si_frsqest(a)); } // correctly returns 0 for 0 <= x < FLT_MIN
#endif
#elif RSG_CPU_INTEL
	__forceinline Vector_4V_Out V4InvertFast         (Vector_4V_In a) { return _mm_rcp_ps(a); }
	__forceinline Vector_4V_Out V4InvSqrtFast        (Vector_4V_In a) { return _mm_rsqrt_ps(a); }
#if __VECTOR_SQRT_INTRINSIC
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return _mm_sqrt_ps(a); }
#else
	__forceinline Vector_4V_Out V4SqrtFast           (Vector_4V_In a) { return _mm_rcp_ps(_mm_rsqrt_ps(a)); } // correctly returns 0 for 0 <= x < FLT_MIN
#endif
#endif

	// NOTE -- 'Precise' functions do an additional Newton-Raphson iteration, but typically this only improves accuracy by 1-2 bits
	__forceinline Vector_4V_Out V4Invert             (Vector_4V_In a) { return V4Invert_NewtonRaphsonRefine (a, V4InvertFast (a)); }
	__forceinline Vector_4V_Out V4InvertPrecise      (Vector_4V_In a) { return V4Invert_NewtonRaphsonRefine (a, V4Invert     (a)); }
	__forceinline Vector_4V_Out V4InvSqrt            (Vector_4V_In a) { return V4InvSqrt_NewtonRaphsonRefine(a, V4InvSqrtFast(a)); }
	__forceinline Vector_4V_Out V4InvSqrtPrecise     (Vector_4V_In a) { return V4InvSqrt_NewtonRaphsonRefine(a, V4InvSqrt    (a)); }
#if __VECTOR_SQRT_INTRINSIC
	__forceinline Vector_4V_Out V4Sqrt               (Vector_4V_In a) { return V4SqrtFast(a); }
	__forceinline Vector_4V_Out V4SqrtPrecise        (Vector_4V_In a) { return V4SqrtFast(a); }
#else
	__forceinline Vector_4V_Out V4Sqrt               (Vector_4V_In a) { return V4And(V4IsGreaterThanOrEqualV(a, V4VConstant(V_FLT_MIN)), V4Scale(a, V4InvSqrt       (a))); }
	__forceinline Vector_4V_Out V4SqrtPrecise        (Vector_4V_In a) { return V4And(V4IsGreaterThanOrEqualV(a, V4VConstant(V_FLT_MIN)), V4Scale(a, V4InvSqrtPrecise(a))); }
#endif
	__forceinline Vector_4V_Out V4InvScaleFast       (Vector_4V_In a, Vector_4V_In b) { return V4Scale(a, V4InvertFast   (b)); }
	__forceinline Vector_4V_Out V4InvScale           (Vector_4V_In a, Vector_4V_In b) { return V4Scale(a, V4Invert       (b)); }
	__forceinline Vector_4V_Out V4InvScalePrecise    (Vector_4V_In a, Vector_4V_In b) { return V4Scale(a, V4InvertPrecise(b)); }

	// private utility functions ..
#if __VECTOR_DENORMAL_HANDLING
	__forceinline Vector_4V_Out _util_V4InvertSafe   (Vector_4V_In a,                 Vector_4V_In retVal, Vector_4V_In            errVal) { return V4SelectFT(V4IsLessThanV(V4Abs(a), V4VConstant(V_FLT_MIN)), retVal           , errVal); }
	__forceinline Vector_4V_Out _util_V4InvScaleSafe (Vector_4V_In a, Vector_4V_In b, Vector_4V_In b_inv,  Vector_4V_In_After3Args errVal) { return V4SelectFT(V4IsLessThanV(V4Abs(b), V4VConstant(V_FLT_MIN)), V4Scale(a, b_inv), errVal); }
#else
	__forceinline Vector_4V_Out _util_V4InvertSafe   (Vector_4V_In a,                 Vector_4V_In retVal, Vector_4V_In            errVal) { return V4SelectFT(V4IsEqualV   (a, V4VConstant(V_ZERO)), retVal           , errVal); }
	__forceinline Vector_4V_Out _util_V4InvScaleSafe (Vector_4V_In a, Vector_4V_In b, Vector_4V_In b_inv,  Vector_4V_In_After3Args errVal) { return V4SelectFT(V4IsEqualV   (b, V4VConstant(V_ZERO)), V4Scale(a, b_inv), errVal); }
#endif
	__forceinline Vector_4V_Out _util_V4InvSqrtSafe  (Vector_4V_In a,                 Vector_4V_In retVal, Vector_4V_In            errVal) { return V4SelectFT(V4IsLessThanV(a, V4VConstant(V_FLT_MIN)), retVal, errVal); }
	__forceinline Vector_4V_Out _util_V4SqrtSafe     (Vector_4V_In a,                 Vector_4V_In retVal, Vector_4V_In            errVal) { return V4SelectFT(V4IsLessThanV(a, V4VConstant(V_ZERO)), retVal, errVal); }

	__forceinline Vector_4V_Out V4InvertFastSafe     (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvertSafe (a, V4InvertFast    (a), errVal); }
	__forceinline Vector_4V_Out V4InvertSafe         (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvertSafe (a, V4Invert        (a), errVal); }
	__forceinline Vector_4V_Out V4InvertPreciseSafe  (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvertSafe (a, V4InvertPrecise (a), errVal); }
	__forceinline Vector_4V_Out V4InvSqrtFastSafe    (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvSqrtSafe(a, V4InvSqrtFast   (a), errVal); }
	__forceinline Vector_4V_Out V4InvSqrtSafe        (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvSqrtSafe(a, V4InvSqrt       (a), errVal); }
	__forceinline Vector_4V_Out V4InvSqrtPreciseSafe (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4InvSqrtSafe(a, V4InvSqrtPrecise(a), errVal); }
	__forceinline Vector_4V_Out V4SqrtFastSafe       (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4SqrtSafe   (a, V4SqrtFast      (a), errVal); }
	__forceinline Vector_4V_Out V4SqrtSafe           (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4SqrtSafe   (a, V4Sqrt          (a), errVal); }
	__forceinline Vector_4V_Out V4SqrtPreciseSafe    (Vector_4V_In a, Vector_4V_In errVal) { return _util_V4SqrtSafe   (a, V4SqrtPrecise   (a), errVal); }

	__forceinline Vector_4V_Out V4InvScaleFastSafe   (Vector_4V_In a, Vector_4V_In b, Vector_4V_In errVal) { return _util_V4InvScaleSafe(a, b, V4InvertFast   (b), errVal); }
	__forceinline Vector_4V_Out V4InvScaleSafe       (Vector_4V_In a, Vector_4V_In b, Vector_4V_In errVal) { return _util_V4InvScaleSafe(a, b, V4Invert       (b), errVal); }
	__forceinline Vector_4V_Out V4InvScalePreciseSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errVal) { return _util_V4InvScaleSafe(a, b, V4InvertPrecise(b), errVal); }

	__forceinline Vector_4V_Out V4SqrtFastUnsafe(Vector_4V_In a) // max relative error is slightly over half that of V4SqrtFastSafe, but will fail for values less than FLT_MIN
	{
		return V4Scale(a, V4InvSqrtFast(a));
	}

	__forceinline Vector_4V_Out V4SqrtFastSafeZero(Vector_4V_In a) // max relative error is slightly over half that of V4SqrtFastSafe, but errVal has to be zero
	{
		return V4SelectFT(V4IsGreaterThanOrEqualV(a, V4VConstant(V_FLT_MIN)), V4VConstant(V_ZERO), V4SqrtFastUnsafe(a));
	}

	//============================================================================

	__forceinline Vector_4V_Out V4Normalize(Vector_4V_In inVector)
	{
		Vector_4V invSqrtVector = V4InvMagV( inVector );
		return V4Scale(inVector, invSqrtVector);
	}

	__forceinline Vector_4V_Out V4NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V4MagSquaredV(inVector);
		Vector_4V invSqrtVector = V4InvSqrt( dotVector );
		Vector_4V resultVector = V4Scale(inVector, invSqrtVector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline Vector_4V_Out V4NormalizeFast(Vector_4V_In inVector)
	{
		Vector_4V invSqrtVector = V4InvMagVFast( inVector );
		return V4Scale(inVector, invSqrtVector);
	}

	__forceinline Vector_4V_Out V4NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold)
	{
		Vector_4V dotVector = V4MagSquaredV(inVector);
		Vector_4V invSqrtVector = V4InvSqrtFast(dotVector);
		Vector_4V resultVector = V4Scale(inVector, invSqrtVector);

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_GT_THEN_ELSE( dotVector, magSqThreshold, resultVector, errValVect );
	}

	__forceinline float V4Dot(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4DotV(a, b) );
	}

	__forceinline Vector_4V_Out V4Average(Vector_4V_In a, Vector_4V_In b)
	{
		return V4Scale( V4Add(a, b), V4VConstant(V_HALF) );
	}

	__forceinline Vector_4V_Out V4Lerp( const float& t, Vector_4V_In a, Vector_4V_In b )
	{
		return V4Lerp( V4LoadScalar32IntoSplatted(t), a, b );
	}

	__forceinline Vector_4V_Out V4Lerp( Vector_4V_In t, Vector_4V_In a, Vector_4V_In b )
	{
		return V4AddScaled( a, V4Subtract(b, a), t );
	}

	__forceinline Vector_4V_Out V4Pow( Vector_4V_In x, Vector_4V_In y )
	{
		// pow( x, y ) = 2^( y*log2(x) )
		return V4Expt( V4Scale( y, V4Log2(x) ) );
	}

	__forceinline Vector_4V_Out V4PowPrecise( Vector_4V_In x, Vector_4V_In y)
	{
#if __XENON
		return XMVectorPow(x, y);
#elif __PPU
		return powf4(x, y);
#else
		return V4Pow(x, y);
#endif
	}


	__forceinline Vector_4V_Out V4Log10( Vector_4V_In x )
	{
		return V4Scale( V4Log2( x ), V4VConstant(V_LOG2_TO_LOG10) );
	}

	__forceinline Vector_4V_Out V4Modulus( Vector_4V_In x, Vector_4V_In mod )
	{
		// x - ( mod*static_cast<int>(x/mod) );
		return V4SubtractScaled( x, mod, V4RoundToNearestIntZero( V4InvScale(x, mod) ) );
	}

	//============================================================================
	// Magnitude and distance

	__forceinline float V4Mag( Vector_4V_In v )
	{
		return GetX( V4MagV(v) );
	}

	__forceinline float V4MagFast( Vector_4V_In v )
	{
		return GetX( V4MagVFast(v) );
	}

	__forceinline Vector_4V_Out V4MagV( Vector_4V_In v )
	{
		return V4Sqrt( V4MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V4MagVFast( Vector_4V_In v )
	{
		return V4SqrtFast( V4MagSquaredV(v) );
	}

	__forceinline float V4MagSquared( Vector_4V_In v )
	{
		return GetX( V4MagSquaredV( v ) );
	}

	__forceinline float V4InvMag( Vector_4V_In v )
	{
		return GetX( V4InvMagV( v ) );
	}

	__forceinline float V4InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V4InvMagVSafe( v, errValVect ) );
	}

	__forceinline float V4InvMagFast( Vector_4V_In v )
	{
		return GetX( V4InvMagVFast( v ) );
	}

	__forceinline float V4InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V4InvMagVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V4InvMagV( Vector_4V_In v )
	{
		return V4InvSqrt( V4MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V4InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V4MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrt( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline Vector_4V_Out V4InvMagVFast( Vector_4V_In v )
	{
		return V4InvSqrtFast( V4MagSquaredV(v) );
	}

	__forceinline Vector_4V_Out V4InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V magSqVector = V4MagSquaredV( v );
		Vector_4V resultVector = V4InvSqrtFast( magSqVector );

		// Error-check (for divide by 0.0 (also works for -0.0))
		return IF_EQ_THEN_ELSE( magSqVector, V4VConstant(V_ZERO), errValVect, resultVector );
	}

	__forceinline float V4Dist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4DistV(a, b) );
	}

	__forceinline float V4DistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4DistVFast(a, b) );
	}

	__forceinline Vector_4V_Out V4DistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V4MagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V4DistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V4MagVFast( V4Subtract( a, b ) );
	}

	__forceinline float V4InvDist(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4InvDistV( a, b ) );
	}

	__forceinline float V4InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V4InvDistVSafe( a, b, errValVect ) );
	}

	__forceinline float V4InvDistFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4InvDistVFast( a, b ) );
	}

	__forceinline float V4InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V4InvDistVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V4InvDistV(Vector_4V_In a, Vector_4V_In b)
	{
		return V4InvMagV( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V4InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V4InvMagVSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline Vector_4V_Out V4InvDistVFast(Vector_4V_In a, Vector_4V_In b)
	{
		return V4InvMagVFast( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V4InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return V4InvMagVFastSafe( V4Subtract( a, b ), errValVect );
	}

	__forceinline float V4DistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return V4MagSquared( V4Subtract( a, b ) );
	}

	__forceinline Vector_4V_Out V4DistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		return V4MagSquaredV( V4Subtract( a, b ) );
	}

	__forceinline float V4InvMagSquared( Vector_4V_In v )
	{
		return GetX( V4InvMagSquaredV( v ) );
	}

	__forceinline float V4InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V4InvMagSquaredVSafe( v, errValVect ) );
	}

	__forceinline float V4InvMagSquaredFast( Vector_4V_In v )
	{
		return GetX( V4InvMagSquaredVFast( v ) );
	}

	__forceinline float V4InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		return GetX( V4InvMagSquaredVFastSafe( v, errValVect ) );
	}

	__forceinline Vector_4V_Out V4InvMagSquaredV( Vector_4V_In v )
	{
		Vector_4V outVect = V4MagSquaredV( v );
		return V4Invert( outVect );
	}

	__forceinline Vector_4V_Out V4InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V4MagSquaredV( v );
		return V4InvertSafe( outVect, errValVect );
	}

	__forceinline Vector_4V_Out V4InvMagSquaredVFast( Vector_4V_In v )
	{
		Vector_4V outVect = V4MagSquaredV( v );
		return V4InvertFast( outVect );
	}

	__forceinline Vector_4V_Out V4InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect )
	{
		Vector_4V outVect = V4MagSquaredV( v );
		return V4InvertFastSafe( outVect, errValVect );
	}
	
	__forceinline float V4InvDistSquared(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4InvDistSquaredV( a, b ) );
	}

	__forceinline float V4InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V4InvDistSquaredVSafe( a, b, errValVect ) );
	}

	__forceinline float V4InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b)
	{
		return GetX( V4InvDistSquaredVFast( a, b ) );
	}

	__forceinline float V4InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		return GetX( V4InvDistSquaredVFastSafe( a, b, errValVect ) );
	}

	__forceinline Vector_4V_Out V4InvDistSquaredV(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V4DistSquaredV( a, b );
		return V4Invert( outVector );
	}

	__forceinline Vector_4V_Out V4InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V4DistSquaredV( a, b );
		return V4InvertSafe( outVector, errValVect );
	}

	__forceinline Vector_4V_Out V4InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b)
	{
		Vector_4V outVector = V4DistSquaredV( a, b );
		return V4InvertFast( outVector );
	}

	__forceinline Vector_4V_Out V4InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect)
	{
		Vector_4V outVector = V4DistSquaredV( a, b );
		return V4InvertFastSafe( outVector, errValVect );
	}

	//============================================================================
	// Comparison functions

	__forceinline Vector_4V_Out V4SameSignV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return V4IsGreaterThanOrEqualV( V4Scale( inVector1, inVector2 ), V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V4SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return V4IsGreaterThanOrEqualAll( V4Scale( inVector1, inVector2 ), V4VConstant(V_ZERO) );
	}

	__forceinline Vector_4V_Out V4IsZeroV(Vector_4V_In inVector)
	{
		return V4IsEqualV( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V4IsZeroAll(Vector_4V_In inVector)
	{
		return V4IsEqualAll( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline unsigned int V4IsZeroNone(Vector_4V_In inVector)
	{
		return V4IsEqualNone( inVector, V4VConstant(V_ZERO) );
	}

	__forceinline Vector_4V_Out V4IsCloseV(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		return V4And(testVect1, testVect2);
	}

	__forceinline Vector_4V_Out V4IsNotCloseV(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsLessThanV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsGreaterThanV(inVector2, plusEpsVect);
		return V4Or(testVect1, testVect2);
	}

	__forceinline unsigned int V4IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V4IsEqualIntAll( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline unsigned int V4IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps)
	{
		Vector_4V minusEpsVect = V4Subtract(inVector1, eps);
		Vector_4V plusEpsVect = V4Add(inVector1, eps);
		Vector_4V testVect1 = V4IsGreaterThanOrEqualV(inVector2, minusEpsVect);
		Vector_4V testVect2 = V4IsLessThanOrEqualV(inVector2, plusEpsVect);
		Vector_4V bothVect = V4And(testVect1, testVect2);
		return V4IsEqualIntNone( bothVect, V4VConstant(V_MASKXYZW) );
	}

	__forceinline Vector_4V_Out V4MaxElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		Vector_4V z = V4SplatZ( inVector );
		Vector_4V w = V4SplatW( inVector );
		return V4Max( V4Max(x,y), V4Max(z,w) );
	}

	__forceinline Vector_4V_Out V4MinElement(Vector_4V_In inVector)
	{
		Vector_4V x = V4SplatX( inVector );
		Vector_4V y = V4SplatY( inVector );
		Vector_4V z = V4SplatZ( inVector );
		Vector_4V w = V4SplatW( inVector );
		return V4Min( V4Min(x,y), V4Min(z,w) );
	}

	__forceinline Vector_4V_Out V4Clamp( Vector_4V_In inVector, Vector_4V_In lowBound, Vector_4V_In highBound )
	{
		return V4Min( V4Max( inVector, lowBound ), highBound );
	}

	__forceinline Vector_4V_Out V4Saturate( Vector_4V_In inVector )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);

		return V4Min( V4Max( inVector, _zero ), _one );
	}

	__forceinline Vector_4V_Out V4SelectVect(Vector_4V_In choiceVectorX, Vector_4V_In zero, Vector_4V_In nonZero)
	{
		return V4SelectFT( V4SplatX(choiceVectorX), zero, nonZero );
	}

	__forceinline Vector_4V_Out V4SlowInOut( Vector_4V_In t )
	{
		Vector_4V _point_5 = V4VConstant(V_HALF);
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V clampedInput = V4Clamp( t, _zero, _one );

		// output = 0.5 * (1.0 - cos(t*PI)) = 0.5 - 0.5*cos(t*PI)
		Vector_4V theCos = V4Cos( V4Scale( clampedInput, V4VConstant(V_PI) ) );
		return V4SubtractScaled( _point_5, _point_5, theCos );	
	}

	__forceinline Vector_4V_Out V4SlowIn( Vector_4V_In t )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V clampedInput = V4Clamp( t, _zero, _one );

		// output = 1.0 - cos(0.5*t*PI) = 1.0 - cos(t*(PI/2))
		Vector_4V theCos = V4Cos( V4Scale( clampedInput, V4VConstant(V_PI_OVER_TWO) ) );
		return V4Subtract( _one, theCos );
	}

	__forceinline Vector_4V_Out V4SlowOut( Vector_4V_In t )
	{
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V clampedInput = V4Clamp( t, _zero, _one );

		// output = sin(0.5*t*PI) = sin(t*(PI/2))
		return V4Sin( V4Scale( clampedInput, V4VConstant(V_PI_OVER_TWO) ) );
	}

	__forceinline Vector_4V_Out V4BellInOut( Vector_4V_In t )
	{
		Vector_4V _point_5 = V4VConstant(V_HALF);
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V clampedInput = V4Clamp( t, _zero, _one );

		// output = 0.5( 1.0 - cos(t*(2*PI)) ) = 0.5 - 0.5*cos(t*(2*PI))
		Vector_4V theCos = V4Cos( V4Scale( clampedInput, V4VConstant(V_TWO_PI) ) );
		return V4SubtractScaled( _point_5, _point_5, theCos );
	}

	__forceinline Vector_4V_Out V4Range( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper )
	{
		//FastAssert( 0 != V4IsEqualIntAll( V4IsEqualV(lower, upper), V4VConstant(V_ZERO) ) &&
		//			"Divide-by-zero. Please don't set the range to nothing." );

		// (t-lower) / (upper-lower)
		Vector_4V top = V4Subtract( t, lower );
		Vector_4V bottom = V4Subtract( upper, lower );
		return V4InvScale( top, bottom );
	}

	__forceinline Vector_4V_Out V4RangeSafe( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper, Vector_4V_In_After3Args errValVect )
	{
		// (t-lower) / (upper-lower)
		Vector_4V top = V4Subtract( t, lower );
		Vector_4V bottom = V4Subtract( upper, lower );
		return V4InvScaleSafe( top, bottom, errValVect );
	}

	__forceinline Vector_4V_Out V4RangeClamp( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper )
	{
		//FastAssert( 0 != V4IsEqualIntAll( V4IsEqualV(lower, upper), V4VConstant(V_ZERO) ) &&
		//			"Divide-by-zero. Please don't set the range to nothing." );

		// Clamp[ (t-lower) / (upper-lower), 0, 1 ]
		Vector_4V top = V4Subtract( t, lower );
		Vector_4V bottom = V4Subtract( upper, lower );
		return V4Clamp( V4InvScale( top, bottom ), V4VConstant(V_ZERO), V4VConstant(V_ONE) );
	}

	__forceinline Vector_4V_Out V4Ramp( Vector_4V_In x, Vector_4V_In funcInA, Vector_4V_In funcInB, Vector_4V_In_After3Args funcOutA, Vector_4V_In_After3Args funcOutB )
	{
		Vector_4V t = V4RangeClamp( x, funcInA, funcInB );
		return V4Lerp( t, funcOutA, funcOutB );
	}

	__forceinline Vector_4V_Out V4RangeFast( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper )
	{
		//FastAssert( 0 != V4IsEqualIntAll( V4IsEqualV(lower, upper), V4VConstant(V_ZERO) ) &&
		//			"Divide-by-zero. Please don't set the range to nothing." );

		// (t-lower) / (upper-lower)
		Vector_4V top = V4Subtract( t, lower );
		Vector_4V bottom = V4Subtract( upper, lower );
		return V4InvScaleFast( top, bottom );
	}

	__forceinline Vector_4V_Out V4RangeClampFast( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper )
	{
		//FastAssert( 0 != V4IsEqualIntAll( V4IsEqualV(lower, upper), V4VConstant(V_ZERO) ) &&
		//			"Divide-by-zero. Please don't set the range to nothing." );

		// Clamp[ (t-lower) / (upper-lower), 0, 1 ]
		Vector_4V top = V4Subtract( t, lower );
		Vector_4V bottom = V4Subtract( upper, lower );
		return V4Clamp( V4InvScaleFast( top, bottom ), V4VConstant(V_ZERO), V4VConstant(V_ONE) );
	}

	__forceinline Vector_4V_Out V4RampFast( Vector_4V_In x, Vector_4V_In funcInA, Vector_4V_In funcInB, Vector_4V_In_After3Args funcOutA, Vector_4V_In_After3Args funcOutB )
	{
		Vector_4V t = V4RangeClampFast( x, funcInA, funcInB );
		return V4Lerp( t, funcOutA, funcOutB );
	}

	//============================================================================
	// Quaternions

	__forceinline Vector_4V_Out V4QuatDotV( Vector_4V_In inQuat1, Vector_4V_In inQuat2 )
	{
		return V4DotV( inQuat1, inQuat2 );
	}

	__forceinline float V4QuatDot( Vector_4V_In inQuat1, Vector_4V_In inQuat2 )
	{
		return V4Dot( inQuat1, inQuat2 );
	}

	__forceinline Vector_4V_Out V4QuatConjugate( Vector_4V_In inQuat )
	{
		Vector_4V negInput = V4Negate( inQuat );

		// The following is one vrlimi128 instruction on XBox360.
		return V4PermuteTwo<X1,Y1,Z1,W2>( negInput, inQuat ); // < -x,-y,-z, w >
	}

	__forceinline Vector_4V_Out V4QuatNormalize( Vector_4V_In inQuat )
	{
		return V4Normalize( inQuat );
	}

	__forceinline Vector_4V_Out V4QuatNormalizeSafe( Vector_4V_In inQuat, Vector_4V_In errValVect, Vector_4V_In magSqThreshold )
	{
		return V4NormalizeSafe( inQuat, errValVect, magSqThreshold );
	}

	__forceinline Vector_4V_Out V4QuatNormalizeFast( Vector_4V_In inQuat )
	{
		return V4NormalizeFast( inQuat );
	}

	__forceinline Vector_4V_Out V4QuatNormalizeFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect, Vector_4V_In magSqThreshold )
	{
		return V4NormalizeFastSafe( inQuat, errValVect, magSqThreshold );
	}

	__forceinline Vector_4V_Out V4QuatInvert( Vector_4V_In inQuat )
	{
		Vector_4V numerator = V4QuatConjugate( inQuat );
		Vector_4V denominator = V4MagSquaredV( inQuat );
		return V4InvScale( numerator, denominator );
	}

	__forceinline Vector_4V_Out V4QuatInvertSafe( Vector_4V_In inQuat, Vector_4V_In errValVect )
	{
		Vector_4V numerator = V4QuatConjugate( inQuat );
		Vector_4V denominator = V4MagSquaredV( inQuat );
		return V4InvScaleSafe( numerator, denominator, errValVect );
	}

	__forceinline Vector_4V_Out V4QuatInvertFast( Vector_4V_In inQuat )
	{
		Vector_4V numerator = V4QuatConjugate( inQuat );
		Vector_4V denominator = V4MagSquaredV( inQuat );
		return V4InvScaleFast( numerator, denominator );
	}

	__forceinline Vector_4V_Out V4QuatInvertFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect )
	{
		Vector_4V numerator = V4QuatConjugate( inQuat );
		Vector_4V denominator = V4MagSquaredV( inQuat );
		return V4InvScaleFastSafe( numerator, denominator, errValVect );
	}

	__forceinline Vector_4V_Out V4QuatInvertNormInput( Vector_4V_In inNormQuat )
	{
		return V4QuatConjugate( inNormQuat );
	}

	inline Vector_4V_Out V4QuatSlerpNear( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 )
	{
		mthAssertf( GetX(t)==GetY(t) && GetX(t)==GetZ(t) && GetX(t)==GetW(t) , "t input vector not uniform!" );

		//              inNormQuat1*sin((1-t)(theta)) + inNormQuat2*sin((t)(theta))
		// quatResult = -----------------------------------------------------------
		//                                       sin(theta)

		Vector_4V quatResult;
		Vector_4V negT;
		Vector_4V sinArg; // <(1-t)(theta), (t)(theta), ..., ...>
		Vector_4V theSin;
		Vector_4V theSin1;
		Vector_4V theSin2;

		Vector_4V theta;
		Vector_4V sinTheta;
		Vector_4V cosTheta;
		Vector_4V cosThetaLTZero;

		Vector_4V _signMask = V4VConstant(V_80000000);
		Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);

#define NEGATE( V ) V4Xor( (V), _signMask )
		// Find cos(theta) the quick way.
		cosTheta = V4QuatDotV( inNormQuat1, inNormQuat2 );

		// Correct for a long rotation.
		cosThetaLTZero = V4IsLessThanV( cosTheta, _zero );
		cosTheta = V4SelectFT( cosThetaLTZero, cosTheta, NEGATE(cosTheta) );

		// Compute sin(theta) from cos(theta). (sin = sqrt(1-cos^2)).
		sinTheta = V4Sqrt( V4SubtractScaled( _one, cosTheta, cosTheta ) );

		// Compute theta so that we can compute sin((1-t)(theta)) and sin((t)(theta)).
		// [ arctan( sin(theta)/cos(theta) ) = theta ]
		theta = V4Arctan2( sinTheta, cosTheta );

		// Compute the two sines at once.
		negT = NEGATE( t );
		sinArg = V4Add( negT, _one );
		sinArg = V4MergeXY( sinArg, t );
		sinArg = V4Scale( sinArg, theta );
		theSin = V4Sin( sinArg );
		theSin = V4InvScale( theSin, sinTheta ); // include the denominator

		// Extract the components.
		theSin1 = V4SplatX( theSin );
		theSin2 = V4SplatY( theSin );

		// Invert this sign if need be. (TODO: Not sure if this is necessary?)
		theSin2 = V4SelectFT( cosThetaLTZero, theSin2, NEGATE(theSin2) );

		// Now just scale by the input quats and add.
		quatResult = V4Scale( inNormQuat1, theSin1 );
		quatResult = V4AddScaled( quatResult, inNormQuat2, theSin2 );
		return V4Normalize( quatResult );
#undef NEGATE
	}

	inline Vector_4V_Out V4QuatSlerp( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 )
	{
		mthAssertf( GetX(t)==GetY(t) && GetX(t)==GetZ(t) && GetX(t)==GetW(t) , "t input vector not uniform!" );

		//              inNormQuat1*sin((1-t)(theta)) + inNormQuat2*sin((t)(theta))
		// quatResult = -----------------------------------------------------------
		//                                       sin(theta)

		Vector_4V quatResult;
		Vector_4V negT;
		Vector_4V sinArg; // <(1-t)(theta), (t)(theta), ..., ...>
		Vector_4V theSin;
		Vector_4V theSin1;
		Vector_4V theSin2;

		Vector_4V theta;
		Vector_4V sinTheta;
		Vector_4V cosTheta;
		//Vector_4V cosThetaLTZero;

		Vector_4V _signMask = V4VConstant(V_80000000);
		//Vector_4V _zero = V4VConstant(V_ZERO);
		Vector_4V _one = V4VConstant(V_ONE);

#define NEGATE( V ) V4Xor( (V), _signMask )
		// Find cos(theta) the quick way.
		cosTheta = V4QuatDotV( inNormQuat1, inNormQuat2 );

		// Correct for a long rotation.
		//cosThetaLTZero = V4IsLessThanV( cosTheta, _zero );
		//cosTheta = V4SelectFT( cosThetaLTZero, cosTheta, NEGATE(cosTheta) );

		// Compute sin(theta) from cos(theta). (sin = sqrt(1-cos^2)).
		sinTheta = V4Sqrt( V4SubtractScaled( _one, cosTheta, cosTheta ) );

		// Compute theta so that we can compute sin((1-t)(theta)) and sin((t)(theta)).
		// [ arctan( sin(theta)/cos(theta) ) = theta ]
		theta = V4Arctan2( sinTheta, cosTheta );

		// Compute the two sines at once.
		negT = NEGATE( t );
		sinArg = V4Add( negT, _one );
		sinArg = V4MergeXY( sinArg, t );
		sinArg = V4Scale( sinArg, theta );
		theSin = V4Sin( sinArg );
		theSin = V4InvScale( theSin, sinTheta ); // include the denominator

		// Extract the components.
		theSin1 = V4SplatX( theSin );
		theSin2 = V4SplatY( theSin );

		// Invert this sign if need be. (TODO: Not sure if this is necessary?)
		//theSin2 = V4SelectFT( cosThetaLTZero, theSin2, NEGATE(theSin2) );

		// Now just scale by the input quats and add.
		quatResult = V4Scale( inNormQuat1, theSin1 );
		quatResult = V4AddScaled( quatResult, inNormQuat2, theSin2 );
		return V4Normalize( quatResult );
#undef NEGATE
	}
	
	__forceinline Vector_4V_Out V4QuatNlerp( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 )
	{
		Vector_4V lerpedQuat = V4Lerp( t, inNormQuat1, inNormQuat2 );
		return V4QuatNormalize( lerpedQuat );
	}

	__forceinline Vector_4V_Out V4QuatNlerpFast( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 )
	{
		Vector_4V lerpedQuat = V4Lerp( t, inNormQuat1, inNormQuat2 );
		return V4QuatNormalizeFast( lerpedQuat );
	}

	__forceinline Vector_4V_Out V4QuatTwistAngle( Vector_4V_In inQuat, Vector_4V_In v )
	{
		Vector_4V tmpV = V3QuatRotate( v, inQuat );

		Vector_4V	tmpQ = V4QuatFromVectors( tmpV, v );
					tmpQ = V4QuatMultiply( tmpQ, inQuat );

		Vector_4V dot = V3DotV( v, tmpQ );

		Vector_4V _two = V4VConstant(V_TWO);
		Vector_4V outAngle = V4Scale( _two, V4Arctan2( dot, V4SplatW( tmpQ ) ) );

		Vector_4V _pi		= V4VConstant(V_PI);
		Vector_4V _neg_pi	= V4VConstant(V_NEG_PI);
		Vector_4V _two_pi	= V4VConstant(V_TWO_PI);

		outAngle = V4SelectFT(	V4IsGreaterThanV( outAngle, _pi )
								, V4SelectFT( V4IsLessThanV( outAngle, _neg_pi ), outAngle, V4Add( outAngle, _two_pi ) )
								, V4Subtract( outAngle, _two_pi )
							);
		return outAngle;
	}

	__forceinline Vector_4V_Out V4QuatScaleAngle( Vector_4V_In inQuat, Vector_4V_In scale )
	{
		Vector_4V _pi_over_two = V4VConstant(V_PI_OVER_TWO);
		Vector_4V _pi = V4VConstant(V_PI);
		Vector_4V _negone = V4VConstant(V_NEGONE);
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V _mask000F = V4VConstant(V_MASKW);
		Vector_4V _epsilon = V4VConstant(V_FLT_EPSILON);

		// convert to axis angle
		Vector_4V dotVector = V3MagSquaredV(inQuat);
		Vector_4V invSqrtVector = V4InvSqrt(dotVector);
		Vector_4V axis = V4Scale(inQuat, invSqrtVector);
		Vector_4V angle = V4Arccos(V4Clamp(V4SplatW(inQuat), _negone, _one));

		// scale the angle
		angle = IF_GT_THEN_ELSE(angle, _pi_over_two, V4Subtract(angle, _pi), angle);
		angle = V4Scale(scale, angle);

		// convert back to quaternion
		Vector_4V theCos, theSin;
		V4SinAndCos(theSin, theCos, angle);
		Vector_4V outQuat = V4SelectFT(_mask000F, V4Scale(axis, theSin), theCos);
		return IF_GT_THEN_ELSE(dotVector, _epsilon, outQuat, inQuat);
	}

	__forceinline Vector_4V_Out V4QuatFromAxisAngle( Vector_4V_In normAxis, Vector_4V_In radians )
	{
		Vector_4V theCos, theSin;
		Vector_4V _point_five = V4VConstant(V_HALF);
		Vector_4V _mask000F = V4VConstant(V_MASKW);

		V4SinAndCos( theSin, theCos, V4Scale( radians, _point_five ) );
		return V4SelectFT( _mask000F, V4Scale( normAxis, theSin ), theCos );
	}

	__forceinline Vector_4V_Out V4QuatFromXAxisAngle( Vector_4V_In radians )
	{
		Vector_4V theCos, theSin, theResult;
		Vector_4V _point_five = V4VConstant(V_HALF);
		Vector_4V _maskF00F = V4VConstant(V_MASKXW);

		V4SinAndCos( theSin, theCos, V4Scale( radians, _point_five ) );

		// resolves to MergeXY (1 instruction on all 3 platforms)
		theResult = V4PermuteTwo<X1,X2,Y1,Y2>( theSin, theCos );

		theResult = V4And( theResult, _maskF00F );

		return theResult;
	}

	__forceinline Vector_4V_Out V4QuatFromYAxisAngle( Vector_4V_In radians )
	{
		Vector_4V theCos, theSin, theResult;
		Vector_4V _point_five = V4VConstant(V_HALF);
		Vector_4V _mask0F0F = V4VConstant(V_MASKYW);

		V4SinAndCos( theSin, theCos, V4Scale( radians, _point_five ) );

		// resolves to __vsldoi()/vec_sld()/native Win32 shuffle (1 instruction on all 3 platforms)
		theResult = V4PermuteTwo<Z1,W1,X2,Y2>( theSin, theCos );

		theResult = V4And( theResult, _mask0F0F );

		return theResult;
	}

	__forceinline Vector_4V_Out V4QuatFromZAxisAngle( Vector_4V_In radians )
	{
		Vector_4V theCos, theSin, theResult;
		Vector_4V _point_five = V4VConstant(V_HALF);
		Vector_4V _mask00FF = V4VConstant(V_MASKZW);

		V4SinAndCos( theSin, theCos, V4Scale( radians, _point_five ) );
		
		// resolves to MergeXY (1 instruction on all 3 platforms)
		theResult = V4PermuteTwo<X1,X2,Y1,Y2>( theSin, theCos );

		theResult = V4And( theResult, _mask00FF );

		return theResult;
	}

	__forceinline void V4QuatToAxisAngle( Vector_4V_InOut axis, Vector_4V_InOut radians, Vector_4V_In inQuat )
	{
		Vector_4V _negone	= V4VConstant(V_NEGONE);
		Vector_4V _one		= V4VConstant(V_ONE);
		Vector_4V _two		= V4VConstant(V_TWO);

		axis = V4NormalizeSafe(V4And(inQuat, V4VConstant(V_MASKXYZ)), V4VConstant(V_ZERO));
		radians = V4Scale( _two, V4Arccos( V4Clamp(V4SplatW(inQuat), _negone, _one) ) );
	}

	__forceinline Vector_4V_Out V4QuatGetAngle( Vector_4V_In inQuat )
	{
		inQuat = V4SplatW(inQuat);

		// This is what I used to have here. But I think the assert is just going to annoy people. The clamp is what the old lib did anyways.
		//ASSERT_ONLY( Vector_4V _negone = V4VConstant(V_NEGONE); )
		//ASSERT_ONLY( Vector_4V _one = V4VConstant(V_ONE); )
		//if( !(V4IsGreaterThanOrEqualAll(inQuat, _negone) != 0 && V4IsLessThanOrEqualAll(inQuat, _one) != 0) )
		//	Vec::V4Print( inQuat );
		//mthAssert(	V4IsGreaterThanOrEqualAll(inQuat, _negone) != 0 && V4IsLessThanOrEqualAll(inQuat, _one) != 0 &&
		//		"Note that your quat's 'W' is invalid for the Arccos you're about to do... it will be clamped valid for __ASSERT builds only!" );
		//ASSERT_ONLY( inQuat = V4Clamp( inQuat, _negone, _one ); )

		Vector_4V _negone	= V4VConstant(V_NEGONE);
		Vector_4V _one		= V4VConstant(V_ONE);
		inQuat = V4Clamp( inQuat, _negone, _one );
		
		Vector_4V _two		= V4VConstant(V_TWO);
		return V4Scale( _two, V4Arccos( inQuat ) );
	}


	__forceinline Vector_4V_Out V4QuatRelAngle( Vector_4V_In inQuat1, Vector_4V_In inQuat2 )
	{
		Vector_4V _negone	= V4VConstant(V_NEGONE);
		Vector_4V _one		= V4VConstant(V_ONE);
		Vector_4V _zero		= V4VConstant(V_ZERO);
		Vector_4V _two		= V4VConstant(V_TWO);
		Vector_4V c			= V4DotV( inQuat1, inQuat2 );

		return V4SelectFT( V4IsLessThanV( c, _negone ), V4SelectFT( V4IsGreaterThanV( c, _one ), V4Scale( _two, V4Arccos( c ) ), _zero ), _zero );
	}


	__forceinline Vector_4V_Out V4QuatFromAxis_AngleFromVectors( Vector_4V_In fromN, Vector_4V_In toN, Vector_4V_In axis, Vector_4V_In_After3Args dot_in)
	{
		Vector_4V cross		= V3Cross( fromN, toN );
		Vector_4V __dot		= V3DotV( cross, axis );
		Vector_4V _zero		= V4VConstant(V_ZERO);			// zero threshold
		Vector_4V __axis	= V4SelectFT( V4IsGreaterThanV( __dot, _zero ), V4Negate(axis), axis );
		return V4QuatFromAxisAngle( __axis, V4Arccos(V4Clamp( dot_in, V4VConstant(V_NEGONE), V4VConstant(V_ONE) )) );
	}


	__forceinline Vector_4V_Out V4QuatIdentity()
	{
		return V4VConstant(V_ZERO_WONE);			// identity quaternion;
	}

	__forceinline Vector_4V_Out V4QuatFromVectors(  Vector_4V_In from, Vector_4V_In to, Vector_4V_In axis )
	{
		Vector_4V scaleFrom	= V3DotV( from, axis );
		Vector_4V scaleTo	= V3DotV( to, axis );
		Vector_4V fromN		= V4SubtractScaled( from, axis, scaleFrom ),
					toN		= V4SubtractScaled( to, axis, scaleTo );

		fromN	= V3NormalizeFast( fromN );
		toN		= V3NormalizeFast( toN );

		Vector_4V dot = V3DotV( fromN, toN );

		Vector_4V _one			= V4VConstant(V_ONE);
		Vector_4V _flt_small6	= V4VConstant(V_FLT_SMALL_6);		
		Vector_4V _abs_negone	= V4Abs( V4Subtract( dot, _one) );

		if( V4IsLessThanAll( _abs_negone, _flt_small6 ) )
			return V4QuatIdentity();

		Vector_4V _abs_one		= V4Abs( V4Add( dot, _one) );
		if( V4IsLessThanAll( _abs_one, _flt_small6 ) )
		{
			return V4QuatFromAxisAngle( axis, V4VConstant(V_PI) );
		}

		return V4QuatFromAxis_AngleFromVectors( fromN, toN, axis, dot );
	}

	__forceinline Vector_4V_Out V4QuatFromAxis_AxisFromIndex( Vector_4V_In fromN, Vector_4V_In toN, Vector_4V_In dot )
	{
		Vector_4V _out = V3Cross( fromN, toN );
		_out = Vec::V4PermuteTwo<Vec::X2,Vec::Y2,Vec::Z2,Vec::X1>( V4Add( dot, V4VConstant(V_ONE) ), _out );			// set out.w to dot + 1.0f
		return V4Normalize( _out );
	}

	__forceinline Vector_4V_Out V4QuatAxisFromIndex(  Vector_4V_In from )
	{
		Vector_4V _y_axis = V4VConstant(V_Y_AXIS_WZERO);
		Vector_4V _z_axis = V4VConstant(V_Z_AXIS_WZERO);

		Vector_4V _x = V4SplatX( from );
		Vector_4V _y = V4SplatY( from );
		Vector_4V _z = V4SplatZ( from );

		return V4SelectFT( V4IsLessThanV(_x,_y), V4SelectFT( V4IsLessThanV(_x,_z), _z_axis, _y_axis ), V4SelectFT( V4IsLessThanV(_y,_z), _z_axis, _y_axis ) );
	}

	__forceinline Vector_4V_Out V4QuatFromVectors(  Vector_4V_In from, Vector_4V_In to )
	{
		Vector_4V	fromN	= V3NormalizeFast( from ),
					toN		= V3NormalizeFast( to );

		Vector_4V _one			= V4VConstant(V_ONE);
		Vector_4V _flt_small6	= V4VConstant(V_FLT_SMALL_6);

		Vector_4V dot			= V3DotV( fromN, toN );
		Vector_4V _abs_negone	= V4Abs( V4Subtract( dot, _one) );
		Vector_4V _abs_one		= V4Abs( V4Add( dot, _one) );

		if( V4IsLessThanAll( _abs_negone, _flt_small6 ) )
			return V4QuatIdentity();

		return V4QuatFromAxis_AxisFromIndex( fromN, V4SelectFT( V4IsLessThanV( _abs_one, _flt_small6 ), toN, V4QuatAxisFromIndex(from) ), dot);
	}

	__forceinline Vector_4V_Out V4QuatGetUnitDirection( Vector_4V_In inQuat )
	{
		Vector_4V inVect = inQuat;
		Vector_4V outVect = V3Normalize( inVect );
		return outVect;
	}

	__forceinline Vector_4V_Out V4QuatGetUnitDirectionFast( Vector_4V_In inQuat )
	{
		Vector_4V inVect = inQuat;
		Vector_4V outVect = V3NormalizeFast( inVect );
		return outVect;
	}
	
	__forceinline Vector_4V_Out V4QuatGetUnitDirectionSafe( Vector_4V_In inQuat, Vector_4V_In errValVect )
	{
		Vector_4V outVect = inQuat;
		Vector_4V magSq = V3MagSquaredV( outVect );
		Vector_4V isGTEpsilon = V4IsGreaterThanV( magSq, V4VConstant(V_FLT_SMALL_12) );
		outVect = V4Scale( outVect, V4InvSqrt( magSq ) );
		return V4SelectFT( isGTEpsilon, errValVect, outVect );
	}

	__forceinline Vector_4V_Out V4QuatGetUnitDirectionFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect )
	{
		Vector_4V outVect = inQuat;
		Vector_4V magSq = V3MagSquaredV( outVect );
		Vector_4V isGTEpsilon = V4IsGreaterThanV( magSq, V4VConstant(V_FLT_SMALL_12) );
		outVect = V4Scale( outVect, V4InvSqrtFast( magSq ) );
		return V4SelectFT( isGTEpsilon, errValVect, outVect );
	}

	__forceinline Vector_4V_Out V4QuatPrepareSlerp( Vector_4V_In quat1, Vector_4V_In quatToNegate )
	{
		Vector_4V dotP = V4DotV( quat1, quatToNegate );
		Vector_4V dotPLTZero = V4IsLessThanV( dotP, V4VConstant(V_ZERO) );
		return V4SelectFT( dotPLTZero, quatToNegate, V4Negate(quatToNegate) );
	}

	//============================================================================
	// Trigonometry

	// Forceinlined, since if not, you're actually better off using V4Sin() then V4Cos(), b/c of returning by reference.
	__forceinline void V4SinAndCos( Vector_4V_InOut inOutSine, Vector_4V_InOut inOutCosine, Vector_4V_In inVector )
	{
#if 0
		// Find the # of PI's in the input, round to nearest.
		// "#PI = inVect * 1/PI"
		Vector_4V numPi = V4Scale( inVector, V4VConstant(V_ONE_OVER_PI) );
		numPi = V4RoundToNearestInt( numPi );
		Vector_4V isEven = V4IsEvenV( numPi );

		// The actual sine.
		Vector_4V sineVect = Util::V4SinHelper( inVector, numPi );

		// We need to invert the sign of the result if 'numPi' was an odd number.
		inOutSine = V4SelectFT( isEven, V4Negate(sineVect), sineVect );

		// sin^2 + cos^2 = 1 ==> cos = (+-)sqrt( Max(0,1-sin^2) ). "(+-)" handled in the next step.
		inOutCosine = V4Sqrt( V4Max( V4VConstant(V_ZERO), V4SubtractScaled( V4VConstant(V_ONE), inOutSine, inOutSine ) ) );
		// Invert the sign if 'numPi' was an odd number.
		inOutCosine = V4SelectFT( isEven, V4Negate(inOutCosine), inOutCosine );
#else
		Vector_4V xl,xl2,xl3;
		Vector_4V q;
		Vector_4V offsetSin, offsetCos;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V INT_1 = V4VConstant(V_INT_1);
		Vector_4V _2_2_2_2 = V4VConstant(V_INT_2);
		Vector_4V _zero = V4VConstant(V_ZERO);

		xl = V4Scale(inVector, V4VConstant(V_TWO_OVER_PI));

		xl = V4Add(xl,V4SelectFT(INT_80000000,V4VConstant(V_HALF),xl));
		q = V4FloatToIntRaw<0>(xl);

		offsetSin = V4And(q,V4VConstant(V_INT_3));
		offsetCos = V4AddInt( INT_1, offsetSin );

		Vector_4V qf = V4IntToFloatRaw<0>(q);
		Vector_4V p1 = V4SubtractScaled(inVector,V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(),qf);
		xl  = V4SubtractScaled(p1,V4VConstant<0x33A22169,0x33A22169,0x33A22169,0x33A22169>(),qf);

		xl2 = V4Scale(xl,xl);
		xl3 = V4Scale(xl2,xl);
	    
		Vector_4V ct1 = V4AddScaled(V4VConstant<0x3D2AA036,0x3D2AA036,0x3D2AA036,0x3D2AA036>(),V4VConstant<0xBAB24993,0xBAB24993,0xBAB24993,0xBAB24993>(),xl2);
		Vector_4V st1 = V4AddScaled(V4VConstant<0x3C088342,0x3C088342,0x3C088342,0x3C088342>(),V4VConstant<0xB94C8C6E,0xB94C8C6E,0xB94C8C6E,0xB94C8C6E>(),xl2);

		Vector_4V ct2 = V4AddScaled(V4VConstant<0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF>(),ct1,xl2);
		Vector_4V st2 = V4AddScaled(V4VConstant<0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1>(),st1,xl2);
	    
		Vector_4V cx = V4AddScaled(V4VConstant(V_ONE),ct2,xl2);
		Vector_4V sx = V4AddScaled(xl,st2,xl3);

		Vector_4V sinMask = V4IsEqualIntV(V4And(offsetSin,INT_1),_zero);
		Vector_4V cosMask = V4IsEqualIntV(V4And(offsetCos,INT_1),_zero);    
		Vector_4V sinOutTemp;
		Vector_4V cosOutTemp;
		sinOutTemp = V4SelectFT(sinMask,cx,sx);
		cosOutTemp = V4SelectFT(cosMask,cx,sx);

		sinMask = V4IsEqualIntV(V4And(offsetSin,_2_2_2_2),_zero);
		cosMask = V4IsEqualIntV(V4And(offsetCos,_2_2_2_2),_zero);
	    
		inOutSine = V4SelectFT(sinMask,V4Xor(INT_80000000,sinOutTemp),sinOutTemp);
		inOutCosine = V4SelectFT(cosMask,V4Xor(INT_80000000,cosOutTemp),cosOutTemp);
#endif
	}

	inline Vector_4V_Out V4Sin( Vector_4V_In inVector )
	{
#if 1
		// Find the # of PI's in the input, round to nearest.
		// "#PI = inVect * 1/PI"
		Vector_4V numPi = V4Scale( inVector, V4VConstant(V_ONE_OVER_PI) );
		numPi = V4RoundToNearestInt( numPi );

		// The actual sine.
		Vector_4V sineVect = Util::V4SinHelper( inVector, numPi );

		// We need to invert the sign of the result if 'numPi' was an odd number.
		return V4SelectFT( V4IsEvenV(numPi), V4Negate(sineVect), sineVect );
#else
		Vector_4V	xl,xl2,xl3,res;
		Vector_4V   q;

		xl = V4Scale(inVector, V4VConstant(V_TWO_OVER_PI));

		//xl = V4Add(xl,V4SelectFT(V4VConstant(V_80000000),V4VConstant(V_HALF),xl)); // The next line is faster for repeated calls.
		xl = V4Add(xl,V4SelectFT(V4VConstant<0x80000000,0x80000000,0x80000000,0x80000000>(),V4VConstant<U32_HALF,U32_HALF,U32_HALF,U32_HALF>(),xl));
		q = V4FloatToIntRaw<0>(xl);

		Vector_4V offset = V4And(q,V4VConstant(V_INT_3));

		Vector_4V qf = V4IntToFloatRaw<0>(q);
		Vector_4V p1 = V4SubtractScaled(inVector,qf,V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>());
		xl  = V4SubtractScaled(p1,qf,V4VConstant<0x33A22169,0x33A22169,0x33A22169,0x33A22169>());
	   
		xl2 = V4Scale(xl,xl);
		xl3 = V4Scale(xl2,xl);
	    
		Vector_4V ct1 = V4AddScaled(V4VConstant<0x3D2AA036,0x3D2AA036,0x3D2AA036,0x3D2AA036>(),V4VConstant<0xBAB24993,0xBAB24993,0xBAB24993,0xBAB24993>(),xl2);
		Vector_4V st1 = V4AddScaled(V4VConstant<0x3C088342,0x3C088342,0x3C088342,0x3C088342>(),V4VConstant<0xB94C8C6E,0xB94C8C6E,0xB94C8C6E,0xB94C8C6E>(),xl2);

		Vector_4V ct2 = V4AddScaled(V4VConstant<0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF>(),ct1,xl2);
		Vector_4V st2 = V4AddScaled(V4VConstant<0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1>(),st1,xl2);
	    
		//Vector_4V cx = V4AddScaled(V4VConstant(V_ONE),ct2,xl2);
		Vector_4V cx = V4AddScaled(V4VConstant<U32_ONE,U32_ONE,U32_ONE,U32_ONE>(),ct2,xl2);
		Vector_4V sx = V4AddScaled(xl,st2,xl3);

		Vector_4V mask1 = V4IsEqualIntV(V4And(offset, V4VConstant(V_INT_1)), V4VConstant(V_ZERO));
		res = V4SelectFT(mask1,cx,sx);

		Vector_4V mask2 = V4IsEqualIntV(V4And(offset,V4VConstant(V_INT_2)),V4VConstant(V_ZERO));
		//res = V4SelectFT(V4Xor(V4VConstant(V_80000000),res),res,mask2);
		res = V4SelectFT(V4Xor(V4VConstant<0x80000000,0x80000000,0x80000000,0x80000000>(),res),res,mask2);

		return res;
#endif
	}

	inline Vector_4V_Out V4Cos( Vector_4V_In inVector )
	{
#if 1

		// Find the # of PI's in the input, round to nearest (towards -infinity).
		// "#PI = inVect * 1/PI"
		Vector_4V numPi = V4Scale( inVector, V4VConstant(V_ONE_OVER_PI) );
		numPi = V4RoundToNearestIntNegInf( numPi );

		Vector_4V sineNumPi = V4Add( numPi, V4VConstant(V_HALF) );

		// The actual sine.
		Vector_4V sineVect = Util::V4SinHelper( inVector, sineNumPi );

		// We need to invert the sign of the result if 'numPi' was an even number.
		return V4SelectFT( V4IsEvenV(numPi), sineVect, V4Negate(sineVect) );
#else
		Vector_4V xl,xl2,xl3,res;
		Vector_4V q;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V _zero = V4VConstant(V_ZERO);

		xl = V4Scale(inVector, V4VConstant(V_TWO_OVER_PI));

		xl = V4Add(xl,V4SelectFT(INT_80000000,V4VConstant(V_HALF),xl));
		q = V4FloatToIntRaw<0>(xl);

		Vector_4V offset = V4Add(V4VConstant(V_INT_1),V4And(q,V4VConstant(V_INT_3)));

		Vector_4V qf = V4IntToFloatRaw<0>(q);
		Vector_4V p1 = V4SubtractScaled(inVector,V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(),qf);
		xl  = V4SubtractScaled(p1,V4VConstant<0x33A22169,0x33A22169,0x33A22169,0x33A22169>(),qf);

		xl2 = V4Scale(xl,xl);
		xl3 = V4Scale(xl2,xl);
	    
		Vector_4V ct1 = V4AddScaled(V4VConstant<0x3D2AA036,0x3D2AA036,0x3D2AA036,0x3D2AA036>(),V4VConstant<0xBAB24993,0xBAB24993,0xBAB24993,0xBAB24993>(),xl2);
		Vector_4V st1 = V4AddScaled(V4VConstant<0x3C088342,0x3C088342,0x3C088342,0x3C088342>(),V4VConstant<0xB94C8C6E,0xB94C8C6E,0xB94C8C6E,0xB94C8C6E>(),xl2);

		Vector_4V ct2 = V4AddScaled(V4VConstant<0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF>(),ct1,xl2);
		Vector_4V st2 = V4AddScaled(V4VConstant<0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1>(),st1,xl2);
	    
		Vector_4V cx = V4AddScaled(V4VConstant(V_ONE),ct2,xl2);
		Vector_4V sx = V4AddScaled(xl,st2,xl3);

		Vector_4V mask1 = V4IsEqualIntV(V4And(offset, V4VConstant(V_INT_1)), _zero);
		res = V4SelectFT(mask1,cx,sx);

		Vector_4V mask2 = V4IsEqualIntV(V4And(offset,V4VConstant(V_INT_2)), _zero);
		res = V4SelectFT(V4Xor(INT_80000000, res), res, mask2);

		return res;
#endif
	}

	inline Vector_4V_Out V4Tan( Vector_4V_In inVector )
	{
#if 1
		// Only accurate in [-PI/4, PI/4] range.

		Vector_4V identityVect = V4VConstant(V_ONE);

		Vector_4V numPi = V4Scale( inVector, V4VConstant(V_TWO_OVER_PI) );
		numPi = V4RoundToNearestInt( numPi );

		Vector_4V inVectModded = V4AddScaled( inVector, V4VConstant(V_NEG_PI_OVER_TWO), numPi );
		
		// A little more -PI/2 accuracy.
		//inVectModded = V4AddScaled( inVectModded, V4VConstant<0xB3A22168U,0xB3A22168U,0xB3A22168U,0xB3A22168U>(), numPi );
		// Even more -PI/2 accuracy.
		//inVectModded = V4AddScaled( inVectModded, V4VConstant<0xBFC90FDAU,0xBFC90FDAU,0xBFC90FDAU,0xBFC90FDAU>(), numPi );

		// Approximation (see macstl).
		Vector_4V vectSq = V4Scale( inVectModded, inVectModded );
		Vector_4V numerator = V4AddScaled( identityVect, V4VConstant<0xBDC433B8U,0xBDC433B8U,0xBDC433B8U,0xBDC433B8U>(), vectSq);
		numerator = V4Scale( numerator, inVectModded );
		Vector_4V denominator = V4AddScaled( V4VConstant<0xBEDBB7AFU,0xBEDBB7AFU,0xBEDBB7AFU,0xBEDBB7AFU>(), V4VConstant<0x3C1F3374U,0x3C1F3374U,0x3C1F3374U,0x3C1F3374U>(), vectSq );
		denominator = V4AddScaled( identityVect, denominator, vectSq );

		// If 'numPi' was an even number:	we need numerator/denominator
		// If 'numPi' was an odd number:	we need -denominator/numerator
		Vector_4V isEven = V4IsEvenV(numPi);
		Vector_4V zeroVect =  V4VConstant(V_ZERO);

		return V4SelectFT(	V4IsEqualV( inVector, zeroVect ),	// Prevent divide-by-zero.
							V4InvScale( V4SelectFT( isEven, V4Negate(denominator), numerator ),
										V4SelectFT( isEven, numerator, denominator ) ),
							zeroVect );
#else
		Vector_4V xl,xl2,xl3,res;
		Vector_4V   q;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V _zero = V4VConstant(V_ZERO);

		xl = V4Scale(inVector, V4VConstant(V_TWO_OVER_PI));

		xl = V4Add(xl,V4SelectFT(INT_80000000,V4VConstant(V_HALF),xl));
		q = V4FloatToIntRaw<0>(xl);

		Vector_4V qf = V4IntToFloatRaw<0>(q);
		Vector_4V p1 = V4SubtractScaled(inVector,V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(),qf);
		xl  = V4SubtractScaled(p1,V4VConstant<0x33A22169,0x33A22169,0x33A22169,0x33A22169>(),qf);
	    
		xl2 = V4Scale(xl,xl);
		xl3 = V4Scale(xl2,xl);
	 
		Vector_4V ct2 = V4AddScaled(V4VConstant<0xBEDBB51E,0xBEDBB51E,0xBEDBB51E,0xBEDBB51E>(),V4VConstant<0x3C1F166D,0x3C1F166D,0x3C1F166D,0x3C1F166D>(),xl2);
		Vector_4V cx = V4AddScaled(V4VConstant(V_ONE),ct2,xl2);
		Vector_4V sx = V4AddScaled(xl,V4VConstant<0xBDC42983,0xBDC42983,0xBDC42983,0xBDC42983>(),xl3);

		Vector_4V cxosx = V4InvScale(cx,sx);
		Vector_4V sxocx = V4InvScale(sx,cx);

		Vector_4V ncxosx = V4Xor(INT_80000000, cxosx);

		Vector_4V mask = V4IsEqualIntV(V4And(q,V4VConstant(V_INT_1)), _zero);
		res = V4SelectFT(mask,ncxosx,sxocx);

		return res;
#endif
	}

	inline Vector_4V_Out V4Arcsin( Vector_4V_In inVector )
	{
#if 0

		Vector_4V _15_over_336 = V4VConstant<0x3D36DB6E,0x3D36DB6E,0x3D36DB6E,0x3D36DB6E>();
		Vector_4V _3_over_40 = V4VConstant<0x3D99999A,0x3D99999A,0x3D99999A,0x3D99999A>();
		Vector_4V _1_over_6 = V4VConstant<0x3E2AAAAB,0x3E2AAAAB,0x3E2AAAAB,0x3E2AAAAB>();
		Vector_4V _1 = V4VConstant(V_ONE);
		Vector_4V _pi_over_2 = V4VConstant(V_PI_OVER_TWO);
		Vector_4V _point_seven_one = V4VConstant<0x3F35C28F,0x3F35C28F,0x3F35C28F,0x3F35C28F>();

		// Approximation 1 (for |x| <= .71)
		Vector_4V vectSq = V4Scale( inVector, inVector );
		Vector_4V vectArcSin1 = V4AddScaled( _3_over_40, vectSq, _15_over_336 );
		vectArcSin1 = V4AddScaled( _1_over_6, vectSq, vectArcSin1 );
		vectArcSin1 = V4AddScaled( _1, vectSq, vectArcSin1 );
		vectArcSin1 = V4Scale( inVector, vectArcSin1 );

		// Approximation 2 (for |x| > .71)
		Vector_4V input2 = V4Sqrt( V4Subtract( _1, vectSq ) );
		Vector_4V vectSq2 = V4Scale( input2, input2 );
		Vector_4V vectArcSin2 = V4AddScaled( _3_over_40, vectSq2, _15_over_336 );
		vectArcSin2 = V4AddScaled( _1_over_6, vectSq2, vectArcSin2 );
		vectArcSin2 = V4AddScaled( _1, vectSq2, vectArcSin2 );
		vectArcSin2 = V4SubtractScaled( _pi_over_2, input2, vectArcSin2 );

		// Choose the appropriate approximation.
		Vector_4V isGTPointSevenOne = V4IsGreaterThanV( inVector, _point_seven_one );
		Vector_4V isLTNegPointSevenOne = V4IsLessThanV( inVector, V4Negate(_point_seven_one) );

		return V4SelectFT(	isGTPointSevenOne,
							V4SelectFT( isLTNegPointSevenOne, vectArcSin1, V4Negate(vectArcSin2) ),
							vectArcSin2		);
#elif 0

		Vector_4V x = inVector;
		Vector_4V constVec0 = V4VConstant<0xC0B18D0B,0x3F6F1640,0xBF012065,0x40B350B8>();
		Vector_4V constVec1 = V4VConstant<U32_HALF,0xBF000000,0xC0000000,0x3FC90FDB>();
		Vector_4V _half = V4SplatX(constVec1);

		Vector_4V positive = V4IsGreaterThanV(x, V4VConstant(V_ZERO));

		x = V4Abs(x);

		Vector_4V gtHalf = V4IsGreaterThanV(x, _half);

		Vector_4V g = V4SelectFT(gtHalf, V4Scale(x,x),V4AddScaled(_half,V4SplatY(constVec1),x));

		x = V4SelectFT(gtHalf,x,V4Scale(V4SplatZ(constVec1),V4Sqrt(g)));

		//Vector_4V denom = V4Add(g,V4VConstant<0xC0B18D0B,0xC0B18D0B,0xC0B18D0B,0xC0B18D0B>());
		//Vector_4V num = V4AddScaled(V4VConstant<0x3F6F1640,0x3F6F1640,0x3F6F1640,0x3F6F1640>(),V4VConstant<0xBF012065,0xBF012065,0xBF012065,0xBF012065>(),g);
		//denom = V4AddScaled(V4VConstant<0x40B350B8,0x40B350B8,0x40B350B8,0x40B350B8>(),denom,g);
		Vector_4V denom = V4Add(g,V4SplatX(constVec0));
		Vector_4V num = V4AddScaled(V4SplatY(constVec0),V4SplatZ(constVec0),g);
		denom = V4AddScaled(V4SplatW(constVec0),denom,g);
		num = V4Scale(V4Scale(x,g),num);

		x = V4Add(x, V4InvScale(num,denom));

		x = V4SelectFT(gtHalf,x,V4Add(x,V4SplatW(constVec1)));

		x = V4SelectFT(positive, V4Xor(V4VConstant(V_80000000), x), x);

		return x;

#else

		// {-0.05806367563904f, -0.41861972469416f, 0.22480114791621f, 2.17337241360606f}
		Vector_4V constVec0 = V4VConstant<0xBD6DD42D,0xBED65553,0x3E663246,0x400B1889>();
		// {0.61657275907170f, 4.29696498283455f, -1.18942822255452f, -6.53784832094831f}
		Vector_4V constVec1 = V4VConstant<0x3F1DD7B6,0x408980BD,0xBF983F2F,0xC0D1360E>();
		// {-1.36926553863413f, -4.48179294237210f, 1.41810672941833f, 5.48179257935713f}
		Vector_4V constVec2 = V4VConstant<0xBFAF4418,0xC08F6AD9,0x3FB58485,0x40AF6AD8>();

		
		Vector_4V V2, V3, AbsV;
		Vector_4V C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11;
		Vector_4V R0, R1, R2;
		Vector_4V OneMinusAbsV, HalfOneMinusAbsV;
		Vector_4V Rsq, Scale, H;
		Vector_4V SignMask;
		Vector_4V OnePlusEpsilon = V4VConstant(V_ONE_PLUS_EPSILON);
		
		Vector_4V V = inVector;

		SignMask = V4VConstant(V_80000000);

		V2 = V4Scale(V, V);
		AbsV = V4Andc(V, SignMask);

		C3 = V4SplatW(constVec0);
		C7 = V4SplatW(constVec1);
		C1 = V4SplatY(constVec0);
		C5 = V4SplatY(constVec1);
		C2 = V4SplatZ(constVec0);
		C6 = V4SplatZ(constVec1);
		C0 = V4SplatX(constVec0);
		C4 = V4SplatX(constVec1);

		OneMinusAbsV = V4Subtract(OnePlusEpsilon, AbsV);

		H = V4VConstant(V_INT_1);

		C7 = V4AddScaled(C7, C3, AbsV);
		C5 = V4AddScaled(C5, C1, AbsV);
		C6 = V4AddScaled(C6, C2, AbsV);
		C4 = V4AddScaled(C4, C0, AbsV);

		V3 = V4Scale(V2, AbsV);

		H = V4IntToFloatRaw<1>(H);

		C11 = V4SplatW(constVec2);
		C9 = V4SplatY(constVec2);
		C10 = V4SplatZ(constVec2);
		C8 = V4SplatX(constVec2);

		Rsq = V4InvSqrtFast(OneMinusAbsV);
		HalfOneMinusAbsV = V4Scale(OneMinusAbsV, H);

		R2 = V;

		C11 = V4AddScaled(C11, C7, AbsV);
		C9 = V4AddScaled(C9, C5, AbsV);
		C10 = V4AddScaled(C10, C6, AbsV);
		C8 = V4AddScaled(C8, C4, AbsV);

		Scale = V4Scale(Rsq, Rsq);

		R2 = V4SubtractScaled(R2, R2, AbsV);

		C11 = V4AddScaled(C11, C10, V3);
		C9 = V4AddScaled(C9, C8, V3);

		H = V4SubtractScaled(H, Scale, HalfOneMinusAbsV);

		R0 = V4Scale(V, C11);
		R1 = V4Scale(R2, C9);

		Rsq = V4AddScaled(Rsq, Rsq, H);

		R0 = V4AddScaled(R0, R1, Rsq);

		return R0;
#endif
	}

	inline Vector_4V_Out V4Arccos( Vector_4V_In inVector )
	{
#if 0
		return V4Subtract( V4VConstant(V_PI_OVER_TWO), V4Arcsin( inVector ) );
#elif 1

		Vector_4V result, xabs;
		Vector_4V t1;
		Vector_4V xabs2, xabs4;
		Vector_4V hi, lo;
		Vector_4V neg, pos;
		Vector_4V select;
		Vector_4V x = inVector;

		xabs = V4Abs(x);
		select = V4ShiftRightAlgebraic<31>( x );

		t1 = V4Sqrt(V4Subtract(V4VConstant(V_ONE), xabs));

		Vector_4V constVec0 = V4VConstant<0x3BDA90C5,0xBAA57A2C,0xBC8BFC66,0x3CFD10F8>();
		Vector_4V constVec1 = V4VConstant<0x3DB63A9E,0xBD4D8392,0xBE5BBFCA,0x3FC90FDA>();


		xabs2 = V4Scale(xabs,  xabs);
		xabs4 = V4Scale(xabs2, xabs2);
		//hi = V4AddScaled(V4VConstant<0x3BDA90C5,0x3BDA90C5,0x3BDA90C5,0x3BDA90C5>(), V4VConstant<0xBAA57A2C,0xBAA57A2C,0xBAA57A2C,0xBAA57A2C>(), xabs);
		//hi = V4AddScaled(V4VConstant<0xBC8BFC66,0xBC8BFC66,0xBC8BFC66,0xBC8BFC66>(), hi, xabs);
		//hi = V4AddScaled(V4VConstant<0x3CFD10F8,0x3CFD10F8,0x3CFD10F8,0x3CFD10F8>(), hi, xabs);
		//lo = V4AddScaled(V4VConstant<0x3DB63A9E,0x3DB63A9E,0x3DB63A9E,0x3DB63A9E>(), V4VConstant<0xBD4D8392,0xBD4D8392,0xBD4D8392,0xBD4D8392>(), xabs);
		//lo = V4AddScaled(V4VConstant<0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA>(), lo, xabs);
		//lo = V4AddScaled(V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(), lo, xabs);
		hi = V4AddScaled(V4SplatX(constVec0), V4SplatY(constVec0), xabs);
		hi = V4AddScaled(V4SplatZ(constVec0), hi, xabs);
		hi = V4AddScaled(V4SplatW(constVec0), hi, xabs);
		lo = V4AddScaled(V4SplatX(constVec1), V4SplatY(constVec1), xabs);
		lo = V4AddScaled(V4SplatZ(constVec1), lo, xabs);
		lo = V4AddScaled(V4SplatW(constVec1), lo, xabs);

		result = V4AddScaled(lo, hi, xabs4);

		/* Adjust the result if x is negative.
		*/
		neg = V4SubtractScaled(V4VConstant(V_PI), result, t1);
		pos = V4Scale(t1, result);

		result = V4SelectFT(select, pos, neg);

		return result;

#else
		return xb::XMVectorACos( inVector );

#endif
	}

	inline Vector_4V_Out V4Arctan( Vector_4V_In inVector )
	{
#if 0
		// source: http://mathworld.wolfram.com/InverseTangent.html

		Vector_4V _1 = V4VConstant(V_ONE);
		Vector_4V _point_5 = V4VConstant(V_HALF);
		Vector_4V _pi_over_2 = V4VConstant(V_PI_OVER_TWO);

		Vector_4V a0 = V4InvSqrt( V4AddScaled( _1, inVector, inVector ) );
		Vector_4V b0 = _1;

		Vector_4V a1 = V4Scale( V4Add( a0, b0 ), _point_5 );
		Vector_4V b1 = V4Sqrt( V4Scale(a1, b0) );

		Vector_4V a2 = V4Scale( V4Add( a1, b1 ), _point_5 );
		Vector_4V b2 = V4Sqrt( V4Scale(a2, b1) );

		Vector_4V a3 = V4Scale( V4Add( a2, b2 ), _point_5 );
//		Vector_4V b3 = V4Sqrt( V4Scale(a3, b2) );

		// Make some artificial +-(PI/2) convergence.
		return V4Clamp( V4InvScale( V4Scale( inVector, a0 ), a3 ), V4Negate(_pi_over_2), _pi_over_2 );
#else
		Vector_4V bias;
		Vector_4V x, x2, x3, x4, x8, x9;
		Vector_4V hi, lo;
		Vector_4V result;
		Vector_4V inv_x;
		Vector_4V sign;
		Vector_4V select;
		Vector_4V xabs;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		x = inVector;

		sign = V4And(x, INT_80000000);
		xabs = V4Andc(x, INT_80000000);
		inv_x = V4Invert(x);
		inv_x = V4Xor(inv_x, INT_80000000);
		select = V4IsGreaterThanV(xabs, V4VConstant(V_ONE));
		bias = V4Or(sign, V4VConstant(V_PI_OVER_TWO));
		bias = V4And(bias, select);

		x = V4SelectFT(select, x, inv_x);

		bias = V4Add(bias, x);
		x2 = V4Scale(x,  x);
		x3 = V4Scale(x2, x);
		x4 = V4Scale(x2, x2);
		x8 = V4Scale(x4, x4);
		x9 = V4Scale(x8, x);
		hi = V4AddScaled(V4VConstant<0xBC846E02,0xBC846E02,0xBC846E02,0xBC846E02>(), V4VConstant<0x3B3BD74A,0x3B3BD74A,0x3B3BD74A,0x3B3BD74A>(), x2);
		hi = V4AddScaled(V4VConstant<0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE>(), hi, x2);
		hi = V4AddScaled(V4VConstant<0xBD9A3174,0xBD9A3174,0xBD9A3174,0xBD9A3174>(), hi, x2);
		hi = V4AddScaled(V4VConstant<0x3DDA3D83,0x3DDA3D83,0x3DDA3D83,0x3DDA3D83>(), hi, x2);
		lo = V4AddScaled(V4VConstant<0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5>(), V4VConstant<0xBE117FC7,0xBE117FC7,0xBE117FC7,0xBE117FC7>(), x2);
		lo = V4AddScaled(V4VConstant<0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C>(), lo, x2);
		lo = V4AddScaled(bias, lo, x3);

		result = V4AddScaled(lo, hi, x9);    
		return result;

#endif
	}

	inline Vector_4V_Out V4Arctan2( Vector_4V_In inVectorY, Vector_4V_In inVectorX )
	{
#if 1
		Vector_4V result;
		Vector_4V combinedConstant = V4VConstant<U32_PI_OVER_TWO,U32_PI,U32_NEG_PI_OVER_TWO,U32_NEG_PI>();
		Vector_4V _zero = V4VConstant(V_ZERO);

		Vector_4V _pi_over_2		= V4SplatX( combinedConstant );
		Vector_4V _pi				= V4SplatY( combinedConstant );
		Vector_4V _neg_pi_over_2	= V4SplatZ( combinedConstant );
		Vector_4V _negpi			= V4SplatW( combinedConstant );

		// Calculate the arctan.
		Vector_4V YOverX = V4InvScale( inVectorY, inVectorX );
		Vector_4V arctan;
		
		//================================================
		// ARCTAN CODE
		//================================================
		Vector_4V bias;
		Vector_4V x, x2, x3, x4, x8, x9;
		Vector_4V hi, lo;
		Vector_4V inv_x;
		Vector_4V sign;
		Vector_4V select;
		Vector_4V xabs;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		x = YOverX;
		sign = V4And(x, INT_80000000);
		xabs = V4Andc(x, INT_80000000);
		inv_x = V4Invert(x);
		inv_x = V4Xor(inv_x, INT_80000000);
		select = V4IsGreaterThanV(xabs, V4VConstant(V_ONE));
		bias = V4Or(sign, V4VConstant(V_PI_OVER_TWO));
		bias = V4And(bias, select);
		x = V4SelectFT(select, x, inv_x);
		bias = V4Add(bias, x);
		x2 = V4Scale(x,  x);
		x3 = V4Scale(x2, x);
		x4 = V4Scale(x2, x2);
		x8 = V4Scale(x4, x4);
		x9 = V4Scale(x8, x);
		hi = V4AddScaled(V4VConstant<0xBC846E02,0xBC846E02,0xBC846E02,0xBC846E02>(), V4VConstant<0x3B3BD74A,0x3B3BD74A,0x3B3BD74A,0x3B3BD74A>(), x2);
		hi = V4AddScaled(V4VConstant<0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE>(), hi, x2);
		hi = V4AddScaled(V4VConstant<0xBD9A3174,0xBD9A3174,0xBD9A3174,0xBD9A3174>(), hi, x2);
		hi = V4AddScaled(V4VConstant<0x3DDA3D83,0x3DDA3D83,0x3DDA3D83,0x3DDA3D83>(), hi, x2);
		lo = V4AddScaled(V4VConstant<0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5>(), V4VConstant<0xBE117FC7,0xBE117FC7,0xBE117FC7,0xBE117FC7>(), x2);
		lo = V4AddScaled(V4VConstant<0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C>(), lo, x2);
		lo = V4AddScaled(bias, lo, x3);
		arctan = V4AddScaled(lo, hi, x9);
		//================================================
		// END ARCTAN CODE
		//================================================

		// Adjust the value based on the quadrant we're in.

		// Here is the logic:
		//if( x < 0 )
		//{
		//	if( y < 0 )
		//	{
		//		subtract pi from result
		//	}
		//	else // y >= 0
		//	{
		//		add pi to result
		//	}
		//}
		//if( x == 0 )
		//{
		//	if( y > 0 )
		//	{
		//		result = PI/2
		//	}
		//	else
		//	{
		//		result = -PI/2
		//	}
		//} 

		// Add or subtract pi to get the value we want.
		Vector_4V arctanPlusOrMinusPi = V4Add( arctan, IF_LT_THEN_ELSE(inVectorY,_zero,_negpi,_pi) );
		result = IF_LT_THEN_ELSE( inVectorX, _zero, arctanPlusOrMinusPi, arctan );

		// Now check for the case when inVectorX is zero (result should then be -pi/2 or pi/2 depending on inVectorY).
		result = IF_EQ_THEN_ELSE( inVectorX, _zero, IF_GT_THEN_ELSE(inVectorY,_zero,_pi_over_2,_neg_pi_over_2), result );

		mthAssertf(	0 != V4IsGreaterThanAll( result, V4Subtract(_negpi, V4VConstant(V_FLT_SMALL_6)) )	&&
					0 != V4IsLessThanAll( result, V4Add(_pi, V4VConstant(V_FLT_SMALL_6)) ),
					"Arctan2() result is out of valid range. InputY = <%f,%f,%f,%f>, InputX = <%f,%f,%f,%f>, Output = <%f,%f,%f,%f>.",
					GetX(inVectorY),GetY(inVectorY),GetZ(inVectorY),GetW(inVectorY),
					GetX(inVectorX),GetY(inVectorX),GetZ(inVectorX),GetW(inVectorX),
					GetX(result),GetY(result),GetZ(result),GetW(result));

		return result;
#elif 0
		return xb::XMVectorATan2( inVectorY, inVectorX );
#else

		Vector_4V bias;
		Vector_4V x, x2, x3, x4, x8, x9;
		Vector_4V hi, lo;
		Vector_4V res;
		Vector_4V inv_x;
		Vector_4V sign;
		Vector_4V select;
		Vector_4V xabs;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		x = V4InvScale( inVectorY, inVectorX );

		Vector_4V constVec0 = V4VConstant<U32_ONE,U32_PI_OVER_TWO,U32_PI,U32_NEG_PI>();
		Vector_4V constVec1 = V4VConstant<0xBC846E02,0x3B3BD74A,0x3D2FC1FE,0xBD9A3174>();
		Vector_4V constVec2 = V4VConstant<0x3DDA3D83,0x3E4CBBE5,0xBE117FC7,0xBEAAAA6C>();

		sign = V4And(x, INT_80000000);
		xabs = V4Andc(x, INT_80000000);
		inv_x = V4Invert(x);
		inv_x = V4Xor(inv_x, INT_80000000);
		//select = V4IsGreaterThanV(xabs, V4VConstant(V_ONE));
		//bias = V4Or(sign, V4VConstant(V_PI_OVER_TWO));
		select = V4IsGreaterThanV(xabs, V4SplatX(constVec0));
		bias = V4Or(sign, V4SplatY(constVec0));
		bias = V4And(bias, select);

		x = V4SelectFT(select, x, inv_x);

		bias = V4Add(bias, x);
		x2 = V4Scale(x,  x);
		x3 = V4Scale(x2, x);
		x4 = V4Scale(x2, x2);
		x8 = V4Scale(x4, x4);
		x9 = V4Scale(x8, x);
		//hi = V4AddScaled(V4VConstant<0xBC846E02,0xBC846E02,0xBC846E02,0xBC846E02>(), V4VConstant<0x3B3BD74A,0x3B3BD74A,0x3B3BD74A,0x3B3BD74A>(), x2);
		//hi = V4AddScaled(V4VConstant<0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE>(), hi, x2);
		//hi = V4AddScaled(V4VConstant<0xBD9A3174,0xBD9A3174,0xBD9A3174,0xBD9A3174>(), hi, x2);
		hi = V4AddScaled(V4SplatX(constVec1), V4SplatY(constVec1), x2);
		hi = V4AddScaled(V4SplatZ(constVec1), hi, x2);
		hi = V4AddScaled(V4SplatW(constVec1), hi, x2);
		//hi = V4AddScaled(V4VConstant<0x3DDA3D83,0x3DDA3D83,0x3DDA3D83,0x3DDA3D83>(), hi, x2);
		//lo = V4AddScaled(V4VConstant<0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5>(), V4VConstant<0xBE117FC7,0xBE117FC7,0xBE117FC7,0xBE117FC7>(), x2);
		//lo = V4AddScaled(V4VConstant<0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C>(), lo, x2);
		hi = V4AddScaled(V4SplatX(constVec2), hi, x2);
		lo = V4AddScaled(V4SplatY(constVec2), V4SplatZ(constVec2), x2);
		lo = V4AddScaled(V4SplatW(constVec2), lo, x2);
		lo = V4AddScaled(bias, lo, x3);

		res = V4AddScaled(lo, hi, x9);    

		Vector_4V yNeg = V4IsGreaterThanV(V4VConstant(V_ZERO),inVectorY);
		Vector_4V xNeg = V4IsGreaterThanV(V4VConstant(V_ZERO),inVectorX);

		Vector_4V bias2 = V4SelectFT(yNeg,V4SplatZ(constVec0),V4SplatW(constVec0));

		Vector_4V newRes = V4Add(bias2, res);

		res = V4SelectFT(xNeg,res,newRes);

		return res;

#endif
	}

	inline Vector_4V_Out V4CanonicalizeAngle( Vector_4V_In inVector )
	{
		// Simple, fast splats.
		Vector_4V _zero = V4VConstant(V_ZERO);

		// Generted with some instructions.
		Vector_4V _one = V4VConstant(V_ONE);
		Vector_4V _negone = V4VConstant(V_NEGONE);
		Vector_4V _half = V4VConstant(V_HALF);

		//Vector_4V _1_over_pi = V4VConstant(V_ONE_OVER_PI);
		//Vector_4V _pi = V4VConstant(V_PI);
		//Vector_4V _2pi = V4VConstant(V_TWO_PI);
		//Vector_4V _negPi = V4VConstant(V_NEG_PI);
		Vector_4V piConstants = V4VConstant<U32_ONE_OVER_PI,U32_PI,U32_TWO_PI,U32_NEG_PI>();
		Vector_4V _1_over_pi = V4SplatX(piConstants);
		Vector_4V _pi = V4SplatY(piConstants);
		Vector_4V _2pi = V4SplatZ(piConstants);
		Vector_4V _negPi = V4SplatW(piConstants);

		Vector_4V isNegative = V4IsLessThanV( inVector, _zero );
		Vector_4V negOrPosOne = V4SelectFT( isNegative, _one, _negone );

		Vector_4V inputMinusPi = V4Subtract( inVector, V4SelectFT(isNegative, _pi, _negPi ) );
		Vector_4V numPiPastBoundary = V4Scale( inputMinusPi, _1_over_pi );

		// If we are (0,2] PI past the canonical boundary, we want to subtract 2*PI.
		// If we are (2,4] PI past the canonical boundary, we want to subtract 4*PI.
		// etc.

		// Convert to 2*PI.
		
		// If we are (0,1] 2*PI past the canonical boundary, we want to subtract 2*PI.
		// If we are (1,2] 2*PI past the canonical boundary, we want to subtract 4*PI.
		// etc.

		// To find the subtract amount, we round "away from zero", then multiply by 2*PI.

		// To round "away from zero", add 1 or -1, then round toward zero.

		Vector_4V num2PiPastBoundary = V4AddScaled( negOrPosOne, numPiPastBoundary, _half );
		Vector_4V subtractFactor = V4RoundToNearestIntZero( num2PiPastBoundary );
		Vector_4V canonAngle = V4SubtractScaled( inVector, _2pi, subtractFactor );

		return canonAngle;
	}

	// NOTE: VALID FOR INPUTS [-PI,PI]
	// Forceinlined, since if not, you're actually better off using V4Sin() then V4Cos(), b/c of returning by reference.
	__forceinline void V4SinAndCosFast( Vector_4V_InOut inOutSine, Vector_4V_InOut inOutCosine, Vector_4V_In inVector )
	{
		mthAssertf( (V4IsGreaterThanOrEqualAll(inVector,V4VConstant(V_NEG_PI)) & V4IsLessThanOrEqualAll(inVector,V4VConstant(V_PI))) ,
					"Input is outside of valid range: call V4CanonicalizeAngle() first." );

#if 0 // Faster, but unfortunately is inaccurate!
		Vector_4V xl2,xl3;

		xl2 = V4Scale(inVector,inVector);
		xl3 = V4Scale(xl2,inVector);

		Vector_4V constVec0 = V4VConstant<0x3D2AA036,0xBAB24993,0x3C088342,0xB94C8C6E>();

		//Vector_4V ct1 = V4AddScaled(V4VConstant<0x3D2AA036,0x3D2AA036,0x3D2AA036,0x3D2AA036>(),V4VConstant<0xBAB24993,0xBAB24993,0xBAB24993,0xBAB24993>(),xl2);
		//Vector_4V st1 = V4AddScaled(V4VConstant<0x3C088342,0x3C088342,0x3C088342,0x3C088342>(),V4VConstant<0xB94C8C6E,0xB94C8C6E,0xB94C8C6E,0xB94C8C6E>(),xl2);
		Vector_4V ct1 = V4AddScaled(V4SplatX(constVec0),V4SplatY(constVec0),xl2);
		Vector_4V st1 = V4AddScaled(V4SplatZ(constVec0),V4SplatW(constVec0),xl2);

		Vector_4V ct2 = V4AddScaled(V4VConstant<0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF,0xBEFFFFDF>(),ct1,xl2);
		Vector_4V st2 = V4AddScaled(V4VConstant<0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1,0xBE2AAAA1>(),st1,xl2);

		inOutCosine = V4AddScaled(V4VConstant(V_ONE),ct2,xl2);
		inOutSine = V4AddScaled(inVector,st2,xl3);
#else
		Vector_4V V2, V3, V4, V5, V6, V7;
		Vector_4V S2, S3;
		Vector_4V C0, C2, C3;
		Vector_4V Sin, Cos;
		Vector_4V V = inVector;
		// {1.0f, -4.95348008918096e-1f, 3.878259962881e-2f, -9.24587976263e-4f}
		Vector_4V constVec0 = V4VConstant<0x3F800000,0xBEFD9E41,0x3D1EDA81,0xBA72600C>();
		// {1.0f, -1.66521856991541e-1f, 8.199913018755e-3f, -1.61475937228e-4f}
		Vector_4V constVec1 = V4VConstant<0x3F800000,0xBE2A84B5,0x3C0658EE,0xB92951DE>();

		V2 = V4Scale(V, V);
		C0 = V4SplatX(constVec0);
		Cos = V4SplatY(constVec0);
		Sin = V4SplatY(constVec1);
		C2 = V4SplatZ(constVec0);
		S2 = V4SplatZ(constVec1);
		C3 = V4SplatW(constVec0);
		S3 = V4SplatW(constVec1);
		V3 = V4Scale(V2, V);
		V4 = V4Scale(V2, V2);
		Cos = V4AddScaled(C0, V2, Cos);
		V5 = V4Scale(V3, V2);
		Sin = V4AddScaled(V, V3, Sin);
		V6 = V4Scale(V3, V3);
		V7 = V4Scale(V4, V3);
		Cos = V4AddScaled(Cos, C2, V4);
		Sin = V4AddScaled(Sin, S2, V5);
		Cos = V4AddScaled(Cos, C3, V6);
		Sin = V4AddScaled(Sin, S3, V7);

		inOutCosine = Cos;
		inOutSine = Sin;
#endif
	}

	// NOTE: VALID FOR INPUTS [-PI,PI]
	inline Vector_4V_Out V4SinFast( Vector_4V_In inVector )
	{
		mthAssertf( (V4IsGreaterThanOrEqualAll(inVector,V4VConstant(V_NEG_PI)) & V4IsLessThanOrEqualAll(inVector,V4VConstant(V_PI))) ,
					"Input is outside of valid range: call V4CanonicalizeAngle() first." );

#if 0 // Faster, but unfortunately is inaccurate!
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V g = V4Scale(inVector,inVector);
		Vector_4V f = V4Andc(inVector,INT_80000000);

		Vector_4V constVec0 = V4VConstant<0x3C08873E,0xB94FB222,0xBE2AAAA4,0x362E9C5B>();

		//Vector_4V t1 = V4Scale(g,g);
		//Vector_4V t2 = V4AddScaled(V4VConstant<0x3C08873E,0x3C08873E,0x3C08873E,0x3C08873E>(),V4VConstant<0xB94FB222,0xB94FB222,0xB94FB222,0xB94FB222>(),g);
		//Vector_4V t3 = V4Scale(V4VConstant<0xBE2AAAA4,0xBE2AAAA4,0xBE2AAAA4,0xBE2AAAA4>(),g);
		//Vector_4V t4 = V4Scale(t1,t1);
		//Vector_4V t5 = V4AddScaled(t3,t1,t2);
		//Vector_4V r  = V4AddScaled(t5,V4VConstant<0x362E9C5B,0x362E9C5B,0x362E9C5B,0x362E9C5B>(),t4);
		Vector_4V t1 = V4Scale(g,g);
		Vector_4V t2 = V4AddScaled(V4SplatX(constVec0),V4SplatY(constVec0),g);
		Vector_4V t3 = V4Scale(V4SplatZ(constVec0),g);
		Vector_4V t4 = V4Scale(t1,t1);
		Vector_4V t5 = V4AddScaled(t3,t1,t2);
		Vector_4V r  = V4AddScaled(t5,V4SplatW(constVec0),t4);

		Vector_4V res = V4AddScaled(f,f,r);

		res = V4SelectFT(INT_80000000, res, inVector);

		return res;
#else
		Vector_4V V2, V5, V7;
		Vector_4V S1, S2, S3;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {1.0f, -1.66521856991541e-1f, 8.199913018755e-3f, -1.61475937228e-4f}
		Vector_4V constVec0 = V4VConstant<0x3F800000,0xBE2A84B5,0x3C0658EE,0xB92951DE>();

		V2 = V4Scale(V, V);
		S1 = V4SplatY(constVec0);
		S2 = V4SplatZ(constVec0);
		S3 = V4SplatW(constVec0);
		Result = V4Scale(V2, V);
		V5 = V4Scale(Result, V2);
		Result = V4AddScaled(V, S1, Result);
		V7 = V4Scale(V5, V2);
		Result = V4AddScaled(Result, S2, V5);
		Result = V4AddScaled(Result, S3, V7);

		return Result;
#endif
	}

	// NOTE: VALID FOR INPUTS [-PI,PI]
	inline Vector_4V_Out V4CosFast( Vector_4V_In inVector )
	{
		mthAssertf( (V4IsGreaterThanOrEqualAll(inVector,V4VConstant(V_NEG_PI)) & V4IsLessThanOrEqualAll(inVector,V4VConstant(V_PI))) ,
					"Input is outside of valid range: call V4CanonicalizeAngle() first." );

#if 0 // NOTE: VALID FOR INPUTS [-PI/2,3*PI/4] // Faster, but unfortunately is inaccurate!

		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V constVec0 = V4VConstant<0x3C08873E,0xB94FB222,0xBE2AAAA4,0x362E9C5B>();
		Vector_4V x = inVector;
		x = V4Subtract(V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(),x);
		x = V4Add(V4VConstant<0x33A22169,0x33A22169,0x33A22169,0x33A22169>(),x);

		Vector_4V g = V4Scale(x,x);
		Vector_4V f = V4Andc(x,INT_80000000);

		//Vector_4V t1 = V4Scale(g,g);
		//Vector_4V t2 = V4AddScaled(V4VConstant<0x3C08873E,0x3C08873E,0x3C08873E,0x3C08873E>(),V4VConstant<0xB94FB222,0xB94FB222,0xB94FB222,0xB94FB222>(),g);
		//Vector_4V t3 = V4Scale(V4VConstant<0xBE2AAAA4,0xBE2AAAA4,0xBE2AAAA4,0xBE2AAAA4>(),g);
		//Vector_4V t4 = V4Scale(t1,t1);
		//Vector_4V t5 = V4AddScaled(t3,t1,t2);
		//Vector_4V r  = V4AddScaled(t5,V4VConstant<0x362E9C5B,0x362E9C5B,0x362E9C5B,0x362E9C5B>(),t4);
		Vector_4V t1 = V4Scale(g,g);
		Vector_4V t2 = V4AddScaled(V4SplatX(constVec0),V4SplatY(constVec0),g);
		Vector_4V t3 = V4Scale(V4SplatZ(constVec0),g);
		Vector_4V t4 = V4Scale(t1,t1);
		Vector_4V t5 = V4AddScaled(t3,t1,t2);
		Vector_4V r  = V4AddScaled(t5,V4SplatW(constVec0),t4);

		Vector_4V res = V4AddScaled(f,f,r);

		res = V4SelectFT(INT_80000000, res, x);

		return res;
#else // NOTE: VALID FOR INPUTS [-PI,PI]
		Vector_4V V2, V4, V6;
		Vector_4V C1, C2, C3;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {1.0f, -4.95348008918096e-1f, 3.878259962881e-2f, -9.24587976263e-4f}
		Vector_4V constVec0 = V4VConstant<0x3F800000,0xBEFD9E41,0x3D1EDA81,0xBA72600C>();

		V2 = V4Scale(V, V);
		Result = V4SplatX(constVec0);
		C1 = V4SplatY(constVec0);
		C2 = V4SplatZ(constVec0);
		C3 = V4SplatW(constVec0);
		V4 = V4Scale(V2, V2);
		Result = V4AddScaled(Result, C1, V2);
		V6 = V4Scale(V4, V2);
		Result = V4AddScaled(Result, C2, V4);
		Result = V4AddScaled(Result, C3, V6);

		return Result;
#endif
	}

	inline Vector_4V_Out V4TanFast( Vector_4V_In inVector )
	{
#if 0
		Vector_4V x,x2,x3;
		Vector_4V constVec0 = V4VConstant<0x3C1F166D,0xBEDBB51E,U32_ONE,0xBDC42983>();
		x = inVector;

		x2 = V4Scale(x,x);
		x3 = V4Scale(x2,x);

		//Vector_4V ct2 = V4AddScaled(V4VConstant<0x3C1F166D,0x3C1F166D,0x3C1F166D,0x3C1F166D>(),x2,V4VConstant<0xBEDBB51E,0xBEDBB51E,0xBEDBB51E,0xBEDBB51E>());
		//Vector_4V cx = V4AddScaled(V4VConstant(V_ONE),ct2,x2);
		//Vector_4V sx = V4AddScaled(x,V4VConstant<0xBDC42983,0xBDC42983,0xBDC42983,0xBDC42983>(),x3);
		Vector_4V ct2 = V4AddScaled(V4SplatX(constVec0),x2,V4SplatY(constVec0));
		Vector_4V cx = V4AddScaled(V4SplatZ(constVec0),ct2,x2);
		Vector_4V sx = V4AddScaled(x,V4SplatW(constVec0),x3);

		Vector_4V res = V4InvScaleFast(sx,cx);
		return res;
#else
		Vector_4V V1, V2, V1D, V1T0, V1T1, V2T2;
		Vector_4V T0, T1, T2;
		Vector_4V N, D;
		Vector_4V OneOverPi, Pi;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {2.484f, -1.954923183e-1f, 2.467401101f, 1/PI}
		Vector_4V constVec0 = V4VConstant<0x401EF9DB,0xBE482F23,0x401DE9E6,U32_ONE_OVER_PI>();

		OneOverPi = V4SplatW(constVec0);

		V1 = V4Scale(V, OneOverPi);
		Pi = V4VConstant(V_PI);
		V1 = V4RoundToNearestInt(V1);
		V1D = V;

		V1D = V4SubtractScaled(V1D, V1, Pi);

		T0 = V4SplatX(constVec0);
		T1 = V4SplatY(constVec0);
		T2 = V4SplatZ(constVec0);

		V2T2 = V4SubtractScaled(T2, V1D, V1D);
		V2 = V4Scale(V1D, V1D);
		V1T0 = V4Scale(V1D, T0);
		V1T1 = V4Scale(V1D, T1);

		D = V4InvertFast(V2T2);
		N = V4AddScaled(V1T0, V2, V1T1);

		Result = V4Scale(N, D);

		return Result;
#endif
	}

	inline Vector_4V_Out V4ArcsinFast( Vector_4V_In inVector )
	{
#if 0
		Vector_4V mask = V4VConstant(V_80000000);
		Vector_4V sign;
		Vector_4V result, xabs;
		Vector_4V t1;
		Vector_4V xabs2, xabs4;
		Vector_4V hi, lo;
		Vector_4V x = inVector;

		Vector_4V constVec0 = V4VConstant<0x3BDA90C5,0xBAA57A2C,0xBC8BFC66,0x3CFD10F8>();
		Vector_4V constVec1 = V4VConstant<0x3DB63A9E,0xBD4D8392,0xBE5BBFCA,0x3FC90FDB>();
		Vector_4V tempVec = V4SplatW(constVec1);

		sign = V4And(x, mask);
		xabs = V4Abs(x);

		t1 = V4Sqrt(V4Subtract(V4VConstant(V_ONE), xabs));

		xabs2 = V4Scale(xabs,  xabs);
		xabs4 = V4Scale(xabs2, xabs2);
		//hi = V4AddScaled(V4VConstant<0x3BDA90C5,0x3BDA90C5,0x3BDA90C5,0x3BDA90C5>(),V4VConstant<0xBAA57A2C,0xBAA57A2C,0xBAA57A2C,0xBAA57A2C>(), xabs);
		//hi = V4AddScaled(V4VConstant<0xBC8BFC66,0xBC8BFC66,0xBC8BFC66,0xBC8BFC66>(), hi, xabs);
		//hi = V4AddScaled(V4VConstant<0x3CFD10F8,0x3CFD10F8,0x3CFD10F8,0x3CFD10F8>(), hi, xabs);
		//lo = V4AddScaled(V4VConstant<0x3DB63A9E,0x3DB63A9E,0x3DB63A9E,0x3DB63A9E>(), V4VConstant<0xBD4D8392,0xBD4D8392,0xBD4D8392,0xBD4D8392>(), xabs);
		//lo = V4AddScaled(V4VConstant<0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA>(), lo, xabs);
		//lo = V4AddScaled(V4VConstant<0x3FC90FDB,0x3FC90FDB,0x3FC90FDB,0x3FC90FDB>(), lo, xabs);
		//result = V4AddScaled(lo, hi, xabs4);
		//result = V4SubtractScaled(result, t1, V4VConstant<0x3FC90FDB,0x3FC90FDB,0x3FC90FDB,0x3FC90FDB>());
		hi = V4AddScaled(V4SplatX(constVec0),V4SplatY(constVec0), xabs);
		hi = V4AddScaled(V4SplatZ(constVec0), hi, xabs);
		hi = V4AddScaled(V4SplatW(constVec0), hi, xabs);
		lo = V4AddScaled(V4SplatX(constVec1), V4SplatY(constVec1), xabs);
		lo = V4AddScaled(V4SplatZ(constVec1), lo, xabs);
		lo = V4AddScaled(tempVec, lo, xabs);
		result = V4AddScaled(lo, hi, xabs4);
		result = V4SubtractScaled(result, t1, tempVec);

		result = V4Or(result, sign);

		return result;
#else
		Vector_4V AbsV, V2, VD, VC0, V2C3;
		Vector_4V C0, C1, C2, C3;
		Vector_4V D, Rsq, SqrtD;
		Vector_4V OnePlusEps;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {1.00000011921f, PI_OVER_2, 0.0f, 0.0f}
		Vector_4V constVec0 = V4VConstant<0x3F800001,U32_PI_OVER_TWO,0,0>();
		// {-1.36178272886711f, 2.37949493464538f, -8.08228565650486e-1f, 2.78440142746736e-1f}
		Vector_4V constVec1 = V4VConstant<0xBFAE4EE5,0x401849A5,0xBF4EE811,0x3E8E8FB5>();

		AbsV = V4Abs(V);

		OnePlusEps = V4SplatX(constVec0);
		OnePlusEps = V4VConstant(V_ONE_PLUS_EPSILON);
		C1 = V4SplatY(constVec1);
		C3 = V4SplatW(constVec1);

		D = V4Subtract(OnePlusEps, AbsV);

		V2 = V4Scale(V, AbsV);
		Result = V4Scale(V, C1);
		C2 = V4SplatZ(constVec1);

		C0 = V4SplatX(constVec1);

		VD = V4Scale(D, AbsV);
		V2C3 = V4Scale(V2, C3);

		Rsq = V4InvSqrtFast(D);
		Result = V4AddScaled(Result, V2, C2);
		VC0 = V4Scale(V, C0);
		SqrtD = V4Scale(D, Rsq);
		Result = V4AddScaled(Result, V2C3, VD);
		Result = V4AddScaled(Result, VC0, SqrtD);

		return Result;
#endif
	}

	inline Vector_4V_Out V4ArccosFast( Vector_4V_In inVector )
	{
#if 0
		Vector_4V result, xabs;
		Vector_4V t1;
		Vector_4V xabs2, xabs4;
		Vector_4V hi, lo;
		Vector_4V neg, pos;
		Vector_4V select;
		Vector_4V x = inVector;
		Vector_4V constVec0 = V4VConstant<0x3BDA90C5,0xBAA57A2C,0xBC8BFC66,0x3CFD10F8>();
		Vector_4V constVec1 = V4VConstant<0x3DB63A9E,0xBD4D8392,0xBE5BBFCA,0x3FC90FDA>();

		xabs = V4Abs(x);
#if __XENON
		select = __vsrab(x, V4VConstant<0x31,0x31,0x31,0x31>());
#elif __PS3
		select = (Vector_4V)vec_sra((Vector_4V_uchar)x, (Vector_4V_uchar)V4VConstant<0x31,0x31,0x31,0x31>());
#endif

		t1 = V4SqrtFast(V4Subtract(V4VConstant(V_ONE), xabs));

		xabs2 = V4Scale(xabs,  xabs);
		xabs4 = V4Scale(xabs2, xabs2);
		//hi = V4AddScaled(V4VConstant<0x3BDA90C5,0x3BDA90C5,0x3BDA90C5,0x3BDA90C5>(), V4VConstant<0xBAA57A2C,0xBAA57A2C,0xBAA57A2C,0xBAA57A2C>(), xabs);
		//hi = V4AddScaled(V4VConstant<0xBC8BFC66,0xBC8BFC66,0xBC8BFC66,0xBC8BFC66>(), hi, xabs);
		//hi = V4AddScaled(V4VConstant<0x3CFD10F8,0x3CFD10F8,0x3CFD10F8,0x3CFD10F8>(), hi, xabs);
		//lo = V4AddScaled(V4VConstant<0x3DB63A9E,0x3DB63A9E,0x3DB63A9E,0x3DB63A9E>(), V4VConstant<0xBD4D8392,0xBD4D8392,0xBD4D8392,0xBD4D8392>(), xabs);
		//lo = V4AddScaled(V4VConstant<0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA,0xBE5BBFCA>(), lo, xabs);
		//lo = V4AddScaled(V4VConstant<0x3FC90FDA,0x3FC90FDA,0x3FC90FDA,0x3FC90FDA>(), lo, xabs);
		hi = V4AddScaled(V4SplatX(constVec0), V4SplatY(constVec0), xabs);
		hi = V4AddScaled(V4SplatZ(constVec0), hi, xabs);
		hi = V4AddScaled(V4SplatW(constVec0), hi, xabs);
		lo = V4AddScaled(V4SplatX(constVec1), V4SplatY(constVec1), xabs);
		lo = V4AddScaled(V4SplatZ(constVec1), lo, xabs);
		lo = V4AddScaled(V4SplatW(constVec1), lo, xabs);

		result = V4AddScaled(lo, hi, xabs4);

		neg = V4SubtractScaled(V4VConstant(V_PI), result, t1);
		pos = V4Scale(t1, result);

		result = V4SelectFT(select, pos, neg);

		return result;
#else
		Vector_4V AbsV, V2, VD, VC0, V2C3;
		Vector_4V C0, C1, C2, C3;
		Vector_4V D, Rsq, SqrtD;
		Vector_4V OnePlusEps, HalfPi;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {-1.36178272886711f, 2.37949493464538f, -8.08228565650486e-1f, 2.78440142746736e-1f}
		Vector_4V constVec0 = V4VConstant<0xBFAE4EE5,0x401849A5,0xBF4EE811,0x3E8E8FB5>();

		AbsV = V4Abs( V );

		OnePlusEps = V4VConstant(V_ONE_PLUS_EPSILON);
		C1 = V4SplatY(constVec0);
		C3 = V4SplatW(constVec0);

		D = V4Subtract(OnePlusEps, AbsV);

		V2 = V4Scale(V, AbsV);
		Result = V4Scale(V, C1);
		C2 = V4SplatZ(constVec0);

		C0 = V4SplatX(constVec0);

		VD = V4Scale(D, AbsV);
		V2C3 = V4Scale(V2, C3);

		Rsq = V4InvSqrtFast(D);
		Result = V4AddScaled(Result, V2, C2);
		VC0 = V4Scale(V, C0);
		SqrtD = V4Scale(D, Rsq);
		Result = V4AddScaled(Result, V2C3, VD);
		HalfPi = V4VConstant(V_PI_OVER_TWO);
		Result = V4AddScaled(Result, VC0, SqrtD);
		Result = V4Subtract(HalfPi, Result);

		return Result;
#endif
	}

	inline Vector_4V_Out V4ArctanFast( Vector_4V_In inVector )
	{
#if 0
		Vector_4V bias;
		Vector_4V x2, x3, x4, x8, x9;
		Vector_4V hi, lo;
		Vector_4V result;
		Vector_4V inv_x;
		Vector_4V sign;
		Vector_4V select;
		Vector_4V xabs;
		Vector_4V x = inVector;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);

		Vector_4V constVec0 = V4VConstant<0xBC846E02,0x3B3BD74A,0x3D2FC1FE,0xBD9A3174>();
		Vector_4V constVec1 = V4VConstant<0x3DDA3D83,0x3E4CBBE5,0xBE117FC7,0xBEAAAA6C>();

		sign = V4And(x, INT_80000000);
		xabs = V4Andc(x, INT_80000000);
		inv_x = V4InvertFast(x);
		inv_x = V4Xor(inv_x, INT_80000000);
		select = V4IsGreaterThanV(xabs, V4VConstant(V_ONE));
		bias = V4Or(sign, V4VConstant(V_PI_OVER_TWO));
		bias = V4And(bias, select);

		x = V4SelectFT(select, x, inv_x);

		bias = V4Add(bias, x);
		x2 = V4Scale(x,  x);
		x3 = V4Scale(x2, x);
		x4 = V4Scale(x2, x2);
		x8 = V4Scale(x4, x4);
		x9 = V4Scale(x8, x);
		//hi = V4AddScaled(V4VConstant<0xBC846E02,0xBC846E02,0xBC846E02,0xBC846E02>(), V4VConstant<0x3B3BD74A,0x3B3BD74A,0x3B3BD74A,0x3B3BD74A>(), x2);
		//hi = V4AddScaled(V4VConstant<0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE>(), hi, x2);
		//hi = V4AddScaled(V4VConstant<0xBD9A3174,0xBD9A3174,0xBD9A3174,0xBD9A3174>(), hi, x2);
		//hi = V4AddScaled(V4VConstant<0x3DDA3D83,0x3DDA3D83,0x3DDA3D83,0x3DDA3D83>(), hi, x2);
		//lo = V4AddScaled(V4VConstant<0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5>(), V4VConstant<0xBE117FC7,0xBE117FC7,0xBE117FC7,0xBE117FC7>(), x2);
		//lo = V4AddScaled(V4VConstant<0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C>(), lo, x2);
		hi = V4AddScaled(V4SplatX(constVec0), V4SplatY(constVec0), x2);
		hi = V4AddScaled(V4SplatZ(constVec0), hi, x2);
		hi = V4AddScaled(V4SplatW(constVec0), hi, x2);
		hi = V4AddScaled(V4SplatX(constVec1), hi, x2);
		lo = V4AddScaled(V4SplatY(constVec1), V4SplatZ(constVec1), x2);
		lo = V4AddScaled(V4SplatW(constVec1), lo, x2);
		lo = V4AddScaled(bias, lo, x3);

		result = V4AddScaled(lo, hi, x9);

		return result;
#else
		Vector_4V AbsV, V2S2, N, D;
		Vector_4V HalfPi;
		Vector_4V Result;
		Vector_4V V = inVector;
		// {7.689891418951e-1f, 1.104742493348f, 8.661844266006e-1f, PI_OVER_2}
		Vector_4V constVec0 = V4VConstant<0x3F44DC79,0x3F8D6834,0x3F5DBE43,U32_PI_OVER_TWO>();

		AbsV = V4Abs( V );

		V2S2 = V4SplatZ(constVec0);
		HalfPi = V4SplatW(constVec0);
		N = V4SplatX(constVec0);
		D = V4SplatY(constVec0);

		V2S2 = V4AddScaled(V2S2, V, V);
		N = V4AddScaled(N, AbsV, HalfPi);
		D = V4AddScaled(V2S2, AbsV, D);
		N = V4Scale(N, V);
		D = V4InvertFast(D);

		Result = V4Scale(N, D);

		return Result;
#endif
	}

	inline Vector_4V_Out V4Arctan2Fast( Vector_4V_In inVectorY, Vector_4V_In inVectorX )
	{
#if 0
		Vector_4V bias;
		Vector_4V x2, x3, x4, x8, x9;
		Vector_4V hi, lo;
		Vector_4V res;
		Vector_4V inv_x;
		Vector_4V sign;
		Vector_4V select;
		Vector_4V xabs;
		Vector_4V y = inVectorY;
		Vector_4V x = inVectorX;
		Vector_4V INT_80000000 = V4VConstant(V_80000000);

		Vector_4V constVec0 = V4VConstant<0xBC846E02,0x3B3BD74A,0x3D2FC1FE,0xBD9A3174>();
		Vector_4V constVec1 = V4VConstant<0x3DDA3D83,0x3E4CBBE5,0xBE117FC7,0xBEAAAA6C>();
		Vector_4V constVec2 = V4VConstant<U32_ONE,U32_PI_OVER_TWO,U32_PI,U32_NEG_PI>();

		sign = V4And(x, INT_80000000);
		xabs = V4Andc(x, INT_80000000);
		inv_x = V4InvertFast(x);
		inv_x = V4Xor(inv_x, INT_80000000);
		//select = V4IsGreaterThanV(xabs, V4VConstant(V_ONE));
		select = V4IsGreaterThanV(xabs, V4SplatX(constVec2));
		//bias = V4Or(sign, V4VConstant(V_PI_OVER_TWO));
		bias = V4Or(sign, V4SplatY(constVec2));
		bias = V4And(bias, select);

		x = V4SelectFT(select, x, inv_x);

		bias = V4Add(bias, x);
		x2 = V4Scale(x,  x);
		x3 = V4Scale(x2, x);
		x4 = V4Scale(x2, x2);
		x8 = V4Scale(x4, x4);
		x9 = V4Scale(x8, x);
		//hi = V4AddScaled(V4VConstant<0xBC846E02,0xBC846E02,0xBC846E02,0xBC846E02>(), V4VConstant<0x3B3BD74A,0x3B3BD74A,0x3B3BD74A,0x3B3BD74A>(), x2);
		//hi = V4AddScaled(V4VConstant<0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE,0x3D2FC1FE>(), hi, x2);
		//hi = V4AddScaled(V4VConstant<0xBD9A3174,0xBD9A3174,0xBD9A3174,0xBD9A3174>(), hi, x2);
		//hi = V4AddScaled(V4VConstant<0x3DDA3D83,0x3DDA3D83,0x3DDA3D83,0x3DDA3D83>(), hi, x2);
		//lo = V4AddScaled(V4VConstant<0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5,0x3E4CBBE5>(), V4VConstant<0xBE117FC7,0xBE117FC7,0xBE117FC7,0xBE117FC7>(), x2);
		//lo = V4AddScaled(V4VConstant<0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C,0xBEAAAA6C>(), lo, x2);
		hi = V4AddScaled(V4SplatX(constVec0), V4SplatY(constVec0), x2);
		hi = V4AddScaled(V4SplatZ(constVec0), hi, x2);
		hi = V4AddScaled(V4SplatW(constVec0), hi, x2);
		hi = V4AddScaled(V4SplatX(constVec1), hi, x2);
		lo = V4AddScaled(V4SplatY(constVec1), V4SplatZ(constVec1), x2);
		lo = V4AddScaled(V4SplatW(constVec1), lo, x2);
		lo = V4AddScaled(bias, lo, x3);
		res = V4AddScaled(lo, hi, x9);

		Vector_4V yNeg = V4IsGreaterThanV(V4VConstant(V_ZERO),y);
		Vector_4V xNeg = V4IsGreaterThanV(V4VConstant(V_ZERO),x);

		//Vector_4V bias2 = V4SelectFT(yNeg, V4VConstant(V_PI),V4VConstant(V_NEG_PI));
		Vector_4V bias2 = V4SelectFT(yNeg, V4SplatZ(constVec2), V4SplatW(constVec2));

		Vector_4V newRes = V4Add(bias2, res);

		res = V4SelectFT(xNeg,res,newRes);

		return res;
#else
		Vector_4V Reciprocal;
		Vector_4V V;
		Vector_4V YSign;
		Vector_4V Pi, PiOverTwo, PiOverFour, ThreePiOverFour, HalfPi;
		Vector_4V YEqualsZero, XEqualsZero, XIsPositive, YEqualsInfinity, XEqualsInfinity, FiniteYGreaterZero;
		Vector_4V ATanResultValid;
		Vector_4V R0, R1, R2, R3, R4, R5, R6, R7;
		Vector_4V Zero;
		Vector_4V Result;
		Vector_4V x = inVectorX;
		Vector_4V y = inVectorY;
		Vector_4V ATan2Constants = V4VConstant<U32_PI,U32_PI_OVER_TWO,U32_PI_OVER_FOUR,U32_THREE_PI_OVER_FOUR>();
		Vector_4V INT_80000000 = V4VConstant(V_80000000);
		Vector_4V _inf = V4VConstant(V_INF);

		Reciprocal = V4InvertFast(x);

		YSign = V4And(y, INT_80000000);
		Zero = V4VConstant(V_ZERO);
		XIsPositive = V4And(x, INT_80000000);
		FiniteYGreaterZero = V4IsGreaterThanV(y, Zero);
		YEqualsInfinity = V4Andc(y, INT_80000000);
		XEqualsInfinity = V4Andc(x, INT_80000000);

		Pi = V4SplatX(ATan2Constants);
		HalfPi = PiOverTwo = V4SplatY(ATan2Constants); // PiOverTwo is later modified
		ATanResultValid = V4VConstant(V_MASKXYZW);
		PiOverFour = V4SplatZ(ATan2Constants);
		ThreePiOverFour = V4SplatW(ATan2Constants);

		XIsPositive = V4IsEqualIntV(XIsPositive, Zero);
		XEqualsZero = V4IsEqualV(x, Zero);
		YEqualsZero = V4IsEqualV(y, Zero);
		YEqualsInfinity = V4IsEqualIntV(YEqualsInfinity, _inf);
		XEqualsInfinity = V4IsEqualIntV(XEqualsInfinity, _inf);

		V = V4Scale(y, Reciprocal);

		Pi = V4Or(Pi, YSign);
		PiOverTwo = V4Or(PiOverTwo, YSign);
		PiOverFour = V4Or(PiOverFour, YSign);
		ThreePiOverFour = V4Or(ThreePiOverFour, YSign);

		FiniteYGreaterZero = V4SelectFT(YEqualsInfinity, FiniteYGreaterZero, Zero);

		R1 = V4SelectFT(XIsPositive, Pi, YSign);
		R2 = V4SelectFT(XEqualsZero, ATanResultValid, PiOverTwo);
		R4 = V4SelectFT(XIsPositive, ThreePiOverFour, PiOverFour);
		R3 = V4SelectFT(YEqualsZero, R2, R1);
		R5 = V4SelectFT(XEqualsInfinity, PiOverTwo, R4);
		R6 = V4SelectFT(YEqualsInfinity, R3, R5);
		R7 = V4SelectFT(FiniteYGreaterZero, R6, R1);
		Result = V4SelectFT(XEqualsInfinity, R6, R7);
		ATanResultValid = V4IsEqualIntV(Result, ATanResultValid);


		//================================================
		// Normal arctan
		//================================================
		Vector_4V AbsV, V2S2, N, D;
		// {7.689891418951e-1f, 1.104742493348f, 8.661844266006e-1f, PI_OVER_2}
		Vector_4V constVec0 = V4VConstant<0x3F44DC79,0x3F8D6834,0x3F5DBE43,U32_PI_OVER_TWO>();
		AbsV = V4Abs( V );
		V2S2 = V4SplatZ(constVec0);
		N = V4SplatX(constVec0);
		D = V4SplatY(constVec0);
		V2S2 = V4AddScaled(V2S2, V, V);
		N = V4AddScaled(N, AbsV, HalfPi);
		D = V4AddScaled(V2S2, AbsV, D);
		N = V4Scale(N, V);
		D = V4InvertFast(D);
		R0 = V4Scale(N, D);

		Result = V4SelectFT(ATanResultValid, Result, R0);

		return Result;
#endif
	}
	
} // namespace Vec
} // namespace rage
