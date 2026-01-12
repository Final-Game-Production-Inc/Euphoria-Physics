// 
// ik/solvers/TorsoVehicleSolver.h
// 
// Copyright (C) 2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef TORSOVEHICLESOLVER_H
#define TORSOVEHICLESOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that applies procedural vehicle effects to torso. 
// Derives from crIKSolverBase base class.
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"

#include "fwtl/pool.h"
#include "vectormath/quatv.h"
#include "vectormath/vec3v.h"
#include "vectormath/vec4v.h"

#include "Animation/debug/AnimDebug.h"
#include "ik/IkConfig.h"

class CIkRequestTorsoVehicle;
class CTorsoVehicleIkSolverProxy;
class CPed;

namespace rage
{
	class fwEntity;
};

class CTorsoVehicleIkSolver : public crIKSolverBase
{
public:
	enum eSpinePart { SPINE0, SPINE1, SPINE2, SPINE3, NUM_SPINE_PARTS };
	enum eArmType { LEFT_ARM, RIGHT_ARM, NUM_ARMS };
	enum eArmPart { UPPER, ELBOW, HAND, IKHELPER, NUM_ARM_PARTS };
	enum eLegType { LEFT_LEG, RIGHT_LEG, NUM_LEGS };
	enum eLegPart { THIGH, CALF, FOOT, TOE, NUM_LEG_PARTS };
	enum eSolverType { SOLVER_LEAN, SOLVER_ARM_REACH, NUM_SOLVERS };

	struct Params
	{
		Vec3V avSpineDeltaLimits[NUM_SPINE_PARTS][2];	// [Min|Max]
		Vec3V avSpineJointLimits[NUM_SPINE_PARTS][2];	// [Min|Max]
		float afSpineWeights[NUM_SPINE_PARTS];
		float afBlendRate[NUM_SOLVERS];
		float fDeltaRate;
		float afAnimatedLeanLimits[2];					// [Bk|Fwd]
	};
	static Params ms_Params;

	struct ArmReachSituation
	{
		Vec3V vSourcePosition;
		Vec3V vTargetPosition;
		Vec3V vToTarget;
		ScalarV vReachDistance;

		float fBlend;
		bool bValid;
	};

	CTorsoVehicleIkSolver(CPed* pPed, CTorsoVehicleIkSolverProxy* pProxy);
	~CTorsoVehicleIkSolver();

	FW_REGISTER_CLASS_POOL(CTorsoVehicleIkSolver);

	virtual void Reset();
	virtual void Iterate(float dt, crSkeleton& skeleton);

	void Request(const CIkRequestTorsoVehicle& request);
	void PreIkUpdate(float deltaTime);
	void PostIkUpdate(float deltaTime);

#if __IK_DEV
	virtual void DebugRender();
	static void AddWidgets(bkBank* pBank);
#endif //__IK_DEV

private:
	void Update(float deltaTime, crSkeleton& skeleton);

	void SolveLean(float deltaTime, crSkeleton& skeleton);
	void SolveArmReach(crSkeleton& skeleton);
	void SolveSpine(crSkeleton& skeleton, const ArmReachSituation& reachSituation);

	void CalculateArmLength();

	bool IsValid();

	fwEntity* GetAttachParent() const;
	float GetAnimatedLeanAngle() const;

#if __IK_DEV
	bool CaptureDebugRender();
#endif // __IK_DEV

	Vec3V m_vDeltaEulers;

	CPed* m_pPed;
	CTorsoVehicleIkSolverProxy* m_pProxy;

	float m_fArmLength;
	float m_afBlend[NUM_SOLVERS];

	u16 m_auSpineBoneIdx[NUM_SPINE_PARTS];
	u16 m_auArmBoneIdx[NUM_ARMS][NUM_ARM_PARTS];
	u16 m_uFacingBoneIdx;
	u16 m_uNeckBoneIdx;
	u16 m_uHeadBoneIdx;

	u8 m_bBonesValid		: 1;
	u8 m_bActive			: 1;

#if __IK_DEV
	static CDebugRenderer ms_debugHelper;
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
#endif //__IK_DEV
};

#endif // TORSOVEHICLESOLVER_H
