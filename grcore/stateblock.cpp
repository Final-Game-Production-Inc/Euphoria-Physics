// 
// grcore/stateblock.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "stateblock.h"
#include "stateblock_internal.h"

#include "grcore/effect_typedefs.h"
#include "grcore/config.h"			// for D3D11_ONLY on some platforms

#include "atl/map.h"
#include "string/stringhash.h"
#include <string.h>		// for memcmp
#include "system/magicnumber.h"
#include "system/param.h"
#include "security/obfuscatedtypes.h"
#include "profile/timebars.h"

#if __WIN32
#include "device.h"
#include "wrapper_d3d.h"
#if __D3D11
#include "system/d3d11.h"
#else
#include "system/d3d9.h"
#endif
#elif __PPU
#include "wrapper_gcm.h"
#include "grcorespu.h"
#include <cell/gcm.h>
#elif __SPU
#include "atl/simplecache.h"
#define DUMB_SET_STATE		__ASSERT		// enable only non-cached code paths to compare code size (we need the extra space on assert builds, it's about 2k savings)
#elif RSG_ORBIS
#include "device.h"
#include "gfxcontext_gnm.h"
#include <math.h>
#endif

// On d3d11 we can store a user-defined value inside the state block to reference. It shows in PIX, too.
#define CACHESTATEBLOCKNAMES_D3D11			(__D3D11 && __WIN32PC && !__FINAL)

#if CACHESTATEBLOCKNAMES_D3D11
#define CACHESTATEBLOCKNAMES_D3D11_ONLY(x)	x
#else
#define CACHESTATEBLOCKNAMES_D3D11_ONLY(x)
#endif // CACHESTATEBLOCKNAMES_D3D11

#if !__SPU

#if !__FINAL
PARAM(nolazystateblock,"Do not ignore redundant state block sets on main processor");
#endif

#if __WIN32PC
#define SetRenderState_Inline(s,v) SetRenderState(s,v)
#endif

namespace rage { 

#if RSG_ORBIS
extern __THREAD grcContextHandle *g_grcCurrentContext;
#define gfxc (*g_grcCurrentContext)
#endif
PPU_ONLY(extern spuGcmState s_spuGcmState);
PPU_ONLY(extern int g_SamplerStateCount);
PPU_ONLY(extern uint32_t g_WindowPixelCenter);
PPU_ONLY(extern uint32_t g_MainColorWrite);
extern grcSamplerState *g_SamplerStates;
#if __D3D11
extern ID3D11SamplerState **g_SamplerStates11;
#endif

#if RSG_ORBIS
	static sce::Gnm::AnisotropyRatio remapAnisotropic[] = { 
		sce::Gnm::kAnisotropyRatio1,	// 0
		sce::Gnm::kAnisotropyRatio1,	// 1
		sce::Gnm::kAnisotropyRatio2,	// 2
		sce::Gnm::kAnisotropyRatio2,	// 3
		sce::Gnm::kAnisotropyRatio4,	// 4
		sce::Gnm::kAnisotropyRatio4,	// 5
		sce::Gnm::kAnisotropyRatio4,	// 6
		sce::Gnm::kAnisotropyRatio4,	// 7
		sce::Gnm::kAnisotropyRatio8,	// 8
		sce::Gnm::kAnisotropyRatio8,	// 9
		sce::Gnm::kAnisotropyRatio8,	// 10
		sce::Gnm::kAnisotropyRatio8,	// 11
		sce::Gnm::kAnisotropyRatio8,	// 12
		sce::Gnm::kAnisotropyRatio8,	// 13
		sce::Gnm::kAnisotropyRatio8,	// 14
		sce::Gnm::kAnisotropyRatio8,	// 15
		sce::Gnm::kAnisotropyRatio16 };
#endif

#if (RSG_PC || RSG_DURANGO || RSG_ORBIS) && !__RESOURCECOMPILER	&& !__TOOL	
	static u32 uOverrideAnisotropic = 0;
#endif

#if __D3D11
ID3D11RasterizerState* grcRasterizerState::AllocateState(const grcRasterizerStateDesc &desc)
{
	ID3D11RasterizerState *pState = NULL;
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("grcRasterizerState::AllocateState", 0.04f);
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateRasterizerState((D3D11_RASTERIZER_DESC*)&desc,&pState));
	return pState;
}

ID3D11DepthStencilState* grcDepthStencilState::AllocateState(const grcDepthStencilStateDesc &desc)
{
	ID3D11DepthStencilState *pState = NULL;
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("grcDepthStencilState::AllocateState", 0.04f);
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateDepthStencilState((D3D11_DEPTH_STENCIL_DESC*)&desc,&pState));

	if (__ASSERT && pState)
	{
		grcDepthStencilStateDesc ret;
		pState->GetDesc((D3D11_DEPTH_STENCIL_DESC*)&ret);
		grcAssertf(ret == desc, "CreateDepthStencilState mutated the state!");
	}

	return pState;
}

ID3D11BlendState* grcBlendState::AllocateState(const grcBlendStateDesc &desc)
{
	ID3D11BlendState *pState = NULL;
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("grcBlendState::AllocateState", 0.04f);
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateBlendState((D3D11_BLEND_DESC*)&desc,&pState));
	return pState;
}


#if RSG_DURANGO
static D3D11X_TEX_CLAMP	ConvertToTexClamp(int addressTex)
{
	switch(addressTex)
	{
		case(grcSSV::TADDRESS_WRAP):
			return(D3D11X_TEX_CLAMP_WRAP);
			break;
		case(grcSSV::TADDRESS_MIRROR):
			return(D3D11X_TEX_CLAMP_MIRROR);
			break;
		case(grcSSV::TADDRESS_CLAMP):
			return(D3D11X_TEX_CLAMP_LAST_TEXEL);
			break;
		case(grcSSV::TADDRESS_BORDER):
			return(D3D11X_TEX_CLAMP_BORDER);
			break;
		case(grcSSV::TADDRESS_MIRRORONCE):
			return(D3D11X_TEX_CLAMP_MIRROR_ONCE_LAST_TEXEL);
			break;
		case(0):
		default:
			return(D3D11X_TEX_CLAMP_WRAP);
			break;
	}
}

static D3D11X_TEX_ANISO_RATIO ConvertToMaxAnisoRatio(int filter, u32 maxAniso)
{
	if(filter != grcSSV::FILTER_ANISOTROPIC)
		return(D3D11X_TEX_ANISO_RATIO_1);

	if(maxAniso < 2)
		return(D3D11X_TEX_ANISO_RATIO_1);

	if(maxAniso < 4)
		return(D3D11X_TEX_ANISO_RATIO_2);

	if(maxAniso < 8)
		return(D3D11X_TEX_ANISO_RATIO_4);

	if(maxAniso < 16)
		return(D3D11X_TEX_ANISO_RATIO_8);

	return(D3D11X_TEX_ANISO_RATIO_16);
}

static D3D11X_TEX_DEPTH_COMPARE ConvertToDepthCompare(int func)
{
#if 1
	// if any of these fails, then use alternative (and slower) "switch" path below:
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_NEVER		== (grcRSV::CMP_NEVER-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_LESS			== (grcRSV::CMP_LESS-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_EQUAL		== (grcRSV::CMP_EQUAL-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_LESSEQUAL	== (grcRSV::CMP_LESSEQUAL-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_GREATER		== (grcRSV::CMP_GREATER-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_NOTEQUAL		== (grcRSV::CMP_NOTEQUAL-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_GREATEREQUAL == (grcRSV::CMP_GREATEREQUAL-1));
	CompileTimeAssert(D3D11X_TEX_DEPTH_COMPARE_ALWAYS		== (grcRSV::CMP_ALWAYS-1));

	Assert((func >= grcRSV::CMP_NEVER) && (func <= grcRSV::CMP_ALWAYS));
	return (D3D11X_TEX_DEPTH_COMPARE)(func-1);
#else
	switch(func)
	{
		case(grcRSV::CMP_NEVER):
			return(D3D11X_TEX_DEPTH_COMPARE_NEVER);
			break;
		case(grcRSV::CMP_LESS):
			return(D3D11X_TEX_DEPTH_COMPARE_LESS);
			break;
		case(grcRSV::CMP_EQUAL):
			return(D3D11X_TEX_DEPTH_COMPARE_EQUAL);
			break;
		case(grcRSV::CMP_LESSEQUAL):
			return(D3D11X_TEX_DEPTH_COMPARE_LESSEQUAL);
			break;
		case(grcRSV::CMP_GREATER):
			return(D3D11X_TEX_DEPTH_COMPARE_GREATER);
			break;
		case(grcRSV::CMP_NOTEQUAL):
			return(D3D11X_TEX_DEPTH_COMPARE_NOTEQUAL);
			break;
		case(grcRSV::CMP_GREATEREQUAL):
			return(D3D11X_TEX_DEPTH_COMPARE_GREATEREQUAL);
			break;
		case(grcRSV::CMP_ALWAYS):
		default:
			return(D3D11X_TEX_DEPTH_COMPARE_ALWAYS);
			break;
	}
#endif
}

static D3D11X_TEX_XY_FILTER ConvertMinFilter(int filter)
{
	if(filter == grcSSV::FILTER_ANISOTROPIC)
		return(D3D11X_TEX_XY_FILTER_ANISO_BILINEAR);

	if(	(filter == grcSSV::FILTER_MIN_MAG_MIP_POINT)				||
		(filter == grcSSV::FILTER_MIN_MAG_POINT_MIP_LINEAR)			||
		(filter == grcSSV::FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT)	||
		(filter == grcSSV::FILTER_MIN_POINT_MAG_MIP_LINEAR)			)
    {
		return D3D11X_TEX_XY_FILTER_POINT;
    }

	return(D3D11X_TEX_XY_FILTER_BILINEAR);
}

static D3D11X_TEX_XY_FILTER ConvertMagFilter(int filter)
{
	if(filter == grcSSV::FILTER_ANISOTROPIC)
		return(D3D11X_TEX_XY_FILTER_ANISO_BILINEAR);

	if(	(filter == grcSSV::FILTER_MIN_MAG_MIP_POINT)				||
		(filter == grcSSV::FILTER_MIN_MAG_POINT_MIP_LINEAR)			||
		(filter == grcSSV::FILTER_MIN_LINEAR_MAG_MIP_POINT)			||
		(filter == grcSSV::FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR)	)
    {
		return D3D11X_TEX_XY_FILTER_POINT;
    }

	return(D3D11X_TEX_XY_FILTER_BILINEAR);
}

static D3D11X_TEX_MIP_FILTER ConvertMipFilter(int filter)
{
	if(	(filter == grcSSV::FILTER_MIN_MAG_MIP_POINT)				||
		(filter == grcSSV::FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT)	||
		(filter == grcSSV::FILTER_MIN_LINEAR_MAG_MIP_POINT)			||
		(filter == grcSSV::FILTER_MIN_MAG_LINEAR_MIP_POINT)			)
    {
		return D3D11X_TEX_MIP_FILTER_POINT;
    }

	return(D3D11X_TEX_MIP_FILTER_LINEAR);
}
#endif //RSG_DURANGO...

ID3D11SamplerState* grcSamplerState::AllocateState(const grcSamplerStateDesc &desc)
{
	ID3D11SamplerState *pState = NULL;

#if RSG_DURANGO
	D3D11X_SAMPLER_DESC samplerDescX;
	sysMemSet(&samplerDescX, 0x00, sizeof(D3D11X_SAMPLER_DESC));

	samplerDescX.ClampX					= ConvertToTexClamp(desc.AddressU);
	samplerDescX.ClampY					= ConvertToTexClamp(desc.AddressV);
	samplerDescX.ClampZ					= ConvertToTexClamp(desc.AddressW);
	samplerDescX.MaxAnisotropicRatio	= ConvertToMaxAnisoRatio(desc.Filter, desc.MaxAnisotropy);
	samplerDescX.DepthCompareFunction	= ConvertToDepthCompare(desc.ComparisonFunc);
	samplerDescX.ForceUnnormalized		= FALSE;
	samplerDescX.AnisotropicThreshold	= 0;
	samplerDescX.MCCoordTrunc			= FALSE;
	samplerDescX.ForceDegamma			= FALSE;
    samplerDescX.AnisotropicBias		= (D3D11X_UNSIGNED_1_5)0;
    samplerDescX.TruncateCoordinates	= FALSE;
    samplerDescX.DisableCubemapWrap		= FALSE;
	samplerDescX.FilterMode				= D3D11X_TEX_FILTER_MODE_LERP;
	samplerDescX.MinLOD					= D3DFloatToUnsigned_4_8(desc.MinLod);
	samplerDescX.MaxLOD					= D3DFloatToUnsigned_4_8(desc.MaxLod);
	samplerDescX.PerfMip				= desc.TrilinearThresh;						// 8
	samplerDescX.PerfZ					= 0;
	samplerDescX.LODBias				= D3DFloatToSigned_5_8(desc.MipLodBias);
	samplerDescX.LODBiasSecondary		= 0;
	samplerDescX.XYMinFilter			= ConvertMinFilter(desc.Filter);
	samplerDescX.XYMagFilter			= ConvertMagFilter(desc.Filter);
	samplerDescX.MipFilter				= ConvertMipFilter(desc.Filter);
	samplerDescX.ZFilter				= D3D11X_TEX_Z_FILTER_NONE;
	samplerDescX.MipPointPreclamp		= FALSE;
	samplerDescX.DisableLSBCeil			= FALSE;
	samplerDescX.FilterPrecisionFix		= FALSE;
	samplerDescX.BorderColorIndex		= 0;
	samplerDescX.BorderColorType		= D3D11X_TEX_BORDER_COLOR_TRANS_BLACK;

	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateSamplerStateX(&samplerDescX,&pState));
#else	// RSG_DURANGO
	PF_AUTO_PUSH_TIMEBAR_BUDGETED("CreateSamplerState", 0.04f);
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateSamplerState((D3D11_SAMPLER_DESC*)&desc,&pState));
#endif	// RSG_DURANGO

	return pState;
}
#endif

#if RSG_DURANGO && _XDK_VER >= 10812
ID3D11RasterizerState* grcRasterizerState::AllocateStateX(const grcRasterizerStateDesc& /*desc*/, ID3D11RasterizerState* /*originalState*/)
{
	return NULL;
}

ID3D11DepthStencilState* grcDepthStencilState::AllocateStateX(const grcDepthStencilStateDesc& desc, ID3D11DepthStencilState* originalState)
{
	D3D11_DEPTH_STENCIL_DESC xdesc;
	originalState->GetDescX(&xdesc);

	xdesc.BackfaceEnable = desc.BackfaceEnable;
	xdesc.DepthBoundsEnable = desc.DepthBoundsEnable;
	xdesc.ColorWritesOnDepthFailEnable = desc.ColorWritesOnDepthFailEnable;
	xdesc.ColorWritesOnDepthPassDisable = desc.ColorWritesOnDepthPassDisable;

//  potentially dangerous extra state removed
//	xdesc.StencilReadMaskBack = desc.StencilReadMaskBack;
//	xdesc.StencilWriteMaskBack = desc.StencilWriteMaskBack;
//	xdesc.StencilTestRefValueFront =desc.StencilTestRefValueFront;
//	xdesc.StencilTestRefValueBack = desc.StencilTestRefValueBack;
//	xdesc.StencilOpRefValueFront = desc.StencilOpRefValueFront;
//	xdesc.StencilOpRefValueBack = desc.StencilOpRefValueBack;

	ID3D11DepthStencilState *pState = NULL;
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateDepthStencilStateX((D3D11_DEPTH_STENCIL_DESC*)&xdesc,&pState));

	return pState;
}

ID3D11BlendState* grcBlendState::AllocateStateX(const grcBlendStateDesc& /*desc*/, ID3D11BlendState* /*originalState*/)
{
	return NULL;
}

ID3D11SamplerState* grcSamplerState::AllocateStateX(const grcSamplerStateDesc& /*desc*/, ID3D11SamplerState* /*originalState*/)
{
	return NULL;
}
#endif

namespace grcStateBlock {

#if !LAZY_STATEBLOCKS
DECLARE_MTR_THREAD bool DSS_Dirty, RS_Dirty, BS_Dirty;
#endif // !LAZY_STATEBLOCKS

grcDepthStencilStateHandle DSS_Default;
grcDepthStencilStateHandle DSS_LessEqual;
grcDepthStencilStateHandle DSS_IgnoreDepth;
grcDepthStencilStateHandle DSS_ForceDepth;
grcDepthStencilStateHandle DSS_TestOnly;
grcDepthStencilStateHandle DSS_TestOnly_LessEqual;
grcRasterizerStateHandle RS_Default;
#if __XENON
grcRasterizerStateHandle RS_Default_HalfPixelOffset;
#endif //__XENON
grcRasterizerStateHandle RS_NoBackfaceCull;
#if (__D3D11 || __GNM) && !__FINAL
grcRasterizerStateHandle RS_WireFrame;
#endif
grcBlendStateHandle BS_Default;
grcBlendStateHandle BS_Default_WriteMaskNone;
grcBlendStateHandle BS_Normal;
grcBlendStateHandle BS_Add;
grcBlendStateHandle BS_Subtract;
grcBlendStateHandle BS_Min;
grcBlendStateHandle BS_Max;
grcBlendStateHandle BS_CompositeAlpha;
grcBlendStateHandle BS_AlphaAdd;
grcBlendStateHandle BS_AlphaSubtract;
grcSamplerStateHandle SS_Default;

DECLARE_MTR_THREAD grcBlendStateHandle BS_Active, BS_Previous;
DECLARE_MTR_THREAD grcDepthStencilStateHandle DSS_Active, DSS_Previous;
DECLARE_MTR_THREAD grcRasterizerStateHandle RS_Active, RS_Previous;
DECLARE_MTR_THREAD u8 ActiveStencilRef, PreviousStencilRef;
DECLARE_MTR_THREAD u32 ActiveBlendFactors, PreviousBlendFactors;
DECLARE_MTR_THREAD u64 ActiveSampleMask, PreviousSampleMask;

template <typename H,class T,class D,class S,typename indexType,u32 maxSize> class grcStateBlockStore
{
	static const u16 MaxRefCount = 65535;
public:
	grcStateBlockStore() {
		CompileTimeAssert(indexType(maxSize) == maxSize);		// Make sure we can represent the sentinel; if this fails, indexType must be made wider.
		FirstUsed = maxSize;
		FirstFree = 0;
		Used = MaxUsed = 0;
		for (u32 i=0; i<maxSize; i++) {
			RefCounts[i] = 0;
			Next[i] = indexType(i+1);
#if __D3D11
			State[i] = NULL;
#endif
		}
	}

	const T& operator[](size_t i) {
		Assert(i != INVALID_STATEBLOCK);
		i--;
		Assert(i < MaxUsed);
		Assert(RefCounts[i]);
		return Store[i];
	}

	H Allocate(const T& data,const D& D3D11_ONLY(desc)) {
		u32 hash = atDataHash((char*)&data, sizeof(data)), i;
		for (i=FirstUsed; i!=maxSize && (Hashes[i] != hash || memcmp(&data,Store+i,sizeof(data))); i = Next[i])
			;
		if (i == maxSize) {
			if (FirstFree == maxSize) {
				Quitf(ERR_GFX_STATE,"Raise grcStateBlockStore size from %u in stateblock.cpp (see call stack for which one)",maxSize);
			}
			else {
				// Pull head entry off the free list, add it to the used list
				i = FirstFree;
				FirstFree = Next[i];
				Next[i] = FirstUsed;
				FirstUsed = indexType(i);

				// Remember the new hash and refcount
				Hashes[i] = hash;
				RefCounts[i] = 1;
				Store[i] = data;
#if RSG_DURANGO && _XDK_VER >= 10812
				AssertMsg(GRCDEVICE.GetCurrent(),"Cannot create state blocks before calling grcDevice::InitClass");
				State[i] = T::AllocateState(desc);
				StateX[i] = T::AllocateStateX(desc, State[i]);
#elif __D3D11
				AssertMsg(GRCDEVICE.GetCurrent(),"Cannot create state blocks before calling grcDevice::InitClass");
				State[i] = T::AllocateState(desc);
#endif

				// Track highest entry ever used (so we know how many to transfer down to SPU)
				if (i+1 > MaxUsed)
					MaxUsed = indexType(i+1);
			}
		}
		else if (RefCounts[i] != MaxRefCount)
			RefCounts[i]++;
		return static_cast<H>(i + 1);
	}

#if __D3D11
	void RecreateState(H i, const D& D3D11_ONLY(desc)) {
		Assert(i != INVALID_STATEBLOCK); 
		i=static_cast<H>(i-1);
		TrapGE(i,MaxUsed);
		Assert(State[i]);
		State[i]->Release();
		AssertMsg(GRCDEVICE.GetCurrent(),"Cannot create state blocks before calling grcDevice::InitClass");
		State[i] = T::AllocateState(desc);
	}

	S* GetState(H i) {
		Assert(i != INVALID_STATEBLOCK); 
		i=static_cast<H>(i-1);
		TrapGE(i,MaxUsed);
		Assert(State[i]);
		return State[i];
	}
#endif

#if RSG_DURANGO && _XDK_VER >= 10812
	S* GetStateX(H i) {
		Assert(i != INVALID_STATEBLOCK); 
		i=static_cast<H>(i-1);
		TrapGE(i,MaxUsed);
		Assert(StateX[i]);
		return StateX[i];
	}
#endif

	void AddRef(H i) {
		Assert(i != INVALID_STATEBLOCK); 
		i=static_cast<H>(i-1);
		Assert(RefCounts[i]);
		if (RefCounts[i] != MaxRefCount)
			++RefCounts[i];
	}

	void Release(H i) {
		Assert(i != INVALID_STATEBLOCK); 
		i=static_cast<H>(i-1);
		Assert(RefCounts[i]);
		if (RefCounts[i] != MaxRefCount && !--RefCounts[i]) {
#if __D3D11
			PF_AUTO_PUSH_TIMEBAR_BUDGETED("State::Release", 0.04f);
			State[i]->Release();
			State[i] = NULL;
#endif
			// Find ourselves on the used list, remove self
			indexType *prev = &FirstUsed;
			while (*prev != i)
				prev = &Next[*prev];	// will crash here if you passed in an invalid or previously freed handle
			*prev = Next[i];
			// Reinsert ourself onto the head of the free list.
			Next[i] = FirstFree;
			FirstFree = indexType(i);
				--Used;
		}
	}

	T *GetStore() { return (T*) ((char*)Store - (sizeof(T))); }	// NOTE THE "-1" EQUIVALENT HERE, avoids -1 on every handle lookup on SPU
#if __D3D11
	S** GetStates() { return State - 1; } // NOTE THE -1 HERE, avoids -1 on every handle lookup on SPU
#endif

	const indexType* GetFreeList() const { return Next; }
	indexType GetFirstUsed() const { return FirstUsed; }

	u32 GetMaxUsed() const { return MaxUsed; }
	u32 GetMaxSize() const { return maxSize; }

private:
	T Store[maxSize];				// the store itself
	u32 Hashes[maxSize];			// hash codes to speed up comparisons against existing types
	u16 RefCounts[maxSize];			// reference counts
#if __D3D11
	S* State[maxSize];
#endif
#if RSG_DURANGO && _XDK_VER >= 10812
	S* StateX[maxSize];
#endif
	indexType Next[maxSize];		// next item in list (every object is either on free list or used list)
	indexType FirstUsed,FirstFree;	// head pointers for each list
	indexType Used, MaxUsed;
};

grcStateBlockStore<grcDepthStencilStateHandle,grcDepthStencilState,grcDepthStencilStateDesc,ID3D11DepthStencilState,u8,144> DepthStencilStateStore;
#if RSG_PC
	grcStateBlockStore<grcRasterizerStateHandle,grcRasterizerState,grcRasterizerStateDesc,ID3D11RasterizerState,u8,128> RasterizerStateStore;
#else
	grcStateBlockStore<grcRasterizerStateHandle,grcRasterizerState,grcRasterizerStateDesc,ID3D11RasterizerState,u8,80> RasterizerStateStore;
#endif
grcStateBlockStore<grcBlendStateHandle,grcBlendState,grcBlendStateDesc,ID3D11BlendState,u8,244> BlendStateStore;
#if RSG_PC
grcStateBlockStore<grcSamplerStateHandle,grcSamplerState,grcSamplerStateDesc,ID3D11SamplerState,u8,80> SamplerStateStore;
#else
grcStateBlockStore<grcSamplerStateHandle,grcSamplerState,grcSamplerStateDesc,ID3D11SamplerState,u8,64> SamplerStateStore;
#endif

void InitClass()
{
	grcDepthStencilStateDesc DefaultDepthStencilStateBlockDesc;
	grcRasterizerStateDesc DefaultRasterizerStateBlockDesc;
	grcBlendStateDesc DefaultBlendStateBlockDesc;
	grcSamplerStateDesc DefaultSamplerStateBlockDesc;

	DefaultDepthStencilStateBlockDesc.DepthFunc = grcRSV::CMP_LESS;
	DSS_Default = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);
	DefaultDepthStencilStateBlockDesc.DepthFunc = grcRSV::CMP_LESSEQUAL;
	DSS_LessEqual = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);
	DefaultDepthStencilStateBlockDesc.DepthWriteMask = false;
	DSS_TestOnly_LessEqual = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);
	DefaultDepthStencilStateBlockDesc.DepthFunc = grcRSV::CMP_LESS;
	DSS_TestOnly = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);
	DefaultDepthStencilStateBlockDesc.DepthFunc = grcRSV::CMP_ALWAYS;
	DefaultDepthStencilStateBlockDesc.DepthEnable = false;
	DSS_IgnoreDepth = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);
	DefaultDepthStencilStateBlockDesc.DepthEnable = true;
	DefaultDepthStencilStateBlockDesc.DepthWriteMask = true;
	DSS_ForceDepth = CreateDepthStencilState(DefaultDepthStencilStateBlockDesc);

	RS_Default = CreateRasterizerState(DefaultRasterizerStateBlockDesc);
	DefaultRasterizerStateBlockDesc.CullMode = grcRSV::CULL_NONE;
	RS_NoBackfaceCull = CreateRasterizerState(DefaultRasterizerStateBlockDesc);

