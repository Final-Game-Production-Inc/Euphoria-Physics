// 
// grcore/effect_gcm.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "effect.h"
#include "effect_internal.h"
#include "stateblock_internal.h"

#include "system/cache.h"
#include "system/typeinfo.h"
#include "system/param.h"
#include "system/nelem.h"
#include "system/spuintrinsics.h"
#include "system/replay.h"

#include "system/criticalsection_spu.h"
#include "paging/knownrefpool.h"

#include <string.h>		// for memcpy

#if __PS3

#include "effect_values.h"
#include "wrapper_gcm.h"
#include <sdk_version.h>

#if !__SPU
#include "texture.h"
#include "wrapper_gcm.h"
#if GCM_REPLAY
# if CELL_SDK_VERSION <= 0x300001
# include <GcmReplay.h>
# endif
#endif


#if __BANK
#include "bank/bank.h"
#endif

#include "wrapper_gcm.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "grcorespu.h"
#include "system/task.h"
#endif

#include "edge_jobs.h"
#include "effect_config.h"
#include "edgegeomspu.h"


namespace rage {

#if __PPU
	u128 grcFragmentProgram::sm_Constants[256];
#else
	u128 *grcFragmentProgram::sm_Constants;
#endif

	extern CellSpurs *g_Spurs;

#if !__SPU
	u16 g_VertexShaderInputs;
#endif

