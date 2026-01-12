// 
// vector/vector4_default.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTOR4_DEFAULT_H
#define VECTOR_VECTOR4_DEFAULT_H

namespace rage
{
#ifndef VECTOR4_CONST
#define VECTOR4_CONST
	inline Vector4::Vector4()
	{
#if __INIT_NAN
		if (!g_DisableInitNan)
		{
			MakeNan(x);
			MakeNan(y);
			MakeNan(z);
			MakeNan(w);
		}
#endif // __INIT_NAN
	}
#endif // VECTOR4_CONST

#ifndef VECTOR4_CONST_F4
#define VECTOR4_CONST_F4
	inline Vector4::Vector4(float x0,float y0,float z0,float w0)
	{
		x = x0;
		y = y0;
		z = z0;
		w = w0;
	}
#endif // VECTOR4_CONST_F4

#ifndef VECTOR4_CONST_F1
#define VECTOR4_CONST_F1
	inline Vector4::Vector4(float v)
	{
		x = y = z = w = v;
	}
#endif // VECTOR4_CONST_F1

#ifndef VECTOR4_ZEROTYPE
#define VECTOR4_ZEROTYPE
	inline Vector4::Vector4( _ZeroType )
	{
		x = y = z = w = 0.0f;
	}
#endif // VECTOR4_ZEROTYPE


#ifndef VECTOR4_CONST_V
#define VECTOR4_CONST_V
	inline Vector4::Vector4(const Vector4 &vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = vec.w;
	}
#endif // VECTOR4_CONST_V

#ifndef VECTOR4_CONST_RES
#define VECTOR4_CONST_RES
	inline Vector4::Vector4(class datResource&)
	{
	}
#endif // VECTOR4_CONST_RES

#ifndef VECTOR4_GETX
#define VECTOR4_GETX
	inline float Vector4::GetX() const
	{
		return x;
	}
#endif // VECTOR4_GETX

#ifndef VECTOR4_GETY
#define VECTOR4_GETY
	inline float Vector4::GetY() const
	{
		return y;
	}
#endif // VECTOR4_GETY

#ifndef VECTOR4_GETZ
#define VECTOR4_GETZ
	inline float Vector4::GetZ() const
	{
		return z;
	}
#endif // VECTOR4_GETZ

#ifndef VECTOR4_GETW
#define VECTOR4_GETW
	inline float Vector4::GetW() const
	{
		return w;
	}
#endif // VECTOR4_GETW

#ifndef VECTOR4_SETX
#define VECTOR4_SETX
	inline void Vector4::SetX(float f)
	{
		x = f;
	}
#endif // VECTOR4_SETX

#ifndef VECTOR4_SETY
#define VECTOR4_SETY
	inline void Vector4::SetY(float f)
	{
		y = f;
	}
#endif // VECTOR4_SETY

#ifndef VECTOR4_SETZ
#define VECTOR4_SETZ
	inline void Vector4::SetZ(float f)
	{
		z = f;
	}
#endif // VECTOR4_SETZ

#ifndef VECTOR4_SETW
#define VECTOR4_SETW
	inline void Vector4::SetW(float f)
	{
		w = f;
	}
#endif // VECTOR4_SETW

#ifndef VECTOR4_SPLATX
#define VECTOR4_SPLATX
	inline void Vector4::SplatX()
	{
		x = y = z = w = x;
	}
#endif // VECTOR4_SPLATX

#ifndef VECTOR4_SPLATY
#define VECTOR4_SPLATY
	inline void Vector4::SplatY()
	{
		x = y = z = w = y;
	}
#endif // VECTOR4_SPLATY

#ifndef VECTOR4_SPLATZ
#define VECTOR4_SPLATZ
	inline void Vector4::SplatZ()
	{
		x = y = z = w = z;
	}
#endif // VECTOR4_SPLATZ_V

#ifndef VECTOR4_SPLATW
#define VECTOR4_SPLATW
	inline void Vector4::SplatW()
	{
		x = y = z = w = w;
	}
#endif // VECTOR4_SPLATW

#ifndef VECTOR4_SPLATX_V
#define VECTOR4_SPLATX_V
	inline void Vector4::SplatX(Vector4Param in)
	{
		x = y = z = w = in.x;
	}
#endif // VECTOR4_SPLATX_V

#ifndef VECTOR4_SPLATY_V
#define VECTOR4_SPLATY_V
	inline void Vector4::SplatY(Vector4Param in)
	{
		x = y = z = w = in.y;
	}
#endif // VECTOR4_SPLATY_V

#ifndef VECTOR4_SPLATZ_V
#define VECTOR4_SPLATZ_V
	inline void Vector4::SplatZ(Vector4Param in)
	{
		x = y = z = w = in.z;
	}
#endif // VECTOR4_SPLATZ_V

#ifndef VECTOR4_SPLATW_V
#define VECTOR4_SPLATW_V
	inline void Vector4::SplatW(Vector4Param in)
	{
		x = y = z = w = in.w;
	}
#endif // VECTOR4_SPLATW_V

#ifndef VECTOR4_GETXV
#define VECTOR4_GETXV
	inline Vector4 Vector4::GetXV() const
	{
		Vector4 ret(*this);
		ret.SplatX();
		return ret;
	}
#endif // VECTOR4_GETXV

#ifndef VECTOR4_GETYV
#define VECTOR4_GETYV
	inline Vector4 Vector4::GetYV() const
	{
		Vector4 ret(*this);
		ret.SplatY();
		return ret;
	}
#endif // VECTOR4_GETYV

#ifndef VECTOR4_GETZV
#define VECTOR4_GETZV
	inline Vector4 Vector4::GetZV() const
	{
		Vector4 ret(*this);
		ret.SplatZ();
		return ret;
	}
#endif // VECTOR4_GETZV

#ifndef VECTOR4_GETWV
#define VECTOR4_GETWV
	inline Vector4 Vector4::GetWV() const
	{
		Vector4 ret(*this);
		ret.SplatW();
		return ret;
	}
#endif // VECTOR4_GETWV

#ifndef VECTOR4_SET_F4
#define VECTOR4_SET_F4
	inline void Vector4::Set(float x0,float y0,float z0,float w0)
	{
		x = x0;
		y = y0;
		z = z0;
		w = w0;
	}
#endif // VECTOR4_SET_F4

#ifndef VECTOR4_SET_V
#define VECTOR4_SET_V
	inline void Vector4::Set(const Vector4& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
		w = a.w;
	}
#endif // VECTOR4_SET_V

#ifndef VECTOR4_SET_F
#define VECTOR4_SET_F
	inline void Vector4::Set(float s)	
	{
		x = y = z = w = s;
	}
#endif // VECTOR4_SET_F

#ifndef VECTOR4_SETSCALED
#define VECTOR4_SETSCALED
	inline void Vector4::SetScaled(const Vector4& a, float s)
	{
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
		w = a.w * s;
	}
#endif // VECTOR4_SETSCALED

#ifndef VECTOR4_SETSCALED_V
#define VECTOR4_SETSCALED_V
	inline void Vector4::SetScaled(Vector4Param a, Vector4Param s)
	{
		x = a.x * s.x;
		y = a.y * s.y;
		z = a.z * s.z;
		w = a.w * s.w;
	}
#endif // VECTOR4_SETSCALED_V

#ifndef VECTOR4_ZERO
#define VECTOR4_ZERO
	inline void Vector4::Zero()
	{
		x = y = z = w = 0.0f;
	}
#endif // VECTOR4_ZERO

#ifndef VECTOR4_GETVECTOR3_V
#define VECTOR4_GETVECTOR3_V
	inline void Vector4::GetVector3(Vector3& vec) const
	{
		vec.x = x;
		vec.y = y;
		vec.z = z;
#if VECTORIZED_PADDING
		vec.w = w;
#endif
	}
#endif // VECTOR4_GETVECTOR3_V

#ifndef VECTOR4_GETVECTOR3
#define VECTOR4_GETVECTOR3
	inline Vector3 Vector4::GetVector3() const
	{
		Vector3 ret;
		ret.x = x;
		ret.y = y;
		ret.z = z;
#if VECTORIZED_PADDING
		ret.w = w;
#endif
		return ret;
	}
#endif // VECTOR4_GETVECTOR3

#ifndef VECTOR4_SETVECTOR3
#define VECTOR4_SETVECTOR3
	inline void Vector4::SetVector3(Vector3Param vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
#if VECTORIZED_PADDING
	#if HACK_GTA4
		w = 0.0f;
	#else
		w = vec.w;
	#endif
#endif
	}
#endif // VECTOR4_SETVECTOR3

#ifndef VECTOR4_SETVECTOR3CLEARW
#define VECTOR4_SETVECTOR3CLEARW
	inline void Vector4::SetVector3ClearW(Vector3Param vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = 0.f;
	}
#endif // VECTOR4_SETVECTOR3CLEARW

#ifndef VECTOR4_SETVECTOR3LEAVEW
#define VECTOR4_SETVECTOR3LEAVEW
	inline void Vector4::SetVector3LeaveW(Vector3Param vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}
#endif // VECTOR4_SETVECTOR3LEAVEW

#ifndef VECTOR4_CLEARW
#define VECTOR4_CLEARW
	inline void Vector4::ClearW()
	{
		w = 0.0f;
	}
#endif

#ifndef VECTOR4_ADDVECTOR3
#define VECTOR4_ADDVECTOR3
	inline void Vector4::AddVector3(Vector3Param vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
#if VECTORIZED_PADDING
	#if HACK_GTA4
		// do nothing
	#else
		w += vec.w;
	#endif
#endif
	}
#endif // VECTOR4_ADDVECTOR3

#ifndef VECTOR4_ADDVECTOR3XYZ
#define VECTOR4_ADDVECTOR3XYZ
	inline void Vector4::AddVector3XYZ(Vector3Param vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
	}
#endif // VECTOR4_ADDVECTOR3XYZ

#ifndef VECTOR4_PACK1010102
#define VECTOR4_PACK1010102
    inline u32 Vector4::Pack1010102() const
	{
		u32 wPack = w > 0.5f? 1 << 30 : w < -0.5f? -1 << 30 : 0;
		return PackFixedPoint(x,10,0) | PackFixedPoint(y,10,10) | PackFixedPoint(z,10,20) | wPack;
	}
#endif // VECTOR4_PACK1010102

#ifndef VECTOR4_UNPACK1010102
#define VECTOR4_UNPACK1010102
	inline void Vector4::Unpack1010102(u32 packed)
	{
		int ux = (packed & 0x3FF) << 22;
		int uy = ((packed >> 10) & 0x3FF) << 22;
		int uz = ((packed >> 20) & 0x3FF) << 22;
		int uw = (int)packed;
		float fx = (float)(ux >> 22);
		float fy = (float)(uy >> 22);
		float fz = (float)(uz >> 22);
		float fw = (float)(uw >> 30);

		const float scale = 0.0019569471624266144814090019569472f;	 // 1/511
		Set(fx * scale, fy * scale, fz * scale, fw);
	}
#endif // VECTOR4_UNPACK1010102

#ifndef VECTOR4_PACKCOLOR
#define VECTOR4_PACKCOLOR
	inline void Vector4::PackColor()
	{
		u32 r = (u8)x;
		u32 g = (u8)y;
		u32 b = (u8)z;
		u32 a = (u8)w;
		ix = (b | (g << 8) | (r << 16) | (a << 24));
	}
#endif // VECTOR4_PACKCOLOR

#ifndef VECTOR4_UNPACKCOLOR
#define VECTOR4_UNPACKCOLOR
	inline void Vector4::UnpackColor()
	{
		float b = (float)((u8)(ix));
		float g = (float)((u8)(ix >> 8));
		float r = (float)((u8)(ix >> 16));
		float a = (float)((u8)(ix >> 24));
		
		x = r;
		y = g;
		z = b;
		w = a;
	}
#endif // VECTOR4_UNPACKCOLOR

#ifndef VECTOR4_SCALE
#define VECTOR4_SCALE
	inline void Vector4::Scale(float f)
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
	}
