// 
// ik/solvers/LegSolver.h
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef LEGSOLVER_H
#define LEGSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
//	Player leg ik:
//	Player physics capsule moves the character but this may leave the character legs
//	intersecting with other geometry
//
//	Leg position based IK:
//	Fire a test capsule down at each foot position to determine the height and orientation of the ground
//	at the current position.
//	Try to orientate the foot to align with the surface of the geometry
//	Try to position the foot to not intersect the geometry (moving the pelvis if necessary)
//	Perform leg IK to correctly orient the knee.
//
//  Derives from CIkSolver base class. Create and add to an existing CBaseIkManager to
//  apply the solver to a ped
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolver.h"

#include "fwtl/pool.h"
#include "phcore/constants.h"
#include "system/timer.h"
#include "vector/vector3.h"

#include "Animation/debug/AnimDebug.h"
#include "Peds/Ped.h"
#include "Task/System/Tuning.h"

namespace rage
{
	class phIntersection;
}

class CDynamicEntity;
class CLegIkSolverProxy;
class CIkRequestLeg;

class CLegIkSolver: public crIKSolverLegs
{
public:

	enum eSupportingFootMode { 
		FURTHEST_BACKWARD_FOOT, FURTHEST_FORWARD_FOOT, 
		HIGHEST_FOOT, LOWEST_FOOT,
		HIGHEST_FOOT_INTERSECTION, LOWEST_FOOT_INTERSECTION,
		MOST_RECENT_LOCKED_Z};

	struct Tunables : CTuning
	{
		Tunables();

		struct InterpolationSettings
		{
			float m_Rate;
			bool m_AccelerationBased;
			bool m_ScaleAccelWithDelta;
			bool m_ZeroRateOnDirectionChange;
			float m_AccelRate;

			float IncrementTowards( float value, float target, float& rate, float deltaTime, float scale = 1.0f );

			PAR_SIMPLE_PARSABLE;
		};

		InterpolationSettings m_PelvisInterp;
		InterpolationSettings m_PelvisInterpMoving;
		InterpolationSettings m_PelvisInterpOnDynamic;
		InterpolationSettings m_FootInterp;
		InterpolationSettings m_FootInterpIntersecting;
		InterpolationSettings m_FootInterpMoving;
		InterpolationSettings m_FootInterpIntersectingMoving;
		InterpolationSettings m_FootInterpOnDynamic;

		InterpolationSettings m_StairsPelvisInterp;
		InterpolationSettings m_StairsPelvisInterpMoving;
		InterpolationSettings m_StairsPelvisInterpCoverAim;
		InterpolationSettings m_StairsFootInterp;
		InterpolationSettings m_StairsFootInterpIntersecting;
		InterpolationSettings m_StairsFootInterpCoverAim;

		float m_VelMagStairsSpringMin;
		float m_VelMagStairsSpringMax;
		float m_StairsSpringMultiplierMin;
		float m_StairsSpringMultiplierMax;

		float m_DownStairsPelvisMaxDeltaZMoving;
		float m_DownStairsPelvisMaxNegativeDeltaZMoving;
		float m_UpStairsPelvisMaxDeltaZMoving;
		float m_UpStairsPelvisMaxNegativeDeltaZMoving;

		float m_StairsPelvisMaxNegativeDeltaZCoverAim;

		PAR_PARSABLE;
	};

	enum eFootPart { HEEL, BALL, NUM_FOOT_PARTS };

	// Temporary working data passed between various worker functions
	struct RootAndPelvisSituation
	{
		// 
		Matrix34 rootGlobalMtx;

		// the height (z) of the intersection with the ground above/below the root
		Vector3 vGroundPosition;

		// the normal of the intersection with the ground above/below the root
		Vector3 vGroundNormal;

		// the height (z) of the root if the floor was flat
		float fAnimZRoot;

		// 
		float fSlopePitch;

		// 
		float fSpeed;

