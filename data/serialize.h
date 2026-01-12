// 
// data/serialize.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_SERIALIZE_H
#define DATA_SERIALIZE_H

#include "system/bit.h"
#include "system/new.h"
#include "string/string.h"

namespace rage {

// 
// PURPOSE
//	This class handles the serialization of data to/from a stream
//	It specifies a serialization interfaces, and classes that derive from
//  it will specify the actual method and format for serialization
// <FLAG Component>
class datSerialize {
public:
	datSerialize() : m_Flags(0) { /*empty*/ };
	virtual ~datSerialize() { /*empty*/ }

	// PURPOSE: Serialize one object
	virtual void Serialize(bool &) = 0;
	virtual void Serialize(u8 &) = 0;
	virtual void Serialize(s8 &) = 0;
	virtual void Serialize(u16 &) = 0;
	virtual void Serialize(s16 &) = 0;
	virtual void Serialize(u32 &) = 0;
	virtual void Serialize(s32 &) = 0;
	virtual void Serialize(f32 &) = 0;
	virtual void Serialize(u64 &) = 0;

	// PURPOSE: Serialize a null-terminated string
	virtual void Put(const char *string) = 0;

	// PURPOSE: Unserialize a null-terminated string. Storage for the new string must be pre-allocated.
	//			UNLESS, size if 0, at which point the string is just discarded.
	virtual void Get(char *outString,int size) = 0;

	// PURPOSE: Returns true if we're reading/writing a binary format
	inline bool IsBinary() const;

	// PURPOSE: Returns true if we're reading from the data file
	inline bool IsRead() const;

	// PURPOSE: Returns true if an error was encountered during serialization up to this point
	inline bool HasError() const;
	
protected:
	inline void SetRead();
	inline void SetBinary();
	inline void SetError();
	
	// Flag enums
	enum {	FLG_READ=BIT0, 
			FLG_BINARY=BIT1, 
			FLG_ERROR=BIT2, 
			FLG_LAST=BIT3 };
	u32		m_Flags;				// Flags indicating status of serialize object
};

inline bool datSerialize::IsBinary() const {
	return ((m_Flags & FLG_BINARY) == FLG_BINARY); 
}

inline bool datSerialize::IsRead() const {
	return ((m_Flags & FLG_READ) == FLG_READ); 
}

inline bool datSerialize::HasError() const {
	return ((m_Flags & FLG_ERROR) == FLG_ERROR); 
}

inline void datSerialize::SetRead() {
	m_Flags |= FLG_READ;
}

inline void datSerialize::SetBinary() {
	m_Flags |= FLG_BINARY;
}

inline void datSerialize::SetError() {
	m_Flags |= FLG_ERROR;
}

// --------------------
//	Serialize operators
// --------------------
inline datSerialize & operator<< (datSerialize &s, datSerialize &) {
	AssertMsg(0, "Can't serialize a datSerialize object");
	return s;
}

inline datSerialize & operator<< (datSerialize &s, bool &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, u8 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, s8 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, u16 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, s16 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, u32 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, s32 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, u64 &v) {
	s.Serialize(v);
	return s;
}
inline datSerialize & operator<< (datSerialize &s, float &v) {
	s.Serialize(v);
	return s;
}

// PURPOSE: Act as a generic "catch all" case.  Any class that doesn't explicitly override
//	the operator<< will have it's Serialize method called.
//	If that Serialize method does not exist, a compile error will result.
//
//	** DEALING WITH COMPILER ERRORS WITH BELOW CODE **
//		If you are trying to serialize data and you hit a compile error in this code you may:
//		-- Create a global operator<< function that serializes the data
//			This is useful for serializing third party classes without having to
//			directly modify the source code.
//			i.e.:   inline datSerialize & operator(datSerialize &ser, SomeClass &foo)
//		-- Add a method to the class that generates this error, called Serialize.
//			This is generally a cleaner solution and is easier for people to follow.
//			i.e.:   void SomeClass::Serialize(datSerialize &ser);
//		NOTE THAT THE GLOBAL OPERATOR FUNCTION TAKES PRECEDENT OVER THE SERIALIZE METHOD
template<class T> datSerialize & operator<< (datSerialize &s, T &obj ) {
	// IMPORTANT -- If you get a compiler error here, see above PURPOSE block for info on
	//		how to resolve the error.
	obj.Serialize(s);
	return s;
}



// Manipulators -- manipulators are function calls made from within the serialization stream
//	You can use them for whatever you see fit.  RAGE has an example with the datNewLine
//	Example of how to add a "manipulator":
//		datSerialize & datShowReadStatus( datSerialize &s ) { Displayf("Read is %s", s.IsRead() ? "on" : "off"); return s; }
//	Then, your serialization code would have something like:
//		ser << val1 << val2 << datShowReadStatus << val3 << val4;
//	Notice that no parenthesis were used after datShowReadStatus
inline datSerialize & operator<< (datSerialize &s, datSerialize & (*func)(datSerialize &) ) {
	return ( func(s) );
}

// These classes are intended only to be created and passed into the
//	serializer object.  It is really only useful as a helper -- you
//	are free to do the same thing yourself, but these helpers may have
//	future support for optimizations.
//	Usage Example:
//		char text[50];
//		ser << count << datArray<Vector3>(myArray, count) << datString(text, 50)
//
//	NOTE: These classes also show simple examples of how to add support
//		for serialization in your custom classes
//
// -- Array helper
template <class T, class CountType=int> class datArray {
public:
	// PURPOSE: This version will allocate data as needed
	datArray(T **data, CountType *count);

	// PURPOSE: This version reuses data
	datArray(T *data, CountType count);

private:
	T *			m_Data;
	T **		m_AllocData;			// Only used when allocations are needed
	CountType *	m_AllocCount;
	CountType	m_Count;

private:
	// This function is tricky, mainly because it has to be declared as "const" so that a datArray can be declared as a
	//	function argument.  This doesn't appear to be mentioned in any texts on C++
	friend datSerialize & operator<< ( datSerialize &ser, const datArray &array ) {
		CountType count = array.m_Count;
		ser << count;
		// If this template was configured to allocate memory, do it now
		T *data = array.m_Data;
		if ( array.m_AllocData != 0 && ser.IsRead() == true && array.m_Count < count )	{
			data = rage_new T[count];
			// Free old memory
			if ( *array.m_AllocData )
				delete[] *array.m_AllocData;
			*array.m_AllocData = data;
			*array.m_AllocCount = count;
		}
		for (CountType i = 0; i < count; ++i)
			ser << data[i];
		return ser;
	}
};

// -- Label helper
//	Labels are only used in text files, and they are there to only help the reader
//	Labels are ignored on load
class datLabel {
public:
	datLabel(const char *text);

private:
	const char *m_Text;

private:
#ifdef __SNC__
	friend datSerialize & operator<< ( datSerialize &ser, datLabel &label ) {
		if ( ser.IsBinary() == false ) {
			if ( ser.IsRead() )
				ser.Get(0,0);	// Ignore it
			else{
				ser.Put(label.m_Text);
			}
		}
		return ser;
	}
#endif

