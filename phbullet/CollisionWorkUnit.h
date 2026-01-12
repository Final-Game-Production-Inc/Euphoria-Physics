//
// publllet/CollisionWorkUnit.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBULLET_COLLISIONWORKUNIT_H
#define PHBULLET_COLLISIONWORKUNIT_H

#include <assert.h>
#include "vector/matrix34.h"

#if TRACK_COLLISION_TYPE_PAIRS
#include "phbound/bound.h" // for phBound::NUM_BOUND_TYPES
#endif

#include "phcore/constants.h"
#include "phcore/pool.h"
#include "GjkSimplexCache.h"

namespace rage {

class phArticulatedCollider;
class phBound;
class phCompositePointers;
class phContact;
class phManifold;
struct phMaterialPair;
struct phTaskCollisionPair;
typedef volatile unsigned sysSpinLockToken;
class Mat34V;

}

struct PairListWorkUnitInput
{
	rage::Vector3 m_minimumConcaveThickness;

	rage::ScalarV m_allowedPenetration;

    rage::phTaskCollisionPair* m_pairListPtr;
    int* m_numPairsMM;
    bool* m_allPairsReadyMM;
    int m_numPairsPerMutexLock;

	bool m_selfCollisionsEnabled;
	bool m_useOctantMap;
	bool m_useBoxBoxDistance;
	bool m_fastCapsuleToCapsule;
	bool m_fastCapsuleToTriangle;
	bool m_fastDiscToTriangle;
	bool m_fastBoxToTriangle;
	BANK_ONLY(bool m_sweepFromSafe;)
	EARLY_FORCE_SOLVE_ONLY(bool m_pushCollision;)
   
    bool m_debugBool1;
    bool m_debugBool2;
    bool m_debugBool3;
    bool m_debugBool4;
    bool m_debugBool5;
    bool m_debugBool6;

    rage::sysSpinLockToken* m_pairConsumeToken;
    int* m_pairConsumeIndex;

	rage::phPool<rage::phManifold>::SpuInitParams m_manifoldPoolInitParams;
	rage::phPool<rage::phContact>::SpuInitParams m_contactPoolInitParams;
	rage::phPool<rage::phCompositePointers>::SpuInitParams m_compositePointersPoolInitParams;

	void* m_materialArray;
	rage::u32 m_numMaterials;
	rage::u32 m_materialStride;
	rage::u32 m_materialMask;
	const rage::phMaterialPair* m_MaterialOverridePairs;
	rage::u32 m_NumMaterialOverridePairs;

	rage::Mat34V* m_InstLastMatrices;
	rage::u8* m_LevelIdxToLastMatrixMap;
	void* m_LevelObjectDataArray;			// Really should be rage::phLevelNew::phObjectData but I don't want to #include levelnew.h and you can't forward declare nested classes.

	float m_TimeStep;
	int m_MinManifoldPointLifetime;

#if TRACK_COLLISION_TYPE_PAIRS
	rage::u32 (*m_typePairTable)[rage::phBound::NUM_BOUND_TYPES];
#endif

#if ALLOW_MID_PHASE_SWAP
	bool m_UseNewMidPhaseCollision;
#endif

#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE 
	rage::GJKCacheSystem * m_gjkCacheSystem_EA;
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE

#if __BANK && __PS3
	bool m_UseGJKCache;
	bool m_UseFramePersistentGJKCache;
#endif // __BANK && __PS3
};

#endif //PHBULLET_COLLISIONWORKUNIT_H