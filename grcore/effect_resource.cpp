//
// grcore/effect_resource.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#include "effect.h"
#include "effectcache.h"
#include "effect_internal.h"

#if !__SPU
#include "texture.h"
#include "diag/art_channel.h"
#endif


namespace rage {

#if __SPU

	// NOTE: temporary debug data to gather info on deleted textures
	// before crashing on drawable spu
#if __BANK
	u32 ppuEffectAddr=0;
	u32 ppuCmdAddr=0;
#endif

#if USE_PACKED_GCMTEX
#	define GRCINSTANCEDATA_MARK_TEX_FLAG    0x80000000u
#else
#	define GRCINSTANCEDATA_MARK_TEX_FLAG    0x00000001u
#endif

// Fetching only marked textures when USE_PACKED_GCMTEX costs about 10,000 sample hits (5-6%) due to additional
// logic in spuGet_ and MarkUsedTextures.  The fetch itself is slightly longer when we fetch all of them, of course,
// but it's still faster to grab everything.  This opens us up to referencing potentially invalid textures,
// but I believe that's harmless when USE_PACKED_GCMTEX is enabled
#define FETCH_ONLY_MARKED		(!USE_PACKED_GCMTEX)

#if FETCH_ONLY_MARKED
void grcInstanceData::FetchMarkedTextures(datResource &rsc,unsigned maskMarked,unsigned maskUse)
{
	const unsigned textureCount = TextureCount;

	// Check for space could give false positives here
	Assertf(spuScratch + TextureCount*SPU_TEXTURE_SIZE < spuScratchTop,"%p + %d*32 >= %p",spuScratch,TextureCount,spuScratchTop);

	char *nextTexture = spuScratch;

	// Fetch all of the textures and do the fixups.
	// (Could just build a list here instead but we stall out anyway)
	for (unsigned i=0; i<textureCount; i++) {
#if USE_PACKED_GCMTEX
		const u32 textureIdx = Data()[i].TextureIndex;
		if (textureIdx & maskMarked) {
			sysDmaGet(nextTexture,(uint32_t)(pSpuGcmState->PackedTextureArray + (textureIdx&maskUse)),SPU_TEXTURE_SIZE,spuGetTag);
			Data()[i].Texture = (grcTextureObject*)nextTexture;
			nextTexture += SPU_TEXTURE_SIZE;
		}
#if TRAP_UNMARKED_TEXTURES
		else if (textureIdx) {
			// Trash the texture so that we'll catch any errors if we do try to render with it
			Data()[i].Texture = (grcTextureObject*)-1;
		}
#endif // TRAP_UNMARKED_TEXTURES
#else
		const spuTexture *textureEa = Data()[i].Texture;
		if ((u32)textureEa & maskMarked) {
			textureEa = (spuTexture*)((u32)textureEa & maskUse);
#if __BANK
			// NOTE: temporary debug data to gather info on deleted textures
			// before crashing on drawable spu
			if(!DbgGetTextureAddr(textureEa))
			{
				u16 techniqueId = (u16)(((spuCmd_grcEffect__Bind*)ppuCmdAddr)->technique);
				u16 passId = (u16)(((spuCmd_grcEffect__Bind*)ppuCmdAddr)->passIndex);
				grcDisplayf("Bad texture detected: effect_ptr=0x%X technique=%u pass=%u modelinfo=%u", ppuEffectAddr, techniqueId, passId, pSpuGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx);
			}
#endif //__BANK
			// Note, the full object is 48 bytes but we don't need anything beyond the first 32.
			sysDmaGet(nextTexture,GetTextureAddr(textureEa),SPU_TEXTURE_SIZE,spuGetTag);
			Data()[i].Texture = (spuTexture*)nextTexture;
			nextTexture += SPU_TEXTURE_SIZE;
		}
#if TRAP_UNMARKED_TEXTURES
		else if (textureEa) {
			Data()[i].Texture = (spuTexture*)-1;
		}
#endif // TRAP_UNMARKED_TEXTURES
#endif
	}

	const unsigned count = Count;
	for (unsigned i=textureCount; i<count; i++)
		rsc.PointerFixup(Data()[i].Any);

	// Wait for textures to arrive
	sysDmaWaitTagStatusAll(1<<spuGetTag);

	// Don't need NameHash on SPU

	spuScratch = nextTexture;

	if (spuScratch > spuScratchTop)
		Quitf("grcInstanceData rsc ctor - scratch overflow getting effect instance data");
}

void grcInstanceData::MarkUsedTextures(const grcProgram *program)
{
	const u32 *const cEnd = program->m_Constants.End();
	for (const u32 *c=program->m_Constants.Begin(); c<cEnd; ++c)
	{
		grcInstanceData::Entry *const e = Data()+*c;
		if (Likely(!e->Count))
		{
#			if USE_PACKED_GCMTEX
				u32 textureIdx = e->TextureIndex;
				textureIdx |= GRCINSTANCEDATA_MARK_TEX_FLAG & -!!textureIdx;
				e->TextureIndex = textureIdx;
#			else
				u32 textureEa = (u32)e->Texture;
				textureEa |= !!textureEa;
				e->Texture = (spuTexture*)textureEa;
#			endif
		}
	}
}

void grcInstanceData::MarkUsedTextures(const grcVertexProgram *vertProgs,const grcFragmentProgram *fragProgs,const grcEffect_Technique_Pass *pass)
{
	MarkUsedTextures(vertProgs+pass->VertexProgramIndex);
	MarkUsedTextures(fragProgs+pass->FragmentProgramIndex);
}

void grcInstanceData::MarkUsedTextures(const grcEffect *effect,unsigned technique)
{
	// Need to check validity of technique here, since we may be validly be passed grcetNONE
	const unsigned tIdx = technique - 1;
	if (Likely(tIdx < (unsigned)effect->m_Techniques.GetCount()))
	{
		const grcEffect::Technique *const t = &effect->m_Techniques[tIdx];
		const grcEffect::Technique::Pass *const pEnd = t->Passes.End();
		const grcVertexProgram   *const vertProgs = effect->m_VertexPrograms.Begin();
		const grcFragmentProgram *const fragProgs = effect->m_FragmentPrograms.Begin();
		for (const grcEffect::Technique::Pass *p=t->Passes.Begin(); p<pEnd; ++p)
		{
			MarkUsedTextures(vertProgs,fragProgs,p);
		}
	}
}

#else		// FETCH_ONLY_MARKED

void grcInstanceData::FetchTextures(datResource &rsc)
{
	const unsigned textureCount = TextureCount;

	// Check for space could give false positives here
	Assertf(spuScratch + TextureCount*SPU_TEXTURE_SIZE < spuScratchTop,"%p + %d*32 >= %p",spuScratch,TextureCount,spuScratchTop);

	char *nextTexture = spuScratch;

	// Fetch all of the textures and do the fixups.
	// (Could just build a list here instead but we stall out anyway)
	for (unsigned i=0; i<textureCount; i++) {
		const u32 textureIdx = Data()[i].TextureIndex;
		if (textureIdx) {
			sysDmaGet(nextTexture,(uint32_t)(pSpuGcmState->PackedTextureArray + textureIdx),SPU_TEXTURE_SIZE,spuGetTag);
			Data()[i].Texture = (grcTextureObject*)nextTexture;
			nextTexture += SPU_TEXTURE_SIZE;
		}
	}

	const unsigned count = Count;
	for (unsigned i=textureCount; i<count; i++)
		rsc.PointerFixup(Data()[i].Any);

	// Wait for textures to arrive
	sysDmaWaitTagStatusAll(1<<spuGetTag);

	// Don't need NameHash on SPU

	spuScratch = nextTexture;

	if (spuScratch > spuScratchTop)
		Quitf("grcInstanceData rsc ctor - scratch overflow getting effect instance data");
}

#endif		// FETCH_ONLY_MARKED

grcInstanceData::grcInstanceData(datResource &rsc)
{
	rsc.PointerFixup(Entries);
#if FETCH_ONLY_MARKED
	FetchMarkedTextures(rsc,~0,~0);
#else
	FetchTextures(rsc);
#endif
}

grcInstanceData::grcInstanceData(datResource &rsc,const grcEffect *effect,unsigned technique,unsigned passIndex)
{
	rsc.PointerFixup(Entries);
#if FETCH_ONLY_MARKED
	const grcEffect::Technique *const t = &effect->m_Techniques[technique-1];
	const grcEffect::Technique::Pass *const pass = &t->Passes[passIndex];
	const grcVertexProgram *const vertProgs = effect->m_VertexPrograms.Begin();
	const grcFragmentProgram *const fragProgs = effect->m_FragmentPrograms.Begin();
	MarkUsedTextures(vertProgs,fragProgs,pass);
	FetchMarkedTextures(rsc,GRCINSTANCEDATA_MARK_TEX_FLAG,~GRCINSTANCEDATA_MARK_TEX_FLAG);
#else
	FetchTextures(rsc);
#endif
}

grcInstanceData::grcInstanceData(datResource &rsc,unsigned techniqueGroupId,unsigned drawTypeMask)
{
	rsc.PointerFixup(Entries);
#if FETCH_ONLY_MARKED
	const grcEffect *const effect = s_effectCache->Get(Basis);
	unsigned drawType = 0;
	do
	{
		if (Likely(drawTypeMask&1))
		{
			const unsigned technique = effect->GetDrawTechnique(techniqueGroupId,drawType);
			MarkUsedTextures(effect,technique);
		}
		++drawType;
	}
	while (Unlikely((drawTypeMask>>=1)!=0));
	FetchMarkedTextures(rsc,GRCINSTANCEDATA_MARK_TEX_FLAG,~GRCINSTANCEDATA_MARK_TEX_FLAG);
#else
	FetchTextures(rsc);
#endif
}

void grcInstanceData::SpuGet()
{
	// This function is called when the grcInstanceData is resident but the Data payload is not.
	// Bail immediately if there is no payload.
	if (!SpuSize) {
		Entries = NULL;
		return;
	}

	// Pull over payload section.  Use a fenced get on spuBindTag to ensure that
	// any in flight puts from grcEffect__SetVar_grcTexture have completed.
	grcInstanceData *temp = (grcInstanceData*) spuScratch;
	sysDmaGetfAndWait(spuScratch,(u32)Entries,SpuSize,spuBindTag);
	spuScratch += SpuSize;

	// Chain to shared code
	datResource rsc(temp,Entries,SpuSize);
	rage_placement_new(this) grcInstanceData(rsc);
}

void grcInstanceData::SpuGet(const grcEffect *effect,unsigned technique,unsigned passIndex)
{
	// This function is called when the grcInstanceData is resident but the Data payload is not.
	// Bail immediately if there is no payload.
	if (!SpuSize) {
		Entries = NULL;
		return;
	}

	// Pull over payload section
	grcInstanceData *temp = (grcInstanceData*) spuScratch;
	sysDmaGetAndWait(spuScratch,(u32)Entries,SpuSize,spuGetTag);
	spuScratch += SpuSize;

	// Chain to shared code
	datResource rsc(temp,Entries,SpuSize);
	rage_placement_new(this) grcInstanceData(rsc,effect,technique,passIndex);
}

grcEffect* grcEffect::spuGet_(grcEffect *&ptr) {
	using namespace rage;
	if ((u32)ptr < 256*1024)
		return ptr;

#if DEBUG_GET
	spuGetName = "effect class data";
#endif

	ptr = (grcEffect*) spuGetData(ptr, sizeof(grcEffect));
#if DEBUG_GET
	spuGetName = "effect container data";
#endif
	void *container = spuGetData(ptr->m_Container.m_Base, ptr->m_Container.m_Size);

	datResource rsc(container,ptr->m_Container.m_Base,ptr->m_Container.m_Size);
	ptr->Place(ptr,rsc);

	return ptr;
}

#else		// !__SPU

extern bool s_EnableFallback;
extern grcEffect *s_FallbackEffect;

#define CHECK_FOR_SHADER_MATCHES		(1)

extern void FixupTexture(grcTexture *&ptr,datResource &rsc);

grcInstanceData::grcInstanceData(datResource &rsc)
{
#if __TOOL
	(void)rsc;
#else //__TOOL
	rsc.PointerFixup(Entries);
#if CHECK_FOR_SHADER_MATCHES || RAGE_SUPPORT_TESSELLATION_TECHNIQUES
	u32 *NameHash = (u32*)((char*)Entries + (SpuSize));
#endif
#if CHECK_FOR_SHADER_MATCHES
	bool matches = false;
#endif
	if (!datResource_IsDefragmentation) {
		// Reconnect to original effect (these live in a union together)
		if (!MaterialHashCode) {
			OUTPUT_ONLY(u32 basisHashCode = BasisHashCode);
			Basis = grcEffect::LookupEffect(BasisHashCode);
			if (!Basis) {
				grcErrorf("Attempting to load resource '%s', no material, and cannot find effect (hashcode 0x%x).", rsc.GetDebugName(), basisHashCode);
				Basis = s_EnableFallback ? s_FallbackEffect : NULL;
				Assert(s_FallbackEffect);
			}
		}
		else {
			OUTPUT_ONLY(u32 mtlHashCode = MaterialHashCode);
			Material = grcEffect::LookupMaterial(MaterialHashCode);
			if (!Material) {
				grcErrorf("Attempting to load resource '%s' referencing material doesn't exist (hashcode 0x%x).", rsc.GetDebugName(), mtlHashCode);
				OUTPUT_ONLY(u32 basisHashCode = BasisHashCode);
				Basis = grcEffect::LookupEffect(BasisHashCode);
				if (!Basis) {
					grcErrorf("...and can't find the original effect either, using fallback effect, this will look bad! (hashcode 0x%x).", basisHashCode);
					Basis = s_EnableFallback ? s_FallbackEffect : NULL;
				}
			}
			else {
				// Make sure the underlying basis didn't change?
				OUTPUT_ONLY(u32 basisHashCode = BasisHashCode);
				Assert(Material->Basis == grcEffect::LookupEffect(BasisHashCode));
				Basis = Material->Basis;
				if (!Basis) {
					grcErrorf("Attempting to load resource '%s' referencing an effect that doesn't exist (hashcode 0x%x).", rsc.GetDebugName(), basisHashCode );
					Basis = s_EnableFallback ? s_FallbackEffect : NULL;
				}

			}
		}

		Assertf(Basis, "grcInstanceData '%s' Basis is NULL", rsc.GetDebugName());

#if CHECK_FOR_SHADER_MATCHES
		if (Basis->GetVarCount() == Count && Basis->m_InstanceData.TextureCount == TextureCount) {
			matches = true;
			for (int i=0; i<Count && matches; i++) {
				bool texMatched = ((Basis->m_Locals[i].Type == grcEffect::VT_TEXTURE) == IsTexture(i));
				bool hashMatched = Basis->m_Locals[i].NameHash == NameHash[i];
				matches = texMatched && hashMatched;
			}
		}
#if !__GAMETOOL
		else
		{
			grcDebugf2( "Shader basis<->resource variable count mismatch" );			
		}
#endif // !__GAMETOOL
#endif
	}
#if CHECK_FOR_SHADER_MATCHES
	else
		matches = true;

	if (!matches)
	{
#if !(RSG_ORBIS || RSG_DURANGO) // I can't take it. Neither can I.
		CONSOLE_ONLY(grcWarningf("Resourced instance data in resource '%s' (effect '%s') doesn't match the shader any longer; you need new resources.", rsc.GetDebugName(), Basis->m_EffectName);)
#endif
	}


	// HACK - KS
	grcEffect::sm_MatchError |= !matches;

	// TODO: Establish whether to still do this work on !__DEV or __FINAL builds?
	// Should we spew errors, require a magic command line parameter, or what?
	if (!matches) {
		Assert(!datResource_IsDefragmentation);
		// Remember original raw data, then rebuild a clone in normal memory.
		// (rebuilding the clone won't free the original buffer)
		Entry *origData = Entries;
		int origCount = Count;
		u32 *origNames = NameHash;
		u32 origTotalSize = TotalSize;
		u8 origBucket = DrawBucket;
		u32 origBucketMask = DrawBucketMask;
		u8 origFlags = Flags;
		Basis->Clone(*this);

		NameHash = (u32*)((char*)Entries + SpuSize);

		// Copy the previous parameters over (use i for orig data, j for current data)
		// Note that changing the type of an existing variable from texture to non-texture or vice-versa will end badly.
		// If you change a float type, it copies based on the new size.
		for (int i=0; i<origCount; i++) {
			for (int j=0; j<Count; j++) {
				if (origNames[i] == NameHash[j]) {
					// grcDisplayf("Fixing up %s:%s",Basis->m_Locals[j].Name.c_str(),Basis->m_Locals[j].Semantic.c_str());
					if (Basis->m_Locals[j].Type == grcEffect::VT_TEXTURE) {
						Entries[j].Texture = origData[i].Texture;
#if ENABLE_DEFRAGMENTATION
						Entries[j].Texture.PointerFixup(rsc);
#else
						FixupTexture(Entries[j].Texture,rsc);
#endif
					}
					else {
						// If the data changed type we'll copy as much data as we can.
						rsc.PointerFixup(origData[i].Float);
						memcpy(Entries[j].Float,origData[i].Float,(origData[i].Count<Entries[j].Count?origData[i].Count:Entries[j].Count)<<4);
					}

					// Make sure we don't try to copy this parameter again if there's a duplicate
					// NO LONGER -- it causes defragmentation to get confused and try to repatch
					// and totally gums up the works.
					// NameHash[j] = 0;
					break;
				}
			}
		}

		DrawBucketMask = origBucketMask;
		DrawBucket = origBucket;

		// No reason to delete the original data, it's already in a resource.
		// Besides, we can't delete it because we use the original texture data if necessary.
		// delete[] (char*) origData;

		// New pass -- if the data will fit back were it was, copy it back again
		artAssertf(!__PS3 || origTotalSize >= SpuSize,"Resourced instance data in '%s' too big (by %d bytes), will use slower rendering paths.  PLEASE RE-CONVERT THIS MODEL WITH RAGEBUILDER.",rsc.GetDebugName(),SpuSize - origTotalSize);
		if ((origTotalSize >= SpuSize) && (origFlags & FLAG_EXTRA_DATA)) {
			TotalSize = SpuSize;
			memcpy(origData, Entries, SpuSize);
			ptrdiff_t fixup = (ptrdiff_t)origData - (ptrdiff_t)Entries;
			delete[] Entries;
			Entries = origData;

			for (int i=TextureCount; i<Count; i++)
				Entries[i].Any = (void*)((ptrdiff_t)Entries[i].Any + fixup);
		}
	}
	else 
#endif	// CHECK_FOR_SHADER_MATCHES
		if (datResource_IsDefragmentation)
	{
		// Textures are already indirect, no fixup necessary.
		for (int i=0; i<Count; i++) {
			if (!IsTexture(i))
				rsc.PointerFixup(Entries[i].Any);
		}
	}
	else
	{
		for (int i=0; i<Count; i++) {
			if (IsTexture(i))
#if ENABLE_DEFRAGMENTATION
				Entries[i].Texture.PointerFixup(rsc);
#else
				FixupTexture(Entries[i].Texture,rsc);
#endif
			else
				rsc.PointerFixup(Entries[i].Any);
		}
	}

	// Make sure register and sampler state set are correctly reinitialized from actual parameters
	for (int i=0; i<Count; i++) {
		Entries[i].Register = Basis->m_Locals[i].Register;
		Entries[i].SamplerStateSet = Basis->m_Locals[i].SamplerStateSet;
		ASSERT_ONLY(Entries[i].SavedSamplerStateSet = INVALID_STATEBLOCK);
#if EFFECT_CHECK_ENTRY_TYPES
		Entries[i].Type = IsTexture(i) ? ET_TEXTURE : ET_FLOAT;
#endif //EFFECT_CHECK_ENTRY_TYPES
	}

#if RAGE_SUPPORT_TESSELLATION_TECHNIQUES
	if (!datResource_IsDefragmentation)
	{
		bool bSetTessellationBit = false;

		grcEffectVar useTessellationVar = Basis->LookupVarByHash(ATSTRINGHASH("UseTessellation", 0x4620a35d));

		if(useTessellationVar != grcevNONE)
		{
			float useTessellationValue = 0.0f;
			Basis->GetVar(useTessellationVar, useTessellationValue);

			if(useTessellationValue == 1.0f)
			{
				bSetTessellationBit = true;
			}
		}

		if(bSetTessellationBit)
		{
			DrawBucketMask = BUCKETMASK_SET_TESSELLATION_BIT(DrawBucketMask);
		}
		else
		{
			DrawBucketMask = BUCKETMASK_CLEAR_TESSELLATION_BIT(DrawBucketMask);
		}
	}
#endif // RAGE_SUPPORT_TESSELLATION_TECHNIQUES

#if MULTIPLE_RENDER_THREADS
	// We still need this because the data might not be current any longer because of shader parameter changes.
	// We will skip the memory allocation though in this case.
	ExpandForMultipleThreads(false);
#endif

	Basis->UpdateTextureReferences(*this);

#endif // __TOOL
}


void grcEffect::UpdateTextureReferences(grcInstanceData &instanceData)
{
	for (int i=0; i<m_Locals.GetCount(); i++) {
		grcEffectVar v = GetVarByIndex(i);
		if (HasAnnotation(v,ATSTRINGHASH("ResourceName",0x4f5ce147))) {
			const char *resourceName = GetAnnotationData(v,ATSTRINGHASH("ResourceName",0x4f5ce147),"__unknown");
			grcTexture *tex = grcTextureFactory::GetInstance().ConnectToTextureReference(resourceName);
			if (tex)
				SetVar(instanceData,v,tex);
		}
	}
}


#if !__SPU && __DECLARESTRUCT
void grcInstanceData::DeclareStruct(datTypeStruct &s)
{
	// Swap the "owned" float data
	size_t offset = Count * sizeof(Entry);
	offset = (offset + 15) & ~15;

	// The NameHash is included in here.
	u32 *start = (u32*)((char*)Entries + offset);
	u32 *stop = (u32*)((char*)Entries + (TotalSize));
	while (start < stop)
		datSwapper(*start++);

	// Swap the pointer table at the start of the instance data.
	for (int i=0; i<Count; i++) {
		// Swap the contents if it's a texture
		if (IsTexture(i)) {
			if (Entries[i].Texture && Entries[i].Texture->GetResourceType() == grcTexture::REFERENCE) {
				datTypeStruct s;
				Entries[i].Texture->DeclareStruct(s);
			}
#if USE_PACKED_GCMTEX
			if (Entries[i].Texture.index)
				Entries[i].Texture.index = (u32)*(grcTexture**)Entries[i].Texture.index;
#elif __TOOL || !ENABLE_DEFRAGMENTATION
			if (Entries[i].Texture)
				Entries[i].Texture = Entries[i].Texture;
#else
			if (Entries[i].Texture.ptr)
				Entries[i].Texture.ptr = (grcTexture**)*Entries[i].Texture.ptr;
#endif
		}
		// Swap the pointer
		datSwapper(Entries[i].Any);
	}
	STRUCT_BEGIN(grcInstanceData);
	STRUCT_FIELD_VP(Entries);
	BasisHashCode = Basis->GetHashCode();
	STRUCT_FIELD(BasisHashCode);
	STRUCT_FIELD(Count);
	STRUCT_FIELD(DrawBucket);
	STRUCT_FIELD(PhysMtl_DEPRECATED);
	STRUCT_FIELD(Flags);

	STRUCT_FIELD(SpuSize);
	STRUCT_FIELD(TotalSize);

	MaterialHashCode = Material? atStringHash(Material->MaterialName) : 0;
	STRUCT_FIELD(MaterialHashCode);
	STRUCT_FIELD(DrawBucketMask);
	STRUCT_FIELD(IsInstanced);
	STRUCT_IGNORE(UserFlags);
	STRUCT_FIELD(pad);
	STRUCT_FIELD(TextureCount);
	STRUCT_FIELD(SortKey_DEPRECATED);

	STRUCT_END();
}
#endif



#endif		// ___SPU



}		// namespace rage
