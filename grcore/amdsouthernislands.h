//
// grcore/amdsouthernislands.h
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_AMDSOUTHERNISLANDS_H
#define GRCORE_AMDSOUTHERNISLANDS_H

#if RSG_ORBIS
#	include <sdk_version.h>
#endif



////////////////////////////////////////////////////////////////////////////////
//  Raw buffer resource macros
////////////////////////////////////////////////////////////////////////////////

// BASE_ADDRESS     Byte address.
#define AMDSISLANDS_BUFFER_RESOURCE_U32_0(BASE_ADDRESS)                        \
	((rage::u32)(                                                              \
		(0xffffffffu & ((rage::uptr)(BASE_ADDRESS)))))

// BASE_ADDRESS     Byte address.
// STRIDE           Bytes 0 to 16383.
// CACHE_SWIZZLE    Buffer access. Optionally, swizzle texture cache TC L1 cache banks.
// SWIZZLE_ENABLE   Swizzle AOS according to stride, index_stride, and element_size, else linear (stride * index + offset).
#define AMDSISLANDS_BUFFER_RESOURCE_U32_1(BASE_ADDRESS, STRIDE, CACHE_SWIZZLE, SWIZZLE_ENABLE) \
	((rage::u32)(                                                              \
		(0x0000ffffu & ((rage::uptr)(BASE_ADDRESS) >> 32))                     \
	  | (0x3fff0000u & ((STRIDE) << 16))                                       \
	  | (0x40000000u & ((CACHE_SWIZZLE) << 30))                                \
	  | (0x80000000u & ((SWIZZLE_ENABLE) << 31))))

// NUM_RECORDS      In units of stride.
#define AMDSISLANDS_BUFFER_RESOURCE_U32_2(NUM_RECORDS)                         \
	((rage::u32)(NUM_RECORDS))

// DST_SEL_X        Destination channel select.
// DST_SEL_Y        Destination channel select.
// DST_SEL_Z        Destination channel select.
// DST_SEL_W        Destination channel select.
// NUM_FORMAT       Numeric data type (float, int, ...).
// DATA_FORMAT      Number of fields and size of each field.
// ELEMENT_SIZE     2, 4, 8, or 16 bytes (NI = 4). Used for swizzled buffer addressing.
// INDEX_STRIDE     8, 16, 32, or 64 (NI = 16). Used for swizzled buffer addressing.
// ADD_TID_ENABLE   Add thread ID to the index for to calculate the address.
// ATC              .
// HASH_ENABLE      1 = buffer addresses are hashed for better cache performance.
// HEAP             1 = buffer is a heap. out-of-range if offset = 0 or >= num_records.
// MTYPE            .
#define AMDSISLANDS_BUFFER_RESOURCE_U32_3(DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W, NUM_FORMAT, DATA_FORMAT, ELEMENT_SIZE, INDEX_STRIDE, ADD_TID_ENABLE, ATC, HASH_ENABLE, HEAP, MTYPE) \
	((rage::u32)(                                                              \
		(0x00000007u & (DST_SEL_X))                                            \
	  | (0x00000038u & ((DST_SEL_Y) << 3))                                     \
	  | (0x000001c0u & ((DST_SEL_Z) << 6))                                     \
	  | (0x00000e00u & ((DST_SEL_W) << 9))                                     \
	  | (0x00007000u & ((NUM_FORMAT) << 12))                                   \
	  | (0x00078000u & ((DATA_FORMAT) << 15))                                  \
	  | (0x00180000u & ((ELEMENT_SIZE) << 19))                                 \
	  | (0x00600000u & ((INDEX_STRIDE) << 21))                                 \
	  | (0x00800000u & ((ADD_TID_ENABLE) << 23))                               \
	  | (0x01000000u & ((ATC) << 24))                                          \
	  | (0x02000000u & ((HASH_ENABLE) << 25))                                  \
	  | (0x04000000u & ((HEAP) << 26))                                         \
	  | (0x38000000u & ((MTYPE) << 27))                                        \
	  | (0xc0000000u & (/*TYPE*/0 << 30))))



////////////////////////////////////////////////////////////////////////////////
//  Raw image resource macros
////////////////////////////////////////////////////////////////////////////////

