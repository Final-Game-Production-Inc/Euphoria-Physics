//
// atl/wstring.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_WSTRING_H
#define ATL_WSTRING_H

#include "string/unicode.h"
#include "string/string.h"

#ifndef StringAssert
#define StringAssert(x)		FastAssert(x)
#endif

namespace rage {

extern char16 null_wide_string[];

/*
PURPOSE
	The atWideString class implements a safe wide string object.  Range checking is done on all operations.
*/
class atWideString {

	void _Append(const char *s,int l);

#if __UNICODE
	void _Append(const char16 *s,int l);
#endif

public:
	// PURPOSE: Default constructor
	atWideString() { 
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
	}

	// PURPOSE: Constructor initializes string from a C string
	// PARAMS: s - String to initialize atString object with
	atWideString(const char *s) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		if (s)
			_Append(s,(int)strlen(s));
	}

#if __UNICODE
	// PURPOSE: Constructor initializes string from a Unicode string
	// PARAMS: s - String to initialize atString object with
	atWideString(const char16 *s) {
		m_Data = (char16*) 0;
		m_Length = 0;
		m_Allocated = 0;
		if (s)
			_Append(s,(int)wcslen(s));
	}
#endif

	// PURPOSE: Copy constructor
	// PARAMS: that - atWideString object to initialize this object with
	atWideString(const atWideString &that) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		_Append(that.m_Data,that.m_Length);	
	}

	// PURPOSE: Destructor
	~atWideString() { delete[] m_Data; }

	// PURPOSE: Conversion operator.  This allows an atWideString to be
	// passed to any function accepting an unsigned short* with no extra work.
	// RETURNS: Associated string data, or empty (not null) string if
	// atWideString object was empty
	operator const char16 * () const {
		return m_Length? m_Data : null_wide_string;
	}

	// PURPOSE: Assignment operator.
	// PARAMS: s - String to assign to this object
	// RETURNS: Reference to this object
	atWideString& operator=(const char *s) {
		m_Length = 0;
		if (s)
			_Append(s,(int)strlen(s));
		return *this;
	}

#if __UNICODE
	// PURPOSE: Assignment operator.
	// PARAMS: s - String to assign to this object
	// RETURNS: Reference to this object
	atWideString& operator=(const char16 *s) {
		m_Length = 0;
		if (s)
			_Append(s,(int)wcslen(s));
		return *this;
	}
#endif

	// PURPOSE: Assignment operator.
	// PARAMS: that - String to assign to this object
	// RETURNS: Reference to this object
	atWideString& operator=(const atWideString &that) {
		m_Length = 0;
		_Append(that.m_Data,that.m_Length);
		return *this;
	}

	// PURPOSE: Concatenation operator
	// PARAMS: s - String to append to current string object
	// RETURNS: Reference to this object
	// NOTES:
	// friend operator+ is intentionally omitted here because
	// it causes a lot of temporaries to be generated.
	// instead of <CODE>atWideString foo = "baz" + "blarg";</CODE>
	// use <CODE>atWideString foo("baz"); foo += "blarg";</CODE>
	atWideString& operator+=(const char *s) {
		if (s)
			_Append(s,(int)strlen(s));
		return *this;
	}

	// PURPOSE: Concatenation operator
	// PARAMS: s - String to append to current string object
	// RETURNS: Reference to this object
	// NOTES:
	// friend operator+ is intentionally omitted here because
	// it causes a lot of temporaries to be generated.
	// instead of <CODE>atWideString foo = "baz" + "blarg";</CODE>
	// use <CODE>atWideString foo("baz"); foo += "blarg";</CODE>
	atWideString& operator+=(const atWideString &that) {
		_Append(that.m_Data,that.m_Length);
		return *this;
	}

	// PURPOSE: Concatenation operator
	// PARAMS: ch - Character to append to current string object
	// RETURNS: Reference to this object
	atWideString& operator+=(char ch) {
		char s[2]; s[0] = ch; s[1] = 0;
		_Append(s,1);
		return *this;
	}

	// PURPOSE: Array accessor
	// PARAMS: i - Index of (range-checked) character to return
	// RETURNS: Character at that address
	char16& operator[] (int i) {
		StringAssert(i >= 0 && i <= m_Length);
		return m_Data[i];
	}

	// PURPOSE: Array accessor
	// PARAMS: i - Index of (range-checked) character to return
	// RETURNS: Character at that address
	char16 operator[] (int i) const {
		StringAssert(i >= 0 && i < m_Length);
		return m_Data[i];
	}

	// PURPOSE: Returns length of string object
	// RETURNS: Length of string object (in unicode characters, not bytes)
	// NOTES: Length is explicitly stored, so this operation is O(1)
	int GetLength() const {
		return m_Length;
	}

	// PURPOSE: Resets contents of string
	// NOTES: Does not reclaim storage.  Currently only deleting the
	// string will do that.
	void Reset() {
		m_Length = 0;
	}


	// PURPOSE: Resets the string and reclaims storage
	void Clear()
	{
		if (m_Data)
		{
			delete[] m_Data;
		}
		m_Allocated = 0;
		m_Length = 0;
		m_Data = 0;
	}

private:
	char16 *m_Data;						// String payload.  Ignored if length is zero, else is guaranteed to be null-terminated
	unsigned short m_Length;			// Current length of string
	unsigned short m_Allocated;			// Current amount of space allocated for string
};

}	// namespace rage

#endif
