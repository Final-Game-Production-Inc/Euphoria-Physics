// 
// phsolver/applyimpulseandpushfixedpointmovandmov.cpp 
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

void ApplyImpulseAndPushFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& NOT_EARLY_FORCE_SOLVE_ONLY(globals))
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V Kinv = manifold.GetConstraintMatrix();

#if USE_CENTRE_POINT_FOR_CONSTRAINTS
	Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
	localPos=localPos*ScalarV(V_HALF);
	Vec3V localPosA = localPos - colliderA->GetPosition();
	Vec3V localPosB = localPos - colliderB->GetPosition();
#else
	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
	Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

#if !EARLY_FORCE_SOLVE
	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);
	Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

	ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = separateBias * (cp.GetWorldPosB() - cp.GetWorldPosA());

	Vec3V deltaVel = localVelocityB - localVelocityA + separate;
	Vec3V impulse = Multiply(Kinv, deltaVel);

	cp.SetPreviousSolution(impulse);

	velocityA += impulse * Vec3VFromF32(manifold.GetMassInvA());
	angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,impulse));

	velocityB -= impulse * Vec3VFromF32(manifold.GetMassInvB());
	angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,impulse));

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());

	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
#endif

	// PUSH

	Vec3V pushA = colliderA->GetPush();
	Vec3V pushB = colliderB->GetPush();
	Vec3V turnA = colliderA->GetTurn();
	Vec3V turnB = colliderB->GetTurn();

	Vec3V localPushA = pushA + Cross(turnA,localPosA);
	Vec3V localPushB = pushB + Cross(turnB,localPosB);

	Vec3V relativePush = localPushA-localPushB;

	Vec3V deltaPos = cp.GetWorldPosB() - cp.GetWorldPosA() - relativePush;
	Vec3V push = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(Kinv, deltaPos));

	cp.SetPreviousPush(push);

	pushA += push * Vec3VFromF32(manifold.GetMassInvA());
	turnA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,push));

	pushB -= push * Vec3VFromF32(manifold.GetMassInvB());
	turnB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,push));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsFixedPointMovAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	colliderA->SetPush(RCC_VECTOR3(pushA));
	colliderA->SetTurn(RCC_VECTOR3(turnA));

	colliderB->SetPush(RCC_VECTOR3(pushB));
	colliderB->SetTurn(RCC_VECTOR3(turnB));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
