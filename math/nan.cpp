//
// math/nan.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "nan.h"
#include "channel.h"

namespace rage {


#if __WIN32PC && !defined(_M_X64)
void EnableNanSignal(bool enable) {
	u16 oldfpstate, fpstate;
#if __DEV
	const u16 maskedExceptions = 0x305;	// turn off 53 bit precision, zero divide, and invalid operation
#else
	const u16 maskedExceptions = 0x300;	// if not in dev mode, leave everything masked so video driver doesn't crash.
#endif
	// leaving overflow on seems to cause too many problems in legitimate code

	// Remember previous state so we can warn if it's getting hosed.
	static bool lastEnable;

	_asm fnstcw [oldfpstate];
	_asm fninit						// discard any pending errors

	if (enable && lastEnable && (oldfpstate & maskedExceptions))		//lint !e727 !e530 not explicitly initialized
		mthWarningf("EnableNanSignal: Had been turned off by someone else?");
	if (enable)
		fpstate = (u16) (oldfpstate & ~maskedExceptions);	// unmask signal
	else
		fpstate = (u16) (oldfpstate | 0x205);				// mask signal, set precision to 53 bits

	// mthDisplayf("EnableNanSignal: was %x, now %x",oldfpstate,fpstate);
	_asm fldcw [fpstate]

	lastEnable = enable;
}					//lint !e550 fpstate not accessed
#else
void EnableNanSignal(bool) { }
#endif

__THREAD int g_DisableInitNan = 1;

} //namespace rage
