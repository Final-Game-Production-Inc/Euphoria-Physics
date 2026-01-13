// 
// grcore/texturefactory_gnm.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "texturefactory_gnm.h"
#include "texture_gnm.h"
#include "rendertarget_gnm.h"
#include "texturereference.h"
#include "image.h"	// for WritePNG

#if __GNM

#include "gfxcontext_gnm.h"

namespace rage {

grcRenderTargetGNM *grcTextureFactoryGNM::sm_FrontBuffer, *grcTextureFactoryGNM::sm_BackBuffer;
grcRenderTargetGNM *grcTextureFactoryGNM::sm_FrontBufferDepth, *grcTextureFactoryGNM::sm_BackBufferDepth;

DECLARE_MTR_THREAD const sce::Gnm::DepthRenderTarget *s_CurDepth;
DECLARE_MTR_THREAD const sce::Gnm::RenderTarget* s_CurColors[grcmrtColorCount];

#if __ASSERT
DECLARE_MTR_THREAD const sce::Gnm::RenderTarget *s_CurLockSingleTarget = NULL;
#endif

// currently bound grcRenderTargetGNM
DECLARE_MTR_THREAD const grcRenderTargetGNM *s_pColor = NULL;
DECLARE_MTR_THREAD const grcRenderTargetGNM *s_pDepth = NULL;

DECLARE_MTR_THREAD sce::Gnm::NumSamples s_CurSupersampleFrequency;
static const bool s_bAutoFinishRendering = false;

grcTextureFactoryGNM::grcTextureFactoryGNM()
{
}

grcTextureFactoryGNM::~grcTextureFactoryGNM()
{

}

void grcTextureFactoryGNM::SetPrivateSurfaces(sce::Gnm::RenderTarget *front,sce::Gnm::DepthRenderTarget *frontDepth,sce::Gnm::RenderTarget *back,sce::Gnm::DepthRenderTarget *backDepth)
{
	sm_FrontBuffer->m_Target = front; sm_FrontBuffer->m_GnmTexture.initFromRenderTarget(front,false); Assert(front->getDataFormat().m_asInt == sm_FrontBuffer->m_GnmTexture.getDataFormat().m_asInt); sm_FrontBuffer->m_CachedTexturePtr = &sm_FrontBuffer->m_GnmTexture;
	sm_FrontBufferDepth->m_DepthTarget = frontDepth;
	sm_BackBuffer->m_Target = back; sm_BackBuffer->m_GnmTexture.initFromRenderTarget(back,false); Assert(back->getDataFormat().m_asInt == sm_BackBuffer->m_GnmTexture.getDataFormat().m_asInt); sm_BackBuffer->m_CachedTexturePtr = &sm_BackBuffer->m_GnmTexture;
	sm_BackBufferDepth->m_DepthTarget = backDepth;
	
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	sm_FrontBufferDepth->m_GnmTexture.initFromDepthRenderTarget(frontDepth, false);
	sm_BackBufferDepth->m_GnmTexture.initFromDepthRenderTarget(backDepth, false);
#else
	sm_FrontBufferDepth->m_GnmTexture.initFromDepthRenderTarget(frontDepth);
	sm_BackBufferDepth->m_GnmTexture.initFromDepthRenderTarget(backDepth);
#endif //SCE_ORBIS_SDK_VERSION
	
	s_CurDepth = backDepth;
	memset( s_CurColors, 0, sizeof(s_CurColors) );
	// Displayf( "SetPrivateSurfaces front(address=0x%p), back(address=0x%p)", front->getBaseAddress(), back->getBaseAddress() );
	s_CurColors[0] = back;
	Assert( back->getNumFragments() == backDepth->getNumFragments() );
}

grcRenderTarget* grcTextureFactoryGNM::GetBackBuffer(bool)
{
	return sm_BackBuffer;
}

const grcRenderTarget* grcTextureFactoryGNM::GetBackBuffer(bool) const
{
	return sm_BackBuffer;
}

grcRenderTarget* grcTextureFactoryGNM::GetBackBufferDepth(bool)
{
	return sm_BackBufferDepth;
}

const grcRenderTarget* grcTextureFactoryGNM::GetBackBufferDepth(bool) const
{
	return sm_BackBufferDepth;
}

grcRenderTarget* grcTextureFactoryGNM::GetFrontBuffer(bool)
{
	return sm_FrontBuffer; 
}

const grcRenderTarget* grcTextureFactoryGNM::GetFrontBuffer(bool) const
{
	return sm_FrontBuffer;
}

grcRenderTarget* grcTextureFactoryGNM::GetFrontBufferDepth(bool)
{
	return sm_FrontBufferDepth;
}

const grcRenderTarget* grcTextureFactoryGNM::GetFrontBufferDepth(bool) const
{
	return sm_FrontBufferDepth;
}

grcTextureFactory* grcTextureFactory::CreatePagedTextureFactory(bool bMakeActive)
{
	grcTextureFactory *pFactory = rage_new grcTextureFactoryGNM;
	if	(bMakeActive)
		SetInstance(*pFactory);
	return pFactory;
}

grcTexture *grcTextureFactoryGNM::Create(const char *filename,TextureCreateParams *params)
{
	grcTexture *tex = LookupTextureReference(filename);
	if (tex)
		return tex;

	return rage_new grcTextureGNM(filename,params);
}

grcTexture *grcTextureFactoryGNM::Create(grcImage *image,TextureCreateParams *params)
{
	return rage_new grcTextureGNM(image,params);
}

grcTexture* grcTextureFactoryGNM::Create(u32 width, u32 height, u32 format, void* pBuffer, u32 numMips, TextureCreateParams * params)
{
	grcTextureFactory::TextureCreateParams temp(grcTextureFactory::TextureCreateParams::VIDEO,grcTextureFactory::TextureCreateParams::TILED);
	if (params)
		temp = *params;
	temp.MipLevels = numMips;
	
	return rage_new grcTextureGNM(width,height,format,pBuffer,&temp);
}

grcTexture* grcTextureFactoryGNM::Create(u32 width, u32 height, u32 depth, u32 format, void* pBuffer, TextureCreateParams * params)
{
	grcTextureFactory::TextureCreateParams temp(grcTextureFactory::TextureCreateParams::VIDEO,grcTextureFactory::TextureCreateParams::TILED);
	if (params)
		temp = *params;
	temp.MipLevels = 1;

	return rage_new grcTextureGNM(width,height,depth,format,pBuffer,&temp);
}

grcRenderTarget *grcTextureFactoryGNM::CreateRenderTarget(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params, grcRenderTarget* originalTarget)
{
	sce::Gnm::Texture* pOrigTex = originalTarget != NULL ? (sce::Gnm::Texture*)originalTarget->GetTexturePtr() : NULL;
	grcRenderTarget *const target = rage_new grcRenderTargetGNM(pName,eType,nWidth,nHeight,nBitsPerPixel,params, pOrigTex);
	RegisterRenderTarget(target);

	return target;
}

#if !__RESOURCECOMPILER
grcRenderTarget *grcTextureFactoryGNM::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture, grcRenderTarget* UNUSED_PARAM(originalTarget))
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	grcRenderTargetGNM *tgt = NULL;
	tgt = rage_new grcRenderTargetGNM(pName, pTexture, grcrtPermanent);

	RegisterRenderTarget(tgt);

	return tgt;
}

grcRenderTarget *grcTextureFactoryGNM::CreateRenderTarget(const char *pName, const grcRenderTarget *pTexture, grcTextureFormat eShaderViewFormat /*= grctfNone*/)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	/*
	if (originalTarget)
	{
		((grcRenderTargetGNM*)originalTarget)->DestroyInternalData();
		if( textureFormat != grctfNone )
			((grcRenderTargetGNM*)originalTarget)->CreateFromTextureObject(pName,pTexture, textureFormat, depthStencilReadOnly);
		else
			((grcRenderTargetGNM*)originalTarget)->CreateFromTextureObject(pName,pTexture, depthStencilReadOnly);
		return originalTarget;
	}
	*/

	grcRenderTargetGNM *tgt = NULL;
	/*
	const sce::Gnm::DataFormat oFormat = pTexture->getDataFormat();
	grcTextureFormat textureFormat = TranslateToRageFormat(*(u32*)&oFormat);
	bool depthStencilReadOnly = oFormat.getStencilFormat() == sce::Gnm::kStencilInvalid ? true : false;
	
	if( textureFormat != grctfNone )
		tgt = rage_new grcRenderTargetGNM(pName,pTexture);
	else
	*/
		tgt = rage_new grcRenderTargetGNM(pName, (grcRenderTargetGNM*)pTexture, eShaderViewFormat);

	RegisterRenderTarget(tgt);

	return tgt;
}

grcRenderTarget *grcTextureFactoryGNM::CreateProxyRenderTarget(const char *pName, sce::Gnm::RenderTarget *color, sce::Gnm::DepthRenderTarget *depth, sce::Gnm::DepthRenderTarget *stencil)
{
	grcRenderTargetGNM *const tgt = new grcRenderTargetGNM(pName,color,depth,stencil);
	RegisterRenderTarget(tgt);
	return tgt;
}

grcTexture *grcTextureFactoryGNM::CreateProxyFmaskTexture(const sce::Gnm::RenderTarget *color)
{
	return color && color->getFmaskAddress() ? new grcTextureGNM(*color,'f') : NULL;
}
#endif	//!__RESOURCECOMPILER

extern __THREAD float s_ZScale, s_ZOffset;
__THREAD u32 s_CmaskDirty = 0;

static sce::GpuAddress::TilingParameters tp;

static void copyscan_depth(u8 *dest,void *src,int y,int width,int stride,u8* gamma)
{
	uint64_t offsetTiled;
	float *zread = (float*)s_CurDepth->getZReadAddress();
	for (int x=0; x<width; x++) {
		sce::GpuAddress::computeTiledElementByteOffset(&offsetTiled, &tp, x, y, 0, 0);
		float z = zread[offsetTiled>>2];
		if (z == 0)
			dest[x*3+0] = dest[x*3+1] = dest[x*3+2] = 0;
		else if (z >= 1)
			dest[x*3+0] = dest[x*3+1] = dest[x*3+2] = 255;
		else {
			dest[x*3] = 255;
			dest[x*3+1] = 0;
			dest[x*3+2] = z * 255;
		}
	}
}

NOSTRIP_XPARAM(noFastClearColor);
NOSTRIP_XPARAM(noFastClearDepth);
XPARAM(noStencil);

void WriteDepthBuffer(const char *name)
{
	Assert(s_CurDepth);
	Assert(!s_CurDepth->getHtileAccelerationEnable());
	AssertMsg(PARAM_noFastClearDepth.Get() && PARAM_noStencil.Get(),"WriteDepthBuffer requires both -noFastClearDepth and -noStencil on the command line in order to work properly.");
	GRCDEVICE.CpuWaitOnGpuIdle();
	sysIpcSleep(100);

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	tp.initFromDepthRenderTarget(s_CurDepth, 0);
#else
	tp.initFromDepthSurface(s_CurDepth, 0);
#endif
	// sce::Gnm::SizeAlign sa = sce::GpuAddress::computeUntiledSurfaceSize(&tp);
	// char *tempBuffer = rage_aligned_new(sa.m_align) char[sa.m_size];
	// sce::GpuAddress::detileSurface(tempBuffer,s_CurDepth->getZReadAddress(),&tp);
	grcImage::WritePNG(name,copyscan_depth,s_CurDepth->getWidth(),s_CurDepth->getHeight(),s_CurDepth->getZReadAddress(),s_CurDepth->getPitch() * 4,1.0f);
	// delete[] tempBuffer;
}

bool grcTextureFactoryGNM::IsAnythingLocked() const
{
	return s_CurColors[0] != sm_BackBuffer->GetColorTarget() || s_CurDepth != sm_BackBufferDepth->GetDepthTarget();
}

void grcTextureFactoryGNM::SetArrayView(u32 uArrayIndex)
{
	// we are only locking 0 render target index with 0 mip level for now
	// TODO: implemente for other rt index/mip level if needed

	const sce::Gnm::RenderTarget *const sceColor = s_pColor? s_pColor->GetColorTarget() : NULL;
	if (s_pColor)
	{
		if (s_pColor && s_pColor->GetArraySize() > 1)
		{
			s_pColor->SetColorTargetArrayView(uArrayIndex);
		}
		gfxc.setRenderTarget(0, sceColor);
	}
	else
		AssertMsg(0,"Must change array slice of currently locked color target\n");

	const sce::Gnm::DepthRenderTarget *const sceDepth = s_pDepth? s_pDepth->GetDepthTarget() : NULL;
	if (s_pDepth)
	{
		if (s_pDepth && s_pDepth->GetArraySize() > 1)
		{
			s_pDepth->SetDepthTargetArrayView(uArrayIndex);
		}
		gfxc.setDepthRenderTarget(sceDepth);
	}
	else
		AssertMsg(0,"Must change array slice of currently locked depth target\n");
}