#if __XENON
	grcRasterizerStateDesc DefaultHalfPixelRasterizerStateBlockDesc;
	DefaultHalfPixelRasterizerStateBlockDesc.HalfPixelOffset = true;
	RS_Default_HalfPixelOffset = CreateRasterizerState(DefaultHalfPixelRasterizerStateBlockDesc);
#endif //__XENON

#if (__D3D11 || __GNM) && !__FINAL
	DefaultRasterizerStateBlockDesc.CullMode = grcRSV::CULL_BACK;
	DefaultRasterizerStateBlockDesc.FillMode = grcRSV::FILL_WIREFRAME;
	RS_WireFrame = CreateRasterizerState(DefaultRasterizerStateBlockDesc);
#endif

	BS_Default = CreateBlendState(DefaultBlendStateBlockDesc);
	DefaultBlendStateBlockDesc.BlendRTDesc[0].RenderTargetWriteMask = grcRSV::COLORWRITEENABLE_NONE;
	BS_Default_WriteMaskNone = CreateBlendState(DefaultBlendStateBlockDesc);
	DefaultBlendStateBlockDesc.BlendRTDesc[0].RenderTargetWriteMask = grcRSV::COLORWRITEENABLE_ALL;

	SS_Default = CreateSamplerState(DefaultSamplerStateBlockDesc);

#define setbs(rt,d,s,o) rt.DestBlend = rt.DestBlendAlpha = grcRSV::BLEND_##d; rt.SrcBlend = rt.SrcBlendAlpha = grcRSV::BLEND_##s; rt.BlendOp = rt.BlendOpAlpha = grcRSV::BLENDOP_##o;

	grcBlendStateDesc::grcRenderTargetBlendDesc &rt = DefaultBlendStateBlockDesc.BlendRTDesc[0];
	rt.BlendEnable = true;
	setbs(rt,ONE,ONE,ADD);
	BS_Add = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,INVSRCALPHA, SRCALPHA, ADD);
	BS_Normal = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,ONE,ONE,SUBTRACT);
	BS_Subtract = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,ONE,ONE,MIN);
	BS_Min = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,ONE,ONE,MAX);
	BS_Max = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,INVSRCALPHA,ONE,ADD);
	BS_CompositeAlpha = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,ONE,SRCALPHA,ADD);
	BS_AlphaAdd = CreateBlendState(DefaultBlendStateBlockDesc);
	setbs(rt,ONE,SRCALPHA,REVSUBTRACT);
	BS_AlphaSubtract = CreateBlendState(DefaultBlendStateBlockDesc);

#undef setbs

#if __PPU
	// Tell drawablespu where our data structures live on the PPU.
	s_spuGcmState.DepthStencilStateStore = u32(DepthStencilStateStore.GetStore());
	s_spuGcmState.RasterizerStateStore = u32(RasterizerStateStore.GetStore());
	s_spuGcmState.BlendStateStore = u32(BlendStateStore.GetStore());
	s_spuGcmState.SamplerStateStore = u32(SamplerStateStore.GetStore());

	s_spuGcmState.RS_Default = RS_Default;
	s_spuGcmState.BS_Default = BS_Default;
	s_spuGcmState.DSS_Default = DSS_Default;
	s_spuGcmState.rs_Active = s_spuGcmState.rs_Previous = RS_Invalid;
	s_spuGcmState.dss_Active = s_spuGcmState.dss_Previous = DSS_Invalid;
	s_spuGcmState.bs_Active = s_spuGcmState.bs_Previous = BS_Invalid;
#endif

	g_SamplerStates = SamplerStateStore.GetStore();
#if __D3D11
	g_SamplerStates11 = SamplerStateStore.GetStates();
#endif
}


void ShutdownClass()
{
	DestroyBlendState(BS_Default);
	DestroyRasterizerState(RS_Default);
	DestroyDepthStencilState(DSS_Default);
}

