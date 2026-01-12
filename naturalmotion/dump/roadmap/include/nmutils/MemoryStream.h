#ifndef NM_MEMORY_STREAM_H
#define NM_MEMORY_STREAM_H

#include "nmmath/OldMath.h"
#include "nmutils/NMTypes.h"
#include "nmutils/NMCustomMemory.h"

#include <vector>

namespace NMutils
{

  class MemoryStreamWriter;
  class MemoryStreamReader;

  /**
   * \defgroup DataStreams Data Streams
   *
   * These classes and functions are used to create and manipulate
   * data serialisations. They are all defined in the NMutils namespace.
   */

  /**
   * \ingroup DataStreams
   * \brief A block of memory for streaming in and out of.
   *
   * %MemoryStream provides access to a block of memory that is
   * used by MemoryStreamWriter and MemoryStreamReader to allow
   * the serialisation of data. After data is written to the
   * %MemoryStream using a MemoryStreamWriter, the data in the
   * stream may be accessed with getDataBlock() and saved. A
   * new %MemoryStream can be constructed from this later.
   */
  class MemoryStream
  {
  public:
    friend class MemoryStreamWriter;
    friend class MemoryStreamRawWriter;
    friend class MemoryStreamReader;

    MemoryStream( bool isTypeChecked, const NMutils::MemoryConfiguration *customAlloc);
    MemoryStream( bool isTypeChecked, size_t initialSize, const NMutils::MemoryConfiguration *customAlloc);
    MemoryStream( const MemoryStream& other, const NMutils::MemoryConfiguration *customAlloc);
    
    static size_t getHeaderSize();

    /**
     * \brief Describes Data Ownership
     *
     * Specifies whether the datablock is owned by the MemoryStream
     * or not.
     */
    enum DataOwnershipMode
    {
      kCopyDataBlock,     /**< The source data will be copied */
      kReferenceDataBlock /**< The source data will be referenced */
    };
    MemoryStream( DataOwnershipMode mode, void *dataBlock, size_t dataSize, bool isTypeChecked, const NMutils::MemoryConfiguration *customAlloc);
    
    MemoryStream& operator=( const MemoryStream& other );
    virtual ~MemoryStream();    
    
    void    keepDataOnDelete(bool keepData);
    static void freeDataBlock( void* dataBlock );

    void*   getDataBlock() const;
    size_t  getAllocedSize() const;    
    bool    isTypeChecked() const;
    size_t  getUsedSize() const;
    bool    isReadOnly() const;

    bool    minimiseAllocatedSpace();

    bool    reAlloc(size_t newSize); // INTERNAL

    const NMutils::MemoryConfiguration* getMemoryConfig() { return &m_memory; }
    
    bool    setUsedSize(size_t usedSize);
    bool    forceSetUsedSize(size_t usedSize);  
    void*   getInsertionPoint();
  protected:
    bool    incrementUsedSize(size_t delta);    

    void*   getLock() const;
    void    setLock(void *lock);
    bool    setTypeCheckFlag(bool isTypeCheckFlag);
    bool    setExternalDataBlock(void* dataBlock, size_t dataLen, bool isTypeChecked);
    bool    setBookmarkCount(size_t num);
  private:
    
    NMutils::MemoryConfiguration  m_memory;

    void   *m_data;
    size_t  m_allocedSize;
    size_t  m_usedSize;
    bool    m_isTypeChecked;
    bool    m_freeDataOnDelete;
    void*   m_lock;
    bool    m_isReadOnly;
  };  

