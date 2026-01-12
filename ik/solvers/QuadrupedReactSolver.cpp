// 
// ik/solvers/QuadrupedReactSolver.cpp
// 
// Copyright (C) 2014 Rockstar Games.  All Rights Reserved.
// 

#include "QuadrupedReactSolver.h"

#if IK_QUAD_REACT_SOLVER_ENABLE

#include "fwdebug/picker.h"
#include "math/angmath.h"

#include "camera/CamInterface.h"
#include "ik/IkManager.h"
#include "Peds/ped.h"

#define ENABLE_QUADRUPED_REACT_SOLVER_WIDGETS	0

#if IK_QUAD_REACT_SOLVER_USE_POOL
FW_INSTANTIATE_CLASS_POOL(CQuadrupedReactSolver, QUAD_REACT_SOLVER_POOL_MAX, atHashString("CQuadrupedReactSolver",0x6f14d82e));
#else
s8 CQuadrupedReactSolver::ms_NoOfFreeSpaces = QUAD_REACT_SOLVER_POOL_MAX;
#endif // IK_QUAD_REACT_SOLVER_USE_POOL

CQuadrupedReactSolver::Params CQuadrupedReactSolver::ms_Params =
{
	// avSpineLimits[kMaxSpineBones][2]
	{
		{ Vec3V( -0.120f, -0.120f, -0.120f), Vec3V(  0.120f,  0.120f,  0.120f) },
		{ Vec3V( -0.150f, -0.120f, -0.150f), Vec3V(  0.150f,  0.120f,  0.150f) },
		{ Vec3V( -0.180f, -0.120f, -0.180f), Vec3V(  0.180f,  0.120f,  0.180f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) }
	},

	// avNeckLimits[kMaxNeckBones][2]
	{ 
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) },
		{ Vec3V( -0.205f, -0.120f, -0.120f), Vec3V(  0.205f,  0.120f,  0.120f) }
	},

	// avPelvisLimits[2]
	{ 
		Vec3V( -0.175f, -0.050f, -0.050f), Vec3V(  0.175f,  0.050f,  0.050f)
	},

	{ 0.349f, 0.349f, 0.349f },							// afMaxDeflectionAngle[kNumTypes]
	PI * 0.5f,											// fMaxDeflectionAngleLimit
	80.0f,												// fMaxCameraDistance

	// afSpineStiffness[kNumTypes][kMaxSpineBones]
	{
		{ 0.025f, 0.025f, 0.025f, 0.025f },					// kLocal
		{ 0.025f, 0.025f, 0.025f, 0.025f },					// kRemote
		{ 0.025f, 0.025f, 0.025f, 0.025f }					// kRemoteRemote
	},
	// afNeckStiffness[kNumTypes][kMaxNeckBones]
	{
		{ 0.025f, 0.025f, 0.20f, 0.25f, 0.30f, 0.35f },	// kLocal
		{ 0.025f, 0.025f, 0.20f, 0.25f, 0.30f, 0.35f },	// kRemote
		{ 0.025f, 0.025f, 0.20f, 0.25f, 0.30f, 0.35f }	// kRemoteRemote
	},
	{ 0.10f, 0.10f, 0.10f },							// afPelvisStiffness[kNumTypes]
	{ 0.80f, 0.80f, 0.80f },							// afReactMagnitude[kNumTypes]
	{ 1.00f, 1.00f, 1.00f },							// afReactMagnitudeNetModifier[kNumTypes]
	{ 0.80f, 0.80f, 0.80f },							// afReactSpeedModifier[kNumTypes]
	3.0f,												// fMaxCharacterSpeed
	4.0f,												// fReactDistanceModifier
	{ 15.0f, 100.0f },									// afReactDistanceLimits[2]
	0.20f,												// fMaxRandomImpulsePositionOffset
	120,												// uSecondaryMotionDelayMS
	{ 800, 800, 800 },									// auReactDurationMS[kNumTypes]
	1.5f												// fWeaponGroupModifierSniper
};

#if __IK_DEV
CQuadrupedReactSolver::DebugParams CQuadrupedReactSolver::ms_DebugParams =
{
	0,													// m_iNoOfTexts
	false												// m_bRender
};
CDebugRenderer CQuadrupedReactSolver::ms_DebugHelper;
#endif //__IK_DEV

