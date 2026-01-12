// 
// phsolver/applyimpulseandpushfixedpointartandfix.cpp  
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseAndPushFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V Kinv = manifold.GetConstraintMatrix();

	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
	int link = colliderA->GetLinkFromComponent(manifold.GetComponentA());

	Vec3V localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(link,localPosA.GetIntrin128());

	// PUSH

	Vec3V deltaPos = cp.GetWorldPosB() - cp.GetWorldPosA() - localPushA;

	Vec3V push = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(Kinv, deltaPos));

	cp.SetPreviousPush(push);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrFixedPointArtAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, push, localPosA);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
