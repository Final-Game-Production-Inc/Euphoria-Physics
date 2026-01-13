// 
// grcore/edgegeomspu_mc4.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EDGEGEOMSPU_MC4_H
#define GRCORE_EDGEGEOMSPU_MC4_H

#include "edgegeomspu.h"

namespace rage
{
namespace edgegeomspujob
{

/*
struct CellSpursEdgeJob_DmaList : public CellSpursEdgeJob_DmaList_Base
{
} ;
*/

struct CellSpursEdgeJob_CachedDmaList : public CellSpursEdgeJob_CachedDmaList_Base
{
	Dma DamageTexture;
	Dma RestMatrix;
} ;

struct CellSpursEdgeJob : public CellSpursEdgeJob_Base</*CellSpursEdgeJob_DmaList*/CellSpursEdgeJob_DmaList_Base, CellSpursEdgeJob_CachedDmaList>
{
	// Code like this will not work anymore.  CellSpursEdgeJob may not add any
	// further data members, since drawablespu may truncate the structure sent
	// to edgegeomspu if some of the spuGcmStateBaseEdge data is not required.

// 	u16 TransformType;
// 	float TransformParam;
// 	u16 DamageParams[4];
} ALIGNED(128);

} // namespace edgegeomspujob
} // namespace rage

#endif // GRCORE_EDGEGEOMSPU_MC4_H 
