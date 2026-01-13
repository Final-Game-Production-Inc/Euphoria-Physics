// 
// grcore/indexbuffer_ogl.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"
#if __OPENGL

#include "grcore/indexbuffer.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"

#include <string.h>


namespace rage
{

const grcIndexBuffer* grcLockedIndexBuffer = 0;

grcIndexBuffer::grcIndexBuffer(int indexCount, int indexSizeBytes)
{
	m_Locked = false;
	m_IndexCount = indexCount;
	m_IndexSize = indexSizeBytes;

	// Create the buffer
	glGenBuffers(1, &m_OGLBuffer);

	// Initialize the entire buffer to zeros
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_OGLBuffer);
	int tempBytes = indexSizeBytes * indexCount;
	u8* tempIndices = Alloca(u8, tempBytes);
	memset(tempIndices, 0, tempBytes);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tempBytes, tempIndices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

grcIndexBuffer::grcIndexBuffer(datResource&)
{
	Assert(0 && "Implement me");
}

grcIndexBuffer::~grcIndexBuffer()
{
	// Unlock the buffer
	Unlock();

	// Delete the buffer
	glDeleteBuffers(1, &m_OGLBuffer);
}

bool grcIndexBuffer::Lock()
{
	// Make sure there isnt already a locked index buffer
	if( grcLockedIndexBuffer && !m_Locked )
	{
		grcErrorf("Only one index buffer can be locked at a time");
		return false;
	}

	if( !m_Locked )
	{
		// Bind this index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_OGLBuffer);

		// Map the buffer to a memory address
		u8* pLockPtr = (u8*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
		Assert(pLockPtr);
		if( !pLockPtr )
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			grcLockedIndexBuffer = 0;
			return false;
		}

		// Mark it as locked
		m_LockPtr = pLockPtr;
		m_Locked = true;
		grcLockedIndexBuffer = this;
	}
	return m_Locked;
}

bool grcIndexBuffer::Unlock() const
{
	if( m_Locked )
	{
		// Make sure this buffer is the locked buffer
		if( grcLockedIndexBuffer != this )
		{
			grcErrorf("Cannot unlock an index buffer that isnt the currently locked index buffer");
			return false;
		}

		// Unmap the memory
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		m_LockPtr = 0;

		// Bind the 0 index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Mark this buffer as unlocked
		m_Locked = false;
		grcLockedIndexBuffer = 0;

		return true;
	}
	return false;
}


void grcIndexBuffer::Set(rage::grcIndexBuffer* pSource)
{
	Assert(m_IndexCount == pSource->GetIndexCount());
	Assert(m_IndexSize == pSource->GetIndexSize());

	// Allocate temp space
	int tempBytes = m_IndexCount * m_IndexSize;
	u8* tempSpace = Alloca(u8, tempBytes);

	// Lock the source buffer
	pSource->Lock();

	// Copy the index data into the temp space
	sysMemCpy(tempSpace, pSource->GetLockPointer(), tempBytes);

	// Unlock the source buffer
	pSource->Unlock();

	// Lock this buffer
	Lock();

	// Copy the data into the buffer
	sysMemCpy(m_LockPtr, tempSpace, tempBytes);

	// Unlock this buffer
	Unlock();
}

#if __DECLARESTRUCT
void grcIndexBuffer::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(grcIndexBuffer);
	STRUCT_FIELD(m_IndexCount);
	STRUCT_FIELD(m_IndexSize);
	STRUCT_FIELD(m_IndexData);
	STRUCT_FIELD(m_OGLBuffer);
	STRUCT_FIELD(m_LockPtr);
	STRUCT_FIELD(m_Locked);
	STRUCT_IGNORE(pad0);
	STRUCT_IGNORE(pad1);
	STRUCT_IGNORE(pad2);
	STRUCT_END();
}
#endif

} // namespace rage

#endif // __OPENGL
