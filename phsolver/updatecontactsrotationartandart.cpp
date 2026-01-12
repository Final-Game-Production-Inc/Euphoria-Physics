// 
// phsolver/updatecontactsrotationartandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/colliderdispatch.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "pharticulated/articulatedbody.cpp"
#include "pharticulated/articulatedcollider.cpp"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
		Assert(colliderB->IsArticulated());

		Mat33V jacA;
		colliderA->GetInverseInertiaMatrixArt(jacA, manifold.GetComponentA());
		Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
		Scale(jacA, jacA, massInvScaleAV);

		Mat33V jacB;
		colliderB->GetInverseInertiaMatrixArt(jacB, manifold.GetComponentB());
		Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
		Scale(jacB, jacB, massInvScaleBV);

		Mat33V K(-jacA - jacB);
		InvertFull(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
