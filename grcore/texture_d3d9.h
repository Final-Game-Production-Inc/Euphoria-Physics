//
// grcore/texture_d3d9.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTUREDX9_H
#define GRCORE_TEXTUREDX9_H

#if	__WIN32PC

#include	"atl/array.h"
#include	"grcore/device.h"
#include	"grcore/resourcecache.h"
#include	"grcore/texture.h"
#include	"grcore/texturepc.h"
#include	"vector/vector3.h"

namespace rage {

// DOM-IGNORE-BEGIN
class grcRenderTargetPC;

class	grcTextureFactoryDX9 :	public	grcTextureFactoryPC
{
public:
	grcTextureFactoryDX9();
	~grcTextureFactoryDX9();
		
	virtual bool		SupportsFormat(grcTextureFormat);
	virtual u32			Translate(grcTextureFormat);
	virtual u32			GetD3DFormat(grcImage *);
	virtual u32			GetImageFormat(u32 eFormat) { return GetImageFormatStaticDX9(eFormat); }
	static u32			GetImageFormatStaticDX9(u32);
	virtual u32			GetBitsPerPixel(u32 uInternalFormat) { return GetBitsPerPixelStaticDX9(uInternalFormat); }
	static u32			GetBitsPerPixelStaticDX9(u32 uInternalFormat);

	grcTexture			*Create				(const char *pFilename,TextureCreateParams *params);
	grcTexture			*Create				(class grcImage*,TextureCreateParams *params);
	grcTexture			*Create				(u32 width, u32 height, u32 eFormat, void* pBuffer, u32 numMips, TextureCreateParams *params);
	grcRenderTarget		*CreateRenderTarget	(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));
	grcRenderTarget		*CreateRenderTarget	(const char *pName, const grcTextureObject *pTexture WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));
	void				LockRenderTarget	(int index, const grcRenderTarget *pColor, const grcRenderTarget *pDepth, u32 layer = 0,bool lockDepth = true, u32 D3D11_ONLY(mipToLock) = 0);
	void				UnlockRenderTarget	(int index,const grcResolveFlags *);
	void				LockMRT				(const grcRenderTarget *pColor[grcmrtColorCount], const grcRenderTarget *pDepth, const u32 *D3D11_ONLY(mipsToLock) = NULL);
	void				UnlockMRT			(const grcResolveFlagsMrt* resolveFlags = NULL);
	void				PlaceTexture		(class datResource &, grcTexture &);

	virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
	virtual       grcRenderTarget *GetBackBuffer(bool realize=true);
	virtual const grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const;
	virtual       grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer));
	virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
	virtual       grcRenderTarget *GetFrontBufferDepth(bool realize=true);
	virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
	virtual       grcRenderTarget *GetBackBufferDepth(bool realize=true);
	virtual void BindDefaultRenderTargets() {}

	virtual void Lost() { /* Displayf("Texture Factory - Lost"); */ }
	virtual void Reset() { /* Displayf("Texture Factory - Reset"); */ }

protected:
	static const int		MAX_RENDER_TARGETS = 4;
	bool					m_PreviousDepthLock;
	grcDeviceSurface		*m_PreviousTargets[MAX_RENDER_TARGETS];
	grcDeviceSurface		*m_PreviousDepth;
	grcDeviceSurface		*m_CurTexDepthSurface;
	const grcRenderTarget	*m_DepthTarget;
	s32						m_PreviousWidth;
	s32						m_PreviousHeight;
};

#if __PAGING
	#define PAGING_SWITCH(_if_PAGING_,_else_) _if_PAGING_
#else
	#define PAGING_SWITCH(_if_PAGING_,_else_) _else_
#endif

class grcTextureDX9 : public grcTexturePC
{
	friend class grcTextureFactoryPC;
public:
	static grcTexture*	GetPrivateData(const grcTextureObject *pTexObj);

protected:
	virtual void		DeviceLost();
	virtual void		DeviceReset();
	virtual void		CreateFromBackingStore(bool bRecreate = false);
	virtual void		SetPrivateData();

