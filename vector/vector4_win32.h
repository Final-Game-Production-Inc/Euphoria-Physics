// 
// vector/vector4_win32.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_VECTOR4_WIN32_H
#define VECTOR_VECTOR4_WIN32_H

namespace rage
{
#ifndef VECTOR4_ZEROTYPE
#define VECTOR4_ZEROTYPE
	inline Vector4::Vector4( _ZeroType )
	{
		xyzw = _vzerofp;
	}
#endif // VECTOR4_ZEROTYPE

	inline Vector4::Vector4(const __m128& set)
	{
		xyzw = set;
	}

#if !__TOOL
#ifndef VECTOR4_OPERATOR_ASSIGN
#define VECTOR4_OPERATOR_ASSIGN
	VEC4_INLINE Vector4& Vector4::operator=(const Vector4& a)
	{
		xyzw = a.xyzw;
		return *this;
	}
#endif // VECTOR4_OPERATOR_ASSIGN

#ifndef VECTOR4_CONST_V
#define VECTOR4_CONST_V
	VEC4_INLINE Vector4::Vector4(const Vector4 &vec)
	{
		xyzw = vec.xyzw;
	}
#endif // VECTOR4_CONST_V
#endif // !__TOOL

} // namespace rage

#endif // VECTOR_VECTOR4_WIN32_H
