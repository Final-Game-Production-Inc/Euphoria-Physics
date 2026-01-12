//
// physics/constrainthinge.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constrainthinge.h"

#include "collider.h"
#include "simulator.h"

#include "pharticulated/articulatedcollider.h"
#include "phbound/support.h"
#include "phcore/pool.h"
#include "grprofile/drawmanager.h"
#include "math/angmath.h"
#include "vector/colors.h"

namespace rage {

EXT_PFD_DECLARE_ITEM_SLIDER(ConstraintLimitScale);

phConstraintHinge::phConstraintHinge(const Params& params)
	: phConstraintBase(params)
	, m_FixedPointTopManifold(NULL)
	, m_FixedPointBottomManifold(NULL)
	, m_FixedPointMiddleManifold(NULL)
	, m_RotationConstraint(phConstraintRotation::Params(params,
														params.worldAxis,
#if __PFDRAW
														params.worldAnchor,
#endif /// __PFDRAW
														params.minLimit,
														params.maxLimit,
														params.minSoftLimit,
														params.maxSoftLimit,
														params.softLimitStrength,
														params.softLimitDamping))
{
	phInst* instanceA = params.instanceA;
	phInst* instanceB = params.instanceB;

	// Get manifold for fixed point top constraint
	phManifold* fixedPointTopManifold = AllocateManifold(instanceA, instanceB, &m_FixedPointTopConstraintMatrix);
	if (fixedPointTopManifold == NULL)
	{
		FlagForDestruction();
		return;
	}

	// Get manifold for fixed point bottom constraint
	phManifold* fixedPointBottomManifold = AllocateManifold(instanceA, instanceB, &m_FixedPointBottomConstraintMatrix);
	if (fixedPointBottomManifold == NULL)
	{
		PHMANIFOLD->Release(fixedPointTopManifold);
		FlagForDestruction();
		return;
	}

	phManifold* fixedPointMiddleManifold = NULL;
	if(params.useMiddleFixedPoint)
	{
		fixedPointMiddleManifold = AllocateManifold(instanceA, instanceB, &m_FixedPointMiddleConstraintMatrix);
		if(fixedPointMiddleManifold == NULL)
		{
			PHMANIFOLD->Release(fixedPointTopManifold);
			PHMANIFOLD->Release(fixedPointBottomManifold);
			FlagForDestruction();
			return;
		}
	}

	m_RotationConstraint.SetBreakable(params.breakable, params.breakingStrength);

	// We need a rotation constraint if we have limits
	m_LimitsEnabled = params.minLimit != -PI || params.maxLimit != PI;

	m_FixedPointTopManifold = fixedPointTopManifold;
	m_FixedPointBottomManifold = fixedPointBottomManifold;
	m_FixedPointMiddleManifold = fixedPointMiddleManifold;

	// Take a look at our constraint axis to figure out where to put the fixed point constraints
	Vec3V worldDirection = Normalize(params.worldAxis);
	Vec3V localAxisA = UnTransform3x3Ortho(instanceA->GetMatrix(), worldDirection);

	//For transforming the top and bottom world points into the component spaces of the instances
	Mat34V partMatrixA, partMatrixB;
	GetWorldMatrix(instanceA, m_ComponentA, partMatrixA);
	GetWorldMatrix(instanceB, m_ComponentB, partMatrixB);

	Assertf(instanceA->GetArchetype()->GetBound()->GetType() != phBound::BVH, "Trying to attach a hinge constraint to a BVH, %s", instanceA->GetArchetype()->GetFilename());

	// Top point
	Vec3V supportLocalTopA = instanceA->GetArchetype()->GetBound()->LocalGetSupportingVertex(localAxisA.GetIntrin128());
	Vec3V supportWorldTopA = Transform(instanceA->GetMatrix(), supportLocalTopA);
	Vec3V supportDeltaTop = supportWorldTopA - params.worldAnchor;
	Vec3V projectedTop = Dot(supportDeltaTop, worldDirection) * worldDirection;
	Vec3V worldPositionTop = params.worldAnchor + projectedTop;

	// Bottom point
	Vec3V supportLocalBottomA = instanceA->GetArchetype()->GetBound()->LocalGetSupportingVertex((-localAxisA).GetIntrin128());
	Vec3V supportWorldBottomA = Transform(instanceA->GetMatrix(), supportLocalBottomA);
	Vec3V supportDeltaBottom = supportWorldBottomA - params.worldAnchor;
	Vec3V projectedBottom = Dot(supportDeltaBottom, worldDirection) * worldDirection;
	Vec3V worldPositionBottom = params.worldAnchor + projectedBottom;

	ScalarV fixedPointSeparation = Mag(worldPositionTop - worldPositionBottom);
	ScalarV minSeparation = ScalarVFromF32(params.minFixedPointSeparation);
	ScalarV additionalSeparation = minSeparation - fixedPointSeparation;

	if( IsGreaterThanAll(additionalSeparation, ScalarV(V_ZERO)) )
	{
		ScalarV halfAdditionalSeparation = ScalarV(V_HALF) * additionalSeparation;
		worldPositionTop += halfAdditionalSeparation * worldDirection;
		worldPositionBottom -= halfAdditionalSeparation * worldDirection;
	}

	// Store fixed point positions in the phContacts
	phContact* fixedPointTopContact = GetContactPoint(fixedPointTopManifold);
	Vec3V localPointOnATop = UnTransformOrtho(partMatrixA, worldPositionTop);
	Vec3V localPointOnBTop = UnTransformOrtho(partMatrixB, worldPositionTop);
	fixedPointTopContact->SetLocalPosA(localPointOnATop);
	fixedPointTopContact->SetLocalPosB(localPointOnBTop);
	fixedPointTopContact->SetWorldPosA(worldPositionTop);
	fixedPointTopContact->SetWorldPosB(worldPositionTop);
	fixedPointTopManifold->SetConstraintType(phManifold::FIXED_POINT);

	phContact* fixedPointBottomContact = GetContactPoint(fixedPointBottomManifold);
	Vec3V localPointOnABottom = UnTransformOrtho(partMatrixA, worldPositionBottom);
	Vec3V localPointOnBBottom = UnTransformOrtho(partMatrixB, worldPositionBottom);
	fixedPointBottomContact->SetLocalPosA(localPointOnABottom);
	fixedPointBottomContact->SetLocalPosB(localPointOnBBottom);
	fixedPointBottomContact->SetWorldPosA(worldPositionBottom);
	fixedPointBottomContact->SetWorldPosB(worldPositionBottom);
	fixedPointBottomManifold->SetConstraintType(phManifold::FIXED_POINT);

	if(fixedPointMiddleManifold)
	{
		Vec3V worldPositionMiddle = params.worldAnchor;
		phContact* fixedPointMiddleContact = GetContactPoint(fixedPointMiddleManifold);
		fixedPointMiddleContact->SetLocalPosA(UnTransformOrtho(partMatrixA, worldPositionMiddle));
		fixedPointMiddleContact->SetLocalPosB(UnTransformOrtho(partMatrixB, worldPositionMiddle));
		fixedPointMiddleContact->SetWorldPosA(worldPositionMiddle);
		fixedPointMiddleContact->SetWorldPosB(worldPositionMiddle);
		fixedPointMiddleManifold->SetConstraintType(phManifold::FIXED_POINT);
	}
}

phConstraintHinge::~phConstraintHinge()
{
	if (m_FixedPointBottomManifold)
	{
		PHMANIFOLD->Release(m_FixedPointBottomManifold);
	}

	if (m_FixedPointTopManifold)
	{
		PHMANIFOLD->Release(m_FixedPointTopManifold);
	}

	if(m_FixedPointMiddleManifold)
	{
		PHMANIFOLD->Release(m_FixedPointMiddleManifold);
	}
}

#if !__FINAL
void phConstraintHinge::SetOwner(const char* file, int line)
{
	phConstraintBase::SetOwner(file, line);

	m_RotationConstraint.SetOwner(file, line);
}
#endif // !__FINAL

bool phConstraintHinge::VirtualUpdateFixedPointManifold(phManifold* manifold, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	Assert(manifold); // Should have disabled ourselves if we didn't get a manifold
	manifold->RefreshContactPoints(-1);
	phContact* contact = GetContactPoint(manifold);
	if (!contact)
	{
		FlagForDestruction();
		return false;
	}
	contact->SetTargetRelVelocity(Vec3V(V_ZERO));

	Vec3V errorTop = contact->GetWorldPosB() - contact->GetWorldPosA();
	ScalarV errorMagTop = Mag(errorTop);
	BoolV smallErrorTop = IsLessThan(errorMagTop, ScalarV(V_FLT_SMALL_6));
	errorTop = SelectFT(smallErrorTop, errorTop * Invert(errorMagTop), Vec3V(V_Z_AXIS_WZERO));

	contact->SetWorldNormal(errorTop);
	contact->SetDepth(errorMagTop);

	if (addToSolver)
	{
		AddManifoldToSolver(manifold, instanceA, colliderA, instanceB, colliderB);
	}

	return true;
}

void phConstraintHinge::VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	// Update each of the fixed point manifolds.
	if(	!VirtualUpdateFixedPointManifold(m_FixedPointTopManifold,instanceA,colliderA,instanceB,colliderB,addToSolver) ||
		!VirtualUpdateFixedPointManifold(m_FixedPointBottomManifold,instanceA,colliderA,instanceB,colliderB,addToSolver) ||
		(m_FixedPointMiddleManifold && !VirtualUpdateFixedPointManifold(m_FixedPointMiddleManifold,instanceA,colliderA,instanceB,colliderB,addToSolver)))
	{
		// If any of the manifolds couldn't update just kill the constraint.
		FlagForDestruction();
		return;
	}

