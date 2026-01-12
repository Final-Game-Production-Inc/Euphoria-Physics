// 
// phsolver/preresponseartandfix.cpp 
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

void PreResponseArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	bool applyWarmStart = globals.applyWarmStart && !colliderA->GetClearNextWarmStart();
	bool clearWarmStart = globals.clearWarmStart || colliderA->GetClearNextWarmStart();

	if (applyWarmStart)
	{
		for(int c=0;c<manifold.GetNumContacts();c++) {
			phContact &cp = manifold.GetContactPoint(c);
			if (Likely(cp.IsContactActive()))
			{
				Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

				if (applyWarmStart)
				{
					// apply impulse
					Vec3V compositeImpulse = cp.ComputeTotalArtFixImpulse();//cp.GetWorldNormal() * Vec3V(cp.GetAccumImpulse()) + cp.GetTangent() * Vec3V(cp.GetAccumFriction());

					int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
					CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, compositeImpulse, localPosA);
				}
				else if (clearWarmStart)
				{
					cp.SetPreviousSolution(Vec3V(V_ZERO));
				}
			}
		}
	}
}

} // namespace rage
