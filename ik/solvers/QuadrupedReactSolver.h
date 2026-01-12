// 
// ik/solvers/QuadrupedReactSolver.h
// 
// Copyright (C) 2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef QUADRUPEDREACTSOLVER_H
#define QUADRUPEDREACTSOLVER_H

#include "ik/IkConfig.h"

#if IK_QUAD_REACT_SOLVER_ENABLE

#include "crbody/iksolver.h"

#include "fwtl/pool.h"
#include "vectormath/mat34v.h"
#include "vectormath/vec3v.h"

#include "Animation/debug/AnimDebug.h"

#define QUAD_REACT_SOLVER_POOL_MAX	8

class CPed;
class CIkRequestBodyReact;

class CQuadrupedReactSolver : public crIKSolverBase
{
public:
	CQuadrupedReactSolver(CPed* pPed);
	~CQuadrupedReactSolver();

#if IK_QUAD_REACT_SOLVER_USE_POOL
	FW_REGISTER_CLASS_POOL(CQuadrupedReactSolver);
#endif // IK_QUAD_REACT_SOLVER_USE_POOL

	virtual void Reset();
	virtual bool IsDead() const;
	virtual void Iterate(float dt, crSkeleton& skeleton);

	void Request(const CIkRequestBodyReact& request);

	void Update(float deltaTime);
	void Apply(crSkeleton& skeleton);

	void PostIkUpdate(float deltaTime);

	bool IsActive() const;

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
	static void TestReact();
#endif //__IK_DEV
	
	static const int kMaxReacts		= 6;
	static const int kMaxSpineBones	= 4;
	static const int kMaxNeckBones	= 6;

	struct BoneSituation
	{
		Vec3V vAxis;		// Rotation axis in object space based on approximated torque vector
		float fMaxAngle;	// Max angle of rotation about axis
		float fAngle;		// Current angle of rotation about axis

		void Reset()
		{
			vAxis.ZeroComponents();
			fMaxAngle = 0.0f;
			fAngle = 0.0f;
		}
	};

	struct ReactSituation
	{
		BoneSituation aoSpineBones[kMaxSpineBones];
		BoneSituation aoNeckBones[kMaxNeckBones];
		BoneSituation oHeadBone;
		BoneSituation oPelvisBone;

		float fDuration;
		float fSecondaryMotionDuration;

		u32 bActive : 1;
		u32 bLocalInflictor : 1;

		ReactSituation()
		{
			Reset();
		}

		void Reset()
		{
			for (int spineBone = 0; spineBone < kMaxSpineBones; ++spineBone)
				aoSpineBones[spineBone].Reset();

			for (int neckBone = 0; neckBone < kMaxNeckBones; ++neckBone)
				aoNeckBones[neckBone].Reset();

			oHeadBone.Reset();
			oPelvisBone.Reset();

			fDuration = 0.0f;
			fSecondaryMotionDuration = 0.0f;
			bActive = false;
			bLocalInflictor = true;
		}
	};

	struct Params
	{
		enum Type
		{
			kLocal,
			kRemote,
			kRemoteRemote,
			kNumTypes
		};

		Vec3V avSpineLimits[kMaxSpineBones][2];	// [Min|Max]
		Vec3V avNeckLimits[kMaxNeckBones][2];	// [Min|Max]
		Vec3V avPelvisLimits[2];				// [Min|Max]

		float afMaxDeflectionAngle[kNumTypes];
		float fMaxDeflectionAngleLimit;
		float fMaxCameraDistance;

		float afSpineStiffness[kNumTypes][kMaxSpineBones];
		float afNeckStiffness[kNumTypes][kMaxNeckBones];
		float afPelvisStiffness[kNumTypes];

		float afReactMagnitude[kNumTypes];
		float afReactMagnitudeNetModifier[kNumTypes];

		float afReactSpeedModifier[kNumTypes];
		float fMaxCharacterSpeed;

		float fReactDistanceModifier;
		float afReactDistanceLimits[2];

		float fMaxRandomImpulsePositionOffset;

		u16 uSecondaryMotionDelayMS;
		u16 auReactDurationMS[kNumTypes];

		float fWeaponGroupModifierSniper;
	};
	static Params ms_Params;

#if __IK_DEV
	struct DebugParams
	{
		int m_iNoOfTexts;
		bool m_bRender;
	};
	static DebugParams		ms_DebugParams;
	static CDebugRenderer	ms_DebugHelper;
#endif

#if !IK_QUAD_REACT_SOLVER_USE_POOL
	static s8				ms_NoOfFreeSpaces;
#endif // !IK_QUAD_REACT_SOLVER_USE_POOL

protected:
	void Begin();

private:
	bool IsValid();
	bool IsBlocked();
	float SampleResponseCurve(float fInterval);
	float GetReactDistanceModifier();
	float GetWeaponGroupModifier(u32 weaponGroup);
	int GetBoneIndex(int component);
	int GetTypeIndex(const CIkRequestBodyReact& request);
	int GetTypeIndex() const;
	void ClampAngles(Vec3V_InOut eulers, Vec3V_In min, Vec3V_In max);
	void ClampAngles(QuatV_InOut q, Vec3V_In min, Vec3V_In max);

	ReactSituation m_aoReacts[kMaxReacts];
	CPed* m_pPed;

	u16 m_auSpineBoneIdx[kMaxSpineBones];
	u16 m_auNeckBoneIdx[kMaxNeckBones];
	u16 m_uHeadBoneIdx;
	u16 m_uPelvisBoneIdx;
	u16 m_uRootBoneIdx;

	u16 m_uSpineBoneCount	: 3;
	u16 m_uNeckBoneCount	: 3;
	u16 m_bRemoveSolver		: 1;
	u16 m_bBonesValid		: 1;
};

#endif // IK_QUAD_REACT_SOLVER_ENABLE

#endif // QUADRUPEDREACTSOLVER_H
