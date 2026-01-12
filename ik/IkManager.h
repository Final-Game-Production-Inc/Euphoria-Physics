// 
// ik/IkManager.h
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef IKMANAGER_H
#define	IKMANAGER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK (well procedural animation really) for peds
//
// CIkManager - Full featured version inherits from CBaseIkManager to be used by peds
//
// Call Init on creation (after the skeleton has been created)
// Call Shutdown once on deletion
//
// This class contains a collection of different "ik" solvers
// Many of the solvers were originally spread all over the codebase
// and were written and maintained by different authors
// This class is an attempt to consolidate them into one place

///////////////////////////////////////////////////////////////////////////////

// Rage headers
#include "crbody/kinematics.h"
#include "paging/ref.h"
#include "vector/vector3.h"

// Game headers
#include "Animation/AnimBones.h"
#include "ik/solvers/ArmIkSolver.h"
#include "IKManagerSolverTypes.h"
#include "crbody/iksolver.h"
#include "Peds/pedDefines.h"

#if __IK_DEV
#include "Animation/debug/AnimDebug.h"
#endif // __IK_DEV

#define IK_DEBUG_VERBOSE (0 && __IK_DEV )
#define NUM_INTERSECTIONS_FOR_PROBES (16)

#define PEDIK_ARMS_FADEIN_TIME	(0.5f)
#define PEDIK_ARMS_FADEOUT_TIME (0.3333f) 

#define PEDIK_ARMS_FADEIN_TIME_QUICK	(0.0666f)
#define PEDIK_ARMS_FADEOUT_TIME_QUICK	(0.1333f)

class CEntity;
class CDynamicEntity;
class CPed;
class CBodyLookIkSolverProxy;
class CBodyLookIkSolver;
class CIkRequestArm;
class CIkRequestResetArm;
class CIkRequestBodyLook;
class CIkRequestBodyReact;
class CIkRequestBodyRecoil;
class CIkRequestLeg;
class CIkRequestTorsoVehicle;

namespace rage
{
	class crClip;
	class crFrame;
}

// IK Part (sync'd to IK_PART in commands_ped.sch)
enum eIkPart
{
	IK_PART_INVALID,
	IK_PART_HEAD,
	IK_PART_SPINE,
	IK_PART_ARM_LEFT,
	IK_PART_ARM_RIGHT,
	IK_PART_LEG_LEFT,
	IK_PART_LEG_RIGHT
};

// LookAt flags
enum eHeadIkFlags
{
	LF_SLOW_TURN_RATE			= (1<<0),	// turn the head slowly
	LF_FAST_TURN_RATE			= (1<<1),	// turn the head quickly

	LF_WIDE_YAW_LIMIT			= (1<<2),	// use the wide hard coded yaw limit
	LF_WIDE_PITCH_LIMIT			= (1<<3),	// use the wide hard coded pitch limit
	
	LF_WIDEST_YAW_LIMIT			= (1<<4),	// use the widest hard coded yaw limit
	LF_WIDEST_PITCH_LIMIT		= (1<<5),	// use the widest hard coded pitch limit

	LF_NARROW_YAW_LIMIT			= (1<<6),	// use the narrow hard coded yaw limit
	LF_NARROW_PITCH_LIMIT		= (1<<7),	// use the narrow hard coded pitch limit
	
	LF_NARROWEST_YAW_LIMIT		= (1<<8),	// use the narrowest hard coded yaw limit
	LF_NARROWEST_PITCH_LIMIT	= (1<<9),	// use the narrowest hard coded pitch limit

	LF_USE_TORSO				= (1<<10),	// NOT ENABLED - Use the torso as well as the head when looking at the target
	LF_WHILE_NOT_IN_FOV			= (1<<11),	// keep tracking the target even if they are not in the hard coded FOV	
	LF_USE_CAMERA_FOCUS			= (1<<12),	// look at the position the camera is looking at
	LF_USE_EYES_ONLY			= (1<<13),	// use eyes only to look at target
	LF_USE_LOOK_DIR				= (1<<14),	// use information in look dir DOF

	LF_FROM_SCRIPT				= (1<<15),	// used to identify this look at as originating from script

	LF_USE_REF_DIR_ABSOLUTE		= (1<<16),	// force absolute reference direction mode

