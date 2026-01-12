// 
// data/rson.h 
// 
// Copyright (C) Forever Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_RSON_H
#define DATA_RSON_H

#include "string/string.h"
#include "atl/string.h"

/*
    RSON is a text based data definition language inspired by
    JSON (http://www.json.org), which is based on JavaScript, though
    RSON more closely resembles Lua.

    RSON consists of a hierarchy of objects.  Objects can have names.
    Composite objects are surrounded by '{' and '}' and contain a comma
    separate collection of more objects.

    RSON is intended to be as expressive as XML but far less verbose.

    Example RSON:

    config=
    {
        Windows=
        {
            MainWindow={x=100,y=300,w=600,h=400,background={color=red,image="c:/foo.jpg"},
            LogWindow={x=32,y=30,w=500,h=100,background={color=white}
        },
        RecentFiles=
        {
            "c:/Documents/jackass.doc",
            "c:/Documents/YearEndSummary.xls",
            "c:/Temp/MyCV.doc"
        }
    }

    RsonWriter is used to compose RSON objects, while RsonReader is used to
    parse RSON objects.

    RSON's initial purpose was to construct an optimized request/response
    protocol for accessing web services from game consoles.  Between the
    console and the web server sits a proxy that transforms RSON requests
    into SOAP and/or REST-ful web service requests and forwards them to the
    appropriate web server.  The proxy receives the response from the server,
    transforms it to RSON format, and forwards it to the game console.

    There are C# versions of RsonWriter and RsonReader located in the RockOn
    project directory.
*/

namespace rage
{

class Rson
{
public:

    //PURPOSE
    //  Returns true if the character is reserved in RSON.
    static bool IsReserved(const char c);

    //PURPOSE
    //  Returns true if the character is whitespace.
    static bool IsSpace(const char c);

	//PURPOSE
	//  Allows us to disable the snscanf optimization via tunable
	//  if we find problems. Can be removed in a future TU.
	static void SetAllowSnscanf(const bool snscanf);

	static bool sm_AllowSnscanf;
};

enum RsonFormat
{
    RSON_FORMAT_INVALID     = -1,
    RSON_FORMAT_LUA,
    RSON_FORMAT_JSON,
    RSON_FORMAT_BINARY
};

class RsonWriter
{
public:

    static const RsonFormat DEFAULT_FORMAT  = RSON_FORMAT_LUA;

	static const char* FLOAT_WRITE_FORMAT;

    RsonWriter();

    //PURPOSE
    //  Construct an RsonWriter which will write to the given buffer.
    RsonWriter(char* buf, const unsigned lenofBuf, const RsonFormat format);
    template<int SIZE>
    RsonWriter(char (&buf)[SIZE], const RsonFormat format)
    {
        Init(buf, SIZE, format);
    }

    //PURPOSE
    //  Initialize an RsonWriter that will write to the given buffer.
    void Init(char* buf, const unsigned lenofBuf, const RsonFormat format);
    template<int SIZE>
    void Init(char (&buf)[SIZE], const RsonFormat format)
    {
        Init(buf, SIZE, format);
    }

    //PURPOSE
    //  Initialize an RsonWriter that doesn't write to any buffer, but
    //  will calculate the buffer size required for what's written.
    void InitNull(const RsonFormat format);

    //PURPOSE
    //  Reset to the post initialization state.
    //NOTES
    //  The buffer passed to Init() will continue to be used.
    //  Compatibility flags will persist.
    void Reset();

    //PURPOSE
    //  Reset to the post construction state, but save the current
    //  "comma" state.  IOW, if another item is written  the comma
    //  logic will proceed as if there were prior items added.
    //NOTES
    //  This is used to continue writing after the buffer has been filled
    //  and flushed, which can occur if the buffer is not large enough
    //  to contain the entire collection of items.
    void ResetToContinue();

    //PURPOSE
    //  Returns the format of the generated string.
    RsonFormat GetFormat() const;

    //PURPOSE
    //  Begins a new named object and returns the prior cursor position.
    //  Appends '<name>={'.
    //PARAMS
    //  name        - Name of object.  Can be NULL for unnamed objects.
    //  cursor      - If non-NULL will contain prior cursor position on return.
    //NOTES
    //  Used to begin an object and preserve the cursor position in case
    //  the caller needs to "undo".
    bool Begin(const char* name, int* cursor);

