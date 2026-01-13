// 
// grcore/effect_gnm.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "effect.h"
#include "effect_internal.h"
#include "stateblock_internal.h"
#include "vertexdecl.h"

#if __GNM

#include "channel.h"

#include "buffer_gnm.h"
#include "gfxcontext_gnm.h"
#include "rendertarget_gnm.h"
#if __ASSERT
#include "texturefactory_gnm.h"
#endif

#include "file/stream.h"
#include "system/membarrier.h"
#include "system/memops.h"
#include "gnmx.h"

#if SCE_ORBIS_SDK_VERSION >= (0x00990020u)
	#include "grcore/gnmx/shader_parser.h"
	namespace SHADERPARSER = sce::Gnmx;
#else
	#include <shader/shader_parser.h>
	namespace SHADERPARSER = sce::Shader::Binary;
#endif	//SCE_ORBIS_SDK_VERSION
#include <shader/binary.h>

namespace rage {
//NOTE: Cue code ignores calls to set buffers as NULL. This would be fine if Sony validation didn't then complain that the last address 
//set to that V# is garbage. So instead of NULL, we'll set a fake buffer. For more details, see https://ps4.scedev.net/support/issue/22117
static grcBufferUAV sEffectGnm_NullUAVStructuredBuffer(grcBuffer_Structured, true);

PARAM(noFetchRemapping,"Disable semantic remap table for the fetch shader");
extern __THREAD grcContextHandle *g_grcCurrentContext;
extern grcSamplerState *g_SamplerStates;

__THREAD u8 g_VertexShaderImmediateMode;
__THREAD grcVertexProgram::DeclSetup *g_VertexDeclSetup;
__THREAD const grcVertexProgram *grcVertexProgram::CurrentProgram;
__THREAD const grcHullProgram *grcHullProgram::CurrentProgram;
__THREAD const grcDomainProgram *grcDomainProgram::CurrentProgram;

#if EFFECT_CACHE_PROGRAM
u32 grcProgram::ComputeFingerprint(const void *code,u32 codeSize)
{
	// Association hash isn't useful for this purpose.
	SHADERPARSER::ShaderInfo si, si2;
	const sce::Shader::Binary::Header &hdr = *(sce::Shader::Binary::Header*)code;
	const size_t sdbBytes = 8;		// Extra stuff at the end of the ucode that breaks merging.
	switch (hdr.m_shaderType) {
	case sce::Shader::Binary::kShaderTypeGsShader:
		SHADERPARSER::parseGsShader(&si, &si2, code);
		// Special case.
		return atDataHash((const char*)si.m_gpuShaderCode,si.m_gpuShaderCodeSize-sdbBytes, atDataHash(si2.m_gpuShaderCode,si2.m_gpuShaderCodeSize-sdbBytes));
	default:
		{
			sce::Gnmx::ShaderType eShaderType;
			const sce::Shader::Binary::Header		*binaryHeader = reinterpret_cast<const sce::Shader::Binary::Header*>(code);
			const sce::Gnmx::ShaderFileHeader		*header		  = reinterpret_cast<const sce::Gnmx::ShaderFileHeader*>(binaryHeader + 1);
			eShaderType = (sce::Gnmx::ShaderType)header->m_type;

#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
			SHADERPARSER::parseShader(&si,code);
#else
			SHADERPARSER::parseShader(&si,code, eShaderType); 
#endif
			return atDataHash((const char*)si.m_gpuShaderCode,si.m_gpuShaderCodeSize-sdbBytes);
		}
	}
}
#endif // EFFECT_CACHE_PROGRAM

void grcProgram::SetTextureCommon(sce::Gnm::ShaderStage stage,int address,const grcTextureObject *data,u16 stateHandle)
{
	if (!data)
		data = (__DEV ? grcTexture::None : grcTexture::NoneBlack)->GetCachedTexturePtr();
	Assert( data->isTexture() );
	
	// there are pure textures, read-only buffers, and MSAA textures, all of which do not need samplers
	if (stateHandle != INVALID_STATEBLOCK)
	{
		Assert(address < sce::Gnm::kSlotCountSampler);
		gfxc.setSamplers(stage, address, 1, &g_SamplerStates[stateHandle].s);
	}
	gfxc.setTextures(stage, address, 1, data);
}

#if ENABLE_LCUE
void grcProgram::UpdateGlobalTexture(int stage,int index,const grcTextureObject *data, u16 stateHandle)
{
	SetTextureCommon(stage,index,data,stateHandle);
}
#endif

void grcProgram::BindCommon(sce::Gnm::ShaderStage stage, int stageType) const
{
	for (int i=0; i<m_numCBuffers; i++) {
		grcCBuffer *cbuf = m_pCBuffers[i];
		if (!cbuf->GetIsExplicitBind()) {
			int reg = cbuf->GetRegister(stageType);
			if (cbuf->GetDirty()) {
				cbuf->ResetDirty();
				Assert(cbuf->GetSize() < 64000);
				void *ctmp = gfxc.allocateFromCommandBuffer(m_pCBuffers[i]->GetSize(),sce::Gnm::kEmbeddedDataAlignment16);
				grcGfxContext::initAsConstantBuffer(*cbuf->GetBuffer(),ctmp,cbuf->GetSize());
				sysMemCpy(ctmp,cbuf->GetDataPtr(),cbuf->GetSize());
				// Note: If you get a NULL dereference crash in this memcpy, then
				// this cbuffer is likely being reused outside of the alloc scope
				// where BeginUpdate was last called.
			}
			Assert(cbuf->GetBuffer()->getBaseAddress());
			gfxc.setConstantBuffers(stage,reg,1,cbuf->GetBuffer());
		}
	}

#	if GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING
		u32 *dbg = (u32*)gfxc.embedDataDcb(5);
		dbg[0] = 'EGAR';
		dbg[1] = 'GORP';
		dbg[2] = (u32)stage;
		dbg[3] = (u32)(u64)this;
		dbg[4] = (u32)((u64)this>>32);
#	endif
}

void grcProgram::SetFlag(int address, int value, u32 offset, grcCBuffer *cbuffer)
{
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			*(int*)((char*)cbuffer->GetDataPtrForThread(i) + offset) = value;
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		*(int*)((char*)cbuffer->GetDataPtr() + offset) = value;
		cbuffer->SetDirtySingleThread();
	}
}


void grcProgram::SetParameter(int /*address*/, const float* value, int count,u32 offset, grcCBuffer *cbuffer)
{
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i) + offset,value,count<<4);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr() + offset,value,count<<4);
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetParameterW(int /*address*/, const float* value, int count, u32 offset, grcCBuffer *cbuffer)
{
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i) + offset,value,count << 2);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr() + offset,value,count << 2);
		cbuffer->SetDirtySingleThread();
	}
}

void grcProgram::SetParameterI(int address, const int* value, int count, u32 offset, grcCBuffer *cbuffer)
{
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i) + offset,value,count << 2);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr() + offset,value,count << 2);
		cbuffer->SetDirtySingleThread();
	}
}


void grcProgram::SetLocalParameter(int /*address*/,float *value,int count,u32 offset,grcCBuffer *cbuffer,u8 type)
{
	// 'count' is in quadwords, which is why the entries below for MATRIX43 and MATRIX44 are 16 instead of 64.
	static const u8 bytesByType[] = {
		0,	// NONE
		1*4,	// INT
		1*4,	// FLOAT
		2*4,	// VECTOR2
		3*4,	// VECTOR3
		4*4,	// VECTOR4
		0,	// TEXTURE
		1*4,	// BOOL
		4*4,	// MATRIX43
		4*4,	// MATRIX44
		0,	// STRING
		1*4,	// INT
		2*4,	// INT2
		3*4,	// INT3
		4*4,	// INT4
		0,  // STRUCTUREDBUFFER
		0,   // SAMPLERSTATE
		1*4,	// UNUSED1
		1*4,	// UNUSED2
		1*4,	// UNUSED3
		1*4,	// UNUSED4
		0,	// UAV_STRUCTURED
		0,	// UAV_TEXTURE
		0,	// BYTEADDRESS_BUFFER
		0,	// UAV_BYTEADDRESS
	};
	CompileTimeAssert( NELEM(bytesByType) == grcEffect::VT_COUNT );
	sysMemCpy((char*)cbuffer->GetDataPtr() + offset,value,count * bytesByType[type]);
	cbuffer->SetDirtySingleThread();
}

#if SUPPORT_UAV
void grcProgram::SetUnorderedTextureUAV(sce::Gnm::ShaderStage stage, int address, const grcTextureUAV *pTexture)
{
	Assert( address < sce::Gnm::kSlotCountRwResource );
	Assert( stage == sce::Gnm::kShaderStageCs );
	const sce::Gnm::Texture *const tex = pTexture->GetUnorderedAccessView();
	Assert( !tex || tex->getResourceMemoryType() != sce::Gnm::kResourceMemoryTypeRO );
	gfxc.setRwTextures( stage, address, 1, tex );
}

void grcProgram::SetStructuredBuffer(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer)
{
	const sce::Gnm::Buffer *const buf = pBuffer ? pBuffer->GetUnorderedAccessView() : sEffectGnm_NullUAVStructuredBuffer.GetUnorderedAccessView(); //NULL;
	gfxc.setBuffers( stage, address, 1, buf );
}

void grcProgram::SetStructuredBufferUAV(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer, int unorderedCount)
{
	Assert( address < sce::Gnm::kSlotCountRwResource );

	const sce::Gnm::Buffer *const buf = pBuffer ? pBuffer->GetUnorderedAccessView() : NULL;
	Assert( !buf || buf->getResourceMemoryType() != sce::Gnm::kResourceMemoryTypeRO );
	gfxc.setRwBuffers( stage, address, 1, buf );

	if (unorderedCount >= 0 && pBuffer)
	{
		gfxc.clearAppendConsumeCounters( pBuffer->m_GdsCounterOffset, 0, 1, unorderedCount );
	}
}

void grcProgram::SetDataBuffer(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer)
{
	SetStructuredBuffer( stage,address,pBuffer );
}

void grcProgram::SetDataBufferUAV(sce::Gnm::ShaderStage stage, int address, const grcBufferUAV *pBuffer)
{
	SetStructuredBufferUAV( stage,address,pBuffer );
}

void grcProgram::SetNumAppendConsumeCounters(sce::Gnm::ShaderStage stage, u32 base, u32 numCounters)
{
	gfxc.setAppendConsumeCounterRange( stage, base, numCounters*4 );
}
#endif // SUPPORT_UAV

//----------------------------------------------------------------------------------//

static uint32_t initTessellation(const sce::Gnmx::LsShader *const localShader, const sce::Gnmx::HsShader *const hullShader, sce::Gnm::ShaderStage domainStage)
{	
	// Comment from patches-simple-sample.cpp:
	// THIS CRASH, only happens on A0 cards so I am disabling this clamp on numPatches.
	// we start getting crashes when numPatches>=16.
	// For instance TFs all 8.0 on a quad domain and with numPatches==16
	// triggers a crash in every example we've tried so far.
	const uint32_t maxPatches = 15;
	
	// computing LDS size
	const sce::Gnm::HullStateConstants &hst = hullShader->m_hullStateConstants;
	const uint32_t patchSize = hst.m_numInputCP * localShader->m_lsStride +
		hst.m_numOutputCP * hst.m_cpStride + hst.m_numPatchConst * 16;
	const uint32_t alignMask = 0x3F;
	const uint32_t maxLdsSize  = ((maxPatches * patchSize) + alignMask) & ~alignMask;
	
	uint32_t numPatches = 0, vgtPrims = 0;
	sce::Gnmx::computeVgtPrimitiveAndPatchCounts(&vgtPrims, &numPatches, hullShader->getNumVgprs(), maxLdsSize, localShader, hullShader);
	Assert(numPatches <= maxPatches);

	uint8_t numVgtPrimitivesMinus1 = (uint8_t)(vgtPrims-1);
	gfxc.setVgtControl(numVgtPrimitivesMinus1, sce::Gnm::kVgtPartialVsWaveDisable, sce::Gnm::kVgtSwitchOnEopDisable);

	auto tessConstants = gfxc.allocateTemp<sce::Gnm::TessellationDataConstantBuffer>();
	tessConstants->init(&hst, localShader->m_lsStride, numPatches);			// build constants
	gfxc.setTessellationDataConstantBuffer(tessConstants, domainStage);

	return numPatches;
}

//----------------------------------------------------------------------------------//

void grcVertexProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle) const
{
	SetTextureCommon(GetGnmStage(), address, data, stateHandle);
}

void grcVertexProgram::SetGlobalTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
	SetTextureCommon(sce::Gnm::kShaderStageEs, address, data, stateHandle);
	SetTextureCommon(sce::Gnm::kShaderStageVs, address, data, stateHandle);
	SetTextureCommon(sce::Gnm::kShaderStageLs, address, data, stateHandle);
}

void grcVertexProgram::Bind() const
{
	// Most of the work is done in grcVertexProgram::Bind(grcVertexDeclarator*)
	g_VertexShaderImmediateMode = ImmFVF;
	CurrentProgram = this;
#if ENABLE_LCUE
	if (bIsEsShader)
		gfxc.setEsShader( ProgramES, SRO );
	else if (bIsLsShader)
		gfxc.setLsShader( ProgramLS, NULL, SRO );
	else
		gfxc.setVsShader( ProgramVS, SRO );
#endif
}

grcVertexProgram::DeclSetup::~DeclSetup()
{
	FetchDecl->Release();
	freeVideoSharedMemory(FetchShader);
}

sysCriticalSectionToken s_declSetupCs;

PARAM(disassemblefetch,"Disassemble fetch shaders to TTY");

static void DisassembleFetchShader(u32 *ff,u32 *ffStop)
{
	while (ff < ffStop) {
		char buf[64] = "";
		u32 opc = *ff;
		const char *opcs = "???";
		if ((opc & 0xFC000000) == 0xE0000000) {
			switch ((opc >> 18) & 127) {
			case 0: opcs = "BUFFER_LOAD_FORMAT_X"; break;
			case 1: opcs = "BUFFER_LOAD_FORMAT_XY"; break;
			case 2: opcs = "BUFFER_LOAD_FORMAT_XYZ"; break;
			case 3: opcs = "BUFFER_LOAD_FORMAT_XYZW"; break;
			}
			grcDisplayf("Fetch %p: %08x",ff,*ff);
			opc = *++ff;
			u32 vaddr = opc & 255;
			u32 vdata = (opc >> 8) & 255;
			u32 srsrc = (opc >> 16) & 31;
			u32 soffset = (opc >> 24) & 255;
			sprintf(buf,"%s vgpr[%d], V#%d via sgpr[%d] + %d",
				opcs,vdata, vaddr,srsrc*4,soffset-128);
		}
		else if ((opc & 0xFC000000) == 0xE8000000) {
			sprintf(buf,"LDS/GDS %d",(opc >> 18) & 255);
		}
		else if ((opc & 0xFF800000) == 0xBF800000) {
			switch ((opc >> 16) & 127) {
			case 0: opcs = "S_NOP"; break;
			case 1: opcs = "S_ENDPGM"; break; 
			case 2: opcs = "S_BRANCH"; break;
			case 10: opcs = "S_BARRIER"; break;
			case 12: opcs = "S_WAITCNT"; break;
			}
			sprintf(buf,"%s %d",opcs,opc & 65535);
		}
		else if ((opc & 0xFF800000) == 0xBE800000) {
			switch ((opc >> 8) & 255) {
			case 32: opcs = "S_SETPC_B64"; break;
			case 33: opcs = "S_SWAPPC_B64"; break;
			}
			sprintf(buf,"%s sgpr[%d]",opcs,opc & 255);
			if ((opc & 0xFF80FFFF) == 0xBE8003FF)
				sprintf(buf,"S_MOV_B32 sgpr[%d],0x%08x",(opc >> 16) & 127,*++ff);
		}
		else if ((opc & 0xF0000000) == 0xB0000000) {
			switch ((opc >> 23) & 31) {
			case 0: opcs = "S_MOVK_I32"; break;
			case 15: opcs = "S_ADDK_I32"; break;
			case 16: opcs = "S_MULK_I32"; break;
			}
			sprintf(buf,"%s sgpr[%d],%d",opcs,(opc >> 16) & 127,s16(opc & 65535));
		}
		else if ((opc & 0xF8000000) == 0xC0000000) {
			int scale = 1;
			switch ((opc >> 22) & 31) {
			case 0: opcs = "S_LOAD_DWORD"; break;
			case 1: opcs = "S_LOAD_DWORDX2"; scale=2; break;
			case 2: opcs = "S_LOAD_DWORDX4"; scale=4; break;
			case 3: opcs = "S_LOAD_DWORDX8"; scale=8; break;
			case 4: opcs = "S_LOAD_DWORDX16"; scale=16; break;
			case 8: opcs = "S_BUFFER_LOAD_DWORD"; break;
			case 9: opcs = "S_BUFFER_LOAD_DWORDX2"; scale=2; break;
			case 10: opcs = "S_BUFFER_LOAD_DWORDX4"; scale=4; break;
			case 11: opcs = "S_BUFFER_LOAD_DWORDX8"; scale=8; break;
			case 12: opcs = "S_BUFFER_LOAD_DWORDX16"; scale=16; break;
			case 30: opcs = "S_MEMTIME"; break;
			case 31: opcs = "S_DCACHE_INV"; break;
			}
			u32 sdst = (opc >> 15) & 127;
			u32 sbase = ((opc >> 9) & 31) << 1;
			if (opc & 256)
				sprintf(buf,"%s sgpr[%d], sgpr[%d]+%d",opcs,sdst,sbase,(opc & 255) * scale);
			else 
				sprintf(buf,"%s sgpr[%d], sgpr[%d]+sgpr[%d]",opcs,sdst,sbase,opc & 255);
		}
		else if ((opc & 0xC0000000) == 0x80000000) {
			switch ((opc >> 23) & 127) {
			case 30: opcs = "S_LSHL_B32"; break;
			}
			sprintf(buf,"%s sgpr[%d],sgpr[%d],sgpr[%d]",opcs,(opc >> 16) & 127,(opc >> 8) & 255,(opc & 255));
		}

		grcDisplayf("Fetch %p: %08x %s",ff,*ff,buf);
		++ff;
	}
	grcDisplayf("-----");
}

