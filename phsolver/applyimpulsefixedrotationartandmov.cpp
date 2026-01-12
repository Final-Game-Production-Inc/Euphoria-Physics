// 
// phsolver/applyimpulsefixedrotationartandmov.cpp 
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

void ApplyImpulseFixedRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Vec3V angVelA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	Vec3V angVelB = colliderB->GetAngVelocity();

	Vec3V deltaAng = Clamp(cp.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * cp.GetWorldNormal();

	// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
	Vec3V separate = separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - angVelB - separate;

	Mat33V angInertia = manifold.GetConstraintMatrix();
	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	cp.SetPreviousSolution(angImpulse);

	Vec3V angImpulseA = angImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, angImpulseA.GetIntrin128());

	angVelB -= Multiply(manifold.GetInertiaInvB(), angImpulse);

	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

} // namespace rage
