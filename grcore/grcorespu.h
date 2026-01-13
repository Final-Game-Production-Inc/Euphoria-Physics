// 
// grcore/grcorespu.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_GRCORESPU_H 
#define GRCORE_GRCORESPU_H 

#define DRAWABLESPU_STATS		            (__BANK && __PS3)

#define DEBUG_ALLOW_CACHED_STATES_TOGGLE    (__BANK && 0)

#if DEBUG_ALLOW_CACHED_STATES_TOGGLE
	#define DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(...)  __VA_ARGS__
#else
	#define DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(...)
#endif

#if HACK_GTA4
	#define EDGE_NUM_MODEL_CLIP_PLANES    12
	#define EDGE_NUM_TRIANGLE_CLIP_PLANES 8
	#define EDGE_STRUCT_SIZE_NO_ASSERT    (EDGE_NUM_MODEL_CLIP_PLANES != 12 || EDGE_NUM_TRIANGLE_CLIP_PLANES != 4)
#else
	#define EDGE_NUM_MODEL_CLIP_PLANES    8
	#define EDGE_NUM_TRIANGLE_CLIP_PLANES 4
	#define EDGE_STRUCT_SIZE_NO_ASSERT    0
#endif

// these can be disabled to reduce stress on SPU at the expense of the GPU
#define ENABLE_EDGE_CULL_CLIPPED_TRIANGLES                     ((1) && EDGE_NUM_TRIANGLE_CLIP_PLANES > 0)
#define ENABLE_EDGE_CULL_CLIPPED_TRIANGLES_FOR_NON_AABB_MODELS ((0) && ENABLE_EDGE_CULL_CLIPPED_TRIANGLES) // too slow to call edgeGeomCullClippedTriangles on everything

CompileTimeAssert(EDGE_NUM_MODEL_CLIP_PLANES    % 4 == 0);
CompileTimeAssert(EDGE_NUM_TRIANGLE_CLIP_PLANES % 4 == 0);
CompileTimeAssert(EDGE_NUM_TRIANGLE_CLIP_PLANES <= EDGE_NUM_MODEL_CLIP_PLANES);

#define SPU_FRAGMENT_PROGRAM_CACHE_SIZE 16384

#if !__FINAL
enum
{
	CULLERDEBUGFLAG_CULL_FRUSTUM_LRTB           = 1<<0, // cull models/geoms against frustum LRTB planes
	CULLERDEBUGFLAG_CULL_EDGE_CLIP_PLANES       = 1<<1, // cull models/geoms/triangles against EdgeClipPlanes
	CULLERDEBUGFLAG_CULL_TRIANGLES              = 1<<2, // cull triangles against EdgeClipPlanes
	CULLERDEBUGFLAG_SPUMODEL_DRAW               = 1<<3, // draw using spuModel::Draw codepath
	CULLERDEBUGFLAG_SPUMODEL_DRAWSKINNED        = 1<<4, // draw using spuModel::DrawSkinned codepath
	CULLERDEBUGFLAG_SPUGEOMETRYQB_DRAW          = 1<<5, // draw using spuGeometryQB::Draw codepath
	CULLERDEBUGFLAG_GRMGEOMETRYEDGE_DRAW        = 1<<6, // draw using grmGeometryEdge__Draw codepath
	CULLERDEBUGFLAG_GRMGEOMETRYEDGE_DRAWSKINNED = 1<<7, // draw using grmGeometryEdge__Draw codepath
};

enum
{
	FRAGSTRIP_DEBUGFLAG_FORCE_DISABLE           = 1<<0, // force disable branch stripping of all fragment programs
	FRAGSTRIP_DEBUGFLAG_FORCE_ENABLE			= 1<<1, // force enable branch stripping of all fragment programs
};
#endif // !__FINAL

#if __PS3

#include "grcore/stateblock.h"
#include "grcore/vertexdecl.h"
#include "grcore/wrapper_gcm.h"
#include "math/float16.h"
#include "paging/handle.h"
#include "system/criticalsection.h"
#include "system/noncopyable.h"

#if __PPU
#include "vector/matrix44.h"
#include "vectormath/mat44v.h"
#else
#include "math/intrinsics.h"
#endif

#if __SPU
#define SIMPLIFIED_ATL
#include "atl/array.h"
#include <cell/gcm_spu.h>
#include <cell/gcm/gcm_method_data.h>
#include "system/spuget.h"
#include "system/spuintrinsics.h"
#else
#include "atl/array.h"
#include <cell/gcm.h>
#endif

#include "edge/geom/edgegeom_structs.h"
#include "grcore/edge_jobs.h"
#include "grcore/gcmringbuffer.h"
#include "profile/cellspurstrace.h"
#include "paging/base_spu.h"			// for TRACK_REFERENCE_BACKTRACE

struct CellGcmTexture;

namespace rage {

#if DRAWABLESPU_STATS

enum
{
	DCC_NO_CATEGORY = 0,
	DCC_LOD,
	DCC_SLOD1,
	DCC_SLOD2,
	DCC_SLOD3,
	DCC_SLOD4,
	DCC_PROPS,
	DCC_VEG,
	DCC_PEDS,
	DCC_VEHICLES,
	DCC_MAX_CONTEXT
};
struct spuDrawableStats {
	// EDGE
	u32 TotalIndices,			// total number of indices processed by EDGE jobs
		VisibleIndices,			// total number that were deemed visible (not offscreen etc)
		CullClippedIndices,		// total number of indices that were processed through edgeGeomCullClippedTriangles
		EdgeJobs,				// total number of edge jobs
		TrivialRejectEdgeJobs;	// total number of edge jobs that trivially rejected all input
	u32 EdgeOutputSpaceUsed;	// total amount of output space used
	u32 EdgeOutputWaitTicks[6];	// In SPU decrementer ticks, 79800 cycles per millisecond, per EDGE spu

	// drawablespu; note that these need to be declared in drawablespu.cpp as g_Stat_whatever variables
	// for the macros to work.  They also needed to be added to the ACCUM calls in grcorespu_footer.h.
	u32 GcmCommands,				// total number of gcm commands
		MacroCommands,				// total number of macro commands
		MissingTechnique,			// number of times grmShader::BeginDraw returned zero passes
		ModelsCulled,				// number of models that were culled entirely by their root AABB's
		ModelsDrawn,				// number of models that were rendered (any model not culled is drawn)
		GeomsCulled,				// number of geometries that were culled by their local AABB's (may include ones that were not even in this bucket because of how the code is set up)
		GeomsDrawn,					// number of geometries that were drawn (were not culled, and were in current render bucket)
		DrawableDrawCalls,			// number of rmcDrawable::Draw calls processed on SPU
		DrawableDrawSkinnedCalls;	// number of rmcDrawable::DrawSkinned calls processed on SPU
	u32 EntityDrawCalls,			// number of draw calls (includes skinned) for a specific entity
		GcmDrawCalls,				// number of direct GCM calls (non-edge)
		GcmDrawIndices;				// number of indices sent via direct GCM calls (non-edge)
	u32 RingBufferUsed;				// amount of output ring buffer space (not macro, actual GCM) consumed.
	u16 DrawCallsPerContext[DCC_MAX_CONTEXT];

} ALIGNED(128);
#define DRAWABLESPU_STATS_ONLY(...)	__VA_ARGS__
#define DRAWABLESPU_STATS_ADD(x,y)	(g_Stats.x += (y))
#define DRAWABLESPU_STATS_CONTEXT_ADD(x,y)	(g_Stats.DrawCallsPerContext[x] += (y))
#else
#define DRAWABLESPU_STATS_ONLY(...)
#define DRAWABLESPU_STATS_ADD(x,y)	
#define DRAWABLESPU_STATS_CONTEXT_ADD(x,y)
#endif
#define DRAWABLESPU_STATS_INC(x)	DRAWABLESPU_STATS_ADD(x,1)
#define DRAWABLESPU_STATS_CONTEXT_INC(x)	DRAWABLESPU_STATS_CONTEXT_ADD(x,1)

class grcTexture;
class grmMatrixSet;
class spuMatrixSet;
class grcEffect;
class sysTaskLog;
class grcFragmentProgram;
class grcInstanceData;

#define BLOCK_DEPTHACCESS		0x01
#define BLOCK_ALPHATEST			0x02
#define BLOCK_ALPHATOMASK		0x04
#define BLOCK_STENCILACCESS		0x08
#define MSAA_ENABLED			0x10
#define BLOCK_COLORWRITE		0x20

#define _VECTORMATH_SHUF_X 0x00010203
#define _VECTORMATH_SHUF_Y 0x04050607
#define _VECTORMATH_SHUF_Z 0x08090a0b
#define _VECTORMATH_SHUF_W 0x0c0d0e0f
#define _VECTORMATH_SHUF_A 0x10111213
#define _VECTORMATH_SHUF_B 0x14151617
#define _VECTORMATH_SHUF_C 0x18191a1b
#define _VECTORMATH_SHUF_D 0x1c1d1e1f
#define _VECTORMATH_SHUF_XAYB ((vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_A, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_B })
#define _VECTORMATH_SHUF_ZCWD ((vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_C, _VECTORMATH_SHUF_W, _VECTORMATH_SHUF_D })

#define FIFOTAG							9
#define PPU_FIFO_SEGMENT_SIZE			8192
#define SPU_FIFO_SEGMENT_SIZE			8192		// Must be at least as large as PPU_FIFO_SEGMENT_SIZE
#define COMMAND_BUFFER_SEGMENT_SIZE		1024

#if __PPU
typedef Matrix44 spuMatrix44;
struct spuMatrix43 
{
	__vector4 x, y, z;
};
#else
struct spuMatrix43;
struct spuMatrix44
{
	__vector4 a, b, c, d;

	__vector4 Transform(__vector4 vec) const
	{
		// Stolen from sony vector lib
		vec_float4 tmp0, tmp1, res;
		vec_float4 xxxx, yyyy, zzzz, wwww;
		vec_uchar16 shuffle_xxxx = (vec_uchar16)spu_splats((int)0x00010203);
		vec_uchar16 shuffle_yyyy = (vec_uchar16)spu_splats((int)0x04050607);
		vec_uchar16 shuffle_zzzz = (vec_uchar16)spu_splats((int)0x08090a0b);
		vec_uchar16 shuffle_wwww = (vec_uchar16)spu_splats((int)0x0c0d0e0f);
		xxxx = spu_shuffle( vec, vec, shuffle_xxxx );
		yyyy = spu_shuffle( vec, vec, shuffle_yyyy );
		zzzz = spu_shuffle( vec, vec, shuffle_zzzz );
		wwww = spu_shuffle( vec, vec, shuffle_wwww );
		tmp0 = spu_mul( a, xxxx );
		tmp1 = spu_mul( b, yyyy );
		tmp0 = spu_madd( c, zzzz, tmp0 );
		tmp1 = spu_madd( d, wwww, tmp1 );
		res = spu_add( tmp0, tmp1 );
		return res ;
	}

	void Transform(const spuMatrix44 &m1,const spuMatrix44 &m2)
	{
		a = m1.Transform(m2.a);
		b = m1.Transform(m2.b);
		c = m1.Transform(m2.c);
		d = m1.Transform(m2.d);
	}

	void Transpose(const spuMatrix44 &m)
	{
		// Verified the compiler really is smart enough to avoid an extra copy between resN and final destination.
		// Would have been shocked had that not been the case, but still worth checking.
		vec_float4 tmp0, tmp1, tmp2, tmp3, res0, res1, res2, res3;
		tmp0 = spu_shuffle( m.a, m.c, _VECTORMATH_SHUF_XAYB );
		tmp1 = spu_shuffle( m.b, m.d, _VECTORMATH_SHUF_XAYB );
		tmp2 = spu_shuffle( m.a, m.c, _VECTORMATH_SHUF_ZCWD );
		tmp3 = spu_shuffle( m.b, m.d, _VECTORMATH_SHUF_ZCWD );
		res0 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_XAYB );
		res1 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_ZCWD );
		res2 = spu_shuffle( tmp2, tmp3, _VECTORMATH_SHUF_XAYB );
		res3 = spu_shuffle( tmp2, tmp3, _VECTORMATH_SHUF_ZCWD );
		a = res0;
		b = res1;
		c = res2;
		d = res3;
	}

	void Transpose(const spuMatrix43 &m);
};

struct spuMatrix43
{
	__vector4 x, y, z;

	void Transpose(const spuMatrix44 &m)
	{
		vec_float4 tmp0, tmp1, tmp2, tmp3, res0, res1, res2;
		tmp0 = spu_shuffle( m.a, m.c, _VECTORMATH_SHUF_XAYB );
		tmp1 = spu_shuffle( m.b, m.d, _VECTORMATH_SHUF_XAYB );
		tmp2 = spu_shuffle( m.a, m.c, _VECTORMATH_SHUF_ZCWD );
		tmp3 = spu_shuffle( m.b, m.d, _VECTORMATH_SHUF_ZCWD );
		res0 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_XAYB );
		res1 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_ZCWD );
		res2 = spu_shuffle( tmp2, tmp3, _VECTORMATH_SHUF_XAYB );
		x = res0;
		y = res1;
		z = res2;
	}
};

void spuMatrix44::Transpose(const spuMatrix43 &m)
{
	vec_float4 tmp0, tmp1, tmp2, tmp3, res0, res1, res2, res3;
	vec_float4 ZZZ1 =  (vec_float4){0.0f,0.0f,0.0f,1.0f};
	tmp0 = spu_shuffle( m.x, m.z, _VECTORMATH_SHUF_XAYB );
	tmp1 = spu_shuffle( m.y, ZZZ1, _VECTORMATH_SHUF_XAYB );
	tmp2 = spu_shuffle( m.x, m.z, _VECTORMATH_SHUF_ZCWD );
	tmp3 = spu_shuffle( m.y, ZZZ1, _VECTORMATH_SHUF_ZCWD );
	res0 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_XAYB );
	res1 = spu_shuffle( tmp0, tmp1, _VECTORMATH_SHUF_ZCWD );
	res2 = spu_shuffle( tmp2, tmp3, _VECTORMATH_SHUF_XAYB );
	res3 = spu_shuffle( tmp2, tmp3, _VECTORMATH_SHUF_ZCWD );
	a = res0;
	b = res1;
	c = res2;
	d = res3;
}

#endif

struct spuVertexDeclaration
{
	static const int c_MaxAttributes = 16;
	struct AttributeFormat {	// Register 0x1740 + 4*i
		unsigned divider:16, stride:8, count:4, type:4;
	};
	union {
		AttributeFormat Format[c_MaxAttributes];
		unsigned FormatU[c_MaxAttributes];
#if __SPU
		qword FormatV[4];
#endif
	};
#if __SPU
	qword OffsetV, StreamV;
#else
	unsigned char	Offset[c_MaxAttributes];
	unsigned char	Stream[c_MaxAttributes];
#endif
	unsigned Stream0Size;
	int				RefCount;
	unsigned StreamFrequencyMode;		// must live at offset 8 within quadword.
	unsigned pad;
};

struct spuVertexBuffer
{
	u32	vptr;
	u16					m_VertCount;			// +4
	mutable bool		m_Locked;				// +6
	u8					m_Flags;				// +7
	mutable void*		m_LockPtr;				// +8
	u32					m_Stride;				// +12
	u8*					m_VertexData;			// +16
	u32					m_LockThread;			// +20
	void*				m_Fvf;					// +24
	void				*m_GCMBuffer;
};

struct spuIndexBuffer
{
	u32 vptr;
	int				m_IndexCount;
	u16				*m_IndexData;
	u32				m_Offset;
};

#if USE_PACKED_GCMTEX
#define SPU_TEXTURE_SIZE 16
#else
#define SPU_TEXTURE_SIZE 32
#endif

struct spuTexture
{
	enum {	
		NORMAL,				// Standard texture
		RENDERTARGET,		// Render target
		REFERENCE,			// Reference to an existing texture
		DICTIONARY_REFERENCE, // Reference to an existing texture in a dictionary
	};

	u32 vptr;						// +0
	u32 *m_PageMap;					// +4
	CellGcmTexture	m_Texture;		// +8

	// Only the first 32 bytes of an spuTexture are typically actually resident, so we leave the others off
	u8 unsafe[20];

	CellGcmTexture*	GetTexturePtr() { return &m_Texture; }
	CellGcmTexture& GetGcmTexture() { return m_Texture; }
};


// IF YOU ADD ANYTHING HERE, PLEASE ADJUST spuCommands[] in wrapper_gcm.cpp.
enum spuCommandsBase {
	grcDevice__BeginFrame,
	grcDevice__EndFrame,
	grcDevice__Finish,

	grcVertexProgram__SetFlag,

	grcEffect__SetGlobalFloatCommon,
	grcEffect__SetGlobalVar_grcTexture,
	grcEffect__SetEdgeViewportCullEnable,

	grcViewport__RegenerateDevice,
	grcViewport__SetWindow,
	grcDevice__SetScissor,
	grcState__SetLightingMode,

	grcDevice__WriteStateStruct,

	grcDevice__Jump,
	grcEffect__RenderStateBlockMask,

	grcEffect__Bind,
	grcEffect__IncrementalBind,
	grcEffect__UnBind,

	grcEffect__SetVarFloatCommon,
	grcEffect__SetVarFloatCommonByRef,
	grcEffect__SetVar_grcTexture,
	grcEffect__SetVar_Color32,

	grcEffect__ClearCachedState,
	// grcEffect__SetCullMode,
	grcDevice__ChangeTextureRemap,
	// grcDevice__RunCommandBuffer,
	// grcDevice__ReturnFromCommandBuffer,
	// grcDevice__NextCommandBufferSegment,
	grcDevice__DrawPrimitive,
	grcDevice__DrawIndexedPrimitive,
	grcDevice__DrawInstancedPrimitive,
	grcDevice__SetSurface,
	grcGeometryJobs__SetOccluders,
	grcGeometryJobs__SetEdgeNoPixelTestEnable,
#if __BANK
	grcGeometryJobs__SetEdgeCullDebugFlags,
#endif // __BANK
	grcDevice__SetPixelShaderConstantF,
	grcDevice__SetEdgeClipPlane,
	grcDevice__SetEdgeClipPlaneEnable,
#if !__FINAL
	grmModel__SetCullerDebugFlags,
	grcEffect__SetFragStripDebugFlags,
#endif // !__FINAL
	grmModel__SetCullerAABB,

#if !__FINAL
	grcDevice__PushFaultContext,
	grcDevice__PopFaultContext,
	grcStateBlock__SetWireframeOverride,
#endif
	grcDevice__CreateGraphicsJob,
#if HACK_GTA4
	grcEffect__CopyByte,
#endif
#if DRAWABLESPU_STATS
	grcEffect__FetchStats,
#endif
	grcStateBlock__SetDepthStencilState,
	grcStateBlock__SetRasterizerState,
	grcStateBlock__SetBlendState,
#if LAZY_STATEBLOCKS
	grcStateBlock__Flush,
#endif
	grcEffect__SetSamplerState,
	grcEffect__PushSamplerState,
	grcEffect__PopSamplerState,
	grcDevice__grcBegin,
	grcDevice__BindVertexFormat,

	GRCORE_COMMAND_BASE_COUNT
};

// We freely intermix SPU and GPU commands together.  The GPU has the following basic format:
// u16 size; u16 register;
// The upper four bits of size can be nonzero in some situations, as can the lower two bits.
// The size is the count of words NOT including this command word itself.  The register field
// uses the two LSB's to indicate various things, and 0x3 is invalid, so we use that to
// mark all of our commands.  This allows the PPU to insert GPU commands as necessary; the
// SPU job just copies GPU data over into its FIFO as necessary.

struct spuCmd_Any {
	u16 size; u8 subcommand; u8 command;

private:
	//Do not try to copy spu commands! Certain values are filled in by the creation/allocation macros.
	NON_COPYABLE(spuCmd_Any);

	//Adding in default constructor/destructor b/c they were there before adding NON_COPYABLE
public:
	spuCmd_Any()	{ }
	~spuCmd_Any()	{ }
};

struct spuCmd_spuCommand__nop: public spuCmd_Any {
	// no operation, only used to maintain padding.
};

struct spuCmd_grcEffect__SetDefaultRenderState: public spuCmd_Any {
	// subcommand is grceRenderState enum 
	u32 payload;
};

struct spuCmd_grcEffect__SetDefaultSamplerState: public spuCmd_Any {
	// subcommand is grceSamplerState enum.
	u32 payload;
};

struct spuCmd_grcEffect__PushDefaultSamplerState: public spuCmd_Any {
	// subcommand is grceSamplerState enum.
	u32 payload;
};

struct spuCmd_grcEffect__SetGlobalFloatCommon: public spuCmd_Any {
	u8 Register, Usage; u16 qwCount;
	u32 pad1, pad2;		// so that alignedPayload is, you know, aligned.
	// subcommand is global variable handle
	// payload varies with the type of the variable
	u32 alignedPayload[0];
};

CompileTimeAssert(sizeof(spuCmd_grcEffect__SetGlobalFloatCommon) == 16);

struct spuCmd_grcEffect__SetVarFloatCommon: public spuCmd_Any {
	// subcommand is local variable handle
	// payload varies with the type of the variable
	void *dest;
	u16 stride, count;
	const grcEffect* effect;
	u32 alignedPayload[0];
};

CompileTimeAssert(sizeof(spuCmd_grcEffect__SetVarFloatCommon) == 16);

struct spuCmd_grcEffect__SetVarFloatCommonByRef: public spuCmd_Any {
	// subcommand is local variable handle
	// payload varies with the type of the variable
	void *dest;
	u16 stride, count;
	const grcEffect* effect;
	void *src;
};

struct spuCmd_grcEffect__SetGlobalVar_grcTexture: public spuCmd_Any {
	u8 Register, Usage;
	u16 SamplerStateSet;
#if USE_PACKED_GCMTEX
	u32 textureHandle;
#else
	void *texture;
#endif
};

struct spuCmd_grcEffect__SetVar_grcTexture: public spuCmd_Any {
	void *instanceData;
	const grcEffect* effect;
#if USE_PACKED_GCMTEX
	u32 textureHandle;
#else
	pgHandle<grcTexture> textureHandle;
#endif
	void* data;
#if TRACK_REFERENCE_BACKTRACE
	u16 backtrace;
#endif
};

struct spuCmd_grcEffect__SetVar_Color32: public spuCmd_Any {
	void *instanceData;
	const grcEffect* effect;
	float *dest;		// on PPU
	u32 colors[0];
};

/* struct spuCmd_grcDevice__SetVertexShaderConstantByRef: public spuCmd_Any {
	void *data;
	u16 count, regbase;
}; */

struct spuCmd_grcEffect__Bind: public spuCmd_Any {
	u16 technique, passIndex;		// subcommand is unused, available for something
	grcEffect *effect;		// Effect to bind
	grcInstanceData *instanceData;
#if __BANK
	u8 minRegisterCount; // Needed for shmoo'ing
	u8 Pad[3];
#endif

};

struct spuCmd_grcEffect__IncrementalBind: public spuCmd_Any {
	u16 technique, passIndex;		// subcommand is unused, available for something
	grcEffect *effect;		// Effect to bind
	grcInstanceData *instanceData;

};

