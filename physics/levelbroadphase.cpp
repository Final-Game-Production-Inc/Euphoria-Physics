// 
// physics/levelbroadphase.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#include "levelbroadphase.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "collider.h"
#include "iterator.h"
#include "levelnew.h"
#include "simulator.h"
#include "sleep.h"

#include <algorithm>

// for the scratchpad.... should be broken out into it's own class with access protocols and junk.
#include "phsolver/contactmgr.h"
#include "physics/simulator.h"

#include "system/miniheap.h"
#include "system/taskheader.h"
#include "system/task.h"
#include "vectormath/classes.h"

namespace rage {


void phLevelBroadPhase::getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const
{

	Assert( m_nRetainedPair == 0 );

	u16 iHandle;
	u16 iHandleOut;
	for( iHandle = 0, iHandleOut = 0; iHandle < m_highestHandle+1; iHandle++ )
	{
		if( m_handleUsed[iHandle] == 1 )
		{
			phInst *inst = m_Level->GetInstance(iHandle);
			Assert( inst );

			Vec3V obbExtents, obbCenter;
			inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(obbExtents, obbCenter);
			Mat34V tempMat = inst->GetMatrix();
			const Vec3V aabbExtents = geomBoxes::ComputeAABBExtentsFromOBB(tempMat.GetMat33ConstRef(), obbExtents);
			const Vec3V aabbCenter = Transform(tempMat, obbCenter);

			const Vec3V maxs = aabbCenter + aabbExtents;
			const Vec3V mins = aabbCenter - aabbExtents;

			(*aabbMin)[iHandleOut] = RCC_VECTOR3(mins);
			(*aabbMax)[iHandleOut] = RCC_VECTOR3(maxs);
			(*pOwner)[iHandleOut] = iHandle;
			iHandleOut++;
		}
	}

	nHandles = iHandleOut;
}


u16 phLevelBroadPhase::getNumHandles() const
{
	return m_nHandle;
}

void LevelBroadphaseTask( ::rage::sysTaskParameters & );

void LevelBroadphaseTask(sysTaskParameters& params)
{
	u32 numTestInstanes = params.UserData[0].asInt;
	u16* testInstances = (u16*)params.UserData[1].asPtr;
	u32* currentTestInstance = (u32*)params.UserData[2].asPtr;
	u32* numPairs = (u32*)params.UserData[3].asPtr;
	btBroadphasePair* newPairs = (btBroadphasePair*)params.UserData[4].asPtr;
	const u32 maxNumPairs = params.UserData[5].asInt;

	u32 listIndex;
	while ((listIndex = sysInterlockedIncrement(currentTestInstance) - 1) < numTestInstanes)
	{
		u16 levelIndex = testInstances[listIndex];
		phCollider* collider1 = PHSIM->GetCollider(levelIndex);

		phInst* inst1 = PHLEVEL->GetInstance(levelIndex);
		phArchetype* archetype1 = inst1->GetArchetype();
		Assert(archetype1);
		u32 typeFlags = archetype1->GetTypeFlags();
		u32 includeFlags = archetype1->GetIncludeFlags();
		const phBound* bound1 = archetype1->GetBound();
		Assert(bound1);

		// Get the radius of the object's enclosing sphere, and the sphere's center in world coordinates.
		Vector3 center1;
		Mat34V tempMat;
		tempMat = inst1->GetMatrix();
        center1 = VEC3V_TO_VECTOR3(bound1->GetWorldCentroid(tempMat));
        Vector3 radius1 = SCALARV_TO_VECTOR3( bound1->GetRadiusAroundCentroidV() );

        // Find the motion since the last frame, and use it to change the bounding sphere if it is large enough.
		if (collider1)
		{
			tempMat = inst1->GetMatrix();
			geomSpheres::ExpandRadiusFromMotion((reinterpret_cast<Matrix34*>(&tempMat))->d,RCC_VECTOR3(collider1->GetLastInstanceMatrix().GetCol3ConstRef()),radius1,center1);
		}

		Vector3 halfWidth1(Vector3::ZeroType), boxCenter1;
        bound1->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(halfWidth1), RC_VEC3V(boxCenter1));

		if (collider1)
		{
			tempMat = inst1->GetMatrix();
			Mat34V_ConstRef lastMat = collider1->GetLastInstanceMatrix();
        	geomBoxes::ExpandOBBFromMotion( tempMat, lastMat, RC_VEC3V(halfWidth1), RC_VEC3V(boxCenter1) );
		}

        Matrix34 aMat;
		tempMat = inst1->GetMatrix();
        aMat.Set3x3( *(reinterpret_cast<Matrix34*>(&tempMat)) );
		tempMat = inst1->GetMatrix();
        (reinterpret_cast<Matrix34*>(&tempMat))->Transform(boxCenter1, aMat.d);

		// Create a list of objects in the physics level octree nodes that touch this active object's sphere.
		//PF_START(BroadphaseLevel);

#if ENABLE_PHYSICS_LOCK
		phIterator it(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else	// ENABLE_PHYSICS_LOCK
		phIterator it;
#endif	// ENABLE_PHYSICS_LOCK
		it.InitCull_Sphere(center1, radius1.x);
		it.SetIncludeFlags(includeFlags);
		it.SetTypeFlags(typeFlags);
		u16 objectLevelIndex2 = it.GetFirstLevelIndex(PHLEVEL);
		while (objectLevelIndex2 != (u16)(-1))
		{
			//PF_STOP(BroadphaseLevel);

			// See if the culled object should be tested for collision with this active object.
			// Set the collision test to occur if the culled object is not active.
			const phCollider* collider2 = NULL;
			bool testCollision = true;
			bool object2IsActive = PHLEVEL->IsActive(objectLevelIndex2);
			if (object2IsActive)
			{
				// The culled object is active. Set a collision test to occur if the culled active object is sleeping, or if it has a
				// higher level index (so that active objects do not do collision tests with each other twice per frame).
				collider2 = PHSIM->GetActiveCollider(objectLevelIndex2);
				Assert(collider2);
				testCollision = objectLevelIndex2 > inst1->GetLevelIndex();
			}

			if (testCollision)
			{
				// This culled object is not active, or it is active active and sleeping, or it is active and not sleeping and it has a higher level index.
				// The level index test prevents colliders from hitting each other twice in the same frame. A greater than test is used so that each collider
				// will handle collisions with all following colliders, so that the contact manager can easily group contacts by collider. Active object index
				// numbers are sorted so that if the level index is greater, then the active index is also greater.
				// See if the two objects' archetype flags match.
				phInst* inst2 = PHLEVEL->GetInstance(objectLevelIndex2);
				const phArchetype* archetype2 = inst2->GetArchetype();
				Assert(archetype2);
				Assert(archetype2->GetBound() != NULL);
				const phBound* bound2 = archetype2->GetBound();

				Vector3 halfWidth2(ORIGIN), boxCenter2;
                bound2->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(halfWidth2), RC_VEC3V(boxCenter2));

				if (object2IsActive)
				{
					// Find the motion since the last frame, and use it to change the bounding sphere if it is large enough.
					Assert(collider2);
					Mat34V_ConstRef tempMat = inst2->GetMatrix();
					Mat34V_ConstRef lastMat = collider2->GetLastInstanceMatrix();
                    geomBoxes::ExpandOBBFromMotion( tempMat, lastMat, RC_VEC3V(halfWidth2), RC_VEC3V(boxCenter2) );
				}

				Matrix34 bMat;
				Mat34V tempMat = inst2->GetMatrix();
                bMat.Set3x3( *(reinterpret_cast<Matrix34*>(&tempMat)) );
                (reinterpret_cast<Matrix34*>(&tempMat))->Transform(boxCenter2, bMat.d);

				// Compute rotation matrix expressing b in a's coordinate frame
				Matrix34 matrix;
				matrix.DotTranspose(aMat, bMat);

				halfWidth1.And(VEC3_ANDW);
				halfWidth2.And(VEC3_ANDW);

				if( geomBoxes::TestBoxToBoxOBBFaces (halfWidth1, halfWidth2, matrix) )
				{
					// The bounding spheres overlap, so do a collision test.
					//CollisionIntersection(inst1, inst2);
					//Assert(m_numInCache < m_OverlappingPairArray->maxNumPairs);

					u32 newPairIndex = sysInterlockedIncrement(numPairs) - 1;

					// If we've filled up all of the available pair slots just finish now.
					if(newPairIndex >= maxNumPairs)
					{
						return;
					}

					u16 generationIdA = PHLEVEL->GetGenerationID(levelIndex);
					u16 generationIdB = PHLEVEL->GetGenerationID(objectLevelIndex2);
 					newPairs[newPairIndex].Set((u16)levelIndex, generationIdA, (u16)objectLevelIndex2, generationIdB);
					newPairs[newPairIndex].SetManifold(NULL);
				}
			}

			//PF_START(BroadphaseLevel);

			// Get the next culled instance.
			objectLevelIndex2 = it.GetNextLevelIndex(PHLEVEL);
		}

		//PF_STOP(BroadphaseLevel);
	}
}


