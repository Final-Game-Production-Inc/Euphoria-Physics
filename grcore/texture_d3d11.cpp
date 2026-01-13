#include	"texture_d3d11.h"
#include	"texturefactory_d3d11.h"

#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"
#include	"grcore/d3dwrapper.h"

#if	RSG_PC &&  __D3D11

#include	"device.h"
#include	"image.h"
#include	"viewport.h"

#include	"system/memory.h"
#include	"system/param.h"
#include	"grprofile/pix.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"
#include	"paging/rscbuilder.h"
#include	"profile/timebars.h"
#include	"vector/matrix34.h"
#include	"system/param.h"
#include	"system/xtl.h"
#include	<wbemidl.h>

#include	<d3d9.h>
#include	"wrapper_d3d.h"
// #include	<d3DX11async.h>
// DOM-IGNORE-BEGIN 
#if RSG_PC && NV_SUPPORT
#include "../../../3rdParty/NVidia/nvapi.h"
#include "../../../3rdParty/NVidia/nvstereo.h"
#endif

// DOM-IGNORE-END
XPARAM(usetypedformat);
XPARAM(noSRGB);
PARAM(usewhitemissingtexture,"Sets the missing texture to be white.");
PARAM(VisualMipMap, "Visualize Mip Map Layers");
PARAM(lockableResourcedTextures, "Resourced textures are made lock/unlock-able.");
PARAM(lockableResourcedTexturesCount, "Number of lockable resourced textures accomodated.");

#if __TOOL
namespace rage
{
	XPARAM(noquits);
}
#endif

#define ENFORCE_UNIQUE_TARGET_NAMES	0

#define USE_STAGING_TEXTURE 0 // USE_RESOURCE_CACHE
#if USE_STAGING_TEXTURE
#define STAGING_ONLY(x)	x
#define NONSTAGING_ONLY(x)
#else
#define STAGING_ONLY(x)
#define NONSTAGING_ONLY(x) x
#endif

#define SURFACE IDirect3DSurface9
#define TEXTURE IDirect3DBaseTexture9

