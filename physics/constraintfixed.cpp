//
// physics/constraintfixed.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintfixed.h"

#include "collider.h"
#include "inst.h"
#include "simulator.h"

#include "phcore/pool.h"
#include "vector/colors.h"

namespace rage {

phConstraintFixed::phConstraintFixed(const Params& params)
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

	if (params.translation)
	{
		phManifold* translationManifold = AllocateManifold(instanceA, instanceB, &m_TranslationConstraintMatrix);

		if (translationManifold == NULL)
		{
			PHMANIFOLD->Release(rotationManifold);
			FlagForDestruction();
			return;
		}

		phContact* contact = GetContactPoint(translationManifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		contact->SetLocalPosA(Vec3V(V_ZERO));
		contact->SetLocalPosB(Vec3V(V_ZERO));
		contact->ComputeWorldPointsFromLocal(translationManifold);

		Vec3V worldPosition;
		if (!instanceA || PHLEVEL->IsFixed(instanceA->GetLevelIndex()))
		{
			worldPosition = contact->GetWorldPosB();
		}
		else if (!instanceB || PHLEVEL->IsFixed(instanceB->GetLevelIndex()))
		{
			worldPosition = contact->GetWorldPosA();
		}
		else
		{
			worldPosition = Average(contact->GetWorldPosA(), contact->GetWorldPosB());
		}

		contact->SetWorldPosA(worldPosition);
		contact->SetWorldPosB(worldPosition);
		contact->ComputeLocalPointsFromWorld(translationManifold);
		translationManifold->SetConstraintType(phManifold::FIXED_POINT);

		m_TranslationManifold = translationManifold;
	}
	else
	{
		m_TranslationManifold = NULL;
	}

	QuatV orientationA, orientationB;
	orientationA = GetWorldQuaternion(instanceA,m_ComponentA);
	orientationB = GetWorldQuaternion(instanceB,m_ComponentB);
	m_RelOrientation = InvertNormInput(orientationB);
	m_RelOrientation = Multiply(m_RelOrientation, orientationA);

	m_RotationManifold = rotationManifold;
}

phConstraintFixed::~phConstraintFixed()
{
	if (m_TranslationManifold)
	{
		PHMANIFOLD->Release(m_TranslationManifold);
	}

	if (m_RotationManifold)
	{
		PHMANIFOLD->Release(m_RotationManifold);
	}
}

void phConstraintFixed::SetLocalPosA(Vec3V_In localPosA)
{	
	if (m_TranslationManifold)
	{
		phContact* contact = GetContactPoint(m_TranslationManifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		if (m_Swapped)
		{
			contact->SetLocalPosB(localPosA);
			contact->ComputeWorldPointBFromLocal(m_TranslationManifold); //Perhaps unnecessary because this keeps happening throughout the simulator update
		}
		else
		{
			contact->SetLocalPosA(localPosA);
			contact->ComputeWorldPointAFromLocal(m_TranslationManifold);
		}
	}
}

void phConstraintFixed::SetRelativeRotation(QuatV_In relativeRotation)
{	
	m_RelOrientation = relativeRotation;
}

void phConstraintFixed::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	if (m_TranslationManifold)
	{
		m_TranslationManifold->RefreshContactPoints(-1);
		phContact* contact = GetContactPoint(m_TranslationManifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}
		contact->SetTargetRelVelocity(Vec3V(V_ZERO));

		Vec3V error = contact->GetWorldPosB() - contact->GetWorldPosA();
		ScalarV errorMag = Mag(error);
		BoolV smallError = IsLessThan(errorMag, ScalarV(V_FLT_SMALL_6));
		error = SelectFT(smallError, error * Invert(errorMag), Vec3V(V_Z_AXIS_WZERO));
		contact->SetWorldNormal(error);
		contact->SetDepth(errorMag);
		if (addToSolver)
		{
			AddManifoldToSolver(m_TranslationManifold, instanceA, colliderA, instanceB, colliderB);
		}
	}

	// Should always have a rotation manifold or we should have disabled ourselves
	Assert(m_RotationManifold);

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
	}
}

bool phConstraintFixed::UpdateBreaking()
{
	if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
	{
		if( m_TranslationManifold )
		{
			phContact* contact = GetContactPoint(m_TranslationManifold);
			if (!contact)
			{
				FlagForDestruction();
				return false;
			}

			if( BreakIfImpulseSufficient(m_TranslationManifold) )
			{
				return true;
			}
		}

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
void phConstraintFixed::ProfileDraw()
{
	if (IsFlaggedForDestruction())
	{
		return;
	}

	if (m_TranslationManifold)
	{
		bool oldLighting = grcLighting(false);

		phContact* contact = GetContactPoint(m_TranslationManifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		Vec3V worldPosA = contact->GetWorldPosA();

		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosA), XAXIS, YAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosA), XAXIS, ZAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosA), YAXIS, ZAXIS);

		Vec3V worldPosB = contact->GetWorldPosB();

		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosB), XAXIS, YAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosB), XAXIS, ZAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosB), YAXIS, ZAXIS);

		grcDrawLine(VEC3V_TO_VECTOR3(worldPosA), VEC3V_TO_VECTOR3(worldPosB), Color_red);

		grcLighting(oldLighting);
	}
}
#endif

} // namespace rage