	grcDevice::Result	Init(const char *pFilename,class grcImage*,grcTextureFactory::TextureCreateParams *params);

public:
	grcTextureDX9(const char *pFilename,grcTextureFactory::TextureCreateParams *params);
	grcTextureDX9(class grcImage *pImage,grcTextureFactory::TextureCreateParams *params);
	grcTextureDX9(u32 width, u32 height, grcTextureFormat eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params);
	grcTextureDX9(class datResource &rsc);
	virtual ~grcTextureDX9	(void);

	virtual u32			GetImageFormat() const { return grcTextureFactoryDX9::GetImageFormatStaticDX9(m_nFormat); } // cast this to grcImage::Format
	virtual int			GetBitsPerPixel() const { return grcTextureFactoryDX9::GetBitsPerPixelStaticDX9(m_nFormat); }

	static bool			LockRectStaticDX9	(grcDeviceTexture* pTexture, void* pBackingStore, u8 imageType, int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags);
	static void			UnlockRectStaticDX9	(grcDeviceTexture* pTexture, void* pBackingStore, u8 imageType, const grcTextureLock &lock);

	virtual bool		LockRect	(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const { return LockRectStaticDX9(m_CachedTexturePtr, PAGING_SWITCH(m_BackingStore, NULL), m_ImageType, layer, mipLevel, lock, uLockFlags); }
	virtual void		UnlockRect	(const grcTextureLock &lock) const { UnlockRectStaticDX9(m_CachedTexturePtr, PAGING_SWITCH(m_BackingStore, NULL), m_ImageType, lock); }

	virtual bool		Copy(const grcImage*);
	virtual bool		Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
	virtual bool		Copy(const void * pvSrc, u32 uWidth, u32 uHeight, u32 uDepth);
	virtual bool		CopyTo(grcImage* pImage, bool bInvert = false);
	virtual bool		Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) { return false; }

	virtual bool		IsValid() const;

	virtual ChannelBits	FindUsedChannels() const;

#if __DECLARESTRUCT
	void				DeclareStruct(class datTypeStruct &s);
#endif
};


class grcRenderTargetDX9 : public grcRenderTargetPC
{
	friend class grcTextureFactoryDX9;

public:
	grcRenderTargetDX9(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params);
	grcRenderTargetDX9(const char *name, const grcTextureObject *pTexture);
	virtual	~grcRenderTargetDX9	(void);

	const grcTextureObject	*GetTexturePtr	(void) const;
	grcTextureObject		*GetTexturePtr	(void);
	const grcDeviceSurface	*GetSurfacePtr	(void) const;
	grcDeviceSurface		*GetSurfacePtr	(void);
	const grcDeviceSurface	*GetOffscreenSurfacePtr	(void) const;
	grcDeviceSurface		*GetOffscreenSurfacePtr	(void);

	virtual bool		LockRect(int layer, int mipLevel, grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;// { return grcTextureDX9::LockRectStaticDX9(m_CachedTexturePtr, NULL, 0, layer, mipLevel, lock, uLockFlags); }
	virtual void		UnlockRect(const grcTextureLock &lock) const;// { grcTextureDX9::UnlockRectStaticDX9(m_CachedTexturePtr, NULL, 0, lock); }

	virtual bool		Copy(const grcTexture* pSource, s32 dstSliceIndex, s32 dstMipIndex, s32 srcSliceIndex, s32 srcMipIndex);
	virtual bool		CopyTo(grcImage* pImage, bool bInvert = false, u32 uPixelOffset = 0);
	virtual bool		Copy2D(const void * /*pSrc*/, u32 /*imgFormat*/, u32 /*uWidth*/, u32 /*uHeight*/, u32 /*numMips*/) { return false; }

	virtual void		Resolve(grcRenderTarget* resolveTo, int destSliceIndex = 0);

	virtual bool		IsValid() const;

	virtual u32			GetRequiredMemory() const;

	virtual void Update() {}

	virtual void	GenerateMipmaps() {}

protected:
	virtual void		DeviceLost();
	virtual void		DeviceReset();
	virtual void		SetPrivateData();

	virtual void		ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params);
	virtual void		CreateSurface();
	virtual u32			GetDepthTextureFormat();

	grcDeviceSurface	*m_Surface;
	grcDeviceSurface	*m_OffscreenSurface;
	const grcTextureObject	*m_ResolvedTarget;
};

// DOM-IGNORE-END

}	// namespace rage

#endif

#endif
