//
// grcore/edge_callbacks_mc4.cpp
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

#if HACK_MC4
#if __SPU
#include <spu_intrinsics.h>

#include "edge/geom/edgegeom.h"

// from edgegeom_masks.cpp
extern const vec_uchar16 s_0A0B0c0d;
extern const vec_uchar16 s_AaBb;
extern const vec_uchar16 s_AaCc;
extern const vec_uchar16 s_ABa0;
extern const vec_uchar16 s_ABCD;
extern const vec_uchar16 s_ABCDEFGHd000MNOP;
extern const vec_uchar16 s_BbDd;
extern const vec_uchar16 s_BCDD;
extern const vec_uchar16 s_CcCc;
extern const vec_uchar16 s_CDa0;
extern const vec_uchar16 s_cdAB;
extern const vec_uchar16 s_DdBb;
extern const vec_uchar16 s_FBDbdf00;

// from edgegeom_cull.cpp
extern void transformVertexesForCull(EdgeGeomSpuContext *ctx);

#define EDGE_GEOM_TRANSFORM_VERTEXES_FOR_CULL_CALLBACK Mc4EnvMapTransformVertexesForCull
#define EDGE_GEOM_PRETRANSFORM_CALLBACK Mc4PreTransformCallback

#define MC4_VEHICLE_DAMAGE_TEXTURE_SIZE_LOG2 6
#define MC4_VEHICLE_DAMAGE_TEXTURE_SIZE (1<<MC4_VEHICLE_DAMAGE_TEXTURE_SIZE_LOG2)

inline vec_float4 vec_sum_all(vec_float4 v)
{
	v = vec_add(v, vec_sld(v, v, 4));
	v = vec_add(v, vec_sld(v, v, 8));
	return v;
}

inline vec_float4 Normalize3(vec_float4 v)
{
	// mask off w
	const vec_float4 fZeroW = { 1, 1, 1, 0 };
	v = spu_mul(v, fZeroW);

	const vec_uint4 minusZeroU = vec_splat_u32(-1);
	vec_float4 minusZero = (vec_float4)vec_sl(minusZeroU, minusZeroU);
	vec_float4 lengthSquared = vec_madd(v, v, minusZero);
	lengthSquared = vec_sum_all(lengthSquared);
	return vec_madd(v, vec_rsqrte(lengthSquared), minusZero);
}

#define _VECTORMATH_SHUF_YZXW (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_W }
#define _VECTORMATH_SHUF_ZXYW (vec_uchar16)(vec_uint4){ _VECTORMATH_SHUF_Z, _VECTORMATH_SHUF_X, _VECTORMATH_SHUF_Y, _VECTORMATH_SHUF_W }

static inline vec_float4 _vmathVfCross( vec_float4 vec0, vec_float4 vec1 )
{
	vec_float4 tmp0, tmp1, tmp2, tmp3, result;
	tmp0 = spu_shuffle( vec0, vec0, _VECTORMATH_SHUF_YZXW );
	tmp1 = spu_shuffle( vec1, vec1, _VECTORMATH_SHUF_ZXYW );
	tmp2 = spu_shuffle( vec0, vec0, _VECTORMATH_SHUF_ZXYW );
	tmp3 = spu_shuffle( vec1, vec1, _VECTORMATH_SHUF_YZXW );
	result = spu_mul( tmp0, tmp1 );
	result = spu_nmsub( tmp2, tmp3, result );
	return result;
}

