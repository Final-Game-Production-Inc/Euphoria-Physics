// 
// system/spu_random.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef MATH_SPURANDOM_H
#define MATH_SPURANDOM_H

#if __SPU
#include <stdlib.h> // init_MT, rand_real1_MT_f4
#include <spu_mfcio.h> // spu_read_decrementer()

namespace rage
{

class mthSpuRandom
{
public:
	static void Init()
	{
		init_MT(spu_read_decrementer());
	}

	static void Init(u32 seed)
	{
		init_MT(seed);
	}

	static vec_float4 GetRandomNumberInRange_f4(vec_float4 a, vec_float4 b)
	{
		vec_float4 r0	= rand_real1_MT_f4();
		vec_float4 bma	= spu_sub(b, a);
		vec_float4 r1	= spu_madd(r0, bma, a);	// r1 = r0 * (b - a) + a
		return r1;
	}

	static vec_float4 GetRandomNumberInRange_f4() // returns vector of [0..1] values
	{
		return rand_real1_MT_f4();
	}
};

} // namespace rage

#endif // __SPU
#endif // MATH_SPURANDOM_H
