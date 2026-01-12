// 
// phsolver/updatecontactsfixedpointartandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/colliderdispatch.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "pharticulated/articulatedbody.cpp"
#include "pharticulated/articulatedcollider.cpp"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsFixedPointArtAndArt(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
		Assert(colliderB->IsArticulated());

#if USE_CENTRE_POINT_FOR_ART_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
		Vec3V localPosB = localPos - colliderB->GetPosition();
#else
		/// Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
		/// Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

		Mat33V jac;

		Mat33V jacA;
		colliderA->GetInverseMassMatrix(jacA, cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
		Vec3V massInvMultAV = Vec3VFromF32(manifold.GetMassInvScaleA());
		Scale(jacA, jacA, massInvMultAV);

		Mat33V jacB;
		colliderB->GetInverseMassMatrix(jacB, cp.GetWorldPosB().GetIntrin128(), manifold.GetComponentB());
		Add(jac, jacA, jacB);
		Vec3V massInvMultBV = Vec3VFromF32(manifold.GetMassInvScaleB());
		Scale(jacB, jacB, massInvMultBV);

		// use different Jacobian for self collisions
		if ( colliderA==colliderB ) 
		{
			Matrix33 tempJac;
			colliderA->GetInverseMassMatrixSelf(RC_MAT33V(tempJac), cp.GetWorldPosA().GetIntrin128(), cp.GetWorldPosB().GetIntrin128(), manifold.GetComponentA(), manifold.GetComponentB());
			Subtract(jac, jac, RCC_MAT33V(tempJac));
			tempJac.Transpose();
			Subtract(jac, jac, RCC_MAT33V(tempJac));
		}
		InvertFull(jac, jac);
		Transpose(manifold.GetConstraintMatrix(), jac);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
