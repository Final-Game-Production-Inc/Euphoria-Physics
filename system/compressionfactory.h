// 
// system/compressionfactory.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_COMPRESSIONFACTORY_H 
#define SYSTEM_COMPRESSIONFACTORY_H 

#include "atl/array.h"
#include "atl/binmap.h"

namespace rage {

	class sysCompressionData;

	// PURPOSE: A class to construct and destruct multiple sets of compressed and uncompressed
	//  data into consecutive streams.  Mirrored in C# in rage/base/tools/ragCompression, 
	//  this class serves as an interface between the game and Rag.
	//
	//  Classes derived from sysCompressionData must be registered in order to operate the 
	//  compress and decompress functions.
	class sysCompressionFactory
	{
	public:
		sysCompressionFactory( u32 typeCapacity=4, u32 dataCapacity=16 );
		~sysCompressionFactory();

		// PURPOSE: Adds a derived sysCompressionData class to the types that can be
		//  handled by this factory.  This class will take ownership over the memory
		//  for the sysCompressionData object and will automatically clean it up
		//  when the factory is destructed.
		// PARAMS:
		//  data - the derived class to register.
		void RegisterCompressionData( sysCompressionData *cData );

		// PURPOSE: Retrieves the list of sysCompressionData objects.  This can be used
		//  to grow the list before calling CompressData or to read the list after calling
		//  DecompressData.  Clearing the list and deleting every item is up to the user.
		// RETURNS: array of sysCompressionData objects
		atArray<sysCompressionData*>& GetData();

		// PURPOSE: Return the state of m_copyCompressedData
		// RETURNS: m_copyCompressedData
		bool GetCopyCompressedData() const;

		// PURPOSE: Specifies whether each sysCompressionData should hold a copy of the compressed data when decompressed
		//  via DecompressData.
		// PARAMS: b - true or false
		void SetCopyCompressedData( bool b );

		// PURPOSE: Using all items in the sysCompressionData list, builds one massive stream out of each set of compressed data,
		//  adding the header info to each one.  Compression is performed if it wasn't already.
		// PARAMS:
		//  dataSize - return by reference the size of the buffer returned
		// RETURNS: the compressed data.  null on error.
		// NOTE:  The user will be responsible for deleting the buffer that was returned
		u8* CompressData( u32 &dataSize );

		// PURPOSE: Takes in an array of bytes and processes each compressed stream, decompressing as it goes.
		//  Multiple sets of compressed data could exist in the data array, and a partial set may exist at the end
		//  of the array.  When this partial data exists, we return true.  Call DecompressData again with the rest of the data
		//  (example: reading from a Socket) to process the remaining data and continue.
		// PARAMS:
		//  data - the data we want decompressed
		//  dataSize - length of data or the number of bytes we want processed
		// RETURNS: true if not all of the data could be processed
		bool DecompressData( u8* data, u32 dataSize );

		// PURPOSE: Compresses the given data and adds the header.  This will not add an entry to the compression data list.
		//  The user is responsible for deleting the memory.
		// PARAMS:
		//  type - which registered sysCompressionData to use.  What is returned by sysCompressionData::GetHeaderPrefix();
		//  uncompressedData - data that we want to compress
		//  uncompressedSize - number of bytes to compress
		//  dataSize - return by reference the size of the buffer returned
		//  compressedStream - Optional. If specified, this is the buffer the compressed data will be written in.
		//    If NULL, this function allocates memory that the user needs to delete later.
		//  compressedStreamSize - Optional. Size of the buffer specified in compressedStream. Ignored if compressedStream
		//    is NULL. Note that this is for informational purposes only, the function will trash memory if the buffer is
		//    to small. Call GetCompressedDataSize() if you're not sure about the proper size.
		// RETURNS: the compressed data.  null on error.
		u8* CompressData( const char* type, u8* uncompressedData, u32 uncompressedSize, u32 &dataSize, u8 *compressedStream = NULL, u32 compressedStreamSize = 0 );

		// PURPOSE: Compresses the given data and adds the header.  This will not add an entry to the compression data list.
		//  The user is responsible for deleting the memory.
		// PARAMS:
		//  cData - sysCompressionData that has, at minimum, its DecompressedData buffer and DecompressedSize set.
		//  dataSize - return by reference the size of the buffer returned
		//  compressedStream - Optional. If specified, this is the buffer the compressed data will be written in.
		//    If NULL, this function allocates memory that the user needs to delete later.
		//  compressedStreamSize - Optional. Size of the buffer specified in compressedStream. Ignored if compressedStream
		//    is NULL. Note that this is for informational purposes only, the function will trash memory if the buffer is
		//    to small. Call GetCompressedDataSize() if you're not sure about the proper size.
		// RETURNS: the compressed data.  null on error.
		static u8* CompressData( sysCompressionData* cData, u32 &dataSize, u8 *compressedStream = NULL, u32 compressedStreamSize = 0 );

		// PURPOSE: Return the maximum number of bytes required for a buffer that will contain the compressed
		// data of a stream.
		//  type - which registered sysCompressionData to use.  What is returned by sysCompressionData::GetHeaderPrefix();
		//  uncompressedSize - number of bytes that are supposed to be compressed
		u32 GetCompressedDataSize(const char* type, u32 uncompressedSize) const;

	private:
		bool m_copyCompressedData;
		u8* m_readData;
		int m_readDataSize;
		int m_start;
		int m_end;
		atStringMap<sysCompressionData*> m_registeredTypes;
		atArray<sysCompressionData*> m_compressionData;
	};

	inline atArray<sysCompressionData*>& sysCompressionFactory::GetData() 
	{ 
		return m_compressionData; 
	}

	inline bool sysCompressionFactory::GetCopyCompressedData() const 
	{ 
		return m_copyCompressedData; 
	}

	inline void sysCompressionFactory::SetCopyCompressedData( bool b ) 
	{ 
		m_copyCompressedData = b; 
	}

} // namespace rage

#endif // SYSTEM_COMPRESSIONFACTORY_H 
