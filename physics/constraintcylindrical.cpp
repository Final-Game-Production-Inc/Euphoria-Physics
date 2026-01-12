//
// physics/constraintcylindrical.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintcylindrical.h"

#include "collider.h"
#include "inst.h"
#include "simulator.h"

#include "math/angmath.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/pool.h"
#include "grprofile/drawmanager.h"
#include "profile/element.h"
#include "vector/colors.h"

namespace rage {

	namespace phContactStats
	{
		EXT_PF_TIMER(phConstraintCylindricalUpdate);
	}
	using namespace phContactStats;


	phConstraintCylindrical::phConstraintCylindrical(const Params& params)
		: phConstraintBase(params)
		, m_Manifold(NULL)
		, m_MaxLimit(params.maxLimit)
	{
		phInst* instanceA = params.instanceA;
		phInst* instanceB = params.instanceB;

		// We need a rotation constraint
		phManifold* manifold = AllocateManifold(instanceA, instanceB, &m_ConstraintMatrix);

		if (manifold == NULL)
		{
			FlagForDestruction();
			return;
		}

		m_Manifold = manifold;

		SetWorldAxis(params.worldAxis, instanceA, instanceB);

		manifold->SetConstraintType(phManifold::FIXED_ROTATION);

#if __PFDRAW
		phInst* instance = instanceA ? instanceA : instanceB;
		Vec3V worldDrawPosition = SelectFT(IsEqual(params.worldDrawPosition, Vec3V(V_FLT_LARGE_8)), params.worldDrawPosition, instance->GetPosition());
		Vec3V needleDirection = NormalizeSafe(worldDrawPosition - instance->GetWorldCentroid(), Vec3V(V_X_AXIS_WZERO));
		Vec3V worldDirection = Normalize(params.worldAxis);
		needleDirection = Cross(Cross(needleDirection, worldDirection), worldDirection);
		m_LimitsDrawDirection = needleDirection;
		m_NeedleDrawDirection = UnTransform3x3Ortho(instance->GetMatrix(), m_LimitsDrawDirection);
		m_LocalDrawPosition = UnTransformOrtho(instance->GetMatrix(), worldDrawPosition);
#endif // __PFDRAW
	}

	phConstraintCylindrical::~phConstraintCylindrical()
	{
		if (m_Manifold)
		{
			PHMANIFOLD->Release(m_Manifold);
		}
	}

	void phConstraintCylindrical::SetWorldAxis(Vec3V_In worldAxis)
	{
		SetWorldAxis(worldAxis, GetInstanceA(), GetInstanceB());
	}

	void phConstraintCylindrical::SetWorldAxis(Vec3V_In worldAxis, phInst* instanceA, phInst* instanceB)
	{
		// First, take a look at our constraint axis to figure out where to put the fixed point constraints
		Vec3V worldDirection = Normalize(worldAxis);
		if (instanceA)
		{
			m_ConstraintAxisA = UnTransform3x3Ortho(instanceA->GetMatrix(), worldDirection);
		}
		else
		{
			m_ConstraintAxisA = worldDirection;
		}

		// Set up the rotation constraint
		if (instanceB)
		{
			m_ConstraintAxisB = UnTransform3x3Ortho(instanceB->GetMatrix(), worldDirection);
		}
		else
		{
			m_ConstraintAxisB = worldDirection;
		}

		QuatV orientationA, orientationB;
		orientationA = GetWorldQuaternion(instanceA,m_ComponentA);
		orientationB = GetWorldQuaternion(instanceB,m_ComponentB);
		m_RelOrientation = InvertNormInput(orientationB);
		m_RelOrientation = Multiply(m_RelOrientation, orientationA);
	}

#define		FAST_VirtualUpdate		1

