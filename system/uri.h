// 
// system/uri.h 
// 
// Copyright (C) 1999-2020 Rockstar Games.  All Rights Reserved. 
// 

#ifndef NET_URI_H 
#define NET_URI_H 

#include "string/string.h"

namespace rage
{

//PURPOSE
//  List of supported URI schemes
enum sysUriScheme
{
    URISCHEME_INVALID = -1,
    URISCHEME_HTTP,
    URISCHEME_HTTPS,
    URISCHEME_WS,
    URISCHEME_WSS,
    URISCHEME_TITLE,
    URISCHEME_COUNT,
};

#define	MAX_HOSTNAME_BUF_SIZE (128)
#define MAX_URI_PATH_BUF_SIZE (512)
#define MAX_QUERY_STRING_BUF_SIZE (512)

//PURPOSE
//  Struct containing Uri informations such as host, port etc
class sysUri
{
public:
    static sysUri INVALID_URI;

    static const int MAX_SCHEME_LENGTH = 15 + 1; // 15 for 'ms-xbl-2340236c', +1 for the null term;
    static const int MAX_STRING_BUF_SIZE = (MAX_SCHEME_LENGTH - 1) 
                                            + 3/*://*/ 
                                            + (MAX_HOSTNAME_BUF_SIZE - 1) //scheme minus null term
                                            + 1/*:*/ 
                                            + 5/*port*/ 
                                            + (MAX_URI_PATH_BUF_SIZE - 1) //uri path minus null term
                                            + (MAX_QUERY_STRING_BUF_SIZE - 1) //query string minus null term
                                            + 1/*nul*/;
    sysUri();

    void Reset();

    //PURPOSE
    //  Rebuilds the Uri after parsing.
    //PARAMS
    //  dest - A buffer that will be filled with the result.
    char* GetCompleteUri(char(&dest)[MAX_STRING_BUF_SIZE]) const;
    char* GetCompleteUri(char* dest, const unsigned sizeOfDest) const;

    //PURPOSE
    //  Rebuilds the host address with port after parsing (so without path and query string).
    char* GetHostUri(char(&dest)[MAX_STRING_BUF_SIZE]) const;
    char* GetHostUri(char* dest, const unsigned sizeOfDest) const;

    //PURPOSE
    //  Reads a named query string value from the Uri after parsing.
    //PARAMS
    //  dest - A buffer that will be filled with the result. Currently we only allow a buffer sized to exactly match the max needed.
    bool GetQueryStringValue(char(&dest)[MAX_QUERY_STRING_BUF_SIZE], const char* name) const;

    bool IsValid() const;
    bool operator==(const sysUri& rhs) const;
    bool operator!=(const sysUri& rhs) const;

#if !__NO_OUTPUT
    //PURPOSE
    //  Convenience function to return a string representation of the Uri
    //  without having to supply a buffer. Used in calls to printf()-like functions.
    //  Use GetCompleteUri() and supply a buffer in all other cases.
    const char* ToString() const;
#endif

#if RSG_DURANGO
    //PURPOSE
    //	Sets the title ID that can be used on XB1 for custom launch actions.
    static void SetTitleId(const unsigned titleId);
#endif // RSG_DURANGO

    //PURPOSE
    //  Parses an URI and extract the infos
    //PARAMS
    //  uri    - the uri to parse
    //  infos  - the struct that will hold the result
    //  bEncodeSpaceAsPlus - when encoding the path, should spaces be encoded as '+' or '%20'
    //  bUrlEncodePath - set to 'false' to disable URL encoding of the path during parsing.
    //INFOS
    //  URI syntax (https://en.wikipedia.org/wiki/Uniform_Resource_Identifier)
    //  SCHEME:[//[user:password@]HOST[:PORT]][/]PATH[?query][#fragment]
    static bool Parse(const char* uri, sysUri& infos, bool bEncodeSpaceAsPlus = true, bool bUrlEncodePath = true);

    //PURPOSE
    //  Returns true if the specified character would have to be encoded in an uri
    //  c               - The character to check
    //  excludedChars   - Null terminated string comprising characters
    //                    that are excluded from encoding.
    //                    Can be NULL.
    static bool CharNeedsUrlEncoding(const int c, const char* excludedChars);

