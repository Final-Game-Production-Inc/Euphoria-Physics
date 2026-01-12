// 
// ik/solvers/BodyLookSolver.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "BodyLookSolver.h"

#include "diag/art_channel.h"
#include "fwdebug/picker.h"
#include "math/amath.h"

#include "animation/MovePed.h"
#include "camera/CamInterface.h"
#include "ik/solvers/BodyLookSolverProxy.h"
#include "peds/Ped.h"
#include "peds/PedIntelligence.h"
#include "scene/world/GameWorld.h"
#include "task/Motion/TaskMotionBase.h"

#if __IK_DEV
#include "ik/IkManager.h"
#endif // __IK_DEV

#define BODYLOOK_SOLVER_POOL_MAX		16
#define ENABLE_LOOKIK_SOLVER_WIDGETS	0

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CBodyLookIkSolver, BODYLOOK_SOLVER_POOL_MAX, 0.19f, atHashString("CBodyLookIkSolver",0xc50b0c6c));

ANIM_OPTIMISATIONS();

Vec3V CBodyLookIkSolver::ms_avTorsoDeltaLimits[LOOKIK_ROT_LIM_NUM][2] =		// [Min|Max][Yaw|Roll|Pitch]
{
	{ Vec3V(   0.0f,   0.0f,   0.00f), Vec3V(  0.0f,  0.0f, 0.00f) },		// LOOKIK_ROT_LIM_OFF
	{ Vec3V( -10.0f,  -2.5f,  -3.75f), Vec3V( 10.0f,  2.0f, 1.25f) },		// LOOKIK_ROT_LIM_NARROWEST
	{ Vec3V( -13.0f,  -5.0f,  -7.50f), Vec3V( 13.0f,  5.0f, 2.50f) },		// LOOKIK_ROT_LIM_NARROW
	{ Vec3V( -17.5f,  -7.5f, -11.25f), Vec3V( 17.5f,  7.5f, 3.75f) },		// LOOKIK_ROT_LIM_WIDE
	{ Vec3V( -20.0f, -10.0f, -15.00f), Vec3V( 20.0f, 10.0f, 5.00f) }		// LOOKIK_ROT_LIM_WIDEST
};

Vec3V CBodyLookIkSolver::ms_avNeckDeltaLimits[2][LOOKIK_ROT_LIM_NUM][2] =	// [Min|Max][Yaw|Roll|Pitch]
{
	{																			// Biped
		{ Vec3V(   0.00f,   0.00f,   0.00f), Vec3V(  0.00f,  0.00f,  0.00f) },	// LOOKIK_ROT_LIM_OFF
		{ Vec3V(  -5.00f,  -3.00f,  -5.00f), Vec3V(  5.00f,  3.00f,  5.00f) },	// LOOKIK_ROT_LIM_NARROWEST
		{ Vec3V( -10.00f,  -6.00f,  -8.00f), Vec3V( 10.00f,  6.00f,  8.00f) },	// LOOKIK_ROT_LIM_NARROW
		{ Vec3V( -15.00f, -10.00f, -15.00f), Vec3V( 15.00f, 10.00f, 15.00f) },	// LOOKIK_ROT_LIM_WIDE
		{ Vec3V( -20.00f, -15.00f, -20.00f), Vec3V( 20.00f, 15.00f, 20.00f) }	// LOOKIK_ROT_LIM_WIDEST
	},
	{																			// Quadruped
		{ Vec3V(   0.00f,   0.00f,   0.00f), Vec3V(  0.00f,  0.00f,  0.00f) },	// LOOKIK_ROT_LIM_OFF
		{ Vec3V( -50.00f,  -3.00f, -30.00f), Vec3V( 50.00f,  3.00f, 30.00f) },	// LOOKIK_ROT_LIM_NARROWEST
		{ Vec3V( -60.00f,  -6.00f, -40.00f), Vec3V( 60.00f,  6.00f, 40.00f) },	// LOOKIK_ROT_LIM_NARROW
		{ Vec3V( -70.00f, -10.00f, -50.00f), Vec3V( 70.00f, 10.00f, 50.00f) },	// LOOKIK_ROT_LIM_WIDE
		{ Vec3V( -80.00f, -15.00f, -60.00f), Vec3V( 80.00f, 15.00f, 60.00f) }	// LOOKIK_ROT_LIM_WIDEST
	}
};

Vec3V CBodyLookIkSolver::ms_avHeadDeltaLimits[2][LOOKIK_ROT_LIM_NUM][2] =	// [Min|Max][Yaw|Roll|Pitch]
{
	{																			// Biped
		{ Vec3V(   0.00f,   0.00f,   0.00f), Vec3V(  0.00f,  0.00f,  0.00f) },	// LOOKIK_ROT_LIM_OFF
		{ Vec3V(  -5.00f,  -2.00f,  -5.00f), Vec3V(  5.00f,  2.00f,  5.00f) },	// LOOKIK_ROT_LIM_NARROWEST
		{ Vec3V( -15.00f,  -5.00f, -15.00f), Vec3V( 15.00f,  5.00f, 15.00f) },	// LOOKIK_ROT_LIM_NARROW
		{ Vec3V( -30.00f, -10.00f, -30.00f), Vec3V( 30.00f, 10.00f, 30.00f) },	// LOOKIK_ROT_LIM_WIDE
		{ Vec3V( -45.00f, -15.00f, -45.00f), Vec3V( 45.00f, 15.00f, 45.00f) }	// LOOKIK_ROT_LIM_WIDEST
	},
	{																			// Quadruped
		{ Vec3V(   0.00f,   0.00f,   0.00f), Vec3V(  0.00f,  0.00f,  0.00f) },	// LOOKIK_ROT_LIM_OFF
		{ Vec3V( -10.00f,  -2.00f, -10.00f), Vec3V( 10.00f,  2.00f, 10.00f) },	// LOOKIK_ROT_LIM_NARROWEST
		{ Vec3V( -20.00f,  -5.00f, -20.00f), Vec3V( 20.00f,  5.00f, 20.00f) },	// LOOKIK_ROT_LIM_NARROW
		{ Vec3V( -30.00f, -10.00f, -30.00f), Vec3V( 30.00f, 10.00f, 30.00f) },	// LOOKIK_ROT_LIM_WIDE
		{ Vec3V( -40.00f, -15.00f, -35.00f), Vec3V( 40.00f, 15.00f, 35.00f) }	// LOOKIK_ROT_LIM_WIDEST
	}
};

Vec3V CBodyLookIkSolver::ms_avTorsoJointLimits[NUM_SPINE_PARTS][2] =		// [Min|Max][Yaw|Roll|Pitch]
{
	{ Vec3V( -15.0f, -10.0f, -15.0f), Vec3V( 15.0f,  10.0f, 15.0f) },		// SPINE0
	{ Vec3V( -15.0f, -10.0f, -15.0f), Vec3V( 15.0f,  10.0f, 15.0f) },		// SPINE1
	{ Vec3V( -15.0f, -10.0f, -15.0f), Vec3V( 15.0f,  10.0f, 15.0f) },		// SPINE2
	{ Vec3V( -20.0f, -15.0f, -20.0f), Vec3V( 20.0f,  15.0f, 20.0f) }		// SPINE3
};

Vec3V CBodyLookIkSolver::ms_avNeckJointLimits[2] =							// [Min|Max][Yaw|Roll|Pitch]
{
	Vec3V( -25.0f, -20.0f, -30.0f),
	Vec3V(  25.0f,  20.0f,  25.0f)
};

Vec3V CBodyLookIkSolver::ms_avHeadJointLimits[2] =							// [Min|Max][Yaw|Roll|Pitch]
{
	Vec3V( -35.0f, -30.0f,  -6.0f),
	Vec3V(  35.0f,  30.0f,  22.5f)
};

Vec3V CBodyLookIkSolver::ms_avEyeJointLimits[2] =							// [Min|Max][Yaw|Roll|Pitch]
{
	Vec3V( -20.0f, -180.0f, -12.0f),
	Vec3V(  20.0f,  180.0f,  15.0f)
};

float CBodyLookIkSolver::ms_afBlendRate[LOOKIK_BLEND_RATE_NUM] = 
{
	3.00f,																	// LOOKIK_BLEND_RATE_SLOWEST
	2.00f,																	// LOOKIK_BLEND_RATE_SLOW
	1.00f,																	// LOOKIK_BLEND_RATE_NORMAL
	0.50f,																	// LOOKIK_BLEND_RATE_FAST
	0.25f,																	// LOOKIK_BLEND_RATE_FASTEST
	0.00f																	// LOOKIK_BLEND_RATE_INSTANT
};

float CBodyLookIkSolver::ms_afSolverBlendRateScale[COMP_NUM] =
{
	  1.20f,																// COMP_TORSO
	  1.60f,																// COMP_NECK
	  1.50f,																// COMP_HEAD
	100.00f																	// COMP_EYE
};

float CBodyLookIkSolver::ms_afSpineWeight[NUM_SPINE_PARTS] =
{
	0.10f,																	// SPINE0
	0.20f,																	// SPINE1
	0.30f,																	// SPINE2
	0.40f																	// SPINE3
};

float CBodyLookIkSolver::ms_afHeadAttitudeWeight[LOOKIK_HEAD_ATT_NUM] = 
{
	0.00f,																	// LOOKIK_HEAD_ATT_OFF
	0.33f,																	// LOOKIK_HEAD_ATT_LOW
	0.66f,																	// LOOKIK_HEAD_ATT_MED
	1.00f																	// LOOKIK_HEAD_ATT_FULL
};

float CBodyLookIkSolver::ms_afCompSpring[COMP_NUM] = 
{
	 76.0f,																	// COMP_TORSO
	 56.0f,																	// COMP_NECK
	180.0f,																	// COMP_HEAD
	 76.0f																	// COMP_EYE
};

float CBodyLookIkSolver::ms_afCompDamping[COMP_NUM] =
{
	74.0f,																	// COMP_TORSO
	25.0f,																	// COMP_NECK
	48.0f,																	// COMP_HEAD
	10.0f																	// COMP_EYE
};

float CBodyLookIkSolver::ms_afCompInvMass[COMP_NUM] =
{
	0.1f,																	// COMP_TORSO
	0.2f,																	// COMP_NECK
	0.2f,																	// COMP_HEAD
	1.0f																	// COMP_EYE
};

float CBodyLookIkSolver::ms_afTargetSpringScale[LOOKIK_TURN_RATE_NUM] =
{
	0.7f,																	// LOOKIK_TURN_RATE_SLOW
	1.0f,																	// LOOKIK_TURN_RATE_NORMAL
	1.3f,																	// LOOKIK_TURN_RATE_FAST
	2.2f,																	// LOOKIK_TURN_RATE_FASTEST
	5.0f																	// LOOKIK_TURN_RATE_INSTANT
};

float CBodyLookIkSolver::ms_afRefDirBlendRate[COMP_NUM][2] =
{
	{ 0.6f, -0.6f },														// COMP_TORSO
	{ 1.0f, -0.8f },														// COMP_NECK
	{ 2.0f, -1.0f },														// COMP_HEAD
	{ 2.0f, -1.0f }															// COMP_EYE
};

float CBodyLookIkSolver::ms_afCompSpringRel						= 180.0f;
float CBodyLookIkSolver::ms_afCompDampingRel					= 48.0f;
float CBodyLookIkSolver::ms_afCompInvMassRel					= 0.2f;
float CBodyLookIkSolver::ms_fCharSpeedModifier					= 0.20f;
float CBodyLookIkSolver::ms_fCharAngSpeedModifier				= 1.00f;
float CBodyLookIkSolver::ms_afCharAngSpeedCounterModifier[2]	= { 3.8f, 0.3f };
float CBodyLookIkSolver::ms_fDeltaLimitBlendRate				= 40.0f;
float CBodyLookIkSolver::ms_fRollLimitBlendRate					= 1.25f;
float CBodyLookIkSolver::ms_afAngLimit[2]						= { 90.0f, 5.0f };
float CBodyLookIkSolver::ms_fEpsilon							= 0.0001f;

u8 CBodyLookIkSolver::ms_auRefDirWeight[2][16][4] =							// [COMP_TORSO|COMP_NECK|COMP_HEAD|COMP_EYE]
{
	{																			// Biped
		{   0,   0,   0,   0 },
		{ 100,   0,   0,   0 },													// COMP_TORSO
		{   0, 100,   0,   0 },													// COMP_NECK
		{  50,  50,   0,   0 },													// COMP_TORSO, COMP_NECK
		{   0,   0, 100,   0 },													// COMP_HEAD
		{  10,   0,  90,   0 },													// COMP_TORSO, COMP_HEAD
		{   0,  50,  50,   0 },													// COMP_NECK, COMP_HEAD
		{  10,  40,  50,   0 },													// COMP_TORSO, COMP_NECK, COMP_HEAD
		{   0,   0,   0, 100 },													// COMP_EYE
		{  40,   0,   0,  60 },													// COMP_TORSO, COMP_EYE
		{   0,  20,   0,  80 },													// COMP_NECK, COMP_EYE
		{  20,  10,   0,  70 },													// COMP_TORSO, COMP_NECK, COMP_EYE
		{   0,   0,  50,  50 },													// COMP_HEAD, COMP_EYE
		{  10,   0,  40,  50 },													// COMP_TORSO, COMP_HEAD, COMP_EYE
		{   0,  30,  30,  40 },													// COMP_NECK, COMP_HEAD, COMP_EYE
		{  10,  30,  30,  30 }													// COMP_TORSO, COMP_NECK, COMP_HEAD, COMP_EYE
	},
	{																			// Quadruped
		{   0,   0,   0,   0 },
		{   0,   0,   0,   0 },													// COMP_TORSO
		{   0, 100,   0,   0 },													// COMP_NECK
		{   0, 100,   0,   0 },													// COMP_TORSO, COMP_NECK
		{   0,   0, 100,   0 },													// COMP_HEAD
		{   0,   0, 100,   0 },													// COMP_TORSO, COMP_HEAD
		{   0,  70,  30,   0 },													// COMP_NECK, COMP_HEAD
		{   0,  70,  30,   0 },													// COMP_TORSO, COMP_NECK, COMP_HEAD
		{   0,   0,   0,   0 },													// COMP_EYE
		{   0,   0,   0,   0 },													// COMP_TORSO, COMP_EYE
		{   0, 100,   0,   0 },													// COMP_NECK, COMP_EYE
		{   0, 100,   0,   0 },													// COMP_TORSO, COMP_NECK, COMP_EYE
		{   0,   0, 100,   0 },													// COMP_HEAD, COMP_EYE
		{   0,   0, 100,   0 },													// COMP_TORSO, COMP_HEAD, COMP_EYE
		{   0,  70,  30,   0 },													// COMP_NECK, COMP_HEAD, COMP_EYE
		{   0,  70,  30,   0 }													// COMP_TORSO, COMP_NECK, COMP_HEAD, COMP_EYE
	}
};

#if __IK_DEV
CDebugRenderer CBodyLookIkSolver::ms_debugHelper;
bool CBodyLookIkSolver::ms_bRenderDebug				= false;
bool CBodyLookIkSolver::ms_bRenderFacing			= false;
bool CBodyLookIkSolver::ms_bRenderMotion			= false;
int CBodyLookIkSolver::ms_iNoOfTexts				= 0;
#endif //__IK_DEV
#if __IK_DEV || __BANK
bool CBodyLookIkSolver::ms_bRenderTargetPosition	= false;
#endif // __IK_DEV || __BANK

CBodyLookIkSolver::CBodyLookIkSolver(CPed* pPed, CBodyLookIkSolverProxy* pProxy)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pProxy(pProxy)
, m_pPed(pPed)
{
	ikAssert(m_pProxy);

	const CBaseCapsuleInfo* pCapsuleInfo = m_pPed->GetCapsuleInfo();
	m_uBipedQuadruped = !pCapsuleInfo || pCapsuleInfo->IsBiped() ? 0 : 1;

	bool bBonesValid = true;

	// Spine
	const eAnimBoneTag aeSpineBoneTags[] = { BONETAG_SPINE0, BONETAG_SPINE1, BONETAG_SPINE2, BONETAG_SPINE3 };
	for (int spineBoneTagIndex = 0; spineBoneTagIndex < NUM_SPINE_PARTS; ++spineBoneTagIndex)
	{
		m_auSpineBoneIdx[spineBoneTagIndex] = (u16)m_pPed->GetBoneIndexFromBoneTag(aeSpineBoneTags[spineBoneTagIndex]);
		bBonesValid &= (m_auSpineBoneIdx[spineBoneTagIndex] != (u16)-1);
	}

	// Eyes
	m_auEyeBoneIdx[LEFT_EYE]  = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_EYE);
	m_auEyeBoneIdx[RIGHT_EYE] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_EYE);
	m_bAlternateEyeRig = false;

	if ((m_auEyeBoneIdx[LEFT_EYE] == (u16)-1) || (m_auEyeBoneIdx[RIGHT_EYE] == (u16)-1))
	{
		// Not found, try second set since bone tag names don't seem to be consistent across rigs
		m_auEyeBoneIdx[LEFT_EYE]  = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_EYE2);
		m_auEyeBoneIdx[RIGHT_EYE] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_EYE2);
		m_bAlternateEyeRig = true;
	}

	// Upper body
	m_uHeadBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);
	m_uNeckBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);
	m_uLookBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_LOOK_DIR);
	m_uFacingBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_FACING_DIR);
	m_uSpineRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE_ROOT);
	m_uRootBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	m_auUpperArmBoneIdx[LEFT_ARM] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_UPPERARM);
	m_auUpperArmBoneIdx[RIGHT_ARM] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_UPPERARM);
	m_auClavicleBoneIdx[LEFT_ARM] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_CLAVICLE);
	m_auClavicleBoneIdx[RIGHT_ARM] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_CLAVICLE);

	bBonesValid &= (m_uHeadBoneIdx != (u16)-1) &&
				   (m_uNeckBoneIdx != (u16)-1) &&
				   (m_uLookBoneIdx != (u16)-1) &&
				   (m_uFacingBoneIdx != (u16)-1) &&
				   (m_uSpineRootBoneIdx != (u16)-1) &&
				   (m_auUpperArmBoneIdx[LEFT_ARM] != (u16)-1) &&
				   (m_auUpperArmBoneIdx[RIGHT_ARM] != (u16)-1) &&
				   (m_auClavicleBoneIdx[LEFT_ARM] != (u16)-1) &&
				   (m_auClavicleBoneIdx[RIGHT_ARM] != (u16)-1);

	m_bBonesValid = bBonesValid;
	artAssertf(bBonesValid, "Modelname = (%s) is missing bones necessary for body look ik. Expects the following bones: "
							"BONETAG_SPINE[0..3], BONETAG_L_EYE, BONETAG_R_EYE, BONETAG_HEAD, BONETAG_NECK, BONETAG_LOOK_DIR (ik_head), BONETAG_FACING_DIR (ik_root), "
							"BONETAG_SPINE_ROOT, BONETAG_ROOT, BONETAG_L_UPPERARM, BONETAG_R_UPPERARM, BONETAG_L_CLAVICLE, BONETAG_R_CLAVICLE", pPed ? pPed->GetModelName() : "Unknown model");

	// Bone counts
	m_uNeckBoneCount = 0;

	if (m_uHeadBoneIdx != (u16)-1)
	{
		crSkeleton& skeleton = *m_pPed->GetSkeleton();
		const s16* aParentIndices = skeleton.GetSkeletonData().GetParentIndices();
		u16 uBoneIdx = m_uHeadBoneIdx;

		while (aParentIndices[uBoneIdx] != m_auSpineBoneIdx[SPINE3])
		{
			m_uNeckBoneCount++;
			uBoneIdx = aParentIndices[uBoneIdx];
		}
	}

	SetPrority(8);
	Reset();
}

void CBodyLookIkSolver::Reset()
{
	ikAssert(m_pPed);
	const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

	m_avDeltaLimits[COMP_TORSO][0] = ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[0][0];//ms_avTorsoDeltaLimits[LOOKIK_ROT_LIM_OFF][0];
	m_avDeltaLimits[COMP_TORSO][1] = ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[0][1];//ms_avTorsoDeltaLimits[LOOKIK_ROT_LIM_OFF][1];
	m_avDeltaLimits[COMP_NECK][0] = ikSettings.GetNeckDeltaLimits().m_DeltaLimits[0][0];//ms_avNeckDeltaLimits[m_uBipedQuadruped][LOOKIK_ROT_LIM_OFF][0];
	m_avDeltaLimits[COMP_NECK][1] = ikSettings.GetNeckDeltaLimits().m_DeltaLimits[0][1];//ms_avNeckDeltaLimits[m_uBipedQuadruped][LOOKIK_ROT_LIM_OFF][1];
	m_avDeltaLimits[COMP_HEAD][0] = ikSettings.GetHeadDeltaLimits().m_DeltaLimits[0][0];//ms_avHeadDeltaLimits[m_uBipedQuadruped][LOOKIK_ROT_LIM_OFF][0];
	m_avDeltaLimits[COMP_HEAD][1] = ikSettings.GetHeadDeltaLimits().m_DeltaLimits[0][1];//ms_avHeadDeltaLimits[m_uBipedQuadruped][LOOKIK_ROT_LIM_OFF][1];

	m_afSolverBlendRate[0] = ms_afBlendRate[LOOKIK_BLEND_RATE_NORMAL];
	m_afSolverBlendRate[1] = ms_afBlendRate[LOOKIK_BLEND_RATE_NORMAL];

	m_afRollLimitScale[0] = 1.0f;
	m_afRollLimitScale[1] = 1.0f;

	m_fPrevHeading = TWO_PI;

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		m_afSolverBlend[compIndex] = 0.0f;
		m_afRefDirBlend[compIndex] = (m_uBipedQuadruped == 0) ? 1.0f : 0.0f;

		m_aeRefDir[compIndex] = LOOKIK_REF_DIR_ABSOLUTE;
		m_auRefDirWeight[compIndex] = 255;
	}

	m_eTurnRate = LOOKIK_TURN_RATE_NORMAL;
	m_eHeadAttitude = LOOKIK_HEAD_ATT_FULL;

	const u32 uRotLimSize = (COMP_NUM - 1) * LOOKIK_ROT_ANGLE_NUM;
	memset(m_aeRotLim, LOOKIK_ROT_LIM_OFF, uRotLimSize);
	memset(m_aeRotLimPrev, LOOKIK_ROT_LIM_OFF, uRotLimSize);
	memset(m_aeRotLimPend, LOOKIK_ROT_LIM_OFF, uRotLimSize);

	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		m_aeArmComp[armIndex] = LOOKIK_ARM_COMP_OFF;
		m_aeArmCompPrev[armIndex] = LOOKIK_ARM_COMP_OFF;
		m_aeArmCompPend[armIndex] = LOOKIK_ARM_COMP_OFF;

		m_afArmCompBlend[armIndex] = 1.0f;
	}

	m_bInit = true;
	m_bRefDirFacing = false;
	m_bAiming = false;

	ResetTargetPositions();
}

void CBodyLookIkSolver::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skeleton))
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyLook] || CBaseIkManager::ms_DisableAllSolvers)
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
		sprintf(debugText, "%s\n", "BodyLookSolver::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	//Update();
}

void CBodyLookIkSolver::PreIkUpdate(float UNUSED_PARAM(deltaTime))
{
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyLook] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

	// Cache state
	m_bAiming = m_pPed->GetPedResetFlag(CPED_RESET_FLAG_IsAiming);
}

void CBodyLookIkSolver::PostIkUpdate(float deltaTime)
{
	// Called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyLook] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "%s\n", "CBodyLookIkSolver::PostIkUpdate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //_DEV

	Update(deltaTime);
}

void CBodyLookIkSolver::Request(const CIkRequestBodyLook& request)
{
	if (!(m_pProxy->GetFlags() & LOOKIK_USE_ANIM_TAGS))
	{
		m_aeRefDir[COMP_TORSO] = (u8)request.GetRefDirTorso();
		m_aeRefDir[COMP_NECK] = (u8)request.GetRefDirNeck();
		m_aeRefDir[COMP_HEAD] = (u8)request.GetRefDirHead();
		m_aeRefDir[COMP_EYE] = (u8)request.GetRefDirEye();

		m_eTurnRate = (u8)request.GetTurnRate();
		m_eHeadAttitude = (u8)request.GetHeadAttitude();

		m_afSolverBlendRate[0] = (ms_afBlendRate[request.GetBlendInRate()] != 0.0f) ? 1.0f / ms_afBlendRate[request.GetBlendInRate()] : 1000.0f;
		m_afSolverBlendRate[1] = (ms_afBlendRate[request.GetBlendOutRate()] != 0.0f) ? 1.0f / ms_afBlendRate[request.GetBlendOutRate()] : 1000.0f;

		// Copy requested values to pending
		memcpy(m_aeRotLimPend, request.m_auRotLim, (COMP_NUM - 1) * LOOKIK_ROT_ANGLE_NUM);
		m_aeArmCompPend[LEFT_ARM] = request.m_auArmComp[LEFT_ARM];
		m_aeArmCompPend[RIGHT_ARM] = request.m_auArmComp[RIGHT_ARM];
	}
}

void CBodyLookIkSolver::Update(float deltaTime)
{
	if (!IsValid())
	{
		m_pProxy->SetComplete(true);
		return;
	}

	if (fwTimer::IsGamePaused() || m_pPed->m_nDEflags.bFrozenByInterior || m_pPed->GetUsingRagdoll() || m_pPed->GetDisableBodyLookSolver() || m_pPed->GetDisableHeadSolver())
	{
#if ENABLE_DRUNK
		if (m_pPed->GetUsingRagdoll() && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_IsDrunk))
		{
			// If we're running a drunk behaviour, keep the solver target updated so the drunk NM task can pass the updated look target to NM.
			m_pProxy->UpdateTargetPosition();
			return;
		}
		else 
#endif // ENABLE_DRUNK
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableBodyLookSolver() || m_pPed->GetDisableHeadSolver())
		{
			Reset();
			m_pProxy->SetComplete(true);
		}

		return;
	}

	crSkeleton& skeleton = *m_pPed->GetSkeleton();
	SolverContext context;

	context.m_fDeltaTime = deltaTime;

	// Store distance sq to camera
	Vec3V vPedToCamera(Subtract(VECTOR3_TO_VEC3V(camInterface::GetPos()), m_pPed->GetTransform().GetPosition()));
	ScalarV sMagSquared(MagSquared(vPedToCamera));
	context.m_fDistSqToCamera = sMagSquared.Getf();

	// Facing dir is mover dir rotated by delta stored in facing dir bone (IK_Root)
	context.m_mtxReferenceDir = m_pPed->GetTransform().GetMatrix();
	Transform(context.m_mtxReferenceDir, context.m_mtxReferenceDir, skeleton.GetLocalMtx(m_uFacingBoneIdx));

	if (!UpdateBlend(context))
	{
		return;
	}

	context.m_vTargetPosition = ClampTargetPosition(context);
	context.m_vEulersAdditive = Vec3V(V_ZERO);
	context.m_vEulersAdditiveComp = Vec3V(V_ZERO);

	// Interpolate target positions for each component
	InterpolateTargetPositions(skeleton, context);

	// Calculate relative angles if any component's reference direction is set to relative
	if (context.m_bRefDirRelative)
	{
		CalculateRelativeAngles(skeleton, context);
	}
	else
	{
		// Reset
		memset(m_auRefDirWeight, 255, COMP_NUM);
		m_bRefDirFacing = false;
	}

	SolveTorso(skeleton, context);
	SolveNeck(skeleton, context);
	SolveHead(skeleton, context);
	SolveEyes(skeleton, context);

#if __IK_DEV || __BANK
	if (ms_bRenderTargetPosition IK_DEV_ONLY(&& !CaptureDebugRender()))
	{
		Mat34V mtxHeadGlobal;
		m_pPed->GetSkeleton()->GetGlobalMtx(m_uHeadBoneIdx, mtxHeadGlobal);
		grcDebugDraw::Line(mtxHeadGlobal.GetCol3(), m_pProxy->GetTargetPosition(), m_pPed->IsLocalPlayer() ? Color_SteelBlue : (!(m_pProxy->GetFlags() & LOOKIK_USE_ANIM_TAGS) ? Color_yellow : Color_orange));
	}
#endif // __IK_DEV || __BANK
}

bool CBodyLookIkSolver::UpdateBlend(SolverContext& context)
{
	m_pProxy->UpdateActiveState();

	// Update settings and check for tags
	UpdateSettings();

	// FOV
	if (m_pProxy->GetFlags() & LOOKIK_USE_FOV)
	{
		Vec3V vToTarget(Subtract(m_pProxy->GetTargetPosition(), context.m_mtxReferenceDir.GetCol3()));
		vToTarget = NormalizeFast(vToTarget);

		if (IsLessThanAll(Dot(vToTarget, context.m_mtxReferenceDir.GetCol1()), ScalarV(V_ZERO)))
		{
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_FOV, "CBodyLookIkSolver::UpdateBlend");
			m_pProxy->SetActive(false);
		}
	}

	// Forced abort
	if (m_pProxy->IsAborted() || m_pProxy->IsBlocked())
	{
		IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_ABORT, "CBodyLookIkSolver::UpdateBlend");
		m_pProxy->SetActive(false);
	}

#if __IK_DEV || __BANK
	if (!m_pProxy->IsActive() && ms_bRenderTargetPosition IK_DEV_ONLY(&& !CaptureDebugRender()))
	{
		Mat34V mtxHeadGlobal;
		m_pPed->GetSkeleton()->GetGlobalMtx(m_uHeadBoneIdx, mtxHeadGlobal);
		grcDebugDraw::Line(mtxHeadGlobal.GetCol3(), m_pProxy->GetTargetPosition(), Color_red);
	}
