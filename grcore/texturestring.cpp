//
// grcore/texturestring.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "texturestring.h"
#include "string/string.h"

using namespace rage;

grcTextureString::grcTextureString(const char *filename) { 
	m_Name = StringDuplicate(filename); 
}


grcTextureString::~grcTextureString() { 
}


grcTextureFactory* grcTextureFactory::CreateStringTextureFactory(
		bool bMakeActive) {
	grcTextureFactoryString* f = ::rage_new grcTextureFactoryString;

	// This used to be done here, but that seems bad as none of the other
	// Create...TextureFactory() functions do that. If needed, the caller
	// will have to do this manually or call grcTextureFactory::InitClass().
	// grcTexture::None = (f)->Create("none");

	if(bMakeActive)
		sm_Instance = f;

	return f;
}

const grcRenderTarget *grcTextureFactoryString::GetBackBuffer(bool /*realize*/) const {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

grcRenderTarget *grcTextureFactoryString::GetBackBuffer(bool realize) {
	return GetBackBuffer(realize);
}

const grcRenderTarget *grcTextureFactoryString::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) const {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

grcRenderTarget *grcTextureFactoryString::GetFrontBuffer(bool nextBuffer) {
	return GetFrontBuffer(nextBuffer);
}

const grcRenderTarget *grcTextureFactoryString::GetFrontBufferDepth(bool /*realize=true*/) const {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

grcRenderTarget *grcTextureFactoryString::GetFrontBufferDepth(bool realize/*=true*/) {
	return GetFrontBufferDepth(realize);
}

const grcRenderTarget *grcTextureFactoryString::GetBackBufferDepth(bool /*realize*/) const {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

grcRenderTarget *grcTextureFactoryString::GetBackBufferDepth(bool realize) {
	return GetBackBufferDepth(realize);
}

void grcTextureFactoryString::BindDefaultRenderTargets() {

}
