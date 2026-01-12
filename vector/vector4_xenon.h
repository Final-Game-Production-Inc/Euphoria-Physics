// 
// vector/vector4_xenon.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTOR4_XENON_H
#define VECTOR_VECTOR4_XENON_H

namespace rage
{

#if __XENON || __PPU
#define VEC4_INLINE __forceinline
#else
#define VEC4_INLINE inline
#endif

#if __XENON
	#define Vec4VectorSwizzle(V, E0, E1, E2, E3) __vpermwi	((V), ((E0) << 6) | ((E1) << 4) | ((E2) << 2) | (E3) | \
														(((E0) & ~3) << 6) | (((E1) & ~3) << 6) | \
														(((E2) & ~3) << 6) | (((E3) & ~3) << 6))
#elif __SPU
	static VEC4_INLINE __vector4 _v4perm(__vector4 a,_uvector4 control) {
		return (__vector4) vec_perm((_ivector4)a,(_ivector4)a,(vector unsigned char)control);
	}
#define Vec4VectorSwizzle(V, E0, E1, E2, E3)	_v4perm(V,(_uvector4){E0,E1,E2,E3})
#else
	static VEC4_INLINE __vector4 _v4perm(__vector4 a,_uvector4 control) {
		return (__vector4) vec_perm(a,a,(_ucvector4)control);
	}
#define Vec4VectorSwizzle(V, E0, E1, E2, E3)	_v4perm(V,(_uvector4){E0,E1,E2,E3})
#endif


	VEC4_INLINE	__vector4 NewtonRaphsonRecip4(__vector4 val)
	{
		__vector4 recip = __vrefp(val);
		return __vmaddfp(recip, __vnmsubfp(recip, val, VECTOR4_IDENTITY), recip);
	}

#ifndef VECTOR4_ZEROTYPE
#define VECTOR4_ZEROTYPE
	VEC4_INLINE Vector4::Vector4( _ZeroType )
	{
		xyzw = _vzerofp;
	}
#endif // VECTOR4_ZEROTYPE

#ifndef VECTOR4_CONST_V
#define VECTOR4_CONST_V
	VEC4_INLINE Vector4::Vector4(const Vector4 &vec)
	{
		xyzw = vec.xyzw;
	}
#endif // VECTOR4_CONST_V

#ifndef VECTOR4_SPLATX
#define VECTOR4_SPLATX
	VEC4_INLINE void Vector4::SplatX()
	{
		xyzw = __vspltw(xyzw, 0);
	}
#endif // VECTOR4_SPLATX

#ifndef VECTOR4_SPLATY
#define VECTOR4_SPLATY
	VEC4_INLINE void Vector4::SplatY()
	{
		xyzw = __vspltw(xyzw, 1);
	}
#endif // VECTOR4_SPLATY

#ifndef VECTOR4_SPLATZ
#define VECTOR4_SPLATZ
	VEC4_INLINE void Vector4::SplatZ()
	{
		xyzw = __vspltw(xyzw, 2);
	}
#endif // VECTOR4_SPLATZ

#ifndef VECTOR4_SPLATW
#define VECTOR4_SPLATW
	VEC4_INLINE void Vector4::SplatW()
	{
		xyzw = __vspltw(xyzw, 3);
	}
#endif // VECTOR4_SPLATW

#ifndef VECTOR4_SPLATX_V
#define VECTOR4_SPLATX_V
	VEC4_INLINE void Vector4::SplatX(Vector4Param in)
	{
		xyzw = __vspltw(in, 0);
	}
#endif // VECTOR4_SPLATX_V

#ifndef VECTOR4_SPLATY_V
#define VECTOR4_SPLATY_V
	VEC4_INLINE void Vector4::SplatY(Vector4Param in)
	{
		xyzw = __vspltw(in, 1);
	}
#endif // VECTOR4_SPLATY_V

#ifndef VECTOR4_SPLATZ_V
#define VECTOR4_SPLATZ_V
	VEC4_INLINE void Vector4::SplatZ(Vector4Param in)
	{
		xyzw = __vspltw(in, 2);
	}
#endif // VECTOR4_SPLATZ_V

#ifndef VECTOR4_SPLATW_V
#define VECTOR4_SPLATW_V
	VEC4_INLINE void Vector4::SplatW(Vector4Param in)
	{
		xyzw = __vspltw(in, 3);
	}
#endif // VECTOR4_SPLATW_V

#ifndef VECTOR4_SET_V
#define VECTOR4_SET_V
	VEC4_INLINE void Vector4::Set(const Vector4& a)
	{
		xyzw = a.xyzw;
	}
#endif // VECTOR4_SET_V

#ifndef VECTOR4_SETSCALED_V
#define VECTOR4_SETSCALED_V
	VEC4_INLINE void Vector4::SetScaled(Vector4Param a, Vector4Param s)
	{
		xyzw = __vmulfp(a, s);
	}
#endif // VECTOR4_SETSCALED_V

#ifndef VECTOR4_ZERO
#define VECTOR4_ZERO
	VEC4_INLINE void Vector4::Zero()
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//xyzw = __vxor(xyzw, xyzw);
		xyzw = _vzerofp;
	}
#endif // VECTOR4_ZERO

#ifndef VECTOR4_GETVECTOR3_V
#define VECTOR4_GETVECTOR3_V
	VEC4_INLINE void Vector4::GetVector3(Vector3& vec) const
	{
		vec.xyzw = xyzw;
	}
#endif // VECTOR4_GETVECTOR3_V

#ifndef VECTOR4_GETVECTOR3
#define VECTOR4_GETVECTOR3
	VEC4_INLINE Vector3 Vector4::GetVector3() const
	{
		Vector3 ret;
		ret.xyzw = xyzw;
		return ret;
	}
