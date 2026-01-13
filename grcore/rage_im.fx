#ifndef __RAGE_IM_FX
#define __RAGE_IM_FX

#ifndef PRAGMA_DCL
	#pragma dcl position diffuse texcoord0 normal
	#define PRAGMA_DCL
#endif


// Configure the megashder 

#ifndef RAGE_DEPENDENCY_MODE
	#define DISABLE_RAGE_LIGHTING
	#define NO_SKINNING
	const static float ClearColor = 0;
	const static float GeneralParams0 = 1;
	const static float GeneralParams1 = 1;
	#define TECH_NAME(name)	name
#else
	#define TECH_NAME(name) rage_##name
#endif

#include "../shaderlib/rage_common.fxh"
#include "../shaderlib/rage_diffuse_sampler.fxh"
#include "../shaderlib/rage_xplatformtexturefetchmacros.fxh"

BeginSampler(sampler2D,TransparentDstMap,TransparentDstMapSampler,TransparentDstMap)
ContinueSampler(sampler2D,TransparentDstMap,TransparentDstMapSampler,TransparentDstMap)
	MIN_POINT_MAG_POINT_MIP_NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;

BeginDX10Sampler(sampler, Texture2D<float4>, TransparentSrcMap,TransparentSrcMapSampler,TransparentSrcMap)
ContinueSampler(sampler2D,TransparentSrcMap,TransparentSrcMapSampler,TransparentSrcMap)
	MIN_POINT_MAG_POINT_MIP_NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;

BeginDX10Sampler(sampler, Texture2D<float>, DepthPointMap, DepthMapPointSampler, DepthPointMap)
ContinueSampler(sampler2D,DepthPointMap,DepthMapPointSampler,DepthPointMap)
	MIN_POINT_MAG_POINT_MIP_NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;


// Point filter
BeginSampler(sampler2D,RenderPointMap,RenderMapPointSampler,RenderPointMap)
ContinueSampler(sampler2D,RenderPointMap,RenderMapPointSampler,RenderPointMap)
   MinFilter = POINT;
   MagFilter = POINT;
   MipFilter = POINT;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;

#if __PS3 || __PSSL
// Quincunx filter for PS3
BeginSampler(sampler2D,RenderQuincunxMap,RenderMapQuincunxSampler,RenderQuincunxMap)
ContinueSampler(sampler2D,RenderQuincunxMap,RenderMapQuincunxSampler,RenderQuincunxMap)
   MinFilter = QUINCUNX;
   MagFilter = QUINCUNX;
   MipFilter = NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;

// Quincunx Alt. filter for PS3
BeginSampler(sampler2D,RenderQuincunxAltMap,RenderMapQuincunxAltSampler,RenderQuincunxAltMap)
ContinueSampler(sampler2D,RenderQuincunxAltMap,RenderMapQuincunxAltSampler,RenderQuincunxAltMap)
   MinFilter = QUINCUNX_ALT;
   MagFilter = QUINCUNX_ALT;
   MipFilter = NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;

// gauss filter for the PS3
// those sampler states are only available on the PS3
BeginSampler(sampler2D,RenderGaussMap,RenderMapGaussSampler,RenderGaussMap)
ContinueSampler(sampler2D,RenderGaussMap,RenderMapGaussSampler,RenderGaussMap)
   MinFilter = GAUSSIAN;
   MagFilter = GAUSSIAN;
   MipFilter = NONE;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
EndSampler;
#endif // __PS3

struct rageVertexOutput 
{
    DECLARE_POSITION( pos )
    float2 texCoord					: TEXCOORD0;
    float4 color0					: COLOR0;
};

float3 NormalBlend(float4 src, float3 dest)
{
	float alpha = min(1.0f, src.a);
	return dest * (1.0f - alpha) + alpha * src.rgb;
}

struct rageVertexInputUnlit {
	// This covers the default rage vertex format (non-skinned)
    float3 pos			: POSITION;
	float4 diffuse		: COLOR0;
    float2 texCoord0	: TEXCOORD0;
};

struct rageVertexInputLit {
	// This covers the default rage vertex format (non-skinned)
    float3 pos			: POSITION;
    float3 normal		: NORMAL;
	float4 diffuse		: COLOR0;
    float2 texCoord0	: TEXCOORD0;
};