	void phConstraintCylindrical::VirtualUpdate(Vec::V3Param128 UNUSED_PARAM(invTimeStep), phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
	{
		phContact* contact = GetContactPoint(m_Manifold);
		if (!contact)
		{
			FlagForDestruction();
			return;
		}
		contact->SetTargetRelVelocity(Vec3V(V_ZERO));

		PF_FUNC(phConstraintCylindricalUpdate);

#if FAST_VirtualUpdate
		const bool isMaxLimitPositive = m_MaxLimit > 0.0f ? true: false;
#endif

		Assertf(GetUsePushes(), "Cylindrical constraints with pushes turned off are not currently supported");

		// Should always have a manifold or we should have disabled ourselves
		Assert(m_Manifold);

		// Sliding rotation

		// Get the world constraint axes.
		Mat34V _ident(V_IDENTITY);

		Vec3V worldAxisA = Transform3x3(instanceA ? instanceA->GetMatrix() : _ident, m_ConstraintAxisA);
		Vec3V worldAxisB = Transform3x3(instanceB ? instanceB->GetMatrix() : _ident, m_ConstraintAxisB);

		// Compute the relative orientation.
		Vec3V relOrientation = Cross(worldAxisA,worldAxisB);

		// Get the normal and depth (angle) of the rotation to get A from its current orientation to its target orientation.
		ScalarV sine = Mag(relOrientation);
		ScalarV _zero(V_ZERO);

#if FAST_VirtualUpdate
		BoolV cond0 = IsGreaterThan( sine,ScalarV(V_FLT_MIN) );
		
		Vec3V normalA = SelectFT(cond0, Vec3V(V_X_AXIS_WZERO), InvScale(relOrientation,Vec3V(sine)) );
				 sine = SelectFT(cond0, sine, Min(ScalarV(V_ONE), sine) );
		ScalarV depth = SelectFT(cond0, _zero, Arcsin(sine) );

		depth = SelectFT( IsLessThan(Dot(worldAxisA,worldAxisB),_zero), depth, Subtract(ScalarVFromF32(PI),depth) );
#else
		ScalarV depth;
		Vec3V normalA;
		if (IsGreaterThanAll(sine,ScalarV(V_FLT_MIN)))
		{
			normalA = InvScale(relOrientation,Vec3V(sine));
			sine = Min(ScalarV(V_ONE), sine); // Passing in anything bigger than 1.0 into Arcsin is a recipe for QNaNs
			depth = Arcsin(sine); // A necessary LHS :(
		}
		else
		{
			normalA = Vec3V(V_X_AXIS_WZERO);
			depth = _zero;
		}

		if (IsLessThanAll(Dot(worldAxisA,worldAxisB),_zero))
		{
			depth = Subtract(ScalarVFromF32(PI),depth);
		}

#endif // FAST_VirtualUpdate

		depth -= ScalarVFromF32(m_MaxLimit);
		
		contact->SetWorldNormal(normalA);
		contact->SetDepth(depth);

#if FAST_VirtualUpdate
		bool localAtLimit;
		if (isMaxLimitPositive)
		{
			const bool res = IsGreaterThanAll(depth,_zero) ? true: false;
			if( res )
				m_Manifold->SetConstraintType(phManifold::SLIDING_ROTATION);

			localAtLimit = res;
		}
#else
		if (m_MaxLimit > 0.0f)
		{
			if (IsGreaterThanAll(depth,_zero))
			{
				m_AtLimit = true;

				// Fill in the contact data.
				m_Manifold->SetConstraintType(phManifold::SLIDING_ROTATION);
			}
			else
			{
				m_AtLimit = false;
			}
		}
#endif //FAST_VirtualUpdate
		else
		{
#if FAST_VirtualUpdate
			localAtLimit = true;
#else
			m_AtLimit = true;
#endif

			// Stuff the world axis into the local position
			contact->SetLocalPosA(worldAxisB);
			m_Manifold->SetConstraintType(phManifold::PIVOTING_ROTATION);
		}

#if FAST_VirtualUpdate
		if (localAtLimit && addToSolver)
#else
		if (m_AtLimit && addToSolver)
#endif
		{
			AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
		}
		else
		{
			contact->SetPreviousSolution(Vec3V(_zero));
		}

#if FAST_VirtualUpdate
		m_AtLimit = localAtLimit;
#endif

		if( colliderA && !colliderB && IsGreaterThanOrEqualAll(depth,_zero) )
		{
			// We're attached to the world, so set our inv ang inertia
			Vec3V invAngInertia=colliderA->GetSolverInvAngInertia();
			invAngInertia-=m_ConstraintAxisA*Dot(invAngInertia,m_ConstraintAxisA);
			colliderA->SetTranslationConstraintInvAngInertia(RCC_VECTOR3(invAngInertia));
		}
	}


	void phConstraintCylindrical::EnforceWorldConstraints()
	{
		// We make the off-axis inertia infinite, but the on-axis we make the object heavier. This fixes a problem where a constrained
		// object rotates into a dynamic object, pinning it against a wall, which freaks out if we make all directions infinite. Without
		// the multiplier though, heavy objects hitting doors cause the doors to freak out.
		static float s_InvInertiaMultiplier = 0.1f;

		ScalarV invInertiaMultiplier(V_ZERO);

		if (!m_AtLimit)
		{
			invInertiaMultiplier = ScalarV(s_InvInertiaMultiplier);
		}

		if (!GetInstanceB())
		{
			phInst* instanceA = GetInstanceA();

			if (instanceA)
			{
				phCollider* colliderA = PHSIM->GetCollider(instanceA->GetLevelIndex());

				if (colliderA)
				{
					Vec3V angInertia = colliderA->GetInvAngInertia();
					Vec3V modifiedAngInertia = Dot(angInertia, m_ConstraintAxisA) * m_ConstraintAxisA * invInertiaMultiplier;
					colliderA->SetSolverInvAngInertia(modifiedAngInertia.GetIntrin128());
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
					Vec3V angInertia = colliderB->GetInvAngInertia();
					Vec3V modifiedAngInertia = Dot(angInertia, m_ConstraintAxisB) * m_ConstraintAxisB * invInertiaMultiplier;
					colliderB->SetSolverInvAngInertia(modifiedAngInertia.GetIntrin128());
				}
			}
		}
	}

	bool phConstraintCylindrical::UpdateBreaking()
	{
		if( !IsFlaggedForDestruction() && m_Breakable && !m_Broken )
		{
			phContact* contact = GetContactPoint(m_Manifold);
			if (!contact)
			{
				FlagForDestruction();
				return false;
			}

			if ( BreakIfImpulseSufficient(m_Manifold) )
			{
				return true;
			}
		}

		return false;
	}

#if __PFDRAW
	EXT_PFD_DECLARE_ITEM_SLIDER(ConstraintLimitScale);

	void phConstraintCylindrical::ProfileDraw()
	{
		if (IsFlaggedForDestruction())
		{
			return;
		}

		if (phInst* instanceA = GetInstanceA())
		{
			Mat34V worldMatrixB;
			GetWorldMatrix(m_Manifold->GetInstanceB(), m_ComponentB, worldMatrixB);

			Vec3V worldAxisB = Transform3x3(worldMatrixB, m_ConstraintAxisB);

			Mat34V worldMatrixA;
			GetWorldMatrix(m_Manifold->GetInstanceA(), m_ComponentA, worldMatrixA);

			Vec3V worldDrawDirection = Transform3x3(worldMatrixA, m_NeedleDrawDirection);

			Vec3V limitPosition = Transform(instanceA->GetMatrix(), m_LocalDrawPosition);
			ScalarV limitScale = ScalarVFromF32(PFD_ConstraintLimitScale.GetValue());
			grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), VEC3V_TO_VECTOR3(limitPosition + worldDrawDirection * limitScale), m_AtLimit ? Color_green : Color_red);

			Vec3V crossDirection = Cross(worldAxisB, m_LimitsDrawDirection);

			grcColor(Color_white);
			grcDrawArc(PFD_ConstraintLimitScale.GetValue() * 0.8f,
				VEC3V_TO_VECTOR3(limitPosition),
				VEC3V_TO_VECTOR3(m_LimitsDrawDirection),
				VEC3V_TO_VECTOR3(crossDirection),
				0,
				m_MaxLimit,
				37);

			Vector3 minLimitEnd;
			minLimitEnd.Set(VEC3V_TO_VECTOR3(m_LimitsDrawDirection));
			minLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
			minLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

			grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), minLimitEnd, Color_white);

			Vector3 maxLimitEnd;
			maxLimitEnd.SetScaled(VEC3V_TO_VECTOR3(m_LimitsDrawDirection),cosf(m_MaxLimit));
			maxLimitEnd.AddScaled(VEC3V_TO_VECTOR3(crossDirection),sinf(m_MaxLimit));
			maxLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
			maxLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

			grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), maxLimitEnd, Color_white);
		}
	}
#endif

} // namespace rage
