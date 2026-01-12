// 
// phsolver/applyimpulseartjoint.cpp 
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

#if !__PS3 && __BANK
extern bool g_DisableJoints;
#endif

void ApplyImpulseArtJoint(phManifold& manifold, const phForceSolverGlobals& globals)
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
	ScalarV halfAllowedAnglePenetration = globals.allowedAnglePenetration * ScalarV(V_HALF);

	for (int limitDofIndex = 0; limitDofIndex < numJointLimitDofs; ++limitDofIndex)
	{
		int jointLimitIndex = articulatedCollider->GetJointLimitIndex(limitDofIndex);
		int jointDofIndex = articulatedCollider->GetJointLimitDofIndex(limitDofIndex);
		ScalarV jointReponse = Invert(articulatedCollider->GetJointLimitResponseV(limitDofIndex));

		ScalarV jointExcess = Max(ScalarV(V_ZERO), Min(articulatedCollider->GetJointLimitExcess(limitDofIndex) - halfAllowedAnglePenetration, halfAllowedAnglePenetration));
		ScalarV separate = globals.separateBias * Max(ScalarV(V_ZERO), jointExcess);

		Vec3V velocity = articulatedBody->GetIncrementalJointLimitVelocity(jointLimitIndex, jointDofIndex);
		ScalarV constraintVelocity = SplatX(velocity); // Incremental velocity from impulses (pos = separating)

		ScalarV constraintImpulse = -(separate + constraintVelocity) * jointReponse;

		ScalarV oldImpulse = articulatedCollider->GetAccumJointImpulse(limitDofIndex);
		ScalarV newImpulse = Min(oldImpulse + constraintImpulse, ScalarV(V_ZERO));
		articulatedCollider->SetAccumJointImpulse(limitDofIndex, newImpulse);
		constraintImpulse = newImpulse - oldImpulse;

		articulatedBody->ApplyIncrementalJointLimitImpulse(jointLimitIndex, jointDofIndex, constraintImpulse);
	}
}

} // namespace rage
