//
// physics/btAxisSweep1.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef AXIS_SWEEP_1_H
#define AXIS_SWEEP_1_H

#include "physics/btImmediateModeBroadphase.h"

#if ENABLE_UNUSED_PHYSICS_CODE

namespace rage
{

class btAxisSweep1 : public btImmediateModeBroadphase
{

public:
	btAxisSweep1( u16 maxHandles, u16 maxPairs, btOverlappingPairCache *existingPairCache ) : 
	  btImmediateModeBroadphase( maxHandles, maxPairs, existingPairCache )
	{
		m_IsIncremental = false;
	}

	virtual ~btAxisSweep1(){}

	void pruneActiveOverlappingPairs();

};

}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif
