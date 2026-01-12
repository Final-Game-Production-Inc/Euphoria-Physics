// 
// ik/solvers/BodyRecoilSolver.cpp
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#include "BodyRecoilSolver.h"

#include "diag/art_channel.h"
#include "fwdebug/picker.h"

#include "ik/IkRequest.h"
#include "ik/solvers/ArmIkSolver.h"
#include "Peds/ped.h"
#include "vectormath/classfreefuncsv.h"

#define BODYRECOIL_SOLVER_POOL_MAX			16
#define ENABLE_BODYRECOIL_SOLVER_WIDGETS	0

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CBodyRecoilIkSolver, BODYRECOIL_SOLVER_POOL_MAX, 0.61f, atHashString("CBodyRecoilIkSolver",0xf8ccee67));

ANIM_OPTIMISATIONS();

CBodyRecoilIkSolver::Params CBodyRecoilIkSolver::ms_Params =
{
	{
		{																				// Single Shot
			{ Vec4V(0.7f, 0.8f, 0.74f, 0.64f), Vec4V(0.60f, 0.60f, 0.36f, 0.24f) },		// [Tran|Rot|Head|Pelvis][In|Out]
			{ 0.3f, 0.3f },																// afSpineStrength[2]	// [Yaw|Pitch]
			0.20f,																		// fClavicleStrength
			0.15f,																		// fRootStrength
			0.30f																		// fPelvisStrength
		},
		{																				// Auto
			{ Vec4V(1.0f, 1.0f, 0.6f, 0.44f), Vec4V(0.64f, 0.64f, 0.32f, 0.32f) },		// [Tran|Rot|Head|Pelvis][In|Out]
			{ 0.6f, 0.6f },																// afSpineStrength[2]	// [Yaw|Pitch]
			0.50f,																		// fClavicleStrength
			0.18f,																		// fRootStrength
			0.50f																		// fPelvisStrength
		},
	},																					// aWeaponParams[2]

	Vec4V(0.1f, 0.2f, 0.3f, 0.4f),														// vSpineScale

#if __IK_DEV
	false,																				// bRenderTarget
#endif // __IK_DEV
};

#if __IK_DEV
CDebugRenderer CBodyRecoilIkSolver::ms_debugHelper;
bool CBodyRecoilIkSolver::ms_bRenderDebug = false;
int CBodyRecoilIkSolver::ms_iNoOfTexts = 0;
#endif //__IK_DEV

CBodyRecoilIkSolver::CBodyRecoilIkSolver(CPed *pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pPed(pPed)
, m_uFlags(0)
{
	SetPrority(3);
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
		{ BONETAG_L_CLAVICLE, BONETAG_L_UPPERARM, BONETAG_L_IK_HAND },
		{ BONETAG_R_CLAVICLE, BONETAG_R_UPPERARM, BONETAG_R_IK_HAND }
	};
	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		for (int armPartIndex = 0; armPartIndex < NUM_ARM_PARTS; ++armPartIndex)
		{
			m_auArmBoneIdx[armIndex][armPartIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeArmBoneTags[armIndex][armPartIndex]);
			bBonesValid &= (m_auArmBoneIdx[armIndex][armPartIndex] != (u16)-1);
		}
	}

	static const eAnimBoneTag aeLegBoneTags[NUM_LEGS][NUM_LEG_PARTS] = 
	{ 
		{ BONETAG_L_THIGH, BONETAG_L_CALF, BONETAG_L_FOOT, BONETAG_L_TOE },
		{ BONETAG_R_THIGH, BONETAG_R_CALF, BONETAG_R_FOOT, BONETAG_R_TOE }
	};
	for (int legIndex = 0; legIndex < NUM_LEGS; ++legIndex)
	{
		for (int legPartIndex = 0; legPartIndex < NUM_LEG_PARTS; ++legPartIndex)
		{
			m_auLegBoneIdx[legIndex][legPartIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeLegBoneTags[legIndex][legPartIndex]);
			bBonesValid &= (m_auLegBoneIdx[legIndex][legPartIndex] != (u16)-1);
		}
	}

	m_uRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	m_uPelvisBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_PELVIS);
	m_uSpineRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE_ROOT);
	m_uNeckBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);

	bBonesValid &= (m_uPelvisBoneIdx != (u16)-1) && (m_uRootBoneIdx != (u16)-1) && (m_uNeckBoneIdx != (u16)-1) && (m_uSpineRootBoneIdx != (u16)-1);

	m_bBonesValid = bBonesValid;

	artAssertf(bBonesValid, "Modelname = (%s) is missing bones necessary for body recoil ik. Expects the following bones: "
		"BONETAG_SPINE0-3/ROOT, BONETAG_[L/R]_UPPERARM, BONETAG_[L/R]_IK_HAND, BONETAG_[L/R]_THIGH, "
		"BONETAG_[L/R]_CALF, BONETAG_[L/R]_FOOT, BONETAG_[L/R]_TOE", pPed ? pPed->GetModelName() : "Unknown model");
}

CBodyRecoilIkSolver::~CBodyRecoilIkSolver()
{
	ResetArm();
	Reset();
}

void CBodyRecoilIkSolver::Reset()
{
	m_Situation.Reset();
	m_uFlags = 0;
	m_bSolverComplete = false;
	m_bActive = false;
}

bool CBodyRecoilIkSolver::IsDead() const
{
	return m_bSolverComplete;
}

void CBodyRecoilIkSolver::Iterate(float dt, crSkeleton& skeleton)
{
	// Iterate is called by the motiontree thread. It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyRecoil] || CBaseIkManager::ms_DisableAllSolvers)
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
		sprintf(debugText, "%s\n", "BodyRecoil::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(dt, skeleton);

	if (!IsBlocked())
	{
		Apply(skeleton);
	}
}

void CBodyRecoilIkSolver::Request(const CIkRequestBodyRecoil& request)
{
	m_uFlags = request.GetFlags();
	m_bActive = true;
}

void CBodyRecoilIkSolver::PreIkUpdate(float UNUSED_PARAM(deltaTime))
{
	if (m_bActive && (m_uFlags & (BODYRECOILIK_APPLY_LEFTARMIK | BODYRECOILIK_APPLY_RIGHTARMIK)))
	{
		crIKSolverArms::eArms arm = (m_uFlags & BODYRECOILIK_APPLY_LEFTARMIK) ? crIKSolverArms::LEFT_ARM : crIKSolverArms::RIGHT_ARM;
		CArmIkSolver* pArmSolver = static_cast<CArmIkSolver*>(m_pPed->GetIkManager().GetSolver(IKManagerSolverTypes::ikSolverTypeArm));

		if ((pArmSolver == NULL) || (!pArmSolver->GetArmActive(arm) && (pArmSolver->GetArmBlend(arm) == 0.0f)) || ((pArmSolver->GetControlFlags(arm) & AIK_TARGET_COUNTER_BODY_RECOIL) != 0))
		{
			crSkeleton* pSkeleton = m_pPed->GetSkeleton();

			if (pSkeleton)
			{
				// Apply arm IK to counter body recoil
				const eAnimBoneTag hand = (arm == crIKSolverArms::LEFT_ARM) ? BONETAG_L_HAND : BONETAG_R_HAND;
				const Mat34V& mtxHand = pSkeleton->GetObjectMtx(m_pPed->GetBoneIndexFromBoneTag(hand));
				Vector3 vOffset(VEC3V_TO_VECTOR3(mtxHand.GetCol3()));
#if FPS_MODE_SUPPORTED
				Quaternion qOffset(QUATV_TO_QUATERNION(QuatVFromMat34V(mtxHand)));
				m_pPed->GetIkManager().SetRelativeArmIKTarget(arm, m_pPed, BONETAG_INVALID, vOffset, qOffset, AIK_USE_FULL_REACH | AIK_USE_ORIENTATION | AIK_TARGET_COUNTER_BODY_RECOIL, 0.0f, PEDIK_ARMS_FADEOUT_TIME_QUICK);
#else
				m_pPed->GetIkManager().SetRelativeArmIKTarget(arm, m_pPed, BONETAG_INVALID, vOffset, AIK_USE_FULL_REACH | AIK_TARGET_COUNTER_BODY_RECOIL, 0.0f, PEDIK_ARMS_FADEOUT_TIME_QUICK);
#endif // FPS_MODE_SUPPORTED
			}
		}
	}
}

void CBodyRecoilIkSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		m_bActive = false;

		// Reset solver instantly if ped is not visible since the motion tree update will be skipped
		// and the solver could be reused by another ped
		if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
		{
			ResetArm();
			Reset();
			m_bSolverComplete = true;
		}
	}
}

void CBodyRecoilIkSolver::Update(float deltaTime, crSkeleton& skeleton)
{
	ikAssertf(IsValid(), "CBodyRecoilIkSolver not valid for this skeleton");

	if (!IsValid())
	{
		m_bSolverComplete = true;
		return;
	}

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || m_pPed->GetUsingRagdoll() || m_pPed->GetDisableBodyRecoilSolver())
	{
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableBodyRecoilSolver())
		{
			ResetArm();
			Reset();
			m_bSolverComplete = true;
		}

		return;
	}

	// Recoil matrix stored in IK helper bone
	const Mat34V& mtxRecoil = skeleton.GetLocalMtx(m_auArmBoneIdx[RIGHT_ARM][IKHELPER]);

	Vec3V vAxis;
	ScalarV vAngle;
	QuatV qRotation(QuatVFromMat34V(mtxRecoil));
	QuatVToAxisAngle(vAxis, vAngle, qRotation);

	// Keep rotation angle positive when rotating back
	if (IsLessThanAll(Dot(Negate(mtxRecoil.GetCol1()), vAxis), ScalarV(V_ZERO)))
	{
		vAngle = Negate(vAngle);
	}

	const ScalarV vTranslation(Mag(mtxRecoil.GetCol3()));
	Vec4V vTargetValues(vTranslation, vAngle, vAngle, vAngle);

	if (!m_bActive)
	{
		if (IsZeroAll(m_Situation.vDampedValues))
		{
			ResetArm();
			Reset();
			m_bSolverComplete = true;
			return;
		}

		vTargetValues.ZeroComponents();
	}

	const CWeapon* pWeapon = m_pPed->GetWeaponManager()->GetEquippedWeapon();
	int weaponType = pWeapon && pWeapon->GetWeaponInfo() && pWeapon->GetWeaponInfo()->GetIsAutomatic() ? 1 : 0;

	const Vec4V vZero(V_ZERO);
	const ScalarV vTimeStep(deltaTime);

	Vec4V vBlendRate(SelectFT(IsLessThanOrEqual(m_Situation.vDampedValues, vTargetValues), Negate(ms_Params.aWeaponParams[weaponType].avBlendRates[1]), ms_Params.aWeaponParams[weaponType].avBlendRates[0]));

	m_Situation.vDampedValues = AddScaled(m_Situation.vDampedValues, vBlendRate, vTimeStep); 

	// Clamp to target value if current value is beyond
	VecBoolV vbIsBeyondTarget(And(IsGreaterThan(vBlendRate, vZero), IsGreaterThan(m_Situation.vDampedValues, vTargetValues)));
	VecBoolV vbIsBelowTarget(And(IsLessThan(vBlendRate, vZero), IsLessThan(m_Situation.vDampedValues, vTargetValues)));
	VecBoolV vbCondition(Or(vbIsBeyondTarget, vbIsBelowTarget));
	vbCondition = Or(vbCondition, IsZero(vBlendRate));

	m_Situation.vDampedValues = SelectFT(vbCondition, m_Situation.vDampedValues, vTargetValues);
	m_Situation.fTargetRotation = vAngle.Getf();

#if __IK_DEV
	if(CaptureDebugRender())
	{
		char debugText[64];
		formatf(debugText, "Recoil Trans XYZ = %7.4f, %7.4f, %7.4f\n", mtxRecoil.GetCol3().GetXf(), mtxRecoil.GetCol3().GetYf(), mtxRecoil.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "Recoil Rot       = %7.4f\n", vAngle.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

void CBodyRecoilIkSolver::Apply(rage::crSkeleton& skeleton)
{
	const Mat34V& mtxParent = *skeleton.GetParentMtx();
	const Mat34V& mtxPrimarySpine = skeleton.GetObjectMtx(m_auSpineBoneIdx[SPINE3]);

	const bool bLegIKEnabled = m_pPed->GetIkManager().IsActive(IKManagerSolverTypes::ikSolverTypeLeg);
	const CPedWeaponManager* pWeaponManager = m_pPed->GetWeaponManager();
	const CWeapon* pWeapon = pWeaponManager ? pWeaponManager->GetEquippedWeapon() : NULL;
	const CObject* pWeaponObject = pWeaponManager ? pWeaponManager->GetEquippedWeaponObject() : NULL;
	int weaponType = pWeapon && pWeapon->GetWeaponInfo() && pWeapon->GetWeaponInfo()->GetIsAutomatic() ? 1 : 0;
	int baseBoneIndex = m_auSpineBoneIdx[SPINE0];

	const WeaponParams& weaponParams = ms_Params.aWeaponParams[weaponType];

	const ScalarV vScalarZero(V_ZERO);
	const ScalarV vScalarEps(V_FLT_SMALL_4);

	Mat34V mtxTransform;
	Vec3V vAxis;

	// Component values
	ScalarV vSpineYawValue(Scale(m_Situation.vDampedValues.GetX(), ScalarV(weaponParams.afSpineStrength[0])));
	ScalarV vSpinePitchValue(Scale(m_Situation.vDampedValues.GetY(), ScalarV(weaponParams.afSpineStrength[1])));
	ScalarV vClavValue(Scale(m_Situation.vDampedValues.GetX(), ScalarV(weaponParams.fClavicleStrength)));
	ScalarV vRootValue(Scale(m_Situation.vDampedValues.GetW(), ScalarV(weaponParams.fRootStrength)));
	ScalarV vPelvValue(Scale(m_Situation.vDampedValues.GetW(), ScalarV(weaponParams.fPelvisStrength)));

	if (pWeapon && (pWeaponManager->GetEquippedWeaponAttachPoint() == CPedEquippedWeapon::AP_LeftHand))
	{
		// Negate
		vSpineYawValue = Negate(vSpineYawValue);
	}

	// Calculate rotation matrix
	Mat34V mtxRotation;
	mtxRotation.SetCol3(mtxPrimarySpine.GetCol3());

	if (pWeaponObject)
	{
		// Use weapon forward
		mtxTransform = pWeaponObject->GetMatrix();
		vAxis = UnTransform3x3Full(mtxParent, mtxTransform.GetCol0());
	}
	else
	{
		// Use torso target
		Vec3V vTargetPosition(VECTOR3_TO_VEC3V(m_pPed->GetIkManager().GetTorsoTarget()));
		vTargetPosition = UnTransformFull(mtxParent, vTargetPosition);
		vAxis = Subtract(vTargetPosition, mtxPrimarySpine.GetCol3());
		vAxis = NormalizeFast(vAxis);
	}
	mtxRotation.SetCol1(vAxis);

	vAxis = Cross(vAxis, UnTransform3x3Full(mtxParent, Vec3V(V_UP_AXIS_WZERO)));
	vAxis = NormalizeFast(vAxis);
	mtxRotation.SetCol0(vAxis);

	vAxis = Cross(mtxRotation.GetCol0(), mtxRotation.GetCol1());
	vAxis = NormalizeFast(vAxis);
	mtxRotation.SetCol2(vAxis);

	// Rotate components
	QuatV qRot;
	QuatV qYaw;
	QuatV qPitch;

	// Root/Pelvis/SpineRoot
	if (bLegIKEnabled && (IsGreaterThanAll(Abs(vRootValue), vScalarEps) || IsGreaterThanAll(Abs(vPelvValue), vScalarEps)))
	{
		// Skeleton update from root bone due to translation and pelvis rotation
		baseBoneIndex = m_uRootBoneIdx;

		const Mat34V& mtxPelvis = skeleton.GetObjectMtx(m_uPelvisBoneIdx);
		Mat34V& mtxPelvisLocal = skeleton.GetLocalMtx(m_uPelvisBoneIdx);
		const Mat34V& mtxSpineRoot = skeleton.GetObjectMtx(m_uSpineRootBoneIdx);
		Mat34V& mtxSpineRootLocal = skeleton.GetLocalMtx(m_uSpineRootBoneIdx);
		Mat34V& mtxRootLocal = skeleton.GetLocalMtx(m_uRootBoneIdx);

		qRot = QuatVFromAxisAngle(UnTransform3x3Full(mtxPelvis, mtxRotation.GetCol0()), vPelvValue);
		Mat34VFromQuatV(mtxTransform, qRot);
		Transform3x3(mtxPelvisLocal, mtxTransform, mtxPelvisLocal);

		qRot = QuatVFromAxisAngle(UnTransform3x3Full(mtxSpineRoot, mtxRotation.GetCol0()), vPelvValue);
		Mat34VFromQuatV(mtxTransform, qRot);
		Transform3x3(mtxSpineRootLocal, mtxTransform, mtxSpineRootLocal);

		Vec3V vTranslation(mtxRotation.GetCol1());
		vTranslation.SetZ(vScalarZero);
		vTranslation = NormalizeFast(vTranslation);
		vTranslation = Scale(vTranslation, vRootValue);
		mtxRootLocal.SetCol3(Subtract(mtxRootLocal.GetCol3(), vTranslation));
	}

	// Spine/Arms
	if (IsGreaterThanAll(Abs(vSpineYawValue), vScalarEps) || IsGreaterThanAll(Abs(vSpinePitchValue), vScalarEps))
	{
		ScalarV vYaw(Negate(vSpineYawValue));
		ScalarV vPitch(Negate(vSpinePitchValue));

		if (bLegIKEnabled)
		{
			// Account for pelvis rotation
			vPitch = Subtract(vPitch, vPelvValue);
		}

		for (int spineBone = 0; spineBone < NUM_SPINE_PARTS; ++spineBone)
		{
			const Mat34V& mtxSpine = skeleton.GetObjectMtx(m_auSpineBoneIdx[spineBone]);
			Mat34V& mtxSpineLocal = skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBone]);

			qYaw = QuatVFromAxisAngle(UnTransform3x3Full(mtxSpine, mtxRotation.GetCol2()), Scale(vYaw, ScalarV(ms_Params.vSpineScale[spineBone])));
			qPitch = QuatVFromAxisAngle(UnTransform3x3Full(mtxSpine, mtxRotation.GetCol0()), Scale(vSpinePitchValue, ScalarV(ms_Params.vSpineScale[spineBone])));
			qRot = Multiply(qPitch, qYaw);
			Mat34VFromQuatV(mtxTransform, qRot);
			Transform3x3(mtxSpineLocal, mtxTransform, mtxSpineLocal); 
		}

		vYaw = vSpineYawValue;

		// Counter arms
		for (int arm = 0; arm < NUM_ARMS; ++arm)
		{
			const Mat34V& mtxArm = skeleton.GetObjectMtx(m_auArmBoneIdx[arm][UPPER]);
			Mat34V& mtxArmLocal = skeleton.GetLocalMtx(m_auArmBoneIdx[arm][UPPER]);

			qYaw = QuatVFromAxisAngle(UnTransform3x3Full(mtxArm, mtxRotation.GetCol2()), vYaw);
			qPitch = QuatVFromAxisAngle(UnTransform3x3Full(mtxArm, mtxRotation.GetCol0()), vPitch);
			qRot = Multiply(qPitch, qYaw);
			Mat34VFromQuatV(mtxTransform, qRot);
			Transform3x3(mtxArmLocal, mtxTransform, mtxArmLocal); 
		}

		// Counter neck
		{
			const Mat34V& mtxNeck = skeleton.GetObjectMtx(m_uNeckBoneIdx);
			Mat34V& mtxNeckLocal = skeleton.GetLocalMtx(m_uNeckBoneIdx);

			qYaw = QuatVFromAxisAngle(UnTransform3x3Full(mtxNeck, mtxRotation.GetCol2()), vYaw);
			qPitch = QuatVFromAxisAngle(UnTransform3x3Full(mtxNeck, mtxRotation.GetCol0()), vPitch);
			qRot = Multiply(qPitch, qYaw);
			Mat34VFromQuatV(mtxTransform, qRot);
			Transform3x3(mtxNeckLocal, mtxTransform, mtxNeckLocal);
		}
	}

	// Clavicle
	if (IsGreaterThanAll(Abs(vClavValue), vScalarEps))
	{
		const Mat34V& mtxClav = skeleton.GetObjectMtx(m_auArmBoneIdx[RIGHT_ARM][CLAVICLE]);
		Mat34V& mtxClavLocal = skeleton.GetLocalMtx(m_auArmBoneIdx[RIGHT_ARM][CLAVICLE]);

		qRot = QuatVFromAxisAngle(UnTransform3x3Full(mtxClav, mtxRotation.GetCol2()), Negate(vClavValue));
		Mat34VFromQuatV(mtxTransform, qRot);
		Transform3x3(mtxClavLocal, mtxTransform, mtxClavLocal);
	}

	// Update
	skeleton.PartialUpdate(baseBoneIndex);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		const Mat34V& mtxRecoil = skeleton.GetLocalMtx(m_auArmBoneIdx[RIGHT_ARM][IKHELPER]);
		const Vec3V vEulersXYZ(Mat34VToEulersXYZ(mtxRecoil));
		const ScalarV vRotation(Negate(vEulersXYZ.GetY()));
		const ScalarV vTranslation(Mag(mtxRecoil.GetCol3()));

		char debugText[128];
		formatf(debugText, "SPNY: %7.4f %7.4f %7.4f", vTranslation.Getf(), m_Situation.vDampedValues.GetXf(), vSpineYawValue.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "SPNP: %7.4f %7.4f %7.4f", vRotation.Getf(), m_Situation.vDampedValues.GetYf(), vSpinePitchValue.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "CLAV: %7.4f %7.4f %7.4f", vTranslation.Getf(), m_Situation.vDampedValues.GetXf(), vClavValue.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "ROOT: %7.4f %7.4f %7.4f", vRotation.Getf(), m_Situation.vDampedValues.GetWf(), vRootValue.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		formatf(debugText, "PELV: %7.4f %7.4f %7.4f", vRotation.Getf(), m_Situation.vDampedValues.GetWf(), vPelvValue.Getf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		if (ms_Params.bRenderTarget)
		{
			Transform(mtxRotation, mtxParent, mtxRotation);

			const ScalarV vScalarOne(V_ONE);
			Vec3V vStart(mtxRotation.GetCol3());
			Vec3V vEnd(AddScaled(vStart, mtxRotation.GetCol0(), vScalarOne));
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_red4);
			vEnd = AddScaled(vStart, mtxRotation.GetCol1(), vScalarOne);
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_green4);
			vEnd = AddScaled(vStart, mtxRotation.GetCol2(), vScalarOne);
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue4);
		}
	}
#endif // __IK_DEV
}

void CBodyRecoilIkSolver::ResetArm()
{
	if (m_uFlags & (BODYRECOILIK_APPLY_LEFTARMIK | BODYRECOILIK_APPLY_RIGHTARMIK))
	{
		crIKSolverArms::eArms arm = (m_uFlags & BODYRECOILIK_APPLY_LEFTARMIK) ? crIKSolverArms::LEFT_ARM : crIKSolverArms::RIGHT_ARM;
		CArmIkSolver* pArmSolver = static_cast<CArmIkSolver*>(m_pPed->GetIkManager().GetSolver(IKManagerSolverTypes::ikSolverTypeArm));

		if (pArmSolver && (pArmSolver->GetArmActive(arm) || (pArmSolver->GetArmBlend(arm) > 0.0f)) && ((pArmSolver->GetControlFlags(arm) & AIK_TARGET_COUNTER_BODY_RECOIL) != 0))
		{
			pArmSolver->ResetArm(arm);
		}
	}
}

bool CBodyRecoilIkSolver::IsValid()
{
	return m_bBonesValid;
}

bool CBodyRecoilIkSolver::IsBlocked()
{
	return m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK) || m_pPed->GetMovePed().GetSecondaryBlendHelper().IsFlagSet(APF_BLOCK_IK);
}

