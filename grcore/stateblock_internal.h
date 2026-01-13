// 
// grcore/stateblock_internal.h 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_STATEBLOCK_INTERNAL_H 
#define GRCORE_STATEBLOCK_INTERNAL_H 

#if __XENON
#include <system/xtl.h>		// for BLENDSTATE structure
#endif

#if __WIN32
#include "grcore/config.h"
#endif

#if RSG_ORBIS
#include <gnm/regs.h>
#include <gnm/sampler.h>
#endif

struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;
struct ID3D11SamplerState;

namespace rage {

struct grcDepthStencilStateDesc;
struct grcRasterizerStateDesc;
struct grcBlendStateDesc;

// "Portable" implementations for pre-DX11 platforms.  Note alignment is only necessary on PS3 so we intentionally don't use ALIGNAS here.

struct grcDepthStencilState {
#if __WIN32 || RSG_ORBIS
	u8 DepthEnable,
		DepthWriteMask;
	u8 StencilEnable;
	u8 TwoSidedStencil;
	u16 DepthFunc;					// CMP_... in effect_values.h; default CMP_LESS
	u8 StencilReadMask;				// default: 0xFF
	u8 StencilWriteMask;			// default: 0xFF

	u16 FrontStencilFailOp, FrontStencilDepthFailOp, FrontStencilPassOp, FrontStencilFunc;
	u16 BackStencilFailOp, BackStencilDepthFailOp, BackStencilPassOp, BackStencilFunc;
# if RSG_ORBIS
	sce::Gnm::DepthStencilControl dsc;
	sce::Gnm::StencilControl sc;
	sce::Gnm::StencilOpControl soc;
# endif
# if __D3D11
	static ID3D11DepthStencilState* AllocateState(const grcDepthStencilStateDesc&);
# endif
# if RSG_DURANGO
	static ID3D11DepthStencilState *AllocateStateX(const grcDepthStencilStateDesc&, ID3D11DepthStencilState *);
#endif
#elif __PS3
	u8 DepthEnable:1, DepthWriteMask:1, StencilEnable:1, TwoSidedStencil:1, DepthFunc:4;	// or in 0x200 to use DepthFunc (could be only three bits)
	u8 FrontStencilFunc:4, BackStencilFunc: 4;	// or in 0x200 to use these (could be only three bits)
	u8 StencilReadMask, StencilWriteMask;
	u16 FrontStencilFailOp, FrontStencilDepthFailOp, FrontStencilPassOp;
	u16 BackStencilFailOp, BackStencilDepthFailOp, BackStencilPassOp;
#endif
#if TRACK_DEPTH_BOUNDS_STATE
	//D3D11X extensions
	u8 DepthBoundsEnable;
#endif
} ;

struct grcRasterizerState {
#if __WIN32 || RSG_ORBIS
	u16 FillMode;
	u16 CullMode;
	union { float f; unsigned u; } DepthBias;
	float DepthBiasClamp;
	union { float f; unsigned u; } SlopeScaledDepthBias;
	// u8 DepthClipEnable;
	// u8 FrontCounterClockwise;
	// u16 ScissorEnable;
	u32 MultisampleEnable:1, HalfPixelOffset:1, pad:30;	// Make sure any pad bits are initialized or duplicate state merging will fail!
# if RSG_ORBIS
	sce::Gnm::PrimitiveSetup ps;
# endif
# if __D3D11
	static ID3D11RasterizerState* AllocateState(const grcRasterizerStateDesc&);
# endif
# if RSG_DURANGO
	static ID3D11RasterizerState *AllocateStateX(const grcRasterizerStateDesc&, ID3D11RasterizerState *);
#endif
#elif __PS3
	u16 FillMode;
	u16 CullFace;
	u8 CullFaceEnable:1, HalfPixelOffset:1, pad:6 ;		// Make sure any pad bits are initialized or duplicate state merging will fail!
	u8 EdgeCullMode;
	u16 PolygonOffsetFillEnable; // really only one bit
	union { float f; unsigned u; } Offset0;
	union { float f; unsigned u; } Offset1;
#endif
} ;

struct grcBlendState {
#if __XENON
	D3DBLENDSTATE Targets[4];
	u8 WriteMasks[4];
	u8 AlphaTestComparison, AlphaToMaskOffsets,
		AlphaTestEnable:1, AlphaToCoverageEnable:1, HighPrecisionBlendEnable:1, IndependentBlendEnable:1, 
		Enable0:1, Enable1:1, Enable2:1, Enable3:1;
#elif __PS3
	// On PS3 you can only enable/disable alpha blending entirely per MRT but cannot change the function.
	// You can control the write mask per channel per MRT though.
	u8 Enable0:1,Enable123:3,AlphaTestEnable:1,AlphaFunc:3;	// or 0x200 in with AlphaFunc.
	u8 BlendOp:4, BlendOpAlpha:4;	// or 0x8000 into these before passing them down
	u16 WriteMask123;	// preformatted for cellGcmSetColorMaskMrt
	u32 WriteMask0;		// preformatted for cellGcmSetColorMask
	u32 SrcBlend, DestBlend; // color and alpha packed together.
#elif RSG_ORBIS
	struct Target {
		u8 BlendEnable;
		u8 SrcBlend, DestBlend, BlendOp, 
			SrcBlendAlpha, DestBlendAlpha,  BlendOpAlpha;
	};
	u32 ColorWriteMask;
	u8 AlphaToMaskOffsets, AlphaToCoverageEnable;
	Target Targets[8];			// this is redundant, only for reading the state without a bunch of extra complexity
	sce::Gnm::BlendControl bc[8];
	bool IndependentBlendEnable;
#else
	struct Target {
		u8 BlendEnable;
		u8 RenderTargetWriteMask;
		u16 SrcBlend, DestBlend, BlendOp, 
			SrcBlendAlpha, DestBlendAlpha,  BlendOpAlpha;
	};
	u8 AlphaToCoverageEnable, IndependentBlendEnable;
	u16 AlphaTestComparison;
	int AlphaTestEnable;	// this could be a u8 but we pad it out for alignment.
	Target Targets[8];
# if __D3D11
	static ID3D11BlendState *AllocateState(const grcBlendStateDesc&);
# endif
# if RSG_DURANGO
	static ID3D11BlendState *AllocateStateX(const grcBlendStateDesc&, ID3D11BlendState *);
#endif
#endif
} ;

struct grcSamplerState {
#if __WIN32 || RSG_ORBIS
	u8 MinFilter, MagFilter, MipFilter;	// Translated from DX11 enumerated types
	u8 AddressU, AddressV, AddressW;	// TADDRESS_CLAMP
	u8 MaxAnisotropy;				// 16
	u8 BorderColor, BorderColorW;	// only black(0) or white(~0) are supported on DX9-era hardware.  (Xenon supports "opaque" black too)
	u8 TrilinearThresh;
	u8 MinMipLevel, MaxMipLevel;	// MinMipLevel == MaxLod, MaxMipLevel == MinLod
	union { unsigned u; float f; } MipLodBias;				// 0.0f
# if __D3D11 || RSG_ORBIS
	int CompareFunc;
# endif
# if __D3D11
	static ID3D11SamplerState *AllocateState(const grcSamplerStateDesc&);
# endif
# if RSG_DURANGO
	static ID3D11SamplerState *AllocateStateX(const grcSamplerStateDesc&, ID3D11SamplerState *);
#endif
# if RSG_ORBIS
	sce::Gnm::Sampler s;
# endif
#else
	u32 control0,
		address,
		filter;
	u16 control2, border;
#endif
} ;

#if __PS3
// Alias all of the payload types into a union to guarantee they're all the same size.
// This lets us keep all of them in the same cache (addresses are guaranteed unique so we
// can discriminate based on use context) and reduce the amount of code.
union grcStateBlockUnion {
	grcDepthStencilState DepthStencil;
	grcRasterizerState Rasterizer;
	grcBlendState Blend;
	grcSamplerState Sampler;
};
CompileTimeAssert(sizeof(grcStateBlockUnion)==16);
#endif

} // namespace rage

#endif // GRCORE_STATEBLOCK_INTERNAL_H 
