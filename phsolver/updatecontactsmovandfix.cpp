// 
// phsolver/updatecontactsmovandfix.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

#if ACCUMULATION_SOLVER_MATH == 0

	void UpdateContactsMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
	{
		phCollider* colliderA = manifold.GetColliderA();
		Mat33V mA(*(Mat33V*)&colliderA->GetMatrix());

		manifold.SetMassInvA(colliderA->GetInvMass());
#if !__SPU
		phMathInertia::GetInverseInertiaMatrix(mA,colliderA->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvA());
#else
		globals.GetInverseInertiaMatrix(mA,colliderA->GetSolverInvAngInertia().GetIntrin128(),manifold.GetInertiaInvA());
#endif
		Vec3V velocityA = colliderA->GetVelocity();
		Vec3V angVelA = colliderA->GetAngVelocity();

#if FORCESOLVER_USE_NEW_BOUNCE_VELOCITY
		// The 'incoming' velocities are the velocities that this object had before forces and impulses (including gravity) were applied for this frame and
		//   before any velocities are changed in response to collisions/constraints.  It is important that we use these values for determining our target,
		//   post-collision-response, velocity, because we don't want an object resting on a surface to bounce due to reflecting the effects of one frame's
		//   worth of gravitational acceleration.  (Strictly speaking it *would* be okay to reflect the portion of the velocity that accumulated before the
		//   collision actually occurred but that's a little extra calculation for probably no observable benefit.)
		const Vec3V incomingLinearVelocityA = colliderA->GetVelocityBeforeForce();
		const Vec3V incomingAngularVelocityA = colliderA->GetAngVelocityBeforeForce();
#endif

		for(int c=0;c<manifold.GetNumContacts();c++) {
			phContact &cp = manifold.GetContactPoint(c);
			if (Likely(cp.IsContactActive()))
			{
				//cp.worldPoint[0] = rotate(stateA.fQ,cp.getLocalPointA());
				//cp.worldPoint[1] = rotate(stateB.fQ,cp.getLocalPointB());
				Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
				Mat33V scaleMatrix;
				Mat33VFromScale(scaleMatrix, Vec3VFromF32(manifold.GetMassInvA()));
				Mat33V cross;
				CrossProduct(cross, localPosA);
				Mat33V term;
				Multiply(term, cross, manifold.GetInertiaInvA());
				Multiply(term, term, cross);

				Mat33V K(scaleMatrix - term);

				cp.SetImpulseDen(InvertSafe(Dot(Multiply(K, cp.GetWorldNormal()),cp.GetWorldNormal()), ScalarV(V_ZERO)));
				cp.SetAccumPush(ScalarV(V_ZERO));

				// calc tangent
#if 1
				const Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);
#else
				// With this version the tangent direction depends only on the initial motion that the object has (see the comments above regarding those
				//   velocities) and doesn't change as the object's velocity changes due to the warm start applications from below.  It's probably more
				//   stable this way but I haven't seen any confirmed improvement so I'm leaving it disabled by default for now.
				const Vec3V localVelocityA = incomingLinearVelocityA + Cross(incomingAngularVelocityA,localPosA);
#endif
				//if(solverIO.deformMeshEnable && cp.subDataType1 == SubData::SubDataFacetLocal) {
				//	velocityB += rotate(stateB.fQ,cp.localVelocity[1]);
				//}

				const Vec3V relativeVelocity = localVelocityA - cp.GetTargetRelVelocity();

				//				const Vec3V oldTangent = cp.GetTangent();
				const Vec3V newTangent = NormalizeSafe(Cross(cp.GetWorldNormal(),Cross(cp.GetWorldNormal(),relativeVelocity)), Vec3V(V_ZERO));
				cp.SetTangent(newTangent);
				cp.SetFrictionDen(InvertSafe(Dot(Multiply(K, newTangent), newTangent), ScalarV(V_ZERO)));

				if (globals.calculateBounceAndTangent.Getb())
				{
#if !FORCESOLVER_USE_NEW_BOUNCE_VELOCITY
					ScalarV normalVelocity = Dot(relativeVelocity, cp.GetWorldNormal());
					normalVelocity = SelectFT(normalVelocity > globals.minBounce, normalVelocity, ScalarV(V_ZERO));
#else
					const Vec3V incomingLocalVelocityA = incomingLinearVelocityA + Cross(incomingAngularVelocityA, localPosA);
					const Vec3V incomingRelativeVelocity = incomingLocalVelocityA - cp.GetTargetRelVelocity();
					const ScalarV normalVelocity = Dot(incomingRelativeVelocity, cp.GetWorldNormal());
#endif
					Vec3V bounceVelocity = Scale(cp.GetWorldNormal(), normalVelocity * cp.GetElasticityV());
					cp.SetTargetRelVelocity(cp.GetTargetRelVelocity() - bounceVelocity);
				}
			}
		}
	}

