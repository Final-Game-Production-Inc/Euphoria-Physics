// 
// ik/solvers/TorsoVehicleSolver.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "TorsoVehicleSolver.h"

#include "diag/art_channel.h"
#include "fwdebug/picker.h"
#include "phcore/phmath.h"
#include "vectormath/classfreefuncsv.h"

#include "animation/EventTags.h"
#include "ik/IkRequest.h"
#include "ik/solvers/ArmIkSolver.h"
#include "ik/solvers/TorsoVehicleSolverProxy.h"
#include "Peds/ped.h"
#include "vehicles/vehicle.h"
#include "vehicles/Automobile.h"
#include "vehicles/Bike.h"

#define TORSOVEHICLE_SOLVER_POOL_MAX		16
#define ENABLE_TORSOVEHICLE_SOLVER_WIDGETS	0

FW_INSTANTIATE_CLASS_POOL(CTorsoVehicleIkSolver, TORSOVEHICLE_SOLVER_POOL_MAX, atHashString("CTorsoVehicleIkSolver",0xd6b42498));

ANIM_OPTIMISATIONS();

CTorsoVehicleIkSolver::Params CTorsoVehicleIkSolver::ms_Params =
{
	{																		// avSpineDeltaLimits[NUM_SPINE_PARTS][2]
		{ Vec3V( -5.0f, -7.0f, -5.0f), Vec3V( 5.0f, 7.0f, 5.0f) },			// SPINE0
		{ Vec3V( -5.0f, -7.0f, -5.0f), Vec3V( 5.0f, 7.0f, 5.0f) },			// SPINE1
		{ Vec3V( -5.0f, -7.0f, -5.0f), Vec3V( 5.0f, 7.0f, 5.0f) },			// SPINE2
		{ Vec3V( -5.0f, -7.0f, -5.0f), Vec3V( 5.0f, 7.0f, 5.0f) }			// SPINE3
	},
	{																		// avSpineJointLimits[NUM_SPINE_PARTS][2]
		{ Vec3V( -10.0f, -20.0f, -8.0f), Vec3V( 10.0f, 20.0f, 60.0f) },		// SPINE0
		{ Vec3V( -10.0f, -13.0f, -8.0f), Vec3V( 10.0f, 13.0f, 18.0f) },		// SPINE1
		{ Vec3V( -10.0f, -13.0f, -8.0f), Vec3V( 10.0f, 13.0f, 18.0f) },		// SPINE2
		{ Vec3V( -10.0f, -13.0f, -8.0f), Vec3V( 10.0f, 13.0f, 25.0f) }		// SPINE3
	},
	{ 0.3f, 0.25f, 0.25f, 0.2f },											// afSpineWeights[NUM_SPINE_PARTS]
	{ 1.5f, 5.0f },															// afBlendRate[NUM_SOLVERS]
	5.0f,																	// fDeltaRate
	{ 10.0f, 40.0f }														// afAnimatedLeanLimits[2]
};

#if __IK_DEV
CDebugRenderer CTorsoVehicleIkSolver::ms_debugHelper;
bool CTorsoVehicleIkSolver::ms_bRenderDebug = false;
int CTorsoVehicleIkSolver::ms_iNoOfTexts = 0;
#endif //__IK_DEV

