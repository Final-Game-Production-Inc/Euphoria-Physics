// 
// ik/solvers/HeadSolver.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#include "HeadSolver.h"

#if ENABLE_HEAD_SOLVER

#include "diag/art_channel.h"
#include "fwdebug/picker.h"

#include "animation/anim_channel.h"
#include "animation/MovePed.h"
#include "camera/CamInterface.h"
#include "ik/IkManager.h"
#include "math/angmath.h"
#include "peds/Ped.h"
#include "peds/PedIntelligence.h"
#include "scene/world/GameWorld.h"
#include "vectormath/classfreefuncsv.h"
#include "camera/helpers/Frame.h"
//#include "debug/DebugDraw/DebugVolume.h"
#define HEAD_SOLVER_POOL_MAX 0

//instantiate the class memory pool
FW_INSTANTIATE_CLASS_POOL( CHeadIkSolver, HEAD_SOLVER_POOL_MAX, atHashString("CHeadIkSolver",0xbfde67e7) );

ANIM_OPTIMISATIONS();

float CHeadIkSolver::ms_headRateSlow   = 2.0f;
float CHeadIkSolver::ms_headRateNormal = 5.0f;
float CHeadIkSolver::ms_headRateFast   = 10.0f;

Vector3 CHeadIkSolver::ms_headLimitsNarrowestMin = Vector3( ( DtoR * -15.0f), -PI, ( DtoR * -25.0f));
Vector3 CHeadIkSolver::ms_headLimitsNarrowestMax = Vector3( ( DtoR * 15.0f),   PI, ( DtoR * -11.5f));

Vector3 CHeadIkSolver::ms_headLimitsNarrowMin	 = Vector3( ( DtoR * -30.0f), -PI, ( DtoR * -35.0f));
Vector3 CHeadIkSolver::ms_headLimitsNarrowMax	 = Vector3( ( DtoR * 30.0f),   PI, ( DtoR * 0.5f));

Vector3 CHeadIkSolver::ms_headLimitsNormalMin	 = Vector3( ( DtoR * -45.0f), -PI, ( DtoR * -45.0f));
Vector3 CHeadIkSolver::ms_headLimitsNormalMax	 = Vector3( ( DtoR *  45.0f),  PI, ( DtoR *  11.5f));

Vector3 CHeadIkSolver::ms_headLimitsWideMin		 = Vector3( ( DtoR * -70.0f), -PI, ( DtoR * -55.0f));
Vector3 CHeadIkSolver::ms_headLimitsWideMax		 = Vector3( ( DtoR * 70.0f),   PI, ( DtoR * 21.5f));

Vector3 CHeadIkSolver::ms_headLimitsWidestMin	 = Vector3( ( DtoR * -90.0f), -PI, ( DtoR * -65.0f));
Vector3 CHeadIkSolver::ms_headLimitsWidestMax	 = Vector3( ( DtoR * 90.0f),   PI, ( DtoR * 31.5f));

CHeadIkSolver::EyeParams CHeadIkSolver::ms_EyeParams =
{
	{ { DtoR * -20.0f, DtoR * 20.0f }, { DtoR * -17.0f, DtoR * 20.0f } },	// afLimitsNormal[2][2] [Yaw|Pitch][Min|Max]
	25.0f,																	// fMaxCameraDistance
	false,																	// bForceEyesOnly

#if __IK_DEV
	Vector3(Vector3::ZeroType),												// vTargetOverride
	false																	// bOverrideTarget
#endif // __IK_DEV
};

float CHeadIkSolver::ms_OnlyWhileInFOV			= 0.2f;

float CHeadIkSolver::m_blendInTime				= 0.1f;
float CHeadIkSolver::m_blendOutTime				= 1.0f;

#if __IK_DEV
bool	CHeadIkSolver::ms_bRenderDebug			= false;
int		CHeadIkSolver::ms_iNoOfTexts			= 0;
CDebugRenderer CHeadIkSolver::ms_debugHelper;
bool	CHeadIkSolver::ms_bUseLookDir			= false;
#endif //__IK_DEV

//////////////////////////////////////////////////////////////////////////

CHeadIkSolver::CHeadIkSolver(CPed *pPed)
: crIKSolverBase(IK_SOLVER_TYPE_CUSTOM)
, m_pPed(pPed)
{
	m_HeadBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_HEAD);
	m_NeckBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_NECK);
	m_LookBoneIdx = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_LOOK_DIR);

	artAssertf( m_HeadBoneIdx != (u16)-1 && m_NeckBoneIdx != (u16)-1 && m_LookBoneIdx != (u16)-1, 
		"Modelname = (%s) is missing bones necessary for head ik. Expects the following bones: BONETAG_HEAD, BONETAG_NECK, BONETAG_LOOK_DIR", pPed ? pPed->GetModelName() : "Unknown");

	m_EyeBoneIdx[LEFT_EYE]  = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_EYE);
	m_EyeBoneIdx[RIGHT_EYE] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_EYE);
	m_isAlternateRig = false;

	if ((m_EyeBoneIdx[LEFT_EYE] == (u16)-1) || (m_EyeBoneIdx[RIGHT_EYE] == (u16)-1))
	{
		// Not found, try second set since bone tag names don't seem to be consistent across rigs
		m_EyeBoneIdx[LEFT_EYE]  = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_L_EYE2);
		m_EyeBoneIdx[RIGHT_EYE] = (u16)m_pPed->GetBoneIndexFromBoneTag(BONETAG_R_EYE2);
		m_isAlternateRig = true;
	}

	SetPrority(6);
	Reset();
}

