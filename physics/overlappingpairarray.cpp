// 
// physics/overlappingpairarray.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "overlappingpairarray.h"

#include "inst.h"
#include "levelnew.h"

#include "profile/element.h"

namespace rage {

namespace SimulatorStats
{
	EXT_PF_TIMER(IslandBuilding);
}

using namespace SimulatorStats;

#if !__SPU
phOverlappingPairArray::phOverlappingPairArray(int maxPairCount, int maxObjectCountParam, int maxLevelIndexParam)
	: pairs(0, (maxPairCount + 15) & ~15)
{
	maxLevelIndex = maxLevelIndexParam = (maxLevelIndexParam + 15) & ~15;
	maxObjectCount = maxObjectCountParam = (maxObjectCountParam + 15) & ~15;

	int numIslandGrains = (maxPairCount + 15) & ~15;
	grains = reinterpret_cast<phIslandGrain*>(rage_new u8[sizeof(phIslandGrain)*numIslandGrains]); // Avoid constructor since it is expensive and we'll memset in ProcessIslands when necessary
	objects = rage_new phIslandGrain*[maxLevelIndexParam];
	objectsByIndex = rage_new u16[maxLevelIndexParam];
	objectsByIsland = rage_new u16[maxObjectCountParam];
	firstObjectInIsland = rage_new u16[maxObjectCountParam];

	Reset();
}

size_t phOverlappingPairArray::ComputeSizeForNewClass(int maxPairCount, int maxObjectCount, int maxLevelIndex)
{
	size_t neededSpace = sizeof(phOverlappingPairArray);
	neededSpace += maxPairCount * sizeof(phTaskCollisionPair); // pairs
	neededSpace += maxPairCount * sizeof(phIslandGrain); // grains
	neededSpace += maxLevelIndex * sizeof(phIslandGrain*); // objects

	// objectsByIndex	
	neededSpace += maxLevelIndex * sizeof(u16); 

	// objectsByIsland, firstObjectInIsland
	neededSpace += maxObjectCount * sizeof(u16) * 2;

	return neededSpace;
}

phOverlappingPairArray::~phOverlappingPairArray()
{
	delete [] firstObjectInIsland;
	delete [] objectsByIsland;
	delete [] objectsByIndex;
	delete [] objects;
	delete [] grains;
}

void phOverlappingPairArray::Reset()
{
	pairs.Resize(0);

	numCollisionPairs = -1;
	numIslands = -1;
	numObjects = -1;

#if __DEV
	sysMemSet(objects, 0xDE, maxLevelIndex * sizeof(phIslandGrain*));
	sysMemSet(grains, 0xDE, pairs.GetCapacity() * sizeof(phIslandGrain));
#endif // __DEV
}
#endif

class GenerateInstsFunctor
{
	phOverlappingPairArray* m_PairArray;

public:
	GenerateInstsFunctor(phOverlappingPairArray* pairArray)
		: m_PairArray(pairArray)
	{
	}

	__forceinline bool Process(phTaskCollisionPair& pair) const
	{
		m_PairArray->AddInstsToCurrentIsland(pair);

		return false;
	}

