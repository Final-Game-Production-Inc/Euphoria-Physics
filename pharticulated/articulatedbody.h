//
// pharticulated/articulatedbody.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_ARTICULATEDBODY_TYPE_H
#define PHARTICULATED_ARTICULATEDBODY_TYPE_H

#include "joint.h"
#include "joint1dof.h"
#include "joint3dof.h"
#include "prismaticjoint.h"
#include "jointdispatch.h"
#include "bodypart.h"

#include "system/cache.h"
#include "atl/array.h"

#include "data/struct.h"
#include "data/safestruct.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#include "paging/base.h"
#include "paging/ref.h"

#define CAPTURE_STACK_INVALIDATING_JOINT_LIMITS 0

#if CAPTURE_STACK_INVALIDATING_JOINT_LIMITS
#include "system/stack.h"
#endif // CAPTURE_STACK_INVALIDATING_JOINT_LIMITS

#define PHARTICULATEDBODY_MULTITHREADED_VALDIATION __BANK
#if PHARTICULATEDBODY_MULTITHREADED_VALDIATION
#define PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(X) X
#else // PHARTICULATEDBODY_MULTITHREADED_VALDIATION
#define PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(X)
#endif // PHARTICULATEDBODY_MULTITHREADED_VALDIATION

namespace rage {


	class phBoundComposite;

	// Needed for PS3 compile ?
	class sysDmaPlan;
	class phJoint;
	class phJoint1Dof;
	class phJoint3Dof;
	class phPrismaticJoint;
	class phArticulatedBodyPart;
	class phJointType;
	class phJoint1DofType;
	class phJoint3DofType;
	class phPrismaticJointType;

#if __WIN32 && __64BIT
#pragma warning(push)
#pragma warning(disable: 4324)
#endif

	//PURPOSE: phArticulatedBodyType is a type for the collection of joints and body parts that make up a physically articulated object.
	//		Articulated means that it consists of a set of rigid parts that are joined together (like a truck and trailer, or an animal).
	//		This can also be used to simulate flexible objects that can be approximated by connected rigid parts, such as a ponytail.
	//		Every phArticulatedBodyType is owned by a phArticulatedCharacterLOD. 
	class phArticulatedBodyType : public pgBase
	{
		friend class phArticulatedBody;
		friend class fragType;

	public:
		// The maximum number of body parts, joints, and joint degrees of freedom in an articulated body.
		static const int MAX_NUM_LINKS = 23;
		static const int MAX_NUM_JOINTS = MAX_NUM_LINKS-1;
		static const int MAX_NUM_JOINT_DOFS = 3*MAX_NUM_JOINTS;

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s);
#endif

		DECLARE_PLACE(phArticulatedBodyType);

		phArticulatedBodyType(bool locallyOwned = false);
		phArticulatedBodyType(datResource &rsc);
		~phArticulatedBodyType();

		phJointType * GetJointTypes(int i) { return m_phJointTypes[i].ptr; }
		Vec4V_ConstRef GetResourcedAngInertiaXYZmassW(int i) { return m_ResourcedAngInertiaXYZmassW[i]; }

		int GetNumBodyParts () const { return m_NumLinks; }

		phArticulatedBodyType* Clone();

#if __PPU
		//void GenerateDmaPlan(sysDmaPlan& dmaPlan);
#endif

		void SetNumBodyParts(int numLinks);

		inline int GetJointParentIndex(int linkNum) const { return m_JointParentIndex[linkNum]; }

	protected:
		// PURPOSE: a parent body part index number for every joint index
		// NOTES:	this could contain only MAX_NUM_LINKS-1 but no sense changing the class size just for that
		int m_JointParentIndex[MAX_NUM_LINKS];

		// Please remove this member upon re-resource of articulated bodies
		float m_ReplaceUponReResource;

		// !!! Remove this the next time you rebuild ped ragdoll assets
		float m_AngularDecayRate;

		// Array of joint types
		datOwner<phJointType> *m_phJointTypes;

		// Array of masses and angular inertias not used during simulation, but used to initialize the body parts
		datOwner<Vec4V> m_ResourcedAngInertiaXYZmassW;

		// PURPOSE: the number of links (body parts)
		// NOTES:	this can be changed to remove (disable) body parts
		u8 m_NumLinks;
		u8 m_NumJoints;

		// Array of joint types of joints pointed to in m_JointPtr, to avoid spu dma.
		u8 m_JointTypes[MAX_NUM_JOINTS];

		// False if the AB type data is shared (or owned by fragType instead of the AB instance)
		bool m_LocallyOwned;

		datPadding<3, u8> m_Pad;
	} ;

