//
// physics/constraintdistance.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintdistance.h"

#include "collider.h"
#include "simulator.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/pool.h"
#include "math/angmath.h"
#include "vector/colors.h"

namespace rage {

phConstraintDistance::phConstraintDistance(const Params& params)
	: phConstraintBase(params)
	, m_Manifold(NULL)
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;
	phManifold* manifold = AllocateManifold(instanceA, instanceB, &m_ConstraintMatrix);

	if (manifold == NULL)
	{
		FlagForDestruction();
		return;
	}

	manifold->SetConstraintType(phManifold::SLIDING_POINT);
	m_Manifold = manifold;

	m_SeparationVelocity = 0;
	m_MinDistance = params.minDistance;
	m_MaxDistance = params.maxDistance;
	m_AllowedPenetration = params.allowedPenetration;
	m_SoftAttach = params.softAttach;

	phContact* contact = GetContactPoint(manifold);

	if( params.constructUsingLocalPositions )
	{
		contact->SetLocalPosA(params.localPosA);
		contact->SetLocalPosB(params.localPosB);
		contact->ComputeWorldPointsFromLocal(manifold);

		if (params.maxDistance <= 0.0f)
		{
			m_MaxDistance = Dist(contact->GetWorldPosA(), contact->GetWorldPosB()).Getf();
		}
	}
	else
	{
        Assertf( (IsEqualAll(params.localPosA, Vec3V(V_ZERO)) && IsEqualAll(params.localPosB, Vec3V(V_ZERO))) , "Local constraint positions set but constructUsingLocalPositions not set, is this intended?");

		Vec3V worldAnchorA = params.worldAnchorA;
		Vec3V worldAnchorB = params.worldAnchorB;

		contact->SetWorldPosA(worldAnchorA);
		contact->SetWorldPosB(worldAnchorB);
		contact->ComputeLocalPointsFromWorld(manifold);

		if (params.maxDistance <= 0.0f)
		{
			m_MaxDistance = Dist(worldAnchorA, worldAnchorB).Getf();
		}
	}

    contact->SetWorldNormal( Vec3V(V_Z_AXIS_WZERO) );
}

phConstraintDistance::~phConstraintDistance()
{
	if (m_Manifold)
	{
		PHMANIFOLD->Release(m_Manifold);
	}
}

void phConstraintDistance::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
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

	static ScalarV minSoftThreshold(V_FLT_SMALL_2);

	Vec3V delta = contact->GetWorldPosB() - contact->GetWorldPosA();
	ScalarV distance = Mag(delta);
	ScalarV allowedPenetration(m_AllowedPenetration);
	ScalarV minDist = ScalarVFromF32(m_MinDistance) - allowedPenetration;
	ScalarV maxDist = ScalarVFromF32(m_MaxDistance) + allowedPenetration;

	// Find out which limit we are closer to
	ScalarV deltaToMin = distance - minDist;
	ScalarV deltaToMax = maxDist - distance;
	BoolV closerToMax = IsLessThanOrEqual(Abs(deltaToMax + ScalarV(V_FLT_SMALL_6)), Abs(deltaToMin + ScalarV(V_FLT_SMALL_6)));

	// Compute how deeply we are violating the constraint limit
	ScalarV depth = -SelectFT(closerToMax, deltaToMin, deltaToMax);
	if (IsGreaterThanAll(depth, -allowedPenetration))
	{
		ConstraintAssert(IsLessThanAll(depth, ScalarV(PH_CONSTRAINT_ASSERT_DEPTH)), "constraint depth is very large (%f) <%f, %f, %f> <=> <%f, %f, %f>. min (%.2f) max (%.2f), frame (%d)"
			, depth.Getf(), VEC3V_ARGS(contact->GetWorldPosA()), VEC3V_ARGS(contact->GetWorldPosB()), m_MinDistance, m_MaxDistance, m_FrameCreated);

		// Reverse direction if we're less than min
		Vec3V direction = InvScaleSafe(delta,distance,Vec3V(V_ZERO));
		Vec3V worldNormal = SelectFT(closerToMax,-direction,direction);

		// Set the target relative velocity, only if the constraint is at its limit
		// - When soft attaching we apply a fairly strong attractive force by setting the relative velocity negative here
		contact->SetTargetRelVelocity(-Scale(direction, m_SoftAttach ? ScalarV(V_NEGONE) : ScalarVFromF32(m_SeparationVelocity)));

		m_AtLimit = true;

		contact->SetWorldNormal(worldNormal);
		// When performing a soft attach we always set zero depth to avoid any position correction but 
		//  still hold the points from moving farther away from each other (at least along the separation direction between them, anyway)
		contact->SetDepth(m_SoftAttach ? ScalarV(V_ZERO) : depth);
		contact->ActivateContact();

		if (addToSolver)
		{
			AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
		}

		// Even if we haven't reached our limits yet there is a minimum distance at which point we may 
		//  never actually reach that goal without the hard limits (Regardless, this distance is so small that the snap should be unnoticeable)
		if (IsLessThanOrEqualAll(depth, Subtract(minSoftThreshold, allowedPenetration)))
		{
			m_SoftAttach = false;
		}
	}
	else
	{
		m_AtLimit = false;
		m_SoftAttach = false;
		contact->SetTargetRelVelocity(Vec3V(V_ZERO));
	}
}

void phConstraintDistance::SetMaxDistance(float maxDistance)
{
	m_MaxDistance = maxDistance;
}

void phConstraintDistance::SetMinDistance(float minDistance)
{
	m_MinDistance = minDistance;
}


void phConstraintDistance::SetSeparationVelocity(float separationVelocity)
{
	m_SeparationVelocity = separationVelocity;
}

bool phConstraintDistance::UpdateBreaking()
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
void phConstraintDistance::ProfileDraw()
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

	if (m_AtLimit)
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

void phConstraintDistance::SetWorldPosA(Vec3V_In worldPosA)
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

void phConstraintDistance::SetWorldPosB(Vec3V_In worldPosB)
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

Vec3V_Out phConstraintDistance::GetWorldPosA()
{
	if (m_Manifold)
	{
		Assertf( IsInstAValid() , "Getting distance constraint contact position when one of the insts doesn't exist");

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

Vec3V_Out phConstraintDistance::GetWorldPosB()
{
	if (m_Manifold)
	{
		Assertf( IsInstBValid() , "Getting distance constraint contact position when one of the insts doesn't exist");

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

void phConstraintDistance::SetLocalPosA(Vec3V_In localPosA)
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

void phConstraintDistance::SetLocalPosB(Vec3V_In localPosB)
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

Vec3V_Out phConstraintDistance::GetLocalPosA()
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

Vec3V_Out phConstraintDistance::GetLocalPosB()
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
