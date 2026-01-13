//
// grcore/edge_callbacks_gta4.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//
#if HACK_GTA4 && __PS3
#include "effect_config.h"	// USE_EDGE
#include "../../../../game/shader_source/vehicles/vehicle_common_values.h"

#if __SPU
#include <spu_intrinsics.h>

#include "edge/geom/edgegeom.h"
#include "edge/geom/edgegeom_config.h"

#define VEHICLEDAMAGE_SPA			(1)

#if VEHICLEDAMAGE_SPA
#	include "grcore/edge_vehicledamage.h"
#endif

// from edgegeom_masks.cpp
extern const vec_uchar16 s_000A000C000E000G;
extern const vec_uchar16 s_0A0B0c0d;
extern const vec_uchar16 s_AaBb;
extern const vec_uchar16 s_AaCc;
extern const vec_uchar16 s_ABa0;
extern const vec_uchar16 s_ABCD;
extern const vec_uchar16 s_ABCDEFGHd000MNOP;
extern const vec_uchar16 s_BCAD;
extern const vec_uchar16 s_BbDd;
extern const vec_uchar16 s_BCDD;
extern const vec_uchar16 s_CcCc;
extern const vec_uchar16 s_CDa0;
extern const vec_uchar16 s_cdAB;
extern const vec_uchar16 s_DdBb;
extern const vec_uchar16 s_DdCc;
extern const vec_uchar16 s_FBDbdf00;

// from edgegeom_cull.cpp
extern void transformVertexesForCull(EdgeGeomSpuContext *ctx);

#define EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK Gta5TransformVertexesForCull // this just calls transformVertexesForCull
#define EDGE_GEOM_PRETRANSFORM_CALLBACK                Gta4PreTransformCallback
#define EDGE_GEOM_POSTSKINING_CALLBACK                 Gta4PostSkiningCallback
#define EDGE_GEOM_PRECOMPRESS_CALLBACK                 Gta4PreCompressCallback


#if !VEHICLEDAMAGE_SPA
//
//
//
//
__forceinline
void GetRadialPos(vec_float4& Rx, vec_float4& Ry, vec_float4& Rz,
				  vec_float4 u, vec_float4 v,
				  const vec_float4 invHalfDim, const vec_float4 vecOne, const vec_float4 vecTwo, const vec_float4 vecMinusOne)
{													
	//texSampleCoords -= 0.5f;
	//texSampleCoords *= 2.0f;
	vec_float4 un		= spu_msub(u, invHalfDim, vecOne);
	vec_float4 vn		= spu_msub(v, invHalfDim, vecOne);
	vec_float4 u2v2		= spu_madd(vn, vn, spu_mul(un, un));
	
	//float fLength = 0.0f;
	//if(dot(texSampleCoords, texSampleCoords) > 0.0f)
	//	fLength = length(texSampleCoords);
	vec_float4 fLength	= spu_mul(u2v2, spu_rsqrte(u2v2));

	//R.z = 1.0f - 2.0f*fLength;
	Rz	= spu_nmsub(vecTwo, fLength, vecOne);				
	//if(R.z > 1.0f) R.z = 1.0f;
	Rz = spu_sel(Rz, vecOne,		spu_cmpgt(Rz, vecOne));
	//else if(R.z < -1.0f) R.z = -1.0f;
	Rz = spu_sel(Rz, vecMinusOne,	spu_cmpgt(vecMinusOne,Rz));


	//	float fXYMult = 0.0f;
	//	if(R.z < 1.0f && R.z > -1.0f && fLength > 0.0f)
	//		fXYMult = sqrt(1.0f - R.z*R.z) / fLength;
	vec_float4 tmp0		= spu_nmsub(Rz, Rz, vecOne);					
	vec_float4 tmp1		= spu_rsqrte(tmp0);						
	vec_float4 fXYMult	= spu_mul(spu_mul(tmp0, tmp1), fLength);

	//R.x = texSampleCoords.x * fXYMult;
	//R.y = texSampleCoords.y * fXYMult;
	Rx	= spu_mul(un, fXYMult);							
	Ry	= spu_mul(vn, fXYMult);							
}

