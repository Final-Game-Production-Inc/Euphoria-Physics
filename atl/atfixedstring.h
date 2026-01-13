//
// atl/string.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_ATFIXEDSTRING_H
#define ATL_ATFIXEDSTRING_H

#include <stdio.h>
#include <string.h>

#include "array.h"
#include "data/safestruct.h"

namespace rage {

#ifndef StringAssert
#define StringAssert(x)		FastAssert(x)
#endif

	/*
	PURPOSE
	The atFixedString class implements a safe string object of a fixed maximum length. 
	Storage is internal to this object.
	Range checking is done on all operations.
	<FLAG Component>
	*/
	template <size_t _Size>
	class atFixedString 
	{

	protected:
		 
		void _Append(const char *s,size_t len)
		{
			StringAssert(m_Length + len + 1 <= _Size);
			memcpy(m_Data + m_Length, s, len);
			m_Data[m_Length + len] = 0;
			m_Length = (unsigned short) (m_Length + len);
		}


	public:

		static const size_t npos = (size_t)-1;

		// PURPOSE: Default constructor
		atFixedString() 
		{ 
			m_Data[0] = 0;
			m_Length = 0;
		}

		// PURPOSE: Constructor initializes string from a C string
		// PARAMS: s - String to initialize atString object with
		atFixedString(const char *s) 
		{
			m_Length = 0;
			if (s)
			{
				_Append(s,(int)strlen(s));
			}
			else
			{
				m_Data[0] = 0;
			}
		}

		// PURPOSE: Copy constructor
		// PARAMS: that - atString object to initialize this object with
		atFixedString(const atFixedString &that)
		{
			m_Length = 0;
			_Append(that.m_Data,that.m_Length);	
		}

		// PURPOSE: Append constructor; initializes a string as the
		//	concatenation of two other strings
		// PARAMS: that - First string
		//	s - Second string
		atFixedString(const atFixedString &that,const char *s) 
		{
			m_Length = 0;
			_Append(that.m_Data,that.m_Length);
			if (s)
				_Append(s,(int)strlen(s));
		}

		// PURPOSE: Append constructor; initializes a string as the
		//	concatenation of two other strings
		// PARAMS: that - First string
		//	that2 - Second string
		atFixedString(const atFixedString &that,const atFixedString &that2) 
		{
			m_Length = 0;
			_Append(that.m_Data,that.m_Length);
			_Append(that2.m_Data,that2.m_Length);
		}

		atFixedString( datResource& /*rsc*/ )	
		{
		}

		DECLARE_PLACE(atFixedString);

		// PURPOSE: Destructor
		~atFixedString()
		{ }

		// PURPOSE: Append an arbitrary null terminated string and return the length.
		size_t Append(const char* s)
		{
			size_t len = strlen(s);
			_Append(s, len);
			return len;
		}

		void Append(const char* buffer, size_t len)
		{
			_Append(buffer, len);
		}

		// PURPOSE: Allow callers to have arbitrary objects formatted and appended.
		template <typename T> void AppendFormatted(const T& value, const char* szFormat)
		{
			char buffer[_Size];
			size_t len = strlen( formatf_sized(buffer, _Size, szFormat, value) );
			_Append(buffer, len);
		}

		// PURPOSE: Conversion operator.  This allows an atFixedString to be
		// passed to any function accepting a const char* with no extra work.
		// RETURNS: Associated string data, or empty (not null) string if
		// atFixedString object was empty
		operator const char * () const 
		{
			return m_Length? m_Data : "";
		}

		// PURPOSE: Assignment operator.
		// PARAMS: s - String to assign to this object
		// RETURNS: Reference to this object
		atFixedString& operator=(const char *s) 
		{
			m_Length = 0;
			if (s)
				_Append(s,(int)strlen(s));
			return *this;
		}

		// PURPOSE: Assignment operator.
		// PARAMS: that - String to assign to this object
		// RETURNS: Reference to this object
		atFixedString& operator=(const atFixedString &that) {
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
		// instead of <CODE>atFixedString foo = "baz" + "blarg";</CODE>
		// use <CODE>atFixedString foo("baz"); foo += "blarg";</CODE>
		atFixedString& operator+=(const char *s) 
		{
			if (s)
				_Append(s,(int)strlen(s));
			return *this;
		}

		// PURPOSE: Concatenation operator
		// PARAMS: that - String to append to current string object
		// RETURNS: Reference to this object
		// NOTES:
		// friend operator+ is intentionally omitted here because
		// it causes a lot of temporaries to be generated.
		// instead of <CODE>atFixedString foo = "baz" + "blarg";</CODE>
		// use <CODE>atFixedString foo("baz"); foo += "blarg";</CODE>
		atFixedString& operator+=(const atFixedString &that)
		{
			_Append(that.m_Data,that.m_Length);
			return *this;
		}

		// PURPOSE: Concatenation operator
		// PARAMS: ch - Character to append to current string object
		// RETURNS: Reference to this object
		atFixedString& operator+=(char ch) 
		{
			char s[2];
			s[0] = ch; 
			s[1] = 0;
			_Append(s,1);
			return *this;
		}

		// Call the default append for the specified type.
		template <typename _Type> atFixedString& operator+=(const _Type& object) {
			AppendFormatted(*this, object);
			return *this;
		}

		atFixedString& operator<<(const char* s)
		{
			Append(s);
			return *this;
		}

		// Call the default append for the specified type.
		template <typename _Type> atFixedString& operator<<(const _Type& object) {
			AppendFormatted(*this, object);
			return *this;
		}

		// PURPOSE: Equality operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if strings are exactly the same (case-sensitive), else false
		bool operator==(const char *s) const
		{
			if (s == NULL)
				return false;
			else
				return !strcmp(*this,s); 
		}

		// PURPOSE: Equality operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if strings are exactly the same (case-sensitive), else false
		bool operator==(const atFixedString &s) const 
		{ 
			return m_Length == s.m_Length && !strcmp(*this,s); 
		}

		// PURPOSE: Less-than operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically smaller than s, else false
		bool operator<(const char *s) const 
		{ 
			return strcmp(*this,s) < 0; 
		}

		// PURPOSE: Less-than operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically smaller than s, else false
		bool operator<(const atFixedString &s) const 
		{ 
			return strcmp(*this,s) < 0; 
		}

		// PURPOSE: Less-than-or-equal operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically smaller than or equal to s, else false
		bool operator<=(const char *s) const 
		{ 
			return strcmp(*this,s) <= 0; 
		}

		// PURPOSE: Less-than-or-equal operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically smaller than or equal to s, else false
		bool operator<=(const atFixedString &s) const 
		{ 
			return strcmp(*this,s) <= 0; 
		}

		// PURPOSE: Greater-than operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically larger than s, else false
		bool operator>(const char *s) const 
		{ 
			return strcmp(*this,s) > 0; 
		}

		// PURPOSE: Greater-than operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically larger than s, else false
		bool operator>(const atFixedString &s) const 
		{ 
			return strcmp(*this,s) > 0; 
		}

		// PURPOSE: Greater-than-or-equal operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically larger than or equal to s, else false
		bool operator>=(const char *s) const 
		{ 
			return strcmp(*this,s) >= 0; 
		}

		// PURPOSE: Greater-than-or-equal operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if current string is lexicographically larger than or equal to s, else false
		bool operator>=(const atFixedString &s) const 
		{ 
			return strcmp(*this,s) >= 0; 
		}

		// PURPOSE: Inequality operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if strings are different (case-sensitive), else false
		bool operator!=(const char *s) const
		{
			if (s == NULL)
				return true;	// this object always has a string pointer, perhaps zero length, but it exists.
			else
				return strcmp(*this,s) != 0;
		}


		// PURPOSE: Inequality operator
		// PARAMS: s - String to compare with current string
		// RETURNS: True if strings are different (case-sensitive), else false
		bool operator!=(const atFixedString &s) const 
		{ 
			return m_Length != s.m_Length || strcmp(*this,s) != 0; 
		}

		// PURPOSE: Array accessor
		// PARAMS: i - Index of (range-checked) character to return
		// RETURNS: Character at that address
		char& operator[] (int i) 
		{
			StringAssert(i >= 0 && i < m_Length);
			return m_Data[i];
		}

		// PURPOSE: Array accessor
		// PARAMS: i - Index of (range-checked) character to return
		// RETURNS: Character at that address
		char operator[] (int i) const 
		{
			StringAssert(i >= 0 && i < m_Length);
			return m_Data[i];
		}

		// PURPOSE:	For compatibility with std::string
		unsigned length() const 
		{		
			return m_Length;
		}

		// PURPOSE: For compatibility with std::string
		bool empty() const
		{
			return m_Length == 0;
		}

		// PURPOSE: For compatibility with std::string
		//	c: character to find
		//	pos: position of the first character in the string to be considered in the search
		size_t find_first_of(char c, size_t pos = 0) const
		{
			if (m_Length) 
			{
				for (size_t i = pos; i < m_Length; i++)
				{
					if (m_Data[i] == c)
						return i;
				}
			}

			return npos;
		}

		// PURPOSE: For compatibility with std::string
		//	c: character to find
		//	pos: position of the last character in the string to be considered in the search
		size_t find_last_of(char c, size_t pos = npos) const
		{
			if (m_Length) 
			{
				// start at the end of the string
				size_t i = m_Length - 1;

				// if 'pos' was provided and within the string, start here instead.
				if (pos < i)
					i = pos;

				// iterate from the start position to zero, searching for the first instance of 'c'
				for (; i < npos; i--)
				{
					if (m_Data[i] == c)
					{
						return i;
					}
				}
			}

			return npos;
		}

		// PURPOSE: For compatibility with std::string
		char back()
		{
			if (m_Length)
			{
				return m_Data[m_Length-1];
			}

			return '\0';
		}

		// PURPOSE: For those unfortunate times where code (ie widgets) simply must have access to the buffer.
		// Note: Any code that does this really should be refactored, this was added as a first step towards 
		// making potentially risky code more string-safe by at least making it obvious that pointers could
		// be misused.
		char* GetInternalBuffer()
		{
			return m_Data;
		}

		// PURPOSE: Return the size of the internal buffer.
		int GetInternalBufferSize() const
		{
			return _Size;
		}

		// PURPOSE:	For compatibility with std::string
		const char *c_str() const 
		{
			return m_Data;
		}

		// PURPOSE: Returns length of string object
		// RETURNS: Length of string object
		// NOTES: Length is explicitly stored, so this operation is O(1)
		int GetLength() const 
		{
			return m_Length;
		}

		// PURPOSE: Allow for explicit setting of the active length.
		// NOTE: Does not modify the buffer, it is assumed the caller will have modified it directly either
		// via GetInternalBuffer() (please avoid that if you can) or (preferably) operator[] usage.
		void SetLengthUnsafe(short length)
		{
			Assert(length >= 0 && length <= _Size);
			m_Length = length;
		}

		// PURPOSE: Resets contents of string
		// NOTES: Does not reclaim storage.  Currently only deleting the
		// string will do that.
		void Reset() 
		{
			m_Data[0] = 0;
			m_Length = 0;
		}

		// PURPOSE: Truncates string to specified length
		// PARAMS: lastPos - Index of the first character in the string
		//	to delete.  Truncate(0) is the same as Reset.
		void Truncate(short lastPos) 
		{
			StringAssert(lastPos >= 0 && lastPos <= m_Length);
			m_Length = lastPos;
			m_Data[m_Length] = 0;
		}

		// PURPOSE: Initializes one string with substring of another.
		// PARAMS: str - Source string
		// 	firstChar - index of first character in string to use
		//	length - number of characters starting at firstChar to
		//		copy to this string object.  If length is -1 (the default)
		//		firstChar to end-of-string is used.  Length is also
		//		clamped to end-of-string if necessary
		void Set(const atFixedString &str,int firstChar,int length = -1)
		{
			Assert(firstChar >= 0 && firstChar <= str.GetLength());
			if (length == -1 || length > (str.GetLength() - firstChar))
				length = str.GetLength() - firstChar;
			m_Length = 0;
			_Append(str.m_Data + firstChar,length);
		}

		// PURPOSE: Sets string using substring of a NULL-terminated string.
		// PARAMS: str - Source string
		//  strLength - length of source string
		// 	firstChar - index of first character in string to use
		//	length - number of characters starting at firstChar to
		//		copy to this string object.  If length is -1 (the default)
		//		firstChar to end-of-string is used.  Length is also
		//		clamped to end-of-string if necessary
		void Set(const char *str, int strLength, int firstChar, int length = -1) {
			Assert(firstChar >= 0 && firstChar <= strLength);
			if(-1 == length || length > (strLength - firstChar))
				length = strLength - firstChar;
			m_Length = 0;
			_Append(str + firstChar, length);
		}


		// PURPOSE: Makes the string lowercase, in-place
		void Lowercase() 
		{
			if (m_Length) 
			{
				for (char* c=m_Data; *c; c++) 
				{
					if (*c >= 'A' && *c <= 'Z')
					{
						*c += 32;
					}
				}
			}
		}

		// PURPOSE: Makes the string uppercase, in-place
		void Uppercase() 
		{
			if (m_Length) 
			{
				for (char* c=m_Data; *c; c++)
				{
					if (*c >= 'a' && *c <= 'z')
					{
						*c -= 32;
					}
				}
			}
		}

		//
		// PURPOSE
		//	Determines whether the end of this instance matches the specified string.
		// PARAMS
		//	str - the string to compare against
		// RETURNS
		//	true if the end of this instance matches value; otherwise, false.  This function is case sensitive.
		//
		bool EndsWith(const char* str) const 
		{
			int length=(int)strlen(str);
			if (length>m_Length)
				return false;

			const char* data=m_Data+(m_Length-length);
			for (int i=length-1;i>=0;i--)
			{
				if (data[i]!=str[i])
					return false;
			}

			return true;
		}

		//
		// PURPOSE
		//	Determines whether the beginning of this instance matches the specified string.
		// PARAMS
		//	str - the string to compare against
		// RETURNS
		//	true if the beginning of this instance matches value; otherwise, false.  This function is case sensitive.
		//
		bool StartsWith(const char* str) const 
		{
			int length=(int)strlen(str);
			if (length>m_Length)
				return false;

			for (int i=0;i<length;i++)
			{
				if (m_Data[i]!=str[i])
					return false;
			}

			return true;
		}

		// PURPOSE
		//	Replace all instances of one character with another.
		void Replace(char from, char to)
		{
			for (int i=0;i<m_Length;i++)
			{
				if (m_Data[i] == from)
					m_Data[i] = to;
			}
		}

		// PURPOSE
		//  Serialize string
		void Serialize(datSerialize& ser)
		{
			if( ser.IsRead() )
			{
				int len;
				ser << len;

				StringAssert(len <= _Size);

				if (len > 0)
				{
					ser.Get( m_Data, len );
					m_Length = (unsigned short) len-1;	
				}
				else
				{
					m_Length = 0;
					m_Data[0] = 0;
				}
			}
			else
			{
				int len = m_Length;

				if (len > 0)
				{
					len++;
					ser << len;
					ser.Put( m_Data );
				}
				else
				{
					ser << len;
				}
			}
		}

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct& s)
		{
			SSTRUCT_BEGIN(atFixedString<_Size>)
			SSTRUCT_FIELD(atFixedString<_Size>, m_Length)
			SSTRUCT_CONTAINED_ARRAY_COUNT(atFixedString<_Size>, m_Data, _Size)
			SSTRUCT_END(atFixedString<_Size>)
		}
#endif

