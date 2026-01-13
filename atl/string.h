//
// atl/string.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_STRING_H
#define ATL_STRING_H

#include <stdio.h>
#include <string.h>

#include "array.h"
#include "data/struct.h"

namespace rage {

#ifndef StringAssert
#define StringAssert(x)		FastAssert(x)
#endif

/*
PURPOSE
	The atString class implements a safe string object. Range checking is done on all operations.
<FLAG Component>
*/
class atString {

	void _Append(const char *s,int l);
	void _Append(const unsigned short *s);

public:

	static const int npos = -1;

	// PURPOSE: Default constructor
	atString() { 
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
	}

	// PURPOSE: Constructor initializes string from a C string
	// PARAMS: s - String to initialize atString object with
	explicit atString(const char *s) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		if (s)
			_Append(s,(int)strlen(s));
	}

	// PURPOSE: Constructor initializes string from a Unicode string
	// PARAMS: s - String to initialize atString object with
	//		only least-significant byte of each character is used
	atString(const unsigned short *s) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		_Append(s);
	}

	// PURPOSE: Move constructor
	// PARAMS: other - atString object to move this object from
	atString(atString&& other)
		: m_Data(other.m_Data)
		, m_Length(other.m_Length)
		, m_Allocated(other.m_Allocated)
	{
		other.m_Data = nullptr;
		other.m_Length = 0;
		other.m_Allocated = 0;
	}

	// PURPOSE: Copy constructor
	// PARAMS: that - atString object to initialize this object with
	atString(const atString &that) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		_Append(that.m_Data,that.m_Length);	
	}

	// PURPOSE: Append constructor; initializes a string as the
	//	concatenation of two other strings
	// PARAMS: that - First string
	//	s - Second string
	atString(const atString &that,const char *s) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		_Append(that.m_Data,that.m_Length);
		if (s)
			_Append(s,(int)strlen(s));
	}

	// PURPOSE: Append constructor; initializes a string as the
	//	concatenation of two other strings
	// PARAMS: that - First string
	//	that2 - Second string
	atString(const atString &that,const atString &that2) {
		m_Data = 0; 
		m_Length = 0;
		m_Allocated = 0;
		_Append(that.m_Data,that.m_Length);
		_Append(that2.m_Data,that2.m_Length);
	}

	atString( datResource& rsc )	
	{
		rsc.PointerFixup( m_Data );
	}

	DECLARE_PLACE(atString);

	// PURPOSE: Destructor
	~atString() 
	{ 
		Clear();
	}

	// PURPOSE: Conversion operator.  This allows an atString to be
	// passed to any function accepting a const char* with no extra work.
	// RETURNS: Associated string data, or empty (not null) string if
	// atString object was empty
	operator const char * () const {
		return m_Length? m_Data : "";
	}

	// PURPOSE: Preallocates some memory for a string, the string must be empty first.
	void Reserve(unsigned short bytes);

	// PURPOSE: Makes sure the string size matches the number of bytes - might truncate or overallocate with existing string!
	void Resize(unsigned short bytes);

	// PURPOSE: Assignment operator.
	// PARAMS: s - String to assign to this object
	// RETURNS: Reference to this object
	atString& operator=(const char *s) {
		m_Length = 0;
		if (s)
			_Append(s,(int)strlen(s));
		return *this;
	}

	// PURPOSE: Assignment operator.
	// PARAMS: that - String to assign to this object
	// RETURNS: Reference to this object
	atString& operator=(const atString &that) {
		if (this != &that) {
			m_Length = 0;
			_Append(that.m_Data,that.m_Length);
		}
		return *this;
	}

	// PURPOSE
	// Move assignment operator.
	// PARAMS: that - String to move to this object
	// RETURNS: Reference to this object
	atString& operator=(atString&& other)
	{
		Clear();
		m_Data = other.m_Data;
		m_Length = other.m_Length;
		m_Allocated = other.m_Allocated;
		other.m_Data = nullptr;
		other.m_Length = 0;
		other.m_Allocated = 0;
		return *this;
	}

	// PURPOSE: Concatenation operator
	// PARAMS: s - String to append to current string object
	// RETURNS: Reference to this object
	// NOTES:
	// friend operator+ is intentionally omitted here because
	// it causes a lot of temporaries to be generated.
	// instead of <CODE>atString foo = "baz" + "blarg";</CODE>
	// use <CODE>atString foo("baz"); foo += "blarg";</CODE>
	atString& operator+=(const char *s) {
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
	// instead of <CODE>atString foo = "baz" + "blarg";</CODE>
	// use <CODE>atString foo("baz"); foo += "blarg";</CODE>
	atString& operator+=(const atString &that) {
		_Append(that.m_Data,that.m_Length);
		return *this;
	}

	// PURPOSE: Concatenation operator
	// PARAMS: ch - Character to append to current string object
	// RETURNS: Reference to this object
	atString& operator+=(char ch) {
		char s[2]; s[0] = ch; s[1] = 0;
		_Append(s,1);
		return *this;
	}

	// PURPOSE: Equality operator
	// PARAMS: left, right - strings to compare
	// RETURNS: True if strings are exactly the same (case-sensitive), else false
	friend bool operator==(const char* left, const atString& right);
	friend bool operator==(const atString& right, const char* left);

	// PURPOSE: Less-than operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically smaller than s, else false
	bool operator<(const char *s) const { return strcmp(*this,s) < 0; }

	// PURPOSE: Less-than operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically smaller than s, else false
	bool operator<(const atString &s) const { return strcmp(*this,s) < 0; }

	// PURPOSE: Less-than-or-equal operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically smaller than or equal to s, else false
	bool operator<=(const char *s) const { return strcmp(*this,s) <= 0; }

	// PURPOSE: Less-than-or-equal operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically smaller than or equal to s, else false
	bool operator<=(const atString &s) const { return strcmp(*this,s) <= 0; }

	// PURPOSE: Greater-than operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically larger than s, else false
	bool operator>(const char *s) const { return strcmp(*this,s) > 0; }

	// PURPOSE: Greater-than operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically larger than s, else false
	bool operator>(const atString &s) const { return strcmp(*this,s) > 0; }

	// PURPOSE: Greater-than-or-equal operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically larger than or equal to s, else false
	bool operator>=(const char *s) const { return strcmp(*this,s) >= 0; }

	// PURPOSE: Greater-than-or-equal operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if current string is lexicographically larger than or equal to s, else false
	bool operator>=(const atString &s) const { return strcmp(*this,s) >= 0; }

	// PURPOSE: Inequality operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if strings are different (case-sensitive), else false
	bool operator!=(const char *s) const
	{
		if (s == NULL)
			return (m_Data != NULL);
		else
			return strcmp(*this,s) != 0;
	}

	// PURPOSE: Inequality operator
	// PARAMS: s - String to compare with current string
	// RETURNS: True if strings are different (case-sensitive), else false
	bool operator!=(const atString &s) const { return m_Length != s.m_Length || strcmp(*this,s) != 0; }

	// PURPOSE: Array accessor
	// PARAMS: i - Index of (range-checked) character to return
	// RETURNS: Character at that address
	char& operator[] (int i) {
		StringAssert(i >= 0 && i <= m_Length);
		return m_Data[i];
	}

	// PURPOSE: Array accessor
	// PARAMS: i - Index of (range-checked) character to return
	// RETURNS: Character at that address
	char operator[] (int i) const {
		StringAssert(i >= 0 && i <= m_Length);
		return m_Data[i];
	}

	// PURPOSE:	For compatibility with std::string
	unsigned length() const {		
		return GetLength();
	}

	// PURPOSE:	For compatibility with std::string
	unsigned size() const {	
		return GetLength();
	}

	// PURPOSE:	For compatibility with std::string
	bool empty() const {		
		return (GetLength() == 0);
	}

	// PURPOSE:	For compatibility with std::string
	const char *c_str() const {
		return m_Length ? m_Data : "";
	}

	// PURPOSE: Returns length of string object
	// RETURNS: Length of string object
	// NOTES: Length is explicitly stored, so this operation is O(1)
	int GetLength() const {
		return m_Length;
	}

	// PURPOSE: Returns amount of allocated space for the string
	// RETURNS: Amount of allocated space for the string
	// NOTES: Allocated is explicitly stored, so this operation is O(1)
	int GetAllocated() const {
		return m_Allocated;
	}

	// PURPOSE: Distinguishes between an empty string and a null string.
	// RETURNS: Returns if the string is null or not.
	bool IsNull() const {
		return m_Data == NULL;
	}

	// PURPOSE: Resets contents of string
	// NOTES: Does not reclaim storage.  Currently only deleting the
	// string will do that.
	void Reset() {
		m_Length = 0;
		if (m_Data)
		{
			m_Data[0] = 0;
		}
	}

	// PURPOSE: Truncates string to specified length
	// PARAMS: lastPos - Index of the first character in the string
	//	to delete.  Truncate(0) is the same as Reset.
	void Truncate(int lastPos) {
		StringAssert(lastPos >= 0 && lastPos <= m_Length);
		m_Length = (unsigned short)lastPos;
		if (m_Data) {
			m_Data[m_Length] = 0;
		}
	}

	// PURPOSE: Allow for explicit setting of the active length.
	// NOTE: Does not modify the buffer, it is assumed the caller will have modified it directly either
	// via c_str() (please avoid that if you can) or (preferably) operator[] usage. An example usage would
	// be converting from a wide string using WideCharToMultiByte on the c_str().
	void SetLengthUnsafe(unsigned short length)
	{
		if (AssertVerify(length < m_Allocated))
		{
			m_Length = length;
			if (m_Data)
			{
				m_Data[m_Length] = 0;
			}
		}
	}

	// PURPOSE: Does what Truncate should do.
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

	void ClearAndSanitize()
	{
		if (m_Data)
		{
			sysMemSet(m_Data, 0, m_Length);
		}
		Clear();
	}

	// PURPOSE: Initializes one string with substring of another.
	// PARAMS: str - Source string
	// 	firstChar - index of first character in string to use
	//	length - number of characters starting at firstChar to
	//		copy to this string object.  If length is npos (the default)
	//		firstChar to end-of-string is used.  Length is also
	//		clamped to end-of-string if necessary
	void Set(const atString &str,int firstChar,int length = npos);

	// PURPOSE: Sets string using substring of a NULL-terminated string.
	// PARAMS: str - Source string
	//  strLength - length of source string
	// 	firstChar - index of first character in string to use
	//	length - number of characters starting at firstChar to
	//		copy to this string object.  If length is npos (the default)
	//		firstChar to end-of-string is used.  Length is also
	//		clamped to end-of-string if necessary
	void Set(const char *str, int strLength, int firstChar, int length = npos);

	// PURPOSE: Makes the string lowercase, in-place
	void Lowercase();

	// PURPOSE: Makes the string uppercase, in-place
	void Uppercase();

	// PURPOSE
	//	Determines whether the end of this instance matches the specified string.
	// PARAMS
	//	str - the string to compare against
	// RETURNS
	//	true if the end of this instance matches value; otherwise, false.  This function is case sensitive.
	bool EndsWith( const char* str ) const;

    // PURPOSE
    //	Determines whether the end of this instance matches the specified string.
    // PARAMS
    //	str - the string to compare against
    // RETURNS
    //	true if the end of this instance matches value; otherwise, false.  This function is case sensitive.
    bool EndsWith( const atString &s ) const
    {
        return EndsWith( s.c_str() );
    }

	// PURPOSE
	//	Determines whether the beginning of this instance matches the specified string.
	// PARAMS
	//	str - the string to compare against
	// RETURNS
	//	true if the beginning of this instance matches value; otherwise, false.  This function is case sensitive.
	bool StartsWith(const char* str) const;

    // PURPOSE
    //	Determines whether the beginning of this instance matches the specified string.
    // PARAMS
    //	str - the string to compare against
    // RETURNS
    //	true if the beginning of this instance matches value; otherwise, false.  This function is case sensitive.    
    bool StartsWith( const atString &s ) const
    {
        return StartsWith( s.c_str() );
    }

    // PURPOSE
    //	Finds the first instance of the specified character
    // PARAMS
    //	c - the char to find
    //  startIndex - the index to start searching at
    // RETURNS
    //	The index of 'c', or npos if it is not found
    int IndexOf( char c, int startIndex=npos ) const;

    // PURPOSE
    //	Finds the first instance of the specified string
    // PARAMS
    //	str - the string to find
    //  startIndex - the index to start searching at
    // RETURNS
    //	The index of 'str', or npos if it is not found
    int IndexOf( const char* str, int startIndex=npos ) const;

    // PURPOSE
    //	Finds the first instance of the specified string
    // PARAMS
    //	str - the string to find
    //  startIndex - the index to start searching at
    // RETURNS
    //	The index of 'str', or npos if it is not found
    int IndexOf( const atString &s, int startIndex=npos ) const
    {
        return IndexOf( s.c_str(), startIndex );
    }

    // PURPOSE
    //	Finds the last instance of the specified character
    // PARAMS
    //	c - the char to find
    // RETURNS
    //	The index of 'c', or npos if it is not found
    int LastIndexOf( char c ) const;

    // PURPOSE
    //	Finds the last instance of the specified string
    // PARAMS
    //	str - the string to find
    // RETURNS
    //	The index of 'str', or npos if it is not found
    int LastIndexOf( const char* str ) const;

    // PURPOSE
    //	Finds the last instance of the specified string
    // PARAMS
    //	str - the string to find
    // RETURNS
    //	The index of 'str', or npos if it is not found
    int LastIndexOf( const atString &s ) const
    {
        return LastIndexOf( s.c_str() );
    }

	// PURPOSE
	//	Replaces a substring of our string with the contents of another string.
	// PARAMS
	//	startPos - the starting position in our string to begin replacement.
	//  length - the length of our substring to replace.
	//	other - the other string to insert into our string.
	// EXAMPLE
	//	atString a("This is a bad example");
	//  atString b("good");
	//  a.ReplaceSubstring(10,3,b); // a == "This is a good example"
	void ReplaceSubstring(int startPos, int numChars, const char* other, const int otherLen);
	void ReplaceSubstring(int startPos, int numChars, const atString& other) {
		ReplaceSubstring(startPos, numChars, other.m_Data, other.m_Length);
	}

    // PURPOSE
    //	Finds every instance of 'search' and replace it with 'replace'
    // PARAMS
    //	search - the string to find
    //  replace - the string to replace it with
    void Replace( const char *search, const char *replace );

    // PURPOSE
    //	Finds every instance of 'search' and replace it with 'replace'
    // PARAMS
    //	search - the string to find
    //  replace - the string to replace it with
    void Replace( const atString &search, const atString &replace )
    {
        Replace( search.c_str(), replace.c_str() );
    }

	// PURPOSE
	//	static method which doesn't require passing atString arguments
	static void Replace(char* str, size_t strMaxLen, const char* search, const char* replace)
	{
		atString temp( str );
		temp.Replace( search, replace );
		safecpy( str, temp.c_str(), strMaxLen );
	}

    // PURPOSE: Splits the string into two parts at the first occurrence of 'c', producing two new strings.
    // PARAMS:
    //  left - the left part of the split
    //  right - the right part of the split
    //  c - the character at which to split
    void Split( atString& left, atString& right, char c ) const;

    // PURPOSE: Splits the string into two parts at the first occurrence of 's', producing two new strings.
    // PARAMS:
    //  left - the left part of the split
    //  right - the right part of the split
    //  s - the string at which to split
    void Split( atString& left, atString& right, const char* s ) const;

    // PURPOSE: Splits the string into two parts at the first occurrence of 's', producing two new strings.
    // PARAMS:
    //  left - the left part of the split
    //  right - the right part of the split
    //  s - the string at which to split
    void Split( atString& left, atString& right, const atString &s ) const
    {
        Split( left, right, s.c_str() );
    }

    // PURPOSE: Splits the string at each occurrence of 'c'.
    // PARAMS:
    //  split - the list containing the results
    //  c - the character at which to split
    //  removeEmptyStrings - whether or not to include empty strings in the resulting list
    void Split( atArray<atString> &split, char c, bool removeEmptyStrings=false ) const;

    // PURPOSE: Splits the string at each occurrence of 'c'.
    // PARAMS:
    //  split - the list containing the results
    //  str - the string at which to split
    //  removeEmptyStrings - whether or not to include empty strings in the resulting list
    void Split( atArray<atString> &split, const char* str, bool removeEmptyStrings=false ) const;

    // PURPOSE: Splits the string at each occurrence of 'c'.
    // PARAMS:
    //  split - the list containing the results
    //  s - the atString at which to split
    //  removeEmptyStrings - whether or not to include empty strings in the resulting list
    void Split( atArray<atString> &split, const atString &s, bool removeEmptyStrings=false ) const
    {
        Split( split, s.c_str(), removeEmptyStrings );
    }

	// PURPOSE: Joins the elements of the array
	// PARAMS:
	//		beginIdx - index at which to begin the join. Negative values count from the end of the string
	//		endIdx - Last index to include. Negative values count from the end of the array, npos is the final element
	//		separator - string that will separate each of the joined strings
	static atString Join(const atArray<atString> & strs, int beginIdx, int endIdx, const char* separator="");
	static atString Join(const atArray<atString> & strs, const char* separator="") { return Join(strs, 0, npos, separator); }

	// PURPOSE
	//	Appends a portion of another string to our string.
	// PARAMS
	//	str - other string to append to ours
	//	length - length of the other string to append (npos == the entire string)
	void Append(const char* str, int length = npos);

	// PURPOSE
	//	Appends 'count' copies of the input character 'c' to our string
	void Append(int count, char c);

    // PURPOSE: Trims all whitespace characters off the start and end of the string
    void TrimLeft();
    void TrimRight();
    void Trim();

	// PURPOSE
	//	Erase a substring of the string
	// PARAMS
	//	startPos - the position to begin erasing from
	//  length - the length of the substring to erase
	//  bResize - if the string should be resized after the erase operation completes.
	void Erase(int startPos, int length = npos, bool bResize = false);

    // PURPOSE: Serialize string
	void Serialize(datSerialize&);


#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s)
	{
		STRUCT_BEGIN(atString);
		STRUCT_FIELD_VP(m_Data);
		STRUCT_FIELD(m_Length);
		STRUCT_FIELD(m_Allocated);
		STRUCT_END();
	}
#endif

private:
	char *m_Data;				// String payload.  Ignored if length is zero, else is guaranteed to be null-terminated
	unsigned short m_Length,	// Current length of string
				m_Allocated;	// Current amount of space allocated for string
};