namespace rage 
{

#define D3DFMT_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

extern GUID TextureBackPointerGuid;

/*======================================================================================================================================*/
/* class grcTextureDX11.																												*/
/*======================================================================================================================================*/

u32 grcTextureDX11::sm_ExtraDatasCount = 0;
grcTextureDX11_ExtraData *grcTextureDX11::sm_pExtraDatas = NULL;

// Called when we need to get a GPU update inserted into the "draw list" or similar.
__THREAD void (*grcTextureDX11::sm_pInsertUpdateGPUCopyCommandIntoDrawList)(void *, void *, u32 Layer, u32 MipLevel, bool uponCancelMemoryHint) = 0;
// Called when a buffer`s pending GPU updates need to be canceled.
__THREAD void (*grcTextureDX11::sm_pCancelPendingUpdateGPUCopy)(void *) = 0;
// Callback to provide memory for game thread buffer updates - must be live for 2 frames.
void *(*grcTextureDX11::sm_pAllocTempMemory)(u32 size) = 0;

// DX11 TODO:- Put this in grcTextureFactory::TextureCreateParams.
#define GRC_TEXTURE_D3D11_SYNC_MODE grcsTextureSync_Mutex

/*--------------------------------------------------------------------------------------------------*/
/* Creation/destruction related functions.															*/
/*--------------------------------------------------------------------------------------------------*/

#define MISSING_TEXTURE_SIZE_POWER_2	7
#define MISSING_TEXTURE_CHECK_SIZE_POWER_2	3

grcTextureDX11::grcTextureDX11(const char* pFilename, grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	grcImage *			pImage = NULL;
	grcDevice::Result	uReturnCode = 0;

	sysMemStartTemp();
	if	(strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
	{
		pImage = grcImage::Load(pFilename);
		int	w = pImage->GetWidth();
		int	h = pImage->GetHeight();

		if	((w & 3) || (h & 3))
		{
			grcErrorf("grcTexturePC - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);
			pImage->Release();
			pImage = NULL;
		}
		else
		{
			sysMemEndTemp();

			uReturnCode = Init(pFilename,pImage,params);

			if (uReturnCode!=0)
			{
				grcErrorf("Unable to create texture, filename: %s : return code %s", pFilename, uReturnCode);
				sysMemStartTemp();
				pImage->Release();
				pImage = NULL;
			}
		}
	}

	if (!pImage)
	{
#if !__FINAL
		if (strcmp(pFilename,"none")==0
		|| (strcmp(pFilename,"nonresident")==0 
#if !__FINAL
		&& PARAM_usewhitemissingtexture.Get()
#endif
		))
#endif
		{	// Create a white texture
			u32 texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFF;

			pImage = grcImage::Create(4, 4, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
			u32 *texels = (u32*) pImage->GetBits();

			for	(int i = 0; i < 16; i++)
				texels[i] = texel;
		}
#if !__FINAL
		else
		{
			// Create a multi-chequered texture of many colours.
			u32 colours[4] = { 0xffffffff, 0x00ff0000, 0x000000ff, 0x00ffff00 };
			pImage = grcImage::Create(0x1 << MISSING_TEXTURE_SIZE_POWER_2, 0x1 << MISSING_TEXTURE_SIZE_POWER_2, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
			u32 *texels = (u32*) pImage->GetBits();

			for(u32 i=0; i<(0x1 << MISSING_TEXTURE_SIZE_POWER_2); i++)
			{
				for(u32 j=0; j<(0x1 << MISSING_TEXTURE_SIZE_POWER_2); j++)
				{
					u32 u = i >> MISSING_TEXTURE_CHECK_SIZE_POWER_2;
					u32 v = j >> MISSING_TEXTURE_CHECK_SIZE_POWER_2;
					u32 index = (u & 0x1) | ((v & 0x1) << 0x1);
					*texels++ = colours[index];
				}
			}
		}
#endif
		Assert(pImage);
		sysMemEndTemp();

		uReturnCode = Init(pFilename,pImage,params);
		AssertMsg(uReturnCode==0,"Unable to create backup texture");

	}

	if (pImage)
	{
		sysMemStartTemp();
		pImage->Release();
		pImage = NULL;
		sysMemEndTemp();
	}
}


grcTextureDX11::grcTextureDX11(grcImage *pImage,grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	const char* name = "image";
#if __BANK || __RESOURCECOMPILER
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK || __RESOURCECOMPILER
	NOTFINAL_ONLY(grcDevice::Result initResult =) Init(name,pImage,params);
	AssertMsg(initResult==0, "Unable to create texture");

#if !__FINAL
	if (initResult != 0)
	{
		grcImage* checkerImage = grcImage::MakeChecker(4,Color32(1.f,1.f,1.f,1.f), Color32(0.f,0.f,0.f,0.f));
		Assert(checkerImage);

		initResult = Init(name,pImage,params);
		AssertMsg(initResult==0,"Unable to create checker texture, either");
		checkerImage->Release();
	}
#endif // __FINAL
}


grcTextureDX11::grcTextureDX11(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	CreateGivenDimensions(width, height, 1, eFormat, pBuffer, params);
}


grcTextureDX11::grcTextureDX11(u32 width, u32 height, u32 depth, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	CreateGivenDimensions(width, height, depth, eFormat, pBuffer, params);
}


grcTextureDX11::grcTextureDX11(datResource &rsc) : grcTexturePC(rsc) 
{
#if __PAGING
	rsc.PointerFixup(m_BackingStore);

	if (!rsc.IsDefragmentation())
	{
		m_nFormat = grcTextureFactoryDX11::TranslateDX9ToDX11Format(m_nFormat, m_IsSRGB_Byte);
		// Remove I'm not seeing this anywhere
		if (!Verifyf(m_nFormat != 0, "Format is 0!"))
		{
			m_nFormat = DXGI_FORMAT_R8G8B8A8_UINT;
		}

		if (m_BackingStore)
			CreateFromBackingStore();
	}

#if !REUSE_RESOURCE
	if( !m_InfoBits.m_OwnsBackingStore )
	{
		WIN32PC_ONLY(Assert((m_BackingStore == NULL) || sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_PHYSICAL)->GetPointerOwner(m_BackingStore) != NULL);)
		CHECK_FOR_PHYSICAL_PTR(m_BackingStore);
	}
#endif // !REUSE_RESOURCE
#endif // __PAGING
	SetPrivateData();
	//m_CachedTexturePtr = m_Texture;
}


grcTextureDX11::~grcTextureDX11	(void)
{
	DeleteResources();
}

void grcTextureDX11::DeleteResources()
{
	grcAssertf(((sm_pInsertUpdateGPUCopyCommandIntoDrawList != NULL) && (sm_pCancelPendingUpdateGPUCopy != NULL)) || ((sm_pInsertUpdateGPUCopyCommandIntoDrawList == NULL) && (sm_pCancelPendingUpdateGPUCopy == NULL)), "grcBufferD3D11::CleanUp()...No GPU update cancel function set!");

	// Texture might be updated and destroyed upon the same frame so we need to cancel pending GPU copy updates.
	if(sm_pCancelPendingUpdateGPUCopy)
	{
		sm_pCancelPendingUpdateGPUCopy((void *)this);
	}

#if __PAGING
	grcDeviceTexture *pStaging = GetStagingTexture();
	GRCDEVICE.DeleteTexture(pStaging);

	if(m_pExtraData)
	{
		FreeExtraData(m_pExtraData);
	}
#endif

	if (m_pShaderResourceView)
	{
		SAFE_RELEASE_RESOURCE(m_pShaderResourceView);
		m_pShaderResourceView = NULL;
	}

	m_nMipCount = m_nMipCount + m_CutMipLevels;
	m_Width <<= m_CutMipLevels;
	m_Height <<= m_CutMipLevels;
	m_nMipStride <<= m_CutMipLevels;

	UnlinkFromChain();
#if __PAGING
	// DX11 TODO:- Check to see if these two deletes boils down to the same thing.
	if(m_InfoBits.m_OwnsBackingStore == false)
	{
		physical_delete(m_BackingStore);
	}
	else
	{
		FreeCPUCopy();
	}
#endif

	GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	m_CachedTexturePtr = NULL;

	ASSERT_ONLY(m_InfoBits.m_HasBeenDeleted = true);
}


void grcTextureDX11::CreateFromBackingStore(bool PAGING_ONLY(bRecreate)) 
{
#if __PAGING	
	const u8 *pSource = NULL;
	bool bVolumeMap = (GetImageType() == grcImage::VOLUME);

	if (bRecreate == false)
	{
		pSource = (u8*)m_BackingStore;
		m_CutMipLevels = (u8)GetMipLevelScaleQuality(m_ImageType, m_Width, m_Height, m_nMipCount, m_nFormat);

		for (u32 i = 0; i < m_CutMipLevels; i++)
		{
			int	mipSize = GetTotalMipSize(i);
			pSource += mipSize; // Skip Mip Level
			m_nMipCount--;		// Reduce Number of Levels
		}	
		m_Width >>= m_CutMipLevels;
		m_Height >>= m_CutMipLevels;
		if (bVolumeMap) m_Depth >>= m_CutMipLevels;
		m_nMipStride >>= m_CutMipLevels;
	}

#if !RSG_DURANGO
	// This used to clamp to dimension of 4 for NVidia driver issue, but that's bollox, because it was clamping to 8.
	// Now it clamps to 2, because we can't generate a 1x1 loadable mips even if the life of our dear fans depended on it.
	if ((m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC5_SNORM) || // DXT1, DXT3, DXT5, DXT5A, DXN
		(m_nFormat >= DXGI_FORMAT_BC6H_TYPELESS && m_nFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		if (m_nMipCount > 1)
		{
			s32 iMaxMips = _FloorLog2(Min(m_Width, m_Height));
			if (iMaxMips > 1)
			{
				m_nMipCount = (u8)(((s32)m_nMipCount > (iMaxMips)) ? Max((iMaxMips),1) : m_nMipCount);
			}
		}
	}
#endif

	//--------------------------------------------------------------------------------------------------//

#if !__FINAL && !RSG_DURANGO // Ability to color up the mipmaps to visualize mip map usage
	static u32 uVisualMipMap = 0;
	PARAM_VisualMipMap.Get(uVisualMipMap);

	if (!bVolumeMap && uVisualMipMap)
	{
		typedef struct _Color565
		{
			u16	uRed	: 5;
			u16 uGreen	: 6;
			u16 uBlue	: 5;
		} Color565;
		
		const Color565 auColors[] = { { 31, 63, 31 }, { 31, 63, 30 }, // White	// Level 0
									{ 31, 63, 0  }, { 31, 62, 0  }, // Cyan		// Level 1
									{  0, 63, 31 }, {  0, 63, 30 }, // Yellow	// Level 2
									{  0, 63,  0 }, {  0, 62,  0 }, // Green	// Level 3
									{ 31,  0,  0 }, { 30,  0,  0 }, // Blue		// Level 4
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 5
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 6
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 7
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 8
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 9
									{  0,  0, 31 }, {  0,  0, 30 }, // Red		// Level 10
									{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 11
									{  0,  0, 31 }, {  0,  0, 30 }  // Red		// Level 12
							  };
		const u32 uNumColors = sizeof(auColors) / sizeof(Color565);
		/*				
		// Randomize Colors
		for (u32 uColor = 0; uColor < uNumColors; uColor++)
		{
			Color565 &oColor = auColors[uColor];
			oColor.uRed = rand() % 31;
			oColor.uGreen = rand() % 63;
			oColor.uBlue = rand() % 31;
		}
		*/

		u8* pbyImageData = (u8*)pSource;

		// Only support DXT1 and DXT5
		const bool bIsDXT1 = (m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC1_UNORM_SRGB);
		const bool bIsDXT5 = (m_nFormat >= DXGI_FORMAT_BC3_TYPELESS && m_nFormat <= DXGI_FORMAT_BC3_UNORM_SRGB);

		if (bIsDXT1 || bIsDXT5)
		{
			static u32 uViewMipLevels = uNumColors; // 1
			u32 uTextureHeight = GetHeight();
			u32 uTextureWidth  = GetWidth();
			u32 uMipLevels	   = Max(1U, Min(uViewMipLevels, (u32)GetMipMapCount()));
			const u32 uBlockSize = bIsDXT1 ? 8 : 16;
		
			for (u32 uMipLevel = 0; uMipLevel < uMipLevels; uMipLevel++)
			{
				for (u32 uHeight = 0; uHeight < uTextureHeight; uHeight += 8)
				{
					for (u32 uWidth = 0; uWidth < uTextureWidth; uWidth += 8)
					{
						Color565* puColor = (Color565*)(&pbyImageData[((uTextureWidth / 4) * (uHeight / 4) + // Height Offset
												 					   (uWidth / 4)) * // Width Offset
																	   uBlockSize]); // Size of DXT1 or DXT5 Block in bytes (2 * u16 + 4 by 4 * 2 bit blocks) + 4 Bytes for alpha on DXT5
						*puColor = auColors[uMipLevel * 2 + 0];
						puColor++;
						*puColor = auColors[uMipLevel * 2 + 1];
					}
				}

				pbyImageData += (uTextureWidth * uTextureHeight / 2);
				uTextureHeight = Max(uTextureHeight >> 1, 4U);
				uTextureWidth = Max(uTextureWidth >> 1, 4U);
			}
		}		
	}
#endif // __FINAL

	//--------------------------------------------------------------------------------------------------//

	// DX11 TODO:- Convert/confront these texture format types.
	static bool convert555 = true;
	if(convert555 && (m_nFormat == DXGI_FORMAT_B5G5R5A1_UNORM || m_nFormat ==  DXGI_FORMAT_B5G6R5_UNORM))
	{
		m_nFormat = DXGI_FORMAT_R16_UNORM;
	}

	//--------------------------------------------------------------------------------------------------//

	D3D11_SUBRESOURCE_DATA *paoSrcData = NULL;

	const u32 uArraySize = m_nMipCount;
	sysMemStartTemp();
	paoSrcData = rage_new D3D11_SUBRESOURCE_DATA[uArraySize];
	sysMemEndTemp();
	Assert(paoSrcData != NULL);

	// Visit each mip-map.
	for (u32 uMip = 0; uMip < m_nMipCount; uMip++)
	{
		const int iMipStride = GetStride(uMip);
		const int iMipRows = GetRowCount(uMip);
		const int iMipDepth = GetSliceCount(uMip);

		// Set up the sub-resource data.
		paoSrcData[uMip].pSysMem = pSource;
		paoSrcData[uMip].SysMemPitch = iMipStride;
		paoSrcData[uMip].SysMemSlicePitch = iMipStride*iMipRows;
		pSource += iMipStride*iMipRows*iMipDepth;
	}

	CREATE_INTERNAL_INFO CreateInfo = {0};
	CreateInfo.SubresourceCount = m_nMipCount;
	CreateInfo.pSubresourceData = paoSrcData;

	grcBindFlag extraBindFlags = grcBindNone;
	grcTextureCreateType CreateType = grcsTextureCreate_NeitherReadNorWrite;
	
	//If this is a scripted render target create it with the correct render target flags.
	if( m_Name )
	{
		char scriptRTName[10];
		strncpy(scriptRTName, m_Name, 9);
		scriptRTName[9] = '\0';
		u32 scriptHash = atStringHash(scriptRTName);
		if(scriptHash == ATSTRINGHASH("script_rt",0x5ad21149))
		{
			extraBindFlags = grcBindRenderTarget;
			CreateType = grcsTextureCreate_ReadWriteHasStaging;
			if ((m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC5_SNORM) || // DXT1, DXT3, DXT5, DXT5A, DXN
				(m_nFormat >= DXGI_FORMAT_BC6H_TYPELESS && m_nFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
			{
				m_nFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			}

			u32 typlessFormat = grcTextureFactoryDX11::TranslateToTypelessFormat(m_nFormat);
			m_nFormat = grcTextureFactoryDX11::TranslateToTextureFormat(typlessFormat);
		}
#if RSG_PC
		else if (m_ExtraFlags == 0)
		{
			// search for textures that always need to be writeable
			// avoids having to destroy and re-create them on the render thread later
			if(scriptHash == ATSTRINGHASH("pedmugsho",0x15e18c1a))
			{
				// non DXT1 mugshots need to be lockable for software compression
				if ( m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC1_UNORM_SRGB )
					CreateType = grcsTextureCreate_Write;
			}
			else if (scriptHash == ATSTRINGHASH("mp_crewpa",0x7c521f6a))
			{
				if(atStringHash(m_Name) == ATSTRINGHASH("mp_crewpalette",0xd88596c5))
				{
					CreateType = grcsTextureCreate_Write;
				}
			}
		}
#endif
	}

#if RSG_PC
	m_StereoRTMode = grcDevice::AUTO;
#endif

	if(PARAM_lockableResourcedTextures.Get())
		CreateType = grcsTextureCreate_ReadWriteHasStaging;

	bool IsFromBackingStore = true;
	if(m_ExtraFlags != 0)
	{
		CreateType = grcsTextureCreate_ReadWriteHasStaging;
#if RSG_PC
		IsFromBackingStore = false; // Need read access to force it to copy
		m_BackingStore = NULL; // Physical memory delete will toss this for us
#endif // RSG_PC
	}

	grcDevice::Result uReturnCode = CreateInternal(CreateInfo, CreateType, GRC_TEXTURE_D3D11_SYNC_MODE, extraBindFlags, IsFromBackingStore);

	sysMemStartTemp();
	delete [] paoSrcData;
	sysMemEndTemp();

	//--------------------------------------------------------------------------------------------------//

#if 1 //this should never happen, but if it does, print out some stuff
	if ((m_CachedTexturePtr==NULL) || (uReturnCode != S_OK))
	{
		Printf("CreateTexture() failed return code:0x%08x- m_Width=%d m_Height=%d m_nMipCount=%d m_nFormat=%d m_ImageType=%d", uReturnCode, m_Width, m_Height, m_nMipCount, m_nFormat, m_ImageType);
		//Quitf("D3D Error - Failed to create texture - Please restart the game");
		Assertf(0, "D3D Error %x - Failed to create texture", uReturnCode);
	}
#endif

#if __ASSERT && !REUSE_RESOURCE
	if( !m_InfoBits.m_OwnsBackingStore )
	{
		WIN32PC_ONLY(Assert(sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_PHYSICAL)->GetPointerOwner(m_BackingStore) != NULL);)
		CHECK_FOR_PHYSICAL_PTR(m_BackingStore);
	}
#endif

#endif // __PAGING
}


grcDevice::Result grcTextureDX11::Init( const char *pFilename, grcImage *pImage, grcTextureFactory::TextureCreateParams *params )
{
	u32	nFormat = (static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).GetD3DFormat(pImage));

	m_Name = grcSaveTextureNames ? StringDuplicate(pFilename) : 0;
	m_ImageType = (u8)pImage->GetType();
	m_Width = pImage->GetWidth();
	m_Height = pImage->GetHeight();
	m_Depth = pImage->GetDepth();
	m_nMipCount = (u8)(pImage->GetExtraMipCount() + 1);
	m_nFormat = nFormat;
	m_nMipStride = (u16)pImage->GetStride();
	m_InfoBits.m_IsSRGB = pImage->IsSRGB();
	m_CutMipLevels = 0;
	m_BackingStore = NULL;
	m_InfoBits.m_OwnsBackingStore = false;
	Assign(m_LayerCount,pImage->GetLayerCount()-1);
	m_ExtraFlags = pImage->IsSysMem() ? 1 : 0;
#if __64BIT
	m_ExtraFlagsPadding = 0;
#endif //__64BIT

#if __PAGING
	u8 *pStorage = NULL;
	int nStorageSize = 0;
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());

	if(paging) 
	{
		m_CachedTexturePtr = NULL;
		grcImage *tmp, *pLayer = pImage;
		while ( pLayer ) 
		{
			tmp = pLayer;
			while (tmp) 
			{
				nStorageSize += tmp->GetSize();
				tmp = tmp->GetNext();
			}
			pLayer = pLayer->GetNextLayer();
		}
		pStorage = (u8*)(m_BackingStore = physical_new(nStorageSize,256));
	}
	else
#endif
	{
		D3D11_SUBRESOURCE_DATA *paoSrcData = NULL;
		u32 ImageCount = pImage->GetLayerCount();
		Assert(pImage->GetType() != grcImage::CUBE || ImageCount%6 == 0);
		u32 SubresourceCount = m_nMipCount*ImageCount;

		sysMemStartTemp();
		paoSrcData = rage_new D3D11_SUBRESOURCE_DATA[SubresourceCount];
		sysMemEndTemp();
		Assert(paoSrcData != NULL);
		grcImage *poImage = pImage;

		// Visit each image in the source.
		for (u32 uArray = 0; uArray < ImageCount; uArray++)
		{
			Assert(poImage != NULL);
			grcImage* poMipImage = poImage;

			// Visit each mip-map in the current image.
			for (u32 uMip = 0; uMip < (u32)m_nMipCount; uMip++)
			{
				// Set up sub-resource info for it.
				Assert(poMipImage != NULL);
				paoSrcData[uMip + uArray * m_nMipCount].pSysMem = poMipImage->GetBits();
				paoSrcData[uMip + uArray * m_nMipCount].SysMemPitch = GetStride(uMip);
				paoSrcData[uMip + uArray * m_nMipCount].SysMemSlicePitch = GetStride(uMip) * GetRowCount(uMip);
				poMipImage = poMipImage->GetNext();
			}
			// Move onto the next image/cube face.
			poImage = poImage->GetNextLayer();
		}

		CREATE_INTERNAL_INFO CreateInfo = {0};
		CreateInfo.SubresourceCount = SubresourceCount;
		CreateInfo.pSubresourceData = paoSrcData;

		grcTextureCreateType CreateType = GetCreateTypeFromParams(params);
#if RSG_PC
		m_StereoRTMode = grcDevice::AUTO;
#endif
		grcDevice::Result uReturnCode = CreateInternal(CreateInfo, CreateType, GRC_TEXTURE_D3D11_SYNC_MODE, grcBindNone, false);

		sysMemStartTemp();
		delete [] paoSrcData;
		sysMemEndTemp();

		//--------------------------------------------------------------------------------------------------//

	#if __TOOL
		if(rage::PARAM_noquits.Get())
		{
			if(((HRESULT)uReturnCode) == E_OUTOFMEMORY)
			{
				grcErrorf("Error 0x%08x (out of memory) occurred creating texture %s", uReturnCode, pFilename);
			}
			else if(uReturnCode != 0)
			{
				grcErrorf("Error 0x%08x occurred creating texture %s", uReturnCode, pFilename);
			}
		}
		else
		{
	#endif
			Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %ud (out of memory) occurred creating texture %s", uReturnCode, pFilename);
			Assertf((uReturnCode == 0), "Error %ud occurred creating texture %s", uReturnCode, pFilename);
	#if __TOOL
		}
	#endif
#if __PAGING
		RESOURCE_CACHE_MANAGE_ONLY( if (!m_InfoBits.m_OwnsBackingStore) m_BackingStore = NULL);
#endif
		if(uReturnCode != 0)
		{
			return uReturnCode;
		}
	}
	return 0;
}


grcDevice::Result grcTextureDX11::CreateGivenDimensions(u32 width, u32 height, u32 depth, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params)
{
	Assert(grcTextureFactoryPC::HasInstance());
	Assert(params != NULL);

#if __PAGING
	m_BackingStore = NULL;
	m_InfoBits.m_OwnsBackingStore = false;
#endif

	m_InfoBits.m_IsSRGB = false; // pImage->IsSRGB(); // PC TODO - Implement
	m_CutMipLevels = 0;

#if __RESOURCECOMPILER && !__BANK
	m_Name = NULL; 
#else	
	m_Name = BANK_ONLY(grcSaveTextureNames ? GetCustomLoadName(NULL) :) NULL;
	if (m_Name != NULL)
		m_Name = StringDuplicate(m_Name);
#endif // __RESOURCECOMPILER && !__BANK

	m_ImageType = (depth == 1) ? (u8)grcImage::STANDARD : (u8)grcImage::VOLUME;
	Assign(m_LayerCount,0); //pImage->GetLayerCount()-1);
	m_Width = (u16)width;
	m_Height = (u16)height;
	m_Depth = (u16)depth;
	m_nMipCount = (u8)(params && params->MipLevels > 0 ? params->MipLevels : 1);
	m_nFormat = grcTextureFactoryDX11::ConvertToD3DFormat(eFormat);
	m_nMipStride = (u16)((GetBitsPerPixel()*m_Width)/8);
	m_pShaderResourceView = NULL;
	m_pExtraData = NULL;
	m_ExtraFlags = 0;
#if __64BIT
	m_ExtraFlagsPadding = 0;
#endif //__64BIT

#if RSG_PC
	m_StereoRTMode = (GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled()) ? (params ? params->StereoRTMode : grcDevice::AUTO) : grcDevice::AUTO;
#endif

	//--------------------------------------------------------------------------------------------------//

	CREATE_INTERNAL_INFO CreateInfo = {0};
	D3D11_SUBRESOURCE_DATA *paoSrcData = NULL;

	if (pBuffer)
	{
		const u8 *pSource = (const u8*)pBuffer;
		const u32 uArraySize = m_nMipCount;
		sysMemStartTemp();
		paoSrcData = rage_new D3D11_SUBRESOURCE_DATA[uArraySize];
		sysMemEndTemp();
		Assert(paoSrcData != NULL);

		for (u32 uMip = 0; uMip < m_nMipCount; uMip++)
		{
			const int iMipStride = GetStride(uMip);
			const int iMipRows = GetRowCount(uMip);
			const int iMipDepth = GetSliceCount(uMip);

			paoSrcData[uMip].pSysMem = pSource;
			paoSrcData[uMip].SysMemPitch = iMipStride;
			paoSrcData[uMip].SysMemSlicePitch = iMipStride*iMipRows;	
			pSource += iMipStride*iMipRows*iMipDepth;
		}

		CreateInfo.SubresourceCount = m_nMipCount;
		CreateInfo.pSubresourceData = paoSrcData;
	}
	else
	{
		CreateInfo.SubresourceCount = 0;
		CreateInfo.pSubresourceData = NULL;
	}

	grcBindFlag ExtraBindFlag = grcBindNone;

	if(params && params->Type == grcTextureFactory::TextureCreateParams::RENDERTARGET)
	{
		ExtraBindFlag = grcBindRenderTarget;
	}
	grcTextureCreateType CreateType = GetCreateTypeFromParams(params);
	grcDevice::Result Ret = CreateInternal(CreateInfo, CreateType, GRC_TEXTURE_D3D11_SYNC_MODE, ExtraBindFlag, false);

	sysMemStartTemp();
	if (paoSrcData != NULL)
		delete [] paoSrcData;
	sysMemEndTemp();

	return Ret;
}


grcDevice::Result grcTextureDX11::CreateInternal(CREATE_INTERNAL_INFO &CreateInfo, grcTextureCreateType CreateType, grcsTextureSyncType SyncType, grcBindFlag ExtraBindFlags, bool 
#if !REUSE_RESOURCE
												 IsFromBackingStore
#endif // !REUSE_RESOURCE
												 )
{
	// DX11 TODO:- Write the thread boundary stuff to avoid context locks! 
	(void *)SyncType;
	grcDevice::Result uReturnCode;
#if REUSE_RESOURCE
	CreateType = ProcessCreateType(CreateType);
#endif // REUSE_RESOURCE

	m_CachedTexturePtr = NULL;
	m_pExtraData = NULL;
	m_pShaderResourceView = NULL;

	// Record read/write access.
	m_InfoBits.m_ReadWriteAccess = CreateType;
	m_InfoBits.m_Dynamic = (CreateType >= grcsTextureCreate_ReadWriteDynamic) ? true : false;

	// Are we creating a 2D texture (or cubemap) ?
	if(m_ImageType != grcImage::VOLUME)
	{
		u32 Usage;
		u32 CPUAccessFlags;
		D3D11_TEXTURE2D_DESC oDesc;
		oDesc.ArraySize = m_LayerCount + 1;
		// DX11 TODO:- Convert/confront these texture format types.
		static bool convert555 = true;
		if(convert555 && (m_nFormat == DXGI_FORMAT_B5G5R5A1_UNORM || m_nFormat ==  DXGI_FORMAT_B5G6R5_UNORM))
		{
			m_nFormat = DXGI_FORMAT_R16_UNORM;
		}
		oDesc.Format = (DXGI_FORMAT)m_nFormat;
		oDesc.Width = m_Width;
		oDesc.Height = m_Height;
		oDesc.MipLevels = m_nMipCount;
		oDesc.SampleDesc.Count = 1;
		oDesc.SampleDesc.Quality = 0;
		oDesc.MiscFlags = (m_ImageType == grcImage::CUBE) ? grcResourceTextureCube : grcResourceNone;
		oDesc.BindFlags = grcBindShaderResource | ExtraBindFlags;
		GetDescUsageAndCPUAccessFlags(CreateType, Usage, CPUAccessFlags);
		oDesc.CPUAccessFlags = (UINT)CPUAccessFlags;
		oDesc.Usage = (D3D11_USAGE)Usage;

#if __ASSERT
		// Touch the memory before the driver gets it to see if its an access violation
		if (oDesc.Usage != (D3D11_USAGE)grcUsageDynamic  && CreateInfo.pSubresourceData)
		{
			u32 uFakeCount = 0;
			for (u32 uMip = 0; uMip < m_nMipCount; uMip++)
			{
				u8 *pbyBase = (u8*)CreateInfo.pSubresourceData[uMip].pSysMem;
				uFakeCount += pbyBase[0];
				uFakeCount += pbyBase[CreateInfo.pSubresourceData[uMip].SysMemSlicePitch - 1];
			}
			Assert(uFakeCount < ~0);
		}
#endif // __ASSERT

		// Create the copy the GPU will use.
		uReturnCode = GRCDEVICE.GetCurrent()->CreateTexture2D(&oDesc, 
#if !REUSE_RESOURCE			
							(oDesc.Usage != (D3D11_USAGE)grcUsageDynamic) ? CreateInfo.pSubresourceData : 
#endif // !REUSE_RESOURCE
							NULL, (ID3D11Texture2D**)&m_CachedTexturePtr);

		Assertf((HRESULT)uReturnCode != E_OUTOFMEMORY, "Error %x (out of memory) occurred creating texture '%s'!", uReturnCode, GetName());
		Assertf((uReturnCode == 0), "Error %x occurred creating texture '%s'!", uReturnCode, GetName());
		Assert(m_CachedTexturePtr != NULL);

		//--------------------------------------------------------------------------------------------------//

		m_pExtraData = NULL;

		if(DoesNeedStagingTexture(CreateType))
		{
			m_pExtraData = AllocExtraData(SyncType);
			FatalAssert(m_pExtraData != NULL);

			oDesc.BindFlags = grcBindNone;
			oDesc.Usage = (D3D11_USAGE)grcUsageStage;
			oDesc.CPUAccessFlags = grcCPUWrite | grcCPURead;

			if (m_StereoRTMode == grcDevice::STEREO)
			{
				oDesc.Width = m_Width * 2;
				oDesc.Height = m_Height + 1;
			}

#if __ASSERT
			if (CreateInfo.pSubresourceData)
			{
				u32 uFakeCount = 0;
				for (u32 uMip = 0; uMip < m_nMipCount; uMip++)
				{
					u8 *pbyBase = (u8*)CreateInfo.pSubresourceData[uMip].pSysMem;
					uFakeCount += pbyBase[0];
					uFakeCount += pbyBase[CreateInfo.pSubresourceData[uMip].SysMemSlicePitch - 1];
				}
				Assert(uFakeCount < ~0);
			}
#endif // __ASSERT

			// Create the staging texture.
			uReturnCode = GRCDEVICE.GetCurrent()->CreateTexture2D(&oDesc, 
#if !REUSE_RESOURCE
				CreateInfo.pSubresourceData
#else
				NULL
#endif // !REUSE_RESOURCE
				, (ID3D11Texture2D**)&m_pExtraData->m_pStagingTexture);
			Assertf((HRESULT)uReturnCode != E_OUTOFMEMORY, "Error %x (out of memory) occurred creating staging texture", uReturnCode);
			Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
			Assert(m_pExtraData->m_pStagingTexture != NULL);
		}

		//--------------------------------------------------------------------------------------------------//

		// Create the resource view.
		D3D11_SHADER_RESOURCE_VIEW_DESC oViewDesc;
		oViewDesc.Format = oDesc.Format;

		if (m_ImageType == grcImage::STANDARD ||
			m_ImageType == grcImage::DEPTH)
		{
			if (oDesc.ArraySize > 1)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				oViewDesc.Texture2DArray.MostDetailedMip = 0;
				oViewDesc.Texture2DArray.MipLevels = oDesc.MipLevels;
				oViewDesc.Texture2DArray.FirstArraySlice = 0;
				oViewDesc.Texture2DArray.ArraySize = oDesc.ArraySize;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				oViewDesc.Texture2D.MostDetailedMip = 0;
				oViewDesc.Texture2D.MipLevels = oDesc.MipLevels;
			}
		}
		else if (m_ImageType == grcImage::CUBE)
		{
			if (oDesc.ArraySize > 6)
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				oViewDesc.TextureCubeArray.MostDetailedMip = 0;
				oViewDesc.TextureCubeArray.MipLevels = oDesc.MipLevels;
				oViewDesc.TextureCubeArray.First2DArrayFace = 0;
				oViewDesc.TextureCubeArray.NumCubes = oDesc.ArraySize/6;
			}
			else
			{
				oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				oViewDesc.TextureCube.MostDetailedMip = 0;
				oViewDesc.TextureCube.MipLevels = oDesc.MipLevels;
			}
		}

		if((uReturnCode == 0) && (m_CachedTexturePtr != NULL))
		{
			uReturnCode = GRCDEVICE.GetCurrent()->CreateShaderResourceView((ID3D11Resource*)m_CachedTexturePtr, &oViewDesc, (ID3D11ShaderResourceView**)&m_pShaderResourceView);
			Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
		}
	}
	else // volume texture
	{
		u32 Usage;
		u32 CPUAccessFlags;
		D3D11_TEXTURE3D_DESC oDesc;
		oDesc.Depth = m_Depth;
		oDesc.Format = (DXGI_FORMAT)m_nFormat;
		oDesc.Width = m_Width;
		oDesc.Height = m_Height;
		oDesc.MipLevels = m_nMipCount;
		oDesc.MiscFlags = grcResourceNone;
		oDesc.BindFlags = grcBindShaderResource | ExtraBindFlags;
		GetDescUsageAndCPUAccessFlags(CreateType, Usage, CPUAccessFlags);
		oDesc.CPUAccessFlags = (UINT)CPUAccessFlags;
		oDesc.Usage = (D3D11_USAGE)Usage;

		// Create the copy the GPU will use.
		uReturnCode = GRCDEVICE.GetCurrent()->CreateTexture3D(&oDesc, 
#if !REUSE_RESOURCE
							(oDesc.Usage == (D3D11_USAGE)grcUsageImmutable) ? CreateInfo.pSubresourceData : 
#endif // !REUSE_RESOURCE
							NULL, (ID3D11Texture3D**)&m_CachedTexturePtr);

		Assertf((HRESULT)uReturnCode != E_OUTOFMEMORY, "Error %x (out of memory) occurred creating texture", uReturnCode);
		Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
		Assert(m_CachedTexturePtr != NULL);

		//--------------------------------------------------------------------------------------------------//

		m_pExtraData = NULL;

		if(DoesNeedStagingTexture(CreateType))
		{
			m_pExtraData = AllocExtraData(SyncType);

			oDesc.BindFlags = grcBindNone;
			oDesc.Usage = (D3D11_USAGE)grcUsageStage;
			oDesc.CPUAccessFlags = grcCPUWrite | grcCPURead;

			// Create the staging texture.
			uReturnCode = GRCDEVICE.GetCurrent()->CreateTexture3D(&oDesc, NULL, (ID3D11Texture3D**)&m_pExtraData->m_pStagingTexture);
			Assertf((HRESULT)uReturnCode != E_OUTOFMEMORY, "Error %x (out of memory) occurred creating texture", uReturnCode);
			Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
			Assert(m_pExtraData->m_pStagingTexture != NULL);
		}

		//--------------------------------------------------------------------------------------------------//

		D3D11_SHADER_RESOURCE_VIEW_DESC oViewDesc;
		oViewDesc.Format = oDesc.Format;
		oViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		oViewDesc.Texture3D.MostDetailedMip = 0;
		oViewDesc.Texture3D.MipLevels = oDesc.MipLevels;

		if((uReturnCode == 0) && (m_CachedTexturePtr != NULL))
		{
			uReturnCode = GRCDEVICE.GetCurrent()->CreateShaderResourceView((ID3D11Resource*)m_CachedTexturePtr, &oViewDesc, (ID3D11ShaderResourceView**)&m_pShaderResourceView);
			Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
		}
	}

	//--------------------------------------------------------------------------------------------------//
#if RSG_RSC
	m_InfoBits.m_OwnsBackingStore = false;
#endif // RSG_RSC
	m_InfoBits.m_Dirty = false;


	if (UsesBackingStoreForLocks(CreateType)
#if !REUSE_RESOURCE
		&& (IsFromBackingStore == false)
#endif // REUSE_RESOURCE
		)
	{
		// Calculate the amount of memory needed.
		u32 MemRequired = CalculateMemoryForASingleLayer()*(m_LayerCount + 1);

#if REUSE_RESOURCE
		if (CreateInfo.pSubresourceData != NULL && CreateType < grcsTextureCreate_ReadWriteDynamic)
		{
			m_BackingStore = NULL;
			m_InfoBits.m_Dirty = true;
		}
#endif // REUSE_RESOURCE

		// Allocate it.
		AllocCPUCopy(MemRequired);
		m_InfoBits.m_OwnsBackingStore = true;

		// Do we have an initial image ?
		if(CreateInfo.pSubresourceData)
		{
			// Copy this into the system memory version.
			D3D11_SUBRESOURCE_DATA *pSrc = CreateInfo.pSubresourceData;
			u8 *pDest = (u8 *)m_BackingStore;

			// Copy each layer.
			for(u32 uArray=0; uArray<(u32)(m_LayerCount + 1); uArray++)
			{
				// Copy each mip-level.
				for(u32 uMip=0; uMip<m_nMipCount; uMip++)
				{
					u8 *pDestSliceStart = pDest;
					u8 *pSliceStart = (u8 *)pSrc->pSysMem;
					u32 noOfSlices = GetSliceCount(uMip);

					u32 destMemPitch = GetStride(uMip);
					u32 destSliceMemPitch = destMemPitch*GetRowCount(uMip);

					u32 noOfRows = GetRowCount(uMip);

					u32 amountToCopyPerRow = min(pSrc->SysMemPitch, destMemPitch);

					// Copy each slice.
					for(u32 uSlice=0; uSlice<noOfSlices; uSlice++)
					{
						u8 *pSrcRowStart = pSliceStart;
						u8 *pDestRowStart = pDestSliceStart;

						for(u32 uRow=0; uRow<noOfRows; uRow++)
						{
							// Copy over a row.
							sysMemCpy((void *)pDestRowStart, (void *)pSrcRowStart, amountToCopyPerRow);
							// Move down a row in pSrc and pDest.
							pSrcRowStart += pSrc->SysMemPitch;
							pDestRowStart += destMemPitch;
						}
						// Move onto the next slice.
						pSliceStart += pSrc->SysMemSlicePitch;
						pDestSliceStart += destSliceMemPitch;
					}
					// Move onto the next mip-map.
					pDest += destSliceMemPitch*noOfSlices;
					pSrc++;
				}
			}
		}
	}

	SetPrivateData();
	//m_CachedTexturePtr = m_Texture;

	ASSERT_ONLY(m_InfoBits.m_HasBeenDeleted = false);
	return uReturnCode;
}


grcTextureCreateType grcTextureDX11::GetCreateTypeFromParams(grcTextureFactory::TextureCreateParams *pParams)
{
	grcTextureCreateType Ret = grcsTextureCreate_NeitherReadNorWrite;

	if(pParams != NULL)
	{
		if((pParams->LockFlags == grcsWrite) && (pParams->Memory == grcTextureFactory::TextureCreateParams::VIDEO))
		{
			// Write access via backing store then UpdateSubresource().
			Ret = grcsTextureCreate_Write;
		}
		// Interpret grcsDiscard as "use dynamic texture".
		else if((pParams->LockFlags == (grcsWrite | grcsDiscard)) && (pParams->Memory == grcTextureFactory::TextureCreateParams::VIDEO))
		{
			if(pParams->ThreadUseHint == grcTextureFactory::TextureCreateParams::THREAD_USE_HINT_CAN_BE_UPDATE_THREAD)
			{
				// Write access via backing store then Map()/Unmap() of dynamic texture.
				Ret = grcsTextureCreate_WriteDynamic;  
			}
			else
			{
				// Write access via Map()/Unmap() of dynamic texture.
				Ret = grcsTextureCreate_WriteOnlyFromRTDynamic; 
			}
		}
		else if((pParams->LockFlags & (grcsRead | grcsWrite)) && (pParams->Memory == grcTextureFactory::TextureCreateParams::VIDEO))
		{
			// Read access via backing store. Write access via backing store then Map()/Unmap() of dynamic texture.
			Ret = grcsTextureCreate_ReadWriteDynamic;
		}
		// Do we have read/write access specified OR SYSTEM (read access) OR STAGING (write access) (see grcTextureFactory::TextureCreateParams::Memory_t) ?
		else if((pParams->LockFlags & (grcsRead | grcsWrite)) || (pParams->Memory == grcTextureFactory::TextureCreateParams::SYSTEM) || (pParams->Memory == grcTextureFactory::TextureCreateParams::STAGING))
		{
			// Read access via backing store. Write access via backing store then UpdateSubresource(). Has staging texture for GPU->CPU operations.
			Ret = grcsTextureCreate_ReadWriteHasStaging;
		}
	}
	return Ret;
}

grcTextureCreateType grcTextureDX11::ProcessCreateType(grcTextureCreateType createType)
{
#if REUSE_RESOURCE
	createType = (createType == grcsBufferCreate_NeitherReadNorWrite) ? grcsTextureCreate_Write : createType;
#endif // REUSE_RESOURCE
	return createType;
}

void grcTextureDX11::GetDescUsageAndCPUAccessFlags(grcTextureCreateType createType, u32 &Usage, u32 &CPUAccessFlags)
{
	switch(createType)
	{
	case grcsTextureCreate_NeitherReadNorWrite:
		{
			Usage = static_cast<D3D11_USAGE>(grcUsageImmutable);
			CPUAccessFlags = grcCPUNoAccess;
			break;
		}
	case grcsTextureCreate_ReadWriteHasStaging:
		{
			Usage = static_cast<D3D11_USAGE>(grcUsageDefault);
			CPUAccessFlags = grcCPUNoAccess; // We gain access via a staging texture.
			break;
		}
	case grcsTextureCreate_ReadWriteDynamic:
	case grcsTextureCreate_WriteOnlyFromRTDynamic:
	case grcsTextureCreate_WriteDynamic:
		{
			Usage = static_cast<D3D11_USAGE>(grcUsageDynamic);
			CPUAccessFlags = grcCPUWrite;
			break;
		}
	case grcsTextureCreate_Write:
		{
			Usage = static_cast<D3D11_USAGE>(grcUsageDefault);
			CPUAccessFlags = grcCPUNoAccess;
			break;
		}
	default:
		{
			AssertMsg(NULL, "grcTextureDX11::CreateInternal()...Unknown create type type.");
			Usage = static_cast<D3D11_USAGE>(grcUsageImmutable);
			CPUAccessFlags = grcCPUNoAccess;
			break;
		}
	}
}


bool grcTextureDX11::DoesNeedStagingTexture(grcTextureCreateType CreateType)
{
	// Will we require CPU access ?
	if((CreateType == grcsTextureCreate_ReadWriteHasStaging) /*||
		(CreateType == grcsTextureCreate_ReadWriteDynamic)*/) // TODO - Oscar - Should be able to use system memory and avoid staging texture
	{
		return true;
	}
	return false;
}


bool grcTextureDX11::UsesBackingStoreForLocks(grcTextureCreateType CreateType)
{
	if(CreateType < grcsTextureCreate_WriteOnlyFromRTDynamic)
		return true;
	return false;
}



u32 grcTextureDX11::CalculateMemoryForASingleLayer() const
{
	return CalclulateMemoryUpToMipCount(m_nMipCount);
}


u32 grcTextureDX11::CalclulateMemoryUpToMipCount(u32 MipCount) const
{
	u32 Ret = 0;

	for (u32 uMip = 0; uMip < MipCount; uMip++)
	{
		Ret += GetTotalMipSize(uMip);
	}
	return Ret;
}


// Allocates memory for the CPU side copy buffer.
void grcTextureDX11::AllocCPUCopy(u32 Size)
{
	USE_MEMBUCKET(MEMBUCKET_RESOURCE);
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL);
	Assert(m_BackingStore == NULL);
	m_InfoBits.m_OwnsBackingStore = true;

	//m_BackingStore = rage_new u8[Size];
	ASSERT_ONLY(++sysMemStreamingCount);
	sysMemAllowResourceAlloc++;
	Size = (u32)pgRscBuilder::ComputeLeafSize(Size, false);
	m_BackingStore = pAllocator->Allocate(Size, 16);
	sysMemAllowResourceAlloc--;
	ASSERT_ONLY(--sysMemStreamingCount);
}


// Free the CPU copy.
void grcTextureDX11::FreeCPUCopy()
{
	if (!m_BackingStore)
		return;

	USE_MEMBUCKET(MEMBUCKET_RESOURCE);
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL);
	//delete(m_BackingStore);
	sysMemAllowResourceAlloc++;
	pAllocator->DeferredFree(m_BackingStore);
	sysMemAllowResourceAlloc--;
	m_BackingStore = NULL;
}

// Make internal texture to lockable textures (write only)
void grcTextureDX11::BeginTempCPUResource()
{
	if (m_InfoBits.m_ReadWriteAccess == grcsTextureCreate_NeitherReadNorWrite)
	{
		grcAssertf((m_InfoBits.m_ReadWriteAccess == grcsTextureCreate_NeitherReadNorWrite), "grcTextureDX11::ConvertToLockableTexture()...Must have immutable.\n");

		// delete the previous texture
		GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
		m_CachedTexturePtr = NULL;

		if (m_pShaderResourceView)
		{
			SAFE_RELEASE_RESOURCE(m_pShaderResourceView);
			m_pShaderResourceView = NULL;
		}

		// Calculate the amount of memory needed.
		u32 MemRequired = CalculateMemoryForASingleLayer()*(m_LayerCount + 1);

		// Allocate it.
		if( m_BackingStore == NULL )
 			m_BackingStore = rage_new u8[MemRequired];

		m_InfoBits.m_ReadWriteAccess = 1;
		m_InfoBits.m_OwnsBackingStore = 0x1;
	}
}

void grcTextureDX11::EndTempCPUResource()
{
	if (m_BackingStore)
	{
		grcTexturePC::PushTextureQuality(grcTexturePC::HIGH);
		CreateFromBackingStore();
		grcTexturePC::PopTextureQuality();
	}

	delete m_BackingStore;
	m_BackingStore = NULL;
	m_InfoBits.m_OwnsBackingStore = false;
}
/*--------------------------------------------------------------------------------------------------*/
/* Lock/Unlock.																						*/
/*--------------------------------------------------------------------------------------------------*/


bool grcTextureDX11::LockRect(int layer, int mipLevel, grcTextureLock &lock, u32 uLockFlags) const
{
	AssertMsg(m_InfoBits.m_HasBeenDeleted == false, "Texture has been deleted.");
	return LockRect_Internal(layer, mipLevel, lock, uLockFlags, false);
}

void grcTextureDX11::UnlockRect(const grcTextureLock &lock) const
{
	AssertMsg(m_InfoBits.m_HasBeenDeleted == false, "Texture has been deleted.");
	UnlockRect_Internal(lock, false);
}


// Performs a Lock().
bool grcTextureDX11::LockRect_Internal(int layer, int mipLevel, grcTextureLock &lock, u32 uLockFlags, bool AllowContextLock) const
{
	grcAssertf((m_InfoBits.m_ReadWriteAccess != grcsTextureCreate_NeitherReadNorWrite), "grcTextureDX11::LockRect_Internal()...Texture has no read/write access!\n");

	u32 LockType = uLockFlags & (grcsRead | grcsWrite);

	// Zero flags usually means read/write.
	if(uLockFlags == 0)
	{
		LockType = grcsRead | grcsWrite;
	}
	if(uLockFlags & grcsDiscard)
	{
		grcAssertf(((uLockFlags & grcsNoOverwrite) == 0), "grcTextureDX11::LockRect_Internal()..Can`t do grcsDiscard AND grcsNoOverwrite.");
		LockType |= grcsDiscard;
	}
	if(uLockFlags & grcsNoOverwrite)
	{
		LockType |= grcsNoOverwrite;
	}

	//--------------------------------------------------------------------------------------------------//

	// Fill in the lock structure.
	lock.MipLevel = mipLevel;
	lock.Pitch = GetStride(mipLevel);
	lock.BitsPerPixel = GetBitsPerPixel();
	lock.Width = m_Width >> mipLevel;
	lock.Height = m_Height >> mipLevel;
	lock.RawFormat = m_nFormat;
	lock.Layer = layer;

	if(UsesBackingStoreForLocks((grcTextureCreateType)m_InfoBits.m_ReadWriteAccess))
	{
		Assert(m_BackingStore != NULL);

		// Record for later.
		if (m_pExtraData)
		{
			
			m_pExtraData->m_LockFlags = LockType;

			// Only perform thread boundary precautions upon writes and NOT on the "render" thread.
			if((LockType & grcsWrite) && (GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == false) && (AllowContextLock == false))
			{
				grcAssertf(m_pExtraData != NULL, "grcTextureDX11::LockRect_Internal()...Texture has no extra data (required for lock/unlocks!\n");

				// Adhere to specified thread boundary mechanism.
				if(m_pExtraData->m_SyncType == grcsTextureSync_Mutex)
				{
					PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture LockRect_Internal - grcsTextureSync_Mutex", 0.4f);
					sysIpcLockMutex(m_pExtraData->m_Mutex);
				}
				else if(m_pExtraData->m_SyncType == grcsTextureSync_DirtySemaphore)
				{
					PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture LockRect_Internal - grcsTextureSync_DirtySemaphore", 0.4f);
					sysIpcWaitSema(m_pExtraData->m_DirtySemaphore);
				}
			}
		}
		if (m_BackingStore == NULL)
		{
			return false;
		}

		// Calculate the offset into our CPU copy.
		lock.Base = (void *)((char *)m_BackingStore + CalculateMemoryForASingleLayer()*layer + CalclulateMemoryUpToMipCount(mipLevel));
	}
	else
	{
		PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture LockRect_Internal - Map Discard", 0.4f);
		grcAssertf(m_InfoBits.m_ReadWriteAccess == grcsTextureCreate_WriteOnlyFromRTDynamic, "grcTextureDX11::LockRect_Internal()...Expected write only access!\n");
		grcAssertf((GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread()==true), "grcTextureDX11::LockRect_Internal()...Can only called from render thread,");

		D3D11_MAPPED_SUBRESOURCE MappedResource;

		// Map into the dynamic texture.
		D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
		HRESULT hRes = g_grcCurrentContext->Map(m_CachedTexturePtr, layer*m_nMipCount + mipLevel, mapType, 0, &MappedResource);
		grcAssertf(hRes == S_OK, "grcTextureDX11::UpdateGPUCopy_Internal()...Failed to Map() resource (error = %08x).", hRes);

		if (hRes == S_OK)
		{
			// Calculate the offset.
			lock.Base = (void *)((char *)MappedResource.pData + CalculateMemoryForASingleLayer()*layer + CalclulateMemoryUpToMipCount(mipLevel));
			// Set the pitch.
			lock.Pitch = MappedResource.RowPitch;
		}
		else
		{
			lock.Base = NULL;
			lock.Pitch = 0;
			return false;
		}
	}
	return true;
}


// Performs the UnLock().
void grcTextureDX11::UnlockRect_Internal(const grcTextureLock &lock, bool AllowContextLocks) const
{
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture UnlockRect_Internal", 0.4f);
	m_InfoBits.m_Dirty = false;
	grcAssertf((m_InfoBits.m_ReadWriteAccess != grcsTextureCreate_NeitherReadNorWrite), "grcTextureDX11::LockRect_Internal()...Texture has no read/write access!\n");

	if(UsesBackingStoreForLocks((grcTextureCreateType)m_InfoBits.m_ReadWriteAccess))
	{
		if (m_pExtraData)
		{
			grcAssertf(m_pExtraData != NULL, "grcTextureDX11::LockRect_Internal()...Texture has no extra data (required for lock/unlocks!\n");

			if(m_pExtraData->m_LockFlags == grcsRead)
			{
				// We have nothing to for read lock/unlocks.
				return;
			}
		}

		// Are we on the "render" thread ? 
		if(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == true)
		{			
			// Update the GPU copy.
			UpdateGPUCopy_Internal(lock.Layer, lock.MipLevel, lock.Base);
		}
		// Are we allowed to lock the main context ?
		else if(AllowContextLocks)
		{
			GRCDEVICE.LockContext();
			// Update the GPU copy.
			UpdateGPUCopy_Internal(lock.Layer, lock.MipLevel, lock.Base);
			GRCDEVICE.UnlockContext();
		}
		// Use the route which expects the info to migrate over threads.
		else
		{
			if (m_pExtraData != NULL)
			{
				// The GPU side needs to be updated...Perform specified thread boundary operations.
				if(m_pExtraData->m_SyncType == grcsTextureSync_Mutex)
				{
					sysIpcUnlockMutex(m_pExtraData->m_Mutex);
				}
			}

			// Has a mechanism for dealing with GPU updates been registered ?
			if(sm_pInsertUpdateGPUCopyCommandIntoDrawList)
			{	
				bool isTempMemory = false;
				void *pGPUImage = lock.Base;

				// Are we to copy the data to prevent it being over-written by any subsequent lock()/unlock()s this frame ?
				if(m_pExtraData != NULL && m_pExtraData->m_SyncType == grcsTextureSync_CopyData)
				{
					u32 AmountToCopy = GetStride(lock.MipLevel)*(m_Height >> lock.MipLevel);
					pGPUImage = AllocTempMemory(AmountToCopy);

					// NOTE:- A fail safe for this temp allocation returning NULL is to not update the GPU version.
					if(pGPUImage)
					{
						sysMemCpy(pGPUImage, lock.Base, AmountToCopy);
					}
					isTempMemory = true;
				}
				// Get a command inserted into the draw list, or similar, to deal with the update.
				sm_pInsertUpdateGPUCopyCommandIntoDrawList((void *)this, pGPUImage, lock.Layer, lock.MipLevel, isTempMemory);
			}
			else
			{
				grcWarningf("grcTextureDX11::UnlockRect_Internal()...GPU version of buffer not updated!\n");
			}
		}
	}
	else
	{
		grcAssertf(m_InfoBits.m_ReadWriteAccess == grcsTextureCreate_WriteOnlyFromRTDynamic, "grcTextureDX11::LockRect_Internal()...Expected write only access!\n");
		grcAssertf((GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread()==true), "grcTextureDX11::LockRect_Internal()...Can only called from render thread,");

		// Unmap the dynamic texture.
		g_grcCurrentContext->Unmap(m_CachedTexturePtr, lock.MipLevel);
	}
}


// Updates the GPU copy.
void grcTextureDX11::UpdateGPUCopy(u32 Layer, u32 MipLevel, void *pBase) const
{
	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcTextureDX11::UpdateGPUCopy()...Can only call this from the render thread!\n");
	UpdateGPUCopy_WithThreadSyncing(Layer, MipLevel, pBase);
}


void grcTextureDX11::UpdateGPUCopy_WithThreadSyncing(u32 Layer, u32 MipLevel, void *pBase) const
{
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture UpdateGPUCopy_WithThreadSyncing", 0.4f);
	// Adhere to specified thread boundary mechanism.
	if((m_pExtraData != NULL) && m_pExtraData->m_SyncType == grcsTextureSync_Mutex)
	{
		sysIpcLockMutex(m_pExtraData->m_Mutex);
	}

	UpdateGPUCopy_Internal(Layer, MipLevel, pBase);

	// Perform specified thread boundary operations.
	if (m_pExtraData != NULL)
	{
		if(m_pExtraData->m_SyncType == grcsTextureSync_Mutex)
		{
			sysIpcUnlockMutex(m_pExtraData->m_Mutex);
		}
		else if(m_pExtraData->m_SyncType == grcsTextureSync_DirtySemaphore)
		{
			sysIpcSignalSema(m_pExtraData->m_DirtySemaphore);
		}
	}
}


void grcTextureDX11::UpdateGPUCopy_Internal(u32 Layer, u32 MipLevel, void *pBase) const
{
#if USE_TELEMETRY
	static char szTexName[256];
	static const char szNull[] = "NULL";
	formatf(szTexName, sizeof(szTexName) - 1, "Texture UpdateGPUCopy_Internal %s W:%d H:%d",GetName() ? GetName() : szNull, GetWidth(), GetHeight());
	PF_AUTO_PUSH_TIMEBAR_BUDGETED(szTexName, 0.4f);
#endif

	if (!m_InfoBits.m_Dynamic)
	{
		// Update the main texture.
#if __ASSERT
		// Touch the memory before the driver gets it to see if its an access violation
		u8 *pbyBase = (u8*)pBase;
		u32 uFakeCount = pbyBase[0];
		uFakeCount+= pbyBase[GetStride(MipLevel) * GetRowCount(MipLevel) - 1];
		Assert(uFakeCount < ~0);
#endif // __ASSERT
		D3D11_BOX DstBox = { 0, 0, Layer, m_Width >> MipLevel, m_Height >> MipLevel, Layer + 1 };
		g_grcCurrentContext->UpdateSubresource(GetCachedTexturePtr(), MipLevel, &DstBox, pBase, GetStride(MipLevel), GetStride(MipLevel) * GetRowCount(MipLevel));
	}
	else
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;

		// Map into the texture (either the staging one or the main one).
		u32 subresource = Layer*m_nMipCount + MipLevel;
		HRESULT hRes = g_grcCurrentContext->Map((m_pExtraData == NULL || m_pExtraData->m_pStagingTexture == NULL) ? m_CachedTexturePtr : m_pExtraData->m_pStagingTexture, subresource, (m_pExtraData == NULL) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_READ_WRITE, 0, &MappedResource);
		grcAssertf(hRes == S_OK, "grcTextureDX11::UpdateGPUCopy_Internal()...Failed to Map() resource (error = %08x).", hRes);
		if (hRes != S_OK)
		{
			Warningf("Failed to UpdateGPUCopy_Internal");
			return;
		}
	
		m_InfoBits.m_Dirty = false;
		// Copy the data into it.
		char *pSrc = (char *)pBase;
		char *pDest = (char *)MappedResource.pData;
		u32 MipStride = GetStride(MipLevel);

		// NOTE:- A fail safe for this temp allocation returning NULL is to not update the GPU version.
		if(pBase)
		{
			if (m_StereoRTMode == grcDevice::STEREO)
			{
				grcAssertf(m_ImageType != grcImage::VOLUME, "grcTextureDX11::UpdateGPUCopy_Internal()...Volume textures not supported yet.");

				// assume no mipmap for stereo texture
				for (int i = 0; i < m_Height; i++)
				{
					sysMemCpy(pDest,pSrc,MipStride);
					sysMemCpy(pDest + m_nMipStride,pSrc,MipStride);
					pSrc += MipStride;
					pDest += MappedResource.RowPitch;
				}
#if NV_SUPPORT
				nv::stereo::LPNVSTEREOIMAGEHEADER header = (nv::stereo::LPNVSTEREOIMAGEHEADER)((unsigned char*)MappedResource.pData + MappedResource.RowPitch * m_Height);
				header->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
				header->dwWidth = m_Width * 2;
				header->dwHeight = m_Height + 1;
				header->dwBPP = (GetBitsPerPixel() / 8);
				header->dwFlags = 0;
#endif
			}
			else
			{
				for(int j=0; j < GetSliceCount(MipLevel); j++)
				{
					char *pSrcInner = pSrc;
					char *pDestInner = pDest;

					for(int i=0; i < GetRowCount(MipLevel); i++)
					{
						sysMemCpy(pDestInner, pSrcInner, MipStride);
						pSrcInner += MipStride;
						pDestInner += MappedResource.RowPitch;
					}
					pSrc += MipStride*GetRowCount(MipLevel);
					pDest += MappedResource.DepthPitch;
				}
			}
		}

		g_grcCurrentContext->Unmap((m_pExtraData == NULL || m_pExtraData->m_pStagingTexture == NULL) ? m_CachedTexturePtr : m_pExtraData->m_pStagingTexture, subresource);

		//--------------------------------------------------------------------------------------------------//

		// Now update the main texture if the mapping above went into the staging texture.
		if (m_pExtraData && m_pExtraData->m_pStagingTexture != NULL)
		{
			if (m_StereoRTMode == grcDevice::STEREO)
			{
				D3D11_BOX stereoSrcBox = { 0, 0, 0, m_Width, m_Height, 1 };
				g_grcCurrentContext->CopySubresourceRegion(static_cast <ID3D11Resource *>(m_CachedTexturePtr), 0, 0, 0, 0, static_cast <ID3D11Resource *>(m_pExtraData->m_pStagingTexture), 0, &stereoSrcBox);
			}
			else
				g_grcCurrentContext->CopyResource(m_CachedTexturePtr, m_pExtraData->m_pStagingTexture);
		}
	}
}

