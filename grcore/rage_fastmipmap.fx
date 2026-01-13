#define DISABLE_RAGE_LIGHTING
#define NO_SKINNING
#include "../shaderlib/rage_common.fxh"
#include "../shaderlib/rage_shadowmap_common.fxh"

// Fast mip map code for creating fast mip maps of the tree imposters.
//------------------------------------------------------------------------------------

#pragma dcl position diffuse texcoord1 normal

#if HACK_GTA4
	#if __PS3
		#define FLOAT4 half4
	#else // __PS3
		#define FLOAT4 float4
	#endif // __PS3
	#pragma constant 50
#endif

BeginSampler(sampler2D,BaseTexture,BaseSampler, BaseTex)
    string UIName = "Base Texture";
ContinueSampler(sampler2D,BaseTexture,BaseSampler,BaseTex)
		AddressU  = CLAMP;        
		AddressV  = CLAMP;
		AddressW  = CLAMP;
		MIN_LINEAR_MAG_LINEAR_MIP_LINEAR;
EndSampler;

BeginConstantBufferDX10(rage_fastmipmap_locals)
float4 TexelSize;
const float AlphaRange = 255.0f / 84.0f;
const float MipMapLod : MipMapLod;
EndConstantBufferDX10(rage_fastmipmap_locals)

float4 FastMipMap(rageVertexOutputPassThrough IN)
{
#if 1
	float2 HalfTexelSize = TexelSize.xy * 0.5f;
	float4 res = ( tex2D(BaseSampler, IN.texCoord0.xy + float2(HalfTexelSize.x, HalfTexelSize.y))
			+ tex2D(BaseSampler, IN.texCoord0.xy + float2(-HalfTexelSize.x, HalfTexelSize.y))
			+ tex2D(BaseSampler, IN.texCoord0.xy + float2(HalfTexelSize.x, -HalfTexelSize.y))
			+ tex2D(BaseSampler, IN.texCoord0.xy + float2(-HalfTexelSize.x, -HalfTexelSize.y)))
			* 0.25f;
#else
	float2 offset = float2( -1.5f, 0.5f ) * TexelSize.xy;
	float4 s1 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.xx );
	float4 s2 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.yx );
	float4 s3 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.xy );
	float4 s4 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.yy );
	
	float4 weights = float4( s1.w, s2.w, s3.w, s4.w );
	weights = saturate( weights * AlphaRange );
	
	float4 res = s1 * weights.x + s2 * weights.y + s3 * weights.z + s4 * weights.w;
	res *= 1.0f/ dot( weights, 1.0f);
#endif

	
	return res;
}

struct Out4Targets
{
	FLOAT4  col0 : SV_Target0;
	FLOAT4  col1 : SV_Target1;
	FLOAT4  col2 : SV_Target2;
	FLOAT4  col3 : SV_Target3;
};

// -------------------------------------------------------------
// PSFastMipMap
// - creates a mip map from a larger texture by using 4 samples in conjunction with bilinear filtering to
//	create the texture with a simple box filter.
// -------------------------------------------------------------
Out4Targets PSFastMipMap( rageVertexOutputPassThrough IN )
{
	Out4Targets OUT;

	FLOAT4 c1 = FastMipMap(IN);

	// modified so outputs to 4 rendertargets
	OUT.col0 = c1;
	OUT.col1 = c1;
	OUT.col2 = c1;
	OUT.col3 = c1;

	return OUT;

}

FLOAT4 PSFastMipMapCutOut( rageVertexOutputPassThrough IN ): COLOR
{
	float4 res = FastMipMap(IN);
#if __PS3
	if (res.a < 0.5f)
	{
		discard;
	}
#else
	clip(res.a - 0.5f);
#endif

	return res;
}