CTorsoVehicleIkSolver::CTorsoVehicleIkSolver(CPed *pPed, CTorsoVehicleIkSolverProxy* pProxy)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_vDeltaEulers(V_ZERO)
, m_pPed(pPed)
, m_pProxy(pProxy)
{
	SetPrority(5);
	Reset();

	bool bBonesValid = true;

	static const eAnimBoneTag aeSpineBoneTags[] = { BONETAG_SPINE0, BONETAG_SPINE1, BONETAG_SPINE2, BONETAG_SPINE3 };
	for (int spineBoneTagIndex = 0; spineBoneTagIndex < NUM_SPINE_PARTS; ++spineBoneTagIndex)
	{
		m_auSpineBoneIdx[spineBoneTagIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeSpineBoneTags[spineBoneTagIndex]);
		bBonesValid &= (m_auSpineBoneIdx[spineBoneTagIndex] != (u16)-1);
	}

	static const eAnimBoneTag aeArmBoneTags[NUM_ARMS][NUM_ARM_PARTS] = 
	{ 
		{ BONETAG_L_UPPERARM, BONETAG_L_FOREARM, BONETAG_L_HAND, BONETAG_L_IK_HAND },
		{ BONETAG_R_UPPERARM, BONETAG_R_FOREARM, BONETAG_R_HAND, BONETAG_R_IK_HAND }
	};
	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		for (int armPartIndex = 0; armPartIndex < NUM_ARM_PARTS; ++armPartIndex)
		{
			m_auArmBoneIdx[armIndex][armPartIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeArmBoneTags[armIndex][armPartIndex]);
			bBonesValid &= (m_auArmBoneIdx[armIndex][armPartIndex] != (u16)-1);
		}
	}

	m_uFacingBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_FACING_DIR);
	m_uNeckBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);
	m_uHeadBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);

	bBonesValid &= (m_uHeadBoneIdx != (u16)-1);

	m_bBonesValid = bBonesValid;

	artAssertf(bBonesValid, "Modelname = (%s) is missing bones necessary for torso vehicle ik. Expects the following bones: "
		"BONETAG_SPINE0-3/IK_ROOT, BONETAG_[L/R]_UPPERARM, BONETAG_[L/R]_FOREARM, BONETAG_[L/R]_IK_HAND", pPed ? pPed->GetModelName() : "Unknown model");

	CalculateArmLength();
}

CTorsoVehicleIkSolver::~CTorsoVehicleIkSolver()
{
	Reset();
}

void CTorsoVehicleIkSolver::Reset()
{
	m_vDeltaEulers.ZeroComponents();
	for (int solver = 0; solver < NUM_SOLVERS; ++solver)
	{
		m_afBlend[solver] = 0.0f;
	}
	m_bActive = false;
}

void CTorsoVehicleIkSolver::Iterate(float dt, crSkeleton& skeleton)
{
	// Iterate is called by the motiontree thread. It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorsoVehicle] || CBaseIkManager::ms_DisableAllSolvers)
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
		sprintf(debugText, "%s\n", "TorsoVehicle::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(dt, skeleton);
	SolveLean(dt, skeleton);
	SolveArmReach(skeleton);
}

void CTorsoVehicleIkSolver::Request(const CIkRequestTorsoVehicle& UNUSED_PARAM(request))
{
	m_bActive = true;
}

void CTorsoVehicleIkSolver::PreIkUpdate(float UNUSED_PARAM(deltaTime))
{
}

void CTorsoVehicleIkSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		m_bActive = false;

		// Reset solver instantly if ped is not visible since the motion tree update will be skipped
		// and the solver could be reused by another ped
		if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
		{
			Reset();
			m_pProxy->SetComplete(true);
		}
	}
}