#if __PSSL
rageVertexOutput VS_Rage_TransformUnlit(rageVertexInputUnlit IN)
#else
// Declaring that we expect a lit vertex when we don't even use it is kinda dumb but it
// looks like a bunch of DX11 code assumes this now.
rageVertexOutput VS_Rage_TransformUnlit(rageVertexInputLit IN)
#endif
{
    rageVertexOutput OUT;
    
    // Write out final position & texture coords
    OUT.pos =  mul(float4(IN.pos,1), gWorldViewProj);
    OUT.texCoord = IN.texCoord0.xy;
    OUT.color0 = IN.diffuse;
    return OUT;
}

rageVertexOutput VS_Rage_TransformLit(rageVertexInputLit IN)
{
    rageVertexOutput OUT;
    
    float3 inNrm = normalize(IN.normal);

    // Write out final position & texture coords
    OUT.pos =  mul(float4(IN.pos,1), gWorldViewProj);
    OUT.texCoord = IN.texCoord0.xy;

    float3 normal = normalize(mul(inNrm, (float3x3)gWorld));
    float3 cam = (float3) gViewInverse[2];
    float atten = max(dot(normal,cam), 0.25);

    OUT.color0 = IN.diffuse * float4(atten,atten,atten,1.0);

    return OUT;
}

float4 PS_Rage_Textured( rageVertexOutput IN): COLOR
{
	// Texture Lookup
	float4 texColor = tex2D(DiffuseSampler, IN.texCoord);
	
	// Composite diffuse, texture, and lighting
	float4 litDiffuseTex = IN.color0 * texColor;
	
	return litDiffuseTex;
}

//We need to ensure the input matches the vertex declaration correctly so we
//need a different input structure. This breaks DX11 if its wrong, other platforms
//manage to fix it up itself.
struct rageVertexBlitIn {
	float3 pos			: POSITION;
	float3 normal		: NORMAL;
	float4 diffuse		: COLOR0;
	float2 texCoord0	: TEXCOORD0;
};

struct rageVertexBlit {
	// This is used for the simple vertex and pixel shader routines
	DECLARE_POSITION(  pos)
	float4 diffuse		: COLOR0;
	float2 texCoord0	: TEXCOORD0;
};

struct rageVertexMsaaIn {
	float3 pos			: POSITION;
	float2 texCoord0	: TEXCOORD0;
};

struct rageVertexMsaa {
	DECLARE_POSITION(  pos)
	float2 texCoord0	: TEXCOORD0;
};

struct rageVertexClearIn {
	float3 pos			: POSITION;
};

struct rageVertexClear {
	// This is used for the simple vertex and pixel shader routines
	DECLARE_POSITION( pos )
};

#if RSG_ORBIS
struct rageVertexClearGS {
	// This is used for the simple vertex and pixel shader routines
	float4 pos : PASSPOS;
};

struct rageVertexClearGSOUT {
	DECLARE_POSITION(pos)
	uint rt_index : SV_RenderTargetArrayIndex;
};
#endif //RSG_ORBIS


rageVertexBlit VS_Blit(rageVertexBlitIn IN)
{
	rageVertexBlit OUT;
	OUT.pos = float4( IN.pos.xyz, 1.0f);
	OUT.diffuse	= IN.diffuse;
	OUT.texCoord0 = IN.texCoord0 ;
	return(OUT);
}

rageVertexClear VS_Clear(rageVertexClearIn IN) {
	rageVertexClear OUT;
	OUT.pos = float4(IN.pos.xyz, 1.0f);
	return(OUT);
}

#if RSG_ORBIS
rageVertexClearGS VS_ClearGS(rageVertexClear IN) {
	rageVertexClearGS OUT = IN;
	return(OUT);
}

static int s_arraysize = 6;	// make this constant buffer
[maxvertexcount(18)]
void GS_Clear(triangle rageVertexClearGS input[3], inout TriangleStream<rageVertexClearGSOUT> OutputStream)
{
	rageVertexClearGSOUT output;
	for (int arrayindex = 0; arrayindex < s_arraysize; arrayindex++)
	{
		for(int i = 0; i < 3; i++)
		{
			output.pos = input[i].pos;
			output.rt_index = arrayindex;
			OutputStream.Append(output);
		}
		OutputStream.RestartStrip();
	}
}
#endif


half4 PS_Blit(rageVertexBlit IN): COLOR {
	float4 texel = tex2Dlod(DiffuseSampler, float4(IN.texCoord0, 0.0, GeneralParams1.x));
	return half4(texel * IN.diffuse * GeneralParams0);
}

half4 PS_BlitDepth(rageVertexBlit IN, out float depth : DEPTH): COLOR
{
#if __PS3
	depth = texDepth2D(DepthMapPointSampler, IN.texCoord0);
#else
	depth = f1tex2D(DepthMapPointSampler, IN.texCoord0);
#endif
	return 0;	//whatever
}

float4 PS_BlitTransparent(rageVertexBlit IN) : COLOR
{
	float4 src = h4tex2D(TransparentSrcMapSampler, IN.texCoord0);

#if RAGE_ENCODEOPAQUECOLOR
	float3 dest = rageDecodeOpaqueColor(h4tex2D(TransparentDstMapSampler, IN.texCoord0));
	return rageEncodeOpaqueColor(NormalBlend(src, dest));
#else
	return src;
#endif // RAGE_ENCODEOPAQUECOLOR
}

half4 PS_CopyTransparentEdgeBlur(rageVertexBlit IN) : COLOR
{
#if __XENON
	half4 src = h4tex2D(TransparentSrcMapSampler, IN.texCoord0);
	half4 dest = h4tex2D(DiffuseSampler, IN.texCoord0);
	return src + (1.0f - src.a) * dest;
#else
	return (half4)tex2D(TransparentSrcMapSampler, IN.texCoord0);
#endif // __XENON
}

half4 PS_BlitTransparentEdgeBlur(rageVertexBlit IN): COLOR
{
#ifndef RAGE_DEPENDENCY_MODE
	return (half4)tex2D(TransparentSrcMapSampler, IN.texCoord0);
#elif 1
	return (half4)PackHdr(UnpackColor(h4tex2D(TransparentSrcMapSampler, IN.texCoord0.xy)));
#elif 0
	return rageTex2DBicubic(TransparentSrcMapSampler, IN.texCoord0, TexelSize));
#elif 0
	half4 samples[] =
	{
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(-1, -1)*texelSize.xy),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(1, -1)*texelSize.xy),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(-1, 1)*texelSize.xy),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(1, 1)*texelSize.xy),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(1, 0)*texelSize.xy),
		h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + half2(0.0f, texelSize.y)),
	};

	half4 sampleAlpha = half4(samples[0].a, samples[1].a, samples[2].a, samples[3].a);
	half4 hasAlpha = sampleAlpha > 0.0f;
	half edgeValue = dot(hasAlpha, 1.0f.xxxx);
	half2 f = frac(IN.texCoord0 * texelSize.zw); // fractional position within texel
	if (edgeValue != 4.0f && edgeValue != 0.0f)
	{
		// We have found an edge... blur more

		// filter in x
		half4 w = rageComputeCubicWeights(f.x);
		half4 t0 = rageCubicFilter(w, samples[0],
							  		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(0, -1)*texelSize.xy),
							  		samples[1],
							  		h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(2, -1)*texelSize.xy) );
		half4 t1 = rageCubicFilter(w, h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(-1, 0)*texelSize.xy),
									samples[4],
									samples[5],
									h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(2, 0)*texelSize.xy) );
		half4 t2 = rageCubicFilter(w, samples[2],
									samples[6],
									samples[3],
									h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(2, 1)*texelSize.xy) );
		half4 t3 = rageCubicFilter(w, h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(-1, 2)*texelSize.xy),
									h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(0, 2)*texelSize.xy),
									h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(1, 2)*texelSize.xy),
									h4tex2D(TransparentSrcMapSampler, IN.texCoord0+half2(2, 2)*texelSize.xy) );
		// filter in y
		w = rageComputeCubicWeights(f.y);
		return rageCubicFilter(w, t0, t1, t2, t3);
	}
	else
	{
		half4 tA = lerp(samples[4], samples[5], f.x );
		half4 tB = lerp(samples[6], samples[3], f.x);
		return lerp(tA, tB, f.y);

	}
#else
	half2 halfTexelSize = texelSize.xy * 0.5f;
	half4x4 sampleC;
	sampleC[0] = h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + halfTexelSize);
	sampleC[1] = h4tex2D(TransparentSrcMapSampler, IN.texCoord0 - halfTexelSize);
	sampleC[2] = h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + half2(halfTexelSize.x, -halfTexelSize.y));
	sampleC[3] = h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + half2(-halfTexelSize.x, halfTexelSize.y));
	half4 accumSample = sampleC[0] + sampleC[1] + sampleC[2] + sampleC[3];
	half4 sampleAlpha = half4(sampleC[0].a, sampleC[1].a, sampleC[2].a, sampleC[3].a);
	half4 hasAlpha = sampleAlpha > 0.0f;
	half edgeValue = dot(hasAlpha, 1.0f.xxxx);
	if (edgeValue != 4.0f && edgeValue != 0.0f)
	{
		accumSample += h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + texelSize.xy);
		accumSample += h4tex2D(TransparentSrcMapSampler, IN.texCoord0 - texelSize.xy);
		accumSample += h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + half2(texelSize.x, -texelSize.y));
		accumSample += h4tex2D(TransparentSrcMapSampler, IN.texCoord0 + half2(-texelSize.x, texelSize.y));

		// We have found an edge... blur more
		return accumSample * 0.125f;
	}
	else
	{
		return accumSample * 0.25f;
	}
