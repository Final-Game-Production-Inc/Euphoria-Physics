// 
// phsolver/applyimpulseandpushartandfix.cpp
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseAndPushArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Mat33V constraintAxis;
			MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
			constraintAxis.SetCol0(cp.GetWorldNormal());

			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			int link = colliderA->GetLinkFromComponent(manifold.GetComponentA());

			Vec3V localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(link,localPosA.GetIntrin128());

			// PUSH

			Vec3V pushArticulated;
			pushArticulated = UnTransformOrtho(constraintAxis,localPushA);

			Vec3V worldPush(V_ZERO);
			const ScalarV allowedDepth = globals.allowedPenetration + colliderA->GetExtraAllowedPenetrationV();
			const ScalarV excessDepth = cp.GetDepthV() - allowedDepth;
			const ScalarV saveForNextTimeDepth = Max(ScalarV(V_ZERO),Min(ScalarV(V_HALF)*excessDepth,allowedDepth));
			// Do not use the post-collision normal relative speed to reduce the push relative speed to abool any amount of the needed
			// push that will be removed by the post-collision velocity over the next frame, because the collider's position won't be updated
			// from its post-collision velocity until the next simulator update.
			ScalarV effectiveDepth = excessDepth-saveForNextTimeDepth;

			Vec3V neededPush = cp.GetWorldNormal() * effectiveDepth;

			neededPush = UnTransformOrtho(constraintAxis, neededPush);

			pushArticulated = Subtract( pushArticulated, neededPush );

			Mat33V dinv = cp.GetDinv();
			Vec3V p = Multiply(dinv,pushArticulated);
			p = Negate(p);
			Vec3V totalConstraintPush(cp.GetPreviousPush());

			p = Subtract( p, totalConstraintPush );

			Vec3V frictionPlaneProjection = cp.GetFrictionPlaneProjection();

			Vec3V push = INTRIN_TO_VEC3V(SolveWithFriction( VEC3V_TO_INTRIN(p), VEC3V_TO_INTRIN(frictionPlaneProjection), cp.GetFrictionV().GetIntrin128() ));
			push = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), push);

			Vec3V adjustedPush = Add( push, totalConstraintPush );

#if TURN_CLAMPING
			Vec3V compositePush = Multiply(constraintAxis, adjustedPush);

			Vec3V fullPush, fullTurn;
			bodyA->PredictImpulseEffects(link, compositePush, localPosA, fullPush, fullTurn);

			ScalarV fullTurnMag = Mag(fullTurn);
			ScalarV clampedTurnMag = Min(globals.maxTurn, fullTurnMag);
			BoolV small = IsLessThan(fullTurnMag, ScalarV(V_FLT_MIN));
			ScalarV turnRatio = clampedTurnMag * Invert(fullTurnMag);
			turnRatio = SelectFT(small, turnRatio, ScalarV(V_ONE));

			cp.SetPreviousPush(-push * turnRatio);

			Vec3V clampedPush = compositePush * turnRatio;

			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, clampedPush, localPosA);

			localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(link,localPosA.GetIntrin128());

			ScalarV newNormalPush = Dot(localPushA,cp.GetWorldNormal());

			const ScalarV newExcessDepth = cp.GetDepthV() - allowedDepth;
			const ScalarV newSaveForNextTimeDepth = Max(ScalarV(V_ZERO),Min(ScalarV(V_HALF)*newExcessDepth,allowedDepth));
			ScalarV linearPush = newExcessDepth-newSaveForNextTimeDepth - newNormalPush;

			linearPush = Max(linearPush, ScalarV(V_ZERO));
			linearPush = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), linearPush);

			//FastAssert(IsLessThanAll(Abs(linearPush), ScalarV(V_FLT_SMALL_3)) || !IsEqualAll(turnRatio, ScalarV(V_ONE)));
			FastAssert(linearPush.Getf() == linearPush.Getf());

			worldPush = bodyA->GetMass(link) * linearPush * cp.GetWorldNormal();

			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, worldPush, bodyA->GetLink(link).GetPositionV());

#else // TURN_CLAMPING
			cp.SetPreviousPush(-push);

			worldPush = Multiply(constraintAxis, adjustedPush);

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
			phForceSolverDebugRecord record;
			record.SetFunctionName("ArtContactsArtAndFix");
			record.SetColliders(colliderA,0);
			record.SetNormal(cp.GetWorldNormal());
			record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, worldPush, localPosA);
#endif

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
			record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
			phForceSolver::AddForceSolverDebugRecord(record);
#endif
			// Clear the target relative velocity 
			if(!manifold.IsConstraint())
			{
				cp.SetTargetRelVelocity(Vec3V(V_ZERO));
			}
		}
	}
}

} // namespace rage
