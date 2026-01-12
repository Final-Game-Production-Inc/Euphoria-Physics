// 
// data/datcompressiondata.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "datcompressiondata.h"

#include <string.h>

#include "data/compress.h"

#include "system/new.h"

namespace rage {

datCompressionData::datCompressionData()
{
}

datCompressionData::~datCompressionData()
{
}

datCompressionData* datCompressionData::CreateNewInstance() const
{
	return rage_new datCompressionData;
}

u32 datCompressionData::GetCompressUpperBound( u32 uncompressedSize ) const
{
	return datCompressUpperBound( uncompressedSize );
}

int datCompressionData::BuildHeader( u8* buf, int ASSERT_ONLY(length) ) const
{
	const char* prefix = GetHeaderPrefix();
	int len = (int) strlen( prefix );
	for ( int i = 0; i < len; ++i )
	{
		buf[i] = prefix[i];
	}

	int index = len;

	u32 compressedSize = GetCompressedSize();
	buf[index++] = (u8)(compressedSize);
	buf[index++] = (u8)(compressedSize >> 8);
	buf[index++] = (u8)(compressedSize >> 16);
	buf[index++] = (u8)(compressedSize >> 24);
	buf[index++] = ':';

	u32 decompressedSize = GetDecompressedSize();
	buf[index++] = (u8)(decompressedSize);
	buf[index++] = (u8)(decompressedSize >> 8);
	buf[index++] = (u8)(decompressedSize >> 16);
	buf[index++] = (u8)(decompressedSize >> 24);
	buf[index++] = ':';

	Assert( index < length );
	return index;
}

bool datCompressionData::ParseHeader( u8* data, u32 dataLength )
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

	u32 dSize = data[index++];
	dSize |= data[index++] << 8;
	dSize |= data[index++] << 16;
	dSize |= data[index++] << 24;

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
	SetDecompressedSize( dSize );
	return true;
}

u32 datCompressionData::Compress( u8* compressedData, u32 compressedDataLength, u8* uncompressedData, u32 uncompressedSize )
{
	memset( compressedData, 0, compressedDataLength );
	u32 compressedSize = datCompress( compressedData, compressedDataLength, uncompressedData, uncompressedSize );
	Assert( compressedSize <= compressedDataLength );
	return compressedSize;
}

bool datCompressionData::Decompress( u8* decompressedData, u32 decompressedSize, u8* compressedData, u32 compressedSize )
{
	memset( decompressedData, 0, decompressedSize );
	u32 dSize = datDecompress( decompressedData, decompressedSize, compressedData, compressedSize );
	Assert( dSize <= decompressedSize );
	return dSize > 0;
}


} // namespace rage
