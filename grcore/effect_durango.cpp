//
// grcore/effect_durango.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#if RSG_DURANGO

#include "effect.h"
#include "effect_internal.h"
#include "stateblock_internal.h"
#include "vertexdecl.h"

#include "channel.h"

#include "grcore/buffer_durango.h"
#include "grcore/wrapper_d3d.h"
#include "gfxcontext_durango.h"
#include "rendertarget_durango.h"

#include "file/stream.h"
#include "system/membarrier.h"
#include "system/memops.h"

#define SET_FAST_TEXTURES_AND_SAMPLERS      (0)
#define SET_FAST_SHADER_RESOURCE_VIEWS      (0)

namespace rage {

extern DECLARE_MTR_THREAD grcVertexShader     *s_VertexShader;
extern DECLARE_MTR_THREAD grcPixelShader      *s_PixelShader;
extern DECLARE_MTR_THREAD grcComputeShader    *s_ComputeShader;
extern DECLARE_MTR_THREAD grcDomainShader     *s_DomainShader;
extern DECLARE_MTR_THREAD grcGeometryShader   *s_GeometryShader;
extern DECLARE_MTR_THREAD grcHullShader       *s_HullShader;
extern DECLARE_MTR_THREAD ID3D11InputLayout   *s_InputLayout;

extern __THREAD grcContextHandle *g_grcCurrentContext;
extern grcSamplerState *g_SamplerStates;

__THREAD u8 g_VertexShaderImmediateMode;
DECLARE_MTR_THREAD grcVertexProgram::DeclSetup *g_VertexDeclSetup;

extern ID3D11SamplerState **g_SamplerStates11;
extern ID3D11Buffer *s_CommonBufferCB;

template<ShaderType SHADER_TYPE> struct SetFastHelper;
template<> struct SetFastHelper<VS_TYPE>{enum{value=D3D11X_SET_FAST_VS};};
template<> struct SetFastHelper<HS_TYPE>{enum{value=D3D11X_SET_FAST_HS};};
template<> struct SetFastHelper<DS_TYPE>{enum{value=D3D11X_SET_FAST_DS};};
template<> struct SetFastHelper<GS_TYPE>{enum{value=D3D11X_SET_FAST_GS};};
template<> struct SetFastHelper<PS_TYPE>{enum{value=D3D11X_SET_FAST_PS};};
template<> struct SetFastHelper<CS_TYPE>{enum{value=D3D11X_SET_FAST_CS};};
#define SET_FAST(SHADER_TYPE)  ((D3D11X_SET_FAST)(SetFastHelper<SHADER_TYPE>::value))

#define WITH_OFFSET_ADD D3D11X_SET_FAST_VS_WITH_OFFSET
CompileTimeAssert(D3D11X_SET_FAST_VS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_VS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_HS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_HS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_DS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_DS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_GS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_GS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_PS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_PS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_CS    + WITH_OFFSET_ADD == D3D11X_SET_FAST_CS_WITH_OFFSET);
CompileTimeAssert(D3D11X_SET_FAST_IA_VB + WITH_OFFSET_ADD == D3D11X_SET_FAST_IA_VB_WITH_OFFSET);
#define SET_FAST_WITH_OFFSET(SHADER_TYPE)  ((D3D11X_SET_FAST)(SET_FAST(SHADER_TYPE)+WITH_OFFSET_ADD))

template<ShaderType SHADER_TYPE>
void grcDevice::SetConstantBuffer(grcContextHandle *ctx, u32 slot, const void *data)
{
	Assert(data);
	ctx->SetFastResources(
		SET_FAST_WITH_OFFSET(SHADER_TYPE), slot, s_CommonBufferCB, (uptr)data-FIXED_PLACEMENT_BASE);
}

// Force instantiate all shader types
template void grcDevice::SetConstantBuffer<VS_TYPE>(grcContextHandle*, u32, const void*);
template void grcDevice::SetConstantBuffer<PS_TYPE>(grcContextHandle*, u32, const void*);
template void grcDevice::SetConstantBuffer<CS_TYPE>(grcContextHandle*, u32, const void*);
template void grcDevice::SetConstantBuffer<DS_TYPE>(grcContextHandle*, u32, const void*);
template void grcDevice::SetConstantBuffer<GS_TYPE>(grcContextHandle*, u32, const void*);
template void grcDevice::SetConstantBuffer<HS_TYPE>(grcContextHandle*, u32, const void*);

#if SET_FAST_TEXTURES_AND_SAMPLERS

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetTextureCommon(int address, ID3D11ShaderResourceView *srv, u16 stateHandle) {
		Assert(address < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

		// there are pure textures, read-only buffers, and MSAA textures, all of which do not need samplers
		if (stateHandle != INVALID_STATEBLOCK) {
			Assert(address < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
			ID3D11SamplerState *const ss = g_SamplerStates11[stateHandle];
			g_grcCurrentContext->SetFastResources(
				SET_FAST(SHADER_TYPE), address, ss,  0,
				SET_FAST(SHADER_TYPE), address, srv, 0);
		}
		else
			g_grcCurrentContext->SetFastResources(
				SET_FAST(SHADER_TYPE), address, srv, 0);
	}

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetTextureCommon(int address, const grcTexture *data, u16 stateHandle) {
		if (!data)
			data = __DEV ? grcTexture::None : grcTexture::NoneBlack;
		ID3D11ShaderResourceView *const srv = const_cast<grcTexture*>(data)->GetTextureView();
		SetTextureCommon<SHADER_TYPE>(address, srv, stateHandle);
	}

#else

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetTextureCommon(int address, const grcTexture *data, u16 stateHandle) {
		// TODO: Should switch over to SetFastResources, but current code "may" be
		// relying on the automatic hazard tracking.  Also need a way to handle
		// buf==NULL.
		grcContextHandle *const ctx = g_grcCurrentContext;
		ID3D11ShaderResourceView *const srv = (ID3D11ShaderResourceView*)(data ? const_cast<grcTexture*>(data)->GetTextureView() : NULL);
		Assert(address < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
		switch (SHADER_TYPE) {
			case VS_TYPE: ctx->VSSetShaderResources(address, 1, &srv); break;
			case PS_TYPE: ctx->PSSetShaderResources(address, 1, &srv); break;
			case CS_TYPE: ctx->CSSetShaderResources(address, 1, &srv); break;
			case DS_TYPE: ctx->DSSetShaderResources(address, 1, &srv); break;
			case GS_TYPE: ctx->GSSetShaderResources(address, 1, &srv); break;
			case HS_TYPE: ctx->HSSetShaderResources(address, 1, &srv); break;
			case NONE_TYPE: break;
		}
		if (stateHandle != INVALID_STATEBLOCK) {
			Assert(address < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
			ID3D11SamplerState *const ss = g_SamplerStates11[stateHandle];
			ctx->SetFastResources(
				SET_FAST(SHADER_TYPE), address, ss, 0);
		}
	}

#endif

#if SET_FAST_SHADER_RESOURCE_VIEWS

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetShaderResourceViewCommon(int address, ID3D11ShaderResourceView *srv) {
		Assert(address < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
		Assert(srv);
		g_grcCurrentContext->SetFastResources(
			SET_FAST(SHADER_TYPE), address, srv, 0);
	}

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetShaderResourceViewCommon(int address, const grcBufferUAV *buf) {
		SetShaderResourceViewCommon<SHADER_TYPE>(address, const_cast<grcBufferUAV*>(buf)->GetShaderResourceView());
	}

#else

	template<ShaderType SHADER_TYPE>
	void grcProgram::SetShaderResourceViewCommon(int address, const grcBufferUAV *buf) {
		ID3D11ShaderResourceView *const srv = buf ? const_cast<grcBufferUAV*>(buf)->GetShaderResourceView() : NULL;
		switch (SHADER_TYPE) {
			case VS_TYPE: g_grcCurrentContext->VSSetShaderResources(address, 1, &srv); break;
			case PS_TYPE: g_grcCurrentContext->PSSetShaderResources(address, 1, &srv); break;
			case CS_TYPE: g_grcCurrentContext->CSSetShaderResources(address, 1, &srv); break;
			case DS_TYPE: g_grcCurrentContext->DSSetShaderResources(address, 1, &srv); break;
			case GS_TYPE: g_grcCurrentContext->GSSetShaderResources(address, 1, &srv); break;
			case HS_TYPE: g_grcCurrentContext->HSSetShaderResources(address, 1, &srv); break;
			case NONE_TYPE: break;
		}
	}

#endif

template<ShaderType SHADER_TYPE>
void grcProgram::BindCommon() const {
	grcContextHandle *const ctxD3d = g_grcCurrentContext;
	grcGfxContext *const ctxGrc = grcGfxContext::current();
	for (int i=0; i<m_numCBuffers; i++) {
		grcCBuffer *cbuf = m_pCBuffers[i];
		if (!cbuf->GetIsExplicitBind()) {
			int reg = cbuf->GetRegister(SHADER_TYPE);
			if (cbuf->GetDirty()) {
				// Crashing in here because cbuf->GetDataPtr() is returning a bad pointer?
				// Check that the alloc scopes are correctly setup in the higher level rendering
				// code.  If the constant buffer was been set with BeginUpdate, then the alloc
				// scope closed too early, you will run into troubles.  g_MatrixBase /
				// "rage_matrices" is one such constant buffer that does this (set via
				// grcViewport::RegenerateDevice()).
				cbuf->ResetDirty();
				Assert(cbuf->GetSize() < 64000);
				void *ctmp = ctxGrc->allocateTemp(m_pCBuffers[i]->GetSize(), 16);
				cbuf->SetBuffer(ctmp);
				sysMemCpy(ctmp, cbuf->GetDataPtr(), cbuf->GetSize());
			}
			const void *const baseAddr = cbuf->GetBuffer();
			grcDevice::SetConstantBuffer<SHADER_TYPE>(ctxD3d, reg, baseAddr);
		}
	}
}

void grcProgram::SetFlag(int UNUSED_PARAM(address), int value, u32 offset, grcCBuffer *cbuffer) {
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			*(int*)((char*)cbuffer->GetDataPtrForThread(i)+offset) = value;
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		*(int*)((char*)cbuffer->GetDataPtr()+offset) = value;
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetParameter(int UNUSED_PARAM(address), const float *value, int count, u32 offset, grcCBuffer *cbuffer) {
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i)+offset, value, count<<4);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr()+offset, value, count<<4);
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetParameterW(int UNUSED_PARAM(address), const float *value, int count, u32 offset, grcCBuffer *cbuffer) {
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i)+offset, value, count<<2);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr()+offset, value, count<<2);
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetParameterI(int UNUSED_PARAM(address), const int *value, int count, u32 offset, grcCBuffer *cbuffer) {
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i)+offset, value, count<<2);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr()+offset, value, count<<2);
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetLocalParameter(int UNUSED_PARAM(address), float *value, int count, u32 offset, grcCBuffer *cbuffer, u8 type) {
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
	sysMemCpy((char*)cbuffer->GetDataPtr()+offset, value, count*bytesByType[type]);
	cbuffer->SetDirtySingleThread();
}

#if SUPPORT_UAV
void grcComputeProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<CS_TYPE>(address, pBuffer);
}

void grcComputeProgram::SetDataBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<CS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV

#if SUPPORT_UAV

void grcComputeProgram::SetStructuredBufferUAV(int address, const grcBufferUAV *pBuffer, int unorderedCount) {
	RecordComputeProgramResourceForVectorDXAPICall(address, (void*)pBuffer, unorderedCount);
}

void grcComputeProgram::SetUnorderedTextureUAV(int address, const grcTextureUAV *pTexture) {
	RecordComputeProgramResourceForVectorDXAPICall(address, (void*)pTexture);
}

void grcComputeProgram::SetDataBufferUAV(int address, const grcBufferUAV *pBuffer) {
	RecordComputeProgramResourceForVectorDXAPICall(address, (void*)pBuffer);
}

#endif // SUPPORT_UAV

void grcVertexProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) const {
	SetTextureCommon<VS_TYPE>(address, data, stateHandle);
}

#if SUPPORT_UAV
void grcVertexProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<VS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV

void grcVertexProgram::Bind() const {
	if (s_VertexShader != Program)
		GRCDEVICE.SetVertexShader(s_VertexShader = Program, this);
	if (Program)
		BindCommon<VS_TYPE>();
}

/*--------------------------------------------------------------------------------------*/
/* grcFragmentProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcFragmentProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) {
	SetTextureCommon<PS_TYPE>(address, data, stateHandle);
}

#if SUPPORT_UAV
void grcFragmentProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<PS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV

void grcFragmentProgram::Bind() const {
	if (s_PixelShader != Program)
		GRCDEVICE.SetPixelShader(s_PixelShader = Program, this);
	if (Program)
		BindCommon<PS_TYPE>();
}

/*--------------------------------------------------------------------------------------*/
/* grcComputeProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcComputeProgram::Bind() const {
	if (s_ComputeShader != Program)
		GRCDEVICE.SetComputeShader(s_ComputeShader = Program, this);
	if (Program)
		BindCommon<CS_TYPE>();
}

void grcComputeProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) {
	SetTextureCommon<CS_TYPE>(address, data, stateHandle);
}

/*--------------------------------------------------------------------------------------*/
/* grcDomainProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcDomainProgram::Bind() const {
	if (s_DomainShader != Program)
		GRCDEVICE.SetDomainShader(s_DomainShader = Program, this);
	if (Program)
		BindCommon<DS_TYPE>();
}

void grcDomainProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) const {
	SetTextureCommon<DS_TYPE>(address, data, stateHandle);
}

#if SUPPORT_UAV
void grcDomainProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<DS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV

/*--------------------------------------------------------------------------------------*/
/* grcGeometryProgram.																	*/
/*--------------------------------------------------------------------------------------*/

