

#if	(RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO

#include "config.h"
#include "device.h"
#include "image.h"
#include "effect_mrt_config.h"
#include "texture_durango.h"
#include "texturefactory_d3d11.h"
#include "wrapper_d3d.h"
#include "system/memory.h"
#include "system/param.h"
#include "grprofile/pix.h"
#include "diag/tracker.h"
#include "string/string.h"
#include "data/resource.h"
#include "data/struct.h"
#include "grcore/buffer_durango.h"

// DOM-IGNORE-BEGIN 

// DOM-IGNORE-END
XPARAM(usetypedformat);
XPARAM(noSRGB);
PARAM(usewhitemissingtexture,"Sets the missing texture to be white.");


// LDS DMC TEMP:-
//extern size_t g_LastTextureAlignment;
//extern rage::grcOrbisDurangoTileMode g_LastTileMode;

#if __TOOL
namespace rage
{
	XPARAM(noquits);
}
#endif

#include "atl/pool.h"
#include "atl/inlist.h"
#include "system/externalheap.h"
#include "system/criticalsection.h"

#if (RSG_DURANGO && __D3D11_MONO_DRIVER)
#pragma comment(lib,"xg_x.lib")
#else
#pragma comment(lib,"xg.lib")
#endif //RSG_DURANGO && __D3D11_MONO_DRIVER

// TEMP:- Work around until project generation is fixed (see B*1582860).
#if !__D3D11
#include "../../../3rdparty/durango/xg.h"
#else // !__D3D11
#include <xg.h>
#endif // !__D3D11

#if RSG_DURANGO
#include <xdk.h>
#if _XDK_VER > 9299
#include "system/d3d11.h"
#endif // _XDK_VER > 9299
#endif // RSG_DURANGO

#if __RESOURCECOMPILER
#define RESOURCECOMPILER_ONLY(x) x
int g_useVirtualMemory = 0;
unsigned g_overrideAlignment = 0;
#else
#define RESOURCECOMPILER_ONLY(x)
#endif