	RequestWorldLimitEnforcement();

	if (m_LimitsEnabled)
	{
		m_RotationConstraint.phConstraintRotation::VirtualUpdate(invTimeStep, instanceA, colliderA, instanceB, colliderB, addToSolver);
	}
}

void phConstraintHinge::EnforceWorldConstraints()
{
	if (m_LimitsEnabled)
	{
		m_RotationConstraint.phConstraintRotation::EnforceWorldConstraints();
	}

	if (!GetInstanceB())
	{
		phInst* instanceA = GetInstanceA();

		if (instanceA)
		{
			phCollider* colliderA = PHSIM->GetCollider(instanceA->GetLevelIndex());

			if (colliderA)
			{
				colliderA->SetSolverInvMass(0);
				colliderA->SetNeedsUpdateBeforeFinalIterations(true);
			}
		}
	}
	else if (!GetInstanceA())
	{
		phInst* instanceB = GetInstanceB();

		if (instanceB)
		{
			phCollider* colliderB = PHSIM->GetCollider(instanceB->GetLevelIndex());

			if (colliderB)
			{
				colliderB->SetSolverInvMass(0);
				colliderB->SetNeedsUpdateBeforeFinalIterations(true);
			}
		}
	}
}

void phConstraintHinge::SetBreakable(bool breakable, float breakingStrength)
{
	phConstraintBase::SetBreakable(breakable, breakingStrength);
	m_RotationConstraint.SetBreakable(breakable, breakingStrength);
}

