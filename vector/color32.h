//
// vector/color32.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_COLOR32_H
#define VECTOR_COLOR32_H

#include "data/resource.h"
#include "data/serialize.h"
#include "data/struct.h"
#include "grcore/config.h"
#include "system/codecheck.h"
#include "system/platform.h"

#include "math/amath.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#define COLOR32_INITIALIZE_DEFAULT_CONSTRUCTOR (!HACK_MC4)

namespace rage {

//=============================================================================
// Color32
//
// PURPOSE:	Provides an abstraction of a packed color value.
// NOTES:		The color value is platform-independent, 0xaarrggbb, which also matches
//				the pixel format of our grcImage class and the D3DCOLOR definition.
// <FLAG Component>
//
class Color32
{
public:
	// PURPOSE:	Default constructor.  Sets color to opaque white.
	inline Color32();

	// PURPOSE: Resource constructor, for color32s created from a resource
	// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
	inline Color32(datResource& ) {}

	// PURPOSE:	Constructor
	// PARAMS:
	//	color - the color to copy, in 0xaarrggbb format.
	// NOTES:
	//	this constructor is declared as explicit to prevent the user from unintentionally 
	//  passing an int to a function that takes a Color32.
	explicit inline Color32(u32 color);

	// PURPOSE:	Constructor
	// PARAMS:
	//	r -	red component (0 to 255)
	//	g -	green component (0 to 255)
	//	b -	blue component (0 to 255)
	//	a - optional alpha component (0 to 255)
	inline Color32(int r, int g, int b, int a = 255);

	// PURPOSE:	Constructor
	// PARAMS:
	//	r -	red component (0 to 255)
	//	g -	green component (0 to 255)
	//	b -	blue component (0 to 255)
	//	a - optional alpha component (0 to 255)
	inline Color32(u32 r, u32 g, u32 b, u32 a = 255);

	// PURPOSE:	Constructor
	// PARAMS:
	//	r -	red component (0 to 1)
	//	g -	green component (0 to 1)
	//	b -	blue component (0 to 1)
	//	a - optional alpha component (0 to 1)
	inline Color32(float r, float g, float b, float a = 1.0f);

	inline Color32(Vec3V_In color); // note - these constructors clamp the inputs to 0..1
	inline Color32(Vec4V_In color);

	// PURPOSE:	Constructor
	// PARAMS:
	//	color -	vector holding red, green and blue components as floats
	// NOTES:	The default value is used for alpha.
	inline Color32(const Vector3& color);

	// PURPOSE:	Constructor
	// PARAMS:
	//	color -	vector holding red, green, blue and alpha components as floats
	inline Color32(const Vector4& color);

	// PURPOSE
	//	Set the color based on a u32 with the 0xaarrggbb format.
	// PARAMS
	//	color - the color to copy from (must be in 0xaarrggbb format)
	inline void Set(u32 color);

	// PURPOSE: Sets color value defined by integers.
	// PARAMS:
	//	r -	red component (0 to 255)
	//	g -	green component (0 to 255)
	//	b -	blue component (0 to 255)
	//	a - optional alpha component (0 to 255)
	inline void Set(int r, int g, int b, int a = 255);

	// PURPOSE: Sets color value defined by floats.
	// PARAMS:
	//	r -	red component (0 to 1)
	//	g -	green component (0 to 1)
	//	b -	blue component (0 to 1)
	//	a - optional alpha component (0 to 1)
	inline void Setf(float r, float g, float b, float a = 1.0f);

	// PURPOSE:	Sets color value defined by bytes
	// PARAMS:
	//	r -	red component (0 to 255)
	//	g -	green component (0 to 255)
	//	b -	blue component (0 to 255)
	//	a - optional alpha component (0 to 255)
	// NOTES: slightly faster than Set because it doesn't need to clamp its input
	inline void SetBytes(u8 r, u8 g, u8 b, u8 a = 255);

	// PURPOSE: Get the component defined by an integer (0 to 255)
	// RETURNS:	component of color
	inline u8 GetRed() const;
	inline u8 GetGreen() const;
	inline u8 GetBlue() const;
	inline u8 GetAlpha() const;

