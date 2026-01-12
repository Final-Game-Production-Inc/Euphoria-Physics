// 
// ik/solvers/ArmIkSolver.cpp
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#include "ArmIkSolver.h"

#include "phcore/phmath.h"

#include "diag/art_channel.h"
#include "animation/AnimBones.h"
#include "animation/MovePed.h"
#include "fwdebug/picker.h"
#include "ik/IkManager.h"
#include "ik/IkRequest.h"
#include "ik/solvers/BodyRecoilSolver.h"
#include "Peds/ped.h"

#define ARM_SOLVER_POOL_MAX 64

//instantiate the class memory pool
FW_INSTANTIATE_CLASS_POOL_SPILLOVER( CArmIkSolver, ARM_SOLVER_POOL_MAX, 0.56f, atHashString("CArmIkSolver",0x3b8f741a) );

ANIM_OPTIMISATIONS();

float CArmIkSolver::ms_afBlendDuration[ARMIK_BLEND_RATE_NUM] = 
{
	2.000f,	// ARMIK_BLEND_RATE_SLOWEST
	1.000f,	// ARMIK_BLEND_RATE_SLOW
	0.500f,	// ARMIK_BLEND_RATE_NORMAL
	0.267f,	// ARMIK_BLEND_RATE_FAST
	0.150f,	// ARMIK_BLEND_RATE_FASTEST
	0.000f	// ARMIK_BLEND_RATE_INSTANT
};

#if __IK_DEV
bool	CArmIkSolver::ms_bRenderDebug		= false;
int		CArmIkSolver::ms_iNoOfTexts			= 0;
CDebugRenderer CArmIkSolver::ms_debugHelper;
#endif //__IK_DEV

//////////////////////////////////////////////////////////////////////////

float	CArmIkSolver::ms_fDoorBargeMoveBlendThreshold = 1.75f;

CArmIkSolver::CArmIkSolver(CPed *pPed)
: crIKSolverArms()
, m_pPed(pPed)
{
	SetPrority(7);
	memset(&m_Targets[0], 0, sizeof(Target)*NUM_ARMS);

	m_bBonesValid = true;

	// Roll bones below used exclusively for alternate source recoil motion. Requested that recoil motion is always read from right roll bones for now.
	static const eAnimBoneTag armBoneTags[NUM_ARMS][NUM_ARM_PARTS] = 
	{ 
		{BONETAG_L_HAND, BONETAG_L_FOREARM, BONETAG_L_UPPERARM, BONETAG_L_PH_HAND, BONETAG_L_IK_HAND, BONETAG_CH_L_HAND, BONETAG_R_FOREARMROLL, BONETAG_R_ARMROLL},
		{BONETAG_R_HAND, BONETAG_R_FOREARM, BONETAG_R_UPPERARM, BONETAG_R_PH_HAND, BONETAG_R_IK_HAND, BONETAG_CH_R_HAND, BONETAG_R_FOREARMROLL, BONETAG_R_ARMROLL}
	};

	for (int i=0; i < NUM_ARMS; ++i)
	{
		m_isActive[i] = false;

		ArmGoal& arm = m_Arms[i];
		arm.m_HandBoneIdx        = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][HAND]);
		arm.m_ElbowBoneIdx       = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][FOREARM]);
		arm.m_ShoulderBoneIdx    = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][UPPERARM]);
		arm.m_PointHelperBoneIdx = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][PH_HAND]);
		arm.m_HandIkHelperBoneIdx = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][IK_HAND]);
		arm.m_ConstraintHelperBoneIdx = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][CH_HAND]);
		arm.m_ForeArmRollBoneIdx = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][FOREARM_ROLL]);
		arm.m_UpperArmRollBoneIdx = (u16) pPed->GetBoneIndexFromBoneTag(armBoneTags[i][UPPERARM_ROLL]);

		m_bBonesValid &= (arm.m_HandBoneIdx != (u16)-1) && 
						 (arm.m_ElbowBoneIdx != (u16)-1) && 
						 (arm.m_ShoulderBoneIdx != (u16)-1) && 
						 (arm.m_PointHelperBoneIdx != (u16)-1) && 
						 (arm.m_HandIkHelperBoneIdx != (u16)-1);
		artAssertf(m_bBonesValid, "Modelname = (%s) is missing bones necessary for arm ik. Expects the following bones: "
			  "BONETAG_L_HAND, BONETAG_R_HAND, BONETAG_L_FOREARM, BONETAG_R_FOREARM, BONETAG_L_UPPERARM, "
			  "BONETAG_R_UPPERARM, BONETAG_L_PH_HAND, BONETAG_R_PH_HAND, BONETAG_L_IK_HAND, BONETAG_R_IK_HAND", pPed ? pPed->GetModelName() : "Unknown");

		if (arm.m_ShoulderBoneIdx != (u16)-1)
		{
			arm.m_TerminatingIdx = (u16)pPed->GetSkeleton()->GetTerminatingPartialBone(arm.m_ShoulderBoneIdx);
			m_bBonesValid &= (arm.m_TerminatingIdx != (u16)-1);
		}
		artAssertf(arm.m_TerminatingIdx != (u16)-1, "Modelname = (%s) is missing shoulder terminating bones necessary for arm ik.", pPed ? pPed->GetModelName() : "Unknown");

		arm.m_Blend = 0.0f;

		Target& target = m_Targets[i];
		target.m_Entity = NULL;
		target.m_BoneIdx = -1;
		target.m_Offset = Vec3V(V_ZERO);
		target.m_Additive = QuatV(V_IDENTITY);
		target.m_Rotation = QuatV(V_IDENTITY);
		target.m_Helper = TransformV(V_IDENTITY);
		target.m_Blend = 0.0f;
		target.m_BlendInRate = 1.0f/PEDIK_ARMS_FADEIN_TIME;
		target.m_BlendOutRate = 1.0f/PEDIK_ARMS_FADEOUT_TIME;
		target.m_controlFlags = AIK_TARGET_WRT_IKHELPER;
	}
}

//////////////////////////////////////////////////////////////////////////

bool CArmIkSolver::IsDead() const
{
	return m_Targets[LEFT_ARM].m_Blend==0.f && !m_isActive[LEFT_ARM] 
		&& m_Targets[RIGHT_ARM].m_Blend==0.f && !m_isActive[RIGHT_ARM];
}

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Find the circle with position p1 and radius h in the plane where the two spheres intersect
// NOTES: http://astronomy.swin.edu.au/~pbourke/geometry/2circle/
void PPUCalculateP2AndH(Vector3& p0, Vector3& p1, float r0, float r1, Vector3& p2, float &h)
{
	// Determine d the distance between the centre of the circles
	Vector3 p0ToP1(p1 - p0);
	float d = p0ToP1.Mag();

	// Determine a the distance from p0 to p2
	float a = ((r0 * r0) - (r1 * r1) + (d * d)) / (2.0f * d);

	// Determine p2 the coordinates of p2
	// So p2 = p0 + a (p1 - p0) / d
	p2 = p0 + a * (p1 - p0) / d;

	// Determine h the radius of the circle
	float hSquared = (r0 * r0) - (a * a);
	if(hSquared<=0.0f)
	{
		a = r0;
		h = 0.0f;
	}
	else
	{
		//radius of intersection of 2 spheres
		h = sqrtf(hSquared); 
		ikAssert(rage::FPIsFinite(h));
	}
}

////////////////////////////////////////////////////////////////////////////////

void CArmIkSolver::PreIkUpdate(float deltaTime)
{
	IK_DEV_ONLY(char debugText[128];)

	if (!IsValid())
	{
		return;
	}

	if (!CanUpdate())
	{
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableArmSolver() || (m_pPed->IsDead() && !m_pPed->GetIsVisibleInSomeViewportThisFrame()))
		{
			Reset();
		}
		return;
	}

#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeArm] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if(CaptureDebugRender())
	{
		ms_debugHelper.Reset();
		ms_iNoOfTexts = 0;

		sprintf(debugText, "%s", "ArmIK::PreIkUpdate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	crSkeleton& skeleton = *m_pPed->GetSkeleton();
	bool bBlocked = IsBlocked();

	for (int i = 0; i < NUM_ARMS; ++i)
	{
		ArmGoal& arm = m_Arms[i];
		Target& target = m_Targets[i];
		bool& bActive = m_isActive[i];
		arm.m_Enabled = false;

		// Tag support
		if (target.m_controlFlags & AIK_USE_ANIM_BLOCK_TAGS)
		{
			fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
			if (pAnimDirector)
			{
				const CClipEventTags::CArmsIKEventTag* pProp = static_cast<const CClipEventTags::CArmsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::ArmsIk));
				if (pProp && !pProp->GetAllowed() && ((pProp->GetLeft() && (i == LEFT_ARM)) || (pProp->GetRight() && (i == RIGHT_ARM))))
				{
					bActive = false;
				}
			}
		}
		else if (target.m_controlFlags & AIK_USE_ANIM_ALLOW_TAGS) 
		{
			bActive = false;
			fwAnimDirector* pAnimDirector = m_pPed->GetAnimDirector();
			if (pAnimDirector)
			{
				const CClipEventTags::CArmsIKEventTag* pProp = static_cast<const CClipEventTags::CArmsIKEventTag*>(pAnimDirector->FindPropertyInMoveNetworks(CClipEventTags::ArmsIk));
				if (pProp && pProp->GetAllowed() && ((pProp->GetLeft() && i == LEFT_ARM) || (pProp->GetRight() && i == RIGHT_ARM)))
				{
					bActive = true;
				}
			}
		}

		// Blocked by anims
		if (bBlocked)
			bActive = false;

		// Range support
		if (bActive && (target.m_BlendInRange > 0.0f) && (target.m_BlendOutRange > 0.0f))
		{
			Mat34V globalShoulderMtx;
			Mat34V globalHandMtx;
			Mat34V destMtx;
			Mat34V destGlobalMtx;

			// Check for vehicle attachment since solver is run prior to physical attachment (delayed root attachment).
			// Calculate goal relative to attachment position if necessary since character's root may be offset.
			Mat34V attachParentMtx;
			CPhysical* pAttachParent = GetAttachParent(attachParentMtx);

			Mat34V parentMtx((target.m_Entity && (target.m_Entity == pAttachParent)) ? attachParentMtx : *skeleton.GetParentMtx());

			// Pre-calculate target position so that range can be compared
			skeleton.GetGlobalMtx(arm.m_ShoulderBoneIdx, globalShoulderMtx);
			skeleton.GetGlobalMtx(arm.m_HandBoneIdx, globalHandMtx);

			const bool bWeaponGrip = (i == LEFT_ARM) && (m_Arms[RIGHT_ARM].m_Enabled) && (m_Targets[LEFT_ARM].m_controlFlags & AIK_TARGET_WEAPON_GRIP);

			StoreHelperTransform((eArms)i, m_isActive[i], skeleton, arm, target);

			const bool bGlobalSpace = CalculateTargetPosition(skeleton, arm, target, parentMtx, destMtx, bWeaponGrip);

			destGlobalMtx = destMtx;
			if (!bGlobalSpace)
			{
				Transform(destGlobalMtx, parentMtx, destGlobalMtx);
			}

			Vec3V vToTarget(Subtract(destGlobalMtx.GetCol3(), globalShoulderMtx.GetCol3()));
			ScalarV vDistSq(MagSquared(vToTarget));

			if ((target.m_Blend == 0.0f) && (vDistSq.Getf() > rage::square(target.m_BlendInRange)))
			{
				bActive = false;
			}

			if ((target.m_Blend > 0.0f) && (vDistSq.Getf() > rage::square(target.m_BlendOutRange)))
			{
				bActive = false;
			}

		#if __IK_DEV
			if(CaptureDebugRender())
			{
				sprintf(debugText, "BlendIn Range = %6.4f, BlendOutRange = %6.4f, Range = %6.4f\n", target.m_BlendInRange, target.m_BlendOutRange, Sqrtf(vDistSq.Getf()));
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}
		#endif //__IK_DEV
		}

		target.m_Blend += bActive ? target.m_BlendInRate * deltaTime : -target.m_BlendOutRate * deltaTime;
		target.m_Blend = Clamp(target.m_Blend, 0.0f, 1.0f);

		arm.m_Blend = target.m_Blend;
	}
}

////////////////////////////////////////////////////////////////////////////////

