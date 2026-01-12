// 
// ik/solvers/QuadLegSolver.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved.
// 

#ifndef QUADLEGSOLVER_H
#define QUADLEGSOLVER_H

#include "ik/IkConfig.h"

#if IK_QUAD_LEG_SOLVER_ENABLE

#include "crbody/iksolver.h"

#include "fwtl/pool.h"
#include "phcore/constants.h"
#include "system/timer.h"
#include "vectormath/mat34v.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"

#include "ik/IkConfig.h"
#include "Animation/debug/AnimDebug.h"
#include "Task/System/Tuning.h"

#define QUAD_LEG_SOLVER_POOL_MAX	8

namespace WorldProbe
{
	class CShapeTestResults;
}

class CIkRequestLeg;
class CQuadLegIkSolverProxy;
class CPed;

class CQuadLegIkSolver: public crIKSolverQuadruped
{
public:
	struct Tunables : CTuning
	{
		Tunables();

		float m_RootBlendInRate;
		float m_RootBlendOutRate;
		float m_RootInterpolationRate;
		float m_RootGroundNormalInterpolationRate;
		float m_RootMinLimitScale;
		float m_RootMaxLimitScale;

		float m_LegBlendInRate;
		float m_LegBlendOutRate;
		float m_LegMinLengthScale;
		float m_LegMaxLengthScale;
		float m_EffectorInterpolationRate;
		float m_EffectorGroundNormalInterpolationRate;

		float m_ShapeTestRadius;
		float m_ShapeTestLengthScale;

		float m_MaxIntersectionHeightScale;
		float m_ClampNormalLimit;
		float m_AnimHeightBlend;

		PAR_PARSABLE;
	};

	struct RootSituation
	{
		Vec3V		m_avGroundPosition[2];	// [Front|Back]
		Vec3V		m_avGroundNormal[2];	// [Front|Back]

		float		m_afDeltaLimits[2];		// [Min|Max]

		float		m_fDeltaHeight;
		float		m_fBlend;

		u8			m_bIntersection		: 1;
		u8			m_bMoving			: 1;
	};

	struct LegSituation
	{
		TransformV	m_Target;

		Vec3V		m_vProbePosition;
		Vec3V		m_vPrevPosition;
		Vec3V		m_vPrevVelocity;

		Vec3V		m_vGroundNormal;

		float		m_fGroundHeight;
		float		m_fDeltaHeight;
		float		m_fBlend;

		u8			m_uFrontBack		: 1;
		u8			m_bIntersection		: 1;
	};

	struct ShapeTestData
	{
		ShapeTestData(bool bAllocate = true);
		~ShapeTestData();

		void Allocate();
		void Free();
		void Reset();

		WorldProbe::CShapeTestResults* m_apResults[CQuadLegIkSolver::NUM_LEGS];
		bool m_bOwnResults;
	};

#if __IK_DEV
	struct DebugParams
	{
		float		m_fGroundHeightDelta;
		int			m_iNoOfTexts;
		bool		m_bRenderDebug;
		bool		m_bRenderProbes;
		bool		m_bRenderSweptProbes;
		u8			m_uForceSolverMode;
	};
#endif

public:
	CQuadLegIkSolver(CPed* pPed, CQuadLegIkSolverProxy* pProxy);
	~CQuadLegIkSolver();

#if IK_QUAD_LEG_SOLVER_USE_POOL
	FW_REGISTER_CLASS_POOL(CQuadLegIkSolver);
#endif // IK_QUAD_LEG_SOLVER_USE_POOL

	virtual void Iterate(crSkeleton& skeleton);
	virtual void Reset();

	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

	void Request(const CIkRequestLeg& request);
	void PartialReset();

#if __IK_DEV
	virtual void DebugRender();
	static void AddWidgets(bkBank* pBank);
#endif

private:
	bool IsValid() const { return m_bBonesValid && (m_pShapeTestData != NULL); }
	bool IsBlocked() const;

	u32 GetMode() const;

	void CalcDefaults(const crSkeleton& skeleton);

	bool CanUseAsyncShapeTests() const;
	void SubmitAsyncShapeTests();
	void SubmitShapeTests();
	void DeleteShapeTestData();
	bool CapsuleTest(Vec3V_In vStart, Vec3V_In vEnd, float fRadius, u32 uShapeTestIncludeFlags, WorldProbe::CShapeTestResults& shapeTestResults);

	u32 GetShapeTestIncludeFlags() const;
	float GetShapeTestRadius() const;
	float GetShapeTestMinOffset(u32 uFrontBack) const;
	float GetShapeTestMaxOffset(u32 uFrontBack) const;

	float GetMaxIntersectionHeight(u32 uFrontBack) const;

	void PreProcessResults();
	void ProcessResults();

	bool SolveRoot(crSkeleton& skeleton);
	void SetupLeg(crSkeleton& skeleton, crIKSolverQuadruped::Goal& goal, LegSituation& legSituation);

	Vec3V_Out ClampNormal(Vec3V_In vNormal);
	Vec3V_Out SmoothNormal(Vec3V_In vDesired, Vec3V_In vCurrent, const float fRate, const float fTimeStep, const bool bForceDesired);

#if __IK_DEV
	bool CaptureDebugRender();
	void DebugRenderLowerBody(const crSkeleton &skeleton, Color32 sphereColor, Color32 lineColor);
#endif

public:
	static Tunables			ms_Tunables;

#if __IK_DEV
	static CDebugRenderer	ms_DebugHelper;
	static DebugParams		ms_DebugParams;
#endif

#if !IK_QUAD_LEG_SOLVER_USE_POOL
	static s8				ms_NoOfFreeSpaces;
#endif // !IK_QUAD_LEG_SOLVER_USE_POOL

private:
	LegSituation			m_aLegSituation[NUM_LEGS];
	RootSituation			m_RootSituation;

	CQuadLegIkSolverProxy*	m_pProxy;
	ShapeTestData*			m_pShapeTestData;
	CPed*					m_pPed;

	float					m_afDefaultHeightAboveCollision[2];	// [Front|Back]
	float					m_afLegLength[2];

	u16						m_uRootBoneIdx;
	u16						m_uPelvisBoneIdx;
	u16						m_uSpineBoneIdx;

	u8 m_bActive			: 1;
	u8 m_bSolve				: 1;
	u8 m_bBonesValid		: 1;
	u8 m_bInstantSetup		: 1;
};

#endif // IK_QUAD_LEG_SOLVER_ENABLE

#endif // QUADLEGSOLVER_H