	__forceinline bool NextIsland() const
	{
		m_PairArray->NextIsland();

		return false;
	}
};

void phOverlappingPairArray::ComputeIslands(bool computeObjects, bool includeNonTouching)
{
	PF_FUNC(IslandBuilding);

	islands.Reset();

	numPairs = pairs.GetCount();

	// Reset the largest possible amount of the object array we will use
	sysMemSet(objects, 0, Min(maxLevelIndex, PHLEVEL->GetCurrentMaxLevelIndex() + 1) * sizeof(phIslandGrain*));

	// We know exactly how many grains we'll use so only reset that many
	sysMemSet(grains, 0, numPairs * sizeof(phIslandGrain));

	for (int i = 0; i < numPairs; ++i)
	{
		phManifold* rootManifold = pairs[i].manifold;

		if (rootManifold == NULL)
		{
			continue;
		}

		bool hasContacts = false;

		if (rootManifold->IsSelfCollision())
		{
			hasContacts = true;
		}
		else if (rootManifold->CompositeManifoldsEnabled())
		{
			hasContacts = rootManifold->GetNumCompositeManifolds() > 0;
		}
		else
		{
			hasContacts = rootManifold->GetNumContacts() > 0;
		}

		if (!hasContacts)
		{
			continue;
		}

		int levelIndexA = pairs[i].levelIndex1;
		phInst* instA = NULL;
		phLevelBase::eObjectState stateA = phLevelBase::OBJECTSTATE_NONEXISTENT;
		if (PHLEVEL->IsInLevel(levelIndexA))
		{
			instA = PHLEVEL->GetInstance(levelIndexA);
			stateA = PHLEVEL->GetState(levelIndexA);
		}

		int levelIndexB = pairs[i].levelIndex2;
		phInst* instB = NULL;
		phLevelBase::eObjectState stateB = phLevelBase::OBJECTSTATE_NONEXISTENT;
		if (PHLEVEL->IsInLevel(levelIndexB))
		{
			instB = PHLEVEL->GetInstance(levelIndexB);
			stateB = PHLEVEL->GetState(levelIndexB);
		}

		bool connectA = stateA == phLevelBase::OBJECTSTATE_ACTIVE || (stateA == phLevelBase::OBJECTSTATE_INACTIVE && instA->IsBreakable(instB));
		bool connectB = stateB == phLevelBase::OBJECTSTATE_ACTIVE || (stateB == phLevelBase::OBJECTSTATE_INACTIVE && instB->IsBreakable(instA));

		if (!connectA && !connectB)
		{
			// If neither one of the instances should be connected to an island (e.g. two non-breakable inactive instances) then don't even add the grain
			continue;
		}

		phIslandGrain* representative = &grains[i];

		TrapGE(i, pairs.GetCapacity());
		islands.Insert(representative);
		
		if (instA)
		{
			if (connectA)
			{
				// See if this object already has a representative, so the object array will hold a value
				TrapLT(levelIndexA, 0);
				TrapGE(levelIndexA, maxLevelIndex);
				if (phIslandGrain* repGrain = objects[levelIndexA])
				{
					repGrain = islands.Find(repGrain);
					representative = islands.UnionFound(representative, repGrain);
				}
			}

			TrapLT(levelIndexA, 0);
			TrapGE(levelIndexA, maxLevelIndex);
			objects[levelIndexA] = representative;
		}

		if (instB)
		{
			if (connectB)
			{
				TrapLT(levelIndexB, 0);
				TrapGE(levelIndexB, maxLevelIndex);
				if (phIslandGrain* repGrain = objects[levelIndexB])
				{
					repGrain = islands.Find(repGrain);
					representative = islands.UnionFound(representative, repGrain);

					// If we change the island rep, we need to tell objects A also
					if (levelIndexA != phInst::INVALID_INDEX)
					{
						TrapLT(levelIndexA, 0);
						TrapGE(levelIndexA, maxLevelIndex);
						objects[levelIndexA] = representative;
					}
				}
			}

			TrapLT(levelIndexB, 0);
			TrapGE(levelIndexB, maxLevelIndex);
			objects[levelIndexB] = representative;
		}
	}

	numIslands = 0;
	numObjects = 0;

	if (computeObjects)
	{
		int currentMaxLevelIndex = PHLEVEL->GetCurrentMaxLevelIndex();
		currentMaxLevelIndex = Max(currentMaxLevelIndex, maxLevelIndex);

		if (currentMaxLevelIndex >= 0)
		{
			sysMemSet(objectsByIndex, 0xff, sizeof(u16) * currentMaxLevelIndex);

			IterateIslandPairs(GenerateInstsFunctor(this));

			if (includeNonTouching)
			{
				// Insert all the active insts that are not in pairs into their own islands, otherwise they won't sleep
				const phLevelNew *physicsLevel = PHLEVEL;
				for(int activeIndex = physicsLevel->GetNumActive() - 1; activeIndex >= 0; --activeIndex)
				{
					const int levelIndex = physicsLevel->GetActiveLevelIndex(activeIndex);
					Assert(physicsLevel->IsActive(levelIndex));

					TrapLT(levelIndex, 0);
					TrapGE(levelIndex, maxLevelIndex);
					if(objectsByIndex[levelIndex] == 0xffff)
					{
						objectsByIndex[levelIndex] = (u16)numObjects;

						TrapLT(numObjects, 0);
						TrapGE(numObjects, maxObjectCount);
						objectsByIsland[numObjects++] = (u16)levelIndex;

						if (numIslands == 0 || numObjects > firstObjectInIsland[numIslands - 1])
						{
							TrapLT(numIslands, 0);
							TrapGE(numIslands, maxObjectCount);
							firstObjectInIsland[numIslands++] = (u16)numObjects;
						}
					}
				}
			}
		}
	}
}

bool KeepPair(phTaskCollisionPair& pair)
{
	if(phManifold* manifold = pair.manifold)
	{
		if(manifold->IsSelfCollision())
		{
			// Always keep self collisions as they're necessary for joint limits
			return true;
		}
		else if(manifold->CompositeManifoldsEnabled())
		{
			// Peds and Vehicles tend to delete all contacts with the ground and handle resolution internally so checking the sub-manifolds
			//   can get rid of a lot of manifolds
			for(int compositeManifoldIndex = 0; compositeManifoldIndex < manifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
			{
				if(manifold->GetCompositeManifold(compositeManifoldIndex)->GetNumContacts() > 0)
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			// non-composite collision or constraint
			return manifold->GetNumContacts() > 0;
		}
	}
	else
	{
		return false;
	}
}

void phOverlappingPairArray::RemoveEmptyManifolds()
{ 
	int prunedPairs = 0;
	int prunedCollisionPairs = 0;
	int writePairIndex = 0;
	for(int pairIndex = 0; pairIndex < pairs.GetCount(); ++pairIndex)
	{
		if(KeepPair(pairs[pairIndex]))
		{
			pairs[writePairIndex] = pairs[pairIndex];
			++writePairIndex;
		}
		else
		{
			++prunedPairs;
			if(pairIndex < numCollisionPairs)
			{
				++prunedCollisionPairs;
			}
		}
	}

	pairs.Resize(pairs.GetCount() - prunedPairs);
	numCollisionPairs -= prunedCollisionPairs;
}

} // namespace rage