#endif // VECTOR4_SCALE

#ifndef VECTOR4_SCALEV
#define VECTOR4_SCALEV
	inline void Vector4::Scale(const Vector4& f)
	{
		x *= f.x;
		y *= f.y;
		z *= f.z;
		w *= f.w;
	}
#endif // VECTOR4_SCALEV

#ifndef VECTOR4_SCALE_V
#define VECTOR4_SCALE_V
	inline void Vector4::Scale(const Vector4& a,float f)
	{
		x = a.x * f;
		y = a.y * f;
		z = a.z * f;
		w = a.w * f;
	}
#endif // VECTOR4_SCALE_V

#ifndef VECTOR4_SCALE3
#define VECTOR4_SCALE3
	inline void Vector4::Scale3(float f)
	{
		x *= f;
		y *= f;
		z *= f;
	}
#endif // VECTOR4_SCALE3

#ifndef VECTOR4_SCALE3_V
#define VECTOR4_SCALE3_V
	inline void Vector4::Scale3(const Vector4& a, float f)
	{
		x = a.x * f;
		y = a.y * f;
		z = a.z * f;
	}
#endif // VECTOR4_SCALE3_V

#ifndef VECTOR4_INVSCALE
#define VECTOR4_INVSCALE
	inline void Vector4::InvScale(float f)
	{
		float inv = 1.0f / f;
		x *= inv;
		y *= inv;
		z *= inv;
		w *= inv;
	}
#endif // VECTOR4_INVSCALE

#ifndef VECTOR4_INVSCALE_V
#define VECTOR4_INVSCALE_V
	inline void Vector4::InvScale(Vector4Param f)
	{
		x /= f.x;
		y /= f.y;
		z /= f.z;
		w /= f.w;
	}
#endif // VECTOR4_INVSCALE_V

#ifndef VECTOR4_INVSCALE_VF
#define VECTOR4_INVSCALE_VF
	inline void Vector4::InvScale(Vector4Param a, float f)
	{
		float inv = 1.0f / f;
		x = a.x * inv;
		y = a.y * inv;
		z = a.z * inv;
		w = a.w * inv;
	}
#endif // VECTOR4_INVSCALE_VF

#ifndef VECTOR4_INVSCALE_VV3
#define VECTOR4_INVSCALE_VV3
	inline void Vector4::InvScale(const Vector4& a, Vector3Param f)
	{
		x = a.x / f.x;
		y = a.y / f.y;
		z = a.z / f.z;
		w = a.w;
	}
#endif // VECTOR4_INVSCALE_VV3

#ifndef VECTOR4_INVSCALE_VV
#define VECTOR4_INVSCALE_VV
	inline void Vector4::InvScale(Vector4Param a, Vector4Param f)
	{
		x = a.x / f.x;
		y = a.y / f.y;
		z = a.z / f.z;
		w = a.w / f.w;
	}
#endif // VECTOR4_INVSCALE_VV

#ifndef VECTOR4_ADD_F4
#define VECTOR4_ADD_F4
	inline void Vector4::Add(float sx, float sy, float sz, float sw)
	{
		x += sx;
		y += sy;
		z += sz;
		w += sw;
	}
#endif // VECTOR4_ADD_F4

#ifndef VECTOR4_ADD_V
#define VECTOR4_ADD_V
	inline void Vector4::Add(Vector4Param a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		w += a.w;
	}
#endif // VECTOR4_ADD_V

#ifndef VECTOR4_ADD_VV
#define VECTOR4_ADD_VV
	inline void Vector4::Add(Vector4Param a,Vector4Param b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
		w = a.w + b.w;
	}
#endif // VECTOR4_ADD_VV

#ifndef VECTOR4_ADD_VV3
#define VECTOR4_ADD_VV3
	inline void Vector4::Add(Vector3Param a, Vector3Param b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
	}
#endif // VECTOR4_ADD_VV3

#ifndef VECTOR4_ADDSCALED_F
#define VECTOR4_ADDSCALED_F
	inline void Vector4::AddScaled(const Vector4& a,float s)
	{
		x += a.x * s;
		y += a.y * s;
		z += a.z * s;
		w += a.w * s;
	}
#endif // VECTOR4_ADDSCALED_F

#ifndef VECTOR4_ADDSCALED_V
#define VECTOR4_ADDSCALED_V
	inline void Vector4::AddScaled(Vector4Param a,Vector4Param s)
	{
		x += a.x * s.x;
		y += a.y * s.y;
		z += a.z * s.z;
		w += a.w * s.w;
	}
