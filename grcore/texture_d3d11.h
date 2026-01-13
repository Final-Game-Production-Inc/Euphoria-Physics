#ifndef GRCORE_TEXTUREDX11_H
#define GRCORE_TEXTUREDX11_H

#if	RSG_PC // && __D3D11 toolbuilder_gta5_dev_rex_2_targets doesn`t like __D3D11 being here.

#include	"atl/array.h"
#include	"grcore/device.h"
#include	"grcore/texture.h"
#include	"vector/vector3.h"
#include	"grcore/resourcecache.h"
#include	"grcore/texturepc.h"
#include	"grcore/locktypes.h"

struct D3D11_SUBRESOURCE_DATA;

// The number of textures which can have read/write access.
#define GRC_TEXTURE_DX11_EXTRA_DATA_MAX 512

namespace rage 
{

// DOM-IGNORE-BEGIN
class grcRenderTargetDX11;


class grcTextureDX11 : public grcTexturePC
{
		friend class grcTextureFactoryPC;
	
		// Creation/destruction related functions.
public:
		grcTextureDX11(const char *pFilename,grcTextureFactory::TextureCreateParams *params);
		grcTextureDX11(class grcImage *pImage,grcTextureFactory::TextureCreateParams *params);
		grcTextureDX11(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);
		grcTextureDX11(u32 width, u32 height, u32 depth, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);
		grcTextureDX11(class datResource &rsc);
		virtual ~grcTextureDX11();

		void DeleteResources();

protected:
		void				CreateFromBackingStore(bool bRecreate = false);
private:
		grcDevice::Result	Init(const char *pFilename,class grcImage*, grcTextureFactory::TextureCreateParams *params);
		grcDevice::Result   CreateGivenDimensions(u32 width, u32 height, u32 depth, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);

		typedef struct CREATE_INTERNAL_INFO
		{
			u32 SubresourceCount;
			D3D11_SUBRESOURCE_DATA *pSubresourceData;
		};
		grcDevice::Result CreateInternal(CREATE_INTERNAL_INFO &CreateInfo, grcTextureCreateType CreateType, grcsTextureSyncType SyncType, grcBindFlag ExtraBindFlags, bool IsFromBackingStore);
		static grcTextureCreateType ProcessCreateType(grcTextureCreateType createType);
		static void GetDescUsageAndCPUAccessFlags(grcTextureCreateType createType, u32 &Usage, u32 &CPUAccessFlags);
		static bool DoesNeedStagingTexture(grcTextureCreateType CreateType);
		static bool UsesBackingStoreForLocks(grcTextureCreateType CreateType);
		u32 CalculateMemoryForASingleLayer() const;
		u32 CalclulateMemoryUpToMipCount(u32 MipCount) const;
		grcTextureCreateType GetCreateTypeFromParams(grcTextureFactory::TextureCreateParams *pParams);

