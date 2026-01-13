// 
// system/nocompressiondata.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_NOCOMPRESSIONDATA_H 
#define SYSTEM_NOCOMPRESSIONDATA_H 

#include "compressiondata.h"

namespace rage {

// PURPOSE:	Class for integrating uncompressed data buffers with a compression factory.
//	Mirrored in C# in rage/base/tools/ragCompression, this class is an interface between the game
//	and Rag.
class sysNoCompressionData : public sysCompressionData
{
public:
    sysNoCompressionData();
    ~sysNoCompressionData();

    // PURPOSE: Creates a new instance of this class to be used by the sysCompressionFactory
    // RETURNS: pointer to new instance
    sysNoCompressionData* CreateNewInstance() const;

    // PURPOSE: Retrieves an estimate of the maximum length the of the compressed data with the given size using fiCompressUpperBound
    // PARAMS:
    //  uncompressedSize - size of the data to be compressed
    // RETURNS: maximum length of compressedData
    u32 GetCompressUpperBound( u32 uncompressedSize ) const;

    // PURPOSE: Retrieves the size of this class's header data
    // RETURNS: number of bytes of header data
    int GetHeaderSize() const;

    // PURPOSE: Retrieves the unique header prefix string so the Compression Factory can find the start of a CompressedData set
    // RETURNS: header prefix string
    const char* GetHeaderPrefix() const;

    // PURPOSE: Builds the header section for the Compression Factory
    // PARAMS:
    //  buf - buffer to write to
    //  length - length of buffer to make sure we don't write past the end
    // RETURNS: length of header
    int BuildHeader( u8* buf, int length ) const;

    // PURPOSE: Assuming compressedData starts with a valid header, retrieves the compressed data size and decompressed data size
    // PARAMS:
    //  data - compressed data, starting with the header prefix
    //  dataLength - length of compressedData
    // RETURNS: true if the header was parsed correctly
    bool ParseHeader( u8* data, u32 dataLength );

    // PURPOSE: Compresses the data into a buffer using fiCompress
    // PARAMS:
    //  compressedData - buffer to write the compressed data to
    //  compressedDataLength - length of the buffer
    //  uncompressedData - data that we want to compress
    //  uncompressedSize - number of bytes to be compressed
    // RETURNS: the number of bytes compressed to compressedData.  0 on error.
    u32 Compress( u8* compressedData, u32 compressedDataLength, u8* uncompressedData, u32 uncompressedSize );

    // PURPOSE: Decompresses the data into a buffer of the specified length using fiDecompress
    // PARAMS:
    //  decompressedData - buffer to decompress the data to
    //  decompressedSize - number of bytes in the uncompressed stream (the original DecompressedSize)
    //  compressedData - data to be decompressed
    //  compressedSize - number of bytes to be decompressed
    // RETURNS: true on success
    bool Decompress( u8* decompressedData, u32 decompressedSize, u8* compressedData, u32 compressedSize );

    // PURPOSE: Retrieves the unique header prefix string so the Compression Factory can find the start of a CompressedData set
    //  Useful when calling for sysCompressionFactory::CompressData( const char*, u8*, u32, u32& ).
    // RETURNS: header prefix string
    static const char* GetStaticHeaderPrefix();
};

inline u32 sysNoCompressionData::GetCompressUpperBound( u32 uncompressedSize ) const
{
    return uncompressedSize;
}

inline int sysNoCompressionData::GetHeaderSize() const
{
    return 9; // header: CHN:0000: where the 0000 is a u32 and everything else is a character
}

inline const char* sysNoCompressionData::GetHeaderPrefix() const
{
    return sysNoCompressionData::GetStaticHeaderPrefix();
}

inline const char* sysNoCompressionData::GetStaticHeaderPrefix()
{
    return "CHN:";
}

} // namespace rage

#endif // SYSTEM_NOCOMPRESSIONDATA_H 
