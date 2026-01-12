// 
// phsolver/updatecontactsfixedpointartandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
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

void UpdateContactsFixedPointArtAndMov(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phCollider* colliderB = manifold.GetColliderB();
		Assert(!colliderB->IsArticulated());

		Mat33V mB(*(Mat33V*)&colliderB->GetMatrix());
		manifold.SetMassInvB(colliderB->GetSolverInvMass() * manifold.GetMassInvScaleB());
		phMathInertia::GetInverseInertiaMatrix(mB,colliderB->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvB());
		Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
		Scale(manifold.GetInertiaInvB(), manifold.GetInertiaInvB(), massInvScaleBV);

#if USE_CENTRE_POINT_FOR_ART_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
		Vec3V localPosB = localPos - colliderB->GetPosition();
#else
		/// Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
		Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

		Mat33V jacA;
		colliderA->GetInverseMassMatrix(jacA, cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
		Transpose(jacA, jacA);
		Vec3V massInvMultAV = Vec3VFromF32(manifold.GetMassInvScaleA());
		Scale(jacA, jacA, massInvMultAV);

		Mat33V scaleMatrix;
		Mat33VFromScale(scaleMatrix, Vec3VFromF32(manifold.GetMassInvB()));
		Mat33V crossB;
		CrossProduct(crossB, localPosB);
		Mat33V termB;
		Multiply(termB, crossB, manifold.GetInertiaInvB());
		Multiply(termB, termB, crossB);
		Mat33V K(jacA + scaleMatrix - termB);

		InvertFull(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}


} // namespace rage