struct spuCmd_grcEffect__UnBind: public spuCmd_Any {
	u16 technique, passIndex;		// subcommand is unused, available for something
	grcEffect *effect;		// Effect to un-bind

};

struct spuCmd_grcDevice__WriteStateStruct: public spuCmd_Any {
	u16 offset;
	u8  count;
	u8  value;
};

struct spuCmd_grcDevice__Jump: public spuCmd_Any {
	u32 jumpLocalOffset;	// Memory offset of the jump target (the address of the list 
							// you want to jump to from the main GCM FIFO)
	u32 *jumpBack;			// PPU address of the return jump (the address of the last instruction 
							// in that list, which will be patched to jump back to the main GCM FIFO)
};

struct spuCmd_grcDevice__ChangeTextureRemap: public spuCmd_Any {
	u32 texture;
	u32 remap;
};

struct spuCmd_grcDevice__RunCommandBuffer: public spuCmd_Any {
	// If page size is nonzero, command buffer will be aligned to address that is
	// multiple of it, so the LSB's are free to store the (initial segment size-1).
	// We do this so that the PPU never has to waste a cache miss looking at the
	// command buffer's contents.
	u32 addrAndSize;
};

struct spuCmd_grcDevice__NextCommandBufferSegment: public spuCmd_Any {
	// Same as RunCommandBuffer.
	u32 addrAndSize;
};

struct spuCmd_grcDevice__DrawPrimitive: public spuCmd_Any {
	// primtype is in subcommand
	spuVertexDeclaration *decl;
	u32 startVertex;
	u32 vertexCount;
	u32 vertexData[grcVertexDeclaration::c_MaxStreams];		// this needs to be qw-aligned!
};

struct spuCmd_grcDevice__DrawIndexedPrimitive: public spuCmd_Any {
	// primtype is in subcommand
	spuVertexDeclaration *decl;
	u32 indexData;
	u32 indexCount;
	u32 vertexData[grcVertexDeclaration::c_MaxStreams];		// this needs to be qw-aligned!

};

struct spuCmd_grcDevice__DrawInstancedPrimitive: public spuCmd_Any {
	// primtype is in subcommand
	spuVertexDeclaration *decl;
	u32 indexData;
	const void *instData;
	u32 vertexData[grcVertexDeclaration::c_MaxStreams]; 	// this needs to be qw-aligned!
	u32 indexCount;
	u16 instCount;
	u16	elemSizeQW; 
};

struct spuCmd_grcEffect__RenderStateBlockMask: public spuCmd_Any {
	u16 Mask;				// Bit mask for render states we want to block
	u16 Pad;
};
AlignedCompileTimeAssert(sizeof(spuCmd_grcEffect__RenderStateBlockMask), 4);

struct spuCmd_grcViewport__RegenerateDevice: public spuCmd_Any {
#if __PPU		// First four are in this order because they're sent down as one block
	Mat44V		world, worldView, composite, viewInverse, view, proj, frustum;
#else
	spuMatrix44 world, worldView, composite, viewInverse, view, proj, frustum;
#endif
} ;

struct spuCmd_grcDevice__SetSurface: public spuCmd_Any {
	u8 colorFormat;
	u8 depthFormat;
	u8 depthFormatType;
};

struct spuCmd_grcViewport__SetWindow: public spuCmd_Any {	// sampleFlavor lives in subcommand
	uint16_t scissorArea[4];
	float depthRange[2];
	float viewportScales[4];
	float viewportOffsets[4];
};


struct spuCmd_grcDevice__SetScissor: public spuCmd_Any {
	uint16_t scissorArea[4];
};

struct spuCmd_grcGeometryJobs__SetOccluders: public spuCmd_Any { // count lives in subcommand
	void* occluderQuads; // array of up to eight quads (4 x float4)
};

#if __BANK
struct spuCmd_grcGeometryJobs__SetEdgeCullDebugFlags: public spuCmd_Any {
	uint16_t flags; // eEdgeGeomCullingFlags
	uint16_t pad;
};
#endif // __BANK

struct spuCmd_grcDevice__SetPixelShaderConstantF: public spuCmd_Any {	// address in subcommand
	u32 alignedPayload[0];
};

struct spuCmd_grcDevice__SetEdgeClipPlane: public spuCmd_Any {
	__vector4 plane;
};

struct spuCmd_grcDevice__PushFaultContext: public spuCmd_Any {
	char labelText[0];
};

struct spuCmd_grcDevice__CreateGraphicsJob: public spuCmd_Any {		// user data count in subcommand
	const char *JobStart, *JobSize;
	size_t InputSize;
	void *InputData;
	size_t ScratchSize, SpuStackSize;
	int asInt[0];
};

#if HACK_GTA4
struct spuCmd_grcEffect__CopyByte: public spuCmd_Any {
	void *srcPtr;
	void *dstPtr;
};
#endif //HACK_GTA4...

#if DRAWABLESPU_STATS
struct spuCmd_grcEffect__FetchStats: public spuCmd_Any {
	spuDrawableStats *dstPtr;
};
#endif

struct spuCmd_grcStateBlock__SetDepthStencilState: public spuCmd_Any {	// stencil ref is in subcommand
	grcDepthStencilStateHandle handle;
};

struct spuCmd_grcStateBlock__SetRasterizerState: public spuCmd_Any {
	grcRasterizerStateHandle handle;
};

struct spuCmd_grcStateBlock__SetBlendState: public spuCmd_Any {	// alpha ref is in subcommand
	grcBlendStateHandle handle;
	u32 multisample;
	u32 argb;
};

struct spuCmd_grcStateBlock__LockState: public spuCmd_Any { // index is in subcommand
	ASSERT_ONLY(u32 key);
};

struct spuCmd_grcStateBlock__UnlockState: public spuCmd_Any { // index is in subcommand
	ASSERT_ONLY(u32 key);
};

struct spuCmd_grcEffect__SetSamplerState: public spuCmd_Any {	// var index is in subcommand
	grcInstanceData *instanceData;
	grcSamplerStateHandle handle, pad[3];
	const grcEffect* effect;
};

struct spuCmd_grcEffect__PushSamplerState: public spuCmd_Any {	// var index is in subcommand
	grcInstanceData *instanceData;
	grcSamplerStateHandle handle, pad[3];
	const grcEffect* effect;
};

struct spuCmd_grcEffect__PopSamplerState: public spuCmd_Any {	// var index is in subcommand
	grcInstanceData *instanceData;
	const grcEffect* effect;
};

struct spuCmd_grcDevice__grcBegin: public spuCmd_Any {	// normalCount is in subcommand
	u16 common;		// stride << 8
	u8 colorCount, texCount;
};

struct spuCmd_grcDevice__BindVertexFormat: public spuCmd_Any {
	void *vertexdecl;
};

#if !__FINAL
struct spuDrawable;
struct spuGeometry;
#endif // !__FINAL

struct CGta4DbgSpuInfoStruct
{
	union
	{
		u32		payload;
		struct
		{
			u32 gta4ModelInfoIdx	: 16;	// modelInfoIdx of currently rendered entity
			u32 gta4RenderPhaseID	: 5;	// current renderphaseID
			u32 gta4ModelInfoType	: 5;	// ModelInfoType (MI_TYPE_VEHICLE, MI_TYPE_PED, etc.)
			u32 gta4MaxTextureSize	: 4;	// maxTexture size
			u32 gta4Pad000			: 2;	// free
		};
	};
#if __PPU
	CGta4DbgSpuInfoStruct()	{	Invalidate(); 	}
#endif
	void Invalidate()
	{
		payload				= 0;
		gta4ModelInfoIdx	= u16(-1);
	}
};
CompileTimeAssert(sizeof(CGta4DbgSpuInfoStruct)==4);

