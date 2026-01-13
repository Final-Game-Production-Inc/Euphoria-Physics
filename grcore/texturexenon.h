//
// grcore/texturexenon.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTUREXENON_H
#define GRCORE_TEXTUREXENON_H

#if	__XENON | __WIN32PC

#include	"math/amath.h"
#include	"device.h"
#include	"grcore/texture.h"
#include	"texturexenonproxy.h"

struct D3DBaseTexture;

namespace rage {

class grcRenderTargetXenon;
class grcTextureXenon;

class grcTextureFactoryXenon : public grcTextureFactory
{
	public:
		grcTextureFactoryXenon();
		~grcTextureFactoryXenon();
		
		grcTexture *		Create				(const char *pFilename,TextureCreateParams *params);
		grcTexture *		Create				(class grcImage *pImage,TextureCreateParams *params);
		grcTexture *		Create				(u32 width, u32 height, u32 format, void* pBuffer, u32 mipLevels = 1, TextureCreateParams * params = NULL);
		
		grcRenderTarget	*	CreateRenderTarget	(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));
		grcRenderTarget	*	CreateRenderTarget	(const char *pName, const grcTextureObject *pTexture WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));

		void				LockRenderTarget	(int index,const grcRenderTarget *pColor, const grcRenderTarget *pDepth, u32 layer = 0,bool lockDepth = true, u32 D3D11_OR_ORBIS_ONLY(mipToLock) = 0);
		void				UnlockRenderTarget	(int index,const grcResolveFlags *);
		void				LockMRT				(const grcRenderTarget *pColor[grcmrtColorCount], const grcRenderTarget *pDepth, const u32 *D3D11_ONLY(mipsToLock) = NULL );
		void				UnlockMRT			(const grcResolveFlagsMrt* resolveFlags = NULL);
		void				PlaceTexture		(class datResource &, grcTexture &);
		virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
		virtual grcRenderTarget *GetBackBuffer(bool realize=true);
		virtual const grcRenderTarget *GetFrontBuffer(bool nextBuffer=false) const;
		virtual grcRenderTarget *GetFrontBuffer(bool nextBuffer=false);
		virtual const grcRenderTarget *GetFrontBufferFromIndex(int index) const;
		virtual grcRenderTarget *GetFrontBufferFromIndex(int index);
		virtual int	GetCurrentFrontBufferIndex();
		virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
		virtual grcRenderTarget *GetFrontBufferDepth(bool realize=true);
		virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
		virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);
		virtual const grcRenderTarget *GetBackBufferDepthForceRealize();
		virtual bool CanGetBackBufferDepth();
		virtual bool CanGetBackBuffer();
		virtual void BindDefaultRenderTargets() {}

		virtual u32 GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory);

	protected:
		static const int	MAX_RENDER_TARGETS = 4;
		bool				m_PreviousDepthLock;
		grcDeviceSurface	*m_PreviousTargets[MAX_RENDER_TARGETS];
		grcRenderTarget		*m_CurrentTargets[MAX_RENDER_TARGETS];
		grcDeviceSurface	*m_PreviousDepth;
		s32					m_PreviousWidth;
		s32					m_PreviousHeight;
		grcRenderTarget		*m_CurrentTarget;
		grcRenderTarget		*m_CurrentDepth;
		grcRenderTargetXenon *m_BackBuffer;

#if HACK_GTA4
		grcRenderTargetXenon *m_DepthBuffers[3];
		grcRenderTargetXenon *m_FrontBuffers[3];
#else
		grcRenderTargetXenon *m_DepthBuffers[2];
		grcRenderTargetXenon *m_FrontBuffers[2];
#endif
};



class grcRenderTargetXenon : public grcRenderTarget
{
	friend class grcTextureFactoryXenon;
public:
	grcRenderTargetXenon( const char *name);
	grcRenderTargetXenon( const char *name,grcRenderTargetType type,int width,int height,int bpp, grcTextureFactory::CreateParams *params);
	grcRenderTargetXenon( const char *pName, const grcTextureObject *pTexture);
	~grcRenderTargetXenon();

	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int GetDepth() const { return 1; }

	int GetArraySize() const { return 0; }

	virtual void AllocateMemoryFromPool();
	virtual void ReleaseMemoryToPool();

	virtual void UpdateMemoryLocation(const grcTextureObject *pTexture);
	
	bool LockRect		(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	void UnlockRect		(const grcTextureLock &lock) const;

	const grcTextureObject *GetTexturePtr() const;
	grcTextureObject *GetTexturePtr();


	void Realize(const grcResolveFlags *,int index);

	void Untile(int level);

	// supports mip mapping of generated textures
	void CreateMipMaps( const grcResolveFlags * , int /*index*/);
	void Blur( const grcResolveFlags*);

	int GetMipMapCount() const;
	int GetBitsPerPixel() const;
	bool IsGammaEnabled() const;
	void SetGammaEnabled(bool enabled);
	u32 GetTextureSignedMask() const;
	void SetTextureSignedMask(u32);

	void SetColorExpBias(int val);
	int  GetColorExpBias() const;

	const grcTextureFormat GetFormat() const {return m_Format;}

	grcDeviceSurface* GetSurface(bool bind = false) const;

	ChannelBits FindUsedChannels() const;

	const void* GetCurrentPoolMemPtr() const { return m_CurrentPoolMemPtr; }

	void SetExpAdjust(int iExpAdjust);

	// HiZ memory size of surface in HiZ tile units
	u16 GetHiZSize() const;
	bool SetupAsFakeStencilTexture(grcRenderTarget *pDepthRT);

#if HACK_GTA4
	inline void LockSurface(u32 layer, u32 mip = 0) const
	{
		FastAssert(layer < GetLayerCount());
		FastAssert(mip < (u32)GetMipMapCount());
		m_LockedLayer = layer;
		m_LockedMip = mip;
	}
#endif

private:
	u16 m_Width, m_Height;

	grcTextureFormat m_Format;

	void *m_Bits;
	grcDeviceSurface *m_Surface;
	mutable u32 m_LockedLayer;
	mutable u32 m_LockedMip;

	grcRenderTargetXenon(grcDeviceTexture *baseTex);
	grcRenderTargetXenon();
	
	void InitWithDepthSurface(grcDeviceSurface *surface, u32* m_PoolAllocSize = NULL);
	void InitWithSurface(grcDeviceSurface *surface);
#if !HACK_GTA4
	inline void LockSurface(u32 layer, u32 mip = 0) const
	{
		FastAssert(layer < GetLayerCount());
		FastAssert(mip < (u32)GetMipMapCount());
		m_LockedLayer = layer;
		m_LockedMip = mip;
	}
#endif

	D3DBaseTexture *m_Texture;

	// render target memory pool data specific to XENON
	u32			m_PoolAllocSize;		// how much memory this target needs from the render target pool
	u32			m_MipAddrOffset;		// the offset of the mip from the base texture (needed when the target moves)
	void		*m_CurrentPoolMemPtr;	// current pointer in the rt pool, can change each lock/unlock

};

class grcTextureXenon : public grcTextureXenonProxy
{
	friend grcTextureFactoryXenon;	
public:
				grcTextureXenon	();
				grcTextureXenon	(const char *pFilename,grcTextureFactory::TextureCreateParams *params);
				grcTextureXenon	(class grcImage *pImage,grcTextureFactory::TextureCreateParams *params);
				grcTextureXenon	(datResource&);
				grcTextureXenon	(u32 width , u32 height, u32 mipLevels, grcDeviceTexture* baseTex);

				~grcTextureXenon(void);
	void		Bind			(int stage) const;

	const grcTextureObject *GetTexturePtr	(void) const;
	grcTextureObject *GetTexturePtr	(void);

	virtual u32 GetImageFormat() const; // cast this to grcImage::Format

	virtual void SetTextureSwizzle(eTextureSwizzle  r, eTextureSwizzle  g, eTextureSwizzle  b, eTextureSwizzle  a, bool bApplyToExistingSwizzle);
	virtual void GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const;

	int			GetWidth		(void) const { return(m_Width); }
	void		SetWidth		(u16 w) { m_Width = w; }
	int			GetHeight		(void) const { return(m_Height); }
	void		SetHeight		(u16 h) { m_Height = h; }
	int			GetDepth		(void) const { return 1; }
	int			GetBitsPerPixel() const;
	int			GetMipMapCount() const { return m_MipCount; }
	void		SetMipMapCount	(int m) { m_MipCount = m; }
	bool		IsGammaEnabled() const;
	void		SetGammaEnabled(bool enabled);
	u32			GetTextureSignedMask() const;
	void		SetTextureSignedMask(u32);
	bool		LockRect		(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags = (grcsRead | grcsWrite)) const;
	void		UnlockRect		(const grcTextureLock &lock) const;
	bool		Copy(const grcImage*);
	bool		Copy2D(const void* pSrc, u32 imgFormat, u32 uWidth, u32 uHeight, u32 numMips);

	void		Resize(u32 width, u32 height);

	ChannelBits FindUsedChannels() const;

	void		Tile(int level);

	static u32  GetInternalFormat(u32 imgFormat, bool bIsLinear);

private:
	void		Init			(const char *pFilename,grcImage *pImage,grcTextureFactory::TextureCreateParams *params);
};

}	// namespace rage

#endif

#endif
