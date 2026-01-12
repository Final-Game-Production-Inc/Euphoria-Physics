//  
// phsolver/applyimpulseandpushsliderotationartandfix.cpp  
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

void ApplyImpulseAndPushSlideRotationArtAndFix(phManifold& manifold, const phForceSolverGlobals& SPU_ONLY(globals))
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	Assert(manifold.GetNumContacts() == 1);

	phContact& contactPoint = manifold.GetContactPoint(0);
	Assert(contactPoint.IsContactActive());

	Vec3V worldNormalA = contactPoint.GetWorldNormal();
	int linkA = colliderA->GetLinkFromComponent(manifold.GetComponentA());
	Mat33V angInertia = manifold.GetConstraintMatrix();

	Vec3V turnA = CALL_MEMBER_FN(*bodyA, GetAngularVelocity)(linkA);

	// PUSH

	turnA = Scale(worldNormalA,Dot(turnA,worldNormalA));
	Vec3V deltaAng = contactPoint.GetDepthV() * worldNormalA - turnA;

	Vec3V turn = SelectFT(manifold.GetUsePushesV(), Vec3V(V_ZERO), Multiply(angInertia, deltaAng));

	Vec3V oldTurn = contactPoint.GetPreviousPush();
	Vec3V newTurn = oldTurn + turn;
	ScalarV turnAroundAxis = Dot(newTurn, worldNormalA);
	newTurn = SelectFT(IsGreaterThan(turnAroundAxis, ScalarV(V_ZERO)), Vec3V(V_ZERO), newTurn);
	contactPoint.SetPreviousPush(newTurn);
	turn = newTurn - oldTurn;

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	phForceSolverDebugRecord record;
	record.SetFunctionName("ArtConstrSlideRotationArtAndFix");
	record.SetColliders(colliderA,0);
	record.SetNormal(contactPoint.GetWorldNormal());
	record.SetStartVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
#endif

	CALL_MEMBER_FN(*bodyA, ApplyAngImpulse)(linkA, turn.GetIntrin128());

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
	record.SetEndVelocities(colliderA->GetVelocity(),Vector3(0,0,0),colliderA->GetAngVelocity(),Vector3(0,0,0));
	phForceSolver::AddForceSolverDebugRecord(record);
#endif
}

} // namespace rage
