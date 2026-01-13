//
// grcore/effect_render.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "effect.h"
#include "effect_config.h"

#if !__SPU
#include "texture.h"
#endif

#if __D3D11 && EFFECT_PRESERVE_STRINGS && ENABLE_PIX_TAGGING
#include "profile/timebars.h"
#endif

#if RSG_ORBIS
#include "grcore/gfxcontext_gnm.h"
#endif
#if EFFECT_TRACK_COMPARISON_ERRORS
#include "grcore/image.h"
#endif

#if EFFECT_CHECK_ENTRY_TYPES
static void CheckEntryType(const grcInstanceData::Entry &entry, grcInstanceData::EntryType type)
{
	grcAssertf(entry.Type == type, "Entry %p, has been set with type %u, but expected entry type (%u)", &entry, entry.Type, type);
}
#else
# define CheckEntryType(entry, type)	do{} while(0)
#endif

namespace rage {

struct grcSamplerState;
grcSamplerState *g_SamplerStates;
int g_SamplerStateCount;
#if MULTIPLE_RENDER_THREADS
__THREAD u8 g_RenderThreadIndex;
__THREAD bool g_IsSubRenderThread = false;
#endif

#if __ASSERT
extern bool g_bDoNullTextureOnBindCheck;
#endif

#if __ASSERT
DECLARE_MTR_THREAD bool g_bIgnoreNullTextureOnBind = false;
#endif // __ASSERT

DECLARE_MTR_THREAD const grcEffect::Technique *grcEffect::sm_CurrentTechnique;
DECLARE_MTR_THREAD grcEffectTechnique grcEffect::sm_CurrentTechniqueHandle;
DECLARE_MTR_THREAD int grcEffect::sm_CurrentPassIndex;
DECLARE_MTR_THREAD const grcEffect::Technique::Pass *grcEffect::sm_CurrentPass;
DECLARE_MTR_THREAD const grcEffect::Technique::Pass *grcEffect::sm_CurrentBind;
ASSERT_ONLY(DECLARE_MTR_THREAD const grcEffect* grcEffect::sm_CurrentEffect);
DECLARE_MTR_THREAD bool grcEffect::sm_RestoreState;

#if __PS3 && !__SPU
extern u16 g_VertexShaderInputs;
extern u32 g_GcmInitBufferSize;
BANK_ONLY( extern u8 g_MinRegisterCount ); // Needed for shmoo'ing
#endif


#if RSG_PC || RSG_DURANGO || RSG_ORBIS	// Well, really DX11...

#if !defined ENABLE_LCUE
#define  ENABLE_LCUE 0
#endif // !defined ENABLE_LCUE

#if __D3D11 || ENABLE_LCUE

grcProgramResource s_GlobalProgramResources[NUMBER_OF_RENDER_THREADS][EFFECT_TEXTURE_COUNT];
u16 s_GlobalProgramSamplers[NUMBER_OF_RENDER_THREADS][EFFECT_SAMPLER_COUNT];

void grcProgram::SetGlobalTexture(int index,  void *pTexture, int stateHandle)
{
	if (stateHandle != INVALID_STATEBLOCK && AssertVerify(index < EFFECT_SAMPLER_COUNT))
	{
		if(!g_IsSubRenderThread)
		{
			for(int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			{
				s_GlobalProgramSamplers[i][index] = (u16)stateHandle;
			}
		}
		else
		{
			s_GlobalProgramSamplers[g_RenderThreadIndex][index] = (u16)stateHandle;
		}
	}

	if (AssertVerify(index < EFFECT_TEXTURE_COUNT))
	{
		if(!g_IsSubRenderThread)
		{
			for(int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
			{
				s_GlobalProgramResources[i][index].pAny = pTexture;
			}
		}
		else
		{
			s_GlobalProgramResources[g_RenderThreadIndex][index].pAny = pTexture;
		}
	}
}

#endif // __D3D11 || ENABLE_LCUE


void grcComputeProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
#if EFFECT_BIND_UNORDERED_RESOURCES
	data.Bind<grcComputeProgram,USAGE_COMPUTEPROGRAM>(*this,locals);
#elif !(__TOOL || __RESOURCECOMPILER)
	data.BindCompute(*this,locals);
#else
	(void)data;
	(void)locals;
#endif // EFFECT_BIND_UNORDERED_RESOURCES

#if RSG_PC
#if EFFECT_USE_CONSTANTBUFFERS && !(__TOOL || __RESOURCECOMPILER) && !RSG_ORBIS	//Apparently this isn't defined on Orbis.
	SetConstantBuffer<grcComputeProgram>();
#endif
#endif

#if __D3D11 && !(__TOOL || __RESOURCECOMPILER)
	SetUnorderedResource();
#endif
}

inline void grcDomainProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
	data.Bind<grcDomainProgram,USAGE_DOMAINPROGRAM>(*this,locals); 
}

inline void grcGeometryProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
	data.Bind<grcGeometryProgram,USAGE_GEOMETRYPROGRAM>(*this,locals); 
}

inline void grcHullProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
	data.Bind<grcHullProgram,USAGE_HULLPROGRAM>(*this,locals); 
}

#endif	//platforms

#if !__PPU
#if __PS3 && __SPU
#define s_FatTexturesBound (pSpuGcmState->FatTexturesBound)
#define s_LastTextureOffsets (pSpuGcmState->LastTextureOffsets)
#else
#endif

#define ENABLE_TEXTURE_CHECK	 (EFFECT_TEXTURE_SUBTYPING && __ASSERT)
#if ENABLE_TEXTURE_CHECK
static void CheckTextureOnBind(u8 slot, const grcTexture *texture, const u16 stateHandle, const char *programName, const char *varName, int expectMSAA, int expectComparison)
{
	const char *textureName = texture ? texture->GetName() : "_null_";
	grcAssertf( texture || (g_bIgnoreNullTextureOnBind == true) || (g_bDoNullTextureOnBindCheck == false),
		"No texture bound to slot %d (%s) of %s", slot, varName, programName );
	(void)textureName;
#if __ASSERT && 0
	// you can assert on different combination of variable / texture name at will
	grcAssertf((textureName && strstr(textureName,"Stencil") != NULL) == (strstr(varName,"Stencil") != NULL),
		"Stencil texture mismatch: shader %s expects %s, while given texture is %s",
		programName, varName, textureName);
#endif
#if EFFECT_TRACK_MSAA_ERRORS
	grcAssertf( expectMSAA<0 || ((expectMSAA>0) == (texture && texture->GetMSAA())),
		"MSAA mismatch at slot %d (%s): shader %s expects %s, while given texture %s is %s", 
		slot, varName, programName, expectMSAA ? "MSAA" : "non-MSAA", 
		textureName, texture && texture->GetMSAA() ? "MSAA" : "non-MSAA" );
#else
	(void)expectMSAA;
#endif //EFFECT_TRACK_MSAA_ERRORS
#if EFFECT_TRACK_COMPARISON_ERRORS
	if (expectComparison>=0 && stateHandle != INVALID_STATEBLOCK)
	{
		const bool isGivenComparison = grcStateBlock::IsComparisonSampler(stateHandle);
		grcAssertf( (expectComparison>0) == isGivenComparison,
			"Sample comparison mismatch at slot %d (%s): shader %s expects %s, while given sampler %s is %s",
			slot, varName, programName, expectComparison ? "CMP" : "non-CMP",
			textureName, isGivenComparison ? "CMP" : "non-CMP" );
		if (texture)
		{
			const grcImage::Format format = static_cast<grcImage::Format>(texture->GetImageFormat());
			grcAssertf( !isGivenComparison || (texture && grcImage::IsFormatDepthCompatible(format)),
				"Sample comparison mismatch at slot %d (%s) in shader %s: texture doesn't have a depth-compatible format (%d), while given sampler %s is comparison",
				slot, varName, programName, format, textureName);
		}
	}
#else
	(void)stateHandle; (void)expectComparison;
#endif //EFFECT_TRACK_COMPARISON_ERRORS
}
#endif //ENABLE_TEXTURE_CHECK


#if RSG_PC || RSG_DURANGO || RSG_ORBIS
template <class Derived,int mask> void grcInstanceData::Bind(const Derived &program, const atArray <grcParameter> &locals) const 
#else
template <class Derived,int mask> void grcInstanceData::Bind(const Derived &program) const 
#endif
{
#if RSG_ORBIS && ENABLE_LCUE
	program.Bind();
#endif

#if !(__D3D11 || RSG_ORBIS)
	int count = program.m_Constants.GetCount();
	for (int ii=0; ii<count; ii++) 
	{
		int localIndex = program.m_Constants[ii]; 
		Entry &e = Data()[localIndex];

		if (e.Count)
		{
		#if RSG_PC
			program.SetLocalParameter(e.Register,e.Float,e.Count,locals[localIndex].GetCBufOffset(),locals[localIndex].GetParentCBuf(), locals[localIndex].GetType());
		#else
			program.SetParameter(e.Register,e.Float,e.Count);
		#endif
		}
		else
		{
		#if __PS3
			// Remember the fat textures
			if (mask == USAGE_FRAGMENTPROGRAM && e.Texture) 
			{
		# if USE_PACKED_GCMTEX
				u8 format = (e.Texture->format >> 8) & ~(CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_UN);
		# else
				u8 format = e.Texture->GetGcmTexture().format & ~(CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_UN);
		# endif
				if (format == CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT || format == CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT)
					s_FatTexturesBound |= (1 << e.Register);
			}

		# if USE_PACKED_GCMTEX
		# if TRAP_UNMARKED_TEXTURES
			if (Unlikely(e.Texture == (grcTextureObject*)-1)) __debugbreak();
		# endif
			program.SetTexture(e.Register,e.Texture,e.SamplerStateSet);

		# else // USE_PACKED_GCMTEX 

		# if TRAP_UNMARKED_TEXTURES
			if (Unlikely(e.Texture == (spuTexture*)-1)) __debugbreak();
		# endif
			program.SetTexture(e.Register,e.Texture?&e.Texture->GetGcmTexture():NULL,e.SamplerStateSet);
		# endif // USE_PACKED_GCMTEX

		#elif __XENON
			{
				program.SetTexture(e.Register,e.Texture->GetCachedTexturePtr(),e.SamplerStateSet);
			}
		#else
			{
				program.SetTexture(e.Register,e.Texture,e.SamplerStateSet);
			}
		#endif
		}
	}

#else // !(__D3D11 || RSG_ORBIS)
	#if EFFECT_BIND_UNORDERED_RESOURCES
		const sce::Gnm::ShaderStage stage = program.GetGnmStage();
		BufferCounterTracker countTracker;
	#endif // EFFECT_BIND_UNORDERED_RESOURCES

	int count = program.m_Constants.GetCount();
	for (int ii=0; ii<count; ii++) 
	{
		int localIndex = program.m_Constants[ii]; 

	#if (RSG_ORBIS && ENABLE_LCUE) || __D3D11
		if (localIndex & SPECIAL_GLOBAL_TYPE_MASK)
		{
			int index = localIndex & SPECIAL_GLOBAL_INDEX_MASK;
			switch(localIndex & SPECIAL_GLOBAL_TYPE_MASK)
			{
			case SPECIAL_GLOBAL_VT_TEXTURE:
				{
					const u16 sampler = (index >= EFFECT_SAMPLER_COUNT ? INVALID_STATEBLOCK : s_GlobalProgramSamplers[g_RenderThreadIndex][index]);
				#if ENABLE_TEXTURE_CHECK
					// There is a problem - we don't have the shader variable name here. We could iterate through globals to match
					// the register, but what if there are multiple matches? Leaving it for TODO
					const TextureSubtype subType = static_cast<TextureSubtype>((localIndex & SPECIAL_GLOBAL_SUBTYPE_MASK) >> SPECIAL_GLOBAL_SUBTYPE_SHIFT);
					const int isComparison = (subType == TEXTURE_SAMPLER) || (subType == TEXTURE_REGULAR) ?
						((localIndex >> SPECIAL_GLOBAL_SUBTYPE_SHIFT) & SPECIAL_TEMP_SUBTYPE_SAMPLER_BIT) ? 1 : 0
						: -1;	// no sampler needed
					CheckTextureOnBind( static_cast<u8>(index),
						s_GlobalProgramResources[g_RenderThreadIndex][index].GetTexPtr(),
						sampler,
						program.GetEntryName(),
					#if EFFECT_TRACK_GLOBALS
						grcEffect::GetCBufferGlobal(program.GetRegisterToGlobalTable()[index])->GetName(),
					#else
						"_global_",	
					#endif
						subType == TEXTURE_MSAA ? 1 : 0,
						isComparison
						);
				#endif //ENABLE_TEXTURE_CHECK
				#if __D3D11
					program.UpdateGlobalTexture(index, s_GlobalProgramResources[g_RenderThreadIndex][index].GetTexPtr(), sampler);
				#else
					program.UpdateGlobalTexture(index, s_GlobalProgramResources[g_RenderThreadIndex][index].GetTexPtr()->GetCachedTexturePtr(), sampler);
				#endif
					break;
				}
			case SPECIAL_GLOBAL_VT_STRUCTUREDBUFFER:
				{
					grcAssertf(false, "grcInstanceData::Bind()...Can`t do global structured buffers yet.");
					break;
				}
			}
			continue;
		}
	#endif // RSG_ORBIS && ENABLE_LCUE || __D3D11

		Entry &e = Data()[localIndex];
		const grcParameter& local = locals[localIndex];
		const u8 localType = local.GetType();
		const grcEffect::VarType varType = static_cast<grcEffect::VarType>( localType );
		const u32 cbOffset = local.GetCBufOffset();

		switch (varType)
		{
		case grcEffect::VT_TEXTURE:
			{
				CheckEntryType(e, ET_TEXTURE);
			#if ENABLE_TEXTURE_CHECK
				CheckTextureOnBind( static_cast<u8>(cbOffset),
					e.Texture, e.SamplerStateSet,
					program.GetEntryName(), local.GetName(),
					local.IsMsaaTexture(), local.IsComparisonFilter() );
			#endif
			#if __D3D11
				program.SetTexture(cbOffset,e.Texture,e.SamplerStateSet);
			#else // __D3D11
				program.SetTexture(e.Register,e.Texture->GetCachedTexturePtr(),e.SamplerStateSet);
			#endif // __D3D11
				break;
			}
	#if EFFECT_BIND_UNORDERED_RESOURCES
		case grcEffect::VT_STRUCTUREDBUFFER:
			{
				CheckEntryType(e, ET_BUFFER);
				if (locals[localIndex].IsStackBuffer())
				{
					countTracker.Update(cbOffset, e.RW_Buffer, -1);	//hack
				}
				program.SetStructuredBuffer(stage, cbOffset, e.RO_Buffer);
				break;
			}
		case grcEffect::VT_UAV_STRUCTURED:
			{
				CheckEntryType(e, ET_BUFFER);
				int counterInit = -1;
				if (locals[localIndex].IsStackBuffer())
				{
					counterInit = program.m_pTexContainers[cbOffset + (program.m_TexEndSlot+1)]->GetStructuredBufferInitCount();
					countTracker.Update(cbOffset, e.RW_Buffer, counterInit);
				}
				program.SetStructuredBufferUAV(stage, cbOffset, e.RW_Buffer, counterInit);
				break;
			}
		case grcEffect::VT_UAV_TEXTURE:
			{
				CheckEntryType(e, ET_TEXTURE);
				program.SetUnorderedTextureUAV(stage, cbOffset, e.RW_Texture);
				break;
			}
		case grcEffect::VT_BYTEADDRESSBUFFER:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetDataBuffer(stage, cbOffset, e.RO_Buffer);
				break;
			}
		case grcEffect::VT_UAV_BYTEADDRESS:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetDataBufferUAV(stage, cbOffset, e.RW_Buffer);
			}
	#else	// EFFECT_BIND_UNORDERED_RESOURCES
		case grcEffect::VT_STRUCTUREDBUFFER:
		case grcEffect::VT_UAV_STRUCTURED:
			{
				// Note that CheckEntryType cannot (currently) be called here safely.  It is
				// lame, but some code will have garbage in e.RO_Buffer, yet later sets
				// things up correctly SetRenderTargetsWithUAV.  The garbage never makes it
				// though to D3D.
				program.SetStructuredBuffer(cbOffset, e.RO_Buffer);
				break;
			}
		case grcEffect::VT_UAV_TEXTURE:
		case grcEffect::VT_BYTEADDRESSBUFFER:
		case grcEffect::VT_UAV_BYTEADDRESS:
			Assertf(0, "UAV Resource unexpected here");
	#endif	// EFFECT_BIND_UNORDERED_RESOURCES
		default:
			{
				CheckEntryType(e, ET_FLOAT);
			# if TRACK_CONSTANT_BUFFER_CHANGES
				bool added = program.SetLocalParameter(e.Register,e.Float,e.Count,cbOffset,local.GetParentCBuf(), localType);
				if (added && grcEffect::sm_TrackConstantBufferUsage)
					local.IncrementUsageCount();
			# else	// TRACK_CONSTANT_BUFFER_CHANGES
				program.SetLocalParameter(e.Register,e.Float,e.Count,cbOffset,local.GetParentCBuf(), localType);
			# endif	// TRACK_CONSTANT_BUFFER_CHANGES
				break;
			}
		}
	}
	#if EFFECT_BIND_UNORDERED_RESOURCES
		if (countTracker.m_Count)
			program.SetNumAppendConsumeCounters( stage, countTracker.m_Base, countTracker.m_Count );
	#endif	//EFFECT_BIND_UNORDERED_RESOURCES
#endif // !(__D3D11 || RSG_ORBIS)

#if (RSG_ORBIS && ENABLE_LCUE)
	program.BindConstants();
#else
	program.Bind();
#endif
}


#if __D3D11 || RSG_ORBIS
void grcInstanceData::BindCompute(const grcComputeProgram &program,const atArray <grcParameter> &locals) const 
{
#if RSG_ORBIS && ENABLE_LCUE
	program.Bind();
#endif

	int count = program.m_Constants.GetCount();
	for (int ii=0; ii<count; ii++) 
	{
		int localIndex = program.m_Constants[ii]; 
		Entry &e = Data()[localIndex];
		const grcEffect::VarType varType = static_cast<grcEffect::VarType>( locals[localIndex].GetType() );

		switch (varType)
		{
		case grcEffect::VT_TEXTURE:
			{
				CheckEntryType(e, ET_TEXTURE);
			#if __D3D11
				program.SetTexture(locals[localIndex].GetCBufOffset(),e.Texture,e.SamplerStateSet);
			#else // __D3D11
				program.SetTexture(e.Register,e.Texture->GetCachedTexturePtr(),e.SamplerStateSet);
			#endif // __D3D11
				break;
			}
	#if SUPPORT_UAV && !EFFECT_BIND_UNORDERED_RESOURCES
		case grcEffect::VT_STRUCTUREDBUFFER:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetStructuredBuffer(locals[localIndex].GetCBufOffset(), e.RO_Buffer);
				break;
			}
		case grcEffect::VT_UAV_STRUCTURED:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetStructuredBufferUAV(locals[localIndex].GetCBufOffset(), e.RW_Buffer, program.m_pTexContainers[locals[localIndex].GetCBufOffset() + (program.m_TexEndSlot+1)]->GetStructuredBufferInitCount());
				break;
			}
		case grcEffect::VT_UAV_TEXTURE:
			{
				CheckEntryType(e, ET_TEXTURE);
				program.SetUnorderedTextureUAV(locals[localIndex].GetCBufOffset(), e.RW_Texture);
				break;
			}
		case grcEffect::VT_BYTEADDRESSBUFFER:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetDataBuffer(locals[localIndex].GetCBufOffset(), e.RO_Buffer);
				break;
			}
		case grcEffect::VT_UAV_BYTEADDRESS:
			{
				CheckEntryType(e, ET_BUFFER);
				program.SetDataBufferUAV(locals[localIndex].GetCBufOffset(), e.RW_Buffer);
			}
	#endif // SUPPORT_UAV && !EFFECT_BIND_UNORDERED_RESOURCES
		default:
			{
				CheckEntryType(e, ET_FLOAT);
		# if TRACK_CONSTANT_BUFFER_CHANGES
				bool added = program.SetLocalParameter(e.Register,e.Float,e.Count,locals[localIndex].GetCBufOffset(),locals[localIndex].GetParentCBuf(), locals[localIndex].GetType());
				if (added && grcEffect::sm_TrackConstantBufferUsage)
					locals[localIndex].IncrementUsageCount();
		# else	// TRACK_CONSTANT_BUFFER_CHANGES
				program.SetLocalParameter(e.Register,e.Float,e.Count,locals[localIndex].GetCBufOffset(),locals[localIndex].GetParentCBuf(), locals[localIndex].GetType());
		# endif	// TRACK_CONSTANT_BUFFER_CHANGES
				break;
			}
		}
	}

