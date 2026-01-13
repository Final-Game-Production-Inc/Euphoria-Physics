//
// grcore/texturedefault.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_TEXTUREDEFAULT_H
#define GRCORE_TEXTUREDEFAULT_H

#include "grcore/config.h"

#if __OPENGL
#include "grcore/textureogl.h"
namespace rage
{
	typedef grcTextureOGL grcTextureDefault;
	typedef grcTextureFactoryOGL grcTextureFactoryDefault;
	typedef grcRenderTargetOGL grcRenderTargetDefault;
} // namespace rage
#elif __GCM
#include "grcore/texturegcm.h"
namespace rage
{
	typedef grcTextureGCM grcTextureDefault;
	typedef grcTextureFactoryGCM grcTextureFactoryDefault;
	typedef grcRenderTargetGCM grcRenderTargetDefault;
} // namespace rage
#elif __WIN32PC
#include "grcore/texturepc.h"
#include "grcore/texture_d3d9.h"
#include "grcore/texture_d3d11.h"
namespace rage
{
	typedef grcTexturePC grcTextureDefault;
	typedef grcTextureFactoryPC grcTextureFactoryDefault;
	typedef grcRenderTargetPC grcRenderTargetDefault;
#if __D3D11
	typedef grcTextureDX11 grcTextureD3D11;
	typedef grcRenderTargetDX11 grcRenderTargetD3D11;
#endif // __D3D11
} // namespace rage
#elif RSG_DURANGO
#include "grcore/texturepc.h"
#include "grcore/texture_durango.h"
#include "grcore/rendertarget_durango.h"
namespace rage
{
	typedef grcTextureDurango grcTextureDefault;
	typedef grcTextureFactoryDX11 grcTextureFactoryDefault;
	typedef grcRenderTargetDurango grcRenderTargetDefault;
	typedef grcTextureDurango grcTextureD3D11;
	typedef grcRenderTargetDurango grcRenderTargetD3D11;
} // namespace rage
#elif __XENON
#include "grcore/texturexenon.h"
namespace rage
{
	typedef grcTextureXenon grcTextureDefault;
	typedef grcTextureFactoryXenon grcTextureFactoryDefault;
	typedef grcRenderTargetXenon grcRenderTargetDefault;
} // namespace rage
#elif __PSP2
#include "grcore/texturepsp2.h"
namespace rage 
{
	typedef grcTexturePSP2 grcTextureDefault;
	typedef grcTextureFactoryPSP2 grcTextureFactoryDefault;
	typedef grcRenderTargetPSP2 grcRenderTargetDefault;
}
#elif __GNM
#include "grcore/texture_gnm.h"
#include "grcore/rendertarget_gnm.h"
#include "grcore/texturefactory_gnm.h"
namespace rage 
{
	typedef grcTextureGNM grcTextureDefault;
	typedef grcTextureFactoryGNM grcTextureFactoryDefault;
	typedef grcRenderTargetGNM grcRenderTargetDefault;
}
#endif

#endif
