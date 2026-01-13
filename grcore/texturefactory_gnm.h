// 
// grcore/texturefactory_gnm.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_TEXTUREFACTORY_GNM_H
#define GRCORE_TEXTUREFACTORY_GNM_H

#include "grcore/config.h"
#include "grcore/setup.h"

#if __GNM || __RESOURCECOMPILER

#include "atl/bitset.h"
#include "texture.h"
#include "device.h"

namespace sce { namespace Gnm { class Texture; class RenderTarget; class DepthRenderTarget; } }

namespace rage {

	class grcImage;
	class grcRenderTargetGNM;
	class datResource;

	class grcTextureFactoryGNM: public grcTextureFactory {
		friend class grcDevice;
	public:
		grcTextureFactoryGNM();
		~grcTextureFactoryGNM();

		virtual grcTexture *Create(const char *filename,TextureCreateParams *params);
		virtual grcTexture *Create(grcImage *image,TextureCreateParams *params);
		virtual grcTexture* Create(u32 width, u32 height, u32 format, void* pBuffer, u32 numMips, TextureCreateParams * params = NULL);
		virtual grcTexture* Create(u32 width, u32 height, u32 depth, u32 format, void* pBuffer, TextureCreateParams * params = NULL);

		virtual grcRenderTarget *CreateRenderTarget(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params, grcRenderTarget* originalTarget = NULL);
#if !__RESOURCECOMPILER
		virtual grcRenderTarget *CreateRenderTarget(const char *pName, const grcTextureObject *pTexture, grcRenderTarget* originalTarget = NULL);
		virtual grcRenderTarget *CreateRenderTarget(const char *pName, const grcRenderTarget *pTexture, grcTextureFormat eShaderViewFormat);
		grcRenderTarget *CreateProxyRenderTarget(const char *pName, sce::Gnm::RenderTarget *color, sce::Gnm::DepthRenderTarget *depth, sce::Gnm::DepthRenderTarget *stencil);
		grcTexture		*CreateProxyFmaskTexture(const sce::Gnm::RenderTarget *target);
#endif // !__RESOURCECOMPILER

		virtual void SetArrayView		(u32 uArrayIndex); 
		virtual void LockRenderTarget	(int index,const grcRenderTarget *pColor, const grcRenderTarget *pDepth, u32 layer = 0, bool lockDepth = true, u32 D3D11_OR_ORBIS_ONLY(mipToLock) = 0);
		virtual void UnlockRenderTarget	(int index,const grcResolveFlags *pResolveFlags = NULL);

		virtual void LockMRT(const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock) = NULL);
		virtual void UnlockMRT(const grcResolveFlagsMrt* resolveFlags = NULL);

		void UnlockColorTarget(int index);
		void UnlockDepthTarget();

		bool IsTargetLocked(const grcRenderTarget *target) const;

#if DEVICE_EQAA
		void FinishRendering();
		void SwitchFragmentCompression(bool allow);
#endif

#if SUPPORT_RENDERTARGET_DUMP
		void UnlockBackBuffer();
#endif	//SUPPORT_RENDERTARGET_DUMP

#if __ASSERT
		static void CheckOutputFormat(uint8_t outputFormat);
		static void SetLockSingleTarget(const sce::Gnm::RenderTarget *target);
#endif

		void RelockRenderTargets();

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
		static grcTextureFormat TranslateToRageFormat(u32 oOrbisFormat);

		void SetBufferDepth(grcRenderTargetGNM *depthBuffer);
		void SetBackBufferDepth(grcRenderTargetGNM *depthBuffer);
		
		static grcTextureFactory *CreatePagedTextureFactory(bool bMakeActive = true);
		static void SetPrivateSurfaces(sce::Gnm::RenderTarget *front, sce::Gnm::DepthRenderTarget *frontDepth,
			sce::Gnm::RenderTarget *back, sce::Gnm::DepthRenderTarget *backDepth);

		void BindDefaultRenderTargets() {}
	private:

		bool IsAnythingLocked() const;
		
#if !MULTIPLE_RENDER_THREADS
		// this is used to store pointers to render targets in-between frames
		// we need this mainly for the LockRenderTarget() / UnlockRenderTarget() pair of functions
		const grcRenderTarget* 	m_LockColorRenderTargets[grcmrtColorCount];
		const grcRenderTarget* 	m_LockDepthRenderTarget;
#endif	//!MULTIPLE_RENDER_THREADS
		u16 					m_FrontBufferMemoryPoolID;

		static grcRenderTargetGNM *sm_FrontBuffer, *sm_BackBuffer;
		static grcRenderTargetGNM *sm_FrontBufferDepth, *sm_BackBufferDepth;
	};

}	// namespace rage

#endif		// __GNM

#endif