CQuadrupedReactSolver::CQuadrupedReactSolver(CPed *pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pPed(pPed)
{
	SetPrority(4);
	Reset();

	crSkeleton& skeleton = *pPed->GetSkeleton();
	const s16* aParentIndices = skeleton.GetSkeletonData().GetParentIndices();
	bool bBonesValid = true;

	m_uRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	m_uPelvisBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_PELVIS);
	m_uHeadBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);

	bBonesValid &= (m_uRootBoneIdx != (u16)-1) && (m_uPelvisBoneIdx != (u16)-1) && (m_uHeadBoneIdx != (u16)-1);

	// TODO: Generalize spine bone enumeration similar to neck bones below.
	//		 Need a common reference bone as a child of the last spine bone (eg. clavicle)
	//		 Unfortunately, not all quadrupeds have the clavicle as a child of the last spine bone.
	const eAnimBoneTag aeSpineBoneTags[] = { BONETAG_SPINE0, BONETAG_SPINE1, BONETAG_SPINE2, BONETAG_SPINE3 };
	for (int spineBone = 0; spineBone < kMaxSpineBones; ++spineBone)
	{
		m_auSpineBoneIdx[spineBone] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeSpineBoneTags[spineBone]);
		bBonesValid &= (m_auSpineBoneIdx[spineBone] != (u16)-1);
	}
	m_uSpineBoneCount = kMaxSpineBones;

	u16 uBoneIdx = m_uHeadBoneIdx;
	m_uNeckBoneCount = 0;
	while (aParentIndices[uBoneIdx] != m_auSpineBoneIdx[m_uSpineBoneCount - 1])
	{
		m_uNeckBoneCount++;
		uBoneIdx = aParentIndices[uBoneIdx];
	}
	Assertf(m_uNeckBoneCount, "Skeleton has no neck bones");

	uBoneIdx = m_uHeadBoneIdx;
	for (int neckBone = m_uNeckBoneCount - 1; neckBone >= 0; --neckBone)
	{
		m_auNeckBoneIdx[neckBone] = aParentIndices[uBoneIdx];
		uBoneIdx = aParentIndices[uBoneIdx];
	}

	m_bBonesValid = bBonesValid;

	Assertf(bBonesValid, "Modelname = (%s) is missing bones necessary for react ik. Expecting: BONETAG_ROOT, BONETAG_PELVIS, BONETAG_SPINE[0..3], BONETAG_NECK, BONETAG_HEAD", pPed ? pPed->GetModelName() : "Unknown model");
}

CQuadrupedReactSolver::~CQuadrupedReactSolver()
{
	Reset();
}

void CQuadrupedReactSolver::Reset()
{
	for (int react = 0; react < kMaxReacts; ++react)
	{
		m_aoReacts[react].Reset();
	}

	m_bRemoveSolver = false;
}

bool CQuadrupedReactSolver::IsDead() const
{
	return m_bRemoveSolver;
}

void CQuadrupedReactSolver::Iterate(float dt, crSkeleton& skeleton)
{
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeQuadrupedReact] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		ms_DebugHelper.Reset();
		ms_DebugParams.m_iNoOfTexts = 0;

		char debugText[32];
		formatf(debugText, 32, "%s", "CQuadrupedReactSolver::Begin");
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(dt);

	if (IsActive())
	{
		if (!IsBlocked())
		{
			if (IsValid())
			{
				Apply(skeleton);
			}
		}
	}
	else
	{
		Reset();
		m_bRemoveSolver = true;
	}
}

