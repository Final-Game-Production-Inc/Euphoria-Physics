// 
// data/growbuffer.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_GROWBUFFER_H 
#define DATA_GROWBUFFER_H 

#include "file/handle.h"

namespace rage 
{
class sysMemAllocator;
class fiDevice;

//PURPOSE
//  Grow-able memory buffer.
class datGrowBuffer
{
public:

	enum Flags
	{
		//Don't allocate memory, just determine required buffer size.
		NULL_BUFFER     = 0x01,
		//Don't grow the buffer - use the fixed buffer passed to Reset().
		FIXED_BUFFER    = 0x02,
		//Null-terminate the buffer
		NULL_TERMINATE  = 0x04,
		//Buffer is read only
		READ_ONLY       = 0x08
	};

    //Increment by which the buffer is grown.
    //This MUST be a power of 2.
    enum
    {
        DEFAULT_GROW_INCR = 256
    };

	datGrowBuffer();

	~datGrowBuffer();

	//PURPOSE
	//  Initialize the buffer.
	//PARAMS
	//  allocator   - Optional.  Used to allocate memory.  If NULL the global
	//                allocator will be used.
	//  flags       - Bit combination of Flags.
	//  growIncr	- increment to increase the grow buffer by when expanding
	//					 (NOTE: must be power of 2)
	void Init(sysMemAllocator* allocator,
		const unsigned flags,
		const unsigned growIncr = DEFAULT_GROW_INCR);

	//PURPOSE
	//  Initialize the buffer with a fixed buffer.  No memory allocation
	//  will be performed and the GrowBuffer will not grow.
	//PARAMS
	//  buffer      - Fixed buffer.
	//  bufLen      - Length of buffer.
	//  flags       - Bit combination of Flags.
	void Init(void* buffer,
		const unsigned bufLen,
		const unsigned flags);

	//PURPOSE
	//  Initialize the buffer with a fixed read only buffer.
    //  The length of the grow buffer will be equal to bufLen.
	//PARAMS
	//  buffer      - Fixed buffer.
	//  bufLen      - Length of buffer.
	//  flags       - Bit combination of Flags.
	void InitReadOnly(const void* buffer,
		                const unsigned bufLen);

	//PURPOSE
	//  Clear the buffer and free memory.
	void Clear();

	//PURPOSE
	//  Resets the length to 0. 
	void Reset();

    //PURPOSE
    //  Returns a fiHandle for use with a fiDeviceGrowBuffer.
    fiHandle GetFiHandle();

    //PURPOSE
    //  Returns an instance of fiDeviceGrowBuffer to use with
    //  the return value of GetFiHandle.
    const fiDevice* GetFiDevice();

	//PURPOSE
	//  Return current flags.
	unsigned GetFlags() const;

	//PURPOSE
	//  Append contents of buf to this buffer.
	//PARAMS
	//  buf     - Buffer to append.
	//  bufLen  - Length of buffer.
    //RETURNS
    //  Number of bytes appended, or -1 for error
	int Append(const void* buf, const unsigned bufLen);

	//PURPOSE
	//  Prepend contents of buf to this buffer.
	//PARAMS
	//  buf     - Buffer to append.
	//  bufLen  - Length of buffer.
    //RETURNS
    //  Number of bytes prepended, or -1 for error
	int Prepend(const void* buf, const unsigned bufLen);

	//PURPOSE
	//  Append entire contents of buf to this buffer.
	//PARAMS
	//  buf     - Buffer to append.
	//  bufLen  - Length of buffer.
    //RETURNS
    //  True if bufLen bytes were appended.
    bool AppendOrFail(const void* buf, const unsigned bufLen);

	//PURPOSE
	//  Prepend entire contents of buf to this buffer.
	//PARAMS
	//  buf     - Buffer to append.
	//  bufLen  - Length of buffer.
    //RETURNS
    //  True if bufLen bytes were prepended.
    bool PrependOrFail(const void* buf, const unsigned bufLen);

	//PURPOSE
	//  Truncate the buffer.
	//PARAMS
	//  newLen      - New buffer length.  Must be shorter than the current
	//                buffer length.
	void Truncate(const unsigned newLen);

	//PURPOSE
	//  Removes "len" bytes starting at "start".  If start + len is longer
	//  than the buffer length then the buffer will be truncated to the
	//  byte just prior to "start".
	void Remove(const int start, const unsigned len);

	//PURPOSE
	//  Consume bufLen bytes from the grow buffer, starting from the byte at
    //  index 0.  Remaining bytes are transfered to the beginning of the
    //  growbuffer.
	//PARAMS
	//  buf     - Destination buffer
	//  bufLen  - Length of destination buffer.
	//RETURNS
	//  Number of bytes consumed.
	unsigned Consume(void* buf, const unsigned bufLen);

	//PURPOSE
	//  If the new capacity is greater than the current capacity then
	//  preallocate at least enough memory to satisfy the new capacity.
	//  Doesn't change the buffer length, just the capacity.
	//PARAMS
	//  newCapacity     - New capacity of buffer.
	bool Preallocate(const unsigned newCapacity);

	//PURPOSE
	//  Returns a pointer to the underlying buffer.
	void* GetBuffer();

	//PURPOSE
	//  Returns a pointer to the underlying buffer.
	const void* GetBuffer() const;

	//PURPOSE
	//  Returns the length of the buffer.
	unsigned Length() const;

	//PURPOSE
	//  Returns the capacity of the buffer.
	unsigned GetCapacity() const;

	//PURPOSE
	//  Returns true if the buffer is growable.
	bool IsGrowable() const;

	//PURPOSE
	//  Returns true if the buffer is read only.
    bool IsReadOnly() const;

	//PURPOSE
	//  Returns true if the buffer is writable.
    bool IsWritable() const;

	//PURPOSE
	//  Remove at most len bytes from buffer, starting at start.
	static void Remove(void* buffer,
		unsigned* bufLen,
		const unsigned start,
		const unsigned len);

private:

	//PURPOSE
	//  Allocates memory using either the allocator passed to Init() or
	//  the global allocator.
	void* Allocate(const unsigned size);

	//PURPOSE
	//  Frees memory.
	void Free(void* data);

	//Prevent copying
	datGrowBuffer(const datGrowBuffer&);
	datGrowBuffer& operator=(const datGrowBuffer&);

	u8* m_Buf;
	unsigned m_Capacity;
	unsigned m_Len;
	unsigned m_Flags;
	sysMemAllocator* m_Allocator;
	unsigned m_growIncr;  //Multiples of size to grow the buffer in.

    fiHandle m_FiHandle;

	ASSERT_ONLY(bool m_Initialized);
};

} // namespace rage

#endif

