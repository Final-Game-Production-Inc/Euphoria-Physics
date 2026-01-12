//
// math/nan.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_NAN_H
#define MATH_NAN_H

#include "system/tls.h"

namespace rage {

#if ((__DEV && __WIN32PC) || __RESOURCECOMPILER || (!__OPTIMIZED)) && !__SPU
#define __INIT_NAN 1
#else
#define __INIT_NAN 0
#endif

// PURPOSE: Make an invalid (not a number) floating point value.
// PARAMS:
//	f -	reference to the floating point value
#if __PPU
	inline void MakeNan(float &f) { 
		union { float f; unsigned u; } x;
		x.u = 0x7F800001;
		f = x.f;
	}
#else
	inline void MakeNan(float &f) { (*(int*)&f) = 0x7F800001; } //lint !e740
#endif

/// Use FPIsFinite instead.
/// inline bool IsNan(const float f) { return !FPIsFinite(f); }

// PURPOSE: Set up the conditions under which invalid floating-point calculations cause signals.
// PARAMS:
//	enable -	a boolean to tell whether to enable or disable the signals,
//				which is also used as a bit field to define what to enable (see the notes)
// NOTES: The input boolean can be used as a bit field to define how signals are enabled:
//	- bit 0: Invalid operation
//	- bit 1: Denormalized operation
//	- bit 2: Zero divide
//	- bit 3: Overflow
//	- bit 4: Underflow
//	- bit 5: Precision
//	- bits 8-9: Precision control (00=24, 01=reserved, 10=53, 11=64)
void EnableNanSignal(bool);

// stops Vectors from initing to NAN if nonzero
extern __THREAD int g_DisableInitNan;

} // namespace rage

#endif