		// The kAppendFormattedBufferSize is used by the standard AppendFormatted functions to determine what size of 
		// stack buffer to create for performing format() formatting into. 
		enum { kAppendFormattedBufferSize = _Size };


		//
		// Append and concatenation for: float
		//
		void AppendFormatted(float v, const char* szFormat = "%f")
		{
			char buffer[kAppendFormattedBufferSize];	
			formatf(buffer, kAppendFormattedBufferSize, szFormat, v);
			Append(buffer);
		};

		atFixedString& operator <<(float v)
		{
			AppendFormatted(v);
			return *this;
		}

		atFixedString& operator +=(float v) {
			AppendFormatted(v);
			return *this;
		}

		//
		// Append and concatenation for: int
		//
		void AppendFormatted(int v, const char* szFormat = "%d")
		{
			char buffer[kAppendFormattedBufferSize];	
			formatf(buffer, szFormat, v);
			Append(buffer);
		};

		atFixedString& operator <<(int v)
		{
			AppendFormatted(v);
			return *this;
		}

		atFixedString& operator +=(int v) {
			AppendFormatted(v);
			return *this;
		}

		//
		// Append and concatenation for: u32
		//
		void AppendFormatted(u32 v, const char* szFormat = "%u")
		{
			char buffer[kAppendFormattedBufferSize];	
			formatf(buffer, szFormat, v);
			Append(buffer);
		};

		atFixedString& operator <<(u32 v)
		{
			AppendFormatted(v);
			return *this;
		}

		atFixedString& operator +=(u32 v) {
			AppendFormatted(v);
			return *this;
		}

	protected:
		unsigned short m_Length;	// Current length of string
		char m_Data[_Size];			// String payload.  Ignored if length is zero, else is guaranteed to be null-terminated
	};


	//////////////////////////////////////////////////////////////////////////
	// AppendFormatted for native types.
	//
	// To support other types (ie Vec3V) add a similar block of code in the appropriate header.
	//
	template <size_t SIZE> 
	void AppendFormatted(atFixedString<SIZE>& string, float v, const char* szFormat = "%g")
	{
		char buffer[SIZE];
#if RSG_WIN32 || RSG_DURANGO
		size_t len = _snprintf(buffer, SIZE, szFormat, v);
#else
		size_t len = snprintf(buffer, SIZE, szFormat, v);
#endif
		string.Append(buffer,len);
	};

	template <size_t SIZE, class DATATYPE >
	atFixedString<SIZE>& operator <<(atFixedString<SIZE>& string, DATATYPE v)
	{
		AppendFormatted(string, v);
		return string;
	}
}	// namespace rage



#endif