#if !(RSG_ORBIS && ENABLE_LCUE)
	program.Bind();
#endif
}
#endif // __D3D11 || RSG_ORBIS

template <class Derived> void grcInstanceData::CopyData(const Derived &program, const grcInstanceData &source) const
{
	for (int ii=0; ii<program.m_Constants.GetCount(); ii++) 
	{
		int localIndex = program.m_Constants[ii]; 

		if (localIndex & SPECIAL_GLOBAL_TYPE_MASK)
			continue;

		Data()[localIndex] = source.Data()[localIndex];
	}
}

#endif	// !__PPU

#if RSG_PC || RSG_DURANGO || RSG_ORBIS

inline void grcVertexProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
	data.Bind<grcVertexProgram,USAGE_VERTEXPROGRAM>(*this,locals); 
}

inline void grcFragmentProgram::Bind(const grcInstanceData &data,const atArray<grcParameter> &locals) const
{ 
	data.Bind<grcFragmentProgram,USAGE_FRAGMENTPROGRAM>(*this,locals); 
}

#else

inline void grcVertexProgram::Bind(const grcInstanceData &data) const
{ 
	data.Bind<grcVertexProgram,USAGE_VERTEXPROGRAM>(*this); 
}

inline void grcFragmentProgram::Bind(const grcInstanceData &data) const
{ 
	data.Bind<grcFragmentProgram,USAGE_FRAGMENTPROGRAM>(*this); 
}

#endif		// __D3D11

void grcEffect::ApplyDefaultRenderStates()
{
	grcStateBlock::Default();
}


