//
// physics/constraintfixed.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintfixedrotation.h"

#include "collider.h"
#include "inst.h"
#include "simulator.h"

#include "phcore/pool.h"
#include "vector/colors.h"

namespace rage {

phConstraintFixedRotation::phConstraintFixedRotation(const Params& params)
	: phConstraintBase(params)
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;
	phManifold* rotationManifold = AllocateManifold(instanceA, instanceB, &m_RotationConstraintMatrix);

	if (rotationManifold == NULL)
	{
		FlagForDestruction();
		return;
	}

	rotationManifold->SetConstraintType(phManifold::FIXED_ROTATION);

	QuatV orientationA, orientationB;
	orientationA = GetWorldQuaternion(instanceA,m_ComponentA);
	orientationB = GetWorldQuaternion(instanceB,m_ComponentB);
	m_RelOrientation = InvertNormInput(orientationB);
	m_RelOrientation = Multiply(m_RelOrientation, orientationA);

	m_RotationManifold = rotationManifold;
}

phConstraintFixedRotation::~phConstraintFixedRotation()
{
	if (m_RotationManifold)
	{
		PHMANIFOLD->Release(m_RotationManifold);
	}
}

void phConstraintFixedRotation::VirtualEnforceWorldConstraints(phCollider* collider)
{
	collider->SetNeedsUpdateBeforeFinalIterations(true);

	collider->SetSolverInvAngInertia(Vector3(0,0,0));
}

void phConstraintFixedRotation::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	// Compute the relative orientation.
	QuatV orientationA = GetWorldQuaternion(instanceA,m_ComponentA);
	QuatV orientationB = GetWorldQuaternion(instanceB,m_ComponentB);
	QuatV targetOrientationA = Multiply(orientationB,m_RelOrientation);
	targetOrientationA = PrepareSlerp(orientationA,targetOrientationA);
	QuatV relOrientation = Multiply(targetOrientationA,Invert(orientationA));

	// Get the normal and depth (angle) of the rotation to get A from its current orientation to its target orientation.
	Vec3V normal = GetUnitDirectionSafe(relOrientation);
 	ScalarV depth = GetAngle(relOrientation);

	// GetAngle returns non-zero for zero angles...patch that up here or else we'll cause an undesired rotation
 	BoolV tooSmall = IsGreaterThan(relOrientation.GetW(), ScalarV(V_ONE) - ScalarV(V_FLT_SMALL_6));
 	depth = SelectFT(tooSmall, depth, ScalarV(V_ZERO));

	phContact* contact = GetContactPoint(m_RotationManifold);
	if (!contact)
	{
		FlagForDestruction();
		return;
	}
	contact->SetTargetRelVelocity(Vec3V(V_ZERO));

	contact->SetWorldNormal(normal);
	contact->SetDepth(depth);

	if (addToSolver)
	{
		AddManifoldToSolver(m_RotationManifold, instanceA, colliderA, instanceB, colliderB);

		RequestWorldLimitEnforcement();
	}
}

bool phConstraintFixedRotation::UpdateBreaking()
{
	if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
	{
		phContact* contact = GetContactPoint(m_RotationManifold);
		if (!contact)
		{
			FlagForDestruction();
			return false;
		}

		if( BreakIfImpulseSufficient(m_RotationManifold) )
		{
			return true;
		}
	}

	return false;
}

#if __PFDRAW
void phConstraintFixedRotation::ProfileDraw()
{

}
#endif

} // namespace rage
