//
// grcore/texturepc.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTUREPC_H
#define GRCORE_TEXTUREPC_H

#if	RSG_PC || RSG_DURANGO

#include	"atl/array.h"
#include	"grcore/config.h"
#include	"grcore/device.h"
#include	"grcore/effect_mrt_config.h"
#include	"grcore/rendertarget_common.h"
#include	"grcore/texture.h"
#include	"system/threadtype.h"
#include	"vector/vector3.h"

struct IUnknown;

namespace rage {

// DOM-IGNORE-BEGIN
class grcRenderTargetPC;
class grcImage;

class grcTextureDX11_ExtraData
{
public:
	grcTextureDX11_ExtraData() 
	{ 
		m_SyncType = 0;
		m_LockFlags = 0;
		m_pStagingTexture = NULL;
		m_Mutex = NULL;
	}
	~grcTextureDX11_ExtraData() 
	{ 
		m_pStagingTexture = NULL;
		m_Mutex = NULL;
	}

public:
	u32 m_SyncType : 2, m_LockFlags : 2;
	grcDeviceTexture *m_pStagingTexture;
	union
	{
		sysIpcMutex m_Mutex;
		sysIpcSema m_DirtySemaphore;
	};
};


class grcTextureFactoryPC : public grcTextureFactory
{
	public:
		static grcTextureFactoryPC* CreateTextureFactory();

		grcTextureFactoryPC();
		virtual ~grcTextureFactoryPC();
		
		virtual bool				SupportsFormat(grcTextureFormat) { Assert(0 && "Pure Virtual Function"); return false; }
		virtual u32					Translate(grcTextureFormat) { Assert(0 && "Pure Virtual Function"); return 0; } 
		virtual u32					GetD3DFormat(grcImage *) { Assert(0 && "Pure Virtual Function"); return 0; } 
		virtual u32					GetImageFormat(u32) { Assert(0 && "Pure Virtual Function"); return 0; } 
		virtual u32					GetBitsPerPixel(u32) { Assert(0 && "Pure Virtual Function"); return 0; } 

		virtual grcTexture			*Create				(const char *,TextureCreateParams *) { Assert(0 && "Pure Virtual Function"); return NULL; }
		virtual grcTexture			*Create				(class grcImage*,TextureCreateParams *) { Assert(0 && "Pure Virtual Function"); return NULL; }
		virtual grcTexture			*Create				(u32, u32, u32, void*, u32, TextureCreateParams *) { Assert(0 && "Pure Virtual Function"); return NULL; }
		virtual grcRenderTarget		*CreateRenderTarget	(const char *, grcRenderTargetType, int, int, int, CreateParams* WIN32PC_ONLY(, grcRenderTarget*) DURANGO_ONLY(, grcRenderTarget*))
									{ Assert(0 && "Pure Virtual Function"); return NULL; }
		virtual grcRenderTarget		*CreateRenderTarget	(const char *, const grcTextureObject* WIN32PC_ONLY(, grcRenderTarget*) DURANGO_ONLY(, grcRenderTarget*))
									{ AssertMsg(0, "Pure Virtual Function"); return NULL; }
		virtual void				LockRenderTarget	(int, const grcRenderTarget *, const grcRenderTarget *, u32 = 0,bool = true, u32 = 0) { Assert(0 && "Pure Virtual Function"); }
		virtual void				UnlockRenderTarget	(int, const grcResolveFlags *) { Assert(0 && "Pure Virtual Function"); }
		virtual void				LockMRT				(const grcRenderTarget **, const grcRenderTarget *, const u32* = NULL) { Assert(0 && "Pure Virtual Function"); }
		virtual void				UnlockMRT			(const grcResolveFlagsMrt* = NULL) { Assert(0 && "Pure Virtual Function"); }

		virtual u32					GetTextureDataSize	(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory);

		virtual void				PlaceTexture		(class datResource &, grcTexture &) { Assert(0 && "Pure Virtual Function"); };
		// TO BE CALLED BY grcRenderTarget ONLY
		virtual void				RemoveRenderTarget	(grcRenderTargetPC *tgt);

		virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
		virtual grcRenderTarget *GetBackBuffer(bool realize=true);
		virtual const grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const;
		virtual grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer));
		virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
		virtual grcRenderTarget *GetFrontBufferDepth(bool realize=true);
		virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
		virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);
		virtual void BindDefaultRenderTargets() { Assert(0 && "Pure Virtual Function"); }

		virtual void Lost() { Assert(0 && "Pure Virtual Function"); }
		virtual void Reset() { Assert(0 && "Pure Virtual Function"); }

		static void ReportUnusedTextures();

	#if DEBUG_SEALING_OF_DRAWLISTS
		virtual void RecordDrawListDebugString(const char *pStr) { (void)pStr; }
		virtual char *GetDrawListDebugString() { return (char *)NULL; }
		virtual bool AreAnyRenderTargetsSet() { return false; }
		virtual bool HaveAnyDrawsBeenIssuedWithNoTargetsSet() { return false; }
		virtual void OutputSetRenderTargets() {}
		virtual void OnDraw() {}
	#endif // DEBUG_SEALING_OF_DRAWLISTS

	protected:
		// PURPOSE: Properly handle a device lost event
		static void DeviceLost();

		// PURPOSE: Properly handle a device reset event
		static void DeviceReset();

	#if RSG_PC
		static const int MAX_RENDERTARGETS = 768;
		static atFixedArray<grcRenderTargetPC*, MAX_RENDERTARGETS> sm_ActiveTargets;
	#endif // RSG_PC
};

