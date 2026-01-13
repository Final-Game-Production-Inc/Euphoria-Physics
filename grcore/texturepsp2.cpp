//
// grcore/texturepsp2.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//
#include "texturepsp2.h"

#if __PSP2

#pragma diag_suppress 828

namespace rage {

grcRenderTargetPSP2 *s_FrontBuffer, *s_DepthBuffer, *s_BackBuffer;

grcRenderTarget* grcTextureFactoryPSP2::GetBackBuffer(bool) { return s_BackBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetBackBuffer(bool) const { return s_BackBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetBackBufferDepth(bool) { return s_DepthBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetBackBufferDepth(bool) const { return s_DepthBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetFrontBuffer(bool) { return s_FrontBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetFrontBuffer(bool) const { return s_FrontBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetFrontBufferDepth(bool) { return s_DepthBuffer; }

grcRenderTarget* grcTextureFactoryPSP2::GetFrontBufferDepth(bool) const { return s_DepthBuffer; }

grcTextureFactoryPSP2::grcTextureFactoryPSP2()
{
}

void grcTextureFactoryPSP2::LockRenderTarget(int index,const grcRenderTarget *color,const grcRenderTarget *depth,
					  u32 layer, bool lockDepth, int nTextureAtlasFace, u32 mipLevel)
{
}

void grcTextureFactoryPSP2::UnlockRenderTarget(int index,const grcResolveFlags* flags, int nTextureAtlasFace)
{
}

void grcTextureFactoryPSP2::LockMRT(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth)
{
}

void grcTextureFactoryPSP2::UnlockMRT(const grcResolveFlagsMrt* resolveFlags)
{
}


grcTexture* grcTextureFactoryPSP2::Create(const char *filename,grcTextureFactory::TextureCreateParams *params)
{
	return 0;
}

grcTexture* grcTextureFactoryPSP2::Create(grcImage *,grcTextureFactory::TextureCreateParams *params)
{
	return 0;
}

grcTexture* grcTextureFactoryPSP2::Create(u32 width,u32 height,u32 format,void *pBuffer,grcTextureFactory::TextureCreateParams *params)
{
	return 0;
}

grcRenderTarget* grcTextureFactoryPSP2::CreateRenderTarget(const char*,grcRenderTargetType,int,int,int,grcTextureFactory::CreateParams*)
{
	return 0;
}

grcRenderTarget* grcTextureFactoryPSP2::CreateRenderTarget(const char*,const SceGxmTexture*)
{
	return 0;
}

void grcTextureFactoryPSP2::PlaceTexture(class datResource& rsc,grcTexture &tex)
{
}

void grcTextureFactoryPSP2::BindDefaultRenderTargets()
{
}


grcTextureFactory* grcTextureFactory::CreatePagedTextureFactory(bool bMakeActive)
{
	grcTextureFactory *pFactory = rage_new grcTextureFactoryPSP2;

	if	(bMakeActive)
		SetInstance(*pFactory);

	return(pFactory);
}

}	// namespace rage

#endif
