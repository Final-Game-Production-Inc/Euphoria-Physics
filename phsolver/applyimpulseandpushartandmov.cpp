// 
// phsolver/applyimpulseandpushartandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseAndPushArtAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	phCollider* colliderB = manifold.GetColliderB();
#if !EARLY_FORCE_SOLVE
	Vec3V velocityB = colliderB->GetVelocity();
	Vec3V angVelB = colliderB->GetAngVelocity();
#endif // !EARLY_FORCE_SOLVE
	Vec3V pushB = colliderB->GetPush();
	Vec3V turnB = colliderB->GetTurn();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			Vec3V localPosB = cp.GetWorldPosB() - colliderB->GetPosition();

			int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());

			Vec3V localPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());

			// PUSH

			Vec3V localPushB = pushB + Cross(turnB,localPosB);
			Vec3V relativePush = localPushA-localPushB;

			ScalarV normalPush = Dot(relativePush,cp.GetWorldNormal());

			ScalarV push = (cp.GetDepthV() - globals.allowedPenetration - normalPush) * cp.GetImpulseDen();

			ScalarV oldPush = cp.GetAccumPush();
			ScalarV newPush = Max(oldPush + push, ScalarV(V_ZERO));
			push = newPush - oldPush;

#if TURN_CLAMPING
			Vec3V compositePush = cp.GetWorldNormal() * push;

			Vec3V fullPushA, fullTurnA;
			bodyA->PredictImpulseEffects(linkA, compositePush, localPosA, fullPushA, fullTurnA);

			Vec3V fullPushB = compositePush * Vec3VFromF32(manifold.GetMassInvB());
			Vec3V fullTurnB = Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositePush));

			ScalarV fullTurnMagA = Mag(fullTurnA);
			ScalarV fullTurnMagB = Mag(fullTurnB);
			ScalarV biggerTurnMag = Max(fullTurnMagA, fullTurnMagB);
			ScalarV clampedTurnMag = Min(globals.maxTurn, biggerTurnMag);
			BoolV small = IsLessThan(biggerTurnMag, ScalarV(V_FLT_MIN));
			ScalarV turnRatio = clampedTurnMag * Invert(biggerTurnMag);
			turnRatio = SelectFT(small, turnRatio, ScalarV(V_ONE));

			cp.SetAccumPush(newPush * turnRatio);

			Vec3V clampedPushA = compositePush * ScalarV(manifold.GetMassInvScaleA()) * turnRatio;
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, clampedPushA, localPosA);

			pushB -= fullPushB * turnRatio;
			turnB -= fullTurnB * turnRatio;

			Vec3V newLocalPushA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA,localPosA.GetIntrin128());
			Vec3V newLocalPushB = pushB + Cross(turnB,localPosB);

			Vec3V newRelativePush = newLocalPushA-newLocalPushB;

			ScalarV newNormalPush = Dot(newRelativePush,cp.GetWorldNormal());

			ScalarV newDisallowedPenetration = cp.GetDepthV() - globals.allowedPenetration - newNormalPush;
			ScalarV massInvA = InvertSafe(bodyA->GetMass(linkA));
			ScalarV massInvB = ScalarV(manifold.GetMassInvB());
			ScalarV linearPush = newDisallowedPenetration * InvertSafe(massInvA + massInvB) * ScalarV(V_HALF);
			linearPush = Max(linearPush, ScalarV(V_ZERO));
			linearPush = SelectFT(manifold.GetUsePushesV(), ScalarV(V_ZERO), linearPush);

			//FastAssert(IsLessThanAll(Abs(linearPush), ScalarV(V_FLT_SMALL_3)) || !IsEqualAll(turnRatio, ScalarV(V_ONE)));
			FastAssert(linearPush.Getf() == linearPush.Getf());

			Vec3V pushA		= linearPush * cp.GetWorldNormal() * massInvA * Vec3VFromF32(manifold.GetMassInvScaleA());			
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, pushA, localPosA);

			Vec3V linearPushB = linearPush * cp.GetWorldNormal() * massInvB;
			pushB -= linearPushB;
#else // TURN_CLAMPING
			cp.SetAccumPush(newPush);

			Vec3V compositePush = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), cp.GetWorldNormal() * Vec3V(push));

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
			phForceSolverDebugRecord record;
			record.SetFunctionName("ArtContactsArtAndMov");
			record.SetColliders(colliderA,0);
			record.SetNormal(cp.GetWorldNormal());
			record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif
				
			Vec3V pushA    = compositePush    * Vec3VFromF32(manifold.GetMassInvScaleA());			
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, pushA.GetIntrin128(), localPosA.GetIntrin128());

			pushB -= compositePush * Vec3VFromF32(manifold.GetMassInvB());
			turnB -= Multiply(manifold.GetInertiaInvB(), Cross(localPosB,compositePush));
#endif // TURN_CLAMPING

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
			record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
			phForceSolver::AddForceSolverDebugRecord(record);
#endif
		}
	}

	colliderB->SetPush(pushB.GetIntrin128());
	colliderB->SetTurn(turnB.GetIntrin128());
}

} // namespace rage
