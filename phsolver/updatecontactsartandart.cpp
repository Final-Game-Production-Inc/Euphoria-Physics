// 
// phsolver/updatecontactsartandart.cpp 
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

SOLVER_OPTIMISATIONS()

namespace rage {

void UpdateContactsArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	for(int c=0;c<manifold.GetNumContacts();c++) {
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
			int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());

			Mat33V jac;

			Mat33V jacA;
			CALL_MEMBER_FN(*colliderA, GetInverseMassMatrix)(jacA, cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
			Vec3V massInvScaleAV = Vec3VFromF32(manifold.GetMassInvScaleA());
			Scale(jacA, jacA, massInvScaleAV);

			Mat33V jacB;
			CALL_MEMBER_FN(*colliderB, GetInverseMassMatrix)(jacB, cp.GetWorldPosB().GetIntrin128(), manifold.GetComponentB());
			Vec3V massInvScaleBV = Vec3VFromF32(manifold.GetMassInvScaleB());
			Scale(jacB, jacB, massInvScaleBV);
			Add(jac, jacA, jacB);

			// use different Jacobian for self collisions
			if ( colliderA==colliderB ) 
			{
				Matrix33 tempJac;
				CALL_MEMBER_FN(*colliderA, GetInverseMassMatrixSelf)(RC_MAT33V(tempJac), cp.GetWorldPosA().GetIntrin128(), cp.GetWorldPosB().GetIntrin128(), manifold.GetComponentA(), manifold.GetComponentB());
				Subtract(jac, jac, RCC_MAT33V(tempJac));
				tempJac.Transpose();
				Subtract(jac, jac, RCC_MAT33V(tempJac));
			}
			Transpose(jac, jac);

			cp.SetImpulseDen(InvertSafe(Dot(Multiply(jac, cp.GetWorldNormal()),cp.GetWorldNormal()), ScalarV(V_ZERO)));

			Vec3V tangent;
			if (globals.calculateBounceAndTangent.Getb())
			{
				Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocityNoProp)(linkA, localPosA.GetIntrin128());
				Vec3V localVelocityB = CALL_MEMBER_FN(*bodyB, GetLocalVelocityNoProp)(linkB, localPosB.GetIntrin128());
				Vec3V relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();
				tangent = NormalizeSafe(Cross(cp.GetWorldNormal(),Cross(cp.GetWorldNormal(),relativeVelocity)), Vec3V(V_ZERO));
			}
			else
			{
				tangent = cp.GetTangent();
			}
			cp.SetTangent(tangent);
			cp.SetFrictionDen(InvertSafe(Dot(Multiply(jac, tangent), tangent), ScalarV(V_ZERO)));
			cp.SetAccumPush(ScalarV(V_ZERO));
		}
	}
}

} // namespace rage