void CTorsoVehicleIkSolver::Update(float deltaTime, crSkeleton& UNUSED_PARAM(skeleton))
{
	ikAssertf(IsValid(), "CTorsoVehicleIkSolver not valid for this skeleton");

	if (!IsValid())
	{
		m_pProxy->SetComplete(true);
		return;
	}

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoVehicleSolver())
	{
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoVehicleSolver())
		{
			Reset();
			m_pProxy->SetComplete(true);
		}

		return;
	}

	const fwEntity* pAttachParent = GetAttachParent();
	const bool bVehicleValid = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsEnteringVehicle) || (pAttachParent != NULL);
	float afBlendRate[NUM_SOLVERS];

	if (!m_bActive || !bVehicleValid || m_pProxy->IsBlocked())
	{
		if (((m_afBlend[SOLVER_LEAN] == 0.0f) && (m_afBlend[SOLVER_ARM_REACH] == 0.0f)) || !bVehicleValid)
		{
			Reset();
			m_pProxy->SetComplete(true);
			return;
		}

		for (int solver = 0; solver < NUM_SOLVERS; ++solver)
		{
			afBlendRate[solver] = -ms_Params.afBlendRate[solver];
		}
	}
	else
	{
		for (int solver = 0; solver < NUM_SOLVERS; ++solver)
		{
			afBlendRate[solver] = ms_Params.afBlendRate[solver];
		}
	}

	const u32 uFlags = m_pProxy->m_Request.GetFlags();
	const bool bDoingDriveby = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsDoingDriveby);
	const bool bAllowLean = pAttachParent && (uFlags & (TORSOVEHICLEIK_APPLY_LEAN | TORSOVEHICLEIK_APPLY_LEAN_RESTRICTED));

	if (!bAllowLean || m_pPed->GetPedResetFlag(CPED_RESET_FLAG_PuttingOnHelmet))
	{
		afBlendRate[SOLVER_LEAN] = -ms_Params.afBlendRate[SOLVER_LEAN];
	}

	// Check for blocking tags
	fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
	if (pAnimDirector)
	{
		if (!bDoingDriveby)
		{
			// Using LookIK block tag to block lean solver for tuck poses
			const CClipEventTags::CLookIKControlEventTag* pLookIkControl = static_cast<const CClipEventTags::CLookIKControlEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LookIkControl));
			if (pLookIkControl && !pLookIkControl->GetAllowed())
			{
				afBlendRate[SOLVER_LEAN] = -ms_Params.afBlendRate[SOLVER_LEAN];
			}
		}

		// Using ArmIK block tag to block lean solver
		const CClipEventTags::CArmsIKEventTag* pArmIkControl = static_cast<const CClipEventTags::CArmsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::ArmsIk));
		if (pArmIkControl && !pArmIkControl->GetAllowed() && pArmIkControl->GetLeft() && pArmIkControl->GetRight())
		{
			afBlendRate[SOLVER_LEAN] = -ms_Params.afBlendRate[SOLVER_LEAN];
		}
	}

	for (int solver = 0; solver < NUM_SOLVERS; ++solver)
	{
		m_afBlend[solver] += deltaTime * afBlendRate[solver];
		m_afBlend[solver] = Clamp(m_afBlend[solver], 0.0f, 1.0f);
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[64];
		formatf(debugText, "Blend: %5.3f, %5.3f", m_afBlend[SOLVER_LEAN], m_afBlend[SOLVER_ARM_REACH]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "Lean : %5.3f, %5.3f", m_pProxy->m_Request.GetAnimatedLean(), GetAnimatedLeanAngle() * RtoD);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV
}

void CTorsoVehicleIkSolver::SolveLean(float deltaTime, crSkeleton& skeleton)
{
	IK_DEV_ONLY(char debugText[128];)

	if (m_afBlend[SOLVER_LEAN] > 0.0f)
	{
		const fwEntity* pAttachParent = GetAttachParent();
		if (pAttachParent == NULL)
			return;

		const ScalarV vToRadians(V_TO_RADIANS);
		const ScalarV vDelta(Clamp(ms_Params.fDeltaRate * deltaTime, 0.0f, 1.0f));
		const ScalarV vAnimatedLeanAngle(GetAnimatedLeanAngle());
		const ScalarV vSolverBlend(m_afBlend[SOLVER_LEAN]);
		const ScalarV vEpsilon(V_FLT_SMALL_6);

		const u32 uFlags = m_pProxy->m_Request.GetFlags();
		const bool bUsePedOrientation = (uFlags & TORSOVEHICLEIK_APPLY_WRT_PED_ORIENTATION) != 0;

		Mat34V mtxPed(m_pPed->GetTransform().GetMatrix());
		Mat34V mtxVehicle(bUsePedOrientation ? mtxPed : pAttachParent->GetTransform().GetMatrix());
		Mat34V mtxTarget(mtxVehicle);
		Mat34V mtxSource;
		Mat34V mtxTransform;

		Vec3V vEulersXYZ[2];
		Vec3V vLimits[2];
		Vec3V vAxis;

		QuatV qDeltaScaled;
		QuatV qSpineLocal;
		QuatV qSpineTarget;
		QuatV qCounterRotation;

		ScalarV vAngle;

		// Calculate level vehicle orientation target
		Vec3V vFwdDirection(mtxTarget.GetCol1());
		vFwdDirection.SetZ(vEpsilon);
		vFwdDirection = Normalize(vFwdDirection);

		Vec3V vUpDirection(mtxTarget.GetCol2());
		vUpDirection.SetX(vEpsilon);
		vUpDirection.SetY(vEpsilon);
		vUpDirection = Normalize(vUpDirection);

		mtxTarget.SetCol1(vFwdDirection);
		mtxTarget.SetCol2(Normalize(Cross(vUpDirection, vFwdDirection)));
		mtxTarget.SetCol0(Normalize(Cross(vFwdDirection, mtxTarget.GetCol2())));

		// Add animated lean
		Mat34VRotateLocalZ(mtxTarget, vAnimatedLeanAngle);

		// Calculate orientation source. Using facing direction which is mover direction rotated by delta stored in facing dir bone (IK_Root)
		Mat34V mtxFacingDir(mtxPed);
		Transform3x3(mtxFacingDir, mtxFacingDir, skeleton.GetLocalMtx(m_uFacingBoneIdx));

		// Counter any yaw from facing direction orientation relative to vehicle forward
		qCounterRotation = QuatVFromVectors(mtxFacingDir.GetCol1(), mtxVehicle.GetCol1(), mtxFacingDir.GetCol2());
		Mat34VFromQuatV(mtxTransform, qCounterRotation);
		Transform3x3(mtxFacingDir, mtxTransform, mtxFacingDir);

		mtxSource.SetCol0(mtxFacingDir.GetCol2());
		mtxSource.SetCol1(mtxFacingDir.GetCol1());
		mtxSource.SetCol2(Negate(mtxFacingDir.GetCol0()));
		mtxSource.SetCol3(mtxFacingDir.GetCol3());

		// Calculate delta to counter vehicle orientation
		QuatV qTarget(QuatVFromMat34V(mtxTarget));
		QuatV qDelta(QuatVFromMat34V(mtxSource));
		qDelta = InvertNormInput(qDelta);
		qDelta = Multiply(qDelta, qTarget);

		// Interpolate current delta eulers to raw value
		Vec3V vDeltaEulersRaw(QuatVToEulersXYZ(qDelta));

		// Scale the delta to restrict the amount of torso movement
		if (m_pProxy)
		{
			m_vDeltaEulers = Scale(m_vDeltaEulers, ScalarVFromF32(m_pProxy->m_Request.GetDeltaScale()));
		}

		m_vDeltaEulers = Lerp(vDelta, m_vDeltaEulers, vDeltaEulersRaw);
		qDelta = QuatVFromEulersXYZ(m_vDeltaEulers);

	#if __IK_DEV
		if (CaptureDebugRender())
		{
			sprintf(debugText, "DELTA(Raw) : %8.3f, %8.3f, %8.3f", vDeltaEulersRaw.GetXf() * RtoD, vDeltaEulersRaw.GetYf() * RtoD, vDeltaEulersRaw.GetZf() * RtoD);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			sprintf(debugText, "DELTA(Curr): %8.3f, %8.3f, %8.3f", m_vDeltaEulers.GetXf() * RtoD, m_vDeltaEulers.GetYf() * RtoD, m_vDeltaEulers.GetZf() * RtoD);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif // __IK_DEV

		// Apply rotation, scaled to each spine bone
		for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
		{
			// Scale by spine weight
			QuatVToAxisAngle(vAxis, vAngle, qDelta);
			vAngle = Scale(vAngle, ScalarV(ms_Params.afSpineWeights[spineBoneIndex]));
			qDeltaScaled = QuatVFromAxisAngle(vAxis, vAngle);

			// Clamp to delta limits
			vLimits[0] = ms_Params.avSpineDeltaLimits[spineBoneIndex][0];
			vLimits[1] = ms_Params.avSpineDeltaLimits[spineBoneIndex][1];
			vLimits[0] = Scale(vLimits[0], vToRadians);
			vLimits[1] = Scale(vLimits[1], vToRadians);

			vEulersXYZ[0] = QuatVToEulersXYZ(qDeltaScaled);
			vEulersXYZ[1] = Clamp(vEulersXYZ[0], vLimits[0], vLimits[1]);

			qDeltaScaled = Normalize(QuatVFromEulersXYZ(vEulersXYZ[1]));

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "SPN%d : %8.3f, %8.3f, %8.3f (Delta)", spineBoneIndex, vEulersXYZ[0].GetXf() * RtoD, vEulersXYZ[0].GetYf() * RtoD, vEulersXYZ[0].GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "SPN%d : %8.3f, %8.3f, %8.3f (Delta Actual)", spineBoneIndex, vEulersXYZ[1].GetXf() * RtoD, vEulersXYZ[1].GetYf() * RtoD, vEulersXYZ[1].GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZ[0], vEulersXYZ[1]) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV

			// Rotate spine local
			Mat34V& mtxSpineLocal = skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBoneIndex]);
			qSpineLocal = QuatVFromMat34V(mtxSpineLocal);

			qSpineTarget = Multiply(qSpineLocal, qDeltaScaled);

			// Clamp to joint limits
			TUNE_GROUP_FLOAT(TORSO_VEHICLE_IK, LIMITED_LEAN, 0.4f, 0.0f, 1.0f, 0.01f);
			const bool bLimitLean = (uFlags & TORSOVEHICLEIK_APPLY_LIMITED_LEAN) != 0;
			const ScalarV scScale = ScalarVFromF32(bLimitLean ? LIMITED_LEAN : 1.0f);
			vLimits[0] = Scale(ms_Params.avSpineJointLimits[spineBoneIndex][0], scScale);
			vLimits[1] = Scale(ms_Params.avSpineJointLimits[spineBoneIndex][1], scScale);
			vLimits[0] = Scale(vLimits[0], vToRadians);
			vLimits[1] = Scale(vLimits[1], vToRadians);

			vEulersXYZ[0] = QuatVToEulersXYZ(qSpineTarget);
			vEulersXYZ[1] = Clamp(vEulersXYZ[0], vLimits[0], vLimits[1]);

			qSpineTarget = Normalize(QuatVFromEulersXYZ(vEulersXYZ[1]));

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "SPN%d : %8.3f, %8.3f, %8.3f (Joint)", spineBoneIndex, vEulersXYZ[0].GetXf() * RtoD, vEulersXYZ[0].GetYf() * RtoD, vEulersXYZ[0].GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "SPN%d : %8.3f, %8.3f, %8.3f (Joint Actual)", spineBoneIndex, vEulersXYZ[1].GetXf() * RtoD, vEulersXYZ[1].GetYf() * RtoD, vEulersXYZ[1].GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZ[0], vEulersXYZ[1]) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV

			// Blend local spine from skeleton to local spine target
			if (CanSlerp(qSpineLocal, qSpineTarget))
			{
				qSpineTarget = PrepareSlerp(qSpineLocal, qSpineTarget);
				qSpineLocal = Slerp(SlowInOut(vSolverBlend), qSpineLocal, qSpineTarget);
			}

			ikAssertf(IsFiniteAll(qSpineLocal), "qSpineLocal not finite");
			Mat34VFromQuatV(mtxSpineLocal, qSpineLocal, mtxSpineLocal.GetCol3());
		}

		// Counter head and neck for animated lean at half the animated lean angle
		const Mat34V& mtxNeck = skeleton.GetObjectMtx(m_uNeckBoneIdx);
		const Mat34V& mtxHead = skeleton.GetObjectMtx(m_uHeadBoneIdx);
		Mat34V& mtxNeckLocal = skeleton.GetLocalMtx(m_uNeckBoneIdx);
		Mat34V& mtxHeadLocal = skeleton.GetLocalMtx(m_uHeadBoneIdx);

		Vec3V vCounterRotationAxis(UnTransform3x3Ortho(*skeleton.GetParentMtx(), mtxTarget.GetCol2()));
		ScalarV vCounterAngle(Scale(Negate(vAnimatedLeanAngle), ScalarV(V_QUARTER)));
		vCounterAngle = Scale(vCounterAngle, vSolverBlend);

		qCounterRotation = QuatVFromAxisAngle(UnTransform3x3Full(mtxNeck, vCounterRotationAxis), vCounterAngle);
		Mat34VFromQuatV(mtxTransform, qCounterRotation);
		Transform3x3(mtxNeckLocal, mtxTransform, mtxNeckLocal);

		qCounterRotation = QuatVFromAxisAngle(UnTransform3x3Full(mtxHead, vCounterRotationAxis), vCounterAngle);
		Mat34VFromQuatV(mtxTransform, qCounterRotation);
		Transform3x3(mtxHeadLocal, mtxTransform, mtxHeadLocal);

		// Update skeleton
		skeleton.PartialUpdate(m_auSpineBoneIdx[SPINE0]);

	#if __IK_DEV
		if (CaptureDebugRender())
		{
			Vec3V vDebugPosition(AddScaled(mtxVehicle.GetCol3(), mtxVehicle.GetCol0(), ScalarV(-0.5f)));

			Mat34V mtxDebug(mtxTarget);
			mtxDebug.SetCol3(vDebugPosition);
			ms_debugHelper.Axis(RCC_MATRIX34(mtxDebug), 0.3f);

			mtxDebug = mtxSource;
			mtxDebug.SetCol3(vDebugPosition);
			ms_debugHelper.Axis(RCC_MATRIX34(mtxDebug), 0.2f);
		}
	#endif // __IK_DEV
	}
}

void CTorsoVehicleIkSolver::SolveArmReach(rage::crSkeleton& skeleton)
{
	IK_DEV_ONLY(char debugText[64];)

	if (m_afBlend[SOLVER_ARM_REACH] > 0.0f)
	{
		ArmReachSituation aArmReachSituation[NUM_ARMS];

		const ScalarV vArmLength(m_fArmLength);
		const ScalarV vScalarEps(V_ZERO);
		const ScalarV vScalarMaxReach(0.50f);

		Mat34V mtxParent(*skeleton.GetParentMtx());
		const fwEntity* pAttachParent = GetAttachParent();
	
		CArmIkSolver* pArmSolver = static_cast<CArmIkSolver*>(m_pPed->GetIkManager().GetSolver(IKManagerSolverTypes::ikSolverTypeArm));
		if (pArmSolver && pAttachParent)
		{
			// Check attachment since solver is run prior to physical attachment (delayed root attachment).
			// Calculate target relative to attachment matrix since character's root may be offset.
			Mat34V mtxAttach(V_IDENTITY);
			CPhysical* pPhysical = pArmSolver->GetAttachParent(mtxAttach);

			if (pPhysical == pAttachParent)
			{
				mtxParent = mtxAttach;
			}
		}

		// Calculate how far each arm is from their respective target positions
		for (int armIndex = 0; armIndex < crIKSolverArms::NUM_ARMS; armIndex++)
		{
			ArmReachSituation& reachSituation = aArmReachSituation[armIndex];

			reachSituation.bValid = pArmSolver && 
									(pArmSolver->GetArmActive((crIKSolverArms::eArms)armIndex) || (pArmSolver->GetArmBlend((crIKSolverArms::eArms)armIndex) > 0.0f)) && 
									(m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsEnteringVehicle) || (pArmSolver->GetTarget((crIKSolverArms::eArms)armIndex) == pAttachParent));

			if (reachSituation.bValid)
			{
				reachSituation.vSourcePosition = skeleton.GetObjectMtx(m_auArmBoneIdx[armIndex][UPPER]).GetCol3();
				reachSituation.vTargetPosition = UnTransformFull(mtxParent, pArmSolver->GetTargetPosition((crIKSolverArms::eArms)armIndex));
				reachSituation.vToTarget = Subtract(reachSituation.vTargetPosition, reachSituation.vSourcePosition);
				reachSituation.vReachDistance = Subtract(Mag(reachSituation.vToTarget), vArmLength);
				reachSituation.fBlend = pArmSolver->GetArmBlend((crIKSolverArms::eArms)armIndex);
				reachSituation.bValid &= (IsGreaterThanAll(reachSituation.vReachDistance, vScalarEps) != 0) && (IsLessThanAll(reachSituation.vReachDistance, vScalarMaxReach) != 0);

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					formatf(debugText, "RcD %s: %6.3f", armIndex == 0 ? "Lt" : "Rt", reachSituation.vReachDistance.Getf());
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				}
			#endif // __IK_DEV
			}
		}

		// Adjust torso so that arms can reach
		if (aArmReachSituation[LEFT_ARM].bValid || aArmReachSituation[RIGHT_ARM].bValid)
		{
			if (aArmReachSituation[LEFT_ARM].bValid && aArmReachSituation[RIGHT_ARM].bValid)
			{
				const float fLtReachDistance = aArmReachSituation[LEFT_ARM].vReachDistance.Getf() * aArmReachSituation[LEFT_ARM].fBlend;
				const float fRtReachDistance = aArmReachSituation[RIGHT_ARM].vReachDistance.Getf() * aArmReachSituation[RIGHT_ARM].fBlend;

				// Calculate weight between arms based on reach distance
				const ScalarV vWeight(fRtReachDistance / (fLtReachDistance + fRtReachDistance));

				ArmReachSituation reachSituation;
				reachSituation.vSourcePosition = Lerp(vWeight, aArmReachSituation[LEFT_ARM].vSourcePosition, aArmReachSituation[RIGHT_ARM].vSourcePosition);
				reachSituation.vTargetPosition = Lerp(vWeight, aArmReachSituation[LEFT_ARM].vTargetPosition, aArmReachSituation[RIGHT_ARM].vTargetPosition);
				reachSituation.vToTarget = Subtract(reachSituation.vTargetPosition, reachSituation.vSourcePosition);
				reachSituation.vReachDistance = Max(aArmReachSituation[LEFT_ARM].vReachDistance, aArmReachSituation[RIGHT_ARM].vReachDistance);
				reachSituation.fBlend = Max(aArmReachSituation[LEFT_ARM].fBlend, aArmReachSituation[RIGHT_ARM].fBlend);

				SolveSpine(skeleton, reachSituation);

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					formatf(debugText, "Wt: %5.3f RcD: %5.3f RcB: %5.3f", vWeight.Getf(), reachSituation.vReachDistance.Getf(), reachSituation.fBlend);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				}
			#endif // __IK_DEV
			}
			else
			{
				// Adjust torso for one arm only
				int armIndex = aArmReachSituation[LEFT_ARM].bValid ? LEFT_ARM : RIGHT_ARM;
				SolveSpine(skeleton, aArmReachSituation[armIndex]);

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					formatf(debugText, "Wt: %5.3f RcD: %5.3f RcB: %5.3f", (float)armIndex, aArmReachSituation[armIndex].vReachDistance.Getf(), aArmReachSituation[armIndex].fBlend);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				}
			#endif // __IK_DEV
			}

			skeleton.PartialUpdate(m_auSpineBoneIdx[SPINE0]);
		}
	}
}