class	grcTexturePC	:	public	grcTexture
{
		friend class grcTextureFactoryPC;

	private:
		static u32 m_uQuality;						// From settings.xml
		static __THREAD u32 m_uQualityOverride;		// Can be overridden per thread

	public:
		enum
		{
			LOW = 0,
			MEDIUM,
			HIGH,
			INVALID
		};
		static void SetTextureQuality(u32 eQuality) { Assert(eQuality <= HIGH); Assert(sysThreadType::IsUpdateThread()); m_uQuality = eQuality; }
		static void PushTextureQuality(u32 eQuality) { Assert(eQuality <= HIGH); m_uQualityOverride = eQuality; }
		static void PopTextureQuality() { Assert(m_uQualityOverride != INVALID); m_uQualityOverride = INVALID; }
		static u32  GetTextureQuality() { return m_uQualityOverride == INVALID ? m_uQuality : m_uQualityOverride; }
		static u32  GetMipLevelScaleQuality(u32 uImageType, u32 uWidth, u32 uHeight, u32 uMipCount, u32 uFormat);

#if RSG_PC
		static grcTexture*	GetPrivateData(const grcTextureObject *pTexObj);

		u32 GetFormat() const { return m_nFormat; }

	protected:
		u32					m_ExtraFlags;
	#if __64BIT
		u32					m_ExtraFlagsPadding;
	#endif // __64BIT
		u16					m_Width;
		u16					m_Height;
		u16					m_Depth;
		u16					m_nMipStride;
		u32					m_nFormat;		
		u8					m_ImageType;
		u8					m_nMipCount;
		u8					m_CutMipLevels;
	#if !__D3D11
		bool				m_IsSRGB;
	#else
		// NOTE:- On PC sizeof(bool) = 1 (8 bits).
		struct grcDX11InfoBits
		{
			u8					m_IsSRGB : 1,
								m_ReadWriteAccess : 3,
								m_OwnsBackingStore : 1,
								m_HasBeenDeleted : 1,
								m_Dynamic : 1;
			mutable u8			m_Dirty : 1;
		};
		union
		{
			grcDX11InfoBits     m_InfoBits;
			bool				m_IsSRGB_Byte;
		};
	#endif
		static grcTexturePC	*sm_First;
		grcTexturePC		*m_Next;
		grcTexturePC		*m_Previous;
		
#if __PAGING
		void				*m_BackingStore;
#endif
		// DX10 requires more stuff - DX9 and DX10 must share texture data structure space so that they can share data
		grcDeviceView*		m_pShaderResourceView;
#if !__D3D11
		// Staging texture as normal.
		grcDeviceTexture*	m_StagingTexture;
#else
		// Pointer to a structure which contains the staging texture + other info needed for locking/unlocking.
		grcTextureDX11_ExtraData	*m_pExtraData;
#endif

		virtual void DeviceLost() = 0;
		virtual void DeviceReset() = 0;
		virtual void CreateFromBackingStore(bool bRecreate = false) = 0;
		virtual void SetPrivateData() = 0;

public:
		grcTexturePC(grcTextureFactory::TextureCreateParams *params);
		grcTexturePC(class datResource &rsc);
		virtual ~grcTexturePC();

		grcTextureObject	*GetTexturePtr	(void);
		const grcTextureObject *GetTexturePtr(void) const;

		grcDeviceView *GetTextureView();

		void HDOverrideSwap(grcTexturePC* alternateTexture);

		int					GetWidth		(void) const { return(m_Width);  }
		int					GetHeight		(void) const { return(m_Height); }
		int					GetDepth		(void) const { return(m_Depth);  }
		int					GetImageType	(void) const { return(m_ImageType); }
		int					GetMipMapCount() const { return m_nMipCount; }
		int					GetArraySize	(void) const { return 1; }	// need to fix this for tex array
		
