#include	"rendertarget_durango.h"
#include	"texturefactory_d3d11.h"

#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"
#include	"grcore/d3dwrapper.h"
#include	"grcore/gfxcontext.h"

#if	RSG_DURANGO

#include	"system/d3d11.h"
#include	<xg.h>

#include	"device.h"
#include	"image.h"
#include	"viewport.h"

#include	"system/memory.h"
#include	"system/param.h"
#include "grprofile/pix.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"
#include	"vector/matrix34.h"
#include	"system/param.h"
#include	"system/xtl.h"
#include	<wbemidl.h>

#include	"wrapper_d3d.h"

XPARAM(usetypedformat);

#define _LARGE_PAGE_SIZE (64*1024)	// can't find an XDK definition of this


namespace rage 
{

extern GUID TextureBackPointerGuid;

const DWORD dwProtect = PAGE_READONLY; 

#if DEVICE_EQAA
CoverageData::CoverageData()
: texture(NULL)
, donor(NULL)
, compressionEnabled(false)
, manualDecoding(true)
, resolveType(ResolveSW_Simple)
, supersampleFrequency(0)
{}
#endif // DEVICE_EQAA

/*======================================================================================================================================*/
/* class grcRenderTargetDurango.																										*/
/*======================================================================================================================================*/


grcRenderTargetDurango::grcRenderTargetDurango(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params, grcRenderTargetDurango * origTarget) 
: grcRenderTarget(), m_Multisample(0)
{
	m_Name = StringDuplicate(name);
	m_Texture = 0;
	m_CreatedFromTextureObject = false;
	m_CreatedFromPreAllcatedMem = false;

	if (origTarget)
	{
		if (params) // limited support for now, SVG will support full rendertarget pools again
		{			
			Assertf(params->ESRAMPhase==0 && params->Multisample==grcDevice::MSAA_None && params->MipLevels==1, "overlpping rendertargets are only minimally supported");	
		}
		m_VirtualAddress = origTarget->m_VirtualAddress;
		m_VirtualSize = origTarget->m_VirtualSize;
		m_CreatedFromPreAllcatedMem = true;
	}

	m_pShaderResourceView = NULL;
	m_pUnorderedAccessView = NULL;

	m_MipLevels = 1;
	m_ArraySize = 1;
	m_PerArraySliceRTV = 0;

	m_LockableTexture = 0;

	ReCreate(type, width, height, bpp, params);
}


grcRenderTargetDurango::grcRenderTargetDurango(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params ) 
: grcRenderTarget(), m_Multisample(0)
{
	CreateFromTextureObject( name, pTexture, eShaderViewFormat, depthStencilReadOnly, params );
}


grcRenderTargetDurango::grcRenderTargetDurango(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly,grcTextureFactory::CreateParams *params ) 
: grcRenderTarget(), m_Multisample(0)
{
	CreateFromTextureObject( name, pTexture, depthStencilReadOnly, params );
}


grcRenderTargetDurango::~grcRenderTargetDurango() 
{
	grcTextureFactory::UnregisterRenderTarget(this);

	DestroyInternalData();
}


void grcRenderTargetDurango::DestroyInternalData()
{
	DestroySRVAndTargetViews();

	if (!m_CreatedFromTextureObject)
	{
		GRCDEVICE.DeleteTexture(m_Texture);

		if (m_VirtualAddress)
		{
			if (!m_CreatedFromPreAllcatedMem)
			{
				if (m_ESRAMVirtualAddress)
				{
					ESRAMManager::Free(m_VirtualAddress, m_VirtualSize, m_ESRAMVirtualAddress, m_ESRAMNumPages);
				}
				else
				{
					sysMemPhysicalFree(m_VirtualAddress);
				}
			}
			m_VirtualAddress = NULL;
		}
	}

	if (m_LockableTexture)
	{
		GRCDEVICE.DeleteTexture(m_LockableTexture);
		m_LockableTexture = NULL;

		if (m_StagingVirtualAddress)
		{
			sysMemPhysicalFree(m_StagingVirtualAddress);
			m_StagingVirtualAddress = NULL;
		}

	}
	m_CachedTexturePtr = NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
grcTexture::ChannelBits grcRenderTargetDurango::FindUsedChannels() const
{
	return FindUsedChannelsFromRageFormat(GetFormat());
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if DYNAMIC_ESRAM
grcTextureObject*	grcRenderTargetDurango::GetTextureObject()
{
	return (grcTextureObject*)m_Texture;
}

bool grcRenderTargetDurango::AltSwapTest() const
{
	if (m_AltTarget == NULL)
		return false;
	if (m_UseAltTestFunc)
	{
		return (*m_UseAltTestFunc)();
	}
	else
	{
		return m_UseAltTarget;
	}
}
#endif

grcTextureObject*	grcRenderTargetDurango::GetTexturePtr()
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return m_AltTarget->GetTexturePtr();
	}
#endif
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return (grcTextureObject*)m_Texture;
}


const grcTextureObject*	grcRenderTargetDurango::GetTexturePtr() const 
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return m_AltTarget->GetTexturePtr();
	}
#endif
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return (grcTextureObject*)m_Texture;
}


grcDeviceView* grcRenderTargetDurango::GetTextureView()
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return m_AltTarget->GetTextureView();
	}
#endif
#if !__FINAL
	if (m_pShaderResourceView == NULL)
		Warningf("Render Target %s Format can not be sampled as texture", m_Name);
#endif // __FINAL

	return m_pShaderResourceView;
}

const grcDeviceView* grcRenderTargetDurango::GetTargetView(u32 uMip, u32 uLayer)
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return static_cast<grcRenderTargetDurango*>(m_AltTarget)->GetTargetView(uMip, uLayer);
	}
#endif
	//AssertVerify(SetTargetView(uMip, uLayer));
	Assert(uMip < m_MipLevels);
	Assertf(m_TargetViews[uLayer*m_MipLevels+uMip], "GetTargetView with layer %d and mip %d failed", uLayer, uMip);
	return m_TargetViews[uLayer*m_MipLevels+uMip];
}


const grcDeviceView* grcRenderTargetDurango::GetUnorderedAccessView() const
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return static_cast<grcRenderTargetDurango*>(m_AltTarget)->GetUnorderedAccessView();
	}
#endif
#if !__FINAL
	if (m_pUnorderedAccessView == NULL)
		Warningf("Render Target Format can not be used as UAV");
#endif // __FINAL

	return m_pUnorderedAccessView;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_TRACK_MSAA_RESOLVES
bool grcRenderTargetDurango::HasBeenResolvedTo(grcRenderTarget* resolveTo)
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return  static_cast<grcRenderTargetDurango*>(m_AltTarget)->HasBeenResolvedTo(resolveTo);
	}
#endif
	Assert(resolveTo);
	ASSERT_ONLY(grcTextureObject *const destTexturePtr = resolveTo->GetTexturePtr();)

	return m_DebugLastResolvedObject == destTexturePtr;
}
#endif

void grcRenderTargetDurango::Resolve(grcRenderTarget* resolveTo, int destSliceIndex)
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return  static_cast<grcRenderTargetDurango*>(m_AltTarget)->Resolve(resolveTo, destSliceIndex);
	}
#endif
	Assert(resolveTo);
	ASSERT_ONLY(grcTextureObject *const destTexturePtr = resolveTo->GetTexturePtr();)