void grcTextureFactoryGNM::LockRenderTarget	(int index,const grcRenderTarget *pColor_, const grcRenderTarget *pDepth_, u32 layer, bool lockDepth, u32 D3D11_OR_ORBIS_ONLY(mipToLock))
{
	Assert(pColor_ || pDepth_);
	Assert(!IsAnythingLocked());
	
	const grcRenderTargetGNM *const pColor = static_cast<const grcRenderTargetGNM*>(pColor_);
	const grcRenderTargetGNM *const pDepth = static_cast<const grcRenderTargetGNM*>(pDepth_);
	s_CmaskDirty = pColor && const_cast<grcRenderTargetGNM*>(pColor)->PopCmaskDirty() ? 1 : 0;
	sce::Gnm::NumSamples	numSamples		= sce::Gnm::kNumSamples1;
	sce::Gnm::NumFragments	numFragments	= sce::Gnm::kNumFragments1;
	sce::Gnm::NumSamples	numIterations	= sce::Gnm::kNumSamples1;
	uint32_t width = 1, height = 1;

	if (lockDepth) {
		const sce::Gnm::DepthRenderTarget *const sceDepth = pDepth? pDepth->GetDepthTarget() : NULL;
		if (pDepth && pDepth->GetArraySize() > 1)
			pDepth->SetDepthTargetArrayView(layer);
		gfxc.setDepthRenderTarget(sceDepth);
		s_CurDepth = sceDepth;
		if (pDepth)	{
			Assert(sceDepth);
			const_cast<grcRenderTargetGNM*>(pDepth)->DebugSetUnresolved();
			s_pDepth = pDepth;
		}
		if (sceDepth)	{
			width = sceDepth->getWidth();
			height = sceDepth->getHeight();
			numFragments = sceDepth->getNumFragments();
			numSamples = static_cast<sce::Gnm::NumSamples>( numFragments );
			numIterations = numSamples;
		}
	}

	if (pColor)	{
		const_cast<grcRenderTargetGNM*>(pColor)->LockMipLevel(mipToLock);
		const_cast<grcRenderTargetGNM*>(pColor)->DebugSetUnresolved();
		s_pColor = pColor;
	}
	const sce::Gnm::RenderTarget *const sceColor = pColor? pColor->GetColorTarget() : NULL;
	if (pColor && pColor->GetArraySize() > 1)
		pColor->SetColorTargetArrayView(layer);
	gfxc.setRenderTarget(index, sceColor);
	s_CurColors[index] = sceColor;

	if (sceColor)	{
#if __ASSERT
		if (lockDepth && pDepth && pDepth->GetDepthTarget())
		{
			Assert(width == sceColor->getWidth());
			Assert(height == sceColor->getHeight());
			Assert(numFragments == sceColor->getNumFragments());
		}else
#endif // __ASSERT
		{
			width = sceColor->getWidth();
			height = sceColor->getHeight();
			numFragments = sceColor->getNumFragments();
		}
		numSamples = sceColor->getNumSamples();
		numIterations = pColor->GetCoverageData().superSample;
	}

	s_CurSupersampleFrequency = numIterations;
	gfxc.setupScreenViewport(0, 0, width, height, s_ZScale, s_ZOffset);

	// disable Scissor
	gfxc.setWindowScissor(0,0,16383,16383,sce::Gnm::kWindowOffsetDisable);

	grcDevice::SetSize(width, height);
	GRCDEVICE.SetAACount(1<<numSamples,1<<numFragments,1<<numIterations);

	if (pColor)
		pColor->SetSampleLocations();

#if !MULTIPLE_RENDER_THREADS
	// Store active targets
	memset(m_LockColorRenderTargets, 0, sizeof(m_LockColorRenderTargets));
	m_LockColorRenderTargets[0] = pColor_;
	m_LockDepthRenderTarget = pDepth_;
#endif	//!MULTIPLE_RENDER_THREADS
#if __ASSERT
	s_CurLockSingleTarget = NULL;
#endif
}

