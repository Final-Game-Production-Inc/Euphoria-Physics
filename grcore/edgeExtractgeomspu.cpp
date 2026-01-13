// 
// grcore/edgeExtractgeomspu.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
//
//
#if __SPU 

#include "system/taskheaderspu.h"
#include "system/task.h"

#include "edge/geom/edgegeom.h"
#include "edge/geom/edgegeom_internal.h"
#include "edge/libedgegeomtool/libedgegeomtool.h"
#include <alloca.h>
#include "vector/vector4.h"
#include <cell/atomic.h>

using namespace rage;
#include "grcore/channel.h"
#include "system/memops.h"
#include "edgeExtractgeomspu.h"

// compiled in breakpoint
//if (whatever) {
//	si_stopd(si_from_uint(0), si_from_uint(1), si_from_uint(2));
//}
//__asm__("stopd 0,0,0");

#define CSTR_EDGEJOBNAME	"edgeExtractGeomSpu"	// handy shortcut for prints, asserts, etc.

#if 0
	#define dbgPrintf(f, args...) Displayf(CSTR_EDGEJOBNAME": [SPU%d,%d] " f, cellSpursGetCurrentSpuId(), __LINE__, ##args)
#else
	#define dbgPrintf(f, args...)
#endif

#define EXTRACT_DMATAG		(8)
#define EXTRACT_DMATAG2		(9)
#define EXTRACT_DMATAG3		(10)
#define EXTRACT_DMATAG4		(11)

extern char _start[];
//extern char *_end;



#define MAX_CLIPPED_VERTS	(11)

static u8		AtomicInfo[128] ALIGNED(128);
vec_float4**	pClipPlanesSPU	=NULL;	// 80 pointers to SPU data
vec_float4*		BoneNormals		=NULL;	// 80 pointers to SPU data
vec_float4*		ClipPlanes		=NULL;	// storage for 6 user clipplanes 


static inline u32 AlignSize16(u32 size)
{
	return (size + 0xF) & ~0xF;
}

inline u8 MapBoneIdx(u8 boneIndex, u32 boneOffsetPoint, u32 boneOffset0, u32 boneOffset1)
{
	if(boneIndex >= boneOffsetPoint)
		boneIndex += boneOffset1-boneOffsetPoint;
	else
		boneIndex += boneOffset0;

	return boneIndex;
}

static qword* DecompressAttributeStreamIntoUniformTable(EdgeGeomSpuContext* ctx,
											EdgeGeomAttributeId edgeAttrID, EdgeGeomVertexStreamDescription *srcStreamDesc,
											void *vertexes, u32 numVertexes, u8 dmaTagToWait);

static EdgeGeomAttributeBlock* GetAttributeBlockFromVertexStreamDesc(EdgeGeomVertexStreamDescription *vertStreamDesc, EdgeGeomAttributeId edgeAttrID);

static void ConvertUVsOneMinusV(vec_float4 *uvs, u32 numVertexes);

//static Vector4 NormalizeNormal(Vector4 normal);
static int ClipPolyAgainstPlane(Vector4* pVerts, Vector4* pNorms, Vector4* pCpvs, int& numVerts, const Vector4 &plane,const Vector3 &clipNormal, float dotProdThreshold);


// ptrs to vetex streams in output vertex buffer:
static u32 eaVertexStreamsPtrs[CExtractGeomParams::obvIdxMax] ;


#define c_GetInput(size)			((size)? c.GetInput((size)) : NULL)

//
//
//
//
void edgeExtractGeomSpu(sysTaskContext& c)
{
EdgeGeomSpuContext	geomCtx	;

#if TASK_SPU_DEBUG_SPEW
	spu_printf("????????%08x: StartEdge %08x@%i\n", ~spu_readch(SPU_RdDec), (uint32_t)jobContext->eaJobDescriptor, cellSpursGetCurrentSpuId());
	spu_printf("io=%06x-%06x scratch=%06x-%06x\n", 
		(uint32_t)jobContext->ioBuffer, job->header.sizeInOrInOut +(uint32_t)jobContext->ioBuffer,
		(uint32_t)jobContext->sBuffer, job->header.sizeScratch * 16 + (uint32_t)jobContext->sBuffer);
#endif//TASK_SPU_DEBUG_SPEW

	// alloc static arrays:
	pClipPlanesSPU	= (vec_float4**)AllocaAligned(u8, EXTRACTGEOM_pClipPlanesSPU_SIZE, 16);
	Assert(pClipPlanesSPU);
	Assert16(pClipPlanesSPU);
	BoneNormals		= (vec_float4*)AllocaAligned(u8, EXTRACTGEOM_BoneNormals_SIZE, 16);
	Assert(BoneNormals);
	Assert16(BoneNormals);
	ClipPlanes		= (vec_float4*)AllocaAligned(u8, EXTRACTGEOM_ClipPlanes_SIZE, 16);
	Assert(ClipPlanes);
	Assert16(ClipPlanes);

CExtractGeomParams& info = *(CExtractGeomParams*)c.GetUserData(0);

	void* ScratchStart	=						c.GetScratch(0);
    void* InputStart	=						c.GetInput(0);
//	void* OutputStart	=						c.GetOutput(0);
//	dbgPrintf("ScratchStart=0x%X, size=%d \nInputStart=0x%X, size=%d \n OutputStart=0x%X, size=%d.", (u32)ScratchStart, (u32)c.ScratchSize(), (u32)InputStart, (u32)c.InputSize(), (u32)OutputStart, (u32)c.OutputSize());
//	dbgPrintf("etext=0x%X, start=0x%X, end=0x%X, start_end_size=%d", (u32)_etext, (u32)_start, (u32)_end, (u32)(_end-_start));

	EdgeGeomViewportInfo *viewportInfo	= NULL;	//( EdgeGeomViewportInfo* )TaskParams.ReadOnly[9];

	EdgeGeomVertexStreamDescription *SpuOutputStreamDesc		= (EdgeGeomVertexStreamDescription*)c_GetInput(info.InputSizes[CExtractGeomParams::idxSpuOutputStreamDescSize]);
	EdgeGeomVertexStreamDescription *rsxOnlyStreamDesc			= (EdgeGeomVertexStreamDescription*)c_GetInput(info.InputSizes[CExtractGeomParams::idxRsxOnlyStreamDescSize]);
	void							*rsxOnlyVertices			= c_GetInput(info.InputSizes[CExtractGeomParams::idxRsxOnlyVertexesSize]);
	
	void *indices						= c_GetInput(info.InputSizes[CExtractGeomParams::idxIndexSizes]);
	void *skinIndicesAndWeights			= c_GetInput(info.InputSizes[CExtractGeomParams::idxSkinIsAndWsSizes]);
	void *skinMatrices					= (info.m_extractMask & CExtractGeomParams::extractSkin) ? c_GetInput(info.m_skinMatricesNum * 48) : skinIndicesAndWeights;
	void *verticesA						= c_GetInput(info.InputSizes[CExtractGeomParams::idxSpuVertexesSizesA]);
	void *verticesB						= c_GetInput(info.InputSizes[CExtractGeomParams::idxSpuVertexesSizesB]);
	void *fixedOffsetsA					= c_GetInput(info.InputSizes[CExtractGeomParams::idxFixedOffsetsSizeA]);
	void *fixedOffsetsB					= c_GetInput(info.InputSizes[CExtractGeomParams::idxFixedOffsetsSizeB]);
	
	EdgeGeomVertexStreamDescription *PrimarySpuInputStreamDesc	= (EdgeGeomVertexStreamDescription*)c_GetInput(info.InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeA]);
	EdgeGeomVertexStreamDescription *SecondarySpuInputStreamDesc= (EdgeGeomVertexStreamDescription*)c_GetInput(info.InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeB]);
	EdgeGeomSpuConfigInfo			*spuConfigInfo				= c.GetInputAs<EdgeGeomSpuConfigInfo>();

	u8 *boneRemapLut                    = (u8*)c_GetInput(info.InputSizes[CExtractGeomParams::idxBoneRemapLutSize]);

