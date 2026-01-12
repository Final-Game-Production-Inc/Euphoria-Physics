// 
// ik/solvers/BodyLookSolver.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef BODYLOOKSOLVER_H
#define BODYLOOKSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// Look solver that uses the torso, neck, head, and eyes.
// Derives from CIkSolver base class.
//
// This look solver will try and look at the target provided, with respect to the 
// yaw and pitch limits, using the torso, neck, head, and eyes depending upon the
// supplied options.
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"

#include "fwtl/pool.h"
#include "vectormath/mat34v.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"

#include "Animation/debug/AnimDebug.h"
#include "ik/IkRequest.h"
#include "peds/PedIKSettings.h"

class CBodyLookIkSolverProxy;
class CPed;

class CBodyLookIkSolver: public crIKSolverBase
{
public:
	enum SpinePart { SPINE0, SPINE1, SPINE2, SPINE3, NUM_SPINE_PARTS };
	enum Eye { LEFT_EYE, RIGHT_EYE, NUM_EYES };
	enum Arm { LEFT_ARM, RIGHT_ARM, NUM_ARMS };
	enum Component { COMP_TORSO, COMP_NECK, COMP_HEAD, COMP_EYE, COMP_NUM };

	CBodyLookIkSolver(CPed* pPed, CBodyLookIkSolverProxy* pProxy);
	~CBodyLookIkSolver() {};

	FW_REGISTER_CLASS_POOL(CBodyLookIkSolver);

	virtual void Reset();
	virtual void Iterate(float dt, crSkeleton& skeleton);

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	void Request(const CIkRequestBodyLook& request);

	float GetBlend(CBodyLookIkSolver::Component component) const	{ return m_afSolverBlend[component]; }
	u8 GetTurnRate() const											{ return m_eTurnRate; }
	u8 GetHeadRotLim(LookIkRotationAngle rotAngle) const			{ return m_aeRotLim[COMP_HEAD][rotAngle]; }

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
#endif //__IK_DEV

private:
	struct SolverContext
	{
		Mat34V m_mtxReferenceDir;
		Mat34V m_mtxObjToRefDirSpace;
		Mat33V m_mtxReference;

		Vec3V m_vTargetPosition;
		Vec3V m_avTargetPosition[COMP_NUM];
		Vec3V m_vTargetDirRel;

		Vec3V m_vEulersAdditive;
		Vec3V m_vEulersAdditiveComp;

		float m_fDeltaTime;
		float m_fDistSqToCamera;
		bool m_bRefDirRelative;
		u8 m_auActive[COMP_NUM];
		u8 m_auRefDirRel[COMP_NUM];
		u8 m_auRefDirWeight[COMP_NUM];
	};

	struct InterpolationContext
	{
		Vec2V m_vForceModifier;
		Vec2V m_vAngSpeed;
		Vec2V m_vAngSpeedForceModifier;

		float m_fDeltaTime;
		bool m_bInstant;
	};

	bool IsValid() const { return m_bBonesValid; }

	void Update(float deltaTime);
	bool UpdateBlend(SolverContext& context);
	void UpdateSettings();

	bool SolveTorso(rage::crSkeleton& skeleton, SolverContext& context);
	bool SolveNeck(rage::crSkeleton& skeleton, SolverContext& context);
	bool SolveHead(rage::crSkeleton& skeleton, SolverContext& context);
	bool SolveEyes(rage::crSkeleton& skeleton, SolverContext& context);
	bool SolveArmCompensation(rage::crSkeleton& skeleton);

	void InitializeDeltaLimits();
	bool CanSlerp(QuatV_In q1, QuatV_In q2);
	Vec3V_Out ClampTargetPosition(SolverContext& context);

	Vec3V_Out InterpolateYawPitch(Vec3V_In vToTarget, Vec2V_Ref vYawPitchCurrent, Vec2V_Ref vYawPitchVelCurrent, ScalarV_In vCompSpring, ScalarV_In vCompDamping, ScalarV_In vCompInvMass, const InterpolationContext& interpContext);
	void InterpolateTargetPositions(rage::crSkeleton& skeleton, SolverContext& context);
	void CalculateRelativeAngles(rage::crSkeleton& skeleton, SolverContext& context);
	void CalculateRelativeAngleContribution(Component eComp, Vec3V_In vMinLimit, Vec3V_In vMaxLimit, SolverContext& context, Vec3V_InOut vEulers);
	void CalculateRelativeAngleRotation(Component eComp, Vec3V_In vMinLimit, Vec3V_In vMaxLimit, SolverContext& context, QuatV_InOut qReferenceToTarget);

	void SetDefaultYawPitch(Component eComp, Vec3V_ConstRef vEulers, Vec3V_Ref vEulersClamped, Vec3V avCurrentLimits[2], const CIKDeltaLimits& deltaLimits);
	void ScaleRollLimit(u32 uYawClamped, Vec3V_In vEulersXYZ, float& fCurrentRollLimitScale, Vec3V_Ref vMinLimit, Vec3V_Ref vMaxLimit, float fDeltaTime);

	void ResetTargetPositions();
	void ResetOnActivate();

	void CanUseComponents(SolverContext& context, bool abCanUseComponent[COMP_NUM]);

	Vec2V			m_avYawPitch[COMP_NUM];
	Vec2V			m_avYawPitchVel[COMP_NUM];
	Vec2V			m_vYawPitchRel;
	Vec2V			m_vYawPitchRelVel;
	Vec3V			m_avDeltaLimits[COMP_NUM][2];						// [Min|Max]

	float			m_afSolverBlendRate[2];								// [In|Out]
	float			m_afSolverBlend[COMP_NUM];
	float			m_afRefDirBlend[COMP_NUM];
	float			m_afArmCompBlend[NUM_ARMS];
	float			m_afRollLimitScale[2];
	float			m_fPrevHeading;

	CBodyLookIkSolverProxy*	m_pProxy;
	CPed*			m_pPed;

	u16				m_uRootBoneIdx;
	u16				m_uSpineRootBoneIdx;
	u16				m_auSpineBoneIdx[NUM_SPINE_PARTS];
	u16				m_auEyeBoneIdx[NUM_EYES];
	u16				m_uHeadBoneIdx;
	u16				m_uNeckBoneIdx;
	u16				m_uLookBoneIdx;
	u16				m_uFacingBoneIdx;
	u16				m_auUpperArmBoneIdx[NUM_ARMS];
	u16				m_auClavicleBoneIdx[NUM_ARMS];

	u8				m_aeRotLim[COMP_NUM - 1][LOOKIK_ROT_ANGLE_NUM];		// COMP_EYE ignored
	u8				m_aeRotLimPrev[COMP_NUM - 1][LOOKIK_ROT_ANGLE_NUM];	// COMP_EYE ignored
	u8				m_aeRotLimPend[COMP_NUM - 1][LOOKIK_ROT_ANGLE_NUM];	// COMP_EYE ignored

	u8				m_aeRefDir[COMP_NUM];
	u8				m_auRefDirWeight[COMP_NUM];

	u8				m_eTurnRate;
	u8				m_eHeadAttitude;

	u8				m_aeArmComp[NUM_ARMS];
	u8				m_aeArmCompPrev[NUM_ARMS];
	u8				m_aeArmCompPend[NUM_ARMS];

	u8				m_uNeckBoneCount;

	u8				m_bInit					: 1;
	u8				m_bBonesValid			: 1;
	u8				m_bAlternateEyeRig		: 1;
	u8				m_bRefDirFacing			: 1;
	u8				m_uBipedQuadruped		: 1;
	u8				m_bAiming				: 1;

	static Vec3V	ms_avTorsoDeltaLimits[LOOKIK_ROT_LIM_NUM][2];		// [Min|Max]
	static Vec3V	ms_avNeckDeltaLimits[2][LOOKIK_ROT_LIM_NUM][2];		// [Min|Max]
	static Vec3V	ms_avHeadDeltaLimits[2][LOOKIK_ROT_LIM_NUM][2];		// [Min|Max]

	static Vec3V	ms_avTorsoJointLimits[NUM_SPINE_PARTS][2];			// [Min|Max]
	static Vec3V	ms_avNeckJointLimits[2];							// [Min|Max]
	static Vec3V	ms_avHeadJointLimits[2];							// [Min|Max]
	static Vec3V	ms_avEyeJointLimits[2];								// [Min|Max]

	static float	ms_afBlendRate[LOOKIK_BLEND_RATE_NUM];
	static float	ms_afSolverBlendRateScale[COMP_NUM];
	static float	ms_afSpineWeight[NUM_SPINE_PARTS];
	static float	ms_afHeadAttitudeWeight[LOOKIK_HEAD_ATT_NUM];

	static float	ms_afCompSpring[COMP_NUM];
	static float	ms_afCompDamping[COMP_NUM];
	static float	ms_afCompInvMass[COMP_NUM];
	static float	ms_afCompSpringRel;
	static float	ms_afCompDampingRel;
	static float	ms_afCompInvMassRel;

	static float	ms_afTargetSpringScale[LOOKIK_TURN_RATE_NUM];

	static float	ms_fCharSpeedModifier;
	static float	ms_fCharAngSpeedModifier;
	static float	ms_afCharAngSpeedCounterModifier[2];				// [Still|Moving]
	static float	ms_fDeltaLimitBlendRate;
	static float	ms_fRollLimitBlendRate;
	static float	ms_afRefDirBlendRate[COMP_NUM][2];					// [LookDir|FacingDir]
	static float	ms_afAngLimit[2];									// [Default|Underwater]
	static float	ms_fEpsilon;
	static u8		ms_auRefDirWeight[2][16][4];

#if __IK_DEV
	static CDebugRenderer	ms_debugHelper;
	static bool				ms_bRenderDebug;
	static bool				ms_bRenderFacing;
	static bool				ms_bRenderMotion;
	static int				ms_iNoOfTexts;
#endif //__IK_DEV

public:
#if __IK_DEV || __BANK
	static bool				ms_bRenderTargetPosition;
#endif // __IK_DEV || __BANK
};

#endif // BODYLOOKSOLVER_H
