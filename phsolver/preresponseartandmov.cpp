// 
// phsolver/preresponseartandmov.cpp  
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void PreResponseArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();
	Assert(!colliderB->IsArticulated());

	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	bool applyWarmStart = globals.applyWarmStart && !colliderA->GetClearNextWarmStart() && !colliderB->GetClearNextWarmStart() ;
	bool clearWarmStart = globals.clearWarmStart || colliderA->GetClearNextWarmStart() || colliderB->GetClearNextWarmStart();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			if (applyWarmStart)
			{
				//Needs more testing
				//Vec3V bounceVelocity = Scale(relativeVelocity, cp.GetElasticityV());
				//cp.SetTargetRelVelocity(cp.GetTargetRelVelocity() - bounceVelocity);

				Vec3V compositeImpulse = cp.ComputeTotalImpulse();

#if POSITIVE_DEPTH_ONLY_NEW
#if USE_NEGATIVE_DEPTH_TUNABLE
				if (g_UseNegativeDepthForceCancel)
#endif
				compositeImpulse = SelectFT(BoolV(cp.IsPositiveDepth()), Vec3V(V_ZERO), compositeImpulse);
#endif

				// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
				//   other equally, so they each receive half of the impulse.
				int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
				Vec3V impulseA = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
				CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, impulseA, localPosA);

				velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
				angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));

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

	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

} // namespace rage
