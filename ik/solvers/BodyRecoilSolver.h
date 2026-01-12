// 
// ik/solvers/BodyRecoilSolver.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef BODYRECOILSOLVER_H
#define BODYRECOILSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that applies procedural recoil to body using motion stored in 
// BONETAG_R_IK_HAND dof. Derives from crIKSolverBase base class.
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"

#include "fwtl/pool.h"
#include "vectormath/vec4v.h"

#include "Animation/debug/AnimDebug.h"
#include "ik/IkConfig.h"

class CIkRequestBodyRecoil;
class CPed;

class CBodyRecoilIkSolver : public crIKSolverBase
{
public:
	struct RecoilSituation
	{
		Vec4V vDampedValues;				// [Tran|Rot|Head|Pelvis]
		float fTargetRotation;

		void Reset()
		{
			vDampedValues.ZeroComponents();
			fTargetRotation = 0.0f;
		}
	};

	struct WeaponParams
	{
		Vec4V avBlendRates[2];				// [Tran|Rot|Head|Pelvis][In|Out]
		float afSpineStrength[2];			// [Yaw|Pitch]
		float fClavicleStrength;
		float fRootStrength;
		float fPelvisStrength;
	};

	struct Params
	{
		WeaponParams aWeaponParams[2];		// [Single|Auto]
		Vec4V vSpineScale;
#if __IK_DEV
		bool bRenderTarget;
#endif // __IK_DEV
	};
	static Params ms_Params;

	enum eSpinePart { SPINE0, SPINE1, SPINE2, SPINE3, NUM_SPINE_PARTS };
	enum eArmType { LEFT_ARM, RIGHT_ARM, NUM_ARMS };
	enum eArmPart { CLAVICLE, UPPER, IKHELPER, NUM_ARM_PARTS };
	enum eLegType { LEFT_LEG, RIGHT_LEG, NUM_LEGS };
	enum eLegPart { THIGH, CALF, FOOT, TOE, NUM_LEG_PARTS };

	CBodyRecoilIkSolver(CPed* pPed);
	~CBodyRecoilIkSolver();

	FW_REGISTER_CLASS_POOL(CBodyRecoilIkSolver);

	virtual void Reset();
	virtual bool IsDead() const;
	virtual void Iterate(float dt, crSkeleton& skeleton);

	void Request(const CIkRequestBodyRecoil& request);
	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	const RecoilSituation& GetSituation() const { return m_Situation; }

#if __IK_DEV
	virtual void DebugRender();
	static void AddWidgets(bkBank* pBank);
#endif //__IK_DEV

private:
	void Update(float deltaTime, crSkeleton& skeleton);
	void Apply(crSkeleton& skeleton);

	void ResetArm();

	bool IsValid();
	bool IsBlocked();

#if __IK_DEV
	bool CaptureDebugRender();
#endif // __IK_DEV

	RecoilSituation m_Situation;
	CPed* m_pPed;
	u32 m_uFlags;

	u16 m_auSpineBoneIdx[NUM_SPINE_PARTS];
	u16 m_auArmBoneIdx[NUM_ARMS][NUM_ARM_PARTS];
	u16 m_auLegBoneIdx[NUM_LEGS][NUM_LEG_PARTS];
	u16 m_uRootBoneIdx;
	u16 m_uPelvisBoneIdx;
	u16 m_uSpineRootBoneIdx;
	u16 m_uNeckBoneIdx;

	u32 m_bSolverComplete	: 1;
	u32 m_bBonesValid		: 1;
	u32 m_bActive			: 1;

#if __IK_DEV
	static CDebugRenderer ms_debugHelper;
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
#endif //__IK_DEV
};

#endif // BODYRECOILSOLVER_H
