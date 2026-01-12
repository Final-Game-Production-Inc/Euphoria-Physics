// 
// physics/colliderdispatch.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_COLLIDERDISPATCH_H 
#define PHYSICS_COLLIDERDISPATCH_H 

#include "physics\collider.h"
#include "pharticulated\articulatedcollider.h"

namespace rage {

#if __PS3
inline u32 phCollider::GetDmaPlanSize ()
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<phCollider*>(this)->GetDmaPlanSizeRigid();
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<phArticulatedCollider*>(this)->GetDmaPlanSizeArt();
	}
}
#endif

inline void phCollider::Update (Vec::V3Param128 timeStep, Vec::V3Param128 gravity)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->UpdateImp(timeStep, gravity);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->UpdateImp(timeStep, gravity);
		return;
	}
}

inline Vec3V_Out phCollider::GetImpulse (Vec::V3Param128 timeStep) const
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<const phCollider*>(this)->GetImpulseRigid(timeStep);
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return Vec3V(V_ZERO_WONE);
	}
}

inline void phCollider::SetVelocity (Vec::V3Param128 velocity)
{
	Assert(IsFiniteAll(RCC_VEC3V(velocity)));
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->SetVelocityImp(velocity);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->SetVelocityImp(velocity);
		return;
	}
}


inline void phCollider::SetAngVelocity (Vec::V3Param128 angVelocity)
{
	Assert(IsFiniteAll(RCC_VEC3V(angVelocity)));
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->SetAngVelocityImp(angVelocity);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->SetAngVelocityImp(angVelocity);
		return;
	}
}

inline void phCollider::UpdateVelocity (Vec::V3Param128 timeStep, bool saveVelocities)	
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->UpdateVelocityImp(timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->UpdateVelocityImp(timeStep, saveVelocities);
		return;
	}
}


inline void phCollider::UpdateVelocityFromExternal (Vec::V3Param128 timeStep)	
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->UpdateVelocityFromExternalImp(timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return;
	}
}


inline void phCollider::ApplyInternalForces (Vec::V3Param128 timeStep)	
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyInternalForces(timeStep);
		return;
	}
}


inline void phCollider::UpdateVelocityFromImpulse (Vec::V3Param128 timeStep)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->UpdateVelocityFromImpulseImp(timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->UpdateVelocityFromImpulseImp(timeStep);
		return;
	}
}


inline void phCollider::Move (Vec::V3Param128 timeStep, bool usePushes)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->MoveImp(timeStep, usePushes);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->MoveImp(timeStep, usePushes);
		return;
	}
}


inline void phCollider::UpdatePositionFromVelocity (Vec::V3Param128 timeStep)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->UpdatePositionFromVelocityImp(timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->UpdatePositionFromVelocityImp(timeStep);
		return;
	}
}

inline float phCollider::GetMass(int component) const
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<const phCollider*>(this)->GetMass();
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<const phArticulatedCollider*>(this)->GetMassArt(component);
#endif
	}
}

inline ScalarV phCollider::GetMassV(int component) const
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<const phCollider*>(this)->GetMassV();
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<const phArticulatedCollider*>(this)->GetMassVArt(component);
#endif
	}
}

inline void phCollider::GetInertiaMatrix (Mat33V_InOut outInertia, int component) const
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<const phCollider*>(this)->GetInertiaMatrixImp(outInertia, component);
		return;
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<const phArticulatedCollider*>(this)->GetInertiaMatrixImp(outInertia, component);
		return;
#endif
	}
}


inline void phCollider::ApplyGravity (Vec::V3Param128 gravity, Vec::V3Param128 timestep)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyGravityImp(gravity, timestep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyGravityImp(gravity, timestep);
		return;
	}
}


inline void phCollider::DampMotion (Vec::V3Param128 timeStep)
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->DampMotionImp(timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->DampMotionImp(timeStep);
		return;
	}
}


inline void phCollider::ApplyForce (Vec::V3Param128 force, Vec::V3Param128 position, Vec::V3Param128 timestep, int component)
{
	PDR_ONLY(debugPlayback::RecordApplyForce(*GetInstance(), force, position, component));
	Assert(IsFiniteAll(RCC_VEC3V(force)));
	Assert(IsFiniteAll(RCC_VEC3V(position)));

	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyForceImp(force, position, component);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyForceImp(force, position, component, timestep);
		return;
	}
}


inline void phCollider::ApplyTorque (Vec::V3Param128 torque, Vec::V3Param128 timestep)
{
	PDR_ONLY(debugPlayback::RecordApplyTorque(*GetInstance(), torque));
	Assert(IsFiniteAll(RCC_VEC3V(torque)));

	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyTorqueImp(torque);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyTorqueImp(torque, timestep);
		return;
	}
}


inline void phCollider::ApplyForceCenterOfMass (Vec::V3Param128 force, Vec::V3Param128 timestep)
{
	PDR_ONLY(debugPlayback::RecordApplyForceCG(*GetInstance(), force));
	Assert(IsFiniteAll(RCC_VEC3V(force)));

	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyForceCenterOfMassImp(force);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyForceCenterOfMassImp(force, timestep);
		return;
	}
}

#if !USE_NEW_SELF_COLLISION
inline void phCollider::SelfCollision(phManifold* manifold)
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->SelfCollision(manifold);
		return;
	}
}
#endif // !USE_NEW_SELF_COLLISION

inline void phCollider::SetColliderMatrixFromInstance ()
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->SetColliderMatrixFromInstanceRigid();
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->SetColliderMatrixFromInstanceArt();
		return;
	}
}

inline float phCollider::GetTotalInternalMotion() const
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<const phCollider*>(this)->GetTotalInternalMotionRigid();
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<const phArticulatedCollider*>(this)->GetTotalInternalMotionArt();
	}
}

inline void phCollider::GetInverseInertiaMatrix (Mat33V_InOut invInertia, int component) const
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer won't be correct
	switch (m_ColliderType)
	{
	case TYPE_RIGID_BODY:
		static_cast<const phCollider*>(this)->GetInverseInertiaMatrixRigid(invInertia);
		return;
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<const phArticulatedCollider*>(this)->GetInverseInertiaMatrixArt(invInertia, component);
		return;
#endif
	}
}

inline void phCollider::GetInvMassMatrix (Mat33V_InOut invMassMatrix, Vec::V3Param128 sourcePos, const Vec3V* responsePos,
								   int sourceComponent, int responseComponent) const
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer won't be correct
	switch (m_ColliderType)
	{
	case TYPE_RIGID_BODY:
		static_cast<const phArticulatedCollider*>(this)->GetInvMassMatrixRigid(invMassMatrix, sourcePos, responsePos);
		return;
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<const phArticulatedCollider*>(this)->GetInvMassMatrixArt(invMassMatrix, sourcePos, responsePos, sourceComponent, responseComponent);
		return;
#endif
	}
}

inline Vec3V_Out phCollider::GetLocalVelocity (Vec::V3Param128 position, int component) const
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<const phCollider*>(this)->GetLocalVelocityRigid(position);
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<const phArticulatedCollider*>(this)->GetLocalVelocityArt(position, component);
	}
}

inline void phCollider::ApplyAngAccel (Vec::V3Param128 angAccel, Vec::V3Param128 timeStep)
{
	Assert(IsFiniteAll(RCC_VEC3V(angAccel)));
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyAngAccelRigid(angAccel, timeStep);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyAngAccelArt(angAccel);
		return;
	}
}

inline void phCollider::ApplyImpulse (Vec::V3Param128 impulse, Vec::V3Param128 position, int component, float breakScale)
{
	Assert(IsFiniteAll(RCC_VEC3V(impulse)));
	Assert(IsFiniteAll(RCC_VEC3V(position)));
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyImpulseRigid(impulse, position);
		return;
#if PHCONTACT_SUPPORT_ARTICULATED
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyImpulseArt(impulse, position, component, breakScale);
		return;
	}
#endif
}


inline void phCollider::ApplyImpulseCenterOfMass (Vec::V3Param128 impulse)
{
	Assert(IsFiniteAll(RCC_VEC3V(impulse)));
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyImpulseCenterOfMassRigid(impulse);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyImpulseCenterOfMassArt(impulse);
		return;
	}
}


inline void phCollider::ApplyAngImpulse (Vec::V3Param128 angImpulse, int component)
{
	Assert(IsFiniteAll(RCC_VEC3V(angImpulse)));
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyAngImpulseRigid(angImpulse);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyAngImpulseArt(angImpulse, component);
		return;
	}
}


inline void phCollider::ApplyJointAngImpulse (Vec::V3Param128 angImpulse, int jointIndex)
{
	Assert(IsFiniteAll(RCC_VEC3V(angImpulse)));
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->ApplyJointAngImpulseRigid(angImpulse);
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->ApplyJointAngImpulseArt(angImpulse, jointIndex);
		return;
	}
}


inline void phCollider::RevertImpulses()
{
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		static_cast<phCollider*>(this)->RevertImpulsesRigid();
		return;
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		static_cast<phArticulatedCollider*>(this)->RestoreVelocities();
		return;
	}
}


inline bool phCollider::Rejuvenate ()
{
	// SPU needs to dispatch based on m_ColliderType, since virtual pointer isn't fixed up.
	switch (m_ColliderType)
	{
	case phCollider::TYPE_RIGID_BODY:
		return static_cast<phCollider*>(this)->RejuvenateImp();
	default:
		// 	case phCollider::TYPE_ARTICULATED_BODY:
		// 	case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
		return static_cast<phArticulatedCollider*>(this)->RejuvenateImp();
	}
}

inline bool phCollider::IncrementAndCheckRejuvenation()
{
	return ++m_RejuvenateCount > NUM_REJUVENATE_UPDATES;
}


} // namespace rage

#endif // PHYSICS_COLLIDERDISPATCH_H 
