// 
// phsolver/applyimpulseandpushartandart.cpp
//  
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "physics/contact.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseAndPushArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
			int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());

			Vec3V localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());
			Vec3V localPushB = CALL_MEMBER_FN(*bodyB, GetLocalVelocity)(linkB,localPosB.GetIntrin128());

			// PUSH

			Vec3V relativePush = localPushA-localPushB;

			ScalarV normalPush = Dot(relativePush,cp.GetWorldNormal());

			ScalarV push = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), (cp.GetDepthV() - globals.allowedPenetration - normalPush) * cp.GetImpulseDen());

			ScalarV oldPush = cp.GetAccumPush();
			ScalarV newPush = Max(oldPush + push, ScalarV(V_ZERO));
			push = newPush - oldPush;

			Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
			Vec3V massInvScaleB = Vec3VFromF32(manifold.GetMassInvScaleB());

#if TURN_CLAMPING
			Vec3V compositePush = cp.GetWorldNormal() * push;

			Vec3V fullPushA, fullTurnA;
			bodyA->PredictImpulseEffects(linkA, compositePush, localPosA, fullPushA, fullTurnA);

			Vec3V fullPushB, fullTurnB;
			bodyB->PredictImpulseEffects(linkB, compositePush, localPosB, fullPushB, fullTurnB);

			ScalarV fullTurnMagA = Mag(fullTurnA);
			ScalarV fullTurnMagB = Mag(fullTurnB);
			ScalarV biggerTurnMag = Max(fullTurnMagA, fullTurnMagB);
			ScalarV clampedTurnMag = Min(globals.maxTurn, biggerTurnMag);
			BoolV small = IsLessThan(biggerTurnMag, ScalarV(V_FLT_MIN));
			ScalarV turnRatio = clampedTurnMag * Invert(biggerTurnMag);
			turnRatio = SelectFT(small, turnRatio, ScalarV(V_ONE));

			cp.SetAccumPush(newPush * turnRatio);

			Vec3V clampedPushA = compositePush * massInvScaleA * turnRatio;
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, clampedPushA, localPosA);

			Vec3V clampedPushB = compositePush * massInvScaleB * turnRatio;
			CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -clampedPushB, localPosB);

			localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());
			localPushB = CALL_MEMBER_FN(*bodyB, GetLocalVelocity)(linkB,localPosB.GetIntrin128());

			Vec3V newRelativePush = localPushA-localPushB;

			ScalarV newNormalPush = Dot(newRelativePush,cp.GetWorldNormal());

			ScalarV newDisallowedPenetration = cp.GetDepthV() - globals.allowedPenetration - newNormalPush;
			ScalarV massInvA = InvertSafe(bodyA->GetMass(linkA));
			ScalarV massInvB = InvertSafe(bodyB->GetMass(linkB));
			ScalarV linearPush = newDisallowedPenetration * InvertSafe(massInvA + massInvB) * ScalarV(V_HALF);
			linearPush = Max(linearPush, ScalarV(V_ZERO));
			linearPush = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), linearPush);

			//FastAssert(IsLessThanAll(Abs(linearPush), ScalarV(V_FLT_SMALL_3)) || !IsEqualAll(turnRatio, ScalarV(V_ONE)));
			FastAssert(linearPush.Getf() == linearPush.Getf());

			compositePush = linearPush * cp.GetWorldNormal();

			Vec3V pushA         = compositePush    * massInvScaleA;
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, pushA, bodyA->GetLink(linkA).GetPositionV());

			Vec3V pushB         = compositePush    * massInvScaleB;
			CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -pushB, bodyB->GetLink(linkB).GetPositionV());

#else // TURN_CLAMPING
			cp.SetAccumPush(newPush);

			Vec3V compositePush = cp.GetWorldNormal() * Vec3V(push);

			Vec3V pushA         = compositePush    * massInvScaleA;
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, pushA, localPosA);

			Vec3V pushB         = compositePush    * massInvScaleB;
			CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -pushB, localPosB);

#endif // TURN_CLAMPING

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
			record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
			phForceSolver::AddForceSolverDebugRecord(record);
#endif
		}
	}
}


} // namespace rage
