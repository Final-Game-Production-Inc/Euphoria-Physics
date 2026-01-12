// 
// ik/solvers/RootSlopeFixupSolver.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
// 

#include "RootSlopeFixupSolver.h"

#include "diag/art_channel.h"
#include "fwanimation/directorcomponentcreature.h"
#include "fwdebug/picker.h"
#include "vectormath/classfreefuncsv.h"

#if __IK_DEV
#include "animation/debug/AnimViewer.h"
#endif
#include "animation/anim_channel.h"
#include "animation/MovePed.h"
#include "ik/IkManager.h"
#include "peds/Ped.h"

#define ROOT_SLOPE_FIXUP_SOLVER_POOL_MAX 32

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CRootSlopeFixupIkSolver, ROOT_SLOPE_FIXUP_SOLVER_POOL_MAX, 0.45f, atHashString("CRootSlopeFixupIkSolver",0x4380bd1e));

ANIM_OPTIMISATIONS();

#if __IK_DEV
CDebugRenderer CRootSlopeFixupIkSolver::ms_debugHelper;
bool CRootSlopeFixupIkSolver::ms_bRenderDebug = false;
int CRootSlopeFixupIkSolver::ms_iNoOfTexts = 0;
float CRootSlopeFixupIkSolver::ms_fDebugPitchSlope = 0.0f;
float CRootSlopeFixupIkSolver::ms_fDebugRollSlope = 0.0f;
float CRootSlopeFixupIkSolver::ms_fDebugBlend = -1.0f;
float CRootSlopeFixupIkSolver::ms_fDebugBoundHeightOffset = 0.0f;
#endif

float CRootSlopeFixupIkSolver::ms_fMaximumFixupPitchAngle = 43.0f * DtoR;
float CRootSlopeFixupIkSolver::ms_fMaximumFixupRollAngle = 43.0f * DtoR;
float CRootSlopeFixupIkSolver::ms_fMaximumProbeHeightDifference = 0.6f;
float CRootSlopeFixupIkSolver::ms_fMaximumDistanceBeforeNextProbe = 0.25f;
float CRootSlopeFixupIkSolver::ms_fGroundNormalApproachRate = REALLY_SLOW_BLEND_IN_DELTA;
float CRootSlopeFixupIkSolver::ms_fMaximumProbeWaitTime = 0.1f;
float CRootSlopeFixupIkSolver::ms_fCapsuleProbeRadius = 0.15f;
float CRootSlopeFixupIkSolver::ms_fMaximumFixupPitchDifference = 10.0f * DtoR;

const Vec3V vUpAxis = Vec3V(V_UP_AXIS_WZERO);

eAnimBoneTag eBoneTag[CRootSlopeFixupIkSolver::NUM_PROBE] = { BONETAG_L_CLAVICLE, BONETAG_R_THIGH, BONETAG_L_CALF, BONETAG_R_CLAVICLE, BONETAG_L_THIGH, BONETAG_R_CALF };
CRootSlopeFixupIkSolver::eProbe eProbeSetStart[CRootSlopeFixupIkSolver::NUM_PROBESET] = { CRootSlopeFixupIkSolver::PROBE_SET_A_START, CRootSlopeFixupIkSolver::PROBE_SET_B_START };
CRootSlopeFixupIkSolver::eProbe eProbeSetEnd[CRootSlopeFixupIkSolver::NUM_PROBESET] = { CRootSlopeFixupIkSolver::PROBE_SET_A_END, CRootSlopeFixupIkSolver::PROBE_SET_B_END };