//////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::Reset()
{
	m_time					= -1;
	m_offsetBoneTag			= -1;
	m_offsetPos				= Vector3(0.0f,0.0f,0.0f);
	m_targetPos				= Vector3(0.0f, 0.0f, 0.0f);
	m_nonNullEntity			= false;
	m_updateTarget			= true;
	m_isBlendingOut			= false;
	m_requestPriority		= 3;
	m_blend					= 0.0f;

	m_headSolverStatus		= 0;

	m_removeSolver		    = false;

	if (m_pPed && m_pPed->GetSkeleton() && m_HeadBoneIdx!=(u16)-1 && m_NeckBoneIdx!=(u16)-1)
	{
		m_previousLocalHead = RC_MATRIX34(m_pPed->GetSkeleton()->GetLocalMtx(m_HeadBoneIdx));
	}
}

//////////////////////////////////////////////////////////////////////////

bool CHeadIkSolver::IsDead() const
{
	return m_removeSolver;
}

///////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::Update(float deltaTime)
{
	m_headSolverStatus = 0;

	// Early out if any required bones are missing
	if ( m_HeadBoneIdx == (u16)-1 ||  m_NeckBoneIdx == (u16)-1)
	{
		return;
	}

	if( fwTimer::IsGamePaused() ||  m_pPed->m_nDEflags.bFrozenByInterior ||  m_pPed->GetUsingRagdoll() || m_pPed->GetDisableHeadSolver())
	{
		// If we're running a drunk behaviour, keep the solver target updated so the drunk
		//  nm task can pass the updated head look target to naturalmotion
		if(m_pPed->GetUsingRagdoll() && m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_IsDrunk))
		{
			UpdateTargetPosition();
			return;
		}
		//otherwise if we're in ragdoll or blocking head ik, get rid of the solver
		else if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableHeadSolver())
		{
			Reset();
			m_removeSolver = true;
		}
		return;
	}
	else
	{
		UpdateBlends();

		if (m_removeSolver)
		{
			return;
		}

		crSkeleton* pSkel = m_pPed->GetSkeleton();

		Matrix34 globalHeadBoneMat;
		pSkel->GetGlobalMtx(m_HeadBoneIdx, RC_MAT34V(globalHeadBoneMat));

		/*TUNE_GROUP_FLOAT ( HEAD_SOLVER, fConeAngle, 45.0f, 0.0f, 180.0f, 0.01f);
		TUNE_GROUP_FLOAT ( HEAD_SOLVER, fConeLength, 10.0f, 0.0f, 100.0f, 0.01f);
		TUNE_GROUP_FLOAT ( HEAD_SOLVER, fConeOpacity, 1.0f, 0.0f, 100.0f, 0.01f);
		Matrix34 mat = MAT34V_TO_MATRIX34(m_pPed->GetTransform().GetMatrix());
		CDebugVolume cone;
		cone.SetFlatCone(mat, fConeAngle, fConeLength);
		cone.Draw(globalHeadBoneMat.d, globalHeadBoneMat.b, fConeOpacity);*/

		Matrix34 globalNeckBoneMat;
		pSkel->GetGlobalMtx(m_NeckBoneIdx, RC_MAT34V(globalNeckBoneMat));

		Matrix34 globalNeckRotHeadPosMat(globalNeckBoneMat);
		globalNeckRotHeadPosMat.d = globalHeadBoneMat.d;
	
		// Find the goal relative to the neck bones rotation and the head bones position
		Vector3 localTarget(VEC3_ZERO);
		globalNeckRotHeadPosMat.UnTransform(m_targetPos, localTarget);

#if __IK_DEV
		if (CaptureDebugRender())
		{	
			char debugText[100];

			atString flagsAsString;
			m_flags & LF_USE_TORSO ? flagsAsString += "LF_USE_TORSO | " : "";
			m_flags & LF_SLOW_TURN_RATE? flagsAsString += "LF_SLOW_TURN_RATE | " : "";
			m_flags & LF_FAST_TURN_RATE? flagsAsString += "LF_FAST_TURN_RATE | " : "";
			m_flags & LF_USE_CAMERA_FOCUS? flagsAsString += "LF_USE_CAMERA_FOCUS | " : "";
			m_flags & LF_FROM_SCRIPT? flagsAsString += "LF_FROM_SCRIPT | " : "";
			m_flags & LF_WIDE_YAW_LIMIT? flagsAsString += "LF_WIDE_YAW_LIMIT | " : "";
			m_flags & LF_WIDE_PITCH_LIMIT? flagsAsString += "LF_WIDE_PITCH_LIMIT | " : "";
			m_flags & LF_WIDEST_YAW_LIMIT? flagsAsString += "LF_WIDEST_YAW_LIMIT | " : "";
			m_flags & LF_WIDEST_PITCH_LIMIT? flagsAsString += "LF_WIDEST_PITCH_LIMIT | " : "";
			m_flags & LF_NARROW_YAW_LIMIT? flagsAsString += "LF_NARROW_YAW_LIMIT | " : "";
			m_flags & LF_NARROW_PITCH_LIMIT? flagsAsString += "LF_NARROW_PITCH_LIMIT | " : "";
			m_flags & LF_NARROWEST_YAW_LIMIT? flagsAsString += "LF_NARROWEST_YAW_LIMIT | " : "";
			m_flags & LF_NARROWEST_PITCH_LIMIT? flagsAsString += "LF_NARROWEST_PITCH_LIMIT | " : "";
			m_flags & LF_USE_EYES_ONLY? flagsAsString += "LF_USE_EYES_ONLY | " : "";
			m_flags & LF_WHILE_NOT_IN_FOV? flagsAsString += "LF_WHILE_NOT_IN_FOV | " : "";
			m_flags & LF_USE_LOOK_DIR? flagsAsString += "LF_USE_LOOK_DIR | " : "";

			sprintf(debugText, "m_flags = %s\n", flagsAsString.c_str());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

			sprintf(debugText, "m_targetPos %6.4f, %6.4f, %6.4f\n", m_targetPos.x, m_targetPos.y, m_targetPos.z);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			ms_debugHelper.Line(globalHeadBoneMat.d, m_targetPos, Color_green, Color_green);

			sprintf(debugText, "localTarget %6.4f, %6.4f, %6.4f\n", localTarget.x, localTarget.y, localTarget.z);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

			// Test if the ped is directly in front of him
			Vector3 vToWatchTarget= m_targetPos - globalHeadBoneMat.d;
			vToWatchTarget.NormalizeSafe();
			Vector3 vForward = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetB());
			vForward.Normalize();

			static float fDotTest = 0.2f;		
			if ( vForward.Dot(vToWatchTarget) > fDotTest )
			{
				sprintf(debugText, "vForward.Dot(vToWatchTarget) = %6.4f\n", vForward.Dot(vToWatchTarget));
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			else
			{
				sprintf(debugText, "vForward.Dot(vToWatchTarget) = %6.4f\n", vForward.Dot(vToWatchTarget));
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}

		}
#endif //__IK_DEV		

		// Project points behind us into points infront of us
		Vector3 localToEntityMatrixTarget(VEC3_ZERO);
		Matrix34 entityMatrix = MAT34V_TO_MATRIX34(m_pPed->GetMatrix());
		entityMatrix.UnTransform(m_targetPos, localToEntityMatrixTarget);
		static dev_float fFrontPlane = 0.0f;
		if ((localToEntityMatrixTarget.y < fFrontPlane) || localTarget.IsZero())
		{
			localTarget.y = 0.05f; // restrict depth of target to the front plane
			localTarget.x = 0.0f; // restrict height of target to 0.0f
		}
		localTarget.Normalize();

		Vector3 adjustedTargetPos;
		globalNeckRotHeadPosMat.Transform(localTarget, adjustedTargetPos);

#if __IK_DEV
		if (CaptureDebugRender())
		{	
			char debugText[100];
			sprintf(debugText, "localTarget (post-normalize)  %6.4f, %6.4f, %6.4f\n", localTarget.x, localTarget.y, localTarget.z);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

			sprintf(debugText, "adjustedTargetPos %6.4f, %6.4f, %6.4f\n", adjustedTargetPos.x, adjustedTargetPos.y, adjustedTargetPos.z);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_blue, debugText);
			ms_debugHelper.Line(globalHeadBoneMat.d, adjustedTargetPos, Color_blue, Color_blue);
		}
#endif //__IK_DEV		

		Vector3 front = adjustedTargetPos - globalHeadBoneMat.d;
		front.Normalize();

		bool bUseLookDir = ((m_flags & LF_USE_LOOK_DIR) IK_DEV_ONLY(|| ms_bUseLookDir)) && (m_LookBoneIdx != (u16)-1);

		if (bUseLookDir)
		{
			Quaternion qHeadBoneRotation;

			// Look direction bone represents the reference look direction for the pose
			Matrix34 globalLookBoneMat;
			pSkel->GetGlobalMtx(m_LookBoneIdx, RC_MAT34V(globalLookBoneMat));

			// Generate rotation from reference look direction to target (to preserve any roll in the reference look direction)
			Quaternion qLookBoneToTarget;
			qLookBoneToTarget.FromVectors(globalLookBoneMat.b, front);

			// Rotate reference look direction to target
			globalLookBoneMat.ToQuaternion(qHeadBoneRotation);
			qHeadBoneRotation.MultiplyFromLeft(qLookBoneToTarget);

			// Calculate desired head orientation (look direction bone is a child of the head)
			Quaternion qLookBone;
			RCC_MATRIX34(pSkel->GetLocalMtx(m_LookBoneIdx)).ToQuaternion(qLookBone);
			qLookBone.Inverse();

			qHeadBoneRotation.Multiply(qLookBone);
			globalHeadBoneMat.FromQuaternion(qHeadBoneRotation);
		}
		else
		{
			Matrix34 tempGlobalMatrix;
			camFrame::ComputeWorldMatrixFromFront(front, tempGlobalMatrix);
			tempGlobalMatrix.d = globalHeadBoneMat.d;
			globalHeadBoneMat.a = tempGlobalMatrix.c;
			globalHeadBoneMat.b = tempGlobalMatrix.b;
			globalHeadBoneMat.c = -tempGlobalMatrix.a;
		}

		Matrix34 &localHeadBoneMat = RC_MATRIX34(pSkel->GetLocalMtx(m_HeadBoneIdx));
		Matrix34 targetLocalHeadBoneMat(localHeadBoneMat);

		// Update the local head matrix
		targetLocalHeadBoneMat.Set3x3(globalHeadBoneMat);
		targetLocalHeadBoneMat.Dot3x3Transpose(globalNeckBoneMat);
		if (!targetLocalHeadBoneMat.IsOrthonormal())
		{
			targetLocalHeadBoneMat.Normalize();
		}

		// Calculate the target eulers
		Vector3 targetEulers;
		targetLocalHeadBoneMat.ToEulersZYX(targetEulers);