#if !__SPU
void grcEffect::Bind(grcEffectTechnique tech) const {
	sm_CurrentTechniqueHandle = tech;
	if (tech) {
		FastAssert(!sm_CurrentPass);
		const Technique::Pass *newBind = &m_Techniques[tech-1].Passes[0];
		if (newBind != sm_CurrentBind) {
#if GRCORE_ON_SPU > 1
			grcStateBlock::FlushAndMakeDirty();
			SPU_COMMAND(grcEffect__Bind,0);
			Assign(cmd->technique,tech);
			cmd->passIndex = 0;
			cmd->effect = const_cast<grcEffect*>(this);
			cmd->instanceData = const_cast<grcInstanceData*>(&m_InstanceData);
			BANK_ONLY( cmd->minRegisterCount = g_MinRegisterCount ); // Needed for shmoo'ing

			// Update this now so that vertex declaration binds work as expected.
			g_VertexShaderInputs = m_VertexPrograms[(sm_CurrentBind = newBind)->VertexProgramIndex].Configuration.attributeInputMask;

			// TODO: May be faster to just unconditionally invalidate all of these?
			grcStateBlock::BS_Dirty = newBind->BlendStateHandle != INVALID_STATEBLOCK;
			grcStateBlock::RS_Dirty = newBind->RasterizerStateHandle != INVALID_STATEBLOCK;
			grcStateBlock::DSS_Dirty = newBind->DepthStencilStateHandle != INVALID_STATEBLOCK;

#else
			(sm_CurrentBind = newBind)->Bind(m_VertexPrograms[newBind->VertexProgramIndex],
											m_FragmentPrograms[newBind->FragmentProgramIndex], 
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
											m_ComputePrograms[newBind->ComputeProgramIndex], 
											m_DomainPrograms[newBind->DomainProgramIndex], 
											m_GeometryPrograms[newBind->GeometryProgramIndex],
											m_HullPrograms[newBind->HullProgramIndex],
#endif // RSG_PC || RSG_DURANGO || RSG_ORBIS
											m_InstanceData
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
											,m_Locals
#endif
											);

#endif	// GRCORE_ON_SPU
		}
	}
	else
		sm_CurrentPass = sm_CurrentBind = NULL;
}
#endif

void grcEffect::BeginPass(int passIndex,const grcInstanceData &instanceData) const
{
	ASSERT_ONLY(sm_CurrentEffect = this);
	Assert(!sm_CurrentPass);
#if __WIN32PC && __BANK
	if (!Verifyf(sm_CurrentTechnique,"Caller didn’t check BeginDraw result properly"))
		return;
#endif
	sm_CurrentPass = &sm_CurrentTechnique->Passes[sm_CurrentPassIndex = passIndex];
	Assert(sm_CurrentPass);

#if GRCORE_ON_SPU > 1
	SPU_COMMAND(grcEffect__Bind,0);
	Assign(cmd->technique, sm_CurrentTechniqueHandle);
	Assign(cmd->passIndex, passIndex);
	cmd->effect = const_cast<grcEffect*>(this);
	cmd->instanceData = const_cast<grcInstanceData*>(&instanceData);
	BANK_ONLY( cmd->minRegisterCount = g_MinRegisterCount ); // Needed for shmoo'ing

	// Update this now so that vertex declaration binds work as expected.
	// Normally it's done in Pass::Bind but we don't call that here.
	g_VertexShaderInputs = m_VertexPrograms[sm_CurrentPass->VertexProgramIndex].Configuration.attributeInputMask;
	// If we're not restoring state when we're done, invalidate cached state blocks since the SPU may have changed them.
	if (!sm_RestoreState) {
		// TODO: May be faster to just unconditionally invalidate all of these?
		grcStateBlock::BS_Dirty = sm_CurrentPass->BlendStateHandle != INVALID_STATEBLOCK;
		grcStateBlock::RS_Dirty = sm_CurrentPass->RasterizerStateHandle != INVALID_STATEBLOCK;
		grcStateBlock::DSS_Dirty = sm_CurrentPass->DepthStencilStateHandle != INVALID_STATEBLOCK;
	}
#else

	sm_CurrentPass->Bind(m_VertexPrograms[sm_CurrentPass->VertexProgramIndex],
						m_FragmentPrograms[sm_CurrentPass->FragmentProgramIndex], 
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
						m_ComputePrograms[sm_CurrentPass->ComputeProgramIndex], 
						m_DomainPrograms[sm_CurrentPass->DomainProgramIndex], 
						m_GeometryPrograms[sm_CurrentPass->GeometryProgramIndex], 
						m_HullPrograms[sm_CurrentPass->HullProgramIndex], 
#endif //RSG_PC || RSG_DURANGO || RSG_ORBIS
						instanceData
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
						,m_Locals
#endif
						);
#endif	// GRCORE_ON_SPU
}


void grcEffect::EndPass() const
{
	ASSERT_ONLY(sm_CurrentEffect = NULL);
	Assert(sm_CurrentPass != NULL);
#if GRCORE_ON_SPU > 1
	SPU_COMMAND(grcEffect__UnBind, 0);
	Assign(cmd->technique, sm_CurrentTechniqueHandle);
	Assign(cmd->passIndex, sm_CurrentPassIndex);
	cmd->effect = const_cast<grcEffect*>(this);
#else
	sm_CurrentPass->UnBind(sm_RestoreState);
#endif
	sm_CurrentPass = NULL;
}

