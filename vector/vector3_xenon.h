// 
// vector/vector3_xenon.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTOR3_XENON_H
#define VECTOR_VECTOR3_XENON_H


// Default Vector3 Implementations

namespace rage
{

#if __XENON
#define Vec3VectorSwizzle(V, E0, E1, E2, E3) __vpermwi	((V), ((E0) << 6) | ((E1) << 4) | ((E2) << 2) | (E3) | \
														(((E0) & ~3) << 6) | (((E1) & ~3) << 6) | \
														(((E2) & ~3) << 6) | (((E3) & ~3) << 6))
#elif __SPU
static VEC3_INLINE __vector4 _v3perm(__vector4 a,_uvector4 control) {
	return (__vector4) vec_perm(a,a,(_ucvector4)control);
}
#define Vec3VectorSwizzle(V, E0, E1, E2, E3)	_v3perm(V,(_uvector4){E0,E1,E2,E3})

#else
static VEC3_INLINE __vector4 _v3perm(__vector4 a,_uvector4 control) {
	return (__vector4) vec_perm(a,a,(_ucvector4)control);
}
#define Vec3VectorSwizzle(V, E0, E1, E2, E3)	_v3perm(V,(_uvector4){E0,E1,E2,E3})
#endif

#define VEC3_CLEAR_W(v)		__vand(v, VEC3_ANDW)

#if !__SPU
extern __vector4 __VECTOR4_ZERO;
extern __vector4 __VECTOR4_ONE;
extern __vector4 __VECTOR4_HALF;
extern __vector4 __VECTOR4_INF;
extern __vector4 __VECTOR4_NAN;
extern __vector4 __VECTOR4_ONLYW;
extern __vector4 __VECTOR4_ANDY;
#endif

VEC3_INLINE __vector4 NewtonRaphsonRecip(__vector4 val)
{
	__vector4 recip = __vrefp(val);
	return __vmaddfp(recip, __vnmsubfp(recip, val, __VECTOR4_ONE), recip);
}
VEC3_INLINE __vector4 NewtonRaphsonRsqrt(__vector4 val)
{
	__vector4 zero = _vzerofp;
	__vector4 rsqrt = __vrsqrtefp(val);
	_uvector4 test = __vcmpeqfp(rsqrt, __VECTOR4_INF);
	rsqrt = __vsel(rsqrt, zero, test);
	__vector4 squaredEstimate = __vmulfp(rsqrt, rsqrt);
	__vector4 halfEstimate = __vmulfp(__VECTOR4_HALF, rsqrt);
	return __vmaddfp(__vnmsubfp(val, squaredEstimate, __VECTOR4_ONE), halfEstimate, rsqrt);
}

VEC3_INLINE __vector4 NewtonRaphsonSqrt(__vector4 val)
{
	return __vmulfp(val, NewtonRaphsonRsqrt(val));
}

	//=============================================================================
	// Implementations

	VEC3_INLINE Vector3::Vector3( _ZeroType )
	{
		xyzw = _vzerofp;
	}

	VEC3_INLINE Vector3::Vector3(const __vector4& set)
	{
		Vec3CheckAlignment();
		xyzw = set;
	}

#ifndef VECTOR3_OPERATOR_ASSIGN
#define VECTOR3_OPERATOR_ASSIGN
	VEC3_INLINE Vector3& Vector3::operator=(const Vector3& v)
	{
		Vec3CheckAlignment();
		xyzw = v.xyzw;
		return *this;
	}
#endif // VECTOR3_OPERATOR_ASSIGN

#ifndef VECTOR3_CONST
#define VECTOR3_CONST
	VEC3_INLINE Vector3::Vector3()
	{
		Vec3CheckAlignment();
		// x, y, z are NOT initialized for performance reasons in this constructor

#if __INIT_NAN
		if (!g_DisableInitNan)
		{
			float temp;
			MakeNan(temp);
			xyzw = _vsplatf(temp);
		}
#endif // __INIT_NAN
	}	//lint !e1541 constructor might possibly not initialize
#endif // VECTOR3_CONST

#ifndef VECTOR3_CONST_V
#define VECTOR3_CONST_V
	VEC3_INLINE Vector3::Vector3(const Vector3 &vec)
	{
		Vec3CheckAlignment();
		xyzw = vec;
	}
#endif // VECTOR3_CONSTV

#if VECTORIZED_PADDING
#ifndef VECTOR3_CONST_V3
#define VECTOR3_CONST_V3
	VEC3_INLINE Vector3::Vector3(Vector3Param vecX, Vector3Param vecY, Vector3Param vecZ)
	{
		Vec3CheckAlignment();
		__vector4 yy = __vmrglw(vecY, vecY);
		__vector4 xz = __vmrglw(vecX, vecZ);
		xyzw = __vmrglw(xz, yy);
	}
#endif // VECTOR3_CONSTV3
#endif // VECTORIZED_PADDING

#ifndef VECTOR3_SPLATX
#define VECTOR3_SPLATX
	VEC3_INLINE void Vector3::SplatX(Vector3Param in)
	{
		xyzw = __vspltw(in, 0);
	}
#endif // VECTOR3_SPLATX

#ifndef VECTOR3_SPLATY
#define VECTOR3_SPLATY
	VEC3_INLINE void Vector3::SplatY(Vector3Param in)
	{
		xyzw = __vspltw(in, 1);
	}
#endif // VECTOR3_SPLATY

#ifndef VECTOR3_SPLATZ
#define VECTOR3_SPLATZ
	VEC3_INLINE void Vector3::SplatZ(Vector3Param in)
	{
		xyzw = __vspltw(in, 2);
	}
#endif // VECTOR3_SPLATZ

#ifndef VECTOR3_SPLATW
#define VECTOR3_SPLATW
	VEC3_INLINE void Vector3::SplatW(Vector3Param in)
	{
		xyzw = __vspltw(in, 3);
	}
#endif // VECTOR3_SPLATW

#ifndef VECTOR3_SET_V
#define VECTOR3_SET_V
	VEC3_INLINE void Vector3::Set(const Vector3 &a)
	{
		Vec3CheckAlignment();
		xyzw = a;
	}
#endif // VECTOR3_SETV

#ifndef VECTOR3_SET_F
#define VECTOR3_SET_F
	VEC3_INLINE void Vector3::Set(float s)
	{
		Vec3CheckAlignment();
		xyzw = _vsplatf(s);
	}
#endif // VECTOR3_SETF

#ifndef VECTOR3_SETSCALED
#define VECTOR3_SETSCALED
	VEC3_INLINE void Vector3::SetScaled(Vector3Param a, float s)
	{
		__vector4 splt = _vsplatf(s);
		xyzw = __vmulfp(a, splt);					// xyzw = a * ssss
	}
#endif // VECTOR3_SETSCALED

#ifndef VECTOR3_SETSCALED_V
#define VECTOR3_SETSCALED_V
	VEC3_INLINE void Vector3::SetScaled(Vector3Param a, Vector3Param s)
	{
		xyzw = __vmulfp(a, s);			// xyzw = a * ssss
	}
#endif // VECTOR3_SETSCALED_V

#ifndef VECTOR3_ZERO
#define VECTOR3_ZERO
	VEC3_INLINE void Vector3::Zero()
	{
		Vec3CheckAlignment();
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//xyzw = __vxor(xyzw, xyzw);
		xyzw = _vzerofp;
	}
#endif // VECTOR3_ZERO

#if !defined(VECTOR3_PACK1010102) && __XENON
#define VECTOR3_PACK1010102
	VEC3_INLINE u32 Vector3::Pack1010102() const
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
#endif

#if !defined(VECTOR3_UNPACK1010102) && __XENON
#define VECTOR3_UNPACK1010102
	VEC3_INLINE void Vector3::Unpack1010102(u32 packed)
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
#endif

#if !defined(VECTOR3_OPERATOR_BRACKET_C) && !(__PS3)
#define VECTOR3_OPERATOR_BRACKET_C
	VEC3_INLINE const float &Vector3::operator[](int i) const
	{
		return xyzw.v[i];
	}
#endif // VECTOR3_OPERATORBRACKETC

#if !defined(VECTOR3_OPERATOR_BRACKET) && !(__PS3)
#define VECTOR3_OPERATOR_BRACKET
	VEC3_INLINE float &Vector3::operator[](int i)
	{
		return xyzw.v[i];
	}
#endif // VECTOR3_OPERATORBRACKET

#ifndef VECTOR3_ADD_V
#define VECTOR3_ADD_V
	VEC3_INLINE void Vector3::Add(Vector3Param a)
	{
		xyzw = __vaddfp(xyzw, a);
	}
#endif // VECTOR3_ADDV

#ifndef VECTOR3_ADD_V2
#define VECTOR3_ADD_V2
	VEC3_INLINE void Vector3::Add(Vector3Param a, Vector3Param b)
	{
		xyzw = __vaddfp(b, a);
	}
#endif // VECTOR3_ADDV2

#ifndef VECTOR3_ADDSCALED
#define VECTOR3_ADDSCALED
	VEC3_INLINE void Vector3::AddScaled(Vector3Param a,float s)
	{
		__vector4 splt = _vsplatf(s);
		xyzw = __vmaddfp(a, splt, xyzw);		// xyzw = (a * ssss) + xyzw
	}
#endif // VECTOR3_ADDSCALED

#ifndef VECTOR3_ADDSCALED_V
#define VECTOR3_ADDSCALED_V
	VEC3_INLINE void Vector3::AddScaled(Vector3Param a, Vector3Param s)
	{
		xyzw = __vmaddfp(a, s, xyzw);		// xyzw = (a * ssss) + xyzw
	}
#endif // VECTOR3_ADDSCALED_V

#ifndef VECTOR3_ADDSCALED_2
#define VECTOR3_ADDSCALED_2
	VEC3_INLINE void Vector3::AddScaled(Vector3Param a, Vector3Param b,float s)
	{
		__vector4 splt = _vsplatf(s);
		xyzw = __vmaddfp(b, splt, a);		// xyzw = (b * ssss) + a
	}
#endif // VECTOR3_ADDSCALED2

#ifndef VECTOR3_ADDSCALED_2V
#define VECTOR3_ADDSCALED_2V
	VEC3_INLINE void Vector3::AddScaled(Vector3Param a, Vector3Param b, Vector3Param s)
	{
		xyzw = __vmaddfp(b, s, a);		// xyzw = (b * ssss) + a
	}
#endif // VECTOR3_ADDSCALED_2V

#ifndef VECTOR3_SUBTRACT_V
#define VECTOR3_SUBTRACT_V
	VEC3_INLINE void Vector3::Subtract(Vector3Param a)
	{
		xyzw = __vsubfp(xyzw, a);
	}
#endif // VECTOR3_SUBTRACTV

#ifndef VECTOR3_SUBTRACT_V2
#define VECTOR3_SUBTRACT_V2
	VEC3_INLINE void Vector3::Subtract(Vector3Param a, Vector3Param b)
	{
		xyzw = __vsubfp(a, b);
	}
#endif

#ifndef VECTOR3_SUBTRACTSCALED
#define VECTOR3_SUBTRACTSCALED
	VEC3_INLINE void Vector3::SubtractScaled(Vector3Param a,float s)
	{
		__vector4 splt = _vsplatf(s);
		xyzw = __vnmsubfp(a, splt, xyzw);		// xyzw = -((a * ssss) - xyzw)
	}
#endif // VECTOR3_SUBTRACTSCALED

#ifndef VECTOR3_SUBTRACTSCALEDV
#define VECTOR3_SUBTRACTSCALEDV
	VEC3_INLINE void Vector3::SubtractScaled(Vector3Param a,Vector3Param s)
	{
		xyzw = __vnmsubfp(a, s, xyzw);		// xyzw = -((a * ssss) - xyzw)
	}
#endif // VECTOR3_SUBTRACTSCALEDV

#ifndef VECTOR3_SUBTRACTSCALED_2
#define VECTOR3_SUBTRACTSCALED_2
	VEC3_INLINE void Vector3::SubtractScaled(Vector3Param a, Vector3Param b,float s)
	{
		__vector4 splt = _vsplatf(s);
		xyzw = __vnmsubfp(b, splt, a);	// xyzw = -((b * ssss) - a)
	}
#endif // VECTOR3_SUBTRACTSCALED2

#ifndef VECTOR3_SUBTRACTSCALED_2V
#define VECTOR3_SUBTRACTSCALED_2V
	VEC3_INLINE void Vector3::SubtractScaled(Vector3Param a, Vector3Param b,Vector3Param s)
	{
		xyzw = __vnmsubfp(b, s, a);	// xyzw = -((b * ssss) - a)
	}
#endif // VECTOR3_SUBTRACTSCALED2V

#ifndef VECTOR3_SCALE_F
#define VECTOR3_SCALE_F
	VEC3_INLINE void Vector3::Scale(float f)
	{
		__vector4 splt = _vsplatf(f);
		xyzw = __vmulfp(xyzw, splt);				// xyzw = xyzw * ffff
	}
#endif // VECTOR3_SCALEF

#ifndef VECTOR3_SCALE_VF
#define VECTOR3_SCALE_VF
	VEC3_INLINE void Vector3::Scale(Vector3Param a,float f)
	{
		__vector4 splt = _vsplatf(f);
		xyzw = __vmulfp(a, splt);				// xyzw = a * ffff
	}
#endif // VECTOR3_SCALEVF

