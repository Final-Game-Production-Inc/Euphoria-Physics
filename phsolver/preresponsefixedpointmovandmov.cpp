// 
// phsolver/preresponsefixedpointmovandmov.cpp 
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

void PreResponseFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	cp.SetPreviousPush(Vec3V(V_ZERO));

	if(globals.applyWarmStart)
	{
		phCollider* colliderA = manifold.GetColliderA();
		phCollider* colliderB = manifold.GetColliderB();

		Vec3V velocityA = colliderA->GetVelocity();
		Vec3V velocityB = colliderB->GetVelocity();
		Vec3V angVelA = colliderA->GetAngVelocity();
		Vec3V angVelB = colliderB->GetAngVelocity();

#if USE_CENTRE_POINT_FOR_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
		Vec3V localPosB = localPos - colliderB->GetPosition();
#else
		Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
		Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif
		Vec3V compositeImpulse = cp.GetPreviousSolution();

		velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
		angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));

		velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
		angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));

		colliderA->SetVelocityOnly(velocityA.GetIntrin128());
		colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());

		colliderB->SetVelocityOnly(velocityB.GetIntrin128());
		colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
