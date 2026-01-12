//
// math/testnan.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "nan.h"

#include <stdio.h>

using namespace rage;

int main()
{

#if __INIT_NAN && __WIN32PC
	float a,b,c;
	MakeNan(a);
	MakeNan(b);

	unsigned short oldfpstate, fpstate;
	_asm fnstcw [oldfpstate]
	fpstate = (oldfpstate & ~0x3F); // unmask all exceptions
	_asm fldcw [fpstate]

	c = a + b;
	printf("%f = %f + %f\n", c, a, b);
#endif

	return 0;
}