		// true if the probe is intersecting with the ground above/below foot
		bool bIntersection;

		bool bMoving			: 1;
		bool bOnStairs			: 1;
		bool bOnSlope			: 1;
		bool bOnSlopeLoco		: 1;	// slope locomotion in use
		bool bMovingUpwards		: 1;
		bool bCover				: 1;
		bool bLowCover			: 1;
		bool bCoverAim			: 1;
		bool bNearEdge			: 1;
		bool bInVehicle			: 1;	// in a vehicle that requires leg IK
		bool bInVehicleReverse	: 1;
		bool bEnteringVehicle	: 1;
		bool bOnPhysical		: 1;
		bool bOnMovingPhysical	: 1;
		bool bInstantSetup		: 1;
		bool bMelee				: 1;
	};

	// Temporary working data passed between various worker functions
	struct LegSituation
	{
		Matrix34 thighGlobalMtx;
		Matrix34 calfGlobalMtx;
		Matrix34 footGlobalMtx;
		Matrix34 toeGlobalMtx;
		Mat34V toeLocalMtx;

		// the height (z) of the foot if the floor was flat
		float fAnimZFoot;

		// the height (z) of the toe if the floor was flat
		float fAnimZToe;

		// the height (z) of the intersection with the ground above/below the root
		Vector3 vGroundPosition;

		// the normal of the intersection with the ground above/below the root
		Vector3 vGroundNormal;

		// true if the probe is intersecting with the ground above/below foot
		bool bIntersection;

		// true if the probe is intersecting a physical entity
		bool bOnPhysical;
		bool bOnMovingPhysical;

		// true if the leg is considered to be supporting 
		bool bSupporting;

		// true if foot orientation requires fixup
		bool bFixupFootOrientation;
	};

	// Non temporary working data
	struct RootAndPelvisGoal
	{
		Vector3 m_vGroundNormalSmoothed;

		// 0 is fully blended out 1 is fully blended in
		float m_fPelvisBlend;					

#if FPS_MODE_SUPPORTED
		// externally driven pelvis offset
		float m_fExternalPelvisOffset;
		float m_fExternalPelvisOffsetApplied;
#endif // FPS_MODE_SUPPORTED

		// smoothed z height of the pelvis
		float m_fPelvisDeltaZSmoothed;
		float m_fPelvisDeltaZ;

		float m_fNearEdgePelvisInterpScaleBlend;
		float m_fOnSlopeLocoBlend;

		// Bone indices cached on initialization for speed
		u16 m_uPelvisBoneIdx;

		// Enable disable the pelvis fixup
		bool m_bEnablePelvis;
	};

	// Non temporary working data
	struct LegGoal
	{
		// the orientation of the ground under the foot (the orientation we wish the supporting foot to match)
		Vector3 m_vGroundNormalSmoothed;	

		// the position of the ground under the foot (the position we wish the supporting foot to match)
		Vector3 m_vGroundPositionSmoothed;

		// We only update the x and y co-ordinates of the probe(s) to a tolerance to
		// help reduce any hysteresis in the results due to tiny movements in foot position
		Vector3 m_vProbePosition[NUM_FOOT_PARTS];

		// Track previous foot part position/velocity to help with async probe prediction
		Vector3 m_vPrevPosition[NUM_FOOT_PARTS];
		Vector3 m_vPrevVelocity[NUM_FOOT_PARTS];

		// 0 is fully blended out 1 is fully blended in
		float m_fLegBlend;					

		// 0 is fully blended out 1 is fully blended in
		float m_fFootOrientationBlend;	

		// smoothed z height of the foot
		float m_fFootDeltaZSmoothed;
		float m_fFootDeltaZ;

		// The foot height can be optionally locked to a particular z coord
		// this can be useful for avoiding foot slipping when planted.
		bool m_bLockFootZ;
		bool m_bLockedFootZCoordValid;
		float m_fLockedFootZCoord;

		// The locked foot angle when on slopes and the foot is planted
		float m_fFootAngleLocked;
		float m_fFootAngleSmoothed;

		// True if the foot was flattened due to the calculated foot angle being too large from the shape test results
		bool bFootFlattened;
	};

	struct ShapeTestData
	{
		ShapeTestData(bool bAllocate = true);
		~ShapeTestData();

		void Allocate();
		void Free();
		void Reset();

		WorldProbe::CShapeTestResults* m_apResults[CLegIkSolver::NUM_LEGS][CLegIkSolver::NUM_FOOT_PARTS];
		bool m_bOwnResults;
	};

public:

	//constructor and destructor
	CLegIkSolver(CPed* pPed, CLegIkSolverProxy* pProxy);
	~CLegIkSolver();

	FW_REGISTER_CLASS_POOL(CLegIkSolver);

	//	PURPOSE:
	virtual void Iterate(float dt, crSkeleton& skel);

	// PURPOSE:
	void PreIkUpdate(float deltaTime);

	// PURPOSE:
	void Update(float deltaTime, bool bMainThread);

	// PURPOSE:
	void PostIkUpdate(float deltaTime);

	// PURPOSE: 
	virtual void Reset();

	void PartialReset();

	// PURPOSE:
	void Request(const CIkRequestLeg& request);

	//	PURPOSE:
	float GetPelvisDeltaZForCamera(bool shouldIgnoreIkControl = false) const { return (shouldIgnoreIkControl || m_bIKControlsPedPos) ? (m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed * m_rootAndPelvisGoal.m_fPelvisBlend) : 0.0f; }
	
	float GetMaxPelvisDeltaZ(RootAndPelvisSituation& sit);
	float GetMaxAllowedPelvisDeltaZ();

	float GetMinPelvisDeltaZ(RootAndPelvisSituation& sit);
	float GetMinAllowedPelvisDeltaZ(const RootAndPelvisSituation& rootAndPelvisSituation);

	// PURPOSE: 
	float GetLegBlend(eLeg leg) const { return m_LegGoalArray[leg].m_fLegBlend; }

	// PURPOSE: Return the capsule spring strength to be used
	float GetSpringStrengthForStairs();
	
	// PURPOSE: 
	float GetPelvisBlend() const { return m_rootAndPelvisGoal.m_fPelvisBlend; }

#if FPS_MODE_SUPPORTED
	// PURPOSE: 
	bool IsFirstPersonStealthTransition() const { return m_bFPSStealthToggled; }

	// PURPOSE: 
	bool IsFirstPersonModeTransition() const { return m_bFPSModeToggled; }
#endif // FPS_MODE_SUPPORTED

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	void DebugRenderLowerBody(const crSkeleton &skeleton, Color32 sphereColor, Color32 lineColor);
	static void AddWidgets(bkBank *pBank);
	static void ToggleRenderPeds();
#endif //__IK_DEV

private:

	// PURPOSE: Helper method - to calculate the collision normal and height at a particular world position
	// PARAMS:	vStartPos	  - The start position in the world to perform the capsule test
	//			vEndPos		  - The end position in the world to perform the capsule test
	//			vGroundNormal - The normal of the intersection with the ground
	//			vGroundWorldZ - The z coord of the intersection with the ground
	void CalcGroundIntersection(const Vector3& vStartPos, const Vector3& vEndPos, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation);

	// PURPOSE: Helper method - to calculate the collision normal and height at a particular world position
	// PARAMS:	vStartPos	  - The start position in the world to perform the capsule test
	//			vEndPos		  - The end position in the world to perform the capsule test
	//			vGroundNormal - The normal of the intersection with the ground
	//			vGroundWorldZ - The z coord of the intersection with the ground
	//void CalcRootGroundIntersection(const Vector3& vStartPos, const Vector3& vEndPos, RootAndPelvisSituation &rootAndPelvisSituation);

