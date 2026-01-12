// 
// ik/solvers/HeadSolver.h
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef HEADSOLVER_H
#define HEADSOLVER_H

#define ENABLE_HEAD_SOLVER	0
#if ENABLE_HEAD_SOLVER

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that controls head look at Ik. Derives from
// CIkSolver base class. 
//
// The head solver will try and look at the target provided with respect to the 
// yaw and pitch limits
// In addition targets to the far left and far right and behind the ped will 
// have their pitch limits reduced. 
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"

#include "fwtl/pool.h"
#include "vector/vector3.h"
#include "vector/matrix34.h"
#include "Animation/AnimBones.h"
#include "Animation/debug/AnimDebug.h"
#include "scene/RegdRefTypes.h"
#include "vector/matrix34.h" 

class CHeadIkSolver: public crIKSolverBase
{
public:
	CHeadIkSolver( CPed *pPed );

	~CHeadIkSolver() { };

	FW_REGISTER_CLASS_POOL(CHeadIkSolver);

	// PURPOSE: 
	virtual void Iterate(float dt, crSkeleton& skel);

	// PURPOSE: 
	virtual bool IsDead() const;

	// PURPOSE:
	//void PreIkUpdate();

	// PURPOSE:
	void Update(float deltaTime);

	// PURPOSE: 
	virtual void Reset();

	// PURPOSE:
	void PostIkUpdate(float deltaTime);

	const CEntity* GetLookAtEntity();
	u32 GetHeadSolverStatus() { return m_headSolverStatus; }
	void BlendIn();
	void BlendOut();
	bool GetTargetPosition(Vector3& pos);
	void UpdateTargetPosition();
	void UpdateInfo(const char* idString, u32 hashKey, const CEntity* pEntity, s32 offsetBoneTag, const Vector3 *pOffsetPos, s32 time = -1, s32 blendInTime = 1000, s32 blendOutTime = 1000, s32 priority = 3, u32 flags = 0);

	u32 GetHashKey() const				{ return m_hashKey; }
	s8 GetLookAtPriority()				{ return m_requestPriority; }
	u32 GetFlags() const				{ return m_flags; }
	const CEntity* GetEntity() const	{ return m_pEntity.Get(); }
	void SetTargetPosition(const Vector3 &targetPos)	{ m_targetPos = targetPos; }
	void UpdateBlends();
	void UpdateEyes();

	// PURPOSE: 
	float GetHeadBlend()				{ return m_blend; }

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
#endif //__IK_DEV

public:
	enum eEyes { LEFT_EYE, RIGHT_EYE, NUM_EYES };

	static float ms_headRateSlow;
	static float ms_headRateNormal;
	static float ms_headRateFast;
	
	static Vector3 ms_headLimitsNarrowestMin;
	static Vector3 ms_headLimitsNarrowestMax;
	static Vector3 ms_headLimitsNarrowMin;
	static Vector3 ms_headLimitsNarrowMax;
	static Vector3 ms_headLimitsNormalMin;
	static Vector3 ms_headLimitsNormalMax;
	static Vector3 ms_headLimitsWideMin;
	static Vector3 ms_headLimitsWideMax;
	static Vector3 ms_headLimitsWidestMin;
	static Vector3 ms_headLimitsWidestMax;

	static float ms_OnlyWhileInFOV;

	static s32 m_blendInTime;				// time to blend in the ik pose (in milliseconds)
	static s32 m_blendOutTime;				// time to blend out the ik pose (in milliseconds)

	struct EyeParams
	{
		float afLimitsNormal[2][2];				// [Yaw|Pitch][Min|Max]
		float fMaxCameraDistance;
		bool bForceEyesOnly;

	#if __IK_DEV
		Vector3 vTargetOverride;
		bool bOverrideTarget;
	#endif // __IK_DEV
	};
	static EyeParams ms_EyeParams;

private:

	CPed*			m_pPed;
	RegdConstEnt	m_pEntity;			// look at this specific entity (optional)

	s32				m_time;				// time to hold ik pose (in milliseconds)
										// -1 means hold ik pose forever
	s32				m_offsetBoneTag;	// look at this specific bonetag in the above entity (entity must therefore be a pedestrian)
										// -1 means no boneTag
	Vector3			m_offsetPos;		// if neither an entity or a bonetag exist this defines a position in world space to look at (optional)
										// if just an entity exists this defines an offset w.r.t the entity
										// if an entity and a bonetag exist this defines an offset w.r.t the bone
	Vector3			m_targetPos;		// The target position we want to look at
	float			m_blend;			// The current amount of blend between the skeleton pose and the ik pose
										// 0.0 = fully skeleton, 1.0 = fully ik
	u32				m_flags;			//
	u32				m_hashKey;
	s32				m_headSolverStatus;

	Matrix34		m_previousLocalHead;
	u16				m_HeadBoneIdx;
	u16				m_NeckBoneIdx;
	u16				m_LookBoneIdx;
	u16				m_EyeBoneIdx[NUM_EYES];
	s8				m_requestPriority;	// used to decide if subsequent look at tasks can interrupt this task
	u32				m_nonNullEntity :1;	// Is the entity valid?
	u32				m_updateTarget  :1;	// If true update the target position
	u32				m_isBlendingOut :1;	// Are we currently blending out?
	u32				m_removeSolver  :1;
	u32				m_isAlternateRig:1;

#if __IK_DEV
	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
	static CDebugRenderer ms_debugHelper;

	static bool ms_bUseLookDir;
#endif //__IK_DEV
};

#endif // ENABLE_HEAD_SOLVER

#endif // HEADSOLVER_H