#endif // VECTOR4_ADDSCALED_V

#ifndef VECTOR4_ADDSCALED_VF
#define VECTOR4_ADDSCALED_VF
	inline void Vector4::AddScaled(const Vector4& a,const Vector4& b,float s)
	{
		x = a.x + b.x * s;
		y = a.y + b.y * s;
		z = a.z + b.z * s;
		w = a.w + b.w * s;
	}
#endif // VECTOR4_ADDSCALED_VF

#ifndef VECTOR4_ADDSCALED_VV
#define VECTOR4_ADDSCALED_VV
	inline void Vector4::AddScaled(Vector4Param a,Vector4Param b,Vector4Param s)
	{
		x = a.x + b.x * s.x;
		y = a.y + b.y * s.y;
		z = a.z + b.z * s.z;
		w = a.w + b.w * s.w;
	}
#endif // VECTOR4_ADDSCALED_VV

#ifndef VECTOR4_ADDSCALED3_F
#define VECTOR4_ADDSCALED3_F
	inline void Vector4::AddScaled3(const Vector4& a,float s)
	{
		x += a.x * s;
		y += a.y * s;
		z += a.z * s;
	}
#endif // VECTOR4_ADDSCALED3_F

#ifndef VECTOR4_ADDSCALED3_V
#define VECTOR4_ADDSCALED3_V
	inline void Vector4::AddScaled3(const Vector4& a,const Vector4& s)
	{
		x += a.x * s.x;
		y += a.y * s.y;
		z += a.z * s.z;
	}
#endif // VECTOR4_ADDSCALED3_V

#ifndef VECTOR4_ADDSCALED3_VF
#define VECTOR4_ADDSCALED3_VF
	inline void Vector4::AddScaled3(const Vector4& a,const Vector4& b,float s)
	{
		x = a.x + b.x * s;
		y = a.y + b.y * s;
		z = a.z + b.z * s;
	}
#endif // VECTOR4_ADDSCALED_VF

#ifndef VECTOR4_ADDSCALED3_VV
#define VECTOR4_ADDSCALED3_VV
	inline void Vector4::AddScaled3(const Vector4& a,const Vector4& b,const Vector4& s)
	{
		x = a.x + b.x * s.x;
		y = a.y + b.y * s.y;
		z = a.z + b.z * s.z;
	}
#endif // VECTOR4_ADDSCALED3_VV

#ifndef VECTOR4_SUBTRACT_F4
#define VECTOR4_SUBTRACT_F4
	inline void Vector4::Subtract(float sx, float sy, float sz, float sw)
	{
		x -= sx;
		y -= sy;
		z -= sz;
		w -= sw;
	}
#endif // VECTOR4_SUBTRACT_F4

#ifndef VECTOR4_SUBTRACT_V
#define VECTOR4_SUBTRACT_V
	inline void Vector4::Subtract(Vector4Param a)
	{
		x -= a.x;
		y -= a.y;
		z -= a.z;
		w -= a.w;
	}
#endif // VECTOR4_SUBTRACT_V

#ifndef VECTOR4_SUBTRACT_VV
#define VECTOR4_SUBTRACT_VV
	inline void Vector4::Subtract(Vector4Param a,Vector4Param b)
	{
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
		w = a.w - b.w;
	}
#endif // VECTOR4_SUBTRACT_VV

#ifndef VECTOR4_SUBTRACT_VV3
#define VECTOR4_SUBTRACT_VV3
	inline void Vector4::Subtract(Vector3Param a,Vector3Param b)
	{
		x = a.x - b.x;
		y = a.y - b.y;
		z = a.z - b.z;
	}
#endif // VECTOR4_SUBTRACT_VV3

#ifndef VECTOR4_SUBTRACTSCALED_F
#define VECTOR4_SUBTRACTSCALED_F
	inline void Vector4::SubtractScaled(const Vector4& a, float s)
	{
		x -= a.x * s;
		y -= a.y * s;
		z -= a.z * s;
		w -= a.w * s;
	}
#endif // VECTOR4_SUBTRACTSCALED_F

#ifndef VECTOR4_SUBTRACTSCALED_V
#define VECTOR4_SUBTRACTSCALED_V
	inline void Vector4::SubtractScaled(Vector4Param a, Vector4Param s)
	{
		x -= a.x * s.x;
		y -= a.y * s.y;
		z -= a.z * s.z;
		w -= a.w * s.w;
	}
#endif // VECTOR4_SUBTRACTSCALED_V

#ifndef VECTOR4_SUBTRACTSCALED_VF
#define VECTOR4_SUBTRACTSCALED_VF
	inline void Vector4::SubtractScaled(const Vector4& a, const Vector4& b,float s)
	{
		x = a.x - b.x * s;
		y = a.y - b.y * s;
		z = a.z - b.z * s;
		w = a.w - b.w * s;
	}
#endif // VECTOR4_SUBTRACTSCALED_VF

#ifndef VECTOR4_SUBTRACTSCALED_VV
#define VECTOR4_SUBTRACTSCALED_VV
	inline void Vector4::SubtractScaled(Vector4Param a, Vector4Param b,Vector4Param s)
	{
		x = a.x - b.x * s.x;
		y = a.y - b.y * s.y;
		z = a.z - b.z * s.z;
		w = a.w - b.w * s.w;
	}
#endif // VECTOR4_SUBTRACTSCALED_VV

#ifndef VECTOR4_MULTIPLY
#define VECTOR4_MULTIPLY
	inline void Vector4::Multiply(Vector4Param a)
	{
		x *= a.x;
		y *= a.y;
		z *= a.z;
		w *= a.w;
	}
#endif // VECTOR4_MULTIPLY

#ifndef VECTOR4_MULTIPLY_V
#define VECTOR4_MUTLIPLY_V
	inline void Vector4::Multiply(Vector4Param a,Vector4Param b)
	{
		x = a.x * b.x;
		y = a.y * b.y;
		z = a.z * b.z;
		w = a.w * b.w;
	}
#endif // VECTOR4_MULTIPLY_V

#ifndef VECTOR4_NEGATE
#define VECTOR4_NEGATE
	inline void Vector4::Negate()
	{
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}
#endif // VECTOR4_NEGATE

#ifndef VECTOR4_NEGATE_V
#define VECTOR4_NEGATE_V
	inline void Vector4::Negate(Vector4Param a)
	{
		x = -a.x;
		y = -a.y;
		z = -a.z;
		w = -a.w;
	}
#endif // VECTOR4_NEGATE_V

#ifndef VECTOR4_ABS
#define VECTOR4_ABS
	inline void Vector4::Abs()
	{
		x = fabs(x);
		y = fabs(y);
		z = fabs(z);
		w = fabs(w);
	}
#endif // VECTOR4_ABS

#ifndef VECTOR4_ABS_V
#define VECTOR4_ABS_V
	inline void Vector4::Abs(Vector4Param a)
	{
		x = fabs(a.x);
		y = fabs(a.y);
		z = fabs(a.z);
		w = fabs(a.w);
	}
#endif // VECTOR4_ABS_V

#ifndef VECTOR4_INVERT
#define VECTOR4_INVERT
	inline void Vector4::Invert()
	{
		x = 1.0f / x;
		y = 1.0f / y;
		z = 1.0f / z;
		w = 1.0f / w;
	}
#endif // VECTOR4_INVERT

#ifndef VECTOR4_INVERT_V
#define VECTOR4_INVERT_V
	inline void Vector4::Invert(Vector4Param a)
	{
		x = 1.0f / a.x;
		y = 1.0f / a.y;
		z = 1.0f / a.z;
		w = 1.0f / a.w;
	}
#endif // VECTOR4_INVERT_V

#ifndef VECTOR4_INVERT_SAFE
#define VECTOR4_INVERT_SAFE
	inline void Vector4::InvertSafe()
	{
		x = ( x != 0.0f ? 1.0f / x : FLT_MAX );
		y = ( y != 0.0f ? 1.0f / y : FLT_MAX );
		z = ( z != 0.0f ? 1.0f / z : FLT_MAX );
		w = ( w != 0.0f ? 1.0f / w : FLT_MAX );
	}
#endif // VECTOR4_INVERT_SAFE

