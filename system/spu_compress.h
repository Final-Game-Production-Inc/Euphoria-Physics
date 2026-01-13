// 
// system/spu_compress.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SPU_COMPRESS_H
#define SYSTEM_SPU_COMPRESS_H

#if __SPU
#include <spu_intrinsics.h>

namespace rage
{

// Description:
// Pack the high halves of each 32 bit word into a vector of shorts.
static vec_uchar16 s_packHighHalves = (vec_uchar16){2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31};

// Description:
// Pack the low halves of each 32 bit word into a vector of shorts.
static vec_uchar16 s_packLowHalves = (vec_uchar16){0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29};

// Description:
// Pack bytes into the low half of each 32 bit word.
static vec_uchar16 s_packBytes = (vec_uchar16){3, 7, 0x80, 0x80, 11, 15, 0x80, 0x80, 19, 23, 0x80, 0x80, 27, 31, 0x80, 0x80};

// Description:
// Performs a maximum operation on the 4 corresponding integer fields of the specified vec_int4s.
// Arguments:
// vec1 - First vector of 8 shorts.
// vec2 - Second vector of 8 shorts.
// Returns:
// Vector of 8 shorts containing the maximum int from vec1 or vec2.
static inline vec_short8 vecMax(vec_short8 vec1, vec_short8 vec2)
{
	return spu_sel(vec1, vec2, spu_cmpgt(vec2, vec1));
}

// Description:
// Performs a minimum operation on the 4 corresponding float fields of the specified vec_float4s.
// Arguments:
// vec1 - First vector of 4 floats.
// vec2 - Second vector of 4 floats.
// Returns:
// Vector of 4 floats containing the minimum float from vec1 or vec2.
static inline vec_float4 vecMinMax(vec_float4 vec, vec_float4 minVal, vec_float4 maxVal)
{
	// Predicates for min/max
	vec_uint4 cmpMin = spu_cmpgt(vec, minVal);
	vec_uint4 cmpMax = spu_cmpgt(vec, maxVal);

	vec = spu_sel(minVal, vec, cmpMin);
	vec = spu_sel(vec, maxVal, cmpMax);

	return vec;
}

// Description:
// Packs an array of floats into quadwords.  8 floats into a quadword as 8 half floats.  This does not deal with denormalized or NaN values.
// Arguments:
// dst - Destination buffer for half floats.
// src - Source buffer for floats.
// numFloats - Number of floats to convert
static inline void packFloatsToHalf(vec_uint4 * __restrict__ dst, const vec_float4 *src, unsigned int numFloats)
{
	vec_uchar16 packLowHalves	= s_packLowHalves;
	vec_uint4	mantissaOne = spu_splats(0x00800000u);
	vec_uint4	mantissaMask = spu_splats(0x007FFFFFu);

	unsigned int numQuads = ((numFloats + 7) >> 3);

	const vec_uint4 *intSrc = (const vec_uint4 *)src;

	// Prime the loop.
	vec_uint4 floats1 = *intSrc++;
	vec_uint4 floats2 = *intSrc++;
	vec_uint4 floats1Left3 = spu_sl(floats1, 3);
	vec_uint4 floats2Left3 = spu_sl(floats2, 3);
	vec_ushort8 results1 = (vec_ushort8)spu_shuffle(floats1Left3, floats2Left3, packLowHalves);

	// Pack the exponents for denormal and saturation testing.
	vec_ushort8 packedSignExp = (vec_ushort8)spu_shuffle(floats1, floats2, packLowHalves);
	vec_ushort8 packedMaskedExp = spu_and(packedSignExp, (short)(255<<7));
	vec_ushort8 packedExp = spu_rl(packedMaskedExp, 32-23);

	// A source exponent of -15 equates to 1 bit shift of mantissa in the denormal result.  Also 6 bits shift for getting mantissa in right place for destination. Source exponent bias is 127
	vec_short8 denormShift = spu_sub((vec_short8)packedExp, spu_splats((short)((127+5)-15)));
	vec_uint4 denormResults1 = spu_sel(mantissaOne, floats1, mantissaMask);
	vec_uint4 denormResults2 = spu_sel(mantissaOne, floats2, mantissaMask);

	// Pack the floats (8 at a time).
	// 24 cycle loop, giving performance of over 1 billion floats per second per spu (3.2 GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_uint4 *const intSrcEnd = intSrc + (numQuads*2);
	while (intSrc != intSrcEnd)
	{

///////////////////////
// Normal processing //
///////////////////////

		// Start with normal processed result.  May replace with saturated or denormalised later...
		vec_ushort8 result = spu_sel(results1, packedSignExp, spu_splats((unsigned short)0xC000));

//////////////////////////
// Saturated processing //
//////////////////////////

		// Predicate for saturated result
		vec_ushort8 predSaturated = spu_cmpgt(packedMaskedExp, spu_splats((unsigned short)((127+15)<<7)));
		result = spu_sel(result, spu_splats((unsigned short)0x7C00u), predSaturated);

/////////////////////////////
// Denormalised processing //
/////////////////////////////

		// Generate predicate for denormal.
		vec_ushort8 predDenorm = spu_cmpgt(spu_splats((unsigned short)((-14+127)<<7)), packedMaskedExp);
		vec_ushort8 denormInvalid = spu_cmpgt(spu_splats((short)-15), denormShift);	// Mask for denormal results invalid due to excessive shift.

		// Merge mantissas with leading 1.
		vec_ushort8 packedMantissas = (vec_ushort8)spu_shuffle(denormResults1, denormResults2, (vec_uchar16){1,2, 5,6, 9,10, 13,14, 17,18, 21,22, 25,26, 29,30});

		// Shift the mantissas.
		vec_ushort8 denormResult = spu_rlmask(packedMantissas, denormShift);

		// Mask out the mantissa if it shifted right out (zero).
		denormResult = spu_andc(denormResult, denormInvalid);

		// Select the denormal into the result.
		result = spu_sel(result, denormResult, predDenorm);

		// Load next loop inputs
		floats1 = *intSrc++;
		floats2 = *intSrc++;
		floats1Left3 = spu_sl(floats1, 3);
		floats2Left3 = spu_sl(floats2, 3);
		results1 = (vec_ushort8)spu_shuffle(floats1Left3, floats2Left3, packLowHalves);

/////////////////////////////////////////////////
// Combine all the results in the right places //
/////////////////////////////////////////////////

		// Put the sign bits in - this doesn't depend on the type of result.  Write the output.
		*dst++ = (vec_uint4)spu_sel(result, packedSignExp, spu_splats((unsigned short)0x8000u));

		// Pack the exponents for denormal and saturation testing.
		packedSignExp = (vec_ushort8)spu_shuffle(floats1, floats2, packLowHalves);
		packedMaskedExp = spu_and(packedSignExp, (short)(255<<7));
		packedExp = spu_rl(packedMaskedExp, 32-23);

		// Start working out the next result.
		// A source exponent of -15 equates to 1 bit shift of mantissa.  Also 6 bits shift for getting mantissa in right place for destination. Source exponent bias is 127
		denormShift = spu_sub((vec_short8)packedExp, spu_splats((short)((127+5)-15)));
		denormResults1 = spu_sel(mantissaOne, floats1, mantissaMask);
		denormResults2 = spu_sel(mantissaOne, floats2, mantissaMask);
	}
}

// Description:
// Packs an array of floats into quadwords.  16 floats into a quadword as 16 unsigned bytes.  This does not deal with denormalized or NaN values.
// Arguments:
// dst - Destination buffer for unsigned bytes.
// src - Source buffer for floats.
// numFloats - Number of floats to convert
static inline void packFloatsToUByte(vec_uint4 * __restrict__ dst, const vec_float4 *src, unsigned int numFloats)
{
	// Work out how many quadwords to output.
	unsigned int numQwordsOut = (numFloats + 15) >> 4;

	// Load constants into registers
	vec_uchar16 packHalves = s_packLowHalves;
	vec_uchar16 packBytes = s_packBytes;
	vec_float4 dstMinVal = spu_splats(0.0f);
	vec_float4 dstMaxVal = spu_splats(1.0f);
	vec_float4 scale = spu_splats(255.0f);

	// Prime the loop
	vec_float4 floats1 = *src++;
	vec_float4 floats2 = *src++;
	vec_float4 floats3 = *src++;
	vec_float4 floats4 = *src++;

	// Generate overflow and underflow predicates.
	vec_uint4 overflow1 = spu_cmpgt(floats1, dstMaxVal);
	vec_uint4 overflow2 = spu_cmpgt(floats2, dstMaxVal);
	vec_uint4 overflow3 = spu_cmpgt(floats3, dstMaxVal);
	vec_uint4 overflow4 = spu_cmpgt(floats4, dstMaxVal);
	vec_uint4 underflow1 = spu_cmpgt(dstMinVal, floats1);
	vec_uint4 underflow2 = spu_cmpgt(dstMinVal, floats2);
	vec_uint4 underflow3 = spu_cmpgt(dstMinVal, floats3);
	vec_uint4 underflow4 = spu_cmpgt(dstMinVal, floats4);

	// Scale for output range
	floats1 = spu_mul(floats1, scale);
	floats2 = spu_mul(floats2, scale);
	floats3 = spu_mul(floats3, scale);
	floats4 = spu_mul(floats4, scale);

	// Convert to bytes
	vec_uint4 ints1 = spu_convtu(floats1, 0);
	vec_uint4 ints2 = spu_convtu(floats2, 0);
	vec_uint4 ints3 = spu_convtu(floats3, 0);
	vec_uint4 ints4 = spu_convtu(floats4, 0);

	// Load next loop inputs
	vec_float4 nextFloats1 = *src++;
	vec_float4 nextFloats2 = *src++;
	vec_float4 nextFloats3 = *src++;
	vec_float4 nextFloats4 = *src++;

	// Pack the floats (16 at a time).
	// 22 cycle loop giving performance as over 2.3 billion floats per second per spu (3.2 GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_float4 *const srcEnd = src + (numQwordsOut*4);
	while (src != srcEnd)
	{
		// And pack them into a quadword
		ints1 = spu_shuffle(ints1, ints2, packBytes);
		ints3 = spu_shuffle(ints3, ints4, packBytes);
		ints1 = spu_shuffle(ints1, ints3, packHalves);

		// Pack overflow and underflow masks
		overflow1 = spu_shuffle(overflow1, overflow2, packBytes);
		overflow3 = spu_shuffle(overflow3, overflow4, packBytes);
		overflow1 = spu_shuffle(overflow1, overflow3, packHalves);
		underflow1 = spu_shuffle(underflow1, underflow2, packBytes);
		underflow3 = spu_shuffle(underflow3, underflow4, packBytes);
		underflow1 = spu_shuffle(underflow1, underflow3, packHalves);

		// And combine with output and write
		ints1 = spu_or(ints1, overflow1);
		*dst++ = spu_andc(ints1, underflow1);

		// Generate overflow and underflow predicates.
		overflow1 = spu_cmpgt(nextFloats1, dstMaxVal);
		overflow2 = spu_cmpgt(nextFloats2, dstMaxVal);
		overflow3 = spu_cmpgt(nextFloats3, dstMaxVal);
		overflow4 = spu_cmpgt(nextFloats4, dstMaxVal);
		underflow1 = spu_cmpgt(dstMinVal, nextFloats1);
		underflow2 = spu_cmpgt(dstMinVal, nextFloats2);
		underflow3 = spu_cmpgt(dstMinVal, nextFloats3);
		underflow4 = spu_cmpgt(dstMinVal, nextFloats4);

		// Scale for output range
		floats1 = spu_mul(nextFloats1, scale);
		floats2 = spu_mul(nextFloats2, scale);
		floats3 = spu_mul(nextFloats3, scale);
		floats4 = spu_mul(nextFloats4, scale);

		nextFloats1 = *src++;
		nextFloats2 = *src++;
		nextFloats3 = *src++;
		nextFloats4 = *src++;

		// Convert to bytes
		ints1 = spu_convtu(floats1, 0);
		ints2 = spu_convtu(floats2, 0);
		ints3 = spu_convtu(floats3, 0);
		ints4 = spu_convtu(floats4, 0);
	}
}

// Description:
// Packs an array of floats into quadwords.  8 floats into a quadword as 8 unsigned shorts.  This does not deal with denormalized or NaN values.
// Arguments:
// dst - Destination buffer for unsigned shorts.
// src - Source buffer for floats.
// numFloats - Number of floats to convert
static inline void packFloatsToUShort(vec_uint4 * __restrict__ dst, const vec_float4 *src, unsigned int numFloats)
{
	// Work out how many quadwords to output.
	numFloats += 7;

	unsigned int numQwordsOut2 = numFloats >> 4;
	unsigned int numQwordsOut = numFloats & 8;

	// Load constants into registers
	vec_uchar16 packHalves = s_packHighHalves;
	vec_float4 dstMinVal = spu_splats(0.0f);
	vec_float4 dstMaxVal = spu_splats(1.0f);
	vec_float4 scale = spu_splats(65535.0f);

	// Prime the loop
	vec_float4 floats1 = *src++;
	vec_float4 floats2 = *src++;
	vec_float4 floats3 = *src++;
	vec_float4 floats4 = *src++;

	// Generate overflow and underflow predicates.
	vec_uint4 overflow1 = spu_cmpgt(floats1, dstMaxVal);
	vec_uint4 overflow2 = spu_cmpgt(floats2, dstMaxVal);
	vec_uint4 overflow3 = spu_cmpgt(floats3, dstMaxVal);
	vec_uint4 overflow4 = spu_cmpgt(floats4, dstMaxVal);
	vec_uint4 underflow1 = spu_cmpgt(dstMinVal, floats1);
	vec_uint4 underflow2 = spu_cmpgt(dstMinVal, floats2);
	vec_uint4 underflow3 = spu_cmpgt(dstMinVal, floats3);
	vec_uint4 underflow4 = spu_cmpgt(dstMinVal, floats4);

	// Scale for output range
	floats1 = spu_mul(floats1, scale);
	floats2 = spu_mul(floats2, scale);
	floats3 = spu_mul(floats3, scale);
	floats4 = spu_mul(floats4, scale);

	// Convert to bytes
	vec_uint4 ints1 = spu_convtu(floats1, 0);
	vec_uint4 ints2 = spu_convtu(floats2, 0);
	vec_uint4 ints3 = spu_convtu(floats3, 0);
	vec_uint4 ints4 = spu_convtu(floats4, 0);

	// Load next loop inputs
	vec_float4 nextFloats1 = *src++;
	vec_float4 nextFloats2 = *src++;
	vec_float4 nextFloats3 = *src++;
	vec_float4 nextFloats4 = *src++;

	// Pack the floats (16 at a time).
	// 24 cycle loop giving performance of over 2.1 billion floats per second per spu (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_float4 *const srcEnd = src + (numQwordsOut2 * 4);
	while (src != srcEnd)
	{
		// And pack them into a quadword and place in output buffer
		ints1 = spu_shuffle(ints1, ints2, packHalves);
		ints3 = spu_shuffle(ints3, ints4, packHalves);

		// Pack overflow and underflow masks
		overflow1 = spu_shuffle(overflow1, overflow2, packHalves);
		overflow3 = spu_shuffle(overflow3, overflow4, packHalves);
		underflow1 = spu_shuffle(underflow1, underflow2, packHalves);
		underflow3 = spu_shuffle(underflow3, underflow4, packHalves);

		ints1 = spu_or(ints1, overflow1);
		*dst++ = spu_andc(ints1, underflow1);

		ints3 = spu_or(ints3, overflow3);
		*dst++ = spu_andc(ints3, underflow3);

		// Generate overflow and underflow predicates.
		overflow1 = spu_cmpgt(nextFloats1, dstMaxVal);
		overflow2 = spu_cmpgt(nextFloats2, dstMaxVal);
		overflow3 = spu_cmpgt(nextFloats3, dstMaxVal);
		overflow4 = spu_cmpgt(nextFloats4, dstMaxVal);
		underflow1 = spu_cmpgt(dstMinVal, nextFloats1);
		underflow2 = spu_cmpgt(dstMinVal, nextFloats2);
		underflow3 = spu_cmpgt(dstMinVal, nextFloats3);
		underflow4 = spu_cmpgt(dstMinVal, nextFloats4);

		// Scale for output range
		floats1 = spu_mul(nextFloats1, scale);
		floats2 = spu_mul(nextFloats2, scale);
		floats3 = spu_mul(nextFloats3, scale);
		floats4 = spu_mul(nextFloats4, scale);

		// Load next loop inputs
		nextFloats1 = *src++;
		nextFloats2 = *src++;
		nextFloats3 = *src++;
		nextFloats4 = *src++;

		// Convert to bytes
		ints1 = spu_convtu(floats1, 0);
		ints2 = spu_convtu(floats2, 0);
		ints3 = spu_convtu(floats3, 0);
		ints4 = spu_convtu(floats4, 0);
	}

	// Store the scrag end
	if (numQwordsOut)
	{
		// And pack them into a quadword and place in output buffer
		ints1 = spu_shuffle(ints1, ints2, packHalves);
		overflow1 = spu_shuffle(overflow1, overflow2, packHalves);
		underflow1 = spu_shuffle(underflow1, underflow2, packHalves);

		ints1 = spu_or(ints1, overflow1);
		*dst++ = spu_andc(ints1, underflow1);
	}
}

// Description:
// Packs an array of floats into quadwords.  8 floats into a quadword as 8 shorts.  This does not deal with denormalized or NaN values.
// Arguments:
// dst - Destination buffer for shorts.
// src - Source buffer for floats.
// numFloats - Number of floats to convert
static inline void packFloatsToShort(vec_uint4 * __restrict__ dst, const vec_float4 *src, unsigned int numFloats)
{
	// Work out how many quadwords to output.
	numFloats += 7;

	unsigned int numQwordsOut2 = numFloats >> 4;
	unsigned int numQwordsOut = numFloats & 8;

	// Load constants into registers
	vec_uchar16 packHalves = s_packHighHalves;
	vec_float4 dstMinVal = spu_splats(-1.0f);
	vec_float4 dstMaxVal = spu_splats(1.0f);
	vec_float4 scale = spu_splats(32767.0f);
	vec_int4 overflowVal = spu_splats((int)0x7FFF7FFF);
	vec_int4 underflowVal = spu_splats((int)0x80008000);

	// Prime the loop
	vec_float4 floats1 = *src++;
	vec_float4 floats2 = *src++;
	vec_float4 floats3 = *src++;
	vec_float4 floats4 = *src++;

	// Generate overflow and underflow predicates.
	vec_uint4 overflow1 = spu_cmpgt(floats1, dstMaxVal);
	vec_uint4 overflow2 = spu_cmpgt(floats2, dstMaxVal);
	vec_uint4 overflow3 = spu_cmpgt(floats3, dstMaxVal);
	vec_uint4 overflow4 = spu_cmpgt(floats4, dstMaxVal);
	vec_uint4 underflow1 = spu_cmpgt(dstMinVal, floats1);
	vec_uint4 underflow2 = spu_cmpgt(dstMinVal, floats2);
	vec_uint4 underflow3 = spu_cmpgt(dstMinVal, floats3);
	vec_uint4 underflow4 = spu_cmpgt(dstMinVal, floats4);

	// Scale for output range
	floats1 = spu_mul(floats1, scale);
	floats2 = spu_mul(floats2, scale);
	floats3 = spu_mul(floats3, scale);
	floats4 = spu_mul(floats4, scale);

	// Convert to bytes
	vec_int4 ints1 = spu_convts(floats1, 0);
	vec_int4 ints2 = spu_convts(floats2, 0);
	vec_int4 ints3 = spu_convts(floats3, 0);
	vec_int4 ints4 = spu_convts(floats4, 0);

	// Load next loop inputs
	vec_float4 nextFloats1 = *src++;
	vec_float4 nextFloats2 = *src++;
	vec_float4 nextFloats3 = *src++;
	vec_float4 nextFloats4 = *src++;

	// Pack the floats (16 at a time).
	// 24 cycle loop giving performance of over 2.1 billion floats per second per spu (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_float4 *const srcEnd = src + (numQwordsOut2 * 4);
	while (src != srcEnd)
	{
		// And pack them into a quadword and place in output buffer
		ints1 = (vec_int4)spu_shuffle(ints1, ints2, packHalves);
		ints3 = (vec_int4)spu_shuffle(ints3, ints4, packHalves);

		// Pack overflow and underflow masks
		overflow1 = spu_shuffle(overflow1, overflow2, packHalves);
		overflow3 = spu_shuffle(overflow3, overflow4, packHalves);
		underflow1 = spu_shuffle(underflow1, underflow2, packHalves);
		underflow3 = spu_shuffle(underflow3, underflow4, packHalves);

		ints1 = spu_sel(ints1, overflowVal, overflow1);
		ints1 = spu_sel(ints1, underflowVal, underflow1);
		ints3 = spu_sel(ints3, overflowVal, overflow3);
		ints3 = spu_sel(ints3, underflowVal, underflow3);

		// Merge in clamp values.
		*dst++ = (vec_uint4)ints1;
		*dst++ = (vec_uint4)ints3;

		// Generate overflow and underflow predicates.
		overflow1 = spu_cmpgt(nextFloats1, dstMaxVal);
		overflow2 = spu_cmpgt(nextFloats2, dstMaxVal);
		overflow3 = spu_cmpgt(nextFloats3, dstMaxVal);
		overflow4 = spu_cmpgt(nextFloats4, dstMaxVal);
		underflow1 = spu_cmpgt(dstMinVal, nextFloats1);
		underflow2 = spu_cmpgt(dstMinVal, nextFloats2);
		underflow3 = spu_cmpgt(dstMinVal, nextFloats3);
		underflow4 = spu_cmpgt(dstMinVal, nextFloats4);

		// Scale for output range
		floats1 = spu_mul(nextFloats1, scale);
		floats2 = spu_mul(nextFloats2, scale);
		floats3 = spu_mul(nextFloats3, scale);
		floats4 = spu_mul(nextFloats4, scale);

		// Load next loop inputs
		nextFloats1 = *src++;
		nextFloats2 = *src++;
		nextFloats3 = *src++;
		nextFloats4 = *src++;

		// Convert to bytes
		ints1 = spu_convts(floats1, 0);
		ints2 = spu_convts(floats2, 0);
		ints3 = spu_convts(floats3, 0);
		ints4 = spu_convts(floats4, 0);
	}

	// Store the scrag end
	if (numQwordsOut)
	{
		// And pack them into a quadword and place in output buffer
		ints1 = spu_shuffle(ints1, ints2, packHalves);

		// Pack overflow and underflow masks
		overflow1 = spu_shuffle(overflow1, overflow2, packHalves);
		underflow1 = spu_shuffle(underflow1, underflow2, packHalves);

		ints1 = spu_sel(ints1, overflowVal, overflow1);
		ints1 = spu_sel(ints1, underflowVal, underflow1);

		// Merge in clamp values.
		*dst++ = (vec_uint4)ints1;
	}
}

// Description:
// Packs an array of 3 floats into 11bit-11bit-10bit ints.
// Arguments:
// dst - Destination buffer for 11bit-11bit-10bit ints.
// src - Source buffer for floats.
// num3Floats - Number of 3 floats to convert
static inline void pack3FloatsTo11_11_10(vec_uint4 *__restrict__ dst, const vec_float4 *src, int num3Floats)
{
	// Mask used to pack floats into 11_11_10 format
	static const vec_uint4   mask0 = (vec_uint4) {0xFFFFF800, 0xFFFFF800, 0xFFFFF800, 0xFFFFF800};
	static const vec_uint4   mask1 = (vec_uint4) {0xFFC00000, 0xFFC00000, 0xFFC00000, 0xFFC00000};

	// Shuffle masks to generate {x1 x2 x3 x4} vectors
	static const vec_uchar16 gShuffWord036x = (vec_uchar16) {
		0x00, 0x01, 0x02, 0x03,
		0x0c, 0x0d, 0x0e, 0x0f,
		0x18, 0x19, 0x1a, 0x1b,
		0x80, 0x80, 0x80, 0x80};
	static const vec_uchar16 gShuffWord0125 = (vec_uchar16) {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b,
		0x14, 0x15, 0x16, 0x17};

	// Shuffle mask to generate {y1 y2 y3 y4} vectors
	static const vec_uchar16 gShuffWord147x = (vec_uchar16) {
		0x04, 0x05, 0x06, 0x07,
		0x10, 0x11, 0x12, 0x13,
		0x1c, 0x1d, 0x1e, 0x1f,
		0x80, 0x80, 0x80, 0x80};
	static const vec_uchar16 gShuffWord0126 = (vec_uchar16) {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b,
		0x18, 0x19, 0x1a, 0x1b};

	// Shuffle mask to generate {z1 z2 z3 z4} vectors
	static const vec_uchar16 gShuffWord25xx = (vec_uchar16) {
		0x08, 0x09, 0x0a, 0x0b,
		0x14, 0x15, 0x16, 0x17,
		0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80};
	static const vec_uchar16 gShuffWord0147 = (vec_uchar16) {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x10, 0x11, 0x12, 0x13,
		0x1c, 0x1d, 0x1e, 0x1f};

	// Minimum and maximum values for input floats
	vec_float4 minVal = spu_splats(-1.0f);
	vec_float4 maxValXY = spu_splats(1023.0f/1024.0f);
	vec_float4 maxValZ  = spu_splats(511.0f/512.0f);

	// Min and max converted values
	vec_int4 overflowValue = spu_splats((511<<22)|(1023<<11)|(1023<<0));
	vec_int4 underflowValue = spu_splats((512<<22)|(1024<<11)|(1024<<0));

	// Read inputs
	vec_float4 vec0 = *src++;
	vec_float4 vec1 = *src++;
	vec_float4 vec2 = *src++;

	// Re-shuffle data
	vec_float4 x = spu_shuffle(vec0, vec1, gShuffWord036x);
	x = spu_shuffle(x, vec2, gShuffWord0125);

	vec_float4 y = spu_shuffle(vec0, vec1, gShuffWord147x);
	y = spu_shuffle(y, vec2, gShuffWord0126);

	vec_float4 z = spu_shuffle(vec0, vec1, gShuffWord25xx);
	z = spu_shuffle(z, vec2, gShuffWord0147);

	// 21 cycle loop.
	while (num3Floats > 0)
	{
		num3Floats -= 4;

		// Read in next vectors
		vec0 = *src++;
		vec1 = *src++;
		vec2 = *src++;

		vec_uint4 overflowX = spu_cmpgt(x, maxValXY);
		vec_uint4 overflowY = spu_cmpgt(y, maxValXY);
		vec_uint4 overflowZ = spu_cmpgt(z, maxValZ);
		vec_uint4 underflowX = spu_cmpgt(minVal, x);
		vec_uint4 underflowY = spu_cmpgt(minVal, y);
		vec_uint4 underflowZ = spu_cmpgt(minVal, z);

		// Scale floats
		vec_int4 intX = spu_convts(x,10); // 1024 * (1<<0)
		vec_int4 intY = spu_convts(y,21); // 1024 * (1<<11)
		vec_int4 intZ = spu_convts(z,31); //  512 * (1<<22)

		// Pack overflow and underflow
		vec_uint4 overflow = spu_sel(overflowX, overflowY, mask0);
		overflow = spu_sel(overflow, overflowZ, mask1);
		vec_uint4 underflow = spu_sel(underflowX, underflowY, mask0);
		underflow = spu_sel(underflow, underflowZ, mask1);

		// Pack converted value
		vec_int4 packed = spu_sel(intX, intY, mask0);
		packed = spu_sel(packed, intZ, mask1);

		// Combine overflow and underflow.
		packed = spu_sel(packed, overflowValue, overflow);
		packed = spu_sel(packed, underflowValue, underflow);

		*dst++ = (vec_uint4)packed;

		x = spu_shuffle(vec0, vec1, gShuffWord036x);
		x = spu_shuffle(x, vec2, gShuffWord0125);

		y = spu_shuffle(vec0, vec1, gShuffWord147x);
		y = spu_shuffle(y, vec2, gShuffWord0126);

		z = spu_shuffle(vec0, vec1, gShuffWord25xx);
		z = spu_shuffle(z, vec2, gShuffWord0147);
	}
}

} //  namespace rage

#endif // __SPU
#endif // SYSTEM_SPU_COMPRESS_H
