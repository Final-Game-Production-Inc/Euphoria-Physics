//
// pheffects/explosionmgr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_EXPLOSIONMGR_H
#define PHEFFECTS_EXPLOSIONMGR_H

#include "vector/vector3.h"

namespace rage {

	class phInst;
	class phExplosionType;
	class phInstBehaviorExplosion;

	////////////////////////////////////////////////////////////////
	// phExplosionMgr

	// PURPOSE
	//   Class that handles most of the mundane aspects of using phInstBehaviorExplosion's to create explosions, such as pooling, monitoring, and removal.
	// NOTES
	//   This class does *not* handle the updating of the phInstBehaviorExplosion objects.  That is handled (for all phInstBehavior's) by phSimulator.

	class phExplosionMgr
	{
	public:
		phExplosionMgr();
		~phExplosionMgr();

		void Init(u16 MaxExplosions);
		void Shutdown();

		void Update();

		phInstBehaviorExplosion * SpawnExplosion(Vector3::Vector3Param ExplosionPos, const phExplosionType *ExplosionType);

	protected:
		u16 m_MaxExplosions;

		phInst * m_ExplosionInstances;
		phInstBehaviorExplosion * m_ExplosionBehaviors;
	};

} // namespace rage

#endif
