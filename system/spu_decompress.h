// 
// system/spu_decompress.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SPU_DECOMPRESS_H
#define SYSTEM_SPU_DECOMPRESS_H

#if __SPU
#include <spu_intrinsics.h>

namespace rage
{

// Description:
// Shuffle word to unpack bytes 0, 1, 2, and 3 to 32 bit values.
static vec_uchar16 s_unpackXBytes = (vec_uchar16){0x80, 0x80, 0x80,   0, 0x80, 0x80, 0x80,   1, 0x80, 0x80, 0x80,   2, 0x80, 0x80, 0x80,   3};

// Description:
// Shuffle word to unpack bytes 4, 5, 6, and 7 to 32 bit values.
static vec_uchar16 s_unpackYBytes = (vec_uchar16){0x80, 0x80, 0x80,   4, 0x80, 0x80, 0x80,   5, 0x80, 0x80, 0x80,   6, 0x80, 0x80, 0x80,   7};

// Description:
// Shuffle word to unpack bytes 8, 9, 10, and 11 to 32 bit values.
static vec_uchar16 s_unpackZBytes = (vec_uchar16){0x80, 0x80, 0x80,   8, 0x80, 0x80, 0x80,   9, 0x80, 0x80, 0x80, 0xa, 0x80, 0x80, 0x80, 0xb};

// Description:
// Shuffle word to unpack bytes 12, 13, 14, and 15 to 32 bit values.
static vec_uchar16 s_unpackWBytes = (vec_uchar16){0x80, 0x80, 0x80, 0xc, 0x80, 0x80, 0x80, 0xd, 0x80, 0x80, 0x80, 0xe, 0x80, 0x80, 0x80, 0xf};

// Description:
// Shuffle word to unpack shorts 0, 1, 2, and 3 to 32 bit values.
static vec_uchar16 s_unpackXYShorts = (vec_uchar16){0x80, 0x80, 0, 1, 0x80, 0x80,   2,   3, 0x80, 0x80,   4,   5, 0x80, 0x80,   6,   7};

// Description:
// Shuffle word to unpack shorts 4, 5, 6, and 7 to 32 bit values.
static vec_uchar16 s_unpackZWShorts = (vec_uchar16){0x80, 0x80, 8, 9, 0x80, 0x80, 0xa, 0xb, 0x80, 0x80, 0xc, 0xd, 0x80, 0x80, 0xe, 0xf};

// Description:
// Shuffle word to unpack shorts 0, 1, 2, and 3 to the upper 16 bits of each 32 bit value.
static vec_uchar16 s_unpackXYShortsUp = (vec_uchar16){0, 1, 0x80, 0x80,   2,   3, 0x80, 0x80,   4,   5, 0x80, 0x80,   6,   7, 0x80, 0x80};

// Description:
// Shuffle word to unpack shorts 4, 5, 6, and 7 to the upper 16 bits of each 32 bit value.
static vec_uchar16 s_unpackZWShortsUp = (vec_uchar16){8, 9, 0x80, 0x80, 0xa, 0xb, 0x80, 0x80, 0xc, 0xd, 0x80, 0x80, 0xe, 0xf, 0x80, 0x80};

// Description:
// Shuffle word to expand the lowest byte of each short in XY to a word.
static vec_uchar16 s_expandXYshorts = (vec_uchar16){0, 0, 0, 0, 2, 2, 2, 2, 4, 4, 4, 4, 6, 6, 6, 6};

// Description:
// Shuffle word to expand the lowest byte of each short in ZW to a word.
static vec_uchar16 s_expandZWshorts = (vec_uchar16){8, 8, 8, 8, 10, 10, 10, 10, 12, 12, 12, 12, 14, 14, 14, 14};

// Description:
// Shuffle masks to generate {x1 y1 z1 x2} vectors.
static vec_uchar16 s_shuffWord04x1  = (vec_uchar16) {
	0x00, 0x01, 0x02, 0x03,
	0x10, 0x11, 0x12, 0x13,
	0x80, 0x80, 0x80, 0x80,
	0x04, 0x05, 0x06, 0x07};
// Description:
// Shuffle masks to generate {x1 y1 z1 x2} vectors.
static vec_uchar16 s_shuffWord0143 = (vec_uchar16) {
	0x00, 0x01, 0x02, 0x03,
	0x04, 0x05, 0x06, 0x07,
	0x10, 0x11, 0x12, 0x13,
	0x0c, 0x0d, 0x0e, 0x0f};

// Description:
// Shuffle mask to generate {y2 z2 x3 y3} vectors.
static vec_uchar16 s_shuffWord5x26 = (vec_uchar16) {
	0x14, 0x15, 0x16, 0x17,
	0x80, 0x80, 0x80, 0x80,
	0x08, 0x09, 0x0a, 0x0b,
	0x18, 0x19, 0x1a, 0x1b};

// Description:
// Shuffle mask to generate {y2 z2 x3 y3} vectors.
static vec_uchar16 s_shuffWord0523 = (vec_uchar16) {
	0x00, 0x01, 0x02, 0x03,
	0x14, 0x15, 0x16, 0x17,
	0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f};

// Description:
// Shuffle mask to generate {z3 x4 y4 z4} vectors.
static vec_uchar16 s_shuffWordx37x = (vec_uchar16) {
	0x80, 0x80, 0x80, 0x80,
	0x0c, 0x0d, 0x0e, 0x0f,
	0x1c, 0x1d, 0x1e, 0x1f,
	0x80, 0x80, 0x80, 0x80};

// Description:
// Shuffle mask to generate {z3 x4 y4 z4} vectors.
static vec_uchar16 s_shuffWord6127 = (vec_uchar16) {
	0x18, 0x19, 0x1a, 0x1b,
	0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b,
	0x1c, 0x1d, 0x1e, 0x1f};

// Description:
// Pack an array of bytes into floats.  16 unsigned bytes from a quadword as 16 floats.
// Arguments:
// dst - Destination buffer for floats.
// src - Source buffer for unsigned bytes.
// numBytes - Number of bytes to convert
static inline void unpackUByteToFloats(vec_float4 * __restrict__ dst, const vec_uint4 *src, unsigned int numBytes)
{
	numBytes += 15;

	unsigned int numQwordsIn2 = numBytes >> 5;
	unsigned int numQwordsIn = numBytes & 16;

	vec_uchar16 unpackXBytes = s_unpackXBytes;
	vec_uchar16 unpackYBytes = s_unpackYBytes;
	vec_uchar16 unpackZBytes = s_unpackZBytes;
	vec_uchar16 unpackWBytes = s_unpackWBytes;
	vec_float4 scale = spu_splats(1.0f / 255.0f);

	// Prime the loop
	vec_uint4 ints1 = *src++;
	vec_uint4 ints2 = *src++;
	vec_uint4 nextInts1 = *src++;
	vec_uint4 nextInts2 = *src++;

	vec_uint4 ints1X = spu_shuffle(ints1, ints1, unpackXBytes);
	vec_uint4 ints1Y = spu_shuffle(ints1, ints1, unpackYBytes);
	vec_uint4 ints1Z = spu_shuffle(ints1, ints1, unpackZBytes);
	vec_uint4 ints1W = spu_shuffle(ints1, ints1, unpackWBytes);
	vec_uint4 ints2X = spu_shuffle(ints2, ints2, unpackXBytes);
	vec_uint4 ints2Y = spu_shuffle(ints2, ints2, unpackYBytes);
	vec_uint4 ints2Z = spu_shuffle(ints2, ints2, unpackZBytes);
	vec_uint4 ints2W = spu_shuffle(ints2, ints2, unpackWBytes);

	vec_float4	result1, result2, result3, result4, result5, result6, result7, result8;

	result1 = spu_mul(spu_convtf(ints1X, 0), scale);
	result2 = spu_mul(spu_convtf(ints1Y, 0), scale);
	result3 = spu_mul(spu_convtf(ints1Z, 0), scale);
	result4 = spu_mul(spu_convtf(ints1W, 0), scale);

	// Unpack the floats (32 at a time).
	// 21 cycle loop giving performance of over 4.8 billion floats per second per spu (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_uint4 *const srcEnd = src + (numQwordsIn2 * 2);
	while (src != srcEnd)
	{
		result5 = spu_mul(spu_convtf(ints2X, 0), scale);
		result6 = spu_mul(spu_convtf(ints2Y, 0), scale);
		result7 = spu_mul(spu_convtf(ints2Z, 0), scale);
		result8 = spu_mul(spu_convtf(ints2W, 0), scale);

		*dst++ = result1;
		*dst++ = result2;
		*dst++ = result3;
		*dst++ = result4;
		*dst++ = result5;
		*dst++ = result6;
		*dst++ = result7;
		*dst++ = result8;

		ints1X = spu_shuffle(nextInts1, nextInts1, unpackXBytes);
		ints1Y = spu_shuffle(nextInts1, nextInts1, unpackYBytes);
		ints1Z = spu_shuffle(nextInts1, nextInts1, unpackZBytes);
		ints1W = spu_shuffle(nextInts1, nextInts1, unpackWBytes);
		ints2X = spu_shuffle(nextInts2, nextInts2, unpackXBytes);
		ints2Y = spu_shuffle(nextInts2, nextInts2, unpackYBytes);
		ints2Z = spu_shuffle(nextInts2, nextInts2, unpackZBytes);
		ints2W = spu_shuffle(nextInts2, nextInts2, unpackWBytes);

		nextInts1 = *src++;
		nextInts2 = *src++;

		result1 = spu_mul(spu_convtf(ints1X, 0), scale);
		result2 = spu_mul(spu_convtf(ints1Y, 0), scale);
		result3 = spu_mul(spu_convtf(ints1Z, 0), scale);
		result4 = spu_mul(spu_convtf(ints1W, 0), scale);
	}

	// Write the scrag end
	if (numQwordsIn)
	{
		*dst++ = result1;
		*dst++ = result2;
		*dst++ = result3;
		*dst++ = result4;
	}
}

// Description:
// Pack an array of unsigned shorts into floats.  8 unsigned shorts from a quadword as 8 floats.
// Arguments:
// dst - Destination buffer for floats.
// src - Source buffer for unsigned shorts.
// numShorts - Number of unsigned shorts to convert
static inline void unpackUShortToFloats(vec_float4 * __restrict__ dst, const vec_uint4 *src, unsigned int numShorts)
{
	numShorts += 7;

	unsigned int numQwordsIn2 = numShorts >> 4;
	unsigned int numQwordsIn = numShorts & 8;

	vec_uchar16 unpackXYShorts = s_unpackXYShorts;
	vec_uchar16 unpackZWShorts = s_unpackZWShorts;
	vec_float4 scale = spu_splats(1.0f / 65535.0f);

	// Prime the loop
	vec_uint4 ints1 = *src++;
	vec_uint4 ints2 = *src++;
	vec_uint4 nextInts1 = *src++;
	vec_uint4 nextInts2 = *src++;

	vec_uint4 ints1XY = spu_shuffle(ints1, ints1, unpackXYShorts);
	vec_uint4 ints1ZW = spu_shuffle(ints1, ints1, unpackZWShorts);
	vec_uint4 ints2XY = spu_shuffle(ints2, ints2, unpackXYShorts);
	vec_uint4 ints2ZW = spu_shuffle(ints2, ints2, unpackZWShorts);

	vec_float4	result1, result2, result3, result4;

	result1 = spu_mul(spu_convtf(ints1XY, 0), scale);
	result2 = spu_mul(spu_convtf(ints1ZW, 0), scale);

	// Unpack the floats (16 at a time).
	// 16 cycle loop giving performance of just over 3.2 billion floats per second per spu (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_uint4 *const srcEnd = src + (numQwordsIn2 * 2);
	while (src != srcEnd)
	{
		result3 = spu_mul(spu_convtf(ints2XY, 0), scale);
		result4 = spu_mul(spu_convtf(ints2ZW, 0), scale);

		*dst++ = result1;
		*dst++ = result2;
		*dst++ = result3;
		*dst++ = result4;

		ints1XY = spu_shuffle(nextInts1, nextInts1, unpackXYShorts);
		ints1ZW = spu_shuffle(nextInts1, nextInts1, unpackZWShorts);
		ints2XY = spu_shuffle(nextInts2, nextInts2, unpackXYShorts);
		ints2ZW = spu_shuffle(nextInts2, nextInts2, unpackZWShorts);

		nextInts1 = *src++;
		nextInts2 = *src++;

		result1 = spu_mul(spu_convtf(ints1XY, 0), scale);
		result2 = spu_mul(spu_convtf(ints1ZW, 0), scale);
	}

	if (numQwordsIn)
	{
		*dst++ = result1;
		*dst++ = result2;
	}
}

// Description:
// Pack an array of shorts into floats.  8 shorts from a quadword as 8 floats.
// Arguments:
// dst - Destination buffer for floats.
// src - Source buffer for shorts.
// numShorts - Number of shorts to convert
static inline void unpackShortToFloats(vec_float4 * __restrict__ dst, const vec_int4 *src, unsigned int numShorts)
{
	numShorts += 7;

	unsigned int numQwordsIn2 = numShorts >> 4;
	unsigned int numQwordsIn = numShorts & 8;

	vec_uchar16 unpackXYShortsUp = s_unpackXYShortsUp;
	vec_uchar16 unpackZWShortsUp = s_unpackZWShortsUp;
	vec_float4 scale = spu_splats(1.0f / 2147483647.0f);

	// Prime the loop
	vec_int4 ints1 = *src++;
	vec_int4 ints2 = *src++;
	vec_int4 nextInts1 = *src++;
	vec_int4 nextInts2 = *src++;
	vec_int4 ints1XY = spu_shuffle(ints1, ints1, unpackXYShortsUp);
	vec_int4 ints1ZW = spu_shuffle(ints1, ints1, unpackZWShortsUp);
	vec_int4 ints2XY = spu_shuffle(ints2, ints1, unpackXYShortsUp);
	vec_int4 ints2ZW = spu_shuffle(ints2, ints1, unpackZWShortsUp);

	vec_float4	result1, result2, result3, result4;

	result1 = spu_mul(spu_convtf(ints1XY, 0), scale);
	result2 = spu_mul(spu_convtf(ints1ZW, 0), scale);

	// Unpack the floats (16 at a time).
	// 16 cycle loop giving performance of 3.2 billion floats per second per spu. (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_int4 *const srcEnd = src + (numQwordsIn2 * 2);
	while (src != srcEnd)
	{
		result3 = spu_mul(spu_convtf(ints2XY, 0), scale);
		result4 = spu_mul(spu_convtf(ints2ZW, 0), scale);

		*dst++ = result1;
		*dst++ = result2;
		*dst++ = result3;
		*dst++ = result4;

		ints1XY = spu_shuffle(nextInts1, nextInts1, unpackXYShortsUp);
		ints1ZW = spu_shuffle(nextInts1, nextInts1, unpackZWShortsUp);
		ints2XY = spu_shuffle(nextInts2, nextInts2, unpackXYShortsUp);
		ints2ZW = spu_shuffle(nextInts2, nextInts2, unpackZWShortsUp);

		nextInts1 = *src++;
		nextInts2 = *src++;

		result1 = spu_mul(spu_convtf(ints1XY, 0), scale);
		result2 = spu_mul(spu_convtf(ints1ZW, 0), scale);
	}

	// Write the scrag end if necessary.
	if (numQwordsIn)
	{
		*dst++ = result1;
		*dst++ = result2;
	}
}

// Description:
// Pack an array of halfs into floats.  8 halfs from a quadword as 8 floats.
// Arguments:
// dst - Destination buffer for floats.
// src - Source buffer for halfs.
// numHalfs - Number of halfs to convert
static inline void unpackHalfToFloats(vec_float4 * __restrict__ dst, const vec_uint4 *src, unsigned int numHalfs)
{
	unsigned int numQwordsIn = (numHalfs + 7) >> 3;
	vec_uchar16 unpackXYShorts = s_unpackXYShorts;
	vec_uchar16 unpackZWShorts = s_unpackZWShorts;
	vec_uchar16 unpackXYShortsUp = s_unpackXYShortsUp;
	vec_uchar16 unpackZWShortsUp = s_unpackZWShortsUp;
	vec_uchar16 expandXYshorts = s_expandXYshorts;
	vec_uchar16 expandZWshorts = s_expandZWshorts;
	vec_uint4 sign16 = spu_splats((unsigned int)0x80008000);
	vec_uint4 sign32 = spu_splats((unsigned int)0x80000000);
	vec_ushort8 mantissa16 = spu_splats((unsigned short)0x03FF);

	// Prime the loop
	vec_uint4 halves = *src++;
	vec_uint4 manExp = spu_andc(halves, sign16);
	// Unpack signs
	vec_uint4 signsXY = spu_shuffle(halves, halves, unpackXYShortsUp);
	vec_uint4 signsZW = spu_shuffle(halves, halves, unpackZWShortsUp);
	vec_uint4 intsXY = spu_shuffle(manExp, manExp, unpackXYShorts);
	vec_uint4 intsZW = spu_shuffle(manExp, manExp, unpackZWShorts);
	vec_uint4 predNotDenorm = (vec_uint4)spu_cmpgt((vec_ushort8)manExp, mantissa16);

	// Shift mantissa and exponents up to right place.
	vec_uint4 resultXY = spu_sl(intsXY, 13);
	vec_uint4 resultZW = spu_sl(intsZW, 13);

	// Prefetch for next loop iteration.
	vec_uint4 nextHalves = *src++;

	// Unpack the floats (8 at a time).
	// 15 cycle loop, giving performance of just over 1.7 billion floats per second per spu. (3.2GHz clock).
	// Cycle counts obtained from spusim in PASuite.
	const vec_uint4 *const srcEnd = src + numQwordsIn;
	while (src != srcEnd)
	{
		// Prefetch for next next loop iteration...!
		vec_uint4 nextNextHalves = *src++;

		// Bias the exponents.
		resultXY = spu_add(resultXY, (112 << 23));
		resultZW = spu_add(resultZW, (112 << 23));

		// Check for exponent > 0 - this implies not denormalized
		// NB. If exponent == 31 - this implies Inf or NaN - unsupported
		vec_uint4 predNotDenormXY = spu_shuffle(predNotDenorm, predNotDenorm, expandXYshorts);
		vec_uint4 predNotDenormZW = spu_shuffle(predNotDenorm, predNotDenorm, expandZWshorts);

		// 10 is length of mantissa, 15 is exponent bias (log2(16384)).  Exponent is zero for denorm, sign is masked.
		vec_float4 denormXY = spu_convtf(intsXY, 10+15);
		vec_float4 denormZW = spu_convtf(intsZW, 10+15);

		// Recombine the sign bit, select denormal result or not.
		resultXY = spu_sel(spu_sel((vec_uint4)denormXY, resultXY, predNotDenormXY), signsXY, sign32);
		resultZW = spu_sel(spu_sel((vec_uint4)denormZW, resultZW, predNotDenormZW), signsZW, sign32);

		// Output floats
		*dst++ = (vec_float4)resultXY;
		*dst++ = (vec_float4)resultZW;

		// Do processing for next loop iteration.
		manExp = spu_andc(nextHalves, sign16);
		// Unpack signs
		signsXY = spu_shuffle(nextHalves, nextHalves, unpackXYShortsUp);
		signsZW = spu_shuffle(nextHalves, nextHalves, unpackZWShortsUp);
		intsXY = spu_shuffle(manExp, manExp, unpackXYShorts);
		intsZW = spu_shuffle(manExp, manExp, unpackZWShorts);
		predNotDenorm = (vec_uint4)spu_cmpgt((vec_ushort8)manExp, spu_splats((unsigned short)0x03FF));

		// Shift mantissa and exponents up to right place.
		resultXY = spu_sl(intsXY, 13);
		resultZW = spu_sl(intsZW, 13);

		nextHalves = nextNextHalves;
	}
}

// Description:
// Pack an array of 11_11_10 into 3D floats.
// Arguments:
// dst - Destination buffer for 3D floats.
// src - Source buffer for 11_11_10 values.
// num - Number of vertices to convert
static inline void pack11_11_10To3Floats(vec_float4 * __restrict__ dst, const vec_int4 *src, int num)
{
	vec_uchar16 shuffWord04x1 = s_shuffWord04x1;
	vec_uchar16 shuffWord0143 = s_shuffWord0143;
	vec_uchar16 shuffWord5x26 = s_shuffWord5x26;
	vec_uchar16 shuffWord0523 = s_shuffWord0523;
	vec_uchar16 shuffWordx37x = s_shuffWordx37x;
	vec_uchar16 shuffWord6127 = s_shuffWord6127;

	// masks to extract xyz values;
	vec_uint4 maskX = spu_splats((unsigned int)0x000007ff);
	vec_uint4 maskY = spu_splats((unsigned int)0x003ff800);
	vec_uint4 maskZ = spu_splats((unsigned int)0xFFC00000);

	vec_int4 input = *src++;

	vec_int4 intX = (vec_int4) spu_and((vec_uint4)input,maskX);
	vec_int4 intY = (vec_int4) spu_and((vec_uint4)input,maskY);
	vec_int4 intZ = (vec_int4) spu_and((vec_uint4)input,maskZ);

	input = *src++;

	vec_float4 x = spu_convtf(intX,10); // 1024 * (1<<0)
	vec_float4 y = spu_convtf(intY,21); // 1024 * (1<<11)
	vec_float4 z = spu_convtf(intZ,31); // 512 * (1<<22)

	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");

	// 12 cycle loop
	while (num > 0)
	{
		num -=4;

		vec_float4 vec0  = spu_shuffle(x, y, shuffWord04x1);
		vec0  = spu_shuffle(vec0, z, shuffWord0143);

		vec_float4 vec1  = spu_shuffle(x, y, shuffWord5x26);
		vec1  = spu_shuffle(vec1, z, shuffWord0523);

		vec_float4 vec2  = spu_shuffle(x, y, shuffWordx37x);
		vec2  = spu_shuffle(vec2, z, shuffWord6127);

		*dst++ = vec0;
		*dst++ = vec1;
		*dst++ = vec2;

		intX = (vec_int4) spu_and((vec_uint4)input, maskX);
		intY = (vec_int4) spu_and((vec_uint4)input, maskY);
		intZ = (vec_int4) spu_and((vec_uint4)input, maskZ);

		input = *src++;

		x = spu_convtf(intX,10); // 1024 * (1<<0)
		y = spu_convtf(intY,21); // 1024 * (1<<11)
		z = spu_convtf(intZ,31); // 512 * (1<<22)
	}

}

} // namespace rage

#endif // __SPU
#endif // SYSTEM_SPU_DECOMPRESS_H
