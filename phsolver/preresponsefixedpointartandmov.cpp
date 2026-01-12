// 
// phsolver/preresponsefixedpointartandmov.cpp  
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

void PreResponseFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
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
		phCollider* colliderB = manifold.GetColliderB();
		Assert(!colliderB->IsArticulated());

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
		Vec3V compositeImpulseArt = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
		CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, compositeImpulseArt, localPosA);

		Vec3V velocityB = colliderB->GetVelocity();
		Vec3V angVelB = colliderB->GetAngVelocity();

		velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
		angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));

		colliderB->SetVelocityOnly(velocityB.GetIntrin128());
		colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
