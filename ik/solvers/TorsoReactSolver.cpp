// 
// ik/solvers/TorsoReactSolver.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
// 

#include "TorsoReactSolver.h"

#include "diag/art_channel.h"
#include "fwdebug/picker.h"

#include "animation/anim_channel.h"
#include "animation/MovePed.h"
#include "camera/CamInterface.h"
#include "ik/IkManager.h"
#include "math/angmath.h"
#include "peds/Ped.h"
#include "peds/PedIntelligence.h"
#include "scene/world/GameWorld.h"
#include "vectormath/classfreefuncsv.h"
#include "camera/helpers/Frame.h"
#include "task/combat/taskcombatmelee.h"
#include "Task/Vehicle/TaskInVehicle.h"

#define TORSOREACT_SOLVER_POOL_MAX 16
#define ENABLE_BODYREACT_SOLVER_WIDGETS	0

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CTorsoReactIkSolver, TORSOREACT_SOLVER_POOL_MAX, 0.38f, atHashString("CTorsoReactIkSolver",0xa7f63267));

ANIM_OPTIMISATIONS();

CTorsoReactIkSolver::Params CTorsoReactIkSolver::ms_Params =
{
	// avSpineLimits
	{
		{ Vector3( -0.120f, -0.120f, -0.120f), Vector3(  0.120f,  0.120f,  0.120f) },
		{ Vector3( -0.150f, -0.120f, -0.150f), Vector3(  0.150f,  0.120f,  0.150f) },
		{ Vector3( -0.180f, -0.120f, -0.180f), Vector3(  0.180f,  0.120f,  0.180f) },
		{ Vector3( -0.205f, -0.120f, -0.120f), Vector3(  0.205f,  0.120f,  0.120f) }
	},

	// avPelvisLimits
	{ Vector3( -0.175f, -0.050f, -0.050f), Vector3(  0.175f,  0.050f,  0.050f) },

	{ 0.05f, 0.05f, 0.13f, 0.05f },	// afRootLimit[NUM_TYPES]

	{ 0.13f, 0.13f, 0.15f, 0.15f },	// afMaxDeflectionAngle[NUM_TYPES]
	PI,								// fMaxDeflectionAngleLimit
	80.0f,							// fMaxCameraDistance

	{
		{ 0.1f, 0.2f, 0.4f, 0.8f },
		{ 0.1f, 0.2f, 0.4f, 0.8f },
		{ 0.2f, 0.4f, 0.6f, 1.0f },
		{ 0.1f, 0.2f, 0.4f, 0.8f }
	},								// afSpineStiffness[NUM_TYPES][NUM_SPINE_PARTS]
	{ 0.10f, 0.60f, 0.80f, 0.80f },	// afPelvisStiffness[NUM_TYPES]
	{ 0.05f, 0.20f, 1.00f, 0.20f },	// afRootStiffness[NUM_TYPES]

	{ 0.80f, 0.70f, 1.25f, 1.00f },	// afReactMagnitude[NUM_TYPES]

	{ 1.00f, 1.50f, 1.00f, 1.00f },	// afReactMagnitudeNetModifier[NUM_TYPES]

	{ 1.20f, 0.80f, 0.80f, 0.0f },	// afReactSpeedModifier[NUM_TYPES]
	3.0f,							// fMaxCharacterSpeed

	0.250f,							// fReactVehicleModifier
	0.175f,							// fReactFirstPersonVehicleModifier
	0.259f,							// fReactVehicleLimit
	1.0f,							// fReactMeleeModifierLocalPlayer
	1.5f,							// fReactMeleeModifier
	1.5f,							// fReactEnteringVehicleModifier;
	0.5f,							// fReactLowCoverModifier

	0.5f,							// fReactAimingModifier
#if FPS_MODE_SUPPORTED
	0.05f,							// fReactFPSModifier
#endif // FPS_MODE_SUPPORTED

	4.0f,							// fReactDistanceModifier
	{ 15.0f, 100.0f },				// afReactDistanceLimits[2]

	{ 0.00f, 0.00f, 0.07f, 0.07f },	// afMinDistFromCenterThreshold[NUM_TYPES]
	{ 0.00f, 0.00f, 0.15f, 0.15f },	// afMaxDistFromCenterThreshold[NUM_TYPES]

	-0.12f,							// fNeckDelay
	{ 420, 420, 375, 375 },			// auReactDurationMS[NUM_TYPES]

	{ 0.20f, 0.20f, 1.00f, 1.00f },	// afLowerBodyDeflectionAngleModifier[NUM_TYPES]
	{ 0.00f, 0.00f, 0.00f, 0.00f },	// afLowerBodyMinDistFromCenterThreshold[NUM_TYPES]
	{ 0.00f, 0.00f, 2.00f, 2.00f },	// afLowerBodyMaxDistFromCenterThreshold[NUM_TYPES]

	{ 1.5f, 1.5f, 1.5f, 1.5f },		// afWeaponGroupModifierSniper
	{ 1.0f, 1.0f, 1.0f, 0.7f },		// afWeaponGroupModifierShotgun

#if __IK_DEV
	Vector3(0.0f, 0.0f, 1.0f),		// vTestPosition
	Vector3(0.0f, 1.0f, 0.0f),		// vTestDirection
	false							// bDrawTest
#endif // __IK_DEV
};

#if __IK_DEV
CDebugRenderer CTorsoReactIkSolver::ms_debugHelper;
bool CTorsoReactIkSolver::ms_bRenderDebug = false;
int CTorsoReactIkSolver::ms_iNoOfTexts = 0;
#endif //__IK_DEV

CTorsoReactIkSolver::CTorsoReactIkSolver(CPed *pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pPed(pPed)
{
	SetPrority(4);
	Reset();

	bool bBonesValid = true;

	const eAnimBoneTag aeSpineBoneTags[] = { BONETAG_SPINE0, BONETAG_SPINE1, BONETAG_SPINE2, BONETAG_SPINE3 };
	for (int spineBoneTagIndex = 0; spineBoneTagIndex < NUM_SPINE_PARTS; ++spineBoneTagIndex)
	{
		m_auSpineBoneIdx[spineBoneTagIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeSpineBoneTags[spineBoneTagIndex]);
		bBonesValid &= (m_auSpineBoneIdx[spineBoneTagIndex] != (u16)-1);
	}

	const eAnimBoneTag aeArmBoneTags[] = { BONETAG_L_UPPERARM, BONETAG_R_UPPERARM };
	for (int armBoneTagIndex = 0; armBoneTagIndex < NUM_ARMS; ++armBoneTagIndex)
	{
		m_auArmBoneIdx[armBoneTagIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeArmBoneTags[armBoneTagIndex]);
		bBonesValid &= (m_auArmBoneIdx[armBoneTagIndex] != (u16)-1);
	}

	const eAnimBoneTag aeLegBoneTags[NUM_LEGS][NUM_LEG_PARTS] = 
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

	m_uPelvisBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_PELVIS);
	m_uRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	m_uNeckBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);;

	bBonesValid &= (m_uPelvisBoneIdx != (u16)-1) && (m_uRootBoneIdx != (u16)-1) && (m_uNeckBoneIdx != (u16)-1);

	m_bonesValid = bBonesValid;

	artAssertf(bBonesValid, "Modelname = (%s) is missing bones necessary for torso react ik. Expects the following bones: "
							"BONETAG_SPINE0, BONETAG_SPINE1, BONETAG_SPINE2, BONETAG_SPINE3, BONETAG_L_UPPERARM, BONETAG_R_UPPERARM, "
							"BONETAG_L_THIGH, BONETAG_R_THIGH, BONETAG_L_CALF, BONETAG_R_CALF, BONETAG_L_FOOT, BONETAG_R_FOOT, BONETAG_L_TOE, BONETAG_R_TOE", pPed ? pPed->GetModelName() : "Unknown model");
}

CTorsoReactIkSolver::~CTorsoReactIkSolver()
{
	Reset();
}

void CTorsoReactIkSolver::Reset()
{
	for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
	{
		m_aoReacts[reactIndex].Reset();
	}

	m_removeSolver = false;
}

bool CTorsoReactIkSolver::IsDead() const
{
	return m_removeSolver;
}

void CTorsoReactIkSolver::Iterate(float dt, crSkeleton& skeleton)
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorsoReact] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		ms_debugHelper.Reset();
		ms_iNoOfTexts = 0;

		if (ms_Params.bDrawTest)
		{
			Vector3 vTestDirection(ms_Params.vTestDirection);
			vTestDirection.NormalizeFast();

			ms_debugHelper.Sphere(ms_Params.vTestPosition, 0.02f, Color_blue);
			ms_debugHelper.Line(ms_Params.vTestPosition, ms_Params.vTestPosition + vTestDirection, Color_blue);
		}

		char debugText[100];
		sprintf(debugText, "%s\n", "TorsoReact::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(dt);

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
		m_removeSolver = true;
	}
}

void CTorsoReactIkSolver::Request(const CIkRequestBodyReact& request)
{
	ikAssertf(IsValid(), "Torso react solver not valid for this skeleton");

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	if (pSkeleton == NULL)
		return;

	int typeIndex = GetTypeIndex(request);

	float fReactMagnitude = ms_Params.afReactMagnitude[typeIndex];

	if (fReactMagnitude == 0.0f)
		return;

	// Reduce magnitude if character has a vehicle/mount and impulse is from the front, to prevent arms from not being able to reach,
	// unless the torso vehicle solver is running since it will compensate the torso if the arms cannot reach
	if (!m_pPed->IsDead() && ((m_pPed->GetIsDrivingVehicle() && !m_pPed->GetIkManager().IsActive(IKManagerSolverTypes::ikSolverTypeTorsoVehicle)) || m_pPed->GetMyMount()))
	{
		Mat34V mtxSpine;
		pSkeleton->GetGlobalMtx(m_auSpineBoneIdx[NUM_SPINE_PARTS - 1], mtxSpine);

		Vec3V vDir(request.GetDirection());
		vDir = Negate(vDir);
		vDir = NormalizeFast(vDir);
		ScalarV sDot = Dot(vDir, mtxSpine.GetCol1());

		bool bFirstPerson = m_pPed->IsInFirstPersonVehicleCamera();
		if(bFirstPerson)
		{
			fReactMagnitude *= ms_Params.fReactFirstPersonVehicleModifier;
		}
		else if (sDot.Getf() > ms_Params.fReactVehicleLimit)
		{
			fReactMagnitude *= ms_Params.fReactVehicleModifier;
		}
	}
	else if(m_pPed->GetIsEnteringVehicle(false))
	{
		fReactMagnitude *= ms_Params.fReactEnteringVehicleModifier;
	}
	
	// If in middle of melee task, modify react magnitude.
	CTaskMeleeActionResult *pMeleeActionResult = m_pPed->GetPedIntelligence()->GetTaskMeleeActionResult();
	if(pMeleeActionResult && !pMeleeActionResult->ShouldAllowInterrupt())
	{
		if(m_pPed->IsLocalPlayer())
		{
			fReactMagnitude *= ms_Params.fReactMeleeModifierLocalPlayer;
		}
		else
		{		
			fReactMagnitude *= ms_Params.fReactMeleeModifier;
		}
	}

	// Modify react magnitude based on aiming
	if (m_pPed->IsLocalPlayer() && m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsAiming))
	{
		fReactMagnitude *= ms_Params.fReactAimingModifier;
	}