const bool bIsSkinning		= ((spuConfigInfo->indexesFlavorAndSkinningFlavor&0xF) != EDGE_GEOM_SKIN_NONE);
const bool bUseClipPlanes	= info.ClipPlanesEa && info.ClipPlaneCount;
const u32  nClipPlaneCount	= info.ClipPlaneCount;
ASSERT_ONLY(const u32 nNumBones = info.ClipPlaneCount);
const bool bIsHardSkinned	= info.m_bIsHardSkinned;
const u8 extractMask		= info.m_extractMask;
Assert(extractMask);
u8	*pBackupSkinIndexes = NULL;
u32 minUniformTables    = 0;
u32 extraUniformTables  = 0;

	if(bUseClipPlanes)
	{
		// dma the PPU array of pointers to SPU:

		const u32 clipPlanesPtrDmaSize = AlignSize16(nClipPlaneCount*sizeof(vec_float4*));
		Assert(clipPlanesPtrDmaSize <= EXTRACTGEOM_pClipPlanesSPU_SIZE);
		sysDmaGet(pClipPlanesSPU, (u32)info.ClipPlanesEa, clipPlanesPtrDmaSize,				EXTRACT_DMATAG);

		sysDmaGet(&BoneNormals[0],(u32)info.pBoneNormals, nClipPlaneCount*sizeof(vec_float4),EXTRACT_DMATAG2);
		
		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG);	// wait for pClipPlanesSPU

		// Go through each of thmae bone pointers and DMA in the frustum  ~(6 plane equations)
		u32 planeCount=0;
		for(u32 i=0; i<nClipPlaneCount; i++)
		{
			if(pClipPlanesSPU[i])
			{
				if(planeCount < EXTRACTGEOM_MAX_SPU_CLIPPLANES_USED)
				{
					//	grcDisplayf(" get clip planes %d/%d PPU=%p to %p count %d",i,info.ClipPlaneCount,pClipPlanesSPU[i],&clipPlanes[count*6],count);
					sysDmaGet(&ClipPlanes[planeCount*6], (u32)pClipPlanesSPU[i], 6*sizeof(vec_float4), EXTRACT_DMATAG);
					pClipPlanesSPU[i] = &ClipPlanes[planeCount*6];
				}
				else
				{
					pClipPlanesSPU[i] = NULL;
					Assertf(0, CSTR_EDGEJOBNAME": Clip plane count exceeded!");
				}

				planeCount++;
			}
		}

		// determine how many additional uniform tables we need to allocate
		if(rsxOnlyVertices)
		{
			Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");

			// If color or normal streams exist in the rsx only stream, then increment the number of tables
			minUniformTables += !!GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
			minUniformTables += !!GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_NORMAL);

			// Once the tables have been decompressed from the rsx only stream,
			// if there is space, then we copy the table over the top of the rsx
			// only stream, and free up the uniform table.
			u32 spaceInRsxOnlyStream = rsxOnlyStreamDesc->stride >> 4;
			extraUniformTables = minUniformTables>spaceInRsxOnlyStream ? minUniformTables-spaceInRsxOnlyStream : 0;
		}

		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG);	// wait for ClipPlanes
		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG2); // wait for BoneNormals

	} //if(bUseClipPlanes)...


	// validate buffer order
	u32 errCode = edgeGeomValidateBufferOrder(
		SpuOutputStreamDesc,
		indices, 
		skinMatrices,	skinIndicesAndWeights,  
		verticesA, 		verticesB, 
		viewportInfo,	NULL/*localToWorld*/,
		spuConfigInfo, 
		fixedOffsetsA, fixedOffsetsB,
		PrimarySpuInputStreamDesc, SecondarySpuInputStreamDesc
	);
	if(errCode != 0)
	{
		spu_printf(CSTR_EDGEJOBNAME": Edge buffer order incorrect; errCode = 0x%08X\n", errCode);
		spu_stop(0);
	}

	if(info.damageTexture)
	{
		Assertf(0, CSTR_EDGEJOBNAME": Unsupported code path reached (damageTexture)!");
	}


EdgeGeomCustomVertexFormatInfo customFormatInfo;
	memset(&customFormatInfo, 0x00, sizeof(EdgeGeomCustomVertexFormatInfo));
	customFormatInfo.inputStreamDescA	= PrimarySpuInputStreamDesc;
	customFormatInfo.inputStreamDescB	= SecondarySpuInputStreamDesc;
	customFormatInfo.outputStreamDesc	= SpuOutputStreamDesc;

	// when clip planes are enabled, we can require up to two additional uniform
	// tables because of decompressing color and normals from the rsx only
	// stream
	u32 uniformTableCount = (spuConfigInfo->flagsAndUniformTableCount & 0x0f) + 1;
	uniformTableCount += extraUniformTables;
	uniformTableCount = Max(uniformTableCount, minUniformTables);
	spuConfigInfo->flagsAndUniformTableCount = (spuConfigInfo->flagsAndUniformTableCount & 0xf0) | (uniformTableCount - 1);

	// initialise edge
	edgeGeomInitialize(
		&geomCtx, 
		spuConfigInfo, 
		ScratchStart,	c.ScratchSize(),	//	jobContext->sBuffer, job->Header.sizeScratch << 4,
		InputStart,		c.InputSize(),		//	jobContext->ioBuffer, job->Header.sizeInOrInOut,
		EXTRACT_DMATAG,	// dmaTag
		viewportInfo, 
		NULL,			// inLocalToWorldMatrix
		&customFormatInfo,
		NULL,			// customTransformInfo
		0				// gcmControlEa
	);



	vec_float4 *coloursInRsxCompressed = NULL;
	vec_float4 *normalsInRsxCompressed = NULL;
	if(bUseClipPlanes)
	{
		Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");

		// decompress the colours if they are in the rsx only stream
		qword *coloursToMove = NULL;
		if(rsxOnlyVertices && GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR))
		{
			coloursToMove = DecompressAttributeStreamIntoUniformTable(&geomCtx,
								EDGE_GEOM_ATTRIBUTE_ID_COLOR, rsxOnlyStreamDesc,
								rsxOnlyVertices, spuConfigInfo->numVertexes, EXTRACT_DMATAG2);
			Assertf(coloursToMove, CSTR_EDGEJOBNAME": Error decompressing cpvs to uniform table! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID); // caller: 0x01=DecalManager, 0x02=SmashObject
		}

		// check if normals present in primary or secondary stream
		// if not, then decompress them from the rsx only stream
		bool bNormalsIdFound=false;
		if(PrimarySpuInputStreamDesc)
		{
			if(GetAttributeBlockFromVertexStreamDesc(PrimarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_NORMAL))
			{
				bNormalsIdFound = true;
			}
		}
		
		if((!bNormalsIdFound) && SecondarySpuInputStreamDesc)
		{
			if(GetAttributeBlockFromVertexStreamDesc(SecondarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_NORMAL))
			{
				bNormalsIdFound = true;
			}
		}

		qword *normalsToMove = NULL;
		if((!bNormalsIdFound) && rsxOnlyStreamDesc) // TODO -- do we need to check rsxOnlyStreamDesc here? (the assert above would catch it)
		{
			// TODO -- should we only do this if rsxOnlyVertices is not NULL?
			if(/*rsxOnlyVertices &&*/ GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_NORMAL))
			{
				bNormalsIdFound = true;
				// decompress normals into uniform table:
				normalsToMove = DecompressAttributeStreamIntoUniformTable(&geomCtx,
									EDGE_GEOM_ATTRIBUTE_ID_NORMAL, rsxOnlyStreamDesc,
									rsxOnlyVertices, spuConfigInfo->numVertexes, EXTRACT_DMATAG2);
				Assertf(normalsToMove, CSTR_EDGEJOBNAME": Error decompressing normals to uniform table! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID); // caller: 0x01=DecalManager, 0x02=SmashObject
			}
		}

		#if HACK_GTA4_MODELINFOIDX_ON_SPU
			Assertf(bNormalsIdFound, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc: Normals not found in primary, secondary or rsxOnly streams! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID); // caller: 0x01=DecalManager, 0x02=SmashObject
		#endif

		// TODO -- what do we do here if rsxOnlyVertices is NULL?

		// the rsxOnlyVertices memory is not needed anymore, if there is space,
		// move the decompressed colour and normals there, and free up the
		// uniform tables.
		u32 spaceInRsxOnlyStream = rsxOnlyStreamDesc->stride >> 4;
		const u32 streamSize = spuConfigInfo->numVertexes * sizeof(vec_float4);
		void *trashedRsxOnlyVertices = rsxOnlyVertices;
		if(coloursToMove && spaceInRsxOnlyStream)
		{
			coloursInRsxCompressed = (vec_float4*)trashedRsxOnlyVertices;
			trashedRsxOnlyVertices = (char*)trashedRsxOnlyVertices + streamSize;
			sysMemCpy(coloursInRsxCompressed, coloursToMove, streamSize);
			--spaceInRsxOnlyStream;
			ASSERT_ONLY(int uniIndex0=) edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
			Assertf(uniIndex0!=-1, "Unassigned colours's have invalid uniform table index!");
		}
		if(normalsToMove && spaceInRsxOnlyStream)
		{
			normalsInRsxCompressed = (vec_float4*)trashedRsxOnlyVertices;
		  //trashedRsxOnlyVertices = (char*)trashedRsxOnlyVertices + streamSize;
			sysMemCpy(normalsInRsxCompressed, normalsToMove, streamSize);
		  //--spaceInRsxOnlyStream;
			ASSERT_ONLY(int uniIndex0=) edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_NORMAL);
			Assertf(uniIndex0!=-1, "Unassigned normals's have invalid uniform table index!");
		}

	}// if(bUseClipPlanes)...

	// decompress spuVerticesA & B:
	edgeGeomDecompressVertexes(&geomCtx, verticesA, fixedOffsetsA, verticesB, fixedOffsetsB);

	if (bIsSkinning && (extractMask & CExtractGeomParams::extractSkin))
	{
		edgeGeomSkinVertexes(&geomCtx, skinMatrices, info.m_skinMatricesNum, skinIndicesAndWeights);
	}

	if(bUseClipPlanes && bIsSkinning)
	{
		if(geomCtx.spuConfigInfo.numVertexes > EXTRACTGEOM_MAX_SPU_VERTS)
		{
			dbgPrintf("ERROR: num verts=%d. Too many verts for create projected textures on SPU!", geomCtx.spuConfigInfo.numVertexes);
			Assert(0);
		}

		u8* pSkinIndiciesAndWeights	= (u8*)skinIndicesAndWeights;
		const u32 sizeOfEdgeMatrix = sizeof(EdgeGeomLocalToWorldMatrix);	// 3x4*float = 3*16 bytes 
		CompileTimeAssert(sizeOfEdgeMatrix == 48);
		const u32 BoneOffset0		= (info.skinMatricesByteOffsets0)/sizeOfEdgeMatrix; // how many matricies the dma skipped from the matrix palette
		const u32 BoneOffset1		= (info.skinMatricesByteOffsets1)/sizeOfEdgeMatrix; // how many matricies the dma skipped from the matrix palette
		const u32 BoneOffsetPoint	= (info.skinMatricesSizes0)/sizeOfEdgeMatrix;
		dbgPrintf("boneOffset0=%d boneOffset1=%d at point %d", BoneOffset0, BoneOffset1, BoneOffsetPoint);

		//back up the bones and weights for later
		const u8 skinningFlavour = spuConfigInfo->indexesFlavorAndSkinningFlavor&0xf;
		const bool bUseSingleBone =	(skinningFlavour == kSkinSingleBoneNoScaling)			||
									(skinningFlavour == kSkinSingleBoneUniformScaling)		||
									(skinningFlavour == kSkinSingleBoneNonUniformScaling);

		const u32 stride = bUseSingleBone? 1 : 8;
		const u32 offset = bUseSingleBone? 0 : 1;
		
		const u32 numVerts = geomCtx.spuConfigInfo.numVertexes;
		pBackupSkinIndexes = (u8*)Alloca(u8, numVerts*stride);
		Assert(pBackupSkinIndexes);

		for(u32 i=0; i<numVerts; i++)
		{
			if(bIsHardSkinned)
			{
				// extract 1 bone only:
				u8 boneIndex = pSkinIndiciesAndWeights[i*stride + offset];
				
				// map boneIdx from SPU to PPU index space:
				boneIndex = MapBoneIdx(boneIndex, BoneOffsetPoint, BoneOffset0, BoneOffset1);

				pBackupSkinIndexes[i] = boneIndex;
			}
			else
			{
				// extract & process all 4 weights+bones:
				Assert(bUseSingleBone==false);	// hardskinned allowed only on single bone

				// backup per-vertex 4x weights and 4x bone idxs:
				u64* pBackupSkinIndexes8		= (u64*)&pBackupSkinIndexes[i*stride];
				u64* pSkinIndiciesAndWeights8	= (u64*)&pSkinIndiciesAndWeights[i*stride];
				*pBackupSkinIndexes8 = *pSkinIndiciesAndWeights8;

				u8 boneIndex0 = pBackupSkinIndexes[i*stride + 1];
				u8 boneIndex1 = pBackupSkinIndexes[i*stride + 3];
				u8 boneIndex2 = pBackupSkinIndexes[i*stride + 5];
				u8 boneIndex3 = pBackupSkinIndexes[i*stride + 7];

				// map boneIdx from SPU to PPU index space:
				boneIndex0 = MapBoneIdx(boneIndex0, BoneOffsetPoint, BoneOffset0, BoneOffset1);
				boneIndex1 = MapBoneIdx(boneIndex1, BoneOffsetPoint, BoneOffset0, BoneOffset1);
				boneIndex2 = MapBoneIdx(boneIndex2, BoneOffsetPoint, BoneOffset0, BoneOffset1);
				boneIndex3 = MapBoneIdx(boneIndex3, BoneOffsetPoint, BoneOffset0, BoneOffset1);

				pBackupSkinIndexes[i*stride + 1] = boneIndex0;
				pBackupSkinIndexes[i*stride + 3] = boneIndex1;
				pBackupSkinIndexes[i*stride + 5] = boneIndex2;
				pBackupSkinIndexes[i*stride + 7] = boneIndex3;
			}// if(!bIsHardSkinned)...
		}
	} //if(bUseClipPlanes && bIsSkinning)...


	edgeGeomDecompressIndexes(&geomCtx, indices); // trashes the boneIndexes and weights, and maybe even verticesA