		virtual bool		LockRect		(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const = 0;
		virtual void		UnlockRect		(const grcTextureLock &lock) const = 0;

#if __DECLARESTRUCT
		void				DeclareStruct(class datTypeStruct &s);
#endif

		virtual bool		Copy(const grcImage*) { return false; }
		virtual bool		Copy(const grcTexture* pSource, s32 dstSliceIndex = -1, s32 dstMipIndex = -1, s32 srcSliceIndex = 0, s32 srcMipIndex = 0) = 0;
		virtual bool		Copy(const void *, u32, u32, u32) { return false; }
		virtual bool		CopyTo(grcImage*, bool = false) {return false; }

		void				UnlinkFromChain();

	#if !__D3D11
		bool				IsSRGB() const { return m_IsSRGB; }
	#else
		bool				IsSRGB() const { return m_InfoBits.m_IsSRGB; }
	#endif

		virtual bool		IsValid() const;

#if __BANK
		virtual bool		Save() { return false; }
#endif // __BANK
#endif // RSG_PC
};


#if RSG_PC
class	grcRenderTargetPC	:	public	grcRenderTarget
{
	friend class grcTextureFactoryPC;
	friend class grcTextureFactoryDX9;
	friend class grcTextureFactoryDX10;

public:
	grcRenderTargetPC();
	virtual	~grcRenderTargetPC();

	static bool			SupportsFormat(grcTextureFormat eFormat);

	const grcTextureObject  *GetTexturePtr	(void) const;
	grcTextureObject		*GetTexturePtr	(void);

	int					GetWidth		(void) const { return(m_Width); }
	int					GetHeight		(void) const { return(m_Height); }
	int					GetDepth		(void) const { return 1; }
	int					GetMipMapCount	(void) const { return m_MipLevels; }
	int					GetArraySize	(void) const { return m_ArraySize; }
	int					GetBitsPerPixel() const;
	const grcTextureFormat GetFormat() const {return (grcTextureFormat)m_Format;}

	virtual bool		Copy(const grcTexture* pSource, s32 dstSliceIndex = -1, s32 dstMipIndex = -1, s32 srcSliceIndex = 0, s32 srcMipIndex = 0) = 0;
	virtual bool		CopyTo(grcImage* pImage, bool bInvert = false, u32 uPixelOffset = 0) = 0;
	virtual bool		Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) = 0;

#if __D3D9
	void							SetUnresolved()		{ m_ResolvedObject = NULL; }
	const grcTextureObject*			GetResolved() const	{ return m_ResolvedObject; }
#else
	inline const grcTextureObject*	DebugGetLastResolved() const;
	inline void 					DebugSetUnresolved();
#endif

	virtual void		Resolve(grcRenderTarget* resolveTo, int destSliceIndex = 0)=0;

	virtual bool		IsValid() const;

	virtual u32			GetRequiredMemory() const = 0;
	static s64			GetTotalMemory();
	static s64			GetTotalStereoMemory();

	virtual grcDevice::MSAAMode GetMSAA() const { return (grcDevice::MSAAMode)m_Multisample; }
	u32 GetMSAAQuality() const { return m_MultisampleQuality; }

	virtual ChannelBits FindUsedChannels() const;

	virtual void Update()=0;

#if RSG_PC || RSG_DURANGO
	virtual void	GenerateMipmaps() = 0;
#endif

protected:
	inline void			SetSlotId(s32 slotId);
	inline s32			GetSlotId();
	virtual void		DeviceLost() = 0;
	virtual void		DeviceReset() = 0;
	virtual void		SetPrivateData() = 0;

protected:
	virtual void		ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) = 0;
	virtual void		CreateSurface() = 0;
	virtual u32			GetDepthTextureFormat() = 0;

#if __D3D9
	const grcTextureObject*	m_ResolvedObject;
#elif DEBUG_TRACK_MSAA_RESOLVES
	// Track resolves to ensure they are not done redundantly.  Tracking
	// requires multi-threaded rendering to be disabled.
	const grcTextureObject*	m_DebugLastResolvedObject;
#endif

	grcDeviceTexture	*m_UNUSED_Texture;
	grcDeviceTexture	*m_LockableTexture;
	bool				m_Lockable;
	bool				m_LockableTexUpdated;
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
#endif
};

inline void	grcRenderTargetPC::SetSlotId(s32 slotId) { 
	// Change 256 & u8 below if you need to raise slotid
	FastAssert("Need to increase size of m_SLotId" && slotId < 768); // RAY increased the maximum count
	m_SlotId = (u16) slotId;
}

inline s32 grcRenderTargetPC::GetSlotId() {
	 return (s32) m_SlotId;
}

#if !__D3D9

inline const grcTextureObject* grcRenderTargetPC::DebugGetLastResolved() const
{
#if DEBUG_TRACK_MSAA_RESOLVES
	return m_DebugLastResolvedObject;
#else
	return NULL;
#endif
}

inline void grcRenderTargetPC::DebugSetUnresolved()
{
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedObject = NULL;
#endif
}

#endif // !__D3D9

#endif // RSG_PC

// DOM-IGNORE-END

}	// namespace rage

#endif	// RSG_PC || RSG_DURANGO

#endif // GRCORE_TEXTUREPC_H
