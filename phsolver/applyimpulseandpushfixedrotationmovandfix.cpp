// 
// phsolver/applyimpulseandpushfixedrotationmovandfix.cpp 
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

void ApplyImpulseAndPushFixedRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& NOT_EARLY_FORCE_SOLVE_ONLY(globals))
{
	phCollider* colliderA = manifold.GetColliderA();

#if !EARLY_FORCE_SOLVE
	Vec3V angVelA = colliderA->GetAngVelocity();
#endif

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

#if !EARLY_FORCE_SOLVE
	Vec3V ang = cp.GetDepthV() * cp.GetWorldNormal();
	ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = separateBias * ang;

	Vec3V deltaAngVel = separate - angVelA;
	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	cp.SetPreviousSolution(angImpulse);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsFixedRotationMovAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
#endif

	// PUSH

	Vec3V turnA = colliderA->GetTurn();

	Vec3V deltaAng = cp.GetDepthV() * cp.GetWorldNormal() - turnA;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	cp.SetPreviousPush(turn);

	turnA += Multiply(manifold.GetInertiaInvA(), turn);

	colliderA->SetTurn(RCC_VECTOR3(turnA));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
