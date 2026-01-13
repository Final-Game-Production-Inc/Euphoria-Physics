//
// grcore/texturestring.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTURESTRING_H
#define GRCORE_TEXTURESTRING_H

#include "texture.h"

namespace rage {

class grcTextureString: public grcTexture {
public:
	grcTextureString(const char *filename);
	~grcTextureString();
	
	int GetWidth() const { return 0; }
	int GetHeight() const { return 0; }
	int GetBitsPerPixel() const { return 0; }
	bool Copy(const grcImage* /*pImage*/) { return true; }
	virtual int GetDepth() const { return 0; }
	virtual int GetMipMapCount() const { return 1; }
	virtual int GetArraySize() const { return 1; }
	virtual ChannelBits FindUsedChannels() const {ChannelBits ret(false); return ret;}
};


class grcTextureFactoryString: public grcTextureFactory {
public:
	grcTexture* Create(const char *filename,grcTextureFactory::TextureCreateParams*) { return rage_new grcTextureString(filename); }
	grcTexture* Create(class grcImage*,grcTextureFactory::TextureCreateParams*) { return rage_new grcTextureString("image"); }
	grcRenderTarget* CreateRenderTarget(const char *,grcRenderTargetType,int,int,int,CreateParams*, grcRenderTarget*)
					{ return NULL; }
	grcRenderTarget* CreateRenderTarget(const char *, const grcTextureObject * , grcRenderTarget*)
					{ return NULL; }
	void LockRenderTarget(int /*index*/,const grcRenderTarget *,const grcRenderTarget *,u32 /*layer*/=0,bool=true,u32 =0) { }
	void UnlockRenderTarget(int,const grcResolveFlags *) { }
	void LockMRT(const grcRenderTarget* [grcmrtColorCount], const grcRenderTarget *, const u32* = NULL) { }
	void UnlockMRT(const grcResolveFlagsMrt* /*resolveFlags*/ = NULL ) { }

	void PlaceTexture(class datResource&,grcTexture&) { }

	virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
	virtual grcRenderTarget *GetBackBuffer(bool realize=true);
	virtual const grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const;
	virtual grcRenderTarget *GetFrontBuffer(bool UNUSED_PARAM(nextBuffer));
	virtual const grcRenderTarget *GetFrontBufferDepth(bool realize=true) const;
	virtual grcRenderTarget *GetFrontBufferDepth(bool realize =true);
	virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
	virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);
	virtual void BindDefaultRenderTargets();
};

}	// namespace rage

#endif
