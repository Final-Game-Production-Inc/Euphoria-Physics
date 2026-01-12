// 
// phsolver/applyimpulsemovandmov.cpp
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

void ApplyImpulseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
#if USE_CENTRE_POINT_FOR_CONTACTS
			Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
			localPos*=ScalarV(V_HALF);
			Vec3V localPosA = localPos - colliderA->GetPosition();
			Vec3V localPosB = localPos - colliderB->GetPosition();
#else
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);
			Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

			Vec3V relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();

#if !USE_PRECOMPUTE_SEPARATEBIAS
			// Find a quantity (that we're going to treat as extra velocity to counteract) that's based on the amount of disallowed penetration at the
			//   points of contact.
			// This is sometimes called "bias velocity".
			// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
			ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), Min(cp.GetDepthV() - globals.halfAllowedPenetration, globals.halfAllowedPenetration));
#else
			(void)globals;
			const ScalarV separate = /*globals.separateBias * */ cp.GetSeparateBias_();
#endif

			// Find the actual relative velocity along the normal between the points of contact.
			ScalarV normalVelocity = Dot(relativeVelocity,cp.GetWorldNormal());

			// The descriptively named "num" represents the amount of local relative velocity along the normal that we want to counteract at these
			//   contact points in order to achieve our desired final local relative velocity.
#if POSITIVE_DEPTH_ONLY_NEW
#	if USE_NEGATIVE_DEPTH_TUNABLE
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth() || !g_UseNegativeDepthForceCancel1), ScalarV(V_ZERO), -normalVelocity);
#	else // USE_NEGATIVE_DEPTH_TUNABLE
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -normalVelocity);
#	endif // USE_NEGATIVE_DEPTH_TUNABLE
#else // POSITIVE_DEPTH_ONLY_NEW
			ScalarV num = -normalVelocity;
#endif // POSITIVE_DEPTH_ONLY_NEW

			// Finally, determine the impulse that we would need to apply in order to counteract all of the velocity that we wish to counteract.
			ScalarV impulse = (num + separate) * cp.GetImpulseDen();

			// Update the accumulated impulse in the contact by adding in the impulse we just computed, taking care to ensure that our impulse never
			//   becomes such that it is pulling the two objects together.
			ScalarV oldImpulse = cp.GetAccumImpulse();
			ScalarV newImpulse = Max(oldImpulse + impulse, ScalarV(V_ZERO));
			impulse = newImpulse - oldImpulse;

			cp.SetAccumImpulse(newImpulse);

			Vec3V compositeImpulse = cp.GetWorldNormal() * Vec3V(impulse);

			// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
			//   other equally, so they each receive half of the impulse.
			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));
			velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
			angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));

			// FRICTION

			// Re-calculate the local velocities.
			// NOTE: Do we really need to update the velocities between these two steps?  The similar loop in PreResponseMovAndMov() doesn't do that.
			localVelocityA = velocityA + Cross(angVelA,localPosA);
			localVelocityB = velocityB + Cross(angVelB,localPosB);
			relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();

			// The maximum frictional impulse should be proportional to the normal load between the two objects and the coefficient of friction between them.
			ScalarV maxFriction = cp.GetFrictionV() * newImpulse;
			ScalarV tangentVelocity = Dot(relativeVelocity,cp.GetTangent());

			ScalarV friction = -tangentVelocity * cp.GetFrictionDen();

			// Clamp the frictional impulse according to the maximums and store it in the contact.
			// Interestingly, this doesn't seem to be doing anything to ensure that friction doesn't reverse the direction of the local relative velocity.
			ScalarV oldFriction = cp.GetAccumFriction();
			ScalarV newFriction = Clamp(oldFriction + friction, -maxFriction, maxFriction);
			friction = newFriction - oldFriction;

			cp.SetAccumFriction(newFriction);

			compositeImpulse = cp.GetTangent() * Vec3V(friction);

			// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
			//   other equally, so they each receive half of the impulse.
			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));
			velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
			angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));
		}
	}

	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());

	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
}

#else // ACCUMULATION_SOLVER_MATH == 0

void ApplyImpulseMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Mat33V constraintAxis;
			MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
			constraintAxis.SetCol0(cp.GetWorldNormal());

#if USE_CENTRE_POINT_FOR_CONTACTS
			Vec3V localPos=cp.GetWorldPosA() + cp.GetWorldPosB();
			localPos*=ScalarV(V_HALF);
			Vec3V localPosA = localPos - colliderA->GetPosition();
			Vec3V localPosB = localPos - colliderB->GetPosition();
#else
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();
#endif

			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);
			Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

			Vec3V relativeVelocity;
			relativeVelocity = UnTransformOrtho(constraintAxis,localVelocityA-localVelocityB);

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

			Vec3V worldImpulse = Multiply(constraintAxis, ff);

			// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
			//   other equally, so they each receive half of the impulse.
			velocityA += worldImpulse * Vec3VFromF32(manifold.massInv[0]);
			angVelA += Multiply(manifold.inertiaInv[0], Cross(localPosA,worldImpulse));
			velocityB -= worldImpulse * Vec3VFromF32(manifold.massInv[1]);
			angVelB -= Multiply(manifold.inertiaInv[1], Cross(localPosB,worldImpulse));
		}
	}

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);

	colliderB->SetVelocityOnly(velocityB);
	colliderB->SetAngVelocityOnly(angVelB);
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