    //PURPOSE
    //  Begins a new named array and returns the prior cursor position.
    //  Appends '<name>=['.
    //PARAMS
    //  name        - Name of object.  Can be NULL for unnamed arrays.
    //  cursor      - If non-NULL will contain prior cursor position on return.
    //NOTES
    //  Used to begin an array and preserve the cursor position in case
    //  the caller needs to "undo".
    bool BeginArray(const char* name, int* cursor);

	//PURPOSE
	//  Returns amount of space available in our buffer
	int Available() const;

	//PURPOSE
	//  Returns total size of buffer
	int Size() const;

    //PURPOSE
    //  Ends the current object.  Appends a '}'.
    bool End();

    //PURPOSE
    //  Returns the current cursor position.
    int GetCursor() const;

    //PURPOSE
    //  Returns the current depth of sub-objects.
    //NOTES
    //  For exampe, for the object {foo={bar={baz=123}}}, the maximum depth is thre
    //  for the "baz" element.
    int GetDepth() const;

    //PURPOSE
    //  Undo back to the position where the cursor was saved (see Begin()).
    //PARAMS
    //  cursor      - Cursor position saved from a call to Begin().
    //NOTES
    //  If HasError() returns true prior to calling Undo(), and Undo()
    //  succeeds, HasError() will subsequently return false.
    void Undo(const int cursor);

    //PURPOSE
    //  Write an integer value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteInt(const char* name, const int value);

    //PURPOSE
    //  Write a signed 64-bit value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteInt64(const char* name, const s64 value);

    //PURPOSE
    //  Write an unsigned value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteUns(const char* name, const unsigned value);

    //PURPOSE
    //  Write an unsigned 64-bit value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteUns64(const char* name, const u64 value);

    //PURPOSE
    //  Write a float value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteFloat(const char* name, const float value, const char* format = FLOAT_WRITE_FORMAT);

    //PURPOSE
    //  Write an unsigned value in hexadecimal, preceded by 0x.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteHex(const char* name, const unsigned value);

    //PURPOSE
    //  Write an unsigned 64-bit value in hexadecimal, preceded by 0x.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteHex64(const char* name, const u64 value);

	//PURPOSE
	//  Write an double value.
	//PARAMS
	//  name        - Name of the value.  Can be NULL.
	//  value       - The value.
	bool WriteDouble(const char* name, const double value);

    //PURPOSE
    //  Encodes binary to base64 (RFC 4648).  This implementation does
    //  not add padding as defined by RFC 4648.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - Bytes to encode.
    //  len         - Number of bytes
    bool WriteBinary(const char* name, const void* value, const unsigned len);

    //PURPOSE
    //  Write a boolean value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteBool(const char* name, const bool value);

    //PURPOSE
    //  Write a string value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteString(const char* name, const char* value);

    //PURPOSE
    //  Write a raw string value.  The value will not be enclosed
    //  in quotes.  Special characters will not be escaped.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    bool WriteRawString(const char* name, const char* value);

    //PURPOSE
    //  Write a value.
    //PARAMS
    //  name        - Name of the value.  Can be NULL.
    //  value       - The value.
    //NOTES
    //  This function is specialized in the .cpp.
    template<typename T>
    bool WriteValue(const char* name, T value);

	//PURPOSE
	//  Writes a user-defined type.
	//  Must be implemented by the application.
	//  e.g. bool WriteRson(const char* name, RsonWriter* rw) const;
	//PARAMS
	//  name        - Name of the value.  Can be NULL.
	//  t			- The value to write.
	template<typename T>
	bool WriteUser(const char* name, T& t)
	{
		 return t.WriteRson(name, this);
	}

    //PURPOSE
    //  Write an array of values surrounded by '{' '}'.
    //PARAMS
    //  name        - Name of the array.  Can be NULL.
    //  values      - The values.
    //  count       - Number of values to write.
    template<typename T>
    bool WriteArray(const char* name, T* values, const unsigned count)
    {
        bool success = this->BeginArray(name, NULL);
        for(int i = 0; i < (int) count && success; ++i)
        {
            success = this->WriteValue(NULL, values[i]);
        }
        return success && this->End();
    }

	// PURPOSE
	//	Inserts a string into the Rson buffer
	bool Insert(const char* string);

    //PURPOSE
    //  Returns true if there was an error while writing.
    //NOTES
    //  If there's an error no further writing will occur until one
    //  of the Reset() functions is called.
    bool HasError() const;

