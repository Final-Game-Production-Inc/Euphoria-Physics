// 
// vector/vector3_default.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
//  

#ifndef VECTOR_VECTOR3_DEFAULT_H
#define VECTOR_VECTOR3_DEFAULT_H


// Default Vector3 Implementations
#include "allbitsf.h"

namespace rage
{
	//=============================================================================
	// Implementations

#ifndef VECTOR3_OPERATOR_ASSIGN
#define VECTOR3_OPERATOR_ASSIGN
	inline Vector3& Vector3::operator=(const Vector3& v)
	{
Vec3CheckAlignment();
		Set(v.x, v.y, v.z);
		w = v.w;
		return *this;
	}
#endif // VECTOR3_OPERATOR_ASSIGN

#ifndef VECTOR3_CONST
#define VECTOR3_CONST
	inline Vector3::Vector3()
	{
		Vec3CheckAlignment();
		// x, y, z are NOT initialized for performance reasons in this constructor

#if __INIT_NAN
		if (!g_DisableInitNan)
		{
			MakeNan(x);
			MakeNan(y);
			MakeNan(z);
			MakeNan(w);
		}
#endif // __INIT_NAN
	}	//lint !e1541 constructor might possibly not initialize
#endif // VECTOR3_CONST

#ifndef VECTOR3_CONST_F3
#define VECTOR3_CONST_F3
	inline Vector3::Vector3(float setX, float setY, float setZ)
		: x(setX)
		, y(setY)
		, z(setZ)
	{
		Vec3CheckAlignment();
#if __INIT_NAN
		if (!g_DisableInitNan)
		{
			MakeNan(w);
		}
#endif // __INIT_NAN
	}
#endif // VECTOR3_CONSTF3

#ifndef VECTOR3_CONST_F4
#define VECTOR3_CONST_F4
	inline Vector3::Vector3(float setX, float setY, float setZ, float setW)
		: x(setX)
		, y(setY)
		, z(setZ)
		, w(setW)
	{
		Vec3CheckAlignment();
	}
#endif // VECTOR3_CONSTF3

#ifndef VECTOR3_CONST_V
#define VECTOR3_CONST_V
	inline Vector3::Vector3(const Vector3 &vec)
		: x(vec.x)
		, y(vec.y)
		, z(vec.z)
#if VECTORIZED_PADDING
		, w(vec.w)
#endif
	{
		Vec3CheckAlignment();
	}
#endif // VECTOR3_CONSTV

#if VECTORIZED_PADDING
#ifndef VECTOR3_CONST_V3
#define VECTOR3_CONST_V3
	inline Vector3::Vector3(Vector3Param vecX, Vector3Param vecY, Vector3Param vecZ)
	{
		Vec3CheckAlignment();
		x = vecX.w;
		y = vecY.w;
		z = vecZ.w;
#if __INIT_NAN
		if (!g_DisableInitNan)
		{
			MakeNan(w);
		}
#endif // __INIT_NAN
	}
#endif // VECTOR3_CONSTV3
#endif

#ifndef VECTOR3_CONST_RES
#define VECTOR3_CONST_RES
	inline Vector3::Vector3(class datResource&)
	{
	} //lint !e1401 
#endif // VECTOR3_CONSTRES

#ifndef VECTOR3_SET_F3
#define VECTOR3_SET_F3
	inline void Vector3::Set(float sx,float sy,float sz)
	{
		Vec3CheckAlignment();
		x=sx;
		y=sy;
		z=sz;
	}
#endif // VECTOR3_SETF3

#ifndef VECTOR3_SET_V
#define VECTOR3_SET_V
	inline void Vector3::Set(const Vector3 &a)
	{
		Vec3CheckAlignment();
		x=a.x;
		y=a.y;
		z=a.z;
	}
#endif // VECTOR3_SETV

#ifndef VECTOR3_SETFLAT
#define VECTOR3_SETFLAT
	inline void Vector3::SetXZ(const Vector2 &a)
	{
		x=a.x; y=0.0f; z=a.y;
	}
#endif // VECTOR3_SETFLAT

#ifndef VECTOR3_SET_F
#define VECTOR3_SET_F
	inline void Vector3::Set(float s)
	{
		x=y=z=s;
	}
#endif // VECTOR3_SETF

#ifndef VECTOR3_SETSCALED
#define VECTOR3_SETSCALED
	inline void Vector3::SetScaled(Vector3Param a, float s)
	{
		x=s*a.x;
		y=s*a.y;
		z=s*a.z;
	}
#endif // VECTOR3_SETSCALED

#ifndef VECTOR3_SETSCALED_V
#define VECTOR3_SETSCALED_V
	inline void Vector3::SetScaled(Vector3Param a, Vector3Param s)
	{
		x = s.x * a.x;
		y = s.y * a.y;
		z = s.z * a.z;
	} 
#endif // VECTOR3_SETSCALED

#ifndef VECTOR3_ZERO
#define VECTOR3_ZERO
	inline void Vector3::Zero()
	{
		x=y=z=0.0f;
	}
#endif // VECTOR3_ZERO

#ifndef VECTOR3_PACK1010102
#define VECTOR3_PACK1010102
	inline u32 Vector3::Pack1010102() const
	{
		return PackFixedPoint(x,10,0) | PackFixedPoint(y,10,10) | PackFixedPoint(z,10,20);
	}
#endif

#ifndef VECTOR3_UNPACK1010102
#define VECTOR3_UNPACK1010102
	inline void Vector3::Unpack1010102(u32 packed)
	{
		int _ux = (packed & 0x3FF) << 22;
		int _uy = ((packed >> 10) & 0x3FF) << 22;
		int _uz = ((packed >> 20) & 0x3FF) << 22;
		float fx = (float)(_ux >> 22);
		float fy = (float)(_uy >> 22);
		float fz = (float)(_uz >> 22);

		Set(fx, fy, fz);
        	Scale(0.001953125f);		// 1 / 512
	}
#endif

#ifndef VECTOR3_GETVECTOR2XY
#define VECTOR3_GETVECTOR2XY
	inline void Vector3::GetVector2XY(Vector2& outVec) const
	{
		outVec.x=x;
		outVec.y=y;
	}
#endif // VECTOR3_GETVECTOR2XY

#ifndef VECTOR3_GETVECTOR2YX
#define VECTOR3_GETVECTOR2YX
	inline void Vector3::GetVector2YX(Vector2& outVec) const
	{
		outVec.x=y;
		outVec.y=x;
	}
#endif // VECTOR3_GETVECTOR2YX

#ifndef VECTOR3_GETVECTOR2XZ
#define VECTOR3_GETVECTOR2XZ
	inline void Vector3::GetVector2XZ(Vector2& outVec)	const
	{
		outVec.x=x;
		outVec.y=z;
	}
#endif // VECTOR3_GETVECTOR2XZ

#ifndef VECTOR3_GETVECTOR2ZX
#define VECTOR3_GETVECTOR2ZX
	inline void Vector3::GetVector2ZX(Vector2& outVec) const
	{
		outVec.x=z;
		outVec.y=x;
	}
#endif // VECTOR3_GETVECTOR2ZX

#ifndef VECTOR3_GETVECTOR2YZ
#define VECTOR3_GETVECTOR2YZ
	inline void Vector3::GetVector2YZ(Vector2& outVec)	const
	{
		outVec.x=y;
		outVec.y=z;
	}
#endif // VECTOR3_GETVECTOR2YZ

#ifndef VECTOR3_GETVECTOR2ZY
#define VECTOR3_GETVECTOR2ZY
	inline void Vector3::GetVector2ZY(Vector2& outVec) const
	{
		outVec.x=z;
		outVec.y=y;
	}
#endif // VECTOR3_GETVECTOR2ZY

#ifndef VECTOR3_OPERATOR_BRACKET_C
#define VECTOR3_OPERATOR_BRACKET_C
	inline const float &Vector3::operator[](int i) const
	{
		return(((const float*)this)[i]); //lint !e740 unusual pointer cast
	}
#endif // VECTOR3_OPERATORBRACKETC

#ifndef VECTOR3_OPERATOR_BRACKET
#define VECTOR3_OPERATOR_BRACKET
	inline float &Vector3::operator[](int i)
	{
		return(((float*)this)[i]); //lint !e740 unusual pointer cast
	}	//lint !e1762 function could be made const (const already exists)
#endif // VECTOR3_OPERATORBRACKET

#ifndef VECTOR3_SETX
#define VECTOR3_SETX
	inline void Vector3::SetX(float f)
	{
		x=f;
	}
#endif // VECTOR3_SETX

#ifndef VECTOR3_SETY
#define VECTOR3_SETY
	inline void Vector3::SetY(float f)
	{
		y=f;
	}
#endif // VECTOR3_SETY

#ifndef VECTOR3_SETZ
#define VECTOR3_SETZ
	inline void Vector3::SetZ(float f)
	{
		z=f;
	}
#endif // VECTOR3_SETZ

#ifndef VECTOR3_GETUP
#define VECTOR3_GETUP
	inline float Vector3::GetUp()
	{
		return Dot(g_UnitUp);
	}
#endif	// VECTOR3_GETUP

#ifndef VECTOR3_GETUPV
#define VECTOR3_GETUPV
	inline Vector3 Vector3::GetUpV()
	{
		Vector3 upVector(g_UnitUp);
		upVector.Scale(GetUp());
		return upVector;
	}
#endif	// VECTOR3_GETUPV

#ifndef VECTOR3_GETHORIZONTALV
#define VECTOR3_GETHORIZONTALV
	inline Vector3 Vector3::GetHorizontalV()
	{
		Vector3 horizontal(*this);
		horizontal.SubtractScaled(g_UnitUp,GetUp());
		return horizontal;
	}
#endif	// VECTOR3_GETHORIZONTALV

#ifndef VECTOR3_GETHORIZONTALMAG2
#define VECTOR3_GETHORIZONTALMAG2
	inline float Vector3::GetHorizontalMag2()
	{
		return Mag2()-square(GetUp());
	}
#endif	// VECTOR3_GETHORIZONTALMAG2

#ifndef VECTOR3_GETHORIZONTALMAG
#define VECTOR3_GETHORIZONTALMAG
	inline float Vector3::GetHorizontalMag()
	{
		return SqrtfSafe(GetHorizontalMag2());
	}
#endif	// VECTOR3_GETHORIZONTALMAG

#ifndef VECTOR3_SPLATX
#define VECTOR3_SPLATX
	inline void Vector3::SplatX(Vector3Param in)
	{
		x = y = z = in.x;
	}
#endif // VECTOR3_SPLATX

#ifndef VECTOR3_SPLATY
#define VECTOR3_SPLATY
	inline void Vector3::SplatY(Vector3Param in)
	{
		x = y = z = in.y;
	}
#endif // VECTOR3_SPLATY

#ifndef VECTOR3_SPLATZ
#define VECTOR3_SPLATZ
	inline void Vector3::SplatZ(Vector3Param in)
	{
		x = y = z = in.z;
	}
#endif // VECTOR3_SPLATZ

#if VECTORIZED_PADDING
#ifndef VECTOR3_SPLATW
#define VECTOR3_SPLATW
	inline void Vector3::SplatW(Vector3Param in)
	{
		x = y = z = in.w;
	}
#endif // VECTOR3_SPLATW
#endif // VECTORIZED_PADDING

#ifndef VECTOR3_GETX
#define VECTOR3_GETX
	inline float Vector3::GetX() const
	{
		return x;
	}
#endif // VECTOR3_GETX

#ifndef VECTOR3_GETY
#define VECTOR3_GETY
	inline float Vector3::GetY() const
	{
		return y;
	}
#endif // VECTOR3_GETY

#ifndef VECTOR3_GETZ
#define VECTOR3_GETZ
	inline float Vector3::GetZ() const
	{
		return z;
	}
#endif // VECTOR3_GET

#if VECTORIZED_PADDING

#ifndef VECTOR3_GETW
#define VECTOR3_GETW
	inline float Vector3::GetW() const
	{
		return w;
	}
#endif // VECTOR3_GET

#ifndef VECTOR3_GETWASUNSIGNEDINT
#define VECTOR3_GETWASUNSIGNEDINT
	inline unsigned int Vector3::GetWAsUnsignedInt()  const
	{
		return uw;
	}
#endif // VECTOR3_GETWASUNSIGNEDINT

#ifndef VECTOR3_SETW
#define VECTOR3_SETW
	inline void Vector3::SetW(float f)
	{
		w=f;
	}
#endif // VECTOR3_SETZ

#ifndef VECTOR3_SETWASUNSIGNEDINT
#define VECTOR3_SETWASUNSIGNEDINT
	inline void Vector3::SetWAsUnsignedInt(unsigned int _w) 
	{
		uw = _w;
	}
#endif // VECTOR3_SETWASUNSIGNEDINT

#endif // VECTORIZED_PADDING

#ifndef VECTOR3_ADD_F3
#define VECTOR3_ADD_F3
	inline void Vector3::Add(float sx,float sy,float sz)
	{
		x+=sx; y+=sy; z+=sz;
	}
#endif

#ifndef VECTOR3_ADD_V
#define VECTOR3_ADD_V
	inline void Vector3::Add(Vector3Param a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
	}
#endif // VECTOR3_ADDV

#ifndef VECTOR3_ADD_V2
#define VECTOR3_ADD_V2
	inline void Vector3::Add(Vector3Param a, Vector3Param b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
	}
#endif // VECTOR3_ADDV2

#ifndef VECTOR3_ADDSCALED
#define VECTOR3_ADDSCALED
	inline void Vector3::AddScaled(Vector3Param a,float s)
	{
		x+=s*a.x;
		y+=s*a.y;
		z+=s*a.z;
	}
#endif // VECTOR3_ADDSCALED

#ifndef VECTOR3_ADDSCALED_V
#define VECTOR3_ADDSCALED_V
	inline void Vector3::AddScaled(Vector3Param a, Vector3Param s)
	{
		x+=s.x*a.x;
		y+=s.y*a.y;
		z+=s.z*a.z;
	}
#endif // VECTOR3_ADDSCALED

#ifndef VECTOR3_ADDSCALED_2
#define VECTOR3_ADDSCALED_2
	inline void Vector3::AddScaled(Vector3Param a, Vector3Param b,float s)
	{
		x=a.x+s*b.x; y=a.y+s*b.y; z=a.z+s*b.z;
	}
#endif // VECTOR3_ADDSCALED2

#ifndef VECTOR3_ADDSCALED_2V
#define VECTOR3_ADDSCALED_2V
	inline void Vector3::AddScaled(Vector3Param a, Vector3Param b, Vector3Param s)
	{
		x= a.x + (s.x * b.x); 
		y= a.y + (s.y * b.y); 
		z= a.z + (s.z * b.z);
	}
#endif // VECTOR3_ADDSCALED_2V

#ifndef VECTOR3_SUBTRACT_F3
#define VECTOR3_SUBTRACT_F3
	inline void Vector3::Subtract(float sx,float sy,float sz)
	{
		x -= sx;
		y -= sy;
		z -= sz;
	}
#endif // VECTOR3_SUBTRACTF3

#ifndef VECTOR3_SUBTRACT_V
#define VECTOR3_SUBTRACT_V
	inline void Vector3::Subtract(Vector3Param a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
	}
#endif // VECTOR3_SUBTRACTV

#ifndef VECTOR3_SUBTRACT_V2
#define VECTOR3_SUBTRACT_V2
	inline void Vector3::Subtract(Vector3Param a, Vector3Param b)
	{
		x=a.x-b.x; y=a.y-b.y; z=a.z-b.z;
	}
#endif // VECTOR3_SUBTRACTV2

#ifndef VECTOR3_SUBTRACTSCALED
#define VECTOR3_SUBTRACTSCALED
	inline void Vector3::SubtractScaled(Vector3Param a,float s)
	{
		x-=s*a.x; y-=s*a.y; z-=s*a.z;
	}
#endif // VECTOR3_SUBTRACTSCALED

#ifndef VECTOR3_SUBTRACTSCALEDV
#define VECTOR3_SUBTRACTSCALEDV
	inline void Vector3::SubtractScaled(Vector3Param a,Vector3Param s)
	{
		x-=s.x*a.x; y-=s.y*a.y; z-=s.z*a.z;
	}
#endif // VECTOR3_SUBTRACTSCALEDV

#ifndef VECTOR3_SUBTRACTSCALED_2
#define VECTOR3_SUBTRACTSCALED_2
	inline void Vector3::SubtractScaled(Vector3Param a, Vector3Param b,float s)
	{
		x=a.x-s*b.x; y=a.y-s*b.y; z=a.z-s*b.z;
	}
#endif // VECTOR3_SUBTRACTSCALED2

#ifndef VECTOR3_SUBTRACTSCALED_2V
#define VECTOR3_SUBTRACTSCALED_2V
	inline void Vector3::SubtractScaled(Vector3Param a, Vector3Param b,Vector3Param s)
	{
		x=a.x-s.x*b.x; y=a.y-s.y*b.y; z=a.z-s.z*b.z;
	}
#endif // VECTOR3_SUBTRACTSCALED2V

#ifndef VECTOR3_SCALE_F
#define VECTOR3_SCALE_F
	inline void Vector3::Scale(float f)
	{
		x*=f; y*=f; z*=f;
	}
#endif // VECTOR3_SCALEF

#ifndef VECTOR3_SCALE_VF
#define VECTOR3_SCALE_VF
	inline void Vector3::Scale(Vector3Param a,float f)
	{
		x=a.x*f; y=a.y*f; z=a.z*f;
	}
#endif // VECTOR3_SCALEVF

#ifndef VECTOR3_INVSCALE_F
#define VECTOR3_INVSCALE_F
	inline void Vector3::InvScale(float f)
	{
		float invF = 1.0f / f;
		x*=invF; y*=invF; z*=invF;
	}
#endif // VECTOR3_INVSCALEF

#ifndef VECTOR3_INVSCALE_FV
#define VECTOR3_INVSCALE_FV
	inline void Vector3::InvScale(Vector3Param f)
	{
		x /= f.x;
		y /= f.y;
		z /= f.z;
	}
#endif // VECTOR3_INVSCALEF

#ifndef VECTOR3_INVSCALE_VF
#define VECTOR3_INVSCALE_VF
	inline void Vector3::InvScale(Vector3Param a,float f)
	{
		float invF = 1.0f / f;
		x=a.x*invF; y=a.y*invF; z=a.z*invF;
	}
#endif // VECTOR3_INVSCALEVF

#ifndef VECTOR3_INVSCALE_VFV
#define VECTOR3_INVSCALE_VFV
	inline void Vector3::InvScale(Vector3Param a,Vector3Param f)
	{
		x = a.x / f.x;
		y = a.y / f.y;
		z = a.z / f.z;
	}
#endif // VECTOR3_INVSCALEVF

#ifndef VECTOR3_MUL_V
#define VECTOR3_MUL_V
	inline void Vector3::Multiply(Vector3Param a)
	{
		x*=a.x; y*=a.y; z*=a.z;
	}
#endif // VECTOR3_MULV

#ifndef VECTOR3_MUL_V2
#define VECTOR3_MUL_V2
	inline void Vector3::Multiply(Vector3Param a, Vector3Param b)
	{
		x=a.x*b.x; y=a.y*b.y; z=a.z*b.z;
	}
#endif // VECTOR3_MULV2

#ifndef VECTOR3_NEGATE
#define VECTOR3_NEGATE
	inline void Vector3::Negate()
	{
		x=-x; y=-y; z=-z;
	}
#endif // VECTOR3_NEGATE

#ifndef VECTOR3_NEGATE_V
#define VECTOR3_NEGATE_V
	inline void Vector3::Negate(Vector3Param a)
	{
		x=-a.x; y=-a.y; z=-a.z;
	}
#endif // VECTOR3_NEGATEV

#ifndef VECTOR3_INVMAG
#define VECTOR3_INVMAG
	inline float Vector3::InvMag() const
	{
		return invsqrtf(__dot3(x,x,y,y,z,z));
	}
#endif // VECTOR3_INVMAG

#ifndef VECTOR3_INVMAGV
#define VECTOR3_INVMAGV
	inline Vector3 Vector3::InvMagV() const
	{
		float mag = invsqrtf(__dot3(x,x,y,y,z,z));
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_INVMAG

#ifndef VECTOR3_INVMAG_FAST
#define VECTOR3_INVMAG_FAST
	inline float Vector3::InvMagFast() const
	{
		return invsqrtf_fast(__dot3(x,x,y,y,z,z));
	}
#endif // VECTOR3_INVMAG

#ifndef VECTOR3_INVMAG_FASTV
#define VECTOR3_INVMAG_FASTV
	inline Vector3 Vector3::InvMagFastV() const
	{
		float mag = invsqrtf_fast(__dot3(x,x,y,y,z,z));
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_INVMAG

#ifndef VECTOR3_ABS
#define VECTOR3_ABS
	inline void Vector3::Abs()
	{
		x=fabsf(x); y=fabsf(y); z=fabsf(z);
	}
#endif // VECTOR3_ABS

#ifndef VECTOR3_ABS_V
#define VECTOR3_ABS_V
	inline void Vector3::Abs(const Vector3& a)
	{
		x=fabsf(a.x); y=fabsf(a.y); z=fabsf(a.z);
	}
#endif // VECTOR3_ABSV

#ifndef VECTOR3_INVERT
#define VECTOR3_INVERT
	inline void Vector3::Invert()
	{
		x = 1.0f / x; y = 1.0f / y; z = 1.0f / z;
	}
#endif // VECTOR3_INVERT

#ifndef VECTOR3_INVERT_V
#define VECTOR3_INVERT_V
	inline void Vector3::Invert(Vector3Param a)
	{
		x=1.0f/a.x; y=1.0f/a.y; z=1.0f/a.z;
	}
#endif // VECTOR3_INVERTV

#ifndef VECTOR3_INVERTSAFE
#define VECTOR3_INVERTSAFE
	inline void Vector3::InvertSafe()
	{
		x=(x!=0.0f?1.0f/x:FLT_MAX);y=(y!=0.0f?1.0f/y:FLT_MAX);z=(z!=0.0f?1.0f/z:FLT_MAX);
	}
#endif // VECTOR3_INVERTSAFE

#ifndef VECTOR3_INVERTSAFE_V
#define VECTOR3_INVERTSAFE_V
	inline void Vector3::InvertSafe(Vector3Param a, Vector3Param zeroInverse)
	{
		x=(a.x!=0.0f?1.0f/a.x:zeroInverse.x);y=(a.y!=0.0f?1.0f/a.y:zeroInverse.y);z=(a.z!=0.0f?1.0f/a.z:zeroInverse.z);
	}
#endif // VECTOR3_IVNERTSAFEV

#ifndef VECTOR3_NORMALIZE
#define VECTOR3_NORMALIZE
	inline void Vector3::Normalize()
	{
		Scale(InvMag());
	}
#endif // VECTOR3_NORMALIZE

#ifndef VECTOR3_NORMALIZE_FAST
#define VECTOR3_NORMALIZE_FAST
	inline void Vector3::NormalizeFast()
	{
		Scale(InvMagFast());
	}
#endif // VECTOR3_NORMALIZE_FAST

#ifndef VECTOR3_NORMALIZESAFE1
#define VECTOR3_NORMALIZESAFE1
	inline void Vector3::NormalizeSafe()
	{
		Vector3 magSq = Mag2V();

		Vector3 selector = magSq.IsGreaterThanV(VEC3_SMALL_FLOAT);

		Vector3 r;
		r.Multiply( *this, magSq.RecipSqrtV() );

		Set( selector.Select( XAXIS, r ) );
	}
#endif // VECTOR3_NORMALIZESAFE1

#ifndef VECTOR3_NORMALIZESAFE2
#define VECTOR3_NORMALIZESAFE2
	inline void Vector3::NormalizeSafe(Vector3Param fallbackSafeVec)
	{
		Vector3 magSq = Mag2V();

		Vector3 selector = magSq.IsGreaterThanV(VEC3_SMALL_FLOAT);

		Vector3 r;
		r.Multiply( *this, magSq.RecipSqrtV() );

		Set( selector.Select( fallbackSafeVec, r ) );
	}
#endif // VECTOR3_NORMALIZESAFE2

#ifndef VECTOR3_NORMALIZESAFE3
#define VECTOR3_NORMALIZESAFE3
	inline void Vector3::NormalizeSafe(Vector3Param fallbackSafeVec, float mag2Limit)
	{
		f32 magSq = Mag2();
		if (magSq>mag2Limit)
		{
			Scale(invsqrtf(magSq));
		}
		else
		{
			Set(fallbackSafeVec);
		}
	}
#endif // VECTOR3_NORMALIZESAFE3

#ifndef VECTOR3_NORMALIZESAFERET
#define VECTOR3_NORMALIZESAFERET
	inline bool Vector3::NormalizeSafeRet()
	{
		Vector3 magSq = Mag2V();

		Vector3 selector = magSq.IsGreaterThanV(VEC3_SMALL_FLOAT);

		Vector3 r;
		r.Multiply( *this, magSq.RecipSqrtV() );

		Set( selector.Select( YAXIS, r ) );
		return selector.IsTrueTrueTrue();
	}
#endif // VECTOR3_NORMALIZESAFE1

#ifndef VECTOR3_NORMALIZESAFEV
#define VECTOR3_NORMALIZESAFEV
	VEC3_INLINE void Vector3::NormalizeSafeV(Vector3Param fallbackSafeVec, Vector3Param mag2Limit)
	{
		Vector3 magSq = Mag2V();

		Vector3 selector = magSq.IsGreaterThanV(mag2Limit);

		Vector3 r;
		r.Multiply( *this, magSq.RecipSqrtV() );

		Set( selector.Select( fallbackSafeVec, r ) );
	}
#endif // VECTOR3_NORMALIZESAFE


#ifndef VECTOR3_NORMALIZE_V
#define VECTOR3_NORMALIZE_V
	inline void Vector3::Normalize(Vector3Param a)
	{
		Scale(a,a.InvMag());
	}
#endif // VECTOR3_NORMALIZEV

#ifndef VECTOR3_NORMALIZE_FAST_V
#define VECTOR3_NORMALIZE_FAST_V
	inline void Vector3::NormalizeFast(Vector3Param a)
	{
		Scale(a,a.InvMagFast());
	}
#endif // VECTOR3_NORMALIZEV

#ifndef VECTOR3_CROSS
#define VECTOR3_CROSS
	inline void Vector3::Cross(Vector3Param a)
	{
		register float newX=y*a.z-z*a.y;
		register float newY=z*a.x-x*a.z;
		z=x*a.y-y*a.x;
		x=newX;
		y=newY;
	}
#endif // VECTOR3_CROSS


#ifndef VECTOR3_CROSS_2
#define VECTOR3_CROSS_2
	inline void Vector3::Cross(Vector3Param a, Vector3Param b)
	{
		FastAssert(this!=&a && this!=&b && "Don't use this Cross function with this as an argument.");	//lint !e506 constant value boolean
		x=a.y*b.z-a.z*b.y;
		y=a.z*b.x-a.x*b.z;
		z=a.x*b.y-a.y*b.x;
	}
#endif // VECTOR3_CROSS2

#ifndef VECTOR3_CROSSSAFE
#define VECTOR3_CROSSSAFE
	inline void Vector3::CrossSafe(Vector3Param a, Vector3Param b)
	{
		register float newX, newY, newZ;
		newX=a.y*b.z-a.z*b.y;
		newY=a.z*b.x-a.x*b.z;
		newZ=a.x*b.y-a.y*b.x;

		x=newX;
		y=newY;
		z=newZ;
	}
#endif // VECTOR3_CROSSAFE

#ifndef VECTOR3_CROSSNEGATE
#define VECTOR3_CROSSNEGATE
	inline void Vector3::CrossNegate(Vector3Param a)
	{
		register float newX, newY, newZ;

		newX=-y*a.z+z*a.y;
		newY=-z*a.x+x*a.z;
		newZ=-x*a.y+y*a.x;

		x=newX;
		y=newY;
		z=newZ;
	}
#endif // VECTOR3_CROSSNEGATE

#ifndef VECTOR3_CROSSX
#define VECTOR3_CROSSX
	inline float Vector3::CrossX(const Vector3& a) const
	{
		return y*a.z-z*a.y;
	}
#endif // VECTOR3_CROSSX

#ifndef VECTOR3_CROSSY
#define VECTOR3_CROSSY
	inline float Vector3::CrossY(const Vector3& a) const
	{
		return z*a.x-x*a.z;
	}
#endif // VECTOR3_CROSSY

#ifndef VECTOR3_CROSSZ
#define VECTOR3_CROSSZ
	inline float Vector3::CrossZ(const Vector3& a) const
	{
		return x*a.y-y*a.x;
	}
#endif // VECTOR3_CROSSZ

#ifndef VECTOR3_DOT
#define VECTOR3_DOT
	inline float Vector3::Dot(const Vector3& a) const
	{
		return __dot3(x,a.x,y,a.y,z,a.z);
	}
#endif // VECTOR3_DOT

#ifndef VECTOR3_DOTV
#define VECTOR3_DOTV
	inline Vector3 Vector3::DotV(Vector3Param a) const
	{
		float fDot = __dot3(x,a.x,y,a.y,z,a.z);
		return Vector3(fDot, fDot, fDot);
	}
#endif // VECTOR3_DOTV

#ifndef VECTOR3_DOTV2
#define VECTOR3_DOTV2
	inline void Vector3::DotV(Vector3Param a,Vector3Param b)
	{
		float fDot = __dot3(b.x,a.x,b.y,a.y,b.z,a.z);
		x = y = z = fDot;
	}
#endif // VECTOR3_DOTV2

#ifndef VECTOR3_FLATDOT
#define VECTOR3_FLATDOT
	inline float Vector3::XZDot(const Vector3& a) const
	{
		return x*a.x+z*a.z;
	}
#endif

#ifndef VECTOR3_ADDCROSSED
#define VECTOR3_ADDCROSSED
	inline void Vector3::AddCrossed(Vector3Param a, Vector3Param b)
	{
#if __XENON && VECTORIZED
		FastAssert(&xyzw != &a && &xyzw != &b && "Don't use AddCrossed with this as an argument.");	//lint !e506 constant value bollean
#else
		FastAssert(this!=&a && this!=&b && "Don't use AddCrossed with this as an argument.");	//lint !e506 constant value bollean
#endif
		x+=a.y*b.z-a.z*b.y;	y+=a.z*b.x-a.x*b.z;	z+=a.x*b.y-a.y*b.x;
	}
#endif // VECTOR3_ADDCROSSED

#ifndef VECTOR3_SUBTRACTCROSSED
#define VECTOR3_SUBTRACTCROSSED
	inline void Vector3::SubtractCrossed(Vector3Param a, Vector3Param b)
	{
#if __XENON && VECTORIZED
		FastAssert(&xyzw != &a && &xyzw != &b && "Don't use SubtractCrossed with this as an argument.");	//lint !e506 constant value bollean
#else
		FastAssert(this!=&a && this!=&b && "Don't use SubtractCrossed with this as an argument.");	//lint !e506 constant value bollean
#endif
		x-=a.y*b.z-a.z*b.y;	y-=a.z*b.x-a.x*b.z;	z-=a.x*b.y-a.y*b.x;
	}
#endif // VECTOR3_SUBTRACTCROSSED

#ifndef VECTOR3_AVERAGE
#define VECTOR3_AVERAGE
	inline void Vector3::Average(Vector3Param a)
	{
		x=0.5f*(x+a.x); y=0.5f*(y+a.y); z=0.5f*(z+a.z);
	}
#endif // VECTOR3_AVERAGE

#ifndef VECTOR3_AVERAGE_2
#define VECTOR3_AVERAGE_2
	inline void Vector3::Average(Vector3Param a, Vector3Param b)
	{
		x=0.5f*(a.x+b.x); y=0.5f*(a.y+b.y); z=0.5f*(a.z+b.z);
	}
#endif // VECTOR3_AVERAGE2

#ifndef VECTOR3_LERP
#define VECTOR3_LERP
	inline void Vector3::Lerp(float t,Vector3Param a)
	{
		Lerp(t, *this, a);
	}
#endif // VECTOR3_LERP

#ifndef VECTOR3_LERPV
#define VECTOR3_LERPV
	inline void Vector3::Lerp(Vector3Param t,Vector3Param a)
	{
		x = x + (a.x - x) * t.x;
		y = y + (a.y - y) * t.y;
		z = z + (a.z - z) * t.z;
	}
#endif // VECTOR3_LERP

#ifndef VECTOR3_LERP_2
#define VECTOR3_LERP_2
	inline void Vector3::Lerp(float t,Vector3Param a,Vector3Param b)
	{
		x=a.x+(b.x-a.x)*t;
		y=a.y+(b.y-a.y)*t;
		z=a.z+(b.z-a.z)*t;
	}
#endif // VECTOR3_LERP_2

#ifndef VECTOR3_LERP_2V
#define VECTOR3_LERP_2V
	inline void Vector3::Lerp(Vector3Param t,Vector3Param a,Vector3Param b)
	{
		x=a.x+(b.x-a.x)*t.x;
		y=a.y+(b.y-a.y)*t.y;
		z=a.z+(b.z-a.z)*t.z;
	}
#endif // VECTOR3_LERP_2V

#ifndef VECTOR3_SQRTV
#define VECTOR3_SQRTV
	inline Vector3 Vector3::SqrtV() const
	{
		return Vector3(sqrtf(x), sqrtf(y), sqrtf(z));
	}
#endif

#ifndef VECTOR3_RECIPSQRTV
#define VECTOR3_RECIPSQRTV
    inline Vector3 Vector3::RecipSqrtV() const
    {
        return Vector3(1.0f / sqrtf(x), 1.0f / sqrtf(y), 1.0f / sqrtf(z));
    }
#endif

#ifndef VECTOR3_MAG
#define VECTOR3_MAG
	inline float Vector3::Mag() const
	{
		return sqrtf(__dot3(x,x,y,y,z,z));
	}
#endif // VECTOR3_MAG

#ifndef VECTOR3_MAGV
#define VECTOR3_MAGV
	inline Vector3 Vector3::MagV() const
	{
		float mag = sqrtf(__dot3(x,x,y,y,z,z));
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_MAGV

#ifndef VECTOR3_MAGFASTV
#define VECTOR3_MAGFASTV
	inline Vector3 Vector3::MagFastV() const
	{
		float mag = sqrtf(__dot3(x,x,y,y,z,z));
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_MAGFASTV

#ifndef VECTOR3_MAG2
#define VECTOR3_MAG2
	inline float Vector3::Mag2() const
	{
		return __dot3(x,x,y,y,z,z);
	}
#endif // VECTOR3_MAG2

#ifndef VECTOR3_MAG2V
#define VECTOR3_MAG2V
	inline Vector3 Vector3::Mag2V() const
	{
		float mag = __dot3(x,x,y,y,z,z);
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_MAG2

#ifndef VECTOR3_FLATMAG
#define VECTOR3_FLATMAG
	inline float Vector3::XZMag() const
	{
		return sqrtf(x*x+z*z);
	}
#endif // VECTOR3_FLATMAG

#ifndef VECTOR3_FLATMAGV
#define VECTOR3_FLATMAGV
	inline Vector3 Vector3::XZMagV() const
	{
		float mag = sqrtf(x*x+z*z);
		return Vector3(mag, mag, mag);
	}
#endif // VECTOR3_FLATMAGV

#ifndef VECTOR3_FLATMAG2
#define VECTOR3_FLATMAG2
	inline float Vector3::XZMag2() const
	{
		return x*x+z*z;
	}			
#endif // VECTOR3_FLATMAG2

#ifndef VECTOR3_FLATMAG2V
#define VECTOR3_FLATMAG2V
	inline Vector3 Vector3::XZMag2V() const
	{
		float mag = x*x+z*z;
		return Vector3(mag, mag, mag);
	}			
#endif // VECTOR3_FLATMAG2V

#ifndef VECTOR3_XYMAG
#define VECTOR3_XYMAG
	inline float Vector3::XYMag() const
	{
		return sqrtf(x*x+y*y);
	}
#endif // VECTOR3_XYMAG

#ifndef VECTOR3_XYMAG2
#define VECTOR3_XYMAG2
	inline float Vector3::XYMag2() const
	{
		return x*x+y*y;
	}			
#endif // VECTOR3_XYMAG2

#ifndef VECTOR3_XYMAGV
#define VECTOR3_XYMAGV
	inline Vector3 Vector3::XYMagV() const
	{
		float mag = XYMag();
		return Vector3(mag,mag,mag);
	}
#endif // VECTOR3_XYMAG

#ifndef VECTOR3_XYMAG2V
#define VECTOR3_XYMAG2V
	inline Vector3 Vector3::XYMag2V() const
	{
		float mag = XYMag2();
		return Vector3(mag,mag,mag);
	}			
#endif // VECTOR3_XYMAG2

#ifndef VECTOR3_DIST
#define VECTOR3_DIST
	inline float Vector3::Dist(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float ry=y-a.y;
		register float rz=z-a.z;
		return sqrtf(rx*rx+ry*ry+rz*rz);
	}
#endif // VECTOR3_DIST

#ifndef VECTOR3_DISTV
#define VECTOR3_DISTV
	inline Vector3 Vector3::DistV(Vector3Param a) const
	{
		return (*this - a).MagV();
	}
#endif // VECTOR3_DIST

#ifndef VECTOR3_INVDIST
#define VECTOR3_INVDIST
	inline float Vector3::InvDist(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float ry=y-a.y;
		register float rz=z-a.z;
		return invsqrtf(rx*rx+ry*ry+rz*rz);
	}
#endif // VECTOR3_INVDIST

#ifndef VECTOR3_INVDISTV
#define VECTOR3_INVDISTV
	inline Vector3 Vector3::InvDistV(Vector3Param a) const
	{
		return (*this - a).InvMagV();
	}
#endif // VECTOR3_INVDIST

#ifndef VECTOR3_DIST2
#define VECTOR3_DIST2
	inline float Vector3::Dist2(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float ry=y-a.y;
		register float rz=z-a.z;
		return rx*rx+ry*ry+rz*rz;
	}
#endif // VECTOR3_DIST2

#ifndef VECTOR3_DIST2V
#define VECTOR3_DIST2V
	inline Vector3 Vector3::Dist2V(Vector3Param a) const
	{
		Vector3 temp = *this;
		temp.Subtract(a);
		return temp.DotV(temp);
	}
#endif // VECTOR3_DIST2

#ifndef VECTOR3_INVDIST2
#define VECTOR3_INVDIST2
	inline float Vector3::InvDist2(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float ry=y-a.y;
		register float rz=z-a.z;
		return 1.0f/(rx*rx+ry*ry+rz*rz);
	}
#endif // VECTOR3_INVDIST2

#ifndef VECTOR3_INVDIST2V
#define VECTOR3_INVDIST2V
	inline Vector3 Vector3::InvDist2V(Vector3Param a) const
	{
		register float rx=x-a.x;
		register float ry=y-a.y;
		register float rz=z-a.z;
		float dist = 1.0f/(rx*rx+ry*ry+rz*rz);
		return Vector3(dist, dist, dist);
	}
#endif // VECTOR3_INVDIST2

#ifndef VECTOR3_FLATDIST
#define VECTOR3_FLATDIST
	inline float Vector3::XZDist(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float rz=z-a.z;
		return sqrtf(rx*rx+rz*rz);
	}
#endif // VECTOR3_FLATDIST


#ifndef VECTOR3_FLATDISTV
#define VECTOR3_FLATDISTV
	inline Vector3 Vector3::XZDistV(Vector3Param a) const
	{
		register float rx=x-a.x;
		register float rz=z-a.z;
		register float r = sqrtf(rx*rx+rz*rz);
		Vector3 rv;
		rv.Set(r);
		return rv;
	}
#endif // VECTOR3_FLATDISTV

#ifndef VECTOR3_FLATDIST2
#define VECTOR3_FLATDIST2
	inline float Vector3::XZDist2(const Vector3& a) const
	{
		register float rx=x-a.x;
		register float rz=z-a.z;
		return rx*rx+rz*rz;
	}
#endif // VECTOR3_FLATDIST2

#ifndef VECTOR3_FLATDIST2V
#define VECTOR3_FLATDIST2V
	inline Vector3 Vector3::XZDist2V(Vector3Param a) const
	{
		register float rx=x-a.x;
		register float rz=z-a.z;
		Vector3 rv;
		rv.Set( rx*rx+rz*rz );
		return rv;
	}
#endif // VECTOR3_FLATDIST2

#ifndef VECTOR3_CLAMPMAG
#define VECTOR3_CLAMPMAG
	inline void Vector3::ClampMag (float minMag, float maxMag)
	{
		float mag2 = Mag2();
		if (mag2>square(maxMag))
		{
			// The vector's magnitude is larger than maxMag, so scale it down.
			Scale(maxMag*invsqrtf(mag2));
		}
		else if (mag2<square(minMag))
		{
			// The vector's magnitude is smaller than minMag, so scale it up.
			if (mag2>1.0e-12f)
			{
				// The vector's magnitude is large enough to tell what its direction is and to scale it up.
				Scale(minMag*invsqrtf(mag2));
			}
			else
			{
				// The vector is nearly zero, so make it point up with the minimum magnitude.
#if __SPU
				Scale(YAXIS,minMag);
#else
				Scale(g_UnitUp,minMag);
#endif
			}
		}
		else if (!FPIsFinite(mag2))
		{
			mthAssertf( false, "Warning: input to ClampMag() was an invalid floating point value." );

			// The vector's magnitude is not a number, so make it point up with the minimum magnitude.
#if __SPU
			Scale(YAXIS,minMag);
#else
			Scale(g_UnitUp,minMag);
#endif
		}
	}
#endif // VECTOR3_CLAMPMAG

#ifndef VECTOR3_ISZERO
#define VECTOR3_ISZERO
	inline bool Vector3::IsZero() const
	{
		if(x!=0.0f) return false;
		else if(y!=0.0f) return false;
		else if(z==0.0f) return true;
		else return false;
	}
#endif // VECTOR3_ISZERO

#ifndef VECTOR3_ISZEROV
#define VECTOR3_ISZEROV
	inline Vector3 Vector3::IsZeroV() const
	{
		Vector3 ret;
		ret.x = (x == 0 ? allBitsF : 0);
		ret.y = (y == 0 ? allBitsF : 0);
		ret.z = (z == 0 ? allBitsF : 0);
		return ret;
	}
#endif // VECTOR3_ISZEROV

#ifndef VECTOR3_ISZEROV4
#define VECTOR3_ISZEROV4
	inline Vector3 Vector3::IsZeroV4() const
	{
		Vector3 isZero;
		isZero.x = (x == 0 ? allBitsF : 0);
		isZero.y = (y == 0 ? allBitsF : 0);
		isZero.z = (z == 0 ? allBitsF : 0);
#if VECTORIZED_PADDING
		isZero.w = (w == 0 ? allBitsF : 0);
#endif
		return isZero;
	}
#endif // VECTOR3_ISZEROV4

#ifndef VECTOR3_ISNONZERO
#define VECTOR3_ISNONZERO
	inline bool Vector3::IsNonZero() const
	{
		if(x!=0.0f) return true;
		else if(y!=0.0f) return true;
		else if(z==0.0f) return false;
		else return true;
	}
#endif // VECTOR3_ISNONZERO

#ifndef VECTOR3_FINITEELEMENTS
#define VECTOR3_FINITEELEMENTS
	inline bool Vector3::FiniteElements() const
	{
		return (FPIsFinite(x) && FPIsFinite(y) && FPIsFinite(z));
	}
#endif // VECTOR3_FINITEELEMENTS

#ifndef VECTOR3_FINITEELEMENTSV4
#define VECTOR3_FINITEELEMENTSV4
	inline Vector3 Vector3::FiniteElementsV4() const
	{
		Vector3 finiteElements;
		finiteElements.x = (FPIsFinite(x) ? allBitsF : 0);
		finiteElements.y = (FPIsFinite(y) ? allBitsF : 0);
		finiteElements.z = (FPIsFinite(z) ? allBitsF : 0);
#if VECTORIZED_PADDING
		finiteElements.w = (FPIsFinite(w) ? allBitsF : 0);
#endif
		return finiteElements;
	}
#endif // VECTOR3_FINITEELEMENTSV4

#ifndef VECTOR3_ISEQUAL
#define VECTOR3_ISEQUAL
	inline bool Vector3::IsEqual(Vector3Param a) const
	{
		return (x==a.x) && (y==a.y) && (z==a.z);
	}
#endif // VECTOR3_ISEQUAL

#ifndef VECTOR3_ISEQUALV
#define VECTOR3_ISEQUALV
	inline Vector3 Vector3::IsEqualV(Vector3Param a) const
	{
		Vector3 ret;
		ret.x = (x == a.x ? allBitsF : 0);
		ret.y = (y == a.y ? allBitsF : 0);
		ret.z = (z == a.z ? allBitsF : 0);
		return ret;
	}
#endif // VECTOR3_ISEQUALV

#ifndef VECTOR3_ISNOTEQUAL
#define VECTOR3_ISNOTEQUAL
	inline bool Vector3::IsNotEqual(Vector3Param a) const
	{
		return (x!=a.x) || (y!=a.y) || (z!=a.z);
	}
#endif // VECTOR3_ISNOTEQUAL

#ifndef VECTOR3_ISCLOSE
#define VECTOR3_ISCLOSE
	inline bool Vector3::IsClose(Vector3Param a,float eps) const
	{
		return (x>=a.x-eps && x<=a.x+eps) && (y>=a.y-eps && y<=a.y+eps) && (z>=a.z-eps && z<=a.z+eps);
	}
#endif // VECTOR3_ISCLOSE

#ifndef VECTOR3_ISCLOSEV
#define VECTOR3_ISCLOSEV
	inline bool Vector3::IsClose(Vector3Param a,Vector3Param eps) const
	{
		return (x>=a.x-eps.x && x<=a.x+eps.x) && (y>=a.y-eps.y && y<=a.y+eps.y) && (z>=a.z-eps.z && z<=a.z+eps.z);
	}
#endif // VECTOR3_ISCLOSEV

#ifndef VECTOR3_ISGREATERTHAN
#define VECTOR3_ISGREATERTHAN
	inline bool Vector3::IsGreaterThan(Vector3Param a) const
	{
		return (x > a.x) && (y > a.y) && ( z > a.z);
	}
#endif // VECTOR3_ISGREATERTHAN

#ifndef VECTOR3_ISGREATERTHANV
#define VECTOR3_ISGREATERTHANV
	inline Vector3 Vector3::IsGreaterThanV(Vector3Param a) const
	{
		return Vector3((x > a.x) ? allBitsF : 0.0f, (y > a.y) ? allBitsF : 0.0f, (z > a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISGREATERTHANV

#ifndef VECTOR3_ISGREATERTHANV4
#define VECTOR3_ISGREATERTHANV4
	inline Vector3 Vector3::IsGreaterThanV4(Vector3Param a) const
	{
		return Vector3((x > a.x) ? allBitsF : 0.0f, (y > a.y) ? allBitsF : 0.0f, (z > a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISGREATERTHANV

#ifndef VECTOR3_ISGREATERTHANVR
#define VECTOR3_ISGREATERTHANVR
	inline Vector3 Vector3::IsGreaterThanVR(const Vector3& a, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		Vector3 ret(0.0f, 0.0f, 0.0f);
		if( x > a.x )
		{
			ret.x = allBitsF;
			bx = true;
		}
		if( y > a.y )
		{
			ret.y = allBitsF;
			by = true;
		}
		if( z > a.z )
		{
			ret.z = allBitsF;
			bz = true;
		}
		r = 0;
		if( bx && by && bz )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR3_ISGREATERTHANVR

#ifndef VECTOR3_ISGREATEROREQUALTHAN
#define VECTOR3_ISGREATEROREQUALTHAN
	inline bool Vector3::IsGreaterOrEqualThan(Vector3Param a) const
	{
		return (x >= a.x) && (y >= a.y) && ( z >= a.z);
	}
#endif // VECTOR3_ISGREATEROREQUALTHAN

#ifndef VECTOR3_ISGREATEROREQUALTHANV
#define VECTOR3_ISGREATEROREQUALTHANV
	inline Vector3 Vector3::IsGreaterOrEqualThanV(Vector3Param a) const
	{
		return Vector3((x >= a.x) ? allBitsF : 0.0f, (y >= a.y) ? allBitsF : 0.0f, (z >= a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISGREATEROREQUALTHANV

#ifndef VECTOR3_ISGREATEROREQUALTHANVR
#define VECTOR3_ISGREATEROREQUALTHANVR
	inline Vector3 Vector3::IsGreaterOrEqualThanVR(Vector3Param va, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		Vector3 ret(0.0f, 0.0f, 0.0f);
		Vector3 a(va);
		if( x >= a.x )
		{
			ret.x = allBitsF;
			bx = true;
		}
		if( y >= a.y )
		{
			ret.y = allBitsF;
			by = true;
		}
		if( z >= a.z )
		{
			ret.z = allBitsF;
			bz = true;
		}
		r = 0;
		if( bx && by && bz )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR3_ISGREATEROREQUALTHANVR



#ifndef VECTOR3_ISLESSTHAN
#define VECTOR3_ISLESSTHAN
	inline bool Vector3::IsLessThanAll(Vector3Param a) const
	{
		return (x < a.x) && (y < a.y) && ( z < a.z);
	}

	inline bool Vector3::IsLessThanDoNotUse(Vector3Param a) const
	{
		return (x < a.x) && (y < a.y) && ( z < a.z);
	}
#endif // VECTOR3_ISLESSTHAN

#ifndef VECTOR3_ISLESSTHANV
#define VECTOR3_ISLESSTHANV
	inline Vector3 Vector3::IsLessThanV(Vector3Param a) const
	{
		return Vector3((x < a.x) ? allBitsF : 0.0f, (y < a.y) ? allBitsF : 0.0f, (z < a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISLESSTHANV

#ifndef VECTOR3_ISLESSTHANV4
#define VECTOR3_ISLESSTHANV4
	inline Vector3 Vector3::IsLessThanV4(Vector3Param a) const
	{
		return Vector3((x < a.x) ? allBitsF : 0.0f, (y < a.y) ? allBitsF : 0.0f, (z < a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISLESSTHANV4

#ifndef VECTOR3_ISLESSTHANVR
#define VECTOR3_ISLESSTHANVR
	inline Vector3 Vector3::IsLessThanVR(const Vector3& a, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		Vector3 ret(0.0f, 0.0f, 0.0f);
		if( x < a.x )
		{
			ret.x = allBitsF;
			bx = true;
		}
		if( y < a.y )
		{
			ret.y = allBitsF;
			by = true;
		}
		if( z < a.z )
		{
			ret.z = allBitsF;
			bz = true;
		}
		r = 0;
		if( bx && by && bz )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR3_ISLESSTHANVR

#ifndef VECTOR3_ISLESSTHANVR4
#define VECTOR3_ISLESSTHANVR4
	inline Vector3 Vector3::IsLessThanVR4(const Vector3& a, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		Vector3 ret(0.0f, 0.0f, 0.0f);
		if( x < a.x )
		{
			ret.x = allBitsF;
			bx = true;
		}
		if( y < a.y )
		{
			ret.y = allBitsF;
			by = true;
		}
		if( z < a.z )
		{
			ret.z = allBitsF;
			bz = true;
		}
		r = 0;
		if( bx && by && bz )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR3_ISLESSTHANVR4

#ifndef VECTOR3_ISLESSOREQUALTHAN
#define VECTOR3_ISLESSOREQUALTHAN
	inline bool Vector3::IsLessOrEqualThanAll(Vector3Param a) const
	{
		return (x <= a.x) && (y <= a.y) && ( z <= a.z);
	}

	inline bool Vector3::IsLessOrEqualThanDoNotUse(Vector3Param a) const
	{
		return (x <= a.x) && (y <= a.y) && ( z <= a.z);
	}
#endif // VECTOR3_ISLESSTHAN

#ifndef VECTOR3_ISLESSOREQUALTHANV
#define VECTOR3_ISLESSOREQUALTHANV
	inline Vector3 Vector3::IsLessOrEqualThanV(Vector3Param a) const
	{
		return Vector3((x <= a.x) ? allBitsF : 0.0f, (y <= a.y) ? allBitsF : 0.0f, (z <= a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISLESSTHANV

#ifndef VECTOR3_ISLESSOREQUALTHANV4
#define VECTOR3_ISLESSOREQUALTHANV4
	inline Vector3 Vector3::IsLessOrEqualThanV4(Vector3Param a) const
	{
		return Vector3((x <= a.x) ? allBitsF : 0.0f, (y <= a.y) ? allBitsF : 0.0f, (z <= a.z) ? allBitsF : 0.0f);
	}
#endif // VECTOR3_ISLESSTHANV4



#ifndef VECTOR3_ISTRUETRUETRUE
#define VECTOR3_ISTRUETRUETRUE
    inline bool Vector3::IsTrueTrueTrue() const
    {
        return *(u32*)(&x) == 0xffffffff && *(u32*)(&y) == 0xffffffff && *(u32*)(&z) == 0xffffffff;
    }
#endif // VECTOR3_ISTRUETRUETRUE

#ifndef VECTOR3_ISFALSEFALSEFALSE
#define VECTOR3_ISFALSEFALSEFALSE
	inline bool Vector3::IsFalseFalseFalse() const
	{
		return *(u32*)(&x) == 0x00000000 && *(u32*)(&y) == 0x00000000 && *(u32*)(&z) == 0x00000000;
	}
#endif // VECTOR3_ISFALSEFALSEFALSE

#ifndef VECTOR3_SELECT
#define VECTOR3_SELECT
	inline Vector3 Vector3::Select(Vector3Param zero, Vector3Param nonZero) const
	{
		Vector3 r;
		if( x != 0.0f )
		{
			r.x = nonZero.x;
		}
		else
		{
			r.x = zero.x;
		}


		if( y != 0.0f )
		{
			r.y = nonZero.y;
		}
		else
		{
			r.y = zero.y;
		}

		if( z != 0.0f )
		{
			r.z = nonZero.z;
		}
		else
		{
			r.z = zero.z;
		}

		return r;

	}
#endif

#ifndef VECTOR3_MAX
#define VECTOR3_MAX
	inline void Vector3::Max(Vector3Param a, Vector3Param b)
	{
		x = a.x > b.x ? a.x : b.x;
		y = a.y > b.y ? a.y : b.y;
		z = a.z > b.z ? a.z : b.z;
	}
#endif // VECTOR3_MAX

#ifndef VECTOR3_MIN
#define VECTOR3_MIN
	inline void Vector3::Min(Vector3Param a, Vector3Param b)
	{
		x = a.x < b.x ? a.x : b.x;
		y = a.y < b.y ? a.y : b.y;
		z = a.z < b.z ? a.z : b.z;
	}
#endif // VECTOR3_MIN

#ifndef VECTOR3_AND
#define VECTOR3_AND
	inline void Vector3::And(Vector3Param _and)
	{
		*this &= _and;
	}
#endif	// VECTOR3_AND

#ifndef VECTOR3_OR
#define VECTOR3_OR
	inline void Vector3::Or(Vector3Param _or)
	{
		*this |= _or;
	}
#endif // VECTOR3_OR

#ifndef VECTOR3_XOR
#define VECTOR3_XOR
	inline void Vector3::Xor(Vector3Param _xor)
	{
		*this ^= _xor;
	}
#endif // VECTOR3_XOR

#ifndef VECTOR3_MERGEXY
#define VECTOR3_MERGEXY
	inline void Vector3::MergeXY(Vector3Param vY)
	{
		z = y;
		y = vY.x;
#if VECTORIZED_PADDING
		w = vY.y;
#endif
	}
#endif // VECTOR3_MERGEXY

#ifndef VECTOR3_MERGEXY_V
#define VECTOR3_MERGEXY_V
	inline void Vector3::MergeXY(Vector3Param vX, Vector3Param vY)
	{
		x = vX.x;
		y = vY.x;
		z = vX.y;
#if VECTORIZED_PADDING
		w = vY.y;
#endif
	}
#endif // VECTOR3_MERGEXY_V

#if VECTORIZED_PADDING
#ifndef VECTOR3_MERGEZW
#define VECTOR3_MERGEZW
	inline void Vector3::MergeZW(Vector3Param vW)
	{
		x = z;
		y = vW.z;
#if VECTORIZED_PADDING
		z = w;
		w = vW.w;
#endif
	}
#endif // VECTOR3_MERGEZW

#ifndef VECTOR3_MERGEZW_V
#define VECTOR3_MERGEZW_V
	inline void Vector3::MergeZW(Vector3Param vZ, Vector3Param vW)
	{
		x = vZ.z;
		y = vW.z;
#if VECTORIZED_PADDING
		z = vZ.w;
		w = vW.w;
#endif
	}
#endif // VECTOR3_MERGEZW_V

#ifndef VECTOR3_PERMUTE
#define VECTOR3_PERMUTE
    template <int permX, int permY, int permZ, int permW>
    inline void Vector3::Permute(Vector3Param v)
    {
		FastAssert(permX >= 0 && permX < 4);
		FastAssert(permY >= 0 && permY < 4);
		FastAssert(permZ >= 0 && permZ < 4);
		FastAssert(permW >= 0 && permW < 4);

		Vector3 temp(v);
        x = temp[permX];
        y = temp[permY];
        z = temp[permZ];
        w = temp[permW];
    }
#endif // VECTOR3_PERMUTE
#endif // VECTORIZED_PADDING

#ifndef VECTOR3_OPERATOR_TESTEQUAL
#define VECTOR3_OPERATOR_TESTEQUAL
	inline bool Vector3::operator==(Vector3Param a) const
	{
		return IsEqual(a);
	}
#endif // VECTOR3_OPERATORTESTEQUAL

#ifndef VECTOR3_OPERATOR_NOTEQUAL
#define VECTOR3_OPERATOR_NOTEQUAL
	inline bool Vector3::operator!=(Vector3Param a) const
	{
		return IsNotEqual(a);
	}
#endif // VECTOR3_OPERATORNOTEQUAL

#ifndef VECTOR3_FASTANGLE
#define VECTOR3_FASTANGLE
	inline float Vector3::FastAngle(Vector3Param v) const
	{
		return rage::Acosf(Clamp(Dot(v), -1.f, 1.f));
	} 
#endif // VECTOR3_FASTANGLE

#ifndef VECTOR3_OPERATOR_PLUS_V
#define VECTOR3_OPERATOR_PLUS_V
	inline Vector3 Vector3::operator+(Vector3Param V) const
	{
		return(Vector3(x+V.x,y+V.y,z+V.z));
	}
#endif // VECTOR3_OPERATORPLUSV

#ifndef VECTOR3_OPERATOR_MINUS_V
#define VECTOR3_OPERATOR_MINUS_V
	inline Vector3 Vector3::operator-(Vector3Param V) const
	{
		return(Vector3(x-V.x,y-V.y,z-V.z));
	}
#endif // VECTOR3_OPERATORMINUSV

#ifndef VECTOR3_OPERATOR_NEGATE_V
#define VECTOR3_OPERATOR_NEGATE_V
	inline Vector3 Vector3::operator-() const
	{
		return(Vector3(-x,-y,-z));
	}
#endif // VECTOR3_OPERATORNEGATEV

#ifndef VECTOR3_OPERATOR_MUL_F
#define VECTOR3_OPERATOR_MUL_F
	inline Vector3 Vector3::operator*(const float f) const
	{
		return(Vector3(x*f,y*f,z*f));
	}
#endif // VECTOR3_OPERATORMUL_F

#ifndef VECTOR3_OPERATOR_MUL_FV
#define VECTOR3_OPERATOR_MUL_FV
	inline Vector3 Vector3::operator*(Vector3Param f) const
	{
		return(Vector3(x*f.x,y*f.y,z*f.z));
	}
#endif // VECTOR3_OPERATORMUL_FV

#ifndef VECTOR3_OPERATOR_DIV_F
#define VECTOR3_OPERATOR_DIV_F
	inline Vector3 Vector3::operator/(const float f) const
	{
		float of(1.0f/f);
		return(Vector3(x*of,y*of,z*of));
	}
#endif // VECTOR3_OPERATORDIVF

#ifndef VECTOR3_OPERATOR_DIV_V
#define VECTOR3_OPERATOR_DIV_V
	inline Vector3 Vector3::operator/(Vector3Param f) const
	{
		return Vector3(x / f.x, y / f.y, z / f.z);
	}
#endif

#ifndef VECTOR3_OPERATOR_MUL_V
#define VECTOR3_OPERATOR_MUL_V
	inline Vector3 operator*(const float f,Vector3::Vector3Param V)
	{
		return(Vector3(f*V.x,f*V.y,f*V.z));
	}
#endif // VECTOR3_OPERATORMULV

#ifndef VECTOR3_OPERATOR_OR
#define VECTOR3_OPERATOR_OR
	inline Vector3 Vector3::operator|(Vector3Param f) const
	{
		Vector3 ret;
		u32* pRet = (u32*)&ret.x;
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pRet[0] = pThis[0] | pOther[0];
		pRet[1] = pThis[1] | pOther[1];
		pRet[2] = pThis[2] | pOther[2];
		return ret;
	}
#endif // VECTOR3_OPERATOR_OR

#ifndef VECTOR3_OPERATOR_AND
#define VECTOR3_OPERATOR_AND
	inline Vector3 Vector3::operator&(Vector3Param f) const
	{
		Vector3 ret;
		u32* pRet = (u32*)&ret.x;
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pRet[0] = pThis[0] & pOther[0];
		pRet[1] = pThis[1] & pOther[1];
		pRet[2] = pThis[2] & pOther[2];
		return ret;
	}
#endif // VECTOR3_OPERATOR_AND

#ifndef VECTOR3_OPERATOR_XOR
#define VECTOR3_OPERATOR_XOR
	inline Vector3 Vector3::operator^(Vector3Param f) const
	{
		Vector3 ret;
		u32* pRet = (u32*)&ret.x;
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pRet[0] = pThis[0] ^ pOther[0];
		pRet[1] = pThis[1] ^ pOther[1];
		pRet[2] = pThis[2] ^ pOther[2];
		return ret;
	}
#endif // VECTOR3_OPERATOR_XOR

#ifndef VECTOR3_OPERATOR_PLUSEQUAL
#define VECTOR3_OPERATOR_PLUSEQUAL
	inline void Vector3::operator+=(Vector3Param V)
	{
		x+=V.x;y+=V.y;z+=V.z;
	}
#endif // VECTOR3_OPERATORPLUSEQUAL

#ifndef VECTOR3_OPERATOR_MINUSEQUAL
#define VECTOR3_OPERATOR_MINUSEQUAL
	inline void Vector3::operator-=(Vector3Param V)
	{
		x-=V.x;y-=V.y;z-=V.z;
	}
#endif // VECTOR3_OPERATORMINUSEQUAL

#ifndef VECTOR3_OPERATOR_TIMESEQUAL
#define VECTOR3_OPERATOR_TIMESEQUAL
	inline void Vector3::operator*=(const float f)
	{
		x *= f;
		y *= f;
		z *= f;
	}
#endif // VECTOR3_OPERATORTIMESEQUAL

#ifndef VECTOR3_OPERATOR_TIMESEQUAL_V
#define VECTOR3_OPERATOR_TIMESEQUAL_V
	inline void Vector3::operator*=(Vector3Param f)
	{
		x *= f.x;
		y *= f.y;
		z *= f.z;
	}
#endif // VECTOR3_OPERATORTIMESEQUAL_V

#ifndef VECTOR3_OPERATOR_DIVEQUAL
#define VECTOR3_OPERATOR_DIVEQUAL
	inline void Vector3::operator/=(const float f)
	{
		float of(1.0f/f);x*=of;y*=of;z*=of;
	}
#endif // VECTOR3_OPERATORDIVEQUAL

#ifndef VECTOR3_OPERATOR_DIVEQUAL_V
#define VECTOR3_OPERATOR_DIVEQUAL_V
	inline void Vector3::operator/=(Vector3Param f)
	{
		InvScale(f);
	}
#endif // VECTOR3_OPERATORDIVEQUAL_V

#ifndef VECTOR3_OPERATOR_OREQUAL
#define VECTOR3_OPERATOR_OREQUAL
	inline void Vector3::operator|=(Vector3Param f)
	{
#if __PPU
		xyzw = __vor(xyzw, f.xyzw);
#else
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pThis[0] |= pOther[0];
		pThis[1] |= pOther[1];
		pThis[2] |= pOther[2];
#endif
	}
#endif // VECTOR3_OPERATOR_OREQUAL

#ifndef VECTOR3_OPERATOR_ANDEQUAL
#define VECTOR3_OPERATOR_ANDEQUAL
	inline void Vector3::operator&=(Vector3Param f) 
	{
#if __PPU
		xyzw = __vand(xyzw, f.xyzw);
#else
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pThis[0] &= pOther[0];
		pThis[1] &= pOther[1];
		pThis[2] &= pOther[2];
#endif
	}
#endif // VECTOR3_OPERATOR_ANDEQUAL

#ifndef VECTOR3_OPERATOR_XOREQUAL
#define VECTOR3_OPERATOR_XOREQUAL
	inline void Vector3::operator^=(Vector3Param f)
	{
#if __PPU
		xyzw = __vxor(xyzw, f.xyzw);
#else
		u32* pThis = (u32*)this;
		u32* pOther = (u32*)&f.x;
		pThis[0] ^= pOther[0];
		pThis[1] ^= pOther[1];
		pThis[2] ^= pOther[2];
#endif
	}
#endif // VECTOR3_OPERATOR_XOREQUAL

#ifndef VECTOR3_CLAMPANGLE
#define VECTOR3_CLAMPANGLE
	inline float Vector3::ClampAngle(float r)
	{
		float q = r - ((int)(r * (1.0f / (2.0f * PI))) * (2.0f * PI));
		return( q > PI ? q - (2.0f * PI) : (q < -PI ? q + (2.0f * PI) : q) );
	}
#endif // VECTOR3_CLAMPANGLE

#ifndef VECTOR3_FINDALTERNATEXYZ
#define VECTOR3_FINDALTERNATEXYZ
	inline void Vector3::FindAlternateXYZ(Vector3 &outRotation)
	{
		outRotation.x = ClampAngle(PI + x);
		outRotation.y = ClampAngle(PI - y);
		outRotation.z = ClampAngle(PI + z);
	}
#endif // VECTOR3_FINDALTERNATEXYZ

#ifndef VECTOR3_FINDALTERNATEXZY
#define VECTOR3_FINDALTERNATEXZY
	inline void Vector3::FindAlternateXZY(Vector3 &outRotation)
	{
		outRotation.x = ClampAngle(PI + x);
		outRotation.y = ClampAngle(PI + y);
		outRotation.z = ClampAngle(PI - z);
	}
#endif // VECTOR3_FINDALTERNATEXZY

#ifndef VECTOR3_LOG
#define VECTOR3_LOG
	inline void Vector3::Log()
	{
		x = log(x);
		y = log(y);
		z = log(z);
#if VECTORIZED_PADDING
		w = log(w);
#endif
	}
#endif // VECTOR3_LOG

#ifndef VECTOR3_LOG_V
#define VECTOR3_LOG_V
	inline void Vector3::Log(Vector3Param v)
	{
		x = log(v.x);
		y = log(v.y);
		z = log(v.z);
#if VECTORIZED_PADDING
		w = log(v.w);
#endif
	}
#endif	// VECTOR3_LOG_V

#ifndef VECTOR3_LOG10
#define VECTOR3_LOG10
	inline void Vector3::Log10()
	{
		x = log10(x);
		y = log10(y);
		z = log10(z);
#if VECTORIZED_PADDING
		w = log10(w);
#endif
	}
#endif // VECTOR3_LOG10

#ifndef VECTOR3_LOG10_V
#define VECTOR3_LOG10_V
	inline void Vector3::Log10(Vector3Param v)
	{
		x = log10(v.x);
		y = log10(v.y);
		z = log10(v.z);
#if VECTORIZED_PADDING
		w = log10(v.w);
#endif
	}
#endif // VECTOR3_LOG10_V


inline void Vector3::RotateX(float radians)
{
	float tsin = rage::Sinf(radians);
	float tcos = rage::Cosf(radians);

	float t = y * tcos - z * tsin;
	z = y * tsin + z * tcos;
	y = t;
}


inline void Vector3::RotateY(float radians)
{
	float tsin = rage::Sinf(radians);
	float tcos = rage::Cosf(radians);

	float t = z * tcos - x * tsin;
	x = z * tsin + x * tcos;
	z = t;
}


inline void Vector3::RotateZ(float radians)
{
	float tsin = rage::Sinf(radians);
	float tcos = rage::Cosf(radians);

	float t = x * tcos - y * tsin;
	y = x * tsin + y * tcos;
	x = t;
}

inline float Vector3::Angle(const Vector3 &v) const
{
	float mag = sqrtf(Mag2() * v.Mag2());

	float dotOverMag = Selectf(mag - VERY_SMALL_FLOAT, Dot(v) / mag, 1.f);

	return rage::Acosf(Clamp(dotOverMag, -1.f, 1.f));
}

inline float Vector3::AngleX(const Vector3& a) const
{
	float mag = sqrtf((y * y + z * z) * (a.y * a.y + a.z * a.z));

	return Selectf(mag - VERY_SMALL_FLOAT, Selectf(CrossX(a), 1, -1) * rage::Acosf(Clamp((y * a.y + z * a.z) / mag, -1.f, 1.f)), 0.f);
}

inline float Vector3::AngleY(const Vector3& a) const
{
	float mag = sqrtf((x * x + z * z) * (a.x * a.x + a.z * a.z));

	return Selectf(mag - VERY_SMALL_FLOAT, Selectf(CrossY(a), 1, -1) * rage::Acosf(Clamp((x * a.x + z * a.z) / mag, -1.f, 1.f)), 0.f);
}

inline float Vector3::AngleZ(const Vector3& a) const
{
	float mag = sqrtf((x * x + y * y) * (a.x * a.x + a.y * a.y));

	return Selectf(mag - VERY_SMALL_FLOAT, Selectf(CrossZ(a), 1, -1) * rage::Acosf(Clamp((x * a.x + y * a.y) / mag, -1.f, 1.f)), 0.f);
}

inline float Vector3::FastAngleX(const Vector3& a) const
{
	return Selectf(CrossX(a), 1, -1) * rage::Acosf(Clamp(y * a.y + z * a.z, -1.f, 1.f));
}

inline float Vector3::FastAngleY(const Vector3& a) const
{
	return Selectf(CrossY(a), 1, -1) * rage::Acosf(Clamp(x * a.x + z * a.z, -1.f, 1.f));
}

inline float Vector3::FastAngleZ(const Vector3& a) const
{
	return Selectf(CrossZ(a), 1, -1) * rage::Acosf(Clamp(x * a.x + y * a.y, -1.f, 1.f));
}

inline Vector3 InvertSafe( Vector3::Vector3Param v )
{
	Vector3 vi;
	Vector3 comparitor = Vector3(v).IsZeroV();
	vi.Invert(v);
	vi = comparitor.Select( vi, VEC3_MAX );
	return vi;
}

}	// namespace rage

#endif // VECTOR3_DEFAULT_INL
