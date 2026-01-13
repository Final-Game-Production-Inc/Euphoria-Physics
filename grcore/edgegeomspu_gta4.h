//
// grcore/edgegeomspu.h
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_EDGEGEOMSPU_GTA4_H
#define GRCORE_EDGEGEOMSPU_GTA4_H

#include "effect_config.h"
#include "edgegeomspu.h"
#include "grcorespu.h"

namespace rage
{
namespace edgegeomspujob
{

	// WARNING - See warning below in edgegeomspu.h
struct CellSpursEdgeJob_DmaList : public CellSpursEdgeJob_DmaList_Base
{
};

// WARNING - See warning below in edgegeomspu.h
struct CellSpursEdgeJob_CachedDmaList : public CellSpursEdgeJob_CachedDmaList_Base
{
	Dma ExtraData;
};

// WARNING - See warning below in edgegeomspu.h
struct CellSpursEdgeJob : public CellSpursEdgeJob_Base</*CellSpursEdgeJob_DmaList*/CellSpursEdgeJob_DmaList_Base, CellSpursEdgeJob_CachedDmaList>
{
} ALIGNED(128);

} // namespace edgegeomspujob
} // namespace rage

#endif // GRCORE_EDGEGEOMSPU_GTA4_H
