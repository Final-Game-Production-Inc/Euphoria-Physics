// 
// phsolver/updatecontactsrotationmovandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

#if __SPU
#include "phcore/phmath.cpp"
#endif

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsRotationMovAndMov(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact& cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phCollider* colliderA = manifold.GetColliderA();
		phCollider* colliderB = manifold.GetColliderB();

		Mat33V mA(*(Mat33V*)&colliderA->GetMatrix());
		Mat33V mB(*(Mat33V*)&colliderB->GetMatrix());

		manifold.SetMassInvA(colliderA->GetSolverInvMass() * manifold.GetMassInvScaleA());
		manifold.SetMassInvB(colliderB->GetSolverInvMass() * manifold.GetMassInvScaleB());

		phMathInertia::GetInverseInertiaMatrix(mA,colliderA->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvA());
		phMathInertia::GetInverseInertiaMatrix(mB,colliderB->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvB());
		Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
		Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
		Scale(manifold.GetInertiaInvA(), manifold.GetInertiaInvA(), massInvScaleAV);
		Scale(manifold.GetInertiaInvB(), manifold.GetInertiaInvB(), massInvScaleBV);

		//  Vec3V angVelA = colliderA->GetAngVelocity();
		/// Vec3V angVelB = colliderB->GetAngVelocity();
		Mat33V K(-manifold.GetInertiaInvA() - manifold.GetInertiaInvB());
		InvertFull(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
