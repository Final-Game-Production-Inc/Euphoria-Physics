// 
// phsolver/applyimpulsefixedrotationmovandmov.cpp
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

void ApplyImpulseFixedRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V K = manifold.GetConstraintMatrix();

	Vec3V deltaAng = Clamp(cp.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * cp.GetWorldNormal();

	// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
	Vec3V separate = separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - angVelB - separate;
	Vec3V angImpulse = Multiply(K, deltaAngVel);

	cp.SetPreviousSolution(angImpulse);

	angVelA += Multiply(manifold.GetInertiaInvA(), angImpulse);
	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

} // namespace rage
