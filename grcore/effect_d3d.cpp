// 
// grcore/effect_d3d.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "effect.h"
#include "effect.inl"
#include "channel.h"
#include "stateblock_internal.h"

#if __WIN32

#include "file/asset.h"
#include "file/device.h"
#include "file/stream.h"
#include "string/stringhash.h"
#include "system/cache.h"
#include "system/platform.h"
#include "system/magicnumber.h"
#include "vector/vector4.h"
#include "atl/array_struct.h"

#include "effect_values.h"
#include "texture.h"
#include "system/nelem.h"
#include "system/codecheck.h"
#include "grcore/channel.h"
#if __BANK
#include "bank/bank.h"
#endif

#if __WIN32
#include "device.h"
#include "system/xtl.h"
#include "system/d3d9.h"
#include "grcore/effect_internal.h"
#include "resourcecache.h"
#include "grcore/stateblock.h"
#elif __PPU
#include "wrapper_gcm.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "patcherspu.h"
#include "system/task.h"
#endif

#if __WIN32PC
#include "grcore/d3dwrapper.h"
#endif

#if __XENON
#include "grcore/texturexenon.h"
#include "grcore/texturereference.h"
#endif

#if __D3D
#include "grcore/wrapper_d3d.h"
#endif

#if __D3D11
#include "shaderlib/rage_constants.h"
#endif

#define SURFACE IDirect3DSurface9
#define TEXTURE IDirect3DBaseTexture9

#define STALL_DETECTION	0
#if STALL_DETECTION
#define STALL_ONLY_RENDERTHREAD(x) x
#define STALL_TIME 0.05f
#include "system/timer.h"
#endif

namespace rage {

#if __WIN32
extern grcSamplerState *g_SamplerStates;
ID3D11SamplerState **g_SamplerStates11;

#if (RSG_PC || RSG_DURANGO)
DECLARE_MTR_THREAD grcVertexShader *s_VertexShader;
DECLARE_MTR_THREAD grcPixelShader *s_PixelShader = (grcPixelShader*) ~0U;
DECLARE_MTR_THREAD grcComputeShader *s_ComputeShader = (grcComputeShader*) ~0U;
DECLARE_MTR_THREAD grcDomainShader *s_DomainShader = (grcDomainShader*) ~0U;
DECLARE_MTR_THREAD grcGeometryShader *s_GeometryShader = (grcGeometryShader*) ~0U;
DECLARE_MTR_THREAD grcHullShader *s_HullShader = (grcHullShader*) ~0U;
#if RSG_DURANGO
DECLARE_MTR_THREAD ID3D11InputLayout *s_InputLayout = (ID3D11InputLayout*) ~0U;
#endif
#else
static IDirect3DVertexShader9 *s_VertexShader;
static IDirect3DPixelShader9 *s_PixelShader = (IDirect3DPixelShader9*) ~0U;
static void *s_ComputeShader = (void*) ~0U;
static void *s_DomainShader = (void*) ~0U;
static void *s_GeometryShader = (void*) ~0U;
static void *s_HullShader = (void*) ~0U;

#if __DEV
u32 g_MaxTextureSize = 0;
#endif

#endif // (RSG_PC || RSG_DURANGO)


#if !__D3D11
void grcVertexProgram::Bind() const
{
	if (s_VertexShader != Program)
#if RSG_PC
		GRCDEVICE.SetVertexShader(s_VertexShader = Program,this);
#else
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShader(s_VertexShader = Program));
		if (Constants)
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShaderConstantF(ConstantBase,Constants,ConstantCount));
#endif
}

void grcFragmentProgram::Bind() const
{
	if (s_PixelShader != Program)
#if RSG_PC
		GRCDEVICE.SetPixelShader(s_PixelShader = Program, this);
#else
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetPixelShader(s_PixelShader = Program));
#endif
}

#if !__XENON

void grcComputeProgram::Bind() const
{
}

void grcDomainProgram::Bind() const
{
}

void grcGeometryProgram::Bind() const
{
}

void grcHullProgram::Bind() const
{
}

#endif // !__XENON

#endif // !__D3D11


#if RSG_PC && !__D3D11
bool grcVertexProgram::SetLocalParameter(int address, float *data,int count, u32 offset, grcCBuffer *pEffectVar, u8 type)
{
#if __D3D9
	(void)type;
	(void)pEffectVar;
	(void)offset;
	GRCDEVICE.SetVertexShaderConstantF(address,data,count);
	return true;
#else
	AssertMsg(NULL, "grcVertexProgram::SetLocalParameter()...Unsupported\n");
	return false;
#endif
}

void grcVertexProgram::SetLocalFlag(int address,bool value, u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x01);
		GRCDEVICE.SetVertexShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetVertexShaderConstantB(address,value);
	}
}

void grcVertexProgram::SetParameter(int address, const float *data,int count, u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x01);
		GRCDEVICE.SetVertexShaderConstantF(address,data,count,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetVertexShaderConstantF(address,data,count);
	}
}

void grcVertexProgram::SetParameterW(int address, const float *data,int count, u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x01);
		GRCDEVICE.SetVertexShaderConstantFW(address,data,count,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetVertexShaderConstantFW(address,data,count);
	}
}

void grcVertexProgram::SetFlag(int address,bool value, u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x01);
		GRCDEVICE.SetVertexShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetVertexShaderConstantB(address,value);
	}
}

void grcVertexProgram::SetFlag(int address,int value, u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x01);
		GRCDEVICE.SetVertexShaderConstantI(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetVertexShaderConstantI(address,value);
	}
}

#elif __XENON

void grcVertexProgram::SetParameter(int address, const float *data,int count)
{
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShaderConstantF(address,data,count));
}

void grcVertexProgram::SetFlag(int address,bool value)
{
	BOOL b = value;
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShaderConstantB(address,&b,1));
}
void grcVertexProgram::SetFlag(int address,int value)
{
	int iv[4] = {value};
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShaderConstantI(address,iv,1));
}

