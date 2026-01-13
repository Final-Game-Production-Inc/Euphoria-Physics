// 
// grcore/texturexenonproxy.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "grcore/texturestring.h"

#ifndef GRCORE_TEXTUREXENONPROXY_H
#define GRCORE_TEXTUREXENONPROXY_H

#include "grcore/device.h"
#include "vector/vector3.h"

struct D3DBaseTexture;

namespace rage {

class grcTextureFactoryXenonProxy: public grcTextureFactoryString {
public:
	grcTexture			*Create				(const char *pFilename,grcTextureFactory::TextureCreateParams *params);
	grcTexture			*Create				(class grcImage *pImage,grcTextureFactory::TextureCreateParams *params);

	void ClearFreeList();

	static void CreatePagedTextureFactory();
};

class grcTextureXenonProxy: public grcTexture {
public:
	grcTextureXenonProxy();
	grcTextureXenonProxy(datResource&);
	~grcTextureXenonProxy();

#if __RESOURCECOMPILER
	grcTextureXenonProxy(const char*,grcTextureFactory::TextureCreateParams *params);
	grcTextureXenonProxy(grcImage*,grcTextureFactory::TextureCreateParams *params);
	void Init(const char *pFilename,grcImage *pImage,grcTextureFactory::TextureCreateParams *params);

	const grcTextureObject *GetTexturePtr	(void) const { return reinterpret_cast<const void*>(m_Texture); }
	grcTextureObject   *GetTexturePtr	(void) { return reinterpret_cast<void*>(m_Texture); }
	int					GetWidth		(void) const { return(m_Width); }
	int					GetHeight		(void) const { return(m_Height); }
	int					GetDepth		(void) const { return 1; }
	int					GetMipMapCount	(void) const { return(m_MipCount); }
	int					GetBitsPerPixel	(void) const { return 0; /* overridden by grcTextureXenon */ }

	bool Copy(const grcImage*) { return false; }

	static int AddFreeSlots(bool isDXT1,void *addr,int slots,int overflowingSlots,const char *filename, bool tailOnly=false);
	static void *FindFreeSlot(bool isDXT1,int slots,int overflowingSlots,const char *filename, bool tailOnly=false);
	static void DisplayFreeSlotMem(); 

	virtual ChannelBits FindUsedChannels() const {return ChannelBits(false);}
#endif

	virtual int GetArraySize(void) const { return 0; }

	virtual void SetTextureSwizzle(eTextureSwizzle  r, eTextureSwizzle  g, eTextureSwizzle  b, eTextureSwizzle  a, bool bApplyToExistingSwizzle);
	virtual void GetTextureSwizzle(eTextureSwizzle& r, eTextureSwizzle& g, eTextureSwizzle& b, eTextureSwizzle& a) const;

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct&);
#endif

protected:
	D3DBaseTexture		*m_Texture;
	u16					m_Width, m_Height;
	int					m_MipCount;
};


}	// namespace rage

#endif	// GRCORE_TEXTUREXENONPROXY_H
