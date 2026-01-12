#ifndef COLLISION_MEMORY_H
#define COLLISION_MEMORY_H

#include "PrimitiveCache.h"

namespace rage
{

struct phCollisionMemory
{
#if __SPU

	//enum { spuCollisionBufferSize = 23 * 1024 };
	//u8 m_spuCollisionBuffer[spuCollisionBufferSize] ;

	enum { primBufferSize = 4 * 1024 };
	u8 m_primBuffer[primBufferSize];
	PrimitiveCache m_primCache;

#else // __SPU

	enum { primBufferSize = 16 * 1024 };
		// TODO: Allocate from the constraint solver memory buffer.
	u8 m_primBuffer[primBufferSize];
	PrimitiveCache m_primCache;

#endif // __SPU

	phCollisionMemory()
	{
		m_primCache.m_buffer.SetBuffer(m_primBuffer,primBufferSize);
	}
};

} // namespace rage

#endif // COLLISION_MEMORY_H