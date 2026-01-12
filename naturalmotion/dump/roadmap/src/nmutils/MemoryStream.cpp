#include "nmutils/MemoryStream.h"
#include "nmutils/TypeUtils.h"

#ifdef NM_PLATFORM_WIN32
#include <crtdbg.h>
#endif // NM_PLATFORM_WIN32

#if defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_WII)
# include <wchar.h>
# include <string.h> // for memcpy
#endif // NM_PLATFORM_CELL_PPU || NM_PLATFORM_WII


#define DEFAULT_INITIAL_ALLOC_SIZE 4096
#define INVALID_UNREAD_POS 0xffffffff

typedef char TypeCode;

namespace
{
  const size_t StreamHeaderSize = sizeof(unsigned int)*2;
  const TypeCode TypeCode_char = 1;
  const TypeCode TypeCode_bool = 2;
  const TypeCode TypeCode_int = 3;
  const TypeCode TypeCode_unsigned_int = 4;
  const TypeCode TypeCode_float = 5;
  const TypeCode TypeCode_double = 6;
  const TypeCode TypeCode_Array_char = 7;
  const TypeCode TypeCode_Array_bool = 8;
  const TypeCode TypeCode_Array_int = 9;
  const TypeCode TypeCode_Array_unsigned_int = 10;
  const TypeCode TypeCode_Array_float = 11;
  const TypeCode TypeCode_Array_double = 12;
  const TypeCode TypeCode_MemoryStream = 13;
  const TypeCode TypeCode_NMU64 = 14;
  const TypeCode TypeCode_Array_NMU64 = 15;
  const TypeCode TypeCode_NMVector3 = 16;
  const TypeCode TypeCode_Array_NMVector3 = 17;
  const TypeCode TypeCode_NMVector4 = 18;
  const TypeCode TypeCode_Array_NMVector4 = 19;
  const TypeCode TypeCode_NMMatrix3 = 20;
  const TypeCode TypeCode_Array_NMMatrix3 = 21;
  const TypeCode TypeCode_NMMatrix4 = 22;
  const TypeCode TypeCode_Array_NMMatrix4 = 23;
  // these match the type codes defined above
  enum MemoryStreamTypeCodes
  {
    _kChar = TypeCode_char,
    _kBool,
    _kInt,
    _kUInt,
    _kFloat,
    _kDouble,
    _kArrayChar,
    _kArrayBool,
    _kArrayInt,
    _kArrayUInt,
    _kArrayFloat,
    _kArrayDouble,
    _kMemoryStream,
    _kNMU64,
    _kArrayNMU64,
    _kVector3,
    _kArrayVector3,
    _kVector4,
    _kArrayVector4,
    _kMatrix3,
    _kArrayMatrix3,
    _kMatrix4,
    _kArrayMatrix4,
    _numberOfKnownTypecodes
  };
  bool MS_ensureSpaceAvailable(NMutils::MemoryStream* stream, NMutils::MemoryStreamWriter::ReallocMode reallocMode, size_t reallocParam, size_t requiredSpace)
  {
    size_t availableSpace = stream->getAllocedSize() - stream->getUsedSize();
    if(availableSpace < requiredSpace)
    {
      size_t extraSpaceRequired = requiredSpace - availableSpace;
      switch(reallocMode)
      {
      case NMutils::MemoryStreamWriter::kMinimal:
        return stream->reAlloc(stream->getAllocedSize() + extraSpaceRequired);
      case NMutils::MemoryStreamWriter::kConservative:
        return stream->reAlloc(stream->getAllocedSize() + (extraSpaceRequired * reallocParam) );
      case NMutils::MemoryStreamWriter::kBlock:
        {
          size_t blocksRequired = (extraSpaceRequired / reallocParam) + (unsigned int)(extraSpaceRequired%reallocParam != 0);
          return stream->reAlloc(stream->getAllocedSize() + (blocksRequired * reallocParam));
        }
      case NMutils::MemoryStreamWriter::kGeometric:
        {
          size_t allocedSpace = stream->getAllocedSize();
          size_t usedSpace    = stream->getUsedSize();
          while(allocedSpace < requiredSpace+usedSpace)
            allocedSpace *= 2;
          return stream->reAlloc( allocedSpace );
        }
      default:
        AssertMsg(0, "We shouldn't be here!");
        return false;
      }
    }
    // If we get here, we have enough space.
    return true;
  }
}

namespace NMutils
{
#define SET_USEDSIZE_HEADER {*(unsigned int*)m_data = (unsigned)m_usedSize;}
#define SET_BOOKMARK_HEADER(num) {*(((unsigned int*)m_data)+1) = (unsigned)num;}
size_t endianSwapKnownArrayType(unsigned char typecode, unsigned char* data, size_t arrayCount); 
size_t endianSwapKnownTypecode(unsigned char typecode, unsigned char* data);
  /**
   * Returns the byte size of the header that must be at the beginning
   * of a MemoryStream. 
   */
  size_t MemoryStream::getHeaderSize()
  {
    return StreamHeaderSize;
  }