void grcGeometryProgram::Bind() const {
	if (s_GeometryShader != Program)
		GRCDEVICE.SetGeometryShader(s_GeometryShader = Program, this);
	if (Program)
		BindCommon<GS_TYPE>();
}

void grcGeometryProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) {
	SetTextureCommon<GS_TYPE>(address, data, stateHandle);
}

#if SUPPORT_UAV
void grcGeometryProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<GS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV

/*--------------------------------------------------------------------------------------*/
/* grcHullProgram.																		*/
/*--------------------------------------------------------------------------------------*/

void grcHullProgram::Bind() const {
	if (s_HullShader != Program)
		GRCDEVICE.SetHullShader(s_HullShader = Program, this);
	if (Program)
		BindCommon<HS_TYPE>();
}

void grcHullProgram::SetTexture(int address, const grcTexture *data, u16 stateHandle) {
	SetTextureCommon<HS_TYPE>(address, data, stateHandle);
}

#if SUPPORT_UAV
void grcHullProgram::SetStructuredBuffer(int address, const grcBufferUAV *pBuffer) {
	SetShaderResourceViewCommon<HS_TYPE>(address, pBuffer);
}
#endif // SUPPORT_UAV


void grcEffect::InitClass() {
}

void grcEffect::BeginFrame() {
	grcEffect::ApplyDefaultRenderStates();
	grcEffect::ApplyDefaultSamplerStates();
}

