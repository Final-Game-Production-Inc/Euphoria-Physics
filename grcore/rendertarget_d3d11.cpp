#include	"rendertarget_d3d11.h"
#include	"texture_d3d11.h"
#include	"texturefactory_d3d11.h"

#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"
#include	"grcore/d3dwrapper.h"

#if	RSG_PC && __D3D && __D3D11

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

// DOM-IGNORE-BEGIN 
#if RSG_PC
#include "../../../3rdParty/NVidia/nvapi.h"
#endif
// DOM-IGNORE-END
XPARAM(usetypedformat);

namespace rage 
{

extern GUID TextureBackPointerGuid;

/*======================================================================================================================================*/
/* class grcRenderTargetDX11.																											*/
/*======================================================================================================================================*/


grcRenderTargetDX11::grcRenderTargetDX11(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) :
	grcRenderTargetPC()
{
	m_Name = StringDuplicate(name);
	//m_Texture = 0;

	m_CreatedFromTextureObject = false;

	m_pShaderResourceView = NULL;
	m_pSecondaryShaderResourceView = NULL;
	m_pUnorderedAccessView = NULL;

	m_MipLevels = 1;
	m_ArraySize = 1;
	m_PerArraySliceRTV = 0;

	m_LockableTexture = 0;

	ReCreate(type, width, height, bpp, params);
}

grcRenderTargetDX11::grcRenderTargetDX11(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params ) : grcRenderTargetPC()
{
	CreateFromTextureObject( name, pTexture, eShaderViewFormat, depthStencilReadOnly, params );
}

grcRenderTargetDX11::grcRenderTargetDX11(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params ) : grcRenderTargetPC()
{
	CreateFromTextureObject( name, pTexture, depthStencilReadOnly, params );
}

void grcRenderTargetDX11::CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, DepthStencilFlags depthStencilReadOnly, grcTextureFactory::CreateParams *params )
{
	ID3D11Texture2D *poTexture = (ID3D11Texture2D*)(pTexture);
	D3D11_TEXTURE2D_DESC oDesc;
	poTexture->GetDesc(&oDesc);

	grcTextureFormat eFormat = static_cast<grcTextureFormat>(grcTextureFactoryDX11::TranslateToRageFormat(oDesc.Format));

	CreateFromTextureObject( name, pTexture, eFormat, depthStencilReadOnly, params );
}

