//
// vector/Color64.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_COLOR64_H
#define VECTOR_COLOR64_H

#include "data/serialize.h"

namespace rage {

class Vector3;
class Vector4;


//=============================================================================
// Color64
//
// PURPOSE:	Provides an abstraction of a packed color value.
// NOTES:		The color value is platform-independent, 0xaarrggbb, which also matches
//				the pixel format of our grcImage class and the D3DCOLOR definition.
// <FLAG Component>
//
class Color64
{
public:
	// PURPOSE:	Constructor
	// PARAMS:
	//	color - the color to copy, in 0xaarrggbb format.
	// NOTES:
	//	this constructor is declared as explicit to prevent the user from unintentionally 
	//  passing an int to a function that takes a Color64.
	explicit inline Color64(u64 color);

	// PURPOSE:	Constructor
	// PARAMS:
	//	r -	red component (0 to 65535)
	//	g -	green component (0 to 65535)
	//	b -	blue component (0 to 65535)
	//	a - optional alpha component (0 to 65535)
	inline Color64(int r,int g,int b,int a = 65535);

	// PURPOSE:	Constructor
	// PARAMS:
	//	r -	red component (0 to 1)
	//	g -	green component (0 to 1)
	//	b -	blue component (0 to 1)
	//	a - optional alpha component (0 to 1)
	inline Color64(float r,float g,float b,float a = 1.0f);

	// PURPOSE:	Constructor
	// PARAMS:
	//	color -	vector holding red, green and blue components as floats
	// NOTES:	The default value is used for alpha.
	Color64(const Vector3& color);

	// PURPOSE:	Constructor
	// PARAMS:
	//	color -	vector holding red, green, blue and alpha components as floats
	Color64(const Vector4& color);

	//
	// PURPOSE
	//	Set the color based on a u64 with the 0xaarrggbb format.
	// PARAMS
	//	color - the color to copy from (must be in 0xaarrggbb format)
	//
	inline void Set(u64 color);

	// PURPOSE: Sets color value defined by integers.
	// PARAMS:
	//	r -	red component (0 to 65535)
	//	g -	green component (0 to 65535)
	//	b -	blue component (0 to 65535)
	//	a - optional alpha component (0 to 65535)
	void Set(int r,int g,int b,int a = 65535);

	// PURPOSE: Sets color value defined by floats.
	// PARAMS:
	//	r -	red component (0 to 1)
	//	g -	green component (0 to 1)
	//	b -	blue component (0 to 1)
	//	a - optional alpha component (0 to 1)
	inline void Setf(float r,float g,float b,float a = 1.0f);

	// PURPOSE: Get the red component defined by an integer (0 to 65535)
	// RETURNS:	Red component of color
	inline u16 GetRed() const;

	// PURPOSE: Get the red component defined by a float (0 to 1)
	// RETURNS:	Red component of color
	inline float GetRedf() const;

	// PURPOSE: Sets red component directly
	// PARAMS:
	//	r -	red component (0 to 65535)
	inline void SetRed(int r);

	// PURPOSE: Get the green component defined by an integer (0 to 65535)
	// RETURNS: Green component of color
	inline u16 GetGreen() const;

	// PURPOSE: Get the green component defined by a float (0 to 1)
	// RETURNS: Green component of color
	inline float GetGreenf() const;

	// PURPOSE: Sets green component directly
	// PARAMS:
	//	g -	green component (0 to 65535)
	inline void SetGreen(int g);

	// PURPOSE: Get the blue component defined by an integer (0 to 65535)
	// RETURNS: Blue component of color
	inline u16 GetBlue() const;

	// PURPOSE: Get the blue component defined by a float (0 to 1)
	// RETURNS: Blue component of color
	inline float GetBluef() const;

	// PURPOSE: Sets blue component directly
	// PARAMS:
	//	b -	blue component (0 to 65535)
	inline void SetBlue(int b);

	// PURPOSE: Get the alpha component defined by an integer (0 to 65535)
	// RETURNS: Alpha component of color
	inline u16 GetAlpha() const;

	// PURPOSE: Get the alpha component defined by a float (0 to 1)
	// RETURNS: Alpha component of color
	inline float GetAlphaf() const;