#if DEBUG_TRACK_MSAA_RESOLVES
	Assertf(m_DebugLastResolvedObject != destTexturePtr, "Redundant resolve");
#endif

	Assert(GetTexturePtr() != destTexturePtr);
	GRCDEVICE.ResolveMsaaBuffer( static_cast<grcRenderTargetDurango*>(resolveTo), this, destSliceIndex );

#if DEBUG_TRACK_MSAA_RESOLVES
	if (!static_cast<grcTextureFactoryDX11&>(grcTextureFactory::GetInstance()).IsTargetLocked(this))
		m_DebugLastResolvedObject = destTexturePtr;
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool grcRenderTargetDurango::LockRect(int ASSERT_ONLY(layer), int /*ASSERT_ONLY*/(mipLevel),grcTextureLock &lock, u32 uLockFlags ) const
{
	Assert(layer == 0);
	Assertf(m_LockableTexture != 0 || (strcmp(m_Name, "BackBuf0" ) == 0) || (strcmp(m_Name, "BackBuffer")==0), "Render target not created with m_Lockable set");
	AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to Lock render target.");

	lock.MipLevel = mipLevel;
	lock.BitsPerPixel = GetBitsPerPixel();
	lock.Width = m_Width;
	lock.Height = m_Height;
	lock.RawFormat = GetFormat();
	lock.Layer = 0;

	bool result = false;

#if __BANK
	if (m_LockableTexture == 0 && ((strcmp(m_Name, "BackBuf0" ) == 0) || (strcmp(m_Name, "BackBuffer") == 0)))
	{	// For the back buffer, create a staging texture that can be locked.
		D3D11_TEXTURE2D_DESC oStagingDesc;
		ID3D11Texture2D* backBuffer = (ID3D11Texture2D*)m_Texture;
		backBuffer->GetDesc(&oStagingDesc);
		if (oStagingDesc.SampleDesc.Count == 1)
		{
			oStagingDesc.CPUAccessFlags = grcCPURead;
			oStagingDesc.Usage = (D3D11_USAGE)grcUsageStage;
			oStagingDesc.MipLevels = m_MipLevels;
			oStagingDesc.MiscFlags = grcResourceNone;
			oStagingDesc.BindFlags = grcBindNone;
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateTexture2D(&oStagingDesc, NULL, (ID3D11Texture2D**)&m_LockableTexture));

			// Bodgy hack to set the private data
			grcRenderTargetDurango* thisTarget = const_cast<grcRenderTargetDurango*>(this);
			thisTarget->SetPrivateData();
		}
	}
#endif

	if (m_LockableTexture && g_grcCurrentContext && (uLockFlags & grcsRead))
	{
		if (!m_LockableTexUpdated)
			g_grcCurrentContext->CopyResource(m_LockableTexture, m_Texture);

		D3D11_MAPPED_SUBRESOURCE	MappedResource;
		HRESULT						hr = g_grcCurrentContext->Map(m_LockableTexture, mipLevel, D3D11_MAP_READ, 0, &MappedResource);
		CHECK_HRESULT(hr);

		// Calculate the offset.
		lock.Base = MappedResource.pData;
		// Set the pitch.
		lock.Pitch = MappedResource.RowPitch;

		result = (hr == S_OK);
	}

	return result;
}

void grcRenderTargetDurango::UnlockRect(const grcTextureLock &lock) const
{
	Assertf(m_LockableTexture != 0, "Render target not created with m_Lockable set");
	AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to Unlock render target.");

	if (g_grcCurrentContext)
	{
		// Unmap the dynamic texture.
		g_grcCurrentContext->Unmap(m_LockableTexture, lock.MipLevel);
	}
}

u32 grcRenderTargetDurango::GetImageFormat() const
{
	return grcTextureFactoryDX11::GetImageFormatStatic(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool grcRenderTargetDurango::Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex)
{
#if DYNAMIC_ESRAM
	if (AltSwapTest())
	{
		return  m_AltTarget->Copy(pSource, dstSliceIndex, dstMipIndex, srcSliceIndex, srcMipIndex);
	}
#endif
	// PC TODO - DX11 Doesn't support StretchRect.  You must do the work yourself via VS/PS.
	// PC TODO - DX10.0 doesnt support copy of depth stencil textures.
	Assert(pSource != NULL);
	Assert(pSource->GetWidth() == GetWidth());
	Assert(pSource->GetHeight() == GetHeight());

	Update();

	ID3D11Texture2D* poSrc = 0;
	ID3D11Texture2D* poDst = 0;
	if (m_Multisample)
	{
		// Assume that source and target are multisampled if the dest is.
		const grcRenderTargetDurango* pSourceDX11 = static_cast<const grcRenderTargetDurango*>(pSource);
		Assert( m_Multisample == pSourceDX11->m_Multisample );
		poSrc = (ID3D11Texture2D*)(pSourceDX11->GetTexturePtr());
		poDst = reinterpret_cast<ID3D11Texture2D *>(GetTexturePtr());
		DebugSetUnresolved();
	}
	else
	{
		poSrc = (ID3D11Texture2D*)(pSource->GetTexturePtr());
		poDst = reinterpret_cast<ID3D11Texture2D *>(GetTexturePtr());
	}

	Assert(poSrc != NULL);
	Assert(poDst != NULL);
	Assert(poSrc != poDst);

	//Just copy a secified mip level
	if( dstSliceIndex != -1 || dstMipIndex != -1 || srcSliceIndex > 0 || srcMipIndex > 0)
	{
		Assert(dstSliceIndex >= 0 && dstMipIndex >= 0 && srcSliceIndex >= 0 && srcMipIndex >= 0);
		u32 DstX = 0;
		u32 DstY = 0;
		u32 DstZ = 0;
		u32 subDstResource = D3D11CalcSubresource(dstMipIndex, dstSliceIndex, GetMipMapCount());
		u32 subSrcResource = D3D11CalcSubresource(srcMipIndex, srcSliceIndex, GetMipMapCount());
		g_grcCurrentContext->CopySubresourceRegion(poDst, subDstResource, DstX, DstY, DstZ, poSrc, subSrcResource, NULL);
	}
	else
		g_grcCurrentContext->CopyResource(poDst, poSrc);

	grcGfxContext::current()->insertHangCheckpoint();

	return true;
}

void grcRenderTargetDurango::ClearAsync(u32 clearVal0, u32 clearVal1)
{
	GRCDEVICE.ShaderMemset32((char*)m_VirtualAddress + m_PlaneOffsets[0], clearVal0, m_PlaneSizes[0]);
	GRCDEVICE.ShaderMemset32((char*)m_VirtualAddress + m_PlaneOffsets[1], clearVal1, m_PlaneSizes[1]);
}

bool grcRenderTargetDurango::CopyTo(grcImage* pImage, bool /* bInvert*/, u32 uPixelOffset)
{
	if (pImage == NULL)
	{
		return false;
	}

	if ((GetLayerCount() != (int)pImage->GetLayerCount())	||
		(GetWidth() != pImage->GetWidth())					||
		(GetMipMapCount() != (pImage->GetExtraMipCount() + 1)))
	{
		return false;
	}

	if (((GetWidth() * GetHeight()) - (int)uPixelOffset) < (pImage->GetWidth() * pImage->GetHeight()))
	{
		return false;
	}

	bool bResult = false;

	// PC TODO - Implement this functions.  Can only be done on Staging friendly assets.
	return bResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool grcRenderTargetDurango::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr()); 
#endif // __FINAL
}