#if __ASSERT
	// verify whether all input data is located in expected places:
	if (extractMask & CExtractGeomParams::extractPos)
	{
		// positions:
		if(!edgeGeomGetPositionUniformTable(&geomCtx))
		{
#if	HACK_GTA4_MODELINFOIDX_ON_SPU
			Displayf(CSTR_EDGEJOBNAME": uniform positions are NULL! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
#endif
			Assertf(0, CSTR_EDGEJOBNAME": uniform positions are NULL!");
		}
	}

	if (extractMask & CExtractGeomParams::extractNorm)
	{
		// normals:
		if(!edgeGeomGetNormalUniformTable(&geomCtx))
		{
#if	HACK_GTA4_MODELINFOIDX_ON_SPU
			Displayf(CSTR_EDGEJOBNAME": uniform normals are NULL! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
			//Assertf(0, CSTR_EDGEJOBNAME": uniform normals are NULL!");
#endif
		}
	}

	if(!bUseClipPlanes)
	{
		if (extractMask & CExtractGeomParams::extractTan)
		{
			// tangents (optional):
			//	Assertf(GetAttributeBlockFromVertexStreamDesc(SecondarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_TANGENT), CSTR_EDGEJOBNAME": SecondaryInputStream doesn't contain tangents!");
			//	Assertf(edgeGeomGetTangentUniformTable(&geomCtx), CSTR_EDGEJOBNAME": uniform tangents are NULL!");
			if(!GetAttributeBlockFromVertexStreamDesc(SecondarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_TANGENT))	{dbgPrintf(CSTR_EDGEJOBNAME": SecondaryInputStream doesn't contain tangents!");}
			if(!edgeGeomGetTangentUniformTable(&geomCtx))															{dbgPrintf(CSTR_EDGEJOBNAME": uniform tangents are NULL!");}
		}

		if (extractMask & CExtractGeomParams::extractUv)
		{
			Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");

			// uvs:
			if(!GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_UV0))
			{
#if HACK_GTA4_MODELINFOIDX_ON_SPU
				Displayf(CSTR_EDGEJOBNAME": rsxOnlyStream doesn't contain UV0s! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
#endif
				//Assertf(0,CSTR_EDGEJOBNAME": rsxOnlyStream doesn't contain UV0s!");
			}
		}

	#if HACK_GTA4_MODELINFOIDX_ON_SPU
		if (extractMask & CExtractGeomParams::extractCol)
		{
			Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");

			// colors:
			const bool bColorsOnRsxStream = (rsxOnlyStreamDesc && GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR));
			// try again on primary desc:
			const bool bColorsOnPrimaryStream = (PrimarySpuInputStreamDesc && GetAttributeBlockFromVertexStreamDesc(PrimarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR));
			// ... and again on secondary desc:
			const bool bColorsOnSecondaryStream = (SecondarySpuInputStreamDesc && GetAttributeBlockFromVertexStreamDesc(SecondarySpuInputStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR));
			if( !(bColorsOnRsxStream || bColorsOnPrimaryStream || bColorsOnSecondaryStream) )
			{
				if ((extractMask & CExtractGeomParams::extractColNoWarning) == 0)
				{
					Displayf(CSTR_EDGEJOBNAME": rsxOnlyStream/Primary/SecondarySpuInputStream don't contain COLORs! (modelIdx=%d, type=%d, caller=%x).", info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
				}
			}
		}
	#endif
	}
#endif //__ASSERT


vec_float4 *positions	=NULL;
vec_float4 *normals		=NULL;
vec_float4 *colours		=NULL;
vec_float4 *uvs			=NULL;
vec_float4 *tangents	=NULL;

	memset(eaVertexStreamsPtrs, 0x00, sizeof(eaVertexStreamsPtrs));

	positions	= (vec_float4*)edgeGeomGetPositionUniformTable(&geomCtx);
	Assertf(positions, CSTR_EDGEJOBNAME": uniform positions are NULL!");	// this is really BAD!
	normals		= (vec_float4*)edgeGeomGetNormalUniformTable(&geomCtx);
	normals     = normals ? normals : normalsInRsxCompressed;
	colours		= (vec_float4*)edgeGeomGetUniformTableByAttribute(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
	colours     = colours ? colours : coloursInRsxCompressed;
	tangents	= (vec_float4*)edgeGeomGetTangentUniformTable(&geomCtx);

	if(bUseClipPlanes)
	{
		dbgPrintf("use clip planes=%d, skinning=%d.", nClipPlaneCount, bIsSkinning);

		u16*  pIndices = (u16*)geomCtx.indexesLs;

		u32 vertCountSent = 0;
		bool bOverflow = false;

		// clip each triangle against its own particular frustum if it has one for its bone index
		const u32 numIndexes = geomCtx.spuConfigInfo.numIndexes;
		for(u32 tri=0; tri<numIndexes; tri+=3)
		{
			u32 boneIndex1 = 0;
			if(bIsHardSkinned)
			{	// this is hard skinned - i.e. all tris are only influenced by a single bone
				const u32 vtxIdx = pIndices[tri];
				Assert(vtxIdx < geomCtx.spuConfigInfo.numVertexes);
				boneIndex1 = pBackupSkinIndexes[ vtxIdx ];
				if (boneRemapLut)
				{
					boneIndex1 = boneRemapLut[boneIndex1];
				}
				boneIndex1 = nClipPlaneCount>1? boneIndex1 : 0;
			}
			else
			{	// this is not hard skinned - i.e. each vertex of the tri can be influenced by up to 4 bones
				// get the skinning information for this tri
				int   boneIndices[3][4];
				float boneWeights[3][4];
				for(u32 vtx=0; vtx<3; vtx++)
				{
					const u32 vtxIdx = pIndices[tri+vtx];
					Assert(vtxIdx < geomCtx.spuConfigInfo.numVertexes);

					const float inv255 = 1.0f/255.0f;
					boneWeights[vtx][0] = pBackupSkinIndexes[ vtxIdx*8 + 0] * inv255;
					boneWeights[vtx][1] = pBackupSkinIndexes[ vtxIdx*8 + 2] * inv255;
					boneWeights[vtx][2] = pBackupSkinIndexes[ vtxIdx*8 + 4] * inv255;
					boneWeights[vtx][3] = pBackupSkinIndexes[ vtxIdx*8 + 6] * inv255;

					boneIndices[vtx][0] = pBackupSkinIndexes[ vtxIdx*8 + 1];
					boneIndices[vtx][1] = pBackupSkinIndexes[ vtxIdx*8 + 3];
					boneIndices[vtx][2] = pBackupSkinIndexes[ vtxIdx*8 + 5];
					boneIndices[vtx][3] = pBackupSkinIndexes[ vtxIdx*8 + 7];
				}

				// multiple bones affecting these verts - pick the one with most influence
				float accumBoneWeights[DECAL_MAX_BONES];
				for(u32 bone=0; bone<DECAL_MAX_BONES; bone++)
				{
					accumBoneWeights[bone] = 0.0f;
				}

				for(u32 vtx=0; vtx<3; vtx++)
				{
					for(u32 bone=0; bone<4; bone++)
					{
						Assert(boneIndices[vtx][bone] < DECAL_MAX_BONES);
						accumBoneWeights[ boneIndices[vtx][bone] ] += boneWeights[vtx][bone];
					}
				}

				float maxBoneWeight = 0.0f;
				for(u32 bone=0; bone<DECAL_MAX_BONES; bone++)
				{
					if (accumBoneWeights[bone]>maxBoneWeight)
					{
						maxBoneWeight = accumBoneWeights[bone];
						boneIndex1 = bone;
					}
				}

				if (boneRemapLut)
				{
					boneIndex1 = boneRemapLut[boneIndex1];
				}

				boneIndex1 = nClipPlaneCount>1? boneIndex1 : 0;
			}
			#if HACK_GTA4_MODELINFOIDX_ON_SPU
				Assertf(boneIndex1 == 0xff || boneIndex1 < nNumBones, CSTR_EDGEJOBNAME": Invalid bone index %d found (numBones=%d) (modelIdx=%d, type=%d, caller=%x).", boneIndex1, nNumBones, info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
			#endif

			if(boneIndex1 != 0xff && pClipPlanesSPU[boneIndex1])
			{
				Vector4		pos[MAX_CLIPPED_VERTS];
				Vector4		nrm[MAX_CLIPPED_VERTS];
				Vector4     cpv[MAX_CLIPPED_VERTS];

				// this bone has been setup with a frustum
				pos[0] = (Vector4)positions[ pIndices[tri+0] ];
				pos[1] = (Vector4)positions[ pIndices[tri+1] ];
				pos[2] = (Vector4)positions[ pIndices[tri+2] ];
				nrm[0] = normals ? (Vector4)normals[ pIndices[tri+0] ] : Vector4(0,0,1,0);
				nrm[1] = normals ? (Vector4)normals[ pIndices[tri+1] ] : Vector4(0,0,1,0);
				nrm[2] = normals ? (Vector4)normals[ pIndices[tri+2] ] : Vector4(0,0,1,0);
				cpv[0] = colours ? (Vector4)colours[ pIndices[tri+0] ] : Vector4(1,1,1,1);
				cpv[1] = colours ? (Vector4)colours[ pIndices[tri+1] ] : Vector4(1,1,1,1);
				cpv[2] = colours ? (Vector4)colours[ pIndices[tri+2] ] : Vector4(1,1,1,1);

				// re-normalize unpacked normals:
				nrm[0].SetW(0.0f);	//nrm[0] = NormalizeNormal(nrm[0]);
				nrm[1].SetW(0.0f);	//nrm[1] = NormalizeNormal(nrm[1]);
				nrm[2].SetW(0.0f);	//nrm[2] = NormalizeNormal(nrm[2]);

				// see CProjTexMan::ClipPolyAgainstPlane
				int numVerts=3;
				Vector4 *planes = (Vector4*)(pClipPlanesSPU[boneIndex1]); // the 6 planes have been pretransformed on the PPU to "bone space"

				for(u32 planeIdx=0; planeIdx<6; planeIdx++)
				{
					//grcDisplayf(" clip plane %d",planeIndex);
					if(ClipPolyAgainstPlane(&pos[0], &nrm[0], &cpv[0], numVerts, planes[planeIdx],(Vector3)BoneNormals[boneIndex1], info.m_dotProdThreshold) == 0)
					{
						continue;	// this has been rejected - return
					}
					Assert(numVerts < MAX_CLIPPED_VERTS);
				}
				
				// how many verts came back after clipping usually 0, but could be as many as numplanes+3
				if(numVerts) 
				{
					//	grcDisplayf(" did clipping result %d verts",numVerts);
					pos[0].w = (float)numVerts;
					pos[1].w = (float)boneIndex1;
					#if HACK_GTA4_MODELINFOIDX_ON_SPU
						Assertf(boneIndex1<nNumBones, CSTR_EDGEJOBNAME": Invalid bone index %d found (numBones=%d) (modelIdx=%d, type=%d, caller=%x).", boneIndex1, nNumBones, info.gta4SpuInfoStruct.gta4ModelInfoIdx, info.gta4SpuInfoStruct.gta4ModelInfoType, info.gta4SpuInfoStruct.gta4RenderPhaseID);
					#endif
					
					Vector4* oldPPU = (Vector4*)cellAtomicAdd32((u32*)AtomicInfo, (u64)info.outputBufferVertsEa, numVerts*3*sizeof(Vector4));
					vertCountSent += numVerts*3;
					
					if(&oldPPU[numVerts*3] <= info.EndOfOutputArray)
					{
						const u32 dmaSize = numVerts*sizeof(Vector4);
						sysDmaPut((void*)pos, u64(oldPPU),            dmaSize, EXTRACT_DMATAG2);
						sysDmaPut((void*)nrm, u64(oldPPU+numVerts),   dmaSize, EXTRACT_DMATAG2);
						sysDmaPut((void*)cpv, u64(oldPPU+numVerts*2), dmaSize, EXTRACT_DMATAG2);
					}
					else
					{
						// reached the end, rewind the pointer
						if(!bOverflow)
						{	// only show message once
							spu_printf(CSTR_EDGEJOBNAME": Handle SPU projectedTexture Array Overflow (Crash Averted) old PPU=%p  endPPU=%p numverts=%d vertCountSent=%d.", oldPPU, info.EndOfOutputArray, numVerts, vertCountSent);
						}

						cellAtomicAdd32((u32*)AtomicInfo, (u64)info.outputBufferVertsEa, -numVerts*3*sizeof(Vector4));
						bOverflow = true;
					}
					sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG2);

				} // if(numVerts)...

			} // if(pClipPlanesSPU[boneIndex1])...
	
		}// for(u32 tri=0; tri<numIndexes; tri+=3)...
		
		dbgPrintf("Clipped Projected texture Verts Sent back=%d.", vertCountSent);

	}// if(bUseClipPlanes)...
	else
	{
		dbgPrintf("DMA'ing back %d positions (0x%X) and %d indexes (0x%X).", geomCtx.spuConfigInfo.numVertexes, (u32)positions, geomCtx.spuConfigInfo.numIndexes, (u32)geomCtx.indexesLs);

		const u32 dmaSize		= geomCtx.spuConfigInfo.numVertexes * sizeof(Vector4);
		const u32 offsetSize	= info.numTotalVerts * sizeof(Vector4);
		u32 offset = 0;

		if (extractMask & CExtractGeomParams::extractPos)
		{
			// put back positions:
			const u32 dstPosEa = info.outputBufferVertsEa + offset*offsetSize;
			sysDmaLargePut(positions, dstPosEa,	dmaSize, EXTRACT_DMATAG2);
			dbgPrintf("dma: positions=0x%X", (u32)positions);
			eaVertexStreamsPtrs[CExtractGeomParams::obvIdxPositions] = dstPosEa;
			offset++;
		}

		// put back normals:
		if(normals && (extractMask & CExtractGeomParams::extractNorm))
		{
			const u32 dstNormalsEa = info.outputBufferVertsEa + offset*offsetSize;
			sysDmaLargePut(normals,	dstNormalsEa,	dmaSize, EXTRACT_DMATAG);
			dbgPrintf("dma: normals=0x%X", (u32)normals);
			eaVertexStreamsPtrs[CExtractGeomParams::obvIdxNormals] = dstNormalsEa;
			offset++;
		}

		// put back indexes:
		if(1)
		{
			// convert from index into local PPU vert array we are building up
			u16 *pIndexes = (u16*)geomCtx.indexesLs;
			const u32 idxCount = geomCtx.spuConfigInfo.numIndexes;
			for(u32 i=0; i<idxCount; i++)
			{
				*(pIndexes++) += info.vertexOffsetForBatch;
			}

			if(Unlikely((info.outputBufferIndiciesEa & 0xf) != 0))
			{
				CompileTimeAssert(sizeof(u16) == 2);

				u8 newIndexBuffer[2][16] ;
				u32 dbTag[2] = { EXTRACT_DMATAG3, EXTRACT_DMATAG4 };
				bool dbIndex = false;

				u32 indexEa = info.outputBufferIndiciesEa & ~0xf;
				sysDmaGet(newIndexBuffer[dbIndex], indexEa, 16, EXTRACT_DMATAG3);

				u32 addrDelta = info.outputBufferIndiciesEa - indexEa;
				Assert((addrDelta & 0x1) == 0);

				u16* sourceIndices = (u16*)geomCtx.indexesLs;
				s32 numIndices = geomCtx.spuConfigInfo.numIndexes;

				sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG3);

				// handle first line by patching the last bytes to align it
				for (u32 i = addrDelta; i < 16; i += 2, --numIndices)
				{
					*(u16*)&(newIndexBuffer[dbIndex][i]) = *(sourceIndices++);
				}

				// dma first line
				sysDmaPut(newIndexBuffer[dbIndex], indexEa, 16, dbTag[dbIndex]);

				// handle the rest of the index buffer
				while (numIndices > 0)
				{
					indexEa += 16;
					dbIndex = !dbIndex;

					u16* indices = (u16*)newIndexBuffer[dbIndex];

					// convert from index into local PPU vert array we are building up
					for(u32 i = 0; i < 8; ++i)
					{
						*(indices++) = *(sourceIndices++);
					}

					sysDmaPut(newIndexBuffer[dbIndex], indexEa, 16, dbTag[dbIndex]);
					numIndices -= 8;

					sysDmaWaitTagStatusAll(1<<dbTag[!dbIndex]);
				}
			}
			else
			{
				//round dma up to 16 bytes
				const u32 dmaIndexSize = ((geomCtx.spuConfigInfo.numIndexes*sizeof(u16))+0x0f)&(~0xf);
				sysDmaLargePut((void*)geomCtx.indexesLs, info.outputBufferIndiciesEa, dmaIndexSize, EXTRACT_DMATAG);
			}
		}

		// free positions from uniform table:
		ASSERT_ONLY(int uniIndex0=) edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_POSITION);
		Assertf(uniIndex0!=-1, "Unassigned positions have invalid uniform table index!");
		//dbgPrintf("\n Unassigned positions: index=%d", uniIndex0);
//		int uniIndex1 = edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_NORMAL);
//		dbgPrintf("\n Unassigned normals: index=%d", uniIndex1);


		if (extractMask & CExtractGeomParams::extractUv)
		{
			Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");
			Assertf(rsxOnlyVertices, CSTR_EDGEJOBNAME": rsxOnlyVertices is NULL!");

			// decompress UVs into uniform table:
			uvs = (vec_float4*)DecompressAttributeStreamIntoUniformTable(&geomCtx,
				EDGE_GEOM_ATTRIBUTE_ID_UV0, rsxOnlyStreamDesc,
				rsxOnlyVertices, spuConfigInfo->numVertexes, EXTRACT_DMATAG2);
			if(uvs)
			{
				// UVs: "v = 1.0-v" conversion (as done in grcVertexBufferEditor::GetUV()):
				ConvertUVsOneMinusV(uvs, spuConfigInfo->numVertexes);

				const u32 dstUVsEa = info.outputBufferVertsEa + offset*offsetSize;
				sysDmaLargePut(uvs, dstUVsEa,	dmaSize, EXTRACT_DMATAG2);
				dbgPrintf("dma: uvs=0x%X", (u32)uvs);
				eaVertexStreamsPtrs[CExtractGeomParams::obvIdxUVs] = dstUVsEa;
				offset++;

				ASSERT_ONLY(int uniIndex0=) edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_UV0);
				//dbgPrintf("\n Unassigned uvs: index=%d", uniIndex0);
				Assertf(uniIndex0!=-1, "Unassigned uv0's have invalid uniform table index!");
			}// uvs...
		}

		if (extractMask & CExtractGeomParams::extractCol)
		{
			Assertf(rsxOnlyStreamDesc, CSTR_EDGEJOBNAME": rsxOnlyStreamDesc is NULL!");
			Assertf(rsxOnlyVertices, CSTR_EDGEJOBNAME": rsxOnlyVertices is NULL!");

			// decompress COLORs into uniform table:
			colours = (vec_float4*)DecompressAttributeStreamIntoUniformTable(&geomCtx,
				EDGE_GEOM_ATTRIBUTE_ID_COLOR, rsxOnlyStreamDesc,
				rsxOnlyVertices, spuConfigInfo->numVertexes, EXTRACT_DMATAG2);
			// ..and try again with secondary vertices:
			if(!colours)
			{
				colours = (vec_float4*)DecompressAttributeStreamIntoUniformTable(&geomCtx,
					EDGE_GEOM_ATTRIBUTE_ID_COLOR, SecondarySpuInputStreamDesc,
					verticesB, spuConfigInfo->numVertexes, EXTRACT_DMATAG2);
			}

			if(colours)
			{
				const u32 dstColorsEa = info.outputBufferVertsEa + offset*offsetSize;
				sysDmaLargePut(colours, dstColorsEa,	dmaSize, EXTRACT_DMATAG);
				dbgPrintf("dma: colours=0x%X", (u32)colours);
				eaVertexStreamsPtrs[CExtractGeomParams::obvIdxColors] = dstColorsEa;
				offset++;

				ASSERT_ONLY(int uniIndex0=) edgeGeomUnassignUniformTable(&geomCtx, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
				//dbgPrintf("\n Unassigned colors: index=%d", uniIndex0);
				Assertf(uniIndex0!=-1, "Unassigned colors have invalid uniform table index!");
			}
		}

		if(tangents && (extractMask & CExtractGeomParams::extractTan))
		{
			const u32 dstTangentsEa = info.outputBufferVertsEa + offset*offsetSize;
			sysDmaLargePut(tangents, dstTangentsEa, dmaSize, EXTRACT_DMATAG);
			dbgPrintf("dma: tangents=0x%X", (u32)tangents);
			eaVertexStreamsPtrs[CExtractGeomParams::obvIdxTangents] = dstTangentsEa;
			offset++;
		}

		// output pointers to streams (if requested):
		if(info.outputBufferVertsPtrsEa)
		{
			const u32 dmaSize = sizeof(eaVertexStreamsPtrs);
			CompileTimeAssert16(dmaSize);
			sysDmaLargePut(eaVertexStreamsPtrs, info.outputBufferVertsPtrsEa, dmaSize, EXTRACT_DMATAG);
		}

		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG);
		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG2);
		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG3);
		sysDmaWaitTagStatusAll(1<<EXTRACT_DMATAG4);
	}//	if(!bUseClipPlanes)...

	edgeGeomFinalize(&geomCtx);


#if TASK_SPU_DEBUG_SPEW
	spu_printf("????????%08x: EndEdge %08x@%i\n", ~spu_readch(SPU_RdDec), (uint32_t)jobContext->eaJobDescriptor, cellSpursGetCurrentSpuId());
#endif//TASK_SPU_DEBUG_SPEW
	
#if TASK_SPU_EXTRA_CHECKS
	// task_spu.h: cellSpursJobMain2()'s sets check markers around buffers, now check whether they're still there:
	checkSpursMarkers(CSTR_EDGEJOBNAME);
#endif

	// SPURS job will finish the outstanding pushbuffer DMA (from the ioBuffer) 
	// this is the reason why ioBuffer is declared as an input AND output buffer 
//	grcDisplayf(" End extract Geom SPU id %d",info.ExtractID);

}// end of edgeExtractGeomSpu()...





//
//
//
//
static
EdgeGeomAttributeBlock* GetAttributeBlockFromVertexStreamDesc(EdgeGeomVertexStreamDescription *vertStreamDesc, EdgeGeomAttributeId edgeAttrID)
{
	const u32 numBlocks = vertStreamDesc->numBlocks;

	EdgeGeomGenericBlock *nextAttribute = vertStreamDesc->blocks;

	for(u32 i=0; i<numBlocks; i++)
	{
		EdgeGeomAttributeBlock *block = &nextAttribute->attributeBlock;
		nextAttribute++;

		if(block->edgeAttributeId == edgeAttrID)
			return(block);
	}

	return(NULL);
}

