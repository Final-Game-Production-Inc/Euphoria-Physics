// 
// grcore/edgeExtractgeomspu.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EDGEEXTRACTGEOMSPU_H 
#define GRCORE_EDGEEXTRACTGEOMSPU_H 

#include "grcore\effect_config.h"
#include "grcorespu.h"
#include "vector\vector3.h"
#include "vector\vector4.h"

namespace rage {

//
//
//
//
struct CExtractGeomParams
{
// handy indexes to InputSizes[]:
enum {
	idxSpuOutputStreamDescSize = 0,
	idxSpuInputStreamDescSizeA,
	idxSpuInputStreamDescSizeB,
	idxRsxOnlyStreamDescSize,
	idxIndexSizes,
	idxSkinIsAndWsSizes,
	idxSpuVertexesSizesA,
	idxSpuVertexesSizesB,
	idxFixedOffsetsSizeA,
	idxFixedOffsetsSizeB,
	idxRsxOnlyVertexesSize,
	idxBoneRemapLutSize,
	idxMax
};

// indices to outputBufferVertsPtrsEa:
enum {
	obvIdxPositions				= 0,
	obvIdxNormals,
	obvIdxUVs,
	obvIdxColors,
	
	obvIdxTangents,
	obvIdxNone0,
	obvIdxNone1,
	obvIdxNone2,
	
	obvIdxMax
};

enum {
	extractPos		= BIT(0),
	extractNorm		= BIT(1),
	extractUv		= BIT(2),
	extractCol		= BIT(3),
	extractTan		= BIT(4),
	extractSkin		= BIT(5),

	extractColNoWarning = BIT(6),
};

	u32						outputBufferVertsEa;
	u32						outputBufferIndiciesEa;
	u32						vertexOffsetForBatch;
	u32						numTotalVerts;

	u32						skinMatricesByteOffsets0;
	u32						skinMatricesByteOffsets1;
	u32						skinMatricesSizes0;
	const Vector3*			pBoneNormals;

	u32						ClipPlanesEa;
	u32						ClipPlaneCount;
	Vector4*				EndOfOutputArray;
	u32						outputBufferVertsPtrsEa;

	float					m_dotProdThreshold;
	void*					damageTexture;
	float					boundsRadius;
#if HACK_GTA4_MODELINFOIDX_ON_SPU
	CGta4DbgSpuInfoStruct	gta4SpuInfoStruct;
#endif		

	u16						InputSizes[CExtractGeomParams::idxMax];

	u16						m_bIsHardSkinned:1;
	u16						m_extractMask	:7;
	u16						m_skinMatricesNum : 8;
};


#define EXTRACTGEOM_MAX_SPU_CLIPPLANES			(100)
#define EXTRACTGEOM_MAX_SPU_CLIPPLANES_USED		(64)
#define EXTRACTGEOM_MAX_SPU_VERTS				(2048)

#define DECAL_MAX_BONES							(128)

#define EXTRACTGEOM_pClipPlanesSPU_SIZE			(sizeof(vec_float4*)* EXTRACTGEOM_MAX_SPU_CLIPPLANES)
#define EXTRACTGEOM_BoneNormals_SIZE			(sizeof(vec_float4)	* EXTRACTGEOM_MAX_SPU_CLIPPLANES)
#define EXTRACTGEOM_ClipPlanes_SIZE				(sizeof(vec_float4)	* 6*EXTRACTGEOM_MAX_SPU_CLIPPLANES_USED)

} // namespace rage

#endif // GRCORE_EDGEEXTRACTGEOMSPU_H 
