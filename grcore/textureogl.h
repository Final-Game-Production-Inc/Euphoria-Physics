// 
// grcore/textureogl.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_TEXTUREOGL_H
#define GRCORE_TEXTUREOGL_H

#include "grcore/config.h"

#if __OPENGL

#include "texture.h"

namespace rage {

class grcImage;
class datResource;

class grcTextureFactoryOGL: public grcTextureFactory {
public:
	virtual grcTexture *Create(const char *filename);
	virtual grcTexture *Create(grcImage *image);
	virtual grcRenderTarget *CreateRenderTarget(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params);
	virtual void LockRenderTarget	(int index,const grcRenderTarget *pColor, const grcRenderTarget *pDepth, grcCubeFace eFace = grcPositiveX,bool lockDepth=true,int nTextureAtlasFace = -1,u32 mipLevel = 0);
	virtual void UnlockRenderTarget	(int index,const grcResolveFlags *);
	virtual void LockMRT	(const grcRenderTarget *pColor[grcmrtColorCount], const grcRenderTarget *pDepth);
	virtual void UnlockMRT	(const grcResolveFlagsMrt* resolveFlags = NULL);
	virtual void PlaceTexture		(datResource &, grcTexture **, int);
	virtual const grcRenderTarget *GetBackBuffer(bool realize=true) const;
	virtual grcRenderTarget *GetBackBuffer(bool realize=true);
	virtual const grcRenderTarget *GetFrontBuffer(bool nextBuffer) const;
	virtual grcRenderTarget *GetFrontBuffer(bool nextBuffer);
	virtual const grcRenderTarget *GetBackBufferDepth(bool realize=true) const;
	virtual grcRenderTarget *GetBackBufferDepth(bool realize=true);
};

class grcTextureOGL: public grcTexture {
public:
	grcTextureOGL(const char *filename);
	grcTextureOGL(grcImage *image);
	grcTextureOGL(class datResource&);
	~grcTextureOGL();

	virtual const char *GetName() const;
	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual int GetDepth() const { return 1; }
	virtual int GetMipMapCount() const { return 1; }
	virtual void *GetTexturePtr();
	virtual const void *GetTexturePtr() const;

private:
	void Init(const char *filename,grcImage *image /* may be NULL*/);
	const char* m_Name; 
	u32 m_TexObj;
	u16 m_Width, m_Height;
	int m_Format;
	u16 m_TargetEnum;	// current enums are not bigger than 16bits
	u8 pad0, pad1;
};

class grcRenderTargetOGL: public grcRenderTarget {
	friend class grcTextureFactoryOGL;
public:
	grcRenderTargetOGL(const char *name,grcRenderTargetType type,int width,int height,int bitsPerPixel,grcTextureFactory::CreateParams *params);
	~grcRenderTargetOGL();

	virtual const char *GetName() const;
	virtual int GetWidth() const;
	virtual int GetHeight() const;
	virtual int GetDepth() const { return 1; }
	virtual int GetMipMapCount() const { return 1; }
	virtual void *GetTexturePtr() const;

private:
	const char *m_Name;
	unsigned m_Surface, m_TexObj, m_Framebuffer;
	int m_Width, m_Height;
};

}	// namespace rage

#endif		// __OPENGL

#endif
