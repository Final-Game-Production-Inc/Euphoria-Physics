// 
// phsolver/applyimpulsesliderotationartandart.cpp 
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

void ApplyImpulseSlideRotationArtAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();
	phArticulatedCollider* colliderB = (phArticulatedCollider*)manifold.GetColliderB();
	Assert(colliderB->IsArticulated());
	phArticulatedBody* bodyB = colliderB->GetBody();

	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Vec3V angVelA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	int linkB = colliderB->GetLinkFromComponent(manifold.GetComponentB());
	Vec3V angVelB = CALL_MEMBER_FN(*bodyB, GetAngularVelocity)(linkB);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());
	Vec3V worldNormalA = contactPoint.GetWorldNormal();

	Mat33V angInertia = manifold.GetConstraintMatrix();

	ScalarV separateBias = globals.separateBias; // * ScalarVFromF32(manifold.GetSeparateBias());
	Vec3V deltaAng = Clamp(contactPoint.GetDepthV() - globals.allowedAnglePenetration, ScalarV(V_ZERO), globals.allowedAnglePenetration) * contactPoint.GetWorldNormal();
	Vec3V separate = separateBias * deltaAng;

	Vec3V deltaAngVel = angVelA - angVelB - separate;

	Assert(manifold.GetNumContacts() == 1);

	deltaAngVel = Scale(worldNormalA,Dot(deltaAngVel,worldNormalA));

	Vec3V angImpulse = Multiply(angInertia, deltaAngVel);

	Vec3V oldAngImpulse = contactPoint.GetPreviousSolution();
	Vec3V newAngImpulse = oldAngImpulse + angImpulse;
	ScalarV impulseAroundAxis = Dot(newAngImpulse, worldNormalA);
	newAngImpulse = SelectFT(IsGreaterThan(impulseAroundAxis, ScalarV(V_ZERO)), Vec3V(V_ZERO), newAngImpulse);
	contactPoint.SetPreviousSolution(newAngImpulse);
	angImpulse = newAngImpulse - oldAngImpulse;

	Vec3V massInvScaleA = Vec3VFromF32(manifold.GetMassInvScaleA());
	Vec3V angImpulseA   = angImpulse * massInvScaleA;
	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, angImpulseA.GetIntrin128());

	Vec3V massInvScaleB = Vec3VFromF32(manifold.GetMassInvScaleB());
	Vec3V angImpulseB   = angImpulse * massInvScaleB;
	CALL_MEMBER_FN(*bodyB, ApplyAngImpulse)(linkB, (-angImpulseB).GetIntrin128());
}


} // namespace rage