#ifndef VECTOR4_INVERTSAFE_V
#define VECTOR4_INVERTSAFE_V
	inline void Vector4::InvertSafe(const Vector4& a)
	{
		x = ( a.x != 0.0f ? 1.0f / a.x : FLT_MAX );
		y = ( a.y != 0.0f ? 1.0f / a.y : FLT_MAX );
		z = ( a.z != 0.0f ? 1.0f / a.z : FLT_MAX );
		w = ( a.w != 0.0f ? 1.0f / a.w : FLT_MAX );
	}
#endif // VECTOR4_INVERTSAFE_V

#ifndef VECTOR4_NORMALIZE
#define VECTOR4_NORMALIZE
	inline void Vector4::Normalize()
	{
		Scale(InvMag());
	}
#endif // VECTOR4_NORMALIZE

#ifndef VECTOR4_NORMALIZE_FAST
#define VECTOR4_NORMALIZE_FAST
	inline void Vector4::NormalizeFast()
	{
		Scale(InvMag());
	}
#endif // VECTOR4_NORMALIZE_FAST

#ifndef VECTOR4_NORMALIZE_V
#define VECTOR4_NORMALIZE_V
	inline void Vector4::Normalize(Vector4Param a)
	{
		Set(a);
		Normalize();
	}
#endif // VECTOR4_NORMALIZE_V

#ifndef VECTOR4_NORMALIZE_FAST_V
#define VECTOR4_NORMALIZE_FAST_V
	inline void Vector4::NormalizeFast(Vector4Param a)
	{
		Set(a);
		NormalizeFast();
	}
#endif // VECTOR4_NORMALIZE_FAST_V

#ifndef VECTOR4_NORMALIZE3
#define VECTOR4_NORMALIZE3
	inline void Vector4::Normalize3()
	{
		Scale3(InvMag3());
	}
#endif // VECTOR4_NORMALIZE3

#ifndef VECTOR4_NORMALIZE_FAST3
#define VECTOR4_NORMALIZE_FAST3
	inline void Vector4::NormalizeFast3()
	{
		Scale3(InvMag3());
	}
#endif // VECTOR4_NORMALIZE_FAST3

#ifndef VECTOR4_NORMALIZE3_V
#define VECTOR4_NORMALIZE3_V
	inline void Vector4::Normalize3(Vector4Param a)
	{
		Set(a);
		Normalize3();
	}
#endif // VECTOR4_NORMALIZE3_V

#ifndef VECTOR4_NORMALIZE_FAST3_V
#define VECTOR4_NORMALIZE_FAST3_V
	inline void Vector4::NormalizeFast3(Vector4Param a)
	{
		Set(a);
		NormalizeFast3();
	}
#endif // VECTOR4_NORMALIZE_FAST3_V

#ifndef VECTOR4_DOT
#define VECTOR4_DOT
	inline float Vector4::Dot(Vector4Param a) const
	{
		return ((x * a.x) + (y * a.y) + (z * a.z) + (w * a.w));
	}
#endif // VECTOR4_DOT

#ifndef VECTOR4_DOT_V
#define VECTOR4_DOT_V
	inline Vector4 Vector4::DotV(Vector4Param a) const
	{
		float dot = Dot(a);
		return Vector4(dot, dot, dot, dot);
	}
#endif // VECTOR4_DOT_V

#ifndef VECTOR4_DOT3
#define VECTOR4_DOT3
	inline float Vector4::Dot3(Vector4Param a) const
	{
		return ((x * a.x) + (y * a.y) + (z * a.z));
	}
#endif // VECTOR4_DOT3

#ifndef VECTOR4_DOT3_V
#define VECTOR4_DOT3_V
	inline float Vector4::Dot3(Vector3Param a) const
	{
		return ((x * a.x) + (y * a.y) + (z * a.z));
	}
#endif // VECTOR4_DOT3_V

#ifndef VECTOR4_DOT3V_V
#define VECTOR4_DOT3V_V
    inline Vector4 Vector4::Dot3V(Vector4Param a) const
	{
		float dot = Dot3(a);
		return Vector4(dot, dot, dot, dot);
	}
#endif // VECTOR4_DOT3V_V

#ifndef VECTOR4_DOT3V_V3
#define VECTOR4_DOT3V_V3
    inline Vector4 Vector4::Dot3V(Vector3Param a) const
	{
		float dot = Dot3(a);
		return Vector4(dot, dot, dot, dot);
	}
#endif // VECTOR4_DOT3V_V3

