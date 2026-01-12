//
// math/random.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "random.h"

#include "amath.h"

#include "system/param.h"

#if !__SPU
PARAM(randomseed, "Used a fixed seed value for all random number streams");
#endif

namespace rage {
#if !__SPU
	mthRandom g_ReplayRand(1, true);
	mthRandom g_DrawRand;
#endif
} // namespace rage

using namespace rage;

namespace rage
{
#if __DEBUGLOG
mthRandom::mthRandom(int seed, bool debug)
	: m_ReplayDebug(debug)
{
	Reset(seed);
}
#else
mthRandom::mthRandom(int seed, bool /*debug*/)
{
	Reset(seed);
}
#endif // __DEBUGLOG

void mthRandom::SetFullSeed(u64 seed)
{ 
#if !__SPU
	if (PARAM_randomseed.Get())
	{
		Reset(0);
	}
	else
#endif
	{
		m_Seed0 = u32(seed >> 32);
		m_Seed1 = u32(seed);
	}
}

	// Gaussian distributed random number generator
	// Returns random gaussian random number clustered around the mean with a given variance.
	// Adapted from Numerical Recipes in C
	float mthRandom::GetGaussian(float mean, float variance)
	{
		// generate two gaussian random numbers at a time
		// and store one of them for the next call
		
		if (m_StoredGaussianExists)
		{
			m_StoredGaussianExists = false;

			return mean + variance * m_StoredGaussian;
		}
		else
		{
			float x,y;
			float radiusSquared;

			// pick a point inside a unit circle (but excluding the origin)
			do
			{
				x = 2.f * GetFloat() - 1.f;
				y = 2.f * GetFloat() - 1.f;
				radiusSquared = x*x + y*y;
			}
			while (radiusSquared >= 1.f || radiusSquared == 0.f);

			// do a Box-Muller transformation on radiusSquared
			float transform = sqrtf(-2.f * logf(radiusSquared) / radiusSquared);

			// store the gaussian random number from the x coordinate for the next time we're called
			m_StoredGaussian = x * transform;
			m_StoredGaussianExists = true;

			// return the gaussian random number from the y coordinate		
			return mean + variance * y * transform;
		}
	}



	mthRandomMWC::cmwc_state mthRandomMWC::ms_cmwc;
	u32		mthRandomMWC::ms_initialSeed = 0;

	// Make 32 bit random number (some systems use 16 bit RAND_MAX [Visual C 2012 uses 15 bits!])
	u32 rand32(void)
	{
		u32 result = rand();
		return result << 16 | rand();
	}

	// Init the state with seed
	void mthRandomMWC::initCMWC(struct cmwc_state *state, u32 seed)
	{
		srand(seed);        
		for (int i = 0; i < CMWC_CYCLE; i++)
			state->Q[i] = rand32();
		do
		state->c = rand32();
		while (state->c >= CMWC_C_MAX);
		state->i = CMWC_CYCLE - 1;
	}

	// CMWC engine
	u32 mthRandomMWC::randCMWC(struct cmwc_state *state)  //EDITED parameter *state was missing
	{
		u64 const a = 18782; // as Marsaglia recommends
		u32 const m = 0xfffffffe; // as Marsaglia recommends
		u64 t;
		u32 x;

		state->i = (state->i + 1) & (CMWC_CYCLE - 1);
		t = a * state->Q[state->i] + state->c;
		/* Let c = t / 0xfffffff, x = t mod 0xffffffff */
		state->c = t >> 32;
		x = (u32)(t + state->c);
		if (x < state->c) {
			x++;
			state->c++;
		}
		return state->Q[state->i] = m - x;
	}

	mthRandomMWC::mthRandomMWC(u32 seed, bool /*debug*/)
	{
		Reset(seed);
	}

	void mthRandomMWC::Reset(u32 seed)
	{
		ms_initialSeed = seed;
		initCMWC(&ms_cmwc, seed);
	}

	s32 mthRandomMWC::GetInt(void)
	{
		u32 retVal = randCMWC(&ms_cmwc);

		// All 32 bits are random but it would break too much code
		// if the sign bit was left in.
		retVal = retVal & ( ~0u >> 1 );
		return(retVal);
	}

	u32 mthRandomMWC::GetRawInt(void)
	{
		u32 retVal = randCMWC(&ms_cmwc);
		return(retVal);
	}

} // namespace rage
