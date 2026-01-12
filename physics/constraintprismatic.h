//
// physics/constraintprismatic.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_PRISMATIC_H
#define PHYSICS_CONSTRAINT_PRISMATIC_H

#include "constraintbase.h"
#include "constraintfixedrotation.h"

#include "vectormath/mat33v.h"
#include "vectormath/mat34v.h"

namespace rage {

// Two objects can slide across each other but not rotate
class phConstraintPrismatic : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// The slider is assumed to be at zero displacement initially
		float minDisplacement; //minDisplacement <= 0
		float maxDisplacement; //maxDisplacement >= 0

		// Expressed in the local space of object B
		Vec3V slideAxisLocalB;
		
		Params()
			: phConstraintBase::Params(PRISMATIC)
			, minDisplacement(0.0f)
			, maxDisplacement(0.0f)
			, slideAxisLocalB(V_X_AXIS_WZERO)
		{
		}
	};

//interface for users
	virtual void SetBreakable(bool breakable=true, float breakingStrength=10000.0f);
	virtual void SetBroken(bool broken=true);

//interface for simulator
public:

	phConstraintPrismatic(const Params& params);
	virtual ~phConstraintPrismatic();

	virtual const char* GetTypeName()
	{
		return "prismatic";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();
	virtual void VirtualEnforceWorldConstraints(phCollider* collider);
	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_Manifolds[ePCM_SLIDEAXIS_MINLIMIT]);
		DisableManifold(m_Manifolds[ePCM_SLIDEAXIS_MAXLIMIT]);
		DisableManifold(m_Manifolds[ePCM_PERPAXIS1_MINLIMIT]);
		DisableManifold(m_Manifolds[ePCM_PERPAXIS1_MAXLIMIT]);
		DisableManifold(m_Manifolds[ePCM_PERPAXIS2_MINLIMIT]);
		DisableManifold(m_Manifolds[ePCM_PERPAXIS2_MAXLIMIT]);

		m_RotationConstraint->DisableManifolds();
	}

private:

	Vec3V_Out GetComponentCentreOfMassInLocalSpace(phInst* instance, int component);

	enum ePrismaticConstraintManifolds
	{
		ePCM_SLIDEAXIS_MINLIMIT,
		ePCM_SLIDEAXIS_MAXLIMIT,
		ePCM_PERPAXIS1_MINLIMIT,
		ePCM_PERPAXIS1_MAXLIMIT,
		ePCM_PERPAXIS2_MINLIMIT,
		ePCM_PERPAXIS2_MAXLIMIT,
		ePCM_COUNT,
	};

	phManifold* m_Manifolds[ePCM_COUNT];
	Mat33V		m_ConstraintMatrices[ePCM_COUNT];

	Mat34V		m_SlideCoordinateSystem_LocalSpaceB;	//Slide axis and two perpendicular vectors (that all move with B)
														//Origin of A is at (0,0,0) in the slide coordinate system initially
	Vec3V		m_MinLimits, m_MaxLimits;				//e.g. (-1,0,0) to (4,0,0). In the slide coord system. Initial displacement is (0,0,0)
	
	phConstraintFixedRotation* m_RotationConstraint;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_FIXED_H
