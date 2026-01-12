// 
// physics/overlappingpairarray.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_OVERLAPPINGPAIRARRAY_H 
#define PHYSICS_OVERLAPPINGPAIRARRAY_H 

#include "inst.h"
#include "manifold.h"

#include "atl/inunionfind.h"
#include "math/amath.h"
#include "system/new.h"


namespace rage {

class phCollider;
class phInst;
class phManifold;

struct phTaskCollisionPair
{
	u16 levelIndex1;
	u16 generationId1;
	u16 levelIndex2;
	u16 generationId2;
	phManifold* manifold;
};

inline bool operator<(const phTaskCollisionPair& a, const phTaskCollisionPair& b) 
{ 
    return a.levelIndex1 < b.levelIndex1 || 
        (a.levelIndex1 == b.levelIndex1 &&
        a.levelIndex2 < b.levelIndex2); 
}

struct phIslandGrain
{
	atInUnionFindLink<phIslandGrain> link;

	void Reset()
	{
		::new (this) phIslandGrain;
	}
};

typedef atInUnionFind<phIslandGrain, &phIslandGrain::link> phIslandFind;

struct phOverlappingPairArray
{
#if __SPU
	phOverlappingPairArray() { }
	~phOverlappingPairArray() { }
#else
    phOverlappingPairArray(int maxPairCount, int maxObjectCount, int maxLevelIndex);
    ~phOverlappingPairArray();
	static size_t ComputeSizeForNewClass(int maxPairCount, int maxObjectCount, int maxLevelIndex);

	void Reset();
#endif

	// PURPOSE: Compute the simulation islands within the overlapping pair array
	// PARAMS:
	//    computeObjects - Do compute the unique objects (in the "objectsByIsland", "firstObjectInIsland", and "objectsByIndex" members) in this pair array (not used for push pairs)
	//    includeNonTouching - Do we include all instances not in the pair array as separate islands? We turn this off for push collision solves (not used for breaking pairs)
	void ComputeIslands(bool computeObjects, bool includeNonTouching);

	// PURPOSE: Remove all empty manifolds from the pair array
	void RemoveEmptyManifolds();

#if __ASSERT
	bool AreIslandsComputed()
	{
		return numObjects != -1 && numPairs <= pairs.GetCount() && numIslands != -1;
	}
#endif // __ASSERT

	template <class T>
	void IterateIslandPairs(const T& callback)
	{
		Assert(AreIslandsComputed());
		phIslandFind::Iterator firstIsland(islands.GetFirstClass());

		phIslandFind::Iterator nextIsland;
		for (phIslandFind::Iterator it(firstIsland); it.Item(); it = nextIsland)
		{
			nextIsland = it.GetNextClass();

			for (phIslandFind::Iterator islandIt = it; islandIt != nextIsland; islandIt.Next())
			{
				phIslandGrain* grain = islandIt.Item();
				int grainIndex = (int)(grain - grains);
				phTaskCollisionPair& pair = pairs[grainIndex];

				bool skipToNextIsland = callback.Process(pair);

				if (skipToNextIsland)
				{
					islandIt = nextIsland;
					break;
				}
			}

			callback.NextIsland();
		}
	}

	template <class T>
	void IterateIslandInsts(const T& callback)
	{
		Assert(AreIslandsComputed());
		int islandIndex = 0;
		int lastIslandFirstObject = 0;
		for (int objectIndex = 0; objectIndex < numObjects; ++objectIndex)
		{
			int skipToNextIsland = callback.Process(objectsByIsland[objectIndex], objectIndex - lastIslandFirstObject);

			if (skipToNextIsland || objectIndex == firstObjectInIsland[islandIndex] - 1)
			{
				bool repeatIsland = callback.NextIsland(firstObjectInIsland[islandIndex] - lastIslandFirstObject);

				if (repeatIsland)
				{
					if (islandIndex <= 0)
					{
						objectIndex = -1;
					}
					else
					{
						objectIndex = firstObjectInIsland[islandIndex - 1] - 1;
					}
				}
				else
				{
					lastIslandFirstObject = firstObjectInIsland[islandIndex];
					objectIndex = lastIslandFirstObject - 1;
					islandIndex++;
				}
			}
		}

		Assert(islandIndex == numIslands);
	}

	void AddInstsToCurrentIsland(phTaskCollisionPair& pair);
	void NextIsland();

	typedef atArray<phTaskCollisionPair, 0, u32> PairArray;
    PairArray pairs;
	int maxLevelIndex;
	int maxObjectCount;
	int numCollisionPairs;

	phIslandFind islands;

	// Indexed by level index, indicating the index within each island occupied by each object
	u16* objectsByIndex;

	// While iterating through objectByIsland, these indicate where the borders are between islands
	u16* firstObjectInIsland;

	// Level indices, separated into sections by island
	u16* objectsByIsland;

private:
	phIslandGrain* grains;
	phIslandGrain** objects;

public:
	int numIslands;
	int numObjects;
	int numPairs;
	bool allPairsReady;
} ;

__forceinline void phOverlappingPairArray::AddInstsToCurrentIsland(phTaskCollisionPair& pair)
{
	int levelIndexA = pair.levelIndex1;
	if (levelIndexA != phInst::INVALID_INDEX)
	{
		// The object pointer here is used an indication that this object has not been recorded into an island
		// We clear it afterwards, with the bonus that then it will be all zero for the next iteration.
		TrapLT(levelIndexA, 0);
		TrapGE(levelIndexA, maxLevelIndex);
		if (objects[levelIndexA] != NULL)
		{
			objects[levelIndexA] = NULL;
			objectsByIndex[levelIndexA] = (u16)numObjects;

			TrapLT(numObjects, 0);
			TrapGE(numObjects, maxObjectCount);
			objectsByIsland[numObjects++] = (u16)levelIndexA;
		}
	}

	int levelIndexB = pair.levelIndex2;
	if (levelIndexB != phInst::INVALID_INDEX)
	{
		TrapLT(levelIndexB, 0);
		TrapGE(levelIndexB, maxLevelIndex);
		if (objects[levelIndexB] != NULL)
		{
			objects[levelIndexB] = NULL;
			objectsByIndex[levelIndexB] = (u16)numObjects;

			TrapLT(numObjects, 0);
			TrapGE(numObjects, maxObjectCount);
			objectsByIsland[numObjects++] = (u16)levelIndexB;
		}
	}
}

__forceinline void phOverlappingPairArray::NextIsland()
{
	TrapLT(numIslands, 0);
	TrapGE(numIslands, maxObjectCount);
	firstObjectInIsland[numIslands++] = (u16)numObjects;
}

} // namespace rage

#endif // PHYSICS_OVERLAPPINGPAIRARRAY_H 