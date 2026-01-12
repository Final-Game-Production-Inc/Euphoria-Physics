#ifndef PHCORE_BITVECTOR_H
#define PHCORE_BITVECTOR_H

#include "constants.h"
#include "math/amath.h"

namespace rage
{

class BitVectorBase
{
protected:

	u32 * m_bitBuffer;		// The bit buffer.
	u32 m_bitBufferSize;	// The size of the bit buffer array.
	u32 m_bitVectorSize;	// The size of the bit vector.


	template <const bool SPU_DMA> __forceinline u32 GetBitChunk(const u32 bitBufferIndex, const int DMA_TAG_ID) const
	{
		FastAssert(SPU_DMA == false || DMA_TAG_ID != INVALID_DMA_TAG_ID);
		// Assume bitBufferIndex has been validated.
#if __SPU
		const u32 bitChunk = SPU_DMA ? cellDmaGetUint32((uint64_t)(m_bitBuffer+bitBufferIndex), DMA_TAG(DMA_TAG_ID), 0, 0) : m_bitBuffer[bitBufferIndex];
#else // __SPU
		(void)DMA_TAG_ID;
		const u32 bitChunk = m_bitBuffer[bitBufferIndex];
#endif // __SPU
		return bitChunk;
	}

	template <const bool SPU_DMA> __forceinline void SetBitChunk(const u32 bitBufferIndex, const u32 bitChunk, const int DMA_TAG_ID)
	{
		FastAssert(SPU_DMA == false || DMA_TAG_ID != INVALID_DMA_TAG_ID);
		// Assume bitBufferIndex has been validated.
#if __SPU
		if (SPU_DMA)
			cellDmaPutUint32(bitChunk, (uint64_t)(m_bitBuffer+bitBufferIndex), DMA_TAG(DMA_TAG_ID), 0, 0);
		else
#else
		(void)DMA_TAG_ID;
#endif // __SPU
			m_bitBuffer[bitBufferIndex] = bitChunk;
	}

public:

	enum
	{
		INVALID_DMA_TAG_ID = -1,
	};

	BitVectorBase() 
	{
	}

	BitVectorBase(u32 * const bitBuffer, const u32 bitBufferSize, const u32 bitVectorSize) : m_bitBuffer(bitBuffer), m_bitBufferSize(bitBufferSize), m_bitVectorSize(bitVectorSize)
	{
		FastAssert(m_bitBufferSize * 32 >= m_bitVectorSize);
	}

	// SetAll: Sets all bits to either one or zero.
	void SetAll(const u32 bitVal)
	{
		const int Val = bitVal ? 0xFFFFFFFF : 0;
		memset(m_bitBuffer,Val,sizeof(u32)*m_bitBufferSize);
	}

	// SetBit: Sets the value at BitIndex and returns the previous value at BitIndex.
	template <const bool SPU_DMA> u32 SetBitGeneric(const u32 bitIndex, const u32 bitVal, const int DMA_TAG_ID)
	{
		FastAssert(bitIndex < m_bitVectorSize);
		const u32 bitBufferIndex = bitIndex >> 5;//bitIndex / 32;
		const u32 BitChunkIndex = bitIndex & (32-1);//bitIndex % 32;
		FastAssert(bitBufferIndex < m_bitBufferSize);
		FastAssert(BitChunkIndex < 32);

		const u32 bitChunkMask = (1 << BitChunkIndex);

		const u32 bitChunk = GetBitChunk<SPU_DMA>(bitBufferIndex,DMA_TAG_ID);

		const u32 setOneChunk = bitChunk | bitChunkMask;
		const u32 setZeroChunk = bitChunk & ~bitChunkMask;
#if 0
		const u32 selectMask = GenerateMaskNZ(bitVal);
		const u32 newBitChunk = ISelectI(selectMask,setZeroChunk,setOneChunk);
#else
		const u32 newBitChunk = bitVal ? setOneChunk : setZeroChunk;
#endif

		SetBitChunk<SPU_DMA>(bitBufferIndex,newBitChunk,DMA_TAG_ID);

		const u32 currentBitVal = bitChunk & bitChunkMask;
		return currentBitVal;
	}

	// GetBit: Returns the value at bitIndex.
	template <const bool SPU_DMA> u32 GetBitGeneric(const u32 bitIndex, const int DMA_TAG_ID) const
	{
		FastAssert(bitIndex < m_bitVectorSize);
		const u32 bitBufferIndex = bitIndex >> 5;//bitIndex / 32;
		const u32 BitChunkIndex = bitIndex & (32-1);//bitIndex % 32;
		FastAssert(bitBufferIndex < m_bitBufferSize);
		FastAssert(BitChunkIndex < 32);
		const u32 bitChunkMask = (1 << BitChunkIndex);
		const u32 bitChunk = GetBitChunk<SPU_DMA>(bitBufferIndex,DMA_TAG_ID);
		const u32 currentBitVal = bitChunk & bitChunkMask;
		return currentBitVal;
	}

	u32 SetBit(const u32 bitIndex, const u32 bitVal)
	{
		return SetBitGeneric<false>(bitIndex,bitVal,INVALID_DMA_TAG_ID);
	}

	u32 GetBit(const u32 bitIndex) const
	{
		return GetBitGeneric<false>(bitIndex,INVALID_DMA_TAG_ID);
	}

#if __SPU
	u32 SetBitDMA(const u32 bitIndex, const u32 bitVal, const int DMA_TAG_ID)
	{
		return SetBitGeneric<true>(bitIndex,bitVal,DMA_TAG_ID);
	}

	u32 GetBitDMA(const u32 bitIndex, const int DMA_TAG_ID) const
	{
		return GetBitGeneric<true>(bitIndex,DMA_TAG_ID);
	}
#endif // __SPU

	u32 GetBitVectorSize() const
	{
		return m_bitVectorSize;
	}
};

template <const int BIT_VECTOR_SIZE> class BitVectorFixedSize : public BitVectorBase
{
	enum
	{
		BIT_BUFFER_SIZE = BIT_VECTOR_SIZE / 32 + 1
	};

	u32 m_buffer[BIT_BUFFER_SIZE];

public:

	BitVectorFixedSize() : BitVectorBase(m_buffer,BIT_BUFFER_SIZE,BIT_VECTOR_SIZE) 
	{
	}
};

class BitVectorAllocated : public BitVectorBase
{

public:

	BitVectorAllocated(const u32 bitVectorSize) : BitVectorBase(NULL,0,0) 
	{
		Allocate(bitVectorSize);
	}
	
	BitVectorAllocated() : BitVectorBase(NULL,0,0) 
	{
	}
	
	~BitVectorAllocated() 
	{ 
		Free();
	}

	void Allocate(const u32 bitVectorSize)
	{
		FastAssert(m_bitBuffer == NULL);	// Require user to explicitly free before re-allocating.
		FastAssert(bitVectorSize > 0 && bitVectorSize < 1024 * 64);
		m_bitVectorSize = bitVectorSize;
		m_bitBufferSize = bitVectorSize / 32 + 1;
		m_bitBuffer = rage_new u32[m_bitBufferSize];
	}

	void Free()
	{
		if (m_bitBuffer)
		{
			delete [] m_bitBuffer;
			m_bitBuffer = NULL;
			m_bitBufferSize = 0;
			m_bitVectorSize = 0;
		}
	}
};

}; // namespace rage

#endif // PHCORE_BITVECTOR_H