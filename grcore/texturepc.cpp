//
// grcore/texturepc.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"
#include	"grcore/d3dwrapper.h"

#if	RSG_PC || RSG_DURANGO

#include	"device.h"
#include	"image.h"
#include	"viewport.h"

#include	"system/memory.h"
#include	"system/param.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"
#include	"vector/matrix34.h"
#include	"system/param.h"
#include	"system/xtl.h"
#include	"system/d3d11.h"

#include	"grcore/texture.h"
#include	"grcore/texturepc.h"
#include	"grcore/texture_d3d9.h"
#include	"grcore/texture_d3d11.h"
#include	"grcore/texturefactory_d3d11.h"
#include	"grcore/wrapper_d3d.h"

#if __TOOL
namespace rage
{
	XPARAM(noquits);
}
#endif


namespace rage {

//=============================================================================
// grcTextureFactory
//=============================================================================
grcTextureFactory	*grcTextureFactory::CreatePagedTextureFactory	(bool bMakeActive)
{
	grcTextureFactory	*pFactory = NULL;

#if __D3D9
	pFactory = rage_new	grcTextureFactoryDX9;
#elif __D3D11
	pFactory = rage_new	grcTextureFactoryDX11;
#endif

	if	(bMakeActive)
		sm_Instance = pFactory;

	return(pFactory);
}

u32	grcTexturePC::m_uQuality = grcTexturePC::HIGH;
u32 __THREAD grcTexturePC::m_uQualityOverride = grcTexturePC::INVALID;

// #if !__RESOURCECOMPILER		// In resourcecompiler builds, we get a version from texturegcm... sigh.

void grcRenderTargetPoolEntry::FreePoolMemory() 
{
}

void grcRenderTargetPoolEntry::AllocatePoolMemory(u32 , bool , int , void * )
{
}

void grcRenderTargetPoolEntry::InitializeMemory(const grcRTPoolCreateParams & )	
{
}

// #endif

//=============================================================================
// grcTextureFactoryPC
//=============================================================================

#if RSG_PC

grcTexturePC* grcTexturePC::sm_First;
sysCriticalSectionToken s_TextureList;

u64 grcRenderTargetPC::sm_TotalMemory = 0;
u64 grcRenderTargetPC::sm_TotalStereoMemory = 0;
atFixedArray<grcRenderTargetPC *, grcTextureFactoryPC::MAX_RENDERTARGETS>	grcTextureFactoryPC::sm_ActiveTargets;
static int sCallbacksRegistered;

GUID TextureBackPointerGuid = { // 637c4f8d-7724-436d-b2d1-49125b3a2a25
	0x637c4f8d,
	0x7724,
	0x436d,
	{0xb2, 0xd1, 0x49, 0x12, 0x5b, 0x3a, 0x2a, 0x25}
};

#define D3DFMT_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

grcTextureFactoryPC::grcTextureFactoryPC() 
{
#if RSG_PC && !__D3D11
	// Register the callbacks
	if ( ++sCallbacksRegistered == 1) {
		Functor0 resetCb = MakeFunctor(grcTextureFactoryPC::DeviceReset);
		Functor0 lostCb = MakeFunctor(grcTextureFactoryPC::DeviceLost);

		GRCDEVICE.RegisterDeviceLostCallbacks(lostCb, resetCb);
		sCallbacksRegistered = true;
	}
#endif
}

grcTextureFactoryPC::~grcTextureFactoryPC() {
#if RSG_PC
	--sCallbacksRegistered;
#endif

	while(!sm_ActiveTargets.empty())
	{
		grcRenderTargetPC *last = sm_ActiveTargets[sm_ActiveTargets.GetCount()-1];
		Warningf("Render Target %s still allocated", last->GetName());
		delete last;
	}

	sm_ActiveTargets.Reset();
}

u32 grcTextureFactoryPC::GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool UNUSED_PARAM(bIsCubeMap), bool UNUSED_PARAM(bIsLinear), bool UNUSED_PARAM(bLocalMemory))
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


