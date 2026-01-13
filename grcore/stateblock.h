// 
// grcore/stateblock.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_STATEBLOCK_H 
#define GRCORE_STATEBLOCK_H 

#include "grcore/config.h"
#include "grcore/effect_values.h"

#if RSG_DURANGO
#include <xdk.h>
#endif // RSG_DURANGO

// You generally should be using grcStateBlock::RS_Invalid/DSS/BS/SS instead of this value.
#define INVALID_STATEBLOCK				0

#define LAZY_STATEBLOCKS				(__WIN32 || RSG_ORBIS)

#define TRACK_DEPTH_BOUNDS_STATE		((RSG_ORBIS && __DEV) || (RSG_DURANGO && _XDK_VER >= 10542))

#define TRACK_DEFAULT_DEPTHFUNC			0

namespace rage {

// These state block descriptions are temporary, only necessary for registering the underlying state block object.
// The state block object is API-dependent.
// The descriptions are chosen to be binary-compatible with DX10 definitions.

#define MAX_RAGE_SAMPLERS			16

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
#define MAX_RAGE_RENDERTARGET_COUNT	8
#else
#define MAX_RAGE_RENDERTARGET_COUNT	4
#endif

__forceinline int FixupDepthDirection(int depthfunc)
{
#if USE_INVERTED_PROJECTION_ONLY
#if __D3D11
	const u8 reverseDirection[] = { 0, // padded by one 
#else	
	const u8 reverseDirection[] = {
#endif
		grcRSV::CMP_NEVER, grcRSV::CMP_GREATER, grcRSV::CMP_EQUAL, grcRSV::CMP_GREATEREQUAL, grcRSV::CMP_LESS, grcRSV::CMP_NOTEQUAL, grcRSV::CMP_LESSEQUAL,	grcRSV::CMP_ALWAYS 
	};
	return reverseDirection[depthfunc];
#else // USE_INVERTED_PROJECTION_ONLY
	return depthfunc;
#endif // USE_INVERTED_PROJECTION_ONLY
}


struct grcDepthStencilStateDesc
{
	grcDepthStencilStateDesc() : 
		DepthEnable(true), 
		DepthWriteMask(true), 
#if TRACK_DEFAULT_DEPTHFUNC
		DepthFunc(0xff),
#else
		DepthFunc(grcRSV::CMP_LESS),
#endif
		StencilEnable(false), 
		StencilReadMask(0xFF), 
		StencilWriteMask(0xFF)
#if RSG_ORBIS
		,DepthBoundsEnable(false)
#endif
#if RSG_DURANGO && _XDK_VER >= 10542
		//D3D11X extensions
		,BackfaceEnable(false)
		,DepthBoundsEnable(false)
		,ColorWritesOnDepthFailEnable(false)
		,ColorWritesOnDepthPassDisable(false)
// dangerous extra state removed until it can be supported correctly
//		,StencilReadMaskBack(0)
//		,StencilWriteMaskBack(0)
//		,StencilTestRefValueFront(0)
//		,StencilTestRefValueBack(0)
//		,StencilOpRefValueFront(0)
//		,StencilOpRefValueBack(0)
#endif //RSG_DURANGO && _XDK_VER >= 10542
		{ }

#if __ASSERT
	bool operator ==(const grcDepthStencilStateDesc &d) const
	{
		return DepthEnable == d.DepthEnable &&
			(!DepthEnable || (
				DepthWriteMask == d.DepthWriteMask &&
				DepthFunc == d.DepthFunc
			)) &&
			StencilEnable == d.StencilEnable &&
			(!StencilEnable || (
				StencilReadMask == d.StencilReadMask &&
				StencilWriteMask == d.StencilWriteMask &&
				FrontFace == d.FrontFace &&
				BackFace == d.BackFace
			));
	}
#endif //__ASSERT


	struct grcStencilOpDesc
	{
		grcStencilOpDesc() : 
			StencilFailOp(grcRSV::STENCILOP_KEEP), 
			StencilDepthFailOp(grcRSV::STENCILOP_KEEP), 
			StencilPassOp(grcRSV::STENCILOP_KEEP), 
			StencilFunc(grcRSV::CMP_ALWAYS) { }

#if __ASSERT
		bool operator ==(const grcStencilOpDesc &d) const
		{
			return StencilFailOp == d.StencilFailOp &&
				StencilDepthFailOp == d.StencilDepthFailOp &&
				StencilPassOp == d.StencilPassOp &&
				StencilFunc == d.StencilFunc;
		}
#endif //__ASSERT

		int StencilFailOp,
			StencilDepthFailOp,
			StencilPassOp,
			StencilFunc;
	};

	int DepthEnable;
	int DepthWriteMask;
	int DepthFunc;
	int StencilEnable;
	u8 StencilReadMask;
	u8 StencilWriteMask;
	grcStencilOpDesc FrontFace, BackFace;

#if RSG_ORBIS
	int DepthBoundsEnable;
#endif //RSG_ORBIS

#if RSG_DURANGO && _XDK_VER >= 10542
	//D3D11X extensions
	int BackfaceEnable;
	int DepthBoundsEnable;
	int ColorWritesOnDepthFailEnable;
	int ColorWritesOnDepthPassDisable;
	u8 StencilReadMaskBack;
	u8 StencilWriteMaskBack;
	u8 StencilTestRefValueFront;
	u8 StencilTestRefValueBack;
	u8 StencilOpRefValueFront;
	u8 StencilOpRefValueBack;
#endif //RSG_DURANGO && _XDK_VER >= 10542
};

struct grcRasterizerStateDesc
{
	grcRasterizerStateDesc() : 
		FillMode(grcRSV::FILL_SOLID) 
		,CullMode(grcRSV::CULL_BACK) 
		,FrontCounterClockwise(true)
		,DepthBiasDX10(0)
		,DepthBiasClamp(0.0f)
		,SlopeScaledDepthBias(0.0f)
		,DepthClipEnable(true)
		,ScissorEnable(true)
		,MultisampleEnable(false)	// would only affect lines under DX10.1+ anyway
		,AntialiasedLineEnable(false)
		,HalfPixelOffset(false),
		DepthBiasDX9(0.0f) 
		{ }

	int FillMode;
	int CullMode;
	int FrontCounterClockwise;
	int DepthBiasDX10;				// (this is converted into a float on DX9-era hardware)
	float DepthBiasClamp;			// (this has no equivalent on DX9-era hardware)
	float SlopeScaledDepthBias;
	int DepthClipEnable;
	int ScissorEnable;
	int MultisampleEnable;
	int AntialiasedLineEnable;
	int HalfPixelOffset;			// DX9 Extension
	float DepthBiasDX9;				// DX9 Extension
};

struct grcBlendStateDesc 
{
	grcBlendStateDesc() : 
		AlphaToCoverageEnable(false)
		,IndependentBlendEnable(false)
		,AlphaToMaskOffsets(grcRSV::ALPHATOMASKOFFSETS_SOLID)
	{
	}

	struct grcRenderTargetBlendDesc
	{
		grcRenderTargetBlendDesc() : 
			BlendEnable(false),
			SrcBlend(grcRSV::BLEND_ONE), 
			DestBlend(grcRSV::BLEND_ZERO), 
			BlendOp(grcRSV::BLENDOP_ADD), 
			SrcBlendAlpha(grcRSV::BLEND_ONE), 
			DestBlendAlpha(grcRSV::BLEND_ZERO), 
			BlendOpAlpha(grcRSV::BLENDOP_ADD),
			RenderTargetWriteMask(grcRSV::COLORWRITEENABLE_ALL)
		{
		}

		int BlendEnable;
		int SrcBlend;
		int DestBlend;
		int BlendOp;
		int SrcBlendAlpha;
		int DestBlendAlpha;
		int BlendOpAlpha;
		u8  RenderTargetWriteMask;
	};

	int AlphaToCoverageEnable;
	int IndependentBlendEnable;

	grcRenderTargetBlendDesc BlendRTDesc[MAX_RAGE_RENDERTARGET_COUNT];

	int AlphaToMaskOffsets;			// 360 extension
};

struct grcSamplerStateDesc
{
	grcSamplerStateDesc() : 
		Filter(grcSSV::FILTER_MIN_MAG_MIP_LINEAR)
		,AddressU(grcSSV::TADDRESS_WRAP)
		,AddressV(grcSSV::TADDRESS_WRAP)
		,AddressW(grcSSV::TADDRESS_WRAP)
		,MipLodBias(0.0f)
		,MaxAnisotropy(1)
		,ComparisonFunc(grcRSV::CMP_NEVER)
		,BorderColorRed(0), BorderColorGreen(0),	BorderColorBlue(0), BorderColorAlpha(0)
		,MinLod(0.0f)
		,MaxLod(12.0f)
		,TrilinearThresh(grcSSV::TRILINEAR_ONEFOURTH)
#if __D3D9
		,TextureZFunc(grcSSV::TEXTUREZFUNC_GREATER)
#endif // __D3D9
		,AlphaKill(false)
	{
	}

	int Filter;						// FILTER_... type (new to D3D10; subsumes old Min/Mag/Mip states.)
	int AddressU,
		AddressV,
		AddressW;
	float MipLodBias;
	u32 MaxAnisotropy;
	int ComparisonFunc;				// (only used by certain filter modes, DX10 only)
	float BorderColorRed, BorderColorGreen,BorderColorBlue,BorderColorAlpha;
	float MinLod;
	float MaxLod;
	int TrilinearThresh;			// 360/PS3 extension: trilinear threshold.
#if __D3D9
	int TextureZFunc;
#endif // __D3D9
	int AlphaKill;					// PS3 extension: alpha kill functionality, see cellGcmSetTextureControlAlphaKill docs
};

/*
	This class allows you to create, destroy, set, and get the various state block objects.

	TBD: Should we even provide a facility to get the current state?  That would encourage people
	to remember the current state and then reset it on the way out again, which causes us to make
	a lot of extra state calls that have to be weeded out by lazy state evaluations.
*/
namespace grcStateBlock
{
	void InitClass();
	void ShutdownClass();

	void FrackBlendStates(u32 rand1, u32 rand2);

	enum grcDepthStencilStateHandleEnum { DSS_Invalid = INVALID_STATEBLOCK };
	enum grcRasterizerStateHandleEnum { RS_Invalid = INVALID_STATEBLOCK };
	enum grcBlendStateHandleEnum { BS_Invalid = INVALID_STATEBLOCK };
	enum grcSamplerStateHandleEnum { SS_Invalid = INVALID_STATEBLOCK };
}

// In the current implementation it is always safe to store a handle in a u8 instead if you need the space.
typedef grcStateBlock::grcDepthStencilStateHandleEnum grcDepthStencilStateHandle;
typedef grcStateBlock::grcRasterizerStateHandleEnum grcRasterizerStateHandle;
typedef grcStateBlock::grcBlendStateHandleEnum grcBlendStateHandle;
typedef grcStateBlock::grcSamplerStateHandleEnum grcSamplerStateHandle;
CompileTimeAssert(sizeof(grcDepthStencilStateHandle) == sizeof(int));		// RemapRenderState[] temporary code in effect.h assumes this.

namespace grcStateBlock
{
	extern grcDepthStencilStateHandle DSS_Default;		// Test and write on, compare is CMP_LESS
	extern grcDepthStencilStateHandle DSS_LessEqual;	// same as Default, but compare function is CMP_LESSEQUAL
	extern grcDepthStencilStateHandle DSS_IgnoreDepth;	// function is CMP_ALWAYS, DepthWriteMask is false, DepthEnable is false.
	extern grcDepthStencilStateHandle DSS_ForceDepth;	// function is CMP_ALWAYS, DepthWriteMask is true, DepthEnable is true.
	extern grcDepthStencilStateHandle DSS_TestOnly;		// DepthWriteMask is false, function is CMP_LESS
	extern grcDepthStencilStateHandle DSS_TestOnly_LessEqual;	// DepthWriteMask is false, function is CMP_LESSEQUAL
	extern grcRasterizerStateHandle RS_Default;
	extern grcRasterizerStateHandle RS_NoBackfaceCull;
#if (__D3D11 || __GNM) && !__FINAL
	extern grcRasterizerStateHandle RS_WireFrame;
#endif
#if __XENON
	extern grcRasterizerStateHandle RS_Default_HalfPixelOffset;
#endif //__XENON
	extern grcBlendStateHandle BS_Default;
	extern grcBlendStateHandle BS_Default_WriteMaskNone;
	extern grcSamplerStateHandle SS_Default;

	// These match up with the old "blend sets" in grcState code:
	extern grcBlendStateHandle BS_Normal;
	extern grcBlendStateHandle BS_Add;
	extern grcBlendStateHandle BS_Subtract;
	extern grcBlendStateHandle BS_Min;
	extern grcBlendStateHandle BS_Max;
	extern grcBlendStateHandle BS_CompositeAlpha;
	extern grcBlendStateHandle BS_AlphaAdd;
	extern grcBlendStateHandle BS_AlphaSubtract;

	/* For all of the functions below:
		- Create...State will return a handle to a suitable object.  If the object already existed before,
			we increment the reference count on the previous object and return it again.
		- Destroy...State decrements the reference count on the object, freeing it if it goes to zero.
			Destroying state objects should not happen within a frame of them having last been used by the GPU!
		- Set...State makes that state block active on the GPU.  If there are extra parameters, these
			define states that are still "fine grained" and are not part of the state block.
		- Get...StateDesc will, given a handle, return a StateDesc to the caller that can be adjusted
			and passed back to a Create...State function.

		There are three basic groups of state (four if you include sampler states).  You create a stateblock by filling 
		out a structure and passing it to the appropriate Create function and get back a handle.

		To make that stateblock current, pass the handle into the matching Set function.  The handles are type-safe 
		on __DEV builds.  Some state is still fine-grained – things like the stencil reference value, or the blend 
		constant color, or the alpha reference value.  You can pass those into overloaded versions of the Set functions; 
		otherwise you get a default value for it.

		Making a stateblock current causes all its properties to be made active at once; setting a stateblock replaces 
		several individual grcState::Set… calls and will give us much better opportunities for redundant state elimination.  
		The biggest hassle with stateblocks is that you have to understand what the default values for states are, 
		and live with the fact that states you may not care about will still be affected by your code.  You’ll find 
		many cases where people have built several permutations of stateblocks they need ahead of time; this is fine, 
		within reason, because duplicate stateblocks are merged and reference-counted.

		In general you shouldn’t be constantly creating and destroying stateblocks.  In the current implementation, 
		destroying a stateblock you just used that frame has undefined results.  (If you destroy a stateblock and 
		immediately create a new one, you are guaranteed to get the same slot you had before since by design there 
		is no multithreaded support; all stateblocks should be created either at init time or in the render thread).

		You can access the currently active state block handle but it’s currently undocumented because using any 
		old-style grcState::Set… function will invalidate the associated active stateblock.  Once grcState:: is gone 
		there will be an official interface so you can save and restore the active stateblock if you want (under the 
		hood the grcEffect system uses this to make sure that stateblock state is unchanged after you’ve finished 
		rendering a drawable etc).

		You can also retrieve the properties of any given state block handle, although I don’t expect this to be used very often.
	*/

	grcDepthStencilStateHandle CreateDepthStencilState(const grcDepthStencilStateDesc &desc, const char* name=0);
	void DestroyDepthStencilState(grcDepthStencilStateHandle);
	void SetDepthStencilState(grcDepthStencilStateHandle newState,unsigned stencilRef);
	void SetDepthStencilState(grcDepthStencilStateHandle newState);
	void GetDepthStencilStateDesc(grcDepthStencilStateHandle handle,grcDepthStencilStateDesc& outDesc);

	grcRasterizerStateHandle CreateRasterizerState(const grcRasterizerStateDesc &desc, const char* name=0);
	void DestroyRasterizerState(grcRasterizerStateHandle);
	void SetRasterizerState(grcRasterizerStateHandle newState);
	void GetRasterizerStateDesc(grcRasterizerStateHandle handle,grcRasterizerStateDesc& outDesc);


	inline unsigned PackBlendFactors(float blendFactors[4])
	{
		return blendFactors? (int(blendFactors[0] * 255.0f) << 16) | (int(blendFactors[1] * 255.0f) << 8) | int(blendFactors[2] * 255.0f) | (int(blendFactors[3] * 255.0f) << 24) : ~0U;
	}
	inline void UnpackBlendFactors(float dest[4],unsigned factors)
	{
		const float cvt = 1.0f / 255;
		dest[0] = ((factors >> 16) & 255) * cvt;
		dest[1] = ((factors >> 8) & 255) * cvt;
		dest[2] = (factors & 255) * cvt;
		dest[3] = (factors >> 24) * cvt;
	}
	grcBlendStateHandle CreateBlendState(const grcBlendStateDesc &desc, const char* name=0);
	void DestroyBlendState(grcBlendStateHandle);
	void SetBlendState(grcBlendStateHandle newState,unsigned blendFactors,u64 sampleMask);
	inline void SetBlendState(grcBlendStateHandle newState,float blendFactors[4],u64 sampleMask)
	{
		SetBlendState(newState,PackBlendFactors(blendFactors),sampleMask);
	}
	void SetBlendState(grcBlendStateHandle newState);
	void GetBlendStateDesc(grcBlendStateHandle handle,grcBlendStateDesc& outDesc);

	grcSamplerStateHandle CreateSamplerState(const grcSamplerStateDesc &desc);
	void DestroySamplerState(grcSamplerStateHandle);
	void GetSamplerStateDesc(grcSamplerStateHandle handle,grcSamplerStateDesc& outDesc);
#if __D3D11 || RSG_ORBIS
	void SetAnisotropicValue(u32 newAnisotropyValue);
#endif

	void SetStates(grcRasterizerStateHandle rs,grcDepthStencilStateHandle dss,grcBlendStateHandle bs);

	void Default();

	// Ability to attach a name to one of the state blocks (Currently of use DX11 only)
	void SetName( grcBlendStateHandle blendState,const char* name );
	void SetName( grcDepthStencilStateHandle depthStencilState,const char* name );
	void SetName( grcRasterizerStateHandle RasterizerState,const char* name );

#if !__FINAL && !__SPU
	bool SetWireframeOverride(bool enable);
#endif

#if __D3D11
	// can't forward-declare ID3D11DepthStencilState
	const void* GetDepthStencilStateRaw(grcDepthStencilStateHandle handle);
#endif

#if __SPU
#define BS_Active pSpuGcmState->bs_Active
#define BS_Previous pSpuGcmState->bs_Previous
#define DSS_Active pSpuGcmState->dss_Active
#define DSS_Previous pSpuGcmState->dss_Previous
#define RS_Active pSpuGcmState->rs_Active
#define RS_Previous pSpuGcmState->rs_Previous
#define ActiveStencilRef pSpuGcmState->activeStencilRef
#define PreviousStencilRef pSpuGcmState->previousStencilRef
#define ActiveAlphaRef pSpuGcmState->activeAlphaRef
#define PreviousAlphaRef pSpuGcmState->previousAlphaRef
#define ActiveBlendFactors pSpuGcmState->activeBlendFactors
#define PreviousBlendFactors pSpuGcmState->previousBlendFactors
#define ActiveSampleMask pSpuGcmState->activeSampleMask
#define PreviousSampleMask pSpuGcmState->previousSampleMask
#define BS_Flushed pSpuGcmState->bs_Flushed
#define DSS_Flushed pSpuGcmState->dss_Flushed
#define RS_Flushed pSpuGcmState->rs_Flushed
#define FlushedBlendFactors pSpuGcmState->flushedBlendFactors
#define FlushedSampleMask pSpuGcmState->flushedSampleMask
#define FlushedAlphaRef pSpuGcmState->flushedAlphaRef
#define FlushedStencilRef pSpuGcmState->flushedStencilRef
#define Stateblock_Dirty pSpuGcmState->stateblockDirty
# if !__FINAL
#define WireframeOverride (pSpuGcmState->wireframeOverride)
# endif
#else
	extern DECLARE_MTR_THREAD bool BS_Dirty, RS_Dirty, DSS_Dirty;
	extern DECLARE_MTR_THREAD grcRasterizerStateHandle RS_Active, RS_Previous, RS_Flushed;
	extern DECLARE_MTR_THREAD grcDepthStencilStateHandle DSS_Active, DSS_Previous, DSS_Flushed;
	extern DECLARE_MTR_THREAD grcBlendStateHandle BS_Active, BS_Previous, BS_Flushed;

	extern DECLARE_MTR_THREAD u8 ActiveStencilRef, PreviousStencilRef;
	extern DECLARE_MTR_THREAD u32 ActiveBlendFactors, PreviousBlendFactors;
	extern DECLARE_MTR_THREAD u64 ActiveSampleMask, PreviousSampleMask;
	NOTFINAL_ONLY(extern DECLARE_MTR_THREAD bool WireframeOverride);
#endif

#if LAZY_STATEBLOCKS
# if __SPU
	inline void Flush();	// Defined in drawablespu, nobody else should need it
# else
	extern DECLARE_MTR_THREAD u8 Stateblock_Dirty;
	void DoFlush();
	inline void Flush() { if (Stateblock_Dirty) DoFlush(); }
# endif
#else
	inline void Flush() { }
#endif

	void MakeDirty();
#if __PPU && LAZY_STATEBLOCKS
	inline void FlushAndMakeDirty() { if (Stateblock_Dirty) DoFlush(); MakeDirty(); }
	#define FlushThrough(force) Flush(); SPU_SIMPLE_COMMAND(grcStateBlock__Flush,force)			// NOT multistatement-safe, use with care (done this way to deal with likely namespace prefix)

#else
	inline void FlushAndMakeDirty() { }
	inline void FlushThrough(bool) { }
#endif

	bool IsComparisonSampler(u16 stateHandle);
};

} // namespace rage

#endif // GRCORE_STATEBLOCK_H 