#endif // __XENON

#if __XENON
const u32 MaxSampler = 26;
#else
const u32 MaxSampler = 256 + 32;
#endif

#if (RSG_PC || RSG_DURANGO)
static DECLARE_MTR_THREAD grcTexture *s_TextureShadow[MaxSampler];
static DECLARE_MTR_THREAD u32 s_SamplerShadow[MaxSampler][grcessCOUNT];
#else
static grcTextureObject *s_TextureShadow[MaxSampler];
static u32 s_SamplerShadow[MaxSampler][grcessCOUNT];
#endif // __WIN32PC

#if __WIN32PC
static int DECLARE_MTR_THREAD s_StateShadow[grcersCOUNT];
static u16 DECLARE_MTR_THREAD s_SamplerStateShadow[MaxSampler];
#else
static int s_StateShadow[grcersCOUNT];
static u16 s_SamplerStateShadow[MaxSampler];
#endif

void grcEffect::ClearCachedState()
{
	sm_CurrentPass = NULL;
	sm_CurrentBind = NULL;

	s_VertexShader = NULL;
#if (RSG_PC || RSG_DURANGO)
	s_PixelShader = (grcPixelShader*) ~0U;
	s_ComputeShader = (grcComputeShader*) ~0U;
	s_DomainShader = (grcDomainShader*) ~0U;
	s_GeometryShader = (grcGeometryShader*) ~0U;
	s_HullShader = (grcHullShader*) ~0U;
#else
	s_PixelShader = (IDirect3DPixelShader9*) ~0U;
	s_ComputeShader = (void*) ~0U;
	s_DomainShader = (void*) ~0U;
	s_GeometryShader = (void*) ~0U;
	s_HullShader = (void*) ~0U;
#endif
	memset(s_SamplerShadow,0xCD,sizeof(s_SamplerShadow));
	memset(s_TextureShadow,0xCD,sizeof(s_TextureShadow));
	memset(s_StateShadow,0xCD,sizeof(s_StateShadow));
	memset(s_SamplerStateShadow,0xCD,sizeof(s_SamplerStateShadow));
}

void grcEffect::ClearCachedSamplerState()
{
	memset(s_SamplerStateShadow,0xCD,sizeof(s_SamplerStateShadow));
}

#if (RSG_PC || RSG_DURANGO)
#define SetShadowedSamplerState(sampler, state, stateId, value, version) \
	if (state <= D3DSAMP_DMAPOFFSET) \
	{ \
		if (s_SamplerShadow[sampler][stateId] != (value)) \
			GRCDEVICE.GetCurrent()->SetSamplerState(sampler,state,(s_SamplerShadow[sampler][stateId] = (value))); \
	}
#else
// We do this as a macro to increase the chances of 360 being able to fully inline the switch
#define SetShadowedSamplerState(sampler, state, stateId, value) \
        do { if (s_SamplerShadow[sampler][stateId] != (value)) \
                GRCDEVICE.GetCurrent()->SetSamplerState(sampler,state,(s_SamplerShadow[sampler][stateId] = (value))); } while (0)
#endif // __WIN32PC

#if __ASSERT && __XENON
PARAM(checksampleroverlap,"Check for sampler overlap; some false positives so off by default");
#endif

#if !__D3D11
#if __WIN32PC
void grcProgram::SetParameterCommon(u32 sampler,const /*grcTextureObject*/grcTexture  * __restrict data,u16 
#if !__RESOURCECOMPILER									
									samplerStateSet
#endif
									)