	using namespace grcRSV;
	using namespace grcSSV;


#if __PPU
	grcGeometryJobs g_GeometryJobs;
#endif

// from device_gcm.cpp
#if __SPU
BANK_ONLY( u8 g_MinRegisterCount=2 );  // Needed for shmoo
const u32 g_MaxAlphaRef=255;
#else
BANK_ONLY( extern u8 g_MinRegisterCount );
extern u32 g_MaxAlphaRef;
extern CellGcmSurface g_GcmSurface;
extern CellGcmEnum g_DepthFormatType;
#endif // __SPU

#if __DEV && !__SPU
bool g_EnableDepthReplace = true;
bool g_EnablePixelKill = true;
bool g_EnableTexpToTexDemotion = true;
#endif // __DEV && !__SPU

u16 g_RenderStateBlockMask = 0;

#if __SPU

#define s_FragmentOffset	(pSpuGcmState->CurrentFragmentOffset)
#define s_VertexProgram		(pSpuGcmState->CurrentVertexProgram)
#define s_BranchBits		(pSpuGcmState->BranchBits)
static void *s_LastCachedProgram;
static uint32_t s_LastCachedFragmentOffset;


void grcEffect::ClearCachedState()
{
	s_VertexProgram = NULL;
	sm_CurrentBind = NULL;
	s_FragmentOffset = ~0U;
	s_LastCachedProgram = NULL;
	s_LastCachedFragmentOffset = ~0U;	// for clarity, not really necessary because s_LastCachedProgram is cleared first.
}

__forceinline qword *ReserveMethodSizeAlignedN(CellGcmContextData *ctxt,uint32_t byteCount)
{
	// Extra 16 bytes because we may lose it due to alignment
	if (Unlikely((char*)ctxt->current + byteCount + 16 > (char*)ctxt->end))
		gcmCallback(ctxt,0);

	qword writePtr = si_from_ptr(ctxt->current);

	// Load the first quadword we're pointing at.
	// Do a masked shift right of a one's mask based on the 4 LSB's
	// and do a complemented and to clear the necessary bits (turning them into GCM nop's)
	qword prevQuad = si_lqd(writePtr,0);
	qword mask = si_rotqmby(si_il(-1),si_sfi(si_andi(writePtr,15),0));
	qword newQuad = si_andc(prevQuad,mask);
	si_stqd(newQuad,writePtr,0);

	qword alignedWritePtr = (si_andi(si_ai(writePtr,16),~15));
	ctxt->current = (uint32_t*) si_to_ptr(si_a(alignedWritePtr,si_from_int(byteCount)));

	return (qword*) si_to_ptr(alignedWritePtr);
}

void grcVertexProgram::Bind() const
{
	// Always update the shader input mask, it's cheap and avoids problems when
	// two subsequent jobs use the same s_VertexProgram (which is cached).
	g_VertexShaderInputs = Configuration.attributeInputMask;

	// Always use a call here -- otherwise we have to transfer the entire vertex ucode
	// and default constants into local memory, which gets rather expensive.
	// NOTE -- vertex program cannot live in VRAM so its offset will never be zero
	if (s_VertexProgram != (void*)ProgramOffset) {
#if INLINE_SMALL_VERTEX_PROGRAM_LIMIT
		if (!ProgramSize)
#endif
			GCM_DEBUG(GCM::cellGcmSetCallCommand(GCM_CONTEXT,ProgramOffset));
#if INLINE_SMALL_VERTEX_PROGRAM_LIMIT
		else {
			// Displayf("grabbing vp %x size %u",ProgramOffset,ProgramSize);
			qword *dest = ReserveMethodSizeAlignedN(GCM_CONTEXT,ProgramSize);
			sysDmaGetAndWait(dest,(uint64_t)ProgramOffset,ProgramSize,FIFOTAG);
		}
#endif
		s_VertexProgram = (void*)ProgramOffset;
	}
	g_VertexShaderInputs = Configuration.attributeInputMask;
}

void grcFragmentProgram::Bind() const
{
	if (Configuration.offset)
		Bind(Configuration.offset,true);
	else if (Program != s_LastCachedProgram) {
		Bind(s_LastCachedFragmentOffset = Cache(),false);
		s_LastCachedProgram = Program;
	}
	else	// In case we switched between a cached and non-cached program.
		Bind(s_LastCachedFragmentOffset,false);
}

int g_LowestConstant, g_HighestConstant;

void grcFragmentProgram::SetParameter(int address,const float *data,int count)
{
	Assert(address + count <= 256);
	Assert(!((u32)data & 15));		// TODO: could have a slow path
	u128 *src = (u128*) data;
	u128 *dest = &sm_Constants[address];
	int lowest = address, highest = address + count;
	unsigned isDifferent = 0;
	do {
		u128 srcData = *src++;
		// Do the swizzling now to save time later.
		u128 destData = spu_shuffle(srcData,srcData,(vec_uchar16)(2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13));
		// TODO: could optimize this to remember the comparison result and clear s_LastCachedProgram only once without any branches.
		// Actually, it turns out the compiler already does a great job doing a conditional store into s_LastCachedProgram anyway.
		isDifferent |= vec_any_ne(*dest,destData);
		*dest++ = destData;
	} while (--count);
	if (isDifferent) {
		s_LastCachedProgram = NULL;
		if (lowest < g_LowestConstant)
			g_LowestConstant = lowest;
		if (highest > g_HighestConstant)
			g_HighestConstant = highest;
	}
}


void grcFragmentProgram::Bind(u32 offset,bool isStatic) const
{
	CellCgbFragmentProgramConfiguration config = Configuration;
	u32 newInputMask = Configuration.attributeInputMask | ((s_BranchBits >> 20) & 0xfc0);
	bool dirty = Configuration.attributeInputMask != newInputMask;
	config.attributeInputMask = newInputMask;

#if __BANK
	// Shmoo the register count
	if (g_MinRegisterCount > config.registerCount)
	{
		config.registerCount = g_MinRegisterCount;
		dirty = true;
	}
#endif

	if (s_FragmentOffset != offset || dirty) {
		config.offset = s_FragmentOffset = offset;
		uint32_t location = CELL_GCM_LOCATION_LOCAL;
		// Invalidate uses 2 words, PLL uses 6 + 2*texMask words
		if (GCM_CONTEXT->current + 2 + 6 + 2*16 > GCM_CONTEXT->end)
			gcmCallback(GCM_CONTEXT,0);
		if (!isStatic) {
			GCM_DEBUG(GCMU::cellGcmSetInvalidateTextureCache(GCM_CONTEXT, CELL_GCM_INVALIDATE_TEXTURE));
			location = pSpuGcmState->FragmentCacheLocation;
		}
		GCM_DEBUG(GCMU::cellGcmSetFragmentProgramLoadLocation(GCM_CONTEXT,&config,location));
		PF_INCREMENT(cellGcmSetFragmentProgram_Calls);
	}
}


void grcVertexProgram::SetFlag(int address,bool value)
{
	u32 mask = 1 << address;
	if (value)
		s_BranchBits |= mask;
	else
		s_BranchBits &= ~mask;
	GCM_STATE(SetTransformBranchBits,s_BranchBits);
}


const qword mergeWords = MAKE_SHUFFLE(A0,A1,A2,A3,B0,B1,B2,B3,A0,A1,A2,A3,B0,B1,B2,B3);
const qword mergeDoubleWords = MAKE_SHUFFLE(A0,A1,A2,A3,A4,A5,A6,A7,B0,B1,B2,B3,B4,B5,B6,B7);
const qword mergeLowerZero = MAKE_SHUFFLE(A0,A1,A2,A3,B0,B1,B2,B3,00,00,00,00,00,00,00,00);

inline qword MAKE_QWORD(uint32_t a,uint32_t b,uint32_t c,uint32_t d)
{
	qword lo = si_shufb(si_from_uint(a),si_from_uint(b),mergeWords);
	qword hi = si_shufb(si_from_uint(c),si_from_uint(d),mergeWords);
	return si_shufb(lo,hi,mergeDoubleWords);
}

inline qword MAKE_QWORD_ZZ(uint32_t a,uint32_t b)
{
	// return MAKE_QWORD(a,b,0,0);
	return si_shufb(si_from_uint(a),si_from_uint(b),mergeLowerZero);
}



void grcVertexProgram::SetTexture(int texUnit,const grcTextureObject *gcmTex,u16 samplerStateSet)
{
	const grcSamplerState &ss = g_SamplerStates[samplerStateSet];
	Assertf(texUnit < CELL_GCM_MAX_VERTEX_TEXTURE,"Invalid vertex texture sampler index %d",texUnit);
	texUnit <<= 5;
	qword *q = ReserveMethodSizeAligned(GCM_CONTEXT,gcmTex? 4 : 1);

	if (!gcmTex) {
		q[0] = MAKE_QWORD_ZZ(CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (texUnit), 1), 0);
		return;
	}
#if !USE_PACKED_GCMTEX
	ASSERT_ONLY(const u8 f128Format = CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR);
	ASSERT_ONLY(const u8 f32Format = CELL_GCM_TEXTURE_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR);
	AssertMsg(gcmTex->format == f128Format || gcmTex->format == f32Format,"PS3 Vertex texture must be float1 or float4 linear with normalized tex coords.");
	AssertMsg((gcmTex->offset & 127) == 0,"Vertex texture must be 128b aligned");
	AssertMsg(gcmTex->format != f128Format || (gcmTex->pitch & 15) == 0,"Vertex texture pitch must be 16b aligned for CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT");
	AssertMsg(gcmTex->format != f32Format || (gcmTex->pitch & 3) == 0,"Vertex texture pitch must be 4b aligned for CELL_GCM_TEXTURE_X32_FLOAT");
	AssertMsg(gcmTex->depth == 1,"3D vertex textures not supported by PS3");
	AssertMsg(gcmTex->cubemap == 0,"Cubemap vertex textures not supported by PS3");
	AssertMsg(gcmTex->mipmap > 0 && gcmTex->mipmap <= 13, "Invalid number of mip levels specified for vertex texture");

