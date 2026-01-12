// 
// phsolver/updatecontactsartandfix.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "mathext/lcp.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/colliderdispatch.h"
#include "physics/contact.h"
#include "physics/manifold.h"
 
SOLVER_OPTIMISATIONS()

namespace rage {

	void UpdateContactsArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
	{
		phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
		Assert(colliderA->IsArticulated());
		phArticulatedBody* bodyA = colliderA->GetBody();

		for(int c=0;c<manifold.GetNumContacts();c++) {
			phContact &cp = manifold.GetContactPoint(c);
			if (Likely(cp.IsContactActive()))
			{
				Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition(); // - because of #if 0 block below

				int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());

				Mat33V constraintAxis;
				MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
				constraintAxis.SetCol0(cp.GetWorldNormal());

				if (globals.calculateBounceAndTangent.Getb())
				{
					Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocityNoProp)(linkA, localPosA.GetIntrin128());
					const Vec3V relativeVelocity = localVelocityA - cp.GetTargetRelVelocity();

					ScalarV normalVelocity = Dot(relativeVelocity, cp.GetWorldNormal());
					normalVelocity = SelectFT(normalVelocity > -globals.minBounce, normalVelocity, ScalarV(V_ZERO));
					Vec3V bounceVelocity = Scale(cp.GetWorldNormal(), normalVelocity * cp.GetElasticityV());

					cp.SetTargetRelVelocity(UnTransformOrtho(constraintAxis, cp.GetTargetRelVelocity()) + bounceVelocity); 
				}

				Mat33V artJacobianTemp;
				CALL_MEMBER_FN(*colliderA, GetInverseMassMatrix)(artJacobianTemp, cp.GetWorldPosA().GetIntrin128(), manifold.GetComponentA());
				Transpose(artJacobianTemp, artJacobianTemp);

				Mat33V artJacobian;

				// Transform the given inverse mass matrix from the local coordinates of parentA to world coordinates.
				Multiply(artJacobian, artJacobianTemp, constraintAxis);

				// Transform it from world coordinates into the local coordinates of parentB.
				UnTransformOrtho(artJacobian, constraintAxis, artJacobian);

				// Transpose the resulting inverse mass matrix because ...
				Transpose(artJacobian, artJacobian);

				cp.SetFrictionPlaneProjection(InvScaleSafe(artJacobian.GetCol0(), Vec3V(SplatX(artJacobian.GetCol0())), Vec3V(V_ZERO) ));
				VALIDATE_PHYSICS_ASSERTF(artJacobian.GetM00f()+artJacobian.GetM11f()+artJacobian.GetM22f()>=0.0f,"Contact force solver inverting a bad matrix, diagonals = %f, %f, %f",artJacobian.GetM00f(),artJacobian.GetM11f(),artJacobian.GetM22f());
				Mat33V dinv;
				InvertFull(dinv, artJacobian);
				BoolV is00Zero(IsZero(artJacobian.GetCol0().GetX()));
				// We get zero Jacobians if we have infinite mass vs. infinite mass...don't use the invert that or it's QNaN city
				dinv = SelectFT(is00Zero, dinv, Mat33V(V_IDENTITY));
				cp.SetDinv(dinv);
				cp.SetPreviousPush(Vec3V(V_ZERO));
			}
		}
	}

} // namespace rage