void grcVertexProgram::Bind(const grcVertexDeclaration *decl) const
{
	// From simplet-simple-fs:
	// 
	// Semantic wise, the vertex shader expects:
	//   0 -> Position
	//   1 -> Color
	//
	// The vertex buffer is setup as follow:
	//   0 -> Position
	//   1 -> Garbage
	//   2 -> Color
	//
	// The remap semantic table is a table an integer describing the expected vertex input position for each element
	// in the array of vertex buffer descriptor.
	//
	// In this sample:
	//   0 -> Position  ->  0
	//   1 -> Garbage   -> -1 -- Unused value.
	//   2 -> Color     ->  1
	//
	// The value of -1 is purely arbitrary; the importance is to have a value which cannot be matched with a vertex semantic.
	// In this case since the vertex shader have only 2 input semantic value (0 for the position and 1 for the color), we
	// could have used: 2 to represent the unused value.

	// The declarator always describes the order the vertex data appears.  Each declarator slot becomes a Buffer slot in
	// the setVertexBuffers array input.
	// The order in the declarator may not match the order in the current shader's input assembly table.

	// First, see if this shader already has a remap for this declaration.
	DeclSetup *ds = FirstDecl;
	while (ds)
		if (ds->FetchDecl == decl)
			break;
		else
			ds = ds->Next;

	if (!ds && FVF) {
		s_declSetupCs.Lock();

		// Check again after we grab the critsec in case somebody else tried and succeeded simultaneously
		ds = FirstDecl;
		while (ds)
			if (ds->FetchDecl == decl)
				break;
			else
				ds = ds->Next;
		if (!ds) {
			ds = rage_new DeclSetup;
			ds->Next = FirstDecl;
			ds->FetchDecl = decl;
			decl->AddRef();

			bool customFetch = false; // ENABLE_LCUE;
			// Tables info:
			// Semantics[elementId]				= rageSemantics
			// VsharpRemap[rageSemantics]		= v#Index
			// SemanticRemap[rageSemantics]		= orbisSemantics
			// RemapSemanticTable[bindIndex]	= orbisSemantics
			// RemapTable[bindIndex]			= elementId
			// Note: we chose bindIndex = v#Index

			// Figure out the step rates for instances
			u8 *const steps = ds->InstanceStepRates;
			steps[0] = steps[1] = 0;
			sce::Gnm::FetchShaderInstancingMode instanceMode[16] = {sce::Gnm::kFetchShaderUseVertexIndex};
			for (int i=0; i<decl->ElementCount; ++i) {
				const u8 inputId = VsharpRemap[decl->Semantics[i]];
				if (inputId == 0xFF)
					continue;
				Assert( inputId < NELEM(instanceMode) );
				sce::Gnm::FetchShaderInstancingMode &inputMode = instanceMode[inputId];
				const u8 div = decl->Dividers[i];
				// Any instancing cannot use custom fetch
				if (div==0)	{
					inputMode = sce::Gnm::kFetchShaderUseVertexIndex;
				}else if (div==1)	{
					inputMode = sce::Gnm::kFetchShaderUseInstanceId;
					customFetch = false;
				}else if (!steps[0] || steps[0] == div)	{
					steps[0] = div;
					inputMode = sce::Gnm::kFetchShaderUseInstanceIdOverStepRate0;
					customFetch = false;
				}else if (!steps[1] || steps[1] == div)	{
					steps[1] = div;
					inputMode = sce::Gnm::kFetchShaderUseInstanceIdOverStepRate1;
					customFetch = false;
				}else	{
					Assertf(0,"Too many instance steps are used by the declaration!");
				}
			}

			// Build the attribute mapping table
			uint32_t tempRemapTable[16];	//a copy of RemapSemanticTable to pass in the fetch build state
			sysMemSet(tempRemapTable,0xFF,sizeof(tempRemapTable));
			sysMemSet(ds->RemapSemanticTable,0xFF,sizeof(ds->RemapSemanticTable));
			sysMemSet(ds->RemapTable,15,sizeof(ds->RemapTable));	// Missing channels all map to decl slot 15.
			for (int i=0; i<decl->ElementCount; i++) {
				// For each declarator element, figure out its FVF index, and use that to figure out
				// which vertex input slot (if any) the shader expects it in
				u8 shaderSemantic = SemanticRemap[decl->Semantics[i]];
				u8 vsharpIndex = VsharpRemap[decl->Semantics[i]];
				if (vsharpIndex != 0xFF) {
					ds->RemapSemanticTable[vsharpIndex] = tempRemapTable[vsharpIndex] = shaderSemantic;
					ds->RemapTable[vsharpIndex] = i;
					if (decl->Streams[i] || decl->Offsets[i] > 64)	// multiple streams can't use custom fetch, and super-huge vertices cannot either
						customFetch = false;
				}
			}

			for (int i=0; i<decl->ElementCount; i++)
				grcDebugf2("remapSemanticTable[%d] = %d",i,ds->RemapSemanticTable[i]);

			if (PARAM_noFetchRemapping.Get())	{
				if (bIsEsShader)	{
					ds->FetchShader = allocateVideoSharedMemory( sce::Gnmx::computeEsFetchShaderSize(ProgramES), sce::Gnm::kAlignmentOfFetchShaderInBytes );
					sce::Gnmx::generateEsFetchShader( ds->FetchShader, &ds->Modifier, ProgramES, instanceMode );
					ds->InputCount = ProgramES->m_numInputSemantics;
				}else if (bIsLsShader)	{
					ds->FetchShader = allocateVideoSharedMemory( sce::Gnmx::computeLsFetchShaderSize(ProgramLS), sce::Gnm::kAlignmentOfFetchShaderInBytes );
					sce::Gnmx::generateLsFetchShader( ds->FetchShader, &ds->Modifier, ProgramLS );
					ds->InputCount = ProgramLS->m_numInputSemantics;
				}else	{
					ds->FetchShader = allocateVideoSharedMemory( sce::Gnmx::computeVsFetchShaderSize(ProgramVS), sce::Gnm::kAlignmentOfFetchShaderInBytes );
					sce::Gnmx::generateVsFetchShader( ds->FetchShader, &ds->Modifier, ProgramVS, instanceMode );
					ds->InputCount = ProgramVS->m_numInputSemantics;
				}
				ds->IsFetchShaderRemapped = false;
				Assert(!customFetch);
			}
			else	{ // PARAM_noFetchRemapping.Get()
				sce::Gnm::FetchShaderBuildState fb = {0};
				if (bIsEsShader)	{
					sce::Gnm::generateEsFetchShaderBuildState(&fb, 
						&ProgramES->m_esStageRegisters, ProgramES->m_numInputSemantics, instanceMode
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
						,0,0
#endif
						);
					fb.m_numInputSemantics	= ProgramES->m_numInputSemantics;
					fb.m_inputSemantics		= ProgramES->getInputSemanticTable();
					fb.m_numInputUsageSlots	= ProgramES->m_common.m_numInputUsageSlots;
					fb.m_inputUsageSlots	= ProgramES->getInputUsageSlotTable();
				}
				else if (bIsLsShader)	{
					sce::Gnm::generateLsFetchShaderBuildState(&fb, 
						&ProgramLS->m_lsStageRegisters, ProgramLS->m_numInputSemantics
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
						,instanceMode, 0,0
#endif
						);
					fb.m_numInputSemantics	= ProgramLS->m_numInputSemantics;
					fb.m_inputSemantics		= ProgramLS->getInputSemanticTable();
					fb.m_numInputUsageSlots	= ProgramLS->m_common.m_numInputUsageSlots;
					fb.m_inputUsageSlots	= ProgramLS->getInputUsageSlotTable();
				}
				else	{
					sce::Gnm::generateVsFetchShaderBuildState(&fb, 
						&ProgramVS->m_vsStageRegisters, ProgramVS->m_numInputSemantics, instanceMode
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
						,0,0
#endif
						);
					fb.m_numInputSemantics	= ProgramVS->m_numInputSemantics;
					fb.m_inputSemantics		= ProgramVS->getInputSemanticTable();
					fb.m_numInputUsageSlots	= ProgramVS->m_common.m_numInputUsageSlots;
					fb.m_inputUsageSlots	= ProgramVS->getInputUsageSlotTable();
				}

				for (int i=0; i<fb.m_numInputSemantics; i++) {
					grcDebugf2("vsharpTable[%d] = %d",i,ds->RemapTable[i]);
					// Assertf(ds->RemapTable[i] != 15,"V#%d will use zero for all inputs",i);
					if ((int)tempRemapTable[i] < 0)	{
						Assertf(0, "Shader '%s' V# %d is not covered by the vertex declaration", m_EntryName.c_str(), i);
						ds->RemapSemanticTable[i] = tempRemapTable[i] = fb.m_inputSemantics[i].m_semantic;
					}
				}

				fb.m_numElementsInRemapTable= fb.m_numInputSemantics;
				fb.m_semanticsRemapTable	= tempRemapTable;

				for (int i=0; i<fb.m_numInputSemantics; i++)
					grcDebugf2("is %d. semantic %d elsize %d VGPR %d",i,fb.m_inputSemantics[i].m_semantic,fb.m_inputSemantics[i].m_sizeInElements,fb.m_inputSemantics[i].m_vgpr);
				for (int i=0; i<fb.m_numInputUsageSlots; i++)
					grcDebugf2("us %d. api %d",i,fb.m_inputUsageSlots[i].m_apiSlot);

				// Custom fetches require at most one extra word more than the normal method.
				uint32_t *fba = (uint32_t*) allocateVideoSharedMemory(fb.m_fetchShaderBufferSize + customFetch * 4 * fb.m_numInputSemantics, sce::Gnm::kAlignmentOfFetchShaderInBytes);
				sce::Gnm::generateFetchShader(fba, &fb);
				ds->FetchShader = fba;
			#if SCE_ORBIS_SDK_VERSION >= (0x00930020u)
				ds->Modifier = fb.m_shaderModifier;
				grcDebugf2("Modifier: %08x",ds->Modifier);
			#endif
				if (PARAM_disassemblefetch.Get())
					DisassembleFetchShader(fba,fba+fb.m_fetchShaderBufferSize/4);

				if (customFetch) {
					uint32_t *f = fba;
					uint32_t sgpr = (fba[0] >> 15 & 127);
					uint32_t word3 = ds->FetchDecl->VertexDword3[ds->RemapTable[0]];
					uint32_t s_mov_b32 = 0xBE8003FF | ((sgpr+3) << 16);
					*f++ = 0xc0800300 | (sgpr << 15); // S_LOAD_DWORDX4 sgpr[16], sgpr[2]+0
					*f++ = 0xbf8c007f; // S_WAITCNT 127
					for (int i=0; i<fb.m_numInputSemantics; i++) {
						uint32_t newWord3 = decl->VertexDword3[ds->RemapTable[i]];
						if (newWord3 != word3) {
							*f++ = s_mov_b32;
							*f++ = newWord3;
							word3 = newWord3;
						}
						*f++ = 0xE0002000 | ((fb.m_inputSemantics[i].m_sizeInElements - 1) << 18);	// BUFFER_LOAD_FORMAT_XY[Z][W]
						// This uses a TBUFFER_LOAD_FORMAT_... instruction instead, but you lose the swizzle control.
						// *f++ = 0xE8002000 | ((vst[i].m_sizeInElements - 1) << 16) | (((word3 >> 12) & 7) << 23) | (((word3 >> 15) & 15) << 19);
						*f++ = 
							(fb.m_inputSemantics[i].m_vgpr << 8) |		// vgpr[]
							(sgpr << 14) |								// sgpr[]
							((128 + decl->Offsets[ds->RemapTable[i]]) << 24);
					}
					*f++ = 0xbf8c0000; // S_WAITCNT 0
					*f++ = 0xbe802000; // S_SETPC_B64 sgpr[0]
					*f++ = decl->ElementCount;
					if ((size_t)f - (size_t)fba > fb.m_fetchShaderBufferSize + customFetch * 4 * fb.m_numInputSemantics)
						Quitf("Fix memory size computation!");

					if (PARAM_disassemblefetch.Get())
						DisassembleFetchShader(fba,f);
					ds->InputCount = 1;
				}
				else
					ds->InputCount = fb.m_numInputSemantics;
				ds->IsFetchShaderRemapped = true;
			} // PARAM_noFetchRemapping.Get()
		}

		// Only when completely initialized can we link the new DeclSetup
		// into the list, since other threads can scan the linked list
		// without locking the crit sec.
		sysMemWriteBarrier();
		FirstDecl = ds;

		s_declSetupCs.Unlock();
	}

#if DEVICE_CLIP_PLANES
	GRCDEVICE.ResolveClipPlanes();
#endif	//DEVICE_CLIP_PLANES

	if (ds && (ds->InstanceStepRates[0] + ds->InstanceStepRates[1]))
		gfxc.setInstanceStepRate( ds->InstanceStepRates[0], ds->InstanceStepRates[1] );
	g_VertexDeclSetup = ds;

#if ENABLE_LCUE
	if (ds)
	{
		if (bIsEsShader)
			gfxc.setEsFetchShader(ds->Modifier, ds->FetchShader);
		else if (bIsLsShader)
			gfxc.setLsFetchShader(ds->Modifier, ds->FetchShader);
		else
			gfxc.setVsFetchShader(ds->Modifier, ds->FetchShader);
	}
#else
# if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	if (bIsEsShader)
		gfxc.setEsShader( ProgramES, ds ? ds->FetchShader : NULL );
	else if (bIsLsShader)
		gfxc.setLsShader( ProgramLS, ds ? ds->FetchShader : NULL );
	else
		gfxc.setVsShader( ProgramVS, ds ? ds->FetchShader : NULL );
# else
	u32 vsWaveLimit = 0;    // 0 = no limit
	if (bIsEsShader)
		gfxc.setEsShader( ProgramES, ds? ds->Modifier : 0, ds ? ds->FetchShader : NULL );
	else if (bIsLsShader)
	{
		const grcHullProgram *const hullProgram = grcHullProgram::GetCurrent();
		Assert( hullProgram != NULL );
		const grcDomainProgram *const domainProgram = grcDomainProgram::GetCurrent();
		Assert( domainProgram != NULL );
		grcHullShader *const ProgramHS = hullProgram->GetProgram();
		Assert( ProgramLS != NULL && ProgramHS != NULL );
		const uint32_t numPatches = initTessellation( ProgramLS, ProgramHS, domainProgram->GetGnmStage());
		
		gfxc.setLsHsShaders( ProgramLS, ds ? ds->Modifier : 0, ds ? ds->FetchShader : NULL, ProgramHS, numPatches );

		// From https://ps4.scedev.net/support/issue/30256/_Rare_GPU_hangs_with_tessellation,
		// it was suggested to try setting SPI_SHADER_PGM_RSRC3_VS.WAVE_LIMIT to 256/numVsExportParams.
		//
		// GNM doesn't really expose a way to get the number of export parameters from the domain shader,
		// but from CIK_3D_registers_v2.pdf,
		//
		//      +-----------------------------------------------------------------------------------------------+
		//      | SPI:SPI_VS_OUT_CONFIG � [R/W] � 32 bits � Access: 32 � GpuF0MMReg:0x286c4                     |
		//      +-----------------------------------------------------------------------------------------------+
		//      | DESCRIPTION: VS output configuration                                                          |
		//      +-----------------+------+---------+------------------------------------------------------------+
		//      | Field Name      | Bits | Default | Description                                                |
		//      +-----------------+------+---------+------------------------------------------------------------+
		//      | VS_EXPORT_COUNT |  5:1 |  0x0    | Number of vectors exported by the VS (value is minus 1)    |
		//      | VS_HALF_PACK    |  6   |  0x0    | Setting this bit causes the VGT to only load VS            |
		//      |                 |      |         | wavefronts half full of verts and the SPI to alloc/dealloc |
		//      |                 |      |         | half the param cache space for each wave. Required for     |
		//      |                 |      |         | configs with > 1 quad pipe when                            |
		//      |                 |      |         | (((VS_EXPORT_COUNT + 1) *                                  |
		//      |                 |      |         | GPU__GC__QP_PER_SIMD * 2 ) >                               |
		//      |                 |      |         | GPU__SX__PARAMETER_CACHE_DEPTH)                            |
		//      +-----------------+------+---------+------------------------------------------------------------+
		//
		u32 numVsExportParams;
		FatalAssert(!domainProgram->IsEsShader());
		numVsExportParams = ((((sce::Gnmx::VsShader*)(domainProgram->GetProgram()))->m_vsStageRegisters.m_spiVsOutConfig&0x3e)>>1)+1;
		vsWaveLimit = 256/numVsExportParams;
		vsWaveLimit = vsWaveLimit>0x3f ? 0 : vsWaveLimit;     // SPI_SHADER_PGM_RSRC3_VS.WAVE_LIMIT has 6 bits, 0 means no limit
	}
	else
		gfxc.setVsShader( ProgramVS, ds ? ds->Modifier : 0, ds ? ds->FetchShader : NULL );
	gfxc.setVsWaveLimit(vsWaveLimit);
# endif
#endif
	CurrentProgram->BindCommon( GetGnmStage(), VS_TYPE );
}


