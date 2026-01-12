// 
// data/sax.h 
// 
// Copyright (C) Forever Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_SAX_H
#define DATA_SAX_H

namespace rage
{

//PURPOSE
//  A SAX-like (Simple API for XML) parser.
class datSaxReader
{
public:

    datSaxReader();

    virtual ~datSaxReader();

    //PURPOSE
    //  Call prior to calling Parse() for the first time.
    void Begin();

    //PURPOSE
    //  Call after parsing the entire document.
    void End();

    //PURPOSE
    //  Returns true after calling Begin() and before calling End(),
    //  assuming no errors occur.
    bool Pending() const;

    //PURPOSE
    //  Returns true after calling End() assuming no errors occurred.
    bool Succeeded() const;

    //PURPOSE
    //  Returns true if an error occurred.
    bool Failed() const;

    //PURPOSE
    //  Parses an XML stream.
    //PARAMS
    //  buf         - XML buffer
    //  offset      - Offset within buf to start parsing
    //  lenofBuf    - Length of buf
    //RETURNS
    //  Number of chars consumed while parsing.
    unsigned Parse(const char* buf, const int offset, const unsigned lenofBuf);

    //Following are SAX-like methods.  http://www.saxproject.org/
    //Subclasses should override the ones of interest.
    //This implementation passes the empty string for url and localName.

    //PURPOSE
    //  Called at the start of an XML document
    virtual void startDocument();

    //PURPOSE
    //  Called at the end of an XML document
    virtual void endDocument();

    //PURPOSE
    //  Called at the start of an XML element
    //PARAMS
    //  uri         - The Namespace URI.
    //                (Empty string in this implementation)
    //  localName   - The local name (without prefix).
    //                (Empty string in this implementation)
    //  qName       - The qualified name (with prefix)
    virtual void startElement(const char* uri, const char* localName, const char* qName);

    //PURPOSE
    //  Called at the end of an XML element
    //PARAMS
    //  uri         - The Namespace URI.
    //                (Empty string in this implementation)
    //  localName   - The local name (without prefix).
    //                (Empty string in this implementation)
    //  qName       - The qualified name (with prefix)
    virtual void endElement(const char* uri, const char* localName, const char* qName);

    //PURPOSE
    //  Called as text is parsed between start/end tags.
    //PARAMS
    //  ch      - Character buffer
    //  start   - Offset into character buffer
    //  length  - Length of character buffer
    virtual void characters(const char* ch, const int start, const int length);

    //PURPOSE
    //  Called for each attribute in a start element.
    //PARAMS
    //  tagName     - Qualified tag name
    //  attrName    - Attribute name
    //  attrVal     - Attribute value
    //NOTES
    //  This isn't part of SAX but it's useful to have.
    virtual void attribute(const char* tagQName, const char* attrName, const char* attrVal);

    //PURPOSE
    //  Returns true if the string requires XML encoding.
    static bool NeedsXmlEncoding(const char* str,
                                const unsigned len);

    //PURPOSE
    //  XML encodes a character.
    //PARAMS
    //  dst     - Destination string
    //  dstLen  - In: Length of destination buffer.
    //            Out: Length of encoded string.
    //  c       - Source character
    //RETURNS
    //  True on success.
    static bool XmlEncode(char* dst,
                            unsigned* dstLen,
                            const int c);

    //PURPOSE
    //  XML encodes a string.
    //PARAMS
    //  dstLen      - In: Length of destination buffer.
    //                Out: Length of encoded string.
    //  src         - Source string
    //  srcLen      - Length of source string
    //  dst         - Destination buffer
    //  numConsumed - Number of source chars consumed.
    //RETURNS
    //  True on success.
    static bool XmlEncode(char* dst,
                            unsigned* dstLen,
                            const char* src,
                            const unsigned srcLen,
                            unsigned* numConsumed);

    //PURPOSE
    //  Decodes an XML encoded string.
    //PARAMS
    //  dst         - Destination buffer
    //  dstLen      - In: Length of destination buffer.
    //                Out: Length of encoded string.
    //  src         - Source string
    //  srcLen      - Length of source string
    //  numConsumed - Number of source chars consumed.
    //RETURNS
    //  True on success.
    static bool XmlDecode(char* dst,
                            unsigned* dstLen,
                            const char* src,
                            const unsigned srcLen,
                            unsigned* numConsumed);

    //PURPOSE
    //  Decodes an escaped character.  Returns -1 on failure.
    static int XmlDecodeEscape(const char* src,
                                const unsigned srcLen,
                                unsigned* numConsumed);

#if __ASSERT
    //For unit testing
    static int TestXmlEncoding();
#endif

private:

    enum State
    {
        STATE_INVALID,
        STATE_START_DOCUMENT,
        STATE_EXPECT_START_LEFT,
        STATE_EXPECT_START_NAME,
        STATE_SCAN_START_NAME,
        STATE_EXPECT_ATTR_NAME,
        STATE_SCAN_ATTR_NAME,
        STATE_EXPECT_EQ,
        STATE_EXPECT_START_QUOTE,
        STATE_SCAN_ATTR_VALUE,
        STATE_EXPECT_SEMICOLON,
        STATE_EXPECT_CHAR_DATA,
        STATE_EXPECT_ESCAPED_CHAR_DATA,
        STATE_EXPECT_COMMENT_OR_CDATA,
        STATE_EXPECT_START_CDATA,
        STATE_EXPECT_CDATA,
        STATE_EXPECT_END_CDATA,
        STATE_EXPECT_END_NAME,
        STATE_SCAN_END_NAME,
        STATE_EXPECT_END_RIGHT,
        STATE_EXPECT_END_SECTION,

        STATE_FINISHED,

        STATE_ERROR
    };

    State m_State;
    char m_Token[32];
    int m_TokenLen;
    char m_TagQName[64];
    int m_TagNameLen;
    char m_AttrName[64];
    int m_AttrNameLen;
    char m_AttrVal[1024];
    int m_AttrValLen;
    int m_CharDataOffset;
    int m_QuoteChar;
};

}   //namespace rage

#endif  //DATA_SAX_H