#endif
}

// -------------------------------------------------------------
// Simple pixel shader
// -------------------------------------------------------------
float4 PS_Copy( rageVertexBlit IN): COLOR
{
     return tex2D(DiffuseSampler, IN.texCoord0);
}

half4 PS_Clear(rageVertexClear IN): COLOR
{
	return ClearColor;
}

technique Clear
{
    pass p0
    {
        VertexShader = compile VERTEXSHADER VS_Clear();
        PixelShader  = compile PIXELSHADER PS_Clear();
    }
}

#if __PS3 || RSG_ORBIS || __D3D11
// ----------------------------------------------------------------------------------------------- //

struct PSoutMrt2
{
	half4	color0	: SV_Target0;
	half4	color1	: SV_Target1;
};

// ----------------------------------------------------------------------------------------------- //

struct PSoutMrt3
{
	half4	color0	: SV_Target0;
	half4	color1	: SV_Target1;
	half4	color2	: SV_Target2;
};

// ----------------------------------------------------------------------------------------------- //

struct PSoutMrt4
{
	half4	color0	: SV_Target0;
	half4	color1	: SV_Target1;
	half4	color2	: SV_Target2;
	half4	color3	: SV_Target3;
};

// ----------------------------------------------------------------------------------------------- //

float PS_ClearR32(rageVertexClear IN): COLOR
{
	return ClearColor.x;
}

// ----------------------------------------------------------------------------------------------- //

PSoutMrt2 PS_ClearMrt2(rageVertexClear IN)
{
	PSoutMrt2 OUT;
	OUT.color0 = ClearColor;
	OUT.color1 = ClearColor;
	return(OUT);
}

// ----------------------------------------------------------------------------------------------- //

PSoutMrt3 PS_ClearMrt3(rageVertexClear IN)
{
	PSoutMrt3 OUT;
	OUT.color0 = ClearColor;
	OUT.color1 = ClearColor;
	OUT.color2 = ClearColor;
	return(OUT);
}

// ----------------------------------------------------------------------------------------------- //

PSoutMrt4 PS_ClearMrt4(rageVertexClear IN)
{
	PSoutMrt4 OUT;
	OUT.color0 = ClearColor;
	OUT.color1 = ClearColor;
	OUT.color2 = ClearColor;
	OUT.color3 = ClearColor;
	return(OUT);
}

technique ClearMrt2
{
    pass p0
    {
		VertexShader = compile VERTEXSHADER VS_Clear();
		PixelShader  = compile PIXELSHADER PS_ClearMrt2();
	}
}

technique ClearMrt3
{
    pass p0
    {
		VertexShader = compile VERTEXSHADER VS_Clear();
		PixelShader  = compile PIXELSHADER PS_ClearMrt3();
	}
}

technique ClearMrt4
{
    pass p0
    {
		VertexShader = compile VERTEXSHADER VS_Clear();
		PixelShader  = compile PIXELSHADER PS_ClearMrt4();
	}
}

#if RSG_ORBIS
technique ClearArray
{
	pass p0
	{
		VertexShader = compile VSGS_SHADER VS_ClearGS();
		SetGeometryShader(compileshader(gs_5_0, GS_Clear()));
		PixelShader  = compile PIXELSHADER PS_Clear()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
	}
}
#endif

// ----------------------------------------------------------------------------------------------- //

technique ClearR32
{
	pass p0
	{
		VertexShader = compile VERTEXSHADER VS_Clear();
		PixelShader  = compile PIXELSHADER PS_ClearR32() CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1))	PS4_TARGET(FMT_32_R);
	}
}

#endif // __PS3 || RSG_ORBIS || __D3D11