void grcRenderTargetDX11::CreateFromTextureObject(const char *name, const grcTextureObject *pTexture, grcTextureFormat eShaderViewFormat, DepthStencilFlags depthStencilReadOnly,grcTextureFactory::CreateParams *params )
{
#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	m_bHasBeenWrittenTo = false;
#endif

	m_Name = StringDuplicate(name);
	m_CachedTexturePtr = const_cast<grcTextureObject*>(pTexture);
	m_LockableTexture = 0;
	m_CreatedFromTextureObject = true;

	ID3D11Texture2D *poTexture = (ID3D11Texture2D*)(pTexture);
	D3D11_TEXTURE2D_DESC oDesc;
	poTexture->GetDesc(&oDesc);

	m_pShaderResourceView = NULL;
	m_pSecondaryShaderResourceView = NULL;
	m_pUnorderedAccessView = NULL;
	m_MipLevels = (u16) oDesc.MipLevels;

	m_Width = (u16)oDesc.Width;
	m_Height = (u16)oDesc.Height;
	Assert(oDesc.Usage == D3D11_USAGE_DEFAULT);
	Assert(oDesc.ArraySize == 1);
	Assert(oDesc.MipLevels == 1);
	bool bCanBeUsedAsTexture = (oDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) ? true : false;
	m_Multisample = (u8)oDesc.SampleDesc.Count > 1 ? (u8)oDesc.SampleDesc.Count : 0;
	m_MultisampleQuality = (u8)oDesc.SampleDesc.Quality;
	m_Format = eShaderViewFormat;
	m_ArraySize = (u8)oDesc.ArraySize;
	m_PerArraySliceRTV = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->PerArraySliceRTV : 0;

	DXGI_FORMAT eD3DFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat(eShaderViewFormat));
	DXGI_FORMAT eTexFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTextureFormat(eD3DFormat));

	m_Type = ((m_Format == grctfD24S8) || (m_Format == grctfD32F) || (m_Format == grctfX24G8) || (m_Format == grctfD32FS8) || (m_Format == grctfX32S8)) ? grcrtDepthBuffer : grcrtBackBuffer;

	if (bCanBeUsedAsTexture && (eTexFormat != DXGI_FORMAT_UNKNOWN)) // Render target is not sampleable
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC oViewDesc;
		oViewDesc.Format = eTexFormat;
		
		
		if (m_ArraySize > 1)
		{
			if (oDesc.SampleDesc.Count > 1)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				oViewDesc.Texture2DMSArray.FirstArraySlice = 0;
				oViewDesc.Texture2DMSArray.ArraySize = oDesc.ArraySize;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				oViewDesc.Texture2DArray.MostDetailedMip = 0;
				oViewDesc.Texture2DArray.MipLevels = oDesc.MipLevels;
				oViewDesc.Texture2DArray.FirstArraySlice = 0;
				oViewDesc.Texture2DArray.ArraySize = oDesc.ArraySize;
			}
		}
		else
		{
			if (oDesc.SampleDesc.Count > 1)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				oViewDesc.Texture2D.MostDetailedMip = 0;
				oViewDesc.Texture2D.MipLevels = oDesc.MipLevels;
			}
		}

		ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateShaderResourceView(poTexture, &oViewDesc, (ID3D11ShaderResourceView**)&m_pShaderResourceView);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
	}
	else
	{
		m_pShaderResourceView = NULL;
	}

	if (m_PerArraySliceRTV)
	{
		Assert(m_TargetViews.GetCapacity()==0);
		m_TargetViews.Resize(m_PerArraySliceRTV*m_MipLevels);
		for (int i=0;i<m_TargetViews.GetCount();i++)
			m_TargetViews[i] = NULL;

		for( u32 i = 0; i < m_PerArraySliceRTV; i++ )
		{
			for (u32 j = 0; j < m_MipLevels; j++)
				CreateTargetView(i,j,m_ArraySize, depthStencilReadOnly);
		}
	}
	else
	{
		Assert(m_TargetViews.GetCapacity()==0);
		m_TargetViews.Resize(1*m_MipLevels);
		for (int i=0;i<m_TargetViews.GetCount();i++)
			m_TargetViews[i] = NULL;
		for (u32 i = 0; i < m_MipLevels; i++)
			CreateTargetView(0,i,m_ArraySize, depthStencilReadOnly);
	}

	SetPrivateData();
}

grcRenderTargetDX11::~grcRenderTargetDX11() {
#if defined(SRC_DST_TRACKING) && SRC_DST_TRACKING
	grcTextureFactoryDX11::MultiMapTargetTextureSources::iterator itDstLower, itDstUpper, it;

	sysCriticalSection cs(grcTextureFactoryDX11::sm_Lock);
	itDstLower = grcTextureFactoryDX11::sm_mmapSourceTextureList.lower_bound(this);
	itDstUpper = grcTextureFactoryDX11::sm_mmapSourceTextureList.upper_bound(this);
	grcTextureFactoryDX11::sm_mmapSourceTextureList.erase(itDstLower, itDstUpper);
#endif // SRC_DST_TRACKING
	grcTextureFactory::UnregisterRenderTarget(this);

	DestroyInternalData();

	// If there's a texture factory, tell it we're leaving.
	// (need this check because some render targets don't get destroyed until after the factory is gone)
	if (grcTextureFactory::HasInstance())
		static_cast<grcTextureFactoryPC &> (grcTextureFactory::GetInstance()).RemoveRenderTarget(this);
}

