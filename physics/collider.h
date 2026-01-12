//
// physics/collider.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//



#ifndef PHYSICS_COLLIDER_H
#define PHYSICS_COLLIDER_H

#include "diag/debuglog.h"
#include "phbullet/DiscreteCollisionDetectorInterface.h"		// For TRACK_COLLISION_TIME.  That probably should be #defined here, not there.
#include "phcore/constants.h"
#include "grprofile/drawcore.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "paging/ref.h"
#include "physics/debugevents.h"

#if __PS3
#include "system/dmaplan.h"
#endif

// PURPOSE: Determine whether or not to test matrix rejuvenation. This is only used when testing to see how often rejuvenation is necessary.
#define REJUVENATE_TEST			0

// PURPOSE: Define the fractional error allowed on the matrix axes for orthonormality, used only when REJUVENATE_TEST is turned on.
#define REJUVENATE_ERROR		0.01f
#define REJUVENATE_ERROR_NEW_VEC 0.02f	// Mat34V orthonormal functions need twice the error to return the same results

// PURPOSE: Define the number of collider updates before rejuvenating the matrix.
#define	NUM_REJUVENATE_UPDATES	32

#define ASSERT_LARGE_PUSH_AND_POSITION_CHANGES 1

#ifndef PHCONTACT_SUPPORT_ARTICULATED
#define PHCONTACT_SUPPORT_ARTICULATED 1
#endif

// This is currently duplicated in bound.h and archetype.h - it should really be in a central location.
#if __SPU
#define PH_NON_SPU_VIRTUAL 
#define PH_NON_SPU_PURE_VIRTUAL
#define PH_NON_SPU_VIRTUAL_ONLY(X) X
#else
#define PH_NON_SPU_VIRTUAL virtual
#define PH_NON_SPU_PURE_VIRTUAL =0
#define PH_NON_SPU_VIRTUAL_ONLY(X)
#endif


namespace rage {

class phBound;
class phInst;
class phManifold;
class phSleep;

//=============================================================================
// phCollider
//
// PURPOSE:
//   Controller for an active physical object.  It contains pointers to a 
//   physics instance (phInst) for the object's position and orientation, 
//   parameters for the object's motion, and a phSleep for controlling whether
//   the object should remain active.
//

class phCollider
{
public:
	enum Type
	{
		TYPE_RIGID_BODY,
		TYPE_ARTICULATED_BODY,
		TYPE_ARTICULATED_LARGE_ROOT
	};

	phCollider ();
	phCollider (class datResource &rsc);
	PH_NON_SPU_VIRTUAL ~phCollider ();

	// PURPOSE: Initialize the collider with a physics instance.
	// PARAMS:
	//	instance -	the physics instance for the collider
	//	sleep -		optional sleep for the collider
	// NOTES:
	//	If no phSleep is provided, the collider will always remain active.
	PH_NON_SPU_VIRTUAL void Init (phInst* instance, phSleep* sleep=NULL);

	// PURPOSE: Initialize this collider with the given instance.
	// PARAMS:
	//	instance -	the physics instance for the collider
	void SetInstanceAndReset (phInst* instance);

	// PURPOSE: Initialize the mass and inertia with physical parameters from the instance's archetype.
	void InitInertia ();

	// PURPOSE: Set the sleep for this collider.
	// PARAMS:
	//	sleep -	the phSleep for the collider
	void SetSleep (phSleep* sleep);

	PH_NON_SPU_VIRTUAL void SetType (int type);

	// PURPOSE: Get the collider's velocity and angular velocity from its instance.
	void GetMotionFromInstance ();

	// PURPOSE: Set the collider's mass and angular inertia their inverses.
	// PARAMS:
	//	mass -			the collider's mass
	//	angInertia -	the collider's new angular inertia along its coordinate axes
	// NOTES:
	// 1.	The collider's coordinate axes are always its principal axes for angular inertia, which is why angular
	//		inertia is a vector instead of a matrix.
	// 2.	Negative mass or angular inertia are not allowed (zero is allowed).
	void SetInertia (Vec::V3Param128 vMass, Vec::V3Param128 vAngInertia);

	// PURPOSE: Return the collider, its instance and its sleep to their original states.
	PH_NON_SPU_VIRTUAL void Reset ();

	// PURPOSE:	Stop all motion by setting all the updated parameters to zero.
	PH_NON_SPU_VIRTUAL void Freeze ();

	// PURPOSE: Reset all the applied forces, impulses and pushes.  This is called after every update.
	PH_NON_SPU_VIRTUAL void ZeroForces ();

	// PURPOSE: Change the collider's velocity, angular velocity, position and orientation from forces and impulses.
	// PARAMS:
	//	timeStep -	the time interval over which to update the collider
	// NOTES:	The time step argument is passed down from the physics simulator update to provide simple control of the simulated time step. 
	inline void Update (Vec::V3Param128 timeStep, Vec::V3Param128 gravity);
	void UpdateImp (Vec::V3Param128 timeStep, Vec::V3Param128 gravity);

#if !USE_NEW_SELF_COLLISION
    inline void SelfCollision(phManifold* manifold);
#endif // !USE_NEW_SELF_COLLISION

	// PURPOSE: Update the position and orientation from the velocity and angular velocity.
	// PARAMS:
	//	timeStep -	the time interval over which to update the collider's position and orientation
	inline void UpdatePositionFromVelocity (Vec::V3Param128 timeStep);
	void UpdatePositionFromVelocityImp (Vec::V3Param128 timeStep);
		
	void UpdatePositionFromVelocityIncludingForce (Vec::V3Param128 timeStep);

	// PURPOSE: Update the collider's sleep.
	// PARAMS:
	//	timeStep -	the time interval over which to update the collider's sleep
	void UpdateSleep (float timeStep);

#if __ASSERT
	// PURPOSE: Check if the instance or collider has been moved since the last call to SetInstanceMatrixFromCollider
	//            or SetColliderMatrixFromInstance
	// RETURN: true if the matrices are lined up
	void ValidateInstanceMatrixAlignedWithCollider(ScalarV_In tolerance = ScalarV(V_FLT_SMALL_2)) const;
#endif // __ASSERT

	void ComputeInstanceMatrixFromCollider(Mat34V_InOut instanceMatrix) const;

	// PURPOSE: Set the matrix of the collider's instance from the collider's matrix and the bound's centroid offset.
	void SetInstanceMatrixFromCollider ();

	// PURPOSE: Use the instance matrix and the bound's center of gravity offset to set the collider matrix.
	// NOTES:
	//	1.	The collider is always located at the center of gravity.
	//	2.	If the bound does not have a center of gravity offset, then the center of gravity is the instance location.
	inline void SetColliderMatrixFromInstance ();
	void SetColliderMatrixFromInstanceRigid ();

	// PURPOSE: Explicitly set m_LastVelocity and m_LastAngVelocity to m_Velocity and m_AngVelocity.
	// NOTES: After calling this function, calls to RevertImpulses will set the velocities back to what they were
	//        when this function was called.
	inline void UpdateLastVelocities();

	// PURPOSE: Update the velocity from the force and impulse, and the angular velocity from the torque and angular impulse.
	// PARAMS:
	//	timeStep	- the time interval covered by this update
	inline void UpdateVelocity (Vec::V3Param128 timeStep, bool saveVelocities = false);
	void UpdateVelocityImp (Vec::V3Param128 timeStep);

	inline void UpdateVelocityFromExternal (Vec::V3Param128 timeStep);
	inline void ApplyInternalForces (Vec::V3Param128 timeStep);
	void UpdateVelocityFromExternalImp (Vec::V3Param128 timeStep);

	// PURPOSE: Update the velocity from the impulse, and the angular velocity from the angular impulse.
	// PARAMS:
	//	timeStep -	the time interval over which to update the collider's sleep
	inline void UpdateVelocityFromImpulse (Vec::V3Param128 timeStep);
	void UpdateVelocityFromImpulseImp (Vec::V3Param128 timeStep);

	// PURPOSE: Change the collider's position and orientation from its push and turn.
	void UpdateFromPushAndTurn ();

#if TRACK_PUSH_COLLIDERS
	// PURPOSE: phSimulator only function to track which colliders are in push pairs and need post-push processing
	void SetIsInPushPair(bool isInPushPair) { m_IsInPushPair = isInPushPair; }
	bool GetIsInPushPair() const { return m_IsInPushPair; }

	// Purpose: This gets called during phSimulator::ApplyPushes on all active colliders that didn't receive pushes
	void UpdateApplyZeroPushes();
#endif // TRACK_PUSH_COLLIDERS

	// PURPOSE: Update the collider's last instance matrix from the current collider matrix
	void UpdateLastMatrixFromCurrent (bool updateLastSafeMatrixToo);

	// PURPOSE: Update the collider's position and orientation form pushes and turns (not from velocities).
	// NOTES:
	//	1.	This calls UpdateFromPushAndTurn, updates the instance matrix and informs the physics level that the collider moved.
	//	2.	This is derived in constrained colliders to constrain the position and orientation.
	inline void Move (Vec::V3Param128 timeStep, bool usePushes);
	void MoveImp (Vec::V3Param128 timeStep, bool usePushes);

