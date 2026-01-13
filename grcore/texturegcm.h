// 
// grcore/texturegcm.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_TEXTUREGCM_H
#define GRCORE_TEXTUREGCM_H

#include "grcore/config.h"

#if __GCM || __RESOURCECOMPILER

#include "atl/bitset.h"
#include "texture.h"
#include "wrapper_gcm.h"
#include "device.h"

enum
{
	CELL_GCM_TEXTURE_GAMMA_RGB  = CELL_GCM_TEXTURE_GAMMA_R | CELL_GCM_TEXTURE_GAMMA_G | CELL_GCM_TEXTURE_GAMMA_B,
	CELL_GCM_TEXTURE_GAMMA_MASK = CELL_GCM_TEXTURE_GAMMA_A | CELL_GCM_TEXTURE_GAMMA_RGB,
};

namespace rage {

	class grcImage;
	class grcRenderTargetGCM;
	class datResource;

	class grcTextureFactoryGCM: public grcTextureFactory {
	public:
		grcTextureFactoryGCM();
		~grcTextureFactoryGCM();

		virtual grcTexture *Create(const char *filename,TextureCreateParams *params);
		virtual grcTexture *Create(grcImage *image,TextureCreateParams *params);
		virtual grcTexture* Create(u32 width, u32 height, u32 format, void* pBuffer, u32 numMips = 1, TextureCreateParams * params = NULL);
		virtual grcRenderTarget *CreateRenderTarget(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));
		virtual grcRenderTarget *CreateRenderTarget(const char *pName, const grcTextureObject *pTexture WIN32PC_ONLY(, grcRenderTarget* originalTarget = NULL));
		virtual void LockRenderTarget	(int index,const grcRenderTarget *pColor, const grcRenderTarget *pDepth, u32 layer = 0, bool lockDepth = true, u32 D3D11_OR_ORBIS_ONLY(mipToLock) = 0);
		virtual void UnlockRenderTarget	(int index,const grcResolveFlags *);

		virtual void LockMRT(const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock) = NULL);
		virtual void UnlockMRT(const grcResolveFlagsMrt* resolveFlags = NULL);

		virtual void PlaceTexture		(datResource &, grcTexture &);
		virtual grcRenderTarget *GetBackBuffer(bool realize=true);
		virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
		virtual grcRenderTarget *GetFrontBuffer(bool nextBuffer=false);
		virtual const grcRenderTarget *GetFrontBuffer(bool nextBuffer=false) const;
		virtual grcRenderTarget *GetFrontBufferDepth(bool realize=true);
		virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
		virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);
		virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
		
		virtual u32 GetTextureDataSize(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool bIsCubeMap, bool bIsLinear, bool bLocalMemory);

		void SetBufferDepth(grcRenderTargetGCM *depthBuffer);
		void SetBackBufferDepth(grcRenderTargetGCM *depthBuffer);
		
		static grcTextureFactory *CreatePagedTextureFactory(bool bMakeActive = true);

		void BindDefaultRenderTargets();
	private:
		
		// this is used to store pointers to render targets in-between frames
		// we need this mainly for the LockRenderTarget() / UnlockRenderTarget() pair of functions
		const grcRenderTarget* 	m_LockColorRenderTargets[grcmrtColorCount];
		const grcRenderTarget* 	m_LockDepthRenderTarget;
		u16 					m_FrontBufferMemoryPoolID;
	};


	class grcTextureGCM: public grcTexture {
	public:
		grcTextureGCM(const char *filename,grcTextureFactory::TextureCreateParams *params);
		grcTextureGCM(grcImage *image,grcTextureFactory::TextureCreateParams *params);
		grcTextureGCM(u32 width,u32 height,u32 format,void *pBuffer, u32 numMips, grcTextureFactory::TextureCreateParams *params);

		grcTextureGCM(class datResource&);
		~grcTextureGCM();

		virtual int GetWidth() const { return m_Texture.width; }
		virtual int GetHeight() const { return m_Texture.height; }
		virtual int GetDepth() const { return m_Texture.depth; }
		virtual int GetMipMapCount() const { return m_Texture.mipmap; }
		virtual int GetArraySize() const { return 0; }
		virtual int GetBitsPerPixel() const;
		u32 GetLinesPerPitch() const;
		virtual bool IsGammaEnabled() const { return (m_Texture._padding & CELL_GCM_TEXTURE_GAMMA_RGB) == CELL_GCM_TEXTURE_GAMMA_RGB; }
		virtual void SetGammaEnabled(bool enabled)
		{
			if (enabled && gcm::TextureFormatSupportsSrgb(m_Texture.format))
				m_Texture._padding |= CELL_GCM_TEXTURE_GAMMA_RGB;
			else
				m_Texture._padding &= ~CELL_GCM_TEXTURE_GAMMA_RGB;
		}
		virtual u32 GetTextureSignedMask() const { return m_Texture._padding >> 4; }
		void SetTextureSignedMask(u32 mask) { m_Texture._padding = u8((m_Texture._padding & 15) | (mask << 4)); }
		virtual const grcTextureObject *GetTexturePtr() const { return reinterpret_cast<const grcTextureObject*>(&m_Texture); }
		virtual grcTextureObject *GetTexturePtr() { return reinterpret_cast<grcTextureObject*>(&m_Texture); }

		virtual u32 GetImageFormat() const; // cast this to grcImage::Format

		virtual bool LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 uLockFlags = (grcsRead | grcsWrite)) const;
		virtual void UnlockRect(const grcTextureLock &/*lock*/) const;
		bool Copy(const grcImage *image);
		bool Copy2D(const void* pSrc, u32 imgFormat, u32 uWidth, u32 uHeight, u32 numMips);

		//MIPS AND MULTIPLE LAYERS ARE CURRENTLY NOT SUPPORTED
		void Resize(u32 width, u32 height);