void CQuadrupedReactSolver::Request(const CIkRequestBodyReact& request)
{
	ikAssertf(IsValid(), "CQuadrupedReactSolver not valid for this skeleton");

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	if (pSkeleton == NULL)
		return;

	int typeIndex = GetTypeIndex();

	float fReactMagnitude = ms_Params.afReactMagnitude[typeIndex];

	if (fReactMagnitude == 0.0f)
		return;

	// Modify react magnitude based on speed of character
	float fReactMagnitudeModifierRatio = Clamp(m_pPed->GetVelocity().Mag() / ms_Params.fMaxCharacterSpeed, 0.0f, 1.0f);
	float fReactMagnitudeModifier = 1.0f + (fReactMagnitudeModifierRatio * ms_Params.afReactSpeedModifier[typeIndex]);
	fReactMagnitude *= fReactMagnitudeModifier;

	// Modify react magnitude for network games
	if (NetworkInterface::IsGameInProgress())
	{
		fReactMagnitude *= ms_Params.afReactMagnitudeNetModifier[typeIndex];
		fReactMagnitude *= GetWeaponGroupModifier(request.GetWeaponGroup());
	}

	// Scale and clamp
	const float fMaxDeflectionAngle = Clamp(fReactMagnitude * ms_Params.afMaxDeflectionAngle[typeIndex], 0.0f, ms_Params.fMaxDeflectionAngleLimit);

	// Find free react record
	int freeIndex = -1;

	// Limit number of reactions per frame. Otherwise we're not necessarily able to service all of the requests
	// that end up coming in on subsequent frames. Shotguns give one request per pellet so for the assault shotgun which has a high
	// rate of fire it's easily possible to not have enough free react records to react to a second shot.
	const int kMaxNumReactsStartedPerFrame = 3;
	int numReactsStartedThisFrame = 0;

	for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
	{
		if (m_aoReacts[reactIndex].bActive && (m_aoReacts[reactIndex].fDuration == 0.0f))
		{
			numReactsStartedThisFrame++;
			if (numReactsStartedThisFrame >= kMaxNumReactsStartedPerFrame)
			{
				return;
			}
		}
	}

	const float fMultipleReactModifier = 1.0f - ((float)Min(numReactsStartedThisFrame, kMaxNumReactsStartedPerFrame - 1) / (float)kMaxNumReactsStartedPerFrame);

	// Get inactive impulse
	for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
	{
		if (!m_aoReacts[reactIndex].bActive)
		{
			freeIndex = reactIndex;
			break;
		}
	}

	if (freeIndex == -1)
		return;

	// Setup react
	ReactSituation& react = m_aoReacts[freeIndex];
	react.bActive = true;
	react.bLocalInflictor = request.GetLocalInflictor();
	react.fDuration = 0.0f;
	react.fSecondaryMotionDuration = ((float)-ms_Params.uSecondaryMotionDelayMS) * 0.001f;

	const float fRandomOffset = ms_Params.fMaxRandomImpulsePositionOffset;

	// Calculate impulse position/direction in object space
	const Mat34V& mtxParent = *pSkeleton->GetParentMtx();
	Vec3V vImpulsePosition(UnTransformFull(mtxParent, request.GetPosition()));
	Vec3V vImpulse(UnTransform3x3Full(mtxParent, request.GetDirection()));
	vImpulse = Normalize(vImpulse);
	vImpulse = Scale(vImpulse, ScalarV(fReactMagnitude));

	// Randomly adjust impulse position
	// Build up/side axis relative to direction of impulse
	Vec3V vSide(Normalize(Cross(vImpulse, Vec3V(V_UP_AXIS_WZERO))));
	Vec3V vUp(Normalize(Cross(vSide, vImpulse)));
	vImpulsePosition = AddScaled(vImpulsePosition, vSide, ScalarV(fwRandom::GetRandomNumberInRange(-fRandomOffset, fRandomOffset)));
	vImpulsePosition = AddScaled(vImpulsePosition, vUp, ScalarV(fwRandom::GetRandomNumberInRange(-fRandomOffset, fRandomOffset)));

	// Setup bones
	const Vec3V vZero(V_ZERO);
	const ScalarV vScalarZero(V_ZERO);
	Vec3V vRadius;
	Vec3V vTorque;
	ScalarV vMagnitude;

	// Spine bones
	for (int spineBone = 0; spineBone < m_uSpineBoneCount; ++spineBone)
	{
		BoneSituation& boneSituation = react.aoSpineBones[spineBone];
		boneSituation.fAngle = 0.0f;

		const Mat34V& mtxSpine = pSkeleton->GetObjectMtx(m_auSpineBoneIdx[spineBone]);

		vRadius = Subtract(vImpulsePosition, mtxSpine.GetCol3());
		vTorque = Cross(vRadius, vImpulse);
		vMagnitude = Mag(vTorque);

		boneSituation.vAxis = InvScaleSafe(vTorque, vMagnitude, vZero);
		boneSituation.fMaxAngle = Min(Scale(vMagnitude, ScalarV(ms_Params.afSpineStiffness[typeIndex][spineBone])).Getf(), fMaxDeflectionAngle) * fMultipleReactModifier;
	}

	// Neck bones
	for (int neckBone = 0; neckBone < m_uNeckBoneCount; ++neckBone)
	{
		BoneSituation& boneSituation = react.aoNeckBones[neckBone];
		boneSituation.fAngle = 0.0f;

		const Mat34V& mtxNeck = pSkeleton->GetObjectMtx(m_auNeckBoneIdx[neckBone]);

		vRadius = Subtract(vImpulsePosition, mtxNeck.GetCol3());
		vTorque = Cross(vRadius, vImpulse);
		vMagnitude = Mag(vTorque);

		boneSituation.vAxis = InvScaleSafe(vTorque, vMagnitude, vZero);
		boneSituation.fMaxAngle = Min(Scale(vMagnitude, ScalarV(ms_Params.afNeckStiffness[typeIndex][neckBone])).Getf(), fMaxDeflectionAngle) * fMultipleReactModifier;
	}

	// Head
	react.oHeadBone = react.aoNeckBones[m_uNeckBoneCount - 1];

	// Pelvis
	BoneSituation& boneSituation = react.oPelvisBone;
	boneSituation.fAngle = 0.0f;

	const Mat34V& mtxPelvis = pSkeleton->GetObjectMtx(m_uPelvisBoneIdx);

	vRadius = Subtract(vImpulsePosition, mtxPelvis.GetCol3());
	vTorque = Cross(vRadius, vImpulse);
	vMagnitude = Mag(vTorque);

	boneSituation.vAxis = InvScaleSafe(vTorque, vMagnitude, vZero);
	boneSituation.fMaxAngle = Min(Scale(vMagnitude, ScalarV(ms_Params.afPelvisStiffness[typeIndex])).Getf(), fMaxDeflectionAngle) * fMultipleReactModifier;
}