	// PURPOSE: Rotate the 3x3 orientation part of the orientation and position matrix.
	// PARAMS:
	//	rotation - the rotation axis (vector direction) and angle (vector magnitude)
	// NOTES:	If a rotation is done, the angular velocity is recalculated from the angular momentum (which
	//			does not change with the rotation) and the new orientation.
	void RotateCollider (Vec::V3Param128 rotation);

	// PURPOSE:	Clamp the linear velocity.
	// PARAMS: velocity - the velocity to clamp
	// RETURN: The resulting clamped velocity
	// NOTES:	If the velocity exceeds the upper limit, then it is clamped and the momentum is recalculated.
	Vec3V_Out ClampVelocity (Vec::V3Param128 velocity);

	// PURPOSE:	Clamp the linear velocity.
	//	NOTES:	If the velocity exceeds the upper limit, then it is clamped and the momentum is recalculated.
	void ClampVelocity ();

	// PURPOSE: Renormalize and reorthogonalize the collider's orientation matrix to avoid drift.
	// NOTES:	This will renormalize and reorthogonalize the orientation matrix at random intervals between NUM_REJUVENATE_UPDATES and
	//			NUM_REJUVENATE_UPDATES/2 update cycles. When REJUVENATE_TEST is true, updates will only occur when an error is larger
	//			than REJUVENATE_ERROR.
	inline bool Rejuvenate ();

	// PURPOSE: Increments the rejuvenate count and checks if it above NUM_REJUVENATE_UPDATES
	// RETURN: TRUE if above limit
	inline bool IncrementAndCheckRejuvenation();
	void ResetRejuvenateCounter() { m_RejuvenateCount = 0; }

	// PURPOSE:	Clamp the angular velocity.
	// PARAMS: angVelocity - the angular velocity to clamp
	// RETURN: The resulting clamped angular velocity
	// NOTES:	If the angular velocity exceeds the upper limit, then it is clamped and the angular momentum is recalculated.
	Vec3V_Out ClampAngularVelocity (Vec::V3Param128 angVelocity);

	// PURPOSE:	Clamp the angular velocity.
	// NOTES:	If the angular velocity exceeds the upper limit, then it is clamped and the angular momentum is recalculated.
	void ClampAngularVelocity ();

#if __PFDRAW
	// PURPOSE: Draw the collider's bound.
	PH_NON_SPU_VIRTUAL void ProfileDraw () const;
#endif

	// PURPOSE: Get a pointer to the collider's instance.
	// NOTES:	For colliders that are in use, this should never be NULL. It should only be NULL for colliders in an unused pool.
	phInst* GetInstance () const;

	// PURPOSE: GetType returns 1 for articulated colliders, 0 otherwise
	// NOTES: This is so the SPU can tell which colliders are articulated
	int GetType() const;

	// PURPOSE:	Mark this collider as penetrating another object.
	// NOTES:	This stops the collider's safe last instance matrix from updating.
	void SetCurrentlyPenetrating ();

    // PURPOSE:	Get the number of frames this collider has been penetrating since it was last not penetrating
    // NOTES: the number of frames this collider has been penetrating since it was last not penetrating	
    u16 GetCurrentlyPenetratingCount () const;

	EARLY_FORCE_SOLVE_ONLY(bool GetNeedsCollision () const;)

	// PURPOSE:	Clear this collider's non-penetrating flag, in case it's been placed intersecting the ground.
	// NOTES:	This is done when teleporting so that the safe last matrix is updated until the collider reaches a safe position.
	void ClearNonPenetratingAfterTeleport();

	// PURPOSE:	Prevent this object from triggering a push collision on its island, regardless of the magnitude of pushes and turns received.
	void DisablePushCollisions();

	// PURPOSE:	Allow this object to trigger push collisions on its island.
	void EnablePushCollisions();

	void PreventPushCollisions();

	void AllowPushCollisions();

	bool GetPreventsPushCollisions();

	// PURPOSE: Get the sleep pointer of this collider.
	// RETURN:	a pointer to the phSleep used by this collider
	phSleep* GetSleep () const;

	// PURPOSE: Get the collider's mass in kilograms.
	// RETURN:	the collider's mass in kilograms
	float GetMass () const;
	ScalarV_Out GetMassV () const;

	// PURPOSE: Get the mass of the collider if rigid, or the link of the given component if articulated
	// PARAMS:
	//		component - component index, used only if the collider is articulated
	// RETURN: The mass in kilgrams of the collider if it's rigid or the mass of the component's link if it's articulated
	float GetMass(int component) const;
	ScalarV GetMassV(int component) const;

	// PURPOSE: Get the collider's inverse mass.
	// RETURN:	the collider's inverse mass
	float GetInvMass () const;
	ScalarV_Out GetInvMassV () const;

	// PURPOSE: Get the collider's velocity in meters/second.
	// RETURN:	the collider's velocity in meters/second
	Vec3V_ConstRef GetVelocity () const;

	// PURPOSE: Get the collider's angular velocity in radians/second.
	// RETURN:	the collider's angular velocity in radians/second
	Vec3V_ConstRef GetAngVelocity () const;

	// PURPOSE: Get the collider's velocity after the last integration
	// RETURN: the collider's velocity after the last integration
	Vec3V_ConstRef GetLastVelocity() const;

	// PURPOSE: Get the collider's angular velocity after the last integration
	// RETURN: the collider's angular velocity after the last integration
	Vec3V_ConstRef GetLastAngVelocity() const;

	// PURPOSE: Get the collider's velocity before forces were applied
	Vec3V_ConstRef GetVelocityBeforeForce() const;

	// PURPOSE: Get the collider's angular velocity before forces were applied
	Vec3V_ConstRef GetAngVelocityBeforeForce() const;

	// PURPOSE: Calculate the momentum of the collider
	// RETURN: the momentum of the collider
	Vec3V_Out CalculateMomentum () const;

	// PURPOSE: Calculate the angular momentum of the collider
	// RETURN: the angular momentum of the collider
	// NOTES: this function requires a matrix multiplication
	Vec3V_Out CalculateAngMomentum () const;

	// PURPOSE: Calculate the angular momentum of this collider
	// RETURN: the angular momentum of the collider
	// NOTES: This function will always treat the collider like phCollider, regardless of type
	Vec3V_Out CalculateAngMomentumRigid () const;

	// PURPOSE: Get the collider's angular inertia vector.
	// RETURN:	the collider's angular inertia vector
	// NOTES:
	//	The angular inertia is a vector because the principal axes are assumed to be aligned along the collider's coordinate matrix axes.
	//	Use GetInertiaMatrix() to get the angular inertia matrix in world coordinates.
	Vec3V_ConstRef GetAngInertia () const;

	// PURPOSE: Get the collider's inverse angular inertia vector.
	// RETURN:	the collider's inverse angular inertia vector
	// NOTES:
	//	The inverse angular inertia is a vector because the principal axes are assumed to be aligned along the collider's coordinate matrix axes.
	//	Use GetInverseInertiaMatrix() to get the inverse angular inertia matrix in world coordinates.
	Vec3V_ConstRef GetInvAngInertia () const;

	// PURPOSE: Get the collider's accumulated, unused push.
	// RETURN:	the push that has been applied to the collider but not yet used to change its position
	Vec3V_ConstRef GetPush () const;

	// PURPOSE: Set the collider's push
	void SetPush(Vec::V3Param128 push);

	// PURPOSE: Get the collider's applied push.
	// RETURN:	the push that has been applied to the collider since the last frame and used to change its position
	Vec3V_ConstRef GetAppliedPush () const;

	// PURPOSE: Get the collider's accumulated, unused turn.
	// RETURN:	the turn that has been applied to the collider but not yet used to change its orientation
	Vec3V_ConstRef GetTurn () const;

	// PURPOSE: Set the collider's turn
	void SetTurn(Vec::V3Param128 turn);

	// PURPOSE: Get the collider's applied turn.
	// RETURN:	the turn that has been applied to the collider since the last frame and used to change its orientation
	Vec3V_ConstRef GetAppliedTurn () const;

	// PURPOSE:	Get the internal motion of this collider.
	// RETURN:	some approximation of the total amount of internal motion of the collider
	// NOTES:	This always return zero for base class colliders because they are rigid. It is derived by articulated colliders to estimate internal motion.
	inline float GetTotalInternalMotion () const;
	float GetTotalInternalMotionRigid () const;

	// PURPOSE:	Compute the rotational inertia matrix in world coordinates about the center of mass. 
	// PARAMS:
	//	outInertia - reference into which to write the rotational inertia matrix in world coordinates
	//	component - optional composite bound part index number, used by articulated colliders
	inline void GetInertiaMatrix (Mat33V_InOut outInertia, int component=0) const;
	void GetInertiaMatrixImp (Mat33V_InOut outInertia, int component=0) const;

	// PURPOSE: Compute the rotational inertia matrix in world coordinates about the a position offset from the center of mass. 
	// PARAMS:
	//	offset - position relative to the center of mass about which to get the inertia matrix
	//	outInertia - reference into which to write the rotational inertia matrix in world coordinates
	void GetInertiaMatrix (Mat33V_InOut outInertia, Vec::V3Param128 offset) const;

	// PURPOSE: Get the inverse of the collider's inertia matrix in world coordinates.
	// PARAMS:
	//	outInvInertia - reference into which to write the inverse inertia matrix
	//	component - optional composite bound part index (only used by articulated colliders)
	// NOTES:
	//	1.	An angular impuse transformed by the inverse inertia matrix is the change in angular velocity.
	//	2.	When the collider's matrix is aligned with the world coordinate directions, the inverse inertia matrix is diagonal.
	inline void GetInverseInertiaMatrix (Mat33V_InOut outInvInertia, int component=0) const;
	void GetInverseInertiaMatrixRigid (Mat33V_InOut outInvInertia) const;

	// PURPOSE: Compute the inertia of this collider rotating about the given axis.
	// PARAMS:
	//	worldAxis - the axis of rotation in world coordinates
	// RETURN:	the inertia for rotations about the given axis
	Vec3V_Out ComputeInertiaAboutAxis (Vec::V3Param128 worldAxis) const;

	// PURPOSE: Compute the inverse inertia of this collider rotating about the given axis.
	// PARAMS:
	//	worldAxis - the axis of rotation in world coordinates
	// RETURN:	the inverse inertia for rotations about the given axis
	Vec3V_Out ComputeInverseInertiaAboutAxis (Vec::V3Param128 worldAxis) const;

	// PURPOSE: Get the matrix that transforms an impulse into the resulting response velocity.
	// PARAMS:
	//	invMassMatrix - reference in which to write the inverse mass matrix
	//	sourcePos - the world position of the impulse application point
	//	responsePos - optional pointer to the world position of the velocity response (default is to use the source position)
	//	sourceComponent - optional composite bound part number at the impulse position, for use by articulated colliders
	//	responseComponent - optional composite bound part number at the velocity response position, for use by articulated colliders
	//	sourceNormal - optional unit direction of the impulse
	// NOTES:	The inverse mass matrix is defined to satisfy RM = [(r X R) I ] X q  with impulse application point relative position r,
	//			response point relative position q, and world inertia matrix I.
	inline void GetInvMassMatrix (Mat33V_InOut invMassMatrix, Vec::V3Param128 sourcePos, const Vec3V* responsePos=NULL, int sourceComponent=0, int responseComponent=0) const;
	void GetInvMassMatrixRigid (Mat33V_InOut invMassMatrix, Vec::V3Param128 sourcePos, const Vec3V* responsePos=NULL) const;

	// PURPOSE: Get the translational part of the inverse mass matrix.
	// PARAMS:
	//	invMassMatrixTranslation - reference into which to write the translational part of the inverse mass matrix
	//	sourceNormal - optional unit direction of the impulse
	void GetInvMassMatrixTranslation (Mat33V_InOut outMtx, Vec::V3Param128 sourceNormal=Vec3V(V_ZERO).GetIntrin128()) const;

	// PURPOSE: Compute the angular velocity reponse from an impulse at the given position.
	// PARAMS:
	//	outMtx - reference in which to write the inverse torque mass matrix
	//	sourcePos - the position of either velocity response or of the applied impulse 
	// NOTES:	This returns (I^{1-}(px) where (px) is the crossproduct matrix
	void GetInvTorqueMassMatrix (Mat33V_InOut outMtx, Vec::V3Param128 sourcePos) const;

	// PURPOSE: Set the maximum speed of this collider.
	// PARAMS:
	//	maxSpeed - the maximum speed of this collider
	// NOTES:	All colliders have an enforced maximum speed. The default is DEFAULT_MAX_SPEED.
	void SetMaxSpeed (float maxSpeed);

	// PURPOSE: Set the maximum angular speed of this collider.
	// PARAMS:
	//	maxAngSpeed - the maximum angular speed of this collider
	// NOTES:	All colliders have an enforced maximum angular speed. The default is DEFAULT_MAX_ANG_SPEED.
	void SetMaxAngSpeed (float maxAngSpeed);

	// PURPOSE: Get the maximum speed of this collider.
	// RETURN:	the maximum speed of this collider
	// NOTES:	All colliders have an enforced maximum speed. The default is DEFAULT_MAX_SPEED.
	float GetMaxSpeed () const;

	// PURPOSE: Get the maximum angular speed of this collider.
	// RETURN:	the maximum angular speed of this collider
	// NOTES:	All colliders have an enforced maximum angular speed. The default is DEFAULT_MAX_ANG_SPEED.
	float GetMaxAngSpeed () const;

	// PURPOSE: Set the gravity factor of this collider.
	// PARAMS:
	//	gravityFactor - the new gravity factor of this collider
	// NOTES:	This will be initialized from the archetype when SetInstanceAndReset() is called.
	void SetGravityFactor(float gravityFactor);

	// PURPOSE: Get the gravity factor of this collider.
	// RETURN:	the gravity factor of this collider
	// NOTES:	This will be initialized from the archetype when SetInstanceAndReset() is called.
	const float &GetGravityFactor() const;

	// PURPOSE: Get the orientation and position matrix of this collider in world coordinates.
	// RETURN:	a const reference to the orthonormal orientation and position matrix of this collider
	Mat34V_ConstRef GetMatrix () const;

	// PURPOSE: Set the orientation matrix of this collider.
	// PARAMS:
	//	matrix - the orthonormal orientation and position matrix of this collider
	// NOTES:	This does not set the collider's instance matrix. That will be set from the new collider matrix during the next update,
	//			but it would be safest to call SetInstanceMatrixFromCollider() after calling this.
	void SetMatrix (Mat34V_In matrix);

	// PURPOSE: Get the world position of this collider.
	// RETURN:	the position of this collider in world coordinates
	Vec3V_ConstRef GetPosition () const;

	// PURPOSE:	Set the world position of this collider.
	// PARAMS:
	//	position - the new position of this collider in world coordinates
	void SetPosition (Vec::V3Param128 position);

	// PURPOSE: Get this collider's instance matrix on the previous frame.
	// RETURN:	a reference to this collider's instance matrix from the previous frame
	Mat34V_ConstRef GetLastInstanceMatrix () const;

	// PURPOSE: Set this collider's instance matrix on the previous frame.
	// PARAMS:
	//	last - this collider's instance matrix on the previous frame
	// NOTES:	The last instance matrix is stored by the collider and used for collision detection. This is called externally only
	//			when initializing colliders in motion from moving instances.
	void SetLastInstanceMatrix (Mat34V_In last);

	Mat34V_ConstRef GetLastSafeInstanceMatrix () const;

	// PURPOSE:	Get the velocity of this object at the given world position. 
	// PARAMS:
	//	position -	the world position at which to get the velocity
	//	velocity -	the velocity of the object at the given position
	//	component - optional composite bound part number, used by articulated colliders
	inline Vec3V_Out GetLocalVelocity (Vec::V3Param128 position, int component=0) const;
	Vec3V_Out GetLocalVelocityRigid (Vec::V3Param128 position) const;

	// PURPOSE: Get the acceleration in world coordinates at the given position.
	// PARAMS:
	//	position - the world position at which to get the acceleration of the collider
	//	acceleration - reference in which to write the acceleration at the given position
	Vec3V_Out GetLocalAcceleration (Vec::V3Param128 position) const;

	// PURPOSE: Apply the force of gravity to the collider.
	// NOTES:	This is derived by constrained colliders to modify the effective center of mass.
	inline void ApplyGravity (Vec::V3Param128 gravity, Vec::V3Param128 timestep);
	void ApplyGravityImp (Vec::V3Param128 gravity, Vec::V3Param128 timeStep);

	// PURPOSE: Damp the collider's motion.
	// PARAMS:
	//	timeStep - the time interval over which to apply damping forces
	inline void DampMotion (Vec::V3Param128 timeStep);
	void DampMotionImp (Vec::V3Param128 timeStep);

	// PURPOSE: Apply the force of air resistance to the collider.
	// PARAMS:
	//	airDensity - optional density of air in mks units (kg/m^3)
	//	windVelocity - optional velocity of the wind
	//	bound - optional bound to use in place of the collider's bound for finding air resistance
	// NOTES:	Only polyhedral bounds are used in this unfinished method.
	void ApplyAirResistance (Vec::V3Param128 timestep, float airDensity=1.0f, Vec::V3Param128 windVelocity=Vec3V(V_ZERO).GetIntrin128(), const phBound* bound=NULL);

	// PURPOSE: Apply a push. 
	// PARAMS:
	//	push - the push to apply to the collider
	//	position - optional position at which to apply the push
	//	component - optional composite bound part index number on which to apply the push
	// NOTES:	The optional arguments are used only by articulated colliders.
	void ApplyPush (Vec::V3Param128 push, Vec::V3Param128 position=ORIGIN);

