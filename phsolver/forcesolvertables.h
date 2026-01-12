// 
// physics/forcesolvertables.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_FORCESOLVERTABLES_H 
#define PHYSICS_FORCESOLVERTABLES_H 

#include "forcesolver.h"

#include "physics/collider.h"

namespace rage {

extern phForceSolver::ConstraintTable g_UpdateContactsConstraintTable;
extern phForceSolver::ConstraintTable g_PreResponseConstraintTable;
extern phForceSolver::ConstraintTable g_ApplyImpulseConstraintTable;
extern phForceSolver::ConstraintTable g_ApplyImpulseAndPushConstraintTable;

#if __PS3
extern phForceSolver::SpuConstraintTable g_UpdateContactsSpuConstraintTable;
extern phForceSolver::SpuConstraintTable g_PreResponseSpuConstraintTable;
extern phForceSolver::SpuConstraintTable g_ApplyImpulseSpuConstraintTable;
extern phForceSolver::SpuConstraintTable g_ApplyImpulseAndPushSpuConstraintTable;
#endif

inline static int GetColliderTypeIndex (const phCollider* collider)
{
	return (collider ? (collider->IsArticulated() ? 2 : 1) : 0);
}

} // namespace rage

#endif // PHYSICS_FORCESOLVERTABLES_H 
