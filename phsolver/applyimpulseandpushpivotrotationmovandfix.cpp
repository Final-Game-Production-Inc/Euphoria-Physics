// 
// phsolver/applyimpulseandpushpivotrotationmovandfix.cpp 
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

void ApplyImpulseAndPushPivotRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	phCollider* colliderA = manifold.GetColliderA();

#if !EARLY_FORCE_SOLVE
	Vec3V angVelA = colliderA->GetAngVelocity();
#endif

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	/// Vec3V worldNormalA = contactPoint.GetWorldNormal();
	Vec3V axis = contactPoint.GetLocalPosA();

#if !EARLY_FORCE_SOLVE
	//Vec3V ang = contactPoint.GetDepthV() * worldNormalA;
	//ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V separate = Vec3V(V_ZERO);//separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - separate;
	Vec3V angVelProjection = Dot(deltaAngVel, axis) * axis;
	Vec3V clampedDeltaAngVel = angVelProjection - deltaAngVel;
	Vec3V angImpulse = Multiply(angInertia, clampedDeltaAngVel);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ConstraintsPivotRotationMovAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(contactPoint.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
#endif

	// PUSH

	Vec3V turnA = colliderA->GetTurn();

	Vec3V deltaAng = contactPoint.GetDepthV() * contactPoint.GetWorldNormal() - turnA;
	Vec3V turnProjection = Dot(deltaAng, axis) * axis;
	Vec3V clampedDeltaAng = deltaAng - turnProjection;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, clampedDeltaAng));

	contactPoint.SetPreviousPush(turn);

	turnA += Multiply(manifold.GetInertiaInvA(), turn);

	colliderA->SetTurn(RCC_VECTOR3(turnA));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
