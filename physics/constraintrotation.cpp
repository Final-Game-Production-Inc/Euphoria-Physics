//
// physics/constraintrotation.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintrotation.h"

#include "collider.h"
#include "inst.h"
#include "simulator.h"

#include "math/angmath.h"
#include "pharticulated/articulatedcollider.h"
#include "phcore/pool.h"
#include "physics/colliderdispatch.h"
#include "grprofile/drawmanager.h"
#include "profile/element.h"
#include "vector/colors.h"

namespace rage {

	namespace phContactStats
	{
		EXT_PF_TIMER(phConstraintRotationUpdate);
	}
	using namespace phContactStats;

phConstraintRotation::phConstraintRotation(const Params& params)
	: phConstraintBase(params)
	, m_Manifold(NULL)
	, m_MinLimit(params.minLimit)
	, m_MaxLimit(params.maxLimit)
	, m_MinSoftLimit(params.minSoftLimit)
	, m_MaxSoftLimit(params.maxSoftLimit)
	, m_SoftLimitStrength(params.softLimitStrength)
	, m_SoftLimitDamping(params.softLimitDamping)
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

phConstraintRotation::~phConstraintRotation()
{
	if (m_Manifold)
	{
		PHMANIFOLD->Release(m_Manifold);
	}
}

void phConstraintRotation::SetWorldAxis(Vec3V_In worldAxis)
{
	SetWorldAxis(worldAxis, GetInstanceA(), GetInstanceB());
}

void phConstraintRotation::SetWorldAxis(Vec3V_In worldAxis, phInst* instanceA, phInst* instanceB)
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

#define FAST_RotationVirtualUpdate		1

void phConstraintRotation::VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver)
{
	PF_FUNC(phConstraintRotationUpdate);

	// Should always have a manifold or we should have disabled ourselves
	Assert(m_Manifold);

	// Sliding rotation

	// Get the world constraint axes.
	QuatV orientationA = GetWorldQuaternion(instanceA,m_ComponentA);
	QuatV orientationB = GetWorldQuaternion(instanceB,m_ComponentB);
	Assert(MagSquared(orientationA).Getf() < 1.02f && IsTrueAll(IsFinite(orientationA))); //Asserts because a NaN occurred in relOrientation once
	Assert(MagSquared(orientationB).Getf() < 1.02f && IsTrueAll(IsFinite(orientationB)));

	QuatV targetOrientationA = Multiply(orientationB, m_RelOrientation);
	Assert(MagSquared(targetOrientationA).Getf() < 1.02f && IsTrueAll(IsFinite(targetOrientationA)));

	targetOrientationA = PrepareSlerp(orientationA, targetOrientationA);
	Assert(MagSquared(targetOrientationA).Getf() < 1.02f && IsTrueAll(IsFinite(targetOrientationA)));

	QuatV relOrientation = Multiply(targetOrientationA, Invert(orientationA));
	Assert(MagSquared(relOrientation).Getf() < 1.02f && IsTrueAll(IsFinite(relOrientation)));

	Vec3V rotation = Scale(GetUnitDirectionSafe(relOrientation),Vec3VFromF32(RCC_QUATERNION(relOrientation).GetAngle()));
	Vec3V worldAxisB = Transform3x3(instanceB ? instanceB->GetMatrix() : Mat34V(V_IDENTITY),m_ConstraintAxisB);
	rotation = Scale(worldAxisB,Dot(rotation,worldAxisB));

	ScalarV _zero(V_ZERO);

#if FAST_RotationVirtualUpdate

	const bool isSoftLimitStrengthPositive = m_SoftLimitStrength > 0.0f ? true: false;

	ScalarV angle = Negate( Dot(rotation,worldAxisB) );
	ScalarV minLimitV = ScalarVFromF32(m_MinLimit);
	ScalarV maxLimitV = ScalarVFromF32(m_MaxLimit);
	ScalarV underMin = CanonicalizeAngle( Subtract( minLimitV, angle ) );
	ScalarV overMax = CanonicalizeAngle( Subtract( angle, maxLimitV ) );
	Vec3V relAngVelocity = GetAngularVelocity(instanceA,colliderA,m_ComponentA);
	relAngVelocity -= GetAngularVelocity(instanceB,colliderB,m_ComponentB);

	BoolV cond0 = IsEqual( minLimitV, maxLimitV );
	BoolV cond1 = IsGreaterThan( underMin, _zero );
	BoolV cond2 = IsGreaterThan( _zero, overMax );
	BoolV cond3 = IsLessThan( Abs(underMin), Abs(overMax) );
	BoolV condFinal = Or( And(And(cond0,cond1), cond2), cond3 );

	ScalarV depth  = SelectFT( condFinal, overMax, underMin );
	Vec3V normalA  = SelectFT( condFinal, Negate(worldAxisB), worldAxisB );
	relAngVelocity = SelectFT( condFinal, Negate(relAngVelocity), relAngVelocity );

	phContact* contact = GetContactPoint(m_Manifold);
	if (!contact)
	{
		FlagForDestruction();
		return;
	}
	contact->SetTargetRelVelocity(Vec3V(V_ZERO));

	bool localAtLimit = IsGreaterThanAll(depth,-phSimulator::GetAllowedAnglePenetrationV()) ? true: false;
	if ( localAtLimit )
	{
		// Fill in the contact data.
		contact->SetWorldNormal(normalA);
		contact->SetDepth(depth);
		m_Manifold->SetConstraintType(phManifold::SLIDING_ROTATION);
		//		m_Manifold->SetConstraintType(m_MinLimit == m_MaxLimit ? phManifold::FIXED_ROTATION : phManifold::SLIDING_ROTATION);

		if (addToSolver)
		{
			AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
		}
	}
	else
	{
		contact->SetPreviousSolution(Vec3V(_zero));
	}

	m_AtLimit = localAtLimit;

	if ( colliderA && !colliderB && IsGreaterThanOrEqualAll(depth,_zero) )
	{
		// We're attached to the world, so set our inv ang inertia
		Vec3V invAngInertia=colliderA->GetSolverInvAngInertia();
		invAngInertia-=m_ConstraintAxisA*Dot(invAngInertia,m_ConstraintAxisA);
		colliderA->SetTranslationConstraintInvAngInertia(RCC_VECTOR3(invAngInertia));
	}

	// Soft limits
	if( isSoftLimitStrengthPositive )
	{
		ScalarV minSoftLimitV = ScalarVFromF32(m_MinSoftLimit);
		ScalarV maxSoftLimitV = ScalarVFromF32(m_MaxSoftLimit);

		ScalarV underMin = CanonicalizeAngle( Subtract(minSoftLimitV,angle) );
		ScalarV overMax = CanonicalizeAngle( Subtract(angle,maxSoftLimitV) );

		BoolV cond0 = IsEqual( minSoftLimitV, maxSoftLimitV );
		BoolV cond1 = IsGreaterThan( underMin, _zero );
		BoolV cond2 = IsGreaterThan( _zero, overMax );
		BoolV cond3 = IsLessThan( Abs(underMin), Abs(overMax) );
		BoolV condFinal = Or( And(And(cond0,cond1), cond2), cond3 );

		ScalarV depth  = SelectFT( condFinal, overMax, underMin );
		Vec3V normalA  = SelectFT( condFinal, Negate(worldAxisB), worldAxisB );
		relAngVelocity = SelectFT( condFinal, Negate(relAngVelocity), relAngVelocity );

#else


	const float angle = -Dot(rotation,worldAxisB).Getf();
	const float underMin = SubtractAngleShorter(m_MinLimit,angle);
	const float overMax = SubtractAngleShorter(angle,m_MaxLimit);
	Vec3V relAngVelocity = GetAngularVelocity(instanceA,colliderA,m_ComponentA);
	relAngVelocity -= GetAngularVelocity(instanceB,colliderB,m_ComponentB);
	ScalarV depth;
	Vec3V normalA;
	if ((m_MinLimit == m_MaxLimit && underMin > 0.0f && overMax < 0.0f) || FPAbs(underMin) < FPAbs(overMax))
	{
		// The current angle is closer to the minimum than to the maximum.
		depth = ScalarVFromF32(underMin);
		normalA = worldAxisB;
	}
	else
	{
		// The current angle is closer to the maximum than to the minimum.
		depth = ScalarVFromF32(overMax);
		normalA = Negate(worldAxisB);
		relAngVelocity = Negate(relAngVelocity);
	}

	phContact* contact = GetContactPoint(m_Manifold);
	if (!contact)
	{
		FlagForDestruction();
		return;
	}
	contact->SetTargetRelVelocity(Vec3V(V_ZERO));

	if (IsGreaterThanAll(depth,-phSimulator::GetAllowedAnglePenetrationV()))
	{
		m_AtLimit = true;

		// Fill in the contact data.
		contact->SetWorldNormal(normalA);
		contact->SetDepth(depth);
		m_Manifold->SetConstraintType(phManifold::SLIDING_ROTATION);
//		m_Manifold->SetConstraintType(m_MinLimit == m_MaxLimit ? phManifold::FIXED_ROTATION : phManifold::SLIDING_ROTATION);

		if (addToSolver)
		{
			AddManifoldToSolver(m_Manifold, instanceA, colliderA, instanceB, colliderB);
		}
	}
	else
	{
		contact->SetPreviousSolution(Vec3V(_zero));

		m_AtLimit = false;
	}

	if ( colliderA && !colliderB && IsGreaterThanOrEqualAll(depth,_zero) )
	{
		// We're attached to the world, so set our inv ang inertia
		Vec3V invAngInertia=colliderA->GetSolverInvAngInertia();
		invAngInertia-=m_ConstraintAxisA*Dot(invAngInertia,m_ConstraintAxisA);
		colliderA->SetTranslationConstraintInvAngInertia(RCC_VECTOR3(invAngInertia));
	}

	// Soft limits
	if (m_SoftLimitStrength > 0.0f)
	{
		const float underMin = SubtractAngleShorter(m_MinSoftLimit,angle);
		const float overMax = SubtractAngleShorter(angle,m_MaxSoftLimit);

		if ((m_MinSoftLimit == m_MaxSoftLimit && underMin > 0.0f && overMax < 0.0f) || FPAbs(underMin) < FPAbs(overMax))
		{
			// The current angle is closer to the minimum than to the maximum.
			depth = ScalarVFromF32(underMin);
			normalA = worldAxisB;
		}
		else
		{
			// The current angle is closer to the maximum than to the minimum.
			depth = ScalarVFromF32(overMax);
			normalA = Negate(worldAxisB);
			relAngVelocity = Negate(relAngVelocity);
		}

#endif // FAST_RotationVirtualUpdate
		if (IsLessThanAll(depth, _zero))
		{
			Vec3V angAcceleration = Scale(normalA, depth);
			angAcceleration = Scale(angAcceleration, Vec3VFromF32(m_SoftLimitStrength));

			// Find the angular velocity of object B.
			Vec3V relAngVelocity;
			if (instanceB)
			{
				if (colliderB)
				{
					relAngVelocity = colliderB->GetAngVelocity();
				}
				else
				{
					relAngVelocity = instanceB->GetExternallyControlledAngVelocity();
				}
			}
			else
			{
				relAngVelocity = Vec3V(_zero);
			}

			// Subtract the angular velocity of object A.
			if (colliderA)
			{
				relAngVelocity = Subtract(relAngVelocity, colliderA->GetAngVelocity());
			}
			else
			{
				Vec3V angVelocityA = instanceA->GetExternallyControlledAngVelocity();
				relAngVelocity = Subtract(relAngVelocity, angVelocityA);
			}

			// Add the torque from damping on object A.
			angAcceleration = AddScaled(angAcceleration, relAngVelocity, Vec3VFromF32(m_SoftLimitDamping));

	#if FAST_RotationVirtualUpdate
			// Make sure the angular acceleration isn't enough to shoot past the target.
			ScalarV timeStep = InvertFast( ScalarV(invTimeStep) );
			ScalarV delAngleMag = Scale( Mag(angAcceleration), Scale(timeStep,timeStep));
			ScalarV angleMag = Abs(angle);

			angAcceleration = SelectFT( IsGreaterThan(delAngleMag,angleMag ), angAcceleration, 
				Scale(angAcceleration, Vec3V(Scale(angleMag, InvertFast(delAngleMag))) )
				);
	#else

			// Make sure the angular acceleration isn't enough to shoot past the target.
			float timeStep = 1.0f/ScalarV(invTimeStep).Getf();
			float delAngleMag = Mag(angAcceleration).Getf() * square(timeStep);
			float angleMag = FPAbs(angle);
			if (delAngleMag>angleMag)
			{
				angAcceleration = Scale(angAcceleration, Vec3VFromF32(angleMag/delAngleMag));
			}

	#endif // FAST_RotationVirtualUpdate

			Mat33V angInertia(V_ZERO);
			if (colliderA)
			{
				if (colliderB)
				{
					// There are two colliders, so invert the sum of their inverse inertia matrices to get the effective angular inertia.
					colliderA->GetInverseInertiaMatrix(angInertia);
					Mat33V invInertiaB;
					colliderB->GetInverseInertiaMatrix(invInertiaB);
					Add(angInertia, angInertia, invInertiaB);
					InvertFull(angInertia, angInertia);
				}
				else
				{
					// There is only one collider, so get its inertia matrix.
					colliderA->GetInertiaMatrix(angInertia);
				}
			}
			else if (colliderB)
			{
				// There is only one collider, so get its inertia matrix.
				colliderB->GetInertiaMatrix(angInertia);
			}

			// Use the angular acceleration and the angular inertia to get the torque.
			Vec3V torque = Multiply( angInertia, angAcceleration );

			// Apply the torque to object A.
			if (colliderA)
			{
	#if FAST_RotationVirtualUpdate
				colliderA->ApplyTorque(torque.GetIntrin128(), timeStep.GetIntrin128ConstRef());
	#else
				colliderA->ApplyTorque(torque.GetIntrin128(), ScalarVFromF32(timeStep).GetIntrin128ConstRef());
	#endif
			}
			else
			{
				instanceA->ApplyTorque(torque.GetIntrin128());
			}

			if (instanceB)
			{
				// Negate the force and torque and apply them to object B.
				torque = Negate(torque);
				if (colliderB)
				{
	#if FAST_RotationVirtualUpdate
					colliderB->ApplyTorque(torque.GetIntrin128(), timeStep.GetIntrin128ConstRef());
	#else
					colliderB->ApplyTorque(torque.GetIntrin128(), ScalarVFromF32(timeStep).GetIntrin128ConstRef());
	#endif
				}
				else
				{
					instanceB->ApplyTorque(torque.GetIntrin128());
				}
			}
		}
	}
}

Vec3V_Out phConstraintRotation::GetAngularVelocity (phInst* instance, phCollider* collider, int component)
{
	if (collider)
	{
		// This object is active, so get its collider's angular velocity.
		if (!collider->IsArticulated())
		{
			// This is a rigid collider, so get its angular velocity.
			return collider->GetAngVelocity();
		}
		else
		{
			// This is an articulated collider, so get the angular velocity of the constrained component.
			return static_cast<phArticulatedCollider*>(collider)->GetAngVelocityOfBodyPart(component);
		}
	}
	else if (instance)
	{
		// This object is not active, so get its angular velocity from the instance.
		Vector3 angVelocity = VEC3V_TO_VECTOR3(instance->GetExternallyControlledAngVelocity());
		return VECTOR3_TO_VEC3V(angVelocity);
	}

	return Vec3V(V_ZERO);
}

// We make the off-axis inertia infinite, but the on-axis we make the object heavier. This fixes a problem where a constrained
// object rotates into a dynamic object, pinning it against a wall, which freaks out if we make all directions infinite. Without
// the multiplier though, heavy objects hitting doors cause the doors to freak out.
static float s_InvInertiaMultiplier = 0.1f;

void phConstraintRotation::EnforceWorldConstraints()
{
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

bool phConstraintRotation::UpdateBreaking()
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

void phConstraintRotation::ProfileDraw()
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

		Vec3V limitsDrawDirection = Transform3x3(worldMatrixB, m_LimitsDrawDirection);
		Vec3V crossDirection = Cross(worldAxisB, limitsDrawDirection);

		grcColor(Color_white);
		grcDrawArc(PFD_ConstraintLimitScale.GetValue() * 0.8f,
			VEC3V_TO_VECTOR3(limitPosition),
			VEC3V_TO_VECTOR3(limitsDrawDirection),
			VEC3V_TO_VECTOR3(crossDirection),
			m_MinLimit,
			m_MaxLimit,
			37);

		Vector3 minLimitEnd;
		minLimitEnd.SetScaled(VEC3V_TO_VECTOR3(limitsDrawDirection),cosf(m_MinLimit));
		minLimitEnd.AddScaled(VEC3V_TO_VECTOR3(crossDirection),sinf(m_MinLimit));
		minLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
		minLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

		grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), minLimitEnd, Color_white);

		Vector3 maxLimitEnd;
		maxLimitEnd.SetScaled(VEC3V_TO_VECTOR3(limitsDrawDirection),cosf(m_MaxLimit));
		maxLimitEnd.AddScaled(VEC3V_TO_VECTOR3(crossDirection),sinf(m_MaxLimit));
		maxLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
		maxLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

		grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), maxLimitEnd, Color_white);

		// Soft limits
		if (m_SoftLimitStrength > 0.0f)
		{
			Vector3 minSoftLimitEnd;
			minSoftLimitEnd.SetScaled(VEC3V_TO_VECTOR3(limitsDrawDirection),cosf(m_MinSoftLimit));
			minSoftLimitEnd.AddScaled(VEC3V_TO_VECTOR3(crossDirection),sinf(m_MinSoftLimit));
			minSoftLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
			minSoftLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

			grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), minSoftLimitEnd, Color_yellow);

			Vector3 maxSoftLimitEnd;
			maxSoftLimitEnd.SetScaled(VEC3V_TO_VECTOR3(limitsDrawDirection),cosf(m_MaxSoftLimit));
			maxSoftLimitEnd.AddScaled(VEC3V_TO_VECTOR3(crossDirection),sinf(m_MaxSoftLimit));
			maxSoftLimitEnd.Scale(PFD_ConstraintLimitScale.GetValue());
			maxSoftLimitEnd.Add(VEC3V_TO_VECTOR3(limitPosition));

			grcDrawLine(VEC3V_TO_VECTOR3(limitPosition), maxSoftLimitEnd, Color_yellow);
		}
	}
}
#endif

} // namespace rage
