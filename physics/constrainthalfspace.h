//
// physics/constrainthalfspace.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_HALF_SPACE_H
#define PHYSICS_CONSTRAINT_HALF_SPACE_H

#include "constraintbase.h"

namespace rage {

// Keep a point on two objects from moving past each other along a user-defined direction. This is really the same interally as a contact.
class phConstraintHalfSpace : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// World normal along which to enforce the constraint
		Vec3V worldNormal;

		// World space position of attach point on object A
		Vec3V worldAnchorA;

		// World space position of attach point on object B
		Vec3V worldAnchorB;

		//Alternatively you can specify the two points to constrain in local space. Set constructUsingLocalPositions to true to construct the constraint this way
		Vec3V localPosA;
		Vec3V localPosB;
		bool  constructUsingLocalPositions;

		Params()
			: phConstraintBase::Params(HALFSPACE)
			, worldNormal(V_Z_AXIS_WZERO)
			, worldAnchorA(V_ZERO)
			, worldAnchorB(V_ZERO)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
		{
		}

		Params(const phConstraintBase::Params& baseParams)
			: phConstraintBase::Params(baseParams, HALFSPACE)
			, worldNormal(V_Z_AXIS_WZERO)
			, worldAnchorA(V_ZERO)
			, worldAnchorB(V_ZERO)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
		{
		}
	};

	phConstraintHalfSpace(const Params& params);
	virtual ~phConstraintHalfSpace();

	virtual const char* GetTypeName()
	{
		return "half-space";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	void SetWorldPosA(Vec3V_In worldPosA);
	void SetWorldPosB(Vec3V_In worldPosB);
	void SetLocalPosA(Vec3V_In localPosA);
	void SetLocalPosB(Vec3V_In localPosB);
	void SetWorldNormal(Vec3V_In worldNormal);
	Vec3V_Out GetWorldPosA();
	Vec3V_Out GetWorldPosB();
	Vec3V_Out GetLocalPosA();
	Vec3V_Out GetLocalPosB();

	virtual bool UpdateBreaking();

//interface for simulator
public:

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_Manifold);
	}

private:

	// The manifold for the constraint
	phManifold* m_Manifold;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_HALF_SPACE_H
