// 
// system/compressionfactory.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "compressionfactory.h"
#include "compressiondata.h"
#include "system/memops.h"

namespace rage {

	sysCompressionFactory::sysCompressionFactory( u32 typeCapacity, u32 arrayCapacity )
		: m_copyCompressedData(false)
		, m_readData(NULL)
		, m_readDataSize(0)
		, m_start(0)
		, m_end(0)
	{
		m_registeredTypes.Reserve( typeCapacity );
		m_compressionData.Reserve( arrayCapacity );
	}

	sysCompressionFactory::~sysCompressionFactory()
	{
		if ( m_readData )
		{
			delete m_readData;
			m_readData = NULL;
		}

		m_readDataSize = 0;

		m_compressionData.clear();

		atStringMap<sysCompressionData*>::Iterator end = m_registeredTypes.End();
		for ( atStringMap<sysCompressionData*>::Iterator iter = m_registeredTypes.Begin(); iter != end; ++iter )
		{
			delete *iter;
		}
		m_registeredTypes.Reset();
	}

	void sysCompressionFactory::RegisterCompressionData( sysCompressionData *cData )
	{
		m_registeredTypes.SafeInsert( cData->GetHeaderPrefix(), cData );
		m_registeredTypes.FinishInsertion();
	}

	u8* sysCompressionFactory::CompressData( u32 &dataSize )
	{
		int count = m_compressionData.GetCount();

		// Compress if needed and determine total length
		dataSize = 0;
		for ( int i = 0; i < count; ++i )
		{
			sysCompressionData* cData = m_compressionData[i];
			if ( cData->GetCompressedSize() == 0 )
			{
				cData->Compress();
			}

			if ( cData->GetCompressedSize() > 0 )
			{
				dataSize += cData->GetHeaderSize() + cData->GetCompressedSize();
			}
		}

		if ( dataSize == 0 )
		{
			return NULL;
		}

		// build the whole stream
		u8* compressedStream = rage_new u8[dataSize];
		memset( compressedStream, 0, dataSize );
		u8* writePtr = compressedStream;
		for ( int i = 0; i < count; ++i )
		{
			sysCompressionData* cData = m_compressionData[i];
			if ( cData->GetCompressedSize() > 0 )
			{
				int headerSize = cData->BuildHeader( writePtr, int((compressedStream + dataSize) - writePtr) );
				writePtr += headerSize;
				sysMemCpy( writePtr, cData->GetCompressedData(), cData->GetCompressedSize() );
				writePtr += cData->GetCompressedSize();
			}
		}

		return compressedStream;
	}

