// 
// phsolver/updatecontactsfixedpointmovandmov.cpp 
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

void UpdateContactsFixedPointMovAndMov(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
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

		/// Vec3V velocityA = colliderA->GetVelocity();
		/// Vec3V velocityB = colliderB->GetVelocity();
		/// Vec3V angVelA = colliderA->GetAngVelocity();
		/// Vec3V angVelB = colliderB->GetAngVelocity();

#if USE_CENTRE_POINT_FOR_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
		Vec3V localPosB = localPos - colliderB->GetPosition();
#else
		Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
		Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif
		Mat33V scaleMatrix;
		Mat33VFromScale(scaleMatrix, Vec3VFromF32(manifold.GetMassInvA()) + Vec3VFromF32(manifold.GetMassInvB()));
		Mat33V crossA, crossB;
		CrossProduct(crossA, localPosA);
		CrossProduct(crossB, localPosB);
		Mat33V termA, termB;
		Multiply(termA, crossA, manifold.GetInertiaInvA());
		Multiply(termA, termA, crossA);
		Multiply(termB, crossB, manifold.GetInertiaInvB());
		Multiply(termB, termB, crossB);
		Mat33V K(scaleMatrix - termA - termB);
		InvertFull(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