#endif // VECTOR4_GETVECTOR3

#ifndef VECTOR4_SETVECTOR3
#define VECTOR4_SETVECTOR3
	VEC4_INLINE void Vector4::SetVector3(Vector3Param vec)
	{
#if HACK_GTA4
	#if __SPU
		xyzw = __vand(vec, VEC3_ANDW);
	#else
		xyzw = __vand(vec, VEC4_ANDW);
	#endif
#else
		xyzw = vec;
#endif // HACK_GTA4...
	}
#endif // VECTOR4_SETVECTOR3

#ifndef VECTOR4_SETVECTOR3CLEARW
#define VECTOR4_SETVECTOR3CLEARW
	VEC4_INLINE void Vector4::SetVector3ClearW(Vector3Param vec)
	{
		xyzw = __vand(vec, VEC3_ANDW);
	}
#endif // VECTOR4_SETVECTOR3CLEARW

#ifndef VECTOR4_CLEARW
#define VECTOR4_CLEARW
	VEC4_INLINE void Vector4::ClearW()
	{
		xyzw = __vand(xyzw, VEC3_ANDW);
	}
#endif

#ifndef VECTOR4_ADDVECTOR3
#define VECTOR4_ADDVECTOR3
	VEC4_INLINE void Vector4::AddVector3(Vector3Param vec)
	{
#if HACK_GTA4
	#if __SPU
		xyzw = __vaddfp(xyzw, __vand(vec, VEC3_ANDW));
	#else
		xyzw = __vaddfp(xyzw, __vand(vec, VEC4_ANDW));
	#endif
#else
		xyzw = __vaddfp(xyzw, vec);
#endif // HACK_GTA4...
	}
#endif // VECTOR4_ADDVECTOR3

#ifndef VECTOR4_ADDVECTOR3XYZ
#define VECTOR4_ADDVECTOR3XYZ
	VEC4_INLINE void Vector4::AddVector3XYZ(Vector3Param vec)
	{
#if HACK_GTA4
	#if __SPU
		xyzw = __vaddfp(xyzw, __vand(vec, VEC3_ANDW));
	#else
		xyzw = __vaddfp(xyzw, __vand(vec, VEC4_ANDW));
	#endif
#else
		xyzw = __vaddfp(xyzw, __vand(vec, VEC4_ANDW));
#endif //HACK_GTA4...
	}
#endif // VECTOR4_ADDVECTOR3XYZ

#if 0	// This code doesn't work for w properly since vpkd3d treats it as unsigned
		// when we need it signed.  Use the default versions for now, although a better
		// solution might be to special-case the W channel here.