void grcRenderTargetDX11::DestroyInternalData()
{
	if (!m_CreatedFromTextureObject)
	{
		sm_TotalMemory -= GetRequiredMemory();
		if ((m_Width != m_Height) &&
			(m_Width >= GRCDEVICE.GetWidth()) && 
			(m_Height >= GRCDEVICE.GetHeight()))
		{
			sm_TotalStereoMemory -= GetRequiredMemory();
		}
	}

	if (m_pShaderResourceView != NULL)
	{
		SAFE_RELEASE_RESOURCE(m_pShaderResourceView);
	}

	if (m_pUnorderedAccessView != NULL)
	{
		SAFE_RELEASE_RESOURCE(m_pUnorderedAccessView);
	}

	for( int i = 0; i < m_TargetViews.GetCount(); i++ )
	{
		if (m_TargetViews[i] != NULL)
			SAFE_RELEASE_RESOURCE(m_TargetViews[i]);
	}

	m_TargetViews.Reset();

	if (!m_CreatedFromTextureObject)
	{
		GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	}

	if (m_LockableTexture)
	{
		GRCDEVICE.DeleteTexture(m_LockableTexture);
		m_LockableTexture = NULL;
	}

/*
TODO: not dealt with these yet
	grcDeviceView		*m_pUnorderedAccessView;

	bool				m_bUseAsUAV;

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	bool				m_bHasBeenWrittenTo;
#endif*/

	/*
From grcRenderTargetPC
TODO: nothing happens to any of this....except m_Texture
	grcDeviceTexture	*m_Texture;
	grcDeviceTexture	*m_LockableTexture;
	bool				m_Lockable;
	bool				m_LockableTexUpdated;
	bool				m_bResolved;
	u32					m_Format;
	u16					m_Width;
	u16					m_Height;
	u16					m_Depth;
	u8					Unused_Padding;
	u8					m_BitsPerPix;
	u8					m_Multisample;			// MSAA Off == 0, else the number of samples.
	u8					m_MultisampleQuality;

	static u64			sm_TotalMemory;
	static u64			sm_TotalStereoMemory;
	u16					m_SlotId;
	u16					m_MipLevels;

	#if RSG_PC
	grcDevice::Stereo_t m_StereoRTMode;
	#endif*/

	/*
From grcTexture
TODO nothing dealt with except m_CachedTexturePtr
	CellGcmTexture			m_Texture;		// 24 bytes.  Really ought to have 360 data here too, but it's 13 words instead of only 6.  Would fill out the cache line nicely though.
	const char				*m_Name;		// +32
	mutable u16				m_RefCount;		// +36
	u8						m_ResourceTypeAndPrivateFlags;	// +38
	u8						m_LayerCount; // This is actually one less that the real layer count so that we can store 256.
	datPadding64(4,m_pad64)
		grcTextureObject		*m_CachedTexturePtr;	// +40
	u32						m_PhysicalSize;	// +44
	u32						m_HandleIndex;	// +48
*/
	m_CachedTexturePtr = NULL;
}

void grcRenderTargetDX11::SetPrivateData()
{
#if !__D3D11_MONO_DRIVER && !__FINAL
	char szTempName[256];
	if (m_CachedTexturePtr)
	{
		ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(m_CachedTexturePtr);
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

bool grcRenderTargetDX11::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return m_CreatedFromTextureObject || grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr());
#endif // __FINAL
}

void grcRenderTargetDX11::Update()
{
	if (m_CachedTexturePtr == NULL)
		CreateSurface();
}

