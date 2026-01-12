//
// vector/vector3_strings.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECTOR3STRINGS_H
#define VECTOR_VECTOR3STRINGS_H

#include "vector3.h"

namespace rage 
{
	template <size_t _Size> class atFixedString;

	// PURPOSE:
	//	Provide string append and concatenation functionality for supported classes. Currently this means atFixedString<>.
	//
	template <size_t _Size> 
	void AppendFormatted(atFixedString<_Size>& string, const Vector3& v, const char* szFormat = "%f %f %f")
	{
		char buffer[atFixedString<_Size>::kAppendFormattedBufferSize];	
		formatf(buffer, atFixedString<_Size>::kAppendFormattedBufferSize, szFormat, v.x, v.y, v.z);
		string.Append(buffer);
	};

	template <size_t _Size> 
	atFixedString<_Size>& operator <<(atFixedString<_Size>& string, const Vector3& v)
	{
		::AppendFormatted(string, v);
		return string;
	}

	template <size_t _Size> 
	atFixedString<_Size>& operator +=(atFixedString<_Size>& string, const Vector3& v)
	{
		::AppendFormatted(string, v);
		return string;
	}

}

#endif
