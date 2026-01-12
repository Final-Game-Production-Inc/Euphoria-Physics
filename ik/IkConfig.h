// 
// ik/IkConfig.h
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef IKCONFIG_H
#define IKCONFIG_H

#include "diag/channel.h"

RAGE_DECLARE_CHANNEL(ik)

#define ikAssert(cond)						RAGE_ASSERT(ik,cond)
#define ikAssertf(cond,fmt,...)				RAGE_ASSERTF(ik,cond,fmt,##__VA_ARGS__)
#define ikVerifyf(cond,fmt,...)				RAGE_VERIFYF(ik,cond,fmt,##__VA_ARGS__)
#define ikErrorf(fmt,...)					RAGE_ERRORF(ik,fmt,##__VA_ARGS__)
#define ikWarningf(fmt,...)					RAGE_WARNINGF(ik,fmt,##__VA_ARGS__)
#define ikDisplayf(fmt,...)					RAGE_DISPLAYF(ik,fmt,##__VA_ARGS__)
#define ikDebugf1(fmt,...)					RAGE_DEBUGF1(ik,fmt,##__VA_ARGS__)
#define ikDebugf2(fmt,...)					RAGE_DEBUGF2(ik,fmt,##__VA_ARGS__)
#define ikDebugf3(fmt,...)					RAGE_DEBUGF3(ik,fmt,##__VA_ARGS__)
#define ikLogf(severity,fmt,...)			RAGE_LOGF(ik,severity,fmt,##__VA_ARGS__)
#define ikCondLogf(cond,severity,fmt,...)	RAGE_CONDLOGF(cond,ik,severity,fmt,##__VA_ARGS__)

#define __IK_DEV __DEV

#if __IK_DEV
#define IK_DEV_ONLY(...)	__VA_ARGS__
#else
#define IK_DEV_ONLY(...)
#endif

#if __IK_DEV
#define IK_DEV_SET_LOOK_AT_RETURN_CODE(eLookIkReturnCode,fmt,...) if(m_pPed) { m_pPed->GetIkManager().SetLookIkReturnCode(eLookIkReturnCode); } \
	ikDebugf3("%u %p %s %s " fmt, fwTimer::GetFrameCount(), m_pPed, m_pPed ? m_pPed->GetModelName() : "(unknown)", eLookIkReturnCodeToString(eLookIkReturnCode), ##__VA_ARGS__)
#else
#define IK_DEV_SET_LOOK_AT_RETURN_CODE(eLookIkReturnCode,fmt,...)
#endif // __IK_DEV

#define IK_QUAD_LEG_SOLVER_ENABLE		0	// Also enable tunables in ik/Metadata.psc and type in ik/IkManagerSolverTypes.psc
#define IK_QUAD_LEG_SOLVER_USE_POOL		0

#if IK_QUAD_LEG_SOLVER_ENABLE
#define IK_QUAD_LEG_SOLVER_ONLY(...)	__VA_ARGS__
#else
#define IK_QUAD_LEG_SOLVER_ONLY(...)
#endif // IK_QUAD_LEG_SOLVER_ENABLE

#define IK_QUAD_REACT_SOLVER_ENABLE		0	// Also enable type in ik/IkManagerSolverTypes.psc
#define IK_QUAD_REACT_SOLVER_USE_POOL	0

#if IK_QUAD_REACT_SOLVER_ENABLE
#define IK_QUAD_REACT_SOLVER_ONLY(...)	__VA_ARGS__
#else
#define IK_QUAD_REACT_SOLVER_ONLY(...)
#endif // IK_QUAD_REACT_SOLVER_ENABLE

#endif // IKCONFIG_H