struct spuGcmStateBaseEdge {
	EdgeGeomViewportInfo EdgeInfo;				// 112 (7 bytes of slop inside here we could borrow if we were evil)
	EdgeGeomLocalToWorldMatrix EdgeWorld;		// 48

	// TODO: at most one of edgeTintPalettePtr, damageTexture, or clothMorphData can be nonzero, and that address
	// always gets placed into the cached dma field ExtraData, so we really only need 2 bits somewhere to discriminate
	// between these three possible owners.

	// Quadword
	void* EdgeOccluderQuads;
	void *edgeTintPalettePtr;
	u8 EdgeClipPlaneEnable;	// 0x00=none, 0xff is extended support for up to EDGE_NUM_MODEL_CLIP_PLANES, else first four clip planes are active
	u8 DipSwitches;
	u16 EdgeCullDebugFlags; // should only be used with ENABLE_EDGE_CULL_DEBUGGING
	u32 __u32_pad00;

	// Quadword
	CellGcmControl *Control;	// pointer to gpu control register (put, get and ref, in that order) (this is static and could be obtained from a PPU variable)
	u8 EdgeViewportCullEnable:1, EdgeCullMode:2, BlendShapeDrawBuffer:2, IsVehicleGeom:1, __pad_bits00:2;
	u8 clothNumVerts;
#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
	u8 ModelCullStatus;
#else
	u8 __u8_pad01;
#endif
	u8 shadowType; // shadowType is 'eEdgeShadowType'
	CGta4DbgSpuInfoStruct gSpuGta4DebugInfo;
	void *statsBuffer;

	// Quadword
	const void* damageTexture;
	float damageBoundRadius;
	u8 EdgeOccluderQuadCount; u8 edgeTintFlags; u16 __u16_pad02; /* CullMode could be 2 bits, and EdgeViewportCullEnable eliminated, if we did translation ahead of time */
	void *clothMorphData;

	float damageTextureOffset[3];
	u32 __u32_pad01;

#if ENABLE_EDGE_CULL_CLIPPED_TRIANGLES
	// Must be last field in struct.  When drawablespu copies the struct to the
	// CellSpursEdgeJob, it will skip these planes if they have not been enabled
	// in EdgeClipPlaneEnable.  SPURS job streamer will still dma "something"
	// in, but it will be garbage.  This saves some space in the RSX segment.
	__vector4 EdgeClipPlanesTransposed[EDGE_NUM_TRIANGLE_CLIP_PLANES];
#endif // ENABLE_EDGE_CULL_CLIPPED_TRIANGLES

} ;		// implicit by virtue of containing vectors.

CompileTimeAssert(sizeof(spuGcmStateBaseEdge)==272 || EDGE_STRUCT_SIZE_NO_ASSERT); // was 384

__forceinline bool spuGcmShadowWarpEnabled(u8 shadowType)
{
	return shadowType == EDGE_SHADOWTYPE_PARABOLOID_SHADOWS;
}

#ifndef MINIMAL_SPU_GCM_STATE

struct gpuMemCpy {
	static const u32 MaxCopy = 128*2;
	struct Job { u32 DestOffset:31, DestLoc:1, SrcOffset:31, SrcLoc:1, Bytes; u32 DoneOffset; } ;

#if __PPU
	gpuMemCpy();
	bool Push(void *dest,const void *src,size_t bytes,u32 &setToNonZeroWhenDone);
	bool IsEmpty() const;
	void IncPendingDrawablespuJobs();
#else
	// Pop a job off the fifo if there is any, else decrement the pending
	// drawablespu jobs counter.  Returns true iff a job was grabbed.
	//
	// Also grabs a copy of m_doneSrcOffset (if returing true).  This is
	// unrelated to popping a job off the fifo, but it can be done for free at
	// the same time.
	//
	static bool Pop(u32 thisEa, Job *job, u32 *doneSrcOffset, u32 mfcTag);
#endif

	union {
		u64     m_atomic64;
		struct {
			u16     m_putIdx;   // next index writen to by ppu
			u16     m_getIdx;   // next index read from by spu
			u16     m_pending;  // number of pending drawablespujJobs
			u16     m_reserved;
		};
	};

	u32 m_doneSrcOffset;

	Job m_buffer[MaxCopy];

#if __PPU
	// This critical section is only to synchronize between ppu threads,
	// ensuring only one is Pushing a new job at the same time.  The
	// synchronization between the ppu and spu is lockfree.
	sysCriticalSectionToken     m_ppuPushCritSec;
#endif

} ALIGNED(128);

struct spuGcmSamplerState
{
	u32 offset;         // 0x1a00 + texUnit*0x20
	u32 format;         // 0x1a04 + texUnit*0x20
	u32 address;        // 0x1a08 + texUnit*0x20
	u32 control0;       // 0x1a0c + texUnit*0x20
	u32 control1;       // 0x1a10 + texUnit*0x20
	u32 filter;         // 0x1a14 + texUnit*0x20
	u32 imageRect;      // 0x1a18 + texUnit*0x20
	u32 borderColor;    // 0x1a1c + texUnit*0x20

	u32 control2;       // 0x0b00 + texUnit*4
	u32 control3;       // 0x1840 + texUnit*4
};

struct spuGcmStateBase: public spuGcmStateBaseEdge {
	grcGeometryJobs EdgeJobs;					// 128, but aligned

	spuMatrix44 World, View, Proj, FrustumLRTB, LocalLRTB;		// 64 each

	__vector4 EdgeClipPlanes[EDGE_NUM_MODEL_CLIP_PLANES];

	gcmRingBuffer FragmentFifo;			// 48 each
	gcmRingBuffer CommandFifo;

	// Keep these grouped by quadwords so we can tell if space is being wasted
	u32 LastMicrocodeOffset;	// address of last microcode download (allocated by SPU)
	void *NextSegment;
	void *LastFragment;
	u32 NextSegmentOffset;

	void *NextNextSegment;
	u32 CurrentFragmentOffset;
	void *CurrentVertexProgram;
	u32 BranchBits;

	void *forceShader;
	char* _binary_edgegeomspu_job_elf_start;
	char* _binary_edgegeomspu_job_elf_size;
	s8 ForcedTechniqueGroupId, LightingMode, FragmentCacheLocation /* CELL_GCM_LOCATION_LOCAL/MAIN */;
#if !__FINAL
	u8 CullerDebugFlags;
#else
	u8 __pad1[1];
#endif

	struct {
		u32 VertexFormats[16];

		// Depth/Stencil state block
		struct {
			u32 DepthTestEnable;
			u32 DepthWriteMask;
			u32 DepthFunc;
			u32 StencilTestEnable;
			u32 StencilTwoSidedEnable;
			u32 StencilWriteMask;
			u32 BackStencilWriteMask;
			u32 StencilFail;
			u32 StencilDepthFail;
			u32 StencilPass;
			u32 StencilFunc;
			u32 StencilRef;
			u32 StencilCmpMask;
			u32 BackStencilFail;
			u32 BackStencilDepthFail;
			u32 BackStencilPass;
			u32 BackStencilFunc;
			u32 BackStencilRef;
			u32 BackStencilCmpMask;

		} DepthStencil;

		// Blend state block
		struct {
			u32 BlendEnable;
			u32 BlendEnableMrt;
			u32 AlphaTestEnable;
			u32 AlphaFunc;
			u32 AlphaRef;
			u32 BlendColor;
			u32 BlendColor2;
			u32 BlendEquation;
			u32 BlendSrcFactor;
			u32 BlendDstFactor;
			u32 AntiAliasingControl;
			u32 ColorMask;
			u32 ColorMaskMrt;

		} Blend;

		spuGcmSamplerState SamplerState[16];

	} CachedStates;

	u32 JobTime, FifoStallTime, FragmentStallTime;
	u16 RenderStateBlockMask, FatTexturesBound;