	LF_NUM_FLAGS				= 17,
};

// Leg IK flags
enum eLegIkFlags
{
	// if false the feet will try and connect with the ground and the pelvis will be moved to help
	PEDIK_LEGS_AND_PELVIS_OFF		= (1<<0), 
	
	// if true then the leg Ik will not fade out when off it will snap out
	PEDIK_LEGS_AND_PELVIS_FADE_OFF	= (1<<1), 
	
	// if true then the leg Ik will always fade out for cases where it would normally snap out
	PEDIK_LEGS_AND_PELVIS_FADE_ON	= (1<<2), 
	
    // if the leg ik is enabled it will blend in during ik allow tags (per arm)
	PEDIK_LEGS_USE_ANIM_ALLOW_TAGS	= (1<<5),

	// if the leg ik is enabled it will blend out during ik block tags (per arm)
	PEDIK_LEGS_USE_ANIM_BLOCK_TAGS	= (1<<6),

	// if the leg ik is enabled it will do an instant setup with no blends
	PEDIK_LEGS_INSTANT_SETUP		= (1<<7)
};

// Leg IK modes - kept here due to direct reference by commands_ped.cpp
enum eLegIkModes
{
	LEG_IK_MODE_OFF,		// No leg IK at all
	LEG_IK_MODE_PARTIAL,	// Fixup legs based on standing capsule impacts
	LEG_IK_MODE_FULL,		// Fixup legs using probes for each foot
	LEG_IK_MODE_FULL_MELEE,	// Fixup legs using probes for each foot with melee support

	LEG_IK_MODE_NUM
};

#if __IK_DEV
enum eLookIkReturnCode
{
	LOOKIK_RC_NONE = 0,
	LOOKIK_RC_DISABLED,
	LOOKIK_RC_INVALID_PARAM,
	LOOKIK_RC_CHAR_DEAD,
	LOOKIK_RC_BLOCKED,
	LOOKIK_RC_BLOCKED_BY_ANIM,
	LOOKIK_RC_BLOCKED_BY_TAG,
	LOOKIK_RC_OFFSCREEN,
	LOOKIK_RC_RANGE,
	LOOKIK_RC_FOV,
	LOOKIK_RC_PRIORITY,
	LOOKIK_RC_INVALID_TARGET,
	LOOKIK_RC_TIMED_OUT,
	LOOKIK_RC_ABORT,

	LOOKIK_RC_NUM
};

const char *eLookIkReturnCodeToString(eLookIkReturnCode eReturnCode);
#endif // __IK_DEV

///////////////////////////////////////////////////////////////////////////////
// CBaseIkManager 
// container and manager class for IkSolver component system
// supports the adding and updating of an arbitrary number of
// Ik solvers that can be applied to a ped
///////////////////////////////////////////////////////////////////////////////
class CBaseIkManager
{
public:
	typedef IKManagerSolverTypes::Flags eIkSolverType;

	// Now auto generated from ikmanagersolvertypes.psc into ikmanagersolvertypes.h
	// PURPOSE: Defines the different solver types available
	// NOTE: to create solvers of the new type a case must be added in the AddSolver method of the CBaseIkManager class
	/*enum eIkSolverType
	{
		ikSolverTypeLeg,						// enabled on player
		ikSolverTypePointGunInDirection,		// enabled 
		ikSolverTypeHead,						// disabled / will be re-enabled soon
		ikSolverTypeArm,						// enabled
		ikSolverTypeCount						// used to determine the size of the storage array in IkManager
	};*/

	// PURPOSE:	Entry used for scoring peds for solver allocation.
	struct PedIkPriority
	{
	public:
		// PURPOSE:	Comparison function for sorting
		static bool OrderLessThan(const PedIkPriority& a, const PedIkPriority& b) { return a.m_Order < b.m_Order; }

		CPed*	m_pPed;
		u16		m_Order;
		u16		m_Dirty;
	};

	// PURPOSE: Debug info for retail crashes
	union DebugInfo
	{
		struct
		{
			u8 ownedBy;						// Assuming max 256 peds
			u8 deletedBy;					// Assuming max 256 peds
			u8 deletedType			: 4;
			u8 nonOwnedSolverPre	: 1;
			u8 nonOwnedSolverPost	: 1;
		} info;
		u32 infoAsUInt;
	};