void grcRenderTargetDX11::ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) {
	if(m_CachedTexturePtr)
	{
		if (!m_CreatedFromTextureObject)
		{
			sm_TotalMemory -= GetRequiredMemory();
			if ((width != height) &&
				(width >= GRCDEVICE.GetWidth()) && 
				(height >= GRCDEVICE.GetHeight()))
			{
				sm_TotalStereoMemory -= GetRequiredMemory();
			}
		}
		if (m_pShaderResourceView != NULL)
		{
			SAFE_RELEASE_RESOURCE(m_pShaderResourceView);
		}

		if (m_pUnorderedAccessView != NULL)
		{
			SAFE_RELEASE_RESOURCE(m_pUnorderedAccessView);
		}

		if (m_TargetViews.GetCount())
		{
			for( int i = 0; i < m_TargetViews.GetCount(); i++ )
			{
				if (m_TargetViews[i] != NULL)
					SAFE_RELEASE_RESOURCE(m_TargetViews[i]);
			}

			m_TargetViews.Reset();
		}

		GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
		m_CachedTexturePtr = NULL;

		if (m_LockableTexture)
		{
			GRCDEVICE.DeleteTexture(m_LockableTexture);
			m_LockableTexture = NULL;
		}
	}

	m_Width = (u16) width;
	m_Height = (u16) height;
	m_Type = type;
	//m_Texture = 0;
	m_Multisample = 0;
	m_ArraySize = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->ArraySize : 1;
	m_PerArraySliceRTV = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->PerArraySliceRTV : 0;
	m_MultisampleQuality = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1000)) ? params->MultisampleQuality : 0;;
	m_MipLevels = (u16) (params? params->MipLevels : 1);
	m_bUseAsUAV = ((params != NULL) && (GRCDEVICE.GetDxFeatureLevel()>= 1100))? params->UseAsUAV : false;

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	m_bHasBeenWrittenTo = false;
#endif

	bool useFloat = params? params->UseFloat : false;
	if (params && params->CreateAABuffer)
	{
		m_Multisample = (u8)params->Multisample;
		m_MultisampleQuality = (u8)params->MultisampleQuality;
	}

	grcTextureFormat tf = params? params->Format : grctfNone;
	if (tf == grctfNone)
	{
		if (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap || m_Type == grcrtDepthBufferCubeMap) 
		{
			tf = useFloat? grctfD32FS8 : grctfD24S8;
		}
		else
		{
			tf = useFloat? (bpp==32? grctfR32F : grctfR16F) : (bpp==32? grctfA8R8G8B8 : grctfR5G6B5);
		}
	}
	else if (m_Type == grcrtDepthBuffer)
	{
		AssertMsg( tf == grctfD32F || tf == grctfD24S8 || tf == grctfD32FS8, "Invalid depth buffer format" );
		if( tf != grctfD32F && tf != grctfD24S8 && tf != grctfD32FS8 )
		{
			tf = useFloat? grctfD32F : grctfD24S8; //(bpp == 32) ? grctfD24S8 : grctfD24S8;
		}
	}

	m_Format = (u8) tf;
	m_BitsPerPix = (u8)grcTextureFactoryDX11::GetD3D10BitsPerPixel((u32)grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format)); //(u8) bpp;
	m_Lockable = params? params->Lockable : false;
	m_LockableTexUpdated = false;
	DebugSetUnresolved();


	m_StereoRTMode = (GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled()) ? (params ? params->StereoRTMode : grcDevice::AUTO) : grcDevice::AUTO;
 
	CreateSurface();
}