void FrackBlendStates(u32 rand1, u32 rand2)
{
#if RSG_PC && __D3D11
	// Loop over all blend states...
	const u8* indices = BlendStateStore.GetFreeList();
	ID3D11BlendState** ppStates = BlendStateStore.GetStates()+1;
	u32 maxUsed = BlendStateStore.GetMaxUsed();
	if(maxUsed > 110) // Only tamper with the first "few" blend states for more consistant results
		maxUsed = 110;
	u32 targetA = rand1 % maxUsed;
	u32 targetB = rand2 % maxUsed;
	if(targetA > targetB)
	{
		u32 tmp = targetA;
		targetA = targetB;
		targetB = tmp;
	}
	targetB -= targetA;

	ID3D11BlendState** ppSrc = 0;
	for (u32 i=BlendStateStore.GetFirstUsed(); i!=BlendStateStore.GetMaxSize(); i=indices[i])
	{
		// Is this the state we're looking for?
		if(targetA > 0)
		{
			--targetA;
			continue;
		}

		// Found the source blend state?
		if(!ppSrc)
		{
			ppSrc = ppStates+i;
		}

		// Find target blend state
		if(targetB > 0)
		{
			--targetB;
			continue;
		}

		// Swap 'em
		ID3D11BlendState* pTemp = *ppSrc;
		*ppSrc = ppStates[i];
		ppStates[i] = pTemp;
		break;
	}
#else
	(void)rand1;
	(void)rand2;
#endif
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif

grcDepthStencilStateHandle CreateDepthStencilState(const grcDepthStencilStateDesc &desc_,const char* CACHESTATEBLOCKNAMES_D3D11_ONLY(name))
{
	grcDepthStencilStateDesc desc = desc_;

#if TRACK_DEFAULT_DEPTHFUNC
	Assert(desc.DepthFunc != 0xFF);
	if( desc.DepthFunc == 0xFF)
	{
		desc.DepthFunc = grcRSV::CMP_LESS;
	}
#endif // TRACK_DEFAULT_DEPTHFUNC

#if SUPPORT_INVERTED_VIEWPORT
	using namespace grcRSV;
#if __D3D11
	static u8 reverseDirection[] = { 0, // padded by one 
#else	
	static u8 reverseDirection[] = {
#endif
		grcRSV::CMP_NEVER, grcRSV::CMP_GREATER, grcRSV::CMP_EQUAL, grcRSV::CMP_GREATEREQUAL, grcRSV::CMP_LESS,		grcRSV::CMP_NOTEQUAL, grcRSV::CMP_LESSEQUAL,	grcRSV::CMP_ALWAYS 
	};
	desc.DepthFunc = reverseDirection[desc.DepthFunc];
#endif // SUPPORT_INVERTED_VIEWPORT


	grcDepthStencilState state;
	//Initializing to zero to zero out padding also
	sysMemSet(&state, 0, sizeof(state));
	// We can't use the Assign macros here because we intentionally truncate bits on PS3
	state.DepthEnable=desc.DepthEnable;
	state.DepthWriteMask=desc.DepthWriteMask;
	// Force the depth function to "always" if it's disabled
	state.DepthFunc=desc.DepthEnable?desc.DepthFunc:grcRSV::CMP_ALWAYS;
	state.StencilEnable=desc.StencilEnable;
	if (state.StencilEnable) {
		state.StencilReadMask=desc.StencilReadMask;
		state.StencilWriteMask=desc.StencilWriteMask;
		state.FrontStencilFailOp=desc.FrontFace.StencilFailOp;
		state.FrontStencilDepthFailOp=desc.FrontFace.StencilDepthFailOp;
		state.FrontStencilPassOp=desc.FrontFace.StencilPassOp;
		state.FrontStencilFunc=desc.FrontFace.StencilFunc;
		state.BackStencilFailOp=desc.BackFace.StencilFailOp;
		state.BackStencilDepthFailOp=desc.BackFace.StencilDepthFailOp;
		state.BackStencilPassOp=desc.BackFace.StencilPassOp;
		state.BackStencilFunc=desc.BackFace.StencilFunc;
	}
	else {
		state.StencilReadMask=0xFF;
		state.StencilWriteMask=0xFF;
		state.FrontStencilFailOp=grcRSV::STENCILOP_KEEP;
		state.FrontStencilDepthFailOp=grcRSV::STENCILOP_KEEP;
		state.FrontStencilPassOp=grcRSV::STENCILOP_KEEP;
		state.FrontStencilFunc=grcRSV::CMP_ALWAYS;
		state.BackStencilFailOp=grcRSV::STENCILOP_KEEP;
		state.BackStencilDepthFailOp=grcRSV::STENCILOP_KEEP;
		state.BackStencilPassOp=grcRSV::STENCILOP_KEEP;
		state.BackStencilFunc=grcRSV::CMP_ALWAYS;
	}

	// Figure this out now since current API's have it on a separate enable.
	state.TwoSidedStencil = memcmp(&desc.FrontFace,&desc.BackFace,sizeof(desc.FrontFace)) != 0;

#if RSG_ORBIS
	state.dsc.init();
	state.dsc.setDepthEnable(state.DepthEnable);
	state.dsc.setDepthControl((sce::Gnm::DepthControlZWrite)state.DepthWriteMask,(sce::Gnm::CompareFunc)state.DepthFunc);
	state.dsc.setSeparateStencilEnable(true);	// could use state.TwoSidedStencil here, not sure if that would really matter.
	state.dsc.setStencilEnable(state.StencilEnable);
	state.dsc.setStencilFunction((sce::Gnm::CompareFunc)state.FrontStencilFunc);
	state.dsc.setStencilFunctionBack((sce::Gnm::CompareFunc)state.BackStencilFunc);
	state.dsc.setDepthBoundsEnable(desc.DepthBoundsEnable);
	state.sc.init();
	state.sc.m_mask = state.StencilReadMask;
	state.sc.m_writeMask = state.StencilWriteMask;
	state.sc.m_opVal = 1;		// Could provide external access to this, but hardware only supports Add/Sub, not Inc/Dec
	state.soc.init();
	state.soc.setStencilOps((sce::Gnm::StencilOp)state.FrontStencilFailOp,(sce::Gnm::StencilOp)state.FrontStencilPassOp,(sce::Gnm::StencilOp)state.FrontStencilDepthFailOp);
	state.soc.setStencilOpsBack((sce::Gnm::StencilOp)state.BackStencilFailOp,(sce::Gnm::StencilOp)state.BackStencilPassOp,(sce::Gnm::StencilOp)state.BackStencilDepthFailOp);

#endif
#if TRACK_DEPTH_BOUNDS_STATE
	state.DepthBoundsEnable = desc.DepthBoundsEnable;
#endif

	grcDepthStencilStateHandle result = DepthStencilStateStore.Allocate(state,desc);
#if CACHESTATEBLOCKNAMES_D3D11
	SetName(result,name);
#endif

#if __ASSERT
	grcDepthStencilStateDesc test;
	GetDepthStencilStateDesc(result, test);
	Assert(desc_.DepthEnable == test.DepthEnable);
	Assert(desc_.DepthWriteMask == test.DepthWriteMask);
	Assert(desc_.StencilEnable == test.StencilEnable);
#if !TRACK_DEFAULT_DEPTHFUNC
	Assert(!desc_.DepthEnable || desc_.DepthFunc == test.DepthFunc);
#endif // !TRACK_DEFAULT_DEPTHFUNC
	if (desc_.StencilEnable) {
		Assert(desc_.StencilReadMask == test.StencilReadMask);
		Assert(desc_.StencilWriteMask == test.StencilWriteMask);
		Assert(desc_.FrontFace.StencilFailOp == test.FrontFace.StencilFailOp);
		Assert(desc_.FrontFace.StencilDepthFailOp == test.FrontFace.StencilDepthFailOp);
		Assert(desc_.FrontFace.StencilPassOp == test.FrontFace.StencilPassOp);
		Assert(desc_.FrontFace.StencilFunc == test.FrontFace.StencilFunc);
		Assert(desc_.BackFace.StencilFailOp == test.BackFace.StencilFailOp);
		Assert(desc_.BackFace.StencilDepthFailOp == test.BackFace.StencilDepthFailOp);
		Assert(desc_.BackFace.StencilPassOp == test.BackFace.StencilPassOp);
		Assert(desc_.BackFace.StencilFunc == test.BackFace.StencilFunc);
	}
#endif
	return result;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void GetDepthStencilStateDesc(grcDepthStencilStateHandle handle,grcDepthStencilStateDesc& outDesc)
{
	const grcDepthStencilState &state = DepthStencilStateStore[handle];
	outDesc.DepthEnable = state.DepthEnable;
	outDesc.DepthWriteMask = state.DepthWriteMask;
	outDesc.StencilEnable = state.StencilEnable;
#if SUPPORT_INVERTED_VIEWPORT
	using namespace grcRSV;
#if __D3D11
	static u8 reverseDirection[] = { 0, // padded by one 
#else
	static u8 reverseDirection[] = {
#endif
		grcRSV::CMP_NEVER, grcRSV::CMP_GREATER, grcRSV::CMP_EQUAL, grcRSV::CMP_GREATEREQUAL, grcRSV::CMP_LESS,		grcRSV::CMP_NOTEQUAL, grcRSV::CMP_LESSEQUAL,	grcRSV::CMP_ALWAYS 
	};
	outDesc.DepthFunc = reverseDirection[state.DepthFunc];
#else
	outDesc.DepthFunc = state.DepthFunc PS3_ONLY(| 0x200);
#endif // SUPPORT_INVERTED_VIEWPORT
	outDesc.StencilReadMask = state.StencilReadMask;
	outDesc.StencilWriteMask = state.StencilWriteMask;
	outDesc.FrontFace.StencilFailOp = state.FrontStencilFailOp;
	outDesc.FrontFace.StencilDepthFailOp = state.FrontStencilDepthFailOp;
	outDesc.FrontFace.StencilPassOp = state.FrontStencilPassOp;
	outDesc.FrontFace.StencilFunc = state.FrontStencilFunc PS3_ONLY(| 0x200);
	outDesc.BackFace.StencilFailOp = state.BackStencilFailOp;
	outDesc.BackFace.StencilDepthFailOp = state.BackStencilDepthFailOp;
	outDesc.BackFace.StencilPassOp = state.BackStencilPassOp;
	outDesc.BackFace.StencilFunc = state.BackStencilFunc PS3_ONLY(| 0x200);
#if TRACK_DEPTH_BOUNDS_STATE
	outDesc.DepthBoundsEnable = state.DepthBoundsEnable;
#endif
}

void DestroyDepthStencilState(grcDepthStencilStateHandle handle)
{
	DepthStencilStateStore.Release(handle);
}

grcRasterizerStateHandle CreateRasterizerState(const grcRasterizerStateDesc &desc_,const char* CACHESTATEBLOCKNAMES_D3D11_ONLY(name))
{
	grcRasterizerState state;
	AssertMsg(desc_.FrontCounterClockwise,"RAGE requires front be CW for DX9 interoperability");
	AssertMsg(desc_.ScissorEnable,"RAGE requires scissor test to always be enabled");

	grcRasterizerStateDesc desc = desc_;
#if SUPPORT_INVERTED_VIEWPORT
	desc.SlopeScaledDepthBias = -desc.SlopeScaledDepthBias;
	desc.DepthBiasClamp = -desc.DepthBiasClamp;
	desc.DepthBiasDX9 = -desc.DepthBiasDX9;
#endif // SUPPORT_INVERTED_VIEWPORT 


	//Initializing to zero to zero out padding also
	sysMemSet(&state, 0, sizeof(state));
#if __WIN32
	Assign(state.FillMode,desc.FillMode);
	Assign(state.CullMode,desc.CullMode);
	state.DepthBias.f = desc.DepthBiasDX9;
#if __D3D11
	if (!desc.DepthBiasDX10)
	{
		//On DX11 the depth bias needs to be scaled by the 2 ^ depth buffer bit depth
		// DX11 TODO:- Check this is the correct way for depth bias in DX11, this is how we did it on LAN appears to work correctly.
		const u32 DepthBufferBitDepth = 24;
		float ValueScaledToDepthBufferRange = desc.DepthBiasDX9 * ( (0x1 << DepthBufferBitDepth) - 1 );
		state.DepthBias.u = (unsigned)ValueScaledToDepthBufferRange;
		desc.DepthBiasDX10 = state.DepthBias.u;
	}
#endif
	Assign(state.SlopeScaledDepthBias.f,desc.SlopeScaledDepthBias);
	Assign(state.DepthBiasClamp, desc.DepthBiasClamp);
	BitfieldAssign(state.MultisampleEnable,desc.MultisampleEnable);
	BitfieldAssign(state.HalfPixelOffset,desc.HalfPixelOffset);
	state.pad = 0;
#elif __PS3
	using namespace grcRSV;
	Assign(state.FillMode,desc.FillMode);
	BitfieldAssign(state.CullFaceEnable,desc.CullMode != CULL_NONE);
	BitfieldAssign(state.HalfPixelOffset,desc.HalfPixelOffset);
	Assign(state.CullFace,desc.CullMode == CULL_CCW?CELL_GCM_FRONT:CELL_GCM_BACK);
	Assign(state.EdgeCullMode,desc.CullMode==CULL_CW?EDGE_GEOM_CULL_BACKFACES_AND_FRUSTUM:desc.CullMode==CULL_CCW?EDGE_GEOM_CULL_FRONTFACES_AND_FRUSTUM:EDGE_GEOM_CULL_FRUSTUM);
	if (desc.DepthBiasDX9 || desc.SlopeScaledDepthBias) {
		state.PolygonOffsetFillEnable = true;
		state.Offset0.f = desc.SlopeScaledDepthBias;
		state.Offset1.f = desc.DepthBiasDX9 * 65536.0f;
	}
	else {
		state.PolygonOffsetFillEnable = false;
		state.Offset0.u = state.Offset1.u = 0;
	}
	state.pad = 0;
#elif RSG_ORBIS
	Assign(state.FillMode,desc.FillMode);
	Assign(state.CullMode,desc.CullMode);
	state.DepthBias.f = desc.DepthBiasDX9;
	Assign(state.SlopeScaledDepthBias.f,desc.SlopeScaledDepthBias);
	Assign(state.DepthBiasClamp, desc.DepthBiasClamp);
	BitfieldAssign(state.MultisampleEnable,desc.MultisampleEnable);
	state.ps.init();
	state.ps.setCullFace(desc.CullMode == grcRSV::CULL_NONE? sce::Gnm::kPrimitiveSetupCullFaceNone : desc.CullMode == grcRSV::CULL_BACK? sce::Gnm::kPrimitiveSetupCullFaceBack : sce::Gnm::kPrimitiveSetupCullFaceFront);
	state.ps.setFrontFace(desc.FrontCounterClockwise ? sce::Gnm::kPrimitiveSetupFrontFaceCcw : sce::Gnm::kPrimitiveSetupFrontFaceCw);
	state.ps.setPolygonOffsetEnable(sce::Gnm::kPrimitiveSetupPolygonOffsetEnable, sce::Gnm::kPrimitiveSetupPolygonOffsetEnable);
	sce::Gnm::PrimitiveSetupPolygonMode fillMode = static_cast<sce::Gnm::PrimitiveSetupPolygonMode>(desc.FillMode);
	state.ps.setPolygonMode(fillMode, fillMode);

#endif

	grcRasterizerStateHandle result	= RasterizerStateStore.Allocate(state,desc);

#if CACHESTATEBLOCKNAMES_D3D11
	SetName(result,name);
#endif

#if __ASSERT
	grcRasterizerStateDesc test;
	GetRasterizerStateDesc(result,test);
	Assert(desc_.FillMode == test.FillMode);
	Assert(desc_.CullMode == test.CullMode);
	//Assert(desc_.DepthBiasDX9 == test.DepthBiasDX9); // Depth bias will never match because of depthBiasDX10...
	Assert(desc_.SlopeScaledDepthBias == test.SlopeScaledDepthBias);
	Assert(desc_.MultisampleEnable == test.MultisampleEnable);
#endif
	return result;
}

void GetRasterizerStateDesc(grcRasterizerStateHandle handle,grcRasterizerStateDesc& outDesc)
{
	const grcRasterizerState &state = RasterizerStateStore[handle];
#if __WIN32 || RSG_ORBIS
	outDesc.FillMode = state.FillMode;
	outDesc.CullMode = state.CullMode;
	outDesc.DepthBiasDX9 = state.DepthBias.f;
#if __D3D11
	outDesc.DepthBiasDX10 = state.DepthBias.u;
#endif
	outDesc.SlopeScaledDepthBias = state.SlopeScaledDepthBias.f;
	outDesc.DepthBiasClamp = state.DepthBiasClamp;
	outDesc.MultisampleEnable = state.MultisampleEnable;
	outDesc.HalfPixelOffset = state.HalfPixelOffset;

#if SUPPORT_INVERTED_VIEWPORT
	outDesc.SlopeScaledDepthBias = -outDesc.SlopeScaledDepthBias;
	outDesc.DepthBiasClamp = -outDesc.DepthBiasClamp;
	outDesc.DepthBiasDX9 = -outDesc.DepthBiasDX9;
#endif // SUPPORT_INVERTED_VIEWPORT 

#elif __PS3
	using namespace grcRSV;
	outDesc.FillMode = state.FillMode;
	outDesc.CullMode = (state.CullFaceEnable? (state.CullFace==CELL_GCM_FRONT?CULL_CCW:CULL_CW) : CULL_NONE);
	outDesc.SlopeScaledDepthBias = state.Offset0.f;
	outDesc.DepthBiasDX9 = (state.Offset1.f / 65536.0f);
	outDesc.HalfPixelOffset = state.HalfPixelOffset;
#endif
}

void DestroyRasterizerState(grcRasterizerStateHandle handle)
{
	RasterizerStateStore.Release(handle);
}

#if __PS3
bool anyDifferent(int a,int b,int c,int d) { return a!=b || a!=c || a!=d; }
#endif


grcBlendStateHandle CreateBlendState(const grcBlendStateDesc &_desc,const char* CACHESTATEBLOCKNAMES_D3D11_ONLY(name))
{
	// Durango overloads AlphaToCoverageEnable in order to implement AlphaToMaskOffsets. See below where we set AlphaToCoverageEnable. In order
	// to hide this mess from higher level code, we fix it up here by forcing a grcBlendStateDesc copy & patch on Durango (thus we need a const
	// copy of the struct on Durango, this stays a ref on everything else).
#if RSG_DURANGO
	grcBlendStateDesc desc = _desc; // make a non-const local copy of the struct 
#else
	const grcBlendStateDesc &desc = _desc; // should NOP
#endif

	using namespace grcRSV;
	grcBlendState state;
	//Initializing to zero to zero out padding also
	sysMemSet(&state, 0, sizeof(state));
#if __ASSERT
	if (!desc.IndependentBlendEnable) {
		// Make sure nothing was modified from default if this flag is off, since that's likely user error.
		for (int i=1; i<=3; i++) {
			const grcBlendStateDesc::grcRenderTargetBlendDesc &thisRT = desc.BlendRTDesc[i];
			Assertf(thisRT.BlendEnable == false && thisRT.SrcBlend == BLEND_ONE && thisRT.DestBlend == BLEND_ZERO &&
				thisRT.BlendOp == BLENDOP_ADD && thisRT.SrcBlendAlpha == BLEND_ONE && thisRT.DestBlendAlpha == BLEND_ZERO &&
				thisRT.BlendOpAlpha == BLENDOP_ADD && thisRT.RenderTargetWriteMask == 15,"IndependentBlendEnable is false but target %d has non-default settings",i);
		}
	}
#endif
#if __XENON
	state.IndependentBlendEnable = desc.IndependentBlendEnable != 0;
	for (int d=0; d<4; d++) {
		int s=desc.IndependentBlendEnable? d : 0;
		if (desc.BlendRTDesc[s].BlendEnable) {
			BitfieldAssign(state.Targets[d].SrcBlend,desc.BlendRTDesc[s].SrcBlend);
			BitfieldAssign(state.Targets[d].DestBlend,desc.BlendRTDesc[s].DestBlend);
			BitfieldAssign(state.Targets[d].BlendOp,desc.BlendRTDesc[s].BlendOp);
			BitfieldAssign(state.Targets[d].SrcBlendAlpha,desc.BlendRTDesc[s].SrcBlendAlpha);
			BitfieldAssign(state.Targets[d].DestBlendAlpha,desc.BlendRTDesc[s].DestBlendAlpha);
			BitfieldAssign(state.Targets[d].BlendOpAlpha,desc.BlendRTDesc[s].BlendOpAlpha);
		}
		else {
			state.Targets[d].SrcBlend = D3DBLEND_ONE;
			state.Targets[d].DestBlend = D3DBLEND_ZERO;
			state.Targets[d].BlendOp = D3DBLENDOP_ADD;
			state.Targets[d].SrcBlendAlpha = D3DBLEND_ONE;
			state.Targets[d].DestBlendAlpha = D3DBLEND_ZERO;
			state.Targets[d].BlendOpAlpha = D3DBLENDOP_ADD;
		}
		Assign(state.WriteMasks[d],desc.BlendRTDesc[s].RenderTargetWriteMask);
	}
	if (state.IndependentBlendEnable) {
		state.Enable0 = desc.BlendRTDesc[0].BlendEnable != 0;
		state.Enable1 = desc.BlendRTDesc[1].BlendEnable != 0;
		state.Enable2 = desc.BlendRTDesc[2].BlendEnable != 0;
		state.Enable3 = desc.BlendRTDesc[3].BlendEnable != 0;
	}
	else
		state.Enable0 = state.Enable1 = state.Enable2 = state.Enable3 = desc.BlendRTDesc[0].BlendEnable != 0;
	Assign(state.AlphaToMaskOffsets,desc.AlphaToMaskOffsets);
	BitfieldAssign(state.AlphaToCoverageEnable,desc.AlphaToCoverageEnable);
	state.HighPrecisionBlendEnable = state.Targets[0].SrcBlend != D3DBLEND_ONE || state.Targets[0].SrcBlendAlpha != D3DBLEND_ONE || state.Targets[0].BlendOp != D3DBLENDOP_ADD ||
		state.Targets[0].DestBlend != state.Targets[0].DestBlendAlpha || (state.Targets[0].DestBlend != D3DBLEND_ZERO && state.Targets[0].DestBlend != D3DBLEND_ONE);

#elif __PS3
	bool independent = desc.IndependentBlendEnable;
#if __ASSERT
	if (independent) {
		// On PS3, you can individually enable/disable blending on a per MRT basis,
		// and you can also change the write mask on a per MRT basis, but you cannot change
		// the blend function in any way.  This doesn't test every last permutation.
		for (int i=1; i<4; i++) {
			if (desc.BlendRTDesc[0].BlendEnable && desc.BlendRTDesc[i].BlendEnable) {
				Assert(desc.BlendRTDesc[0].SrcBlend == desc.BlendRTDesc[i].SrcBlend);
				Assert(desc.BlendRTDesc[0].DestBlend == desc.BlendRTDesc[i].DestBlend);
				Assert(desc.BlendRTDesc[0].BlendOp == desc.BlendRTDesc[i].BlendOp);
				Assert(desc.BlendRTDesc[0].SrcBlendAlpha == desc.BlendRTDesc[i].SrcBlendAlpha);
				Assert(desc.BlendRTDesc[0].DestBlendAlpha == desc.BlendRTDesc[i].DestBlendAlpha);
				Assert(desc.BlendRTDesc[0].BlendOpAlpha == desc.BlendRTDesc[i].BlendOpAlpha);
			}
		}
	}
#endif
	state.Enable0 = desc.BlendRTDesc[0].BlendEnable;
	state.Enable123 = desc.BlendRTDesc[independent? 1 : 0].BlendEnable |
		(desc.BlendRTDesc[independent? 2 : 0].BlendEnable << 1) |
		(desc.BlendRTDesc[independent? 3 : 0].BlendEnable << 2);
	if (state.Enable0 | state.Enable123) {
		state.BlendOp = desc.BlendRTDesc[0].BlendOp & 0xF;	// must or in 0x8000 for this to actually be valid.
		state.BlendOpAlpha = desc.BlendRTDesc[0].BlendOpAlpha & 0xF; // must or in 0x8000 for this to actually be valid.
		state.SrcBlend = desc.BlendRTDesc[0].SrcBlend | (desc.BlendRTDesc[0].SrcBlendAlpha << 16);
		state.DestBlend = desc.BlendRTDesc[0].DestBlend | (desc.BlendRTDesc[0].DestBlendAlpha << 16);
	}
	else {
		state.BlendOp = grcRSV::BLENDOP_ADD & 0xF;	// must or in 0x8000 for this to actually be valid.
		state.BlendOpAlpha = grcRSV::BLENDOP_ADD & 0xF; // must or in 0x8000 for this to actually be valid.
		state.SrcBlend = grcRSV::BLEND_ONE | (grcRSV::BLEND_ONE << 16);
		state.DestBlend = grcRSV::BLEND_ZERO | (grcRSV::BLEND_ZERO << 16);
	}

	u8 mask0 = desc.BlendRTDesc[0].RenderTargetWriteMask;
	state.WriteMask0 = 
		((mask0 & COLORWRITEENABLE_RED)? CELL_GCM_COLOR_MASK_R : 0) |
		((mask0 & COLORWRITEENABLE_GREEN)? CELL_GCM_COLOR_MASK_G : 0) |
		((mask0 & COLORWRITEENABLE_BLUE)? CELL_GCM_COLOR_MASK_B : 0) |
		((mask0 & COLORWRITEENABLE_ALPHA)? CELL_GCM_COLOR_MASK_A : 0) |
		(desc.AlphaToCoverageEnable? 16 : 0);	// Put in correct bit position already
	u8 mask1 = desc.BlendRTDesc[independent? 1 : 0].RenderTargetWriteMask;
	u8 mask2 = desc.BlendRTDesc[independent? 2 : 0].RenderTargetWriteMask;
	u8 mask3 = desc.BlendRTDesc[independent? 3 : 0].RenderTargetWriteMask;
	state.WriteMask123 = 
		mask0 |			// Store a copy here so it's easier to retrieve
		((mask1 & COLORWRITEENABLE_RED)? CELL_GCM_COLOR_MASK_MRT1_R : 0) |
		((mask1 & COLORWRITEENABLE_GREEN)? CELL_GCM_COLOR_MASK_MRT1_G : 0) |
		((mask1 & COLORWRITEENABLE_BLUE)? CELL_GCM_COLOR_MASK_MRT1_B : 0) |
		((mask1 & COLORWRITEENABLE_ALPHA)? CELL_GCM_COLOR_MASK_MRT1_A : 0) |
		((mask2 & COLORWRITEENABLE_RED)? CELL_GCM_COLOR_MASK_MRT2_R : 0) |
		((mask2 & COLORWRITEENABLE_GREEN)? CELL_GCM_COLOR_MASK_MRT2_G : 0) |
		((mask2 & COLORWRITEENABLE_BLUE)? CELL_GCM_COLOR_MASK_MRT2_B : 0) |
		((mask2 & COLORWRITEENABLE_ALPHA)? CELL_GCM_COLOR_MASK_MRT2_A : 0) |
		((mask3 & COLORWRITEENABLE_RED)? CELL_GCM_COLOR_MASK_MRT3_R : 0) |
		((mask3 & COLORWRITEENABLE_GREEN)? CELL_GCM_COLOR_MASK_MRT3_G : 0) |
		((mask3 & COLORWRITEENABLE_BLUE)? CELL_GCM_COLOR_MASK_MRT3_B : 0) |
		((mask3 & COLORWRITEENABLE_ALPHA)? CELL_GCM_COLOR_MASK_MRT3_A : 0);
#elif RSG_PC || RSG_DURANGO || RSG_ORBIS
	ORBIS_ONLY(state.ColorWriteMask = 0);
	for (int i=0; i<8; i++) {
		const grcBlendStateDesc::grcRenderTargetBlendDesc &target(desc.BlendRTDesc[desc.IndependentBlendEnable? i : 0]);
		Assign(state.Targets[i].BlendEnable,target.BlendEnable);
		if (target.BlendEnable) {
			Assign(state.Targets[i].SrcBlend,target.SrcBlend);
			Assign(state.Targets[i].DestBlend,target.DestBlend);
			Assign(state.Targets[i].BlendOp,target.BlendOp);
			Assign(state.Targets[i].SrcBlendAlpha,target.SrcBlendAlpha);
			Assign(state.Targets[i].DestBlendAlpha,target.DestBlendAlpha);
			Assign(state.Targets[i].BlendOpAlpha,target.BlendOpAlpha);
		}
		else {
			Assign(state.Targets[i].SrcBlend,grcRSV::BLEND_ONE);
			Assign(state.Targets[i].DestBlend,grcRSV::BLEND_ZERO);
			Assign(state.Targets[i].BlendOp,grcRSV::BLENDOP_ADD);
			Assign(state.Targets[i].SrcBlendAlpha,grcRSV::BLEND_ONE);
			Assign(state.Targets[i].DestBlendAlpha,grcRSV::BLENDOP_ADD);
			Assign(state.Targets[i].BlendOpAlpha,grcRSV::BLEND_ZERO);
		}
# if RSG_ORBIS
		state.ColorWriteMask |= (target.RenderTargetWriteMask << (i * 4));
		Assign(state.AlphaToMaskOffsets,desc.AlphaToMaskOffsets);
		Assign(state.AlphaToCoverageEnable,desc.AlphaToCoverageEnable);
		state.bc[i].init();
		state.bc[i].setBlendEnable(target.BlendEnable);
		state.bc[i].setSeparateAlphaEnable(true);
		state.bc[i].setColorEquation((sce::Gnm::BlendMultiplier)state.Targets[i].SrcBlend,(sce::Gnm::BlendFunc)state.Targets[i].BlendOp,(sce::Gnm::BlendMultiplier)state.Targets[i].DestBlend);
		state.bc[i].setAlphaEquation((sce::Gnm::BlendMultiplier)state.Targets[i].SrcBlendAlpha,(sce::Gnm::BlendFunc)state.Targets[i].BlendOpAlpha,(sce::Gnm::BlendMultiplier)state.Targets[i].DestBlendAlpha);
# else
		Assign(state.Targets[i].RenderTargetWriteMask,target.RenderTargetWriteMask);
# endif
	}
	Assign(state.IndependentBlendEnable,desc.IndependentBlendEnable);
# if __WIN32
	
#if RSG_DURANGO
	// Durango exposes AlphaToMaskOffsets by taking special flags for AlphaToCoverageEnable (0=off, 1=dither, 2=solid). We have to patch up the desc
	// here to setup the overloaded AlphaToCoverageEnable parameter. Both the state and desc params must be patched because both are used by BlendStateStore.Allocate()
	desc.AlphaToCoverageEnable = desc.AlphaToCoverageEnable ? ( desc.AlphaToMaskOffsets == grcRSV::ALPHATOMASKOFFSETS_SOLID ? 2 : 1 ) : 0;
	Assign(state.AlphaToCoverageEnable,desc.AlphaToCoverageEnable);
#else
	Assign(state.AlphaToCoverageEnable,desc.AlphaToCoverageEnable);
#endif
	Assign(state.AlphaToCoverageEnable,desc.AlphaToCoverageEnable);
# endif
#endif

	grcBlendStateHandle result = BlendStateStore.Allocate(state,desc);

#if CACHESTATEBLOCKNAMES_D3D11
	SetName(result,name);
#endif

#if __ASSERT
	grcBlendStateDesc test;
	GetBlendStateDesc(result, test);
#if !RSG_ORBIS
	Assert(desc.AlphaToCoverageEnable == test.AlphaToCoverageEnable);
#endif // !RSG_ORBIS
	AssertMsg(desc.IndependentBlendEnable == test.IndependentBlendEnable,"You probably set IndependentBlendEnable when you didn't need to (all four targets have the same settings)");

	for (int i=0; i<MAX_RAGE_RENDERTARGET_COUNT; i++) {
		const grcBlendStateDesc::grcRenderTargetBlendDesc &desc_(desc.BlendRTDesc[desc.IndependentBlendEnable?i:0]), &test_(test.BlendRTDesc[test.IndependentBlendEnable?i:0]);
		Assert(desc_.BlendEnable == test_.BlendEnable);
		// If blending is enabled, make sure the fields match.  Otherwise it doesn't really matter.
		if (desc_.BlendEnable) {
			Assert(desc_.SrcBlend == test_.SrcBlend);
			Assert(desc_.DestBlend == test_.DestBlend);
			Assert(desc_.BlendOp == test_.BlendOp);
			Assert(desc_.SrcBlendAlpha == test_.SrcBlendAlpha);
			Assert(desc_.DestBlendAlpha == test_.DestBlendAlpha);
			Assert(desc_.BlendOpAlpha == test_.BlendOpAlpha);
			Assert(desc_.RenderTargetWriteMask == test_.RenderTargetWriteMask);
		}
	}
	XENON_ONLY(Assert(desc.AlphaToMaskOffsets == test.AlphaToMaskOffsets));
#endif
	return result;
}

void GetBlendStateDesc(grcBlendStateHandle handle,grcBlendStateDesc& outDesc)
{
	const grcBlendState &state = BlendStateStore[handle];
#if __XENON
	outDesc.AlphaToCoverageEnable = state.AlphaToCoverageEnable;
	outDesc.IndependentBlendEnable = state.IndependentBlendEnable;
	for (int i=0; i<4; i++) {
		outDesc.BlendRTDesc[i].SrcBlend = state.Targets[i].SrcBlend;
		outDesc.BlendRTDesc[i].DestBlend = state.Targets[i].DestBlend;
		outDesc.BlendRTDesc[i].BlendOp = state.Targets[i].BlendOp;
		outDesc.BlendRTDesc[i].SrcBlendAlpha = state.Targets[i].SrcBlendAlpha;
		outDesc.BlendRTDesc[i].DestBlendAlpha = state.Targets[i].DestBlendAlpha;
		outDesc.BlendRTDesc[i].BlendOpAlpha = state.Targets[i].BlendOpAlpha;
		outDesc.BlendRTDesc[i].RenderTargetWriteMask = state.WriteMasks[i];
	}
	outDesc.BlendRTDesc[0].BlendEnable = state.Enable0;
	outDesc.BlendRTDesc[1].BlendEnable = state.Enable1;
	outDesc.BlendRTDesc[2].BlendEnable = state.Enable2;
	outDesc.BlendRTDesc[3].BlendEnable = state.Enable3;
	outDesc.AlphaToMaskOffsets = state.AlphaToMaskOffsets;
#elif __PS3
	using namespace grcRSV;
	outDesc.AlphaToCoverageEnable = (state.WriteMask0 & 16) != 0;
	// outDesc.IndependentBlendEnable = ... figure out later
	outDesc.BlendRTDesc[0].BlendEnable = state.Enable0;
	outDesc.BlendRTDesc[1].BlendEnable = (state.Enable123 & 1) != 0;
	outDesc.BlendRTDesc[2].BlendEnable = (state.Enable123 & 2) != 0;
	outDesc.BlendRTDesc[3].BlendEnable = (state.Enable123 & 4) != 0;
	for (int i=0; i<4; i++) {
		outDesc.BlendRTDesc[i].BlendOp = state.BlendOp | 0x8000;
		outDesc.BlendRTDesc[i].BlendOpAlpha = state.BlendOpAlpha | 0x8000;
		outDesc.BlendRTDesc[i].SrcBlend = state.SrcBlend & 0xFFFF;
		outDesc.BlendRTDesc[i].SrcBlendAlpha = state.SrcBlend >> 16;
		outDesc.BlendRTDesc[i].DestBlend = state.DestBlend & 0xFFFF;
		outDesc.BlendRTDesc[i].DestBlendAlpha = state.DestBlend >> 16;
	}
	outDesc.BlendRTDesc[0].RenderTargetWriteMask =
		((state.WriteMask0 & CELL_GCM_COLOR_MASK_R)? COLORWRITEENABLE_RED : 0) |
		((state.WriteMask0 & CELL_GCM_COLOR_MASK_G)? COLORWRITEENABLE_GREEN : 0) |
		((state.WriteMask0 & CELL_GCM_COLOR_MASK_B)? COLORWRITEENABLE_BLUE : 0) |
		((state.WriteMask0 & CELL_GCM_COLOR_MASK_A)? COLORWRITEENABLE_ALPHA : 0);
	outDesc.BlendRTDesc[1].RenderTargetWriteMask =
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT1_R)? COLORWRITEENABLE_RED : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT1_G)? COLORWRITEENABLE_GREEN : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT1_B)? COLORWRITEENABLE_BLUE : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT1_A)? COLORWRITEENABLE_ALPHA : 0);
	outDesc.BlendRTDesc[2].RenderTargetWriteMask =
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT2_R)? COLORWRITEENABLE_RED : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT2_G)? COLORWRITEENABLE_GREEN : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT2_B)? COLORWRITEENABLE_BLUE : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT2_A)? COLORWRITEENABLE_ALPHA : 0);
	outDesc.BlendRTDesc[3].RenderTargetWriteMask =
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT3_R)? COLORWRITEENABLE_RED : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT3_G)? COLORWRITEENABLE_GREEN : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT3_B)? COLORWRITEENABLE_BLUE : 0) |
		((state.WriteMask123 & CELL_GCM_COLOR_MASK_MRT3_A)? COLORWRITEENABLE_ALPHA : 0);
	outDesc.IndependentBlendEnable = 
		anyDifferent(outDesc.BlendRTDesc[0].BlendEnable,outDesc.BlendRTDesc[1].BlendEnable,
			outDesc.BlendRTDesc[2].BlendEnable,outDesc.BlendRTDesc[3].BlendEnable) ||
		anyDifferent(outDesc.BlendRTDesc[0].RenderTargetWriteMask,outDesc.BlendRTDesc[1].RenderTargetWriteMask,
			outDesc.BlendRTDesc[2].RenderTargetWriteMask,outDesc.BlendRTDesc[3].RenderTargetWriteMask);
