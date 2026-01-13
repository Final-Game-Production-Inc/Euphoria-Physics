// 
// grcore/rendertarget_gnm.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_RENDERTARGET_GNM_H
#define GRCORE_RENDERTARGET_GNM_H

#include "grcore/config.h"
#include "grcore/effect_mrt_config.h"
#include "grcore/setup.h"

#if __GNM

#include "atl/bitset.h"
#include "rendertarget_common.h"
#include "texture.h"
#include "wrapper_gcm.h"
#include "device.h"

#include <gnm.h>

#define GNM_RENDERTARGET_DUMP	(SUPPORT_RENDERTARGET_DUMP || __DEV)

namespace sce { namespace Gnm { class Texture; class RenderTarget; class DepthRenderTarget; } }

namespace rage {

	class grcImage;
	class grcRenderTargetGNM;
	class grcTextureGNM;
	class datResource;

	struct CoverageData
	{
		CoverageData();
		bool isMultisampled;
		const grcTextureGNM	*texture;
		grcRenderTargetGNM	*donor;
		ResolveType resolveType;
		sce::Gnm::NumSamples	superSample;
		bool isCmaskDirty;
	};

#if GNM_RENDERTARGET_DUMP
	enum DumpMask	{
		DUMP_COLOR	= 1<<0,
		DUMP_CMASK	= 1<<1,
		DUMP_FMASK	= 1<<2,
		DUMP_DEPTH	= 1<<4,
		DUMP_STENCIL= 1<<5,
		DUMP_HTILE	= 1<<6,
		DUMP_META	= DUMP_CMASK | DUMP_FMASK | DUMP_HTILE,
		DUMP_COLOR_ALL	= DUMP_COLOR | DUMP_CMASK | DUMP_FMASK,
		DUMP_DEPTH_ALL	= DUMP_DEPTH | DUMP_STENCIL | DUMP_HTILE,
		DUMP_ALL	= DUMP_COLOR_ALL | DUMP_DEPTH_ALL,
	};
#endif	// GNM_RENDERTARGET_DUMP

	class grcRenderTargetGNM : public grcRenderTarget 
	{
		friend class grcTextureFactoryGNM;
		friend class grcDevice;	
		friend class grcRenderTargetPoolEntry;

		grcRenderTargetGNM(const char *name,
			grcRenderTargetType type,
			int width,
			int height,
			int bitsPerPixel,
			grcTextureFactory::CreateParams *params,
			sce::Gnm::Texture* pOrigTex);

		grcRenderTargetGNM(const char *name, const grcRenderTargetGNM *pTexture, grcTextureFormat eShaderViewFormat = grctfNone);
		grcRenderTargetGNM(const char *name, const grcTextureObject *pTexture, grcRenderTargetType type);
		grcRenderTargetGNM(const char *name, sce::Gnm::RenderTarget *color, sce::Gnm::DepthRenderTarget *depth, sce::Gnm::DepthRenderTarget *stencil);
		grcRenderTargetGNM();

		~grcRenderTargetGNM();

		void SetName(const char *name);

	public:
		static void InitMipShader();

		bool LockMipLevel(int mipLevel);

		virtual void AllocateMemoryFromPool();
		virtual void ReleaseMemoryToPool();

		virtual void UpdateMemoryLocation(const grcTextureObject *pTexture);

		virtual int GetWidth() const { return m_GnmTexture.getWidth(); }
		virtual int GetHeight() const { return m_GnmTexture.getHeight(); }
		virtual int GetDepth() const { return m_GnmTexture.getDepth(); }
		virtual int GetMipMapCount() const { return m_GnmTexture.getLastMipLevel() + 1; }
		virtual int GetArraySize() const { return m_GnmTexture.getLastArraySliceIndex() + 1; }
		virtual int GetBitsPerPixel() const;
		virtual bool IsGammaEnabled() const;
		virtual void SetGammaEnabled(bool enabled);
		virtual grcTextureObject *GetTexturePtr() { return &m_GnmTexture; }
		virtual const grcTextureObject *GetTexturePtr() const { return &m_GnmTexture; }
		virtual u32 GetTextureSignedMask() const { return m_Texture._padding >> 4; }
		void SetTextureSignedMask(u32 mask) { m_Texture._padding = u8((m_Texture._padding & 15) | (mask << 4)); }

		static u32 TranslateImageFormat(const sce::Gnm::DataFormat &dataFormat);
		virtual u32	GetImageFormat() const;

		const sce::Gnm::RenderTarget *GetColorTarget()		const { return m_Target; }
		const sce::Gnm::DepthRenderTarget *GetDepthTarget()	const { return m_DepthTarget; }

		const void SetColorTargetArrayView(u32 uArrayIndex = 0) const;
		const void SetDepthTargetArrayView(u32 uArrayIndex = 0) const;

		virtual void GenerateMipmaps();

		virtual grcDevice::MSAAMode GetMSAA() const;