		// Allocates memory for the CPU side copy buffer.
		void AllocCPUCopy(u32 Size);
		// Free the CPU copy.
		void FreeCPUCopy();

public:
		// Lock/Unlock.
		bool				LockRect		(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
		void				UnlockRect		(const grcTextureLock &lock) const;
		// Performs a Lock().
		bool				LockRect_Internal(int layer, int mipLevel, grcTextureLock &lock, u32 uLockFlags, bool AllowContextLocks) const;
		// Performs the UnLock().
		void				UnlockRect_Internal(const grcTextureLock &lock, bool AllowContextLocks) const;
		
		// Updates the GPU copy.
		void				UpdateGPUCopy(u32 Layer, u32 MipLevel, void *pBase) const;
#if RSG_PC
		void				UpdateGPUCopyFromBackingStore();
#endif
private:
		void				UpdateGPUCopy_WithThreadSyncing(u32 Layer, u32 MipLevel, void *pBase) const;
		void				UpdateGPUCopy_Internal(u32 Layer, u32 MipLevel, void *pBase) const;
#if RSG_PC
		void				UpdateGPUCopy();
#endif // RSG_PC

public:
		bool				UpdateCPUCopy(bool dontStallWhenDrawing = false);
		bool				UpdateCPUCopy_OnlyFromRenderThread(bool dontStallWhenDrawing = false);
		void				CopyFromGPUToStagingBuffer();
		bool				MapStagingBufferToBackingStore(bool dontStallWhenDrawing = false);

#if RSG_PC
		bool				IsImmutable();
		void				StripImmutability(bool bPreserveTextureData=true);
#endif // RSG_PC

private:
		bool				UpdateCPUCopy_Internal(bool dontStallWhenDrawing = false);

public:
		// Make me to lockable texture 
		void				BeginTempCPUResource();
		void				EndTempCPUResource();
		grcDevice::Result	InitializeTempStagingTexture();
		void				ReleaseTempStagingTexture();
#if !__D3D11
		bool				HasStagingTexture(){ return m_StagingTexture != NULL;}
#else
		bool				HasStagingTexture(){ return (m_pExtraData && m_pExtraData->m_pStagingTexture);}
#endif


public:
		// Copy functions.
		bool				Copy(const grcImage*);
		bool				Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
		bool				Copy(const void * pvSrc, u32 uWidth, u32 uHeight, u32 uDepth);
		bool				CopyTo(grcImage* pImage, bool bInvert = false);
		bool				Copy2D(const void *pSrc, u32 imgFormat, u32 uWidth, u32 uHeight, u32 numMips);
		bool				Copy2D(const void *pSrc, const grcPoint & oSrcDim, const grcRect & oDstRect, const grcTextureLock &lock, s32 iMipLevel);

		// Misc access functions.
		virtual u32			GetImageFormat() const;
		int					GetBitsPerPixel() const;
		int					GetStride(u32 uMipLevel) const;
		int					GetRowCount(u32 uMipLevel) const;
		int					GetSliceCount(u32 uMipLevel) const;
		int					GetTotalMipSize(u32 uMipLevel) const;
		int					GetBlockHeight_UNUSED(u32 uMipLevel) const;
		virtual bool		IsValid() const;
		virtual ChannelBits	FindUsedChannels() const;
protected:
		void				SetPrivateData();

private:
		// Extra info functions.
		static grcTextureDX11_ExtraData *AllocExtraData(grcsTextureSyncType SyncType);
		static void FreeExtraData(grcTextureDX11_ExtraData *pExtraData);
		// Returns the staging texture.
		grcDeviceTexture *GetStagingTexture() const;

public:
		// Sets the callback function to insert GPU updates into the draw list (or equivalent) for the calling thread.
		static void SetInsertUpdateCommandIntoDrawListFunc(void (*pFunc)(void *, void *, u32 Layer, u32 MipLevel, bool uponCancelMemoryHint)) { sm_pInsertUpdateGPUCopyCommandIntoDrawList = pFunc; }
		// Sets the cancel update GPU copy call back.
		static void SetCancelPendingUpdateGPUCopy(void (*pFunc)(void *)) { sm_pCancelPendingUpdateGPUCopy = pFunc; }
		// Sets the temp allocator call back.
		static void SetAllocTempMemory(void *(*pFunc)(u32)) { sm_pAllocTempMemory = pFunc; }

protected:
		// Texture factory functions.
		void				DeviceLost();
		void				DeviceReset();

#if RSG_PC
		grcDevice::Stereo_t m_StereoRTMode;
#endif
public:
	#if __DECLARESTRUCT
		void				DeclareStruct(class datTypeStruct &s);
	#endif

public:
	static void *AllocTempMemory(u32 Size);

private:
		static u32 sm_ExtraDatasCount;
		static grcTextureDX11_ExtraData *sm_pExtraDatas;

		// Called when we need to get a GPU update inserted into the "draw list" or similar.
		static __THREAD void (*sm_pInsertUpdateGPUCopyCommandIntoDrawList)(void *, void *, u32 Layer, u32 MipLevel, bool uponCancelMemoryHint);
		// Called when a buffer`s pending GPU updates needs to be canceled.
		static __THREAD void (*sm_pCancelPendingUpdateGPUCopy)(void *);
		// Callback to provide memory for game thread buffer updates - must be live for 2 frames.
		static void *(*sm_pAllocTempMemory)(u32 size);

		friend class dlCmdGrcDeviceUpdateBuffer;
};

	// DOM-IGNORE-END

}	// namespace rage

#endif	// RSG_PC // && __D3D11 toolbuilder_gta5_dev_rex_2_targets doesn`t like __D3D11 being here.

#endif // GRCORE_TEXTUREDX11_H
