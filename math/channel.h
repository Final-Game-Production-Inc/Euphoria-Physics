// 
// math/channel.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef MATH_CHANNEL_H 
#define MATH_CHANNEL_H 

////////////////////////////////////////////////
// Important Note!
//
// Math and Vectormath shouldn't depend on each other tightly
// so vectormath defines its own copies of these macros if necessary
// The #ifndef below prevents errors if both math\channel.h and vectormath\channel.h
// are #included in a file.

#ifndef VECTORMATH_CHANNEL_H 

#include "diag/channel.h"

// DOM-IGNORE-BEGIN
RAGE_DECLARE_CHANNEL(Math)

#define mthAssert(cond)						RAGE_ASSERT(Math,cond)
#define mthAssertf(cond,fmt,...)			RAGE_ASSERTF(Math,cond,fmt,##__VA_ARGS__)
#define mthVerifyf(cond,fmt,...)			RAGE_VERIFYF(Math,cond,fmt,##__VA_ARGS__)
#define mthErrorf(fmt,...)					RAGE_ERRORF(Math,fmt,##__VA_ARGS__)
#define mthWarningf(fmt,...)				RAGE_WARNINGF(Math,fmt,##__VA_ARGS__)
#define mthDisplayf(fmt,...)				RAGE_DISPLAYF(Math,fmt,##__VA_ARGS__)
#define mthDebugf1(fmt,...)					RAGE_DEBUGF1(Math,fmt,##__VA_ARGS__)
#define mthDebugf2(fmt,...)					RAGE_DEBUGF2(Math,fmt,##__VA_ARGS__)
#define mthDebugf3(fmt,...)					RAGE_DEBUGF3(Math,fmt,##__VA_ARGS__)
#define mthLogf(severity,fmt,...)			RAGE_LOGF(Math,severity,fmt,##__VA_ARGS__)
#define mthCondLogf(cond,severity,fmt,...)	RAGE_CONDLOGF(cond,Math,severity,fmt,##__VA_ARGS__)
// DOM-IGNORE-END

#endif // VECTORMATH_CHANNEL_H

#endif // MATH_CHANNEL_H 