#elif (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	for (int i=0; i<8; i++) {
		outDesc.BlendRTDesc[i].BlendEnable = state.Targets[i].BlendEnable;
		outDesc.BlendRTDesc[i].SrcBlend = state.Targets[i].SrcBlend;
		outDesc.BlendRTDesc[i].DestBlend = state.Targets[i].DestBlend;
		outDesc.BlendRTDesc[i].BlendOp = state.Targets[i].BlendOp;
		outDesc.BlendRTDesc[i].SrcBlendAlpha = state.Targets[i].SrcBlendAlpha;
		outDesc.BlendRTDesc[i].DestBlendAlpha = state.Targets[i].DestBlendAlpha;
		outDesc.BlendRTDesc[i].BlendOpAlpha = state.Targets[i].BlendOpAlpha;
# if RSG_ORBIS
		outDesc.BlendRTDesc[i].RenderTargetWriteMask = (state.ColorWriteMask >> (i * 4)) & 15;
# else
		outDesc.BlendRTDesc[i].RenderTargetWriteMask = state.Targets[i].RenderTargetWriteMask;
# endif
	}
	outDesc.IndependentBlendEnable = state.IndependentBlendEnable;
# if __WIN32
	outDesc.AlphaToCoverageEnable = state.AlphaToCoverageEnable;	
	outDesc.AlphaToCoverageEnable = state.AlphaToCoverageEnable;
# endif
#endif
}

void DestroyBlendState(grcBlendStateHandle handle)
{
	BlendStateStore.Release(handle);
}