//
//
//
//
static
qword* DecompressAttributeStreamIntoUniformTable(EdgeGeomSpuContext* ctx,
											EdgeGeomAttributeId edgeAttrID, EdgeGeomVertexStreamDescription *srcStreamDesc,
											void *vertexes, u32 numVertexes, u8 dmaTagToWait)
{
qword *dstAddr = NULL;


	// is there UV0 attribute stream in rsxOnlyStream?
	EdgeGeomAttributeBlock *srcBlockAttr = GetAttributeBlockFromVertexStreamDesc(srcStreamDesc, edgeAttrID);
	//		Assertf(blockUV0, CSTR_EDGEJOBNAME": no UV0 attribute block found!");
	if(srcBlockAttr)
	{
		u32 zeroFixedOffsets[1]  = {0};

		const u32 structSize = sizeof(EdgeGeomVertexStreamDescription)+sizeof(EdgeGeomAttributeBlock);
		u8 _tmp[structSize] ;
		memset(_tmp, 0x00, structSize);

		// build custom vertexStreamDesc with 1 attr block:
		EdgeGeomVertexStreamDescription *customDesc = (EdgeGeomVertexStreamDescription*)&_tmp[0];

		EdgeGeomAttributeBlock *customBlock = &customDesc->blocks[0].attributeBlock;
		memcpy(customBlock, srcBlockAttr, sizeof(EdgeGeomAttributeBlock));

		customDesc->numAttributes	= 1;
		customDesc->numBlocks		= 1;
		customDesc->stride			= srcStreamDesc->stride;

		// wait for prev dma to finish storing from the dest buffer:
		sysDmaWaitTagStatusAll(1 << dmaTagToWait);

		//dbgPrintf("about to decompress uv0s! numVerts=%d", spuConfigInfo->numVertexes);
		// decompress!
		DecompressVertexesByDescription(ctx,
			vertexes, &zeroFixedOffsets[0], (const EdgeGeomVertexStreamDescription*)customDesc,
			numVertexes, 0.0f/*blendFactor*/, NULL/*blendedVertexTable*/);

		dstAddr = edgeGeomGetUniformTableByAttribute(ctx, edgeAttrID);
		//dbgPrintf("uvs decompressed to 0x%X", (u32)uvs);

		if(u32(dstAddr) == u32(-1))
			dstAddr = NULL;
	}

	return(dstAddr);

}// end of DecompressAttributeStreamIntoUniformTable()...


//
//
// UVs: "v = 1.0-v" conversion (as done in grcVertexBufferEditor::GetUV()):
//
static
void ConvertUVsOneMinusV(vec_float4 *uvs, u32 numVertexes)
{
	vec_float4	fOne	= spu_splats(1.0f);
	vec_uchar16 s_AbCD	= {0x00, 0x01, 0x02, 0x03,  0x14, 0x15, 0x16, 0x17,  0x08, 0x09, 0x0a, 0x0b,  0x0c, 0x0d, 0x0e, 0x0f};


const u32 count		= numVertexes;
const u32 count8	= numVertexes & (~0x07);
u32 i=0;

	for(i=0; i<count8; i+=8)
	{
		vec_float4 uv0a = uvs[i+0];
		vec_float4 uv1a = uvs[i+1];
		vec_float4 uv2a = uvs[i+2];
		vec_float4 uv3a = uvs[i+3];
		vec_float4 uv4a = uvs[i+4];
		vec_float4 uv5a = uvs[i+5];
		vec_float4 uv6a = uvs[i+6];
		vec_float4 uv7a = uvs[i+7];

		vec_float4 uv0b = spu_nmsub(uv0a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv1b = spu_nmsub(uv1a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv2b = spu_nmsub(uv2a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv3b = spu_nmsub(uv3a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv4b = spu_nmsub(uv4a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv5b = spu_nmsub(uv5a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv6b = spu_nmsub(uv6a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv7b = spu_nmsub(uv7a, fOne, fOne);	// uv1 = 1.0 - uv0;

		vec_float4 uv0c = spu_shuffle(uv0a, uv0b, s_AbCD);
		vec_float4 uv1c = spu_shuffle(uv1a, uv1b, s_AbCD);
		vec_float4 uv2c = spu_shuffle(uv2a, uv2b, s_AbCD);
		vec_float4 uv3c = spu_shuffle(uv3a, uv3b, s_AbCD);
		vec_float4 uv4c = spu_shuffle(uv4a, uv4b, s_AbCD);
		vec_float4 uv5c = spu_shuffle(uv5a, uv5b, s_AbCD);
		vec_float4 uv6c = spu_shuffle(uv6a, uv6b, s_AbCD);
		vec_float4 uv7c = spu_shuffle(uv7a, uv7b, s_AbCD);

		uvs[i+0] = uv0c;
		uvs[i+1] = uv1c;
		uvs[i+2] = uv2c;
		uvs[i+3] = uv3c;
		uvs[i+4] = uv4c;
		uvs[i+5] = uv5c;
		uvs[i+6] = uv6c;
		uvs[i+7] = uv7c;
	}

	// last 0-7 verts:
	for(; i<count; i++)
	{
		vec_float4 uv0a = uvs[i];
		vec_float4 uv0b = spu_nmsub(  uv0a, fOne, fOne);	// uv1 = 1.0 - uv0;
		vec_float4 uv0c = spu_shuffle(uv0a, uv0b, s_AbCD);
		uvs[i] = uv0c;
	}

}// end of ConvertUVsOneMinusV()...



//
//
// works like CProjTexInst::ClipPolyAgainstPlane():
//
static int ClipPolyAgainstPlane(Vector4* pVerts, Vector4* pNorms, Vector4* pCpvs, int& numVerts,
								const Vector4 &plane,const Vector3 &clipNormal, float dotProdThreshold)
{
	// calc this triangles normal
const Vector3 edge10(Vec::Vector_4(	pVerts[1]-pVerts[0]	));
const Vector3 edge20(Vec::Vector_4(	pVerts[2]-pVerts[0]	));
Vector3 faceNormal(edge10);
	faceNormal.Cross(edge20);
	faceNormal.Normalize();

	// check if this tri needs rejected by checking its face normal
	const float dot = faceNormal.Dot(clipNormal);
	if (dot < dotProdThreshold)
	{
		numVerts = 0;
		return(0);
	}

// calc the distances to the plane of each vertex
float dists[MAX_CLIPPED_VERTS];

	//	Vector4& plane = setupData.m_xformClipPlanes[planeId];
	Vector4* pV = pVerts;
	for(int i=0; i<numVerts; i++)
	{
		//		ASSERT(CMaths::IsNumericallyValid(pVerts[i].x));
		//		ASSERT(CMaths::IsNumericallyValid(pVerts[i].y));
		//		ASSERT(CMaths::IsNumericallyValid(pVerts[i].z));
		dists[i] = (pV->x*plane.x + pV->y*plane.y + pV->z*plane.z) - plane.w;
		//		grcDisplayf(" got to dists calc %d",i);
		pV++;
	}

	// go through the dists outputting the clipped verts
int numClippedVerts = 0;
Vector4 clippedVerts[MAX_CLIPPED_VERTS+1];
Vector4 clippedNorms[MAX_CLIPPED_VERTS+1];
Vector4 clippedCpvs [MAX_CLIPPED_VERTS+1];

	for (int i=0; i<numVerts; i++)
	{	
		//		grcDisplayf(" clip vert %d",i);
		int id1 = i;
		int id2 = i+1;

		if (i==numVerts-1)
		{
			id2 = 0;
		}

		if (dists[id1]<=0.0f)
		{
			if (dists[id2]<=0.0f)
			{
				// in, in - add them both 
				clippedVerts[numClippedVerts] = pVerts[id2];
				clippedNorms[numClippedVerts] = pNorms[id2];
				clippedCpvs [numClippedVerts] = pCpvs [id2];
				numClippedVerts++;
			}
			else
			{
				// in, out  - add the intersection
				float t = dists[id1]/(dists[id1]-dists[id2]);
				// somehow the Xenon complains when t * v (ambiguous) but even weirder
				// it likes v * t
				clippedVerts[numClippedVerts] = pVerts[id1] + ((pVerts[id2]-pVerts[id1]) * t);
				clippedNorms[numClippedVerts] = pNorms[id1] + ((pNorms[id2]-pNorms[id1]) * t);
				clippedCpvs [numClippedVerts] = pCpvs [id1] + ((pCpvs [id2]-pCpvs [id1]) * t);
				numClippedVerts++;
			}
		}
		else if (dists[id2]<=0.0f)
		{
			// out, in - add the intersecion and the 2nd point
			float t = dists[id1]/(dists[id1]-dists[id2]);
			// same as above
			clippedVerts[numClippedVerts] = pVerts[id1] + ((pVerts[id2]-pVerts[id1]) * t);
			clippedNorms[numClippedVerts] = pNorms[id1] + ((pNorms[id2]-pNorms[id1]) * t);
			clippedCpvs [numClippedVerts] = pCpvs [id1] + ((pCpvs [id2]-pCpvs [id1]) * t);
			numClippedVerts++;
			clippedVerts[numClippedVerts] = pVerts[id2];
			clippedNorms[numClippedVerts] = pNorms[id2];
			clippedCpvs [numClippedVerts] = pCpvs [id2];
			numClippedVerts++;
		}
	}

#if __ASSERT
	if (numClippedVerts > numVerts+1)
	{
		// something has gone wrong - a max of 1 extra vert should have been added only
		Printf("Plane is %.2f %.2f %.2f %.2f\n", plane.x, plane.y, plane.z, plane.w);
		Printf("Verts are %.2f %.2f %.2f\n", pVerts->x, pVerts->y, pVerts->z);
		Assertf(0, "Projected Texture Triangle Clipping started with %d verts and ended with %d\n", numVerts, numClippedVerts);
	}
#endif

	//	grcDisplayf(" output verts ");
	// copy over the clipped verts
	for (int i=0; i<numClippedVerts; i++)
	{
		pVerts[i] = clippedVerts[i];
		pNorms[i] = clippedNorms[i];
		pCpvs [i] = clippedCpvs [i];
	}


	numVerts = numClippedVerts;
	//	grcDisplayf(" return");

	return numClippedVerts;

}// end of ClipPolyAgainstPlane()...


/*
inline float SignX(float x)
{
	return((x>=0.0f) ? (1.0f) : (-1.0f));
}

//
// check if normal is correctly normalized
// if not, then fix it somehow in most sensible way:
//
static Vector4 NormalizeNormal(Vector4 normal)
{
	normal.SetW(0.0f);

	Vector4 newNormal = normal;

	const float len = normal.Mag();
	if(len != 1.0f)
	{
		float nx = normal.GetX();
		float ny = normal.GetY();
		float nz = 0.0f;

		if(Abs(nx) >= 1.0f)
		{
			nx = 1.0f * SignX(nx);
			ny = nz = 0.0f;
		}
		else if(Abs(ny) >= 1.0f)
		{
			ny = 1.0f * SignX(ny);
			nx = nz = 0.0f;
		}
		else
		{
			const float nz0 = 1.0f - nx*nx - ny*ny;
			nz = Sqrtf(Abs(nz0));
		}

		newNormal = Vector4(nx, ny, nz, 0.0f);
		//		Printf("\n NormalizeNormalSPU: renormalizing (%.5f, %.5f, %.5f) to (%.5f, %.5f, %.5f)", normal.GetX(),normal.GetY(),normal.GetZ(), newNormal.GetX(),newNormal.GetY(),newNormal.GetZ());
	}

	return newNormal;
}// end of NormalizeNormal()...
*/

#endif // __SPU...