  /**
   * Construct an empty memory stream with default initial size.
   *\n If <tt>isTypeChecked</tt> is <tt>true</tt>, then the readers and writers
   * will add type information into the data stream which may be used for
   * error checking and introspection.
   *\n The logger is optional, and will be used by the readers and writers.
   */
  MemoryStream::MemoryStream( bool isTypeChecked, const NMutils::MemoryConfiguration *customAlloc)
    : m_memory(*customAlloc), m_allocedSize(DEFAULT_INITIAL_ALLOC_SIZE), m_usedSize(StreamHeaderSize), 
      m_isTypeChecked(isTypeChecked), m_freeDataOnDelete(true),
      m_lock(0), m_isReadOnly(false)
  {
#if 0
    if ((customAlloc && !m_freeDataOnDelete))
    {
      Warningf("MemoryStream:: using custom allocators with manually-managed memory free'ing flags - this will leak!");
    }
#endif 
    FastAssert(customAlloc != 0);

    // We require enough space to write the block size and bookmark count
    FastAssert( m_allocedSize >= StreamHeaderSize );
    m_data = m_memory.m_allocator(m_allocedSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    SET_USEDSIZE_HEADER;
    SET_BOOKMARK_HEADER(0);
  }

  /**
   * Construct an empty memory stream with specified initial size. If the initial
   * size is less than the required header size (see getHeaderSize()), then it
   * is set to be equal to the header size.
   *\n If <tt>isTypeChecked</tt> is <tt>true</tt>, then the readers and writers
   * will add type information into the data stream which may be used for
   * error checking and introspection.
   *\n The logger is optional, and will be used by the readers and writers.
   */
  MemoryStream::MemoryStream( bool isTypeChecked, size_t initialSize, const NMutils::MemoryConfiguration *customAlloc)
    : m_memory(*customAlloc), m_allocedSize(initialSize), m_usedSize(StreamHeaderSize), m_isTypeChecked(isTypeChecked), m_freeDataOnDelete(true),
      m_lock(0), m_isReadOnly(false)
  {
#if 0    
    if ((customAlloc && !m_freeDataOnDelete))
    {
      Warningf("MemoryStream:: using custom allocators with manually-managed memory free'ing flags - this will leak!");
    }
#endif
    FastAssert(customAlloc != 0);

    if(m_allocedSize < StreamHeaderSize )
      m_allocedSize = StreamHeaderSize;
    m_data = m_memory.m_allocator(m_allocedSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    SET_USEDSIZE_HEADER;
    SET_BOOKMARK_HEADER(0);
  }

  /**
   * Construct a memory stream using a preexisting datablock. 
   *\n The block must be valid MemoryStream data, and so be at least as
   * large as the required header (see getHeaderSize()).
   *\n If <tt>mode</tt> is <tt>kCopyDataBlock</tt>, then the <tt>dataBlock</tt>
   * is copied. If <tt>mode</tt> is <tt>kReferenceDataBlock</tt> then the 
   * <tt>dataBlock</tt> is used directly and the %MemoryStream cannot be written to.
   *\n If <tt>isTypeChecked</tt> is <tt>true</tt>, it is assumed that the data
   * in the block contains type checking codes.
   *\n The logger is optional, and will be used by the readers and writers.
   */
  MemoryStream::MemoryStream( DataOwnershipMode mode, void *dataBlock, size_t dataSize, bool isTypeChecked, const NMutils::MemoryConfiguration *customAlloc)
    : m_memory(*customAlloc), m_allocedSize(dataSize), m_usedSize(dataSize), m_isTypeChecked(isTypeChecked),
      m_lock(0)
  {
#if 0
    if ((customAlloc && !m_freeDataOnDelete))
    {
      Warningf("MemoryStream:: using custom allocators with manually-managed memory free'ing flags - this will leak!");
    }
#endif
    FastAssert(customAlloc != 0);

    FastAssert(dataSize >= StreamHeaderSize);
    if(mode == kCopyDataBlock)
    {
      m_data = m_memory.m_allocator(m_allocedSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
      memcpy(m_data, dataBlock, m_allocedSize);
      m_freeDataOnDelete = true;
      m_isReadOnly = false;
    }
    else if(mode == kReferenceDataBlock)
    {
      m_data = dataBlock;
      m_freeDataOnDelete = false;
      m_isReadOnly = true;
    }
    else { AssertMsg(0, "Unknown Mode" ); }
  }

  /**
   * Construct a copy of a %MemoryStream. 
   */
  MemoryStream::MemoryStream( const MemoryStream& other , const NMutils::MemoryConfiguration *customAlloc)
    : m_memory(*customAlloc), m_allocedSize(other.m_allocedSize), m_usedSize(other.m_usedSize), m_isTypeChecked(other.m_isTypeChecked), m_freeDataOnDelete(true),
      m_lock(0), m_isReadOnly(false)
  {
#if 0
    if ((customAlloc && !m_freeDataOnDelete))
    {
      Warningf("MemoryStream:: using custom allocators with manually-managed memory free'ing flags - this will leak!");
    }
#endif
    FastAssert(customAlloc != 0);

    m_data = m_memory.m_allocator(m_allocedSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    memcpy(m_data, other.m_data, m_allocedSize);
  }

  /**
   * Assign %MemoryStream to another. Note that the logger is not assigned. 
   */
  MemoryStream& MemoryStream::operator=( const MemoryStream& other )
  {
    if(&other == this)
      return *this;
    
    if(m_lock)
    {
      Errorf("MemoryStream:: Cannot assign to a locked MemoryStream.");
      return *this;
    }

    m_memory.m_deallocator(m_data, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    m_allocedSize = other.m_allocedSize;
    m_data = m_memory.m_allocator(m_allocedSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    memcpy(m_data, other.m_data, m_allocedSize);
    m_usedSize = other.m_usedSize;
    m_isTypeChecked = other.m_isTypeChecked;
    m_isReadOnly = false;
    // NB: Keep the same logger.

    return *this;
  }

  /**
   * Destroy a MemoryStream. The data block will be freed unless keepDataOnDelete() is
   * called prior to the destructor.
   */
  MemoryStream::~MemoryStream()
  {
    if(m_lock != 0)
      Errorf("MemoryStream:: Deleting locked MemoryStream.");
    if(m_freeDataOnDelete && !m_isReadOnly)
      m_memory.m_deallocator(m_data, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
  }

  /**
   * Normally, the data-block will be destroyed when the %MemoryStream is 
   * deleted. However, this function allows the data to be kept. The pointer
   * to the data block (returned by getDataBlock()) must be stored immediately
   * prior to destroying the %MemoryStream object. The data is then owned by
   * the caller and must be freed with freeDataBlock() when no longer required.
   */
  void MemoryStream::keepDataOnDelete(bool keepData)
  {
    if(!m_isReadOnly)
      m_freeDataOnDelete = !keepData;
    else
      Errorf("MemoryStream:: Cannot delete external dataBlock.");
  }

  // static
  /**
   * This must be used to free data-blocks that have been kept after 
   * MemoryStreams have been deleted. See keepDataOnDelete().
   */
  void MemoryStream::freeDataBlock(void* datablock)
  {
    free(datablock);
  }

  /**
   * \internal
   * Reallocates the data block. Fails if <tt>newSize</tt> is less than
   * the used size.
   */
  bool MemoryStream::reAlloc(size_t newSize)
  {
    if(newSize < m_usedSize)
    {
      Warningf("MemoryStream:: reAlloc called with smaller size than used data size.");
      return false;
    }

    if(m_allocedSize == newSize)
    {
      return true;
    }

    m_data = m_memory.m_reallocator(m_data, newSize, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);
    m_allocedSize = newSize;
    return true;
  }

  /**
   * Returns a pointer to the data-block in the memory stream.  
   */
  void* MemoryStream::getDataBlock() const
  {
    return m_data;
  }

  /**
   * Returns the allocated size of the data-block.
   */
  size_t MemoryStream::getAllocedSize() const
  {
    return m_allocedSize;
  }

  /**
   * Returns the size of the valid data within the data-block. 
   */
  size_t MemoryStream::getUsedSize() const
  {
    return m_usedSize;
  }

  /**
   * \internal
   * Returns a pointer to the first unused byte in the datablock. 
   * This function does no checking, and may return an invalid
   * value if there is no free space.
   */
  void* MemoryStream::getInsertionPoint()
  {
    return ((char*)m_data) + m_usedSize;
  }

  /**
   * Returns <tt>true</tt> if type checking is enabled. 
   */
  bool MemoryStream::isTypeChecked() const
  {
    return m_isTypeChecked;
  }

  /**
   * Returns <tt>true</tt> if the stream's data-block is external,
   * and so is flagged read-only.
   */
  bool MemoryStream::isReadOnly() const
  {
    return m_isReadOnly;
  }

  /**
   * \internal
   * Sets the number of bytes in the data-block that are considered
   * to be valid data. 
   * \n Returns <tt>false</tt> and does nothing (except logging an error) if 
   * the used size would be greater than the allocated size or smaller than
   * the stream header size;
   */
  bool MemoryStream::setUsedSize(size_t usedSize)
  {
    if(usedSize > m_allocedSize)
    {
      Errorf("MemoryStream:: Attempt to set the used size to be greater than the allocated size.");
      return false;
    }
    if(usedSize < StreamHeaderSize)
    {
      Errorf("MemoryStream:: Attempt to set the used size to be smaller than the stream header size.");
      return false;
    }
    m_usedSize = usedSize;
    SET_USEDSIZE_HEADER;
    return true;
  }

  /**
   * \internal
   * Sets the used size member. Does not set header value.
   */
  bool MemoryStream::forceSetUsedSize(size_t usedSize)
  {
    if(usedSize > m_allocedSize)
    {
      Errorf("MemoryStream:: Attempt to set the used size to be greater than the allocated size.");
      return false;
    }
    m_usedSize = usedSize;
    return true;
  }


  /**
   * \internal
   * Increments the number of bytes in the data-block that are considered
   * to be valid data.
   * \n Returns <tt>false</tt> and does nothing (except logging an error) if 
   * the used size would be greater than the allocated size.
   */
  bool MemoryStream::incrementUsedSize(size_t delta)
  {
    if(m_usedSize + delta > m_allocedSize)
    {
      Errorf("MemoryStream:: Attempt to increment the used size to be greater than the allocated size.");
      return false;
    }
    m_usedSize += delta;
    SET_USEDSIZE_HEADER;
    return true;
  }

  /**
   * \internal
   * Returns the current lock, or 0 if the stream is not locked. 
   */
  void* MemoryStream::getLock() const
  {
    return m_lock;
  }

  /**
   * \internal
   * Locks or unlocks the stream.
   */
  void MemoryStream::setLock(void *lock)
  {
    m_lock = lock;
  }

  /**
   * \internal
   * Sets the state of the type-check flag. This can only be called
   * when the usedSize is zero, because it changes how the data block
   * is interpreted.
   */
  bool MemoryStream::setTypeCheckFlag(bool isTypeCheckFlag)
  {
    if(m_usedSize > StreamHeaderSize)
    {
      Errorf("MemoryStream:: Cannot set typeCheck flag when stream contains data.");
      return false;
    }
    m_isTypeChecked = isTypeCheckFlag;
    return true;
  }

  /**
   * \internal
   * Sets the data-block pointer to the external source, marks the block
   * as keep-on-delete and read-only. This can only be called when the
   * usedSize is zero. Frees the current data-block unless flagged otherwise.
   */
  bool MemoryStream::setExternalDataBlock(void* dataBlock, size_t dataLen, bool isTypeChecked)
  {
    if(m_usedSize > StreamHeaderSize)
    {
      Errorf("MemoryStream:: Cannot set data-block source when stream contains data.");
      return false;
    }
    
    if(m_freeDataOnDelete)
      m_memory.m_deallocator(m_data, m_memory.m_userData, NM_MEMORY_TRACKING_ARGS);

    m_isTypeChecked = isTypeChecked;
    m_data = dataBlock;
    m_usedSize = dataLen;
    m_allocedSize = dataLen;
    m_freeDataOnDelete = false;
    m_isReadOnly = true;
    SET_USEDSIZE_HEADER;
    SET_BOOKMARK_HEADER(0);
    
    return true;
  }

  /**
   * Resizes the datablock to exactly the used space. Returns <tt>true</tt>
   * if successful. If this function fails, the data may be damaged.
   */
  bool MemoryStream::minimiseAllocatedSpace()
  {
    return reAlloc(m_usedSize);
  }

  /**
   * \internal
   * Sets the header to the number of bookmarks in the stream. Fails and
   * returns <tt>false</tt> if the stream could not contain the specified
   * number of bookmarks.
   */
  bool MemoryStream::setBookmarkCount(size_t num)
  {
    if(m_usedSize < StreamHeaderSize + (sizeof(unsigned int)*num) )
    {
      Errorf("MemoryStream:: Attempt to specify more bookmarks than can fit in stream.");
      return false;
    }
    SET_BOOKMARK_HEADER(num);
    return true;
  }

#define GET_USEDSIZE_HEADER  (*(unsigned int*)m_stream->getDataBlock())
#define GET_BOOKMARK_HEADER  (*(((unsigned int*)m_stream->getDataBlock())+1))
#define GET_BOOKMARKS_SIZE   (sizeof(unsigned int)*GET_BOOKMARK_HEADER)
#define GET_BOOKMARKS_OFFSET (m_stream->getUsedSize() - GET_BOOKMARKS_SIZE)

  /**
   * Constructs a new %MemoryStreamWriter that will write to the specified
   * MemoryStream. This will lock the stream, so that it cannot be used
   * with other MemoryStreamWriters or Readers until this object is deleted.
   *\n This uses the default allocation strategy of <tt>kBlock,4096</tt>.
   */
  MemoryStreamWriter::MemoryStreamWriter(MemoryStream* stream, bool writeAligned /*=false*/)
    : m_writeAligned(writeAligned), m_stream(stream), m_reallocMode(kGeometric), m_reallocParam(4096)
  {
    m_bookmarks = new std::vector<size_t>;
    if(m_stream->getLock() != 0)
    {
      Errorf("MemoryStreamWriter:: MemoryStream is locked in ctor");
    }
    else
    {
      m_stream->setLock(this);
      recoverBookmarks();
    }
  }

  /**
   * Constructs a new %MemoryStreamWriter that will write to the specified
   * MemoryStream. This will lock the stream, so that it cannot be used
   * with other MemoryStreamWriters or Readers until this object is deleted.
   * See setReallocMode() for the meaning of the <tt>reallocMode</tt> and
   * <tt>reallocParam</tt> arguments.
   */
  MemoryStreamWriter::MemoryStreamWriter(MemoryStream* stream, ReallocMode reallocMode, size_t reallocParam /*=0*/, bool writeAligned /*=false*/ )
    : m_writeAligned(writeAligned), m_stream(stream), m_reallocMode(reallocMode), m_reallocParam(reallocParam)
  {
    m_bookmarks = new std::vector<size_t>;
    if(m_stream->getLock() != 0)
    {
      Errorf("MemoryStreamWriter:: MemoryStream is locked in ctor");
    }
    else
    {
      m_stream->setLock(this);
      recoverBookmarks();
    }
  }

  /**
   * Destroys the %MemoryStreamWriter. This unlocks the MemoryStream so that
   * it can be used with other readers and writers. It also minimises the allocated
   * space in the stream and writes the bookmarks segment.
   */
  MemoryStreamWriter::~MemoryStreamWriter()
  {
    size_t bookMarksSize = m_bookmarks->size() * sizeof(unsigned int);
    if(bookMarksSize > 0)
    {
      if(!ensureSpaceAvailable(bookMarksSize))
      {
        Errorf("MemoryStreamWriter:: Failed to allocate space for bookmarks. Bookmarks will not be written.");
        m_stream->setBookmarkCount(0);
      }
      else
      {
        memcpy(m_stream->getInsertionPoint(), &(*m_bookmarks)[0], bookMarksSize );
        m_stream->incrementUsedSize(bookMarksSize);
        m_stream->setBookmarkCount(m_bookmarks->size());
      }
    }
    else
    {
      m_stream->setBookmarkCount(0);
    }
    
    m_stream->minimiseAllocatedSpace();
    if(m_stream->getLock() == this)
      m_stream->setLock(0);
    
    delete m_bookmarks;
  }

  /**
   * \internal
   * The MemoryStreamWriter cannot be copied.
   */
  MemoryStreamWriter::MemoryStreamWriter(const MemoryStreamWriter& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamWriter cannot be copied.");
  }

  /**
   * \internal
   * The MemoryStreamWriter cannot be copied.
   */
  MemoryStreamWriter& MemoryStreamWriter::operator=(const MemoryStreamWriter& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamWriter cannot be copied.");
    return *this;
  }

  /**
   * Sets the reallocation strategy to be used when more space is required
   * in the MemoryStream. 
   * \n A non-zero <tt>param</tt> is required for modes kConservative and kBlock
   * as it specifies the multiple and block-size respectively.
   * \n Returns <tt>true</tt> if parameters are correct.
   */
  bool MemoryStreamWriter::setReallocMode(ReallocMode mode, size_t param /*=0*/)
  {
    if( (mode == kConservative || mode == kBlock) && param < 1 )
    {
      Errorf("MemoryStreamWriter:: Non-zero param required to setReallocMode().");
      return false;
    }

    m_reallocMode = mode;
    m_reallocParam = param;
    return true;
  }

  /**
   * \internal
   *
   * Attempts to ensure that at least <tt>requiredSpace</tt> bytes are
   * available (between the allocated and used sizes) in the MemoryStream.
   * This will reallocate when necessary, according to the current reallocation
   * mode.
   * \n Returns <tt>false</tt> if reallocation fails.
   */
  bool MemoryStreamWriter::ensureSpaceAvailable(size_t requiredSpace)
  {
    return MS_ensureSpaceAvailable(m_stream, m_reallocMode, m_reallocParam, requiredSpace);
  }

  /**
   * \internal
   * Read bookmarks from the stream and store them in local vector.
   */
  bool MemoryStreamWriter::recoverBookmarks()
  {
    if(m_stream->m_usedSize <= StreamHeaderSize + GET_BOOKMARKS_SIZE)
      return false;

    m_bookmarks->clear();
    unsigned int numBookmarks = GET_BOOKMARK_HEADER;
    if(numBookmarks > 0)
    {      
      unsigned int* bookmarks = (unsigned int*)(((char*)m_stream->getDataBlock()) + GET_BOOKMARKS_OFFSET);
      for(unsigned int i = 0; i < numBookmarks; ++i)
      {
        m_bookmarks->push_back(bookmarks[i]);
      }
      m_stream->setUsedSize(m_stream->getUsedSize()-numBookmarks*sizeof(unsigned int));
      m_stream->setBookmarkCount(0);
    }
    return true;
  }

  /**
   * Adds a bookmark to the stream so that a MemoryStreamReader will be
   * able to jump to this insertion point. That is, the reader will move
   * in the stream such that the next read will read the data written by
   * the write that follows this %writeBookmark() call. If no data is
   * written after the bookmark, then the reader will jump to EOF.
   * \n Returns <tt>true</tt> if the bookmarking succeeds. You cannot
   * bookmark the same location twice.
   */
  bool MemoryStreamWriter::writeBookmark()
  {
    if(m_bookmarks->size() > 0 && m_bookmarks->back() == m_stream->getUsedSize())
    {
      Warningf("MemoryStreamWriter:: Cannot bookmark the same location twice.");
      return false;
    }
    m_bookmarks->push_back(m_stream->getUsedSize());
    return true;
  }

#define FAIL_IF_LOCKED_OR_READONLY \
  if(m_stream->getLock() != this) \
  { \
    Errorf("MemoryStreamWriter:: Cannot write to locked stream."); \
    return false; \
  } \
  if(m_stream->isReadOnly()) \
  { \
    Errorf("MemoryStreamWriter:: Cannot write to read-only stream."); \
    return false; \
  }

#define ENSURE_SPACE_OR_FAIL(type) \
  if(!ensureSpaceAvailable(m_stream->isTypeChecked()? sizeof(type) + sizeof(TypeCode) : sizeof(type))) \
  { \
    Errorf("MemoryStreamWriter:: Unable to allocate require space."); \
    return false; \
  }

#define ENSURE_SPACE_ARRAY_OR_FAIL(type, arraylength) \
  if(!ensureSpaceAvailable(m_stream->isTypeChecked()? (sizeof(type)*arraylength) + sizeof(unsigned int) + sizeof(TypeCode) : (sizeof(type)*arraylength) + sizeof(unsigned int) )) \
  { \
    Errorf("MemoryStreamWriter:: Unable to allocate require space."); \
    return false; \
  }

/**
 * Note: typecodes written into write-aligned streams will have their upper bit
 * set so that the (unaligned) streams are backwards compatible and alignment does
 * not incur a format change
 */
#define WRITE_TYPECHECK_CODE(code) \
  if(m_stream->isTypeChecked()) \
  { \
    char codeToWrite = (char)TypeCode_ ##code;\
    if (isWriteAlignedEnabled())\
      codeToWrite |= 0x80;\
    *((char*)m_stream->getInsertionPoint()) = codeToWrite; \
    m_stream->incrementUsedSize(sizeof(TypeCode)); \
  } \

#define WRITE_TYPECHECK_ARRAY_CODE(code) \
  if(m_stream->isTypeChecked()) \
  { \
    char codeToWrite = (char)TypeCode_Array_ ##code;\
    if (isWriteAlignedEnabled())\
      codeToWrite |= 0x80;\
    *((char*)m_stream->getInsertionPoint()) = codeToWrite; \
    m_stream->incrementUsedSize(sizeof(TypeCode)); \
  } \

#define WRITE_SINGLE_VALUE(type, val) \
  { *((type*)m_stream->getInsertionPoint()) = val; \
  m_stream->incrementUsedSize(sizeof(type)); }

#define WRITE_ARRAY(type, length, val) \
  {*((unsigned int*)m_stream->getInsertionPoint()) = (unsigned)length; \
  m_stream->incrementUsedSize(sizeof(unsigned int)); \
  memcpy(m_stream->getInsertionPoint(), val, sizeof(type)*length); \
  m_stream->incrementUsedSize(sizeof(type)*length);}

/**
 * the ENSURE_ALIGNMENT macro will, if aligned writing is enabled, pad the
 * current stream position out to the aligned boundry specified by the size of
 * the passed type - eg. floats will be aligned to the nearest 4-byte boundary,
 * NMVector4s will be aligned to 128bits, and so on. In debug, the padding will
 * be filled with 0xEC for easy identification in a hex editor, etc for debugging
 * purposes; in release, it will be zeros so that it compresses better.
 */

#ifdef _DEBUG
  #define ALIGNMENT_PADDING_VALUE   (0xEC)
#else
  #define ALIGNMENT_PADDING_VALUE   (0x00)
#endif

#define ENSURE_ALIGNMENT(type) \
  {\
    if (isWriteAlignedEnabled() && (sizeof(type) > 1)) \
    {\
      int _remains = (m_stream->getUsedSize() % 4);\
      if (_remains > 0)\
      {\
        int _padSpace = 4 - _remains;\
        if(!ensureSpaceAvailable(_padSpace + sizeof(type))) \
        { \
          Errorf("MemoryStreamWriter:: Unable to allocate require space for alignment padding."); \
          return false; \
        }\
        memset(m_stream->getInsertionPoint(), ALIGNMENT_PADDING_VALUE, _padSpace); \
        m_stream->incrementUsedSize(_padSpace);\
      }\
    }\
  }\

#define ENSURE_ALIGNMENT_ARRAY(type, arraylength) \
  {\
    if (isWriteAlignedEnabled() && (sizeof(type) > 1)) \
    {\
      int _remains = (m_stream->getUsedSize() % 4);\
      if (_remains > 0)\
      {\
        int _padSpace = 4 - _remains;\
        if(!ensureSpaceAvailable(_padSpace + sizeof(int) + (sizeof(type) * arraylength))) \
        { \
          Errorf("MemoryStreamWriter:: Unable to allocate require space for alignment padding."); \
          return false; \
        }\
        memset(m_stream->getInsertionPoint(), ALIGNMENT_PADDING_VALUE, _padSpace); \
        m_stream->incrementUsedSize(_padSpace);\
      }\
    }\
  }\

  /**
   * Writes a single <tt>char</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeChar(char data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(char);
    WRITE_TYPECHECK_CODE(char);
    ENSURE_ALIGNMENT(char);
    WRITE_SINGLE_VALUE(char, data);
    return true;
  }

  /**
   * Writes a single <tt>bool</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeBool(bool data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(bool);
    WRITE_TYPECHECK_CODE(bool);
    ENSURE_ALIGNMENT(bool);
    WRITE_SINGLE_VALUE(bool, data);
    return true;
  }

  /**
   * Writes a single <tt>int</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeInt(int data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(int);
    WRITE_TYPECHECK_CODE(int);
    ENSURE_ALIGNMENT(int);
    WRITE_SINGLE_VALUE(int, data);
    return true;
  }

  /**
   * Writes a single <tt>unsigned int</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeUInt(unsigned int data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(unsigned int);
    WRITE_TYPECHECK_CODE(unsigned_int);
    ENSURE_ALIGNMENT(unsigned int);
    WRITE_SINGLE_VALUE(unsigned int, data);
    return true;
  }

  /**
   * Writes a single <tt>NMutils::NMU64</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeUInt64(NMutils::NMU64 data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(NMU64);
    WRITE_TYPECHECK_CODE(NMU64);
    ENSURE_ALIGNMENT(NMU64);
    WRITE_SINGLE_VALUE(NMU64, data);
    return true;
  }

  /**
   * Writes a single <tt>float</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeFloat(float data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(float);
    WRITE_TYPECHECK_CODE(float);
    ENSURE_ALIGNMENT(float);
    WRITE_SINGLE_VALUE(float, data);
    return true;
  }

  /**
   * Writes a single <tt>double</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeDouble(double data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(double);
    WRITE_TYPECHECK_CODE(double);
    ENSURE_ALIGNMENT(double);
    WRITE_SINGLE_VALUE(double, data);
    return true;
  }

  /**
   * Writes an array of <tt>char</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   *\n Note that because no endianness conversion or striding will be 
   * performed with arrays of <tt>char</tt>s, %writeCharArray() can be used
   * to store arbitrary blocks of data in a MemoryStream.
   */
  bool MemoryStreamWriter::writeCharArray(const char* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(char, length);
    WRITE_TYPECHECK_ARRAY_CODE(char);
    ENSURE_ALIGNMENT_ARRAY(char, length);
    WRITE_ARRAY(char, length, data);
    return true;
  }

  /**
   * Writes string until the null terminator.
   */
  bool MemoryStreamWriter::writeCharArray(const char* data)
  {
    return writeCharArray(data, strlen(data) + 1);
  }

  /**
   * Writes wide character string until the null terminator.
   */
  bool MemoryStreamWriter::writeWCharArray(const wchar_t* data)
  {
    return writeCharArray((char*)data, (wcslen(data) + 1) * sizeof(wchar_t));
  }

  /**
   * Writes an array of <tt>bool</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeBoolArray(bool* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(bool, length);
    WRITE_TYPECHECK_ARRAY_CODE(bool);
    ENSURE_ALIGNMENT_ARRAY(bool, length);
    WRITE_ARRAY(bool, length, data);
    return true;
  }

  /**
   * Writes an array of <tt>int</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeIntArray(int* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(int, length);
    WRITE_TYPECHECK_ARRAY_CODE(int);
    ENSURE_ALIGNMENT_ARRAY(int, length);
    WRITE_ARRAY(int, length, data);
    return true;
  }

  /**
   * Writes an array of <tt>unsigned int</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeUIntArray(unsigned int* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(unsigned int, length);
    WRITE_TYPECHECK_ARRAY_CODE(unsigned_int);
    ENSURE_ALIGNMENT_ARRAY(unsigned int, length);
    WRITE_ARRAY(unsigned int, length, data);
    return true;
  }

  /**
   * Writes an array of <tt>NMutils::NMU64</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeUInt64Array(NMutils::NMU64* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(NMU64, length);
    WRITE_TYPECHECK_ARRAY_CODE(NMU64);
    ENSURE_ALIGNMENT_ARRAY(NMU64, length);
    WRITE_ARRAY(NMU64, length, data);
    return true;
  }

  /**
   * Writes an array of <tt>float</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeFloatArray(float* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(float, length);
    WRITE_TYPECHECK_ARRAY_CODE(float);
    ENSURE_ALIGNMENT_ARRAY(float, length);
    WRITE_ARRAY(float, length, data);
    return true;
  }

  /**
   * Writes an array of <tt>double</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeDoubleArray(double* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(double, length);
    WRITE_TYPECHECK_ARRAY_CODE(double);
    ENSURE_ALIGNMENT_ARRAY(double, length);
    WRITE_ARRAY(double, length, data);
    return true;
  }

#define WRITE_SINGLE_VALUE_WITH_MEMCPY(type, val) \
  { memcpy(m_stream->getInsertionPoint(), val, sizeof(type)); \
  m_stream->incrementUsedSize(sizeof(type)); }

  /**
   * Writes a single <tt>NMutils::NMVector3</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeVector3(NMutils::NMVector3ConstPtr data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(NMVector3);
    WRITE_TYPECHECK_CODE(NMVector3);
    ENSURE_ALIGNMENT(NMVector3);
    WRITE_SINGLE_VALUE_WITH_MEMCPY(NMVector3, data);
    return true;
  }
  
  /**
   * Writes an array of <tt>NMutils::NMVector3</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeVector3Array(NMutils::NMVector3* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(NMVector3, length);
    WRITE_TYPECHECK_ARRAY_CODE(NMVector3);
    ENSURE_ALIGNMENT_ARRAY(NMVector3, length);
    WRITE_ARRAY(NMVector3, length, data);
    return true;
  }

  /**
   * Writes a single <tt>NMutils::NMVector4</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeVector4(NMutils::NMVector4ConstPtr data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(NMVector4);
    WRITE_TYPECHECK_CODE(NMVector4);
    ENSURE_ALIGNMENT(NMVector4);
    WRITE_SINGLE_VALUE_WITH_MEMCPY(NMVector4, data);
    return true;
  }

  /**
   * Writes an array of <tt>NMutils::NMVector4</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeVector4Array(NMutils::NMVector4* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(NMVector4, length);
    WRITE_TYPECHECK_ARRAY_CODE(NMVector4);
    ENSURE_ALIGNMENT_ARRAY(NMVector4, length);
    WRITE_ARRAY(NMVector4, length, data);
    return true;
  }

  /**
   * Writes a single <tt>NMutils::NMMatrix3</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeMatrix3(NMutils::NMMatrix3ConstPtr data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(NMMatrix3);
    WRITE_TYPECHECK_CODE(NMMatrix3);
    ENSURE_ALIGNMENT(NMMatrix3);
    WRITE_SINGLE_VALUE_WITH_MEMCPY(NMMatrix3, data);
    return true;
  }

  /**
   * Writes an array of <tt>NMutils::NMMatrix3</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeMatrix3Array(NMutils::NMMatrix3* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(NMMatrix3, length);
    WRITE_TYPECHECK_ARRAY_CODE(NMMatrix3);
    ENSURE_ALIGNMENT_ARRAY(NMMatrix3, length);
    WRITE_ARRAY(NMMatrix3, length, data);
    return true;
  }

  /**
   * Writes a single <tt>NMutils::NMMatrix4</tt> to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeMatrix4(NMutils::NMMatrix4ConstPtr data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_OR_FAIL(NMMatrix4);
    WRITE_TYPECHECK_CODE(NMMatrix4);
    ENSURE_ALIGNMENT(NMMatrix4);
    WRITE_SINGLE_VALUE_WITH_MEMCPY(NMMatrix4, data);
    return true;
  }

  /**
   * Writes an array of <tt>NMutils::NMMatrix4</tt>s of specified length to the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeMatrix4Array(NMutils::NMMatrix4* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    ENSURE_SPACE_ARRAY_OR_FAIL(NMMatrix4, length);
    WRITE_TYPECHECK_ARRAY_CODE(NMMatrix4);
    ENSURE_ALIGNMENT_ARRAY(NMMatrix4, length);
    WRITE_ARRAY(NMMatrix4, length, data);
    return true;
  }

  /**
   * Writes the contents of another memory stream into the stream.
   *\n Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamWriter::writeMemoryStream(MemoryStream* data)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    size_t spaceRequired = data->getUsedSize() + sizeof(unsigned int) + sizeof(bool);
    if(m_stream->isTypeChecked())
      spaceRequired += sizeof(TypeCode);

    if(!ensureSpaceAvailable(spaceRequired))
    {
      Errorf("MemoryStreamWriter:: Unable to allocate required space.");
      return false;
    }

    WRITE_TYPECHECK_CODE(MemoryStream);
    ENSURE_ALIGNMENT(NMVector4); // err hack, just align to regular boundary

    if(!ensureSpaceAvailable(spaceRequired))
    {
      Errorf("MemoryStreamWriter:: Unable to allocate required space.");
      return false;
    }

    *((unsigned int*)m_stream->getInsertionPoint()) = (unsigned)data->getUsedSize();
    m_stream->incrementUsedSize(sizeof(unsigned int));
    *((bool*)m_stream->getInsertionPoint()) = data->isTypeChecked();
    m_stream->incrementUsedSize(sizeof(bool));
    memcpy(m_stream->getInsertionPoint(), data->getDataBlock(), data->getUsedSize());
    m_stream->incrementUsedSize(data->getUsedSize());
    return true;
  }

  /**
   * Constructs a new %MemoryStreamRawWriter that will write to the specified
   * MemoryStream. This will lock the stream, so that it cannot be used
   * with other MemoryStreamWriters or Readers until this object is deleted.
   *\n This uses the default allocation strategy of <tt>kBlock,4096</tt>.
   */
  #define RAWMEMORYSTREAM_ENDIANSWAP(_typeCode, rawData) endianSwapKnownTypecode(_typeCode, (unsigned char *)(void *) rawData)
  #define RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_arrayCode, rawData, arrayEntries) endianSwapKnownArrayType(_arrayCode, (unsigned char *)(void *) rawData, arrayEntries)
 
  MemoryStreamRawWriter::MemoryStreamRawWriter(MemoryStream* stream, bool writeHeaderOnDelete, bool byteSwap)
    : m_writeHeaderOnDelete(writeHeaderOnDelete), m_byteSwap(byteSwap), m_stream(stream), m_reallocMode(MemoryStreamWriter::kBlock), m_reallocParam(4096)
  {
    if(m_stream->getLock() != 0)
    {
      Errorf("MemoryStreamRawWriter:: MemoryStream is locked in ctor");
    }
    else
    {
      m_stream->setLock(this);
      m_stream->forceSetUsedSize(0);
    }
  }

  /**
   * Constructs a new %MemoryStreamRawWriter that will write to the specified
   * MemoryStream. This will lock the stream, so that it cannot be used
   * with other MemoryStreamWriters or Readers until this object is deleted.
   * See setReallocMode() for the meaning of the <tt>reallocMode</tt> and
   * <tt>reallocParam</tt> arguments.
   */
  MemoryStreamRawWriter::MemoryStreamRawWriter(MemoryStream* stream, MemoryStreamWriter::ReallocMode reallocMode, size_t reallocParam /*=0*/, bool writeHeaderOnDelete, bool byteSwap )
    : m_writeHeaderOnDelete(writeHeaderOnDelete), m_byteSwap(byteSwap), m_stream(stream), m_reallocMode(reallocMode), m_reallocParam(reallocParam)
  {
    if(m_stream->getLock() != 0)
    {
      Errorf("MemoryStreamRawWriter:: MemoryStream is locked in ctor");
    }
    else
    {
      m_stream->setLock(this);
      m_stream->forceSetUsedSize(0);
    }
  }

  /**
   * Destroys the %MemoryStreamRawWriter. This unlocks the MemoryStream so that
   * it can be used with other readers and writers. It also minimises the allocated
   * space in the stream.
   */
  MemoryStreamRawWriter::~MemoryStreamRawWriter()
  {
    // TODO: fix me.  This has the side effect of writing the used size at byte 0 of the 
    // stream. The docs state that the raw memory stream does not write the header.  
    // Because some features might depend on this behavior I am not changing it for now
    if(m_writeHeaderOnDelete)
    {
      m_stream->setUsedSize(m_stream->getUsedSize());
    }
    m_stream->minimiseAllocatedSpace();
    if(m_stream->getLock() == this)
      m_stream->setLock(0);
  }

  /**
   * \internal
   * The MemoryStreamRawWriter cannot be copied.
   */
  MemoryStreamRawWriter::MemoryStreamRawWriter(const MemoryStreamRawWriter& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamRawWriter cannot be copied.");
  }

  /**
   * \internal
   * The MemoryStreamRawWriter cannot be copied.
   */
  MemoryStreamRawWriter& MemoryStreamRawWriter::operator=(const MemoryStreamRawWriter& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamRawWriter cannot be copied.");
    return *this;
  }

  /**
   * Sets the reallocation strategy to be used when more space is required
   * in the MemoryStream. 
   * \n A non-zero <tt>param</tt> is required for modes kConservative and kBlock
   * as it specifies the multiple and block-size respectively.
   * \n Returns <tt>true</tt> if parameters are correct.
   */
  bool MemoryStreamRawWriter::setReallocMode(MemoryStreamWriter::ReallocMode mode, size_t param /*=0*/)
  {
    if( (mode == MemoryStreamWriter::kConservative || mode == MemoryStreamWriter::kBlock) && param < 1 )
    {
      Errorf("MemoryStreamRawWriter:: Non-zero param required to setReallocMode().");
      return false;
    }

    m_reallocMode = mode;
    m_reallocParam = param;
    return true;
  }

  bool MemoryStreamRawWriter::writeChar(char data) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kChar, &data);
    }
    return writeData(&data, sizeof(char)); 
  }
  
  bool MemoryStreamRawWriter::writeCharArray(const char* data, size_t length) 
  { 
    // char arrays dont need to be swapped
    return writeData((void*)data, sizeof(char) * length); 
  }

  bool MemoryStreamRawWriter::writeBool(bool data) 
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kBool, &data);
    }
    return writeData(&data, sizeof(bool)); 
  }

  bool MemoryStreamRawWriter::writeBoolArray(bool* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayBool, data, length); 
    }
    return writeData(data, sizeof(bool) * length); 
  }

  bool MemoryStreamRawWriter::writeInt(int data) 
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kInt, &data);
    }
    return writeData(&data, sizeof(int)); 
  }

  bool MemoryStreamRawWriter::writeIntArray(int* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayInt, data, length); 
    }
    return writeData(data, sizeof(int) * length);
  }

  bool MemoryStreamRawWriter::writeUInt(unsigned int data) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kUInt, &data); 
    }
    return writeData(&data, sizeof(unsigned int));
  }

  bool MemoryStreamRawWriter::writeUIntArray(unsigned int* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayUInt, data, length); 
    }
    return writeData(data, sizeof(unsigned int) * length); 
  }

  bool MemoryStreamRawWriter::writeUInt64(NMU64 data) 
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kNMU64, &data); 
    }
    return writeData(&data, sizeof(NMU64)); 
  }

  bool MemoryStreamRawWriter::writeUInt64Array(NMU64* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayNMU64, data, length); 
    }
    return writeData(data, sizeof(NMU64) * length); 
  }

  bool MemoryStreamRawWriter::writeFloat(float data)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kFloat, &data); 
    }
    return writeData(&data, sizeof(float)); 
  }

  bool MemoryStreamRawWriter::writeFloatArray(float* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayFloat, data, length); 
    }
    return writeData(data, sizeof(float) * length);
  }

  bool MemoryStreamRawWriter::writeDouble(double data) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kDouble, &data); 
    }
    return writeData(&data, sizeof(double)); 
  }

  bool MemoryStreamRawWriter::writeDoubleArray(double* data, size_t length) 
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayDouble, data, length); 
    }
    return writeData(data, sizeof(double) * length); 
  }

  bool MemoryStreamRawWriter::writeVector3(NMVector3ConstPtr data) 
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kVector3, data); 
    }
    return writeData(&data, sizeof(NMVector3)); 
  }

  bool MemoryStreamRawWriter::writeVector3Array(NMVector3* data, size_t length)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayVector3, data, length); 
    }
    return writeData(data, sizeof(NMVector3) * length); 
  }

  bool MemoryStreamRawWriter::writeVector4(NMVector4ConstPtr data)
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kVector4, data); 
    }
    return writeData(&data, sizeof(NMVector4));
  }

  bool MemoryStreamRawWriter::writeVector4Array(NMVector4* data, size_t length)
  {
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayVector4, data, length); 
    }
    return writeData(data, sizeof(NMVector4) * length); 
  }

  bool MemoryStreamRawWriter::writeMatrix3(NMMatrix3ConstPtr data)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kMatrix3, data); 
    }
    return writeData(&data, sizeof(NMMatrix3)); 
  }

  bool MemoryStreamRawWriter::writeMatrix3Array(NMMatrix3* data, size_t length)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayMatrix3, data, length); 
    }
    return writeData(data, sizeof(NMMatrix3) * length); 
  }

  bool MemoryStreamRawWriter::writeMatrix4(NMMatrix4ConstPtr data)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP(_kMatrix4, data); 
    }
    return writeData(&data, sizeof(NMMatrix4)); 
  }

  bool MemoryStreamRawWriter::writeMatrix4Array(NMMatrix4* data, size_t length)
  { 
    if(m_byteSwap)
    {
      RAWMEMORYSTREAM_ENDIANSWAP_ARRAY(_kArrayMatrix4, data, length); 
    }
    return writeData(data, sizeof(NMMatrix4) * length); 
  }

  /**
   * \internal
   *
   * Attempts to ensure that at least <tt>requiredSpace</tt> bytes are
   * available (between the allocated and used sizes) in the MemoryStream.
   * This will reallocate when necessary, according to the current reallocation
   * mode.
   * \n Returns <tt>false</tt> if reallocation fails.
   */
  bool MemoryStreamRawWriter::ensureSpaceAvailable(size_t requiredSpace)
  {
    return MS_ensureSpaceAvailable(m_stream, m_reallocMode, m_reallocParam, requiredSpace);
  }

  /**
   * Appends the data to the stream. Returns <tt>true</tt> if successful.
   */
  bool MemoryStreamRawWriter::writeData(void* data, size_t length)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    if(!ensureSpaceAvailable(length))
    {
      Errorf("MemoryStreamRawWriter:: Unable to allocate required space.");
      return false;
    }    
    memcpy(m_stream->getInsertionPoint(), data, length);
    m_stream->forceSetUsedSize(m_stream->getUsedSize() + length);
    return true;
  }

  bool MemoryStreamRawWriter::padToAlignment(unsigned int alignmentValue)
  {
    FAIL_IF_LOCKED_OR_READONLY;
    if (alignmentValue> 1) 
    {
      int _remains = (m_stream->getUsedSize() % alignmentValue);
      if (_remains > 0)
      {
        int _padSpace = alignmentValue - _remains;
        if(!ensureSpaceAvailable(_padSpace + alignmentValue)) 
        { 
          Errorf("MemoryStreamRawWriter:: Unable to allocate require space for alignment padding."); 
          return false; 
        }
        memset(m_stream->getInsertionPoint(), ALIGNMENT_PADDING_VALUE, _padSpace); 
        m_stream->forceSetUsedSize(m_stream->getUsedSize() + _padSpace);
      }
    }
    return true; 
  }

  /**
   * Constructs a new %MemoryStreamReader that will read from the specified
   * MemoryStream. This will lock the stream, so that it cannot be used
   * with other MemoryStreamReaders or Writers until this object is deleted.
   */
  MemoryStreamReader::MemoryStreamReader(MemoryStream* stream)
    : m_stream(stream), m_currentPos(0), m_unreadPos(INVALID_UNREAD_POS), 
    m_nextBookmarkWasLastMove(false), m_lastBookmarkJumpedTo(0), m_haveJumpedBack(false)
  {
    if(m_stream->getLock() != 0)
    {
      Errorf("MemoryStreamReader:: MemoryStream is locked in ctor");
    }
    else
    {
      m_stream->setLock(this);
      FastAssert( m_stream->getUsedSize() == GET_USEDSIZE_HEADER );
      m_currentPos = StreamHeaderSize;
    }
  }

  /**
   * Destroys the %MemoryStreamReader. This unlocks the MemoryStream so that
   * it can be used with other readers and writers.
   */
  MemoryStreamReader::~MemoryStreamReader()
  {
    if(m_stream->getLock() == this)
      m_stream->setLock(0);
  }

  /**
   * \internal
   * The MemoryStreamReader cannot be copied.
   */
  MemoryStreamReader::MemoryStreamReader(const MemoryStreamReader& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamReader cannot be copied.");
  }

  /**
   * \internal
   * The MemoryStreamReader cannot be copied.
   */
  MemoryStreamReader& MemoryStreamReader::operator=(const MemoryStreamReader& other)
  {
    ((void)other);
    AssertMsg(0, "MemoryStreamReader cannot be copied.");
    return *this;
  }