#endif // __IK_DEV || __BANK

	// Active components
	context.m_bRefDirRelative = false;

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		context.m_auActive[compIndex] = (((compIndex != COMP_EYE) && ((m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_YAW] != LOOKIK_ROT_LIM_OFF) || (m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_PITCH] != LOOKIK_ROT_LIM_OFF))) ||
										 ((compIndex == COMP_EYE) && (m_aeRefDir[COMP_EYE] != LOOKIK_REF_DIR_HEAD))) != 0;

		context.m_auRefDirRel[compIndex] = (((m_aeRefDir[compIndex] == LOOKIK_REF_DIR_FACING) || (m_aeRefDir[compIndex] == LOOKIK_REF_DIR_MOVER)) &&
										    ((compIndex == COMP_EYE) || ((m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_YAW] != LOOKIK_ROT_LIM_OFF) || (m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_PITCH] != LOOKIK_ROT_LIM_OFF)))) != 0;

		context.m_bRefDirRelative |= (context.m_auRefDirRel[compIndex] != 0);
	}

	// Update blends
	const bool bActive = m_pProxy->IsActive();
	const bool bInstant = m_pProxy->m_Request.IsInstant();
	float fTimeStep = context.m_fDeltaTime;

	// Reference direction blend
	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		const float fRefDirBlendRate = ms_afRefDirBlendRate[compIndex][(context.m_auRefDirRel[compIndex] != 0) ? 1 : 0];

		m_afRefDirBlend[compIndex] += fTimeStep * fRefDirBlendRate;
		m_afRefDirBlend[compIndex] = Clamp(m_afRefDirBlend[compIndex], 0.0f, 1.0f);

		context.m_bRefDirRelative |= (m_afRefDirBlend[compIndex] < 1.0f);
	}

	// Arm compensation blend (double the torso blend in rate)
	const float fArmCompBlendRate = !bInstant ? (m_afSolverBlendRate[0] * ms_afSolverBlendRateScale[COMP_TORSO] * 2.0f) : 1000.0f;
	const float fArmCompBlendDelta = fTimeStep * fArmCompBlendRate;
	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		m_afArmCompBlend[armIndex] += fArmCompBlendDelta;
		m_afArmCompBlend[armIndex] = Clamp(m_afArmCompBlend[armIndex], 0.0f, 1.0f);
	}

	// Interpolate delta limits
	Vec3V vDeltaMax(ScalarV(ms_fDeltaLimitBlendRate * fTimeStep));
	Vec3V vZero(V_ZERO);
	Vec3V vOne(V_ONE);

	ikAssert(m_pPed);
	const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

	for (int compIndex = 0; compIndex < COMP_EYE; ++compIndex)
	{
		u8 uYLimit = m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_YAW];
		u8 uPLimit = m_aeRotLim[compIndex][LOOKIK_ROT_ANGLE_PITCH];

		for (int limitIndex = 0; limitIndex < 2; ++limitIndex)
		{
			Vec3V vTarget;

			if (compIndex == COMP_TORSO)
			{
				vTarget = Vec3V(ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][0], ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][1], ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[uPLimit][limitIndex][2]);
				//vTarget = Vec3V(ms_avTorsoDeltaLimits[uYLimit][limitIndex][0], ms_avTorsoDeltaLimits[uYLimit][limitIndex][1], ms_avTorsoDeltaLimits[uPLimit][limitIndex][2]);
			}
			else if (compIndex == COMP_NECK)
			{
				vTarget = Vec3V(ikSettings.GetNeckDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][0], ikSettings.GetNeckDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][1], ikSettings.GetNeckDeltaLimits().m_DeltaLimits[uPLimit][limitIndex][2]);
				//vTarget = Vec3V(ms_avNeckDeltaLimits[m_uBipedQuadruped][uYLimit][limitIndex][0], ms_avNeckDeltaLimits[m_uBipedQuadruped][uYLimit][limitIndex][1], ms_avNeckDeltaLimits[m_uBipedQuadruped][uPLimit][limitIndex][2]);
			}
			else
			{
				vTarget = Vec3V(ikSettings.GetHeadDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][0], ikSettings.GetHeadDeltaLimits().m_DeltaLimits[uYLimit][limitIndex][1], ikSettings.GetHeadDeltaLimits().m_DeltaLimits[uPLimit][limitIndex][2]);
				//vTarget = Vec3V(ms_avHeadDeltaLimits[m_uBipedQuadruped][uYLimit][limitIndex][0], ms_avHeadDeltaLimits[m_uBipedQuadruped][uYLimit][limitIndex][1], ms_avHeadDeltaLimits[m_uBipedQuadruped][uPLimit][limitIndex][2]);
			}

			if (bActive && context.m_auActive[compIndex])
			{
				if (!bInstant)
				{
					Vec3V vDelta(Subtract(vTarget, m_avDeltaLimits[compIndex][limitIndex]));

					Vec3V vInterp(InvertSafe(Abs(vDelta), vZero));
					vInterp = Scale(vInterp, vDeltaMax);
					vInterp = Min(vInterp, vOne);

					m_avDeltaLimits[compIndex][limitIndex] = Lerp(vInterp, m_avDeltaLimits[compIndex][limitIndex], vTarget);
				}
				else
				{
					m_avDeltaLimits[compIndex][limitIndex] = vTarget;
				}
			}
			else if (m_afSolverBlend[compIndex] == 0.0f)
			{
				// Only set limits to target once component has fully blended out (ie. hold last limits while component is blending out)
				m_avDeltaLimits[compIndex][limitIndex] = vTarget;
			}
		}
	}

	// Solver blend
	const float fBlendInRate = !bInstant ? m_afSolverBlendRate[0] : 1000.0f;
	const float fBlendOutRate = -m_afSolverBlendRate[1];
	bool bBlendedOut = true;

	bool abCanUseComponent[COMP_NUM];
	CanUseComponents(context, abCanUseComponent);

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		const float fDelta = fTimeStep * ((bActive && abCanUseComponent[compIndex] && context.m_auActive[compIndex]) ? fBlendInRate : fBlendOutRate);

		m_afSolverBlend[compIndex] += fDelta * ms_afSolverBlendRateScale[compIndex];
		m_afSolverBlend[compIndex] = Clamp(m_afSolverBlend[compIndex], 0.0f, 1.0f);
		bBlendedOut &= (m_afSolverBlend[compIndex] == 0.0f);
	}

	if (bBlendedOut && ((m_pProxy->GetHoldTime() != (u16)-1) || m_pProxy->IsAborted() || m_pProxy->IsBlocked()))
	{
		Reset();
		m_pProxy->SetComplete(true);
		return false;
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "Aiming : %d", m_bAiming ? 1 : 0);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "FLAGS  : %s %s\n", (m_pProxy->GetFlags() & LOOKIK_USE_ANIM_TAGS) ? "LOOKIK_USE_ANIM_TAGS" : "", (m_pProxy->GetFlags() & LOOKIK_SCRIPT_REQUEST) ? "LOOKIK_SCRIPT_REQUEST" : "");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "BLEND   TNHE: %4.2f, %4.2f, %4.2f, %4.2f\n", m_afSolverBlend[COMP_TORSO], m_afSolverBlend[COMP_NECK], m_afSolverBlend[COMP_HEAD], m_afSolverBlend[COMP_EYE]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "REFDIR  TNHE: %4.2f, %4.2f, %4.2f, %4.2f\n", m_afRefDirBlend[COMP_TORSO], m_afRefDirBlend[COMP_NECK], m_afRefDirBlend[COMP_HEAD], m_afRefDirBlend[COMP_EYE]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "AC BLEND  LR: %4.2f, %4.2f\n", m_afArmCompBlend[LEFT_ARM], m_afArmCompBlend[RIGHT_ARM]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "DL TORSO YRP: (%6.1f, %6.1f, %6.1f) (%6.1f, %6.1f, %6.1f)\n", m_avDeltaLimits[COMP_TORSO][0].GetXf(), 
																						  m_avDeltaLimits[COMP_TORSO][0].GetYf(), 
																						  m_avDeltaLimits[COMP_TORSO][0].GetZf(),
																						  m_avDeltaLimits[COMP_TORSO][1].GetXf(), 
																						  m_avDeltaLimits[COMP_TORSO][1].GetYf(), 
																						  m_avDeltaLimits[COMP_TORSO][1].GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "DL NECK  YRP: (%6.1f, %6.1f, %6.1f) (%6.1f, %6.1f, %6.1f)\n", m_avDeltaLimits[COMP_NECK][0].GetXf(), 
																						  m_avDeltaLimits[COMP_NECK][0].GetYf(), 
																						  m_avDeltaLimits[COMP_NECK][0].GetZf(),
																						  m_avDeltaLimits[COMP_NECK][1].GetXf(), 
																						  m_avDeltaLimits[COMP_NECK][1].GetYf(), 
																						  m_avDeltaLimits[COMP_NECK][1].GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "DL HEAD  YRP: (%6.1f, %6.1f, %6.1f) (%6.1f, %6.1f, %6.1f)\n", m_avDeltaLimits[COMP_HEAD][0].GetXf(), 
																						  m_avDeltaLimits[COMP_HEAD][0].GetYf(), 
																						  m_avDeltaLimits[COMP_HEAD][0].GetZf(),
																						  m_avDeltaLimits[COMP_HEAD][1].GetXf(), 
																						  m_avDeltaLimits[COMP_HEAD][1].GetYf(), 
																						  m_avDeltaLimits[COMP_HEAD][1].GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	return true;
}

void CBodyLookIkSolver::UpdateSettings()
{
	fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
	u32 uFlags = m_pProxy->GetFlags();

	bool bUseAnimTags = (uFlags & LOOKIK_USE_ANIM_TAGS) != 0;
	bool bBlockAnimTags = (uFlags & LOOKIK_BLOCK_ANIM_TAGS) != 0;
	bool bEyesOnlyLookAt = (m_pProxy->m_Request.GetLookAtFlags() != -1) && ((m_pProxy->m_Request.GetLookAtFlags() & LF_USE_EYES_ONLY) != 0);
	bool bSettingsUpdated = false;
	bool bReactivate = false;

	if (pAnimDirector && !bBlockAnimTags)
	{
		const CClipEventTags::CLookIKEventTag* pProp = static_cast<const CClipEventTags::CLookIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LookIk));
		ikAssertf(!pProp || (pProp->GetKey() == CClipEventTags::LookIk.GetHash()), "Invalid property from move network. Expecting LookIk key %08X, but got key %08X", CClipEventTags::LookIk.GetHash(), pProp->GetKey());

		if (pProp && (pProp->GetKey() == CClipEventTags::LookIk.GetHash()) && !bEyesOnlyLookAt)
		{
			// Override original settings with anim properties
			m_aeRefDir[COMP_TORSO]	= pProp->GetTorsoReferenceDirection();
			m_aeRefDir[COMP_NECK]	= pProp->GetNeckReferenceDirection();
			m_aeRefDir[COMP_HEAD]	= pProp->GetHeadReferenceDirection();
			m_aeRefDir[COMP_EYE]	= pProp->GetEyeReferenceDirection();

			m_eTurnRate = pProp->GetTurnSpeed();
			m_eHeadAttitude = pProp->GetHeadAttitude();

			m_afSolverBlendRate[0] = (ms_afBlendRate[pProp->GetBlendInSpeed()] != 0.0f) ? 1.0f / ms_afBlendRate[pProp->GetBlendInSpeed()] : 1000.0f;
			m_afSolverBlendRate[1] = (ms_afBlendRate[pProp->GetBlendOutSpeed()] != 0.0f) ? 1.0f / ms_afBlendRate[pProp->GetBlendOutSpeed()] : 1000.0f;

			m_aeRotLimPend[COMP_TORSO][LOOKIK_ROT_ANGLE_YAW]	= pProp->GetTorsoYawDeltaLimit();
			m_aeRotLimPend[COMP_TORSO][LOOKIK_ROT_ANGLE_PITCH]	= pProp->GetTorsoPitchDeltaLimit();
			m_aeRotLimPend[COMP_NECK][LOOKIK_ROT_ANGLE_YAW]		= pProp->GetNeckYawDeltaLimit();
			m_aeRotLimPend[COMP_NECK][LOOKIK_ROT_ANGLE_PITCH]	= pProp->GetNeckPitchDeltaLimit();
			m_aeRotLimPend[COMP_HEAD][LOOKIK_ROT_ANGLE_YAW]		= pProp->GetHeadYawDeltaLimit();
			m_aeRotLimPend[COMP_HEAD][LOOKIK_ROT_ANGLE_PITCH]	= pProp->GetHeadPitchDeltaLimit();

			m_aeArmCompPend[LEFT_ARM]  = pProp->GetLeftArmCompensation();
			m_aeArmCompPend[RIGHT_ARM] = pProp->GetRightArmCompensation();

			bSettingsUpdated = true;
		}
		else if (bUseAnimTags && !bEyesOnlyLookAt)
		{
			// No tag exists, so disable all components if driving settings directly with anim tags
			memset(m_aeRotLimPend, LOOKIK_ROT_LIM_OFF, (COMP_NUM - 1) * LOOKIK_ROT_ANGLE_NUM);
			m_aeRefDir[COMP_EYE] = LOOKIK_REF_DIR_HEAD;

			m_aeArmCompPend[LEFT_ARM] = LOOKIK_ARM_COMP_OFF;
			m_aeArmCompPend[RIGHT_ARM] = LOOKIK_ARM_COMP_OFF;

			// Check if all components have blended out
			bool bBlendedOut = true;

			for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
			{
				if (m_afSolverBlend[compIndex] > 0.0f)
				{
					bBlendedOut = false;
					break;
				}
			}

			if (bBlendedOut)
			{
				// Once all components have blended out, allow the solver to re-init when a new tag comes in
				bReactivate = true;
			}
		}

		// Check for block tag
		const CClipEventTags::CLookIKControlEventTag* pPropControl = static_cast<const CClipEventTags::CLookIKControlEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::LookIkControl));

		if (pPropControl && !pPropControl->GetAllowed())
		{
			IK_DEV_SET_LOOK_AT_RETURN_CODE(LOOKIK_RC_BLOCKED_BY_TAG, "CBodyLookIkSolver::UpdateSettings");
			m_pProxy->SetActive(false);
		}
	}

	// Ignore/force specific settings for quadrupeds
	if (m_uBipedQuadruped == 1)
	{
		m_aeRefDir[COMP_TORSO]	= LOOKIK_REF_DIR_FACING;
		m_aeRefDir[COMP_NECK]	= LOOKIK_REF_DIR_FACING;
		m_aeRefDir[COMP_HEAD]	= LOOKIK_REF_DIR_FACING;
		m_aeRefDir[COMP_EYE]	= LOOKIK_REF_DIR_HEAD;

		m_aeRotLimPend[COMP_TORSO][LOOKIK_ROT_ANGLE_YAW]	= LOOKIK_ROT_LIM_OFF;
		m_aeRotLimPend[COMP_TORSO][LOOKIK_ROT_ANGLE_PITCH]	= LOOKIK_ROT_LIM_OFF;

		m_aeArmCompPend[LEFT_ARM]	= LOOKIK_ARM_COMP_OFF;
		m_aeArmCompPend[RIGHT_ARM]	= LOOKIK_ARM_COMP_OFF;
	}

	// Set and track previous setting
	for (int compIndex = 0; compIndex < COMP_EYE; ++compIndex)
	{
		for (int rotAngIndex = 0; rotAngIndex < LOOKIK_ROT_ANGLE_NUM; ++rotAngIndex)
		{
			// Check if pending setting is different from current
			if (m_aeRotLimPend[compIndex][rotAngIndex] != m_aeRotLim[compIndex][rotAngIndex])
			{
				m_aeRotLimPrev[compIndex][rotAngIndex] = m_aeRotLim[compIndex][rotAngIndex];
				m_aeRotLim[compIndex][rotAngIndex] = m_aeRotLimPend[compIndex][rotAngIndex];
			}
		}
	}

	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		// Check if pending setting is different from current and re-calculate current blend
		if (m_aeArmCompPend[armIndex] != m_aeArmComp[armIndex])
		{
			float fArmCompWeight = Lerp(m_afArmCompBlend[armIndex], ms_afHeadAttitudeWeight[m_aeArmCompPrev[armIndex]], ms_afHeadAttitudeWeight[m_aeArmComp[armIndex]]);

			m_aeArmCompPrev[armIndex] = m_aeArmComp[armIndex];

			m_afArmCompBlend[armIndex] = (fArmCompWeight - ms_afHeadAttitudeWeight[m_aeArmCompPrev[armIndex]]) / (ms_afHeadAttitudeWeight[m_aeArmCompPend[armIndex]] - ms_afHeadAttitudeWeight[m_aeArmCompPrev[armIndex]]);
			m_afArmCompBlend[armIndex] = Clamp(m_afArmCompBlend[armIndex], 0.0f, 1.0f);

			m_aeArmComp[armIndex] = m_aeArmCompPend[armIndex];
		}
	}

	if (m_bInit && (!bUseAnimTags || bSettingsUpdated))
	{
		ResetOnActivate();
		m_bInit = false;
	}

	if (bReactivate)
	{
		m_bInit = true;
	}
}