void grcEffect::EndFrame() {
}

#if RSG_DURANGO
void grcEffect::ForceSetContextShaders(grcContextHandle *ctx)
{
	ctx->IASetInputLayout(s_InputLayout);
	ctx->VSSetShader(s_VertexShader,   NULL, 0);
	if ((uptr)s_PixelShader    != ~0U) ctx->PSSetShader(s_PixelShader,    NULL, 0);
	if ((uptr)s_ComputeShader  != ~0U) ctx->CSSetShader(s_ComputeShader,  NULL, 0);
	if ((uptr)s_DomainShader   != ~0U) ctx->DSSetShader(s_DomainShader,   NULL, 0);
	if ((uptr)s_GeometryShader != ~0U) ctx->GSSetShader(s_GeometryShader, NULL, 0);
	if ((uptr)s_HullShader     != ~0U) ctx->HSSetShader(s_HullShader,     NULL, 0);
}
#endif

grcCBuffer::grcCBuffer() {
	MaxSize = 0;
	sysMemSet(CurrSize, 0, sizeof(CurrSize));
	sysMemSet(BackingStore, 0, sizeof(BackingStore));
	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
	sysMemSet(Buffer, 0, sizeof(Buffer));
	ExplicitBind = false;
}

grcCBuffer::grcCBuffer(u32 size, bool directLockOnly) {
	MaxSize = size;
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = size;

	if (directLockOnly == false) {
		BackingStore[0] = rage_new char[size*NUMBER_OF_RENDER_THREADS];
		sysMemSet(BackingStore[0], 0, size);
		for (unsigned i=1; i<NELEM(BackingStore); ++i)
			BackingStore[i] = BackingStore[i-1]+size;
	}
	else
		sysMemSet(BackingStore, 0, sizeof(BackingStore));

	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
	sysMemSet(Buffer, 0, sizeof(Buffer));
	ExplicitBind = false;
}

grcCBuffer::~grcCBuffer() {
	delete[] BackingStore[0];
}

void grcCBuffer::Init(bool) {
}

void grcCBuffer::Load(fiStream &S) {
	S.ReadInt(&MaxSize,1);
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = MaxSize;
	S.ReadShort(Registers, 6);

	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer, count);
	Name = buffer;

	grcDebugf2("CBuffer::Load - %s, register VS %d PS %d, size %d", buffer, Registers[0], Registers[1], MaxSize);
	NameHash = atStringHash(Name);

	BackingStore[0] = rage_new char[MaxSize*NUMBER_OF_RENDER_THREADS];
	sysMemSet(BackingStore[0], 0, MaxSize);
	for (unsigned i=1; i<NELEM(BackingStore); ++i)
		BackingStore[i] = BackingStore[i-1]+MaxSize;
}

