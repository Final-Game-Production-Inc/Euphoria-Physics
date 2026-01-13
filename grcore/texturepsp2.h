//
// grcore/texturepsp2.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//
#ifndef GRCORE_TEXTUREPSP2_H
#define GRCORE_TEXTUREPSP2_H

#include "grcore/texture.h"

namespace rage
{

#if __PSP2

class grcTexturePSP2: public grcTexture
{
public:
	grcTexturePSP2(datResource&);
	int GetWidth() const { return m_Texture.width; }
	int GetHeight() const { return m_Texture.height; }
	int GetDepth() const { return 1; }
	int GetMipMapCount() const { return m_Texture.mipcount; }
	bool Copy(const grcImage*);
	ChannelBits FindUsedChannels() const;
	int GetBitsPerPixel() const;
};


class grcRenderTargetPSP2: public grcRenderTarget
{
public:
	grcRenderTargetPSP2();
};


class grcTextureFactoryPSP2: public grcTextureFactory
{
public:
	grcTextureFactoryPSP2();
	grcRenderTarget* GetBackBuffer(bool) const;
	grcRenderTarget* GetBackBuffer(bool);
	grcRenderTarget* GetFrontBuffer(bool) const;
	grcRenderTarget* GetFrontBuffer(bool);
	grcRenderTarget* GetBackBufferDepth(bool) const;
	grcRenderTarget* GetBackBufferDepth(bool);
	grcRenderTarget* GetFrontBufferDepth(bool) const;
	grcRenderTarget* GetFrontBufferDepth(bool);
	grcTexture* Create(const char *filename,TextureCreateParams *params = NULL);
	grcTexture* Create(class grcImage*,TextureCreateParams *params = NULL);
	grcTexture* Create(u32 /*width*/, u32 /*height*/, u32 /*eFormat*/, void* /*pBuffer*/, TextureCreateParams * /*params*/ = NULL);
	grcRenderTarget* CreateRenderTarget(const char*,grcRenderTargetType,int,int,int,grcTextureFactory::CreateParams*);
	grcRenderTarget* CreateRenderTarget(const char*,const SceGxmTexture*);
	void LockRenderTarget(int index,const grcRenderTarget *color,const grcRenderTarget *depth,
		u32 layer = 0, bool lockDepth = true, int nTextureAtlasFace = -1, u32 mipLevel = 0);
	void UnlockRenderTarget(int index,const grcResolveFlags* flags = NULL, int nTextureAtlasFace = -1);
	void LockMRT(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth);
	void UnlockMRT(const grcResolveFlagsMrt* resolveFlags = NULL);
	void PlaceTexture(class datResource& rsc,grcTexture &tex);
	void BindDefaultRenderTargets();
};

#endif

}

#endif	// GRCORE_TEXTUREPSP2_H
