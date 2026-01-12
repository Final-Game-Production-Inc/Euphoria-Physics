//
// pharticulated/jointdispatch.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_JOINTDISPATCH_TYPE_H
#define PHARTICULATED_JOINTDISPATCH_TYPE_H

#include "joint1dof.h"
#include "joint3dof.h"
#include "prismaticjoint.h"

namespace rage {

//////////////////////////////////////////////////////////////////////////
// phJoint dispatch functions
//////////////////////////////////////////////////////////////////////////

__forceinline void phJoint::MatchChildToParentPositionAndVelocity(phArticulatedBody *body)
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->MatchChildToParentPositionAndVelocity(body);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->MatchChildToParentPositionAndVelocity(body);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->MatchChildToParentPositionAndVelocity(body);
		return;
	}
}

__forceinline int phJoint::ComputeNumHardJointLimitDofs (phArticulatedBody *body)
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		return static_cast<phJoint1Dof*>(this)->ComputeNumHardJointLimitDofs(body);
	case JNT_3DOF:
		return static_cast<phJoint3Dof*>(this)->ComputeNumHardJointLimitDofs(body);
		//case PRISM_JNT:
	default:
		return static_cast<phPrismaticJoint*>(this)->ComputeNumHardJointLimitDofs(body);
	}
}

__forceinline void phJoint::GetJointLimitDetailedInfo (phArticulatedBody *body, int limitNum, int& jointLimitID, float& dofExcess, float& response)
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->GetJointLimitDetailedInfo(body, limitNum, jointLimitID, dofExcess, response);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->GetJointLimitDetailedInfo(body, limitNum, jointLimitID, dofExcess, response);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->GetJointLimitDetailedInfo(body, limitNum, jointLimitID, dofExcess, response);
		return;
	}
}

__forceinline void phJoint::PrecalcResponsesToJointImpulse( phArticulatedBody* theBody, int jointNum, int jointLimitID )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->PrecalcResponsesToJointImpulse(theBody, jointNum, jointLimitID);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->PrecalcResponsesToJointImpulse(theBody, jointNum, jointLimitID);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->PrecalcResponsesToJointImpulse(theBody, jointNum, jointLimitID);
		return;
	}
}

__forceinline float phJoint::GetPrecalcJointLimitResponse( phArticulatedBody* theBody, int jointNum, int jointLimitID )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		return static_cast<phJoint1Dof*>(this)->GetPrecalcJointLimitResponse(theBody, jointNum, jointLimitID);
	case JNT_3DOF:
		return static_cast<phJoint3Dof*>(this)->GetPrecalcJointLimitResponse(theBody, jointNum, jointLimitID);
		//case PRISM_JNT:
	default:
		return static_cast<phPrismaticJoint*>(this)->GetPrecalcJointLimitResponse(theBody, jointNum, jointLimitID);
	}
}

__forceinline void phJoint::ApplyJointImpulse( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->ApplyJointImpulse(body, jointLimitID, impulse);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->ApplyJointImpulse(body, jointLimitID, impulse);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->ApplyJointImpulse(body, jointLimitID, impulse);
		return;
	}
}

__forceinline void phJoint::ApplyJointPush( phArticulatedBody *body, int jointLimitID, ScalarV_In push )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->ApplyJointPush(body, jointLimitID, push);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->ApplyJointPush(body, jointLimitID, push);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->ApplyJointPush(body, jointLimitID, push);
		return;
	}
}

__forceinline void phJoint::ApplyJointImpulseAndPush( phArticulatedBody *body, int jointLimitID, ScalarV_In impulse, ScalarV_In push )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->ApplyJointImpulseAndPush(body, jointLimitID, impulse, push);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->ApplyJointImpulseAndPush(body, jointLimitID, impulse, push);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->ApplyJointImpulseAndPush(body, jointLimitID, impulse, push);
		return;
	}
}

__forceinline void phJoint::GetJointLimitAxis( phArticulatedBody *body, int jointLimitID, Vector3& axis, Vector3& position ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->GetJointLimitAxis(body, jointLimitID, axis, position);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->GetJointLimitAxis(body, jointLimitID, axis, position);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->GetJointLimitAxis(body, jointLimitID, axis, position);
		return;
	}
}

__forceinline void phJoint::GetJointLimitUnitImpulse( int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial ) 
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->GetJointLimitUnitImpulse(jointLimitID, unitImpulseSpatial);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->GetJointLimitUnitImpulse(jointLimitID, unitImpulseSpatial);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->GetJointLimitUnitImpulse(jointLimitID, unitImpulseSpatial);
		return;
	}
}

__forceinline void phJoint::GetPrecalcJointLimitAngleResponse( phArticulatedBody& theBody, 
	int jointNum, int jointLimitID, Vector3& threeReponses )
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->GetPrecalcJointLimitAngleResponse(theBody, jointNum, jointLimitID, threeReponses);
		return;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->GetPrecalcJointLimitAngleResponse(theBody, jointNum, jointLimitID, threeReponses);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->GetPrecalcJointLimitAngleResponse(theBody, jointNum, jointLimitID, threeReponses);
		return;
	}
}

__forceinline void phJoint::TransformByASupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByASupdown(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByASupdown(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByASupdown(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByASdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByASdownup(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByASdownup(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByASdownup(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByASdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByASdownupAndAdd(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByASdownupAndAdd(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByASdownupAndAdd(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByASupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByASupdownAndAdd(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByASupdownAndAdd(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByASupdownAndAdd(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByAupdown( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAupdown(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAupdown(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAupdown(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByAupdownTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAupdownTwice(from, to, from2, to2);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAupdownTwice(from, to, from2, to2);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAupdownTwice(from, to, from2, to2);
		return;
	}
}

__forceinline void phJoint::TransformByAdownup( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAdownup(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAdownup(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAdownup(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByAdownupTwice( const phPhaseSpaceVector& from, phPhaseSpaceVector& to, const phPhaseSpaceVector& from2, phPhaseSpaceVector& to2 ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAdownupTwice(from, to, from2, to2);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAdownupTwice(from, to, from2, to2);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAdownupTwice(from, to, from2, to2);
		return;
	}
}

__forceinline void phJoint::TransformByAdownupAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAdownupAndAdd(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAdownupAndAdd(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAdownupAndAdd(from, to);
		return;
	}
}

__forceinline void phJoint::TransformByAupdownAndAdd( const phPhaseSpaceVector& from, phPhaseSpaceVector& to ) const
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformByAupdownAndAdd(from, to);
		return;
	case JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformByAupdownAndAdd(from, to);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformByAupdownAndAdd(from, to);
		return;
	}
}

__forceinline void phJoint::Freeze ()
{
	switch (GetJointType())
	{
	case JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->Freeze();
		break;
	case JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->Freeze();
		break;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->Freeze();
		break;
	}

	m_VelocityToPropDown.SetZero();
}

__forceinline float phJoint::GetJointLimitResponse (phArticulatedBody *body, int limitDirectionIndex)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		return static_cast<phJoint1Dof*>(this)->GetJointLimitResponse(body, limitDirectionIndex);
	case phJoint::JNT_3DOF:
		return static_cast<phJoint3Dof*>(this)->GetJointLimitResponse(body, limitDirectionIndex);
		//case PRISM_JNT:
	default:
		return static_cast<phPrismaticJoint*>(this)->GetJointLimitResponse(body, limitDirectionIndex);
	}
}
#ifdef USE_SOFT_LIMITS
__forceinline void phJoint::EnforceSoftJointLimits (float timeStep)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->EnforceSoftJointLimits(timeStep);
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->EnforceSoftJointLimits(timeStep);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->EnforceSoftJointLimits(timeStep);
		return;
	}
}
#endif
__forceinline void phJoint::SetAxis (phArticulatedBody *body, const Vector3& jointPosition, const Vector3& axisDirection)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->SetAxis(body, jointPosition, axisDirection);
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->SetAxis(body, jointPosition, axisDirection);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->SetAxis(body, jointPosition, axisDirection);
		return;
	}
}

__forceinline void phJoint::SetAngleLimits (float limit1, float limit2)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->SetAngleLimits(limit1, limit2);
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->SetAngleLimits(limit1, limit2);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->SetAngleLimits(limit1, limit2);
		return;
	}
}

__forceinline void phJoint::ComputeAndApplyMuscleTorques (phArticulatedBody *body, float timeStep)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->ComputeAndApplyMuscleTorques(body, timeStep);
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->ComputeAndApplyMuscleTorques(body, timeStep);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->ComputeAndApplyMuscleTorques(body, timeStep);
		return;
	}
}

__forceinline void phJoint::PrecomputeAupdown()
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->PrecomputeAupdown();
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->PrecomputeAupdown();
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->PrecomputeAupdown();
		return;
	}
}

__forceinline void phJoint::PrecomputeAdownup(phArticulatedBody *body)
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<phJoint1Dof*>(this)->PrecomputeAdownup(body);
		return;
	case phJoint::JNT_3DOF:
		static_cast<phJoint3Dof*>(this)->PrecomputeAdownup(body);
		return;
		//case PRISM_JNT:
	default:
		static_cast<phPrismaticJoint*>(this)->PrecomputeAdownup(body);
		return;
	}
}

__forceinline void phJoint::TransformInertiaByASdownup( phArticulatedBodyInertia& dest ) const
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformInertiaByASdownup(dest);
		return;
	case phJoint::JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformInertiaByASdownup(dest);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformInertiaByASdownup(dest);
		return;
	}
}

__forceinline void phJoint::TransformInertiaByASupdown( phArticulatedBodyInertia& dest ) const
{
	switch (GetJointType())
	{
	case phJoint::JNT_1DOF:
		static_cast<const phJoint1Dof*>(this)->TransformInertiaByASupdown(dest);
		return;
	case phJoint::JNT_3DOF:
		static_cast<const phJoint3Dof*>(this)->TransformInertiaByASupdown(dest);
		return;
		//case PRISM_JNT:
	default:
		static_cast<const phPrismaticJoint*>(this)->TransformInertiaByASupdown(dest);
		return;
	}
}

} // namespace rage

#endif // PHARTICULATED_JOINTDISPATCH_TYPE_H