	// PURPOSE: Get the component defined by a float (0 to 1)
	// RETURNS:	component of color
	inline float GetRedf() const;
	inline float GetGreenf() const;
	inline float GetBluef() const;
	inline float GetAlphaf() const;

	// PURPOSE: Sets component directly (without clamping)
	// PARAMS:
	//	r,g,b,a - component (0 to 255)
	inline void SetRed(int r);
	inline void SetGreen(int g);
	inline void SetBlue(int b);
	inline void SetAlpha(int a);

	inline Vec4V_Out GetRGBA() const;
	inline Vec4V_Out GetARGB() const; // (This one's a little cheaper than GetRGBA(), as this is the order that the packed color is stored in.)
	inline Vec3V_Out GetRGB() const { return GetRGBA().GetXYZ(); }

	inline void SetFromRGBA( Vec4V_In rgbaVec ); // note - you must ensure the input values are 0..1
	inline void SetFromARGB( Vec4V_In argbVec );

	// PURPOSE:	Get the packed color value.
	// RETURNS: Platform-independent packed color value
	inline u32 GetColor() const;

	// PURPOSE: Get the platform dependent packed color value
	// RETURNS: Platform-dependent packed color value
	inline u32 GetDeviceColor() const;

	// PURPOSE: Set color from platform dependent packed color value
	inline void SetFromDeviceColor(u32 colorDC);

	Color32 operator*(const Color32& that) const;

	inline void operator+=(const Color32& that);

	// PURPOSE: Determine if two colors are equal.
	// RETURNS: true if the given color equals this color, false if it does not
	inline bool operator==(const Color32& that) const;

	// PURPOSE: Determine if two colors are not equal
	// RETURNS: true if the given color does not equal this color, false if it does
	inline bool operator!=(const Color32& that) const;

	inline FASTRETURNCHECK(Color32) MergeAlpha(u8 newAlpha) const;
	inline FASTRETURNCHECK(Color32) MultiplyAlpha(u8 alpha) const;

	static const int s_ChannelShiftA = 24;
	static const int s_ChannelShiftR = 16;
	static const int s_ChannelShiftG = 8;
	static const int s_ChannelShiftB = 0;

	static const u32 s_ChannelMaskA = 0xFF000000;
	static const u32 s_ChannelMaskR = 0x00FF0000;
	static const u32 s_ChannelMaskG = 0x0000FF00;
	static const u32 s_ChannelMaskB = 0x000000FF;
	
	static const u32 s_ChannelMaskRGB = s_ChannelMaskR | s_ChannelMaskG | s_ChannelMaskB;

private:
	friend datSerialize & operator<< (datSerialize &s, Color32 &c);
	u32 m_Color;

public:
	static const int RORC_VERSION = 1;
	DECLARE_PLACE(Color32);
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s)
	{
		STRUCT_BEGIN(Color32);
		STRUCT_FIELD(m_Color);
		STRUCT_END();
	}
#endif // __DECLARESTRUCT
};

// Specialization of Lerp template from amath.h
//template<>
inline Color32 Lerp(float t, Color32 a, Color32 b)
{
	return Color32(
		(u8)Lerp(t, (int)a.GetRed  (), (int)b.GetRed  ()),
		(u8)Lerp(t, (int)a.GetGreen(), (int)b.GetGreen()),
		(u8)Lerp(t, (int)a.GetBlue (), (int)b.GetBlue ()),
		(u8)Lerp(t, (int)a.GetAlpha(), (int)b.GetAlpha())
	);
}

// PURPOSE: Serialize color data
inline datSerialize & operator<< (datSerialize &s, Color32 &c) {
	s << c.m_Color;
	return s;
}

//=============================================================================
// Implementations

namespace Color32_util {

//static __forceinline float Clamp_0_1(float f)
//{
//	return f<0? 0 : f>1? 1 : f;
//}

static __forceinline u8 Clamp_0_255(int i)
{
	return i<0? 0 : i>255? 255 : u8(i);
}

} // namespace Color32_util

__forceinline  Color32::Color32()
{
#if COLOR32_INITIALIZE_DEFAULT_CONSTRUCTOR
	Set(255,255,255,255);
#endif
}

__forceinline  Color32::Color32(u32 color)
{
	m_Color = color;
}

__forceinline  Color32::Color32(int r, int g, int b, int a)
{
	Set(r,g,b,a);
}

__forceinline  Color32::Color32(u32 r, u32 g, u32 b, u32 a)
{
	Set(r,g,b,a);
}

__forceinline  Color32::Color32(float r, float g, float b, float a)
{
	Setf(r,g,b,a);
}

__forceinline  Color32::Color32(Vec3V_In color)
{
	//this function auto clamps the color
	SetFromRGBA( Vec4V(color, ScalarV(V_ONE)) );
}

__forceinline  Color32::Color32(Vec4V_In color)
{
	//this function auto clamps the color
	SetFromRGBA( color );
}

__forceinline  Color32::Color32(const Vector3& color)
{
	//this function auto clamps the color
	const Vec4V colorV = Vec4V(RCC_VEC3V(color),ScalarV(V_ONE));
	SetFromRGBA( colorV );
}

__forceinline Color32::Color32(const Vector4& color)
{
	//this function auto clamps the color
	const Vec4V colorV =  RCC_VEC4V(color);
	SetFromRGBA( colorV );
}

__forceinline void Color32::Set(u32 color)
{
	m_Color = color;
}

__forceinline void Color32::Set(int r, int g, int b, int a)
{
	SetBytes(Color32_util::Clamp_0_255(r), Color32_util::Clamp_0_255(g), Color32_util::Clamp_0_255(b), Color32_util::Clamp_0_255(a));
}

__forceinline void Color32::Setf(float r, float g, float b, float a)
{
	Set(int(r * 255.0f), int(g * 255.0f), int(b * 255.0f), int(a * 255.0f));
}

__forceinline void Color32::SetBytes(u8 r, u8 g, u8 b, u8 a)
{
	m_Color = (r << s_ChannelShiftR) | (g << s_ChannelShiftG) | (b << s_ChannelShiftB) | (a << s_ChannelShiftA);
}

__forceinline  u8 Color32::GetRed  () const { return u8(m_Color >> s_ChannelShiftR); }
__forceinline  u8 Color32::GetGreen() const { return u8(m_Color >> s_ChannelShiftG); }
__forceinline  u8 Color32::GetBlue () const { return u8(m_Color >> s_ChannelShiftB); }
__forceinline  u8 Color32::GetAlpha() const { return u8(m_Color >> s_ChannelShiftA); }

__forceinline  float Color32::GetRedf  () const { return GetRed  () * (1.0f / 255.0f); }
__forceinline  float Color32::GetGreenf() const { return GetGreen() * (1.0f / 255.0f); }
__forceinline  float Color32::GetBluef () const { return GetBlue () * (1.0f / 255.0f); }
__forceinline  float Color32::GetAlphaf() const { return GetAlpha() * (1.0f / 255.0f); }

__forceinline  void Color32::SetRed  (int r) { m_Color = (m_Color & ~s_ChannelMaskR) | (u8(r) << s_ChannelShiftR); }
__forceinline  void Color32::SetGreen(int g) { m_Color = (m_Color & ~s_ChannelMaskG) | (u8(g) << s_ChannelShiftG); }
__forceinline  void Color32::SetBlue (int b) { m_Color = (m_Color & ~s_ChannelMaskB) | (u8(b) << s_ChannelShiftB); }
__forceinline  void Color32::SetAlpha(int a) { m_Color = (m_Color & ~s_ChannelMaskA) | (u8(a) << s_ChannelShiftA); }

__forceinline  Vec4V_Out Color32::GetRGBA() const
{
	return GetARGB().Get<Vec::Y,Vec::Z,Vec::W,Vec::X>();
}

inline Vec4V_Out Color32::GetARGB() const
{
	Vec4V colorOut = Vec4VFromU32( m_Color ); // Stored as <ARGB,ARGB,ARGB,ARGB>
	Vec4V _zero(V_ZERO);

#if !RSG_CPU_INTEL
	Vec4V _0a0r_0g0b_0a0r_0g0b = MergeXYByte( _zero, colorOut );
	Vec4V _000a_000r_000g_000b = MergeXYShort( _zero, _0a0r_0g0b_0a0r_0g0b );
#else
	Vec4V _0a0r_0g0b_0a0r_0g0b = MergeXYByte( colorOut, _zero );
	Vec4V _000a_000r_000g_000b = MergeXYShort( _0a0r_0g0b_0a0r_0g0b, _zero );
	_000a_000r_000g_000b = _000a_000r_000g_000b.Get<Vec::W,Vec::Z,Vec::Y,Vec::X>();
#endif

	// Convert to float (note that we could divide by 256.0f for free, if desired (set template param = 8)... but not 255.0f)
	colorOut = IntToFloatRaw<0>( _000a_000r_000g_000b );

	// Divide by 255.0f (multiply by 1.0f/255.0f)
	colorOut *= ScalarVConstant<U32_ONE_OVER_255>();

	return colorOut;
}


inline void Color32_StoreFromARGB(u32 *out, Vec4V_In argbVec)
{
	Vec4V colorIn = argbVec;

	// Multiply by 255.0f
	Vec4V twoFiftyFive(Vec::V4VConstantSplat<U32_255>());
	Vec4V zero(V_ZERO);
	colorIn *= twoFiftyFive;
	// Clamping between zero and 255
	colorIn = Clamp(colorIn, zero, twoFiftyFive);

	// Convert to int
	colorIn = FloatToIntRaw<0>( colorIn );

	const u8 a = (u8)(((const u32*)(&colorIn))[0]);
	const u8 r = (u8)(((const u32*)(&colorIn))[1]);
	const u8 g = (u8)(((const u32*)(&colorIn))[2]);
	const u8 b = (u8)(((const u32*)(&colorIn))[3]);
	*out = (r << Color32::s_ChannelShiftR) | (g << Color32::s_ChannelShiftG) | (b << Color32::s_ChannelShiftB) | (a << Color32::s_ChannelShiftA);
}

inline void Color32_StoreFromRGBA(u32 *out, Vec4V_In rgbaVec)
{
	Vec4V colorIn = rgbaVec;

	// Multiply by 255.0f
	Vec4V twoFiftyFive(Vec::V4VConstantSplat<U32_255>());
	Vec4V zero(V_ZERO);
	colorIn *= twoFiftyFive;
	// Clamping between zero and 255
	colorIn = Clamp(colorIn, zero, twoFiftyFive);

	// Convert to int
	colorIn = FloatToIntRaw<0>( colorIn );

	const u8 a = (u8)(((const u32*)(&colorIn))[3]);
	const u8 r = (u8)(((const u32*)(&colorIn))[0]);
	const u8 g = (u8)(((const u32*)(&colorIn))[1]);
	const u8 b = (u8)(((const u32*)(&colorIn))[2]);
	*out = (r << Color32::s_ChannelShiftR) | (g << Color32::s_ChannelShiftG) | (b << Color32::s_ChannelShiftB) | (a << Color32::s_ChannelShiftA);
}