void grcFragmentProgram::SetParameter(int address,const float* value,int count,u32 offset,grcCBuffer *cbuffer)
{
#if MULTIPLE_RENDER_THREADS
	if (!g_IsSubRenderThread) {
		for (int i=0; i<MULTIPLE_RENDER_THREADS; i++)
			sysMemCpy((char*)cbuffer->GetDataPtrForThread(i) + offset,value,count << 4);
		cbuffer->SetDirtyAllThreads();
	}
	else
#endif
	{
		sysMemCpy((char*)cbuffer->GetDataPtr() + offset,value,count << 4);
		cbuffer->SetDirtySingleThread();
	}
}


void grcFragmentProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
	SetTextureCommon(sce::Gnm::kShaderStagePs,address,data,stateHandle);
}

void grcFragmentProgram::Bind() const
{
	GRCDEVICE.SetAASuperSample(IsPerSample);
#if __ASSERT
	grcTextureFactoryGNM::CheckOutputFormat(ColorOutputFormat);
#endif
#if ENABLE_LCUE
	gfxc.setPsShader(Program, SRO);
#else
	gfxc.setPsShader(Program);
	BindCommon(GetGnmStage(), PS_TYPE);
#endif
}


void grcComputeProgram::Bind() const
{
#if ENABLE_LCUE
	gfxc.setCsShader(Program,SRO);
#else
	gfxc.setCsShader(Program);
	BindCommon(GetGnmStage(), CS_TYPE);
#endif
}

void grcComputeProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
	SetTextureCommon(sce::Gnm::kShaderStageCs,address,data,stateHandle);
}


void grcDomainProgram::Bind() const
{
	CurrentProgram = this;
#if ENABLE_LCUE
	if (bIsEsShader)
		gfxc.setEsShader(ProgramES,SRO);
	else
		gfxc.setVsShader(ProgramVS,SRO);
#else
	if (bIsEsShader)
		gfxc.setEsShader(ProgramES,0,NULL);
	else
		gfxc.setVsShader(ProgramVS,0,NULL);

	BindCommon(GetGnmStage(), DS_TYPE);
#endif
}

void grcDomainProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle) const
{
	SetTextureCommon(GetGnmStage(),address,data,stateHandle);
}


void grcGeometryProgram::Bind() const
{
#if ENABLE_LCUE
	//gfxc.setGsShader(Program,SRO);
	Assert(false);
#else
	if (Program)
	{
		gfxc.setGsVsShaders(Program);
		BindCommon(GetGnmStage(), GS_TYPE);
	}
#endif
}

void grcGeometryProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
	SetTextureCommon(sce::Gnm::kShaderStageGs,address,data,stateHandle);
}



void grcHullProgram::Bind() const
{
	CurrentProgram = this;

	if (!Program)
	{
		return;
	}

#if ENABLE_LCUE
	gfxc.setHsShader( Program, SRO );
#else
	BindCommon(GetGnmStage(), HS_TYPE);
#endif	//ENABLE_LCUE
}

void grcHullProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
	SetTextureCommon(sce::Gnm::kShaderStageHs,address,data,stateHandle);
}



void grcEffect::InitClass()
{
	const sce::Gnm::SizeAlign nullSizeAlign = { 4, 4 };
	sEffectGnm_NullUAVStructuredBuffer.Allocate(nullSizeAlign, true, NULL, 4);
}

void grcEffect::ShutdownClass()
{
}

grcCBuffer::grcCBuffer()
{
	MaxSize = 0;
	sysMemSet(CurrSize, 0, sizeof(CurrSize));
	sysMemSet(BackingStore, 0, sizeof(BackingStore));
	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
	sysMemSet(Buffer,0,sizeof(Buffer));
	ExplicitBind = false;
}

grcCBuffer::grcCBuffer(u32 size, bool directLockOnly)
{
	MaxSize = size;
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = size;

	if(directLockOnly == false)
	{
		BackingStore[0] = rage_new char[size * NUMBER_OF_RENDER_THREADS];
		sysMemSet(BackingStore[0], 0, size);
		for (unsigned i=1; i<NELEM(BackingStore); ++i)
			BackingStore[i] = BackingStore[i-1]+size;
	}
	else
		sysMemSet(BackingStore, 0, sizeof(BackingStore));

	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
	sysMemSet(Buffer,0,sizeof(Buffer));
	ExplicitBind = false;
}


grcCBuffer::~grcCBuffer()
{
	if(BackingStore[0])
		delete[] BackingStore[0];
}

void grcCBuffer::Init(bool)
{
}

void grcCBuffer::Load(fiStream &S)
{
	S.ReadInt(&MaxSize,1);
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = MaxSize;
	S.ReadShort(Registers,6);

	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer,count);
	Name = buffer;

	grcDebugf2("CBuffer::Load - %s, register VS %d PS %d, size %d", buffer, Registers[0], Registers[1], MaxSize);
	NameHash = atStringHash(Name);

	BackingStore[0] = rage_new char[MaxSize * NUMBER_OF_RENDER_THREADS];
	sysMemSet(BackingStore[0], 0, MaxSize);
	for (unsigned i=1; i<NELEM(BackingStore); ++i)
		BackingStore[i] = BackingStore[i-1]+MaxSize;
}

