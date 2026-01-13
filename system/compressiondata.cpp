// 
// system/compressiondata.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "compressiondata.h"

#include <string.h>

#include "system/new.h"
#include "system/memops.h"

namespace rage {

sysCompressionData::sysCompressionData()
	: m_ownsCompressedData(false)
	, m_compressedData(NULL)
	, m_compressedDataLength(0)
	, m_compressedSize(0)
	, m_ownsDecompressedData(false)
	, m_decompressedData(NULL)
	, m_decompressedDataLength(0)
	, m_decompressedSize(0)
{
}

sysCompressionData::~sysCompressionData()
{
	if ( m_ownsCompressedData && m_compressedData )
	{
		delete m_compressedData;
	}

	if ( m_ownsDecompressedData && m_decompressedData )
	{
		delete m_decompressedData;
	}

	m_compressedData = m_decompressedData = NULL;
	m_ownsCompressedData = m_ownsDecompressedData = false;
	m_compressedDataLength = m_decompressedDataLength = 0;
	m_compressedSize = m_decompressedSize = 0;
}

void sysCompressionData::SetCompressedData( u8* compressedData, u32 compressedDataLength, u32 compressedSize, bool copy )
{
	if ( m_ownsCompressedData && m_compressedData )
	{
		delete m_compressedData;
		m_compressedData = NULL;
		m_compressedDataLength = 0;
		m_ownsCompressedData = false;
	}

	//Assert( m_compressedData == NULL );

	if ( copy )
	{
		m_compressedData = rage_new u8[compressedDataLength];
		sysMemCpy( m_compressedData, compressedData, compressedDataLength );
		m_ownsCompressedData = true;
	}
	else
	{
		m_compressedData = compressedData;
	}

	m_compressedDataLength = compressedDataLength;
	m_compressedSize = compressedSize;
}

void sysCompressionData::SetDecompressedData( u8* decompressedData, u32 decompressedDataLength, u32 decompressedSize, bool copy )
{
	if ( m_ownsDecompressedData && m_decompressedData )
	{
		delete m_decompressedData;
		m_decompressedData = NULL;
		m_decompressedDataLength = 0;
		m_ownsDecompressedData = false;
	}

	//Assert( m_decompressedData == NULL );

	if ( copy )
	{
		m_decompressedData = rage_new u8[decompressedDataLength];
		sysMemCpy( m_decompressedData, decompressedData, decompressedDataLength );
		m_ownsDecompressedData = true;
	}
	else
	{
		m_decompressedData = decompressedData;
	}

	m_decompressedDataLength = decompressedDataLength;
	m_decompressedSize = decompressedSize;
}

bool sysCompressionData::Compress( bool resize )
{
	Assert( m_decompressedData != NULL );
	Assert( m_decompressedSize > 0 );

	// call the derived GetCompressUpperBound method
	u32 compressedLength = GetCompressUpperBound( m_decompressedSize );
	if ( m_ownsCompressedData && m_compressedData && (m_compressedDataLength < compressedLength) )
	{
		delete m_compressedData;
		m_compressedData = NULL;
		m_compressedDataLength = 0;
		m_ownsCompressedData = false;
	}

	if ( m_compressedData == NULL )
	{
		m_compressedData = rage_new u8[compressedLength];
		m_compressedDataLength = compressedLength;
		m_ownsCompressedData = true;
	}

	// call the derived Compress method
	m_compressedSize = Compress( m_compressedData, m_compressedDataLength, m_decompressedData, m_decompressedSize );
	Assert( m_compressedSize <= m_compressedDataLength );

	if ( resize && m_ownsCompressedData && (m_compressedSize > 0) && (m_compressedSize < m_compressedDataLength) )
	{
		u8* data = rage_new u8[m_compressedSize];
		sysMemCpy( data, m_compressedData, m_compressedSize );
		delete m_compressedData;		

		m_compressedData = data;
		m_compressedDataLength = m_compressedSize;
		m_ownsCompressedData = true;
	}

	return m_compressedSize > 0;
}

bool sysCompressionData::Decompress( bool resize )
{
	Assert( m_compressedData != NULL );
	Assert( m_compressedSize > 0 );
	Assert( m_decompressedSize > 0 );

	if ( m_ownsDecompressedData && m_decompressedData && (m_decompressedDataLength < m_decompressedSize) )
	{
		delete m_decompressedData;
		m_decompressedData = NULL;
		m_decompressedDataLength = 0;
		m_ownsDecompressedData = false;
	}

	if ( m_decompressedData == NULL )
	{
		m_decompressedData = rage_new u8[m_decompressedSize];
		m_decompressedDataLength = m_decompressedSize;
		m_ownsDecompressedData = true;
	}

	// call the derived Decompress method
	bool result = Decompress( m_decompressedData, m_decompressedSize, m_compressedData, m_compressedSize );
	if ( result && resize && m_ownsDecompressedData && (m_decompressedSize > 0) && (m_decompressedSize < m_decompressedDataLength) )
	{
		u8* data = rage_new u8[m_decompressedSize];
		sysMemCpy( data, m_decompressedData, m_decompressedSize );
		delete m_decompressedData;

		m_decompressedData = data;
		m_decompressedDataLength = m_decompressedSize;
		m_ownsDecompressedData = true;
	}

	return result;
}

} // namespace rage
