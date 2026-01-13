#ifndef GRCORE_TEXTURE_DURANGO_H
#define GRCORE_TEXTURE_DURANGO_H

#if	(RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO

#include	"atl/array.h"
#include	"grcore/config_switches.h"
#include	"grcore/device.h"
#include	"grcore/texture.h"
#include	"vector/vector3.h"
#include	"grcore/resourcecache.h"
#include	"grcore/texturepc.h"
#include	"grcore/locktypes.h"
#include	"grcore/orbisdurangoresourcebase.h"
#include	"grcore/image.h"

struct XG_TEXTURE2D_DESC;
struct XG_TEXTURE3D_DESC;
struct D3D11_TEXTURE2D_DESC;
struct XG_RESOURCE_LAYOUT;

enum XG_TILE_MODE;
enum XG_FORMAT;
enum XG_RESOURCE_DIMENSION;

struct ID3D11Resource;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SHADER_RESOURCE_VIEW_DESC;

class XGTextureAddressComputer;

namespace rage 
{

// DOM-IGNORE-BEGIN
class grcRenderTargetDX11;
class grcDurangoPlacementTexture;
struct grcDurangoPlacementTextureLockInfo;

/*======================================================================================================================================*/
/* grcDurangoPlacementTexture classes.																									*/
/*======================================================================================================================================*/

class grcDurangoPlacementTexture : public grcOrbisDurangoTextureBase 
{
/*--------------------------------------------------------------------------------------------------*/
/* Internal classes.																				*/
/*--------------------------------------------------------------------------------------------------*/

public:
	// A list of images at one mip-level to represent an mip slice.
	class MipSliceElement
	{
	public:
		MipSliceElement()
		{
			pData = NULL;
			pitch = 0;
			pSameMipInNextArraySlice = NULL;
		}
		~MipSliceElement()
		{
		}

	public:
		void *pData;
		size_t pitch;
		class MipSliceElement *pSameMipInNextArraySlice; // Next one at the same mip-level.
	};

	//--------------------------------------------------------------------------------------------------//

private:
	// TODO:- Maybe make rectangular.
	class LockMemoryArea
	{
	public:
		LockMemoryArea() { m_pMem = NULL; m_Size = 0; }
		~LockMemoryArea() {}

		bool IsValid()
		{
			return (m_pMem != NULL);
		}

		bool operator ==(LockMemoryArea &other)
		{
			if(m_pMem != other.m_pMem)
				return false;
			if(m_Size != other.m_Size)
				return false;
			return true;
		}

	#if RSG_DURANGO
		void FlushCpu();
	#endif // RSG_DURANGO

	public:
		void *m_pMem;
		size_t m_Size;
	};

/*--------------------------------------------------------------------------------------------------*/
/* Constructors/Destructor.																			*/
/*--------------------------------------------------------------------------------------------------*/

public:
	grcDurangoPlacementTexture();
	grcDurangoPlacementTexture(u8 type);
	grcDurangoPlacementTexture(GRC_ORBIS_DURANGO_TEXTURE_DESC &info, MipSliceElement *pArrayOfMipSlices, void *pPreAllocatedGraphicsMem, u8 type);
	grcDurangoPlacementTexture(datResource &rsc);
	~grcDurangoPlacementTexture();

	void Init(MipSliceElement *pArrayOfMipSlices, void *pPreAllocatedGraphicsMemory);
	void AllocateMemory();
	void Fill(MipSliceElement *pArrayOfMipSlices);
	void ComputeMipOffsets(bool bIsHdSplit = false);

/*--------------------------------------------------------------------------------------------------*/
/* Interface.																						*/
/*--------------------------------------------------------------------------------------------------*/

public:
#if RSG_DURANGO
	void Lock(grcDurangoPlacementTextureLockInfo &lockInfoIn, grcTextureLock &lockInfoOut, u32 uLockFlags);
	void Unlock();
	// TODO:- Change this for a mark as dirty when this texture is written to as a render target.
	void UpdateCPUCopy();
	void UpdateGPUCopy();
	grcDeviceView *GetSRV();
	grcTextureObject *GetTextureObject() const;

private:
	void ComputeLockInfo(grcDurangoPlacementTextureLockInfo &lockInfoIn, grcTextureLock &lockInfoOut, LockMemoryArea &MemArea);
#endif // RSG_DURANGO

/*--------------------------------------------------------------------------------------------------*/
/* Internal helper functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

private:
	void CopyOverTexels(MipSliceElement *pArrayOfMipSlices, u32 maxNumberOfMips);
#if RSG_DURANGO
public:
	void CreateD3DResources(grcOrbisDurangoTextureBase &info, void *pGPUVirtualAddr);
private:
	void DestroyD3DResources();
	void AllocateXGLayout();
#endif // RSG_DURANGO
	void CalculateMemoryRequirements(grcOrbisDurangoTextureBase &info, size_t &memory, size_t &alignment);

/*--------------------------------------------------------------------------------------------------*/
/* XG/DX11 desc functions.																			*/
/*--------------------------------------------------------------------------------------------------*/

public:
	// XG desc functions.
	void FillInXGDesc2D(XG_TEXTURE2D_DESC &outDesc) const;
	void FillInXGDesc3D(XG_TEXTURE3D_DESC &outDesc) const;
#if RSG_DURANGO
	// DX11 desc functions.
	void FillInD3D11Desc2D(D3D11_TEXTURE2D_DESC &outDesc) const;
	void FillInD3D11Desc3D(D3D11_TEXTURE3D_DESC &outDesc) const;
	void FillInSRVDesc(D3D11_SHADER_RESOURCE_VIEW_DESC &outViewDesc) const;
#endif // RSG_DURANGO