#if __PS3
// -------------------------------------------------------------
// 
// -------------------------------------------------------------
rageVertexMsaa VS_Msaa2x(rageVertexMsaa IN) 
{
	rageVertexMsaa OUT = (rageVertexMsaa)0;
	
	OUT.pos =IN.pos;
	OUT.texCoord0 = IN.texCoord0 + TexelSize.xy;
 
	return OUT;
}

// -------------------------------------------------------------
// 
// -------------------------------------------------------------
rageVertexMsaa VS_Msaa2x2Tap(rageVertexMsaa IN) 
{
	rageVertexMsaa OUT = (rageVertexMsaa)0;
	
	OUT.pos =IN.pos;
	OUT.texCoord0 = IN.texCoord0;
 
	return OUT;
}

// -------------------------------------------------------------
// 
// -------------------------------------------------------------
rageVertexMsaa VS_4xAA(rageVertexMsaa IN) 
{
	rageVertexMsaa OUT = (rageVertexMsaa)0;
	
	OUT.pos =IN.pos;
	OUT.texCoord0 = IN.texCoord0;
 
	return OUT;
}

#if RAGE_ENCODEOPAQUECOLOR
float4 PS_Msaa2xOpaque(rageVertexMsaa IN) : COLOR
{
	float2 halfTexelSizeX = float2(TexelSize.x * 0.5f, 0.0f);
	return rageEncodeOpaqueColor((rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 + halfTexelSizeX)) +
		rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 - halfTexelSizeX))) * 0.5f);
}

float4 PS_Msaa4x(rageVertexMsaa IN) : COLOR
{
	float2 halfTexelSizeX = float2(TexelSize.x * 0.5f, 0.0f);
	float2 halfTexelSizeY = float2(0.0f, TexelSize.y * 0.5f);
	
	float3 color = rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 + halfTexelSizeX));
	color += rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 - halfTexelSizeX));
	color += rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 + halfTexelSizeY));
	color += rageDecodeOpaqueColor(tex2D(RenderMapPointSampler, IN.texCoord0 - halfTexelSizeY));
	
	return rageEncodeOpaqueColor(color * 0.25f);
}
#else
float4 PS_Msaa2xAccuview( rageVertexMsaa IN): COLOR
{
	float2 sampleATexCoords = rageGetPs3Msaa2xSampleA(IN.texCoord0, TexelSize.z);
	float2 sampleBTexCoords = rageGetPs3Msaa2xSampleB(IN.texCoord0, TexelSize.z);
	float4 sampleA = tex2D(RenderMapQuincunxSampler, sampleATexCoords);
	float4 sampleB = tex2D(RenderMapQuincunxAltSampler, sampleBTexCoords);
	
	return (sampleA + sampleB) * 0.5f;
}

float4 PS_Msaa2xQuincunx( rageVertexMsaa IN): COLOR
{
	// Quincunx
	float2 d0 = float2(0.0001f, 0.0f);
	return tex2D(RenderMapQuincunxSampler, IN.texCoord0 + d0);
}

float4 PS_Msaa2xQuincunxAlt( rageVertexMsaa IN): COLOR
{
	// Quincunx Alt.
	float2 d0 = float2(-0.0001f, -0.001f);
	return tex2D(RenderMapQuincunxAltSampler, IN.texCoord0 + d0);
}

float4 PS_Msaa2x2Tap( rageVertexMsaa IN): COLOR
{
	float2 sampleATexCoords = rageGetPs3Msaa2xSampleA(IN.texCoord0, TexelSize.z);
	float2 sampleBTexCoords = rageGetPs3Msaa2xSampleB(IN.texCoord0, TexelSize.z);
	float4 sampleA = tex2D(RenderMapPointSampler, sampleATexCoords);
	float4 sampleB = tex2D(RenderMapPointSampler, sampleBTexCoords);
	
	return (sampleA + sampleB) * 0.5f;
}

float4 PS_Msaa4x( rageVertexMsaa IN): COLOR
{
     return tex2D(RenderMapGaussSampler, IN.texCoord0);
}
#endif // RAGE_ENCODEOPAQUECOLOR

float4 PS_Msaa2xTransparent(rageVertexMsaa IN) : COLOR
{
	float2 halfTexelSizeX = float2(TexelSize.x * 0.5f, 0.0f);
	float4 src = (tex2D(TransparentSrcMapSampler, IN.texCoord0 + halfTexelSizeX) +
		tex2D(TransparentSrcMapSampler, IN.texCoord0 - halfTexelSizeX)) * 0.5f;
	float3 dest = rageDecodeOpaqueColor(h4tex2D(TransparentDstMapSampler, IN.texCoord0));
		
	return rageEncodeOpaqueColor(NormalBlend(src, dest));
}

