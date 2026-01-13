#if RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#include "channel.h"
#include "config.h"
#include "device.h"
#include "buffer_durango.h"
#include "locktypes.h"
#include "wrapper_d3d.h"
#include "system/memory.h"
#include "diag/tracker.h"
#include "string/string.h"
#include "data/resource.h"
#include "data/struct.h"
#include "system/criticalsection.h"
#include "grprofile/pix.h"

#if (RSG_DURANGO && __D3D11_MONO_DRIVER)
#pragma comment(lib,"xg_x.lib")
#else
#pragma comment(lib,"xg.lib")
#endif //RSG_DURANGO && __D3D11_MONO_DRIVER

// TEMP:- Work around until project generation is fixed (see B*1582860).
#if !__D3D11
#include "../3rdparty/durango/xg.h"
#else // !__D3D11
#include <xg.h>
#endif // !__D3D11

#if RSG_DURANGO 
#if _XDK_VER > 9299
#include "system/d3d11.h"
#endif // _XDK_VER > 9299
#endif // RSG_DURANGO

namespace rage 
{

#if ENABLE_PIX_TAGGING
XPARAM(pixDebug);
#endif // ENABLE_PIX_TAGGING

static u32 XGBindFlagFromGrcBindFlag(u32 bindFlag)
{
	u32 ret = 0;

	if(bindFlag & grcBindVertexBuffer)
		ret |= XG_BIND_VERTEX_BUFFER;

	if(bindFlag & grcBindIndexBuffer)
		ret |= XG_BIND_INDEX_BUFFER;

	if(bindFlag & grcBindConstantBuffer)
		ret |= XG_BIND_CONSTANT_BUFFER;

	if(bindFlag & grcBindShaderResource)
		ret |= XG_BIND_SHADER_RESOURCE;

	if(bindFlag & grcBindUnorderedAccess)
		ret |= XG_BIND_UNORDERED_ACCESS;

	return ret;
}

#if __ASSERT
static size_t CalculateResourceAlignment(u32 bindType, size_t size, size_t stride)
{
	XG_BUFFER_DESC xgDesc = {0};
	XG_RESOURCE_LAYOUT xgLayout;

	xgDesc.ByteWidth = size;
	xgDesc.BindFlags = XGBindFlagFromGrcBindFlag(bindType);
	xgDesc.Usage = XG_USAGE_DEFAULT;
	xgDesc.CPUAccessFlags = 0;
	xgDesc.MiscFlags = 0;
	xgDesc.StructureByteStride = stride;
	CHECK_HRESULT(XGComputeBufferLayout(&xgDesc, &xgLayout));

	return xgLayout.BaseAlignmentBytes;
}
#endif // __ASSERT


/*======================================================================================================================================*/
/* grcBufferDurango class.																												*/
/*======================================================================================================================================*/


// Capacity in structures (not bytes).
#define GRC_BUFFER_DURANGO_LOCK_INFO_POOL_SIZE 256

#if RSG_DURANGO
atPool < grcBufferDurango::LOCK_INFO > grcBufferDurango::LOCK_INFO::s_Pool;

#if __BANK
static u32 s_Pool_AllocCount = 0;
static u32 s_Pool_HighWaterMark = 0;
#endif //__BANK


grcBufferDurango::LOCK_INFO::LOCK_INFO()
{
	m_FlushOffsets_CPU[0] = m_FlushOffsets_CPU[1] = 0;
	m_FlushOffsets_GPU[0] = m_FlushOffsets_GPU[1] = 0;
#if __ASSERT
	m_ThreadIdx = -1;
	m_FrameThreadIdxIsValidOn  = ~0u;
	m_FrameLastLockDiscardedOn = ~0u;
	m_FrameLastRendereredFrom  = ~0u;
#endif //__ASSERT
}

grcBufferDurango::LOCK_INFO::~LOCK_INFO() 
{

}

void *grcBufferDurango::LOCK_INFO::operator new(size_t size RAGE_NEW_EXTRA_ARGS_UNUSED)
{
#if __BANK
	if(++s_Pool_AllocCount > s_Pool_HighWaterMark)	
	{
		s_Pool_HighWaterMark = s_Pool_AllocCount;

		// LDS DMC TEMP:-
		grcWarningf("grcBufferDurango::LOCK_INFO::operator new()...High water mark %d.\n", s_Pool_HighWaterMark);

		if(s_Pool_HighWaterMark >= (u32)(0.9f*(float)GRC_BUFFER_DURANGO_LOCK_INFO_POOL_SIZE))
			grcWarningf("grcBufferDurango::LOCK_INFO::operator new()....High water mark %d of %d near capacity.\n", s_Pool_HighWaterMark, GRC_BUFFER_DURANGO_LOCK_INFO_POOL_SIZE);
	}
#endif //__BANK

	if(!s_Pool.IsInitialized())
		s_Pool.Init(GRC_BUFFER_DURANGO_LOCK_INFO_POOL_SIZE);

	return s_Pool.New(size);
}


void grcBufferDurango::LOCK_INFO::operator delete(void *buf)
{
	Assertf(s_Pool.IsInitialized(), "grcBufferDurango::LOCK_INFO::operator delete()...Pool not initialised.");
	Assertf(s_Pool.IsInPool(buf), "grcBufferDurango::LOCK_INFO::operator delete()...Element not in pool.");
#if __BANK
	s_Pool_AllocCount--;
#endif //__BANK
	s_Pool.Delete(buf);
}


#if __ASSERT
void grcBufferDurango::LOCK_INFO::OnGetD3DBuffer()
{
	m_FrameLastRendereredFrom = GRCDEVICE.GetFrameCounter();
}

#if MULTIPLE_RENDER_THREADS
#if MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD
#define INDEX_FROM_G_RENDER_THREAD_INDEX 0 // Device owning thread thread and sub render thread 0 (which is the main render thread) share slot 0.
#else
#define INDEX_FROM_G_RENDER_THREAD_INDEX 1 // Device owning thread uses slot 0, sub-render threads use 1 onwards.
#endif
#define INDEX_FROM_G_RENDER_THREAD_INDEX 0 // g_RenderThreadIdx = 0 always.
#endif

static int GetThreadIdx()
{
#if !MULTIPLE_RENDER_THREADS
	return 0;
#else // !MULTIPLE_RENDER_THREADS
	if(g_IsSubRenderThread)
		return g_RenderThreadIndex + INDEX_FROM_G_RENDER_THREAD_INDEX;
	else
		// Use the entry for the device owning thread (usually the main render thread).
		// g_RenderThreadIndex won`t be valid if called from game thread during safe area for example, but g_IsSubRenderThread is.
		return 0;
#endif // !MULTIPLE_RENDER_THREADS
}

void grcBufferDurango::LOCK_INFO::OnLock(int lockFlags)
{
	const u32 frameCounter = GRCDEVICE.GetFrameCounter();
	if(m_FrameThreadIdxIsValidOn != frameCounter)
	{
		m_FrameThreadIdxIsValidOn = frameCounter;
		m_ThreadIdx = GetThreadIdx();
	}
	else
	{
		grcAssertf(m_ThreadIdx == GetThreadIdx(), "grcBufferDurango::LOCK_INFO::OnLock()...Buffer used by multiple threads.");
	}

	if(lockFlags & grcsDiscard)
	{
		grcAssertf(m_FrameLastLockDiscardedOn != frameCounter, "grcBufferDurango::LOCK_INFO::OnLock()...Can`t write discard more than 1 per frame.");
		m_FrameLastLockDiscardedOn = frameCounter;
	}else
	if(m_FrameLastRendereredFrom == frameCounter)
	{
		grcAssertf((lockFlags & ((u32)grcsWrite | (u32)grcsDiscard)) == 0, "grcBufferDurango::LOCK_INFO::OnLock()...Can`t write to a buffer after rendering draw from it.");
	}
}
#endif //__ASSERT

#endif // RSG_DURANGO

//--------------------------------------------------------------------------------------------------//

grcBufferDurango::grcBufferDurango()
: grcOrbisDurangoBufferBase()
{
}


grcBufferDurango::grcBufferDurango(size_t size, size_t stride, u32 bindType, void *pPreAllocatedMemory)
: grcOrbisDurangoBufferBase()
{
	Initialise(size, stride, bindType, pPreAllocatedMemory);
}


grcBufferDurango::grcBufferDurango(class datResource& rsc, u32 bindType) 
: grcOrbisDurangoBufferBase(rsc)
{
#if RSG_DURANGO

#if ENABLE_PIX_TAGGING
	if(PARAM_pixDebug.Get())
		Initialise(m_Size, 0, grcBindIndexBuffer | grcBindVertexBuffer, m_pGraphicsMemory); // Make a D3D buffer object per resource as it`s easier to view contents in PIX.
	else
#endif // ENABLE_PIX_TAGGING
	{
		(void*) bindType;
		GetUserMemory()->m_pD3DBuffer = NULL;
	}
#else
	(void)bindType;	
#endif // RSG_DURANGO
}


grcBufferDurango::~grcBufferDurango()
{
	CleanUp();
}


void grcBufferDurango::Initialise(size_t size, size_t stride, u32 bindType, void *pPreAllocatedMemory, u32 miscFlags, bool flushBeforeUse)
{
	Assertf(((bindType & (u32)grcBindConstantBuffer) == grcBindConstantBuffer) || ((bindType & (u32)grcBindConstantBuffer) == 0), "grcBufferDurango::Initialise()...Can`t combine grcBindConstantBuffer with other bind types.");
	m_Flags = (bindType & (u32)grcBindConstantBuffer) ? GRC_BUFFER_DURANGO_IS_CONSTANT_BUFFER : 0;
	m_Size = size;
	m_pGraphicsMemory = NULL;

#if RSG_DURANGO
	GetUserMemory()->m_pD3DBuffer = NULL;

	if(pPreAllocatedMemory == NULL)
	{
		XG_BUFFER_DESC xgDesc = {0};
		xgDesc.ByteWidth = size;
		xgDesc.BindFlags = XGBindFlagFromGrcBindFlag(bindType);
		xgDesc.Usage = XG_USAGE_DEFAULT;
		xgDesc.CPUAccessFlags = xgDesc.StructureByteStride = 0;
		xgDesc.MiscFlags = miscFlags;

		if(bindType & grcBindUnorderedAccess)
		{
			xgDesc.StructureByteStride = stride;
		}

		XG_RESOURCE_LAYOUT xgLayout;
		CHECK_HRESULT(XGComputeBufferLayout(&xgDesc, &xgLayout));

		m_Flags |= GRC_BUFFER_DURANGO_OWNS_GRAPHICS_MEM;
		int memSize = (xgLayout.SizeBytes + 4095) & ~4095;
		int memAlignment = (xgLayout.BaseAlignmentBytes + 4095) & ~4095;
		m_pGraphicsMemory = sysMemPhysicalAllocate(memSize, memAlignment, PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
		TrapZ((uptr)m_pGraphicsMemory);
		CreateD3DResources(bindType, stride, miscFlags, flushBeforeUse);
	}
	else
	{
		m_pGraphicsMemory = pPreAllocatedMemory;

		// We can only "translate" vertex, index and constant buffers.
		// NOTE: -pixDebug can cause us to run out of memory in CreateD3DResources() below
		// as it means we're (intentionally) not deleting stuff to help PIX debugging.
		if((u32)bindType & ~((u32)grcBindVertexBuffer | (u32)grcBindIndexBuffer | (u32)grcBindConstantBuffer) PIX_TAGGING_ONLY(|| PARAM_pixDebug.Get()))
		{
		#if __ASSERT
			size_t alignment = CalculateResourceAlignment(bindType, size, stride);
			void *pAligned = (void *)(((size_t)pPreAllocatedMemory + (alignment-1)) & ~(alignment-1));
			grcAssertf(pPreAllocatedMemory == pAligned, "grcBufferDurango::grcBufferDurango()...Bad alignment!"); 
		#endif //__ASSERT
			CreateD3DResources(bindType, stride, miscFlags, flushBeforeUse);
		}
	}
#else // RSG_DURANGO
	(void)pPreAllocatedMemory;
	(void)stride;
	(void)miscFlags;
	(void)flushBeforeUse;
	XG_BUFFER_DESC xgDesc = {0};
	xgDesc.ByteWidth = size;
	xgDesc.BindFlags = XGBindFlagFromGrcBindFlag(bindType);
	xgDesc.Usage = XG_USAGE_DEFAULT;
	xgDesc.CPUAccessFlags = xgDesc.MiscFlags = xgDesc.StructureByteStride = 0;

	XG_RESOURCE_LAYOUT xgLayout;
	CHECK_HRESULT(XGComputeBufferLayout(&xgDesc, &xgLayout));
	m_pGraphicsMemory = (u8 *)physical_new(xgLayout.SizeBytes, xgLayout.BaseAlignmentBytes);
#endif // RSG_DURANGO
}


void grcBufferDurango::CleanUp()
{
	ReleaseD3DBufferHack();

#if RSG_DURANGO
	if(m_pGraphicsMemory && (m_Flags & GRC_BUFFER_DURANGO_OWNS_GRAPHICS_MEM))
		sysMemPhysicalFree(m_pGraphicsMemory);
#else // RSG_DURANGO
	if(m_pGraphicsMemory)
		physical_delete(m_pGraphicsMemory);
#endif // RSG_DURANGO

	m_pGraphicsMemory = NULL;
}


void grcBufferDurango::CreateD3DResources(u32 bindType, u32 stride, u32 miscFlags, bool flushBeforeUse)
{
	ReleaseD3DBufferHack();

#if RSG_DURANGO
	// Force a GPU flush before use.
	if(flushBeforeUse)
		m_Flags |= GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE;

	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth = m_Size;
	desc.BindFlags = XGBindFlagFromGrcBindFlag(bindType);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = miscFlags;
	desc.StructureByteStride = 0;

	if(bindType & grcBindUnorderedAccess)
	{
		desc.StructureByteStride = stride;
	}
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreatePlacementBuffer(&desc, m_pGraphicsMemory, (ID3D11Buffer**)&GetUserMemory()->m_pD3DBuffer));

#else
	(void)bindType;
	(void)stride;
	(void)miscFlags;
	(void)flushBeforeUse;
#endif // RSG_DURANGO
}


void grcBufferDurango::ReleaseD3DBufferHack()
{
#if RSG_DURANGO
	if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
	{
		GetUserMemory()->m_pD3DBuffer = FreeLockInfo();
	}
	if(GetUserMemory()->m_pD3DBuffer) 
		GetUserMemory()->m_pD3DBuffer->Release();
	GetUserMemory()->m_pD3DBuffer = NULL;
#endif // RSG_DURANGO
}


void *grcBufferDurango::Lock(u32 flags, u32 offset, u32 size, void **ppLockBase) const
{
#if RSG_DURANGO
	UpdateFlushRange(flags, offset, size);

#if __ASSERT
	if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
		GetUserMemory()->m_pLockInfo->OnLock(flags);
#endif // __ASSERT

	if(ppLockBase)
		*ppLockBase = m_pGraphicsMemory;
	return (void *)((u8 *)m_pGraphicsMemory + offset);
#else // RSG_DURANGO
	(void)size;
	(void)flags;
	if(ppLockBase)
		*ppLockBase = m_pGraphicsMemory;
	return (void *)((u8 *)m_pGraphicsMemory + offset);
#endif // RSG_DURANGO
}


void grcBufferDurango::Unlock(u32 flags) const
{
	PerformCPUFlush(flags);
}


void grcBufferDurango::Unlock(u32 flags, u32 offset, u32 size) const
{
	UpdateFlushRange(flags, offset, size);
	PerformCPUFlush(flags);
}


void *grcBufferDurango::LockAll()
{
	return (void *)m_pGraphicsMemory;
}


void grcBufferDurango::UnlockAll()
{
#if RSG_DURANGO
	D3DFlushCpuCache(m_pGraphicsMemory, m_Size);
	// Force a GPU flush before use.
	m_Flags |= GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE;
#endif // RSG_DURANGO
}


void grcBufferDurango::UpdateFlushRange(u32 flags, u32 offset, u32 size) const
{
#if RSG_DURANGO
	if(flags == 0)
		flags = (u32)grcsWrite | (u32)grcsRead;

	if((flags & ((u32)grcsWrite | (u32)grcsDiscard)) != 0)
	{
		size_t low = offset;
		size_t high = (size == 0 && offset == 0) ? m_Size : size + offset;

		// Do we just have a full buffers worth ?
		if(low == 0 && high == m_Size)
			m_Flags |= (GRC_BUFFER_DURANGO_WHOLE_BUFFER_LOCKED | GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE); // Don`t bother with LockInfo...
		else
		{
			LOCK_INFO *pLockInfo = GetLockInfo(); // Allocates LOCK_INFO if needs be.

			// Record the CPU flush area.
			pLockInfo->m_FlushOffsets_CPU[0] = low;
			pLockInfo->m_FlushOffsets_CPU[1] = high;

			// Maintain the GPU area to flush.
			if(low < pLockInfo->m_FlushOffsets_GPU[0])
				pLockInfo->m_FlushOffsets_GPU[0] = low;
			if(high > pLockInfo->m_FlushOffsets_GPU[1])
				pLockInfo->m_FlushOffsets_GPU[1] = high;
		}
	}
#else
	(void)flags;
	(void)offset;
	(void)size;
#endif // RSG_DURANGO
}


void grcBufferDurango::PerformCPUFlush(u32 flags) const
{
#if RSG_DURANGO
	if(flags == 0)
		flags = (u32)grcsWrite | (u32)grcsRead;

	if(flags & ((u32)grcsWrite | (u32)grcsDiscard))
	{
		if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
		{
			if(m_Flags & GRC_BUFFER_DURANGO_WHOLE_BUFFER_LOCKED)
			{
				D3DFlushCpuCache((void *)m_pGraphicsMemory, m_Size);
				m_Flags &= ~GRC_BUFFER_DURANGO_WHOLE_BUFFER_LOCKED;
			}
			else
			{
				LOCK_INFO *pLockInfo = GetUserMemory()->m_pLockInfo;
				D3DFlushCpuCache((void *)((u8*)m_pGraphicsMemory + pLockInfo->m_FlushOffsets_CPU[0]), pLockInfo->m_FlushOffsets_CPU[1] - pLockInfo->m_FlushOffsets_CPU[0]);
				pLockInfo->m_FlushOffsets_CPU[0] = 0;
				pLockInfo->m_FlushOffsets_CPU[1] = 0;
			}
		}
		else
		{
			// Assume a whole buffers worth.
			D3DFlushCpuCache((void *)m_pGraphicsMemory, m_Size); 
			m_Flags &= ~GRC_BUFFER_DURANGO_WHOLE_BUFFER_LOCKED;
		}
	}
#else
	(void)flags;
#endif // RSG_DURANGO
}


ID3D11Buffer *grcBufferDurango::GetD3DBuffer() const
{
#if RSG_DURANGO
	grcAssertf(GetUserMemory()->m_pD3DBuffer, "grcBufferDurango::GetD3DBuffer()...Encountered a buffer without D3D resources.\n");

	ID3D11Buffer *pRet;
	if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
		pRet = GetUserMemory()->m_pLockInfo->m_pD3DBuffer;
	else
		pRet = GetUserMemory()->m_pD3DBuffer;

	//AJG-NOTE: This prevents me from creating device resources on the update thread without locking the device context... so I enable this by skipping the renderthread specific calls that don't
	//			really need to happen just to create unordered access/shader resource views since the buffer isn't actually getting used right now. And since I'm not updating the flag, the flush
	//			will still happen on the render thread when the buffer resource is actually going to be used.
	if(!GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread())
		return pRet;

#if __ASSERT
	if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
		GetUserMemory()->m_pLockInfo->OnGetD3DBuffer();
#endif // __ASSERT

	if(m_Flags & GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE)
	{
		m_Flags &= ~GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE;
		g_grcCurrentContext->FlushGpuCacheRange(0, (u8 *)m_pGraphicsMemory, m_Size);
	}
	else if(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO)
	{
		LOCK_INFO *pLockInfo = GetUserMemory()->m_pLockInfo;

		if(pLockInfo->m_FlushOffsets_GPU[1] > pLockInfo->m_FlushOffsets_GPU[0])
		{
			g_grcCurrentContext->FlushGpuCacheRange(0,(u8 *)m_pGraphicsMemory + pLockInfo->m_FlushOffsets_GPU[0], pLockInfo->m_FlushOffsets_GPU[1] - pLockInfo->m_FlushOffsets_GPU[0]);
			pLockInfo->m_FlushOffsets_GPU[0] = m_Size;
			pLockInfo->m_FlushOffsets_GPU[1] = 0;
		}
	}
	return pRet;
#else // RSG_DURANGO
	Assertf(0, "grcVertexBufferDurango::GetD3DBuffer()...Not implemented!\n");
	return NULL;
#endif // RSG_DURANGO
}


void grcBufferDurango::Flush()
{
#if RSG_DURANGO
	D3DFlushCpuCache(m_pGraphicsMemory, m_Size);
	m_Flags |= GRC_BUFFER_DURANGO_GPU_FLUSH_BEFORE_USE;
#endif // RSG_DURANGO
}


size_t grcBufferDurango::GetSize()
{
	return m_Size;
}


bool grcBufferDurango::IsValid() const
{
	return true;
}

#if RSG_DURANGO
grcBufferDurango::LOCK_INFO *grcBufferDurango::GetLockInfo() const
{
	if(!(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO))
	{
		ID3D11Buffer *pID3D11Buffer = GetUserMemory()->m_pD3DBuffer;
		GetUserMemory()->m_pLockInfo = rage_new LOCK_INFO;
		GetUserMemory()->m_pLockInfo->m_pD3DBuffer = pID3D11Buffer;
		m_Flags |= GRC_BUFFER_DURANGO_HAS_LOCK_INFO;
	}
	return GetUserMemory()->m_pLockInfo;
}


ID3D11Buffer *grcBufferDurango::FreeLockInfo()
{
	grcAssertf(m_Flags & GRC_BUFFER_DURANGO_HAS_LOCK_INFO, "grcBufferDurango::FreeLockInfo()...Expected LOCK INFO.");
	ID3D11Buffer *pRet = GetUserMemory()->m_pLockInfo->m_pD3DBuffer;
	delete GetUserMemory()->m_pLockInfo;
	return pRet;
}


#endif // RSG_DURANGO


#if __DECLARESTRUCT
void grcBufferDurango::DeclareStruct(datTypeStruct &s)
{
	grcOrbisDurangoBufferBase::DeclareStruct(s);

	STRUCT_BEGIN(grcBufferDurango);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


/*======================================================================================================================================*/
/* grcBufferResourceDurango class.																										*/
/*======================================================================================================================================*/

#if RSG_DURANGO

grcBufferResourceDurango::grcBufferResourceDurango() : grcBufferDurango()
{
	m_SRView = NULL;
	m_UAView = NULL;
}


grcBufferResourceDurango::~grcBufferResourceDurango()
{
	CleanUp();
}


void grcBufferResourceDurango::Initialise(u32 Count, u32 Stride, u32 BindType, void *pPreAllocatedMem, u32 miscFlags, u32 UAVBufferFlags, bool flushBeforeUse)
{
	// | grcBindShaderResource to ensure we don`t use address translation.
	grcBufferDurango::Initialise(Count * Stride, Stride, BindType | grcBindShaderResource, pPreAllocatedMem, miscFlags, flushBeforeUse);

	if(BindType & grcBindShaderResource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.ElementWidth = Count;
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateShaderResourceView(GetD3DBuffer(), &srvDesc, (ID3D11ShaderResourceView**)&m_SRView));
	}
	if(BindType & grcBindUnorderedAccess)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = UAVBufferFlags;
		uavDesc.Buffer.NumElements = Count;
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateUnorderedAccessView(GetD3DBuffer(), &uavDesc, (ID3D11UnorderedAccessView**)&m_UAView));
	}
}

void grcBufferResourceDurango::CleanUp()
{
	if(m_SRView)
	{
		m_SRView->Release();
		m_SRView = NULL;
	}
	if(m_UAView)
	{
		m_UAView->Release();
		m_UAView = NULL;
	}
}

#endif // RSG_DURANGO

}; // namespace rage

#endif // RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)
