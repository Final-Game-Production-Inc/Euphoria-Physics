// 
// phsolver/applyimpulseandpushfixedrotationartandfix.cpp   
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

void ApplyImpulseAndPushFixedRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	int link = colliderA->GetLinkFromComponent(manifold.GetComponentA());

	Vec3V turnA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(link);

	// PUSH

	Vec3V deltaAng = cp.GetDepthV() * cp.GetWorldNormal() - turnA;
	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	cp.SetPreviousPush(turn);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrFixedRotationArtAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(link, turn.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
