// 
// phsolver/applyimpulsemovandfix.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()



namespace rage {

#if ACCUMULATION_SOLVER_MATH == 0

void ApplyImpulseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);

			//if(solverIO.deformMeshEnable && cp.subDataType1 == SubData::SubDataFacetLocal) {
			//	velocityB += rotate(stateB.fQ,cp.localVelocity[1]);
			//}

			Vec3V relativeVelocity = localVelocityA - cp.GetTargetRelVelocity();

			// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
#if !USE_PRECOMPUTE_SEPARATEBIAS
			ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), Min(cp.GetDepthV() - globals.halfAllowedPenetration, globals.halfAllowedPenetration));
#else
			(void)globals;
			const ScalarV separate = /*globals.separateBias * */ cp.GetSeparateBias_();
#endif
			ScalarV normalVelocity = Dot(relativeVelocity,cp.GetWorldNormal());

#if POSITIVE_DEPTH_ONLY_NEW
#	if USE_NEGATIVE_DEPTH_TUNABLE
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth() || !g_UseNegativeDepthForceCancel1), ScalarV(V_ZERO), -normalVelocity);
#	else // USE_NEGATIVE_DEPTH_TUNABLE
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -normalVelocity);
#	endif // USE_NEGATIVE_DEPTH_TUNABLE
#else // POSITIVE_DEPTH_ONLY_NEW
			ScalarV num = -normalVelocity;
#endif // POSITIVE_DEPTH_ONLY_NEW

			ScalarV impulse = (num + separate) * cp.GetImpulseDen();

			ScalarV oldImpulse = cp.GetAccumImpulse();
			ScalarV newImpulse = Max(oldImpulse + impulse, ScalarV(V_ZERO));
			impulse = newImpulse - oldImpulse;

			cp.SetAccumImpulse(newImpulse);

			Vec3V compositeImpulse = cp.GetWorldNormal() * Vec3V(impulse);

			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));

			// FRICTION

			localVelocityA = velocityA + Cross(angVelA,localPosA);
			//if(solverIO.deformMeshEnable && cp.subDataType1 == SubData::SubDataFacetLocal) {
			//	velocityB += rotate(stateB.fQ,cp.localVelocity[1]);
			//}

			relativeVelocity = localVelocityA - cp.GetTargetRelVelocity();

			ScalarV maxFriction = cp.GetFrictionV() * newImpulse;
			ScalarV tangentVelocity = Dot(relativeVelocity,cp.GetTangent());
			ScalarV friction = -tangentVelocity * cp.GetFrictionDen();

			ScalarV oldFriction = cp.GetAccumFriction();
			ScalarV newFriction = Clamp(oldFriction + friction, -maxFriction, maxFriction);
			friction = newFriction - oldFriction;
			cp.SetAccumFriction(newFriction);

			compositeImpulse = cp.GetTangent() * Vec3V(friction);

			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));
		}
	}

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
}

#else // ACCUMULATION_SOLVER_MATH == 0

void ApplyImpulseMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Mat33V constraintAxis;
			MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
			constraintAxis.SetCol0(cp.GetWorldNormal());

			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);

			Vec3V relativeVelocity = UnTransformOrtho(constraintAxis,localVelocityA);

			relativeVelocity = Subtract(relativeVelocity, cp.GetTargetRelVelocity());

			ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.m_SeparateBias);
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), Min(cp.GetDepthV() - globals.halfAllowedPenetration, globals.halfAllowedPenetration));
			Vec3V separateImpulse = UnTransformOrtho(constraintAxis,separate * cp.GetWorldNormal());

			relativeVelocity = Subtract(relativeVelocity, separateImpulse);

			// 	Vec3V f = CalculateConstraintImpulse(constraintVelocity);
			Vec3V f = Multiply(cp.GetDinv(),relativeVelocity);
			f = Negate(f);
			Vec3V totalConstraintImpulse(cp.GetPreviousSolution());

			f = Subtract( f, totalConstraintImpulse );

			Vec3V ff = INTRIN_TO_VEC3V(SolveWithFriction( VEC3V_TO_INTRIN(f), VEC3V_TO_INTRIN(cp.GetFrictionPlaneProjection()), cp.GetFrictionV().GetIntrin128() ));

			ff = Add( ff, totalConstraintImpulse );

			cp.SetPreviousSolution(Subtract( totalConstraintImpulse, ff ));

			Vec3V worldImpulse = Multiply( constraintAxis, ff );

			// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
			//   other equally, so they each receive half of the impulse.
			velocityA += worldImpulse * Vec3VFromF32(manifold.massInv[0]);
			angVelA += Multiply(manifold.inertiaInv[0], Cross(localPosA,worldImpulse));
		}
	}

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