#ifndef VECTOR4_PACK1010102
#define VECTOR4_PACK1010102
    VEC4_INLINE u32 Vector4::Pack1010102() const
	{
		static const __vector4 PackScale = {-(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
                                       -(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
                                       -(float)((1 << ((10) - 1)) - 1) / (float)(1 << 22),
                                       -(float)((1 << (2)) - 1) / (float)(1 << 22)};

		static const __vector4 three = {3.0f, 3.0f, 3.0f, 3.0f};
		register __vector4 pack = __vnmsubfp(xyzw, PackScale, three);
		pack = __vpkd3d(pack, pack, VPACK_NORMPACKED32, VPACK_32, 0);
		return *(u32*)&pack.w;
	}
#endif // VECTOR4_PACK1010102

#ifndef VECTOR4_UNPACK1010102
#define VECTOR4_UNPACK1010102
	VEC4_INLINE void Vector4::Unpack1010102(u32 packed)
	{
		static const __vector4	UnpackScaleOffset = {-(float)(1 << 22) / (float)((1 << (10 - 1)) - 1), // Signed scale
                                                 -(float)(1 << 23) / (float)((1 << 2) - 1), // Unsigned scale
                                                 -(float)(1 << 22) / (float)((1 << (10 - 1)) - 1) * 3.0f, // Signed offset
                                                 -(float)(1 << 23) / (float)((1 << 2) - 1)}; // Unsigned offset

		__vector4 V = __lvlx(&packed, 0);
		__vector4 r = __vpermwi(UnpackScaleOffset, 0xAB);
		__vector4 UnpackScale = __vpermwi(UnpackScaleOffset, 0x1);
		V = __vsldoi(V, V, 1 << 2);
		V = __vupkd3d(V, VPACK_NORMPACKED32);
		xyzw = __vnmsubfp(V, UnpackScale, r);
	}
#endif // VECTOR4_UNPACK1010102
#endif // 0

#if !defined(VECTOR4_PACKCOLOR) && __XENON
#define VECTOR4_PACKCOLOR
	VEC4_INLINE void Vector4::PackColor()
	{
		__vector4 temp = Vec4VectorSwizzle(xyzw, VEC_PERM_W, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_Z);
		temp = (__vector4) __vctsxs(temp, 0);
		temp = (__vector4) __vpkuwum(temp, temp);
		xyzw = (__vector4) __vpkuhum(temp, temp);
	}
#endif // VECTOR4_PACKCOLOR

#if !defined(VECTOR4_UNPACKCOLOR) && __XENON
#define VECTOR4_UNPACKCOLOR
	VEC4_INLINE void Vector4::UnpackColor()
	{
		__vector4 temp = __vupklsb(xyzw);
		temp = __vupklsh(temp);
		temp = __vcfsx(temp, 0);

		xyzw = Vec4VectorSwizzle(temp, VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_W, VEC_PERM_X); 
	}
#endif // VECTOR4_UNPACKCOLOR

#ifndef VECTOR4_INVSCALE
#define VECTOR4_INVSCALE
	VEC4_INLINE void Vector4::InvScale(float f)
	{
		__vector4 temp = NewtonRaphsonRecip4(_vsplatf(f));		// temp = 1111 / ffff
		xyzw = __vmulfp(xyzw, temp);						// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR4_INVSCALE

#ifndef VECTOR4_INVSCALE_V
#define VECTOR4_INVSCALE_V
	VEC4_INLINE void Vector4::InvScale(Vector4Param f)
	{
		__vector4 temp = NewtonRaphsonRecip4(f);		// temp = 1111 / ffff
		xyzw = __vmulfp(xyzw, temp);						// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR4_INVSCALE_V

#ifndef VECTOR4_INVSCALE_VF
#define VECTOR4_INVSCALE_VF
	VEC4_INLINE void Vector4::InvScale(Vector4Param a, float f)
	{
		__vector4 temp = NewtonRaphsonRecip4(_vsplatf(f));		// temp = 1111 / ffff
		xyzw = __vmulfp(a, temp);						// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR4_INVSCALE_VF

#ifndef VECTOR4_INVSCALE_VV
#define VECTOR4_INVSCALE_VV
	VEC4_INLINE void Vector4::InvScale(Vector4Param a, Vector4Param f)
	{
		__vector4 temp = NewtonRaphsonRecip4(f);		// temp = 1111 / ffff
		xyzw = __vmulfp(a, temp);						// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR4_INVSCALE_VV

#ifndef VECTOR4_ADD_V
#define VECTOR4_ADD_V
	VEC4_INLINE void Vector4::Add(Vector4Param a)
	{
		xyzw = __vaddfp(xyzw, a);
	}
#endif // VECTOR4_ADD_V

#ifndef VECTOR4_ADD_VV
#define VECTOR4_ADD_VV
	VEC4_INLINE void Vector4::Add(Vector4Param a,Vector4Param b)
	{
		xyzw = __vaddfp(a, b);
	}
#endif // VECTOR4_ADD_VV

#ifndef VECTOR4_ADDSCALED_V
#define VECTOR4_ADDSCALED_V
	VEC4_INLINE void Vector4::AddScaled(Vector4Param a,Vector4Param s)
	{
		xyzw = __vmaddfp(a, s, xyzw);
	}
#endif // VECTOR4_ADDSCALED_V

#ifndef VECTOR4_ADDSCALED_VV
#define VECTOR4_ADDSCALED_VV
	VEC4_INLINE void Vector4::AddScaled(Vector4Param a,Vector4Param b,Vector4Param s)
	{
		xyzw = __vmaddfp(b, s, a);
	}
#endif // VECTOR4_ADDSCALED_VV

#ifndef VECTOR4_SUBTRACT_V
#define VECTOR4_SUBTRACT_V
	VEC4_INLINE void Vector4::Subtract(Vector4Param a)
	{
		xyzw = __vsubfp(xyzw, a);
	}
#endif // VECTOR4_SUBTRACT_V

#ifndef VECTOR4_SUBTRACT_VV
#define VECTOR4_SUBTRACT_VV
	VEC4_INLINE void Vector4::Subtract(Vector4Param a,Vector4Param b)
	{
		xyzw = __vsubfp(a, b);
	}
#endif // VECTOR4_SUBTRACT_VV


#ifndef VECTOR4_SUBTRACTSCALED_V
#define VECTOR4_SUBTRACTSCALED_V
	VEC4_INLINE void Vector4::SubtractScaled(Vector4Param a, Vector4Param s)
	{
		xyzw = __vnmsubfp(a, s, xyzw);
	}
#endif // VECTOR4_SUBTRACTSCALED_V

#ifndef VECTOR4_SUBTRACTSCALED_VV
#define VECTOR4_SUBTRACTSCALED_VV
	VEC4_INLINE void Vector4::SubtractScaled(Vector4Param a, Vector4Param b,Vector4Param s)
	{
		xyzw = __vnmsubfp(b, s, a);
	}
#endif // VECTOR4_SUBTRACTSCALED_VV

#ifndef VECTOR4_MULTIPLY
#define VECTOR4_MULTIPLY
	VEC4_INLINE void Vector4::Multiply(Vector4Param a)
	{
		xyzw = __vmulfp(xyzw, a);
	}
#endif // VECTOR4_MULTIPLY

#ifndef VECTOR4_MULTIPLY_V
#define VECTOR4_MULTIPLY_V
	VEC4_INLINE void Vector4::Multiply(Vector4Param a,Vector4Param b)
	{
		xyzw = __vmulfp(a, b);
	}
#endif // VECTOR4_MULTIPLY_V

#ifndef VECTOR4_NEGATE
#define VECTOR4_NEGATE
	VEC4_INLINE void Vector4::Negate()
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//__vector4 zero = __vxor(xyzw, xyzw);
		__vector4 zero = _vzerofp;
		xyzw = __vsubfp(zero, xyzw);
	}
#endif // VECTOR4_NEGATE

#ifndef VECTOR4_NEGATE_V
#define VECTOR4_NEGATE_V
	VEC4_INLINE void Vector4::Negate(Vector4Param a)
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//__vector4 zero = __vxor(a, a);
		__vector4 zero = _vzerofp;
		xyzw = __vsubfp(zero, a);
	}
#endif // VECTOR4_NEGATE_V

#ifndef VECTOR4_ABS
#define VECTOR4_ABS
	VEC4_INLINE void Vector4::Abs()
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//__vector4 zero = __vxor(xyzw, xyzw);
		__vector4 zero = _vzerofp;
		xyzw = __vmaxfp(xyzw, __vsubfp(zero, xyzw));
	}
#endif // VECTOR4_ABS

#ifndef VECTOR4_ABS_V
#define VECTOR4_ABS_V
	VEC4_INLINE void Vector4::Abs(Vector4Param a)
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//__vector4 zero = __vxor(a, a);
		__vector4 zero = _vzerofp;
		xyzw = __vmaxfp(a, __vsubfp(zero, a));
	}
#endif // VECTOR4_ABS_V

#ifndef VECTOR4_INVERT
#define VECTOR4_INVERT
	VEC4_INLINE void Vector4::Invert()
	{
		xyzw = NewtonRaphsonRecip4(xyzw);
	}
#endif // VECTOR4_INVERT

#ifndef VECTOR4_INVERT_V
#define VECTOR4_INVERT_V
	VEC4_INLINE void Vector4::Invert(Vector4Param a)
	{
		xyzw = NewtonRaphsonRecip4(a);
	}
#endif // VECTOR4_INVERT_V

#ifndef VECTOR4_NORMALIZE
#define VECTOR4_NORMALIZE
	VEC4_INLINE void Vector4::Normalize()
	{
		__vector4 dot = __vdot4fp(xyzw, xyzw);
		__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR4_NORMALIZE

#ifndef VECTOR4_NORMALIZE_FAST
#define VECTOR4_NORMALIZE_FAST
	VEC4_INLINE void Vector4::NormalizeFast()
	{
		__vector4 dot = __vdot4fp(xyzw, xyzw);
		__vector4 rsqrt = __vrsqrtefp(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR4_NORMALIZE_FAST

#ifndef VECTOR4_NORMALIZE_V
#define VECTOR4_NORMALIZE_V
	VEC4_INLINE void Vector4::Normalize(Vector4Param a)
	{
		__vector4 dot = __vdot4fp(a, a);
		__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
		xyzw = __vmulfp(a, rsqrt);
	}
#endif // VECTOR4_NORMALIZE_V

#ifndef VECTOR4_NORMALIZE_FAST_V
#define VECTOR4_NORMALIZE_FAST_V
	VEC4_INLINE void Vector4::NormalizeFast(Vector4Param a)
	{
		__vector4 dot = __vdot4fp(a, a);
		__vector4 rsqrt = __vrsqrtefp(dot);
		xyzw = __vmulfp(a, rsqrt);
	}
#endif // VECTOR4_NORMALIZE_FAST_V

#ifndef VECTOR4_NORMALIZE3
#define VECTOR4_NORMALIZE3
	VEC4_INLINE void Vector4::Normalize3()
	{
		__vector4 dot = __vdot3fp(xyzw, xyzw);
		__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR4_NORMALIZE3

#ifndef VECTOR4_NORMALIZE_FAST3
#define VECTOR4_NORMALIZE_FAST3
	VEC4_INLINE void Vector4::NormalizeFast3()
	{
		__vector4 dot = __vdot3fp(xyzw, xyzw);
		__vector4 rsqrt = __vrsqrtefp(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR4_NORMALIZE_FAST3

#ifndef VECTOR4_NORMALIZE3_V
#define VECTOR4_NORMALIZE3_V
	VEC4_INLINE void Vector4::Normalize3(Vector4Param a)
	{
		__vector4 dot = __vdot3fp(a, a);
		__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
		xyzw = __vmulfp(a, rsqrt);
	}
#endif // VECTOR4_NORMALIZE3_V

#ifndef VECTOR4_NORMALIZE_FAST3_V
#define VECTOR4_NORMALIZE_FAST3_V
	VEC4_INLINE void Vector4::NormalizeFast3(Vector4Param a)
	{
		__vector4 dot = __vdot3fp(a, a);
		__vector4 rsqrt = __vrsqrtefp(dot);
		xyzw = __vmulfp(a, rsqrt);
	}
#endif // VECTOR4_NORMALIZE_FAST3_V

#ifndef VECTOR4_DOT
#define VECTOR4_DOT
	VEC4_INLINE float Vector4::Dot(Vector4Param a) const
	{
		__vector4 temp = __vdot4fp(xyzw, a);
		return _vscalar(temp);
	}
#endif // VECTOR4_DOT

#ifndef VECTOR4_DOT_V
#define VECTOR4_DOT_V
	VEC4_INLINE Vector4 Vector4::DotV(Vector4Param a) const
	{
		return __vdot4fp(xyzw, a);
	}
#endif // VECTOR4_DOT_V

#ifndef VECTOR4_DOT3
#define VECTOR4_DOT3
	VEC4_INLINE float Vector4::Dot3(Vector4Param a) const
	{
		__vector4 temp = __vdot3fp(xyzw, a);
		return _vscalar(temp);
	}
#endif // VECTOR4_DOT3

#ifndef VECTOR4_DOT3_V
#define VECTOR4_DOT3_V
	VEC4_INLINE float Vector4::Dot3(Vector3Param a) const
	{
#if HACK_GTA4
		// __vdot3fp() seems to work as expected with Vector3:
//		__vector4 temp = __vdot3fp(xyzw, __vand(a, VEC4_ANDW));
		__vector4 temp = __vdot3fp(xyzw, a);	
#else
		__vector4 temp = __vdot3fp(xyzw, a);
#endif // HACK_GTA4...
		return _vscalar(temp);
	}
#endif // VECTOR4_DOT3_V

#ifndef VECTOR4_DOT3V_V
#define VECTOR4_DOT3V_V
    VEC4_INLINE Vector4 Vector4::Dot3V(Vector4Param a) const
	{
		return __vdot3fp(xyzw, a);
	}
#endif // VECTOR4_DOT3V_V

#ifndef VECTOR4_DOT3V_V3
#define VECTOR4_DOT3V_V3
    VEC4_INLINE Vector4 Vector4::Dot3V(Vector3Param a) const
	{
#if HACK_GTA4
		// __vdot3fp() seems to work as expected with Vector3:
//		return __vdot3fp(xyzw, __vand(a, VEC4_ANDW));
		return __vdot3fp(xyzw, a);
#else
		return __vdot3fp(xyzw, a);
#endif //HACK_GTA4...
	}
#endif // VECTOR4_DOT3V_V3

#ifndef VECTOR4_AVERAGE
#define VECTOR4_AVERAGE
	VEC4_INLINE void Vector4::Average(Vector4Param a)
	{
		xyzw = __vmulfp(__vaddfp(xyzw, a), VEC4_HALF);
	}
#endif // VECTOR4_AVERAGE

#ifndef VECTOR4_AVERAGE_V
#define VECTOR4_AVERAGE_V
	VEC4_INLINE void Vector4::Average(Vector4Param a, Vector4Param b)
	{
		xyzw = __vmulfp(__vaddfp(b, a), VEC4_HALF);
	}
#endif // VECTOR4_AVERAGE_V

#ifndef VECTOR4_LERPV_V
#define VECTOR4_LERPV_V
	VEC4_INLINE void Vector4::Lerp(Vector4Param t, Vector4Param a, Vector4Param b)
	{
		xyzw = __vmaddfp(__vsubfp(b, a), t, a);
	}
#endif // VECTOR4_LERPV_V

#ifndef VECTOR4_LERPV
#define VECTOR4_LERPV
	VEC4_INLINE void Vector4::Lerp(Vector4Param t, Vector4Param a)
	{
		xyzw = __vmaddfp(__vsubfp(a, xyzw), t, xyzw);
	}
#endif // VECTOR4_LERPV

#ifndef VECTOR4_POW
#define VECTOR4_POW
	VEC4_INLINE void Vector4::Pow(Vector4Param a, Vector4Param b)
	{
		// pow( x, y ) = exp( y*log(x) )
		xyzw = __vexptefp(__vmulfp(b, __vlogefp(a)));
	}
#endif // VECTOR4_POW

#ifndef VECTOR4_EXP
#define VECTOR4_EXP
	VEC4_INLINE void Vector4::Exp(Vector4Param a)
	{
		xyzw = __vexptefp(a);
	}
#endif // VECTOR4_EXP

#ifndef VECTOR4_LOG
#define VECTOR4_LOG
	VEC4_INLINE void Vector4::Log()
	{
		xyzw = __vlogefp(xyzw);
	}
#endif // VECTOR4_LOG

#ifndef VECTOR4_LOG_V
#define VECTOR4_LOG_V
	VEC4_INLINE void Vector4::Log(Vector4Param v)
	{
		xyzw = __vlogefp(v);
	}
#endif	// VECTOR4_LOG_V

#ifndef VECTOR4_LOG10
#define VECTOR4_LOG10
	VEC4_INLINE void Vector4::Log10()
	{
		xyzw = __vmulfp(__vlogefp(xyzw), l2tol10);
	}
#endif // VECTOR4_LOG10

#ifndef VECTOR4_LOG10_V
#define VECTOR4_LOG10_V
	VEC4_INLINE void Vector4::Log10(Vector4Param v)
	{
		xyzw = __vmulfp(__vlogefp(v), l2tol10);
	}
#endif // VECTOR4_LOG10_V

#ifndef VECTOR4_SQRTV
#define VECTOR4_SQRTV
	VEC4_INLINE Vector4 Vector4::SqrtV() const
	{
		return NewtonRaphsonSqrt(xyzw);
	}
#endif // VECTOR4_SQRTV


#ifndef VECTOR4_MAGV
#define VECTOR4_MAGV
	VEC4_INLINE Vector4 Vector4::MagV() const
	{
		return NewtonRaphsonSqrt(__vdot4fp(xyzw, xyzw));
	}
#endif // VECTOR4_MAGV

#ifndef VECTOR4_MAG2V
#define VECTOR4_MAG2V
	VEC4_INLINE Vector4 Vector4::Mag2V() const
	{
		return __vdot4fp(xyzw, xyzw);
	}
#endif // VECTOR4_MAG2V

#ifndef VECTOR4_MAG3V
#define VECTOR4_MAG3V
	VEC4_INLINE Vector4 Vector4::Mag3V() const
	{
		return NewtonRaphsonSqrt(__vdot3fp(xyzw, xyzw));
	}
#endif // VECTOR4_MAG3V

#ifndef VECTOR4_MAG32V
#define VECTOR4_MAG32V
	VEC4_INLINE Vector4 Vector4::Mag32V() const
	{
		return __vdot3fp(xyzw, xyzw);
	}
#endif // VECTOR4_MAG32V

#ifndef VECTOR4_INVMAGV
#define VECTOR4_INVMAGV
	VEC4_INLINE Vector4 Vector4::InvMagV() const
	{
		return NewtonRaphsonRsqrt(__vdot4fp(xyzw, xyzw));
	}
#endif // VECTOR4_INVMAGV

#ifndef VECTOR4_INVMAG3V
#define VECTOR4_INVMAG3V
	VEC4_INLINE Vector4 Vector4::InvMag3V() const
	{
		return NewtonRaphsonRsqrt(__vdot3fp(xyzw, xyzw));
	}
#endif // VECTOR4_INVMAG3V

#ifndef VECTOR4_DISTV
#define VECTOR4_DISTV
	VEC4_INLINE Vector4 Vector4::DistV(Vector4Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return NewtonRaphsonSqrt(__vdot4fp(temp, temp));
	}
#endif // VECTOR4_DISTV

#ifndef VECTOR4_INVDISTV
#define VECTOR4_INVDISTV
	VEC4_INLINE Vector4 Vector4::InvDistV(Vector4Param a) const	
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return NewtonRaphsonRsqrt(__vdot4fp(temp, temp));
	}
#endif // VECTOR4_INVDISTV

#ifndef VECTOR4_DIST2V
#define VECTOR4_DIST2V
	VEC4_INLINE Vector4 Vector4::Dist2V(Vector4Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return __vdot4fp(temp, temp);
	}
#endif // VECTOR4_DIST2V

#ifndef VECTOR4_INVDIST2V
#define VECTOR4_INVDIST2V
    VEC4_INLINE Vector4 Vector4::InvDist2V(Vector4Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		temp = __vdot4fp(temp, temp);
		return NewtonRaphsonRecip4(temp);
	}
#endif // VECTOR4_INVDIST2V

#ifndef VECTOR4_DIST3V
#define VECTOR4_DIST3V
	VEC4_INLINE Vector4 Vector4::Dist3V(Vector4Param a) const	
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return NewtonRaphsonSqrt(__vdot3fp(temp, temp));
	}
#endif // VECTOR4_DIST3V

#ifndef VECTOR4_INVDIST3V
#define VECTOR4_INVDIST3V
	VEC4_INLINE Vector4 Vector4::InvDist3V(Vector4Param a) const	
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return NewtonRaphsonRsqrt(__vdot3fp(temp, temp));
	}
#endif // VECTOR4_INVDIST3V

#ifndef VECTOR4_DIST32V
#define VECTOR4_DIST32V
	VEC4_INLINE Vector4 Vector4::Dist32V(Vector4Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		temp = __vdot3fp(temp, temp);
		return NewtonRaphsonRecip4(temp);
	}
#endif // VECTOR4_DIST32V

#ifndef VECTOR4_INVDIST32V
#define VECTOR4_INVDIST32V
	VEC4_INLINE Vector4 Vector4::InvDist32V(Vector4Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		temp = __vdot3fp(temp, temp);
		return NewtonRaphsonRecip4(temp);
	}
#endif // VECTOR4_INVDIST32V

#ifndef VECTOR4_FLOAT_TO_INT
#define VECTOR4_FLOAT_TO_INT
	VEC4_INLINE void Vector4::FloatToInt()
	{
		xyzw = (__vector4) __vctsxs(xyzw, 0);
	}
#endif // VECTOR4_FLOAT_TO_INT

#if __XENON
#ifndef VECTOR4_INT_TO_FLOAT
#define VECTOR4_INT_TO_FLOAT
	VEC4_INLINE void Vector4::IntToFloat()
	{
		xyzw = __vcfsx((_ivector4) xyzw, 0);
	}
#endif // VECTOR4_INT_TO_FLOAT
#endif

#ifndef VECTOR4_ROUND_TO_NEAREST_INT
#define VECTOR4_ROUND_TO_NEAREST_INT
	VEC4_INLINE void Vector4::RoundToNearestInt()
	{
		xyzw = (__vector4) __vrfin(xyzw);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_ZERO
#define VECTOR4_ROUND_TO_NEAREST_INT_ZERO
	VEC4_INLINE void Vector4::RoundToNearestIntZero()
	{
		xyzw = __vrfiz(xyzw);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_ZERO

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF
#define VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF
	VEC4_INLINE void Vector4::RoundToNearestIntNegInf()
	{
		xyzw = __vrfin(xyzw);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_POS_INF
#define VECTOR4_ROUND_TO_NEAREST_INT_POS_INF
	VEC4_INLINE void Vector4::RoundToNearestIntPosInf()
	{
		xyzw = __vrfip(xyzw);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_POS_INF

#ifndef VECTOR4_ISZERO
#define VECTOR4_ISZERO
	VEC4_INLINE bool Vector4::IsZero() const
	{
		return _vequalfp(_vzerofp, xyzw) != 0;
	}
#endif // VECTOR4_ISZERO

#ifndef VECTOR4_ISNONZERO
#define VECTOR4_ISNONZERO
	VEC4_INLINE bool Vector4::IsNonZero() const
	{
		return !_vequalfp(_vzerofp, xyzw);
	}
#endif // VECTOR4_ISNONZERO

#ifndef VECTOR4_ISEQUAL
#define VECTOR4_ISEQUAL
	VEC4_INLINE bool Vector4::IsEqual(Vector4Param a) const
	{
		return _vequalfp(a, xyzw) != 0;
	}
#endif // VECTOR4_ISEQUAL

#ifndef VECTOR4_ISEQUALV
#define VECTOR4_ISEQUALV
	VEC4_INLINE Vector4 Vector4::IsEqualV(Vector4Param a) const
	{
		return Vector4((__vector4) __vcmpeqfp(a, xyzw));
	}
#endif // VECTOR4_ISEQUALV

#if !defined(VECTOR4_ISEQUALIV) && __XENON	// TODO: Dunno why PS3 doesn't like this!
#define VECTOR4_ISEQUALIV
	VEC4_INLINE Vector4 Vector4::IsEqualIV(Vector4Param a) const
	{
		_uvector4 temp = __vcmpequw((_uvector4)a, (_uvector4)xyzw);
		return Vector4( (__vector4) temp );
	}
#endif // VECTOR4_ISEQUALIV


#ifndef VECTOR4_ISNOTEQUAL
#define VECTOR4_ISNOTEQUAL
	VEC4_INLINE bool Vector4::IsNotEqual(Vector4Param a) const
	{
		return !_vequalfp(a, xyzw);
	}
#endif // VECTOR4_ISNOTEQUAL

#ifndef VECTOR4_ISCLOSE_V
#define VECTOR4_ISCLOSE_V
	VEC4_INLINE bool Vector4::IsClose(Vector4Param a, Vector4Param eps) const
	{
		__vector4 neps = __vsubfp(a, eps);
		__vector4 peps = __vaddfp(a, eps);
		_uvector4 test1 = __vcmpgefp(xyzw, neps);
		_uvector4 test2 = __vcmpgefp(peps, xyzw);
		_uvector4 both = __vand(test1, test2);
		return _vequal(both,_vall1) != 0;
	}
#endif // VECTOR4_ISCLOSE_V

#ifndef VECTOR4_ISGREATERTHAN
#define VECTOR4_ISGREATERTHAN
	VEC4_INLINE bool Vector4::IsGreaterThan(Vector4Param a) const
	{
		return _vgreaterfp(xyzw, a) != 0;
	}
#endif // VECTOR4_ISGREATERTHAN

#ifndef VECTOR4_ISGREATERTHANV
#define VECTOR4_ISGREATERTHANV
	VEC4_INLINE Vector4 Vector4::IsGreaterThanV(Vector4Param a) const
	{
		return Vector4((__vector4) __vcmpgtfp(xyzw, a));
	}
#endif // VECTOR4_ISGREATERTHANV

#if !defined(VECTOR4_ISGREATERTHANVR) && __XENON
#define VECTOR4_ISGREATERTHANVR
	VEC4_INLINE Vector4 Vector4::IsGreaterThanVR(Vector4Param a, u32& r) const
	{
		return __vcmpgtfpR(xyzw, a, &r);
	}
#endif // VECTOR4_ISGREATERTHANVR

#ifndef VECTOR4_ISLESSTHAN
#define VECTOR4_ISLESSTHAN
	VEC4_INLINE bool Vector4::IsLessThanAll(Vector4Param a) const
	{
		return _vgreaterfp(a, xyzw) != 0;
	}

	VEC4_INLINE bool Vector4::IsLessThanDoNotUse(Vector4Param a) const
	{
		return !_vgequalfp(xyzw, a);
	}
#endif // VECTOR4_ISLESSTHAN

#ifndef VECTOR4_ISLESSTHANV
#define VECTOR4_ISLESSTHANV
	VEC4_INLINE Vector4 Vector4::IsLessThanV(Vector4Param a) const
	{
		return Vector4((__vector4) __vcmpgtfp(a, xyzw));
	}
#endif // VECTOR4_ISLESSTHANV

#if !defined(VECTOR4_ISLESSTHANVR) && __XENON
#define VECTOR4_ISLESSTHANVR
	VEC4_INLINE Vector4 Vector4::IsLessThanVR(Vector4Param a, u32& r) const
	{
		return __vcmpgtfpR(a, xyzw, &r);
	}
#endif // VECTOR4_ISLESSTHANVR


#ifndef VECTOR4_SELECT
#define VECTOR4_SELECT
	VEC4_INLINE Vector4 Vector4::Select(Vector4Param zero, Vector4Param nonZero) const
	{
		return __vsel(zero, nonZero, xyzw);
	}
#endif // VECTOR4_SELECT
#ifndef VECTOR4_COMPUTEPLANE_VVV
#define VECTOR4_COMPUTEPLANE_VVV
	inline void Vector4::ComputePlane(Vector3::Param a,Vector3::Param b, Vector3::Param c)
	{
		Vector3 norm;
		Vector4 wV;
		Vector3 ab = (Vector3)b - a;
		Vector3 ac = (Vector3)c - a;
		norm.Cross(ab, ac);
		norm.Normalize();
		xyzw = norm.DotV(a).xyzw;
#if __XENON
		xyzw = __vrlimi(norm.xyzw, xyzw, 1, 0);
#else
		And(VEC3_MASKW);
		norm.And(VEC3_ANDW);
		Or(norm.xyzw);
#endif
	}
#endif // VECTOR4_COMPUTEPLANE_VVV

#ifndef VECTOR4_MAX
#define VECTOR4_MAX
	VEC4_INLINE void Vector4::Max(Vector4Param a, Vector4Param b)
	{
		xyzw = __vmaxfp(a, b);
	}
#endif // VECTOR4_MAX

#ifndef VECTOR4_MIN
#define VECTOR4_MIN
	VEC4_INLINE void Vector4::Min(Vector4Param a, Vector4Param b)
	{
		xyzw = __vminfp(a, b);
	}
#endif // VECTOR4_MIN

#ifndef VECTOR4_AND
#define VECTOR4_AND
	VEC4_INLINE void Vector4::And(Vector4Param _and)
	{
		xyzw = __vand(xyzw, _and);
	}
#endif // VECTOR4_AND

#ifndef VECTOR4_OR
#define VECTOR4_OR
	VEC4_INLINE void Vector4::Or(Vector4Param _or)
	{
		xyzw = __vor(xyzw, _or);
	}
#endif // VECTOR4_OR

#ifndef VECTOR4_XOR
#define VECTOR4_XOR
	VEC4_INLINE void Vector4::Xor(Vector4Param _xor)
	{
		xyzw = __vxor(xyzw, _xor);
	}
#endif // VECTOR4_XOR

#ifndef VECTOR4_MERGEXY
#define VECTOR4_MERGEXY
	VEC4_INLINE void Vector4::MergeXY(Vector4Param vY)
	{
		xyzw = __vmrghw(xyzw, vY);
	}
#endif // VECTOR4_MERGEXY

#ifndef VECTOR4_MERGEXY_V
#define VECTOR4_MERGEXY_V
	VEC4_INLINE void Vector4::MergeXY(Vector4Param vX, Vector4Param vY)
	{
		xyzw = __vmrghw(vX, vY);
	}
#endif // VECTOR4_MERGEXY_V

#ifndef VECTOR4_MERGEZW
#define VECTOR4_MERGEZW
	VEC4_INLINE void Vector4::MergeZW(Vector4Param vW)
	{
		xyzw = __vmrglw(xyzw, vW);
	}
#endif // VECTOR4_MERGEZW

#ifndef VECTOR4_MERGEZW_V
#define VECTOR4_MERGEZW_V
	VEC4_INLINE void Vector4::MergeZW(Vector4Param vZ, Vector4Param vW)
	{
		xyzw = __vmrglw(vZ, vW);
	}
#endif // VECTOR4_MERGEZW_V


#ifndef VECTOR4_PERMUTE
#define VECTOR4_PERMUTE
	template <int permX, int permY, int permZ, int permW>
	VEC4_INLINE void Vector4::Permute(Vector4Param v)
	{
		xyzw = Vec4VectorSwizzle(v, permX, permY, permZ, permW);
	}
#endif // VECTOR4_PERMUTE

#ifndef VECTOR4_OPERATOR_ASSIGN
#define VECTOR4_OPERATOR_ASSIGN
	VEC4_INLINE Vector4& Vector4::operator=(const Vector4& a)
	{
		xyzw = a.xyzw;
		return *this;
	}
#endif // VECTOR4_OPERATOR_ASSIGN

#ifndef VECTOR4_OPERATOR_PLUS
#define VECTOR4_OPERATOR_PLUS
	VEC4_INLINE Vector4 Vector4::operator+(Vector4Param a) const
	{
		return __vaddfp(xyzw, a);
	}
#endif // VECTOR4_OPERATOR_PLUS

#ifndef VECTOR4_OPERATOR_MINUS
#define VECTOR4_OPERATOR_MINUS
	VEC4_INLINE Vector4 Vector4::operator-(Vector4Param a) const
	{
		return __vsubfp(xyzw, a);
	}
#endif // VECTOR4_OPERATOR_MINUS

#ifndef VECTOR4_OPERATOR_NEGATE
#define VECTOR4_OPERATOR_NEGATE
	VEC4_INLINE Vector4 Vector4::operator-() const
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//return __vsubfp(__vxor(xyzw, xyzw), xyzw);
		return __vsubfp(_vzerofp, xyzw);
	}
#endif // VECTOR4_OPERATOR_NEGATE

#ifndef VECTOR4_OPERATOR_MUL_V
#define VECTOR4_OPERATOR_MUL_V
	VEC4_INLINE Vector4 Vector4::operator*(Vector4Param f) const
	{
		return __vmulfp(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_MUL_V

#ifndef VECTOR4_OPERATOR_OR
#define VECTOR4_OPERATOR_OR
	VEC4_INLINE Vector4 Vector4::operator|(Vector4Param f) const
	{
		return __vor(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_OR

#ifndef VECTOR4_OPERATOR_AND
#define VECTOR4_OPERATOR_AND
	VEC4_INLINE Vector4 Vector4::operator&(Vector4Param f) const
	{
		return __vand(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_AND

#ifndef VECTOR4_OPERATOR_XOR
#define VECTOR4_OPERATOR_XOR
	VEC4_INLINE Vector4 Vector4::operator^(Vector4Param f) const
	{
		return __vxor(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_XOR

#ifndef VECTOR4_OPERATOR_PLUSEQUAL
#define VECTOR4_OPERATOR_PLUSEQUAL
	VEC4_INLINE void Vector4::operator+=(Vector4Param V)
	{
		xyzw = __vaddfp(xyzw, V);
	}
#endif // VECTOR4_OPERATOR_PLUSEQUAL

#ifndef VECTOR4_OPERATOR_MINUSEQUAL
#define VECTOR4_OPERATOR_MINUSEQUAL
	VEC4_INLINE void Vector4::operator-=(Vector4Param V)	
	{
		xyzw = __vsubfp(xyzw, V);
	}
#endif // VECTOR4_OPERATOR_MINUSEQUAL

#ifndef VECTOR4_OPERATOR_TIMESEQUAL_V
#define VECTOR4_OPERATOR_TIMESEQUAL_V
	VEC4_INLINE void Vector4::operator*=(Vector4Param f)	
	{
		xyzw = __vmulfp(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_TIMESEQUAL_V

#ifndef VECTOR4_OPERATOR_OREQUAL
#define VECTOR4_OPERATOR_OREQUAL
	VEC4_INLINE void Vector4::operator|=(Vector4Param f)
	{
		xyzw = __vor(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_OREQUAL

#ifndef VECTOR4_OPERATOR_ANDEQUAL
#define VECTOR4_OPERATOR_ANDEQUAL
	VEC4_INLINE void Vector4::operator&=(Vector4Param f)
	{
		xyzw = __vand(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_ANDEQUAL

#ifndef VECTOR4_OPERATOR_XOREQUAL
#define VECTOR4_OPERATOR_XOREQUAL
	VEC4_INLINE void Vector4::operator^=(Vector4Param f)
	{
		xyzw = __vxor(xyzw, f);
	}
#endif // VECTOR4_OPERATOR_XOREQUAL

#ifndef VECTOR4_SCALEV
#define VECTOR4_SCALEV
	__forceinline void Vector4::Scale(const Vector4& f)
	{
		xyzw = __vmulfp( xyzw, f.xyzw );
	}
#endif // VECTOR4_SCALEV


} // namespace rage

#endif // VECTOR_VECTOR4_XENON_H