#if RESOURCE_HEADER
		inline void* GetMemoryOffset() const { return m_MemoryOffsets; }
#endif
		inline u32 GetMemoryOffset(u32 layer, u32 mip)
		{
			FastAssert(layer < GetLayerCount());
			FastAssert(mip < m_Texture.mipmap);
			FastAssert(m_MemoryOffsets);
			return m_MemoryOffsets[mip * GetLayerCount() + layer];
		}

		inline void SetMemoryOffset(u32 layer, u32 mip, u32 newOffset)
		{
			FastAssert(layer < GetLayerCount());
			FastAssert(mip < m_Texture.mipmap);
			FastAssert(m_MemoryOffsets);
			m_MemoryOffsets[mip * GetLayerCount() + layer] = newOffset;
		}

		void OverrideMipMapCount(u32 newMipCount) { m_Texture.mipmap = (u8)newMipCount; }

		// RETURNS: a raw pointer to the pixel data
		void* GetBits();

		// Removes the top mip
		void RemoveTopMip();

		virtual void SetTextureSwizzle(eTextureSwizzle  r, eTextureSwizzle  g, eTextureSwizzle  b, eTextureSwizzle  a, bool bApplyToExistingSwizzle);
		virtual void GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const;

#if __DECLARESTRUCT && !__PPU
		void DeclareStruct(datTypeStruct &s);