namespace rage 
{

#define D3DFMT_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

extern GUID TextureBackPointerGuid;

/*======================================================================================================================================*/
/* grcDurangoPlacementTexture classes.																									*/
/*======================================================================================================================================*/

struct grcDurangoPlacementTextureLockInfo
{
	int layer;
	int mipLevel;
};


#define GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_CPU_FLUSH_ON_READ 0x1
#define GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_GPU_FLUSH_ON_BIND 0x2

#if RSG_DURANGO

void grcDurangoPlacementTexture::LockMemoryArea::FlushCpu()
{
	D3DFlushCpuCache(m_pMem, m_Size);
	m_pMem = NULL;
	m_Size = 0;
}

#endif // RSG_DURANGO

/*--------------------------------------------------------------------------------------------------*/
/* Constructors/Destructor.																			*/
/*--------------------------------------------------------------------------------------------------*/

sysCriticalSectionToken grcDurangoPlacementTexture::m_CritialSectionToken;

grcDurangoPlacementTexture::grcDurangoPlacementTexture()
{
}


grcDurangoPlacementTexture::grcDurangoPlacementTexture(u8 type)
: grcOrbisDurangoTextureBase(type)
{
}


grcDurangoPlacementTexture::grcDurangoPlacementTexture(GRC_ORBIS_DURANGO_TEXTURE_DESC &info, MipSliceElement *pArrayOfMipSlices, void *pPreAllocatedGraphicsMem, u8 type) 
: grcOrbisDurangoTextureBase(info, type)
{
	Init(pArrayOfMipSlices, pPreAllocatedGraphicsMem);
}


grcDurangoPlacementTexture::grcDurangoPlacementTexture(datResource &rsc) 
: grcOrbisDurangoTextureBase(rsc)
{
#if __D3D11
	CreateD3DResources(*this, m_pGraphicsMem);
	ComputeMipOffsets(); //some placement textures are locked, for example the compressed ped / headshot destination
#endif // __D3D11
}


grcDurangoPlacementTexture::~grcDurangoPlacementTexture() 
{
#if RSG_DURANGO
	DestroyD3DResources();

	if(GetOwnsAllocations() && !GetUsesPreAllocatedMemory() && m_pGraphicsMem)
		sysMemPhysicalFree(m_pGraphicsMem);

#elif __RESOURCECOMPILER
	physical_delete(m_pGraphicsMem);
#endif // __RESOURCECOMPILER

	m_pGraphicsMem = NULL;
}


void grcDurangoPlacementTexture::Init(MipSliceElement *pArrayOfMipSlices, void *pPreAllocatedGraphicsMemory)
{
	if (pPreAllocatedGraphicsMemory == NULL)
	{
		AllocateMemory();

		// Fill it.
		Fill(pArrayOfMipSlices);
	}
	else
	{
#if !__RESOURCECOMPILER
		Assert(pPreAllocatedGraphicsMemory!= NULL);
		m_pGraphicsMem = pPreAllocatedGraphicsMemory;
#endif
		SetUsesPreAllocatedMemory(true);
	}

	// Create lock info.
	ComputeMipOffsets();
#if __D3D11
	// Create the texture.
	CreateD3DResources(*this, m_pGraphicsMem);
#endif // __D3D11
}


void grcDurangoPlacementTexture::AllocateMemory()
{
	size_t alignment;

	// Allocate the memory requirements
	CalculateMemoryRequirements(*this, m_GraphicsMemorySize, alignment);
	// LDS DMC TEMP:-
	//g_LastTextureAlignment = alignment;
	m_pGraphicsMem = NULL;

#if RSG_DURANGO
	m_pGraphicsMem = sysMemPhysicalAllocate(m_GraphicsMemorySize, alignment, PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
	FatalAssert(m_pGraphicsMem != NULL);
#elif __RESOURCECOMPILER
	if (g_overrideAlignment)
		alignment = g_overrideAlignment;

	if(g_useVirtualMemory && g_sysPlatform == platform::ORBIS)
	{
		m_pGraphicsMem = rage_aligned_new(alignment) char[m_GraphicsMemorySize];
	}
	else
	{
		m_pGraphicsMem = physical_new(m_GraphicsMemorySize, alignment);
	}
#endif// __RESOURCECOMPILER

	SetPhysicalSize(m_GraphicsMemorySize);
}


void grcDurangoPlacementTexture::Fill(MipSliceElement *pArrayOfMipSlices)
{
	// Fill the texture.
	if(pArrayOfMipSlices)
		CopyOverTexels(pArrayOfMipSlices, m_Texture.GetMipMap());
}
	

void grcDurangoPlacementTexture::ComputeMipOffsets(bool RESOURCECOMPILER_ONLY(bIsHdSplit))
{
	XG_RESOURCE_LAYOUT xgLayout;
#if __RESOURCECOMPILER
	XG_RESOURCE_LAYOUT xgLayoutChain;
#endif

	if(m_Texture.GetImageType() != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC xgDesc;

		FillInXGDesc2D(xgDesc);
		CHECK_HRESULT(XGComputeTexture2DLayout(&xgDesc, &xgLayout));

#if __RESOURCECOMPILER
		if (bIsHdSplit)
		{
			xgDesc.Width >>= 1;
			xgDesc.Height >>= 1;
			xgDesc.MipLevels--;
			CHECK_HRESULT(XGComputeTexture2DLayout(&xgDesc, &xgLayoutChain));
		}
#endif
	}
	else
	{
		XG_TEXTURE3D_DESC xgDesc;

		FillInXGDesc3D(xgDesc);
		CHECK_HRESULT(XGComputeTexture3DLayout(&xgDesc, &xgLayout));
	}

#if __RESOURCECOMPILER
	if (bIsHdSplit)
	{
		u32 hdMipSize = xgLayout.Plane[0].MipLayout[0].SizeBytes;
		u32 pitch = xgLayout.Plane[0].MipLayout[0].PitchBytes;
		u32 offset = xgLayout.Plane[0].MipLayout[0].OffsetBytes;
		SetLockInfo(0, offset, pitch);

		for(int i=0; i<GetMipMapCount()-1; i++)
		{
			u32 pitch = xgLayoutChain.Plane[0].MipLayout[i].PitchBytes;
			u32 offset = xgLayoutChain.Plane[0].MipLayout[i].OffsetBytes + hdMipSize;
			SetLockInfo(i + 1, offset, pitch);
		}
	}
	else
#endif
	{
		for(int i=0; i<GetMipMapCount(); i++)
		{
			u32 pitch = xgLayout.Plane[0].MipLayout[i].PitchBytes;
			u32 offset = xgLayout.Plane[0].MipLayout[i].OffsetBytes;
			SetLockInfo(i, offset, pitch);
		}
	}
}


/*--------------------------------------------------------------------------------------------------*/
/* Interface.																						*/
/*--------------------------------------------------------------------------------------------------*/

#if RSG_DURANGO

void grcDurangoPlacementTexture::Lock(grcDurangoPlacementTextureLockInfo &lockInfoIn, grcTextureLock &lockInfoOut, u32 uLockFlags)
{
	sysCriticalSection cs(m_CritialSectionToken);

	LockMemoryArea newArea;
	ComputeLockInfo(lockInfoIn, lockInfoOut, newArea);

	// Is the lock of read type ?
	if((uLockFlags & grcsRead) && (GetUserMemory()->m_CacheFlags & GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_CPU_FLUSH_ON_READ))
	{
		// Make sure we`re accessing the latest copy (we think the flush cache functions really mean update/sync).
		D3DFlushCpuCache(m_pGraphicsMem, m_GraphicsMemorySize);
		GetUserMemory()->m_CacheFlags &= ~GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_CPU_FLUSH_ON_READ;
	}

	// Is the lock of write type ?
	if(uLockFlags & (grcsWrite | grcsDiscard | grcsNoOverwrite))
	{
		// Record the memory area.
		GetUserMemory()->m_MemoryArea = newArea; 
	}
}


void grcDurangoPlacementTexture::Unlock()
{
	sysCriticalSection cs(m_CritialSectionToken);

	if(GetUserMemory()->m_MemoryArea.IsValid())
	{
		GetUserMemory()->m_MemoryArea.FlushCpu();
		GetUserMemory()->m_CacheFlags |= GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_GPU_FLUSH_ON_BIND;
	}
}


// TODO:- Change this for a mark as dirty when this texture is written to as a render target.
void grcDurangoPlacementTexture::UpdateCPUCopy()
{
	sysCriticalSection cs(m_CritialSectionToken);

#if _XDK_VER >= 9524
	g_grcCurrentContext->FlushGpuCacheRange(0, m_pGraphicsMem, m_GraphicsMemorySize);
#endif //_XDK_VER >= 9524
	GetUserMemory()->m_CacheFlags |= GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_CPU_FLUSH_ON_READ;
}


void grcDurangoPlacementTexture::UpdateGPUCopy()
{
	sysCriticalSection cs(m_CritialSectionToken);

	D3DFlushCpuCache(m_pGraphicsMem, m_GraphicsMemorySize);
	GetUserMemory()->m_CacheFlags |= GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_GPU_FLUSH_ON_BIND;
}


grcDeviceView *grcDurangoPlacementTexture::GetSRV()
{
	sysCriticalSection cs(m_CritialSectionToken);

	if(GetUserMemory()->m_CacheFlags & GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_GPU_FLUSH_ON_BIND)
	{
	#if _XDK_VER >= 9524
		g_grcCurrentContext->FlushGpuCacheRange(0, m_pGraphicsMem, m_GraphicsMemorySize);
	#endif //_XDK_VER >= 9524
		GetUserMemory()->m_CacheFlags &= ~GRC_DURANGO_PLACEMENT_TEXTURE_NEEDS_GPU_FLUSH_ON_BIND;
	}
	return GetUserMemory()->m_pShaderView;
}


grcTextureObject *grcDurangoPlacementTexture::GetTextureObject() const
{
	return GetUserMemory()->m_pTexture;
}


void grcDurangoPlacementTexture::ComputeLockInfo(grcDurangoPlacementTextureLockInfo &lockInfoIn, grcTextureLock &lockInfoOut, LockMemoryArea &MemArea)
{
	size_t pitch = 0;
	size_t slicePitch = 0;
	size_t offset = 0;

	GetLockInfo(lockInfoIn.mipLevel, offset, pitch);
	slicePitch = pitch*GetRowCount(lockInfoIn.mipLevel);
	char *pBase = (char *)m_pGraphicsMem + offset;
		
	// Compute lock return info.
	lockInfoOut.MipLevel = lockInfoIn.mipLevel;
	lockInfoOut.Layer = lockInfoIn.layer;
	lockInfoOut.Pitch = pitch;
	lockInfoOut.Base = (void *)(pBase + (size_t)(slicePitch*lockInfoIn.layer));
	lockInfoOut.BitsPerPixel = GetBitsPerPixelFromFormat((grcTextureFormat)ConvertTogrcFormat((GRC_TEMP_XG_FORMAT)m_Texture.GetFormat()));
	lockInfoOut.Width = Max<int>(1, m_Texture.GetWidth() >> lockInfoIn.mipLevel);
	lockInfoOut.Height = Max<int>(1, m_Texture.GetHeight() >> lockInfoIn.mipLevel);
	lockInfoOut.RawFormat = (u32)m_Texture.GetFormat();

	// Detail the memory area.
	MemArea.m_pMem = lockInfoOut.Base;
	MemArea.m_Size = slicePitch;
}

#if !__FINAL && !__RESOURCECOMPILER
// see durango_samples_aug_2013\XTexConv_08_2013_qfe10\tools\XTexConv\DirectXTexXboxDetile.cpp
grcImage* grcTextureDurango::CreateLinearImageCopy(int maxMips) const
{
	if (maxMips == 0)
	{
		maxMips = this->GetMipMapCount();
	}
	else
	{
		maxMips = Min<int>(this->GetMipMapCount(), maxMips);
	}

	XGTextureAddressComputer *pxgComputer = NULL;
	grcDevice::Result uReturnCode;

	if(GetPlacementTextureCellGcmTextureWrapper().GetImageType() != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC xgDesc;
		GetPlacementTexture()->FillInXGDesc2D(xgDesc);
		uReturnCode = XGCreateTexture2DComputer(&xgDesc, &pxgComputer);
	}
	else
	{
		XG_TEXTURE3D_DESC xgDesc;
		GetPlacementTexture()->FillInXGDesc3D(xgDesc);
		uReturnCode = XGCreateTexture3DComputer(&xgDesc, &pxgComputer);
	}

	Assertf((uReturnCode == 0), "Error %x occurred creating texture computer", uReturnCode);
	grcAssertf(pxgComputer, "grcTextureDurango::CreateLinearImageCopy()...Failed to create XG address computer.");

	grcImage* copy = grcImage::Create(
		this->GetWidth(),
		this->GetHeight(),
		this->GetDepth(),
		(grcImage::Format)this->GetImageFormat(),
		(grcImage::ImageType)this->GetImageType(),
		maxMips - 1,
		this->GetLayerCount() - 1
	);

	grcImage* layer = copy;

	for (int layerIndex = 0; layerIndex < (int)this->GetLayerCount(); layerIndex++)
	{
		grcImage* mip = layer;

		for (int mipIndex = 0; mipIndex < maxMips; mipIndex++)
		{
			sysMemSet(mip->GetBits(), 0, mip->GetSize());

			((grcTextureDurango *)this)->GetMipLinearTexels(mip->GetBits(), layerIndex, mipIndex, mip->GetSize(), pxgComputer);

			mip = mip->GetNext();
		}

		layer = layer->GetNextLayer();
	}

	// Free the computer.
	pxgComputer->Release();

	return copy;
}
#endif // !__FINAL && !__RESOURCECOMPILER

void grcTextureDurango::GetLinearTexels(void* linearMem, int memSize, int maxMips) const
{
	if (linearMem == NULL)
		return;

	if (maxMips == 0)
	{
		maxMips = this->GetMipMapCount();
	}
	else
	{
		maxMips = Min<int>(this->GetMipMapCount(), maxMips);
	}

	XGTextureAddressComputer *pxgComputer = NULL;
	grcDevice::Result uReturnCode;

	if(GetPlacementTextureCellGcmTextureWrapper().GetImageType() != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC xgDesc;
		GetPlacementTexture()->FillInXGDesc2D(xgDesc);
		uReturnCode = XGCreateTexture2DComputer(&xgDesc, &pxgComputer);
	}
	else
	{
		XG_TEXTURE3D_DESC xgDesc;
		GetPlacementTexture()->FillInXGDesc3D(xgDesc);
		uReturnCode = XGCreateTexture3DComputer(&xgDesc, &pxgComputer);
	}

	Assertf((uReturnCode == 0), "Error %x occurred creating texture computer", uReturnCode);
	grcAssertf(pxgComputer, "grcTextureDurango::GetLinearTexels()...Failed to create XG address computer.");

	for (int layerIndex = 0; layerIndex < (int)this->GetLayerCount(); layerIndex++)
	{
		for (int mipIndex = 0; mipIndex < maxMips; mipIndex++)
		{
			((grcTextureDurango *)this)->GetMipLinearTexels(linearMem, layerIndex, mipIndex, memSize, pxgComputer);
		}
	}

	// Free the computer.
	pxgComputer->Release();

}

void grcTextureDurango::GetMipLinearTexels(void* pTexels, int layerIndex, int mipIndex, int memSize, XGTextureAddressComputer* pxgComputer)
{
	u8* pDst = (u8*)pTexels;
	u32 mipSize = 0;

	grcTextureLock lock;
	if (AssertVerify(this->LockRect(layerIndex, mipIndex, lock, grcsRead | grcsAllowVRAMLock)))
	{
		const int blockStride = GetStride(mipIndex);
		const int blockRows = GetRowCount(mipIndex);
		const int blockSize = GetBitsPerPixel()*(grcImage::IsFormatDXTBlockCompressed((grcImage::Format)GetImageFormat()) ? 16 : 1)/8;
		const int numVolumeSlices = Max<int>(1, this->GetDepth() >> mipIndex);

		if (blockSize == 12)
		{
			// TODO -- support R32G32B32 (96-bit) formats
		}
		else for (int sliceIndex = 0; sliceIndex < numVolumeSlices; sliceIndex++)
		{
			for (int rowIndex = 0; rowIndex < blockRows; rowIndex++)
			{
				for (int x = 0; x < blockStride; x += blockSize)
				{
					const u32 baseOffset = (u32)pxgComputer->GetMipLevelOffsetBytes(
						layerIndex, // plane (layer?)
						mipIndex
						);
					const u32 elementOffset = (u32)pxgComputer->GetTexelElementOffsetBytes(
						layerIndex, // plane (layer?)
						mipIndex,
						x/blockSize,
						rowIndex,
						sliceIndex, // z
						0 // sample
						);

					const u32 srcOffset = elementOffset - baseOffset;
					const u32 dstOffset = x + (rowIndex + sliceIndex*blockRows)*blockStride;

					if (AssertVerify(dstOffset + blockSize <= (u32)memSize))
					{
						sysMemCpy(pDst + dstOffset, (const u8*)lock.Base + srcOffset, blockSize);
						mipSize += blockSize;
					}
				}
			}
		}
		this->UnlockRect(lock);
		pDst += mipSize;
	}
}

#endif // RSG_DURANGO

/*--------------------------------------------------------------------------------------------------*/
/* Internal helper functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

void grcDurangoPlacementTexture::CopyOverTexels(MipSliceElement *pArrayOfMipSlices, u32 sizeOfMipsArray)
{
	sysMemStartTemp();

	XGTextureAddressComputer *pxgComputer = NULL;
	grcDevice::Result uReturnCode;

	if(m_Texture.GetImageType() != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC xgDesc;
		FillInXGDesc2D(xgDesc);
		uReturnCode = XGCreateTexture2DComputer(&xgDesc, &pxgComputer);
	}
	else
	{
		XG_TEXTURE3D_DESC xgDesc;
		FillInXGDesc3D(xgDesc);
		uReturnCode = XGCreateTexture3DComputer(&xgDesc, &pxgComputer);
	}

	Assertf((uReturnCode == 0), "Error %x occurred creating texture computer", uReturnCode);
	grcAssertf(pxgComputer, "grcDurangoPlacementTexture::CopyOverTexels()...Failed to create XG address computer.");

	// Visit each mip-level, up to the limit of what we have data for
	for(u32 mipLevel=0; mipLevel < sizeOfMipsArray; mipLevel++) 
	{
		int mipStride = GetStride(mipLevel);
		int rowCount = GetRowCount(mipLevel);

		// Construct the contiguous mip-slice (all images for 1 mip level on after the other).
		char *pContiguousMipSlice = rage_new char[mipStride*rowCount*m_Texture.GetDepth()];
		char *pDest = pContiguousMipSlice;

		MipSliceElement *pMipInArraySlice = &pArrayOfMipSlices[mipLevel];

		for(int arraySlice=0; arraySlice<max(1, (m_Texture.GetDepth() >> mipLevel)); arraySlice++)
		{
			char *pSrc = (char *)pMipInArraySlice->pData;

			for(int row=0; row<rowCount; row++)
			{
				sysMemCpy(pDest, pSrc, mipStride);
				pSrc += pMipInArraySlice->pitch; // Move on a row in the source.
				pDest += mipStride; // Move on  a row in dest.
			}
			pMipInArraySlice = pMipInArraySlice->pSameMipInNextArraySlice;
		}

		// Use the XG computer to copy over the texels.
		pxgComputer->CopyIntoSubresource(m_pGraphicsMem, 0, mipLevel, pContiguousMipSlice, mipStride, mipStride*rowCount);
		// Free the contiguous mip images.
		delete [] pContiguousMipSlice;
	}
	// Free the computer.
	pxgComputer->Release();

	sysMemEndTemp();
}

#if RSG_DURANGO

void grcDurangoPlacementTexture::CreateD3DResources(grcOrbisDurangoTextureBase &info, void *pGPUVirtualAddr)
{
	(void)info;
	DestroyD3DResources();

	HRESULT uReturnCode;
	grcDeviceHandle *dev = GRCDEVICE.GetCurrent();

	if(m_Texture.GetImageType() != grcImage::VOLUME)
	{
		// Create the texture...
		D3D11_TEXTURE2D_DESC texDesc;
		FillInD3D11Desc2D(texDesc);
		uReturnCode = dev->CreatePlacementTexture2D(&texDesc, grcOrbisDurangoTileModeToDurango((grcOrbisDurangoTileMode)m_Texture.GetTileMode()), 0, pGPUVirtualAddr, (ID3D11Texture2D **)&GetUserMemory()->m_pTexture);
		Assertf((uReturnCode == 0), "grcDurangoPlacementTexture::InitAtGraphicsMemoryAddress() Error %x from CreatePlacementTexture2D().", uReturnCode);
		CHECK_HRESULT(uReturnCode);
	}
	else
	{
		// Create the texture...
		D3D11_TEXTURE3D_DESC texDesc;
		FillInD3D11Desc3D(texDesc);
		uReturnCode = dev->CreatePlacementTexture3D(&texDesc, grcOrbisDurangoTileModeToDurango((grcOrbisDurangoTileMode)m_Texture.GetTileMode()), 0, pGPUVirtualAddr, (ID3D11Texture3D **)&GetUserMemory()->m_pTexture);
		Assertf((uReturnCode == 0), "grcDurangoPlacementTexture::InitAtGraphicsMemoryAddress() Error %x from CreatePlacementTexture2D().", uReturnCode);
		CHECK_HRESULT(uReturnCode);
	}

	// Create the resource view.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	FillInSRVDesc(srvDesc);
	uReturnCode = dev->CreateShaderResourceView((ID3D11Resource*)GetUserMemory()->m_pTexture, &srvDesc, (ID3D11ShaderResourceView**)&GetUserMemory()->m_pShaderView);
	Assertf((uReturnCode == 0), "grcDurangoPlacementTexture::InitAtGraphicsMemoryAddress() Error %x from CreateShaderResourceView().", uReturnCode);
	CHECK_HRESULT(uReturnCode);
}


void grcDurangoPlacementTexture::DestroyD3DResources()
{
	if(GetUserMemory()->m_pShaderView)
		GetUserMemory()->m_pShaderView->Release();
	if(GetUserMemory()->m_pTexture)
		GetUserMemory()->m_pTexture->Release();

	GetUserMemory()->m_pShaderView = NULL;
	GetUserMemory()->m_pTexture = NULL;
}

#endif // RSG_DURANGO

void grcDurangoPlacementTexture::CalculateMemoryRequirements(grcOrbisDurangoTextureBase &info, size_t &memory, size_t &alignment)
{
	(void)info;

	if(m_Texture.GetImageType() != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC xgDesc;
		XG_RESOURCE_LAYOUT layout;
		FillInXGDesc2D(xgDesc);
		CHECK_HRESULT(XGComputeTexture2DLayout(&xgDesc, &layout));
		memory = layout.SizeBytes;
		alignment = layout.BaseAlignmentBytes;
	}
	else
	{
		XG_TEXTURE3D_DESC xgDesc;
		XG_RESOURCE_LAYOUT layout;
		FillInXGDesc3D(xgDesc);
		CHECK_HRESULT(XGComputeTexture3DLayout(&xgDesc, &layout));
		memory = layout.SizeBytes;
		alignment = layout.BaseAlignmentBytes;
	}

#if RSG_DURANGO
	// We`ll be calling D3DAllocateGraphicsMemory, so we 4k need alignment on size and position.
	memory = (memory + 4095) & ~4095;
	alignment = (alignment + 4095) & ~4095;
#endif // RSG_DURANGO

#if __RESOURCECOMPILER
	// We require 256 byte alignment for Orbis compatibility.
	alignment = (alignment + 255) & ~255;
#endif //__RESOURCECOMPILER
}


/*--------------------------------------------------------------------------------------------------*/
/* XG desc functions.																				*/
/*--------------------------------------------------------------------------------------------------*/


void grcDurangoPlacementTexture::FillInXGDesc2D(XG_TEXTURE2D_DESC &outDesc) const
{
	grcAssertf(m_Texture.GetImageType() != grcImage::VOLUME, "grcOrbisDurangoTextureBase::FillInXGDesc2D()...Got volume texture.");
	// Dimensions.
	outDesc.Width = m_Texture.GetWidth(); outDesc.Height = m_Texture.GetHeight(); outDesc.ArraySize = m_Texture.GetDimension();
	// Mips and format.
	outDesc.MipLevels = m_Texture.GetMipMap(); outDesc.Format = (XG_FORMAT)m_Texture.GetFormat(); 
	// Sample count.
	outDesc.SampleDesc.Count = 1; outDesc.SampleDesc.Quality = 0;

	// Misc other bits.
	outDesc.Usage = XG_USAGE_DEFAULT;
	outDesc.BindFlags = XG_BIND_SHADER_RESOURCE;

	if(m_Texture.GetBindFlag() & grcBindRenderTarget)
		outDesc.BindFlags |= XG_BIND_RENDER_TARGET;

	outDesc.CPUAccessFlags = 0;
	outDesc.MiscFlags = 0;
	outDesc.TileMode = grcOrbisDurangoTileModeToDurango((grcOrbisDurangoTileMode)m_Texture.GetTileMode());
	outDesc.ESRAMOffsetBytes = 0;
#if defined(_XDK_VER) && _XDK_VER >= 10542
	outDesc.ESRAMUsageBytes = 0;
#endif
	outDesc.Pitch = 0;
}


void grcDurangoPlacementTexture::FillInXGDesc3D(XG_TEXTURE3D_DESC &outDesc) const
{
	grcAssertf(m_Texture.GetImageType() == grcImage::VOLUME, "grcOrbisDurangoTextureBase::FillInXGDesc2D()...NOT got volume texture.");
	// Dimensions.
	outDesc.Width = m_Texture.GetWidth(); outDesc.Height = m_Texture.GetHeight(); outDesc.Depth = m_Texture.GetDepth();
	// Mips and format.
	outDesc.MipLevels = m_Texture.GetMipMap(); outDesc.Format = (XG_FORMAT)m_Texture.GetFormat(); 

	// Misc other bits.
	outDesc.Usage = XG_USAGE_DEFAULT;
	outDesc.BindFlags = XG_BIND_SHADER_RESOURCE;

	if(m_Texture.GetBindFlag() & grcBindRenderTarget)
		outDesc.BindFlags |= XG_BIND_RENDER_TARGET;

	outDesc.CPUAccessFlags = 0;
	outDesc.MiscFlags = 0;
	outDesc.TileMode = grcOrbisDurangoTileModeToDurango((grcOrbisDurangoTileMode)m_Texture.GetTileMode());
	outDesc.ESRAMOffsetBytes = 0;
#if defined(_XDK_VER) && _XDK_VER >= 10542
	outDesc.ESRAMUsageBytes = 0;
#endif
	outDesc.Pitch = 0;
}


/*--------------------------------------------------------------------------------------------------*/
/* DX11 desc functions.																				*/
/*--------------------------------------------------------------------------------------------------*/

#if RSG_DURANGO

void grcDurangoPlacementTexture::FillInD3D11Desc2D(D3D11_TEXTURE2D_DESC &outDesc) const
{
	grcAssertf(m_Texture.GetImageType() != grcImage::VOLUME, "grcOrbisDurangoTextureBase::FillInXGDesc2D()...Got volume texture.");
	// Dimensions.
	outDesc.Width = m_Texture.GetWidth(); outDesc.Height = m_Texture.GetHeight(); outDesc.ArraySize = m_Texture.GetDimension();
	// Mips and format.
	outDesc.MipLevels = m_Texture.GetMipMap(); outDesc.Format = (DXGI_FORMAT)m_Texture.GetFormat(); 
	// Sample count.
	outDesc.SampleDesc.Count = 1; outDesc.SampleDesc.Quality = 0;

	// Misc other bits.
	outDesc.Usage = D3D11_USAGE_DEFAULT;
	outDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if(m_Texture.GetBindFlag() & grcBindRenderTarget)
		outDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	outDesc.CPUAccessFlags = 0;
	outDesc.MiscFlags = 0;
	outDesc.ESRAMOffsetBytes = 0;
#if _XDK_VER >= 10542
	outDesc.ESRAMUsageBytes = 0;
#endif
}


void grcDurangoPlacementTexture::FillInD3D11Desc3D(D3D11_TEXTURE3D_DESC &outDesc) const
{
	grcAssertf(m_Texture.GetImageType() == grcImage::VOLUME, "grcOrbisDurangoTextureBase::FillInXGDesc2D()...NOT got volume texture.");
	// Dimensions.
	outDesc.Width = m_Texture.GetWidth(); outDesc.Height = m_Texture.GetHeight(); outDesc.Depth = m_Texture.GetDepth();
	// Mips and format.
	outDesc.MipLevels = m_Texture.GetMipMap(); outDesc.Format = (DXGI_FORMAT)m_Texture.GetFormat(); 

	// Misc other bits.
	outDesc.Usage = D3D11_USAGE_DEFAULT;
	outDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if(m_Texture.GetBindFlag() & grcBindRenderTarget)
		outDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	outDesc.CPUAccessFlags = 0;
	outDesc.MiscFlags = 0;
	outDesc.ESRAMOffsetBytes = 0;
#if _XDK_VER >= 10542
	outDesc.ESRAMUsageBytes = 0;
#endif
}


void grcDurangoPlacementTexture::FillInSRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC &outViewDesc) const
{
	outViewDesc.Format = (DXGI_FORMAT)m_Texture.GetFormat();

	const int mipCount = m_Texture.GetMipMap();
	const int arraySize = m_Texture.GetDimension();

	if (m_Texture.GetImageType() == grcImage::STANDARD
	||	m_Texture.GetImageType() == grcImage::DEPTH)
	{
		if (arraySize > 1)
		{
			outViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			outViewDesc.Texture2DArray.MostDetailedMip = 0;
			outViewDesc.Texture2DArray.MipLevels = mipCount;
			outViewDesc.Texture2DArray.FirstArraySlice = 0;
			outViewDesc.Texture2DArray.ArraySize = arraySize;
		}
		else
		{
			outViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			outViewDesc.Texture2D.MostDetailedMip = 0;
			outViewDesc.Texture2D.MipLevels = mipCount;
		}
	}
	else if (m_Texture.GetImageType() == grcImage::CUBE)
	{
		if (arraySize > 6)
		{
			outViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			outViewDesc.TextureCubeArray.MostDetailedMip = 0;
			outViewDesc.TextureCubeArray.MipLevels = mipCount;
			outViewDesc.TextureCubeArray.First2DArrayFace = 0;
			outViewDesc.TextureCubeArray.NumCubes = arraySize/6;
		}
		else
		{
			outViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			outViewDesc.TextureCube.MostDetailedMip = 0;
			outViewDesc.TextureCube.MipLevels = mipCount;
		}
	}
	else if (m_Texture.GetImageType() == grcImage::VOLUME)
	{
		outViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		outViewDesc.Texture3D.MostDetailedMip = 0;
		outViewDesc.Texture3D.MipLevels = mipCount;
	}

	Assertf(outViewDesc.ViewDimension != D3D11_SRV_DIMENSION_UNKNOWN, "Unrecognised view dimension");
}

#endif // RSG_DURANGO

/*--------------------------------------------------------------------------------------------------*/
/* Misc functions.																					*/
/*--------------------------------------------------------------------------------------------------*/


grcOrbisDurangoTileMode grcDurangoPlacementTexture::ComputeTileMode(grcImage::ImageType imageType, XG_FORMAT fmt, u32 width, u32 height, u32 depth, grcBindFlag inBindFlag)
{
#if __RESOURCECOMPILER
	if(g_sysPlatform == platform::DURANGO)
#endif //__RESOURCECOMPILER
	{

		XG_TILE_MODE ret = XG_TILE_MODE_LINEAR;
		XG_BIND_FLAG bindFlag = XG_BIND_SHADER_RESOURCE;

		if(inBindFlag & grcBindRenderTarget)
			bindFlag = (XG_BIND_FLAG)(bindFlag | XG_BIND_RENDER_TARGET);

		ret = XGComputeOptimalTileMode(ComputeResourceDimension(imageType), fmt, width, height, depth, 1, bindFlag);
		return DurangoTogrcOrbisDurangoTileMode(ret);
	}
#if __RESOURCECOMPILER
	else
	{
		if(depth == 1)
			return grcodTileMode_Thin_1dThin; // To make resources compatible with Orbis.
		else
			return grcodTileMode_Display_LinearAligned; // Not sure about 3d textures.

	}
#endif // __RESOURCECOMPILER

}

XG_RESOURCE_DIMENSION grcDurangoPlacementTexture::ComputeResourceDimension(grcImage::ImageType imageType)
{
	if(imageType == grcImage::VOLUME)
		return XG_RESOURCE_DIMENSION_TEXTURE3D;
	else
		return XG_RESOURCE_DIMENSION_TEXTURE2D;
}

#if __DECLARESTRUCT
void grcDurangoPlacementTexture::DeclareStruct(class datTypeStruct &s)
{
	grcOrbisDurangoTextureBase::DeclareStruct(s);

	STRUCT_BEGIN(grcDurangoPlacementTexture);
	STRUCT_END();
}
#endif //__DECLARESTRUCT


#define MISSING_TEXTURE_SIZE_POWER_2	7
#define MISSING_TEXTURE_CHECK_SIZE_POWER_2	3

/*======================================================================================================================================*/
/* class grcTextureDurango proper.																										*/
/*======================================================================================================================================*/

grcTextureDurango::grcTextureDurango(const char* pFilename, grcTextureFactory::TextureCreateParams *params) : grcDurangoPlacementTexture(params ? (u8)(params->Type): 0)
{
	grcImage *pImage = NULL;
#if __RESOURCECOMPILER
	void *pProcessProxy = NULL;
#endif // __RESOURCECOMPILER

	sysMemStartTemp();

	if(strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
	{		
#if __RESOURCECOMPILER
		grcImage::ImageList images;
		bool result = grcImage::RunCustomLoadFunc( pFilename, images, &pProcessProxy );
		if ( result )
			pImage = images[1];

		if (!result || pImage == NULL) // try loading normally
#endif // __RESOURCECOMPILER
		{
			pImage = grcImage::Load(pFilename);
		}
	}

	/*if(pImage)
	{
		int	w = pImage->GetWidth();
		int	h = pImage->GetHeight();

		if((w & 3) || (h & 3))
		{
			grcErrorf("grcTextureDurango::grcTextureDurango - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);
			pImage->Release();
			pImage = NULL;
#if __RESOURCECOMPILER
			pProcessProxy = NULL;
#endif
		}
	}*/

	if(!pImage)
	{
		u32 texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFF;
		pImage = grcImage::Create(1, 1, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
		u32 *texels = (u32*) pImage->GetBits();
		texels[0] = texel;
	}

	sysMemEndTemp();

	grcDevice::Result uReturnCode =  Init(pFilename,pImage,params);

#if __RESOURCECOMPILER
	RunCustomProcessFunc(pProcessProxy);
#endif

	sysMemStartTemp();

	if(pImage)
	{
		pImage->Release();
		pImage = NULL;
	}

	if(uReturnCode != 0)
	{
		u32 texel = 0xFFFFFFFF;
		pImage = grcImage::Create(1, 1, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
		u32 *texels = (u32*) pImage->GetBits();
		texels[0] = texel;

		Init(pFilename,pImage,params);

		pImage->Release();
		pImage = NULL;
	}

	sysMemEndTemp();
}


grcTextureDurango::grcTextureDurango(grcImage *pImage,grcTextureFactory::TextureCreateParams *params) : grcDurangoPlacementTexture(params ? (u8)(params->Type): 0)
{
	const char* name = "image";
#if __BANK || __RESOURCECOMPILER
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK || __RESOURCECOMPILER

	Init(name,pImage,params);
}


grcTextureDurango::grcTextureDurango(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcDurangoPlacementTexture(params ? (u8)(params->Type): 0)
{
#if __D3D11
	m_Name = NULL;
	CreateGivenDimensions(width, height, 1, 1, ConvertToXGFormat(eFormat), grcBindNone, pBuffer, params);
#else // __D3D11
	(void)(width+height);
	(void)eFormat;
	(void)pBuffer;
	(void)params;
	Assertf(0, "grcTextureDurango::grcTextureDurango(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcTextureDurango_x64ResourceData(params)...Not implemented!");
#endif // __D3D11
}


grcTextureDurango::grcTextureDurango(u32 width, u32 height, u32 depth, u32 numMips, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcDurangoPlacementTexture(params ? (u8)(params->Type): 0)
{
#if __D3D11
	m_Name = NULL;
	CreateGivenDimensions(width, height, depth, numMips, ConvertToXGFormat(eFormat), grcBindNone, pBuffer, params);
#else // __D3D11
	(void)(width+height+depth);
	(void)eFormat;
	(void)pBuffer;
	(void)params;
	(void)numMips;	
	Assertf(0, "grcTextureDurango::grcTextureDurango(u32 width, u32 height, u32 depth, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params) : grcTextureDurango_x64ResourceData(params)...Not implemented!");
#endif // __D3D11
}

#if __D3D11

grcTextureDurango::grcTextureDurango(datResource &rsc) : grcDurangoPlacementTexture(rsc)
{
	SetPrivateData();
}

#endif // __D3D11

#if DEVICE_EQAA

grcTextureDurango::grcTextureDurango(grcDeviceTexture *eqaaTexture, const grcDevice::MSAAMode &mode)
{
	Assertf(mode.m_uSamples >= mode.m_uFragments && (mode.m_uSamples > 1), "Invalid EQAA mode");
	m_ResourceTypeAndConversionFlags = (m_ResourceTypeAndConversionFlags & 0xfc) | grcTexture::RENDERTARGET;

	const D3D11_SHADER_RESOURCE_VIEW_DESC fmaskViewDesc = 
	{
		static_cast<DXGI_FORMAT>(mode.GetFmaskFormat()),// DXGI_FORMAT Format;
		D3D11_SRV_DIMENSION_TEXTURE2D,					// D3D11_SRV_DIMENSION ViewDimension;
		{
			0,	// UINT MostDetailedMip;
			1,	// UINT MipLevels;
		},												// D3D11_TEX2D_SRV Texture2D;
	};
	ASSERT_ONLY(HRESULT uReturnCode = ) GRCDEVICE.GetCurrent()->CreateShaderResourceView( eqaaTexture, &fmaskViewDesc, &GetUserMemory()->m_pShaderView );
	Assertf((uReturnCode == 0), "Error %x occurred creating texture view", uReturnCode);
}

#endif // DEVICE_EQAA

grcTextureDurango::~grcTextureDurango()
{
	//TODO
}


grcDevice::Result grcTextureDurango::Init(const char *pFilename, grcImage *pImageIn, grcTextureFactory::TextureCreateParams *params)
{
	grcImage *pImage = pImageIn;

#if __RESOURCECOMPILER
	// Sometimes we encounter DXT style textures marked a scripted RTs, we can`t render to them, they need to be 32bit really.
	if(!strncmp(pFilename, "script_rt", strlen("script_rt")) && grcImage::IsFormatDXTBlockCompressed(pImage->GetFormat()))
	{
		Warningf("WARNING!- [From file %s] grcTextureDurango::Init()...Can`t use DXTn textures as rendertargets (promoting to 32bit).\n", pFilename);
		sysMemStartTemp();
		pImage = grcImage::Create(pImageIn->GetWidth(), pImageIn->GetHeight(), pImageIn->GetDepth(), grcImage::A8R8G8B8, pImageIn->GetType(), false,0);
		sysMemEndTemp();
	}
#endif //__RESOURCECOMPILER

	m_Name = grcSaveTextureNames ? StringDuplicate(pFilename) : 0;

	//--------------------------------------------------------------------------------------------------//

	GRC_ORBIS_DURANGO_TEXTURE_DESC placementCreateInfo;

	placementCreateInfo.m_Width = pImage->GetWidth();
	placementCreateInfo.m_Height = pImage->GetHeight();
	placementCreateInfo.m_Depth = pImage->GetDepth();
	placementCreateInfo.m_ArrayDimension = pImage->GetLayerCount();
	placementCreateInfo.m_NoOfMips = (u8)(pImage->GetExtraMipCount() + 1);
#if __RESOURCECOMPILER
	placementCreateInfo.m_NoOfMips += pImage->GetConstructorInfo().m_virtualHdMips;
	bool isHdSplit = pImage->GetConstructorInfo().m_virtualHdMips > 0;
#endif
	placementCreateInfo.m_ImageType = pImage->GetType();
	placementCreateInfo.m_XGFormat = GetXGFormatFromGRCImageFormat(pImage->GetFormat());
	placementCreateInfo.m_ExtraBindFlags = (u16)grcBindNone;
	placementCreateInfo.m_TileMode = grcodTileMode_Display_LinearAligned;

	if(params && params->Type == grcTextureFactory::TextureCreateParams::RENDERTARGET)
		placementCreateInfo.m_ExtraBindFlags |= (u16)grcBindRenderTarget;

	bool forceLinear = false;
	if(params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR)
		forceLinear = true;

	if(!forceLinear)
		placementCreateInfo.m_TileMode = grcDurangoPlacementTexture::ComputeTileMode(pImage->GetType(), (XG_FORMAT)GetXGFormatFromGRCImageFormat(pImage->GetFormat()), 
			pImage->GetWidth(), pImage->GetHeight(), pImage->GetDepth(), (grcBindFlag)placementCreateInfo.m_ExtraBindFlags);

	// Record this info.
	SetFromDescription(placementCreateInfo);

#if __RESOURCECOMPILER
	if (!forceLinear && pImage->GetConstructorInfo().m_bIsLdMipChain && placementCreateInfo.m_NoOfMips > 0)
	{
		// if this is the low mip chain of a HD split texture we need to compute the layout of the full HD texture
		// and pick the tile mode of its mip[1] to use for mip[0] on this mip chain.
		// on durango it seems that the mip chain gets a different tile mode than the top mip.
		// if we don't do this, once we puzzle together the hd texture at runtime it will mean the
		// actual tile mode of mip[1] will be different from the one expected by the hardware and we'll get
		// visual artifacts.
		XG_RESOURCE_LAYOUT xgLayout;
		XG_TEXTURE2D_DESC xgDesc;
		FillInXGDesc2D(xgDesc);
		xgDesc.Width <<= 1;
		xgDesc.Height <<= 1;
		xgDesc.MipLevels++;
		CHECK_HRESULT(XGComputeTexture2DLayout(&xgDesc, &xgLayout));

		placementCreateInfo.m_TileMode = (grcOrbisDurangoTileMode)xgLayout.Plane[0].MipLayout[1].TileMode;
		m_Texture.SetTileMode((u8)placementCreateInfo.m_TileMode);
	}
#endif

	ComputeMipOffsets();

#if __RESOURCECOMPILER
	if (pImage->GetConstructorInfo().m_virtualHdMips > 0)
		m_Texture.SetMipMap((u8)(pImage->GetExtraMipCount() + 1));
#endif

	//--------------------------------------------------------------------------------------------------//

	u32 depthCount = pImage->GetDepth();
	u32 imageCount = pImage->GetLayerCount();
	u32 SubresourceCount = GetMipMapCount()*pImage->GetDepth()*imageCount;

	sysMemStartTemp();
	grcDurangoPlacementTexture::MipSliceElement *pMipSlices = NULL;
	pMipSlices = rage_new grcDurangoPlacementTexture::MipSliceElement [SubresourceCount];
	sysMemEndTemp();
	grcImage *pImageLayer = pImage;

	// Visit each image in the source.
	for (u32 imageIdx = 0; imageIdx < imageCount; imageIdx++)
	{
		Assert(pImageLayer != NULL);

		// Visit each mip-map in the current image.
		for (u32 uMip = 0; uMip < (u32)GetMipMapCount(); uMip++)
		{
			// Visit each slice in the current mip-map.
			for(u32 slice = 0; slice < max(1, (depthCount >> uMip)); slice++)
			{
				grcDurangoPlacementTexture::MipSliceElement *pSlice = &pMipSlices[uMip + slice*GetMipMapCount() + imageIdx*depthCount*GetMipMapCount()];
				pSlice->pData = pImageLayer->GetBits(slice, uMip);
				pSlice->pitch = GetStride(uMip);
				pSlice->pSameMipInNextArraySlice = NULL;

				if(slice > 0)
					// Link the slice to the previous slice.
						pMipSlices[uMip +  (slice-1)*GetMipMapCount() + imageIdx*depthCount*GetMipMapCount()].pSameMipInNextArraySlice = pSlice;
				else if(imageIdx > 0)
					// Link first depth slice of the last slice of the previous image of the same mip-level.
					pMipSlices[uMip +  0*GetMipMapCount() + (imageIdx-1)*depthCount*GetMipMapCount()].pSameMipInNextArraySlice = pSlice;
			}
		}
		// Move onto the next image/cube face.
		pImageLayer = pImageLayer->GetNextLayer();
	}

#if __RESOURCECOMPILER
	g_overrideAlignment = pImage->GetConstructorInfo().m_overrideAlignment;
#endif

	//--------------------------------------------------------------------------------------------------//

	grcDurangoPlacementTexture::Init(pMipSlices, NULL);

	//--------------------------------------------------------------------------------------------------//

#if __RESOURCECOMPILER
	if (pImage->GetConstructorInfo().m_virtualHdMips > 0)
	{
		m_Texture.SetMipMap(placementCreateInfo.m_NoOfMips);
		ComputeMipOffsets(isHdSplit);
	}
	g_overrideAlignment = 0;
#endif

	sysMemStartTemp();
	delete [] pMipSlices;
	sysMemEndTemp();

#if __RESOURCECOMPILER
	if(pImage != pImageIn)
	{
		sysMemStartTemp();
		pImage->Release();
		sysMemEndTemp();
	}
#endif //__RESOURCECOMPILER

	return S_OK;
}


grcDevice::Result grcTextureDurango::CreateGivenDimensions(u32 width, u32 height, u32 depth, u32 noOfMips, GRC_TEMP_XG_FORMAT eFormat, grcBindFlag extraBindFlags, void* pBuffer, grcTextureFactory::TextureCreateParams *params)
{
	GRC_ORBIS_DURANGO_TEXTURE_DESC placementCreateInfo;

	placementCreateInfo.m_Width = width;
	placementCreateInfo.m_Height = height;
	placementCreateInfo.m_Depth = depth;
	placementCreateInfo.m_ArrayDimension = 1;
	placementCreateInfo.m_NoOfMips = noOfMips;
	placementCreateInfo.m_XGFormat = eFormat;
	placementCreateInfo.m_ImageType = (depth == 1) ? (u8)grcImage::STANDARD : (u8)grcImage::VOLUME;
	placementCreateInfo.m_ExtraBindFlags = (u16)extraBindFlags;
	placementCreateInfo.m_TileMode = grcodTileMode_Display_LinearAligned;

	if(params)
		if(params->Type == grcTextureFactory::TextureCreateParams::RENDERTARGET)
			placementCreateInfo.m_ExtraBindFlags |= (u16)grcBindRenderTarget;

	bool forceLinear = false;
	if(params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR)
		forceLinear = true;

	if(forceLinear == false)
		placementCreateInfo.m_TileMode = grcDurangoPlacementTexture::ComputeTileMode((grcImage::ImageType)placementCreateInfo.m_ImageType, (XG_FORMAT)eFormat, 
		width, height, depth, (grcBindFlag)placementCreateInfo.m_ExtraBindFlags);

	// LDS DMC TEMP:-
	//placementCreateInfo.m_TileMode = grcodTileMode_Thin_1dThin;
	//g_LastTileMode = placementCreateInfo.m_TileMode;

	// Record this info.
	SetFromDescription(placementCreateInfo);
	ComputeMipOffsets();

	//--------------------------------------------------------------------------------------------------//

	grcDurangoPlacementTexture::MipSliceElement *pMipSlices = NULL;

	if (pBuffer)
	{
		const u8 *pSource = (const u8*)pBuffer;
		const u32 uArraySize = GetMipMapCount()*GetDepth();
		sysMemStartTemp();
		pMipSlices = rage_new grcDurangoPlacementTexture::MipSliceElement[uArraySize*GetDepth()];
		sysMemEndTemp();

		for (int uMip = 0; uMip < GetMipMapCount(); uMip++)
		{
			for(int depth=0; depth<GetDepth(); depth++)
			{
				int	iMipSize = GetStride(uMip)*GetRowCount(uMip);
				pMipSlices[uMip + depth*GetMipMapCount()].pData = (void *)pSource;
				pMipSlices[uMip + depth*GetMipMapCount()].pitch = GetStride(uMip);
				pMipSlices[uMip + depth*GetMipMapCount()].pSameMipInNextArraySlice = NULL;

				if(depth > 0)
					pMipSlices[uMip + (depth-1)*GetMipMapCount()].pSameMipInNextArraySlice = &pMipSlices[uMip + depth*GetMipMapCount()];

				pSource += iMipSize;
			}
		}
	}

	//--------------------------------------------------------------------------------------------------//

	grcDurangoPlacementTexture::Init(pMipSlices, 
#if RSG_DURANGO	// Make sure the alignment is correct, since we support allocations from more types of memory these days
		!((size_t)pBuffer & 255) ? pBuffer : NULL
#else
		pBuffer
#endif
		);

	//--------------------------------------------------------------------------------------------------//

	sysMemStartTemp();
	if(pMipSlices)
		delete [] pMipSlices;
	sysMemEndTemp();

	return S_OK;
}


/*--------------------------------------------------------------------------------------------------*/
/* grcTexture functions.																			*/
/*--------------------------------------------------------------------------------------------------*/

#if __D3D11

const grcTextureObject *grcTextureDurango::GetTexturePtr() const
{
	return GetTextureObject();
}


grcTextureObject *grcTextureDurango::GetTexturePtr()
{
	return GetTextureObject();
}


const grcDeviceView *grcTextureDurango::GetTextureView() const
{
	return ((grcDurangoPlacementTexture *)this)->GetSRV();
}


grcDeviceView *grcTextureDurango::GetTextureView()
{
	return GetSRV();
}

#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Lock/Unlock.																						*/
/*--------------------------------------------------------------------------------------------------*/

#if __D3D11

bool grcTextureDurango::LockRect(int layer, int mipLevel, grcTextureLock &lock, u32 uLockFlags) const
{
	//AssertMsg(m_InfoBits.m_HasBeenDeleted == false, "Texture has been deleted.");

	grcDurangoPlacementTextureLockInfo lockInfo;
	lockInfo.layer = layer;
	lockInfo.mipLevel = mipLevel;

	// Not sure why LockRect() is const!
	((grcDurangoPlacementTexture *)this)->Lock(lockInfo, lock, uLockFlags);

	return true;
}

void grcTextureDurango::UnlockRect(const grcTextureLock &lock) const
{
	(void)lock;
	//AssertMsg(m_InfoBits.m_HasBeenDeleted == false, "Texture has been deleted.");
	((grcDurangoPlacementTexture *)this)->Unlock();
}


// Updates the GPU copy.
void grcTextureDurango::UpdateGPUCopy(u32 Layer, u32 MipLevel, void *pBase) const
{
	(void)Layer;
	(void)MipLevel;
	(void)pBase;
	//AssertMsg(m_InfoBits.m_HasBeenDeleted == false, "Texture has been deleted.");
	((grcDurangoPlacementTexture *)this)->UpdateGPUCopy();
}



// Updates the CPU copy.
void grcTextureDurango::UpdateCPUCopy()
{
	UpdateCPUCopy_Internal();
}


// Updates the CPU copy.
bool grcTextureDurango::UpdateCPUCopy_OnlyFromRenderThread(bool dontStallWhenDrawing)
{
	(void)dontStallWhenDrawing;
	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcTextureDurango::UpdateCPUCopy()... Can only call this from the render thread!\n");
	return UpdateCPUCopy_Internal(dontStallWhenDrawing);
}


// Updates the CPU copy.
bool grcTextureDurango::UpdateCPUCopy_Internal(bool dontStallWhenDrawing)
{
	(void)dontStallWhenDrawing;
	grcDurangoPlacementTexture::UpdateCPUCopy();
	return true;
}


// LDS DMC TEMP:-
void  grcTextureDurango::CopyFromGPUToStagingBuffer()
{
}


bool grcTextureDurango::MapStagingBufferToBackingStore(bool dontStallWhenDrawing)
{
	dontStallWhenDrawing;
	return true;
}


void grcTextureDurango::BeginTempCPUResource()
{
}

void grcTextureDurango::EndTempCPUResource()
{
}

#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Copy functions.																					*/
/*--------------------------------------------------------------------------------------------------*/


bool grcTextureDurango::Copy(const grcImage*)
{
	AssertMsg(0, "grcTextureDurango::Copy - Not implemented");
	/*
	if (m_LayerCount != pImage->GetLayerCount()-1 || GetWidth() != pImage->GetWidth() || GetHeight() != pImage->GetHeight() || 
		GetMipMapCount() != pImage->GetExtraMipCount()+1)
		return false;
	*/
	return false;
}


#if __D3D11

bool grcTextureDurango::Copy(const void * /*pvSrc*/, u32 uWidth, u32 uHeight, u32 /*uDepth*/)
{
	if (/*m_LayerCount != pImage->GetLayerCount()-1 ||*/ GetWidth() != (int)uWidth || GetHeight() != (int)uHeight) // || m_nMipCount != pImage->GetExtraMipCount()+1)
	{
		return false;
	}

	return true;
}

bool grcTextureDurango::Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex)
{
	Assert(pSource != NULL);
	Assert(pSource->GetWidth() == GetWidth());
	Assert(pSource->GetHeight() == GetHeight());

	ID3D11Texture2D* poSrc = (ID3D11Texture2D*)(pSource->GetTexturePtr());
	ID3D11Texture2D* poDst = static_cast<ID3D11Texture2D *>(GetTexturePtr());	
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

bool grcTextureDurango::CopyTo(grcImage* pImage, bool bInvert)
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

		int	mipSize = GetStride(0) * GetHeight();
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

bool grcTextureDurango::Copy2D(const void* pSrc, u32 imgFormat, u32 width, u32 height, u32 numMips)
{
	grcImage::Format format = (grcImage::Format)(imgFormat);

	// get out if something doesn't match
	if (GetLayerCount() != 1U || format != (grcImage::Format)GetImageFormat() ||  GetWidth() != (int)width || GetHeight() != (int)height || GetMipMapCount() != (int)numMips)
	{
		return false;
	}

	// cache mip data
	const u32 bpp		= grcImage::GetFormatBitsPerPixel(format);
	const u32 blockSize	= grcImage::IsFormatDXTBlockCompressed(format) ? 4 : 1;

	u32 mipWidth = width;
	u32 mipHeight = height;
	u32 mipPitch = mipWidth*bpp/8;

	if (imgFormat == grcImage::DXT1 || imgFormat == grcImage::DXT5A)
	{
		mipPitch = ((mipWidth+3)/4) * 8;
	}
	// TODO -- support BC6H, BC7
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

		if (grcImage::IsFormatDXTBlockCompressed(format))
		{
			if (format == grcImage::DXT1 || format == grcImage::DXT5A)
			{
				mipPitch = ((mipWidth+3)/4) * 8;
			}
			else
			{
				mipPitch = ((mipWidth+3)/4) * 16;
			}
		}

		curMipOffset += mipSizeInBytes;
		mipSizeInBytes = (mipHeight*mipPitch);

		pSrcMip += mipSizeInBytes;

	}

	return true;
}

void grcTextureDurango::TileInPlace(u32 numMips)
{
	//note this doesn't deal with volume, also you have to have previously locked this
	Assertf(m_pGraphicsMem, "No texture info to tile!");

	MipSliceElement* mipElement = (MipSliceElement*)alloca(sizeof(MipSliceElement)*numMips);

	u64 offset = 0;
	u64 pitch = 0;

	for (u32 i = 0; i < numMips; i++)
	{
		GetLockInfo(i, offset, pitch);

		mipElement[i].pData = (char *)m_pGraphicsMem + offset;
		mipElement[i].pitch = pitch;
		mipElement[i].pSameMipInNextArraySlice = NULL;
	}

	CopyOverTexels(&mipElement[0], numMips);
}
#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Misc access functions.																			*/
/*--------------------------------------------------------------------------------------------------*/

u32	grcTextureDurango::GetImageFormat() const
{ 
#if __D3D11
	return grcTextureFactoryDX11::GetImageFormatStatic((DXGI_FORMAT)m_Texture.GetFormat());
#else // !__D3D11
	Assertf(0, "grcTextureDurango::GetImageFormat()...Not implemented!");
	return 0;
#endif // !__D3D11
}

#if __D3D11

bool grcTextureDurango::IsValid() const 
{ 
	return true;
}


void grcTextureDurango::SetPrivateData()
{
#if !__D3D11_MONO_DRIVER
	grcTexture* pTex = this;

	if (GetTexturePtr())
	{
		if (GetName())
		{
			CHECK_HRESULT(((ID3D11DeviceChild*)GetTexturePtr())->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen(GetName())+1,GetName()));
		}
		CHECK_HRESULT(((ID3D11DeviceChild*)GetTexturePtr())->SetPrivateData(TextureBackPointerGuid,sizeof(pTex),&pTex));
	}

	if (GetTextureView())
	{
		char szTempName[256];
		sprintf_s(szTempName, "%s - Resource", GetName());
		static_cast<ID3D11DeviceChild*>(GetTextureView())->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName);
	}
#endif // !__D3D11_MONO_DRIVER
}

#endif // __D3D11

#if __DECLARESTRUCT
void grcTextureDurango::DeclareStruct(class datTypeStruct &s)
{
	grcDurangoPlacementTexture::DeclareStruct(s);
	STRUCT_BEGIN(grcTextureDurango);
	STRUCT_END();
}
#endif //__DECLARESTRUCT

}	// namespace rage

#endif	// (RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO


