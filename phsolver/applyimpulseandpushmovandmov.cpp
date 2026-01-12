// 
// phsolver/applyimpulseandpushmovandmov.cpp
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

void ApplyImpulseAndPushMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

#if !EARLY_FORCE_SOLVE
	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();
#endif

	Vec3V pushA = colliderA->GetPush();
	Vec3V pushB = colliderB->GetPush();
	Vec3V turnA = colliderA->GetTurn();
	Vec3V turnB = colliderB->GetTurn();

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

#if !EARLY_FORCE_SOLVE
			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);
			Vec3V localVelocityB = velocityB + Cross(angVelB,localPosB);

			Vec3V relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();

			// Find the actual relative velocity along the normal between the points of contact.
			ScalarV normalVelocity = Dot(relativeVelocity,cp.GetWorldNormal());

			// The descriptively named "num" represents the amount of local relative velocity along the normal that we want to counteract at these
			//   contact points in order to achieve our desired final local relative velocity.

			// Skip the impulse portion of the calculation when the depth is negative, and allow the push calculation to proceed as usual.

#if POSITIVE_DEPTH_ONLY
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -normalVelocity);
#else
			ScalarV num = -normalVelocity;
#endif

			ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), cp.GetDepthV() - globals.allowedPenetration);

			// Finally, determine the impulse that we would need to apply in order to counteract all of the velocity that we wish to counteract.
			ScalarV impulse = (num + separate) * cp.GetImpulseDen();

			// Update the accumulated impulse in the contact by adding in the impulse we just computed, taking care to ensure that our impulse never
			//   becomes such that it is pulling the two objects together.
			ScalarV oldImpulse = cp.GetAccumImpulse();
			ScalarV newImpulse = Max(oldImpulse + impulse, ScalarV(V_ZERO));
			impulse = newImpulse - oldImpulse;

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

#if POSITIVE_DEPTH_ONLY
			ScalarV friction = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -tangentVelocity * cp.GetFrictionDen());
#else
			ScalarV friction = -tangentVelocity * cp.GetFrictionDen();
#endif

			// Clamp the frictional impulse according to the maximums and store it in the contact.
			// Interestingly, this doesn't seem to be doing anything to ensure that friction doesn't reverse the direction of the local relative velocity.
			ScalarV oldFriction = cp.GetAccumFriction();
			ScalarV newFriction = Clamp(oldFriction + friction, -maxFriction, maxFriction);
			friction = newFriction - oldFriction;

			compositeImpulse = cp.GetTangent() * Vec3V(friction);

			// Adjust the velocities of bodies A and B according to the impulse from this contact.  By Newton's third law, each object pushes on the
			//   other equally, so they each receive half of the impulse.
			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));
			velocityB -= compositeImpulse * Vec3VFromF32(manifold.GetMassInvB());
			angVelB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositeImpulse));

			cp.SetAccumImpulse(newImpulse);
			cp.SetAccumFriction(newFriction);
#endif

			// PUSH

			Vec3V localPushA = pushA + Cross(turnA,localPosA);
			Vec3V localPushB = pushB + Cross(turnB,localPosB);

			Vec3V relativePush = localPushA-localPushB;

			ScalarV normalPush = Dot(relativePush,cp.GetWorldNormal());

			// Start by assuming that we are going to apply a push to counteract all of the disallowed penetration.
			ScalarV disallowedPenetration = cp.GetDepthV() - globals.allowedPenetration - normalPush;
			ScalarV pushToApply = disallowedPenetration * cp.GetImpulseDen();
			pushToApply = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), pushToApply);

			// Try to add in pushToApply, but make sure that push for this contact point never becomes 'suction'.
			ScalarV oldAccumPush = cp.GetAccumPush();
			ScalarV newAccumPush = Max(oldAccumPush + pushToApply, ScalarV(V_ZERO));
			pushToApply = newAccumPush - oldAccumPush;

#if TURN_CLAMPING
			Vec3V compositePush = cp.GetWorldNormal() * pushToApply;

			Vec3V fullPushA = compositePush * Vec3VFromF32(manifold.GetMassInvA());
			Vec3V fullTurnA = Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositePush));

			Vec3V fullPushB = compositePush * Vec3VFromF32(manifold.GetMassInvB());
			Vec3V fullTurnB = Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositePush));

			ScalarV fullTurnMagA = Mag(fullTurnA);
			ScalarV fullTurnMagB = Mag(fullTurnB);
			ScalarV biggerTurnMag = Max(fullTurnMagA, fullTurnMagB);
			ScalarV clampedTurnMag = Min(globals.maxTurn, biggerTurnMag);
			BoolV small = IsLessThan(biggerTurnMag, ScalarV(V_FLT_MIN));
			ScalarV turnRatio = clampedTurnMag * Invert(biggerTurnMag);
			turnRatio = SelectFT(small, turnRatio, ScalarV(V_ONE));

			cp.SetAccumPush(newAccumPush * turnRatio);

			pushA += fullPushA * turnRatio;
			turnA += fullTurnA * turnRatio;

			pushB -= fullPushB * turnRatio;
			turnB -= fullTurnB * turnRatio;

			Vec3V newLocalPushA = pushA + Cross(turnA,localPosA);
			Vec3V newLocalPushB = pushB + Cross(turnB,localPosB);

			Vec3V newRelativePush = newLocalPushA-newLocalPushB;

			ScalarV newNormalPush = Dot(newRelativePush,cp.GetWorldNormal());

			ScalarV newDisallowedPenetration = cp.GetDepthV() - globals.allowedPenetration - newNormalPush;
			ScalarV massInvA = ScalarV(manifold.GetMassInvA());
			ScalarV massInvB = ScalarV(manifold.GetMassInvB());
			ScalarV linearPush = newDisallowedPenetration * InvertSafe(massInvA + massInvB) * ScalarV(V_HALF);
			linearPush = Max(linearPush, ScalarV(V_ZERO));
			linearPush = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), linearPush);

			//FastAssert(IsLessThanAll(Abs(linearPush), ScalarV(V_FLT_SMALL_3)) || !IsEqualAll(turnRatio, ScalarV(V_ONE)));
			FastAssert(linearPush.Getf() == linearPush.Getf());

			pushA += linearPush * cp.GetWorldNormal() * massInvA;
			pushB -= linearPush * cp.GetWorldNormal() * massInvB;
