// 
// ik/solvers/LegSolver.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#include "LegSolver.h"

#include "diag/art_channel.h"
#include "fwdebug/picker.h"

#include "animation/anim_channel.h"
#include "animation/AnimBones.h"
#include "animation/debug/AnimViewer.h"
#include "animation/MovePed.h"
#include "game/modelindices.h"
#include "ik/IkManager.h"
#include "ik/solvers/LegSolverProxy.h"
#include "modelinfo/PedModelInfo.h"
#include "Peds/ped.h"
#include "Peds/PedCapsule.h"
#include "Peds/PedIntelligence.h"
#include "Physics/Physics.h"
#include "debug/DebugScene.h"
#include "Task/Combat/Cover/TaskCover.h"
#include "Task/Vehicle/TaskInVehicle.h"
#include "Task/Vehicle/TaskExitVehicle.h"

#define LEG_SOLVER_POOL_MAX			10
#define ENABLE_LEGIK_SOLVER_WIDGETS	0

// Instantiate the class memory pool
FW_INSTANTIATE_CLASS_POOL(CLegIkSolver, LEG_SOLVER_POOL_MAX, atHashString("CLegIkSolver",0x40cd8e90));

// Tunable parameters. ///////////////////////////////////////////////////
CLegIkSolver::Tunables CLegIkSolver::sm_Tunables;
IMPLEMENT_IK_SOLVER_TUNABLES(CLegIkSolver, 0x40cd8e90);
//////////////////////////////////////////////////////////////////////////

ANIM_OPTIMISATIONS();

#define IK_SPU (__PS3 && 0)

//PRAGMA_OPTIMIZE_OFF()

#if IK_SPU
#include "ikspu.h"
bool g_IkSpu = true;
DECLARE_TASK_INTERFACE(ikspu);
#endif

bool CLegIkSolver::ms_bPelvisFixup = true;
bool CLegIkSolver::ms_bLegFixup = true;
bool CLegIkSolver::ms_bFootOrientationFixup = true;
bool CLegIkSolver::ms_bFootHeightFixup = true;

float CLegIkSolver::ms_fAnimPelvisZBlend = 1.0f;
float CLegIkSolver::ms_fAnimFootZBlend = 0.25f;

float CLegIkSolver::ms_afSolverBlendRate[2] = { 5.0f, 1.0f };
float CLegIkSolver::ms_fFootDeltaZRate = 0.75f;
float CLegIkSolver::ms_fFootDeltaZRateIntersecting = 2.0f;
float CLegIkSolver::ms_fPelvisDeltaZRate = 0.5f;
float CLegIkSolver::ms_fGroundNormalRate = 5.0f;
float CLegIkSolver::ms_fFootOrientationBlendRate = 14.0f;

float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZStanding = -0.37f;
float CLegIkSolver::ms_fPelvisMaxPositiveDeltaZStanding = 0.37f;
float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZMoving	= 0.0f;
float CLegIkSolver::ms_fPelvisMaxPositiveDeltaZMoving	= 0.0f;
float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZMovingNearEdge = -0.18f;
float CLegIkSolver::ms_fPelvisMaxPositiveDeltaZMovingNearEdge = 0.18f;
float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZVehicle	= -0.60f;
float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZStairs	= -0.52f;
float CLegIkSolver::ms_fPelvisMaxNegativeDeltaZOnDynamicEntity = -0.60f;

float CLegIkSolver::ms_fNearEdgePelvisInterpScale = 1.08f;
float CLegIkSolver::ms_fNearEdgePelvisInterpScaleRate = 5.0f;
float CLegIkSolver::ms_fOnSlopeLocoBlendRate = 7.0f;

float CLegIkSolver::ms_afFootMaxPositiveDeltaZ[3] = { 0.34f, 0.38f, 0.52f };

float CLegIkSolver::ms_fMinDistanceFromFootToRoot = 0.152f;
float CLegIkSolver::ms_fMinDistanceFromFootToRootForCover = 0.02f;
float CLegIkSolver::ms_fMinDistanceFromFootToRootForLanding = 0.25f;
float CLegIkSolver::ms_fRollbackEpsilon = 0.0f;

float CLegIkSolver::ms_fMovingTolerance = 1.0f;
float CLegIkSolver::ms_fOnSlopeTolerance = 0.1f;
float CLegIkSolver::ms_fStairHeightTolerance = 0.05f;
float CLegIkSolver::ms_fProbePositionDeltaTolerance = 0.01f;
float CLegIkSolver::ms_fNormalVerticalThreshold = 0.1f;

int CLegIkSolver::ms_nNormalMovementSupportingLegMode = CLegIkSolver::FURTHEST_FORWARD_FOOT;
int CLegIkSolver::ms_nUpSlopeSupportingLegMode = CLegIkSolver::FURTHEST_FORWARD_FOOT;
int CLegIkSolver::ms_nDownSlopeSupportingLegMode = CLegIkSolver::FURTHEST_FORWARD_FOOT;
int CLegIkSolver::ms_nUpStairsSupportingLegMode = CLegIkSolver::FURTHEST_FORWARD_FOOT;
int CLegIkSolver::ms_nDownStairsSupportingLegMode = CLegIkSolver::FURTHEST_FORWARD_FOOT;
int CLegIkSolver::ms_nNormalStandingSupportingLegMode = CLegIkSolver::LOWEST_FOOT_INTERSECTION;
int CLegIkSolver::ms_nLowCoverSupportingLegMode = CLegIkSolver::HIGHEST_FOOT_INTERSECTION;

bool CLegIkSolver::ms_bClampFootOrientation = true;
float CLegIkSolver::ms_fFootOrientationPitchClampMin = 0.5;
float CLegIkSolver::ms_fFootOrientationPitchClampMax = 2.2f;

bool CLegIkSolver::ms_bClampPostiveAnimZ = false;	
bool CLegIkSolver::ms_bPositiveFootDeltaZOnly = true;

bool CLegIkSolver::ms_bUseMultipleShapeTests = true;
bool CLegIkSolver::ms_bUseMultipleAsyncShapeTests = true;
float CLegIkSolver::ms_fFootAngleSlope = -15.0f * DtoR;
float CLegIkSolver::ms_afFootAngleClamp[2] = { 38.0f * DtoR, 32.0f * DtoR };
float CLegIkSolver::ms_fVehicleToeGndPosTolerance = 0.085f;
float CLegIkSolver::ms_afFootRadius[2] = { 0.07f, 0.04f };

#if __IK_DEV
bool	CLegIkSolver::ms_bForceLegIk = false;
int		CLegIkSolver::ms_nForceLegIkMode = 0;

bool	CLegIkSolver::ms_bRenderSkin = true;
bool	CLegIkSolver::ms_bRenderSweptProbes = false;
bool	CLegIkSolver::ms_bRenderProbeIntersections = true;
bool	CLegIkSolver::ms_bRenderLineIntersections = false;
bool	CLegIkSolver::ms_bRenderSituations = false;
bool	CLegIkSolver::ms_bRenderSupporting= false;
bool	CLegIkSolver::ms_bRenderInputNormal = false;
bool	CLegIkSolver::ms_bRenderSmoothedNormal = false;
bool	CLegIkSolver::ms_bRenderBeforeIK = true;
bool	CLegIkSolver::ms_bRenderAfterPelvis = false; 
bool	CLegIkSolver::ms_bRenderAfterIK = true; 

float	CLegIkSolver::ms_fXOffset = -0.5f;
float	CLegIkSolver::ms_debugSphereScale = 0.02f;
bool	CLegIkSolver::ms_bRenderDebug = false;
int		CLegIkSolver::ms_iNoOfTexts = 0;
CDebugRenderer CLegIkSolver::ms_debugHelper;

sysTimer CLegIkSolver::ms_perfTimer;
#endif //__IK_DEV


//////////////////////////////////////////////////////////////////////////

inline float IncrementTowards(float from, float to, float amount)
{
	float delta = to-from;
	ikAssert(amount>=0.0f);
	if (fabs(delta)<amount)
	{
		return to;
	}
	else
	{
		return delta < 0.0f ? from-amount : from+amount;
	}

}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::Tunables::InterpolationSettings::IncrementTowards(float from, float to, float& rate, float deltaTime, float scale)
{
	float delta = to-from;
	float amount = 0.f;
	
	if (m_AccelerationBased)
	{
		if (delta==0.0f)
		{
			// set the rate back to zero
			rate=0.0f;
		}
		else
		{
			if (m_ScaleAccelWithDelta)
			{
				float accel = m_AccelRate*abs(delta);
				rate += delta<0.0f ? -accel : accel;
			}
			else
			{
				rate += delta<0.0f ? -m_AccelRate : m_AccelRate;
			}
		}

		rate = Clamp(rate, -m_Rate, m_Rate);

		if (m_ZeroRateOnDirectionChange)
		{
			if (
				(rate<0.0f && delta>0.0f)
				|| (rate>0.0 && delta<0.0f)
				)
			{
				rate=0.0f;
			}
		}

		amount = abs(rate);
	}
	else
	{
		amount = m_Rate;
		rate = 0.0f;
	}

	amount*=scale;
	amount*=deltaTime;

	if (fabs(delta)<amount)
	{
		return to;
	}
	else
	{
		return delta < 0.0f ? from-amount : from+amount;
	}
}

//////////////////////////////////////////////////////////////////////////

CLegIkSolver::ShapeTestData::ShapeTestData(bool bAllocate)
: m_bOwnResults(bAllocate)
{
	if (bAllocate)
	{
		Allocate();
	}
	else
	{
		for (int legPart = 0; legPart < CLegIkSolver::NUM_LEGS; ++legPart)
		{
			for (int footPart = 0; footPart < CLegIkSolver::NUM_FOOT_PARTS; ++footPart)
			{
				m_apResults[legPart][footPart] = NULL;
			}
		}
	}
}

CLegIkSolver::ShapeTestData::~ShapeTestData()
{
	if (m_bOwnResults)
	{
		Reset();
		Free();
	}
}

void CLegIkSolver::ShapeTestData::Allocate()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CLegIkSolver::NUM_LEGS; ++legPart)
	{
		for (int footPart = 0; footPart < CLegIkSolver::NUM_FOOT_PARTS; ++footPart)
		{
			m_apResults[legPart][footPart] = rage_new WorldProbe::CShapeTestResults(NUM_INTERSECTIONS_FOR_PROBES);
		}
	}
}

void CLegIkSolver::ShapeTestData::Free()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CLegIkSolver::NUM_LEGS; ++legPart)
	{
		for (int footPart = 0; footPart < CLegIkSolver::NUM_FOOT_PARTS; ++footPart)
		{
			if (m_apResults[legPart][footPart])
			{
				delete m_apResults[legPart][footPart];
				m_apResults[legPart][footPart] = NULL;
			}
		}
	}
}

void CLegIkSolver::ShapeTestData::Reset()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CLegIkSolver::NUM_LEGS; ++legPart)
	{
		for (int footPart = 0; footPart < CLegIkSolver::NUM_FOOT_PARTS; ++footPart)
		{
			if (m_apResults[legPart][footPart])
			{
				m_apResults[legPart][footPart]->Reset();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CLegIkSolver::CLegIkSolver(CPed* pPed, CLegIkSolverProxy* pProxy)
: crIKSolverLegs()
, m_pPed(pPed)
, m_pProxy(pProxy)
, m_pShapeTestData(NULL)
#if FPS_MODE_SUPPORTED
, m_fDeltaTime(0.0f)
#endif // FPS_MODE_SUPPORTED
, m_LastLockedZ(0)
{
	static const eAnimBoneTag legBoneTags[NUM_LEGS][NUM_LEG_PARTS] = 
	{
		{ BONETAG_L_THIGH, BONETAG_L_CALF, BONETAG_L_FOOT, BONETAG_L_TOE, BONETAG_L_PH_FOOT, BONETAG_L_THIGHROLL },
		{ BONETAG_R_THIGH, BONETAG_R_CALF, BONETAG_R_FOOT, BONETAG_R_TOE, BONETAG_R_PH_FOOT, BONETAG_R_THIGHROLL }
	};

	m_bBonesValid = true;

	crSkeleton& skeleton = *m_pPed->GetSkeleton();

	for (int i = 0; i < NUM_LEGS; ++i)
	{
		crIKSolverLegs::Goal& solverGoal = GetGoal(i);

		solverGoal.m_BoneIdx[crIKSolverLegs::THIGH]		= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][THIGH]);
		solverGoal.m_BoneIdx[crIKSolverLegs::CALF]		= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][CALF]);
		solverGoal.m_BoneIdx[crIKSolverLegs::FOOT]		= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][FOOT]);
		solverGoal.m_BoneIdx[crIKSolverLegs::TOE]		= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][TOE]);
		solverGoal.m_BoneIdx[crIKSolverLegs::PH]		= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][PH]);
		solverGoal.m_BoneIdx[crIKSolverLegs::THIGHROLL] = (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[i][THIGHROLL]);

		if (solverGoal.m_BoneIdx[crIKSolverLegs::FOOT] != (u16)-1)
		{
			solverGoal.m_TerminatingIdx = (u16)skeleton.GetTerminatingPartialBone(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT]);
		}

		m_bBonesValid &= (solverGoal.m_BoneIdx[crIKSolverLegs::THIGH]	  != (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverLegs::CALF]	  != (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverLegs::FOOT]	  != (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverLegs::TOE]		  != (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverLegs::PH]		  != (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverLegs::THIGHROLL] != (u16)-1) &&
						 (solverGoal.m_TerminatingIdx != (u16)-1);

		artAssertf(m_bBonesValid, "Modelname = (%s) is missing bones necessary for leg ik. Expects the following bones: "
								  "BONETAG_L/R_THIGH, BONETAG_L/R_CALF, BONETAG_L/R_FOOT, BONETAG_L/R_TOE, BONETAG_L/R_PH_FOOT, BONETAG_L/R_THIGHROLL, Foot Termination", pPed ? pPed->GetModelName() : "Unknown model");
	}

	CalcHeightOfBindPoseFootAboveCollision(skeleton);

	SetPrority(6);
	Reset();
	m_rootAndPelvisGoal.m_bEnablePelvis = true;
}