CRootSlopeFixupIkSolver::CRootSlopeFixupIkSolver(CPed *pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM),
m_pPed(pPed),
m_fBlend(0.0f),
m_fBlendRate(0.0f),
#if FPS_MODE_SUPPORTED
m_xHeadTransformMatrix(V_IDENTITY),
m_xSpine3TransformMatrix(V_IDENTITY),
#endif
m_vGroundNormal(V_ZERO),
m_vDesiredGroundNormal(V_Z_AXIS_WZERO),
m_vLastSlopeCalculationPosition(V_ZERO),
m_bBonesValid(true),
m_bRemoveSolver(false),
m_bForceMainThread(false)
{
	SetPrority(3);
	Reset();

	m_uRootBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	for (int iBone = 0; iBone < NUM_PROBE && m_bBonesValid; iBone++)
	{
		m_uBoneIndex[iBone] = m_pPed->GetBoneIndexFromBoneTag(eBoneTag[iBone]);
		if (!artVerifyf(m_uBoneIndex[iBone] != -1, "Modelname = (%s) is missing bone %s necessary for root slope fixup ik.",
			pPed != NULL ? pPed->GetModelName() : "Unknown model", crSkeletonData::DebugConvertBoneIdToName(static_cast<u16>(eBoneTag[iBone]))))
		{
			m_bBonesValid = false;
			break;
		}
	}

#if FPS_MODE_SUPPORTED
	m_uHeadBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);
	m_uSpine3BoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE3);
#endif
}

CRootSlopeFixupIkSolver::~CRootSlopeFixupIkSolver()
{
	Reset();
}

void CRootSlopeFixupIkSolver::PreIkUpdate(float deltaTime)
{
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeRootSlopeFixup] || CBaseIkManager::ms_DisableAllSolvers)
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
		sprintf(debugText, "%s\n", "CRootSlopeFixupIkSolver::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(deltaTime);

	m_bForceMainThread = false;

	// If full leg IK mode solver is running, then slope fixup needs to be applied on the main thread
	// before the leg IK solver so that the feet are in the correct position for the leg IK probes.
	if (m_pPed->GetIkManager().HasSolver(IKManagerSolverTypes::ikSolverTypeLeg))
	{
		u32 uMode = m_pPed->m_PedConfigFlags.GetPedLegIkMode();

		if ((uMode == LEG_IK_MODE_FULL) || (uMode == LEG_IK_MODE_FULL_MELEE))
		{
			m_bForceMainThread = true;
		}
	}

	if (m_bForceMainThread)
	{
		crSkeleton* pSkeleton = m_pPed->GetSkeleton();

		if (pSkeleton)
		{
			SolveRoot(*pSkeleton);
		}
	}
}

void CRootSlopeFixupIkSolver::Iterate(float UNUSED_PARAM(dt), crSkeleton& skeleton)
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeRootSlopeFixup] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

	if (!m_bForceMainThread)
	{
		SolveRoot(skeleton);
	}
}

bool CRootSlopeFixupIkSolver::IsDead() const
{
	return m_bRemoveSolver;
}