	// PURPOSE: Helper method - to perform capsule tests - see CalculateGroundIntersection()
	bool CapsuleTest(const Vector3 &vStart, const Vector3 &vFinish, float fRadius, int iShapeTestIncludeFlags, WorldProbe::CShapeTestResults& refResults);

	//	PURPOSE: Helper method - 
	bool ProbeTest(const Vector3 &posA,const Vector3 &posB, const u32 iLineTestFlags);

	//	PURPOSE: Helper method - 
	void CalcSmoothedNormal(const Vector3 &vDesiredNormal, const Vector3 &vCurrentNormal, const float fSmoothRate, Vector3 &vSmoothedNormal, float fDeltaTime, const bool bForceDesired = false);

	//	PURPOSE: Helper method - 
	void ClampNormal(Vector3& vNormal);

	//	PURPOSE: Helper method - 
	void CalcHeightOfBindPoseFootAboveCollision(const crSkeleton& skeleton);

	// PURPOSE: Helper method - restores the specified child bone to its bind pose transformation relative to the parent bone
	void RestoreBoneToZeroPoseRelativeToParent(int iChildBoneIdx, int iParentBoneIdx);

	// PURPOSE: Helper method - 
	float GetDistanceBetweenBones(int iBoneIdx1, int iBoneIdx2);
	
	// PURPOSE: Helper method -
	void WorkOutRootAndPelvisSituationUsingPedCapsule(RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime);
	
	// PURPOSE: Helper method -
	//void WorkOutRootAndPelvisSituationUsingProbe(RootAndPelvisSituation& rootAndPelvisSituation);

	// PURPOSE: Helper method -
	void WorkOutLegSituationUsingPedCapsule(crIKSolverLegs::Goal& solverGoal, LegGoal& legGoal, LegSituation& legSituation, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime);

	// PURPOSE: Helper method -
	void WorkOutLegSituationUsingShapeTests(crIKSolverLegs::Goal& solverGoal, LegGoal& legGoal, LegSituation& legSituation, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime);

	// PURPOSE: Helper method -
	u32 GetShapeTestIncludeFlags() const;
	void DeleteShapeTestData();
	bool CanUseAsyncShapeTests(RootAndPelvisSituation& rootAndPelvisSituation) const;
	void SetupData(const crSkeleton& skeleton, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation);
	void SubmitShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data);
	void SubmitAsyncShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data, float deltaTime);
	void PreProcessResults(LegGoal* legGoalArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data);
	void ProcessResults(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data, float deltaTime);
	void WorkOutLegSituationsUsingMultipleShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime);

	// PURPOSE: Updates the blend factors for the legs and pelvis
	bool UpdateBlends(bool bBlendOut, bool bLeftFootOnFloor, bool bRightFootOnFloor, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime);

#if FPS_MODE_SUPPORTED
	// PURPOSE:
	void UpdateFirstPersonState(float deltaTime);
#endif // FPS_MODE_SUPPORTED

	// PURPOSE: Return true from this method to block leg IK from being applied
	bool BlockLegIK();

	// PURPOSE:
	void FixupLegsAndPelvis(float deltaTime);
	
	// PURPOSE:
	bool FixupPelvis(RootAndPelvisSituation &rootAndPelvisSituation, LegSituation &leftLegSituation, LegSituation &rightLegSituation, float deltaTime);

	// PURPOSE:
	void FixupLeg(crIKSolverLegs::Goal& solverGoal, LegGoal& legGoal, LegSituation& legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime);

	// PURPOSE:
	void SetLeg(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime);

	// PURPOSE:
	void FixupFootOrientation(crIKSolverLegs::Goal& solverGoal, LegGoal& legGoal, LegSituation& legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime);

	// PURPOSE:
	void FixupFootHeight(LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime);

	// PURPOSE:
	void ApplySupportingFootMode( eSupportingFootMode supportingFootMode,
		LegSituation &leftLegSituation, LegSituation &rightLegSituation,
		LegGoal& leftLegGoal, LegGoal& rightLegGoal);

	// PURPOSE:
	bool UseMainThread() const;

	// PURPOSE:
	u32 GetMode() const;

	// PURPOSE:
	float GetFootRadius(RootAndPelvisSituation& rootAndPelvisSituation) const;

	// PURPOSE:
	float GetFootMaxPositiveDeltaZ(RootAndPelvisSituation& rootAndPelvisSituation) const; 

	// PURPOSE:
	float GetMinDistanceFromFootToRoot(RootAndPelvisSituation& rootAndPelvisSituation) const;

	// PURPOSE:
	float GetHeightOfAnimRootAboveCollision(const RootAndPelvisSituation& rootAndPelvisSituation, const LegSituation& supportingLegSituation) const;

