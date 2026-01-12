// 
// phsolver/updatecontactsfixedpointmovandfix.cpp 
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

void UpdateContactsFixedPointMovAndFix(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact &cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phCollider* colliderA = manifold.GetColliderA();

		Mat33V mA(*(Mat33V*)&colliderA->GetMatrix());

		manifold.SetMassInvA(colliderA->GetTranslationConstraintInvMass());

		phMathInertia::GetInverseInertiaMatrix(mA,colliderA->GetTranslationConstraintInvAngInertia().GetIntrin128(),manifold.GetInertiaInvA());

		/// Vec3V velocityA = colliderA->GetVelocity();
		/// Vec3V angVelA = colliderA->GetAngVelocity();

		//cp.worldPoint[0] = rotate(stateA.fQ,cp.getLocalPointA());
		//cp.worldPoint[1] = rotate(stateB.fQ,cp.getLocalPointB());
#if USE_CENTRE_POINT_FOR_CONSTRAINTS
		Vec3V localPos=cp.GetWorldPosA()+cp.GetWorldPosB();
		localPos*=ScalarV(V_HALF);
		Vec3V localPosA = localPos - colliderA->GetPosition();
#else
		Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
#endif
		Mat33V scaleMatrix;
		Mat33VFromScale(scaleMatrix, Vec3VFromF32(manifold.GetMassInvA()));
		Mat33V cross;
		CrossProduct(cross, localPosA);
		Mat33V term;
		Multiply(term, cross, manifold.GetInertiaInvA());
		Multiply(term, term, cross);
		Mat33V K(scaleMatrix - term);
		InvertFull(manifold.GetConstraintMatrix(), K);

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