	u32 DepthStencilStateStore;
	u32 RasterizerStateStore;
	u32 BlendStateStore;
	u32 SamplerStateStore;

	u32 DefaultRenderStates_unused_available;
	u32 FragmentConstants;
	u8 RS_Default, DSS_Default, BS_Default, __pad1_5;
	u16 VertexShaderInputs;
	u8 SurfaceColorFormat, SurfaceDepthFormat, SurfaceDepthFormatType;
	u8 __pad2:4;
	u8 wireframeOverride:1;
	u8 FlashEnable:1;
	u8 CullerAABB:1;	// 0=none, 1=default
#if DEBUG_ALLOW_CACHED_STATES_TOGGLE
	u8 DebugSetAllStates:1;
#endif

	u32 Globals;
	u32 GlobalsHeap;
	//u32 TrackStart, 
	//	TrackEnd;

	u32 //KnownRefToken,
		//KnownRefPool,
		GpuMemCpyAddr;
	grcBlendStateHandle bs_Active, bs_Previous;
	grcDepthStencilStateHandle dss_Active, dss_Previous;
	grcRasterizerStateHandle rs_Active, rs_Previous;
	u8 activeStencilRef, previousStencilRef;
	u8 activeAlphaRef, previousAlphaRef;
	u32 activeBlendFactors, previousBlendFactors;
	u32 activeSampleMask, previousSampleMask;
	u8 bs_Flushed, dss_Flushed, rs_Flushed;
	u8 flushedAlphaRef, flushedStencilRef, stateblockDirty;
	u32 flushedBlendFactors, flushedSampleMask;

#if USE_PACKED_GCMTEX
	PackedCellGcmTexture *PackedTextureArray;
#endif

#if !__FINAL
	// u32	TrackArray;
	spuDrawable* CurrentDrawable;
	spuGeometry* CurrentGeometry;
	u8 FaultNext, FaultIndex;
	bool isCapturingStats;
	u32 DmaTime;
	enum { FaultStackSize=8, FaultContextSize=144 };
	u8 FaultStack[FaultStackSize];
	char FaultContext[FaultContextSize];
#endif // !__FINAL
};

#include "grcore/grcorespu_gamestate.h"

#endif

#define InvalidateSpuGcmState(FIELD, VALUE)                                    \
	do {                                                                       \
		uint32_t size   = sizeof(((spuGcmStateBase*)0)->FIELD);                \
		uint32_t offset = OffsetOf(spuGcmStateBase, FIELD);                    \
		do {                                                                   \
			uint32_t count = Min(size, 256u);                                  \
			SPU_COMMAND(grcDevice__WriteStateStruct, 0);                       \
			cmd->offset = (u16)offset;                                         \
			cmd->count  = (u8)(count - 1);                                     \
			cmd->value  = (VALUE);                                             \
			size   -= count;                                                   \
			offset += count;                                                   \
		} while (Unlikely(size));                                              \
	}                                                                          \
	while (0)

__forceinline void ALIGN_CONTEXT_TO_16(CellGcmContextData *data) {
	vec_uint4 nops = vec_splat_u32( 0 );
	uint32_t * current = data->current;
	vec_stvlx( nops, 0, current );
	// round the pointer up to next multiple of 16. Or leave it unchanged if we were already aligned.
	data->current = (uint32_t*)( ( (uintptr_t) current + 15 ) & ~15 );
}

#if !__SPU

#if __DEV
// This may trigger a segment flush.
spuCmd_Any* spuAllocateCommand(CellGcmContextData *ctxt,u8 command,u8 subcommand,u32 totalSize);
#else
__forceinline uint32_t* spuAllocateCommand(CellGcmContextData *data,u8 command,u8 subcommand,u32 totalSize)
{
	Assert((totalSize & 3) == 0);

	// Make sure there's enough room including any potential alignment slop
	if ((char*)data->current + totalSize + 12 > (char*)data->end)
		data->callback(data, totalSize>>2);
	// If command is at least two quadwords, make sure it's aligned.
	// (if we had to invoke the callback we know it will already be suitably aligned)
	else if (totalSize > 31)
		ALIGN_CONTEXT_TO_16(data);

	uint32_t *result = data->current;
	data->current += (totalSize>>2);

	*result = (totalSize << 16) | (subcommand << 8) | (command<<1) | 1;
	return result;
}
#endif

template <class T> __forceinline T* spuAllocate(CellGcmContextData *ctxt,u8 command,u8 subcommand = 0,u32 extraData = 0) {
	return (T*) spuAllocateCommand(ctxt,command,subcommand,sizeof(T) + extraData);
};

struct grcJobParameters {
	size_t InputSize;
	void *InputData;
	size_t ScratchSize;			// must be multiple of 16
	size_t SpuStackSize, 
		UserDataCount;
	union {
		float asFloat;
		int asInt;
		void *asPtr;
		unsigned int asUInt;
		bool asBool;
	} UserData[20];
};

__forceinline void grcCreateGraphicsJob(const char * /*jobName*/,const char *jobStart,const char *jobSize,const grcJobParameters &params) 
{
	// We don't support separate output, or read-only data.
	Assert(params.InputSize<=16384U);
	Assert(params.UserDataCount<=20);
	spuCmd_grcDevice__CreateGraphicsJob* cmd = spuAllocate< spuCmd_grcDevice__CreateGraphicsJob>(GCM_CONTEXT, grcDevice__CreateGraphicsJob,params.UserDataCount,params.UserDataCount * sizeof(void*));
	cmd->JobStart = jobStart;
	cmd->JobSize = jobSize;
	cmd->InputSize = params.InputSize;
	cmd->InputData = params.InputData;
	cmd->ScratchSize = params.ScratchSize;
	cmd->SpuStackSize = params.SpuStackSize;
	memcpy(cmd->asInt,&params.UserData[0].asInt,params.UserDataCount * sizeof(void*));
}

#define SPU_COMMAND(x,args...)			::rage::spuCmd_##x *cmd = ::rage::spuAllocate< ::rage::spuCmd_##x>(GCM_CONTEXT,::rage::x,args)
#define SPU_SIMPLE_COMMAND(x,args...)	::rage::spuAllocate< ::rage::spuCmd_Any>(GCM_CONTEXT,::rage::x,args)

#else

#define BEGIN_SPU_COMMAND(x)	case ::rage::x: { ::rage::spuCmd_##x *cmd = static_cast< ::rage::spuCmd_##x*>(any);
#define END_SPU_COMMAND			break; }

#define BEGIN_SPU_SIMPLE_COMMAND(x)		case ::rage::x: {
#define END_SPU_SIMPLE_COMMAND			break; }
#endif


#if __SPU && !defined(MINIMAL_SPU_GCM_STATE)

enum { DISABLED_VERTEX_FORMAT = CELL_GCM_METHOD_DATA_VERTEX_DATA_ARRAY_FORMAT(0, 0, 0, CELL_GCM_VERTEX_F) };

