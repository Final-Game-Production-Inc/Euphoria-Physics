// 
// ik/solvers/QuadLegSolver.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved. 
// 

#include "QuadLegSolver.h"

#if IK_QUAD_LEG_SOLVER_ENABLE

#include "diag/art_channel.h"
#include "fwdebug/picker.h"

#include "animation/anim_channel.h"
#include "animation/AnimBones.h"
#include "animation/debug/AnimViewer.h"
#include "ik/solvers/QuadLegSolverProxy.h"
#include "Peds/Ped.h"

#define ENABLE_QUADLEGIK_SOLVER_WIDGETS	0

#if IK_QUAD_LEG_SOLVER_USE_POOL
FW_INSTANTIATE_CLASS_POOL(CQuadLegIkSolver, QUAD_LEG_SOLVER_POOL_MAX, atHashString("CQuadLegIkSolver",0xe72d5263));
#else
s8 CQuadLegIkSolver::ms_NoOfFreeSpaces = QUAD_LEG_SOLVER_POOL_MAX;
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

CQuadLegIkSolver::Tunables CQuadLegIkSolver::ms_Tunables;
IMPLEMENT_IK_SOLVER_TUNABLES(CQuadLegIkSolver);

#if __IK_DEV
CDebugRenderer CQuadLegIkSolver::ms_DebugHelper;
CQuadLegIkSolver::DebugParams CQuadLegIkSolver::ms_DebugParams =
{
	0.0f,		// m_fGroundHeightDelta
	0,			// m_iNoOfTexts
	false,		// m_bRenderDebug
	true,		// m_bRenderProbes
	false,		// m_bRenderSweptProbes
	0			// m_uForceSolverMode
};
#endif // __IK_DEV

CQuadLegIkSolver::ShapeTestData::ShapeTestData(bool bAllocate)
: m_bOwnResults(bAllocate)
{
	if (bAllocate)
	{
		Allocate();
	}
	else
	{
		for (int legPart = 0; legPart < CQuadLegIkSolver::NUM_LEGS; ++legPart)
		{
			m_apResults[legPart] = NULL;
		}
	}
}

CQuadLegIkSolver::ShapeTestData::~ShapeTestData()
{
	if (m_bOwnResults)
	{
		Reset();
		Free();
	}
}

void CQuadLegIkSolver::ShapeTestData::Allocate()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CQuadLegIkSolver::NUM_LEGS; ++legPart)
	{
		m_apResults[legPart] = rage_new WorldProbe::CShapeTestResults(NUM_INTERSECTIONS_FOR_PROBES);
	}
}

void CQuadLegIkSolver::ShapeTestData::Free()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CQuadLegIkSolver::NUM_LEGS; ++legPart)
	{
		if (m_apResults[legPart])
		{
			delete m_apResults[legPart];
			m_apResults[legPart] = NULL;
		}
	}
}

void CQuadLegIkSolver::ShapeTestData::Reset()
{
	ikAssert(m_bOwnResults);

	for (int legPart = 0; legPart < CQuadLegIkSolver::NUM_LEGS; ++legPart)
	{
		if (m_apResults[legPart])
		{
			m_apResults[legPart]->Reset();
		}
	}
}

CQuadLegIkSolver::CQuadLegIkSolver(CPed* pPed, CQuadLegIkSolverProxy* pProxy)
: crIKSolverQuadruped()
, m_pProxy(pProxy)
, m_pShapeTestData(NULL)
, m_pPed(pPed)
, m_uRootBoneIdx((u16)-1)
, m_uPelvisBoneIdx((u16)-1)
, m_uSpineBoneIdx((u16)-1)
, m_bActive(false)
, m_bSolve(false)
, m_bBonesValid(false)
, m_bInstantSetup(false)
{
	static const eAnimBoneTag legBoneTags[NUM_LEGS][NUM_LEG_PARTS] = 
	{
		{ BONETAG_L_UPPERARM, BONETAG_L_FOREARM, BONETAG_L_HAND, BONETAG_L_FINGER0, BONETAG_L_FINGER01, BONETAG_L_PH_HAND },
		{ BONETAG_R_UPPERARM, BONETAG_R_FOREARM, BONETAG_R_HAND, BONETAG_R_FINGER0, BONETAG_R_FINGER01, BONETAG_R_PH_HAND },
		{ BONETAG_L_THIGH, BONETAG_L_CALF, BONETAG_L_FOOT, BONETAG_L_TOE, BONETAG_L_TOE1, BONETAG_L_PH_FOOT },
		{ BONETAG_R_THIGH, BONETAG_R_CALF, BONETAG_R_FOOT, BONETAG_R_TOE, BONETAG_R_TOE1, BONETAG_R_PH_FOOT }
	};

	m_bBonesValid = true;

	crSkeleton& skeleton = *m_pPed->GetSkeleton();

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		crIKSolverQuadruped::Goal& solverGoal = GetGoal(leg);

		solverGoal.m_BoneIdx[crIKSolverQuadruped::THIGH]= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][THIGH]);
		solverGoal.m_BoneIdx[crIKSolverQuadruped::CALF]	= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][CALF]);
		solverGoal.m_BoneIdx[crIKSolverQuadruped::FOOT]	= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][FOOT]);
		solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE]	= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][TOE]);
		solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE1] = (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][TOE1]);
		solverGoal.m_BoneIdx[crIKSolverQuadruped::PH]	= (u16)pPed->GetBoneIndexFromBoneTag(legBoneTags[leg][PH]);

		if (solverGoal.m_BoneIdx[crIKSolverQuadruped::FOOT] != (u16)-1)
		{
			solverGoal.m_TerminatingIdx = (u16)skeleton.GetTerminatingPartialBone(solverGoal.m_BoneIdx[crIKSolverQuadruped::FOOT]);
		}

		m_bBonesValid &= (solverGoal.m_BoneIdx[crIKSolverQuadruped::THIGH]	!= (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverQuadruped::CALF]	!= (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverQuadruped::FOOT]	!= (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE]	!= (u16)-1) &&
						 (solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE1]	!= (u16)-1) &&
						 (solverGoal.m_TerminatingIdx != (u16)-1);

		artAssertf(m_bBonesValid, "Model %s missing bones for quadruped leg IK. Missing BONETAG_L/R_THIGH, BONETAG_L/R_CALF, BONETAG_L/R_FOOT, BONETAG_L/R_TOE, BONETAG_L/R_TOE1", pPed ? pPed->GetModelName() : "Unknown");
	}

	m_uRootBoneIdx = (u16)pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	m_uPelvisBoneIdx = (u16)pPed->GetBoneIndexFromBoneTag(BONETAG_PELVIS);
	m_uSpineBoneIdx = (u16)pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE3);

	m_bBonesValid &= (m_uRootBoneIdx != (u16)-1) && (m_uPelvisBoneIdx != (u16)-1) && (m_uSpineBoneIdx != (u16)-1);
	artAssertf(m_bBonesValid, "Model %s missing BONETAG_ROOT, BONETAG_PELVIS, BONETAG_SPINE3 bones for quadruped leg IK", pPed ? pPed->GetModelName() : "Unknown");

	m_pShapeTestData = rage_new ShapeTestData();

	CalcDefaults(skeleton);
	SetPrority(6);
	Reset();
}

CQuadLegIkSolver::~CQuadLegIkSolver()
{
	DeleteShapeTestData();
}

void CQuadLegIkSolver::Iterate(crSkeleton& skeleton)
{
	IK_DEV_ONLY(char debugText[128];)

	if (!m_bSolve)
	{
		return;
	}

	if (SolveRoot(skeleton))
	{
		Mat34V mtxBone;

		for (int leg = 0; leg < NUM_LEGS; ++leg)
		{
			skeleton.GetGlobalMtx(GetGoal(leg).m_BoneIdx[crIKSolverQuadruped::TOE1], mtxBone);
			TransformVFromMat34V(m_aLegSituation[leg].m_Target, mtxBone);
		}
	}

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
#if __IK_DEV
		if (CaptureDebugRender())
		{
			formatf(debugText, sizeof(debugText), "Leg %d      :", leg);
			ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
		}
#endif // __IK_DEV

		SetupLeg(skeleton, GetGoal(leg), m_aLegSituation[leg]);
	}
}

void CQuadLegIkSolver::Reset()
{
	TransformV Identity(V_IDENTITY);
	Vec3V vUpAxis(V_UP_AXIS_WZERO);
	Vec3V vZero(V_ZERO);

	m_bActive = false;
	m_bSolve = false;

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];

		legSituation.m_Target = Identity;

		legSituation.m_vGroundNormal = vUpAxis;
		legSituation.m_fGroundHeight = 0.0f;
		legSituation.m_fDeltaHeight = 0.0f;
		legSituation.m_fBlend = 0.0f;

		legSituation.m_uFrontBack = (leg >= 2);
		legSituation.m_bIntersection = false;
	}

	PartialReset();

	m_RootSituation.m_avGroundPosition[0] = vZero;
	m_RootSituation.m_avGroundPosition[1] = vZero;
	m_RootSituation.m_avGroundNormal[0] = vUpAxis;
	m_RootSituation.m_avGroundNormal[1] = vUpAxis;
	m_RootSituation.m_fDeltaHeight = 0.0f;
	m_RootSituation.m_fBlend = 0.0f;
	m_RootSituation.m_bIntersection = false;
	m_RootSituation.m_bMoving = false;
}

void CQuadLegIkSolver::PartialReset()
{
	crSkeleton& skeleton = *m_pPed->GetSkeleton();
	Mat34V mtxBone;
	Vec3V vZero(V_ZERO);

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		crIKSolverQuadruped::Goal& solverGoal = GetGoal(leg);
		LegSituation& legSituation = m_aLegSituation[leg];

		skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE1], mtxBone);

		legSituation.m_vProbePosition = mtxBone.GetCol3();
		legSituation.m_vPrevPosition = mtxBone.GetCol3();
		legSituation.m_vPrevVelocity = vZero;
	}
}

void CQuadLegIkSolver::PreIkUpdate(float deltaTime)
{
	IK_DEV_ONLY(char debugText[128];)

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();
	CIkManager& ikManager = m_pPed->GetIkManager();
	const bool bLowLOD = (GetMode() == LEG_IK_MODE_PARTIAL);
	m_bSolve = true;

	// Reset low level goals
	for (int leg = 0; leg < NUM_LEGS; leg++)
	{
		GetGoal(leg).m_Enabled = false;
	}

	if (pSkeleton == NULL)
	{
		m_bSolve = false;
		return;
	}

#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeQuadLeg] || CBaseIkManager::ms_DisableAllSolvers)
	{
		m_bSolve = false;
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		ms_DebugHelper.Reset();
		ms_DebugParams.m_iNoOfTexts = 0;
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, "QuadLegIkSolver::PreIkUpdate");

		formatf(debugText, sizeof(debugText), "Solver LOD : %s", bLowLOD ? "Low" : "High");
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Root Limits: %6.3f, %6.3f", m_RootSituation.m_afDeltaLimits[0], m_RootSituation.m_afDeltaLimits[1]);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "PH Height  : %6.3f, %6.3f", m_afDefaultHeightAboveCollision[0], m_afDefaultHeightAboveCollision[1]);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Leg Lengths: %6.3f %6.3f", m_afLegLength[0], m_afLegLength[1]);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "ST Offset F: %6.3f, %6.3f", GetShapeTestMinOffset(0), GetShapeTestMaxOffset(0));
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "ST Offset B: %6.3f, %6.3f", GetShapeTestMinOffset(1), GetShapeTestMaxOffset(1));
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		DebugRenderLowerBody(*pSkeleton, Color_blue1, Color_blue4);
	}