#ifndef VECTOR4_CROSS3
#define VECTOR4_CROSS3
	inline void Vector4::Cross(Vector3Param b)
	{
		float rx = __cross2(y, b.z, z, b.y);
		float ry = __cross2(z, b.x, x, b.z);
		float rz = __cross2(x, b.y, y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS3

#ifndef VECTOR4_CROSS4
#define VECTOR4_CROSS4
	inline void Vector4::Cross(const Vector4& b)
	{
		float rx = __cross2(y, b.z, z, b.y);
		float ry = __cross2(z, b.x, x, b.z);
		float rz = __cross2(x, b.y, y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS4

#ifndef VECTOR4_CROSS3_V
#define VECTOR4_CROSS3_V
	inline void Vector4::Cross(Vector3Param a,Vector3Param b)
	{
		float rx = __cross2(a.y, b.z, a.z, b.y);
		float ry = __cross2(a.z, b.x, a.x, b.z);
		float rz = __cross2(a.x, b.y, a.y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS3_V

#ifndef VECTOR4_CROSS43_V
#define VECTOR4_CROSS43_V
	inline void Vector4::Cross(const Vector4& a,Vector3Param b)
	{
		float rx = __cross2(a.y, b.z, a.z, b.y);
		float ry = __cross2(a.z, b.x, a.x, b.z);
		float rz = __cross2(a.x, b.y, a.y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS43_V

#ifndef VECTOR4_CROSS34_V
#define VECTOR4_CROSS34_V
	inline void Vector4::Cross(Vector3Param a,const Vector4& b)
	{
		float rx = __cross2(a.y, b.z, a.z, b.y);
		float ry = __cross2(a.z, b.x, a.x, b.z);
		float rz = __cross2(a.x, b.y, a.y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS34_V

#ifndef VECTOR4_CROSS4_V
#define VECTOR4_CROSS4_V
	inline void Vector4::Cross(const Vector4& a,const Vector4& b)
	{
		float rx = __cross2(a.y, b.z, a.z, b.y);
		float ry = __cross2(a.z, b.x, a.x, b.z);
		float rz = __cross2(a.x, b.y, a.y, b.x);

		x = rx;
		y = ry;
		z = rz;
		w = 1;
	}
#endif // VECTOR4_CROSS4_V

#ifndef VECTOR4_AVERAGE
#define VECTOR4_AVERAGE
	inline void Vector4::Average(Vector4Param a)
	{
		x = (x + a.x) * 0.5f;
		y = (y + a.y) * 0.5f;
		z = (z + a.z) * 0.5f;
		w = (w + a.w) * 0.5f;
	}
#endif // VECTOR4_AVERAGE

#ifndef VECTOR4_AVERAGE_V
#define VECTOR4_AVERAGE_V
	inline void Vector4::Average(Vector4Param a, Vector4Param b)
	{
		x = (b.x + a.x) * 0.5f;
		y = (b.y + a.y) * 0.5f;
		z = (b.z + a.z) * 0.5f;
		w = (b.w + a.w) * 0.5f;
	}
#endif // VECTOR4_AVERAGE_V

#ifndef VECTOR4_LERP_V
#define VECTOR4_LERP_V
	inline void Vector4::Lerp(float t, const Vector4& a, const Vector4& b)
	{
		x = a.x + ((b.x - a.x) * t);
		y = a.y + ((b.y - a.y) * t);
		z = a.z + ((b.z - a.z) * t);
		w = a.w + ((b.w - a.w) * t);
	}
#endif // VECTOR4_LERP_V

#ifndef VECTOR4_LERPV_V
#define VECTOR4_LERPV_V
	inline void Vector4::Lerp(Vector4Param t, Vector4Param a, Vector4Param b)
	{
		x = a.x + ((b.x - a.x) * t.x);
		y = a.y + ((b.y - a.y) * t.y);
		z = a.z + ((b.z - a.z) * t.z);
		w = a.w + ((b.w - a.w) * t.w);
	}
#endif // VECTOR4_LERPV_V

#ifndef VECTOR4_LERP
#define VECTOR4_LERP
	inline void Vector4::Lerp(float t, const Vector4& a)
	{
		x = x + ((a.x - x) * t);
		y = y + ((a.y - y) * t);
		z = z + ((a.z - z) * t);
		w = w + ((a.w - w) * t);
	}
#endif // VECTOR4_LERP

#ifndef VECTOR4_LERPV
#define VECTOR4_LERPV
	inline void Vector4::Lerp(Vector4Param t, Vector4Param a)
	{
		x = x + ((a.x - x) * t.x);
		y = y + ((a.y - y) * t.y);
		z = z + ((a.z - z) * t.z);
		w = w + ((a.w - w) * t.w);
	}
#endif // VECTOR4_LERPV

#ifndef VECTOR4_POW
#define VECTOR4_POW
	inline void Vector4::Pow(Vector4Param a, Vector4Param b)
	{
		x = powf(a.x, b.x);
		y = powf(a.y, b.y);
		z = powf(a.z, b.z);
		w = powf(a.w, b.w);
	}
#endif // VECTOR4_POW

#ifndef VECTOR4_EXP
#define VECTOR4_EXP
	inline void Vector4::Exp(Vector4Param a)
	{
		x = expf(a.x);
		y = expf(a.y);
		z = expf(a.z);
		w = expf(a.w);
	}
#endif // VECTOR4_EXP

#ifndef VECTOR4_LOG
#define VECTOR4_LOG
	inline void Vector4::Log()
	{
		x = log(x);
		y = log(y);
		z = log(z);
		w = log(w);
	}
#endif // VECTOR4_LOG

#ifndef VECTOR4_LOG_V
#define VECTOR4_LOG_V
	inline void Vector4::Log(Vector4Param v)
	{
		x = log(v.x);
		y = log(v.y);
		z = log(v.z);
		w = log(v.w);
	}
#endif	// VECTOR4_LOG_V

#ifndef VECTOR4_LOG10
#define VECTOR4_LOG10
	inline void Vector4::Log10()
	{
		x = log10(x);
		y = log10(y);
		z = log10(z);
		w = log10(w);
	}
#endif // VECTOR4_LOG10

#ifndef VECTOR4_LOG10_V
#define VECTOR4_LOG10_V
	inline void Vector4::Log10(Vector4Param v)
	{
		x = log10(v.x);
		y = log10(v.y);
		z = log10(v.z);
		w = log10(v.w);
	}
#endif // VECTOR4_LOG10_V

#ifndef VECTOR4_SQRTV
#define VECTOR4_SQRTV
	inline Vector4 Vector4::SqrtV() const
	{
		Vector4 ret;
		ret.x = Sqrtf(x);
		ret.y = Sqrtf(y);
		ret.z = Sqrtf(z);
		ret.w = Sqrtf(w);
		return ret;
	}
#endif // VECTOR4_SQRTV

#ifndef VECTOR4_MAG
#define VECTOR4_MAG
	inline float Vector4::Mag() const
	{
		return sqrtf(Dot(*this));
	}
#endif // VECTOR4_MAG

#ifndef VECTOR4_MAGV
#define VECTOR4_MAGV
	inline Vector4 Vector4::MagV() const
	{
		float mag = sqrtf(Dot(*this));
		return Vector4(mag, mag, mag, mag);
	}
#endif // VECTOR4_MAGV

#ifndef VECTOR4_MAG2
#define VECTOR4_MAG2
	inline float Vector4::Mag2() const
	{
		return Dot(*this);
	}
#endif // VECTOR4_MAG2

#ifndef VECTOR4_MAG2V
#define VECTOR4_MAG2V
	inline Vector4 Vector4::Mag2V() const
	{
		float mag = Dot(*this);
		return Vector4(mag, mag, mag, mag);
	}
#endif // VECTOR4_MAG2V

#ifndef VECTOR4_MAG3
#define VECTOR4_MAG3
	inline float Vector4::Mag3() const
	{
		return sqrtf(Dot3(*this));
	}
#endif // VECTOR4_MAG3

#ifndef VECTOR4_MAG3V
#define VECTOR4_MAG3V
	inline Vector4 Vector4::Mag3V() const
	{
		float mag = sqrtf(Dot3(*this));
		return Vector4(mag, mag, mag, mag);
	}
#endif // VECTOR4_MAG3V

#ifndef VECTOR4_MAG32
#define VECTOR4_MAG32
	inline float Vector4::Mag32() const
	{
		return Dot3(*this);
	}
#endif // VECTOR4_MAG32

#ifndef VECTOR4_MAG32V
#define VECTOR4_MAG32V
	inline Vector4 Vector4::Mag32V() const
	{
		float mag = Dot3(*this);
		return Vector4(mag, mag, mag, mag);
	}
#endif // VECTOR4_MAG32V

#ifndef VECTOR4_INVMAG
#define VECTOR4_INVMAG
	inline float Vector4::InvMag() const
	{
		return invsqrtf(Dot(*this));
	}
#endif // VECTOR4_INVMAG

#ifndef VECTOR4_INVMAGV
#define VECTOR4_INVMAGV
	inline Vector4 Vector4::InvMagV() const
	{
		float imag = invsqrtf(Dot(*this));
		return Vector4(imag, imag, imag, imag);
	}
#endif // VECTOR4_INVMAGV

#ifndef VECTOR4_INVMAG3
#define VECTOR4_INVMAG3
	inline float Vector4::InvMag3() const
	{
		return invsqrtf(Dot3(*this));
	}
#endif // VECTOR4_INVMAG3

#ifndef VECTOR4_INVMAG3V
#define VECTOR4_INVMAG3V
	inline Vector4 Vector4::InvMag3V() const
	{
		float imag = invsqrtf(Dot3(*this));
		return Vector4(imag, imag, imag, imag);
	}
#endif // VECTOR4_INVMAG3V

#ifndef VECTOR4_DIST
#define VECTOR4_DIST
	inline float Vector4::Dist(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag();
	}
#endif // VECTOR4_DIST

#ifndef VECTOR4_DISTV
#define VECTOR4_DISTV
	inline Vector4 Vector4::DistV(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.MagV();
	}
#endif // VECTOR4_DISTV

#ifndef VECTOR4_INVDIST
#define VECTOR4_INVDIST
	inline float Vector4::InvDist(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.InvMag();
	}
#endif // VECTOR4_INVDIST

#ifndef VECTOR4_INVDISTV
#define VECTOR4_INVDISTV
	inline Vector4 Vector4::InvDistV(Vector4Param a) const	
	{
		Vector4 temp = *this - a;
		return temp.InvMagV();
	}
#endif // VECTOR4_INVDISTV

#ifndef VECTOR4_DIST2
#define VECTOR4_DIST2
	inline float Vector4::Dist2(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag2();
	}
#endif // VECTOR4_DIST2

#ifndef VECTOR4_DIST2V
#define VECTOR4_DIST2V
	inline Vector4 Vector4::Dist2V(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag2V();
	}
#endif // VECTOR4_DIST2V

#ifndef VECTOR4_INVDIST2
#define VECTOR4_INVDIST2
	inline float Vector4::InvDist2(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return 1.0f / temp.Mag2();
	}
#endif // VECTOR4_INVDIST2

#ifndef VECTOR4_INVDIST2V
#define VECTOR4_INVDIST2V
    inline Vector4 Vector4::InvDist2V(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		Vector4 ret = temp.Mag2V();
		ret.x = 1.0f / ret.x;
		ret.y = 1.0f / ret.y;
		ret.z = 1.0f / ret.z;
		ret.w = 1.0f / ret.w;
		return ret;
	}
#endif // VECTOR4_INVDIST2V

#ifndef VECTOR4_DIST3
#define VECTOR4_DIST3
	inline float Vector4::Dist3(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag3();
	}
#endif // VECTOR4_DIST3

#ifndef VECTOR4_DIST3V
#define VECTOR4_DIST3V
	inline Vector4 Vector4::Dist3V(Vector4Param a) const	
	{
		Vector4 temp = *this - a;
		return temp.Mag3V();
	}
#endif // VECTOR4_DIST3V

#ifndef VECTOR4_INVDIST3
#define VECTOR4_INVDIST3
	inline float Vector4::InvDist3(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.InvMag3();
	}
#endif // VECTOR4_INVDIST3

#ifndef VECTOR4_INVDIST3V
#define VECTOR4_INVDIST3V
	inline Vector4 Vector4::InvDist3V(Vector4Param a) const	
	{
		Vector4 temp = *this - a;
		return temp.InvMag3V();
	}
#endif // VECTOR4_INVDIST3V

#ifndef VECTOR4_DIST32
#define VECTOR4_DIST32
	inline float Vector4::Dist32(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag32();
	}
#endif // VECTOR4_DIST32

#ifndef VECTOR4_DIST32V
#define VECTOR4_DIST32V
	inline Vector4 Vector4::Dist32V(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return temp.Mag32V();
	}
#endif // VECTOR4_DIST32V

#ifndef VECTOR4_INVDIST32
#define VECTOR4_INVDIST32
	inline float Vector4::InvDist32(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		return 1.0f / temp.Mag32();
	}
#endif // VECTOR4_INVDIST32

#ifndef VECTOR4_INVDIST32V
#define VECTOR4_INVDIST32V
	inline Vector4 Vector4::InvDist32V(Vector4Param a) const
	{
		Vector4 temp = *this - a;
		Vector4 ret = temp.Mag32V();
		ret.x = 1.0f / ret.x;
		ret.y = 1.0f / ret.y;
		ret.z = 1.0f / ret.z;
		ret.w = 1.0f / ret.w;
		return ret;
	}
#endif // VECTOR4_INVDIST32V

#ifndef VECTOR4_FLOAT_TO_INT
#define VECTOR4_FLOAT_TO_INT
	inline void Vector4::FloatToInt()
	{
		ix = (int)x;
		iy = (int)y;
		iz = (int)z;
		iw = (int)w;
	}
#endif // VECTOR4_FLOAT_TO_INT

#ifndef VECTOR4_INT_TO_FLOAT
#define VECTOR4_INT_TO_FLOAT
	inline void Vector4::IntToFloat()
	{
		x = (float)ix;
		y = (float)iy;
		z = (float)iz;
		w = (float)iw;
	}
#endif // VECTOR4_INT_TO_FLOAT

#ifndef VECTOR4_ROUND_TO_NEAREST_INT
#define VECTOR4_ROUND_TO_NEAREST_INT
	inline void Vector4::RoundToNearestInt()
	{
		x = (float)((int)(x + 0.5f));
		y = (float)((int)(y + 0.5f));
		z = (float)((int)(z + 0.5f));
		w = (float)((int)(w + 0.5f));
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_ZERO
#define VECTOR4_ROUND_TO_NEAREST_INT_ZERO
	inline void Vector4::RoundToNearestIntZero()
	{
		x = (float)((int)x);
		y = (float)((int)y);
		z = (float)((int)z);
		w = (float)((int)w);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_ZERO

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF
#define VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF
	inline void Vector4::RoundToNearestIntNegInf()
	{
		x = floorf(x);
		y = floorf(y);
		z = floorf(z);
		w = floorf(w);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_NEG_INF

#ifndef VECTOR4_ROUND_TO_NEAREST_INT_POS_INF
#define VECTOR4_ROUND_TO_NEAREST_INT_POS_INF
	inline void Vector4::RoundToNearestIntPosInf()
	{
		x = ceilf(x);
		y = ceilf(y);
		z = ceilf(z);
		w = ceilf(w);
	}
#endif // VECTOR4_ROUND_TO_NEAREST_INT_POS_INF

#ifndef VECTOR4_ISZERO
#define VECTOR4_ISZERO
	inline bool Vector4::IsZero() const
	{
		return (x == 0 && y == 0 && z == 0 && w == 0);
	}
#endif // VECTOR4_ISZERO

#ifndef VECTOR4_ISNONZERO
#define VECTOR4_ISNONZERO
	inline bool Vector4::IsNonZero() const
	{
		return (x != 0 || y != 0 || z != 0 || w != 0);
	}
#endif // VECTOR4_ISNONZERO

#ifndef VECTOR4_ISEQUAL
#define VECTOR4_ISEQUAL
	inline bool Vector4::IsEqual(Vector4Param a) const
	{
		return ( x == a.x && y == a.y && z == a.z && w == a.w );
	}
#endif // VECTOR4_ISEQUAL

#ifndef VECTOR4_ISEQUALV
#define VECTOR4_ISEQUALV
	inline Vector4 Vector4::IsEqualV(Vector4Param a) const
	{
		Vector4 ret;
		ret.x = (x == a.x ? allBitsF : 0);
		ret.y = (y == a.y ? allBitsF : 0);
		ret.z = (z == a.z ? allBitsF : 0);
		ret.w = (w == a.w ? allBitsF : 0);
		return ret;
	}
#endif // VECTOR4_ISEQUALV

#ifndef VECTOR4_ISEQUALIV
#define VECTOR4_ISEQUALIV
	inline Vector4 Vector4::IsEqualIV(const Vector4& a) const
	{
		Vector4 ret;
		ret.x = (ix == a.ix ? allBitsF : 0);
		ret.y = (iy == a.iy ? allBitsF : 0);
		ret.z = (iz == a.iz ? allBitsF : 0);
		ret.w = (iw == a.iw ? allBitsF : 0);
		return ret;
	}
#endif // VECTOR4_ISEQUALIV

#ifndef VECTOR4_ISNOTEQUAL
#define VECTOR4_ISNOTEQUAL
	inline bool Vector4::IsNotEqual(Vector4Param a) const
	{
		return ( (x != a.x) || (y != a.y) || (z != a.z) || (w != a.w) );
	}
#endif // VECTOR4_ISNOTEQUAL

#ifndef VECTOR4_ISCLOSE
#define VECTOR4_ISCLOSE
	inline bool Vector4::IsClose(const Vector4& a, float eps) const
	{
		return (x>=a.x-eps && x<=a.x+eps) && (y>=a.y-eps && y<=a.y+eps) && (z>=a.z-eps && z<=a.z+eps) && (w>=a.w-eps && w<=a.w+eps);
	}
#endif // VECTOR4_ISCLOSE

#ifndef VECTOR4_ISCLOSE_V
#define VECTOR4_ISCLOSE_V
	inline bool Vector4::IsClose(Vector4Param a, Vector4Param eps) const
	{
		return	( x >= (a.x - eps.x) && x <= (a.x + eps.x) ) && 
				( y >= (a.y - eps.y) && y <= (a.y + eps.y) ) && 
				( z >= (a.z - eps.z) && z <= (a.z + eps.z) ) && 
				( w >= (a.w - eps.w) && w <= (a.w + eps.w) );
	}
#endif // VECTOR4_ISCLOSE_V

#ifndef VECTOR4_ISGREATERTHAN
#define VECTOR4_ISGREATERTHAN
	inline bool Vector4::IsGreaterThan(Vector4Param a) const
	{
		return ( (x > a.x) && (y > a.y) && (z > a.z) && (w > a.w) );
	}
#endif // VECTOR4_ISGREATERTHAN

#ifndef VECTOR4_ISGREATERTHANV
#define VECTOR4_ISGREATERTHANV
	inline Vector4 Vector4::IsGreaterThanV(Vector4Param a) const
	{
		return Vector4((x > a.x) ? allBitsF : 0.0f, (y > a.y) ? allBitsF : 0.0f, (z > a.z) ? allBitsF : 0.0f, (w > a.w) ? allBitsF : 0.0f);
	}
#endif // VECTOR4_ISGREATERTHANV

#ifndef VECTOR4_ISGREATERTHANVR
#define VECTOR4_ISGREATERTHANVR
	inline Vector4 Vector4::IsGreaterThanVR(const Vector4& a, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		bool bw = false;
		Vector4 ret(0.0f, 0.0f, 0.0f, 0.0f);
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
		if( w > a.w )
		{
			ret.w = allBitsF;
			bw = true;
		}
		r = 0;
		if( bx && by && bz && bw )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR4_ISGREATERTHANVR

#ifndef VECTOR4_ISLESSTHAN
#define VECTOR4_ISLESSTHAN
	inline bool Vector4::IsLessThanAll(Vector4Param a) const
	{
		return ( x < a.x && y < a.y && z < a.z && w < a.w );
	}

	inline bool Vector4::IsLessThanDoNotUse(Vector4Param a) const
	{
		return ( x < a.x && y < a.y && z < a.z && w < a.w );
	}
#endif // VECTOR4_ISLESSTHAN

#ifndef VECTOR4_ISLESSTHANV
#define VECTOR4_ISLESSTHANV
	inline Vector4 Vector4::IsLessThanV(Vector4Param a) const
	{
		return Vector4((x < a.x) ? allBitsF : 0.0f, (y < a.y) ? allBitsF : 0.0f, (z < a.z) ? allBitsF : 0.0f, (w < a.w) ? allBitsF : 0.0f);
	}
#endif // VECTOR4_ISLESSTHANV

#ifndef VECTOR4_ISLESSTHANVR
#define VECTOR4_ISLESSTHANVR
	inline Vector4 Vector4::IsLessThanVR(const Vector4& a, u32& r) const
	{
		bool bx = false;
		bool by = false;
		bool bz = false;
		bool bw = false;
		Vector4 ret(0.0f, 0.0f, 0.0f, 0.0f);
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
		if( w < a.w )
		{
			ret.w = allBitsF;
			bw = true;
		}
		r = 0;
		if( bx && by && bz && bw )
		{
			r = VEC3_CMP_VAL;
		}
		return ret;
	}
#endif // VECTOR4_ISLESSTHANVR

#ifndef VECTOR4_SELECT
#define VECTOR4_SELECT
	inline Vector4 Vector4::Select(Vector4Param zero, Vector4Param nonZero) const
	{
#if __PS3
		return Vector4(__vsel(zero.xyzw, nonZero.xyzw, uxyzw));
#else
		Vector4 ret;
		u32* pThisPtr = (u32*)&x;
		u32* pZeroPtr = (u32*)&zero;
		u32* pOnePtr = (u32*)&nonZero;
		u32* pOutPtr = (u32*)&ret;
        
		for( int i = 0; i < 4; i++ )
		{
			u32 mask = 0x80000000;
			u32 inval = pThisPtr[i];
			u32 zeroval = pZeroPtr[i];
			u32 oneval = pOnePtr[i];
			u32 outval = 0;
			for( int j = 0; j < 32; j++ )
			{
				if( inval & mask )
				{
					outval |= (oneval & mask);
				}
				else
				{
					outval |= (zeroval & mask);
				}
				mask >>= 1;
			}
			pOutPtr[i] = outval;
		}
		return ret;
#endif
	}
#endif // VECTOR4_SELECT

#ifndef VECTOR4_MAX
#define VECTOR4_MAX
	inline void Vector4::Max(Vector4Param a, Vector4Param b)
	{
		x = a.x > b.x ? a.x : b.x;
		y = a.y > b.y ? a.y : b.y;
		z = a.z > b.z ? a.z : b.z;
		w = a.w > b.w ? a.w : b.w;
	}
#endif // VECTOR4_MAX

#ifndef VECTOR4_MIN
#define VECTOR4_MIN
	inline void Vector4::Min(Vector4Param a, Vector4Param b)
	{
		x = a.x < b.x ? a.x : b.x;
		y = a.y < b.y ? a.y : b.y;
		z = a.z < b.z ? a.z : b.z;
		w = a.w < b.w ? a.w : b.w;
	}
#endif // VECTOR4_MIN

#ifndef VECTOR4_COMPUTEPLANE_VVV
#define VECTOR4_COMPUTEPLANE_VVV
	inline void Vector4::ComputePlane(Vector3::Param a,Vector3::Param b,Vector3::Param c)
	{
		Vector3 norm;
		Vector3 ab = b - a;
		Vector3 ac = c - a;
		norm.Cross(ab, ac);
		norm.Normalize();

		x = norm.x;
		y = norm.y;
		z = norm.z;
		w = norm.Dot(a);
	}
#endif // VECTOR4_COMPUTEPLANE_VVV

#ifndef VECTOR4_COMPUTEPLANE_VV
#define VECTOR4_COMPUTEPLANE_VV
	inline void Vector4::ComputePlane(const Vector3 &position, const Vector3 & direction)
	{
		x = direction.x;
		y = direction.y;
		z = direction.z;
		w = direction.Dot(position);
	}
#endif // VECTOR4_COMPUTEPLANE_VV

#ifndef VECTOR4_DISTANCETOPLANE
#define VECTOR4_DISTANCETOPLANE
	inline float Vector4::DistanceToPlane(Vector3Param a) const
	{
		return Dot3(a) - w;
	}
#endif // VECTOR4_DISTANCETOPLANE

#ifndef VECTOR4_DISTANCETOPLANEV3
#define VECTOR4_DISTANCETOPLANEV3
	inline Vector4 Vector4::DistanceToPlaneV(Vector3Param a) const
	{
//		Vector3 vW;
//		Vector3 vNrm(x, y, z);
//		vW.Set(w);
//		return vNrm.DotV(a) - vW;
		Vector4 vW;
		vW.SplatW(*this);
		return Dot3V(a) - vW;
	}
#endif // VECTOR4_DISTANCETOPLANEV3

#ifndef VECTOR4_DISTANCETOPLANEV4
#define VECTOR4_DISTANCETOPLANEV4
	inline Vector4 Vector4::DistanceToPlaneV(Vector4Param a) const
	{
		Vector4 vW;
		vW.SplatW(*this);
		return Dot3V(a) - vW;
	}
#endif // VECTOR4_DISTANCETOPLANEV4

#ifndef VECTOR4_AND
#define VECTOR4_AND
	inline void Vector4::And(Vector4Param _and)
	{
#if __PS3
		xyzw = __vand(xyzw, _and.xyzw);
#else
		u32* pThisPtr = (u32*)&x;
		u32* pInPtr = (u32*)&_and;

		for( int i = 0; i < 4; i++ )
			pThisPtr[i] &= pInPtr[i];
#endif
	}
#endif // VECTOR4_AND

#ifndef VECTOR4_OR
#define VECTOR4_OR
	inline void Vector4::Or(Vector4Param _or)
	{
#if __PS3
		xyzw = __vor(xyzw, _or.xyzw);
#else
		u32* pThisPtr = (u32*)&x;
		u32* pInPtr = (u32*)&_or;

		for( int i = 0; i < 4; i++ )
			pThisPtr[i] |= pInPtr[i];
#endif
	}
#endif // VECTOR4_OR

#ifndef VECTOR4_XOR
#define VECTOR4_XOR
	inline void Vector4::Xor(Vector4Param _xor)
	{
#if __PS3
		xyzw = __vxor(xyzw, _xor.xyzw);
#else
		u32* pThisPtr = (u32*)&x;
		u32* pInPtr = (u32*)&_xor;

		for( int i = 0; i < 4; i++ )
			pThisPtr[i] ^= pInPtr[i];
#endif
	}
#endif // VECTOR4_XOR

#ifndef VECTOR4_MERGEXY
#define VECTOR4_MERGEXY
	inline void Vector4::MergeXY(Vector4Param vY)
	{
		z = y;
		y = vY.x;
		w = vY.y;

	}
#endif // VECTOR4_MERGEXY

#ifndef VECTOR4_MERGEXY_V
#define VECTOR4_MERGEXY_V
	inline void Vector4::MergeXY(Vector4Param vX, Vector4Param vY)
	{
		x = vX.x;
		y = vY.x;
		z = vX.y;
		w = vY.y;
	}
#endif // VECTOR4_MERGEXY_V

#ifndef VECTOR4_MERGEZW
#define VECTOR4_MERGEZW
	inline void Vector4::MergeZW(Vector4Param vW)
	{
		x = z;
		y = vW.z;
		z = w;
		w = vW.w;
	}
#endif // VECTOR4_MERGEZW

#ifndef VECTOR4_MERGEZW_V
#define VECTOR4_MERGEZW_V
	inline void Vector4::MergeZW(Vector4Param vZ, Vector4Param vW)
	{
		x = vZ.z;
		y = vW.z;
		z = vZ.w;
		w = vW.w;
	}
#endif // VECTOR4_MERGEZW_V

#ifndef VECTOR4_PERMUTE
#define VECTOR4_PERMUTE
	template <int permX, int permY, int permZ, int permW>
	inline void Vector4::Permute(Vector4Param v)
	{
		Vector4 temp(v);
		x = temp[permX];
		y = temp[permY];
		z = temp[permZ];
		w = temp[permW];
	}
#endif // VECTOR4_PERMUTE

#ifndef VECTOR4_OPERATOR_ASSIGN
#define VECTOR4_OPERATOR_ASSIGN
	inline Vector4& Vector4::operator=(const Vector4& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
		w = a.w;
		return *this;
	}
#endif // VECTOR4_OPERATOR_ASSIGN

#ifndef VECTOR4_OPERATOR_TESTEQUAL
#define VECTOR4_OPERATOR_TESTEQUAL
	inline bool Vector4::operator==(Vector4Param a) const
	{
		return IsEqual(a);
	}
#endif // VECTOR4_OPERATOR_TESTEQUAL

#ifndef VECTOR4_OPERATOR_NOTEQUAL
#define VECTOR4_OPERATOR_NOTEQUAL
	inline bool Vector4::operator!=(Vector4Param a) const
	{
		return IsNotEqual(a);
	}
#endif // VECTOR4_OPERATOR_NOTEQUAL

#ifndef VECTOR4_OPERATOR_PLUS
#define VECTOR4_OPERATOR_PLUS
	inline Vector4 Vector4::operator+(Vector4Param a) const
	{
		Vector4 ret;
		ret.Add(*this, a);
		return ret;
	}
#endif // VECTOR4_OPERATOR_PLUS

#ifndef VECTOR4_OPERATOR_MINUS
#define VECTOR4_OPERATOR_MINUS
	inline Vector4 Vector4::operator-(Vector4Param a) const
	{
		Vector4 ret;
		ret.Subtract(*this, a);
		return ret;
	}
#endif // VECTOR4_OPERATOR_MINUS

#ifndef VECTOR4_OPERATOR_NEGATE
#define VECTOR4_OPERATOR_NEGATE
	inline Vector4 Vector4::operator-() const
	{
		Vector4 ret = *this;
		ret.Negate();
		return ret;
	}
#endif // VECTOR4_OPERATOR_NEGATE

#ifndef VECTOR4_OPERATOR_MUL_F
#define VECTOR4_OPERATOR_MUL_F
	inline Vector4 Vector4::operator*(const float f) const
	{
		Vector4 ret;
		ret.Scale(*this, f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_MUL_F

#ifndef VECTOR4_OPERATOR_MUL_V
#define VECTOR4_OPERATOR_MUL_V
	inline Vector4 Vector4::operator*(Vector4Param f) const
	{
		Vector4 ret;
		ret.Multiply(*this, f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_MUL_V

#ifndef VECTOR4_OPERATOR_DIV_F
#define VECTOR4_OPERATOR_DIV_F
	inline Vector4 Vector4::operator/(const float f) const
	{
		Vector4 ret;
		ret.InvScale(*this, f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_DIV_F

#ifndef VECTOR4_OPERATOR_DIV_V
#define VECTOR4_OPERATOR_DIV_V
	inline Vector4 Vector4::operator/(Vector4Param f) const
	{
		Vector4 ret;
		ret.InvScale((Vector4Param)*this, f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_DIV_V

#ifndef VECTOR4_OPERATOR_OR
#define VECTOR4_OPERATOR_OR
	inline Vector4 Vector4::operator|(Vector4Param f) const
	{
		Vector4 ret = *this;
		ret.Or(f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_OR

#ifndef VECTOR4_OPERATOR_AND
#define VECTOR4_OPERATOR_AND
	inline Vector4 Vector4::operator&(Vector4Param f) const
	{
		Vector4 ret = *this;
		ret.And(f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_AND

#ifndef VECTOR4_OPERATOR_XOR
#define VECTOR4_OPERATOR_XOR
	inline Vector4 Vector4::operator^(Vector4Param f) const
	{
		Vector4 ret = *this;
		ret.Xor(f);
		return ret;
	}
#endif // VECTOR4_OPERATOR_XOR

#ifndef VECTOR4_OPERATOR_PLUSEQUAL
#define VECTOR4_OPERATOR_PLUSEQUAL
	inline void Vector4::operator+=(Vector4Param V)
	{
		Add(V);
	}
#endif // VECTOR4_OPERATOR_PLUSEQUAL

#ifndef VECTOR4_OPERATOR_MINUSEQUAL
#define VECTOR4_OPERATOR_MINUSEQUAL
	inline void Vector4::operator-=(Vector4Param V)	
	{
		Subtract(V);
	}
#endif // VECTOR4_OPERATOR_MINUSEQUAL

#ifndef VECTOR4_OPERATOR_TIMESEQUAL
#define VECTOR4_OPERATOR_TIMESEQUAL
	inline void Vector4::operator*=(const float f)
	{
		Scale(f);
	}
#endif // VECTOR4_OPERATOR_TIMESEQUAL

#ifndef VECTOR4_OPERATOR_TIMESEQUAL_V
#define VECTOR4_OPERATOR_TIMESEQUAL_V
	inline void Vector4::operator*=(Vector4Param f)	
	{
		Multiply(f);
	}
#endif // VECTOR4_OPERATOR_TIMESEQUAL_V

#ifndef VECTOR4_OPERATOR_DIVEQUAL_F
#define VECTOR4_OPERATOR_DIVEQUAL_F
	inline void Vector4::operator/=(const float f)
	{
		InvScale(f);
	}
#endif // VECTOR4_OPERATOR_DIVEQUAL_F

#ifndef VECTOR4_OPERATOR_DIVEQUAL_V
#define VECTOR4_OPERATOR_DIVEQUAL_V
	inline void Vector4::operator/=(Vector4Param f)
	{
		InvScale(f);
	}
#endif // VECTOR4_OPERATOR_DIVEQUAL_V

#ifndef VECTOR4_OPERATOR_OREQUAL
#define VECTOR4_OPERATOR_OREQUAL
	inline void Vector4::operator|=(Vector4Param f)
	{
		Or(f);
	}
#endif // VECTOR4_OPERATOR_OREQUAL

#ifndef VECTOR4_OPERATOR_ANDEQUAL
#define VECTOR4_OPERATOR_ANDEQUAL
	inline void Vector4::operator&=(Vector4Param f)
	{
		And(f);
	}
#endif // VECTOR4_OPERATOR_ANDEQUAL

#ifndef VECTOR4_OPERATOR_XOREQUAL
#define VECTOR4_OPERATOR_XOREQUAL
	inline void Vector4::operator^=(Vector4Param f)
	{
		Xor(f);
	}
#endif // VECTOR4_OPERATOR_XOREQUAL

#ifndef VECTOR4_OPERATOR_BRACKET_C
#define VECTOR4_OPERATOR_BRACKET_C
	inline const float& Vector4::operator[](int i) const
	{
		const float* pFloats = (const float*)&x;
		return pFloats[i];
	}
#endif // VECTOR4_OPERATOR_BRACKET_C

#ifndef VECTOR4_OPERATOR_BRACKET
#define VECTOR4_OPERATOR_BRACKET
	inline float& Vector4::operator[](int i)
	{
		float* pFloats = (float*)&x;
		return pFloats[i];
	}
#endif // VECTOR4_OPERATOR_BRACKET

#ifndef CONVERT_VECTOR3_TO_VECTOR4
#define CONVERT_VECTOR3_TO_VECTOR4
inline Vector4& Convert(Vector4& dest, Vector4::Vector3Param src)
{
	dest.x = src.x;
	dest.y = src.y;
	dest.z = src.z;
	dest.w = 1.0f;
	return dest;
}
#endif // CONVERT_VECTOR3_TO_VECTOR4

#ifndef CONVERT_VECTOR4_TO_VECTOR3
#define CONVERT_VECTOR4_TO_VECTOR3
inline Vector3& Convert(Vector3&dest,const Vector4& src)
{
	// To be absolutely correct, we really ought to divide through by W
	dest.x = src.x;
	dest.y = src.y;
	dest.z = src.z;
	return dest;
}
#endif // CONVERT_VECTOR4_TO_VECTOR3

#ifndef CONVERT_VECTOR4_TO_VECTOR3_FULL
#define CONVERT_VECTOR4_TO_VECTOR3_FULL
inline Vector3& ConvertFull(Vector3&dest,const Vector4& src)
{
	float iw = 1.0f / src.w;
	dest.x = src.x * iw;
	dest.y = src.y * iw;
	dest.z = src.z * iw;
	return dest;
}
#endif

} // namespace rage

#endif // VECTOR_VECTOR4_DEFAULT_H