void CArmIkSolver::Update()
{
	IK_DEV_ONLY(char debugText[256];)

	// Early out if any required bones are missing
	if (!IsValid())
	{
		return;
	}

	if (!CanUpdate())
	{
		if (m_pPed->GetUsingRagdoll() || m_pPed->GetDisableArmSolver() || (m_pPed->IsDead() && !m_pPed->GetIsVisibleInSomeViewportThisFrame()))
		{
			Reset();
		}
		return;
	}

	crSkeleton& skeleton = *m_pPed->GetSkeleton();
	Mat34V parentMtx;
	Mat34V globalHandMtx;
	Mat34V destMtx;

	// Check for vehicle attachment since solver is run prior to physical attachment (delayed root attachment).
	// Calculate goal relative to attachment position if necessary since character's root may be offset.
	Mat34V attachParentMtx;
	CPhysical* pAttachParent = GetAttachParent(attachParentMtx);

	// Updating arms in reverse order so that the left arm can potentially use the results from the right. For example, to support recoil with 2-handed weapons.
	for (int i = NUM_ARMS - 1; i >= 0; --i)
	{
		ArmGoal& arm = m_Arms[i];
		Target& target = m_Targets[i];

		if (target.m_Blend > 0.0f)
		{
		#if __IK_DEV
			TUNE_GROUP_INT(IK_DEBUG, DEBUG_SPECIFIC_ARM, -1, -1, 1, 1);
			const bool bRenderDebug = (DEBUG_SPECIFIC_ARM == -1 || DEBUG_SPECIFIC_ARM == i);
			Mat34V mtxGlobal;

			if (bRenderDebug)
			{
				if (CaptureDebugRender())
				{
					sprintf(debugText, "%s\n", i ? "Right Arm" : "Left Arm");
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

					sprintf(debugText, "m_Blend = %6.4f\n", target.m_Blend);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

					sprintf(debugText, "m_BlendInRate = %6.4f\n", target.m_BlendInRate);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

					sprintf(debugText, "m_BlendOutRate = %6.4f\n", target.m_BlendOutRate);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

					atString flagsAsString;
					target.m_controlFlags & AIK_TARGET_WRT_HANDBONE		? flagsAsString += "AIK_TARGET_WRT_HANDBONE | " : "";
					target.m_controlFlags & AIK_TARGET_WRT_POINTHELPER	? flagsAsString += "AIK_TARGET_WRT_POINTHELPER | " : "";
					target.m_controlFlags & AIK_TARGET_WRT_IKHELPER		? flagsAsString += "AIK_TARGET_WRT_IKHELPER | " : "";
					target.m_controlFlags & AIK_TARGET_WRT_CONSTRAINTHELPER ? flagsAsString += "AIK_TARGET_WRT_CONSTRAINTHELPER | " : "";
					target.m_controlFlags & AIK_APPLY_RECOIL			? flagsAsString += "AIK_APPLY_RECOIL | " : "";
					target.m_controlFlags & AIK_USE_FULL_REACH			? flagsAsString += "AIK_USE_FULL_REACH | " : "";
					target.m_controlFlags & AIK_USE_ANIM_ALLOW_TAGS		? flagsAsString += "AIK_USE_ANIM_ALLOW_TAGS | " : "";
					target.m_controlFlags & AIK_USE_ANIM_BLOCK_TAGS		? flagsAsString += "AIK_USE_ANIM_BLOCK_TAGS | " : "";
					target.m_controlFlags & AIK_TARGET_WEAPON_GRIP		? flagsAsString += "AIK_TARGET_WEAPON_GRIP | " : "";
					target.m_controlFlags & AIK_USE_ORIENTATION			? flagsAsString += "AIK_USE_ORIENTATION | " : "";
					target.m_controlFlags & AIK_TARGET_ROTATION			? flagsAsString += "AIK_TARGET_ROTATION | " : "";
					target.m_controlFlags & AIK_APPLY_ALT_SRC_RECOIL	? flagsAsString += "AIK_APPLY_ALT_SRC_RECOIL | " : "";
					target.m_controlFlags & AIK_HOLD_HELPER				? flagsAsString += "AIK_HOLD_HELPER | " : "";
					target.m_controlFlags & AIK_USE_TWIST_CORRECTION	? flagsAsString += "AIK_USE_TWIST_CORRECTION | " : "";

					sprintf(debugText, "m_controlFlags = %s\n", flagsAsString.c_str());
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

					sprintf(debugText, "m_Targets[%i].m_BoneIdx = %i\n", i, target.m_BoneIdx);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				}
			}
		#endif //__IK_DEV

			parentMtx = (target.m_Entity && (target.m_Entity == pAttachParent)) ? attachParentMtx : *skeleton.GetParentMtx();

			skeleton.GetGlobalMtx(arm.m_HandBoneIdx, globalHandMtx);

			const bool bWeaponGrip = (i == LEFT_ARM) && (m_Arms[RIGHT_ARM].m_Enabled) && (m_Targets[LEFT_ARM].m_controlFlags & AIK_TARGET_WEAPON_GRIP);

			StoreHelperTransform((eArms)i, m_isActive[i], skeleton, arm, target);

			const bool bGlobalSpace = CalculateTargetPosition(skeleton, arm, target, parentMtx, destMtx, bWeaponGrip);

		#if __IK_DEV
			if (bRenderDebug && CaptureDebugRender())
			{
				mtxGlobal = destMtx;
				if (!bGlobalSpace)
				{
					Transform(mtxGlobal, parentMtx, mtxGlobal);
				}

				sprintf(debugText, "B - destMtx.d = %6.4f, %6.4f, %6.4f\n", mtxGlobal.GetCol3().GetXf(), mtxGlobal.GetCol3().GetYf(), mtxGlobal.GetCol3().GetZf());
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

				TUNE_GROUP_FLOAT(ARM_IK, fAxisSizeB , 0.02f, 0.0f, 1.0f, 0.001f);
				ms_debugHelper.Axis(RCC_MATRIX34(mtxGlobal), fAxisSizeB);
			}
		#endif //__IK_DEV

			if (bGlobalSpace)
			{
				UnTransformOrtho(destMtx, parentMtx, destMtx);
			}

			// External effector offset
			if (!IsEqualAll(target.m_Additive, QuatV(V_IDENTITY)))
			{
				QuatV qDest(QuatVFromMat34V(destMtx));
				qDest = Multiply(target.m_Additive, qDest);
				Mat34VFromQuatV(destMtx, qDest, destMtx.GetCol3());
			}

			u16 uFlags = 0;

			if (target.m_controlFlags & (AIK_APPLY_RECOIL | AIK_APPLY_ALT_SRC_RECOIL))
			{
				ApplyRecoil((eArms)i, arm, skeleton, destMtx);
				uFlags |= crIKSolverArms::ArmGoal::USE_ORIENTATION;
			}

			if (target.m_controlFlags & AIK_USE_FULL_REACH)
			{
				uFlags |= crIKSolverArms::ArmGoal::USE_FULL_REACH;
			}

			if (target.m_controlFlags & AIK_USE_ORIENTATION)
			{
				uFlags |= crIKSolverArms::ArmGoal::USE_ORIENTATION;
			}

			if (target.m_controlFlags & AIK_USE_TWIST_CORRECTION)
			{
				uFlags |= crIKSolverArms::ArmGoal::USE_TWIST_CORRECTION;
			}

			// Interpolate
			const Mat34V& mtxHand = skeleton.GetObjectMtx(arm.m_HandBoneIdx);

			destMtx.SetCol3(Lerp(ScalarV(target.m_Blend), mtxHand.GetCol3(), destMtx.GetCol3()));

			if ((target.m_controlFlags & (AIK_USE_ORIENTATION | AIK_APPLY_RECOIL | AIK_APPLY_ALT_SRC_RECOIL)) && (target.m_Blend < 1.0f))
			{
				QuatV qHand(QuatVFromMat34V(mtxHand));
				QuatV qDest(QuatVFromMat34V(destMtx));

				if (CanSlerp(qHand, qDest))
				{
					qDest = PrepareSlerp(qHand, qDest);
					qDest = Slerp(ScalarV(target.m_Blend), qHand, qDest);

					Mat34VFromQuatV(destMtx, qDest, destMtx.GetCol3());
				}
			}

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "C - destMtx.d = %6.4f, %6.4f, %6.4f\n", destMtx.GetCol3().GetXf(), destMtx.GetCol3().GetYf(), destMtx.GetCol3().GetZf());
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}
		#endif //__IK_DEV

			// Update
			arm.m_Target = destMtx;
			arm.m_Flags = uFlags;
			arm.m_Enabled = true;

		#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "%s\n", i ? "Right Arm" : "Left Arm");
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

				Matrix34 p0_Mtx;
				skeleton.GetGlobalMtx(arm.m_HandBoneIdx, RC_MAT34V(p0_Mtx));
				Matrix34 p3_Mtx;
				skeleton.GetGlobalMtx(arm.m_ElbowBoneIdx, RC_MAT34V(p3_Mtx));
				Matrix34 p1_Mtx;
				skeleton.GetGlobalMtx(arm.m_ShoulderBoneIdx, RC_MAT34V(p1_Mtx));
				Mat34V targetMat34V(arm.m_Target); 
				Transform(targetMat34V, *skeleton.GetParentMtx(), targetMat34V);
				Matrix34 target_Mtx(RC_MATRIX34(targetMat34V));

				// Work out the rotation between p1_to_target and p1_to_p0
				Vector3 p1_to_target(target_Mtx.d-p1_Mtx.d);
				float d = p1_to_target.Mag();

				float r0 = (p3_Mtx.d-p0_Mtx.d).Mag();
				float r1 = (p3_Mtx.d-p1_Mtx.d).Mag();

				float fPercentageReach = (target.m_controlFlags & AIK_USE_FULL_REACH) ? 0.98f : 0.95f;
				float max_d = (r0 + r1) * fPercentageReach;
				if(d>max_d)
				{
					// Target is out of reach
					sprintf(debugText, "d = %6.4f, max_d = %6.4f, new_d = %6.4f\n", d, max_d, max_d);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_red, debugText);
				}
				else
				{
					// Target is within reach
					sprintf(debugText, "d = %6.4f, max_d = %6.4f, new_d = %6.4f\n", d, max_d, d);
					ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
				}
			}
		#endif //__IK_DEV

// For testing non vectorised ARM IK on PPU
/*
			Vector3 hVector(0.0f, 0.0f, 1.0f);

			Matrix34 p0_Mtx;
			skeleton.GetGlobalMtx(arm.m_HandBoneIdx, RC_MAT34V(p0_Mtx));
			Matrix34 p3_Mtx;
			skeleton.GetGlobalMtx(arm.m_ElbowBoneIdx, RC_MAT34V(p3_Mtx));
			Matrix34 p1_Mtx;
			skeleton.GetGlobalMtx(arm.m_ShoulderBoneIdx, RC_MAT34V(p1_Mtx));
			float r0 = (p3_Mtx.d-p0_Mtx.d).Mag();
			float r1 = (p3_Mtx.d-p1_Mtx.d).Mag();

			Matrix34 target_Mtx(RC_MATRIX34(arm.m_Target));
			Vector3 p1_to_p0(p0_Mtx.d-p1_Mtx.d);

			{
				// Work out p2 and h (and the direction of h) for the original arm
				// P0 = hand.d
				// P1 = shoulder.d
				// P2 = point that forms 2 right handed tris
				// P3 = elbow.d
				// r0 = |P3-P0| = lowerArmLen
				// r1 = |P3-P1| = upperArmLen
				// a = |P2-P0| 
				// b = |P2-P1|

				Vector3 p2(VEC3_ZERO);
				float h = 0.0f;
				PPUCalculateP2AndH(p0_Mtx.d, p1_Mtx.d, r0, r1, p2, h);

				// The elbow is h distance from P2
				Vector3 p2_to_p3(p3_Mtx.d-p2);

				// The direction of p3 from p2
				hVector = p2_to_p3;			

#if __IK_DEV
				if(CaptureDebugRender())
				{
					static dev_float bSphereSize = 0.01f;
					ms_debugHelper.Sphere(p2, bSphereSize, Color_purple);
					ms_debugHelper.Line(p2, p2+hVector, Color_purple, Color_purple);
				}
#endif //__IK_DEV

			} // Original arm

			// Work out p2 and h for the target arm (use the direction of h from the original arm)
			{
				Vector3 p2(VEC3_ZERO);
				float h = 0.0f;

				// Work out the rotation between p1_to_target and p1_to_p0
				Vector3 p1_to_target(target_Mtx.d-p1_Mtx.d);
				float d = p1_to_target.Mag();

#if __IK_DEV
				if(CaptureDebugRender())
				{
					ms_debugHelper.Line(p1_Mtx.d, p1_Mtx.d+p1_to_target, Color_purple, Color_purple);
					ms_debugHelper.Line(p1_Mtx.d, p1_Mtx.d+p1_to_p0, Color_purple, Color_purple);
				}
#endif //__IK_DEV

				p1_to_p0.Normalize();
				p1_to_target.Normalize();

				Matrix34 diff(M34_IDENTITY);
				diff.MakeRotateTo(p1_to_p0, p1_to_target);
				//p1_Mtx.Dot3x3(diff);
				//p3_Mtx.d = p1_Mtx.d + (p1_Mtx.a * r1);
				diff.Transform3x3(hVector);

//#if __IK_DEV
				//if(CaptureDebugRender())
				//{
				//	ms_debugHelper.Line(p1_Mtx.d, p3_Mtx.d, Color_white, Color_white);
				//}
//#endif //__IK_DEV
				static float percentageReach = 0.95f;
				float max_d = r0+r1 * percentageReach;
				if(d>max_d)
				{
					// Target is out of reach 
					// Calculate nearest point to target we can reach
					p1_to_target.Normalize();
					p1_to_target = p1_to_target * max_d;
					p0_Mtx.d = p1_Mtx.d + p1_to_target;
					d = max_d;
				}
				else
				{
					// Target is within reach
					p0_Mtx.d  = target_Mtx.d;
				}

				// P0 = hand.d
				// P1 = shoulder.d
				// P2 = point that forms 2 right handed tris
				// P3 = elbow.d
				// r0 = |P3-P0| = lowerArmLen
				// r1 = |P3-P1| = upperArmLen
				// a = |P2-P0| 
				// b = |P2-P1|
				// d = |P1-P0| = hadnToShoulder
				PPUCalculateP2AndH(p0_Mtx.d, p1_Mtx.d, r0, r1, p2, h);

				// The elbow is h distance from P2 we just need a direction to go in.				
				ikAssert(rage::FPIsFinite(hVector.x));
				ikAssert(rage::FPIsFinite(hVector.y));
				ikAssert(rage::FPIsFinite(hVector.z));
				hVector.Normalize();
				hVector *= h;
				p3_Mtx.d = p2+hVector;
				ikAssert(rage::FPIsFinite(p3_Mtx.d.x));
				ikAssert(rage::FPIsFinite(p3_Mtx.d.y));
				ikAssert(rage::FPIsFinite(p3_Mtx.d.z));

#if __IK_DEV
				if(CaptureDebugRender())
				{
					static dev_float bSphereSize = 0.01f;
					ms_debugHelper.Sphere(p2, bSphereSize, Color_purple);
					ms_debugHelper.Line(p2, p2+hVector, Color_purple, Color_purple);
				}
#endif //__IK_DEV			
				p1_Mtx.Dot3x3(diff);
				skeleton.SetGlobalMtx(arm.m_ShoulderBoneIdx, RC_MAT34V(p1_Mtx));
				skeleton.SetGlobalMtx(arm.m_HandBoneIdx, RC_MAT34V(p0_Mtx));
				skeleton.SetGlobalMtx(arm.m_ElbowBoneIdx, RC_MAT34V(p3_Mtx));
				m_pPed->GetIkManager().OrientateBoneATowardBoneB(arm.m_ShoulderBoneIdx, arm.m_ElbowBoneIdx);
				m_pPed->GetIkManager().OrientateBoneATowardBoneB(arm.m_ElbowBoneIdx, arm.m_HandBoneIdx);

				// Move the hand to the reachable target updating any child bones (e.g. finger bones)
				for(int i=arm.m_HandBoneIdx+1; i<arm.m_TerminatingIdx; ++i)
				{
					s32 parentIdx = m_pPed->GetSkeletonData().GetParentIndex(i); 
					Transform(skeleton.GetObjectMtx(i), skeleton.GetObjectMtx(parentIdx), skeleton.GetLocalMtx(i));
				}

				//skeleton.InverseUpdate();
				//skeleton.Update();

			} // // Target arm
*/
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::Reset()
{
	for(int i=0; i < NUM_ARMS; ++i)
	{
		m_isActive[i] = false;

		Target& target = m_Targets[i];
		target.m_Blend = 0.0f;

		ArmGoal& arm = m_Arms[i];
		arm.m_Blend = 0.0f;
		arm.m_Enabled = false;
	}
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::Iterate(float UNUSED_PARAM(dt), crSkeleton& UNUSED_PARAM(skel))
{
	// Iterate is called by the motiontree thread
	// It is called every frame, except when paused, except when offscreen
#if __IK_DEV || __BANK
	if (CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeArm] || CBaseIkManager::ms_DisableAllSolvers)
	{
		return;
	}
#endif // __IK_DEV || __BANK

#if __IK_DEV
	if(CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "%s", "ArmIK::Iterate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update();
}
//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::PostIkUpdate(float deltaTime)
{
	// Called every frame, even when paused, even when offscreen

	if (deltaTime > 0.0f)
	{
		// Reset since solver must be requested each frame
		for (int i = 0; i < NUM_ARMS; ++i)
		{
			m_isActive[i] = false;
		}
	}

	/*
#if __IK_DEV
	ms_debugHelper.Reset();
	ms_iNoOfTexts = 0;

	if(CaptureDebugRender())
	{
		char debugText[100];
		sprintf(debugText, "%s", "ArmIK::PostIkUpdate");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	Update();*/
 
	// For testing vectorised ARM IK on PPU
	/*for(int i=0; i < NUM_ARMS; ++i)
	{
		const ArmGoal& arm = m_Arms[i];
		if(arm.m_Enabled)
		{
			SolverHelper sh;
			sh.m_Locals = m_pPed->GetSkeleton()->GetLocalMtxs();
			sh.m_Objects = m_pPed->GetSkeleton()->GetObjectMtxs();
			sh.m_NumBones = m_pPed->GetSkeletonData().GetNumBones();
			sh.m_Parent = *m_pPed->GetSkeleton()->GetParentMtx();
			sh.m_ParentIndices = m_pPed->GetSkeletonData().GetParentIndices();
			arm.Solve(sh);
		}
	}*/
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::Request(const CIkRequestArm& request)
{
	ikAssert(IsFiniteAll(request.GetTargetOffset()));
	ikAssert(IsFiniteAll(request.GetTargetRotation()));
	ikAssert(IsFiniteAll(request.GetAdditive()));

	const eArms arm = request.GetArm() == IK_ARM_LEFT ? LEFT_ARM : RIGHT_ARM;
	m_isActive[arm] = true;

	Target& target = m_Targets[arm];
	if (request.GetTargetEntity() && request.GetTargetEntity()->GetIsDynamic())
	{
		target.m_Entity = RegdConstDyn((CDynamicEntity*)request.GetTargetEntity());
	}
	else
	{
		target.m_Entity = NULL;
	}
	target.m_BoneIdx = (int)(target.m_Entity && (request.GetTargetBoneTag() != BONETAG_INVALID) ? target.m_Entity->GetBoneIndexFromBoneTag(request.GetTargetBoneTag()) : -1);
	target.m_Offset = request.GetTargetOffset();
	target.m_Additive = request.GetAdditive();
	target.m_Rotation = request.GetTargetRotation();
	target.m_controlFlags = request.GetFlags() | AIK_TARGET_ROTATION;
	target.m_BlendInRange = request.GetBlendInRange();
	target.m_BlendOutRange = request.GetBlendOutRange();

	if (request.IsBlendInRateAnEnum())
	{
		target.m_BlendInRate =(ms_afBlendDuration[request.GetBlendInRate()] != 0.0f) ? 1.0f / ms_afBlendDuration[request.GetBlendInRate()] : INSTANT_BLEND_IN_DELTA;
	}
	else
	{
		target.m_BlendInRate =request.GetBlendInRateFloat();
	}

	if (request.IsBlendOutRateAnEnum())
	{
		target.m_BlendOutRate =(ms_afBlendDuration[request.GetBlendOutRate()] != 0.0f) ? 1.0f / ms_afBlendDuration[request.GetBlendOutRate()] : INSTANT_BLEND_IN_DELTA;
	}
	else
	{
		target.m_BlendOutRate =request.GetBlendOutRateFloat();
	}
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::SetAbsoluteTarget(crIKSolverArms::eArms side, Vec3V_In targetPos, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{ 
	ikAssert(rage::FPIsFinite(targetPos.GetXf()));
	ikAssert(rage::FPIsFinite(targetPos.GetYf()));
	ikAssert(rage::FPIsFinite(targetPos.GetZf()));

	m_isActive[side]=true;
	Target& target = m_Targets[side];
	target.m_Offset = targetPos;
	target.m_Additive = QuatV(V_IDENTITY);
	target.m_Rotation = QuatV(V_IDENTITY);
	target.m_controlFlags = controlFlags;
	target.m_BlendInRate = blendInTime != 0.0f ? 1.0f / blendInTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendOutRate = blendOutTime != 0.0f ? 1.0f / blendOutTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendInRange = blendInRange;
	target.m_BlendOutRange = blendOutRange;

	// unused 
	target.m_Entity = NULL;
	target.m_BoneIdx = -1;
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::SetRelativeTarget(crIKSolverArms::eArms side, const CDynamicEntity* entity, int boneIdx, Vec3V_In offset, s32 controlFlags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{
	ikAssert(rage::FPIsFinite(offset.GetXf()));
	ikAssert(rage::FPIsFinite(offset.GetYf()));
	ikAssert(rage::FPIsFinite(offset.GetZf()));

	m_isActive[side]=true;
	Target& target = m_Targets[side];
	target.m_Entity = RegdConstDyn(entity);
	target.m_BoneIdx = boneIdx;
	target.m_Offset = offset;
	target.m_Additive = QuatV(V_IDENTITY);
	target.m_Rotation = QuatV(V_IDENTITY);
	target.m_controlFlags = controlFlags;
	target.m_BlendInRate = blendInTime != 0.0f ? 1.0f / blendInTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendOutRate = blendOutTime != 0.0f ? 1.0f / blendOutTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendInRange = blendInRange;
	target.m_BlendOutRange = blendOutRange;
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::SetRelativeTarget(crIKSolverArms::eArms side, const CDynamicEntity* entity, int boneIdx, Vec3V_In offset, QuatV_In rotation, s32 flags, float blendInTime, float blendOutTime, float blendInRange, float blendOutRange)
{
	ikAssert(rage::FPIsFinite(offset.GetXf()));
	ikAssert(rage::FPIsFinite(offset.GetYf()));
	ikAssert(rage::FPIsFinite(offset.GetZf()));

	m_isActive[side]=true;
	Target& target = m_Targets[side];
	target.m_Entity = RegdConstDyn(entity);
	target.m_BoneIdx = boneIdx;
	target.m_Offset = offset;
	target.m_Additive = QuatV(V_IDENTITY);
	target.m_Rotation = rotation;
	target.m_controlFlags = flags | AIK_TARGET_ROTATION;
	target.m_BlendInRate = blendInTime != 0.0f ? 1.0f / blendInTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendOutRate = blendOutTime != 0.0f ? 1.0f / blendOutTime : INSTANT_BLEND_IN_DELTA;
	target.m_BlendInRange = blendInRange;
	target.m_BlendOutRange = blendOutRange;
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::ResetArm(eArms arm)
{
	m_Targets[arm].m_Blend = 0.0f;
	m_isActive[arm] = false;
}

//////////////////////////////////////////////////////////////////////////

#if ENABLE_FRAG_OPTIMIZATION
void CArmIkSolver::GetGlobalMtx(const CDynamicEntity* entity, const s32 boneIdx, Mat34V_InOut outMtx) const
{

	const crSkeleton* pSkeleton = entity->GetSkeleton();

	// Calling fwEntity::GetGlobalMtx() on an entity with a skeleton results in waiting for animations to complete
	// Waiting for animations to complete in the motion tree update may result in a deadlock
	if(pSkeleton)
	{
		pSkeleton->GetGlobalMtx(boneIdx, outMtx);
	}
	else
	{
		// Calling fwEntity::GetGlobalMtx() on an entity without a skeleton does not wait for animations to complete
		entity->GetGlobalMtx(boneIdx, RC_MATRIX34(outMtx));
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

Vec3V_Out CArmIkSolver::GetTargetPosition(eArms arm) const
{
	crSkeleton* pSkeleton = m_pPed->GetSkeleton();

	if (pSkeleton == NULL)
	{
		return Vec3V(V_ZERO);
	}

	const Target& target = m_Targets[arm];
	const ArmGoal& armGoal = m_Arms[arm];

	Mat34V mtxTarget(V_IDENTITY);
	Mat34V mtxHelper;
	Mat34V mtxInverse;

	Mat34V mtxHand;
	pSkeleton->GetGlobalMtx(armGoal.m_HandBoneIdx, mtxHand);

	if (target.m_Entity)
	{
		mtxTarget.SetCol3(target.m_Offset);

		if (target.m_controlFlags & AIK_TARGET_ROTATION)
		{
			Mat34VFromQuatV(mtxTarget, target.m_Rotation, target.m_Offset);
		}

		if (target.m_BoneIdx != -1)
		{
			Mat34V mtxBone;
#if ENABLE_FRAG_OPTIMIZATION
			GetGlobalMtx(target.m_Entity, target.m_BoneIdx, mtxBone);
#else
			target.m_Entity->GetSkeleton()->GetGlobalMtx(target.m_BoneIdx, mtxBone);
#endif					
			Transform(mtxTarget, mtxBone, mtxTarget);
		}
		else
		{
			Transform(mtxTarget, target.m_Entity->GetMatrix(), mtxTarget);
		}
	}
	else
	{
		mtxTarget = mtxHand;
		mtxTarget.SetCol3(target.m_Offset);
	}

	if (target.m_controlFlags & AIK_TARGET_WRT_POINTHELPER)
	{
		if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
		{
			pSkeleton->GetGlobalMtx(armGoal.m_PointHelperBoneIdx, mtxHelper);
			mtxTarget.SetCol3(Add(mtxTarget.GetCol3(), Subtract(mtxHand.GetCol3(), mtxHelper.GetCol3())));
		}
		else
		{
			// Since point helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
			InvertTransformFull(mtxInverse, pSkeleton->GetLocalMtx(armGoal.m_PointHelperBoneIdx));
			Transform(mtxTarget, mtxTarget, mtxInverse);
		}
	}
	else if (target.m_controlFlags & AIK_TARGET_WRT_IKHELPER)
	{
		if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
		{
			pSkeleton->GetGlobalMtx(armGoal.m_HandIkHelperBoneIdx, mtxHelper);
			mtxTarget.SetCol3(Add(mtxTarget.GetCol3(), Subtract(mtxHand.GetCol3(), mtxHelper.GetCol3())));
		}
		else
		{
			// Since IK helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
			InvertTransformFull(mtxInverse, pSkeleton->GetLocalMtx(armGoal.m_HandIkHelperBoneIdx));
			Transform(mtxTarget, mtxTarget, mtxInverse);
		}
	}
	else if (target.m_controlFlags & AIK_TARGET_WRT_CONSTRAINTHELPER)
	{
		float fCHWeight;
		Vec3V vCHTranslation;
		QuatV qCHRotation;

		const bool bValid = m_pPed->GetConstraintHelperDOFs((arm == RIGHT_ARM), fCHWeight, vCHTranslation, qCHRotation);
		if (bValid)
		{
			Mat34VFromQuatV(mtxHelper, qCHRotation, vCHTranslation);

			if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
			{
				Transform(mtxHelper, mtxHand, mtxHelper);
				mtxTarget.SetCol3(Add(mtxTarget.GetCol3(), Subtract(mtxHand.GetCol3(), mtxHelper.GetCol3())));
			}
			else
			{
				// Since helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
				InvertTransformFull(mtxInverse, mtxHelper);
				Transform(mtxTarget, mtxTarget, mtxInverse);
			}
		}
	}

	return mtxTarget.GetCol3();
}

//////////////////////////////////////////////////////////////////////////

bool CArmIkSolver::IsBlocked() const
{
	return m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_IsHandCuffed) ||
		   m_pPed->GetMovePed().GetBlendHelper().IsFlagSet(APF_BLOCK_IK) ||
		   m_pPed->GetMovePed().GetSecondaryBlendHelper().IsFlagSet(APF_BLOCK_IK);
}

//////////////////////////////////////////////////////////////////////////

bool CArmIkSolver::CanUpdate() const
{
	if (m_pPed->GetDisableArmSolver())
		return false;

	if (fwTimer::IsGamePaused())
		return false;
	
	if (m_pPed->m_nDEflags.bFrozenByInterior)
		return false;
	
	if (m_pPed->GetUsingRagdoll())
		return false;

	if (m_pPed->IsDead() && !m_pPed->GetIsVisibleInSomeViewportThisFrame())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////

bool CArmIkSolver::CalculateTargetPosition(const rage::crSkeleton& skeleton, const crIKSolverArms::ArmGoal& arm, const CArmIkSolver::Target& target, Mat34V& parentMtx, Mat34V& destMtx, bool bWeaponGrip)
{
	IK_DEV_ONLY(char debugText[100];)

	bool bGlobalSpace = true;

	Mat34V mtxInverse;

	Mat34V handMtx;
	skeleton.GetGlobalMtx(arm.m_HandBoneIdx, handMtx);
	Mat34V globalPointHelperMtx;
	skeleton.GetGlobalMtx(arm.m_PointHelperBoneIdx, globalPointHelperMtx);
	Mat34V globalIkHelperMtx;
	skeleton.GetGlobalMtx(arm.m_HandIkHelperBoneIdx, globalIkHelperMtx);

#if __IK_DEV
	Mat34V mtxGlobal;

	if(CaptureDebugRender())
	{
		static dev_float bSphereSize = 0.01f;
		static dev_float bAxisSize = 0.02f;

		// Render the original animated arm
		Mat34V globalUpperArmMtx;
		skeleton.GetGlobalMtx(arm.m_ShoulderBoneIdx, globalUpperArmMtx);
		Vec3V globalUpperArmPos(globalUpperArmMtx.d());
		Mat34V globalForeArmMtx;
		skeleton.GetGlobalMtx(arm.m_ElbowBoneIdx, globalForeArmMtx);
		Vec3V globalForeArmPos(globalForeArmMtx.d());
		Vec3V globalHandPos(handMtx.d());
		Vec3V globalPointHelperPos(globalPointHelperMtx.d());
		Vec3V globalIkHelperPos(globalIkHelperMtx.d());

		ms_debugHelper.Line(RC_VECTOR3(globalUpperArmPos), RC_VECTOR3(globalForeArmPos), Color_green, Color_green);
		ms_debugHelper.Sphere(RC_VECTOR3(globalUpperArmPos), bSphereSize, Color_green);
		ms_debugHelper.Line(RC_VECTOR3(globalForeArmPos), RC_VECTOR3(globalHandPos), Color_green, Color_green);
		ms_debugHelper.Sphere(RC_VECTOR3(globalForeArmPos), bSphereSize, Color_green);
		ms_debugHelper.Sphere(RC_VECTOR3(globalHandPos), bSphereSize, Color_green);

		ms_debugHelper.Sphere(RC_VECTOR3(globalPointHelperPos), bSphereSize, Color_white);
		ms_debugHelper.Sphere(RC_VECTOR3(globalIkHelperPos), bSphereSize, Color_cyan);

		sprintf(debugText, "globalHandMtx.d = %6.4f, %6.4f, %6.4f\n", handMtx.GetCol3().GetXf(), handMtx.GetCol3().GetYf(), handMtx.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_green, debugText);
		ms_debugHelper.Axis(RCC_MATRIX34(handMtx), bAxisSize);

		sprintf(debugText, "globalPointHelperMtx.d = %6.4f, %6.4f, %6.4f\n", globalPointHelperMtx.GetCol3().GetXf(), globalPointHelperMtx.GetCol3().GetYf(), globalPointHelperMtx.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		TUNE_GROUP_FLOAT(ARM_IK, fPointHelperAxisSize , 0.02f, 0.0f, 1.0f, 0.001f);
		ms_debugHelper.Axis(RCC_MATRIX34(globalPointHelperMtx), fPointHelperAxisSize);

		sprintf(debugText, "globalIkHelperMtx.d = %6.4f, %6.4f, %6.4f\n", globalIkHelperMtx.GetCol3().GetXf(), globalIkHelperMtx.GetCol3().GetYf(), globalIkHelperMtx.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_cyan, debugText);
		TUNE_GROUP_FLOAT(ARM_IK, fIkHelperAxisSize , 0.02f, 0.0f, 1.0f, 0.001f);
		ms_debugHelper.Axis(RCC_MATRIX34(globalIkHelperMtx), fIkHelperAxisSize);

		Vector3 direction = RC_VECTOR3(globalHandPos) - RC_VECTOR3(globalForeArmPos);
		float foreArmLength = direction.Mag();
		direction = RC_VECTOR3(globalForeArmPos) - RC_VECTOR3(globalUpperArmPos);
		float upperArmLength = direction.Mag();
		direction = RC_VECTOR3(globalHandPos) - RC_VECTOR3(globalUpperArmPos);
		float diff = direction.Mag();
		sprintf(debugText, "Before - forearm length = %6.4f, upperarm length = %6.4f, diff = %6.4f,\n", foreArmLength, upperArmLength, diff);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV

	if(target.m_Entity)
	{
		// Calculate target in object space if target entity is this ped
		bGlobalSpace = target.m_Entity != m_pPed;

		destMtx = Mat34V(V_IDENTITY);
		destMtx.GetCol3Ref() = target.m_Offset;

		if (target.m_controlFlags & AIK_TARGET_ROTATION)
		{
			Mat34VFromQuatV(destMtx, target.m_Rotation, target.m_Offset);
		}

		if (target.m_BoneIdx!=-1)
		{
			Mat34V boneMtx;
			if (!bWeaponGrip)
			{
				if (bGlobalSpace)
				{
#if ENABLE_FRAG_OPTIMIZATION
					GetGlobalMtx(target.m_Entity, target.m_BoneIdx, boneMtx);
#else
					target.m_Entity->GetSkeleton()->GetGlobalMtx(target.m_BoneIdx, boneMtx);
#endif
				}
				else
				{
					boneMtx = skeleton.GetObjectMtx(target.m_BoneIdx);
				}
			}
			else
			{
				ikAssertf(!bGlobalSpace, "Weapon grip setup assumed to be relative to this ped");

				// Left arm weapon grip is setup as a local offset from the right point helper bone.
				// Cannot use the right point helper matrix directly from the skeleton above since
				// the right arm could have been adjusted. Instead, calculating the updated
				// location of the right point helper bone from the pending right arm target.

				// Calculate updated right hand point helper matrix from right hand target (point helper is a child of the hand)
				Transform(boneMtx, m_Arms[RIGHT_ARM].m_Target, skeleton.GetLocalMtx(target.m_BoneIdx));
			}
			Transform(destMtx, boneMtx, destMtx);
		}
		else if (bGlobalSpace)
		{
			Transform(destMtx, target.m_Entity->GetMatrix(), destMtx);
		}
	}
	else
	{
		destMtx = handMtx;
		destMtx.GetCol3Ref() = target.m_Offset;
	}

#if __IK_DEV
	if(CaptureDebugRender())
	{
		mtxGlobal = destMtx;
		if (!bGlobalSpace)
		{
			Transform(mtxGlobal, parentMtx, mtxGlobal);
		}

		sprintf(debugText, "A - destMtx.d = %6.4f, %6.4f, %6.4f\n", mtxGlobal.GetCol3().GetXf(), mtxGlobal.GetCol3().GetYf(), mtxGlobal.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		TUNE_GROUP_FLOAT(ARM_IK, fAxisSizeA , 0.02f, 0.0f, 1.0f, 0.001f);
		ms_debugHelper.Axis(RCC_MATRIX34(mtxGlobal), fAxisSizeA);
	}
#endif //__IK_DEV

	if (target.m_controlFlags & AIK_TARGET_WRT_HANDBONE)
	{
		// do nothing
#if __IK_DEV
		if(CaptureDebugRender())
		{
			Vec3V subtract(V_ZERO);
			sprintf(debugText, "AIK_TARGET_WRT_HANDBONE = %6.4f, %6.4f, %6.4f\n", subtract.GetXf(), subtract.GetYf(), subtract.GetZf());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
#endif //__IK_DEV
	}
	else if (target.m_controlFlags & AIK_TARGET_WRT_POINTHELPER)
	{
		if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
		{
			if (bGlobalSpace)
			{
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(handMtx.GetCol3(), globalPointHelperMtx.GetCol3()));
			}
			else
			{
				const Mat34V& mtxHand = skeleton.GetObjectMtx(arm.m_HandBoneIdx);
				const Mat34V& mtxPH = skeleton.GetObjectMtx(arm.m_PointHelperBoneIdx);
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(mtxHand.GetCol3(), mtxPH.GetCol3()));
			}
		}
		else
		{
			// Since point helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
			Mat34V mtxHelper;
			Mat34VFromTransformV(mtxHelper, target.m_Helper);
			InvertTransformFull(mtxInverse, mtxHelper);

			Transform(destMtx, destMtx, mtxInverse);
		}

#if __IK_DEV
		if(CaptureDebugRender())
		{
			Vec3V subtract(Subtract(handMtx.GetCol3(), globalIkHelperMtx.GetCol3()));
			sprintf(debugText, "AIK_TARGET_WRT_POINTHELPER = %6.4f, %6.4f, %6.4f\n", subtract.GetXf(), subtract.GetYf(), subtract.GetZf());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
#endif //__IK_DEV
	}
	else if (target.m_controlFlags & AIK_TARGET_WRT_IKHELPER)
	{
		if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
		{
			if (bGlobalSpace)
			{
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(handMtx.GetCol3(), globalIkHelperMtx.GetCol3()));
			}
			else
			{
				const Mat34V& mtxHand = skeleton.GetObjectMtx(arm.m_HandBoneIdx);
				const Mat34V& mtxIH = skeleton.GetObjectMtx(arm.m_HandIkHelperBoneIdx);
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(mtxHand.GetCol3(), mtxIH.GetCol3()));
			}
		}
		else
		{
			// Since IK helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
			Mat34V mtxHelper;
			Mat34VFromTransformV(mtxHelper, target.m_Helper);
			InvertTransformFull(mtxInverse, mtxHelper);

			Transform(destMtx, destMtx, mtxInverse);
		}

#if __IK_DEV
		if(CaptureDebugRender())
		{
			Vec3V subtract(Subtract(handMtx.GetCol3(), globalIkHelperMtx.GetCol3()));
			sprintf(debugText, "AIK_TARGET_WRT_IKHELPER = %6.4f, %6.4f, %6.4f\n", subtract.GetXf(), subtract.GetYf(), subtract.GetZf());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
#endif //__IK_DEV
	}
	else if (target.m_controlFlags & AIK_TARGET_WRT_CONSTRAINTHELPER)
	{
		Mat34V mtxHelper;
		Mat34VFromTransformV(mtxHelper, target.m_Helper);

		if (!(target.m_controlFlags & AIK_USE_ORIENTATION))
		{
			Mat34V mtxCH;

			if (bGlobalSpace)
			{
				Transform(mtxCH, handMtx, mtxHelper);
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(handMtx.GetCol3(), mtxCH.GetCol3()));
			}
			else
			{
				const Mat34V& mtxHand = skeleton.GetObjectMtx(arm.m_HandBoneIdx);
				Transform(mtxCH, mtxHand, mtxHelper);
				destMtx.GetCol3Ref() = Add(destMtx.GetCol3(), Subtract(mtxHand.GetCol3(), mtxCH.GetCol3()));
			}
		}
		else
		{
			// Since helper bone is assumed to be snapped to the target, transform back to get required position/orientation of hand bone
			InvertTransformFull(mtxInverse, mtxHelper);
			Transform(destMtx, destMtx, mtxInverse);
		}

#if __IK_DEV
		if(CaptureDebugRender())
		{
			Transform(mtxGlobal, handMtx, mtxHelper);

			Vec3V subtract(Subtract(handMtx.GetCol3(), mtxGlobal.GetCol3()));
			sprintf(debugText, "AIK_TARGET_WRT_CONSTRAINTHELPER = %6.4f, %6.4f, %6.4f\n", subtract.GetXf(), subtract.GetYf(), subtract.GetZf());
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		}
#endif //__IK_DEV
	}

	// Adjust target position to compensate for the high heel expression that will run after the solve.
	if (m_pPed->GetPedConfigFlag(CPED_CONFIG_FLAG_HasHighHeels))
	{
		CObject* pWeaponObject = m_pPed->GetWeaponManager() ? m_pPed->GetWeaponManager()->GetEquippedWeaponObject() : NULL;

		// But only if the target position is not relative to ourself or our weapon.
		if ((target.m_Entity == NULL) || ((target.m_Entity != m_pPed) && (target.m_Entity != pWeaponObject)))
		{
			const float fHighHeelHeight = m_pPed->GetIkManager().GetHighHeelHeight();
			const float fHighHeelBlend = m_pPed->GetHighHeelExpressionDOF();

			Vec3V vOffset(V_UP_AXIS_WZERO);
			if (!bGlobalSpace)
			{
				vOffset = UnTransform3x3Full(parentMtx, vOffset);
			}
			vOffset = Scale(vOffset, ScalarV(fHighHeelBlend * fHighHeelHeight));

			destMtx.GetCol3Ref() = Subtract(destMtx.GetCol3(), vOffset);

#if __IK_DEV
			if (CaptureDebugRender())
			{
				sprintf(debugText, "High Heel Adjustment = %4.2f %6.3f\n", fHighHeelBlend, -fHighHeelBlend * fHighHeelHeight);
				ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			}
#endif //__IK_DEV
		}
	}

	return bGlobalSpace;
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::ApplyRecoil(const eArms arm, const crIKSolverArms::ArmGoal& armGoal, const rage::crSkeleton& skeleton, Mat34V& destMtx)
{
	if (m_pPed->GetWeaponManager() == NULL)
		return;

	const CWeapon* pWeapon = m_pPed->GetWeaponManager()->GetEquippedWeapon();

	if (pWeapon == NULL)
		return;

	if (pWeapon->GetEntity() == NULL)
		return;

	const CWeaponModelInfo* pWeaponModelInfo = pWeapon->GetWeaponModelInfo();
	const crSkeleton* pWeaponSkeleton = pWeapon->GetEntity() ? pWeapon->GetEntity()->GetSkeleton() : NULL;

	if ((pWeaponModelInfo == NULL) || (pWeaponSkeleton == NULL))
		return;

	Vec3V vAxis;
	ScalarV vAngle;

	Mat34V mtxRecoil(V_IDENTITY);

	if ((m_Targets[arm].m_controlFlags & AIK_APPLY_ALT_SRC_RECOIL) != 0)
	{
		if ((armGoal.m_ForeArmRollBoneIdx >= 0) && (armGoal.m_UpperArmRollBoneIdx >= 0))
		{
			// Rotation
			mtxRecoil = skeleton.GetLocalMtx(armGoal.m_ForeArmRollBoneIdx);

			{
				TUNE_GROUP_FLOAT(ARM_IK, fRotationScale, 1.0f, 0.0f, 10.0f, 0.01f);
				Vec3V vEulers(Mat34VToEulersXYZ(mtxRecoil));
				vEulers = Scale(vEulers, ScalarV(fRotationScale));
				Mat34VFromEulersXYZ(mtxRecoil, vEulers);
			}

			// Translation
			mtxRecoil.GetCol3Ref() = Scale(Mat34VToEulersXYZ(skeleton.GetLocalMtx(armGoal.m_UpperArmRollBoneIdx)), ScalarV(V_TO_DEGREES));

			{
				TUNE_GROUP_FLOAT(ARM_IK, fTranslationScale, 1.0f, 0.0f, 1000.0f, 1.0f);
				mtxRecoil.GetCol3Ref() = Scale(mtxRecoil.GetCol3(), ScalarV(fTranslationScale));
			}
		}
	}
	else
	{
		// Always read recoil information from right IK helper
		mtxRecoil = skeleton.GetLocalMtx(m_Arms[RIGHT_ARM].m_HandIkHelperBoneIdx);
	}

	QuatV qRecoil(QuatVFromMat34V(mtxRecoil));
	QuatVToAxisAngle(vAxis, vAngle, qRecoil);

	if (IsLessThanAll(Mag(mtxRecoil.GetCol3()), ScalarV(V_FLT_SMALL_3)) && IsLessThanAll(Abs(vAngle), ScalarV(V_FLT_SMALL_2)))
	{
		return;
	}

	Mat34V mtxWeapon;

	// Weapon grip matrix
	Mat34V mtxWeaponGrip(V_IDENTITY);
	s32 gripBoneIdx = pWeaponModelInfo->GetBoneIndex(WEAPON_GRIP_R);
	if (gripBoneIdx >= 0)
	{
		mtxWeaponGrip = pWeaponSkeleton->GetSkeletonData().GetDefaultTransform(gripBoneIdx);
	}

	// Need updated point helper matrix in object space (transform by pending hand matrix since point helper is a child of the hand)
	Mat34V mtxPH = skeleton.GetLocalMtx(armGoal.m_PointHelperBoneIdx);
	Transform(mtxPH, destMtx, mtxPH);

	// Pre-recoil weapon matrix in object space (attaching weapon to incoming hand)
	InvertTransformFull(mtxWeapon, mtxWeaponGrip);
	Transform(mtxWeapon, mtxPH, mtxWeapon);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		Mat34V mtxGlobal(mtxWeapon);
		Transform(mtxGlobal, *skeleton.GetParentMtx(), mtxGlobal); 
		ms_debugHelper.Axis(RCC_MATRIX34(mtxGlobal), 0.15f);
	}
#endif //__IK_DEV

	// Transform recoil offset into weapon space
	Vec3V vRecoil(Transform3x3(mtxWeapon, mtxRecoil.GetCol3()));

	// Transform recoil rotation into weapon space
	const CBodyRecoilIkSolver* pBodyRecoilSolver = static_cast<const CBodyRecoilIkSolver*>(m_pPed->GetIkManager().GetSolver(IKManagerSolverTypes::ikSolverTypeBodyRecoil));
	if (pBodyRecoilSolver)
	{
		const CBodyRecoilIkSolver::RecoilSituation& situation = pBodyRecoilSolver->GetSituation();

		// Use damped translation value from recoil solver
		vRecoil = NormalizeSafe(vRecoil, Vec3V(V_ZERO));
		vRecoil = Scale(vRecoil, situation.vDampedValues.GetX());

		// Use damped rotation value from recoil solver
		ScalarV vRotation(situation.fTargetRotation);
		vRotation = InvertSafe(vRotation, ScalarV(V_ZERO));
		vRotation = Scale(vRotation, situation.vDampedValues.GetY());
		vAngle = Scale(vAngle, vRotation);
	}

	vAxis = Transform3x3(mtxWeapon, vAxis);
	qRecoil = QuatVFromAxisAngle(vAxis, vAngle);

	// Apply recoil to updated weapon matrix
	QuatV qWeapon(QuatVFromMat34V(mtxWeapon));
	qWeapon = Multiply(qRecoil, qWeapon);
	Mat34VFromQuatV(mtxWeapon, qWeapon, Add(mtxWeapon.GetCol3(), vRecoil));

#if __IK_DEV
	if (CaptureDebugRender())
	{
		Mat34V mtxGlobal(mtxWeapon);
		Transform(mtxGlobal, *skeleton.GetParentMtx(), mtxGlobal); 

		const ScalarV vOffset(0.15f);
		Vec3V vStart(mtxGlobal.GetCol3());
		Vec3V vEnd(AddScaled(vStart, mtxGlobal.GetCol0(), vOffset));
		ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_red4);
		vEnd = AddScaled(vStart, mtxGlobal.GetCol1(), vOffset);
		ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_green4);
		vEnd = AddScaled(vStart, mtxGlobal.GetCol2(), vOffset);
		ms_debugHelper.Line(RCC_VECTOR3(vStart), RCC_VECTOR3(vEnd), Color_blue4);
	}
#endif //__IK_DEV

	// Calculate updated hand matrix
	InvertTransformFull(destMtx, skeleton.GetLocalMtx(armGoal.m_PointHelperBoneIdx));
	Transform(destMtx, mtxWeaponGrip, destMtx);
	Transform(destMtx, mtxWeapon, destMtx);

#if __IK_DEV
	if (CaptureDebugRender())
	{
		Vec3V vEulers(Mat34VToEulersXYZ(mtxRecoil));

		char debugText[64];
		sprintf(debugText, "Recoil Disp XYZ = %8.4f, %8.4f, %8.4f\n", mtxRecoil.GetCol3().GetXf(), mtxRecoil.GetCol3().GetYf(), mtxRecoil.GetCol3().GetZf());
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
		sprintf(debugText, "Recoil Rot  XYZ = %8.4f, %8.4f, %8.4f\n", vEulers.GetXf() * RtoD, vEulers.GetYf() * RtoD, vEulers.GetZf() * RtoD);
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
	}
#endif //__IK_DEV
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::StoreHelperTransform(const eArms arm, const bool bActive, const rage::crSkeleton& skeleton, const crIKSolverArms::ArmGoal& armGoal, CArmIkSolver::Target& target)
{
	if (target.m_controlFlags & AIK_USE_ORIENTATION)
	{
		if (target.m_controlFlags & (AIK_TARGET_WRT_POINTHELPER | AIK_TARGET_WRT_IKHELPER | AIK_TARGET_WRT_CONSTRAINTHELPER))
		{
			// Store helper transform while solver is active or AIK_HOLD_HELPER option is not set
			if (bActive || ((target.m_controlFlags & AIK_HOLD_HELPER) == 0))
			{
				if (target.m_controlFlags & (AIK_TARGET_WRT_POINTHELPER | AIK_TARGET_WRT_IKHELPER))
				{
					u32 boneIdx = (u32)((target.m_controlFlags & AIK_TARGET_WRT_POINTHELPER) ? armGoal.m_PointHelperBoneIdx : armGoal.m_HandIkHelperBoneIdx);
					TransformVFromMat34V(target.m_Helper, skeleton.GetLocalMtx(boneIdx));
				}
				else
				{
					Mat34V mtxHelper(V_IDENTITY);

					Vec3V vCHTranslation;
					QuatV qCHRotation;
					float fCHWeight;

					const bool bValid = m_pPed->GetConstraintHelperDOFs((arm == RIGHT_ARM), fCHWeight, vCHTranslation, qCHRotation);
					if (bValid)
					{
						Mat34VFromQuatV(mtxHelper, qCHRotation, vCHTranslation);
					}

					TransformVFromMat34V(target.m_Helper, mtxHelper);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CPhysical* CArmIkSolver::GetAttachParent(Mat34V& attachParentMtx)
{
	CPhysical* pAttachParent = NULL;

	fwAttachmentEntityExtension* pExtension = m_pPed->GetAttachmentExtension();
	if(pExtension)
	{
		CPhysical* pAttachParentForced = static_cast<CPhysical*>(pExtension->GetAttachParentForced());

		if(pAttachParentForced && pAttachParentForced->GetIsTypeVehicle() && pExtension->GetIsAttached())
		{
			Mat34V offsetMtx;
			Mat34VFromQuatV(offsetMtx, QUATERNION_TO_QUATV(pExtension->GetAttachQuat()), VECTOR3_TO_VEC3V(pExtension->GetAttachOffset()));

			if((pExtension->GetOtherAttachBone() > -1) && pAttachParentForced->GetSkeleton())
			{
				pAttachParentForced->GetSkeleton()->GetGlobalMtx(pExtension->GetOtherAttachBone(), attachParentMtx);
			}
			else
			{
				attachParentMtx = pAttachParentForced->GetMatrix();
			}

			Transform(attachParentMtx, attachParentMtx, offsetMtx);
			pAttachParent = pAttachParentForced;
		}
	}

	return pAttachParent;
}

//////////////////////////////////////////////////////////////////////////

#if __IK_DEV

bool CArmIkSolver::CaptureDebugRender()
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

void CArmIkSolver::DebugRender()
{
	// Called every frame, even when paused, even when offscreen

	// don't do this if the game is paused or the ped is offscreen
	if(CaptureDebugRender() && !fwTimer::IsGamePaused() && m_pPed->GetIsVisibleInSomeViewportThisFrame())
	{
		char debugText[100];
		sprintf(debugText, "%s\n", "ArmIK::DebugRender");
		ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);

		for(int i=0; i < NUM_ARMS; ++i)
		{
			ArmGoal& arm = m_Arms[i];

			static dev_float bSphereSize = 0.01f;
			//static dev_float bAxisSize = 0.02f;

			Mat34V globalHandMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(arm.m_HandBoneIdx, globalHandMtx);
			Vec3V globalHandPos(globalHandMtx.d());

			Mat34V globalForeArmMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(arm.m_ElbowBoneIdx, globalForeArmMtx);
			Vec3V globalForeArmPos(globalForeArmMtx.d());

			Mat34V globalUpperArmMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(arm.m_ShoulderBoneIdx, globalUpperArmMtx);
			Vec3V globalUpperArmPos(globalUpperArmMtx.d());

			Mat34V globalPointHelperMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(arm.m_PointHelperBoneIdx, globalPointHelperMtx);
			Vec3V globalPointHelperPos(globalPointHelperMtx.d());

			Mat34V globalIkHelperMtx;
			m_pPed->GetSkeleton()->GetGlobalMtx(arm.m_HandIkHelperBoneIdx, globalIkHelperMtx);
			Vec3V globalIkHelperPos(globalIkHelperMtx.d());

			grcDebugDraw::Sphere(RC_VECTOR3(globalHandPos), bSphereSize, Color_red);
			grcDebugDraw::Sphere(RC_VECTOR3(globalForeArmPos), bSphereSize, Color_red);
			grcDebugDraw::Sphere(RC_VECTOR3(globalUpperArmPos), bSphereSize, Color_red);
			grcDebugDraw::Sphere(RC_VECTOR3(globalPointHelperPos), bSphereSize, Color_red);
			grcDebugDraw::Sphere(RC_VECTOR3(globalIkHelperPos), bSphereSize, Color_red);
			grcDebugDraw::Line(RC_VECTOR3(globalHandPos), RC_VECTOR3(globalForeArmPos), Color_red, Color_red);
			grcDebugDraw::Line(RC_VECTOR3(globalForeArmPos), RC_VECTOR3(globalUpperArmPos), Color_red, Color_red);

			/*
			Vector3 direction = RC_VECTOR3(globalHandPos) - RC_VECTOR3(globalForeArmPos);
			float foreArmLength = direction.Mag();
			direction = RC_VECTOR3(globalForeArmPos) - RC_VECTOR3(globalUpperArmPos);
			float upperArmLength = direction.Mag();
			direction = RC_VECTOR3(globalHandPos) - RC_VECTOR3(globalUpperArmPos);
			float diff = direction.Mag();
			char debugText[100];
			sprintf(debugText, "After - forearm length = %6.4f, upperarm length = %6.4f, diff = %6.4f,\n", foreArmLength, upperArmLength, diff);
			ms_debugHelper.Text2d(ms_iNoOfTexts++, Color_white, debugText);
			*/
		}
	}

	if (CaptureDebugRender())
	{
		ms_debugHelper.Render();
	}
}

//////////////////////////////////////////////////////////////////////////

void CArmIkSolver::AddWidgets(bkBank *pBank)
{
	pBank->PushGroup("CArmIkSolver", false, NULL);
	pBank->AddToggle("Disable", &CBaseIkManager::ms_DisableSolvers[IKManagerSolverTypes::ikSolverTypeArm], NullCB, "");
	pBank->AddToggle("Debug", &ms_bRenderDebug, NullCB, "");
	pBank->AddSlider("Door Barge Move Blend Threshold",&ms_fDoorBargeMoveBlendThreshold, 0.0f, 5.0f, 0.01f);
	pBank->PopGroup();
}

//////////////////////////////////////////////////////////////////////////

#endif //__IK_DEV