void grcRenderTargetDX11::CreateSurface() {
	DXGI_FORMAT eFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));
	DXGI_FORMAT eTypelessFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTypelessFormat(eFormat));
	DXGI_FORMAT eTexFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::TranslateToTextureFormat(eFormat));

	bool bDepthFormat = (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap || m_Type == grcrtDepthBufferCubeMap) || 
						(m_Format == grctfD24S8) || (m_Format == grctfD32F || (m_Format == grctfD32FS8) || (m_Format == grctfD16));

	D3D11_TEXTURE2D_DESC oDesc;
	oDesc.ArraySize = m_ArraySize;
	//Only use typless format on depthstencil.
	oDesc.Format = bDepthFormat ? eTypelessFormat : eFormat;
	oDesc.Width = m_Width;
	oDesc.Height = m_Height;
	oDesc.MipLevels = m_MipLevels;
	oDesc.SampleDesc.Count = m_Multisample ? m_Multisample : 1;
	oDesc.SampleDesc.Quality = m_Multisample ? m_MultisampleQuality : 0;
	oDesc.Usage = (D3D11_USAGE)grcUsageDefault;
	oDesc.MiscFlags = (m_MipLevels > 1 ? grcResourceGenerateMips : grcResourceNone) | ((m_Type == grcrtCubeMap || m_Type == grcrtDepthBufferCubeMap) ? grcResourceTextureCube : grcResourceNone);
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


	if (m_StereoRTMode != grcDevice::AUTO)
		GRCDEVICE.ForceStereorizedRT(m_StereoRTMode);

	if (!m_Multisample)
	{
		ASSERT_ONLY(HRESULT uReturnCode;)
		{
			ASSERT_ONLY(uReturnCode = ) GRCDEVICE.GetCurrent()->CreateTexture2D(&oDesc, NULL, (ID3D11Texture2D**)&m_CachedTexturePtr);
		}
		Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %x (out of memory) occurred creating texture", uReturnCode);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
		Assert(m_CachedTexturePtr != NULL);

		if (m_Lockable)
		{
			// Also create a staging texture that can be locked.
			D3D11_TEXTURE2D_DESC oStagingDesc = oDesc;
			oStagingDesc.CPUAccessFlags = grcCPURead;
			oStagingDesc.Usage = (D3D11_USAGE)grcUsageStage;
			oStagingDesc.MipLevels = m_MipLevels;
			oStagingDesc.MiscFlags = grcResourceNone;
			oStagingDesc.BindFlags = grcBindNone;
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateTexture2D(&oStagingDesc, NULL, (ID3D11Texture2D**)&m_LockableTexture));
		}
	}
	else
	{
		// Check the requested level is available and clamp down to the best available.
		UINT availableQualityLevels;
		CHECK_HRESULT( GRCDEVICE.GetCurrent()->CheckMultisampleQualityLevels(oDesc.Format, oDesc.SampleDesc.Count, &availableQualityLevels) );
		Assertf(oDesc.SampleDesc.Quality < availableQualityLevels, "Invalid MSAA quality level %d specified for format %d, SampleCount %d. Will clamp to best available instead.", oDesc.SampleDesc.Quality, oDesc.Format, oDesc.SampleDesc.Count);
		if (oDesc.SampleDesc.Quality >= availableQualityLevels)
			oDesc.SampleDesc.Quality = availableQualityLevels-1;

		// create msaa texture 
		ASSERT_ONLY(HRESULT uReturnCode;)
		{
			ASSERT_ONLY(uReturnCode = ) GRCDEVICE.GetCurrent()->CreateTexture2D(&oDesc, NULL, (ID3D11Texture2D**)&m_CachedTexturePtr);
		}
		Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %x (out of memory) occurred creating texture", uReturnCode);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
		Assert(m_CachedTexturePtr != NULL);
	}

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

		ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateUnorderedAccessView( (ID3D11Resource*)m_CachedTexturePtr, &UAVDesc, (ID3D11UnorderedAccessView**)&m_pUnorderedAccessView);
		Assertf((uReturnCode == 0), "Error %ud occurred creating texture unordered access view", uReturnCode);
	}
	else
	{
		m_pUnorderedAccessView = NULL;
	}

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
			if (m_Multisample)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				oViewDesc.Texture2DMSArray.FirstArraySlice = 0;
				oViewDesc.Texture2DMSArray.ArraySize = oDesc.ArraySize;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				oViewDesc.Texture2DArray.MostDetailedMip = 0;
				oViewDesc.Texture2DArray.MipLevels = oDesc.MipLevels;
				oViewDesc.Texture2DArray.FirstArraySlice = 0;
				oViewDesc.Texture2DArray.ArraySize = oDesc.ArraySize;
			}
		}
		else
		{
			if (m_Multisample)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				oViewDesc.Texture2D.MostDetailedMip = 0;
				oViewDesc.Texture2D.MipLevels = oDesc.MipLevels;
			}
		}

		ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateShaderResourceView(m_CachedTexturePtr, &oViewDesc, (ID3D11ShaderResourceView**)&m_pShaderResourceView);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
	}

	if (m_PerArraySliceRTV)
	{
		Assert(m_TargetViews.GetCapacity()==0);
		m_TargetViews.Resize(m_PerArraySliceRTV*m_MipLevels);
		for (int i=0;i<m_TargetViews.GetCount();i++)
			m_TargetViews[i] = NULL;

		for( u32 i = 0; i < m_PerArraySliceRTV; i++ )
		{
			for (u32 j = 0; j < m_MipLevels; j++)
				CreateTargetView(i,j,m_ArraySize, DepthStencilRW);
		}
	}
	else
	{
		Assert(m_TargetViews.GetCapacity()==0);
		m_TargetViews.Resize(1*m_MipLevels);
		for (int i=0;i<m_TargetViews.GetCount();i++)
			m_TargetViews[i] = NULL;

		for (u32 i = 0; i < m_MipLevels; i++)
			CreateTargetView(0,i,m_ArraySize, DepthStencilRW);
	}

	SetPrivateData();
	//m_CachedTexturePtr = m_Texture;

	sm_TotalMemory += GetRequiredMemory();

	if ((m_Width != m_Height) &&
		(m_Width >= GRCDEVICE.GetWidth()) && 
		(m_Height >= GRCDEVICE.GetHeight()))
	{
		sm_TotalStereoMemory += GetRequiredMemory();
	}

#if RSG_PC
	if (m_StereoRTMode != grcDevice::AUTO)
		GRCDEVICE.ForceStereorizedRT(grcDevice::AUTO);
#endif
}

u32 grcRenderTargetDX11::GetDepthTextureFormat()
{
#if 1
	AssertMsg(false,"This needs serious work");
	return 0;
#else
	IDirect3D9* pDevice;

	if (FAILED(GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice)))
	{
		Printf("Unable to retrieve D3D Device");
		Quitf("D3D Error - Unable to retrieve D3D Device. Please re-boot your system");
		// return 0;
	}

	u32 uFormat = (u32)D3DFMT_D24S8;

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
												D3DDEVTYPE_HAL,
												D3DFMT_X8R8G8B8,
												D3DUSAGE_DEPTHSTENCIL,
												D3DRTYPE_TEXTURE,
												(D3DFORMAT)(MAKEFOURCC('I','N','T','Z')))))
	{
		uFormat = (u32)(MAKEFOURCC('I','N','T','Z'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('R','A','W','Z')))))
	{
		uFormat = (u32)(MAKEFOURCC('R','A','W','Z'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('D','F','2','4')))))
	{
		uFormat = (u32)(MAKEFOURCC('D','F','2','4'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('D','F','1','6')))))
	{
		uFormat = (u32)(MAKEFOURCC('D','F','1','6'));
	}
	pDevice->Release();

	return uFormat;
#endif
}

const grcDeviceView* grcRenderTargetDX11::GetUnorderedAccessView() const
{
#if !__FINAL
	if (m_pUnorderedAccessView == NULL)
	{
		Warningf("Render Target Format can not be used as UAV");
	}
#endif // __FINAL

	return m_pUnorderedAccessView;
}

grcTextureObject*	grcRenderTargetDX11::GetTexturePtr()
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

const grcTextureObject*	grcRenderTargetDX11::GetTexturePtr() const 
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

grcDeviceView* grcRenderTargetDX11::GetTextureView()
{
	if ((m_pShaderResourceView == NULL) && (m_StereoRTMode != grcDevice::AUTO))
	{
		CreateSurface();
	}
#if !__FINAL
	if (m_pShaderResourceView == NULL)
	{
		Warningf("Render Target %s Format can not be sampled as texture", m_Name);
	}
#endif // __FINAL

	return m_pShaderResourceView;
}

const grcDeviceView* grcRenderTargetDX11::GetTextureView() const
{
#if !__FINAL
	if (m_pShaderResourceView == NULL)
	{
		Warningf("Render Target %s Format can not be sampled as texture", m_Name);
	}
#endif // __FINAL

	return m_pShaderResourceView;
}

const grcDeviceView* grcRenderTargetDX11::GetTargetView(u32 uMip, u32 uLayer)
{
	//AssertVerify(SetTargetView(uMip, uLayer));
	Assert(uMip < m_MipLevels);
	Assertf(m_TargetViews[uLayer*m_MipLevels+uMip], "GetTargetView with layer %d and mip %d failed", uLayer, uMip);
	return m_TargetViews[uLayer*m_MipLevels+uMip];
}

bool grcRenderTargetDX11::CreateTargetView(u32 uArraySlice, u32 uMip, u32 uArraySize, DepthStencilFlags depthStencilReadOnly)
{
	int arrayIndex = uArraySlice*m_MipLevels+uMip;

	Assert(m_TargetViews[arrayIndex] == NULL);

	DXGI_FORMAT eFormat = static_cast<DXGI_FORMAT>(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));

	bool bDepthFormat = (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap || m_Type == grcrtDepthBufferCubeMap) || 
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
#if __ASSERT
			HRESULT uReturnCode;
#endif
			ASSERT_ONLY( uReturnCode = ) GRCDEVICE.GetCurrent()->CreateRenderTargetView((ID3D11Resource*)m_CachedTexturePtr, &oViewDesc, (ID3D11RenderTargetView**)&m_TargetViews[arrayIndex]);
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

			ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateDepthStencilView((ID3D11Resource*)m_CachedTexturePtr, &oViewDesc, (ID3D11DepthStencilView**)&m_TargetViews[arrayIndex]);
			Assertf((uReturnCode == 0), "Error %x occurred creating render target view", uReturnCode);
		}
	}
	return true;
}

bool grcRenderTargetDX11::IsReadOnlyFormat(u32 format)
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

bool grcRenderTargetDX11::SetTextureView(u32)
{
	return true;
}

void grcRenderTargetDX11::DeviceLost() 
{
	/*
	sm_TotalMemory -= GetRequiredMemory();
	if ((m_Width != m_Height) &&
		(m_Width >= GRCDEVICE.GetWidth()) && 
		(m_Height >= GRCDEVICE.GetHeight()))
	{
		sm_TotalStereoMemory -= GetRequiredMemory();
	}

	GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	*/
}


void grcRenderTargetDX11::DeviceReset() 
{
	/*
	CreateSurface();	
	*/
}

void grcRenderTargetDX11::UpdateLockableTexture(bool bInitUpdate)
{
	if (!bInitUpdate)
	{
		if (g_grcCurrentContext)
			g_grcCurrentContext->CopyResource(m_LockableTexture, m_CachedTexturePtr);

		m_LockableTexUpdated = true;
	}
	else
		m_LockableTexUpdated = false;
}

u32 grcRenderTargetDX11::GetImageFormat() const
{
	return grcTextureFactoryDX11::GetImageFormatStatic(grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)m_Format));
}

bool grcRenderTargetDX11::LockRect(int ASSERT_ONLY(layer), int /*ASSERT_ONLY*/(mipLevel),grcTextureLock &lock, u32 uLockFlags ) const
{
	Assert(layer == 0);
	Assertf(m_LockableTexture != 0 || (strcmp(m_Name, "BackBuf0" ) == 0) || (strcmp(m_Name, "BackBuffer")==0), "Render target not created with m_Lockable set");
	AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to Lock render target.");

	lock.MipLevel = mipLevel;
	lock.BitsPerPixel = GetBitsPerPixel();
	lock.Width = m_Width >> mipLevel;
	lock.Height = m_Height >> mipLevel;
	lock.RawFormat = GetFormat();
	lock.Layer = 0;
	lock.Pitch = lock.Width * lock.BitsPerPixel / 8;

	bool result = false;

#if __BANK
	if (m_LockableTexture == 0 && ((strcmp(m_Name, "BackBuf0" ) == 0) || (strcmp(m_Name, "BackBuffer") == 0)))
	{	// For the back buffer, create a staging texture that can be locked.
		D3D11_TEXTURE2D_DESC oStagingDesc;
		ID3D11Texture2D* backBuffer = (ID3D11Texture2D*)m_CachedTexturePtr;
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
			grcRenderTargetDX11* thisTarget = const_cast<grcRenderTargetDX11*>(this);
			thisTarget->SetPrivateData();
		}
	}
#endif

	if (m_LockableTexture && g_grcCurrentContext && (uLockFlags & grcsRead))
	{
		if (!m_LockableTexUpdated)
			g_grcCurrentContext->CopyResource(m_LockableTexture, m_CachedTexturePtr);

		D3D11_MAPPED_SUBRESOURCE	MappedResource;
		HRESULT						hr = g_grcCurrentContext->Map(m_LockableTexture, mipLevel, D3D11_MAP_READ, 0, &MappedResource);
		CHECK_HRESULT(hr);

		
		if (hr == S_OK)
		{
			// Calculate the offset.
			lock.Base = MappedResource.pData;
			// Set the pitch.
			lock.Pitch = MappedResource.RowPitch;
		}
		else
		{
			lock.Base = NULL;
			lock.Pitch = 0;
		}
		result = (hr == S_OK);
	}

	return result;
}

void grcRenderTargetDX11::UnlockRect(const grcTextureLock & lock) const
{
	Assertf(m_LockableTexture != 0, "Render target not created with m_Lockable set");
	AssertMsg(GRCDEVICE.CheckThreadOwnership(),"Thread that doesn't own the GPU trying to Unlock render target.");

	if (g_grcCurrentContext)
	{
		// Unmap the dynamic texture.
		g_grcCurrentContext->Unmap(m_LockableTexture, lock.MipLevel);
	}
}

void grcRenderTargetDX11::GenerateMipmaps()
{
	if (g_grcCurrentContext && (m_MipLevels > 1) && m_pShaderResourceView)
		g_grcCurrentContext->GenerateMips( (ID3D11ShaderResourceView*) m_pShaderResourceView );
}