/*
PURPOSE
	The atStringArray class is used for an array of strings.
*/
class atStringArray : public atArray <atString> {};

// PURPOSE:
//	Breaks string up into pathname elements and normalizes slashes appropriate for OS
// (\ for Win32, / for PS2+GameCube)
// PARAMS: src - Source string (which will be broken up)
//		path - Output string, receives path portion of src
//		basename - Output string, receives basename portion of src
//		ext - Output string, receives extension portion of src
extern void atParsePathElements(const atString &src,atString &path,atString &basename,atString &ext);

// PURPOSE:
//	Breaks string up into pathname elements and normalizes slashes appropriate for OS
// (\ for Win32, / for PS2+GameCube)
// PARAMS: src - Source string (which will be broken up)
//		path - Output string, receives path portion of src
//		filename - Output string, receives basename and extension portion of src
extern void atParsePathElements(const atString &src,atString &path,atString &file);

class atVarString : public atString
{
public:
	PRINTF_LIKE_N(2) inline atVarString(const char* format, ...)
	{
		char temp[4096] = "";
		va_list args;
		va_start(args, format);
		vsnprintf(temp, sizeof(temp), format, args);
		va_end(args);

		atString::operator=(temp);
	}
};

//------------------------------
// atString operators
//------------------------------

// PURPOSE: Equality operator
// PARAMS: left, right - strings to compare
// RETURNS: True if strings are exactly the same (case-sensitive), else false
inline bool operator==(const atString& left, const atString& right)
{
	return left.GetLength() == right.GetLength() && !strcmp(left,right);
}

// PURPOSE: Equality operator
// PARAMS: left, right - strings to compare
// RETURNS: True if strings are exactly the same (case-sensitive), else false
inline bool operator==(const char* left, const atString& right)
{
	if (left == NULL)
		return (right.m_Data == NULL);
	else
		return !strcmp(left,right); 
}

// PURPOSE: Equality operator
// PARAMS: left, right - strings to compare
// RETURNS: True if strings are exactly the same (case-sensitive), else false
inline bool operator==(const atString& left, const char* right)
{
	if (right == NULL)
		return (left.m_Data == NULL);
	else
		return !strcmp(left,right); 
}

}	// namespace rage

#endif // ATL_STRING_H