    //PURPOSE
    //  Return the null-terminated string that has been built.
    const char* ToString() const;

    //PURPOSE
    //  Returns the length of the string that has been built.
    //  The length does not include the terminating null.
    unsigned Length() const;

    static int TestMe();

private:

    bool NeedComma(const char c) const;

    bool NeedComma() const;

    const char* Comma() const;

    template<typename T>
    bool Write(const char* fmt, const char* name, const T& value)
    {
        if(name && this->InArray())
        {
            AssertMsg(false, "Array elements can't have names");
            m_Error = true;
        }
        else if(RSON_FORMAT_JSON == m_Format && !name && this->InObject())
        {
            AssertMsg(false, "JSON elements must have names");
            m_Error = true;
        }
        else if(!m_Error)
        {
            char tmpFmt[64];
            char buf[1024];

            if(name)
            {
                if(RSON_FORMAT_JSON == m_Format)
                {
                    formatf(tmpFmt, sizeof(tmpFmt), "%s\"%s\":%s", this->Comma(), name, fmt);
                }
                else
                {
                    formatf(tmpFmt, sizeof(tmpFmt), "%s%s=%s", this->Comma(), name, fmt);
                }
            }
            else
            {
                formatf(tmpFmt, sizeof(tmpFmt), "%s%s", this->Comma(), fmt);
            }

            formatf(buf, sizeof(buf), tmpFmt, value);

			m_Error = !this->Write(buf);
        }

        return !m_Error;
    }

	bool Write(const char* buf);

	bool Write(const char c);

    bool InObject() const;

    bool InArray() const;

    unsigned m_State;
    int m_Depth;

    char* m_Buf;
    int m_Cursor;
    unsigned m_LenofBuf;

    RsonFormat m_Format;

    bool m_Error        : 1;
    bool m_NeedComma    : 1;
    bool m_NullBuffer   : 1;    //True if InitNull was called.
};

class RsonReader
{
public:

    RsonReader();
    RsonReader(const char* buf, const unsigned lenofBuf);
    template<int SIZE>
    RsonReader(const char (&buf)[SIZE])
    {
        Init(buf);
    }

    //PURPOSE
    //  Initializes the reader with a nul-terminated string.
    //PARAMS
    //  buf         - The string.
    //  offset      - Offset in string at which to start parsing.
    //  lenofBuf    - Length of string buffer.
    //NOTES
    //  The initializing string is not copied.  It must remain valid
    //  for the lifetime of the RsonReader instance, and any RsonReader
    //  instance that is initialized by calling GetFirstMember() and
    //  GetNextSibling().
    bool Init(const char* buf, const unsigned lenofBuf);
    bool Init(const char* const buf, const int offset, const unsigned lenofBuf);
    template<int SIZE>
    bool Init(const char (&buf)[SIZE])
    {
        return Init(buf, SIZE);
    }

    //PURPOSE
    //  Clears the reader to the post-construction state.
    void Clear();

    //PURPOSE
    //  Returns true if the reader has been initialized with valid RSON data.
    bool HasData() const;

    //PURPOSE
    //  Returns true if the named member is present.
    bool HasMember(const char* name) const;

    //PURPOSE
    //  Retrieves the first member in the collection.
    //PARAMS
    //  rr      - On successful return will reference the first member.
    bool GetFirstMember(RsonReader* rr) const;

    //PURPOSE
    //  Retrieves the current member's next sibling.
    //PARAMS
    //  rr      - On successful return will reference the sibling.
    bool GetNextSibling(RsonReader* rr);

    //PURPOSE
    //  Retrieves the named member from the collection.
    //PARAMS
    //  name    - Name of the member to retrieve.
    //  rr      - On successful return will reference the first member.
    bool GetMember(const char* name, RsonReader* rr) const;

	//PURPOSE
	//  Checks if the named member has duplicate entries
	//PARAMS
	//  name    - Name of the member to check.
	bool HasDuplicateEntries(const char* name) const;

	//PURPOSE
	//  Returns true if the current object has a name.
	bool HasName() const;

	//PURPOSE
    //  Retrieves the current object's name.
    //PARAMS
    //  name        - Buffer in which to copy the name.
    //  maxChars    - Number of chars available in the buffer.
    bool GetName(char* name, const int maxChars) const;
    template<int SIZE>
    bool GetName(char (&name)[SIZE]) const
    {
        return GetName(name, SIZE);
    }

