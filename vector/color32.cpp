// 
// vector/color32.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "color32.h"

#include "vector3.h"
#include "vector4.h"

using namespace rage;

#if __PSP2

// UNTESTED, implementation is crap
Vec4V_Out rage::MergeXYByte(Vec4V_In a,Vec4V_In b)
{
	// Should be using vzip[q]_u8 here...
	char *aa = (char*) &a;
	char *bb = (char*) &b;
	char mix[16];
	for (int i=0; i<8; i++) { mix[i*2] = aa[i]; mix[i*2+1] = bb[i]; }
	return *(Vec4V*)mix;
}

// UNTESTED, implementation is crap
Vec4V_Out rage::MergeXYShort(Vec4V_In a,Vec4V_In b)
{
	// Should be using vzip[q]_u16 here...
	short *aa = (short*) &a;
	short *bb = (short*) &b;
	short mix[8];
	for (int i=0; i<4; i++) { mix[i*2] = aa[i]; mix[i*2+1] = bb[i]; }
	return *(Vec4V*)mix;
}

#endif // __PSP2

IMPLEMENT_PLACE(Color32);
