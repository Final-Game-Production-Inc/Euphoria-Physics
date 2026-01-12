// 
// fragmentnm/nm_channel.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef INC_NM_CHANNEL_H 
#define INC_NM_CHANNEL_H 

#include "diag/channel.h"

RAGE_DECLARE_CHANNEL(nm)
#ifdef NM_RASH
RAGE_DEFINE_CHANNEL(nm)
#endif
#define nmAssert(cond)							RAGE_ASSERT(nm,cond)
#define nmAssertf(cond,fmt,...)					RAGE_ASSERTF(nm,cond,fmt,##__VA_ARGS__)
#define nmVerifyf(cond,fmt,...)					RAGE_VERIFYF(nm,cond,fmt,##__VA_ARGS__)
#define nmFatalf(fmt,...)						RAGE_FATALF(nm,fmt,##__VA_ARGS__)
#define nmErrorf(fmt,...)						RAGE_ERRORF(nm,fmt,##__VA_ARGS__)
#define nmWarningf(fmt,...)						RAGE_WARNINGF(nm,fmt,##__VA_ARGS__)
#define nmDisplayf(fmt,...)						RAGE_DISPLAYF(nm,fmt,##__VA_ARGS__)
#define nmDebugf1(fmt,...)						RAGE_DEBUGF1(nm,fmt,##__VA_ARGS__)
#define nmDebugf2(fmt,...)						RAGE_DEBUGF2(nm,fmt,##__VA_ARGS__)
#define nmDebugf3(fmt,...)						RAGE_DEBUGF3(nm,fmt,##__VA_ARGS__)
#define nmLogf(severity,fmt,...)				RAGE_LOGF(nm,severity,fmt,##__VA_ARGS__)
#define nmCondLogf(cond,severity,fmt,...)		RAGE_CONDLOGF(cond,nm,severity,fmt,##__VA_ARGS__)

#define ENABLE_SEMAPHORE_CHECK (0)
#if ENABLE_SEMAPHORE_CHECK
#define nmFastAssert(cond)					FastAssert(cond)
#else // ENABLE_SEMAPHORE_CHECK
#define nmFastAssert(cond)					RAGE_ASSERT(nm,cond)
#endif // ENABLE_SEMAPHORE_CHECK

#endif // INC_NM_CHANNEL_H 