CLegIkSolver::~CLegIkSolver()
{
	DeleteShapeTestData();
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::PreIkUpdate(float deltaTime)
{
	// Called every frame, except when paused, except when offscreen, except when low lod
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeLeg] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		ms_debugHelper.Reset();
		ms_iNoOfTexts = 0;

		char debugText[100];
		sprintf(debugText, "%s\n", "LegIKSolver::PreIkUpdate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //_DEV

	// Reset
	for (int i = 0; i < NUM_LEGS; i++)
	{
		GetGoal(i).m_Enabled = false;
	}

	// Force solver onto main thread if necessary
	m_bForceMainThread = UseMainThread();

#if FPS_MODE_SUPPORTED
	UpdateFirstPersonState(deltaTime);
#endif // FPS_MODE_SUPPORTED

	// Main thread update
	Update(deltaTime, true);
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::Update(float deltaTime, bool bMainThread)
{
	// Early out if any required bones are missing
	if (!m_bBonesValid)
	{
		m_pProxy->SetComplete(true);
		return;
	}

	const bool bMotionTreeThread = (GetMode() == LEG_IK_MODE_PARTIAL) && !m_bForceMainThread;

	if (bMainThread && bMotionTreeThread)
	{
		// Delete allocated shape test data on main thread if solver will
		// be running on motion tree thread since the shape test data is not needed
		// and any pending shape tests will be aborted
		DeleteShapeTestData();
	}

	// Partial mode solution runs on motion tree thread while full mode runs on main thread
	if (!(bMainThread ^ bMotionTreeThread))
	{
		return;
	}

	if( fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior ||  m_pPed->GetUsingRagdoll() || m_pPed->GetDisableLegSolver() )
	{
		// If we go into ragdoll reset the solver
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableLegSolver())
		{
			Reset();
			m_pProxy->SetComplete(true);
		}
		return;
	}
	else
	{
		m_bIKControlsPedPos = false;
		FixupLegsAndPelvis(deltaTime);
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		m_bActive = false;

		// Only clear on the final pass if running multiple update passes
		m_pPed->GetIkManager().ClearFlag(PEDIK_LEGS_INSTANT_SETUP);

#if __IK_DEV
		// Render the after ik bones position and orientation as lines to the right of the character
		if(CaptureDebugRender() && ms_bRenderAfterIK)
		{
			crSkeleton* pSkeleton = m_pPed->GetSkeleton();
			if (pSkeleton)
			{
				DebugRenderLowerBody(*pSkeleton, Color_red1, Color_red4);
			}
		}
#endif //__IK_DEV
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::Iterate(float dt, crSkeleton& UNUSED_PARAM(skel))
{
	// Called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeLeg] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	//char debugText[100];
	//if (CaptureDebugRender())
	//{
	//	sprintf(debugText, "%s\n", "LegIKSolver::Iterate);
	//	ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	//}
#endif //_DEV

	// Motiontree thread update
	Update(dt, false);
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::Reset()
{
	m_bActive = false;
	m_bIKControlsPedPos = false;
	m_bForceMainThread = false;
	m_bOnStairSlope = false;
#if FPS_MODE_SUPPORTED
	m_bFPSMode = m_pPed->IsFirstPersonShooterModeEnabledForPlayer(false);
	m_bFPSModePrevious = m_bFPSMode;
	m_bFPSStealthToggled = false;
	m_bFPSModeToggled = false;
#endif // FPS_MODE_SUPPORTED

	m_rootAndPelvisGoal.m_vGroundNormalSmoothed = g_UnitUp;
	m_rootAndPelvisGoal.m_fPelvisBlend = 0.0f;
#if FPS_MODE_SUPPORTED
	m_rootAndPelvisGoal.m_fExternalPelvisOffset = 0.0f;
	m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied = 0.0f;
#endif // FPS_MODE_SUPPORTED
	m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = 0.0f;
	m_rootAndPelvisGoal.m_fPelvisDeltaZ = 0.0f;
	m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend = 0.0f;
	m_rootAndPelvisGoal.m_fOnSlopeLocoBlend = 0.0f;

	for (int i = 0; i < NUM_LEGS; ++i)
	{
		crIKSolverLegs::Goal& solverGoal = GetGoal(i);
		solverGoal.m_Enabled = false;

		LegGoal& leg = m_LegGoalArray[i];
		leg.m_fLegBlend = 0.0f;
		leg.m_fFootOrientationBlend = 0.0f;
		leg.m_fFootDeltaZSmoothed = 0.0f;
		leg.m_fFootDeltaZ=0.0f;
		leg.m_vGroundPositionSmoothed.Set(0.0f,0.0f,0.0f);
		leg.m_vGroundNormalSmoothed.Set(0.0f,0.0f,1.0f);

		leg.m_bLockFootZ = false;
		leg.m_bLockedFootZCoordValid = false;
		leg.m_fLockedFootZCoord = 0.0f;

		leg.m_fFootAngleLocked = 0.0f;
		leg.m_fFootAngleSmoothed = 0.0f;

		leg.bFootFlattened = false;
	}

	PartialReset();

	Vec3V vMinLimit(-PI, -PI, ms_fFootOrientationPitchClampMin); // default is 0.586502f
	Vec3V vMaxLimit( PI,  PI, ms_fFootOrientationPitchClampMax); // default is 2.086502f
	SetLimits(vMinLimit, vMaxLimit);
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::PartialReset()
{
	crSkeleton* skeleton = m_pPed->GetSkeleton();
	if( !skeleton )
		return;
	Mat34V mtxGlobal;

	for (int i = 0; i < NUM_LEGS; ++i)
	{
		crIKSolverLegs::Goal& solverGoal = GetGoal(i);
		LegGoal& leg = m_LegGoalArray[i];

		for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
		{
			skeleton->GetGlobalMtx(solverGoal.m_BoneIdx[(footPart == 0) ? crIKSolverLegs::FOOT : crIKSolverLegs::TOE], mtxGlobal);

			Vector3 vPosition(VEC3V_TO_VECTOR3(mtxGlobal.GetCol3()));
			leg.m_vProbePosition[footPart].Set(vPosition);
			leg.m_vPrevPosition[footPart].Set(vPosition);
			leg.m_vPrevVelocity[footPart].Zero();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::Request(const CIkRequestLeg& UNUSED_PARAM(request))
{
	m_bActive = true;
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::WorkOutLegSituationUsingPedCapsule(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation& legSituation, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime)
{
	ikAssert(m_pPed);
	crSkeleton* pSkel = m_pPed->GetSkeleton();
	if (!ikVerifyf(pSkel, "No valid skeleton!"))
		return;

	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], RC_MAT34V(legSituation.thighGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(legSituation.calfGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(legSituation.footGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(legSituation.toeGlobalMtx));
	legSituation.toeLocalMtx = pSkel->GetLocalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE]);

	legSituation.fAnimZFoot = legSituation.footGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;
	legSituation.fAnimZToe = legSituation.toeGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;

	legSituation.vGroundNormal = rootAndPelvisSituation.vGroundNormal;

	legSituation.bFixupFootOrientation = !rootAndPelvisSituation.bOnStairs && !legSituation.vGroundNormal.IsZero();
	legSituation.bOnPhysical = false;
	legSituation.bOnMovingPhysical = false;

	// For two points on a plane a, b and the plane normal N we know N.a = N.b
	// Therefore take a as the ground pos from the ped's capsule and b as the foot position
	// then solve for b.z
	// => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
	legSituation.vGroundPosition.z = legSituation.vGroundNormal.Dot(rootAndPelvisSituation.vGroundPosition) - legSituation.vGroundNormal.x*legSituation.footGlobalMtx.d.x - legSituation.vGroundNormal.y*legSituation.footGlobalMtx.d.y;
	legSituation.vGroundPosition.z = (legSituation.vGroundNormal.z != 0.0f) ? legSituation.vGroundPosition.z / legSituation.vGroundNormal.z : rootAndPelvisSituation.vGroundPosition.z;
	// Use x and y position from the foot
	legSituation.vGroundPosition.x = legSituation.footGlobalMtx.d.x;
	legSituation.vGroundPosition.y = legSituation.footGlobalMtx.d.y;
	
	CalcSmoothedNormal(legSituation.vGroundNormal, legGoal.m_vGroundNormalSmoothed, ms_fGroundNormalRate, legGoal.m_vGroundNormalSmoothed, deltaTime, rootAndPelvisSituation.bInstantSetup);
	legSituation.bIntersection = rootAndPelvisSituation.bIntersection;

	// Keep in sync in case stairs are encountered and solver switches to async shape test mode
	legGoal.m_vPrevPosition[HEEL] = legSituation.footGlobalMtx.d;
	legGoal.m_vPrevPosition[BALL] = legSituation.toeGlobalMtx.d;
	legGoal.m_vPrevVelocity[HEEL].Zero();
	legGoal.m_vPrevVelocity[BALL].Zero();
}


//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::WorkOutLegSituationUsingShapeTests(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation& legSituation, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime)
{
	ikAssert(m_pPed);
	crSkeleton* pSkel = m_pPed->GetSkeleton();
	if (!ikVerifyf(pSkel, "No valid skeleton!"))
		return;
	
	Matrix34 rootMtx;
	pSkel->GetGlobalMtx(0, RC_MAT34V(rootMtx));

	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], RC_MAT34V(legSituation.thighGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(legSituation.calfGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(legSituation.footGlobalMtx));
	pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(legSituation.toeGlobalMtx));

	legSituation.fAnimZFoot = legSituation.footGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;
	legSituation.fAnimZToe = legSituation.toeGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;

	legSituation.bFixupFootOrientation = false;

#if __IK_DEV
	if(CaptureDebugRender() && ms_bRenderProbeIntersections)
	{
		// Render a pink line from the foot matrix to the foot matrix projected to the floor
		Vector3 vFoot(legSituation.footGlobalMtx.d);
		vFoot.z = rootAndPelvisSituation.vGroundPosition.z;
		ms_debugHelper.Line(legSituation.footGlobalMtx.d, vFoot, Color_pink);

		// Render a pink line from the toe matrix to the toe matrix projected to the floor
		Vector3 vToe(legSituation.toeGlobalMtx.d);
		vToe.z = rootAndPelvisSituation.vGroundPosition.z;
		ms_debugHelper.Line(legSituation.toeGlobalMtx.d, vToe, Color_pink);
	}
#endif //__IK_DEV

	// Determine the height above and below the foot for the probe
	float sfAbove = GetMaxAllowedPelvisDeltaZ();
	float sfBelow = fabsf(GetMinAllowedPelvisDeltaZ(rootAndPelvisSituation));
	Vector3 vAbove(0.0f, 0.0f, sfAbove);
	Vector3 vBelow(0.0f, 0.0f, sfBelow);

	// First do a test to check if the foot is intersecting something.
	// If so, we only want to test upwards for a surface to stand on

	// Test a line from the foot's actual position to the root position adjusted to the height of the foot
	if(ProbeTest(Vector3(rootMtx.d.x, rootMtx.d.y, legSituation.footGlobalMtx.d.z), legSituation.footGlobalMtx.d, GetShapeTestIncludeFlags()))
	{
		// foot is intersecting scenery (perhaps a wall if you are running) so only look upwards
		vBelow.z = 0.0f;

#if __IK_DEV
		if (CaptureDebugRender())
		{
			ms_debugHelper.Text3d(legSituation.footGlobalMtx.d, Color_red1, 0, 0, "OnlyUp");
		}
#endif //__IK_DEV
	}

	// Calculate a start position for the line halfway between the foot and the toe
	Vector3 vCentre = (legSituation.footGlobalMtx.d + legSituation.toeGlobalMtx.d) * 0.5f;
	vCentre.z = legSituation.footGlobalMtx.d.z;

	// This code snaps the x and y co-ordinates of the probe to a tolerance
	// This helps reduce hysteresis in the results due to tiny movements in foot position
	if (fabsf(legGoal.m_vProbePosition[0].x - vCentre.x) > ms_fProbePositionDeltaTolerance)
	{
		legGoal.m_vProbePosition[0].x = vCentre.x;
	}
	if (fabsf(legGoal.m_vProbePosition[0].y - vCentre.y) > ms_fProbePositionDeltaTolerance)
	{
		legGoal.m_vProbePosition[0].y = vCentre.y;
	}
	//if (fabsf(leg.m_hysterisProbePosition.z - vCentre.z) > ms_fProbePositionDeltaTolerance)
	{
		legGoal.m_vProbePosition[0].z = vCentre.z;
	}

	vCentre = legGoal.m_vProbePosition[0];

	Vector3 vStartOfProbe(vCentre + vAbove);
	Vector3 vEndOfProbe(vCentre - vBelow);	
	CalcGroundIntersection(vStartOfProbe, vEndOfProbe, legSituation, rootAndPelvisSituation);
	CalcSmoothedNormal(legSituation.vGroundNormal, legGoal.m_vGroundNormalSmoothed, ms_fGroundNormalRate, legGoal.m_vGroundNormalSmoothed, deltaTime, rootAndPelvisSituation.bInstantSetup);
}

//////////////////////////////////////////////////////////////////////////

u32 CLegIkSolver::GetShapeTestIncludeFlags() const
{
	// Physics archetypes against which to test foot intersections.
	// Switched off for vehicles *unless* the ped is actually standing on a vehicle.
	// This avoids issues when walking up against cars with low bumpers.
	u32 uShapeTestIncludeFlags = ArchetypeFlags::GTA_MAP_TYPE_MOVER;

	const CPhysical* pPhysical = m_pPed->GetGroundPhysical() ? m_pPed->GetGroundPhysical() : m_pPed->GetClimbPhysical();

	if (pPhysical)
	{
		if (pPhysical->GetIsTypeVehicle())
		{
			uShapeTestIncludeFlags |= ArchetypeFlags::GTA_VEHICLE_TYPE;
		}
		else if (pPhysical->GetIsTypeObject())
		{
			const CObject* pObject = (const CObject*)pPhysical;

			uShapeTestIncludeFlags |= ArchetypeFlags::GTA_OBJECT_TYPE;

			if (pObject->m_nObjectFlags.bVehiclePart)
			{
				// Include glass component
				uShapeTestIncludeFlags |= ArchetypeFlags::GTA_VEHICLE_NON_BVH_TYPE;
			}
		}
		else if (pPhysical->GetIsTypePed())
		{
#if ENABLE_HORSE
			if (static_cast<const CPed*>(pPhysical)->GetHorseComponent() != NULL)
			{
				uShapeTestIncludeFlags |= ArchetypeFlags::GTA_HORSE_RAGDOLL_TYPE;
			}
			else
#endif
			{
				uShapeTestIncludeFlags |= ArchetypeFlags::GTA_RAGDOLL_TYPE;
			}
		}
	}
	else if (m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsGoingToStandOnExitedVehicle) || m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsVaulting))
	{
		uShapeTestIncludeFlags |= ArchetypeFlags::GTA_VEHICLE_TYPE;
	}

	if (GetMode() == LEG_IK_MODE_FULL_MELEE)
	{
		uShapeTestIncludeFlags |= ArchetypeFlags::GTA_RAGDOLL_TYPE;
	}

	return uShapeTestIncludeFlags;
}

void CLegIkSolver::DeleteShapeTestData()
{
	if (m_pShapeTestData)
	{
		delete m_pShapeTestData;
		m_pShapeTestData = NULL;
	}
}

bool CLegIkSolver::CanUseAsyncShapeTests(RootAndPelvisSituation& rootAndPelvisSituation) const
{
	if (GetMode() == LEG_IK_MODE_PARTIAL)
	{
		// Always use async shape tests if in partial mode since async shape tests will only be used for stairs
		return true;
	}

	const CPhysical* pPhysical = m_pPed->GetGroundPhysical() ? m_pPed->GetGroundPhysical() : m_pPed->GetClimbPhysical();

	if (pPhysical)
	{
		if (pPhysical->GetIsInWater() || pPhysical->GetWasInWater() || (pPhysical->GetVelocity().Mag2() > 0.0f))
		{
			// Use sync shape tests if on a moving physical
			return false;
		}
	}

	if (rootAndPelvisSituation.bLowCover && rootAndPelvisSituation.bMoving)
	{
		// Feet accelerate/decelerate too quickly in the low cover loco anims so cannot predict good probe positions
		return false;
	}

	return true;
}

void CLegIkSolver::SetupData(const crSkeleton& skeleton, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation)
{
	ikAssert(m_pPed);
	ikAssert(legSituationArray);

	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		crIKSolverLegs::Goal& solverGoal = GetGoal(legPart);
		LegSituation& legSituation = legSituationArray[legPart];

		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], RC_MAT34V(legSituation.thighGlobalMtx));
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(legSituation.calfGlobalMtx));
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(legSituation.footGlobalMtx));
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(legSituation.toeGlobalMtx));
		legSituation.toeLocalMtx = skeleton.GetLocalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE]);

		legSituation.fAnimZFoot = legSituation.footGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;
		legSituation.fAnimZToe = legSituation.toeGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;

		legSituation.vGroundPosition = Vector3(legSituation.footGlobalMtx.d.x, legSituation.footGlobalMtx.d.y, m_pPed->GetTransform().GetPosition().GetZf() - m_pPed->GetCapsuleInfo()->GetGroundToRootOffset());
		legSituation.vGroundNormal.Set(0.0f, 0.0f, 1.0f);

		legSituation.bIntersection = false;
		legSituation.bSupporting = false;
		legSituation.bFixupFootOrientation = false;
		legSituation.bOnPhysical = false;
		legSituation.bOnMovingPhysical = false;

	#if __IK_DEV
		if(CaptureDebugRender() && ms_bRenderProbeIntersections)
		{
			// Render a pink line from the foot matrix to the foot matrix projected to the floor
			Vector3 vFoot(legSituation.footGlobalMtx.d.x, legSituation.footGlobalMtx.d.y, rootAndPelvisSituation.vGroundPosition.z);
			ms_debugHelper.Line(legSituation.footGlobalMtx.d, vFoot, Color_pink);

			// Render a pink line from the toe matrix to the toe matrix projected to the floor
			Vector3 vToe(legSituation.toeGlobalMtx.d.x, legSituation.toeGlobalMtx.d.y, rootAndPelvisSituation.vGroundPosition.z);
			ms_debugHelper.Line(legSituation.toeGlobalMtx.d, vToe, Color_pink);
		}
	#endif //__IK_DEV
	}
}

void CLegIkSolver::SubmitShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data)
{
	ikAssert(m_pPed);
	ikAssert(legGoalArray);
	ikAssert(legSituationArray);

	// Determine the height above and below the foot for the probe
	const u32 uShapeTestIncludeFlags = GetShapeTestIncludeFlags();
	const Vector3 vAbove(0.0f, 0.0f, GetMaxAllowedPelvisDeltaZ());
	const float fMinAllowedPelvisDeltaZ = GetMinAllowedPelvisDeltaZ(rootAndPelvisSituation);

	Vector3 avBelow[NUM_LEGS];
	Vector3 vCentre;
	Vector3 vProbeStart(rootAndPelvisSituation.rootGlobalMtx.d);

	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		LegSituation& legSituation = legSituationArray[legPart];
		avBelow[legPart].Set(0.0f, 0.0f, fMinAllowedPelvisDeltaZ);

		// Do intersecting probe tests if not on a dynamic entity, since the dynamic entity motion can be noisy enough (eg. boats)
		// that it's better to keep finding intersections all along the capsule.
		if (!rootAndPelvisSituation.bOnPhysical)
		{
			// Test a line from the foot's actual position to the root position adjusted to the height of the foot
			vProbeStart.z = legSituation.footGlobalMtx.d.z;

			if (ProbeTest(vProbeStart, legSituation.footGlobalMtx.d, uShapeTestIncludeFlags))
			{
				// Foot is intersecting scenery (perhaps a wall if you are running) so only look upwards
				avBelow[legPart].z = 0.0f;
			}
		}
	}

	// Batch required capsule tests
	WorldProbe::CShapeTestBatchDesc batchDesc;
	batchDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	batchDesc.SetIncludeFlags(uShapeTestIncludeFlags);
	batchDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	ALLOC_AND_SET_CAPSULE_DESCRIPTORS(batchDesc,NUM_LEGS*NUM_FOOT_PARTS);

	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;

	const float fFootRadius = GetFootRadius(rootAndPelvisSituation);

	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		LegGoal& legGoal = legGoalArray[legPart];
		LegSituation& legSituation = legSituationArray[legPart];

		const Matrix34* apGlobalMtx[NUM_FOOT_PARTS] = { &legSituation.footGlobalMtx, &legSituation.toeGlobalMtx };

		for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
		{
			const Matrix34& footPartGlobalMtx = *apGlobalMtx[footPart];

			// This code snaps the x and y co-ordinates of the foot probes to a tolerance
			// This helps reduce hysteresis in the results due to tiny movements in foot position
			if (fabsf(legGoal.m_vProbePosition[footPart].x - footPartGlobalMtx.d.x) > ms_fProbePositionDeltaTolerance)
			{
				legGoal.m_vProbePosition[footPart].x = footPartGlobalMtx.d.x;
			}
			if (fabsf(legGoal.m_vProbePosition[footPart].y - footPartGlobalMtx.d.y) > ms_fProbePositionDeltaTolerance)
			{
				legGoal.m_vProbePosition[footPart].y = footPartGlobalMtx.d.y;
			}
			{
				legGoal.m_vProbePosition[footPart].z = footPartGlobalMtx.d.z;
			}

			// Construct and add capsule test
			capsuleDesc.SetResultsStructure(data.m_apResults[legPart][footPart]);
			capsuleDesc.SetCapsule(legGoal.m_vProbePosition[footPart] + vAbove, legGoal.m_vProbePosition[footPart] + avBelow[legPart], fFootRadius);
			capsuleDesc.SetIsDirected(true);
			batchDesc.AddCapsuleTest(capsuleDesc);
		}
	}

	// Submit
	IK_DEV_ONLY(ms_perfTimer.Reset();)
	WorldProbe::GetShapeTestManager()->SubmitTest(batchDesc);
#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[32];
		sprintf(debugText, "Shape tests = %6.2fus\n", ms_perfTimer.GetUsTime());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV
}

void CLegIkSolver::SubmitAsyncShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data, float deltaTime)
{
	ikAssert(m_pPed);
	ikAssert(legGoalArray);
	ikAssert(legSituationArray);

	const float fFootRadius = GetFootRadius(rootAndPelvisSituation);
	const u32 uShapeTestIncludeFlags = GetShapeTestIncludeFlags();

	// Reset shape test data
	data.Reset();

	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;
	capsuleDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	capsuleDesc.SetIncludeFlags(uShapeTestIncludeFlags);
	capsuleDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	capsuleDesc.SetIsDirected(true);

	// Determine the height above and below the foot for the probe
	const Vector3 vAbove(0.0f, 0.0f, GetMaxAllowedPelvisDeltaZ());
	const float fMinAllowedPelvisDeltaZ = GetMinAllowedPelvisDeltaZ(rootAndPelvisSituation);

	Vector3 avBelow[NUM_LEGS];
	Vector3 vCentre;
	Vector3 vProbeStart(rootAndPelvisSituation.rootGlobalMtx.d);

	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		LegSituation& legSituation = legSituationArray[legPart];
		avBelow[legPart].Set(0.0f, 0.0f, fMinAllowedPelvisDeltaZ);

		// Do intersecting probe tests if not on a dynamic entity, since the dynamic entity motion can be noisy enough (eg. boats)
		// that it's better to keep finding intersections all along the capsule.
		if (!rootAndPelvisSituation.bOnPhysical)
		{
			// Test a line from the foot's actual position to the root position adjusted to the height of the foot
			vProbeStart.z = legSituation.footGlobalMtx.d.z;

			if (ProbeTest(vProbeStart, legSituation.footGlobalMtx.d, uShapeTestIncludeFlags))
			{
				// Foot is intersecting scenery (perhaps a wall if you are running) so only look upwards
				avBelow[legPart].z = 0.0f;
			}
		}
	}

	// Velocity direction
	Vector3 vVelocity(m_pPed->GetVelocity());
	vVelocity.SetZ(0.0f);
	vVelocity.NormalizeSafe();

	// Always updating probe position if character has any velocity (moving, turning while idle, etc.) to get smoother
	// probe results on detailed slopes and help reduce foot jitter
	const float fProbePositionDeltaTolerance = (rootAndPelvisSituation.fSpeed > 0.0f) ? 0.0f : ms_fProbePositionDeltaTolerance;

	// Submit heel tests first, followed by toe tests in case not all tests are ready next update
	for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
	{
		for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
		{
			LegGoal& legGoal = legGoalArray[legPart];
			LegSituation& legSituation = legSituationArray[legPart];

			const Matrix34& footPartGlobalMtx = (footPart == HEEL) ? legSituation.footGlobalMtx : legSituation.toeGlobalMtx;

			// This code snaps the x and y co-ordinates of the foot probes to a tolerance
			// This helps reduce hysteresis in the results due to tiny movements in foot position
			if (fabsf(legGoal.m_vProbePosition[footPart].x - footPartGlobalMtx.d.x) > fProbePositionDeltaTolerance)
			{
				legGoal.m_vProbePosition[footPart].x = footPartGlobalMtx.d.x;
			}
			if (fabsf(legGoal.m_vProbePosition[footPart].y - footPartGlobalMtx.d.y) > fProbePositionDeltaTolerance)
			{
				legGoal.m_vProbePosition[footPart].y = footPartGlobalMtx.d.y;
			}
			{
				legGoal.m_vProbePosition[footPart].z = footPartGlobalMtx.d.z;
			}

			Vector3 vFootVelocity(Vector3::ZeroType);
			if (deltaTime > 0.0f)
			{
				vFootVelocity = (footPartGlobalMtx.d - legGoal.m_vPrevPosition[footPart]) / deltaTime;
			}
			legGoal.m_vPrevPosition[footPart] = footPartGlobalMtx.d;

			Vector3 vFootAcceleration(Vector3::ZeroType);
			if (deltaTime > 0.0f)
			{
				vFootAcceleration = (vFootVelocity - legGoal.m_vPrevVelocity[footPart]) / deltaTime;
			}
			legGoal.m_vPrevVelocity[footPart] = vFootVelocity;

			if (rootAndPelvisSituation.bMoving)
			{
				// Project foot velocity onto character's velocity
				vFootVelocity = vVelocity * vFootVelocity.Dot(vVelocity);

				// Project foot acceleration onto character's velocity
				vFootAcceleration = vVelocity * vFootAcceleration.Dot(vVelocity);

				// Extrapolate probe position
				legGoal.m_vProbePosition[footPart] += (vFootVelocity * deltaTime) + (vFootAcceleration * (0.5f * square(deltaTime)));
			}

			capsuleDesc.SetResultsStructure(data.m_apResults[legPart][footPart]);
			capsuleDesc.SetCapsule(legGoal.m_vProbePosition[footPart] + vAbove, legGoal.m_vProbePosition[footPart] + avBelow[legPart], fFootRadius);

			WorldProbe::GetShapeTestManager()->SubmitTest(capsuleDesc, WorldProbe::PERFORM_ASYNCHRONOUS_TEST);
		}
	}
}

void CLegIkSolver::PreProcessResults(LegGoal* legGoalArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data)
{
	ikAssert(m_pPed);
	ikAssert(legGoalArray);

	Vector3 vAbove(Vector3::ZeroType);
	Vector3 vBelow(Vector3::ZeroType);

	const float fFootRadius = GetFootRadius(rootAndPelvisSituation);
	u32 uShapeTestIncludeFlags = 0;

	if (rootAndPelvisSituation.bInstantSetup)
	{
		// Reset since results are no longer valid if doing an instant setup this frame
		data.Reset();
	}

	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
		{
			WorldProbe::CShapeTestResults& capsuleResultsFootPart = *data.m_apResults[legPart][footPart];

			// Check if results are not ready yet. Typically this is when the async probe system is under heavy load (eg. large AI fire fights), so submit a sync test instead
			if (capsuleResultsFootPart.GetWaitingOnResults() || (capsuleResultsFootPart.GetResultsStatus() == WorldProbe::TEST_NOT_SUBMITTED))
			{
				// Cancel current shape test if necessary
				capsuleResultsFootPart.Reset();

				// Issue sync shape test
				LegGoal& legGoal = legGoalArray[legPart];

				if (vAbove.z == 0.0f)
				{
					vAbove.z = GetMaxAllowedPelvisDeltaZ();
					vBelow.z = GetMinAllowedPelvisDeltaZ(rootAndPelvisSituation);
				}

				if (uShapeTestIncludeFlags == 0)
				{
					uShapeTestIncludeFlags = GetShapeTestIncludeFlags();
				}

				CapsuleTest(legGoal.m_vProbePosition[footPart] + vAbove, legGoal.m_vProbePosition[footPart] + vBelow, fFootRadius, (int)uShapeTestIncludeFlags, capsuleResultsFootPart);
			}
		}
	}
}

void CLegIkSolver::ProcessResults(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, ShapeTestData& data, float deltaTime)
{
	ikAssert(m_pPed);
	ikAssert(legGoalArray);
	ikAssert(legSituationArray);

	const bool bOnFoot = !m_pPed->GetIsInVehicle() && !m_pPed->GetIsOnMount();
	const bool bOnSlope = rootAndPelvisSituation.bOnSlope && !rootAndPelvisSituation.bNearEdge;
	const bool bValidateIntersectionHeight = bOnFoot;
	const float fFootMaxPositiveDeltaZ = GetFootMaxPositiveDeltaZ(rootAndPelvisSituation);

	// Results
	for (int legPart = 0; legPart < NUM_LEGS; ++legPart)
	{
		LegGoal& legGoal = legGoalArray[legPart];
		LegSituation& legSituation = legSituationArray[legPart];

		const Matrix34* apGlobalMtx[NUM_FOOT_PARTS] = { &legSituation.footGlobalMtx, &legSituation.toeGlobalMtx };

		// T value goes from 0.0f at start of shape test to 1.0f at end
		// The lowest t value will give us the highest intersection point
		int anHighestIntersectionIndex[NUM_FOOT_PARTS] = { -1, -1 };
		Vector3 avHighestIntersectionPosition[NUM_FOOT_PARTS] = { legSituation.footGlobalMtx.d, legSituation.toeGlobalMtx.d };

		for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
		{
			WorldProbe::CShapeTestResults& capsuleResultsFootPart = *data.m_apResults[legPart][footPart];

			if (capsuleResultsFootPart.GetResultsReady() && (capsuleResultsFootPart.GetNumHits() > 0))
			{
				const Matrix34& footPartGlobalMtx = *apGlobalMtx[footPart];
				avHighestIntersectionPosition[footPart].z = -FLT_MAX;

				u32 resultIndex = 0;
				for (WorldProbe::ResultIterator it = capsuleResultsFootPart.begin(); it < capsuleResultsFootPart.last_result(); ++it, ++resultIndex)
				{
					// Find the highest intersection
					if (it->GetHitDetected())
					{
						const float fHitPositionZ = it->GetHitPosition().z;
						const float fIntersectionHeight = fHitPositionZ - rootAndPelvisSituation.vGroundPosition.z;

						if (bValidateIntersectionHeight && (fIntersectionHeight > fFootMaxPositiveDeltaZ))
						{
							// Ignore intersections beyond a certain height from the character's ground position
							// Ignore all further intersections below this one since the character is likely penetrating an object
							break;
						}

						Vector3 vIntersectionPosition(footPartGlobalMtx.d.x, footPartGlobalMtx.d.y, fHitPositionZ);

						if(vIntersectionPosition.z > avHighestIntersectionPosition[footPart].z)
						{
							avHighestIntersectionPosition[footPart] = vIntersectionPosition;
							anHighestIntersectionIndex[footPart] = resultIndex;
						}
					}
				}
			}
		}

		// Check for at least one intersection
		if ((anHighestIntersectionIndex[HEEL] != -1) || (anHighestIntersectionIndex[BALL] != -1))
		{
			legSituation.bIntersection = true;

			if ((anHighestIntersectionIndex[HEEL] != -1) && (anHighestIntersectionIndex[BALL] != -1))
			{
				if (rootAndPelvisSituation.bOnStairs)
				{
					if (!rootAndPelvisSituation.bMoving || rootAndPelvisSituation.bMovingUpwards)
					{
						// Take the highest intersection found
						legSituation.vGroundPosition.z = Max(avHighestIntersectionPosition[HEEL].z, avHighestIntersectionPosition[BALL].z);
					}
					else
					{
						if (fabsf(avHighestIntersectionPosition[HEEL].z - avHighestIntersectionPosition[BALL].z) > ms_fStairHeightTolerance)
						{
							// Foot is across two steps
							bool bLowerStepIntersection = false;

							if (!m_bOnStairSlope)
							{
								// Check if the heel had also intersected the same step as the ball of the foot
								WorldProbe::CShapeTestResults& capsuleResultsFootPart = *data.m_apResults[legPart][HEEL];
								if (capsuleResultsFootPart.GetNumHits() > 0)
								{
									for (WorldProbe::ResultIterator it = capsuleResultsFootPart.begin(); it < capsuleResultsFootPart.last_result(); ++it)
									{
										if (it->GetHitDetected())
										{
											if (fabsf(it->GetHitPosition().z - avHighestIntersectionPosition[BALL].z) <= ms_fStairHeightTolerance)
											{
												bLowerStepIntersection = true;
												break;
											}
										}
									}
								}
							}

							if (bLowerStepIntersection && (fabsf(avHighestIntersectionPosition[BALL].z - rootAndPelvisSituation.vGroundPosition.z) < fabsf(avHighestIntersectionPosition[HEEL].z - rootAndPelvisSituation.vGroundPosition.z)))
							{
								// If the foot is predominantly over the lower step and the ped capsule has already moved to the lower step, take the lowest intersection height
								legSituation.vGroundPosition.z = Min(avHighestIntersectionPosition[HEEL].z, avHighestIntersectionPosition[BALL].z);
							}
							else
							{
								// Keep the foot on the higher step
								legSituation.vGroundPosition.z = avHighestIntersectionPosition[HEEL].z;
							}
						}
						else
						{
							// Foot is on the same step
							legSituation.vGroundPosition.z = avHighestIntersectionPosition[HEEL].z;
						}
					}

					legGoal.bFootFlattened = false;
				}
				else
				{
					// Calculate angle of foot from highest intersection positions to check whether the implied ground normal can be used. Otherwise, keep the foot flat.
					Vector3 vFootDirection(avHighestIntersectionPosition[BALL] - avHighestIntersectionPosition[HEEL]);

					float fXYMag = vFootDirection.XYMag();
					float fAngle = (fXYMag > 0.0f) ? fabsf(rage::safe_atan2f(vFootDirection.z, fXYMag)) : 0.0f;

					const bool bCanFlattenFoot = (!rootAndPelvisSituation.bMoving || !bOnSlope) && !rootAndPelvisSituation.bOnPhysical && !rootAndPelvisSituation.bInVehicle;

					if (bCanFlattenFoot && ((!legGoal.bFootFlattened && (fAngle >= ms_afFootAngleClamp[0])) || (legGoal.bFootFlattened && (fAngle >= ms_afFootAngleClamp[1]))))
					{
						// Foot angle is too great, so keep foot flat
						if (rootAndPelvisSituation.bMoving)
						{
							// Take the highest intersection found
							legSituation.vGroundPosition.z = Max(avHighestIntersectionPosition[HEEL].z, avHighestIntersectionPosition[BALL].z);
						}
						else
						{
							// Favour the intersection z value closest to the character's ground z. Helps cases with stepped geometry where both
							// toes intersect the higher geometry while the character capsule intersects the lower
							const float fDistGndToHeel = fabsf(avHighestIntersectionPosition[HEEL].z - rootAndPelvisSituation.vGroundPosition.z);
							const float fDistGndToToe = fabsf(avHighestIntersectionPosition[BALL].z - rootAndPelvisSituation.vGroundPosition.z);
							legSituation.vGroundPosition.z = fDistGndToHeel < fDistGndToToe ? avHighestIntersectionPosition[HEEL].z : avHighestIntersectionPosition[BALL].z;
						}

						legSituation.bFixupFootOrientation = true;
						legGoal.bFootFlattened = true;
					}
					else
					{
						if (!vFootDirection.IsZero())
						{
							vFootDirection.Normalize();

							// Generate ground normal relative to world up
							Vector3 vGroundNormal;

							vGroundNormal.Cross(vFootDirection, ZAXIS);
							vGroundNormal.Normalize();
							vGroundNormal.Cross(vFootDirection);
							vGroundNormal.Normalize();

							if (!vGroundNormal.IsZero())
							{
								ClampNormal(vGroundNormal);
								legSituation.vGroundNormal = vGroundNormal;

								// For vehicles such as bicycles/motorcycles, only fixup orientation if animated foot is on the ground (ignoring feet that are animated onto pedals/pegs)
								if (!rootAndPelvisSituation.bEnteringVehicle && (!rootAndPelvisSituation.bInVehicle || (legSituation.fAnimZToe < ms_fVehicleToeGndPosTolerance)))
								{
									legSituation.bFixupFootOrientation = true;
								}
							}
						}

						legSituation.vGroundPosition.z = avHighestIntersectionPosition[HEEL].z;
						legGoal.bFootFlattened = false;
					}
				}
			}
			else
			{
				// Leave ground normal at default and set position to the highest intersection point
				legSituation.vGroundPosition.z = (anHighestIntersectionIndex[HEEL] != -1) ? avHighestIntersectionPosition[HEEL].z : avHighestIntersectionPosition[BALL].z;
				legGoal.bFootFlattened = false;
			}

			for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
			{
				if (anHighestIntersectionIndex[footPart] != -1)
				{
					const WorldProbe::CShapeTestHitPoint& hitPoint = (*data.m_apResults[legPart][footPart])[anHighestIntersectionIndex[footPart]];
					const CEntity* pEntity = hitPoint.GetHitEntity();
					const CPhysical* pPhysical = pEntity && pEntity->GetIsPhysical() ? (CPhysical*)pEntity : NULL;

					if (pPhysical)
					{
						legSituation.bOnPhysical = true;

						if (!pPhysical->GetIsTypePed() || (!((CPed*)pPhysical)->GetUsingRagdoll() || (GetMode() != LEG_IK_MODE_FULL_MELEE)))
						{
							legSituation.bOnMovingPhysical |= (pPhysical->GetIsInWater() || pPhysical->GetWasInWater() || (pPhysical->GetVelocity().Mag2() > 0.0f) || (pPhysical->GetAngVelocity().Mag2() > 0.0f));
						}
					}
				}
			}
		}
		else
		{
			legGoal.bFootFlattened = false;
		}

		CalcSmoothedNormal(legSituation.vGroundNormal, legGoal.m_vGroundNormalSmoothed, ms_fGroundNormalRate, legGoal.m_vGroundNormalSmoothed, deltaTime, rootAndPelvisSituation.bInstantSetup);

#if __IK_DEV
		if(CaptureDebugRender() && ms_bRenderProbeIntersections)
		{
			Vector3 vAbove(0.0f, 0.0f, GetMaxAllowedPelvisDeltaZ());
			Vector3 vBelow(0.0f, 0.0f, fabsf(GetMinAllowedPelvisDeltaZ(rootAndPelvisSituation)));
		
			rage::Color32 aColour[NUM_FOOT_PARTS] = { Color_yellow, Color_orange };

			for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
			{
				ms_debugHelper.Line(legGoal.m_vProbePosition[footPart] + vAbove, legGoal.m_vProbePosition[footPart] - vBelow, aColour[footPart]);
			}

			if (ms_bRenderSweptProbes)
			{
				static dev_float t = 0.0f;
				static dev_float rate = 0.1f;

				t += deltaTime * rate;
				t = fmodf(t, 1.0f);

				const float fFootRadius = GetFootRadius(rootAndPelvisSituation);

				for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
				{
					Vector3 vStartPos(legGoal.m_vProbePosition[footPart] + vAbove);
					Vector3 vEndPos(legGoal.m_vProbePosition[footPart] - vBelow);

					ms_debugHelper.Sphere(vStartPos, fFootRadius, aColour[footPart], false);
					ms_debugHelper.Sphere(vEndPos, fFootRadius, aColour[footPart], false);

					Vector3 vecSweptSphere(VEC3_ZERO);
					vecSweptSphere.x = Lerp(t, vStartPos.x,  vEndPos.x);
					vecSweptSphere.y = Lerp(t, vStartPos.y,  vEndPos.y);
					vecSweptSphere.z = Lerp(t, vStartPos.z,  vEndPos.z);

					ms_debugHelper.Sphere(vecSweptSphere, fFootRadius, aColour[footPart], false);
				}
			}

			for (int footPart = 0; footPart < NUM_FOOT_PARTS; ++footPart)
			{
				WorldProbe::CShapeTestResults& capsuleResultsFootPart = *data.m_apResults[legPart][footPart];

				bool bIgnoreIntersection = false;

				if (capsuleResultsFootPart.GetResultsReady() && (capsuleResultsFootPart.GetNumHits() > 0))
				{
					const Matrix34& footPartGlobalMtx = *apGlobalMtx[footPart];

					for (WorldProbe::ResultIterator it = capsuleResultsFootPart.begin(); it < capsuleResultsFootPart.last_result(); ++it)
					{
						if (it->GetHitDetected())
						{
							Color32 colour(footPart == HEEL ? Color_yellow4 : Color_orange4);

							if (anHighestIntersectionIndex[footPart] != -1 && it == (capsuleResultsFootPart.begin()+anHighestIntersectionIndex[footPart]))
							{
								colour = (footPart == HEEL) ? Color_yellow : Color_orange;
							}

							if (bIgnoreIntersection || (bValidateIntersectionHeight && (it->GetHitPosition().z - rootAndPelvisSituation.vGroundPosition.z) > fFootMaxPositiveDeltaZ))
							{
								// Ignore intersections beyond a certain height from the character's ground position
								colour = (footPart == HEEL) ? Color_red : Color_red4;
								bIgnoreIntersection = true;
							}

							// Render the position of the intersection
							static dev_float bSphereSize = 0.01f;
							ms_debugHelper.Sphere(it->GetHitPosition(), bSphereSize, colour, false);

							Vector3 vIntersectionPosToFootIntersection(footPartGlobalMtx.d.x, footPartGlobalMtx.d.y, it->GetHitPosition().z);
							ms_debugHelper.Line(it->GetHitPosition(), vIntersectionPosToFootIntersection, colour);
						}
					}
				}
			}
		}
#endif
	}
}

void CLegIkSolver::WorkOutLegSituationsUsingMultipleShapeTests(LegGoal* legGoalArray, LegSituation* legSituationArray, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime)
{
	ikAssert(m_pPed);
	ikAssert(legGoalArray);
	ikAssert(legSituationArray);

	crSkeleton& skeleton = *m_pPed->GetSkeleton();

	SetupData(skeleton, legSituationArray, rootAndPelvisSituation);

	if (ms_bUseMultipleAsyncShapeTests && CanUseAsyncShapeTests(rootAndPelvisSituation))
	{
		if (m_pShapeTestData == NULL)
		{
			m_pShapeTestData = rage_new ShapeTestData();
		}

		if (deltaTime > 0.0f)
		{
			PreProcessResults(legGoalArray, rootAndPelvisSituation, *m_pShapeTestData);
		}
		ProcessResults(legGoalArray, legSituationArray, rootAndPelvisSituation, *m_pShapeTestData, deltaTime);
#if FPS_MODE_SUPPORTED
		if (!m_bFPSMode || (deltaTime == 0.0f))
		{
			SubmitAsyncShapeTests(legGoalArray, legSituationArray, rootAndPelvisSituation, *m_pShapeTestData, m_fDeltaTime);
		}
#else
		if (deltaTime > 0.0f)
		{
			SubmitAsyncShapeTests(legGoalArray, legSituationArray, rootAndPelvisSituation, *m_pShapeTestData, deltaTime);
		}
#endif // FPS_MODE_SUPPORTED
	}
	else
	{
		if (m_pShapeTestData && (m_pShapeTestData->m_apResults[0][0]->GetResultsStatus() != WorldProbe::TEST_NOT_SUBMITTED))
		{
			// Reset pending shape test data
			m_pShapeTestData->Reset();
		}

		ShapeTestData data(false);

		WorldProbe::CShapeTestHitPoint intersections[NUM_LEGS][NUM_FOOT_PARTS][NUM_INTERSECTIONS_FOR_PROBES];
		WorldProbe::CShapeTestResults results[NUM_LEGS][NUM_FOOT_PARTS] = 
		{
			{ WorldProbe::CShapeTestResults(intersections[LEFT_LEG][HEEL], NUM_INTERSECTIONS_FOR_PROBES), WorldProbe::CShapeTestResults(intersections[LEFT_LEG][BALL], NUM_INTERSECTIONS_FOR_PROBES) },
			{ WorldProbe::CShapeTestResults(intersections[RIGHT_LEG][HEEL], NUM_INTERSECTIONS_FOR_PROBES), WorldProbe::CShapeTestResults(intersections[RIGHT_LEG][BALL], NUM_INTERSECTIONS_FOR_PROBES) }
		};

		for (int legPart = 0; legPart < CLegIkSolver::NUM_LEGS; ++legPart)
		{
			for (int footPart = 0; footPart < CLegIkSolver::NUM_FOOT_PARTS; ++footPart)
			{
				data.m_apResults[legPart][footPart] = &results[legPart][footPart];
			}
		}

		SubmitShapeTests(legGoalArray, legSituationArray, rootAndPelvisSituation, data);
		ProcessResults(legGoalArray, legSituationArray, rootAndPelvisSituation, data, deltaTime);
	}
}

/*void CLegIkSolver::CalcRootGroundIntersection(const Vector3& vStartPos, const Vector3& vEndPos, RootAndPelvisSituation& rootAndPelvisSituation)
{
	rootAndPelvisSituation.bIntersection = false;

	u32 iShapeTestIncludeFlags = GetShapeTestIncludeFlags();

	// assume the ground normal should be straight up
	rootAndPelvisSituation.vGroundNormal.Set(0.0f,0.0f,1.0f);

	// if the probe hits a ragdoll that's not prone then we repeat the test again ignoring ragdolls test 
	// JA Believe this was added to work around issues during melee fighting
	// JA Only enable when in Melee?
	bool bRepeatProbe = true;
	while(bRepeatProbe)
	{
		bRepeatProbe = false;

		static dev_float fCylinderRadius = 0.1f;
		WorldProbe::CShapeTestHitPoint capsuleIsects[NUM_INTERSECTIONS_FOR_PROBES];
		WorldProbe::CShapeTestResults capsuleResults(capsuleIsects, NUM_INTERSECTIONS_FOR_PROBES);
		CapsuleTest(vStartPos, vEndPos, fCylinderRadius, iShapeTestIncludeFlags, capsuleResults);

		// T value goes from 0.0f at start of shape test to 1.0f at end
		// The lowest t value will give us the highest intersection point
		float fHighestZHeight = -FLT_MAX;
		bool bBVH = false;
		int	nHighestIntersection = -1;
		if(capsuleResults.GetNumHits() > 0)
		{
			u32 i = 0;
			for(WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it, ++i)
			{			
				// Find the highest intersection
				// Ignore normals that are nearly horizontal
				if(it->GetHitDetected())
				{
					// For two points on a plane a, b and the plane normal N we know N.a = N.b
					// Therefore take a as the ground pos from the ped's capsule and b as the foot position
					// then solve for b.z
					// => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
					float fHeight = it->GetHitPosition().GetZ();

					if ( it->GetHitInst() && it->GetHitInst()->GetArchetype() && it->GetHitInst()->GetArchetype()->GetBound() && it->GetHitInst()->GetArchetype()->GetBound()->GetType() == phBound::BVH)
					{
						fHeight	+= CONCAVE_DISTANCE_MARGIN;
					}

					if(fHeight) > fHighestZHeight)
					{
						if ( it->GetHitInst() && it->GetHitInst()->GetArchetype() && it->GetHitInst()->GetArchetype()->GetBound() && it->GetHitInst()->GetArchetype()->GetBound()->GetType() == phBound::BVH)
						{
							bBVH = true;
						}
						else
						{
							bBVH = false;
						}
						fHighestZHeight = fHeight;
						nHighestIntersection = i;
					}
				}
			}
		}

		// Force no collision (for testing)
		static dev_bool bFakeNoCollision = false;

		// Did we collide with anything?
		if(nHighestIntersection != -1 && !bFakeNoCollision)
		{
			// Did the chosen line test hit a prone ragdoll?
			const phInst *pPhInst = capsuleResults[nHighestIntersection].GetHitInst();
			if(pPhInst)
			{
				CEntity *pEntity = CPhysics::GetEntityFromInst((phInst*)pPhInst);
				if(pEntity && pEntity->GetIsTypePed())
				{
					CPed *pPed = (CPed*)pEntity;
					EstimatedPose thePedsPose = pPed->EstimatePose();

					if( (thePedsPose != POSE_ON_FRONT)
						&& (thePedsPose != POSE_ON_RHS)
						&&	(thePedsPose != POSE_ON_BACK)
						&&	(thePedsPose != POSE_ON_LHS) )
					{
						// ped is not laying flat
						if(iShapeTestIncludeFlags & ArchetypeFlags::GTA_RAGDOLL_TYPE)
						{
							iShapeTestIncludeFlags &=~ ArchetypeFlags::GTA_RAGDOLL_TYPE;
							bRepeatProbe = true;

							// go back to while loop and try again without colliding with peds
							continue;
						}
					}
				}
			}


			// The normal of the surface we are colliding with
			rootAndPelvisSituation.vGroundNormal = capsuleResults[nHighestIntersection].GetHitNormal();

			// The z position of the surface we are colliding with
			rootAndPelvisSituation.vGroundPosition = capsuleResults[nHighestIntersection].GetHitPosition();
			rootAndPelvisSituation.vGroundPosition.z = fHighestZHeight; 
			rootAndPelvisSituation.bIntersection = true;

#if __IK_DEV
			if (CaptureDebugRender() && ms_bRenderSituations)
			{
				char debugText[100];
				sprintf(debugText, "\nRootAndPelvis\n");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
				sprintf(debugText, "BVH = %s \n", bBVH ? "TRUE" : "FALSE");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
				sprintf(debugText, "fHighestZHeight = %f \n", fHighestZHeight);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
				sprintf(debugText, "rootAndPelvisSituation.vGroundPosition.z = %f \n", rootAndPelvisSituation.vGroundPosition.z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
				sprintf(debugText, "GroundPos = %f \n", m_pPed->GetGroundPos().z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
				sprintf(debugText, "DIFF = %f \n", rootAndPelvisSituation.vGroundPosition.z - m_pPed->GetGroundPos().z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
#endif //__IK_DEV

		}
		else
		{
			// The normal of the surface we are colliding with
			// No collision normal so assume surface is flat
			// (we could use the normal of the foot from the animation)
			rootAndPelvisSituation.vGroundNormal = Vector3(0.0f ,0.0f, 1.0f);

			// No collision height so pretend we are on flat ground
			// JA - Maybe using the z height of the foot would be better?
			rootAndPelvisSituation.vGroundPosition.x = m_pPed->GetTransform().GetPosition().GetXf();
			rootAndPelvisSituation.vGroundPosition.y = m_pPed->GetTransform().GetPosition().GetYf();
			rootAndPelvisSituation.vGroundPosition.z = m_pPed->GetTransform().GetPosition().GetZf() - m_pPed->GetCapsuleInfo()->GetGroundToRootOffset();
			rootAndPelvisSituation.bIntersection = false;
		}

#if __IK_DEV
		if(CaptureDebugRender() && ms_bRenderProbeIntersections)
		{
			// Render the 3 intersections
			// Render a green line if the line test did not intersect with anything
			// Render a red line if the line test did intersect with something
			// Render a white sphere at the chosen intersection
			// Render a yellow sphere at the other intersections
			ms_debugHelper.Line(vStartPos, vEndPos, Color_red);

			if (ms_bRenderSweptProbes)
			{
				static dev_float t = 0.0f;
				static dev_float rate = 0.1f;

				ms_debugHelper.Sphere(vStartPos, fCylinderRadius, Color_red, false);
				ms_debugHelper.Sphere(vEndPos, fCylinderRadius, Color_red, false);
				t += fwTimer::GetTimeStep() * rate;
				t = fmodf(t, 1.0f);
				Vector3 vecSweptSphere(VEC3_ZERO);
				vecSweptSphere.x = Lerp(t, vStartPos.x,  vEndPos.x);
				vecSweptSphere.y = Lerp(t, vStartPos.y,  vEndPos.y);
				vecSweptSphere.z = Lerp(t, vStartPos.z,  vEndPos.z);

				ms_debugHelper.Sphere(vecSweptSphere, fCylinderRadius, Color_red, false);
			}

			for(WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it)
			{
				if(it->GetHitDetected())
				{
					Color32 colour(Color_yellow);
					if(it->GetHitNormal().z > ms_fNormalVerticalThreshold)
					{
						colour = Color_yellow;
					}
					else
					{
						colour = Color_green;
					}

					static dev_float bSphereSize = 0.01f;
					if(it == (capsuleResults.begin()+nHighestIntersection))
					{
						colour = Color_white;
					}

					// Render the position of the intersection
					ms_debugHelper.Sphere(it->GetHitPosition(), bSphereSize, colour, true);

					// Render the normal of the intersection
					static dev_float fNormalScale = 0.1f;
					ms_debugHelper.Line(it->GetHitPosition(), it->GetHitPosition() + (it->GetHitNormal() * fNormalScale), Color_purple);					
				}
			}
		}
#endif //__IK_DEV
	}
}*/

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::CalcGroundIntersection(const Vector3& vStartPos, const Vector3& vEndPos, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation)
{
	const bool bOnFoot = !m_pPed->GetIsInVehicle() && !m_pPed->GetIsOnMount();
	const float fFootMaxPositiveDeltaZ = GetFootMaxPositiveDeltaZ(rootAndPelvisSituation);

	legSituation.bIntersection = false;
	legSituation.bOnPhysical = false;
	legSituation.bOnMovingPhysical = false;

	u32 iShapeTestIncludeFlags = GetShapeTestIncludeFlags();

	// assume the ground normal should be straight up
	legSituation.vGroundNormal.Set(0.0f,0.0f,1.0f);

	// if the probe hits a ragdoll that's not prone then we repeat the test again ignoring ragdolls test 
	// JA Believe this was added to work around issues during melee fighting
	// JA Only enable when in Melee?
	bool bRepeatProbe = true;
	while(bRepeatProbe)
	{
		bRepeatProbe = false;

		static dev_float fCylinderRadius = 0.1f;
		WorldProbe::CShapeTestHitPoint capsuleIsects[NUM_INTERSECTIONS_FOR_PROBES];
		WorldProbe::CShapeTestResults capsuleResults(capsuleIsects, NUM_INTERSECTIONS_FOR_PROBES);
		CapsuleTest(vStartPos, vEndPos, fCylinderRadius, iShapeTestIncludeFlags, capsuleResults);
		
		// T value goes from 0.0f at start of shape test to 1.0f at end
		// The lowest t value will give us the highest intersection point
		bool bBVH = false;
		int	nHighestIntersectionIndex = -1;
		Vector3 vHighestIntersectionNormal(0.0f ,0.0f, 1.0f);
		Vector3 vHighestIntersectionPosition(legSituation.footGlobalMtx.d);
		vHighestIntersectionPosition.z = -FLT_MAX;
		if(capsuleResults.GetNumHits() > 0)
		{
			u32 i = 0;
			for(WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it, ++i)
			{	
				// Find the highest intersection
				if(it->GetHitDetected())
				{
					/*
					Vector3 vIntersectionNormal(it->GetHitPolyNormal());
					Vector3 vIntersectionPosition(it->GetHitPosition());
					if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || (vIntersectionNormal.z < ms_fNormalVerticalThreshold))
					{
						vIntersectionNormal = Vector3(0.0f ,0.0f, 1.0f);
					}

					// For two points on a plane a, b and the plane normal N we know N.a = N.b
					// Therefore take a as the ground pos from the ped's capsule and b as the foot position
					// then solve for b.z
					// => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
					vIntersectionPosition.z = vIntersectionNormal.Dot(vIntersectionPosition) - vIntersectionNormal.x*legSituation.footGlobalMtx.d.x - vIntersectionNormal.y*legSituation.footGlobalMtx.d.y;
					vIntersectionPosition.z /= vIntersectionNormal.z;
					vIntersectionPosition.x = legSituation.footGlobalMtx.d.x;
					vIntersectionPosition.y = legSituation.footGlobalMtx.d.y;
					*/

					if(bOnFoot && ((it->GetHitPosition().z - rootAndPelvisSituation.vGroundPosition.z) > fFootMaxPositiveDeltaZ))
					{
						// Ignore intersections beyond a certain height from the character's ground position
						// Ignore all further intersections below this one since the character is likely penetrating an object
						break;
					}

					Vector3 vIntersectionNormal(it->GetHitPolyNormal());
					if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || (vIntersectionNormal.z < ms_fNormalVerticalThreshold))
					{
						vIntersectionNormal = Vector3(0.0f ,0.0f, 1.0f);
					}
					Vector3 vIntersectionPosition(legSituation.footGlobalMtx.d.x, legSituation.footGlobalMtx.d.y, it->GetHitPosition().z);

					if(vIntersectionPosition.z > vHighestIntersectionPosition.z)
					{
						if ( it->GetHitInst() && it->GetHitInst()->GetArchetype() && it->GetHitInst()->GetArchetype()->GetBound() && it->GetHitInst()->GetArchetype()->GetBound()->GetType() == phBound::BVH)
						{
							bBVH = true;
						}
						else
						{
							bBVH = false;
						}
						vHighestIntersectionNormal = vIntersectionNormal;
						vHighestIntersectionPosition = vIntersectionPosition;
						nHighestIntersectionIndex = i;
					}
				}
			}
		}

		// Force no collision (for testing)
		static dev_bool bFakeNoCollision = false;

		// Did we collide with anything?
		if(nHighestIntersectionIndex != -1 && !bFakeNoCollision)
		{
			// Did the chosen line test hit a prone ragdoll?
			const phInst *pPhInst = capsuleResults[nHighestIntersectionIndex].GetHitInst();
			if(pPhInst)
			{
				CEntity *pEntity = CPhysics::GetEntityFromInst((phInst*)pPhInst);
				if(pEntity && pEntity->GetIsTypePed())
				{
					CPed *pPed = (CPed*)pEntity;
					EstimatedPose thePedsPose = pPed->EstimatePose();

					if( (thePedsPose != POSE_ON_FRONT)
						&& (thePedsPose != POSE_ON_RHS)
						&&	(thePedsPose != POSE_ON_BACK)
						&&	(thePedsPose != POSE_ON_LHS) )
					{
						// ped is not laying flat
						if(iShapeTestIncludeFlags & ArchetypeFlags::GTA_RAGDOLL_TYPE)
						{
							iShapeTestIncludeFlags &=~ ArchetypeFlags::GTA_RAGDOLL_TYPE;
							bRepeatProbe = true;

							// go back to while loop and try again without colliding with peds
							continue;
						}
					}
				}
			}


			legSituation.bIntersection = true;

			// The normal of the surface we are colliding with
			legSituation.vGroundNormal = vHighestIntersectionNormal;

			// The position of the surface we are colliding with
			legSituation.vGroundPosition = vHighestIntersectionPosition; 

#if __IK_DEV
			if (CaptureDebugRender() && ms_bRenderSituations)
			{
				char debugText[100];
				sprintf(debugText, "\nLeg\n");
				sprintf(debugText, "BVH = %s \n", bBVH ? "TRUE" : "FALSE");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "vHighestIntersectionPosition.z = %f \n", vHighestIntersectionPosition.z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "legSituation.vGroundPosition.z = %f \n", legSituation.vGroundPosition.z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "rootAndPelvisSituation.vGroundPosition.z = %f \n", rootAndPelvisSituation.vGroundPosition.z);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "DIFF = %f \n\n", legSituation.vGroundPosition.z - rootAndPelvisSituation.vGroundPosition.z );
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}
#endif //__IK_DEV

		}
		else
		{
			// The normal of the surface we are colliding with
			// No collision normal so assume surface is flat
			// (we could use the normal of the foot from the animation)
			legSituation.vGroundNormal = Vector3(0.0f ,0.0f, 1.0f);

			/*
			// No collision height so pretend we are on flat ground
			// JA - Maybe using the z height of the foot would be better?
			legSituation.vGroundPosition.x = m_pPed->GetTransform().GetPosition().GetXf();
			legSituation.vGroundPosition.y = m_pPed->GetTransform().GetPosition().GetYf();
			legSituation.vGroundPosition.z = m_pPed->GetTransform().GetPosition().GetZf() - m_pPed->GetCapsuleInfo()->GetGroundToRootOffset();
			legSituation.bIntersection = false;

			// For two points on a plane a, b and the plane normal N we know N.a = N.b
			// Therefore take a as the ground pos from the ped's capsule and b as the foot position
			// then solve for b.z
			// => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
			legSituation.vGroundPosition.z = legSituation.vGroundNormal.Dot(legSituation.vGroundPosition) - legSituation.vGroundNormal.x*legSituation.footGlobalMtx.d.x - legSituation.vGroundNormal.y*legSituation.footGlobalMtx.d.y;
			legSituation.vGroundPosition.z /= legSituation.vGroundNormal.z;
			legSituation.vGroundPosition.x = legSituation.footGlobalMtx.d.x;
			legSituation.vGroundPosition.y = legSituation.footGlobalMtx.d.y;
			*/

			legSituation.bIntersection = false;
			legSituation.vGroundPosition = Vector3(legSituation.footGlobalMtx.d.x, legSituation.footGlobalMtx.d.y, m_pPed->GetTransform().GetPosition().GetZf() - m_pPed->GetCapsuleInfo()->GetGroundToRootOffset());
		}

#if __IK_DEV
		if(CaptureDebugRender() && ms_bRenderProbeIntersections)
		{
			// Render the 3 intersections
			// Render a green line if the line test did not intersect with anything
			// Render a red line if the line test did intersect with something
			// Render a white sphere at the chosen intersection
			// Render a yellow sphere at the other intersections
			ms_debugHelper.Line(vStartPos, vEndPos, Color_red);

			if (ms_bRenderSweptProbes)
			{
				static dev_float t = 0.0f;
				static dev_float rate = 0.1f;

				ms_debugHelper.Sphere(vStartPos, fCylinderRadius, Color_red, false);
				ms_debugHelper.Sphere(vEndPos, fCylinderRadius, Color_red, false);
				t += fwTimer::GetTimeStep() * rate;
				t = fmodf(t, 1.0f);
				Vector3 vecSweptSphere(VEC3_ZERO);
				vecSweptSphere.x = Lerp(t, vStartPos.x,  vEndPos.x);
				vecSweptSphere.y = Lerp(t, vStartPos.y,  vEndPos.y);
				vecSweptSphere.z = Lerp(t, vStartPos.z,  vEndPos.z);

				ms_debugHelper.Sphere(vecSweptSphere, fCylinderRadius, Color_red, false);
			}

			bool bIgnoreIntersection = false;

			for(WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it)
			{
				if(it->GetHitDetected())
				{
					Color32 colour(Color_yellow);

					Vector3 vIntersectionNormal(it->GetHitPolyNormal());
					Vector3 vIntersectionPosition(it->GetHitPosition());
					if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || (vIntersectionNormal.z < ms_fNormalVerticalThreshold))
					{
						vIntersectionNormal = Vector3(0.0f ,0.0f, 1.0f);
					}

					if(nHighestIntersectionIndex != -1 && it == (capsuleResults.begin()+nHighestIntersectionIndex))
					{
						colour = Color_white;
					}

					if(bIgnoreIntersection || (bOnFoot && ((it->GetHitPosition().z - rootAndPelvisSituation.vGroundPosition.z) > fFootMaxPositiveDeltaZ)))
					{
						// Ignore intersections beyond a certain height from the character's ground position
						colour = Color_red;
						bIgnoreIntersection = true;
					}

					// Render the position of the intersection
					static dev_float bSphereSize = 0.01f;
					ms_debugHelper.Sphere(it->GetHitPosition(), bSphereSize, colour, true);

					// Render the normal of the intersection
					static dev_float fPolyNormalScale = 0.1f;
					ms_debugHelper.Line(it->GetHitPosition(), it->GetHitPosition() + (it->GetHitPolyNormal() * fPolyNormalScale), Color_green3);

					static dev_float fTweakedNormalScale = 0.15f;
					ms_debugHelper.Line(it->GetHitPosition(), it->GetHitPosition() + (vIntersectionNormal * fTweakedNormalScale), Color_blue3);

					/*// For two points on a plane a, b and the plane normal N we know N.a = N.b
					// Therefore take a as the ground pos from the ped's capsule and b as the foot position
					// then solve for b.z
					// => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
					float fHeight = vIntersectionNormal.Dot(it->GetHitPosition()) - vIntersectionNormal.x*legSituation.footGlobalMtx.d.x - vIntersectionNormal.y*legSituation.footGlobalMtx.d.y;
					fHeight /= vIntersectionNormal.z;

					Vector3 vIntersectionPosToFootIntersectionwithPlane(legSituation.footGlobalMtx.d);
					vIntersectionPosToFootIntersectionwithPlane.z = fHeight;*/

					Vector3 vIntersectionPosToFootIntersection(legSituation.footGlobalMtx.d.x, legSituation.footGlobalMtx.d.y, it->GetHitPosition().z);
					ms_debugHelper.Line(it->GetHitPosition(), vIntersectionPosToFootIntersection, colour);
				}
			}
		}
#endif //__IK_DEV
	}
}

//////////////////////////////////////////////////////////////////////////
void CLegIkSolver::FixupFootOrientation(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime)
{
	IK_DEV_ONLY(Vector3 originalFootPosition(legSituation.footGlobalMtx.d);)

	const float fBlendDelta = deltaTime * ms_fFootOrientationBlendRate;

	const bool bOnSlope = rootAndPelvisSituation.bOnSlope || rootAndPelvisSituation.bOnSlopeLoco;
	const bool bStandingOnSlope = bOnSlope && !rootAndPelvisSituation.bMoving && !rootAndPelvisSituation.bOnStairs && !rootAndPelvisSituation.bInVehicle && !rootAndPelvisSituation.bEnteringVehicle;

	float fBlendTarget = 0.0f;
	if ((bStandingOnSlope || legSituation.bFixupFootOrientation) && !rootAndPelvisSituation.bInVehicleReverse)
	{
		fBlendTarget = 1.0f;
	}

	// work out a blend factor for the orientation of the foot based on how near the floor the foot is in the anim
	legGoal.m_fFootOrientationBlend = !rootAndPelvisSituation.bInstantSetup ? IncrementTowards(legGoal.m_fFootOrientationBlend, fBlendTarget, fBlendDelta) : fBlendTarget;

	// blend in/out the foot orientation
	float fBlend = legGoal.m_fFootOrientationBlend * legGoal.m_fLegBlend;

	if (legSituation.bFixupFootOrientation)
	{
		const Vec3V vFootAxis(RC_VEC3V(legSituation.footGlobalMtx.c));
		ScalarV vRotationAngle(V_ZERO);
		QuatV qRotation;

		if (rootAndPelvisSituation.bMoving || !IsCloseAll(ScalarV(legGoal.m_vGroundNormalSmoothed.z), ScalarV(V_ONE), ScalarV(V_FLT_SMALL_3)))
		{
			Vec3V vRotationAxis;

			// Update foot orientation based on amount of rotation of ground normal from world up about foot's right axis
			qRotation = QuatVFromVectors(Vec3V(V_UP_AXIS_WZERO), RC_VEC3V(legGoal.m_vGroundNormalSmoothed), vFootAxis);
			QuatVToAxisAngle(vRotationAxis, vRotationAngle, qRotation);

			// Keep rotation angle about foot axis (may get flipped by QuatVFromVectors)
			if (IsLessThanAll(Dot(vFootAxis, vRotationAxis), ScalarV(V_ZERO)))
			{
				vRotationAngle = Negate(vRotationAngle);
			}
		}

		// Update foot angle while not locked
		if (!legGoal.m_bLockFootZ)
		{
			legGoal.m_fFootAngleLocked = vRotationAngle.Getf();
		}

		legGoal.m_fFootAngleSmoothed = !rootAndPelvisSituation.bInstantSetup ? Lerp(Clamp(fBlendDelta, 0.0f, 1.0f), legGoal.m_fFootAngleSmoothed, legGoal.m_fFootAngleLocked) : legGoal.m_fFootAngleLocked;

		if (fBlend > 0.0f)
		{
			Mat34V mtxRotation;

			vRotationAngle.Setf(legGoal.m_fFootAngleSmoothed);

			// Slope loco anims are animated to a default slope angle, so apply correction
			vRotationAngle = Add(vRotationAngle, ScalarV(ms_fFootAngleSlope * m_rootAndPelvisGoal.m_fOnSlopeLocoBlend));

			if (rootAndPelvisSituation.bInVehicle)
			{
				// Counter slope of vehicle when rotating the foot since the character's up vector is no longer aligned with world up
				const ScalarV vSlopePitch(rootAndPelvisSituation.fSlopePitch);
				vRotationAngle = Add(vRotationAngle, SelectFT(IsLessThan(vRotationAngle, ScalarV(V_ZERO)), Negate(vSlopePitch), vSlopePitch));
			}

			vRotationAngle = Scale(vRotationAngle, ScalarV(fBlend));
			qRotation = QuatVFromAxisAngle(vFootAxis, vRotationAngle);

			// Update foot/toe
			Mat34VFromQuatV(mtxRotation, qRotation);
			Transform3x3(RC_MAT34V(legSituation.footGlobalMtx), mtxRotation, RC_MAT34V(legSituation.footGlobalMtx));
			Transform(RC_MAT34V(legSituation.toeGlobalMtx), RC_MAT34V(legSituation.footGlobalMtx), legSituation.toeLocalMtx);
		}
	}
	else
	{
		if (fBlend > 0.0f)
		{
			Matrix34 floor;

			// The foot bone has an unusual orientation in the bind pose, which makes comparisons with the ground normal difficult
			// Strip off the bind pose rotation to make comparisons with the ground normal easier
			const Matrix34& bindPoseMatrix = RCC_MATRIX34(m_pPed->GetSkeletonData().GetDefaultTransform(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT]));
			Matrix34 bindPoseTranspose(bindPoseMatrix);
			bindPoseTranspose.Transpose(bindPoseMatrix);

			legSituation.footGlobalMtx.Dot3x3FromLeft(bindPoseTranspose);

			// How flat is the foot in the anim? Once the bind pose transform is removed the bone's x axis should point downwards
			// check this against the ped's downward axis to determine how flat the foot is in the anim.
			float howFlat = DotProduct(legSituation.footGlobalMtx.a, -VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetC()));

			// orient the foot axis towards the (opposite) direction of the ground normal, interpolating with t and howFlat 
			// (these should allow for smooth blending as the foot reaches and leaves the ground)
			Vector3	delta = ((-legGoal.m_vGroundNormalSmoothed)-legSituation.footGlobalMtx.a)*fBlend*howFlat;

			if (!delta.IsZero())
			{
				floor.a = legSituation.footGlobalMtx.a+delta;
				floor.c = legSituation.footGlobalMtx.c;
				floor.b.Cross(floor.c,floor.a);
				floor.Normalize();
				floor.d = legSituation.footGlobalMtx.d;

				legSituation.footGlobalMtx.Interpolate(legSituation.footGlobalMtx, floor, fBlend);
			}

			//reapply the bind pose orientation we removed earlier
			legSituation.footGlobalMtx.Dot3x3FromLeft(bindPoseMatrix);
		}

		legGoal.m_fFootAngleLocked = 0.0f;
		legGoal.m_fFootAngleSmoothed = 0.0f;
	}

#if __IK_DEV
	ikAssertf(legSituation.footGlobalMtx.d.IsClose(originalFootPosition, 0.005f), "The foot has been moved by FixupFootOrientation");
	if (CaptureDebugRender())
	{
		char debugText[96];
		formatf(debugText, sizeof(debugText), "AngBlend : %8.3f Angle : %6.3f AngSmoothed   : %6.3f Flat: %d", fBlend, legGoal.m_fFootAngleLocked, legGoal.m_fFootAngleSmoothed, legGoal.bFootFlattened ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

//////////////////////////////////////////////////////////////////////////
void CLegIkSolver::FixupFootHeight(LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime)
{
	IK_DEV_ONLY(char debugText[128];)

	float fTargetFootWorldZ = 0.0f;

	// Work out the lowest point the foot can be and keep the foot and toe above ground
	float fMinHeightOfAnimFootAboveCollision = m_afHeightOfBindPoseFootAboveCollision[HEEL];
	float fHeightOfAnimFootAboveCollision = legSituation.fAnimZFoot * ms_fAnimFootZBlend;

	const bool bOnSlope = rootAndPelvisSituation.bOnSlope || rootAndPelvisSituation.bOnSlopeLoco;
	const bool bOnSlopeOrOldStairs = (bOnSlope || rootAndPelvisSituation.bOnStairs) && !m_bOnStairSlope;

	if (bOnSlopeOrOldStairs || rootAndPelvisSituation.bInVehicle)
	{
		float fSlopeHeight = 0.0f;

		if (!rootAndPelvisSituation.bOnStairs)
		{
			// Calculate theoretical ground position z of toe based on foot angle and foot ground position z
			Vec3V vFootToToe(Subtract(RC_VEC3V(legSituation.toeGlobalMtx.d), RC_VEC3V(legSituation.footGlobalMtx.d)));
			ScalarV vSlopeHeight(Tan(ScalarV(legGoal.m_fFootAngleLocked)));
			vSlopeHeight = Scale(vSlopeHeight, MagXY(vFootToToe));
			fSlopeHeight = vSlopeHeight.Getf();
		}

		// Account for any adjustment in the pelvis height
		const float fAdjustedAnimZFoot = legSituation.footGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;
		fHeightOfAnimFootAboveCollision = fAdjustedAnimZFoot * ms_fAnimFootZBlend;

		// Raise foot to be above intersection points based on which part of the foot is lower
		const float fLowestZFoot = legSituation.footGlobalMtx.d.z - m_afHeightOfBindPoseFootAboveCollision[HEEL];
		const float fLowestZToe  = legSituation.toeGlobalMtx.d.z - m_afHeightOfBindPoseFootAboveCollision[BALL];

		if ((m_rootAndPelvisGoal.m_fOnSlopeLocoBlend > 0.0f) || (fLowestZToe < fLowestZFoot))
		{
			fMinHeightOfAnimFootAboveCollision = m_afHeightOfBindPoseFootAboveCollision[BALL] + fSlopeHeight + (legSituation.footGlobalMtx.d.z - legSituation.toeGlobalMtx.d.z);
		}
	}
	else if (legSituation.fAnimZToe < legSituation.fAnimZFoot)
	{
		fMinHeightOfAnimFootAboveCollision = (legSituation.fAnimZFoot - legSituation.fAnimZToe) + m_afHeightOfBindPoseFootAboveCollision[BALL];
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "MinHeight: %8.3f Height: %6.3f", fMinHeightOfAnimFootAboveCollision, fHeightOfAnimFootAboveCollision);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	fHeightOfAnimFootAboveCollision = MAX(fMinHeightOfAnimFootAboveCollision, fHeightOfAnimFootAboveCollision);

	float fGroundPositionZ;
	if ((rootAndPelvisSituation.bOnStairs || (m_rootAndPelvisGoal.m_fOnSlopeLocoBlend > 0.0f)) && legGoal.m_bLockFootZ && legGoal.m_bLockedFootZCoordValid && legSituation.bSupporting)
	{
		fGroundPositionZ = legGoal.m_fLockedFootZCoord;
		ikDebugf3("Leg ik - foot using locked z coord %.3f", legGoal.m_fLockedFootZCoord);
	}
	else
	{
		fGroundPositionZ = legSituation.vGroundPosition.z;
		legGoal.m_fLockedFootZCoord = legSituation.vGroundPosition.z;
		legGoal.m_bLockedFootZCoordValid = true;
	}
	fTargetFootWorldZ = fGroundPositionZ + fHeightOfAnimFootAboveCollision;

	// Enforce a minimum height from the foot to the root
	float fMinWorldZ = rootAndPelvisSituation.rootGlobalMtx.d.z - GetMinDistanceFromFootToRoot(rootAndPelvisSituation);
#if __IK_DEV
	if (CaptureDebugRender() && fMinWorldZ < fTargetFootWorldZ)
	{
		ms_debugHelper.Text3d(legSituation.footGlobalMtx.d, Color_red1, 0, 0, "ApplyMin");
	}
#endif //__IK_DEV
	fTargetFootWorldZ = MIN(fTargetFootWorldZ, fMinWorldZ);

	// Work out the delta z to the target
	float fFootDeltaZ = fTargetFootWorldZ - legSituation.footGlobalMtx.d.z;
	if ((!rootAndPelvisSituation.bLowCover || rootAndPelvisSituation.bMoving) && ms_bPositiveFootDeltaZOnly && fFootDeltaZ < 0.0f)
	{
		fFootDeltaZ = 0.0f;
	}

	fFootDeltaZ *= legGoal.m_fLegBlend;

	if (!rootAndPelvisSituation.bInstantSetup)
	{
		const float scale = 1.0f;

		if (legSituation.bOnMovingPhysical)
		{
			legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_FootInterpOnDynamic.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime);
		}
		else if (((legSituation.footGlobalMtx.d.z - m_afHeightOfBindPoseFootAboveCollision[HEEL]) < fGroundPositionZ) || ((legSituation.toeGlobalMtx.d.z - m_afHeightOfBindPoseFootAboveCollision[BALL]) < fGroundPositionZ))
		{
			if (rootAndPelvisSituation.bOnStairs)
			{
				if (!rootAndPelvisSituation.bCoverAim)
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_StairsFootInterpIntersecting.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
				else
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_StairsFootInterpCoverAim.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
			}
			else
			{
				if (rootAndPelvisSituation.bMoving)
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_FootInterpIntersectingMoving.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
				else
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_FootInterpIntersecting.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
			}
		}
		else
		{
			if (rootAndPelvisSituation.bOnStairs)
			{
				if (!rootAndPelvisSituation.bCoverAim)
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_StairsFootInterp.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
				else
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_StairsFootInterpCoverAim.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
			}
			else
			{
				if (rootAndPelvisSituation.bMoving)
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_FootInterpMoving.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
				else
				{
					legGoal.m_fFootDeltaZSmoothed = sm_Tunables.m_FootInterp.IncrementTowards(legGoal.m_fFootDeltaZSmoothed, fFootDeltaZ, legGoal.m_fFootDeltaZ, deltaTime, scale);
				}
			}
		}
	}
	else
	{
		legGoal.m_fFootDeltaZSmoothed = fFootDeltaZ;
	}

	legSituation.footGlobalMtx.d.z += legGoal.m_fFootDeltaZSmoothed;

#if __IK_DEV
	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "TargetZ  : %8.3f DeltaZ: %6.3f DeltaZSmoothed: %6.3f", fTargetFootWorldZ, fFootDeltaZ, legGoal.m_fFootDeltaZSmoothed);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

//////////////////////////////////////////////////////////////////////////
void CLegIkSolver::FixupLeg(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime)
{
	// Calculate the foot orientation
	if (ms_bFootOrientationFixup)
		FixupFootOrientation(solverGoal, legGoal, legSituation, rootAndPelvisSituation, deltaTime);

	// Calculate the foot position (z only)
	if (ms_bFootHeightFixup)
		FixupFootHeight(legGoal, legSituation, rootAndPelvisSituation, deltaTime);

	//WORK OUT KNEE POSITION BASED ON PELVIS AND FOOT POSITION

	static dev_float stretch = 1.0f;

	crSkeleton& skel = *m_pPed->GetSkeleton();
	const crSkeletonData& skelData = skel.GetSkeletonData();
	Vector3	vThighToCalfInBindPose = RCC_VECTOR3(skelData.GetBoneData(solverGoal.m_BoneIdx[crIKSolverLegs::CALF])->GetDefaultTranslation());
	float fThighLenInBindPose = vThighToCalfInBindPose.Mag() * stretch;
	ikAssert(rage::FPIsFinite(fThighLenInBindPose));

	Vector3	vCalfToFootInBindPose = RCC_VECTOR3(skelData.GetBoneData(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT])->GetDefaultTranslation());
	float fCalfLenInBindPose = vCalfToFootInBindPose.Mag() * stretch;
	ikAssert(rage::FPIsFinite(fCalfLenInBindPose));

	// foot is p0, hip is p1  knee is p3 , p2 is a point that forms 2 right handed tris, find p3 given length of two sides of triangle
	// http://astronomy.swin.edu.au/~pbourke/geometry/2circle/

	Vector3	vFootToThigh = legSituation.thighGlobalMtx.d-legSituation.footGlobalMtx.d;
	float legLenInBindPose = fCalfLenInBindPose+fThighLenInBindPose;
	float d = vFootToThigh.Mag();
	if(d>legLenInBindPose)
	{
		vFootToThigh.Normalize();
		vFootToThigh = vFootToThigh  * (legLenInBindPose - ms_fRollbackEpsilon);
		legSituation.footGlobalMtx.d = legSituation.thighGlobalMtx.d - vFootToThigh;
		d = vFootToThigh.Mag();
	}

	{
		float a = ((fCalfLenInBindPose*fCalfLenInBindPose)-(fThighLenInBindPose*fThighLenInBindPose)+d*d)/(2.0f*d);

		// P2 =p0+a(p1-p0)/d
		Vector3 P2 = legSituation.footGlobalMtx.d+a*(vFootToThigh)/d;
		ikAssert(d>0.0f);
		float hSquared = fCalfLenInBindPose*fCalfLenInBindPose-a*a;
		float h;
		if(hSquared<=0.0f)
		{
			a=fCalfLenInBindPose;
			h=0.0f;
		}
		else
		{
			h = sqrtf(hSquared); //radius of intersection of 2 spheres
			ikAssert(rage::FPIsFinite(h));
		}

		// The Knee joint is h distance from P2, we just need a direction to go in.
		Vector3	hVector;
		vFootToThigh.Normalize();
		hVector.Cross(vFootToThigh,legSituation.calfGlobalMtx.c);

		// hvector needs to be perpendicular to vFootToHip, 
		// currently it could be tilted a bit out, so get cross product, then again to get the original vector properly perp
		Vector3 Perp = hVector;
		Perp.Cross(-vFootToThigh);
		Vector3	Perp2;
		Perp2.Cross(Perp,vFootToThigh);
		hVector = Perp2;
		hVector.Normalize();

		ikAssert(rage::FPIsFinite(hVector.x));
		ikAssert(rage::FPIsFinite(hVector.y));
		ikAssert(rage::FPIsFinite(hVector.z));
		hVector *= h;

		legSituation.calfGlobalMtx.d = hVector+P2;
		ikAssert(rage::FPIsFinite(legSituation.calfGlobalMtx.d.x));
		ikAssert(rage::FPIsFinite(legSituation.calfGlobalMtx.d.y));
		ikAssert(rage::FPIsFinite(legSituation.calfGlobalMtx.d.z));
	}

	//SAVE BACK THE NEW GLOBAL MATRICES

	//save altered bone transforms back to skeleton
	skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(legSituation.footGlobalMtx));
	skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(legSituation.calfGlobalMtx));

	m_pPed->GetIkManager().OrientateBoneATowardBoneB(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], solverGoal.m_BoneIdx[crIKSolverLegs::CALF]); // point the hip and knee
	m_pPed->GetIkManager().OrientateBoneATowardBoneB(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], solverGoal.m_BoneIdx[crIKSolverLegs::FOOT]);

	//update the toe based on the foot transform
	//RestoreBoneToZeroPoseRelativeToParent(legGoal.m_uToeBoneIdx, legGoal.m_uFootBoneIdx);
	legSituation.toeGlobalMtx = RC_MATRIX34(skel.GetLocalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE]));
	legSituation.toeGlobalMtx.Dot(legSituation.footGlobalMtx);
	skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(legSituation.toeGlobalMtx));

	// Clamp the maximum rotations of the foot and toe bones
	if (ms_bClampFootOrientation && !m_pPed->GetPedResetFlag( CPED_RESET_FLAG_DoNotClampFootIk ))
	{
		// Get the toe bone matrix (in global space)
		Matrix34 ltoeBoneGlobalMat;
		skel.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(ltoeBoneGlobalMat));

		// Get the foot bone matrix (in global space)
		Matrix34 lfootBoneGlobalMat;
		skel.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(lfootBoneGlobalMat));

		// Get the calf bone matrix (in global space)
		Matrix34 lcalfBoneGlobalMat;
		skel.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(lcalfBoneGlobalMat));

		// Calculate the foot bone matrix (in local space)
		Matrix34 lfootBoneLocalMat(lfootBoneGlobalMat);
		lfootBoneLocalMat.Dot3x3Transpose(lcalfBoneGlobalMat);

		// Calculate the toe bone matrix (in local space)
		Matrix34 ltoeBoneLocalMat(ltoeBoneGlobalMat);
		ltoeBoneLocalMat.DotTranspose(lfootBoneGlobalMat);

		Vector3 euler;
		Vector3 vMin, vMax;

		GetLimits(RC_VEC3V(vMin), RC_VEC3V(vMax));

		// Clamp the foot bone in euler space?
		lfootBoneLocalMat.ToEulersXYZ(euler);
		euler.x = MAX(vMin.x, euler.x);
		euler.x = MIN(vMax.x, euler.x);
		euler.y = MAX(vMin.y, euler.y);
		euler.y = MIN(vMax.y, euler.y);
		euler.z = MAX(vMin.z, euler.z);
		euler.z = MIN(vMax.z, euler.z);
		lfootBoneLocalMat.FromEulersXYZ(euler);

		// Calculate the new foot bone matrix (in global space)
		lfootBoneGlobalMat.Set3x3(lfootBoneLocalMat);
		lfootBoneGlobalMat.Dot3x3(lcalfBoneGlobalMat);

		// Calculate the new foot bone matrix (in global space)
		ltoeBoneGlobalMat.Set(ltoeBoneLocalMat);
		ltoeBoneGlobalMat.Dot(lfootBoneGlobalMat);

		//save back the left foot global matrices
		skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(ltoeBoneGlobalMat));
		skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(lfootBoneGlobalMat));
		skel.SetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(lcalfBoneGlobalMat));
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::SetLeg(crIKSolverLegs::Goal& solverGoal, LegGoal &legGoal, LegSituation &legSituation, RootAndPelvisSituation &rootAndPelvisSituation, float deltaTime)
{
	crSkeleton& skeleton = *m_pPed->GetSkeleton();

	// Calculate the foot orientation
	if (ms_bFootOrientationFixup)
		FixupFootOrientation(solverGoal, legGoal, legSituation, rootAndPelvisSituation, deltaTime);

	// Calculate the foot position (z only)
	if (ms_bFootHeightFixup)
		FixupFootHeight(legGoal, legSituation, rootAndPelvisSituation, deltaTime);

	solverGoal.m_Enabled = true;
	solverGoal.m_Flags = crIKSolverLegs::Goal::USE_ORIENTATION;

	if (ms_bClampFootOrientation && !m_pPed->GetPedResetFlag( CPED_RESET_FLAG_DoNotClampFootIk ))
	{
		solverGoal.m_Flags |= crIKSolverLegs::Goal::USE_LIMITS;
	}

	// Transform to object space
	UnTransformFull(solverGoal.m_Target, *skeleton.GetParentMtx(), RC_MAT34V(legSituation.footGlobalMtx));
	ikAssertf(IsFiniteAll(solverGoal.m_Target), "solverGoal.m_Target not finite");
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::RestoreBoneToZeroPoseRelativeToParent(int ChildBoneIdx, int ParentBoneIdx)
{
	crSkeleton& skel = *m_pPed->GetSkeleton();
	const crSkeletonData& skelData = skel.GetSkeletonData();

	Matrix34 MatParentGlobal;
	skel.GetGlobalMtx(ParentBoneIdx, RC_MAT34V(MatParentGlobal));
	Matrix34 MatChildGlobal;
	skel.GetGlobalMtx(ChildBoneIdx, RC_MAT34V(MatChildGlobal));

	Matrix34 ChildOriRelative(RCC_MATRIX34(skelData.GetDefaultTransform(ChildBoneIdx)));

	// MatParent is in world space
	ChildOriRelative.Dot(MatParentGlobal);
	MatChildGlobal = ChildOriRelative;

	//save back the child matrix
	skel.SetGlobalMtx(ChildBoneIdx, RC_MAT34V(MatChildGlobal));
}


//////////////////////////////////////////////////////////////////////////
//
// Updates the blend values for the torso and right and left legs,
// based on the state of the legs (i.e. are they on the ground)
// and the blend out flag.
//
// RETURNS: true if the ik is fully blended out (right and left leg and pelvis)

bool CLegIkSolver::UpdateBlends(bool bBlendOut, bool bLeftFootOnFloor, bool bRightFootOnFloor, RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime)
{
	if (!rootAndPelvisSituation.bNearEdge FPS_MODE_SUPPORTED_ONLY(|| m_bFPSMode) || rootAndPelvisSituation.bOnPhysical)
	{
		m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend -= ms_fNearEdgePelvisInterpScaleRate * deltaTime;
		m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend = Max(m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend, 0.0f);
	}
	else
	{
		m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend = 1.0f;
	}

	const float fOnSlopeLocoBlendRate = !rootAndPelvisSituation.bInstantSetup ? ms_fOnSlopeLocoBlendRate : INSTANT_BLEND_IN_DELTA;
	m_rootAndPelvisGoal.m_fOnSlopeLocoBlend += (rootAndPelvisSituation.bOnSlopeLoco && rootAndPelvisSituation.bMovingUpwards ? fOnSlopeLocoBlendRate : -fOnSlopeLocoBlendRate) * deltaTime;
	m_rootAndPelvisGoal.m_fOnSlopeLocoBlend = Clamp(m_rootAndPelvisGoal.m_fOnSlopeLocoBlend, 0.0f, 1.0f);

	LegGoal &leftLegGoal = m_LegGoalArray[LEFT_LEG];
	LegGoal &rightLegGoal = m_LegGoalArray[RIGHT_LEG];

	float fBlendRate = ms_afSolverBlendRate[GetMode() == LEG_IK_MODE_PARTIAL];
	
	// Calc override blend rate (if available).
	if (m_pProxy->GetOverriddenBlendTime() > 0.0f)
	{
		fBlendRate = 1.0f/m_pProxy->GetOverriddenBlendTime();
	}

	// Determine the blend delta
	float fBlendDelta = !rootAndPelvisSituation.bInstantSetup && m_pPed->GetIsVisibleInSomeViewportThisFrame() ? deltaTime * fBlendRate : INSTANT_BLEND_IN_DELTA;

	// If the left foot is not on the floor or we don't want any ik
	if(bBlendOut || !bLeftFootOnFloor)
	{
		// Blend out the left foot ik
		leftLegGoal.m_fLegBlend = MAX(leftLegGoal.m_fLegBlend-fBlendDelta,0.0f);
	}
	else
	{
		// Blend in the left foot ik
		leftLegGoal.m_fLegBlend = MIN(leftLegGoal.m_fLegBlend+fBlendDelta,1.0f);
	}

	// If the right foot is not on the floor or we don't want any ik
	if(bBlendOut || !bRightFootOnFloor)
	{
		// Blend out the right foot ik
		rightLegGoal.m_fLegBlend = MAX(rightLegGoal.m_fLegBlend-fBlendDelta,0.0f);
	}
	else
	{
		// Blend in the right foot ik
		rightLegGoal.m_fLegBlend = MIN(rightLegGoal.m_fLegBlend+fBlendDelta,1.0f);
	}

	// If we don't want any ik
	if( bBlendOut || !m_rootAndPelvisGoal.m_bEnablePelvis )
	{
		// Blend out the pelvis ik
		m_rootAndPelvisGoal.m_fPelvisBlend = MAX(m_rootAndPelvisGoal.m_fPelvisBlend-fBlendDelta,0.0f);
	}
	else
	{
		// Blend in the pelvis ik
		m_rootAndPelvisGoal.m_fPelvisBlend = MIN(m_rootAndPelvisGoal.m_fPelvisBlend+fBlendDelta,1.0f);
	}

	// Has the pelvis and leg ik finished blending out?
	if ( bBlendOut && !(leftLegGoal.m_fLegBlend > 0.0f || rightLegGoal.m_fLegBlend > 0.0f || m_rootAndPelvisGoal.m_fPelvisBlend > 0.0f) )
	{
		// Blend out the pelvis and leg ik instantly
		Reset();

		CIkManager& ikManager = m_pPed->GetIkManager();

		// Leg ik is on by default
		//ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_OFF);

		// Blending out slowly (rather than instantly) is on by default
		//ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);

		ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_ON);
		ikManager.ClearFlag(PEDIK_LEGS_USE_ANIM_ALLOW_TAGS);
		ikManager.ClearFlag(PEDIK_LEGS_USE_ANIM_BLOCK_TAGS);
		ikManager.ClearFlag(PEDIK_LEGS_INSTANT_SETUP);

		m_pProxy->SetComplete(true);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
#if FPS_MODE_SUPPORTED
void CLegIkSolver::UpdateFirstPersonState(float deltaTime)
{
	const float fRequestedPelvisOffset = m_pProxy->m_Request.GetPelvisOffset();

	if (deltaTime == 0.0f)
	{
		// Hold previous state
	#if __IK_DEV
		if (CaptureDebugRender())
		{
			char debugText[64];
			sprintf(debugText, "PelvisDeltaZExternal = %6.3f FP: %d %d", fRequestedPelvisOffset, m_bFPSMode ? 1 : 0, m_bFPSModePrevious ? 1 : 0);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif // __IK_DEV

		return;
	}

	// Cache valid delta
	m_fDeltaTime = deltaTime;

	m_bFPSMode = m_pPed->IsFirstPersonShooterModeEnabledForPlayer(false);

	if (m_bFPSMode && (m_pPed->GetMotionData()->GetWasUsingStealthInFPS() != m_pPed->GetMotionData()->GetIsUsingStealthInFPS()))
	{
		m_bFPSStealthToggled = true;
	}

	m_bFPSModeToggled = false;

	if (m_bFPSMode != m_bFPSModePrevious)
	{
		m_bFPSModeToggled = true;

		// Cannot blend pelvis offset on first person state change
		float fExternalPelvisOffsetCorrection = 0.0f;

		if (m_bFPSMode)
		{
			fExternalPelvisOffsetCorrection = Clamp((m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed + fRequestedPelvisOffset), ms_fPelvisMaxNegativeDeltaZStanding, ms_fPelvisMaxPositiveDeltaZStanding) - m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed;
			m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied = fExternalPelvisOffsetCorrection;
		}
		else
		{
			fExternalPelvisOffsetCorrection = -m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied;
			m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied = 0.0f;
		}

		m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed += fExternalPelvisOffsetCorrection;

		// Force an instant update
		m_pPed->GetIkManager().SetFlag(PEDIK_LEGS_INSTANT_SETUP);
	}

	m_rootAndPelvisGoal.m_fExternalPelvisOffset = fRequestedPelvisOffset;

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[64];
		sprintf(debugText, "PelvisDeltaZExternal = %6.3f FP: %d %d", fRequestedPelvisOffset, m_bFPSMode ? 1 : 0, m_bFPSModePrevious ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	m_bFPSModePrevious = m_bFPSMode;
}
#endif // FPS_MODE_SUPPORTED

//////////////////////////////////////////////////////////////////////////
// Returns the difference in z coordinate of the two bones in object space
// boneIndex1 -
// boneIndex2 - 

float CLegIkSolver::GetDistanceBetweenBones(int bone1, int bone2)
{
	Matrix34& boneMatrix1 = RC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(bone1));
	Matrix34& boneMatrix2 = RC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(bone2));

	Vector3 delta = boneMatrix1.d - boneMatrix2.d;
	return delta.Mag();
}

#if __IK_DEV
//////////////////////////////////////////////////////////////////////////
void CLegIkSolver::DebugRenderLowerBody(const crSkeleton &skeleton, Color32 sphereColor, Color32 lineColor)
{
	Vector3	sw = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetA())*ms_fXOffset;

	// static Color32 colourSphereBeforeIk(Color_blue1);
	// static Color32 colourLineBeforeIk(Color_blue4);

	int BonePelvisIdx = m_pPed->GetBoneIndexFromBoneTag(BONETAG_PELVIS);
	Matrix34 Root;
	Matrix34 Pelvis;
	skeleton.GetGlobalMtx(0, RC_MAT34V(Root));
	skeleton.GetGlobalMtx(BonePelvisIdx, RC_MAT34V(Pelvis));
	ms_debugHelper.Sphere(sw+Root.d, ms_debugSphereScale, sphereColor);
	ms_debugHelper.Sphere(sw+Pelvis.d, ms_debugSphereScale, sphereColor);

	for (int i=0; i<NUM_LEGS; i++)
	{
		crIKSolverLegs::Goal& solverGoal = GetGoal(i);
		LegGoal &legGoal = m_LegGoalArray[i];

		Matrix34 Thigh;
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], RC_MAT34V(Thigh));
		Matrix34 Knee;
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF], RC_MAT34V(Knee));
		Matrix34 Foot;
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT], RC_MAT34V(Foot));
		Matrix34 Toe;
		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE], RC_MAT34V(Toe));
		ms_debugHelper.Line(sw+Pelvis.d, sw + Thigh.d, lineColor, lineColor);
		ms_debugHelper.Line(sw+Thigh.d, sw+ Knee.d, lineColor, lineColor);
		ms_debugHelper.Line(sw+Knee.d, sw+Foot.d, lineColor, lineColor);
		ms_debugHelper.Line(sw+Foot.d, sw+Toe.d, lineColor, lineColor);
		ms_debugHelper.Sphere(sw+Knee.d, ms_debugSphereScale, sphereColor);
		ms_debugHelper.Sphere(sw+Foot.d, ms_debugSphereScale, sphereColor);
		ms_debugHelper.Sphere(sw+Toe.d ,ms_debugSphereScale, sphereColor);
		
		if (legGoal.m_bLockFootZ && legGoal.m_bLockedFootZCoordValid)
		{
			Vector3 LockedTargetPos = sw+Foot.d;
			LockedTargetPos.z = legGoal.m_fLockedFootZCoord;
			ms_debugHelper.Sphere(LockedTargetPos, ms_debugSphereScale, Color_orange);
		}
	}
}
#endif // __IK_DEV
//////////////////////////////////////////////////////////////////////////

/*void CLegIkSolver::WorkOutRootAndPelvisSituationUsingProbe(RootAndPelvisSituation& rootAndPelvisSituation)
{
	// get the root global matrix
	m_pPed->GetSkeleton()->GetGlobalMtx(0, RC_MAT34V(rootAndPelvisSituation.rootGlobalMtx));

	// get the normal and position of a line probe dropped down from the root
	static dev_float fRootProbeAbove = 0.0f;
	static dev_float fRootProbeBelow = 1.4f;
	Vector3 vAbove(0.0f, 0.0f, fRootProbeAbove);
	Vector3 vBelow(0.0f, 0.0f, fRootProbeBelow);
	Vector3 vStartOfRootProbe(VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()) + vAbove);
	Vector3 vEndOfRootProbe(VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()) - vBelow);
	CalcRootGroundIntersection(vStartOfRootProbe, vEndOfRootProbe, rootAndPelvisSituation);

	rootAndPelvisSituation.fAnimZRoot = rootAndPelvisSituation.rootGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;

	// get the height (z) of the pelvis if the floor was flat
	//rootAndPelvisSituation.fAnimZPelvis = rootAndPelvisSituation.pelvisGlobalMtx - rootAndPelvisSituation.vGroundPosition.z;
}*/

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::WorkOutRootAndPelvisSituationUsingPedCapsule(RootAndPelvisSituation& rootAndPelvisSituation, float deltaTime)
{
	// Root global
	m_pPed->GetSkeleton()->GetGlobalMtx(0, RC_MAT34V(rootAndPelvisSituation.rootGlobalMtx));

	// Cover
	rootAndPelvisSituation.bCover = false;
	rootAndPelvisSituation.bLowCover = false;
	rootAndPelvisSituation.bCoverAim = false;
	if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_UsingCoverPoint))
	{
		CCoverPoint* pCoverPoint = m_pPed->GetCoverPoint();
		rootAndPelvisSituation.bCover = pCoverPoint != NULL;
		rootAndPelvisSituation.bLowCover = pCoverPoint && (pCoverPoint->GetHeight() != CCoverPoint::COVHEIGHT_TOOHIGH) && !m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsAimingFromCover);
		rootAndPelvisSituation.bCoverAim = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsDoingCoverAimIntro) ||
										   m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsAimingFromCover)	 ||
										   m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsDoingCoverAimOutro);
	}

	// Movement
	const CPhysical* pPhysical = m_pPed->GetGroundPhysical();
	rootAndPelvisSituation.bOnPhysical = pPhysical != NULL;
	rootAndPelvisSituation.bOnMovingPhysical = pPhysical && (pPhysical->GetIsInWater() || pPhysical->GetWasInWater() || (pPhysical->GetVelocity().Mag2() > 0.0f) || (pPhysical->GetAngVelocity().Mag2() > 0.0f));

	Vector3 vVelocity(m_pPed->GetVelocity());
	if (pPhysical)
	{
		vVelocity -= pPhysical->GetVelocity();
	}
	rootAndPelvisSituation.fSpeed = vVelocity.Mag();
	rootAndPelvisSituation.bMoving = rootAndPelvisSituation.fSpeed > ms_fMovingTolerance;

	if (rootAndPelvisSituation.bCover)
	{
		const CTaskMotionInCover* pMotionTask = static_cast<CTaskMotionInCover*>(m_pPed->GetPedIntelligence()->FindTaskActiveByType(CTaskTypes::TASK_MOTION_IN_COVER));
		if (pMotionTask)
		{
			rootAndPelvisSituation.bMoving = (pMotionTask->GetState() == CTaskMotionInCover::State_Moving);
		}
	}

	// Stairs/Slopes/Edges
	rootAndPelvisSituation.bOnStairs = m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_OnStairs);
	rootAndPelvisSituation.bOnSlopeLoco = m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_SlopeDetected) && !rootAndPelvisSituation.bOnStairs && rootAndPelvisSituation.bMoving;
	rootAndPelvisSituation.bNearEdge = m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_EdgeDetected);

	rootAndPelvisSituation.bInVehicle = false;
	rootAndPelvisSituation.bInVehicleReverse = false;
	rootAndPelvisSituation.bEnteringVehicle = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsEnteringVehicle) && m_pPed->IsInOrAttachedToCar();

	if ((m_pPed->GetGroundPos().z == PED_GROUNDPOS_RESET_Z) || (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle) && !m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsEnteringOrExitingVehicle)))
	{
		Vector3 vPedPosition(VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()));
		Vector3 vGroundNormal(ZAXIS);

		rootAndPelvisSituation.bIntersection = false;

		// No collision height so pretend we are on flat ground
		rootAndPelvisSituation.vGroundPosition = vPedPosition;
		rootAndPelvisSituation.vGroundPosition.z -= m_pPed->GetCapsuleInfo()->GetGroundToRootOffset();

		// No collision normal so assume surface is flat (we could use the normal of the foot from the animation)
		rootAndPelvisSituation.vGroundNormal = vGroundNormal;

		// Check if leg IK is required and attached to a vehicle, in which case, use information from vehicle
		if (m_pPed->GetIsAttached())
		{
			CEntity* pAttachEntity = static_cast<CEntity*>(m_pPed->GetAttachParent());

			if (pAttachEntity && pAttachEntity->GetIsTypeVehicle())
			{
				CVehicle* pVehicle = static_cast<CVehicle*>(pAttachEntity);
				int nbWheels = pVehicle->GetNumWheels();
				int nbWheelsTouching = 0;

				Vector3 vGroundPosition(Vector3::ZeroType);

				for (int wheelIndex = 0; wheelIndex < nbWheels; ++wheelIndex)
				{
					CWheel* pWheel = pVehicle->GetWheel(wheelIndex);

					if (pWheel && pWheel->GetIsTouching())
					{
						vGroundPosition += pWheel->GetHitPos();
						vGroundNormal += pWheel->GetHitNormal();
						nbWheelsTouching++;
					}
				}

				if (nbWheelsTouching > 0)
				{
					const float fInvNbWheelsTouching = 1.0f / (float)nbWheelsTouching;
					vGroundPosition *= fInvNbWheelsTouching;
					vGroundNormal *= fInvNbWheelsTouching;

					vGroundNormal.Normalize();

					rootAndPelvisSituation.vGroundPosition.z = vGroundPosition.z;
					rootAndPelvisSituation.vGroundNormal = vGroundNormal;
					rootAndPelvisSituation.bIntersection = true;
				}

				rootAndPelvisSituation.bInVehicle = true;

				const ScalarV vFwdVelocity(Dot(VECTOR3_TO_VEC3V(pVehicle->GetVelocity()), pVehicle->GetTransform().GetB()));
				rootAndPelvisSituation.bInVehicleReverse = !pVehicle->IsInAir() && (pVehicle->GetThrottle() < 0.0f) && IsLessThanAll(vFwdVelocity, ScalarV(V_ZERO));
			}
		}
		else if (rootAndPelvisSituation.bInstantSetup)
		{
			bool bResult = false;

			// If doing an instant setup, try to find a valid ground normal directly
			WorldProbe::FindGroundZFor3DCoord(TOP_SURFACE, vPedPosition, &bResult, &vGroundNormal);

			if (bResult)
			{
				rootAndPelvisSituation.bIntersection = true;
				rootAndPelvisSituation.vGroundNormal = vGroundNormal;
				ClampNormal(rootAndPelvisSituation.vGroundNormal);
			}
		}
	}
	else
	{
		rootAndPelvisSituation.bIntersection = true;
		rootAndPelvisSituation.vGroundPosition = m_pPed->GetGroundPos();

		const Vector3& vPedGroundNormal = m_pPed->GetGroundNormal();

		if (GetMode() != LEG_IK_MODE_PARTIAL)
		{
			rootAndPelvisSituation.vGroundNormal = vPedGroundNormal;
		}
		else
		{
			bool bUseMaxGroundNormal = true;

			if (rootAndPelvisSituation.bOnSlopeLoco)
			{
				// Slope detected. If the ped is getting onto the slope, try to use the ped's actual ground normal instead of the
				// max ground normal since it helps the forward foot to conform to the slope sooner during the transition when the
				// ped is using partial mode. Must use the ped's max ground normal if the ped is orientated across the perceived slope
				// since the slope may actually be stepped geometry and not a true slope, which will cause partial mode to setup badly.
				const Vector3& vSlopeNormal = m_pPed->GetSlopeNormal();
				const float fSlopeHeading = rage::Atan2f(-vSlopeNormal.x, vSlopeNormal.y);
				const float fHeadingToSlope = fwAngle::LimitRadianAngleSafe(m_pPed->GetCurrentHeading() - fSlopeHeading);
				const float fHeadingToSlopeLimit = 170.0f * DtoR;

				bUseMaxGroundNormal = fabsf(fHeadingToSlope) < fHeadingToSlopeLimit;
			}

			// Use max ground normal if available for partial mode except if slope loco is being used in which case always use the ped ground normal
			// since the max ground normal could still be vertical when getting onto a slope.
			const Vec3V& vPedMaxGroundNormal = m_pPed->GetMaxGroundNormal();
			rootAndPelvisSituation.vGroundNormal = bUseMaxGroundNormal && (IsZeroAll(vPedMaxGroundNormal) == 0) ? RCC_VECTOR3(vPedMaxGroundNormal) : vPedGroundNormal;
		}

		ikAssertf(IsFiniteAll(VECTOR3_TO_VEC3V(rootAndPelvisSituation.vGroundPosition)), "Ped's incoming ground position is invalid");
		ikAssertf(IsFiniteAll(VECTOR3_TO_VEC3V(rootAndPelvisSituation.vGroundNormal)), "Ped's incoming ground normal is invalid");
		ikAssertf(rootAndPelvisSituation.vGroundNormal.IsNonZero(), "Ped's incoming ground normal is zero");

		ClampNormal(rootAndPelvisSituation.vGroundNormal);
	}

	// Smooth incoming ground normal
	CalcSmoothedNormal(rootAndPelvisSituation.vGroundNormal, m_rootAndPelvisGoal.m_vGroundNormalSmoothed, ms_fGroundNormalRate, m_rootAndPelvisGoal.m_vGroundNormalSmoothed, deltaTime, rootAndPelvisSituation.bInstantSetup);
	rootAndPelvisSituation.vGroundNormal = m_rootAndPelvisGoal.m_vGroundNormalSmoothed;

	// Get the height (z) of the pelvis if the floor was flat
	rootAndPelvisSituation.fAnimZRoot = rootAndPelvisSituation.rootGlobalMtx.d.z - rootAndPelvisSituation.vGroundPosition.z;

#if __IK_DEV
	if(CaptureDebugRender() && ms_bRenderSituations)
	{
		static dev_float fNormalScale = 0.4f;
		static dev_float fSphereSize = 0.01f;

		// Render the position of the situation
		ms_debugHelper.Sphere(rootAndPelvisSituation.vGroundPosition, fSphereSize, Color_green, true);

		// Render the normal of the situation
		ms_debugHelper.Line(rootAndPelvisSituation.vGroundPosition, rootAndPelvisSituation.vGroundPosition + (rootAndPelvisSituation.vGroundNormal * fNormalScale), Color_purple);
	}
#endif //__IK_DEV
}
//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::ApplySupportingFootMode( 
	eSupportingFootMode supportingFootMode,
	LegSituation &leftLegSituation, LegSituation &rightLegSituation,
	LegGoal& UNUSED_PARAM(leftLegGoal), LegGoal& UNUSED_PARAM(rightLegGoal))
{
	const u16 uLeftFootBoneIdx = GetGoal(LEFT_LEG).m_BoneIdx[crIKSolverLegs::FOOT];
	const u16 uRightFootBoneIdx = GetGoal(RIGHT_LEG).m_BoneIdx[crIKSolverLegs::FOOT];

	switch	(supportingFootMode)
	{
		case FURTHEST_BACKWARD_FOOT:
		{
			const Matrix34& leftFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uLeftFootBoneIdx));
			const Matrix34& rightFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uRightFootBoneIdx));
			if (leftFootObjMtx.d.y < rightFootObjMtx.d.y)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		case FURTHEST_FORWARD_FOOT:
		{
			const Matrix34& leftFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uLeftFootBoneIdx));
			const Matrix34& rightFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uRightFootBoneIdx));
			if (leftFootObjMtx.d.y > rightFootObjMtx.d.y)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		case HIGHEST_FOOT:
		{
			const Matrix34& leftFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uLeftFootBoneIdx));
			const Matrix34& rightFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uRightFootBoneIdx));
			if (leftFootObjMtx.d.z > rightFootObjMtx.d.z)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		case LOWEST_FOOT:
		{
			const Matrix34& leftFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uLeftFootBoneIdx));
			const Matrix34& rightFootObjMtx = RCC_MATRIX34(m_pPed->GetSkeleton()->GetObjectMtx(uRightFootBoneIdx));
			if (leftFootObjMtx.d.z < rightFootObjMtx.d.z)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		case HIGHEST_FOOT_INTERSECTION:
		{
			if (leftLegSituation.vGroundPosition.z > rightLegSituation.vGroundPosition.z)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		case LOWEST_FOOT_INTERSECTION:
		{
			if (leftLegSituation.vGroundPosition.z < rightLegSituation.vGroundPosition.z)
			{
				leftLegSituation.bSupporting = true;
				rightLegSituation.bSupporting = false;
			}
			else
			{
				leftLegSituation.bSupporting = false;
				rightLegSituation.bSupporting = true;
			}
		}
		break;

		default: 
		{
			leftLegSituation.bSupporting = false;
			rightLegSituation.bSupporting = false;
		}
		break;

		case MOST_RECENT_LOCKED_Z:
			{
				if (m_LastLockedZ == LEFT_LEG)
				{
					leftLegSituation.bSupporting = true;
					rightLegSituation.bSupporting = false;
				}
				else
				{
					leftLegSituation.bSupporting = false;
					rightLegSituation.bSupporting = true;
				}
			}
			break;
	}

};

//////////////////////////////////////////////////////////////////////////

bool CLegIkSolver::UseMainThread() const
{
	// Force main thread update if ped is not visible since the motion tree 
	// update will be skipped, but parts of the solver still need to be processed
	if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
	{
		return true;
	}

	// Force main thread update if a react is active so that the goal positions
	// of the feet are calculated prior to the react solver running (since it may 
	// modify the root and pelvis).
	const CIkManager& ikManager = m_pPed->GetIkManager();
	if (ikManager.IsActive(IKManagerSolverTypes::ikSolverTypeTorsoReact) || 
		ikManager.IsActive(IKManagerSolverTypes::ikSolverTypeBodyRecoil))
	{
		return true;
	}

	// Force main thread update so that probes can be used to place the feet properly while
	// the ped is on stairs (partial mode solver is not accurate enough).
	if ((GetMode() == LEG_IK_MODE_PARTIAL) && 
		(m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_OnStairs)) && 
		!ikManager.IsActive(IKManagerSolverTypes::ikSolverTypeRootSlopeFixup) &&
		ms_bUseMultipleAsyncShapeTests)
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

u32 CLegIkSolver::GetMode() const
{
	u32 uMode = m_pPed->m_PedConfigFlags.GetPedLegIkMode();

#if __IK_DEV
	if (ms_bForceLegIk)
	{
		uMode = (u32)ms_nForceLegIkMode;
	}
#endif // __IK_DEV

	return uMode;
}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::GetFootRadius(RootAndPelvisSituation& rootAndPelvisSituation) const
{
	int index = (rootAndPelvisSituation.bOnStairs && !rootAndPelvisSituation.bMovingUpwards) || rootAndPelvisSituation.bOnPhysical || rootAndPelvisSituation.bInVehicle ? 1 : 0;
	return ms_afFootRadius[index];
}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::GetFootMaxPositiveDeltaZ(RootAndPelvisSituation& rootAndPelvisSituation) const
{
	int index = 0;

	if ((rootAndPelvisSituation.bOnSlope && !rootAndPelvisSituation.bOnStairs) || (GetMode() == LEG_IK_MODE_FULL_MELEE))
	{
		index = 2;
	}
	else if (rootAndPelvisSituation.bOnPhysical)
	{
		index = 1;
	}

	return ms_afFootMaxPositiveDeltaZ[index];
}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::GetMinDistanceFromFootToRoot(RootAndPelvisSituation& rootAndPelvisSituation) const
{
	if (m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsLanding))
	{
		return ms_fMinDistanceFromFootToRootForLanding;
	}
	else if (rootAndPelvisSituation.bLowCover)
	{
		return ms_fMinDistanceFromFootToRootForCover;
	}

	return ms_fMinDistanceFromFootToRoot;
}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::GetHeightOfAnimRootAboveCollision(const RootAndPelvisSituation& rootAndPelvisSituation, const LegSituation& supportingLegSituation) const
{
	float fHeightOfAnimFootAboveCollision = supportingLegSituation.fAnimZFoot * ms_fAnimFootZBlend;
	float fMinHeightOfAnimFootAboveCollision = m_afHeightOfBindPoseFootAboveCollision[HEEL];

	if (supportingLegSituation.fAnimZToe < supportingLegSituation.fAnimZFoot)
	{
		fMinHeightOfAnimFootAboveCollision = m_afHeightOfBindPoseFootAboveCollision[BALL] + (supportingLegSituation.fAnimZFoot - supportingLegSituation.fAnimZToe);
	}

	return MAX(fMinHeightOfAnimFootAboveCollision, fHeightOfAnimFootAboveCollision) + (rootAndPelvisSituation.fAnimZRoot - supportingLegSituation.fAnimZFoot);
}

//////////////////////////////////////////////////////////////////////////