public:

	//////////////////////////////////////////////////////////////////////////
	//	Statics
	//////////////////////////////////////////////////////////////////////////

	static Tunables sm_Tunables;

	// Apply the pelvis height fixup?
	static bool	ms_bPelvisFixup;
	
	// Apply the legs fixup?
	static bool ms_bLegFixup;

	// Apply the foot orientation fixup?
	static bool	ms_bFootOrientationFixup;

	// Apply the foot height fixup?
	static bool	ms_bFootHeightFixup;

	// Controls the blend of the foot delta
	static float ms_fAnimFootZBlend;

	// Controls the blend of the pelvis delta
	static float ms_fAnimPelvisZBlend;

	// Controls the rate we increment towards the desired solver blend
	static float ms_afSolverBlendRate[2];

	// Controls the rate we increment towards the desired foot delta
	static float ms_fFootDeltaZRate;

	// Controls the rate we increment towards the desired foot delta when intersecting the ground
	static float ms_fFootDeltaZRateIntersecting;

	// Controls the rate we increment towards the desired pelvis delta
	static float ms_fPelvisDeltaZRate;

	// Controls the rate we increment towards the desired ground normal
	static float ms_fGroundNormalRate;

	// Controls the rate we increment towards the desired foot orientation
	static float ms_fFootOrientationBlendRate;

	// Enforce a minimum height from the foot to the root 
	static float ms_fMinDistanceFromFootToRoot;

	// Enforce a minimum height from the foot to the root for cover
	static float ms_fMinDistanceFromFootToRootForCover;

	// Enforce a minimum height from the foot to the root for landing
	static float ms_fMinDistanceFromFootToRootForLanding;

	// Allow only positive foot delta z
	static bool ms_bPositiveFootDeltaZOnly;

	// The magnitude of the velocity at which we consider we ourselves to be moving
	static float ms_fMovingTolerance;

	// The + or - pitch at which we consider ourselves to be on a slope
	static float ms_fOnSlopeTolerance; 

	// Difference in intersection height that intersections are considered to be on separate steps
	static float ms_fStairHeightTolerance;

	// The amount the x and y co-ordinates of the foot must move before we relocate the probe
	// This helps reduce hysteresis in the results due to tiny movements in foot position
	static float ms_fProbePositionDeltaTolerance;

	static bool	ms_bClampFootOrientation;
	static float ms_fFootOrientationPitchClampMin;
	static float ms_fFootOrientationPitchClampMax;

	// During ground intersection calculation intersections with normals less than this 
	// tolerance are not considered (intention is to disregard walls etc)
	static float ms_fNormalVerticalThreshold;

	//	PURPOSE:
	static float ms_fPelvisMaxNegativeDeltaZStanding;
	static float ms_fPelvisMaxNegativeDeltaZMoving;
	static float ms_fPelvisMaxNegativeDeltaZMovingNearEdge;
	static float ms_fPelvisMaxNegativeDeltaZVehicle;
	static float ms_fPelvisMaxNegativeDeltaZStairs;
	static float ms_fPelvisMaxNegativeDeltaZOnDynamicEntity;
	static float ms_fPelvisMaxPositiveDeltaZStanding;
	static float ms_fPelvisMaxPositiveDeltaZMoving;
	static float ms_fPelvisMaxPositiveDeltaZMovingNearEdge;

	static float ms_fNearEdgePelvisInterpScale;
	static float ms_fNearEdgePelvisInterpScaleRate;
	static float ms_fOnSlopeLocoBlendRate;

	static float ms_afFootMaxPositiveDeltaZ[3];					// [Default|Physical|Melee,Slope]

	//	PURPOSE:
	static float ms_fRollbackEpsilon;
	static bool	ms_bClampPostiveAnimZ;

	//	PURPOSE:
	static int ms_nNormalMovementSupportingLegMode;
	static int ms_nUpSlopeSupportingLegMode;
	static int ms_nDownSlopeSupportingLegMode;
	static int ms_nUpStairsSupportingLegMode;
	static int ms_nDownStairsSupportingLegMode;
	static int ms_nNormalStandingSupportingLegMode;
	static int ms_nLowCoverSupportingLegMode;

	//	PURPOSE:
	static bool ms_bUseMultipleShapeTests;
	static bool ms_bUseMultipleAsyncShapeTests;
	static float ms_fFootAngleSlope;
	static float ms_afFootAngleClamp[2];						// [In|Out]
	static float ms_fVehicleToeGndPosTolerance;
	static float ms_afFootRadius[2];							// [No Stairs|Stairs]

#if __IK_DEV
	//	PURPOSE:
	static bool ms_bForceLegIk;
	static int ms_nForceLegIkMode;

	//	PURPOSE:
	static bool ms_bRenderSkin;
	static bool	ms_bRenderSweptProbes;
	static bool	ms_bRenderProbeIntersections;
	static bool	ms_bRenderLineIntersections;
	static bool	ms_bRenderSituations;
	static bool	ms_bRenderSupporting;
	static bool	ms_bRenderInputNormal;
	static bool	ms_bRenderSmoothedNormal;
	static bool	ms_bRenderBeforeIK;
	static bool	ms_bRenderAfterPelvis;
	static bool	ms_bRenderAfterIK;

	//	PURPOSE:
	static float ms_fXOffset;
	static float ms_debugSphereScale;	
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
	static CDebugRenderer ms_debugHelper;

	//	PURPOSE:
	static sysTimer ms_perfTimer;
#endif //__IK_DEV

private:
	//	PURPOSE:
	CPed* m_pPed;

	//	PURPOSE:
	CLegIkSolverProxy* m_pProxy;

	//	PURPOSE:
	ShapeTestData* m_pShapeTestData;

	//	PURPOSE:
	RootAndPelvisGoal m_rootAndPelvisGoal;

	//	PURPOSE:
	LegGoal m_LegGoalArray[NUM_LEGS];

	//	PURPOSE:
	float m_afHeightOfBindPoseFootAboveCollision[NUM_FOOT_PARTS];

#if FPS_MODE_SUPPORTED
	//	PURPOSE: Cached delta time when solver is run in multiple passes
	float m_fDeltaTime;
#endif // FPS_MODE_SUPPORTED

	int m_LastLockedZ;

	u8 m_bActive				: 1;
	u8 m_bIKControlsPedPos		: 1;
	u8 m_bBonesValid			: 1;
	u8 m_bForceMainThread		: 1;
	u8 m_bOnStairSlope			: 1;
#if FPS_MODE_SUPPORTED
	u8 m_bFPSMode				: 1;
	u8 m_bFPSModePrevious		: 1;
	u8 m_bFPSStealthToggled		: 1;
	u8 m_bFPSModeToggled		: 1;
#endif // FPS_MODE_SUPPORTED
};


#endif // LEGSOLVER_H
