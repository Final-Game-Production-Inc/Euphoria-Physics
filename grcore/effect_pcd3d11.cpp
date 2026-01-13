//
// grcore/effect_pcd3d11.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "config.h"
#if RSG_PC && __D3D11

#include "effect.h"
#include "effect.inl"

#include "channel.h"
#include "stateblock_internal.h"


#include "system/xtl.h"
#define DX_ONLY_INCLUDE_ONCE

#if __D3D11_MONO_DRIVER
#include <D3D11Shader_x.h>
#include <D3DCompiler_x.h>
#else	// __D3D11_MONO_DRIVER
#include <D3D11Shader.h>
#include <D3DCompiler.h>
#endif	// __D3D11_MONO_DRIVER

#include "file/asset.h"
#include "file/device.h"
#include "file/stream.h"
#include "profile/element.h"
#include "profile/timebars.h"
#include "string/stringhash.h"
#include "system/cache.h"
#include "system/platform.h"
#include "system/magicnumber.h"
#include "system/membarrier.h"
#include "vector/vector4.h"
#include "atl/array_struct.h"
#include "atl/array.h"

#include "effect_values.h"
#include "texture.h"
#include "system/nelem.h"
#include "system/codecheck.h"
#include "grcore/channel.h"
#include "grcore/image.h"
#if __BANK
#include "bank/bank.h"
#endif

#include "buffer_d3d11.h"
#include "device.h"
#include "system/xtl.h"
#include "grcore/effect_internal.h"
#include "resourcecache.h"
#include "grcore/stateblock.h"
#include "grcore/d3dwrapper.h"
#include "grcore/wrapper_d3d.h"
#include "shaderlib/rage_constants.h"

#define SURFACE IDirect3DSurface9
#define TEXTURE IDirect3DBaseTexture9

#define STALL_DETECTION	0
#if STALL_DETECTION
#define STALL_ONLY_RENDERTHREAD(x) x
#define STALL_TIME 0.05f
#include "system/timer.h"
#endif