grcSamplerStateHandle CreateSamplerState(const grcSamplerStateDesc &desc)
{
	using namespace grcSSV;
	grcSamplerState state;
	//Initializing to zero to zero out padding also
	sysMemSet(&state, 0, sizeof(state));

	// We map Quincunx to Anisotropic on non-ps3 platforms, so don't issue a spurious assert in those cases.
	PS3_ONLY(Assert((desc.Filter == grcSSV::FILTER_ANISOTROPIC && desc.MaxAnisotropy != 1.0f) || (desc.Filter != grcSSV::FILTER_ANISOTROPIC && desc.MaxAnisotropy == 1.0f)));
	Assert(desc.MinLod <= desc.MaxLod);

#if __WIN32
#define REMAP(min,mag,mip) state.MinFilter = grcSSV::TEXF_##min; state.MagFilter = grcSSV::TEXF_##mag; state.MipFilter = grcSSV::TEXF_##mip; break

	switch (desc.Filter) {
		case FILTER_MIN_MAG_MIP_POINT: REMAP(POINT,POINT,POINT);
		case FILTER_MIN_MAG_POINT_MIP_LINEAR: REMAP(POINT,POINT,LINEAR);
		case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: REMAP(POINT,LINEAR,POINT);
		case FILTER_MIN_POINT_MAG_MIP_LINEAR: REMAP(POINT,LINEAR,LINEAR);
		case FILTER_MIN_LINEAR_MAG_MIP_POINT: REMAP(LINEAR,POINT,POINT);
		case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR: REMAP(LINEAR,POINT,LINEAR);
		case FILTER_MIN_MAG_LINEAR_MIP_POINT: REMAP(LINEAR,LINEAR,POINT);
		case FILTER_MIN_MAG_MIP_LINEAR: REMAP(LINEAR,LINEAR,LINEAR);
#if RSG_PC || RSG_DURANGO
		// TODO: A mip filter of anisotropic is definitely undefined, but not sure about MAG filter.
		// It may be restricted only to certain formats, which will be a pain to detect since we'll
		// need to examine the texture bound to the sampler at runtime.
		case FILTER_ANISOTROPIC: REMAP(ANISOTROPIC,LINEAR,LINEAR);
#elif __XENON
		case FILTER_ANISOTROPIC: REMAP(ANISOTROPIC,ANISOTROPIC,LINEAR);
#endif
#if RSG_PC || RSG_DURANGO
		case FILTER_COMPARISON_MIN_MAG_MIP_POINT: REMAP(POINT,POINT,POINT);
		case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR: REMAP(POINT,POINT,LINEAR);
		case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT: REMAP(POINT,LINEAR,POINT);
		case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR: REMAP(POINT,LINEAR,LINEAR);
		case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT: REMAP(LINEAR,POINT,POINT);
		case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR: REMAP(LINEAR,POINT,LINEAR);
		case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT: REMAP(LINEAR,LINEAR,POINT);
		case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR: REMAP(LINEAR,LINEAR,LINEAR);
		case FILTER_COMPARISON_ANISOTROPIC: REMAP(ANISOTROPIC,LINEAR,LINEAR);
#endif
		default: Assertf(false,"Unhandled filter type %x in CreateSamplerState",desc.Filter);
	}
	Assign(state.AddressU,desc.AddressU);
	Assign(state.AddressV,desc.AddressV);
	Assign(state.AddressW,desc.AddressW);
	Assign(state.MaxAnisotropy, 
#if (RSG_PC || RSG_DURANGO) && !__RESOURCECOMPILER	&& !__TOOL	
		(uOverrideAnisotropic && desc.MaxAnisotropy) ? uOverrideAnisotropic : 
#endif		
		desc.MaxAnisotropy);

	Assign(state.MipLodBias.f,desc.MipLodBias);
	AssertMsg((desc.BorderColorBlue==1.0f&&desc.BorderColorGreen==1.0f&&desc.BorderColorRed==1.0f)||
		(desc.BorderColorAlpha==1.0f&&desc.BorderColorBlue==0.0f&&desc.BorderColorGreen==0.0f&&desc.BorderColorRed==0.0f)||
		(desc.BorderColorAlpha==0.0f&&desc.BorderColorBlue==0.0f&&desc.BorderColorGreen==0.0f&&desc.BorderColorRed==0.0f),
		"RAGE requires black or white for border color for Xbox360 compatibility");
	state.BorderColor = desc.BorderColorBlue? 255 : 0;
	state.BorderColorW = desc.BorderColorAlpha == 1.0f;
	Assign(state.MaxMipLevel,int(desc.MinLod));
	Assign(state.MinMipLevel,int(desc.MaxLod));
	Assign(state.TrilinearThresh,desc.TrilinearThresh);
#if __D3D11
	Assign(state.CompareFunc,desc.ComparisonFunc);
#endif
#undef REMAP

#elif RSG_ORBIS
#define REMAP(min,mag,mip) state.MinFilter = grcSSV::TEXF_##min; state.MagFilter = grcSSV::TEXF_##mag; state.MipFilter = grcSSV::TEXF_##mip; break;

	switch (desc.Filter) {
	case FILTER_MIN_MAG_MIP_POINT: REMAP(POINT,POINT,POINT);
	case FILTER_MIN_MAG_POINT_MIP_LINEAR: REMAP(POINT,POINT,LINEAR);
	case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: REMAP(POINT,LINEAR,POINT);
	case FILTER_MIN_POINT_MAG_MIP_LINEAR: REMAP(POINT,LINEAR,LINEAR);
	case FILTER_MIN_LINEAR_MAG_MIP_POINT: REMAP(LINEAR,POINT,POINT);
	case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR: REMAP(LINEAR,POINT,LINEAR);
	case FILTER_MIN_MAG_LINEAR_MIP_POINT: REMAP(LINEAR,LINEAR,POINT);
	case FILTER_MIN_MAG_MIP_LINEAR: REMAP(LINEAR,LINEAR,LINEAR);
	case FILTER_ANISOTROPIC: REMAP(ANISOTROPIC,ANISOTROPIC,LINEAR);
	case FILTER_COMPARISON_MIN_MAG_MIP_POINT: REMAP(POINT,POINT,POINT);
	case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR: REMAP(POINT,POINT,LINEAR);
	case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT: REMAP(POINT,LINEAR,POINT);
	case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR: REMAP(POINT,LINEAR,LINEAR);
	case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT: REMAP(LINEAR,POINT,POINT);
	case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR: REMAP(LINEAR,POINT,LINEAR);
	case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT: REMAP(LINEAR,LINEAR,POINT);
	case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR: REMAP(LINEAR,LINEAR,LINEAR);
	case FILTER_COMPARISON_ANISOTROPIC: REMAP(ANISOTROPIC,ANISOTROPIC,LINEAR);
	default: Assertf(false,"unhandled filter %x",desc.Filter);
	}
	Assign(state.AddressU,desc.AddressU);
	Assign(state.AddressV,desc.AddressV);
	Assign(state.AddressW,desc.AddressW);
	Assign(state.MaxAnisotropy, 
#if !__RESOURCECOMPILER	&& !__TOOL	
		(uOverrideAnisotropic && desc.MaxAnisotropy) ? uOverrideAnisotropic : 
#endif		
		desc.MaxAnisotropy);
	Assign(state.CompareFunc, desc.ComparisonFunc);
#if __D3D9
	state.CompareFunc = grcSSV::TEXTUREZFUNC_GREATER;
#endif // __D3D9
	Assign(state.MipLodBias.f,desc.MipLodBias);
	state.BorderColor = desc.BorderColorBlue? 255 : 0;
	state.BorderColorW = desc.BorderColorAlpha == 1.0f;
	Assign(state.MaxMipLevel,int(desc.MinLod));
	Assign(state.MinMipLevel,int(desc.MaxLod));
	Assign(state.TrilinearThresh,desc.TrilinearThresh);

	static sce::Gnm::MipFilterMode remapMip[] = { sce::Gnm::kMipFilterModeNone, sce::Gnm::kMipFilterModePoint, sce::Gnm::kMipFilterModeLinear, sce::Gnm::kMipFilterModeLinear, sce::Gnm::kMipFilterModeLinear };
	static sce::Gnm::FilterMode remapMinMag[] = { sce::Gnm::kFilterModePoint, sce::Gnm::kFilterModePoint, sce::Gnm::kFilterModeBilinear, sce::Gnm::kFilterModeAnisoBilinear, sce::Gnm::kFilterModeAnisoBilinear };
	static sce::Gnm::ZFilterMode remapVolumeMinMag[] = { sce::Gnm::kZFilterModePoint, sce::Gnm::kZFilterModePoint, sce::Gnm::kZFilterModeLinear, sce::Gnm::kZFilterModeLinear, sce::Gnm::kZFilterModeLinear };
	static sce::Gnm::DepthCompare remapDepthComparison[] = { sce::Gnm::kDepthCompareNever, sce::Gnm::kDepthCompareLess, sce::Gnm::kDepthCompareEqual, sce::Gnm::kDepthCompareLessEqual, sce::Gnm::kDepthCompareGreater, sce::Gnm::kDepthCompareNotEqual, sce::Gnm::kDepthCompareGreaterEqual, sce::Gnm::kDepthCompareAlways };

	state.s.init();
	state.s.setMipFilterMode(remapMip[state.MipFilter]);
	state.s.setXyFilterMode(remapMinMag[state.MagFilter],remapMinMag[state.MinFilter]);
	state.s.setZFilterMode(remapVolumeMinMag[state.MipFilter]);
	state.s.setWrapMode((sce::Gnm::WrapMode)state.AddressU,(sce::Gnm::WrapMode)state.AddressV,(sce::Gnm::WrapMode)state.AddressW);
	state.s.setBorderColor(state.BorderColor ? sce::Gnm::kBorderColorOpaqueWhite : (state.BorderColorW ? sce::Gnm::kBorderColorOpaqueBlack : sce::Gnm::kBorderColorTransBlack));
	state.s.setDepthCompareFunction(remapDepthComparison[state.CompareFunc]);
	state.s.setAnisotropyRatio(uOverrideAnisotropic ? remapAnisotropic[uOverrideAnisotropic] : remapAnisotropic[state.MaxAnisotropy]);
	state.s.setLodBias(int(state.MipLodBias.f * 256.0) & 0x3FFF,0);
	state.s.setDisableCubeWrap(false);
	state.s.setForceUnnormalized(false);
	state.s.setAnisotropyBias(0);
	state.s.setForceDegamma(false);
#elif __PPU
	static u8 remapWrap[] = {
		CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_MIRROR, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, 
		CELL_GCM_TEXTURE_BORDER, CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP
	};
	static u8 remapAnisotropy[17] = {
		CELL_GCM_TEXTURE_MAX_ANISO_1, CELL_GCM_TEXTURE_MAX_ANISO_1, CELL_GCM_TEXTURE_MAX_ANISO_2, CELL_GCM_TEXTURE_MAX_ANISO_2,		// 0-3
		CELL_GCM_TEXTURE_MAX_ANISO_4, CELL_GCM_TEXTURE_MAX_ANISO_4, CELL_GCM_TEXTURE_MAX_ANISO_6, CELL_GCM_TEXTURE_MAX_ANISO_6,		// 4-7
		CELL_GCM_TEXTURE_MAX_ANISO_8, CELL_GCM_TEXTURE_MAX_ANISO_8, CELL_GCM_TEXTURE_MAX_ANISO_10, CELL_GCM_TEXTURE_MAX_ANISO_10,	// 8-11
		CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12, CELL_GCM_TEXTURE_MAX_ANISO_12,	// 12-15
		CELL_GCM_TEXTURE_MAX_ANISO_16
	};

	u32 filters = 0;
	switch (desc.Filter) { // min/mip << 16, then mag << 24:
		case FILTER_MIN_MAG_MIP_POINT: filters =  (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_NEAREST_NEAREST << 16) | (CELL_GCM_TEXTURE_NEAREST << 24); break;
		case FILTER_MIN_MAG_POINT_MIP_LINEAR: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_NEAREST_LINEAR << 16) | (CELL_GCM_TEXTURE_NEAREST << 24); break;
		case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_NEAREST_NEAREST << 16) | (CELL_GCM_TEXTURE_LINEAR << 24); break;
		case FILTER_MIN_POINT_MAG_MIP_LINEAR: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_NEAREST_LINEAR << 16) | (CELL_GCM_TEXTURE_LINEAR << 24); break;
		case FILTER_MIN_LINEAR_MAG_MIP_POINT: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_LINEAR_NEAREST << 16) | (CELL_GCM_TEXTURE_NEAREST << 24); break;
		case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_LINEAR_LINEAR << 16) | (CELL_GCM_TEXTURE_NEAREST << 24); break;
		case FILTER_MIN_MAG_LINEAR_MIP_POINT: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_LINEAR_NEAREST << 16) | (CELL_GCM_TEXTURE_LINEAR << 24); break;
		case FILTER_MIN_MAG_MIP_LINEAR: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_LINEAR_LINEAR << 16) | (CELL_GCM_TEXTURE_LINEAR << 24); break;
		case FILTER_ANISOTROPIC: filters = (CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX << 13) | (CELL_GCM_TEXTURE_LINEAR_LINEAR << 16) | (CELL_GCM_TEXTURE_LINEAR << 24); break; // also handled below
		case FILTER_GAUSSIAN: filters = (CELL_GCM_TEXTURE_CONVOLUTION_GAUSSIAN << 13) | (CELL_GCM_TEXTURE_CONVOLUTION_MIN << 16) | (CELL_GCM_TEXTURE_CONVOLUTION_MAG << 24); break;
		default: Assertf(false,"Unhandled filter type %x in CreateSamplerState",desc.Filter);
	}

	// (cmd)[1] = CELL_GCM_ENDIAN_SWAP(((maxAniso) << 4) | ((maxlod) << 7) | ((minlod) << 19) | ((enable) << 31));
	state.control0 = (desc.AlphaKill? 4 : 0) | (remapAnisotropy[desc.Filter == FILTER_ANISOTROPIC? desc.MaxAnisotropy : 0] << 4) | ((int(desc.MaxLod * 256.0f) & 0xFFF) << 7) | ((int(desc.MinLod * 256.0f) & 0xFFF) << 19) | (1 << 31);

	// (cmd)[1] = CELL_GCM_ENDIAN_SWAP((wraps) | ((anisoBias) << 4) | ((wrapt) << 8) | ((unsignedRemap) << 12) | ((wrapr) << 16) | ((gamma) << 20) |((signedRemap) << 24) | ((zfunc) << 28));
	state.address = (remapWrap[desc.AddressU]) | (remapWrap[desc.AddressV] << 8) | (remapWrap[desc.AddressW] << 16) | (desc.TextureZFunc << 28);

	// (cmd)[1] = CELL_GCM_ENDIAN_SWAP((mipBias) | ((conv) << 13) | ((minFilter) << 16) | ((magFilter) << 24) | ((as) << 28) | ((rs) << 29) | ((gs) << 30) | ((bs) << 31) );
	state.filter = (int(desc.MipLodBias * 256.0f) & 0x1FFF) | filters;

	// Xenon only supports black and white; we could actually support more border colors here though and move the trilinear thresh somewhere else.
	// For now, let's use four-bit color so we can use si_fsmb directly to expand it into 32 bit "color".  Note that since it produces a 128-bit
	// result and the "preferred slot" is actually the most significant word, the bits all need to be shifted left 12 to produce the correct
	// result.  If we didn't explicitly store the 0x2D00 we could keep TrilinearThresh and the border in the same halfword.
	state.control2 = desc.TrilinearThresh | 0x2D00;
	state.border = (desc.BorderColorAlpha? 8<<12:0) | (desc.BorderColorRed? 4<<12:0) | (desc.BorderColorGreen? 2<<12:0) | (desc.BorderColorBlue? 1<<12:0);
#endif

	grcSamplerStateHandle result = SamplerStateStore.Allocate(state,desc);
	PPU_ONLY(g_SamplerStateCount = SamplerStateStore.GetMaxUsed()+1);
#if __ASSERT
	grcSamplerStateDesc test;
	GetSamplerStateDesc(result,test);
	Assert(desc.Filter == test.Filter);
	Assert(desc.AddressU == test.AddressU);
	Assert(desc.AddressV == test.AddressV);
	Assert(desc.AddressW == test.AddressW);
	Assert(fabsf(desc.MipLodBias - test.MipLodBias) < (1.0f / 256.0f));
#if !__RESOURCECOMPILER	&& !__TOOL
	Assert(uOverrideAnisotropic || desc.MaxAnisotropy == test.MaxAnisotropy);
#endif // !__RESOURCECOMPILER	&& !__TOOL
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	Assert(desc.ComparisonFunc == test.ComparisonFunc);
#endif
	Assert(desc.BorderColorRed == test.BorderColorRed);
	Assert(desc.BorderColorGreen == test.BorderColorGreen);
	Assert(desc.BorderColorBlue == test.BorderColorBlue);
	Assert(desc.BorderColorAlpha == test.BorderColorAlpha);
	Assert(fabsf(desc.MinLod - test.MinLod) < (1.0f / 256.0f));
	Assert(fabsf(desc.MaxLod - test.MaxLod) < (1.0f / 256.0f));
	Assert(desc.TrilinearThresh == test.TrilinearThresh);
#if __D3D9
	Assert(desc.TextureZFunc == test.TextureZFunc);
#endif // __D3D9
	PS3_ONLY(Assert(desc.AlphaKill == test.AlphaKill));
#endif
	return result;
}