//
//
//
//
void ApplyVehicleDamage(const EdgeGeomSpuContext* ctx, const void* damageTex, const float *damageTexOffset, float boundRadius)
{
//	__asm__("stopd 0,0,0");
	const vec_uint4 texturepitch= spu_splats( u32(GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3) );
	const vec_uint4 texelsize	= { 3,3,3,3 };
	const vec_uchar16 splat3	= { 0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03 };
	const vec_uchar16 splat7	= { 0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07 };
	const vec_uchar16 splatB	= { 0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b };
	const vec_uchar16 splatF	= { 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f };

	const vec_uint4 leftmask	= { 0x00010203,0x04058080,0x80808080,0x80808080 };
	const vec_uint4 rightmask	= { 0x80808080,0x80808080,0x00010203,0x04058080 };

	const vec_uchar16 getlX		= { 0x00,0x80,0x80,0x80, 0x08,0x80,0x80,0x80, 0x10,0x80,0x80,0x80, 0x18,0x80,0x80,0x80 };
	const vec_uchar16 getlY		= { 0x01,0x80,0x80,0x80, 0x09,0x80,0x80,0x80, 0x11,0x80,0x80,0x80, 0x19,0x80,0x80,0x80 };
	const vec_uchar16 getlZ		= { 0x02,0x80,0x80,0x80, 0x0a,0x80,0x80,0x80, 0x12,0x80,0x80,0x80, 0x1a,0x80,0x80,0x80 };
	const vec_uchar16 getrX		= { 0x03,0x80,0x80,0x80, 0x0b,0x80,0x80,0x80, 0x13,0x80,0x80,0x80, 0x1b,0x80,0x80,0x80 };
	const vec_uchar16 getrY		= { 0x04,0x80,0x80,0x80, 0x0c,0x80,0x80,0x80, 0x14,0x80,0x80,0x80, 0x1c,0x80,0x80,0x80 };
	const vec_uchar16 getrZ		= { 0x05,0x80,0x80,0x80, 0x0d,0x80,0x80,0x80, 0x15,0x80,0x80,0x80, 0x1d,0x80,0x80,0x80 };

	const vec_uchar16 shufAaBb	= { 0x00,0x01,0x02,0x03, 0x10,0x11,0x12,0x13, 0x04,0x05,0x06,0x07, 0x14,0x15,0x16,0x17 };
	const vec_uchar16 shufBbAa	= { 0x04,0x05,0x06,0x07, 0x14,0x15,0x16,0x17, 0x00,0x01,0x02,0x03, 0x10,0x11,0x12,0x13 };
	const vec_uchar16 shufCcDd	= { 0x08,0x09,0x0A,0x0B, 0x18,0x19,0x1A,0x1B, 0x0C,0x0D,0x0E,0x0F, 0x1C,0x1D,0x1E,0x1F };
	const vec_uchar16 shufDdCc	= { 0x0C,0x0D,0x0E,0x0F, 0x1C,0x1D,0x1E,0x1F, 0x08,0x09,0x0A,0x0B, 0x18,0x19,0x1A,0x1B };
	const vec_uchar16 shufCDab	= { 0x08,0x09,0x0A,0x0B, 0x0C,0x0D,0x0E,0x0F, 0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17 };
	const vec_uchar16 selABcd	= { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF };

	const vec_uchar16 shufAaCD	= { 0x00,0x01,0x02,0x03, 0x10,0x11,0x12,0x13, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };
	const vec_uchar16 shufAbCD	= { 0x00,0x01,0x02,0x03, 0x14,0x15,0x16,0x17, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };
	const vec_uchar16 shufAcCD	= { 0x00,0x01,0x02,0x03, 0x18,0x19,0x1a,0x1b, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };
	const vec_uchar16 shufAdCD	= { 0x00,0x01,0x02,0x03, 0x1c,0x1d,0x1e,0x1f, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f };

	const vec_float4 vecZero	= spu_splats(0.0f);
	const vec_float4 vecZeroOne	= spu_splats(0.1f);
	const vec_float4 vecZero3333= spu_splats(0.3333f);
	const vec_float4 vecOne		= spu_splats(1.0f);
	const vec_float4 vecMinusOne= spu_splats(-1.0f);
	const vec_float4 vecTwo		= spu_splats(2.0f);
	const vec_float4 invHalfDim	= spu_splats(2.0f / GTA_VEHICLE_DAMAGE_TEXTURE_SIZE);
	const vec_float4 dimo2		= spu_splats(GTA_VEHICLE_DAMAGE_TEXTURE_SIZE / 2.0f);
	const vec_float4 dimo4		= spu_splats(GTA_VEHICLE_DAMAGE_TEXTURE_SIZE / 4.0f);
	const vec_float4 dimClamp	= spu_splats(GTA_VEHICLE_DAMAGE_TEXTURE_SIZE - 1.0001f);
	const vec_float4 deltaScale	= spu_splats(GTA_VEHICLE_DAMAGE_DELTA_SCALE);
	const vec_float4 deltaScaleOverBoundRadius	= spu_splats(GTA_VEHICLE_DAMAGE_DELTA_SCALE / boundRadius);

	const vec_uint4 dmgTexp		= (vec_uint4)spu_splats((u32)damageTex);
	const vec_float4 dmgTexOffX = spu_splats(damageTexOffset[0]);
	const vec_float4 dmgTexOffY = spu_splats(damageTexOffset[1]);
	const vec_float4 dmgTexOffZ = spu_splats(damageTexOffset[2]);

	vec_float4* __restrict__ pPos	= (vec_float4*)ctx->positionTable;
	vec_float4* __restrict__ pNorm	= (vec_float4*)ctx->normalTable;
	vec_float4* __restrict__ pDiffuse=(vec_float4*)edgeGeomGetUniformTableByAttribute((EdgeGeomSpuContext*)ctx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);

	const vec_uint4	DiffuseSel = spu_splats( u32(pDiffuse? 0xffffffff : 0x00000000) );

	vec_float4 fakeDiffuseStorage[4];

	// 4 verts per loop
	const u32 count4 = (ctx->spuConfigInfo.numVertexes + 3) >> 2;
	for(u32 i=0; i<count4; i++)
	{
		vec_float4 tmp0, tmp1, tmp2, tmp3, tmp4;
		// ------------------------------------------------------------
		// deform positions
		// ------------------------------------------------------------

		// load positions x 4
		vec_float4 p0 = pPos[0];
		vec_float4 p1 = pPos[1];
		vec_float4 p2 = pPos[2];
		vec_float4 p3 = pPos[3];

		// swizzle positions xyzw xyzw xyzw xyzw -> xxxx yyyy zzzz
		vec_float4 ps0	= spu_shuffle(p0, p1, shufAaBb);
		vec_float4 ps1	= spu_shuffle(p0, p1, shufCcDd);
		vec_float4 ps2	= spu_shuffle(p2, p3, shufBbAa);
		vec_float4 ps3	= spu_shuffle(p2, p3, shufDdCc);
		vec_float4 x	= spu_sel(ps0, ps2, selABcd);
		vec_float4 y	= spu_shuffle(ps0, ps2, shufCDab);
		vec_float4 z	= spu_sel(ps1, ps3, selABcd);

		vec_float4 Rx	= spu_add(x, dmgTexOffX);
		vec_float4 Ry	= spu_add(y, dmgTexOffY);
		vec_float4 Rz	= spu_add(z, dmgTexOffZ);

		// load diffuse color x4
		vec_float4 diffuse0	= pDiffuse[0]; 
		vec_float4 diffuse1	= pDiffuse[1];
		vec_float4 diffuse2	= pDiffuse[2];
		vec_float4 diffuse3	= pDiffuse[3];
		// damage scale: swizzle diffuse.G into GGGG:
		vec_float4 ds0		= spu_shuffle(diffuse0, diffuse1, shufAaBb);	// ds0=R0R1 G0G1
		//vec_float4 ds1	= spu_shuffle(diffuse0, diffuse1, shufCcDd);
		vec_float4 ds2		= spu_shuffle(diffuse2, diffuse3, shufBbAa);	// ds2=G2G3 R2R3
		//vec_float4 ds3	= spu_shuffle(diffuse2, diffuse3, shufDdCc);
		//vec_float4 rrrr	= spu_sel(ds0, ds2, selABcd);
		vec_float4 gggg		= spu_shuffle(ds0, ds2, shufCDab);
		//vec_float4 bbbb	= spu_sel(ds1, ds3, selABcd);
		vec_float4 gDmgScale= spu_sel(vecOne, gggg, DiffuseSel);	// gDmgScale= pDiffuse? gggg : 1.0f;


		// get pos magnitudes
		vec_float4 x2y2		= spu_madd(Ry, Ry, spu_mul(Rx, Rx));
		vec_float4 x2y2z2	= spu_madd(Rz, Rz, x2y2);
		vec_float4 invLen	= spu_rsqrte(x2y2z2);
		vec_float4 magR		= spu_mul(x2y2z2, invLen);


		// float2 texSampleCoords = GetTexCoordFromRadialPos(R);
		// get uv from pos
		//	float zOffset = 0.5f * (1.0f - R.z);
		vec_float4 zOffset = spu_nmsub(Rz, invLen, vecOne);
		//	and re-normalise just this (will get scaled by previously normalised z)
		//	texSampleCoords = normalize(texSampleCoords);
		vec_float4 invLenxy = spu_rsqrte(x2y2);
		vec_float4 u = spu_mul(Rx, invLenxy);
		vec_float4 v = spu_mul(Ry, invLenxy);
		//	texSampleCoords *= zOffset;
		u = spu_mul(u, zOffset);
		v = spu_mul(v, zOffset);
		//	texSampleCoords *= 0.5f;
		//	texSampleCoords += 0.5f;
		u = spu_madd(u, dimo4, dimo2);
		v = spu_madd(v, dimo4, dimo2);

		// 'over clamp' uv to prevent bilinear fetches going out of bounds
		u = spu_sel(u, dimClamp, spu_cmpgt(u,dimClamp));	// u=<0; 128)
		v = spu_sel(v, dimClamp, spu_cmpgt(v,dimClamp));	// v=<0; 128)

		vec_uint4 ui = spu_convtu(u, 0);
		vec_uint4 vi = spu_convtu(v, 0);
		// get fractional parts of uv
		vec_float4 u0 = spu_convtf(ui, 0);
		vec_float4 v0 = spu_convtf(vi, 0);
		vec_float4 uf = spu_sub(u, u0);
		vec_float4 vf = spu_sub(v, v0);

//////////////////////////////////////////////////////////////////////////////////////////////
		// New version of fetch

		//float s = 1.01f/128.0f;
		//float2 texSampleOffset = fmod(texSampleCoords, s.xx);
		//float4 sample   = tex2Dlod(DamageSampler, float4(texSampleCoords					+ float2(0, 0),0,0));
		//float4 sampleR  = tex2Dlod(DamageSampler, float4(texSampleCoords - texSampleOffset	+ float2(s, 0),0,0));
		//float4 sampleU  = tex2Dlod(DamageSampler, float4(texSampleCoords - texSampleOffset	+ float2(0, s),0,0));
		//float4 sampleRU = tex2Dlod(DamageSampler, float4(texSampleCoords - texSampleOffset	+ float2(s, s),0,0));

		// Integer number of QW's per line required - This is true for 128*3 width
		vec_uint4 off = (vec_uint4)si_mpya((qword)ui, (qword)texelsize, (qword)dmgTexp);
		vec_uint4 byteOff = spu_and(off, 15);
		
		// As V pitch is multiple of 16, just take byte offset directly from U
		off = (vec_uint4)si_mpya( (qword)vi, (qword)texturepitch, (qword)off );
		vec_uint4 off1 = spu_slqwbyte(off,  4);
		vec_uint4 off2 = spu_slqwbyte(off,  8);
		vec_uint4 off3 = spu_slqwbyte(off, 12);

		// Build 4 extraction masks
		vec_uint4 mask0 = spu_add( leftmask,  (vec_uint4)spu_shuffle(byteOff, byteOff, splat3));
		vec_uint4 mask1 = spu_add( rightmask, (vec_uint4)spu_shuffle(byteOff, byteOff, splat7));
		vec_uint4 mask2 = spu_add( leftmask,  (vec_uint4)spu_shuffle(byteOff, byteOff, splatB));
		vec_uint4 mask3 = spu_add( rightmask, (vec_uint4)spu_shuffle(byteOff, byteOff, splatF));

		// Get 4 QWs for vert 0
		vec_int4 tl = (vec_int4)si_lqd( (qword)off,0 );
		vec_int4 tr = (vec_int4)si_lqd( (qword)off,16 );
		vec_int4 bl = (vec_int4)si_lqd( (qword)off,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 );
		vec_int4 br = (vec_int4)si_lqd( (qword)off,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 + 16 );

		// Aligned pixel pairs
		qword t0 = si_shufb( (qword)tl,(qword)tr,(qword)mask0 );
		qword b0 = si_shufb( (qword)bl,(qword)br,(qword)mask0 );

		// and QW's for vert 1
		tl = (vec_int4)si_lqd( (qword)off1,0 );
		tr = (vec_int4)si_lqd( (qword)off1,16 );
		bl = (vec_int4)si_lqd( (qword)off1,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 );
		br = (vec_int4)si_lqd( (qword)off1,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 + 16 );
		qword t1 = si_shufb( (qword)tl,(qword)tr,(qword)mask1 );
		qword b1 = si_shufb( (qword)bl,(qword)br,(qword)mask1 );

		// and QW's for vert 2
		tl = (vec_int4)si_lqd( (qword)off2,0 );
		tr = (vec_int4)si_lqd( (qword)off2,16 );
		bl = (vec_int4)si_lqd( (qword)off2,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 );
		br = (vec_int4)si_lqd( (qword)off2,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 + 16 );
		qword t2 = si_shufb( (qword)tl,(qword)tr,(qword)mask2 );
		qword b2 = si_shufb( (qword)bl,(qword)br,(qword)mask2 );

		// and QW's for vert 3
		tl = (vec_int4)si_lqd( (qword)off3,0 );
		tr = (vec_int4)si_lqd( (qword)off3,16 );
		bl = (vec_int4)si_lqd( (qword)off3,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 );
		br = (vec_int4)si_lqd( (qword)off3,GTA_VEHICLE_DAMAGE_TEXTURE_SIZE*3 + 16 );
		qword t3 = si_shufb( (qword)tl,(qword)tr,(qword)mask3 );
		qword b3 = si_shufb( (qword)bl,(qword)br,(qword)mask3 );


		qword t01 = spu_or( t0,t1 ); // t01 = [RGBR|GB00|RGBR|GB00]
		qword t23 = spu_or( t2,t3 ); // t23 = [RGBR|GB00|RGBR|GB00]
		qword b01 = spu_or( b0,b1 ); // ...
		qword b23 = spu_or( b2,b3 );


		// Now extract vector of TL,TR,BL, and BR for X,Y and Z
		tl = (vec_int4)spu_shuffle( t01,t23,getlX );	// tl = [ x0<<24 | x1<<24 | x2<<24 | x3<<24 ]; - x shifted left by 24 bits, but keeps correct sign
		tr = (vec_int4)spu_shuffle( t01,t23,getrX );
		bl = (vec_int4)spu_shuffle( b01,b23,getlX );
		br = (vec_int4)spu_shuffle( b01,b23,getrX );

		vec_float4 Samplex	= spu_convtf( tl, (7+24) );		// Signed between -1 <= x < 1
		vec_float4 SampleRx	= spu_convtf( tr, (7+24) );
		vec_float4 SampleUx	= spu_convtf( bl, (7+24) );
		vec_float4 SampleRUx= spu_convtf( br, (7+24) );

		tl = (vec_int4)spu_shuffle( t01,t23,getlY );
		tr = (vec_int4)spu_shuffle( t01,t23,getrY );
		bl = (vec_int4)spu_shuffle( b01,b23,getlY );
		br = (vec_int4)spu_shuffle( b01,b23,getrY );

		vec_float4 Sampley	= spu_convtf( tl, (7+24) );		// Signed between -1 <= x < 1
		vec_float4 SampleRy = spu_convtf( tr, (7+24) );
		vec_float4 SampleUy = spu_convtf( bl, (7+24) );
		vec_float4 SampleRUy= spu_convtf( br, (7+24) );

		tl = (vec_int4)spu_shuffle( t01,t23,getlZ );
		tr = (vec_int4)spu_shuffle( t01,t23,getrZ );
		bl = (vec_int4)spu_shuffle( b01,b23,getlZ );
		br = (vec_int4)spu_shuffle( b01,b23,getrZ );

		vec_float4 Samplez	= spu_convtf( tl, (7+24) );		// Signed between -1 <= x < 1
		vec_float4 SampleRz = spu_convtf( tr, (7+24) );
		vec_float4 SampleUz = spu_convtf( bl, (7+24) );
		vec_float4 SampleRUz= spu_convtf( br, (7+24) );
//////////////////////////////////////////////////////////////////////////////////////////////


		// bilinear interpolation x 4

		//float2 normalisedSampleOffset = texSampleOffset / s;
		//float4 smoothedSample = sample*(1.0f - normalisedSampleOffset.x)*(1.0f - normalisedSampleOffset.y);
		//smoothedSample += sampleR*(normalisedSampleOffset.x)*(1.0f - normalisedSampleOffset.y);
		//smoothedSample += sampleU*(1.0f - normalisedSampleOffset.x)*(normalisedSampleOffset.y);
		//smoothedSample += sampleRU*(normalisedSampleOffset.x)*(normalisedSampleOffset.y);
		tmp0 = spu_nmsub(Samplex, uf, Samplex);
		tmp1 = spu_nmsub(SampleUx,uf, SampleUx);
		tmp2 = spu_madd(SampleRx, uf, tmp0);
		tmp3 = spu_madd(SampleRUx,uf, tmp1);
		tmp4 = spu_nmsub(tmp2, vf, tmp2);
		vec_float4 smoothedSampleX = spu_madd(tmp3, vf, tmp4);

		tmp0 = spu_nmsub(Sampley, uf, Sampley);
		tmp1 = spu_nmsub(SampleUy,uf, SampleUy);
		tmp2 = spu_madd(SampleRy, uf, tmp0);
		tmp3 = spu_madd(SampleRUy,uf, tmp1);
		tmp4 = spu_nmsub(tmp2, vf, tmp2);
		vec_float4 smoothedSampleY = spu_madd(tmp3, vf, tmp4);

		tmp0 = spu_nmsub(Samplez, uf, Samplez);
		tmp1 = spu_nmsub(SampleUz,uf, SampleUz);
		tmp2 = spu_madd(SampleRz, uf, tmp0);
		tmp3 = spu_madd(SampleRUz,uf, tmp1);
		tmp4 = spu_nmsub(tmp2, vf, tmp2);
		vec_float4 smoothedSampleZ = spu_madd(tmp3, vf, tmp4);

		// scale samples
		//smoothedSample *= min(1.0f, magR / BoundRadius);
		//smoothedSample *= GTA_VEHICLE_DAMAGE_DELTA_SCALE;
		vec_float4 lenscale			= spu_mul(magR, deltaScaleOverBoundRadius);
		vec_float4 SmoothSampleScale= spu_sel(lenscale, deltaScale, spu_cmpgt(lenscale, deltaScale));

		//float retVal = saturate(1.0f-saturate(length(smoothedSample.xyz))*4.0f);
		vec_float4 dSmoothedSampleX = spu_mul(smoothedSampleX, SmoothSampleScale);
		vec_float4 dSmoothedSampleY = spu_mul(smoothedSampleY, SmoothSampleScale);
		vec_float4 dSmoothedSampleZ = spu_mul(smoothedSampleZ, SmoothSampleScale);

		vec_float4 dSmoothedSampleX2Y2Z2		= spu_madd(dSmoothedSampleX,dSmoothedSampleX, spu_madd(dSmoothedSampleY,dSmoothedSampleY, spu_mul(dSmoothedSampleZ,dSmoothedSampleZ)));
		vec_float4 dSmoothedSampleX2Y2Z2invLen	= spu_rsqrte(dSmoothedSampleX2Y2Z2);
		vec_float4 dSmoothedSampleLen			= spu_mul(dSmoothedSampleX2Y2Z2, dSmoothedSampleX2Y2Z2invLen);
		dSmoothedSampleLen = spu_sel(dSmoothedSampleLen, vecOne,	spu_cmpgt(dSmoothedSampleLen, vecOne));
		dSmoothedSampleLen = spu_sel(dSmoothedSampleLen, vecZero,	spu_cmpgt(vecZero, dSmoothedSampleLen));

		vec_float4 dSpecDmgScale = dSmoothedSampleLen;

		dSpecDmgScale = spu_nmsub(dSpecDmgScale, spu_splats(4.0f), vecOne);
		dSpecDmgScale = spu_sel(dSpecDmgScale, vecOne,	spu_cmpgt(dSpecDmgScale, vecOne));
		dSpecDmgScale = spu_sel(dSpecDmgScale, vecZero,	spu_cmpgt(vecZero, dSpecDmgScale));

		diffuse0 = spu_shuffle(diffuse0, dSpecDmgScale, shufAaCD);
		diffuse1 = spu_shuffle(diffuse1, dSpecDmgScale, shufAbCD);
		diffuse2 = spu_shuffle(diffuse2, dSpecDmgScale, shufAcCD);
		diffuse3 = spu_shuffle(diffuse3, dSpecDmgScale, shufAdCD);

		vec_uint4 pDiffuseDestSel = spu_sel((vec_uint4)si_from_ptr(fakeDiffuseStorage), (vec_uint4)si_from_ptr(pDiffuse), DiffuseSel);
		vec_float4* __restrict__ pDiffuseDest = (vec_float4*)si_to_ptr((qword)pDiffuseDestSel);

		pDiffuseDest[0] = diffuse0;
		pDiffuseDest[1] = diffuse1;
		pDiffuseDest[2] = diffuse2;
		pDiffuseDest[3] = diffuse3;

		// update positions
		// outPos = _inPos + smoothedSample.xyz*gDmgScale;
		SmoothSampleScale = spu_mul(SmoothSampleScale, gDmgScale);
		x = spu_madd(smoothedSampleX, SmoothSampleScale, x);
		y = spu_madd(smoothedSampleY, SmoothSampleScale, y);
		z = spu_madd(smoothedSampleZ, SmoothSampleScale, z);


		// deswizzle positions xxxx yyyy zzzz -> xyz1 xyz1 xyz1 xyz1
		ps0	= spu_shuffle(x, y,		shufAaBb);
		ps1	= spu_shuffle(x, y,		shufCcDd);
		ps2	= spu_shuffle(z, vecOne,shufBbAa);
		ps3	= spu_shuffle(z, vecOne,shufDdCc);
		p0	= spu_sel(ps0, ps2, selABcd);
		p1	= spu_shuffle(ps0, ps2,	shufCDab);
		p2	= spu_sel(ps1, ps3, selABcd);
		p3	= spu_shuffle(ps1, ps3, shufCDab);

		// store positions x 4
		pPos[0] = p0;
		pPos[1] = p1;
		pPos[2] = p2;
		pPos[3] = p3;

		pPos += 4;
		pDiffuse += 4;


		// ------------------------------------------------------------
		// ok now to calculate deformation's affect on the normals
		// ------------------------------------------------------------
		if(pNorm)
		{
			// load normals x 4
			vec_float4 n0 = pNorm[0];
			vec_float4 n1 = pNorm[1];
			vec_float4 n2 = pNorm[2];
			vec_float4 n3 = pNorm[3];

			// swizzle normals xyzw xyzw xyzw xyzw -> xxxx yyyy zzzz
			vec_float4 ns0	= spu_shuffle(n0, n1, shufAaBb);
			vec_float4 ns1	= spu_shuffle(n0, n1, shufCcDd);
			vec_float4 ns2	= spu_shuffle(n2, n3, shufBbAa);
			vec_float4 ns3	= spu_shuffle(n2, n3, shufDdCc);
			vec_float4 nx	= spu_sel(ns0, ns2, selABcd);
			vec_float4 ny	= spu_shuffle(ns0, ns2, shufCDab);
			vec_float4 nz	= spu_sel(ns1, ns3, selABcd);

			vec_float4 outnx= nx;
			vec_float4 outny= ny;
			vec_float4 outnz= nz;


			// work out the radial position of the base sample
			//float2 Tsample = texSampleCoords - texSampleOffset;
			//float3 Rsample = GetRadialPosFromTexCoord(Tsample);
			//sample *= min(1.0f, magR / BoundRadius);
			//sample *= GTA_VEHICLE_DAMAGE_DELTA_SCALE;
			vec_float4 RSamplex, RSampley, RSamplez;
			GetRadialPos(RSamplex, RSampley, RSamplez, u0, v0, invHalfDim, vecOne, vecTwo, vecMinusOne);
			Samplex = spu_mul(Samplex, SmoothSampleScale);
			Sampley = spu_mul(Sampley, SmoothSampleScale);
			Samplez = spu_mul(Samplez, SmoothSampleScale);


			// work out the radial position of the right sample
			//float2 TsampleR = texSampleCoords - texSampleOffset + float2(s, 0);
			//float3 RsampleR = GetRadialPosFromTexCoord(TsampleR) + float3(0.00000001f, 0.00000001f, 0.00000001f);
			//sampleR *= min(1.0f, magR / BoundRadius);
			//sampleR *= GTA_VEHICLE_DAMAGE_DELTA_SCALE;
			vec_float4 u1 = spu_add(u0, vecOne);
			vec_float4 RSampleRx, RSampleRy, RSampleRz;
			GetRadialPos(RSampleRx, RSampleRy, RSampleRz, u1, v0, invHalfDim, vecOne, vecTwo, vecMinusOne);
			SampleRx = spu_mul(SampleRx, SmoothSampleScale);
			SampleRy = spu_mul(SampleRy, SmoothSampleScale);
			SampleRz = spu_mul(SampleRz, SmoothSampleScale);



			//float3 Rdiff = RsampleR - Rsample;
			vec_float4 Rdiffx = spu_sub(RSampleRx, RSamplex);
			vec_float4 Rdiffy = spu_sub(RSampleRy, RSampley);
			vec_float4 Rdiffz = spu_sub(RSampleRz, RSamplez);
			
			//float3 Sdiff = (float3)(sampleR - sample);
			vec_float4 Sdiffx = spu_sub(SampleRx, Samplex);
			vec_float4 Sdiffy = spu_sub(SampleRy, Sampley);
			vec_float4 Sdiffz = spu_sub(SampleRz, Samplez);

			//float fMult = dot(Sdiff, _inNormal);
			vec_float4 fMult = spu_mul(Sdiffx, nx);
			fMult = spu_madd(Sdiffy, ny, fMult);
			fMult = spu_madd(Sdiffz, nz, fMult);

			//if(dot(Rdiff, Rdiff) > 0.0f)
			//{
			//	fMult /= dot(Rdiff, Rdiff);
			//	outNormal += normalDmgMask*Rdiff*fMult*gDmgScale;
			//}
			vec_float4 dotRdiff = spu_mul(Rdiffx, Rdiffx);
			dotRdiff = spu_madd(Rdiffy, Rdiffy, dotRdiff);
			dotRdiff = spu_madd(Rdiffz, Rdiffz, dotRdiff);

			vec_uint4 dotRdiffSelect = spu_cmpgt(dotRdiff, vecZero);
			fMult = spu_sel(vecZero, spu_mul(fMult, spu_re(dotRdiff)), dotRdiffSelect);
			fMult = spu_mul(fMult, gDmgScale);
			outnx = spu_madd(spu_mul(fMult,vecZeroOne), Rdiffx, outnx);
			outny = spu_madd(fMult, Rdiffy, outny);
			outnz = spu_madd(spu_mul(fMult,vecZero3333),Rdiffz, outnz);


			// work out the radial position of the upper sample
			//float2 TsampleU = texSampleCoords - texSampleOffset + float2(0, s);
			//float3 RsampleU = GetRadialPosFromTexCoord(TsampleU);
			//sampleU *= min(1.0f, magR / BoundRadius);
			//sampleU *= GTA_VEHICLE_DAMAGE_DELTA_SCALE;
			vec_float4 v1 = spu_add(v0, vecOne);
			vec_float4 RSampleUx, RSampleUy, RSampleUz;
			GetRadialPos(RSampleUx, RSampleUy, RSampleUz, u0, v1, invHalfDim, vecOne, vecTwo, vecMinusOne);
			SampleUx = spu_mul(SampleUx, SmoothSampleScale);
			SampleUy = spu_mul(SampleUy, SmoothSampleScale);
			SampleUz = spu_mul(SampleUz, SmoothSampleScale);

			//Rdiff = RsampleU - Rsample;
			Rdiffx = spu_sub(RSampleUx, RSamplex);
			Rdiffy = spu_sub(RSampleUy, RSampley);
			Rdiffz = spu_sub(RSampleUz, RSamplez);
			//Sdiff = (float3)(sampleU - sample);
			Sdiffx = spu_sub(SampleUx, Samplex);
			Sdiffy = spu_sub(SampleUy, Sampley);
			Sdiffz = spu_sub(SampleUz, Samplez);

			//fMult = dot(Sdiff, _inNormal);
			fMult = spu_mul(Sdiffx,  nx);
			fMult = spu_madd(Sdiffy, ny, fMult);
			fMult = spu_madd(Sdiffz, nz, fMult);
			
			//if(dot(Rdiff, Rdiff) > 0.0f)
			//{
			//	fMult /= dot(Rdiff, Rdiff);
			//	outNormal += normalDmgMask*Rdiff*fMult*gDmgScale;
			//}
			dotRdiff = spu_mul(Rdiffx, Rdiffx);
			dotRdiff = spu_madd(Rdiffy, Rdiffy, dotRdiff);
			dotRdiff = spu_madd(Rdiffz, Rdiffz, dotRdiff);
			dotRdiffSelect = spu_cmpgt(dotRdiff, vecZero);
			fMult = spu_sel(vecZero, spu_mul(fMult, spu_re(dotRdiff)), dotRdiffSelect);
			fMult = spu_mul(fMult, gDmgScale);
			outnx = spu_madd(spu_mul(fMult,vecZeroOne), Rdiffx, outnx);
			outny = spu_madd(fMult, Rdiffy, outny);
			outnz = spu_madd(spu_mul(fMult,vecZero3333),Rdiffz, outnz);

			// normalise normals
			//outNormal = normalize(outNormal);
			tmp0 = spu_mul(outnx, outnx);
			tmp0 = spu_madd(outny, outny, tmp0);
			tmp0 = spu_madd(outnz, outnz, tmp0);
			tmp0 = spu_rsqrte(tmp0);
			outnx = spu_mul(outnx, tmp0);
			outny = spu_mul(outny, tmp0);
			outnz = spu_mul(outnz, tmp0);

			// deswizzle normals xxxx yyyy zzzz -> xyz0 xyz0 xyz0 xyz0
			ns0 = spu_shuffle(outnx, outny,		shufAaBb);
			ns1 = spu_shuffle(outnx, outny,		shufCcDd);
			ns2 = spu_shuffle(outnz, vecZero,	shufBbAa);
			ns3 = spu_shuffle(outnz, vecZero,	shufDdCc);
			n0  = spu_sel(ns0, ns2, selABcd);
			n1  = spu_shuffle(ns0, ns2, shufCDab);
			n2  = spu_sel(ns1, ns3, selABcd);
			n3  = spu_shuffle(ns1, ns3, shufCDab);

			// store normals x 4
			pNorm[0] = n0;
			pNorm[1] = n1;
			pNorm[2] = n2;
			pNorm[3] = n3;

			pNorm += 4;
		} // if(pNorm)...

	} // for(u32 i=0; i<count4; i++)...
}// end of ApplyVehicleDamage()...
#endif //VEHICLEDAMAGE_SPA...

