// 
// phsolver/applyimpulseandpushpivotrotationartandmov.cpp  
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

void ApplyImpulseAndPushPivotRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();
	Assert(!colliderB->IsArticulated());

	Assert(manifold.GetNumContacts() == 1);
	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	/// Vec3V worldNormalA = contactPoint.GetWorldNormal();

	Mat33V angInertia = manifold.GetConstraintMatrix();
	Vec3V axis = contactPoint.GetLocalPosA();

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());

	Vec3V turnA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	// PUSH

	Vec3V turnB = colliderB->GetTurn();
	Vec3V deltaAng = turnA - turnB - contactPoint.GetDepthV() * contactPoint.GetWorldNormal();
	Vec3V turnProjection = Dot(deltaAng, axis) * axis;
	Vec3V clampedDeltaAng = deltaAng - turnProjection;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, clampedDeltaAng));

	contactPoint.SetPreviousPush(turn);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrPivotRotationArtAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(contactPoint.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
	turnA               = turn       * massInvScaleA;
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, turnA.GetIntrin128());

	turnB -= Multiply(manifold.GetInertiaInvB(), turn);
	colliderB->SetTurn(turnB.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