const grcRenderTarget *grcTextureFactoryPC::GetBackBuffer(bool realize) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryPC::GetBackBuffer(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryPC::GetFrontBuffer(bool nextBuffer) const {
	return GetFrontBuffer(nextBuffer);
}

grcRenderTarget *grcTextureFactoryPC::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryPC::GetFrontBufferDepth(bool realize) const {
	return GetFrontBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryPC::GetFrontBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryPC::GetBackBufferDepth(bool realize) const {
	return GetBackBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryPC::GetBackBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

void grcTextureFactoryPC::RemoveRenderTarget(grcRenderTargetPC *tgt) {
	int idx = tgt->GetSlotId();
	Assert("Can't remove a target twice" && sm_ActiveTargets[idx] == tgt);
	grcRenderTargetPC *last = sm_ActiveTargets[sm_ActiveTargets.GetCount()-1];
	sm_ActiveTargets[idx] = last;
	last->SetSlotId(idx);
	sm_ActiveTargets.Pop();
}

void grcTextureFactoryPC::DeviceLost() {
	for (int i = 0; i < sm_ActiveTargets.GetCount(); ++i) {
		sm_ActiveTargets[i]->DeviceLost();
	}

	if (grcTextureFactory::HasInstance())
		((grcTextureFactoryPC*)&grcTextureFactory::GetInstance())->Lost();

	grcTexturePC *t = grcTexturePC::sm_First;
	while (t) {
		t->DeviceLost();
		t = t->m_Next;
	}

#if USE_RESOURCE_CACHE
	grcResourceCache::DeviceLost();
#endif // USE_RESOURCE_CACHE
}

void grcTextureFactoryPC::DeviceReset() {
	for (int i = 0; i < sm_ActiveTargets.GetCount(); ++i) {
		sm_ActiveTargets[i]->DeviceReset();
	}

	grcTexturePC *t = grcTexturePC::sm_First;
	while (t) {
		t->DeviceReset();
		t = t->m_Next;
	}

	((grcTextureFactoryPC*)&grcTextureFactory::GetInstance())->Reset();

#if USE_RESOURCE_CACHE
	grcResourceCache::DeviceReset();
#endif // USE_RESOURCE_CACHE
}

void grcTextureFactoryPC::ReportUnusedTextures()
{
	// grcTexturePC::Lock(); // PC TODO - Missing safety lock on resource list in textures

	Displayf("Loaded but Unused Textures");
	grcTexturePC *t = grcTexturePC::sm_First;
	while (t) {
		if (t->GetTexturePtr() == NULL)
		{
			Displayf("Texture %s Width %d Height %d", t->GetName(), t->GetWidth(), t->GetHeight());
		}
		t = t->m_Next;
	}

	// grcTexturePC::Unlock(); // PC TODO
}

//=============================================================================
// grcTexturePC
//=============================================================================

grcTexturePC::grcTexturePC(datResource &rsc) : grcTexture(rsc) {
#if !__RESOURCECOMPILER
	if (datResource_IsDefragmentation)
	{
		SYS_CS_SYNC(s_TextureList);
		// Repair linked list nodes (including the head!)
		rsc.PointerFixup(m_Previous);
		rsc.PointerFixup(m_Next);
		if (m_Previous)
			m_Previous->m_Next = this;
		else
			sm_First = this;
		if (m_Next)
			m_Next->m_Previous = this;
	}
	else
#endif

	{
		SYS_CS_SYNC(s_TextureList);
		m_Previous = NULL;
		m_Next = sm_First;
		sm_First = this;
#if !__RESOURCECOMPILER
		if (m_Next != NULL)
			m_Next->m_Previous = this;
#endif // !__RESOURCECOMPILER
	}

	rsc.PointerFixup(m_Name);
}

grcTexturePC::grcTexturePC(grcTextureFactory::TextureCreateParams *params) : grcTexture(params ? (u8)(params->Type): 0)
{
	m_Previous = NULL;
#if !__RESOURCECOMPILER
	SYS_CS_SYNC(s_TextureList);
	m_Next = sm_First;
	sm_First = this;

	if (m_Next != NULL)
	{
		m_Next->m_Previous = this;
	}
#else
	m_Next = 0;		// Will be repaired in rsc ctor
#endif // !__RESOURCECOMPILER
}


grcTexture*	grcTexturePC::GetPrivateData(const grcTextureObject *pTexObj)
{
	rage::grcTexture *pTexture = NULL;
	UINT sizeofData = sizeof(pTexture);

#if __D3D9
		if (pTexObj)
			CHECK_HRESULT(((IDirect3DTexture9*)pTexObj)->GetPrivateData(TextureBackPointerGuid,&pTexture,(DWORD*)&sizeofData));
#elif __D3D11
		if (pTexObj)
			CHECK_HRESULT(((ID3D11DeviceChild*)pTexObj)->GetPrivateData(TextureBackPointerGuid, &sizeofData, &pTexture));
#endif

	if (!pTexture)
		grcWarningf("GetPrivateData failed?  obj=%p",pTexObj);
	return pTexture;
}


bool grcTexturePC::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr()); 
#endif // __FINAL
}

u32 grcTexturePC::GetMipLevelScaleQuality(u32 uImageType, u32 uWidth, u32 uHeight, u32 uMipCount, u32 uFormat)
{
	// TODO -- this code doesn't consider volumes or arrays .. does it matter?
	u32 uMipLevelsDown = HIGH - GetTextureQuality();
	if (uMipLevelsDown == 0)
		return 0;

	if (uMipCount <= 1)
		return 0;

	Assert(uWidth >= 4);
	Assert(uHeight >= 4);

	u32 uScaleQuality = 0;
	while(uMipLevelsDown)
	{
		u32 uMemory = uWidth * uHeight * ((uImageType == grcImage::CUBE) ? 6 : 1) * static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).GetBitsPerPixel(uFormat) / 8; 
		uMemory = (u32)((float)uMemory * ((uMipCount > 1) ? 1.4f : 1.0f));

		if (uMemory < D3DRUNTIME_MEMORYALIGNMENT)
			return uScaleQuality;

		uWidth >>= 1;
		uHeight >>= 1;
		uMipCount--;
		if ((uWidth < 4) || (uHeight < 4) || (uMipCount == 0))
			return uScaleQuality;		

		uScaleQuality++;
		uMipLevelsDown--;
	}
	return uScaleQuality;
}

