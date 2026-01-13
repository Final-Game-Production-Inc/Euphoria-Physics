// 
// grcore/textureogl.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "grcore/config.h"
#if __OPENGL


#include "textureogl.h"
#include "texturereference.h"

#include "image.h"

#include "data/resource.h"
#include "string/string.h"
#include "system/memory.h"
#include "system/typeinfo.h"

#include "opengl.h"
#include "device.h"

// seems like this should work, but it doesn't
#define DEPTH_TARGET_USES_TEXTURE 0

namespace rage {

grcTextureFactory *grcTextureFactory::CreatePagedTextureFactory(bool bMakeActive) 
{
	grcTextureFactory *pFactory = rage_new grcTextureFactoryOGL;

	if	(bMakeActive)
		sm_Instance = pFactory;

	return(pFactory);
}

grcTextureOGL::grcTextureOGL(const char *pFilename)
{
	grcImage	*pImage = NULL;

	sysMemStartTemp();
	if	(strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
		pImage = grcImage::Load(pFilename);

	if	(pImage)
	{
		int	w = pImage->GetWidth();
		int	h = pImage->GetHeight();

		if	((w & 3) || (h & 3))
		{
			grcErrorf("grcTexturePC - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);
			pImage->Release();
			pImage = NULL;
		}
	}

	if	(!pImage)
	{
		u32 texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFF;

		pImage = grcImage::Create(4, 4, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false, 0);
		u32 *texels = (u32*) pImage->GetBits();

		for	(int i = 0; i < 16; i++)
			texels[i] = texel;	
	}
	sysMemEndTemp();

	Init(pFilename,pImage);

	sysMemStartTemp();
	pImage->Release();
	sysMemEndTemp();
}

grcTextureOGL::grcTextureOGL(grcImage *pImage)
{
	const char* name = "image";
#if __BANK
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK
	Init(name,pImage,params);
}

grcTextureOGL::~grcTextureOGL()
{
	glDeleteTextures(1,&m_TexObj);
}

void grcTextureOGL::Init(const char * filename,grcImage *image)
{
	m_Name = grcSaveTextureNames ? StringDuplicate(filename) : NULL;

	m_Width = image->GetWidth();
	m_Height = image->GetHeight();

	bool compressed = false; 
	GLint internalformat = 0;
	GLenum format = 0;
	GLenum type = 0;

	grcImage::Format rage_format = image->GetFormat();
	switch (rage_format)
	{
	case grcImage::DXT1: compressed = true; internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
	case grcImage::DXT3: compressed = true; internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
	case grcImage::DXT5: compressed = true; internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
	case grcImage::A8R8G8B8:      internalformat = GL_RGBA;        format = GL_ARGB_SCE; type = GL_UNSIGNED_BYTE;  break;
	case grcImage::A16B16G16R16F: internalformat = GL_RGBA16F_ARB; format = GL_RGBA;     type = GL_HALF_FLOAT_ARB; break;
	default: grcWarningf("Texture %s has an unsupported format", filename);
	}

	GLenum image_target;
	if (image->GetType() == grcImage::CUBE)
	{
		m_TargetEnum = GL_TEXTURE_CUBE_MAP;
		image_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}
	else
	{
		m_TargetEnum = GL_TEXTURE_2D;
		image_target = GL_TEXTURE_2D;
	}

	glGenTextures(1,&m_TexObj);
	Assert(m_TexObj);

	glBindTexture(m_TargetEnum, m_TexObj);

	grcImage* layer = image;
	while (layer) 
	{
		int level = 0;
		grcImage* mip = layer;
		while (mip)
		{
			if (compressed)
			{
				glCompressedTexImage2D(image_target,
					level,
					internalformat,
					mip->GetWidth(),
					mip->GetHeight(),
					false,
					mip->GetSize(),
					mip->GetBits());
			}
			else
			{
				glTexImage2D(image_target,
					level,
					internalformat,
					mip->GetWidth(),
					mip->GetHeight(),
					false,
					format,
					type,
					mip->GetBits());
			}		
			mip = mip->GetNext();
			++level;
		}
		if (m_TargetEnum == GL_TEXTURE_CUBE_MAP)
			image_target++;
		layer = layer->GetNextLayer();
	}

	glTexParameteri(m_TargetEnum,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(m_TargetEnum,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(m_TargetEnum,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(m_TargetEnum,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glBindTexture(m_TargetEnum, 0);
}

grcTextureOGL::grcTextureOGL(datResource &) {
}


int grcTextureOGL::GetWidth() const {
	return m_Width;
}


int grcTextureOGL::GetHeight() const {
	return m_Height;
}

const char *grcTextureOGL::GetName() const {
	return m_Name;
}

const void *grcTextureOGL::GetTexturePtr() const {
	m_LastBind = GRCDEVICE.GetFrameCounter();
	return reinterpret_cast<const void*>(m_TexObj);
}

void *grcTextureOGL::GetTexturePtr() {
	m_LastBind = GRCDEVICE.GetFrameCounter();
	return reinterpret_cast<void*>(m_TexObj);
}

const void *grcRenderTargetOGL::GetTexturePtr() const {
	m_LastBind = GRCDEVICE.GetFrameCounter();
	return reinterpret_cast<const void*>(m_TexObj);
}

void *grcRenderTargetOGL::GetTexturePtr() {
	m_LastBind = GRCDEVICE.GetFrameCounter();
	return reinterpret_cast<void*>(m_TexObj);
}

grcTexture* grcTextureFactoryOGL::Create(const char *filename) {
	char	buffer[256];

	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	RAGE_TRACK_NAME(name);

	StringNormalize(buffer, filename, sizeof(buffer));

	grcTexture *tex = LookupTextureReference(buffer);
	if (tex)
		return rage_new grcTextureReference(buffer, tex);

	return rage_new grcTextureOGL(buffer);
}


grcTexture* grcTextureFactoryOGL::Create(grcImage *image) {
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);

	return rage_new grcTextureOGL(image);
}


#if __PPU
#define glGenRenderbuffersEXT glGenRenderbuffersOES
#define glGenFramebuffersEXT glGenFramebuffersOES
#define glBindRenderbufferEXT glBindRenderbufferOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glRenderbufferStorageEXT glRenderbufferStorageOES
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER_OES
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE_OES
#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT_OES
#define glDeleteRenderbuffersEXT glDeleteRenderbuffersOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
#define glFramebufferTexture2DEXT glFramebufferTexture2DOES
#define glFramebufferRenderbufferEXT glFramebufferRenderbufferOES
#define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
#define RT_FORMAT GL_ARGB_SCE
#else
#define RT_FORMAT GL_RGBA8
#endif

grcRenderTargetOGL::grcRenderTargetOGL(const char *name,grcRenderTargetType type,int width,int height,int bitsPerPixel,grcTextureFactory::CreateParams *params) {
	m_Name = StringDuplicate(name);
	m_Width = width;
	m_Height = height;
	m_Type = type;
	//bool useFloat = params? params->UseFloat : false;
	m_Surface = m_TexObj = 0;
	glGenFramebuffersEXT(1, &m_Framebuffer);
	if (type == grcrtDepthBuffer || type == grcrtShadowMap) 
	{
#if DEPTH_TARGET_USES_TEXTURE
		glGenTextures(1, &m_TexObj);
		glBindTexture(GL_TEXTURE_2D, m_TexObj);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
#else // DEPTH_TARGET_USES_TEXTURE
		glGenRenderbuffersEXT(1, &m_Surface);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_Surface);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
//		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
#endif // DEPTH_TARGET_USES_TEXTURE
	}
	else if (type == grcrtPermanent) 
	{
		GLuint internalformat;
		GLenum format;
		GLenum type;
		GLint  filter;

		switch (params->Format)
		{
		case grctfNone:
		case grctfA8R8G8B8:
			internalformat = RT_FORMAT;
			format = GL_RGBA;
			type = GL_UNSIGNED_BYTE;
			filter = GL_LINEAR;
			break;
#if __PPU
		case grctfA16B16G16R16F:
		case grctfA16B16G16R16F_NoExpand:
			internalformat = GL_RGBA16F_ARB;
			format = GL_RGBA;
			type = GL_FLOAT;
			filter = GL_LINEAR;
			break;
		case grctfR32F:
			internalformat = GL_RGBA32F_ARB;
			format = GL_RGBA;
			type = GL_FLOAT;
			filter = GL_NEAREST;
			break;
#endif
		default: 
			grcErrorf("unsupported render target format");
			internalformat = GL_RGBA32F_ARB;
			format = GL_RGBA;
			type = GL_FLOAT;
			filter = GL_NEAREST;
			break;
		}

		glGenTextures(1, &m_TexObj);
		glBindTexture(GL_TEXTURE_2D, m_TexObj);
		// this happens to be the fastest render target type, it is also required if the color/depth targets are power of 2
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_ALLOCATION_HINT_SCE,GL_TEXTURE_TILED_GPU_SCE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		// generate the blank image data after we're set all the options
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		grcWarningf("unsupported render target type %d\n", type);
	}
}


grcRenderTargetOGL::~grcRenderTargetOGL() {
	if (m_TexObj)
		glDeleteTextures(1, &m_TexObj);
	if (m_Surface)
		glDeleteRenderbuffersEXT(1, &m_Surface);
	glDeleteFramebuffersEXT(1, &m_Framebuffer);
}

const char *grcRenderTargetOGL::GetName() const {
	return m_Name;
}


int grcRenderTargetOGL::GetWidth() const {
	return m_Width;
}

int grcRenderTargetOGL::GetHeight() const {
	return m_Height;
}

grcRenderTarget *grcTextureFactoryOGL::CreateRenderTarget(const char *pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params) {

	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	return rage_new grcRenderTargetOGL(pName,eType,nWidth,nHeight,nBitsPerPixel,params);
}

static const grcRenderTargetOGL *s_LockedColor, *s_LockedDepth;
static int s_PreviousWidth, s_PreviousHeight;

void grcTextureFactoryOGL::LockRenderTarget	(int index,const grcRenderTarget *color,const grcRenderTarget *depth,u32 /*layer*/,bool lockDepth,int /*nTextureAtlasFace*/,u32 /*mipLevel*/) {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ((grcRenderTargetOGL*) (color? color : depth))->m_Framebuffer);
	if (color) {
		s_LockedColor = SafeCast(const grcRenderTargetOGL,color);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index, GL_TEXTURE_2D, s_LockedColor->m_TexObj, 0);
	}
	if (depth && lockDepth) {
		s_LockedDepth = SafeCast(const grcRenderTargetOGL,depth);
#if DEPTH_TARGET_USES_TEXTURE
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, s_LockedColor->m_TexObj, 0);
#else
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, s_LockedDepth->m_Surface);
#endif
	}
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		grcErrorf("framebuffer bad %d",status);
	}
	s_PreviousWidth = GRCDEVICE.GetWidth();
	s_PreviousHeight = GRCDEVICE.GetHeight();
	if (s_LockedColor) {
		GRCDEVICE.SetSize(s_LockedColor->m_Width,s_LockedColor->m_Height);
		glViewport(0,0,s_LockedColor->m_Width,s_LockedColor->m_Height);
	}
	else {
		GRCDEVICE.SetSize(s_LockedDepth->m_Width,s_LockedDepth->m_Height);
		glViewport(0,0,s_LockedDepth->m_Width,s_LockedDepth->m_Height);
	}

}