	uint32_t format = (gcmTex->location + 1) | (gcmTex->dimension << 4) 
		| (gcmTex->format << 8) | (gcmTex->mipmap << 16);
	uint32_t imagerect = gcmTex->height | (gcmTex->width << 16);
	uint32_t control3 = gcmTex->pitch;
#else
	uint32_t format = gcmTex->format & 0x000FFFF3;	// border and cubemap are not supported for vertex textures (and border is hard-coded to 1)
	uint32_t imagerect = gcmTex->imagerect;
	uint32_t control3 = gcmTex->pitch & 0xFFFE;
#endif
	uint32_t offset = gcmTex->offset;

	q[0] = MAKE_QWORD(CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET + (texUnit), 2), offset, format,
		CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (texUnit), 1));
	q[1] = MAKE_QWORD(control3, CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (texUnit), 1), imagerect,
		CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (texUnit), 1));
	q[2] = MAKE_QWORD((CELL_GCM_TRUE << 31),
		CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (texUnit), 1),
		si_to_uint(si_fsmb(si_from_uint(ss.border))),
		CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS + (texUnit), 1));
	q[3] = MAKE_QWORD_ZZ(ss.address & 0xF0F,CELL_GCM_METHOD(CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER + (texUnit), 1));
}

void grcFragmentProgram::SetTexture(int texUnit,const grcTextureObject *gcmTex,u16 samplerStateSet)
{
	const grcSamplerState &ss = g_SamplerStates[samplerStateSet];

	ValidateSpuPtr(gcmTex);
	Assertf(texUnit < 16,"Invalid fragment texture sampler index %d",texUnit);

	const u32 maxNumWords = 9 + 2 + 2;
	u32 *cmd = EnsureMethodSpaceWords(GCM_CONTEXT, maxNumWords);

	spuGcmSamplerState *curr = pSpuGcmState->CachedStates.SamplerState + texUnit;
	spuGcmSamplerState wanted;

	if (Unlikely(!gcmTex)) {
		// When there is no texture specified, disable the sampler, and make
		// sure the crossbar remap is set to something sensible.  A disabled
		// texture sampler always fetches (0,0,0,0), but this is before the
		// crossbar remapping, which can then force components to 1.  Don't just
		// force a remap of (0,0,0,0), since we do want to disable the texture
		// fetch (for performance reasons) anyways.
		const u32 wantedControl0 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL0(CELL_GCM_FALSE, 0, 0, CELL_GCM_TEXTURE_MAX_ANISO_1);
		const u32 wantedControl1 = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL1(CELL_GCM_TEXTURE_REMAP_ORDER_XYXY,
			CELL_GCM_TEXTURE_REMAP_REMAP,  CELL_GCM_TEXTURE_REMAP_REMAP,  CELL_GCM_TEXTURE_REMAP_REMAP,  CELL_GCM_TEXTURE_REMAP_REMAP,
			CELL_GCM_TEXTURE_REMAP_FROM_B, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_A);
		wanted.control0 = wantedControl0;
		wanted.control1 = wantedControl1;
		cmd = GenerateConsecutiveRegistersCmdBuf(cmd, &curr->control0, &wanted.control0, CELL_GCM_NV4097_SET_TEXTURE_CONTROL0+texUnit*0x20, 2
			DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(, pSpuGcmState));
		GCM_CONTEXT->current = cmd;
		curr->control0 = wantedControl0;
		curr->control1 = wantedControl1;
		return;
	}

#if HACK_GTA4_MODELINFOIDX_ON_SPU & 0
	if( pSpuGcmState->gSpuGta4DebugInfo.gta4MaxTextureSize > 0 )
	{
		int maxSize = 1 << (pSpuGcmState->gSpuGta4DebugInfo.gta4MaxTextureSize-1);
		int texMaxSize = rage::Max(gcmTex->width,gcmTex->height);
		if( texMaxSize > maxSize )
		{
			int minlod = 0;
			int size = texMaxSize;
			while( size > maxSize )
			{
				minlod++;
				size >>= 1;
			}

			ss.minlod = minlod;
		}
	}
#endif // HACK_GTA4_MODELINFOIDX_ON_SPU

#if !USE_PACKED_GCMTEX
	wanted.offset        = gcmTex->offset;
	wanted.format        = CELL_GCM_METHOD_DATA_TEXTURE_BORDER_FORMAT(gcmTex->location, gcmTex->cubemap, gcmTex->dimension, gcmTex->format, gcmTex->mipmap, 1/*border*/);
	wanted.address       = ss.address | ((gcmTex->_padding & 0x0f) << 20); // 0x0f == CELL_GCM_TEXTURE_GAMMA_MASK
	wanted.control0      = ss.control0;
	wanted.control1      = gcmTex->remap;
	wanted.filter        = ss.filter  | ((gcmTex->_padding & 0xf0) << 24); // 0xf0 == component signed bits
	wanted.imageRect     = CELL_GCM_METHOD_DATA_TEXTURE_IMAGE_RECT(gcmTex->height, gcmTex->width);
	wanted.borderColor   = si_to_uint(si_fsmb(si_from_uint(ss.border)));
	wanted.control2      = ss.control2;
	wanted.control3      = CELL_GCM_METHOD_DATA_TEXTURE_CONTROL3(gcmTex->pitch, gcmTex->depth);
#else
	wanted.offset        = gcmTex->offset;
	wanted.format        = gcmTex->format & 0x000FFFFF;
	wanted.address       = ss.address | ((gcmTex->format & 0x00F00000)); // 0x0f == CELL_GCM_TEXTURE_GAMMA_MASK
	wanted.control0      = ss.control0;
	wanted.control1      = gcmTex->remap | ((u32)(gcmTex->pitch & 1) << 16);
	wanted.filter        = ss.filter  | (gcmTex->format & 0xF0000000); // 0xf0 == component signed bits
	wanted.imageRect     = gcmTex->imagerect;
	wanted.borderColor   = si_to_uint(si_fsmb(si_from_uint(ss.border)));
	wanted.control2      = ss.control2;
	wanted.control3      = (gcmTex->pitch & 0xFFFE) | (1 << (20 + ((gcmTex->format >> 24) & 0xF)));
#endif

	DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(const bool forceDiff = pSpuGcmState->DebugSetAllStates;)

	// GCM method macros restriction, if format, imageRect or control3 change,
	// then offset, control1, imageRect and control3 must also be rewriten.
	// Trash the current values to make sure that happens.
	if (curr->format != wanted.format | curr->imageRect != wanted.imageRect | curr->control3 != wanted.control3)
	{
		curr->offset    = ~0;
		curr->control1  = ~0;
		curr->imageRect = ~0;
		curr->control3  = ~0;
	}

#if __BANK //&& 0
{
	// Validate the texture registers to try and track down the RSX crash
	// B*982467.  Once that is fixed this code can be disabled, but probably
	// still worth keeping it around for the future, so simply comment in the &&
	// 0 above.
	//
	// To keep code size down, and to reduce performance impact, we are simply
	// checking that reserved bits in the registers really are zero.  This
	// should be good enough to catch the problem before passing it on to the
	// RSX, but more stringent tests could be added if required.
	//
	static const qword s_mask0 = (qword)(vec_uint4){
		0xe0000000,     // CELL_GCM_NV4097_SET_TEXTURE_OFFSET
		0xfff000c0,     // CELL_GCM_NV4097_SET_TEXTURE_FORMAT
		0x0f000000,     // CELL_GCM_NV4097_SET_TEXTURE_ADDRESS
		0x0000000b      // CELL_GCM_NV4097_SET_TEXTURE_CONTROL0
	};
	static const qword s_mask1 = (qword)(vec_uint4){
		0xfff00000,     // CELL_GCM_NV4097_SET_TEXTURE_CONTROL1
		0xf8f88000,     // CELL_GCM_NV4097_SET_TEXTURE_FILTER
		0xe000e000,     // CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT
		0x00000000,     // CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR
	};
	qword test0 = si_and(s_mask0, si_lqd(si_from_ptr(&wanted), 0));
	qword test1 = si_and(s_mask1, si_lqd(si_from_ptr(&wanted), 16));
	qword test = si_gb(si_clgti(si_or(test0, test1), 0));
	si_hgti(test, 0);
}
#endif

	cmd = GenerateConsecutiveRegistersCmdBuf(cmd, &curr->offset, &wanted.offset, CELL_GCM_NV4097_SET_TEXTURE_OFFSET+texUnit*0x20, 8
		DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(, pSpuGcmState));

	if (curr->control2 != wanted.control2 DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		cmd[0] = CELL_GCM_METHOD_HEADER_TEXTURE_CONTROL2(texUnit, 1);
		cmd[1] = wanted.control2;
		cmd += 2;
	}
	if (curr->control3 != wanted.control3 DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		cmd[0] = CELL_GCM_METHOD_HEADER_TEXTURE_CONTROL3(texUnit, 1);
		cmd[1] = wanted.control3;
		cmd += 2;
	}

	GCM_CONTEXT->current = cmd;

	memcpy(curr, &wanted, sizeof(*curr));
}


