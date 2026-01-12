//
// physics/constraintrotation.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_ROTATION_H
#define PHYSICS_CONSTRAINT_ROTATION_H

#include "constraintbase.h"

#include "vectormath/mat33v.h"

namespace rage {

// Constrain rotation along one axis
class phConstraintRotation : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// The direction of the axis rotation in world space
		Vec3V worldAxis;

		// Where in world space to position the profile drawing (moves with object A)
		Vec3V worldDrawPosition;

		// The maximum number of radians the object can rotate from its starting position around the axis
		float minLimit;

		// The same, but in the opposite direction
		float maxLimit;

		// When beyond this angle, a restoring force will push it back towards this angle
		float minSoftLimit;

		// When beyond this angle in the other direction, a restoring force will push it back towards this angle
		float maxSoftLimit;

		// How strong of a soft limit torque to apply
		float softLimitStrength;

		// How much damping to apply when computing the soft limit torque
		float softLimitDamping;

		Params()
			: phConstraintBase::Params(ROTATION)
			, worldAxis(V_Z_AXIS_WZERO)
			, worldDrawPosition(V_FLT_LARGE_8)
			, minLimit(-PI)
			, maxLimit(PI)
			, minSoftLimit(-PI)
			, maxSoftLimit(PI)
			, softLimitStrength(0.0f)
			, softLimitDamping(1.0f)
		{
		}

		Params(const phConstraintBase::Params& params, Vec3V_In _worldAxis,
#if __PFDRAW
			Vec3V_In _worldDrawPosition,
#endif // __PFDRAW
			float _minLimit, float _maxLimit, float _minSoftLimit, float _maxSoftLimit, float _softLimitStrength, float _softLimitDamping)
				: phConstraintBase::Params(params, ROTATION)
				, worldAxis(_worldAxis)
#if __PFDRAW
				, worldDrawPosition(_worldDrawPosition)
#endif // __PFDRAW
				, minLimit(_minLimit)
				, maxLimit(_maxLimit)
				, minSoftLimit(_minSoftLimit)
				, maxSoftLimit(_maxSoftLimit)
				, softLimitStrength(_softLimitStrength)
				, softLimitDamping(_softLimitDamping)
		{
		}
	};

	phConstraintRotation(const Params& params);
	virtual ~phConstraintRotation();

	virtual const char* GetTypeName()
	{
		return "rotation";
	}

	void SetMinLimit(float minLimit)
	{
		m_MinLimit = minLimit;
	}

	void SetMaxLimit(float maxLimit)
	{
		m_MaxLimit = maxLimit;
	}

	void SetWorldAxis(Vec3V_In worldAxis);
	void SetWorldAxis(Vec3V_In worldAxis, phInst* instanceA, phInst* instanceB);

//interface for simulator
public:

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();

	virtual void EnforceWorldConstraints();

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_Manifold);
	}

private:

	Vec3V_Out GetAngularVelocity (phInst* instance, phCollider* collider, int component);

	phManifold* m_Manifold; // Manifold for the limit rotation constraint

	float m_MinLimit; // The maximum number of radians the object can rotate from its starting position
	float m_MaxLimit; // The same, but in the other direction

	float m_MinSoftLimit;
	float m_MaxSoftLimit;
	float m_SoftLimitStrength;
	float m_SoftLimitDamping;

	Mat33V m_ConstraintMatrix; // Matrix for the limit constraint

	Vec3V m_ConstraintAxisA; // The axis of rotation in object A space
	Vec3V m_ConstraintAxisB; // The same, but in B space

#if __PFDRAW
	Vec3V m_LimitsDrawDirection; // World draw direction for limits
	Vec3V m_NeedleDrawDirection; // Object A space direction for needle drawing
	Vec3V m_LocalDrawPosition; // Where to draw the limits, relative to object A
#endif

	QuatV m_RelOrientation; // The initial relative orientation of object A and B

	friend class phConstraintHinge;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_ROTATION_H
