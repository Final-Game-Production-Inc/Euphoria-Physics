// 
// phsolver/preresponserotationartandmov.cpp 
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

void PreResponseRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
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

		int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
		Vec3V impulseA = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
		CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, RCC_VECTOR3(impulseA));

		Vec3V angVelB = colliderB->GetAngVelocity();
		angVelB -= Multiply(manifold.GetInertiaInvB(), compositeImpulse);
		colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}

} // namespace rage