#if __IK_DEV
		if (CaptureDebugRender())
		{	
			char debugText[100];
			sprintf(debugText, "targetEulers %6.4f, %6.4f, %6.4f\n", targetEulers.x, targetEulers.y, targetEulers.z);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			ms_debugHelper.Line(globalHeadBoneMat.d, globalHeadBoneMat.d + globalHeadBoneMat.b, Color_white, Color_white);
		}
#endif //__IK_DEV		

		Vector3 rotMin(ms_headLimitsNormalMin);
		Vector3 rotMax(ms_headLimitsNormalMax);

		if (m_flags & LF_WIDEST_PITCH_LIMIT)
		{
			rotMin.z = ms_headLimitsWidestMin.z;
			rotMax.z = ms_headLimitsWidestMax.z;
		} 
		else if (m_flags & LF_WIDE_PITCH_LIMIT)
		{
			rotMin.z = ms_headLimitsWideMin.z;
			rotMax.z = ms_headLimitsWideMax.z;
		}
		else if (m_flags & LF_NARROW_PITCH_LIMIT)
		{
			rotMin.z = ms_headLimitsNarrowMin.z;
			rotMax.z = ms_headLimitsNarrowMax.z;
		} 
		else if (m_flags & LF_NARROWEST_PITCH_LIMIT)
		{
			rotMin.z = ms_headLimitsNarrowestMin.z;
			rotMax.z = ms_headLimitsNarrowestMax.z;
		}

		if (m_flags & LF_WIDEST_YAW_LIMIT)
		{
			rotMin.x = ms_headLimitsWidestMin.x;
			rotMax.x = ms_headLimitsWidestMax.x;
		} 
		else if (m_flags & LF_WIDE_YAW_LIMIT)
		{
			rotMin.x = ms_headLimitsWideMin.x;
			rotMax.x = ms_headLimitsWideMax.x;
		}
		else if (m_flags & LF_NARROW_YAW_LIMIT)
		{
			rotMin.x = ms_headLimitsNarrowMin.x;
			rotMax.x = ms_headLimitsNarrowMax.x;
		}
		else if (m_flags & LF_NARROWEST_YAW_LIMIT)
		{
			rotMin.x = ms_headLimitsNarrowestMin.x;
			rotMax.x = ms_headLimitsNarrowestMax.x;
		}


		// limit the yaw
		if(targetEulers.x > rotMax.x)
		{
			targetEulers.x = rotMax.x;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_YAW;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_YAW;
		}
		else if(targetEulers.x < rotMin.x)
		{
			targetEulers.x = rotMin.x;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_YAW;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_YAW;
		}
	
		// limit the pitch
		if(targetEulers.z > rotMax.z)
		{
			targetEulers.z = rotMax.z;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_PITCH;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_PITCH;
		}
		else if(targetEulers.z < rotMin.z)
		{
			targetEulers.z = rotMin.z;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_PITCH;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_PITCH;
		}

		// limit the pitch
		if(targetEulers.y > rotMax.y)
		{
			targetEulers.y = rotMax.y;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_ROLL;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_ROLL;
		}
		else if(targetEulers.y < rotMin.y)
		{
			targetEulers.y = rotMin.y;
			m_headSolverStatus &= ~CIkManager::IK_SOLVER_REACHED_ROLL;
			m_headSolverStatus |= CIkManager::IK_SOLVER_UNREACHABLE_ROLL;
		}

#if __IK_DEV
		if (CaptureDebugRender())
		{	
			char debugText[100];
			sprintf(debugText, "clampedEulers.x %6.4f\n", targetEulers.x);
			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_YAW)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}
			else if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_YAW)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			else
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}
			
			sprintf(debugText, "clampedEulers.y %6.4f\n", targetEulers.y);
			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_ROLL)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			} 
			else if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_ROLL)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			else
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}

			sprintf(debugText, "clampedEulers.z %6.4f\n", targetEulers.z);
			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_PITCH)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}
			else if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_PITCH)
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			else 
			{
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}

			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_YAW)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_UNREACHABLE_YAW");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}
			if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_YAW)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_REACHED_YAW");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_ROLL)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_REACHED_ROLL");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_ROLL)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_UNREACHABLE_ROLL");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}
			if (m_headSolverStatus & CIkManager::IK_SOLVER_REACHED_PITCH)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_REACHED_PITCH");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
			}
			if (m_headSolverStatus & CIkManager::IK_SOLVER_UNREACHABLE_PITCH)
			{
				sprintf(debugText, "%s\n", "IK_SOLVER_UNREACHABLE_PITCH");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
			}
		}
#endif //__IK_DEV

		targetLocalHeadBoneMat.FromEulersZYX(targetEulers);

		Vector3 vToWatchTarget= m_targetPos - globalHeadBoneMat.d;
		vToWatchTarget.NormalizeSafe();
		Vector3 vForward = VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetB());
		vForward.Normalize();

		/*TUNE_GROUP_BOOL(HEAD_SOLVER, bForceSkip, false);*/

		bool notInFOV = !(m_flags & LF_WHILE_NOT_IN_FOV) && vForward.Dot(vToWatchTarget) < ms_OnlyWhileInFOV;
		bool bEyesOnly = (m_flags & LF_USE_EYES_ONLY) || ms_EyeParams.bForceEyesOnly;
		if (m_isBlendingOut || notInFOV || bEyesOnly /*|| bForceSkip*/)
		{
			targetLocalHeadBoneMat = localHeadBoneMat;
		}

		float fInterpolationRate = ms_headRateNormal;
		if (m_flags & LF_SLOW_TURN_RATE)
		{
			fInterpolationRate = ms_headRateSlow;
		}
		if (m_flags & LF_FAST_TURN_RATE)
		{
			fInterpolationRate = ms_headRateFast;
		}

		float fblendToTarget = rage::Clamp(fInterpolationRate*deltaTime, 0.0f, 1.0f);
		m_previousLocalHead.Interpolate(m_previousLocalHead, targetLocalHeadBoneMat, fblendToTarget);

		localHeadBoneMat.Interpolate(localHeadBoneMat, m_previousLocalHead, m_blend);
		pSkel->PartialUpdate(m_NeckBoneIdx);

#if __IK_DEV
		if (CaptureDebugRender())
		{	
			ms_debugHelper.Line(globalHeadBoneMat.d, globalHeadBoneMat.d+globalHeadBoneMat.b, Color_red, Color_red);
		}
#endif //__IK_DEV		

		UpdateEyes();
	}
}

//////////////////////////////////////////////////////////////////////////


void CHeadIkSolver::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skel))
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeHead] || CBaseIkManager::ms_DisableAllSolvers)
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
		sprintf(debugText, "%s\n", "HeadSolver::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	// Update(dt);
}

///////////////////////////////////////////////////////////////////////////////

