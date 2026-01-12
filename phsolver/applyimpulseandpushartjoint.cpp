// 
// phsolver/applyimpulseandpushartjoint.cpp
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

#if __SPU
bool g_UsePushesForJoints = true;
#else
extern bool g_UsePushesForJoints;
#endif

#if !__PS3 && __BANK
extern bool g_DisableJoints;
#endif

void ApplyImpulseAndPushArtJoint(phManifold& manifold, const phForceSolverGlobals& globals)
{
#if !__PS3 && __BANK
	if (g_DisableJoints)
	{
		return;
	}
#endif

	Assert(manifold.GetColliderA() == manifold.GetColliderB());
	if (!manifold.GetColliderA()->IsArticulated())
	{
		return;
	}

	phArticulatedCollider* articulatedCollider = static_cast<phArticulatedCollider*>(manifold.GetColliderA());
	phArticulatedBody* articulatedBody = articulatedCollider->GetBody();
	int numJointLimitDofs = articulatedCollider->GetNumJointLimitDofs();

	for (int limitDofIndex = 0; limitDofIndex < numJointLimitDofs; ++limitDofIndex)
	{
		int jointLimitIndex = articulatedCollider->GetJointLimitIndex(limitDofIndex);
		int jointDofIndex = articulatedCollider->GetJointLimitDofIndex(limitDofIndex);
		ScalarV jointReponse = Invert(articulatedCollider->GetJointLimitResponseV(limitDofIndex));

		Vec3V push = articulatedBody->GetIncrementalJointLimitVelocity(jointLimitIndex, jointDofIndex);

		ScalarV jointExcess = Max(articulatedCollider->GetJointLimitExcess(limitDofIndex) - globals.allowedAnglePenetration * ScalarV(V_HALF), ScalarV(V_ZERO));
		ScalarV neededPush = SelectFT(BoolV(g_UsePushesForJoints), ScalarV(V_ZERO), -(jointExcess + SplatX(push)) * jointReponse);

		ScalarV oldPush = articulatedCollider->GetAccumJointImpulse(limitDofIndex);
		ScalarV newPush = Min(oldPush + neededPush, ScalarV(V_ZERO));
		articulatedCollider->SetAccumJointImpulse(limitDofIndex, newPush);
		neededPush = newPush - oldPush;

		articulatedBody->ApplyIncrementalJointLimitImpulse(jointLimitIndex, jointDofIndex, neededPush);
	}
}

} // namespace rage
