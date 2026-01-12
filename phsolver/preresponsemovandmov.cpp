// 
// phsolver/preresponsemovandmov.cpp 
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

void PreResponseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	bool applyWarmStart = globals.applyWarmStart && !colliderA->GetClearNextWarmStart() && !colliderB->GetClearNextWarmStart() ;
	bool clearWarmStart = globals.clearWarmStart || colliderA->GetClearNextWarmStart() || colliderB->GetClearNextWarmStart();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
#if USE_CENTRE_POINT_FOR_CONTACTS
			Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
			localPos*=ScalarV(V_HALF);
			Vec3V localPosA = localPos - colliderA->GetPosition();
			Vec3V localPosB = localPos - colliderB->GetPosition();
#else
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif
			if(applyWarmStart)
			{
				// Start off by re-applying last frame's normal and frictional impulses as they are our starting points for this frame's iterations.
//					ScalarV accumFriction(Dot(newTangent, oldTangent) * cp.GetAccumFriction());
				Vec3V compositeImpulse = cp.ComputeTotalImpulse();

#if POSITIVE_DEPTH_ONLY_NEW
#if USE_NEGATIVE_DEPTH_TUNABLE
				if (g_UseNegativeDepthForceCancel)
#endif
				compositeImpulse = SelectFT(BoolV(cp.IsPositiveDepth()), Vec3V(V_ZERO), compositeImpulse);
#endif

				// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
				//   other equally, so they each receive half of the impulse.
				velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
				angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));
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
			else if(clearWarmStart)
			{
				cp.SetPreviousSolution(Vec3V(V_ZERO));
			}
		}
	}

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());

	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

#else // ACCUMULATION_SOLVER_MATH == 0

void PreResponseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
#if USE_CENTRE_POINT_FOR_CONTACTS
			Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
			localPos*=ScalarV(V_HALF);
			Vec3V localPosA = localPos - colliderA->GetPosition();
			Vec3V localPosB = localPos - colliderB->GetPosition();
#else
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif
			// apply impulse
			Vec3V compositeImpulse = cp.ComputeTotalArtFixImpulse();//cp.GetWorldNormal() * Vec3V(cp.GetAccumImpulse()) + cp.GetTangent() * Vec3V(cp.GetAccumFriction());

			if(globals.warmStart)
			{
				// Adjust the velocities of bodies A and B according to the impulse from this contact.
				velocityA += compositeImpulse * Vec3VFromF32(manifold.massInv[0]);
				angVelA += Multiply(manifold.inertiaInv[0], Cross(localPosA,compositeImpulse));
				velocityB -= compositeImpulse * Vec3VFromF32(manifold.massInv[1]);
				angVelB -= Multiply(manifold.inertiaInv[1], Cross(localPosB,compositeImpulse));
			}
		}
	}

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);

	colliderB->SetVelocityOnly(velocityB);
	colliderB->SetAngVelocityOnly(angVelB);
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