	// PURPOSE: Sets alpha component directly
	// PARAMS:
	//	a -	alpha component (0 to 65535)
	inline void SetAlpha(int a);

	// PURPOSE:	Get the packed color value.
	// RETURNS: Platform-independent packed color value
	inline u64 GetColor() const;

	// PURPOSE:	Scale each component of this color by the other color's component floating point values.
	Color64 operator*(const Color64 that) const;

	// PURPOSE: Add each component of the other color to each color of this component
	// RETURNS: the new color that is the result of the addition
	inline Color64 operator+(const Color64 that) const;

	// PURPOSE: Add each component of the other color to each color of this component and sets this color
	inline void operator+=(const Color64 that);

	// PURPOSE: Determine if two colors are equal.
	// RETURNS: true if the given color equals this color, false if it does not
	inline bool operator==(const Color64& that) const;

	// PURPOSE: Determine if two colors are not equal
	// RETURNS: true if the given color does not equal this color, false if it does
	inline bool operator!=(const Color64& that) const;


private:
	friend datSerialize & operator<< (datSerialize &s, Color64 &c);
	u64 m_Color;
};

// PURPOSE: Serialize color data
inline datSerialize & operator<< (datSerialize &s, Color64 &c) {
	s << c.m_Color;
	return s;
}

//=============================================================================
// Implementations

// PURPOSE:	Constructor
Color64::Color64(u64 color)
{
	m_Color=color;
}

void Color64::Set(u64 color)
{
	m_Color=color;
}


void Color64::Setf(float r,float g,float b,float a)
{
	Set(int(r * 65535.0f),int(g * 65535.0f),int(b * 65535.0f),int(a * 65535.0f));
}


Color64::Color64(int r,int g,int b,int a)
{
	Set(r,g,b,a);
}


Color64::Color64(float r,float g,float b,float a)
{
	Setf(r,g,b,a);
}


u16 Color64::GetRed() const
{
	return u16(m_Color >> 32);
}

u16 Color64::GetGreen() const
{
	return u16(m_Color >> 16);
}

u16 Color64::GetBlue() const
{
	return u16(m_Color);
}

u16 Color64::GetAlpha() const
{
	return u16(m_Color >> 48);
}

u64 Color64::GetColor() const
{
	return m_Color;
}

float Color64::GetRedf() const
{
	return GetRed() * (1.0f / 65535.0f);
}

float Color64::GetGreenf() const
{
	return GetGreen() * (1.0f / 65535.0f);
}

float Color64::GetBluef() const
{
	return GetBlue() * (1.0f / 65535.0f);
}

float Color64::GetAlphaf() const
{
	return GetAlpha() * (1.0f / 65535.0f);
}

void Color64::SetRed(int r)
{
	m_Color = (m_Color & 0xFFFF0000FFFFFFFFULL) | (u64(r) << 32);
}

void Color64::SetGreen(int g)
{
	m_Color = (m_Color & 0xFFFFFFFF0000FFFFULL) | (u64(g) << 16);
}

void Color64::SetBlue(int b)
{
	m_Color = (m_Color & 0xFFFFFFFFFFFF0000ULL) | u64(b);
}

void Color64::SetAlpha(int a)
{
	m_Color = (m_Color & 0x0000FFFFFFFFFFFFULL) | (u64(a) << 48);
}

inline Color64 Color64::operator+(const Color64 that) const
{
	return Color64(GetRed() + that.GetRed(), GetGreen() + that.GetGreen(), GetBlue() + that.GetBlue(), GetAlpha() + that.GetAlpha());
}

inline void Color64::operator+=(const Color64 that)
{
	Set(GetRed()	+ that.GetRed(),
		GetGreen()	+ that.GetGreen(),
		GetBlue()	+ that.GetBlue(),
		GetAlpha()	+ that.GetAlpha());
}

bool Color64::operator==(const Color64& that) const
{
	return (this->m_Color == that.m_Color);	
}

bool Color64::operator!=(const Color64& that) const
{
	return (this->m_Color != that.m_Color);	
}

}	// namespace rage

#endif