bool CBodyLookIkSolver::SolveTorso(rage::crSkeleton& skeleton, SolverContext& context)
{
	IK_DEV_ONLY(char debugText[64];)

	if (m_afSolverBlend[COMP_TORSO] > 0.0f)
	{
	#if __IK_DEV
		if (CaptureDebugRender())
		{
			sprintf(debugText, "- TORSO --------------------------------------\n");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif //__IK_DEV

		const bool bRelativeDirMode = m_afRefDirBlend[COMP_TORSO] < 1.0f;
		const bool bAbsoluteMode	= m_afRefDirBlend[COMP_TORSO] > 0.0f;

		const ScalarV vToRadians(V_TO_RADIANS);

		Vec3V vMinDeltaLimit(m_avDeltaLimits[COMP_TORSO][0].GetXY(), Negate(m_avDeltaLimits[COMP_TORSO][1].GetZ()));
		Vec3V vMaxDeltaLimit(m_avDeltaLimits[COMP_TORSO][1].GetXY(), Negate(m_avDeltaLimits[COMP_TORSO][0].GetZ()));
		vMinDeltaLimit = Scale(vMinDeltaLimit, vToRadians);
		vMaxDeltaLimit = Scale(vMaxDeltaLimit, vToRadians);

		QuatV qReferenceToTarget(V_IDENTITY);

		if (bRelativeDirMode)
		{
			CalculateRelativeAngleRotation(COMP_TORSO, vMinDeltaLimit, vMaxDeltaLimit, context, qReferenceToTarget);
		}

		if (bAbsoluteMode)
		{
			const Mat34V& mtxSpineRoot = skeleton.GetObjectMtx(m_uSpineRootBoneIdx);
			const Mat34V& mtxHead = skeleton.GetObjectMtx(m_uHeadBoneIdx);

			Vec3V vReferenceToTarget(Subtract(context.m_avTargetPosition[COMP_TORSO], mtxHead.GetCol3()));
			vReferenceToTarget = Normalize(vReferenceToTarget);

			// Generate rotation from reference spine direction to target (to preserve any roll in the reference direction)
			QuatV qReferenceToTargetAbsolute(QuatVFromVectors(mtxSpineRoot.GetCol1(), vReferenceToTarget));

			// Cross fade if necessary
			if (bRelativeDirMode && CanSlerp(qReferenceToTarget, qReferenceToTargetAbsolute))
			{
				qReferenceToTargetAbsolute = PrepareSlerp(qReferenceToTarget, qReferenceToTargetAbsolute);
				qReferenceToTarget = Slerp(ScalarV(SlowInOut(m_afRefDirBlend[COMP_TORSO])), qReferenceToTarget, qReferenceToTargetAbsolute);
			}
			else
			{
				qReferenceToTarget = qReferenceToTargetAbsolute;
			}
		}

		QuatV qSpineTarget;
		Vec3V vRefToTargetAxis;
		ScalarV vRefToTargetAngle;
		QuatVToAxisAngle(vRefToTargetAxis, vRefToTargetAngle, qReferenceToTarget);

		ikAssert(m_pPed);
		const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

		// Apply rotation, scaled to each spine bone
		for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
		{
			const Mat34V& mtxSpine = skeleton.GetObjectMtx(m_auSpineBoneIdx[spineBoneIndex]);

			// Transform rotation axis into spine bone local and scale angle by spine weight
			Vec3V vAxis(UnTransform3x3Ortho(mtxSpine, vRefToTargetAxis));
			ScalarV vAngle(Scale(vRefToTargetAngle, ScalarV(ms_afSpineWeight[spineBoneIndex])));
			QuatV qReferenceToTargetScaled(QuatVFromAxisAngle(vAxis, vAngle));

			// Clamp to delta limits
			ScalarV vSpineWeight(ms_afSpineWeight[spineBoneIndex]);
			Vec3V vEulersXYZTorso(QuatVToEulersXYZ(qReferenceToTargetScaled));
			Vec3V vMinLimit(Scale(vMinDeltaLimit, vSpineWeight));
			Vec3V vMaxLimit(Scale(vMaxDeltaLimit, vSpineWeight));
			Vec3V vEulersXYZTorsoClamped(Clamp(vEulersXYZTorso, vMinLimit, vMaxLimit));

			qReferenceToTargetScaled = QuatVFromEulersXYZ(vEulersXYZTorsoClamped);
			qReferenceToTargetScaled = Normalize(qReferenceToTargetScaled);

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "SPN%d YRP: %6.1f, %6.1f, %6.1f (Delta)\n", spineBoneIndex, vEulersXYZTorso.GetXf() * RtoD, vEulersXYZTorso.GetYf() * RtoD, -vEulersXYZTorso.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "SPN%d YRP: %6.1f, %6.1f, %6.1f (Delta Actual)\n", spineBoneIndex, vEulersXYZTorsoClamped.GetXf() * RtoD, vEulersXYZTorsoClamped.GetYf() * RtoD, -vEulersXYZTorsoClamped.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZTorso, vEulersXYZTorsoClamped) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV

			// Rotate spine local
			Mat34V& mtxSpineLocal = skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBoneIndex]);
			QuatV qSpineLocal(QuatVFromMat34V(mtxSpineLocal));

			qSpineTarget = Multiply(qSpineLocal, qReferenceToTargetScaled);

			// Clamp to joint limits
			vEulersXYZTorso = QuatVToEulersXYZ(qSpineTarget);
			Vec3V vEulersXYZLocal = QuatVToEulersXYZ(qSpineLocal);

			// Yaw about XAXIS, Pitch about ZAXIS
			vMinLimit.SetX(ms_avTorsoJointLimits[spineBoneIndex][0].GetXf());
			vMinLimit.SetY(ms_avTorsoJointLimits[spineBoneIndex][0].GetYf());
			vMinLimit.SetZ(-ms_avTorsoJointLimits[spineBoneIndex][1].GetZf());

			vMaxLimit.SetX(ms_avTorsoJointLimits[spineBoneIndex][1].GetXf());
			vMaxLimit.SetY(ms_avTorsoJointLimits[spineBoneIndex][1].GetYf());
			vMaxLimit.SetZ(-ms_avTorsoJointLimits[spineBoneIndex][0].GetZf());

			vMinLimit = Scale(vMinLimit, ScalarV(V_TO_RADIANS));
			vMaxLimit = Scale(vMaxLimit, ScalarV(V_TO_RADIANS));

			vEulersXYZTorsoClamped = Clamp(vEulersXYZTorso, vMinLimit, vMaxLimit);

			// Set default yaw/pitch values if sub-component is disabled
			SetDefaultYawPitch(COMP_TORSO, vEulersXYZLocal, vEulersXYZTorsoClamped, m_avDeltaLimits[COMP_TORSO], ikSettings.GetTorsoDeltaLimits());

			qSpineTarget = QuatVFromEulersXYZ(vEulersXYZTorsoClamped);
			qSpineTarget = Normalize(qSpineTarget);

			// Blend local spine from skeleton to local spine target
			if (CanSlerp(qSpineLocal, qSpineTarget))
			{
				qSpineTarget = PrepareSlerp(qSpineLocal, qSpineTarget);
				qSpineLocal = Slerp(SlowInOut(ScalarV(m_afSolverBlend[COMP_TORSO])), qSpineLocal, qSpineTarget);
			}

			ikAssertf(IsFiniteAll(qSpineLocal), "qSpineLocal not finite");

			Mat34VFromQuatV(mtxSpineLocal, qSpineLocal, mtxSpineLocal.GetCol3());

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "SPN%d YRP: %6.1f, %6.1f, %6.1f (Joint)\n", spineBoneIndex, vEulersXYZTorso.GetXf() * RtoD, vEulersXYZTorso.GetYf() * RtoD, -vEulersXYZTorso.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "SPN%d YRP: %6.1f, %6.1f, %6.1f (Joint Actual)\n", spineBoneIndex, vEulersXYZTorsoClamped.GetXf() * RtoD, vEulersXYZTorsoClamped.GetYf() * RtoD, -vEulersXYZTorsoClamped.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZTorso, vEulersXYZTorsoClamped) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV
		}

		SolveArmCompensation(skeleton);

		skeleton.PartialUpdate(m_uSpineRootBoneIdx, false);
		return true;
	}

	return false;
}

bool CBodyLookIkSolver::SolveArmCompensation(rage::crSkeleton& skeleton)
{
	// Calculate the amount of counter rotation required (re-use head attitude weights)
	float afArmCompWeight[NUM_ARMS];
	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		afArmCompWeight[armIndex] = Lerp(m_afArmCompBlend[armIndex], ms_afHeadAttitudeWeight[m_aeArmCompPrev[armIndex]], ms_afHeadAttitudeWeight[m_aeArmComp[armIndex]]);
	}

	if ((afArmCompWeight[LEFT_ARM] == 0.0f) && (afArmCompWeight[RIGHT_ARM] == 0.0f))
	{
		return false;
	}

	QuatV qArmLocal;
	QuatV qCounter;
	Vec3V vDeltaAxis;
	Vec3V vDeltaAxisLocal;
	ScalarV sDeltaAngle;
	ScalarV sDeltaAngleScaled;

	// Determine amount of rotation of primary spine from animated pose
	const Mat34V& mtxPrimarySpinePrev = skeleton.GetObjectMtx(m_auSpineBoneIdx[SPINE3]);

	Mat34V mtxPrimarySpineCurr(skeleton.GetObjectMtx(m_uSpineRootBoneIdx));
	for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
	{
		Transform(mtxPrimarySpineCurr, mtxPrimarySpineCurr, skeleton.GetLocalMtx(m_auSpineBoneIdx[spineBoneIndex]));
	}

	QuatV qDelta(QuatVFromVectors(mtxPrimarySpinePrev.GetCol1(), mtxPrimarySpineCurr.GetCol1()));
	QuatVToAxisAngle(vDeltaAxis, sDeltaAngle, qDelta);

	for (int armIndex = 0; armIndex < NUM_ARMS; ++armIndex)
	{
		if (afArmCompWeight[armIndex] > 0.0f)
		{
			// Transform rotation axis into upper arm local
			vDeltaAxisLocal = UnTransform3x3Ortho(skeleton.GetObjectMtx(m_auClavicleBoneIdx[armIndex]), vDeltaAxis);

			// Invert angle and scale to compensation amount
			sDeltaAngleScaled = Scale(sDeltaAngle, ScalarV(-afArmCompWeight[armIndex]));

			qCounter = QuatVFromAxisAngle(vDeltaAxisLocal, sDeltaAngleScaled);

			Mat34V& mtxArmLocal = skeleton.GetLocalMtx(m_auUpperArmBoneIdx[armIndex]);
			qArmLocal = QuatVFromMat34V(mtxArmLocal);
			qArmLocal = Multiply(qCounter, qArmLocal);
			Mat34VFromQuatV(mtxArmLocal, qArmLocal, mtxArmLocal.GetCol3());
		}
	}

	return true;
}

bool CBodyLookIkSolver::SolveNeck(rage::crSkeleton& skeleton, SolverContext& context)
{
	IK_DEV_ONLY(char debugText[64];)

	if (m_afSolverBlend[COMP_NECK] > 0.0f)
	{
	#if __IK_DEV
		if (CaptureDebugRender())
		{
			sprintf(debugText, "- NECK  --------------------------------------\n");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif //__IK_DEV

		const bool bRelativeDirMode = m_afRefDirBlend[COMP_NECK] < 1.0f;
		const bool bAbsoluteMode	= m_afRefDirBlend[COMP_NECK] > 0.0f;

		const s16* aParentIndices = skeleton.GetSkeletonData().GetParentIndices();

		const ScalarV vBoneWeight((m_uNeckBoneCount <= 1) ? 1.0f : 1.0f / (float)m_uNeckBoneCount);
		const ScalarV vToRadians(V_TO_RADIANS);
		const Mat34V& mtxNeck = skeleton.GetObjectMtx(m_uNeckBoneIdx);

		Vec3V vMinDeltaLimit(m_avDeltaLimits[COMP_NECK][0].GetXY(), Negate(m_avDeltaLimits[COMP_NECK][1].GetZ()));
		Vec3V vMaxDeltaLimit(m_avDeltaLimits[COMP_NECK][1].GetXY(), Negate(m_avDeltaLimits[COMP_NECK][0].GetZ()));
		vMinDeltaLimit = Scale(vMinDeltaLimit, vToRadians);
		vMaxDeltaLimit = Scale(vMaxDeltaLimit, vToRadians);

		Vec3V vMinJointLimit(ms_avNeckJointLimits[0].GetXY(), Negate(ms_avNeckJointLimits[1].GetZ()));
		Vec3V vMaxJointLimit(ms_avNeckJointLimits[1].GetXY(), Negate(ms_avNeckJointLimits[0].GetZ()));
		vMinJointLimit = Scale(vMinJointLimit, vToRadians);
		vMaxJointLimit = Scale(vMaxJointLimit, vToRadians);

		QuatV qReferenceToTarget(V_IDENTITY);

		if (bRelativeDirMode)
		{
			CalculateRelativeAngleRotation(COMP_NECK, vMinDeltaLimit, vMaxDeltaLimit, context, qReferenceToTarget);
		}

		if (bAbsoluteMode)
		{
			const Vec3V vReferenceDirection((m_uBipedQuadruped == 0) ? skeleton.GetObjectMtx(aParentIndices[m_uNeckBoneIdx]).GetCol1() : skeleton.GetObjectMtx(m_auSpineBoneIdx[SPINE2]).GetCol0());
			const Vec3V vReferenceToTarget(Normalize(Subtract(context.m_avTargetPosition[COMP_NECK], mtxNeck.GetCol3())));

			// Generate rotation from reference direction to target (to preserve any roll in the reference direction)
			QuatV qReferenceToTargetAbsolute(QuatVFromVectors(vReferenceDirection, vReferenceToTarget));

			// Cross fade if necessary
			if (bRelativeDirMode && CanSlerp(qReferenceToTarget, qReferenceToTargetAbsolute))
			{
				qReferenceToTargetAbsolute = PrepareSlerp(qReferenceToTarget, qReferenceToTargetAbsolute);
				qReferenceToTarget = Slerp(ScalarV(SlowInOut(m_afRefDirBlend[COMP_NECK])), qReferenceToTarget, qReferenceToTargetAbsolute);
			}
			else
			{
				qReferenceToTarget = qReferenceToTargetAbsolute;
			}
		}

		// Apply rotation, scaled to each bone
		bool bRollLimitAdjusted = false;
		u16 uBoneIdx = m_uHeadBoneIdx;

		Vec3V vAxis;
		Vec3V vEulers;
		Vec3V vEulersClamped;
		ScalarV vAngle;

		QuatVToAxisAngle(vAxis, vAngle, qReferenceToTarget);

		vMinDeltaLimit = Scale(vMinDeltaLimit, vBoneWeight);
		vMaxDeltaLimit = Scale(vMaxDeltaLimit, vBoneWeight);

		// Additive
		const QuatV& qAdditive = m_pProxy->m_Request.GetHeadNeckAdditive();
		const bool bApplyAdditive = IsEqualAll(qAdditive, QuatV(V_IDENTITY)) == 0;

		Vec3V vAdditiveAxis(V_UP_AXIS_WZERO);
		ScalarV vAdditiveAngle(V_ZERO);

		if (bApplyAdditive)
		{
			QuatVToAxisAngle(vAdditiveAxis, vAdditiveAngle, qAdditive);
			vAdditiveAngle = Scale(vAdditiveAngle, ScalarV(1.0f - m_pProxy->m_Request.GetHeadNeckAdditiveRatio()));
		}

		ikAssert(m_pPed);
		const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

		while (aParentIndices[uBoneIdx] != m_auSpineBoneIdx[SPINE3])
		{
			const Mat34V& mtxBone = skeleton.GetObjectMtx(aParentIndices[uBoneIdx]);
			Mat34V& mtxLocal = skeleton.GetLocalMtx(aParentIndices[uBoneIdx]);

			QuatV qLocal(QuatVFromMat34V(mtxLocal));
			const Vec3V vEulersLocal(QuatVToEulersXYZ(qLocal));

			QuatV qRotation(QuatVFromAxisAngle(UnTransform3x3Ortho(mtxBone, vAxis), Scale(vAngle, vBoneWeight)));
			QuatV qTarget(Multiply(qLocal, qRotation));

			// Additive
			if (bApplyAdditive)
			{
				qTarget = Multiply(qTarget, QuatVFromAxisAngle(UnTransform3x3Ortho(mtxBone, vAdditiveAxis), Scale(vAdditiveAngle, vBoneWeight)));
			}

			// TODO: Convert to twist/swing limits to support bipeds and quadrupeds
			if (m_uBipedQuadruped == 0)
			{
				// Joint limits
				vEulers = QuatVToEulersXYZ(qTarget);
				vEulersClamped = Clamp(vEulers, vMinJointLimit, vMaxJointLimit);
				qTarget = Normalize(QuatVFromEulersXYZ(vEulersClamped));

			#if __IK_DEV
				if (CaptureDebugRender())
				{	
					formatf(debugText, sizeof(debugText), "NCK%2d YRP: %6.1f, %6.1f, %6.1f (Joint)", aParentIndices[uBoneIdx], vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, -vEulers.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
					formatf(debugText, sizeof(debugText), "NCK%2d YRP: %6.1f, %6.1f, %6.1f (Joint Actual)", aParentIndices[uBoneIdx], vEulersClamped.GetXf() * RtoD, vEulersClamped.GetYf() * RtoD, -vEulersClamped.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulers, vEulersClamped) ? Color_white : Color_red, debugText);
				}
			#endif //__IK_DEV

				u32 uYawClamped = IsEqualNone(vEulers.GetX(), vEulersClamped.GetX());

				// Delta limits
				QuatV qDelta(UnTransform(qLocal, qTarget));
				vEulers = QuatVToEulersXYZ(qDelta);

				if (!bRollLimitAdjusted)
				{
					// Scale roll delta limit to compensate if yaw was clamped
					ScaleRollLimit(uYawClamped, vEulers, m_afRollLimitScale[0], vMinDeltaLimit, vMaxDeltaLimit, context.m_fDeltaTime);
					bRollLimitAdjusted = true;
				}

				vEulersClamped = Clamp(vEulers, vMinDeltaLimit, vMaxDeltaLimit);

				qDelta = Normalize(QuatVFromEulersXYZ(vEulersClamped));
				qTarget = Multiply(qLocal, qDelta);

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					formatf(debugText, sizeof(debugText), "NCK%2d YRP: %6.1f, %6.1f, %6.1f (Delta)", aParentIndices[uBoneIdx], vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, -vEulers.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
					formatf(debugText, sizeof(debugText), "NCK%2d YRP: %6.1f, %6.1f, %6.1f (Delta Actual)", aParentIndices[uBoneIdx], vEulersClamped.GetXf() * RtoD, vEulersClamped.GetYf() * RtoD, -vEulersClamped.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulers, vEulersClamped) ? Color_white : Color_red, debugText);
				}
			#endif //__IK_DEV

				// Set default yaw/pitch values if sub-component is disabled
				vEulers = QuatVToEulersXYZ(qTarget);
				SetDefaultYawPitch(COMP_NECK, vEulersLocal, vEulers, m_avDeltaLimits[COMP_NECK], ikSettings.GetNeckDeltaLimits());

				qTarget = Normalize(QuatVFromEulersXYZ(vEulers));
			}

			// Blend local neck from skeleton to local neck target
			if (CanSlerp(qLocal, qTarget))
			{
				qTarget = PrepareSlerp(qLocal, qTarget);
				qLocal = Slerp(SlowInOut(ScalarV(m_afSolverBlend[COMP_NECK])), qLocal, qTarget);
			}

			ikAssertf(IsFiniteAll(qLocal), "qLocal not finite");
			Mat34VFromQuatV(mtxLocal, qLocal, mtxLocal.GetCol3());

			uBoneIdx = aParentIndices[uBoneIdx];
		}

		if ((m_afSolverBlend[COMP_HEAD] > 0.0f) && (m_uNeckBoneCount == 1))
		{
			// Update object matrices directly since head solver is active and will update the remaining child bones during its update
			Transform(skeleton.GetObjectMtx(m_uNeckBoneIdx), skeleton.GetObjectMtx(aParentIndices[m_uNeckBoneIdx]), skeleton.GetLocalMtx(m_uNeckBoneIdx));
			Transform(skeleton.GetObjectMtx(m_uHeadBoneIdx), skeleton.GetObjectMtx(aParentIndices[m_uHeadBoneIdx]), skeleton.GetLocalMtx(m_uHeadBoneIdx));
			Transform(skeleton.GetObjectMtx(m_uLookBoneIdx), skeleton.GetObjectMtx(aParentIndices[m_uLookBoneIdx]), skeleton.GetLocalMtx(m_uLookBoneIdx));
		}
		else
		{
			skeleton.PartialUpdate(m_uNeckBoneIdx);
		}

		return true;
	}

	m_afRollLimitScale[0] = 1.0f;
	return false;
}

bool CBodyLookIkSolver::SolveHead(rage::crSkeleton& skeleton, SolverContext& context)
{
	IK_DEV_ONLY(char debugText[64];)

	if (m_afSolverBlend[COMP_HEAD] > 0.0f)
	{
	#if __IK_DEV
		if (CaptureDebugRender())
		{
			sprintf(debugText, "- HEAD  --------------------------------------\n");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif //__IK_DEV

		const bool bRelativeDirMode = m_afRefDirBlend[COMP_HEAD] < 1.0f;
		const bool bAbsoluteMode	= m_afRefDirBlend[COMP_HEAD] > 0.0f;

		const ScalarV vToRadians(V_TO_RADIANS);

		const Mat34V& mtxHead = skeleton.GetObjectMtx(m_uHeadBoneIdx);
		Mat34V& mtxHeadLocal = skeleton.GetLocalMtx(m_uHeadBoneIdx);
		QuatV qHeadLocal(QuatVFromMat34V(mtxHeadLocal));

		Vec3V vMinDeltaLimit(m_avDeltaLimits[COMP_HEAD][0].GetXY(), Negate(m_avDeltaLimits[COMP_HEAD][1].GetZ()));
		Vec3V vMaxDeltaLimit(m_avDeltaLimits[COMP_HEAD][1].GetXY(), Negate(m_avDeltaLimits[COMP_HEAD][0].GetZ()));
		vMinDeltaLimit = Scale(vMinDeltaLimit, vToRadians);
		vMaxDeltaLimit = Scale(vMaxDeltaLimit, vToRadians);

		QuatV qReferenceToTarget(V_IDENTITY);
		Vec3V vAxis;
		ScalarV vAngle;

		if (bRelativeDirMode)
		{
			CalculateRelativeAngleRotation(COMP_HEAD, vMinDeltaLimit, vMaxDeltaLimit, context, qReferenceToTarget);
		}

		if (bAbsoluteMode)
		{
			const Mat34V& mtxLook = skeleton.GetObjectMtx(m_uLookBoneIdx);
			const Vec3V vReferenceToTarget(Normalize(Subtract(context.m_avTargetPosition[COMP_HEAD], mtxHead.GetCol3())));

			// Generate rotation from reference direction to target
			QuatV qReferenceToTargetAbsolute(QuatVFromVectors(mtxLook.GetCol1(), vReferenceToTarget));

			// Cross fade if necessary
			if (bRelativeDirMode && CanSlerp(qReferenceToTarget, qReferenceToTargetAbsolute))
			{
				qReferenceToTargetAbsolute = PrepareSlerp(qReferenceToTarget, qReferenceToTargetAbsolute);
				qReferenceToTarget = Slerp(ScalarV(SlowInOut(m_afRefDirBlend[COMP_HEAD])), qReferenceToTarget, qReferenceToTargetAbsolute);
			}
			else
			{
				qReferenceToTarget = qReferenceToTargetAbsolute;
			}
		}

		QuatVToAxisAngle(vAxis, vAngle, qReferenceToTarget);

		// Transform rotation axis into head space
		qReferenceToTarget = QuatVFromAxisAngle(UnTransform3x3Ortho(mtxHead, vAxis), vAngle);

		// Head target in local space
		QuatV qHeadTarget(Multiply(qHeadLocal, qReferenceToTarget));

		// Additive
		const QuatV& qAdditive = m_pProxy->m_Request.GetHeadNeckAdditive();
		if (!IsEqualAll(qAdditive, QuatV(V_IDENTITY)))
		{
			QuatVToAxisAngle(vAxis, vAngle, qAdditive);
			qHeadTarget = Multiply(qHeadTarget, QuatVFromAxisAngle(UnTransform3x3Ortho(mtxHead, vAxis), Scale(vAngle, ScalarV(m_pProxy->m_Request.GetHeadNeckAdditiveRatio()))));
		}

		// TODO: Convert to twist/swing limits to support bipeds and quadrupeds
		if (m_uBipedQuadruped == 0)
		{
			// Clamp to joint limits. Yaw about XAXIS, Pitch about ZAXIS
			Vec3V vMinJointLimit(ms_avHeadJointLimits[0].GetXY(), Negate(ms_avHeadJointLimits[1].GetZ()));
			Vec3V vMaxJointLimit(ms_avHeadJointLimits[1].GetXY(), Negate(ms_avHeadJointLimits[0].GetZ()));
			vMinJointLimit = Scale(vMinJointLimit, vToRadians);
			vMaxJointLimit = Scale(vMaxJointLimit, vToRadians);

			Vec3V vEulers(QuatVToEulersXYZ(qHeadTarget));
			Vec3V vEulersClamped(Clamp(vEulers, vMinJointLimit, vMaxJointLimit));
			qHeadTarget = Normalize(QuatVFromEulersXYZ(vEulersClamped));

		#if __IK_DEV
			if (CaptureDebugRender())
			{	
				sprintf(debugText, "HEAD YRP: %6.1f, %6.1f, %6.1f (Joint)\n", vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, -vEulers.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "HEAD YRP: %6.1f, %6.1f, %6.1f (Joint Actual)\n", vEulersClamped.GetXf() * RtoD, vEulersClamped.GetYf() * RtoD, -vEulersClamped.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulers, vEulersClamped) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV

			// Clamp to delta limits
			u32 uYawClamped = IsEqualNone(vEulers.GetX(), vEulersClamped.GetX());
			QuatV qHeadDelta(UnTransform(qHeadLocal, qHeadTarget));
			vEulers = QuatVToEulersXYZ(qHeadDelta);

			// Scale roll delta limit to compensate if yaw was clamped
			ScaleRollLimit(uYawClamped, vEulers, m_afRollLimitScale[1], vMinDeltaLimit, vMaxDeltaLimit, context.m_fDeltaTime);

			vEulersClamped = Clamp(vEulers, vMinDeltaLimit, vMaxDeltaLimit);

			qHeadDelta = Normalize(QuatVFromEulersXYZ(vEulersClamped));
			qHeadTarget = Multiply(qHeadLocal, qHeadDelta);

		#if __IK_DEV
			if (CaptureDebugRender())
			{	
				sprintf(debugText, "HEAD YRP: %6.1f, %6.1f, %6.1f (Delta)\n", vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, -vEulers.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				sprintf(debugText, "HEAD YRP: %6.1f, %6.1f, %6.1f (Delta Actual)\n", vEulersClamped.GetXf() * RtoD, vEulersClamped.GetYf() * RtoD, -vEulersClamped.GetZf() * RtoD);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulers, vEulersClamped) ? Color_white : Color_red, debugText);
			}
		#endif //__IK_DEV

			ikAssert(m_pPed);
			const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

			// Set default yaw/pitch values if sub-component is disabled
			vEulers = QuatVToEulersXYZ(qHeadLocal);
			vEulersClamped = QuatVToEulersXYZ(qHeadTarget);
			SetDefaultYawPitch(COMP_HEAD, vEulers, vEulersClamped, m_avDeltaLimits[COMP_HEAD], ikSettings.GetHeadDeltaLimits());
			qHeadTarget = Normalize(QuatVFromEulersXYZ(vEulersClamped));
		}

		// Blend local head from skeleton to local head target
		if (CanSlerp(qHeadLocal, qHeadTarget))
		{
			qHeadTarget = PrepareSlerp(qHeadLocal, qHeadTarget);
			qHeadLocal = Slerp(SlowInOut(ScalarV(m_afSolverBlend[COMP_HEAD])), qHeadLocal, qHeadTarget);
		}

		ikAssertf(IsFiniteAll(qHeadLocal), "qHeadLocal not finite");
		Mat34VFromQuatV(mtxHeadLocal, qHeadLocal, mtxHeadLocal.GetCol3());

		skeleton.PartialUpdate(m_uHeadBoneIdx);
		return true;
	}

	m_afRollLimitScale[1] = 1.0f;
	return false;
}

bool CBodyLookIkSolver::SolveEyes(rage::crSkeleton& skeleton, SolverContext& context)
{
	IK_DEV_ONLY(char debugText[64];)

	QuatV qEyeTarget;

	if (m_afSolverBlend[COMP_EYE] > 0.0f)
	{
	#if __IK_DEV
		if (CaptureDebugRender())
		{
			sprintf(debugText, "- EYES  --------------------------------------\n");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	#endif //__IK_DEV

		const bool bRelativeDirMode = m_afRefDirBlend[COMP_EYE] < 1.0f;
		const bool bAbsoluteMode	= m_afRefDirBlend[COMP_EYE] > 0.0f;

		const Mat34V& mtxHead = skeleton.GetObjectMtx(m_uHeadBoneIdx);
		const ScalarV vDtoR(DtoR);

		QuatV qReferenceToTarget;
		Vec3V vMinLimit;
		Vec3V vMaxLimit;

		float fYAngle = 0.0f;
		float fPAngle = 0.0f;

		if (bRelativeDirMode)
		{
			float fAAngle;

			// Yaw
			fYAngle = context.m_vEulersAdditive.GetXf() * (float)m_auRefDirWeight[COMP_EYE] * 0.01f;						// Scale overall yaw by component weight to get yaw contribution by this component
			fAAngle = fabsf(fYAngle);
			fYAngle = fAAngle > 0.0f ? Min(fAAngle, context.m_vEulersAdditiveComp.GetXf()) * (fYAngle / fAAngle) : 0.0f;	// Clamp yaw contribution to the amount of yaw still remaining

			// Pitch
			fPAngle = context.m_vEulersAdditive.GetZf() * (float)m_auRefDirWeight[COMP_EYE] * 0.01f;
			fAAngle = fabsf(fPAngle);
			fPAngle = fAAngle > 0.0f ? Min(fAAngle, context.m_vEulersAdditiveComp.GetZf()) * (fPAngle / fAAngle) : 0.0f;
		}

		Vec3V vYAxis(m_bAlternateEyeRig ? Vec3V(V_Z_AXIS_WONE) : Vec3V(V_Y_AXIS_WONE));
		Vec3V vPAxis(m_bAlternateEyeRig ? Vec3V(V_Y_AXIS_WONE) : Vec3V(V_X_AXIS_WONE));

		for (int eyeIndex = 0; eyeIndex < NUM_EYES; ++eyeIndex)
		{
			Mat34V& mtxEyeLocal = skeleton.GetLocalMtx(m_auEyeBoneIdx[eyeIndex]);
			QuatV qEyeLocal(QuatVFromMat34V(mtxEyeLocal));

			// TODO: Refactor once default eye orientations are standardized across player, cutscene, and ambient facial rigs
			if (bRelativeDirMode)
			{
				if (m_bAlternateEyeRig)
				{
					// Yaw Z, Pitch Y
					vMinLimit = Vec3V(ms_avEyeJointLimits[0].GetYf(), -ms_avEyeJointLimits[1].GetZf(), ms_avEyeJointLimits[0].GetXf());
					vMaxLimit = Vec3V(ms_avEyeJointLimits[1].GetYf(), -ms_avEyeJointLimits[0].GetZf(), ms_avEyeJointLimits[1].GetXf());
				}
				else
				{
					// Yaw Y, Pitch X
					vMinLimit = Vec3V(-ms_avEyeJointLimits[1].GetZf(), ms_avEyeJointLimits[0].GetXf(), ms_avEyeJointLimits[0].GetYf());
					vMaxLimit = Vec3V(-ms_avEyeJointLimits[0].GetZf(), ms_avEyeJointLimits[1].GetXf(), ms_avEyeJointLimits[1].GetYf());
				}

				vMinLimit = Scale(vMinLimit, vDtoR);
				vMaxLimit = Scale(vMaxLimit, vDtoR);

				qEyeTarget = qEyeLocal;
				qEyeTarget = Multiply(qEyeTarget, QuatVFromAxisAngle(vYAxis, ScalarV(fYAngle)));
				qEyeTarget = Multiply(qEyeTarget, QuatVFromAxisAngle(vPAxis, ScalarV(-fPAngle)));

				QuatV qEyeDefault(QuatVFromMat34V(skeleton.GetSkeletonData().GetDefaultTransform(m_auEyeBoneIdx[eyeIndex])));
				QuatV qEyeDelta(UnTransform(qEyeDefault, qEyeTarget));

				Vec3V vEulersXYZEye(QuatVToEulersXYZ(qEyeDelta));
				Vec3V vEulersXYZEyeClamped(Clamp(vEulersXYZEye, vMinLimit, vMaxLimit));

				qEyeDelta = QuatVFromEulersXYZ(vEulersXYZEyeClamped);
				qEyeDelta = Normalize(qEyeDelta);
				qEyeTarget = Multiply(qEyeDefault, qEyeDelta);

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					sprintf(debugText, "EYE%d    : %6.1f, %6.1f, %6.1f (Joint)\n", eyeIndex, vEulersXYZEye.GetXf() * RtoD, vEulersXYZEye.GetYf() * RtoD, vEulersXYZEye.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
					sprintf(debugText, "EYE%d    : %6.1f, %6.1f, %6.1f (Joint Actual)\n", eyeIndex, vEulersXYZEyeClamped.GetXf() * RtoD, vEulersXYZEyeClamped.GetYf() * RtoD, vEulersXYZEyeClamped.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZEye, vEulersXYZEyeClamped) ? Color_white : Color_red, debugText);
				}
			#endif //__IK_DEV
			}

			if (bAbsoluteMode)
			{
				const Mat34V& mtxEye = skeleton.GetObjectMtx(m_auEyeBoneIdx[eyeIndex]);

				Vec3V vReferenceToTarget(Subtract(context.m_avTargetPosition[COMP_EYE], mtxEye.GetCol3()));
				vReferenceToTarget = Normalize(vReferenceToTarget);

				qReferenceToTarget = QuatVFromVectors(mtxHead.GetCol1(), vReferenceToTarget);

				QuatV qHead(QuatVFromMat34V(mtxHead));
				QuatV qEyeTargetAbsolute(Multiply(qReferenceToTarget, qHead));

				// Transform to local
				qEyeTargetAbsolute = UnTransform(qHead, qEyeTargetAbsolute);

				// Clamp
				Vec3V vEulersXYZEye = QuatVToEulersXYZ(qEyeTargetAbsolute);

				// Yaw about XAXIS, Pitch about ZAXIS
				vMinLimit = Vec3V(ms_avEyeJointLimits[0].GetXf(), ms_avEyeJointLimits[0].GetYf(), -ms_avEyeJointLimits[1].GetZf());
				vMaxLimit = Vec3V(ms_avEyeJointLimits[1].GetXf(), ms_avEyeJointLimits[1].GetYf(), -ms_avEyeJointLimits[0].GetZf());
				vMinLimit = Scale(vMinLimit, vDtoR);
				vMaxLimit = Scale(vMaxLimit, vDtoR);

				Vec3V vEulersXYZEyeClamped(Clamp(vEulersXYZEye, vMinLimit, vMaxLimit));

			#if __IK_DEV
				if (CaptureDebugRender())
				{
					sprintf(debugText, "EYE%d YRP: %6.1f, %6.1f, %6.1f (Joint)\n", eyeIndex, vEulersXYZEye.GetXf() * RtoD, vEulersXYZEye.GetYf() * RtoD, -vEulersXYZEye.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
					sprintf(debugText, "EYE%d YRP: %6.1f, %6.1f, %6.1f (Joint Actual)\n", eyeIndex, vEulersXYZEyeClamped.GetXf() * RtoD, vEulersXYZEyeClamped.GetYf() * RtoD, -vEulersXYZEyeClamped.GetZf() * RtoD);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, IsEqualAll(vEulersXYZEye, vEulersXYZEyeClamped) ? Color_white : Color_red, debugText);
				}
			#endif //__IK_DEV

				qEyeTargetAbsolute = QuatVFromMat34V(skeleton.GetSkeletonData().GetDefaultTransform(m_auEyeBoneIdx[eyeIndex]));
				qEyeTargetAbsolute = Multiply(qEyeTargetAbsolute, QuatVFromAxisAngle(vYAxis, ScalarV(vEulersXYZEyeClamped.GetXf())));
				qEyeTargetAbsolute = Multiply(qEyeTargetAbsolute, QuatVFromAxisAngle(vPAxis, ScalarV(vEulersXYZEyeClamped.GetZf())));

				// Cross fade if necessary
				if (bRelativeDirMode && CanSlerp(qEyeTarget, qEyeTargetAbsolute))
				{
					qEyeTargetAbsolute = PrepareSlerp(qEyeTarget, qEyeTargetAbsolute);
					qEyeTarget = Slerp(ScalarV(SlowInOut(m_afRefDirBlend[COMP_EYE])), qEyeTarget, qEyeTargetAbsolute);
				}
				else
				{
					qEyeTarget = qEyeTargetAbsolute;
				}
			}

			// Blend local eye from skeleton to local eye target
			if (CanSlerp(qEyeLocal, qEyeTarget))
			{
				qEyeTarget = PrepareSlerp(qEyeLocal, qEyeTarget);
				qEyeLocal = Slerp(SlowInOut(ScalarV(m_afSolverBlend[COMP_EYE])), qEyeLocal, qEyeTarget);
			}

			Mat34VFromQuatV(mtxEyeLocal, qEyeLocal, mtxEyeLocal.GetCol3());
		}

		// Update object matrices
		const s16* aParentIndices = skeleton.GetSkeletonData().GetParentIndices();
		for (int eyeIndex = 0; eyeIndex < NUM_EYES; ++eyeIndex)
		{
			Transform(skeleton.GetObjectMtx(m_auEyeBoneIdx[eyeIndex]), skeleton.GetObjectMtx(aParentIndices[m_auEyeBoneIdx[eyeIndex]]), skeleton.GetLocalMtx(m_auEyeBoneIdx[eyeIndex]));
		}

		return true;
	}

	return false;
}

void CBodyLookIkSolver::InitializeDeltaLimits()
{
	ikAssert(m_pPed);
	const CPedIKSettingsInfo& ikSettings = m_pPed->GetIKSettings();

	for (int limitIndex = 0; limitIndex < 2; ++limitIndex)
	{
		m_avDeltaLimits[COMP_TORSO][limitIndex] = Vec3V(ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_TORSO][LOOKIK_ROT_ANGLE_YAW]][limitIndex][0], ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_TORSO][LOOKIK_ROT_ANGLE_YAW]][limitIndex][1], ikSettings.GetTorsoDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_TORSO][LOOKIK_ROT_ANGLE_PITCH]][limitIndex][2]);
		m_avDeltaLimits[COMP_NECK][limitIndex]  = Vec3V(ikSettings.GetHeadDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_NECK][LOOKIK_ROT_ANGLE_YAW]][limitIndex][0], ikSettings.GetHeadDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_NECK][LOOKIK_ROT_ANGLE_YAW]][limitIndex][1], ikSettings.GetHeadDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_NECK][LOOKIK_ROT_ANGLE_PITCH]][limitIndex][2]);
		m_avDeltaLimits[COMP_HEAD][limitIndex]  = Vec3V(ikSettings.GetNeckDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_HEAD][LOOKIK_ROT_ANGLE_YAW]][limitIndex][0], ikSettings.GetNeckDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_HEAD][LOOKIK_ROT_ANGLE_YAW]][limitIndex][1], ikSettings.GetNeckDeltaLimits().m_DeltaLimits[m_aeRotLim[COMP_HEAD][LOOKIK_ROT_ANGLE_PITCH]][limitIndex][2]);
	}
}

bool CBodyLookIkSolver::CanSlerp(QuatV_In q1, QuatV_In q2)
{
	ScalarV sDot(Dot(q1, q2));
	sDot = Abs(sDot);
	sDot = Clamp(sDot, ScalarV(V_ZERO), ScalarV(V_ONE));
	sDot = Arccos(sDot);
	sDot = Sin(sDot);

	return !IsZeroAll(sDot);
}

Vec3V_Out CBodyLookIkSolver::ClampTargetPosition(SolverContext& context)
{
	CTaskMotionBase* pMotionTask = m_pPed->GetCurrentMotionTask();

	const int angLimitIndex = pMotionTask && pMotionTask->IsUnderWater() ? 1 : 0;
	const ScalarV vMaxLimit(ms_afAngLimit[angLimitIndex] * DtoR);
	const ScalarV vZero(V_ZERO);
	const ScalarV vOne(V_ONE);
	const ScalarV vNegOne(V_NEGONE);
	const ScalarV vHalf(V_HALF);
	const ScalarV vMinDistance(0.10f);

	const Vec3V vYAxis(V_Y_AXIS_WONE);
	const Vec3V vUpAxis(V_UP_AXIS_WZERO);

	Vec3V vAxis;
	ScalarV vAngle;

	Vec3V vTargetPositionUnclamped(m_pProxy->GetTargetPosition());
	Vec3V vTargetPosition(UnTransformFull(context.m_mtxReferenceDir, vTargetPositionUnclamped));
	Vec3V vTargetPositionFlat(vTargetPosition.GetXY(), vZero);
	ScalarV vMagnitude(Mag(vTargetPositionFlat));

	QuatV qRotation(QuatVFromVectors(vYAxis, vTargetPositionFlat, vUpAxis));
	QuatVToAxisAngle(vAxis, vAngle, qRotation);

	if (IsLessThanAll(vMagnitude, vMinDistance))
	{
		// If point is too close to character, clamp to a position in front
		vTargetPositionFlat = Scale(vYAxis, vHalf);
		vTargetPositionFlat.SetZ(Clamp(vTargetPosition.GetZ(), vNegOne, vOne));
		vTargetPosition = Transform(context.m_mtxReferenceDir, vTargetPositionFlat);
	}
	else if (IsLessThanAll(vAngle, vMaxLimit))
	{
		vTargetPosition = vTargetPositionUnclamped;
	}
	else
	{
		// Clamp to a consistent side if angle is close to PI
		BoolV vbCloseToPI(IsGreaterThan(vAngle, ScalarV(175.0f * DtoR)));
		vAxis = SelectFT(vbCloseToPI, vAxis, vUpAxis);

		qRotation = QuatVFromAxisAngle(vAxis, vMaxLimit);

		vTargetPositionFlat = Transform(qRotation, vYAxis);
		vTargetPositionFlat = Scale(vTargetPositionFlat, vMagnitude);

		vMagnitude = Max(vMagnitude, vOne);
		vTargetPositionFlat.SetZ(Clamp(vTargetPosition.GetZ(), Negate(vMagnitude), vMagnitude));

		vTargetPosition = Transform(context.m_mtxReferenceDir, vTargetPositionFlat);
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		if (ms_bRenderTargetPosition)
		{
			crSkeleton& skeleton = *m_pPed->GetSkeleton();
			u32 uFlags = m_pProxy->GetFlags();

			Mat34V mtxHeadGlobal;
			skeleton.GetGlobalMtx(m_uHeadBoneIdx, mtxHeadGlobal);

			const Color32 colour1 = !(uFlags & LOOKIK_USE_ANIM_TAGS) ? Color_yellow : Color_orange;
			const Color32 colour2 = !(uFlags & LOOKIK_USE_ANIM_TAGS) ? Color_yellow4 : Color_orange4;

			Vec3V vStart(mtxHeadGlobal.GetCol3());
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vTargetPositionUnclamped), colour1);
			ms_debugHelper.Sphere(RCC_VECTOR3(vTargetPositionUnclamped), 0.02f, colour1, false);

			if (!IsEqualAll(vTargetPositionUnclamped, vTargetPosition))
			{
				// Render clamped target position
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vTargetPosition), colour2);
				ms_debugHelper.Sphere(RCC_VECTOR3(vTargetPosition), 0.04f, colour2, false);
			}
		}
	}
#endif // __IK_DEV

	return vTargetPosition;
}

Vec3V_Out CBodyLookIkSolver::InterpolateYawPitch(Vec3V_In vToTarget, Vec2V_Ref vYawPitchCurrent, Vec2V_Ref vYawPitchVelCurrent, ScalarV_In vCompSpring, ScalarV_In vCompDamping, ScalarV_In vCompInvMass, const InterpolationContext& interpContext)
{
	const ScalarV vTimeStep(interpContext.m_fDeltaTime);
	const ScalarV vScalarEps(ms_fEpsilon);
	const ScalarV vOvershootInterp(0.35f);

	const Vec2V vZero(V_ZERO);
	const Vec2V vPI(V_PI);
	const Vec2V vNegPI(V_NEG_PI);
	const Vec2V vHalfPI(V_PI_OVER_TWO);
	const Vec2V vNegHalfPI(V_NEG_PI_OVER_TWO);
	const Vec2V vYawPitchLimit(135.0f * DtoR, 75.0f * DtoR);
	const Vec2V vNegYawPitchLimit(Negate(vYawPitchLimit));

	// (Yaw, Pitch)
	const Vec2V vYComponent(Negate(vToTarget.GetX()), vToTarget.GetZ());
	const Vec2V vXComponent(SelectFT(IsZero(vToTarget.GetY()), vToTarget.GetY(), vScalarEps), Max(MagXY(vToTarget), vScalarEps));
	const Vec2V vYawPitchTarget(Arctan2(vYComponent, vXComponent));

	const Vec2V vDistance(Clamp(Subtract(vYawPitchTarget, vYawPitchCurrent), vNegHalfPI, vHalfPI));
	const Vec2V vDamping(Scale(vYawPitchVelCurrent, vCompDamping));

	// Update base force modifier with ang speed modifier if the character is rotating in the opposite direction to that needed by the interpolation
	Vec2V vForceModifier(SelectFT(SameSign(interpContext.m_vAngSpeed, vDistance), Scale(interpContext.m_vForceModifier, interpContext.m_vAngSpeedForceModifier), interpContext.m_vForceModifier));

	Vec2V vForce(Scale(Scale(vDistance, vCompSpring), vForceModifier));
	vForce = Subtract(vForce, vDamping);
	const Vec2V vAcc(Scale(vForce, vCompInvMass));

	vYawPitchVelCurrent = AddScaled(vYawPitchVelCurrent, vAcc, vTimeStep);

	// Check if target values will be overshot
	Vec2V vDelta(Scale(vYawPitchVelCurrent, vTimeStep));
	Vec2V vYawPitchPending(Clamp(Add(vYawPitchCurrent, vDelta), vNegPI, vPI));

	VecBoolV vTargetGreaterThanPending(IsGreaterThanOrEqual(Clamp(Subtract(vYawPitchTarget, vYawPitchPending), vNegHalfPI, vHalfPI), vZero));
	VecBoolV vTargetGreaterThanCurrent(IsGreaterThanOrEqual(vDistance, vZero));
	VecBoolV vOvershotTarget(Xor(vTargetGreaterThanPending, vTargetGreaterThanCurrent));

	vYawPitchCurrent = SelectFT(vOvershotTarget, vYawPitchPending, Lerp(vOvershootInterp, vYawPitchTarget, vYawPitchPending));

	// Instant interpolation
	VecBoolV vInstant(BoolV(interpContext.m_bInstant));
	vYawPitchCurrent = SelectFT(vInstant, vYawPitchCurrent, vYawPitchTarget);
	vYawPitchVelCurrent = SelectFT(vInstant, vYawPitchVelCurrent, vZero);

	// Clamp yaw/pitch and velocities
	VecBoolV vOr(Or(IsGreaterThanOrEqual(vYawPitchCurrent, vYawPitchLimit), IsLessThanOrEqual(vYawPitchCurrent, vNegYawPitchLimit)));
	vYawPitchVelCurrent = SelectFT(vOr, vYawPitchVelCurrent, vZero);
	vYawPitchCurrent = Clamp(vYawPitchCurrent, vNegYawPitchLimit, vYawPitchLimit);

#if __IK_DEV
	if (CaptureDebugRender() && ms_bRenderMotion)
	{
		char debugText[128];
		sprintf(debugText, "FM(%8.3f %8.3f) AS(%8.3f %8.3f) AM(%8.3f %8.3f) M(%8.3f %8.3f)", interpContext.m_vForceModifier.GetXf(), interpContext.m_vForceModifier.GetYf(), 
																							 interpContext.m_vAngSpeed.GetXf(), interpContext.m_vAngSpeed.GetYf(), 
																							 interpContext.m_vAngSpeedForceModifier.GetXf(), interpContext.m_vAngSpeedForceModifier.GetYf(), 
																							 vForceModifier.GetXf(), vForceModifier.GetYf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "D (%8.3f %8.3f) F (%8.3f %8.3f) A (%8.3f %8.3f)", vDistance.GetXf(), vDistance.GetYf(), vForce.GetXf(), vForce.GetYf(), vAcc.GetXf(), vAcc.GetYf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "V (%8.3f %8.3f) C (%8.3f %8.3f) T (%8.3f %8.3f)", vYawPitchVelCurrent.GetXf(), vYawPitchVelCurrent.GetYf(), vYawPitchCurrent.GetXf(), vYawPitchCurrent.GetYf(), vYawPitchTarget.GetXf(), vYawPitchTarget.GetYf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	// Convert yaw/pitch to a direction relative to y forward
	Vec2V vSin, vCos;
	SinAndCos(vSin, vCos, vYawPitchCurrent);

	Vec3V vTargetDir;
	vTargetDir.SetZ(vSin.GetY());
	vTargetDir.SetY(Scale(vCos.GetX(), vCos.GetY()));
	vTargetDir.SetX(Negate(Scale(vSin.GetX(), vCos.GetY())));
	vTargetDir = Normalize(vTargetDir);

	return vTargetDir;
}

void CBodyLookIkSolver::InterpolateTargetPositions(rage::crSkeleton& skeleton, SolverContext& context)
{
	const float fTimeStep = context.m_fDeltaTime;
	const float fMaxCharacterSpeed = 3.0f;
	const float fMaxCharacterAngSpeed = TWO_PI;

	const Vec3V vForward(V_Y_AXIS_WONE);
	const Vec2V vZero(V_ZERO);
	const Vec2V vOne(V_ONE);
	const ScalarV vExtrapolatedDistance(2.0f);
	const ScalarV vScalarZero(V_ZERO);
	const ScalarV vScalarOne(V_ONE);

	const Mat34V& mtxNeck = skeleton.GetObjectMtx(m_uNeckBoneIdx);
	const Mat34V& mtxHead = skeleton.GetObjectMtx(m_uHeadBoneIdx);

	Vec3V avReferencePosition[2];
	Vec3V vToTarget;
	Vec3V vTargetDir;

	// Character speed compensation
	const float fCurrHeading = Atan2f(context.m_mtxReferenceDir.GetM10f(), context.m_mtxReferenceDir.GetM11f()); 
	if (m_fPrevHeading == TWO_PI) m_fPrevHeading = fCurrHeading;
	const float fAngSpeed = fTimeStep != 0.0f ? fwAngle::LimitRadianAngle(fCurrHeading - m_fPrevHeading) / fTimeStep : 0.0f;
	const float fCharSpeed = m_pPed->GetVelocity().Mag();
	m_fPrevHeading = fCurrHeading;

	ScalarV vAngSpeedCounterModifier(ms_afCharAngSpeedCounterModifier[!m_pPed->GetMotionData()->GetIsStill()]);
	ScalarV vAngSpeedRatio(Clamp(Abs(fAngSpeed) / fMaxCharacterAngSpeed, 0.0f, 1.0f));
	ScalarV vSpeedRatio(Clamp(fCharSpeed / fMaxCharacterSpeed, 0.0f, 1.0f));
	vAngSpeedRatio = SlowInOut(vAngSpeedRatio);
	vSpeedRatio = SlowInOut(vSpeedRatio);

	// Reduce strength of modifier the faster the character is moving
	vAngSpeedCounterModifier = Lerp(vSpeedRatio, vAngSpeedCounterModifier, vScalarZero);

	// Interpolation modifiers
	InterpolationContext interpContext;

	interpContext.m_fDeltaTime = context.m_fDeltaTime;
	interpContext.m_bInstant = (m_eTurnRate == LOOKIK_TURN_RATE_INSTANT) || m_pProxy->m_Request.IsInstant();

	// 1.0f + (vAngSpeedRatio * vAngSpeedCounterModifier)
	interpContext.m_vAngSpeed = Vec2V(fAngSpeed, fAngSpeed);
	interpContext.m_vAngSpeedForceModifier = Vec2V(vAngSpeedRatio);
	interpContext.m_vAngSpeedForceModifier = Scale(interpContext.m_vAngSpeedForceModifier, vAngSpeedCounterModifier);
	interpContext.m_vAngSpeedForceModifier = Add(interpContext.m_vAngSpeedForceModifier, vOne);

	ScalarV vForceModifier(ms_afTargetSpringScale[m_eTurnRate]);
	vForceModifier = Scale(vForceModifier, Add(Scale(vAngSpeedRatio, ScalarV(ms_fCharAngSpeedModifier)), vScalarOne));
	vForceModifier = Scale(vForceModifier, Add(Scale(vSpeedRatio, ScalarV(ms_fCharSpeedModifier)), vScalarOne));
	interpContext.m_vForceModifier = Vec2V(vForceModifier);

	// Object space to reference dir space transform
	UnTransformFull(context.m_mtxObjToRefDirSpace, context.m_mtxReferenceDir, *skeleton.GetParentMtx());

	// Reference positions in reference dir space
	avReferencePosition[0] = Transform(context.m_mtxObjToRefDirSpace, mtxNeck.GetCol3());
	avReferencePosition[1] = Transform(context.m_mtxObjToRefDirSpace, mtxHead.GetCol3());

	// Target position in reference dir space
	Vec3V vTargetPosition(UnTransformFull(context.m_mtxReferenceDir, context.m_vTargetPosition));

	// Interpolate component yaw/pitch
	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		const int refPosIndex = (compIndex <= COMP_NECK) ? 0 : 1;

		if (m_afSolverBlend[compIndex] > 0.0f)
		{
			if (context.m_auActive[compIndex])
			{
				vToTarget = Subtract(vTargetPosition, avReferencePosition[refPosIndex]);
				vToTarget = NormalizeSafe(vToTarget, vForward);
			}
			else
			{
				vToTarget = vForward;
			}

			vTargetDir = InterpolateYawPitch(vToTarget, m_avYawPitch[compIndex], m_avYawPitchVel[compIndex], ScalarV(ms_afCompSpring[compIndex]), ScalarV(ms_afCompDamping[compIndex]), ScalarV(ms_afCompInvMass[compIndex]), interpContext);
		}
		else
		{
			m_avYawPitch[compIndex] = vZero;
			m_avYawPitchVel[compIndex] = vZero;
			vTargetDir = vForward;
		}

		// Extrapolate a target position from the target direction
		context.m_avTargetPosition[compIndex] = AddScaled(avReferencePosition[refPosIndex], vTargetDir, vExtrapolatedDistance);

		// Transform target position from reference dir space to object space
		context.m_avTargetPosition[compIndex] = UnTransformFull(context.m_mtxObjToRefDirSpace, context.m_avTargetPosition[compIndex]);
	}

	// Interpolate target direction for relative reference direction mode
	if (context.m_bRefDirRelative)
	{
		// Set the spring/damping/invmass values to be the maximum of the enabled components, but not exceeding ms_afCompSpringRel/ms_afCompDampingRel/ms_afCompInvMassRel
		float fCompSpring = 0.0f;
		float fCompDamping = 0.0f;
		float fCompInvMass = 0.0f;

		for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
		{
			if (context.m_auRefDirRel[compIndex] != 0)
			{
				fCompSpring = Max(fCompSpring, ms_afCompSpring[compIndex]);
				fCompDamping = Max(fCompDamping, ms_afCompDamping[compIndex]);
				fCompInvMass = Max(fCompInvMass, ms_afCompInvMass[compIndex]);
			}
		}
		fCompSpring = Min(fCompSpring, ms_afCompSpringRel);
		fCompDamping = Min(fCompDamping, ms_afCompDampingRel);
		fCompInvMass = Min(fCompInvMass, ms_afCompInvMassRel);

		vToTarget = Subtract(vTargetPosition, avReferencePosition[1]);
		vToTarget = NormalizeSafe(vToTarget, vForward);

		vTargetDir = InterpolateYawPitch(vToTarget, m_vYawPitchRel, m_vYawPitchRelVel, ScalarV(fCompSpring), ScalarV(fCompDamping), ScalarV(fCompInvMass), interpContext);
	}
	else
	{
		m_vYawPitchRel = vZero;
		m_vYawPitchRelVel = vZero;
		vTargetDir = vForward;
	}

	// Transform target direction from reference dir space to object space
	context.m_vTargetDirRel = UnTransform3x3Full(context.m_mtxObjToRefDirSpace, vTargetDir);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[64];

		Vec3V vStart;
		Vec3V vEnd;

		if (ms_bRenderMotion)
		{
			sprintf(debugText, "ASR(%8.3f) CSR(%8.3f) ASC(%8.3f)", vAngSpeedRatio.Getf(), vSpeedRatio.Getf(), vAngSpeedCounterModifier.Getf());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}

		if (ms_bRenderTargetPosition)
		{
			vStart = Transform(context.m_mtxReferenceDir, avReferencePosition[0]);

			if (m_afSolverBlend[COMP_TORSO] > 0.0f)
			{
				vEnd = Transform(*skeleton.GetParentMtx(), context.m_avTargetPosition[COMP_TORSO]);
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue4);
				ms_debugHelper.Sphere(RCC_VECTOR3(vEnd), 0.01f, Color_blue4, false);
				ms_debugHelper.Text3d(RCC_VECTOR3(vEnd), Color_white, 0, 0, "t", false); 
			}

			if (m_afSolverBlend[COMP_NECK] > 0.0f)
			{
				vEnd = Transform(*skeleton.GetParentMtx(), context.m_avTargetPosition[COMP_NECK]);
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue3);
				ms_debugHelper.Sphere(RCC_VECTOR3(vEnd), 0.01f, Color_blue3, false);
				ms_debugHelper.Text3d(RCC_VECTOR3(vEnd), Color_white, 0, 0, "n", false); 
			}

			vStart = Transform(context.m_mtxReferenceDir, avReferencePosition[1]);

			if (m_afSolverBlend[COMP_HEAD] > 0.0f)
			{
				vEnd = Transform(*skeleton.GetParentMtx(), context.m_avTargetPosition[COMP_HEAD]);
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue2);
				ms_debugHelper.Sphere(RCC_VECTOR3(vEnd), 0.01f, Color_blue2, false);
				ms_debugHelper.Text3d(RCC_VECTOR3(vEnd), Color_white, 0, 0, "h", false); 
			}

			if (m_afSolverBlend[COMP_EYE] > 0.0f)
			{
				vEnd = Transform(*skeleton.GetParentMtx(), context.m_avTargetPosition[COMP_EYE]);
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue1);
				ms_debugHelper.Sphere(RCC_VECTOR3(vEnd), 0.01f, Color_blue1, false);
				ms_debugHelper.Text3d(RCC_VECTOR3(vEnd), Color_white, 0, 0, "e", false); 
			}

			if (context.m_bRefDirRelative)
			{
				vEnd = Transform(*skeleton.GetParentMtx(), AddScaled(mtxHead.GetCol3(), context.m_vTargetDirRel, vExtrapolatedDistance));
				ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_SteelBlue);
				ms_debugHelper.Sphere(RCC_VECTOR3(vEnd), 0.01f, Color_SteelBlue, false);
				ms_debugHelper.Text3d(RCC_VECTOR3(vEnd), Color_white, 0, 0, "r", false); 
			}
		}

		if (ms_bRenderFacing)
		{
			const ScalarV vScalarOne(V_ONE);
			vStart = context.m_mtxReferenceDir.GetCol3();
			vEnd = AddScaled(vStart, context.m_mtxReferenceDir.GetCol0(), vScalarOne);
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_red4);
			vEnd = AddScaled(vStart, context.m_mtxReferenceDir.GetCol1(), vScalarOne);
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_green4);
			vEnd = AddScaled(vStart, context.m_mtxReferenceDir.GetCol2(), vScalarOne);
			ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue4);
		}
	}
#endif // __IK_DEV
}

void CBodyLookIkSolver::CalculateRelativeAngles(rage::crSkeleton& skeleton, SolverContext& context)
{
	const float fTimeStep = context.m_fDeltaTime;

	// Assume all enabled components are using the same relative direction (use the first relative direction found)
	bool bFacingDir = false;

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		if (context.m_auRefDirRel[compIndex] != 0)
		{
			bFacingDir = (m_aeRefDir[compIndex] == LOOKIK_REF_DIR_FACING);
			break;
		}
	}

	Mat34V mtxRefDir(m_pPed->GetTransform().GetMatrix());
	if (bFacingDir || m_bRefDirFacing)
	{
		// Facing dir is mover dir rotated by delta stored in facing dir bone (IK_Root)
		mtxRefDir = context.m_mtxReferenceDir;
		m_bRefDirFacing = true;
	}
	UnTransform3x3Full(mtxRefDir, *skeleton.GetParentMtx(), mtxRefDir);

	// Transform relative direction from object space to ref dir space in order to calculate reference to target rotation
	QuatV qReferenceToTarget(QuatVFromVectors(Vec3V(V_Y_AXIS_WZERO), Transform(context.m_mtxObjToRefDirSpace, context.m_vTargetDirRel)));
	context.m_vEulersAdditive = QuatVToEulersXYZ(qReferenceToTarget);
	ScalarV vCompZ(context.m_vEulersAdditive.GetZ());
	context.m_vEulersAdditive.SetZ(context.m_vEulersAdditive.GetX());
	context.m_vEulersAdditive.SetX(vCompZ);
	context.m_vEulersAdditive.SetY(Negate(context.m_vEulersAdditive.GetY()));

	// Store angle amount remaining after each enabled component subtracts its contribution
	context.m_vEulersAdditiveComp = Abs(context.m_vEulersAdditive);
	context.m_mtxReference = mtxRefDir.GetMat33ConstRef();

	// Build weight index based on enabled components
	u32 uRefDirWeightIndex = 0;
	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		u32 uValue = (context.m_auRefDirRel[compIndex] != 0) || (m_afRefDirBlend[compIndex] < 1.0f) ? 1 : 0;
		uRefDirWeightIndex |= uValue << compIndex;
	}

	if (*((u32*)m_auRefDirWeight) == 0xFFFFFFFF)
	{
		m_auRefDirWeight[0] = ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][0];
		m_auRefDirWeight[1] = ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][1];
		m_auRefDirWeight[2] = ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][2];
		m_auRefDirWeight[3] = ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][3];
	}

	// Interpolate weights for each component
	Vec4V vRefDirWeightsCurr((float)m_auRefDirWeight[0], (float)m_auRefDirWeight[1], (float)m_auRefDirWeight[2], (float)m_auRefDirWeight[3]);
	Vec4V vRefDirWeightsTarg((float)ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][0], (float)ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][1], (float)ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][2], (float)ms_auRefDirWeight[m_uBipedQuadruped][uRefDirWeightIndex][3]);

	Vec4V vDeltaMax(ScalarV(ms_fDeltaLimitBlendRate * fTimeStep));
	Vec4V vDelta(Subtract(vRefDirWeightsTarg, vRefDirWeightsCurr));

	Vec4V vInterp(InvertSafe(Abs(vDelta), Vec4V(V_ZERO)));
	vInterp = Scale(vInterp, vDeltaMax);
	vInterp = Min(vInterp, Vec4V(V_ONE));

	vRefDirWeightsCurr = Lerp(vInterp, vRefDirWeightsCurr, vRefDirWeightsTarg);

	m_auRefDirWeight[0] = (u8)vRefDirWeightsCurr[0];
	m_auRefDirWeight[1] = (u8)vRefDirWeightsCurr[1];
	m_auRefDirWeight[2] = (u8)vRefDirWeightsCurr[2];
	m_auRefDirWeight[3] = (u8)vRefDirWeightsCurr[3];

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "FACE YRP: %6.1f, %6.1f, %6.1f W: %u %u %u %u\n", context.m_vEulersAdditive.GetXf() * RtoD, context.m_vEulersAdditive.GetYf() * RtoD, context.m_vEulersAdditive.GetZf() * RtoD,
																			 m_auRefDirWeight[0], m_auRefDirWeight[1], m_auRefDirWeight[2], m_auRefDirWeight[3]);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

void CBodyLookIkSolver::CalculateRelativeAngleContribution(Component eComp, Vec3V_In vMinLimit, Vec3V_In vMaxLimit, SolverContext& context, Vec3V_InOut vEulers)
{
	const ScalarV vContributionWeight((float)m_auRefDirWeight[eComp] * 0.01f);
	const Vec3V vZero(V_ZERO);

	// Setup component limits and flip pitch limits
	Vec3V vMin(vMinLimit.GetX(), vMinLimit.GetY(), Negate(vMaxLimit.GetZ()));
	Vec3V vMax(vMaxLimit.GetX(), vMaxLimit.GetY(), Negate(vMinLimit.GetZ()));

	// Scale overall angles by component weight to get angle contributions by this component
	vEulers = Scale(context.m_vEulersAdditive, vContributionWeight);
	Vec3V vAbs(Abs(vEulers));

	// Sign of angles
	Vec3V vSign(InvertSafe(vAbs, vZero));
	vSign = Scale(vSign, vEulers);

	// Clamp angle contributions to angle amounts still remaining
	vEulers = Scale(Min(vAbs, context.m_vEulersAdditiveComp), vSign);

	// Clamp to component limits
	vEulers = Clamp(vEulers, vMin, vMax);

	// Update angle amounts still remaining
	context.m_vEulersAdditiveComp = Max(Subtract(context.m_vEulersAdditiveComp, Abs(vEulers)), vZero);
}

void CBodyLookIkSolver::CalculateRelativeAngleRotation(Component eComp, Vec3V_In vMinLimit, Vec3V_In vMaxLimit, SolverContext& context, QuatV_InOut qReferenceToTarget)
{
	Vec3V vEulers;

	CalculateRelativeAngleContribution(eComp, vMinLimit, vMaxLimit, context, vEulers);

	qReferenceToTarget = QuatVFromAxisAngle(context.m_mtxReference.GetCol1(), vEulers.GetY());
	qReferenceToTarget = Multiply(qReferenceToTarget, QuatVFromAxisAngle(context.m_mtxReference.GetCol2(), vEulers.GetX()));
	qReferenceToTarget = Multiply(qReferenceToTarget, QuatVFromAxisAngle(context.m_mtxReference.GetCol0(), vEulers.GetZ()));

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		formatf(debugText, sizeof(debugText), "REL ANG: %6.1f, %6.1f, %6.1f", vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, vEulers.GetZf() * RtoD);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

void CBodyLookIkSolver::SetDefaultYawPitch(Component eComp, Vec3V_ConstRef vEulers, Vec3V_Ref vEulersClamped, Vec3V avCurrentLimits[2], const CIKDeltaLimits& deltaLimits)
{
	int iLimit;
	float fLimit;
	float fInterp;

	// If the component solver is running but a subcomponent is off, then set the default angle,
	// which is a blend of the incoming local angle and the angle generated by the solver
	// (to avoid pops when subcomponents are disabled. The blend is calculated as the ratio
	// of the currently blended limit to the previously set limit (since the current limit
	// is blending to 0 if disabled).
	if (m_aeRotLim[eComp][LOOKIK_ROT_ANGLE_YAW] == LOOKIK_ROT_LIM_OFF)
	{
		// Determine which side of 0 the currently calculated angle is on
		iLimit = vEulersClamped.GetXf() >= 0.0f ? 1 : 0;

		fLimit = deltaLimits.m_DeltaLimits[m_aeRotLimPrev[eComp][LOOKIK_ROT_ANGLE_YAW]][iLimit][0];
		fInterp = fLimit != 0.0f ? avCurrentLimits[iLimit][0] / fLimit : 0.0f;
		fInterp = Clamp(fInterp, 0.0f, 1.0f);

		vEulersClamped.SetXf(Lerp(fInterp, vEulers.GetXf(), vEulersClamped.GetXf()));
		vEulersClamped.SetYf(Lerp(fInterp, vEulers.GetYf(), vEulersClamped.GetYf()));
	}

	if (m_aeRotLim[eComp][LOOKIK_ROT_ANGLE_PITCH] == LOOKIK_ROT_LIM_OFF)
	{
		iLimit = vEulersClamped.GetZf() >= 0.0f ? 0 : 1;

		fLimit = -deltaLimits.m_DeltaLimits[m_aeRotLimPrev[eComp][LOOKIK_ROT_ANGLE_PITCH]][iLimit][2];
		fInterp = fLimit != 0.0f ? -avCurrentLimits[iLimit][2] / fLimit : 0.0f;
		fInterp = Clamp(fInterp, 0.0f, 1.0f);

		vEulersClamped.SetZf(Lerp(fInterp, vEulers.GetZf(), vEulersClamped.GetZf()));
	}
}

void CBodyLookIkSolver::ScaleRollLimit(u32 uYawClamped, Vec3V_In vEulersXYZ, float& fCurrentRollLimitScale, Vec3V_Ref vMinLimit, Vec3V_Ref vMaxLimit, float fDeltaTime)
{
	const ScalarV vScalarOne(V_ONE);
	const ScalarV vScalarZero(V_ZERO);

	ScalarV vScaleVal(V_ONE);
	ScalarV vAbsYaw(Abs(vEulersXYZ.GetX()));

	if ((uYawClamped != 0) && IsLessThanAll(vAbsYaw, vMaxLimit.GetX()))
	{
		// Scale roll limits by 1.0 - (YawDelta/MaxYawDelta)
		vScaleVal = Scale(InvertSafe(vMaxLimit.GetX(), vScalarZero), vAbsYaw);
		vScaleVal = Subtract(vScalarOne, vScaleVal);
	}

	ScalarV vRollLimitScale(fCurrentRollLimitScale);
	if (!IsEqualAll(vRollLimitScale, vScaleVal))
	{
		ScalarV vInterp(InvertSafe(Abs(Subtract(vScaleVal, vRollLimitScale)), vScalarZero));
		vInterp = Scale(vInterp, ScalarV(fDeltaTime * ms_fRollLimitBlendRate));
		vInterp = Min(vInterp, vScalarOne);

		vRollLimitScale = Lerp(vInterp, vRollLimitScale, vScaleVal);
	}

	vMinLimit.SetY(Scale(vMinLimit.GetY(), vRollLimitScale));
	vMaxLimit.SetY(Scale(vMaxLimit.GetY(), vRollLimitScale));

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "ROLL: Scale %5.3f -> %5.3f Limit %6.1f\n", vRollLimitScale.Getf(), vScaleVal.Getf(), vMaxLimit.GetYf() * RtoD);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	fCurrentRollLimitScale = vRollLimitScale.Getf();
}

void CBodyLookIkSolver::ResetTargetPositions()
{
	const Vec2V vZero(V_ZERO);

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		m_avYawPitch[compIndex] = vZero;
		m_avYawPitchVel[compIndex] = vZero;
	}

	m_vYawPitchRel = vZero;
	m_vYawPitchRelVel = vZero;
}

