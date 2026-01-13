//
// grcore/instancebuffer.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "system/new.h"
#include "instancebuffer.h"
#include "device.h"
#include "system/cache.h"
#include "math/amath.h"
#include "grcore/allocscope.h"
#include "grcore/viewport.h"
#include "grcore/grcorespu.h"
#include "profile/timebars.h"

#if __XENON
#include "system/xtl.h"
#elif __D3D11
#include "wrapper_d3d.h"
#include "system/d3d11.h"
#elif RSG_ORBIS
#include "gfxcontext_gnm.h"
#include "wrapper_gnm.h"
#endif

namespace rage {

grcInstanceBufferBasic* grcInstanceBufferBasic::sm_FirstAlloc[MaxFrames];
grcInstanceBufferBasic* grcInstanceBufferBasic::sm_LastAlloc[MaxFrames];
grcInstanceBufferBasic* grcInstanceBufferBasic::sm_FirstFree;
int grcInstanceBuffer::sm_CurrentFrame;
u64 grcInstanceBuffer::sm_FrameCount;
#if __BANK
u32 grcInstanceBuffer::sm_BytesLockedThisFrame=0;
u32 grcInstanceBuffer::sm_BytesLockedMax=0;
#endif // __BANK
grcInstanceBufferBasic::Allocator *grcInstanceBufferBasic::sm_Allocator = NULL;
// grcFenceHandle s_Fences[grcInstanceBuffer::MaxFrames];

grcInstanceBufferBasic::grcInstanceBufferBasic()
{
	CompileTimeAssert((offsetof(grcInstanceBufferBasic, m_Storage) & 0xf) == 0); //Make sure that the storage container is 16-byte aligned.

	m_Count = m_ElemSizeQW = 0;
#if RSG_PC && __D3D11
	u16 registers[NONE_TYPE] = { 0 };
	registers[VS_TYPE] = (u16)INSTANCE_CONSTANT_BUFFER_SLOT;

	m_CB = rage_new rage::grcCBuffer(Size,false);
	m_CB->SetRegisters(registers);
#elif RSG_ORBIS
	m_CB.initAsConstantBuffer(m_Storage,sizeof(m_Storage));
#endif
	m_Next = NULL;
	m_NextAlloc = NULL;
}

grcInstanceBufferBasic::~grcInstanceBufferBasic()
{
#if RSG_PC && __D3D11
	delete m_CB;
	m_CB = NULL;
#endif // RSG_PC && __D3D11
}


void grcInstanceBuffer::AdvanceCurrentFrame()
{
	++sm_FrameCount;

	if (++sm_CurrentFrame == MaxFrames)
		sm_CurrentFrame = 0;

#if __BANK
	if (sm_BytesLockedThisFrame > sm_BytesLockedMax)
		sm_BytesLockedMax = sm_BytesLockedThisFrame;
	sm_BytesLockedThisFrame = 0;
#endif // __BANK
}

void grcInstanceBufferBasic::DefaultNextFrame()
{
	grcInstanceBuffer::AdvanceCurrentFrame();

	// GRCDEVICE.BlockOnFence(s_Fences[grcInstanceBuffer::GetCurrentFrame()]);
	if (sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()]) {
		Assert(!sm_LastAlloc[grcInstanceBuffer::GetCurrentFrame()]->m_NextAlloc);
		// Last (oldest) allocation this frame's next allocation is current first free
		sm_LastAlloc[grcInstanceBuffer::GetCurrentFrame()]->m_NextAlloc = sm_FirstFree;
		// First free is now this frame's first allocation
		sm_FirstFree = sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()];
		sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()] = NULL;
	}
	// s_Fences[grcInstanceBuffer::GetCurrentFrame()] = GRCDEVICE.InsertFence();
}

grcInstanceBuffer* grcInstanceBufferBasic::DefaultCreate() 
{
	grcInstanceBufferBasic *buffer = sm_FirstFree;

	if (!buffer) {
#if RSG_PC && __D3D11
		// This is just a wrapper around an external constant buffer on D3D11, no reason to waste space on alignment.
		buffer = rage_new grcInstanceBufferBasic();
#elif RSG_ORBIS && 0
		buffer = rage_placement_new(allocateVideoSharedMemory(sizeof(grcInstanceBufferBasic),128)) grcInstanceBufferBasic();
#else
		// Note that flushing at a particular address flushes the entire cache line that
		// happens to live there, so to avoid having to flush an extra cache line due to
		// alignment, let's allocate our instance buffers on appropriate boundaries.
		// Don't have to worry about cache on PS3, but we do DMA the instance data so it'll
		// be faster if it's aligned.
		buffer = rage_aligned_new(128) grcInstanceBufferBasic();
#endif
	}
	else
		sm_FirstFree = sm_FirstFree->m_NextAlloc;

	if(buffer) //Make sure allocation succeeded.
	{
		// Remember the oldest allocation for patching the free lists in NextFrame.
		if (!sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()])
			sm_LastAlloc[grcInstanceBuffer::GetCurrentFrame()] = buffer;

		// Patch into allocation list for the frame
		buffer->m_NextAlloc = sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()];
		buffer->m_Next = NULL;
		sm_FirstAlloc[grcInstanceBuffer::GetCurrentFrame()] = buffer;
	}
	return buffer;
}

void grcInstanceBufferBasic::DefaultDestroy(const grcInstanceBuffer *ib)
{
	delete static_cast<const grcInstanceBufferBasic *>(ib);
}

grcInstanceBuffer* (*grcInstanceBuffer::sm_Create)() = grcInstanceBufferBasic::DefaultCreate;
void (*grcInstanceBuffer::sm_Destroy)(const grcInstanceBuffer*) = grcInstanceBufferBasic::DefaultDestroy;
void (*grcInstanceBuffer::sm_NextFrame)() = grcInstanceBufferBasic::DefaultNextFrame;
u32 grcInstanceBuffer::sm_MaxInstancesPerDraw = grcInstanceBuffer::MaxPerDraw;
u32 grcInstanceBuffer::sm_MaxSizeInQW = grcInstanceBuffer::Size;

void grcInstanceBuffer::ResetFactoryFunctors()
{
	sm_Create = grcInstanceBufferBasic::DefaultCreate;
	sm_Destroy = grcInstanceBufferBasic::DefaultDestroy;
	sm_NextFrame = grcInstanceBufferBasic::DefaultNextFrame;
}


void grcInstanceBuffer::SetFactoryFunctors(grcInstanceBuffer *(*pCreate)(), void (*pDestroy)(const grcInstanceBuffer*), void (*pNextFrame)(), u32 MaxInstancesPerDraw, u32 MaxSizeInQW)
{
	sm_Create = pCreate;
	sm_Destroy = pDestroy;
	sm_NextFrame = pNextFrame;
	sm_MaxInstancesPerDraw = MaxInstancesPerDraw;
	sm_MaxSizeInQW = MaxSizeInQW;
}


void grcInstanceBufferBasic::Unlock(size_t finalCount,size_t elemSizeQw)
{
#if __XENON
	WritebackDC(m_Storage,finalCount * 48);
#endif

	// For Orbis and Durango, m_Storage should be mapped as WB/ONION, so no CPU cache flush required here.

	m_Count = (u32)finalCount;
	m_ElemSizeQW = (u32)elemSizeQw;
}

#if RSG_PC && __D3D11
grcCBuffer *grcInstanceBufferBasic::GetConstantBuffers() const
{
	//Make sure this is called on the render thread.
	Assertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() && g_grcCurrentContext, "Warning: Trying to access Instance Constant Buffer on a thread that doesn't own the device! This will probably crash.");

	const u32 LockSize = m_ElemSizeQW * 16 * m_Count;
	void *pLockPtr = m_CB->BeginUpdate(LockSize);
	sysMemCpy(pLockPtr, m_Storage, LockSize);
	m_CB->EndUpdate();

	BANK_ONLY(sm_BytesLockedThisFrame += LockSize);

	return m_CB;
}
#endif // RSG_PC && __D3D11

void grcInstanceBufferBasic::Bind(PS3_ONLY(spuCmd_grcDevice__DrawInstancedPrimitive &cmd))
{
#if __XENON
	GRCDEVICE.GetCurrent()->SetVertexShaderConstantF(INSTANCE_SHADER_CONSTANT_SLOT, reinterpret_cast<const float *>(GetData()), GetElemSizeQW() * GetCount());
#elif __PS3
	cmd.instCount = GetCount();
	cmd.instData = GetData();
	cmd.elemSizeQW = GetElemSizeQW();
#elif RSG_DURANGO
	grcDevice::SetConstantBuffer<VS_TYPE>(INSTANCE_CONSTANT_BUFFER_SLOT, m_Storage);
#elif RSG_PC && __D3D11
	grcCBuffer* pCBuffer = GetConstantBuffers();
	ID3D11Buffer *pD3DBuffer = pCBuffer->GetBuffer();
	g_grcCurrentContext->VSSetConstantBuffers(INSTANCE_CONSTANT_BUFFER_SLOT, 1, &pD3DBuffer);
#elif RSG_ORBIS
	gfxc.setConstantBuffers(grcVertexProgram::GetCurrent()->GetGnmStage(), INSTANCE_CONSTANT_BUFFER_SLOT, 1, GetConstantBuffers());
#endif
}

GRC_ALLOC_SCOPE_DECLARE_GLOBAL(static, s_InstanceBufferAllocScope)

//GPU
//GpuOwn + GpuLoadVertexShaderConstantF4Pointer was not working. Seems to need a fence before the GpuOwn call. So commenting GpuOwn calls out for now, but may look to re-implement if we can fix it.
void grcInstanceBuffer::BeginInstancing()
{
	GRC_ALLOC_SCOPE_PUSH(s_InstanceBufferAllocScope)
	grcViewport::SetCurrentWorldMtx(MATRIX34_TO_MAT34V(M34_IDENTITY));	//Set Identity matrix so that composite matrices can be used for world to projected space transformation.
	//XENON_ONLY(GRCDEVICE.GetCurrent()->GpuOwnVertexShaderConstantF(64, ((3*grcInstanceBuffer::MaxPerDraw)+3)&~3));
}

void grcInstanceBuffer::EndInstancing()
{
	//XENON_ONLY(GRCDEVICE.GetCurrent()->GpuDisownVertexShaderConstantF(64, ((3*grcInstanceBuffer::MaxPerDraw)+3)&~3));
	GRC_ALLOC_SCOPE_POP(s_InstanceBufferAllocScope)
}

void grcInstanceBufferBasic::InitClass()
{
	InitClass(NULL);
}

void grcInstanceBufferBasic::InitClass(Allocator *alloc)
{
	sm_Allocator = alloc;
}

void grcInstanceBufferBasic::ShutdownClass()
{
	// Make sure everything is on the free list.
	for (int i=0; i<MaxFrames; i++)
		NextFrame();
	for (grcInstanceBufferBasic *j=sm_FirstFree; j; ) {
		grcInstanceBufferBasic *k = j->m_NextAlloc;
		delete j;
		j = k;
	}
}

void *grcInstanceBufferBasic::DoAllocate(size_t size, size_t align)
{
	if(sm_Allocator)
	{
		return sm_Allocator->Allocate(size);
	}
	else
	{
		//No allocator set, so lets allocate using default new
		//1st, to handle out of memory issues gracefully, let's check what's available.
		static const size_t sMinAvailableMemory = 1 * 1024 * 1024; //1 meg?
		void *mem = NULL;
		if(Verifyf(	sysMemAllocator::GetCurrent().GetMemoryAvailable() > sMinAvailableMemory,
					"WARNING! Failed to allocate memory for grcInstanceBuffer! Some instanced geometry will not be rendered! [Memory available = %d | Min Required = %d]",
					ORBIS_ONLY((int)) sysMemAllocator::GetCurrent().GetMemoryAvailable(), ORBIS_ONLY((int)) sMinAvailableMemory))
		{
			mem = sysMemAllocator::GetCurrent().Allocate(size, align);
		}

		return mem;
	}
}

void *grcInstanceBufferBasic::operator new(size_t size RAGE_NEW_EXTRA_ARGS_UNUSED)
{
	return DoAllocate(size, __alignof(grcInstanceBufferBasic));
}

void *grcInstanceBufferBasic::operator new(size_t size, size_t align RAGE_NEW_EXTRA_ARGS_UNUSED)
{
	return DoAllocate(size, align);
}

void grcInstanceBufferBasic::operator delete(void *buf)
{
	if(sm_Allocator)
	{
		sm_Allocator->Free(buf);
	}
	else
	{
		sysMemAllocator::GetCurrent().Free(buf);
	}
}

////////////////////////////////////////
//
// grcStaticInstanceBufferList
//

grcStaticInstanceBufferList::grcStaticInstanceBufferList()
: m_NumInstances(0)
, m_StrideQW(0)
, m_MaxCountPerBatch(0)
{
}

void grcStaticInstanceBufferList::Init(u32 numInstances, u32 numRegistersPerInstance)
{
	FatalAssertf(m_IBs.empty(), "grcStaticInstanceBufferList should never be initialized more than once!");

	m_NumInstances = numInstances;
	m_StrideQW = numRegistersPerInstance;
	m_MaxCountPerBatch = Min(grcInstanceBuffer::SizeQW / m_StrideQW, grcInstanceBuffer::MaxPerDraw);

	u32 ibCount = (m_NumInstances / m_MaxCountPerBatch) + (m_NumInstances % m_MaxCountPerBatch > 0); //ib array size.
	m_IBs.Resize(ibCount);

	ib_array::iterator end = m_IBs.end();
	ib_array::iterator iter;
	for(iter = m_IBs.begin(); iter != end; ++iter)
	{
		ib_array::iterator next = iter + 1;
		iter->SetNext(next != end ? &(*next) : NULL);
	}
}


////////////////////////////////////////
//
// grcVecArrayInstanceBufferList
//

grcVecArrayInstanceBufferList::grcVecArrayInstanceBufferList(u32 numRegistersPerInstance)
: m_Current(NULL)
, m_StrideQW(numRegistersPerInstance)
, m_MaxCountPerBatch(Min(grcInstanceBuffer::SizeQW / m_StrideQW, grcInstanceBuffer::MaxPerDraw))
{
	//Assert(m_StrideQW <= grcInstanceBuffer::SizeQW);
	m_ElemSizeQW = m_StrideQW;

	//Set count to max size so that 1st append call will call NewBuffer right away like the typed list.
	m_Count = m_MaxCountPerBatch;
}


Vector4 *grcVecArrayInstanceBufferList::Append()
{
	if (m_Count < m_MaxCountPerBatch)
	{
		++m_Count;
		Vector4 *curr = m_Current;
		m_Current += m_StrideQW;
		return curr;
	}
	else
		return NewBuffer();
}

Vector4 *grcVecArrayInstanceBufferList::NewBuffer()
{
	Assert(m_MaxCountPerBatch != 0);
	if(m_MaxCountPerBatch == 0)
		return NULL; //Tried asserting already... so return NULL to make sure they aren't ignoring the assert and overflowing buffers.

	if(grcInstanceBuffer *newBuffer = grcInstanceBuffer::Create())
	{
		if (m_Buffer) {
			m_Buffer->Unlock(m_Count, m_StrideQW);
			m_Buffer->SetNext(newBuffer);
		}
		else
			m_First = newBuffer;
		m_Buffer = newBuffer;
		Vector4 *curr = static_cast<Vector4 *>(newBuffer->Lock(m_MaxCountPerBatch, m_ElemSizeQW));
		m_Current = curr + m_StrideQW;
		m_Count = 1;
		return curr;
	}

	return NULL; //Allocation failed! Leave state the same so next time we try to allocate again.
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// grcVec4BufferInstanceBufferList
//
grcVec4BufferInstanceBufferList::grcVec4BufferInstanceBufferList(u32 numRegistersPerInstance) : grcInstanceBufferList()
, m_StrideQW(numRegistersPerInstance)
, m_MaxCountPerBatch(Min(grcInstanceBuffer::GetMaxSizeInQW() / m_StrideQW, grcInstanceBuffer::GetMaxInstancesPerDraw()))
{
	m_ElemSizeQW = m_StrideQW;
	m_First = NULL;
	m_Buffer = NULL;
}


Vector4 *grcVec4BufferInstanceBufferList::OpenBuffer(u32 noOfInstances)
{
	Vector4 *pRet = NULL;
	Assert(noOfInstances <= m_MaxCountPerBatch);

	PF_PUSH_TIMEBAR_DETAIL("grcInstanceBuffer::Create()");
	grcInstanceBuffer *newBuffer = grcInstanceBuffer::Create();
	PF_POP_TIMEBAR_DETAIL();
	if(newBuffer)
	{
		if (m_Buffer)
			m_Buffer->SetNext(newBuffer);
		else
			m_First = newBuffer;

		m_Buffer = newBuffer;
		m_Count = noOfInstances;
		PF_PUSH_TIMEBAR_DETAIL("grcInstanceBuffer::Lock()");
		pRet = static_cast<Vector4 *>(newBuffer->Lock(m_Count, m_ElemSizeQW));
		PF_POP_TIMEBAR_DETAIL();
		newBuffer->SetNext(NULL);
	}
	return pRet;
}


void grcVec4BufferInstanceBufferList::CloseBuffer()
{
	Assert(m_Buffer);
	m_Buffer->Unlock(m_Count, m_ElemSizeQW);
}

grcInstanceBuffer *grcVec4BufferInstanceBufferList::GetFirst()
{
	return m_First;
}

} // namespace rage