bool grcRenderTargetDX11::Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex)
{
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
		const grcRenderTargetDX11* pSourceDX11 = static_cast<const grcRenderTargetDX11*>(pSource);
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

	return true;
}

bool grcRenderTargetDX11::CopyTo(grcImage* pImage, bool /* bInvert*/, u32 uPixelOffset)
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

#if DEBUG_TRACK_MSAA_RESOLVES
bool grcRenderTargetDX11::HasBeenResolvedTo(grcRenderTarget* resolveTo)
{
	grcTextureObject *const destTexturePtr = resolveTo->GetTexturePtr();

	if(GetMSAA() == grcDevice::MSAA_None)
		return true;

	return m_DebugLastResolvedObject == destTexturePtr;
}
#endif

void grcRenderTargetDX11::Resolve(grcRenderTarget* resolveTo, int destSliceIndex)
{
	Assert(resolveTo);
	grcTextureObject *const destTexturePtr = resolveTo->GetTexturePtr();

	if(GetMSAA() == grcDevice::MSAA_None)
		return;

#if DEBUG_TRACK_MSAA_RESOLVES
	Assertf(m_DebugLastResolvedObject != destTexturePtr, "Redundant resolve");
#endif

	grcTextureFormat dstfmt = GetFormat();
	grcTextureObject *const sourceTexturePtr = GetTexturePtr();
	Assert(dstfmt == static_cast<grcRenderTargetDX11*>(resolveTo)->GetFormat());
	Assert(sourceTexturePtr != destTexturePtr);
	Assert(dstfmt != grctfD24S8);
	Assert(dstfmt != grctfD32FS8);

	// Determine the format.

	DXGI_FORMAT fmt = (DXGI_FORMAT)grcTextureFactoryDX11::ConvertToD3DFormat(dstfmt);
	if (fmt == DXGI_FORMAT_UNKNOWN)
		Errorf("Support more formats for resolving subresource (dstfmt = %d)... \n", dstfmt);

	u32 dstSubResource = D3D11CalcSubresource(0, destSliceIndex, resolveTo->GetMipMapCount());
	u32 srcSubResource = D3D11CalcSubresource(0, 0,				 GetMipMapCount());

	// Perform the resolve.
	g_grcCurrentContext->ResolveSubresource(destTexturePtr,dstSubResource,sourceTexturePtr,srcSubResource,fmt);

#if DEBUG_TRACK_MSAA_RESOLVES
	if (!static_cast<grcTextureFactoryDX11&>(grcTextureFactory::GetInstance()).IsTargetLocked(this))
		m_DebugLastResolvedObject = destTexturePtr;
#endif
}

u32 grcRenderTargetDX11::GetRequiredMemory() const
{
	const u32 RequiredAlignment =  (GetWidth() < 256 || GetHeight() < 256) ? D3DRUNTIME_MEMORYALIGNMENT : D3D_AMD_LARGEALIGNMENT;
	// Only currently only has 1 mip level for render targets
	u32 uMem = (u32)((s64)(m_Multisample ? m_Multisample : 1 ) * (s64)GetWidth() * (s64)GetHeight() * (s64)m_BitsPerPix / (s64)8 * (s64)((m_Type == grcrtCubeMap || m_Type == grcrtDepthBufferCubeMap) ? 6 : 1)); 
	return (uMem + (RequiredAlignment - 1)) & ~(RequiredAlignment - 1);
}

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
void grcRenderTargetDX11::CheckDepthWriting()
{
	//Check to see if we're writing to the depth buffer. This is used to determine at what stage
	//we need to copy the depth buffer when we need to read from it.
	grcDepthStencilStateHandle handle = grcStateBlock::DSS_Active;

	grcDepthStencilStateDesc outDesc;
	grcStateBlock::GetDepthStencilStateDesc(handle, outDesc);

	if( outDesc.DepthWriteMask != 0 )
	{
		grcRenderTargetDX11 *depth = ((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->GetActiveDepthBuffer();
		if( depth )
		{
			depth->SetHasBeenWrittenTo(true);
		}
	}
}
#endif

}	// namespace rage

#endif	// RSG_PC && __D3D && __D3D11
