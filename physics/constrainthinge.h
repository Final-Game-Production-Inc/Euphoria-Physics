//
// physics/constrainthinge.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_HINGE_H
#define PHYSICS_CONSTRAINT_HINGE_H

#include "constraintbase.h"
#include "constraintrotation.h"

namespace rage {

class phConstraintHinge : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// A world point along the axis of rotation
		Vec3V worldAnchor;

		// The direction of the axis rotation in world space
		Vec3V worldAxis;

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

		// If the entity is very thin in the worldAxis direction then the hinge can be unstable because its two fixed points
		// (which are found by querying the phBound's support mapping and projecting onto the worldAxis) will be very close. 
		// If less than this distance apart they will be moved apart so that they are this far apart
		float minFixedPointSeparation;

		// This will create a third fixed point constraint at the center of the constraint (not necessarily halfway between the 
		//   two constraints). This can greatly improve stability when using a minFixedPointSeparation on a thin hinge. This should
		//   only be enabled if necessary since it's so costly. 
		bool useMiddleFixedPoint;

		Params()
			: phConstraintBase::Params(HINGE)
			, worldAnchor(V_ZERO)
			, worldAxis(V_Z_AXIS_WZERO)
			, minLimit(-PI)
			, maxLimit(PI)
			, minSoftLimit(-PI)
			, maxSoftLimit(PI)
			, softLimitStrength(0.0f)
			, softLimitDamping(0.0f)
			, minFixedPointSeparation(2.0f)
			, useMiddleFixedPoint(false)
		{
		}
	};

	virtual void SetBreakable(bool breakable=true, float breakingStrength=10000.0f);
	virtual void SetBroken(bool broken=true);

//interface for simulator
public:

	phConstraintHinge(const Params& params);
	virtual ~phConstraintHinge();

#if !__FINAL
	virtual void SetOwner(const char* file, int line);
#endif

	virtual const char* GetTypeName()
	{
		return "hinge";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();

	virtual void EnforceWorldConstraints();

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_FixedPointTopManifold);
		DisableManifold(m_FixedPointBottomManifold);

		m_RotationConstraint.DisableManifolds();
	}

private:

	bool VirtualUpdateFixedPointManifold(phManifold* manifold, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

	phManifold* m_FixedPointTopManifold; // Manifold for the top fixed-point constraint
	phManifold* m_FixedPointBottomManifold; // Manifold for the bottom fixed-point constraint
	phManifold* m_FixedPointMiddleManifold; // Manifold for the optional middle fixed point constraint
	
	bool m_LimitsEnabled; // True if limits were provided

	Mat33V m_FixedPointTopConstraintMatrix; // Matrix for the top fixed-point constraint
	Mat33V m_FixedPointBottomConstraintMatrix; // Matrix for the bottom fixed-point constraint
	Mat33V m_FixedPointMiddleConstraintMatrix; // Matrix for the optional middle fixed-point constraint


	phConstraintRotation m_RotationConstraint;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_HINGE_H