void grcTextureFactoryGNM::UnlockColorTarget(int index)
{
	const sce::Gnm::RenderTarget *const col = s_CurColors[index];
	if (col)
	{
		// TODO: Should add flag to make this optional (need it on during postfx blits that are immediately used), but it doesn't seem to be very expensive.
		gfxc.waitForGraphicsWrites(col->getBaseAddress256ByteBlocks(), col->getSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotCb0 << index,
				sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
				sce::Gnm::kStallCommandBufferParserDisable );
		
		if (s_bAutoFinishRendering)
			GRCDEVICE.FinishRendering( col, s_CmaskDirty & (1<<index) );
	}
	s_CurColors[index] = NULL;
	s_CmaskDirty &= ~(1<<index);

#if SUPPORT_RENDERTARGET_DUMP
	if (m_LockColorRenderTargets[index] && grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
		m_LockColorRenderTargets[index]->SaveTarget();
#endif // SUPPORT_RENDERTARGET_DUMP
#if !MULTIPLE_RENDER_THREADS
	m_LockColorRenderTargets[index] = NULL;
#endif // !MULTIPLE_RENDER_THREADS
}

void grcTextureFactoryGNM::UnlockDepthTarget()
{
	const sce::Gnm::DepthRenderTarget *const depth = s_CurDepth;
	if (depth)
	{
		gfxc.waitForGraphicsWrites(depth->getZWriteAddress256ByteBlocks(), depth->getZSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotDb,
			sce::Gnm::kCacheActionNone/*kCacheActionWriteBackAndInvalidateL1andL2*/, 0x00/*sce::Gnm::kExtendedCacheActionFlushAndInvalidateDbCache*/,
			sce::Gnm::kStallCommandBufferParserDisable );
		
#if RSG_ORBIS
		//GRCDEVICE.DecompressDepthSurface(depth);
#endif
	}
}

void grcTextureFactoryGNM::UnlockRenderTarget(int index,const grcResolveFlags *pResolveFlags)
{
	Assert(IsAnythingLocked());
	UnlockColorTarget(index);

	if (pResolveFlags && pResolveFlags->NeedResolve)
		UnlockDepthTarget();

	const sce::Gnm::RenderTarget *const backBuffer = sm_BackBuffer->GetColorTarget();
	s_CurColors[index] = backBuffer;
	gfxc.setRenderTarget(index, backBuffer);
	s_CurDepth = sm_BackBufferDepth->GetDepthTarget();
	gfxc.setDepthRenderTarget(s_CurDepth);

	s_CurSupersampleFrequency = backBuffer->getNumSamples();
	gfxc.setupScreenViewport(0, 0, grcDevice::GetGlobalWindow().uWidth, grcDevice::GetGlobalWindow().uHeight, s_ZScale, s_ZOffset);
	grcDevice::SetSize(grcDevice::GetGlobalWindow().uWidth, grcDevice::GetGlobalWindow().uHeight);
	
	Assert( s_CurDepth->getNumFragments() == backBuffer->getNumFragments() );
	grcDevice::SetAACount( 1<<backBuffer->getNumSamples(), 1<<backBuffer->getNumFragments(), 1<<backBuffer->getNumFragments() );

#if SUPPORT_RENDERTARGET_DUMP
	if (m_LockDepthRenderTarget && grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
	{
		GRCDEVICE.CpuWaitOnGpuIdle();
		m_LockDepthRenderTarget->SaveTarget();
	}
#endif // SUPPORT_RENDERTARGET_DUMP
#if !MULTIPLE_RENDER_THREADS
	m_LockColorRenderTargets[0] = sm_BackBuffer;
	m_LockDepthRenderTarget = sm_BackBufferDepth;
#endif // !MULTIPLE_RENDER_THREADS
}

#if SUPPORT_RENDERTARGET_DUMP
void grcTextureFactoryGNM::UnlockBackBuffer()
{
	if (grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
	{
		sm_BackBuffer->SaveTarget();
	}
}
#endif //SUPPORT_RENDERTARGET_DUMP

#if __ASSERT
void grcTextureFactoryGNM::CheckOutputFormat(uint8_t outputFormat)
{
	if (!s_CurColors[0])
		return;

	const sce::Gnm::PsTargetOutputMode sourceMode = static_cast<sce::Gnm::PsTargetOutputMode>(outputFormat);
	const sce::Gnm::DataFormat targetFormat = (s_CurLockSingleTarget ? s_CurLockSingleTarget : s_CurColors[0])->getDataFormat();
	sce::Gnm::RenderTargetChannelType targetChannelType = sce::Gnm::kRenderTargetChannelTypeFloat;
	AssertVerify(targetFormat.getRenderTargetChannelType(&targetChannelType));
	const uint32_t targetElements = targetFormat.getNumComponents();
	const uint32_t targetBits = targetFormat.getBitsPerElement() / targetElements;

	switch (sourceMode)
	{
	case sce::Gnm::kPsTargetOutputModeA16B16G16R16Uint:
		grcAssertf(targetChannelType == sce::Gnm::kRenderTargetChannelTypeUInt,
			"PS outputs 16bit Uint, but the actual target has type %u", targetChannelType);
		break;
	case sce::Gnm::kPsTargetOutputModeA16B16G16R16Sint:
		grcAssertf(targetChannelType == sce::Gnm::kRenderTargetChannelTypeSInt,
			"PS outputs 16bit Sint, but the actual target has type %u", targetChannelType);
		break;
	case sce::Gnm::kPsTargetOutputModeA16B16G16R16Unorm:
		grcAssertf(targetChannelType == sce::Gnm::kRenderTargetChannelTypeUNorm,
			"PS outputs 16bit Unorm, but the actual target has type %u", targetChannelType);
		break;
	case sce::Gnm::kPsTargetOutputModeA16B16G16R16Snorm:
		grcAssertf(targetChannelType == sce::Gnm::kRenderTargetChannelTypeSNorm,
			"PS outputs 16bit Snorm, but the actual target has type %u", targetChannelType);
		break;
	case sce::Gnm::kPsTargetOutputModeR32:
		grcAssertf(targetBits == 32 && targetElements == 1, "PS output mode is 32bpp, but the actual target is %ux %ubpp", targetElements, targetBits);
		break;
	case sce::Gnm::kPsTargetOutputModeG32R32:
	case sce::Gnm::kPsTargetOutputModeA32R32:
		grcAssertf(targetBits == 32 && targetElements == 2, "PS output mode is 32bpp, but the actual target is %ux %ubpp", targetElements, targetBits);
		break;
	case sce::Gnm::kPsTargetOutputModeA32B32G32R32:
		grcAssertf(targetBits == 32 && targetElements == 4, "PS output mode is 32bpp, but the actual target is %ux %ubpp", targetElements, targetBits);
		break;
	default:
		//shouldn't we force kPsTargetOutputModeA16B16G16R16Unorm by default?
		grcAssertf((targetBits <= 16 && (
			targetChannelType == sce::Gnm::kRenderTargetChannelTypeFloat	||
			targetChannelType == sce::Gnm::kRenderTargetChannelTypeSrgb)	||
					targetBits <= 10 && (
			targetChannelType == sce::Gnm::kRenderTargetChannelTypeUNorm	||
			targetChannelType == sce::Gnm::kRenderTargetChannelTypeSNorm)),
			"PS does not specify the output format (%u), but the target is not standard (%ubpp)", sourceMode, targetBits);
	}
}
#endif // __ASSERT

void grcTextureFactoryGNM::LockMRT(const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock))
{
	Assert(!IsAnythingLocked());
	
	sce::Gnm::NumSamples	numSamples		= sce::Gnm::kNumSamples1;
	sce::Gnm::NumFragments	numFragments	= sce::Gnm::kNumFragments1;
	sce::Gnm::NumSamples	numIterations	= sce::Gnm::kNumSamples1;
	s_CmaskDirty = 0;
	for (int i=0; i<grcmrtColorCount; i++)
	{
		const grcRenderTargetGNM *const pColor = static_cast<const grcRenderTargetGNM*>(color[i]);
		const sce::Gnm::RenderTarget *const target = pColor?pColor->GetColorTarget() : NULL;
		gfxc.setRenderTarget(i, target);
		s_CurColors[i] = target;
		if (target)
		{
			s_CmaskDirty |= (const_cast<grcRenderTargetGNM*>(pColor)->PopCmaskDirty() ? 1:0) << i;
			Assert( !pColor->GetMSAA().NeedFmask() || (target->getFmaskAddress() && target->getCmaskAddress()) );
			Assert( !numFragments || numFragments==target->getNumFragments() );
			Assert( !numIterations || numIterations==static_cast<sce::Gnm::NumSamples>(target->getNumFragments()) );
			if (numSamples < target->getNumSamples())
				numSamples = target->getNumSamples();
			numFragments = target->getNumFragments();
			numIterations = pColor->GetCoverageData().superSample;
			const_cast<grcRenderTargetGNM*>(pColor)->DebugSetUnresolved();
		}
	}
	const grcRenderTargetGNM *const pDepth = static_cast<const grcRenderTargetGNM*>(depth);
	const sce::Gnm::DepthRenderTarget *const depthTarget = pDepth? pDepth->GetDepthTarget() : NULL;
	gfxc.setDepthRenderTarget(depthTarget);
	s_CurDepth = depthTarget;
	if (depthTarget)
	{
		Assert( !numFragments || numFragments == depthTarget->getNumFragments() );
		const_cast<grcRenderTargetGNM*>(pDepth)->DebugSetUnresolved();
	}

	s_CurSupersampleFrequency = numIterations;
	int width = color[0]? color[0]->GetWidth() : depth->GetWidth();
	int height = color[0]? color[0]->GetHeight() : depth->GetHeight();
	gfxc.setupScreenViewport(0, 0, width,height , s_ZScale, s_ZOffset);
	grcDevice::SetSize(width, height);
	grcDevice::SetAACount( 1<<numSamples, 1<<numFragments, 1<<numIterations );

	// disable scissor
	gfxc.setWindowScissor(0,0,16383,16383,sce::Gnm::kWindowOffsetDisable);

#if !MULTIPLE_RENDER_THREADS
	// Store active targets
	memcpy(m_LockColorRenderTargets, color, sizeof(void*) * grcmrtColorCount);
	m_LockDepthRenderTarget = depth;
#endif
#if __ASSERT
	s_CurLockSingleTarget = NULL;
#endif
}

void grcTextureFactoryGNM::UnlockMRT(const grcResolveFlagsMrt* resolveFlags)
{
	UnlockRenderTarget(0, NULL);
	for (int i=1; i<grcmrtColorCount; i++) {
		UnlockColorTarget(i);
		gfxc.setRenderTarget(i, NULL);
	}
}

#if __ASSERT
void grcTextureFactoryGNM::SetLockSingleTarget(const sce::Gnm::RenderTarget *target)
{
	s_CurLockSingleTarget = target;
}
#endif

void grcTextureFactoryGNM::RelockRenderTargets()
{
	sce::Gnm::NumFragments	numFragments	= s_CurDepth ? s_CurDepth->getNumFragments() : sce::Gnm::kNumFragments1;
	sce::Gnm::NumSamples	numSamples		= static_cast<sce::Gnm::NumSamples>(numFragments);
	uint32_t width = s_CurDepth ? s_CurDepth->getWidth() : 0;
	uint32_t height = s_CurDepth ? s_CurDepth->getHeight() : 0;
	for (int i=0; i<grcmrtColorCount; i++)
	{
		const sce::Gnm::RenderTarget *col = s_CurColors[i];
		gfxc.setRenderTarget(i, col);
		if(col && !i)
		{
			width = col->getWidth();
			height = col->getHeight();
			numSamples		= col->getNumSamples();
			numFragments	= col->getNumFragments();
		}
	}
	gfxc.setDepthRenderTarget( s_CurDepth );

	if (width && height)
	{
		gfxc.setupScreenViewport(0, 0, width, height, s_ZScale, s_ZOffset);
		grcDevice::SetSize(width, height);
		grcDevice::SetAACount( 1<<numSamples, 1<<numFragments, 1<<s_CurSupersampleFrequency );
	}
#if __ASSERT
	s_CurLockSingleTarget = NULL;
#endif
}

bool grcTextureFactoryGNM::IsTargetLocked(const grcRenderTarget *target) const
{
	const grcRenderTargetGNM *gnmTarget = static_cast<const grcRenderTargetGNM*>(target);
	if (gnmTarget->GetColorTarget())
	{
		for (u32 i=0; i<grcmrtColorCount; ++i)
		{
			if (gnmTarget->GetColorTarget() == s_CurColors[i])
				return true;
		}
		return false;
	}else
	{
		return gnmTarget->GetDepthTarget() == s_CurDepth;
	}
}

#if DEVICE_EQAA
void grcTextureFactoryGNM::FinishRendering()
{
	GRCDEVICE.FlushCaches( grcDevice::CACHE_COLOR_META );
	
	for (int i=0; i<grcmrtColorCount; i++)
	{
		GRCDEVICE.FinishRendering( s_CurColors[i], s_CmaskDirty & (1<<i) );
# if SUPPORT_RENDERTARGET_DUMP
		if (m_LockColorRenderTargets[i] && grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
			m_LockColorRenderTargets[i]->SaveTarget();
# endif // SUPPORT_RENDERTARGET_DUMP
	}
	
	//GRCDEVICE.EliminateFastClear();
	s_CmaskDirty = 0;
}

#endif	//DEVICE_EQAA


u32 grcTextureFactoryGNM::GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory)
{
	// TODO -- this code does not consider cubemaps, volumes or arrays .. does it matter?
	int w           = width;
	int h           = height;
	int mips        = mipLevels;
	int layers      = numSlices+1;
	grcImage::Format imgFormat	= (grcImage::Format)format;
	int bpp         = grcImage::GetFormatBitsPerPixel(imgFormat);
	int blockSize   = grcImage::IsFormatDXTBlockCompressed(imgFormat) ? 4 : 1;
	int sizeInBytes = 0;

	while (mips > 0)
	{
		sizeInBytes += (w*h*layers*bpp)/8;

		w = Max<int>(blockSize, w/2);
		h = Max<int>(blockSize, h/2);

		mips--;
	}

	return (u32)sizeInBytes;
}

void grcTextureFactoryGNM::PlaceTexture(class datResource &rsc,grcTexture &tex) {
	switch (tex.GetResourceType()) {
	case grcTexture::NORMAL: ::new (&tex) grcTextureGNM(rsc); break;
	case grcTexture::RENDERTARGET: AssertMsg(0 , "unsafe to reference a rendertarget"); break;
	case grcTexture::REFERENCE: ::new (&tex) grcTextureReference(rsc); break;
	default: Quitf("Bad resource type %d in grcTextureFactoryGNM::PlaceTexture",tex.GetResourceType());
	}
}

grcTextureFormat grcTextureFactoryGNM::TranslateToRageFormat(u32 uTextureFormat)
{
	CompileTimeAssert(sizeof(u32) == sizeof(sce::Gnm::DataFormat));
	sce::Gnm::DataFormat oFormat;
	oFormat.m_asInt = uTextureFormat;

	const sce::Gnm::DataFormat sce__Gnm__kDataFormatX24G8Sint = {{{sce::Gnm::kSurfaceFormatX24_8_32, sce::Gnm::kTextureChannelTypeSInt, sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelY, sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelConstant0}}};
	const sce::Gnm::DataFormat sce__Gnm__kDataFormatD32G8Float = {{{sce::Gnm::kSurfaceFormatX24_8_32, sce::Gnm::kTextureChannelTypeFloat, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelY, sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelConstant0}}};
	const sce::Gnm::DataFormat sce__Gnm__kDataFormatX32G8Sint = {{{sce::Gnm::kSurfaceFormatX24_8_32, sce::Gnm::kTextureChannelTypeSInt, sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelY, sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelConstant0}}};

	if (oFormat.m_asInt == sce::Gnm::kDataFormatR32G32B32A32Float	.m_asInt) return grctfA32B32G32R32F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatB5G6R5Unorm			.m_asInt) return grctfR5G6B5;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR8G8B8A8Unorm		.m_asInt) return grctfA8B8G8R8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16Float			.m_asInt) return grctfR16F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR32Float			.m_asInt) return grctfR32F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR10G10B10A2Unorm	.m_asInt) return grctfA2B10G10R10;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16G16B16A16Float	.m_asInt) return grctfA16B16G16R16F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16G16Unorm			.m_asInt) return grctfG16R16;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16G16Float			.m_asInt) return grctfG16R16F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16G16B16A16Unorm	.m_asInt) return grctfA16B16G16R16;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR8Unorm				.m_asInt) return grctfL8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16Unorm			.m_asInt) return grctfL16;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR8G8Unorm			.m_asInt) return grctfG8R8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatB5G5R5A1Unorm		.m_asInt) return grctfA1R5G5B5;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatB4G4R4A4Unorm		.m_asInt) return grctfA4R4G4B4;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR32G32Float			.m_asInt) return grctfG32R32F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR16Float			.m_asInt) return grctfR16F;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatB8G8R8A8Unorm       .m_asInt) return grctfA8R8G8B8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatB8G8R8X8Unorm		.m_asInt) return grctfX8R8G8B8;
	if (oFormat.m_asInt == sce__Gnm__kDataFormatX24G8Sint			.m_asInt) return grctfX24G8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatA8Unorm				.m_asInt) return grctfA8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR11G11B10Float		.m_asInt) return grctfR11G11B10F;
	if (oFormat.m_asInt == sce__Gnm__kDataFormatD32G8Float			.m_asInt) return grctfD32FS8;
	if (oFormat.m_asInt == sce__Gnm__kDataFormatX32G8Sint			.m_asInt) return grctfX32S8;
	if (oFormat.m_asInt == sce::Gnm::kDataFormatR8G8B8A8Snorm       .m_asInt) return grctfA8B8G8R8_SNORM;

	Warningf("Orbis: No Rage Conversion for Texture Format %u", oFormat.m_asInt);
	return grctfNone;
}

}	// namespace rage

#endif
