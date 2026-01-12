// 
// phsolver/applyimpulsefixedpointartandfix.cpp  
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

void ApplyImpulseFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Mat33V Kinv = manifold.GetConstraintMatrix();

	Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
	int link = colliderA->GetLinkFromComponent(manifold.GetComponentA());

	Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(link, localPosA.GetIntrin128());

	// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
	Vec3V separate = separateBias * ClampMag(cp.GetWorldPosB() - cp.GetWorldPosA(), ScalarV(V_ZERO), globals.halfAllowedPenetration);

	Vec3V num = cp.GetTargetRelVelocity() - localVelocityA;

	Vec3V deltaVel = num + separate;
	Vec3V impulse = Multiply(Kinv, deltaVel);

	cp.SetPreviousSolution(impulse);

	CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, impulse, localPosA);
}

} // namespace rage
