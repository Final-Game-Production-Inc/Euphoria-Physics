//
// physics/constraintfixedrotation.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_FIXED_ROTATION_H
#define PHYSICS_CONSTRAINT_FIXED_ROTATION_H

#include "constraintbase.h"

#include "vectormath/mat33v.h"

namespace rage {

//Makes two objects rotate together
class phConstraintFixedRotation : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		Params()
			: phConstraintBase::Params(FIXEDROTATION)
		{
		}

		Params(const phConstraintBase::Params& baseParams)
			: phConstraintBase::Params(baseParams, FIXEDROTATION)
		{
		}
	};

//interface for simulator
public:

	phConstraintFixedRotation(const Params& params);
	virtual ~phConstraintFixedRotation();

	virtual const char* GetTypeName()
	{
		return "fixedrotation";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();

	virtual void VirtualEnforceWorldConstraints(phCollider* collider);

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);
	
	virtual void DisableManifolds()
	{
		DisableManifold(m_RotationManifold);
	}

private:

	// The manifold for the rotation constraint
	phManifold* m_RotationManifold;

	// The matrix for the rotation constraint
	Mat33V m_RotationConstraintMatrix;

	// The initial relative orientation of the two objects
	QuatV m_RelOrientation;
};

}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_FIXED_ROTATION_H
