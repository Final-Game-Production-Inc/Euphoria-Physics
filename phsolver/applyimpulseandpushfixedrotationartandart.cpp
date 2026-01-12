// 
// phsolver/applyimpulseandpushfixedrotationartandart.cpp  
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

void ApplyImpulseAndPushFixedRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());

	Mat33V angInertia = manifold.GetConstraintMatrix();

	Vec3V turnA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	Vec3V turnB = CALL_MEMBER_FN(*bodyB, GetAngularVelocity)(linkB);

	// PUSH

	Vec3V deltaAng = turnA - turnB - cp.GetDepthV() * cp.GetWorldNormal();
	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	cp.SetPreviousPush(turn);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrRotationArtAndArt");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
	Vec3V massInvScaleB = Vec3VFromF32(manifold.GetMassInvScaleB());

	turnA               = turn       * massInvScaleA;
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, turnA.GetIntrin128());

	turnB               = turn       * massInvScaleB;
	CALL_MEMBER_FN(*bodyB, ApplyAngImpulse)(linkB, (-turnB).GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
