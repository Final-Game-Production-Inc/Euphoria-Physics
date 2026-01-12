// 
// phsolver/preresponsefixedpointartandart.cpp  
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

void PreResponseFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	cp.SetPreviousPush(Vec3V(V_ZERO));

	if(globals.applyWarmStart)
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phArticulatedBody* bodyA = colliderA->GetBody();
		phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
		Assert(colliderB->IsArticulated());
		phArticulatedBody* bodyB = colliderB->GetBody();

		Vec3V compositeImpulse = cp.GetPreviousSolution();

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
		Vec3V compositeImpulseA = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
		CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, compositeImpulseA, localPosA);

		int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
		Vec3V compositeImpulseB = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleB());
		CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -compositeImpulseB, localPosB);
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
