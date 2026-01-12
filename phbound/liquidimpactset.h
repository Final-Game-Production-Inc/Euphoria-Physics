//
// phbound/liquidimpactset.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_LIQUIDIMPACTSET_H
#define PHBOUND_LIQUIDIMPACTSET_H

#include "liquidimpactdata.h"

namespace rage {

	/*
	PURPOSE
		A class to collect the basic information required when two objects are intersecting and you want a liquid collision to happen as a result.
	*/
	class phLiquidImpactSet
	{
	public:
		inline phLiquidImpactSet();

		inline void AddImpactData(const phLiquidImpactData &kImpactToAdd);
		inline void RemoveImpactData(int impactIndex); // Potentially invalidates list order

		enum { kMaxLiquidImpactCount = 1024 };
		u16	m_NumImpacts;
		phLiquidImpactData m_LiquidImpactList[kMaxLiquidImpactCount];

		phBound *m_BoundA;				// The bound that is made into a liquid.
		phBound *m_BoundB;				// The non-liquid bound.

		const Matrix34 *m_CurrentA;
		const Matrix34 *m_CurrentB;
	protected:
	};


	phLiquidImpactSet::phLiquidImpactSet()
	{
		m_NumImpacts = 0;
	}


	void phLiquidImpactSet::AddImpactData(const phLiquidImpactData &kImpactToAdd)
	{
		FastAssert(m_NumImpacts < kMaxLiquidImpactCount);
		LIQUID_ASSERT_LEGIT(kImpactToAdd.m_ForcePos);

#if __DEV
		if(m_NumImpacts >= kMaxLiquidImpactCount)
		{
			return;
		}
#endif
		m_LiquidImpactList[m_NumImpacts] = kImpactToAdd;
		++m_NumImpacts;
	}

	void phLiquidImpactSet::RemoveImpactData(int impactIndex)
	{
		FastAssert(m_NumImpacts > 0);
		FastAssert(impactIndex >= 0 && impactIndex < m_NumImpacts);
#if __DEV
		if(m_NumImpacts <= 0 ||
			 impactIndex < 0 || impactIndex >= m_NumImpacts)
		{
			return;
		}
#endif
		// Assuming order doesn't matter for this container
		if(impactIndex < m_NumImpacts-1)
		{
			m_LiquidImpactList[impactIndex] = m_LiquidImpactList[m_NumImpacts-1];
		}
		--m_NumImpacts;
	}

} // namespace rage

#endif // end of #ifndef PHEFFECTS_LIQUIDIMPACTSET_H
