// 
// system/compressiondata.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_COMPRESSIONDATA_H 
#define SYSTEM_COMPRESSIONDATA_H 

namespace rage {

// PURPOSE:	Interface class for a set of compressed and uncompressed data that is used
//  by sysCompressionFactory.  Mirrored in C# in rage/base/tools/ragCompression, this 
//	class serves as an interface between the game and Rag.  
//
//  Derived classes determine the header data and compression/decompression methods used.
//  GetHeaderPrefix() should be unique.  The derived class must be registered with its
//  Compression Factory so a stream of compressed data can be decompressed properly.
class sysCompressionData
{
public:
	sysCompressionData();
	virtual ~sysCompressionData();

	// PURPOSE: Creates a new instance of this class to be used by the sysCompressionFactory
	// RETURNS: pointer to new instance
	virtual sysCompressionData* CreateNewInstance() const = 0;

	// PURPOSE: Retrieves an estimate of the maximum length the of the compressed data with the given size
	// PARAMS:
	//  uncompressedSize - size of the data to be compressed
	// RETURNS: maximum length of compressedData
	virtual u32 GetCompressUpperBound( u32 uncompressedSize ) const = 0;

	// PURPOSE: Retrieves the size of this class's header data
	// RETURNS: number of bytes of header data
	virtual int GetHeaderSize() const = 0;

	// PURPOSE: Retrieves the unique header prefix string so the Compression Factory can find the start of a CompressedData set
	// RETURNS: header prefix string
	virtual const char* GetHeaderPrefix() const = 0;

	// PURPOSE: Builds the header section for the Compression Factory
	// PARAMS:
	//  buf - buffer to write to
	//  length - length of buffer to make sure we don't write past the end
	// RETURNS: length of header
	virtual int BuildHeader( u8* buf, int length ) const = 0;

	// PURPOSE: Assuming data starts with a valid header, retrieves the compressed data size and decompressed data size
	// PARAMS:
	//  data - compressed data, starting with the header prefix
	//  dataLength - length of compressedData
	// RETURNS: true if the header was parsed correctly
	virtual bool ParseHeader( u8* data, u32 dataLength ) = 0;

	// PURPOSE: Compresses the data into a buffer
	// PARAMS:
	//  compressedData - buffer to write the compressed data to
	//  compressedDataLength - length of the buffer
	//  uncompressedData - data that we want to compress
	//  uncompressedSize - number of bytes to be compressed
	// RETURNS: the number of bytes compressed to compressedData.  0 on error.
	virtual u32 Compress( u8* compressedData, u32 compressedDataLength, u8* uncompressedData, u32 uncompressedSize ) = 0;

	// PURPOSE: Decompresses the data into a buffer of the specified length
	// PARAMS:
	//  decompressedData - buffer to decompress the data to
	//  decompressedSize - number of bytes in the uncompressed stream (the original DecompressedSize)
	//  compressedData - data to be decompressed
	//  compressedSize - number of bytes to be decompressed
	// RETURNS: true on success
	virtual bool Decompress( u8* decompressedData, u32 decompressedSize, u8* compressedData, u32 compressedSize ) = 0;

	// PURPOSE: Indicates whether the CompressedData buffer is managed internally or not.
	// RETURNS: true if the user needs to cleanup the CompressedData buffer manually (i.e. call delete)
	bool CanDeleteCompresedData() const;

	// PURPOSE:  Retrieves the CompressedData buffer.  This could be NULL or "empty" until one of the Compress() 
	//  functions has been called, or SetCompressedData() is called.
	// RETURNS: pointer to CompressedData buffer
	u8* GetCompressedData() const;

	// PURPOSE: Retrieves the length of the CompressedData buffer.  Always >= GetCompressedSize().
	// RETURNS: length of CompressedData.  0 if CompressedData is NULL.
	u32 GetCompressedDataLength() const;

	// PURPOSE: Retrieves the size of the compressed data in CompressedData buffer.  Always <= GetCompressedDataLength().
	// RETURNS: size of compressed data.  0 if we haven't compressed the data yet.
	u32 GetCompressedSize() const;

	// PURPOSE: Sets the size of the data in CompressedData.  Useful when calling SetDecompressedData(...) followed by
	//  Compress(...).  
	void SetCompressedSize( u32 compressedSize );

	// PURPOSE: Indicates whether the DecompressedData buffer is managed internally or not.
	// RETURNS: true if the user needs to cleanup the DecompressedData buffer manually (i.e. call delete)
	bool CanDeleteDecompressedData() const;

	// PURPOSE:  Retrieves the DecompressedData buffer.  This could be NULL or "empty" until one of the Decompress() 
	//  functions has been called, or SetDecompressedData() is called.
	// RETURNS: pointer to DecompressedData buffer
	u8* GetDecompressedData() const;

	// PURPOSE: Retrieves the length of the DecompressedData buffer.  Always >= GetDecompressedSize().
	// RETURNS: length of DecompressedData.  0 if DecompressedData is NULL.
	u32 GetDecompressedDataLength() const;

	// PURPOSE: Retrieves the size of the decompressed data in DecompressedData buffer.  Always <= GetDecompressedDataLength().
	// RETURNS: size of decompressed data.  0 if we haven't decompressed the data yet.
	u32 GetDecompressedSize() const;

	// PURPOSE: Sets the decompressed size of the data in CompressionData.  Useful when calling SetCompressedData(...) followed by
	//  Decompress(...).  Not needed if ParseHeader(...) was called before Decompress(...)
	// PARAMS:
	//  uncompressedSize - size of uncompressed data
	void SetDecompressedSize( u32 uncompressedSize );

