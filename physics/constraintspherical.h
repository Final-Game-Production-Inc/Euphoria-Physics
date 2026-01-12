//
// physics/constraintspherical.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_SPHERICAL_H
#define PHYSICS_CONSTRAINT_SPHERICAL_H

#include "constraintbase.h"

#include "vectormath/mat33v.h"

namespace rage {

// Create a spherical (aka ball-and-socket) joint between two objects
class phConstraintSpherical : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// The world position at which the object or objects are attached
		Vec3V worldPosition;

		//Alternatively you can specify the two points to constrain in local space. Set constructUsingLocalPositions to true to construct the constraint this way
		Vec3V localPosA;
		Vec3V localPosB;
		bool  constructUsingLocalPositions;

		// Mode which does not enforce hard limits until the points come within those limits naturally (However it does prevent them moving farther apart in the meantime)
		bool softAttach;

		Params()
			: phConstraintBase::Params(SPHERICAL)
			, worldPosition(V_ZERO)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
			, softAttach(false)
		{
		}

		Params(const phConstraintBase::Params& baseParams)
			: phConstraintBase::Params(baseParams, SPHERICAL)
			, worldPosition(V_ZERO)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
			, softAttach(false)
		{
		}
	};

	virtual void SetWorldPosA(Vec3V_In worldPosA);
	virtual void SetWorldPosB(Vec3V_In worldPosB);
	void SetLocalPosA(Vec3V_In localPosA);
	void SetLocalPosB(Vec3V_In localPosB);

	Vec3V_Out GetWorldPosA();
	Vec3V_Out GetWorldPosB();
	Vec3V_Out GetLocalPosA();
	Vec3V_Out GetLocalPosB();

//interface for simulator
public:

	phConstraintSpherical(const Params& params);
	virtual ~phConstraintSpherical();

	virtual const char* GetTypeName()
	{
		return "spherical";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

	virtual void VirtualEnforceWorldConstraints(phCollider* collider);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_Manifold);
	}

private:

	// The manifold for the constraint
	phManifold* m_Manifold;

	// The matrix for the constraint
	Mat33V m_ConstraintMatrix;

	// Mode which does not enforce hard limits until the points come within those limits naturally (However it does prevent them moving farther apart in the meantime)
	bool m_SoftAttach;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_SPHERICAL_H
