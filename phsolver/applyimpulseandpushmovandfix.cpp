// 
// phsolver/applyimpulseandpushmovandfix.cpp 
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

void ApplyImpulseAndPushMovAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phCollider* colliderA = manifold.GetColliderA();

#if !EARLY_FORCE_SOLVE
	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
#endif

	Vec3V pushA = colliderA->GetPush();
	Vec3V turnA = colliderA->GetTurn();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			const Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();

#if !EARLY_FORCE_SOLVE
			Vec3V localVelocityA = velocityA + Cross(angVelA,localPosA);

			//if(solverIO.deformMeshEnable && cp.subDataType1 == SubData::SubDataFacetLocal) {
			//	velocityB += rotate(stateB.fQ,cp.localVelocity[1]);
			//}

			Vec3V relativeVelocity = localVelocityA - cp.GetTargetRelVelocity();

			ScalarV normalVelocity = Dot(relativeVelocity,cp.GetWorldNormal());

			ScalarV separateBias = globals.separateBias * ScalarVFromF32(manifold.GetSeparateBias());
			ScalarV allowedPenetration = globals.allowedPenetration + colliderA->GetExtraAllowedPenetrationV();
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), cp.GetDepthV() - allowedPenetration);

#if POSITIVE_DEPTH_ONLY
			ScalarV num = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -normalVelocity);
#else
			ScalarV num = -normalVelocity;
#endif

			ScalarV impulse = (num + separate) * cp.GetImpulseDen();

			ScalarV oldImpulse = cp.GetAccumImpulse();
			ScalarV newImpulse = Max(oldImpulse + impulse, ScalarV(V_ZERO));
			impulse = newImpulse - oldImpulse;

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

#if POSITIVE_DEPTH_ONLY
			ScalarV friction = SelectFT(BoolV(cp.IsPositiveDepth()), ScalarV(V_ZERO), -tangentVelocity * cp.GetFrictionDen());
#else
			ScalarV friction = -tangentVelocity * cp.GetFrictionDen();
#endif

			ScalarV oldFriction = cp.GetAccumFriction();
			ScalarV newFriction = Clamp(oldFriction + friction, -maxFriction, maxFriction);
			friction = newFriction - oldFriction;

			compositeImpulse = cp.GetTangent() * Vec3V(friction);

			velocityA += compositeImpulse * Vec3VFromF32(manifold.GetMassInvA());
			angVelA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositeImpulse));

			cp.SetAccumImpulse(newImpulse);
			cp.SetAccumFriction(newFriction);
#endif

			// PUSH

			Vec3V localPushA = pushA + Cross(turnA,localPosA);

			ScalarV normalPush = Dot(localPushA,cp.GetWorldNormal());

			ScalarV push = (cp.GetDepthV() - globals.allowedPenetration - normalPush) * cp.GetImpulseDen();
			push = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), push);

			ScalarV oldPush = cp.GetAccumPush();
			ScalarV newPush = Max(oldPush + push, ScalarV(V_ZERO));
			push = newPush - oldPush;

#if TURN_CLAMPING
			Vec3V compositePush = cp.GetWorldNormal() * push;

			Vec3V fullPush = compositePush * Vec3VFromF32(manifold.GetMassInvA());
			Vec3V fullTurn = Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositePush));

			ScalarV fullTurnMag = Mag(fullTurn);
			ScalarV clampedTurnMag = Min(globals.maxTurn, fullTurnMag);
			BoolV small = IsLessThan(fullTurnMag, ScalarV(V_FLT_MIN));
			ScalarV turnRatio = clampedTurnMag * Invert(fullTurnMag);
			turnRatio = SelectFT(small, turnRatio, ScalarV(V_ONE));
			Vec3V clampedTurn = fullTurn * turnRatio;
			Vec3V clampedPush = fullPush * turnRatio;

			cp.SetAccumPush(newPush * turnRatio);

			pushA += clampedPush;
			turnA += clampedTurn;

			Vec3V newLocalPushA = pushA + Cross(turnA,localPosA);

			ScalarV newNormalPush = Dot(newLocalPushA,cp.GetWorldNormal());

			ScalarV linearPush = (cp.GetDepthV() - globals.allowedPenetration - newNormalPush);
			linearPush = Max(linearPush, ScalarV(V_ZERO));
			linearPush = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), linearPush);

			//FastAssert(IsLessThanAll(Abs(linearPush), ScalarV(V_FLT_SMALL_3)) || !IsEqualAll(turnRatio, ScalarV(V_ONE)));
			FastAssert(linearPush.Getf() == linearPush.Getf());

			pushA += linearPush * cp.GetWorldNormal();

#else // TURN_CLAMPING
			cp.SetAccumPush(newPush);

			Vec3V compositePush = cp.GetWorldNormal() * push;

			pushA += compositePush * Vec3VFromF32(manifold.GetMassInvA());
			turnA += Multiply(manifold.GetInertiaInvA(), Cross(localPosA,compositePush));
#endif // TURN_CLAMPING

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
		record.SetFunctionName("ContactsMovAndFix");
		record.SetColliders(colliderA,0);
		record.SetNormal(manifold.GetContactPoint(0).GetWorldNormal());
		record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	}
#endif

#if !EARLY_FORCE_SOLVE
	colliderA->SetVelocityOnly(velocityA.GetIntrin128());
	colliderA->SetAngVelocityOnly(angVelA.GetIntrin128());
#endif

	colliderA->SetPush(pushA.GetIntrin128());
	colliderA->SetTurn(turnA.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	if(manifold.GetNumContacts()>0)
	{
		record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
		phForceSolver::AddForceSolverDebugRecord(record);
	}
#endif
}

#else // ACCUMULATION_SOLVER_MATH == 0

void ApplyImpulseAndPushMovAndFix(phManifold& manifold, const phForceSolverGlobals& UNUSED_PARAM(globals))
{
	phCollider* colliderA = manifold.GetColliderA();

	Vec3V velocityA = colliderA->GetVelocity();
	Vec3V angVelA = colliderA->GetAngVelocity();
	Vec3V pushA = colliderA->GetPush();
	Vec3V turnA = colliderA->GetTurn();

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

			if (manifold.m_UsePushes)
			{
				Vec3V localPushA = pushA + Cross(turnA,localPosA);

				Vec3V relativePush = UnTransformOrtho(constraintAxis,localPushA);

				const ScalarV allowedDepth = globals.allowedPenetration;
				const ScalarV excessDepth = cp.GetDepthV() - allowedDepth;
				const ScalarV saveForNextTimeDepth = Max(ScalarV(V_ZERO),Min(ScalarV(V_HALF)*excessDepth,allowedDepth));
				// Do not use the post-collision normal relative speed to reduce the push relative speed to abool any amount of the needed
				// push that will be removed by the post-collision velocity over the next frame, because the collider's position won't be updated
				// from its post-collision velocity until the next simulator update.
				ScalarV effectiveDepth = excessDepth-saveForNextTimeDepth;

				Vec3V neededPush = cp.GetWorldNormal() * effectiveDepth;

				neededPush = UnTransformOrtho( constraintAxis, neededPush );

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
	record.SetFunctionName("ContactsMovAndFix");
	record.SetColliders(colliderA,colliderB);
	record.SetNormal(manifold.GetContactPoint(0).GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
#endif

	colliderA->SetVelocityOnly(velocityA);
	colliderA->SetAngVelocityOnly(angVelA);

	colliderA->SetPush(pushA.GetIntrin128());
	colliderA->SetTurn(turnA.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

#endif // ACCUMULATION_SOLVER_MATH == 0

} // namespace rage
