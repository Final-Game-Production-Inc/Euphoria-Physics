// 
// phsolver/applyimpulseandpushfixedrotationmovandmov.cpp
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

void ApplyImpulseAndPushFixedRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& NOT_EARLY_FORCE_SOLVE_ONLY(globals))
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

#if !EARLY_FORCE_SOLVE
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	Vec3V ang = cp.GetDepthV() * cp.GetWorldNormal();
	ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = separateBias * ang;

	Vec3V deltaAngVel = angVelA - angVelB - separate;
	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	cp.SetPreviousSolution(angImpulse);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);
	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsFixedRotationMovAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
#endif

	// PUSH

	Vec3V turnA = colliderA->GetTurn();
	Vec3V turnB = colliderB->GetTurn();

	Vec3V deltaAng = turnA - turnB - cp.GetDepthV() * cp.GetWorldNormal();
	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	cp.SetPreviousPush(turn);

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