void grcRenderTargetDurango::Update()
{
	if (m_Texture == NULL)
		CreateSurface();
}


void grcRenderTargetDurango::GenerateMipmaps()
{
	if (g_grcCurrentContext && (m_MipLevels > 1) && m_pShaderResourceView)
		g_grcCurrentContext->GenerateMips( (ID3D11ShaderResourceView*) m_pShaderResourceView );
}


void grcRenderTargetDurango::UpdateLockableTexture(bool bInitUpdate)
{
	if (!bInitUpdate)
	{
		if (g_grcCurrentContext)
			g_grcCurrentContext->CopyResource(m_LockableTexture, m_Texture);

		m_LockableTexUpdated = true;
	}
	else
		m_LockableTexUpdated = false;
}


void grcRenderTargetDurango::UpdateMemoryLocation(const grcTextureObject *pTexture)
{
	if((m_CreatedFromTextureObject) && (m_CachedTexturePtr != pTexture))
	{
		m_CachedTexturePtr = (grcTextureObject *)pTexture;
		m_Texture = (grcTextureObject *)pTexture;
		DestroySRVAndTargetViews();
		CreateSRVAndTargetViews(m_DepthStencilFlags, m_ArraySize);
	}
}

#if DEVICE_EQAA
void grcRenderTargetDurango::SetFragmentMaskDonor(grcRenderTargetDurango *donor, ResolveType resolveType)
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
	m_Coverage.compressionEnabled = true;
	m_Coverage.resolveType = resolveType;
}
#endif // DEVICE_EQAA

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void grcRenderTargetDurango::SetPrivateData()
{
#if !__D3D11_MONO_DRIVER
	char szTempName[256];
	if (m_Texture)
	{
		ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(m_Texture);
		UINT iOldNameSize = sizeof(szTempName)-1;
		deviceChild->GetPrivateData(WKPDID_D3DDebugObjectName, &iOldNameSize, szTempName);
		// Don't reset the name if this object was constructed earlier
		if (!iOldNameSize)
		{
			grcTexture* pTex = this;
			CHECK_HRESULT(deviceChild->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( GetName() )+1,GetName()));
			CHECK_HRESULT(deviceChild->SetPrivateData(TextureBackPointerGuid,sizeof(pTex),&pTex));
		}
	}
	const char* name = GetName();
	if (m_LockableTexture)
	{
		sprintf_s(szTempName, "%s%s", name, "_LockableTexture");
		((ID3D11DeviceChild*)m_LockableTexture)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
	}

	for( int i = 0; i < m_TargetViews.GetCount(); i++ )
	{
		if (!m_TargetViews[i])
			continue;
		sprintf_s(szTempName, "%s - Target[%d][%d]", name, i/m_MipLevels,i%m_MipLevels);
		static_cast<ID3D11DeviceChild*>(m_TargetViews[i])->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
	}

	if (m_pShaderResourceView)
	{
		sprintf_s(szTempName, "%s - Resource", name);
		static_cast<ID3D11DeviceChild*>(m_pShaderResourceView)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
	}
#endif	// !__D3D11_MONO_DRIVER
}


bool grcRenderTargetDurango::SetTextureView(u32)
{
	return true;
}


/*static*/ bool grcRenderTargetDurango::IsReadOnlyFormat(u32 format)
{
	bool readOnly = false;
	switch( format )
	{
	case grctfX24G8:
	case grctfX32S8:
		readOnly = true;
		break;
	default:
		readOnly = false;
	}

	return readOnly;
}


/*static*/ bool grcRenderTargetDurango::IsDepthTypeOrFormat(u32 type, u32 format)
{
	switch (type)
	{
	case grcrtDepthBuffer:
	case grcrtShadowMap:
	case grcrtDepthBufferCubeMap:
		return true;
	default:;
	}

	switch (format)
	{
	case grctfD24S8:
	case grctfD32F:
	case grctfD32FS8:
	case grctfD16:
		return true;
	default:;
	}

	return false;
}


u32 grcRenderTargetDurango::GetDepthTextureFormat()
{
	AssertMsg(false,"This needs serious work");
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void grcRenderTargetDurango::CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params )
{
	ID3D11Texture2D *poTexture = (ID3D11Texture2D*)(pTexture);
	D3D11_TEXTURE2D_DESC oDesc;
	poTexture->GetDesc(&oDesc);

	grcTextureFormat eFormat = static_cast<grcTextureFormat>(grcTextureFactoryDX11::TranslateToRageFormat(oDesc.Format));

	CreateFromTextureObject( name, pTexture, eFormat, depthStencilReadOnly, params );
}


void grcRenderTargetDurango::CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params )
{
	m_Name = StringDuplicate(name);
	m_Texture = (grcDeviceTexture*)pTexture;
	m_LockableTexture = 0;
	m_CreatedFromTextureObject = true;
	m_CreatedFromPreAllcatedMem = false;

	ID3D11Texture2D *poTexture = (ID3D11Texture2D*)(m_Texture);
	D3D11_TEXTURE2D_DESC oDesc;
	poTexture->GetDesc(&oDesc);

	m_pShaderResourceView = NULL;
	m_MipLevels = (u16) oDesc.MipLevels;

	m_Width = (u16)oDesc.Width;
	m_Height = (u16)oDesc.Height;

    if (eShaderViewFormat == grctfDXT1)
	{
        eShaderViewFormat = grctfA16B16G16R16;
        m_Width >>= 2;
        m_Height >>= 2;
	}
	else
	{
		Assert(m_MipLevels == 1);
	}

	Assert(oDesc.Usage == D3D11_USAGE_DEFAULT);
	Assert(oDesc.ArraySize == 1);
#if DEVICE_EQAA
	m_Multisample = grcDevice::MSAAMode( oDesc.SampleDesc );
#else
	m_Multisample = oDesc.SampleDesc.Count > 1 ? grcDevice::MSAAMode(oDesc.SampleDesc.Count) : 0;
	m_MultisampleQuality = (u8)oDesc.SampleDesc.Quality;
#endif
	m_Format = eShaderViewFormat;
	
	Assert(oDesc.ArraySize<=255);
	m_ArraySize = (u8)oDesc.ArraySize;
	m_PerArraySliceRTV = (params != NULL) ? params->PerArraySliceRTV : 0;
	m_Type = ((m_Format == grctfD24S8) || (m_Format == grctfD32F) || (m_Format == grctfX24G8) || (m_Format == grctfD32FS8) || (m_Format == grctfX32S8)) ? grcrtDepthBuffer : grcrtBackBuffer;
	m_CompressedDepth = IsDepthTypeOrFormat(m_Type, m_Format) ? (params ? params->EnableCompression : true) : false;

	CreateSRVAndTargetViews(depthStencilReadOnly, m_ArraySize);

	DebugSetUnresolved();
	SetPrivateData();
	m_CachedTexturePtr = m_Texture;

	m_ESRAMPhase = 0;
	m_ESRAMVirtualAddress = NULL;
	m_ESRAMNumPages = 0;
	m_VirtualAddress = NULL;
	m_VirtualSize = 0;
	m_StagingVirtualAddress = NULL;
#if DYNAMIC_ESRAM
	m_UseAltTarget = false;
	m_UseAltTestFunc = NULL;
	m_AltTarget = NULL;
#endif
}


