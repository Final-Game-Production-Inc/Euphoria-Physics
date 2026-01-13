//
// grcore/fastquad.cpp
//
// Copyright (C) 2012 Rockstar Games.  All Rights Reserved.
//

#include "fastquad.h"
#include "device.h"

#if FAST_QUAD_SUPPORT
//--------------------------------------------------------------------------//
//		Quad rendering section: shared between DX9 and DX11					//

#include "math/float16.h"
#include "vertexbuffer.h"


namespace rage
{

namespace quad
{
	static const bool s_bAllowLargeTriangle = true;
	static const grcVertexBuffer *s_MinimalBuffer = NULL;
	static grcVertexDeclaration *s_MinimalDecl = NULL;
	static const grcFvf::grcDataSize format = grcFvf::grcdsFloat2;

	typedef Vector2 VertexType;
	static VertexType makeVertex(float x, float y)
	{
		//const Float16Vec2 out = { Float16(x), Float16(y) };
		//return out;
		return Vector2(x,y);
	}

}


void FastQuad::Init()
{
	if (quad::s_MinimalBuffer != NULL)
	{
		Warningf("Attempting to initialize FastQuad::Init twice");
		return;
	}

	const grcVertexElement minimalElement(0, grcVertexElement::grcvetPosition, 0, sizeof(quad::VertexType), quad::format);
	quad::s_MinimalDecl = GRCDEVICE.CreateVertexDeclaration(&minimalElement, 1, 0);

	grcFvf oFormat;
	oFormat.SetPosChannel(true, quad::format);
	Assert( oFormat.GetTotalSize() == sizeof(quad::VertexType) );

	ALIGNAS(16) quad::VertexType vertices[] =
	{
		quad::makeVertex(1.f,1.f),	//quad[0]
		quad::makeVertex(1.f,0.f),	//quad[1]
		quad::makeVertex(0.f,1.f),	//quad[2]
		quad::makeVertex(0.f,0.f),	//quad[3] = triangle[0]
		quad::makeVertex(0.f,2.f),	//triangle[1]
		quad::makeVertex(2.f,0.f),	//triangle[2]
		quad::makeVertex(-1.f,-1.f),	//dummy
	};

	Assert( !quad::s_MinimalBuffer );
	const int nVert = sizeof(vertices)/sizeof(quad::VertexType);
	quad::s_MinimalBuffer = 
#if RSG_PC && __D3D11
	grcVertexBuffer::CreateWithData( nVert, oFormat, grcsBufferCreate_NeitherReadNorWrite, vertices );
#elif RSG_DURANGO || RSG_ORBIS
	grcVertexBuffer::CreateWithData( nVert, oFormat, vertices );
#else
	grcVertexBuffer::Create( nVert, oFormat, false, false, vertices );
#endif
}

void FastQuad::Shutdown()
{
	if (quad::s_MinimalDecl != NULL)
	{
		GRCDEVICE.DestroyVertexDeclaration(quad::s_MinimalDecl);
		quad::s_MinimalDecl = NULL;
	}
	if (quad::s_MinimalBuffer != NULL)
	{
		delete quad::s_MinimalBuffer;
		quad::s_MinimalBuffer = NULL;
	}
}

void FastQuad::Draw(bool bFullScreen)
{
	GRCDEVICE.SetVertexDeclaration( quad::s_MinimalDecl );
	GRCDEVICE.SetStreamSource( 0, *quad::s_MinimalBuffer, 0, sizeof(quad::VertexType) );

	if (bFullScreen && quad::s_bAllowLargeTriangle)
	{
		//Assert( GetRenderState(D3DRS_CLIPPING) );
		GRCDEVICE.DrawPrimitive( drawTriStrip, 3,3 );
	}else
	{
		// A quad doesn't need HW clipping, but the state is not supported
		//SetRenderState(D3DRS_CLIPPING, false);
		GRCDEVICE.DrawPrimitive( drawTriStrip, 0,4 );
		//SetRenderState(D3DRS_CLIPPING, true);
	}
}

}	//rage
#endif	//FAST_QUAD_SUPPORT

