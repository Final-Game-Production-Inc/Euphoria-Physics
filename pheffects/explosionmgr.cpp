//
// pheffects/explosionmgr.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "explosionmgr.h"

#include "instbehaviorexplosion.h"

#include "physics/colliderdispatch.h"
#include "physics/inst.h"

#include "vectormath/classes.h"


namespace rage {


phExplosionMgr::phExplosionMgr()
{
	m_MaxExplosions = 0;
	m_ExplosionInstances = NULL;
	m_ExplosionBehaviors = NULL;
}


phExplosionMgr::~phExplosionMgr()
{
}


void phExplosionMgr::Init(u16 MaxExplosions)
{
	m_MaxExplosions = MaxExplosions;

	m_ExplosionInstances = rage_new phInst[MaxExplosions];
	m_ExplosionBehaviors = rage_new phInstBehaviorExplosion[MaxExplosions];

	for(int ExplosionIndex = 0; ExplosionIndex < MaxExplosions; ++ExplosionIndex)
	{
		phInst &CurInstance = m_ExplosionInstances[ExplosionIndex];
		phInstBehaviorExplosion &CurBehavior = m_ExplosionBehaviors[ExplosionIndex];

		phBoundSphere * ExplosionBound = rage_new phBoundSphere;
		ExplosionBound->SetSphereRadius(SMALL_FLOAT);
		phArchetype * ExplosionArchetype = rage_new phArchetype;
		ExplosionArchetype->SetBound(ExplosionBound);
		ExplosionArchetype->SetMass(0.0f);
		ExplosionArchetype->SetAngInertia(Vector3(0.0f, 0.0f, 0.0f));
		CurInstance.SetArchetype(ExplosionArchetype);
		CurInstance.SetMatrix( Mat34V(V_IDENTITY) );
		ExplosionBound->Release();

		CurBehavior.SetInstance(CurInstance);
	}
}


void phExplosionMgr::Shutdown()
{
	for(int ExplosionIndex = 0; ExplosionIndex < m_MaxExplosions; ++ExplosionIndex)
	{
		phInst &CurInstance = m_ExplosionInstances[ExplosionIndex];
		CurInstance.SetArchetype(NULL);
	}

	delete [] m_ExplosionBehaviors;
	m_ExplosionBehaviors = NULL;

	delete [] m_ExplosionInstances;
	m_ExplosionInstances = NULL;
}


static int s_ExplosionCount = 0;


void phExplosionMgr::Update()
{
	for (int ExplosionIndex = 0; ExplosionIndex < m_MaxExplosions; ++ExplosionIndex)
	{
		phInst &CurInstance = m_ExplosionInstances[ExplosionIndex];
		phInstBehaviorExplosion &CurBehavior = m_ExplosionBehaviors[ExplosionIndex];

		if (CurInstance.IsInLevel())
		{
			if (!CurBehavior.IsActive())
			{
				// This explosion is not active, so remove it.
				--s_ExplosionCount;
				PHSIM->RemoveInstBehavior(m_ExplosionBehaviors[ExplosionIndex]);
				PHSIM->DeleteObject(m_ExplosionInstances[ExplosionIndex].GetLevelIndex());
			}
		}
	}
}


phInstBehaviorExplosion * phExplosionMgr::SpawnExplosion(Vector3::Vector3Param ExplosionPos, const phExplosionType *ExplosionType)
{
	int ExplosionIndex;
	for(ExplosionIndex = 0; ExplosionIndex < m_MaxExplosions && m_ExplosionInstances[ExplosionIndex].IsInLevel(); ++ExplosionIndex)
	{
	}

	if(ExplosionIndex < m_MaxExplosions)
	{
		++s_ExplosionCount;
		// These will adjust the instance, so we need to do them before the object gets added to the level.
		m_ExplosionBehaviors[ExplosionIndex].SetExplosionType(ExplosionType);
		m_ExplosionBehaviors[ExplosionIndex].Reset();
		m_ExplosionInstances[ExplosionIndex].SetPosition( VECTOR3_TO_VEC3V( Vector3(ExplosionPos) ) );
		PHSIM->AddInactiveObject(&m_ExplosionInstances[ExplosionIndex]);
		if (PHSIM->AddInstBehavior(m_ExplosionBehaviors[ExplosionIndex]))
		{
			int levelIndex = m_ExplosionInstances[ExplosionIndex].GetLevelIndex();
			PHLEVEL->SetInactiveCollidesAgainstInactive(levelIndex,true);
			PHLEVEL->FindAndAddOverlappingPairs(levelIndex);
		}
		else
		{
			// TODO: Rather than do this (try to add it, check for failure, and then clean up if it fails) I'd rather check first if it's going to work before anything gets done.
			PHSIM->DeleteObject(m_ExplosionInstances[ExplosionIndex].GetLevelIndex());
		}

		return &m_ExplosionBehaviors[ExplosionIndex];
	}

	// There weren't any explosions available to be used.
   	return NULL;
}

}	// namespace rage