#endif

		virtual ChannelBits FindUsedChannels() const;

	private:
		void RecalculatePitch();
		void Init(const char *filename,grcImage *image /*may be NULL*/,const grcTextureFactory::TextureCreateParams* params);
		void Init(const char *filename,const grcTextureFactory::TextureCreateParams* params);
		void Init(u32 width, u32 height, u32 format, void* pBuffer, u32 mipLevels, grcTextureFactory::TextureCreateParams * params);

		u32*			m_MemoryOffsets;
	};

	class grcRenderTargetGCM : public grcRenderTarget 
	{
		friend class grcTextureFactoryGCM;
		friend class grcDevice;	
		friend class grcRenderTargetPoolEntry;

		grcRenderTargetGCM(const char *name,
			grcRenderTargetType type,
			int width,
			int height,
			int bitsPerPixel,
			grcTextureFactory::CreateParams *params);

		grcRenderTargetGCM(const char *name, const grcTextureObject *pTexture);

		~grcRenderTargetGCM();

	public:
		virtual void AllocateMemoryFromPool();
		virtual void ReleaseMemoryToPool();

		virtual void UpdateMemoryLocation(const grcTextureObject *pTexture);

		virtual int GetWidth() const { return m_Texture.width; }
		virtual int GetHeight() const { return m_Texture.height; }
		virtual int GetDepth() const { return m_Texture.depth; }
		virtual int GetMipMapCount() const { return m_Texture.mipmap; }
		virtual int GetArraySize() const { return 0; }
		virtual int GetBitsPerPixel() const;
		virtual bool IsGammaEnabled() const { return (m_Texture._padding & CELL_GCM_TEXTURE_GAMMA_RGB) == CELL_GCM_TEXTURE_GAMMA_RGB; }
		virtual void SetGammaEnabled(bool enabled)
		{
			if (enabled && gcm::TextureFormatSupportsSrgb(m_Texture.format))
				m_Texture._padding |= CELL_GCM_TEXTURE_GAMMA_RGB;
			else
				m_Texture._padding &= ~CELL_GCM_TEXTURE_GAMMA_RGB;
		}
		virtual grcTextureObject *GetTexturePtr() { return reinterpret_cast<grcTextureObject*>(&m_Texture); }
		virtual const grcTextureObject *GetTexturePtr() const { return reinterpret_cast<const grcTextureObject*>(&m_Texture); }
		virtual u32 GetTextureSignedMask() const { return m_Texture._padding >> 4; }
		void SetTextureSignedMask(u32 mask) { m_Texture._padding = u8((m_Texture._padding & 15) | (mask << 4)); }

		virtual const grcRenderTarget *GetAABufferPtr() const { return m_AABuffer; }
		virtual grcRenderTarget *GetAABufferPtr() { return m_AABuffer; }
		virtual u8 GetSurfaceFormat() const { return m_SurfaceFormat; }
		virtual void  SetSurfaceFormat(u8 f) { m_SurfaceFormat=f; }
		virtual u8 GetTextureFormat() const  { return (u8)gcm::StripTextureFormat(((CellGcmTexture*)this->GetTexturePtr())->format); }
		virtual bool IsTiled() const { return m_BooleanTraits.IsSet(grcrttInTiledMemory); }
		virtual u8 GetTiledIndex() const { return m_TiledIndex; }
		virtual bool IsSwizzled() const { return m_BooleanTraits.IsSet(grcrttIsSwizzled); }
		virtual bool IsInLocalMemory() const { return m_BooleanTraits.IsSet(grcrttInLocalMemory); }
		virtual bool IsInMemoryPool() const { return m_BooleanTraits.IsSet(grcrttInMemoryPool); }
#if HACK_GTA4
		virtual bool IsUsingZCull() const { return m_BooleanTraits.IsSet(grcrttUseZCulling); }
#endif // HACK_GTA4

		void CreateMipMaps(const grcResolveFlags* resolveFlags, int index);
		static void CreateMipMaps(grcRenderTarget** mipMaps, u32 mipMapCount, int index);
		void Blur(const grcResolveFlags* resolveFlags);
		// Lock a particular face and mip map
		using grcRenderTarget::Lock; // Don't hide virtual base class member function

		inline u32 GetLockedLayer() const { return m_LockedLayer; }
		inline u32 GetLockedMip() const { return m_LockedMip; }
		inline u32 GetMemoryOffset() const { return m_MemoryOffset; }
		u8 GetBitsPerPix() const { return m_BitsPerPix; }
		u32 GetPitch() const { return m_Texture.pitch; }
		void OverridePitch(u32 newPitch) { m_Texture.pitch = newPitch; }
		void OverrideTextureOffsets(u32 baseOffset, u16 newWidth, u16 newHeight);

		virtual bool LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 uLockFlags = (grcsRead | grcsWrite)) const;

		static const atFixedBitSet<15>& GetBitFieldTiledMemory() {return sm_BitFieldTiledMemory;}

		// RETURNS: a Prepatched to 32bitARGB, ready to use depth buffer, to avoid having to patch/unpatch during a frame.
		grcRenderTargetGCM *CreatePrePatchedTarget(bool patchSurface);

		grcRenderTargetGCM *DuplicateTarget(const char *name);

		virtual ChannelBits FindUsedChannels() const;