void phLevelBroadPhase::pruneActiveOverlappingPairs()
{
	// TODO: Remove the phSimulator pointer from the pruneOverlappingPairs function since it's not used anywhere anymore.

	sysMemStartMiniHeap(PHSIM->GetContactMgr()->GetScratchpad(), PHSIM->GetContactMgr()->GetScratchpadSize());

	const u32 kMaxNumPairs = m_pairCache->GetMaxPairs();
	btBroadphasePair* newPairs = rage_new btBroadphasePair[ kMaxNumPairs ];
	u32 numPairs = 0;
	u16* testInstances = rage_new u16[ kMaxNumPairs ];
	u32 numTestInstances = 0;

	sysMemEndMiniHeap();

    // Loop over all the active objects.
    for (int activeIndex = m_Level->GetFirstActiveIndex(); activeIndex != phInst::INVALID_INDEX && numTestInstances < kMaxNumPairs; activeIndex = m_Level->GetNextActiveIndex())
    {
		testInstances[numTestInstances++] = (u16)activeIndex;
    }

	Assert(numTestInstances <= kMaxNumPairs);

	sysTaskParameters params;
	sysMemZeroWords<sizeof(sysTaskParameters)/4>(&params);

	u32 currentTestInstance = 0;
	params.UserData[0].asInt = numTestInstances;
	params.UserData[1].asPtr = testInstances;
	params.UserData[2].asPtr = &currentTestInstance;
	params.UserData[3].asPtr = &numPairs;
	params.UserData[4].asPtr = newPairs;
	params.UserData[5].asInt = kMaxNumPairs;

#if __WIN32
	if (phConfig::GetCollisionWorkerThreadCount()>0)
	{
		sysTaskHandle* taskHandles = Alloca(sysTaskHandle, phConfig::GetCollisionWorkerThreadCount());

		int numWorkerThreads = phConfig::GetCollisionWorkerThreadCount();

		for (int taskIndex = 0; taskIndex < numWorkerThreads; ++taskIndex)
		{
			taskHandles[taskIndex] = sysTaskManager::Create(TASK_INTERFACE(LevelBroadphaseTask), params, phConfig::GetCollisionTaskScheduler());
		}

		if (phConfig::GetCollisionMainThreadAlsoWorks())
		{
			LevelBroadphaseTask(params);
		}

		if (numWorkerThreads > 0)
		{
			sysTaskManager::WaitMultiple(numWorkerThreads, taskHandles);
		}
	}
	else
#endif
	{
		LevelBroadphaseTask(params);
	}

	// LevelBroadphaseTask will increment numPairs (but not write 
	numPairs = Min(numPairs, kMaxNumPairs);

//	if( numPairs + m_nRetainedPair < m_pairCache->GetMaxPairs() )
	{

		int iRetained;
		for( iRetained = 0; iRetained < m_nRetainedPair && numPairs < m_pairCache->GetMaxPairs(); iRetained++ )
		{
			newPairs[numPairs++] = m_retainedPair[iRetained];
		}
	}

	m_pairCache->mergeNewPairs( newPairs, numPairs );

	m_nRetainedPair = 0;
}

} // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE
