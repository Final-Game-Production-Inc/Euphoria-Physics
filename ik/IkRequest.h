// 
// ik/IkRequest.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
// 

#ifndef IKREQUEST_H
#define	IKREQUEST_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
///////////////////////////////////////////////////////////////////////////////

// Rage headers
#include "fwanimation/boneids.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"

// Game headers
#include "ik/IkConfig.h"
#include "Peds/pedDefines.h"

class CEntity;

///////////////////////////////////////////////////////////////////////////////

enum IkArm
{
	IK_ARM_LEFT = 0,
	IK_ARM_RIGHT,
	IK_ARM_NUM
};

///////////////////////////////////////////////////////////////////////////////
enum LookIkReferenceDirection
{
	LOOKIK_REF_DIR_ABSOLUTE = 0,
	LOOKIK_REF_DIR_FACING,
	LOOKIK_REF_DIR_MOVER,
	LOOKIK_REF_DIR_HEAD
};

enum LookIkRotationLimit
{
	LOOKIK_ROT_LIM_OFF = 0,
	LOOKIK_ROT_LIM_NARROWEST,
	LOOKIK_ROT_LIM_NARROW,
	LOOKIK_ROT_LIM_WIDE,
	LOOKIK_ROT_LIM_WIDEST,
	LOOKIK_ROT_LIM_NUM
};

enum LookIkArmCompensation
{
	LOOKIK_ARM_COMP_OFF,
	LOOKIK_ARM_COMP_LOW,
	LOOKIK_ARM_COMP_MED,
	LOOKIK_ARM_COMP_FULL,
	LOOKIK_ARM_COMP_IK,
};

enum LookIkHeadAttitude
{
	LOOKIK_HEAD_ATT_OFF,
	LOOKIK_HEAD_ATT_LOW,
	LOOKIK_HEAD_ATT_MED,
	LOOKIK_HEAD_ATT_FULL,
	LOOKIK_HEAD_ATT_NUM
};

enum LookIkArm
{
	LOOKIK_ARM_LEFT = 0,
	LOOKIK_ARM_RIGHT,
	LOOKIK_ARM_NUM
};

enum LookIkRotationAngle
{
	LOOKIK_ROT_ANGLE_YAW = 0,
	LOOKIK_ROT_ANGLE_PITCH,
	LOOKIK_ROT_ANGLE_NUM
};

enum LookIkTurnRate
{
	LOOKIK_TURN_RATE_SLOW = 0,
	LOOKIK_TURN_RATE_NORMAL,
	LOOKIK_TURN_RATE_FAST,
	LOOKIK_TURN_RATE_FASTEST,
	LOOKIK_TURN_RATE_INSTANT,
	LOOKIK_TURN_RATE_NUM
};

enum LookIkBlendRate
{
	LOOKIK_BLEND_RATE_SLOWEST = 0,
	LOOKIK_BLEND_RATE_SLOW,
	LOOKIK_BLEND_RATE_NORMAL,
	LOOKIK_BLEND_RATE_FAST,
	LOOKIK_BLEND_RATE_FASTEST,
	LOOKIK_BLEND_RATE_INSTANT,
	LOOKIK_BLEND_RATE_NUM
};

enum LookIkFlags
{
	LOOKIK_USE_ANIM_TAGS			= (1 << 0),
	LOOKIK_USE_ANIM_ALLOW_TAGS		= (1 << 1),
	LOOKIK_USE_ANIM_BLOCK_TAGS		= (1 << 2),
	LOOKIK_USE_CAMERA_FOCUS			= (1 << 3),
	LOOKIK_USE_FOV					= (1 << 4),
	LOOKIK_SCRIPT_REQUEST			= (1 << 5),
	LOOKIK_BLOCK_ANIM_TAGS			= (1 << 6),
	LOOKIK_DISABLE_EYE_OPTIMIZATION	= (1 << 7)
};

class CIkRequestBodyLook
{
	friend class CBodyLookIkSolver;
	friend class CBodyLookIkSolverProxy;

public:
	CIkRequestBodyLook();
	CIkRequestBodyLook(const CEntity* pTargetEntity,
					   eAnimBoneTag eTargetBoneTag,
					   Vec3V_In vTargetOffset = Vec3V(V_ZERO_WONE),
					   u32 uFlags = 0,
					   s32 sRequestPriority = 3,
					   u32 uHashKey = 0);