#define FAIL_IF_LOCKED \
  if(m_stream->getLock() != this) \
  { \
    Errorf("MemoryStreamReader:: Cannot read from locked stream."); \
    return kLockedStreamError; \
  }

#define FAIL_IF_EOF(type) \
  if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos) < (m_stream->isTypeChecked()?sizeof(type)+sizeof(TypeCode):sizeof(type)) ) \
  { \
    Errorf("MemoryStreamReader:: Reached EOF in read."); \
    return kEOFError; \
  }

#define FAIL_IF_ARRAY_HEADER_EOF \
  if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos) < (m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int)) ) \
  { \
    Errorf("MemoryStreamReader:: Reached EOF in read for array header."); \
    return kEOFError; \
  }

#define FAIL_IF_BAD_TYPECHECK(typecode, type) \
  if(m_stream->isTypeChecked()) \
  { \
    char rawCode = *(((char*)m_stream->getDataBlock()) + m_currentPos); \
    if (rawCode & 0x80)\
    {\
      SKIP_ALIGNMENT_PADDING(type)\
    }\
    TypeCode code = (TypeCode)(rawCode & 0x7F);\
    if(code != TypeCode_ ##typecode) \
    { \
      Errorf("MemoryStreamReader:: Type check failed on read. Expected %d, got %d.", ( TypeCode_ ##typecode ), code); \
      return kDataTypeError; \
    } \
  }

#define SKIP_ALIGNMENT_PADDING(type) \
  {\
    if (sizeof(type) > 1) \
    {\
      ptrdiff_t _targetPos = m_currentPos + sizeof(TypeCode);\
      ptrdiff_t _remains = (_targetPos % 4);\
      if (_remains > 0)\
      {\
        m_currentPos += (4 - _remains);\
      }\
    }\
  }\

#define FAIL_IF_BAD_ARRAY_TYPECHECK(typecode, type) \
  if(m_stream->isTypeChecked()) \
  { \
    char rawCode = *(((char*)m_stream->getDataBlock()) + m_currentPos); \
    if (rawCode & 0x80)\
    {\
      SKIP_ALIGNMENT_PADDING(type)\
    }\
    TypeCode code = (TypeCode)(rawCode & 0x7F);\
    if(code != TypeCode_Array_ ##typecode) \
    { \
      Errorf("MemoryStreamReader:: Type check failed on array read. Expected %d, got %d.", ( TypeCode_Array_ ##typecode ), code); \
      return kDataTypeError; \
    } \
  }

#define READ_SINGLE_VALUE(target, type) \
  { if(target) \
      *target = *(type*)(((char*)m_stream->getDataBlock()) +(m_stream->isTypeChecked()?m_currentPos+sizeof(TypeCode):m_currentPos) ); \
    m_unreadPos = m_currentPos; \
    m_nextBookmarkWasLastMove = false; \
    m_currentPos += m_stream->isTypeChecked()?sizeof(type)+sizeof(TypeCode):sizeof(type); }

#define READ_ARRAY(target, arraylength, allocedlength, type) \
  {size_t storedLength = *(unsigned int*)(((char*)m_stream->getDataBlock())+(m_stream->isTypeChecked()?m_currentPos+sizeof(TypeCode):m_currentPos)); \
   if(storedLength > allocedlength) \
   { \
     Errorf("MemoryStreamReader:: Not enough space to store array. Requires %d elements.", storedLength); \
     return kArraySizeError; \
   } \
   if((m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos-(m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int))) < (storedLength * sizeof(type)) ) \
   { \
     Errorf("MemoryStreamReader:: EOF encountered in array read."); \
     return kEOFError; \
   } \
   m_unreadPos = m_currentPos; \
   m_currentPos += (m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int)); \
   if(arraylength) \
     *arraylength = storedLength; \
   if(target) \
     memcpy(target, ((char*)m_stream->getDataBlock())+m_currentPos, sizeof(type)*storedLength ); \
   m_nextBookmarkWasLastMove = false; \
   m_currentPos += sizeof(type) * storedLength; }

#define READ_ARRAY_REF(target, arrayLength, type) \
  { size_t storedLength = *(unsigned int*)(((char*)m_stream->getDataBlock())+(m_stream->isTypeChecked()?m_currentPos+sizeof(TypeCode):m_currentPos)); \
    if( (m_stream->getUsedSize() - GET_BOOKMARKS_SIZE - m_currentPos - (m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int))) < storedLength*sizeof(type) ) \
    { \
      Errorf("MemoryStreamReader:: EOF encountered in array read."); \
      return kEOFError; \
    } \
    m_unreadPos = m_currentPos; \
    m_currentPos += (m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int)); \
    if(arrayLength) \
      *arrayLength = storedLength; \
    if(target) \
      *target = (type*)(((char*)m_stream->getDataBlock())+m_currentPos); \
    m_currentPos += storedLength * sizeof(type); \
    m_nextBookmarkWasLastMove = false; }

  /**
   * Reads a <tt>char</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readChar(char* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(char);
    FAIL_IF_BAD_TYPECHECK(char, char);
    READ_SINGLE_VALUE(target, char);
    return kNoError;
  }

  /**
   * Reads a <tt>bool</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readBool(bool* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(bool);
    FAIL_IF_BAD_TYPECHECK(bool, bool);
    READ_SINGLE_VALUE(target, bool);
    return kNoError;
  }

  /**
   * Reads an <tt>int</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readInt(int* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(int);
    FAIL_IF_BAD_TYPECHECK(int, int);
    READ_SINGLE_VALUE(target, int);
    return kNoError;
  }

  /**
   * Reads an <tt>unsigned int</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUInt(unsigned int* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(unsigned int);
    FAIL_IF_BAD_TYPECHECK(unsigned_int, unsigned int);
    READ_SINGLE_VALUE(target, unsigned int);
    return kNoError;
  }

  /**
   * Reads a <tt>NMutils::NMU64</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUInt64(NMutils::NMU64* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(NMU64);
    FAIL_IF_BAD_TYPECHECK(NMU64, NMU64);
    READ_SINGLE_VALUE(target, NMU64);
    return kNoError;
  }

  /**
   * Reads a <tt>float</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readFloat(float* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(float);
    FAIL_IF_BAD_TYPECHECK(float, float);
    READ_SINGLE_VALUE(target, float);
    return kNoError;
  }

  /**
   * Reads a <tt>double</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readDouble(double* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(double);
    FAIL_IF_BAD_TYPECHECK(double, double);
    READ_SINGLE_VALUE(target, double);
    return kNoError;
  }

  /**
   * Reads an array of <tt>char</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readCharArray(char* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(char, char);
    READ_ARRAY(target, arrayLength, allocedLength, char);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>bool</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readBoolArray(bool* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(bool, bool);
    READ_ARRAY(target, arrayLength, allocedLength, bool);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>int</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readIntArray(int* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(int, int);
    READ_ARRAY(target, arrayLength, allocedLength, int);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>unsigned int</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUIntArray(unsigned int* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(unsigned_int, unsigned int);
    READ_ARRAY(target, arrayLength, allocedLength, unsigned int);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMU64</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUInt64Array(NMU64* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMU64, NMU64);
    READ_ARRAY(target, arrayLength, allocedLength, NMU64);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>float</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readFloatArray(float* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(float, float);
    READ_ARRAY(target, arrayLength, allocedLength, float);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>double</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readDoubleArray(double* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(double, double);
    READ_ARRAY(target, arrayLength, allocedLength, double);    
    return kNoError;
  }

  /**
   * Reads an array of <tt>char</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readCharArrayAsReference(char** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(char, char);
    READ_ARRAY_REF(target, arrayLength, char);  
    return kNoError;
  }

  MemoryStreamReader::ReadResult MemoryStreamReader::readWCharArrayAsReference(wchar_t** target, size_t* arrayLength)
  {
    return readCharArrayAsReference((char**)target, arrayLength);
  }

  /**
   * Reads an array of <tt>bool</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readBoolArrayAsReference(bool** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(bool, bool);
    READ_ARRAY_REF(target, arrayLength, bool);  
    return kNoError;
  }

  /**
   * Reads an array of <tt>int</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readIntArrayAsReference(int** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(int, int);
    READ_ARRAY_REF(target, arrayLength, int);  
    return kNoError;
  }

  /**
   * Reads an array of <tt>unsigned int</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUIntArrayAsReference(unsigned int** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(unsigned_int, unsigned int);
    READ_ARRAY_REF(target, arrayLength, unsigned int);  
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMU64</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readUInt64ArrayAsReference(NMutils::NMU64** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMU64, NMU64);
    READ_ARRAY_REF(target, arrayLength, NMU64);  
    return kNoError;
  }

  /**
   * Reads an array of <tt>float</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readFloatArrayAsReference(float** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(float, float);
    READ_ARRAY_REF(target, arrayLength, float);  
    return kNoError;
  }

  /**
   * Reads an array of <tt>double</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readDoubleArrayAsReference(double** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(double, double);
    READ_ARRAY_REF(target, arrayLength, double);  
    return kNoError;
  }

#define READ_SINGLE_VALUE_WITH_MEMCPY(target, type) \
  { if(target) \
      memcpy(target, ((char*)m_stream->getDataBlock()) +(m_stream->isTypeChecked()?m_currentPos+sizeof(TypeCode):m_currentPos), sizeof(type)); \
    m_unreadPos = m_currentPos; \
    m_nextBookmarkWasLastMove = false; \
    m_currentPos += m_stream->isTypeChecked()?sizeof(type)+sizeof(TypeCode):sizeof(type); }

  /**
   * Reads a <tt>NMutils::NMVector3</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector3(NMutils::NMVector3* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(NMVector3);
    FAIL_IF_BAD_TYPECHECK(NMVector3, NMVector3);
    READ_SINGLE_VALUE_WITH_MEMCPY(target, NMVector3);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMVector3</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector3Array(NMutils::NMVector3* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMVector3, NMVector3);
    READ_ARRAY(target, arrayLength, allocedLength, NMVector3);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMVector3</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector3ArrayAsReference(NMutils::NMVector3** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMVector3, NMVector3);
    READ_ARRAY_REF(target, arrayLength, NMVector3);  
    return kNoError;
  }

  /**
   * Reads a <tt>NMutils::NMVector4</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector4(NMutils::NMVector4* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(NMVector4);
    FAIL_IF_BAD_TYPECHECK(NMVector4, NMVector4);
    READ_SINGLE_VALUE_WITH_MEMCPY(target, NMVector4);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMVector4</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector4Array(NMutils::NMVector4* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMVector4, NMVector4);
    READ_ARRAY(target, arrayLength, allocedLength, NMVector4);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMVector4</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readVector4ArrayAsReference(NMutils::NMVector4** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMVector4, NMVector4);
    READ_ARRAY_REF(target, arrayLength, NMVector4);  
    return kNoError;
  }

  /**
   * Reads a <tt>NMutils::NMMatrix3</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix3(NMutils::NMMatrix3* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(NMMatrix3);
    FAIL_IF_BAD_TYPECHECK(NMMatrix3, NMMatrix3);
    READ_SINGLE_VALUE_WITH_MEMCPY(target, NMMatrix3);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMMatrix3</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix3Array(NMutils::NMMatrix3* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMMatrix3, NMMatrix3);
    READ_ARRAY(target, arrayLength, allocedLength, NMMatrix3);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMMatrix3</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix3ArrayAsReference(NMutils::NMMatrix3** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMMatrix3, NMMatrix3);
    READ_ARRAY_REF(target, arrayLength, NMMatrix3);  
    return kNoError;
  }

  /**
   * Reads a <tt>NMutils::NMMatrix4</tt> from the stream. If <tt>target</tt> is <tt>NULL</tt>
   * then the read is still performed, and the value discarded.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix4(NMutils::NMMatrix4* target)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_EOF(NMMatrix4);
    FAIL_IF_BAD_TYPECHECK(NMMatrix4, NMMatrix4);
    READ_SINGLE_VALUE_WITH_MEMCPY(target, NMMatrix4);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMMatrix4</tt> from the stream and stores in <tt>target</tt>.
   * <tt>allocedLength</tt> is the length of the <tt>target</tt> array. The number
   * of elements read into target is stored in <tt>arrayLength</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix4Array(NMutils::NMMatrix4* target, size_t allocedLength, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMMatrix4, NMMatrix4);
    READ_ARRAY(target, arrayLength, allocedLength, NMMatrix4);
    return kNoError;
  }

  /**
   * Reads an array of <tt>NMutils::NMMatrix4</tt> from the stream and, if successful, sets <tt>*target</tt>
   * to point to it, and <tt>*arrayLength</tt> to be its length. The array must be
   * treated as read-only. As the data is never copied, no reference to the array should
   * be maintained after the %MemoryStream is destroyed or written to. If the read fails,
   * a value other than <tt>kNoError</tt> is returned, and <tt>target</tt> and <tt>arrayLength</tt> 
   * are unmodified. Either argument may be <tt>NULL</tt>, and the read will still be performed.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMatrix4ArrayAsReference(NMutils::NMMatrix4** target, size_t* arrayLength)
  {
    FAIL_IF_LOCKED;
    FAIL_IF_ARRAY_HEADER_EOF;
    FAIL_IF_BAD_ARRAY_TYPECHECK(NMMatrix4, NMMatrix4);
    READ_ARRAY_REF(target, arrayLength, NMMatrix4);  
    return kNoError;
  }

  /**
   * Reads an embedded MemoryStream object from the stream and stores in <tt>target</tt>.
   * The existing contents of <tt>target</tt> will be destroyed. If <tt>target</tt> is
   * NULL, the read will still be performed, but the data discarded. If sufficient memory
   * cannot be allocated in <tt>target</tt>, then the function will return <tt>kArraySizeError</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMemoryStream(MemoryStream* target)
  {
    return doReadMemoryStream(target, false);
  }

  /**
   * Reads an embedded MemoryStream object from the stream and makes it available in <tt>target</tt>,
   * as read-only. The data is never copied, and so <tt>target</tt> must be destroyed before
   * the source stream. Further, the source stream should not be written to until <tt>target</tt>
   * has been destroyed.
   * \n The existing contents of <tt>target</tt> will be destroyed. If <tt>target</tt> is
   * NULL, the read will still be performed, but the data discarded. 
   * \n Errors encountered in setting the <tt>target</tt> will be reported as <tt>kArraySizeError</tt>.
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::readMemoryStreamAsReference(MemoryStream* target)
  {
    return doReadMemoryStream(target, true);
  }

  /**
   * \internal
   * See readMemoryStream() and readMemoryStreamAsReference()
   */
  MemoryStreamReader::ReadResult MemoryStreamReader::doReadMemoryStream(MemoryStream* target, bool byReference)
  {
    FAIL_IF_LOCKED;

    // Check EOF in typecheck, header 
    if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos) < 
      (sizeof(unsigned int)+sizeof(bool)+ (m_stream->isTypeChecked()?sizeof(TypeCode):0) ) ) 
    { 
      Errorf("MemoryStreamReader:: Reached EOF in read for MemoryStream header."); 
      return kEOFError; 
    }

    // Do typecheck
    FAIL_IF_BAD_TYPECHECK(MemoryStream, 16);

    // Read header (usedLength, isTypeChecked)
    size_t usedSizeOffset = m_stream->isTypeChecked()?m_currentPos+sizeof(TypeCode):m_currentPos;
    size_t isCheckedOffset = usedSizeOffset + sizeof(unsigned int);
    unsigned int usedSizeInTarget = *(unsigned int*)(((char*)m_stream->getDataBlock())+usedSizeOffset);
    bool isTypeCheckedInTarget = *(bool*)(((char*)m_stream->getDataBlock())+isCheckedOffset);

    // Check EOF in data seg
    size_t dataSegOffset = isCheckedOffset+sizeof(bool);
    if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-dataSegOffset) < usedSizeInTarget ) 
    { 
      Errorf("MemoryStreamReader:: Reached EOF in read for MemoryStream data."); 
      return kEOFError; 
    }

    if(target)
    {
      if(byReference)
      {
        if( !target->setUsedSize(StreamHeaderSize) )
        {
          Errorf("MemoryStreamReader:: Failed to prepare target MemoryStream.");
          return kArraySizeError;
        }
        if( !target->setExternalDataBlock( (void*)(((char*)m_stream->getDataBlock())+dataSegOffset), usedSizeInTarget, isTypeCheckedInTarget) )
        {
          Errorf("MemoryStreamReader:: Failed to set target MemoryStream's data source.");
          return kArraySizeError;
        }
      }
      else
      {
        // Setup target      
        if(!target->setUsedSize(StreamHeaderSize) || !target->reAlloc(usedSizeInTarget) 
          || !target->setTypeCheckFlag(isTypeCheckedInTarget) )
        {
          Errorf("MemoryStreamReader:: Failed to allocate storage in target MemoryStream.");
          return kArraySizeError;
        }

        // Read data into target
        memcpy(target->getDataBlock(), (void*)(((char*)m_stream->getDataBlock())+dataSegOffset), usedSizeInTarget );
        target->setUsedSize(usedSizeInTarget);
      }
    }
    
    // update currentPos
    m_unreadPos = m_currentPos;
    m_currentPos = dataSegOffset + usedSizeInTarget;
    m_nextBookmarkWasLastMove = false;
    return kNoError;
  }

  /**
   * Returns the type of the next data element in the stream. If there is no
   * more data in the stream, then <tt>kEOF</tt> is returned. Otherwise, if
   * the stream is not type-checked, then <tt>kNotTypeCheckedStream</tt> results.
   */
  MemoryStreamReader::DataType MemoryStreamReader::peekNextType()
  {
    // EOF: Just check that optional type-check and at least one byte exist
    if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos) < (m_stream->isTypeChecked()?sizeof(char)+sizeof(TypeCode):sizeof(char)) ) 
    { 
      return kEOF;
    }
        
    // check whether stream is type-checked
    if( !m_stream->isTypeChecked() )
      return kNotTypeCheckedStream;

    // get type number and map back to enum
    TypeCode typeCode = *(TypeCode*)(((char*)m_stream->getDataBlock())+m_currentPos);
    switch (typeCode & 0x7F)
    {
    case TypeCode_char:
      return kChar;
    case TypeCode_Array_char:
      return kCharArray;
    case TypeCode_bool:
      return kBool;
    case TypeCode_Array_bool:
      return kBoolArray;
    case TypeCode_int:
      return kInt;
    case TypeCode_Array_int:
      return kIntArray;
    case TypeCode_unsigned_int:
      return kUInt;
    case TypeCode_Array_unsigned_int:
      return kUIntArray;
    case TypeCode_float:
      return kFloat;
    case TypeCode_Array_float:
      return kFloatArray;
    case TypeCode_double:
      return kDouble;
    case TypeCode_Array_double:
      return kDoubleArray;
    case TypeCode_MemoryStream:
      return kMemoryStream;
    case TypeCode_NMU64:
      return kUInt64;
    case TypeCode_Array_NMU64:
      return kUInt64Array;
    case TypeCode_NMVector3:
      return kVector3;
    case TypeCode_Array_NMVector3:
      return kVector3Array;
    case TypeCode_NMVector4:
      return kVector4;
    case TypeCode_Array_NMVector4:
      return kVector4Array;
    case TypeCode_NMMatrix3:
      return kMatrix3;
    case TypeCode_Array_NMMatrix3:
      return kMatrix3Array;
    case TypeCode_NMMatrix4:
      return kMatrix4;
    case TypeCode_Array_NMMatrix4:
      return kMatrix4Array;
    }

    Errorf("MemoryStreamReader:: PeekNextType encountered an unknown type. (%d)", typeCode);
    return kUnknownTypeError;
  }

  /**
   * If the next type in the stream is an array sets <tt>arrayLength</tt> to 
   * the length of (number of elements in) the array and returns <tt>true</tt>. 
   * Otherwise, returns <tt>false</tt> and does nothing with <tt>arrayLength</tt>.
   */
  bool MemoryStreamReader::peekNextArrayLength(size_t* arrayLength)
  {
    // Check for valid array type
    DataType nextType = peekNextType();
    if(nextType != kCharArray && nextType != kBoolArray && nextType != kIntArray
      && nextType != kUIntArray && nextType != kFloatArray && nextType != kDoubleArray 
      && nextType != kUInt64Array && nextType != kVector3Array && nextType != kVector4Array
      && nextType != kMatrix3Array && nextType != kMatrix4Array
      && nextType != kNotTypeCheckedStream )
    {
      return false;
    }

    // Check EOF for typecode+arraylen or arraylen if not checked
    if( (m_stream->getUsedSize()-GET_BOOKMARKS_SIZE-m_currentPos) < (m_stream->isTypeChecked()?sizeof(unsigned int)+sizeof(TypeCode):sizeof(unsigned int)) ) 
    { 
      return false;
    }

    // Get arraylen
    if(arrayLength)
    {
      size_t lenOffset = m_stream->isTypeChecked() ? m_currentPos + sizeof(TypeCode) : m_currentPos;
      *arrayLength = *(unsigned int*)(((char*)m_stream->getDataBlock())+lenOffset);
    }
   
    return true;
  }

  /**
   * Moves the current read position in the stream to where it was prior to
   * the last read or bookmark-jump operation. Returns <tt>false</tt> if 
   * there was no prior read or jump operation. NB: This can only 'rewind' 
   * one read or jump operation.
   */
  bool MemoryStreamReader::unread()
  {
    if(m_unreadPos == INVALID_UNREAD_POS)
      return false;
    m_currentPos = m_unreadPos;
    m_unreadPos = INVALID_UNREAD_POS;
    return true;
  }

  /**
   * Moves the current read position in the stream to the next bookmarked
   * location. Returns <tt>false</tt> and does nothing if there are no
   * further bookmarks, or the bookmarks are invalid. If the current
   * position is bookmarked, this 'jumps' to the current position, unless
   * the current position was a result of calling this function.
   */
  bool MemoryStreamReader::jumpToNextBookmark()
  {
    if(GET_BOOKMARKS_SIZE + StreamHeaderSize > m_stream->getUsedSize())
      return false; // Invalid bookmarks

    // If we are currently at a bookmark, we have to make a decision:
    //  -- if we got here from jumpToNextBookmark(), then go to the next one
    //  -- otherwise 'jump' to this one.
    
    // Iterate through bookmark targets starting where we last jumped to. 
    // First one we find >= m_currentPos, jump.
    // If we have jumped back, start iterating from the beginning.
    unsigned int target = 0;
    
    if(m_haveJumpedBack)
    {
      m_lastBookmarkJumpedTo = 0;
      m_haveJumpedBack = false;
    }

    for( size_t i = m_lastBookmarkJumpedTo; i < GET_BOOKMARK_HEADER; ++i )
    {
      size_t bookMarkOffset = GET_BOOKMARKS_OFFSET + (i*sizeof(unsigned int));
      unsigned int bookMarkTarget = *(unsigned int*)(((char*)m_stream->getDataBlock())+bookMarkOffset);
      if( (m_nextBookmarkWasLastMove && bookMarkTarget > m_currentPos) 
        || (!m_nextBookmarkWasLastMove && bookMarkTarget >= m_currentPos) )
      {
        target = bookMarkTarget;
        m_lastBookmarkJumpedTo = i;
        break;
      }
    }
    
    if(target < StreamHeaderSize || target > GET_BOOKMARKS_OFFSET)
      return false; // Invalid bookmark target

    m_unreadPos = m_currentPos;
    m_currentPos = target;
    m_nextBookmarkWasLastMove = true;
    return true;
  }

  /**
   * Moves the current read position in the stream to the previous bookmarked
   * location. Returns <tt>false</tt> and does nothing if there are no
   * previous bookmarks, or the bookmarks are invalid. 
   */
  bool MemoryStreamReader::jumpToPreviousBookmark()
  {
    if(GET_BOOKMARKS_SIZE + StreamHeaderSize > m_stream->getUsedSize())
      return false; // Invalid bookmarks

    // Iterate through bookmark targets. First one we find >= m_currentPos, jump to previous.
    unsigned int target = 0;
    bool found = false;
    for( size_t i = 0; i < GET_BOOKMARK_HEADER; ++i )
    {
      size_t bookMarkOffset = GET_BOOKMARKS_OFFSET + (i*sizeof(unsigned int));
      unsigned int bookMarkTarget = *(unsigned int*)(((char*)m_stream->getDataBlock())+bookMarkOffset);
      if(bookMarkTarget >= m_currentPos)
      {
        found = true;
        break;
      }
      target = bookMarkTarget;
    }

    if(!found || target < StreamHeaderSize || target >= GET_BOOKMARKS_OFFSET)
      return false; // Invalid bookmark target

    m_unreadPos = m_currentPos;
    m_currentPos = target;
    m_nextBookmarkWasLastMove = false;
    m_haveJumpedBack = true;
    return true;
  }


  // sizes, in bytes, used when aligning types to data boundaries in write-aligned streams
  const int MemoryStreamTypeCodeAlignmentSizes[_numberOfKnownTypecodes] = 
  {
    -1,
    1,  // _kChar
    1,  // _kBool
    4,  // _kInt
    4,  // _kUInt
    4,  // _kFloat
    4,  // _kDouble
    1,  // _kArrayChar
    1,  // _kArrayBool
    4,  // _kArrayInt
    4,  // _kArrayUInt
    4,  // _kArrayFloat
    4,  // _kArrayDouble
    4, // _kMemoryStream
    4,  // _kNMU64
    4,  // _kArrayNMU64
    4, // _kVector3
    4, // _kArrayVector3
    4, // _kVector4
    4, // _kArrayVector4
    4, // _kMatrix3
    4, // _kArrayMatrix3
    4, // _kMatrix4
    4, // _kArrayMatrix4
  };

  /**
   * swap the data for a given typecode
   */
  size_t endianSwapKnownTypecode(unsigned char typecode, unsigned char* data)
  {
    switch(typecode)
    {
    case _kChar:
    case _kBool:
      // no swap required
      return sizeof(char);

    case _kInt:
    case _kUInt:
    case _kFloat:
      {
        unsigned int *iData = (unsigned int*)(data);
        *iData = NMutils::endianSwap<unsigned int>(*iData);
        return sizeof(int);
      }

    case _kVector3:
      {
        unsigned int *iData = (unsigned int*)(data);
        for (int i=0; i<3; i++)
          iData[i] = NMutils::endianSwap<unsigned int>(iData[i]);

        return sizeof(NMutils::NMVector3);
      }

    case _kVector4:
      {
        unsigned int *iData = (unsigned int*)(data);
        for (int i=0; i<4; i++)
          iData[i] = NMutils::endianSwap<unsigned int>(iData[i]);

        return sizeof(NMutils::NMVector4);
      }

    case _kMatrix3:
      {
        unsigned int *iData = (unsigned int*)(data);
        for (int i=0; i<(3*3); i++)
          iData[i] = NMutils::endianSwap<unsigned int>(iData[i]);

        return sizeof(NMutils::NMMatrix3);
      }

    case _kMatrix4:
      {
        unsigned int *iData = (unsigned int*)(data);
        for (int i=0; i<(4*4); i++)
          iData[i] = NMutils::endianSwap<unsigned int>(iData[i]);

        return sizeof(NMutils::NMMatrix4);
      }

    case _kDouble:
    case _kNMU64:
      {
        NMutils::NMU64 *iData = (NMutils::NMU64*)(data);
        *iData = NMutils::endianSwap<NMutils::NMU64>(*iData);
        return sizeof(NMutils::NMU64);
      }
    }

    FastAssert(0);
    return 0;
  }

  
  size_t endianSwapKnownArrayType(unsigned char typecode, unsigned char* data, size_t arrayCount)
  {
    size_t dataConsume = 0; 
    for (size_t i=0; i<arrayCount; i++)
    {
      size_t skipAhead = 0;
      switch (typecode)
      {
      case _kArrayChar:
      case _kArrayBool:
        skipAhead = sizeof(char);
        break;

      case _kArrayInt:
      case _kArrayUInt:
      case _kArrayFloat:
        skipAhead = endianSwapKnownTypecode(_kUInt, &data[dataConsume]);
        break;

      case _kArrayDouble:
      case _kArrayNMU64:
        skipAhead = endianSwapKnownTypecode(_kNMU64, &data[dataConsume]);
        break;

      case _kArrayVector3:
        skipAhead = endianSwapKnownTypecode(_kVector3, &data[dataConsume]);
        break;

      case _kArrayVector4:
        skipAhead = endianSwapKnownTypecode(_kVector4, &data[dataConsume]);
        break;

      case _kArrayMatrix3:
        skipAhead = endianSwapKnownTypecode(_kMatrix3, &data[dataConsume]);
        break;

      case _kArrayMatrix4:
        skipAhead = endianSwapKnownTypecode(_kMatrix4, &data[dataConsume]);
        break;
      }
      dataConsume += skipAhead;
    }
    return dataConsume; 
  }
  /**
   * This function expects a type-checked memory stream, as it is a post-process
   * and wouldn't be able to figure out types without the embedded type codes
   */
  void doEndianSwapMemoryStream(unsigned char* data, size_t datasize)
  {
    size_t dataConsume = 0;

    // swap the header bytes around
    unsigned int *headerPtr = (unsigned int*)data;
    headerPtr[0] = NMutils::endianSwap<unsigned int>(headerPtr[0]);    // amount used
    headerPtr[1] = NMutils::endianSwap<unsigned int>(headerPtr[1]);    // bookmarks
    dataConsume += sizeof(unsigned int) * 2;

    while (dataConsume < datasize)
    {
      bool alignedBlock = (data[dataConsume] & 0x80) == 0x80;
      unsigned char typecode = data[dataConsume] & 0x7F;  // mask off alignment bit
      dataConsume ++;

      // jump over alignment padding if present
      if (alignedBlock)
      {
        int sizeOfType = MemoryStreamTypeCodeAlignmentSizes[typecode];
        if (sizeOfType > 1)
        {
          int _remains = (dataConsume % sizeOfType);
          if (_remains > 0)
          {
            dataConsume += (sizeOfType - _remains);
          }
        }
      }

      switch(typecode)
      {
        // convert a basic typecode
        case _kChar:
        case _kBool:
        case _kInt:
        case _kUInt:
        case _kFloat:
        case _kVector3:
        case _kVector4:
        case _kMatrix3:
        case _kMatrix4:
        case _kDouble:
        case _kNMU64:
          {
            size_t skipAhead = endianSwapKnownTypecode(typecode, &data[dataConsume]);
            dataConsume += skipAhead;
          }
          break;

        // for array types we swap the array length integer and iterate each
        // array entry using endianSwapKnownTypecode to swap data
        case _kArrayChar:
        case _kArrayBool:
        case _kArrayInt:
        case _kArrayUInt:
        case _kArrayFloat:
        case _kArrayDouble:
        case _kArrayNMU64:
        case _kArrayVector3:
        case _kArrayVector4:
        case _kArrayMatrix3:
        case _kArrayMatrix4:
          {
            unsigned int *arrayLength = (unsigned int*)(&data[dataConsume]), arrayCount;

            arrayCount = *arrayLength;
            *arrayLength = NMutils::endianSwap<unsigned int>(*arrayLength);

            dataConsume += sizeof(unsigned int);
            for (unsigned int i=0; i<arrayCount; i++)
            {
              size_t skipAhead = 0;
              switch (typecode)
              {
                case _kArrayChar:
                case _kArrayBool:
                  skipAhead = sizeof(char);
                  break;

                case _kArrayInt:
                case _kArrayUInt:
                case _kArrayFloat:
                  skipAhead = endianSwapKnownTypecode(_kUInt, &data[dataConsume]);
                  break;

                case _kArrayDouble:
                case _kArrayNMU64:
                  skipAhead = endianSwapKnownTypecode(_kNMU64, &data[dataConsume]);
                  break;

                case _kArrayVector3:
                  skipAhead = endianSwapKnownTypecode(_kVector3, &data[dataConsume]);
                  break;

                case _kArrayVector4:
                  skipAhead = endianSwapKnownTypecode(_kVector4, &data[dataConsume]);
                  break;

                case _kArrayMatrix3:
                  skipAhead = endianSwapKnownTypecode(_kMatrix3, &data[dataConsume]);
                  break;

                case _kArrayMatrix4:
                  skipAhead = endianSwapKnownTypecode(_kMatrix4, &data[dataConsume]);
                  break;
              }
              dataConsume += skipAhead;
            }
          }
          break;

        // recursively call doEndianSwapMemoryStream to swap any embedded streams
        case _kMemoryStream:
          {
            // keep track of block length, we convert it after recursing
            unsigned int *blockLength = (unsigned int*)(&data[dataConsume]);
            dataConsume += sizeof(unsigned int);

            // the embedded stream better be typed or we cannot swap it
            FastAssert((*(bool*)(&data[dataConsume])));
            dataConsume += sizeof(bool);

            // recursively swap embedded streams
            doEndianSwapMemoryStream(&data[dataConsume], *blockLength);
            dataConsume += (*blockLength);

            // convert block length last
            *blockLength = NMutils::endianSwap<unsigned int>(*blockLength);
          }
          break;

        // unknown or mangled type in the stream?
        default:
          FastAssert(0);
          break;
      }
    }
  }

  void endianSwapMemoryStream(NMutils::MemoryStream &stream)
  {
    unsigned char* data = (unsigned char*)stream.getDataBlock();
    size_t datasize = stream.getUsedSize();

    doEndianSwapMemoryStream(data, datasize);
  }

  unsigned int generateCRC(unsigned char* data, size_t datasize)
  {
    const unsigned int modAdler = 65521;

    size_t len = datasize;
    unsigned int a = 1, b = 0;

    while (len) {

      // 5550 is the largest number of sums that can be performed without overflowing b
      size_t tlen = len > 5550 ? 5550 : len;

      len -= tlen;
      do {
        a += *data++;
        b += a;
      } while (--tlen);

      a = (a & 0xffff) + (a >> 16) * (65536-modAdler);
      b = (b & 0xffff) + (b >> 16) * (65536-modAdler);
    }

    /* It can be shown that a <= 0x1013a here, so a single subtract will do. */
    if (a >= modAdler)
      a -= modAdler;

    /* It can be shown that b can reach 0xffef1 here. */
    b = (b & 0xffff) + (b >> 16) * (65536-modAdler);
    if (b >= modAdler)
      b -= modAdler;

    return b << 16 | a;
  }
}