void phConstraintHinge::SetBroken(bool broken)
{
	phConstraintBase::SetBroken(broken);
	m_RotationConstraint.SetBroken(broken);
}

bool phConstraintHinge::UpdateBreaking()
{
	if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
	{
		if (!GetContactPoint(m_FixedPointTopManifold) || 
			!GetContactPoint(m_FixedPointBottomManifold) || 
			(m_FixedPointMiddleManifold && !GetContactPoint(m_FixedPointMiddleManifold)))
		{
			FlagForDestruction();
			return false;
		}

		if ( BreakIfImpulseSufficient(m_FixedPointTopManifold) 
			|| BreakIfImpulseSufficient(m_FixedPointBottomManifold)
			|| (m_FixedPointMiddleManifold && BreakIfImpulseSufficient(m_FixedPointMiddleManifold)))
		{
			return true;
		}

		m_Broken = m_RotationConstraint.UpdateBreaking();
		return m_Broken;
	}

	return false;
}

#if __PFDRAW
void DrawContactHelper(const phContact* pContact)
{
	if(pContact)
	{
		Vec3V worldPosTopA = pContact->GetWorldPosA();

		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopA), XAXIS, YAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopA), XAXIS, ZAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopA), YAXIS, ZAXIS);

		Vec3V worldPosTopB = pContact->GetWorldPosB();

		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopB), XAXIS, YAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopB), XAXIS, ZAXIS);
		grcDrawCircle(0.025f, VEC3V_TO_VECTOR3(worldPosTopB), YAXIS, ZAXIS);

		grcDrawLine(VEC3V_TO_VECTOR3(worldPosTopA), VEC3V_TO_VECTOR3(worldPosTopB), Color_red);
	}

}

void phConstraintHinge::ProfileDraw()
{
	if (IsFlaggedForDestruction())
	{
		return;
	}

	bool oldLighting = grcLighting(false);

	DrawContactHelper(GetContactPoint(m_FixedPointTopManifold));
	DrawContactHelper(GetContactPoint(m_FixedPointBottomManifold));
	if(m_FixedPointMiddleManifold)
	{
		DrawContactHelper(GetContactPoint(m_FixedPointMiddleManifold));
	}

	if (m_LimitsEnabled)
	{
		m_RotationConstraint.phConstraintRotation::ProfileDraw();
	}

	grcLighting(oldLighting);
}
#endif

} // namespace rage