	friend datSerialize & operator<< ( datSerialize &ser, const datLabel &label ) {
		if ( ser.IsBinary() == false ) {
			if ( ser.IsRead() )
				ser.Get(0,0);	// Ignore it
			else{
				ser.Put(label.m_Text);
			}
		}
		return ser;
	}
};

// -- String helper
//	Strings are written out to both text & binary streams, and are needed for data
//	purposes.
class datString {
public:
	// PURPOSE: This version will allocate data as needed
	datString(char **text, int *inoutSize);
	// PURPOSE: This version reuses data
	datString(char *text, int maxBuffSize);

private:
	char *		m_Text;
	int			m_MaxSize;
	// Allocation data
	char **		m_AllocText;
	int	*		m_OutSize;

private:
#ifdef __SNC__
	friend datSerialize & operator<< ( datSerialize &ser, datString &str ) {
		if ( ser.IsRead() )	{
			int count = str.m_MaxSize;
			int maxSize = str.m_MaxSize;
			if ( str.m_AllocText ) {
				// Need a size if we can grow the string to fit
				ser << count;
				if ( count > str.m_MaxSize ) {
					maxSize = count;
				}
			}
			char *text = str.m_Text;
			if ( str.m_AllocText && str.m_MaxSize <= count ) {
				text = rage_new char[count];
				maxSize = count;
				// Free old data
				if ( *str.m_AllocText )
					delete[] *str.m_AllocText;
				*str.m_AllocText = text;
			}
			ser.Get( text, maxSize );
			if ( maxSize > 0 ) {
				text[maxSize-1] = '\0';
			}
			*str.m_OutSize = maxSize;
		}
		else {
			if ( str.m_AllocText ) {
				s32 len = StringLength(str.m_Text) + 1;
				ser << len;
			}
			ser.Put( str.m_Text );
		}
		return ser;
	}
#endif
	friend datSerialize & operator<< ( datSerialize &ser, const datString &str ) {
		if ( ser.IsRead() )	{
			int count = str.m_MaxSize;
			int maxSize = str.m_MaxSize;
			if ( str.m_AllocText ) {
				// Need a size if we can grow the string to fit
				ser << count;
				if ( count > str.m_MaxSize ) {
					maxSize = count;
				}
			}
			char *text = str.m_Text;
			if ( str.m_AllocText && str.m_MaxSize <= count ) {
				text = rage_new char[count];
				maxSize = count;
				// Free old data
				if ( *str.m_AllocText )
					delete[] *str.m_AllocText;
				*str.m_AllocText = text;
			}
			ser.Get( text, maxSize );
			if ( maxSize > 0 ) {
				text[maxSize-1] = '\0';
			}
			*str.m_OutSize = maxSize;
		}
		else {
			if ( str.m_AllocText ) {
				s32 len = StringLength(str.m_Text) + 1;
				ser << len;
			}
			ser.Put( str.m_Text );
		}
		return ser;
	}
};


// **************
// MANIPULATORS
// **************
// -- Newline -- only valid for ascii files
datSerialize & datNewLine( datSerialize &s );



template<class T, class CountType> datArray<T, CountType>::datArray(T **data, CountType *count) 
: m_Data(*data)
, m_AllocData(data)
, m_AllocCount(count)
, m_Count(*count) 
{
}


template<class T, class CountType> datArray<T, CountType>::datArray(T *data, CountType count) 
: m_Data(data)
, m_AllocData(0)
, m_Count(count) 
{
}

inline datLabel::datLabel(const char *text) 
: m_Text(text) 
{
	AssertMsg((strchr(text, ' ') == 0 || text[StringLength(text)-1] == ' ') , "No spaces allowed in labels" );
}

inline datString::datString(char **text, int *inoutSize) 
: m_Text(*text)
, m_MaxSize(0)
, m_AllocText(text)
, m_OutSize(inoutSize) 
{
	if ( inoutSize ) {
		m_MaxSize = *inoutSize;
	}
	else {
		m_OutSize = &m_MaxSize;
	}
}

inline datString::datString(char *text, int maxBuffSize) 
: m_Text(text)
, m_MaxSize(maxBuffSize)
, m_AllocText(0)
, m_OutSize(&m_MaxSize)
{
}


} // namespace rage


#endif	// DATA_SERIALIZE_H