u32 *GenerateConsecutiveRegistersCmdBuf(u32 *cmd, const u32 *currState, const u32 *wantedState, u32 firstReg, u32 num DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(, const spuGcmStateBase *gcmState))
{
#if DEBUG_ALLOW_CACHED_STATES_TOGGLE
	if (Unlikely(gcmState->DebugSetAllStates))
	{
		cmd[0] = CELL_GCM_METHOD(firstReg, num);
		memcpy(cmd+1, wantedState, num*4);
		return cmd+1+num;
	}
#endif

// Original "reference" implementation
#if 0

	// Generate the minimal rsx command buffer for setting the changed
	// registers.  Merge together consecutive register writes into a single
	// command buffer method.
	u32 *methodHeader = NULL;
	bool needMethodHeader = true;
	unsigned methodCount = 0;
	for (unsigned i=0; i<num; ++i)
	{
		if (currState[i] != wantedState[i])
		{
			if (needMethodHeader)
			{
				methodHeader = cmd++;
				*methodHeader = CELL_GCM_METHOD(firstReg+i*4, 0);
				needMethodHeader = false;
			}
			*cmd++ = wantedState[i];
			++methodCount;
		}
		else if (methodCount)
		{
			*methodHeader += methodCount << 18;
			methodCount = 0;
			needMethodHeader = true;
		}
	}
	*methodHeader += methodCount << 18;
	return cmd;

// Optimized version
#else

	Assert(num);
	Assert(((uptr)currState&3)==0);
	Assert(((uptr)wantedState&3)==0);
	qword headerPtr = si_from_ptr(cmd);
	qword dataPtr = si_ai(headerPtr, 4);
	qword headerWriteQuad = si_lqd(headerPtr, 0);
	qword dataWriteQuad = headerWriteQuad;
	qword maskF000 = si_fsmbi(0xf000);
	qword headerMask = si_rotqby(maskF000, si_sfi(headerPtr, 0));
	qword dataInHeaderQuadMask = si_rotqmbyi(headerMask, -4);
	qword dataInDataQuadMask   = si_rotqbyi (headerMask, -4);
	qword shufAAAA = si_ila(0x10203);
	qword headerSplat = si_shufb(si_from_uint(firstReg), si_from_uint(firstReg), shufAAAA);
	qword nextReg = headerSplat;
	qword currPtr = si_from_ptr(currState);
	qword wantPtr = si_from_ptr(wantedState);
	qword currMask = si_rotqby(maskF000, si_sfi(currPtr, 0));
	qword shufDDDDDDDDDDDDDDDD = si_ilh(0x0303);
	qword shufSplatCurr = si_a(shufAAAA, si_shufb(currPtr, currPtr, shufDDDDDDDDDDDDDDDD));
	qword shufSplatWant = si_a(shufAAAA, si_shufb(wantPtr, wantPtr, shufDDDDDDDDDDDDDDDD));
	qword prevSame = si_il(-1);
	qword fourByte = si_ilh(0x0404);
	do
	{
		// Load and splat the current and wanted register values
		qword curr = si_lqd(currPtr, 0);
		qword want = si_lqd(wantPtr, 0);
		shufSplatCurr = si_andbi(shufSplatCurr, 15);
		shufSplatWant = si_andbi(shufSplatWant, 15);
		qword currSplat = si_shufb(curr, curr, shufSplatCurr);
		qword wantSplat = si_shufb(want, want, shufSplatWant);
		shufSplatCurr = si_a(shufSplatCurr, fourByte);
		shufSplatWant = si_a(shufSplatWant, fourByte);

		// Update the saved current value
		curr = si_selb(curr, wantSplat, currMask);
		si_stqd(curr, currPtr, 0);
		currMask = si_rotqbyi(currMask, -4);
		currPtr = si_ai(currPtr, 4);
		wantPtr = si_ai(wantPtr, 4);

		// Check if the RSX already has the wanted register value set
		qword same = si_ceq(currSplat, wantSplat);

		// Update and write the data quad first, as it may get overwritten with the header quad
		dataWriteQuad = si_selb(dataWriteQuad, wantSplat, dataInDataQuadMask);
		si_stqd(dataWriteQuad, dataPtr, 0);

		// Update and write the header quad
		headerSplat = si_a(headerSplat, si_ilhu(0x0004));
		headerWriteQuad = si_selb(headerWriteQuad, headerSplat, si_andc(headerMask, same));
		headerWriteQuad = si_selb(headerWriteQuad, wantSplat, dataInHeaderQuadMask);
		si_stqd(headerWriteQuad, headerPtr, 0);

		// Select the appropriate method header for the next loop
		nextReg = si_ai(nextReg, 4);
		headerSplat = si_selb(headerSplat, nextReg, same);

		// Update the write pointers for the next loop
		headerPtr = si_selb(headerPtr, dataPtr, si_andc(same, prevSame));
		dataPtr = si_a(dataPtr, si_andi(si_nand(same, prevSame), 4));
		prevSame = same;

		// Reload the header quad in case the pointer was updated.  Data quad
		// doesn't need to be reloaded.
		headerWriteQuad = si_lqd(headerPtr, 0);

		// Update the masks
		headerMask = si_rotqby(maskF000, si_sfi(headerPtr, 0));
		dataInDataQuadMask = si_rotqby(maskF000, si_sfi(dataPtr, 0));
		qword dataInHeaderQuadMaskIfDiff = si_rotqmbyi(dataInHeaderQuadMask, -4);
		qword dataInHeaderQuadMaskIfSame = si_rotqmbyi(headerMask, -4);
		dataInHeaderQuadMask = si_selb(dataInHeaderQuadMaskIfDiff, dataInHeaderQuadMaskIfSame, same);
	}
	while(--num);
	return (u32*)si_to_ptr(si_selb(dataPtr, headerPtr, prevSame));

#endif
}

void GenerateVertexFormatCmdBuf(CellGcmContextData *gcmCtx, spuGcmStateBase *gcmState, const u32 *formats, u32 additionalCmdBufWords)
{
	Assert(((uptr)formats&15) == 0);

	// Reserve worst case number of command buffer words that can be required
	// (ie, every vertex format has changed).
	const u32 maxNumWords = 17+additionalCmdBufWords;
	u32 *cmd = EnsureMethodSpaceWords(gcmCtx, maxNumWords);

	// Generate the minimal rsx command buffer for setting the changed vertex
	// data array formats.  Merge together consecutive register writes into a
	// single command buffer method.
	cmd = GenerateConsecutiveRegistersCmdBuf(cmd, gcmState->CachedStates.VertexFormats, formats, CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 16
		DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(, gcmState));

	// memcpy(gcmState->vertexFormats, formats, 64);
	CompileTimeAssert((OffsetOf(spuGcmStateBase, CachedStates.VertexFormats)&15) == 0);
	const qword src = si_from_ptr(formats);
	const qword format0 = si_lqd(src, 0);
	const qword format1 = si_lqd(src, 16);
	const qword format2 = si_lqd(src, 32);
	const qword format3 = si_lqd(src, 48);
	const qword dst = si_from_ptr(gcmState->CachedStates.VertexFormats);
	si_stqd(format0, dst, 0);
	si_stqd(format1, dst, 16);
	si_stqd(format2, dst, 32);
	si_stqd(format3, dst, 48);

	gcmCtx->current = cmd;
}