#if __WIN32 && __64BIT
#pragma warning(pop)
#endif

	//PURPOSE: phArticulatedBody holds instance data for the collection of joints and body parts that make up a physically articulated object.
	//		Articulated means that it consists of a set of rigid parts that are joined together (like a truck and trailer, or an animal).
	//		This can also be used to simulate flexible objects that can be approximated by connected rigid parts, such as a ponytail.
	//		Every phArticulatedBodyType is owned by a fragCacheEntry ??? . 
	class phArticulatedBody
	{
	public:

		phArticulatedBody();
		phArticulatedBody(phArticulatedBodyType *bodyType);
		~phArticulatedBody();
		DECLARE_PLACE (phArticulatedBody);
		void AllocateJointsAndParts (int numParts, const phJoint::JointType* jointTypeList);
		void InitChain (phBoundComposite& compositeBound, bool threeDofJoints=true, const Matrix34& bodyMatrix=M34_IDENTITY, bool interlinked=false);
		void Reset ();

		void NullOutTypeDataPointers();

		int AddRoot( phArticulatedBodyPart& link );

		int GetTypeOfJoint(int jointIndex) const { return m_Type->m_JointTypes[jointIndex]; }

		phArticulatedBodyType& GetType() { return *m_Type; }

		// PURPOSE: Apply a phase space impulse to a body part
		// PARAMS:
		//	trans - the trans component of the phase space impulse
		//	omega - the omega component of the phase space impulse
		void ApplyPhaseSpaceImpulse (int linkIndex, Vec3V_In trans, Vec3V_In omega);

		// PURPOSE: Apply a force to the body part.
		// PARAMS:
		//	force - the force to apply, in world or body coordinates coordinates (same thing for directions)
		//	position - the position at which to apply the force, in body coordinates (position relative to the collider's position)
		void ApplyForce (int linkIndex, const Vector3& force, const Vector3& position, Vec::V3Param128 timeStep);

		// PURPOSE: Apply an impulse to the body part.
		// PARAMS:
		//	impulse - the impulse to apply, in world or body coordinates coordinates (same thing for directions)
		//	position - the position at which to apply the impulse, in body coordinates (position relative to the collider's position)
		void ApplyImpulse (int linkIndex, Vec3V_In impulse, Vec3V_In position);

		// PURPOSE: Apply the given torque to the articulated body part.
		// PARAMS:
		//	torque - the torque to apply to this body part
		void ApplyTorque (int linkIndex, Vec::V3Param128 torque, Vec::V3Param128 timeStep);

		// PURPOSE: Apply the given angular impulse to the articulated body part.
		// PARAMS:
		//	angImpulse - the angular impulse to apply to this body part
		void ApplyAngImpulse (int linkIndex, Vec::V3Param128 angImpulse);

		// PURPOSE: Add a body part (link) connected to the given parent body part index with the given joint.
		//		articulatedCollider->UpdateVelocityArraySizes() should always be called soon after this function.
		// PARAMS:
		//	parentIndex - the index number of the parent body part
		//	joint - the joint connecting the two body parts
		//	bodyPart - the body part to add
		// RETURN:	the index number of the new child body part
		int AddChild (int parentIndex, phJoint& joint, phArticulatedBodyPart& bodyPart);

		void RemoveChild (int linkIndex);

		void PreInit(int numLinks);

		void FixRoot (bool fixed=true) { m_RootIsFixed = fixed; }
		bool RootIsFixed () const { return m_RootIsFixed; }	

		void Freeze ();
		void ZeroLinkVelocities();

		void MovePosition (int partIndex, Vec3V_In offset);

		// PURPOSE: Set outward joint m_VelocityToPropDown to match link(partIndex)'s velocity
		void ResetDownVel(int partIndex);
		bool AreVelocitiesFullyPropagated();

		// PURPOSE: Set the linear velocity of this body part in world coordinates.
		// PARAMS:
		//	linearVelocity - the new linear velocity for this body part
		// NOTES:	This does not recompute the 6d spatial velocity.
		void SetLinearVelocity (int partIndex, Vec3V_In linearVelocity);
		void SetLinearVelocityNoDownVelAdjust (int partIndex, Vec3V_In linearVelocity);

		// PURPOSE: Set the angular velocity of this body part in world coordinates.
		// PARAMS:
		//	angularVelocity - the new angular velocity for this body part
		// NOTES:	This also recomputes the 6d spatial velocity.
		void SetAngularVelocity (int partIndex, Vec3V_In angularVelocity);
		void SetAngularVelocityNoDownVelAdjust (int partIndex, Vec3V_In angularVelocity);

		// PURPOSE: Set the linear and angular velocities of this body part in world coordinates.
		// PARAMS:
		//	linearVelocity - the new linear velocity for this body part
		//	angularVelocity - the new angular velocity for this body part
		// NOTES:	This also recomputes the 6d spatial velocity.
		void SetVelocities (int partIndex, Vec3V_In linearVelocity, Vec3V_In angularVelocity);
		void SetVelocitiesNoDownVelAdjust (int partIndex, Vec3V_In linearVelocity, Vec3V_In angularVelocity);

		void SetVelocity (int partIndex, const phPhaseSpaceVector& velocity);
		void SetVelocityNoDownVelAdjust (int partIndex, const phPhaseSpaceVector& velocity);

		Vec3V_Out GetAngularVelocityNoProp (int partIndex) const;
		Vec3V_Out GetAngularVelocity (int partIndex);

		Vec3V_Out GetLinearVelocityNoProp (int partIndex) const;
		Vec3V_Out GetLinearVelocity (int partIndex);

		// Get the velocity of a point on the link.  Position is relative to the instance position.
		void GetLocalVelocityNoProp( int partIndex, Vec::V3Param128 position, Vector3* returnedVelocity ) const;
		void GetLocalVelocity( int partIndex, Vec::V3Param128 position, Vector3* returnedVelocity );
		Vec3V_Out GetLocalVelocityNoProp( int partIndex, Vec::V3Param128 position ) const;
		Vec3V_Out GetLocalVelocity( int partIndex, Vec::V3Param128 position );

		const phPhaseSpaceVector& GetVelocityNoProp(int partIndex) const { return m_PartVelocities[partIndex]; } 
		const phPhaseSpaceVector& GetVelocity(int partIndex);
		void AddToVelocityNoProp(int partIndex, const phPhaseSpaceVector& addend) { m_PartVelocities[partIndex] += addend; }

		void MoveLinkPosition (int partIndex, Vec3V_In offset);

		void ZeroLinkVelocity(int partIndex);

		// PURPOSE: Get the parent body part index number from the given child body part index number.
		int GetParentNum (int bodyPartIndex) const;

		// Set stiffness of all the joints at once
		void SetStiffness( float stiffness );

		void SetBodyMinimumStiffness( float minStiffness );

		bool HasTypeData() const { return m_Type ? true : false; }

		void SetEffectorsToCurrentPose(float strength, float damping);
		void SetEffectorsToZeroPose();

		// The option to apply gravity to every part of the articulated body EXCEPT for the root
		// was added for gta4 vehicles, so we can apply gravity to the root inside our tyre/suspension integration step
		//
		// DO want to preserve this functionality, so would like to move to main dev branch
		void ApplyGravityToLinks (Vec::V3Param128 gravity, Vec::V3Param128 timestep, float gravityFactor, bool skipRoot=false);

		// PURPOSE: Clamp the body part velocities and angular velocities to maximum allowed values.
		// PARAMS:
		//	maxSpeed - the maximum linear speed
		//	maxAngSpeed - the maximum angular speed
		void ClampVelocities (Vec::V3Param128 maxSpeed, Vec::V3Param128 maxAngSpeed);

		void CalculateInertias();					// Calculate values that depend on positions only

		// PURPOSE: Take the precomputed inertias found by CalculateInertias(), and compute
		//			new velocities from the forces and impulses.
		// PARAMS:
		//	timeStep - the time interval over which to update the body part velocities
		// NOTES:	This includes the effect of bais forces, so it should not be called with zero time step.
		void UpdateVelocities (Vec::V3Param128 timeStep);

		// PURPOSE: Compute the torques from muscles and apply them to the joints.
		// PARAMS:
		//	timeStep - the time interval over which to apply the torques
		void ComputeAndApplyMuscleTorques (float timeStep);

		void UpdatePositionsFromVelocities( Vec::V3Param128 time );	// Update positions only.

		void SaveVelocities(Vec3V *savedLinearVelocities, Vec3V *savedAngularVelocities);
		void SaveAndZeroVelocities(Vec3V * RESTRICT savedLinearVelocities, Vec3V * RESTRICT savedAngularVelocities);
		void FinishPushesWithImpulses (Vec::V3Param128 pushTimeStep, Vec::V3Param128 dampingTimeStep);
		void RestoreVelocities(Vec3V *savedLinearVelocities, Vec3V *savedAngularVelocities);

		void MovePosition (Vec::V3Param128 offset);

		// PURPOSE: Rotate all the articulated body parts by the given rotation matrix.
		// PARAMS:
		//	rotation - the 3x4 matrix with which to rotate the body parts
		// NOTES:	The positions of the non-root body parts are changed to keep the joints together as the body is rotated.
		void RotateBody (Mat34V_In rotation);

		// PURPOSE: Rotate all the articulated body parts by the given turn.
		// PARAMS:
		//	turn - the axis (direction) and angle (magnitude) by which to rotate the body
		// NOTES:	The positions of the non-root body parts are changed to keep the joints together as the body is rotated.
		void RotateBody (Vec::V3Param128 turn);

		// PURPOSE: Rotate all the articulated body parts by the same amount so that the root matrix matches the given matrix.
		// PARAMS:
		//	toMatrix - the new orientation of the root body part
		// NOTES:	The positions of the non-root body parts are changed to keep the joints together as the body is rotated.
		void RotateBodyTo (Mat34V_In toMatrix);
#ifdef USE_SOFT_LIMITS
		// PURPOSE: Apply torques to resolve violations of soft joint limits.
		// PARAMS:
		//	timeStep - the time interval for this update
		// NOTES:	Hard joint limits are resolved in the collision solver.
		void EnforceSoftJointLimits (float timeStep);
#endif
		void SetDriveState (phJoint::driveState state);

	private:
		void SetRotationMatrix(Mat34V_InOut rotationMtx, Vec::V3Param128 u, Vec::V3Param128 theta);

		void CalculateBiasForces(phPhaseSpaceVector* biasForces);

	public:
		// Functions for non-physically updating the positions and
		//		velocities of the articulated links
		void JoinBackPositionsAndVelocitiesToRoot();		// Starting at root, match up positions and velocities of links
		void JoinBackPositionsAndVelocities();				// Same, but also preserves center of mass and momentum

		phArticulatedBodyPart&			GetLink (int linkIndex);
		const phArticulatedBodyPart&	GetLink (int linkIndex) const;
		phArticulatedBodyPart* const*	GetLinkArray();

		phJoint&                GetJoint (int jointIndex);
		const phJoint&          GetJoint( int jointIndex ) const;
		phJoint1Dof&            GetJoint1Dof (int jointIndex);
		const phJoint1Dof&      GetJoint1Dof (int jointIndex) const;
		phJoint3Dof&            GetJoint3Dof (int jointIndex);
		const phJoint3Dof&      GetJoint3Dof (int jointIndex) const;
		phPrismaticJoint&       GetPrismaticJoint (int jointIndex);
		const phPrismaticJoint& GetPrismaticJoint (int jointIndex) const;

		phJoint* const*			GetJointArray();

#if __SPU
		phJoint*				GetJointAddr(int jointIndex);
		phArticulatedBodyPart*	GetLinkAddr(int linkIndex);
		void							FixUpJointPtr(int jointIndex, phJoint* jointAddr);
		void							FixUpLinkPtr(int linkIndex, phArticulatedBodyPart* linkAddr);
#endif

		// PURPOSE: Set the specified body part pointer.
		// PARAMS:
		//	index - the index number of the body part to set
		//	bodyPart - a pointer to the body part
		void SetBodyPart (int index, phArticulatedBodyPart* bodyPart);

		// PURPOSE: Set the specified joint.
		// PARAMS:
		//	index - the index number of the joint to set
		//	joint - the joint
		void SetJoint (int index, phJoint* joint);

		void GetInertiaMatrix (Matrix33& outInertia, int bodyPartIndex=0) const;
		void GetInverseInertiaMatrix (Matrix33& outInvInertia, int bodyPartIndex=0) const;

		// PURPOSE: Get the effective inverse angular inertia for the given joint limit direction.
		// PARAMS:
		//	jointIndex - index of the joint on which to get the limit response
		//	limitDirectionIndex - index number for the direction of the limit (lean1, lean2 or twist for 3dof joints).
		// RETURN:	the response (scalar angular acceleration) to a unit torque in the given direction on the given joint
		float GetJointLimitResponse (int jointIndex, int limitDirectionIndex);

		// The next three "GetInverseMassMatrix" routines do not use PreCalc values.

		// PURPOSE: Compute the inverse mass matrix, which gives velocity response to an impulse.
		// PARAMS:
		//	linkNum - the index number of the link to which the impulse is applied and the velocity response is measured
		//	position - the position relative to the articulated object's instance position
		//	inverseMassMatrix - reference for filling in the inverse mass matrix
		// NOTES:	Velocity and force both measured at "position".
		void GetInverseMassMatrix (int linkNum, Vec::V3Param128 position, Matrix33& inverseMassMatrix) const;

		// PURPOSE: Compute the 3x3 inverse mass cross-inertia matrix.
		// PARAMS:
		//	linkA - the index number of the link to which the impulse is applied
		//	positionA - the position relative to the articulated object's instance position at which the impulse is applied
		//	linkB - the index number of the link to which the velocity response is measured
		//	positionB - the position relative to the articulated object's instance position at which the velocity response is measured
		//	inverseMassMatrix - reference for filling in the inverse mass matrix
		// NOTES:
		//	1.	This applies for a force applied at linkA, positionA, with a velocity response at linkB, positionB.
		//	2.	This uses the "incremental" method, but does not intefere with its operation.
		//	3.	This does not set or use PreCalc information.
		void GetInverseMassMatrix (int linkA, Vec::V3Param128 positionA, int linkB, Vec::V3Param128 positionB, Matrix33& inverseMassMatrix);

		// PURPOSE: Transform the force on linkA at positionA into an acceleration on linkB at positionB.
		// PARAMS:
		//	linkA - the index number of the link to which the impulse is applied
		//	positionA - the position relative to the articulated object's instance position at which the impulse is applied
		//	forceA - the force applied at positionA
		//	linkB - the index number of the link to which the velocity response is measured
		//	positionB - the position relative to the articulated object's instance position at which the velocity response is measured
		//	retAccelB - the resulting acceleration at positionB
		Vec3V_Out GetIMM_Helper (int linkA, Vec::V3Param128 positionA, Vec::V3Param128 forceA, int linkB, Vec::V3Param128 positionB);

		// PURPOSE: Set the linear velocity of the articulated body.
		// PARAMS: velocity	- the new linear velocity
		// NOTES:	This sets the linear velocity of the root body part, and then changes the linear velocities of all the other body parts
		//			by the same amount as the change in the root part's linear velocity, to maintain the current internal motion.
		void SetVelocity (Vec3V_In velocity);

		// PURPOSE: Set the angular velocity of the articulated body.
		// PARAMS: angVelocity	- the new angular velocity
		// NOTES:	This sets the angular velocity of the root body part, and then changes the angular velocities of all the other body parts
		//			by the same amount as the change in the root part's angular velocity, and then changes the linear velocities of all the other
		//			body parts to match the change in the root part's angular velocity, to maintain the current internal motion.
		void SetAngVelocity (Vec3V_In angVelocity);

		// Calculate the responses to a unit "torque" or "impulse" to the given joint limit
		void PrecalcResponsesToJointImpulse(int jointNum, int jointLimitID );
		// Get response to the joint limits's "Rate of Change" in response to a previously
		//		Precalc's impulse/torque.  The returned "response" is the acceleration or
		//		the delta-velocity to the joint limit value.
		float GetPrecalcJointLimitResponse(int jointNum, int jointLimitID );
		// Applies a impulse or torque to the axis corresponding to the joint limit.
		void ApplyJointImpulse( int jointNum, int jointLimitID, ScalarV_In impulse );
		void GetJointLimitAxis ( int jointNum, int jointLimitID, Vec3V_InOut axis, Vec3V_InOut position );
		void GetJointLimitUnitImpulse ( int jointNum, int jointLimitID, phPhaseSpaceVector& unitImpulseSpatial );

		float GetGravityFactor ();
		int GetNumJoints () const { return GetNumBodyParts()-1; }
		int GetNumBodyParts () const { return m_Type->GetNumBodyParts(); }
		u16 FindJointLimitDofs (int* jointIndices, int* dofIndices, float* dofExcessHard, float* dofResponse, float allowedAnglePenetration, u16& numDeepLimitDofsOut);

		// Routines for the "IncrementalDeltaVelocity" calculations
		// The "Init" should be called first.
		// Call the "GetIncremental..." routines to get *incremental* velocity or an angular velocity.
		// Call the "GetIncrementalNet..." routines to get net velocity or an angular velocity,
		//		that includes the starting velocity values plus the incremental values.
		// Call the "ApplyIncremental..." routines to apply an impulse or angular impulse.
		// Call IncorporateIncrementalVelocites() to add all incremental velocities into current velocity
		void ResetPropagationVelocities();

		void ApplyIncrementalImpulse (int link, Vec::V3Param128 position, Vec::V3Param128 impulse);
		int ApplyIncrementalImpulse (int link, Vec3V_In trans, Vec3V_In omega, int incrementalBase);

		void ApplyIncrementalJointLimitImpulse( int jointNum, int jointLimitID, ScalarV_In impulse );

		Vec3V_Out GetIncrementalJointLimitVelocity( int jointNum, int jointLimitID );

		void PredictImpulseEffects( int link, Vec3V_In impulse, Vec3V_In position, Vec3V_InOut linear, Vec3V_InOut angular);

		// Routines for internal use by the incremental update velocity routines
		int AddIncrementalVelocityToLink (int linkNum, const phPhaseSpaceVector* RESTRICT deltaVel, int incrementalBase);
		int PropagateIncrementalToLink(int linkNum, int incrementalBase);
		void PropagateOnceUp( int childLinkNum, int parentLinkNum );
		void PropagateOnceDown( int childLinkNum, int parentLinkNum );
		void EnsureVelocitiesFullyPropagated();
#if __ASSERT
		void CheckVelocitiesFullyPropagated();
#endif

#if PHARTICULATEDBODY_MULTITHREADED_VALDIATION
		void SetReadOnlyOnMainThread(bool readOnly);
		void CheckReadOnlyOnMainThread() const;
#endif // PHARTICULATEDBODY_MULTITHREADED_VALDIATION

		void ResetAllJointLimitAdjustments();

		ScalarV_Out GetMass(int linkNum) const;
		Vec3V_Out GetAngInertia(int linkNum) const;
		Vec4V_Out GetMassAndAngInertia(int linkNum) const;

		// PURPOSE: Set the mass of this articulated body part.
		// PARAMS:
		//	mass - the new mass
		// NOTES:	"Only" in the name here indicates that the angular inertia will not be changed.
		void SetMassOnly (int linkNum, float mass);
		void SetMassOnly (int linkNum, ScalarV_In mass);

		// PURPOSE: Set the angular inertia of this articulated body part.
		// PARAMS:
		//	inertia - the new angular inertia
		// NOTES:	"Only" in the name here indicates that the mass will not be changed.
		void SetAngInertiaOnly (int linkNum, const Vector3& inertia);
		void SetAngInertiaOnly (int linkNum, Vec3V_In inertia);

		// PURPOSE: Set the mass and angular inertia of this articulated body part.
		// PARAMS:
		//	mass - the new mass
		//	angInertia - the new angular inertia
		void SetMassAndAngInertia (int linkNum, float mass, const Vector3& angInertia);
		void SetMassAndAngInertia (int linkNum, ScalarV_In mass, Vec3V_In angInertia);
		void SetMassAndAngInertia (int linkNum, Vec4V_In angInertiaXYZmassW);

		// PURPOSE: Find the closest body part that connects the two given body parts.
		// PARAMS:
		//	linkA - the index number of the first body part
		//	linkB - the index number of the second body part
		// RETURN:	the index number for the least common ancestor of the two given links
		int FindLeastCommonAncestor( int linkA, int linkB );

		// PURPOSE: Find the body part index number of 
		//	int GetChildBodyPartIndex (int bodyPartIndex);

		// PURPOSE: Disable the specified body part.
		// PARAMS:
		//	bodyPartIndex - the index number of the body part to disable
		// NOTES:
		//	1.	If the given body part has children, they will also be disabled.
		//	2.	Removed (disabled) body parts can be restored with RestoreRemovedBodyPart or RestoreAllRemovedBodyParts.
		//	void RemoveBodyPart (int bodyPartIndex);

		// PURPOSE:	Restore the given body part.
		// PARAMS:
		//	bodyPartIndex - the index number of the body part to restore
		// RETURN:	true if the given body part was removed and is now restored, false if it did not exist or was already active
		//	bool RestoreRemovedBodyPart (int bodyPartIndex);

		// PURPOSE: Restore all the removed body parts.
		// RETURN:	true if there were any removed body parts to restore, false if there were not
		//	bool RestoreAllRemovedBodyParts ();

		void RejuvenateMatrices ();

		void PrefetchForUpdate() const;

#if __PPU
		void GenerateDmaPlan(sysDmaPlan& dmaPlan);
#endif

#if PHARTICULATED_DEBUG_SERIALIZE
		void DebugSerialize();
#endif

		void InvalidateJointLimits();

		// <COMBINE phJoint1Dof::Copy>
		void Copy (const phArticulatedBody& original);

		pgRef<phArticulatedBodyType> m_Type;

		// The only links with incremental velocity to propagate upwards are along
		//    the branch from m_IncrementalBase to m_IncrementalTop.
		// Further, there is no link *above* m_IncrementalBase that has incremental 
		//		velocity to propagate downwards.
		int m_IncrementalBase;

		// Array of pointers to the links
		phArticulatedBodyPart* m_LinkPtr[phArticulatedBodyType::MAX_NUM_LINKS] ;

		// Array of pointers to the joints
		phJoint* m_JointPtr[phArticulatedBodyType::MAX_NUM_JOINTS];

		// PURPOSE: linear and angular velocity of the body parts
		// NOTES:	This includes both translational and rotational components.
		//			m_Velocity.trans is the linear velocity, and m_Velocity.omega is the angular velocity.
		//			The angular velocity is about the body position, not the center of mass of the body part,
		//			so the 3D linear velocity (m_LinearVelocity) is not m_Velocity.trans.
		atArray<phPhaseSpaceVector> m_PartVelocities;
		atArray<phPhaseSpaceVector> m_VelocitiesToPropUp;

		// PURPOSE: The angular inertia and mass of the body part.
		// NOTES:
		//   The mass is stored in the W component.
		//   The angular inertia vector contains the "moments of inertia" around
		//   the local x, y, and z axes measured from the center of mass of the object.
		//   This requires that the object be oriented such that it's local coordinate
		//   system aligns with the principle axes of rotation of the object.
		atArray<Vec4V> m_AngInertiaXYZmassW;

		// This was added to fix some activation problems we saw in gta4
		// Is set to true in Reset(), which is called on activation, which then disables joint limits for a single frame
		// Fixed occasional bad activation when ragdoll was hit by a car (most often when hit from behind)
		// 
		// Unless this has been fixed by other means, we probably DO want to pull this across to main dev branch
		bool m_ResetLastFrame;

		bool m_RootIsFixed;

#if __ASSERT
		bool m_JointLimitsInvalid;
#endif // __ASSERT

#if PHARTICULATEDBODY_MULTITHREADED_VALDIATION
		bool m_ReadOnlyOnMainThread;
#endif // PHARTICULATEDBODY_MULTITHREADED_VALDIATION

#if CAPTURE_STACK_INVALIDATING_JOINT_LIMITS
		static const int JOINT_LIMITS_STACK_TRACE_SIZE = 32;
		size_t m_JointLimitsStackTrace[JOINT_LIMITS_STACK_TRACE_SIZE];
#endif // CAPTURE_STACK_INVALIDATING_JOINT_LIMITS

		int m_NumVeryDeepLimitDofs;

		static float sm_RagdollMaxLinearSpeed;
		static float sm_RagdollMaxAngularSpeed;
		static float sm_RagdollDampingLinC;
		static float sm_RagdollDampingLinV;
		static float sm_RagdollDampingLinV2;
		static float sm_RagdollDampingAngC;
		static float sm_RagdollDampingAngV;
		static float sm_RagdollDampingAngV2;
		static float sm_EmergencyMinStiffness;
		static bool sm_EffectorDrivingEnabled;
	};


	// ********************************************************************************
	// Inlined member functions for ArticulatedBody
	// ********************************************************************************

	__forceinline void phArticulatedBody::ZeroLinkVelocity(int partIndex)
	{
		m_PartVelocities[partIndex].SetZero();
		m_VelocitiesToPropUp[partIndex].SetZero();
	}

	inline Vec3V_Out phArticulatedBody::GetAngularVelocityNoProp (int partIndex) const
	{
		return m_PartVelocities[partIndex].omega;
	}

	inline Vec3V_Out phArticulatedBody::GetLinearVelocityNoProp (int partIndex) const
	{
		const phPhaseSpaceVector &velocity = m_PartVelocities[partIndex];
		return AddCrossed(velocity.trans, velocity.omega, GetLink(partIndex).m_Matrix.GetCol3());
	}

	inline void phArticulatedBody::SetLinearVelocityNoDownVelAdjust(int partIndex, Vec3V_In linearVelocity)
	{
		phPhaseSpaceVector &velocity = m_PartVelocities[partIndex];
		velocity.trans = SubtractCrossed(linearVelocity, velocity.omega, GetLink(partIndex).m_Matrix.GetCol3());
		VALIDATE_PHYSICS_ASSERT(velocity==velocity);
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.omega),Vec3V(V_FLT_LARGE_6)));
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.trans),Vec3V(V_FLT_LARGE_6)));
	}

	inline void phArticulatedBody::SetAngularVelocityNoDownVelAdjust (int partIndex, Vec3V_In angularVelocity)
	{
		phPhaseSpaceVector &velocity = m_PartVelocities[partIndex];
		Vec3V linearVelocity = AddCrossed(velocity.trans, velocity.omega, GetLink(partIndex).m_Matrix.GetCol3());
		velocity.omega = angularVelocity;
		velocity.trans = SubtractCrossed(linearVelocity, angularVelocity, GetLink(partIndex).m_Matrix.GetCol3());
		VALIDATE_PHYSICS_ASSERT(velocity==velocity);
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.omega),Vec3V(V_FLT_LARGE_6)));
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.trans),Vec3V(V_FLT_LARGE_6)));
	}

	inline void phArticulatedBody::SetVelocitiesNoDownVelAdjust (int partIndex, Vec3V_In linearVelocity, Vec3V_In angularVelocity)
	{
		SetLinearVelocityNoDownVelAdjust(partIndex, linearVelocity);
		SetAngularVelocityNoDownVelAdjust(partIndex, angularVelocity);
	}

	inline void phArticulatedBody::SetVelocityNoDownVelAdjust (int partIndex, const phPhaseSpaceVector& velocity)
	{
		m_PartVelocities[partIndex] = velocity;
		VALIDATE_PHYSICS_ASSERT(m_PartVelocities[partIndex]==m_PartVelocities[partIndex]);
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.omega),Vec3V(V_FLT_LARGE_6)));
		VALIDATE_PHYSICS_ASSERT(IsLessThanAll(Abs(velocity.trans),Vec3V(V_FLT_LARGE_6)));
	}

	inline void phArticulatedBody::SetLinearVelocity (int partIndex, Vec3V_In linearVelocity)
	{
		SetLinearVelocityNoDownVelAdjust(partIndex, linearVelocity);
		ResetDownVel(partIndex);
	}

	inline void phArticulatedBody::SetAngularVelocity (int partIndex, Vec3V_In angularVelocity)
	{
		SetAngularVelocityNoDownVelAdjust(partIndex, angularVelocity);
		ResetDownVel(partIndex);
	}

	inline void phArticulatedBody::SetVelocities (int partIndex, Vec3V_In linearVelocity, Vec3V_In angularVelocity)
	{
		SetVelocitiesNoDownVelAdjust(partIndex, linearVelocity, angularVelocity);
		ResetDownVel(partIndex);
	}

	inline void phArticulatedBody::SetVelocity (int partIndex, const phPhaseSpaceVector& velocity)
	{
		SetVelocityNoDownVelAdjust(partIndex, velocity);
		ResetDownVel(partIndex);
	}

	//////////////////////////////////////////////////////////////////////////
	// phArticulatedBody inline functions
	//////////////////////////////////////////////////////////////////////////


	inline void phArticulatedBodyType::SetNumBodyParts(int numLinks)
	{
		Assert(numLinks <= MAX_NUM_LINKS);
		m_NumLinks = (u8)numLinks;
		m_NumJoints = m_NumLinks-1;
	}

	inline int phArticulatedBody::GetParentNum (int bodyPartIndex) const
	{
		FastAssert(bodyPartIndex>0&&bodyPartIndex<GetNumBodyParts());
		int jointIndex = bodyPartIndex-1;
		return m_Type->m_JointParentIndex[jointIndex];
	}

	inline void phArticulatedBody::PrefetchForUpdate() const
	{
		for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
		{
			PrefetchDC( &GetLink(partIndex) );
		}

		for (int partIndex=0; partIndex<GetNumBodyParts(); partIndex++)
		{
			PrefetchDC( &(GetLink( partIndex ).m_Matrix.GetCol3Intrin128ConstRef()) );
		}
	}

	inline void phArticulatedBody::InvalidateJointLimits()
	{
		ASSERT_ONLY(m_JointLimitsInvalid = true;)

#if CAPTURE_STACK_INVALIDATING_JOINT_LIMITS && !__SPU
		sysStack::CaptureStackTrace(m_JointLimitsStackTrace, JOINT_LIMITS_STACK_TRACE_SIZE);
#endif // CAPTURE_STACK_INVALIDATING_JOINT_LIMITS
	}

	inline void phArticulatedBody::SetBodyPart (int index, phArticulatedBodyPart* bodyPart)
	{
		m_LinkPtr[index] = bodyPart;
	}

	inline phArticulatedBodyPart& phArticulatedBody::GetLink (int linkIndex)
	{
		return *m_LinkPtr[linkIndex];
	}

	inline const phArticulatedBodyPart& phArticulatedBody::GetLink (int linkIndex) const
	{
		return *m_LinkPtr[linkIndex];
	}

	inline phArticulatedBodyPart* const* phArticulatedBody::GetLinkArray()
	{
		return m_LinkPtr;
	}

	__forceinline phJoint& phArticulatedBody::GetJoint (int jointIndex) 
	{
		return *m_JointPtr[jointIndex];
	}

	__forceinline const phJoint& phArticulatedBody::GetJoint (int jointIndex) const
	{ 
		return *m_JointPtr[jointIndex];
	}

	inline phJoint*const* phArticulatedBody::GetJointArray() 
	{ 
		return m_JointPtr; 
	}