#if RSG_PC
void grcTextureDX11::UpdateGPUCopyFromBackingStore()
{
	Assert(m_BackingStore != NULL);

	const char *pSrc = (char *)m_BackingStore;
	for (u32 Layer = 0; Layer < GetLayerCount(); Layer++)
	{
		for (u8 MipLevel = 0; MipLevel < m_nMipCount; MipLevel++)
		{
			u32 MipStride = GetStride(MipLevel);
			UpdateGPUCopy_Internal(Layer, MipLevel, (void*)pSrc);
			pSrc += MipStride*GetRowCount(MipLevel);
		}
	}
}

void grcTextureDX11::UpdateGPUCopy()
{
#if REUSE_RESOURCE
	if (!m_InfoBits.m_Dirty)
		return;

	Assert(m_BackingStore != NULL);
#if 1 // __DEV
	if (m_BackingStore == NULL)
	{
		m_InfoBits.m_Dirty = false;
		return;
	}
#endif // __DEV
	const char *pSrc = (char *)m_BackingStore;

	for (u32 Layer = 0; Layer < GetLayerCount(); Layer++)
	{
		for (u8 MipLevel = 0; MipLevel < m_nMipCount; MipLevel++)
		{
			u32 MipStride = GetStride(MipLevel);
			UpdateGPUCopy_Internal(Layer, MipLevel, (void*)pSrc);
			pSrc += MipStride*GetRowCount(MipLevel);
		}
	}

	if (!m_InfoBits.m_Dynamic && 
		(m_pExtraData != NULL && m_pExtraData->m_pStagingTexture != NULL)
		)
	{ // Update the content of the staging resources as well
		m_InfoBits.m_Dynamic = true;

		const char *pSrc = (char *)m_BackingStore;

		for (u32 Layer = 0; Layer < GetLayerCount(); Layer++)
		{
			for (u8 MipLevel = 0; MipLevel < m_nMipCount; MipLevel++)
			{
				u32 MipStride = GetStride(MipLevel);
				UpdateGPUCopy_Internal(Layer, MipLevel, (void*)pSrc);
				pSrc += MipStride*GetRowCount(MipLevel);
			}
		}

		m_InfoBits.m_Dynamic = false;
	}

	FreeCPUCopy();
	m_InfoBits.m_Dirty = false;
#endif // REUSE_RESOURCE
}

