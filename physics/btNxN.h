//
// physics/btNxN.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef N_X_N_H
#define N_X_N_H

#include "physics/btImmediateModeBroadphase.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "vector/vector3.h"

namespace rage
{

	class btNxN : public btImmediateModeBroadphase
	{

	public:
		btNxN( u16 maxHandles, u16 maxPairs, btOverlappingPairCache *existingPairCache = NULL ) 
			: btImmediateModeBroadphase( maxHandles, maxPairs, existingPairCache )
		{
			m_IsIncremental = false;
		}

		virtual ~btNxN(){}

		void pruneActiveOverlappingPairs();

	};

}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif
