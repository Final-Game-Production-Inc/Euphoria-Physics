// 
// phsolver/applyimpulsesliderotationmovandfix.cpp
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

void ApplyImpulseSlideRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V angVelA = colliderA->GetAngVelocity();

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	Vec3V worldNormalA = contactPoint.GetWorldNormal();

	ScalarV separateBias = globals.separateBias; // * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V deltaAng = Clamp(contactPoint.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * contactPoint.GetWorldNormal();
	Vec3V separate = separateBias * deltaAng;

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

	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
}


} // namespace rage
