// 
// ik/IkRequest.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "IkRequest.h"

#include "IkManager.h"
#include "scene/Entity.h"

ANIM_OPTIMISATIONS();

///////////////////////////////////////////////////////////////////////////////

CIkRequestBodyLook::CIkRequestBodyLook()
: m_qHeadNeckAdditive(V_IDENTITY)
, m_vTargetOffset(V_ZERO_WONE)
, m_pTargetEntity(NULL)
, m_eTargetBoneTag(BONETAG_INVALID)
, m_uFlags(0)
, m_sLookAtFlags(-1)
, m_uHashKey(0)
, m_uHoldTimeMs(0)
, m_sRequestPriority((s16)CIkManager::IK_LOOKAT_MEDIUM)
, m_fHeadNeckAdditiveRatio(1.0f)
, m_bInstant(false)
{
	Default();
}

CIkRequestBodyLook::CIkRequestBodyLook(const CEntity* pTargetEntity,
									   eAnimBoneTag eTargetBoneTag,
									   Vec3V_In vTargetOffset,
									   u32 uFlags, 
									   s32 sRequestPriority,
									   u32 uHashKey)
: m_qHeadNeckAdditive(V_IDENTITY)
, m_vTargetOffset(vTargetOffset)
, m_pTargetEntity(pTargetEntity)
, m_eTargetBoneTag(eTargetBoneTag)
, m_uFlags(uFlags)
, m_sLookAtFlags(-1)
, m_uHashKey(uHashKey)
, m_uHoldTimeMs(0)
, m_sRequestPriority((s16)sRequestPriority)
, m_fHeadNeckAdditiveRatio(1.0f)
, m_bInstant(false)
{
	animAssertf(IsFiniteAll(vTargetOffset), "Invalid target offset for look request (%8.3f %8.3f %8.3f)", vTargetOffset.GetXf(), vTargetOffset.GetYf(), vTargetOffset.GetZf());

	Default();
}

void CIkRequestBodyLook::SetHeadNeckAdditiveRatio(float fRatio)
{
	m_fHeadNeckAdditiveRatio = Clamp(fRatio, 0.0f, 1.0f);
}

void CIkRequestBodyLook::SetTargetOffset(Vec3V_In vTargetOffset)
{
	animAssertf(IsFiniteAll(vTargetOffset), "Invalid target offset for look request (%8.3f %8.3f %8.3f)", vTargetOffset.GetXf(), vTargetOffset.GetYf(), vTargetOffset.GetZf());
	m_vTargetOffset = vTargetOffset;
}

