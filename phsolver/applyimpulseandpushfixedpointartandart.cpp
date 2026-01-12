// 
// phsolver/applyimpulseandpushfixedpointartandart.cpp 
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

void ApplyImpulseAndPushFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
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
	Mat33V Kinv = manifold.GetConstraintMatrix();

#if USE_CENTRE_POINT_FOR_ART_CONSTRAINTS
	Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
	localPos*=ScalarV(V_HALF);
	Vec3V localPosA = localPos - colliderA->GetPosition();
	Vec3V localPosB = localPos - colliderB->GetPosition();
#else
	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
	Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());

	Vec3V localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());
	Vec3V localPushB = CALL_MEMBER_FN(*bodyB, GetLocalVelocity)(linkB,localPosB.GetIntrin128());

	// PUSH

	Vec3V relativePush = localPushA-localPushB;

	Vec3V deltaPos = cp.GetWorldPosB() - cp.GetWorldPosA() - relativePush;
	Vec3V push = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(Kinv, deltaPos));

	cp.SetPreviousPush(push);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrFixedPointArtAndArt");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	Vec3V pushA    = push    * Vec3VFromF32(manifold.GetMassInvScaleA());
	CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, pushA, localPosA);

	Vec3V pushB    = push    * Vec3VFromF32(manifold.GetMassInvScaleB());
	CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -pushB, localPosB);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
