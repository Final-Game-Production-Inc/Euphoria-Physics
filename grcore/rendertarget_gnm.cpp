// 
// grcore/texturefactory_gnm.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 
#include "rendertarget_gnm.h"
#include "texturereference.h"

#if __GNM

#include "gfxcontext_gnm.h"
#include "image.h"
#include "file/asset.h"
#include "texturefactory_gnm.h"
#include "texture_gnm.h"
#include "grmodel/shader.h"
#include "quads.h"

#if SUPPORT_RENDERTARGET_DUMP
#include <file/asset.h>
#endif

#include "fastquad.h"
#include "image.h"
#include "texture_gnm.h"
#include "texturefactory_gnm.h"

namespace rage {

NOSTRIP_XPARAM(noFastClearColor);
NOSTRIP_XPARAM(noFastClearDepth);
XPARAM(noStencil);

#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
const sce::Gnm::NumFragments	SingleFragment = sce::Gnm::kNumFragments1;
#define NotACubeMap	, false
#else
const sce::Gnm::NumSamples		SingleFragment = sce::Gnm::kNumSamples1;
#define NotACubeMap
#endif //SCE_ORBIS_SDK_VERSION

bool s_CopyByCompute = true;
bool s_UseStencilCompression = false;	// this would slow us down by about 0.05ms

CoverageData::CoverageData()
: isMultisampled(false)
, texture(NULL)
, donor(NULL)
, resolveType(ResolveSW_Simple)	//required for our EQAA
, superSample(sce::Gnm::kNumSamples1)
, isCmaskDirty(false)
{}


static grmShader *s_MipMapShader = NULL;
static grcEffectTechnique s_MipMapTechnique = grcetNONE;
static grcEffectVar s_MipMapSourceVar = grcevNONE, s_MipMapParamsVar = grcevNONE;

void grcRenderTargetGNM::InitMipShader()
{
	s_MipMapShader = grmShaderFactory::GetInstance().Create();
	s_MipMapShader->Load( "im" );

	s_MipMapTechnique = s_MipMapShader->LookupTechnique("mipMap");
	s_MipMapSourceVar = s_MipMapShader->LookupVar("RenderTexArray");
	s_MipMapParamsVar = s_MipMapShader->LookupVar("RefMipBlurParams");
}

static const sce::Gnm::TextureChannelType stencilChannelType = sce::Gnm::kTextureChannelTypeUInt;
static const sce::Gnm::DataFormat kDataFormatX24G8Uint = {{{sce::Gnm::kSurfaceFormatX24_8_32, stencilChannelType, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX}}};
//static const sce::Gnm::DataFormat kDataFormatD32Float = {{{sce::Gnm::kSurfaceFormat32, sce::Gnm::kTextureChannelTypeFloat, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX}}};

extern void* allocateVideoPrivateMemory(const sce::Gnm::SizeAlign &sa);

grcRenderTargetGNM::grcRenderTargetGNM()
{
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif
	m_Target = NULL;
	m_DepthTarget = NULL;
	m_Owned = false;
	m_Name = NULL;
	m_LockedMip = 0;
}

grcRenderTargetGNM::grcRenderTargetGNM(const char *name, const grcRenderTargetGNM *pTexture, grcTextureFormat eShaderViewFormat /*= grctfNone*/)
{
	using namespace sce;

	m_Name = StringDuplicate(name);
	m_Owned = false;
	m_CachedTexturePtr = NULL;

#if __ASSERT
	Gnm::DataFormat format = grcTextureGNMFormats::grctf_to_Orbis[eShaderViewFormat];
	if      (eShaderViewFormat == grctfL8) { format = sce::Gnm::kDataFormatR8Unorm; } // can't render to L8, but we can render to R8 ..
	else if (eShaderViewFormat == grctfL16) { format = sce::Gnm::kDataFormatR16Unorm; } // can't render to L16, but we can render to R16 ..
	else if (eShaderViewFormat == grctfD24S8) { format = sce::Gnm::kDataFormatR32Float; }
	else if (eShaderViewFormat == grctfX24G8) { format = kDataFormatX24G8Uint; }
	else if (eShaderViewFormat == grctfD32F) { format = sce::Gnm::kDataFormatR32Float; }
	Assertf(format.m_asInt != sce::Gnm::kDataFormatInvalid.m_asInt, "grcRenderTargetGNM - format %d not handled", eShaderViewFormat);
#endif	// __ASSERT

	bool bStencil = (eShaderViewFormat == grctfX24G8);
	bool bDepth = (eShaderViewFormat == grctfD24S8) || (eShaderViewFormat == grctfD32F); // no stencil?
	bool bColor = !bStencil && !bDepth;

#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif
	m_Target = NULL;
	m_DepthTarget = NULL;
	
	if (bColor)
		m_GnmTexture.initFromRenderTarget( pTexture->GetColorTarget(), false );

	if (bDepth)
		m_GnmTexture.initFromDepthRenderTarget( pTexture->GetDepthTarget() NotACubeMap );
	{
		m_DepthTarget = pTexture->m_DepthTarget;
		m_Type = grcrtDepthBuffer;
	}

	if (bStencil)
	{
		m_GnmTexture.initFromStencilTarget( pTexture->GetDepthTarget(), stencilChannelType NotACubeMap );
		Assert( m_GnmTexture.getDataFormat().getStencilFormat() );
		if (!m_GnmTexture.getDataFormat().getStencilFormat())
		{
			Quitf("Unable to create a stencil texture");
		}
		m_Type = grcrtDepthBuffer;
	}

	m_CachedTexturePtr = &m_GnmTexture;

#if DEVICE_EQAA
	m_Coverage.isMultisampled = pTexture->GetCoverageData().isMultisampled;
#endif
}

grcRenderTargetGNM::grcRenderTargetGNM(const char *name, const grcTextureObject *pTexture, grcRenderTargetType type)
{
	using namespace sce;
	
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif

	if (type == grcrtPermanent)
	{
		m_Owned = false;
		m_Name = StringDuplicate(name);
		m_Target = rage_new sce::Gnm::RenderTarget;
		m_DepthTarget = NULL;

		u32 width = pTexture->getWidth();
		u32 height = pTexture->getHeight();
		u32 mipcount = pTexture->getLastMipLevel() - pTexture->getBaseMipLevel() + 1;

        sce::Gnm::DataFormat format = pTexture->getDataFormat();
        if (format.getSurfaceFormat() == sce::Gnm::kSurfaceFormatBc1)
		{
            // we allow rendertargets for dxt1 textures when we do dxt compression on the gpu and render into the texture
            format = sce::Gnm::DataFormat::build(sce::Gnm::kRenderTargetFormat16_16_16_16, sce::Gnm::kRenderTargetChannelTypeUNorm, sce::Gnm::kRenderTargetChannelOrderStandard);
			m_GnmTexture.initAs2dArray(	width >> 2,
                                        height >> 2,
                                        pTexture->getDepth(),
                                        mipcount,
                                        format,
                                        pTexture->getTileMode(),
                        #if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
                                        pTexture->getNumFragments(),
                        #else
                                        pTexture->getNumSamples(),
                        #endif
                                        false // Assuming it's never a cubemap, since [type] would be "grcrtCubeMap"
                                        );	
		}
		else
		{
			Assert(pTexture->getDataFormat().supportsRenderTarget());
			m_GnmTexture.initAs2dArray(	width,
                                        height,
                                        pTexture->getDepth(),
                                        mipcount,
                                        format,
                                        pTexture->getTileMode(),
                        #if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
                                        pTexture->getNumFragments(),
                        #else
                                        pTexture->getNumSamples(),
                        #endif
                                        false // Assuming it's never a cubemap, since [type] would be "grcrtCubeMap"
                                        );	
		}


		m_GnmTexture.setBaseAddress(pTexture->getBaseAddress());
		Assert(m_GnmTexture.getDataFormat().m_asInt == format.m_asInt || format.getSurfaceFormat() == sce::Gnm::kSurfaceFormatBc1);

		m_LockedMip = -1;
		LockMipLevel(0);

		m_CachedTexturePtr = &m_GnmTexture;
	}
	else
	{
		m_Owned = false;
#if DEBUG_TRACK_MSAA_RESOLVES
		m_DebugLastResolvedTarget = NULL;
#endif
		m_Target = NULL;
		m_DepthTarget = NULL;
		m_CachedTexturePtr = NULL;
		Assertf(false,"Unhandled type %d in render target ctor",type);
	}
}

grcRenderTargetGNM::grcRenderTargetGNM(const char *name,
	grcRenderTargetType type,
	int width,
	int height,
	int bitsPerPixel,
	grcTextureFactory::CreateParams *params,
	sce::Gnm::Texture* pOrigTex)
{
	using namespace sce;
	m_Name = StringDuplicate(name);
	m_Type = type;

#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif
	m_Target = NULL;
	m_DepthTarget = NULL;
	m_Owned = true;

	const int mipcount = params? params->MipLevels : 1;
	m_ArraySize = params ? params->ArraySize  / ((type == grcrtCubeMap) ? 6 : 1) : 1;
	m_PerArraySliceRTV = params? params->PerArraySliceRTV : 0;

	Assertf(m_ArraySize > 0, "Render target can't have ArraySize of 0.\n");

	grcDevice::MSAAMode multisample = params ? params->Multisample : grcDevice::MSAA_None;
	m_Coverage.isMultisampled = multisample;

	grcTextureFormat tf = params? params->Format : grctfNone;
	bool useFloat = params? params->UseFloat : false;
	if (tf == grctfNone)
		tf = (bitsPerPixel==128? grctfA32B32G32R32F : bitsPerPixel == 64? grctfA16B16G16R16F : bitsPerPixel == 32? (useFloat? grctfR32F : grctfA8R8G8B8) : bitsPerPixel == 16? grctfR5G6B5 : grctfL8);


	if (type == grcrtPermanent || type == grcrtCubeMap)
	{
		m_Target = rage_new sce::Gnm::RenderTarget;
		m_DepthTarget = NULL;
		
		Gnm::DataFormat format = grcTextureGNMFormats::grctf_to_Orbis[tf];
		
		if      (tf == grctfL8) { format = sce::Gnm::kDataFormatR8Unorm; } // can't render to L8, but we can render to R8 ..
		else if (tf == grctfL16) { format = sce::Gnm::kDataFormatR16Unorm; } // can't render to L16, but we can render to R16 ..
		else if (tf == grctfD24S8) { format = sce::Gnm::kDataFormatR32Float; }
		else if (tf == grctfX24G8) { format = kDataFormatX24G8Uint; }

		Assertf(format.m_asInt != sce::Gnm::kDataFormatInvalid.m_asInt, "grcRenderTargetGNM - format %d not handled", tf);
		Assert(format.supportsRenderTarget());

		Gnm::TileMode tileMode;
		const int32_t tilingStatus = GpuAddress::computeSurfaceTileMode(
			&tileMode, 
			(type == grcrtCubeMap) ? GpuAddress::kSurfaceTypeTextureCubemap : GpuAddress::kSurfaceTypeColorTarget,
			format, 
			1);

		if(pOrigTex != NULL)
		{
			// Use the same tiling otherwise the memory won't align the same
			tileMode = pOrigTex->getTileMode();
		}
		else if (tilingStatus != GpuAddress::kStatusSuccess)
		{
			grcErrorf("computeSurfaceTileMode failed (code 0x%x) for render target '%s', cubmap=%d, format=0x%x",
				tilingStatus, name, type == grcrtCubeMap, format.m_asInt);
			tileMode = sce::Gnm::kTileModeThin_2dThin;
		}
		
		if (mipcount > 1)
		{
			Assert( !multisample );
			if (params && params->ForceNoTiling)
			{
				tileMode = Gnm::kTileModeDisplay_LinearAligned;
			}

			Gnm::SizeAlign result = m_GnmTexture.initAs2dArray
				(width, 
				height, 
				m_ArraySize, 
				mipcount, 
				format, 
				tileMode,
				SingleFragment,
				type == grcrtCubeMap);

			void *baseAddress = pOrigTex == NULL ? allocateVideoPrivateMemory(result) : pOrigTex->getBaseAddress();
			m_GnmTexture.setBaseAddress(baseAddress);

			m_LockedMip = -1;
			LockMipLevel(0);
		}
		else
		{
			Assertf(pOrigTex == NULL, "GNM: no support for overlapping textures with no mipmapping");
			Assertf(m_ArraySize == 1, "allocating render target array with no mips will always lock the first array slice\n");

			m_LockedMip = 0;

			bool useCmask = (!params || params->EnableFastClear) && !PARAM_noFastClearColor.Get() &&
				format.getSurfaceFormat() != sce::Gnm::kSurfaceFormat32_32_32_32;
			bool useFmask = false;
			Gnm::SizeAlign cmaskSizeAlign, fmaskSizeAlign;

			if (params && params->ForceNoTiling)
			{
				tileMode = Gnm::kTileModeDisplay_LinearAligned;
			}
			if (multisample)
			{
				Assertf(type != grcrtCubeMap, "cubemap can't be MSAAed. ... for now\n");
				Assert(!(params && params->EnableHighQualityResolve && params->ForceHardwareResolve));
				bool resolveHW = params && params->ForceHardwareResolve;
				const int32_t tilingStatus = GpuAddress::computeSurfaceTileMode(&tileMode,
					resolveHW ? GpuAddress::kSurfaceTypeColorTargetDisplayable : GpuAddress::kSurfaceTypeColorTarget,
					format, multisample);
				if (tilingStatus != GpuAddress::kStatusSuccess)
				{
					grcErrorf("computeSurfaceTileMode failed (code 0x%x) for MSAA color '%s'", tilingStatus, m_Name);
					tileMode = sce::Gnm::kTileModeThin_2dThin;
				}
#if DEVICE_EQAA
				useFmask = (multisample.m_uSamples>1 && params->ForceFragmentMask) || multisample.NeedFmask();
				grcAssertf( !useFmask || GRCDEVICE.IsEQAA(), "Unable to enable Fmask for non EQAA device" );
				//grcAssertf( useCmask == useFmask, "Unable to use fast-clears on a MSAA target without fragment mask: %s", GetName() );
				//useCmask = useFmask || (params->EnableFastClear && s_UseCmaskAA);
				if (multisample.m_uSamples > 1)
				{
					useCmask = useFmask;	//no fast-clears on MSAA without Fmask, see:
					//https://ps4.scedev.net/support/issue/24629/_Misbehaving_kCbModeEliminateFastClear_with_4xMSAA_targets
				}
#endif // DEVICE_EQAA

				//TODO: use u32 instead of sce::Gnm::NumSamples
				switch (params->SupersampleFrequency ? params->SupersampleFrequency : multisample.m_uFragments)
				{
				case 1:
					m_Coverage.superSample = sce::Gnm::kNumSamples1;
					break;
				case 2:
					m_Coverage.superSample = sce::Gnm::kNumSamples2;
					break;
				case 4:
					m_Coverage.superSample = sce::Gnm::kNumSamples4;
					break;
				case 8:
					m_Coverage.superSample = sce::Gnm::kNumSamples8;
					break;
				default:
					Assertf(0, "Invalid supersample frequency: %u", params->SupersampleFrequency);
				}
				
				m_Coverage.resolveType = resolveHW ? ResolveHW :
					(params && params->EnableHighQualityResolve) ? ResolveSW_HighQuality : ResolveSW_Simple;

				if (m_Coverage.resolveType == ResolveSW_Simple && (params && params->EnableNanDetect))
				{
					m_Coverage.resolveType = ResolveSW_Simple_NanDetect;
				}
			}

			const Gnm::SizeAlign sizeAlign = m_Target->init( width, height, m_ArraySize, format, tileMode,
				multisample.GetSamplesEnum(), multisample.GetFragmentsEnum(),
				useCmask ? &cmaskSizeAlign : NULL,
				useFmask ? &fmaskSizeAlign : NULL );

			Assert( m_Target->getCmaskFastClearEnable() == useCmask );
		
			void *const renderTargetAddr = allocateVideoPrivateMemory( sizeAlign );
			void *const cmaskAddr = useCmask ? allocateVideoPrivateMemory( cmaskSizeAlign ) : NULL;
			void *const fmaskAddr = useFmask ? allocateVideoPrivateMemory( fmaskSizeAlign ) : NULL;

			m_Target->setAddresses( renderTargetAddr, cmaskAddr, fmaskAddr );
			m_GnmTexture.initFromRenderTarget( m_Target, type == grcrtCubeMap );

			if (useFmask)
			{
				m_Coverage.texture = rage_new grcTextureGNM( *m_Target, 'f' );
			}else
			{
				//that will cause Fmask address to match Cmask address
				//m_Target->disableFmaskCompressionForMrtWithCmask();
			}
		}
	}
	else if (type == grcrtDepthBuffer || type == grcrtShadowMap || type == grcrtDepthBufferCubeMap) {
		m_DepthTarget = rage_new sce::Gnm::DepthRenderTarget;
		Gnm::SizeAlign stencilSizeAlign;
		Gnm::SizeAlign htileSizeAlign;
		const bool stencil =(!params || params->EnableStencil) && !PARAM_noStencil.Get();
		bool useHtile = (!params || params->EnableFastClear) && !PARAM_noFastClearDepth.Get();
		const Gnm::StencilFormat stencilFormat = stencil? Gnm::kStencil8 : Gnm::kStencilInvalid;		// kStencilInvalid to disable
		Gnm::TileMode depthTileMode;
		Gnm::DataFormat depthFormat = Gnm::DataFormat::build(bitsPerPixel==16 ? Gnm::kZFormat16 : Gnm::kZFormat32Float);
		int32_t tilingStatus;

	
		if (multisample)
		{
			tilingStatus = GpuAddress::computeSurfaceTileMode(&depthTileMode, GpuAddress::kSurfaceTypeDepthOnlyTarget, depthFormat, multisample);
		}else
		{
			tilingStatus = GpuAddress::computeSurfaceTileMode(&depthTileMode, GpuAddress::kSurfaceTypeDepthOnlyTarget, depthFormat, 1);
		}

		if (tilingStatus != GpuAddress::kStatusSuccess)
		{
			grcErrorf("computeSurfaceTileMode failed (code 0x%x) for depth '%s'", tilingStatus, m_Name);
			depthTileMode = sce::Gnm::kTileModeThin_2dThin;
		}

		Gnm::SizeAlign depthTargetSizeAlign;
		if (m_ArraySize > 1)
			depthTargetSizeAlign = m_DepthTarget->init( width, height, m_ArraySize, depthFormat.getZFormat(), stencilFormat, depthTileMode, multisample.GetFragmentsEnum(), stencil ? &stencilSizeAlign : NULL,	useHtile ? &htileSizeAlign : NULL );
		else
			depthTargetSizeAlign = m_DepthTarget->init( width, height, depthFormat.getZFormat(), stencilFormat, depthTileMode, multisample.GetFragmentsEnum(), stencil ? &stencilSizeAlign : NULL,	useHtile ? &htileSizeAlign : NULL );


		void * depthBufferBaseAddr = NULL;

		// overlapping is specific to shadow maps for now, and not generally supported. SVG will support proper rendertarget pools again...
		if (pOrigTex)	
		{
			Assertf(stencil==0 && useHtile==0 && multisample==grcDevice::MSAA_None,"overlapping texture are only minimally supported");	
			depthBufferBaseAddr = pOrigTex->getBaseAddress(); // there better be room and the alignement better be correct.
			m_Owned = false; // so we don't free this one up
		}
		else
		{
			depthBufferBaseAddr = allocateVideoPrivateMemory( depthTargetSizeAlign );
		}

		void *const stencilAddr = stencil ? allocateVideoPrivateMemory( stencilSizeAlign ) : NULL;
		m_DepthTarget->setAddresses( depthBufferBaseAddr, stencilAddr );
		Assert(depthBufferBaseAddr == m_DepthTarget->getZWriteAddress());
		Assert(stencilAddr == m_DepthTarget->getStencilWriteAddress());

		if (useHtile) {
			void *htileAddr = allocateVideoPrivateMemory( htileSizeAlign );
			m_DepthTarget->setHtileAddress( htileAddr );
		}
		m_DepthTarget->setHtileAccelerationEnable( useHtile );
		m_DepthTarget->setHtileStencilDisable(!(stencil && s_UseStencilCompression));

		m_GnmTexture.initFromDepthRenderTarget( m_DepthTarget, type == grcrtDepthBufferCubeMap);
	}
	else
		Assertf(false,"Unhandled type %d in render target ctor",type);

	m_GnmTexture.setResourceMemoryType( params && params->UseAsUAV ? sce::Gnm::kResourceMemoryTypeGC : sce::Gnm::kResourceMemoryTypeRO );

	m_CachedTexturePtr = &m_GnmTexture;
}

grcRenderTargetGNM::grcRenderTargetGNM(const char *name, sce::Gnm::RenderTarget *color, sce::Gnm::DepthRenderTarget *depth, sce::Gnm::DepthRenderTarget *stencil)
{
	m_Name = StringDuplicate(name);
	m_Type = grcrtPermanent;

	m_Owned = false;
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif
	m_Target = NULL;
	m_DepthTarget = NULL;
	m_CachedTexturePtr = &m_GnmTexture;

	if (color) {
		m_Target = rage_new sce::Gnm::RenderTarget;
		memcpy( m_Target, color, sizeof(*color) );
		m_GnmTexture.initFromRenderTarget(color,false);
	}else
	if (depth) {
		m_DepthTarget = rage_new sce::Gnm::DepthRenderTarget;
		memcpy( m_DepthTarget, depth, sizeof(*depth) );
		m_GnmTexture.initFromDepthRenderTarget(depth NotACubeMap);
	}else
	if (stencil) {
		m_DepthTarget = rage_new sce::Gnm::DepthRenderTarget;
		memcpy( m_DepthTarget, depth, sizeof(*depth) );
		m_GnmTexture.initFromStencilTarget(depth, stencilChannelType NotACubeMap);
	}
}

grcRenderTargetGNM::~grcRenderTargetGNM()
{
	grcTextureFactory::UnregisterRenderTarget(this);

	if (m_Owned) {
		if (m_Target) {
			freeVideoPrivateMemory(m_Target->getBaseAddress());
		}else
		if (m_DepthTarget) {
			freeVideoPrivateMemory(m_DepthTarget->getZWriteAddress());
			freeVideoPrivateMemory(m_DepthTarget->getStencilWriteAddress());
			freeVideoPrivateMemory(m_DepthTarget->getHtileAddress());
		}
	}

	if (m_Target) {
		delete m_Target;
		m_Target = NULL;
		if (m_Coverage.texture) {
			delete (m_Coverage.texture);
			m_Coverage.texture = NULL;
		}
	}else
	if (m_DepthTarget) {
		delete m_DepthTarget;
		m_DepthTarget = NULL;
	}
}

bool grcRenderTargetGNM::LockMipLevel(int mipLevel)
{
	if (m_LockedMip == mipLevel) return true;
	Assert( m_GnmTexture.getDataFormat().supportsRenderTarget() );
	int status = m_Target->initFromTexture(&m_GnmTexture, mipLevel);
	Assertf( !status, "Error locking render target: error 0x%x", status );
	m_LockedMip = mipLevel;
	return status==0;
}

void grcRenderTargetGNM::SetName(const char *name)
{
	Assert(!m_Name);
	m_Name = StringDuplicate(name);
}

int grcRenderTargetGNM::GetBitsPerPixel() const
{
	return m_GnmTexture.getDataFormat().getBitsPerElement();
}

bool grcRenderTargetGNM::IsGammaEnabled() const
{
	return false;
}

void grcRenderTargetGNM::SetGammaEnabled(bool enabled)
{

}

void grcRenderTargetGNM::GenerateMipmaps()
{
	GRC_ALLOC_SCOPE_AUTO_PUSH_POP();
	grcBlendStateHandle BS_Previous = grcStateBlock::BS_Active;
	grcDepthStencilStateHandle DSS_Previous = grcStateBlock::DSS_Active;
	grcRasterizerStateHandle RS_Previous = grcStateBlock::RS_Active;

	grcStateBlock::SetStates( grcStateBlock::RS_NoBackfaceCull, grcStateBlock::DSS_IgnoreDepth, grcStateBlock::BS_Default );
	sce::Gnm::RenderTargetChannelType channelType = sce::Gnm::kRenderTargetChannelTypeFloat;
	AssertVerify(m_Target->getDataFormat().getRenderTargetChannelType( &channelType ));
	u32 channelBits = m_Target->getDataFormat().getBitsPerElement()/m_Target->getDataFormat().getNumComponents();
	const int passId = (channelType == sce::Gnm::kRenderTargetChannelTypeUNorm && channelBits == 16) ? 1 : 0; 

	for (int mipLvl = 1; mipLvl < GetMipMapCount(); mipLvl++)
	{
		for (int arraySlice = 0; arraySlice < GetArraySize(); arraySlice++)
		{
			s_MipMapShader->SetVar(s_MipMapParamsVar, Vector4(
				(float)mipLvl - 1, // refMipBlurParams_MipIndex
				(float)arraySlice, // refMipBlurParams_BlurSize
				1, // refMipBlurParams_BlurSizeScale
				((float)(0x1 << mipLvl)) / ((float)GetHeight()) // refMipBlurParams_TexelSizeY
				));

			grcTextureFactory::GetInstance().LockRenderTarget(0, this, NULL, arraySlice, false, mipLvl);	

			s_MipMapShader->SetVar(s_MipMapSourceVar, (grcTexture*) this);

			AssertVerify(s_MipMapShader->BeginDraw(grmShader::RMC_DRAW, true, s_MipMapTechnique));
			s_MipMapShader->Bind(passId);

			grcDrawSingleQuadf(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, Color32());

			s_MipMapShader->UnBind();
			s_MipMapShader->EndDraw();

			s_MipMapShader->SetVar(s_MipMapSourceVar, grcTexture::None);
			s_MipMapShader->SetVar(s_MipMapParamsVar, Vector4(0.0, 0.0, 0.0, 0.0));

			grcTextureFactory::GetInstance().UnlockRenderTarget(0);
		}
	}

	grcStateBlock::SetStates( RS_Previous, DSS_Previous, BS_Previous );

	LockMipLevel(0);
}

grcDevice::MSAAMode grcRenderTargetGNM::GetMSAA() const
{
	grcDevice::MSAAMode mode = m_Target ? grcDevice::MSAAMode(*m_Target) : grcDevice::MSAAMode(m_GnmTexture);
	if (m_Coverage.isMultisampled || m_Coverage.donor)
		mode.m_bEnabled = true;
	return mode;
}

bool grcRenderTargetGNM::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags) const
{
	Assert(layer == 0);
	Assert(mipLevel <= m_GnmTexture.getLastMipLevel());

	uint64_t offset;
	uint64_t size;
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	sce::GpuAddress::computeSurfaceOffsetAndSize(
#else
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(
#endif
		&offset,&size,&m_GnmTexture,mipLevel,0);

	lock.Base = (char*)m_GnmTexture.getBaseAddress() + offset;
	lock.BitsPerPixel = m_GnmTexture.getDataFormat().getBitsPerElement();
	lock.Width = m_GnmTexture.getWidth() >> mipLevel; 
	lock.Height = m_GnmTexture.getHeight() >> mipLevel;
	lock.Layer = 0;
	lock.MipLevel = mipLevel;
	lock.RawFormat = m_GnmTexture.getDataFormat().m_asInt;
	lock.Pitch = (m_GnmTexture.getPitch() * m_GnmTexture.getDataFormat().getBitsPerElement()) >> (3 + mipLevel);
	return true;
}

bool grcRenderTargetGNM::Copy(const grcTexture* pTexture, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex)
{
	// Make sure num fragments per pixel matches between source and dest (when EQAA is enabled, we
	// allow copies between single fragment EQAA buffers and non-EQAA buffers.
	u8 srcNumFragments = (pTexture && pTexture->GetMSAA()>1) ? pTexture->GetMSAA() : 1;
	u8 dstNumFragments = GetMSAA()>1 ? GetMSAA() : 1;

	if (!AssertVerify( pTexture && pTexture!=this &&
		srcNumFragments		== dstNumFragments &&
		GetWidth()			== pTexture->GetWidth() &&
		GetHeight()			== pTexture->GetHeight() &&
		GetBitsPerPixel()	== pTexture->GetBitsPerPixel() ))
	{
		return false;
	}
	u32 uDstLayer = (dstSliceIndex<0) ? 0: (u32)dstSliceIndex;
	u32 uDstMipLevel = (dstMipIndex<0) ?  0: (u32)dstMipIndex;

	u32 uSrcLayer = (srcSliceIndex<0) ? 0: (u32)srcSliceIndex;
	u32 uSrcMipLevel = (srcMipIndex<0) ?  0: (u32)srcMipIndex;

	const sce::Gnm::Texture *const srcTexture = pTexture->GetTexturePtr();

	uint64_t srcOffset, dstOffset,srcSize, dstSize;
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&srcOffset, &srcSize, srcTexture, uSrcMipLevel, uSrcLayer);
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&dstOffset, &dstSize, &m_GnmTexture, uDstMipLevel, uDstLayer);

	Assert(srcSize == dstSize);

	void *const dst = (char*)m_GnmTexture.getBaseAddress() + dstOffset;
	void *const src = (char*)srcTexture->getBaseAddress() + srcOffset;

	if (pTexture->GetResourceType() == grcTexture::RENDERTARGET &&
		static_cast<grcTextureFactoryGNM&>( grcTextureFactory::GetInstance() ).IsTargetLocked(
			static_cast<const grcRenderTargetGNM*>(pTexture)))
	{
		gfxc.waitForGraphicsWrites( (u32)(((u64)src)>>8), srcSize>>8, sce::Gnm::kWaitTargetSlotCb0,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
			sce::Gnm::kStallCommandBufferParserDisable );
	}

	if (s_CopyByCompute)
	{
		eqaa::CopyBuffer( "RT copy", dst, src, srcSize );
	}else
	{
		// warning: this is slow!
		gfxc.copyData( dst, src, srcSize, sce::Gnm::kDmaDataBlockingEnable );
	}

	return true;
}

