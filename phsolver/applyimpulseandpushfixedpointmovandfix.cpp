// 
// phsolver/applyimpulseandpushfixedpointmovandfix.cpp 
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

void ApplyImpulseAndPushFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& NOT_EARLY_FORCE_SOLVE_ONLY(globals))
{
	phCollider* colliderA = manifold.GetColliderA();

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V Kinv = manifold.GetConstraintMatrix();

#if USE_CENTRE_POINT_FOR_CONSTRAINTS
	Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
	localPos=localPos*ScalarV(V_HALF);
	Vec3V localPosA = localPos - colliderA->GetPosition();
#else
	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
#endif

#if !EARLY_FORCE_SOLVE
	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();

	Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);

	//if(solverIO.deformMeshEnable && cp.subDataType1 == SubData::SubDataFacetLocal) {
	//	velocityB += rotate(stateB.fQ,cp.localVelocity[1]);
	//}

	ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = separateBias * (cp.GetWorldPosB() - cp.GetWorldPosA());

	Vec3V deltaVel = cp.GetTargetRelVelocity() - localVelocityA + separate;
	Vec3V impulse = Multiply(Kinv, deltaVel);

	cp.SetPreviousSolution(impulse);

	velocityA += impulse * Vec3VFromF32(manifold.GetMassInvA());
	angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,impulse));

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
#else
	Vec3V impulse(V_ZERO);
#endif

	// PUSH

	Vec3V pushA = colliderA->GetPush();
	Vec3V turnA = colliderA->GetTurn();

	Vec3V localPushA = pushA + Cross(turnA,localPosA);

	Vec3V deltaPos = cp.GetWorldPosB() - cp.GetWorldPosA() - localPushA;

	Vec3V push = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(Kinv, deltaPos));

	cp.SetPreviousPush(push);

	pushA += push * Vec3VFromF32(manifold.GetMassInvA());
	turnA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,push));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsFixedPointMovAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(cp.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	colliderA->SetPush(RCC_VECTOR3(pushA));
	colliderA->SetTurn(RCC_VECTOR3(turnA));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