  /**
   * \ingroup DataStreams
   * \brief Writes data to a %MemoryStream 
   *
   * %MemoryStreamWriter writes data to a MemoryStream. It will resize the
   * allocated memory in the stream according to a user-selectable strategy.
   * If the MemoryStream has type-checking enabled, then %MemoryStreamWriter
   * embeds type data along with the actual data in the stream.
   */
  class MemoryStreamWriter
  {
  public:
    /**
     * \brief %MemoryStream reallocation strategies 
     *
     * The different strategies that may be used to resize the
     * MemoryStream when more space is required. See setReallocMode().
     */
    enum ReallocMode
    {
      kMinimal,       /**< Allocate exactly the extra space required. */
      kConservative,  /**< Allocate a fixed multiple of the extra space required. */
      kBlock,         /**< Allocate in fixed-size blocks */
      kGeometric      /**< Double the allocated space whenever any extra is required */
    };

    MemoryStreamWriter( MemoryStream* stream, ReallocMode reallocMode, size_t reallocParam = 0, bool writeAligned = false );
    MemoryStreamWriter( MemoryStream* stream, bool writeAligned = false );
    virtual ~MemoryStreamWriter();
    
    bool setReallocMode(ReallocMode mode, size_t param = 0);

    bool writeChar(char data);
    bool writeCharArray(const char* data, size_t length);
    bool writeCharArray(const char* data);
    bool writeWCharArray(const wchar_t* data);
    bool writeBool(bool data);
    bool writeBoolArray(bool* data, size_t length);
    bool writeInt(int data);
    bool writeIntArray(int* data, size_t length);
    bool writeUInt(unsigned int data);
    bool writeUIntArray(unsigned int* data, size_t length);
    bool writeUInt64(NMU64 data);
    bool writeUInt64Array(NMU64* data, size_t length);
    bool writeFloat(float data);
    bool writeFloatArray(float* data, size_t length);
    bool writeDouble(double data);
    bool writeDoubleArray(double* data, size_t length);
    bool writeVector3(NMVector3ConstPtr data);
    bool writeVector3Array(NMVector3* data, size_t length);
    bool writeVector4(NMVector4ConstPtr data);
    bool writeVector4Array(NMVector4* data, size_t length);
    bool writeMatrix3(NMMatrix3ConstPtr data);
    bool writeMatrix3Array(NMMatrix3* data, size_t length);
    bool writeMatrix4(NMMatrix4ConstPtr data);
    bool writeMatrix4Array(NMMatrix4* data, size_t length);
    bool writeMemoryStream(MemoryStream* data); 
    bool writeBookmark();

    bool isWriteAlignedEnabled() const { return m_writeAligned; }

    inline const MemoryStream* getMemoryStream() const { return m_stream; }

  protected:
    bool ensureSpaceAvailable(size_t requiredSpace);
    bool recoverBookmarks();

  private:
    MemoryStreamWriter( const MemoryStreamWriter& other );
    MemoryStreamWriter& operator=( const MemoryStreamWriter& other );

    bool          m_writeAligned;
    MemoryStream *m_stream;
    ReallocMode   m_reallocMode;
    size_t        m_reallocParam;
    std::vector<size_t> *m_bookmarks;
  };

  /**
   * \ingroup DataStreams
   * \brief Writes Raw Data to %MemoryStream
   *
   * %MemoryStreamRawWriter writes data directly into a MemoryStream. It does
   * not write type-check codes or bookmarks. Useful for piecewise rebuilding
   * of a stream. Note that the MemoryStream header must be written directly.
   *
   * IMPORTANT: The existing contents of the MemoryStream will be erased by using
   * this writer. It is essential that the MemoryStream header is written. 
   * Normally, this should only be used to re-write the serialised contents of
   * a MemoryStream already created with a MemoryStreamWriter.
   *
   * UPDATE:  Some circumstances require that the header is not written.  A bool param 
   * has been added to the header for these circumstances. 
   *
   * Because the RawWriter does not use type codes , you must specify if the bytes are to be 
   * swapped for a platform with different endianness in the constructor
   */
  class MemoryStreamRawWriter
  {
  public:
    MemoryStreamRawWriter( MemoryStream* stream, MemoryStreamWriter::ReallocMode reallocMode, size_t reallocParam = 0, bool writeHeaderOnDelete = true, bool byteSwap = false );
    MemoryStreamRawWriter( MemoryStream* stream, bool writeHeaderOnDelete = true, bool byteSwap = false );
    virtual ~MemoryStreamRawWriter();

