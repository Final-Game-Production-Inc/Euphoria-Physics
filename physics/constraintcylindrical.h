//
// physics/constraintcylindrical.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONSTRAINT_CYLINDRICAL_H
#define PHYSICS_CONSTRAINT_CYLINDRICAL_H

#include "constraintbase.h"

#include "vectormath/mat33v.h"

namespace rage {

	// Constrain rotation to one axis
	class phConstraintCylindrical : public phConstraintBase
	{
	public:

		// Pass this into PHCONSTRAINT->Insert to create the constraint
		struct Params : public phConstraintBase::Params
		{
			// The direction of the axis of rotation in world space
			Vec3V worldAxis;

			// Where in world space to position the profile drawing (moves with object A)
			Vec3V worldDrawPosition;

			// The maximum number of radians the object can rotate away from the axis
			float maxLimit;

			Params()
				: phConstraintBase::Params(CYLINDRICAL)
				, worldAxis(V_Z_AXIS_WZERO)
				, worldDrawPosition(V_FLT_LARGE_8)
				, maxLimit(0)
			{
			}

			Params(const phConstraintBase::Params& params, Vec3V_In _worldAxis,
#if __PFDRAW
				Vec3V_In _worldDrawPosition,
#endif // __PFDRAW
				float _maxLimit)
				: phConstraintBase::Params(params, CYLINDRICAL)
				, worldAxis(_worldAxis)
#if __PFDRAW
				, worldDrawPosition(_worldDrawPosition)
#endif // __PFDRAW
				, maxLimit(_maxLimit)
			{
			}
		};

		phConstraintCylindrical(const Params& params);
		virtual ~phConstraintCylindrical();

		virtual const char* GetTypeName()
		{
			return "cylindrical";
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

		phManifold* m_Manifold; // Manifold for the limit rotation constraint

		float m_MaxLimit; // The maximum number of radians the object can rotate away from the axis

		Mat33V m_ConstraintMatrix; // Matrix for the limit constraint

		Vec3V m_ConstraintAxisA; // The axis of rotation in object A space
		Vec3V m_ConstraintAxisB; // The same, but in B space

#if __PFDRAW
		Vec3V m_LimitsDrawDirection; // World draw direction for limits
		Vec3V m_NeedleDrawDirection; // Object A space direction for needle drawing
		Vec3V m_LocalDrawPosition; // Where to draw the limits, relative to object A
#endif

		QuatV m_RelOrientation; // The initial relative orientation of object A and B
	};


}  // namespace rage

#endif	// PHYSICS_CONSTRAINT_CYLINDRICAL_H