void grcEffect::CopyPassData(grcEffectTechnique techHandle, int passIndex, const grcInstanceData &instanceData, const grcInstanceData &sourceData) const
{
	if (!AssertVerify(this) ||
		!AssertVerify(0 < techHandle && techHandle <= m_Techniques.GetCount()) ||
		!AssertVerify(0 <= passIndex && passIndex < m_Techniques[techHandle-1].Passes.GetCount()))
	{
		return;
	}
	
	const grcEffect_Technique_Pass &pass = m_Techniques[techHandle-1].Passes[passIndex];
	instanceData.CopyData(m_VertexPrograms[pass.VertexProgramIndex],		sourceData);
	instanceData.CopyData(m_FragmentPrograms[pass.FragmentProgramIndex],	sourceData);
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	instanceData.CopyData(m_ComputePrograms[pass.ComputeProgramIndex],		sourceData);
	instanceData.CopyData(m_DomainPrograms[pass.DomainProgramIndex],		sourceData);
	instanceData.CopyData(m_GeometryPrograms[pass.GeometryProgramIndex],	sourceData);
	instanceData.CopyData(m_HullPrograms[pass.HullProgramIndex],			sourceData);
#endif //RSG_PC || RSG_DURANGO || RSG_ORBIS
}

#if RSG_ORBIS
enum PassDirtyState
{
	PASS_GEOMETRY		= 1<<0,
	PASS_TESSELATION	= 1<<1,
};
static DECLARE_MTR_THREAD uint32_t s_PassDirtyState = 0;
#endif	//RSG_ORBIS

#if __BANK
DECLARE_MTR_THREAD bool grcEffect_Technique_Pass::ms_bEnableShaderRS = true;
#endif

#if (GRCORE_ON_SPU <= 1)
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
void grcEffect::Technique::Pass::Bind(const grcVertexProgram& vp,const grcFragmentProgram& fp, const grcComputeProgram &cp, const grcDomainProgram &dp, const grcGeometryProgram &gp, const grcHullProgram &hp, const grcInstanceData &instanceData, const atArray<grcParameter>& locals) const
#else
void grcEffect::Technique::Pass::Bind(const grcVertexProgram& vp,const grcFragmentProgram& fp, const grcInstanceData &instanceData) const
#endif
{
	ValidateSpuPtr(this);
	ValidateSpuPtr(&vp);
	ValidateSpuPtr(&fp);

#if __D3D11 && EFFECT_PRESERVE_STRINGS && ENABLE_PIX_TAGGING && RAGE_TIMEBARS
	if (PIXIsGPUCapturingCached()) 
	{
		static char tmp[512];
		const char *const fullPSname = fp.GetEntryName();
		const char *const shortPSname = strchr(fullPSname,':');
		snprintf(tmp, sizeof(tmp), "Tech=%s Pass=%s/%s",
			grcEffect::GetCurrentTechniqueName(), vp.GetEntryName(),
			shortPSname ? shortPSname+1 : fullPSname);
		pfAutoMarker pf(tmp,0);
	}
#endif	// RSG_PC && EFFECT_PRESERVE_STRINGS && ENABLE_PIX_TAGGING

#if __BANK
	if(ms_bEnableShaderRS)
	{
#endif
		// Send down render states.
		using namespace grcStateBlock;
		if (RS_Active  != RasterizerStateHandle) {
			if (RasterizerStateHandle != INVALID_STATEBLOCK) {
				RS_Previous = RS_Active;
				SetRasterizerState((grcRasterizerStateHandle)RasterizerStateHandle);		
			}
			else
				RS_Previous = RS_Invalid;
		}

		if (DSS_Active  != DepthStencilStateHandle || ActiveStencilRef != StencilRef) {
			if (DepthStencilStateHandle != INVALID_STATEBLOCK) {
				DSS_Previous = DSS_Active;
				PreviousStencilRef = ActiveStencilRef;
				u8 ref = StencilRef;
#if EFFECT_STENCIL_REF_MASK
				// only affecting the bits selected by GetStencilRefMask
				ref = (StencilRef & grcEffect::GetStencilRefMask()) | (ActiveStencilRef & ~grcEffect::GetStencilRefMask());
#endif //EFFECT_STENCIL_REF_MASK
				SetDepthStencilState((grcDepthStencilStateHandle)DepthStencilStateHandle, ref);
			}
			else
				DSS_Previous = DSS_Invalid;
		}

		if (BS_Active  != BlendStateHandle) {
			if (BlendStateHandle != INVALID_STATEBLOCK) {
				BS_Previous = BS_Active;
				PreviousBlendFactors = ActiveBlendFactors;
				PreviousSampleMask = ActiveSampleMask;
				SetBlendState((grcBlendStateHandle)BlendStateHandle,ActiveBlendFactors,ActiveSampleMask);
			}
			else
				BS_Previous = BS_Invalid;
		}
#if __BANK
	}	//ms_bEnableShaderRS...
#endif

#if __PPU && GCM_REPLAY && EFFECT_PRESERVE_STRINGS
	IS_CAPTURING_REPLAY_DECL
	if (IS_CAPTURING_REPLAY)
		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,vp.GetEntryName());
