// 
// phsolver/applyimpulsepivotrotationmovandmov.cpp
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

void ApplyImpulsePivotRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V K = manifold.GetConstraintMatrix();

	ScalarV separateBias = globals.separateBias; // * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V deltaAng = Clamp(contactPoint.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * contactPoint.GetWorldNormal();
	Vec3V separate = separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - angVelB - separate;
	Vec3V axis = contactPoint.GetLocalPosA();
	Vec3V projection = Dot(deltaAngVel, axis) * axis;
	Vec3V clampedDeltaAngVel = deltaAngVel - projection;
	Vec3V angImpulse = Multiply(K, clampedDeltaAngVel);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);
	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

} // namespace rage