	// PURPOSE: Constructor / destructor
	CBaseIkManager();
	virtual ~CBaseIkManager();

	static void InitClass();

	// PURPOSE: Initialise the ik manager 
	virtual bool Init(CPed *pPed);

	// PURPOSE: Called once every frame immediately before the solvers
	virtual void PreIkUpdate(float UNUSED_PARAM(deltaTime)) {}

	// PURPOSE: Called once every frame immediately after the solvers
	virtual void PostIkUpdate(float UNUSED_PARAM(deltaTime)) {}

	// PURPOSE: Get the number of solvers
	// RETURNS: Number of solvers
	u32 GetNumSolvers() const;

	// PURPOSE: get solver by solver type (const)
	// PARAMS: solverType - the type of solver requested [see IkSolver.h for definition]
	// RETURNS: const pointer to iksolver
	const crIKSolverBase* GetSolver(eIkSolverType solverType) const;

	// PURPOSE: Get solver by solver type (non-const)
	// PARAMS: solverType - the type of solver requested [see IkSolver.h for definition]
	// RETURNS: non-const pointer to iksolver
	crIKSolverBase* GetSolver(eIkSolverType solverType);

	// PURPOSE: Get solver by solver type (non-const), creates the solver if it doesn't already exist
	// PARAMS: solverType - the type of solver requested [see IkSolver.h for definition]
	// RETURNS: non-const pointer to iksolver
	crIKSolverBase* GetOrAddSolver(eIkSolverType solverType);

	// PURPOSE: Enumeration, get solver by index (non-const)
	// PARAMS: solverType - the type of solver to check for [see IkSolver.h for definition]
	// RETURNS: boolean value - true if the ik manager has this solver, false otherwise
	bool HasSolver(eIkSolverType solverType) const;

	// PURPOSE: Add new solver to the Ik manager
	// PARAMS: solverType - The type of solver to add (The Ik manager will take responsibility for object creation and destruction)
	// RETURNS: boolean value - true if the solver was added successfully, false if not (there may already be a solver of that type)
	bool AddSolver(eIkSolverType solverType);

	// PURPOSE: Remove a solver from the Ik manager by solver type
	// PARAMS: solverType - the type of solver to check for [see IkSolver.h for definition]
	// RETURNS: true - if removal successful, false - if removal failed
	bool RemoveSolver(eIkSolverType solverType);

	// PURPOSE: Used to set ped IK flags used for intra solver communication
	// PARAMS: flag: the ped IK flag to set to true
	virtual void SetFlag(u32 flag) {m_flags |= flag;}

	// PURPOSE: Used to set ped IK flags used for intra solver communication
	// PARAMS: flag: the ped IK flag to set to false
	void ClearFlag(u32 flag) {m_flags &= ~flag;}

	// PURPOSE: Used to set ped IK flags used for intra solver communication
	// PARAMS: flag: the ped IK flag to check
	// RETURNS: true if the flag is set, otherwise false
	bool IsFlagSet(u32 flag) {return (m_flags & flag) != 0;}

	// PURPOSE: Return the kinematics object
	crKinematics& GetKinematics() { return m_Kinematics; }

	// PURPOSE: Reconstruct the kinematics with the solvers in the right order
	void RefreshKinematics();

	// PURPOSE: Inits the memory pools for the individual Ik solver classes
	static void InitPools();

	// PURPOSE: Shuts down the memory pools for the individual Ik solver classes
	static void ShutdownPools();

	// PURPOSE: Process solver allocation for running proxies
	static void ProcessSolvers();

	// PURPOSE: Remove completed solvers
	void RemoveCompletedSolvers();

	// PURPOSE: Remove all solvers regardless of completion
	void RemoveAllSolvers();

	// PURPOSE: Tracking information for retail crashes
	static DebugInfo& GetDebugInfo() { return ms_DebugInfo; }
	static u32 GetDebugInfoAsUInt() { return ms_DebugInfo.infoAsUInt; }
	static const CPed* GetPedDeletingSolver() { return ms_pPedDeletingSolver; }
	static void ValidateSolverDeletion(CPed* pPed, eIkSolverType solverType);

#if __IK_DEV
	// PURPOSE: 
	static void DebugRender();

	// PURPOSE: 
	static void DisplayMemory();

	// PURPOSE: 
	static void DisplaySolvers();
#endif //__IK_DEV

#if __IK_DEV || __BANK
	// PURPOSE: Get the debug name of a solver
	static const char* GetSolverName(eIkSolverType solverType);
#endif // __IK_DEV || __BANK

public:
#if __IK_DEV
	// PURPOSE: Debug render helper
	static CDebugRenderer ms_DebugHelper;

	// PURPOSE: Static boolean used to display memory usage
	static bool ms_DisplayMemory;

	// PURPOSE: Static boolean used to display solver usage
	static bool ms_DisplaySolvers;
#endif // __IK_DEV

#if __IK_DEV || __BANK
	// PURPOSE: Static boolean used to disable all solvers
	static bool ms_DisableAllSolvers;

	// PURPOSE: Static booleans to toggle solvers on/off
	static bool ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeCount];
#endif // __IK_DEV || __BANK

protected:
	crKinematics m_Kinematics;
	crIKSolverBase* m_Solvers[IKManagerSolverTypes::ikSolverTypeCount];
	u32 m_flags;
	CPed* m_pPed;

	static u16 ms_SolverPoolSizes[IKManagerSolverTypes::ikSolverTypeCount];
	static const CPed* ms_pPedDeletingSolver;
	static DebugInfo ms_DebugInfo;
};

///////////////////////////////////////////////////////////////////////////////
// CIkManager
// Inherits from CBaseIkManager (above)
///////////////////////////////////////////////////////////////////////////////
class CIkManager : public CBaseIkManager
{
public:

	enum eSolverStatus
	{
		IK_SOLVER_REACHED_YAW		= (1<<0),
		IK_SOLVER_UNREACHABLE_YAW	= (1<<2),
		IK_SOLVER_REACHED_PITCH		= (1<<3),
		IK_SOLVER_UNREACHABLE_PITCH	= (1<<4),
		IK_SOLVER_REACHED_ROLL		= (1<<5),
		IK_SOLVER_UNREACHABLE_ROLL	= (1<<6),
	};

#if FPS_MODE_SUPPORTED
	enum eIkPass
	{
		IK_PASS_MAIN = (1<<0),
		IK_PASS_FPS = (1<<1),
	};
#endif // FPS_MODE_SUPPORTED

	struct HighHeelData
	{
		float m_fHeight;
		float m_fPreExpressionHeight;
		s32 m_RootBoneIdx;
	};

	// Constructor / destructor
	CIkManager() {}

	virtual ~CIkManager();

	// Create the abstract representation of the humanoid body that the ik solvers can use
	bool Init(CPed *pPed);

#if __BANK
	static void InitWidgets();
#endif

	// Override the SetFlag method to enable creation of solvers
	void SetFlag(u32 flag);

	// Called before AI tasks are about to update, useful for resetting flags, etc
	void PreTaskUpdate();

	// Override the PreIkUpdate method to trigger the creation of the handlebar solver when necessary
	void PreIkUpdate(float deltaTime);

	// Override the PostIkUpdate method to do debug rendering
	void PostIkUpdate(float deltaTime);

	void ResetAllSolvers();
	void ResetAllNonLegSolvers();

	bool IsActive(eIkSolverType solverType) const;
	bool IsSupported(eIkSolverType solverType) const;

	// Solver requests
	bool Request(const CIkRequestArm& request FPS_MODE_SUPPORTED_ONLY(, s32 pass = IK_PASS_MAIN | IK_PASS_FPS));
	bool Request(const CIkRequestResetArm& request);
	bool Request(const CIkRequestBodyLook& request FPS_MODE_SUPPORTED_ONLY(, s32 pass = IK_PASS_MAIN | IK_PASS_FPS));
	bool Request(const CIkRequestBodyReact& request);
	bool Request(const CIkRequestBodyRecoil& request);
	bool Request(const CIkRequestLeg& request FPS_MODE_SUPPORTED_ONLY(, s32 pass = IK_PASS_MAIN | IK_PASS_FPS));
	bool Request(const CIkRequestTorsoVehicle& request);

	// Script interface for specifying IK targets
	void SetIkTarget(eIkPart ePart,
					 const CEntity* pTargetEntity,
					 eAnimBoneTag eTargetBoneTag,
					 const Vector3& vTargetOffset,
					 u32 uFlags,
					 int iBlendInTimeMS = -1,
					 int iBlendOutTimeMS = -1,
					 int iPriority = (int)CIkManager::IK_LOOKAT_MEDIUM);

	//////////////////////////////////////////////////////////////////////////
	//
	// Interface methods for the Head look at solver
	//
	//////////////////////////////////////////////////////////////////////////

	// LookAt Priorities
	enum eLookAtPriority
	{
		IK_LOOKAT_VERY_LOW,
		IK_LOOKAT_LOW,
		IK_LOOKAT_MEDIUM,
		IK_LOOKAT_HIGH,
		IK_LOOKAT_VERY_HIGH, 
	};

	// Create an ik task to turn the head toward a target 
	void LookAt(
		u32 hashKey					// hashKey used to identify the ik task
		, const CEntity* pEntity		// target this entity. NULL means no entity
		, s32 time						// time to hold ik pose (in milliseconds)
		, eAnimBoneTag offsetBoneTag	// target this bonetag in the above entity (entity must therefore be a pedestrian) -1 means no boneTag
		, const Vector3* pOffsetPos		// if neither an entity or a bonetag exist this defines a position in world space to look at
		// if just an entity exists this defines an offset w.r.t the entity
		// if an entity and a bonetag exist this defines an offset w.r.t the bone
		, u32 flags = 0					//
		, s32 blendInTime  = 500		// time to blend in/out of ik pose (in milliseconds)
		, s32 blendOutTime = 500		// time to blend in/out of ik pose (in milliseconds)
		, eLookAtPriority priority = IK_LOOKAT_LOW); // used to decide if subsequent look at tasks can interrupt this task


	// Is there an active ik task for the head
	bool IsLooking();
	bool IsLooking(u32 hashKey);

	// Is the target within the FOV of the head
	bool IsTargetWithinFOV(const Vector3& target, const Vector3 &min, const Vector3 &max);

	// Abort the ik task for the head
	void AbortLookAt(s32 blendTime);

	// Abort the ik task for the head if it matches the hashKey
	void AbortLookAt(s32 blendTime, u32 hashKey);

	// Get the entity the head is pointing to
	const CEntity* GetLookAtEntity() const;

	// Get the flags for the head look at
	const u32 GetLookAtFlags() const;

#if __BANK
	// Get the hash key for the head look at
	const u32 GetLookAtHashKey() const;
#endif

	// Get the flags for the head look at
	bool GetLookAtTargetPosition(Vector3& pos);

	void ConvertLookAtFlagsToLookIk(u32 uFlags, CIkRequestBodyLook& request);
	u32 ConvertLookIkToLookAtFlags(const CBodyLookIkSolverProxy* pSolver) const;

	//////////////////////////////////////////////////////////////////////////
	//
	// Interface methods for Torso Ik
	// Turns the torso to face a given target (direction or position)
	// Specifically make spine 3 point toward the target (direction or position)
	// The turn is distributed across spine, spine1, spine2, and spine3
	//
	//////////////////////////////////////////////////////////////////////////
	bool PointGunAtPosition(const Vector3& posn, float fBlendAmount = -1.0f);
	bool PointGunInDirection(float yaw, float pitch, bool bRollPitch = false, float fBlendAmount = -1.0f);
	void GetGunAimAnglesFromPosition(const Vector3 &pos, float &yaw, float &pitch);
	void GetGunAimPositionFromAngles(float yaw, float pitch, Vector3 &pos);
	void GetGunAimDirFromGlobalTarget(const Vector3 &globalTarget, Vector3 &normAimDir);
	void GetGunGlobalTargetFromAimDir(Vector3 &globalTarget, const Vector3 &normAimDir);
	u32 GetTorsoSolverStatus();
	float GetTorsoMinYaw();
	float GetTorsoMaxYaw();
	float GetTorsoMinPitch();
	float GetTorsoMaxPitch();
	float GetTorsoOffsetYaw() const;
	float GetTorsoBlend() const;
	const Vector3& GetTorsoTarget();
	void SetTorsoMinYaw(float minYaw);
	void SetTorsoMaxYaw(float maxYaw);
	void SetTorsoMinPitch(float minPitch);
	void SetTorsoMaxPitch(float maxPitch);
	void SetTorsoOffsetYaw(float torsoOffsetYaw);
	void SetTorsoYawDeltaLimits(float yawDelta, float yawDeltaSmoothRate, float yawDeltaSmoothEase);
	float GetTorsoYaw() const;
	float GetTorsoYawDelta() const;
	float GetTorsoPitch() const;
	void SetDisablePitchFixUp(bool disablePitchFixUp);

