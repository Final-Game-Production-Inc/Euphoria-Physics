// 
// phsolver/applyimpulsefixedrotationartandart.cpp 
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

void ApplyImpulseFixedRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& cp = manifold.GetContactPoint(0);
	Assert(cp.IsContactActive());
	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Vec3V angVelA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
	Vec3V angVelB = CALL_MEMBER_FN(*bodyB, GetAngularVelocity)(linkB);

	Vec3V ang = Clamp(cp.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * cp.GetWorldNormal();
	ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/; // Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
	Vec3V separate = separateBias * ang;

	Vec3V deltaAngVel = angVelA - angVelB - separate;
	Mat33V angInertia = manifold.GetConstraintMatrix();
	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	cp.SetPreviousSolution(angImpulse);

	Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
	Vec3V angImpulseA   = angImpulse * massInvScaleA;
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, angImpulseA.GetIntrin128());

	Vec3V massInvScaleB = Vec3VFromF32(manifold.GetMassInvScaleB());
	Vec3V angImpulseB   = angImpulse * massInvScaleB;
	CALL_MEMBER_FN(*bodyB, ApplyAngImpulse)(linkB, (-angImpulseB).GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),colliderB->GetVelocity(),colliderA->GetAngVelocity(),colliderB->GetAngVelocity());
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
