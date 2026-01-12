// 
// ik/solvers/TorsoSolver.h
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef POINTGUNSOLVER_H
#define	POINTGUNSOLVER_H

///////////////////////////////////////////////////////////////////////////////
// DESCRIPTION
//
// IK solver that controls upper body ik when aiming a gun. Derives from
// CIkSolver base class. Create and add to an existing CBaseIkManager to
// apply the solver to a ped
//
///////////////////////////////////////////////////////////////////////////////

#include "crbody/iksolverbase.h"
#include "fwtl/pool.h"
#include "vector/vector3.h"
#include "fwmaths/Angle.h"
#include "animation/anim_channel.h"
#include "animation/debug/AnimDebug.h"
#include "ik/IkConfig.h"
#include "ik/IkManager.h"

///////////////////////////////////////////////////////////////////////////////
//  LimbMovementInfo
///////////////////////////////////////////////////////////////////////////////

struct LimbMovementInfo
{
	// Information about a bone
	float maxYaw, minYaw, yawDelta;
	float maxPitch, minPitch, pitchDelta;
	float yawDeltaSmoothRate, yawDeltaSmoothEase, headingDeltaScale;
};

///////////////////////////////////////////////////////////////////////////////
//  CPointGunIkSolver
///////////////////////////////////////////////////////////////////////////////

class CTorsoIkSolver: public crIKSolverBase
{
public:
	CTorsoIkSolver(CPed* pPed);

	~CTorsoIkSolver();
	
	FW_REGISTER_CLASS_POOL(CTorsoIkSolver);
	
	virtual void Iterate(float dt, crSkeleton& skel);

	virtual bool IsDead() const; 

#if FPS_MODE_SUPPORTED
	// PURPOSE:
	void PreIkUpdate(float deltaTime);
#endif // FPS_MODE_SUPPORTED

	// PURPOSE:
	void Update(float deltaTime);

	// PURPOSE: 
	virtual void Reset();

	// PURPOSE:
	//void PostIkUpdate();

	// Turns the torso to face a given target (direction or position)
	// Specifically make spine 3 point toward the target (direction or position)
	// The turn is distributed across spine, spine1, spine2, and spine3
	bool PointGunAtPosition(const Vector3& posn, float fBlendAmount = -1.0f);
	bool PointGunInDirection(float yaw, float pitch, bool bRollPitch = false, float fBlendAmount = -1.0f);

	// Network queries for Torso solver
	void GetGunAimAnglesFromPosition(const Vector3 &pos, float &yaw, float &pitch);
	void GetGunAimPositionFromAngles(float yaw, float pitch, Vector3 &pos);
	void GetGunAimDirFromGlobalTarget(const Vector3 &globalTarget, Vector3 &normAimDir);
	void GetGunGlobalTargetFromAimDir(Vector3 &globalTarget, const Vector3 &normAimDir);

	// PURPOSE: 
	CPed* GetPed() const							 { return m_pPed; }
	u32 GetTorsoSolverStatus()						 { return m_torsoMoveResult; }
	float GetTorsoMinYaw()							 { return m_torsoInfo.minYaw; }		// In radians
	float GetTorsoMaxYaw()							 { return m_torsoInfo.maxYaw; }		// In radians
	float GetTorsoMinPitch()						 { return m_torsoInfo.minPitch; }	// In radians
	float GetTorsoMaxPitch()						 { return m_torsoInfo.maxPitch; }	// In radians
	float GetTorsoOffsetYaw() const					 { return m_torsoOffsetYaw; }		// In radians
	float GetTorsoBlend() const						 { return m_torsoBlend; }
	Vector3& GetTorsoTarget()						 { return m_target; }
	void SetTorsoMinYaw(float minYaw)				 { ikAssert(minYaw >= -TWO_PI && minYaw <= TWO_PI); m_torsoInfo.minYaw = minYaw; }
	void SetTorsoMaxYaw(float maxYaw)				 { ikAssert(maxYaw >= -TWO_PI && maxYaw <= TWO_PI); m_torsoInfo.maxYaw = maxYaw; }
	void SetTorsoMinPitch(float minPitch);
	void SetTorsoMaxPitch(float maxPitch);
	void SetTorsoOffsetYaw(float torsoOffsetYaw)	 { ikAssert(m_torsoOffsetYaw >= -TWO_PI && m_torsoOffsetYaw <= TWO_PI); m_torsoOffsetYaw = torsoOffsetYaw; }
	void SetTorsoYawDeltaLimits(float yawDelta, float yawDeltaSmoothRate, float yawDeltaSmoothEase);
	void SetDisablePitchFixUp(bool disablePitchFixUp) { m_disablePitchFixUp = disablePitchFixUp; }

	//Debug functions
	float GetTorsoYaw() const { return m_torsoYaw; }
	float GetTorsoPitch() const { return m_torsoPitch; }

	float GetYawDelta() const;

#if __IK_DEV
	virtual void DebugRender();
	bool CaptureDebugRender();
	static void AddWidgets(bkBank *pBank);
#endif //__IK_DEV

	static float ms_spine0Scale;
	static float ms_spine1Scale;
	static float ms_spine2Scale;
	static float ms_spine3Scale;
	static LimbMovementInfo ms_torsoInfo;

private:

	// Torso solver
	void DoPointGunInDirection();
	void ApplyPointTorsoInDirection();
	u32 MoveTowardTargetPitchAndYaw(const float targetYaw, const float targetPitch, float &yaw, float &pitch, LimbMovementInfo &limbMovementInfo);

	void Smooth(float& fCurrent, float fTarget, float fRate, float fEaseDistance = 0.0f);

	// PointGunAtPosition & PointGunInDirection & DoPointGunInDirection & ApplyPointGunInDirection
	Vector3 m_target;
	CPed* m_pPed;
	float m_torsoBlend;
	float m_torsoYaw;
	float m_torsoPitch;
	u32 m_torsoMoveResult;
	LimbMovementInfo m_torsoInfo;
	float m_torsoOffsetYaw;
	float m_spineYaw;
	float m_spinePitch;
	float m_spinePitchMin;
	float m_spinePitchMax;
	float m_prevHeading;
	float m_yawDeltaSmoothed;
	bool m_torsoBlendIn : 1;
	bool m_autoBlendOut : 1;
	bool m_disablePitchFixUp : 1;
#if FPS_MODE_SUPPORTED
	bool m_fpsMode : 1;
#endif // FPS_MODE_SUPPORTED

#if __IK_DEV
	static bool ms_bOverrideTarget;
	static Vector3 ms_vOverrideTarget;
	static float ms_debugSphereScale;

	static bool ms_bRenderDebug;
	static int ms_iNoOfTexts;
	static CDebugRenderer ms_debugHelper;
#endif //__IK_DEV
};

#endif // POINTGUNSOLVER_H