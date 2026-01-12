#ifndef LINEAR_MEMORY_ALLOCATOR_H
#define LINEAR_MEMORY_ALLOCATOR_H

namespace rage
{

struct LinearMemoryAllocator
{
	u8 * m_cur;
	u8 * m_start;
	u8 * m_end;

	__forceinline void NullBuffer()
	{
		m_start = NULL;
		m_end = NULL;
		m_cur = NULL;
	}

	__forceinline void SetBuffer(u8 * buffer, const int buffer_size)
	{
		FastAssert(buffer);
		FastAssert(buffer_size > 0);
		m_start = buffer;
		m_end = m_start + buffer_size;
		m_cur = m_start;
	}

	__forceinline void Reset()
	{
		m_cur = m_start;
	}

	__forceinline int GetUsed()
	{
		return (int)(m_cur - m_start);
	}

	__forceinline u8 * Align(u8* val, const u32 alignment) const
	{
		FastAssert(((alignment-1)&alignment) == 0 && alignment > 0);	// Alignment must be a power of 2.
		//const size_t aligned_val = ((size_t(val) - 1) & ~(size_t(alignment) - 1)) + alignment;
		const size_t aligned_val = (size_t(val) + size_t(alignment - 1)) & ~(size_t(alignment) - 1);
		return (u8*) aligned_val;
	}

	__forceinline void * Alloc(const u32 size, const u32 alignment)
	{
		u8 * ptr = Align(m_cur,alignment);
		u8 * new_cur = ptr + size;
		if (new_cur <= m_end)
		{
			m_cur = new_cur;
			return ptr;
		}
		else
		{
			FastAssert(0);
			return NULL;
		}
	}

	__forceinline void * FastAlloc(const u32 size, const u32 alignment)
	{
		u8 * ptr = Align(m_cur,alignment);
		m_cur = ptr + size;
		FastAssert(m_cur <= m_end);
		return ptr;
	}

	__forceinline bool CanAlloc(const u32 size, const u32 alignment) const
	{
		u8 * ptr = Align(m_cur,alignment);
		return (ptr + size <= m_end);
	}
};

} // namespace rage

#endif // LINEAR_MEMORY_ALLOCATOR_H
