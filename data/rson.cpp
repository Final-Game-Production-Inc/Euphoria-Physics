// 
// data/rson.cpp 
// 
// Copyright (C) Forever Rockstar Games.  All Rights Reserved. 
// 

#include "rson.h"
#include "data/base64.h"
#include "diag/channel.h"
#include "diag/output.h"
#include "string/unicode.h"
#include "system/memops.h"
#include "system/nelem.h"
#include "system/param.h"

#include <stdio.h>
#include <string.h>

#if __PPU
#include <sdk_version.h>
#endif

#if RSG_WIN32
// avoid sscanf's call to strlen() by specifying the length - this increases performance when parsing large strings
#define RSON_SCANF(input, length, format, ...) (Rson::sm_AllowSnscanf ? _snscanf(input, length, format, __VA_ARGS__) : sscanf(input, format, __VA_ARGS__))
#else
// the length is ignored for platforms that don't have _snscanf - note: not all implementations call strlen()
#define RSON_SCANF(input, length, format, ...) sscanf(input, format, __VA_ARGS__)
#endif

RAGE_DECLARE_CHANNEL(rson)
RAGE_DEFINE_CHANNEL(rson)

#define rsonDebug(fmt, ...)     RAGE_DEBUGF1(rson, fmt, ##__VA_ARGS__)
#define rsonWarning(fmt, ...)   RAGE_WARNINGF(rson, fmt, ##__VA_ARGS__)
#define rsonError(fmt, ...)     RAGE_ERRORF(rson, fmt, ##__VA_ARGS__)

namespace rage
{

#if __DEV && __ASSERT
static const int TESTED_WRITER = RsonWriter::TestMe();
static const int TESTED_READER = RsonReader::TestMe();
#endif

// out of an abundance of caution, the optimization can be disabled via tunable for the next TU.
// We can consider removing this tunable after the next TU.
bool Rson::sm_AllowSnscanf = true;

//////////////////////////////////////////////////////////////////////////
//  Rson
//////////////////////////////////////////////////////////////////////////
bool
Rson::IsReserved(const char c)
{
    static const char RESERVED_CHARS[] = {"{}[]=,:"};
    return (NULL != strchr(RESERVED_CHARS, c));
}

bool
Rson::IsSpace(const char c)
{
    return (c >= 0x09 && c <= 0x0D) || (0x20 == c);
}

void
Rson::SetAllowSnscanf(const bool snscanf)
{
	if(sm_AllowSnscanf != snscanf)
	{
		rsonDebug("SetAllowSnscanf :: %s", snscanf ? "true" : "false");
		sm_AllowSnscanf = snscanf;
	}
}
//////////////////////////////////////////////////////////////////////////
//  RsonWriter
//////////////////////////////////////////////////////////////////////////
RsonWriter::RsonWriter()
{
    this->Init(NULL, 0, DEFAULT_FORMAT);
}

RsonWriter::RsonWriter(char* buf, const unsigned lenofBuf, const RsonFormat format)
{
    this->Init(buf, lenofBuf, format);
}

void
RsonWriter::Init(char* buf, const unsigned lenofBuf, const RsonFormat format)
{
    m_Buf = buf;
    m_LenofBuf = lenofBuf;
    m_Format = format;
    this->Reset();
}

void
RsonWriter::InitNull(const RsonFormat format)
{
    this->Init(NULL, 0, format);
    m_NullBuffer = true;
}

void
RsonWriter::Reset()
{
    m_State = 0;
    m_Depth = 0;
    m_Cursor = 0;
    //Don't clear compatibility flags.
    m_Error = false;
    m_NeedComma = false;
    m_NullBuffer = false;
}

void
RsonWriter::ResetToContinue()
{
    //Preserve some vars.
    const bool needComma = this->NeedComma();
    const unsigned state = m_State;
    const int depth = m_Depth;
    this->Reset();
    m_NeedComma = needComma;
    m_State = state;
    m_Depth = depth;
}

bool
RsonWriter::Begin(const char* name, int* cursor)
{
    if(AssertVerify(m_Depth + 1 <= 32))
    {
	    if(cursor)
	    {
		    *cursor = m_Cursor;
	    }

        if(!m_Error)
        {
            this->Write("%c", name, '{');
        }

        if(!m_Error)
        {
            m_State = (m_State << 1) | 0x01;
            ++m_Depth;
        }
    }
    else
    {

        m_Error = true;
    }

    return !m_Error;
}

bool
RsonWriter::BeginArray(const char* name, int* cursor)
{
    if(AssertVerify(m_Depth + 1 <= 32))
    {
	    if(cursor)
	    {
		    *cursor = m_Cursor;
	    }

        if(!m_Error)
        {
            if(m_Format == RSON_FORMAT_JSON)
            {
                this->Write("%c", name, '[');
            }
            else
            {
                this->Write("%c", name, '{');
            }
        }

        if(!m_Error)
        {
            m_State = (m_State << 1);
            ++m_Depth;
        }
    }
    else
    {

        m_Error = true;
    }

    return !m_Error;
}

bool
RsonWriter::End()
{
    bool success = false;

    if(!AssertVerify(m_Depth > 0))
    {
        m_Error = true;
    }
    else
    {
        if(this->InObject())
        {
            success = this->Write('}');
        }
        else if(this->InArray())
        {
            if(RSON_FORMAT_JSON == m_Format)
            {
                success = this->Write(']');
            }
            else
            {
                success = this->Write('}');
            }
        }

        if(success)
        {
            --m_Depth;
            m_State >>= 1;
        }
    }

    return success;
}

int
RsonWriter::GetCursor() const
{
    return m_Cursor;
}

int
RsonWriter::GetDepth() const
{
    return m_Depth;
}

void
RsonWriter::Undo(const int cursor)
{
    m_Error = false;    //Clear the error before calling Write().

    if(cursor != m_Cursor)
    {
        Assert(cursor >= 0);
        Assert(cursor < m_Cursor);
        Assert(m_NullBuffer || '{' == m_Buf[cursor] || '[' == m_Buf[cursor] || ',' == m_Buf[cursor]);

        //Undo all the updates to m_Depth and m_State that occurred in the
        //section of the string that we're undoing.
        int deltaDepth = 0;
        for(int i = cursor; i < m_Cursor; ++i)
        {
            const char c = m_Buf[i];
            if(RSON_FORMAT_JSON == m_Format)
            {
                if('{' == c || '[' == c)
                {
                    --deltaDepth;
                }
                if('}' == c || ']' == c)
                {
                    ++deltaDepth;
                }
            }
            else if('{' == c)
            {
                --deltaDepth;
            }
            else if('}' == c)
            {
                ++deltaDepth;
            }
        }

        Assert(deltaDepth <= 0);
        m_Depth += deltaDepth;
        m_State >>= -deltaDepth;
        Assert(m_Depth >= 0);

        m_Cursor = cursor;
        this->Write('\0');
        --m_Cursor;
        Assert(m_Cursor >= 0);
        if(m_Cursor > 0)
        {
            m_NeedComma = this->NeedComma(m_Buf[m_Cursor-1]);
        }
        else
        {
            m_NeedComma = false;
        }
    }
}

bool
RsonWriter::WriteInt(const char* name, const int value)
{
    return this->Write("%d", name, value);
}

bool
RsonWriter::WriteInt64(const char* name, const s64 value)
{
    return this->Write("%" I64FMT "d", name, value);
}

bool
RsonWriter::WriteUns(const char* name, const unsigned value)
{
    return this->Write("%u", name, value);
}

bool
RsonWriter::WriteUns64(const char* name, const u64 value)
{
    return this->Write("%" I64FMT "u", name, value);
}

const char* RsonWriter::FLOAT_WRITE_FORMAT = "%.5f";
static const char* DOUBLE_WRITE_FORMAT = "%.5f";
static const char* FLOAT_READ_FORMAT = "%f";
static const char* DOUBLE_READ_FORMAT = "%lf";

bool
RsonWriter::WriteFloat(const char* name, const float value, const char* format)
{
    return this->Write(format, name, value);
}

bool
RsonWriter::WriteHex(const char* name, const unsigned value)
{
    if(RSON_FORMAT_JSON == m_Format)
    {
        char tmp[64];
        formatf(tmp, sizeof(tmp), "0x%X", value);
        return this->WriteString(name, tmp);
    }
    else
    {
        return this->Write("0x%X", name, value);
    }
}

bool
RsonWriter::WriteHex64(const char* name, const u64 value)
{
    if(RSON_FORMAT_JSON == m_Format)
    {
        char tmp[64];
        formatf(tmp, sizeof(tmp), "0x%" I64FMT "X", value);
        return this->WriteString(name, tmp);
    }
    else
    {
	    return this->Write("0x%" I64FMT "X", name, value);
    }
}

bool
RsonWriter::WriteDouble(const char* name, const double value)
{
	return this->Write(DOUBLE_WRITE_FORMAT, name, value);
}

bool
RsonWriter::WriteBinary(const char* name, const void* value, const unsigned len)
{
    if(len == 0)
    {
        //We don't write null binary, but have to allow it to succeed.
        return true;
    }

    bool success;
    
    if(RSON_FORMAT_JSON == m_Format)
    {
        success = this->Write("%s", name, "\"");
    }
    else
    {
        success = this->Write("%s", name, "");
    }

    unsigned numConsumed;
    if(!m_NullBuffer)
    {
        success = datBase64::Encode((const u8*) value,
                                    len,
                                    &m_Buf[m_Cursor],
                                    this->Available(),
                                    &numConsumed);
    }
    else
    {
        numConsumed = datBase64::GetMaxEncodedSize(len);
        success = true;
    }

    if(success)
    {
        //Subtract 1 for the terminating null
        m_Cursor += numConsumed - 1;
    }
    else
    {
        m_Error = true;
    }

    if(success && RSON_FORMAT_JSON == m_Format)
    {
        success = this->Write('\"');
    }

    return success;
}

bool
RsonWriter::WriteBool(const char* name, const bool value)
{
    return this->Write("%s", name, value ? "true" : "false");
}

bool
RsonWriter::WriteString(const char* name, const char* value)
{
	// we can have an array of strings, and the
	// strings in the array don't have names
    bool success = (AssertVerify(name || InArray()))
                    && AssertVerify(value)
					&& this->Write("%s", name, "");

    //Strings containing reserved characters will be enclosed in quotes.

    if(success)
    {
        const size_t len = strlen(value);
        bool quoteIt = (RSON_FORMAT_JSON == m_Format);

        for(int i = 0; i < (int) len && !quoteIt; ++i)
        {
            if(Rson::IsReserved(value[i]) || Rson::IsSpace(value[i]))
            {
                quoteIt = true;
            }
        }

        if(quoteIt)
        {
            success = this->Write('\"');
        }

        bool escaped = false;

        for(int i = 0; i < (int) len && success; ++i)
        {
            if(escaped)
            {
                escaped = false;
            }
            else if('\\' == value[i])
            {
                escaped = true;
            }
            else if('\"' == value[i])
            {
                success = this->Write('\\');
            }

            success = success && this->Write(value[i]);
        }

        if(quoteIt && success)
        {
            success = this->Write('\"');
        }
    }

    return success;
}

bool
RsonWriter::WriteRawString(const char* name, const char* value)
{
    return this->Write("%s", name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const int value)
{
    return this->WriteInt(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const s64 value)
{
    return this->WriteInt64(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const unsigned value)
{
    return this->WriteUns(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const float value)
{
    return this->WriteFloat(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const u64 value)
{
    return this->WriteUns64(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const bool value)
{
    return this->WriteBool(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, char* value)
{
    return this->WriteString(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const char* value)
{
    return this->WriteString(name, value);
}

template<>
bool
RsonWriter::WriteValue(const char* name, const double value)
{
	return this->WriteDouble(name, value);
}

// PURPOSE
//	Inserts a string into the Rson buffer
bool RsonWriter::Insert(const char* string)
{
	return Write(string);
}

bool
RsonWriter::HasError() const
{
    return m_Error;
}

const char*
RsonWriter::ToString() const
{
    return m_Buf;
}

unsigned
RsonWriter::Length() const
{
    return m_Cursor;
}

//private:

bool
RsonWriter::NeedComma(const char c) const
{
    if(RSON_FORMAT_JSON == m_Format)
    {
        return ('{' != c && '[' != c && ',' != c);
    }
    else
    {
        return ('{' != c && ',' != c);
    }
}

bool
RsonWriter::NeedComma() const
{
    return m_NeedComma;
}

const char*
RsonWriter::Comma() const
{
    return this->NeedComma() ? "," : "";
}

int
RsonWriter::Available() const
{
    int avail;
    if(!m_NullBuffer)
    {
        avail = m_LenofBuf - m_Cursor
                - 1;    //minus one for the terminating nul
    }
    else
    {
        avail = 0x7FFFFFFF;
    }

    return avail > 0 ? avail : 0;
}

int 
RsonWriter::Size() const
{
	if(!m_NullBuffer)
	{
		return m_LenofBuf;
	}
	return 0;
}

bool
RsonWriter::Write(const char* buf)
{
    if(!m_Error)
    {
        const int len = (int)strlen(buf);
        if(this->Available() >= len)
        {
            if(!m_NullBuffer)
            {
                sysMemCpy(&m_Buf[m_Cursor], buf, len);
                m_Buf[m_Cursor + len] = '\0';
            }

            m_NeedComma = this->NeedComma(buf[len-1]);

            m_Cursor += len;
        }
        else
        {
            m_Error = true;
        }
    }

    return !m_Error;
}

bool
RsonWriter::Write(const char c)
{
    if(!m_Error)
    {
        if(this->Available() >= 1)
        {
            if(!m_NullBuffer)
            {
                m_Buf[m_Cursor] = c;
                m_Buf[m_Cursor+1] = '\0';
            }

            m_NeedComma = this->NeedComma(c);
            ++m_Cursor;
        }
        else
        {
            m_Error = true;
        }
    }

    return !m_Error;
}

bool
RsonWriter::InObject() const
{
    return (m_Depth > 0) && (1 == (m_State & 0x01));
}

bool
RsonWriter::InArray() const
{
    return (m_Depth > 0) && (0 == (m_State & 0x01));
}

static void TestLuaWriter()
{
    char buf[1024];
    RsonWriter rw(buf, COUNTOF(buf), RSON_FORMAT_LUA);

    u8 binval[] = {'f','o','o','b','a','r'};
    int ints[] = {1,2,3,4};
    unsigned uns[] = {1,2,3,4};
	float floats[] = {1.5f,1.25f,1.125f,1.0625f};
	double doubles[] = {1234.0000000000000009,2.234577000001,1.30000030000457,1.200003400057};

	bool success = rw.Begin(NULL, NULL);
	Assert(success);
	success = rw.WriteDouble("double", 12340000000000000009.0E-016);
	Assert(success);
	success = rw.WriteArray("doubles", doubles, 4);
	Assert(success);
	success = rw.End();
#if __PPU
	Assert(0 == strcmp(rw.ToString(), "{double=1234.00000,doubles={1234.00000,2.23458,1.30000,1.20000}}"));
#else
	Assert(0 == strcmp(rw.ToString(), "{double=1234.00000,doubles={1234.00000,2.23458,1.30000,1.20000}}"));
#endif
	rw.Reset();

    success = rw.Begin(NULL, NULL);
    Assert(success);
    success = rw.WriteInt("int", 123);
    Assert(success);
    success = rw.WriteUns("uns", 0xFFFF1230);
    Assert(success);
    success = rw.WriteFloat("float", 1.334455f);
    Assert(success);
    success = rw.WriteHex("hex", 0xABCDEF);
    Assert(success);
    success = rw.WriteHex64("hex64", 0x123456789ABCDEFull);
    Assert(success);
	success = rw.WriteBinary("foobar", binval, COUNTOF(binval));
	Assert(success);
    success = rw.WriteBool("bool", true);
    Assert(success);
    success = rw.WriteString("string", "fuckoff");
    Assert(success);
    success = rw.WriteString("string2", "fuck off");
    Assert(success);
    success = rw.WriteString("string3", "\"fuck off\"");
    Assert(success);
    success = rw.WriteArray("ints", ints, 4);
    Assert(success);
    success = rw.WriteArray("uns", uns, 4);
    Assert(success);
	success = rw.WriteArray("floats", floats, 4);
	Assert(success);
    success = rw.End();
    Assert(success);

    Assert(0 == strcmp(rw.ToString(),
        "{int=123,uns=4294906416,float=1.33446,hex=0xABCDEF,hex64=0x123456789ABCDEF,foobar=Zm9vYmFy,bool=true,string=fuckoff,string2=\"fuck off\",string3=\"\\\"fuck off\\\"\",ints={1,2,3,4},uns={1,2,3,4},floats={1.50000,1.25000,1.12500,1.06250}}"));

    //Test calling End() when the buffer has been reset to continue
    //and nothing further has been added.  This occurs when the
    //underlying memory buffer was exhausted and the writer was reset
    //to continue, but there was no further data to write.
    rw.Reset();
    success = rw.Begin(NULL, NULL);
    Assert(success);
    success = rw.WriteInt("int", 123);
    Assert(success);
    rw.ResetToContinue();
    success = rw.End();
    Assert(success);

    Assert(0 == strcmp(rw.ToString(), "}"));
}

static void TestJsonWriter()
{
    char buf[1024];
    RsonWriter rw(buf, COUNTOF(buf), RSON_FORMAT_JSON);

    u8 binval[] = {'f','o','o','b','a','r'};
    int ints[] = {1,2,3,4};
    unsigned uns[] = {1,2,3,4};
	float floats[] = {1.5f,1.25f,1.125f,1.0625f};
	double doubles[] = {1234.0000000000000009,2.234577000001,1.30000030000457,1.200003400057};

	bool success = rw.Begin(NULL, NULL);
	Assert(success);
	success = rw.WriteDouble("double", 12340000000000000009.0E-016);
	Assert(success);
	success = rw.WriteArray("doubles", doubles, 4);
	Assert(success);
	success = rw.End();
#if __PPU
	Assert(0 == strcmp(rw.ToString(), "{\"double\":1234.00000,\"doubles\":[1234.00000,2.23458,1.30000,1.20000]}"));
#else
	Assert(0 == strcmp(rw.ToString(), "{\"double\":1234.00000,\"doubles\":[1234.00000,2.23458,1.30000,1.20000]}"));
#endif
	rw.Reset();

    success = rw.Begin(NULL, NULL);
    Assert(success);
    success = rw.WriteInt("int", 123);
    Assert(success);
    success = rw.WriteUns("uns", 0xFFFF1230);
    Assert(success);
    success = rw.WriteFloat("float", 1.334455f);
    Assert(success);
    success = rw.WriteHex("hex", 0xABCDEF);
    Assert(success);
    success = rw.WriteHex64("hex64", 0x123456789ABCDEFull);
    Assert(success);
	success = rw.WriteBinary("foobar", binval, COUNTOF(binval));
	Assert(success);
    success = rw.WriteBool("bool", true);
    Assert(success);
    success = rw.WriteString("string", "fuckoff");
    Assert(success);
    success = rw.WriteString("string2", "fuck off");
    Assert(success);
    success = rw.WriteString("string3", "\"fuck off\"");
    Assert(success);
    success = rw.WriteArray("ints", ints, 4);
    Assert(success);
    success = rw.WriteArray("uns", uns, 4);
    Assert(success);
	success = rw.WriteArray("floats", floats, 4);
	Assert(success);
    success = rw.End();
    Assert(success);

	Assert(0 == strcmp(rw.ToString(),
        "{\"int\":123,\"uns\":4294906416,\"float\":1.33446,\"hex\":\"0xABCDEF\",\"hex64\":\"0x123456789ABCDEF\",\"foobar\":\"Zm9vYmFy\",\"bool\":true,\"string\":\"fuckoff\",\"string2\":\"fuck off\",\"string3\":\"\\\"fuck off\\\"\",\"ints\":[1,2,3,4],\"uns\":[1,2,3,4],\"floats\":[1.50000,1.25000,1.12500,1.06250]}"));

    //Test calling End() when the buffer has been reset to continue
    //and nothing further has been added.  This occurs when the
    //underlying memory buffer was exhausted and the writer was reset
    //to continue, but there was no further data to write.
    rw.Reset();
    success = rw.Begin(NULL, NULL);
    Assert(success);
    success = rw.WriteInt("int", 123);
    Assert(success);
    rw.ResetToContinue();
    success = rw.End();
    Assert(success);

    Assert(0 == strcmp(rw.ToString(), "}"));
}

int
RsonWriter::TestMe()
{
    TestLuaWriter();
    TestJsonWriter();

    return 1;
}

//////////////////////////////////////////////////////////////////////////
//  RsonReader
//////////////////////////////////////////////////////////////////////////
RsonReader::RsonReader()
    : m_Buf(NULL)
    , m_LenofBuf(0)
    , m_NameOffset(-1)
    , m_NameLen(-1)
    , m_ValOffset(-1)
    , m_ValLen(-1)
{
}

RsonReader::RsonReader(const char* buf, const unsigned lenofBuf)
{
    this->Clear();
    Init(buf, lenofBuf);
}

bool
RsonReader::Init(const char* buf, const unsigned lenofBuf)
{
    return Init(buf, 0, lenofBuf);
}

bool
RsonReader::Init(const char* const buf, const int offset, const unsigned lenofBuf)
{
    this->Clear();

    if(AssertVerify(NULL != buf) && lenofBuf > 0)
    {
#if __ASSERT
		bool nullTerminated = false;
		for(int i = lenofBuf; i >= 0; --i)
		{
			if(buf[i] == '\0')
			{
				nullTerminated = true;
				break;
			}
		}
		Assert(nullTerminated);
#endif

		int nameOff = -1, nameLen = -1;
        int valOff = -1, valLen = -1;

        int i = SkipWs(buf, offset, lenofBuf);

        if(i >= 0)
        {
            if('{' == buf[i] || '[' == buf[i])
            {
                //This is a nameless object.

                valOff = i;

                i = EndOfObject(buf, valOff, lenofBuf);
                if(i > valOff)
                {
                    valLen = i - valOff;
                }
                else
                {
                    valOff = -1;
                }
            }
            /*else if(buf[i] == '\"')
            {
                //This is a nameless string value

                valOff = i;
                valLen = EndOfString(buf, valOff, lenofBuf) - valOff;
                nameOff = -1;
                nameLen = -1;
            }*/
            else if(!Rson::IsReserved(buf[i]))
            {
                nameOff = i;

                i = EndOfString(buf, nameOff, lenofBuf);

                if(i > nameOff)
                {
                    nameLen = i - nameOff;

                    i = SkipWs(buf, i, lenofBuf);

                    if(i >= 0 && ('=' == buf[i] || ':' == buf[i]))
                    {
                        if('\"' == buf[nameOff])
                        {
                            //Name is enclosed in quotes
                            ++nameOff;
                            nameLen -= 2;
                        }

                        if(Verifyf((unsigned) nameOff < lenofBuf && nameLen > 0,
                                    "Invalid item name at position:%d", nameOff-1))
                        {
                            ++i;
                            valOff = SkipWs(buf, i, lenofBuf);

                            if(valOff >= i)
                            {
                                if('{' == buf[valOff] || '[' == buf[valOff])
                                {
                                    i = EndOfObject(buf, valOff, lenofBuf);
                                }
                                else
                                {
                                    i = EndOfString(buf, valOff, lenofBuf);
                                }

                                if(i > valOff)
                                {
                                    valLen = i - valOff;

                                    /*if('\"' == buf[valOff])
                                    {
                                        if(!Verifyf(valLen-2 > 0,
                                                "Invalid item value at position:%d", valOff))
                                        {
                                            valOff = valLen = -1;
                                        }
                                    }*/
                                }
                                else
                                {
                                    valOff = valLen = -1;
                                }
                            }
                        }
                    }
                    else
                    {
                        //There is no name for this value.

                        valOff = nameOff;
                        valLen = nameLen;
                        nameOff = -1;
                        nameLen = -1;
                    }
                }
            }
        }

        if(valOff >= 0 && valLen > 0)
        {
            m_ValOffset = valOff;
            m_ValLen = valLen;
            m_Buf = buf;
            m_LenofBuf = lenofBuf;

            if(nameOff >= 0 && nameLen > 0)
            {
                m_NameOffset = nameOff;
                m_NameLen = nameLen;
            }
            else
            {
                m_NameOffset = m_NameLen = -1;
            }
        }
        else
        {
            this->Clear();
        }

        //Check that if we have a non-null name that the name has a length.
        Assert(m_NameOffset < 0 || m_NameLen > 0);
    }

    return (NULL != m_Buf);
}

void
RsonReader::Clear()
{
    m_Buf = NULL;
    m_NameOffset = m_ValOffset = -1;
    m_NameLen = m_ValLen = -1;
    m_LenofBuf = 0;
}

bool
RsonReader::HasData() const
{
    return (NULL != m_Buf);
}

bool
RsonReader::HasMember(const char* name) const
{
    RsonReader tmp;
    return GetMember(name, &tmp);
}

bool
RsonReader::GetFirstMember(RsonReader* rr) const
{
    bool success = false;
    if(m_Buf
        && ('{' == m_Buf[m_ValOffset] || '[' == m_Buf[m_ValOffset])
        && m_ValLen > 0)
    {
        success = rr->Init(&m_Buf[m_ValOffset+1], m_LenofBuf - unsigned(m_ValOffset+1));
    }

    return success;
}

bool
RsonReader::GetNextSibling(RsonReader* rr)
{
    bool success = false;
    if(m_Buf)
    {
        int i = SkipWs(m_Buf, m_ValOffset + m_ValLen, m_LenofBuf);
        if(i >= 0)
        {
            if(',' == m_Buf[i])
            {
                for(++i; m_Buf[i] && (',' == m_Buf[i] || Rson::IsSpace(m_Buf[i])); ++i)
                {
                }

                success = m_Buf[i] && rr->Init(&m_Buf[i], m_LenofBuf - (unsigned)i);
            }
            else if('}' != m_Buf[i] && ']' != m_Buf[i] && '\0' != m_Buf[i])
            {
                rsonWarning("Unexpected character %c at offset:%d in \"%s\"", m_Buf[i], i, m_Buf);
            }
        }
    }

    return success;
}

bool
RsonReader::GetMember(const char* name, RsonReader* rr) const
{
    bool success = false;
	size_t nameLen = strlen(name);
    RsonReader tmp;
    for(bool ok = this->GetFirstMember(&tmp); ok; ok = tmp.GetNextSibling(&tmp))
    {
        if((static_cast<int>(nameLen) == tmp.m_NameLen) && (0 == strncmp(name, tmp.Name(), nameLen)))
        {
            *rr = tmp;
            success = true;
            break;
        }
    }

    return success;
}

bool
RsonReader::HasDuplicateEntries(const char* name) const
{
	// running tally
	unsigned memberCount = 0;

	size_t nameLen = strlen(name);
	RsonReader tmp;
	for(bool ok = this->GetFirstMember(&tmp); ok; ok = tmp.GetNextSibling(&tmp))
	{
		if((static_cast<int>(nameLen) == tmp.m_NameLen) && (0 == strncmp(name, tmp.Name(), nameLen)))
		{
			memberCount++;
			if(memberCount > 1)
				break;
		}
	}

	return memberCount > 1;
}

bool RsonReader::HasName() const
{
	return (m_Buf && m_NameOffset >= 0 && m_NameLen >= 0);
}

bool
RsonReader::GetName(char* name, const int maxChars) const
{
    bool success = false;

    if(m_Buf
        && m_NameLen > 0
        && Verifyf(maxChars > m_NameLen, "Insufficient buffer length for name"))
    {
        const char* s = this->Name();
        const char* e = s + m_NameLen;
        char* d = name;
        for(; s < e; ++s, ++d)
        {
            if('\\' == *s)
            {
                //Escape
                ++s;
            }

            *d = *s;
        }

        *d = '\0';
        success = true;
    }

    return success;
}

bool
RsonReader::GetValue(char* val, const int maxChars) const
{
    bool success = false;

    const unsigned myValLen = this->GetValueLength();

    if(m_Buf
        && Verifyf((unsigned) maxChars > myValLen, "Insufficient buffer length for value"))
    {
        const char* s = this->Value();
        const char* e = s + myValLen;
        char* d = val;
        for(; s < e; ++s, ++d)
        {
            if('\\' == *s)
            {
                //Escape
                ++s;
            }

            *d = *s;
        }

        *d = '\0';
        success = true;
    }

    return success;
}

bool
RsonReader::GetValue(const char* name, char* val, const int maxChars) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.GetValue(val, maxChars);
}

int
RsonReader::GetValueLength() const
{
    return m_Buf ? ('\"' == m_Buf[m_ValOffset] ? m_ValLen-2 : m_ValLen) : 0;
}

int
RsonReader::GetValueLength(const char* name) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) ? rr.GetValueLength() : -1;
}

int
RsonReader::GetBinaryValueLength() const
{
    int len, offset;

    this->GetB64OffsetAndLength(&offset, &len);

    return (len * 6) / 8;
}

int
RsonReader::GetBinaryValueLength(const char* name) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) ? rr.GetBinaryValueLength() : -1;
}

bool
RsonReader::AsInt(int& val) const
{
    const char* p = this->Value();
    return p && RSON_SCANF(p, this->GetValueLength(), "%i", &val) > 0;
}

bool
RsonReader::AsInt8(s8& val) const
{
	const char* p = this->Value();
	return p && RSON_SCANF(p, this->GetValueLength(), "%hhi", &val) > 0;
}

bool
RsonReader::AsInt16(s16& val) const
{
	const char* p = this->Value();
	return p && RSON_SCANF(p, this->GetValueLength(), "%hi", &val) > 0;
}

bool
RsonReader::AsInt64(s64& val) const
{
    const char* p = this->Value();
    return p && RSON_SCANF(p, this->GetValueLength(), "%" I64FMT "d", &val) > 0;
}

bool
RsonReader::AsUns(unsigned& val) const
{
    bool success = false;
    const char* p = this->Value();
    if(p)
    {
		const int valLen = this->GetValueLength();
        if(valLen > 2 && '0' == p[0] && ('x' == p[1] || 'X' == p[1]))
        {
            if('x' == p[1])
            {
                success = (RSON_SCANF(p, valLen, "0x%X", &val) > 0);
            }
            else
            {
                success = (RSON_SCANF(p, valLen, "0X%X", &val) > 0);
            }
        }
        else
        {
            success = (RSON_SCANF(p, valLen, "%u", &val) > 0);
        }
    }

    return success;
}

bool
RsonReader::AsUns8(u8& val) const
{
	bool success = false;
	const char* p = this->Value();
	if(p)
	{
		const int valLen = this->GetValueLength();
		if(valLen > 2 && '0' == p[0] && ('x' == p[1] || 'X' == p[1]))
		{
			if('x' == p[1])
			{
				success = (RSON_SCANF(p, valLen, "0x%hhX", &val) > 0);
			}
			else
			{
				success = (RSON_SCANF(p, valLen, "0X%hhX", &val) > 0);
			}
		}
		else
		{
			success = (RSON_SCANF(p, valLen, "%hhu", &val) > 0);
		}
	}

	return success;
}

bool
RsonReader::AsUns16(u16& val) const
{
	bool success = false;
	const char* p = this->Value();
	if(p)
	{
		const int valLen = this->GetValueLength();
		if(valLen > 2 && '0' == p[0] && ('x' == p[1] || 'X' == p[1]))
		{
			if('x' == p[1])
			{
				success = (RSON_SCANF(p, valLen, "0x%hX", &val) > 0);
			}
			else
			{
				success = (RSON_SCANF(p, valLen, "0X%hX", &val) > 0);
			}
		}
		else
		{
			success = (RSON_SCANF(p, valLen, "%hu", &val) > 0);
		}
	}

	return success;
}

bool
RsonReader::AsUns64(u64& val) const
{
    bool success = false;
    const char* p = this->Value();
    if(p)
    {
		const int valLen = this->GetValueLength();
        if(valLen > 2 && '0' == p[0] && 'x' == p[1])
        {
            if('x' == p[1])
            {
                success = (RSON_SCANF(p, valLen, "0x%" I64FMT "X", &val) > 0);
            }
            else
            {
                success = (RSON_SCANF(p, valLen, "0X%" I64FMT "X", &val) > 0);
            }
        }
        else
        {
            success = (RSON_SCANF(p, valLen, "%" I64FMT "u", &val) > 0);
        }
    }

    return success;
}

bool
RsonReader::AsFloat(float& val) const
{
	const char* p = this->Value();
	return p && RSON_SCANF(p, this->GetValueLength(), FLOAT_READ_FORMAT, &val) > 0;
}

bool
RsonReader::AsDouble(double& val) const
{
	const char* p = this->Value();
	return p && RSON_SCANF(p, this->GetValueLength(), DOUBLE_READ_FORMAT, &val) > 0;
}

bool
RsonReader::AsBinary(void* val, const int maxLen, int* len) const
{
    bool success = false;

    *len = 0;

    if(m_Buf)
    {
        int srcLen, offset;
        this->GetB64OffsetAndLength(&offset, &srcLen);

        *len = (srcLen * 6) / 8;

        if(*len > 0 && Verifyf(maxLen >= *len, "Insufficient buffer length for binary data"))
        {
            u8* dst = (u8*) val;
            int src = offset;
            unsigned tmp = 0;
            unsigned accum = 0;
            unsigned d = 0;

            for(int i = 0; i < srcLen; ++i)
            {
                if('=' == m_Buf[src+i])
                {
                    //Ignore trailing pad characters.
                    break;
                }

                if(accum >= 8)
                {
                    dst[d++] = u8((tmp >> (accum - 8)) & 0xFF);
                    accum -= 8;
                }

                tmp = (tmp << 6) | datBase64::CharToValue(m_Buf[src+i]);

                accum += 6;
            }

            if(accum > 0)
            {
                Assert(8 == accum || 10 == accum || 12 == accum);                
                dst[d++] = u8((tmp >> (accum - 8)) & 0xFF);
            }

            Assert(*len == (int) d);

            success = true;
        }
    }

    return success;
}

bool
RsonReader::AsBool(bool& val) const
{
    bool success = false;
    const char* p = this->Value();
    if(p)
    {
        const unsigned valLen = this->GetValueLength();
        if(4 == valLen)
        {
            success = ('t' == p[0] || 'T' == p[0])
                    && ('r' == p[1] || 'R' == p[1])
                    && ('u' == p[2] || 'U' == p[2])
                    && ('e' == p[3] || 'E' == p[3]);

            if(success)
            {
                val = true;
            }
        }
        else if(5 == valLen)
        {
            success = ('f' == p[0] || 'F' == p[0])
                    && ('a' == p[1] || 'A' == p[1])
                    && ('l' == p[2] || 'L' == p[2])
                    && ('s' == p[3] || 'S' == p[3])
                    && ('e' == p[4] || 'E' == p[4]);

            if(success)
            {
                val = false;
            }
        }
    }

    return success;
}

bool 
RsonReader::AsAtString(atString& string) const
{
	// well this is pretty easy...
	string.Set(this->Value(), this->GetValueLength(), 0);
	return true;
}

bool
RsonReader::AsString(char* val, const int bufLen) const
{
    bool success = false;

    if(m_Buf)
    {
        const unsigned valLen = this->GetValueLength();

        if(Verifyf((unsigned) bufLen > valLen, "Insufficient buffer length for string (%d <= %d)", bufLen , valLen))
        {
            const char* src = this->Value();
            char* dst = val;
            bool escaped = false;
            for(int i = 0; i < (int)valLen; ++i, ++src)
            {
                if(!escaped && '\\' == *src)
                {
                    escaped = true;
                    continue;
                }

                escaped = false;
                *dst++ = *src;
            }

            *dst = '\0';
            success = true;
        }
    }

    return success;
}

bool
RsonReader::AsRawString(char* val, const int bufLen) const
{
    bool success = false;

    if(m_Buf)
    {
        const unsigned valLen = this->GetValueLength();

        if(Verifyf((unsigned) bufLen > valLen, "Insufficient buffer length for string"))
        {
            memcpy(val, this->Value(), valLen);
            val[valLen] = '\0';
            success = true;
        }
    }

    return success;
}

bool
RsonReader::AsReader(RsonReader& rr) const
{
    bool success = false;
    if(m_Buf)
    {
        rr.Init(&m_Buf[m_ValOffset], m_LenofBuf - (unsigned)m_ValOffset);

        success = true;
    }

    return success;
}

template<>
bool
RsonReader::AsValue(int& value) const
{
    return this->AsInt(value);
}

template<>
bool
RsonReader::AsValue(s16& value) const
{
	return this->AsInt16(value);
}

template<>
bool
RsonReader::AsValue(s64& value) const
{
    return this->AsInt64(value);
}

template<>
bool
RsonReader::AsValue(unsigned& value) const
{
    return this->AsUns(value);
}

template<>
bool
RsonReader::AsValue(float& value) const
{
    return this->AsFloat(value);
}

template<>
bool
RsonReader::AsValue(u8& value) const
{
	return this->AsUns8(value);
}

template<>
bool
RsonReader::AsValue(u16& value) const
{
	return this->AsUns16(value);
}

template<>
bool
RsonReader::AsValue(u64& value) const
{
    return this->AsUns64(value);
}

template<>
bool
RsonReader::AsValue(bool& value) const
{
	return this->AsBool(value);
}

template<>
bool
RsonReader::AsValue(double& value) const
{
	return this->AsDouble(value);
}

template<>
bool
RsonReader::AsValue(RsonReader& value) const
{
	return this->AsReader(value);
}

static const char* NULL_VALUE = "null";

bool
RsonReader::IsNull() const
{
    const char* p = this->Value();
    return p && !strnicmp(p, NULL_VALUE, strlen(NULL_VALUE));
}

bool
RsonReader::ReadInt(const char* name, int& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsInt(val);
}

bool
RsonReader::ReadInt16(const char* name, s16& val) const
{
	RsonReader rr;
	return this->GetMember(name, &rr) && rr.AsInt16(val);
}

bool
RsonReader::ReadInt64(const char* name, s64& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsInt64(val);
}

bool
RsonReader::ReadUns(const char* name, unsigned& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsUns(val);
}

bool
RsonReader::ReadUns16(const char* name, u16& val) const
{
	RsonReader rr;
	return this->GetMember(name, &rr) && rr.AsUns16(val);
}

bool
RsonReader::ReadUns64(const char* name, u64& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsUns64(val);
}

bool
RsonReader::ReadFloat(const char* name, float& val) const
{
	RsonReader rr;
	return this->GetMember(name, &rr) && rr.AsFloat(val);
}

bool
RsonReader::ReadDouble(const char* name, double& val) const
{
	RsonReader rr;
	return this->GetMember(name, &rr) && rr.AsDouble(val);
}

bool
RsonReader::ReadBinary(const char* name, void* val, const int maxLen, int* len) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsBinary(val, maxLen, len);
}

bool
RsonReader::ReadBool(const char* name, bool& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsBool(val);
}

bool
RsonReader::ReadString(const char* name, char* val, const int bufLen) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsString(val, bufLen);
}

bool
RsonReader::ReadRawString(const char* name, char* val, const int bufLen) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsRawString(val, bufLen);
}

bool
RsonReader::ReadReader(const char* name, RsonReader& val) const
{
    RsonReader rr;
    return this->GetMember(name, &rr) && rr.AsReader(val);
}

bool 
RsonReader::ReadString(const char* name, atString& val) const
{
	RsonReader rr;
	return this->GetMember(name, &rr) && rr.AsAtString(val);
}

//private:

void
RsonReader::GetB64OffsetAndLength(int* offset, int* len) const
{
    if(AssertVerify(m_Buf))
    {
        *len = this->GetValueLength();
        *offset = int(this->Value() - m_Buf);

        //Trim pad characters.
        while(*len > 0 && '=' == m_Buf[*offset+*len-1])
        {
            --*len;
        }
    }
    else
    {
        *offset = *len = -1;
    }
}

int
RsonReader::SkipWs(const char* buf, const int offset, const unsigned lenofBuf)
{
    Assert(offset >= 0);
    Assert((unsigned) offset < lenofBuf);

    int result = offset;
    if(result >= 0 && (unsigned) result < lenofBuf && '\0' != buf[result])
    {
        for(; (unsigned) result < lenofBuf && buf[result] && Rson::IsSpace(buf[result]); ++result)
        {
        }

        if('\0' == buf[result] || result >= (int) lenofBuf)
        {
            result = -1;
        }
    }
    else
    {
        result = -1;
    }

    return result;
}

int
RsonReader::EndOfToken(const char* buf, const int offset, const unsigned lenofBuf)
{
    Assert(offset >= 0);
    Assert((unsigned) offset < lenofBuf);

    int result = offset;
    if(result >= 0 && (unsigned) result < lenofBuf && '\0' != buf[result])
    {
        for(++result; (unsigned) result < lenofBuf && buf[result]; ++result)
        {
            if(Rson::IsReserved(buf[result]) || Rson::IsSpace(buf[result]))
            {
                break;
            }
        }
    }
    else
    {
        result = -1;
    }

    return result;
}

int
RsonReader::EndOfString(const char* buf, const int offset, const unsigned lenofBuf)
{
    Assert(offset >= 0);
    Assert((unsigned) offset < lenofBuf);

    int result = -1;

    if(offset >= 0 && (unsigned) offset < lenofBuf && '\0' != buf[offset])
    {
        int i = offset;

        if('\"' == buf[i])
        {
            bool escaped = false;

            for(++i; (unsigned) i < lenofBuf && buf[i]; ++i)
            {
                if(escaped)
                {
                    escaped = false;
                    continue;
                }

                if('\\' == buf[i])
                {
                    escaped = true;
                }
                else if('\"' == buf[i])
                {
                    result = ++i;
                    break;
                }
            }
        }
        else
        {
            result = EndOfToken(buf, offset, lenofBuf);
        }
    }

    return result;
}

int
RsonReader::EndOfObject(const char* buf, const int offset, const unsigned lenofBuf)
{
    Assert(offset >= 0);
    Assert((unsigned) offset < lenofBuf);

    int result = -1;

    if(offset >= 0 && (unsigned) offset < lenofBuf && '\0' != buf[offset])
    {
        int i = offset;

        const char openBrace = buf[i];
        const char closeBrace = ('{' == openBrace) ? '}' : ']';

        if(Verifyf('{' == openBrace || '[' == openBrace, ""))
        {
            int braceCount = 1;

            for(++i; i >= offset && (unsigned) i < lenofBuf && buf[i]; ++i)
            {
                if('\"' == buf[i])
                {
                    i = EndOfString(buf, i, lenofBuf) - 1;
                }
                else if(openBrace == buf[i])
                {
                    ++braceCount;
                }
                else if(closeBrace == buf[i])
                {
                    --braceCount;
                    if(0 == braceCount)
                    {
                        result = ++i;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

bool
RsonReader::CheckName(const char* name, const bool ignoreNull) const
{
    if (!m_Buf || m_NameOffset < 0 || m_NameLen <= 0) return false;
    if (strncmp(name, &m_Buf[m_NameOffset], m_NameLen)) return false;
    if (ignoreNull && IsNull()) return false;

    return true;
}

void 
RsonReader::GetRawValueString(char* buf, int bufSize) const
{
	if (Verifyf(m_Buf && m_ValOffset >= 0 && m_ValLen > 0, ""))
	{
		int charsToCopy = m_ValLen;
		if (charsToCopy < bufSize)
		{
			strncpy(buf, &m_Buf[m_ValOffset], charsToCopy);
			buf[charsToCopy] = '\0';
		}
		else
		{
			safecpy(buf, &m_Buf[m_ValOffset], bufSize);
		}
	}
	else
	{
		if (Verifyf(bufSize > 0, ""))
		{
			buf[0] = '\0';
		}
	}
}

const char*
RsonReader::Value() const
{
    return (Verifyf(m_Buf && m_ValOffset >= 0 && m_ValLen > 0, ""))
            ? ('\"' == m_Buf[m_ValOffset] ? &m_Buf[m_ValOffset+1] : &m_Buf[m_ValOffset])
            : "";
}

#define C 1 // Control
#define S 2 // Space
#define E 4 // Escape
#define H 8 // Hex
#define D 16 // Digit

// Table in comments from http://web.cs.mun.ca/~michael/c/ascii-table.html
const u8 g_JsonCharacterTraits[128] = {
	//  Char  Dec  Oct  Hex 
	//  --------------------
	C,	//  (nul)   0 0000 0x00 
	C,	//  (soh)   1 0001 0x01 
	C,	//  (stx)   2 0002 0x02 
	C,	//  (etx)   3 0003 0x03 
	C,	//  (eot)   4 0004 0x04 
	C,	//  (enq)   5 0005 0x05 
	C,	//  (ack)   6 0006 0x06 
	C,	//  (bel)   7 0007 0x07 
	C,	//  (bs)    8 0010 0x08 
	C+S,	//  (ht)    9 0011 0x09 
	C+S,	//  (nl)   10 0012 0x0a 
	C+S,	//  (vt)   11 0013 0x0b 
	C,	//  (np)   12 0014 0x0c 
	C+S,	//  (cr)   13 0015 0x0d 
	C,	//  (so)   14 0016 0x0e 
	C,	//  (si)   15 0017 0x0f 
	C,	//  (dle)  16 0020 0x10 
	C,	//  (dc1)  17 0021 0x11 
	C,	//  (dc2)  18 0022 0x12 
	C,	//  (dc3)  19 0023 0x13 
	C,	//  (dc4)  20 0024 0x14 
	C,	//  (nak)  21 0025 0x15 
	C,	//  (syn)  22 0026 0x16 
	C,	//  (etb)  23 0027 0x17 
	C,	//  (can)  24 0030 0x18 
	C,	//  (em)   25 0031 0x19 
	C,	//  (sub)  26 0032 0x1a 
	C,	//  (esc)  27 0033 0x1b 
	C,	//  (fs)   28 0034 0x1c 
	C,	//  (gs)   29 0035 0x1d 
	C,	//  (rs)   30 0036 0x1e 
	C,	//  (us)   31 0037 0x1f 
	S,	//  (sp)   32 0040 0x20
	0,	//  !      33 0041 0x21
	E,	//  "      34 0042 0x22
	0,	//  #      35 0043 0x23
	0,	//  $      36 0044 0x24
	0,	//  %      37 0045 0x25
	0,	//  &      38 0046 0x26
	0,	//  '      39 0047 0x27
	0,	//  (      40 0050 0x28
	0,	//  )      41 0051 0x29
	0,	//  *      42 0052 0x2a
	0,	//  +      43 0053 0x2b
	0,	//  ,      44 0054 0x2c
	0,  //  -      45 0055 0x2d
	0,	//  .      46 0056 0x2e
	E,	//  /      47 0057 0x2f
	H+D,	//  0      48 0060 0x30
	H+D,	//  1      49 0061 0x31
	H+D,	//  2      50 0062 0x32
	H+D,	//  3      51 0063 0x33
	H+D,	//  4      52 0064 0x34
	H+D,	//  5      53 0065 0x35
	H+D,	//  6      54 0066 0x36
	H+D,	//  7      55 0067 0x37
	H+D,	//  8      56 0070 0x38
	H+D,	//  9      57 0071 0x39
	0,  //  :      58 0072 0x3a
	0,	//  ;      59 0073 0x3b
	0,	//  <      60 0074 0x3c
	0,	//  =      61 0075 0x3d
	0,	//  >      62 0076 0x3e
	0,	//  ?      63 0077 0x3f
	0,	//  @      64 0100 0x40
	H,  //  A      65 0101 0x41
	H,  //  B      66 0102 0x42
	H,  //  C      67 0103 0x43
	H,  //  D      68 0104 0x44
	H,  //  E      69 0105 0x45
	H,  //  F      70 0106 0x46
	0,  //  G      71 0107 0x47
	0,  //  H      72 0110 0x48
	0,  //  I      73 0111 0x49
	0,  //  J      74 0112 0x4a
	0,  //  K      75 0113 0x4b
	0,  //  L      76 0114 0x4c
	0,  //  M      77 0115 0x4d
	0,  //  N      78 0116 0x4e
	0,  //  O      79 0117 0x4f
	0,  //  P      80 0120 0x50
	0,  //  Q      81 0121 0x51
	0,  //  R      82 0122 0x52
	0,  //  S      83 0123 0x53
	0,  //  T      84 0124 0x54
	0,  //  U      85 0125 0x55
	0,  //  V      86 0126 0x56
	0,  //  W      87 0127 0x57
	0,  //  X      88 0130 0x58
	0,  //  Y      89 0131 0x59
	0,  //  Z      90 0132 0x5a
	0,	//  [      91 0133 0x5b
	E,	//  \      92 0134 0x5c
	0,	//  ]      93 0135 0x5d
	0,	//  ^      94 0136 0x5e
	0,  //  _      95 0137 0x5f
	0,	//  `      96 0140 0x60
	H,  //  a      97 0141 0x61
	H+E,  //  b      98 0142 0x62
	H,  //  c      99 0143 0x63
	H,  //  d     100 0144 0x64
	H,  //  e     101 0145 0x65
	H+E,  //  f     102 0146 0x66
	0,  //  g     103 0147 0x67
	0,  //  h     104 0150 0x68
	0,  //  i     105 0151 0x69
	0,  //  j     106 0152 0x6a
	0,  //  k     107 0153 0x6b
	0,  //  l     108 0154 0x6c
	0,  //  m     109 0155 0x6d
	E,  //  n     110 0156 0x6e
	0,  //  o     111 0157 0x6f
	0,  //  p     112 0160 0x70
	0,  //  q     113 0161 0x71
	E,  //  r     114 0162 0x72
	0,  //  s     115 0163 0x73
	E,  //  t     116 0164 0x74
	0,  //  u     117 0165 0x75
	0,  //  v     118 0166 0x76
	0,  //  w     119 0167 0x77
	0,  //  x     120 0170 0x78
	0,  //  y     121 0171 0x79
	0,  //  z     122 0172 0x7a
	0,	//  {     123 0173 0x7b
	0,	//  |     124 0174 0x7c
	0,	//  }     125 0175 0x7d
	0,	//  ~     126 0176 0x7e
	C,	//  (del) 127 0177 0x7f

};

#undef C
#undef S
#undef H
#undef E
#undef D

class JsonValidator
{
public:
	JsonValidator(const char* buf, int bufSize)
	{
		m_LineNum = 1;
		m_CharNum = 1;
		m_Current = buf;
		m_End = buf + bufSize;
	}
	
	bool IsControl(char16 c)		{ return c <= 128 ? (g_JsonCharacterTraits[c] & 1) != 0 : false; }
	bool IsSpace(char16 c)			{ return c <= 128 ? (g_JsonCharacterTraits[c] & 2) != 0 : false; }
	bool IsEscape(char16 c)			{ return c <= 128 ? (g_JsonCharacterTraits[c] & 4) != 0 : false; }
	bool IsHexadecimal(char16 c)	{ return c <= 128 ? (g_JsonCharacterTraits[c] & 8) != 0 : false; }
	bool IsDigit(char16 c)			{ return c <= 128 ? (g_JsonCharacterTraits[c] & 16) != 0: false; }

	char16 Next()
	{
		char16 next;
		int bytes = Utf8ToWideChar(m_Current, next);
		m_Current += bytes;
		if (next == '\n')
		{
			m_LineNum++;
			m_CharNum = 1;
		}
		else
		{
			m_CharNum++;
		}
		return m_Current <= m_End ? next : 0;
	}

	char16 Peek()
	{
		char16 next;
		Utf8ToWideChar(m_Current, next);
		return next;
	}

	bool Expect(char c)
	{
		char16 n = Next();
		if (n != c)
		{
			Warningf("JSON Validation Error: line %d char %d, expected '%c' and got '%c'", m_LineNum, m_CharNum, c, n);
			return false;
		}
		return true;
	}

	void SkipWs()
	{
		while(1)
		{
			if (m_Current >= m_End)
			{
				break;
			}

			if (!IsSpace(Peek()))
			{
				break;
			}
			Next();
		}
	}

	bool ValidateObject(bool bSkipBraces = false)
	{
		SkipWs();
		char16 c;

		if (!bSkipBraces)
		{
			c = Next();
			if (c != '{')
			{
				Warningf("JSON Validation Error: line %d char %d, reading object, expected '%c' and got '%c'", m_LineNum, m_CharNum, '{', c);
				return false;
			}
			SkipWs();
		}

		// Empty list
		if (Peek() == '}')
		{
			Next();
			return true;
		}

		// First element
		if (!ValidateString()) return false;
		SkipWs();
		c = Next();
		if (c != ':')
		{
			Warningf("JSON Validation Error: line %d char %d, reading object, expected '%c' and got '%c'", m_LineNum, m_CharNum, ':', c);
			return false;
		}
		SkipWs();
		if (!ValidateValue()) return false;
		SkipWs();

		// (",", value) pairs
		while(Peek() == ',')
		{
			Next();
			SkipWs();
			if (!ValidateString()) return false;
			SkipWs();
			c = Next();
			if (c != ':')
			{
				Warningf("JSON Validation Error: line %d char %d, reading object, expected '%c' and got '%c'", m_LineNum, m_CharNum, ':', c);
				return false;
			}
			SkipWs();
			if (!ValidateValue()) return false;
			SkipWs();
		}

		if (!bSkipBraces)
		{
			c = Next();
			if (c != '}')
			{
				Warningf("JSON Validation Error: line %d char %d, reading object, expected '%c' and got '%c'", m_LineNum, m_CharNum, '}', c);
				return false;
			}
		}

		return true;
	}


	bool ValidateArray()
	{
		SkipWs();
		char16 c = Next();
		if (c != '[')
		{
			Warningf("JSON Validation Error: line %d char %d, reading array, expected '%c' and got '%c'", m_LineNum, m_CharNum, '[', c);
			return false;
		}
		SkipWs();

		// Empty list
		if (Peek() == ']')
		{
			Next();
			return true;
		}

		// First element
		if (!ValidateValue()) return false;
		SkipWs();

		// (",", value) pairs
		while(Peek() == ',')
		{
			Next();
			SkipWs();
			if (!ValidateValue()) return false;
			SkipWs();
		}

		c = Next();
		if (c != ']')
		{
			Warningf("JSON Validation Error: line %d char %d, reading array, expected '%c' and got '%c'", m_LineNum, m_CharNum, ']', c);
			return false;
		}
		return true;
	}

	bool ValidateLiteral(const char* literal)
	{
		const char* s = literal;
		while(*s)
		{
			char16 c = Next();
			if (c != *s)
			{
				Warningf("JSON Validation Error: line %d char %d, reading literal \"%s\" and got '%c'", m_LineNum, m_CharNum, literal, c);
				return false;
			}
			s++;
		}
		return true;
	}

	bool ValidateValue()
	{
		char16 c = Peek();
		if (c == '"')
		{
			return ValidateString();
		}
		else if (IsDigit(c) || c == '-')
		{
			return ValidateNumber();
		}
		else if (c == '{')
		{
			return ValidateObject();
		}
		else if (c == '[')
		{
			return ValidateArray();
		}
		else if (c == 't')
		{
			return ValidateLiteral("true");
		}
		else if (c == 'f')
		{
			return ValidateLiteral("false");
		}
		else if (c == 'n')
		{
			return ValidateLiteral("null");
		}
		else
		{
			Warningf("JSON Validation Error: line %d char %d, reading a value, found unexpected char '%c'", m_LineNum, m_CharNum, c);
			return false;
		}
	}


	bool ValidateUnicode()
	{
		for(int i = 0; i < 4; i++)
		{
			char16 n = Next();
			if (!IsHexadecimal(n))
			{
				Warningf("JSON Validation Error: line %d char %d, found an invalid hex digit '%c' while parsing unicode escape sequence", m_LineNum, m_CharNum, n); 
				return false;
			}
		}
		return true;
	}

	bool ValidateString()
	{
		if (!Expect('"')) return false;

		while(1)
		{
			char16 n = Next();
			switch(n)
			{
			case '"':
				return true;
			case '\\':
				{
					char16 m = Next();
					if (IsEscape(m))
					{
						// This is ok
					}
					else if (m == 'u')
					{
						if (!ValidateUnicode()) return false;
					}
					else
					{
						Warningf("JSON Validation Error: line %d char %d, found an invalid escape sequence '\\%c' while parsing string", m_LineNum, m_CharNum, m); 
						return false;
					}
				}
				break;
			default:
				if (IsControl(n))
				{
					Warningf("JSON Validation Error: line %d char %d, found a control character 0x%x while parsing string", m_LineNum, m_CharNum, n); 
					return false;
				}
			}
		}
	}

	bool ValidateNumber()
	{
		char16 c = Peek();
		if (c == '-')
		{
			Next();
			c = Peek();
		}
	
		// Read the integral part
		if (c == '0')
		{
			// This is ok
			Next();
		}
		else if (IsDigit(c)) // 1-9
		{
			while(IsDigit(Peek()))
			{
				Next();
			}
		}
		else
		{
			Warningf("JSON Validation Error: line %d char %d, expected a digit and got '%c'", m_LineNum, m_CharNum, c);
			return false;
		}

		c = Peek();

		// Optional fractional part
		if (c == '.')
		{
			Next();
			c = Peek();
			if (!IsDigit(c))
			{
				Warningf("JSON Validation Error: line %d char %d, expected a digit and got '%c'", m_LineNum, m_CharNum, c);
				return false;
			}

			while(IsDigit(Peek()))
			{
				Next();
			}

		}

		c = Peek();

		// Optional exponent
		if (c == 'e' || c == 'E')
		{
			Next();
			c = Peek();
			if (c == '+' || c == '-')
			{
				Next();
				c = Peek();
			}
			if (!IsDigit(c))
			{
				Warningf("JSON Validation Error: line %d char %d, expected a digit and got '%c'", m_LineNum, m_CharNum, c);
				return false;
			}
			while(IsDigit(Peek()))
			{
				Next();
			}
		}
		return true;
	}

	bool ValidateDocument()
	{
		SkipWs();
		if (!ValidateObject()) 
		{
			return false;
		}
		SkipWs();
		if (m_Current != m_End)
		{
			Warningf("JSON Validation Error: line %d char %d, extra data following end of first object", m_LineNum, m_CharNum);
			Warningf("JSON Validation Error: extra content = '%s'", m_Current);
			return false;
		}
		return true;
	}

	int m_LineNum;
	int m_CharNum;
	const char* m_Current;
	const char* m_End;
};

bool
RsonReader::ValidateJson(const char* buf, int bufSize )
{
	JsonValidator v(buf, bufSize);
	return v.ValidateDocument();
}

bool
RsonReader::ValidateJsonObject(const char* buf, int bufSize, bool bSkipBraces)
{
	JsonValidator v(buf, bufSize);
	return v.ValidateObject(bSkipBraces);
}

//private:

const char*
RsonReader::Name() const
{
    return (m_Buf && Verifyf(m_NameOffset >= 0, "")) ? &m_Buf[m_NameOffset] : "";
}

const char* 
RsonReader::GetRawMemberValue( const char* name, int& outStrLen ) const
{
	RsonReader tmp;

	//Empty string means just return the actual object
	if (name == NULL || name[0] == '\0')
	{
		outStrLen = GetValueLength();
		return Value();
	}
	
	if (GetMember(name, &tmp))
	{
		outStrLen = tmp.GetValueLength();
		return tmp.Value();
	}

	outStrLen = 0;
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//  BrsonWriter
//////////////////////////////////////////////////////////////////////////
BrsonWriter::BrsonWriter()
: m_OrigBuf(NULL)
, m_Buf(NULL)
, m_Cursor(0)
, m_BufLength(0)
{
}

BrsonWriter::BrsonWriter(u8* buf, const unsigned len)
{
    Init(buf, len);
}

void
BrsonWriter::Init(u8* buf, const unsigned len)
{
    Clear();

    m_OrigBuf = m_Buf = buf;
    m_BufLength = len;
}

void
BrsonWriter::Clear()
{
    m_OrigBuf = m_Buf = NULL;
    m_Cursor = m_BufLength = 0;
}

bool
BrsonWriter::Begin(const char* name, unsigned* cursor)
{
    if(AssertVerify(m_Cursor < 0xFFFF) && m_BufLength > m_Cursor + 3)
    {
        if(cursor)
        {
            *cursor = (int)m_Cursor;
        }

        if(WriteString(name))
        {
            m_Buf = &m_Buf[m_Cursor];
            m_Buf[0] = BRSONTYPE_OBJECT;
            m_Buf[1] = u8((m_Cursor>>8)&0xFF);
            m_Buf[2] = u8(m_Cursor&0xFF);
            m_Cursor = 3;
            return true;
        }
    }

    return false;
}

bool
BrsonWriter::End()
{
    if(AssertVerify(m_Buf >= m_OrigBuf))
    {
        const unsigned cursor = m_Cursor;
        m_Cursor = (m_Buf[0]<<8) | m_Buf[1];
        m_Buf -= m_Cursor;
        Assert(m_Buf >= m_OrigBuf);
        ++m_Cursor;
        WriteVarInt(cursor);
        if(2 == m_Cursor)
        {
            memmove(&m_Buf[m_Cursor], &m_Buf[m_Cursor+1], cursor);
        }

        m_Cursor += cursor;

        return true;
    }

    return false;
}

bool
BrsonWriter::WriteString(const char* name, const char* value)
{
    return WriteString(name)
            && WriteString(value);
}

bool
BrsonWriter::WriteInt(const char* name, const s64 value)
{
    if(WriteString(name))
    {
        m_Buf[m_Cursor++] = BRSONTYPE_INT;
        return WriteVarInt((value << 1) ^ (value >> 63));
    }

    return false;
}

//private:

bool
BrsonWriter::WriteVarInt(const u64 val)
{
    u64 u = val;
    do
    {
        const u8 b = u8(u&0x7F);
        m_Buf[m_Cursor++] = u8(b | ((u > 0x7F) << 7));
        u >>= 7;
    }
    while(u);
    return true;
}

bool
BrsonWriter::WriteString(const char* str)
{
    const size_t len = strlen(str);
    m_Buf[m_Cursor++] = BRSONTYPE_STRING;
    WriteVarInt(len);
    sysMemCpy(&m_Buf[m_Cursor], str, len);
    m_Cursor += (unsigned)len;

    return true;
}

//////////////////////////////////////////////////////////////////////////
//  BrsonReader
//////////////////////////////////////////////////////////////////////////
BrsonReader::BrsonReader()
: m_ParentBuf(NULL)
, m_ParentLen(0)
, m_Buf(NULL)
, m_Length(0)
{
}

bool
BrsonReader::Init(const u8* buf, const size_t len)
{
    if(!buf || !len)
    {
        return false;
    }

    Clear();

    u64 varInt;
    size_t offset;
    const u8* eob = buf+len;

    switch(buf[0])
    {
    case BRSONTYPE_OBJECT:
    case BRSONTYPE_ARRAY:
        offset = ReadVarInt(&buf[1], len-1, &varInt);
        if(offset && AssertVerify(buf+offset+1+varInt <= eob))
        {
            m_Buf = buf;
            m_Length = unsigned(offset+1+varInt);
            return true;
        }
        break;
    case BRSONTYPE_STRING:
        //Length of key
        offset = ReadVarInt(&buf[1], len-1, &varInt);
        if(offset && AssertVerify(buf+offset+1+varInt <= eob))
        {
            const u8* p = &buf[offset+1+varInt];
            switch(p[0])
            {
            case BRSONTYPE_OBJECT:
            case BRSONTYPE_ARRAY:
            case BRSONTYPE_STRING:
                offset = ReadVarInt(p+1, eob-(p+1), &varInt);
                if(offset && AssertVerify(p+1+offset+varInt <= eob))
                {
                    m_Buf = buf;
                    m_Length = unsigned(p+1+offset+varInt-buf);
                    return true;
                }
            case BRSONTYPE_INT:
                offset = ReadVarInt(p+1, eob-(p+1), &varInt);
                if(offset && AssertVerify(p+1+offset+varInt <= eob))
                {
                    m_Buf = buf;
                    m_Length = p+1+offset-buf;
                    return true;
                }
            }
        }
        break;
    }

    return false;
}

void
BrsonReader::Clear()
{
    m_ParentBuf = 0;
    m_ParentLen = 0;
    m_Buf = NULL;
    m_Length = 0;
}

bool
BrsonReader::GetFirstMember(BrsonReader* rr) const
{
    if(BRSONTYPE_OBJECT == m_Buf[0]
        || BRSONTYPE_ARRAY == m_Buf[0])
    {
        size_t offset;
        u64 varInt;
        offset = ReadVarInt(&m_Buf[1], m_Length-1, &varInt);
        if(offset && AssertVerify(offset+1+varInt <= m_Length))
        {
            const u8* parentBuf = m_ParentBuf;
            const unsigned parentLen = m_ParentLen;
            if(rr->Init(m_Buf+offset+1, m_Length-(offset+1)))
            {
                rr->m_ParentBuf = parentBuf;
                rr->m_ParentLen = parentLen;
                return true;
            }
        }
    }

    return false;
}

bool
BrsonReader::GetNextSibling(BrsonReader* rr) const
{
    if(m_ParentBuf)
    {
        const u8* p = m_Buf + m_Length;
        if(p < m_ParentBuf+m_ParentLen)
        {
            const u8* parentBuf = m_ParentBuf;
            const unsigned parentLen = m_ParentLen;
            if(rr->Init(p, (m_ParentBuf+m_ParentLen)-p))
            {
                rr->m_ParentBuf = parentBuf;
                rr->m_ParentLen = parentLen;
                return true;
            }
        }
    }

    return false;
}

bool
BrsonReader::GetName(char* name, const unsigned maxChars) const
{
    if(!m_Buf || !m_Length)
    {
        return false;
    }

    if(BRSONTYPE_STRING == m_Buf[0])
    {
        u64 varInt;
        const size_t offset = ReadVarInt(&m_Buf[1], m_Length-1, &varInt);
        if(offset && offset+varInt+1 <= m_Length)
        {
            unsigned len = (unsigned)varInt;
            if(len >= maxChars)
            {
                len = maxChars-1;
            }
            sysMemCpy(name, &m_Buf[offset+1], len);
            name[len] = '0';
            return true;
        }
    }

    return false;
}
    
//private:

size_t
BrsonReader::ReadVarInt(const u8* buf, const size_t len, u64* varInt)
{
    *varInt = 0;
    int shift = 0;
    const u8* p = buf;
    do 
    {
        *varInt |= (*p&0x7F)<<shift;
        shift += 7;
        ++p;
    }
    while(p[-1]&0x80 && unsigned(p-buf) < len);

    return !(p[-1]&0x80) ? p - buf : 0;
}

#if __ASSERT
static void TestLuaReader()
{
    const char* rsonTests[] =
    {
        //Value is an escaped string
        "{\"astring\":\"\\\"thestring\\\"\"}",
        //Lots of things
        "{vers=123,foo=bar,user={onname=\"Your Anus\",id=0x43dc7a128add9087},time=1234567,microtime=34359738368}",
        //Extra double quotes
        "{foo = \"bubba\"\",bar=hubba}",
        //Misplaced }
        "{foo = \"bubba\"},bar=hubba}",
        //No {}
        "foo = \"bubba\",bar=hubba",
        //No {}
        "{b0=Zg,d0=f,b1=Zm8,d1=fo,b2=Zm9v,d2=foo,b3=Zm9vYg,d3=fooob,b4=Zm9vYmE,d4=foooba,b5=Zm9vYmFy,d5=fooobar}",
        //Arrays of int, unsigned, float
		"{ints={1,2,3,4},uns={-1,-2,-3,-4},floats={1.5,1.25,1.125,1.0625}}",
		"{doubles={9.0E-016,2.234567890123456,1.3345678901234559,0.0000000000009}}",
        "{Success=true}",
        "{Failure=false}",
        //Array of objects, with spaces in the names
        "{{\"a a\"=1},{\"b b\"=2},{\"c c\"=3}}",
    };

    RsonReader rr, tmp;
    char str[128];
    u64 id, microtime;
    unsigned time, u;

    bool success = rr.Init(rsonTests[0], 0, (unsigned) strlen(rsonTests[0]));
    Assert(success);
    success = rr.ReadString("astring", str, sizeof(str));
    Assert(success);
    Assert(0 == strcmp(str, "\"thestring\""));

    success = rr.Init(rsonTests[1], 0, (unsigned) strlen(rsonTests[1]));
    Assert(success);
    success = rr.GetMember("user", &tmp);
    Assert(success);
    success = tmp.ReadString("onname", str, COUNTOF(str));
    Assert(success && !strcmp("Your Anus", str));
    success = tmp.ReadUns64("id", id);
    Assert(success && 0x43dc7a128add9087ull == id);
    success = rr.ReadUns("time", time);
    Assert(success && 1234567 == time);
    success = rr.ReadUns64("microtime", microtime);
    Assert(success && 34359738368ull == microtime);
    success = rr.ReadUns("foo", u);
    Assert(!success);
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(success && !strcmp("bar", str));
    success = rr.ReadUns("bar", u);
    Assert(!success);

    success = rr.Init(rsonTests[2], 0, (unsigned) strlen(rsonTests[2]));
    Assert(!success);

    success = rr.Init(rsonTests[3], 0, (unsigned) strlen(rsonTests[3]));
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(success && !strcmp("bubba", str));
    success = rr.ReadString("bar", str, COUNTOF(str));
    Assert(!success);

    success = rr.Init(rsonTests[4], 0, (unsigned) strlen(rsonTests[4]));
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(!success);
    success = rr.ReadString("bar", str, COUNTOF(str));
    Assert(!success);
    success = rr.AsString(str, COUNTOF(str));
    Assert(success && !strcmp("bubba", str));

    u8 bv[128];
    int len;
    success = rr.Init(rsonTests[5], 0, (unsigned) strlen(rsonTests[5]));
    success = rr.ReadBinary("b0", bv, COUNTOF(bv), &len);
    Assert(success && 1 == len && 'f' == bv[0]);
    success = rr.ReadBinary("b1", bv, COUNTOF(bv), &len);
    Assert(success && 2 == len && 'f' == bv[0] && 'o' == bv[1]);
    success = rr.ReadBinary("b2", bv, COUNTOF(bv), &len);
    Assert(success && 3 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2]);
    success = rr.ReadBinary("b3", bv, COUNTOF(bv), &len);
    Assert(success && 4 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3]);
    success = rr.ReadBinary("b4", bv, COUNTOF(bv), &len);
    Assert(success && 5 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3] && 'a' == bv[4]);
    success = rr.ReadBinary("b5", bv, COUNTOF(bv), &len);
    Assert(success && 6 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3] && 'a' == bv[4] && 'r' == bv[5]);

    int ints[4];
    unsigned uns[4];
	float floats[4];
	double doubles[4];

    rr.Init(rsonTests[6], 0, (unsigned) strlen(rsonTests[6]));
    int numRead = rr.ReadArray("ints", ints, 4);
    Assert(4 == numRead);
    Assert(1==ints[0] && 2==ints[1] && 3==ints[2] && 4==ints[3]);
    numRead = rr.ReadArray("uns", uns, 4);
    Assert(4 == numRead);
    Assert(unsigned(-1)==uns[0] && unsigned(-2)==uns[1] && unsigned(-3)==uns[2] && unsigned(-4)==uns[3]);
    numRead = rr.ReadArray("floats", floats, 4);
    Assert(4 == numRead);
    Assert(1.5f==floats[0] && 1.25f==floats[1] && 1.125f==floats[2] && 1.0625f==floats[3]);

	rr.Init(rsonTests[7], 0, (unsigned) strlen(rsonTests[7]));
	numRead = rr.ReadArray("doubles", doubles, 4);
	Assert(4 == numRead);
#if __PPU && CELL_SDK_VERSION <= 0x360001
    //Apparently there's a bug in parsing doubles
	Assert(9e-016==doubles[0] && 2.2345678906477442==doubles[1] && 1.3345678905428864==doubles[2] && 9e-13==doubles[3]);
#else
	Assert(9e-016==doubles[0] && 2.2345678901234560==doubles[1] && 1.3345678901234559==doubles[2] && 9e-13==doubles[3]);
#endif

	bool b;
    rr.Init(rsonTests[8], 0, (unsigned) strlen(rsonTests[8]));
    success = rr.ReadBool("Success", b);
    Assert(success);
    Assert(b);

    rr.Init(rsonTests[9], 0, (unsigned) strlen(rsonTests[9]));
    success = rr.ReadBool("Failure", b);
    Assert(success);
    Assert(!b);

    RsonReader readers[10];
    const char* names[] = {"a a", "b b", "c c"};
    rr.Init(rsonTests[10], 0, (unsigned) strlen(rsonTests[10]));
    numRead = rr.AsArray(readers, COUNTOF(readers));
    Assert(COUNTOF(names) == numRead);
    for(int i = 0; i < COUNTOF(names); ++i)
    {
        success = readers[i].GetFirstMember(&rr);
        Assert(success);
        success = rr.GetName(str, sizeof(str));
        Assert(success);
        Assert(!strcmp(str, names[i]));
        int val;
        success = rr.AsInt(val);
        Assert(success);
        Assert(i+1 == val);
    }
}

static void TestJsonReader()
{
    const char* rsonTests[] =
    {
        //Value is an escaped string
        "{\"astring\":\"\\\"thestring\\\"\"}",
        //Lots of things
        "{\"vers\":123,\"foo\":\"bar\",\"user\":{\"onname\":\"Your Anus\",\"id\":\"0x43dc7a128add9087\"},\"time\":1234567,\"microtime\":34359738368}",
        //Extra double quotes
        "{\"foo\" : \"bubba\"\",\"bar\":\"hubba\"}",
        //Misplaced }
        "{\"foo\" : \"bubba\"},\"bar\":\"hubba\"}",
        //No {}
        "\"foo\" : \"bubba\",\"bar\":\"hubba\"",
        //No {}
        "{\"b0\":\"Zg\",\"d0\":\"f\",\"b1\":\"Zm8\",\"d1\":\"fo\",\"b2\":\"Zm9v\",\"d2\":\"foo\",\"b3\":\"Zm9vYg\",\"d3\":\"fooob\",\"b4\":\"Zm9vYmE\",\"d4\":\"foooba\",\"b5\":\"Zm9vYmFy\",\"d5\":\"fooobar\"}",
        //Arrays of int, unsigned, float
		"{\"ints\":[1,2,3,4],\"uns\":[-1,-2,-3,-4],\"floats\":[1.5,1.25,1.125,1.0625]}",
		"{\"doubles\":[9.0E-016,2.234567890123456,1.3345678901234559,0.0000000000009]}",
        "{\"Success\":true}",
        "{\"Failure\":false}",
        //Array of objects.  JSON can have spaces in the name.
        "[{\"a a\":1},{\"b b\":2},{\"c c\":3}]",
    };

    RsonReader rr, tmp;
    char str[128];
    u64 id, microtime;
    unsigned time, u;

    bool success = rr.Init(rsonTests[0], 0, (unsigned) strlen(rsonTests[0]));
    Assert(success);
    success = rr.ReadString("astring", str, sizeof(str));
    Assert(success);
    Assert(0 == strcmp(str, "\"thestring\""));

    success = rr.Init(rsonTests[1], 0, (unsigned) strlen(rsonTests[1]));
    Assert(success);
    success = rr.GetMember("user", &tmp);
    Assert(success);
    success = tmp.ReadString("onname", str, COUNTOF(str));
    Assert(success && !strcmp("Your Anus", str));
    success = tmp.ReadUns64("id", id);
    Assert(success && 0x43dc7a128add9087ull == id);
    success = rr.ReadUns("time", time);
    Assert(success && 1234567 == time);
    success = rr.ReadUns64("microtime", microtime);
    Assert(success && 34359738368ull == microtime);
    success = rr.ReadUns("foo", u);
    Assert(!success);
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(success && !strcmp("bar", str));
    success = rr.ReadUns("bar", u);
    Assert(!success);

    success = rr.Init(rsonTests[2], 0, (unsigned) strlen(rsonTests[2]));
    Assert(!success);

    success = rr.Init(rsonTests[3], 0, (unsigned) strlen(rsonTests[3]));
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(success && !strcmp("bubba", str));
    success = rr.ReadString("bar", str, COUNTOF(str));
    Assert(!success);

    success = rr.Init(rsonTests[4], 0, (unsigned) strlen(rsonTests[4]));
    success = rr.ReadString("foo", str, COUNTOF(str));
    Assert(!success);
    success = rr.ReadString("bar", str, COUNTOF(str));
    Assert(!success);
    success = rr.AsString(str, COUNTOF(str));
    Assert(success && !strcmp("bubba", str));

    u8 bv[128];
    int len;
    success = rr.Init(rsonTests[5], 0, (unsigned) strlen(rsonTests[5]));
    success = rr.ReadBinary("b0", bv, COUNTOF(bv), &len);
    Assert(success && 1 == len && 'f' == bv[0]);
    success = rr.ReadBinary("b1", bv, COUNTOF(bv), &len);
    Assert(success && 2 == len && 'f' == bv[0] && 'o' == bv[1]);
    success = rr.ReadBinary("b2", bv, COUNTOF(bv), &len);
    Assert(success && 3 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2]);
    success = rr.ReadBinary("b3", bv, COUNTOF(bv), &len);
    Assert(success && 4 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3]);
    success = rr.ReadBinary("b4", bv, COUNTOF(bv), &len);
    Assert(success && 5 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3] && 'a' == bv[4]);
    success = rr.ReadBinary("b5", bv, COUNTOF(bv), &len);
    Assert(success && 6 == len && 'f' == bv[0] && 'o' == bv[1] && 'o' == bv[2] && 'b' == bv[3] && 'a' == bv[4] && 'r' == bv[5]);

    int ints[4];
    unsigned uns[4];
	float floats[4];
	double doubles[4];

    rr.Init(rsonTests[6], 0, (unsigned) strlen(rsonTests[6]));
    int numRead = rr.ReadArray("ints", ints, 4);
    Assert(4 == numRead);
    Assert(1==ints[0] && 2==ints[1] && 3==ints[2] && 4==ints[3]);
    numRead = rr.ReadArray("uns", uns, 4);
    Assert(4 == numRead);
    Assert(unsigned(-1)==uns[0] && unsigned(-2)==uns[1] && unsigned(-3)==uns[2] && unsigned(-4)==uns[3]);
    numRead = rr.ReadArray("floats", floats, 4);
    Assert(4 == numRead);
    Assert(1.5f==floats[0] && 1.25f==floats[1] && 1.125f==floats[2] && 1.0625f==floats[3]);

	rr.Init(rsonTests[7], 0, (unsigned) strlen(rsonTests[7]));
	numRead = rr.ReadArray("doubles", doubles, 4);
	Assert(4 == numRead);
#if __PPU && CELL_SDK_VERSION <= 0x360001
	//Apparently there's a bug in parsing doubles
	Assert(9e-016==doubles[0] && 2.2345678906477442==doubles[1] && 1.3345678905428864==doubles[2] && 9e-13==doubles[3]);
#else
Assert(9e-016==doubles[0] && 2.2345678901234560==doubles[1] && 1.3345678901234559==doubles[2] && 9e-13==doubles[3]);
#endif

	bool b;
    rr.Init(rsonTests[8], 0, (unsigned) strlen(rsonTests[8]));
    success = rr.ReadBool("Success", b);
    Assert(success);
    Assert(b);

    rr.Init(rsonTests[9], 0, (unsigned) strlen(rsonTests[9]));
    success = rr.ReadBool("Failure", b);
    Assert(success);
    Assert(!b);

    RsonReader readers[10];
    const char* names[] = {"a a", "b b", "c c"};
    rr.Init(rsonTests[10], 0, (unsigned) strlen(rsonTests[10]));
    numRead = rr.AsArray(readers, COUNTOF(readers));
    Assert(COUNTOF(names) == numRead);
    for(int i = 0; i < COUNTOF(names); ++i)
    {
        success = readers[i].GetFirstMember(&rr);
        Assert(success);
        success = rr.GetName(str, sizeof(str));
        Assert(success);
        Assert(!strcmp(str, names[i]));
        int val;
        success = rr.AsInt(val);
        Assert(success);
        Assert(i+1 == val);
    }
}

#if 0
static void TestBrson()
{
    /*const char* brsonTests[] =
    {
        "o\022s\003foos\003bars\003bazi\010\011",
    };*/

    u8 buf[512];
    unsigned cursor;
    BrsonWriter bw(buf);
    bw.Begin("foo", &cursor);
    bw.WriteString("a", "b");
    bw.WriteString("c", "d");
    bw.End();

    BrsonReader br, br1, br2;
    char name[32];
    br.Init(buf);
    br.GetFirstMember(&br1);
    br1.GetName(name);
    br1.GetNextSibling(&br2);
    br2.GetName(name);
}
#endif

int
RsonReader::TestMe()
{
    TestLuaReader();
    TestJsonReader();
    //TestBrson();

    return 1;
}

#endif  //__ASSERT

}   //namespace rage