    //PURPOSE
    //  Returns true if the string requires URL encoding.
    //  str     - The unencoded string.
    //  len     - Number of characters to encode.
    //  excludedChars   - Null terminated string comprising characters
    //                    that are excluded from encoding.
    //                    Can be NULL.
    static bool NeedsUrlEncoding(const char* str, const unsigned len, const char* excludedChars);

    //PURPOSE
    //  URL encodes a character.
    //PARAMS
    //  dst     - Destination string
    //  dstLen  - In: Length of destination buffer.
    //            Out: Length of encoded string.
    //  c       - Source character
    //  excludedChars   - Null terminated string comprising characters
    //                    that are excluded from encoding.
    //                    Can be NULL.
    //  bEncodeSpaceAsPlus - If true, use '+' for url encoding a space. Otherwise, use '%20' which uses more bytes but
    //                       is required for some web servers. The default is true, ('+')
    //RETURNS
    //  True on success.
    static bool UrlEncode(char* dst,
                          unsigned* dstLen,
                          const int c,
                          const char* excludedChars,
                          bool bEncodeSpaceAsPlus = true);

    //PURPOSE
    //  URL encodes a string.
    //PARAMS
    //  dst         - Destination buffer
    //  dstLen      - In: Length of destination buffer.
    //                Out: Length of encoded string.
    //  src         - Source string
    //  srcLen      - Length of source string
    //  numConsumed - Number of source chars consumed.
    //  excludedChars   - Null terminated string comprising characters
    //                    that are excluded from encoding.
    //                    Can be NULL.
    //  bEncodeSpaceAsPlus - If true, use '+' for url encoding a space. Otherwise, use '%20' which uses more bytes but
    //                       is required for some web servers. The default is true, ('+')
    //RETURNS
    //  True on success.
    static bool UrlEncode(char* dst,
                          unsigned* dstLen,
                          const char* src,
                          const unsigned srcLen,
                          unsigned* numConsumed,
                          const char* excludedChars,
                          bool bEncodeSpaceAsPlus = true);


    //PURPOSE
    //  Returns the length of the string if it were to be URL encoded.
    //PARAMS
    //  str                 - The unencoded string.
    //  len                 - Number of characters to encode.
    //  excludedChars       - Null terminated string comprising characters
    //                        that are excluded from encoding.
    //                        Can be NULL.
    //  bEncodeSpaceAsPlus  - If true, use '+' for url encoding a space. Otherwise, use '%20' which uses more bytes but
    //                        is required for some web servers. The default is true, ('+')
    static unsigned GetUrlEncodedLength(const char* str,
                                        const unsigned len,
                                        const char* excludedChars,
                                        bool bEncodeSpaceAsPlus = true);

    //PURPOSE
    //  Decodes a URL encoded string.
    //PARAMS
    //  dst         - Destination buffer
    //  dstLen      - In: Length of destination buffer.
    //                Out: Length of encoded string.
    //  src         - Source string
    //  srcLen      - Length of source string
    //  numConsumed - Number of source chars consumed.
    //RETURNS
    //  True on success.
    static bool UrlDecode(char* dst,
                          unsigned* dstLen,
                          const char* src,
                          const unsigned srcLen,
                          unsigned* numConsumed);


public:
    static const char* s_supportedSchemes[];

    sysUriScheme m_Scheme;
    char m_Host[MAX_HOSTNAME_BUF_SIZE];
    u16 m_Port;
    char m_Path[MAX_URI_PATH_BUF_SIZE];
    char m_QueryString[MAX_QUERY_STRING_BUF_SIZE];
#if !__NO_OUTPUT
    mutable char m_Str[MAX_STRING_BUF_SIZE];
#endif
};


inline bool sysUri::CharNeedsUrlEncoding(const int c, const char* excludedChars)
{
    //http://tools.ietf.org/html/rfc3986#section-2.3
    bool legal =
        (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || '_' == c || '-' == c || '.' == c || '~' == c;

    if (!legal && excludedChars)
    {
        //If it's in the excluded chars then it's considered legal.
        legal = (NULL != strchr(excludedChars, c));
    }

    return !legal;
}

} // namespace rage

#endif // NET_URI_H 