	bool sysCompressionFactory::DecompressData( u8* data, u32 dataSize )
	{
		bool rtn = false;

		bool ownsReadData = false;
		if ( m_readData == NULL )
		{
			m_readData = data;
			m_readDataSize = dataSize;
		}
		else
		{
			u32 newSize = dataSize + m_readDataSize;
			u8* newData = rage_new u8[newSize];
			u8* writePtr = newData;
			sysMemCpy( writePtr, m_readData, m_readDataSize );
			writePtr += m_readDataSize;
			sysMemCpy( writePtr, data, dataSize );

			delete m_readData;
			m_readData = newData;
			m_readDataSize = newSize;
			ownsReadData = true;
		}

		while ( true )
		{
			m_start = -1;

			char* startStr = (char *)0xffffffff;
			sysCompressionData* cData = NULL;
			char* readDataStr = (char *)(&(m_readData[m_end]));

			// loop through our registered types and try to match a prefix, the earliest if multiple are found
			atStringMap<sysCompressionData*>::Iterator end = m_registeredTypes.End();
			for ( atStringMap<sysCompressionData*>::Iterator iter = m_registeredTypes.Begin(); iter != end; ++iter )
			{
				char* s = strstr( readDataStr, (*iter)->GetHeaderPrefix() );
				if ( (s != NULL) && (s < startStr) )
				{
					startStr = s;
					cData = *iter;
				}
			}

			if ( cData != NULL )
			{
				m_start = int(startStr - (char *)m_readData);

				if ( cData->ParseHeader( &(m_readData[m_start]), m_readDataSize - m_start ) )
				{
					sysCompressionData* newData = cData->CreateNewInstance();

					newData->SetCompressedData( &(m_readData[m_start + cData->GetHeaderSize()]), 
						cData->GetCompressedSize(), cData->GetCompressedSize(), m_copyCompressedData );

					// have to pass in decompressedSize because we didn't call newData->ParseHeader(...)
					if ( newData->Decompress( cData->GetDecompressedSize() ) )
					{
						m_compressionData.Grow() = newData;
						m_end = m_start + cData->GetHeaderSize() + newData->GetCompressedSize();
					}
					else
					{
						Displayf( "Decompression failed" );
					}
				}
				else
				{
					// more data to receive for this set of compressed data
					rtn = true;
					break;
				}
			}
			else
			{
				// end of string
				rtn = false;
				break;
			}
		}

		if ( m_start > -1 )
		{
			// remove data we've already processed
			int newSize = m_readDataSize - m_start;
			u8* newData = rage_new u8[newSize];
			sysMemCpy( newData, &(m_readData[m_start]), newSize );

			if ( ownsReadData )
			{
				delete m_readData;
			}

			m_readData = newData;
			m_readDataSize = newSize;
			m_start = m_end = 0;
		}
		else
		{
			m_start = m_end = 0;
			m_readData = NULL;
			m_readDataSize = 0;
		}

		return rtn;
	}

	u32 sysCompressionFactory::GetCompressedDataSize( const char *type, u32 uncompressedSize) const
	{
		const sysCompressionData* const* cData = m_registeredTypes.SafeGet( type );
		if ( (*cData) == NULL )
		{
			return 0;
		}

		int headerSize = (*cData)->GetHeaderSize();
		return (*cData)->GetCompressUpperBound( uncompressedSize ) + headerSize;
	}

	u8* sysCompressionFactory::CompressData( const char* type, u8* uncompressedData, u32 uncompressedSize, u32 &dataSize, u8 *compressedDataBuffer, u32 compressedDataBufferSize )
	{
		sysCompressionData** cData = m_registeredTypes.SafeGet( type );
		if ( (*cData) == NULL )
		{
			return NULL;
		}

		(*cData)->SetDecompressedData( uncompressedData, uncompressedSize, uncompressedSize );

		u8* compressedStream = sysCompressionFactory::CompressData( *cData, dataSize, compressedDataBuffer, compressedDataBufferSize );
		if ( compressedStream )
		{
			// zero out so we don't get confused or try to delete something we shouldn't
			(*cData)->SetDecompressedData( NULL, 0 );
			(*cData)->SetCompressedData( NULL, 0 );

			return compressedStream;
		}

		return NULL;
	}

	u8* sysCompressionFactory::CompressData( sysCompressionData* cData, u32 &dataSize, u8 *compressedStream, u32 ASSERT_ONLY(compressedStreamSize) )
	{	
		int headerSize = cData->GetHeaderSize();
		u32 dataLength = cData->GetCompressUpperBound( cData->GetDecompressedSize() ) + headerSize;

		if (!compressedStream)
		{
			compressedStream = rage_new u8[dataLength];
		}
		else
		{
			Assertf(dataLength <= compressedStreamSize, "Provided stream buffer is not big enough, needs at least %d bytes", dataLength);
		}

		// use our buffer so we don't have to create a new one to add the header
		cData->SetCompressedData( &(compressedStream[headerSize]), dataLength - headerSize );

		if ( cData->Compress() )
		{
			int hSize = cData->BuildHeader( compressedStream, dataLength );
			Assert( hSize == headerSize );

			dataSize = cData->GetCompressedSize() + hSize;
			return compressedStream;
		}

		return NULL;
	}

} // namespace rage