u128* g_UcodePpuAddr;
u128 *g_CachedUcodeDataBuffer;
u32 g_CachedUcodeDataSize;
u32 g_CachedUcodeProgramSize;	// Amount of the cache that is actually used

void grcFragmentProgram::InitCache(u128 *cachePtr,unsigned cacheSize)
{
	g_UcodePpuAddr = NULL;
	g_CachedUcodeDataBuffer = cachePtr;
	g_CachedUcodeDataSize = cacheSize;
}

static size_t DecompressMicrocode128(qword *dest,const u32 *src,size_t sizeInWords)
{
	static const vec_uint4 table[4] = {
		{ 0, 0, 0, 0 },
		{ 0, 0x3C0000, 0, 0 },
		{ 0, 0, 0x3C0000, 0 },
		{ 0, 0, 0, 0x3C0000 },
	};
	const u32 *stop = src + sizeInWords;
	qword *origDest = dest;
	while (Likely(src < stop)) {
		u32 dest0, dest1, dest2, dest3;
		dest0 = *src++;
		u32 control = *src++;
		dest1 = control & ~0xC000;
		// Compiler can optimize this branch out.
		if (control & 0x8000)
			dest2 = 0xC8000001;
		else
			dest2 = *src++;
		// Compiler can optimize this branch out.
		if (control & 0x4000)
			dest3 = 0xC8000001;
		else
			dest3 = *src++;
		qword dest0123 = MAKE_QWORD(dest0,dest1,dest2,dest3);
		bool hasConstant = !(dest2 & 0x8000);	// Flow control cannot have constants
		// Original code:
		// if ((dest1 & 0x30000) == 0x20000) {	// Constant.
		//		control = dest1;
		//		dest1 &= ~0x3C0000;
		// for each of dest1, dest2, and dest3.

		// Test each of dest1 through dest3 in parallel (masking off any compare of dest0)
		// Note that none of them might match, so we have to handle that case.
		// Compute a mask of which words pass the 0x30000 == 0x20000 test, ignoring dest0 by forcing its compare to fail
		const vec_uint4 mask_123 = { 0, 0x30000, 0x30000, 0x30000 };
		qword test = si_ceq(si_and(dest0123, (qword)mask_123), si_ilhu(0x2));
		// Force mask test to fail if it's flow control
		qword mask = hasConstant? si_gbb(test) : si_il(0);
		hasConstant = (si_to_uint(mask)) != 0;
		// If mask is nonzero, then we have a second operand.
		// Figure out which slot (1, 2, or 3) we had the first match in.
		// this will be 20, 24, or 28, and the rotate only pays attention to the 4 LSB's.
		// it can also be 32 if the test failed (ie there was no second constant)
		qword byteslot = si_clz(mask);
		// We subtract two from this result so that the control checks can be against smaller integer literals.
		// control = dest[slot]
		control = si_to_uint(si_rotqby(dest0123,si_ai(byteslot,-2)));
		// ...and mask out the control bits (0x3C0000) from dest[slot] unless there was no second constant
		qword clearmask = (qword)table[(si_to_uint(byteslot) & 15) >> 2];
		// Compiler is smart enough not to generate branches for these:
		const u32 *src2 = src;
		u32 dest4 = (control & 0x20)? *src2++ : 0;
		u32 dest5 = (control & 0x10)? *src2++ : 0;
		u32 dest6 = (control & 0x08)? *src2++ : 0;
		u32 dest7 = (control & 0x04)? *src2++ : 0;
		*dest++ = si_andc(dest0123, clearmask);
		*dest = MAKE_QWORD(dest4,dest5,dest6,dest7);
		src = hasConstant? src2 : src;
		dest += hasConstant;
	}
	Assert(src == stop);
	return (char*)dest - (char*)origDest;
}