void ApplyVehicleDamage(EdgeGeomSpuContext* ctx, const spuMatrix44* restMtx, const void* damageTex, vec_float4 dmgParams)
{
	const vec_float4 fZero = spu_splats(0.0f);
	if (vec_all_eq(dmgParams, fZero))
		return;

	vec_float4* __restrict__ positions = (vec_float4*)ctx->positionTable;
	vec_float4* __restrict__ normals = (vec_float4*)ctx->normalTable;
	vec_float4* __restrict__ tangents = (vec_float4*)ctx->tangentTable;
	// Load base address of damage texture
	qword pixels = si_from_uint((uint32_t)damageTex);

	const uint32_t X = 0x00010203;
	const uint32_t Y = 0x04050607;
	const uint32_t Z = 0x08090A0B;
	const uint32_t W = 0x0C0D0E0F;
	const uint32_t W2 = 0x1C1D1E1F;
	const vec_uchar16 xControl = (vec_uchar16)((vec_uint4){ X, X, X, X });
	const vec_uchar16 yControl = (vec_uchar16)((vec_uint4){ Y, Y, Y, Y });
	const vec_uchar16 zControl = (vec_uchar16)((vec_uint4){ Z, Z, Z, Z });
	const vec_uchar16 wControl = (vec_uchar16)((vec_uint4){ W, W, W, W });
	const vec_uchar16 leaveWControl = (vec_uchar16)((vec_uint4){ X, Y, Z, W2 });
	const vec_uchar16 redControl = xControl;
	const vec_uchar16 greenControl = yControl;
	const vec_float4 fWidthMinusOne = spu_splats((float)MC4_VEHICLE_DAMAGE_TEXTURE_SIZE - 1.0f);
	const vec_uint4 uWidthMinusOne = vec_splat_u32(MC4_VEHICLE_DAMAGE_TEXTURE_SIZE - 1);
	const vec_uint4 uLog2Width = vec_splat_u32(MC4_VEHICLE_DAMAGE_TEXTURE_SIZE_LOG2);
	const vec_float4 fZeroPointFive = spu_splats(0.5f);
	const vec_float4 fDmgParamsW = vec_perm(dmgParams, dmgParams, wControl);
	const vec_uchar16 scatterColorControl = { 16, 16, 16, 3, 16, 16, 16, 2, 16, 16, 16, 1, 16, 16, 16, 0 };
	const vec_uint4 uZero = vec_splat_u32(0);
	const vec_uint4 uTwo = vec_splat_u32(2);
	const vec_float4 fOne = spu_splats(1.0f);
	const vec_float4 fOneOverTwoFiftyFive = vec_splats(1.0f/255.0f);
	const vec_float4 fZeroW = { 1, 1, 1, 0 };

	uint32_t count = ctx->spuConfigInfo.numVertexes;
	for (uint32_t i = 0; i < count; ++i)
	{
		vec_float4 pos = positions[i];

		// Transform us back to the rest position
		// (the space where the damage texture was mapped)
		vec_float4 restPos;
		if (Likely(restMtx != NULL))
			restPos = restMtx->Transform(pos);
		else
			restPos = pos;

		// CalcDamageOffset()
		vec_float4 texCoord = spu_madd(Normalize3(restPos), fZeroPointFive, fZeroPointFive);
		vec_float4 u = vec_perm(texCoord, texCoord, xControl);
		vec_float4 v = vec_perm(texCoord, texCoord, zControl);

		// * (width-1)
		u = spu_mul(u, fWidthMinusOne);
		v = spu_mul(v, fWidthMinusOne);
		// static cast from float to uint
		vec_uint4 uU = spu_convtu(u, 0);
		vec_uint4 uV = spu_convtu(v, 0);
		vec_uint4 x0 = vec_min(uU, uWidthMinusOne);
		vec_uint4 y0 = vec_min(uV, uWidthMinusOne);
		vec_uint4 y0Width = vec_sl(y0, uLog2Width);
		vec_uint4 offset0 = spu_add(x0, y0Width);

		// Left shift 2 (4 bytes per pixel)
		offset0 = vec_sl(offset0, uTwo);
		// Load the texel from the given offset
		vec_uint4 u0 = (vec_uint4)si_lqx(pixels, (qword)offset0);
		// Scatter rgba
		u0 = vec_perm(u0, uZero, scatterColorControl);
		// static cast from uint to float
		vec_float4 sample0 = spu_convtf(u0, 0);
		// [0, 255] -> [-1, 1]
		sample0 = spu_mul(sample0, fOneOverTwoFiftyFive);

		// Extract red
		vec_float4 sample0Red = vec_perm(sample0, sample0, redControl);
		// Extract green
		vec_float4 sample0Green = vec_perm(sample0, sample0, greenControl);
		// Modulate
		vec_float4 offsetVal = spu_mul(sample0Red, sample0Green);
		// Perturb position
		vec_float4 offsetVec = Normalize3(spu_sub(dmgParams, restPos));
		vec_float4 bump = spu_mul(offsetVal, fDmgParamsW);
		bump = spu_mul(offsetVec, bump);

		// Add bump
		pos = spu_add(pos, bump);
		//positions[i] = offsetVal; // Put the offset value in w
		// Leave w
		positions[i] = vec_perm(pos, positions[i], leaveWControl);

		if (normals && tangents)
		{
			vec_float4 normal = normals[i];
			vec_float4 tangent = tangents[i];

			// rageComputeBinormal
			vec_float4 tangentW = vec_perm(tangent, tangent, wControl);
			normal = spu_mul(normal, fZeroW);
			tangent = spu_mul(tangent, fZeroW);
			vec_float4 binorm = _vmathVfCross(tangent, normal);
			binorm = spu_mul(binorm, tangentW); // w component contains the sign of the binormal

			u = spu_add(u, fOne); // 1 texel offset
			v = spu_add(v, fOne); // 1 texel offset
			uU = spu_convtu(u, 0);
			uV = spu_convtu(v, 0);
			vec_uint4 xX = vec_min(uU, uWidthMinusOne);
			vec_uint4 yY = vec_min(uV, uWidthMinusOne);
			vec_uint4 offsetX = spu_add(xX, y0Width);
			vec_uint4 yYWidth = vec_sl(yY, uLog2Width);
			vec_uint4 offsetY = spu_add(x0, yYWidth);

			// Left shift 2 (4 bytes per pixel)
			offsetX = vec_sl(offsetX, uTwo);
			offsetY = vec_sl(offsetY, uTwo);
			// Load the texels from the given offsets
			vec_uint4 uX = (vec_uint4)si_lqx(pixels, (qword)offsetX);
			vec_uint4 uY = (vec_uint4)si_lqx(pixels, (qword)offsetY);

			// Scatter rgba
			uX = vec_perm(uX, uZero, scatterColorControl);
			uY = vec_perm(uY, uZero, scatterColorControl);
			// static cast from uint to float
			vec_float4 sampleX = spu_convtf(uX, 0);
			vec_float4 sampleY = spu_convtf(uY, 0);
			// [0, 255] -> [-1, 1]
			sampleX = spu_mul(sampleX, fOneOverTwoFiftyFive);
			sampleY = spu_mul(sampleY, fOneOverTwoFiftyFive);

			// Extract red
			vec_float4 sampleXRed = vec_perm(sampleX, sampleX, redControl);
			vec_float4 sampleYRed = vec_perm(sampleY, sampleY, redControl);
			// Extract green
			vec_float4 sampleXGreen = vec_perm(sampleX, sampleX, greenControl);
			vec_float4 sampleYGreen = vec_perm(sampleY, sampleY, greenControl);
			// Modulate
			vec_float4 nX = spu_mul(sampleXRed, sampleXGreen);
			vec_float4 nY = spu_mul(sampleYRed, sampleYGreen);
			// Perturb normal
			normal = spu_sub(normal, spu_mul(tangent, spu_sub(nX, offsetVal)));
			normal = spu_sub(normal, spu_mul(binorm, spu_sub(nY, offsetVal)));
			normal = Normalize3(normal);
			// Leave w
			normals[i] = vec_perm(normal, normals[i], leaveWControl);
		}
	}
}