	//////////////////////////////////////////////////////////////////////////
	//
	// Interface methods for Root Slope Fixup Ik
	//
	//////////////////////////////////////////////////////////////////////////
	void AddRootSlopeFixup(float fBlendRate);

	//////////////////////////////////////////////////////////////////////////
	//
	// Arm Ik solver
	// If the target is within reach move the hand bone to grab the target
	// otherwise reach toward nearest point to target
	// Reposition the elbow and re-orientate the clavicle and forearm
	//
	//////////////////////////////////////////////////////////////////////////
	
	// Set an absolute (world space) target for the arm ik
	void SetAbsoluteArmIKTarget( crIKSolverArms::eArms arm					// left or right arm
		, const Vector3& targetPos							// target position
		, s32 flags	 = 0	 								// control flags
		, float blendInTime  = PEDIK_ARMS_FADEIN_TIME		// time to blend in (in seconds)
		, float blendOutTime = PEDIK_ARMS_FADEOUT_TIME		// time to blend out (in seconds)
		, float blendInRange = 0.0f							// distance from target to blend in
		, float blendOutRange = 0.0f);						// distance from target to blend out

	// Set a relative (bone space/model space/world space) target for the arm ik
	// target this bone index in the above entity -1 means no bone index
	// if the entity is valid and the bone index is valid this defines a target position w.r.t the bone
	// if the entity is valid but the bone index is invalid this defines a target position w.r.t the entity
	// if the entity is invalid and the bone index is invalid this defines a world space target position
	void SetRelativeArmIKTarget(crIKSolverArms::eArms arm					  // left or right arm
		, const CDynamicEntity* targetEntity				  // See main comment
		, int targetBoneIdx									  // See main comment
		, const Vector3& targetOffset						  // See main comment
		, s32 flags = 0										  // control flags
		, float blendInTime  = PEDIK_ARMS_FADEIN_TIME		  // time to blend in (in seconds)
		, float blendOutTime = PEDIK_ARMS_FADEOUT_TIME		  // time to blend out (in seconds)
		, float blendInRange = 0.0f							  // distance from target to blend in
		, float blendOutRange = 0.0f);						  // distance from target to blend out

#if FPS_MODE_SUPPORTED
	// Set a relative (bone space/model space/world space) target (rotation and offset)
	void SetRelativeArmIKTarget(crIKSolverArms::eArms arm	  // left or right arm
		, const CDynamicEntity* targetEntity				  // See main comment
		, int targetBoneIdx									  // See main comment
		, const Vector3& targetOffset						  // See main comment
		, const Quaternion& targetRotation					  // Rotation component
		, s32 flags = 0										  // control flags
		, float blendInTime  = PEDIK_ARMS_FADEIN_TIME		  // time to blend in (in seconds)
		, float blendOutTime = PEDIK_ARMS_FADEOUT_TIME		  // time to blend out (in seconds)
		, float blendInRange = 0.0f							  // distance from target to blend in
		, float blendOutRange = 0.0f);						  // distance from target to blend out
#endif // FPS_MODE_SUPPORTED

	void AimWeapon(crIKSolverArms::eArms eArm, s32 sFlags = 0);

	//////////////////////////////////////////////////////////////////////////
	//
	// Leg ik
	// Physics capsule moves the character but this may leave the character legs
	// intersecting with other geometry.
	// Send three test probes from the back, middle and front of the foot to see if their 
	// are any intersections. 
	// Try to orientate the foot to align with the surface of the geometry
	// Try to position the foot to not intersect the geometry (moving the root if necessary)
	//
	//////////////////////////////////////////////////////////////////////////
	bool CanUseLegIK(bool& bCanUsePelvis, bool& bCanUseFade);

