//
// physics/btSpatialHash.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "physics/btImmediateModeBroadphase.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "vector/vector3.h"
#include "physics/HashBroadPhase16.h"

namespace rage
{

class btSpatialHash : public btImmediateModeBroadphase
{

public:

	// coarsestVoxelResolution is the number of voxels in the widest resolution of the grid ( the one that the biggest objects will be in )
	// each level below will have _more_ voxels
	btSpatialHash( u16 maxHandles, u16 maxPairs, Vector3::Vector3Param vworldMin, Vector3::Vector3Param vworldMax, btOverlappingPairCache *existingPairCache = NULL, int nLevel = 3, int startGridResolution = 256 ) : btImmediateModeBroadphase( maxHandles, maxPairs, existingPairCache )
	{
		m_IsIncremental = false;
		Vector3 worldMin(vworldMin);
		Vector3 worldMax(vworldMax);
		float wx = worldMax.x - worldMin.x;
		float wy = worldMax.y - worldMin.y;
		
		m_bp.init( Max( wx, wy ), worldMin.x, worldMin.y, nLevel, startGridResolution );
	}

	virtual ~btSpatialHash(){}

	void pruneActiveOverlappingPairs();

	CHashBroadPhase16 m_bp;	

};

}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif
