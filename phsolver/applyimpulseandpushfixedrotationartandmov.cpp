// 
// phsolver/applyimpulseandpushfixedrotationartandmov.cpp   
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

void ApplyImpulseAndPushFixedRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	phCollider* colliderB = manifold.GetColliderB();
#if !EARLY_FORCE_SOLVE
	Vec3V angVelB = colliderB->GetAngVelocity();
#endif // !EARLY_FORCE_SOLVE
	Vec3V turnB = colliderB->GetTurn();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());

	Mat33V angInertia = manifold.GetConstraintMatrix();

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());

	Vec3V turnA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	// PUSH

	Vec3V deltaAng = turnA - turnB - cp.GetDepthV() * cp.GetWorldNormal();
	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	cp.SetPreviousPush(turn);

	turnB -= Multiply(manifold.GetInertiaInvB(), turn);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrFixedRotationArtAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
	turnA               = turn       * massInvScaleA;
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, turnA.GetIntrin128());

	colliderB->SetTurn(turnB.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}


} // namespace rage
