// 
// phsolver/preresponserotationmovandmov.cpp 
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

void PreResponseRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	cp.SetPreviousPush(Vec3V(V_ZERO));

	if(globals.applyWarmStart)
	{
		phCollider* colliderA = manifold.GetColliderA();
		phCollider* colliderB = manifold.GetColliderB();

		Vec3V angVelA = colliderA->GetAngVelocity();
		Vec3V angVelB = colliderB->GetAngVelocity();

		Assert(manifold.GetNumContacts() == 1);

		Vec3V compositeImpulse = cp.GetPreviousSolution();

		angVelA += Multiply(manifold.GetInertiaInvA(), compositeImpulse);
		angVelB -= Multiply(manifold.GetInertiaInvB(), compositeImpulse);

		colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
		colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