namespace rage {
#if SUPPORT_UAV
//NOTE: NVIDIA debug drivers didn't like us setting UAVs to NULL, so NVIDIA suggested that we use dummy NULL buffers to avoid any possible issues.
static atRangeArray<grcBufferUAV *, EFFECT_UAV_BUFFER_COUNT> sEffectGnm_NullUAVRawBuffer(NULL);
static atRangeArray<grcBufferUAV *, EFFECT_UAV_BUFFER_COUNT> sEffectGnm_NullUAVStructuredBuffer(NULL);
#endif // SUPPORT_UAV

#if TRACK_CONSTANT_BUFFER_CHANGES
bool grcEffect::sm_TrackConstantBufferUsage = false;
bool grcEffect::sm_PrintConstantBufferUsage = false;
bool grcEffect::sm_TrackConstantBufferResetFrame = true;
bool grcEffect::sm_PrintConstantBufferUsageOnlyChanged = true;
#endif

// Set this flag to be enabled for all but final for now. Until we can do
// sufficient testing that we can be sure we're not seeing rendering errors.
// The worry being that if a UAV slot doesn't have something in it and we're writing
// to it in the shader, then all kinds of undefined behaviour could happen.
bool grcEffect::sm_SetUAVNull = true;

extern DECLARE_MTR_THREAD grcVertexShader *s_VertexShader;
extern DECLARE_MTR_THREAD grcPixelShader *s_PixelShader;
extern DECLARE_MTR_THREAD grcComputeShader *s_ComputeShader;
extern DECLARE_MTR_THREAD grcDomainShader *s_DomainShader;
extern DECLARE_MTR_THREAD grcGeometryShader *s_GeometryShader;
extern DECLARE_MTR_THREAD grcHullShader *s_HullShader;

extern ID3D11SamplerState **g_SamplerStates11;

EXT_PF_COUNTER(CBDynamicMaps);
EXT_PF_COUNTER(CBDynamicMapBytes);

/************************************************************************************************************************/

u32 DECLARE_MTR_THREAD grcCBuffer::sm_DeferredContextCount = 0;

grcCBuffer::grcCBuffer() : Size(0), NameHash(0)
{
	memset(Registers,0,sizeof(Registers));
	Name = "unnamed";
	NameHash = atHashString(Name);
	m_pAlignedBackingStore = NULL;
	m_pAllocatedBackingStore = NULL;
	m_BufferStride = 0;
#if TRACK_CONSTANT_BUFFER_CHANGES
	m_LockCount = 0;
#endif
	m_pBuffer = NULL;
}


grcCBuffer::grcCBuffer(u32 size, bool directLockOnly) :Size(size) ,NameHash(0)
{
	memset(Registers,0,sizeof(Registers));
	Name = "unnamed";
	NameHash = atHashString(Name);
	m_pAlignedBackingStore = NULL;
	m_pAllocatedBackingStore = NULL;
	m_BufferStride = 0;
#if TRACK_CONSTANT_BUFFER_CHANGES
	m_LockCount = 0;
#endif
	m_pBuffer = NULL;
	Init_Internal(directLockOnly);
}


grcCBuffer::~grcCBuffer()
{
	Destroy();
}


void grcCBuffer::Init(bool)
{
	Init_Internal(false);
}

void grcCBuffer::Init_Internal(bool directLockOnly)
{
	CalculateBufferStride(directLockOnly);

	// Allocate the backing store.
	AllocateBackingStore();

	// Create the constant buffer.
	GRCDEVICE.CreateShaderConstantBuffer(Size,&(m_pBuffer) NOTFINAL_ONLY(, Name));

	for(int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
	{
		ResetFlagThreadIdx(i, GRC_CBUFFER_DIRTY_FLAG);
		ResetFlagThreadIdx(i, GRC_CBUFFER_LOCKED_DIRECT);
		SetFlagThreadIdx(i, GRC_CBUFFER_FIRST_USE);
	}
}

void grcCBuffer::Destroy()
{
	// Destroy the constant buffer.
	if(m_pBuffer)
	{
		SAFE_RELEASE_RESOURCE(m_pBuffer);
	}

	// Free up backing store.
	FreeBackingStore();
}

void grcCBuffer::Load(fiStream &S)
{
	//S.ReadInt(&Count, 1);
	S.ReadInt(&Size,1);
	S.ReadShort(Registers,6);

	const u32 maxSize = 4096*16;
	AssertMsg(Size < maxSize,"Constant buffer is too large. Clamping to 4096 slots");
	AssertMsg((Size % 16) == 0,"Warning! Constant buffer maps a partial slot");
	Size = Size > maxSize ? maxSize : Size;

	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer,count);
	Name = buffer;

	// Displayf("CBuffer::Load - %s, register %d, size %d", buffer, Register, Size);
	NameHash = atStringHash(Name);
}

void grcCBuffer::operator=(const grcCBuffer &rhs)
{
	Destroy();
	Size = rhs.Size;
	memcpy(Registers,rhs.Registers,sizeof(Registers));
	NameHash = rhs.NameHash;
	Name = rhs.Name;
	m_pBuffer = rhs.m_pBuffer;
	m_BufferStride = rhs.m_BufferStride;

	if(rhs.m_pAlignedBackingStore)
	{
		u32 noOfCopies = 1;
		AllocateBackingStore();
		memcpy(m_pAlignedBackingStore, rhs.m_pAlignedBackingStore, GetBackingStoreSize());
		noOfCopies *= NUMBER_OF_RENDER_THREADS;

		u32 i;

		// Clear lock/dirty flags.
		for(i=0; i<noOfCopies; i++)
		{
			ResetFlagThreadIdx(i, GRC_CBUFFER_DIRTY_FLAG);
			ResetFlagThreadIdx(i, GRC_CBUFFER_LOCKED_DIRECT);
		}
	}
	if(m_pBuffer)
		m_pBuffer->AddRef();
}


// Dirty flag functions.
void grcCBuffer::SetDirty(u8 uFlag)
{
	(void)uFlag;
	SetFlagThreadIdx(g_RenderThreadIndex, GRC_CBUFFER_DIRTY_FLAG);
}

u8 grcCBuffer::GetDirty()
{
	return GetFlagThreadIdx(g_RenderThreadIndex, GRC_CBUFFER_DIRTY_FLAG);
}

void grcCBuffer::ResetDirty()
{
	ResetFlagThreadIdx(g_RenderThreadIndex, GRC_CBUFFER_DIRTY_FLAG);
}


// Locked flags functions.
bool grcCBuffer::IsLocked(u32 threadIdx) const
{
	return (GetFlagThreadIdx(threadIdx, GRC_CBUFFER_LOCKED_DIRECT) != 0);
}


// Data access functions.
#if MULTIPLE_RENDER_THREADS == 0
void* grcCBuffer::GetDataPtr()
{
	return GetDataPtrForThread_Write(0);
}
#endif

void* grcCBuffer::GetDataPtr_ReadOnly()
{
	return GetDataPtrForThread_ReadOnly(g_RenderThreadIndex);
}

void* grcCBuffer::GetDataPtrForThread(u32 threadIdx)
{
	return GetDataPtrForThread_Write(threadIdx);
}

bool grcCBuffer::Unlock()
{
	// Update the GPU version.
	UpdateD3DBuffer();
	return true;
}

void *grcCBuffer::LockDirect(u32 numBytes)
{
	(void)numBytes;
	AssertMsg(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcCBuffer::LockDirect()...Can only be called from device owning thread");
	grcAssertf(IsLocked() == false, "grcCBuffer::LockDirect()...Already locked.");

	void * pDataBuffer = NULL;

	D3D11_MAPPED_SUBRESOURCE oMappedRes;
	CHECK_HRESULT(g_grcCurrentContext->Map((ID3D11Buffer *)m_pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, (D3D11_MAPPED_SUBRESOURCE*)&oMappedRes));
	pDataBuffer = oMappedRes.pData;

	PF_INCREMENT(CBDynamicMaps);
	PF_INCREMENTBY(CBDynamicMapBytes, Size);

#if MULTIPLE_RENDER_THREADS > 0
	if(g_IsSubRenderThread)
	{
		// Make sure we don`t perform the GPU version update when this buffer is bound.
		BUFFER_HEADER *pHeader = GetBufferHeader(g_RenderThreadIndex);
		pHeader->m_LastDeferredContextCount = sm_DeferredContextCount;
	}
#endif
	SetFlagThreadIdx(g_RenderThreadIndex, GRC_CBUFFER_LOCKED_DIRECT);
	ResetFlagThreadIdx(g_RenderThreadIndex, GRC_CBUFFER_FIRST_USE);

#if TRACK_CONSTANT_BUFFER_CHANGES
	if (grcEffect::sm_TrackConstantBufferUsage)
		m_LockCount++;
#endif

	return pDataBuffer;
}

void grcCBuffer::UnlockDirect()
{
	AssertMsg(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcCBuffer::UnlockDirect()...Can only be called from device owning thread");
	grcAssertf(IsLocked() == true, "grcCBuffer::UnlockDirect()...");

	g_grcCurrentContext->Unmap((ID3D11Buffer *)m_pBuffer, 0);

	BUFFER_HEADER *pHeader = GetBufferHeader(g_RenderThreadIndex);
	pHeader->m_Flags[GRC_CBUFFER_DIRTY_FLAG] = 0;
	pHeader->m_Flags[GRC_CBUFFER_LOCKED_DIRECT] = 0;
#if MULTIPLE_RENDER_THREADS > 0
	pHeader->m_LastDeferredContextCount = sm_DeferredContextCount;
#endif
}


grcBuffer *grcCBuffer::GetBuffer(DEV_ONLY(u8 lockflag))
{
	DEV_ONLY((void)lockflag);
	return m_pBuffer;
}



// Internal dirty flag functions.
u8 grcCBuffer::GetFlagThreadIdx(u32 threadIdx, int flag) const
{
	BUFFER_HEADER *pHeader = GetBufferHeader(threadIdx);
	return (u8)pHeader->m_Flags[flag];
}

void grcCBuffer::SetFlagThreadIdx(u32 threadIdx, int flag)
{
	BUFFER_HEADER *pHeader = GetBufferHeader(threadIdx);
	pHeader->m_Flags[flag] = 0x1;
}

void grcCBuffer::ResetFlagThreadIdx(u32 threadIdx, int flag)
{
	BUFFER_HEADER *pHeader = GetBufferHeader(threadIdx);
	pHeader->m_Flags[flag] = 0x0;
}


// Data pointer functions.
void *grcCBuffer::GetDataPtrForThread_Write(u32 threadIdx)
{
	grcAssertf(m_BufferStride != (GRC_CBUFFER_ALIGN(sizeof(BUFFER_HEADER), GRC_BUFFER_ELEMENT_ALIGN)), "grcCBuffer::GetDataPtrForThread_Write()...Lock direct type only.");
	grcAssertf(IsLocked(threadIdx) == false, "grcCBuffer::GetDataPtrForThread_Write()...Buffer is locked by LockDirect().");
	SetFlagThreadIdx(threadIdx, GRC_CBUFFER_DIRTY_FLAG);
	char *pRet = (char *)GetBufferHeader(threadIdx) + GRC_CBUFFER_ALIGN(sizeof(BUFFER_HEADER), GRC_BUFFER_ELEMENT_ALIGN);
	return pRet;
}

void* grcCBuffer::GetDataPtrForThread_ReadOnly(u32 threadIdx)
{
	grcAssertf(m_BufferStride != (GRC_CBUFFER_ALIGN(sizeof(BUFFER_HEADER), GRC_BUFFER_ELEMENT_ALIGN)), "grcCBuffer::GetDataPtrForThread_Write()...Lock direct type only.");
	grcAssertf(IsLocked(threadIdx) == false, "grcCBuffer::GetDataPtrForThread_Write()...Buffer is locked by LockDirect().");
	char *pRet = (char *)GetBufferHeader(threadIdx) + GRC_CBUFFER_ALIGN(sizeof(BUFFER_HEADER), GRC_BUFFER_ELEMENT_ALIGN);
	return pRet;
}


// Parameter setting functions.
bool grcCBuffer::SetParameter_ThreadIdx(int threadIdx, const float *data, int count, int offset, int size)
{
	char *pDest_ReadOnly = (char *)GetDataPtrForThread_ReadOnly(threadIdx);
	pDest_ReadOnly += offset;

	if(memcmp(pDest_ReadOnly, data, size*count))
	{
		char *pConstData = (char *)GetDataPtrForThread_Write(threadIdx);
		pConstData += offset;
		memcpy(pConstData,data,size*count);

		return true;
	}
	return false;
}


bool grcCBuffer::SetParameter(const float *data, int count, int offset, int size)
{
	bool ret = false;

#if MULTIPLE_RENDER_THREADS > 1
	if (!g_IsSubRenderThread)
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			ret |= SetParameter_ThreadIdx(i, data, count, offset, size);
	else
		ret = SetParameter_ThreadIdx(g_RenderThreadIndex, data, count, offset, size);
#else
	ret = SetParameter_ThreadIdx(0, data, count, offset, size);
#endif
	return ret;
}


// Backing store functions.
void grcCBuffer::CalculateBufferStride(bool directLockOnly)
{
	// Align start of data to required alignment.
	u32 headerSize = GRC_CBUFFER_ALIGN(sizeof(BUFFER_HEADER), GRC_BUFFER_ELEMENT_ALIGN);

	// Align whole buffer to cache line size.
	if(directLockOnly == false)
		m_BufferStride = GRC_CBUFFER_ALIGN(Size + headerSize, GRC_CBUFFER_CACHE_LINE_SIZE);
	else
		m_BufferStride = headerSize;
}


void grcCBuffer::AllocateBackingStore()
{
	u32 Mem = m_BufferStride;

#if MULTIPLE_RENDER_THREADS > 1
	Mem *= NUMBER_OF_RENDER_THREADS;
#endif
	m_pAllocatedBackingStore = rage_new char[Mem + (GRC_CBUFFER_CACHE_LINE_SIZE - 1)];
	// Align start to a cache line.
	m_pAlignedBackingStore = (char *)GRC_CBUFFER_ALIGN((ptrdiff_t)m_pAllocatedBackingStore, (ptrdiff_t)GRC_CBUFFER_CACHE_LINE_SIZE);
	// Clear it all.
	memset(m_pAllocatedBackingStore, 0, Mem);
}

void grcCBuffer::FreeBackingStore()
{
	if(m_pAllocatedBackingStore)
	{
		delete [] m_pAllocatedBackingStore;
		m_pAllocatedBackingStore = NULL;
		m_pAlignedBackingStore = NULL;
	}
}


// LDS DMC TEMP:-
void grcCBuffer::PrintData()
{
	int i;
	float *pData = (float *)GetDataPtrForThread_ReadOnly(g_RenderThreadIndex);
	int count = Size/16;

	if(!strcmp(Name.c_str(), "rage_bonemtx"))
		return;
	if(!strcmp(Name.c_str(), "rage_matrices"))
		return;

	Printf("grcCBuffer::PrintData()...%s\n", Name.c_str());

	for(i=0; i<count; i++)
	{
		Printf("%d) %f, %f, %f, %f\n", i, pData[0], pData[1], pData[2], pData[3]);
		pData += 4;
	}
}


grcCBuffer::BUFFER_HEADER *grcCBuffer::GetBufferHeader(u32 threadIdx)
{
	BUFFER_HEADER *pRet = (BUFFER_HEADER *)(m_pAlignedBackingStore + threadIdx*m_BufferStride);
	return pRet;
}


grcCBuffer::BUFFER_HEADER *grcCBuffer::GetBufferHeader(u32 threadIdx) const
{
	BUFFER_HEADER *pRet = (BUFFER_HEADER *)(m_pAlignedBackingStore + threadIdx*m_BufferStride);
	return pRet;
}


// D3D buffer/deferred context functions.
void grcCBuffer::OnBeginDeferredContext()
{
	if(sm_DeferredContextCount == 0)
	{
		// TODO:- Visit all grcCBuffer when value is zero.

		// Skip this value.
		sm_DeferredContextCount = 1;
	}
	else
	{
		sm_DeferredContextCount++;
	}
}

void grcCBuffer::OnEndDeferredContext()
{
}


bool grcCBuffer::DoesUnlockNeedToBeCalled()
{
	BUFFER_HEADER *pHeader = GetBufferHeader(g_RenderThreadIndex);

	if(pHeader->m_Flags[GRC_CBUFFER_DIRTY_FLAG])
	{
		return true;
	}
	if(pHeader->m_Flags[GRC_CBUFFER_FIRST_USE])
	{
		return true;
	}
#if MULTIPLE_RENDER_THREADS > 0
	// Make sure upon 1st usage in a deferred context the contents are presented to D3D.
	if((g_IsSubRenderThread) && (pHeader->m_LastDeferredContextCount != sm_DeferredContextCount))
	{
		return true;
	}
#endif
	return false;
}


void grcCBuffer::UpdateD3DBuffer()
{
	void *pSrc = GetDataPtr_ReadOnly();
	void *pDest = LockDirect(GetSize());
	sysMemCpy(pDest, pSrc, GetSize());

	UnlockDirect();
}

/************************************************************************************************************************/
/* grcProgram class.																									*/
/************************************************************************************************************************/

bool grcProgram::SetLocalParameterInConstantBuffer(float *data, int count, u32 offset, grcCBuffer *pConstBuffer, u8 type)
{
	// 'count' is in quadwords, which is why the entries below for MATRIX43 and MATRIX44 are 16 instead of 64.
	static const u8 bytesByType[] = {
		0,      // NONE
		1*4,    // INT
		1*4,    // FLOAT
		2*4,    // VECTOR2
		3*4,    // VECTOR3
		4*4,    // VECTOR4
		0,      // TEXTURE
		1*4,    // BOOL
		4*4,    // MATRIX43
		4*4,    // MATRIX44
		0,      // STRING
		1*4,    // INT
		2*4,    // INT2
		3*4,    // INT3
		4*4,    // INT4
		0,      // STRUCTUREDBUFFER
		0,      // SAMPLERSTATE
		1*4,    // UNUSED1
		1*4,    // UNUSED2
		1*4,    // UNUSED3
		1*4,    // UNUSED4
		0,      // UAV_STRUCTURED
		0,      // UAV_TEXTURE
		0,      // BYTEADDRESS_BUFFER
		0,      // UAV_BYTEADDRESS
	};
	CompileTimeAssert( NELEM(bytesByType) == grcEffect::VT_COUNT );
	Assert(bytesByType[type]!=0);

	return pConstBuffer->SetParameter(data, count, offset, bytesByType[type]);
}


bool grcProgram::SetLocalParameter(int /*address*/, float *data,int count, u32 offset, grcCBuffer *pEffectVar, u8 type)
{
	return grcProgram::SetLocalParameterInConstantBuffer(data, count, offset, pEffectVar, type);
}

void grcProgram::SetLocalFlag(int /*address*/,bool value, u32 offset, grcCBuffer *pEffectVar)
{
	float fValue = (float)value;
	pEffectVar->SetParameter(&fValue, 1, offset, sizeof(float));

	/*
	pEffectVar->SetDirty(0x01);
	char *pConstData = (char *)pEffectVar->GetDataPtr();
	pConstData += offset;
	float fValue = (float)value;
	memcpy(pConstData,&fValue,sizeof(float));
	*/
}


void grcProgram::SetParameter(int /*address*/, const float *data,int count, u32 offset, grcCBuffer *pEffectVar)
{
	pEffectVar->SetParameter(data, count, offset, sizeof(float));
	/*
	pEffectVar->SetDirty(0x01);
	char *pConstData = (char *)pEffectVar->GetDataPtr();
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
	*/
}


void grcProgram::SetParameterW(int /*address*/, const float *data,int count, u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	if(pEffectVar->SetParameter(data, count, offset, sizeof(float)))
	{
#if TRACK_CONSTANT_BUFFER_CHANGES
		if(grcEffect::sm_TrackConstantBufferUsage && global)
			global->IncrementUsageCount();
#endif
	}
	/*
	char *pDest_ReadOnly = (char *)pEffectVar->GetDataPtr_ReadOnly();
	pDest_ReadOnly += offset;
	if(memcmp(pDest_ReadOnly, data, sizeof(float)*count))
	{
		pEffectVar->SetDirty(0x01);
		char *pConstData = (char *)pEffectVar->GetDataPtr();
		pConstData += offset;
		memcpy(pConstData,data,sizeof(float)*count);

#if TRACK_CONSTANT_BUFFER_CHANGES
		if(grcEffect::sm_TrackConstantBufferUsage && global)
			global->IncrementUsageCount();
#endif
	}
	*/
}

void grcProgram::SetFlag(int /*address*/, int value, u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	pEffectVar->SetParameter(reinterpret_cast<const float *>(&value), 1, offset, sizeof(int));
	/*
	pEffectVar->SetDirty(0x01);
	char *pConstData = (char *)pEffectVar->GetDataPtr();
	pConstData += offset;
	float fValue = (float)value;
	memcpy(pConstData,&fValue,sizeof(float));
	*/

#if TRACK_CONSTANT_BUFFER_CHANGES
	if(grcEffect::sm_TrackConstantBufferUsage && global)
		global->IncrementUsageCount();
#endif
}

void grcProgram::SetParameterI(int /*address*/,const int* data, int count, u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter*))
{
	pEffectVar->SetParameter(reinterpret_cast<const float *>(data), count, offset, sizeof(int));
}

/************************************************************************************************************************/
/* Specific program types.																								*/
/************************************************************************************************************************/

void grcVertexProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) (data? const_cast<grcTexture*>(data)->GetTextureView() : NULL);
#if REUSE_RESOURCE
	if (data)
	{
		((grcTexture*)data)->UpdateGPUCopy();
	}
#endif // REUSE_RESOURCE
	g_grcCurrentContext->VSSetShaderResources(address,1,&view);
	g_grcCurrentContext->VSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
}

#if SUPPORT_UAV
void grcVertexProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) (pBuffer ? const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView() : NULL);
	g_grcCurrentContext->VSSetShaderResources(address,1,&view);
}
#endif // SUPPORT_UAV

void grcVertexProgram::Bind() const
{
	if (s_VertexShader != Program)
	{
		GRCDEVICE.SetVertexShader(s_VertexShader = Program,this);
	}
}

/*--------------------------------------------------------------------------------------*/
/* grcFragmentProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcFragmentProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
	(void)data;
	// there are pure textures, read-only buffers, and MSAA textures, all of which do not need samplers
	if (stateHandle != INVALID_STATEBLOCK)
	{
		Assert(address < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
		if (address >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)
		{	// NOTE: We really shouldn't need this when the assert above is fixed.
			return;
		}
		g_grcCurrentContext->PSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
	}
	grcProgram::RecordProgramResourceForVectorDXAPICall(PS_TYPE, address, (void *)data);
}

#if SUPPORT_UAV
void grcFragmentProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	grcProgram::RecordProgramResourceForVectorDXAPICall(PS_TYPE, address, (void*)pBuffer);
}
#endif // SUPPORT_UAV

void grcFragmentProgram::Bind() const
{
	if (s_PixelShader != Program)
	{
		GRCDEVICE.SetPixelShader(s_PixelShader = Program, this);
		//Displayf("%s", this->GetEntryName());
	}
}

/*--------------------------------------------------------------------------------------*/
/* grcComputeProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcComputeProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle, bool bSetSamplerOnly)
{
	if (!bSetSamplerOnly)
	{
		Assert(data != NULL);
		ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcTexture*>(data)->GetTextureView();
#if REUSE_RESOURCE
		if (data)
		{
			((grcTexture*)data)->UpdateGPUCopy();
		}
#endif // REUSE_RESOURCE
		g_grcCurrentContext->CSSetShaderResources(address,1,&view);
	}
	g_grcCurrentContext->CSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
}

#if SUPPORT_UAV
void grcComputeProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView();
	g_grcCurrentContext->CSSetShaderResources(address,1,&view);
}
void grcComputeProgram::SetDataBuffer(int address, const grcBufferUAV *pBuffer)
{
	pBuffer = pBuffer ? pBuffer : sEffectGnm_NullUAVRawBuffer[address];
	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView();
	g_grcCurrentContext->CSSetShaderResources(address,1,&view);
}
#endif // SUPPORT_UAV

#if SUPPORT_UAV

void grcComputeProgram::SetStructuredBufferUAV(int address, const grcBufferUAV *pBuffer, int unorderedCount)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	grcProgram::RecordComputeProgramResourceForVectorDXAPICall(address, (void *)pBuffer, unorderedCount);
}

void grcComputeProgram::SetUnorderedTextureUAV(int address, const grcTextureUAV *pTexture)
{
	grcProgram::RecordComputeProgramResourceForVectorDXAPICall(address, (void *)pTexture);
}

void grcComputeProgram::SetDataBufferUAV(int address, const grcBufferUAV *pBuffer)
{
	pBuffer = pBuffer ? pBuffer : sEffectGnm_NullUAVRawBuffer[address];
	grcProgram::RecordComputeProgramResourceForVectorDXAPICall(address, (void *)pBuffer);
}

void grcComputeProgram::Bind() const
{
	if (s_ComputeShader != Program)
	{
		GRCDEVICE.SetComputeShader(s_ComputeShader = Program, this);
	}
}

#endif // SUPPORT_UAV


/*--------------------------------------------------------------------------------------*/
/* grcDomainProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcDomainProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*)
		const_cast<grcTexture*>(data ? data : grcTexture::None)->GetTextureView();
#if REUSE_RESOURCE
	if (data)
	{
		((grcTexture*)data)->UpdateGPUCopy();
	}
#endif // REUSE_RESOURCE

	g_grcCurrentContext->DSSetShaderResources(address,1,&view);
	g_grcCurrentContext->DSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
}

#if SUPPORT_UAV
void grcDomainProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView();
	g_grcCurrentContext->DSSetShaderResources(address,1,&view);
}
#endif // SUPPORT_UAV

void grcDomainProgram::Bind() const
{
	if (s_DomainShader != Program)
	{
		GRCDEVICE.SetDomainShader(s_DomainShader = Program, this);
	}
}

/*--------------------------------------------------------------------------------------*/
/* grcGeometryProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcGeometryProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcTexture*>(data)->GetTextureView();
#if REUSE_RESOURCE
	if (data)
	{
		((grcTexture*)data)->UpdateGPUCopy();
	}
#endif // REUSE_RESOURCE

	g_grcCurrentContext->GSSetShaderResources(address,1,&view);
	g_grcCurrentContext->GSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
}


#if SUPPORT_UAV
void grcGeometryProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView();
	g_grcCurrentContext->GSSetShaderResources(address,1,&view);
}
#endif // SUPPORT_UAV

void grcGeometryProgram::Bind() const
{
	if (s_GeometryShader != Program)
	{
		GRCDEVICE.SetGeometryShader(s_GeometryShader = Program, this);
	}
}

/*--------------------------------------------------------------------------------------*/
/* grcHullProgram.																		*/
/*--------------------------------------------------------------------------------------*/

void grcHullProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcTexture*>(data)->GetTextureView();
#if REUSE_RESOURCE
	if (data)
	{
		((grcTexture*)data)->UpdateGPUCopy();
	}
#endif // REUSE_RESOURCE

	g_grcCurrentContext->HSSetShaderResources(address,1,&view);
	g_grcCurrentContext->HSSetSamplers(address,1,&g_SamplerStates11[stateHandle]);
}


#if SUPPORT_UAV
void grcHullProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer)
{
	if (pBuffer == NULL && !grcEffect::sm_SetUAVNull)
		pBuffer = sEffectGnm_NullUAVStructuredBuffer[address];

	ID3D11ShaderResourceView * const view = (ID3D11ShaderResourceView*) const_cast<grcBufferUAV*>(pBuffer)->GetShaderResourceView();
	g_grcCurrentContext->HSSetShaderResources(address,1,&view);
}
#endif // SUPPORT_UAV

void grcHullProgram::Bind() const
{
	if (s_HullShader != Program)
	{
		GRCDEVICE.SetHullShader(s_HullShader = Program, this);
	}
}

void grcEffect::InitClass()
{
#if SUPPORT_UAV
	if(GRCDEVICE.SupportsFeature(COMPUTE_SHADER_50))
	{
		std::for_each(sEffectGnm_NullUAVRawBuffer.begin(), sEffectGnm_NullUAVRawBuffer.end(),				[](grcBufferUAV *&buffer){
			delete buffer;
			buffer = rage_new grcBufferUAV;
			buffer->Initialise(1, 4, grcBindShaderResource|grcBindUnorderedAccess, grcsBufferCreate_ReadWriteOnceOnly, grcsBufferSync_None, NULL, false, grcBufferMisc_AllowRawViews, grcBuffer_UAV_FLAG_RAW);
		});
		std::for_each(sEffectGnm_NullUAVStructuredBuffer.begin(), sEffectGnm_NullUAVStructuredBuffer.end(),	[](grcBufferUAV *&buffer){
			delete buffer;
			buffer = rage_new grcBufferUAV;
			buffer->Initialise(1, 4, grcBindShaderResource|grcBindUnorderedAccess, grcsBufferCreate_ReadWriteOnceOnly, grcsBufferSync_None, NULL, false, grcBufferMisc_BufferStructured, grcBuffer_UAV_FLAG_APPEND);
		});
	}
#endif // SUPPORT_UAV
}

void grcEffect::BeginFrame()
{
	grcEffect::ApplyDefaultRenderStates();
	grcEffect::ApplyDefaultSamplerStates();
}

void grcEffect::EndFrame()
{
	//Constant buffer usage
#if TRACK_CONSTANT_BUFFER_CHANGES
	if (sm_TrackConstantBufferUsage)
	{
		if (sm_PrintConstantBufferUsage)
			Printf("*** Start Constant Buffer Lock Count ***\n");

		grcEffect::PrintConstantBufferUsage(sm_PrintConstantBufferUsage);

		if (sm_PrintConstantBufferUsage)
			Printf("*** End Constant Buffer Lock Count ***\n");

		sm_PrintConstantBufferUsage = false;
	}
#endif
}

/************************************************************************************************************************/
/* Constant buffer usage.																								*/
/************************************************************************************************************************/

#if TRACK_CONSTANT_BUFFER_CHANGES
void grcEffect::PrintConstantBufferUsage( bool printUsage )
{
	bool printNonChanged = !sm_PrintConstantBufferUsageOnlyChanged;
	bool dontResetUsage = !sm_TrackConstantBufferResetFrame;

	if (printUsage)
		Printf("\n**Global Constant Buffers\n");

	for (s32 iCount = 0; iCount < sm_GlobalsCBuf.GetCount(); iCount++) {
		if (printUsage)
		{
			Printf("%s - %d\n", sm_GlobalsCBuf[iCount].GetName(), sm_GlobalsCBuf[iCount].GetLockCount());

			for (s32 globalsCount = 0; globalsCount < sm_Globals.GetCount(); globalsCount++) {
				if( sm_Globals[globalsCount].GetParentCBuf() == &sm_GlobalsCBuf[iCount] )
					Printf("-%s, %d\n", sm_Globals[globalsCount].GetName(), sm_Globals[globalsCount].GetUsageCount());
			}

			Printf("\n");
		}

		if( !dontResetUsage )
		{
			sm_GlobalsCBuf[iCount].ResetLockCount();
			for (s32 globalsCount = 0; globalsCount < sm_Globals.GetCount(); globalsCount++) {
				sm_Globals[globalsCount].ResetUsageCount();
			}
		}
	}

	if (printUsage)
		Printf("\n**Locals Constant Buffers\n");



	for (s32 effectCount = 0; effectCount < sm_Effects.GetCount(); effectCount++) {
		if (sm_Effects[effectCount]) {
			bool skip = true;
			if( printNonChanged )
			{
				skip = false;
			}
			else
			{
				for (s32 cBufCount = 0; cBufCount < sm_Effects[effectCount]->m_LocalsCBuf.GetCount(); cBufCount++) {
					if( sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->GetLockCount() > 0 )
						skip = false;
				}
			}

			//See if any of the constant buffers have changed otherwise skip it.
			if (!skip) {
				if (printUsage)
					Printf("\nShader: %s\n", sm_Effects[effectCount]->GetEffectName());

				for (s32 cBufCount = 0; cBufCount < sm_Effects[effectCount]->m_LocalsCBuf.GetCount(); cBufCount++) {
					if (printUsage && (sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->GetLockCount() > 0 || printNonChanged)) {
						u32 lockCount = sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->GetLockCount();
						u32 bufferSize = sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->GetSize();
						Printf("CB: %s, %d, %d, %d\n", sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->GetName(), lockCount, bufferSize, lockCount * bufferSize);

						for (s32 localsCount = 0; localsCount < sm_Effects[effectCount]->m_Locals.GetCount(); localsCount++) {
							if( printUsage && sm_Effects[effectCount]->m_Locals[localsCount].GetParentCBuf() == sm_Effects[effectCount]->m_LocalsCBuf[cBufCount] )
								Printf("-%s, %d\n", sm_Effects[effectCount]->m_Locals[localsCount].GetName(), sm_Effects[effectCount]->m_Locals[localsCount].GetUsageCount());
						}
					}
				}
			}

			if( !dontResetUsage )
			{
				for (s32 cBufCount = 0; cBufCount < sm_Effects[effectCount]->m_LocalsCBuf.GetCount(); cBufCount++) {
					sm_Effects[effectCount]->m_LocalsCBuf[cBufCount]->ResetLockCount();
					int numLocals = sm_Effects[effectCount]->m_Locals.GetCount();

					for (s32 localsCount = 0; localsCount < numLocals; localsCount++) {
						if( sm_Effects[effectCount]->m_Locals[localsCount].GetParentCBuf() == sm_Effects[effectCount]->m_LocalsCBuf[cBufCount] )
							sm_Effects[effectCount]->m_Locals[localsCount].ResetUsageCount();
					}
				}
			}
		}
	}
}
#endif // TRACK_CONSTANT_BUFFER_CHANGES

} // namespace rage

#endif	// RSG_PC && __D3D11
