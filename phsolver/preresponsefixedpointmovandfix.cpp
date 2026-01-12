// 
// phsolver/preresponsefixedpointmovandfix.cpp 
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

void PreResponseFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	cp.SetPreviousPush(Vec3V(V_ZERO));

	if(globals.applyWarmStart)
	{
		phCollider* colliderA = manifold.GetColliderA();

		Vec3V velocityA = colliderA->GetVelocity();
		Vec3V angVelA = colliderA->GetAngVelocity();

#if USE_CENTRE_POINT_FOR_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
#else
		Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
#endif
		// apply impulse
		Vec3V compositeImpulse = cp.GetPreviousSolution();

		velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
		angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));

		colliderA->SetVelocityOnly(velocityA.GetIntrin128());
		colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