void grcCBuffer::operator=(const grcCBuffer &rhs) {
	delete[] BackingStore[0];
	MaxSize = rhs.MaxSize;
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = MaxSize;
	sysMemCpy(Registers, rhs.Registers, sizeof(Registers));
	NameHash = rhs.NameHash;
	Name = rhs.Name;
	if (rhs.BackingStore[0]) {
		BackingStore[0] = rage_new char[MaxSize*NUMBER_OF_RENDER_THREADS];
		sysMemCpy(BackingStore[0], rhs.BackingStore[0], MaxSize);
		for (unsigned i=1; i<NELEM(BackingStore); ++i)
			BackingStore[i] = BackingStore[i-1]+MaxSize;
	}
}

void grcCBuffer::SetBackingStoreThread(void *backingStore, u32 threadIdx) {
	BackingStore[threadIdx] = (char*)backingStore;
}

void grcCBuffer::SetBackingStore(void *backingStore) {
	if (g_IsSubRenderThread)
		SetBackingStoreThread(backingStore, g_RenderThreadIndex);
	else for (unsigned i=0; i<NELEM(BackingStore); ++i)
		SetBackingStoreThread(backingStore, i);
}

void grcCBuffer::SetDirtySingleThread() {
	AllocScopeId[g_RenderThreadIndex] = grcContextAllocScope::ID_INVALID;
}

void grcCBuffer::SetDirtyAllThreads() {
	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
}

void grcCBuffer::SetDirty() {
	if (g_IsSubRenderThread)
		SetDirtySingleThread();
	else
		SetDirtyAllThreads();
}

bool grcCBuffer::GetDirty() {
	return !grcGfxContext::isAllocScopeValid(AllocScopeId[g_RenderThreadIndex]);
}

void grcCBuffer::ResetDirty() {
	const unsigned rti = g_RenderThreadIndex;
// 	Assert(BackingStore[rti]);
	AllocScopeId[rti] = grcGfxContext::getAllocScopeId();
}

#if MULTIPLE_RENDER_THREADS
void *grcCBuffer::GetDataPtrForThread(u32 idx) {
	return BackingStore[idx];
}
#else
void *grcCBuffer::GetDataPtr() {
	return BackingStore[0];
}
#endif

bool grcCBuffer::Unlock() {
	SetDirtySingleThread();
	return true;
}

void *grcCBuffer::BeginUpdate(u32 sizeBytes) {
	void *result = grcGfxContext::current()->allocateTemp(sizeBytes, 16);
	Assert(result);
	if (g_IsSubRenderThread) {
		const unsigned rti = g_RenderThreadIndex;
		Buffer[rti] = result;
		AllocScopeId[rti] = grcGfxContext::getAllocScopeId();
#if !__PROFILE && !__FINAL
		// Set to NULL to catch any case where BindCommon tries to copy from here.
		BackingStore[rti] = NULL;
#endif
	}
	else for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++) {
		Buffer[i] = result;
		// TODO: This is a bit wrong.  Really need a way for defered contexts to check allocation scope id of main context.
		AllocScopeId[i] = grcContextAllocScope::ID_INFINITE;
#if !__PROFILE && !__FINAL
		BackingStore[i] = NULL;
#endif
	}
	return result;
}

void grcCBuffer::EndUpdate() {
	// If we're in a draw, we need to flush the parameters directly.
	if (grcEffect::IsInDraw() || GetIsExplicitBind()) {
		Assertf(s_VertexShader, "no currently bound vs.\n");
		grcContextHandle *const ctx = g_grcCurrentContext;
		ResetDirty();
		const void *const baseAddr = GetBuffer();
		grcDevice::SetConstantBuffer<VS_TYPE>(ctx, Registers[VS_TYPE], baseAddr);
		grcDevice::SetConstantBuffer<PS_TYPE>(ctx, Registers[PS_TYPE], baseAddr);
	}
}

void grcCBuffer::SetDataDirect(const void *data, u32 UNUSED_PARAM(sizeBytes)) {
	const unsigned rti = g_RenderThreadIndex;
	Buffer[rti] = const_cast<void*>(data);
	AllocScopeId[rti] = grcContextAllocScope::ID_INFINITE;
}

void grcCBuffer::SetDataIndirect(const void *data, u32 sizeBytes) {
	const unsigned rti = g_RenderThreadIndex;
	BackingStore[rti] = (char*)data;
	AllocScopeId[rti] = grcContextAllocScope::ID_INVALID;
	CurrSize[rti] = sizeBytes;
}

} // namespace rage

#endif	// RSG_DURANGO