void grcRenderTargetGNM::CreateMipMaps(const grcResolveFlags* resolveFlags, int index)
{
	Assert(false);
}

void grcRenderTargetGNM::Blur(const grcResolveFlags* resolveFlags)
{
	Assert(false);
}

grcTexture::ChannelBits grcRenderTargetGNM::FindUsedChannels() const
{
	sce::Gnm::DataFormat fmt = m_GnmTexture.getDataFormat();
	ChannelBits result;
	if (m_Target) {
		if (fmt.m_bits.m_channelX >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_RED);
		if (fmt.m_bits.m_channelY >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_GREEN);
		if (fmt.m_bits.m_channelZ >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_BLUE);
		if (fmt.m_bits.m_channelW >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_ALPHA);
	}
	else if (m_DepthTarget) {
		// TODO: Confirm this is correct
		if (fmt.m_bits.m_channelX >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_DEPTH);
		if (fmt.m_bits.m_channelY >= sce::Gnm::kTextureChannelX)
			result.Set(CHANNEL_STENCIL);
	}
	return result;
}

void grcRenderTargetGNM::AllocateMemoryFromPool()
{
}

void grcRenderTargetGNM::ReleaseMemoryToPool()
{
}

void grcRenderTargetGNM::UpdateMemoryLocation(const grcTextureObject *pTexture) 
{
	void *const base = pTexture->getBaseAddress();
	Assertf(base, "Attempting to update a non-allocated texture");
	m_GnmTexture.setBaseAddress(base);
	if (m_Target)
	{
		Assert(!m_DepthTarget);
		m_Target->setBaseAddress(base);
	}
	else
	{
		Assert(m_DepthTarget);
		m_DepthTarget->setZReadAddress(base);
		m_DepthTarget->setZWriteAddress(base);
	}
}

#if DEBUG_TRACK_MSAA_RESOLVES
bool grcRenderTargetGNM::HasBeenResolvedTo(grcRenderTarget* resolveTo)
{
	Assert( !m_DepthTarget && resolveTo );
	grcRenderTargetGNM *const resolveDest = static_cast<grcRenderTargetGNM*>(resolveTo);

	return m_DebugLastResolvedTarget == resolveDest->GetResolveTarget();
}
#endif

void grcRenderTargetGNM::Resolve(grcRenderTarget* resolveTo, int destSliceIndex, uint debugMask)
{
	Assert( !m_DepthTarget && resolveTo );
	grcRenderTargetGNM *const resolveDest = static_cast<grcRenderTargetGNM*>(resolveTo);

#if DEBUG_TRACK_MSAA_RESOLVES
	sce::Gnm::RenderTarget *const target = resolveDest->GetResolveTarget();
	Assertf(debugMask || m_DebugLastResolvedTarget != target, "Redundant resolve");
#else
	(void)debugMask;
#endif

	if (static_cast<grcTextureFactoryGNM&>(grcTextureFactory::GetInstance()).IsTargetLocked(this))
	{
		Assertf( false,	
			"Resolving Fmask-enabled AA surface during rendering is not allowed: %s", GetName() );
		
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
		gfxc.waitForGraphicsWrites( m_Target->getBaseAddress256ByteBlocks(), m_Target->getSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotCb0,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
			sce::Gnm::kStallCommandBufferParserDisable );
	}

	if (!m_Coverage.isMultisampled)
	{
		//Assertf(0,"Attempted to resolve a non-AA target");
		resolveDest->Copy(this,0);
	}else
	{
		GRCDEVICE.ResolveMsaaBuffer( resolveDest, this, destSliceIndex );	
	}

#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = target;
#endif
}


u32 grcRenderTargetGNM::TranslateImageFormat(const sce::Gnm::DataFormat &dataFormat)
{
	for (u32 uIndex = 0; uIndex < grcImage::FORMAT_COUNT; uIndex++)
	{
		if (dataFormat.m_asInt == grcTextureGNMFormats::grcImage_to_Orbis[uIndex].m_asInt)
			return static_cast<grcImage::Format>(uIndex);
	}
	// adding some exceptions that don't have 1-to-1 relating with grcImage
	if (dataFormat.m_asInt == sce::Gnm::kDataFormatR11G11B10Float.m_asInt)
		return grcImage::A2R10G10B10;
	if (dataFormat.getSurfaceFormat() == sce::Gnm::kSurfaceFormat16_16_16_16)
		return grcImage::A16B16G16R16F;
	return grcImage::UNKNOWN;
}

u32	grcRenderTargetGNM::GetImageFormat() const
{
	const sce::Gnm::Texture *gnmTexture = GetTexturePtr();
	sce::Gnm::DataFormat format = gnmTexture->getDataFormat();
	return TranslateImageFormat( format );
}

const void grcRenderTargetGNM::SetColorTargetArrayView(u32 uArrayIndex) const
{
	if (m_Target)
	{
		m_Target->setArrayView(uArrayIndex,uArrayIndex);
	}
}

const void grcRenderTargetGNM::SetDepthTargetArrayView(u32 uArrayIndex) const
{
	if (m_DepthTarget)
	{
		m_DepthTarget->setArrayView(uArrayIndex,uArrayIndex);
	}
}

void grcRenderTargetGNM::SetFragmentMaskDonor(grcRenderTargetGNM *donor, ResolveType resolveType)
{
	if (donor)
	{
		Assert( !m_Coverage.donor );
		Assert( GetWidth() == donor->GetWidth() && GetHeight() == donor->GetHeight() );
		Assert( GetMSAA().m_uFragments == donor->GetMSAA().m_uFragments );
		//Assert( GetColorTarget()->getTileMode() == donor->GetColorTarget()->getTileMode() );
		//Assert( GetColorTarget()->getDataFormat().m_asInt == donor->GetColorTarget()->getDataFormat().m_asInt );
	}
	
	m_Coverage.donor = donor;
	m_Coverage.resolveType = resolveType;
}

void grcRenderTargetGNM::SetSampleLocations() const
{
	if (m_Coverage.donor && 0)
	{
		// disabled due to Sony B#13035
		GRCDEVICE.SetAALocations( m_Coverage.donor->GetMSAA().m_uSamples, m_Coverage.donor->GetMSAA().m_uFragments );
	}
}