void CBodyLookIkSolver::ResetOnActivate()
{
	InitializeDeltaLimits();
	ResetTargetPositions();
	memset(m_auRefDirWeight, 255, COMP_NUM);

	m_afArmCompBlend[LEFT_ARM] = 1.0f;
	m_afArmCompBlend[RIGHT_ARM] = 1.0f;

	for (int compIndex = 0; compIndex < COMP_NUM; ++compIndex)
	{
		m_afRefDirBlend[compIndex] = (m_uBipedQuadruped == 0) && (m_aeRefDir[compIndex] == 0) ? 1.0f : 0.0f;
	}
}

void CBodyLookIkSolver::CanUseComponents(SolverContext& context, bool abCanUseComponent[COMP_NUM])
{
	const bool bScript = ((m_pProxy->GetFlags() & LOOKIK_SCRIPT_REQUEST) != 0) || (m_pProxy->GetHoldTime() == (u16)-1);

	const float kfMaxSqDistanceTorso = square(8.0f);
	const float kfMaxSqDistanceHead = square(25.0f);
	const float kfMaxSqDistanceEye = square(15.0f);

	abCanUseComponent[COMP_TORSO] = (bScript || (context.m_fDistSqToCamera < kfMaxSqDistanceTorso)) && !m_bAiming;
	abCanUseComponent[COMP_NECK] = bScript || (context.m_fDistSqToCamera < kfMaxSqDistanceHead);
	abCanUseComponent[COMP_HEAD] = abCanUseComponent[COMP_NECK];

	abCanUseComponent[COMP_EYE] = (m_auEyeBoneIdx[LEFT_EYE] != (u16)-1) && (bScript || (context.m_fDistSqToCamera < kfMaxSqDistanceEye));

	if (abCanUseComponent[COMP_EYE] && ((m_pProxy->GetFlags() & LOOKIK_DISABLE_EYE_OPTIMIZATION) == 0))
	{
		Vec3V vNegForward(Negate(context.m_mtxReferenceDir.GetCol1()));
		abCanUseComponent[COMP_EYE] &= (camInterface::GetFront().Dot(RC_VECTOR3(vNegForward)) >= -0.707f);	// ~135deg
	}
}

#if __IK_DEV
void CBodyLookIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

bool CBodyLookIkSolver::CaptureDebugRender()
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

void CBodyLookIkSolver::AddWidgets(bkBank* pBank)
{
	pBank->PushGroup("CBodyLookIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeBodyLook], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");
		pBank->AddToggle("Debug Target", &ms_bRenderTargetPosition, NullCB, "");
		pBank->AddToggle("Debug Facing", &ms_bRenderFacing, NullCB, "");
		pBank->AddToggle("Debug Motion", &ms_bRenderMotion, NullCB, "");

#if ENABLE_LOOKIK_SOLVER_WIDGETS
		pBank->PushGroup("Delta Limits (Degrees)", false);
			static const char* sRotLimit[] = { "Narrowest", "Narrow", "Wide", "Widest" };
			static const char* sPedType[] = { "Biped", "Quadruped" };

			pBank->PushGroup("Torso", false);
				for (int rotLimit = LOOKIK_ROT_LIM_NARROWEST; rotLimit <= LOOKIK_ROT_LIM_WIDEST; ++rotLimit)
				{
					pBank->PushGroup(sRotLimit[rotLimit - 1], false);
						pBank->AddSlider("Yaw Min", &ms_avTorsoDeltaLimits[rotLimit][0][0], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Yaw Max", &ms_avTorsoDeltaLimits[rotLimit][1][0], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Roll Min", &ms_avTorsoDeltaLimits[rotLimit][0][1], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Roll Max", &ms_avTorsoDeltaLimits[rotLimit][1][1], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Pitch Min", &ms_avTorsoDeltaLimits[rotLimit][0][2], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Pitch Max", &ms_avTorsoDeltaLimits[rotLimit][1][2], -180.0f, 180.0f, 1.0f);
					pBank->PopGroup();
				}
			pBank->PopGroup();
			pBank->PushGroup("Neck", false);
				for (int pedType = 0; pedType < 2; ++pedType)
				{
					pBank->PushGroup(sPedType[pedType], false);
					for (int rotLimit = LOOKIK_ROT_LIM_NARROWEST; rotLimit <= LOOKIK_ROT_LIM_WIDEST; ++rotLimit)
					{
						pBank->PushGroup(sRotLimit[rotLimit - 1], false);
							pBank->AddSlider("Yaw Min", &ms_avNeckDeltaLimits[pedType][rotLimit][0][0], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Yaw Max", &ms_avNeckDeltaLimits[pedType][rotLimit][1][0], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Roll Min", &ms_avNeckDeltaLimits[pedType][rotLimit][0][1], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Roll Max", &ms_avNeckDeltaLimits[pedType][rotLimit][1][1], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Pitch Min", &ms_avNeckDeltaLimits[pedType][rotLimit][0][2], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Pitch Max", &ms_avNeckDeltaLimits[pedType][rotLimit][1][2], -180.0f, 180.0f, 1.0f);
						pBank->PopGroup();
					}
					pBank->PopGroup();
				}
			pBank->PopGroup();
			pBank->PushGroup("Head", false);
				for (int pedType = 0; pedType < 2; ++pedType)
				{
					pBank->PushGroup(sPedType[pedType], false);
					for (int rotLimit = LOOKIK_ROT_LIM_NARROWEST; rotLimit <= LOOKIK_ROT_LIM_WIDEST; ++rotLimit)
					{
						pBank->PushGroup(sRotLimit[rotLimit - 1], false);
							pBank->AddSlider("Yaw Min", &ms_avHeadDeltaLimits[pedType][rotLimit][0][0], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Yaw Max", &ms_avHeadDeltaLimits[pedType][rotLimit][1][0], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Roll Min", &ms_avHeadDeltaLimits[pedType][rotLimit][0][1], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Roll Max", &ms_avHeadDeltaLimits[pedType][rotLimit][1][1], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Pitch Min", &ms_avHeadDeltaLimits[pedType][rotLimit][0][2], -180.0f, 180.0f, 1.0f);
							pBank->AddSlider("Pitch Max", &ms_avHeadDeltaLimits[pedType][rotLimit][1][2], -180.0f, 180.0f, 1.0f);
						pBank->PopGroup();
					}
					pBank->PopGroup();
				}
			pBank->PopGroup();
		pBank->PopGroup();

		pBank->PushGroup("Joint Limits (Degrees)", false);
			pBank->PushGroup("Torso", false);
				char szLabel[16];
				for (int spineBoneIndex = 0; spineBoneIndex < NUM_SPINE_PARTS; ++spineBoneIndex)
				{
					sprintf(szLabel, "Spine %d", spineBoneIndex);
					pBank->PushGroup(szLabel, false);
						pBank->AddSlider("Yaw Min", &ms_avTorsoJointLimits[spineBoneIndex][0][0], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Yaw Max", &ms_avTorsoJointLimits[spineBoneIndex][1][0], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Roll Min", &ms_avTorsoJointLimits[spineBoneIndex][0][1], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Roll Max", &ms_avTorsoJointLimits[spineBoneIndex][1][1], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Pitch Min", &ms_avTorsoJointLimits[spineBoneIndex][0][2], -180.0f, 180.0f, 1.0f);
						pBank->AddSlider("Pitch Max", &ms_avTorsoJointLimits[spineBoneIndex][1][2], -180.0f, 180.0f, 1.0f);
					pBank->PopGroup();
				}
			pBank->PopGroup();
			pBank->PushGroup("Neck", false);
				pBank->AddSlider("Yaw Min", &ms_avNeckJointLimits[0][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Yaw Max", &ms_avNeckJointLimits[1][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Min", &ms_avNeckJointLimits[0][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Max", &ms_avNeckJointLimits[1][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Min", &ms_avNeckJointLimits[0][2], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Max", &ms_avNeckJointLimits[1][2], -180.0f, 180.0f, 1.0f);
			pBank->PopGroup();
			pBank->PushGroup("Head", false);
				pBank->AddSlider("Yaw Min", &ms_avHeadJointLimits[0][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Yaw Max", &ms_avHeadJointLimits[1][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Min", &ms_avHeadJointLimits[0][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Max", &ms_avHeadJointLimits[1][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Min", &ms_avHeadJointLimits[0][2], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Max", &ms_avHeadJointLimits[1][2], -180.0f, 180.0f, 1.0f);
			pBank->PopGroup();
			pBank->PushGroup("Eye", false);
				pBank->AddSlider("Yaw Min", &ms_avEyeJointLimits[0][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Yaw Max", &ms_avEyeJointLimits[1][0], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Min", &ms_avEyeJointLimits[0][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Roll Max", &ms_avEyeJointLimits[1][1], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Min", &ms_avEyeJointLimits[0][2], -180.0f, 180.0f, 1.0f);
				pBank->AddSlider("Pitch Max", &ms_avEyeJointLimits[1][2], -180.0f, 180.0f, 1.0f);
			pBank->PopGroup();
		pBank->PopGroup();

		pBank->PushGroup("Blend Rates", false);
			pBank->AddSlider("Slowest", &ms_afBlendRate[LOOKIK_BLEND_RATE_SLOWEST], 0.0f, 100.0f, 0.01f);
			pBank->AddSlider("Slow", &ms_afBlendRate[LOOKIK_BLEND_RATE_SLOW], 0.0f, 100.0f, 0.01f);
			pBank->AddSlider("Normal", &ms_afBlendRate[LOOKIK_BLEND_RATE_NORMAL], 0.0f, 100.0f, 0.01f);
			pBank->AddSlider("Fast", &ms_afBlendRate[LOOKIK_BLEND_RATE_FAST], 0.0f, 100.0f, 0.01f);
			pBank->AddSlider("Fastest", &ms_afBlendRate[LOOKIK_BLEND_RATE_FASTEST], 0.0f, 100.0f, 0.01f);
			pBank->PushGroup("Blend Rate Scale", false);
				pBank->AddSlider("Torso", &ms_afSolverBlendRateScale[COMP_TORSO], 0.0f, 100.0f, 0.01f);
				pBank->AddSlider("Neck", &ms_afSolverBlendRateScale[COMP_NECK], 0.0f, 100.0f, 0.01f);
				pBank->AddSlider("Head", &ms_afSolverBlendRateScale[COMP_HEAD], 0.0f, 100.0f, 0.01f);
				pBank->AddSlider("Eye", &ms_afSolverBlendRateScale[COMP_EYE], 0.0f, 100.0f, 0.01f);
			pBank->PopGroup();
			pBank->AddSeparator();
			pBank->AddSlider("Delta Limit Rate", &ms_fDeltaLimitBlendRate, 0.0f, 100.0f, 0.1f);
			pBank->PushGroup("Ref Dir Blend Rate", false);
				pBank->AddSlider("Torso LookDir", &ms_afRefDirBlendRate[COMP_TORSO][0], 0.0f, 100.0f, 1.0f);
				pBank->AddSlider("Torso FaceDir", &ms_afRefDirBlendRate[COMP_TORSO][1], -100.0f, 0.0f, 1.0f);
				pBank->AddSlider("Neck LookDir", &ms_afRefDirBlendRate[COMP_NECK][0], 0.0f, 100.0f, 1.0f);
				pBank->AddSlider("Neck FaceDir", &ms_afRefDirBlendRate[COMP_NECK][1], -100.0f, 0.0f, 1.0f);
				pBank->AddSlider("Head LookDir", &ms_afRefDirBlendRate[COMP_HEAD][0], 0.0f, 100.0f, 1.0f);
				pBank->AddSlider("Head FaceDir", &ms_afRefDirBlendRate[COMP_HEAD][1], -100.0f, 0.0f, 1.0f);
				pBank->AddSlider("Eye LookDir", &ms_afRefDirBlendRate[COMP_EYE][0], 0.0f, 100.0f, 1.0f);
				pBank->AddSlider("Eye FaceDir", &ms_afRefDirBlendRate[COMP_EYE][1], -100.0f, 0.0f, 1.0f);
			pBank->PopGroup();
			pBank->AddSlider("Roll Limit Rate", &ms_fRollLimitBlendRate, 0.0f, 100.0f, 1.0f);
		pBank->PopGroup();

		pBank->PushGroup("Motion", false);
			pBank->PushGroup("Turn Rate Scale", false);
				pBank->AddSlider("Slow", &ms_afTargetSpringScale[LOOKIK_TURN_RATE_SLOW], 0.0f, 10.0f, 0.1f);
				pBank->AddSlider("Normal", &ms_afTargetSpringScale[LOOKIK_TURN_RATE_NORMAL], 0.0f, 10.0f, 0.1f);
				pBank->AddSlider("Fast", &ms_afTargetSpringScale[LOOKIK_TURN_RATE_FAST], 0.0f, 10.0f, 0.1f);
				pBank->AddSlider("Fastest", &ms_afTargetSpringScale[LOOKIK_TURN_RATE_FASTEST], 0.0f, 10.0f, 0.1f);
			pBank->PopGroup();
			pBank->PushGroup("Spring", false);
				pBank->AddSlider("Torso", &ms_afCompSpring[COMP_TORSO], 0.0f, 500.0f, 1.0f);
				pBank->AddSlider("Neck", &ms_afCompSpring[COMP_NECK], 0.0f, 500.0f, 1.0f);
				pBank->AddSlider("Head", &ms_afCompSpring[COMP_HEAD], 0.0f, 500.0f, 1.0f);
				pBank->AddSlider("Eyes", &ms_afCompSpring[COMP_EYE], 0.0f, 500.0f, 1.0f);
				pBank->AddSlider("RefDirRel", &ms_afCompSpringRel, 0.0f, 500.0f, 1.0f);
			pBank->PopGroup();
			pBank->PushGroup("Damping", false);
				pBank->AddSlider("Torso", &ms_afCompDamping[COMP_TORSO], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Neck", &ms_afCompDamping[COMP_NECK], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Head", &ms_afCompDamping[COMP_HEAD], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Eyes", &ms_afCompDamping[COMP_EYE], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("RefDirRel", &ms_afCompDampingRel, 0.0f, 100.0f, 0.1f);
			pBank->PopGroup();
			pBank->PushGroup("Inv Mass", false);
				pBank->AddSlider("Torso", &ms_afCompInvMass[COMP_TORSO], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Neck", &ms_afCompInvMass[COMP_NECK], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Head", &ms_afCompInvMass[COMP_HEAD], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("Eyes", &ms_afCompInvMass[COMP_EYE], 0.0f, 100.0f, 0.1f);
				pBank->AddSlider("RefDirRel", &ms_afCompInvMassRel, 0.0f, 100.0f, 0.1f);
			pBank->PopGroup();
			pBank->AddSlider("Char Speed Modifier", &ms_fCharSpeedModifier, 0.0f, 100.0f, 0.1f);
			pBank->AddSlider("Char Ang Speed Modifier", &ms_fCharAngSpeedModifier, 0.0f, 100.0f, 0.1f);
			pBank->AddSlider("Char Ang Speed Counter Modifier (Still)", &ms_afCharAngSpeedCounterModifier[0], 0.0f, 100.0f, 0.1f);
			pBank->AddSlider("Char Ang Speed Counter Modifier (Moving)", &ms_afCharAngSpeedCounterModifier[1], 0.0f, 100.0f, 0.1f);
		pBank->PopGroup();

		pBank->PushGroup("Spine Weights (Sum = 1.0)", false);
			pBank->AddSlider("Spine 0", &ms_afSpineWeight[SPINE0], 0.0f, 1.0f, 0.01f);
			pBank->AddSlider("Spine 1", &ms_afSpineWeight[SPINE1], 0.0f, 1.0f, 0.01f);
			pBank->AddSlider("Spine 2", &ms_afSpineWeight[SPINE2], 0.0f, 1.0f, 0.01f);
			pBank->AddSlider("Spine 3", &ms_afSpineWeight[SPINE3], 0.0f, 1.0f, 0.01f);
		pBank->PopGroup();

		pBank->PushGroup("Head Attitude / Arm Compensation Weights", false);
			pBank->AddSlider("Low", &ms_afHeadAttitudeWeight[LOOKIK_HEAD_ATT_LOW], 0.0f, 1.0f, 0.01f);
			pBank->AddSlider("Med", &ms_afHeadAttitudeWeight[LOOKIK_HEAD_ATT_MED], 0.0f, 1.0f, 0.01f);
			pBank->AddSlider("Full", &ms_afHeadAttitudeWeight[LOOKIK_HEAD_ATT_FULL], 0.0f, 1.0f, 0.01f);
		pBank->PopGroup();

		pBank->PushGroup("Tunables", false);
			pBank->AddSlider("Target Yaw Limit (Deg)", &ms_afAngLimit[0], 0.0f, 180.0f, 1.0f);
			pBank->AddSlider("Target Yaw Limit (Water) (Deg)", &ms_afAngLimit[1], 0.0f, 180.0f, 1.0f);
		pBank->PopGroup();
#endif // ENABLE_LOOKIK_SOLVER_WIDGETS

	pBank->PopGroup();
}
#endif //__IK_DEV
