// 
// phsolver/updatecontactsartandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/phmath.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/colliderdispatch.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phCollider* colliderB = manifold.GetColliderB();
	Assert(!colliderB->IsArticulated());

	Mat33V mB(*(Mat33V*)&colliderB->GetMatrix());
	manifold.SetMassInvB(colliderB->GetSolverInvMass() * manifold.GetMassInvScaleB());
#if !__SPU
	phMathInertia::GetInverseInertiaMatrix(mB,colliderB->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvB());
#else
	globals.GetInverseInertiaMatrix(mB,colliderB->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvB());
#endif
	Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
	Scale(manifold.GetInertiaInvB(), manifold.GetInertiaInvB(), massInvScaleBV);

	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());

			Mat33V jac;

			Mat33V jacA;
			CALL_MEMBER_FN(*colliderA, GetInverseMassMatrix)(jacA, cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
			Transpose(jacA, jacA);
			Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
			Scale(jacA, jacA, massInvScaleAV);

			Mat33V scaleMatrix;
			Mat33VFromScale(scaleMatrix, Vec3VFromF32(manifold.GetMassInvB()));
			Mat33V crossB;
			CrossProduct(crossB, localPosB);
			Mat33V termB;
			Multiply(termB, crossB, manifold.GetInertiaInvB());
			Multiply(termB, termB, crossB);
			Mat33V K(scaleMatrix - termB);

			Add(jac, jacA, K);

			cp.SetImpulseDen(InvertSafe(Dot(Multiply(jac, cp.GetWorldNormal()),cp.GetWorldNormal()), ScalarV(V_ZERO)));

			Vec3V tangent;
			if (globals.calculateBounceAndTangent.Getb())
			{
				Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocityNoProp)(linkA, localPosA.GetIntrin128());
				Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

				Vec3V relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();
				tangent = NormalizeSafe(Cross(cp.GetWorldNormal(),Cross(cp.GetWorldNormal(),relativeVelocity)), Vec3V(V_ZERO));
				cp.SetTangent(tangent);
			}
			else
			{
				tangent = cp.GetTangent();
			}

			cp.SetFrictionDen(InvertSafe(Dot(Multiply(jac, tangent), tangent), ScalarV(V_ZERO)));
			cp.SetAccumPush(ScalarV(V_ZERO));
		}
	}
}

} // namespace rage