#ifndef VECTOR3_INVSCALE_F
#define VECTOR3_INVSCALE_F
	VEC3_INLINE void Vector3::InvScale(float f)
	{
		__vector4 splt = _vsplatf(f);
		__vector4 temp = NewtonRaphsonRecip(splt);			// temp = 1111 / ffff
		xyzw = __vmulfp(xyzw, temp);				// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR3_INVSCALEF

#ifndef VECTOR3_INVSCALE_FV
#define VECTOR3_INVSCALE_FV
	VEC3_INLINE void Vector3::InvScale(Vector3Param f)
	{
		__vector4 temp = NewtonRaphsonRecip(f);			// temp = 1111 / ffff
		xyzw = __vmulfp(xyzw, temp);				// xyzw = xyzw * (1111 / ffff)
	}
#endif // VECTOR3_INVSCALEFV

#ifndef VECTOR3_INVSCALE_VF
#define VECTOR3_INVSCALE_VF
	VEC3_INLINE void Vector3::InvScale(Vector3Param a,float f)
	{
		__vector4 splt = _vsplatf(f);
		__vector4 temp = NewtonRaphsonRecip(splt);			// temp = 1111 / ffff
		xyzw = __vmulfp(a, temp);				// xyzw = a * (1111 / ffff)
	}
#endif // VECTOR3_INVSCALEF

#ifndef VECTOR3_INVSCALE_VFV
#define VECTOR3_INVSCALE_VFV
	VEC3_INLINE void Vector3::InvScale(Vector3Param a,Vector3Param f)
	{
		__vector4 temp = NewtonRaphsonRecip(__vspltw(f, 0));			// temp = 1111 / ffff
		xyzw = __vmulfp(a, temp);				// xyzw = a * (1111 / ffff)
	}
#endif // VECTOR3_INVSCALEFV

#ifndef VECTOR3_MUL_V
#define VECTOR3_MUL_V
	VEC3_INLINE void Vector3::Multiply(Vector3Param a)
	{
		xyzw = __vmulfp(xyzw, a);
	}
#endif // VECTOR3_MULV

#ifndef VECTOR3_MUL_V2
#define VECTOR3_MUL_V2
	VEC3_INLINE void Vector3::Multiply(Vector3Param a, Vector3Param b)
	{
		xyzw = __vmulfp(b, a);
	}
#endif // VECTOR3_MULV2

#ifndef VECTOR3_NEGATE
#define VECTOR3_NEGATE
	VEC3_INLINE void Vector3::Negate()
	{
		xyzw = __vsubfp(_vzerofp, xyzw);
	}
#endif // VECTOR3_NEGATE

#ifndef VECTOR3_NEGATE_V
#define VECTOR3_NEGATE_V
	VEC3_INLINE void Vector3::Negate(Vector3Param a)
	{
		xyzw = __vsubfp(_vzerofp, a);
	}
#endif // VECTOR3_NEGATEV

#ifndef VECTOR3_INVMAGV
#define VECTOR3_INVMAGV
	VEC3_INLINE Vector3 Vector3::InvMagV() const
	{
		return NewtonRaphsonRsqrt(__vdot3fp(xyzw, xyzw));
	}
#endif // VECTOR3_INVMAGV

#ifndef VECTOR3_INVMAG_FASTV
#define VECTOR3_INVMAG_FASTV
	VEC3_INLINE Vector3 Vector3::InvMagFastV() const
	{
		return __vrsqrtefp(__vdot3fp(xyzw, xyzw));
	}
#endif // VECTOR3_INVMAGV

#ifndef VECTOR3_ABS
#define VECTOR3_ABS
	VEC3_INLINE void Vector3::Abs()
	{
		_ivector4 SignMask = __vspltisw(-1);
		SignMask = __vslw(SignMask,SignMask);
		xyzw = __vandc(xyzw,(__vector4)SignMask);
	}
#endif // VECTOR3_ABS

#ifndef VECTOR3_ABS_V
#define VECTOR3_ABS_V
	VEC3_INLINE void Vector3::Abs(Vector3Param a)
	{
		_ivector4 SignMask = __vspltisw(-1);
		SignMask = __vslw(SignMask,SignMask);
		xyzw = __vandc(a,(__vector4)SignMask);
	}
#endif // VECTOR3_ABSV

#ifndef VECTOR3_INVERT
#define VECTOR3_INVERT
	VEC3_INLINE void Vector3::Invert()
	{
		xyzw = NewtonRaphsonRecip(xyzw);
	}
#endif // VECTOR3_INVERT

#ifndef VECTOR3_INVERT_V
#define VECTOR3_INVERT_V
	VEC3_INLINE void Vector3::Invert(Vector3Param a)
	{
		xyzw = NewtonRaphsonRecip(a);
	}
#endif // VECTOR3_INVERTV

#ifndef VECTOR3_INVERTSAFE
#define VECTOR3_INVERTSAFE
	inline void Vector3::InvertSafe()
	{
		Vector3 vi;
		Vector3 comparitor = Vector3(xyzw).IsZeroV();
		vi.Invert(xyzw);
		xyzw = comparitor.Select( vi, VEC3_MAX );
	}
#endif // VECTOR3_INVERTSAFE

#ifndef VECTOR3_INVERTSAFE_V
#define VECTOR3_INVERTSAFE_V
	inline void Vector3::InvertSafe(Vector3Param a, Vector3Param zeroInverse)
	{
		Vector3 vi;
		Vector3 comparitor = Vector3(a).IsZeroV();
		vi.Invert(a);
		xyzw = comparitor.Select( vi, zeroInverse );
	}
#endif // VECTOR3_IVNERTSAFEV

#ifndef VECTOR3_NORMALIZE
#define VECTOR3_NORMALIZE
	VEC3_INLINE void Vector3::Normalize()
	{
		__vector4 dot = __vdot3fp(xyzw, xyzw);
		__vector4 rsqrt = NewtonRaphsonRsqrt(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR3_NORMALIZE

#ifndef VECTOR3_NORMALIZE_FAST
#define VECTOR3_NORMALIZE_FAST
	VEC3_INLINE void Vector3::NormalizeFast()
	{
		__vector4 dot = __vdot3fp(xyzw, xyzw);
		__vector4 rsqrt = __vrsqrtefp(dot);
		xyzw = __vmulfp(xyzw, rsqrt);
	}
#endif // VECTOR3_NORMALIZE_FAST

#ifndef VECTOR3_NORMALIZE_V
#define VECTOR3_NORMALIZE_V
	VEC3_INLINE void Vector3::Normalize(Vector3Param a)
	{
		xyzw = __vmulfp(a, NewtonRaphsonRsqrt(__vdot3fp(a, a)));
	}
#endif // VECTOR3_NORMALIZEV

#ifndef VECTOR3_NORMALIZE_FAST_V
#define VECTOR3_NORMALIZE_FAST_V
	VEC3_INLINE void Vector3::NormalizeFast(Vector3Param a)
	{
		xyzw = __vmulfp(a, __vrsqrtefp(__vdot3fp(a, a)));
	}
#endif // VECTOR3_NORMALIZEV

#ifndef VECTOR3_CROSS
#define VECTOR3_CROSS
	VEC3_INLINE void Vector3::Cross(Vector3Param a)
	{
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(xyzw,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmulfp(v1, v2);
		v1 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(xyzw,		VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		xyzw = __vnmsubfp(v1, v2, temp);
	}
#endif // VECTOR3_CROSS

#ifndef VECTOR3_CROSS_2
#define VECTOR3_CROSS_2
	VEC3_INLINE void Vector3::Cross(Vector3Param a, Vector3Param b)
	{
		//x=a.y*b.z-a.z*b.y;
		//y=a.z*b.x-a.x*b.z;
		//z=a.x*b.y-a.y*b.x;
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(b,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmulfp(v1, v2);
		v1 = Vec3VectorSwizzle(b,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		xyzw = __vnmsubfp(v1, v2, temp);
	}
#endif // VECTOR3_CROSS2

#ifndef VECTOR3_CROSSSAFE
#define VECTOR3_CROSSSAFE
	VEC3_INLINE void Vector3::CrossSafe(Vector3Param a, Vector3Param b)
	{
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(b,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmulfp(v1, v2);
		v1 = Vec3VectorSwizzle(b,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		xyzw = __vnmsubfp(v1, v2, temp);
	}
#endif // VECTOR3_CROSSAFE

#ifndef VECTOR3_CROSSNEGATE
#define VECTOR3_CROSSNEGATE
	VEC3_INLINE void Vector3::CrossNegate(Vector3Param a)
	{
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(xyzw,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmulfp(v1, v2);
		v1 = Vec3VectorSwizzle(xyzw,		VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		xyzw = __vnmsubfp(v1, v2, temp);
	}
#endif // VECTOR3_CROSSNEGATE



#ifndef VECTOR3_ADDCROSSED
#define VECTOR3_ADDCROSSED
	VEC3_INLINE void Vector3::AddCrossed(Vector3Param a, Vector3Param b)
	{


		//x +=a.y*b.z-a.z*b.y;
		//y +=a.z*b.x-a.x*b.z;
		//z +=a.x*b.y-a.y*b.x;
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(b,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmaddfp(v1, v2,xyzw);
		v1 = Vec3VectorSwizzle(b,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		xyzw = __vnmsubfp(v1, v2, temp);

	}
#endif // VECTOR3_ADDCROSSED

#ifndef VECTOR3_SUBTRACTCROSSED
#define VECTOR3_SUBTRACTCROSSED
	VEC3_INLINE void Vector3::SubtractCrossed(Vector3Param a, Vector3Param b)
	{


		//x -=a.y*b.z-a.z*b.y;
		//y -=a.z*b.x-a.x*b.z;
		//z -=a.x*b.y-a.y*b.x;
		// TODO: Figure out how vpermwi works!!  No documentation
		__vector4 v1 = Vec3VectorSwizzle(b,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 v2 = Vec3VectorSwizzle(a,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		__vector4 temp = __vmulfp(v1, v2);
		v1 = Vec3VectorSwizzle(b,	VEC_PERM_Y, VEC_PERM_Z, VEC_PERM_X, VEC_PERM_W);
		v2 = Vec3VectorSwizzle(a,	VEC_PERM_Z, VEC_PERM_X, VEC_PERM_Y, VEC_PERM_W);
		__vector4 t2 = __vnmsubfp(v1, v2, temp);

		xyzw = __vsubfp( xyzw, t2 );

	}

#endif // VECTOR3_SUBTRACTCROSSED

#ifndef VECTOR3_DOTV
#define VECTOR3_DOTV
	VEC3_INLINE Vector3 Vector3::DotV(Vector3Param a) const
	{
		return __vdot3fp(a, xyzw);
	}
#endif // VECTOR3_DOTV

#ifndef VECTOR3_DOTV2
#define VECTOR3_DOTV2
	VEC3_INLINE void Vector3::DotV(Vector3Param a, Vector3Param b)
	{
		xyzw = __vdot3fp(a, b);
	}
#endif // VECTOR3_DOTV2

#ifndef VECTOR3_AVERAGE
#define VECTOR3_AVERAGE
	VEC3_INLINE void Vector3::Average(Vector3Param a)
	{					
		xyzw = __vmulfp(__vaddfp(xyzw, a), VEC3_HALF);	
	}
#endif // VECTOR3_AVERAGE

#ifndef VECTOR3_AVERAGE_2
#define VECTOR3_AVERAGE_2
	VEC3_INLINE void Vector3::Average(Vector3Param a, Vector3Param b)
	{				
		xyzw = __vmulfp(__vaddfp(b, a), VEC3_HALF);
	}
#endif // VECTOR3_AVERAGE2

#ifndef VECTOR3_LERP
#define VECTOR3_LERP
	VEC3_INLINE void Vector3::Lerp(float t,Vector3Param a)
	{
		__vector4 splt = _vsplatf(t);
		xyzw = __vmaddfp(__vsubfp(a, xyzw), splt, xyzw);
	}
#endif // VECTOR3_LERP

#ifndef VECTOR3_LERPV
#define VECTOR3_LERPV
	VEC3_INLINE void Vector3::Lerp(Vector3Param t,Vector3Param a)
	{
		xyzw = __vmaddfp(__vsubfp(a, xyzw), t, xyzw);			// xyzw = ((a - xyzw) * t) + xyzw
	}
#endif // VECTOR3_LERP

#ifndef VECTOR3_LERP_2
#define VECTOR3_LERP_2
	VEC3_INLINE void Vector3::Lerp(float t,Vector3Param a,Vector3Param b)
	{
		__vector4 splt = _vsplatf(t);
		xyzw = __vmaddfp(__vsubfp(b, a), splt, a);
	}
#endif // VECTOR3_LERP_2

#ifndef VECTOR3_LERP_2V
#define VECTOR3_LERP_2V
	VEC3_INLINE void Vector3::Lerp(Vector3Param t,Vector3Param a,Vector3Param b)
	{
		xyzw = __vmaddfp(__vsubfp(b, a), t, a);			// xyzw = ((b - a) * t) + a
	}
#endif // VECTOR3_LERP_2V

#ifndef VECTOR3_SQRTV
#define VECTOR3_SQRTV
	VEC3_INLINE Vector3 Vector3::SqrtV() const
	{
		return Vector3(NewtonRaphsonSqrt(xyzw));
	}
#endif

#ifndef VECTOR3_RECIPSQRTV
#define VECTOR3_RECIPSQRTV
    VEC3_INLINE Vector3 Vector3::RecipSqrtV() const
    {
        return Vector3(NewtonRaphsonRsqrt(xyzw));
    }
#endif

#ifndef VECTOR3_MAGV
#define VECTOR3_MAGV
	VEC3_INLINE Vector3 Vector3::MagV() const
	{
		return NewtonRaphsonSqrt(__vdot3fp(xyzw, xyzw));
	}
#endif // VECTOR3_MAGV

#ifndef VECTOR3_MAGFASTV
#define VECTOR3_MAGFASTV
	VEC3_INLINE Vector3 Vector3::MagFastV() const
	{
		return __vrefp(__vrsqrtefp(__vdot3fp(xyzw, xyzw)));
	}
#endif // VECTOR3_MAGFASTV

#ifndef VECTOR3_MAG2V
#define VECTOR3_MAG2V
	VEC3_INLINE Vector3 Vector3::Mag2V() const
	{
		return __vdot3fp(xyzw, xyzw);
	}
#endif // VECTOR3_MAG2

#ifndef VECTOR3_FLATMAGV
#define VECTOR3_FLATMAGV
	inline Vector3 Vector3::XZMagV() const
	{
		__vector4 flat = __vand( xyzw, __VECTOR4_ANDY );
		return NewtonRaphsonSqrt(__vdot3fp(flat, flat));
	}
#endif // VECTOR3_FLATMAGV

#ifndef VECTOR3_FLATMAG2V
#define VECTOR3_FLATMAG2V
	inline Vector3 Vector3::XZMag2V() const
	{
		__vector4 flat = __vand( xyzw, __VECTOR4_ANDY );
		return __vdot3fp( flat, flat );
	}			
#endif // VECTOR3_FLATMAG2V

#ifndef VECTOR3_XYMAGV
#define VECTOR3_XYMAGV
	VEC3_INLINE Vector3 Vector3::XYMagV() const
	{
		__vector4 prod = __vmulfp(xyzw, xyzw);
		return NewtonRaphsonSqrt(__vaddfp(__vspltw(prod, 0), __vspltw(prod, 1)));
	}
#endif // VECTOR3_XYMAG

#ifndef VECTOR3_XYMAG2V
#define VECTOR3_XYMAG2V
	VEC3_INLINE Vector3 Vector3::XYMag2V() const
	{
		__vector4 prod = __vmulfp(xyzw, xyzw);
		return __vaddfp(__vspltw(prod, 0), __vspltw(prod, 1));		
	}			
#endif // VECTOR3_XYMAG2

#ifndef VECTOR3_DISTV
#define VECTOR3_DISTV
	VEC3_INLINE Vector3 Vector3::DistV(Vector3Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return NewtonRaphsonSqrt(__vdot3fp(temp, temp));
	}
#endif // VECTOR3_DISTV

#ifndef VECTOR3_INVDISTV
#define VECTOR3_INVDISTV
	VEC3_INLINE Vector3 Vector3::InvDistV(Vector3Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		temp = __vdot3fp(temp, temp);
		return NewtonRaphsonRsqrt(temp);
	}
#endif // VECTOR3_INVDISTV

#ifndef VECTOR3_DIST2V
#define VECTOR3_DIST2V
	VEC3_INLINE Vector3 Vector3::Dist2V(Vector3Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		return __vdot3fp(temp, temp);
	}
#endif // VECTOR3_DIST2V
	
#ifndef VECTOR3_INVDIST2V
#define VECTOR3_INVDIST2V
	VEC3_INLINE Vector3 Vector3::InvDist2V(Vector3Param a) const
	{
		__vector4 temp = __vsubfp(xyzw, a);
		temp = __vdot3fp(temp, temp);
		return NewtonRaphsonRecip(temp);
	}
#endif // VECTOR3_INVDIST2


#ifndef VECTOR3_FLATDIST2V
#define VECTOR3_FLATDIST2V
	inline Vector3 Vector3::XZDist2V(Vector3Param a) const
	{
		__vector4 b = __vsubfp( xyzw, a );
		__vector4 bflat = __vand( b, __VECTOR4_ANDY );
		return __vdot3fp( bflat, bflat );
	}
#endif // VECTOR3_FLATDIST2V



#ifndef VECTOR3_FLATDISTV
#define VECTOR3_FLATDISTV
	inline Vector3 Vector3::XZDistV(Vector3Param a) const
	{
		__vector4 b = __vsubfp( xyzw, a );
		__vector4 bflat = __vand( b, __VECTOR4_ANDY );
		return NewtonRaphsonSqrt(__vdot3fp(bflat, bflat));
	}
#endif // VECTOR3_FLATDIST2V


#ifndef VECTOR3_ISZERO
#define VECTOR3_ISZERO
	VEC3_INLINE bool Vector3::IsZero() const
	{
		return _vequalfp(__vand(xyzw, VEC3_ANDW), _vzerofp) != 0;
	}
#endif // VECTOR3_ISZERO

#ifndef VECTOR3_ISZEROV
#define VECTOR3_ISZEROV
	VEC3_INLINE Vector3 Vector3::IsZeroV() const
	{
		// __vxor creates a false dependency on xyzw, and may also
		// force the compiler to load it from memory, even though its
		// value is irrelevant.
		//__vector4 zero = __vxor(xyzw, xyzw);
		__vector4 zero = _vzerofp;
		__vector4 test3b = __vand(xyzw, VEC3_ANDW);
		return Vector3((__vector4)__vcmpeqfp(zero, test3b));
	}
#endif // VECTOR3_ISZEROV

#ifndef VECTOR3_ISZEROV4
#define VECTOR3_ISZEROV4
	VEC3_INLINE Vector3 Vector3::IsZeroV4() const
	{
		return Vector3((__vector4)__vcmpeqfp(_vzerofp, xyzw));
	}
#endif // VECTOR3_ISZEROV4

#ifndef VECTOR3_ISNONZERO
#define VECTOR3_ISNONZERO
	VEC3_INLINE bool Vector3::IsNonZero() const
	{
		return !_vequalfp(__vand(xyzw, VEC3_ANDW), _vzerofp);
	}
#endif // VECTOR3_ISNONZERO

#ifndef VECTOR3_ISEQUAL
#define VECTOR3_ISEQUAL
	VEC3_INLINE bool Vector3::IsEqual(Vector3Param a) const
	{
		__vector4 test3a = __vand(a, VEC3_ANDW);
		__vector4 test3b = __vand(xyzw, VEC3_ANDW);
		return _vequalfp(test3a, test3b) != 0;
	}
#endif // VECTOR3_ISEQUAL

#ifndef VECTOR3_ISEQUALV
#define VECTOR3_ISEQUALV
	VEC3_INLINE Vector3 Vector3::IsEqualV(Vector3Param a) const
	{
		__vector4 test3a = __vand(a, VEC3_ANDW);
		__vector4 test3b = __vand(xyzw, VEC3_ANDW);
		return Vector3((__vector4)__vcmpeqfp(test3b, test3a));
	}
#endif // VECTOR3_ISEQUALV

#ifndef VECTOR3_ISNOTEQUAL
#define VECTOR3_ISNOTEQUAL
	VEC3_INLINE bool Vector3::IsNotEqual(Vector3Param a) const
	{
		return !IsEqual(a);
	}
#endif // VECTOR3_ISNOTEQUAL

#ifndef VECTOR3_ISCLOSE
#define VECTOR3_ISCLOSE
	VEC3_INLINE bool Vector3::IsClose(Vector3Param a,float eps) const
	{
		__vector4 test3a = __vand(a, VEC3_ANDW);
		__vector4 test3b = __vand(xyzw, VEC3_ANDW);
		__vector4 veps = _vsplatf(eps);
		__vector4 neps = __vsubfp(test3a, veps);
		__vector4 peps = __vaddfp(test3a, veps);
		_uvector4 test1 = __vcmpgefp(test3b, neps);
		_uvector4 test2 = __vcmpgefp(peps, test3b);
		_uvector4 both = __vand(test1, test2);
		return _vequal(both, _vall1) != 0;
	}
#endif // VECTOR3_ISCLOSE

#ifndef VECTOR3_ISCLOSEV
#define VECTOR3_ISCLOSEV
	VEC3_INLINE bool Vector3::IsClose(Vector3Param a,Vector3Param eps) const
	{
		__vector4 test3a = __vand(a, VEC3_ANDW);
		__vector4 test3b = __vand(xyzw, VEC3_ANDW);
		__vector4 veps = __vand(eps, VEC3_ANDW);
		__vector4 neps = __vsubfp(test3a, veps);
		__vector4 peps = __vaddfp(test3a, veps);
		_uvector4 test1 = __vcmpgefp(test3b, neps);
		_uvector4 test2 = __vcmpgefp(peps, test3b);
		_uvector4 both = __vand(test1, test2);
		return _vequal(both, _vall1) != 0;
	}
#endif // VECTOR3_ISCLOSEV

#ifndef VECTOR3_ISGREATERTHAN
#define VECTOR3_ISGREATERTHAN
	VEC3_INLINE bool Vector3::IsGreaterThan(Vector3Param a) const
	{
		return _vgreaterfp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)) != 0;
	}
#endif // VECTOR3_ISGREATERTHAN

#ifndef VECTOR3_ISGREATERTHANV
#define VECTOR3_ISGREATERTHANV
	VEC3_INLINE Vector3 Vector3::IsGreaterThanV(Vector3Param a) const
	{
		return Vector3((__vector4)__vcmpgtfp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)));
	}
#endif // VECTOR3_ISGREATERTHANV

#ifndef VECTOR3_ISGREATERTHANV4
#define VECTOR3_ISGREATERTHANV4
	VEC3_INLINE Vector3 Vector3::IsGreaterThanV4(Vector3Param a) const
	{
		return Vector3((__vector4)__vcmpgtfp(xyzw, a));
	}
#endif // VECTOR3_ISGREATERTHANV

#if !__PPU && !__SPU	// Disabled for PS3 for now, as the __vcmp..R() intrinsics don't exist. /FF
#ifndef VECTOR3_ISGREATERTHANVR
#define VECTOR3_ISGREATERTHANVR
	VEC3_INLINE Vector3 Vector3::IsGreaterThanVR(Vector3Param a, u32& r) const
	{
		return __vcmpgtfpR(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW), &r);
	}
#endif // VECTOR3_ISGREATERTHANVR
#endif // !__PPU && !__SPU

#ifndef VECTOR3_ISGREATEROREQUALTHAN
#define VECTOR3_ISGREATEROREQUALTHAN
	VEC3_INLINE bool Vector3::IsGreaterOrEqualThan(Vector3Param a) const
	{
		return _vgequalfp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)) != 0;
	}
#endif // VECTOR3_ISGREATEROREQUALTHAN

#ifndef VECTOR3_ISGREATEROREQUALTHANV
#define VECTOR3_ISGREATEROREQUALTHANV
	VEC3_INLINE Vector3 Vector3::IsGreaterOrEqualThanV(Vector3Param a) const
	{
		return Vector3((__vector4)__vcmpgefp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)));
	}
#endif // VECTOR3_ISGREATEROREQUALTHANV

#if !__PPU && !__SPU	// Disabled for PS3 for now, as the __vcmp..R() intrinsics don't exist. /FF
#ifndef VECTOR3_ISGREATEROREQUALTHANVR
#define VECTOR3_ISGREATEROREQUALTHANVR
	VEC3_INLINE Vector3 Vector3::IsGreaterOrEqualThanVR(Vector3Param a, u32& r) const
	{
		return __vcmpgefpR(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW), &r);
	}
#endif // VECTOR3_ISGREATERTHANVR
#endif // !__PPU && !__SPU

#ifndef VECTOR3_ISLESSTHAN
#define VECTOR3_ISLESSTHAN
	VEC3_INLINE bool Vector3::IsLessThanAll(Vector3Param a) const
	{
		return _vgreaterfp(__vor(__vand(a, VEC3_ANDW), VEC3_ONEW), __vand(xyzw, VEC3_ANDW)) != 0;
	}

	VEC3_INLINE bool Vector3::IsLessThanDoNotUse(Vector3Param a) const
	{
		return _vgequalfp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)) == 0;
	}
#endif // VECTOR3_ISLESSTHAN

#ifndef VECTOR3_ISLESSTHANV
#define VECTOR3_ISLESSTHANV
	VEC3_INLINE Vector3 Vector3::IsLessThanV(Vector3Param a) const
	{
		__vector4 atest = __vor(__vand(a, VEC3_ANDW), VEC3_ONEW);
		__vector4 test = __vand(xyzw, VEC3_ANDW);
		return Vector3((__vector4)__vcmpgtfp(atest, test));
	}
#endif // VECTOR3_ISLESSTHANV

#ifndef VECTOR3_ISLESSTHANV4
#define VECTOR3_ISLESSTHANV4
	VEC3_INLINE Vector3 Vector3::IsLessThanV4(Vector3Param a) const
	{
		return Vector3((__vector4)__vcmpgtfp(a, xyzw));
	}
#endif // VECTOR3_ISLESSTHANV4

#ifndef VECTOR3_ISLESSOREQUALTHAN
#define VECTOR3_ISLESSOREQUALTHAN
	VEC3_INLINE bool Vector3::IsLessOrEqualThanAll(Vector3Param a) const
	{
		return _vgequalfp(__vor(__vand(a, VEC3_ANDW), VEC3_ONEW), __vand(xyzw, VEC3_ANDW)) != 0;
	}

	VEC3_INLINE bool Vector3::IsLessOrEqualThanDoNotUse(Vector3Param a) const
	{
		return _vgreaterfp(__vor(__vand(xyzw, VEC3_ANDW), VEC3_ONEW), __vand(a, VEC3_ANDW)) == 0;
	}
#endif // VECTOR3_ISLESSOREQUALTHAN

#ifndef VECTOR3_ISLESSOREQUALTHANV
#define VECTOR3_ISLESSOREQUALTHANV
	VEC3_INLINE Vector3 Vector3::IsLessOrEqualThanV(Vector3Param a) const
	{
		__vector4 atest = __vor(__vand(a, VEC3_ANDW), VEC3_ONEW);
		__vector4 test = __vand(xyzw, VEC3_ANDW);
		return Vector3((__vector4)__vcmpgefp(atest, test));
	}
#endif // VECTOR3_ISLESSOREQUALTHANV

#ifndef VECTOR3_ISLESSOREQUALTHANV4
#define VECTOR3_ISLESSOREQUALTHANV4
	VEC3_INLINE Vector3 Vector3::IsLessOrEqualThanV4(Vector3Param a) const
	{
		return Vector3((__vector4)__vcmpgefp(a, xyzw));
	}
#endif // VECTOR3_ISLESSOREQUALTHANV4

#if !__PPU && !__SPU	// Disabled for PS3 for now, as the __vcmp..R() intrinsics don't exist. /FF
#ifndef VECTOR3_ISLESSTHANVR
#define VECTOR3_ISLESSTHANVR
	VEC3_INLINE Vector3 Vector3::IsLessThanVR(Vector3Param a, u32& r) const
	{
		__vector4 atest = __vor(__vand(a, VEC3_ANDW), VEC3_ONEW);
		__vector4 test = __vand(xyzw, VEC3_ANDW);
		return __vcmpgtfpR(atest, test, &r);
	}
#endif // VECTOR3_ISLESSTHANVR

#ifndef VECTOR3_ISLESSTHANVR4
#define VECTOR3_ISLESSTHANVR4
	VEC3_INLINE Vector3 Vector3::IsLessThanVR4(Vector3Param a, u32& r) const
	{
		return __vcmpgtfpR(a, xyzw, &r);
	}
#endif // VECTOR3_ISLESSTHANVR4
#endif // !__PPU && !__SPU

#ifndef VECTOR3_ISTRUETRUETRUE
#define VECTOR3_ISTRUETRUETRUE
	VEC3_INLINE bool Vector3::IsTrueTrueTrue() const
	{
		__vector4 masked = __vand(xyzw, VEC3_ANDW);

		return _vequal(masked, VEC3_ANDW) != 0;
	}
#endif // VECTOR3_ISTRUETRUETRUE

#ifndef VECTOR3_ISFALSEFALSEFALSE
#define VECTOR3_ISFALSEFALSEFALSE
	VEC3_INLINE bool Vector3::IsFalseFalseFalse() const
	{
		__vector4 masked = __vand(xyzw, VEC3_ANDW);

		return _vequal(masked, __VECTOR4_ZERO) != 0;
	}
#endif // VECTOR3_ISFALSEFALSEFALSE

#ifndef VECTOR3_SELECT
#define VECTOR3_SELECT
	VEC3_INLINE Vector3 Vector3::Select(Vector3Param zero, Vector3Param nonZero) const
	{
		return __vsel(zero, nonZero, xyzw);
	}
#endif

#ifndef VECTOR3_MAX
#define VECTOR3_MAX
	VEC3_INLINE void Vector3::Max(Vector3Param a, Vector3Param b)
	{
		xyzw = __vmaxfp(a, b);
	}
#endif // VECTOR3_MAX

#ifndef VECTOR3_MIN
#define VECTOR3_MIN
	VEC3_INLINE void Vector3::Min(Vector3Param a, Vector3Param b)
	{
		xyzw = __vminfp(a, b);
	}
#endif // VECTOR3_MIN

#ifndef VECTOR3_NOT
#define VECTOR3_NOT
	VEC3_INLINE void Vector3::Not()
	{
		Xor(VEC3_MASKXYZW);
	}
#endif // VECTOR3_NOT

#ifndef VECTOR3_AND
#define VECTOR3_AND
	VEC3_INLINE void Vector3::And(Vector3Param and_)
	{
		xyzw = __vand(xyzw, and_);
	}
#endif	// VECTOR3_AND

#ifndef VECTOR3_OR
#define VECTOR3_OR
	VEC3_INLINE void Vector3::Or(Vector3Param or_)
	{
		xyzw = __vor(xyzw, or_);
	}
#endif // VECTOR3_OR

#ifndef VECTOR3_XOR
#define VECTOR3_XOR
	VEC3_INLINE void Vector3::Xor(Vector3Param xor_)
	{
		xyzw = __vxor(xyzw, xor_);
	}
#endif // VECTOR3_XOR

#ifndef VECTOR3_MERGEXY
#define VECTOR3_MERGEXY
	VEC3_INLINE void Vector3::MergeXY(Vector3Param vY)
	{
		xyzw = __vmrghw(xyzw, vY);
	}
#endif // VECTOR3_MERGEXY

#ifndef VECTOR3_MERGEXY_V
#define VECTOR3_MERGEXY_V
	VEC3_INLINE void Vector3::MergeXY(Vector3Param vX, Vector3Param vY)
	{
		xyzw = __vmrghw(vX, vY);
	}
#endif // VECTOR3_MERGEXY_V

#ifndef VECTOR3_MERGEZW
#define VECTOR3_MERGEZW
	VEC3_INLINE void Vector3::MergeZW(Vector3Param vW)
	{
		xyzw = __vmrglw(xyzw, vW);
	}
#endif // VECTOR3_MERGEZW

#ifndef VECTOR3_MERGEZW_V
#define VECTOR3_MERGEZW_V
	VEC3_INLINE void Vector3::MergeZW(Vector3Param vZ, Vector3Param vW)
	{
		xyzw = __vmrglw(vZ, vW);
	}
#endif // VECTOR3_MERGEZW_V

#ifndef VECTOR3_PERMUTE
#define VECTOR3_PERMUTE
    template <int permX, int permY, int permZ, int permW>
    VEC3_INLINE void Vector3::Permute(Vector3Param v)
    {
        xyzw = Vec3VectorSwizzle(v, permX, permY, permZ, permW);
    }
#endif // VECTOR3_PERMUTE

#ifndef VECTOR3_OPERATOR_PLUS_V
#define VECTOR3_OPERATOR_PLUS_V
	VEC3_INLINE Vector3 Vector3::operator+(Vector3Param V) const
	{
		return(Vector3(__vaddfp(xyzw, V)));
	}
#endif // VECTOR3_OPERATORPLUSV

#ifndef VECTOR3_OPERATOR_MINUS_V
#define VECTOR3_OPERATOR_MINUS_V
	VEC3_INLINE Vector3 Vector3::operator-(Vector3Param V) const
	{
		return(Vector3(__vsubfp(xyzw, V)));
	}
#endif // VECTOR3_OPERATORMINUSV

#ifndef VECTOR3_OPERATOR_NEGATE_V
#define VECTOR3_OPERATOR_NEGATE_V
	VEC3_INLINE Vector3 Vector3::operator-() const
	{
		__vector4 _zero = _vzerofp;
		return(Vector3(__vsubfp(_zero, xyzw)));
	}
#endif // VECTOR3_OPERATORNEGATEV

#ifndef VECTOR3_OPERATOR_MUL_F
#define VECTOR3_OPERATOR_MUL_F
	VEC3_INLINE Vector3 Vector3::operator*(const float f) const
	{
		__vector4 splt = _vsplatf(f);
		return(Vector3(__vmulfp(xyzw, splt)));
	}
#endif // VECTOR3_OPERATORMUL_F

#ifndef VECTOR3_OPERATOR_MUL_FV
#define VECTOR3_OPERATOR_MUL_FV
	VEC3_INLINE Vector3 Vector3::operator*(Vector3Param f) const
	{
		return(Vector3(__vmulfp(xyzw, f)));
	}
#endif // VECTOR3_OPERATORMUL _FV

#ifndef VECTOR3_OPERATOR_DIV_F
#define VECTOR3_OPERATOR_DIV_F
	VEC3_INLINE Vector3 Vector3::operator/(const float f) const
	{
		__vector4 splt = _vsplatf(f);
		__vector4 temp = NewtonRaphsonRecip(splt);			// temp = 1111 / ffff
		return(Vector3(__vmulfp(xyzw, temp)));
	}
#endif // VECTOR3_OPERATOR

#ifndef VECTOR3_OPERATOR_DIV_V
#define VECTOR3_OPERATOR_DIV_V
	VEC3_INLINE Vector3 Vector3::operator/(Vector3Param f) const
	{
		return Vector3(__vmulfp(xyzw, NewtonRaphsonRecip(f)));
	}
#endif

#ifndef VECTOR3_OPERATOR_MUL_V
#define VECTOR3_OPERATOR_MUL_V
	VEC3_INLINE Vector3 operator*(const float f,const Vector3& V)
	{
		__vector4 splt = _vsplatf(f);
		return(Vector3(__vmulfp(V, splt)));
	}
#endif // VECTOR3_OPERATORMULV

#ifndef VECTOR3_OPERATOR_OR
#define VECTOR3_OPERATOR_OR
	VEC3_INLINE Vector3 Vector3::operator|(Vector3Param f) const
	{
		return (Vector3(__vor(xyzw, f)));
	}
#endif // VECTOR3_OPERATOR_OR

#ifndef VECTOR3_OPERATOR_AND
#define VECTOR3_OPERATOR_AND
	VEC3_INLINE Vector3 Vector3::operator&(Vector3Param f) const
	{
		return (Vector3(__vand(xyzw, f)));
	}
#endif // VECTOR3_OPERATOR_AND

#ifndef VECTOR3_OPERATOR_XOR
#define VECTOR3_OPERATOR_XOR
	VEC3_INLINE Vector3 Vector3::operator^(Vector3Param f) const
	{
		return (Vector3(__vxor(xyzw, f)));
	}
#endif // VECTOR3_OPERATOR_XOR

#ifndef VECTOR3_OPERATOR_PLUSEQUAL
#define VECTOR3_OPERATOR_PLUSEQUAL
	VEC3_INLINE void Vector3::operator+=(Vector3Param V)
	{
		xyzw = __vaddfp(xyzw, V);
	}
#endif // VECTOR3_OPERATORPLUSEQUAL

#ifndef VECTOR3_OPERATOR_MINUSEQUAL
#define VECTOR3_OPERATOR_MINUSEQUAL
	VEC3_INLINE void Vector3::operator-=(Vector3Param V)
	{
		xyzw = __vsubfp(xyzw, V);
	}
#endif // VECTOR3_OPERATORMINUSEQUAL

#ifndef VECTOR3_OPERATOR_TIMESEQUAL
#define VECTOR3_OPERATOR_TIMESEQUAL
	VEC3_INLINE void Vector3::operator*=(const float f)
	{
		__vector4 splt = _vsplatf(f);
		xyzw = __vmulfp(xyzw, splt);
	}
#endif // VECTOR3_OPERATORTIMESEQUAL

#ifndef VECTOR3_OPERATOR_TIMESEQUAL_V
#define VECTOR3_OPERATOR_TIMESEQUAL_V
	VEC3_INLINE void Vector3::operator*=(Vector3Param f)
	{
		xyzw = __vmulfp(xyzw, f);
	}
#endif // VECTOR3_OPERATORTIMESEQUAL_V

#ifndef VECTOR3_OPERATOR_DIVEQUAL
#define VECTOR3_OPERATOR_DIVEQUAL
	VEC3_INLINE void Vector3::operator/=(const float f)
	{
		__vector4 splt = _vsplatf(f);
		__vector4 temp = NewtonRaphsonRecip(splt);			// temp = 1111 / ffff
		xyzw = __vmulfp(xyzw, temp);
	}
#endif // VECTOR3_OPERATORDIVEQUAL

#ifndef VECTOR3_OPERATOR_DIVEQUAL_V
#define VECTOR3_OPERATOR_DIVEQUAL_V
	VEC3_INLINE void Vector3::operator/=(Vector3Param f)
	{
		xyzw = __vmulfp(xyzw, NewtonRaphsonRecip(f));
	}
#endif // VECTOR3_OPERATORDIVEQUAL_V

#ifndef VECTOR3_OPERATOR_OREQUAL
#define VECTOR3_OPERATOR_OREQUAL
	VEC3_INLINE void Vector3::operator|=(Vector3Param f)
	{
		xyzw = __vor(xyzw, f);
	}
#endif // VECTOR3_OPERATOR_OREQUAL

#ifndef VECTOR3_OPERATOR_ANDEQUAL
#define VECTOR3_OPERATOR_ANDEQUAL
	VEC3_INLINE void Vector3::operator&=(Vector3Param f)
	{
		xyzw = __vand(xyzw, f);
	}
#endif // VECTOR3_OPERATOR_ANDEQUAL

#ifndef VECTOR3_OPERATOR_XOREQUAL
#define VECTOR3_OPERATOR_XOREQUAL
	VEC3_INLINE void Vector3::operator^=(Vector3Param f)
	{
		xyzw = __vxor(xyzw, f);
	}
#endif // VECTOR3_OPERATOR_XOREQUAL

#ifndef VECTOR3_LOG
#define VECTOR3_LOG
	VEC3_INLINE void Vector3::Log()
	{
		xyzw = __vlogefp(xyzw);
	}
#endif // VECTOR3_LOG

#ifndef VECTOR3_LOG_V
#define VECTOR3_LOG_V
	VEC3_INLINE void Vector3::Log(Vector3Param v)
	{
		xyzw = __vlogefp(v);
	}
#endif	// VECTOR3_LOG_V

#ifndef VECTOR3_LOG10
#define VECTOR3_LOG10
	const __vector4 l2tol10 = {0.301f, 0.301f, 0.301f, 0.301f};
	VEC3_INLINE void Vector3::Log10()
	{
		xyzw = __vmulfp(__vlogefp(xyzw), l2tol10);
	}
#endif // VECTOR3_LOG10

#ifndef VECTOR3_LOG10_V
#define VECTOR3_LOG10_V
	VEC3_INLINE void Vector3::Log10(Vector3Param v)
	{
		xyzw = __vmulfp(__vlogefp(v), l2tol10);
	}
#endif // VECTOR3_LOG10_V

#undef Vec3VectorSwizzle
}	// namespace rage

#endif // VECTOR3_XENON_INL
