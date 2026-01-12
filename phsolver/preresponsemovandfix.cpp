// 
// phsolver/preresponsemovandfix.cpp 
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

#if ACCUMULATION_SOLVER_MATH == 0

void PreResponseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	
	bool applyWarmStart = globals.applyWarmStart && !colliderA->GetClearNextWarmStart();
	bool clearWarmStart = globals.clearWarmStart || colliderA->GetClearNextWarmStart();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact &cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			if(applyWarmStart)
			{
				Vec3V compositeImpulse = cp.ComputeTotalImpulse();

#if POSITIVE_DEPTH_ONLY_NEW
#if USE_NEGATIVE_DEPTH_TUNABLE
				if (g_UseNegativeDepthForceCancel)
#endif
				compositeImpulse = SelectFT(BoolV(cp.IsPositiveDepth()), Vec3V(V_ZERO), compositeImpulse);
#endif

				velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
				angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));

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

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
}

#else // ACCUMULATION_SOLVER_MATH == 0

void PreResponseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact &cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

			// apply impulse
			Vec3V compositeImpulse = cp.ComputeTotalArtFixImpulse();//cp.GetWorldNormal() * Vec3V(cp.GetAccumImpulse()) + cp.GetTangent() * Vec3V(cp.GetAccumFriction());

			if(globals.warmStart)
			{
				velocityA += compositeImpulse * Vec3VFromF32(manifold.massInv[0]);
				angVelA += Multiply(manifold.inertiaInv[0], Cross(localPosA,compositeImpulse));
			}
		}
	}

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