#else
void grcProgram::SetParameterCommon(u32 sampler,const grcTextureObject  * __restrict data,u16 samplerStateSet)
#endif
{
#if !__RESOURCECOMPILER
#if __DEV
	grcSamplerState ss = g_SamplerStates[samplerStateSet];
#else
	const grcSamplerState &ss = g_SamplerStates[samplerStateSet];
#endif	
#endif

	Assert(sampler < MaxSampler);

	if (!data) {
#if RSG_PC
		if (s_TextureShadow[sampler])
			GRCDEVICE.SetTexture(sampler,s_TextureShadow[sampler] = NULL);
#else
		if (s_TextureShadow[sampler]) {
			// IDirect3DDevice9::SetTexture(sampler,NULL) does not disable the
			// sampler correctly on 360 like it does for PC.  Instead we need to
			// manually set a black texture.
			s_TextureShadow[sampler] = NULL;

			static u32 black1x1[] =
			{
			    0x00000003, // DWORD Common = D3DRTYPE_TEXTURE
			    0x00000000, // DWORD ReferenceCount
			    0x00000000, // DWORD Fence
			    0x00000000, // DWORD ReadFence
			    0x00000000, // DWORD Identifier
			    0xffff0000, // DWORD BaseFlush
				0xffff0000, // DWORD MipFlush

				// GPUTEXTURE_FETCH_CONSTANT Format
				0x02000002, // dword[0]
							// DWORD Type           : 2;    // GPUCONSTANTTYPE  = 2
							// DWORD SignX          : 2;    // GPUSIGN          = 0
							// DWORD SignY          : 2;    // GPUSIGN          = 0
							// DWORD SignZ          : 2;    // GPUSIGN          = 0
							// DWORD SignW          : 2;    // GPUSIGN          = 0
							// DWORD ClampX         : 3;    // GPUCLAMP         = 0
							// DWORD ClampY         : 3;    // GPUCLAMP         = 0
							// DWORD ClampZ         : 3;    // GPUCLAMP         = 0
							// DWORD                : 2;                        = 0
							// DWORD                : 1;                        = 0
							// DWORD Pitch          : 9;    // DWORD            = 8
							// DWORD Tiled          : 1;    // BOOL             = 0

				0x00000002, // dword[1]
							// DWORD DataFormat     : 6;    // GPUTEXTUREFORMAT = 2
							// DWORD Endian         : 2;    // GPUENDIAN        = 0
							// DWORD RequestSize    : 2;    // GPUREQUESTSIZE   = 0
							// DWORD Stacked        : 1;    // BOOL             = 0
							// DWORD ClampPolicy    : 1;    // GPUCLAMPPOLICY   = 0
							// DWORD BaseAddress    : 20;   // DWORD            = 0

				0x00000000, // dword[2].TwoD
							// DWORD Width          : 13;   // DWORD            = 0
							// DWORD Height         : 13;   // DWORD            = 0
							// DWORD                : 6;                        = 0

				0x00001249, // dword[3]
							// DWORD NumFormat      : 1;    // GPUNUMFORMAT     = 1 = GPUNUMFORMAT_INTEGER
							// DWORD SwizzleX       : 3;    // GPUSWIZZLE       = 4 = GPUSWIZZLE_0
							// DWORD SwizzleY       : 3;    // GPUSWIZZLE       = 4 = GPUSWIZZLE_0
							// DWORD SwizzleZ       : 3;    // GPUSWIZZLE       = 4 = GPUSWIZZLE_0
							// DWORD SwizzleW       : 3;    // GPUSWIZZLE       = 4 = GPUSWIZZLE_0
							// INT   ExpAdjust      : 6;    // int              = 0
							// DWORD MagFilter      : 2;    // GPUMINMAGFILTER  = 0
							// DWORD MinFilter      : 2;    // GPUMINMAGFILTER  = 0
							// DWORD MipFilter      : 2;    // GPUMIPFILTER     = 0
							// DWORD AnisoFilter    : 3;    // GPUANISOFILTER   = 0
							// DWORD                : 3;                        = 0
							// DWORD BorderSize     : 1;    // DWORD            = 0

				0x00000000, // dword[4]
							// DWORD VolMagFilter   : 1;    // GPUMINMAGFILTER  = 0
							// DWORD VolMinFilter   : 1;    // GPUMINMAGFILTER  = 0
							// DWORD MinMipLevel    : 4;    // DWORD            = 0
							// DWORD MaxMipLevel    : 4;    // DWORD            = 0
							// DWORD MagAnisoWalk   : 1;    // BOOL             = 0
							// DWORD MinAnisoWalk   : 1;    // BOOL             = 0
							// INT   LODBias        : 10;   // int              = 0
							// INT   GradExpAdjustH : 5;    // int              = 0
							// INT   GradExpAdjustV : 5;    // int              = 0

				0x00000200, // dword[5]
							// DWORD BorderColor    : 2;    // GPUBORDERCOLOR   = 0
							// DWORD ForceBCWToMax  : 1;    // BOOL             = 0
							// DWORD TriClamp       : 2;    // GPUTRICLAMP      = 0
							// INT   AnisoBias      : 4;    // int              = 0
							// DWORD Dimension      : 2;    // GPUDIMENSION     = 1
							// DWORD PackedMips     : 1;    // BOOL             = 0
							// DWORD MipAddress     : 20;   // DWORD            = 0
			};
			CompileTimeAssert(sizeof(black1x1) == sizeof(IDirect3DTexture9));

//#			if __ASSERT
//				static bool checked/*=false*/;
//				if (!checked)
//				{
//					const UINT Width        = 1;
//					const UINT Height       = 1;
//					const UINT Levels       = 1;
//					const DWORD Usage       = 0;
//					const D3DFORMAT Format  = (D3DFORMAT)MAKED3DFMT(
//						GPUTEXTUREFORMAT_8,                                                 /*TextureFormat*/
//						GPUENDIAN_NONE,                                                     /*Endian*/
//						FALSE,                                                              /*Tiled*/
//						GPUSIGN_UNSIGNED,                                                   /*TextureSign*/
//						GPUNUMFORMAT_INTEGER,                                               /*NumFormat*/
//						(GPUSWIZZLE_0|GPUSWIZZLE_0<<3|GPUSWIZZLE_0<<6|GPUSWIZZLE_0<<9));    /*Swizzle*/
//					const D3DPOOL Pool      = D3DPOOL_DEFAULT;  // ignored
//					const UINT BaseOffset   = 0;
//					const UINT MipOffset    = 0;
//					const UINT Pitch        = 0;
//					UINT *const pBaseSize   = NULL;
//					UINT *const pMipSize    = NULL;
//					IDirect3DTexture9 checkTex;
//					XGSetTextureHeader(Width, Height, Levels, Usage, Format,
//						Pool, BaseOffset, MipOffset, Pitch, &checkTex, pBaseSize, pMipSize);
//					checked = true;
//					Assert(memcmp(&black1x1, &checked, sizeof(black1x1)) == 0);
//				}
//#			endif

			GRCDEVICE.GetCurrent()->SetTexture(sampler,(IDirect3DTexture9*)black1x1);
		}
#endif
		return;
	}
#if RSG_PC && !__RESOURCECOMPILER
	//Assert(static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert(data->IsValid());
#endif // __WIN32PC

#if !__RESOURCECOMPILER
#if RSG_PC
	// SetTextureFetchConstant is faster still but terrain doesn't work right.
	if (data != s_TextureShadow[sampler]) {
		GRCDEVICE.SetTexture /*FetchConstant*/(sampler,s_TextureShadow[sampler] = const_cast<grcTexture*>(data));
	}
#else

#if __DEV
	if( g_MaxTextureSize > 0 )
	{
		int maxSize = (int)pow(2.0f,(int)(g_MaxTextureSize)-1);
		D3DMIPTAIL_DESC desc;
		const_cast<grcTextureObject*>(data)->GetTailDesc(&desc);
		int texMaxSize = rage::Max(desc.Width,desc.Height);
		if( texMaxSize > maxSize )
		{
			int maxmip = 0;
			int size = texMaxSize;
			while( size > maxSize )
			{
				maxmip++;
				size /= 2;
			}
			
			Assign(ss.MaxMipLevel,maxmip);
		}
	}
#endif


	if (data != s_TextureShadow[sampler])
	#if HACK_GTA4
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetTexture(sampler,s_TextureShadow[sampler] = const_cast<grcTextureObject*>(data)));
	#else
		GRCDEVICE.GetCurrent()->SetTextureFetchConstant(sampler,s_TextureShadow[sampler] = const_cast<grcTextureObject*>(data));
	#endif

