// 
// grcore/vertexbuffer_ogl.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"
#if __OPENGL

#include "grcore/vertexbuffer.h"
#include "grcore/device.h"
#include "grcore/fvf.h"
#include "mesh/mesh.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "atl/array_struct.h"

#include "system/cache.h"
#include "system/memory.h"
#include "system/ipc.h"
#include "system/xtl.h"
#include "diag/tracker.h"

#include "profile/page.h"
#include "profile/group.h"

#include "math/float16.h"

namespace rage
{

const grcVertexBuffer* grcLockedVertexBuffer = 0;

grcVertexBuffer::grcVertexBuffer(int vertCount, u32 usage, const grcFvf& pFvf, bool bDynamic)
{
	m_Dynamic = bDynamic;
	m_Locked = false;
	m_LockThread = 0;

	if( bDynamic )
		m_Fvf = MakeDynamicFvf(&pFvf);
	else
		m_Fvf = rage_new grcFvf(pFvf);
	m_VertCount = vertCount;
	m_Usage = usage;
	m_VertexData = NULL;
	m_Stride = m_Fvf->GetTotalSize();
	if( m_Fvf->IsDynamicOrder() )
	{
		m_Stride += (0x10 - (m_Stride & 0xF));
	}

	// Create the buffer
	glGenBuffers(1, &m_OGLBuffer);

	// Initialize the buffer with junk data
	int tempSize = m_VertCount * m_Stride;
	u8* tempJunk = Alloca(u8, tempSize);
	glBindBuffer(GL_ARRAY_BUFFER, m_OGLBuffer);
	glBufferData(GL_ARRAY_BUFFER, tempSize, tempJunk, bDynamic ? GL_STATIC_DRAW : GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

grcVertexBuffer::grcVertexBuffer(class datResource&)
{
	Assert(0 && "Implement me");
}

grcVertexBuffer::~grcVertexBuffer()
{
	// Unlock the buffer
	Unlock();

	// Delete the buffer
	glDeleteBuffers(1, &m_OGLBuffer);
}

bool grcVertexBuffer::Lock() const
{
#if __VBSTATS
	PF_FUNC(Lock);
#endif

	Assert(!m_Locked || m_LockThread != sysIpcCurrentThreadIdInvalid);
	AssertMsg(!m_Locked || m_LockThread == sysIpcGetCurrentThreadId(), "You are trying to lock a vertex buffer twice from 2 different threads");
	Assert(!m_Locked);

	// Make sure there isnt currently a locked buffer
	if( grcLockedVertexBuffer && grcLockedVertexBuffer != this )
	{
		grcErrorf("Only one vertex buffer can be locked at a time");
		return false;
	}

	// Bind this buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_OGLBuffer);

	// Map the memory
	m_LockPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);

	// Mark this buffer as locked
	m_Locked = true;
	grcLockedVertexBuffer = this;

	m_LockThread = sysIpcGetCurrentThreadId();

	return true;
}

bool grcVertexBuffer::LockReadOnly() const
{
	return this->Lock();
}

bool grcVertexBuffer::Unlock() const
{
#if __VBSTATS
	PF_FUNC(Unlock);
#endif

	Assert(m_Locked);
	Assert(m_LockThread == sysIpcGetCurrentThreadId());

	if( grcLockedVertexBuffer != this )
	{
		grcErrorf("This buffer is not the locked buffer");
		return false;
	}

	// Unmap the memory
	glUnmapBuffer(GL_ARRAY_BUFFER);
	m_LockPtr = 0;

	// Unbind the buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	grcLockedVertexBuffer = 0;

	// Mark this buffer as unlocked
	m_Locked = false;
	m_LockThread = sysIpcCurrentThreadIdInvalid;

	return !m_Locked;
}

void grcVertexBuffer::MakeDynamic()
{
	// Reorder the vertex buffer with padding for optimal memory access on the cpu
	// Position - float4
	// Normal - float4
	// Tangent0 - float4
	// Tangent1 - float4
	// Tangent2 - float4
	// Tangent3 - float4
	// Tangent4 - float4
	// Tangent5 - float4
	// Tangent6 - float4
	// Tangent7 - float4
	// TexCoord0 - float4
	// TexCoord1 - float4
	// TexCoord2 - float4
	// TexCoord3 - float4
	// TexCoord4 - float4
	// TexCoord5 - float4
	// TexCoord6 - float4
	// TexCoord7 - float4
	// Weights - float4
	// Binding - u32
	// Diffuse - u32
	// Specular - u32
	// UnusedPadding - u32

	Assert(0 && "Implement Me");
}

#if __DECLARESTRUCT
void grcVertexBuffer::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(grcVertexBuffer);
	STRUCT_FIELD(m_VertCount);
	STRUCT_FIELD(m_Usage);
	STRUCT_FIELD(m_Fvf);
	STRUCT_FIELD(m_VertexData);

	STRUCT_FIELD(m_OGLBuffer);

	STRUCT_FIELD(m_LockPtr);
	STRUCT_CONTAINED_ARRAY(m_Offsets);
	STRUCT_FIELD(m_Stride);
	STRUCT_FIELD(m_Locked);
	STRUCT_FIELD(m_LockThread);
	STRUCT_FIELD(m_Dynamic);

	STRUCT_END();
}
#endif

} // namespace rage

#endif // __OPENGL
