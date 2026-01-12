//
// physics/constraintfixed.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_FIXED_H
#define PHYSICS_CONSTRAINT_FIXED_H

#include "constraintbase.h"

#include "vectormath/mat33v.h"

namespace rage {

// Two objects are completely unable to move relative to each other
class phConstraintFixed : public phConstraintBase
{
public:

	// Pass this into PHCONSTRAINT->Insert to create the constraint
	struct Params : public phConstraintBase::Params
	{
		// Set this to false if you only want rotation to be fixed
		bool translation;

		Params()
			: phConstraintBase::Params(FIXED)
			, translation(true)
		{
		}
	};

//interface for simulator
public:

	phConstraintFixed(const Params& params);
	virtual ~phConstraintFixed();

	virtual const char* GetTypeName()
	{
		return "fixed";
	}

#if __PFDRAW
	virtual void ProfileDraw();
#endif

	virtual bool UpdateBreaking();

	virtual void VirtualUpdate(Vec::V3Param128 invTimeStep, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB, bool addToSolver);

	void SetLocalPosA(Vec3V_In localPosB);
	void SetRelativeRotation(QuatV_In relativeRotation);

protected:
	virtual void DisableManifolds()
	{
		DisableManifold(m_TranslationManifold);
		DisableManifold(m_RotationManifold);
	}

private:

	// The manifold for the translation constraint
	phManifold* m_TranslationManifold;

	// The manifold for the rotation constraint
	phManifold* m_RotationManifold;

	// The matrix for the translation constraint
	Mat33V m_TranslationConstraintMatrix;

	// The matrix for the rotation constraint
	Mat33V m_RotationConstraintMatrix;

	// The initial relative orientation of the two objects
	QuatV m_RelOrientation;
};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_FIXED_H