float4 PS_Msaa4xTransparent(rageVertexMsaa IN) : COLOR
{
	float2 halfTexelSizeX = float2(TexelSize.x * 0.5f, 0.0f);
	float2 halfTexelSizeY = float2(0.0f, TexelSize.y * 0.5f);
	
	float4 src = tex2D(TransparentSrcMapSampler, IN.texCoord0 + halfTexelSizeX);
	src += tex2D(TransparentSrcMapSampler, IN.texCoord0 - halfTexelSizeX);
	src += tex2D(TransparentSrcMapSampler, IN.texCoord0 + halfTexelSizeY);
	src += tex2D(TransparentSrcMapSampler, IN.texCoord0 - halfTexelSizeY);
	src *= 0.25f;
	
	float3 dest = rageDecodeOpaqueColor(h4tex2D(TransparentDstMapSampler, IN.texCoord0));
	
	return rageEncodeOpaqueColor(NormalBlend(src, dest));
}

#if RAGE_ENCODEOPAQUECOLOR
technique Msaa2xOpaque
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x();
        PixelShader  = compile PIXELSHADER PS_Msaa2xOpaque();
    }
}
#else
technique Msaa2xAccuview
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x2Tap();
        PixelShader  = compile PIXELSHADER PS_Msaa2xAccuview();
    }
}

technique Msaa2xQuincunx
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x();
        PixelShader  = compile PIXELSHADER PS_Msaa2xQuincunx();
    }
}

technique Msaa2xQuincunxAlt
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x();
        PixelShader  = compile PIXELSHADER PS_Msaa2xQuincunxAlt();
    }
}

technique Msaa2x2Tap
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x2Tap();
        PixelShader  = compile PIXELSHADER PS_Msaa2x2Tap();
    }
}
#endif // RAGE_ENCODEOPAQUECOLOR

technique Msaa2xTransparent
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Msaa2x();
        PixelShader  = compile PIXELSHADER PS_Msaa2xTransparent();
    }
}

technique Msaa4x
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_4xAA();
        PixelShader  = compile PIXELSHADER PS_Msaa4x();
    }
}

technique Msaa4xTransparent
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_4xAA();
        PixelShader  = compile PIXELSHADER PS_Msaa4xTransparent();
    }
}
#endif // __PS3

technique TECH_NAME(draw)
{
	pass p0 
	{        
		VertexShader = compile VERTEXSHADER VS_Rage_TransformLit();
		PixelShader  = compile PIXELSHADER PS_Rage_Textured();
	}
}

technique TECH_NAME(unlit_draw)
{
	pass p0 
	{        
		VertexShader = compile VERTEXSHADER VS_Rage_TransformUnlit();
		PixelShader  = compile PIXELSHADER PS_Rage_Textured();
	}
}

technique TECH_NAME(drawskinned)
{
	pass p0 
	{        
		VertexShader = compile VERTEXSHADER VS_Rage_TransformLit();
		PixelShader  = compile PIXELSHADER PS_Rage_Textured();
	}
}

technique TECH_NAME(unlit_drawskinned)
{
	pass p0 
	{        
		VertexShader = compile VERTEXSHADER VS_Rage_TransformUnlit();
		PixelShader  = compile PIXELSHADER PS_Rage_Textured();
	}
}

technique TECH_NAME(blit_draw)
{
    pass p0
    {
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_Blit();
    }
}

technique CopyDepth
{
    pass p0
    {
		AlphaBlendEnable = false;
		AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_BlitDepth()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
    }
}

technique Copy
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_Copy()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
    }
}

technique CopyTransparent
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
#if RAGE_ENCODEOPAQUECOLOR
        AlphaBlendEnable = false;
#else
        AlphaBlendEnable = true;
#endif // RAGE_ENCODEOPAQUECOLOR
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_BlitTransparent()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
    }
}

technique CopyTransparentEdgeBlur
{
    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
		AlphaBlendEnable = true;
		AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_CopyTransparentEdgeBlur()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
    }
}

technique BlitTransparentEdgeBlur
{
    pass P0
    {
		CullMode = NONE;
  		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
    
        VertexShader = compile VERTEXSHADER VS_Blit();
        PixelShader  = compile PIXELSHADER PS_BlitTransparentEdgeBlur()  CGC_FLAGS(CGC_DEFAULTFLAGS_NPC(1));
    }
}


#endif // __RAGE_IM_FX
