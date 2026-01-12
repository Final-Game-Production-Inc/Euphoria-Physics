//
// physics/constrainthalfspace.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constrainthalfspace.h"

#include "collider.h"
#include "simulator.h"

#include "grcore/viewport.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/pool.h"
#include "math/angmath.h"
#include "vector/colors.h"

namespace rage {

phConstraintHalfSpace::phConstraintHalfSpace(const Params& params)
	: phConstraintBase(params)
	, m_Manifold(NULL)
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;
	phManifold* manifold = AllocateManifold(instanceA, instanceB, NULL);

	if (manifold == NULL)
	{
		FlagForDestruction();
		return;
	}

	manifold->SetConstraintType(phManifold::NO_CONSTRAINT_CONTACT);
	m_Manifold = manifold;

	phContact* contact = GetContactPoint(manifold);

	if( params.constructUsingLocalPositions )
	{
		contact->SetLocalPosA(params.localPosA);
		contact->SetLocalPosB(params.localPosB);
		contact->ComputeWorldPointsFromLocal(manifold);
	}
	else
	{
        Assertf( (IsEqualAll(params.localPosA, Vec3V(V_ZERO)) && IsEqualAll(params.localPosB, Vec3V(V_ZERO))) , "Local constraint positions set but constructUsingLocalPositions not set, is this intended?");

		Vec3V worldAnchorA = params.worldAnchorA;
		Vec3V worldAnchorB = params.worldAnchorB;

		contact->SetWorldPosA(worldAnchorA);
		contact->SetWorldPosB(worldAnchorB);
		contact->ComputeLocalPointsFromWorld(manifold);
	}

    contact->SetWorldNormal( params.worldNormal );
}

phConstraintHalfSpace::~phConstraintHalfSpace()
{
	if (m_Manifold)
	{
		PHMANIFOLD->Release(m_Manifold);
	}
}

void phConstraintHalfSpace::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
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

	contact->ActivateContact();

	if (addToSolver)
	{
		AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
	}
}

bool phConstraintHalfSpace::UpdateBreaking()
{
	if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return false;
		}

		if( BreakIfImpulseSufficient(m_Manifold) )
		{
			return true;
		}
	}

	return false;
}

#if __PFDRAW
void phConstraintHalfSpace::ProfileDraw()
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
	Vec3V worldEndA = worldPosA + contact->GetWorldNormal(0);

	Vec3V camera(V_ZERO);
	if (grcViewport::GetCurrent())
	{
		camera = grcViewport::GetCurrentCameraPosition();
	}

	grcDrawArrowOrtho(RCC_VECTOR3(worldPosA), RCC_VECTOR3(worldEndA), Color_green, RCC_VECTOR3(camera), 0.25f, 0.1f);

	Vec3V worldPosB = contact->GetWorldPosB();
	Vec3V worldEndB = worldPosB + contact->GetWorldNormal(1);

	grcDrawArrowOrtho(RCC_VECTOR3(worldPosB), RCC_VECTOR3(worldEndB), Color_green, RCC_VECTOR3(camera), 0.25f, 0.1f);

	if (contact->GetDepth() >= 0.0f)
	{
		grcDrawLine(VEC3V_TO_VECTOR3(worldPosA), VEC3V_TO_VECTOR3(worldPosB), Color_red);
	}
	else
	{
		grcDrawLine(VEC3V_TO_VECTOR3(worldPosA), VEC3V_TO_VECTOR3(worldPosB), Color_green);
	}

	grcLighting(oldLighting);
}
#endif

void phConstraintHalfSpace::SetWorldPosA(Vec3V_In worldPosA)
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

void phConstraintHalfSpace::SetWorldPosB(Vec3V_In worldPosB)
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

Vec3V_Out phConstraintHalfSpace::GetWorldPosA()
{
	if (m_Manifold)
	{
		Assertf( IsInstAValid() , "Getting half-space constraint contact position when one of the insts doesn't exist");

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

Vec3V_Out phConstraintHalfSpace::GetWorldPosB()
{
	if (m_Manifold)
	{
		Assertf( IsInstBValid() , "Getting half-space constraint contact position when one of the insts doesn't exist");

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

void phConstraintHalfSpace::SetLocalPosA(Vec3V_In localPosA)
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

void phConstraintHalfSpace::SetLocalPosB(Vec3V_In localPosB)
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

void phConstraintHalfSpace::SetWorldNormal(Vec3V_In worldNormal)
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
			contact->SetWorldNormal(-worldNormal);
		}
		else
		{
			contact->SetWorldNormal(worldNormal);
		}
	}
}

Vec3V_Out phConstraintHalfSpace::GetLocalPosA()
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

Vec3V_Out phConstraintHalfSpace::GetLocalPosB()
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

} // namespace rage
