//
// pheffects/liquidmgr.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "liquidmgr.h"

#include "phbound/boundsurface.h"
#include "phbound/surfacegrid.h"
#include "physics/archetype.h"
#include "physics/inst.h"
#include "physics/iterator.h"
#include "physics/levelnew.h"
#include "system/timemgr.h"
#include "vectormath/classes.h"
#include "vector/geometry.h"

// Enable this to activate some integrity checks within the liquid manager.
#define DEBUG_LIQUID_MANAGER 0

// Enable this to force all liquids to stay at high LOD.
#define DEBUG_FORCE_HIGH_LOD_LIQUID 1

namespace rage {

#if USE_SURFACES

phLiquidMgr::phLiquidMgr()
{
	m_MaxLiquidInsts = 0;
	m_NumLiquidInsts = 0;
	m_LiquidInstIndices = NULL;

	m_MaxSurfaceGrids = 0;
	m_NumUsedSurfaceGrids = 0;
	m_SurfaceGrids = NULL;
	m_AvailableSurfaceGridIndices = NULL;
	m_BeginHighLODDistance = DEFAULT_HIGH_LOD_DISTANCE;
	m_BeginLowLODDistance = DEFAULT_LOW_LOD_DISTANCE;
}


phLiquidMgr::~phLiquidMgr()
{
}


void phLiquidMgr::Init (u16 maxSurfaceGrids, u16 maxLiquidInsts, float beginLowLODDistance, float beginHighLODDistance)
{
	SetLODDistances(beginLowLODDistance, beginHighLODDistance);
	m_MaxLiquidInsts = maxLiquidInsts;

	m_LiquidInstIndices = rage_new u16[maxLiquidInsts];
	u16 uWaterInstIndex;
	for(uWaterInstIndex = 0; uWaterInstIndex < maxLiquidInsts; ++uWaterInstIndex)
	{
		m_LiquidInstIndices[uWaterInstIndex] = phInst::INVALID_INDEX;
	}

	m_MaxSurfaceGrids = maxSurfaceGrids;

	m_SurfaceGrids = rage_new phSurfaceGrid[maxSurfaceGrids];
	m_AvailableSurfaceGridIndices = rage_new u16[maxSurfaceGrids];
	int GridIndex;
	for(GridIndex = 0; GridIndex < maxSurfaceGrids; ++GridIndex)
	{
		m_SurfaceGrids[GridIndex].Clear();
		m_AvailableSurfaceGridIndices[GridIndex] = (u16)(GridIndex);
	}
}

void phLiquidMgr::SetLODDistances(float BeginLowLODDistance, float BeginHighLODDistance)
{
	m_BeginHighLODDistance = BeginLowLODDistance;
	m_BeginLowLODDistance = BeginHighLODDistance;
}

void phLiquidMgr::AddLiquidInst(phInst *pInst)
{
	Assert(m_NumLiquidInsts < m_MaxLiquidInsts);
	if(m_NumLiquidInsts >= m_MaxLiquidInsts)
	{
		return;
	}
	u16 index = pInst->GetLevelIndex();

#if DEBUG_LIQUID_MANAGER
	int counter = m_NumLiquidInsts;
	while(counter-- > 0)
	{
		Assert(m_LiquidInstIndices[counter] != index);
		phInst *pLiquidInst = PHLEVEL->GetInstance(m_LiquidInstIndices[counter]);
		Assert(!pLiquidInst->GetMatrix().IsClose(pInst->GetMatrix()));
	}
#endif

	m_LiquidInstIndices[m_NumLiquidInsts] = index;

	++m_NumLiquidInsts;
}


void phLiquidMgr::DeleteLiquidInst(phInst *LiquidInst)
{
	Assert(LiquidInst != NULL);

	Assert(LiquidInst->GetArchetype()->GetBound()->GetType() == phBound::SURFACE);
	phBoundSurface *WaterBound = static_cast<phBoundSurface *>(LiquidInst->GetArchetype()->GetBound());

	phSurfaceGrid* pGrid = WaterBound->GetGridData();
	Assert(pGrid);
	if(pGrid)
	{
		pGrid->Reset();
		WaterBound->SetGridData(NULL);

		size_t GridIndex = pGrid - &m_SurfaceGrids[0];
		Assert(GridIndex < m_MaxSurfaceGrids);
		ReturnGridIndexToPool((u16)(GridIndex));
	}

	int LiquidInstIndex;
	bool found = false;
	for(LiquidInstIndex = 0; LiquidInstIndex < m_NumLiquidInsts; ++LiquidInstIndex)
	{
		if(LiquidInst->GetLevelIndex() == m_LiquidInstIndices[LiquidInstIndex])
		{
			found = true;
			break;
		}
	}

	// If this asserts, then you are trying to remove an instance that we're not keeping track of.
	Assert(found);
	if(!found)
	{
		return;
	}

	m_NumLiquidInsts--;

	m_LiquidInstIndices[LiquidInstIndex] = m_LiquidInstIndices[m_NumLiquidInsts];
	m_LiquidInstIndices[m_NumLiquidInsts] = phInst::INVALID_INDEX;
}

// PURPOSE:
//  Return LOD level.
// RETURNS:
//	0 = High LOD, ie. distance is < the specified distance.
//  >=1 = Lower LOD. Currently only "low" supported.
#if DEBUG_FORCE_HIGH_LOD_LIQUID
int phLiquidMgr::CalculateLOD(const phInst* , phBoundSurface*, float)
{
	return 0;
}
#else
int phLiquidMgr::CalculateLOD(const phInst* pInst, phBoundSurface* /*pBound*/, float fDistance)
{
	// Old math to switch LOD was using this:
	// Switch to high LOD:
	// m_CameraMatrix.d.Dist2(WaterInst->GetMatrix().d) < square(104.0f + fabs(m_CameraMatrix.c.y) * 54.0f))
	// Switch to low LOD:
	// m_CameraMatrix.d.Dist2(WaterInst->GetMatrix().d) > square(105.0f + fabs(m_CameraMatrix.c.y) * 55.0f))

	// The vectorized version from /rage/dev
	// Mat34V temp = WaterInst->GetMatrix();
	// if(m_CameraMatrix.d.Dist2((reinterpret_cast<Matrix34*>(&temp))->d) > square(105.0f + fabs(m_CameraMatrix.c.y) * 55.0f))

	Mat34V temp = pInst->GetMatrix();
	float dist2 = m_CameraMatrix.d.Dist2((reinterpret_cast<Matrix34*>(&temp))->d);
	if(dist2 < square(fDistance))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
#endif

void phLiquidMgr::Update()
{
	sysTimer oTimer;

	int WaterInstIndex;
	const Vector3 *WaterInstCenter;
	for(WaterInstIndex = 0; WaterInstIndex < m_NumLiquidInsts; ++WaterInstIndex)
	{
		u16 levelIndex = m_LiquidInstIndices[WaterInstIndex];
		if(levelIndex == phInst::INVALID_INDEX)
		{
			continue;
		}

		const phInst *WaterInst = PHLEVEL->GetInstance(levelIndex);
		if(!WaterInst)
		{
			continue;
		}

		Mat34V temp = WaterInst->GetMatrix();
		WaterInstCenter = &(reinterpret_cast<Matrix34*>(&temp))->d;
		Assert(WaterInst->GetArchetype()->GetBound()->GetType() == phBound::SURFACE);
		phBoundSurface *WaterBound = static_cast<phBoundSurface *>(WaterInst->GetArchetype()->GetBound());

		if(WaterBound->GetGridData() != NULL)
		{
			// This instance is at high LOD.  Let's see if it's gotten far enough away to lower it.
			// These tolerances are intentionally slightly different from the ones used below to prevent an instance from repeatedly changing back and forth between LOD levels.
			if(CalculateLOD(WaterInst, WaterBound, m_BeginLowLODDistance) > 0)
			{
				// This instance is too far away - it should be at the low LOD.  Let's take away its grid if it has one.
				// Let's get the index of this bound's grid and ensure that it came from our pool.
				size_t GridIndex = WaterBound->GetGridData() - &m_SurfaceGrids[0];
				Assert(GridIndex < m_MaxSurfaceGrids);

				WaterBound->GetGridData()->DetachNeighbors();
				WaterBound->SetGridData(NULL);

				// Notify the level of the (likely) change in bounding volumes due to attaching the offset grid.
				PHLEVEL->UpdateObjectLocationAndRadius(levelIndex, (Mat34V_Ptr)(NULL));

				ReturnGridIndexToPool((u16)(GridIndex));
			}
		}
		else
		{
			// This instance is at low LOD.  Let's see if it's gotten close enough to raise it to high LOD.
			// These tolerances are intentionally slightly different from the ones used above to prevent an instance from repeatedly changing back and forth between LOD levels.
			if(CalculateLOD(WaterInst, WaterBound, m_BeginHighLODDistance) == 0)
			{
				u16 GridIndex = GetGridIndexFromPool();

				phSurfaceGrid* pGrid = &m_SurfaceGrids[GridIndex];

				pGrid->Clear(); // zero any lingering wave sim values.
				AttachLiquidToNearbyLiquids(*WaterInstCenter, pGrid);

				WaterBound->SetGridData(pGrid);

				// Notify the level of the (likely) change in bounding volumes due to attaching the offset grid.
				PHLEVEL->UpdateObjectLocationAndRadius(levelIndex, (Mat34V_Ptr)(NULL));
			}
		}
	}
}


void phLiquidMgr::AttachLiquidToNearbyLiquids(Vector3::Vector3Param vkNewGridPos, phSurfaceGrid *NewSurfaceGrid) const
{
	// TODO: Take into account the y position and depth of the instances.
	Vector3 kNewGridPos(vkNewGridPos);
#if ENABLE_PHYSICS_LOCK
	phIterator Iterator(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator Iterator;
#endif	// ENABLE_PHYSICS_LOCK
	Iterator.InitCull_XZCircle(kNewGridPos, 32.0f);
	Iterator.SetStateIncludeFlags(phLevelBase::STATE_FLAG_FIXED);
	u16 ObjectLevelIndex = Iterator.GetFirstLevelIndex(PHLEVEL);

	// Set the new cell coordinates based on the world position
	int x = (int) ((kNewGridPos.x - 32.0f) / 64.0f);
	int z = (int) ((kNewGridPos.z - 32.0f) / 64.0f);
#if DEBUG_SURFACE_GRID_COORDINATES
	Displayf("phLiquidMgr::AttachLiquidToNearbyLiquids SetCellCoordinates Original:%f %f  Quantized:%d %d Grid:%08x", kNewGridPos.x, kNewGridPos.z, x,z, NewSurfaceGrid);
#endif
	NewSurfaceGrid->SetCellCoordinates(x,z);

	while(ObjectLevelIndex != phInst::INVALID_INDEX)
	{
		const phInst *OldInstance = PHLEVEL->GetInstance(ObjectLevelIndex);
		if(OldInstance->GetArchetype()->GetBound()->GetType() == phBound::SURFACE)
		{
			phBoundSurface *OldBound = static_cast<phBoundSurface *>(OldInstance->GetArchetype()->GetBound());

			// There's no point trying to attach to a NULL water grid.
			phSurfaceGrid* pOtherGrid = OldBound->GetGridData();
			if(pOtherGrid)
			{
				NewSurfaceGrid->TryAttachNeighbor(pOtherGrid);
			}
		}

		ObjectLevelIndex = Iterator.GetNextLevelIndex(PHLEVEL);
	}
}


u16 phLiquidMgr::GetGridIndexFromPool()
{
	Assert(m_NumUsedSurfaceGrids < m_MaxSurfaceGrids);
	u16 GridIndex = m_AvailableSurfaceGridIndices[m_NumUsedSurfaceGrids];
	++m_NumUsedSurfaceGrids;

	return GridIndex;
}


void phLiquidMgr::ReturnGridIndexToPool(u16 GridIndex)
{
	Assert(m_NumUsedSurfaceGrids > 0);
	--m_NumUsedSurfaceGrids;
	m_AvailableSurfaceGridIndices[m_NumUsedSurfaceGrids] = GridIndex;
}

#endif // USE_SURFACES

}