	// PURPOSE: Apply a turn. 
	// PARAMS:
	//	turn - the turn in radians to apply to the collider
	void ApplyTurn (Vec::V3Param128 turn);

	// PURPOSE: Apply the given push and apply the necessary turn to result in minimal displacement of the collider at the given
	//			world rotation center.
	// PARAMS:
	//	push - the push to apply
	//	worldPosition - the world position from which to drag the collider by the given push
	//	worldRotationCenter - the world position at which to get minimal displacement
	// NOTES:	This is used for pushing an object from one position while the object pivots about some other position, such as
	//			a joint pushing a trailer while the trailer rotates about its rear wheels. Applying simple pushes in this case can
	//			cause the rear wheels to slide sideways.
	void ApplyPushAndTurn (Vec::V3Param128 push, Vec::V3Param128 worldPosition, Vec::V3Param128 worldRotationCenter);

	// PURPOSE:	Apply an angular acceleration. 
	// PARAMS:
	//	angular acceleration -	the angular acceleration to apply to the collider
	inline void ApplyAngAccel (Vec::V3Param128 angAccel, Vec::V3Param128 timeStep);
	void ApplyAngAccelRigid (Vec::V3Param128 angAccel, Vec::V3Param128 timeStep);

	// PURPOSE: Apply an acceleration. 
	// PARAMS:
	//	acceleration -	the acceleration to apply to the collider
	void ApplyAccel (Vec::V3Param128 acceleration, Vec::V3Param128 timestep);

	// PURPOSE: Apply a force to the collider to cause the given acceleration along the specified world axis.
	// PARAMS:
	//	accel - the acceleration for the collider along the specified world axis
	//	axisIndex - the index number of the axis in world coordinates along which to accelerate the collider (0==X, 1==Y, 2==Z)
	void ApplyAccelOnAxis (float accel, int axisIndex, Vec::V3Param128 timeStep);

	// PURPOSE: Apply a force at a position. 
	// PARAMS:
	//	force - the force to apply to the collider
	//	position - the world position at which to apply the force
	//	component - optional composite bound part number
	inline void ApplyForce (Vec::V3Param128 force, Vec::V3Param128 position, Vec::V3Param128 timestep, int component=0);
	void ApplyForceImp (Vec::V3Param128 force, Vec::V3Param128 position, int component);

	// PURPOSE: Apply a force at the center of mass, and in debug draw mode, draw the force at the given position. 
	// PARAMS
	//	force - the force to apply to the collider
	inline void ApplyForceCenterOfMass (Vec::V3Param128 force, Vec::V3Param128 timestep);
	void ApplyForceCenterOfMassImp (Vec::V3Param128 force);

	// PURPOSE: Apply a torque. 
	// PARAMS:
	//	torque - the torque to apply to the collider
	inline void ApplyTorque (Vec::V3Param128 torque, Vec::V3Param128 timestep);
	void ApplyTorqueImp (Vec::V3Param128 torque);

	// PURPOSE: Apply the given impulse to the collider.
	// PARAMS:
	//	impulse	- the impulse to apply
	//	position - the position in world space at which to apply the force
	//	component - optional composite bound part number
	//	push - optional push to apply
	inline void ApplyImpulse (Vec::V3Param128 impulse, Vec::V3Param128 position, int component=0, float breakScale=1.0f);
	void ApplyImpulseRigid (Vec::V3Param128 impulse, Vec::V3Param128 position);

	// PURPOSE: Apply an impulse with no accompanying angular impulse. 
	// PARAMS:
	//	impulse -	the impulse to apply to the collider
	inline void ApplyImpulseCenterOfMass (Vec::V3Param128 impulse);
	void ApplyImpulseCenterOfMassRigid (Vec::V3Param128 impulse);

	// PURPOSE: Apply an angular impulse. 
	// PARAMS:
	//	angular impulse - the angular impulse to apply to the collider
	//	component - optional composite bound part index of the impulse application, for articulated colliders
	inline void ApplyAngImpulse (Vec::V3Param128 angImpulse, int component=0);
	void ApplyAngImpulseRigid (Vec::V3Param128 angImpulse);

	// PURPOSE: Apply the given angular impulse to the specified joint.
	// PARAMS:
	//	angImpulse - the angular impulse to apply to this joint
	//	jointIndex - the index of the joint
	// NOTES:
	//	Intended for articulated colliders.  The phCollider version simply applies
	//	the impulse to the collider's center.
	// SEE ALSO: phJoint::ApplyAngImpulse()
	inline void ApplyJointAngImpulse (Vec::V3Param128 angImpulse, int jointIndex=0);
	void ApplyJointAngImpulseRigid (Vec::V3Param128 angImpulse);
	
	void ApplyImpetus ( Vec3V_In positionA, Vec3V_In impulseA, const int iElementA, const int iComponentA, const bool isForce, Vec::V3Param128 timestep, float breakScale=1.0f);

  #if HACK_GTA4_FRAG_BREAKANDDAMAGE
	void ApplyImpetusAndDamageImpetus (const bool isForce, const int component, Vec3V_In impulse, Vec3V_In position, Vec3V_In damageImpetus, Vec::V3Param128 timestep);
  #endif

	// PURPOSE:	Apply the given torque and apply the necessary force to result in rotation about the given world position.
	// PARAMS:
	//	torque - the torque to apply
	//	worldPosition	- the world position about which to make the given torque rotate this object
	void ApplyTorqueAndForce (Vec::V3Param128 torque, Vec::V3Param128 worldPosition, Vec::V3Param128 timestep);

	// PURPOSE:	Apply the given impulse to the collider, and use the impulse to change the collider's velocity and angular velocity.
	// PARAMS
	//	impulse - the impulse to apply
	//	position - the position in world space at which to apply the impulse
	//	component - optional composite bound part number
	// NOTES:	Normally, impulses are accumulated until the collider's velocity and angular velocity are changed during an update.
	//			This method is called during breaking to compute breaking object collision reactions without waiting until the next frame.
	void ApplyImpulseChangeMotion (Vec::V3Param128 impulse, Vec::V3Param128 worldPosition, int component=0);

	// PURPOSE: Get the force applied to the collider, including the effective force from the impulse.
	// PARAMS:
	//	invTimeStep	- the inverse time interval, for converting the impulse to a force
	// RETURNS:
	//	the force on this object
	Vec3V_Out GetForce (Vec::V3Param128 invTimeStep) const;

	// PURPOSE: Get the torque applied to the collider, including the effective torque from the angular impulse.
	//	PARAMS:
	//	invTimeStep	- the inverse time interval, for converting the angular impulse to a torque
	// RETURNS:
	//	the torque on this object
	Vec3V_Out GetTorque (Vec::V3Param128 invTimeStep) const;

	// PURPOSE: Get the impulse applied to the collider, including the effective impulse from the force.
	// PARAMS:
	//	timeStep	- the time interval, for converting the force to an impulse
	// RETURNS:
	//	the impulse on this object
	inline Vec3V_Out GetImpulse (Vec::V3Param128 invTimeStep) const;
	Vec3V_Out GetImpulseRigid (Vec::V3Param128 invTimeStep) const;

	// PURPOSE: Get the angular impulse applied to the collider, including the effective angular impulse from the torque.
	// PARAMS:
	//	timeStep	- the time interval, for converting the torque to an angular impulse
	//	angImpulse	- a reference used for the angular impulse on this object, filled in by this method
	Vec3V_Out GetAngImpulse (Vec::V3Param128 invTimeStep) const;

	// PURPOSE: Set the velocity of the collider, and calculate the momentum from the new velocity.
	// PARAMS: velocity	- the new velocity
	inline void SetVelocity (Vec::V3Param128 velocity);
	void SetVelocityImp (Vec::V3Param128 velocity);

	// PURPOSE: Set the velocity of the collider, without recomputing the momentum.
	// PARAMS: velocity	- the new velocity
	void SetVelocityOnly (Vec::V3Param128 velocity);

	// PURPOSE: Set the object's local "zero" point velocity, which damping will approach
	// PARAMS: velocity	- the reference frame's local point velocity
	void SetReferenceFrameVelocity (Vec::V3Param128 velocity);
	
	// PURPOSE: Set the object's lcoal "zero" angular velocity, which damping will approach
	// PARAMS: angVelocity	- the reference frame's local angular velocity
	void SetReferenceFrameAngularVelocity (Vec::V3Param128 angVelocity);

	// PURPOSE: Get the object's local "zero" point velocity, which damping will approach
	// RETURN:	the collider's reference frame's local point velocity
	Vec3V_ConstRef GetReferenceFrameVelocity () const;
	void SetReferenceFrameVelocityDampingRate (float rate);
	ScalarV_Out GetReferenceFrameVelocityDampingRate () const;
	
	// PURPOSE: Get the object's local "zero" angular velocity, which damping will approach
	// RETURN:	the collider's reference frame's local angular velocity
	Vec3V_ConstRef GetReferenceFrameAngularVelocity () const;
	void SetReferenceFrameAngularVelocityDampingRate (float rate);
	ScalarV_Out GetReferenceFrameAngularVelocityDampingRate () const;

	// PURPOSE: Completely disable damping on this collider
	void SetDampingEnabled(bool enableDamping);
	bool IsDampingEnabled() const;

	// PURPOSE: Enable also damping after the force solver
	void SetDoubleDampingEnabled(bool enableDamping);
	bool IsDoubleDampingEnabled() const;

	// PURPOSE: Set the angular velocity of the collider, and calculate the angular momentum from the new angular velocity.
	// PARAMS: angVelocity	- the new angular velocity
	inline void SetAngVelocity (Vec::V3Param128 angVelocity);
	void SetAngVelocityImp (Vec::V3Param128 angVelocity);

	// PURPOSE: Set the angular velocity of the collider without recomputing the angular momentum.
	// PARAMS: angVelocity	- the new angular velocity
	void SetAngVelocityOnly (Vec::V3Param128 angVelocity);

	// PURPOSE: Add momentum to the collider and clamp the velocity
	// PARAMS: deltaMomentum - the change in momentum
	void AddMomentum(Vec::V3Param128 deltaMomentum);

	// PURPOSE: Add angular momentum to the collider and clamp the angular velocity
	// PARAMS: deltaAngMomentum - the change in angular momentum
	void AddAngMomentum(Vec::V3Param128 deltaAngMomentum);

	// PURPOSE: Tell if this collider is articulated.
	// RETURN:	true if this is an articulated collider, false if it is not
	inline bool IsArticulated () const;
	PH_NON_SPU_VIRTUAL inline bool CanBeArticulated() const { return false; }

	// PURPOSE: Tell if this collider is articulated with a large root.
	// RETURN:	true if this is an articulated collider with a dominant root, false otherwise
	// NOTES:	Articulated with a large root means that the majority of the mass is in the root part (such as a car with doors).
	inline bool IsArticulatedLargeRoot () const;

	// IsArticulated but not LargeRoot - added for gta4 to recognise ragdolls
	// Would like this to be made available on rage\dev
	inline bool IsArticulatedSmallRoot () const;

	// PURPOSE: Tell whether this is a constrained collider.
	// RETURN:	This base class version always returns false. Constrained colliders override this method to return true.
	PH_NON_SPU_VIRTUAL bool IsConstrained () const;

	// PURPOSE: Add the given push to the collider's total push.
	// PARAMS:
	//	push - the push to add to the collider's push
	// NOTES:	The usual push application does debug drawing and adds the push in a way that does not duplicate pushes.
	//			This is used by the accumulation solver to move colliders without doing those things.
	void ApplyPushDirect (Vec::V3Param128 push);

	// PURPOSE: Add the given turn to the collider's total turn.
	// PARAMS:
	//	turn - the turn to add to the collider's turn
	// NOTES:	The usual turn application does debug drawing and adds the turn in a way that does not duplicate turns.
	//			This is used by the accumulation solver to rotate colliders without doing those things.
	void ApplyTurnDirect (Vec::V3Param128 turn);

	void SetExtraAllowedPenetration(float extraAllowedPenetration)
	{
		m_ExtraAllowedPenetration = extraAllowedPenetration;
	}

	float GetExtraAllowedPenetration() const
	{
		return m_ExtraAllowedPenetration;
	}

	ScalarV_Out GetExtraAllowedPenetrationV() const
	{
		return ScalarV(m_ExtraAllowedPenetration);
	}

	inline void RevertImpulses();
	void RevertImpulsesRigid();

#if __DEBUGLOG
	void DebugReplay() const;
#endif

#if __DEV
	static void SetDebugColliderUpdate(phCollider* debugCollider);
#endif

	void SetApproximateRadius(float approximateRadius)
	{
		m_ApproximateRadius = approximateRadius;
	}

	float GetApproximateRadius()
	{
		return m_ApproximateRadius;
	}

#if TRACK_COLLISION_TIME
	void DecreaseCollisionTimeSafe(float time);
	void SetCollisionTime(float newTime) {m_CollisionTime = newTime; }

	const float& GetCollisionTime()
	{
		return m_CollisionTime;
	}
#endif

#if __PS3
	class DmaPlan : public sysDmaPlan
	{
	public:
		DmaPlan()
		{
			m_DmaList = &m_DmaListStorage;
			m_Fixups = NULL;
			m_MaxDmas = 1;
			m_MaxFixups = 0;
		}

	private:
		CellDmaListElement m_DmaListStorage;
		//u8 m_Pad[8];
	} ;

	void GenerateDmaPlan(DMA_PLAN_ARGS(phCollider));

	inline u32 GetDmaPlanSize();
	u32 GetDmaPlanSizeRigid()
	{
		return sizeof(DmaPlan);
	}

	sysDmaPlan* GetDmaPlan()
	{
		return m_DmaPlan;
	}
#endif

	SPU_ONLY(void SetLSInstance(phInst* inst) { m_Instance = inst; })

	void SetSolverInvMass(const float& solverInvMass) {m_SolverInvAngInertiaXYZSolverInvMassW.SetWf(solverInvMass);}
	float GetSolverInvMass() const {return m_SolverInvAngInertiaXYZSolverInvMassW.GetWf();}
	void ResetSolverInvMass() {m_SolverInvAngInertiaXYZSolverInvMassW.SetWf(GetInvMass());}

	void SetSolverInvAngInertia(Vec::V3Param128 solverInvAngInertia) {m_SolverInvAngInertiaXYZSolverInvMassW.SetXYZ(Vec3V(solverInvAngInertia));}
	Vec3V_Out GetSolverInvAngInertia() const {return m_SolverInvAngInertiaXYZSolverInvMassW.GetXYZ();}
	void ResetSolverInvAngInertia() { m_SolverInvAngInertiaXYZSolverInvMassW.SetXYZ( m_UseSolverInvAngInertiaResetOverride ? m_SolverInvAngInertiaResetOverride : GetInvAngInertia() ); }

	void SetSolverInvAngInertiaResetOverride( Vec::V3Param128 solverInvAngInertiaResetOverride ){m_SolverInvAngInertiaResetOverride = Vec3V(solverInvAngInertiaResetOverride);}
	Vec3V_ConstRef GetSolverInvAngInertiaResetOverride(){ return m_SolverInvAngInertiaResetOverride; }

	bool GetUseSolverInvAngInertiaResetOverride() { return m_UseSolverInvAngInertiaResetOverride; }
	bool GetUseSolverInvAngInertiaResetOverrideInWheelIntegrator() { return m_UseSolverInvAngInertiaResetOverrideInWheelIntegrator; }
	void UseSolverInvAngInertiaResetOverride(bool bUseSolverInvAngInertiaResetOverride, bool bUseSolverInvAngInertiaResetOverrideInWheelIntegrator = false) { m_UseSolverInvAngInertiaResetOverride = bUseSolverInvAngInertiaResetOverride; m_UseSolverInvAngInertiaResetOverrideInWheelIntegrator = bUseSolverInvAngInertiaResetOverrideInWheelIntegrator;}

	void SetRotationConstraintInvMass(const float rotationConstraintInvMass) {m_RotationConstraintInvAngInertiaXYZInvMassZ.SetWf(rotationConstraintInvMass);}
	float GetRotationConstraintInvMass() const {return m_RotationConstraintInvAngInertiaXYZInvMassZ.GetWf();}
	void ResetRotationConstraintInvMass() {SetRotationConstraintInvMass(GetInvMass());}

	void SetRotationConstraintInvAngInertia(Vec::V3Param128 rotationConstraintInvAngInertia) {m_RotationConstraintInvAngInertiaXYZInvMassZ.SetXYZ(Vec3V(rotationConstraintInvAngInertia));}
	Vec3V_ConstRef GetRotationConstraintInvAngInertia() const {return (Vec3V_ConstRef)m_RotationConstraintInvAngInertiaXYZInvMassZ;}
	void ResetRotationConstraintInvAngInertia() {SetRotationConstraintInvAngInertia(GetInvAngInertia().GetIntrin128ConstRef());}

	void SetTranslationConstraintInvMass(const float translationConstraintInvMass) {m_TranslationConstraintInvAngInertiaXYZInvMassZ.SetWf(translationConstraintInvMass);}
	float GetTranslationConstraintInvMass() const {return m_TranslationConstraintInvAngInertiaXYZInvMassZ.GetWf();}
	void ResetTranslationConstraintInvMass() {SetTranslationConstraintInvMass(GetInvMass());}

	void SetTranslationConstraintInvAngInertia(Vec::V3Param128 translationConstraintInvAngInertia) {m_TranslationConstraintInvAngInertiaXYZInvMassZ.SetXYZ(Vec3V(translationConstraintInvAngInertia));}
	Vec3V_ConstRef GetTranslationConstraintInvAngInertia() const {return (Vec3V_ConstRef)m_TranslationConstraintInvAngInertiaXYZInvMassZ;}
	void ResetTranslationConstraintInvAngInertia() {SetTranslationConstraintInvAngInertia(GetInvAngInertia().GetIntrin128ConstRef());}

	void SetNeedsUpdateBeforeFinalIterations(bool value) {m_NeedsUpdateBeforeFinalIterations=value;}
	bool GetNeedsUpdateBeforeFinalIterations() {return m_NeedsUpdateBeforeFinalIterations;}

	void SetClearNextWarmStart(bool value) {m_ClearNextWarmStart=value;}
	bool GetClearNextWarmStart() {return m_ClearNextWarmStart;}

	// PURPOSE: Add the debugging widget group, which currently live in the "rage - phSimulator" group.
	// NOTES:
	//	Requires DEBUG_COLLIDERS in collider.cpp to be enabled for any widgets to be added.
	static void AddWidgets(bkBank& bank);

protected:
	// PURPOSE: Draw a force vector as an arrow.
	// PARAMS:
	//	force - the force to draw
	//	position - the position at which to draw the force arrow
	// NOTES:	This is only active when debug drawing of forces is turned on.
	void DrawForce (Vec3V_In force, Vec3V_In position);

	// PURPOSE: Draw an impulse vector as an arrow.
	// PARAMS:
	//	impulse - the impulse to draw
	//	position - the position at which to draw the impulse arrow
	// NOTES:	This is only active when debug drawing of impulses is turned on.
	void DrawImpulse (Vec3V_In impulse, Vec3V_In position);

	// PURPOSE: Draw a push vector as an arrow.
	// PARAMS:
	//	push - the push to draw
	//	position - the position at which to draw the push arrow
	// NOTES:	This is only active when debug drawing of pushes is turned on.
	void DrawPush (Vec3V_In push, Vec3V_In position);

	// PURPOSE: Draw a torque vector as an arrow.
	// PARAMS:
	//	torque - the torque to draw
	//	position - the position at which to draw the torque arrow
	// NOTES:	This is only active when debug drawing of torques is turned on.
	void DrawTorque (Vec3V_In torque, Vec3V_In position);

	// PURPOSE: Draw an angular impulse vector as an arrow.
	// PARAMS:
	//	angImpulse - the angular impulse to draw
	//	position - the position at which to draw the angular impulse arrow
	// NOTES:	This is only active when debug drawing of angular impulses is turned on.
	void DrawAngularImpulse (Vec3V_In angImpulse, Vec3V_In position);

	// PURPOSE: Draw a turn vector as an arrow.
	// PARAMS:
	//	turn - the turn to draw
	//	position - the position at which to draw the turn arrow
	// NOTES:	This is only active when debug drawing of turns is turned on.
	void DrawTurn (Vec3V_In turn, Vec3V_In position);

	// PURPOSE: Renormalize and reorthogonalize the collider's orientation matrix to avoid drift.
	// NOTES:	This will renormalize and reorthogonalize the orientation matrix at random intervals between NUM_REJUVENATE_UPDATES and
	//			NUM_REJUVENATE_UPDATES/2 update cycles. When REJUVENATE_TEST is true, updates will only occur when an error is larger
	//			than REJUVENATE_ERROR.
	bool RejuvenateImp ();

	// PURPOSE: Compute the acceleration from damping forces.
	// PARAMS:
	//	velocity - the collider's velocity
	//	timeStep - the time interval over which to damp the motion
	//	invTimeStep - one over the time interval
	//	activeC - whether constant damping is active
	//	dampC - constant damping multiplier
	//	activeV - whether velocity damping is active
	//	dampV - velocity damping multiplier
	//	activeV2 - whether velocity squared damping is active
	//	dampV2 - velocity squared damping multiplier
	// RETURN:	the acceleration from damping
	// NOTES:
	//	1.	This is used for computing damping on both linear and angular motion.
	//	2.	This is also used for each body part in articulated colliders.
	static Vec::V3Return128 ComputeDampingAccel(Vec::V3Param128 velocity, Vec::V3Param128 timeStep, Vec::V3Param128 invTimeStep, Vec::V3Param128_After3Args dampC,
												Vec::V3Param128_After3Args dampV, Vec::V3Param128_After3Args dampV2);


	// PURPOSE: Damp the linear and angular reference frame velocities
	// PARAMS:
	//  timeStep - the time interval over which to damp the reference frame velocities
	void DampReferenceFrameVelocities(Vec::V3Param128 timeStep);

protected:
	// PURPOSE: Make room for the VTBL ptr if the class doesn't have virtuals so object size is same on spu, and ppu.
	PH_NON_SPU_VIRTUAL_ONLY(void* m_VTBLptr;)

	// PURPOSE: the m_ColliderType is 0 for rigid (normal) colliders, 1 for articulated colliders, 2 for constrained colliders
	// NOTES: This is so the SPU can tell which colliders are articulated
	int m_ColliderType;

	// PURPOSE: the physics instance that this collider is controlling
#if __SPU
	phInst *m_Instance;
#else
	pgRef<phInst> m_Instance;
#endif

	// PURPOSE: the controller for making this collider sleep (no updating when motion stops) and awake (when sufficient motion is predicted)
	phSleep* m_Sleep;

	//=========================================================================
	// Physical and simulation properties of the rigid body.

	// PURPOSE: Component-wise inverse of the angular inertia vector and inverse of mass.
	Vec4V m_InvAngInertiaXYZInvMassW;

	//=========================================================================
	// Physical state of the rigid body.

	// PURPOSE: The position and orientation of the object at the center of mass.
	Mat34V m_Matrix;

	// PURPOSE: Linear velocity of the object.
	Vec3V m_Velocity;

	// PURPOSE: Angular velocity of the object.
	Vec3V m_AngVelocity;	// The current speed and direction, and rotation speed of the object

	// PURPOSE: Summation of pushes accumulated thus far this simulation step.
	Vec3V m_Push;
	
	// PURPOSE: Summation of turns (angular pushes) accumulated thus far this simulation step.
	Vec3V m_Turn;

	Vec4V m_SolverInvAngInertiaXYZSolverInvMassW;
	
	Vec3V m_SolverInvAngInertiaResetOverride;

	float m_ExtraAllowedPenetration;

	// PURPOSE: Upper limit on the linear speed (velocity magnitude) for this object.
	float m_MaxSpeed;
		
	// PURPOSE: Upper limit on the angular speed (any component of m_AngVelocity) for this object.
	float m_MaxAngSpeed;

	// PURPOSE: Factor representing the degree to which this object responds to gravity.  (Note: FLAG_NO_GRAVITY instance flag will still
	//   prevent gravity from being applied regardless of what m_GravityFactor is set to.  Perhaps the gravity factor should get set to
	//   zero for instances that have that flag set?)
	float m_GravityFactor;

	// PURPOSE: The velocity of the object during the last call to UpdateLastVelocities()
	Vec3V m_LastVelocity;

	// PURPOSE: The angular velocity of the object during the last call to UpdateLastVelocities()
	Vec3V m_LastAngVelocity;

	// PURPOSE: The velocity before forces were applied
	Vec3V m_VelocityBeforeForce;

	// PURPOSE: The angular velocity before forces were applied
	Vec3V m_AngVelocityBeforeForce;

	// PURPOSE: The angular inertia and mass of the object.
	// NOTES:
	//   The components of this vector must correspond the principle
	//   axes of angular inertia of the body.  This means that the
	//   collider must be oriented to satisfy this and arbitrary off-axis
	//   alignments is handled by orienting the instance instead.
	Vec4V m_AngInertiaXYZMassW;

	// PURPOSE: the matrix of the collider's instance on the previous frame
	// NOTES:	This is used to get the collider's motion for collision detection.
	Mat34V m_LastInstanceMatrix;

	Mat34V m_LastSafeInstanceMatrix;

	// PURPOSE: Linear velocity of the referencing container
	Vec4V m_ReferenceFrameVelocityXYZDampingRateW;
	
	// PURPOSE: Angular velocity of the referencing container
	Vec4V m_ReferenceFrameAngularVelocityXYZDampingRateW;

	//=========================================================================
	// Accumulated actions on this collider.

	// PURPOSE: Summation of force accumulated thus far this simulation step.
	// NOTES: This is in addition to m_Impulse. Forces apply gradually over an entire frame, meaning that externally applied
	//			forces can be cancelled by collisions before they change the object's velocity. Externally applied impulses
	//			change the object's velocity before collisions.
	Vec3V m_Force;

	// PURPOSE: Summation of torque (angular force) accumulated thus far this simulation step.
	// NOTES: This is in addition to m_AngImpulse. Torques apply gradually over an entire frame, meaning that externally applied
	//			torques can be cancelled by collisions before they change the object's angular velocity. Externally applied
	//			angular impulses change the object's angular velocity before collisions.
	Vec3V m_Torque;

	// PURPOSE: Summation of impulse (force * time) accumulated thus far this simulation step.
	// NOTES: This is in addition to m_Force. Forces apply gradually over an entire frame, meaning that externally applied
	//			forces can be cancelled by collisions before they change the object's velocity. Externally applied impulses
	//			change the object's velocity before collisions.
	Vec3V m_Impulse;
		