#if GNM_RENDERTARGET_DUMP
static const int s_ExtraInfoSize = 32;

static void SaveSurface(const char *const path, const void *const address, int width, int height, grcImage::Format format, const sce::GpuAddress::TilingParameters &tp)
{
	Assert( path != NULL && address != NULL );

	grcImage *const poImage = grcImage::Create( width, height, 1, format, grcImage::STANDARD, 0, 0 );
	u32 *const buffer = reinterpret_cast<u32*>( poImage->GetBits() );

	sce::GpuAddress::detileSurface( buffer, address, &tp );

	poImage->SaveDDS( path );
	poImage->Release();
}
static void SaveSurface(const char *const path, const void *const address, const sce::Gnm::Texture &texture, grcImage::Format format)
{
	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture( &texture, 0, 0 );
	SaveSurface( path, address, texture.getWidth(), texture.getHeight(), format, tp );
}

static char *MakeSavePath(const char *const name, const void *const address)
{
	static char szFilename[1024];
	formatf(szFilename, sizeof(szFilename), "Targets/%d/%ums_%s_0x%p",
		GRCDEVICE.GetFrameCounter(), sysTimer::GetSystemMsTime(), name, address );
	
	ASSET.CreateLeadingPath(szFilename);
	ASSET.CreateLeadingPath(szFilename);
	
	return szFilename;
}