#endif // __IK_DEV

	if (!IsValid())
	{
		m_pProxy->SetComplete(true);
		m_bSolve = false;
		return;
	}

	const bool bImmediateReset = m_pPed->GetUsingRagdoll() || m_pPed->GetDisableLegSolver() || m_pPed->IsDead();
	if (bImmediateReset || fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior)
	{
		if (bImmediateReset)
		{
			Reset();
			m_pProxy->SetComplete(true);
		}

		m_bSolve = false;
		return;
	}

	if (IsBlocked())
	{
		m_bActive = false;
	}

	// Instant setup
	m_bInstantSetup = ikManager.IsFlagSet(PEDIK_LEGS_INSTANT_SETUP);

	if (m_bInstantSetup)
	{
		// Reset probe positions and velocities
		PartialReset();
		ikManager.ClearFlag(PEDIK_LEGS_INSTANT_SETUP);
	}

	// Setup
	Mat34V mtxBone;
	const Vec3V vZero(V_ZERO);
	const ScalarV vScalarZero(V_ZERO);
	const ScalarV vGroundPositionResetZ(PED_GROUNDPOS_RESET_Z);

	const CPhysical* pPhysical = m_pPed->GetGroundPhysical();

	// Velocity
	Vec3V vVelocity(m_pPed->GetVelocity());
	if (pPhysical)
	{
		vVelocity = Subtract(vVelocity, VECTOR3_TO_VEC3V(pPhysical->GetVelocity()));
	}
	m_RootSituation.m_bMoving = IsGreaterThanAll(Mag(vVelocity), vScalarZero) != 0;

	// Front ground position and normal
	Vec3V vGroundPosition(VECTOR3_TO_VEC3V(m_pPed->GetGroundPos()));
	Vec3V vGroundNormal(V_UP_AXIS_WZERO);

	if (IsEqualAll(vGroundPosition.GetZ(), vGroundPositionResetZ))
	{
		// No collision height/normal so assume flat ground
		vGroundPosition = m_pPed->GetTransform().GetPosition();
		vGroundPosition.SetZ(Subtract(vGroundPosition.GetZ(), ScalarV(m_pPed->GetCapsuleInfo()->GetGroundToRootOffset())));

		m_RootSituation.m_bIntersection = false;

		if (m_bInstantSetup)
		{
			Vector3 vPedPosition(VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()));
			Vector3 vPedNormal(ZAXIS);
			bool bResult = false;

			// If doing an instant setup, try to find a valid ground normal directly
			WorldProbe::FindGroundZFor3DCoord(TOP_SURFACE, vPedPosition, &bResult, &vPedNormal);

			if (bResult)
			{
				m_RootSituation.m_bIntersection = true;
				vGroundNormal = VECTOR3_TO_VEC3V(vPedNormal);
				ClampNormal(vGroundNormal);
			}
		}
	}
	else
	{
		vGroundNormal = VECTOR3_TO_VEC3V(m_pPed->GetGroundNormal());

		if (bLowLOD)
		{
			// Use max ground normal if possible (non-zero)
			const Vec3V& vMaxGroundNormal = m_pPed->GetMaxGroundNormal();

			if (IsZeroAll(vMaxGroundNormal) == 0)
			{
				vGroundNormal = vMaxGroundNormal;
			}
		}

		ClampNormal(vGroundNormal);

		m_RootSituation.m_bIntersection = true;
	}

	m_RootSituation.m_avGroundNormal[0] = SmoothNormal(vGroundNormal, m_RootSituation.m_avGroundNormal[0], ms_Tunables.m_RootGroundNormalInterpolationRate, deltaTime, m_bInstantSetup != 0);
	m_RootSituation.m_avGroundPosition[0] = vGroundPosition;

	// Rear ground position and normal
	vGroundPosition = m_pPed->GetRearGroundPos();

	if (IsEqualAll(vGroundPosition.GetZ(), vGroundPositionResetZ))
	{
		// No collision height/normal so approximate from front using distance from spine3 to pelvis
		const Mat34V& mtxPelvis = pSkeleton->GetObjectMtx(m_uPelvisBoneIdx);
		const Mat34V& mtxSpine = pSkeleton->GetObjectMtx(m_uSpineBoneIdx);

		ScalarV vMagSpineToPelvis(Mag(Subtract(mtxPelvis.GetCol3(), mtxSpine.GetCol3())));
		vGroundPosition = AddScaled(m_RootSituation.m_avGroundPosition[0], Negate(m_pPed->GetTransform().GetB()), vMagSpineToPelvis);

		// Share front normal
		vGroundNormal = m_RootSituation.m_avGroundNormal[0];
	}
	else
	{
		vGroundNormal = m_pPed->GetRearGroundNormal();

		// TODO: Support from ped for a max rear ground normal similar to CPed::GetMaxGroundNormal()?
		//if (bLowLOD)
		//{
		//	// Use max ground normal if possible (non-zero)
		//	const Vec3V& vMaxGroundNormal = m_pPed->GetMaxRearGroundNormal();

		//	if (IsZeroAll(vMaxGroundNormal) == 0)
		//	{
		//		vGroundNormal = vMaxGroundNormal;
		//	}
		//}

		ClampNormal(vGroundNormal);
	}

	m_RootSituation.m_avGroundNormal[1] = SmoothNormal(vGroundNormal, m_RootSituation.m_avGroundNormal[1], ms_Tunables.m_RootGroundNormalInterpolationRate, deltaTime, m_bInstantSetup != 0);
	m_RootSituation.m_avGroundPosition[1] = vGroundPosition;

#if __IK_DEV
	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "Intersected: %d", m_RootSituation.m_bIntersection ? 1 : 0);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Moving     : %d", m_RootSituation.m_bMoving ? 1 : 0);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	// Initialize leg situations
	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		crIKSolverQuadruped::Goal& solverGoal = GetGoal(leg);
		LegSituation& legSituation = m_aLegSituation[leg];

		pSkeleton->GetGlobalMtx(solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE1], mtxBone);
		TransformVFromMat34V(legSituation.m_Target, mtxBone);

		legSituation.m_fGroundHeight = mtxBone.GetCol3().GetZf() - m_afDefaultHeightAboveCollision[leg >= 2];
		legSituation.m_bIntersection = false;
	}

	// Process intersections
	if (!bLowLOD)
	{
		// Process shape tests
		if (CanUseAsyncShapeTests())
		{
			if (deltaTime > 0.0f)
			{
				PreProcessResults();
			}
			ProcessResults();
			if (deltaTime > 0.0f)
			{
				SubmitAsyncShapeTests();
			}
		}
		else
		{
			SubmitShapeTests();
			ProcessResults();
		}
	}
	else
	{
		if (m_pShapeTestData->m_apResults[0]->GetResultsStatus() != WorldProbe::TEST_NOT_SUBMITTED)
		{
			// Reset pending shape test data if switching from high to low LOD mode
			m_pShapeTestData->Reset();
		}

		for (int leg = 0; leg < NUM_LEGS; ++leg)
		{
			LegSituation& legSituation = m_aLegSituation[leg];

			const Vec3V& vRootGroundPosition = m_RootSituation.m_avGroundPosition[legSituation.m_uFrontBack];
			const Vec3V& vRootGroundNormal = m_RootSituation.m_avGroundNormal[legSituation.m_uFrontBack];
			const Vec3V vToePosition(legSituation.m_Target.GetPosition());

			legSituation.m_vGroundNormal = SmoothNormal(vRootGroundNormal, legSituation.m_vGroundNormal, ms_Tunables.m_EffectorGroundNormalInterpolationRate, deltaTime, m_bInstantSetup != 0);

			// For two points on a plane a, b and the plane normal N we know N.a = N.b Therefore take a as the ground pos from the ped's capsule and b as the foot position
			// then solve for b.z => N.z * b.z = N.Dot(a) - N.x * b.x - N.y * b.y
			const Vec3V vNormalPositionScale(Scale(vRootGroundNormal, vToePosition));

			ScalarV vGroundHeight(Dot(vRootGroundNormal, vRootGroundPosition));
			vGroundHeight = Subtract(vGroundHeight, vNormalPositionScale.GetX());
			vGroundHeight = Subtract(vGroundHeight, vNormalPositionScale.GetY());
			vGroundHeight = InvScaleSafe(vGroundHeight, vRootGroundNormal.GetZ(), vRootGroundPosition.GetZ());

			legSituation.m_vPrevPosition = vToePosition;
			legSituation.m_vPrevVelocity = vZero;

			legSituation.m_fGroundHeight = vGroundHeight.Getf();
			legSituation.m_bIntersection = m_RootSituation.m_bIntersection;
		}
	}

	// Update blends
	float afLegBlendRate[2];
	float afRootBlendRate[2];
	bool bBlendedOut = true;

	afLegBlendRate[0] = (ms_Tunables.m_LegBlendInRate != 0.0f) && !m_bInstantSetup ? 1.0f / ms_Tunables.m_LegBlendInRate : INSTANT_BLEND_IN_DELTA;
	afLegBlendRate[1] = (ms_Tunables.m_LegBlendOutRate != 0.0f) && !m_bInstantSetup ? -1.0f / ms_Tunables.m_LegBlendOutRate : INSTANT_BLEND_OUT_DELTA;
	afRootBlendRate[0] = (ms_Tunables.m_RootBlendInRate != 0.0f) && !m_bInstantSetup? 1.0f / ms_Tunables.m_RootBlendInRate : INSTANT_BLEND_IN_DELTA;
	afRootBlendRate[1] = (ms_Tunables.m_RootBlendOutRate != 0.0f) && !m_bInstantSetup ? -1.0f / ms_Tunables.m_RootBlendOutRate : INSTANT_BLEND_OUT_DELTA;

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];
		const bool bActive = m_bActive && legSituation.m_bIntersection;

		legSituation.m_fBlend += afLegBlendRate[bActive ? 0 : 1] * deltaTime;
		legSituation.m_fBlend = Clamp(legSituation.m_fBlend, 0.0f, 1.0f);

		bBlendedOut &= (legSituation.m_fBlend == 0.0f);
	}

	m_RootSituation.m_fBlend += afRootBlendRate[m_bActive ? 0 : 1] * deltaTime;
	m_RootSituation.m_fBlend = Clamp(m_RootSituation.m_fBlend, 0.0f, 1.0f);
	bBlendedOut &= (m_RootSituation.m_fBlend == 0.0f);

	if (bBlendedOut)
	{
		Reset();
		m_pProxy->SetComplete(true);
		m_bSolve = false;
		return;
	}

#if __IK_DEV
	if (CaptureDebugRender() && ms_DebugParams.m_bRenderProbes)
	{
		const float fRadius = GetShapeTestRadius();
		static float fT = 0.0f;
		static float fRate = 0.1f;

		const rage::Color32 probeColour = Color_yellow;
		const rage::Color32 intersectionColour = Color_purple;

		Vec3V vStart;
		Vec3V vEnd;

		const ScalarV vNormalScale(0.40f);

		if (ms_DebugParams.m_bRenderSweptProbes)
		{
			fT += deltaTime * fRate;
			fT = fmodf(fT, 1.0f);
		}

		for (int leg = 0; leg < NUM_LEGS; ++leg)
		{
			LegSituation& legSituation = m_aLegSituation[leg];

			const Vec3V vShapeTestMaxOffset(0.0f, 0.0f, GetShapeTestMaxOffset(legSituation.m_uFrontBack));
			const Vec3V vShapeTestMinOffset(0.0f, 0.0f, GetShapeTestMinOffset(legSituation.m_uFrontBack));

			vStart = Add(legSituation.m_vProbePosition, vShapeTestMaxOffset);
			vEnd = Add(legSituation.m_vProbePosition, vShapeTestMinOffset);
			ms_DebugHelper.Line(RC_VECTOR3(vStart), RC_VECTOR3(vEnd), probeColour);

			if (ms_DebugParams.m_bRenderSweptProbes)
			{
				ms_DebugHelper.Sphere(RC_VECTOR3(vStart), fRadius, probeColour, false);
				ms_DebugHelper.Sphere(RC_VECTOR3(vEnd), fRadius, probeColour, false);
				vStart = Lerp(ScalarV(fT), vStart, vEnd);
				ms_DebugHelper.Sphere(RC_VECTOR3(vStart), fRadius, probeColour, false);
			}

			if (legSituation.m_bIntersection)
			{
				vStart = legSituation.m_Target.GetPosition();
				vStart.SetZf(legSituation.m_fGroundHeight);
				vEnd = AddScaled(vStart, legSituation.m_vGroundNormal, ScalarV(0.40f));
				ms_DebugHelper.Line(RC_VECTOR3(vStart), RC_VECTOR3(vEnd), intersectionColour);
				ms_DebugHelper.Sphere(RC_VECTOR3(vStart), 0.01f, intersectionColour, false);
			}
		}

		for (int index = 0; index < 2; ++index)
		{
			vStart = m_RootSituation.m_avGroundPosition[index];
			vEnd = AddScaled(vStart, m_RootSituation.m_avGroundNormal[index], vNormalScale);
			ms_DebugHelper.Sphere(RC_VECTOR3(vStart), 0.02f, intersectionColour, false);
			ms_DebugHelper.Line(RC_VECTOR3(vStart), RC_VECTOR3(vEnd), intersectionColour);
		}
	}
#endif // __IK_DEV
}

void CQuadLegIkSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		// Reset
		m_bActive = false;
		m_bInstantSetup = false;

#if __IK_DEV
		if (CaptureDebugRender())
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

void CQuadLegIkSolver::Request(const CIkRequestLeg& UNUSED_PARAM(request))
{
	m_bActive = true;
}

bool CQuadLegIkSolver::IsBlocked() const
{
	if (m_pProxy->IsBlocked())
	{
		return true;
	}

	CIkManager& ikManager = m_pPed->GetIkManager();

	bool bCanUseRoot = true;
	bool bCanUseFade = true;

	if (!ikManager.CanUseLegIK(bCanUseRoot, bCanUseFade))
	{
		return true;
	}

	fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();

	if (pAnimDirector)
	{
		const CClipEventTags::CLegsIKEventTag* pProperty = static_cast<const CClipEventTags::CLegsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LegsIk));

		if (pProperty)
		{
			if (!pProperty->GetAllowed())
			{
				return true;
			}
		}
		else if (ikManager.IsFlagSet(PEDIK_LEGS_USE_ANIM_ALLOW_TAGS))
		{
			// Blocked if using allow tags but no property is found
			return true;
		}
	}

	return false;
}

u32 CQuadLegIkSolver::GetMode() const
{
	u8 uMode = m_pProxy->m_Request.GetMode();

#if __IK_DEV
	if (ms_DebugParams.m_uForceSolverMode > 0)
	{
		uMode = ms_DebugParams.m_uForceSolverMode;
	}
#endif // __IK_DEV

	return uMode;
}

void CQuadLegIkSolver::CalcDefaults(const crSkeleton& skeleton)
{
	m_RootSituation.m_afDeltaLimits[0] = -0.10f;
	m_RootSituation.m_afDeltaLimits[1] = 0.10f;
	m_afDefaultHeightAboveCollision[0] = 0.0f;
	m_afDefaultHeightAboveCollision[1] = 0.0f;
	m_afLegLength[0] = 1.0f;
	m_afLegLength[1] = 1.0f;

	if (m_bBonesValid)
	{
		const crSkeletonData& skeletonData = skeleton.GetSkeletonData();
		const s16* aParentIndices = skeletonData.GetParentIndices();

		const s32 aLegIndex[2] = { LEFT_FRONT_LEG, LEFT_BACK_LEG };
		bool abValid[2] = { false };

		for (s32 legIndex = 0; legIndex < 2; ++legIndex)
		{
			const crIKSolverQuadruped::Goal& solverGoal = GetGoal(aLegIndex[legIndex]);
			s16 boneIndex = solverGoal.m_BoneIdx[crIKSolverQuadruped::PH];

			if (boneIndex >= 0)
			{
				// Toe1 bone
				s16 toeBoneIndex = solverGoal.m_BoneIdx[crIKSolverQuadruped::TOE1];
				Mat34V mtxToe1(skeletonData.GetDefaultTransform(toeBoneIndex));
				while (aParentIndices[toeBoneIndex] >= 0)
				{
					Transform(mtxToe1, skeletonData.GetDefaultTransform(aParentIndices[toeBoneIndex]), mtxToe1);
					toeBoneIndex = aParentIndices[toeBoneIndex];
				}

				// PH bone
				Mat34V mtxPH(skeletonData.GetDefaultTransform(boneIndex));
				while (aParentIndices[boneIndex] >= 0)
				{
					Transform(mtxPH, skeletonData.GetDefaultTransform(aParentIndices[boneIndex]), mtxPH);
					boneIndex = aParentIndices[boneIndex];
				}

				m_afDefaultHeightAboveCollision[legIndex] = mtxToe1.GetCol3().GetZf() - mtxPH.GetCol3().GetZf();
				abValid[legIndex] = true;
			}

			// Leg length
			ScalarV vLegLength(V_ZERO);
			for (int legPart = 1; legPart < NUM_LEG_PARTS - 1; ++legPart)
			{
				const Mat34V& mtxDefault = skeletonData.GetDefaultTransform(solverGoal.m_BoneIdx[legPart]);
				vLegLength = Add(vLegLength, Mag(mtxDefault.GetCol3()));
			}
			m_afLegLength[legIndex] = vLegLength.Getf();
		}

		// For quadruped skeletons that are missing front PH bones, use back height
		if (!abValid[0] && abValid[1])
		{
			m_afDefaultHeightAboveCollision[0] = m_afDefaultHeightAboveCollision[1];
		}

		// Root delta limits relative to leg length
		m_RootSituation.m_afDeltaLimits[0] = m_afLegLength[0] * ms_Tunables.m_RootMinLimitScale;
		m_RootSituation.m_afDeltaLimits[1] = m_afLegLength[0] * ms_Tunables.m_RootMaxLimitScale;
	}
}

bool CQuadLegIkSolver::CanUseAsyncShapeTests() const
{
	const CPhysical* pPhysical = m_pPed->GetGroundPhysical() ? m_pPed->GetGroundPhysical() : m_pPed->GetClimbPhysical();

	if (pPhysical)
	{
		if (pPhysical->GetIsInWater() || pPhysical->GetWasInWater() || (pPhysical->GetVelocity().Mag2() > 0.0f))
		{
			// Use sync shape tests if on a moving physical
			return false;
		}
	}

	return true;
}

void CQuadLegIkSolver::SubmitAsyncShapeTests()
{
	const ScalarV vTimeStep(fwTimer::GetTimeStep());
	const float fShapeTestRadius = GetShapeTestRadius();
	const u32 uShapeTestIncludeFlags = GetShapeTestIncludeFlags();

	// Reset shape test data
	m_pShapeTestData->Reset();

	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;
	capsuleDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	capsuleDesc.SetIncludeFlags(uShapeTestIncludeFlags);
	capsuleDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	capsuleDesc.SetIsDirected(true);

	// Velocity direction
	Vec3V vVelocity(VECTOR3_TO_VEC3V(m_pPed->GetVelocity()));
	vVelocity = NormalizeSafe(vVelocity, Vec3V(V_ZERO));

	// Always updating probe position if character has any velocity (moving, turning while idle, etc.) to get smoother
	// probe results on detailed slopes and help reduce foot jitter
	const ScalarV vTolerance(m_RootSituation.m_bMoving ? 0.0f : 0.01f);
	const Vec3V vProbePositionTolerance(vTolerance, vTolerance, ScalarV(V_ZERO));

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];
		const Vec3V vToePosition(legSituation.m_Target.GetPosition());

		// Determine the height above and below the foot for the probe
		const Vec3V vShapeTestMaxOffset(0.0f, 0.0f, GetShapeTestMaxOffset(legSituation.m_uFrontBack));
		const Vec3V vShapeTestMinOffset(0.0f, 0.0f, GetShapeTestMinOffset(legSituation.m_uFrontBack));

		// Snap x, y of toe probes to a tolerance to help reduce hysteresis in the results due to tiny movements in position
		Vec3V vDist(Abs(Subtract(legSituation.m_vProbePosition, vToePosition)));
		legSituation.m_vProbePosition = SelectFT(IsGreaterThan(vDist, vProbePositionTolerance), legSituation.m_vProbePosition, vToePosition);

		Vec3V vToeVel(Subtract(vToePosition, legSituation.m_vPrevPosition));
		vToeVel = InvScale(vToeVel, vTimeStep);
		legSituation.m_vPrevPosition = vToePosition;

		Vec3V vToeAcc(Subtract(vToeVel, legSituation.m_vPrevVelocity));
		vToeAcc = InvScale(vToeAcc, vTimeStep);
		legSituation.m_vPrevVelocity = vToeVel;

		// Project toe velocity onto character's velocity
		vToeVel = Scale(vVelocity, Dot(vToeVel, vVelocity));

		// Project toe acceleration onto character's velocity
		vToeAcc = Scale(vVelocity, Dot(vToeAcc, vVelocity));

		// Extrapolate probe position (probe position + vt + 1/2at^2)
		Vec3V vDelta(V_HALF);
		vDelta = Scale(vDelta, Scale(vTimeStep, vTimeStep));
		vDelta = Scale(vDelta, vToeAcc);
		vDelta = Add(vDelta, Scale(vToeVel, vTimeStep));

		legSituation.m_vProbePosition = Add(legSituation.m_vProbePosition, vDelta);

		Vec3V vStart(Add(legSituation.m_vProbePosition, vShapeTestMaxOffset));
		Vec3V vEnd(Add(legSituation.m_vProbePosition, vShapeTestMinOffset));

		capsuleDesc.SetCapsule(RC_VECTOR3(vStart), RC_VECTOR3(vEnd), fShapeTestRadius);
		capsuleDesc.SetResultsStructure(m_pShapeTestData->m_apResults[leg]);

		WorldProbe::GetShapeTestManager()->SubmitTest(capsuleDesc, WorldProbe::PERFORM_ASYNCHRONOUS_TEST);
	}
}

void CQuadLegIkSolver::SubmitShapeTests()
{
	const ScalarV vTimeStep(fwTimer::GetTimeStep());
	const float fShapeTestRadius = GetShapeTestRadius();
	const u32 uShapeTestIncludeFlags = GetShapeTestIncludeFlags();

	if (m_pShapeTestData->m_apResults[0]->GetResultsStatus() != WorldProbe::TEST_NOT_SUBMITTED)
	{
		// Reset pending shape test data
		m_pShapeTestData->Reset();
	}

	// Batch required capsule tests
	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;
	WorldProbe::CShapeTestBatchDesc batchDesc;
	batchDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	batchDesc.SetIncludeFlags(uShapeTestIncludeFlags);
	batchDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
#if 0 // Support not integrated yet
	ALLOC_AND_SET_CAPSULE_DESCRIPTORS(batchDesc,NUM_LEGS*NUM_FOOT_PARTS);
#endif

	const Vec3V vProbePositionTolerance(0.01f, 0.01f, 0.0f);

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];
		const Vec3V vToePosition(legSituation.m_Target.GetPosition());

		// Determine the height above and below the foot for the probe
		const Vec3V vShapeTestMaxOffset(0.0f, 0.0f, GetShapeTestMaxOffset(legSituation.m_uFrontBack));
		const Vec3V vShapeTestMinOffset(0.0f, 0.0f, GetShapeTestMinOffset(legSituation.m_uFrontBack));

		// Snap x, y of toe probes to a tolerance to help reduce hysteresis in the results due to tiny movements in position
		Vec3V vDist(Abs(Subtract(legSituation.m_vProbePosition, vToePosition)));
		legSituation.m_vProbePosition = SelectFT(IsGreaterThan(vDist, vProbePositionTolerance), legSituation.m_vProbePosition, vToePosition);

		// Construct and add capsule test
		Vec3V vStart(Add(legSituation.m_vProbePosition, vShapeTestMaxOffset));
		Vec3V vEnd(Add(legSituation.m_vProbePosition, vShapeTestMinOffset));

		capsuleDesc.SetCapsule(RC_VECTOR3(vStart), RC_VECTOR3(vEnd), fShapeTestRadius);
		capsuleDesc.SetResultsStructure(m_pShapeTestData->m_apResults[leg]);
		capsuleDesc.SetIsDirected(true);
		batchDesc.AddCapsuleTest(capsuleDesc);
	}

	WorldProbe::GetShapeTestManager()->SubmitTest(batchDesc);
}

void CQuadLegIkSolver::DeleteShapeTestData()
{
	if (m_pShapeTestData)
	{
		delete m_pShapeTestData;
		m_pShapeTestData = NULL;
	}
}

bool CQuadLegIkSolver::CapsuleTest(Vec3V_In vStart, Vec3V_In vEnd, float fRadius, u32 uShapeTestIncludeFlags, WorldProbe::CShapeTestResults& shapeTestResults)
{
	WorldProbe::CShapeTestCapsuleDesc capsuleDesc;

	capsuleDesc.SetResultsStructure(&shapeTestResults);
	capsuleDesc.SetCapsule(VEC3V_TO_VECTOR3(vStart), VEC3V_TO_VECTOR3(vEnd), fRadius);
	capsuleDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	capsuleDesc.SetIncludeFlags(uShapeTestIncludeFlags);
	capsuleDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	capsuleDesc.SetIsDirected(true);

	IK_DEV_ONLY(sysTimer timer;)
	bool bHit = WorldProbe::GetShapeTestManager()->SubmitTest(capsuleDesc);
	IK_DEV_ONLY(if (CaptureDebugRender()) { char debugText[48]; formatf(debugText, sizeof(debugText), "Shape test : %8.3f us", timer.GetUsTime()); ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText); })

	return bHit;
}

u32 CQuadLegIkSolver::GetShapeTestIncludeFlags() const
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
	}
	else if (/*m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsGoingToStandOnExitedVehicle) ||*/ m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsVaulting))
	{
		uShapeTestIncludeFlags |= ArchetypeFlags::GTA_VEHICLE_TYPE;
	}

	return uShapeTestIncludeFlags;
}

float CQuadLegIkSolver::GetShapeTestRadius() const
{
	return ms_Tunables.m_ShapeTestRadius;
}

float CQuadLegIkSolver::GetShapeTestMinOffset(u32 uFrontBack) const
{
	return m_afLegLength[uFrontBack] * -ms_Tunables.m_ShapeTestLengthScale;
}

float CQuadLegIkSolver::GetShapeTestMaxOffset(u32 uFrontBack) const
{
	return m_afLegLength[uFrontBack] * ms_Tunables.m_ShapeTestLengthScale;
}

float CQuadLegIkSolver::GetMaxIntersectionHeight(u32 uFrontBack) const
{
	return m_afLegLength[uFrontBack] * ms_Tunables.m_MaxIntersectionHeightScale;
}

void CQuadLegIkSolver::PreProcessResults()
{
	const float fShapeTestRadius = GetShapeTestRadius();
	u32 uShapeTestIncludeFlags = 0;

	if (m_bInstantSetup)
	{
		// Reset since results are no longer valid if doing an instant setup
		m_pShapeTestData->Reset();
	}

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];
		WorldProbe::CShapeTestResults& capsuleResults = *(m_pShapeTestData->m_apResults[leg]);

		// Check if results are not ready yet. Typically this is when the async probe system is under heavy load (eg. large AI fire fights), so submit a sync test instead
		if (capsuleResults.GetWaitingOnResults() || (capsuleResults.GetResultsStatus() == WorldProbe::TEST_NOT_SUBMITTED))
		{
			const Vec3V vShapeTestMaxOffset(0.0f, 0.0f, GetShapeTestMaxOffset(legSituation.m_uFrontBack));
			const Vec3V vShapeTestMinOffset(0.0f, 0.0f, GetShapeTestMinOffset(legSituation.m_uFrontBack));

			// Cancel current shape test if necessary
			capsuleResults.Reset();

			// Issue sync shape test
			if (uShapeTestIncludeFlags == 0)
			{
				uShapeTestIncludeFlags = GetShapeTestIncludeFlags();
			}

			CapsuleTest(Add(legSituation.m_vProbePosition, vShapeTestMaxOffset), Add(legSituation.m_vProbePosition, vShapeTestMinOffset), fShapeTestRadius, uShapeTestIncludeFlags, capsuleResults);
		}
	}
}

void CQuadLegIkSolver::ProcessResults()
{
	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		LegSituation& legSituation = m_aLegSituation[leg];
		const float fMaxIntersectionHeight = GetMaxIntersectionHeight(legSituation.m_uFrontBack);
		const int iGroundPositionIndex = leg < 2 ? 0 : 1;
		int iHighestIntersectionIndex = -1;

		const Vec3V vToePosition(legSituation.m_Target.GetPosition());
		Vec3V vHighestIntersectionPosition(vToePosition);
		Vec3V vHighestIntersectionNormal(V_UP_AXIS_WZERO);
		Vec3V vGroundNormal(V_UP_AXIS_WZERO);

		WorldProbe::CShapeTestResults& capsuleResults = *(m_pShapeTestData->m_apResults[leg]);

		if (capsuleResults.GetResultsReady() && (capsuleResults.GetNumHits() > 0))
		{
			vHighestIntersectionPosition.SetZf(-FLT_MAX);
			int iResultIndex = 0;

			for (WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it, ++iResultIndex)
			{
				// Find the highest intersection
				if (it->GetHitDetected())
				{
					const float fHitPositionZ = it->GetHitPosition().z;
					const float fIntersectionHeight = fHitPositionZ - m_RootSituation.m_avGroundPosition[iGroundPositionIndex].GetZf();

					if (fIntersectionHeight > fMaxIntersectionHeight)
					{
						// Ignore intersections beyond a certain height from the character's ground position
						// Ignore all further intersections below this one since the character is likely penetrating an object
						break;
					}

					Vec3V vIntersectionPosition(vToePosition);
					vIntersectionPosition.SetZf(fHitPositionZ);

					if (IsGreaterThanAll(vIntersectionPosition.GetZ(), vHighestIntersectionPosition.GetZ()))
					{
						vHighestIntersectionPosition = vIntersectionPosition;
						vHighestIntersectionNormal = VECTOR3_TO_VEC3V(it->GetHitNormal());
						iHighestIntersectionIndex = iResultIndex;
					}
				}
			}
		}

		if (iHighestIntersectionIndex != -1)
		{
			legSituation.m_bIntersection = true;

			legSituation.m_fGroundHeight = vHighestIntersectionPosition.GetZf();
			vGroundNormal = ClampNormal(vHighestIntersectionNormal);
		}

		legSituation.m_vGroundNormal = SmoothNormal(vGroundNormal, legSituation.m_vGroundNormal, ms_Tunables.m_EffectorGroundNormalInterpolationRate, m_bInstantSetup != 0);

	#if __IK_DEV
		if (CaptureDebugRender() && ms_DebugParams.m_bRenderProbes)
		{
			WorldProbe::CShapeTestResults& capsuleResults = *(m_pShapeTestData->m_apResults[leg]);
			bool bIgnoreIntersection = false;

			if (capsuleResults.GetResultsReady() && (capsuleResults.GetNumHits() > 0))
			{
				for (WorldProbe::ResultIterator it = capsuleResults.begin(); it < capsuleResults.last_result(); ++it)
				{
					if (it->GetHitDetected())
					{
						Color32 colour = ((iHighestIntersectionIndex != -1) && (it == (capsuleResults.begin() + iHighestIntersectionIndex))) ? Color_yellow : Color_yellow4;

						const float fHitPositionZ = it->GetHitPosition().z;
						const float fIntersectionHeight = fHitPositionZ - m_RootSituation.m_avGroundPosition[iGroundPositionIndex].GetZf();

						if (bIgnoreIntersection || (fIntersectionHeight > fMaxIntersectionHeight))
						{
							colour = Color_red4;
							bIgnoreIntersection = true;
						}

						ms_DebugHelper.Sphere(it->GetHitPosition(), 0.01f, colour, false);
					}
				}
			}
		}
	#endif // __IK_DEV
	}
}

bool CQuadLegIkSolver::SolveRoot(crSkeleton& skeleton)
{
	IK_DEV_ONLY(char debugText[128];)

	const int boneCount = skeleton.GetBoneCount();
	const Vec3V vUp(m_pPed->GetTransform().GetC());

	Mat34V mtxRootGlobal;
	skeleton.GetGlobalMtx(m_uRootBoneIdx, mtxRootGlobal);

	// Project ground position onto character up direction
	Vec3V vRootToPosition(Subtract(m_RootSituation.m_avGroundPosition[0], mtxRootGlobal.GetCol3()));
	ScalarV vRootHeight(Dot(vRootToPosition, vUp));

	// Adjust height of character depending on lowest toe.
	Vec3V vPosition;
	ScalarV vHeight;
	int lowestIndex = 0;
	float fLowestHeight = FLT_MAX;

	// Project each leg ground position onto character up direction
	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		vPosition = m_aLegSituation[leg].m_Target.GetPosition();
		vPosition.SetZf(m_aLegSituation[leg].m_fGroundHeight);

		vRootToPosition = Subtract(vPosition, mtxRootGlobal.GetCol3());
		vHeight = Dot(vRootToPosition, vUp);

		if (vHeight.Getf() < fLowestHeight)
		{
			fLowestHeight = vHeight.Getf();
			lowestIndex = leg;
		}
	}

	float fDelta = fLowestHeight - vRootHeight.Getf();
	float fDeltaClamped = Clamp(fDelta, m_RootSituation.m_afDeltaLimits[0], m_RootSituation.m_afDeltaLimits[1]);
	float fDeltaRate = !m_bInstantSetup ? ms_Tunables.m_RootInterpolationRate : INSTANT_BLEND_IN_DELTA;
	Approach(m_RootSituation.m_fDeltaHeight, fDeltaClamped, fDeltaRate, fwTimer::GetTimeStep());

#if __IK_DEV
	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "Root Int Bl: %d %4.2f", m_RootSituation.m_bIntersection ? 1 : 0, m_RootSituation.m_fBlend);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Root Delta : %6.3f %6.3f %6.3f", fDelta, fDeltaClamped, m_RootSituation.m_fDeltaHeight);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Lowest Leg : %6d %6.3f %6.3f", lowestIndex, fLowestHeight, vRootHeight.Getf());
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	fDelta = m_RootSituation.m_fDeltaHeight * m_RootSituation.m_fBlend;

	if (fabsf(fDelta) > SMALL_FLOAT)
	{
		Vec3V vOffset(UnTransform3x3Ortho(*skeleton.GetParentMtx(), m_pPed->GetTransform().GetC()));
		vOffset = Scale(vOffset, ScalarV(fDelta));

		for (int bone = m_uRootBoneIdx; bone < boneCount; ++bone)
		{
			Mat34V& boneMtx = skeleton.GetObjectMtx(bone);
			boneMtx.SetCol3(Add(boneMtx.GetCol3(), vOffset));
		}

		skeleton.InverseUpdate();
		return true;
	}

	return false;
}

void CQuadLegIkSolver::SetupLeg(crSkeleton& skeleton, crIKSolverQuadruped::Goal& goal, LegSituation& legSituation)
{
	ScalarV vGroundHeight(legSituation.m_fGroundHeight);

#if __IK_DEV
	char debugText[128];

	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "Intersected: %d %4.2f", legSituation.m_bIntersection ? 1 : 0, legSituation.m_fBlend);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Gnd Hgt    : %8.3f", legSituation.m_fGroundHeight);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		// Override ground height if necessary
		vGroundHeight = Add(vGroundHeight, ScalarV(ms_DebugParams.m_fGroundHeightDelta));
	}
#endif // __IK_DEV

	const ScalarV vMinHeightAboveCollision(m_afDefaultHeightAboveCollision[legSituation.m_uFrontBack]);
	const ScalarV vMinLegLength(m_afLegLength[legSituation.m_uFrontBack] * ms_Tunables.m_LegMinLengthScale);
	const ScalarV vMaxLegLength(m_afLegLength[legSituation.m_uFrontBack] * ms_Tunables.m_LegMaxLengthScale);
	const ScalarV vAnimHeightBlend(ms_Tunables.m_AnimHeightBlend);

	Mat34V mtxUpper;
	skeleton.GetGlobalMtx(goal.m_BoneIdx[crIKSolverQuadruped::THIGH], mtxUpper);

	Vec3V vEffectorPosition(legSituation.m_Target.GetPosition());

	ScalarV vHeightAboveCollision(Subtract(vEffectorPosition.GetZ(), m_RootSituation.m_avGroundPosition[legSituation.m_uFrontBack].GetZ()));
	vHeightAboveCollision = Scale(vHeightAboveCollision, vAnimHeightBlend);
	vHeightAboveCollision = Max(vHeightAboveCollision, vMinHeightAboveCollision);

	ScalarV vTargetHeight(Add(vGroundHeight, vHeightAboveCollision));

	// Clamp target height relative to top of leg
	ScalarV vMaxHeight(Subtract(mtxUpper.GetCol3().GetZ(), vMinLegLength));
	ScalarV vMinHeight(Subtract(mtxUpper.GetCol3().GetZ(), vMaxLegLength));

	ScalarV vTargetHeightClamped = Clamp(vTargetHeight, vMinHeight, vMaxHeight);

	float fDelta = vTargetHeightClamped.Getf() - vEffectorPosition.GetZf();

	if (m_RootSituation.m_bMoving)
	{
		fDelta = Max(fDelta, 0.0f);
	}

	float fDeltaRate = !m_bInstantSetup ? ms_Tunables.m_EffectorInterpolationRate : INSTANT_BLEND_IN_DELTA;
	Approach(legSituation.m_fDeltaHeight, fDelta, fDeltaRate, fwTimer::GetTimeStep());