#if __ASSERT
	// Check for situation that will cause Xenon GPU abort, textures with different base addresses but same mip address.
	if (PARAM_checksampleroverlap.Get() && sampler < 16) {
		GPUTEXTURE_FETCH_CONSTANT *tf = GRCDEVICE.GetCurrent()->m_Constants.TextureFetch;
		for (u32 i=0; i<16; i++) {
			if (sampler != i && tf[i].BaseAddress == tf[sampler].BaseAddress && tf[i].MipAddress != tf[sampler].MipAddress)
				Assertf(false,"Caught potential GPU abort (different texture sampler indices %u and %u have same base addr (0x%x), different mip addr (0x%x and 0x%x), usually caused by bad RT pools)",sampler,i,tf[sampler].BaseAddress,tf[sampler].MipAddress,tf[i].MipAddress);
		}
	}
#endif

#endif

	s_SamplerStateShadow[sampler] = samplerStateSet;
#endif

#if __XENON
	// SetSamplerState(Sampler, D3DSAMP_ADDRESSU  , AddressU);
	// SetSamplerState(Sampler, D3DSAMP_ADDRESSV  , AddressV);
	// SetSamplerState(Sampler, D3DSAMP_ADDRESSW  , AddressW);
	GRCDEVICE.GetCurrent()->SetSamplerAddressStates(sampler,ss.AddressU,ss.AddressV,ss.AddressW);

	// SetSamplerState(Sampler, D3DSAMP_BORDERCOLOR , BorderColor);
	// SetSamplerState(Sampler, D3DSAMP_WHITEBORDERCOLORW , WhiteBorderColorW);
	// SetSamplerState(Sampler, D3DSAMP_POINTBORDERENABLE , PointBorderEnable);
	// Microsoft confirms the functions are safe to mix and match and recommends that
	// if only the border color is changing, might as well just use the original function.
	// GRCDEVICE.GetCurrent()->SetSamplerBorderStates(sampler,ss.border,FALSE,TRUE);
	GRCDEVICE.GetCurrent()->SetSamplerBorderStates(sampler,ss.BorderColor? ~0U : 0,ss.BorderColorW,TRUE);

	// SetSamplerState(Sampler, D3DSAMP_MINFILTER, MinFilter);
	// SetSamplerState(Sampler, D3DSAMP_MAGFILTER, MagFilter);
	// SetSamplerState(Sampler, D3DSAMP_MIPFILTER, MipFilter);
	// SetSamplerState(Sampler, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
	// SetSamplerState(Sampler, D3DSAMP_MAXANISOTRPY, MaxAnisotropy);
	GRCDEVICE.GetCurrent()->SetSamplerFilterStates(sampler,ss.MinFilter,ss.MagFilter,ss.MipFilter,ss.MaxAnisotropy);

	SetShadowedSamplerState(sampler,D3DSAMP_MIPMAPLODBIAS, grcessMIPMAPLODBIAS, ss.MipLodBias.u);
	SetShadowedSamplerState(sampler,D3DSAMP_MAXMIPLEVEL, grcessMAXMIPLEVEL, ss.MaxMipLevel);
	SetShadowedSamplerState(sampler,D3DSAMP_TRILINEARTHRESHOLD, grcessTRILINEARTHRESHOLD, ss.TrilinearThresh);
	SetShadowedSamplerState(sampler,D3DSAMP_MINMIPLEVEL, grcessMINMIPLEVEL, ss.MinMipLevel);
#elif !__RESOURCECOMPILER && __D3D9
	SetShadowedSamplerState(sampler, D3DSAMP_ADDRESSU,		grcessADDRESSU,			ss.AddressU,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_ADDRESSV,		grcessADDRESSV,			ss.AddressV,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_ADDRESSW,		grcessADDRESSW,			ss.AddressW,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_BORDERCOLOR,	grcessBORDERCOLOR,		ss.BorderColor? ~0U : 0,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_MAGFILTER,		grcessMAGFILTER,		ss.MagFilter,	version);
	SetShadowedSamplerState(sampler, D3DSAMP_MINFILTER,		grcessMINFILTER,		ss.MinFilter,	version);
	SetShadowedSamplerState(sampler, D3DSAMP_MIPFILTER,		grcessMIPFILTER,		ss.MipFilter,	version);
	SetShadowedSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS,	grcessMIPMAPLODBIAS,	ss.MipLodBias.u,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_MAXMIPLEVEL,	grcessMAXMIPLEVEL,		ss.MaxMipLevel,		version);
	SetShadowedSamplerState(sampler, D3DSAMP_MAXANISOTROPY,	grcessMAXANISOTROPY,	ss.MaxAnisotropy,	version);

	//grcTexture* pTexture = GRCDEVICE.GetTexture(data);
	//Assert(pTexture != NULL);
	u32 srgb =  /*pTexture*/data->IsSRGB();
	SetShadowedSamplerState(sampler, D3DSAMP_SRGBTEXTURE,	grcessTEXTUREZFUNC,		srgb,			version);
#endif
}
#endif	// !__D3D11

#if RSG_PC && !__D3D11

grcCBuffer::grcCBuffer() : Size(0), NameHash(0), Data(NULL)
{
	memset(Registers,0,sizeof(Registers));
#if __D3D11
#if TRACK_CONSTANT_BUFFER_CHANGES
	m_LockCount = 0;
#endif
#else
	m_bLocked = false;
	m_pvLockPtr = NULL;
	m_pbySysMemBuf = NULL;
#endif
}


