// 
// phsolver/applyimpulsepivotrotationmovandfix.cpp
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

void ApplyImpulsePivotRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V angVelA = colliderA->GetAngVelocity();

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	ScalarV separateBias = globals.separateBias; // * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V deltaAng = Clamp(contactPoint.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * contactPoint.GetWorldNormal();
	Vec3V separate = separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - separate;
	Vec3V axis = contactPoint.GetLocalPosA();
	Vec3V projection = Dot(deltaAngVel, axis) * axis;
	Vec3V clampedDeltaAngVel = projection - deltaAngVel;
	Vec3V angImpulse = Multiply(angInertia, clampedDeltaAngVel);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
}


} // namespace rage