bool CLegIkSolver::FixupPelvis(RootAndPelvisSituation &rootAndPelvisSituation, LegSituation &leftLegSituation, LegSituation &rightLegSituation, float deltaTime)
{
#if __IK_DEV
	char debugText[100];
	if (CaptureDebugRender())
	{
		sprintf(debugText, "LegIKSolver::FixupPelvis\n");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	// Work out the supporting matrices
	Vector3 vSupportingGroundPosition;
	Vector3 vSupportingFootPosition;
	float fHeightOfAnimRootAboveCollision;

	if (leftLegSituation.bSupporting)
	{
		vSupportingGroundPosition = leftLegSituation.vGroundPosition;
		vSupportingFootPosition = leftLegSituation.vGroundPosition;
		vSupportingFootPosition.z = leftLegSituation.footGlobalMtx.d.z;

		fHeightOfAnimRootAboveCollision = GetHeightOfAnimRootAboveCollision(rootAndPelvisSituation, leftLegSituation);
	}
	else
	{
		vSupportingGroundPosition = rightLegSituation.vGroundPosition;
		vSupportingFootPosition = rightLegSituation.vGroundPosition;
		vSupportingFootPosition.z = rightLegSituation.footGlobalMtx.d.z;

		fHeightOfAnimRootAboveCollision = GetHeightOfAnimRootAboveCollision(rootAndPelvisSituation, rightLegSituation);
	}

#if __IK_DEV
	// Render the supporting ground position and the supporting foot position
	if (CaptureDebugRender() && ms_bRenderSupporting)
	{
		ms_debugHelper.Sphere(vSupportingFootPosition , 0.02f, Color_DarkOrchid, true);
		ms_debugHelper.Sphere(vSupportingGroundPosition, 0.02f, Color_DarkSeaGreen, true);
		if (leftLegSituation.bSupporting)
		{
			ms_debugHelper.Text3d(vSupportingFootPosition , Color_DarkOrchid, 0, 0, "SFL" , true);
			ms_debugHelper.Text3d(vSupportingGroundPosition, Color_DarkSeaGreen, 0, 0, "SGL" , true);
		}
		else
		{
			ms_debugHelper.Text3d(vSupportingFootPosition , Color_DarkOrchid, 0, 0, "SFR" , true);
			ms_debugHelper.Text3d(vSupportingGroundPosition, Color_DarkSeaGreen, 0, 0, "SGR" , true);
		}
	}
#endif //__IK_DEV

	// Use the supporting height to calculate the pelvis target height
	float fTargetPelvisWorldZ;
	if ((rootAndPelvisSituation.bOnStairs && rootAndPelvisSituation.bMoving && !rootAndPelvisSituation.bMovingUpwards) || (!rootAndPelvisSituation.bMoving && rootAndPelvisSituation.bOnPhysical && (GetMode() != LEG_IK_MODE_PARTIAL)))
	{
		// Use the animated distance of the supporting foot from the root instead of the distance of the root relative to the ped's ground position.
		// Keeps the pelvis higher when descending stairs to prevent the trailing leg from being overly bent.
		// Keeps the pelvis at the correct height when partially standing on/straddling a physical object.
		fTargetPelvisWorldZ = vSupportingGroundPosition.z + fHeightOfAnimRootAboveCollision;
	}
	else
	{
		fTargetPelvisWorldZ = vSupportingGroundPosition.z + rootAndPelvisSituation.fAnimZRoot;
	}

	// Is the height of the ped support lower than the support determined by the probe i.e. is the animation intersecting the ground
	// Assumption here is that the animation should never intersect the floor
	// But it can still happen for instance if you cross blend between a crouch and a standing animation or the character has high heels
	const CPedModelInfo* pPedModelInfo = m_pPed->GetPedModelInfo();
	const bool bHighHeels = pPedModelInfo && (pPedModelInfo->GetExternallyDrivenDOFs() & HIGH_HEELS);
	if (!bHighHeels && (vSupportingFootPosition.z < vSupportingGroundPosition.z))
	{
		fTargetPelvisWorldZ += vSupportingGroundPosition.z - vSupportingFootPosition.z;
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		//sprintf(debugText, "rootAndPelvisSituation.pelvisGlobalMtx.d.z = %6.4f\n", rootAndPelvisSituation.pelvisGlobalMtx.d.z);
		sprintf(debugText, "fTargetPelvisWorldZ = %6.4f\n", fTargetPelvisWorldZ);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	// What is the target pelvis height as a delta from the current root height
	float fPelvisDeltaZ = fTargetPelvisWorldZ - rootAndPelvisSituation.rootGlobalMtx.d.z;

#if __IK_DEV
	if (CaptureDebugRender())
	{
		sprintf(debugText, "fPelvisDeltaZ (pre-clamping) = %6.4f\n", fPelvisDeltaZ);

		float fPelvisMaxNegativeDeltaZ = GetMinPelvisDeltaZ(rootAndPelvisSituation);
		float fPelvisMaxPositiveDeltaZ = GetMaxPelvisDeltaZ(rootAndPelvisSituation);

		if (fPelvisDeltaZ<fPelvisMaxNegativeDeltaZ)
		{
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_blue, debugText);
		}
		else if (fPelvisDeltaZ>fPelvisMaxPositiveDeltaZ)
		{
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
		}
		else
		{
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	}		
#endif //__IK_DEV

	if (rootAndPelvisSituation.bInVehicle)
	{
		fPelvisDeltaZ = 0.0f;
	}
	else if (rootAndPelvisSituation.bMoving)
	{
		if ( m_pPed->IsStrafing() )
		{
			fPelvisDeltaZ = 0.0f;
		}
		else
		{
			// Clamp the result between ms_fPelvisMaxNegativeDeltaZMoving and ms_fPelvisMaxPositiveDeltaZMoving
			fPelvisDeltaZ = MAX(fPelvisDeltaZ,GetMinPelvisDeltaZ(rootAndPelvisSituation));
			fPelvisDeltaZ = MIN(fPelvisDeltaZ,GetMaxPelvisDeltaZ(rootAndPelvisSituation));
		}
	}
	else 
	{
		// Clamp the result between ms_fPelvisMaxNegativeDeltaZStanding and ms_fPelvisMaxPositiveDeltaZStanding
		fPelvisDeltaZ = MAX(fPelvisDeltaZ,ms_fPelvisMaxNegativeDeltaZStanding);
		fPelvisDeltaZ = MIN(fPelvisDeltaZ,ms_fPelvisMaxPositiveDeltaZStanding);
	}

#if FPS_MODE_SUPPORTED
	// Apply externally driven pelvis offset
	if (m_rootAndPelvisGoal.m_fExternalPelvisOffset != 0.0f)
	{
		m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied = Clamp((fPelvisDeltaZ + m_rootAndPelvisGoal.m_fExternalPelvisOffset), ms_fPelvisMaxNegativeDeltaZStanding, ms_fPelvisMaxPositiveDeltaZStanding) - fPelvisDeltaZ;

		fPelvisDeltaZ += m_rootAndPelvisGoal.m_fExternalPelvisOffsetApplied;
	}
#endif // FPS_MODE_SUPPORTED

	if (!rootAndPelvisSituation.bInstantSetup)
	{
		float fRateScale = 1.0f;

		if (rootAndPelvisSituation.bCover && !rootAndPelvisSituation.bCoverAim)
		{
			fRateScale *= 0.5f;
		}

#if FPS_MODE_SUPPORTED
		if (m_bFPSMode)
		{
			TUNE_GROUP_FLOAT(FIRST_PERSON_COVER_TUNE, RATE_SCALE_FOR_COVER, 4.0f, 0.0f, 10.0f, 0.01f);
			TUNE_GROUP_FLOAT(FIRST_PERSON_COVER_TUNE, RATE_SCALE_FOR_COVER_AIM_OUTRO, 4.0f, 0.0f, 10.0f, 0.01f);
			TUNE_GROUP_FLOAT(FIRST_PERSON_STEALTH_TUNE, RATE_SCALE_FOR_STEALTH_TOGGLE, 2.0f, 0.0f, 10.0f, 0.01f);
			if (rootAndPelvisSituation.bCover)
			{
				// This flag had been getting reset post-physics and so was never true by the time leg IK was updated.
				// Commenting it out since it had never been used and now that I've fixed the flag to get reset pre-tasks
				// I don't want this logic to start getting used...
				//if (m_pPed && m_pPed->GetPedResetFlag(CPED_RESET_FLAG_CoverOutroRunning))
				//{
				//	fRateScale *= RATE_SCALE_FOR_COVER_AIM_OUTRO;
				//}
				//else
				{
					fRateScale *= RATE_SCALE_FOR_COVER;
				}
			}
			else if (m_bFPSStealthToggled)
			{
				fRateScale *= RATE_SCALE_FOR_STEALTH_TOGGLE;
			}
			else if (!rootAndPelvisSituation.bMelee)
			{
				fRateScale *= 0.7f;
			}
		}
#endif // FPS_MODE_SUPPORTED

		// Smooth the pelvis movement
		if (rootAndPelvisSituation.bOnMovingPhysical)
		{
			m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_PelvisInterpOnDynamic.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
		}
		else if (rootAndPelvisSituation.bOnStairs)
		{
			if (rootAndPelvisSituation.bCoverAim)
			{
				m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_StairsPelvisInterpCoverAim.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
			}
			else if (rootAndPelvisSituation.bMoving)
			{
				m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_StairsPelvisInterpMoving.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
			}
			else
			{
				m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_StairsPelvisInterp.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
			}
		}
		else
		{
			if (rootAndPelvisSituation.bMoving)
			{
				fRateScale *= Lerp(m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend, 1.0f, ms_fNearEdgePelvisInterpScale);
				m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_PelvisInterpMoving.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
			}
			else
			{
				m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = sm_Tunables.m_PelvisInterp.IncrementTowards(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, fPelvisDeltaZ, m_rootAndPelvisGoal.m_fPelvisDeltaZ, deltaTime, fRateScale);
			}
		}
	}
	else
	{
		m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = fPelvisDeltaZ;
	}

#if FPS_MODE_SUPPORTED
	if(m_bFPSMode && m_bFPSStealthToggled && fabsf(m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed - fPelvisDeltaZ) < SMALL_FLOAT)
	{
		m_bFPSStealthToggled = false;
	}
#endif // FPS_MODE_SUPPORTED

#if __IK_DEV
	if (CaptureDebugRender())
	{
		sprintf(debugText, "fPelvisDeltaZ (post-clamping) = %6.4f\n", fPelvisDeltaZ);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "m_SmoothPelvisDeltaZ (post-smoothing) = %6.4f\n", m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	fPelvisDeltaZ = m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed * m_rootAndPelvisGoal.m_fPelvisBlend;

	if (fabsf(fPelvisDeltaZ) > SMALL_FLOAT)
	{
		crSkeleton& skel = *m_pPed->GetSkeleton();
		const int numberBones = skel.GetBoneCount();

		Vec3V vOffset(Scale(Vec3V(V_UP_AXIS_WZERO), ScalarV(fPelvisDeltaZ)));
		vOffset = UnTransform3x3Ortho(*skel.GetParentMtx(), vOffset);

		for (int i=0; i<numberBones; i++)
		{
			Mat34V& boneMtx = skel.GetObjectMtx(i);
			boneMtx.SetCol3(Add(boneMtx.GetCol3(), vOffset));
		}

		skel.InverseUpdate();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::FixupLegsAndPelvis(float deltaTime)
{
#if __IK_DEV
	char debugText[100];
	if (CaptureDebugRender())
	{
		sprintf(debugText, "%s\n", "LegIKSolver::FixupLegsAndPelvis");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_fHeightOfBindPoseFootAboveCollision(Heel, Ball) = (%6.4f, %6.4f)\n", m_afHeightOfBindPoseFootAboveCollision[HEEL], m_afHeightOfBindPoseFootAboveCollision[BALL]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
	
	// Early out if the skeleton is missing
	crSkeleton* pSkel = m_pPed->GetSkeleton();
	if (!ikVerifyf(pSkel, "No valid skeleton!"))
		return;

	// Early out if any required bones are missing
	if (!m_bBonesValid)
		return;

	CIkManager& ikManager = m_pPed->GetIkManager();

	// Do we want to block the leg ik?
	bool bBlockLegIK = BlockLegIK();
	if(bBlockLegIK)
	{
		// Can we blend out the pelvis and leg ik instantly?
		if ( ikManager.IsFlagSet(PEDIK_LEGS_AND_PELVIS_FADE_OFF) || !(m_LegGoalArray[RIGHT_LEG].m_fLegBlend > 0.0f || m_LegGoalArray[LEFT_LEG].m_fLegBlend > 0.0f || m_rootAndPelvisGoal.m_fPelvisBlend > 0.0f) )
		{
			// Blend out the pelvis and leg ik instantly
			Reset();

			// Leg ik is on by default
			//ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_OFF);

			// Blending out slowly (rather than instantly) is on by default
			//m_pPed->GetIkManager().ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);

			// Clear if leg ik was forced to blend out slowly
			ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_ON);

			ikManager.ClearFlag(PEDIK_LEGS_USE_ANIM_ALLOW_TAGS);
			ikManager.ClearFlag(PEDIK_LEGS_USE_ANIM_BLOCK_TAGS);
			ikManager.ClearFlag(PEDIK_LEGS_INSTANT_SETUP);

			m_pProxy->SetComplete(true);
			return;
		}
	}

	// This is done through PreTaskUpdate() now.
	//	// Leg ik is on by default
	//	ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_OFF);
	//	// Blending out slowly (rather than instantly) is on by default
	//	ikManager.ClearFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);

	bool bAllowLegIK = !bBlockLegIK;

	// Block Leg Ik flag set to true?
	if (ikManager.IsFlagSet(PEDIK_LEGS_USE_ANIM_BLOCK_TAGS))
	{
		bAllowLegIK = true;
		fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
		if (pAnimDirector)
		{
			const CClipEventTags::CLegsIKEventTag* pProp = static_cast<const CClipEventTags::CLegsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LegsIk));
			if (pProp && !pProp->GetAllowed())
			{
				bAllowLegIK = false;
			}
		}
	}
	else if(ikManager.IsFlagSet(PEDIK_LEGS_USE_ANIM_ALLOW_TAGS))
	{
		bAllowLegIK = false;
		fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
		if (pAnimDirector)
		{
			const CClipEventTags::CLegsIKEventTag* pProp = static_cast<const CClipEventTags::CLegsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LegsIk));
			if (pProp && pProp->GetAllowed())
			{
				bAllowLegIK = true;
			}
		}
	}

#if __IK_DEV
	// Render the original bones position and orientation as lines to the right of the character
	if(CaptureDebugRender() && ms_bRenderBeforeIK)
	{
		DebugRenderLowerBody(*pSkel, Color_blue1, Color_blue4);
	}
#endif //__IK_DEV

	u32 legIkMode = GetMode();

	const bool bPartialModeWithStairs = (legIkMode == LEG_IK_MODE_PARTIAL) && 
										ms_bUseMultipleAsyncShapeTests &&
										(m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_StairsDetected) || m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_OnStairs)) &&
										m_pPed->GetIsVisibleInSomeViewportThisFrame();

	bool bUseProbe = (legIkMode == LEG_IK_MODE_FULL) || (legIkMode == LEG_IK_MODE_FULL_MELEE) || bPartialModeWithStairs;

	// Work out the root situation
	RootAndPelvisSituation rootAndPelvisSituation;

	// Check if an instant setup is needed
	rootAndPelvisSituation.bInstantSetup = ikManager.IsFlagSet(PEDIK_LEGS_INSTANT_SETUP);

	if (rootAndPelvisSituation.bInstantSetup)
	{
		// Reset probe positions and velocities
		PartialReset();
	}

	/*if (bUseProbe)
	{
		// Expensive and can't be run on SPU due to physics probes
		// Test a probe from above the root to the ground below:
		// If the probe is intersecting anything calculate the z height of the root above the highest intersection
		// If the probe is not intersecting anything return the z height w.r.t to the entity transform 
		WorkOutRootAndPelvisSituationUsingProbe(rootAndPelvisSituation);
	}
	else
	{
		// Cheap and could be run on SPU (uses pre-existing ped capsule collision data)
		// If the ped capsule is intersecting use its normal and position
		// If the ped capsule is not intersecting (i.e. is not on the ground) return the z height w.r.t to the entity transform
		WorkOutRootAndPelvisSituationUsingPedCapsule(rootAndPelvisSituation);
	}*/

	WorkOutRootAndPelvisSituationUsingPedCapsule(rootAndPelvisSituation, deltaTime);

	// Keeping the stair slope flag sticky since the character can be off the stair slope but still on stair collision
	m_bOnStairSlope |= m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_OnStairSlope);
	if (!rootAndPelvisSituation.bOnStairs)
	{
		m_bOnStairSlope = false;
	}

	// Slopes
	const Vector3& vNormal = !rootAndPelvisSituation.bOnStairs ? rootAndPelvisSituation.vGroundNormal : m_pPed->GetSlopeNormal();
	float fSlopeHeading = 0.0f;
	float fHeadingToSlope = 0.0f;

	rootAndPelvisSituation.fSlopePitch = rage::Atan2f(vNormal.XYMag(), vNormal.z);
	rootAndPelvisSituation.bOnSlope = !rootAndPelvisSituation.bOnPhysical && (fabsf(rootAndPelvisSituation.fSlopePitch) > ms_fOnSlopeTolerance);
	rootAndPelvisSituation.bMovingUpwards = false;

	if (rootAndPelvisSituation.bOnSlope)
	{
		fSlopeHeading = rage::Atan2f(-vNormal.x, vNormal.y);
		fHeadingToSlope = fwAngle::LimitRadianAngleSafe(m_pPed->GetCurrentHeading() - fSlopeHeading);
		rootAndPelvisSituation.bMovingUpwards = ((fHeadingToSlope < -HALF_PI) || (fHeadingToSlope > HALF_PI));
	}

	// Melee
	rootAndPelvisSituation.bMelee = m_pPed->GetPedIntelligence() && (m_pPed->GetPedIntelligence()->GetTaskMeleeActionResult() != NULL);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		sprintf(debugText, "Capsule         : %d", rootAndPelvisSituation.bIntersection ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Instant         : %d", rootAndPelvisSituation.bInstantSetup ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Moving          : %d", rootAndPelvisSituation.bMoving? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Upwards         : %d", rootAndPelvisSituation.bMovingUpwards ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Stairs          : %d", rootAndPelvisSituation.bOnStairs ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Stair Slope     : %d", m_bOnStairSlope ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Slope           : %d", rootAndPelvisSituation.bOnSlope ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Slope Loco      : %d (%5.3f)", rootAndPelvisSituation.bOnSlopeLoco ? 1 : 0, m_rootAndPelvisGoal.m_fOnSlopeLocoBlend);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Edge            : %d (%5.3f)", rootAndPelvisSituation.bNearEdge ? 1 : 0, m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Cover           : %d", rootAndPelvisSituation.bCover ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Low Cover       : %d", rootAndPelvisSituation.bLowCover ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Cover Aim       : %d", rootAndPelvisSituation.bCoverAim ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Vehicle, Rev, En: %d %d %d", rootAndPelvisSituation.bInVehicle ? 1 : 0, rootAndPelvisSituation.bInVehicleReverse ? 1 : 0, rootAndPelvisSituation.bEnteringVehicle ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Physical        : %d", rootAndPelvisSituation.bOnPhysical ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Physical Moving : %d", rootAndPelvisSituation.bOnMovingPhysical ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "Melee           : %d", rootAndPelvisSituation.bMelee ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "fSpeed = %6.2f", rootAndPelvisSituation.fSpeed);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "fSlopePitch = %6.4f", rootAndPelvisSituation.fSlopePitch);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "fSlopeHeading = %6.4f", fSlopeHeading);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "fHeadingToSlope = %6.4f", fHeadingToSlope);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		sprintf(debugText, "PelvisDeltaZSmoothed = %6.4f, PelvisDeltaZ = %6.4f", m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed, m_rootAndPelvisGoal.m_fPelvisDeltaZ);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	// Work out the leg situation
	LegSituation legSituationArray[NUM_LEGS];

	if (bUseProbe && ms_bUseMultipleShapeTests)
	{
		WorkOutLegSituationsUsingMultipleShapeTests(m_LegGoalArray, legSituationArray, rootAndPelvisSituation, deltaTime);
	}
	else
	{
		// For each leg
		for (int i=0; i<NUM_LEGS; i++)
		{
			crIKSolverLegs::Goal& solverGoal = GetGoal(i);
			LegGoal& legGoal = m_LegGoalArray[i];
			LegSituation& legSituation = legSituationArray[i];

			if (bUseProbe)
			{
				// Expensive and can't be run on SPU due to physics probes
				// Test a probe from above the foot to below the foot :
				// If the probe is intersecting anything imply a plane from the normal of the intersection
				// calculate the z height of the feet above this plane
				// If the probe is not intersecting anything return the animated z height of the feet
				WorkOutLegSituationUsingShapeTests(solverGoal, legGoal, legSituation, rootAndPelvisSituation, deltaTime);		
			}
			else
			{
				// Cheap and could be run on SPU (uses pre-existing ped capsule collision data)
				// If the ped capsule is intersecting anything imply a plane from the normal of the intersection
				// calculate the z height of the feet above this plane
				// If the ped capsule is not intersecting anything return the animated z height of the feet
				WorkOutLegSituationUsingPedCapsule(solverGoal, legGoal, legSituation, rootAndPelvisSituation, deltaTime);
			}

			// Reset
			legGoal.bFootFlattened = false;
		}
	}

	for (int i=0; i<NUM_LEGS; i++)
	{
		LegSituation& legSituation = legSituationArray[i];

		//Set the info into the footstep helper.
		m_pPed->GetFootstepHelper().SetGroundInfo(i, VECTOR3_TO_VEC3V(legSituation.vGroundPosition), VECTOR3_TO_VEC3V(legSituation.vGroundNormal));

#if __IK_DEV
		if(CaptureDebugRender() && ms_bRenderSituations)
		{
			static dev_float fNormalScale = 0.4f;
			static dev_float fSphereSize = 0.01f;

			// Render the position of the intersection
			ms_debugHelper.Sphere(legSituation.vGroundPosition, fSphereSize, Color_purple, true);

			// Render the normal of the intersection
			ms_debugHelper.Line(legSituation.vGroundPosition, legSituation.vGroundPosition + (legSituation.vGroundNormal * fNormalScale), Color_purple);
		}
#endif //__IK_DEV
	}

	LegSituation &leftLegSituation = legSituationArray[LEFT_LEG];
	LegSituation &rightLegSituation = legSituationArray[RIGHT_LEG];
	LegGoal& leftLegGoal = m_LegGoalArray[LEFT_LEG];
	LegGoal& rightLegGoal = m_LegGoalArray[RIGHT_LEG];

	const CClipEventTags::CLegsIKOptionsEventTag* pTag = static_cast<const CClipEventTags::CLegsIKOptionsEventTag*>(m_pPed->GetAnimDirector()->FindPropertyInMoveNetworks(CClipEventTags::LegsIkOptions));

	leftLegGoal.m_bLockFootZ = false;
	rightLegGoal.m_bLockFootZ = false;

	if (pTag)
	{
		if (pTag->GetLockFootHeight())
		{
			if (pTag->GetRight())
			{
				rightLegGoal.m_bLockFootZ = true;
				m_LastLockedZ = RIGHT_LEG;
			}
			else
			{
				leftLegGoal.m_bLockFootZ = true;
				m_LastLockedZ = LEFT_LEG;
			}
		}
	}

#if __IK_DEV
	if(CaptureDebugRender())
	{
		sprintf(debugText, "LockLFoot= %s, LockRFoot=%s\n", leftLegGoal.m_bLockFootZ ? "TRUE" : "FALSE", rightLegGoal.m_bLockFootZ ? "TRUE" : "FALSE");
		if (leftLegGoal.m_bLockFootZ)
		{
			sprintf(debugText, "LockLFoot= %s, LockRFoot=%s\n", leftLegGoal.m_bLockFootZ ? "TRUE" : "FALSE", rightLegGoal.m_bLockFootZ ? "TRUE" : "FALSE");
		}

		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	//
	// Update the three blend values (right leg, left leg and pelvis) and exit if completely blended out
	//
	bool bBlendedOut = UpdateBlends(!bAllowLegIK, leftLegSituation.bIntersection, rightLegSituation.bIntersection, rootAndPelvisSituation, deltaTime);
	if (bBlendedOut)
	{
		return;
	}

#if __IK_DEV
	// Render the root and leg situations
	if (CaptureDebugRender())
	{
		//sprintf(debugText, "rootAndPelvisSituation.vWorldGroundPosition.z = %6.4f\n", rootAndPelvisSituation.vGroundPosition.z);
		//ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		//if (ms_bRenderInputNormal)
		//{
		//	ms_debugHelper.Line(rootAndPelvisSituation.vGroundPosition,  rootAndPelvisSituation.vGroundPosition+rootAndPelvisSituation.vGroundNormal, Color_white);
		//}

		// For each leg
		for (int i=0; i<NUM_LEGS; i++)
		{
			LegGoal& legGoal = m_LegGoalArray[i];
			LegSituation& legSituation = legSituationArray[i];

			sprintf(debugText, "Leg %2s: z = %6.4f, Phys = %d, PhysMoving = %d", i == 0 ? "Lt" : "Rt", legSituation.vGroundPosition.z, legSituation.bOnPhysical ? 1 : 0, legSituation.bOnMovingPhysical ? 1 : 0);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

			if (ms_bRenderInputNormal)
			{
				ms_debugHelper.Line(legSituation.vGroundPosition,  legSituation.vGroundPosition+legSituation.vGroundNormal, Color_red);
			}

			if (ms_bRenderSmoothedNormal)
			{
				ms_debugHelper.Line(legSituation.vGroundPosition,  legSituation.vGroundPosition+legGoal.m_vGroundNormalSmoothed, Color_white);
			}
		}
	}
#endif //__IK_DEV

	if (ms_bClampPostiveAnimZ)
	{
		rootAndPelvisSituation.fAnimZRoot = MAX(rootAndPelvisSituation.fAnimZRoot, 0.0f);
		// For each leg
		for (int i=0; i<NUM_LEGS; i++)
		{
			LegSituation& legSituation = legSituationArray[i];
			legSituation.fAnimZFoot = MAX(legSituation.fAnimZFoot, 0.0f);
			legSituation.fAnimZToe = MAX(legSituation.fAnimZToe, 0.0f);
		}
	}

	// Supporting foot mode
	if (rootAndPelvisSituation.bMoving)
	{
		ApplySupportingFootMode((eSupportingFootMode)ms_nNormalMovementSupportingLegMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);

		if (rootAndPelvisSituation.bOnStairs)
		{
			if (rootAndPelvisSituation.bMovingUpwards)
			{
				ApplySupportingFootMode((eSupportingFootMode)ms_nUpStairsSupportingLegMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);
			}
			else
			{
				ApplySupportingFootMode((eSupportingFootMode)ms_nDownStairsSupportingLegMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);
			}
		}
		else if (rootAndPelvisSituation.bOnSlope || rootAndPelvisSituation.bOnSlopeLoco)
		{
			if (rootAndPelvisSituation.bMovingUpwards)
			{
				ApplySupportingFootMode((eSupportingFootMode)ms_nUpSlopeSupportingLegMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);
			}
			else
			{
				ApplySupportingFootMode((eSupportingFootMode)ms_nDownSlopeSupportingLegMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);
			}
		}
	}
	else
	{
		eSupportingFootMode eFootMode = !rootAndPelvisSituation.bLowCover ? (eSupportingFootMode)ms_nNormalStandingSupportingLegMode : (eSupportingFootMode)ms_nLowCoverSupportingLegMode;
		ApplySupportingFootMode(eFootMode, leftLegSituation, rightLegSituation, leftLegGoal, rightLegGoal);
	}

	// blend the pelvis movement in and out as ik is switched on and off
	if ((m_rootAndPelvisGoal.m_bEnablePelvis || (m_rootAndPelvisGoal.m_fPelvisBlend > 0.0f)) && ms_bPelvisFixup)
	{
		// Move the pelvis (and any child bones) by deltaz
		if (FixupPelvis(rootAndPelvisSituation, leftLegSituation, rightLegSituation, deltaTime))
		{
			// Update cached root
			pSkel->GetGlobalMtx(0, RC_MAT34V(rootAndPelvisSituation.rootGlobalMtx));

			// For each leg
			for (int i=0; i<NUM_LEGS; i++)
			{
				crIKSolverLegs::Goal& solverGoal = GetGoal(i);
				LegSituation& legSituation = legSituationArray[i];
				pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::TOE],   RC_MAT34V(legSituation.toeGlobalMtx));
				pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::FOOT],  RC_MAT34V(legSituation.footGlobalMtx));
				pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::CALF],  RC_MAT34V(legSituation.calfGlobalMtx));
				pSkel->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverLegs::THIGH], RC_MAT34V(legSituation.thighGlobalMtx));
			}
		}
	}
	else
	{
		m_rootAndPelvisGoal.m_fPelvisDeltaZSmoothed = 0.0f;
	}

