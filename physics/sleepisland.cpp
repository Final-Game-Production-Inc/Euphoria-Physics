// 
// physics/sleepisland.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "sleepisland.h"

#include "simulator.h"
#include "sleepmgr.h"

namespace rage {

phSleepIsland::phSleepIsland(int numObjects)
	: m_NumObjects(numObjects)
{
	m_Objects = (ObjectId*)((u8*)this + sizeof(phSleepIsland));
	sysMemSet(m_Objects,0xFF,sizeof(ObjectId)*numObjects);
}

phSleepIsland::~phSleepIsland()
{
	PHSLEEP->InvalidateSleepIsland(m_Id.index);
}

size_t phSleepIsland::ComputeSize(int numObjects)
{
	Assert(numObjects > 0);
	return sizeof(phSleepIsland) + sizeof(ObjectId) * numObjects;
}

void phSleepIsland::SetObject(int objectIndex, int levelIndex)
{
	TrapLT(objectIndex,0);
	TrapGE(objectIndex,m_NumObjects);
	ObjectId& objectId = m_Objects[objectIndex];
	objectId.levelIndex = (u16)levelIndex;
	objectId.generationId = PHLEVEL->GetGenerationID(levelIndex);
	PHLEVEL->SetSleepIsland(levelIndex, m_Id);
}

void phSleepIsland::SleepObjects()
{
	for (int objectIndex = 0; objectIndex < m_NumObjects; ++objectIndex)
	{
		u16 levelIndex = m_Objects[objectIndex].levelIndex;
		if (PHLEVEL->LegitLevelIndex(levelIndex) && PHLEVEL->IsActive(levelIndex))
		{
			PHSIM->DeactivateObject(levelIndex);
		}
	}
}

void phSleepIsland::WakeObjects()
{
	for (int objectIndex = 0; objectIndex < m_NumObjects; ++objectIndex)
	{
		const ObjectId& objectId = m_Objects[objectIndex];
		u16 levelIndex = objectId.levelIndex;
		u16 generationId = objectId.generationId;
	
		if (PHLEVEL->LegitLevelIndex(levelIndex) && PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, generationId) && PHLEVEL->IsInactive(levelIndex))
		{
			PHLEVEL->ResetSleepIsland(levelIndex);

			if (!PHLEVEL->GetInstance(levelIndex)->GetInstFlag(phInst::FLAG_DONT_WAKE_DUE_TO_ISLAND))
				PHSIM->ActivateObject(levelIndex);
		}
	}

	// Set the number of objects to zero, mainly so this island doesn't draw anymore
	m_NumObjects = 0;
}

#if __PFDRAW
EXT_PFD_DECLARE_ITEM(SleepIslands);

void phSleepIsland::ProfileDraw()
{

	Vec3V islandMin = Vec3V(V_FLT_MAX);
	Vec3V islandMax = -Vec3V(V_FLT_MAX);
	for (int objectIndex = 0; objectIndex < m_NumObjects; ++objectIndex)
	{
		const ObjectId& objectId = m_Objects[objectIndex];
		u16 levelIndex = objectId.levelIndex;
		u16 generationId = objectId.generationId;

		if (PHLEVEL->LegitLevelIndex(levelIndex) && PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex, generationId) && PHLEVEL->IsInactive(levelIndex))
		{
			phInst* inst = PHLEVEL->GetInstance(levelIndex);

			Vec3V obbExtents, obbCenter;
			inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(obbExtents, obbCenter);
			Mat34V_ConstRef current = inst->GetMatrix();
			const Vec3V aabbExtents = geomBoxes::ComputeAABBExtentsFromOBB(current.GetMat33ConstRef(), obbExtents);
			const Vec3V aabbCenter = Transform(current, obbCenter);

			const Vec3V maxs = aabbCenter + aabbExtents;
			const Vec3V mins = aabbCenter - aabbExtents;

			islandMax = Max(islandMax, maxs);
			islandMin = Min(islandMin, mins);
		}
	}

	grcDrawBox(RCC_VECTOR3(islandMin), RCC_VECTOR3(islandMax), PFD_SleepIslands.GetBaseColor());
}
#endif

} // namespace rage
