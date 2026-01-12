// 
// phsolver/applyimpulsefixedpointartandmov.cpp  
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

void ApplyImpulseFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V Kinv = manifold.GetConstraintMatrix();

#if USE_CENTRE_POINT_FOR_ART_CONSTRAINTS
	Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
	localPos*=ScalarV(V_HALF);
	Vec3V localPosA = localPos - colliderA->GetPosition();
	Vec3V localPosB = localPos - colliderB->GetPosition();
#else
	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
	Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());

	Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

	// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
	Vec3V separate = separateBias * ClampMag(cp.GetWorldPosB() - cp.GetWorldPosA(), ScalarV(V_ZERO), globals.halfAllowedPenetration);

	Vec3V num = cp.GetTargetRelVelocity() + localVelocityB-localVelocityA;

	Vec3V deltaVel = num + separate;
	Vec3V impulse = Multiply(Kinv, deltaVel);

	cp.SetPreviousSolution(impulse);

	Vec3V impulseArt = impulse * Vec3VFromF32(manifold.GetMassInvScaleA());
	CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, impulseArt, localPosA);

	velocityB -= impulse * Vec3VFromF32(manifold.GetMassInvB());
	angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,impulse));

	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}
} // namespace rage
