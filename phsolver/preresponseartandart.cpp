// 
// phsolver/preresponseartandart.cpp 
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

void PreResponseArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	bool applyWarmStart = globals.applyWarmStart && !colliderA->GetClearNextWarmStart() && !colliderB->GetClearNextWarmStart() ;
	bool clearWarmStart = globals.clearWarmStart || colliderA->GetClearNextWarmStart() || colliderB->GetClearNextWarmStart();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
			Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB()); 

			if(applyWarmStart)
			{
				//Needs more testing
				//Vec3V bounceVelocity = Scale(relativeVelocity, cp.GetElasticityV());
				//cp.SetTargetRelVelocity(cp.GetTargetRelVelocity() - bounceVelocity);

				Vec3V compositeImpulse = cp.GetWorldNormal() * Vec3V(cp.GetAccumImpulse()) + cp.GetTangent() * Vec3V(cp.GetAccumFriction());

#if POSITIVE_DEPTH_ONLY_NEW
#if USE_NEGATIVE_DEPTH_TUNABLE
				if (g_UseNegativeDepthForceCancel)
#endif
				compositeImpulse = SelectFT(BoolV(cp.IsPositiveDepth()), Vec3V(V_ZERO), compositeImpulse);
#endif

				int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
				Vec3V impulseA = compositeImpulse * massInvScaleAV;
				CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, impulseA, localPosA);

				int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
				Vec3V impulseB = compositeImpulse * massInvScaleBV;
				CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -impulseB, localPosB);

#if POSITIVE_DEPTH_ONLY_NEW
#if USE_NEGATIVE_DEPTH_TUNABLE
				if (g_UseNegativeDepthForceCancel)
#endif
				{
				// If the contact depth is negative then we're not going to be applying any normal or frictional impulses nor will be iterating
				//   over the accumulated impulse, so set those values to zero here.
				cp.SetAccumImpulse(SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), cp.GetAccumImpulse()));
				cp.SetAccumFriction(SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), cp.GetAccumFriction()));
				}
#endif
			}
			else if (clearWarmStart)
			{
				cp.SetPreviousSolution(Vec3V(V_ZERO));
			}
		}
	}
}


} // namespace rage
