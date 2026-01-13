// 
// system/uri.cpp 
// 
// Copyright (C) 1999-2020 Rockstar Games.  All Rights Reserved. 
// 

#include "uri.h"
#include "string/stringutil.h"
#include "system/nelem.h"
#include "diag/channel.h"
#include "diag/seh.h"

#include <stdio.h>

namespace rage
{

RAGE_DEFINE_CHANNEL(uri)

#define uriDebug(fmt, ...)						RAGE_DEBUGF1(uri, fmt, ##__VA_ARGS__)
#define uriDebug1(fmt, ...)						RAGE_DEBUGF1(uri, fmt, ##__VA_ARGS__)
#define uriDebug2(fmt, ...)						RAGE_DEBUGF2(uri, fmt, ##__VA_ARGS__)
#define uriDebug3(fmt, ...)						RAGE_DEBUGF3(uri, fmt, ##__VA_ARGS__)
#define uriDisplay(fmt, ...)					RAGE_DISPLAYF(uri, fmt, ##__VA_ARGS__)
#define uriWarning(fmt, ...)					RAGE_WARNINGF(uri, fmt, ##__VA_ARGS__)
#define uriError(fmt, ...)						RAGE_ERRORF(uri, fmt, ##__VA_ARGS__)
#define uriCondLogf(cond, severity, fmt, ...)	RAGE_CONDLOGF(uri, cond, severity, fmt, ##__VA_ARGS__)
#define uriVerify(cond)							RAGE_VERIFY(uri, cond)
#define uriVerifyf(cond, fmt, ...)				RAGE_VERIFYF(uri, cond, fmt, ##__VA_ARGS__)
#define uriAssert(cond) 						RAGE_ASSERT(uri, cond)
#define uriAssertf(cond, fmt, ...) 				RAGE_ASSERTF(uri, cond, fmt, ##__VA_ARGS__)

sysUri sysUri::INVALID_URI = sysUri();

const char* sysUri::s_supportedSchemes[] =
{
    "http",
    "https",
    "ws",
    "wss",
    ""
};

static_assert((sizeof(sysUri::s_supportedSchemes) / sizeof(char*)) == static_cast<size_t>(URISCHEME_COUNT), "The supported scheme list doesnt match the enum");

static const char HTTP_HEX_DIGITS[] = "0123456789abcdef";

// 5 digits, 1 colon and null terminated
void GetPortString(char(&dest)[7], const u16 port)
{
    dest[0] = '\0';

    if (port != 0)
    {
        formatf(dest, ":%u", port);
    }
}

//////////////////////////////////////////////////////////////////////////
//  URI Infos and Parser
//////////////////////////////////////////////////////////////////////////

sysUri::sysUri()
{
    Reset();
}

void sysUri::Reset()
{
    m_Scheme = URISCHEME_INVALID;
    m_Port = 0;
    m_Host[0] = 0;
    m_Path[0] = 0;
    m_QueryString[0] = 0;
    OUTPUT_ONLY(m_Str[0] = 0;)
}

bool sysUri::IsValid() const
{
    return m_Scheme > URISCHEME_INVALID && m_Scheme < URISCHEME_COUNT;
}

bool sysUri::operator==(const sysUri& rhs) const
{
    return m_Scheme == rhs.m_Scheme
        && m_Port == rhs.m_Port
        && strcmp(m_Host, rhs.m_Host) == 0
        && strcmp(m_Path, rhs.m_Path) == 0
        && strcmp(m_QueryString, rhs.m_QueryString) == 0;
}

bool sysUri::operator!=(const sysUri& rhs) const
{
    return !this->operator==(rhs);
}

char* sysUri::GetCompleteUri(char (&dest)[MAX_STRING_BUF_SIZE]) const
{
    return GetCompleteUri(dest, MAX_STRING_BUF_SIZE);
}

char* sysUri::GetCompleteUri(char* dest, const unsigned sizeOfDest) const
{
    if (!uriVerifyf(sizeOfDest > 0 && sizeOfDest >= MAX_STRING_BUF_SIZE, "Invalid buf len: sizeOfDest=%u", sizeOfDest))
        return dest;

    if (uriVerify(IsValid()))
    {
        char portStr[7];
        GetPortString(portStr, m_Port);
        return formatf_sized(dest, sizeOfDest, "%s://%s%s%s%s",
            s_supportedSchemes[m_Scheme],
            m_Host,
            portStr,
            m_Path,
            m_QueryString
        );
    }
    else
    {
        dest[0] = '\0';
        return dest;
    }
}

char* sysUri::GetHostUri(char(&dest)[MAX_STRING_BUF_SIZE]) const
{
    return GetHostUri(dest, MAX_STRING_BUF_SIZE);
}

char* sysUri::GetHostUri(char* dest, const unsigned sizeOfDest) const
{
    if (!uriVerifyf(sizeOfDest > 0 && sizeOfDest >= MAX_STRING_BUF_SIZE, "Invalid buf len: sizeOfDest=%u", sizeOfDest))
        return dest;

    if (uriVerify(IsValid()))
    {
        char portStr[7];
        GetPortString(portStr, m_Port);
        return formatf_sized(dest, sizeOfDest, "%s://%s%s",
            s_supportedSchemes[m_Scheme],
            m_Host,
            portStr
        );
    }
    else
    {
        dest[0] = '\0';
        return dest;
    }
}

bool sysUri::GetQueryStringValue(char(&dest)[MAX_QUERY_STRING_BUF_SIZE], const char* name) const
{
    rtry
    {
        rverifyall(IsValid());
        rcheckall(!StringNullOrEmpty(name));

        // Find the parameter
        const char* param = stristr(m_QueryString, name);
        rcheckall(param);

        // Find the '=' separator between the Name and Value
        const char* val = strchr(param, '=');
        rcheckall(val);
        val++;

        // Find the start of the next param if one is included.
        const char* end = strchr(val, '&');
        if (end)
        {
            // copy the entire value to the destination
            const unsigned valueLen = ptrdiff_t_to_int(end - val) + 1;
            safecpy(dest, val, valueLen);
        }
        else
        {
            // copy the rest of the string to the destination
            safecpy(dest, val);
        }

        return true;
    }
    rcatchall
    {
        dest[0] = '\0';
        return false;
    }
}

#if RSG_DURANGO
void sysUri::SetTitleId(const unsigned titleId)
{
    static char titleIdBuf[16];
    formatf(titleIdBuf, "ms-xbl-%08x", titleId);

    s_supportedSchemes[URISCHEME_TITLE] = titleIdBuf;
}
#endif

bool sysUri::Parse(const char* uri, sysUri& infos, bool bEncodeSpaceAsPlus, bool bUrlEncodePath)
{
    rtry
    {
        rverify(uri, catchall, uriError("NULL uri"));

        const char* host = strstr(uri, "://");
        rverify(host, catchall, uriError("Failed to locate '://' in uri '%s'", uri));

        char scheme[MAX_SCHEME_LENGTH] = {0};
        strncpy(scheme, uri, host - uri);

        rcheckall(!StringNullOrEmpty(scheme));

        // lookup the scheme in our supported array
        for(int i=0; i< URISCHEME_COUNT; i++)
        {
            if(strcmp(s_supportedSchemes[i], scheme) == 0)
            {
                infos.m_Scheme = (sysUriScheme) i;
            }
        }

        rverify(infos.m_Scheme != URISCHEME_INVALID, 
                catchall, 
                uriError("Could not determine uri scheme from '%s'", scheme));

        // Parse the host (looking for a port, a '/' or the end of the uri)
        host = host + 3; // move 3 characters to skip "://"

        const char* strQueryBegin = strstr(host, "?");

        const char* hostStrEnd = nullptr;
        const char* portStr = strstr(host, ":");

        // If the : is after the ? it's part of the query string and therefore it does not indicate a port number
        if(portStr && (strQueryBegin == nullptr || portStr < strQueryBegin))
        {
            hostStrEnd = portStr;
            portStr++; // skip ':'

            static const int MAX_PORT_STRING_LENGTH = 6; // "65535" + 1 null terminating character
            char port[MAX_PORT_STRING_LENGTH] = {0};
            // ignore any part after the port
            const char* path = strstr(host, "/");
            if(path)
            {
                rverify(path > portStr, 
                    catchall, 
                    uriError("Malformed Uri? Slash found before ':'"));

                size_t portLen = path - portStr;
                rverify(portLen < MAX_PORT_STRING_LENGTH, catchall, uriError("Port too long (%" SIZETFMT "d)", portLen));

                strncpy(port, portStr, portLen);
                port[portLen] = '\0';
            }
            else
            {
                safecpy(port, portStr);
            }

            bool result = sscanf(port, "%hu", &infos.m_Port) == 1;

            rverify(result, 
                    catchall, 
                    uriError("Failed to parse port '%s'", port));
        }
        else
        {
            const char* pathStr = strstr(host, "/");
            if(pathStr)
            {
                hostStrEnd = pathStr;
            }
            else
            {
                // if not, the host is just the whole string after "://"
                hostStrEnd = uri+strlen(uri);
            }
        }

        size_t len = hostStrEnd - host;
        rverify(len > 0, catchall, uriError("URI doesn't contain a host: %s", uri));
        rverify(len < MAX_HOSTNAME_BUF_SIZE, catchall, uriError("Host is too long (%" SIZETFMT "d > %d)", len, MAX_HOSTNAME_BUF_SIZE));

        strncpy(infos.m_Host, host, len);
        infos.m_Host[len] = '\0';

        // Extracts the Path
        const char* pathEnd = nullptr;
        // the path stops where the Query String starts, or at the end of the Uri if there's no query string
        if(strQueryBegin)
        {
            pathEnd = strQueryBegin;
            rverify(strlen(strQueryBegin) < MAX_QUERY_STRING_BUF_SIZE, catchall, uriError("Query String is too long (%" SIZETFMT "d > %d) and will be trucated", strlen(strQueryBegin), MAX_QUERY_STRING_BUF_SIZE));
            safecpy(infos.m_QueryString, strQueryBegin);
        }
        else
        {
            pathEnd = uri+strlen(uri);
        }
        const char* pathStr = strstr(host, "/");
        // If we found a '/' , the path starts here
        if(pathStr)
        {
            len = pathEnd - pathStr;
            rverify(len < MAX_URI_PATH_BUF_SIZE, catchall, uriError("Path is too long (%" SIZETFMT "d > %d)", len, MAX_URI_PATH_BUF_SIZE));

            char tempPath[MAX_URI_PATH_BUF_SIZE];
            strncpy(tempPath, pathStr, len);
            tempPath[len] = '\0';
            StringTrim(tempPath, tempPath, MAX_URI_PATH_BUF_SIZE);
            size_t tempPathLen = strlen(tempPath);

            // Url encode the path (null terminates it)
            if (bUrlEncodePath)
            {
                unsigned dstLen = COUNTOF(infos.m_Path);
                unsigned numConsumed;
                sysUri::UrlEncode(infos.m_Path, &dstLen, tempPath, (unsigned)tempPathLen, &numConsumed, "/", bEncodeSpaceAsPlus);

                rverify(tempPathLen == numConsumed, catchall, uriError("Failed to encode the path (path too long when encoded)"));
            }
            else 
            {
                safecpy(infos.m_Path, tempPath);
            }
        }

    }
    rcatchall
    {
        infos.Reset();
        return false;
    }

    return true;
}

bool
sysUri::NeedsUrlEncoding(const char* str, const unsigned len, const char* excludedChars)
{
    for (int i = 0; i < (int)len; ++i)
    {
        if (CharNeedsUrlEncoding(str[i], excludedChars))
        {
            return true;
        }
    }

    return false;
}

bool
sysUri::UrlEncode(char* dst,
                  unsigned* dstLen,
                  const int c,
                  const char* excludedChars,
                  bool usePlusForSpace)
{
    if (uriVerify(*dstLen >= 4))
    {
        if (' ' == c)
        {
            // Sometimes replacing ' ' with '+' is preferred.
            // Other times, replacing ' ' with '%20' is preferred.
            if (usePlusForSpace)
            {
                dst[0] = '+';
                dst[1] = '\0';
                *dstLen = 1;
            }
            else
            {
                dst[0] = '%';
                dst[1] = '2';
                dst[2] = '0';
                dst[3] = '\0';
                *dstLen = 3;
            }
        }
        else if (CharNeedsUrlEncoding(c, excludedChars))
        {
            dst[0] = '%';
            dst[1] = HTTP_HEX_DIGITS[c / 16];
            dst[2] = HTTP_HEX_DIGITS[c % 16];
            dst[3] = '\0';
            *dstLen = 3;
        }
        else
        {
            dst[0] = (char)c;
            dst[1] = '\0';
            *dstLen = 1;
        }

        return true;
    }

    return false;
}

bool
sysUri::UrlEncode(char* dst,
                  unsigned* dstLen,
                  const char* src,
                  const unsigned srcLen,
                  unsigned* numConsumed,
                  const char* excludedChars,
                  bool usePlusForSpace)
{
    char* p = dst;
    const char* eod = dst + *dstLen;
    *dstLen = 0;
    *numConsumed = 0;

    for (int i = 0; i < (int)srcLen; ++i)
    {
        char encoded[4];
        unsigned encodedLen = COUNTOF(encoded);

        bool result = UrlEncode(encoded, &encodedLen, (unsigned char)src[i], excludedChars, usePlusForSpace);

        uriAssertf(result, "Failed to encode char '%c' (index: %i) in %s", src[i], i, src);
        (void)result;

        if (!uriVerifyf(p + encodedLen < eod, "Buffer is too small to hold the encoded string (%s)", src))
        {
            break;
        }

        for (int i = 0; i < (int)encodedLen; ++i, ++p, ++*dstLen)
        {
            *p = encoded[i];
        }

        *p = '\0';
        ++*numConsumed;
    }

    return true;
}

unsigned
sysUri::GetUrlEncodedLength(const char* str,
                            const unsigned len,
                            const char* excludedChars,
                            bool usePlusForSpace)
{
    unsigned encLen = 0;

    for (int i = 0; i < (int)len; ++i)
    {
        if (' ' == str[i])
        {
            if (usePlusForSpace)
            {
                encLen += 1; //encoded with '+'
            }
            else
            {
                encLen += 3; //encoded with '%20'
            }
        }
        else if (sysUri::CharNeedsUrlEncoding(str[i], excludedChars))
        {
            encLen += 3;
        }
        else
        {
            encLen += 1;
        }
    }

    return encLen;
}

bool
sysUri::UrlDecode(char* dst,
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

    for (int i = 0; i < (int)srcLen && p < eod; ++i, ++p, ++*numConsumed)
    {
        const int c = (unsigned char)src[i];

        if ('%' == c && i < (int)srcLen - 2)
        {
            int tmp;
            if (1 == sscanf(&src[i], "%%%02x", &tmp))
            {
                *p = (char)tmp;
                i += 2;
            }
            else
            {
                uriDebug2("Invalid URL encoding");
                success = false;
            }
        }
        else if ((char)c == '+')
        {
            *p = ' ';
        }
        else
        {
            *p = (char)c;
        }
    }

    if (success)
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

#if !__NO_OUTPUT
const char* sysUri::ToString() const
{
    if (IsValid())
    {
        return m_Str[0] ? m_Str : GetCompleteUri(m_Str);
    }
    else
    {
        //Since this is only for output try to use something helpful
        return "<invalidUri>";
    }
}
#endif

} // namespace rage