void grcRenderTargetDurango::ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) 
{
	if(m_Texture)
	{
		DestroySRVAndTargetViews();
		GRCDEVICE.DeleteTexture(m_Texture);
		m_CachedTexturePtr = NULL;

		if (m_Lockable && m_LockableTexture)
		{
			GRCDEVICE.DeleteTexture(m_LockableTexture);
			m_LockableTexture = NULL;
		}
	}

	m_Width = (u16) width;
	m_Height = (u16) height;
	m_Type = type;
	m_Texture = 0;
	m_Multisample = params ? params->Multisample : grcDevice::MSAA_None;
	
	Assert(params==NULL || GRCDEVICE.GetDxFeatureLevel()<1000 || params->ArraySize<=255);
	m_ArraySize = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->ArraySize : 1;
	m_PerArraySliceRTV = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->PerArraySliceRTV : 0;
	m_MipLevels = (u16) (params? params->MipLevels : 1);
	m_bUseAsUAV = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1100))? params->UseAsUAV : false;

	bool useFloat = params? params->UseFloat : false;

	grcTextureFormat tf = params? params->Format : grctfNone;
	if (tf == grctfNone)
	{
		if (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap || m_Type == grcrtDepthBufferCubeMap) 
		{
			tf = useFloat ? grctfD32FS8 : grctfD24S8;
		}
		else
			tf = useFloat? (bpp==32? grctfR32F : grctfR16F) : (bpp==32? grctfA8R8G8B8 : grctfR5G6B5);
	}
	else if (m_Type == grcrtDepthBuffer)
	{
		AssertMsg( tf == grctfD32F || tf == grctfD24S8 || tf == grctfD32FS8, "Invalid depth buffer format" );

		if( tf != grctfD32F && tf != grctfD24S8 && tf != grctfD32FS8 )
			tf = useFloat? grctfD32F : grctfD24S8; //(bpp == 32) ? grctfD24S8 : grctfD24S8;
	}

	m_Format = (u8) tf;
	m_BitsPerPix = (u8) bpp;
	m_Lockable = params? params->Lockable : false;
	m_LockableTexUpdated = false;
	m_ESRAMPhase = params ? (u32)params->ESRAMPhase : 0;
	m_ESRAMMaxSize = params ? (u32)params->ESRAMMaxSize : 0;
	m_ESRAMVirtualAddress = NULL;
	m_ESRAMNumPages = 0;

	if (!m_CreatedFromPreAllcatedMem)
	{
		m_VirtualAddress = NULL;
		m_VirtualSize = 0;
		m_StagingVirtualAddress = NULL;
	}

	m_CompressedDepth = IsDepthTypeOrFormat(m_Type, m_Format) ? (params ? params->EnableCompression : true) : false;
#if DYNAMIC_ESRAM
	m_UseAltTarget = false;
	m_UseAltTestFunc = NULL;
	m_AltTarget = NULL;
#endif

#if DEVICE_EQAA
	m_Coverage.manualDecoding = params && params->ForceFragmentMask;
	m_Coverage.compressionEnabled = m_Multisample && (m_Multisample.NeedFmask() || m_Coverage.manualDecoding);
	m_Coverage.resolveType = params && params->ForceHardwareResolve ? ResolveHW :
		(params && params->EnableHighQualityResolve) ? ResolveSW_HighQuality : ResolveSW_Simple;
	if (m_Coverage.resolveType == ResolveSW_Simple && (params && params->EnableNanDetect))
	{
		m_Coverage.resolveType = ResolveSW_Simple_NanDetect;
	}
	m_Coverage.supersampleFrequency = params ? params->SupersampleFrequency : 0;
#else // DEVICE_EQAA
	m_MultisampleQuality = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->MultisampleQuality : 0;;
#endif // DEVICE_EQAA

	CreateSurface();
}


void grcRenderTargetDurango::CreateSurface() 
{
	DXGI_FORMAT eFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));
	DXGI_FORMAT eTypelessFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTypelessFormat(eFormat));
	DXGI_FORMAT eTexFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTextureFormat(eFormat));

	const bool bDepthFormat = IsDepthTypeOrFormat(m_Type, m_Format);
	
	D3D11_TEXTURE2D_DESC oDesc = {0};
	oDesc.ArraySize = m_ArraySize;;
	//Only use typeless format on depthstencil.
	oDesc.Format = bDepthFormat ? eTypelessFormat : eFormat;
	oDesc.Width = m_Width;
	oDesc.Height = m_Height;
	oDesc.MipLevels = m_MipLevels;
	oDesc.SampleDesc.Count = m_Multisample ? m_Multisample : 1;
#if DEVICE_EQAA
	oDesc.SampleDesc.Quality = m_Coverage.compressionEnabled ? m_Multisample.DeriveQuality() : 0;
#else
	oDesc.SampleDesc.Quality = m_Multisample ? m_MultisampleQuality : 0;
#endif // DEVICE_EQAA
	oDesc.Usage = (D3D11_USAGE)grcUsageDefault;
	oDesc.MiscFlags = (m_MipLevels > 1 ? grcResourceGenerateMips : grcResourceNone) | ((m_Type == grcrtCubeMap ||  m_Type == grcrtDepthBufferCubeMap) ? grcResourceTextureCube : grcResourceNone);
	oDesc.CPUAccessFlags = grcCPUNoAccess;

	oDesc.BindFlags = grcBindRenderTarget;

	if (bDepthFormat && m_Multisample)
	{
		oDesc.BindFlags = grcBindShaderResource | grcBindDepthStencil;
	}
	else
		oDesc.BindFlags = grcBindShaderResource | (bDepthFormat ? grcBindDepthStencil : grcBindRenderTarget);

	oDesc.BindFlags |= (m_bUseAsUAV ? grcBindUnorderedAccess : grcResourceNone);

	if (!bDepthFormat && PARAM_usetypedformat.Get())
	{
		oDesc.Format = eFormat;
	}

	m_LockableTexture = NULL;

	XG_RESOURCE_LAYOUT colorLayout;
	XG_TILE_MODE tileMode = XG_TILE_MODE_LINEAR;

	{
		XG_TEXTURE2D_DESC	xgDesc;
		*((D3D11_TEXTURE2D_DESC*)&xgDesc) = oDesc;

		if ((oDesc.BindFlags & XG_BIND_DEPTH_STENCIL))
		{
			XG_TILE_MODE sTileMode;
			XGComputeOptimalDepthStencilTileModes(xgDesc.Format, xgDesc.Width, xgDesc.Height, xgDesc.ArraySize, xgDesc.SampleDesc.Count, m_CompressedDepth, &tileMode, &sTileMode);
		}
		else
		{
			tileMode = XGComputeOptimalTileMode(XG_RESOURCE_DIMENSION_TEXTURE2D, xgDesc.Format, xgDesc.Width, xgDesc.Height, xgDesc.ArraySize, xgDesc.SampleDesc.Count, xgDesc.BindFlags);
		}

		Assert(tileMode != XG_TILE_MODE_INVALID);

		xgDesc.TileMode = tileMode;
		xgDesc.Pitch = 0;

		grcDevice::Result uReturnCode;
		XGTextureAddressComputer *pComputer = NULL;
		uReturnCode = XGCreateTexture2DComputer(&xgDesc, &pComputer);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture computer", uReturnCode);
		uReturnCode = pComputer->GetResourceLayout(&colorLayout);
		Assertf((uReturnCode == 0), "Error %x occurred creating resource layout", uReturnCode);
		pComputer->Release();
	}
	
	if (bDepthFormat && !m_CompressedDepth)
	{
		oDesc.MiscFlags |= D3D11X_RESOURCE_MISC_NO_DEPTH_COMPRESSION;
		
		//is there a way to determine we don't have a stencil? grctfD16 is pretty much only used for shadows, so it's safe for now.
		if (m_Format==grctfD16)
		{
			oDesc.MiscFlags |= D3D11X_RESOURCE_MISC_NO_STENCIL_COMPRESSION; 
			
			//Displayf("%dk bytes saved by removing htile",colorLayout.Plane[0].SizeBytes/1024);
			// NOTE: if we're disabling the depth compression, the htile is not need and will not be used, so the PlaneOffsets need to be adjusted (also should save some memory
			// this should have been done with XGCreateTexture2DComputer(), but it does not let you pass in the no compression flags.
			colorLayout.Plane[0].BaseOffsetBytes = 0; // 0 is htile. zero it out
			colorLayout.Plane[0].SizeBytes = 0;
			colorLayout.Plane[1].BaseOffsetBytes = 0; // set the depth back to 0 (where is will actually be placed by CreatePlacementTexture2D) 
			colorLayout.SizeBytes = colorLayout.Plane[1].SizeBytes;
		}
	}

	m_PlaneOffsets[0] = colorLayout.Plane[0].BaseOffsetBytes;
	m_PlaneOffsets[1] = colorLayout.Plane[1].BaseOffsetBytes;
	m_PlaneSizes[0]	  = colorLayout.Plane[0].SizeBytes;
	m_PlaneSizes[1]   = colorLayout.Plane[1].SizeBytes;
	m_LockableTexture = NULL;

	grcDeviceHandle *dev = GRCDEVICE.GetCurrent();

	if (m_CreatedFromPreAllcatedMem)
	{
		Assertf(m_VirtualSize>=colorLayout.SizeBytes, "not enough room in the source rendertarget to overlap new target");
	}
	else
	{
		m_VirtualAddress = NULL;
		if (m_ESRAMPhase)
		{
			m_VirtualAddress = ESRAMManager::Alloc(oDesc, colorLayout, tileMode, this);
			m_VirtualSize = colorLayout.SizeBytes;
		}
	}


	if (!m_Multisample)
	{
		ASSERT_ONLY(HRESULT uReturnCode;)

		if (m_VirtualAddress)
		{
			ASSERT_ONLY(uReturnCode = ) dev->CreatePlacementTexture2D(&oDesc, tileMode, 0, m_VirtualAddress, (ID3D11Texture2D **)&m_Texture);
		}
		else
		{
			m_VirtualAddress = sysMemPhysicalAllocate(colorLayout.SizeBytes, colorLayout.BaseAlignmentBytes, PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
			Assertf((m_VirtualAddress != NULL), "Error (out of memory) occurred creating texture");
			m_VirtualSize = colorLayout.SizeBytes;

			ASSERT_ONLY(uReturnCode = ) dev->CreatePlacementTexture2D(&oDesc, tileMode, 0, m_VirtualAddress, (ID3D11Texture2D **)&m_Texture);
		}
		Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %x (out of memory) occurred creating texture", uReturnCode);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
		Assert(m_Texture != NULL);

		if (m_Lockable)
		{
			// Also create a staging texture that can be locked.
			D3D11_TEXTURE2D_DESC oStagingDesc = oDesc;
			oStagingDesc.CPUAccessFlags = grcCPURead;
			oStagingDesc.Usage = (D3D11_USAGE)grcUsageStage;
			oStagingDesc.MiscFlags = grcResourceNone;
			oStagingDesc.BindFlags = grcBindNone;

			m_StagingVirtualAddress = sysMemPhysicalAllocate(colorLayout.SizeBytes, colorLayout.BaseAlignmentBytes, PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
			Assertf((m_StagingVirtualAddress != NULL), "Error (out of memory) occurred creating staging texture");
			CHECK_HRESULT(dev->CreatePlacementTexture2D(&oStagingDesc, XG_TILE_MODE_LINEAR, 0, m_StagingVirtualAddress, (ID3D11Texture2D **)&m_LockableTexture));
		}
	}
	else
	{
		// Create MSAA texture 
		ASSERT_ONLY(HRESULT uReturnCode;)

	#if DEVICE_EQAA
		bool accessFmask = m_Coverage.compressionEnabled && m_Coverage.manualDecoding && !bDepthFormat;
		// No need to check available quality levels since they are derived here, not passed from the outside
		if (accessFmask)
		{
			oDesc.MiscFlags |= D3D11X_RESOURCE_MISC_NO_COLOR_EXPAND;
		}
	#endif // DEVICE_EQAA
	#if !DEVICE_EQAA || (RSG_DURANGO && _XDK_VER >= 10542)
		if (!bDepthFormat)	// depth-stencil format is not supported by CheckMultisampleQualityLevels() yet
		{
			// Check the requested level is available and clamp down to the best available.
			UINT availableQualityLevels;
			CHECK_HRESULT(dev->CheckMultisampleQualityLevels(oDesc.Format, oDesc.SampleDesc.Count, &availableQualityLevels) );
			Assertf(oDesc.SampleDesc.Quality < availableQualityLevels, "Invalid MSAA quality level %d specified for format %d, SampleCount %d. Will clamp to best available instead.", oDesc.SampleDesc.Quality, oDesc.Format, oDesc.SampleDesc.Count);
			if (oDesc.SampleDesc.Quality >= availableQualityLevels)
				oDesc.SampleDesc.Quality = availableQualityLevels-1;
		}
	#endif // !DEVICE_EQAA || RSG_DURANGO

		if (!m_VirtualAddress)
		{
			m_VirtualAddress = sysMemPhysicalAllocate(colorLayout.SizeBytes, colorLayout.BaseAlignmentBytes, PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
			Assertf((m_VirtualAddress != NULL), "Error (out of memory) occurred creating texture");
			m_VirtualSize = colorLayout.SizeBytes;
		}
		ASSERT_ONLY(uReturnCode = ) dev->CreatePlacementTexture2D(&oDesc, tileMode, 0, m_VirtualAddress, (ID3D11Texture2D **)&m_Texture);
		Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %x (out of memory) occurred creating texture", uReturnCode);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
		Assert(m_Texture != NULL);

	#if DEVICE_EQAA
		if (accessFmask)
		{
			m_Coverage.texture = static_cast<grcTextureFactoryDX11&>(grcTextureFactory::GetInstance()).CreateAsFmask( m_Texture, m_Multisample );
		}
	#endif // DEVICE_EQAA
	}

	//----------------------------------------------------------------------------------------------------------------------------------//
																																		
	m_pUnorderedAccessView = NULL;

	if (m_bUseAsUAV) 
	{
		Assert( m_Multisample==0 );
		Assert( (oDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) );
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory( &UAVDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
		UAVDesc.Format = eTexFormat;
		UAVDesc.ViewDimension = (oDesc.ArraySize > 1) ? D3D11_UAV_DIMENSION_TEXTURE2DARRAY : D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;
		UAVDesc.Texture2DArray.FirstArraySlice = 0;
		UAVDesc.Texture2DArray.ArraySize = oDesc.ArraySize;

		ASSERT_ONLY(HRESULT uReturnCode = ) dev->CreateUnorderedAccessView( (ID3D11Resource*)m_Texture, &UAVDesc, (ID3D11UnorderedAccessView**)&m_pUnorderedAccessView);
		Assertf((uReturnCode == 0), "Error %ud occurred creating texture unordered access view", uReturnCode);
	}

	//----------------------------------------------------------------------------------------------------------------------------------//
	CreateSRVAndTargetViews(DepthStencilRW, (m_PerArraySliceRTV ? m_PerArraySliceRTV : 1));

	DebugSetUnresolved();
	SetPrivateData();
	m_CachedTexturePtr = m_Texture;
}



void grcRenderTargetDurango::CreateSRVAndTargetViews(DepthStencilFlags depthStencilFlags, u32 sliceCount)
{
	DXGI_FORMAT eD3DFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));
	DXGI_FORMAT eTexFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTextureFormat(eD3DFormat));
	m_DepthStencilFlags = depthStencilFlags;
	m_pShaderResourceView = NULL;

	if (eTexFormat != DXGI_FORMAT_UNKNOWN) // Render target is not sampleable
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC oViewDesc;
		oViewDesc.Format = eTexFormat;

		if (m_Type == grcrtCubeMap || m_Type == grcrtDepthBufferCubeMap)
		{
			Assert(!m_Multisample);
			if (m_ArraySize > 6)
			{
				Assert(m_ArraySize%6 == 0);
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				oViewDesc.TextureCubeArray.MostDetailedMip = 0;
				oViewDesc.TextureCubeArray.MipLevels = m_MipLevels;
				oViewDesc.TextureCubeArray.First2DArrayFace = 0;
				oViewDesc.TextureCubeArray.NumCubes = m_ArraySize/6;
			}
			else
			{
				Assert(m_ArraySize == 6);
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				oViewDesc.TextureCube.MostDetailedMip = 0;
				oViewDesc.TextureCube.MipLevels = m_MipLevels;
			}
		}
		else if (m_ArraySize > 1)
		{
			oViewDesc.ViewDimension = m_Multisample ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;

			if (m_Multisample)
			{
				oViewDesc.Texture2DMSArray.ArraySize = m_ArraySize;
				oViewDesc.Texture2DMSArray.FirstArraySlice = 0;
			}
			else
			{
				oViewDesc.Texture2DArray.MipLevels = m_MipLevels;
				oViewDesc.Texture2DArray.ArraySize = m_ArraySize;
				oViewDesc.Texture2DArray.FirstArraySlice = 0;
				oViewDesc.Texture2DArray.MostDetailedMip = 0;
			}

		}
		else
		{
			oViewDesc.ViewDimension = m_Multisample ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
			oViewDesc.Texture2D.MipLevels = m_MipLevels;
			oViewDesc.Texture2D.MostDetailedMip = 0;
		}

		ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateShaderResourceView(m_Texture, &oViewDesc, (ID3D11ShaderResourceView**)&m_pShaderResourceView);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
	}

	Assert(m_TargetViews.GetCapacity()==0);
	m_TargetViews.Resize(sliceCount*m_MipLevels);

	for( u32 i = 0; i < sliceCount; i++ )
	{
		for (u32 j = 0; j < m_MipLevels; j++)
		{
			m_TargetViews[i*m_MipLevels+j] = NULL;
			CreateTargetView(i, j, m_ArraySize, depthStencilFlags);
		}
	}
}


void grcRenderTargetDurango::DestroySRVAndTargetViews()
{
	if (m_pShaderResourceView != NULL)
	{
		m_pShaderResourceView->Release();
		m_pShaderResourceView = NULL;
	}

	for( int i = 0; i < m_TargetViews.GetCount(); i++ )
	{
		if (m_TargetViews[i] != NULL)
			m_TargetViews[i]->Release();
	}

	m_TargetViews.Reset();
}


bool grcRenderTargetDurango::CreateTargetView(u32 uArraySlice, u32 uMip, u32 uArraySize, DepthStencilFlags depthStencilReadOnly)
{
	int arrayIndex = uArraySlice*m_MipLevels+uMip;

	Assert(m_TargetViews[arrayIndex] == NULL);

	DXGI_FORMAT eFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));

	bool bDepthFormat = (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap ||  m_Type == grcrtDepthBufferCubeMap) || 
						(m_Format == grctfD24S8) || (m_Format == grctfD32F) || (m_Format == grctfX24G8) || (m_Format == grctfD32FS8) || (m_Format == grctfX32S8) || (m_Format == grctfD16);

	//If this is a read only texture format dont try to create the target view as it will fail.
	if(!IsReadOnlyFormat(m_Format))
	{
		if (!bDepthFormat)
		{
			D3D11_RENDER_TARGET_VIEW_DESC oViewDesc;
			oViewDesc.Format = eFormat;
			
			// check if this is texture array
			if (uArraySize > 1)
			{
				oViewDesc.ViewDimension = m_Multisample ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;

				if (m_Multisample)
				{
					oViewDesc.Texture2DMSArray.FirstArraySlice = uArraySlice;
					oViewDesc.Texture2DMSArray.ArraySize = m_PerArraySliceRTV ? 1 : uArraySize;
				}
				else
				{
					oViewDesc.Texture2DArray.MipSlice = uMip;
					oViewDesc.Texture2DArray.ArraySize = m_PerArraySliceRTV ?  1 : uArraySize;
					oViewDesc.Texture2DArray.FirstArraySlice = uArraySlice;
				}
			}
			else
			{
				oViewDesc.ViewDimension = m_Multisample ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
				oViewDesc.Texture2D.MipSlice = uMip;
			}
			ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateRenderTargetView((ID3D11Resource*)m_Texture, &oViewDesc, (ID3D11RenderTargetView**)&m_TargetViews[arrayIndex]);
			Assertf(( uReturnCode == 0), "Error %x occurred creating render target view", uReturnCode);
		}
		else
		{
			// If the format has a stencil field create a view to use it
			grcTexture::ChannelBits channels = FindUsedChannels();

			D3D11_DEPTH_STENCIL_VIEW_DESC oViewDesc;
			oViewDesc.Format = eFormat;

			// check if this is texture array
			if (uArraySize > 1)
			{
				oViewDesc.ViewDimension = m_Multisample ? D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D11_DSV_DIMENSION_TEXTURE2DARRAY;

				if (m_Multisample)
				{
					oViewDesc.Texture2DMSArray.FirstArraySlice = uArraySlice;
					oViewDesc.Texture2DMSArray.ArraySize = m_PerArraySliceRTV ? 1 : uArraySize;
				}
				else
				{
					oViewDesc.Texture2DArray.MipSlice = uMip;
					oViewDesc.Texture2DArray.ArraySize = m_PerArraySliceRTV ? 1 : uArraySize;
					oViewDesc.Texture2DArray.FirstArraySlice = uArraySlice;
				}
			}
			else
			{
				oViewDesc.ViewDimension = m_Multisample ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
				oViewDesc.Texture2D.MipSlice = uMip;
			}
	
			// IsReadOnlyDepthAllowed() is not used here intentionally
			if (GRCDEVICE.GetDxFeatureLevel() >= 1100) 
			{
				oViewDesc.Flags = ((depthStencilReadOnly & DepthReadOnly) ? D3D11_DSV_READ_ONLY_DEPTH : 0)
								 | ((depthStencilReadOnly & StencilReadOnly) ? ((channels.IsSet(grcTexture::CHANNEL_STENCIL)) ?D3D11_DSV_READ_ONLY_STENCIL : 0) : 0);
			}
			else
				oViewDesc.Flags = 0;

			ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateDepthStencilView((ID3D11Resource*)m_Texture, &oViewDesc, (ID3D11DepthStencilView**)&m_TargetViews[arrayIndex]);
			Assertf((uReturnCode == 0), "Error %x occurred creating render target view", uReturnCode);
		}
	}
	return true;
}