void CIkRequestBodyLook::Default()
{
	m_uRefDirTorso = LOOKIK_REF_DIR_ABSOLUTE;
	m_uRefDirNeck = LOOKIK_REF_DIR_ABSOLUTE;
	m_uRefDirHead = LOOKIK_REF_DIR_ABSOLUTE;
	m_uRefDirEye = LOOKIK_REF_DIR_ABSOLUTE;

	m_uTurnRate = LOOKIK_TURN_RATE_NORMAL;
	m_uBlendInRate = LOOKIK_BLEND_RATE_NORMAL;
	m_uBlendOutRate = LOOKIK_BLEND_RATE_NORMAL;

	m_uHeadAttitude = LOOKIK_HEAD_ATT_FULL;
	memset(m_auRotLim, 0, 3 * LOOKIK_ROT_ANGLE_NUM);

	// Default head component to wide limits
	m_auRotLim[2][0] = LOOKIK_ROT_LIM_WIDE;
	m_auRotLim[2][1] = LOOKIK_ROT_LIM_WIDE;

	m_auArmComp[0] = LOOKIK_ARM_COMP_OFF;
	m_auArmComp[1] = LOOKIK_ARM_COMP_OFF;
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestArm::CIkRequestArm()
: m_vTargetOffset(V_ZERO)
, m_qTargetRotation(V_IDENTITY)
, m_qAdditive(V_IDENTITY)
, m_pTargetEntity(NULL)
, m_eTargetBoneTag(BONETAG_INVALID)
, m_fBlendInRange(0.0f)
, m_fBlendOutRange(0.0f)
, m_uFlags(0)
, m_uArm(IK_ARM_RIGHT)
{
	Default();
}

CIkRequestArm::CIkRequestArm(IkArm eArm, const CEntity* pTargetEntity, eAnimBoneTag eTargetBoneTag, Vec3V_In vTargetOffset, u32 uFlags)
: m_vTargetOffset(vTargetOffset)
, m_qTargetRotation(V_IDENTITY)
, m_qAdditive(V_IDENTITY)
, m_pTargetEntity(pTargetEntity)
, m_eTargetBoneTag(eTargetBoneTag)
, m_fBlendInRange(0.0f)
, m_fBlendOutRange(0.0f)
, m_uFlags(uFlags)
, m_uArm((u8)eArm)
{
	animAssertf(IsFiniteAll(vTargetOffset), "Invalid target offset for arm request (%8.3f %8.3f %8.3f)", vTargetOffset.GetXf(), vTargetOffset.GetYf(), vTargetOffset.GetZf());

	Default();
}

CIkRequestArm::CIkRequestArm(IkArm eArm, const CEntity* pTargetEntity, eAnimBoneTag eTargetBoneTag, Vec3V_In vTargetOffset, QuatV_In qTargetRotation, u32 uFlags)
: m_vTargetOffset(vTargetOffset)
, m_qTargetRotation(qTargetRotation)
, m_qAdditive(V_IDENTITY)
, m_pTargetEntity(pTargetEntity)
, m_eTargetBoneTag(eTargetBoneTag)
, m_fBlendInRange(0.0f)
, m_fBlendOutRange(0.0f)
, m_uFlags(uFlags)
, m_uArm((u8)eArm)
{
	animAssertf(IsFiniteAll(vTargetOffset), "Invalid target offset for arm request (%8.3f %8.3f %8.3f)", vTargetOffset.GetXf(), vTargetOffset.GetYf(), vTargetOffset.GetZf());
	animAssertf(IsFiniteAll(qTargetRotation), "Invalid target rotation for arm request (%8.3f %8.3f %8.3f %8.3f)", qTargetRotation.GetXf(), qTargetRotation.GetYf(), qTargetRotation.GetZf(), qTargetRotation.GetWf());

	Default();
}

void CIkRequestArm::Default()
{
	m_uBlendInRate = ARMIK_BLEND_RATE_NORMAL;
	m_uBlendOutRate = ARMIK_BLEND_RATE_NORMAL;
	m_bBlendInRateIsEnum = true;
	m_bBlendOutRateIsEnum = true;
}

void CIkRequestArm::SetTargetOffset(Vec3V_In vTargetOffset)
{
	animAssertf(IsFiniteAll(vTargetOffset), "Invalid target offset for arm request (%8.3f %8.3f %8.3f)", vTargetOffset.GetXf(), vTargetOffset.GetYf(), vTargetOffset.GetZf());
	m_vTargetOffset = vTargetOffset;
}

void CIkRequestArm::SetTargetRotation(QuatV_In qRotation)
{
	animAssertf(IsFiniteAll(qRotation), "Invalid target rotation for arm request (%8.3f %8.3f %8.3f %8.3f)", qRotation.GetXf(), qRotation.GetYf(), qRotation.GetZf(), qRotation.GetWf());
	m_qTargetRotation = qRotation;
}

void CIkRequestArm::SetAdditive(QuatV_In qAdditive)
{
	animAssertf(IsFiniteAll(qAdditive), "Invalid additive rotation for arm request (%8.3f %8.3f %8.3f %8.3f)", qAdditive.GetXf(), qAdditive.GetYf(), qAdditive.GetZf(), qAdditive.GetWf());
	m_qAdditive = qAdditive;
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestResetArm::CIkRequestResetArm(IkArm eArm)
: m_uArm((u8)eArm)
{
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestBodyReact::CIkRequestBodyReact()
: m_vPosition(V_ZERO)
, m_vDirection(V_ZERO)
, m_Component(0)
, m_WeaponGroup(0)
, m_bLocalInflictor(true)
{
}

CIkRequestBodyReact::CIkRequestBodyReact(Vec3V_In vPosition,
										 Vec3V_In vDirection,
										 const int component)
: m_vPosition(vPosition)
, m_vDirection(vDirection)
, m_Component(component)
, m_WeaponGroup(0)
, m_bLocalInflictor(true)
{
	animAssertf(IsFiniteAll(vPosition) && IsFiniteAll(vDirection), "Invalid position and/or direction for body react request (%8.3f %8.3f %8.3f) (%8.3f %8.3f %8.3f)", 
		vPosition.GetXf(), vPosition.GetYf(), vPosition.GetZf(),
		vDirection.GetXf(), vDirection.GetYf(), vDirection.GetZf());
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestBodyRecoil::CIkRequestBodyRecoil(u32 uFlags)
: m_uFlags(uFlags)
{
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestLeg::CIkRequestLeg()
#if FPS_MODE_SUPPORTED
: m_fPelvisOffset(0.0f)
, m_uMode(LEG_IK_MODE_PARTIAL)
#else
: m_uMode(LEG_IK_MODE_PARTIAL)
#endif // FPS_MODE_SUPPORTED
{
}

///////////////////////////////////////////////////////////////////////////////

CIkRequestTorsoVehicle::CIkRequestTorsoVehicle()
: m_fAnimatedLean(0.5f)
, m_fReachLimitPercentage(0.95f)
, m_fDeltaScale(1.0f)
, m_uFlags(0)
{
}