    //PURPOSE
    //  Retrieves the current object's value.
    //PARAMS
    //  val         - Buffer in which to copy the value.
    //  maxChars    - Number of chars available in the buffer.
    bool GetValue(char* val, const int maxChars) const;
    template<int SIZE>
    bool GetValue(char (&val)[SIZE]) const
    {
        return GetValue(val, SIZE);
    }

    //PURPOSE
    //  Retrieves the named value from the collection.
    //PARAMS
    //  name        - Name of the value to retrieve.
    //  val         - Buffer in which to copy the value.
    //  maxChars    - Number of chars available in the buffer.
    bool GetValue(const char* name, char* val, const int maxChars) const;
    template<int SIZE>
    bool GetValue(const char* name, char (&val)[SIZE]) const
    {
        return GetValue(name, val, SIZE);
    }

    //PURPOSE
    //  Returns the number of chars required to retrieve the value.
    //NOTES
    //  *** Does not include terminating NULL. ***
    int GetValueLength() const;

    //PURPOSE
    //  Returns the number of chars required to retrieve the value with the
    //  given name.
    //NOTES
    //  *** Does not include terminating NULL. ***
    int GetValueLength(const char* name) const;

    //PURPOSE
    //  Returns the number of bytes that will result from interpreting
    //  the value as a base-64 encoded binary value.  I.e., the number
    //  of bytes required to retrieve the value as binary.
    int GetBinaryValueLength() const;

    //PURPOSE
    //  Returns the number of bytes that will result from interpreting
    //  the value as a base-64 encoded binary value.  I.e., the number
    //  of bytes required to retrieve the value as binary.
    int GetBinaryValueLength(const char* name) const;

    //PURPOSE
    //  Converts the current value to an integer.
    bool AsInt(int& val) const;

	//PURPOSE
	//  Converts the current value to a signed 8-bit number.
	bool AsInt8(s8& val) const;

	//PURPOSE
	//  Converts the current value to a signed 16-bit number.
	bool AsInt16(s16& val) const;

    //PURPOSE
    //  Converts the current value to a signed 64-bit number.
    bool AsInt64(s64& val) const;

    //PURPOSE
    //  Converts the current value to an unsigned.
    bool AsUns(unsigned& val) const;

	//PURPOSE
	//  Converts the current value to an unsigned 16-bit number.
	bool AsUns8(u8& val) const;

	//PURPOSE
	//  Converts the current value to an unsigned 16-bit number.
	bool AsUns16(u16& val) const;

    //PURPOSE
    //  Converts the current value to an unsigned 64-bit number.
    bool AsUns64(u64& val) const;

	//PURPOSE
	//  Converts the current value to a float.
	bool AsFloat(float& val) const;

	//PURPOSE
	//  Converts the current value to a double.
	bool AsDouble(double& val) const;

    //PURPOSE
    //  Converts the current value to a binary value.
    //PARAMS
    //  val     - Destination buffer
    //  maxLen  - Number of bytes in the destination buffer.
    //  len     - On success contains the number of bytes decoded.
    //            On failure, contains the number of bytes required for
    //            decoding.
    //NOTES
    //  Assumes the value is in base-64 format.
    bool AsBinary(void* val, const int maxLen, int* len) const;

    //PURPOSE
    //  Converts the current value to a boolean.
    bool AsBool(bool& val) const;

	//PURPOSE
	//	Returns the current value to an atl string
	bool AsAtString(atString& string) const;

    //PURPOSE
    //  Converts the current value to a string.
    bool AsString(char* val, const int maxChars) const;
    template<int SIZE>
    bool AsString(char (&val)[SIZE]) const
    {
        return AsString(val, SIZE);
    }

    //PURPOSE
    //  Converts the current value to a raw string.  Characters are
    //  not un-escaped.
    bool AsRawString(char* val, const int maxChars) const;
    template<int SIZE>
    bool AsRawString(char (&val)[SIZE]) const
    {
        return AsRawString(val, SIZE);
    }

    //PURPOSE
    //  Uses the current value to initialize an RSON reader.
    //  This is more efficient than extracting the value as
    //  a string to initialize an RSON reader.
    bool AsReader(RsonReader& rr) const;