int grcCBuffer::GetDataIndex() const
{
	int index = 0;
#if __D3D11
	if (m_bThreadSafe)
		index = GRCDEVICE.GetThreadId()+1;	
#endif

	return index;
}

void grcCBuffer::operator=(const grcCBuffer &rhs)
{
	Destroy();

	Size = rhs.Size;
	memcpy(Registers,rhs.Registers,sizeof(Registers));
	NameHash = rhs.NameHash;
	Name = rhs.Name;
	Data = rhs.Data;
#if __D3D9
	Dirty = rhs.Dirty;
	Assert(!rhs.m_bLocked);
	m_bLocked = rhs.m_bLocked;
	m_pvLockPtr = rhs.m_pvLockPtr;
	m_pbySysMemBuf = rage_new char[Size]; 
	Assert(m_pbySysMemBuf != NULL);
	if (rhs.m_pbySysMemBuf != NULL)
		memcpy(m_pbySysMemBuf, rhs.m_pbySysMemBuf, Size);
	else
		memset(m_pbySysMemBuf, 0, Size);
#endif
#if !__RESOURCECOMPILER
#if __D3D11
	int index = GetDataIndex();

	if (Data && (Data[index].Buffer != NULL))
	{
		Data[index].Buffer->AddRef();
		Data[index].pbySysMemBuf = rage_new char[Size]; 
		Assert(Data[index].pbySysMemBuf != NULL);
		if (rhs.Data[index].pbySysMemBuf != NULL)
			memcpy(Data[index].pbySysMemBuf, rhs.Data[index].pbySysMemBuf, Size);
		else
			memset(Data[index].pbySysMemBuf, 0, Size);
	}
#else
	if (Data != NULL)
	{
		Data->AddRef();
	}
#endif
#endif // !__RESOURCECOMPILER
}

grcCBuffer::~grcCBuffer()
{
	Destroy();
}

void grcCBuffer::SetDirty(u8 uFlag)
{
#if __D3D11
	Data[GetDataIndex()].Dirty |= uFlag;
#else
	Dirty |= uFlag;
#endif
}

u8 grcCBuffer::GetDirty()
{
#if __D3D11
	return Data[GetDataIndex()].Dirty;
#else
	return Dirty;
#endif
}

void grcCBuffer::ResetDirty()
{
#if __D3D11
	Data[GetDataIndex()].Dirty = 0;
#else
	Dirty = 0;
#endif
}

bool grcCBuffer::IsLocked(u32 UNUSED_PARAM(threadIdx)) const
{
#if __D3D11
	return Data[GetDataIndex()].bLocked;
#else
	return m_bLocked;
#endif
}

grcBuffer *grcCBuffer::GetBuffer(DEV_ONLY(u8 lockflag))
{
#if __D3D11
	// lockflag = 0 : locking buffer
	// lockflag = 1 : unlocking buffer ... check validity
	// lockflag = 2 : no lock/unlock 
	int index = GetDataIndex();
#if __DEV
	if (lockflag == 0)
		Data[index].threadId = GRCDEVICE.GetThreadId();
	else if (lockflag == 1)
	{
		Assert(Data[index].threadId == GRCDEVICE.GetThreadId());
	}
#endif
	return Data[index].Buffer;
#else
#if __DEV
	(void)lockflag;
#endif
	return Data;
#endif
}

void grcCBuffer::Init(bool)
{
	//bMakeThreadSafe = true;

	// Assume that grcCBuffer::Load succeeded
	Assert((Size > 0) && ((Size % 16) == 0));

#if __D3D11
	int numThreads = 0;
	//if (bMakeThreadSafe)
	//	numThreads = GRCDEVICE.GetNumberOfWorkerThreads();

	Data = rage_new grcBufferInfo[numThreads+1];

	for (int i = 0; i < (numThreads+1); i++)
	{
#if __DEV
		Data[i].threadId = -2;
#endif
		Data[i].Dirty = 0;
		Data[i].bLocked = false;
		Data[i].pvLockPtr = NULL;
		Data[i].pbySysMemBuf = NULL;
		GRCDEVICE.CreateShaderConstantBuffer(Size,&(Data[i].Buffer) NOTFINAL_ONLY(, Name));
		Data[i].pbySysMemBuf = rage_new char[Size];
		Assert(Data[i].pbySysMemBuf != NULL);
	}

	m_bThreadSafe = false;
#else
	Dirty = 0;
	GRCDEVICE.CreateShaderConstantBuffer(Size,&Data NOTFINAL_ONLY(, Name));
	m_pbySysMemBuf = rage_new char[Size];
#endif
}

void grcCBuffer::Destroy()
{
#if __D3D11
	int numThreads = 0;
	if (m_bThreadSafe)
		numThreads = 0; // GRCDEVICE.GetNumberOfWorkerThreads();

	if (Data != NULL)
	{
		for (int i = 0; i < (numThreads+1); i++)
		{
			if (Data[i].bLocked)
			{
				ID3D11Buffer *pBuf = (ID3D11Buffer*)(Data[i].Buffer);
				g_grcCurrentContext->Unmap(pBuf, 0);
				Data[i].bLocked = false;
				if (Data[i].pbySysMemBuf != NULL)
				{
					delete[] Data[i].pbySysMemBuf;
					Data[i].pbySysMemBuf = NULL;
				}
#if __DEV
				Data[i].threadId = -2;
#endif
			}
		}
	}
#else
	if (m_bLocked)
	{
		Warningf("CBuffer %s is locked on deallocation - Unlocking", Name);
		Unlock();
	}
#endif


	if (Data != NULL)
	{
#if !__RESOURCECOMPILER
#if __D3D11
		for (int i = 0; i < (numThreads+1); i++)
		{
			u32 uCount = Data[i].Buffer->AddRef();
			if (uCount > 2)
			{
				//Warningf("grcCBuffer has %d reference counts on deletion - Potential leak unless grcCBuffer was copied", uCount);
			}
			Data[i].Buffer->Release();
			if (uCount <= 2)
			{
				SAFE_RELEASE_RESOURCE(Data[i].Buffer);
			}
			else
			{
				Data[i].Buffer->Release();
			}

			if (Data[i].pbySysMemBuf != NULL)
				delete [] Data[i].pbySysMemBuf;
			Data[i].pbySysMemBuf = NULL;
		}

		delete [] Data;
#else
	u32 uCount = Data->AddRef();
	if (uCount > 2)
		//Warningf("grcCBuffer has %d reference counts on deletion - Potential leak unless grcCBuffer was copied", uCount);
	Data->Release();
	if (uCount <= 2)
	{
		SAFE_RELEASE_RESOURCE(Data);
		if (m_pbySysMemBuf != NULL)
			delete [] m_pbySysMemBuf;
		m_pbySysMemBuf = NULL;
	}
	else
		Data->Release();
#endif
#endif // !__RESOURCECOMPILER
	}
}

void grcCBuffer::Load(fiStream &S)
{
	//S.ReadInt(&Count, 1);
	S.ReadInt(&Size,1);
	S.ReadShort(Registers,6);

	char buffer[256];
	int count = S.GetCh();
	S.Read(buffer,count);
	Name = buffer;

	// Displayf("CBuffer::Load - %s, register %d, size %d", buffer, Register, Size);
	NameHash = atStringHash(Name);
}

void* grcCBuffer::GetDataPtr()
{
#if __D3D11
	int index = GetDataIndex();
	Data[index].bLocked = true;
	Data[index].Dirty = 0x1;
	return Data[index].pbySysMemBuf;

	/*
	if (IsLocked())
	{
		return Data[index].pvLockPtr;
	}
	else
	{
		AssertVerify(Lock());
		return Data[index].pvLockPtr;
	}
	*/
#else
	m_bLocked = true;
	return m_pbySysMemBuf;
	/*
	if (IsLocked())
	{
		return m_pvLockPtr;;
	}
	else
	{
		AssertVerify(Lock());
		return m_pvLockPtr;
	}
	*/
#endif
}


bool grcCBuffer::Lock()
{
#if !__RESOURCECOMPILER && __D3D11
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	#if __D3D11
	D3D11_MAPPED_SUBRESOURCE oMappedRes;
	int index = GetDataIndex();

	ID3D11Buffer *pBuf = (ID3D11Buffer*)(GetBuffer(DEV_ONLY(0)));
	ASSERT_ONLY(HRESULT hRes);
	Assert(pBuf != NULL);

	{
		DEVICE_EKG_COUNTANDTIME(MapCBuffer);
		ASSERT_ONLY(hRes =)g_grcCurrentContext->Map(pBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, (D3D11_MAPPED_SUBRESOURCE*)&oMappedRes);
		Assert(hRes == S_OK);
	}	
	

#if TRACK_CONSTANT_BUFFER_CHANGES
	if (grcEffect::sm_TrackConstantBufferUsage)
		m_LockCount++;
#endif
	
	Data[index].pvLockPtr = oMappedRes.pData;
	#else
	ID3D10Buffer *pBuf = (ID3D10Buffer*)Data;
	ASSERT_ONLY(HRESULT hRes =) pBuf->Map(D3D10_MAP_WRITE_DISCARD,NULL,(void**)&m_pvLockPtr);
	#endif

#if STALL_DETECTION
	if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
	{
		grcWarningf("CBuffer Lock took %f milliseconds", oTime.GetMsTime());
	}
#endif // STALL_DETECTION

	Assertf(SUCCEEDED(hRes), "Failed to Lock CBuffer %x", hRes);

	#if __D3D11
	Data[index].bLocked = (Data[index].pvLockPtr) ? true : false;
	AssertVerify(Data[index].pvLockPtr != NULL);
	return Data[index].bLocked;
	#else
	m_bLocked = (m_pvLockPtr) ? true : false;
	AssertVerify(m_pvLockPtr != NULL);
	return m_bLocked;
	#endif
#else
	return false;
#endif // !__RESOURCECOMPILER && __D3D11
}

bool grcCBuffer::Unlock()
{
#if !__RESOURCECOMPILER && __D3D11
	if (!Lock())
		return false;

#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION
	if (IsLocked())
	{
	#if __D3D11
		int index = GetDataIndex();

		Assert(Data[index].bLocked == true);
		Assert(Data[index].pvLockPtr != NULL);
		Assert(Data[index].pbySysMemBuf != NULL);
		memcpy(Data[index].pvLockPtr,	Data[index].pbySysMemBuf, GetSize());
		/* float *foo = (float*)Data[index].pvLockPtr;
		for (int i=0; i<16; i++,foo+=4)
			Displayf("%f %f %f %f",foo[0],foo[1],foo[2],foo[3]); */

		ID3D11Buffer *pBuf = (ID3D11Buffer*)(GetBuffer(DEV_ONLY(1)));
		{
			DEVICE_EKG_COUNTANDTIME(UnMapCBuffer);
			g_grcCurrentContext->Unmap(pBuf, 0);
		}

		Data[index].Dirty = 0x0;
		Data[index].bLocked = false;
#if __DEV
		Data[index].threadId = -2;
#endif
	#else
		memcpy(m_pvLockPtr,	m_pbySysMemBuf, GetSize());
		ID3D10Buffer *pBuf = (ID3D10Buffer*)Data;
		pBuf->Unmap();
		m_bLocked = false;
	#endif

#if STALL_DETECTION
		if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
		{
			grcWarningf("CBuffer Unlock took %f milliseconds", oTime.GetMsTime());
		}
#endif // STALL_DETECTION
		return true;
	}
#endif // !__RESOURCECOMPILER && __D3D11
	return false;
}

#endif // RSG_PC && !__D3D11

#if RSG_PC && !__D3D11

void grcVertexProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	SetParameterCommon(address + D3DVERTEXTEXTURESAMPLER0,data,stateHandle);
}

