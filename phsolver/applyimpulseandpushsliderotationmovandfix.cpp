// 
// phsolver/applyimpulseandpushsliderotationmovandfix.cpp 
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

void ApplyImpulseAndPushSlideRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& NOT_EARLY_FORCE_SOLVE_ONLY(globals))
{
	phCollider* colliderA = manifold.GetColliderA();


	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	Vec3V worldNormalA = contactPoint.GetWorldNormal();

#if !EARLY_FORCE_SOLVE
	Vec3V angVelA = colliderA->GetAngVelocity();

	Vec3V ang = contactPoint.GetDepthV() * worldNormalA;
	ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = separateBias * ang;

	Vec3V deltaAngVel = separate - angVelA;
	deltaAngVel = Scale(worldNormalA,Dot(deltaAngVel,worldNormalA));

	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	Vec3V oldAngImpulse = contactPoint.GetPreviousSolution();
	Vec3V newAngImpulse = oldAngImpulse + angImpulse;
	ScalarV impulseAroundAxis = Dot(newAngImpulse, worldNormalA);
	newAngImpulse = SelectFT(IsGreaterThan(impulseAroundAxis, ScalarV(V_ZERO)), Vec3V(V_ZERO), newAngImpulse);
	contactPoint.SetPreviousSolution(newAngImpulse);
	angImpulse = newAngImpulse - oldAngImpulse;

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsSlideRotationMovAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(contactPoint.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
#endif

	// PUSH

	Vec3V turnA = colliderA->GetTurn();

	Vec3V deltaTurnA;
	deltaTurnA = Scale(worldNormalA,Dot(-turnA,worldNormalA));

	Vec3V deltaAng = contactPoint.GetDepthV() * worldNormalA + deltaTurnA;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	Vec3V oldTurn = contactPoint.GetPreviousPush();
	Vec3V newTurn = oldTurn + turn;
	ScalarV turnAroundAxis = Dot(newTurn, worldNormalA);
	newTurn = SelectFT(IsGreaterThan(turnAroundAxis, ScalarV(V_ZERO)), Vec3V(V_ZERO), newTurn);
	contactPoint.SetPreviousPush(newTurn);
	turn = newTurn - oldTurn;

	turnA += Multiply(manifold.GetInertiaInvA(), turn);

	colliderA->SetTurn(RCC_VECTOR3(turnA));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
