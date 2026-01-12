//
// physics/btImmediateModeBroadphase.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "physics/btImmediateModeBroadphase.h"

#if ENABLE_UNUSED_PHYSICS_CODE

namespace rage
{

void btImmediateModeBroadphase::mergeNewPairs( btBroadphasePair *newPairs, int nPairs )
{
	if( nPairs + m_nRetainedPair < m_pairCache->GetMaxPairs() )
	{

		int iRetained;
		for( iRetained = 0; iRetained < m_nRetainedPair; iRetained++ )
		{
			newPairs[nPairs] = m_retainedPair[iRetained];
			nPairs++;
		}
	}
	m_pairCache->mergeNewPairs( newPairs, nPairs );

	m_nRetainedPair = 0;
}

}

#endif // ENABLE_UNUSED_PHYSICS_CODE