	// Misc functions.
	static grcOrbisDurangoTileMode grcDurangoPlacementTexture::ComputeTileMode(grcImage::ImageType imageType, XG_FORMAT fmt, u32 width, u32 height, u32 depth, grcBindFlag bindFlag);
	static XG_RESOURCE_DIMENSION ComputeResourceDimension(grcImage::ImageType imageType);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif //__DECLARESTRUCT


#if RSG_DURANGO
	struct USER_MEMORY
	{
		ID3D11Resource *m_pTexture;
		grcDeviceView *m_pShaderView;
		LockMemoryArea m_MemoryArea;
		u32 m_CacheFlags;
	};
	CompileTimeAssert(sizeof(USER_MEMORY) <= sizeof(datPadding<GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE, u64>));
	USER_MEMORY *GetUserMemory() { return (USER_MEMORY *)&m_UserMemory; }
	USER_MEMORY *GetUserMemory() const { return (USER_MEMORY *)&m_UserMemory; }
#endif // RSG_DURANGO

/*--------------------------------------------------------------------------------------------------*/
/* Data members.																					*/
/*--------------------------------------------------------------------------------------------------*/
	
private:
	static sysCriticalSectionToken				m_CritialSectionToken;

	friend class grcDurangoTextureLockAndLayout;
	friend class grcTextureDurango;
};

/*======================================================================================================================================*/
/* class grcTextureDurango.																												*/
/*======================================================================================================================================*/

class grcTextureDurango : public grcDurangoPlacementTexture
{
	friend class grcTextureFactoryPC;

public:
	// Creation/destruction related functions.
	grcTextureDurango(const char *pFilename,grcTextureFactory::TextureCreateParams *params);
	grcTextureDurango(class grcImage *pImage,grcTextureFactory::TextureCreateParams *params);
	grcTextureDurango(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);
	grcTextureDurango(u32 width, u32 height, u32 depth, u32 numMips, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);
#if __D3D11
	grcTextureDurango(class datResource &rsc);
#endif // __D3D11
#if DEVICE_EQAA
	grcTextureDurango(grcDeviceTexture *eqaaTexture, const grcDevice::MSAAMode &mode);	//init as Fmask
#endif // DEVICE_EQAA
	virtual ~grcTextureDurango();

private:
	grcDevice::Result	Init(const char *pFilename,class grcImage*, grcTextureFactory::TextureCreateParams *params);
	grcDevice::Result   CreateGivenDimensions(u32 width, u32 height, u32 depth, u32 noOfMips, GRC_TEMP_XG_FORMAT eFormat, grcBindFlag extraBindFlags, void* pBuffer, grcTextureFactory::TextureCreateParams *params);

#if __D3D11

public:
	// grcTexture functions.
	const grcTextureObject *GetTexturePtr() const;
	grcTextureObject *GetTexturePtr();
	const grcDeviceView *GetTextureView() const;
	grcDeviceView *GetTextureView();
	grcDurangoPlacementTexture* GetPlacementTexture() { return this; }
	const grcDurangoPlacementTexture* GetPlacementTexture() const { return this; }
	grcCellGcmTextureWrapper& GetPlacementTextureCellGcmTextureWrapper() { return GetPlacementTexture()->m_Texture; }
	const grcCellGcmTextureWrapper& GetPlacementTextureCellGcmTextureWrapper() const { return GetPlacementTexture()->m_Texture; }

public:
	// Lock/Unlock.
	bool				LockRect		(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	void				UnlockRect		(const grcTextureLock &lock) const;

	// Updates the GPU copy.
	void				UpdateCPUCopy();
	bool				UpdateCPUCopy_OnlyFromRenderThread(bool dontStallWhenDrawing = false);
	bool				UpdateCPUCopy_Internal(bool dontStallWhenDrawing = false);
	void				UpdateGPUCopy(u32 Layer, u32 MipLevel, void *pBase) const;

	// LDS DMC TEMP:-
	void				CopyFromGPUToStagingBuffer();
	bool				MapStagingBufferToBackingStore(bool dontStallWhenDrawing = false);
	void				BeginTempCPUResource();
	void				EndTempCPUResource();
#endif // __D3D11

public:
	// Copy functions.
	bool				Copy(const grcImage*);
#if __D3D11
	bool				Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
	bool				Copy(const void * pvSrc, u32 uWidth, u32 uHeight, u32 uDepth);
	bool				CopyTo(grcImage* pImage, bool bInvert = false);
	bool				Copy2D(const void *pSrc, u32 imgFormat, u32 uWidth, u32 uHeight, u32 numMips);

	void				TileInPlace(u32 numMips = 1); //helper function for tiling textures that have untiled data written to them

#endif // __D3D11
#if RSG_DURANGO
	void				GetMipLinearTexels(void* pTexels, int layerIndex, int mipIndex, int memSize, XGTextureAddressComputer* pComputer);
	void				GetLinearTexels(void* linearMem, int memSize, int maxMips) const;
#endif
#if RSG_DURANGO && !__FINAL && !__RESOURCECOMPILER
	grcImage*			CreateLinearImageCopy(int maxMips = 0) const;
#endif // RSG_DURANGO && !__FINAL && !__RESOURCECOMPILER

	// Misc access functions.
	u32					GetImageFormat() const;
#if __D3D11
	virtual bool		IsValid() const;
protected:
	void				SetPrivateData();
#endif // __D3D11

#if __DECLARESTRUCT
public:
	void				DeclareStruct(class datTypeStruct &s);
#endif
};

// DOM-IGNORE-END

}	// namespace rage

#endif // (RSG_PC && __64BIT && __RESOURCECOMPILER) || RSG_DURANGO

#endif // GRCORE_TEXTURE_DURANGO_H
