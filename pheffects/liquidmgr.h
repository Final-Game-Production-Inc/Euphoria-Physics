//
// pheffects/liquidmgr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_LIQUIDMGR_H
#define PHEFFECTS_LIQUIDMGR_H


#include "vector/matrix34.h"
#include "phcore/constants.h"

#define DEFAULT_HIGH_LOD_DISTANCE	80.0f
#define DEFAULT_LOW_LOD_DISTANCE	90.0f

namespace rage {

#if USE_SURFACES

class phInst;
class phSurfaceGrid;
class phBoundSurface;

class phLiquidMgr
{
public:
	phLiquidMgr();
	virtual ~phLiquidMgr();

	void Init (u16 maxSurfaceGrids, u16 maxLiquidInsts, float beginLowLODDistance=DEFAULT_LOW_LOD_DISTANCE, float beginHighLODDistance=DEFAULT_HIGH_LOD_DISTANCE);

	virtual void AddLiquidInst(phInst *pInst);
	virtual void DeleteLiquidInst(phInst *pInst);

	inline void SetCameraMatrix(const Matrix34 &kCameraMatrix);

	// Go through and check/adjust the LOD levels for each instance that we know about.
	void Update();

	void SetLODDistances(float BeginLowLODDistance, float BeginHighLODDistance);

protected:
	void AttachLiquidToNearbyLiquids(Vector3::Vector3Param kNewGridPos, phSurfaceGrid *NewSurfaceGrid) const;

	u16 GetGridIndexFromPool();
	void ReturnGridIndexToPool(u16 GridIndex);
	int CalculateLOD(const phInst* pInst, phBoundSurface* pBound, float fFactor);

	u16 *m_LiquidInstIndices;			// The indices of all of the instances that we know about.
	u16 m_MaxLiquidInsts;				// The size of m_LiquidInstIndices.
	u16 m_NumLiquidInsts;				// The number of entries in m_LiquidInstIndices, always [0..m_NumLiquidInsts - 1].

private:
	phSurfaceGrid *m_SurfaceGrids;		// The surface grids to be allotted to the instances.
	u16 m_MaxSurfaceGrids;				// The size of m_SurfaceGrids.
	u16 m_NumUsedSurfaceGrids;			// The number of surface grids that have been allotted to instances.
	u16	*m_AvailableSurfaceGridIndices;	// A list of surface grids that are not allotted to any instances right now.

	float m_BeginHighLODDistance;
	float m_BeginLowLODDistance;

	Matrix34 m_CameraMatrix;			// We used this in determining LOD levels.
};


void phLiquidMgr::SetCameraMatrix(const Matrix34 &kCameraMatrix)
{
	m_CameraMatrix.Set(kCameraMatrix);
}

#endif // USE_SURFACES

} // namespace rage
#endif