void CRootSlopeFixupIkSolver::Update(float deltaTime)
{
	if (!IsValid())
	{
		m_bRemoveSolver = true;
		return;
	}

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || (m_pPed->GetUsingRagdoll() && m_fBlend > 0.0f) || m_pPed->GetDisableRootSlopeFixupSolver() || m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle) || (pSkeleton == NULL))
	{
		// If we go into ragdoll reset the solver
		if ((m_pPed->GetUsingRagdoll() && m_fBlend > 0.0f) || m_pPed->GetDisableRootSlopeFixupSolver() || m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_InVehicle))
		{
			Reset();
			m_bRemoveSolver = true;
		}

		return;
	}

	if (deltaTime > 0.0f)
	{
		UpdateProbeRequest();
	}

	m_fBlend = Clamp(m_fBlend + m_fBlendRate * deltaTime, 0.0f, 1.0f);

	Mat34V xRootMatrix;
	pSkeleton->GetGlobalMtx(m_uRootBoneIndex, xRootMatrix);

	if (deltaTime > 0.0f)
	{
		if (DistSquared(xRootMatrix.d(), m_vLastSlopeCalculationPosition).Getf() >= square(ms_fMaximumDistanceBeforeNextProbe))
		{
			RequestProbe();
		}
	}

	Vector3 vUnitAxis;
	float fDesiredAngle;
	if (!IsZeroAll(m_vGroundNormal) && ComputeRotation(RCC_VECTOR3(m_vGroundNormal), RCC_VECTOR3(m_vDesiredGroundNormal), vUnitAxis, fDesiredAngle))
	{
		float fAngle = 0.0f;
		if (!Approach(fAngle, fDesiredAngle, ms_fGroundNormalApproachRate, deltaTime))
		{
			Mat33V xUnitAxis;
			Mat33VFromAxisAngle(xUnitAxis, RCC_VEC3V(vUnitAxis), ScalarV(fAngle));
			m_vGroundNormal = Multiply(xUnitAxis, m_vGroundNormal);
		}
	}
	else
	{
		m_vGroundNormal = m_vDesiredGroundNormal;
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];

		ScalarV fPitch(V_ZERO);
		ScalarV fRoll(V_ZERO);

		Mat33V xRootHeadingMatrix(xRootMatrix.GetMat33ConstRef());

		// We want a root matrix with only z rotation
		if(ComputeRotation(RCC_VECTOR3(xRootMatrix.GetCol2ConstRef()), g_UnitUp, vUnitAxis, fDesiredAngle))
		{
			Mat33VFromAxisAngle(xRootHeadingMatrix, RCC_VEC3V(vUnitAxis), ScalarV(fDesiredAngle));
			Multiply(xRootHeadingMatrix, xRootHeadingMatrix, xRootMatrix.GetMat33ConstRef());
		}

		CalculatePitchAndRollFromGroundNormal(fPitch, fRoll, xRootHeadingMatrix);

		sprintf(debugText, "Blend: %.4f, Pitch: %.4f, Roll: %.4f", GetBlend(), fPitch.Getf(), fRoll.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		ms_debugHelper.Line(RCC_VECTOR3(m_vLastSlopeCalculationPosition), VEC3V_TO_VECTOR3(AddScaled(m_vLastSlopeCalculationPosition, m_vGroundNormal, ScalarV(V_TWO))), Color_green);
		ms_debugHelper.Line(RCC_VECTOR3(m_vLastSlopeCalculationPosition), VEC3V_TO_VECTOR3(AddScaled(m_vLastSlopeCalculationPosition, m_vDesiredGroundNormal, ScalarV(V_TWO))), Color_DarkGreen);
	}
#endif //__IK_DEV
}

float CRootSlopeFixupIkSolver::GetBlend() const
{
#if __IK_DEV
	if (ms_fDebugBlend >= 0.0f)
	{
		return ms_fDebugBlend;
	}
#endif

	return m_fBlend;
}

float CRootSlopeFixupIkSolver::GetBlendRate() const
{
	return m_fBlendRate;
}

bool CRootSlopeFixupIkSolver::CalculatePitchAndRollFromGroundNormal(ScalarV& fPitch, ScalarV& fRoll, const Mat33V& xParentMatrix) const
{
	bool bValid = false;
	// Ensure that we actually have a slope...
	if(IsLessThanAll(Abs(Dot(m_vGroundNormal, vUpAxis)), ScalarV(0.99f)))
	{
		// Break out pitch and roll separately to make it easier to tune the amount that each gets applied
		Vec3V vSlopePitch = Cross(m_vGroundNormal, xParentMatrix.a());
		if(!IsZeroAll(vSlopePitch))
		{
			vSlopePitch = Normalize(vSlopePitch);
			fPitch = Arctan2(vSlopePitch.GetZ(), MagXY(vSlopePitch));
			fPitch = Clamp(fPitch, ScalarV(-ms_fMaximumFixupPitchAngle), ScalarV(ms_fMaximumFixupPitchAngle));

			bValid = true;
		}

		Vec3V vSlopeRoll = Cross(m_vGroundNormal, xParentMatrix.b());
		if(!IsZeroAll(vSlopeRoll))
		{
			vSlopeRoll = Normalize(vSlopeRoll);
			fRoll = Arctan2(vSlopeRoll.GetZ(), MagXY(vSlopeRoll));
			fRoll = Clamp(fRoll, ScalarV(-ms_fMaximumFixupRollAngle), ScalarV(ms_fMaximumFixupRollAngle));

			bValid = true;
		}
	}

#if __IK_DEV
	if(ms_fDebugPitchSlope != 0.0f)
	{
		fPitch.Setf(ms_fDebugPitchSlope);
		bValid = true;
	}

	if(ms_fDebugRollSlope != 0.0f)
	{
		fRoll.Setf(ms_fDebugRollSlope);
		bValid = true;
	}
#endif

	return bValid;
}

void CRootSlopeFixupIkSolver::Apply(rage::crSkeleton& skeleton)
{
	ScalarV fPitch(V_ZERO);
	ScalarV fRoll(V_ZERO);

	Mat34V xRootMatrix;
	skeleton.GetGlobalMtx(m_uRootBoneIndex, xRootMatrix);

	Vector3 vUnitAxis;
	float fAngle;

	Mat33V xRootHeadingMatrix(xRootMatrix.GetMat33ConstRef());

	// We want a root matrix with only z rotation
	if(ComputeRotation(RCC_VECTOR3(xRootMatrix.GetCol2ConstRef()), g_UnitUp, vUnitAxis, fAngle))
	{
		Mat33VFromAxisAngle(xRootHeadingMatrix, RCC_VEC3V(vUnitAxis), ScalarV(fAngle));
		Multiply(xRootHeadingMatrix, xRootHeadingMatrix, xRootMatrix.GetMat33ConstRef());
	}

	if(CalculatePitchAndRollFromGroundNormal(fPitch, fRoll, xRootHeadingMatrix))
	{
		float fBlend = GetBlend();

		// Want to rotate an idealized matrix (a matrix with only heading rotation) and then apply that rotation to the current root matrix
		Mat33V xPitchMatrix;
		Mat33VFromAxisAngle(xPitchMatrix, xRootHeadingMatrix.a(), fPitch * ScalarV(fBlend));
		Mat33V xRollMatrix;
		Mat33VFromAxisAngle(xRollMatrix, xRootHeadingMatrix.b(), fRoll * ScalarV(fBlend));

		Mat33V xTransformMatrix;
		Multiply(xTransformMatrix, xPitchMatrix, xRollMatrix);
		Multiply(xRootMatrix.GetMat33Ref(), xTransformMatrix, xRootMatrix.GetMat33ConstRef());

		UnTransformOrtho(skeleton.GetLocalMtx(m_uRootBoneIndex), *skeleton.GetParentMtx(), xRootMatrix);
		
#if FPS_MODE_SUPPORTED
		m_xHeadTransformMatrix = skeleton.GetObjectMtx(m_uHeadBoneIndex);
		m_xSpine3TransformMatrix = skeleton.GetObjectMtx(m_uSpine3BoneIndex);
#endif
		
		skeleton.Update();

#if FPS_MODE_SUPPORTED
		UnTransformOrtho(m_xHeadTransformMatrix, m_xHeadTransformMatrix, skeleton.GetObjectMtx(m_uHeadBoneIndex));
		UnTransformOrtho(m_xSpine3TransformMatrix, m_xSpine3TransformMatrix, skeleton.GetObjectMtx(m_uSpine3BoneIndex));
#endif
	}
}

void CRootSlopeFixupIkSolver::SolveRoot(crSkeleton& skeleton)
{
	if (IsActive())
	{
		if (!IsBlocked())
		{
			Apply(skeleton);
		}
	}
	else
	{
		Reset();
		m_bRemoveSolver = true;
	}
}

void CRootSlopeFixupIkSolver::Reset()
{
	m_fBlend = 0.0f;
	m_fBlendRate = INSTANT_BLEND_OUT_DELTA;
	m_bRemoveSolver = false;
}

void CRootSlopeFixupIkSolver::PostIkUpdate()
{
}

void CRootSlopeFixupIkSolver::AddFixup(float fBlendRate)
{
	// Special case: allow the root slope fixup Ik solver to be added if we aren't yet blending it in!
	if(m_pPed->GetUsingRagdoll() && fBlendRate != 0.0f)
	{
		return;
	}

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();
	if (pSkeleton == NULL)
	{
		return;
	}

	Mat34V xRootMatrix;
	pSkeleton->GetGlobalMtx(m_uRootBoneIndex, xRootMatrix);
	if (DistSquared(xRootMatrix.d(), m_vLastSlopeCalculationPosition).Getf() >= square(ms_fMaximumDistanceBeforeNextProbe))
	{
		RequestProbe();
	}

	m_fBlendRate = fBlendRate;
}

Vec3V_Out CRootSlopeFixupIkSolver::CalcPlaneNormal(Vec3V_In vPoint1, Vec3V_In vPoint2, Vec3V_In vPoint3) const
{
	Vec3V vVector1 = Subtract(vPoint1, vPoint2);
	Vec3V vVector2 = Subtract(vPoint2, vPoint3);
	Vec3V vPlaneNormal = NormalizeSafe(Cross(vVector1, vVector2), Vec3V(V_UP_AXIS_WZERO));

	// Always want our plane normal point upwards in the z-axis
	if(vPlaneNormal.GetZf() < 0.0f)
	{
		vPlaneNormal = Negate(vPlaneNormal);
	}

	return vPlaneNormal;
}

