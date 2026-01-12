//
// math/random.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_RANDOM_H
#define MATH_RANDOM_H

#include "diag/debuglog.h"
#include "system/param.h"
#include "vectormath/classes.h"
#if __SPU
#include "intrinsics.h"
#endif

XPARAM(randomseed);

namespace rage {


#if __WIN32
#pragma warning(disable: 4514)
#endif


//=============================================================================
// class mthRandom
/* PURPOSE
    mthRandom makes it simple to maintain multiple random number streams without
    the enable/disable seed function calls.  It also helps to extend to an arbitrary
    number of random number streams (so that a very high level system like AI can be
    isolated from all other systems -- then the lower-level systems can be tested
    independently).

    The system supplies two streams for you but you're free to add more as long 
    as you reset the ones that are replay-sensitive properly.

    A better RNG is here: http://burtleburtle.net/bob/rand/isaacafa.html
    but it has a lot more state and doesn't accept a simple int as a seed.

    More discussion: http://www.mathcom.com/corpdir/techinfo.mdir/scifaq/q210.html

    Current implementation is based on MWCG from here:
    http://cliodhna.cop.uop.edu/~hetrick/na_faq.html

    <FLAG Component>
*/
class mthRandom
{
public:
    // PURPOSE: Initializes the new random number generator
    // PARAMS:
    //   seed   - the new seed (optional)
    mthRandom(int seed = 1, bool debug = false);
    
    // PURPOSE: Reseeds the random number generator
    // PARAMS:
    //   seed   - the new seed (optional)
    //   debug  - turn on replay debugging (optional)
    void Reset(int seed = 1);

    // PURPOSE: Returns the current seed value.
    // RETURNS: the current seed value
    inline int GetSeed();

	// PURPOSE: Returns the current seed value.
	// RETURNS: the current seed value
	inline u64 GetFullSeed();

	// PURPOSE: Sets the current seed value.
	// PARAMS:
	//   seed   - the new seed
	void SetFullSeed(u64 seed);

	// PURPOSE: Return a random boolean, with equal probability for true or false.
	// RETURN: a random boolean
	bool GetBool();

    // PURPOSE: Returns a random Integer, 0 <= return < 2**31.
    // RETURNS: a random integer
    // NOTES:
    //   * See http://cliodhna.cop.uop.edu/~hetrick/na_faq.html (MWCG) for details.
    inline int GetInt();

    // PURPOSE: Returns a random float, 0.0f <= return < 1.0f
    // RETURNS: a random float
    float GetFloat();

	// PURPOSE: Returns N random floats (vectorized versions), 0.0f <= return < 1.0f
	ScalarV_Out GetFloatV();
	Vec2V_Out Get2FloatsV();
	Vec3V_Out Get3FloatsV();
	Vec4V_Out Get4FloatsV();

    // PURPOSE: Returns a uniformly distributed random float number between -v/2 and v;
    // PARAMS:
    //   v      - float, the size of the range the float will vary
    // RETURNS: -v/2 <= result  < v/2
    inline float GetVaried(float v);

    // PURPOSE: Returns a uniformly distributed random float between m and M
    // PARAMS:
    //   minValue   - float, minimum of range
    //   maxValue   - float, maximum of range (exclusive)
    // RETURNS: minValue <= result < maxValue
    inline float GetRanged(float minValue, float maxValue);
	inline ScalarV_Out GetRangedV(ScalarV_In minValue, ScalarV_In maxValue);
	inline Vec2V_Out GetRangedV(Vec2V_In minValue, Vec2V_In maxValue);
	inline Vec3V_Out GetRangedV(Vec3V_In minValue, Vec3V_In maxValue);

    // PURPOSE: Returns a uniformly distributed random int between m and M
    // PARAMS
    //   minValue :   int, the minimum of the range
    //   maxValue :   int, the maximum of the range (inclusive)
    // RETURNS: minValue <= result <= maxValue
    inline int GetRanged(int minValue,int maxValue);