void CTorsoVehicleIkSolver::SolveSpine(crSkeleton& skeleton, const ArmReachSituation& reachSituation)
{
	Mat34V mtxTransform;
	Vec3V vRadius;
	Vec3V vAxis;
	QuatV qRotation;
	ScalarV vTheta;

	const Vec3V vZero(V_ZERO);
	const Vec3V vToTargetNormalized(NormalizeSafe(reachSituation.vToTarget, vZero));
	const ScalarV vScalarZero(V_ZERO);

	for (int spineIndex = SPINE0; spineIndex < NUM_SPINE_PARTS; ++spineIndex)
	{
		const Mat34V& mtxSpine = skeleton.GetObjectMtx(m_auSpineBoneIdx[spineIndex]);
		const ScalarV vSpineWeight(0.1f * (spineIndex + 1) * m_afBlend[SOLVER_ARM_REACH] * reachSituation.fBlend);

		// Calculate rotation axis in object space
		vRadius = Subtract(reachSituation.vSourcePosition, mtxSpine.GetCol3());
		vAxis = Cross(vRadius, vToTargetNormalized);
		vAxis = NormalizeSafe(vAxis, vZero);

		// Approximating arc length s = theta * R => theta = s / R, where s = exceeded reach distance
		vTheta = InvertSafe(Mag(vRadius), vScalarZero);
		vTheta = Scale(vTheta, reachSituation.vReachDistance);
		vTheta = Scale(vTheta, vSpineWeight);

		qRotation = QuatVFromAxisAngle(UnTransform3x3Full(mtxSpine, vAxis), vTheta);
		Mat34VFromQuatV(mtxTransform, qRotation);

		Mat34V& mtxSpineLocal = skeleton.GetLocalMtx(m_auSpineBoneIdx[spineIndex]);
		Transform3x3(mtxSpineLocal, mtxTransform, mtxSpineLocal);
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		const Mat34V& mtxParent = *skeleton.GetParentMtx();
		Vec3V vStart(Transform(mtxParent, reachSituation.vSourcePosition));
		Vec3V vEnd(Transform(mtxParent, reachSituation.vTargetPosition));
		ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_yellow);
	}