void grcTextureDX11::StripImmutability(bool bPreserveTextureData)
{
	// Make copies of the underlying D3D11 texture
	grcTextureObject* pPrevTexture = m_CachedTexturePtr;
	grcDeviceView* pPrevShaderResourceView = m_pShaderResourceView;

	D3D11_TEXTURE2D_DESC desc;
	static_cast<ID3D11Texture2D*>(pPrevTexture)->GetDesc(&desc);
	if ( desc.Usage != D3D11_USAGE_IMMUTABLE )
	{
		return;
	}
	
	// It's OK, we've cached these pointers above
	m_CachedTexturePtr = NULL;
	m_pShaderResourceView = NULL;

	// Create a new texture without the immutable flag
	CREATE_INTERNAL_INFO CreateInfo;
	CreateInfo.SubresourceCount = m_nMipCount;
	CreateInfo.pSubresourceData = NULL;

	grcDevice::Result uReturnCode = CreateInternal(CreateInfo, grcsTextureCreate_Write, GRC_TEXTURE_D3D11_SYNC_MODE, grcBindNone, false);
	if ( uReturnCode != S_OK )
	{
		Printf("CreateTexture() failed return code: 0x%08x- m_Width=%d m_Height=%d m_nMipCount=%d m_nFormat=%d m_ImageType=%d", uReturnCode, m_Width, m_Height, m_nMipCount, m_nFormat, m_ImageType);
		Assertf(0, "D3D Error %x - Failed to create texture", uReturnCode);

		// Try to undo the damage
		m_CachedTexturePtr = pPrevTexture;
		m_pShaderResourceView = pPrevShaderResourceView;
		return;
	}
	

	// If we are to preserve the texture's contents, we need to copy the old texture's data into the new texture
	if ( bPreserveTextureData )
	{
		g_grcCurrentContext->CopyResource(m_CachedTexturePtr, pPrevTexture);
	}

	// Delete the previous texture and resource view
	GRCDEVICE.DeleteTexture(pPrevTexture);
	SAFE_RELEASE_RESOURCE(pPrevShaderResourceView);
}

bool grcTextureDX11::IsImmutable()
{
	D3D11_TEXTURE2D_DESC desc;
	static_cast<ID3D11Texture2D*>(m_CachedTexturePtr)->GetDesc(&desc);
	return (desc.Usage == D3D11_USAGE_IMMUTABLE);
}


#endif // RSG_PC

// Updates the CPU copy.
bool grcTextureDX11::UpdateCPUCopy(bool dontStallWhenDrawing /*= false*/)
{
#if USE_TELEMETRY
	static char szTexName[256];
	static const char szNull[] = "NULL";
	formatf(szTexName, sizeof(szTexName) - 1, "Texture UpdateCPUCopy %s W:%d H:%d",GetName() ? GetName() : szNull, GetWidth(), GetHeight());
	PF_AUTO_PUSH_TIMEBAR_BUDGETED(szTexName, 0.4f);
#endif

	GRCDEVICE.LockContext();
	
	bool result = UpdateCPUCopy_Internal(dontStallWhenDrawing);

	GRCDEVICE.UnlockContext();

	return result;
}


// Updates the CPU copy.
bool grcTextureDX11::UpdateCPUCopy_OnlyFromRenderThread(bool dontStallWhenDrawing)
{
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture UpdateCPUCopy_OnlyFromRenderThread", 0.4f);

	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcTextureDX11::UpdateCPUCopy()... Can only call this from the render thread!\n");
	return UpdateCPUCopy_Internal(dontStallWhenDrawing);
}

