// 
// phsolver/preresponserotationartandart.cpp 
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

void PreResponseRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
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
		phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
		Assert(colliderB->IsArticulated());
		phArticulatedBody* bodyB = colliderB->GetBody();

		Vec3V compositeImpulse = cp.GetPreviousSolution();

		int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
		Vec3V impulseA = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
		CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, RCC_VECTOR3(impulseA));

		int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
		Vec3V impulseB = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleB());
		CALL_MEMBER_FN(*bodyB, ApplyAngImpulse)(linkB, -RCC_VECTOR3(impulseB));
	}
	else if(globals.clearWarmStart)
	{
		cp.SetPreviousSolution(Vec3V(V_ZERO));
	}
}


} // namespace rage