// Note that the elements of 'offsets' must already be proper GCM offsets, and the high bit
// must already be set if it's in main memory.  The host manages that for us.
template<class FMT_CMD_BUF_FUNCTOR>
void BindVertexDeclarationImp(CellGcmContextData *gcmCtx, const spuVertexDeclaration *vd, const u32 *offsets, u16 vertexInputs, const FMT_CMD_BUF_FUNCTOR &fmtCmdBufFunctor)
{
	// This is cryptic as hell, so let's explain it.  We want to map 0x01->0x08, 0x02->0x0A,
	// etc, because si_fsm forms its select mask in left-to-right order but we want the MSB
	// to refer to an array element at offset 3, not 0.  si_rotqby rotates its input left
	// by the specified number of bytes, but only pays attention to the four rightmost bits
	// in the perferred word slot.  The final result needs to be in the correct place for fsm
	// to find it as well, which is why the table is pre-rotated by three bytes.
	const qword bitrevConst = MAKE_QWORD_u8(11,7,15,0,8,4,12,2,10,6,14,1,9,5,13,3);

	// For each i of the sixteen LSB's in vertexInputs, select from FormatU[i] if the bit
	// is set, or use a word from "disabled" if it's clear.
	// TODO: Would the si_fsm stuff be done more simply with a 16-element qword LUT?
	const qword disabled = si_il(DISABLED_VERTEX_FORMAT);
	qword format0123 = si_selb(disabled,vd->FormatV[0],si_fsm(si_rotqby(bitrevConst,si_from_uint(vertexInputs))));
	qword format4567 = si_selb(disabled,vd->FormatV[1],si_fsm(si_rotqby(bitrevConst,si_from_uint(vertexInputs>>4))));
	qword format89AB = si_selb(disabled,vd->FormatV[2],si_fsm(si_rotqby(bitrevConst,si_from_uint(vertexInputs>>8))));
	qword formatCDEF = si_selb(disabled,vd->FormatV[3],si_fsm(si_rotqby(bitrevConst,si_from_uint(vertexInputs>>12))));


	// Generate the vertex array format commands
	qword* q = fmtCmdBufFunctor(gcmCtx, format0123, format4567, format89AB, formatCDEF);

	const qword byteToWord0    =   MAKE_SHUFFLE(00,00,00,A0,00,00,00,A1,00,00,00,A2,00,00,00,A3);
	const qword byteToWord1    = /*MAKE_SHUFFLE(00,00,00,A4,00,00,00,A5,00,00,00,A6,00,00,00,A7);*/ si_ai(byteToWord0, 4);
	const qword byteToWord2    = /*MAKE_SHUFFLE(00,00,00,A8,00,00,00,A9,00,00,00,AA,00,00,00,AB);*/ si_ai(byteToWord0, 8);
	const qword byteToWord3    = /*MAKE_SHUFFLE(00,00,00,AC,00,00,00,AD,00,00,00,AE,00,00,00,AF);*/ si_ai(byteToWord0, 12);
	const qword replicateWord0 =   MAKE_SHUFFLE(A0,A0,A0,A0,A1,A1,A1,A1,A2,A2,A2,A2,A3,A3,A3,A3);
	const qword replicateWord1 = /*MAKE_SHUFFLE(A4,A4,A4,A4,A5,A5,A5,A5,A6,A6,A6,A6,A7,A7,A7,A7);*/ si_orbi(replicateWord0, 4);
	const qword replicateWord2 = /*MAKE_SHUFFLE(A8,A8,A8,A8,A9,A9,A9,A9,AA,AA,AA,AA,AB,AB,AB,AB);*/ si_orbi(replicateWord0, 8);
	const qword replicateWord3 = /*MAKE_SHUFFLE(AC,AC,AC,AC,AD,AD,AD,AD,AE,AE,AE,AE,AF,AF,AF,AF);*/ si_orbi(replicateWord0, 12);
	const qword insertWord2    =   MAKE_SHUFFLE(B0,B1,B2,B3,B4,B5,B6,B7,A8,A9,AA,AB,BC,BD,BE,BF);

	const qword divider_and_offset = MAKE_QWORD_u32(
		0,
		0x00040000 | CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION,
		0,	// stream frequency divider needs to live at modulo 8.
		0x00400000 | CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET );

	// Load up to four offset pointers
	qword offsetsV = si_lqd(si_from_ptr(offsets), 0x0);
	qword OffsetV = vd->OffsetV;	// one byte per stream (byte offset)
	qword StreamV = si_shlqbii(vd->StreamV,2);	// one byte per stream (stream index) (mult by four)

	// compute the four shuffles that extract the correct offset based on stream for
	// each of the sixteen channels.  To do this, we need to turn 0x00->0x00010203,
	// 0x01->04050607, 0x02->08090A0B, 0x03->0C0D0E0F.  Note that if we shift the
	// entire stream array left twice, we'll turn 0x01->0x04, 0x02->0x08, 0x03->0x0C,
	// and then if we replicate that result to the entire word, we'll get 0x00000000,
	// 0x04040404, 0x08080808, or 0x0C0C0C0C.  Then if we just add the constant
	// 0x00010203 across all four fields, we'll get the select we're looking for
	const qword bias = si_ila(0x10203);
	qword streamShuffle0123 = si_a(si_shufb(StreamV,StreamV,replicateWord0),bias);
	qword streamShuffle4567 = si_a(si_shufb(StreamV,StreamV,replicateWord1),bias);
	qword streamShuffle89AB = si_a(si_shufb(StreamV,StreamV,replicateWord2),bias);
	qword streamShuffleCDEF = si_a(si_shufb(StreamV,StreamV,replicateWord3),bias);

	// Compute "offsets[stream] + vd->Offset[i];"
	// First shuffle on each line computes offsets[stream], second shuffle computes vd->Offset[i]);
	qword offset0123 = si_a(si_shufb(offsetsV,offsetsV,streamShuffle0123),si_shufb(OffsetV,OffsetV,byteToWord0));
	qword offset4567 = si_a(si_shufb(offsetsV,offsetsV,streamShuffle4567),si_shufb(OffsetV,OffsetV,byteToWord1));
	qword offset89AB = si_a(si_shufb(offsetsV,offsetsV,streamShuffle89AB),si_shufb(OffsetV,OffsetV,byteToWord2));
	qword offsetCDEF = si_a(si_shufb(offsetsV,offsetsV,streamShuffleCDEF),si_shufb(OffsetV,OffsetV,byteToWord3));

	q[0] = si_shufb(si_lqd(si_from_ptr(vd),offsetof(spuVertexDeclaration,Stream0Size)),divider_and_offset,insertWord2);
	q[1] = offset0123;
	q[2] = offset4567;
	q[3] = offset89AB;
	q[4] = offsetCDEF;

	gcmCtx->current = (u32*)(q+5);
}

struct ChangesOnlyFmtCmdBufFunctor
{
	spuGcmStateBase *gcmState;
	u32              additionalCmdBufWords;

	ChangesOnlyFmtCmdBufFunctor(spuGcmStateBase *gcmState_, u32 additionalCmdBufWords_)
		: gcmState(gcmState_)
		, additionalCmdBufWords(additionalCmdBufWords_)
	{
	}

	inline qword *operator()(CellGcmContextData *gcmCtx, qword format0123, qword format4567, qword format89AB, qword formatCDEF) const
	{
		u32 formats[16] ;
		si_stqd(format0123, si_from_ptr(formats), 0);
		si_stqd(format4567, si_from_ptr(formats), 16);
		si_stqd(format89AB, si_from_ptr(formats), 32);
		si_stqd(formatCDEF, si_from_ptr(formats), 48);
		GenerateVertexFormatCmdBuf(gcmCtx, gcmState, formats, additionalCmdBufWords);
		return MethodNopToQwordAlignment((qword*)gcmCtx->current);
	}
};

void BindVertexDeclaration(CellGcmContextData *gcmCtx, spuGcmStateBase *gcmState, const spuVertexDeclaration *vd, const u32 *offsets, u16 vertexInputs)
{
	const u32 additionalCmdBufWords = 3 + 20; // alignment + 20 commands words for the offsets and frequency divider
	BindVertexDeclarationImp(gcmCtx, vd, offsets, vertexInputs, ChangesOnlyFmtCmdBufFunctor(gcmState, additionalCmdBufWords));
}

struct AllFmtCmdBufFunctor
{
	inline qword *operator()(CellGcmContextData *gcmCtx, qword format0123, qword format4567, qword format89AB, qword formatCDEF) const
	{
		qword *q = ReserveMethodSizeAligned(gcmCtx, 10);
		const qword header = MAKE_QWORD_u32(
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_HEADER_VERTEX_DATA_ARRAY_FORMAT(0, 16));
		q[0] = header;
		q[1] = format0123;
		q[2] = format4567;
		q[3] = format89AB;
		q[4] = formatCDEF;
		return q+5;
	}
};

void ForceBindVertexDeclaration(CellGcmContextData *gcmCtx, const spuVertexDeclaration *vd, const u32 *offsets, u16 vertexInputs)
{
	BindVertexDeclarationImp(gcmCtx, vd, offsets, vertexInputs, AllFmtCmdBufFunctor());
}

#endif // __SPU


} // namespace rage

#endif	// __PS3

#endif // GRCORE_GRCORESPU_H 