#if __IK_DEV

void CBodyRecoilIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

bool CBodyRecoilIkSolver::CaptureDebugRender()
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

void CBodyRecoilIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CBodyRecoilIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyRecoil], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");
		pBank->AddToggle("Debug Target", &ms_Params.bRenderTarget, NullCB, "");

#if ENABLE_BODYRECOIL_SOLVER_WIDGETS
		pBank->PushGroup("Single Shot Weapons", false);
			pBank->PushGroup("Blend Rates", false);
				pBank->AddSlider("Translation Blend In Rate", &ms_Params.aWeaponParams[0].avBlendRates[0][0], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Translation Blend Out Rate", &ms_Params.aWeaponParams[0].avBlendRates[1][0], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Rotation Blend In Rate", &ms_Params.aWeaponParams[0].avBlendRates[0][1], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Rotation Blend Out Rate", &ms_Params.aWeaponParams[0].avBlendRates[1][1], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Head Rotation Blend In Rate", &ms_Params.aWeaponParams[0].avBlendRates[0][2], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Head Rotation Blend Out Rate", &ms_Params.aWeaponParams[0].avBlendRates[1][2], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pelvis Rotation Blend In Rate", &ms_Params.aWeaponParams[0].avBlendRates[0][3], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pelvis Rotation Blend Out Rate", &ms_Params.aWeaponParams[0].avBlendRates[1][3], 0.0f, 10.0f, 0.01f, NullCB, "");
			pBank->PopGroup();
			pBank->PushGroup("Tunables", false);
				pBank->AddSlider("Spine Yaw Strength", &ms_Params.aWeaponParams[0].afSpineStrength[0], 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Spine Pitch Strength", &ms_Params.aWeaponParams[0].afSpineStrength[1], 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Clavicle Strength", &ms_Params.aWeaponParams[0].fClavicleStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Root Strength", &ms_Params.aWeaponParams[0].fRootStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Pelvis Strength", &ms_Params.aWeaponParams[0].fPelvisStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Automatic Weapons", false);
			pBank->PushGroup("Blend Rates", false);
				pBank->AddSlider("Translation Blend In Rate", &ms_Params.aWeaponParams[1].avBlendRates[0][0], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Translation Blend Out Rate", &ms_Params.aWeaponParams[1].avBlendRates[1][0], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Rotation Blend In Rate", &ms_Params.aWeaponParams[1].avBlendRates[0][1], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Rotation Blend Out Rate", &ms_Params.aWeaponParams[1].avBlendRates[1][1], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Head Rotation Blend In Rate", &ms_Params.aWeaponParams[1].avBlendRates[0][2], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Head Rotation Blend Out Rate", &ms_Params.aWeaponParams[1].avBlendRates[1][2], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pelvis Rotation Blend In Rate", &ms_Params.aWeaponParams[1].avBlendRates[0][3], 0.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pelvis Rotation Blend Out Rate", &ms_Params.aWeaponParams[1].avBlendRates[1][3], 0.0f, 10.0f, 0.01f, NullCB, "");
			pBank->PopGroup();
			pBank->PushGroup("Tunables", false);
				pBank->AddSlider("Spine Yaw Strength", &ms_Params.aWeaponParams[1].afSpineStrength[0], 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Spine Pitch Strength", &ms_Params.aWeaponParams[1].afSpineStrength[1], 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Clavicle Strength", &ms_Params.aWeaponParams[1].fClavicleStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Root Strength", &ms_Params.aWeaponParams[1].fRootStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
				pBank->AddSlider("Pelvis Strength", &ms_Params.aWeaponParams[1].fPelvisStrength, 0.0f, 10.0f, 0.1f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
#endif // ENABLE_BODYREACT_SOLVER_WIDGETS

	pBank->PopGroup();
}

#endif //__IK_DEV
