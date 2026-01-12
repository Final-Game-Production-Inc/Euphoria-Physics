// 
// data/base64.cpp
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "base64.h"
#include "system/memory.h"
#include <string.h>
#include "atl/string.h"
#include "string/string.h"

namespace rage
{

static const int BASE64_SHIFT[4] =
{
    10, 4, 6, 8,
};

static const char BASE64_ALPHABET[65] =
{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/"
};

unsigned 
datBase64::GetMaxEncodedSize(const unsigned size)
{
    //Add 1 for the terminating null.
    return (((size + 2) / 3) * 4) + 1;
}

unsigned
datBase64::GetMaxDecodedSize(const char* str)
{
    size_t len = strlen(str);

    if(len)
    {
        //Don't count any padding at the end
        while(str[len-1] == '=') --len;        
        return (unsigned)((len * 6) / 8);
    }

    return 0;
}

bool
datBase64::Encode(const u8* src, 
                  const unsigned srcLen,
                  char* dst,
                  const unsigned dstMaxSize,
                  unsigned* charsUsed)
{
    if(!Verifyf(src && srcLen && dstMaxSize && dst, "Invalid arguments"))
    {
        return false;
    }

    const unsigned maxSize = GetMaxEncodedSize(srcLen);

    if(!Verifyf(dstMaxSize >= maxSize,
                "Insufficient buffer length for string data (dstMaxSize=%u, size required=%u)",
                dstMaxSize, maxSize))
    {
        return false;
    }

    sysMemSet(dst, 0, dstMaxSize);

    int idx0 = 0;    
    int idx1 = 6;
    
    const int numBits = (int)srcLen * 8;
    unsigned len = (numBits + 5) / 6;

    for(unsigned i = 0; i < len; ++i, idx0 += 6, idx1 += 6)
    {
        unsigned tmp = (src[idx0 >> 3] << 8);
        if(idx1 < numBits)
        {
            tmp |= src[idx1 >> 3];
        }

        const int shift = BASE64_SHIFT[i & 0x03];
        const u8 val = (u8) (tmp >> shift) & 0x3F;

        dst[i] = BASE64_ALPHABET[val];
    }

    while(len < maxSize-1)
    {
        dst[len++] = '=';
    }

    Assert(len < dstMaxSize);
    dst[len] = '\0';

    if(charsUsed){*charsUsed = maxSize;}

    return true;
}

bool
datBase64::Decode(const char *src, 
                  const unsigned dstMaxLen,
                  u8 *dst,
                  unsigned *dstLen)
{
    bool success = false;

    *dstLen = 0;

    if(src)
    {
        unsigned maxDecodedLen = GetMaxDecodedSize(src);

        if(maxDecodedLen && Verifyf(dstMaxLen >= maxDecodedLen, "Insufficient buffer length for binary data"))
        {
            DecodeContext dc;
            const int srcLen = (int)strlen(src);
            const unsigned numConsumed = Decode(&dc, src, 0, srcLen, dstMaxLen, dst, dstLen);
            success = AssertVerify(numConsumed == (unsigned)srcLen);
        }
    }

    return success;
}

#if __ASSERT
static int TestMe()
{
    const char* inputStr[] =
    {
        "MTIzNDU2Nzg=",
        "MTIzNDU2Nzg5",
        "MTIzNDU2Nzg5MA==",
        "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXo="
    };
    const char* outputStr[] =
    {
        "12345678",
        "123456789",
        "1234567890",
        "abcdefghijklmnopqrstuvwxyz"
    };

    for(int i = 0; i < 4; ++i)
    {
        datBase64::DecodeContext dc;
        u8 buf[64];
        unsigned dstLen = 0;
        unsigned numConsumed;
        const int srcLen = (int)strlen(inputStr[i]);

        numConsumed = datBase64::Decode(&dc, inputStr[i], 0, srcLen, 64, buf, &dstLen);
        buf[dstLen] = '\0';
        Assert(0 == strcmp(outputStr[i], (const char*) buf));

        numConsumed = dstLen = 0;
        dc.Clear();
        for(int j = 0; j < srcLen; ++j)
        {
            numConsumed += datBase64::Decode(&dc, &inputStr[i][j], 0, 1, 64, buf, &dstLen);
        }

        buf[dstLen] = '\0';
        Assert(0 == strcmp(outputStr[i], (const char*) buf));

        numConsumed = dstLen = 0;
        dc.Clear();
        for(int j = 0; j < srcLen; j += 3)
        {
            numConsumed += datBase64::Decode(&dc, inputStr[i], numConsumed, srcLen, 64, buf, &dstLen);
        }

        buf[dstLen] = '\0';
        Assert(0 == strcmp(outputStr[i], (const char*) buf));
    }

    return 1;
}

static const int TESTED = TestMe();
#endif  //__DEV

unsigned
datBase64::Decode(DecodeContext* ctx,
                  const char *src,
                  const int srcOffset,
                  const int srcLen,
                  const unsigned dstMaxLen,
                  u8 *dst,
                  unsigned* dstLen)
{
    int dstIndex = *dstLen;

    int i;

    //Consume src while:
    //- We haven't reached the end of src
    //- Or, we run out of dest buffer, and the next char is pad
    for(i = srcOffset; i < srcLen && (dstIndex < (int)dstMaxLen || '=' == src[i]); ++i)
    {
        if('=' == src[i])
        {            
            continue; //Ignore trailing pad characters.
        }

        ctx->bits = (ctx->bits << 6) | CharToValue(src[i]);

        ctx->numBits += 6;

        if(ctx->numBits >= 8)
        {
            dst[dstIndex++] = u8((ctx->bits >> (ctx->numBits - 8)) & 0xFF);
            ctx->numBits -= 8;
        }
    }

    *dstLen = dstIndex;

    return i-srcOffset;
}

int
datBase64::CharToValue(const char code)
{
    int val;

    if(code >= 'A' && code <= 'Z')
    {
        val = code - 'A';
    }
    else if(code >= 'a' && code <= 'z')
    {
        val = code - 'a' + 26;
    }
    else if(code >= '0' && code <= '9')
    {
        val = code - '0' + 52;
    }
    else if('+' == code)
    {
        val = 62;
    }
    else if('/' == code)
    {
        val = 63;
    }
    else
    {
        val = -1;
    }

    return val;
}

bool 
datBase64::IsValidBase64Char(const char c)
{
	// check all valid characters
	if(c >= 'A' && c <= 'Z')
		return true; 
	else if(c >= 'a' && c <= 'z')
		return true; 
	else if(c >= '0' && c <= '9')
		return true; 
	else if((c == '+') || (c == '/') || (c == '=')) // '=' is used for padding
		return true;

	return false;
}

bool 
datBase64::IsValidBase64String(const char* src, const unsigned srcLen)
{
	unsigned length = srcLen;
	if(length == 0)
	{
		length = static_cast<unsigned>(StringLength(src));
	}

	// check all characters are valid
	for(unsigned i = 0; i < length; i++)
	{
		if(!IsValidBase64Char(src[i]))
			return false;
	}

	const unsigned MAX_STRING_LENGTH = 1024; 

	// if all characters are valid, check that the re-encoded string matches the source
	unsigned char decodedString[MAX_STRING_LENGTH];
	unsigned decodedLength = 0;
	if(!datBase64::Decode(src, length, decodedString, &decodedLength))
		return false;

	char encodedString[MAX_STRING_LENGTH];
	unsigned encodedLength = 0;
	if(!datBase64::Encode(decodedString, decodedLength, encodedString, MAX_STRING_LENGTH, &encodedLength))
		return false;

	if(strncmp(src, encodedString, length) != 0)
		return false;

	// all tests passed
	return true;
}

}   //namespace rage