bool grcTextureDX11::UpdateCPUCopy_Internal(bool dontStallWhenDrawing)
{
	CopyFromGPUToStagingBuffer();
	return MapStagingBufferToBackingStore(dontStallWhenDrawing);
}

void  grcTextureDX11::CopyFromGPUToStagingBuffer()
{
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("Texture CopyFromGPUToStagingBuffer", 0.4f);
#if REUSE_RESOURCE
	UpdateGPUCopy();
#endif // REUSE_RESOURCE

	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcTextureDX11::UpdateCPUCopy()... Can only call this from the render thread!\n");

	grcAssertf(m_pExtraData, "Extra data is NULL");
	grcAssertf(m_pExtraData->m_pStagingTexture, "Extra data staging texture is NULL");
	if ((m_pExtraData->m_pStagingTexture != NULL) && (m_CachedTexturePtr != NULL))
		g_grcCurrentContext->CopyResource(m_pExtraData->m_pStagingTexture,	m_CachedTexturePtr);
}

bool grcTextureDX11::MapStagingBufferToBackingStore(bool dontStallWhenDrawing)
{
	#define STALL_TIMERS 0

#if STALL_TIMERS
	if (dontStallWhenDrawing) {
		PF_PUSH_TIMEBAR_BUDGETED("Texture No Stall MapStagingBufferToBackingStore", 0.4f);
	} else {
		PF_PUSH_TIMEBAR_BUDGETED("Texture MapStagingBufferToBackingStore", 0.4f);
	}
#endif // STALL_TIMERS
	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcTextureDX11::MapStagingBufferToBackingStore()... Can only call this from the render thread!\n");
	grcAssertf(m_pExtraData, "Extra data is NULL");
	grcAssertf(m_pExtraData->m_pStagingTexture, "Extra data staging texture is NULL");

	// DX11 TODO:- 3D textures.
	grcAssertf(m_ImageType != grcImage::VOLUME, "grcTextureDX11::MapStagingBufferToBackingStore()... Volume textures not supported yet.");
	if (!m_BackingStore)
	{
		AllocCPUCopy(CalculateMemoryForASingleLayer() * GetLayerCount());
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	char *pDest = (char *)m_BackingStore;
	
	for (u32 Layer = 0; Layer < GetLayerCount(); Layer++)
	{
		for (u8 MipLevel = 0; MipLevel < m_nMipCount; MipLevel++)
		{
			// Map into the staging texture.
			u32 subresource = Layer*m_nMipCount + MipLevel;
			HRESULT hRes = g_grcCurrentContext->Map(m_pExtraData->m_pStagingTexture, subresource, D3D11_MAP_READ, dontStallWhenDrawing ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, &MappedResource);
			if( hRes == DXGI_ERROR_WAS_STILL_DRAWING )
			{
#if STALL_TIMERS
				PF_POP_TIMEBAR();
#endif // STALL_TIMERS
				return false;
			}
			grcAssertf(hRes == S_OK, "grcTextureDX11::MapStagingBufferToBackingStore()... Failed to Map() <%s> resource (error = %08x) (%u,%u).", GetName(), hRes, subresource, dontStallWhenDrawing);
			if (hRes != S_OK)
			{
#if STALL_TIMERS
				PF_POP_TIMEBAR();
#endif // STALL_TIMERS
				return false;
			}

			// Copy the data into it.
			char *pSrc = (char *)MappedResource.pData;
			u32 MipStride = GetStride(MipLevel);

			for(int i=0; i<GetRowCount(MipLevel); i++)
			{
				sysMemCpy(pDest, pSrc, MipStride);
				pSrc += MappedResource.RowPitch;
				pDest += MipStride;
			}

			g_grcCurrentContext->Unmap(m_pExtraData->m_pStagingTexture, subresource);
		}
	}
#if STALL_TIMERS
	PF_POP_TIMEBAR();
#endif // STALL_TIMERS
	return true;
}

grcDevice::Result grcTextureDX11::InitializeTempStagingTexture()
{
	Assertf((m_pExtraData == NULL), "This texture already has a staging texture");
	// PC TODO - Oscar - Outside of render thread GetDesc
	D3D11_TEXTURE2D_DESC oDesc;
	((ID3D11Texture2D*)(m_CachedTexturePtr))->GetDesc(&oDesc);

	m_pExtraData = AllocExtraData(GRC_TEXTURE_D3D11_SYNC_MODE);

	m_InfoBits.m_ReadWriteAccess = 1;

	oDesc.BindFlags = grcBindNone;
	oDesc.Usage = (D3D11_USAGE)grcUsageStage;
	oDesc.CPUAccessFlags = grcCPUWrite | grcCPURead;

	// Create the staging texture.
	m_pExtraData->m_pStagingTexture = NULL;
	grcDevice::Result uReturnCode = GRCDEVICE.GetCurrent()->CreateTexture2D(&oDesc, NULL, (ID3D11Texture2D**)&m_pExtraData->m_pStagingTexture);
	Assertf((HRESULT)uReturnCode != E_OUTOFMEMORY, "Error %x (out of memory) occurred creating staging texture", uReturnCode);
	Assertf((uReturnCode == 0), "Error %x occurred creating texture", uReturnCode);
	Assert(m_pExtraData->m_pStagingTexture != NULL);
#if !__FINAL
	if (m_pExtraData && m_pExtraData->m_pStagingTexture)
	{
		char szTempName[256];
		sprintf_s(szTempName, "%s - Staging", GetName());
		static_cast<ID3D11DeviceChild*>(m_pExtraData->m_pStagingTexture)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
	}
#endif // !__FINAL
	return uReturnCode;
}

void grcTextureDX11::ReleaseTempStagingTexture()
{
	m_InfoBits.m_ReadWriteAccess = 0;

	grcDeviceTexture *pStaging = GetStagingTexture();
	GRCDEVICE.DeleteTexture(pStaging);

	if(m_pExtraData)
	{
		FreeExtraData(m_pExtraData);
		m_pExtraData = NULL;
	}
}


/*--------------------------------------------------------------------------------------------------*/
/* Copy functions.																					*/
/*--------------------------------------------------------------------------------------------------*/


bool grcTextureDX11::Copy(const grcImage *pImage)
{
	Assert(GetStagingTexture() != NULL);
	AssertMsg(0, "grcTextureDX11::Copy - Not implemented");
	if (m_LayerCount != pImage->GetLayerCount()-1 || m_Width != pImage->GetWidth() || m_Height != pImage->GetHeight() || 
		m_nMipCount != pImage->GetExtraMipCount()+1)
		return false;

	// PC TODO - Do we need this anymore?

	/*
	D3DLOCKED_RECT	rect;

	TEXTURE* poTexture = (TEXTURE*)m_CachedTexturePtr;

#if USE_STAGING_TEXTURE
	poTexture = (TEXTURE*)CreateStagingCopy();
#endif // USE_RESOURCE_CACHE

	const grcImage	*pLayer = pImage;
	int			layer = 0;
#if __PAGING
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());
	u8 *pStorage = (u8*) m_BackingStore;
#endif
	while ( pLayer ) {
		const grcImage	*pMip = pLayer;
		for	(int i = 0; i < pLayer->GetExtraMipCount() + 1; i++)
		{
			void	*pDest = NULL;
#if __PAGING
			if (paging) {
				pDest = pStorage; 
			}
			else
#endif
			{
				if ( m_ImageType == grcImage::CUBE ) {
					// MAKE SURE D3D STILL MAINTAINS CUBEMAPS IN SEQUENTIAL ORDER
					Assert(
						D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1 &&
						"D3D Cubemap face defines are no longer sequential!");

					static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES)(D3DCUBEMAP_FACE_POSITIVE_X+layer), i, &rect, NULL, 0 );
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					D3DBOX inbox = {0,0,pMip->GetWidth(),pMip->GetHeight(),0,pMip->GetDepth()};
					D3DLOCKED_BOX lbox;
					static_cast<IDirect3DVolumeTexture9 *>(poTexture)->LockBox(i,&lbox,&inbox,0);
					rect.pBits = lbox.pBits;
					rect.Pitch = lbox.RowPitch;
				}
				else {		
					AssertVerify(static_cast<IDirect3DTexture9*>(poTexture)->LockRect(i, &rect, NULL, 0) == S_OK);
				}
				pDest = rect.pBits;
			}

			void	*pSource = (void*) pMip->GetBits();
			int		mipSize = pMip->GetSize();
			Assert((pDest != NULL) && (pSource != NULL));
			if (pDest != NULL)
			{
				sysMemCpy(pDest, pSource, mipSize);
			}

#if __PAGING
			if (paging)
				pStorage += pMip->GetSize();
			else
#endif
				if ( m_ImageType == grcImage::CUBE ) {
					static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES)layer, i);
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					static_cast<IDirect3DVolumeTexture9 *>(poTexture)->UnlockBox(i);
				}
				else {
					static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(i);
				}
			pMip = pMip->GetNext();
		}
		pLayer = pLayer->GetNextLayer();
		layer++;
	}
#if USE_STAGING_TEXTURE
	{
		HRESULT hr = GRCDEVICE.GetCurrent()->UpdateTexture(poTexture, (TEXTURE*)m_Texture);
		if (hr != S_OK)
		{
			Quitf("Failed to UpdateTexture %x", hr);
		}
		grcResourceCache::GetInstance().ReleaseStagingCopy((void*)poTexture, true);
	}
#endif // USE_RESOURCE_CACHE
	*/

	return true;
}

bool grcTextureDX11::Copy(const void * /*pvSrc*/, u32 uWidth, u32 uHeight, u32 /*uDepth*/)
{
	Assert(GetStagingTexture() != NULL);

	if (/*m_LayerCount != pImage->GetLayerCount()-1 ||*/ m_Width != uWidth || m_Height != uHeight) // || m_nMipCount != pImage->GetExtraMipCount()+1)
	{
		return false;
	}

	// PC TODO - Do we need this anymore?
	/*
	D3DLOCKED_RECT	rect;

	TEXTURE* poTexture = (TEXTURE*)m_Texture;

#if USE_RESOURCE_CACHE
	if (GRCDEVICE.GetResourcePool() == RESOURCE_UNMANAGED)
	{
		poTexture = (TEXTURE*)CreateBackupCopy();
	}
#endif // USE_RESOURCE_CACHE

	//const grcImage	*pLayer = pImage;
	int			layer = 0;
#if __PAGING
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());
	u8 *pStorage = (u8*) m_BackingStore;
#endif
	//while ( pLayer )
	{
		//const grcImage	*pMip = pLayer;
		//for	(int i = 0; i < pLayer->GetExtraMipCount() + 1; i++)
		for	(int i = 0; i < 1; i++)
		{
			void	*pDest = NULL;
#if __PAGING
			if (paging) {
				pDest = pStorage; 
			}
			else
#endif
			{
				if ( m_ImageType == grcImage::CUBE ) {
					// MAKE SURE D3D STILL MAINTAINS CUBEMAPS IN SEQUENTIAL ORDER
					Assert(
						D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1 &&
						"D3D Cubemap face defines are no longer sequential!");
					static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES)(D3DCUBEMAP_FACE_POSITIVE_X+layer), i, &rect, NULL, 0 );

				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					//D3DBOX inbox = {0,0,uWidth,uHeight,0,pMip->GetDepth()};
					D3DBOX inbox = {0,0,uWidth,uHeight,0,0};
					D3DLOCKED_BOX lbox;
					static_cast<IDirect3DVolumeTexture9 *>(poTexture)->LockBox(i,&lbox,&inbox,0);
					rect.pBits = lbox.pBits;
					rect.Pitch = lbox.RowPitch;
				}
				else {	
					AssertVerify(static_cast<IDirect3DTexture9*>(poTexture)->LockRect(i, &rect, NULL, 0) == S_OK);
				}
				pDest = rect.pBits;
			}

			void	*pSource = (void*) pvSrc; //pMip->GetBits();
			int		mipSize = (uWidth * uHeight * uDepth) / 8; // pMip->GetSize();
			Assert((pDest != NULL) && (pSource != NULL));
			if (pDest != NULL)
			{
				sysMemCpy(pDest, pSource, mipSize);
			}

#if __PAGING
			if (paging)
				pStorage += mipSize; //pMip->GetSize();
			else
#endif
				if ( m_ImageType == grcImage::CUBE ) {
					static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES)layer, i);
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					static_cast<IDirect3DVolumeTexture9 *>(poTexture)->UnlockBox(i);
				}
				else {
					AssertVerify(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(i) == S_OK);
				}

			//pMip = pMip->GetNext();
		}
		//pLayer = pLayer->GetNextLayer();
		//layer++;
	}

#if USE_RESOURCE_CACHE
	if (GRCDEVICE.GetResourcePool() == RESOURCE_UNMANAGED)
	{
		HRESULT hr = GRCDEVICE.GetCurrent()->UpdateTexture(poTexture, (TEXTURE*)m_Texture);
		if (hr != S_OK)
		{
			Quitf("Failed to UpdateTexture %x", hr);
		}
		grcResourceCache::GetInstance().ReleaseBackup((void*)poTexture, true);
	}
#endif // USE_RESOURCE_CACHE
	*/

	return true;
}

bool grcTextureDX11::Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex)
{
	Assert(pSource != NULL);

#if __ASSERT
	if ( pSource->GetImageFormat() == grcImage::A16B16G16R16 && GetImageFormat() == grcImage::DXT1 )
	{
		Assert(pSource->GetWidth()>>srcMipIndex == GetWidth()>>(dstMipIndex+2)); 
		Assert(pSource->GetHeight()>>srcMipIndex == GetHeight()>>(dstMipIndex+2));
	}
	else
	{
		Assert(pSource->GetWidth()>>srcMipIndex == GetWidth()>>(dstMipIndex>0?dstMipIndex:0)); 
		Assert(pSource->GetHeight()>>srcMipIndex == GetHeight()>>(dstMipIndex>0?dstMipIndex:0));
	}
#endif
		  
	
#if REUSE_RESOURCE
	const_cast<grcTexture*>(pSource)->UpdateGPUCopy();
	UpdateGPUCopy();
#endif // REUSE_RESOURCE

	ID3D11Texture2D* poSrc = static_cast<ID3D11Texture2D*>(const_cast<grcTextureObject*>(pSource->GetTexturePtr()));
	ID3D11Texture2D* poDst = static_cast<ID3D11Texture2D*>(GetTexturePtr());	
	Assertf((poSrc != NULL), "Texture Pointer is null on %s", pSource->GetName());
	Assertf((poDst != NULL), "Texture Pointer is null on %s", GetName());
	Assert(poSrc != poDst);
	if (poSrc == NULL || poDst == NULL)
		return false;

	if( dstSliceIndex != -1 || dstMipIndex != -1 || srcSliceIndex > 0 || srcMipIndex > 0)
	{
		Assert(dstSliceIndex >= 0 && dstMipIndex >= 0 && srcSliceIndex >= 0 && srcMipIndex >= 0);
		u32 DstX = 0;
		u32 DstY = 0;
		u32 DstZ = 0;
		u32 subDstResource = D3D11CalcSubresource(dstMipIndex, dstSliceIndex, GetMipMapCount());
		u32 subSrcResource = D3D11CalcSubresource(srcMipIndex, srcSliceIndex, pSource->GetMipMapCount());
		g_grcCurrentContext->CopySubresourceRegion(poDst, subDstResource, DstX, DstY, DstZ, poSrc, subSrcResource, NULL);
	}
	else
		g_grcCurrentContext->CopyResource(poDst, poSrc);

	return true;	
}

bool grcTextureDX11::CopyTo(grcImage* pImage, bool bInvert)
{
	if (pImage == NULL)
	{
		return false;
	}

	if ((GetLayerCount() != (int)pImage->GetLayerCount())	||
		(GetWidth() != pImage->GetWidth())					||
		(GetHeight() != pImage->GetHeight())				||
		(GetMipMapCount() != (pImage->GetExtraMipCount() + 1)))
	{
		return false;
	}

	bool bResult = false;

	grcTextureLock oLock;

	if (LockRect(0, 0, oLock))
	{
		u8* pSrc = (u8*)oLock.Base;
		u8* pDst = (u8*)pImage->GetBits();

		if (bInvert)
		{
			pSrc = pSrc + (oLock.Pitch * (GetHeight() - 1));
			oLock.Pitch *= -1;
		}

		int	mipSize = m_nMipStride * GetHeight();
		sysMemCpy(pDst, pSrc, mipSize);

		UnlockRect(oLock);

		bResult = true;
	}
	else
	{
		Assert(false && "grcTexturePC::CopyTo - Could not lock texture surface");
	}
	return bResult;
}

bool grcTextureDX11::Copy2D(const void* pSrc, u32 imgFormat, u32 width, u32 height, u32 numMips)
{
#if REUSE_RESOURCE
	UpdateGPUCopy();
#endif // REUSE_RESOURCE

	grcImage::Format format = (grcImage::Format)(imgFormat);

	// get out if something doesn't match
	if (GetLayerCount() != 1U || format != (grcImage::Format)GetImageFormat() ||  m_Width != width || m_Height != height || GetMipMapCount() != (int)numMips)
	{
		return false;
	}

	// cache mip data
	const u32 bpp		= grcImage::GetFormatBitsPerPixel(format);
	const u32 blockSize	= grcImage::IsFormatDXTBlockCompressed(format) ? 4 : 1;

	u32 mipWidth = width;
	u32 mipHeight = height;
	u32 mipPitch = mipWidth*bpp/8;

	if (format == grcImage::DXT1 || format == grcImage::DXT5A)
	{
		mipPitch = ((mipWidth+3)/4) * 8;
	}
	else if (imgFormat == grcImage::DXT3 || imgFormat == grcImage::DXT5 || imgFormat == grcImage::DXN)
	{
		mipPitch = ((mipWidth+3)/4) * 16;
	}

	u32 mipSizeInBytes = (mipHeight*mipPitch);
	u32 curMipOffset = 0;

	const char* pSrcMip = static_cast<const char*>(pSrc);

	// process and upload mips
	for (u32 i = 0; i < numMips; i++)
	{
		grcTextureLock oLock;

		if (LockRect(0, i, oLock))
		{
			Assert(oLock.Pitch == (int)mipPitch);
			// NOTE -- this does not work if stride is not a multiple of 256 bytes!
			sysMemCpy(oLock.Base, pSrcMip, mipSizeInBytes); 
			// unlock mip
			UnlockRect(oLock);
		}

		// update mip data
		mipWidth = Max<u32>(blockSize, mipWidth/2);
		mipHeight = Max<u32>(blockSize, mipHeight/2);
		mipPitch = mipWidth*bpp/8;

		if (format == grcImage::DXT1 || format == grcImage::DXT5A)
		{
			mipPitch = ((mipWidth+3)/4) * 8;
		}
		else if (imgFormat == grcImage::DXT3 || imgFormat == grcImage::DXT5 || imgFormat == grcImage::DXN)
		{
			mipPitch = ((mipWidth+3)/4) * 16;
		}

		curMipOffset += mipSizeInBytes;
		mipSizeInBytes = (mipHeight*mipPitch);

		pSrcMip += mipSizeInBytes;

	}

	return true;
}

