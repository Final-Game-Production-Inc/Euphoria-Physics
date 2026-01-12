// 
// data/sax.cpp 
// 
// Copyright (C) Forever Rockstar Games.  All Rights Reserved. 
// 

#include "sax.h"
#include "system/memops.h"

#include <ctype.h>

namespace rage
{

datSaxReader::datSaxReader()
: m_State(STATE_INVALID)
{
}

datSaxReader::~datSaxReader()
{
}

void
datSaxReader::Begin()
{
    if(AssertVerify(!Pending()))
    {
        m_State = STATE_START_DOCUMENT;
    }
}

void
datSaxReader::End()
{
    Assertf((STATE_EXPECT_START_LEFT == m_State || STATE_START_DOCUMENT == m_State ), "Unexpected state in datSaxReader::End : %d", m_State);
    if(!(STATE_EXPECT_START_LEFT == m_State || STATE_START_DOCUMENT == m_State ))
    {
        m_State = STATE_ERROR;
    }
    else
    {
        m_State = STATE_FINISHED;
    }

    this->endDocument();
}

bool
datSaxReader::Pending() const
{
    return STATE_INVALID != m_State && !Succeeded() && !Failed();
}

bool
datSaxReader::Succeeded() const
{
    return STATE_FINISHED == m_State;
}

bool
datSaxReader::Failed() const
{
    return STATE_ERROR == m_State;
}

static const char START_CDATA[]     = {"[CDATA["};
static const int LENOF_START_CDATA  = sizeof(START_CDATA)-1;
static const char END_CDATA[]       = {"]]>"};
static const int LENOF_END_CDATA    = sizeof(END_CDATA)-1;

unsigned
datSaxReader::Parse(const char* buf, const int offset, const unsigned lenofBuf)
{
    AssertMsg(m_State > STATE_INVALID, "Forgot to call Begin()");

    int i;

    m_CharDataOffset = offset;

    for(i = offset; i < (int)lenofBuf; ++i)
    {
        const int c = (int) buf[i];

        switch(m_State)
        {
        case STATE_INVALID:
            Assert(false);
            break;
        case STATE_START_DOCUMENT:
            this->startDocument();
            m_State = STATE_EXPECT_START_LEFT;
            //break;    NO BREAK!!
        case STATE_EXPECT_START_LEFT:
            if('<' == c)
            {
                m_State = STATE_EXPECT_START_NAME;
            }
            break;
        case STATE_EXPECT_START_NAME:
            if((c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                    || ':' == c || '_' == c)
            {
                m_TagNameLen = 0;
                m_TagQName[m_TagNameLen++] = (char)c;
                m_State = STATE_SCAN_START_NAME;
            }
            else if('/' == c)
            {
                m_State = STATE_EXPECT_END_NAME;
            }
            else if('!' == c)
            {
                m_State = STATE_EXPECT_COMMENT_OR_CDATA;
            }
            else if('?' == c)
            {
                //Skip to the end of the processing directive
                m_State = STATE_EXPECT_END_SECTION;
            }
            else
            {
                Assertf(false, "Error while expecting start name");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_SCAN_START_NAME:
            if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || ':' == c || '_' == c || '-' == c || '.' == c)
            {
                if(AssertVerify(m_TagNameLen < sizeof(m_TagQName)-1))
                {
                    m_TagQName[m_TagNameLen++] = (char)c;
                }
                else
                {
                    Assertf(false, "Error while scanning start name");
                    m_State = STATE_ERROR;
                }
            }
            else
            {
                Assert(m_TagNameLen < sizeof(m_TagQName));

                m_TagQName[m_TagNameLen] = '\0';
                m_State = STATE_EXPECT_ATTR_NAME;
                --i;
                this->startElement("", "", m_TagQName);
            }
            break;
        case STATE_EXPECT_ATTR_NAME:
            if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || ':' == c || '_' == c)
            {
                m_AttrNameLen = 0;
                m_AttrName[m_AttrNameLen++] = (char)c;
                m_State = STATE_SCAN_ATTR_NAME;
            }
            else if('/' == c)
            {
                m_State = STATE_EXPECT_END_RIGHT;
            }
            else if('>' == c)
            {
                m_State = STATE_EXPECT_CHAR_DATA;
                m_CharDataOffset = i+1;
            }
            else if(!isspace(c))
            {
                Assertf(false, "Error while expecting attribute name");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_SCAN_ATTR_NAME:
            if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || ':' == c || '_' == c || '-' == c || '.' == c)
            {
                if(AssertVerify(m_AttrNameLen < sizeof(m_AttrName)-1))
                {
                    m_AttrName[m_AttrNameLen++] = (char)c;
                }
                else
                {
                    Assertf(false, "Error while scanning attribute name");
                    m_State = STATE_ERROR;
                }
            }
            else
            {
                Assert(m_AttrNameLen < sizeof(m_AttrName));

                m_AttrName[m_AttrNameLen] = '\0';
                m_State = STATE_EXPECT_EQ;
                --i;
            }
            break;
        case STATE_EXPECT_EQ:
            if('=' == c)
            {
                m_State = STATE_EXPECT_START_QUOTE;
            }
            else if(!isspace(c))
            {
                Assertf(false, "Unexpected character, expecting '=', found %c", c);
                m_State = STATE_ERROR;
            }
            break;
        case STATE_EXPECT_START_QUOTE:
            if('\"' == c || '\'' == c)
            {
                m_QuoteChar = c;
                m_AttrValLen = 0;
                m_State = STATE_SCAN_ATTR_VALUE;
            }
            else if(!isspace(c))
            {
                Assertf(false, "Unexpected character, expecting start quote, found %c", c);
                m_State = STATE_ERROR;
            }
            break;
        case STATE_SCAN_ATTR_VALUE:
            if(c == m_QuoteChar)
            {
                Assert(m_AttrValLen < sizeof(m_AttrVal));
                Assert(m_TagNameLen);

                m_AttrVal[m_AttrValLen] = '\0';
                char decodeBuf[sizeof(m_AttrVal)];
                unsigned dbLen = sizeof(decodeBuf);
                unsigned numConsumed;
                if(AssertVerify(XmlDecode(decodeBuf,
                                            &dbLen,
                                            m_AttrVal,
                                            m_AttrValLen,
                                            &numConsumed)))
                {
                    this->attribute(m_TagQName, m_AttrName, decodeBuf);
                    m_State = STATE_EXPECT_SEMICOLON;
                }
                else
                {
                    m_State = STATE_ERROR;
                }
            }
            else if(AssertVerify(m_AttrValLen < sizeof(m_AttrVal)-1))
            {
                m_AttrVal[m_AttrValLen++] = (char)c;
            }
            else
            {
                Assertf(false, "Error scanning attribute value");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_EXPECT_SEMICOLON:
            if(';' == c)
            {
                m_State = STATE_EXPECT_ATTR_NAME;
            }
            else if(!isspace(c))
            {
                --i;
                m_State = STATE_EXPECT_ATTR_NAME;
            }
            break;
        case STATE_EXPECT_CHAR_DATA:
            if('<' == c || '&' == c)
            {
                if(i > m_CharDataOffset)
                {
                    unsigned totalConsumed = 0;
                    const unsigned srcLen = i-m_CharDataOffset;
                    while(totalConsumed < srcLen)
                    {
                        unsigned numConsumed;
                        char decodeBuf[256];
                        unsigned bufLen = sizeof(decodeBuf);
                        if(AssertVerify(XmlDecode(decodeBuf,
                                                    &bufLen,
                                                    &buf[m_CharDataOffset+totalConsumed],
                                                    srcLen-totalConsumed,
                                                    &numConsumed)))
                        {
                            this->characters(decodeBuf, 0, bufLen);
                            totalConsumed += numConsumed;
                        }
                        else
                        {
                            m_State = STATE_ERROR;
                            break;
                        }
                    }

                    if(STATE_ERROR != m_State)
                    {
                        m_CharDataOffset = i;
                    }
                }

                if('<' == c)
                {
                    m_State = STATE_EXPECT_START_NAME;
                }
                else
                {
                    //Handle '&'
                    m_TokenLen = 0;
                    m_Token[m_TokenLen++] = (char)c;
                    m_Token[m_TokenLen] = '\0';
                    m_State = STATE_EXPECT_ESCAPED_CHAR_DATA;
                }
            }
            break;
        case STATE_EXPECT_ESCAPED_CHAR_DATA:
            m_Token[m_TokenLen++] = (char)c;
            m_Token[m_TokenLen] = '\0';
            if(';' == c)
            {
                unsigned numConsumed;
                const int escChar = XmlDecodeEscape(m_Token, m_TokenLen, &numConsumed);
                if(escChar < 0)
                {
                    Assertf(false, "Error while expecting escape char");
                    m_State = STATE_ERROR;
                }
                else
                {
                    const char ec = (char)escChar;
                    this->characters(&ec, 0, 1);
                    m_CharDataOffset = i+1;
                    m_State = STATE_EXPECT_CHAR_DATA;
                }
            }
            else if(m_TokenLen == sizeof(m_Token) - 1)
            {
                Assertf(false, "Error while expecting escape char, token length is wrong");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_EXPECT_COMMENT_OR_CDATA:
            if('-' == c)
            {
                //Skip to the end of the comment
                m_State = STATE_EXPECT_END_SECTION;
            }
            else if('[' == c)
            {
                m_TokenLen = 0;
                m_Token[m_TokenLen++] = (char)c;
                m_Token[m_TokenLen] = '\0';
                m_State = STATE_EXPECT_START_CDATA;
            }
            else
            {
                Assertf(false, "Error while expecting comment or data");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_EXPECT_START_CDATA:
            m_Token[m_TokenLen++] = (char)c;
            Assert(m_TokenLen <= LENOF_START_CDATA);
            if(m_TokenLen <= LENOF_START_CDATA && c != START_CDATA[m_TokenLen-1])
            {
                Assert(false);
                m_State = STATE_ERROR;
            }
            else if(LENOF_START_CDATA == m_TokenLen)
            {
                m_TokenLen = 0;
                m_CharDataOffset = i+1;
                m_State = STATE_EXPECT_CDATA;
            }
            break;
        case STATE_EXPECT_CDATA:
            if(']' == c)
            {
                //If we don't have enough characters left
                //to check for the complete end of cdata token, 
                //then we can't consume the remaining characters 
                //until we do. Otherwise we don't know whether
                //to process the remaining characters as
                //CDATA or not. So the caller must buffer
                //until we do, and we must stay in our current
                //state. 
                if ((int)lenofBuf - i - 1 < LENOF_END_CDATA)
                {
                    //Go ahead and output anything up to the potential term sequence
                    //start
                    this->characters(&buf[m_CharDataOffset], 0, i - m_CharDataOffset);

                    //Consume everything up to before the ]
                    return i-offset;
                }
                else
                {
                    m_Token[m_TokenLen++] = (char)c;
                    m_State = STATE_EXPECT_END_CDATA;
                }
            }
            break;
        case STATE_EXPECT_END_CDATA:
            m_Token[m_TokenLen++] = (char)c;
            Assert(m_TokenLen <= LENOF_END_CDATA);
            if(m_TokenLen <= LENOF_END_CDATA && c != END_CDATA[m_TokenLen-1])
            {
                //No end of cdata token - go back to reading cdata.
                //Start reading from the character after we started this token
                i -= (m_TokenLen - 1);
                m_TokenLen = 0;
                m_State = STATE_EXPECT_CDATA;
            }
            else if(LENOF_END_CDATA == m_TokenLen)
            {
                const int lenofChars = i + 1 - m_CharDataOffset - LENOF_END_CDATA;
                if(lenofChars > 0)
                {
                    this->characters(&buf[m_CharDataOffset], 0, lenofChars);
                }
                m_State = STATE_EXPECT_START_LEFT;
            }
            break;
        case STATE_EXPECT_END_NAME:
            if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (':' == c || '_' == c))
            {
                m_TagNameLen = 0;
                m_TagQName[m_TagNameLen++] = (char)c;
                m_State = STATE_SCAN_END_NAME;
            }
            else
            {
                Assertf(false, "Error while expecting end name");
                m_State = STATE_ERROR;
            }
            break;
        case STATE_SCAN_END_NAME:
            if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || ':' == c || '_' == c || '-' == c || '.' == c)
            {
                if(!AssertVerify(m_TagNameLen < sizeof(m_TagQName)-1))
                {
                    Assertf(false, "Error while scanning end name");
                    m_State = STATE_ERROR;
                }
                else
                {
                    m_TagQName[m_TagNameLen++] = (char)c;
                }
            }
            else
            {
                Assert(m_TagNameLen < sizeof(m_TagQName));

                m_TagQName[m_TagNameLen] = '\0';
                m_State = STATE_EXPECT_END_RIGHT;
                --i;
            }
            break;
        case STATE_EXPECT_END_RIGHT:
            if('>' == c)
            {
                this->endElement("", "", m_TagQName);
                m_State = STATE_EXPECT_START_LEFT;
            }
            else if(!isspace(c))
            {
                Assertf(false, "Error while expecting end right");
                m_State = STATE_ERROR;
            }
        case STATE_EXPECT_END_SECTION:
            if('>' == c)
            {
                m_State = STATE_EXPECT_START_LEFT;
            }
            break;

        case STATE_FINISHED:
            //Parse() shouldn't be called any more.
            Assertf(false, "Parsing is finished");
            break;
        case STATE_ERROR:
            break;
        }
    }

    if(STATE_EXPECT_CHAR_DATA == m_State)
    {
        if(i > m_CharDataOffset)
        {
            unsigned totalConsumed = 0;
            const unsigned srcLen = i-m_CharDataOffset;
            while(totalConsumed < srcLen)
            {
                unsigned numConsumed;
                char decodeBuf[256];
                unsigned bufLen = sizeof(decodeBuf);
                if(AssertVerify(XmlDecode(decodeBuf,
                                            &bufLen,
                                            &buf[m_CharDataOffset+totalConsumed],
                                            srcLen-totalConsumed,
                                            &numConsumed)))
                {
                    this->characters(decodeBuf, 0, bufLen);
                    totalConsumed += numConsumed;
                }
                else
                {
                    m_State = STATE_ERROR;
                    break;
                }
            }
        }
    }
    else if(STATE_EXPECT_CDATA == m_State)
    {
        this->characters(&buf[m_CharDataOffset], 0, i - m_CharDataOffset);
    }

    return i-offset;
}

void
datSaxReader::startDocument()
{
}

void
datSaxReader::endDocument()
{
}

void
datSaxReader::startElement(const char* /*uri*/, const char* /*localName*/, const char* /*qName*/)
{
}

void
datSaxReader::endElement(const char* /*uri*/, const char* /*localName*/, const char* /*qName*/)
{
}

void
datSaxReader::characters(const char* /*ch*/, const int /*start*/, const int /*length*/)
{
}

void
datSaxReader::attribute(const char* /*tagQName*/, const char* /*attrName*/, const char* /*attrValue*/)
{
}

bool
datSaxReader::NeedsXmlEncoding(const char* str, const unsigned len)
{
    bool needsIt = false;
    for(int i = 0; i < (int) len; ++i)
    {
        const int c = (unsigned char) str[i];
        const bool legal =
            '&' == c
            || 0x27 == c   //apostrophe
            || '>' == c
            || '<' == c
            || 0x22 == c;  //quote

        if(!legal)
        {
            needsIt = true;
            break;
        }
    }

    return needsIt;
}

bool
datSaxReader::XmlEncode(char* dst, unsigned* dstLen, const int c)
{
    Assert(c >= 0);

    bool success = false;

    if(AssertVerify(*dstLen >= 7))
    {
        const char* src;
        char cc = (char) c;

        switch(c)
        {
        case '&':
            src = "&amp;";
            *dstLen = sizeof("&amp;");
            break;
        case 0x27:      //apostrophe
            src = "&apos;";
            *dstLen = sizeof("&apos;");
            break;
        case '>':
            src = "&gt;";
            *dstLen = sizeof("&gt;");
            break;
        case '<':
            src = "&lt;";
            *dstLen = sizeof("&lt;");
            break;
        case 0x22:      //quote
            src = "&quot;";
            *dstLen = sizeof("&quot;");
            break;
        default:
            src = &cc;
            *dstLen = 1;
            break;
        }

        for(int i = 0; i < (int) *dstLen; ++i)
        {
            dst[i] = src[i];
        }

        dst[*dstLen] = '\0';

        success = true;
    }
    else
    {
        *dstLen = 0;
        dst[0] = '\0';
    }

    return success;
}

bool
datSaxReader::XmlEncode(char* dst,
                        unsigned* dstLen,
                        const char* src,
                        const unsigned srcLen,
                        unsigned* numConsumed)
{
    char* p = dst;
    const char* eod = dst + *dstLen;
    *dstLen = 0;
    *numConsumed = 0;

    for(int i = 0; i < (int) srcLen; ++i)
    {
        static const int BOUNCE_BUF_LEN = 8;
        char encoded[BOUNCE_BUF_LEN];
        unsigned encodedLen = BOUNCE_BUF_LEN;

        XmlEncode(encoded, &encodedLen, (unsigned char) src[i]);

        if(unsigned(eod - p) <= encodedLen)
        {
            break;
        }

        for(int i = 0; i < (int) encodedLen; ++i, ++p, ++*dstLen)
        {
            *p = encoded[i];
        }
        *p = '\0';
        ++*numConsumed;
    }

    return true;
}

bool
datSaxReader::XmlDecode(char* dst,
                        unsigned* dstLen,
                        const char* src,
                        const unsigned srcLen,
                        unsigned* numConsumed)
{
    bool success = true;
    char* p = dst;
    const char* eod = dst + *dstLen - 1;
    *dstLen = 0;
    *numConsumed = 0;

    for(int i = 0; i < (int) srcLen && p < eod; ++i, ++p, ++*numConsumed)
    {
        const char* s = &src[i];

        if('&' == *s)
        {
            if(i < (int)srcLen - 4
                && 'a' == s[1]
                && 'm' == s[2]
                && 'p' == s[3]
                && ';' == s[4])
            {
                *p = (char) 0x26;
                i += 4;
            }
            else if(i < (int)srcLen - 5
                    && 'a' == s[1]
                    && 'p' == s[2]
                    && 'o' == s[3]
                    && 's' == s[4]
                    && ';' == s[5])
            {
                *p = 0x27;
                i += 5;
            }
            else if(i < (int)srcLen - 3
                    && 'g' == s[1]
                    && 't' == s[2]
                    && ';' == s[3])
            {
                *p = 0x3E;
                i += 3;
            }
            else if(i < (int)srcLen - 3
                    && 'l' == s[1]
                    && 't' == s[2]
                    && ';' == s[3])
            {
                *p = 0x3C;
                i += 3;
            }
            else if(i < (int)srcLen - 5
                    && 'q' == s[1]
                    && 'u' == s[2]
                    && 'o' == s[3]
                    && 't' == s[4]
                    && ';' == s[5])
            {
                *p = 0x22;
                i += 5;
            }
            else
            {
                success = false;
            }
        }
        else
        {
            *p = *s;
        }
    }


    if(success)
    {
        *dstLen = ptrdiff_t_to_int(p - dst);
        dst[*dstLen] = '\0';
    }
    else
    {
        *dstLen = 0;
        dst[0] = '\0';
    }

    return success;
}

int
datSaxReader::XmlDecodeEscape(const char* src,
                                const unsigned srcLen,
                                unsigned* numConsumed)
{
    *numConsumed = 0;

    if('&' != src[0])
    {
        return -1;
    }

    if(srcLen >= 5
        && 'a' == src[1]
        && 'm' == src[2]
        && 'p' == src[3]
        && ';' == src[4])
    {
        *numConsumed = 5;
        return '&';
    }
    else if(srcLen >= 6
            && 'a' == src[1]
            && 'p' == src[2]
            && 'o' == src[3]
            && 's' == src[4]
            && ';' == src[5])
    {
        *numConsumed = 6;
        return 0x27;
    }
    else if(srcLen >= 4
            && 'g' == src[1]
            && 't' == src[2]
            && ';' == src[3])
    {
        *numConsumed = 4;
        return '>';
    }
    else if(srcLen >= 4
            && 'l' == src[1]
            && 't' == src[2]
            && ';' == src[3])
    {
        *numConsumed = 4;
        return '<';
    }
    else if(srcLen >= 6
            && 'q' == src[1]
            && 'u' == src[2]
            && 'o' == src[3]
            && 't' == src[4]
            && ';' == src[5])
    {
        *numConsumed = 6;
        return 0x22;
    }

    return -1;
}

#if __ASSERT && 0
static const int TEST_XML_ENCODING = datSaxReader::TestXmlEncoding();

int
datSaxReader::TestXmlEncoding()
{
    char bufA[512], bufB[sizeof(bufA)+1];
    char dstBuf[3*sizeof(bufA)];
    mthRandom rng(1);
    unsigned lenA, lenB, numConsumed;

    for(int i = 0; i < 1000; ++i)
    {
        for(int j = 0; j < sizeof(bufA); ++j)
        {
            bufA[j] = (char)(rng.GetInt() & 0xFF);
        }

        lenA = sizeof(dstBuf);
        Assert(datSaxReader::XmlEncode(dstBuf, &lenA, bufA, sizeof(bufA), &numConsumed));
        Assert(numConsumed == sizeof(bufA));
        lenB = sizeof(bufB);
        Assert(datSaxReader::XmlDecode(bufB, &lenB, dstBuf, lenA, &numConsumed));
        Assert(numConsumed == lenA);
        Assert(!memcmp(bufA, bufB, sizeof(bufA)));
    }

    return 1;
}
#endif

}   //namespace rage