    bool setReallocMode(MemoryStreamWriter::ReallocMode mode, size_t param = 0);

    bool writeData(void* data, size_t length);

    MemoryStream * getMemoryStream() {return m_stream;}
    bool writeChar(char data);
    bool writeCharArray(const char* data, size_t length); 
    bool writeBool(bool data); 
    bool writeBoolArray(bool* data, size_t length); 
    bool writeInt(int data); 
    bool writeIntArray(int* data, size_t length); 
    bool writeUInt(unsigned int data);
    bool writeUIntArray(unsigned int* data, size_t length); 
    bool writeUInt64(NMU64 data); 
    bool writeUInt64Array(NMU64* data, size_t length); 
    bool writeFloat(float data);
    bool writeFloatArray(float* data, size_t length);
    bool writeDouble(double data);
    bool writeDoubleArray(double* data, size_t length);
    bool writeVector3(NMVector3ConstPtr data);
    bool writeVector3Array(NMVector3* data, size_t length);
    bool writeVector4(NMVector4ConstPtr data);
    bool writeVector4Array(NMVector4* data, size_t length);
    bool writeMatrix3(NMMatrix3ConstPtr data);
    bool writeMatrix3Array(NMMatrix3* data, size_t length);
    bool writeMatrix4(NMMatrix4ConstPtr data); 
    bool writeMatrix4Array(NMMatrix4* data, size_t length);
    bool padToAlignment(unsigned int alignmentValue); 
  protected:
    bool ensureSpaceAvailable(size_t requiredSpace);
    bool m_writeHeaderOnDelete; 
    bool m_byteSwap; 
  private:
    MemoryStreamRawWriter( const MemoryStreamRawWriter& other );
    MemoryStreamRawWriter& operator=( const MemoryStreamRawWriter& other );
  
    MemoryStream *m_stream;
    MemoryStreamWriter::ReallocMode   m_reallocMode;
    size_t        m_reallocParam;

    
  };

  /**
   * \ingroup DataStreams
   * \brief Reads data from a %MemoryStream
   *
   * %MemoryStreamReader reads data from a MemoryStream that has been
   * written by a MemoryStreamWriter. If the data was written with
   * type-checking enabled, the reader is able to check the data types
   * as the data is read.
   */
  class MemoryStreamReader
  {
  public:
    MemoryStreamReader( MemoryStream* stream );
    virtual ~MemoryStreamReader();

    /**
     * \brief Result of read function.
     *
     * Indicates the status of a read operation.
     */
    enum ReadResult
    {
      kNoError = 0,       /**< Read successful */
      kEOFError,          /**< Reached end of data block */
      kDataTypeError,     /**< The type-check failed */
      kLockedStreamError, /**< The stream is locked */
      kArraySizeError     /**< The target array is too small */
    };