	// PURPOSE: Return a uniformly distributed random int between m and M when M - m + 1 (the size of the range of output values) is a power of two.
	// PARAMS
	//   minValue :   int, the minimum of the range
	//   maxValue :   int, the maximum of the range (inclusive)
	// RETURNS: minValue <= result <= maxValue
	// NOTES: This will return the exact same values as GetRanged(int, int) but should be somewhat faster because it avoids a modulus.
	inline int GetRangedIntPowerOfTwoRange(int minValue,int maxValue);

    // PURPOSE: Returns a uniformly distributed random int between m and M guaranteed not equal to p 
    // PARAMS
    //   p :   int, the value you don't want to duplicate (often the previous value)
    //   minValue :   int, the minimum of the range
    //   maxValue :   int, the maximum of the range (inclusive)
    // RETURNS: minValue <= result <= maxValue, result != p
    inline int GetRangedDifferent(int p, int minValue, int maxValue);
    
    // PURPOSE: Returns a Gaussian random number with a given mean and variance.
    // PARAMS
    //   mean - float, average returned value will match this
    //   variance - float, about 2 out of 3 of the numbers with appear in the range
    //              [mean-variance,mean+variance]
    // RETURNS: 
    //   This function returns a random number clustered around the given mean.  About 2/3rds
    //   of the time the number will be within [mean-variance, mean+variance], distributed in
    //   a bell curve.
    float GetGaussian(float mean,float variance);

#if __DEBUGLOG
    inline void SetReplayDebug(const bool debug);
#endif // __DEBUGLOG

private:
    inline int GetIntCommon();

    u32 m_Seed0;
    u32 m_Seed1;

    bool m_StoredGaussianExists;

    float m_StoredGaussian;

#if __DEBUGLOG
    bool m_ReplayDebug;
#endif // __DEBUGLOG
};

// this is based on mthRandom so I can drop it in where I need it - but I've isolated it completely and based it on vanilla Marsaglia MWC implementation
class mthRandomMWC
{
private:
	// implementation taken from wikipedia page on multiply with carry algorithm
	// https://en.wikipedia.org/wiki/Multiply-with-carry_pseudorandom_number_generator

	// C99 Complementary Multiply With Carry generator
	// #include <stdint.h>
	// #include <stdio.h>
	// #include <stdlib.h>
	// #include <time.h>

	// CMWC working parts
#define CMWC_CYCLE 4096 // as Marsaglia recommends
#define CMWC_C_MAX 809430660 // as Marsaglia recommends
	struct cmwc_state {
		u32 Q[CMWC_CYCLE];
		u32 c;	// must be limited with CMWC_C_MAX
		unsigned i;
	};

	static struct cmwc_state ms_cmwc;
	static u32		ms_initialSeed;

	void initCMWC(struct cmwc_state *state, u32 seed);
	u32 randCMWC(struct cmwc_state *state);

public:
	// PURPOSE: Initializes the new random number generator
	// PARAMS:
	//   seed   - the new seed (optional)
	mthRandomMWC(u32 seed = 1, bool debug = false);

	// PURPOSE: Reseeds the random number generator
	// PARAMS:
	//   seed   - the new seed (optional)
	//   debug  - turn on replay debugging (optional)
	void Reset(u32 seed = 1);

	// PURPOSE: Returns the current seed value.
	// RETURNS: the current seed value
	//inline int GetSeed();

	// PURPOSE: Returns a random Integer, 0 <= return < 2**31.
	// RETURNS: a random integer
	// NOTES:
	int GetInt();

	u32 GetRawInt();	// get the full random 32 bits