void grcFragmentProgram::SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetPixelShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetPixelShaderConstantB(address,value);
}

bool grcFragmentProgram::SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type)
{
#if __D3D9
	(void)type;
	(void)pEffectVar;
	(void)offset;
	GRCDEVICE.SetPixelShaderConstantF(address,data,count);
	return true;
#else
	AssertMsg(NULL, "grcFragmentProgram::SetLocalParameter()...Unsupported\n");
	return false;
#endif
}

void grcFragmentProgram::SetFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetPixelShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetPixelShaderConstantB(address,value);
	}
}

void grcFragmentProgram::SetParameter(int address, const float *data,int count,u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetPixelShaderConstantF(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetPixelShaderConstantF(address,data,count);
	}
}

void grcFragmentProgram::SetParameterW(int address, const float *data,int count,u32 offset, grcCBuffer *pEffectVar TRACK_CONSTANT_BUFFER_CHANGES_ONLY(, grcParameter *global))
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetPixelShaderConstantFW(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
	{
		GRCDEVICE.SetPixelShaderConstantFW(address,data,count);
	}
}


void grcFragmentProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	SetParameterCommon(address,data,stateHandle);
}

void grcComputeProgram::SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar) const
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetComputeShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetComputeShaderConstantB(address,value);
}

bool grcComputeProgram::SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const
{
	(void)address; 
	(void)data;
	(void)count;
	(void)offset;
	(void)pEffectVar;
	(void)type;
	AssertMsg(NULL, "grcComputeProgram::SetLocalParameter()...Unsupported\n");
	return false;
}

void grcComputeProgram::SetFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetComputeShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetComputeShaderConstantB(address,value);
}

void grcComputeProgram::SetParameter(int address, float *data,int count,u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetComputeShaderConstantF(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetComputeShaderConstantF(address,data,count);
}

void grcComputeProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle, bool bSetSamplerOnly)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	(void)bSetSamplerOnly;
	SetParameterCommon(address,data,stateHandle);
}

void grcDomainProgram::SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar) const
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetDomainShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetDomainShaderConstantB(address,value);
}

bool grcDomainProgram::SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const
{
	(void)address; 
	(void)data;
	(void)count;
	(void)offset;
	(void)pEffectVar;
	(void)type;
	AssertMsg(NULL, "grcComputeProgram::SetLocalParameter()...Unsupported\n");
	return false;
}

void grcDomainProgram::SetFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetDomainShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetDomainShaderConstantB(address,value);
}

void grcDomainProgram::SetParameter(int address, float *data,int count,u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetDomainShaderConstantF(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetDomainShaderConstantF(address,data,count);
}

void grcDomainProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	SetParameterCommon(address,data,stateHandle);
}

void grcGeometryProgram::SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar) const
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetGeometryShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetGeometryShaderConstantB(address,value);
}

bool grcGeometryProgram::SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const
{
	(void)address; 
	(void)data;
	(void)count;
	(void)offset;
	(void)pEffectVar;
	(void)type;
	AssertMsg(NULL, "grcGeometryProgram::SetLocalParameter()...Unsupported\n");
	return false;
}

void grcGeometryProgram::SetFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetGeometryShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetGeometryShaderConstantB(address,value);
}

void grcGeometryProgram::SetParameter(int address, float *data,int count,u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetGeometryShaderConstantF(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetGeometryShaderConstantF(address,data,count);
}

void grcGeometryProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	SetParameterCommon(address,data,stateHandle);
}

void grcHullProgram::SetLocalFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar) const
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetHullShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetHullShaderConstantB(address,value);
}

bool grcHullProgram::SetLocalParameter(int address,float *data,int count,u32 offset, grcCBuffer *pEffectVar, u8 type) const
{
	(void)address; 
	(void)data;
	(void)count;
	(void)offset;
	(void)pEffectVar;
	(void)type;
	AssertMsg(NULL, "grcHullProgram::::SetLocalParameter()...Unsupported\n");
	return false;
}

void grcHullProgram::SetFlag(int address,bool value,u32 offset,grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetHullShaderConstantB(address,value,offset,pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetHullShaderConstantB(address,value);
}

void grcHullProgram::SetParameter(int address, float *data,int count,u32 offset, grcCBuffer *pEffectVar)
{
	if (pEffectVar != NULL)
	{
		pEffectVar->SetDirty(0x10);
		GRCDEVICE.SetHullShaderConstantF(address,data,count,offset, pEffectVar->GetDataPtr());
	}
	else
		GRCDEVICE.SetHullShaderConstantF(address,data,count);
}

void grcHullProgram::SetTexture(int address,const /*grcTextureObject*/grcTexture *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	//Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
	Assert((data == NULL) || (data->IsValid()));
#endif // __WIN32PC
	SetParameterCommon(address,data,stateHandle);
}

#elif __XENON

void grcVertexProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
#endif // __WIN32PC
	SetParameterCommon(address + D3DVERTEXTEXTURESAMPLER0,data,stateHandle);
}

void grcFragmentProgram::SetFlag(int address,bool value)
{
	BOOL b = value;
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetPixelShaderConstantB(address,&b,1));
}

void grcFragmentProgram::SetParameter(int address, const float *data,int count)
{
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetPixelShaderConstantF(address,data,count));
}

void grcFragmentProgram::SetTexture(int address,const grcTextureObject *data,u16 stateHandle)
{
#if __WIN32PC && !__RESOURCECOMPILER
	Assert((data == NULL) || static_cast<grcTexture*>(GRCDEVICE.GetTexture(data))->IsValid());
#endif // __WIN32PC
	SetParameterCommon(address,data,stateHandle);
}

#endif // __XENON

#endif // __WIN32

#if __WIN32
using namespace grcRSV;
# if __XENON
extern D3DFORMAT g_grcDepthFormat;
# endif
#endif	// __WIN32