#if HACK_GTA4
		inline void LockSurface(u32 layer, u32 mip = 0) const;
#endif // HACK_GTA4

#if RSG_PC
		virtual void	GenerateMipmaps() {}
#endif

		// Force this render target to share the zcull region of an existing render target. Only call this if you really know what you're doing.
		bool ForceZCullSharing( const grcRenderTargetGCM* pZCullHostingTarget );

	protected:
		// tiled memory management
		static atFixedBitSet<15> sm_BitFieldTiledMemory;

		// z culling management
		struct zCullRecord {
				zCullRecord(){};
				zCullRecord(const grcRenderTargetGCM* pTarget);
				u32 offset;
				u32 width;
				u32 height;
				u32 zFormat;
				u32 aaFormat;
				u32 zcullDir;
				u32 zcullFormat;
				u32 sFunc;
				u32 sRef;
				u32 sMask;
				u32 zCullStart;
				bool operator==(zCullRecord a) const;
		 };
		 
		typedef struct zCullRecord zCullRecord;

		// Search for an existing zcull record that matches this one
		bool HasMatchingZCullRecord(const zCullRecord& cullRecord, bool enableLogging=true) { return 0 <= FindMatchingZCullRecord(cullRecord,enableLogging); }

		// Search for an existing zcull record that matches this one. Returns the record index if it finds one, or -1 if it doesn't.
		int FindMatchingZCullRecord(const zCullRecord& cullRecord, bool enableLogging=true);
		
		static zCullRecord sm_ZCullRegionsRecord[8];
		static atFixedBitSet<8> sm_BitFieldZCulling;
		
		static u32			sm_TiledMemorySize;
		static u32			sm_ZCullStart;

		static u32			sm_ColorBank;
#if !HACK_GTA4
		static u32			sm_DepthBank;
#endif // !HACK_GTA4
		static u32			sm_CompressionTag;

	private:

		grcRenderTargetGCM(grcRenderTargetGCM *rtGCM);
	
		void AllocateNonPooled();
		void AllocateInPlace(u32 baseOffset);
		void AllocateFromPool();
		void ReleaseFromPool();

		void FreeRenderTarget();

		void ValidatePoolAllocation();
		bool ValidateTiledMemory(u32 memorySize = 0);
		bool ValidateZCulling(bool validateAlignment = true);

		// moves a texture into tiled memory
		u8 MoveIntoTiledMemory(u32 memorySize, bool enableCompression);

		// attach Z culling support to a render target
		void AttachZCulling();

#if !HACK_GTA4
		inline void LockSurface(u32 layer, u32 mip = 0) const;
#endif // !HACK_GTA4

		grcRenderTarget *m_AABuffer;

		// the MSAA technique
		grcDevice::MSAAMode m_MSAA;

		u32*				m_MemoryOffsets;
		u32*				m_MemoryBaseOffsets;	// need the originals offset, for when we move around...
		mutable u32			m_MemoryOffset;
		mutable u32			m_LockedLayer;
		mutable u32			m_LockedMip;

		enum Traits
		{
			grcrttInTiledMemory,
			grcrttInLocalMemory,
			grcrttUseZCulling,
			grcrttIsSwizzled,
			grcrttInMemoryPool,
			grcrttUseTextureMemory,
			grcrttCustomZCullAllocation,
			
			grcrttCount
		};
		atFixedBitSet<grcrttCount> m_BooleanTraits;

		u8		m_SurfaceFormat;
		u8		m_TiledIndex;
		u8		m_Format;
		u8		m_BitsPerPix;
	};

		inline void grcRenderTargetGCM::LockSurface(u32 layer, u32 mip) const
			{
			FastAssert(layer < GetLayerCount());
			FastAssert(mip < (u32)GetMipMapCount());
			FastAssert(m_MemoryOffsets);
			m_MemoryOffset = m_MemoryOffsets[mip * GetLayerCount() + layer];
			m_LockedLayer = layer;
			m_LockedMip = mip;
		}
}	// namespace rage

#endif		// __GCM

#endif
