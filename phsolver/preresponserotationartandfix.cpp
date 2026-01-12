// 
// phsolver/preresponserotationartandfix.cpp 
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

void PreResponseRotationArtAndFix(phManifold& UNUSED_PARAM(manifold), const phForceSolverGlobals& UNUSED_PARAM(globals))
{
#if 0
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	Assert(manifold.GetNumContacts() == 1);
	phContact &cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	Vec3V compositeImpulse = cp.GetPreviousSolution();
#endif

	//int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	//colliderA->ApplyImpulse(linkA, RCC_VECTOR3(localPosA), RCC_VECTOR3(compositeImpulse));
	//colliderA->ApplyImpulse(compositeImpulse.GetIntrin128(), cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
}

} // namespace rage
