// 
// grcore/vertexbuffer_durango.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "config.h"
#include "device.h"
#include "wrapper_d3d.h"
#include "indexbuffer.h"
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
/* grcIndexBufferDurango class.																											*/
/*======================================================================================================================================*/

grcIndexBufferDurango::grcIndexBufferDurango(int indexCount, void *preAllocatedMemory)
: grcIndexBuffer(indexCount)
, m_Buffer(indexCount*sizeof(u16), 0, grcBindIndexBuffer, preAllocatedMemory)
{
	m_IndexData = (u16 *)LockRO();
	UnlockRO();
}


grcIndexBufferDurango::grcIndexBufferDurango(class datResource& rsc) 
: grcIndexBuffer(rsc)
, m_Buffer(rsc, grcBindIndexBuffer)
{
	m_IndexData = (u16 *)LockRO();
	UnlockRO();
}


grcIndexBufferDurango::~grcIndexBufferDurango()
{
	m_IndexData = NULL;
}


void grcIndexBufferDurango::ReleaseD3DBuffer()
{
	m_Buffer.ReleaseD3DBufferHack();
}


const u16 *grcIndexBufferDurango::LockRO() const
{
	return (const u16 *)Lock((u32)grcsRead, 0, 0);
}


u16 *grcIndexBufferDurango::LockWO() const
{
	return Lock((u32)grcsWrite, 0, 0);
}


u16 *grcIndexBufferDurango::LockRW() const
{
	return Lock((u32)grcsRead | (u32)grcsWrite, 0, 0);
}


void grcIndexBufferDurango::UnlockRO() const
{
	Unlock((u32)grcsRead);
}


void grcIndexBufferDurango::UnlockWO() const
{
	Unlock((u32)grcsWrite);
}


void grcIndexBufferDurango::UnlockRW() const
{
	Unlock((u32)grcsRead | (u32)grcsWrite);
}


u16 *grcIndexBufferDurango::Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const
{
	void *pUnused = NULL;
	return (u16 *)m_Buffer.Lock(flags, offset, size, (void **)&pUnused);
}


void grcIndexBufferDurango::Unlock(u32 flags) const
{
	m_Buffer.Unlock(flags);
}


const void *grcIndexBufferDurango::GetUnsafeReadPtr() const
{
	return m_IndexData;
}


#if __DECLARESTRUCT
void grcIndexBufferDurango::DeclareStruct(datTypeStruct &s)
{
	grcIndexBuffer::DeclareStruct(s);

	STRUCT_BEGIN(grcIndexBufferDurango);
	STRUCT_FIELD(m_Buffer);
	STRUCT_END();
}
#endif

/*======================================================================================================================================*/
/* grcVertexDurango functions.																											*/
/*======================================================================================================================================*/

#if RSG_DURANGO && !__RESOURCECOMPILER

const u16  *grcIndexBuffer::LockRO() const 
{ 
	return static_cast<const grcIndexBufferDurango *>(this)->LockRO(); 
}

u16  *grcIndexBuffer::LockWO() const 
{	
	return static_cast<const grcIndexBufferDurango *>(this)->LockWO(); 
}

u16  *grcIndexBuffer::LockRW() const 
{	
	return static_cast<const grcIndexBufferDurango *>(this)->LockRW(); 
}

u16  *grcIndexBuffer::Lock(u32 flags, u32 offset, u32 size) const 
{ 
	return static_cast<const grcIndexBufferDurango *>(this)->Lock(flags, offset, size); 
}

void  grcIndexBuffer::UnlockRO() const 
{ 
	return static_cast<const grcIndexBufferDurango *>(this)->UnlockRO(); 
}

void grcIndexBuffer::UnlockWO() const 
{ 
	return static_cast<const grcIndexBufferDurango *>(this)->UnlockWO(); 
}

void grcIndexBuffer::UnlockRW() const 
{ 
	return static_cast<const grcIndexBufferDurango *>(this)->UnlockRW(); 
}

const void *grcIndexBuffer::GetUnsafeReadPtr() const 
{	
	return static_cast<const grcIndexBufferDurango *>(this)->GetUnsafeReadPtr(); 
}

#endif // RSG_DURANGO && !__RESOURCECOMPILER

}; // namespace rage 

#endif	// (RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO
