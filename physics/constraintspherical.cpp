//
// physics/constraintspherical.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintspherical.h"

#include "collider.h"
#include "inst.h"
#include "simulator.h"

#include "phcore/pool.h"
#include "vector/colors.h"

namespace rage {

phConstraintSpherical::phConstraintSpherical(const Params& params)
	: phConstraintBase(params)
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;
	phManifold* manifold = AllocateManifold(instanceA, instanceB, &m_ConstraintMatrix);

	if (manifold == NULL)
	{
		m_Manifold = NULL;
		FlagForDestruction();
		return;
	}

	m_SoftAttach = params.softAttach;

	Vec3V worldPosition = params.worldPosition;
	phContact* contact = GetContactPoint(manifold);
	if (!contact)
	{
		FlagForDestruction();
		return;
	}

	if( params.constructUsingLocalPositions )
	{
		contact->SetLocalPosA(params.localPosA);
		contact->SetLocalPosB(params.localPosB);
		contact->ComputeWorldPointsFromLocal(manifold);
	}
	else
	{
		contact->SetWorldPosA(worldPosition);
		contact->SetWorldPosB(worldPosition);
		contact->ComputeLocalPointsFromWorld(manifold);
	}

    contact->SetWorldNormal( Vec3V(V_Z_AXIS_WZERO) );

	manifold->SetConstraintType(phManifold::FIXED_POINT);

	m_Manifold = manifold;
}

phConstraintSpherical::~phConstraintSpherical()
{
	if (m_Manifold)
	{
		PHMANIFOLD->Release(m_Manifold);
	}
}

void phConstraintSpherical::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	Assert(m_Manifold); // Should have disabled ourselves if we didn't get a manifold
	m_Manifold->RefreshContactPoints(-1);
	phContact* contact = GetContactPoint(m_Manifold);
	if (!contact)
	{
		FlagForDestruction();
		return;
	}
	contact->SetTargetRelVelocity(Vec3V(V_ZERO));

	Vec3V error = contact->GetWorldPosB() - contact->GetWorldPosA();
	ScalarV errorMag = Mag(error);
	ConstraintAssert(IsLessThanAll(errorMag, ScalarV(PH_CONSTRAINT_ASSERT_DEPTH)), "constraint depth is very large (%f) <%f, %f, %f> <=> <%f, %f, %f>, frame (%d)", errorMag.Getf(), VEC3V_ARGS(contact->GetWorldPosA()), VEC3V_ARGS(contact->GetWorldPosB()), m_FrameCreated);
	BoolV smallError = IsLessThan(errorMag, ScalarV(V_FLT_SMALL_6));
	error = SelectFT(smallError, error * Invert(errorMag), Vec3V(V_Z_AXIS_WZERO));
	contact->SetWorldNormal(error);
	contact->SetDepth(errorMag);

	if (m_SoftAttach)
	{
		const ScalarV minSoftThreshold(V_FLT_SMALL_2);
		const ScalarV attractionScale(V_ONE);
		// When soft attaching we apply a fairly strong attractive force by setting the relative velocity negative here
		contact->SetTargetRelVelocity(Scale(error, attractionScale));

		// When performing a soft attach we always set zero depth to avoid any position correction but 
		//  still hold the points from moving farther away from each other (at least along the separation direction between them, anyway)
		contact->SetDepth(ScalarV(V_ZERO));

		// Even if we haven't reached our limits yet there is a minimum distance at which point we may 
		//  never actually reach that goal without the hard limits (Regardless, this distance is so small that the snap should be unnoticeable)
		if (IsLessThanOrEqualAll(errorMag, minSoftThreshold))
		{
			m_SoftAttach = false;
		}
	}


	if (addToSolver)
	{
		RequestWorldLimitEnforcement();

		AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
	}
}

void phConstraintSpherical::VirtualEnforceWorldConstraints(phCollider* collider)
{
	collider->SetNeedsUpdateBeforeFinalIterations(true);
	collider->SetSolverInvMass(0);
}

bool phConstraintSpherical::UpdateBreaking()
{
	if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return false;
		}

		if (BreakIfImpulseSufficient(m_Manifold) )
		{
			return true;
		}
	}

	return false;
}

void phConstraintSpherical::SetWorldPosA(Vec3V_In worldPosA)
{
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		if (m_Swapped)
		{
			contact->SetWorldPosB(worldPosA);
			contact->ComputeLocalPointBFromWorld(m_Manifold);
		}
		else
		{
			contact->SetWorldPosA(worldPosA);
			contact->ComputeLocalPointAFromWorld(m_Manifold);
		}
	}
}

void phConstraintSpherical::SetWorldPosB(Vec3V_In worldPosB)
{
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		if (m_Swapped)
		{
			contact->SetWorldPosA(worldPosB);
			contact->ComputeLocalPointAFromWorld(m_Manifold);
		}
		else
		{
			contact->SetWorldPosB(worldPosB);
			contact->ComputeLocalPointBFromWorld(m_Manifold);
		}
	}
}

void phConstraintSpherical::SetLocalPosA(Vec3V_In localPosA)
{	
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		if (m_Swapped)
		{
			contact->SetLocalPosB(localPosA);
			contact->ComputeWorldPointBFromLocal(m_Manifold); //Perhaps unnecessary because this keeps happening throughout the simulator update
		}
		else
		{
			contact->SetLocalPosA(localPosA);
			contact->ComputeWorldPointAFromLocal(m_Manifold);
		}
	}
}

void phConstraintSpherical::SetLocalPosB(Vec3V_In localPosB)
{	
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}

		if (m_Swapped)
		{
			contact->SetLocalPosA(localPosB);
			contact->ComputeWorldPointAFromLocal(m_Manifold);
		}
		else
		{
			contact->SetLocalPosB(localPosB);
			contact->ComputeWorldPointBFromLocal(m_Manifold);
		}
	}
}

Vec3V_Out phConstraintSpherical::GetWorldPosA()
{
	if (m_Manifold)
	{
		m_Manifold->RefreshContactPoints(-1);

		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return Vec3V(V_ZERO);
		}

		if (m_Swapped)
		{
			return contact->GetWorldPosB();
		}
		else
		{
			return contact->GetWorldPosA();
		}
	}

	return Vec3V(V_ZERO);
}

Vec3V_Out phConstraintSpherical::GetWorldPosB()
{
	if (m_Manifold)
	{
		m_Manifold->RefreshContactPoints(-1);

		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return Vec3V(V_ZERO);
		}

		if (m_Swapped)
		{
			return contact->GetWorldPosA();
		}
		else
		{
			return contact->GetWorldPosB();
		}
	}

	return Vec3V(V_ZERO);
}

Vec3V_Out phConstraintSpherical::GetLocalPosA()
{
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return Vec3V(V_ZERO);
		}

		if (m_Swapped)
		{
			return contact->GetLocalPosB();
		}
		else
		{
			return contact->GetLocalPosA();
		}
	}

	return Vec3V(V_ZERO);
}

Vec3V_Out phConstraintSpherical::GetLocalPosB()
{
	if (m_Manifold)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return Vec3V(V_ZERO);
		}

		if (m_Swapped)
		{
			return contact->GetLocalPosA();
		}
		else
		{
			return contact->GetLocalPosB();
		}
	}

	return Vec3V(V_ZERO);
}

#if __PFDRAW
void phConstraintSpherical::ProfileDraw()
{
	if (IsFlaggedForDestruction())
	{
		return;
	}

	bool oldLighting = grcLighting(false);

	phContact* contact = GetContactPoint(m_Manifold);
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
#endif

} // namespace rage