#endif

#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	vp.Bind(instanceData,locals);
#else
	vp.Bind(instanceData);
#endif
#if __PPU && GCM_REPLAY && EFFECT_PRESERVE_STRINGS
	if (IS_CAPTURING_REPLAY) {
		cellGcmSetPerfMonPopMarker(GCM_CONTEXT);
		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,fp.GetEntryName());
	}
#endif

#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	fp.Bind(instanceData,locals);
#else
	fp.Bind(instanceData);
#endif
#if __PPU && GCM_REPLAY && EFFECT_PRESERVE_STRINGS
	if (IS_CAPTURING_REPLAY)
		cellGcmSetPerfMonPopMarker(GCM_CONTEXT);
#endif

#if (RSG_PC || RSG_DURANGO || (RSG_ORBIS && !ENABLE_LCUE))	// NOTE - Orbis is trying to bind NULL/empty compute programs, which crashes LCUE
	cp.Bind(instanceData,locals);
	dp.Bind(instanceData,locals);
	gp.Bind(instanceData,locals);
	hp.Bind(instanceData,locals);
#endif	//RSG_PC || RSG_DURANGO || RSG_ORBIS

#if RSG_ORBIS
	Assert( !s_PassDirtyState );
	// following Slide 42/142 of the R10x_GPU_Walkthrough
	gfxc.setActiveShaderStages( dp.GetProgram() ?
		(gp.GetProgram() ? sce::Gnm::kActiveShaderStagesLsHsEsGsVsPs : sce::Gnm::kActiveShaderStagesLsHsVsPs) :
		(gp.GetProgram() ? sce::Gnm::kActiveShaderStagesEsGsVsPs : sce::Gnm::kActiveShaderStagesVsPs)
		);
	s_PassDirtyState =
		(gp.GetProgram() ? PASS_GEOMETRY	: 0)+
		(dp.GetProgram() ? PASS_TESSELATION	: 0);
#endif	//RSG_ORBIS
}
#endif		// GRCORE_ON_SPU <= 1


void grcEffect::Technique::Pass::UnBind(bool restoreState /* = true */) const
{
#if RSG_ORBIS
	if (s_PassDirtyState & PASS_GEOMETRY)
		gfxc.setGsModeOff();
	if (s_PassDirtyState & PASS_TESSELATION)
		gfxc.setVgtControl(255, sce::Gnm::kVgtPartialVsWaveDisable, sce::Gnm::kVgtSwitchOnEopDisable);
	if (s_PassDirtyState & (PASS_GEOMETRY | PASS_TESSELATION))
		gfxc.setActiveShaderStages(sce::Gnm::kActiveShaderStagesVsPs);
	s_PassDirtyState = 0;
#endif	//RG_ORBIS

	if (!restoreState)
	{
		return;
	}

#if (GRCORE_ON_SPU <= 1)
	// Restore every render stat we hit (if any) back to its current default.
	using namespace grcStateBlock;

	if (RS_Previous != RS_Invalid) {
		SetRasterizerState(RS_Previous);
		RS_Previous = RS_Invalid;
	}
	if (DSS_Previous != DSS_Invalid) {
		SetDepthStencilState(DSS_Previous,PreviousStencilRef);
		DSS_Previous = DSS_Invalid;
	}
	if (BS_Previous != BS_Invalid) {
		SetBlendState(BS_Previous,PreviousBlendFactors,PreviousSampleMask);
		BS_Previous = BS_Invalid;
	}
#endif

#if __PS3 && (GRCORE_ON_SPU <= 1)
	// Reset texture bindings for "fat" (more than 32-bits per texel) textures,
	// as they slow things down, even if not sampled from (increases the
	// fragment program register count used for distributing quads to fragment
	// pipes).
# if __PPU
	int index = 0;
	while (s_FatTexturesBound) {
		if (s_FatTexturesBound & 1)
			cellGcmSetTextureControl(GCM_CONTEXT,index,CELL_GCM_FALSE,0,0,CELL_GCM_TEXTURE_MAX_ANISO_1);
		s_FatTexturesBound >>= 1;
		++index;
	}
# else
	int index = 0;
	while (s_FatTexturesBound) {
		if (s_FatTexturesBound & 1) {
			cell::Gcm::Inline::cellGcmSetTextureControl(GCM_CONTEXT,index,CELL_GCM_FALSE,0,0,CELL_GCM_TEXTURE_MAX_ANISO_1);
			pSpuGcmState->CachedStates.SamplerState[index].control0 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL0(CELL_GCM_FALSE,0,0,CELL_GCM_TEXTURE_MAX_ANISO_1);
		}
		s_FatTexturesBound >>= 1;
		++index;
	}
# endif
#endif
}

}	// namespace rage
