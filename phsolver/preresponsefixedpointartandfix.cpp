// 
// phsolver/preresponsefixedpointartandfix.cpp  
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

void PreResponseFixedPointArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
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

		Vec3V compositeImpulse = cp.GetPreviousSolution();

		Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

		int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
		CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, compositeImpulse, localPosA);
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}


} // namespace rage