u32 grcRenderTargetDurango::GetRequiredMemory() const
{
	// Only currently only has 1 mip level for render targets
	// TODO: Fmask + Cmask memory
	return /*(m_Lockable ? 2 : 1) * */  (s64)(m_Multisample ? m_Multisample : 1 ) * (s64)GetWidth() * (s64)GetHeight() * (s64)m_BitsPerPix / (s64)8 *  (s64)((m_Type == grcrtCubeMap ||  m_Type == grcrtDepthBufferCubeMap) ? 6 : 1); 
}


/*======================================================================================================================================*/
/* class ESRAMManager.																													*/
/*======================================================================================================================================*/

#define USE_XG_CUSTOM_LAYOUTS	1
#if USE_XG_CUSTOM_LAYOUTS
// Working round XG bug for EQAA 4sf1, until it's fixed in the March SDK

struct CustomXgLayout{
	XG_TEXTURE2D_DESC desc;
	XG_RESOURCE_LAYOUT layout;
}static customLayouts[] =
{
	{	//CustomXGlayout
		{	// XG_TEXTURE2D_DESC
			1600, 900, 1, 1,
			XG_FORMAT_B8G8R8A8_UNORM,
			{1, 2}, // XG_SAMPLE_DESC
			XG_USAGE_DEFAULT,
			0x28, 0, 0,	0,
#if _XDK_VER >= 10542
			0,	// UINT ESRAMUsageBytes
#endif
			XG_TILE_MODE_2D_THIN,
			0
		},
		{	// XG_RESOURCE_LAYOUT
			0x734000, 0x2000, 1, 3,
			{	//XG_PLANE_LAYOUT[3]
				{
					XG_PLANE_USAGE_DEFAULT,
					0x5aa000, 0, 0x2000, 4,
					{	//XG_MIPLEVEL_LAYOUT[15]
						{
							0x5aa000, 0, 0x5aa000, 
							1600, 6400, 0x2000,
							1600, 928, 1,
							1600, 900, 1, 1,
							XG_TILE_MODE_2D_THIN
						}
					}
				},
				{
					XG_PLANE_USAGE_FRAGMENT_MASK,
					0x186000, 0x5aa000, 8192, 1,
					{	//XG_MIPLEVEL_LAYOUT[15]
						{
							0x186000, 0x5aa000, 0x186000,
							1664, 1664, 0x2000,
							1664, 960, 1,
							1600, 900, 1, 1,
							XG_TILE_MODE_2D_THIN
						}
					}
				},
				{
					XG_PLANE_USAGE_COLOR_MASK,
					0x4000, 0x730000, 0x2000, 0,
					{	//XG_MIPLEVEL_LAYOUT[15]
						0
					}
				}
			},
			XG_RESOURCE_DIMENSION_TEXTURE2D
		}
	},
};

static bool patchLayout(const XG_TEXTURE2D_DESC &desc, XG_RESOURCE_LAYOUT &layout)
{
	if (desc.SampleDesc.Count>1 || !desc.SampleDesc.Quality || layout.Planes!=2)
		return false;

	// plane[0]==color, plane[1]==cmask, we are missing Fmask
	const unsigned widthMask = 0x7F, heightMask = 0x3F;
	const unsigned paddedWidth = (desc.Width + widthMask) & ~widthMask;
	const unsigned paddedHeight = (desc.Height + heightMask) & ~heightMask;
	const unsigned fmaskBytesPerPixel = 1;
	const unsigned fmaskSize = paddedWidth * paddedHeight * fmaskBytesPerPixel;
	const unsigned totalMask = 0x1FFF;
	const unsigned fmaskOffset = (layout.SizeBytes + totalMask) & ~totalMask;
	const XG_MIPLEVEL_LAYOUT fmaskMip =
	{
		fmaskSize,
		fmaskOffset,
		fmaskSize,			// slice size
		paddedWidth,		// pitch pixels
		paddedWidth * fmaskBytesPerPixel,
		totalMask+1,
		paddedWidth,
		paddedHeight,
		1,
		desc.Width,
		desc.Height,
		1,
		1,
		XG_TILE_MODE_2D_THIN
	};
	const XG_PLANE_LAYOUT fmaskLayout =
	{
		XG_PLANE_USAGE_FRAGMENT_MASK,
		fmaskSize,
		fmaskOffset,
		totalMask+1,
		fmaskBytesPerPixel,
		{ fmaskMip }
	};
	layout.SizeBytes = fmaskOffset + fmaskSize;
	layout.Plane[2] = fmaskLayout;
	layout.Planes = 3;
	
	return true;
}
#endif //USE_XG_CUSTOM_LAYOUTS

#if DYNAMIC_ESRAM

void ESRAMManager::ESRAMCopyIn(grcRenderTarget* dst, grcRenderTarget* src)
{
	g_grcCurrentContext->CopyResource(static_cast<grcRenderTargetDurango*>(dst)->GetTextureObject(), static_cast<grcRenderTargetDurango*>(src)->GetTextureObject());
}

void ESRAMManager::ESRAMCopyOut(grcRenderTarget* dst, grcRenderTarget* src)
{
	g_grcCurrentContext->CopyResource(static_cast<grcRenderTargetDurango*>(dst)->GetTextureObject(), static_cast<grcRenderTargetDurango*>(src)->GetTextureObject());
}

#endif

u32 ESRAMManager::GetMainPlaneId(XG_RESOURCE_LAYOUT& colorLayout)
{
	u32 mainPlane = 0;
	if (colorLayout.Plane[1].Usage == XG_PLANE_USAGE_DEPTH)
		mainPlane = 1;
	return mainPlane;
}

#define ESRAM_NUM_PHASES (grcTextureFactory::TextureCreateParams::ESRPHASE_MAX)

static ALIGNAS(64) u8 s_PageArray[ESRAM_NUM_PHASES][512] = {0};

static bool AllocPhases(u64 phaseMask, u64 memSize, u32* ESRAMPageList)
{
	u32 numPages = (memSize + _LARGE_PAGE_SIZE - 1) / _LARGE_PAGE_SIZE;

	u32 numPhases=0;
	u32 n = 0;
	u64 msk = phaseMask;
	u32 phases[ESRAM_NUM_PHASES];
	// find phases for which RT is active
	while (msk)
	{
		if (msk & 1)
		{
			phases[numPhases] = n;
			numPhases++;
		}
		msk >>= 1;
		n++;
	}

	// search for ESRAM pages that are free in all active phases
	u32 firstFree = 0;
	for( u32 i = 0; i < numPages; ++i )
	{
		bool found = false;
		for( u32 j = firstFree; j < 512; ++j )
		{
			bool free = true;
			for ( u32 k = 0; k < numPhases; ++ k)
			{
				if (s_PageArray[phases[k]][j])
				{
					free = false;
					break;
				}
			}
			if (free)
			{
				found = true;
				ESRAMPageList[i] = j;
				firstFree = j+1;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
	}
#if __DEV
	//Printf("Pages:");
#endif
	// mark new pages as allocated
	for( u32 i = 0; i < numPages; ++i )
	{
		u32 j = ESRAMPageList[i];
		for ( u32 k = 0; k < numPhases; ++ k)
		{
			s_PageArray[phases[k]][j] = 1;
		}
#if __DEV
		//Printf(" %u",j);
#endif
	}
#if __DEV
	//Printf("\n");
#endif
	return true;
}

static void FreePhases(u64 phaseMask, u64 numPages, u32* ESRAMPageList)
{
	u32 numPhases=0;
	u32 n = 0;
	u64 msk = phaseMask;
	u32 phases[ESRAM_NUM_PHASES];
	// find phases for which RT is active
	while (msk)
	{
		if (msk & 1)
		{
			phases[numPhases] = n;
			numPhases++;
		}
		msk >>= 1;
		n++;
	}
	// free pages
	for( u32 i = 0; i < numPages; ++i )
	{
		u32 j = ESRAMPageList[i];
		for ( u32 k = 0; k < numPhases; ++ k)
		{
			s_PageArray[phases[k]][j] = 0;
		}
	}
}

void* ESRAMManager::Alloc(D3D11_TEXTURE2D_DESC &oDesc, XG_RESOURCE_LAYOUT& colorLayout, XG_TILE_MODE& tileMode, grcRenderTargetDurango* rt)
{
	u64 phaseMask = rt->m_ESRAMPhase;
	if (phaseMask == 0)
		return 0;
	u64 sizeLimit = rt->m_ESRAMMaxSize;

	if (tileMode == XG_TILE_MODE_INVALID)
		return 0;

#if USE_XG_CUSTOM_LAYOUTS
		patchLayout((XG_TEXTURE2D_DESC&)oDesc, colorLayout);
#endif //USE_XG_CUSTOM_LAYOUTS

	sizeLimit = colorLayout.SizeBytes > sizeLimit ? sizeLimit : 0;
	u64 memSize = colorLayout.SizeBytes;
	u32 mainPlane = GetMainPlaneId(colorLayout);
	if (sizeLimit && colorLayout.Plane[mainPlane].SizeBytes <= sizeLimit)
	{
		// if size limited, only put main plane in ESRAM 
		sizeLimit = colorLayout.Plane[mainPlane].SizeBytes;
	}

/*
	// don't put FMASK in ESRAM - causes UI3DManager to mess up
	if (!sizeLimit)
	{
		for (u32 j = 0; j < colorLayout.Planes; j++)
		{
			if (colorLayout.Plane[j].Usage == XG_PLANE_USAGE_FRAGMENT_MASK)
			{
				sizeLimit = colorLayout.Plane[mainPlane].SizeBytes;
				break;
			}
		}
	}
*/

	bool bPartMap = false;
	if (sizeLimit > 0 && sizeLimit <= memSize)
	{
		memSize = sizeLimit;
		bPartMap = true;
	}

	void *virtualAddress = NULL;

	u32 ESRAMPageList[ 512 ];

	if (AllocPhases(phaseMask, memSize, ESRAMPageList))
	{
#if __BANK
		Printf("ESRAM:<%s>[%x] w,h %u,%u fmt %u size %u\n",rt->GetName(),phaseMask,oDesc.Width,oDesc.Height,oDesc.Format,memSize);
#endif
		rt->m_ESRAMNumPages = (memSize + _LARGE_PAGE_SIZE - 1) / _LARGE_PAGE_SIZE;

		virtualAddress = ESRAMManager::VirtualMap(colorLayout, &rt->m_ESRAMVirtualAddress, ESRAMPageList, rt->m_ESRAMNumPages, bPartMap);
	}
#if __BANK
	else
	{
		//Printf("FAIL:<%s>[%x] w,h %u,%u fmt %u size %u\n",rt->GetName(),phaseMask,oDesc.Width,oDesc.Height,oDesc.Format,memSize);
	}
#endif

	return virtualAddress;
}

void* ESRAMManager::VirtualMap(XG_RESOURCE_LAYOUT& colorLayout,void **pESRAMVirtualAddress, u32* ESRAMPageList, u32 ESRAMNumPages, bool bPartMap)
{
	// allocate virtual address space for the whole target
	const size_t flags = (MEM_GRAPHICS | MEM_RESERVE) | (((colorLayout.SizeBytes & 0x3FFFFF) == 0) ? MEM_4MB_PAGES : MEM_LARGE_PAGES);
	void *virtualAddress = VirtualAllocTracked(nullptr, colorLayout.SizeBytes, flags, dwProtect);
	Assertf((virtualAddress != NULL), "Error (out of memory) occurred creating texture");

	*pESRAMVirtualAddress = virtualAddress;

	if (bPartMap)
	{
		u32 mainPlane = GetMainPlaneId(colorLayout);
		// commit any pages not in ESRAM
		u32 totalPages = (colorLayout.SizeBytes + _LARGE_PAGE_SIZE - 1) / _LARGE_PAGE_SIZE;
		u32 defaultPages = (colorLayout.Plane[mainPlane].SizeBytes + _LARGE_PAGE_SIZE - 1) / _LARGE_PAGE_SIZE;
		u32 remPages = totalPages - ESRAMNumPages;
		u64 topPages = (colorLayout.Plane[mainPlane].BaseOffsetBytes + _LARGE_PAGE_SIZE - 1) / _LARGE_PAGE_SIZE + (defaultPages - ESRAMNumPages) / 2;
		u64 botPages = remPages - topPages;


		// map physical DRAM pages to part of the virtual address space
		if (topPages)
		{
			BYTE* startAddress = reinterpret_cast<BYTE*>( virtualAddress );
			ASSERT_ONLY(void *retAddress = )VirtualAllocTracked(startAddress, topPages * _LARGE_PAGE_SIZE, MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_COMMIT, dwProtect);
			Assertf((retAddress == startAddress),"Virtual memory error");
			*pESRAMVirtualAddress = (reinterpret_cast<BYTE*>( virtualAddress ) + topPages * _LARGE_PAGE_SIZE);
		}
		if (botPages)
		{
			BYTE* startAddress = reinterpret_cast<BYTE*>( virtualAddress ) + ( (totalPages - botPages) * _LARGE_PAGE_SIZE );
			ASSERT_ONLY(void *retAddress = )VirtualAllocTracked(startAddress, botPages * _LARGE_PAGE_SIZE, MEM_GRAPHICS | MEM_LARGE_PAGES | MEM_COMMIT, dwProtect);
			Assertf((retAddress == startAddress),"Virtual memory error");
		}
	}

	// map ESRAM pages to the virtual address space
	CHECK_HRESULT(D3DMapEsramMemory(D3D11_MAP_ESRAM_LARGE_PAGES, *pESRAMVirtualAddress, ESRAMNumPages, ESRAMPageList));
	return virtualAddress;
}

void ESRAMManager::Free(void *VirtualAddress, u64 size, void *ESRAMVirtualAddress, u32 ESRAMNumPages)
{
	if (ESRAMNumPages)
	{
		CHECK_HRESULT(D3DUnmapEsramMemory(D3D11_MAP_ESRAM_LARGE_PAGES, ESRAMVirtualAddress, ESRAMNumPages));
	}
	VirtualFreeTracked(VirtualAddress, size , MEM_RELEASE);
}

}	// namespace rage

#endif // RSG_DURANGO
