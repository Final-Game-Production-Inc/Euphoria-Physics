// 
// phsolver/updatecontactsrotationartandfix.cpp 
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

void UpdateContactsRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact &cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());

		Mat33V K;
		colliderA->GetInverseInertiaMatrixArt(K, manifold.GetComponentA());
		InvertFull(K, K);
		Transpose(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