void ApplyAmbientVehicleDamage(EdgeGeomSpuContext* ctx, vec_float4 dmgParams)
{
	vec_float4* __restrict__ positions = (vec_float4*)ctx->positionTable;
	vec_float4* __restrict__ normals = (vec_float4*)ctx->normalTable;
	vec_float4* __restrict__ tangents = (vec_float4*)ctx->tangentTable;

	vec_float4* __restrict__ speculars = NULL;
	for (uint32_t i = 0; i < 16; ++i)
	{
		if (ctx->uniformTableToAttributeIdMapping[i] == EDGE_GEOM_ATTRIBUTE_ID_SPECULAR)
		{
			speculars = (vec_float4*)edgeGeomGetUniformTable(ctx, i);
			break;
		}
	}
	if (Unlikely(!speculars))
		return;

	const vec_float4 fZero = spu_splats(0.0f);
	if (vec_all_eq(dmgParams, fZero))
		return;

	const uint32_t X = 0x00010203;
	const uint32_t Y = 0x04050607;
	const uint32_t Z = 0x08090A0B;
	const uint32_t W = 0x0C0D0E0F;
	const uint32_t W2 = 0x1C1D1E1F;
	const vec_uchar16 xControl = (vec_uchar16)((vec_uint4){ X, X, X, X });
	const vec_uchar16 yControl = (vec_uchar16)((vec_uint4){ Y, Y, Y, Y });
	const vec_uchar16 zControl = (vec_uchar16)((vec_uint4){ Z, Z, Z, Z });
	const vec_uchar16 wControl = (vec_uchar16)((vec_uint4){ W, W, W, W });
	const vec_uchar16 leaveWControl = (vec_uchar16)((vec_uint4){ X, Y, Z, W2 });
	const vec_float4 fZeroW = { 1, 1, 1, 0 };
	const vec_uint4 uZero = vec_splat_u32(0);
	const vec_float4 fOne = spu_splats(1.0f);
	const vec_uint4 uOne = vec_splat_u32(1);
	const vec_float4 MAX_DAMAGE_AMOUNT = spu_splats(0.5f);
	const vec_float4 fNegativeOne = spu_splats(-1.0f);
	const vec_float4 fScaleDownAndMaskSpecular = { 1.0f/255.0f, 1.0f/255.0f, 1.0f/255.0f, 0.0f };
	const vec_float4 fScaleUpSpecular = spu_splats(255.0f);

	uint32_t count = ctx->spuConfigInfo.numVertexes;

	for (uint32_t i = 0; i < count; ++i)
	{
		vec_float4 pos = positions[i];
		vec_float4 specular = speculars[i];

		specular = spu_mul(specular, fScaleDownAndMaskSpecular);

		// Extract samples
		vec_float4 sample0 = vec_perm(specular, specular, xControl);

		// Figure out quadrant of car (X-Z plane) from position
		// step()
		vec_int4 sQuadrant = vec_sel((vec_int4)uZero, (vec_int4)uOne, vec_cmpge(fZero, pos));
		vec_int4 sQuadrantX = vec_perm(sQuadrant, sQuadrant, xControl);
		vec_int4 sQuadrantZ = vec_perm(sQuadrant, sQuadrant, zControl);
		// idx = quadrant.x * 2 + quadrant.z
		vec_int4 dmgParamsIdx = spu_add(vec_sl(sQuadrantX, uOne), sQuadrantZ);
		vec_float4 dmgVal = spu_splats(spu_extract(dmgParams, spu_extract(dmgParamsIdx, 0)));

		// saturate()
		dmgVal = vec_min(dmgVal, fOne);
		dmgVal = vec_max(dmgVal, fZero);

		// Send the damage value to the shader for scratches
		speculars[i] = spu_mul(dmgVal, fScaleUpSpecular);

		//float3 offsetVec = pos == 0.0f ? 0.0f : (normalize(-pos) * MAX_DAMAGE_AMOUNT);
		vec_float4 offsetVec = Normalize3(spu_mul(pos, fNegativeOne));
		offsetVec = spu_mul(offsetVec, MAX_DAMAGE_AMOUNT);

		// Calculate bump amount
		vec_float4 bump = spu_mul(sample0, dmgVal);
		bump = spu_mul(offsetVec, bump);
		// Perturb position
		pos = spu_add(pos, bump);

		// Leave w
		positions[i] = vec_perm(pos, positions[i], leaveWControl);

		if (normals && tangents)
		{
			vec_float4 normal = normals[i];
			vec_float4 tangent = tangents[i];

			// rageComputeBinormal
			vec_float4 tangentW = vec_perm(tangent, tangent, wControl);
			normal = spu_mul(normal, fZeroW);
			tangent = spu_mul(tangent, fZeroW);
			vec_float4 binorm = _vmathVfCross(tangent, normal);
			binorm = spu_mul(binorm, tangentW); // w component contains the sign of the binormal

			vec_float4 sampleX = vec_perm(specular, specular, yControl);
			vec_float4 sampleY = vec_perm(specular, specular, zControl);

			// Perturb normal
			normal = spu_sub(normal, spu_mul(tangent, spu_sub(sampleX, sample0)));
			normal = spu_sub(normal, spu_mul(binorm, spu_sub(sampleY, sample0)));
			normal = Normalize3(normal);

			// Leave w
			normals[i] = vec_perm(normal, normals[i], leaveWControl);
		}
	}
}