bool grcRenderTargetGNM::SaveColorTarget(const char *const outName, const sce::Gnm::RenderTarget *const target, const DumpMask mask)
{
	if (!target)
		return false;

	const grcImage::Format format = static_cast<grcImage::Format>(TranslateImageFormat( target->getDataFormat() ));
	if (format == grcImage::UNKNOWN)
		return false;

	char *const filePath = MakeSavePath( outName, target->getBaseAddress() );
	char *const pathTail = filePath + strlen(filePath);

	gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
	gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbPixelData );
	gfxc.triggerEvent( sce::Gnm::kEventTypeCacheFlushAndInvEvent );
#if SUPPORT_RENDERTARGET_DUMP
	GRCDEVICE.CpuWaitOnGpuIdle();
#endif

	const char *address = NULL;
	uint64_t offset = 0;
	uint64_t size = 0;
	
	address = (char*)target->getFmaskAddress();
	if ((mask & DUMP_FMASK) && address)
	{
		sce::Gnm::Texture texture;
		grcImage::Format imgFormat = grcImage::A8R8G8B8;
		sce::Gnm::DataFormat tempFormat = sce::Gnm::kDataFormatR8G8B8A8Unorm;
		if (true)	// shader dump
		{
			const sce::Gnm::SizeAlign sizeAlign = texture.initAs2d( target->getWidth(), target->getHeight(), 1, tempFormat,
				sce::Gnm::kTileModeThin_2dThin, SingleFragment );
			void *const buffer = allocateVideoPrivateMemory( sizeAlign );
			texture.setBaseAddress( buffer );
			for(uint32_t i=0; !(i>>target->getNumSamples()); i+=4)
			{
				GRCDEVICE.UntileFmaskSurface( &texture, target, i );
# if SUPPORT_RENDERTARGET_DUMP
				GRCDEVICE.CpuWaitOnGpuIdle();
# endif
				formatf( pathTail, s_ExtraInfoSize, "_fmask%u_0x%p_shader", i>>2, address );
				SaveSurface( filePath, buffer, texture, imgFormat );
			}
			freeVideoPrivateMemory( buffer );
		}
		if (true)	// direct dump
		{
			const unsigned bits = sce::Gnmx::getFmaskShiftBits( target->getNumSamples(), target->getNumFragments() ) << target->getNumSamples();
			switch (bits)
			{
				case 8:
					imgFormat = grcImage::L8; break;
				case 16:
					imgFormat = grcImage::A8L8; break;
				case 32:
					imgFormat = grcImage::A8R8G8B8; break;
				case 1: case 2: case 4:
				case 64: default:
					imgFormat = grcImage::UNKNOWN;
			}

			if (imgFormat != grcImage::UNKNOWN)
			{
				//GRCDEVICE.DecompressFmaskSurface( target );
				texture.initAsFmask( target->getWidth(), target->getHeight(), 1, target->getFmaskTileMode(), target->getNumSamples(), target->getNumFragments() );
				formatf( pathTail, s_ExtraInfoSize, "_fmask_0x%p_direct", address );
				SaveSurface( filePath, address, texture, imgFormat );
			}
		}
		*pathTail = 0;
	}
	
	address = (char*)target->getCmaskAddress();
	if ((mask & DUMP_CMASK) && address)
	{
		formatf( pathTail, s_ExtraInfoSize, "_cmask_0x%p", address );
		const unsigned numTilePairs = (target->getWidth() + 7)/16;
		grcImage *const poImage = grcImage::Create( numTilePairs*2, target->getHeight()/8, 1, grcImage::L8, grcImage::STANDARD, 0, 0 );
		ASSERT_ONLY(const u32 size =) eqaa::UntileCmask( poImage->GetBits(), address, target->getWidth(), target->getHeight(), false, false );
		Assert(size == poImage->GetSize());
		poImage->SaveDDS( filePath );
		poImage->Release();
		*pathTail = 0;
	}
	
	address = (char*)target->getBaseAddress();
	if ((mask & DUMP_COLOR) && address)
	{
		sce::GpuAddress::TilingParameters tp;
		sce::GpuAddress::computeRenderTargetSurfaceOffsetAndSize(&offset,&size, target, 0);
		tp.initFromRenderTarget(target, 0);
		const sce::Gnm::NumFragments logFragments = target->getNumFragments();
		SaveSurface( filePath, address + offset, target->getWidth() << logFragments, target->getHeight(), format, tp );
	}

	return true;
}

bool grcRenderTargetGNM::SaveDepthTarget(const char *const outName, const sce::Gnm::DepthRenderTarget *const target, const DumpMask mask)
{
	if (!target)
		return false;

	char *const filePath = MakeSavePath( outName, target->getZReadAddress() );
	char *const pathTail = filePath + strlen(filePath);
	
	const char *address = NULL;
	sce::GpuAddress::TilingParameters tp;
	uint64_t offset = 0;
	uint64_t size = 0;

	address = (char*)target->getHtileAddress();
	if ((mask & DUMP_HTILE) && address)
	{
		formatf( pathTail, s_ExtraInfoSize, "_htile_0x%p", address );
		grcImage *const poImage = grcImage::Create( target->getWidth()/8, target->getHeight()/8, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0 );
		ASSERT_ONLY(const u32 size =) eqaa::UntileHtile( poImage->GetBits(), address, target->getWidth(), target->getHeight(), false, !target->getHtileStencilDisable() );
		Assert(size == poImage->GetSize());
		poImage->SaveDDS( filePath );
		poImage->Release();
		*pathTail = 0;
	}

	const sce::Gnm::NumFragments logFragments = target->getNumFragments();
	address = (char*)target->getZReadAddress();
	if ((mask & DUMP_DEPTH) && address)
	{
		sce::GpuAddress::computeDepthSurfaceOffsetAndSize(&offset,&size, target, 0);
		tp.initFromDepthSurface(target, 0);
		SaveSurface( filePath, address + offset, target->getWidth() << logFragments, target->getHeight(), grcImage::R32F, tp );
	}
	
	address = (char*)target->getStencilReadAddress();
	if ((mask & DUMP_STENCIL) && address)
	{
		sce::GpuAddress::computeStencilSurfaceOffsetAndSize(&offset,&size, target, 0);
		tp.initFromStencilSurface(target, 0);
		strcpy( pathTail, "_stencil" );
		SaveSurface( filePath, address + offset, target->getWidth() << logFragments, target->getHeight(), grcImage::A8, tp );
		*pathTail = 0;
	}

	return true;
}

bool grcRenderTargetGNM::SaveTarget(const char *outName) const
{ 
	if (!outName)
		outName = GetName();

#if SUPPORT_RENDERTARGET_DUMP
	if (!grcSetupInstance->ShouldCaptureRenderTarget(outName))
		return false;
#endif

	grcTextureLock oLock;
	if (!LockRect(0, 0, oLock, grcsRead))
		return false;

	// Todo:
	// Fmask (polish)
	// Z32F (add to image class)
	// CMask (fix)
	// HTile
	// Volume Textures
	// Cubemaps

	SaveColorTarget( outName, GetColorTarget(), DUMP_ALL );
	SaveDepthTarget( outName, GetDepthTarget(), DUMP_ALL );

	UnlockRect(oLock);

	return true;
}
#endif	//GNM_RENDERTARGET_DUMP

void grcRenderTargetPoolEntry::AllocatePoolMemory(u32 size, bool physical, int alignment, void * buffer)
{
	Assert(false);
}

void grcRenderTargetPoolEntry::FreePoolMemory()
{
	Assert(false);
}

}	// namespace rage

#endif
