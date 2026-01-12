//
// physics/constraintdistance.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_DISTANCE_H
#define PHYSICS_CONSTRAINT_DISTANCE_H

#include "constraintbase.h"

namespace rage {

// Keep a point on two objects closer than, or further than (or both), a certain distance from each other
class phConstraintDistance : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// World space position of attach point on object A
		Vec3V worldAnchorA;

		// World space position of attach point on object B
		Vec3V worldAnchorB;

		//Alternatively you can specify the two points to constrain in local space. Set constructUsingLocalPositions to true to construct the constraint this way
		Vec3V localPosA;
		Vec3V localPosB;
		bool  constructUsingLocalPositions;

		// The minimum distance between the two attach points (if zero, no minimum distance is enforced)
		float minDistance;

		// The maximum distance between the two attach points (if zero or less, the initial distance becomes the max)
		float maxDistance;

		// A depth of allowed penetration beyond the distance limits...use this to increase stability
		float allowedPenetration;

		// Mode which does not enforce hard limits until the points come within those limits naturally (However it does prevent them moving farther apart in the meantime)
		bool softAttach;

		Params()
			: phConstraintBase::Params(DISTANCE)
			, worldAnchorA(V_ZERO)
			, worldAnchorB(V_ZERO)
			, minDistance(0.0f)
			, maxDistance(0.0f)
			, allowedPenetration(0.0f)
			, softAttach(false)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
		{
		}

		Params(const phConstraintBase::Params& baseParams)
			: phConstraintBase::Params(baseParams, DISTANCE)
			, worldAnchorA(V_ZERO)
			, worldAnchorB(V_ZERO)
			, minDistance(0.0f)
			, maxDistance(0.0f)
			, allowedPenetration(0.0f)
			, softAttach(false)
			, localPosA(V_ZERO)
			, localPosB(V_ZERO)
			, constructUsingLocalPositions(false)
		{
		}
	};

	phConstraintDistance(const Params& params);
	virtual ~phConstraintDistance();

	virtual const char* GetTypeName()
	{
		return "distance";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual void SetWorldPosA(Vec3V_In worldPosA);
	virtual void SetWorldPosB(Vec3V_In worldPosB);
	void SetLocalPosA(Vec3V_In localPosA);
	void SetLocalPosB(Vec3V_In localPosB);
	Vec3V_Out GetWorldPosA();
	Vec3V_Out GetWorldPosB();
	Vec3V_Out GetLocalPosA();
	Vec3V_Out GetLocalPosB();

	float GetMaxDistance() const
	{
		return m_MaxDistance;
	}

	float GetAllowedPenetration() const { return m_AllowedPenetration; }

	// PURPOSE: Set the maximum distance of the constraint.
	// PARAMS:
	//	maxDistance - new maximum distance between the two objects
	void SetMaxDistance(float maxDistance);

	// PURPOSE: Set the minimum distance of the constraint.
	// PARAMS:
	//	minDistance - new minimum distance between the two objects
    void SetMinDistance(float minDistance);

	// PURPOSE: Control the relative velocity of the objects along the separation axis.
	//          Useful when shortening or lengthening the rope.
	// PARAMS:
	//   separationVelocity - target relative velocity along the separation axis. Positive means the 
	//                          objects will move away from each other, negative means they will go towards each other. 
	// NOTES: This only matters when the constraint is at its limit
	void SetSeparationVelocity(float separationVelocity);

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
	// The minimum distance between the two attach points (if zero, no minimum distance is enforced)
	float m_MinDistance;

	// The maximum distance between the two attach points
	float m_MaxDistance;

	// Matrix for the manifold
	Mat33V m_ConstraintMatrix;

	// The manifold for the constraint
	phManifold* m_Manifold;

	float m_SeparationVelocity;

	// A depth of allowed penetration beyond the distance limits...use this to increase stability
	float m_AllowedPenetration;

	// Mode which does not enforce hard limits until the points come within those limits naturally (However it does prevent them moving farther apart in the meantime)
	bool m_SoftAttach;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_DISTANCE_H