void grcTextureFactoryOGL::UnlockRenderTarget(int /*index*/,const grcResolveFlags *, int /*nTextureAtlasFace*/) {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	GRCDEVICE.SetSize(s_PreviousWidth,s_PreviousHeight);
	glViewport(0,0,s_PreviousWidth,s_PreviousHeight);
}

void grcTextureFactoryOGL::LockMRT(const grcRenderTarget* color[grcmrtColorCount],const grcRenderTarget * depth)
{
	for (s32 iIndex = 0; iIndex < grcmrtColorCount; iIndex++)
	{
		LockRenderTarget(iIndex, color[iIndex], depth, 0, true, 0, 0);
	}
}

void grcTextureFactoryOGL::UnlockMRT(const grcResolveFlagsMrt* resolveFlags)
{
	for (s32 iIndex = 0; iIndex < grcmrtColorCount; iIndex++)
	{
		UnlockRenderTarget(iIndex, (resolveFlags != NULL) ? (const grcResolveFlags*)resolveFlags[iIndex] : NULL, 0);
	}
}

void grcTextureFactoryOGL::PlaceTexture(datResource &rsc,grcTexture** base,int count) {
	for (int i=0; i<count; i++) {
		rsc.PointerFixup(base[i]);
		switch (base[i]->GetResourceType()) {
			case grcTexture::NORMAL: ::new (base[i]) grcTextureOGL(rsc); break;
			case grcTexture::RENDERTARGET: Assert(0 && "unsafe to reference a rendertarget"); break;
			case grcTexture::REFERENCE: ::new (base[i]) grcTextureReference(rsc); break;
			// case grcTexture::DICTIONARY_REFERENCE: ::new (base[i]) grcDictionaryReference(rsc); break;
			default: Quitf("Bad resource type %d in grcTextureFactoryPC::PlaceTexture",base[i]->GetResourceType());
		}
	}
}

grcRenderTarget *grcTextureFactoryOGL::GetBackBuffer(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryOGL::GetBackBuffer(bool realize) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryOGL::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryOGL::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryOGL::GetBackBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcTexture *grcTextureFactoryOGL::GetBackBufferDepth(bool realize) const {
	return GetBackBuffer(realize);
}

}	// namespace rage

#endif
