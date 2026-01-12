// 
// phsolver/updatecontactsrotationmovandfix.cpp 
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

void UpdateContactsRotationMovAndFix(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	Assert(manifold.GetNumContacts() == 1);
	phContact &cp = manifold.GetContactPoint(0);
	if(Likely(cp.IsContactActive()))
	{
		phCollider* colliderA = manifold.GetColliderA();

		Mat33V mA(*(Mat33V*)&colliderA->GetMatrix());

		manifold.SetMassInvA(colliderA->GetRotationConstraintInvMass());

		phMathInertia::GetInverseInertiaMatrix(mA,colliderA->GetRotationConstraintInvAngInertia().GetIntrin128(),manifold.GetInertiaInvA());

		Assert(manifold.GetNumContacts() == 1);

		InvertFull(manifold.GetConstraintMatrix(), manifold.GetInertiaInvA());

		cp.SetPreviousPush(Vec3V(V_ZERO));
	}
}

} // namespace rage