void grcCBuffer::operator=(const grcCBuffer &rhs)
{
	if(BackingStore[0])
		delete[] BackingStore[0];
	MaxSize = rhs.MaxSize;
	for (unsigned i=0; i<NELEM(CurrSize); ++i)
		CurrSize[i] = MaxSize;
	sysMemCpy(Registers,rhs.Registers,sizeof(Registers));
	NameHash = rhs.NameHash;
	Name = rhs.Name;
	if(rhs.BackingStore[0])
	{
		BackingStore[0] = rage_new char[MaxSize * NUMBER_OF_RENDER_THREADS];
		sysMemCpy(BackingStore[0], rhs.BackingStore[0], MaxSize);
		for (unsigned i=1; i<NELEM(BackingStore); ++i)
			BackingStore[i] = BackingStore[i-1]+MaxSize;
	}
}

void grcCBuffer::SetBackingStoreThread(void *backingStore, u32 threadIdx)
{
	BackingStore[threadIdx] = (char*)backingStore;
}

void grcCBuffer::SetBackingStore(void *backingStore)
{
	if (g_IsSubRenderThread)
		SetBackingStoreThread(backingStore, g_RenderThreadIndex);
	else for (unsigned i=0; i<NELEM(BackingStore); ++i)
		SetBackingStoreThread(backingStore, i);
}

void grcCBuffer::SetDirtySingleThread()
{
	AllocScopeId[g_RenderThreadIndex] = grcContextAllocScope::ID_INVALID;
}

void grcCBuffer::SetDirtyAllThreads()
{
	CompileTimeAssert(grcContextAllocScope::ID_INVALID == 0);
	sysMemSet(AllocScopeId, 0, sizeof(AllocScopeId));
}

void grcCBuffer::SetDirty()
{
	if (g_IsSubRenderThread)
		SetDirtySingleThread();
	else
		SetDirtyAllThreads();
}

bool grcCBuffer::GetDirty()
{
	return !grcGfxContext::isAllocScopeValid(AllocScopeId[g_RenderThreadIndex]);
}

void grcCBuffer::ResetDirty()
{
	AllocScopeId[g_RenderThreadIndex] = grcGfxContext::getAllocScopeId();
}

#if MULTIPLE_RENDER_THREADS
void *grcCBuffer::GetDataPtrForThread(u32 idx)
{
	return BackingStore[idx];
}
#else
void *grcCBuffer::GetDataPtr()
{
	return BackingStore[0];
}
#endif
		
bool grcCBuffer::Unlock()
{
	SetDirtySingleThread();
	return true;
}

void *grcCBuffer::BeginUpdate(u32 sizeBytes)
{
	void *result = gfxc.allocateFromCommandBuffer(sizeBytes, sce::Gnm::kEmbeddedDataAlignment16);
	if (g_IsSubRenderThread) {
		const unsigned rti = g_RenderThreadIndex;
		grcGfxContext::initAsConstantBuffer(Buffer[rti],result,sizeBytes);
		AllocScopeId[rti] = grcGfxContext::getAllocScopeId();
#if !__PROFILE && !__FINAL
		// Set to NULL to catch any case where BindCommon tries to copy from here.
		BackingStore[rti] = NULL;
#endif
	}
	else for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++) {
		grcGfxContext::initAsConstantBuffer(Buffer[i],result,sizeBytes);
		// TODO: This is a bit wrong.  Really need a way for defered contexts to check allocation scope id of main context.
		AllocScopeId[i] = grcContextAllocScope::ID_INFINITE;
		BackingStore[i] = NULL;
#if !__PROFILE && !__FINAL
		BackingStore[i] = NULL;
#endif
	}
	return result;
}

void grcCBuffer::EndUpdate()
{
	// If we're in a draw, we need to flush the parameters directly.
	if (grcEffect::IsInDraw() || GetIsExplicitBind()) {
		Assertf(grcVertexProgram::GetCurrent(), "no currently bound vs.\n");
#if ENABLE_LCUE
		if (gfxc.isConstantBufferUsed(grcVertexProgram::GetCurrent()->GetGnmStage(),Registers[VS_TYPE]))
#endif
			gfxc.setConstantBuffers(grcVertexProgram::GetCurrent()->GetGnmStage(),Registers[VS_TYPE],1,GetBuffer());
#if ENABLE_LCUE
		if (gfxc.isConstantBufferUsed(sce::Gnm::kShaderStagePs,Registers[PS_TYPE]))
#endif
			gfxc.setConstantBuffers(sce::Gnm::kShaderStagePs,Registers[PS_TYPE],1,GetBuffer());
	}
}

void grcCBuffer::SetDataDirect(const void *data, u32 sizeBytes)
{
	initAsConstantBuffer(Buffer[g_RenderThreadIndex],const_cast<void*>(data),sizeBytes);
	AllocScopeId[g_RenderThreadIndex] = grcContextAllocScope::ID_INFINITE;
}

void grcCBuffer::SetDataIndirect(const void *data, u32 sizeBytes)
{
	const unsigned rti = g_RenderThreadIndex;
	BackingStore[rti] = (char*)data;
	AllocScopeId[rti] = grcContextAllocScope::ID_INVALID;
	CurrSize[rti] = sizeBytes;
}

void grcEffect::ClearCachedState()
{
	sm_CurrentPass = NULL;
	sm_CurrentBind = NULL;
}

#if SUPPORT_UAV
void BufferCounterTracker::Update(const u32 slot, grcBufferUAV *const buf, int initCount)	{
	if(buf == NULL)
		return;

	if (initCount>=0 && !buf->m_GdsCounterOffset)
	{
		buf->m_GdsCounterOffset = 4*slot;
	}
	const u32 newBase = buf->m_GdsCounterOffset - 4*slot;
	Assert( newBase < 0x8000 );
	if (m_Count)
	{
		Assertf( m_Base == newBase, "Conflicting GDS offset specified for your Append/Consume buffer" );
		if (m_Count <= slot)
			m_Count = slot+1;
	}else
	{
		m_Base = newBase;
		m_Count = slot+1;
	}
}
#endif	//SUPPORT_UAV

}	// namespace rage

#endif	// __GNM