    //PURPOSE
    //  Converts the current value to the given type.
    //NOTES
    //  This function is specialized in the .cpp.
    template<typename T>
    bool AsValue(T& value) const;
    template<int SIZE>
    bool AsValue(char (&val)[SIZE]) const
    {
        return AsString(val, SIZE);
    }

    //PURPOSE
    //  Checks the name and converts the current value to the given type.
    //PARAMS
    //  name        - Name of the value.
    //  value       - The value.
    //NOTES
    //  This function is specialized in the .cpp.
    //  If the name doesn't match the function will fail.
    //  Example 1:
    //      const char buf[] = "{\"foo\":123}";
    //      RsonReader rr(buf);
    //      rr.GetFirstMember(&rr);
    //      int val;
    //      rr.AsValue("foo", val); //Succeeds
    //      rr.AsValue("bar", val); //Fails
    //  Example 2:
    //      const char buf[] = "\"foo\":123";   //No curlies
    //      RsonReader rr(buf);
    //      int val;
    //      rr.AsValue("foo", val);   //Succeeds
    //      rr.AsValue("bar", val);   //Fails
    template<typename T>
    bool AsValue(const char* name, T& value) const
    {
        return this->CheckName(name) && this->AsValue(value);
    }

    //PURPOSE
    //  Converts the current value to an array of T.
    //PARAMS
    //  values      - The values.
    //  count       - Number of values to read.
    //RETURNS
    //  Number of values read.
    template<typename T>
    int AsArray(T* values, const unsigned count) const
    {
        RsonReader rr;
        bool success = this->GetFirstMember(&rr);
        int numRead;
        for(numRead = 0; numRead < (int) count && success; ++numRead)
        {
            success = rr.AsValue(values[numRead]);

            if(success && !rr.GetNextSibling(&rr))
            {
                break;
            }
        }
        return success ? numRead + 1 : 0;
    }

    //PURPOSE
    //  Returns true if the value of the object is null
    bool IsNull() const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as an integer.
    bool ReadInt(const char* name, int& val) const;

	//PURPOSE
	//  Finds the child element with the given name
	//  and reads the value as an short.
	bool ReadInt16(const char* name, s16& val) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as a signed 64-bit value.
    bool ReadInt64(const char* name, s64& val) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as an unsigned.
    bool ReadUns(const char* name, unsigned& val) const;

	//PURPOSE
	//  Finds the child element with the given name
	//  and reads the value as an unsigned 16-bit value.
	bool ReadUns16(const char* name, u16& val) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as an unsigned 64-bit value.
    bool ReadUns64(const char* name, u64& val) const;

	//PURPOSE
	//  Finds the child element with the given name
    //  and reads the value as a float.
	bool ReadFloat(const char* name, float& val) const;

	//PURPOSE
	//  Finds the child element with the given name
    //  and reads the value as a double.
	bool ReadDouble(const char* name, double& val) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as base-64 format.
    //PARAMS
    //  name    - Name of the value/
    //  val     - Destination buffer
    //  maxLen  - Number of bytes in the destination buffer.
    //  len     - On success contains the number of bytes decoded.
    //            On failure, contains the number of bytes required for
    //            decoding.
    bool ReadBinary(const char* name, void* val, const int maxLen, int* len) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as a boolean.
    bool ReadBool(const char* name, bool& val) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as a string.
    bool ReadString(const char* name, char* val, const int bufLen) const;
    template<int SIZE>
    bool ReadString(const char* name, char (&val)[SIZE]) const
    {
        return ReadString(name, val, SIZE);
    }

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as a raw string.  Characters are
    //  not un-escaped.
    bool ReadRawString(const char* name, char* val, const int bufLen) const;
    template<int SIZE>
    bool ReadRawString(const char* name, char (&val)[SIZE]) const
    {
        return ReadRawString(name, val, SIZE);
    }

	//PURPOSE
	//  Finds the child element with the given name
	//  and reads the value as an atString.
	bool ReadString(const char* name, atString& string) const;

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as an RsonReader.
    bool ReadReader(const char* name, RsonReader& val) const;

    template<typename T>
    bool ReadValue(const char* name, T& val) const
    {
        RsonReader rr;
        return this->GetMember(name, &rr) && rr.AsValue(val);
    }

    //PURPOSE
    //  Finds the child element with the given name
    //  and reads the value as an array of T.
    //PARAMS
    //  name        - Name of the item.
    //  values      - The values.
    //  count       - Number of values to read.
    //RETURNS
    //  Number of values read.
    template<typename T>
    int ReadArray(const char* name, T* values, const unsigned count) const
    {
        RsonReader rr;
        return this->GetMember(name, &rr) ? rr.AsArray(values, count) : 0;
    }