// Strip branches from fully patched fragment program microcode. It is safe to call this on fragment programs even if they don't have branches. 
// There must be enough memory backed by pOriginalUCode (nTotalBufferSize in bytes) to hold both the original code and the stripped code.
void grcFragmentProgram::StripBranches(u128 const *pOriginalUCode, u32 nOriginalUCodeSize, u32 nTotalBufferSize, u128** ppUCodeOut, u32* pUCodeSizeOut)
{
	#define SAFE_BRANCH_STRIPPING (1 && !__FINAL) // We can eliminate several branches here if we know that stripping will succeed

	// Pack original + stripped ucode into the ucode data buffer.
	u32 nAlignedProgramSize = ((nOriginalUCodeSize + 15) & ~15);
	*ppUCodeOut = (u128*)(((char*)pOriginalUCode + nTotalBufferSize) - nAlignedProgramSize);

	#if SAFE_BRANCH_STRIPPING
		// Make sure everything will fit, must have enough room for worst case scenario (when no ucode elimination happens). The cache is
		// certainly not guaranteed to be big enough and we are depending our biggest shaders not using branch stripping for this to work.
		u32 nCombinedProgramsSize = nOriginalUCodeSize + nAlignedProgramSize;
		if ( Likely(nCombinedProgramsSize <= nTotalBufferSize) )
		{
			if ( Likely( CELL_OK == cellGcmCgStripBranchesFromFragmentUCode(pOriginalUCode, nOriginalUCodeSize, *ppUCodeOut, pUCodeSizeOut) ) )
				return; // Stripping succeeded
			else
				Assertf(false, "Fragment branch stripping failed, reverting to original code."); 
		}
		else
		{
			Assertf(false, "grcFragment::Cache too small for fragment branch stripping: Size=%u, Required=%u.", nTotalBufferSize, nCombinedProgramsSize);
		}
		
		// If stripping fails, the ucode stored at pStrippedUCode is not guaranteed to be valid. Fall back to the original ucode.
		*ppUCodeOut = (u128*)pOriginalUCode;
		*pUCodeSizeOut = nOriginalUCodeSize;
	#else // SAFE_BRANCH_STRIPPING
		cellGcmCgStripBranchesFromFragmentUCode(pOriginalUCode, nOriginalUCodeSize, *ppUCodeOut, pUCodeSizeOut);
	#endif // SAFE_BRANCH_STRIPPING
}

u32 grcFragmentProgram::Cache() const
{
	ValidateSpuPtr(this);
	ValidateSpuPtr(CompressedPatchTable);
//	g_PatcherModifiedData = false;

	u128 *ucode = g_CachedUcodeDataBuffer;
	qword isSame = si_il(-1);
	// bool needWait = false;
	if (Program == g_UcodePpuAddr)
	{
		// If we're about reuse the cache, wait in case we hadn't finished the transfer
		// of the PREVIOUSLY cached data outbound just yet.
		sysDmaWaitTagStatusAll(1<<FIFOTAG);
	}
	else
	{
		isSame = si_il(0);		// make sure we allocate a new copy if the cache was invalidated
		if (ProgramSize > g_CachedUcodeDataSize)
			Quitf("Fragment program is too large, call grcFragment::InitCache in grcorespu_header.h with at least %u",ProgramSize);

		// If we're about to reload the cache, wait in case we hadn't finished the transfer
		// of the PREVIOUSLY cached data outbound just yet.
		sysDmaWaitTagStatusAll(1<<FIFOTAG);
		g_UcodePpuAddr = Program;
		if (ProgramSize & 1) {	// Is it compressed?
			// DMA the compressed ucode into the top of the static buffer
			char *temp = ((char*)g_CachedUcodeDataBuffer + g_CachedUcodeDataSize) - ((ProgramSize + 14) & ~15);
			sysDmaLargeGetAndWait(temp,(uint64_t)Program,(ProgramSize + 14) & ~15,FIFOTAG);
			g_CachedUcodeProgramSize = DecompressMicrocode128((qword*)ucode, (u32*)temp, ProgramSize >> 2);
		}
		else {
			sysDmaLargeGetAndWait(ucode,(uint64_t)Program,ProgramSize,FIFOTAG);	
			g_CachedUcodeProgramSize = ProgramSize;
		}
	}

	u8 *p = CompressedPatchTable;
	if (p) {
		do {
			const u128 srcReg = sm_Constants[*p++];
			int subCount = *p & 0x1F;
			int offset = ((*p >> 5) << 8) | p[1];
			p += 2;
			// Catch the case where we didn't really change anything after all	  
			// (for example, a pixel constant was changed but it wasn't one used by the current program)
			isSame = si_and((qword)isSame, si_ceq((qword)ucode[offset],(qword)srcReg));
			ucode[offset] = srcReg;
			while (subCount--) {
				u16 skip = *p++;
				if (Unlikely(skip >= 0xF0))
					skip = *p++ | ((skip & 0xF) << 8);
				offset = offset + 1 + skip;
				isSame = si_and((qword)isSame, si_ceq((qword)ucode[offset],(qword)srcReg));
				ucode[offset] = srcReg;
			}
		} while (*p != 255);
	}

	if (si_to_uint(si_gb(isSame)) != 15) 
	{
#if ENABLE_FRAG_UCODE_BRANCH_STRIPPING
		u32 doBranchStripping = (ProgramFlags & FP_PROGFLAG_STRIP_BRANCHES);

#if !__FINAL
		doBranchStripping = (pSpuGcmState->FragmentStrippingFlags & FRAGSTRIP_DEBUGFLAG_FORCE_DISABLE) ? 0 : doBranchStripping;
		doBranchStripping = (pSpuGcmState->FragmentStrippingFlags & FRAGSTRIP_DEBUGFLAG_FORCE_ENABLE)  ? 1 : doBranchStripping;
#endif
#endif

		// Do branch stripping
		u128* pFinalUCode = ucode;
		u32 nFinalUCodeProgramSize = g_CachedUcodeProgramSize;
#if ENABLE_FRAG_UCODE_BRANCH_STRIPPING
		if ( Unlikely(doBranchStripping) )
			grcFragmentProgram::StripBranches( ucode, g_CachedUcodeProgramSize, g_CachedUcodeDataSize, &pFinalUCode, &nFinalUCodeProgramSize );
#endif

		Assertf(g_CachedUcodeProgramSize >= nFinalUCodeProgramSize, "Branch stripping resulted in more code!");
	
		void *destAddr = gcmAllocateVram(pSpuGcmState->LastMicrocodeOffset,nFinalUCodeProgramSize);
		sysDmaLargePutf(pFinalUCode,(uint64_t)destAddr,nFinalUCodeProgramSize,FIFOTAG);
	}
	return pSpuGcmState->LastMicrocodeOffset;
}

