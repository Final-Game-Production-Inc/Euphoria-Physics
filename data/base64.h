// 
// data/base64.h
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_BASE64_H
#define DATA_BASE64_H

namespace rage
{

#define DAT_BASE64_MAX_ENCODED_SIZE(size)\
    (((((size) + 2) / 3) * 4) + 1)  /*Add 1 for the terminating null*/

//PURPOSE
//  Methods for converting to and from binary to base-64 strings.
class datBase64
{
public:
    //PURPOSE
    //  Returns maximum string length that Encode() will produce for this binary data.
    //NOTE
    //  The value returned includes the terminating null required for strings.
    static unsigned GetMaxEncodedSize(const unsigned size);

    //PURPOSE
    //  Returns maximum binary data length that Decode() will produce for this string.
    static unsigned GetMaxDecodedSize(const char* str);

    //PURPOSE
    //  Encodes binary data as a base-64 string.
    //PARAMS
    //  src         - Source data
    //  srcLen      - Length of source data
    //  dst         - Destination buffer
    //  dstMaxSize  - Maximum length of destination buffer
    //  charsUsed   - OPTIONAL.  On success will contain the number of
    //                chars used from the destination buffer.
    //NOTE
    //  This does NOT append pad characters.
    static bool Encode(const u8* src, 
                       const unsigned srcLen,
                       char* dst,
                       const unsigned dstMaxSize,
                       unsigned* charsUsed);

    //PURPOSE
    //  Decodes a base-64 string into binary data.
    //NOTE
    //  This will ignore pad characters.
    static bool Decode(const char *src, 
                       const unsigned dstMaxLen,
                       u8 *dst,
                       unsigned *dstLen);

    //PURPOSE
    //  Used to hold decode context for the streaming version of Decode
    struct DecodeContext
    {
        DecodeContext()
        {
            Clear();
        }

        void Clear()
        {
            numBits = bits = 0;
        }

        unsigned numBits;
        unsigned bits;
    };

    //PURPOSE
    //  Streaming version of decode
    //PARAMS
    //  ctx         - Decode context.
    //                Initialize to zeros prior to the first call to Decode.
    //  src         - Base64 source string
    //  srcOffset   - Offset into src from which to begin decoding
    //  srcLen      - Length of src
    //  dstMaxLen   - Maximum length of the destination buffer
    //  dst         - Destination buffer
    //  dstLen      - IN: Current length of destination buffer
    //                (should be zero on the first call to Decode).
    //                OUT: Length of destination buffer after decoding.
    //EXAMPLE
    //  void DoDecode(const char* input)
    //  {
    //      DecodeContext dc = {0};
    //      u8 data[8];
    //      unsigned dataLen = 0;
    //      unsigned numConsumed = 0;
    //      const int srcLen = (int)strlen(input);
    //      while(numConsumed < (unsigned) srcLen)
    //      {
    //          numConsumed += datBase64::Decode(&dc, input, (int)numConsumed, srcLen, 8, data, &dataLen);
    //          dataLen = 0;
    //      }
    //  }
    static unsigned Decode(DecodeContext* ctx,
                            const char *src,
                            const int srcOffset,
                            const int srcLen,
                            const unsigned dstMaxLen,
                            u8 *dst,
                            unsigned* dstLen);

    //PURPOSE
    //  Converts a base64 char to the value it represents.
    //  E.g. 'A' represents zero, 'a' represents 26.
    static int CharToValue(const char c);

	//PURPOSE
	//  Returns true if c represents a valid base64 character
	static bool IsValidBase64Char(const char c);

	//PURPOSE
	//  Returns true if src represents a valid base64 string
	static bool IsValidBase64String(const char* src, const unsigned srcLen = 0);
};

}; //namespace rage

#endif //DATA_BASE64_H