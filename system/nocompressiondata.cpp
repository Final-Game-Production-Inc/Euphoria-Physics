// 
// system/nocompressiondata.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "nocompressiondata.h"

#include <string.h>
#include "system/new.h"
#include "system/memops.h"

namespace rage {

sysNoCompressionData::sysNoCompressionData()
{

}

sysNoCompressionData::~sysNoCompressionData()
{

}

sysNoCompressionData* sysNoCompressionData::CreateNewInstance() const
{
    return rage_new sysNoCompressionData;
}

int sysNoCompressionData::BuildHeader( u8* buf, int ASSERT_ONLY(length) ) const
{
    const char* prefix = GetHeaderPrefix();
    int len = (int) strlen( prefix );
    for ( int i = 0; i < len; ++i )
    {
        buf[i] = prefix[i];
    }

    int index = len;

    Assert( GetCompressedSize() == GetDecompressedSize() );
    u32 compressedSize = GetCompressedSize();
    buf[index++] = (u8)(compressedSize);
    buf[index++] = (u8)(compressedSize >> 8);
    buf[index++] = (u8)(compressedSize >> 16);
    buf[index++] = (u8)(compressedSize >> 24);
    buf[index++] = ':';

    Assert( index < length );
    return index;
}

bool sysNoCompressionData::ParseHeader( u8* data, u32 dataLength )
{
    if ( dataLength < (u32)GetHeaderSize() )
    {
        // not enough data
        return false;
    }

    const char* prefix = GetHeaderPrefix();
    int len = (int) strlen( prefix );
    for ( int i = 0; i < len; ++i )
    {
        if ( data[i] != prefix[i] )
        {
            // bad header prefix
            return false;
        }
    }

    int index = len;

    u32 cSize = data[index++];
    cSize |= data[index++] << 8;
    cSize |= data[index++] << 16;
    cSize |= data[index++] << 24;

    if ( data[index++] != ':' )
    {
        // bad header
        return false;
    }

    if ( dataLength < GetHeaderSize() + cSize )
    {
        // not enough data
        return false;
    }

    SetCompressedSize( cSize );
    SetDecompressedSize( cSize );
    return true;
}

u32 sysNoCompressionData::Compress( u8* compressedData, u32 compressedDataLength, u8* uncompressedData, u32 uncompressedSize )
{
    memset( compressedData, 0, compressedDataLength );
    sysMemCpy( compressedData, uncompressedData, uncompressedSize );
    Assert( uncompressedSize <= compressedDataLength );
    return uncompressedSize;
}

bool sysNoCompressionData::Decompress( u8* decompressedData, u32 decompressedSize, u8* compressedData, u32 compressedSize )
{
    memset( decompressedData, 0, decompressedSize );
    sysMemCpy( decompressedData, compressedData, compressedSize );
    return true;
}

} // namespace rage