    //PURPOSE
    //  Returns true if the name of the object
    //  matches the given name.
    bool CheckName(const char* name, const bool ignoreNull = true) const;

	//PURPOSE:
	// Returns the start of the value string, including any leading quote characters if appropriate
	// Fills up buf, up to 'bufSize' characters or the length of the value string (potentially including leading and trailing quotes)
	void GetRawValueString(char* buf, int bufSize) const;

	ASSERT_ONLY(static int TestMe();)

	static bool ValidateJson(const char* buf, int bufSize);
	static bool ValidateJsonObject(const char* buf, int bufSize, bool bSkipBraces);

	//PURPOSE
	//	Get the string pointer to the beginning of the string that represents the value of this
	//		member in the buffer being used by this instance.
	//PARAMS
	//	name			- Name of the item.
	//	outStrLenghth	- length of the string being returned
	//RETURN
	//	pointer to the begining of the value chunk for this member.
	//  If the member doesn't exist, it will return NULL and outStrLength will be 0
	//NOTES
	//	The pointer returned and the length doesn't include the surrounding quotes. 
	//
	//  USE WITH CARE
	//
	const char* GetRawMemberValue(const char* name, int& outStrLength) const;

#if !__NO_OUTPUT
	const char* GetBuffer() const { return m_Buf; }
#endif

protected:

    void GetB64OffsetAndLength(int* offset, int* len) const;

    static int SkipWs(const char* buf, const int offset, const unsigned lenofBuf);

    static int EndOfToken(const char* buf, const int offset, const unsigned lenofBuf);

    static int EndOfString(const char* buf, const int offset, const unsigned lenofBuf);

    static int EndOfObject(const char* buf, const int offset, const unsigned lenofBuf);

    const char* Name() const;

	const char* Value() const;

    const char* m_Buf;
    unsigned m_LenofBuf;
    int m_NameOffset;
    int m_NameLen;
    int m_ValOffset;
    int m_ValLen;
};

enum BrsonType
{
    BRSONTYPE_INVALID   = -1,
    BRSONTYPE_OBJECT    = 0,
    BRSONTYPE_ARRAY     = 1,
    BRSONTYPE_STRING    = 2,
    BRSONTYPE_INT       = 3,
    BRSONTYPE_SINGLE    = 4,
    BRSONTYPE_DOUBLE    = 5,
    BRSONTYPE_TRUE      = 6,
    BRSONTYPE_FALSE     = 7,
};

class BrsonWriter
{
public:
    BrsonWriter();
    BrsonWriter(u8* buf, const unsigned len);
    template<int SIZE>
    BrsonWriter(u8 (&buf)[SIZE])
    {
        Init(buf, SIZE);
    }

    void Init(u8* buf, const unsigned len);
    template<int SIZE>
    void Init(u8 (&buf)[SIZE])
    {
        Init(buf, SIZE);
    }

    void Clear();

    bool Begin(const char* name, unsigned* cursor);

    bool End();

    bool WriteString(const char* name, const char* value);

    bool WriteInt(const char* name, const s64 value);

//private:

    bool WriteVarInt(u64 val);
    bool WriteString(const char* str);

    u8* m_OrigBuf;
    u8* m_Buf;
    unsigned m_Cursor;
    unsigned m_BufLength;
};

class BrsonReader
{
public:

    BrsonReader();

    bool Init(const u8* buf, const size_t len);
    template<size_t SIZE>
    bool Init(const u8 (&buf)[SIZE])
    {
        return Init(buf, SIZE);
    }

    void Clear();

    bool GetFirstMember(BrsonReader* rr) const;

    bool GetNextSibling(BrsonReader* rr) const;

    bool GetName(char* name, const unsigned maxChars) const;
    template<unsigned SIZEOFNAME>
    bool GetName(char (&name)[SIZEOFNAME]) const
    {
        return GetName(name, SIZEOFNAME);
    }

//private:

    static size_t ReadVarInt(const u8* buf, const size_t len, u64* varInt);

    const u8* m_ParentBuf;
    unsigned m_ParentLen;
    const u8* m_Buf;
    size_t m_Length;
};

}   //namespace rage

#endif  //DATA_RSON_H