void GetSamplerStateDesc(grcSamplerStateHandle handle,grcSamplerStateDesc& outDesc)
{
	const grcSamplerState &state = SamplerStateStore[handle];

	using namespace grcSSV;

#if __WIN32 || RSG_ORBIS
	if (state.MinFilter==TEXF_ANISOTROPIC)
		outDesc.Filter = FILTER_ANISOTROPIC;
	else if (state.MipFilter==TEXF_LINEAR) {
		if (state.MinFilter==TEXF_LINEAR)
			outDesc.Filter = state.MagFilter==TEXF_LINEAR? 
#if __D3D11 || RSG_ORBIS
			(state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_MAG_MIP_LINEAR : FILTER_COMPARISON_MIN_MAG_MIP_LINEAR) : (state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR : FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
#else
			 FILTER_MIN_MAG_MIP_LINEAR : FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
#endif
		else // MinFilter = POINT
			outDesc.Filter = state.MagFilter==TEXF_LINEAR? 
#if __D3D11 || RSG_ORBIS
			(state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_POINT_MAG_MIP_LINEAR : FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR) :  (state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_MAG_POINT_MIP_LINEAR : FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
#else
			FILTER_MIN_POINT_MAG_MIP_LINEAR : FILTER_MIN_MAG_POINT_MIP_LINEAR;
#endif
	}
	else { // MipFilter==TEXF_POINT
		if (state.MinFilter==TEXF_LINEAR)
			outDesc.Filter = state.MagFilter==TEXF_LINEAR ? 
#if __D3D11 || RSG_ORBIS
			(state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_MAG_LINEAR_MIP_POINT : FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT) : (state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_LINEAR_MAG_MIP_POINT : FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
#else
			FILTER_MIN_MAG_LINEAR_MIP_POINT: FILTER_MIN_LINEAR_MAG_MIP_POINT;
#endif
		else // MinFilter = POINT
			outDesc.Filter = state.MagFilter==TEXF_LINEAR ? 
#if __D3D11 || RSG_ORBIS
			(state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT) : (state.CompareFunc == grcRSV::CMP_NEVER ? FILTER_MIN_MAG_MIP_POINT : FILTER_COMPARISON_MIN_MAG_MIP_POINT);
#else
			FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT: FILTER_MIN_MAG_MIP_POINT;
#endif
	}
	outDesc.AddressU = state.AddressU;
	outDesc.AddressV = state.AddressV;
	outDesc.AddressW = state.AddressW;
	outDesc.MipLodBias = state.MipLodBias.f;
	outDesc.MaxAnisotropy = state.MaxAnisotropy;

	outDesc.ComparisonFunc = 
#if __D3D11 || RSG_ORBIS
		state.CompareFunc;
#else
		grcRSV::CMP_NEVER;	// not actually retained on this platform
#endif
	outDesc.BorderColorRed = state.BorderColor? 1.0f : 0.0f;
	outDesc.BorderColorGreen = state.BorderColor? 1.0f : 0.0f;
	outDesc.BorderColorBlue = state.BorderColor? 1.0f : 0.0f;
	outDesc.BorderColorAlpha = state.BorderColor || state.BorderColorW? 1.0f : 0.0f;
	outDesc.MinLod = state.MaxMipLevel;
	outDesc.MaxLod = state.MinMipLevel;
	outDesc.TrilinearThresh = state.TrilinearThresh;
	outDesc.AlphaKill = false;
#elif RSG_PS3
	outDesc.MaxLod = ((state.control0 >> 7) & 0xFFF) * (1.0f / 256.0f);
	outDesc.MinLod = ((state.control0 >> 19) & 0xFFF) * (1.0f / 256.0f);
	outDesc.ComparisonFunc = grcRSV::CMP_NEVER;	// not actually retained on this platform
	static u8 remapWrap[] = { 0, TADDRESS_WRAP, TADDRESS_MIRROR, TADDRESS_CLAMP, TADDRESS_BORDER, 0, 0, 0, TADDRESS_MIRRORONCE };
	outDesc.AddressU = remapWrap[state.address & 15];
	outDesc.AddressV = remapWrap[(state.address >> 8) & 15];
	outDesc.AddressW = remapWrap[(state.address >> 16) & 15];
	outDesc.MipLodBias = (s32(state.filter << 19) >> 19) * (1.0f / 256.0f);
	switch ((state.control0 >> 4) & 7) {
		case CELL_GCM_TEXTURE_MAX_ANISO_1: outDesc.MaxAnisotropy = 1; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_2: outDesc.MaxAnisotropy = 2; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_4: outDesc.MaxAnisotropy = 4; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_6: outDesc.MaxAnisotropy = 6; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_8: outDesc.MaxAnisotropy = 8; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_10: outDesc.MaxAnisotropy = 10; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_12: outDesc.MaxAnisotropy = 12; break;
		case CELL_GCM_TEXTURE_MAX_ANISO_16: outDesc.MaxAnisotropy = 16; break;
		default: Assert(false);
	}
	if (outDesc.MaxAnisotropy != 1)
		outDesc.Filter = FILTER_ANISOTROPIC;
	else
		switch (state.filter & 0xFFFF0000) {
			case (CELL_GCM_TEXTURE_NEAREST_NEAREST << 16) | (CELL_GCM_TEXTURE_NEAREST << 24): outDesc.Filter = FILTER_MIN_MAG_MIP_POINT; break;
			case (CELL_GCM_TEXTURE_NEAREST_LINEAR << 16) | (CELL_GCM_TEXTURE_NEAREST << 24): outDesc.Filter = FILTER_MIN_MAG_POINT_MIP_LINEAR; break;
			case (CELL_GCM_TEXTURE_NEAREST_NEAREST << 16) | (CELL_GCM_TEXTURE_LINEAR << 24): outDesc.Filter = FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT; break;
			case (CELL_GCM_TEXTURE_NEAREST_LINEAR << 16) | (CELL_GCM_TEXTURE_LINEAR << 24): outDesc.Filter = FILTER_MIN_POINT_MAG_MIP_LINEAR; break;
			case (CELL_GCM_TEXTURE_LINEAR_NEAREST << 16) | (CELL_GCM_TEXTURE_NEAREST << 24): outDesc.Filter = FILTER_MIN_LINEAR_MAG_MIP_POINT; break;
			case (CELL_GCM_TEXTURE_LINEAR_LINEAR << 16) | (CELL_GCM_TEXTURE_NEAREST << 24): outDesc.Filter = FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; break;
			case (CELL_GCM_TEXTURE_LINEAR_NEAREST << 16) | (CELL_GCM_TEXTURE_LINEAR << 24): outDesc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
			case (CELL_GCM_TEXTURE_LINEAR_LINEAR << 16) | (CELL_GCM_TEXTURE_LINEAR << 24): outDesc.Filter = FILTER_MIN_MAG_MIP_LINEAR; break;
			case (CELL_GCM_TEXTURE_CONVOLUTION_MIN << 16) | (CELL_GCM_TEXTURE_CONVOLUTION_MAG << 24): outDesc.Filter = FILTER_GAUSSIAN; break;
		}
	outDesc.TrilinearThresh = state.control2 & 0x1F;
	outDesc.BorderColorAlpha = state.border & (8<<12)? 1.0f : 0.0f;
	outDesc.BorderColorRed = state.border & (4<<12)? 1.0f : 0.0f;
	outDesc.BorderColorGreen = state.border & (2<<12)? 1.0f : 0.0f;
	outDesc.BorderColorBlue = state.border & (1<<12)? 1.0f : 0.0f;
	outDesc.TextureZFunc = (state.address >> 28);
	outDesc.AlphaKill = (state.control0 & 4) != 0;
#endif
}

void DestroySamplerState(grcSamplerStateHandle handle)
{
	SamplerStateStore.Release(handle);
}

} // name grcStateBlock
} // namespace rage

#endif	// !__SPU

namespace rage {

namespace grcStateBlock {

#if __SPU
atSimpleCache4<grcDepthStencilState> s_DepthStencilStateCache;
atSimpleCache4<grcRasterizerState> s_RasterizerStateCache;
atSimpleCache4<grcBlendState> s_BlendStateCache;
#endif

#if LAZY_STATEBLOCKS
/*
	Lazy state blocks are pretty simple on non-PS3 targets: On a state change, set a dirty bit and remember the new state.
	Then, any time you're about to render something, call a function that checks the dirty bit, and if it's set, forces
	all of states that are different from the last flushed states down to the hardware.

	On the other hand, it's a bit of a nightmare on PS3.  The issue is that PPU can issue state changes and rendering commands
	itself, and it can also issue SPU macros that issue complex rendering commands that themselves issue state changes.

	If we're issuing a command that may change the current stateblocks (grcEffect__Bind or rmcDrawable__Draw, for example),
	then we need to flush any pending state from the PPU (so that SPU will see it) and then place the PPU in a special
	indeterminate state such that the next stateblock commands we issue are forced to always be considered "different".
*/
#if __SPU
void DoFlush();
inline void Flush() { if (Stateblock_Dirty) DoFlush(); }
#else
DECLARE_MTR_THREAD u8 Stateblock_Dirty = 0;
DECLARE_MTR_THREAD grcDepthStencilStateHandle DSS_Flushed = DSS_Invalid;
DECLARE_MTR_THREAD unsigned FlushedStencilRef = 0;
DECLARE_MTR_THREAD grcRasterizerStateHandle RS_Flushed = RS_Invalid;
DECLARE_MTR_THREAD grcBlendStateHandle BS_Flushed = BS_Invalid;
DECLARE_MTR_THREAD u32 FlushedBlendFactors = 0;
DECLARE_MTR_THREAD u64 FlushedSampleMask = 0;
#endif
enum eStateBlockDirty
{
	DEPTH_STENCIL_STATE_DIRTY					= (1<<0),
	RASTERIZER_STATE_DIRTY						= (1<<1),
	BLEND_STATE_DIRTY							= (1<<2)
};
#endif

void SetDepthStencilState(grcDepthStencilStateHandle newState)
{
#if EFFECT_STENCIL_REF_MASK
	SetDepthStencilState(newState, grcEffect::GetStencilRefMask());
#else
	SetDepthStencilState(newState, 0xFF);
#endif
}

void SetDepthStencilState(grcDepthStencilStateHandle newState,unsigned stencilRef)
{
	AssertMsg(newState != DSS_Invalid, "SetDepthStencilState called with DSS_Invalid");		  

#if LAZY_STATEBLOCKS
	if (DSS_Active != newState || ActiveStencilRef != stencilRef) {
		DSS_Active = newState;
		ActiveStencilRef = u8(stencilRef);
		Stateblock_Dirty |= DEPTH_STENCIL_STATE_DIRTY;
	}
}

void FlushDepthStencilState(grcDepthStencilStateHandle newState,unsigned stencilRef)
{
#else
#if !__SPU
	if (NOTFINAL_ONLY(!PARAM_nolazystateblock.Get() &&) DSS_Active==newState && ActiveStencilRef==u8(stencilRef) && !DSS_Dirty)
		return;
	DSS_Dirty = false;
#endif

	DSS_Active = newState;
	ActiveStencilRef = u8(stencilRef);
#endif

#if __PPU
	SPU_COMMAND(grcStateBlock__SetDepthStencilState,stencilRef);
	cmd->handle = newState;
#elif RSG_DURANGO && _XDK_VER >= 10812
	g_grcCurrentContext->OMSetDepthStencilState(DepthStencilStateStore.GetStateX(newState),stencilRef);
#elif __D3D11
	g_grcCurrentContext->OMSetDepthStencilState(DepthStencilStateStore.GetState(newState),stencilRef);
#elif __WIN32
	const grcDepthStencilState &data = DepthStencilStateStore[newState];
	grcDeviceHandle *device = GRCDEVICE.GetCurrent();
#if __XENON && LAZY_STATEBLOCKS
	if (DSS_Flushed == newState) {
		device->SetRenderState_Inline(D3DRS_STENCILREF,FlushedStencilRef = stencilRef);
		return;
	}
#endif
	device->SetRenderState_Inline(D3DRS_ZENABLE,data.DepthEnable);
	device->SetRenderState_Inline(D3DRS_ZWRITEENABLE,data.DepthWriteMask);
	device->SetRenderState_Inline(D3DRS_STENCILENABLE,data.StencilEnable);
	device->SetRenderState_Inline(D3DRS_TWOSIDEDSTENCILMODE,data.TwoSidedStencil);
	device->SetRenderState_Inline(D3DRS_ZFUNC,data.DepthFunc);
	device->SetRenderState_Inline(D3DRS_STENCILMASK,data.StencilReadMask);
	device->SetRenderState_Inline(D3DRS_STENCILWRITEMASK,data.StencilWriteMask);
	device->SetRenderState_Inline(D3DRS_STENCILFAIL,data.FrontStencilFailOp);
	device->SetRenderState_Inline(D3DRS_STENCILZFAIL,data.FrontStencilDepthFailOp);
	device->SetRenderState_Inline(D3DRS_STENCILPASS,data.FrontStencilPassOp);
	device->SetRenderState_Inline(D3DRS_STENCILFUNC,data.FrontStencilFunc);
	device->SetRenderState_Inline(D3DRS_STENCILREF,stencilRef);
	if (data.TwoSidedStencil) {
		device->SetRenderState_Inline(D3DRS_CCW_STENCILFAIL,data.BackStencilFailOp);
		device->SetRenderState_Inline(D3DRS_CCW_STENCILZFAIL,data.BackStencilDepthFailOp);
		device->SetRenderState_Inline(D3DRS_CCW_STENCILPASS,data.BackStencilPassOp);
		device->SetRenderState_Inline(D3DRS_CCW_STENCILFUNC,data.BackStencilFunc);
	}

#elif RSG_ORBIS
	const grcDepthStencilState &data = DepthStencilStateStore[newState];
	sce::Gnm::StencilControl sc = data.sc;
	sc.m_testVal = stencilRef;
	gfxc.setDepthStencilControl(data.dsc);
	gfxc.setStencilSeparate(sc,sc);
	gfxc.setStencilOpControl(data.soc);
#elif __SPU

#  if DUMB_SET_STATE

	const grcDepthStencilState &data = *s_DepthStencilStateCache.Get((grcDepthStencilState*)(pSpuGcmState->DepthStencilStateStore + newState * sizeof(grcDepthStencilState)));

	qword *q = ReserveMethodSizeAligned(GCM_CONTEXT,8);
	uint32_t current0 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE, 1);
	uint32_t current1 = (g_RenderStateBlockMask & BLOCK_DEPTHACCESS)? CELL_GCM_FALSE : data.DepthEnable;
	uint32_t current2 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_DEPTH_MASK, 1);
	uint32_t current3 = (g_RenderStateBlockMask & BLOCK_DEPTHACCESS)? CELL_GCM_FALSE : data.DepthWriteMask;
	q[0] = MAKE_QWORD(current0,current1,current2,current3);

	uint32_t current4 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE, 1);
	uint32_t current5 = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? CELL_GCM_FALSE : data.StencilEnable;
	uint32_t current6 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE, 1);
	uint32_t current7 = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? CELL_GCM_FALSE : data.TwoSidedStencil;
	q[1] = MAKE_QWORD(current4,current5,current6,current7);

	uint32_t current8 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_STENCIL_MASK, 1);
	uint32_t current9 = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? 0x00 : data.StencilWriteMask;
	q[2] = MAKE_QWORD_ZZ(current8,current9);

	uint32_t current12 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_DEPTH_FUNC, 1);
	uint32_t current13 = data.DepthFunc | CELL_GCM_NEVER;
	uint32_t current14 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_STENCIL_OP_FAIL, 3);
	uint32_t current15 = data.FrontStencilFailOp;
	q[3] = MAKE_QWORD(current12,current13,current14,current15);

	uint32_t current16 = data.FrontStencilDepthFailOp;
	uint32_t current17 = data.FrontStencilPassOp;
	uint32_t current18 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_STENCIL_FUNC, 3);
	uint32_t current19 = data.FrontStencilFunc | CELL_GCM_NEVER;
	q[4] = MAKE_QWORD(current16,current17,current18,current19);

	uint32_t current20 = stencilRef;
	uint32_t current21 = data.StencilReadMask;
	uint32_t current22 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL, 3);
	uint32_t current23 = data.BackStencilFailOp;
	q[5] = MAKE_QWORD(current20,current21,current22,current23);

	uint32_t current24 = data.BackStencilDepthFailOp;
	uint32_t current25 = data.BackStencilPassOp;
	uint32_t current26 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BACK_STENCIL_MASK, 1);
	uint32_t current27 = data.StencilWriteMask;
	q[6] = MAKE_QWORD(current24,current25,current26,current27);

	uint32_t current28 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC, 3);
	uint32_t current29 = data.BackStencilFunc | CELL_GCM_NEVER;
	uint32_t current30 = stencilRef;
	uint32_t current31 = data.StencilReadMask;
	q[7] = MAKE_QWORD(current28,current29,current30,current31);

#  else		// DUMB_SET_STATE
	spuGcmState *const pState = pSpuGcmState;

	const grcDepthStencilState &data = *s_DepthStencilStateCache.Get((grcDepthStencilState*)(pSpuGcmState->DepthStencilStateStore + newState * sizeof(grcDepthStencilState)));

	u32 wantedDepthTestEnable       = (g_RenderStateBlockMask & BLOCK_DEPTHACCESS)? CELL_GCM_FALSE : data.DepthEnable;
	u32 wantedDepthWriteMask        = (g_RenderStateBlockMask & BLOCK_DEPTHACCESS)? CELL_GCM_FALSE : data.DepthWriteMask;
	u32 wantedDepthFunc             = data.DepthFunc | CELL_GCM_NEVER;
	u32 wantedStencilTestEnable     = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? CELL_GCM_FALSE : data.StencilEnable;
	u32 wantedStencilTwoSidedEnable = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? CELL_GCM_FALSE : data.TwoSidedStencil;
	u32 wantedStencilWriteMask      = (g_RenderStateBlockMask & BLOCK_STENCILACCESS)? 0x00 : data.StencilWriteMask;
	u32 wantedBackStencilWriteMask  = data.StencilWriteMask;

	u32 wantedStencilFail           = data.FrontStencilFailOp;
	u32 wantedStencilDepthFail      = data.FrontStencilDepthFailOp;
	u32 wantedStencilPass           = data.FrontStencilPassOp;

	u32 wantedStencilFunc           = data.FrontStencilFunc | CELL_GCM_NEVER;
	u32 wantedStencilRef            = stencilRef;
	u32 wantedStencilCmpMask        = data.StencilReadMask;

	u32 wantedBackStencilFail       = data.BackStencilFailOp;
	u32 wantedBackStencilDepthFail  = data.BackStencilDepthFailOp;
	u32 wantedBackStencilPass       = data.BackStencilPassOp;

	u32 wantedBackStencilFunc       = data.BackStencilFunc | CELL_GCM_NEVER;
	u32 wantedBackStencilRef        = stencilRef;
	u32 wantedBackStencilCmpMask    = data.StencilReadMask;

	qword cmd = si_from_ptr(EnsureMethodSpaceWords(GCM_CONTEXT,
		  1         // potential alignment to dword boundary
		+ 7*2       // up to 7 single data word methods
		+ 2         // potential alignment to qword boundary
		+ 4*4));    // up to 4 triple data word methods

	// Align cmd to a dword boundary.  This ensures that the two word methods
	// below never cross a qword boundary.  The reason for optimizing the code
	// like this is for local store reasons, not for spu performance.
	CompileTimeAssert(CELL_GCM_METHOD_NOP == 0);
	qword quad = si_lqd(cmd, 0);
	qword alignMask = si_rotqmby(si_il(-1), si_sfi(si_andi(cmd, 15), 0));
	quad = si_andc(quad, alignMask);
	si_stqd(quad, cmd, 0);
	cmd = si_andi(si_ai(cmd, 7), -8);

	const qword shufAAAA = si_ila(0x10203);
	const qword shufAaAa = MAKE_QWORD_u32(0x00010203, 0x10111213, 0x00010203, 0x10111213);
	const qword maskff00 = si_fsmbi(0xff00);

	qword methodMask = si_shufb(cmd, cmd, shufAAAA);
	methodMask = si_andi(methodMask, 8);
	methodMask = si_ceqi(methodMask, 8);
	methodMask = si_xor(maskff00, methodMask);

	qword diff, diff1, diff2, method, method1, method2;

#	if DEBUG_ALLOW_CACHED_STATES_TOGGLE
		qword forceDiff = si_from_uint(pSpuGcmState->DebugSetAllStates);
		forceDiff = si_clgtbi(forceDiff, 0);
		forceDiff = si_shufb(forceDiff, forceDiff, si_ilh(0x0303));
#	endif

#	define GENERATE_DWORD_CMD_IF_DIFFERENT(STATE, REGISTER)                    \
		/* Generate full qword mask with bits set if the state is being changed */ \
		diff = si_from_int(-(pState->CachedStates.DepthStencil.STATE == wanted##STATE)); \
		diff = si_xori(diff, -1);                                              \
		diff = si_shufb(diff, diff, shufAAAA);                                 \
		DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(diff = si_or(diff, forceDiff);)  \
                                                                               \
		/* Generate a qword with the method in both dwords */                  \
		method = si_shufb(si_from_uint(CELL_GCM_METHOD(REGISTER, 1)), si_from_uint(wanted##STATE), shufAaAa); \
                                                                               \
		/* Insert method into correct dword and store */                       \
		quad = si_selb(quad, method, methodMask);                              \
		si_stqd(quad, cmd, 0);                                                 \
                                                                               \
		/* Advance command buffer pointer if state changed */                  \
		cmd = si_a(cmd, si_andi(diff, 8));                                     \
                                                                               \
		/* Load next qword */                                                  \
		quad = si_lqd(cmd, 0);                                                 \
                                                                               \
		/* Change method mask to other dword if state changed */               \
		methodMask = si_xor(methodMask, diff)

	// Generate dword methods
	GENERATE_DWORD_CMD_IF_DIFFERENT(DepthTestEnable,        CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE);
	GENERATE_DWORD_CMD_IF_DIFFERENT(DepthWriteMask,         CELL_GCM_NV4097_SET_DEPTH_MASK);
	GENERATE_DWORD_CMD_IF_DIFFERENT(DepthFunc,              CELL_GCM_NV4097_SET_DEPTH_FUNC);
	GENERATE_DWORD_CMD_IF_DIFFERENT(StencilTestEnable,      CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE);
	GENERATE_DWORD_CMD_IF_DIFFERENT(StencilTwoSidedEnable,  CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE);
	GENERATE_DWORD_CMD_IF_DIFFERENT(StencilWriteMask,       CELL_GCM_NV4097_SET_STENCIL_MASK);
	GENERATE_DWORD_CMD_IF_DIFFERENT(BackStencilWriteMask,   CELL_GCM_NV4097_SET_BACK_STENCIL_MASK);

	// Align command buffer write pointer to a qword boundary
	alignMask = si_rotqmby(si_il(-1), si_sfi(si_andi(cmd, 15), 0));
	quad = si_andc(quad, alignMask);
	si_stqd(quad, cmd, 0);
	cmd = si_andi(si_ai(cmd, 15), -16);

#	define GENERATE_QWORD_CMD_IF_DIFFERENT(STATE0, STATE1, STATE2, REGISTER)   \
		/* Generate full qword mask with bits set if any of the states are being changed */ \
		diff  = si_from_int(-(pState->CachedStates.DepthStencil.STATE0 == wanted##STATE0)); \
		diff1 = si_from_int(-(pState->CachedStates.DepthStencil.STATE1 == wanted##STATE1)); \
		diff2 = si_from_int(-(pState->CachedStates.DepthStencil.STATE2 == wanted##STATE2)); \
		diff = si_and(diff2, si_and(diff1, diff));                             \
		diff = si_xori(diff, -1);                                              \
		diff = si_shufb(diff, diff, shufAAAA);                                 \
		DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(diff = si_or(diff, forceDiff);)  \
                                                                               \
		/* Generate qword command */                                           \
		method1 = si_shufb(si_from_uint(CELL_GCM_METHOD(REGISTER, 3)), si_from_uint(wanted##STATE0), shufAaAa); \
		method2 = si_shufb(si_from_uint(wanted##STATE1), si_from_uint(wanted##STATE2), shufAaAa); \
		method  = si_selb(method2, method1, maskff00);                         \
                                                                               \
		/* Store method and conditionally advance the command buffer pointer */ \
		si_stqd(method, cmd, 0);                                               \
		cmd = si_a(cmd, si_andi(diff, 16))

	// Generate qword methods
	GENERATE_QWORD_CMD_IF_DIFFERENT(StencilFail,     StencilDepthFail,     StencilPass,        CELL_GCM_NV4097_SET_STENCIL_OP_FAIL);
	GENERATE_QWORD_CMD_IF_DIFFERENT(StencilFunc,     StencilRef,           StencilCmpMask,     CELL_GCM_NV4097_SET_STENCIL_FUNC);
	GENERATE_QWORD_CMD_IF_DIFFERENT(BackStencilFail, BackStencilDepthFail, BackStencilPass,    CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL);
	GENERATE_QWORD_CMD_IF_DIFFERENT(BackStencilFunc, BackStencilRef,       BackStencilCmpMask, CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC);

#	undef  GENERATE_DWORD_CMD_IF_DIFFERENT
#	undef  GENERATE_QWORD_CMD_IF_DIFFERENT

	// Update the put pointer in the gcm context
	GCM_CONTEXT->current = (u32*)si_to_ptr(cmd);

	// Copy the wanted values over to the cached state
#	define UPDATE_CACHED_QWORD(STATE0, STATE1, STATE2, STATE3)                 \
		CompileTimeAssert((OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) & 15) == 0); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE1) == OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) + 4); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE2) == OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) + 8); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE3) == OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) + 12); \
		quad = si_selb(                                                        \
			si_shufb(si_from_uint(wanted##STATE2), si_from_uint(wanted##STATE3), shufAaAa), \
			si_shufb(si_from_uint(wanted##STATE0), si_from_uint(wanted##STATE1), shufAaAa), \
			maskff00);                                                         \
		si_stqd(quad, si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0))

    UPDATE_CACHED_QWORD(DepthTestEnable,       DepthWriteMask,   DepthFunc,            StencilTestEnable);
	UPDATE_CACHED_QWORD(StencilTwoSidedEnable, StencilWriteMask, BackStencilWriteMask, StencilFail);
	UPDATE_CACHED_QWORD(StencilDepthFail,      StencilPass,      StencilFunc,          StencilRef);
	UPDATE_CACHED_QWORD(StencilCmpMask,        BackStencilFail,  BackStencilDepthFail, BackStencilPass);

	// Last qword doesn't use all words for depth stencil state
#	define UPDATE_CACHED_FINAL_QWORD(STATE0, STATE1, STATE2, IGNORED_STATE3)   \
		CompileTimeAssert((OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) & 15) == 0); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE1) == OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) + 4); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE2) == OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0) + 8); \
		quad = si_selb(                                                        \
			si_from_uint(wanted##STATE2),                                      \
			si_shufb(si_from_uint(wanted##STATE0), si_from_uint(wanted##STATE1), shufAaAa), \
			maskff00);                                                         \
		quad = si_selb(quad, si_lqd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0)), si_fsmbi(0x000f)); \
		si_stqd(quad, si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.DepthStencil.STATE0))

	UPDATE_CACHED_FINAL_QWORD(BackStencilFunc, BackStencilRef, BackStencilCmpMask, ?);

#	undef  UPDATE_CACHED_FINAL_QWORD
#	undef  UPDATE_CACHED_QWORD

#  endif		// DUMB_SET_STATE
#endif

#if LAZY_STATEBLOCKS
	DSS_Flushed = newState;
	FlushedStencilRef = stencilRef;
#endif
}

#if __FINAL && !__PS3
const bool WireframeOverride = false;
#elif !__SPU
DECLARE_MTR_THREAD bool WireframeOverride = false;
#endif
void SetRasterizerState(grcRasterizerStateHandle newState)
{
	AssertMsg(newState != RS_Invalid, "SetRasterizerState called with RS_Invalid");

#if LAZY_STATEBLOCKS
	if (RS_Active != newState) {
		RS_Active = newState;
		Stateblock_Dirty |= RASTERIZER_STATE_DIRTY;
	}
}

void FlushRasterizerState(grcRasterizerStateHandle newState)
{
#else
#if !__SPU
	if (NOTFINAL_ONLY(!PARAM_nolazystateblock.Get() &&) RS_Active==newState && !RS_Dirty)
		return;
	RS_Dirty = false;
#endif

	RS_Active = newState;
#endif

#if __PPU
	SPU_COMMAND(grcStateBlock__SetRasterizerState,0);
	cmd->handle = newState;
#elif __D3D11

# if !__FINAL
	if (WireframeOverride)
		newState = RS_WireFrame;
# endif

	g_grcCurrentContext->RSSetState(RasterizerStateStore.GetState(newState));
#elif __WIN32
	const grcRasterizerState &data = RasterizerStateStore[newState];
	grcDeviceHandle *device = GRCDEVICE.GetCurrent();
	device->SetRenderState_Inline(D3DRS_FILLMODE,WireframeOverride? grcRSV::FILL_WIREFRAME : data.FillMode);
	device->SetRenderState_Inline(D3DRS_CULLMODE,data.CullMode);
	device->SetRenderState_Inline(D3DRS_DEPTHBIAS,data.DepthBias.u);
	device->SetRenderState_Inline(D3DRS_SLOPESCALEDEPTHBIAS,data.SlopeScaledDepthBias.u);
	device->SetRenderState_Inline(D3DRS_SCISSORTESTENABLE,TRUE);

# if __XENON
	device->SetRenderState_Inline(D3DRS_HALFPIXELOFFSET, data.HalfPixelOffset);
# endif

#elif RSG_ORBIS
	const grcRasterizerState &data = RasterizerStateStore[newState];
#if !__FINAL
	if (WireframeOverride) {
		sce::Gnm::PrimitiveSetup ps = data.ps;
		ps.setPolygonMode(sce::Gnm::kPrimitiveSetupPolygonModeLine, sce::Gnm::kPrimitiveSetupPolygonModeLine);
		gfxc.setPrimitiveSetup(ps);
	}
	else
#endif //!__FINAL
	gfxc.setPrimitiveSetup(data.ps);
	gfxc.setPolygonOffsetFront(data.SlopeScaledDepthBias.f,data.DepthBias.f);
	gfxc.setPolygonOffsetBack (data.SlopeScaledDepthBias.f,data.DepthBias.f);
	gfxc.setPolygonOffsetClamp(data.DepthBiasClamp);
#elif __SPU

	const grcRasterizerState &data = *s_RasterizerStateCache.Get((grcRasterizerState*)(pSpuGcmState->RasterizerStateStore + newState * sizeof(grcRasterizerState)));
	pSpuGcmState->EdgeCullMode = data.EdgeCullMode;

	qword *q = ReserveMethodSizeAligned(GCM_CONTEXT,4);
	uint32_t current0 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE, 1);
# if __FINAL
	uint32_t current1 = data.FillMode;
# else
	uint32_t current1 = WireframeOverride? grcRSV::FILL_WIREFRAME : data.FillMode;
# endif
	uint32_t current2 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BACK_POLYGON_MODE, 1);
	uint32_t current3 = current1;
	q[0] = MAKE_QWORD(current0,current1,current2,current3);

	uint32_t current4 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_CULL_FACE_ENABLE, 1);
	uint32_t current5 = data.CullFaceEnable;
	uint32_t current6 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_CULL_FACE, 1);
	uint32_t current7 = data.CullFace;
	q[1] = MAKE_QWORD(current4,current5,current6,current7);

	uint32_t current8 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE, 1);
	uint32_t current9 = data.PolygonOffsetFillEnable;
	uint32_t current10 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR, 2);
	uint32_t current11 = data.Offset0.u;
	q[2] = MAKE_QWORD(current8,current9,current10,current11);

	// Offset is encoded assuming a 16-bit depth buffer.  Multiply 256 (ie increase exponent by 8) if it's really 24 bit Z.
	uint32_t current12 = pSpuGcmState->SurfaceDepthFormat == CELL_GCM_SURFACE_Z24S8? data.Offset1.u + (8<<23) : data.Offset1.u;
	q[3] = MAKE_QWORD_ZZ(current12,0);
#endif

#if LAZY_STATEBLOCKS
	RS_Flushed = newState;
#endif
}

#if __D3D11 || RSG_ORBIS
void SetAnisotropicValue(u32 newAnisotropyValue)
{
	uOverrideAnisotropic = newAnisotropyValue;

	for( u32 i= 1; i < SamplerStateStore.GetMaxUsed(); i++)
	{
		grcSamplerStateDesc samplerState;
		GetSamplerStateDesc((grcSamplerStateHandle)i,samplerState);
		u32 anisotropyValue = samplerState.MaxAnisotropy;
		if( anisotropyValue != 0 )
			anisotropyValue = newAnisotropyValue;

		samplerState.MaxAnisotropy = newAnisotropyValue;
#if __D3D11
		SamplerStateStore.RecreateState((grcSamplerStateHandle)i, samplerState);
#elif RSG_ORBIS
		grcSamplerState &state = SamplerStateStore.GetStore()[i];
		state.s.setAnisotropyRatio(remapAnisotropic[newAnisotropyValue]);
		state.s.setLodBias(int(samplerState.MipLodBias * 256.0) & 0x3FFF,0);
#endif // __D3D11
	}
}
#endif //__D3D11 || RSG_ORBIS

void SetBlendState(grcBlendStateHandle newState) { SetBlendState(newState,~0U,~0ULL); }

void SetBlendState(grcBlendStateHandle newState,u32 blendFactors,u64 sampleMask)
{
	AssertMsg(newState != BS_Invalid, "SetBlendState called with BS_Invalid");
	//PIXAutoTag(0x00FFFF00, "SetBlendState sampleMask = %x", (int)sampleMask);	//TEMP
	
#if LAZY_STATEBLOCKS
	if (BS_Active != newState || ActiveBlendFactors != blendFactors || ActiveSampleMask != sampleMask) {
		BS_Active = newState;
		ActiveBlendFactors = blendFactors;
		ActiveSampleMask = sampleMask;
		Stateblock_Dirty |= BLEND_STATE_DIRTY;
	}
}

#if __D3D11
const void* GetDepthStencilStateRaw(grcDepthStencilStateHandle handle)
{
	return DepthStencilStateStore.GetState(handle);
}
#endif //__D3D11

void FlushBlendState(grcBlendStateHandle newState,u32 blendFactors,u64 sampleMask)
{
#else
#if !__SPU
	if (NOTFINAL_ONLY(!PARAM_nolazystateblock.Get() &&) BS_Active==newState && ActiveBlendFactors==blendFactors && ActiveSampleMask==sampleMask && !BS_Dirty)
		return;
	BS_Dirty = false;
#endif

	BS_Active = newState;
	ActiveBlendFactors = blendFactors;
	ActiveSampleMask = sampleMask;
#endif
#if __PPU
	SPU_COMMAND(grcStateBlock__SetBlendState,alphaRef);
	cmd->handle = newState;
	cmd->multisample = sampleMask;
	cmd->argb = blendFactors;
	g_MainColorWrite = BlendStateStore[newState].WriteMask123 & 0xF;	// grcDevice::Clear needs this
#else
# if __D3D11
	float floatBlendFactors[4];
	UnpackBlendFactors(floatBlendFactors, blendFactors);
	g_grcCurrentContext->OMSetBlendState(BlendStateStore.GetState(newState),floatBlendFactors,(u32)(sampleMask & 0xffffffff));
#if RSG_DURANGO
	g_grcCurrentContext->OMSetSampleMask(sampleMask);
#endif // RSG_DURANGO
# elif __WIN32
	const grcBlendState &data = BlendStateStore[newState];
	grcDeviceHandle *device = GRCDEVICE.GetCurrent();
#  if __XENON
# if LAZY_STATEBLOCKS
	if (BS_Flushed == newState) {
		if (FlushedBlendFactors != blendFactors)
			device->SetRenderState_Inline(D3DRS_BLENDFACTOR,FlushedBlendFactors = blendFactors);
		if (FlushedSampleMask != sampleMask)
			device->SetRenderState_Inline(D3DRS_MULTISAMPLEMASK,FlushedSampleMask = sampleMask);
		if (FlushedAlphaRef != alphaRef)
			device->SetRenderState_Inline(D3DRS_ALPHAREF,FlushedAlphaRef = alphaRef);
		return;
	}
#endif

	for (int i=0; i<4; i++)
		device->SetBlendState(i, data.Targets[i]);
	device->SetRenderState_Inline(D3DRS_COLORWRITEENABLE,data.WriteMasks[0]);
	device->SetRenderState_Inline(D3DRS_COLORWRITEENABLE1,data.WriteMasks[1]);
	device->SetRenderState_Inline(D3DRS_COLORWRITEENABLE2,data.WriteMasks[2]);
	device->SetRenderState_Inline(D3DRS_COLORWRITEENABLE3,data.WriteMasks[3]);
	device->SetRenderState_Inline(D3DRS_ALPHATOMASKENABLE, data.AlphaToCoverageEnable);
	device->SetRenderState_Inline(D3DRS_ALPHATOMASKOFFSETS, data.AlphaToMaskOffsets);
	device->SetRenderState_Inline(D3DRS_HIGHPRECISIONBLENDENABLE, data.HighPrecisionBlendEnable);

#  elif __WIN32PC
	device->SetRenderState(D3DRS_ALPHABLENDENABLE,data.Targets[0].BlendEnable);
	device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,TRUE);
	device->SetRenderState(D3DRS_SRCBLEND,data.Targets[0].SrcBlend);
	device->SetRenderState(D3DRS_DESTBLEND,data.Targets[0].DestBlend);
	device->SetRenderState(D3DRS_BLENDOP,data.Targets[0].BlendOp);
	device->SetRenderState(D3DRS_SRCBLENDALPHA,data.Targets[0].SrcBlendAlpha);
	device->SetRenderState(D3DRS_DESTBLENDALPHA,data.Targets[0].DestBlendAlpha);
	device->SetRenderState(D3DRS_BLENDOPALPHA,data.Targets[0].BlendOpAlpha);
	device->SetRenderState(D3DRS_COLORWRITEENABLE,data.Targets[0].RenderTargetWriteMask);
	device->SetRenderState(D3DRS_COLORWRITEENABLE1,data.Targets[1].RenderTargetWriteMask);
	device->SetRenderState(D3DRS_COLORWRITEENABLE2,data.Targets[2].RenderTargetWriteMask);
	device->SetRenderState(D3DRS_COLORWRITEENABLE3,data.Targets[3].RenderTargetWriteMask);
#  endif
	device->SetRenderState_Inline(D3DRS_MULTISAMPLEMASK,(int)sampleMask);

# elif RSG_ORBIS
	const grcBlendState &data = BlendStateStore[newState];
	for (int i=0; i<8; i++)
		gfxc.setBlendControl(i,data.bc[i]);
	// This could be precomputed easily
	gfxc.setRenderTargetMask(data.ColorWriteMask);
#  if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	sce::Gnm::AlphaToMaskControl atm;
	atm.init();
	atm.setEnabled( data.AlphaToCoverageEnable ? sce::Gnm::kAlphaToMaskEnable : sce::Gnm::kAlphaToMaskDisable );
	if (data.AlphaToCoverageEnable)
	{
		atm.setPixelDitherThresholds(
			static_cast<sce::Gnm::AlphaToMaskDitherThreshold>((data.AlphaToMaskOffsets>>0)&3),
			static_cast<sce::Gnm::AlphaToMaskDitherThreshold>((data.AlphaToMaskOffsets>>2)&3),
			static_cast<sce::Gnm::AlphaToMaskDitherThreshold>((data.AlphaToMaskOffsets>>4)&3),
			static_cast<sce::Gnm::AlphaToMaskDitherThreshold>((data.AlphaToMaskOffsets>>6)&3)
			);
		atm.setRoundMode( data.AlphaToMaskOffsets != grcRSV::ALPHATOMASKOFFSETS_SOLID ?
			sce::Gnm::kAlphaToMaskRoundDithered : sce::Gnm::kAlphaToMaskRoundNonDithered );
	}
	gfxc.setAlphaToMaskControl(atm);
#  else //SCE_ORBIS_SDK_VERSION
	gfxc.setAlphaToMask( data.AlphaToCoverageEnable ? sce::Gnm::kAlphaToMaskEnable : sce::Gnm::kAlphaToMaskDisable,
		(data.AlphaToMaskOffsets>>0)&3, (data.AlphaToMaskOffsets>>2)&3,
		(data.AlphaToMaskOffsets>>4)&3, (data.AlphaToMaskOffsets>>6)&3,
		data.AlphaToMaskOffsets != grcRSV::ALPHATOMASKOFFSETS_SOLID ? 1 : 0 );
#  endif //SCE_ORBIS_SDK_VERSION
	gfxc.setAaSampleMask( sampleMask );
	float bf[4];
	UnpackBlendFactors(bf,blendFactors);
	gfxc.setBlendColor(bf[0],bf[1],bf[2],bf[3]);
# elif __SPU

#  if DUMB_SET_STATE
	const grcBlendState &data = *s_BlendStateCache.Get((grcBlendState*)(pSpuGcmState->BlendStateStore + newState * sizeof(grcBlendState)));
	qword nops = si_il(0);

	u32 color0 = blendFactors, color1 = 0;
	if (Unlikely(pSpuGcmState->SurfaceColorFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)) {
		color1 = (color0 & 0xFF000000) | ((color0 >> 8) & 0xFFFF00) | ((color0 >> 16) & 0xFF); // XXYY---- -> XXXXYYYY
		color0 = ((color0 << 16) & 0xFF000000) | ((color0 << 8) & 0xFFFF00) | (color0 & 0xFF); // ----XXYY -> XXXXYYYY
	}
	qword *q = ReserveMethodSizeAligned(GCM_CONTEXT,5);
	uint32_t current0 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_ENABLE, 1);
	uint32_t current1 = data.Enable0;
	uint32_t current2 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT, 1);
	uint32_t current3 = data.Enable123 << 1;
	q[0] = MAKE_QWORD(current0,current1,current2,current3);

	uint32_t current4 = CELL_GCM_METHOD_NOP();
	uint32_t current5 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_COLOR, 1);
	uint32_t current6 = color0;
	uint32_t current7 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_COLOR2, 1);
	q[1] = MAKE_QWORD(current8,current9,current10,current11);

	uint32_t current8  = color1;
	uint32_t current9  = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_EQUATION, 1);
	uint32_t current10 = (data.BlendOp | 0x8000) | ((data.BlendOpAlpha | 0x8000) << 16);
	uint32_t current11 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR, 2); 
	q[2] = MAKE_QWORD(current12,current13,current14,current15);

	uint32_t current12 = data.SrcBlend;
	uint32_t current13 = data.DestBlend;
	uint32_t current14 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL, 1);
	// (cmd)[1] = CELL_GCM_ENDIAN_SWAP((enable) | ((alphaToCoverage) << 4) | ((alphaToOne) << 8) | ((sampleMask) << 16)); 
	uint32_t current15 = ((g_RenderStateBlockMask & MSAA_ENABLED) != 0) | ((g_RenderStateBlockMask & BLOCK_ALPHATOMASK)? 0 : (data.WriteMask0 & 16)) | (0 << 8) | (sampleMask << 16);
	q[3] = MAKE_QWORD(current16,current17,current18,current19);

	uint32_t current16 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_COLOR_MASK, 1);
	uint32_t current17 = data.WriteMask0 & ~16;		// We store alpha to coverage enable here
	uint32_t current18 = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_COLOR_MASK_MRT, 1);
	uint32_t current19 = data.WriteMask123 & ~0xF;
	q[4] = (g_RenderStateBlockMask & BLOCK_COLORWRITE)? nops : MAKE_QWORD(current20,current21,current22,current23);

#  else // DUMB_SET_STATE
	spuGcmState *const pState = pSpuGcmState;
	const grcBlendState &data = *s_BlendStateCache.Get((grcBlendState*)(pSpuGcmState->BlendStateStore + newState * sizeof(grcBlendState)));


	u32 color = blendFactors, color2 = 0;
	if (Unlikely(pState->SurfaceColorFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)) {
		color  = (color & 0xFF000000) | ((color >> 8) & 0xFFFF00) | ((color >> 16) & 0xFF); // XXYY---- -> XXXXYYYY
		color2 = ((color << 16) & 0xFF000000) | ((color << 8) & 0xFFFF00) | (color & 0xFF); // ----XXYY -> XXXXYYYY
	}

	u32 wantedBlendEnable           = data.Enable0;
	u32 wantedBlendEnableMrt        = data.Enable123 << 1;
	u32 wantedBlendColor            = color;
	u32 wantedBlendColor2           = color2;
	u32 wantedBlendEquation         = (data.BlendOp | 0x8000) | ((data.BlendOpAlpha | 0x8000) << 16);
	u32 wantedBlendSrcFactor        = data.SrcBlend;
	u32 wantedBlendDstFactor        = data.DestBlend;
	u32 wantedAntiAliasingControl   = ((g_RenderStateBlockMask & MSAA_ENABLED) != 0) | ((g_RenderStateBlockMask & BLOCK_ALPHATOMASK)? 0 : (data.WriteMask0 & 16)) | (0 << 8) | (sampleMask << 16);
	u32 wantedColorMask             = data.WriteMask0 & ~16;		// We store alpha to coverage enable here
	u32 wantedColorMaskMrt          = data.WriteMask123 & ~0xF;

	u32 *cmd = EnsureMethodSpaceWords(GCM_CONTEXT, 24);

	DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(const bool forceDiff = pSpuGcmState->DebugSetAllStates;)

	if (pState->CachedStates.Blend.BlendEnable != wantedBlendEnable DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_ENABLE, 1);
		cmd[1] = wantedBlendEnable;
		cmd += 2;
	}
	if (pState->CachedStates.Blend.BlendEnableMrt != wantedBlendEnableMrt DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT, 1);
		cmd[1] = wantedBlendEnableMrt;
		cmd += 2;
	}

	qword updateQuad2 = si_il(0);
	qword updateQuad3 = si_fsmbi(0x0f00);
	if (wantedBlendEnable | wantedBlendEnableMrt DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		updateQuad2 = si_il(-1);
		updateQuad3 = si_or(updateQuad3, si_fsmbi(0xf000));
		if (pState->CachedStates.Blend.BlendColor != wantedBlendColor || pState->CachedStates.Blend.BlendColor2 != wantedBlendColor2 DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
		{
			cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_COLOR, 1);
			cmd[1] = wantedBlendColor;
			cmd[2] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_COLOR2, 1);
			cmd[3] = wantedBlendColor2;
			cmd += 4;
		}
		if (pState->CachedStates.Blend.BlendEquation != wantedBlendEquation DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
		{
			cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_EQUATION, 1);
			cmd[1] = wantedBlendEquation;
			cmd += 2;
		}
		if (pState->CachedStates.Blend.BlendSrcFactor != wantedBlendSrcFactor || pState->CachedStates.Blend.BlendDstFactor != wantedBlendDstFactor DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
		{
			cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR, 2);
			cmd[1] = wantedBlendSrcFactor;
			cmd[2] = wantedBlendDstFactor;
			cmd += 3;
		}
	}


	if (pState->CachedStates.Blend.AntiAliasingControl != wantedAntiAliasingControl DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL, 1);
		cmd[1] = wantedAntiAliasingControl;
		cmd += 2;
	}

	if ((~g_RenderStateBlockMask & BLOCK_COLORWRITE) DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
	{
		updateQuad3 = si_or(updateQuad3, si_fsmbi(0x00ff));
		if (pState->CachedStates.Blend.ColorMask != wantedColorMask DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
		{
			cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_COLOR_MASK, 1);
			cmd[1] = wantedColorMask;
			cmd += 2;
		}
		if (pState->CachedStates.Blend.ColorMaskMrt != wantedColorMaskMrt DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY(|| forceDiff))
		{
			cmd[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_COLOR_MASK_MRT, 1);
			cmd[1] = wantedColorMaskMrt;
			cmd += 2;
		}
	}

	GCM_CONTEXT->current = cmd;

	qword quad;
	const qword shufAaAa = MAKE_QWORD_u32(0x00010203, 0x10111213, 0x00010203, 0x10111213);
	const qword maskff00 = si_fsmbi(0xff00);

	const qword origQuad1 = si_lqd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.BlendEnableMrt));
	const qword origQuad2 = si_lqd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.BlendColor));
	const qword origQuad3 = si_lqd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.BlendDstFactor));

#	define MASKED_UPDATE(ORIG, MASK)                                           \
		quad = si_selb((ORIG), quad, (MASK))

	// Copy the wanted values over to the cached state
#	define UPDATE_CACHED_QWORD(STATE0, STATE1, STATE2, STATE3, MASK_OPERATION) \
		CompileTimeAssert((OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE0) & 15) == 0); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE1) == OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE0) + 4); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE2) == OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE0) + 8); \
		CompileTimeAssert(OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE3) == OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE0) + 12); \
		quad = si_selb(                                                        \
			si_shufb(si_from_uint(wanted##STATE2), si_from_uint(wanted##STATE3), shufAaAa), \
			si_shufb(si_from_uint(wanted##STATE0), si_from_uint(wanted##STATE1), shufAaAa), \
			maskff00);                                                         \
		MASK_OPERATION;                                                        \
		si_stqd(quad, si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE0))

	// Firt states don't start on a qword boundary
#	define UPDATE_CACHED_FIRST_QWORD(IGNORED_STATE0, IGNORED_STATE1, IGNORED_STATE2, STATE3, MASK_OPERATION) \
		CompileTimeAssert((OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE3) & 15) == 12); \
		quad = si_lqd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE3)&~15); \
		quad = si_shufb(si_from_uint(wanted##STATE3), quad,                    \
			si_cwd(si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE3)&15)); \
		MASK_OPERATION;                                                        \
		si_stqd(quad, si_from_ptr(pState), OffsetOf(spuGcmStateBase, CachedStates.Blend.STATE3)&~15)

	UPDATE_CACHED_FIRST_QWORD(?,        ?,                   ?,             BlendEnable,    );
	UPDATE_CACHED_QWORD(BlendEnableMrt, si_il(0),            si_il(0),      si_il(0),       MASKED_UPDATE(origQuad1, si_fsmbi(0xf000)));
	UPDATE_CACHED_QWORD(BlendColor,     BlendColor2,         BlendEquation, BlendSrcFactor, MASKED_UPDATE(origQuad2, updateQuad2));
	UPDATE_CACHED_QWORD(BlendDstFactor, AntiAliasingControl, ColorMask,     ColorMaskMrt,   MASKED_UPDATE(origQuad3, updateQuad3));

#	undef  UPDATE_CACHED_FIRST_QWORD
#	undef  UPDATE_CACHED_QWORD
#	undef  MASKED_UPDATE

#  endif		// DUMB_SET_STATE
# endif
#endif

#if LAZY_STATEBLOCKS
	BS_Flushed = newState;
	FlushedBlendFactors = blendFactors;
	FlushedSampleMask = sampleMask;
#endif
}

#if LAZY_STATEBLOCKS
void DoFlush()
{
	// TODO: Add extra layer of checking so that we can do minimal updates on DX9 targets when only secondary factors have changed
	if ((Stateblock_Dirty & DEPTH_STENCIL_STATE_DIRTY) && (DSS_Flushed != DSS_Active || FlushedStencilRef != ActiveStencilRef))
		FlushDepthStencilState(DSS_Active,ActiveStencilRef);
	if ((Stateblock_Dirty & RASTERIZER_STATE_DIRTY) && (RS_Flushed != RS_Active))
		FlushRasterizerState(RS_Active);
	if ((Stateblock_Dirty & BLEND_STATE_DIRTY) && (BS_Flushed != BS_Active || FlushedBlendFactors != ActiveBlendFactors || FlushedSampleMask != ActiveSampleMask))
		FlushBlendState(BS_Active,ActiveBlendFactors,ActiveSampleMask);
	Stateblock_Dirty = 0;
}
#endif

#if !__FINAL && !__SPU
// If this is true, the next SetRasterizerState call will force the fill mode to wireframe regardless of
// what is really in the state handle.  If it is false, the call will behave normally.
bool SetWireframeOverride(bool enable)
{
	bool prev = WireframeOverride; 
	WireframeOverride = enable; 
#if __PPU
	SPU_SIMPLE_COMMAND(grcStateBlock__SetWireframeOverride,enable);
	GCM_STATE(SetFrontPolygonMode,enable?grcRSV::FILL_WIREFRAME : grcRSV::FILL_SOLID);
	GCM_STATE(SetBackPolygonMode,enable?grcRSV::FILL_WIREFRAME : grcRSV::FILL_SOLID);
#elif __D3D11
	// The next SetRasterizerState will set a different state block with wireframe enabled.
	if (prev != WireframeOverride)
	{
		Stateblock_Dirty |= RASTERIZER_STATE_DIRTY;
	}
#elif __WIN32
	GRCDEVICE.GetCurrent()->SetRenderState_Inline(D3DRS_FILLMODE,enable? grcRSV::FILL_WIREFRAME : grcRSV::FILL_SOLID);
#endif
	return prev; 
}
#endif

void SetStates(grcRasterizerStateHandle rs,grcDepthStencilStateHandle dss,grcBlendStateHandle bs)
{
	SetRasterizerState(rs);
	SetDepthStencilState(dss);
	SetBlendState(bs);
}

void Default()
{
	SetStates(RS_Default,DSS_Default,BS_Default);
}

#if CACHESTATEBLOCKNAMES_D3D11
void SetName(grcBlendStateHandle blendState,const char* name)
{
	if(name!=NULL) {
		ID3D11BlendState* d3dState = BlendStateStore.GetState(blendState);
		ASSERT_ONLY( HRESULT hr = ) d3dState->SetPrivateData( WKPDID_D3DDebugObjectName, istrlen( name )+1, name );
		Assert( SUCCEEDED(hr) );
	}
}

void SetName(grcDepthStencilStateHandle depthStencilState,const char* name)
{
	if(name!=NULL) {
		ID3D11DepthStencilState* d3dState = DepthStencilStateStore.GetState(depthStencilState);
		ASSERT_ONLY( HRESULT hr = ) d3dState->SetPrivateData( WKPDID_D3DDebugObjectName, istrlen( name )+1, name );
		Assert( SUCCEEDED(hr) );
	}
}

void SetName(grcRasterizerStateHandle rasterizerState,const char* name)
{
	if(name!=NULL) {
		ID3D11RasterizerState* d3dState = RasterizerStateStore.GetState(rasterizerState);
		ASSERT_ONLY( HRESULT hr = ) d3dState->SetPrivateData( WKPDID_D3DDebugObjectName, istrlen( name )+1, name );
		Assert( SUCCEEDED(hr) );
	}
}

#else
void SetName(grcBlendStateHandle /*blendState*/,const char* /*name*/) {}
void SetName(grcDepthStencilStateHandle /*depthStencilState*/,const char* /*name*/) {}
void SetName(grcRasterizerStateHandle /*RasterizerState*/,const char* /*name*/) {}
#endif // CACHESTATEBLOCKNAMES_D3D11

 void MakeDirty()
 { 
#if LAZY_STATEBLOCKS
	 RS_Flushed = RS_Invalid; DSS_Flushed = DSS_Invalid; BS_Flushed = BS_Invalid; 
	 Stateblock_Dirty = BLEND_STATE_DIRTY | DEPTH_STENCIL_STATE_DIRTY | RASTERIZER_STATE_DIRTY;
#elif !__SPU
	 RS_Dirty = DSS_Dirty = BS_Dirty = true;
#endif
 }

bool IsComparisonSampler(u16 stateHandle)
{
	if (stateHandle != INVALID_STATEBLOCK)
	{
#if __D3D11
		D3D11_SAMPLER_DESC oDesc;
		g_SamplerStates11[stateHandle]->GetDesc(&oDesc);
		return oDesc.Filter > D3D11_FILTER_ANISOTROPIC;
#elif RSG_ORBIS
		return g_SamplerStates[stateHandle].s.getDepthCompareFunction() != sce::Gnm::kDepthCompareNever;
#else
		return false;
#endif //platforms
	}else
	{
		return false;
	}
}

}	// namespace grcStateBlock

}	// namespace rage

#if __WIN32PC
#undef SetRenderState_Inline
#endif