#else // TURN_CLAMPING
			cp.SetAccumPush(newAccumPush);

			Vec3V compositePush = cp.GetWorldNormal() * pushToApply;

			pushA += compositePush * Vec3VFromF32(manifold.GetMassInvA());
			turnA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositePush));

			pushB -= compositePush * Vec3VFromF32(manifold.GetMassInvB());
			turnB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositePush));
#endif
			// Clear the target relative velocity 
			if(!manifold.IsConstraint())
			{
				cp.SetTargetRelVelocity(Vec3V(V_ZERO));
			}
		}
	}

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	if(manifold.GetNumContacts()>0)
	{
		record.SetFunctionName("ContactsMovAndMov");
		record.SetColliders(colliderA,colliderB);
		record.SetNormal(manifold.GetContactPoint(0).GetWorldNormal());
		record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	}
#endif

#if !EARLY_FORCE_SOLVE
	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
	colliderB->SetVelocityOnly(velocityB.GetIntrin128());
	colliderB->SetAngVelocityOnly(angVelB.GetIntrin128());
#endif

	colliderA->SetPush(pushA.GetIntrin128());
	colliderA->SetTurn(turnA.GetIntrin128());
	colliderB->SetPush(pushB.GetIntrin128());
	colliderB->SetTurn(turnB.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	if(manifold.GetNumContacts()>0)
	{
		record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
		phForceSolver::AddForceSolverDebugRecord(record);
	}
#endif
}

#else // ACCUMULATION_SOLVER_MATH == 0

void ApplyImpulseAndPushMovAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();
	phCollider* colliderB = manifold.GetColliderB();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();
	Vec3V pushA = colliderA->GetPush();
	Vec3V pushB = colliderB->GetPush();
	Vec3V turnA = colliderA->GetTurn();
	Vec3V turnB = colliderB->GetTurn();

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

			Vec3V relativeVelocity = UnTransformOrtho(constraintAxis,localVelocityA-localVelocityB);

			relativeVelocity = Subtract(relativeVelocity, cp.GetTargetRelVelocity());

			ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.m_SeparateBias);
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), cp.GetDepthV() - globals.allowedPenetration);
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
			velocityB -= worldImpulse * Vec3VFromF32(manifold.massInv[1]);
			angVelB -= Multiply(manifold.inertiaInv[1], Cross(localPosB,worldImpulse));

			if (manifold.m_UsePushes)
			{
				Vec3V localPushA = pushA + Cross(turnA,localPosA);
				Vec3V localPushB = pushB + Cross(turnB,localPosB);

				Vec3V relativePush = UnTransformOrtho(constraintAxis,localPushA-localPushB);

				const ScalarV allowedDepth = globals.allowedPenetration;
				const ScalarV excessDepth = cp.GetDepthV() - allowedDepth;
				const ScalarV saveForNextTimeDepth = Max(ScalarV(V_ZERO),Min(ScalarV(V_HALF)*excessDepth,allowedDepth));
				// Do not use the post-collision normal relative speed to reduce the push relative speed to abool any amount of the needed
				// push that will be removed by the post-collision velocity over the next frame, because the collider's position won't be updated
				// from its post-collision velocity until the next simulator update.
				ScalarV effectiveDepth = excessDepth-saveForNextTimeDepth;

				Vec3V neededPush = cp.GetWorldNormal() * effectiveDepth;

				neededPush = UnTransformOrtho(constraintAxis, neededPush);

				relativePush = Subtract( relativePush, neededPush );

				Vec3V p = Multiply(cp.GetDinv(), relativePush);
				p = Negate(p);
				Vec3V totalConstraintPush(cp.GetPreviousPush());

				p = Subtract( p, totalConstraintPush );
				p *= Vec3V(V_X_AXIS_WZERO);

				VecBoolV isSeparating = IsLessThan( p, Vec3V(V_ZERO) );
				Vec3V pp = SelectFT( isSeparating, p, Vec3V(V_ZERO) );

				pp = Add( pp, totalConstraintPush );

				cp.SetPreviousPush(Subtract( totalConstraintPush, pp ));

				Vec3V worldPush = Multiply( constraintAxis, pp );

				pushA += worldPush * Vec3VFromF32(manifold.massInv[0]);
				turnA += Multiply(manifold.inertiaInv[0], Cross(localPosA,worldPush));

				pushB -= worldPush * Vec3VFromF32(manifold.massInv[1]);
				turnB -= Multiply(manifold.inertiaInv[1], Cross(localPosB,worldPush));
			}

			// Clear the target relative velocity 
			if(!manifold.IsConstraint())
			{
				cp.SetTargetRelVelocity(Vec3V(V_ZERO));
			}
		}
	}

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ContactsMovAndMov");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(manifold.GetContactPoint(0).GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);

	colliderA->SetPush(pushA.GetIntrin128());
	colliderA->SetTurn(turnA.GetIntrin128());

	colliderB->SetVelocityOnly(velocityB);
	colliderB->SetAngVelocityOnly(angVelB);

	colliderB->SetPush(pushB.GetIntrin128());
	colliderB->SetTurn(turnB.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