#else	// __SPU

void grcEffect::ClearCachedState()
{
	SPU_SIMPLE_COMMAND(grcEffect__ClearCachedState,0);
}

#endif	// __SPU

void grcVertexProgram::SetParameter(int address,const float *data,int count)
{
	ValidateSpuPtr(data);
	GCM_DEBUG(cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,address,count,data));
}

#if __PPU
void grcFragmentProgram::RecordBind() const
{
	Assertf(Configuration.offset,"Recorded fragment program cannot support patching");
	GCM_DEBUG(cellGcmSetFragmentProgramLoad(GCM_CONTEXT,&Configuration));
}

void grcFragmentProgram::RecordSetTexture(int texUnit,const grcTextureObject * gcmTex,u16 samplerStateSet)
{
	extern grcSamplerState *g_SamplerStates;
	const grcSamplerState &ss = g_SamplerStates[samplerStateSet];
	Assertf(texUnit < 16,"Invalid fragment texture sampler index %d",texUnit);
	if (Unlikely(!gcmTex)) {
		GCM_DEBUG(GCM::cellGcmSetTextureControl(GCM_CONTEXT,texUnit,CELL_GCM_FALSE,0,0,CELL_GCM_TEXTURE_MAX_ANISO_1));
		return;
	}

	uint32_t current1 = gcmTex->offset;
	uint32_t current2 = 0x00000008 | (gcmTex->location + 1) | (gcmTex->cubemap << 2) 
		| (gcmTex->dimension << 4) | (gcmTex->format << 8) 
		| (gcmTex->mipmap << 16);

	// Sanity checks
	uint32_t *p = gcm::AllocateFifo_Secret(19);
	uint32_t current0 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_OFFSET + (texUnit) * 32, 2);
	uint32_t current3 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT + (texUnit) * 32, 1);
	p[0] = current0; p[1] = current1; p[2] = current2; p[3] = current3;

	uint32_t current4 = gcmTex->height | (gcmTex->width << 16);	// this is really aligned
	uint32_t current5 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_CONTROL3 + (texUnit) * 4, 1);
	uint32_t current6 = gcmTex->pitch | (gcmTex->depth << 20);
	uint32_t current7 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_CONTROL1 + (texUnit) * 32, 1);
	p[4] = current4; p[5] = current5; p[6] = current6; p[7] = current7;

	uint32_t current8 = gcmTex->remap;
	uint32_t current9 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_CONTROL0 + 0x20 * texUnit, 1);
	uint32_t current10 = ss.control0;
	uint32_t current11 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_FILTER + 0x20 * texUnit, 1);
	p[8] = current8; p[9] = current9; p[10] = current10; p[11] = current11;

	uint32_t current12 = ss.filter;
	uint32_t current13 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_CONTROL2 + 4 * texUnit, 1);
	uint32_t current14 = ss.control2;
	uint32_t current15 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR + 0x20 * texUnit, 1);
	p[12] = current12; p[13] = current13; p[14] = current14; p[15] = current15;

	uint32_t current16 = (ss.border&0x8000? 0xFF000000:0)|(ss.border&0x4000? 0x00FF0000:0)|(ss.border&0x2000? 0x0000FF00:0)|(ss.border&0x1000? 0x000000FF:0);
	uint32_t current17 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TEXTURE_ADDRESS + 0x20 * texUnit, 1);
	uint32_t current18 = ss.address | ((gcmTex->_padding & 0x0f) << 20); // 0x0f == CELL_GCM_TEXTURE_GAMMA_MASK
	p[16] = current16; p[17] = current17; p[18] = current18;
}

