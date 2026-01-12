// 
// vector/allbitsf.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "allbitsf.h"

extern "C" {
float noBitsF; // = 0;
float allBitsF; // = ~0U;
float finf; // = 0x7f800000;
float fnan; // = 0x7fc00000;
}


#if __WIN32
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#endif

static struct InitFloatSpecials_t {
	union FloatToInt
	{
		float f;
		rage::u32 i;
	};

	InitFloatSpecials_t() { 
		FloatToInt f2i;
		f2i.i = 0x0;		noBitsF = f2i.f;
		f2i.i = ~0U;		allBitsF = f2i.f;
		f2i.i = 0x7f800000; finf = f2i.f;
		f2i.i = 0x7fc00000; fnan = f2i.f;
	}
} InitFloatSpecialsInstance PPU_ONLY(__attribute__((init_priority(101))));