	void SetHeadNeckAdditive(QuatV_In qAdditive)			{ m_qHeadNeckAdditive = qAdditive; }
	void SetHeadNeckAdditiveRatio(float fRatio);

	void SetTargetEntity(const CEntity* pEntity)			{ m_pTargetEntity = pEntity; }
	void SetTargetBoneTag(eAnimBoneTag eTargetBoneTag)		{ m_eTargetBoneTag = eTargetBoneTag; }
	void SetTargetOffset(Vec3V_In vTargetOffset);

	void SetFlags(u32 uFlags)								{ m_uFlags = uFlags; }
	void SetLookAtFlags(s32 sFlags)							{ m_sLookAtFlags = sFlags; }
	void SetHashKey(u32 uHashKey)							{ m_uHashKey = uHashKey; }
	void SetRequestPriority(s32 sPriority)					{ m_sRequestPriority = (s16)sPriority; }
	void SetHoldTime(u16 uHoldTimeMs)						{ m_uHoldTimeMs = uHoldTimeMs; }

	void SetRefDirTorso(LookIkReferenceDirection eRefDir)	{ m_uRefDirTorso = (u8)eRefDir; }
	void SetRefDirNeck(LookIkReferenceDirection eRefDir)	{ m_uRefDirNeck = (u8)eRefDir; }
	void SetRefDirHead(LookIkReferenceDirection eRefDir)	{ m_uRefDirHead = (u8)eRefDir; }
	void SetRefDirEye(LookIkReferenceDirection eRefDir)		{ m_uRefDirEye = (u8)eRefDir; }

	void SetTurnRate(LookIkTurnRate eRate)					{ m_uTurnRate = (u8)eRate; }
	void SetBlendInRate(LookIkBlendRate eRate)				{ m_uBlendInRate = (u8)eRate; }
	void SetBlendOutRate(LookIkBlendRate eRate)				{ m_uBlendOutRate = (u8)eRate; }

	void SetHeadAttitude(LookIkHeadAttitude eAttitude)		{ m_uHeadAttitude = (u8)eAttitude; }

	void SetRotLimTorso(LookIkRotationAngle eType, LookIkRotationLimit eLimit)	{ m_auRotLim[0][eType] = (u8)eLimit; }
	void SetRotLimNeck(LookIkRotationAngle eType, LookIkRotationLimit eLimit)	{ m_auRotLim[1][eType] = (u8)eLimit; }
	void SetRotLimHead(LookIkRotationAngle eType, LookIkRotationLimit eLimit)	{ m_auRotLim[2][eType] = (u8)eLimit; }

	void SetArmCompensation(LookIkArm eArm, LookIkArmCompensation eComp)		{ m_auArmComp[eArm] = (u8)eComp; }

	void SetInstant(bool bInstant)							{ m_bInstant = bInstant; }

	const QuatV& GetHeadNeckAdditive() const		{ return m_qHeadNeckAdditive; }
	float GetHeadNeckAdditiveRatio() const			{ return m_fHeadNeckAdditiveRatio; }

	const CEntity* GetTargetEntity() const			{ return m_pTargetEntity; }
	eAnimBoneTag GetTargetBoneTag() const			{ return m_eTargetBoneTag; }
	const Vec3V& GetTargetOffset() const			{ return m_vTargetOffset; }

	u32 GetFlags() const							{ return m_uFlags; }
	s32 GetLookAtFlags() const						{ return m_sLookAtFlags; }
	u32 GetHashKey() const							{ return m_uHashKey; }
	s32 GetRequestPriority() const					{ return (s32)m_sRequestPriority; }
	u16 GetHoldTime() const							{ return m_uHoldTimeMs; }

	LookIkReferenceDirection GetRefDirTorso() const	{ return (LookIkReferenceDirection)m_uRefDirTorso; }
	LookIkReferenceDirection GetRefDirNeck() const	{ return (LookIkReferenceDirection)m_uRefDirNeck; }
	LookIkReferenceDirection GetRefDirHead() const	{ return (LookIkReferenceDirection)m_uRefDirHead; }
	LookIkReferenceDirection GetRefDirEye() const	{ return (LookIkReferenceDirection)m_uRefDirEye; }

	LookIkTurnRate GetTurnRate() const				{ return (LookIkTurnRate)m_uTurnRate; }
	LookIkBlendRate GetBlendInRate() const			{ return (LookIkBlendRate)m_uBlendInRate; }
	LookIkBlendRate GetBlendOutRate() const			{ return (LookIkBlendRate)m_uBlendOutRate; }