//
//
// set CPV.b to 1.0:
//
void ApplyZeroVehicleDamage(const EdgeGeomSpuContext* ctx)
{
	const vec_uchar16 sel00ff0000	= { 0x00,0x00,0x00,0x00, 0xff,0xff,0xff,0xff, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 };

	const vec_float4 vecOne		= spu_splats(1.0f);

	vec_float4* __restrict__ pDiffuse=(vec_float4*)edgeGeomGetUniformTableByAttribute((EdgeGeomSpuContext*)ctx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
	if(!pDiffuse)
	{
		return;	// nothing to do	
	}

#if 1
	// 8 verts per loop (23 steps/loop):
	const u32 count4	= (ctx->spuConfigInfo.numVertexes + 3) >> 2;
	const u32 count8	= count4 >> 1;
	const u32 count0_7	= count4 & 0x1;

	vec_float4 diffuse0n, diffuse1n, diffuse2n, diffuse3n;
	vec_float4 diffuse4n, diffuse5n, diffuse6n, diffuse7n;

	// load current:
	vec_float4 diffuse0	= pDiffuse[0];
	vec_float4 diffuse1	= pDiffuse[1];
	vec_float4 diffuse2	= pDiffuse[2];
	vec_float4 diffuse3	= pDiffuse[3];
	vec_float4 diffuse4	= pDiffuse[4];
	vec_float4 diffuse5	= pDiffuse[5];
	vec_float4 diffuse6	= pDiffuse[6];
	vec_float4 diffuse7	= pDiffuse[7];
	// process current:
	diffuse0 = spu_sel(diffuse0, vecOne, sel00ff0000);
	diffuse1 = spu_sel(diffuse1, vecOne, sel00ff0000);
	diffuse2 = spu_sel(diffuse2, vecOne, sel00ff0000);
	diffuse3 = spu_sel(diffuse3, vecOne, sel00ff0000);
	diffuse4 = spu_sel(diffuse4, vecOne, sel00ff0000);
	diffuse5 = spu_sel(diffuse5, vecOne, sel00ff0000);
	diffuse6 = spu_sel(diffuse6, vecOne, sel00ff0000);
	diffuse7 = spu_sel(diffuse7, vecOne, sel00ff0000);

	for(u32 i=0; i<count8; i++)
	{
			// fetch next:
			diffuse0n = pDiffuse[0+8];
			diffuse1n = pDiffuse[1+8];
			diffuse2n = pDiffuse[2+8];
			diffuse3n = pDiffuse[3+8];
			diffuse4n = pDiffuse[4+8];
			diffuse5n = pDiffuse[5+8];
			diffuse6n = pDiffuse[6+8];
			diffuse7n = pDiffuse[7+8];
		// store current:
		pDiffuse[0] = diffuse0;
		pDiffuse[1] = diffuse1;
		pDiffuse[2] = diffuse2;
		pDiffuse[3] = diffuse3;
		pDiffuse[4] = diffuse4;
		pDiffuse[5] = diffuse5;
		pDiffuse[6] = diffuse6;
		pDiffuse[7] = diffuse7;
		pDiffuse += 8;
			// process next:
			diffuse0 = spu_sel(diffuse0n, vecOne, sel00ff0000);
			diffuse1 = spu_sel(diffuse1n, vecOne, sel00ff0000);
			diffuse2 = spu_sel(diffuse2n, vecOne, sel00ff0000);
			diffuse3 = spu_sel(diffuse3n, vecOne, sel00ff0000);
			diffuse4 = spu_sel(diffuse4n, vecOne, sel00ff0000);
			diffuse5 = spu_sel(diffuse5n, vecOne, sel00ff0000);
			diffuse6 = spu_sel(diffuse6n, vecOne, sel00ff0000);
			diffuse7 = spu_sel(diffuse7n, vecOne, sel00ff0000);
	} // for(u32 i=0; i<count8; i++)...

	// remaining 4 words (if required):
	if(count0_7)
	{
		pDiffuse[0] = diffuse0;
		pDiffuse[1] = diffuse1;
		pDiffuse[2] = diffuse2;
		pDiffuse[3] = diffuse3;
	}

#else
	// 8 verts per loop (23 steps/loop):
	const u32 count4	= (ctx->spuConfigInfo.numVertexes + 3) >> 2;
	const u32 count8	= count4 >> 1;
	const u32 count0_7	= count4 & 0x1;
	for(u32 i=0; i<count8; i++)
	{
		vec_float4 diffuse0	= pDiffuse[0]; 
		vec_float4 diffuse1	= pDiffuse[1];
		vec_float4 diffuse2	= pDiffuse[2];
		vec_float4 diffuse3	= pDiffuse[3];
		vec_float4 diffuse4	= pDiffuse[4]; 
		vec_float4 diffuse5	= pDiffuse[5];
		vec_float4 diffuse6	= pDiffuse[6];
		vec_float4 diffuse7	= pDiffuse[7];
			diffuse0 = spu_sel(diffuse0, vecOne, sel0100);
			diffuse1 = spu_sel(diffuse1, vecOne, sel0100);
			diffuse2 = spu_sel(diffuse2, vecOne, sel0100);
			diffuse3 = spu_sel(diffuse3, vecOne, sel0100);
			diffuse4 = spu_sel(diffuse4, vecOne, sel0100);
			diffuse5 = spu_sel(diffuse5, vecOne, sel0100);
			diffuse6 = spu_sel(diffuse6, vecOne, sel0100);
			diffuse7 = spu_sel(diffuse7, vecOne, sel0100);
		pDiffuse[0] = diffuse0;
		pDiffuse[1] = diffuse1;
		pDiffuse[2] = diffuse2;
		pDiffuse[3] = diffuse3;
		pDiffuse[4] = diffuse4;
		pDiffuse[5] = diffuse5;
		pDiffuse[6] = diffuse6;
		pDiffuse[7] = diffuse7;
		pDiffuse += 8;
	} // for(u32 i=0; i<count8; i++)...

	// remaining 4 words (if required)
	if(count0_7)
	{
		vec_float4 diffuse0	= pDiffuse[0]; 
		vec_float4 diffuse1	= pDiffuse[1];
		vec_float4 diffuse2	= pDiffuse[2];
		vec_float4 diffuse3	= pDiffuse[3];
			diffuse0 = spu_sel(diffuse0, vecOne, sel0100);
			diffuse1 = spu_sel(diffuse1, vecOne, sel0100);
			diffuse2 = spu_sel(diffuse2, vecOne, sel0100);
			diffuse3 = spu_sel(diffuse3, vecOne, sel0100);
		pDiffuse[0] = diffuse0;
		pDiffuse[1] = diffuse1;
		pDiffuse[2] = diffuse2;
		pDiffuse[3] = diffuse3;
	}
#endif
}// end of ApplyZeroVehicleDamage()...

static inline void Gta5TransformVertexesForCull_DualParaboloid(EdgeGeomSpuContext *ctx)
{
#if ENABLE_EDGE_CULL_DEBUGGING
	uint16_t debugFlags = EDGE_CULL_DEBUG_DEFAULT;

	if (ctx->debugFlags & EDGE_CULL_DEBUG_ENABLED)
	{
		debugFlags = ctx->debugFlags;
	}
#endif // ENABLE_EDGE_CULL_DEBUGGING

	//constants, shuffles
	const qword fOne  = (qword)spu_splats(1.0f);
	const qword fZero = (qword)spu_splats(0.0f);
	const qword fHalf = (qword)spu_splats(0.5f);

	const qword s_signMask = (qword)spu_splats(0x80000000);
	const qword s_BbDdFfHh = (qword){0x02, 0x03, 0x12, 0x13, 0x06, 0x07, 0x16, 0x17, 0x0a, 0x0b, 0x1a, 0x1b, 0x0e, 0x0f, 0x1e, 0x1f};

	//distribute viewport scale and offset values
	vec_char16 pViewportScales = (vec_char16)si_from_ptr(ctx->viewportInfo.viewportScales);
	vec_char16 pViewportOffsets = (vec_char16)si_from_ptr(ctx->viewportInfo.viewportOffsets);
	vec_float4 vpScale = (vec_float4)si_lqd(pViewportScales, 0x00);
	vec_float4 vpOffset = (vec_float4)si_lqd(pViewportOffsets, 0x00);
	const vec_uchar16 s_AAAA = (vec_uchar16)si_ila(0x10203);
	const vec_uchar16 s_BBBB = (vec_uchar16)si_orbi((qword)s_AAAA, 0x04);
	const vec_uchar16 s_CCCC = (vec_uchar16)si_orbi((qword)s_AAAA, 0x08);
	const vec_uchar16 s_DDDD = (vec_uchar16)si_orbi((qword)s_AAAA, 0x0C);
//	const vec_uchar16 s_BbAa = (vec_uchar16)si_rotqbyi((qword)s_AaBb, 8);
	const vec_uchar16 s_CcAa = (vec_uchar16)si_rotqbyi((qword)s_AaCc, 8);
	const vec_uchar16 s_DdBb = (vec_uchar16)si_rotqbyi((qword)s_BbDd, 8);
	const vec_uchar16 s_ABcd = (vec_uchar16)si_rotqbyi((qword)s_cdAB, 8);
	qword viewportScaleX = (qword)spu_shuffle(vpScale, vpScale, s_AAAA);
	qword viewportScaleY = (qword)spu_shuffle(vpScale, vpScale, s_BBBB);
//	qword viewportScaleZ = (qword)spu_shuffle(vpScale, vpScale, s_CCCC);
	qword viewportOffsetX = (qword)spu_shuffle(vpOffset, vpOffset, s_AAAA);
	qword viewportOffsetY = (qword)spu_shuffle(vpOffset, vpOffset, s_BBBB);
//	qword viewportOffsetZ = (qword)spu_shuffle(vpOffset, vpOffset, s_CCCC);

	bool  enable_MSAA_2X = (ctx->viewportInfo.sampleFlavor == CELL_GCM_SURFACE_DIAGONAL_CENTERED_2);
	qword select_MSAA_2X = si_il(0);

#if ENABLE_EDGE_CULL_DEBUGGING
	if ((debugFlags & EDGE_CULL_DEBUG_ALLOW_MSAA_2X) == 0)
	{
		enable_MSAA_2X = false;
	}
#endif // ENABLE_EDGE_CULL_DEBUGGING

	if (enable_MSAA_2X)
	{
		select_MSAA_2X  = si_il(-1);
		viewportOffsetX = (qword)spu_splats(0.25f);
		viewportOffsetY = (qword)spu_splats(0.25f);
	}
	else
#if ENABLE_EDGE_CULL_DEBUGGING
	if (debugFlags & EDGE_CULL_DEBUG_QUANT_XY_IN_W)
#endif // ENABLE_EDGE_CULL_DEBUGGING
	{
		viewportOffsetX = si_fa(viewportOffsetX, fHalf);
		viewportOffsetY = si_fa(viewportOffsetY, fHalf);
	}

//	vec_char16 pScissorArea = (vec_char16)si_from_ptr(ctx->viewportInfo.scissorArea);
//	vec_ushort8 scissorArea = (vec_ushort8)si_lqd(pScissorArea,0x00);
//	vec_ushort8 scissorAreaSum = spu_add(scissorArea, spu_rlmaskqwbyte(scissorArea, -4));
//	//spread, convert, broadcast broadcast broadcast broadcast
//	qword scissorAreaW = si_shufb((qword)scissorArea, (qword)scissorAreaSum, (qword)s_0A0B0c0d);
//	qword scissorAreaF = si_cuflt(scissorAreaW, 0);

//	const vec_float4 depthRange0 = (vec_float4)spu_shuffle(scissorArea, scissorArea, s_CCCC);
//	const vec_float4 depthRange1 = (vec_float4)spu_shuffle(scissorArea, scissorArea, s_DDDD);

//	const vec_uint4 flipDepthRange = spu_cmpgt(depthRange0, depthRange1);
//	const qword frustumMaxZz = (qword)spu_sel(depthRange1, depthRange0, flipDepthRange);
//	const qword frustumMinZz = (qword)spu_sel(depthRange0, depthRange1, flipDepthRange);

//	const qword frustumMax = si_shufb(scissorAreaF, frustumMaxZz, (qword)s_CDa0);
//	const qword frustumMin = si_shufb(scissorAreaF, frustumMinZz, (qword)s_ABa0);

	//load and distribute local to world and view projection matrices
	vec_char16 pLocalToWorldMatrix = (vec_char16)si_from_ptr(&(ctx->localToWorldMatrix.matrixData));
	vec_float4 localToWorld0 = (vec_float4)si_lqd(pLocalToWorldMatrix, 0x00);
	vec_float4 localToWorld1 = (vec_float4)si_lqd(pLocalToWorldMatrix, 0x10);
	vec_float4 localToWorld2 = (vec_float4)si_lqd(pLocalToWorldMatrix, 0x20);
	vec_float4 localToWorld3 =  {0.0f, 0.0f, 0.0f, 1.0f};

	vec_char16 pViewProjectionMatrix = (vec_char16)si_from_ptr(ctx->viewportInfo.viewProjectionMatrix);
	vec_float4 viewProjMat0 = (vec_float4)si_lqd(pViewProjectionMatrix, 0x00);
	vec_float4 viewProjMat1 = (vec_float4)si_lqd(pViewProjectionMatrix, 0x10);
	vec_float4 viewProjMat2 = (vec_float4)si_lqd(pViewProjectionMatrix, 0x20);
//	vec_float4 viewProjMat3 = (vec_float4)si_lqd(pViewProjectionMatrix, 0x30);

	//compute LocalToProj matrix from LocalToWorld and ViewportProjection matrices
	vec_float4 vp00 = spu_shuffle(viewProjMat0, viewProjMat0, s_AAAA);
	vec_float4 vp10 = spu_shuffle(viewProjMat1, viewProjMat1, s_AAAA);
	vec_float4 vp20 = spu_shuffle(viewProjMat2, viewProjMat2, s_AAAA);
//	vec_float4 vp30 = spu_shuffle(viewProjMat3, viewProjMat3, s_AAAA);

	vec_float4 localToProjContrib00 = spu_mul(vp00, localToWorld0);
	vec_float4 localToProjContrib10 = spu_mul(vp10, localToWorld0);
	vec_float4 localToProjContrib20 = spu_mul(vp20, localToWorld0);
//	vec_float4 localToProjContrib30 = spu_mul(vp30, localToWorld0);

	vec_float4 vp01 = spu_shuffle(viewProjMat0, viewProjMat0, s_BBBB);
	vec_float4 vp11 = spu_shuffle(viewProjMat1, viewProjMat1, s_BBBB);
	vec_float4 vp21 = spu_shuffle(viewProjMat2, viewProjMat2, s_BBBB);
//	vec_float4 vp31 = spu_shuffle(viewProjMat3, viewProjMat3, s_BBBB);

	vec_float4 localToProjContrib01 = spu_madd(vp01, localToWorld1, localToProjContrib00);
	vec_float4 localToProjContrib11 = spu_madd(vp11, localToWorld1, localToProjContrib10);
	vec_float4 localToProjContrib21 = spu_madd(vp21, localToWorld1, localToProjContrib20);
//	vec_float4 localToProjContrib31 = spu_madd(vp31, localToWorld1, localToProjContrib30);

	vec_float4 vp02 = spu_shuffle(viewProjMat0, viewProjMat0, s_CCCC);
	vec_float4 vp12 = spu_shuffle(viewProjMat1, viewProjMat1, s_CCCC);
	vec_float4 vp22 = spu_shuffle(viewProjMat2, viewProjMat2, s_CCCC);
//	vec_float4 vp32 = spu_shuffle(viewProjMat3, viewProjMat3, s_CCCC);

	vec_float4 localToProjContrib02 = spu_madd(vp02, localToWorld2, localToProjContrib01);
	vec_float4 localToProjContrib12 = spu_madd(vp12, localToWorld2, localToProjContrib11);
	vec_float4 localToProjContrib22 = spu_madd(vp22, localToWorld2, localToProjContrib21);
//	vec_float4 localToProjContrib32 = spu_madd(vp32, localToWorld2, localToProjContrib31);

	vec_float4 vp03 = spu_shuffle(viewProjMat0, viewProjMat0, s_DDDD);
	vec_float4 vp13 = spu_shuffle(viewProjMat1, viewProjMat1, s_DDDD);
	vec_float4 vp23 = spu_shuffle(viewProjMat2, viewProjMat2, s_DDDD);
//	vec_float4 vp33 = spu_shuffle(viewProjMat3, viewProjMat3, s_DDDD);

	vec_float4 localToProj0 = spu_madd(vp03, localToWorld3, localToProjContrib02);
	vec_float4 localToProj1 = spu_madd(vp13, localToWorld3, localToProjContrib12);
	vec_float4 localToProj2 = spu_madd(vp23, localToWorld3, localToProjContrib22);
//	vec_float4 localToProj3 = spu_madd(vp33, localToWorld3, localToProjContrib32);

	//distribute localToProj
	qword localToProj00 = (qword)spu_shuffle(localToProj0, localToProj0, s_AAAA);
	qword localToProj10 = (qword)spu_shuffle(localToProj1, localToProj1, s_AAAA);
	qword localToProj20 = (qword)spu_shuffle(localToProj2, localToProj2, s_AAAA);
//	qword localToProj30 = (qword)spu_shuffle(localToProj3, localToProj3, s_AAAA);

	qword localToProj01 = (qword)spu_shuffle(localToProj0, localToProj0, s_BBBB);
	qword localToProj11 = (qword)spu_shuffle(localToProj1, localToProj1, s_BBBB);
	qword localToProj21 = (qword)spu_shuffle(localToProj2, localToProj2, s_BBBB);
//	qword localToProj31 = (qword)spu_shuffle(localToProj3, localToProj3, s_BBBB);

	qword localToProj02 = (qword)spu_shuffle(localToProj0, localToProj0, s_CCCC);
	qword localToProj12 = (qword)spu_shuffle(localToProj1, localToProj1, s_CCCC);
	qword localToProj22 = (qword)spu_shuffle(localToProj2, localToProj2, s_CCCC);
//	qword localToProj32 = (qword)spu_shuffle(localToProj3, localToProj3, s_CCCC);

	qword localToProj03 = (qword)spu_shuffle(localToProj0, localToProj0, s_DDDD);
	qword localToProj13 = (qword)spu_shuffle(localToProj1, localToProj1, s_DDDD);
	qword localToProj23 = (qword)spu_shuffle(localToProj2, localToProj2, s_DDDD);
//	qword localToProj33 = (qword)spu_shuffle(localToProj3, localToProj3, s_DDDD);

	const unsigned int numIterations = (ctx->spuConfigInfo.numVertexes + 3) >> 2;

	/* ---------------------------------------------------- */
	/* --- DP CODE CHANGE BEGIN --------------------------- */
	/* ---------------------------------------------------- */

	int adjustVertexPointer = numIterations << 6; // x64 (4 vertices)
	int negativeAdjustVertexPointer = -adjustVertexPointer;

	qword outputOffset    = si_from_int(negativeAdjustVertexPointer);
	qword outputOffsetInc = si_from_int(0x40);

	const bool useFastPath = ctx->spuConfigInfo.flagsAndUniformTableCount & EDGE_GEOM_FLAG_STATIC_GEOMETRY_FAST_PATH;

	// TODO -- is ok to omit the extra uniform table EDGE_GEOM_FLAG_INCLUDES_EXTRA_UNIFORM_TABLE if EDGE_GEOM_FLAG_STATIC_GEOMETRY_FAST_PATH is set?
	const qword pTransVerts  = si_from_ptr(useFastPath ? ctx->positionTable : ctx->uniformTables[ctx->spuConfigInfo.flagsAndUniformTableCount & 0xF]);
	const qword pTransVerts0 = si_a(pTransVerts, si_from_int(adjustVertexPointer));
	const qword pTransVerts1 = si_ai(pTransVerts0, 0x10);
	const qword pTransVerts2 = si_ai(pTransVerts0, 0x20);
	const qword pTransVerts3 = si_ai(pTransVerts0, 0x30);

	qword inputOffset    = si_from_int(negativeAdjustVertexPointer);
	qword inputOffsetInc = si_from_int(0x40);

	const qword pVerts  = si_from_ptr(ctx->positionTable);
	const qword pVerts0 = si_a(pVerts, si_from_int(adjustVertexPointer));
	const qword pVerts1 = si_ai(pVerts0, 0x10);
	const qword pVerts2 = si_ai(pVerts0, 0x20);
	const qword pVerts3 = si_ai(pVerts0, 0x30);

	/* -------------------------------------------------- */
	/* --- DP CODE CHANGE END --------------------------- */
	/* -------------------------------------------------- */

	do
	{
		// load four vertices
		const qword xyz0 = si_lqx(pVerts0, inputOffset);
		const qword xyz1 = si_lqx(pVerts1, inputOffset);
		const qword xyz2 = si_lqx(pVerts2, inputOffset);
		const qword xyz3 = si_lqx(pVerts3, inputOffset);

		inputOffset = si_a(inputOffset, inputOffsetInc);

		// start transpose ..
		qword x0y0x2y2 = si_shufb(xyz0, xyz1, (qword)s_AaCc);
		qword x1y1x3y3 = si_shufb(xyz0, xyz1, (qword)s_BbDd);
		qword z2w2z0w0 = si_shufb(xyz2, xyz3, (qword)s_CcAa);
		qword z3w3z1w1 = si_shufb(xyz2, xyz3, (qword)s_DdBb);

		// finish transpose (ignore w)
		const qword xxxx = si_shufb(x0y0x2y2, z2w2z0w0, (qword)s_ABcd);
		const qword yyyy = si_shufb(x1y1x3y3, z3w3z1w1, (qword)s_ABcd);
		const qword zzzz = si_shufb(z2w2z0w0, x0y0x2y2, (qword)s_cdAB);

		// start transform by localToProj ..
		const qword mMultXx = si_fma(xxxx, localToProj00, localToProj03);
		const qword mMultXy = si_fma(xxxx, localToProj10, localToProj13);
		const qword mMultXz = si_fma(xxxx, localToProj20, localToProj23);

		const qword mMultYx = si_fma(yyyy, localToProj01, mMultXx);
		const qword mMultYy = si_fma(yyyy, localToProj11, mMultXy);
		const qword mMultYz = si_fma(yyyy, localToProj21, mMultXz);

		// finish transform by localToProj
		qword resX = si_fma(zzzz, localToProj02, mMultYx);
		qword resY = si_fma(zzzz, localToProj12, mMultYy);
		qword resZ = si_fma(zzzz, localToProj22, mMultYz);

		qword tmpZ = resZ;

		const qword posMagSqr = si_fma(resX, resX, si_fma(resY, resY, si_fm(resZ, resZ)));
		const qword posMagInvEst = si_frsqest(posMagSqr);
		const qword posMagInv = si_fi(posMagSqr, posMagInvEst); // increase precision

		resX = si_fm(resX, posMagInv);
		resY = si_fm(resY, posMagInv);
		resZ = si_fma(resZ, posMagInv, fOne);

		const qword resZRecipEst = si_frest(resZ);
		const qword resZRecip = si_fi(resZ, resZRecipEst); // increase precision

		resX = si_fm(resX, resZRecip); // (x/len)*(1/(1 + z/len)) = x/(z + len)
		resY = si_fm(resY, resZRecip); // (y/len)*(1/(1 + z/len)) = y/(z + len)

		qword xxxxScaled = si_fma(resX, viewportScaleX, viewportOffsetX);
		qword yyyyScaled = si_fma(resY, viewportScaleY, viewportOffsetY);

		// handle MSAA 2X .. (can be removed if we don't need to support this)
		const qword tmpX = si_fs(xxxxScaled, yyyyScaled); // x - y
		const qword tmpY = si_fa(xxxxScaled, yyyyScaled); // x + y
		xxxxScaled = si_selb(xxxxScaled, tmpX, select_MSAA_2X);
		yyyyScaled = si_selb(yyyyScaled, tmpY, select_MSAA_2X);

		const qword zzzzScaled = si_andi(si_fcgt(fZero, tmpZ), 1);

		qword wwwwScaled;

#if ENABLE_EDGE_CULL_DEBUGGING
		if ((debugFlags & EDGE_CULL_DEBUG_QUANT_XY_IN_W) == 0)
		{
			wwwwScaled = s_signMask;
		}
		else
#endif // ENABLE_EDGE_CULL_DEBUGGING
		{
			const qword xxxxQuantized = si_cflts(xxxxScaled, 0);
			const qword yyyyQuantized = si_cflts(yyyyScaled, 0);

			const qword xyxyxyxyQuantized = si_shufb(xxxxQuantized, yyyyQuantized, s_BbDdFfHh);

			wwwwScaled = si_or(xyxyxyxyQuantized, s_signMask);
		}

		x0y0x2y2 = si_shufb(xxxxScaled, yyyyScaled, (qword)s_AaCc);
		x1y1x3y3 = si_shufb(xxxxScaled, yyyyScaled, (qword)s_BbDd);
		z2w2z0w0 = si_shufb(zzzzScaled, wwwwScaled, (qword)s_CcAa);
		z3w3z1w1 = si_shufb(zzzzScaled, wwwwScaled, (qword)s_DdBb);

		const qword xyz0Out = si_shufb(x0y0x2y2, z2w2z0w0, (qword)s_ABcd);
		const qword xyz1Out = si_shufb(x1y1x3y3, z3w3z1w1, (qword)s_ABcd);
		const qword xyz2Out = si_shufb(z2w2z0w0, x0y0x2y2, (qword)s_cdAB);
		const qword xyz3Out = si_shufb(z3w3z1w1, x1y1x3y3, (qword)s_cdAB);

		si_stqx(xyz0Out, pTransVerts0, outputOffset);
		si_stqx(xyz1Out, pTransVerts1, outputOffset);
		si_stqx(xyz2Out, pTransVerts2, outputOffset);
		si_stqx(xyz3Out, pTransVerts3, outputOffset);

		outputOffset = si_a(outputOffset, outputOffsetInc);

	} while (si_to_int(inputOffset) != 0);
}

void Gta5TransformVertexesForCull(EdgeGeomSpuContext *ctx, void *userData)
{
	const spuGcmStateBaseEdge* state = (const spuGcmStateBaseEdge*)userData;

	if (state->shadowType == EDGE_TYPE_PARABOLOID_REFLECTION)
	{
		Gta5TransformVertexesForCull_DualParaboloid(ctx);
	}
	else
	{
		transformVertexesForCull(ctx);
	}
}

static inline void Gta4PreTransformCallback(CellSpursJobContext2* jobContext, CellSpursEdgeJob *job, EdgeGeomSpuContext* ctx, const spuGcmStateBaseEdge& gcmState)
{
	// apply vehicle damage
	// the cached buffer contains the damage texture
	if(gcmState.IsVehicleGeom)
	{
		if(gcmState.damageTexture && job->Header.sizeCacheDmaList)
		{
			const void* damageTex = NULL;
			if (job->CachedDmaList.ExtraData.Size && job->CachedDmaList.ExtraData.Ea)
			{
				damageTex = jobContext->cacheBuffer[0];
			}

		#if VEHICLEDAMAGE_SPA
			if (damageTex)
			{
				vec_float4* __restrict__ pPos	= (vec_float4*)ctx->positionTable;
				vec_float4* __restrict__ pNorm	= (vec_float4*)ctx->normalTable;
				vec_float4* __restrict__ pDiffuse=(vec_float4*)edgeGeomGetUniformTableByAttribute((EdgeGeomSpuContext*)ctx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
				const u32 numVertices = ctx->spuConfigInfo.numVertexes;
				ApplyVehicleDamage(pPos, pNorm, pDiffuse, numVertices, damageTex, gcmState.damageTextureOffset[0], gcmState.damageTextureOffset[1], gcmState.damageTextureOffset[2], gcmState.damageBoundRadius);
			}
		#else
			if(damageTex)
			{
				ApplyVehicleDamage(ctx, damageTex, (const float*)gcmState.damageTextureOffset, gcmState.damageBoundRadius);
			}
		#endif
		}
		else
		{
			if(gcmState.shadowType == EDGE_SHADOWTYPE_NONE)	// only required when actual COLOR0 stream is used for anything (i.e. not in shadows)
			{
				ApplyZeroVehicleDamage(ctx);
			}
		}
	}// if(gcmState.IsVehicleGeom)...

	const bool shadowWarpGeometry = spuGcmShadowWarpEnabled(gcmState.shadowType);
	const bool tintGeometry	= (gcmState.edgeTintPalettePtr!=NULL);

	// andrzej - shouldn't tintGeometry be false when gcmState.shadowType == EDGE_SHADOWTYPE_NONE?
	//           compare the logic in Gta4PreCompressCallback for computing 'tintedGeometry'

	// Hack: for warp shadow geometry, modify output flavour to use 4 component position
	if(shadowWarpGeometry)
	{
		// patch tint geometry straight away - tinting is not done for shadows:
		if(tintGeometry)
		{
			ctx->spuConfigInfo.outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4;
		}

		switch (ctx->spuConfigInfo.outputVertexFormatId)
		{
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_I16Nc4:
			case EDGE_GEOM_RSX_VERTEX_FORMAT_F32c3_X11Y11Z10N_X11Y11Z10N_X11Y11Z10N:
			{
				ctx->spuConfigInfo.outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4;
			}
			break;
		
			case 0xff:
			if(!ctx->customFormatInfo.outputStreamDesc)
			{
				ctx->spuConfigInfo.outputVertexFormatId = EDGE_GEOM_RSX_VERTEX_FORMAT_F32c4;
			}
			else
			{
				if(	ctx->customFormatInfo.outputStreamDesc->blocks[0].attributeBlock.vertexProgramSlotIndex == 0 &&
					ctx->customFormatInfo.outputStreamDesc->blocks[0].attributeBlock.componentCount			== 3 &&
					ctx->customFormatInfo.outputStreamDesc->blocks[0].attributeBlock.format					== EDGE_GEOM_ATTRIBUTE_FORMAT_F32)
				{
					for(u32 i=1; i<ctx->customFormatInfo.outputStreamDesc->numAttributes; i++)
					{
						ctx->customFormatInfo.outputStreamDesc->blocks[i].attributeBlock.offset += 4;
					}
					ctx->customFormatInfo.outputStreamDesc->stride += 4;

					ctx->customFormatInfo.outputStreamDesc->blocks[0].attributeBlock.componentCount = 4;
					ctx->customFormatInfo.outputStreamDesc->blocks[0].attributeBlock.size			+= 4;
				}
			}
			break;
		default:
			break;
		}
	}
}

static inline void Gta4PostSkiningCallback(CellSpursJobContext2* jobContext, CellSpursEdgeJob *job, EdgeGeomSpuContext* ctx, const spuGcmStateBaseEdge& gcmState, uint32_t matrixCount, void *indexesAndWeights)
{
	const uint8_t skinningFlavor = ctx->spuConfigInfo.indexesFlavorAndSkinningFlavor & 0x0F;
	if(matrixCount == 0	|| skinningFlavor == EDGE_GEOM_SKIN_NONE)
		return;

	if( gcmState.clothMorphData )
	{
		const vec_float4* __restrict__ clothData = NULL;
		if (job->CachedDmaList.ExtraData.Size && job->CachedDmaList.ExtraData.Ea)
		{
			clothData = (vec_float4*)jobContext->cacheBuffer[0];
		}

		if( clothData )
		{
			const uint32_t vertexCount = ctx->spuConfigInfo.numVertexes;
			const uint32_t dbldVertCount = vertexCount >> 1;

			const qword q_AAAA = si_ila( 0x00010203 );
			const qword q_BBBB = si_orbi( q_AAAA, 0x04 );
			const qword q_CCCC = si_orbi( q_AAAA, 0x08 );
			const qword q_DDDD = si_orbi( q_AAAA, 0x0C );

			// const qword q_AaBb = (qword)s_AaBb;
			const qword q_BCAD = (qword)s_BCAD;
			const qword q_CABD = si_shufb( q_BCAD, q_AAAA, q_BCAD );
			// const qword q_DdCc = (qword)s_DdCc;
			// const qword q_CcDd = si_rotqbyi( q_DdCc, 8 );

			const qword q_BBBBBBBBJJJJJJJJ = (qword){0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09};
			const qword q_000A000C000E000G = (qword)s_000A000C000E000G;
			const qword q_000I000K000M000O = si_ai( q_000A000C000E000G, 8 );

			const qword q_sld4 = (qword){0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13};
			const qword q_sld8 = (qword){0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
			const qword q_clothTest = (qword){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
			const qword q_5555 = si_fsmbi( 0x5555 );

			// Prepare untransform
			const qword* mtx = (qword*)&ctx->localToWorldMatrix;
			qword tmp0, tmp1, tmp2, tmp3;

			const qword mtxa = mtx[0];
			const qword mtxb = mtx[1];
			const qword mtxc = mtx[2];


			const qword weightScale = (qword)spu_splats(1.0f / 255.0f);
			const qword OdotFivef = (qword)spu_splats(0.5f);
			const qword OdotOnef = (qword)spu_splats(0.1f);
			const qword minusOneDotO = (qword)spu_splats(-1.0f);
//			const qword oneDotO = (qword)spu_splats(1.0f);
//			const qword OdotO = (qword)spu_splats(0.0f);

			qword pIndexesAndWeights = si_from_ptr( indexesAndWeights );
			qword pClothData = si_from_ptr( clothData );

			qword __restrict__ positions = si_from_ptr( ctx->positionTable );
			qword __restrict__ normals	= si_from_ptr( ctx->normalTable );
			
			// if no normals (shadows, ...), dump it all.
			qword noNormals[2];
			const bool doNormals = ctx->normalTable != NULL;
			normals = doNormals ? normals : si_from_ptr( &noNormals );
			qword normalsOffset = (qword)( doNormals ? spu_splats( 0x20 ) : spu_splats( 0x00 ) );
						

			for (uint32_t i = 0; i < dbldVertCount; i++) 
			{
				// Load two sets of index+weight
				qword indexWeightQuad = si_lqd( pIndexesAndWeights, 0x00 );
				pIndexesAndWeights = si_ai( pIndexesAndWeights, 0x10 );
				
				// extract vert 0 weights and scale to 0..1
				tmp0 = si_shufb( indexWeightQuad, q_AAAA, q_000A000C000E000G );
				tmp1 = si_cuflt( tmp0, 0 );
				qword weights_0 = si_fm( weightScale, tmp1 );
				
				// extract vert 1 weights and scale to 0..1
				tmp0 = si_shufb( indexWeightQuad, q_AAAA, q_000I000K000M000O );
				tmp1 = si_cuflt( tmp0 , 0 );
				qword weights_1 = si_fm( weightScale, tmp1 );

				// test for cloth
				tmp0 = si_shufb( indexWeightQuad, indexWeightQuad, q_BBBBBBBBJJJJJJJJ );
				qword isClothTest = si_ceqb( tmp0, q_clothTest );

				// Extract indices
				tmp0 = si_and( indexWeightQuad, q_5555 );
				qword idx0_0 = si_shli( tmp0, 4 );
				qword idx2_0 = si_shlqbyi ( idx0_0, 4  );
				qword idx1_0 = si_roti ( idx2_0, 16  );
				qword idx0_1 = si_shlqbyi( idx0_0, 8 );
				qword idx2_1 = si_shlqbyi( idx0_0, 12 );
				qword idx1_1 = si_roti( idx2_1, 16 );

				if( spu_extract( isClothTest, 0 )  )
				{
					//Vector3 pos0 = clothData[idx0];
					//Vector3 pos1 = clothData[idx1];
					//Vector3 pos2 = clothData[idx2];
					qword vtx0 = si_lqx ( pClothData, idx0_0  );
					qword vtx1 = si_lqx ( pClothData, idx1_0  );
					qword vtx2 = si_lqx ( pClothData, idx2_0  );

					//Vector3 temp = pos0;
					//pos0.x = mtx[0].Dot(temp);
					//pos0.y = mtx[1].Dot(temp);
					//pos0.z = mtx[2].Dot(temp);
					qword xxxx0 = si_shufb ( vtx0, vtx0, q_AAAA  );
					qword yyyy0 = si_shufb ( vtx0, vtx0, q_BBBB  );
					qword zzzz0 = si_shufb ( vtx0, vtx0, q_CCCC  );
					qword posZ0 = si_fm ( zzzz0, mtxc  );
					qword posY0 = si_fma ( yyyy0, mtxb, posZ0  );
					qword pos0 = si_fma ( xxxx0, mtxa, posY0  );

					//temp = pos1;
					//pos1.x = mtx[0].Dot(temp);
					//pos1.y = mtx[1].Dot(temp);
					//pos1.z = mtx[2].Dot(temp);
					qword xxxx1 = si_shufb ( vtx1, vtx1, q_AAAA  );
					qword yyyy1 = si_shufb ( vtx1, vtx1, q_BBBB  );
					qword zzzz1 = si_shufb ( vtx1, vtx1, q_CCCC  );
					qword posZ1 = si_fm ( zzzz1, mtxc  );
					qword posY1 = si_fma ( yyyy1, mtxb, posZ1  );
					qword pos1 = si_fma ( xxxx1, mtxa, posY1  );

					//temp = pos2;
					//pos2.x = mtx[0].Dot(temp);
					//pos2.y = mtx[1].Dot(temp);
					//pos2.z = mtx[2].Dot(temp);
					qword xxxx2 = si_shufb ( vtx2, vtx2, q_AAAA  );
					qword yyyy2 = si_shufb ( vtx2, vtx2, q_BBBB  );
					qword zzzz2 = si_shufb ( vtx2, vtx2, q_CCCC  );
					qword posZ2 = si_fm ( zzzz2, mtxc  );
					qword posY2 = si_fma ( yyyy2, mtxb, posZ2  );
					qword pos2 = si_fma ( xxxx2, mtxa, posY2  );

					//Vector3 v1 = pos0 - pos1;
					//Vector3 v2 = pos2 - pos1;
					qword p0p1 = si_fs ( pos0, pos1 );
					qword p2p1 = si_fs ( pos2, pos1 );

					//Vector3 n;
					//n.Cross(v1,v2);
					tmp0 = si_shufb ( p2p1, p2p1, q_CABD );
					tmp1 = si_shufb ( p0p1, p0p1, q_BCAD );
					tmp2 = si_fm ( tmp0, tmp1 );
					tmp3 = si_shufb ( p2p1, p2p1, q_BCAD );
					qword tmp4 = si_shufb ( p0p1, p0p1, q_CABD );
					qword cross = si_fnms ( tmp3, tmp4, tmp2 );

					// n.Normalize();
					tmp0 = si_fm ( cross, cross );
					tmp1 = si_shufb ( cross, cross, q_sld4 );
					tmp2 = si_fma ( tmp1, tmp1, tmp0 );
					tmp3 = si_shufb ( cross, cross, q_sld8 );
					tmp4 = si_fma ( tmp3, tmp3, tmp2 );
					qword dotp = si_shufb ( tmp4, tmp4, q_AAAA );

					tmp0 = si_frsqest ( dotp  );
					qword nmag = si_fi ( dotp, tmp0 );

					qword norm = si_fm ( cross, nmag );

					//*positions =	weight2 * pos0 + 
					//				weight1 * pos1 + 
					//				weight0 * pos2 +
					//				((weight3)-0.5f)*0.1f * n;
					qword weight0 = si_shufb ( weights_0, q_AAAA, q_AAAA );
					qword weight1 = si_shufb ( weights_0, q_AAAA, q_BBBB );
					qword weight2 = si_shufb ( weights_0, q_AAAA, q_CCCC );
					tmp0 = si_shufb ( weights_0, q_AAAA, q_DDDD );
					tmp1 = si_fs ( tmp0, OdotFivef );
					qword weight3 = si_fm ( tmp1, OdotOnef );

					tmp0 = si_fm ( weight2, pos0 );
					tmp1 = si_fma ( weight1, pos1, tmp0 );
					tmp2 = si_fma ( weight0, pos2, tmp1 );
					tmp3 = si_fma ( weight3, norm, tmp2 );
					si_stqd ( tmp3, positions, 0x0 );

// 					// *normals = sign(((weight3)-0.5f)*0.1f) * n;
// 					tmp0 = si_fcgt(weight3,OdotO);
// 					tmp1 = si_selb(minusOneDotO,oneDotO,tmp0);
// 					tmp2 = si_fm ( norm, tmp1 );
// 					si_stqd ( tmp2, normals, 0x0 );

					// *normals = -n;
					tmp0 = si_fm ( norm, minusOneDotO );
					si_stqd ( tmp0, normals, 0x0 );

				}

				if( spu_extract( isClothTest, 9 )  )
				{
					//Vector3 pos0 = clothData[idx0];
					//Vector3 pos1 = clothData[idx1];
					//Vector3 pos2 = clothData[idx2];
					qword vtx0 = si_lqx( pClothData, idx0_1 );
					qword vtx1 = si_lqx( pClothData, idx1_1 );
					qword vtx2 = si_lqx( pClothData, idx2_1 );

					//Vector3 temp = pos0;
					//pos0.x = mtx[0].Dot(temp);
					//pos0.y = mtx[1].Dot(temp);
					//pos0.z = mtx[2].Dot(temp);
					qword xxxx0 = si_shufb( vtx0, vtx0, q_AAAA );
					qword yyyy0 = si_shufb( vtx0, vtx0, q_BBBB );
					qword zzzz0 = si_shufb( vtx0, vtx0, q_CCCC );
					qword posZ0 = si_fm( zzzz0, mtxc );
					qword posY0 = si_fma( yyyy0, mtxb, posZ0 );
					qword pos0 = si_fma( xxxx0, mtxa, posY0 );

					//temp = pos1;
					//pos1.x = mtx[0].Dot(temp);
					//pos1.y = mtx[1].Dot(temp);
					//pos1.z = mtx[2].Dot(temp);
					qword xxxx1 = si_shufb( vtx1, vtx1, q_AAAA );
					qword yyyy1 = si_shufb( vtx1, vtx1, q_BBBB );
					qword zzzz1 = si_shufb( vtx1, vtx1, q_CCCC );
					qword posZ1 = si_fm( zzzz1, mtxc );
					qword posY1 = si_fma( yyyy1, mtxb, posZ1 );
					qword pos1 = si_fma( xxxx1, mtxa, posY1 );

					//temp = pos2;
					//pos2.x = mtx[0].Dot(temp);
					//pos2.y = mtx[1].Dot(temp);
					//pos2.z = mtx[2].Dot(temp);
					qword xxxx2 = si_shufb ( vtx2, vtx2, q_AAAA  );
					qword yyyy2 = si_shufb ( vtx2, vtx2, q_BBBB  );
					qword zzzz2 = si_shufb ( vtx2, vtx2, q_CCCC   );
					qword posZ2 = si_fm ( zzzz2, mtxc    );
					qword posY2 = si_fma ( yyyy2, mtxb, posZ2   );
					qword pos2 = si_fma ( xxxx2, mtxa, posY2  );

					//Vector3 v1 = pos0 - pos1;
					//Vector3 v2 = pos2 - pos1;
					qword p0p1 = si_fs( pos0, pos1 );
					qword p2p1 = si_fs( pos2, pos1 );

					//Vector3 n;
					//n.Cross(v1,v2);
					tmp0 = si_shufb( p2p1, p2p1, q_CABD );
					tmp1 = si_shufb( p0p1, p0p1, q_BCAD );
					tmp2 = si_fm( tmp0, tmp1 );
					tmp3 = si_shufb( p2p1, p2p1, q_BCAD );
					qword tmp4 = si_shufb( p0p1, p0p1, q_CABD );
					qword cross = si_fnms( tmp3, tmp4, tmp2 );

					// n.Normalize();
					tmp0 = si_fm( cross, cross );
					tmp1 = si_shufb( cross, cross, q_sld4 );
					tmp2 = si_fma( tmp1, tmp1, tmp0 );
					tmp3 = si_shufb( cross, cross, q_sld8 );
					tmp4 = si_fma( tmp3, tmp3, tmp2 );
					qword dotp = si_shufb( tmp4, tmp4, q_AAAA );

					tmp0 = si_frsqest( dotp );
					qword nmag = si_fi( dotp, tmp0 );

					qword norm = si_fm( cross, nmag );

					//*positions =	weight2 * pos0 + 
					//				weight1 * pos1 + 
					//				weight0 * pos2 +
					//				((weight3)-0.5f)*0.1f * n;
					qword weight0 = si_shufb( weights_1, q_AAAA, q_AAAA );
					qword weight1 = si_shufb( weights_1, q_AAAA, q_BBBB );
					qword weight2 = si_shufb( weights_1, q_AAAA, q_CCCC );
					tmp0 = si_shufb( weights_1, q_AAAA, q_DDDD );
					tmp1 = si_fs( tmp0, OdotFivef );
					qword weight3 = si_fm( tmp1, OdotOnef );

					tmp0 = si_fm( weight2, pos0 );
					tmp1 = si_fma( weight1, pos1, tmp0 );
					tmp2 = si_fma( weight0, pos2, tmp1 );
					tmp3 = si_fma( weight3, norm, tmp2 );
					si_stqd( tmp3, positions, 0x10 );

// 					// *normals = sign(((weight3)-0.5f)*0.1f) * n;
// 					tmp0 = si_fcgt(weight3,OdotO);
// 					tmp1 = si_selb(minusOneDotO,oneDotO,tmp0);
// 					tmp2 = si_fm ( norm, tmp1 );
// 					si_stqd ( tmp2, normals, 0x0 );

					// *normals = -n;
					tmp0 = si_fm( norm, minusOneDotO );
					si_stqd( tmp0, normals, 0x10 );
				}

				positions = si_ai( positions, 0x20 );
				normals = si_a( normals, normalsOffset );

			}
		}
	}
}

//
//
//
//
void Gta4PreCompressCallback(CellSpursJobContext2* jobContext, CellSpursEdgeJob *job, EdgeGeomSpuContext* ctx, const spuGcmStateBaseEdge& gcmState)
{
	const bool tintedGeometry = gcmState.edgeTintPalettePtr && (gcmState.shadowType==EDGE_SHADOWTYPE_NONE);	// no tinting for shadows

	if(tintedGeometry)
	{
		vec_float4* tintPalV4	= (vec_float4*)AllocaAligned(vec_float4, 256, 16);	// 4KB
		u32*		tintPal		= (u32*)&tintPalV4[192];							// dmaSize = 1KB

		const bool bTreeDepack = u32(gcmState.edgeTintFlags)		&  0x1;
		if(job->Header.sizeCacheDmaList)
		{
			tintPal = NULL;
			if(job->CachedDmaList.ExtraData.Size && job->CachedDmaList.ExtraData.Ea)
			{
				tintPal = (u32*)jobContext->cacheBuffer[0];
			}
			Assert(tintPal);
		}

		vec_float4* ptrSpecular = NULL;
		if(bTreeDepack)
		{
			ptrSpecular = (vec_float4*)edgeGeomGetUniformTableByAttribute(ctx, EDGE_GEOM_ATTRIBUTE_ID_SPECULAR);
		}
		else
		{
			s32 specularIndex = edgeGeomAssignUniformTable(ctx, EDGE_GEOM_ATTRIBUTE_ID_SPECULAR);
			Assertf(specularIndex!=-1, "tint: specularIndex=%d, gcmState=0x%p, bTreeDepack=%d.", specularIndex, &gcmState, bTreeDepack);
			ptrSpecular = (vec_float4*)edgeGeomGetUniformTable(ctx, specularIndex);
		}
		Assertf(ptrSpecular, "gcmState=0x%p. bTreeDepack=%d. modelInfoIdx=%d.", &gcmState, bTreeDepack, gcmState.gSpuGta4DebugInfo.gta4ModelInfoIdx);

		// trees want to depack tint from COLOR1.B, everybody else from COLOR0.B:
		vec_float4 *ptrDiffuse = bTreeDepack? ptrSpecular : (vec_float4*)edgeGeomGetUniformTableByAttribute(ctx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
		Assertf(ptrDiffuse, "gcmState=0x%p. bTreeDepack=%d. modelInfoIdx=%d.", &gcmState, bTreeDepack, gcmState.gSpuGta4DebugInfo.gta4ModelInfoIdx);

		
		const u32 numVerts = ctx->spuConfigInfo.numVertexes;

		const vec_float4 vec255_0	= spu_splats(255.0f);
		const vec_float4 vec0_5		= spu_splats(0.5f);
		const vec_float4 vecInv255_0= spu_splats(1.0f/255.0f);

		// s_ABCD_EFGH_IJKL_MNOP
		const vec_uchar16 s_000B_000C_000D_000A = {0x80,0x80,0x80,0x01,  0x80,0x80,0x80,0x02,  0x80,0x80,0x80,0x03,  0x80,0x80,0x80,0x00};
		const vec_uchar16 s_000F_000G_000H_000E = {0x80,0x80,0x80,0x05,  0x80,0x80,0x80,0x06,  0x80,0x80,0x80,0x07,  0x80,0x80,0x80,0x04};
		const vec_uchar16 s_000J_000K_000L_000I = {0x80,0x80,0x80,0x09,  0x80,0x80,0x80,0x0A,  0x80,0x80,0x80,0x0B,  0x80,0x80,0x80,0x08};
		const vec_uchar16 s_000N_000O_000P_000M = {0x80,0x80,0x80,0x0D,  0x80,0x80,0x80,0x0E,  0x80,0x80,0x80,0x0F,  0x80,0x80,0x80,0x0C};

		// In-place tint palette decompression:
		// We read 32 bytes (two float4's) at the start of each iteration and write 128 bytes (eight float4's)
		// The loop runs 32 times.
		// First iteration: Read slots float4 192 and 193, writes 0-7
		// Second: 194 and 195, writes 8-15
		// Third: 196 and 197, writes 16-23
		// : : :
		// Second-to-last iteration: 252 and 253, writes 240-247
		// Last iteration: 254 and 255, writes 248-255
		for(u32 i=0; i<256; i+=8)
		{
			vec_uint4 col4Va = *((vec_uint4*)(&tintPal[i+0]));	// ARGB0, ARGB1, ARGB2, ARGB3
			vec_uint4 col4Vb = *((vec_uint4*)(&tintPal[i+4]));	// ARGB4, ARGB5, ARGB6, ARGB7

			vec_uint4 col0a = spu_shuffle(col4Va, col4Va, s_000B_000C_000D_000A);	// RGBA0
			vec_uint4 col1a = spu_shuffle(col4Va, col4Va, s_000F_000G_000H_000E);	// RGBA1
			vec_uint4 col2a = spu_shuffle(col4Va, col4Va, s_000J_000K_000L_000I);	// RGBA2
			vec_uint4 col3a = spu_shuffle(col4Va, col4Va, s_000N_000O_000P_000M);	// RGBA3
			vec_uint4 col4a = spu_shuffle(col4Vb, col4Vb, s_000B_000C_000D_000A);	// RGBA4
			vec_uint4 col5a = spu_shuffle(col4Vb, col4Vb, s_000F_000G_000H_000E);	// RGBA5
			vec_uint4 col6a = spu_shuffle(col4Vb, col4Vb, s_000J_000K_000L_000I);	// RGBA6
			vec_uint4 col7a = spu_shuffle(col4Vb, col4Vb, s_000N_000O_000P_000M);	// RGBA7

			vec_float4 col0b = spu_convtf(col0a, 0);
			vec_float4 col1b = spu_convtf(col1a, 0);
			vec_float4 col2b = spu_convtf(col2a, 0);
			vec_float4 col3b = spu_convtf(col3a, 0);
			vec_float4 col4b = spu_convtf(col4a, 0);
			vec_float4 col5b = spu_convtf(col5a, 0);
			vec_float4 col6b = spu_convtf(col6a, 0);
			vec_float4 col7b = spu_convtf(col7a, 0);

			vec_float4 col0	= spu_mul(col0b, vecInv255_0);
			vec_float4 col1 = spu_mul(col1b, vecInv255_0);
			vec_float4 col2 = spu_mul(col2b, vecInv255_0);
			vec_float4 col3 = spu_mul(col3b, vecInv255_0);
			vec_float4 col4	= spu_mul(col4b, vecInv255_0);
			vec_float4 col5 = spu_mul(col5b, vecInv255_0);
			vec_float4 col6 = spu_mul(col6b, vecInv255_0);
			vec_float4 col7 = spu_mul(col7b, vecInv255_0);

			tintPalV4[i+0] = col0;
			tintPalV4[i+1] = col1;
			tintPalV4[i+2] = col2;
			tintPalV4[i+3] = col3;
			tintPalV4[i+4] = col4;
			tintPalV4[i+5] = col5;
			tintPalV4[i+6] = col6;
			tintPalV4[i+7] = col7;
		}
		
		const u32 numVerts8		= numVerts & (~0x07);
		const u32 numVerts0_7	= numVerts & ( 0x07);	// remainder of verts (0-7)
		const u32 loopCount		= numVerts8+8;
		for(u32 i=0; i<loopCount; i+=8)
		{
			const bool bLastLoop = (i==numVerts8);

			// extract B channel:
			//u32 colIndex = u32( ptrDiffuse4[i*4+2]*255.0f + 0.5f );
			vec_float4		vecDiffuse0 = ptrDiffuse[i+0];
			vec_float4		vecDiffuse1 = ptrDiffuse[i+1];
			vec_float4		vecDiffuse2 = ptrDiffuse[i+2];
			vec_float4		vecDiffuse3 = ptrDiffuse[i+3];
			vec_float4		vecDiffuse4 = ptrDiffuse[i+4];
			vec_float4		vecDiffuse5 = ptrDiffuse[i+5];
			vec_float4		vecDiffuse6 = ptrDiffuse[i+6];
			vec_float4		vecDiffuse7 = ptrDiffuse[i+7];

			vec_float4		vecDiffuse0a= spu_madd(vecDiffuse0, vec255_0, vec0_5);
			vec_float4		vecDiffuse1a= spu_madd(vecDiffuse1, vec255_0, vec0_5);
			vec_float4		vecDiffuse2a= spu_madd(vecDiffuse2, vec255_0, vec0_5);
			vec_float4		vecDiffuse3a= spu_madd(vecDiffuse3, vec255_0, vec0_5);
			vec_float4		vecDiffuse4a= spu_madd(vecDiffuse4, vec255_0, vec0_5);
			vec_float4		vecDiffuse5a= spu_madd(vecDiffuse5, vec255_0, vec0_5);
			vec_float4		vecDiffuse6a= spu_madd(vecDiffuse6, vec255_0, vec0_5);
			vec_float4		vecDiffuse7a= spu_madd(vecDiffuse7, vec255_0, vec0_5);

			vec_uint4		colIndex0a	= spu_convtu(vecDiffuse0a, 0);
			vec_uint4		colIndex1a	= spu_convtu(vecDiffuse1a, 0);
			vec_uint4		colIndex2a	= spu_convtu(vecDiffuse2a, 0);
			vec_uint4		colIndex3a	= spu_convtu(vecDiffuse3a, 0);
			vec_uint4		colIndex4a	= spu_convtu(vecDiffuse4a, 0);
			vec_uint4		colIndex5a	= spu_convtu(vecDiffuse5a, 0);
			vec_uint4		colIndex6a	= spu_convtu(vecDiffuse6a, 0);
			vec_uint4		colIndex7a	= spu_convtu(vecDiffuse7a, 0);

			vec_uint4		colIndex0 = spu_slqwbyte(colIndex0a, 8);
			vec_uint4		colIndex1 = spu_slqwbyte(colIndex1a, 8);
			vec_uint4		colIndex2 = spu_slqwbyte(colIndex2a, 8);
			vec_uint4		colIndex3 = spu_slqwbyte(colIndex3a, 8);
			vec_uint4		colIndex4 = spu_slqwbyte(colIndex4a, 8);
			vec_uint4		colIndex5 = spu_slqwbyte(colIndex5a, 8);
			vec_uint4		colIndex6 = spu_slqwbyte(colIndex6a, 8);
			vec_uint4		colIndex7 = spu_slqwbyte(colIndex7a, 8);

			//u32 col = tintPal[colIndex];
			vec_float4		col0 = tintPalV4[ si_to_uint((qword)colIndex0) ];	// RGBA
			vec_float4		col1 = tintPalV4[ si_to_uint((qword)colIndex1) ];	// RGBA
			vec_float4		col2 = tintPalV4[ si_to_uint((qword)colIndex2) ];	// RGBA
			vec_float4		col3 = tintPalV4[ si_to_uint((qword)colIndex3) ];	// RGBA
			vec_float4		col4 = tintPalV4[ si_to_uint((qword)colIndex4) ];	// RGBA
			vec_float4		col5 = tintPalV4[ si_to_uint((qword)colIndex5) ];	// RGBA
			vec_float4		col6 = tintPalV4[ si_to_uint((qword)colIndex6) ];	// RGBA
			vec_float4		col7 = tintPalV4[ si_to_uint((qword)colIndex7) ];	// RGBA

			if( Likely(!bLastLoop) )
			{	// standard epilog:
				ptrSpecular[i+0] = col0;
				ptrSpecular[i+1] = col1;
				ptrSpecular[i+2] = col2;
				ptrSpecular[i+3] = col3;
				ptrSpecular[i+4] = col4;
				ptrSpecular[i+5] = col5;
				ptrSpecular[i+6] = col6;
				ptrSpecular[i+7] = col7;
			}
			else
			{	// last loop: store only required data (O-7) for last batch:
				if(numVerts0_7 >= 1)	ptrSpecular[i+0] = col0;
				if(numVerts0_7 >= 2)	ptrSpecular[i+1] = col1;
				if(numVerts0_7 >= 3)	ptrSpecular[i+2] = col2;
				if(numVerts0_7 >= 4)	ptrSpecular[i+3] = col3;
				if(numVerts0_7 >= 5)	ptrSpecular[i+4] = col4;
				if(numVerts0_7 >= 6)	ptrSpecular[i+5] = col5;
				if(numVerts0_7 >= 7)	ptrSpecular[i+6] = col6;
			}
		}

	}// if(tintGeometry)...

}// end of Gta4PreCompressCallback()...
#endif // __SPU

#ifdef WANT_POSTSTALLHOLE_CALLBACK
#define EDGE_JOBS_POSTSTALLHOLE_CALLBACK Gta4EdgeJobsPostStallHoleCallback

void Gta4EdgeJobsPostStallHoleCallback(CellSpursEdgeJob* job, CellGcmContextData* /*ctx*/, const spuGcmStateBase& gcmState, const EdgeGeomPpuConfigInfo* /*lsPpuConfigInfo*/)
{
	// CompileTimeAssert((sizeof(job->CachedDmaList)&0x0f)==0);	// sizeof(job->CachedDmaList)==16 (!!!);

	// make sure there will be only 1 user of cached extra data:
	Assert(((gcmState.damageTexture?1:0) + (gcmState.clothMorphData?1:0) + (gcmState.edgeTintPalettePtr?1:0)) <=1 );

	Assert(job->Header.sizeCacheDmaList == 0);

	if(gcmState.damageTexture)
	{
		// read-only cached buffer contains the damage texture
		// expected texture format is 3 component RGB
		const u32 texSize = GTA_VEHICLE_DAMAGE_TEXTURE_SIZE * GTA_VEHICLE_DAMAGE_TEXTURE_SIZE * 3;	// 48KB

		// read-only cached buffer contains the damage texture:
		Assert(u32(gcmState.damageTexture) >= 256*1024);
		Assert16(u32(gcmState.damageTexture));
		Assert16(texSize);

		job->CachedDmaList.ExtraData.Ea		= (u32)gcmState.damageTexture;
		job->CachedDmaList.ExtraData.Size	= texSize;
		job->Header.sizeCacheDmaList		= 8;
	}
	else if (gcmState.clothMorphData)
	{
#if __SPU
		const u32 morphVertCount = gcmState.clothNumVerts;
#else
		float morphVertCountf;
		vec_stvewx (gcmState.clothParams[0], 4, &morphVertCountf);
		const u32 morphVertCount = (u32)morphVertCountf;
#endif		
		const u32 morphSize = ((morphVertCount+3)&(~3)) * sizeof(__vector4);

		// read-only cached buffer contains the damage texture:
		Assert(u32(gcmState.clothMorphData) >= 256*1024);
		Assert16(u32(gcmState.clothMorphData));
		Assert16(morphSize);

		job->CachedDmaList.ExtraData.Ea		= (u32)gcmState.clothMorphData;
		job->CachedDmaList.ExtraData.Size	= morphSize;
		job->Header.sizeCacheDmaList		= 8;
	}
	else if(gcmState.edgeTintPalettePtr)
	{
		FastAssert(u32(gcmState.edgeTintPalettePtr) >= 256*1024);
		Assert16(u32(gcmState.edgeTintPalettePtr));

		job->CachedDmaList.ExtraData.Ea		= (u32)gcmState.edgeTintPalettePtr;
		job->CachedDmaList.ExtraData.Size	= 256*sizeof(u32);
		job->Header.sizeCacheDmaList		= 8;
	}
	Assert(job->Header.eaHighCache == 0);
}
#endif // WANT_POSTSTALLHOLE_CALLBACK

#endif // HACK_GTA4 && __PS3