void grcVertexProgram::RecordBind() const
{
	Assertf(DefaultConstantRegisters && DefaultConstantValues && Program,"Recorded vertex shader bind is missing critical data -- add _REC suffix to the VS name.");
	g_VertexShaderInputs = Configuration.attributeInputMask;
	GCM_DEBUG(cellGcmSetVertexProgramLoad(GCM_CONTEXT,&Configuration,Program));
	for (u32 i=0; i<DefaultConstantCount; i++)
		cellGcmSetVertexProgramConstants(GCM_CONTEXT,DefaultConstantRegisters[i],4,&DefaultConstantValues[i<<2]);
}

void grcVertexProgram::RecordSetParameter(int address,const float *data,int count)
{
	GCM_DEBUG(GCM::cellGcmSetVertexProgramParameterBlock(GCM_CONTEXT,address,count,data));
}


static bool s_EdgeViewportCullEnable;

bool grcEffect::SetEdgeViewportCullEnable(bool enable)
{
	bool previous = s_EdgeViewportCullEnable;
	SPU_SIMPLE_COMMAND(grcEffect__SetEdgeViewportCullEnable, s_EdgeViewportCullEnable = enable);
	return previous;
}

extern spuGcmState s_spuGcmState;

#if !__FINAL
PARAM(edgeRing,"[grcore] Set edge ring buffer size allocation per SPU in kilobytes (default 3072k)");
PARAM(edgeType,"[grcore] Set edge ring buffer memory type (0=local or 1=main, default local)");
PARAM(edgeSpuPrio,"[grcore] Set priority of EDGE jobs on spu (1-15, default 6)");
PARAM(edgeSpuCount,"[grcore] Set number of SPUs that can run EDGE (1-4, default 3)");
#endif

extern size_t g_GcmInitBufferSize;
extern sysCriticalSectionTokenSpu s_KnownRefToken;
extern pgBaseKnownReferencePool s_KnownReferencePool;
extern u16 *s_MemoryReferences;

void grcEffect::InitClass()
{
	int edgeRing = 3*1024;
	int edgeType = CELL_GCM_LOCATION_LOCAL;
	int edgeSpuPrio = 6;
	int edgeSpuCount = 3;
#if !__FINAL
	PARAM_edgeRing.Get(edgeRing);
	PARAM_edgeType.Get(edgeType);
	PARAM_edgeSpuPrio.Get(edgeSpuPrio);
	PARAM_edgeSpuCount.Get(edgeSpuCount);
#endif
	edgeRing <<= 10;

#if __PPU
	s_spuGcmState.Globals = (u32) &sm_Globals;
	s_spuGcmState.FragmentConstants = (u32)grcFragmentProgram::sm_Constants;

#if 0
	s_spuGcmState.TrackStart = (u32)pgBase::TrackStart;
	s_spuGcmState.TrackEnd = (u32)pgBase::TrackEnd;
	s_spuGcmState.KnownRefToken = (u32)&s_KnownRefToken;
	s_spuGcmState.KnownRefPool = (u32)&s_KnownReferencePool;
	NOTFINAL_ONLY(s_spuGcmState.TrackArray = (u32)s_MemoryReferences);
#endif
	s_spuGcmState.CullerAABB = 1;

	// Edge, running on SPU 1-3
	uint8_t edgePriorities[8] = {
		(uint8_t)(edgeSpuCount>=4?edgeSpuPrio:0), 
		(uint8_t)(edgeSpuCount>=3?edgeSpuPrio:0), 
		(uint8_t)(edgeSpuCount>=2?edgeSpuPrio:0), 
		(uint8_t)edgeSpuPrio, 
		0, 0, 0, 0};

	// This limit is still fairly conservative -- it ignores the fact that
	// actually rendering the geometry consumes some buffer space, not to mention
	// any of the state changes.
	unsigned fifoSegmentCount = g_GcmInitBufferSize / SPU_FIFO_SEGMENT_SIZE;
	unsigned jobsPerSegment = SPU_FIFO_SEGMENT_SIZE / sizeof(edgegeomspujob::CellSpursEdgeJob);
	unsigned maxJobs = fifoSegmentCount * jobsPerSegment;

	GEOMETRY_JOBS.Initialize(g_Spurs, 
		maxJobs,
		edgeRing,			// ring buffer per SPU
		0 * 1024 * 1024,	// No shared linear overflow buffer
		edgeType,			// memory type
		edgePriorities);

	SetEdgeViewportCullEnable(true);
#endif
}


u32 grcEffect::GetSpuStateCounts()
{
	extern int g_SamplerStateCount;
	return g_SamplerStateCount;
}


void grcEffect::ShutdownClass()
{
#if GRCORE_ON_SPU
	// Edge
	GEOMETRY_JOBS.Shutdown();	      
#endif
	UnloadAll();
}

#endif		// PPU

void grcEffect::BeginFrame() 
{
	// Common code
	grcEffect::ApplyDefaultRenderStates();
	grcEffect::ApplyDefaultSamplerStates();

#if __PPU
	// Edge
	GEOMETRY_JOBS.BeginFrame();
#endif
}

void grcEffect::EndFrame() 
{	
#if __PPU
	// Edge
	GEOMETRY_JOBS.EndFrame();
#endif
}

void grcEffect::ApplyDefaultSamplerStates()
{
}