#endif
}

void CTorsoVehicleIkSolver::CalculateArmLength()
{
	if (IsValid())
	{
		crSkeleton& skeleton = *m_pPed->GetSkeleton();

		const Mat34V& mtxHand  = skeleton.GetLocalMtx(m_auArmBoneIdx[LEFT_ARM][HAND]);
		const Mat34V& mtxElbow = skeleton.GetLocalMtx(m_auArmBoneIdx[LEFT_ARM][ELBOW]);

		ScalarV vArmLength(Add(Mag(mtxHand.GetCol3()), Mag(mtxElbow.GetCol3())));
		vArmLength = Scale(vArmLength, ScalarV(((CTorsoVehicleIkSolverProxy*)m_pProxy)->m_Request.GetReachLimitPercentage()));
		m_fArmLength = vArmLength.Getf();
	}
}

bool CTorsoVehicleIkSolver::IsValid()
{
	return m_bBonesValid;
}

fwEntity* CTorsoVehicleIkSolver::GetAttachParent() const
{
	fwAttachmentEntityExtension* pExtension = m_pPed->GetAttachmentExtension();

	if (pExtension)
	{
		return pExtension->GetAttachParentForced();
	}

	return NULL;
}

float CTorsoVehicleIkSolver::GetAnimatedLeanAngle() const
{
	// Convert animated lean from [0,1] -> [-1, 1]
	float fAnimatedLean = (m_pProxy->m_Request.GetAnimatedLean() - 0.5f) * 2.0f;
	return fAnimatedLean * ms_Params.afAnimatedLeanLimits[(fAnimatedLean < 0.0f) ? 0 : 1] * DtoR;
}

#if __IK_DEV

void CTorsoVehicleIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

bool CTorsoVehicleIkSolver::CaptureDebugRender()
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

void CTorsoVehicleIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CTorsoVehicleIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorsoVehicle], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");

#if ENABLE_TORSOVEHICLE_SOLVER_WIDGETS
		pBank->PushGroup("Tunables", false);
			pBank->AddSlider("Blend Rate (Solver Lean)", &ms_Params.afBlendRate[SOLVER_LEAN], 0.0f, 10.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Blend Rate (Solver ArmReach)", &ms_Params.afBlendRate[SOLVER_ARM_REACH], 0.0f, 10.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Delta Rate", &ms_Params.fDeltaRate, 0.0f, 100.0f, 1.0f, NullCB, "");
			pBank->PushGroup("Animated Lean Limits", false);
				pBank->AddSlider("Forward", &ms_Params.afAnimatedLeanLimits[1], -180.0f, 180.0f, 1.0f, NullCB, "");
				pBank->AddSlider("Back", &ms_Params.afAnimatedLeanLimits[0], -180.0f, 180.0f, 1.0f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();

		char szLabel[16];
		pBank->PushGroup("Delta Limits (Deg)", false);
			for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
			{
				sprintf(szLabel, "Spine %d", spineBoneIndex);
				pBank->PushGroup(szLabel, false);
					pBank->AddSlider("Yaw Min", &ms_Params.avSpineDeltaLimits[spineBoneIndex][0][0], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Yaw Max", &ms_Params.avSpineDeltaLimits[spineBoneIndex][1][0], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Roll Min", &ms_Params.avSpineDeltaLimits[spineBoneIndex][0][1], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Roll Max", &ms_Params.avSpineDeltaLimits[spineBoneIndex][1][1], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Pitch Min", &ms_Params.avSpineDeltaLimits[spineBoneIndex][0][2], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Pitch Max", &ms_Params.avSpineDeltaLimits[spineBoneIndex][1][2], -180.0f, 180.0f, 1.0f);
				pBank->PopGroup();
			}
		pBank->PopGroup();

		pBank->PushGroup("Joint Limits (Deg)", false);
			for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
			{
				sprintf(szLabel, "Spine %d", spineBoneIndex);
				pBank->PushGroup(szLabel, false);
					pBank->AddSlider("Yaw Min", &ms_Params.avSpineJointLimits[spineBoneIndex][0][0], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Yaw Max", &ms_Params.avSpineJointLimits[spineBoneIndex][1][0], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Roll Min", &ms_Params.avSpineJointLimits[spineBoneIndex][0][1], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Roll Max", &ms_Params.avSpineJointLimits[spineBoneIndex][1][1], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Pitch Min", &ms_Params.avSpineJointLimits[spineBoneIndex][0][2], -180.0f, 180.0f, 1.0f);
					pBank->AddSlider("Pitch Max", &ms_Params.avSpineJointLimits[spineBoneIndex][1][2], -180.0f, 180.0f, 1.0f);
				pBank->PopGroup();
			}
		pBank->PopGroup();

		pBank->PushGroup("Spine Weights", false);
			for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
			{
				sprintf(szLabel, "Spine %d", spineBoneIndex);
				pBank->AddSlider(szLabel, &ms_Params.afSpineWeights[spineBoneIndex], 0.0f, 1.0f, 0.01f);
			}
		pBank->PopGroup();
#endif // ENABLE_TORSOVEHICLE_SOLVER_WIDGETS

	pBank->PopGroup();
}

#endif //__IK_DEV
