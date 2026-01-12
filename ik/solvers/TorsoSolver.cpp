// 
// ik/solvers/PointGunSolver.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

///////////////////////////////////////////////////////////////////////////////
//  INCLUDES
///////////////////////////////////////////////////////////////////////////////
#include "TorsoSolver.h"

// rage includes

// game includes
#include "animation/anim_channel.h"
#include "animation/AnimBones.h"
#include "debug/DebugScene.h"
#include "fwdebug/picker.h"
#include "ik/IkManager.h"
#include "peds/Ped.h"
#include "peds/PedIntelligence.h"
#include "weapons/Weapon.h"

#define TORSO_SOLVER_POOL_MAX 64	//sets the maximum number of these solvers in the pool

FW_INSTANTIATE_CLASS_POOL_SPILLOVER(CTorsoIkSolver, TORSO_SOLVER_POOL_MAX, 0.26f, atHashString("CTorsoIkSolver",0xc5aacc92));

ANIM_OPTIMISATIONS();

float	CTorsoIkSolver::ms_spine0Scale				= 0.1f;
float	CTorsoIkSolver::ms_spine1Scale				= 0.15f;
float	CTorsoIkSolver::ms_spine2Scale				= 0.2f;
float	CTorsoIkSolver::ms_spine3Scale				= 0.5f;

LimbMovementInfo CTorsoIkSolver::ms_torsoInfo = 
{
	( DtoR * 50.0f), ( DtoR * -50.0f), ( DtoR * 45.0f),
	( DtoR * 60.0f), ( DtoR * -80.0f), ( DtoR * 360.0f),
	0.8f, 0.02f, 12.0f
};

#if __IK_DEV
bool	CTorsoIkSolver::ms_bOverrideTarget	= false;
Vector3 CTorsoIkSolver::ms_vOverrideTarget	= Vector3( 0.0f, 1.0f, 0.f);
float	CTorsoIkSolver::ms_debugSphereScale	= 0.02f;

bool	CTorsoIkSolver::ms_bRenderDebug		= false;
int		CTorsoIkSolver::ms_iNoOfTexts		= 0;
CDebugRenderer CTorsoIkSolver::ms_debugHelper;
#endif //__IK_DEV



//////////////////////////////////////////////////////////////////////////

CTorsoIkSolver::CTorsoIkSolver(CPed* pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pPed(pPed)
{
	SetPrority(2);

	Reset();
}

//////////////////////////////////////////////////////////////////////////

CTorsoIkSolver::~CTorsoIkSolver()
{
	CBaseIkManager::ValidateSolverDeletion(m_pPed, IKManagerSolverTypes::ikSolverTypeTorso);
}

//////////////////////////////////////////////////////////////////////////

bool CTorsoIkSolver::IsDead() const
{
	return (m_pPed->GetDeathState() == DeathState_Dead);
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::Reset()
{
	m_target.Set(0.0f, 0.0f, 0.0f);
	m_torsoBlend			= 0.0f;
	m_torsoYaw				= 0.0f;
	m_torsoPitch			= 0.0f;
	m_torsoMoveResult		= 0;

	m_torsoInfo.maxPitch	= ms_torsoInfo.maxPitch;
	m_torsoInfo.minPitch	= ms_torsoInfo.minPitch;
	m_torsoInfo.maxYaw		= ms_torsoInfo.maxYaw;
	m_torsoInfo.minYaw		= ms_torsoInfo.minYaw;
	m_torsoInfo.pitchDelta	= ms_torsoInfo.pitchDelta;
	m_torsoInfo.yawDelta	= ms_torsoInfo.yawDelta;

	m_torsoInfo.yawDeltaSmoothRate	= ms_torsoInfo.yawDeltaSmoothRate;
	m_torsoInfo.yawDeltaSmoothEase	= ms_torsoInfo.yawDeltaSmoothEase;
	m_torsoInfo.headingDeltaScale	= ms_torsoInfo.headingDeltaScale;

	m_torsoOffsetYaw		= 0.0f;
	m_spineYaw				= 0.0f;
	m_spinePitch			= 0.0f;
	m_spinePitchMin			= ms_torsoInfo.minPitch;
	m_spinePitchMax			= ms_torsoInfo.maxPitch;
	m_torsoBlendIn			= false;
	m_disablePitchFixUp		= false;
	m_autoBlendOut			= true;
#if FPS_MODE_SUPPORTED
	m_fpsMode				= false;
#endif // FPS_MODE_SUPPORTED
	m_prevHeading			= TWO_PI;
}

//////////////////////////////////////////////////////////////////////////

#if FPS_MODE_SUPPORTED
void CTorsoIkSolver::PreIkUpdate(float UNUSED_PARAM(deltaTime))
{
	m_fpsMode = m_pPed->IsFirstPersonShooterModeEnabledForPlayer(false, false, false, true, false);
}
#endif // FPS_MODE_SUPPORTED

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::Update(float deltaTime)
{
	ikAssert(m_pPed);

	if( (fwTimer::IsGamePaused()) ||  m_pPed->m_nDEflags.bFrozenByInterior ||  m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoSolver())
	{
		// If we go into ragdoll reset the solver
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableTorsoSolver())
		{
			Reset();
		}
	}
	else
	{

#if __IK_DEV
		static dev_bool bDebugMinAndMaxLerp = false;
		if(bDebugMinAndMaxLerp)
		{
			static dev_float minPitch = -0.5f;
			static dev_float maxPitch = 0.5f;
			SetTorsoMinPitch(minPitch);
			SetTorsoMaxPitch(maxPitch);
		}
#endif //__IK_DEV

		// Reset the move torso flag
		// (must happen before DoPointGunInDirection)
		m_torsoMoveResult = 0;

		static dev_float deltaScale = 3.5f;
		float deltaBlend = deltaTime * deltaScale;
		if (m_torsoBlendIn)
		{
			m_torsoBlend += deltaBlend;
			if (m_torsoBlend > 1.0f)
			{
				m_torsoBlend = 1.0f;
			}
			ApplyPointTorsoInDirection();
		}
		else if (m_torsoBlend>0.0f)
		{
			if (m_autoBlendOut)
			{
				float deltaBlend = deltaTime * deltaScale;
				m_torsoBlend -= deltaBlend;
				if (m_torsoBlend < 0.0f)
				{
					m_torsoBlend = 0.0f;
				}
			}
			ApplyPointTorsoInDirection();
		}
		//else	//can't do this at the moment, because the aim camera continues to use the solver after the anim blends out. 
		//{		//Once created the solver will persist until the ped is removed
		//	//we're fully blended out, time to remove the solver
		//	SetFinished();
		//}

		// Reset
		m_torsoBlendIn = false;

		if (m_torsoBlend == 0.0f)
		{
			m_prevHeading = TWO_PI;
		}

		// Reset the torso information flag
		// (must happen after DoPointGunInDirection)
		m_torsoInfo.yawDelta	= ms_torsoInfo.yawDelta;
		m_torsoInfo.maxYaw		= ms_torsoInfo.maxYaw;
		m_torsoInfo.minYaw		= ms_torsoInfo.minYaw;
		m_torsoInfo.pitchDelta	= ms_torsoInfo.pitchDelta;
		m_torsoInfo.maxPitch	= ms_torsoInfo.maxPitch;
		m_torsoInfo.minPitch	= ms_torsoInfo.minPitch;

		m_torsoInfo.yawDeltaSmoothRate	= ms_torsoInfo.yawDeltaSmoothRate;
		m_torsoInfo.yawDeltaSmoothEase	= ms_torsoInfo.yawDeltaSmoothEase;
		m_torsoInfo.headingDeltaScale	= ms_torsoInfo.headingDeltaScale;

		// Reset the torso offset yaw
		//m_torsoOffsetYaw = 0.0f;

		// Blend out the torso offset yaw
		static dev_float fTorsoBlendOutSpeed = 4.0f * PI;
		float deltaOffsetYaw = deltaTime*fTorsoBlendOutSpeed;
		if( ABS(m_torsoOffsetYaw) <= deltaOffsetYaw )
		{
			m_torsoOffsetYaw = 0.0f;
		}
		else if( m_torsoOffsetYaw > 0.0f )
		{
			m_torsoOffsetYaw -= deltaOffsetYaw;
		}
		else if( m_torsoOffsetYaw < 0.0f )
		{
			m_torsoOffsetYaw += deltaOffsetYaw;
		}
	}

	// Reset auto blend out flag.
	m_autoBlendOut = true;

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "m_target = <%f, %f, %f>\n", m_target.x, m_target.y, m_target.z);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_torsoBlend = %f\n", m_torsoBlend);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_torsoYaw = %f\n", m_torsoYaw);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_torsoPitch = %f\n", m_torsoPitch);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_torsoOffsetYaw = %f\n", m_torsoOffsetYaw);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_spineYaw = %f\n", m_spineYaw);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_spinePitch = %f\n", m_spinePitch);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_spinePitchMin = %f\n", m_spinePitchMin);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_spinePitchMax = %f\n", m_spinePitchMax);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_torsoBlendIn = %s\n", m_torsoBlendIn?"TRUE":"FALSE");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "m_disablePitchFixUp = %s\n", m_disablePitchFixUp?"TRUE":"FALSE");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		if (m_torsoMoveResult & CIkManager::IK_SOLVER_UNREACHABLE_YAW)
		{
			sprintf(debugText, "%s\n", "IK_SOLVER_UNREACHABLE_YAW");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
		if (m_torsoMoveResult & CIkManager::IK_SOLVER_UNREACHABLE_PITCH)
		{
			sprintf(debugText, "%s ", "IK_SOLVER_UNREACHABLE_PITCH");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
		if (m_torsoMoveResult & CIkManager::IK_SOLVER_REACHED_YAW)
		{
			sprintf(debugText, "%s ", "IK_SOLVER_REACHED_YAW");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
		if (m_torsoMoveResult & CIkManager::IK_SOLVER_REACHED_PITCH)
		{
			sprintf(debugText, "%s\n", "IK_SOLVER_REACHED_PITCH");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
	}
#endif //__IK_DEV
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::Iterate(float dt, crSkeleton& UNUSED_PARAM(skel))
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorso] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		ms_debugHelper.Reset();
		ms_iNoOfTexts = 0;

		char debugText[100];
		sprintf(debugText, "%s\n", "CTorsoIkSolver::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update(dt);
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::SetTorsoMinPitch( float minPitch )
{
	ikAssert(minPitch >= -TWO_PI && minPitch <= TWO_PI);
	m_torsoInfo.minPitch = minPitch;
	if( m_torsoPitch > minPitch )
		m_spinePitchMin = minPitch;
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::SetTorsoMaxPitch( float maxPitch )
{
	ikAssert(maxPitch >= -TWO_PI && maxPitch <= TWO_PI);
	m_torsoInfo.maxPitch = maxPitch;
	if( m_torsoPitch < maxPitch )
		m_spinePitchMax = maxPitch;
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::SetTorsoYawDeltaLimits(float yawDelta, float yawDeltaSmoothRate, float yawDeltaSmoothEase)
{
	m_torsoInfo.yawDelta = yawDelta;
	m_torsoInfo.yawDeltaSmoothRate = yawDeltaSmoothRate;
	m_torsoInfo.yawDeltaSmoothEase = yawDeltaSmoothEase;
}

//////////////////////////////////////////////////////////////////////////

float CTorsoIkSolver::GetYawDelta() const
{
	return fwAngle::LimitRadianAngle(m_torsoYaw - m_pPed->GetTransform().GetHeading()) * m_torsoBlend;
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::ApplyPointTorsoInDirection()
{
#if __IK_DEV
	char debugText[100];
	if (CaptureDebugRender())
	{
		sprintf(debugText, "%s\n", "CTorsoIkSolver::ApplyPointGunInDirection");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	ikAssert(m_pPed);

	crSkeleton* pSkel = m_pPed->GetSkeleton();
	if (!ikVerifyf(pSkel, "No valid skeleton!"))
	{
		return;
	}

	// Adding a bank widget from within the motion tree scheduler is a really bad idea (and hangs the PC build)
	// This should create a work list of some kind instead and be drained by the main thread.


	// Assume that the spine 3 bone points at the target in rest
	//s32 rootBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_ROOT);
	s32 spineRootBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE_ROOT);
	s32 spineBoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE0);
	s32 spine1BoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE1);
	s32 spine2BoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE2);
	s32 spine3BoneIndex = m_pPed->GetBoneIndexFromBoneTag(BONETAG_SPINE3);

	//Matrix34 globalRootBoneMat;
	//m_pPed->GetGlobalMtx(rootBoneIndex, globalRootBoneMat);
	Matrix34 globalSpineRootBoneMat;
	pSkel->GetGlobalMtx(spineRootBoneIndex, RC_MAT34V(globalSpineRootBoneMat));
	Matrix34 globalSpineBoneMat; 
	pSkel->GetGlobalMtx(spineBoneIndex, RC_MAT34V(globalSpineBoneMat));
	Matrix34 globalSpine1BoneMat; 
	pSkel->GetGlobalMtx(spine1BoneIndex, RC_MAT34V(globalSpine1BoneMat));
	Matrix34 globalSpine2BoneMat; 
	pSkel->GetGlobalMtx(spine2BoneIndex, RC_MAT34V(globalSpine2BoneMat));
	Matrix34 globalSpine3BoneMat; 
	pSkel->GetGlobalMtx(spine3BoneIndex, RC_MAT34V(globalSpine3BoneMat));

	Vector3 right(XAXIS);
	Vector3 front(YAXIS);
	Vector3 up(ZAXIS);

	// Depending on the situation we may want to pitch in the direction of the target (e.g when in cover)
	// or pitch in the direction the torso is facing
	bool bPitchUsingTorso = true;

	// If the torso offset yaw > 0 and we are in combat then we are in cover
	// Pitch in the direction of the target
	if ( (fabsf(m_torsoOffsetYaw) > 0)
		&& (m_pPed->GetPedIntelligence()->GetQueriableInterface()->IsTaskCurrentlyRunning(CTaskTypes::TASK_IN_COVER, true)) )
	{
		bPitchUsingTorso = false;
	}

	if (!bPitchUsingTorso)
	{
		// Build a orthonormal matrix using the vector from the aiming offset to the target
		// as the forward vector and world z-axis as the approximate up vector
		Vector3 vecAimFrom(VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()));
		if(m_pPed->IsLocalPlayer())
		{
			const CWeapon* pWeapon = m_pPed->GetWeaponManager() ? m_pPed->GetWeaponManager()->GetEquippedWeapon() : NULL;
			if(pWeapon)
			{
				pWeapon->GetWeaponInfo()->GetTorsoAimOffsetForPed(vecAimFrom, *m_pPed);
			}
		}
		front = m_target - vecAimFrom;
		if (front.IsZero())
		{
			front = YAXIS;
		}
		front.Normalize();
		up = ZAXIS;
		right = CrossProduct(front, up);
		right.Normalize();
	}

#if __IK_DEV
	static dev_bool bIgnoreBlend = false;
	if (bIgnoreBlend)
	{
		static dev_float fTestBlend = 1.0f;
		m_torsoBlend = fTestBlend;
	}
#endif // __IK_DEV

	float yaw = fwAngle::LimitRadianAngle(m_torsoYaw - m_pPed->GetTransform().GetHeading());

	static dev_bool bUseOffset = true;
	if (bUseOffset)
	{
		yaw += m_torsoOffsetYaw * m_torsoBlend;
		yaw = fwAngle::LimitRadianAngle(yaw);
	}

	float pitch = m_torsoPitch;

	yaw *= m_torsoBlend;
	pitch *= m_torsoBlend;

	m_torsoMoveResult = MoveTowardTargetPitchAndYaw(yaw, pitch, m_spineYaw, m_spinePitch, m_torsoInfo);

#if FPS_MODE_SUPPORTED
	if (m_fpsMode)
	{
		// Early out prior to skeleton adjustment so that torso solver is disabled for local character
		// but solver values are kept up to date for external queries (eg. multiplayer)
		return;
	}
#endif // FPS_MODE_SUPPORTED

	// If the torso offset yaw is not > 0 we are not in cover
	// Pitch in the direction of the torso
	if (bPitchUsingTorso)
	{
		// Build a orthonormal matrix using the vector from the bone to the target
		// as the forward vector and world z-axis as the approximate up vector
		front = YAXIS;
		float worldYaw = fwAngle::LimitRadianAngle(m_pPed->GetTransform().GetHeading() + m_spineYaw);
		front.RotateZ(worldYaw);
		up = ZAXIS;
		right = CrossProduct(front, up);
		right.Normalize();
	}

#if IK_DEBUG_VERBOSE
	up = CrossProduct(right, front);
	up.Normalize();
#endif // IK_DEBUG_VERBOSE

	Matrix34 &localSpineBoneMat = RC_MATRIX34(pSkel->GetLocalMtx(spineBoneIndex));
	Matrix34 &localSpine1BoneMat = RC_MATRIX34(pSkel->GetLocalMtx(spine1BoneIndex));
	Matrix34 &localSpine2BoneMat = RC_MATRIX34(pSkel->GetLocalMtx(spine2BoneIndex));
	Matrix34 &localSpine3BoneMat = RC_MATRIX34(pSkel->GetLocalMtx(spine3BoneIndex));

	// Apply the adjustment to global spine 0
	globalSpineBoneMat.RotateZ(m_spineYaw * ms_spine0Scale);
	globalSpineBoneMat.RotateUnitAxis(right, m_spinePitch * ms_spine0Scale);
	if (!globalSpineBoneMat.IsOrthonormal())
	{
		globalSpineBoneMat.Normalize();
	}

	// Update the local spine 0 matrix
	localSpineBoneMat.Set3x3(globalSpineBoneMat);
	localSpineBoneMat.Dot3x3Transpose(globalSpineRootBoneMat);
	if (!localSpineBoneMat.IsOrthonormal())
	{
		localSpineBoneMat.Normalize();
	}

	// Update the global spine 1 matrix
	globalSpine1BoneMat.Set3x3(globalSpineBoneMat);
	globalSpine1BoneMat.Dot3x3FromLeft(localSpine1BoneMat);

	// Apply the adjustment to global spine 1 matrix
	globalSpine1BoneMat.RotateZ(m_spineYaw * ms_spine1Scale);
	globalSpine1BoneMat.RotateUnitAxis(right, m_spinePitch * ms_spine1Scale);
	if (!globalSpine1BoneMat.IsOrthonormal())
	{
		globalSpine1BoneMat.Normalize();
	}

	// Update the local spine 1 matrix
	localSpine1BoneMat.Set3x3(globalSpine1BoneMat);
	localSpine1BoneMat.Dot3x3Transpose(globalSpineBoneMat);
	if (!localSpine1BoneMat.IsOrthonormal())
	{
		localSpine1BoneMat.Normalize();
	}

	// Update the global spine 2 matrix
	globalSpine2BoneMat.Set3x3(globalSpine1BoneMat);
	globalSpine2BoneMat.Dot3x3FromLeft(localSpine2BoneMat);

	// Apply the adjustment to global spine 2 matrix
	globalSpine2BoneMat.RotateZ(m_spineYaw * ms_spine2Scale);
	globalSpine2BoneMat.RotateUnitAxis(right, m_spinePitch * ms_spine2Scale);
	if (!globalSpine2BoneMat.IsOrthonormal())
	{
		globalSpine2BoneMat.Normalize();
	}

	// Update the local spine 2 matrix
	localSpine2BoneMat.Set3x3(globalSpine2BoneMat);
	localSpine2BoneMat.Dot3x3Transpose(globalSpine1BoneMat);
	if (!localSpine2BoneMat.IsOrthonormal())
	{
		localSpine2BoneMat.Normalize();
	}

	// Update the global spine 3 matrix
	globalSpine3BoneMat.Set3x3(globalSpine2BoneMat);
	globalSpine3BoneMat.Dot3x3FromLeft(localSpine3BoneMat);

	// Apply the adjustment to spine 3 matrix
	globalSpine3BoneMat.RotateZ(m_spineYaw * ms_spine3Scale);
	globalSpine3BoneMat.RotateUnitAxis(right, m_spinePitch * ms_spine3Scale);
	if (!globalSpine3BoneMat.IsOrthonormal())
	{
		globalSpine3BoneMat.Normalize();
	}

	// Update the local spine 3 matrix
	localSpine3BoneMat.Set3x3(globalSpine3BoneMat);
	localSpine3BoneMat.Dot3x3Transpose(globalSpine2BoneMat);
	if (!localSpine3BoneMat.IsOrthonormal())
	{
		localSpine3BoneMat.Normalize();
	}

	//save the edited global matrices back into the skeleton data
	pSkel->SetGlobalMtx(spineBoneIndex, RC_MAT34V(globalSpineBoneMat));
	pSkel->SetGlobalMtx(spine1BoneIndex, RC_MAT34V(globalSpine1BoneMat));
	pSkel->SetGlobalMtx(spine2BoneIndex, RC_MAT34V(globalSpine2BoneMat));
	pSkel->SetGlobalMtx(spine3BoneIndex, RC_MAT34V(globalSpine3BoneMat));

	// Update any child global matrices
	pSkel->PartialUpdate(spine3BoneIndex);

#if IK_DEBUG_VERBOSE
	const CWeaponInfo *pWeaponInfo = m_pPed->GetWeaponManager() ? m_pPed->GetWeaponManager()->GetEquippedWeaponInfo() : NULL;
	Vector3 vecAimFrom = m_pPed->GetPosition();
	if(m_pPed->IsLocalPlayer())
	{
		pWeaponInfo->GetTorsoAimOffsetForPed(vecAimFrom, *m_pPed);
	}
	m_debugMat1.Set(right, front, up, vecAimFrom);
#endif // IK_DEBUG_VERBOSE

}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::Smooth(float& fCurrent, float fTarget, float fRate, float fEaseDistance)
{
	float fMaxDelta = fRate * fwTimer::GetTimeStep();

	float fDelta = fTarget - fCurrent;
	float fDeltaAbs = fabsf(fDelta);

	float fEase = (fEaseDistance > 0.0f) ? Sinf(Clamp(fDeltaAbs / fEaseDistance, 0.0f, 1.0f) * PI * 0.5f) : 1.0f;

	if (fDeltaAbs > 0.0f)
	{
		fDelta = (fDelta / fDeltaAbs) * Min(fDeltaAbs, Max(fMaxDelta * fEase, 0.0001f));
	}

	fCurrent += fDelta;
}

//////////////////////////////////////////////////////////////////////////

u32 CTorsoIkSolver::MoveTowardTargetPitchAndYaw(const float targetYaw, const float targetPitch, float &yaw, float &pitch, LimbMovementInfo &limbMovementInfo)
{
	const float fTimeStep = fwTimer::GetTimeStep();

	// Reset the solver status
	u32 solverStatus = 0;
	float yawDelta = limbMovementInfo.yawDelta * fTimeStep;
	float pitchDelta =  limbMovementInfo.pitchDelta  * fTimeStep;
	float pitchLimitDelta = PI * fTimeStep;

	// Modify yaw delta rate based on character's heading rate
	float currHeading = m_pPed->GetTransform().GetHeading();
	if (m_prevHeading == TWO_PI)
	{
		m_prevHeading = currHeading;
		m_yawDeltaSmoothed = yawDelta;
	}
	float headingDelta = limbMovementInfo.headingDeltaScale * fabsf(fwAngle::LimitRadianAngle(currHeading - m_prevHeading));
	Smooth(m_yawDeltaSmoothed, Max(yawDelta, headingDelta), limbMovementInfo.yawDeltaSmoothRate, limbMovementInfo.yawDeltaSmoothEase);
	yawDelta = m_yawDeltaSmoothed;
	m_prevHeading = currHeading;

	// Lerp the pitch minimum limit
	if(rage::Abs(m_spinePitchMin - limbMovementInfo.minPitch) < pitchLimitDelta)
	{
		m_spinePitchMin = limbMovementInfo.minPitch;
	}
	// Is the pitch min less than the target pitch min?
	else if(m_spinePitchMin < limbMovementInfo.minPitch)
	{
		m_spinePitchMin += pitchLimitDelta;
	}
	// Is the pitch min more than the target pitch min?
	else if(pitch > limbMovementInfo.minPitch)
	{
		m_spinePitchMin -= pitchLimitDelta;
	}

	// Lerp the pitch maximum limit
	if(rage::Abs(m_spinePitchMax - limbMovementInfo.maxPitch) < pitchDelta)
	{
		m_spinePitchMax = limbMovementInfo.maxPitch;
	}
	// Is the pitch max less than the target pitch max?
	else if(m_spinePitchMax < limbMovementInfo.maxPitch)
	{
		m_spinePitchMax += pitchDelta;
	}
	// Is the pitch max more than the target pitch max?
	else if(pitch > limbMovementInfo.maxPitch)
	{
		m_spinePitchMax -= pitchDelta;
	}

	float yawChange = rage::Abs(yaw - targetYaw);
	float pitchChange = rage::Abs(pitch - targetPitch);

	// Has the bone has reached the target yaw?
	if(yawChange < yawDelta)
	{
		yaw = targetYaw;
		solverStatus |= CIkManager::IK_SOLVER_REACHED_YAW;
	}
	// Is the bones yaw less than the target yaw?
	else if(yaw < targetYaw)
	{
		yaw += yawDelta;
	}
	// Is the bones yaw more than the target yaw?
	else if(yaw > targetYaw)
	{
		yaw -= yawDelta;
	}

	// limit the yaw
	if(yaw > limbMovementInfo.maxYaw)
	{
		yaw = limbMovementInfo.maxYaw;
		solverStatus &= ~CIkManager::IK_SOLVER_REACHED_YAW;
		solverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_YAW;
	}
	else if(yaw < limbMovementInfo.minYaw)
	{
		yaw = limbMovementInfo.minYaw;
		solverStatus &= ~CIkManager::IK_SOLVER_REACHED_YAW;
		solverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_YAW;
	}
	else if(!(yaw == yaw))
	{
		animErrorf("CTorsoIkSolver::MoveTowardTargetPitchAndYaw ped(%p). Yaw is not finite. Reseting solver.", m_pPed);
		Reset();
	}

	// Has the bone reached the target pitch?
	if(pitchChange < pitchDelta)
	{
		pitch = targetPitch;
		solverStatus |= CIkManager::IK_SOLVER_REACHED_PITCH;
	}
	// Is the bones pitch less than the target pitch?
	else if(pitch < targetPitch)
	{
		pitch += pitchDelta;
	}
	// Is the bones pitch more than the target pitch?
	else if(pitch > targetPitch)
	{
		pitch -= pitchDelta;
	}

	// limit the pitch
	if(pitch > m_spinePitchMax)
	{
		pitch = m_spinePitchMax;
		solverStatus &= ~CIkManager::IK_SOLVER_REACHED_PITCH;
		solverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_PITCH;
	}
	else if(pitch < m_spinePitchMin)
	{
		pitch = m_spinePitchMin;
		solverStatus &= ~CIkManager::IK_SOLVER_REACHED_PITCH;
		solverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_PITCH;
	}
	else if(!(pitch == pitch))
	{
		animErrorf("CTorsoIkSolver::MoveTowardTargetPitchAndYaw ped(%p). Pitch is not finite. Reseting solver.", m_pPed);
		Reset();
	}

#if __IK_DEV
	char debugText[100];
	if (CaptureDebugRender())
	{
		sprintf(debugText, "yawDelta = %6.4f, headingDelta = %6.4f\n", yawDelta, headingDelta);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}

	static dev_bool bDebugMinAndMax = false;
	if(bDebugMinAndMax)
	{
		ikDebugf1("minYaw = %f, maxYaw = %f\n", limbMovementInfo.minYaw, limbMovementInfo.maxYaw);
		ikDebugf1("minPitch = %f, maxPitch = %f\n", limbMovementInfo.minPitch, limbMovementInfo.maxPitch);
		ikDebugf1("lerpedPitchMin = %f, lerpedPitchMax = %f\n", m_spinePitchMin, m_spinePitchMax);
	}
#endif // __IK_DEV

	return solverStatus;
}

///////////////////////////////////////////////////////////////////////////////

bool CTorsoIkSolver::PointGunInDirection(float yaw, float pitch, bool UNUSED_PARAM(bRollPitch), float fBlendAmount)
{
	ikAssert(yaw >= -TWO_PI && yaw <= TWO_PI);
	ikAssert(pitch >= -TWO_PI && pitch <= TWO_PI);
	ikAssert(rage::FPIsFinite(yaw));
	ikAssert(rage::FPIsFinite(pitch));

	m_torsoYaw = yaw;
	m_torsoPitch = pitch;
	if (fBlendAmount != -1.0f)
	{
		m_torsoBlend = fBlendAmount;
		m_autoBlendOut = false;
	}
	else
	{
		m_torsoBlendIn = true;
		m_autoBlendOut = true;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool CTorsoIkSolver::PointGunAtPosition(const Vector3& pos, float fBlendAmount)
{
	ikAssert(rage::FPIsFinite(pos.x) && rage::FPIsFinite(pos.y) && rage::FPIsFinite(pos.z));
	m_target = pos;

#ifdef DEBUG
	AssertEntityPointerValid_NotInWorld(m_pPed);
#endif

	float desiredYaw, desiredPitch;
	const CWeapon* pWeapon = m_pPed->GetWeaponManager() ? m_pPed->GetWeaponManager()->GetEquippedWeapon() : NULL;
	if(!pWeapon)
		return false;

	const CWeaponInfo *pWeaponInfo = pWeapon->GetWeaponInfo();

	Vector3 vecAimFrom = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition());

	if(m_pPed->IsLocalPlayer())
	{
		pWeaponInfo->GetTorsoAimOffsetForPed(vecAimFrom, *m_pPed);
	}

#if __IK_DEV
	if(ms_bOverrideTarget)
	{
		const_cast<Vector3&>(pos) = ms_vOverrideTarget + vecAimFrom;
	}
#endif //__IK_DEV

	Vector3 vecAimDirn = pos - vecAimFrom;
	desiredYaw = rage::Atan2f(-vecAimDirn.x, vecAimDirn.y);

	if (!m_disablePitchFixUp)
		desiredPitch = rage::Atan2f(vecAimDirn.z, vecAimDirn.XYMag());
	else
		desiredPitch = 0.0f;

#if __IK_DEV
	static dev_bool RENDER_OFFSET = false;
	if(RENDER_OFFSET)
	{
		grcDebugDraw::Sphere(vecAimFrom, ms_debugSphereScale, Color_white);
	}

	static dev_bool RENDER_LINE = false;
	if(RENDER_LINE)
	{
#if IK_DEBUG_VERBOSE
		m_pPed->GetIkManager().m_debugLineStart = vecAimFrom;
		m_pPed->GetIkManager().m_debugLineFinish = pos;
#endif //IK_DEBUG_VERBOSE
		grcDebugDraw::Line(vecAimFrom, pos, Color_white);
	}

	static dev_bool PRINT_YAW_AND_PITCH = false;
	if(PRINT_YAW_AND_PITCH)
	{
		ikDebugf1("y = %4.2f \t p = %4.2f ?\n", desiredYaw, desiredPitch);
	}
#endif // __IK_DEV

	return PointGunInDirection(desiredYaw, desiredPitch, false, fBlendAmount);
}

///////////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::GetGunAimAnglesFromPosition(const Vector3 &pos, float &yaw, float &pitch)
{
	float xD, yD, xyMag;

	const CWeaponInfo *pWeaponInfo = m_pPed->GetWeaponManager() ? CWeaponInfoManager::GetInfo<CWeaponInfo>(m_pPed->GetWeaponManager()->GetEquippedWeaponHash()) : NULL;

	Vector3 vecAimFrom = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition());
	if(m_pPed->IsLocalPlayer())
	{
		if(pWeaponInfo)
		{
			pWeaponInfo->GetTorsoAimOffsetForPed(vecAimFrom, *m_pPed);
		}
	}

	yaw = fwAngle::GetRadianAngleBetweenPoints(pos.x, pos.y, vecAimFrom.x, vecAimFrom.y);

	xD = vecAimFrom.x - pos.x;
	yD = vecAimFrom.y - pos.y;
	xyMag = rage::Sqrtf(xD*xD + yD*yD);
	pitch = -fwAngle::GetRadianAngleBetweenPoints(pos.z, xyMag, vecAimFrom.z, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::GetGunAimPositionFromAngles(float yaw, float pitch, Vector3 &pos)
{
	Vector3 aimVector(0.0f, 1.0f, 0.0f);
	aimVector.RotateX(pitch);
	aimVector.RotateZ(yaw);

	const CWeaponInfo *pWeaponInfo = m_pPed->GetWeaponManager() ? CWeaponInfoManager::GetInfo<CWeaponInfo>(m_pPed->GetWeaponManager()->GetEquippedWeaponHash()) : NULL;

	ikAssert(pWeaponInfo);
	if (pWeaponInfo)
	{
		Vector3 vecAimFrom = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition());
		if(m_pPed->IsLocalPlayer())
		{
			pWeaponInfo->GetTorsoAimOffsetForPed(vecAimFrom, *m_pPed);
		}

		// aim the gun 50m in front of the player by default (this is what the gun task uses for it's final target position)
		pos = vecAimFrom + (aimVector * 50.0f);
	}
}

///////////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::GetGunAimDirFromGlobalTarget(const Vector3 &globalTarget, Vector3 &normAimDir)
{
	normAimDir = globalTarget;
	normAimDir -= VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition());
	normAimDir.Normalize();
}


///////////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::GetGunGlobalTargetFromAimDir(Vector3 &globalTarget, const Vector3 &normAimDir)
{
	// aim the gun 50m in front of the player by default (this is what the gun task uses for it's final target position)
	globalTarget = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()) + (normAimDir * 50.0f);
}

//////////////////////////////////////////////////////////////////////////

#if __IK_DEV

bool CTorsoIkSolver::CaptureDebugRender()
{
	if (ms_bRenderDebug)
	{
		CEntity* pSelectedEntity = (CEntity*) g_PickerManager.GetSelectedEntity();
		if (pSelectedEntity && pSelectedEntity->GetIsTypePed())
		{
			CPed* pSelectedPed = static_cast<CPed*>(pSelectedEntity);
			if (pSelectedPed == m_pPed)
			{
				return true;
			}

		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

//////////////////////////////////////////////////////////////////////////

void CTorsoIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CTorsoIkSolver", false, NULL);
	pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeTorso], NullCB, "");
	pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");

	pBank->PushGroup("Relative Distribution (Should add up to 1.0)", false);
	pBank->AddSlider("Spine 0 scale",	&ms_spine0Scale,	0.0f, 1.0f, 0.001f, NullCB, "");
	pBank->AddSlider("Spine 1 scale",	&ms_spine1Scale,	0.0f, 1.0f, 0.001f, NullCB, "");
	pBank->AddSlider("Spine 2 scale",	&ms_spine2Scale,	0.0f, 1.0f, 0.001f, NullCB, "");
	pBank->AddSlider("Spine 3 scale",	&ms_spine3Scale,	0.0f, 1.0f, 0.001f, NullCB, "");
	pBank->AddToggle("Override target",		&ms_bOverrideTarget, NullCB, "");
	pBank->AddSlider("Override target x",	&ms_vOverrideTarget.x,	-100.0f, 100.0f, 0.1f, NullCB, "");
	pBank->AddSlider("Override target y",   &ms_vOverrideTarget.y,	-100.0f, 100.0f, 0.1f, NullCB, "");
	pBank->AddSlider("Override target z",   &ms_vOverrideTarget.z,	-100.0f, 100.0f, 0.1f, NullCB,   "");
	pBank->PopGroup();

	pBank->PushGroup("Speed (radians per second)", false);
	pBank->AddSlider("Yaw delta",	&ms_torsoInfo.yawDelta,		-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Pitch delta",	&ms_torsoInfo.pitchDelta,	-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->PopGroup();

	pBank->PushGroup("Limits (radians)", false);
	pBank->AddSlider("Yaw min",		&ms_torsoInfo.minYaw,	-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Yaw max",		&ms_torsoInfo.maxYaw,	-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Pitch min",	&ms_torsoInfo.minPitch,	-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Pitch max",	&ms_torsoInfo.maxPitch,	-10.0f, 10.0f, 0.01f, NullCB, "");
	pBank->PopGroup();

	pBank->PushGroup("Tunables", false);
	pBank->AddSlider("Yaw delta smooth rate",	&ms_torsoInfo.yawDeltaSmoothRate,	0.0f, 100.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Yaw delta smooth ease",	&ms_torsoInfo.yawDeltaSmoothEase,	0.0f, 100.0f, 0.01f, NullCB, "");
	pBank->AddSlider("Heading delta scale",		&ms_torsoInfo.headingDeltaScale,	0.0f, 100.0f, 0.01f, NullCB, "");
	pBank->PopGroup();

	pBank->PopGroup();
}

//////////////////////////////////////////////////////////////////////////

#endif //__IK_DEV



