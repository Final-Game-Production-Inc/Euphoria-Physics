// 
// phsolver/applyimpulsepivotrotationartandmov.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulsePivotRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Vec3V angVelA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

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

	Vec3V angImpulseA = angImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, angImpulseA.GetIntrin128());

	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

} // namespace rage
