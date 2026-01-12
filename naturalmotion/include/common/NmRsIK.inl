/*
 * Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
 *
 * Not to be copied, adapted, modified, used, distributed, sold,
 * licensed or commercially exploited in any manner without the
 * written consent of NaturalMotion. 
 *
 * All non public elements of this software are the confidential
 * information of NaturalMotion and may not be disclosed to any
 * person nor used for any purpose not expressly approved by
 * NaturalMotion in writing.
 *
 */

NM_RS_PARAMETER(NmRsIK, rage::Vector3, Target, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f))
NM_RS_PARAMETER(NmRsIK, bool, TwistIsFixed, false, true, false)
NM_RS_PARAMETER(NmRsIK, bool, CanDoIKGreedy, false, true, false)
NM_RS_PARAMETER(NmRsIK, rage::Vector3, Velocity, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f))
NM_RS_PARAMETER(NmRsIK, float, Twist, -FLT_MAX, FLT_MAX, 0.0f)
NM_RS_PARAMETER(NmRsIK, float, DragReduction, -FLT_MAX, FLT_MAX, 1.0f)
NM_RS_PARAMETER(NmRsIK, float, MaxReachDistance, -FLT_MAX, FLT_MAX, -1.0f)
NM_RS_PARAMETER(NmRsIK, float, Blend, -FLT_MAX, FLT_MAX, 1.0f)
NM_RS_PARAMETER(NmRsIK, bool, UseAdvancedIk, false, true, false)
NM_RS_PARAMETER(NmRsIK, float, AdvancedStaightness, -FLT_MAX, FLT_MAX, 0.0f)
NM_RS_PARAMETER(NmRsIK, float, AdvancedMaxSpeed, -FLT_MAX, FLT_MAX, 0.0f)
NM_RS_PARAMETER(NmRsIK, rage::Vector3, PoleVector, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 1.0f))
NM_RS_PARAMETER(NmRsIK, rage::Vector3, EffectorOffset, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f))
NM_RS_PARAMETER(NmRsIK, int, MatchClavicle, 0, 3, 0)
NM_RS_PARAMETER(NmRsIK, float, MinimumElbowAngle, -FLT_MAX, FLT_MAX, 0.0f)
NM_RS_PARAMETER(NmRsIK, float, MaximumElbowAngle, -FLT_MAX, FLT_MAX, 3.1f)
NM_RS_PARAMETER(NmRsIK, rage::Vector3, WristTarget, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f))
NM_RS_PARAMETER(NmRsIK, rage::Vector3, WristNormal, rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f), rage::Vector3(0.0f, 0.0f, 0.0f))
NM_RS_PARAMETER(NmRsIK, float, WristTwistLimit, -FLT_MAX, FLT_MAX, 0.0f)
NM_RS_PARAMETER(NmRsIK, bool, WristUseActualAngles, false, true, false)
