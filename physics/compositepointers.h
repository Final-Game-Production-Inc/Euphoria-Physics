// 
// physics/compositepointers.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_COMPOSITEPOINTERS_H 
#define PHYSICS_COMPOSITEPOINTERS_H 

#include "diag/trap.h"
#include "system/memops.h"

namespace rage {

class phManifold;
class sysDmaPlan;

class phCompositePointers
{
public:
	// 20 is the minimum without changing either the 128 byte alignment or padding of this class
	// Valid entries(with padding at 4) = 20+64x == 20, 84, 148, ...
	// Padding may be added at a rate of 6x to reduce the pair count by x
	static const int MAX_NUM_COLLIDING_COMPOSITE_PAIRS = 84;

	__forceinline void AboutToBeReleased() { }

	__forceinline phManifold* GetManifold(int index) const
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		return m_Manifolds[index];
	}

	__forceinline phManifold*& GetManifoldRef(int index)
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		return m_Manifolds[index];
	}

	__forceinline sysDmaPlan*& GetSecondManifoldDmaPlanRef()
	{
		return m_SecondManifoldDmaPlan;
	}

	__forceinline void SetSecondManifoldDmaPlan(sysDmaPlan* dmaPlan)
	{
		m_SecondManifoldDmaPlan = dmaPlan;
	}

	__forceinline u8 GetPairComponentA(int index) const
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		return m_Pairs[index << 1];
	}

	__forceinline u8 GetPairComponentB(int index) const
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		return m_Pairs[(index << 1) + 1];
	}

	__forceinline u8 GetPairComponent(int objectIndex, int manifoldIndex) const
	{
		TrapGE(manifoldIndex, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(manifoldIndex, 0);
		TrapGE(objectIndex, 2);
		TrapLT(objectIndex, 0);

		// Hey, for fun we could do a '|' instead of a '+' here.
		return m_Pairs[(manifoldIndex << 1) + objectIndex];
	}

	__forceinline void SetPairComponentA(int index, u8 component)
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		m_Pairs[index << 1] = component;
	}

	__forceinline void SetPairComponentB(int index, u8 component)
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		m_Pairs[(index << 1) + 1] = component;
	}

	__forceinline void GetPairComponent(int objectIndex, int manifoldIndex, u8 component)
	{
		TrapGE(manifoldIndex, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(manifoldIndex, 0);
		TrapGE(objectIndex, 2);
		TrapLT(objectIndex, 0);

		// Hey, for fun we could do a '|' instead of a '+' here.
		m_Pairs[(manifoldIndex << 1) + objectIndex] = component;
	}

	__forceinline void SetManifoldAndPairComponents(int index, phManifold *manifold, u8 componentA, u8 componentB)
	{
		TrapGE(index, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(index, 0);

		m_Manifolds[index] = manifold;
		m_Pairs[index << 1] = componentA;
		m_Pairs[(index << 1) + 1] = componentB;
	}

	__forceinline void CopyArrays(int numManifolds, u8* pairs, phManifold** manifolds)
	{
		TrapGT(numManifolds, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(numManifolds, 0);

		sysMemCpy(m_Manifolds, manifolds, sizeof(phManifold*) * numManifolds);
		sysMemCpy(m_Pairs, pairs, sizeof(u8) * 2 * numManifolds);
	}

	__forceinline void CopyArraysOut(int numManifolds, u8* pairs, phManifold** manifolds)
	{
		Assert(numManifolds >= 0);
		TrapGT(numManifolds, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(numManifolds, 0);

		sysMemCpy(manifolds, m_Manifolds, sizeof(phManifold*) * numManifolds);
		sysMemCpy(pairs, m_Pairs, sizeof(u8) * numManifolds * 2);
	}

	__forceinline void CopyManifoldsArray(int numManifolds, phManifold** manifolds)
	{
		TrapGT(numManifolds, MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
		TrapLT(numManifolds, 0);

		sysMemCpy(m_Manifolds, manifolds, sizeof(phManifold*) * numManifolds);
	}

	phManifold *FindManifoldByComponentIndices(int componentA, int componentB, int numManifolds) const
	{
		// Perform a binary search through the sorted list of manifolds to find out it the one we're looking for is here.
		const u8 * RESTRICT pairs = m_Pairs;
		const u32 targetCompositePair = (componentA << 8) + componentB;
		u32 lastCompositePair = 0xffffffff;	// Impossible value.

		int lowIndex = -1;
		int highIndex = numManifolds;
		while(highIndex - lowIndex > 1)
		{
			const int probeIndex = (lowIndex + highIndex) / 2;
			const u32 currentCompositePair = (pairs[2 * probeIndex] << 8) + pairs[2 * probeIndex + 1];
			if(currentCompositePair < targetCompositePair)
			{
				lowIndex = probeIndex;
			}
			else
			{
				highIndex = probeIndex;
				lastCompositePair = currentCompositePair;
			}
		}

		// highIndex is the index where we found the manifold, *iff* we found the manifold.
		if(lastCompositePair == targetCompositePair)
		{
			FastAssert(highIndex != numManifolds);
			return m_Manifolds[highIndex];
		}
		return NULL;
	}

	__forceinline void SwapComponentPairs(int numPairs)
	{
		u8 *componentPairs = m_Pairs;
		for(int i = numPairs; i > 0; --i)
		{
			// Swap *(componentPairs) and *(componentPairs + 1).  Not using SwapEm() here to avoid pulling in math/amath.h.
			u8 componentA = *componentPairs;
			*componentPairs = *(componentPairs + 1);
			*(componentPairs + 1) = componentA;
			componentPairs += 2;
		}
	}

	__forceinline void Reset()
	{
	}

	static const char* GetClassName()
	{
		return "phCompositePointers";
	}

private:
	// Every two adjacent elements of this array is a pair of components of the two composites involved in this collision
	u8 m_Pairs[MAX_NUM_COLLIDING_COMPOSITE_PAIRS * 2];

	// The "composite manifolds" this "root" manifold controls
	phManifold* m_Manifolds[MAX_NUM_COLLIDING_COMPOSITE_PAIRS];

	sysDmaPlan* m_SecondManifoldDmaPlan;

	datPadding<4, u8> m_Padding;

};

} // namespace rage

#endif // PHYSICS_COMPOSITEPOINTERS_H 