#if __XENON && 0
extern char g_CurrentStreamable[];

static const char *DecodeType(DWORD type) {
	static char buf[32];
	switch (type) {
		case D3DDECLTYPE_FLOAT1: return "float1";
		case D3DDECLTYPE_FLOAT2: return "float2";
		case D3DDECLTYPE_FLOAT3: return "float3";
		case D3DDECLTYPE_FLOAT4: return "float4";
		case D3DDECLTYPE_D3DCOLOR: return "d3dcolor";
		case D3DDECLTYPE_UBYTE4: return "ubyte4";
		case D3DDECLTYPE_DEC4N: return "dec4n";
		case D3DDECLTYPE_DEC3N: return "dec3n";
		case D3DDECLTYPE_FLOAT16_2: return "float16_2";
		case D3DDECLTYPE_FLOAT16_4: return "float16_4";
		default:
			sprintf(buf,"unknown(%x)",type);
			return buf;
	}
}

static void DumpVertexDeclarator(const char *tag,D3DVertexDeclaration *decl,int stride) {
	grcDisplayf("**** %s [stride %d]",tag,stride);
	D3DVERTEXELEMENT9 elem[32];
	UINT count = 32;
	decl->GetDeclaration(elem,&count);
	static const char *usages[] = { 
		"position", "blendweight", "blendindices", "normal",
		"psize", "texcoord", "tangent", "binormal",
		"tessfactor", "invalid", "color", "fog", 
		"depth", "sample" 
	};

	for (UINT i=0; i<count-1; i++)	// _END is included in the count.
		grcDisplayf("%3d. stream=%d offset=%d type=%s usage=%s[%d]",i,
		elem[i].Stream,elem[i].Offset,DecodeType(elem[i].Type),usages[elem[i].Usage],elem[i].UsageIndex);
}

void grcEffect::SetDeclaration(grcVertexDeclaration *decl,int stride,bool isSkinned) {
	for (int t=0; t<m_Techniques.GetCount(); t++) {
		Technique &te = m_Techniques[t];
		bool tSkinned = strstr(te.Name,"skinned") != NULL || strstr(te.Name,"Skinned") != NULL || strstr(te.Name,"SKINNED") != NULL;
		// don't process techniques that are not even close
		if ((isSkinned && !tSkinned) || (!isSkinned && tSkinned))
			continue;
		for (int p=0; p<te.Passes.GetCount(); p++) {
			grcVertexProgram &vse = m_VertexPrograms[te.Passes[p].VertexProgramIndex];

			DWORD hashCode = ((DWORD)(decl)<<4) ^ (DWORD) stride;
			VertexShaderSecretInfo*& secretInfo = reinterpret_cast<VertexShaderSecretInfo*&>(vse.GetProgram()->Identifier);
			if (!secretInfo) {
				// This now gets called in grcDevice::Draw*
				//vse.GetProgram()->Bind(0,decl->D3dDecl,(DWORD*)&stride,NULL);
				secretInfo = rage_new VertexShaderSecretInfo;
				secretInfo->HashCode = hashCode;
				secretInfo->Decl = decl->D3dDecl;
				secretInfo->Stride = stride;
				secretInfo->Streamable = StringDuplicate(g_CurrentStreamable);
				decl->AddRef();	// THIS WILL NEVER GET FREED NOW!
			}
			else if (secretInfo->HashCode != hashCode) {
				grcErrorf("Effect '%s' already bound to an incompatible declarator (or stride)!", m_EffectPath.c_str());
				DumpVertexDeclarator(g_CurrentStreamable,decl->D3dDecl,stride);
				DumpVertexDeclarator(secretInfo->Streamable,secretInfo->Decl,secretInfo->Stride);
				grcErrorf("All objects using a particular technique must have the exact same vertex format.");
				grcErrorf("Cloth and inconsistent texture packing settings are the usual culprits so far.");
			}
		}
	}
}
#endif

#if !__D3D11
void grcEffect::InitClass()
{
}
#endif // !__D3D11

void grcEffect::ShutdownClass()
{
	UnloadAll();
}

void grcEffect::ApplyDefaultSamplerStates()
{
	// Init to invalid state
	memset(s_SamplerShadow, 0xCD, sizeof(s_SamplerShadow));
}

#if !__D3D11
void grcEffect::BeginFrame() {
	grcEffect::ApplyDefaultRenderStates();
	grcEffect::ApplyDefaultSamplerStates();
}

void grcEffect::EndFrame() 
{
}
#endif // !__D3D11

#if 0

#define ASSIGN(x,y) do { x=y; Assertf((x)==(y),"%d!=%d",(x),(y)); } while (0)

void SamplerState::Set(u32 state,u32 value)
{
	switch (state) {
	case grcessADDRESSU: 
		ASSIGN(wraps,value); 
		break;
	case grcessADDRESSV: 
		ASSIGN(wrapt,value); 
		break;
	case grcessADDRESSW: 
		ASSIGN(wrapr,value); 
		break;
	case grcessBORDERCOLOR: 
		border = value; 
		break;
	case grcessMAGFILTER:
		ASSIGN(magFilter,value);
		break;
	case grcessMINFILTER:
		ASSIGN(minFilter,value);
		break;
	case grcessMIPFILTER:
		ASSIGN(mipFilter,value);
		break;
	case grcessMIPMAPLODBIAS: 
		mipBias = value;
		break;
	case grcessMAXMIPLEVEL: 
		ASSIGN(maxlod,value);
		break;
	case grcessMAXANISOTROPY:
		ASSIGN(maxAniso,value-1);
		break;
	case grcessTRILINEARTHRESHOLD: 
#if __XENON
		ASSIGN(triThresh,value); 
#endif
		break;
	case grcessMINMIPLEVEL: 
		ASSIGN(minlod,value);	// nomenclature is backwards from D3D
		break;
	case grcessTEXTUREZFUNC:
		break;
	}
}

#endif


} // namespace rage

#endif	// __WIN32