grcTexturePC::~grcTexturePC	(void)
{
}


void grcTexturePC::UnlinkFromChain() {
	SYS_CS_SYNC(s_TextureList);
	if (sm_First == this)
	{
		sm_First = m_Next;
	}
#if !__RESOURCECOMPILER
	if (m_Next != NULL)
	{
		m_Next->m_Previous = m_Previous;
	}

	if (m_Previous != NULL)
	{
		m_Previous->m_Next = m_Next;
	}
#endif // !__RESOURCECOMPILER
}

const grcTextureObject* grcTexturePC::GetTexturePtr() const
{
	return (const grcTextureObject*)m_CachedTexturePtr;
}


grcTextureObject* grcTexturePC::GetTexturePtr()
{
	return (grcTextureObject*)m_CachedTexturePtr;
}


grcDeviceView* grcTexturePC::GetTextureView()
{
	return m_pShaderResourceView;
}


template<class T>
inline void Swap(T& left, T& right)
{
	T temp = left; left = right; right = temp;
}

void grcTexturePC::HDOverrideSwap(grcTexturePC* alternateTexture)
{
	Swap(alternateTexture->m_Width, m_Width);
	Swap(alternateTexture->m_Height, m_Height);
	Swap(alternateTexture->m_Depth, m_Depth);
	Swap(alternateTexture->m_nMipStride, m_nMipStride);
	Swap(alternateTexture->m_nFormat, m_nFormat);
	Swap(alternateTexture->m_ImageType, m_ImageType);
	Swap(alternateTexture->m_nMipCount, m_nMipCount);
	Swap(alternateTexture->m_CutMipLevels, m_CutMipLevels);
#if __PAGING
	Swap(alternateTexture->m_BackingStore, m_BackingStore);
#endif
	Swap(alternateTexture->m_CachedTexturePtr, m_CachedTexturePtr);
	Swap(alternateTexture->m_pShaderResourceView, m_pShaderResourceView);
#if !__D3D11
	Swap(alternateTexture->m_StagingTexture, m_StagingTexture);
#else
	Swap(alternateTexture->m_pExtraData, m_pExtraData);
#endif
}

//=============================================================================
// grcRenderTargetPC
//=============================================================================

s64	grcRenderTargetPC::GetTotalMemory()
{ 
	return sm_TotalMemory + GetTotalStereoMemory();
}

s64	grcRenderTargetPC::GetTotalStereoMemory()
{ 
#if RSG_DURANGO
	// Pretty sure it will support Stereo, no clue on specifics yet though.
	return 0;
#else
	return (GRCDEVICE.SupportsFeature(AUTOSTEREO) && (GRCDEVICE.GetGPUCount() <= 1)) ? sm_TotalStereoMemory : 0;
#endif
}


grcRenderTargetPC::grcRenderTargetPC()
:	m_LockableTexture( 0 )
{
	// Empty construct for DX10/DX11 constructor methods
}


grcRenderTargetPC::~grcRenderTargetPC() 
{
	// Empty DX9/DX10/DX11 destructors should take care of everything
}


int grcRenderTargetPC::GetBitsPerPixel() const
{
	return m_BitsPerPix;
}


bool grcRenderTargetPC::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr()); 
#endif // __FINAL
}

grcTextureObject*	grcRenderTargetPC::GetTexturePtr()
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

const grcTextureObject*	grcRenderTargetPC::GetTexturePtr() const 
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

grcTexture::ChannelBits grcRenderTargetPC::FindUsedChannels() const
{
	return FindUsedChannelsFromRageFormat(GetFormat());
}

//=============================================================================
// Structure Declarations
//=============================================================================
#if __DECLARESTRUCT
void grcTexturePC::DeclareStruct(datTypeStruct &s) {
	grcTexture::DeclareStruct(s);
	STRUCT_BEGIN(grcTexturePC);
	STRUCT_FIELD(m_ExtraFlags);
#if __64BIT
	STRUCT_IGNORE(m_ExtraFlagsPadding);
#endif //__64BIT
	STRUCT_FIELD(m_Width);
	STRUCT_FIELD(m_Height);
	STRUCT_FIELD(m_Depth);
	STRUCT_FIELD(m_nMipStride);
	STRUCT_FIELD(m_nFormat);
	STRUCT_FIELD(m_ImageType);
	STRUCT_FIELD(m_nMipCount);
	STRUCT_FIELD(m_CutMipLevels);
#if __D3D11
	STRUCT_FIELD(m_IsSRGB_Byte);
#else
	STRUCT_FIELD(m_IsSRGB);
#endif
	STRUCT_FIELD_VP(m_Next);
	STRUCT_FIELD_VP(m_Previous);
#if __PAGING
	STRUCT_FIELD(m_BackingStore);
#endif
	STRUCT_IGNORE(m_pShaderResourceView);
#if __D3D11
	STRUCT_IGNORE(m_pExtraData);
#else
	STRUCT_IGNORE(m_StagingTexture);
#endif
	STRUCT_END();
}
#endif //__DECLARESTRUCT

#endif // RSG_PC

}	// namespace rage

#endif // RSG_PC || RGG_DURANGO
