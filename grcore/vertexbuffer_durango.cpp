// 
// grcore/vertexbuffer_durango.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"
#include "device.h"
#include "wrapper_d3d.h"
#include "vertexbuffer.h"
#include "fvf.h"
#include "channel.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "channel.h"

#if	(RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO

//#pragma comment(lib,"xg.lib")

// TEMP:- Work around until project generation is fixed (see B*1582860).
#if !__D3D11
#include "../3rdparty/durango/xg.h"
#else // !__D3D11
#include <xg.h>
#endif // !__D3D11

#if RSG_DURANGO 
#include <xdk.h>
#if _XDK_VER > 9299
#include "system/d3d11.h"
#endif // _XDK_VER > 9299
#endif // RSG_DURANGO

namespace rage 
{

/*======================================================================================================================================*/
/* grcVertexBufferDurango class.																										*/
/*======================================================================================================================================*/
	
grcVertexBufferDurango::grcVertexBufferDurango(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory) 
: grcVertexBuffer(vertCount,pFvf,bReadWrite,bDynamic,preAllocatedMemory)
, m_Buffer(GetVertexStride()*GetVertexCount(), GetVertexStride(), grcBindVertexBuffer, preAllocatedMemory)
{
	m_VertexData = (u8 *)LockRO();
	UnlockRO();
}


grcVertexBufferDurango::grcVertexBufferDurango(class datResource& rsc) 
: grcVertexBuffer(rsc)
, m_Buffer(rsc, grcBindVertexBuffer)
{
	m_VertexData = (u8 *)LockRO();
	UnlockRO();
}


grcVertexBufferDurango::~grcVertexBufferDurango()
{
	m_VertexData = NULL;
}


void grcVertexBufferDurango::ReleaseD3DBufferHack()
{
	m_Buffer.ReleaseD3DBufferHack();
}


const void *grcVertexBufferDurango::LockRO() const
{
	return (const void *)Lock((u32)grcsRead, 0, 0);
}


void *grcVertexBufferDurango::LockWO() const
{
	return Lock((u32)grcsWrite, 0, 0);
}


void *grcVertexBufferDurango::LockRW() const
{
	return Lock((u32)grcsRead | (u32)grcsWrite, 0, 0);
}


void grcVertexBufferDurango::UnlockRO() const
{
	Unlock((u32)grcsRead);
}


void grcVertexBufferDurango::UnlockWO() const
{
	Unlock((u32)grcsWrite);
}


void grcVertexBufferDurango::UnlockRW() const
{
	Unlock((u32)grcsRead | (u32)grcsWrite);
}


void *grcVertexBufferDurango::Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const
{
	return m_Buffer.Lock(flags, offset, size, (void **)&m_LockPtr);
}


void grcVertexBufferDurango::Unlock(u32 flags) const
{
	m_Buffer.Unlock(flags);
	m_LockPtr = NULL;
}


const void *grcVertexBufferDurango::GetUnsafeReadPtr() const
{
	return m_VertexData;
}


#if __DECLARESTRUCT
void grcVertexBufferDurango::DeclareStruct(datTypeStruct &s)
{
	grcVertexBuffer::DeclareStruct(s);

	STRUCT_BEGIN(grcVertexBufferDurango);
	STRUCT_FIELD(m_Buffer);
	STRUCT_END();
}
#endif

/*======================================================================================================================================*/
/* grcVertexDurango functions.																											*/
/*======================================================================================================================================*/

#if RSG_DURANGO && !__RESOURCECOMPILER

const void *grcVertexBuffer::LockRO() const 
{ 
	return static_cast<const grcVertexBufferDurango *>(this)->LockRO(); 
}

void *grcVertexBuffer::LockWO() const 
{	
	return static_cast<const grcVertexBufferDurango *>(this)->LockWO(); 
}

void *grcVertexBuffer::LockRW() const 
{	
	return static_cast<const grcVertexBufferDurango *>(this)->LockRW(); 
}

void *grcVertexBuffer::Lock(u32 flags, u32 offset, u32 size) const 
{ 
	return static_cast<const grcVertexBufferDurango *>(this)->Lock(flags, offset, size); 
}

void grcVertexBuffer::UnlockRO() const 
{ 
	return static_cast<const grcVertexBufferDurango *>(this)->UnlockRO(); 
}

void grcVertexBuffer::UnlockWO() const 
{ 
	return static_cast<const grcVertexBufferDurango *>(this)->UnlockWO(); 
}

void grcVertexBuffer::UnlockRW() const 
{ 
	return static_cast<const grcVertexBufferDurango *>(this)->UnlockRW(); 
}

const void *grcVertexBuffer::GetUnsafeReadPtr() const 
{	
	return static_cast<const grcVertexBufferDurango *>(this)->GetUnsafeReadPtr(); 
}

#endif // RSG_DURANGO && !__RESOURCECOMPILER

}; // namespace rage 

#endif	// (RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO
