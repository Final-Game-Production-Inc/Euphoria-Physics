// 
// ik/solvers/ArmIkSolver.h
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ARMIKSOLVER_H
#define ARMIKSOLVER_H

#include "crbody/iksolver.h"
#include "fwtl/pool.h"
#include "vectormath/vec3v.h"
#include "vectormath/mat34v.h"
#include "scene/RegdRefTypes.h"
#include "animation/debug/AnimDebug.h"
#include "ik/IkRequest.h"
#include "Peds/pedDefines.h"

class CPed;
class CDynamicEntity;
class CPhysical;

class CArmIkSolver : public crIKSolverArms
{
public:
	// PURPOSE: Default constructor
	CArmIkSolver(CPed *pPed);

	FW_REGISTER_CLASS_POOL(CArmIkSolver);

	// PURPOSE: Iterate the solver from the config
	// NOTE: The method need to run on PPU
	virtual void Iterate(float dt, crSkeleton& skel);
	
	// PURPOSE: Return true if the solver is finished.
	virtual bool IsDead() const; 

	// PURPOSE:
	void PreIkUpdate(float deltaTime);

	// PURPOSE:
	void Update();

	// PURPOSE: 
	virtual void Reset();

	// PURPOSE:
	void PostIkUpdate(float deltaTime);

	// PURPOSE:
	void Request(const CIkRequestArm& request);

	// PURPOSE: Attach the arm to a fixed target
	// PARAMS: side - specify left or right
	// dest - fixed position in world space to be attached
	void SetAbsoluteTarget(eArms side, Vec3V_In dest, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange = 0.0f, float blendOutRange = 0.0f);
	
	// PURPOSE: Attach the arm to a moving bone
	// PARAMS: arm - specify left or right
	// entity - entity to be attached
	// boneIdx - bone index to be attached
	// offset - fixed offset in the local space of the bone
	// useOrientation - if true, the hand will use the bone orientation
	void SetRelativeTarget(eArms side, const CDynamicEntity* entity, int boneIdx, Vec3V_In offset, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange = 0.0f, float blendOutRange = 0.0f);

	// PURPOSE:
	void SetRelativeTarget(eArms side, const CDynamicEntity* entity, int boneIdx, Vec3V_In offset, QuatV_In rotation, s32 flags, float blendInTime, float blendOutTime, float blendInRange = 0.0f, float blendOutRange = 0.0f);

	// PURPOSE:
	void ResetArm(eArms arm);

	// PURPOSE: 
	bool GetArmActive(eArms arm) const					{ return m_isActive[arm]; }

	// PURPOSE: 
	float GetArmBlend(eArms arm) const					{ return m_Targets[arm].m_Blend; }

	// PURPOSE: 
	const CDynamicEntity* GetTarget(eArms arm) const	{ return m_Targets[arm].m_Entity.Get(); }

	// PURPOSE: 
	Vec3V_Out GetTargetPosition(eArms arm) const;

	// PURPOSE:
	float GetBlendInRate(eArms arm) const				{ return m_Targets[arm].m_BlendInRate; }

	// PURPOSE:
	float GetBlendOutRate(eArms arm) const				{ return m_Targets[arm].m_BlendOutRate; }

	// PURPOSE:
	u32 GetControlFlags(eArms arm) const { return m_Targets[arm].m_controlFlags; }

	// PURPOSE:
	void SetControlFlags(eArms arm, u32 controlFlags) { m_Targets[arm].m_controlFlags = controlFlags; }

	// PURPOSE:
	CPhysical* GetAttachParent(Mat34V& attachParentMtx);

	// Temp until Alex gives me the value he wants
	static float GetDoorBargeMoveBlendThreshold()		{ return ms_fDoorBargeMoveBlendThreshold; }

	// PURPOSE:
	static float GetBlendDuration(ArmIkBlendRate blendRate) { return ms_afBlendDuration[blendRate]; }

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
#endif //__IK_DEV

protected:
	struct Target
	{
		Vec3V m_Offset;
		QuatV m_Additive;
		QuatV m_Rotation;
		TransformV m_Helper;
		RegdConstDyn m_Entity;
		int m_BoneIdx;
		float m_Blend;
		float m_BlendInRate;
		float m_BlendOutRate;
		float m_BlendInRange;
		float m_BlendOutRange;
		s32 m_controlFlags;
	};
	Target m_Targets[NUM_ARMS];
	CPed* m_pPed;
	static float ms_fDoorBargeMoveBlendThreshold;
	static float ms_afBlendDuration[ARMIK_BLEND_RATE_NUM];

private:
#if ENABLE_FRAG_OPTIMIZATION
	void GetGlobalMtx(const CDynamicEntity* entity, const s32 boneIdx, Mat34V_InOut outMtx) const;
#endif
	bool IsValid() const								{ return m_bBonesValid; }
	bool IsBlocked() const;
	bool CanUpdate() const;

	bool CalculateTargetPosition(const rage::crSkeleton& skeleton, const crIKSolverArms::ArmGoal& arm, const CArmIkSolver::Target& target, Mat34V& parentMtx, Mat34V& destMtx, bool bWeaponGrip = false);
	void ApplyRecoil(const eArms arm, const crIKSolverArms::ArmGoal& armGoal, const rage::crSkeleton& skeleton, Mat34V& destMtx);
	void StoreHelperTransform(const eArms arm, const bool bActive, const rage::crSkeleton& skeleton, const crIKSolverArms::ArmGoal& armGoal, CArmIkSolver::Target& target);

	bool m_isActive[NUM_ARMS]; // IsBlendingIn

	u8	m_bBonesValid	: 1;

#if __IK_DEV
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
	static CDebugRenderer ms_debugHelper;
#endif //__IK_DEV
};

#endif // ARMIKSOLVER_H