//
// 128-bit Resource: 1D-tex, 2d-tex, 2d-msaa (multi-sample auto-aliasing)
//

// BASE_ADDRESS     256-byte aligned. Also used for fmask-ptr.
#define AMDSISLANDS_IMAGE_RESOURCE_128_U32_0(BASE_ADDRESS)                     \
	((rage::u32)(                                                              \
		(0xffffffffu & ((rage::uptr)(BASE_ADDRESS)))

// BASE_ADDRESS     256-byte aligned. Also used for fmask-ptr.
// MIN_LOD          4.8 (four uint bits, eight fraction bits) format.
// DATA_FORMAT      Number of comps, number of bits/comp.
// NUM_FORMAT       Numeric format.
// MTYPE            .
#define AMDSISLANDS_IMAGE_RESOURCE_128_U32_1(BASE_ADDRESS, MIN_LOD, DATA_FORMAT, NUM_FORMAT, MTYPE) \
	((rage::u32)(                                                              \
		(0x000000ffu & ((rage::uptr)(BASE_ADDRESS) >> 32))                     \
	  | (0x000fff00u & ((MIN_LOD) << 8))                                       \
	  | (0x03f00000u & ((DATA_FORMAT) << 20))                                  \
	  | (0x3c000000u & ((NUM_FORMAT) << 26))                                   \
	  | (0xc0000000u & ((MTYPE) << 30))))

// WIDTH            .
// HEIGHT           .
// PERF_MODULATION  Scales sampler's perf_z, perf_mip, aniso_bias, lod_bias_sec.
// INTERLACED       .
#define AMDSISLANDS_IMAGE_RESOURCE_128_U32_2(WIDTH, HEIGHT, PERF_MODULATION, INTERLACED) \
	((rage::u32)(                                                              \
		(0x00003fffu & (WIDTH))                                                \
	  | (0x0fffc000u & ((HEIGHT) << 14))                                       \
	  | (0x70000000u & ((PERF_MODULATION) << 28))                              \
	  | (0x80000000u & ((INTERLACED) << 31))))

// DST_SEL_X        Destination channel select.
// DST_SEL_Y        Destination channel select.
// DST_SEL_Z        Destination channel select.
// DST_SEL_W        Destination channel select.
// BASE_LEVEL       .
// LAST_LEVEL       For msaa, holds number of samples.
// TILING_INDEX     Lookuptable: 32 x 16
//                  bank_width[2], bank_height[2], num_banks[2],
//                  tile_split[2], macro_tile_aspect[2],
//                  micro_tile_mode[2], array_mode[4].
// MTYPE            .
// ATC              .
// POW2PAD          Memory footprint is padded to pow2 dimensions
#define AMDSISLANDS_IMAGE_RESOURCE_128_U32_3(DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W, BASE_LEVEL, LAST_LEVEL, TILING_INDEX, POW2PAD, MTYPE, ATC, TYPE) \
	((rage::u32)(                                                              \
		(0x00000007u & (DST_SEL_X))                                            \
	  | (0x00000038u & ((DST_SEL_Y) << 3))                                     \
	  | (0x000001c0u & ((DST_SEL_Z) << 6))                                     \
	  | (0x00000e00u & ((DST_SEL_W) << 9))                                     \
	  | (0x0000f000u & ((BASE_LEVEL) << 12))                                   \
	  | (0x000f0000u & ((LAST_LEVEL) << 16))                                   \
	  | (0x01f00000u & ((TILING_INDEX) << 20))                                 \
	  | (0x02000000u & ((POW2PAD) << 25))                                      \
	  | (0x04000000u & ((MTYPE) << 26))                                        \
	  | (0x08000000u & ((ATC) << 27))                                          \
	  | (0xf0000000u & ((TYPE) << 28))))

//
// 256-bit Resource: 1d-array, 2d-array, 3d, cubemap, MSAA
//

// DEPTH            .
// PITCH            In texel units.
#define AMDSISLANDS_IMAGE_RESOURCE_256_U32_4(DEPTH, PITCH)                     \
	((rage::u32)(                                                              \
		(0x00001fffu & (DEPTH))                                                \
	  | (0x07ffe000u & ((PITCH) << 13))                                        \
	  | (0xf8000000u & (/*unused*/0 << 27))))

// BASE_ARRAY       .
// LAST_ARRAY       .
#define AMDSISLANDS_IMAGE_RESOURCE_256_U32_5(BASE_ARRAY, LAST_ARRAY)           \
	((rage::u32)(                                                              \
		(0x00001fffu & (BASE_ARRAY))                                           \
	  | (0x03ffe000u & ((LAST_ARRAY) << 13))                                   \
	  | (0xfc000000u & (/*unused*/0 << 26))))

// MIN_LOD_WARN     Feedback trigger for lod.
// COUNTER_BANK_ID  .
// LOD_HDW_CNT_EN   .
#define AMDSISLANDS_IMAGE_RESOURCE_256_U32_6(MIN_LOD_WARN, COUNTER_BANK_ID, LOD_HDW_CNT_EN) \
	((rage::u32)(                                                              \
		(0x00000fffu & (MIN_LOD_WARN))                                         \
		(0x000ff000u & (COUNTER_BANK_ID))                                      \
		(0x00100000u & (LOD_HDW_CNT_EN))                                       \
	  | (0xffe00000u & (/*unused*/0 << 21))))

#define AMDSISLANDS_IMAGE_RESOURCE_256_U32_7()                                 \
	((rage::u32)0)



////////////////////////////////////////////////////////////////////////////////
// Defined values for fields
////////////////////////////////////////////////////////////////////////////////

// DST_SEL_* values
#define AMDSISLANDS_DST_SEL_0                       0
#define AMDSISLANDS_DST_SEL_1                       1
#define AMDSISLANDS_DST_SEL_R                       4
#define AMDSISLANDS_DST_SEL_G                       5
#define AMDSISLANDS_DST_SEL_B                       6
#define AMDSISLANDS_DST_SEL_A                       7

// NUM_FORMAT values valid in buffer and image resources
#define AMDSISLANDS_NUM_FORMAT_UNORM                0
#define AMDSISLANDS_NUM_FORMAT_SNORM                1
#define AMDSISLANDS_NUM_FORMAT_USCALED              2
#define AMDSISLANDS_NUM_FORMAT_SSCALED              3
#define AMDSISLANDS_NUM_FORMAT_UINT                 4
#define AMDSISLANDS_NUM_FORMAT_SINT                 5
#define AMDSISLANDS_NUM_FORMAT_SNORM_NZ             6
#define AMDSISLANDS_NUM_FORMAT_FLOAT                7

// Additional NUM_FORMAT values valid in image resource only
#define AMDSISLANDS_NUM_FORMAT_SRGB                 9
#define AMDSISLANDS_NUM_FORMAT_UBNORM               10
#define AMDSISLANDS_NUM_FORMAT_UBNORM_NZ            11
#define AMDSISLANDS_NUM_FORMAT_UBINT                12
#define AMDSISLANDS_NUM_FORMAT_UBSCALED             13

// DATA_FORMAT values
#define AMDSISLANDS_DATA_FORMAT_8                   1
#define AMDSISLANDS_DATA_FORMAT_16                  2
#define AMDSISLANDS_DATA_FORMAT_8_8                 3
#define AMDSISLANDS_DATA_FORMAT_32                  4
#define AMDSISLANDS_DATA_FORMAT_16_16               5
#define AMDSISLANDS_DATA_FORMAT_10_11_11            6
#define AMDSISLANDS_DATA_FORMAT_11_11_10            7
#define AMDSISLANDS_DATA_FORMAT_10_10_10_2          8
#define AMDSISLANDS_DATA_FORMAT_2_10_10_10          9
#define AMDSISLANDS_DATA_FORMAT_8_8_8_8             10
#define AMDSISLANDS_DATA_FORMAT_32_32               11
#define AMDSISLANDS_DATA_FORMAT_16_16_16_16         12
#define AMDSISLANDS_DATA_FORMAT_32_32_32            13
#define AMDSISLANDS_DATA_FORMAT_32_32_32_32         14

// From Orbis SDK 930 onwards, Sony changed MTYPE value.
#if RSG_ORBIS && SCE_ORBIS_SDK_VERSION>=0x00930000u
#	define AMDSISLANDS_ORBIS_BUFFER_MTYPE   (4)
#else
#	define AMDSISLANDS_ORBIS_BUFFER_MTYPE   (0)
#endif



////////////////////////////////////////////////////////////////////////////////
// Helper macros that to make things less verbose
////////////////////////////////////////////////////////////////////////////////

//
// Constant buffers
//

// BASE_ADDRESS     Byte address.
#define AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_0(BASE_ADDRESS)               \
	AMDSISLANDS_BUFFER_RESOURCE_U32_0(BASE_ADDRESS)

// BASE_ADDRESS     Byte address.
#define AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_1(BASE_ADDRESS)               \
	AMDSISLANDS_BUFFER_RESOURCE_U32_1(                                         \
		BASE_ADDRESS,                                                          \
		16,                                     /*STRIDE*/                     \
		0,                                      /*CACHE_SWIZZLE*/              \
		0)                                      /*SWIZZLE_ENABLE*/

// NUM_RECORDS      In units of stride.
#define AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_2(NUM_RECORDS)                \
	AMDSISLANDS_BUFFER_RESOURCE_U32_2(NUM_RECORDS)

#define AMDSISLANDS_CONSTANT_BUFFER_RESOURCE_U32_3()                           \
	AMDSISLANDS_BUFFER_RESOURCE_U32_3(                                         \
		AMDSISLANDS_DST_SEL_R,                  /*DST_SEL_X*/                  \
		AMDSISLANDS_DST_SEL_G,                  /*DST_SEL_Y*/                  \
		AMDSISLANDS_DST_SEL_B,                  /*DST_SEL_Z*/                  \
		AMDSISLANDS_DST_SEL_A,                  /*DST_SEL_W*/                  \
		AMDSISLANDS_NUM_FORMAT_FLOAT,           /*NUM_FORMAT*/                 \
		AMDSISLANDS_DATA_FORMAT_32_32_32_32,    /*DATA_FORMAT*/                \
		0,                                      /*ELEMENT_SIZE*/               \
		0,                                      /*INDEX_STRIDE*/               \
		0,                                      /*ADD_TID_ENABLE*/             \
		0,                                      /*ATC*/                        \
		0,                                      /*HASH_ENABLE*/                \
		0,                                      /*HEAP*/                       \
		AMDSISLANDS_ORBIS_BUFFER_MTYPE)         /*MTYPE*/


//
// Vertex buffers
//

// BASE_ADDRESS     Byte address.
#define AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_0(BASE_ADDRESS)                 \
	AMDSISLANDS_BUFFER_RESOURCE_U32_0(BASE_ADDRESS)

// BASE_ADDRESS     Byte address.
// STRIDE           Bytes 0 to 16383.
#define AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_1(BASE_ADDRESS, STRIDE)         \
	AMDSISLANDS_BUFFER_RESOURCE_U32_1(                                         \
		BASE_ADDRESS,                                                          \
		STRIDE,                                                                \
		0,                                      /*CACHE_SWIZZLE*/              \
		0)                                      /*SWIZZLE_ENABLE*/

// NUM_RECORDS      In units of stride.
#define AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_2(NUM_RECORDS)                  \
	AMDSISLANDS_BUFFER_RESOURCE_U32_2(NUM_RECORDS)

// NUM_FORMAT       Numeric data type (float, int, ...).
// DATA_FORMAT      Number of fields and size of each field.
// DST_SEL_X        Destination channel select (specified with single character [R,G,B,A,0,1]).
// DST_SEL_Y        Destination channel select (specified with single character [R,G,B,A,0,1]).
// DST_SEL_Z        Destination channel select (specified with single character [R,G,B,A,0,1]).
// DST_SEL_W        Destination channel select (specified with single character [R,G,B,A,0,1]).
#define AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(NUM_FORMAT, DATA_FORMAT, DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W) \
	AMDSISLANDS_BUFFER_RESOURCE_U32_3(                                         \
		AMDSISLANDS_DST_SEL_##DST_SEL_X,                                       \
		AMDSISLANDS_DST_SEL_##DST_SEL_Y,                                       \
		AMDSISLANDS_DST_SEL_##DST_SEL_Z,                                       \
		AMDSISLANDS_DST_SEL_##DST_SEL_W,                                       \
		AMDSISLANDS_NUM_FORMAT_##NUM_FORMAT,                                   \
		AMDSISLANDS_DATA_FORMAT_##DATA_FORMAT,                                 \
		0,                                      /*ELEMENT_SIZE*/               \
		0,                                      /*INDEX_STRIDE*/               \
		0,                                      /*ADD_TID_ENABLE*/             \
		0,                                      /*ATC*/                        \
		0,                                      /*HASH_ENABLE*/                \
		0,                                      /*HEAP*/                       \
		AMDSISLANDS_ORBIS_BUFFER_MTYPE)         /*MTYPE*/

// The following macros are not intended to be used directly, they are just used
// to implement AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_3.

#define AMDSISLANDS__DEFAULT_DST_SEL_X_8            R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_8            0
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_8            0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_8            1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_16           R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_16           0
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_16           0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_16           1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_8_8          R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_8_8          G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_8_8          0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_8_8          1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_32           R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_32           0
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_32           0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_32           1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_16_16        R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_16_16        G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_16_16        0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_16_16        1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_10_11_11     R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_10_11_11     G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_10_11_11     B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_10_11_11     1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_11_11_10     R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_11_11_10     G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_11_11_10     B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_11_11_10     1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_10_10_10_2   R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_10_10_10_2   G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_10_10_10_2   B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_10_10_10_2   A

#define AMDSISLANDS__DEFAULT_DST_SEL_X_2_10_10_10   R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_2_10_10_10   G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_2_10_10_10   B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_2_10_10_10   A

#define AMDSISLANDS__DEFAULT_DST_SEL_X_8_8_8_8      R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_8_8_8_8      G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_8_8_8_8      B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_8_8_8_8      A

#define AMDSISLANDS__DEFAULT_DST_SEL_X_32_32        R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_32_32        G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_32_32        0
#define AMDSISLANDS__DEFAULT_DST_SEL_W_32_32        1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_16_16_16_16  R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_16_16_16_16  G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_16_16_16_16  B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_16_16_16_16  A

#define AMDSISLANDS__DEFAULT_DST_SEL_X_32_32_32     R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_32_32_32     G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_32_32_32     B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_32_32_32     1

#define AMDSISLANDS__DEFAULT_DST_SEL_X_32_32_32_32  R
#define AMDSISLANDS__DEFAULT_DST_SEL_Y_32_32_32_32  G
#define AMDSISLANDS__DEFAULT_DST_SEL_Z_32_32_32_32  B
#define AMDSISLANDS__DEFAULT_DST_SEL_W_32_32_32_32  A

#define AMDSISLANDS__VERTEX_BUFFER_RESOURCE_REMAP_EVAL_2_U32_3(NUM_FORMAT, DATA_FORMAT, DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W) \
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(NUM_FORMAT, DATA_FORMAT, DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W)

#define AMDSISLANDS__VERTEX_BUFFER_RESOURCE_REMAP_EVAL_1_U32_3(NUM_FORMAT, DATA_FORMAT, DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W) \
	AMDSISLANDS__VERTEX_BUFFER_RESOURCE_REMAP_EVAL_2_U32_3(NUM_FORMAT, DATA_FORMAT, DST_SEL_X, DST_SEL_Y, DST_SEL_Z, DST_SEL_W)

// More simplified version of AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3
// where the remap values are implied from defaults.
//
// NUM_FORMAT       Numeric data type (float, int, ...).
// DATA_FORMAT      Number of fields and size of each field.
#define AMDSISLANDS_VERTEX_BUFFER_RESOURCE_U32_3(NUM_FORMAT, DATA_FORMAT)      \
	AMDSISLANDS__VERTEX_BUFFER_RESOURCE_REMAP_EVAL_1_U32_3(                    \
		NUM_FORMAT,                                                            \
		DATA_FORMAT,                                                           \
		AMDSISLANDS__DEFAULT_DST_SEL_X_##DATA_FORMAT,                          \
		AMDSISLANDS__DEFAULT_DST_SEL_Y_##DATA_FORMAT,                          \
		AMDSISLANDS__DEFAULT_DST_SEL_Z_##DATA_FORMAT,                          \
		AMDSISLANDS__DEFAULT_DST_SEL_W_##DATA_FORMAT)



#endif // GRCORE_AMDSOUTHERNISLANDS_H