	// PURPOSE: Summation of angular impulse (torque * time) accumulated thus far this simulation step. 
	// NOTES: This is in addition to m_Torque. Torques apply gradually over an entire frame, meaning that externally applied
	//			torques can be cancelled by collisions before they change the object's angular velocity. Externally applied
	//			angular impulses change the object's angular velocity before collisions.
	Vec3V m_AngImpulse;

	// PURPOSE: The total push applied and used on this frame.
	// NOTES:	This is used as a buffer between m_Push and m_LastPush because externally applied pushes and collision pushes move
	//			the collider at different times during the simulator's update.
	Vec3V m_AppliedPush;
	
	// PURPOSE: The total turn (angular push) applied and used on this frame.
	Vec3V m_AppliedTurn;

	// Ability to treat collider as though it has infinite mass in new force solver.
	// Bodies fixed to the world with translational constraints
	// will be treated in all other constraints as though 
	// they have infinite mass in order 
	// to strictly enforce world constraints.
	Vec4V m_RotationConstraintInvAngInertiaXYZInvMassZ;
	Vec4V m_TranslationConstraintInvAngInertiaXYZInvMassZ;

	//=========================================================================
	// Other properties



	// PURPOSE: A place to store the last radius before updating the collider
	float m_ApproximateRadius;

#if __PS3
	sysDmaPlan* m_DmaPlan;
#endif

#if TRACK_COLLISION_TIME
	float m_CollisionTime;
#endif

	// PURPOSE: the number of frames since the collider's matrix was last orthonormalized
	// NOTES:
	//	This is used to prevent the collider's matrix from becoming non-orthonormal from roundoff errors.
	u8 m_RejuvenateCount;

	// Flags

	// PURPOSE: collider was deeply penetrated on the last, frame, don't update its last matrix	
	bool m_CurrentlyPenetrating : 1;
	
	// PURPOSE: collider has reached a non-penetrating position after a teleport
	bool m_NonPenetratingAfterTeleport : 1;

	EARLY_FORCE_SOLVE_ONLY(bool m_NeedsCollision : 1;)

	bool m_CausesPushCollisions : 1;

	bool m_UseSolverInvAngInertiaResetOverride : 1;
	bool m_UseSolverInvAngInertiaResetOverrideInWheelIntegrator : 1;

	bool m_PreventsPushCollisions : 1;

	TRACK_PUSH_COLLIDERS_ONLY(bool m_IsInPushPair : 1;)

	bool m_DampingEnabled : 1;

	bool m_DoubleDampingEnabled : 1;

	bool m_NeedsUpdateBeforeFinalIterations : 1;

	bool m_ClearNextWarmStart : 1;

	// avoided bitfield as last member because of compiler issue on VS2012
	u8 m_CurrentlyPenetratingCount;

#if __DEV
	static phCollider* sm_DebugColliderUpdate;
#endif
};


//=============================================================================
// Implementations

inline void phCollider::SetSleep (phSleep* sleep)
{
	m_Sleep = sleep;
}

__forceinline Vec3V_Out phCollider::ClampVelocity(Vec::V3Param128 velocity)
{
	return ClampMag(Vec3V(velocity), ScalarV(V_ZERO), ScalarV(m_MaxSpeed));
}

__forceinline Vec3V_Out phCollider::ClampAngularVelocity(Vec::V3Param128 angVelocity)
{
	return ClampMag(Vec3V(angVelocity), ScalarV(V_ZERO), ScalarV(m_MaxAngSpeed));
}

inline phInst* phCollider::GetInstance () const
{
	return m_Instance;
}

inline int phCollider::GetType() const
{
	return m_ColliderType;
}

inline void phCollider::SetCurrentlyPenetrating ()
{
	if (!m_CurrentlyPenetrating)
	{
		m_CurrentlyPenetrating = true;
		if (++m_CurrentlyPenetratingCount > 254)
		{
			m_CurrentlyPenetratingCount = 254;
		}
	}
}

#if EARLY_FORCE_SOLVE
inline bool phCollider::GetNeedsCollision () const
{
	return m_NeedsCollision;
}
#endif // EARLY_FORCE_SOLVE

inline u16 phCollider::GetCurrentlyPenetratingCount () const
{ 
    return m_CurrentlyPenetratingCount;
}

inline void phCollider::ClearNonPenetratingAfterTeleport()
{
	m_NonPenetratingAfterTeleport = false;
}

inline void phCollider::DisablePushCollisions()
{
	m_CausesPushCollisions = false;
}

inline void phCollider::EnablePushCollisions()
{
	m_CausesPushCollisions = true;
}

inline void phCollider::PreventPushCollisions()
{
	m_PreventsPushCollisions = true;
}

inline void phCollider::AllowPushCollisions()
{
	m_PreventsPushCollisions = false;
}

inline bool phCollider::GetPreventsPushCollisions()
{
	return m_PreventsPushCollisions;
}

inline phSleep* phCollider::GetSleep () const
{
	return m_Sleep;
}

inline float phCollider::GetMass () const
{
	return m_AngInertiaXYZMassW.GetWf();
}

inline ScalarV_Out phCollider::GetMassV () const
{
	return m_AngInertiaXYZMassW.GetW();
}

inline float phCollider::GetInvMass () const
{
	return m_InvAngInertiaXYZInvMassW.GetWf();
}

inline ScalarV_Out phCollider::GetInvMassV () const
{
	return m_InvAngInertiaXYZInvMassW.GetW();
}

inline Vec3V_ConstRef phCollider::GetVelocity () const
{
	return m_Velocity;
}

__forceinline void phCollider::SetVelocityOnly (Vec::V3Param128 velocity)
{
	PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), velocity));
	Assert(IsFiniteAll(Vec3V(velocity)));
	m_Velocity = Vec3V(velocity);
}

inline Vec3V_ConstRef phCollider::GetAngVelocity () const
{
	return m_AngVelocity;
}

__forceinline void phCollider::SetAngVelocityOnly (Vec::V3Param128 angVelocity)
{
	PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), angVelocity));
	Assert(IsFiniteAll(Vec3V(angVelocity)));
	m_AngVelocity = Vec3V(angVelocity);
}

inline void phCollider::UpdateLastVelocities()
{
	m_LastVelocity = m_Velocity;
	m_LastAngVelocity = m_AngVelocity;
}

inline Vec3V_ConstRef phCollider::GetLastVelocity() const
{
	return m_LastVelocity;
}

inline Vec3V_ConstRef phCollider::GetLastAngVelocity() const
{
	return m_LastAngVelocity;
}

inline Vec3V_ConstRef phCollider::GetVelocityBeforeForce() const
{
	return m_VelocityBeforeForce;
}

inline Vec3V_ConstRef phCollider::GetAngVelocityBeforeForce() const
{
	return m_AngVelocityBeforeForce;
}

inline Vec3V_Out phCollider::CalculateMomentum () const
{
	return Scale(m_Velocity, GetMassV());
}

inline Vec3V_Out phCollider::CalculateAngMomentumRigid () const
{
	Mat33V inertiaMtx;
	GetInertiaMatrixImp(inertiaMtx);
	return Multiply(inertiaMtx, m_AngVelocity);
}

inline Vec3V_ConstRef phCollider::GetAngInertia () const
{
	return (Vec3V_ConstRef)m_AngInertiaXYZMassW;
}

inline Vec3V_ConstRef phCollider::GetInvAngInertia () const
{
	return (Vec3V_ConstRef)m_InvAngInertiaXYZInvMassW;
}

inline Vec3V_ConstRef phCollider::GetPush () const
{
	return m_Push;
}

inline void phCollider::SetPush (Vec::V3Param128 push)
{
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(Vec3V(push)), Vec3V(V_FLT_LARGE_6)), "phCollider::SetPush - setting pushes to a large number.  Curr pushes = %f, %f, %f.  New pushes = %f, %f, %f",
		VEC3V_ARGS(m_Push), 
		VEC3V_ARGS(Vec3V(push)));
#endif
	m_Push = Vec3V(push);
}

inline Vec3V_ConstRef phCollider::GetAppliedPush () const
{
	return m_AppliedPush;
}

inline Vec3V_ConstRef phCollider::GetTurn () const
{
	return m_Turn;
}

inline void phCollider::SetTurn (Vec::V3Param128 turn)
{
	m_Turn = Vec3V(turn);
}

inline Vec3V_ConstRef phCollider::GetAppliedTurn () const
{
	return m_AppliedTurn;
}

inline void phCollider::SetMaxSpeed (float maxSpeed)
{
	m_MaxSpeed = maxSpeed;
}

inline void phCollider::SetMaxAngSpeed (float maxAngSpeed)
{
	m_MaxAngSpeed = maxAngSpeed;
}

inline float phCollider::GetMaxSpeed () const
{
	return m_MaxSpeed;
}

inline float phCollider::GetMaxAngSpeed () const
{
	return m_MaxAngSpeed;
}

inline void phCollider::SetGravityFactor(float gravityFactor)
{
	m_GravityFactor = gravityFactor;
}