	LookIkHeadAttitude GetHeadAttitude() const		{ return (LookIkHeadAttitude)m_uHeadAttitude; }

	LookIkRotationLimit GetRotLimTorso(LookIkRotationAngle eType) const	{ return (LookIkRotationLimit)m_auRotLim[0][eType]; }
	LookIkRotationLimit GetRotLimNeck(LookIkRotationAngle eType) const	{ return (LookIkRotationLimit)m_auRotLim[1][eType]; }
	LookIkRotationLimit GetRotLimHead(LookIkRotationAngle eType) const	{ return (LookIkRotationLimit)m_auRotLim[2][eType]; }

	LookIkArmCompensation GetArmCompensation(LookIkArm eArm) const		{ return (LookIkArmCompensation)m_auArmComp[eArm]; }

	bool IsInstant() const							{ return m_bInstant; }

private:
	void Default();

	QuatV m_qHeadNeckAdditive;

	Vec3V m_vTargetOffset;
	const CEntity* m_pTargetEntity;
	eAnimBoneTag m_eTargetBoneTag;

	u32 m_uFlags;
	s32 m_sLookAtFlags;
	u32 m_uHashKey;
	u16 m_uHoldTimeMs;
	s16 m_sRequestPriority;

	float m_fHeadNeckAdditiveRatio;

	u8 m_uRefDirTorso;
	u8 m_uRefDirNeck;
	u8 m_uRefDirHead;
	u8 m_uRefDirEye;

	u8 m_uTurnRate;
	u8 m_uBlendInRate;
	u8 m_uBlendOutRate;

	u8 m_uHeadAttitude;
	u8 m_auRotLim[3][LOOKIK_ROT_ANGLE_NUM];		// [TORSO|NECK|HEAD]
	u8 m_auArmComp[LOOKIK_ARM_NUM];

	u8 m_bInstant	: 1;
};

///////////////////////////////////////////////////////////////////////////////

enum ArmIkBlendRate
{
	ARMIK_BLEND_RATE_SLOWEST = 0,
	ARMIK_BLEND_RATE_SLOW,
	ARMIK_BLEND_RATE_NORMAL,
	ARMIK_BLEND_RATE_FAST,
	ARMIK_BLEND_RATE_FASTEST,
	ARMIK_BLEND_RATE_INSTANT,
	ARMIK_BLEND_RATE_NUM
};

enum eArmIkFlags
{
	AIK_TARGET_WRT_HANDBONE = (1<<0),    // Specify the target relative to the handbone
	AIK_TARGET_WRT_POINTHELPER = (1<<1), // Specify the target relative to the pointhelper
	AIK_TARGET_WRT_IKHELPER = (1<<2),	 // Specify the target relative to the ikhelper
	AIK_APPLY_RECOIL = (1<<3),			 // Apply recoil based on information stored in ikhelper
	AIK_USE_FULL_REACH = (1<<4),		 // Override percentage reach limit in solver
	AIK_USE_ANIM_ALLOW_TAGS = (1<<5),    // if the arm ik is enabled it will blend in during ik allow tags (per arm)
	AIK_USE_ANIM_BLOCK_TAGS = (1<<6),    // if the arm ik is enabled it will blend out during ik block tags (per arm)
	AIK_TARGET_WEAPON_GRIP = (1<<7),	 // Specify the target is a weapon grip target
	AIK_USE_ORIENTATION = (1<<8),		 // Solve for orientation in addition to position
	AIK_TARGET_ROTATION = (1<<9),		 // Target rotation component is valid
	AIK_APPLY_ALT_SRC_RECOIL = (1<<10),	 // Apply alternate source recoil based on information stored in arm roll bones
	AIK_HOLD_HELPER = (1<<11),			 // Hold last active helper transform when solver is blending out
	AIK_TARGET_WRT_CONSTRAINTHELPER = (1<<12),	 // Specify the target relative to the constraint helper
	AIK_USE_TWIST_CORRECTION = (1<<13),	 // Forearm twist correction
	AIK_TARGET_COUNTER_BODY_RECOIL = (1<<14)	// Specify the target is to counter body recoil
};

class CIkRequestArm
{
	friend class CArmIkSolver;

public:
	CIkRequestArm();
	CIkRequestArm(IkArm eArm, const CEntity* pTargetEntity, eAnimBoneTag eTargetBoneTag, Vec3V_In vTargetOffset = Vec3V(V_ZERO_WONE), u32 uFlags = 0);
	CIkRequestArm(IkArm eArm, const CEntity* pTargetEntity, eAnimBoneTag eTargetBoneTag, Vec3V_In vTargetOffset, QuatV_In qTargetRotation, u32 uFlags = 0);

	void SetArm(IkArm eArm)								{ m_uArm = (u8)eArm; }

	void SetTargetEntity(const CEntity* pEntity)		{ m_pTargetEntity = pEntity; }
	void SetTargetBoneTag(eAnimBoneTag eTargetBoneTag)	{ m_eTargetBoneTag = eTargetBoneTag; }
	void SetTargetOffset(Vec3V_In vTargetOffset);
	void SetTargetRotation(QuatV_In qRotation);
	void SetAdditive(QuatV_In qAdditive);

	void SetFlags(u32 uFlags)							{ m_uFlags = uFlags; }

	void SetBlendInRate(ArmIkBlendRate eRate)			{ m_uBlendInRate = (u8)eRate; m_bBlendInRateIsEnum = true; }
	void SetBlendOutRate(ArmIkBlendRate eRate)			{ m_uBlendOutRate = (u8)eRate; m_bBlendInRateIsEnum = true; }

	void SetBlendInRate(float fRate)					{ m_fBlendInRate = fRate; m_bBlendInRateIsEnum = false; }
	void SetBlendOutRate(float fRate)					{ m_fBlendOutRate = fRate; m_bBlendOutRateIsEnum = false; }

	void SetBlendInRange(float fRange)					{ m_fBlendInRange = fRange; }
	void SetBlendOutRange(float fRange)					{ m_fBlendOutRange = fRange; }

	IkArm GetArm() const								{ return (IkArm)m_uArm; }

	const CEntity* GetTargetEntity() const				{ return m_pTargetEntity; }
	eAnimBoneTag GetTargetBoneTag() const				{ return m_eTargetBoneTag; }
	const Vec3V& GetTargetOffset() const				{ return m_vTargetOffset; }
	const QuatV& GetTargetRotation() const				{ return m_qTargetRotation; }
	const QuatV& GetAdditive() const					{ return m_qAdditive; }

	u32 GetFlags() const								{ return m_uFlags; }

	bool IsBlendInRateAnEnum() const					{ return m_bBlendInRateIsEnum; }
	bool IsBlendOutRateAnEnum() const					{ return m_bBlendOutRateIsEnum; }

	ArmIkBlendRate GetBlendInRate() const				{ ikAssert(m_bBlendInRateIsEnum); return (ArmIkBlendRate)m_uBlendInRate; }
	ArmIkBlendRate GetBlendOutRate() const				{ ikAssert(m_bBlendOutRateIsEnum); return (ArmIkBlendRate)m_uBlendOutRate; }

	float GetBlendInRateFloat() const					{ ikAssert(!m_bBlendInRateIsEnum); return m_fBlendInRate; }
	float GetBlendOutRateFloat() const					{ ikAssert(!m_bBlendOutRateIsEnum); return m_fBlendOutRate; }

	float GetBlendInRange() const						{ return m_fBlendInRange; }
	float GetBlendOutRange() const						{ return m_fBlendOutRange; }

private:
	void Default();

	Vec3V m_vTargetOffset;
	QuatV m_qTargetRotation;
	QuatV m_qAdditive;									// Additive rotation in object space
	const CEntity* m_pTargetEntity;
	eAnimBoneTag m_eTargetBoneTag;

	float m_fBlendInRange;
	float m_fBlendOutRange;

	u32 m_uFlags;

	float m_fBlendInRate;
	float m_fBlendOutRate;

	bool m_bBlendInRateIsEnum;	// True if we're storing the blend in rate as an enum in m_uBlendInRate, false if it's as a float in m_fBlendInRate
	bool m_bBlendOutRateIsEnum;	// True if we're storing the blend in rate as an enum in m_uBlendOutRate, false if it's as a float in m_fBlendOutRate

	u8 m_uArm;
	u8 m_uBlendInRate;
	u8 m_uBlendOutRate;
};

///////////////////////////////////////////////////////////////////////////////

class CIkRequestResetArm
{
public:
	CIkRequestResetArm(IkArm eArm);

	void SetArm(IkArm eArm)								{ m_uArm = (u8)eArm; }
	IkArm GetArm() const								{ return (IkArm)m_uArm; }

private:
	u8 m_uArm;
};

///////////////////////////////////////////////////////////////////////////////

class CIkRequestBodyReact
{
	friend class CTorsoReactIkSolver;
	friend class CQuadrupedReactSolver;

public:
	CIkRequestBodyReact();
	CIkRequestBodyReact(Vec3V_In vPosition, Vec3V_In vDirection, const int component);

	void SetPosition(Vec3V_In vPosition)			{ m_vPosition = vPosition; }
	void SetDirection(Vec3V_In vDirection)			{ m_vDirection = vDirection; }
	void SetComponent(const int component)			{ m_Component = component; }
	void SetWeaponGroup(const u32 weaponGroup)		{ m_WeaponGroup = weaponGroup; }
	void SetLocalInflictor(const bool bLocal)		{ m_bLocalInflictor = bLocal; }

	const Vec3V& GetPosition() const				{ return m_vPosition; }
	const Vec3V& GetDirection() const				{ return m_vDirection; }
	int GetComponent() const						{ return m_Component; }
	u32 GetWeaponGroup() const						{ return m_WeaponGroup; }
	bool GetLocalInflictor() const					{ return m_bLocalInflictor; }

private:
	Vec3V m_vPosition;
	Vec3V m_vDirection;
	int m_Component;
	u32 m_WeaponGroup;
	bool m_bLocalInflictor;
};

///////////////////////////////////////////////////////////////////////////////

enum BodyRecoilIkFlags
{
	BODYRECOILIK_APPLY_RIGHTARMIK			= (1 << 0),
	BODYRECOILIK_APPLY_LEFTARMIK			= (1 << 1)
};

class CIkRequestBodyRecoil
{
	friend class CBodyRecoilIkSolver;

public:
	CIkRequestBodyRecoil(u32 uFlags = 0);

	void SetFlags(u32 uFlags)						{ m_uFlags = uFlags; }

	u32 GetFlags() const							{ return m_uFlags; }

private:
	u32 m_uFlags;
};

///////////////////////////////////////////////////////////////////////////////

class CIkRequestLeg
{
	friend class CLegIkSolver;
	friend class CLegIkSolverProxy;
	friend class CQuadLegIkSolver;
	friend class CQuadLegIkSolverProxy;

public:
	CIkRequestLeg();

#if FPS_MODE_SUPPORTED
	void SetPelvisOffset(float fOffset)				{ m_fPelvisOffset = fOffset; }
#endif // FPS_MODE_SUPPORTED
	void SetMode(u8 uMode)							{ m_uMode = uMode; }

#if FPS_MODE_SUPPORTED
	float GetPelvisOffset() const					{ return m_fPelvisOffset; }
#endif // FPS_MODE_SUPPORTED
	u8 GetMode() const								{ return m_uMode; }

private:
#if FPS_MODE_SUPPORTED
	float m_fPelvisOffset;							// Additive pelvis offset
#endif // FPS_MODE_SUPPORTED
	
	u8 m_uMode;
};

///////////////////////////////////////////////////////////////////////////////

enum TorsoVehicleIkFlags
{
	TORSOVEHICLEIK_APPLY_LEAN				 = (1 << 0),
	TORSOVEHICLEIK_APPLY_LEAN_RESTRICTED	 = (1 << 1),
	TORSOVEHICLEIK_APPLY_LIMITED_LEAN		 = (1 << 2),
	TORSOVEHICLEIK_APPLY_WRT_PED_ORIENTATION = (1 << 3)
};

class CIkRequestTorsoVehicle
{
	friend class CTorsoVehicleIkSolver;
	friend class CTorsoVehicleIkSolverProxy;

public:
	CIkRequestTorsoVehicle();

	void SetAnimatedLean(float fAnimatedLean)		{ m_fAnimatedLean = fAnimatedLean; }
	void SetReachLimitPercentage(float fLimit)		{ m_fReachLimitPercentage = fLimit; }
	void SetFlags(u32 uFlags)						{ m_uFlags = uFlags; }
	void SetDeltaScale(float fDeltaScale)			{ m_fDeltaScale = fDeltaScale; }

	float GetAnimatedLean() const					{ return m_fAnimatedLean; }
	float GetReachLimitPercentage() const			{ return m_fReachLimitPercentage; }
	float GetDeltaScale() const						{ return m_fDeltaScale; }
	u32 GetFlags() const							{ return m_uFlags; }

private:
	float m_fAnimatedLean;
	float m_fReachLimitPercentage;
	float m_fDeltaScale;
	u32 m_uFlags;
};

#endif // IKREQUEST_H