#if DEVICE_EQAA
//--------------------------------------------------------------------------//
//		Resolve/Clear/Copy helper functions, shared between Durango & Orbis	//
#include "grmodel/shader.h"
#include "profile/timebars.h"
#if RSG_ORBIS
const unsigned threadsPerWaveFront = sce::Gnm::kThreadsPerWavefront;
#include "buffer_gnm.h"
#include "rendertarget_gnm.h"
#include "texture_gnm.h"
#include "texturefactory_gnm.h"
#elif RSG_DURANGO
const unsigned threadsPerWaveFront = 64;
#include "rendertarget_durango.h"
#include "texture_durango.h"
#endif

namespace rage
{
namespace eqaa
{

#if AA_SAMPLE_DEBUG
Debug::Debug()
: useDeprecatedPattern(false)
, patternRadius(0.9f)
, patternOffset(15.f)
, resolveSideWeight(0.225f)
, resolveDepthThreshold(0.f)
, resolveSkipEdges(true)
{}
static Debug s_Debug;
Debug& GetDebug() { return s_Debug; }
#endif //AA_SAMPLE_DEBUG
#if RSG_ORBIS
Control::Control()
: incoherentEqaaReads(true)
, staticAnchorAssociations(false)
, interpolateCompressedZ(false)
, highQualityTileIntersections(false)
{}
static Control s_Control;
Control& GetControl() { return s_Control; }
#endif //RSG_ORBIS

int		s_ResolveHighQuality = 0;	//-1 = disable, 0 = auto, 1 = force
bool	s_ResolveHighQualityS1 = false;	//force HQ resolve for S/1 modes
static const char *s_ShaderName = "x:\\gta5\\build\\dev_ng\\common\\shaders\\ClearCS";
static grmShader *s_Shader;
static grcEffectTechnique	s_ClearTechnique, s_ResolveTechnique;
static grcEffectVar s_ClearTargetVar, s_ClearSourceVar, s_ClearValueVar;
static grcEffectVar s_ResolveAAParamsVar, s_ResolveAATexture,
	s_ResolveFmaskTexture, s_ResolveDepthTexture;
#if AA_SAMPLE_DEBUG
static grcEffectVar s_ResolveAADebugParamsVar;
#endif //AA_SAMPLE_DEBUG
static grcDepthStencilStateHandle s_CompressDSHandle;
static grcVertexDeclaration *s_VertexDeclaration;
	
void SetEffectName(const char *effectName)
{
	s_ShaderName = effectName;
}

void Init()
{
	if (s_Shader)
		return;
	
	s_Shader = grmShaderFactory::GetInstance().Create();
	s_Shader->Load( s_ShaderName );
	if (!s_Shader)
		Quitf("Unable to create clear effect '%s', cannot continue.",s_ShaderName);

	s_ClearTechnique		= s_Shader->LookupTechnique("clear");
	s_ResolveTechnique		= s_Shader->LookupTechnique("resolve");
	s_ClearTargetVar		= s_Shader->LookupVar("FastClearTarget");
	s_ClearSourceVar		= s_Shader->LookupVar("FastClearSource");
	s_ClearValueVar			= s_Shader->LookupVar("gValue");
	s_ResolveAAParamsVar	= s_Shader->LookupVar("gTargetAAParams");
	s_ResolveAATexture		= s_Shader->LookupVar("AATexture");
	s_ResolveFmaskTexture	= s_Shader->LookupVar("FmaskTexture");
	s_ResolveDepthTexture	= s_Shader->LookupVar("DepthTexture", RSG_ORBIS);
#if AA_SAMPLE_DEBUG
	s_ResolveAADebugParamsVar	= s_Shader->LookupVar("gDebugResolveParams");
#endif //AA_SAMPLE_DEBUG

	grcDepthStencilStateDesc desc;
	desc.DepthFunc = grcRSV::CMP_ALWAYS;
	desc.DepthEnable = desc.StencilEnable = true;
	desc.DepthWriteMask = true;
	desc.StencilWriteMask = 0xFF;
	s_CompressDSHandle = grcStateBlock::CreateDepthStencilState( desc, "Compress depth stencil" );

	s_VertexDeclaration = GRCDEVICE.CreateVertexDeclaration(NULL,0);
}

const grcVertexDeclaration *GetVertexDeclaration()
{
	return s_VertexDeclaration;
}


void ClearBuffer(const char debugName[], void *address, unsigned size, const u32 value)
{
	const int numElems = size/sizeof(uint32_t);

#if RSG_ORBIS
	grcBufferGNM buffer( grcBuffer_Typed, false );
	buffer.Initialize( address, numElems, true, 4, sce::Gnm::kDataFormatR32Uint );
	s_Shader->SetVarUAV( s_ClearTargetVar, &buffer );
#else
	(void)address;
	Assert(!"ClearBuffer is not implemented for DX11");
#endif
	s_Shader->SetVar( s_ClearValueVar, static_cast<int>(value) );

	GRCDEVICE.RunComputation( debugName, *s_Shader, (int)pass::fast_clear_cs, (numElems-1)/threadsPerWaveFront+1, 1, 1 );
	
	GRCDEVICE.SynchronizeComputeToGraphics();
	s_Shader->SetVarUAV( s_ClearTargetVar, static_cast<grcBufferUAV*>(NULL) );
}

void CopyBuffer(const char debugName[], void *dest, void *source, unsigned size)
{
	const int numElems = size/sizeof(uint32_t);

#if RSG_ORBIS
	grcBufferGNM bufDest( grcBuffer_Typed, false );
	bufDest.Initialize( dest, numElems, true, 4,		sce::Gnm::kDataFormatR32Uint );
	grcBufferGNM bufSource( grcBuffer_Typed, false );
	bufSource.Initialize( source, numElems, false, 4,	sce::Gnm::kDataFormatR32Uint );

	s_Shader->SetVarUAV	( s_ClearTargetVar, &bufDest );
	s_Shader->SetVar	( s_ClearSourceVar, &bufSource );
#else
	(void)dest;
	(void)source;
	Assert(!"CopyBuffer is not implemented for DX11");
#endif

	GRCDEVICE.RunComputation( debugName, *s_Shader, (int)pass::fast_copy_cs, (numElems-1)/threadsPerWaveFront+1, 1, 1 );
	
	GRCDEVICE.SynchronizeComputeToGraphics();
	s_Shader->SetVarUAV	( s_ClearTargetVar, static_cast<grcBufferUAV*>(NULL) );
	s_Shader->SetVar	( s_ClearSourceVar, static_cast<grcBufferUAV*>(NULL) );
}


struct Rect	{
	bool active;
	u32 x,y,w,h;
}static DECLARE_MTR_THREAD s_Area = {false,0,0,0,0};

void ResolveArea::Apply()
{
	if (s_Area.active)
	{
		GRCDEVICE.SetScissor( s_Area.x, s_Area.y, s_Area.w, s_Area.h );
	}
}

ResolveArea::ResolveArea(u32 x, u32 y, u32 w, u32 h)
{
	grcAssertf(!s_Area.active, "ResolveArea has already been set");
	s_Area.active = true;
	s_Area.x = x;
	s_Area.y = y;
	s_Area.w = w;
	s_Area.h = h;
}

ResolveArea::~ResolveArea()
{
	grcAssertf(s_Area.active, "ResolveArea has not been set");
	s_Area.active = false;
	//GRCDEVICE.DisableScissor();	//should not be necessary
}


static void DrawQuadInternal(bool toDepth)
{
	grcRasterizerStateHandle RS_prev = grcStateBlock::RS_Active;
	grcDepthStencilStateHandle DSS_prev = grcStateBlock::DSS_Active;
	grcBlendStateHandle BS_prev = grcStateBlock::BS_Active;

	grcStateBlock::SetStates( grcStateBlock::RS_NoBackfaceCull, 
		toDepth ? s_CompressDSHandle : grcStateBlock::DSS_IgnoreDepth,
		grcStateBlock::BS_Default );

#if __D3D11
	GRCDEVICE.DrawPrimitive( drawTriStrip, s_VertexDeclaration, *static_cast<grcVertexBuffer*>(0), 0, 4 );
#else
	GRCDEVICE.DrawPrimitive( drawRects, s_VertexDeclaration, *static_cast<grcVertexBuffer*>(0), 0, 3 );
#endif

	grcStateBlock::SetStates( RS_prev, DSS_prev, BS_prev );
}


void DrawClear(pass::Clear pass)
{
	AssertVerify(s_Shader->BeginDraw(grmShader::RMC_DRAW, true, s_ClearTechnique));
	s_Shader->Bind( pass );

	DrawQuadInternal( false );
	
	s_Shader->UnBind();
	s_Shader->EndDraw();
}

void DrawResolve(pass::Resolve pass, bool toDepth)
{
	ResolveArea::Apply();

	AssertVerify(s_Shader->BeginDraw(grmShader::RMC_DRAW, true, s_ResolveTechnique));
	s_Shader->Bind( pass );

	DrawQuadInternal( toDepth );
	
	s_Shader->UnBind();
	s_Shader->EndDraw();
}

bool ResolveMeSoftly(grcRenderTarget *const destination, const grcRenderTarget *const source, const int destSliceIndex)
{
	const grcRenderTargetMSAA *const sourceAA = static_cast<const grcRenderTargetMSAA*>( source );
	const CoverageData &coverage = sourceAA->GetCoverageData();
	if (coverage.resolveType == ResolveHW)
		return false;

	PF_AUTO_PUSH_TIMEBAR("Resolving MSAA in a shader");
#if RSG_ORBIS
	GRCDEVICE.FlushCaches( grcDevice::CACHE_COLOR_META );
#endif

	const grcDevice::MSAAMode mode	= (coverage.donor ? coverage.donor : sourceAA)->GetMSAA();
	const grcTexture *const fmask	= (coverage.donor ? coverage.donor : sourceAA)->GetCoverageData().texture;
	const IntVector AAParams = { mode.m_uSamples, mode.m_uFragments, mode.GetFmaskShift(), 0 };

	s_Shader->SetVar( s_ResolveAAParamsVar, AAParams );
	s_Shader->SetVar( s_ResolveAATexture, source );
	s_Shader->SetVar( s_ResolveFmaskTexture, fmask );
#if AA_SAMPLE_DEBUG
	const Debug& ed = GetDebug();
	s_Shader->SetVar( s_ResolveAADebugParamsVar, Vector4(ed.resolveSideWeight, ed.resolveDepthThreshold, ed.resolveSkipEdges, 0.f)  );
#endif //AA_SAMPLE_DEBUG
	const u32 debugMask = 0;
	pass::Resolve pass = pass::resolve_msaa;

	if (fmask)
	{
		const unsigned param = (AAParams[0]<<4) + AAParams[1];
		if (debugMask)
		{
			pass = pass::debug_frag_eqaa;
		}
		else
		{
			pass = pass::resolve_eqaa;
			pass = (coverage.resolveType == ResolveSW_Simple_NanDetect) ? pass::resolve_eqaa_nandetect : pass;

			const bool resolveS1 = (AAParams[1]==1) && s_ResolveHighQualityS1;
			if ((coverage.resolveType != ResolveSW_Simple && coverage.resolveType != ResolveSW_Simple_NanDetect) || resolveS1)
			{
				const bool useHQ = s_ResolveHighQuality>0 || resolveS1 ||
					(s_ResolveHighQuality==0 && coverage.resolveType == ResolveSW_HighQuality);
				if (AAParams[0] == AAParams[1])
					pass = pass::resolve_msaa;
				else switch (useHQ * param)
				{
				case 0x41:
					pass = CENTERED ?
#if AA_SAMPLE_DEBUG
						(ed.useDeprecatedPattern ?
							pass::resolve_eqaa_quality_41_centered_old :
							pass::resolve_eqaa_quality_41_dithered) : 
#else
						pass::resolve_eqaa_quality_41_dithered : 
#endif //AA_SAMPLE_DEBUG
						pass::resolve_eqaa_quality_41;
					break;
				case 0x42:
					pass = pass::resolve_eqaa_quality_42;
					break;
				case 0x82:
					pass = pass::resolve_eqaa_quality_82;
					break;
				case 0x104:
					pass = pass::resolve_eqaa_performance_104;
					break;
				default:
					pass = pass::resolve_eqaa_extra;
				}
			}
		}
	}else
	{
		pass = debugMask ? pass::debug_mask_msaa	: pass::resolve_msaa;
	}

	// DX11 CopyResource requires DXGI_SAMPLE_DESC to be exactly the same
#if RSG_ORBIS
	if (AAParams[1]==1 && (pass==pass::resolve_msaa || pass==pass::resolve_eqaa_extra || pass==pass::resolve_eqaa))
	{
		grcAssertf(!destSliceIndex, "Destination slice index is not implemented for RT Copy");
		destination->Copy(source, 0);
	}else
#endif //RSG_ORBIS
	{
		grcTextureFactory::GetInstance().LockRenderTarget( 0, destination, NULL, destSliceIndex );
		DrawResolve( pass );
		grcTextureFactory::GetInstance().UnlockRenderTarget( 0 );
	}
	
	return true;
}


// Cmask dump code is adopted from here:
// https://ps4.scedev.net/support/issue/24629/_Misbehaving_kCbModeEliminateFastClear_with_4xMSAA_targets
// Htile dump code is ported from SDK's htile-sample

static inline uint32_t GetPipeIndexOfTile(uint32_t x, uint32_t y)
{
	u32 pipe = 0;
	pipe |= ( ((x>>0) ^ (y>>0) ^ (x>>1)) & 0x1 ) << 0;
	pipe |= ( ((x>>1) ^ (y>>1)) & 0x1 ) << 1;
	pipe |= ( ((x>>2) ^ (y>>2)) & 0x1 ) << 2;
	return pipe;
}

static u32 UntileCommon(u32 *outNybble, u32 cl_width, u32 cl_height, u32 cl_stride, int tile_shift, u32 tileX, u32 tileY, u32 tilesWide, bool isLinear)
{
	const u32 cl_x = tileX / cl_width;
	const u32 cl_y = tileY / cl_height;
	const u32 surf_pitch_cl = (tilesWide + cl_width - 1) / cl_width;
	const u32 cl_offset = isLinear ? 0 : ((cl_x + surf_pitch_cl * cl_y) * cl_stride);
	const u32 macro_x = (isLinear ? tileX : (tileX % cl_width)) / 4;
	const u32 macro_y = (isLinear ? tileY : (tileY % cl_height)) / 4;
	const u32 macro_pitch = (isLinear ? tilesWide : cl_width) / 4;
	const u32 macro_shift = 1;
	u32 macro_offset = (macro_x + macro_y * macro_pitch) << macro_shift;
	macro_offset &= ~3;
	macro_offset |= (( (tileX>>1) ^ (tileY>>0) )&1) << 0;
	macro_offset |= (( (tileX>>1)              )&1) << 1;
	const u32 tile_number = cl_offset + macro_offset;
	const u32 device_address = tile_shift>0 ? (tile_number<<tile_shift) : (tile_number >> -tile_shift);
	const u32 pipe = GetPipeIndexOfTile(tileX, tileY);
	const u32 pipe_interleave = 256;
	const u32 num_pipes = 8;
	const u32 final_address = (device_address % pipe_interleave) + (pipe * pipe_interleave) +
		(device_address / pipe_interleave) * pipe_interleave * num_pipes;
	if (outNybble)
		*outNybble = tile_number & 1;
	return final_address;
}

static void GetCmaskNibbleOffset(uint32_t *outOffset, uint32_t *outNybble, uint32_t tileX, uint32_t tileY, uint32_t tilesWide, bool isLinear)
{
	*outOffset = UntileCommon(outNybble, 64, 32, 256, -1, tileX, tileY, tilesWide, isLinear);
}

//public
u32 UntileCmask(void *const destination, const void *const rawCmask, unsigned width, unsigned height, bool isLinear, bool packTiles)
{
	PF_AUTO_PUSH_TIMEBAR("Un-tiling Cmask");
	const u32	tileSize = 8;
	const u32	tilesWide = width / tileSize;
	u32	cmaskOffset[2] = {0,0};
	u32	cmaskNibble[2] = {0,0};
	u8	cmaskData[2] = {0,0};
	u8*	pRover = static_cast<uint8_t*>(destination);
	for (unsigned y = 0; y < height; y += tileSize)
	{
		for (unsigned x = 0; x+tileSize < width; x += tileSize * 2)
		{
			u32	tileX = x / tileSize;
			u32	tileY = y / tileSize;
			GetCmaskNibbleOffset(&cmaskOffset[0], &cmaskNibble[0], tileX+0, tileY, tilesWide, isLinear);
			GetCmaskNibbleOffset(&cmaskOffset[1], &cmaskNibble[1], tileX+1, tileY, tilesWide, isLinear);
			cmaskData[0] = static_cast<const uint8_t*>(rawCmask)[cmaskOffset[0]];
			cmaskData[1] = static_cast<const uint8_t*>(rawCmask)[cmaskOffset[1]];
			if (cmaskNibble[0])
				cmaskData[0] >>= 4;
			else
				cmaskData[0] &= 0xF;
			if (cmaskNibble[1])
				cmaskData[1] >>= 4;
			else
				cmaskData[1] &= 0xF;
			if (packTiles)
			{
				*pRover++ = cmaskData[0] | (cmaskData[1] << 4);
			}else
			{
				// shifting for better visibility
				*pRover++ = cmaskData[0]<<4;
				*pRover++ = cmaskData[1]<<4;
			}
		}
	}

	return pRover - static_cast<u8*>(destination);
}

//public
u32 UntileHtile(void *const destination, const void *const rawHtile, unsigned width, unsigned height, bool isLinear, bool withStencil)
{
	const u32	tileSize = 8;
	const u32	tilesWide = width / tileSize;
	u8*	pRover = static_cast<u8*>(destination);
	
	for (unsigned y = 0; y < height/tileSize; ++y)
	{
		for (unsigned x = 0; x < width/tileSize; ++x)
		{
			const u32 address = UntileCommon(NULL, 64, 64, 512, 2, x, y, tilesWide, isLinear);
			const u32 value = static_cast<const u32*>(rawHtile)[address>>2];
			if (withStencil)
			{
				pRover[2] = (value>>4) & 0xFF;		// SR0, SR1, Smem, XX
				pRover[1] = (value>>24) & 0xFF;		// Z-base
				pRover[0] = (value>>10) & 0xFC;		// Z-delta
			}else
			{
				pRover[2] =	(value>>10) & 0xFF;		// Z-min
				pRover[1] = (value>>24) & 0xFF;		// Z-max
				pRover[0] = pRover[1] - pRover[2];	// Z-delta
			}
			pRover[3] = (value&0xF) << 4;	// Z-mask
			pRover += 4;
		}
	}

	return pRover - static_cast<u8*>(destination);
}

//public
void UntileFmask(grcRenderTarget *const destination, const grcTexture *const fmask, const grcDevice::MSAAMode &mode, unsigned sampleOffset)
{
	PF_AUTO_PUSH_TIMEBAR("Un-tiling Fmask");
	const IntVector AAParams = { mode.m_uSamples, mode.m_uFragments, mode.GetFmaskShift(), sampleOffset };

	s_Shader->SetVar( s_ResolveAAParamsVar, AAParams );
	s_Shader->SetVar( s_ResolveFmaskTexture, fmask );

#if RSG_ORBIS
	GRCDEVICE.LockSingleTarget( static_cast<grcRenderTargetMSAA*>(destination)->GetColorTarget() );
#else
	grcTextureFactory::GetInstance().LockRenderTarget( 0, destination, NULL );
#endif

	DrawResolve( pass::copy_fmask );

#if RSG_ORBIS
	static_cast<grcTextureFactoryGNM&>( grcTextureFactory::GetInstance() ).RelockRenderTargets();
#else
	grcTextureFactory::GetInstance().UnlockRenderTarget( 0 );
#endif

	s_Shader->SetVar( s_ResolveFmaskTexture, static_cast<grcTexture*>(NULL) );
}

}	// eqaa
}	// rage
#endif // DEVICE_EQAA