const CEntity* CHeadIkSolver::GetLookAtEntity()
{
	return GetEntity();
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::BlendIn()
{
	m_isBlendingOut = false;
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::BlendOut()
{
	m_isBlendingOut = true;
	m_time = 0;
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::UpdateBlends(float deltaTime)
{
	// Does the target entity still exist?
	if (m_nonNullEntity && !m_pEntity)
	{
		m_nonNullEntity = false;
		m_offsetBoneTag = -1;
		BlendOut();
	}

	// Blend out if the ped has died
	if ( m_pPed->ShouldBeDead())
	{
		BlendOut();
	}

	// Blend out if the head ik is blocked using the ped reset flags (i.e via script or code)
	if (m_pPed->GetHeadIkBlocked())
	{
		BlendOut();
	}

	// Is the head blocked using the ped reset flags specifically for code driven head ik
	if (m_pPed->GetCodeHeadIkBlocked() && !(m_flags&LF_FROM_SCRIPT))
	{
		BlendOut();
	}

	// Blend out if the ped is playing an animation that is incompatible with the ik?
	int iInvalidFlags = APF_BLOCK_IK|APF_BLOCK_HEAD_IK;
	if (m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(iInvalidFlags))
	{
		BlendOut();
	}

	// Local player specific checks
	if (m_pPed->IsLocalPlayer() && CGameWorld::GetMainPlayerInfo() && !CGameWorld::GetMainPlayerInfo()->AreControlsDisabled())
	{
		// Blend out if on a motorcycle
		if (m_pPed->GetVehiclePedInside() && m_pPed->GetVehiclePedInside()->GetVehicleType() == VEHICLE_TYPE_BIKE)
		{
			BlendOut();
		}
		// Blend out if on in combat
		if (m_pPed->GetPedIntelligence()->GetQueriableInterface()->IsTaskCurrentlyRunning(CTaskTypes::TASK_GUN)
			|| m_pPed->GetPedIntelligence()->GetQueriableInterface()->IsTaskCurrentlyRunning(CTaskTypes::TASK_VEHICLE_GUN))
		{
			BlendOut();
		}
	}

	// Calculate the blend
	float deltaBlend = 0.0f;
	if (m_isBlendingOut)
	{
		deltaBlend = deltaTime/m_blendOutTime;
		m_blend -= deltaBlend;
	}
	else
	{
		deltaBlend = deltaTime/m_blendInTime;
		m_blend += deltaBlend;
	}

	m_blend = rage::Clamp(m_blend, 0.0f, 1.0f);

	// Have we finished blending out the ik?
	if (m_time != -1 && m_blend == 0.0f)
	{
		Reset();
		m_removeSolver = true;
	}
	else
	{
		// Have we finished blending in the ik?
		if (m_time!=-1 && m_blend == 1.0f)
		{
			// Decrement the amount of time remaining to hold the ik pose for
			m_time -= (s32)(deltaTime * 1000.0f);
			if (m_time < 0.0f)
			{
				BlendOut();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::UpdateEyes()
{
	if ((m_EyeBoneIdx[LEFT_EYE] == (u16)-1) || (m_EyeBoneIdx[RIGHT_EYE] == (u16)-1))
	{
		return;
	}

	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	Matrix34 mtxReference;
	pSkeleton->GetGlobalMtx(m_HeadBoneIdx, RC_MAT34V(mtxReference));

	// Avoid update if too far away or camera is not facing character
	Vector3 vPedToCamera(camInterface::GetPos() - VEC3V_TO_VECTOR3(m_pPed->GetTransform().GetPosition()));
	if ((vPedToCamera.Mag2() > square(ms_EyeParams.fMaxCameraDistance)) || (camInterface::GetFront().Dot(-mtxReference.b) < 0.0f))
	{
		return;
	}

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "%s\n", "HeadSolver::UpdateEyes");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif // __IK_DEV

	Matrix34 mtxEye;
	Vector3 vTargetPosLocal;
	Vector3 vTargetPos(m_targetPos);
#if __IK_DEV
	if (ms_EyeParams.bOverrideTarget)
	{
		vTargetPos = ms_EyeParams.vTargetOverride;
	}
#endif // __IK_DEV

	for (int eyeIndex = 0; eyeIndex < NUM_EYES; ++eyeIndex)
	{
		pSkeleton->GetGlobalMtx(m_EyeBoneIdx[eyeIndex], RC_MAT34V(mtxEye));

		mtxReference.d = mtxEye.d;
		mtxReference.UnTransform(vTargetPos, vTargetPosLocal);

		const float kfFocalLimit = 0.50f;
		if (vTargetPosLocal.y < kfFocalLimit)
		{
			vTargetPosLocal.y = kfFocalLimit;
			vTargetPosLocal.x = 0.0f;
		}

		float fY = rage::Atan2f(vTargetPosLocal.z, vTargetPosLocal.y);
		float fP = rage::Atan2f(-vTargetPosLocal.x, vTargetPosLocal.y);

		float fYClamped	= Clamp(fY, ms_EyeParams.afLimitsNormal[0][0], ms_EyeParams.afLimitsNormal[0][1]);
		float fPClamped	= Clamp(fP, ms_EyeParams.afLimitsNormal[1][0], ms_EyeParams.afLimitsNormal[1][1]);

#if __IK_DEV
		if (CaptureDebugRender())
		{
			char debugText[100];
			sprintf(debugText, "%s\n", eyeIndex == 0 ? "LEFT_EYE" : "RIGHT_EYE");
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			sprintf(debugText, "yaw, yawClamped %6.4f, %6.4f\n", fY, fYClamped);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			sprintf(debugText, "pitch, pitchClamped %6.4f, %6.4f\n", fP, fPClamped);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

			ms_debugHelper.Axis(mtxReference, 0.02f);
			ms_debugHelper.Line(mtxEye.d, vTargetPos, Color_white, Color_white);
		}
#endif // __IK_DEV

		Vector3 vYAxis(!m_isAlternateRig ? YAXIS : ZAXIS);
		Vector3 vPAxis(!m_isAlternateRig ? XAXIS : YAXIS);

		Quaternion qRotationY;
		qRotationY.FromRotation(vYAxis, fYClamped);
		Quaternion qRotationP;
		qRotationP.FromRotation(vPAxis, fPClamped);

		Quaternion qEye;
		qEye.FromMatrix34(RCC_MATRIX34(pSkeleton->GetSkeletonData().GetDefaultTransform(m_EyeBoneIdx[eyeIndex])));
		qEye.Multiply(qRotationY);
		qEye.Multiply(qRotationP);

		RC_MATRIX34(pSkeleton->GetLocalMtx(m_EyeBoneIdx[eyeIndex])).FromQuaternion(qEye);
	}

	pSkeleton->PartialUpdate(m_HeadBoneIdx);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		for (int eyeIndex = 0; eyeIndex < NUM_EYES; ++eyeIndex)
		{
			pSkeleton->GetGlobalMtx(m_EyeBoneIdx[eyeIndex], RC_MAT34V(mtxReference));
			ms_debugHelper.Line(mtxReference.d, mtxReference.d + (mtxReference.a * 0.02f), Color_red4, Color_red4);
			ms_debugHelper.Line(mtxReference.d, mtxReference.d + (mtxReference.b * 0.02f), Color_green4, Color_green4);
			ms_debugHelper.Line(mtxReference.d, mtxReference.d + (mtxReference.c * 0.02f), Color_blue4, Color_blue4);
		}
	}
#endif // __IK_DEV
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::UpdateInfo(const char* UNUSED_PARAM(idString), u32 hashKey, const CEntity* pEntity, s32 offsetBoneTag, const Vector3 *pOffsetPos, s32 time, s32 UNUSED_PARAM(blendInTime), s32 UNUSED_PARAM(blendOutTime), s32 requestPriority, u32 flags)
{
	// Assume m_pEntity has changed
	m_pEntity = pEntity;

	// Has a particular entity been requested?
	if (m_pEntity)
	{
		m_nonNullEntity = true;
	}
	else
	{
		m_nonNullEntity = false;
	}

	m_hashKey = hashKey;

	m_offsetBoneTag = offsetBoneTag;

	// Has a particular bone in the entity been requested?
	if (m_offsetBoneTag != -1)
	{
		// Check the entity exists?
		ikAssertf(m_pEntity, "The entity is not valid");
	}

	// Has a world position/offset w.r.t the entity/offset w.r.t a bone been requested
	if (pOffsetPos)
	{
		m_offsetPos = *pOffsetPos;
	}
	else
	{
		m_offsetPos = Vector3(0.0f, 0.0f, 0.0f);
	}

	m_time          = time;
	//m_blendInTime   = blendInTime;
	//m_blendOutTime  = blendOutTime;
	m_requestPriority  = (s8)requestPriority;
	m_flags			= flags;

	BlendIn();
}

///////////////////////////////////////////////////////////////////////////////
void CHeadIkSolver::PostIkUpdate(float deltaTime)
{
	// Called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeHead] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if (CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "%s\n", "CHeadIkSolver::PostIkUpdate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //_DEV

	UpdateTargetPosition();

	Update(deltaTime);
}

//////////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::UpdateTargetPosition()
{
	if (m_flags & LF_USE_CAMERA_FOCUS)
	{
		// Get the camera position
		Vector3 destPos = camInterface::GetPos();

		// Determine the position 100 meters in front of the camera
		m_targetPos = destPos + (camInterface::GetFront() * 100.0f);
	}
	else
	{
		// Is there a valid target entity?
		if (m_nonNullEntity && m_pEntity)
		{
			// Is there a valid offset bone?
			if (m_offsetBoneTag != -1 && m_pEntity->GetSkeleton())
			{
				// Add the offset vector (zero by default) to the position of the entity
				Matrix34 boneMatrix(M34_IDENTITY);
				Vector3 bonePos(VEC3_ZERO);

				s32 offsetBoneIndex = -1;
				m_pEntity->GetSkeletonData().ConvertBoneIdToIndex((u16)m_offsetBoneTag, offsetBoneIndex);

				if (offsetBoneIndex!=-1)
				{
					m_pEntity->GetSkeleton()->GetGlobalMtx(offsetBoneIndex, RC_MAT34V(boneMatrix));
				}
				else
				{
					ikAssertf(0, "Unknown target bonetag : %d", m_offsetBoneTag);
				}

				bonePos = boneMatrix.d;
				m_targetPos = VEC3V_TO_VECTOR3(m_pEntity->GetTransform().Transform3x3(VECTOR3_TO_VEC3V(m_offsetPos)));
				m_targetPos	+= bonePos; 
			}
			else
			{
				// Add the offset vector (zero by default) to the position of the entity
				m_pEntity->TransformIntoWorldSpace(m_targetPos, m_offsetPos);
			}
		}
		else
		{
			// use the offset position as a world position
			m_targetPos = m_offsetPos;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool CHeadIkSolver::GetTargetPosition(Vector3& pos)
{
	pos = m_targetPos;
	return true;
}

//////////////////////////////////////////////////////////////////////////

#if __IK_DEV

bool CHeadIkSolver::CaptureDebugRender()
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

void CHeadIkSolver::DebugRender()
{
	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();	
	}
}

//////////////////////////////////////////////////////////////////////////

void CHeadIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CHeadIkSolver", false);
		pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeHead], NullCB, "");
		pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");
		pBank->AddSlider("FOV range (dot product)",	&ms_OnlyWhileInFOV, -1.0f, 1.0f, 0.01f);
		pBank->PushGroup("Speed (degrees per second)", false);
			pBank->AddSlider("Head rate slow",		&ms_headRateSlow,   0.0f, 100.0f, 1.0f);
			pBank->AddSlider("Head rate normal",	&ms_headRateNormal, 0.0f, 100.0f, 1.0f);
			pBank->AddSlider("Head rate fast",		&ms_headRateFast,   0.0f, 100.0f, 1.0f);
		pBank->PopGroup();
		pBank->PushGroup("Narrowest Limits (radians)", false);
			pBank->AddSlider("Yaw min",		&ms_headLimitsNarrowestMin.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Yaw max",		&ms_headLimitsNarrowestMax.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll min",	&ms_headLimitsNarrowestMin.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll max",	&ms_headLimitsNarrowestMax.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch min",	&ms_headLimitsNarrowestMin.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch max",	&ms_headLimitsNarrowestMax.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Narrow Limits (radians)", false);
			pBank->AddSlider("Yaw min",		&ms_headLimitsNarrowMin.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Yaw max",		&ms_headLimitsNarrowMax.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll min",	&ms_headLimitsNarrowMin.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll max",	&ms_headLimitsNarrowMax.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch min",	&ms_headLimitsNarrowMin.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch max",	&ms_headLimitsNarrowMax.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Normal Limits (radians)", false);
			pBank->AddSlider("Yaw min",		&ms_headLimitsNormalMin.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Yaw max",		&ms_headLimitsNormalMax.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll min",	&ms_headLimitsNormalMin.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll max",	&ms_headLimitsNormalMax.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch min",	&ms_headLimitsNormalMin.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch max",	&ms_headLimitsNormalMax.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Wide Limits (radians)", false);
			pBank->AddSlider("Yaw min",		&ms_headLimitsWideMin.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Yaw max",		&ms_headLimitsWideMax.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll min",	&ms_headLimitsWideMin.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll max",	&ms_headLimitsWideMax.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch min",	&ms_headLimitsWideMin.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch max",	&ms_headLimitsWideMax.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Widest Limits (radians)", false);
			pBank->AddSlider("Yaw min",		&ms_headLimitsWidestMin.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Yaw max",		&ms_headLimitsWidestMax.x,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll min",	&ms_headLimitsWidestMin.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Roll max",	&ms_headLimitsWidestMax.y,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch min",	&ms_headLimitsWidestMin.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->AddSlider("Pitch max",	&ms_headLimitsWidestMax.z,   -10.0f, 10.0f, 0.01f, NullCB, "");
		pBank->PopGroup();
		pBank->PushGroup("Eye Ik", false);
			pBank->AddToggle("Force Eyes Only", &ms_EyeParams.bForceEyesOnly);
			pBank->AddSlider("Max Camera Distance", &ms_EyeParams.fMaxCameraDistance, 0.0f, 100.0f, 0.01f, NullCB, "");
			pBank->AddToggle("Override Target",	&ms_EyeParams.bOverrideTarget);
			pBank->AddVector("Target Position", &ms_EyeParams.vTargetOverride, bkSlider::FLOAT_MIN_VALUE, bkSlider::FLOAT_MAX_VALUE, 0.01f );
			pBank->PushGroup("Limits (radians)", false);
				pBank->AddSlider("Yaw min",		&ms_EyeParams.afLimitsNormal[0][0],   -10.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Yaw max",		&ms_EyeParams.afLimitsNormal[0][1],   -10.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pitch min",	&ms_EyeParams.afLimitsNormal[1][0],   -10.0f, 10.0f, 0.01f, NullCB, "");
				pBank->AddSlider("Pitch max",	&ms_EyeParams.afLimitsNormal[1][1],   -10.0f, 10.0f, 0.01f, NullCB, "");
			pBank->PopGroup();
		pBank->PopGroup();
		pBank->PushGroup("Look Ik", false);
			pBank->AddToggle("Use Look Dir", &ms_bUseLookDir);
		pBank->PopGroup();
	pBank->PopGroup();
}

//////////////////////////////////////////////////////////////////////////

#endif //__IK_DEV

#endif // ENABLE_HEAD_SOLVER