void ApplyReflectionTransform(EdgeGeomSpuContext* ctx, vec_float4 gRoadCarWorldHeight)
{
	vec_float4* __restrict__ positions = (vec_float4*)ctx->positionTable;

	vec_float4* __restrict__ diffuses = NULL;
	for (uint32_t i = 0; i < 16; ++i)
	{
		if (ctx->uniformTableToAttributeIdMapping[i] == EDGE_GEOM_ATTRIBUTE_ID_COLOR)
		{
			diffuses = (vec_float4*)edgeGeomGetUniformTable(ctx, i);
			break;
		}
	}
	if (Unlikely(!diffuses))
		return;

	const uint32_t X = 0x00010203;
	const uint32_t Y = 0x04050607;
	const uint32_t Z = 0x08090A0B;
	const uint32_t W = 0x0C0D0E0F;
	const uint32_t X2 = 0x10111213;
	const uint32_t Z2 = 0x18191A1B;
	const uint32_t W2 = 0x1C1D1E1F;
	const vec_uchar16 xControl = (vec_uchar16)((vec_uint4){ X, X, X, X });
	const vec_uchar16 yControl = (vec_uchar16)((vec_uint4){ Y, Y, Y, Y });
	const vec_uchar16 zControl = (vec_uchar16)((vec_uint4){ Z, Z, Z, Z });
	const vec_uchar16 wControl = (vec_uchar16)((vec_uint4){ W, W, W, W });
	const vec_uchar16 leaveXZWControl = (vec_uchar16)((vec_uint4){ X2, Y, Z2, W2 });

	const vec_float4 fScaleDownDiffuse = spu_splats(1.0f/255.0f);
	// MUST Match the values in "mc4\tools\src\mcApplyLODTexturesToLODMeshes\rageLevelLODMaya.cpp"
	const vec_float4 LOWEST_POINT_THE_THE_WORLD_PLUS_ONE = spu_splats(-29.0f);
	const vec_float4 fThirtyTwo = spu_splats(32.0f);
	const vec_float4 fThirtyTwo_fScaleDownDiffuse = spu_mul(fThirtyTwo, fScaleDownDiffuse);
	const vec_float4 fZero = spu_splats(0.0f);
	const vec_float4 fOne = spu_splats(1.0f);

	uint32_t count = ctx->spuConfigInfo.numVertexes;

	for (uint32_t i = 0; i < count; ++i)
	{
		vec_float4 pos = positions[i];
		vec_float4 diffuse = diffuses[i];

		vec_float4 diffuseRed = vec_perm(diffuse, diffuse, xControl);
		vec_float4 diffuseGreen = vec_perm(diffuse, diffuse, yControl);
		vec_float4 diffuseBlue = vec_perm(diffuse, diffuse, zControl);
		vec_float4 diffuseAlpha = vec_perm(diffuse, diffuse, wControl);

		vec_float4 groundPlanesX = spu_madd(diffuseGreen, fScaleDownDiffuse, diffuseRed);
		groundPlanesX = spu_add(groundPlanesX, LOWEST_POINT_THE_THE_WORLD_PLUS_ONE);
		vec_float4 groundPlanesY = spu_madd(diffuseBlue, fThirtyTwo_fScaleDownDiffuse, groundPlanesX);
		vec_float4 groundPlanesZ = spu_madd(diffuseAlpha, fThirtyTwo_fScaleDownDiffuse, groundPlanesY);

		// need to lerp between them, based on car height (or camera height?)
		vec_float4 groundHeight;
		vec_uint4 cmpMask = spu_cmpgt(gRoadCarWorldHeight, groundPlanesZ);
		if (si_to_uint((qword)cmpMask))
		{
			groundHeight = groundPlanesZ;
		}
		else
		{
			cmpMask = spu_cmpgt(gRoadCarWorldHeight, groundPlanesY);
			vec_float4 rangeX = spu_sel(groundPlanesX, groundPlanesY, cmpMask);
			vec_float4 rangeY = spu_sel(groundPlanesY, groundPlanesZ, cmpMask);
			groundHeight = spu_sub(gRoadCarWorldHeight, rangeX);
			groundHeight = spu_mul(groundHeight, spu_frest(spu_sub(rangeY, rangeX)));
			// saturate
			groundHeight = vec_min(groundHeight, fOne);
			groundHeight = vec_max(groundHeight, fZero);
			// lerp
			groundHeight = spu_madd(spu_sub(rangeY, rangeX), groundHeight, rangeX);
		}

		vec_float4 posY = vec_perm(pos, pos, yControl);
		vec_float4 reflectionY = spu_sub(posY, groundHeight);
		reflectionY = vec_max(fZero, reflectionY); // clamp to the ground plane
		reflectionY = spu_sub(groundHeight, reflectionY);

		// Insert the new position.y
		positions[i] = vec_perm(reflectionY, pos, leaveXZWControl);
	}
}

static inline void Mc4EnvMapTransformVertexesForCull(EdgeGeomSpuContext *ctx, void* userData)
{
	CellSpursEdgeJob* job = (CellSpursEdgeJob*)userData;
	if (job->TransformType != 1)
	{
		transformVertexesForCull(ctx);
		return;
	}

	qword dpMapBasisMtx_0_0 = (qword)spu_splats(job->TransformParam);

	//constants, shuffles
	const qword fOne = (qword)spu_splats(1.0f);
	const qword floatZero = (qword)spu_splats(0.0f);

	const qword dpMapFarClipRecip = (qword)spu_splats(1.0f/2000.0f);
	const qword fZeroPointFive = (qword)spu_splats(0.5f);
	const qword fNegOne = (qword)spu_splats(-1.0f);
	const qword fZeroPointSevenFive = (qword)spu_splats(0.75f);

	//distribute viewport scale and offset values
	vec_char16 pViewportScales = (vec_char16)si_from_ptr(ctx->viewportInfo.viewportScales);
	vec_char16 pViewportOffsets = (vec_char16)si_from_ptr(ctx->viewportInfo.viewportOffsets);
	vec_float4 vpScale = (vec_float4)si_lqd(pViewportScales, 0x00);
	vec_float4 vpOffset = (vec_float4)si_lqd(pViewportOffsets, 0x00);
	const vec_uchar16 s_AAAA = (vec_uchar16)si_ila(0x10203);
	const vec_uchar16 s_BBBB = (vec_uchar16)si_orbi((qword)s_AAAA, 0x04);
	const vec_uchar16 s_CCCC = (vec_uchar16)si_orbi((qword)s_AAAA, 0x08);
	const vec_uchar16 s_DDDD = (vec_uchar16)si_orbi((qword)s_AAAA, 0x0C);
	const vec_uchar16 s_BbAa = (vec_uchar16)si_rotqbyi((qword)s_AaBb, 8);
	const vec_uchar16 s_CcAa = (vec_uchar16)si_rotqbyi((qword)s_AaCc, 8);
	const vec_uchar16 s_DdBb = (vec_uchar16)si_rotqbyi((qword)s_BbDd, 8);
	const vec_uchar16 s_ABcd = (vec_uchar16)si_rotqbyi((qword)s_cdAB, 8);
	qword viewportScaleX = (qword)spu_shuffle(vpScale, vpScale, s_AAAA);
	qword viewportScaleY = (qword)spu_shuffle(vpScale, vpScale, s_BBBB);
	qword viewportScaleZ = (qword)spu_shuffle(vpScale, vpScale, s_CCCC);
	qword viewportOffsetX = (qword)spu_shuffle(vpOffset, vpOffset, s_AAAA);
	qword viewportOffsetY = (qword)spu_shuffle(vpOffset, vpOffset, s_BBBB);
	qword viewportOffsetZ = (qword)spu_shuffle(vpOffset, vpOffset, s_CCCC);

	uint8_t vpSampleFlavor = ctx->viewportInfo.sampleFlavor;

	vec_char16 pScissorArea = (vec_char16)si_from_ptr(ctx->viewportInfo.scissorArea);
	vec_ushort8 scissorArea = (vec_ushort8)si_lqd(pScissorArea,0x00);
	vec_ushort8 scissorAreaSum = spu_add(scissorArea, spu_rlmaskqwbyte(scissorArea, -4));
	//spread, convert, broadcast broadcast broadcast broadcast
	qword scissorAreaW = si_shufb((qword)scissorArea, (qword)scissorAreaSum, (qword)s_0A0B0c0d);
	qword scissorAreaF = si_cuflt(scissorAreaW, 0);

	const vec_float4 depthRange0 = (vec_float4)spu_shuffle(scissorArea, scissorArea, s_CCCC);
	const vec_float4 depthRange1 = (vec_float4)spu_shuffle(scissorArea, scissorArea, s_DDDD);

	const vec_uint4 flipDepthRange = spu_cmpgt(depthRange0, depthRange1);
	const qword frustumMaxZz = (qword)spu_sel(depthRange1, depthRange0, flipDepthRange);
	const qword frustumMinZz = (qword)spu_sel(depthRange0, depthRange1, flipDepthRange);

	const qword frustumMax = si_shufb(scissorAreaF, frustumMaxZz, (qword)s_CDa0);
	const qword frustumMin = si_shufb(scissorAreaF, frustumMinZz, (qword)s_ABa0);

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
	vec_float4 viewProjMat3 = (vec_float4)si_lqd(pViewProjectionMatrix, 0x30);

	//compute LocalToProj matrix from LocalToWorld and ViewportProjection matrices
	vec_float4 vp00 = spu_shuffle(viewProjMat0, viewProjMat0, s_AAAA);
	vec_float4 vp10 = spu_shuffle(viewProjMat1, viewProjMat1, s_AAAA);
	vec_float4 vp20 = spu_shuffle(viewProjMat2, viewProjMat2, s_AAAA);
	vec_float4 vp30 = spu_shuffle(viewProjMat3, viewProjMat3, s_AAAA);

	vec_float4 localToProjContrib00 = spu_mul(vp00, localToWorld0);
	vec_float4 localToProjContrib10 = spu_mul(vp10, localToWorld0);
	vec_float4 localToProjContrib20 = spu_mul(vp20, localToWorld0);
	vec_float4 localToProjContrib30 = spu_mul(vp30, localToWorld0);

	vec_float4 vp01 = spu_shuffle(viewProjMat0, viewProjMat0, s_BBBB);
	vec_float4 vp11 = spu_shuffle(viewProjMat1, viewProjMat1, s_BBBB);
	vec_float4 vp21 = spu_shuffle(viewProjMat2, viewProjMat2, s_BBBB);
	vec_float4 vp31 = spu_shuffle(viewProjMat3, viewProjMat3, s_BBBB);

	vec_float4 localToProjContrib01 = spu_madd(vp01, localToWorld1, localToProjContrib00);
	vec_float4 localToProjContrib11 = spu_madd(vp11, localToWorld1, localToProjContrib10);
	vec_float4 localToProjContrib21 = spu_madd(vp21, localToWorld1, localToProjContrib20);
	vec_float4 localToProjContrib31 = spu_madd(vp31, localToWorld1, localToProjContrib30);

	vec_float4 vp02 = spu_shuffle(viewProjMat0, viewProjMat0, s_CCCC);
	vec_float4 vp12 = spu_shuffle(viewProjMat1, viewProjMat1, s_CCCC);
	vec_float4 vp22 = spu_shuffle(viewProjMat2, viewProjMat2, s_CCCC);
	vec_float4 vp32 = spu_shuffle(viewProjMat3, viewProjMat3, s_CCCC);

	vec_float4 localToProjContrib02 = spu_madd(vp02, localToWorld2, localToProjContrib01);
	vec_float4 localToProjContrib12 = spu_madd(vp12, localToWorld2, localToProjContrib11);
	vec_float4 localToProjContrib22 = spu_madd(vp22, localToWorld2, localToProjContrib21);
	vec_float4 localToProjContrib32 = spu_madd(vp32, localToWorld2, localToProjContrib31);

	vec_float4 vp03 = spu_shuffle(viewProjMat0, viewProjMat0, s_DDDD);
	vec_float4 vp13 = spu_shuffle(viewProjMat1, viewProjMat1, s_DDDD);
	vec_float4 vp23 = spu_shuffle(viewProjMat2, viewProjMat2, s_DDDD);
	vec_float4 vp33 = spu_shuffle(viewProjMat3, viewProjMat3, s_DDDD);

	vec_float4 localToProj0 = spu_madd(vp03, localToWorld3, localToProjContrib02);
	vec_float4 localToProj1 = spu_madd(vp13, localToWorld3, localToProjContrib12);
	vec_float4 localToProj2 = spu_madd(vp23, localToWorld3, localToProjContrib22);
	vec_float4 localToProj3 = spu_madd(vp33, localToWorld3, localToProjContrib32);

	//distribute localToProj
	qword localToProj00 = (qword)spu_shuffle(localToProj0, localToProj0, s_AAAA);
	qword localToProj10 = (qword)spu_shuffle(localToProj1, localToProj1, s_AAAA);
	qword localToProj20 = (qword)spu_shuffle(localToProj2, localToProj2, s_AAAA);
	qword localToProj30 = (qword)spu_shuffle(localToProj3, localToProj3, s_AAAA);

	qword localToProj01 = (qword)spu_shuffle(localToProj0, localToProj0, s_BBBB);
	qword localToProj11 = (qword)spu_shuffle(localToProj1, localToProj1, s_BBBB);
	qword localToProj21 = (qword)spu_shuffle(localToProj2, localToProj2, s_BBBB);
	qword localToProj31 = (qword)spu_shuffle(localToProj3, localToProj3, s_BBBB);

	qword localToProj02 = (qword)spu_shuffle(localToProj0, localToProj0, s_CCCC);
	qword localToProj12 = (qword)spu_shuffle(localToProj1, localToProj1, s_CCCC);
	qword localToProj22 = (qword)spu_shuffle(localToProj2, localToProj2, s_CCCC);
	qword localToProj32 = (qword)spu_shuffle(localToProj3, localToProj3, s_CCCC);

	qword localToProj03 = (qword)spu_shuffle(localToProj0, localToProj0, s_DDDD);
	qword localToProj13 = (qword)spu_shuffle(localToProj1, localToProj1, s_DDDD);
	qword localToProj23 = (qword)spu_shuffle(localToProj2, localToProj2, s_DDDD);
	qword localToProj33 = (qword)spu_shuffle(localToProj3, localToProj3, s_DDDD);

	unsigned int numIterations = (ctx->spuConfigInfo.numVertexes + 3) >> 2;

	int adjustVertexPointer = numIterations << 6;
	int negativeAdjustVertexPointer = -adjustVertexPointer;

	qword outputOffset = si_from_int(negativeAdjustVertexPointer);
	qword outputOffsetInc = (qword)
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x40
	};

	bool useFastPath = ctx->spuConfigInfo.flagsAndUniformTableCount & EDGE_GEOM_FLAG_STATIC_GEOMETRY_FAST_PATH;
	vec_uint4 pTransVerts = useFastPath ? (vec_uint4)si_from_ptr(ctx->positionTable)
		: (vec_uint4)si_from_ptr(ctx->uniformTables[ctx->spuConfigInfo.flagsAndUniformTableCount & 0xF]);
	qword pTransVerts0 = (qword)spu_add(pTransVerts,adjustVertexPointer);
	qword pTransVerts1 = si_ai(pTransVerts0, 0x10);
	qword pTransVerts2 = si_ai(pTransVerts0, 0x20);
	qword pTransVerts3 = si_ai(pTransVerts0, 0x30);

	adjustVertexPointer += 0x80;
	negativeAdjustVertexPointer = -adjustVertexPointer;

	qword pVerts0 = (qword)spu_add((vec_uint4)si_from_ptr(ctx->positionTable), adjustVertexPointer);
	qword pVerts1 = si_ai(pVerts0, 0x10);
	qword pVerts2 = si_ai(pVerts0, 0x20);
	qword pVerts3 = si_ai(pVerts0, 0x30);

	qword inputOffset = si_from_int(negativeAdjustVertexPointer - 0x40);
	qword inputOffsetInc = si_from_int(0x40);

	const qword signMask = (qword)spu_splats(0x80000000);

	const qword s_AbCdEfGh = {0x00, 0x01, 0x12, 0x13, 0x04, 0x05, 0x16, 0x17, 0x08, 0x09, 0x1a, 0x1b, 0x0c, 0x0d, 0x1e, 0x1f};

	unsigned int sampleFlavor = 0;
	switch (vpSampleFlavor)
	{
	case CELL_GCM_SURFACE_CENTER_1:				sampleFlavor = 0; break;
	case CELL_GCM_SURFACE_DIAGONAL_CENTERED_2:	sampleFlavor = 1; break;
	case CELL_GCM_SURFACE_SQUARE_CENTERED_4:	sampleFlavor = 1; break;
	case CELL_GCM_SURFACE_SQUARE_ROTATED_4:		sampleFlavor = 2; break;
	}

	const qword sampleFlavorBC = (qword)spu_splats(sampleFlavor);
	qword sampleMask = (qword)spu_splats(-1);
	sampleMask = si_selb(sampleMask, (qword)spu_splats(-4), si_ceqi(sampleFlavorBC, 0));
	sampleMask = si_selb(sampleMask, (qword)spu_splats(-2), si_ceqi(sampleFlavorBC, 1));

	// predeclare variables
	qword resW = (qword)spu_splats(0);
	qword zzzz = (qword)spu_splats(0);
	qword mMultYw = (qword)spu_splats(0);
	qword z2w2z0w0 = (qword)spu_splats(0);
	qword zzzzScaled = (qword)spu_splats(0);
	qword resWcopy = (qword)spu_splats(0);
	qword z3w3z1w1 = (qword)spu_splats(0);
	qword mMultYx = (qword)spu_splats(0);
	qword yyyy = (qword)spu_splats(0);

	qword mMultXx = (qword)spu_splats(0);
	qword mMultYy = (qword)spu_splats(0);
	qword mMultXy = (qword)spu_splats(0);
	qword xyz0 = (qword)spu_splats(0);
	qword xxxxScaled = (qword)spu_splats(0);
	qword xxxxUnitSigned = (qword)spu_splats(0);
	qword xyz2Trans = (qword)spu_splats(0);
	qword x0y0x2y2 = (qword)spu_splats(0);
	qword mMultYz = (qword)spu_splats(0);
	qword mMultXz = (qword)spu_splats(0);
	qword xyz3Trans = (qword)spu_splats(0);
	qword x1y1x3y3 = (qword)spu_splats(0);
	qword yyyyScaled = (qword)spu_splats(0);
	qword yyyyUnitSigned = (qword)spu_splats(0);
	qword tmp0 = (qword)spu_splats(0);
	qword zzzzUnit = (qword)spu_splats(0);
	qword zzzzUnitSigned = (qword)spu_splats(0);
	qword xyz2 = (qword)spu_splats(0);
	qword over2 = (qword)spu_splats(0);
	qword xyz3 = (qword)spu_splats(0);
	qword over3 = (qword)spu_splats(0);
	qword xyz0Trans = (qword)spu_splats(0);
	qword recipEstW = (qword)spu_splats(0);
	qword xyz1Trans = (qword)spu_splats(0);
	qword under2 = (qword)spu_splats(0);
	qword xyz1 = (qword)spu_splats(0);
	qword under3 = (qword)spu_splats(0);
	qword over0 = (qword)spu_splats(0);
	qword frustumTest2Tmp0 = (qword)spu_splats(0);
	qword under1 = (qword)spu_splats(0);
	qword yyxx23 = (qword)spu_splats(0);
	qword under0 = (qword)spu_splats(0);
	qword frustumTest3Tmp0 = (qword)spu_splats(0);
	qword over1 = (qword)spu_splats(0);
	qword zzzz23 = (qword)spu_splats(0);
	qword tmp1 = (qword)spu_splats(0);
	qword xxyy01 = (qword)spu_splats(0);
	qword xxxxAsSigned = (qword)spu_splats(0);
	qword frustumTest0Tmp0 = (qword)spu_splats(0);
	qword yyyyAsSigned = (qword)spu_splats(0);
	qword frustumTest2 = (qword)spu_splats(0);
	qword signW = (qword)spu_splats(0);
	qword frustumTest3 = (qword)spu_splats(0);
	qword resY = (qword)spu_splats(0);
	qword xxxx = (qword)spu_splats(0);
	qword resX = (qword)spu_splats(0);
	qword frustumTest1Tmp0 = (qword)spu_splats(0);
	qword recipW = (qword)spu_splats(0);
	qword xyz2Out = (qword)spu_splats(0);
	qword resZ = (qword)spu_splats(0);
	qword frustumTest0 = (qword)spu_splats(0);
	qword mMultXw = (qword)spu_splats(0);
	qword xyz3Out = (qword)spu_splats(0);
	qword xxxxQuantized = (qword)spu_splats(0);
	qword frustumTest1 = (qword)spu_splats(0);
	qword xyz0Out = (qword)spu_splats(0);
	qword xxxxUnit = (qword)spu_splats(0);
	qword yyyyUnit = (qword)spu_splats(0);
	qword xyz1Out = (qword)spu_splats(0);
	qword yyyyQuantized = (qword)spu_splats(0);
	qword xxxxShifted = (qword)spu_splats(0);
	qword zzzz01 = (qword)spu_splats(0);
	qword xyxyxyxy = (qword)spu_splats(0);
	qword wSafe = (qword)spu_splats(0);

	/*  Loop iteration delimited by tabbing
		For example:
		1.  First iteration
			2.  Second Iteration
				3.  Third Iteration
					4.  Fourth Iteration
	*/

	do
	{
            resW = si_fma(zzzz, localToProj32, mMultYw);
                    z2w2z0w0 = si_shufb(zzzzScaled, resWcopy, (qword)s_CcAa);
        inputOffset = si_a(inputOffset, inputOffsetInc);
                    z3w3z1w1 = si_shufb(zzzzScaled, resWcopy, (qword)s_DdBb);
            mMultYx = si_fma(yyyy, localToProj01, mMultXx);

            mMultYy = si_fma(yyyy, localToProj11, mMultXy);
        xyz0 = si_lqx(pVerts0, inputOffset);
                xxxxScaled = si_fma(xxxxUnitSigned, viewportScaleX, viewportOffsetX);
                    xyz2Trans = si_shufb(z2w2z0w0, x0y0x2y2, (qword)s_cdAB);
            mMultYz = si_fma(yyyy, localToProj21, mMultXz);
                    xyz3Trans = si_shufb(z3w3z1w1, x1y1x3y3, (qword)s_cdAB);
                yyyyScaled = si_fma(yyyyUnitSigned, viewportScaleY, viewportOffsetY);
            tmp0 = si_frest(resW);
                zzzzScaled = si_fma(zzzzUnitSigned, viewportScaleZ, viewportOffsetZ);
        xyz2 = si_lqx(pVerts2, inputOffset);
                    over2 = si_fcgt(xyz2Trans, frustumMax);
        xyz3 = si_lqx(pVerts3, inputOffset);
                    over3 = si_fcgt(xyz3Trans, frustumMax);
                    xyz0Trans = si_shufb(x0y0x2y2, z2w2z0w0, (qword)s_ABcd);
            recipEstW = si_fi(resW, tmp0);
                    xyz1Trans = si_shufb(x1y1x3y3, z3w3z1w1, (qword)s_ABcd);
                    under2 = si_fcgt(frustumMin, xyz2Trans);
        xyz1 = si_lqx(pVerts1, inputOffset);
                    under3 = si_fcgt(frustumMin, xyz3Trans);
                x1y1x3y3 = si_shufb(xxxxScaled, yyyyScaled, (qword)s_BbDd);
                    over0 = si_fcgt(xyz0Trans, frustumMax);
                    frustumTest2Tmp0 = si_shufb(under2, over2, (qword)s_FBDbdf00);
                    under1 = si_fcgt(frustumMin, xyz1Trans);
        yyxx23 = si_shufb(xyz2, xyz3, (qword)s_BbAa);
                    under0 = si_fcgt(frustumMin, xyz0Trans);
                    frustumTest3Tmp0 = si_shufb(under3, over3, (qword)s_FBDbdf00);
                    over1 = si_fcgt(xyz1Trans, frustumMax);
        zzzz23 = si_shufb(xyz2, xyz3, (qword)s_CcCc);
            tmp1 = si_fnms(recipEstW, resW, fOne);
        xxyy01 = si_shufb(xyz0, xyz1, (qword)s_AaBb);
                xxxxAsSigned = si_cflts(xxxxScaled, 3);
                    frustumTest0Tmp0 = si_shufb(under0, over0, (qword)s_FBDbdf00);
                yyyyAsSigned = si_cflts(yyyyScaled, 3);
                    frustumTest2 = si_gbh(frustumTest2Tmp0);
            signW = si_and(resW, signMask);
                    frustumTest3 = si_gbh(frustumTest3Tmp0);
            resY = si_fma(zzzz, localToProj12, mMultYy);
        xxxx = si_shufb(xxyy01, yyxx23, (qword)s_ABcd);
            resX = si_fma(zzzz, localToProj02, mMultYx);
                    frustumTest1Tmp0 = si_shufb(under1, over1, (qword)s_FBDbdf00);
            recipW = si_fma(tmp1, recipEstW, recipEstW);
                    xyz2Out = si_shufb(xyz2Trans, frustumTest2, (qword)s_ABCDEFGHd000MNOP);
            resZ = si_fma(zzzz, localToProj22, mMultYz);

			qword preDepth = resZ;
			qword absRecipW = spu_andc(recipW, signMask);
 			resX = si_fm(resX, absRecipW);
 			resY = si_fm(resY, absRecipW);
 			resZ = si_fm(resZ, absRecipW);

			qword posMag2 = si_fma(resX, resX, si_fma(resY, resY, si_fm(resZ, resZ)));
			qword posMag = si_frsqest(posMag2);
			resX = si_fm(resX, posMag);
			resY = si_fm(resY, posMag);
			resZ = si_fma(resZ, posMag, fOne);

			qword resZRecip = si_frest(resZ);
			resX = si_fm(resX, resZRecip);
			resY = si_fm(resY, resZRecip);

			resZ = si_fm(preDepth, dpMapFarClipRecip);

			resW = fOne;

			resX = si_fm(resX, fZeroPointFive);

			resX = si_selb(si_fs(resX, fZeroPointFive),
				si_fma(resX, fNegOne, fZeroPointFive),
				(qword)vec_cmpge((vec_float4)dpMapBasisMtx_0_0, (vec_float4)fZeroPointSevenFive));

                    frustumTest0 = si_gbh(frustumTest0Tmp0);
        mMultXw = si_fma(xxxx, localToProj30, localToProj33);
                    xyz3Out = si_shufb(xyz3Trans, frustumTest3, (qword)s_ABCDEFGHd000MNOP);
                xxxxQuantized = si_and(xxxxAsSigned, sampleMask);
                    frustumTest1 = si_gbh(frustumTest1Tmp0);
        mMultXz = si_fma(xxxx, localToProj20, localToProj23);
                    si_stqx(xyz2Out, pTransVerts2, outputOffset);
        mMultXx = si_fma(xxxx, localToProj00, localToProj03);
                    xyz0Out = si_shufb(xyz0Trans, frustumTest0, (qword)s_ABCDEFGHd000MNOP);
            xxxxUnit = resX;//si_fm(resX, recipW);
                    si_stqx(xyz3Out, pTransVerts3, outputOffset);
            yyyyUnit = resY;//si_fm(resY, recipW);
                    xyz1Out = si_shufb(xyz1Trans, frustumTest1, (qword)s_ABCDEFGHd000MNOP);
            zzzzUnit = resZ;//si_fm(resZ, recipW);
        yyyy = si_shufb(yyxx23, xxyy01, (qword)s_cdAB);
        mMultXy = si_fma(xxxx, localToProj10, localToProj13);
                    si_stqx(xyz0Out, pTransVerts0, outputOffset);
                yyyyQuantized = si_and(yyyyAsSigned, sampleMask);
                xxxxShifted = si_shlqbyi(xxxxQuantized, 2);

                    si_stqx(xyz1Out, pTransVerts1, outputOffset);
        mMultYw = si_fma(yyyy, localToProj31, mMultXw);

        outputOffset = si_a(outputOffset, outputOffsetInc);
        zzzz01 = si_shufb(xyz0, xyz1, (qword)s_CcCc);

                xyxyxyxy = si_shufb(xxxxShifted, yyyyQuantized, s_AbCdEfGh);
            xxxxUnitSigned = si_xor(xxxxUnit, signW);

            yyyyUnitSigned = si_xor(yyyyUnit, signW);
			zzzzUnitSigned = si_xor(zzzzUnit, signW);
        zzzz = si_shufb(zzzz01, zzzz23, (qword)s_ABcd);

                x0y0x2y2 = si_shufb(xxxxScaled, yyyyScaled, (qword)s_AaCc);
                resWcopy = si_selb(xyxyxyxy, wSafe, signMask);
        outputOffsetInc = si_shufb(outputOffsetInc, outputOffsetInc, (qword)s_BCDD);
            wSafe = si_fcgt(resW, floatZero);
	} while (si_to_int(inputOffset) != 0);
}