#if 0
void SamplerState::Set(u32 state,u32 value)
{
	static u8 remapWrap[] = {
		CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_MIRROR, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, 
		CELL_GCM_TEXTURE_BORDER, CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP
	};
	static u8 remapAnisotropy[17] = {
		CELL_GCM_TEXTURE_MAX_ANISO_1, CELL_GCM_TEXTURE_MAX_ANISO_1, CELL_GCM_TEXTURE_MAX_ANISO_2, CELL_GCM_TEXTURE_MAX_ANISO_2,		// 0-3
		CELL_GCM_TEXTURE_MAX_ANISO_4, CELL_GCM_TEXTURE_MAX_ANISO_4, CELL_GCM_TEXTURE_MAX_ANISO_6, CELL_GCM_TEXTURE_MAX_ANISO_6,		// 4-7
		CELL_GCM_TEXTURE_MAX_ANISO_8, CELL_GCM_TEXTURE_MAX_ANISO_8, CELL_GCM_TEXTURE_MAX_ANISO_10, CELL_GCM_TEXTURE_MAX_ANISO_10,	// 8-11
		CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12,	// 12-15
		CELL_GCM_TEXTURE_MAX_ANISO_16
	};
	/*
	CELL_GCM_TEXTURE_NEAREST			Read the values of the texel closest to the texel coordinates and use them as the texture values
	CELL_GCM_TEXTURE_LINEAR				Read the values of the four texels closest to the texel coordinates and use the weighted average of these values as the texture values. Depending on the wrap mode, use the value of the texture border as well
	CELL_GCM_TEXTURE_NEAREST_NEAREST	Select the mipmap level closest to the texel coordinates and from a texture of that level, read the values of the texel closest to the texel coordinates and use them as the texture values
	CELL_GCM_TEXTURE_LINEAR_NEAREST 	Select the mipmap level closest to the texel coordinates and from a texture of that level, read the values of the four texels closest to the texel coordinates and use the weighted average of these values as the texture values
	CELL_GCM_TEXTURE_NEAREST_LINEAR 	Select the two mipmap levels closest to the texel coordinates and from a texture of each level, read the values of the texel closest to the texel coordinates, and use the weighted averages of these values as the texture values
	CELL_GCM_TEXTURE_LINEAR_LINEAR		Select the two mipmap levels closest to the texel coordinates and from a texture of each level, read the values of the four texels closest to the texel coordinates, take the weighted averages of these four values, and then use the weighted average of the values of the two levels as the texture values
	CELL_GCM_TEXTURE_CONVOLUTION_MIN	Calculate the weighted average of 2x3 or 3x3 texels that is close to the texel coordinate.	The calculation expression for the weighted average is specified with conv.
	filters: TEXF_NONE = 0, TEXF_POINT = 1, TEXF_LINEAR = 2, TEXF_ANISOTROPIC = 3 */
	static u8 remapMin[4][7] =	{ // [MIPFILTER][MINFILTER] 
		//						   TEXF_NONE				TEXF_POINT							TEXF_LINEAR							TEXF_ANISOTROPIC
		/* NO MIPMAPPING: */	{ CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST,			CELL_GCM_TEXTURE_LINEAR,			CELL_GCM_TEXTURE_LINEAR,			CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN },
		/* NEAREST MIPMAP: */	{ CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST_NEAREST,	CELL_GCM_TEXTURE_LINEAR_NEAREST,	CELL_GCM_TEXTURE_LINEAR_NEAREST,	CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN },
		/* TRILINEAR: */		{ CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST_LINEAR,	CELL_GCM_TEXTURE_LINEAR_LINEAR,		CELL_GCM_TEXTURE_LINEAR_LINEAR,		CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN },
		/* ANISOTROPIC: */		{ CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST_LINEAR,	CELL_GCM_TEXTURE_LINEAR_LINEAR,		CELL_GCM_TEXTURE_LINEAR_LINEAR,		CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MIN }
	};

	static u8 remapMag[7] = { CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_NEAREST, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_MAG, CELL_GCM_TEXTURE_CONVOLUTION_MAG, CELL_GCM_TEXTURE_CONVOLUTION_MAG };
	static u8 remapConv[7] = { CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX,
		CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX, CELL_GCM_TEXTURE_CONVOLUTION_GAUSSIAN, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX_ALT };

	switch (state) {
	case grcessADDRESSU: 
		BitfieldAssign(wraps,remapWrap[value]); 
		break;
	case grcessADDRESSV: 
		BitfieldAssign(wrapt,remapWrap[value]); 
		break;
	case grcessADDRESSW: 
		BitfieldAssign(wrapr,remapWrap[value]); 
		break;
	case grcessBORDERCOLOR: 
		border = value? -1 : 0; 
		break;
	case grcessMAGFILTER:
		BitfieldAssign(magFilterFake,value);
		break;
	case grcessMINFILTER:
		BitfieldAssign(minFilterFake,value);
		break;
	case grcessMIPFILTER:
		BitfieldAssign(mipFilterFake,value);
		break;
	case grcessMIPMAPLODBIAS: 
		union { float f; u32 u; } x;
		x.u = value;
		value = int(x.f * 256.0f) & 0x1FFF;
		BitfieldAssign(mipBias,value);
		break;
	case grcessMAXMIPLEVEL: 
		BitfieldAssign(minlod,value);	// nomenclature is backwards from D3D
		break;
	case grcessMAXANISOTROPY:
		BitfieldAssign(anisoFake,value);
		break;
	case grcessTRILINEARTHRESHOLD: 
		BitfieldAssign(triThresh,value); 
		break;
	case grcessMINMIPLEVEL: 
		BitfieldAssign(maxlod,value);	// nomenclature is backwards from D3D
		break;
	case grcessTEXTUREZFUNC:
		BitfieldAssign(zfunc,value); 
		break;
	}

	// Several states are interdependent, so clean them up based on cached values:
	if ((1<<state) & ((1<<grcessMAGFILTER)|(1<<grcessMINFILTER)|(1<<grcessMIPFILTER)|(1<<grcessMAXANISOTROPY))) {
		BitfieldAssign(magFilter,remapMag[magFilterFake]);
		BitfieldAssign(minFilter,remapMin[mipFilterFake][minFilterFake]);

		if (minFilterFake != TEXF_ANISOTROPIC && magFilterFake != TEXF_ANISOTROPIC)
			maxAniso = CELL_GCM_TEXTURE_MAX_ANISO_1;
		else
			BitfieldAssign(maxAniso,remapAnisotropy[anisoFake]);
		conv = remapConv[Max(minFilterFake,magFilterFake)];
	}
}
#endif

}	// namespace rage

#endif	// __PS3