void CRootSlopeFixupIkSolver::ResetProbe()
{
	for (int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
	{
		m_ProbeResults[iProbe].Reset();
	}
}

void CRootSlopeFixupIkSolver::RequestProbe()
{
	// No point in waiting for an old probe to complete - just kill it and start a new probe
	ResetProbe();

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	if(!ikVerifyf(pSkeleton != NULL, "CRootSlopeFixupIkSolver::RequestProbe: Invalid skeleton"))
	{
		return;
	}

	Vec3V vGlobalPosition[NUM_PROBE];
	Mat34V_ConstRef xParentMatrix = *pSkeleton->GetParentMtx();
	for (int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
	{
		vGlobalPosition[iProbe] = Transform(xParentMatrix, pSkeleton->GetObjectMtx(m_uBoneIndex[iProbe]).d());
	}

	// Only set the last slope calculation position when initially requesting the probe!
	m_vLastSlopeCalculationPosition = Transform(xParentMatrix, pSkeleton->GetObjectMtx(m_uRootBoneIndex).d());

	// Since the clavicles and root of the character will be lying further from the ped's actual position we need to account for big slopes and probe higher and lower than for the feet
	static const ScalarV fProbeUpZOffset[NUM_PROBE] = { ScalarV(2.0f), ScalarV(1.5f), ScalarV(1.0f), ScalarV(2.0f), ScalarV(1.5f), ScalarV(1.0f) };
	static const ScalarV fProbeDownZOffset[NUM_PROBE] = { ScalarV(3.0f), ScalarV(2.0f), ScalarV(1.5f), ScalarV(3.0f), ScalarV(2.0f), ScalarV(1.5f) };

#if __IK_DEV
	static const u32 uKeyStart = ATSTRINGHASH("probe_line", 0x33868FFC);
	if (CaptureDebugRender())
	{
		for(int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
		{
			CPhysics::ms_debugDrawStore.Clear(uKeyStart + iProbe);
		}
	}
#endif

	WorldProbe::CShapeTestCapsuleDesc probeDesc;
	probeDesc.SetIncludeFlags(ArchetypeFlags::GTA_ALL_TYPES_MOVER);
	probeDesc.SetExcludeEntity(m_pPed, WorldProbe::EIEO_DONT_ADD_VEHICLE | WorldProbe::EIEO_DONT_ADD_VEHICLE_OCCUPANTS);
	probeDesc.SetTypeFlags(ArchetypeFlags::GTA_AI_TEST);
	probeDesc.SetIsDirected(true);
	
	for(int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
	{
		probeDesc.SetResultsStructure(&m_ProbeResults[iProbe]);
		probeDesc.SetCapsule(VEC3V_TO_VECTOR3(Vec3V(vGlobalPosition[iProbe].GetX(), vGlobalPosition[iProbe].GetY(), Add(vGlobalPosition[iProbe].GetZ(), fProbeUpZOffset[iProbe]))), 
			VEC3V_TO_VECTOR3(Vec3V(vGlobalPosition[iProbe].GetX(), vGlobalPosition[iProbe].GetY(), Subtract(vGlobalPosition[iProbe].GetZ(), fProbeDownZOffset[iProbe]))), ms_fCapsuleProbeRadius);

#if __IK_DEV
		if(CaptureDebugRender())
		{
			CPhysics::ms_debugDrawStore.AddCapsule(VECTOR3_TO_VEC3V(probeDesc.GetStart()), VECTOR3_TO_VEC3V(probeDesc.GetEnd()), ms_fCapsuleProbeRadius, iProbe < PROBE_SET_A_END ? Color_white : Color_yellow, 10000, uKeyStart + iProbe, false);
		}
#endif

		WorldProbe::GetShapeTestManager()->SubmitTest(probeDesc, WorldProbe::PERFORM_ASYNCHRONOUS_TEST);
	}
}

void CRootSlopeFixupIkSolver::UpdateProbeRequest()
{
	bool bAllProbeResultsReady = true;

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();
	if(ikVerifyf(pSkeleton != NULL, "CRootSlopeFixupIkSolver::UpdateProbeRequest: Invalid skeleton"))
	{
		Mat34V_ConstRef xParentMatrix = *pSkeleton->GetParentMtx();

		for (int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
		{
			if (!m_ProbeResults[iProbe].GetResultsReady())
			{
				bAllProbeResultsReady = false;
				break;
			}
			else
			{
				Vec3V vGlobalPosition = Transform(xParentMatrix, pSkeleton->GetObjectMtx(m_uBoneIndex[iProbe]).d());
				for (int iIntersection = 0; iIntersection < m_ProbeResults[iProbe].GetSize(); iIntersection++)
				{
					// Reject hits with anything but map collision for now (map collision is anything that is tagged as mapped collision but that doesn't itself
					// collide with map collision)...
					// Do this now - as the probe results come in - rather than waiting for all probes to be ready. The instance might be deleted from the level by
					// the time all the probes are ready and we'd have lost the opportunity to inspect the type and include flags
					phInst* pInstance = m_ProbeResults[iProbe][iIntersection].GetHitInst();
					if (pInstance != NULL && (!CPhysics::GetLevel()->GetInstanceTypeFlag(pInstance->GetLevelIndex(), ArchetypeFlags::GTA_ALL_MAP_TYPES) ||
						CPhysics::GetLevel()->GetInstanceIncludeFlag(pInstance->GetLevelIndex(), ArchetypeFlags::GTA_ALL_MAP_TYPES)))
					{
						// If the rejected hit is above our bone position then only reject the single hit - otherwise reject the entire probe
						// This will stop vehicles, object, etc. that are above the ped from being considered when calculating slope but will still allow
						// vehicles, objects, etc. that are below the ped to stop a slope from being calculated...
						if (m_ProbeResults[iProbe][iIntersection].GetHitPosition().z > vGlobalPosition.GetZf())
						{
							m_ProbeResults[iProbe][iIntersection].Reset();
						}
						else
						{
							m_ProbeResults[iProbe].Reset();
							break;
						}
					}
					else
					{
						// If we've found a valid intersection move it to the first element
						m_ProbeResults[iProbe][0] = m_ProbeResults[iProbe][iIntersection];
						break;
					}
				}
			}
		}
	}

	if (bAllProbeResultsReady)
	{
		CalcSlope();
		ResetProbe();
	}
}

void CRootSlopeFixupIkSolver::CalcSlope()
{
	const int NUM_BODYSET = 2;

	const Vec3V vUpAxis(V_UP_AXIS_WZERO);
	const Vec3V vZero(V_ZERO);

#if __IK_DEV
	static const u32 uKeyHitPointStart = ATSTRINGHASH("probe_hit_point", 0xEE85E4A8);
	static const u32 uKeyPlaneStart = ATSTRINGHASH("probe_hit_plane", 0xFA733E55);
	
	if (CaptureDebugRender())
	{
		for (int iProbe = 0; iProbe < NUM_PROBE; iProbe++)
		{
			CPhysics::ms_debugDrawStore.Clear(uKeyHitPointStart + iProbe);
		}

		for (int iProbeSet = 0; iProbeSet < NUM_PROBESET; iProbeSet++)
		{
			CPhysics::ms_debugDrawStore.Clear(uKeyPlaneStart + iProbeSet);
		}

		for (int iBodySet = 0; iBodySet < NUM_BODYSET; iBodySet++)
		{
			CPhysics::ms_debugDrawStore.Clear(uKeyPlaneStart + NUM_PROBESET + iBodySet);
		}
	}
#endif

	m_vDesiredGroundNormal = vZero;

	// Try to calculate a slope from the available upper and lower body probes
	static const eProbe aeBodyProbes[NUM_BODYSET][4] =
	{
		{ PROBE_CLAVICLE_R, PROBE_CLAVICLE_L, PROBE_THIGH_L, PROBE_THIGH_R },	// Upper body
		{ PROBE_CALF_R, PROBE_CALF_L, PROBE_THIGH_R, PROBE_THIGH_L }			// Lower body
	};

	Vec3V avHitPositions[3];
	int iNbPlaneNormals = 0;
	ScalarV fLastBodySetSlope(0.0f);

	for (int iBodySet = 0; iBodySet < 2; iBodySet++)
	{
		int iValidPositionCount = 0;

		for (int iProbe = 0; iProbe < 4 && iValidPositionCount < 3; iProbe++)
		{
			const WorldProbe::CShapeTestHitPoint& hitPoint = m_ProbeResults[aeBodyProbes[iBodySet][iProbe]][0];

			if (hitPoint.IsAHit())
			{
				avHitPositions[iValidPositionCount] = hitPoint.GetHitPositionV();
				iValidPositionCount++;
			}
		}

		if (iValidPositionCount == 3)
		{
			Vec3V vPlaneNormal = CalcPlaneNormal(avHitPositions[0], avHitPositions[1], avHitPositions[2]);
			ScalarV fMagXY(MagXY(vPlaneNormal));
			ScalarV fBodySetSlope(!IsZeroAll(fMagXY) ? Arctan2(vPlaneNormal.GetZ(), fMagXY) : ScalarV(V_PI_OVER_TWO));

			// If the difference between the slope calculated for this particular plane is too far off from
			// the last plane then don't use this plane
			if (iNbPlaneNormals == 0 || Abs(Subtract(fBodySetSlope, fLastBodySetSlope)).Getf() <= ms_fMaximumFixupPitchDifference)
			{
				m_vDesiredGroundNormal += vPlaneNormal;
#if __IK_DEV
				if (CaptureDebugRender())
				{
					CPhysics::ms_debugDrawStore.AddPoly(avHitPositions[0], avHitPositions[1], avHitPositions[2], Color_cyan4, 10000, uKeyPlaneStart + NUM_PROBESET + iBodySet);
				}
#endif
				iNbPlaneNormals++;
			}

			fLastBodySetSlope = fBodySetSlope;
		}
	}

	// Only use the probes if we found valid probes for more than one body set
	if (iNbPlaneNormals > 1)
	{
		m_vDesiredGroundNormal = NormalizeSafe(InvScale(m_vDesiredGroundNormal, ScalarV((float)iNbPlaneNormals)), vUpAxis);
	}
	else
	{
		// By default the ground normal is just the z-axis
		m_vDesiredGroundNormal = vUpAxis;
	}
}

#if __IK_DEV
void CRootSlopeFixupIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

bool CRootSlopeFixupIkSolver::CaptureDebugRender()
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

void CRootSlopeFixupIkSolver::AddRootSlopeFixupCB()
{
	CEntity* pEntity = CDebugScene::FocusEntities_Get(0);
	if (pEntity != NULL && pEntity->GetIsTypePed())
	{
		static_cast<CPed*>(pEntity)->GetIkManager().AddRootSlopeFixup(SLOW_BLEND_IN_DELTA);
	}
}

void CRootSlopeFixupIkSolver::RemoveRootSlopeFixupCB()
{
	CEntity* pEntity = CDebugScene::FocusEntities_Get(0);
	if (pEntity != NULL && pEntity->GetIsTypePed())
	{
		CRootSlopeFixupIkSolver* pSolver = static_cast<CRootSlopeFixupIkSolver*>(static_cast<CPed*>(pEntity)->GetIkManager().GetSolver(IKManagerSolverTypes::ikSolverTypeRootSlopeFixup));
		if (pSolver != NULL)
		{
			pSolver->SetBlendRate(SLOW_BLEND_OUT_DELTA);
		}
	}
}

void CRootSlopeFixupIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CRootSlopeFixupIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeRootSlopeFixup], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");

		pBank->AddSlider("Maximum Pitch Fixup Angle", &ms_fMaximumFixupPitchAngle, -PI, PI, 0.01f);
		pBank->AddSlider("Maximum Roll Fixup Angle", &ms_fMaximumFixupRollAngle, -PI, PI, 0.01f);
		pBank->AddSlider("Maximum Probe Height Difference", &ms_fMaximumProbeHeightDifference, 0.0f, 10.0f, 0.01f);
		pBank->AddSlider("Maximum Distance Before Next Probe", &ms_fMaximumDistanceBeforeNextProbe, 0.0f, 10.0f, 0.01f);
		pBank->AddSlider("Ground Normal Approach Rate", &ms_fGroundNormalApproachRate, 0.0f, 20.0f, 0.1f);
		pBank->AddSlider("Maximum Probe Wait Time", &ms_fMaximumProbeWaitTime, 0.0f, 5.0f, 0.01f);

		pBank->AddSeparator();
		pBank->AddSlider("Debug Pitch Slope", &ms_fDebugPitchSlope, -PI, PI, 0.01f);
		pBank->AddSlider("Debug Roll Slope", &ms_fDebugRollSlope, -PI, PI, 0.01f);
		pBank->AddSlider("Debug Blend", &ms_fDebugBlend, -1.0f, 1.0f, 0.01f);
		pBank->AddButton("Add Root Slope Fixup", CRootSlopeFixupIkSolver::AddRootSlopeFixupCB);
		pBank->AddButton("Remove Root Slope Fixup", CRootSlopeFixupIkSolver::RemoveRootSlopeFixupCB);
	pBank->PopGroup();
}
#endif //__IK_DEV

bool CRootSlopeFixupIkSolver::IsValid()
{
	return m_bBonesValid;
}

bool CRootSlopeFixupIkSolver::IsActive() const
{
	return m_fBlend > 0.0f || m_fBlendRate >= 0.0f;
}

bool CRootSlopeFixupIkSolver::IsBlocked()
{
	return m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK) || 
		   m_pPed->GetMovePed().GetSecondaryBlendHelper().IsFlagSet(APF_BLOCK_IK) ||
		   m_fBlend == 0.0f;
}
