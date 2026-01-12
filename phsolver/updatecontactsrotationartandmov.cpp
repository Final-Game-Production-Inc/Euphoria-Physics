//  
// phsolver/updatecontactsrotationartandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/colliderdispatch.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "pharticulated/articulatedbody.cpp"
#include "pharticulated/articulatedcollider.cpp"
#include "phcore/phmath.cpp"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsRotationArtAndMov(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phCollider* colliderB = manifold.GetColliderB();
		Assert(!colliderB->IsArticulated());

		Mat33V jacA;
		colliderA->GetInverseInertiaMatrixArt(jacA, manifold.GetComponentA());
		Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
		Scale(jacA, jacA, massInvScaleAV);

		Mat33V mB(*(Mat33V*)&colliderB->GetMatrix());
		manifold.SetMassInvB(colliderB->GetSolverInvMass() * manifold.GetMassInvScaleB());
		phMathInertia::GetInverseInertiaMatrix(mB,colliderB->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvB());
		Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
		Scale(manifold.GetInertiaInvB(), manifold.GetInertiaInvB(), massInvScaleBV);

		Mat33V K(-jacA - manifold.GetInertiaInvB());

		InvertFull(manifold.GetConstraintMatrix(), K);


		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