	// get the initial seeding value used to generate the state table
	int GetInitialSeed() { return(ms_initialSeed); }
};

//=============================================================================
// Globals

// PURPOSE
//   An instance of the mthRandom class that is globally available and to be used
//   for random numbers that affect gameplay (e.g. change the way replays would 
//   play out).
extern mthRandom g_ReplayRand;

// PURPOSE
//   An instance of the mthRandom class that is globally available and to be used
//   for random numbers that don't affect gameplay and my change with viewing angle
//   or other non-gameplay related factors (e.g. don't change the way replays
//   would play out).
extern mthRandom g_DrawRand;


//=============================================================================
// Implementations

// Moved to inline as plantsmgr calls this so much
inline void mthRandom::Reset(int seed)
{
#if __BANK
//Comment this out until x64 issues are resolved.
//#if __ASSERT
//	Assertf(seed == 1 || PARAM_randomseed.IsInitialized(),"mthRandom::Reset dynamic seed value used before PARAM_ system initialized, seed[%d]",seed);
//#endif
	PARAM_randomseed.Get(seed);
#endif
	const unsigned useed = ( unsigned ) seed;
	m_Seed0 = useed + !useed;   // Make sure zero doesn't happen
	m_Seed1 = ( ( useed << 16 ) | ( useed >> 16 ) ) ^ useed;
	m_StoredGaussianExists = false;
	m_StoredGaussian = 0.0f;
}

inline int mthRandom::GetSeed()
{ 
    return (int) m_Seed0; 
}

inline u64 mthRandom::GetFullSeed()
{ 
	return ((u64(m_Seed0) << 32) | m_Seed1);
}


inline float mthRandom::GetVaried(float v)
{
    return(v*(GetFloat()-0.5f));
}

inline float mthRandom::GetRanged(float m,float M)
{
    return(GetFloat()*(M-m)+m);
}

inline ScalarV_Out mthRandom::GetRangedV(ScalarV_In m, ScalarV_In M)
{
	return (GetFloatV() * (M-m) + m);
}

inline Vec2V_Out mthRandom::GetRangedV(Vec2V_In m, Vec2V_In M)
{
	return (Get2FloatsV() * (M-m) + m);
}

inline Vec3V_Out mthRandom::GetRangedV(Vec3V_In m, Vec3V_In M)
{
	return (Get3FloatsV() * (M-m) + m);
}

inline int mthRandom::GetRanged(int m,int M)
{
    return((GetInt()%(M-m+1))+m);
}

inline int mthRandom::GetRangedIntPowerOfTwoRange(int m,int M)
{
	// Ensure that the range is of a size that's a power of two - doing it the 'long' way to avoid #includ'ing anything new.
	FastAssert(((M - m + 1) & (M - m)) == 0);
	return((GetInt()&(M-m))+m);
}

inline int mthRandom::GetRangedDifferent(int p, int m,int M)
{
    FastAssert(p >= m && p <= M);
    p = p + 1 + GetInt()%(M-m);
    p = ((p - m) % (M-m+1)) + m;
    return p;
}

#if __DEBUGLOG
inline void mthRandom::SetReplayDebug(const bool debug)
{
    m_ReplayDebug = debug; 
}
#endif


inline int mthRandom::GetIntCommon()
{
    /*
    http://cliodhna.cop.uop.edu/~hetrick/na_faq.html

    [George Marsaglia]:
    Here is a simple version, one so simple and good I predict
    it will be the system generator for many future computers:
    x(n)=a*x(n-1)+carry mod 2^32
    With multiplier 'a' chosen from a large set of easily found
    integers, the period is a*2^31-1, around 2^60, and
    I have yet to find a test it will not pass!

    The 'carry' works like this, which shows how well this
    method will serve as a system RNG:
    Have seed x and c.  Form a*x+c in 64 bits.  All modern
    CPU's have instructions for this: sparc, intel, etc.
    Then the new x is the bottom 32 bits.
    the new carry is the top 32 bits.

    The period is the order of b=2^32 in the group of residues
    relatively prime to m=a*2^32-1.  One need only choose a's
    of some 30 bits for which m=a*2^32-1 is a safeprime:
    both m and (m-1)/2 are prime.  There are thousands of them.

    In general, for any choice of 'a', let m=a*2^32-1.  If both m
    and (m-1)/2 are prime then the period will be (m-1)/2.
    Even if 'a' is, say, your social security number, the
    period will most likely be on the order of 2^50 or more.
    (For mine, it is 2^54.8).

    For 32-bit generators, some possible values of 'a' are:
    1967773755 1517746329 1447497129 1655692410 1606218150
    2051013963 1075433238 1557985959 1781943330 1893513180
    */

#if __SPU
	u64 temp = _spuMulu32Byu32Outu64(1557985959u, m_Seed0) + m_Seed1;
#else
	u64 temp = (u64)1557985959 * m_Seed0 + m_Seed1;
#endif
    m_Seed0 = u32( temp & ~0u );
    m_Seed1 = u32( ( temp >> 32 ) & ~0u );
    // All 32 bits are random but it would break too much code
    // if the sign bit was left in.
    return m_Seed0 & ( ~0u >> 1 );
}

#if __DEBUGLOG
inline int mthRandom::GetInt()
{
    if (m_ReplayDebug)
    {
        diagDebugLog(diagDebugLogMisc, 'mrs0', &m_Seed0); 
		diagDebugLog(diagDebugLogMisc, 'mrs1', &m_Seed1); 
    }

	return GetIntCommon();
}
#else
inline int mthRandom::GetInt()
{
    return GetIntCommon();
}
#endif

__forceinline ScalarV_Out mthRandom::GetFloatV()
{

	// Not static, staying thread-safe.
	int anInt;

	// Int rand # generation.
	anInt = GetInt();

	// Necessary LHS, but at least doesn't touch float registers.
	ScalarV combined = ScalarVFromS32(anInt);

	// Mask out all of each word but the mantissa (lower 23 bits).
#if __XENON && UNIQUE_VECTORIZED_TYPE
	ScalarV mantissaMask( __vsrw( __vspltisw(-1), __vspltisw(9) ) );
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
	ScalarV mantissaMask( (Vec::Vector_4V)vec_sr( vec_splat_s32(-1), vec_splat_u32(9) ) );
#else
	ScalarV mantissaMask( Vec::V4VConstant<0x007FFFFF,0x007FFFFF,0x007FFFFF,0x007FFFFF>() );
#endif
	ScalarV masked = combined & mantissaMask;

	// Convert from int to float and divide by (2e23f).
#if (__XENON || __PS3) && UNIQUE_VECTORIZED_TYPE
	// IntToFloatRaw<N> where N != 0 is efficient on Xenon/PS3, but emulated/slow on Win32.
	ScalarV maskedAsFloats = IntToFloatRaw<23>( masked );
#else
	// This is a good Win32 alternative since we are dividing by a constant amount.
	ScalarV oneOver2ToThe23rd( Vec::V4VConstant<0x34000000,0x34000000,0x34000000,0x34000000>() );
	ScalarV maskedAsFloats = IntToFloatRaw<0>( masked );
	maskedAsFloats *= oneOver2ToThe23rd;
#endif

	return maskedAsFloats;
}

__forceinline Vec2V_Out mthRandom::Get2FloatsV()
{

	// Not static, staying thread-safe.
	ALIGNAS(16) int twoInts[2] ;

	// Int rand # generation.
	twoInts[0] = GetInt();
	twoInts[1] = GetInt();

	// Necessary LHS, but at least doesn't touch float registers.
	Vec2V combined = *((Vec2V*)(&twoInts[0]));

	// Mask out all of each word but the mantissa (lower 23 bits).
#if __XENON && UNIQUE_VECTORIZED_TYPE
	Vec2V mantissaMask( __vsrw( __vspltisw(-1), __vspltisw(9) ) );
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
	Vec2V mantissaMask( (Vec::Vector_4V)vec_sr( vec_splat_s32(-1), vec_splat_u32(9) ) );
#else
	Vec2V mantissaMask( Vec::V4VConstant<0x007FFFFF,0x007FFFFF,0x007FFFFF,0x007FFFFF>() );
#endif
	Vec2V masked = combined & mantissaMask;

	// Convert from int to float and divide by (2e23f).
#if (__XENON || __PS3) && UNIQUE_VECTORIZED_TYPE
	// IntToFloatRaw<N> where N != 0 is efficient on Xenon/PS3, but emulated/slow on Win32.
	Vec2V maskedAsFloats = IntToFloatRaw<23>( masked );
#else
	// This is a good Win32 alternative since we are dividing by a constant amount.
	Vec2V oneOver2ToThe23rd( Vec::V4VConstant<0x34000000,0x34000000,0x34000000,0x34000000>() );
	Vec2V maskedAsFloats = IntToFloatRaw<0>( masked );
	maskedAsFloats *= oneOver2ToThe23rd;
#endif

	return maskedAsFloats;
}

__forceinline Vec3V_Out mthRandom::Get3FloatsV()
{

	// Not static, staying thread-safe.
	ALIGNAS(16) int threeInts[3] ;

	// Int rand # generation.
	threeInts[0] = GetInt();
	threeInts[1] = GetInt();
	threeInts[2] = GetInt();

	// Necessary LHS, but at least doesn't touch float registers.
	Vec3V combined = *((Vec3V*)(&threeInts[0]));

	// Mask out all of each word but the mantissa (lower 23 bits).
#if __XENON && UNIQUE_VECTORIZED_TYPE
	Vec3V mantissaMask( __vsrw( __vspltisw(-1), __vspltisw(9) ) );
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
	Vec3V mantissaMask( (Vec::Vector_4V)vec_sr( vec_splat_s32(-1), vec_splat_u32(9) ) );
#else
	Vec3V mantissaMask( Vec::V4VConstant<0x007FFFFF,0x007FFFFF,0x007FFFFF,0x007FFFFF>() );
#endif
	Vec3V masked = combined & mantissaMask;

	// Convert from int to float and divide by (2e23f).
#if (__XENON || __PS3) && UNIQUE_VECTORIZED_TYPE
	// IntToFloatRaw<N> where N != 0 is efficient on Xenon/PS3, but emulated/slow on Win32.
	Vec3V maskedAsFloats = IntToFloatRaw<23>( masked );
#else
	// This is a good Win32 alternative since we are dividing by a constant amount.
	Vec3V oneOver2ToThe23rd( Vec::V4VConstant<0x34000000,0x34000000,0x34000000,0x34000000>() );
	Vec3V maskedAsFloats = IntToFloatRaw<0>( masked );
	maskedAsFloats *= oneOver2ToThe23rd;
#endif

	return maskedAsFloats;
}

__forceinline Vec4V_Out mthRandom::Get4FloatsV()
{

	// Not static, staying thread-safe.
	ALIGNAS(16) int fourInts[4] ;

	// Int rand # generation.
	fourInts[0] = GetInt();
	fourInts[1] = GetInt();
	fourInts[2] = GetInt();
	fourInts[3] = GetInt();

	// Necessary LHS, but at least doesn't touch float registers.
	Vec4V combined = *((Vec4V*)(&fourInts[0]));

	// Mask out all of each word but the mantissa (lower 23 bits).
#if __XENON && UNIQUE_VECTORIZED_TYPE
	Vec4V mantissaMask( __vsrw( __vspltisw(-1), __vspltisw(9) ) );
#elif __PS3 && UNIQUE_VECTORIZED_TYPE
	Vec4V mantissaMask( (Vec::Vector_4V)vec_sr( vec_splat_s32(-1), vec_splat_u32(9) ) );
#else
	Vec4V mantissaMask( Vec::V4VConstant<0x007FFFFF,0x007FFFFF,0x007FFFFF,0x007FFFFF>() );
#endif
	Vec4V masked = combined & mantissaMask;

	// Convert from int to float and divide by (2e23f).
#if (__XENON || __PS3) && UNIQUE_VECTORIZED_TYPE
	// IntToFloatRaw<N> where N != 0 is efficient on Xenon/PS3, but emulated/slow on Win32.
	Vec4V maskedAsFloats = IntToFloatRaw<23>( masked );
#else
	// This is a good Win32 alternative since we are dividing by a constant amount.
	Vec4V oneOver2ToThe23rd( Vec::V4VConstant<0x34000000,0x34000000,0x34000000,0x34000000>() );
	Vec4V maskedAsFloats = IntToFloatRaw<0>( masked );
	maskedAsFloats *= oneOver2ToThe23rd;
#endif

	return maskedAsFloats;
}

inline float mthRandom::GetFloat()
{
	// Mask lower 23 bits, multiply by 1/2**23.
	return (GetInt() & ((1<<23)-1)) * 0.00000011920928955078125f;
}


inline bool mthRandom::GetBool()
{
	// Take the lowest bit from a random integer.
	return (GetInt() & 1);
}



} // namespace rage

#endif