inline void Color32_StoreDeviceFromARGB(u32 *out, Vec4V_In argbVec)
{
	Vec4V colorIn = argbVec;

	Vec4V twoFiftyFive(Vec::V4VConstantSplat<U32_255>());
	Vec4V zero(V_ZERO);
	colorIn *= twoFiftyFive;
	colorIn = Clamp(colorIn, zero, twoFiftyFive);

	colorIn = FloatToIntRaw<0>( colorIn );

	const u8 a = (u8)(((const u32*)(&colorIn))[0]);
	const u8 r = (u8)(((const u32*)(&colorIn))[1]);
	const u8 g = (u8)(((const u32*)(&colorIn))[2]);
	const u8 b = (u8)(((const u32*)(&colorIn))[3]);
	*out = (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

inline void Color32_StoreDeviceFromRGBA(u32 *out, Vec4V_In rgbaVec)
{
	Vec4V colorIn = rgbaVec;

	Vec4V twoFiftyFive(Vec::V4VConstantSplat<U32_255>());
	Vec4V zero(V_ZERO);
	colorIn *= twoFiftyFive;
	colorIn = Clamp(colorIn, zero, twoFiftyFive);

	colorIn = FloatToIntRaw<0>( colorIn );

	const u8 r = (u8)(((const u32*)(&colorIn))[0]);
	const u8 g = (u8)(((const u32*)(&colorIn))[1]);
	const u8 b = (u8)(((const u32*)(&colorIn))[2]);
	const u8 a = (u8)(((const u32*)(&colorIn))[3]);
	*out = (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

inline void Color32::SetFromRGBA( Vec4V_In rgbaVec )
{
	//Save a few cycle by skipping the permute instruction
	//SetFromARGB( rgbaVec.Get<Vec::W,Vec::X,Vec::Y,Vec::Z>() );
	Color32_StoreFromRGBA( &m_Color, rgbaVec );
}
inline void Color32::SetFromARGB( Vec4V_In argbVec )
{
	Color32_StoreFromARGB( &m_Color, argbVec );
}

__forceinline  u32 Color32::GetColor() const
{
	return m_Color;
}

inline u32 Color32::GetDeviceColor() const
{
#if __RESOURCECOMPILER
	if (!(g_sysPlatform == platform::PSP2))
	{
		return (GetRed()) | (GetGreen() << 8) | (GetBlue() << 16) | (GetAlpha() << 24);
	}
	else
	{
		return GetColor();		
	}
#else
	#if !__PSP2
		return (GetRed()) | (GetGreen() << 8) | (GetBlue() << 16) | (GetAlpha() << 24);
	#else
		return GetColor();
	#endif
#endif
}

inline void Color32::SetFromDeviceColor(u32 colorDC)
{
#if RSG_CPU_INTEL
	#if __RESOURCECOMPILER
		if(!(g_sysPlatform == platform::PSP2))
		{	// ABGR incoming to ARGB in m_Color
			Set( (colorDC & 0x000000ff), 
				 (colorDC & 0x0000ff00) >> 8, 
				 (colorDC & 0x00ff0000) >> 16, 
				 (colorDC & 0xff000000) >> 24 );
		}
		else
		{
			Set(colorDC);
		}
	#else
		// ABGR incoming to ARGB in m_Color
		#if !__PSP2
			Set( (colorDC & 0x000000ff), 
				 (colorDC & 0x0000ff00) >> 8, 
				 (colorDC & 0x00ff0000) >> 16, 
				 (colorDC & 0xff000000) >> 24 );
		#else
			Set(colorDC);
		#endif
	#endif
#else
	Set(colorDC);
#endif
}

inline Color32 Color32::operator*(const Color32& that) const
{
	return Color32(
		(GetRed() * that.GetRed()) / 255,
		(GetGreen() * that.GetGreen()) / 255,
		(GetBlue() * that.GetBlue()) / 255,
		(GetAlpha() * that.GetAlpha()) / 255);
}

inline void Color32::operator+=(const Color32& that)
{
	Set(GetRed() + that.GetRed(),
		GetGreen() + that.GetGreen(),
		GetBlue() + that.GetBlue(),
		GetAlpha() + that.GetAlpha());
}

inline bool Color32::operator==(const Color32& that) const
{
	return (this->m_Color == that.m_Color);	
}

inline bool Color32::operator!=(const Color32& that) const
{
	return (this->m_Color != that.m_Color);	
}

inline FASTRETURNCHECK(Color32) Color32::MergeAlpha(u8 newAlpha) const
{
	return Color32((m_Color & s_ChannelMaskRGB) | (newAlpha << s_ChannelShiftA));
}

inline FASTRETURNCHECK(Color32) Color32::MultiplyAlpha(u8 alpha) const
{
	const u16 a = (u16)alpha;
	const u16 b = (u16)GetAlpha();
	return MergeAlpha((a*b)>>8);
}

} // namespace rage

#endif // VECTOR_COLOR32_H