#if __IK_DEV
	if (CaptureDebugRender())
	{
		formatf(debugText, sizeof(debugText), "Tgt Hgt Lim: %8.3f %8.3f", vMinHeight.Getf(), vMaxHeight.Getf());
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Tgt Hgt    : %8.3f %8.3f", vTargetHeight.Getf(), vTargetHeightClamped.Getf());
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		formatf(debugText, sizeof(debugText), "Delta      : %8.3f %8.3f", fDelta, legSituation.m_fDeltaHeight);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	fDelta = legSituation.m_fDeltaHeight * legSituation.m_fBlend;

	if (fDelta != 0.0f)
	{
		goal.m_Enabled = true;
		goal.m_Flags = 0;

		Mat34V mtxTarget;
		Mat34VFromTransformV(mtxTarget, legSituation.m_Target);
		mtxTarget.SetM23(vEffectorPosition.GetZf() + fDelta);

		// Transform to object space
		UnTransformFull(mtxTarget, *skeleton.GetParentMtx(), mtxTarget);
		ikAssertf(IsFiniteAll(mtxTarget), "CQuadLegIkSolver::SetupLeg - goal.m_Target not finite");

		TransformVFromMat34V(goal.m_Target, mtxTarget);
	}
}

Vec3V_Out CQuadLegIkSolver::ClampNormal(Vec3V_In vNormal)
{
	const ScalarV vLimit(ms_Tunables.m_ClampNormalLimit);
	const Vec3V vAxis(V_UP_AXIS_WZERO);

	Vec3V vInNormal(vNormal);
	ikAssert(!IsZeroAll(vInNormal));

	ScalarV vAngle(Dot(vAxis, vInNormal));
	vAngle = Arccos(vAngle);

	if (IsGreaterThanAll(vAngle, vLimit))
	{
		Vec3V vRotationAxis(Cross(vAxis, vInNormal));
		vRotationAxis = NormalizeSafe(vRotationAxis, Vec3V(V_X_AXIS_WZERO));

		QuatV qRotation(QuatVFromAxisAngle(vRotationAxis, vLimit));
		vInNormal = Transform(qRotation, vAxis);
	}

	return vInNormal;
}

Vec3V_Out CQuadLegIkSolver::SmoothNormal(Vec3V_In vDesired, Vec3V_In vCurrent, const float fRate, const float fTimeStep, const bool bForceDesired)
{
	Vec3V vNormal(vCurrent);

	if (IsZeroAll(vCurrent) || bForceDesired)
	{
		vNormal = vDesired;
	}
	else
	{
		ScalarV vAngle(Dot(vCurrent, vDesired));
		vAngle = Clamp(vAngle, ScalarV(V_ZERO), ScalarV(V_ONE));
		vAngle = Arccos(vAngle);

		ScalarV vDelta(Scale(vAngle, ScalarV(fRate)));
		vDelta = Scale(vDelta, ScalarV(fTimeStep));

		if (IsLessThanAll(vAngle, vDelta))
		{
			vNormal = vDesired;
		}
		else
		{
			Vec3V vRotationAxis(Cross(vCurrent, vDesired));
			vRotationAxis = Normalize(vRotationAxis);

			QuatV qRotation(QuatVFromAxisAngle(vRotationAxis, vDelta));
			vNormal = Transform(qRotation, vCurrent);
			vNormal = NormalizeSafe(vNormal, Vec3V(V_UP_AXIS_WZERO));
		}
	}

	return vNormal;
}

#if __IK_DEV

void CQuadLegIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_DebugHelper.Render();
	}
}

void CQuadLegIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CQuadLegIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeQuadLeg], NullCB, "");
		pBank->AddToggle("Debug", &ms_DebugParams.m_bRenderDebug, NullCB, "");

		pBank->PushGroup("Debug Rendering", false);
			pBank->AddToggle("Render Probes", &CQuadLegIkSolver::ms_DebugParams.m_bRenderProbes);
			pBank->AddToggle("Render Swept Probes", &CQuadLegIkSolver::ms_DebugParams.m_bRenderSweptProbes);
		pBank->PopGroup();

#if ENABLE_QUADLEGIK_SOLVER_WIDGETS
		pBank->PushGroup("Settings", false);
			pBank->AddSlider("Ground Height Delta", &CQuadLegIkSolver::ms_DebugParams.m_fGroundHeightDelta, -2.0f, 2.0f, 0.01f);
			static const char* sSolverModes[] =
			{
				"Default",
				"Partial",
				"Full"
			};
			pBank->AddCombo("Forced Solver Mode", &CQuadLegIkSolver::ms_DebugParams.m_uForceSolverMode, 3, sSolverModes);
		pBank->PopGroup();
#endif // ENABLE_QUADLEGIK_SOLVER_WIDGETS
	pBank->PopGroup();
}

bool CQuadLegIkSolver::CaptureDebugRender()
{
	if (ms_DebugParams.m_bRenderDebug)
	{
		CEntity* pSelectedEntity = (CEntity*)g_PickerManager.GetSelectedEntity();

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

void CQuadLegIkSolver::DebugRenderLowerBody(const crSkeleton &skeleton, Color32 sphereColor, Color32 lineColor)
{
	TUNE_GROUP_FLOAT(QUAD_LEG_IK, fDebugLowerBodySideOffset, -0.6f, -1.0f, 1.0f, 0.01f);
	TUNE_GROUP_FLOAT(QUAD_LEG_IK, fDebugLowerBodySphereSize, 0.02f, 0.0f, 0.2f, 0.001f);

	Matrix34 mtxRoot;
	Matrix34 mtxPelvis;
	Matrix34 mtxSpine;
	Matrix34 mtxBone;

	Vector3 vSideOffset(VEC3V_TO_VECTOR3(Scale(m_pPed->GetTransform().GetA(), ScalarV(fDebugLowerBodySideOffset))));

	skeleton.GetGlobalMtx(m_uRootBoneIdx, RC_MAT34V(mtxRoot));
	skeleton.GetGlobalMtx(m_uPelvisBoneIdx, RC_MAT34V(mtxPelvis));
	skeleton.GetGlobalMtx(m_uSpineBoneIdx, RC_MAT34V(mtxSpine));

	ms_DebugHelper.Sphere(mtxRoot.d + vSideOffset, fDebugLowerBodySphereSize, sphereColor);
	ms_DebugHelper.Sphere(mtxPelvis.d + vSideOffset, fDebugLowerBodySphereSize, sphereColor);
	ms_DebugHelper.Sphere(mtxSpine.d + vSideOffset, fDebugLowerBodySphereSize, sphereColor);

	ms_DebugHelper.Line(mtxRoot.d + vSideOffset, mtxPelvis.d + vSideOffset, lineColor, lineColor);
	ms_DebugHelper.Line(mtxRoot.d + vSideOffset, mtxSpine.d + vSideOffset, lineColor, lineColor);

	for (int leg = 0; leg < NUM_LEGS; ++leg)
	{
		crIKSolverQuadruped::Goal& solverGoal = GetGoal(leg);

		Vector3 vStart(leg <= RIGHT_FRONT_LEG ? mtxSpine.d : mtxPelvis.d);

		for (int legPart = 0; legPart < NUM_LEG_PARTS; ++legPart)
		{
			if (solverGoal.m_BoneIdx[legPart] != (u16)-1)
			{
				skeleton.GetGlobalMtx(solverGoal.m_BoneIdx[legPart], RC_MAT34V(mtxBone));
				ms_DebugHelper.Line(vStart + vSideOffset, mtxBone.d + vSideOffset, lineColor, lineColor);
				ms_DebugHelper.Sphere(mtxBone.d + vSideOffset, fDebugLowerBodySphereSize, sphereColor);
				vStart = mtxBone.d;
			}
		}
	}
}

#endif // __IK_DEV

#endif // IK_QUAD_LEG_SOLVER_ENABLE