static inline void Mc4PreTransformCallback(CellSpursJobContext2* jobContext, CellSpursEdgeJob *job, EdgeGeomSpuContext* ctx, const spuGcmState& /*gcmState*/)
{
	const bool isSkinning = ((job->SpuConfigInfo->indexesFlavorAndSkinningFlavor & 0xF) != EDGE_GEOM_SKIN_NONE);
	const bool outputVertices = job->SpuConfigInfo->outputVertexFormatId != 0xff || ctx->customFormatInfo.outputStreamDesc;

	if (outputVertices && !isSkinning)
	{
		switch (job->TransformType)
		{
		case 0:
			{
				// apply vehicle damage
				// the cached buffer contains the damage texture and rest matrix
				if (job->Header.sizeCacheDmaList)
				{
					uint8_t* damageTex = NULL;
					if (job->CachedDmaList.DamageTexture.Size && job->CachedDmaList.DamageTexture.Ea)
					{
						damageTex = (uint8_t*)jobContext->cacheBuffer[0];
					}

					const spuMatrix44* restMtx = NULL;
					if (job->CachedDmaList.RestMatrix.Size && job->CachedDmaList.RestMatrix.Ea)
					{
						restMtx = (const spuMatrix44*)jobContext->cacheBuffer[1];
					}
					
					Float16* fDamageParams = reinterpret_cast<Float16*>(&job->DamageParams[0]);
					vec_float4 vDamageParams =
					{
						fDamageParams[0].Get(),
						fDamageParams[1].Get(),
						fDamageParams[2].Get(),
						fDamageParams[3].Get()
					};

					ApplyVehicleDamage(ctx, restMtx, damageTex, vDamageParams);
				}
				else
				{
					Float16* fDamageParams = reinterpret_cast<Float16*>(&job->DamageParams[0]);
					vec_float4 vDamageParams =
					{
						fDamageParams[0].Get(),
						fDamageParams[1].Get(),
						fDamageParams[2].Get(),
						fDamageParams[3].Get()
					};

					ApplyAmbientVehicleDamage(ctx, vDamageParams);
				}
			}
			break;
		case 2:
			{
				vec_float4 gRoadCarWorldHeight = spu_splats(job->TransformParam);
				ApplyReflectionTransform(ctx, gRoadCarWorldHeight);
			}
			break;
		default:
			break;
		}
	}
}
#endif // __SPU

