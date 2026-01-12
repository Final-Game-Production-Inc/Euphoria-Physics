// 
// phsolver/applyimpulseartandart.cpp  
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

void ApplyImpulseArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
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
			Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(linkA, localPosA.GetIntrin128());

			int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
			Vec3V localVelocityB = CALL_MEMBER_FN(*bodyB, GetLocalVelocity)(linkB, localPosB.GetIntrin128());

			Vec3V relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();

			ScalarV normalVelocity = Dot(relativeVelocity,cp.GetWorldNormal());
#if !USE_PRECOMPUTE_SEPARATEBIAS
			// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
			ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), Min(cp.GetDepthV() - globals.halfAllowedPenetration, globals.halfAllowedPenetration));
#else
			(void)globals;
			const ScalarV separate = /*globals.separateBias * */ cp.GetSeparateBias_();
#endif

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

			Vec3V compositeImpulse = cp.GetWorldNormal() * Vec3V(impulse);

			relativeVelocity = localVelocityA-localVelocityB - cp.GetTargetRelVelocity();

			ScalarV maxFriction = cp.GetFrictionV() * newImpulse;
			ScalarV tangentVelocity = Dot(relativeVelocity,cp.GetTangent());
			ScalarV friction = -tangentVelocity * cp.GetFrictionDen();

			ScalarV oldFriction = cp.GetAccumFriction();
			ScalarV newFriction = Clamp(oldFriction + friction, -maxFriction, maxFriction);
			friction = newFriction - oldFriction;

			cp.SetAccumImpulse(newImpulse);
			cp.SetAccumFriction(newFriction);

			compositeImpulse += cp.GetTangent() * Vec3V(friction);

			Vec3V impulseA = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleA());
			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(linkA, impulseA, localPosA );

			Vec3V impulseB = compositeImpulse * Vec3VFromF32(manifold.GetMassInvScaleB());
			CALL_MEMBER_FN(*bodyB, ApplyImpulse)(linkB, -impulseB, localPosB );
		}
	}
}

} // namespace rage