    ReadResult readChar(char* target);
    ReadResult readCharArray(char* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readCharArrayAsReference(char** target, size_t* arrayLength);
    ReadResult readWCharArrayAsReference(wchar_t** target, size_t* arrayLength);
    ReadResult readBool(bool* target);
    ReadResult readBoolArray(bool* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readBoolArrayAsReference(bool** target, size_t* arrayLength);
    ReadResult readInt(int* target);
    ReadResult readIntArray(int* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readIntArrayAsReference(int** target, size_t* arrayLength);
    ReadResult readUInt(unsigned int* target);
    ReadResult readUIntArray(unsigned int* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readUIntArrayAsReference(unsigned int** target, size_t* arrayLength);
    ReadResult readUInt64(NMU64* target);
    ReadResult readUInt64Array(NMU64* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readUInt64ArrayAsReference(NMU64** target, size_t* arrayLength);
    ReadResult readFloat(float* target);
    ReadResult readFloatArray(float* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readFloatArrayAsReference(float** target, size_t* arrayLength);
    ReadResult readDouble(double* target);
    ReadResult readDoubleArray(double* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readDoubleArrayAsReference(double** target, size_t* arrayLength);
    ReadResult readVector3(NMVector3* target);
    ReadResult readVector3Array(NMVector3* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readVector3ArrayAsReference(NMVector3** target, size_t* arrayLength);
    ReadResult readVector4(NMVector4* target);
    ReadResult readVector4Array(NMVector4* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readVector4ArrayAsReference(NMVector4** target, size_t* arrayLength);
    ReadResult readMatrix3(NMMatrix3* target);
    ReadResult readMatrix3Array(NMMatrix3* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readMatrix3ArrayAsReference(NMMatrix3** target, size_t* arrayLength);
    ReadResult readMatrix4(NMMatrix4* target);
    ReadResult readMatrix4Array(NMMatrix4* target, size_t allocedLength, size_t* arrayLength);
    ReadResult readMatrix4ArrayAsReference(NMMatrix4** target, size_t* arrayLength);
    ReadResult readMemoryStream(MemoryStream* target);
    ReadResult readMemoryStreamAsReference(MemoryStream* target);
    
    bool unread();
    bool jumpToPreviousBookmark();
    bool jumpToNextBookmark();

    inline const MemoryStream* getMemoryStream() const { return m_stream; }

    /**
     * \brief The data type 
     *
     * This enum is used by peekNextType().
     */
    enum DataType
    {
      kNotTypeCheckedStream,  /**< The stream is not type-checked, so type cannot be determined */
      kEOF,                   /**< There is no more data */
      kUnknownTypeError,      /**< The type code is unknown */
      kChar,          /**< Single char */
      kCharArray,     /**< Array of char */
      kBool,          /**< Single bool */
      kBoolArray,     /**< Array of bool */
      kInt,           /**< Single int */
      kIntArray,      /**< Array of int */
      kUInt,          /**< Single unsigned int */
      kUIntArray,     /**< Array of unsigned int */
      kFloat,         /**< Single float */
      kFloatArray,    /**< Array of float */
      kDouble,        /**< Single double */
      kDoubleArray,   /**< Array of double */
      kMemoryStream,  /**< Embedded MemoryStream */
      kUInt64,        /**< Single NMutils::NMU64 */
      kUInt64Array,   /**< Array of NMutils::NMU64 */
      kVector3,       /**< Single NMutils::NMVector3 */
      kVector3Array,  /**< Array of NMutils::NMVector3 */
      kVector4,       /**< Single NMutils::NMVector4 */
      kVector4Array,  /**< Array of NMutils::NMVector4 */
      kMatrix3,       /**< Single NMutils::NMMatrix3 */
      kMatrix3Array,  /**< Array of NMutils::NMMatrix3 */
      kMatrix4,       /**< Single NMutils::NMMatrix4 */
      kMatrix4Array   /**< Array of NMutils::NMMatrix4 */
    };

    DataType peekNextType();
    bool     peekNextArrayLength(size_t* arrayLength);

  protected:
    ReadResult doReadMemoryStream(MemoryStream* target, bool byReference);

  private:
    MemoryStreamReader( const MemoryStreamReader& other );
    MemoryStreamReader& operator=( const MemoryStreamReader& other );

    MemoryStream *m_stream;
    size_t m_currentPos;
    size_t m_unreadPos;
    bool   m_nextBookmarkWasLastMove;
    size_t m_lastBookmarkJumpedTo;
    bool   m_haveJumpedBack;
  };

  /**
   * endian-swap the given memory stream; requires type-checking to be enabled for this and any embedded streams.
   */
  void endianSwapMemoryStream(NMutils::MemoryStream &stream);

  /**
   * An optimised version of ADLER-32 CRC generation
   * Reference: http://en.wikipedia.org/wiki/Adler-32
   */
  unsigned int generateCRC(unsigned char* data, size_t datasize);
}

#endif // NM_MEMORY_STREAM_H