	// PURPOSE: Sets the CompressedData buffer. Such as before calling one of the Decompress functions.
	// PARAMS:
	//  compressedData - compressed data buffer
	//  compressedDataLength - length of compressedData
	//  compressedSize - number of bytes of compressed data.  0 if no data present.
	//  decompressedSize - number of bytes in the uncompressed data.  Must be non-zero if Decompress() is to be called next.
	//  copy - true will make a copy of the compressedData and its memory will be managed internally
	void SetCompressedData( u8* compressedData, u32 compressedDataLength, u32 compressedSize=0, bool copy=false );

	// PURPOSE: Sets the DecompressedData buffer. Such as before calling one of the Compress functions.
	// PARAMS:
	//  decompressedData - the data we will want to compress
	//  decompressedDataLength - length of decompressedData
	//  decompressedSize - number of bytes used.  0 if no data present.
	//  copy - true will make a copy of the decompressedData and its memory will be managed internally
	void SetDecompressedData( u8* decompressedData, u32 decompressedDataLength, u32 decompressedSize=0, bool copy=false );

	// PURPOSE: Compresses the data in DecompressedData, saving everything in our member variables
	// PARAMS:
	//  resize - true will resize CompressedData so that GetCompressedDataLength() == GetCompressedSize() after compressing the data,
	//    but only if !CanDeleteCompresedData()
	// RETURNS: true on success
	// NOTE: Can only be called if the Decompressed Size was set to a non-zero value
	bool Compress( bool resize=false );

	// PURPOSE: Compresses the data in DecompressedData, saving everything in our member variables
	// PARAMS:
	//  compressedSize - the number of bytes that was added to DecompressedData
	//  resize - true will resize CompressedData so that GetCompressedDataLength() == GetCompressedSize() after compressing the data,
	//    but only if !CanDeleteCompresedData()
	// RETURNS: true on success
	// NOTE: This needs to be called instead of Compress(bool) if the Decompressed Size was never set
	bool Compress( u32 uncompressedSize, bool resize=false );

	// PURPOSE: Decompresses the data in CompressedData, saving everything in our member variables
	// PARAMS:
	//  resize - true will resize DecompressedData so that GetDecompressedDataLength() == GetDecompressedSize() after compressing the data,
	//    but only if !CanDeleteDecompresedData()
	// RETURNS: true on success
	// NOTE: Can only be called if both DecompressedSize and CompressedSize was set manually or if ParseHeader(...) returned true.
	bool Decompress( bool resize=false );

	// PURPOSE: Decompresses the data in CompressedData, saving everything in our member variables
	// PARAMS:
	//  uncompressedSize - the size of the data before it was compressed
	//  resize - true will resize DecompressedData so that GetDecompressedDataLength() == GetDecompressedSize() after compressing the data,
	//    but only if !CanDeleteDecompresedData()
	// RETURNS: true on success
	// NOTE: Needs to be called if CompressedSize was set but not DecompressedSize, or ParseHeader(...) was not called.
	bool Decompress( u32 uncompressedSize, bool resize=false );

	// PURPOSE: Decompresses the data in CompressedData, saving everything in our member variables
	// PARAMS:
	//  compressedSize - the number of bytes that was added to CompressedData
	//  uncompressedSize - the size of the data before it was compressed
	//  resize - true will resize DecompressedData so that GetDecompressedDataLength() == GetDecompressedSize() after compressing the data,
	//    but only if !CanDeleteDecompresedData()
	// RETURNS: true on success
	// NOTE: Needs to be called if both CompressedSize and DecompressedSize were not set, or ParseHeader(...) was not called.
	bool Decompress( u32 compressedSize, u32 uncompressedSize, bool resize=false );

private:
	bool m_ownsCompressedData;
	u8* m_compressedData;
	u32 m_compressedDataLength;
	u32 m_compressedSize;

	bool m_ownsDecompressedData;
	u8* m_decompressedData;
	u32 m_decompressedDataLength;
	u32 m_decompressedSize;
};

inline bool sysCompressionData::CanDeleteCompresedData() const
{
	return !m_ownsCompressedData;
}

inline u8* sysCompressionData::GetCompressedData() const 
{ 
	return m_compressedData; 
}

inline u32 sysCompressionData::GetCompressedDataLength() const 
{ 
	return m_compressedDataLength; 
}

inline u32 sysCompressionData::GetCompressedSize() const 
{ 
	return m_compressedSize; 
}

inline void sysCompressionData::SetCompressedSize( u32 compressedSize ) 
{ 
	m_compressedSize = compressedSize; 
}

inline bool sysCompressionData::CanDeleteDecompressedData() const 
{ 
	return !m_ownsDecompressedData; 
}

inline u8* sysCompressionData::GetDecompressedData() const 
{ 
	return m_decompressedData; 
}

inline u32 sysCompressionData::GetDecompressedDataLength() const 
{ 
	return m_decompressedDataLength; 
}

inline u32 sysCompressionData::GetDecompressedSize() const 
{ 
	return m_decompressedSize; 
}

inline void sysCompressionData::SetDecompressedSize( u32 uncompressedSize ) 
{ 
	m_decompressedSize = uncompressedSize; 
}

inline bool sysCompressionData::Compress( u32 uncompressedSize, bool resize ) 
{ 
	m_decompressedSize = uncompressedSize; 
	return Compress( resize ); 
}

inline bool sysCompressionData::Decompress( u32 uncompressedSize, bool resize ) 
{ 
	m_decompressedSize = uncompressedSize; 
	return Decompress( resize ); 
}

inline bool sysCompressionData::Decompress( u32 compressedSize, u32 uncompressedSize, bool resize ) 
{ 
	m_compressedSize = compressedSize; 
	m_decompressedSize = uncompressedSize; 
	return Decompress( resize ); 
}

} // namespace rage

#endif // SYSTEM_COMPRESSIONDATA_H 