#if FPS_MODE_SUPPORTED
	// Modify react magnitude if first person shooter mode is active
	if (m_pPed->IsFirstPersonShooterModeEnabledForPlayer(false))
	{
		fReactMagnitude *= ms_Params.fReactFPSModifier;
	}
#endif // FPS_MODE_SUPPORTED

	// Modify react magnitude based on speed of character
	float fReactMagnitudeModifierRatio = Clamp(m_pPed->GetVelocity().Mag() / ms_Params.fMaxCharacterSpeed, 0.0f, 1.0f);
	float fReactMagnitudeModifier = 1.0f + (fReactMagnitudeModifierRatio * ms_Params.afReactSpeedModifier[typeIndex]);
	fReactMagnitude *= fReactMagnitudeModifier;

	// Modify react magnitude for network games
	if (NetworkInterface::IsGameInProgress())
	{
		fReactMagnitude *= ms_Params.afReactMagnitudeNetModifier[typeIndex];
		fReactMagnitude *= GetWeaponGroupModifier(typeIndex, request.GetWeaponGroup());
	}

	// Cover
	const bool bCover = m_pPed->GetIsInCover();

	if (bCover && ((typeIndex == Params::REMOTE_PLAYER) || (typeIndex == Params::REMOTE_REMOTE)))
	{
		CTaskInCover* pCoverTask = static_cast<CTaskInCover*>(m_pPed->GetPedIntelligence()->FindTaskActiveByType(CTaskTypes::TASK_IN_COVER));

		if (pCoverTask && !pCoverTask->IsCoverFlagSet(CTaskCover::CF_TooHighCoverPoint))
		{
			// Modify react magnitude if in low cover (reduce magnitude to compensate for crouching low cover pose)
			fReactMagnitude *= ms_Params.fReactLowCoverModifier;
		}
	}

	// Find free react record
	int freeIndex = -1;

	// We can only start up this many reactions per frame - otherwise we're not necessarily able to service all of the requests
	// that end up coming in on subsequent frames. Shotguns give one request per pellet so for the assault shotgun which has a high
	// rate of fire it's easily possible to not have enough free react records to react to a second shot.
	if (NetworkInterface::IsGameInProgress())
	{
		int numReactsStartedThisFrame = 0;
		static const int maximumNumReactsStartedPerFrame = 3;
		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive && m_aoReacts[reactIndex].fDuration == 0.0f)
			{
				numReactsStartedThisFrame++;
				if (numReactsStartedThisFrame >= maximumNumReactsStartedPerFrame)
				{
					return;
				}
			}
		}
	}

	// Game was shipped with only 4 react slots so maintaining that maximum for SP
	// Otherwise - 6 total react slots with a maximum of 2 that can be filled in a frame (i.e. from a single shot)
	// gives us essentially a triple-buffered system which allows us to handle hits from rapid firing multi-pellet weapons
	// like the assault shotgun
	static const int MaxReactsSP = 4;
	int numReacts = NetworkInterface::IsGameInProgress() ? MaxReacts : MaxReactsSP;

	// Get inactive impulse
	for (int reactIndex = 0; reactIndex < numReacts; ++reactIndex)
	{
		if (!m_aoReacts[reactIndex].bActive)
		{
			freeIndex = reactIndex;
			break;
		}
	}

	if (freeIndex == -1)
		return;

	ReactSituation& react = m_aoReacts[freeIndex];
	Vector3 vRadius;

	// Calculate impulse direction in object space
	const Matrix34& mtxParent = RCC_MATRIX34(*pSkeleton->GetParentMtx());

	Vector3 vImpulsePosition(VEC3V_TO_VECTOR3(request.GetPosition()));
	mtxParent.UnTransform(vImpulsePosition);

	Vector3 vImpulse(VEC3V_TO_VECTOR3(request.GetDirection()));
	mtxParent.UnTransform3x3(vImpulse);
	vImpulse.NormalizeFast();
	vImpulse *= fReactMagnitude;

	// Scale and clamp
	const float fMaxDeflectionAngle = Clamp(fReactMagnitude * ms_Params.afMaxDeflectionAngle[typeIndex], 0.0f, ms_Params.fMaxDeflectionAngleLimit);

	// Upper or lower body impulse
	int boneIndex = GetBoneIndex(request.GetComponent());

	bool bLowerBody = false;
	for (int legIndex = 0; legIndex < NUM_LEGS; ++legIndex)
	{
		for (int legPartIndex = 0; legPartIndex < NUM_LEG_PARTS; ++legPartIndex)
		{
			if (m_auLegBoneIdx[legIndex][legPartIndex] == boneIndex)
			{
				bLowerBody = true;
				break;
			}
		}
	}

	// Adjust impulse position if necessary
	if (!bCover)
	{
		// Adjust impulse position away from center of primary spine bone if necessary, to introduce more yaw torque
		const float SpineBoneDistThreshold = bLowerBody ? fwRandom::GetRandomNumberInRange(ms_Params.afLowerBodyMinDistFromCenterThreshold[typeIndex], ms_Params.afLowerBodyMaxDistFromCenterThreshold[typeIndex]) : fwRandom::GetRandomNumberInRange(ms_Params.afMinDistFromCenterThreshold[typeIndex], ms_Params.afMaxDistFromCenterThreshold[typeIndex]);
		if (SpineBoneDistThreshold > 0.0f)
		{
			const Matrix34& mtxSpine = RCC_MATRIX34(pSkeleton->GetObjectMtx(m_auSpineBoneIdx[NUM_SPINE_PARTS - 1]));

			vRadius.Subtract(vImpulsePosition, mtxSpine.d);

			float fRadiusDotSpine = vRadius.Dot(mtxSpine.c);
			float fAbsRadiusDotSpine = fabsf(fRadiusDotSpine);

			const float EPSILON = 0.00001f;
			if (fAbsRadiusDotSpine < SpineBoneDistThreshold)
			{
				vImpulsePosition += (mtxSpine.c * ((fRadiusDotSpine + EPSILON) / (fAbsRadiusDotSpine + EPSILON)) * (SpineBoneDistThreshold - (fAbsRadiusDotSpine + EPSILON)));
			}
		}
	}
	else
	{
		// Adjust impulse position towards mid section of character (spine bone 1) so that reacts don't cause the character
		// to rotate backwards and potentially intersect with what they are against while in cover
		const Mat34V& mtxSpine = pSkeleton->GetObjectMtx(m_auSpineBoneIdx[1]);
		const float fMaxRandomOffset = 0.025f;

		// Build up/side axis relative to direction of impulse
		const Vec3V vImpulseDirection(VECTOR3_TO_VEC3V(vImpulse));
		Vec3V vAdjustedPosition(VECTOR3_TO_VEC3V(vImpulsePosition));

		Vec3V vSide(Cross(vImpulseDirection, Vec3V(V_UP_AXIS_WZERO)));
		vSide = Normalize(vSide);
		Vec3V vUp(Cross(vSide, vImpulseDirection));
		vUp = Normalize(vUp);

		// Project vector from impulse position to spine bone onto up/side axis
		Vec3V vToSpine(Subtract(mtxSpine.GetCol3(), vAdjustedPosition));
		ScalarV vSideOffset(Dot(vToSpine, vSide));
		ScalarV vUpOffset(Dot(vToSpine, vUp));

		// Add randomness
		vSideOffset = Add(vSideOffset, ScalarV(fwRandom::GetRandomNumberInRange(-fMaxRandomOffset, fMaxRandomOffset)));
		vUpOffset = Add(vUpOffset, ScalarV(fwRandom::GetRandomNumberInRange(-fMaxRandomOffset, fMaxRandomOffset)));

		vAdjustedPosition = AddScaled(vAdjustedPosition, vSide, vSideOffset);
		vAdjustedPosition = AddScaled(vAdjustedPosition, vUp, vUpOffset);

		vImpulsePosition = VEC3V_TO_VECTOR3(vAdjustedPosition);
	}

	// Setup react
	react.bActive = true;
	react.bLocalInflictor = request.GetLocalInflictor();
	react.fDuration = 0.0f;
	react.fNeckDuration = ms_Params.fNeckDelay;

	// Setup bones
	Vector3 vZero(Vector3::ZeroType);
	Vector3 vTorque;
	float fTorque;

	const float fSpineMaxDeflectionAngleModifier = !bLowerBody ? 1.0f : ms_Params.afLowerBodyDeflectionAngleModifier[typeIndex];

	// Spine bones
	for (int spineBone = 0; spineBone < NUM_SPINE_PARTS; ++spineBone)
	{
		const Matrix34& mtxSpine = RCC_MATRIX34(pSkeleton->GetObjectMtx(m_auSpineBoneIdx[spineBone]));

		vRadius.Subtract(vImpulsePosition, mtxSpine.d);
		vTorque.Cross(vRadius, vImpulse);
		fTorque = vTorque.Mag();

		react.aoSpineBones[spineBone].vAxis = (fTorque != 0.0f) ? vTorque / fTorque : vZero;
		react.aoSpineBones[spineBone].fAngle = 0.0f;
		react.aoSpineBones[spineBone].fMaxAngle = fMaxDeflectionAngle * fSpineMaxDeflectionAngleModifier * ms_Params.afSpineStiffness[typeIndex][spineBone];
	}

	// Neck
	react.oNeckBone = react.aoSpineBones[NUM_SPINE_PARTS - 1];

	// Pelvis
	const Matrix34& mtxPelvis = RCC_MATRIX34(pSkeleton->GetObjectMtx(m_uPelvisBoneIdx));

	vRadius.Subtract(vImpulsePosition, mtxPelvis.d);
	vTorque.Cross(vRadius, vImpulse);
	fTorque = vTorque.Mag();

	react.oPelvisBone.vAxis = (fTorque != 0.0f) ? vTorque / fTorque : vZero;
	react.oPelvisBone.fAngle = 0.0f;
	react.oPelvisBone.fMaxAngle = fMaxDeflectionAngle * ms_Params.afPelvisStiffness[typeIndex];

	// Root
	react.oRootBone.vAxis = vImpulse;
	react.oRootBone.vAxis.z = 0.0f;
	react.oRootBone.vAxis.NormalizeFast();
	react.oRootBone.fAngle = 0.0f;
	react.oRootBone.fMaxAngle = !bCover ? fMaxDeflectionAngle * ms_Params.afRootStiffness[typeIndex] : 0.0f;
}

void CTorsoReactIkSolver::Update(float deltaTime)
{
	ikAssertf(IsValid(), "Torso react solver not valid for this skeleton");

	if (!IsValid())
	{
		m_removeSolver = true;
		return;
	}

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoReactSolver())
	{
		// If we go into ragdoll reset the solver
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoReactSolver())
		{
			Reset();
			m_removeSolver = true;
		}

		return;
	}

	int typeIndex = GetTypeIndex();

	const float fMaxDuration = (float)ms_Params.auReactDurationMS[typeIndex] / 1000.0f;
	const float fReactDistanceModifier = GetReactDistanceModifier();

	for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
	{
		ReactSituation& reactSituation = m_aoReacts[reactIndex];

		if (reactSituation.bActive)
		{
			reactSituation.fDuration += deltaTime;
			reactSituation.fNeckDuration += deltaTime;

			float fInterval = Clamp(reactSituation.fDuration / fMaxDuration, 0.0f, 1.0f);
			float fSample = SampleResponseCurve(fInterval);

			float fNeckInterval = Clamp(reactSituation.fNeckDuration / fMaxDuration, 0.0f, 1.0f);
			float fNeckSample = SampleResponseCurve(fNeckInterval);

			for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
			{
				reactSituation.aoSpineBones[spineBoneIndex].fAngle = fSample * Clamp(reactSituation.aoSpineBones[spineBoneIndex].fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			}

			reactSituation.oPelvisBone.fAngle = fSample * Clamp(reactSituation.oPelvisBone.fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			reactSituation.oRootBone.fAngle = fSample * Clamp(reactSituation.oRootBone.fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);
			reactSituation.oNeckBone.fAngle = fNeckSample * Clamp(reactSituation.oNeckBone.fMaxAngle * fReactDistanceModifier, 0.0f, ms_Params.fMaxDeflectionAngleLimit);

			reactSituation.bActive = (fInterval != 1.0f) || (fNeckInterval != 1.0f);
		}
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];

		sprintf(debugText, "R: {     SP0,     SP1,     SP2,     SP3 } {    NECK,  PELVIS,    ROOT }");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			ReactSituation& reactSituation = m_aoReacts[reactIndex];

			if (reactSituation.bActive)
			{
				sprintf(debugText, "%1d: { %7.4f, %7.4f, %7.4f, %7.4f } { %7.4f, %7.4f, %7.4f }", reactIndex, 
																								  reactSituation.aoSpineBones[0].fAngle,
																								  reactSituation.aoSpineBones[1].fAngle,
																								  reactSituation.aoSpineBones[2].fAngle,
																								  reactSituation.aoSpineBones[3].fAngle,
																								  reactSituation.oNeckBone.fAngle,
																								  reactSituation.oPelvisBone.fAngle,
																								  reactSituation.oRootBone.fAngle);
			}
			else
			{
				sprintf(debugText, "%1d:", reactIndex);
			}

			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}

		sprintf(debugText, "Distance Modifier: %5.2f", fReactDistanceModifier);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

void CTorsoReactIkSolver::Apply(rage::crSkeleton& skeleton)
{
	ikAssertf(IsValid(), "Torso react solver not valid for this skeleton");

	const int typeIndex = GetTypeIndex();
	const int PrimarySpineBone = NUM_SPINE_PARTS - 1;
	const bool bLegIKEnabled = m_pPed->GetIkManager().IsActive(IKManagerSolverTypes::ikSolverTypeLeg);

	Matrix34 mtxRotationLocal;
	Matrix34 mtxRotationUnitAxis;
	Vector3 vAxisLocal;

	// Rotate spines
	for (int spineBone = 0; spineBone < NUM_SPINE_PARTS; ++spineBone)
	{
		const Matrix34& mtxSpine = RCC_MATRIX34(skeleton.GetObjectMtx(m_auSpineBoneIdx[spineBone]));

		mtxRotationLocal.Identity();

		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				mtxSpine.UnTransform3x3(m_aoReacts[reactIndex].aoSpineBones[spineBone].vAxis, vAxisLocal);
				vAxisLocal.NormalizeFast();

				mtxRotationUnitAxis.MakeRotateUnitAxis(vAxisLocal, m_aoReacts[reactIndex].aoSpineBones[spineBone].fAngle);
				mtxRotationLocal.Dot3x3(mtxRotationUnitAxis);
			}
		}

		ClampAngles(mtxRotationLocal, ms_Params.avSpineLimits[spineBone][0], ms_Params.avSpineLimits[spineBone][1]);
		RC_MATRIX34(skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBone])).Dot3x3FromLeft(mtxRotationLocal);
	}

	if (bLegIKEnabled)
	{
		// Rotate pelvis
		const Matrix34& mtxPelvis = RCC_MATRIX34(skeleton.GetObjectMtx(m_uPelvisBoneIdx));

		mtxRotationLocal.Identity();

		Vector3 vRootDisplacement(Vector3::ZeroType);

		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				mtxPelvis.UnTransform3x3(m_aoReacts[reactIndex].oPelvisBone.vAxis, vAxisLocal);
				vAxisLocal.NormalizeFast();

				mtxRotationUnitAxis.MakeRotateUnitAxis(vAxisLocal, m_aoReacts[reactIndex].oPelvisBone.fAngle);
				mtxRotationLocal.Dot3x3(mtxRotationUnitAxis);

				vRootDisplacement += m_aoReacts[reactIndex].oRootBone.vAxis * m_aoReacts[reactIndex].oRootBone.fAngle;
			}
		}

		ClampAngles(mtxRotationLocal, ms_Params.avPelvisLimits[0], ms_Params.avPelvisLimits[1]);
		RC_MATRIX34(skeleton.GetLocalMtx(m_uPelvisBoneIdx)).Dot3x3FromLeft(mtxRotationLocal);

		// Translate root
		const float EPSILON = 0.00001f;
		float fRootDisplacement = vRootDisplacement.Mag();
		float fRootDisplacementClamped = Min(fRootDisplacement, ms_Params.afRootLimit[typeIndex]);

		RC_MATRIX34(skeleton.GetLocalMtx(m_uRootBoneIdx)).d += vRootDisplacement * ((fRootDisplacementClamped + EPSILON) / (fRootDisplacement + EPSILON));
	}

	// Counter rotate neck
	const Matrix34& mtxNeck = RCC_MATRIX34(skeleton.GetObjectMtx(m_uNeckBoneIdx));

	mtxRotationLocal.Identity();

	for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
	{
		if (m_aoReacts[reactIndex].bActive)
		{
			mtxNeck.UnTransform3x3(m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].vAxis, vAxisLocal);
			vAxisLocal.NormalizeFast();

			mtxRotationUnitAxis.MakeRotateUnitAxis(vAxisLocal, m_aoReacts[reactIndex].oNeckBone.fAngle - m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].fAngle);
			mtxRotationLocal.Dot3x3(mtxRotationUnitAxis);
		}
	}

	ClampAngles(mtxRotationLocal, ms_Params.avSpineLimits[PrimarySpineBone][0], ms_Params.avSpineLimits[PrimarySpineBone][1]);
	RC_MATRIX34(skeleton.GetLocalMtx(m_uNeckBoneIdx)).Dot3x3FromLeft(mtxRotationLocal);

	// Counter rotate arm(s)
	{
		const Matrix34& mtxRightArm = RCC_MATRIX34(skeleton.GetObjectMtx(m_auArmBoneIdx[1]));

		mtxRotationLocal.Identity();

		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				mtxRightArm.UnTransform3x3(m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].vAxis, vAxisLocal);
				vAxisLocal.NormalizeFast();

				mtxRotationUnitAxis.MakeRotateUnitAxis(vAxisLocal, -m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].fAngle);
				mtxRotationLocal.Dot3x3(mtxRotationUnitAxis);
			}
		}

		ClampAngles(mtxRotationLocal, ms_Params.avSpineLimits[PrimarySpineBone][0], ms_Params.avSpineLimits[PrimarySpineBone][1]);
		RC_MATRIX34(skeleton.GetLocalMtx(m_auArmBoneIdx[1])).Dot3x3FromLeft(mtxRotationLocal);
	}

	{
		const Matrix34& mtxLeftArm = RCC_MATRIX34(skeleton.GetObjectMtx(m_auArmBoneIdx[0]));

		mtxRotationLocal.Identity();

		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive)
			{
				mtxLeftArm.UnTransform3x3(m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].vAxis, vAxisLocal);
				vAxisLocal.NormalizeFast();

				mtxRotationUnitAxis.MakeRotateUnitAxis(vAxisLocal, -m_aoReacts[reactIndex].aoSpineBones[PrimarySpineBone].fAngle);
				mtxRotationLocal.Dot3x3(mtxRotationUnitAxis);
			}
		}

		ClampAngles(mtxRotationLocal, ms_Params.avSpineLimits[PrimarySpineBone][0], ms_Params.avSpineLimits[PrimarySpineBone][1]);
		RC_MATRIX34(skeleton.GetLocalMtx(m_auArmBoneIdx[0])).Dot3x3FromLeft(mtxRotationLocal);
	}

	// Update
	skeleton.PartialUpdate(bLegIKEnabled ? m_uRootBoneIdx : m_auSpineBoneIdx[0]);
}

void CTorsoReactIkSolver::PostIkUpdate(float deltaTime)
{
	if (deltaTime > 0.0f)
	{
		// Reset solver instantly if ped is not visible since the motion tree update will be skipped
		// and the solver could be reused by another ped
		if (!m_pPed->GetIsVisibleInSomeViewportThisFrame())
		{
			Reset();
			m_removeSolver = true;
		}
	}
}

#if __IK_DEV

void CTorsoReactIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

bool CTorsoReactIkSolver::CaptureDebugRender()
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

void CTorsoReactIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CTorsoReactIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorsoReact], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");

#if ENABLE_BODYREACT_SOLVER_WIDGETS
		pBank->PushGroup("Spine", false);
			const char* aszSpineBoneTitle[] = { "Spine 0", "Spine 1", "Spine 2", "Spine 3" };
			pBank->PushGroup("Limits (radians)", false);
				for (int spineBone = 0; spineBone < NUM_SPINE_PARTS; ++spineBone)
				{
					pBank->AddTitle(aszSpineBoneTitle[spineBone]);
					pBank->AddSlider("Min", &ms_Params.avSpineLimits[spineBone][0], -PI, PI, 0.01f, NullCB, "");
					pBank->AddSlider("Max", &ms_Params.avSpineLimits[spineBone][1], -PI, PI, 0.01f, NullCB, "");
				}
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Pelvis", false);
			pBank->PushGroup("Limits (radians)", false);
				pBank->AddSlider("Min", &ms_Params.avPelvisLimits[0], -PI, PI, 0.01f, NullCB, "");
				pBank->AddSlider("Max", &ms_Params.avPelvisLimits[1], -PI, PI, 0.01f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Neck", false);
			pBank->AddSlider("Delay", &ms_Params.fNeckDelay, -100.0f, 100.0f, 0.1f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Tunables", false);
			pBank->AddSlider("Max Deflection Angle Limit (radians)", &ms_Params.fMaxDeflectionAngleLimit, 0.0f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Max Camera Distance", &ms_Params.fMaxCameraDistance, 0.0f, 1000.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Vehicle Modifier", &ms_Params.fReactVehicleModifier, 0.01f, 1.0f, 0.01f, NullCB, "");
			pBank->AddSlider("First Person Vehicle Modifier", &ms_Params.fReactFirstPersonVehicleModifier, 0.01f, 1.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Melee Modifier Local Player", &ms_Params.fReactMeleeModifierLocalPlayer, 0.01f, 5.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Melee Modifier", &ms_Params.fReactMeleeModifier, 0.01f, 5.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Entering Modifier", &ms_Params.fReactEnteringVehicleModifier, 0.01f, 5.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Vehicle Limit", &ms_Params.fReactVehicleLimit, -1.0f, 1.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Low Cover", &ms_Params.fReactLowCoverModifier, 0.01f, 1.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Aiming Modifier", &ms_Params.fReactAimingModifier, 0.01f, 1.0f, 0.01f, NullCB, "");
#if FPS_MODE_SUPPORTED
			pBank->AddSlider("FPS Modifier", &ms_Params.fReactFPSModifier, 0.01f, 1.0f, 0.01f, NullCB, "");
#endif // FPS_MODE_SUPPORTED
			pBank->AddSlider("Camera Distance Modifier", &ms_Params.fReactDistanceModifier, 0.01f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Camera Distance Modifier Min Limit", &ms_Params.afReactDistanceLimits[0], 0.01f, 100.0f, 0.1f, NullCB, "");
			pBank->AddSlider("Camera Distance Modifier Max Limit", &ms_Params.afReactDistanceLimits[1], 0.01f, 100.0f, 0.1f, NullCB, "");
			const char* aszType[] = { "AI", "LOCAL_PLAYER", "REMOTE_PLAYER", "REMOTE_REMOTE" };
			for (int typeIndex = 0; typeIndex < Params::NUM_TYPES; ++typeIndex)
			{
				pBank->PushGroup(aszType[typeIndex], false);
					pBank->AddSlider("React Duration (ms)", &ms_Params.auReactDurationMS[typeIndex], 0, 1000, 10, NullCB, "");
					pBank->AddSlider("Max Deflection Angle (radians)", &ms_Params.afMaxDeflectionAngle[typeIndex], 0.0f, 100.0f, 0.1f, NullCB, "");
					pBank->AddSlider("React Magnitude", &ms_Params.afReactMagnitude[typeIndex], 0.0f, 1000.0f, 0.1f, NullCB, "");
					pBank->AddSlider("React Magnitude Net Modifier", &ms_Params.afReactMagnitudeNetModifier[typeIndex], 0.0f, 10.0f, 0.05f, NullCB, "");
					pBank->AddSlider("React Speed Modifier", &ms_Params.afReactSpeedModifier[typeIndex], 0.0f, 100.0f, 0.1f, NullCB, "");
					pBank->AddSlider("Min Dist From Spine Center", &ms_Params.afMinDistFromCenterThreshold[typeIndex], 0.0f, 1.0f, 0.01f, NullCB, "");
					pBank->AddSlider("Max Dist From Spine Center", &ms_Params.afMaxDistFromCenterThreshold[typeIndex], 0.0f, 1.0f, 0.01f, NullCB, "");
					pBank->AddSlider("Lower Body Deflection Angle Modifier", &ms_Params.afLowerBodyDeflectionAngleModifier[typeIndex], 0.0f, 100.0f, 0.1f, NullCB, "");
					pBank->AddSlider("Lower Body Min Dist From Spine Center", &ms_Params.afLowerBodyMinDistFromCenterThreshold[typeIndex], 0.0f, 1.0f, 0.01f, NullCB, "");
					pBank->AddSlider("Lower Body Max Dist From Spine Center", &ms_Params.afLowerBodyMaxDistFromCenterThreshold[typeIndex], 0.0f, 2.0f, 0.01f, NullCB, "");
					pBank->PushGroup("Stiffness", false);
						for (int spineBone = 0; spineBone < NUM_SPINE_PARTS; ++spineBone)
						{
							pBank->AddSlider(aszSpineBoneTitle[spineBone], &ms_Params.afSpineStiffness[typeIndex][spineBone], -100.0f, 100.0f, 0.1f, NullCB, "");
						}
						pBank->AddSlider("Pelvis", &ms_Params.afPelvisStiffness[typeIndex], -100.0f, 100.0f, 0.1f, NullCB, "");
						pBank->AddSlider("Root", &ms_Params.afRootStiffness[typeIndex], -100.0f, 100.0f, 0.1f, NullCB, "");
					pBank->PopGroup();
					pBank->AddSlider("Root Max Displacement", &ms_Params.afRootLimit[typeIndex], 0.0f, 1.0f, 0.01f);
					pBank->PushGroup("Weapon Group Modifiers", false);
						pBank->AddSlider("Sniper", &ms_Params.afWeaponGroupModifierSniper[typeIndex], 0.0f, 10.0f, 0.05f, NullCB, "");
						pBank->AddSlider("Shotgun", &ms_Params.afWeaponGroupModifierShotgun[typeIndex], 0.0f, 10.0f, 0.05f, NullCB, "");
					pBank->PopGroup();
				pBank->PopGroup();
			}
		pBank->PopGroup();
		pBank->PushGroup("Test Harness", false);
			pBank->AddToggle("Debug", &ms_Params.bDrawTest, NullCB, "");
			pBank->AddSlider("Position", &ms_Params.vTestPosition, Vector3(-1000.0f, -1000.0f, -1000.0f), Vector3(1000.0f, 1000.0f, 1000.0f), Vector3(0.01f, 0.01f, 0.01f), NullCB, "");
			pBank->AddSlider("Direction", &ms_Params.vTestDirection, Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f), Vector3(0.01f, 0.01f, 0.01f), NullCB, "");
			pBank->AddButton("Test", CTorsoReactIkSolver::TestReact);
		pBank->PopGroup();
#endif // ENABLE_BODYREACT_SOLVER_WIDGETS

	pBank->PopGroup();
}

void CTorsoReactIkSolver::TestReact()
{
	CEntity* pSelectedEntity = (CEntity*)g_PickerManager.GetSelectedEntity();

	if (pSelectedEntity && pSelectedEntity->GetIsTypePed())
	{
		CPed* pSelectedPed = static_cast<CPed*>(pSelectedEntity);

		CTorsoReactIkSolver::ms_Params.vTestDirection.NormalizeFast();

		phIntersection oIntersection;

		WorldProbe::CShapeTestProbeDesc probeDesc;
		WorldProbe::CShapeTestFixedResults<> probeResult;
		probeDesc.SetResultsStructure(&probeResult);
		probeDesc.SetStartAndEnd(CTorsoReactIkSolver::ms_Params.vTestPosition, CTorsoReactIkSolver::ms_Params.vTestPosition + (CTorsoReactIkSolver::ms_Params.vTestDirection * 2.0f));
		probeDesc.SetIncludeFlags(ArchetypeFlags::GTA_PED_INCLUDE_TYPES);
		probeDesc.SetTypeFlags(ArchetypeFlags::GTA_PED_TYPE);

		int component = -1;
		if(WorldProbe::GetShapeTestManager()->SubmitTest(probeDesc))
		{
			component = (int)probeResult[0].GetHitComponent();
		}

		CIkRequestBodyReact request(RC_VEC3V(CTorsoReactIkSolver::ms_Params.vTestPosition), RC_VEC3V(CTorsoReactIkSolver::ms_Params.vTestDirection), component);
		pSelectedPed->GetIkManager().Request(request);
	}
}

#endif //__IK_DEV

bool CTorsoReactIkSolver::IsValid()
{
	return m_bonesValid;
}

bool CTorsoReactIkSolver::IsActive() const
{
	for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
	{
		const ReactSituation& reactSituation = m_aoReacts[reactIndex];

		if (reactSituation.bActive)
		{
			return true;
		}
	}

	return false;
}

bool CTorsoReactIkSolver::IsBlocked()
{
	return m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK) || 
		   m_pPed->GetMovePed().GetSecondaryBlendHelper().IsFlagSet(APF_BLOCK_IK);
}

float CTorsoReactIkSolver::SampleResponseCurve(float fInterval)
{
	// Interval that the curve is at maximum (1.0f)
	const float IntervalAtMaximum = 0.2f;

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

float CTorsoReactIkSolver::GetReactDistanceModifier()
{
	// Modify react magnitude based on distance of character from camera
	ScalarV vReactDistanceModifier(V_ONE);

	if (NetworkInterface::IsNetworkOpen() && m_pPed->IsNetworkClone())
	{
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
	}

	return vReactDistanceModifier.Getf();
}

float CTorsoReactIkSolver::GetWeaponGroupModifier(int type, u32 weaponGroup)
{
	if ((type < NUM_TYPES) && (weaponGroup > 0))
	{
		if (weaponGroup == WEAPONGROUP_SNIPER)
		{
			return ms_Params.afWeaponGroupModifierSniper[type];
		}
		else if (weaponGroup == WEAPONGROUP_SHOTGUN)
		{
			return ms_Params.afWeaponGroupModifierShotgun[type];
		}
	}

	return 1.0f;
}

int CTorsoReactIkSolver::GetBoneIndex(int component)
{
	if (component == -1)
		return -1;

	rage::fragInst* pFragInst = m_pPed->GetFragInst();

	if (pFragInst == NULL)
		return -1;

	rage::phArchetype* pArchetype = pFragInst->GetArchetype();

	if (pArchetype == NULL)
		return -1;

	rage::phBound* pBound = pArchetype->GetBound();

	if (pBound == NULL)
		return -1;

	if (pBound->GetType() != rage::phBound::COMPOSITE)
		return -1;

	const rage::fragType* pFragType = pFragInst->GetType();
	rage::fragPhysicsLOD* pTypePhysics = pFragInst->GetTypePhysics();

	if ((pFragType == NULL) || (pTypePhysics == NULL))
		return -1;

	if (pTypePhysics->GetAllChildren() == NULL)
		return -1;

	if ((component >= 0) && (component < pTypePhysics->GetNumChildren()))
	{
		fragTypeChild* pChild = pTypePhysics->GetChild(component);

		if (pChild)
		{
			return pChild->GetBoneID();
		}
	}

	return -1;
}

int CTorsoReactIkSolver::GetTypeIndex(const CIkRequestBodyReact& request)
{
	int typeIndex = m_pPed->IsLocalPlayer() ? Params::LOCAL_PLAYER : m_pPed->IsAPlayerPed() ? Params::REMOTE_PLAYER : Params::AI;

	if ((typeIndex == Params::REMOTE_PLAYER) && !request.GetLocalInflictor())
	{
		typeIndex = Params::REMOTE_REMOTE;
	}

	return typeIndex;
}

int CTorsoReactIkSolver::GetTypeIndex() const
{
	int typeIndex = m_pPed->IsLocalPlayer() ? Params::LOCAL_PLAYER : m_pPed->IsAPlayerPed() ? Params::REMOTE_PLAYER : Params::AI;

	if (typeIndex == Params::REMOTE_PLAYER)
	{
		for (int reactIndex = 0; reactIndex < MaxReacts; ++reactIndex)
		{
			if (m_aoReacts[reactIndex].bActive && !m_aoReacts[reactIndex].bLocalInflictor)
			{
				typeIndex = Params::REMOTE_REMOTE;
				break;
			}
		}
	}

	return typeIndex;
}

void CTorsoReactIkSolver::ClampAngles(Vector3& eulers, const Vector3& min, const Vector3& max)
{
	eulers.x = AngleClamp(eulers.x, min.x, max.x);
	eulers.y = AngleClamp(eulers.y, min.y, max.y);
	eulers.z = AngleClamp(eulers.z, min.z, max.z);
}

void CTorsoReactIkSolver::ClampAngles(Matrix34& m, const Vector3& min, const Vector3& max)
{
	Vector3 eulers;
	m.ToEulersXYZ(eulers);
	ClampAngles(eulers, min, max);
	m.FromEulersXYZ(eulers);
}
