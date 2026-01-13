//
// system/spurscheck
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_SPURSCHECK_H
#define SYSTEM_SPURSCHECK_H

#if __PS3

#if __PPU
#	include <sys/timer.h>
#	include "stack.h"
#	define SPURS_CHECK(x)                                                          \
		do {                                                                       \
			const int spurs_check_result = (x);                                    \
			if (Unlikely(spurs_check_result != CELL_OK)) {                         \
				do {                                                               \
					/* Add a delay so that the spurs fault handler              */ \
					/* has a chance to run                                      */ \
					sys_timer_usleep(1000 * 1000);                                 \
				} while(0                                                          \
					NOTFINAL_ONLY(|| rage::sysStack::HasExceptionBeenThrown()));   \
				Quitf("%s failed, code %x", #x, spurs_check_result);               \
			}                                                                      \
		} while(0)

#elif __SPU
#	define SPURS_CHECK(x)                                                          \
		do {                                                                       \
			const int spurs_check_result = (x);                                    \
			if (Unlikely(spurs_check_result != CELL_OK)) {                         \
				Quitf("%s failed, code %x", #x, spurs_check_result);               \
			}                                                                      \
		} while(0)
#endif

#endif // __PS3

#endif // SYSTEM_SPURSCHECK_H