	// PURPOSE: Utility function used to point one bone in a skeleton in the direction of another bone in the same skeleton
	// PARAMS: boneAIdx - The id of the bone to orient, boneBIdx - The id of the bone to orient towards
	void OrientateBoneATowardBoneB(s32 boneAIdx, s32 boneBIdx);

	// PURPOSE: Override leg blend. I.e. useful for specific tasks (such as fall etc) to turn on IK faster.
	void SetOverrideLegIKBlendTime(float fTime);
	void ResetOverrideLegIKBlendTime();

#if FPS_MODE_SUPPORTED
	// PURPOSE: Externally driven pelvis offset support
	void SetExternallyDrivenPelvisOffset(float offset)	{ m_fExternallyDrivenPelvisOffset = offset; }
	float GetExternallyDrivenPelvisOffset() const		{ return m_fExternallyDrivenPelvisOffset; }

	// PURPOSE: 
	bool IsFirstPersonStealthTransition() const;

	// PURPOSE: 
	bool IsFirstPersonModeTransition() const;
#endif // FPS_MODE_SUPPORTED

	float GetPelvisDeltaZForCamera(bool shouldIgnoreIkControl = false) const;
	float GetSpringStrengthForStairs();

	static u32 GetDefaultLegIkMode(const CPed* pPed);

#if __IK_DEV
	void DebugRenderPed();
#endif //__IK_DEV

#if __IK_DEV
	void SetLookIkReturnCode(eLookIkReturnCode eReturnCode)	{ m_eLookIkReturnCode = eReturnCode; }
	eLookIkReturnCode GetLookIkReturnCode() const			{ return m_eLookIkReturnCode; }
#endif // __IK_DEV

#if FPS_MODE_SUPPORTED

	void CacheInputFrameForFpsUpdatePass(CPed& ped);
	void PerformFpsUpdatePass(CPed& ped);

	const crSkeleton* GetThirdPersonSkeleton() const { return m_pThirdPersonSkeleton; }

#endif // FPS_MODE_SUPPORTED

	void InitHighHeelData();
	void BeginHighHeelData(float deltaTime);
	void EndHighHeelData(float deltaTime);
	float GetHighHeelHeight() const { return m_HighHeelData.m_fHeight; }

private:

#if FPS_MODE_SUPPORTED
	enum eFirstPersonRequests
	{
		kRequestArmRight = 0,
		kRequestArmLeft,
		kRequestLook,
		kRequestLeg,

		kNumRequests
	};

	struct FirstPersonPassRequests
	{
		bool m_hasRequests[kNumRequests];

		CIkRequestArm m_armRequests[CArmIkSolver::NUM_ARMS];
		CIkRequestBodyLook m_lookRequest;
		CIkRequestLeg m_legRequest;

		RegdConstEnt m_pLookTarget;
		bool m_bLookTargetParam;

		void Reset();
	};

	// PURPOSE: Get the FPS request structure. will be lazily initialised if necessary.
	FirstPersonPassRequests& GetFpsRequests();
	FirstPersonPassRequests* m_pFirstPersonRequests;

	// PURPOSE: The skeleton used to store the third person pass skeleton for rendering
	crSkeleton*	m_pThirdPersonSkeleton;

	// PURPOSE: Cache off the matrices prior to the pre-render update for third person (so we can restore them for third person).
	crFrame* m_pThirdPersonInputFrame;

	float m_fExternallyDrivenPelvisOffset;
#endif // FPS_MODE_SUPPORTED

	HighHeelData m_HighHeelData;

#if __IK_DEV
	eLookIkReturnCode m_eLookIkReturnCode;
#endif // __IK_DEV

#if FPS_MODE_SUPPORTED
	float m_fDeltaTime;

	u8 m_bFPSUpdatePass	: 1;
#endif // FPS_MODE_SUPPORTED

	static bool ms_bSampleHighHeelHeight;

}; // CIkManager

#endif // IKMANAGER_H

//////////////////////////////////////////////////////////////////////////////

