// 
// ik/solvers/RootSlopeFixupSolver.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ROOTSLOPEFIXUPSOLVER_H
#define ROOTSLOPEFIXUPSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that applies procedural rotation to the root to fix-up animations
// played on a slope. Derives from crIKSolverBase base class. 
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"
#include "fwtl/pool.h"
#include "vector/vector3.h"

#include "physics/WorldProbe/shapetestresults.h"
#include "animation/debug/AnimDebug.h"
#include "ik/IkConfig.h"

class CPed;

class CRootSlopeFixupIkSolver : public crIKSolverBase
{
public:

	enum eProbe
	{
		PROBE_INVALID = -1,
		PROBE_SET_A_START = 0,
		PROBE_CLAVICLE_L = PROBE_SET_A_START,
		PROBE_THIGH_R,
		PROBE_CALF_L,
		PROBE_SET_A_END,
		PROBE_SET_B_START = PROBE_SET_A_END,
		PROBE_CLAVICLE_R = PROBE_SET_B_START,
		PROBE_THIGH_L,
		PROBE_CALF_R,
		PROBE_SET_B_END,
		NUM_PROBE = PROBE_SET_B_END 
	};

	enum eProbeSet
	{
		PROBESET_A,
		PROBESET_B,
		NUM_PROBESET
	};

	CRootSlopeFixupIkSolver(CPed* pPed);

	~CRootSlopeFixupIkSolver();

	FW_REGISTER_CLASS_POOL(CRootSlopeFixupIkSolver);

	virtual void Iterate(float dt, crSkeleton& skeleton);

	virtual bool IsDead() const;

	void PreIkUpdate(float deltaTime);

	void Update(float deltaTime);

	bool CalculatePitchAndRollFromGroundNormal(ScalarV& fPitch, ScalarV& fRoll, const Mat33V& xParentMatrix) const;
	void Apply(crSkeleton& skeleton);
	void SolveRoot(crSkeleton& skeleton);

	virtual void Reset();

	void PostIkUpdate();

	void AddFixup(float fBlendRate);

#if FPS_MODE_SUPPORTED
	Mat34V_Out GetHeadTransformMatrix() const { return m_xHeadTransformMatrix; }
	Mat34V_Out GetSpine3TransformMatrix() const { return m_xSpine3TransformMatrix; }
#endif

	float GetBlend() const;
	float GetBlendRate() const;
	void SetBlendRate(float fBlendRate) { m_fBlendRate = fBlendRate; }

	bool IsActive() const;

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
	static void AddRootSlopeFixupCB();
	static void RemoveRootSlopeFixupCB();
#endif //__IK_DEV

private:

	void GetProbeStartEnd(int& iProbeStart, int& iProbeEnd);
	Vec3V_Out CalcPlaneNormal(Vec3V_In vPoint1, Vec3V_In vPoint2, Vec3V_In vPoint3) const;
	void CalcSlope();
	void ResetProbe();
	void RequestProbe();
	void UpdateProbeRequest();

	bool IsValid();
	bool IsBlocked();

	WorldProbe::CShapeTestResults m_ProbeResults[NUM_PROBE];

#if FPS_MODE_SUPPORTED
	Mat34V m_xHeadTransformMatrix;
	Mat34V m_xSpine3TransformMatrix;
#endif

	Vec3V m_vGroundNormal;
	Vec3V m_vDesiredGroundNormal;
	Vec3V m_vLastSlopeCalculationPosition;

	CPed* m_pPed;

	float m_fBlend;
	float m_fBlendRate;
	
	s32 m_uRootBoneIndex;
#if FPS_MODE_SUPPORTED
	s32 m_uHeadBoneIndex;
	s32 m_uSpine3BoneIndex;
#endif
	s32 m_uBoneIndex[NUM_PROBE];

	u8 m_bRemoveSolver : 1;
	u8 m_bBonesValid : 1;
	u8 m_bForceMainThread : 1;

	static float ms_fMaximumFixupPitchAngle;
	static float ms_fMaximumFixupRollAngle;
	static float ms_fMaximumProbeHeightDifference;
	static float ms_fMaximumDistanceBeforeNextProbe;
	static float ms_fGroundNormalApproachRate;
	static float ms_fMaximumProbeWaitTime;
	static float ms_fCapsuleProbeRadius;
	static float ms_fMaximumFixupPitchDifference;

#if __IK_DEV
	static CDebugRenderer ms_debugHelper;
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
	static float ms_fDebugPitchSlope;
	static float ms_fDebugRollSlope;
	static float ms_fDebugBlend;
	static float ms_fDebugBoundHeightOffset;
#endif //__IK_DEV
};

#endif // ROOTSLOPEFIXUPSOLVER_H