inline const float &phCollider::GetGravityFactor() const
{
	return m_GravityFactor;
}

inline void phCollider::GetInertiaMatrixImp (Mat33V_InOut inertia, int UNUSED_PARAM(component)) const
{
	// Find Matrix<-1> I Matrix  - the inertia matrix (tensor) in global coordinates.
	// This matrix multiplies vectors from the right side.
	Mat33V mat = m_Matrix.GetMat33();
	Mat33V transposeMat;
	Transpose( transposeMat, mat );

	Mat33V v_inertia;
	const Vec3V v_angInertia = GetAngInertia();
	v_inertia.SetCol0( Scale( transposeMat.GetCol0(), v_angInertia ) );
	v_inertia.SetCol1( Scale( transposeMat.GetCol1(), v_angInertia ) );
	v_inertia.SetCol2( Scale( transposeMat.GetCol2(), v_angInertia ) );
	Multiply( v_inertia, mat, v_inertia );

	inertia = v_inertia;
}

inline Mat34V_ConstRef phCollider::GetMatrix () const
{
	return m_Matrix;
}

inline void phCollider::SetMatrix (Mat34V_In matrix)
{
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(matrix.GetCol3()), Vec3V(V_FLT_LARGE_6)), "phCollider::SetMatrix - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()), 
		VEC3V_ARGS(matrix.GetCol3()));
#endif
	m_Matrix = matrix;
}

inline Vec3V_ConstRef phCollider::GetPosition () const
{
	return m_Matrix.GetCol3ConstRef();
}

inline void phCollider::SetPosition (Vec::V3Param128 position)
{
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll( Abs(Vec3V(position)), Vec3V(V_FLT_LARGE_6)), "phCollider::SetPosition - setting matrix to a large number.  Curr mat pos = %f, %f, %f",
		VEC3V_ARGS(m_Matrix.GetCol3()));
#endif
	m_Matrix.SetCol3( Vec3V(position) );
}

inline Mat34V_ConstRef phCollider::GetLastInstanceMatrix () const
{
	return m_LastInstanceMatrix;
}

inline void phCollider::SetLastInstanceMatrix (Mat34V_In lastSafeInstanceMatrix)
{
	m_LastInstanceMatrix = m_LastSafeInstanceMatrix = lastSafeInstanceMatrix;
}

inline Mat34V_ConstRef phCollider::GetLastSafeInstanceMatrix () const
{
	return m_LastSafeInstanceMatrix;
}

inline void phCollider::ApplyPushDirect (Vec::V3Param128 push)
{
	Vec3V setPush = Add( m_Push, Vec3V(push) );
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
	Assertf(IsLessThanAll(Abs(setPush), Vec3V(V_FLT_LARGE_6)), "phCollider::ApplyPushDirect - setting pushes to a large number.  Curr pushes = %f, %f, %f.  New pushes = %f, %f, %f",
		VEC3V_ARGS(m_Push), 
		VEC3V_ARGS(setPush));
#endif
	m_Push = setPush;
}

inline void phCollider::ApplyTurnDirect (Vec::V3Param128 turn)
{
	m_Turn = Add( m_Turn, Vec3V(turn) );
}

inline Vec3V_Out phCollider::GetForce (Vec::V3Param128 invTimeStep) const
{
	return AddScaled(m_Force, m_Impulse, ScalarV(invTimeStep));
}

inline Vec3V_Out phCollider::GetTorque (Vec::V3Param128 invTimeStep) const
{
	return AddScaled(m_Torque, m_AngImpulse, ScalarV(invTimeStep));
}

inline Vec3V_Out phCollider::GetImpulseRigid (Vec::V3Param128 timeStep) const
{
	return AddScaled(m_Impulse, m_Force, ScalarV(timeStep));
}

inline Vec3V_Out phCollider::GetAngImpulse (Vec::V3Param128 timeStep) const
{
	return AddScaled(m_AngImpulse, m_Torque, ScalarV(timeStep));
}

inline Vec3V_ConstRef phCollider::GetReferenceFrameVelocity () const
{ 
	return (Vec3V_ConstRef)m_ReferenceFrameVelocityXYZDampingRateW; 
}

inline Vec3V_ConstRef phCollider::GetReferenceFrameAngularVelocity () const
{ 
	return (Vec3V_ConstRef)m_ReferenceFrameAngularVelocityXYZDampingRateW; 
}

inline void phCollider::SetReferenceFrameVelocityDampingRate (float rate)
{ 
	m_ReferenceFrameVelocityXYZDampingRateW.SetWf(rate); 
}

inline ScalarV_Out phCollider::GetReferenceFrameVelocityDampingRate () const
{
	return m_ReferenceFrameVelocityXYZDampingRateW.GetW();
}

inline void phCollider::SetReferenceFrameAngularVelocityDampingRate (float rate)
{ 
	m_ReferenceFrameAngularVelocityXYZDampingRateW.SetWf(rate); 
}

inline ScalarV_Out phCollider::GetReferenceFrameAngularVelocityDampingRate () const
{
	return m_ReferenceFrameAngularVelocityXYZDampingRateW.GetW();
}

inline void phCollider::SetDampingEnabled(bool enableDamping)
{
	m_DampingEnabled = enableDamping;
}

inline bool phCollider::IsDampingEnabled() const
{
	return m_DampingEnabled;
}

inline void phCollider::SetDoubleDampingEnabled(bool enableDamping)
{
	m_DoubleDampingEnabled = enableDamping;
}

inline bool phCollider::IsDoubleDampingEnabled() const
{
	return m_DoubleDampingEnabled;
}

inline bool phCollider::IsArticulated () const
{
	return m_ColliderType == TYPE_ARTICULATED_BODY || 	m_ColliderType == TYPE_ARTICULATED_LARGE_ROOT;
}

inline bool phCollider::IsArticulatedLargeRoot () const
{
	return m_ColliderType == TYPE_ARTICULATED_LARGE_ROOT;
}

// Added for gta4 to distinguish ragdolls
// Would like this to be made available on rage\dev
inline bool phCollider::IsArticulatedSmallRoot () const
{
	return m_ColliderType == TYPE_ARTICULATED_BODY;
}

inline Vec::V3Return128 phCollider::ComputeDampingAccel(Vec::V3Param128 velocity, Vec::V3Param128 UNUSED_PARAM(timeStep), Vec::V3Param128 invTimeStep, Vec::V3Param128_After3Args dampC,
														Vec::V3Param128_After3Args dampV, Vec::V3Param128_After3Args dampV2)
{
	const Vec3V velocityLocal(velocity);
	const Vec3V velocityDir = NormalizeSafe(velocityLocal, Vec3V(V_ZERO));
	// This dot product wouldn't need to happen if we manually normalized velocityLocal above and saved off its magnitude
	ScalarV speed = Dot(velocityLocal, velocityDir);

	// Find the acceleration from velocity squared damping.
	Vec3V acceleration = Scale( Scale(velocityLocal, speed), Vec3V(dampV2));

	// Find and add the acceleration from velocity damping.
	acceleration = AddScaled(acceleration, velocityLocal, Vec3V(dampV));

	// Find and add the acceleration from constant damping.
	acceleration = AddScaled(acceleration, velocityDir, Vec3V(dampC));

	// Make sure the acceleration won't reverse the velocity.
	Vec3V maxAcceleration = Scale(velocityLocal, ScalarV(invTimeStep)); // velocity*invTimeStep
	// This would incorrectly clamp for negative damping values in that they would pop to the opposite direction upon reaching the mag threshold
	acceleration = SelectFT( IsLessThan(Abs(acceleration), Abs(maxAcceleration)), maxAcceleration, acceleration );

	// Negate the acceleration to make it opposite current velocity and then return the damping.
	return Negate(acceleration).GetIntrin128();
}

inline void phCollider::DampReferenceFrameVelocities(Vec::V3Param128 timeStep)
{
	ScalarV v_LinearDecayRate = GetReferenceFrameVelocityDampingRate();
	ScalarV v_AngularDecayRate = GetReferenceFrameAngularVelocityDampingRate();
	ScalarV v_one(V_ONE);
	ScalarV v_timeStep = ScalarV(timeStep);
	ScalarV v_linearDamping = SubtractScaled( v_one, v_timeStep, v_LinearDecayRate );
	ScalarV v_angularDamping = SubtractScaled( v_one, v_timeStep, v_AngularDecayRate );
	SetReferenceFrameVelocity(Scale(GetReferenceFrameVelocity(), v_linearDamping).GetIntrin128ConstRef());
	SetReferenceFrameAngularVelocity(Scale(GetReferenceFrameAngularVelocity(), v_angularDamping).GetIntrin128ConstRef());
}

#if TRACK_PUSH_COLLIDERS
inline void phCollider::UpdateApplyZeroPushes()
{
	m_AppliedPush = m_AppliedTurn = Vec3V(V_ZERO);
	m_NeedsCollision = false;
}
#endif // TRACK_PUSH_COLLIDERS

} // namespace rage

#endif
