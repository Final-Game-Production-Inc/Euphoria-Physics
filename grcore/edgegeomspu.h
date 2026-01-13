// 
// grcore/edgegeomspu.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EDGEGEOMSPU_H 
#define GRCORE_EDGEGEOMSPU_H 

#include <cell/spurs/job_descriptor.h>
#include "edge/geom/edgegeom_structs.h"

namespace rage
{
namespace edgegeomspujob
{

struct Dma
{
	u32 Size;
	u32 Ea;
};

// WARNING - See warning below.
struct CellSpursEdgeJob_DmaList_Base
{
	Dma SpuOutputStreamDesc;
	Dma RsxOnlyStreamDesc;
	Dma Indexes0;
	Dma Indexes1;
	Dma SkinningMatrices0;
	Dma SkinningMatrices1;
	Dma SkinningIndexesAndWeights0;
	Dma SkinningIndexesAndWeights1;
	Dma PrimaryVertexes0;
	Dma PrimaryVertexes1;
	Dma PrimaryVertexes2;
	Dma SecondaryVertexes0;
	Dma SecondaryVertexes1;
	Dma SecondaryVertexes2;
	Dma PrimaryFixedPointOffsets;
	// Dma SecondaryFixedPointOffsets;
	Dma PrimarySpuInputStreamDesc;
	Dma SecondarySpuInputStreamDesc;
};

struct CellSpursEdgeJob_CachedDmaList_Base
{
};

// WARNING - Do not modify this structure without updating the code in edge_jobs.cpp.
// To improve performance, that code has been rewritten to write quadwords directly into
// this structure.  The header is exactly 48 bytes, cleared with three quadword writes.
// Every element of this structure is filled with packed quadword writes (well, except
// for a few bits in the Header which get rewritten, but that's a private Sony structure).
template <typename DmaListType, typename CachedDmaListType>
struct CellSpursEdgeJob_Base
{
	CellSpursJobHeader Header;

	DmaListType DmaList;
	CachedDmaListType CachedDmaList;

	EdgeGeomSpuConfigInfo SpuConfigInfo;

	float Offset[3] ;
	u32 CommandBufferHoleEa;

	u32 OutputBufferInfoEa;
	u32 BlendShapesEa;
	u32 RsxOnlyVertexOffset;
	u16 VertexShaderInputMask;
	u16 NumBlendShapes;

	// Must be the last field in this struct, since it is not always copied over
	// completely by drawablespu.  So CellSpursEdgeJob which derives from
	// CellSpursEdgeJob_Base must not add any data fields.
	spuGcmStateBaseEdge SpuGcmState;
};

} // namespace edgegeomspujob
} // namespace rage

#if HACK_GTA4
#include "edgegeomspu_gta4.h"
#elif HACK_MC4
#include "edgegeomspu_mc4.h"
#else
namespace rage
{
namespace edgegeomspujob
{
struct CellSpursEdgeJob_DmaList : public CellSpursEdgeJob_DmaList_Base {};
struct CellSpursEdgeJob_CachedDmaList : public CellSpursEdgeJob_CachedDmaList_Base {};
struct CellSpursEdgeJob : public CellSpursEdgeJob_Base<CellSpursEdgeJob_DmaList_Base, CellSpursEdgeJob_CachedDmaList_Base> {} ALIGNED(128);
} // namespace edgegeomspujob
} // namespace rage
#endif

#endif // GRCORE_EDGEGEOMSPU_H 