bool grcTextureDX11::Copy2D(const void * pSrc, const grcPoint & oSrcDim, const GRCRECT & oDstRect, const grcTextureLock & UNUSED_PARAM(lock), s32 iMipLevel) 
{ 
#if REUSE_RESOURCE
	UpdateGPUCopy();
#endif // REUSE_RESOURCE

	// oSrcDim - Should contain source pitch and height - Assumes to have matching bytes per pixel
	// oDstRect - Should contain destation pixel rectangle to draw into
	Assert(pSrc != NULL);
	Assert(oSrcDim.x > 0);
	Assert(oSrcDim.y > 0);
	Assert(oDstRect.x1 < oDstRect.x2);
	Assert(oDstRect.y1 < oDstRect.y2);
	Assert(iMipLevel >= 0);
	Assert(iMipLevel < GetMipMapCount());

	D3D11_BOX oDest;
	oDest.left = oDstRect.x1;
	oDest.top = oDstRect.y1;
	oDest.front = iMipLevel;
	oDest.right = oDstRect.x2;
	oDest.bottom = oDstRect.y2;
	oDest.back = iMipLevel + 1;

	g_grcCurrentContext->UpdateSubresource(GetCachedTexturePtr(), iMipLevel, &oDest, pSrc, oSrcDim.x, oSrcDim.x * oSrcDim.y);

	return true;
}

/*--------------------------------------------------------------------------------------------------*/
/* Misc access functions.																			*/
/*--------------------------------------------------------------------------------------------------*/


u32	grcTextureDX11::GetImageFormat() const
{ 
	return grcTextureFactoryDX11::GetImageFormatStatic(m_nFormat); // cast this to grcImage::Format
}


int grcTextureDX11::GetBitsPerPixel() const
{
	return static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).GetBitsPerPixel(m_nFormat);
}


int	grcTextureDX11::GetStride(u32 uMipLevel) const
{
	int width = Max<int>(1, GetWidth() >> uMipLevel);

	if ((m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC1_UNORM_SRGB) || // DXT1
		(m_nFormat >= DXGI_FORMAT_BC4_TYPELESS && m_nFormat <= DXGI_FORMAT_BC4_SNORM)) // DXT5A
	{
		// Round width up to next multiple of 4, then multiply by 2 (which is really divide by 4 to get # blocks and multiply by 8 bytes per block)
		return ((width + 3) & ~3) * 2;
	}
	else
	if ((m_nFormat >= DXGI_FORMAT_BC2_TYPELESS && m_nFormat <= DXGI_FORMAT_BC3_UNORM_SRGB) || // DXT3 or DXT5
		(m_nFormat >= DXGI_FORMAT_BC5_TYPELESS && m_nFormat <= DXGI_FORMAT_BC5_SNORM) || // DXN
		(m_nFormat >= DXGI_FORMAT_BC6H_TYPELESS && m_nFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		// Round width up to next multiple of 4, then multiply by 4 (which is really divide by 4 to get # blocks and multiply by 16 bytes per block)
		return ((width + 3) & ~3) * 4;
	}
	else
	{
		return (width * GetBitsPerPixel()) / 8;
	}
}


int grcTextureDX11::GetRowCount(u32 uMipLevel) const
{
	int ret = Max<int>(1, (int)m_Height >> uMipLevel);

	if ((m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC5_SNORM) || // DXT1, DXT3, DXT5, DXT5A, DXN
		(m_nFormat >= DXGI_FORMAT_BC6H_TYPELESS && m_nFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		// Account for DXT/BC formats being in 4x4 blocks.
		ret = Max<int>(1, (ret + 3) >> 2);
	}

	return ret;
}

int grcTextureDX11::GetSliceCount(u32 uMipLevel) const
{
	return Max(1, (int)m_Depth >> uMipLevel);
}

int grcTextureDX11::GetTotalMipSize(u32 uMipLevel) const
{
	return GetStride(uMipLevel) * GetRowCount(uMipLevel) * GetSliceCount(uMipLevel);
}

int grcTextureDX11::GetBlockHeight_UNUSED(u32 uMipLevel) const
{
	if ((m_nFormat >= DXGI_FORMAT_BC1_TYPELESS && m_nFormat <= DXGI_FORMAT_BC5_SNORM) || // DXT1, DXT3, DXT5, DXT5A, DXN
		(m_nFormat >= DXGI_FORMAT_BC6H_TYPELESS && m_nFormat <= DXGI_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		uMipLevel += 2; //Basically, if texture is a block type, divide height by 4 in addition to the mip divider.
	}


	return Max<int>(1, (int)m_Height >> uMipLevel);
}

bool grcTextureDX11::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr()); 
#endif // __FINAL
}


grcTexture::ChannelBits grcTextureDX11::FindUsedChannels() const
{
	ChannelBits bits(false);

	switch(m_nFormat)
	{
		// RGBA	
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		bits.Set(CHANNEL_ALPHA);
		break;

		// RGB
	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		break;

		// RG
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		break;

		// R
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		bits.Set(CHANNEL_RED);
		break;

		// G
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		bits.Set(CHANNEL_GREEN);
		break;

		// A
	case DXGI_FORMAT_A8_UNORM:
		bits.Set(CHANNEL_ALPHA);
		break;

		// Depth
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
		bits.Set(CHANNEL_DEPTH);
		break;

	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		bits.Set(CHANNEL_DEPTH);
		bits.Set(CHANNEL_STENCIL);
		break;

	default:
		grcWarningf("Don't know what channels are in use in texture format %d", m_nFormat);
		break;
	}

	return bits;
}


/*--------------------------------------------------------------------------------------------------*/
/* Extra info functions.																			*/
/*--------------------------------------------------------------------------------------------------*/


grcTextureDX11_ExtraData *grcTextureDX11::AllocExtraData(grcsTextureSyncType SyncType)
{
	u32 i;

	if(sm_pExtraDatas == NULL)
	{
		sm_ExtraDatasCount = GRC_TEXTURE_DX11_EXTRA_DATA_MAX;

		// Make more lockable textures available when using the MAX Shader Editor.
		if(PARAM_lockableResourcedTextures.Get())
		{
			if(PARAM_lockableResourcedTexturesCount.Get())
				PARAM_lockableResourcedTexturesCount.Get(sm_ExtraDatasCount);
			else
				sm_ExtraDatasCount = 1024;
		}
		sm_pExtraDatas = rage_new grcTextureDX11_ExtraData[sm_ExtraDatasCount];
	}


	for(i=0; i<sm_ExtraDatasCount; i++)
	{
		if(sm_pExtraDatas[i].m_pStagingTexture == NULL)
		{
			sm_pExtraDatas[i].m_SyncType = SyncType;

			if(sm_pExtraDatas[i].m_SyncType == grcsTextureSync_Mutex)
			{
				sm_pExtraDatas[i].m_Mutex = sysIpcCreateMutex();
			}
			else if(sm_pExtraDatas[i].m_SyncType == grcsTextureSync_DirtySemaphore)
			{
				sm_pExtraDatas[i].m_DirtySemaphore = sysIpcCreateSema(1);
			}
			return &sm_pExtraDatas[i];
		}
	}

	grcErrorf("Possible Leak of Staging Textures");
	for(i=0; i<sm_ExtraDatasCount; i++)
	{
		if(sm_pExtraDatas[i].m_pStagingTexture != NULL)
		{
			char szName[256] = { "" };
			if (sm_pExtraDatas[i].m_pStagingTexture)
			{
				ID3D11DeviceChild *const deviceChild = static_cast<ID3D11DeviceChild*>(sm_pExtraDatas[i].m_pStagingTexture);
				UINT iOldNameSize = sizeof(szName)-1;

				deviceChild->GetPrivateData(WKPDID_D3DDebugObjectName, &iOldNameSize, szName);
				if (!iOldNameSize)
				{
					grcTexture* poTexture = NULL;
					UINT iSize = sizeof(poTexture);
					deviceChild->GetPrivateData(TextureBackPointerGuid, &iSize ,&poTexture);
					if (iSize)
					{
						if (poTexture->GetName())
							formatf(szName, "%s", poTexture->GetName());
						else
							formatf(szName, "No name in texture");
					}
					else
					{
						formatf(szName, "Leaked Texture no longer exists");
					}
				}
			}
			grcErrorf("%d - %s", i, szName);
		}
	}

	if(PARAM_lockableResourcedTextures.Get())
		grcErrorf("grcTextureDX11::AllocExtraData()...Out of extra datas. Try using command line param lockableResourcedTexturesCount!\n");
	else
		grcErrorf("grcTextureDX11::AllocExtraData()...Out of extra datas. Increase GRC_TEXTURE_DX11_EXTRA_DATA_MAX.\n");

	return NULL;
}


void grcTextureDX11::FreeExtraData(grcTextureDX11_ExtraData *pExtraData)
{
	pExtraData->m_pStagingTexture = NULL;

	if((pExtraData->m_SyncType == grcsTextureSync_Mutex) && (pExtraData->m_Mutex))
	{
		sysIpcDeleteMutex(pExtraData->m_Mutex);
		pExtraData->m_Mutex = NULL;
	}
	else if((pExtraData->m_SyncType == grcsTextureSync_DirtySemaphore) && (pExtraData->m_DirtySemaphore))
	{
		sysIpcDeleteSema(pExtraData->m_DirtySemaphore);
		pExtraData->m_DirtySemaphore = NULL;
	}

	pExtraData->m_SyncType = grcsTextureSync_None;
}


// Returns the staging texture.
grcDeviceTexture *grcTextureDX11::GetStagingTexture() const
{
	grcDeviceTexture *pRet = NULL;

	if(m_pExtraData)
	{
		pRet = m_pExtraData->m_pStagingTexture;
	}
	return pRet;
}


/*--------------------------------------------------------------------------------------------------*/
/* Texture factory functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

void grcTextureDX11::DeviceLost() 
{
	/* DX11 does not have lost devices
	if (m_StagingTexture)
	{
		GRCDEVICE.DeleteTexture(m_Texture);
		m_Texture = NULL;
		if (m_pShaderResourceView)
		{
			SAFE_RELEASE_RESOURCE(m_pShaderResourceView);
			m_pShaderResourceView = NULL;
		}
	}
	*/
}


void grcTextureDX11::DeviceReset() 
{
	/*
	if (m_StagingTexture)
	{
		CreateFromBackingStore(true);
	}
	*/
}


void grcTextureDX11::SetPrivateData()
{
#if !__D3D11_MONO_DRIVER && !__FINAL
	grcTexture* pTex = this;

	if (m_CachedTexturePtr)
	{
		if (GetName())
		{
			CHECK_HRESULT(((ID3D11DeviceChild*)m_CachedTexturePtr)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen(GetName())+1,GetName()));
			//Displayf("%p - %s", m_CachedTexturePtr, GetName());
		}
		CHECK_HRESULT(((ID3D11DeviceChild*)m_CachedTexturePtr)->SetPrivateData(TextureBackPointerGuid,sizeof(pTex),&pTex));

	}

	if (m_pShaderResourceView)
	{
		char szTempName[256];
		sprintf_s(szTempName, "%s - Resource", GetName());
		static_cast<ID3D11DeviceChild*>(m_pShaderResourceView)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
		//Displayf("%p - %s", m_pShaderResourceView, szTempName);
	}

	if (m_pExtraData && m_pExtraData->m_pStagingTexture)
	{
		char szTempName[256];
		sprintf_s(szTempName, "%s - Staging", GetName());
		static_cast<ID3D11DeviceChild*>(m_pExtraData->m_pStagingTexture)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
		//Displayf("%p - %s", m_pExtraData->m_pStagingTexture, szTempName);
	}
#endif	// __D3D11_MONO_DRIVER
}


#if __DECLARESTRUCT
void grcTextureDX11::DeclareStruct(datTypeStruct &s) {
	grcTexturePC::DeclareStruct(s);
}
#endif


void *grcTextureDX11::AllocTempMemory(u32 Size)
{
	Assertf(sm_pAllocTempMemory, "grcBufferD3D11::AllocTempMemory()....No temp allocator set.");
	return sm_pAllocTempMemory(Size);
}

}	// namespace rage

#endif	// RSG_PC &&  __D3D11