#if __SPU
	inline phJoint* phArticulatedBody::GetJointAddr(int jointIndex)
	{
		return &GetJoint(jointIndex);
	}

	inline phArticulatedBodyPart* phArticulatedBody::GetLinkAddr(int linkIndex)
	{
		return &GetLink(linkIndex);
	}

	inline void phArticulatedBody::FixUpJointPtr(int jointIndex, phJoint* jointAddr)
	{
		m_JointPtr[jointIndex] = jointAddr;
	}

	inline void phArticulatedBody::FixUpLinkPtr(int linkIndex, phArticulatedBodyPart* linkAddr)
	{
		m_LinkPtr[linkIndex] = linkAddr;
	}
#endif

	__forceinline void phArticulatedBody::GetJointLimitAxis ( int jointNum, int jointLimitID, Vec3V_InOut axis, Vec3V_InOut position )
	{
		GetJoint(jointNum).GetJointLimitAxis( this, jointLimitID, RC_VECTOR3(axis), RC_VECTOR3(position) );
	}

	inline void phArticulatedBody::GetJointLimitUnitImpulse ( int jointNum, int jointLimitID, 
		phPhaseSpaceVector& unitImpulseSpatial )
	{
		GetJoint(jointNum).GetJointLimitUnitImpulse( jointLimitID, unitImpulseSpatial );
	}

	inline void phArticulatedBody::PrecalcResponsesToJointImpulse( int jointNum, int jointLimitID )
	{
		GetJoint(jointNum).PrecalcResponsesToJointImpulse( this, jointNum, jointLimitID );
	}

	inline float phArticulatedBody::GetPrecalcJointLimitResponse(  int jointNum, int jointLimitID )
	{
		return GetJoint(jointNum).GetPrecalcJointLimitResponse( this, jointNum, jointLimitID );
	}

	inline void phArticulatedBody::ApplyPhaseSpaceImpulse (int linkIndex, Vec3V_In trans, Vec3V_In omega)
	{
		PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
		m_IncrementalBase = ApplyIncrementalImpulse(linkIndex, trans, omega, m_IncrementalBase);
	}

	inline void phArticulatedBody::ApplyImpulse (int link, Vec3V_In impulse, Vec3V_In position)
	{
		PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
		if (IsZeroAll(impulse))
			return;
		m_IncrementalBase = ApplyIncrementalImpulse(link, Cross(position, impulse), impulse, m_IncrementalBase);
	}

	inline void phArticulatedBody::ApplyAngImpulse (int link, Vec::V3Param128 angImpulse)
	{
		PHARTICULATEDBODY_MULTITHREADED_VALDIATION_ONLY(CheckReadOnlyOnMainThread();)
		if (IsZeroAll(RCC_VEC3V(angImpulse)))
			return;
		m_IncrementalBase = ApplyIncrementalImpulse(link, RCC_VEC3V(angImpulse), Vec3V(V_ZERO), m_IncrementalBase);
	}

	inline void phArticulatedBody::ApplyForce (int linkIndex, const Vector3& force, const Vector3& position, Vec::V3Param128 timeStep)
	{
		Vec3V impulse = Scale(RCC_VEC3V(force), ScalarV(timeStep));
		ApplyImpulse(linkIndex, impulse, RCC_VEC3V(position));
	}

	inline void phArticulatedBody::ApplyTorque (int linkIndex, Vec::V3Param128 torque, Vec::V3Param128 timeStep)
	{
		Vec3V angImpulse = Scale(RCC_VEC3V(torque), ScalarV(timeStep));
		ApplyAngImpulse(linkIndex, angImpulse.GetIntrin128ConstRef());
	}

	inline void phArticulatedBody::ApplyJointImpulse(int jointNum, int jointLimitID, ScalarV_In impulse )
	{
		if (IsZeroAll(impulse))
			return;
		GetJoint(jointNum).ApplyJointImpulse( this, jointLimitID, impulse );
	}

	__forceinline Vec3V_Out phArticulatedBody::GetIncrementalJointLimitVelocity( int jointNum, int jointLimitID )
	{
		int childLinkNum = jointNum+1;
		int parentLinkNum = m_Type->m_JointParentIndex[jointNum];
		m_IncrementalBase = PropagateIncrementalToLink(childLinkNum,m_IncrementalBase);

		if (!m_VelocitiesToPropUp[childLinkNum].IsZero())
		{
			PropagateOnceUp( childLinkNum, parentLinkNum );
			m_IncrementalBase = parentLinkNum;
		}

		phJoint& theJoint = GetJoint(jointNum);

		phPhaseSpaceVector jointUnitImpulse;
		theJoint.GetJointLimitUnitImpulse( jointLimitID, jointUnitImpulse );

		phPhaseSpaceVector childVelocity = GetVelocityNoProp(theJoint.GetChildLinkIndex());
		phPhaseSpaceVector parentVelocity = GetVelocityNoProp(theJoint.GetParentLinkIndex());

		Vector3 ret = (jointUnitImpulse^childVelocity) - (jointUnitImpulse^parentVelocity);
		return RCC_VEC3V(ret);
	}

	__forceinline void phArticulatedBody::PredictImpulseEffects( int link, Vec3V_In impulse, Vec3V_In position, Vec3V_InOut linear, Vec3V_InOut angular)
	{
		phPhaseSpaceVector spatialImpulse;
		spatialImpulse.trans = Cross(position, impulse);
		spatialImpulse.omega = impulse;

		phPhaseSpaceVector deltaVelocity;
		phArticulatedBodyPart& theLink = GetLink(link);
		theLink.m_LinkInertiaInverse.Transform( spatialImpulse, deltaVelocity );
		angular = Cross(deltaVelocity.omega, position);
		linear = deltaVelocity.trans;
	}

	inline void phArticulatedBody::SetJoint (int index, phJoint* joint)
	{
		m_JointPtr[index] = joint;
		m_Type->m_JointTypes[index] = joint->GetJointType();
	}

	inline phJoint1Dof& phArticulatedBody::GetJoint1Dof (int jointIndex) 
	{
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::JNT_1DOF);
		return *static_cast<phJoint1Dof*>(&GetJoint(jointIndex));
	}

	inline const phJoint1Dof& phArticulatedBody::GetJoint1Dof (int jointIndex) const 
	{ 
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::JNT_1DOF);
		return *static_cast<const phJoint1Dof*>(&GetJoint(jointIndex)); 
	}

	inline phJoint3Dof& phArticulatedBody::GetJoint3Dof (int jointIndex) 
	{
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::JNT_3DOF);
		return *static_cast<phJoint3Dof*>(&GetJoint(jointIndex));
	}

	inline const phJoint3Dof& phArticulatedBody::GetJoint3Dof (int jointIndex) const 
	{ 
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::JNT_3DOF);
		return *static_cast<const phJoint3Dof*>(&GetJoint(jointIndex)); 
	}

	inline phPrismaticJoint& phArticulatedBody::GetPrismaticJoint (int jointIndex) 
	{
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::PRISM_JNT);
		return *static_cast<phPrismaticJoint*>(&GetJoint(jointIndex));
	}

	inline const phPrismaticJoint& phArticulatedBody::GetPrismaticJoint (int jointIndex) const 
	{ 
		FastAssert(GetJoint(jointIndex).GetJointType()==phJoint::PRISM_JNT);
		return *static_cast<const phPrismaticJoint*>(&GetJoint(jointIndex)); 
	}

	inline float phArticulatedBody::GetJointLimitResponse (int jointIndex, int limitDirectionIndex)
	{
		return GetJoint(jointIndex).GetJointLimitResponse(this, limitDirectionIndex);
	}

	inline ScalarV_Out phArticulatedBody::GetMass(int linkNum) const
	{
		return m_AngInertiaXYZmassW[linkNum].GetW();
	}

	inline Vec3V_Out phArticulatedBody::GetAngInertia(int linkNum) const
	{
		return m_AngInertiaXYZmassW[linkNum].GetXYZ();
	}

	inline Vec4V_Out phArticulatedBody::GetMassAndAngInertia(int linkNum) const
	{
		return m_AngInertiaXYZmassW[linkNum];
	}


	inline void phArticulatedBody::SetMassOnly (int linkNum, float mass)
	{
		m_AngInertiaXYZmassW[linkNum].SetW(ScalarVFromF32(mass));
	}

	inline void phArticulatedBody::SetMassOnly (int linkNum, ScalarV_In mass)
	{
		m_AngInertiaXYZmassW[linkNum].SetW(mass);
	}

	inline void phArticulatedBody::SetAngInertiaOnly (int linkNum, const Vector3& inertia)
	{
		m_AngInertiaXYZmassW[linkNum].SetXYZ(RCC_VEC3V(inertia));
	}

	inline void phArticulatedBody::SetAngInertiaOnly (int linkNum, Vec3V_In inertia)
	{
		m_AngInertiaXYZmassW[linkNum].SetXYZ(inertia);
	}

	inline void phArticulatedBody::SetMassAndAngInertia (int linkNum, float mass, const Vector3& angInertia)
	{
		Vec4V angInertiaXYZmassW;
		angInertiaXYZmassW = RCC_VEC4V(angInertia);
		angInertiaXYZmassW.SetW(mass);
		m_AngInertiaXYZmassW[linkNum] = angInertiaXYZmassW;
	}

	inline void phArticulatedBody::SetMassAndAngInertia (int linkNum, ScalarV_In mass, Vec3V_In angInertia)
	{
		m_AngInertiaXYZmassW[linkNum].SetXYZ(angInertia);
		m_AngInertiaXYZmassW[linkNum].SetW(mass);
	}

	inline void phArticulatedBody::SetMassAndAngInertia (int linkNum, Vec4V_In angInertiaXYZmassW)
	{
		m_AngInertiaXYZmassW[linkNum] = angInertiaXYZmassW;
	}

} // namespace rage

#endif // ARTICULATEDBODY_TYPE_H