#else // ACCUMULATION_SOLVER_MATH == 0

	void PreResponseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
	{
		phCollider* colliderA = manifold.GetColliderA();
		Mat33V mA(*(Mat33V*)&colliderA->GetMatrix());

		manifold.massInv[0] = colliderA->GetInvMass();

		phMathInertia::GetInverseInertiaMatrix(mA,colliderA->GetInvAngInertia(),manifold.inertiaInv[0]);

		for(int c=0;c<manifold.GetNumContacts();c++) {
			phContact &cp = manifold.GetContactPoint(c);
			if (Likely(cp.IsContactActive()))
			{
				Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

				Mat33V constraintAxis;
				MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
				constraintAxis.SetCol0(cp.GetWorldNormal());

				Vec3V targetConstraintVelocity(V_ZERO);

				Vec3V bounceVelocityA;
				colliderA->GetLocalVelocity(cp.GetWorldPosA().GetIntrin128(), bounceVelocityA, cp.GetComponentA());
				bounceVelocityA = Scale(bounceVelocityA, cp.GetElasticityV());
				targetConstraintVelocity += bounceVelocityA;

				// Find the effective depth to modify the effective relative velocity along the contact normal.
				ScalarV normalRelVel = Dot(targetConstraintVelocity, constraintAxis.GetCol0());

				if(!cp.IsConstraint())
				{
					// Allow some penetration, down to GetAllowedPenetration(). This makes the target velocity for the solver not zero, but just enough
					// to reach the allowed penetration at the end of the frame, if the allowed penetration is not already exceeded.
					ScalarV adjustmentDepth = Max(ScalarV(V_ZERO), globals.allowedPenetration-cp.GetDepthV());
					normalRelVel += adjustmentDepth * globals.invTimeStep;
				}

				Vec3V rv( normalRelVel,Dot(targetConstraintVelocity, constraintAxis.GetCol1()),Dot(targetConstraintVelocity, constraintAxis.GetCol2()));

				cp.SetTargetRelVelocity(UnTransformOrtho(constraintAxis, cp.GetTargetRelVelocity()) - rv);

				// Set the transposed coordinate system.
				Mat33V contactLocal;
				Transpose( contactLocal, constraintAxis );

				Mat33V cTwA;
				cTwA.SetCol0( Cross( localPosA, constraintAxis.GetCol0() ) );
				cTwA.SetCol1( Cross( localPosA, constraintAxis.GetCol1() ) );
				cTwA.SetCol2( Cross( localPosA, constraintAxis.GetCol2() ) );
				Transpose( cTwA, cTwA );

				// Set the linear part of the Jacobian for the rigid body to the translational part of the inverse mass matrix.
				Mat33V scaleMatrixA;
				Mat33VFromScale(scaleMatrixA, Vec3VFromF32(manifold.massInv[0]));

				// Multiply translational part of the inverse mass matrix by the local coordinates.
				Multiply(scaleMatrixA,contactLocal,scaleMatrixA);

				// Multiply the linear part of the Jacobian by the coordinate system transpose, and set the output.
				Mat33V linearJacobianA;
				Multiply(linearJacobianA, scaleMatrixA, constraintAxis);

				Mat33V angularJacobianA;
				Multiply(angularJacobianA,cTwA,manifold.inertiaInv[0]);

				Mat33V JJt;
				Mat33V Jt;
				Transpose( Jt, cTwA );
				Multiply( JJt, angularJacobianA, Jt ); // concetentation Jw0*Jt
				Mat33V dinv;
				Add( dinv, JJt, linearJacobianA );

				cp.SetFrictionPlaneProjection(InvScale(dinv.GetCol0(), Vec3V(SplatX(dinv.GetCol0())) ));

				VALIDATE_PHYSICS_ASSERTF(dinv.GetM00f()+dinv.GetM11f()+dinv.GetM22f()>=0.0f,"Contact force solver inverting a bad matrix, diagonals = %f, %f, %f",dinv.GetM00f(),dinv.GetM11f(),dinv.GetM22f());
				LCPSolver::InvertPositiveDefinite( (float*)(&dinv), 4 );
				cp.SetDinv(dinv);

				cp.SetPreviousPush(Vec3V(V_ZERO));
			}
		}
	}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