#ifdef GRCORE_EDGE_JOBS_H
#define EDGE_JOBS_POSTSTALLHOLE_CALLBACK Mc4EdgeJobsPostStallHoleCallback

void Mc4EdgeJobsPostStallHoleCallback(CellSpursEdgeJob* job, CellGcmContextData* ctx, const spuGcmState& gcmState, EdgeGeomPpuConfigInfo* lsPpuConfigInfo)
{
	if (lsPpuConfigInfo->spuConfigInfo.outputVertexFormatId != 0xff || lsPpuConfigInfo->spuOutputStreamDesc)
	{
		switch (gcmState.transformType)
		{
		case 0:
			{
				if (gcmState.damageTexture)
				{
					// read-only cached buffer contains the damage texture and the rest matrix

					cellSpursJobGetCacheList(&job->CachedDmaList.RestMatrix, sizeof(spuMatrix44), (uint32_t)gcmState.restMtx);

					Assert(256*1024<=(uintptr_t)gcmState.damageTexture);
					// expected texture format is 4 component ARGB
					uint32_t texSize = 64*64*4;
					cellSpursJobGetCacheList(&job->CachedDmaList.DamageTexture, texSize, (uint32_t)gcmState.damageTexture);
					job->Header.eaHighCache = 0;
					job->Header.sizeCacheDmaList = sizeof(job->CachedDmaList);
				}
				else
				{
					job->Header.sizeCacheDmaList = 0;
				}

				for (u32 i = 0; i < 4; ++i)
				{
					job->Mc4Params.DamageParams[i] = gcmState.damageParams[i].GetData();
				}
			}
			break;
		case 1: // env map
			{
				job->Header.sizeCacheDmaList = 0;
				job->Mc4Params.Cookie = 0xdeadbeef;
				job->Mc4Params.TransformParam = gcmState.transformParam;
			}
			break;
		case 2: // reflection map
			{
				job->Header.sizeCacheDmaList = 0;
				job->Mc4Params.Cookie = 0xfeedface;
				job->Mc4Params.TransformParam = gcmState.transformParam;
			}
			break;
		default:
			{
				job->Header.sizeCacheDmaList = 0;
			}
			break;
		}
	}
}
#endif // GRCORE_EDGE_JOBS_H

#endif // HACK_MC4