// -------------------------------------------------------------
// PSFastMipMap
// - creates a mip map from a larger texture by using 4 samples in conjunction with bilinear filtering to
//	create the texture with a simple box filter.
// -------------------------------------------------------------
FLOAT4 PSDownSampleX2( rageVertexOutputPassThroughTexOnly IN): COLOR
{
	float2	offset = float2( -0.5f, 0.5f ) * TexelSize.xy;
	return tex2D( BaseSampler, IN.texCoord0.xy + offset.xx); // just get a bilinearly filtered sample of the texture
}

FLOAT4 PSBlur( rageVertexOutputPassThroughTexOnly IN): COLOR
{
	float2	offset = float2( -1.0f, 1.0f ) *  TexelSize.xy;
	float4 s1 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.xx );
	float4 s2 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.yx  );
	float4 s3 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.xy  );
	float4 s4 = tex2D( BaseSampler, IN.texCoord0.xy +  offset.yy );   // may want to calculate offsets properally so does guassian blur	
	return ( s1 + s2 + s3 + s4 ) / 4.0f;
}

// -------------------------------------------------------------
// DownsampleDepthPS
// - downsamples the DepthTextureSampler0 depth texture very fast
// -------------------------------------------------------------
void DownsampleDepthPS( rageVertexOutputPassThroughTexOnly IN,
                        out float4 oColor : COLOR)//,
                        //out float oDepth : DEPTH )
{
    // Fetch four samples
    float4 SampledDepth;
#if __XENON
	float2 Tex = IN.texCoord0.xy;
    asm {
        tfetch2D SampledDepth.x___, Tex, DepthTextureSampler0, OffsetX = -0.5, OffsetY = -0.5
        tfetch2D SampledDepth._x__, Tex, DepthTextureSampler0, OffsetX =  0.5, OffsetY = -0.5
        tfetch2D SampledDepth.__x_, Tex, DepthTextureSampler0, OffsetX = -0.5, OffsetY =  0.5
        tfetch2D SampledDepth.___x, Tex, DepthTextureSampler0, OffsetX =  0.5, OffsetY =  0.5
    };
#elif __PS3
		SampledDepth.x = texDepth2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2(-0.5, -0.5)).x;
 		SampledDepth.y = texDepth2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2( 0.5, -0.5)).x;
		SampledDepth.z = texDepth2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2(-0.5,  0.5)).x;
		SampledDepth.w = texDepth2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2( 0.5,  0.5)).x;
#else
		SampledDepth.x = tex2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2(-0.5, -0.5)).x;
 		SampledDepth.y = tex2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2( 0.5, -0.5)).x;
		SampledDepth.z = tex2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2(-0.5,  0.5)).x;
		SampledDepth.w = tex2D( DepthTextureSampler0, IN.texCoord0.xy.xy + float2( 0.5,  0.5)).x;
#endif   
    
    // Find the maximum.
    SampledDepth.xy = max( SampledDepth.xy, SampledDepth.zw );
    SampledDepth.x = max( SampledDepth.x, SampledDepth.y );

    oColor = SampledDepth.x;
 //   oDepth = SampledDepth.x;
}


//----------------------------------------------------------------------------------------------

technique draw
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXform();
        PixelShader  = compile PIXELSHADER  PSFastMipMap();
    }
}

technique drawBlit
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXform();
        PixelShader  = compile PIXELSHADER  PSFastMipMap();
    }
}

technique drawBlur
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXformTexOnly();
        PixelShader  = compile PIXELSHADER  PSBlur();
    }
}

technique drawBlitX2
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXformTexOnly();
        PixelShader  = compile PIXELSHADER  PSDownSampleX2();
    }
}

technique drawDepth
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXformTexOnly();
        PixelShader  = compile PIXELSHADER  DownsampleDepthPS();
    }
}

technique drawCutOut
{

    pass P0
    {
		CullMode = NONE;
		ZEnable = false;
		ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        VertexShader = compile VERTEXSHADER VS_ragePassThroughNoXform();
        PixelShader  = compile PIXELSHADER  PSFastMipMapCutOut();
    }
}
