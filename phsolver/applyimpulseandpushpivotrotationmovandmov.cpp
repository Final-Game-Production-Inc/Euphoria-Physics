// 
// phsolver/applyimpulseandpushpivotrotationmovandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseAndPushPivotRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

#if !EARLY_FORCE_SOLVE
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();
#endif

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();
	Vec3V axis = contactPoint.GetLocalPosA();

	/// Vec3V worldNormalA = contactPoint.GetWorldNormal();

#if !EARLY_FORCE_SOLVE
	//Vec3V worldNormalA = contactPoint.GetWorldNormal();

	//Vec3V deltaAng = contactPoint.GetDepthV() * worldNormalA;

	// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	//ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
	Vec3V separate = Vec3V(V_ZERO);//separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - angVelB - separate;
	Vec3V angVelProjection = Dot(deltaAngVel, axis) * axis;
	Vec3V clampedDeltaAngVel = deltaAngVel - angVelProjection;
	Vec3V angImpulse = Multiply(angInertia, clampedDeltaAngVel);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);
	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsPivotRotationMovAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(contactPoint.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
#endif

	Vec3V turnA = colliderA->GetTurn();
	Vec3V turnB = colliderB->GetTurn();

	Vec3V deltaAng = turnA - turnB - contactPoint.GetDepthV() * contactPoint.GetWorldNormal();
	Vec3V turnProjection = Dot(deltaAng, axis) * axis;
	Vec3V clampedDeltaAng = deltaAng - turnProjection;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, clampedDeltaAng));

	contactPoint.SetPreviousPush(turn);

	turnA += Multiply(manifold.GetInertiaInvA(), turn);
	turnB -= Multiply(manifold.GetInertiaInvB(), turn);

	colliderA->SetTurn(RCC_VECTOR3(turnA));
	colliderB->SetTurn(RCC_VECTOR3(turnB));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