void CQuadrupedReactSolver::Update(float deltaTime)
{
	ikAssertf(IsValid(), "CQuadrupedReactSolver not valid for this skeleton");

	if (!IsValid())
	{
		m_bRemoveSolver = true;
		return;
	}

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoReactSolver())
	{
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoReactSolver())
		{
			Reset();
			m_bRemoveSolver = true;
		}

		return;
	}

	int typeIndex = GetTypeIndex();

	const float fMaxDuration = (float)ms_Params.auReactDurationMS[typeIndex] / 1000.0f;
	const float fReactDistanceModifier = GetReactDistanceModifier();

	for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
	{
		ReactSituation& reactSituation = m_aoReacts[reactIndex];

		if (reactSituation.bActive)
		{
			reactSituation.fDuration += deltaTime;
			reactSituation.fSecondaryMotionDuration += deltaTime;

			float fInterval = Clamp(reactSituation.fDuration / fMaxDuration, 0.0f, 1.0f);
			float fSample = SampleResponseCurve(fInterval);

			float fSecondaryMotionInterval = Clamp(reactSituation.fSecondaryMotionDuration / fMaxDuration, 0.0f, 1.0f);
			float fSecondaryMotionSample = SampleResponseCurve(fSecondaryMotionInterval);

			for (int spineBoneIndex = 0; spineBoneIndex < m_uSpineBoneCount; ++spineBoneIndex)
			{
				reactSituation.aoSpineBones[spineBoneIndex].fAngle = fSample * Clamp(reactSituation.aoSpineBones[spineBoneIndex].fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			}

			for (int neckBoneIndex = 0; neckBoneIndex < m_uNeckBoneCount; ++neckBoneIndex)
			{
				reactSituation.aoNeckBones[neckBoneIndex].fAngle = fSample * Clamp(reactSituation.aoNeckBones[neckBoneIndex].fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			}

			reactSituation.oPelvisBone.fAngle = fSample * Clamp(reactSituation.oPelvisBone.fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			reactSituation.oHeadBone.fAngle = fSecondaryMotionSample * Clamp(reactSituation.oHeadBone.fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);

			reactSituation.bActive = (fInterval != 1.0f) || (fSecondaryMotionInterval != 1.0f);
		}
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[128];

		formatf(debugText, 128, "R: {   SP0,   SP1,   SP2,   SP3}{   NK0,   NK1,   NK2,   NK3,   NK4,   NK5}{  HEAD,  PELV}{ DUR,SDUR}");
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);

		for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
		{
			ReactSituation& reactSituation = m_aoReacts[reactIndex];

			if (reactSituation.bActive)
			{
				formatf(debugText, 128, "%1d: {%6.3f,%6.3f,%6.3f,%6.3f}{%6.3f,%6.3f,%6.3f,%6.3f,%6.3f,%6.3f}{%6.3f,%6.3f}{%4d,%4d}", reactIndex, 
					reactSituation.aoSpineBones[0].fAngle, reactSituation.aoSpineBones[1].fAngle, reactSituation.aoSpineBones[2].fAngle, reactSituation.aoSpineBones[3].fAngle,
					reactSituation.aoNeckBones[0].fAngle, reactSituation.aoNeckBones[1].fAngle, reactSituation.aoNeckBones[2].fAngle, 
					reactSituation.aoNeckBones[3].fAngle, reactSituation.aoNeckBones[4].fAngle, reactSituation.aoNeckBones[5].fAngle,
					reactSituation.oHeadBone.fAngle, reactSituation.oPelvisBone.fAngle,
					(int)(reactSituation.fDuration * 1000.0f), (int)(reactSituation.fSecondaryMotionDuration * 1000.0f));
			}
			else
			{
				formatf(debugText, 128, "%1d:", reactIndex);
			}

			ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
		}

		formatf(debugText, 128, "Distance Modifier: %5.2f", fReactDistanceModifier);
		ms_DebugHelper.Text2d(ms_DebugParams.m_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

void CQuadrupedReactSolver::Apply(rage::crSkeleton& skeleton)
{
	ikAssertf(IsValid(), "CQuadrupedReactSolver not valid for this skeleton");

	const bool bLegIKEnabled = m_pPed->GetIkManager().IsActive(IKManagerSolverTypes::ikSolverTypeQuadLeg);

	// Rotate spines
	for (int spineBone = 0; spineBone < m_uSpineBoneCount; ++spineBone)
	{
		const Mat34V& mtxBone = skeleton.GetObjectMtx(m_auSpineBoneIdx[spineBone]);
		Mat34V& mtxLocal = skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBone]);

		Mat34V mtxTransform;
		QuatV qRotation(V_IDENTITY);

		for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				Vec3V vLocal(UnTransform3x3Full(mtxBone, m_aoReacts[reactIndex].aoSpineBones[spineBone].vAxis));
				qRotation = Multiply(qRotation, QuatVFromAxisAngle(vLocal, ScalarV(m_aoReacts[reactIndex].aoSpineBones[spineBone].fAngle)));
			}
		}

		ClampAngles(qRotation, ms_Params.avSpineLimits[spineBone][0], ms_Params.avSpineLimits[spineBone][1]);
		Mat34VFromQuatV(mtxTransform, qRotation);
		Transform3x3(mtxLocal, mtxTransform, mtxLocal);
	}

	if (bLegIKEnabled)
	{
		// Rotate pelvis
		const Mat34V& mtxBone = skeleton.GetObjectMtx(m_uPelvisBoneIdx);
		Mat34V& mtxLocal = skeleton.GetLocalMtx(m_uPelvisBoneIdx);

		Mat34V mtxTransform;
		QuatV qRotation(V_IDENTITY);

		for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				Vec3V vLocal(UnTransform3x3Full(mtxBone, m_aoReacts[reactIndex].oPelvisBone.vAxis));
				qRotation = Multiply(qRotation, QuatVFromAxisAngle(vLocal, ScalarV(m_aoReacts[reactIndex].oPelvisBone.fAngle)));
			}
		}

		ClampAngles(qRotation, ms_Params.avPelvisLimits[0], ms_Params.avPelvisLimits[1]);
		Mat34VFromQuatV(mtxTransform, qRotation);
		Transform3x3(mtxLocal, mtxTransform, mtxLocal);
	}

	// Rotate neck
	for (int neckBone = 0; neckBone < m_uNeckBoneCount; ++neckBone)
	{
		const Mat34V& mtxBone = skeleton.GetObjectMtx(m_auNeckBoneIdx[neckBone]);
		Mat34V& mtxLocal = skeleton.GetLocalMtx(m_auNeckBoneIdx[neckBone]);

		Mat34V mtxTransform;
		QuatV qRotation(V_IDENTITY);

		for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				Vec3V vLocal(UnTransform3x3Full(mtxBone, m_aoReacts[reactIndex].aoNeckBones[neckBone].vAxis));
				qRotation = Multiply(qRotation, QuatVFromAxisAngle(vLocal, ScalarV(m_aoReacts[reactIndex].aoNeckBones[neckBone].fAngle)));
			}
		}

		ClampAngles(qRotation, ms_Params.avNeckLimits[neckBone][0], ms_Params.avNeckLimits[neckBone][1]);
		Mat34VFromQuatV(mtxTransform, qRotation);
		Transform3x3(mtxLocal, mtxTransform, mtxLocal);
	}

	// Counter rotate head
	const Mat34V& mtxBone = skeleton.GetObjectMtx(m_uHeadBoneIdx);
	Mat34V& mtxLocal = skeleton.GetLocalMtx(m_uHeadBoneIdx);

	Mat34V mtxTransform;
	QuatV qRotation(V_IDENTITY);

	for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
	{
		if (m_aoReacts[reactIndex].bActive)
		{
			Vec3V vLocal(UnTransform3x3Full(mtxBone, m_aoReacts[reactIndex].aoNeckBones[m_uNeckBoneCount - 1].vAxis));
			qRotation = Multiply(qRotation, QuatVFromAxisAngle(vLocal, ScalarV(m_aoReacts[reactIndex].oHeadBone.fAngle - m_aoReacts[reactIndex].aoNeckBones[m_uNeckBoneCount - 1].fAngle)));
		}
	}

	ClampAngles(qRotation, ms_Params.avNeckLimits[m_uNeckBoneCount - 1][0], ms_Params.avNeckLimits[m_uNeckBoneCount - 1][1]);
	Mat34VFromQuatV(mtxTransform, qRotation);
	Transform3x3(mtxLocal, mtxTransform, mtxLocal);

	// Update
	skeleton.PartialUpdate(bLegIKEnabled ? m_uRootBoneIdx : m_auSpineBoneIdx[0]);
}

void CQuadrupedReactSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		// Reset solver instantly if ped is not visible since the motion tree update will be skipped and solver could be reused by another ped
		if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
		{
			Reset();
			m_bRemoveSolver = true;
		}
	}
}

#if __IK_DEV

void CQuadrupedReactSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_DebugHelper.Render();
	}
}

bool CQuadrupedReactSolver::CaptureDebugRender()
{
	if (ms_DebugParams.m_bRender)
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

void CQuadrupedReactSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CQuadrupedReactSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeQuadrupedReact], NullCB, "");
		pBank->AddToggle("Debug", &ms_DebugParams.m_bRender, NullCB, "");

#if ENABLE_QUADRUPED_REACT_SOLVER_WIDGETS
		pBank->PushGroup("Spine", false);
			const char* aszSpineBoneTitle[] = { "Spine 0", "Spine 1", "Spine 2", "Spine 3" };
			pBank->PushGroup("Limits (radians)", false);
				for (int spineBone = 0; spineBone < kMaxSpineBones; ++spineBone)
				{
					pBank->AddTitle(aszSpineBoneTitle[spineBone]);
					pBank->AddSlider("MinX", &ms_Params.avSpineLimits[spineBone][0][0], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MinY", &ms_Params.avSpineLimits[spineBone][0][1], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MinZ", &ms_Params.avSpineLimits[spineBone][0][2], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxX", &ms_Params.avSpineLimits[spineBone][1][0], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxY", &ms_Params.avSpineLimits[spineBone][1][1], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxZ", &ms_Params.avSpineLimits[spineBone][1][2], -PI, PI, 0.01f, NullCB, "");
				}
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Neck", false);
			const char* aszNeckBoneTitle[] = { "Neck 0", "Neck 1", "Neck 2", "Neck 3", "Neck 4", "Neck 5" };
			pBank->PushGroup("Limits (radians)", false);
				for (int neckBone = 0; neckBone < kMaxNeckBones; ++neckBone)
				{
					pBank->AddTitle(aszNeckBoneTitle[neckBone]);
					pBank->AddSlider("MinX", &ms_Params.avNeckLimits[neckBone][0][0], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MinY", &ms_Params.avNeckLimits[neckBone][0][1], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MinZ", &ms_Params.avNeckLimits[neckBone][0][2], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxX", &ms_Params.avNeckLimits[neckBone][1][0], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxY", &ms_Params.avNeckLimits[neckBone][1][1], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("MaxZ", &ms_Params.avNeckLimits[neckBone][1][2], -PI, PI, 0.01f, NullCB, "");
				}
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Pelvis", false);
			pBank->PushGroup("Limits (radians)", false);
				pBank->AddSlider("MinX", &ms_Params.avPelvisLimits[0][0], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("MinY", &ms_Params.avPelvisLimits[0][1], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("MinZ", &ms_Params.avPelvisLimits[0][2], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("MaxX", &ms_Params.avPelvisLimits[1][0], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("MaxY", &ms_Params.avPelvisLimits[1][1], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("MaxZ", &ms_Params.avPelvisLimits[1][2], -PI, PI, 0.01f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Tunables", false);
			pBank->AddSlider("Max Deflection Angle Limit (radians)", &ms_Params.fMaxDeflectionAngleLimit, 0.0f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Max Character Speed", &ms_Params.fMaxCharacterSpeed, 0.0f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Max Camera Distance", &ms_Params.fMaxCameraDistance, 0.0f, 1000.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Camera Distance Modifier", &ms_Params.fReactDistanceModifier, 0.01f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Camera Distance Modifier Min Limit", &ms_Params.afReactDistanceLimits[0], 0.01f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Camera Distance Modifier Max Limit", &ms_Params.afReactDistanceLimits[1], 0.01f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Random Impulse Position Offset", &ms_Params.fMaxRandomImpulsePositionOffset, -1.0f, 1.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Secondary Motion Delay", &ms_Params.uSecondaryMotionDelayMS, 0, 1000, 10, NullCB, "");

			const char* aszType[] = { "kLocal", "kRemote", "kRemoteRemote" };
			for (int typeIndex = 0; typeIndex < Params::kNumTypes; ++typeIndex)
			{
				pBank->PushGroup(aszType[typeIndex], false);
					pBank->AddSlider("React Duration (ms)", &ms_Params.auReactDurationMS[typeIndex], 0, 1000, 10, NullCB, "");
					pBank->AddSlider("Max Deflection Angle (radians)", &ms_Params.afMaxDeflectionAngle[typeIndex], 0.0f, 100.0f, 0.1f, NullCB, "");
					pBank->AddSlider("React Magnitude", &ms_Params.afReactMagnitude[typeIndex], 0.0f, 1000.0f, 0.1f, NullCB, "");
					pBank->AddSlider("React Magnitude Net Modifier", &ms_Params.afReactMagnitudeNetModifier[typeIndex], 0.0f, 10.0f, 0.05f, NullCB, "");
					pBank->AddSlider("React Speed Modifier", &ms_Params.afReactSpeedModifier[typeIndex], 0.0f, 100.0f, 0.1f, NullCB, "");
					pBank->PushGroup("Stiffness", false);
						for (int spineBone = 0; spineBone < kMaxSpineBones; ++spineBone)
						{
							pBank->AddSlider(aszSpineBoneTitle[spineBone], &ms_Params.afSpineStiffness[typeIndex][spineBone], -100.0f, 100.0f, 0.1f, NullCB, "");
						}
						for (int neckBone = 0; neckBone < kMaxNeckBones; ++neckBone)
						{
							pBank->AddSlider(aszNeckBoneTitle[neckBone], &ms_Params.afNeckStiffness[typeIndex][neckBone], -100.0f, 100.0f, 0.1f, NullCB, "");
						}
						pBank->AddSlider("Pelvis", &ms_Params.afPelvisStiffness[typeIndex], -100.0f, 100.0f, 0.1f, NullCB, "");
					pBank->PopGroup();
				pBank->PopGroup();
			}
			pBank->PushGroup("Weapon Group Modifiers", false);
				pBank->AddSlider("Sniper", &ms_Params.fWeaponGroupModifierSniper, 0.0f, 10.0f, 0.05f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
#endif // ENABLE_QUADRUPED_REACT_SOLVER_WIDGETS

	pBank->PopGroup();
}

#endif //__IK_DEV

bool CQuadrupedReactSolver::IsValid()
{
	return m_bBonesValid;
}

bool CQuadrupedReactSolver::IsActive() const
{
	for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
	{
		const ReactSituation& reactSituation = m_aoReacts[reactIndex];

		if (reactSituation.bActive)
		{
			return true;
		}
	}

	return false;
}

bool CQuadrupedReactSolver::IsBlocked()
{
	return false;
}

float CQuadrupedReactSolver::SampleResponseCurve(float fInterval)
{
	// Interval that the curve is at maximum (1.0f)
	static float IntervalAtMaximum = 0.2f;

	float fSample = 0.0f;
	float fAngle;

	// Curve is Cos for [0, IntervalAtMaximum] and Cos^3 for [IntervalAtMaximum, 1]
	if (fInterval < IntervalAtMaximum)
	{
		fAngle = (fInterval / IntervalAtMaximum) * HALF_PI;

		// Ramp from 0.0f -> 1.0f
		fSample = rage::Cosf(HALF_PI - fAngle);
	}
	else
	{
		fAngle = ((fInterval - IntervalAtMaximum) / (1.0f - IntervalAtMaximum)) * HALF_PI;

		// Ramp from 1.0f -> 0.0f
		fSample = rage::Cosf(fAngle);
		fSample *= fSample * fSample;
	}

	return fSample;
}

float CQuadrupedReactSolver::GetReactDistanceModifier()
{
	// Modify react magnitude based on distance of character from camera
	ScalarV vReactDistanceModifier(V_ONE);

	const ScalarV vScalarZero(V_ZERO);
	const ScalarV vScalarOne(V_ONE);

	const ScalarV vMinLimit(ms_Params.afReactDistanceLimits[0]);
	const ScalarV vMaxLimit(ms_Params.afReactDistanceLimits[1]);

	ScalarV vDistToCamera(Mag(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition())));

	ScalarV vReactModifierRatio(Subtract(vMaxLimit, vMinLimit));
	vReactModifierRatio = InvertSafe(vReactModifierRatio, vScalarZero);
	vReactModifierRatio = Scale(vReactModifierRatio, Subtract(vDistToCamera, vMinLimit));
	vReactModifierRatio = Clamp(vReactModifierRatio, vScalarZero, vScalarOne);

	vReactDistanceModifier = AddScaled(vReactDistanceModifier, vReactModifierRatio, ScalarV(ms_Params.fReactDistanceModifier));

	return vReactDistanceModifier.Getf();
}

float CQuadrupedReactSolver::GetWeaponGroupModifier(u32 weaponGroup)
{
	if (weaponGroup > 0)
	{
		if (weaponGroup == WEAPONGROUP_SNIPER)
		{
			return ms_Params.fWeaponGroupModifierSniper;
		}
	}

	return 1.0f;
}

int CQuadrupedReactSolver::GetBoneIndex(int component)
{
	if (component == -1)
		return -1;

	rage::fragInst* pFragInst = m_pPed->GetFragInst();

	if (pFragInst == NULL)
		return -1;

	rage::phBound* pBound = pFragInst->GetArchetype()->GetBound();

	if (pBound->GetType() != rage::phBound::COMPOSITE)
		return -1;

	return pFragInst->GetType()->GetBoneIndexFromID(pFragInst->GetTypePhysics()->GetChild(component)->GetBoneID());
}

int CQuadrupedReactSolver::GetTypeIndex(const CIkRequestBodyReact& request)
{
	int typeIndex = m_pPed->IsNetworkClone() ? Params::kRemote : Params::kLocal;

	if ((typeIndex == Params::kRemote) && !request.GetLocalInflictor())
	{
		typeIndex = Params::kRemoteRemote;
	}

	return typeIndex;
}

int CQuadrupedReactSolver::GetTypeIndex() const
{
	int typeIndex = m_pPed->IsNetworkClone() ? Params::kRemote : Params::kLocal;

	if (typeIndex == Params::kRemote)
	{
		for (int reactIndex = 0; reactIndex < kMaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive && !m_aoReacts[reactIndex].bLocalInflictor)
			{
				typeIndex = Params::kRemoteRemote;
				break;
			}
		}
	}

	return typeIndex;
}

void CQuadrupedReactSolver::ClampAngles(Vec3V_InOut eulers, Vec3V_In min, Vec3V_In max)
{
	eulers.SetXf(AngleClamp(eulers.GetXf(), min.GetXf(), max.GetXf()));
	eulers.SetYf(AngleClamp(eulers.GetYf(), min.GetYf(), max.GetYf()));
	eulers.SetZf(AngleClamp(eulers.GetZf(), min.GetZf(), max.GetZf()));
}

void CQuadrupedReactSolver::ClampAngles(QuatV_InOut q, Vec3V_In min, Vec3V_In max)
{
	Vec3V eulers(QuatVToEulersXYZ(q));
	ClampAngles(eulers, min, max);
	q = QuatVFromEulersXYZ(eulers);
}

#endif // IK_QUAD_REACT_SOLVER_ENABLE