		// virtual u8 GetSurfaceFormat() const { return m_SurfaceFormat; }
		// virtual void  SetSurfaceFormat(u8 f) { m_SurfaceFormat=f; }
		// virtual u8 GetTextureFormat() const  { return (u8)gcm::StripTextureFormat(((CellGcmTexture*)this->GetTexturePtr())->format); }
		// virtual bool IsTiled() const { return m_BooleanTraits.IsSet(grcrttInTiledMemory); }
		// virtual u8 GetTiledIndex() const { return m_TiledIndex; }
		// virtual bool IsSwizzled() const { return m_BooleanTraits.IsSet(grcrttIsSwizzled); }
		// virtual bool IsInLocalMemory() const { return m_BooleanTraits.IsSet(grcrttInLocalMemory); }
		// virtual bool IsInMemoryPool() const { return m_BooleanTraits.IsSet(grcrttInMemoryPool); }
#if HACK_GTA4
		// virtual bool IsUsingZCull() const { return m_BooleanTraits.IsSet(grcrttUseZCulling); }
#endif // HACK_GTA4

		void CreateMipMaps(const grcResolveFlags* resolveFlags, int index);
		static void CreateMipMaps(grcRenderTarget** mipMaps, u32 mipMapCount, int index);
		void Blur(const grcResolveFlags* resolveFlags);
		// Lock a particular face and mip map
		using grcRenderTarget::Lock; // Don't hide virtual base class member function

		/* inline u32 GetLockedLayer() const { return m_LockedLayer; }
		inline u32 GetLockedMip() const { return m_LockedMip; }
		inline u32 GetMemoryOffset() const { return m_MemoryOffset; }
		u8 GetBitsPerPix() const { return m_BitsPerPix; }
		u32 GetPitch() const { return m_Texture.pitch; } */

		virtual bool LockRect(int /*layer*/, int /*mipLevel*/,grcTextureLock &/*lock*/, u32 uLockFlags = (grcsRead | grcsWrite)) const;

		virtual bool Copy(const grcTexture *pTexture, s32 dstSliceIndex = -1, s32 dstMipIndex = -1, s32 srcSliceIndex = 0, s32 srcMipIndex = 0);

		// static const atFixedBitSet<15>& GetBitFieldTiledMemory() {return sm_BitFieldTiledMemory;}

		// RETURNS: a Prepatched to 32bitARGB, ready to use depth buffer, to avoid having to patch/unpatch during a frame.
		grcRenderTargetGNM *CreatePrePatchedTarget(bool patchSurface);

		grcRenderTargetGNM *DuplicateTarget(const char *name);

		virtual ChannelBits FindUsedChannels() const;
#if HACK_GTA4
		inline void LockSurface(u32 layer, u32 mip = 0) const;
#endif // HACK_GTA4

		const sce::Gnm::Texture	*GetUnorderedAccessView()	const { Assert(m_GnmTexture.isTexture()); return &m_GnmTexture; }

		inline const sce::Gnm::RenderTarget* DebugGetLastResolved() const;
		inline void DebugSetUnresolved();
#if DEBUG_TRACK_MSAA_RESOLVES
		bool HasBeenResolvedTo(grcRenderTarget* resolveTo);
#endif
		void Resolve(grcRenderTarget* resolveTo, int destSliceIndex = 0, uint debugMask = 0);
		inline sce::Gnm::RenderTarget* GetResolveTarget()	{ Assert(m_Target); return m_Target; }

		const CoverageData& GetCoverageData() const	{ return m_Coverage; }
		void SetFragmentMaskDonor(grcRenderTargetGNM *donor, ResolveType resolveType);
		void SetSampleLocations() const;
		const grcTextureGNM* GetResolveFragmentMask() const	{ return (m_Coverage.donor ? m_Coverage.donor : this )->GetCoverageData().texture; }

		void PushCmaskDirty()	{ m_Coverage.isCmaskDirty=true; }
		bool PopCmaskDirty()	{ bool r=m_Coverage.isCmaskDirty;  m_Coverage.isCmaskDirty=false; return r; }

#if GNM_RENDERTARGET_DUMP
		static bool SaveColorTarget(const char *const outName, const sce::Gnm::RenderTarget *const target, const DumpMask mask);
		static bool SaveDepthTarget(const char *const outName, const sce::Gnm::DepthRenderTarget *const target, const DumpMask mask);
		virtual bool SaveTarget(const char *pszOutName = NULL) const;
#endif	//GNM_RENDERTARGET_DUMP
	protected:
		// Exactly one of these two will be nonzero.
		sce::Gnm::RenderTarget *m_Target;				// Depending on how target was created, this may or may not be an owned pointer
		sce::Gnm::DepthRenderTarget *m_DepthTarget;		// Depending on how target was created, this may or may not be an owned pointer

		int m_LockedMip;

		// Texture object aliasing either of these
		sce::Gnm::Texture m_GnmTexture;		// note sizeof(CellGcmTexture) is 24, but this is 32, so we can't recycle that space.

#if DEBUG_TRACK_MSAA_RESOLVES
		// Track resolves to ensure they are not done redundantly.  Tracking
		// requires multi-threaded rendering to be disabled.
		const sce::Gnm::RenderTarget *m_DebugLastResolvedTarget;
#endif

		CoverageData	m_Coverage;

		bool m_Owned;
	};

inline const sce::Gnm::RenderTarget* grcRenderTargetGNM::DebugGetLastResolved() const
{
#if DEBUG_TRACK_MSAA_RESOLVES
	return m_DebugLastResolvedTarget;
#else
	return NULL;
#endif
}

inline void grcRenderTargetGNM::DebugSetUnresolved()
{
#if DEBUG_TRACK_MSAA_RESOLVES
	m_DebugLastResolvedTarget = NULL;
#endif
}

}	// namespace rage

#endif		// __GNM

#endif