#if __IK_DEV
	// Render the after pelvis height adjustment
	if(CaptureDebugRender() && ms_bRenderAfterPelvis)
	{
		DebugRenderLowerBody(*pSkel, Color_green1, Color_green4);
		sprintf(debugText, "Smooth left foot Z= %6.4f, left foot delta z (acceleration based)= %6.4f\n", m_LegGoalArray[LEFT_LEG].m_fFootDeltaZSmoothed, m_LegGoalArray[LEFT_LEG].m_fFootDeltaZ);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "Smooth right foot Z= %6.4f, right foot delta z (acceleration based)= %6.4f\n", m_LegGoalArray[RIGHT_LEG].m_fFootDeltaZSmoothed, m_LegGoalArray[RIGHT_LEG].m_fFootDeltaZ);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	if (ms_bLegFixup)
	{
		for (int i=0; i<NUM_LEGS; i++)
		{
			SetLeg(GetGoal(i), m_LegGoalArray[i], legSituationArray[i], rootAndPelvisSituation, deltaTime);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool CLegIkSolver::CapsuleTest(const Vector3 &vStart, const Vector3 &vFinish, float radius, int iShapeTestIncludeFlags, WorldProbe::CShapeTestResults& refResults)
{
	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;
	capsuleDesc.SetResultsStructure(&refResults);
	capsuleDesc.SetCapsule(vStart, vFinish, radius);
	capsuleDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	capsuleDesc.SetIncludeFlags(iShapeTestIncludeFlags);
	capsuleDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	capsuleDesc.SetIsDirected(true);

	IK_DEV_ONLY(ms_perfTimer.Reset();)
	bool bHit = WorldProbe::GetShapeTestManager()->SubmitTest(capsuleDesc);
#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[32];
		sprintf(debugText, "Shape test = %6.2fus\n", ms_perfTimer.GetUsTime());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	return	bHit;
}

//////////////////////////////////////////////////////////////////////////

bool CLegIkSolver::ProbeTest(const Vector3 &vStart,const Vector3 &vFinish, const u32 iShapeTestIncludeFlags)
{
	WorldProbe::CShapeTestProbeDesc probeDesc;
	probeDesc.SetStartAndEnd(vStart, vFinish);
	probeDesc.SetIncludeFlags(iShapeTestIncludeFlags);
	probeDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	bool bHit = WorldProbe::GetShapeTestManager()->SubmitTest(probeDesc);
	
#if __IK_DEV
	if (CaptureDebugRender() && ms_bRenderLineIntersections)
	{	
		if(bHit)
		{
			ms_debugHelper.Line(vStart,vFinish,Color_red,Color_red);
		}
		else
		{
			ms_debugHelper.Line(vStart,vFinish,Color_green,Color_green);
		}
	}
#endif //__IK_DEV
	
	return	bHit;
}

//////////////////////////////////////////////////////////////////////////

bool CLegIkSolver::BlockLegIK()
{
	bool bCanUseFade = true;
	bool bBlock = !m_pPed->GetIkManager().CanUseLegIK(m_rootAndPelvisGoal.m_bEnablePelvis, bCanUseFade);

	if (!bCanUseFade)
	{
		m_pPed->GetIkManager().SetFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);
	}

	if (bBlock)
	{
		return true;
	}

#if __IK_DEV
	if(ms_bForceLegIk && ms_nForceLegIkMode == LEG_IK_MODE_OFF)
	{
		// Blend out instantly
		m_pPed->GetIkManager().SetFlag(PEDIK_LEGS_AND_PELVIS_FADE_OFF);
		return true;
	}
#endif

	if (!m_bActive || m_pProxy->IsBlocked())
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

float CLegIkSolver::GetSpringStrengthForStairs()
{
	if (sm_Tunables.m_VelMagStairsSpringMax>sm_Tunables.m_VelMagStairsSpringMin)
	{
		CPed* pPed = m_pPed;
		if (pPed)
		{
			float velMag = pPed->GetVelocity().Mag();
			float t = (velMag - sm_Tunables.m_VelMagStairsSpringMin) / (sm_Tunables.m_VelMagStairsSpringMax-sm_Tunables.m_VelMagStairsSpringMin);
			t=Clamp(t, 0.0f, 1.0f);
			float strength = Lerp(t, sm_Tunables.m_StairsSpringMultiplierMin, sm_Tunables.m_StairsSpringMultiplierMax);
			return strength;
		}
	}

	return 1.0f;
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::CalcSmoothedNormal(const Vector3 &vDesiredNormal, const Vector3 &vCurrentNormal, const float fSmoothRate, Vector3 &vSmoothedNormal, const float fDeltaTime, const bool bForceDesired)
{
	// The following code lerps the old normal toward the new normal using quaternions
	// It will probably be cheaper to replace this with the following approach :
	// Use a dot product to decide if we are within the angle increment (if
	// so simple use the new normal) if not use the cross product of the
	// old and new vectors to make a rotation axis
	// Make a rotation matrix using this rotation axis and the angle increment
	// Rotate the old normal vector by this matrix

	vSmoothedNormal = vCurrentNormal;

	static dev_bool bOldStyle = false;
	if(bOldStyle)
	{
		Quaternion rotationFromOldToNew;
		rotationFromOldToNew.FromVectors(vCurrentNormal, vDesiredNormal);
		float fAngle = rotationFromOldToNew.GetAngle();

		float fLerpAngleInc = fDeltaTime * fSmoothRate;

		// Is the rotation angle more than one angle increment
		if (fabsf(fAngle) > fLerpAngleInc)
		{
			// Lerp the old normal toward the new normal by one angle increment
			rotationFromOldToNew.ScaleAngle(fLerpAngleInc/fabsf(fAngle));
			rotationFromOldToNew.Transform(vSmoothedNormal);
		}
		else
		{
			// The new normal is within one angle increment so snap to the new normal
			vSmoothedNormal = vDesiredNormal;
		}
	}
	else
	{
		ikAssert(vDesiredNormal.IsNonZero());
		if(vCurrentNormal.IsZero() || bForceDesired) /// JA - wtf?
		{
			vSmoothedNormal = vDesiredNormal;
		}
		else
		{
			float fAngleBetween = vCurrentNormal.Dot(vDesiredNormal);
			fAngleBetween = rage::Clamp(fAngleBetween,0.0f,1.0f);

			fAngleBetween = rage::Acosf(fAngleBetween);
			float fLerpAngleInc = fDeltaTime * (fSmoothRate * fAngleBetween);

			if(rage::Abs(fAngleBetween) < fLerpAngleInc)
			{
				vSmoothedNormal = vDesiredNormal;
			}
			else
			{
				// Create rotation about cross product
				Vector3 vRotationAxis = vCurrentNormal;
				vRotationAxis.Cross(vDesiredNormal);
				vRotationAxis.Normalize();
				Quaternion quat;
				quat.FromRotation(vRotationAxis,fLerpAngleInc);
				quat.Transform(vSmoothedNormal);
				vSmoothedNormal.NormalizeSafe(g_UnitUp);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::ClampNormal(Vector3& vNormal)
{
	const ScalarV vLimit(0.25f * PI);
	const Vec3V vAxis(V_UP_AXIS_WZERO);

	Vec3V vInNormal(VECTOR3_TO_VEC3V(vNormal));
	ikAssert(!IsZeroAll(vInNormal));

	ScalarV vAngle(Dot(vAxis, vInNormal));
	vAngle = Arccos(vAngle);

	if (IsGreaterThanAll(vAngle, vLimit))
	{
		Vec3V vRotationAxis(Cross(vAxis, vInNormal));
		vRotationAxis = NormalizeSafe(vRotationAxis, Vec3V(V_X_AXIS_WZERO));

		QuatV qRotation(QuatVFromAxisAngle(vRotationAxis, vLimit));
		vInNormal = Transform(qRotation, vAxis);

		vNormal = VEC3V_TO_VECTOR3(vInNormal);
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::CalcHeightOfBindPoseFootAboveCollision(const crSkeleton& skeleton)
{
	if (m_bBonesValid)
	{
		const crSkeletonData& skeletonData = skeleton.GetSkeletonData();
		const s16* aParentIndices = skeletonData.GetParentIndices();

		// Calculate bind pose position of foot (using left foot as reference)
		const crIKSolverLegs::Goal& solverGoal = GetGoal(LEFT_LEG);

		s16 boneIndex = solverGoal.m_BoneIdx[crIKSolverLegs::FOOT];
		Mat34V mtxFoot(skeletonData.GetDefaultTransform(boneIndex));

		while (aParentIndices[boneIndex] > 0)
		{
			Transform(mtxFoot, skeletonData.GetDefaultTransform(aParentIndices[boneIndex]), mtxFoot);
			boneIndex = aParentIndices[boneIndex];
		}

		// Calculate bind pose position of toe and foot point helper (using left foot as reference)
		Vec3V vToe(Transform(mtxFoot, skeletonData.GetDefaultTransform(solverGoal.m_BoneIdx[crIKSolverLegs::TOE]).GetCol3()));
		Vec3V vFootPH(Transform(mtxFoot, skeletonData.GetDefaultTransform(solverGoal.m_BoneIdx[crIKSolverLegs::PH]).GetCol3()));

		m_afHeightOfBindPoseFootAboveCollision[HEEL] = mtxFoot.GetCol3().GetZf() - vFootPH.GetZf();
		m_afHeightOfBindPoseFootAboveCollision[BALL] = vToe.GetZf() - vFootPH.GetZf();
	}
	else
	{
		m_afHeightOfBindPoseFootAboveCollision[HEEL] = 0.0f;
		m_afHeightOfBindPoseFootAboveCollision[BALL] = 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
float CLegIkSolver::GetMaxPelvisDeltaZ(RootAndPelvisSituation& sit)
{
	if (sit.bMoving)
	{
		if (sit.bOnStairs)
		{
			if (sit.bMovingUpwards)
			{
				return sm_Tunables.m_UpStairsPelvisMaxDeltaZMoving;
			}
			else
			{
				return sm_Tunables.m_DownStairsPelvisMaxDeltaZMoving;
			}
		}
		else
		{
			return (m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend == 0.0f) ? ms_fPelvisMaxPositiveDeltaZMoving : ms_fPelvisMaxPositiveDeltaZMovingNearEdge;
		}
	}
	else
	{
		return ms_fPelvisMaxPositiveDeltaZStanding;
	}
}

//////////////////////////////////////////////////////////////////////////
float CLegIkSolver::GetMaxAllowedPelvisDeltaZ()
{
	float max = sm_Tunables.m_UpStairsPelvisMaxDeltaZMoving;

	if (sm_Tunables.m_DownStairsPelvisMaxDeltaZMoving>max)
		max = sm_Tunables.m_DownStairsPelvisMaxDeltaZMoving;
	if (ms_fPelvisMaxPositiveDeltaZMoving>max)
		max = ms_fPelvisMaxPositiveDeltaZMoving;
	if (ms_fPelvisMaxPositiveDeltaZMovingNearEdge>max)
		max = ms_fPelvisMaxPositiveDeltaZMovingNearEdge;
	if (ms_fPelvisMaxPositiveDeltaZStanding>max)
		max = ms_fPelvisMaxPositiveDeltaZStanding;
	return max;
}

//////////////////////////////////////////////////////////////////////////
float CLegIkSolver::GetMinPelvisDeltaZ(RootAndPelvisSituation& sit)
{
	if (sit.bMoving)
	{
		if (sit.bOnStairs)
		{
			if (sit.bCoverAim)
			{
				return sm_Tunables.m_StairsPelvisMaxNegativeDeltaZCoverAim;
			}
			else if (sit.bMovingUpwards)
			{
				return sm_Tunables.m_UpStairsPelvisMaxNegativeDeltaZMoving;
			}
			else
			{
				return sm_Tunables.m_DownStairsPelvisMaxNegativeDeltaZMoving;
			}
		}
		else
		{
			return (m_rootAndPelvisGoal.m_fNearEdgePelvisInterpScaleBlend == 0.0f) ? ms_fPelvisMaxNegativeDeltaZMoving : ms_fPelvisMaxNegativeDeltaZMovingNearEdge;
		}
	}
	else
	{
		return ms_fPelvisMaxNegativeDeltaZStanding;
	}
}

//////////////////////////////////////////////////////////////////////////
float CLegIkSolver::GetMinAllowedPelvisDeltaZ(const RootAndPelvisSituation& rootAndPelvisSituation)
{
	float min = sm_Tunables.m_UpStairsPelvisMaxNegativeDeltaZMoving;

	if (sm_Tunables.m_DownStairsPelvisMaxNegativeDeltaZMoving<min)
		min = sm_Tunables.m_DownStairsPelvisMaxNegativeDeltaZMoving;
	if (sm_Tunables.m_StairsPelvisMaxNegativeDeltaZCoverAim<min)
		min = sm_Tunables.m_StairsPelvisMaxNegativeDeltaZCoverAim;
	if (ms_fPelvisMaxNegativeDeltaZMoving<min)
		min = ms_fPelvisMaxNegativeDeltaZMoving;
	if (ms_fPelvisMaxNegativeDeltaZMovingNearEdge<min)
		min = ms_fPelvisMaxNegativeDeltaZMovingNearEdge;
	if (ms_fPelvisMaxNegativeDeltaZStanding<min)
		min = ms_fPelvisMaxNegativeDeltaZStanding;
	if (rootAndPelvisSituation.bInVehicle && (ms_fPelvisMaxNegativeDeltaZVehicle<min))
		min = ms_fPelvisMaxNegativeDeltaZVehicle;
	if (rootAndPelvisSituation.bOnStairs && (ms_fPelvisMaxNegativeDeltaZStairs<min))
		min = ms_fPelvisMaxNegativeDeltaZStairs;
	if (rootAndPelvisSituation.bOnPhysical && (ms_fPelvisMaxNegativeDeltaZOnDynamicEntity < min))
		min = ms_fPelvisMaxNegativeDeltaZOnDynamicEntity;

	if (rootAndPelvisSituation.bOnSlope && !rootAndPelvisSituation.bOnStairs && !rootAndPelvisSituation.bInVehicle)
	{
		// Scale probe down further based on the steepness of the slope (eg. 1.5 at 45deg)
		min *= 1.0f + Clamp(rootAndPelvisSituation.fSlopePitch / (PI * 0.5f), 0.0f, 1.0f);
	}

	return min;
}

//////////////////////////////////////////////////////////////////////////

#if __IK_DEV

bool CLegIkSolver::CaptureDebugRender()
{
	if (ms_bRenderDebug)
	{
		CEntity* pSelectedEntity = (CEntity*) g_PickerManager.GetSelectedEntity();
		if (pSelectedEntity && pSelectedEntity->GetIsTypePed())
		{
			CPed* pSelectedPed = static_cast<CPed*>(pSelectedEntity);
			if (pSelectedPed == m_pPed)
			{
				return true;
			}

		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CLegIkSolver", false);
	pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeLeg], NullCB, "");
	pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");

	pBank->PushGroup("Debug Rendering", false);
		pBank->AddToggle("Render skin",	&ms_bRenderSkin, ToggleRenderPeds, NULL);
		pBank->AddSlider("ms_fXOffset", &CLegIkSolver::ms_fXOffset, -1.0f, 1.0f, 0.01f);
		pBank->AddToggle("ms_renderSweptProbes",		&CLegIkSolver::ms_bRenderSweptProbes);
		pBank->AddToggle("ms_bRenderProbeIntersections", &CLegIkSolver::ms_bRenderProbeIntersections);
		pBank->AddToggle("ms_bRenderLineIntersections",	 &CLegIkSolver::ms_bRenderLineIntersections);
		pBank->AddToggle("ms_bRenderSituations",		 &CLegIkSolver::ms_bRenderSituations);	
		pBank->AddToggle("ms_bRenderSupporting",		 &CLegIkSolver::ms_bRenderSupporting);	
		pBank->AddToggle("ms_renderInputNormal",		&CLegIkSolver::ms_bRenderInputNormal);
		pBank->AddToggle("ms_renderSmoothedNormal",		&CLegIkSolver::ms_bRenderSmoothedNormal);
		pBank->AddToggle("ms_renderBeforeIK",			&CLegIkSolver::ms_bRenderBeforeIK);
		pBank->AddToggle("ms_renderAfterPelvis",		&CLegIkSolver::ms_bRenderAfterPelvis);	
		pBank->AddToggle("ms_renderAfterIK",			&CLegIkSolver::ms_bRenderAfterIK);
	pBank->PopGroup(); // Debug Rendering

#if ENABLE_LEGIK_SOLVER_WIDGETS
	pBank->PushGroup("Settings", false);
		pBank->AddToggle("ms_bPelvisFixup", &CLegIkSolver::ms_bPelvisFixup);
		pBank->AddToggle("ms_bLegFixup", &CLegIkSolver::ms_bLegFixup);
		pBank->AddToggle("ms_bFootOrientationFixup", &CLegIkSolver::ms_bFootOrientationFixup);
		pBank->AddToggle("ms_bFootHeightFixup", &CLegIkSolver::ms_bFootHeightFixup);

		pBank->AddSlider("ms_fAnimFootZBlend", &CLegIkSolver::ms_fAnimFootZBlend, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fAnimPelvisZBlend", &CLegIkSolver::ms_fAnimPelvisZBlend, 0.0f, 1.0f, 0.001f);

		pBank->AddSlider("ms_afSolverBlendRate (Full Mode)", &CLegIkSolver::ms_afSolverBlendRate[0], 0,100.0f,0.1f);
		pBank->AddSlider("ms_afSolverBlendRate (Partial Mode)", &CLegIkSolver::ms_afSolverBlendRate[1], 0,100.0f,0.1f);
		pBank->AddSlider("ms_fPelvisDeltaZRate", &CLegIkSolver::ms_fPelvisDeltaZRate, 0,100.0f,0.1f);

		pBank->AddSlider("ms_fPelvisDeltaZMax", &CLegIkSolver::ms_fPelvisDeltaZRate, 0,100.0f,0.1f);
		pBank->AddSlider("ms_fPelvisDeltaZAccel", &CLegIkSolver::ms_fPelvisDeltaZRate, 0,100.0f,0.1f);
		
		pBank->AddSlider("ms_fFootDeltaZRate", &CLegIkSolver::ms_fFootDeltaZRate, 0,100.0f,0.1f);
		pBank->AddSlider("ms_fFootDeltaZRateIntersecting", &CLegIkSolver::ms_fFootDeltaZRateIntersecting, 0,100.0f,0.1f);
		pBank->AddSlider("ms_fGroundNormalRate", &CLegIkSolver::ms_fGroundNormalRate,0,10.0f,0.1f);
		pBank->AddSlider("ms_fFootOrientationBlendRate", &CLegIkSolver::ms_fFootOrientationBlendRate,0,100.0f,0.1f);

		pBank->AddSlider("ms_fPelvisMaxNegativeDeltaZStanding", &CLegIkSolver::ms_fPelvisMaxNegativeDeltaZStanding, -1.0f, 0.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxPositiveDeltaZStanding", &CLegIkSolver::ms_fPelvisMaxPositiveDeltaZStanding, 0.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxNegativeDeltaZMoving", &CLegIkSolver::ms_fPelvisMaxNegativeDeltaZMoving, -1.0f, 0.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxPositiveDeltaZMoving", &CLegIkSolver::ms_fPelvisMaxPositiveDeltaZMoving, 0.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxNegativeDeltaZMovingNearEdge", &CLegIkSolver::ms_fPelvisMaxNegativeDeltaZMovingNearEdge, -1.0f, 0.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxPositiveDeltaZMovingNearEdge", &CLegIkSolver::ms_fPelvisMaxPositiveDeltaZMovingNearEdge, 0.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_fPelvisMaxNegativeDeltaZVehicle", &CLegIkSolver::ms_fPelvisMaxNegativeDeltaZVehicle, -1.0f, 0.0f, 0.01f);

		pBank->AddSlider("ms_fNearEdgePelvisInterpScale", &CLegIkSolver::ms_fNearEdgePelvisInterpScale, 0.0f, 10.0f, 0.01f);
		pBank->AddSlider("ms_fNearEdgePelvisInterpScaleRate", &CLegIkSolver::ms_fNearEdgePelvisInterpScaleRate, 0.0f, 10.0f, 0.01f);
		pBank->AddSlider("ms_fOnSlopeLocoBlendRate", &CLegIkSolver::ms_fOnSlopeLocoBlendRate, 0.0f, 10.0f, 0.01f);

		pBank->AddSlider("ms_afFootMaxPositiveDeltaZ", &CLegIkSolver::ms_afFootMaxPositiveDeltaZ[0], 0.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_afFootMaxPositiveDeltaZ (Slope, OnDynamic)", &CLegIkSolver::ms_afFootMaxPositiveDeltaZ[1], 0.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_afFootMaxPositiveDeltaZ (Melee)", &CLegIkSolver::ms_afFootMaxPositiveDeltaZ[2], 0.0f, 1.0f, 0.01f);

		pBank->AddToggle("ms_bClampFootOrientation", &CLegIkSolver::ms_bClampFootOrientation);
		pBank->AddSlider("ms_fFootOrientationPitchClampMin", &CLegIkSolver::ms_fFootOrientationPitchClampMin, -PI, PI, 0.001f);
		pBank->AddSlider("ms_fFootOrientationPitchClampMax", &CLegIkSolver::ms_fFootOrientationPitchClampMax, -PI, PI, 0.001f);

		pBank->AddToggle("ms_bClampPostiveAnimZ", &CLegIkSolver::ms_bClampPostiveAnimZ);	

		pBank->AddSlider("ms_fMovingTolerance", &CLegIkSolver::ms_fMovingTolerance, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fOnSlopeTolerance", &CLegIkSolver::ms_fOnSlopeTolerance, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fStairHeightTolerance", &CLegIkSolver::ms_fStairHeightTolerance, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fProbePositionDeltaTolerance", &CLegIkSolver::ms_fProbePositionDeltaTolerance, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fRollbackEpsilon", &CLegIkSolver::ms_fRollbackEpsilon, 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_fNormalVerticalThreshold", &CLegIkSolver::ms_fNormalVerticalThreshold, 0.0f, 1.0f, 0.001f);

		pBank->AddToggle("ms_bUseMultipleShapeTests", &CLegIkSolver::ms_bUseMultipleShapeTests);
		pBank->AddToggle("ms_bUseMultipleAsyncShapeTests", &CLegIkSolver::ms_bUseMultipleAsyncShapeTests);
		pBank->AddSlider("ms_fFootAngleSlope", &CLegIkSolver::ms_fFootAngleSlope, -PI, PI, 0.1f);
		pBank->AddSlider("ms_fFootAngleClamp In", &CLegIkSolver::ms_fFootAngleClamp[0], 0.0f, PI, 0.1f);
		pBank->AddSlider("ms_fFootAngleClamp Out", &CLegIkSolver::ms_fFootAngleClamp[1], 0.0f, PI, 0.1f);
		pBank->AddSlider("ms_fVehicleToeGndPosTolerance", &CLegIkSolver::ms_fVehicleToeGndPosTolerance, -1.0f, 1.0f, 0.01f);
		pBank->AddSlider("ms_afFootRadius", &CLegIkSolver::ms_afFootRadius[0], 0.0f, 1.0f, 0.001f);
		pBank->AddSlider("ms_afFootRadius (Stairs, OnDynamic)", &CLegIkSolver::ms_afFootRadius[1], 0.0f, 1.0f, 0.001f);

		static const char* sSupportingFootMode[]=
		{
			"FURTHEST_BACKWARD_FOOT",
			"FURTHEST_FOREWARD_FOOT",
			"HIGHEST_FOOT",
			"LOWEST_FOOT",
			"HIGHEST_FOOT_INTERSECTION",
			"LOWEST_FOOT_INTERSECTION",
			"MOST_RECENT_LOCKED_Z",
		};

		pBank->AddCombo("ms_nNormalMovementSupportingLegMode", &CLegIkSolver::ms_nNormalMovementSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nNormalStandingSupportingLegMode", &CLegIkSolver::ms_nNormalStandingSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nUpSlopeSupportingLegMode", &CLegIkSolver::ms_nUpSlopeSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nDownSlopeSupportingLegMode", &CLegIkSolver::ms_nDownSlopeSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nUpStairsSupportingLegMode", &CLegIkSolver::ms_nUpStairsSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nDownStairsSupportingLegMode", &CLegIkSolver::ms_nDownStairsSupportingLegMode,7,sSupportingFootMode);
		pBank->AddCombo("ms_nLowCoverSupportingLegMode", &CLegIkSolver::ms_nLowCoverSupportingLegMode,7,sSupportingFootMode);

		pBank->AddToggle("ms_bForceLegIk",&CLegIkSolver::ms_bForceLegIk);
		static const char* sLegIkModes[]=
		{
			"Leg IK Off",
			"Leg IK Partial",
			"Leg IK Full",
			"Leg IK Full Melee"
		};
		pBank->AddCombo("ms_nForceIkMode", &CLegIkSolver::ms_nForceLegIkMode,4,sLegIkModes);
	pBank->PopGroup(); // Settings
#endif // ENABLE_LEGIK_SOLVER_WIDGETS

	pBank->PopGroup();
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

//////////////////////////////////////////////////////////////////////////

void CLegIkSolver::ToggleRenderPeds()
{
	// .. but this only hides the ped in non-wireframe phases
	CPedDrawHandler::SetEnableRendering(ms_bRenderSkin);
}

//////////////////////////////////////////////////////////////////////////

#endif //__IK_DEV



