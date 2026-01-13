//
// grcore/wrapper_gcm.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//
#ifndef GRCORE_WRAPPER_GCM_H
#define GRCORE_WRAPPER_GCM_H

#define CELL_GCM_ENUM_INVALID (static_cast<CellGcmEnum>(0xffffffff))

struct CellGcmTexture;

struct PackedCellGcmTexture {
	rage::u32 format;		// ((location) + 1) | ((cubemap) << 2) | ((border) << 3) | ((dimension) << 4) | ((format) << 8) | ((mipmap) << 16) )
	// upper 12 bits are available for our use if we mask them off.
	// 31:28 bits taken from upper 4 bits of _padding (already shifted to correct spot)
	// 27:24 bits are log2(depth)
	// 23:20 bits taken from lower 4 bits of _padding (also already shifted to correct spot)

	rage::u32 imagerect;		// six bits free here if necessary (1-4096 only needs 13 bits)

	rage::u16 remap;			// upper 16 bits always clear?
	rage::u16 pitch;			// probably some free bits here too.

	rage::u32 offset;

	void PackFrom(const CellGcmTexture&);
	void UnpackTo(CellGcmTexture&);
};

#if __SPU
struct CellGcmContextData;
int32_t gcmCallback(CellGcmContextData *data,uint32_t /*amt*/);
#endif

#if __PPU || __WIN32 || RSG_ORBIS		// TODO: Do we really want to keep the CellGcmTexture in the base class?

#include "data/struct.h"
#include "system/memory.h"
#include "system/new.h"
#include "atl/bitset.h"
#include "grprofile/pix.h"
#include "grcore/channel.h"
#include "system/spurs_gcm_config.h"

#if __PPU

#if !__OPTIMIZED
// #define CELL_GCM_DEBUG -- this is PAINFULLY slow
#endif

// Enable this when linking with the hud.
// You also need to change "-lgcm_cmd -lgcm_sys_stub" to "-lgcm_hud -lsheap_stub" in the Linker Inputs pane.

//#define GCM_HUD
// GCM_HUD is now defined in system/pix.h

// Enable this to link with gcm replay.  Need to add -lcapture to Linker Inputs pane.
#define GCM_REPLAY (((__DEV ||__BANK) && __OPTIMIZED) || __PROFILE)

#define CELL_GCM_DENY_IMPLICIT_ARG
#include <cell/gcm.h>
#if GCM_HUD
#undef GCM_PF
#include <cell/gcm_hud.h>
#endif

#else	// !__PPU

namespace rage {
	class datTypeStruct;
};

typedef rage::u8 uint8_t;
typedef rage::u16 uint16_t;
typedef rage::u32 uint32_t;

struct CellGcmTexture {
	uint8_t format;
	uint8_t mipmap;
	uint8_t dimension;
	uint8_t cubemap;

	uint32_t remap;

	uint16_t width;
	uint16_t height;
	uint16_t depth;
	uint8_t location;
	uint8_t _padding;

	uint32_t pitch;
	uint32_t offset;

#if __DECLARESTRUCT
	void DeclareStruct(rage::datTypeStruct &S);
#endif
};

enum CellGcmEnum {
	//	Enable
	CELL_GCM_FALSE	= (0),
	CELL_GCM_TRUE	= (1),

	// Location
	CELL_GCM_LOCATION_LOCAL	= (0),
	CELL_GCM_LOCATION_MAIN	= (1),


	// SetSurface
	CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5	= (1),
	CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5	= (2),
	CELL_GCM_SURFACE_R5G6B5				= (3),
	CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8	= (4),
	CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8	= (5),
	CELL_GCM_SURFACE_A8R8G8B8			= (8),
	CELL_GCM_SURFACE_B8					= (9),
	CELL_GCM_SURFACE_G8B8				= (10),
	CELL_GCM_SURFACE_F_W16Z16Y16X16		= (11),
	CELL_GCM_SURFACE_F_W32Z32Y32X32		= (12),
	CELL_GCM_SURFACE_F_X32				= (13),
	CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8	= (14),
	CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8	= (15),
	CELL_GCM_SURFACE_A8B8G8R8			= (16),

	CELL_GCM_SURFACE_Z16	= (1),
	CELL_GCM_SURFACE_Z24S8	= (2),

	CELL_GCM_SURFACE_PITCH		= (1),
	CELL_GCM_SURFACE_SWIZZLE	= (2),

	CELL_GCM_SURFACE_CENTER_1				= (0),
	CELL_GCM_SURFACE_DIAGONAL_CENTERED_2	= (3),
	CELL_GCM_SURFACE_SQUARE_CENTERED_4		= (4),
	CELL_GCM_SURFACE_SQUARE_ROTATED_4		= (5),

	CELL_GCM_SURFACE_TARGET_NONE	= (0),
	CELL_GCM_SURFACE_TARGET_0		= (1),
	CELL_GCM_SURFACE_TARGET_1		= (2),
	CELL_GCM_SURFACE_TARGET_MRT1	= (0x13),
	CELL_GCM_SURFACE_TARGET_MRT2	= (0x17),
	CELL_GCM_SURFACE_TARGET_MRT3	= (0x1f),

	// SetClearSurface
	CELL_GCM_CLEAR_Z	= (1<<0),
	CELL_GCM_CLEAR_S	= (1<<1),
	CELL_GCM_CLEAR_R	= (1<<4),
	CELL_GCM_CLEAR_G	= (1<<5),
	CELL_GCM_CLEAR_B	= (1<<6),
	CELL_GCM_CLEAR_A	= (1<<7),
	CELL_GCM_CLEAR_M	= (0xf3),

	// SetVertexDataArray
	CELL_GCM_VERTEX_S1		= (1),
	CELL_GCM_VERTEX_F		= (2),
	CELL_GCM_VERTEX_SF		= (3),
	CELL_GCM_VERTEX_UB		= (4),
	CELL_GCM_VERTEX_S32K	= (5),
	CELL_GCM_VERTEX_CMP		= (6),
	CELL_GCM_VERTEX_UB256	= (7),

	CELL_GCM_VERTEX_S16_NR		            = (1),
	CELL_GCM_VERTEX_F32		                = (2),
	CELL_GCM_VERTEX_F16		                = (3),
	CELL_GCM_VERTEX_U8_NR		            = (4),
	CELL_GCM_VERTEX_S16_UN	                = (5),
	CELL_GCM_VERTEX_S11_11_10_NR		    = (6),
	CELL_GCM_VERTEX_U8_UN	                = (7),

	// SetTexture
	CELL_GCM_TEXTURE_B8						= (0x81),
	CELL_GCM_TEXTURE_A1R5G5B5				= (0x82),
	CELL_GCM_TEXTURE_A4R4G4B4				= (0x83),
	CELL_GCM_TEXTURE_R5G6B5					= (0x84),
	CELL_GCM_TEXTURE_A8R8G8B8				= (0x85),
	CELL_GCM_TEXTURE_COMPRESSED_DXT1		= (0x86),
	CELL_GCM_TEXTURE_COMPRESSED_DXT23		= (0x87),
	CELL_GCM_TEXTURE_COMPRESSED_DXT45		= (0x88),
	CELL_GCM_TEXTURE_G8B8					= (0x8B),
	CELL_GCM_TEXTURE_R6G5B5					= (0x8F),
	CELL_GCM_TEXTURE_DEPTH24_D8				= (0x90),
	CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT		= (0x91),
	CELL_GCM_TEXTURE_DEPTH16				= (0x92),
	CELL_GCM_TEXTURE_DEPTH16_FLOAT			= (0x93),
	CELL_GCM_TEXTURE_X16					= (0x94),
	CELL_GCM_TEXTURE_Y16_X16				= (0x95),
	CELL_GCM_TEXTURE_R5G5B5A1				= (0x97),
	CELL_GCM_TEXTURE_COMPRESSED_HILO8		= (0x98),
	CELL_GCM_TEXTURE_COMPRESSED_HILO_S8		= (0x99),
	CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT	= (0x9A),
	CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT	= (0x9B),
	CELL_GCM_TEXTURE_X32_FLOAT				= (0x9C),
	CELL_GCM_TEXTURE_D1R5G5B5				= (0x9D),
	CELL_GCM_TEXTURE_D8R8G8B8				= (0x9E),
	CELL_GCM_TEXTURE_Y16_X16_FLOAT			= (0x9F),
	CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8	= (0xAD),
	CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8	= (0xAE),

	CELL_GCM_TEXTURE_SZ	= (0x00),
	CELL_GCM_TEXTURE_LN	= (0x20),
	CELL_GCM_TEXTURE_NR	= (0x00),
	CELL_GCM_TEXTURE_UN	= (0x40),

	CELL_GCM_TEXTURE_DIMENSION_1		= (1),
	CELL_GCM_TEXTURE_DIMENSION_2		= (2),
	CELL_GCM_TEXTURE_DIMENSION_3		= (3),

	CELL_GCM_TEXTURE_REMAP_ORDER_XYXY	= (0),
	CELL_GCM_TEXTURE_REMAP_ORDER_XXXY	= (1),
	CELL_GCM_TEXTURE_REMAP_FROM_A		= (0),
	CELL_GCM_TEXTURE_REMAP_FROM_R		= (1),
	CELL_GCM_TEXTURE_REMAP_FROM_G		= (2),
	CELL_GCM_TEXTURE_REMAP_FROM_B		= (3),
	CELL_GCM_TEXTURE_REMAP_ZERO			= (0),
	CELL_GCM_TEXTURE_REMAP_ONE			= (1),
	CELL_GCM_TEXTURE_REMAP_REMAP		= (2),

	// SetTextureFilter
	CELL_GCM_TEXTURE_NEAREST			= (1),
	CELL_GCM_TEXTURE_LINEAR				= (2),
	CELL_GCM_TEXTURE_NEAREST_NEAREST	= (3),
	CELL_GCM_TEXTURE_LINEAR_NEAREST		= (4),
	CELL_GCM_TEXTURE_NEAREST_LINEAR		= (5),
	CELL_GCM_TEXTURE_LINEAR_LINEAR		= (6),
	CELL_GCM_TEXTURE_CONVOLUTION_MIN	= (7),
	CELL_GCM_TEXTURE_CONVOLUTION_MAG	= (4),
	CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX		= (1),
	CELL_GCM_TEXTURE_CONVOLUTION_GAUSSIAN		= (2),
	CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX_ALT	= (3),

	// SetTextureAddress
	CELL_GCM_TEXTURE_WRAP						= (1),
	CELL_GCM_TEXTURE_MIRROR						= (2),
	CELL_GCM_TEXTURE_CLAMP_TO_EDGE				= (3),
	CELL_GCM_TEXTURE_BORDER						= (4),
	CELL_GCM_TEXTURE_CLAMP						= (5),
	CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE	= (6),
	CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER			= (7),
	CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP			= (8),

	CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL	= (0),
	CELL_GCM_TEXTURE_UNSIGNED_REMAP_BIASED	= (1),

	CELL_GCM_TEXTURE_ZFUNC_NEVER	= (0),
	CELL_GCM_TEXTURE_ZFUNC_LESS		= (1),
	CELL_GCM_TEXTURE_ZFUNC_EQUAL	= (2),
	CELL_GCM_TEXTURE_ZFUNC_LEQUAL	= (3),
	CELL_GCM_TEXTURE_ZFUNC_GREATER	= (4),
	CELL_GCM_TEXTURE_ZFUNC_NOTEQUAL	= (5),
	CELL_GCM_TEXTURE_ZFUNC_GEQUAL	= (6),
	CELL_GCM_TEXTURE_ZFUNC_ALWAYS	= (7),

	CELL_GCM_TEXTURE_GAMMA_R	= (1<<0),
	CELL_GCM_TEXTURE_GAMMA_G	= (1<<1),
	CELL_GCM_TEXTURE_GAMMA_B	= (1<<2),
	CELL_GCM_TEXTURE_GAMMA_A	= (1<<3),

	// SetTextureControl
	CELL_GCM_TEXTURE_MAX_ANISO_1	= (0),
	CELL_GCM_TEXTURE_MAX_ANISO_2	= (1),
	CELL_GCM_TEXTURE_MAX_ANISO_4	= (2),
	CELL_GCM_TEXTURE_MAX_ANISO_6	= (3),
	CELL_GCM_TEXTURE_MAX_ANISO_8	= (4),
	CELL_GCM_TEXTURE_MAX_ANISO_10	= (5),
	CELL_GCM_TEXTURE_MAX_ANISO_12	= (6),
	CELL_GCM_TEXTURE_MAX_ANISO_16	= (7),

	// SetDrawArrays, SetDrawIndexArray
	CELL_GCM_PRIMITIVE_POINTS			= (1),
	CELL_GCM_PRIMITIVE_LINES			= (2),
	CELL_GCM_PRIMITIVE_LINE_LOOP		= (3),
	CELL_GCM_PRIMITIVE_LINE_STRIP		= (4),
	CELL_GCM_PRIMITIVE_TRIANGLES		= (5),
	CELL_GCM_PRIMITIVE_TRIANGLE_STRIP	= (6),
	CELL_GCM_PRIMITIVE_TRIANGLE_FAN		= (7),
	CELL_GCM_PRIMITIVE_QUADS			= (8),
	CELL_GCM_PRIMITIVE_QUAD_STRIP		= (9),
	CELL_GCM_PRIMITIVE_POLYGON			= (10),

	// SetColorMask
	CELL_GCM_COLOR_MASK_B	= (1<<0),
	CELL_GCM_COLOR_MASK_G	= (1<<8),
	CELL_GCM_COLOR_MASK_R	= (1<<16),
	CELL_GCM_COLOR_MASK_A	= (1<<24),

	// SetColorMaskMrt
	CELL_GCM_COLOR_MASK_MRT1_A	= (1<<4),
	CELL_GCM_COLOR_MASK_MRT1_R	= (1<<5),
	CELL_GCM_COLOR_MASK_MRT1_G	= (1<<6),
	CELL_GCM_COLOR_MASK_MRT1_B	= (1<<7),
	CELL_GCM_COLOR_MASK_MRT2_A	= (1<<8),
	CELL_GCM_COLOR_MASK_MRT2_R	= (1<<9),
	CELL_GCM_COLOR_MASK_MRT2_G	= (1<<10),
	CELL_GCM_COLOR_MASK_MRT2_B	= (1<<11),
	CELL_GCM_COLOR_MASK_MRT3_A	= (1<<12),
	CELL_GCM_COLOR_MASK_MRT3_R	= (1<<13),
	CELL_GCM_COLOR_MASK_MRT3_G	= (1<<14),
	CELL_GCM_COLOR_MASK_MRT3_B	= (1<<15),

	// SetAlphaFunc, DepthFunc, StencilFunc
	CELL_GCM_NEVER		= (0x0200),
	CELL_GCM_LESS		= (0x0201),
	CELL_GCM_EQUAL		= (0x0202),
	CELL_GCM_LEQUAL		= (0x0203),
	CELL_GCM_GREATER	= (0x0204),
	CELL_GCM_NOTEQUAL	= (0x0205),
	CELL_GCM_GEQUAL		= (0x0206),
	CELL_GCM_ALWAYS		= (0x0207),

	// SetBlendFunc
	CELL_GCM_ZERO						= (0),
	CELL_GCM_ONE						= (1),
	CELL_GCM_SRC_COLOR					= (0x0300),
	CELL_GCM_ONE_MINUS_SRC_COLOR		= (0x0301),
	CELL_GCM_SRC_ALPHA					= (0x0302),
	CELL_GCM_ONE_MINUS_SRC_ALPHA		= (0x0303),
	CELL_GCM_DST_ALPHA					= (0x0304),
	CELL_GCM_ONE_MINUS_DST_ALPHA		= (0x0305),
	CELL_GCM_DST_COLOR					= (0x0306),
	CELL_GCM_ONE_MINUS_DST_COLOR		= (0x0307),
	CELL_GCM_SRC_ALPHA_SATURATE			= (0x0308),
	CELL_GCM_CONSTANT_COLOR				= (0x8001),
	CELL_GCM_ONE_MINUS_CONSTANT_COLOR	= (0x8002),
	CELL_GCM_CONSTANT_ALPHA				= (0x8003),
	CELL_GCM_ONE_MINUS_CONSTANT_ALPHA	= (0x8004),

	// SetBlendEquation
	CELL_GCM_BLEND_COLOR			= (0x8005),
	CELL_GCM_FUNC_ADD				= (0x8006),
	CELL_GCM_MIN					= (0x8007),
	CELL_GCM_MAX					= (0x8008),
	CELL_GCM_BLEND_EQUATION			= (0x8009),
	CELL_GCM_FUNC_SUBTRACT			= (0x800A),
	CELL_GCM_FUNC_REVERSE_SUBTRACT	= (0x800B),
	CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED	= (0x0000F005),
	CELL_GCM_FUNC_ADD_SIGNED				= (0x0000F006),
	CELL_GCM_FUNC_REVERSE_ADD_SIGNED		= (0x0000F007),

	// SetCullFace
	CELL_GCM_FRONT			= (0x0404),
	CELL_GCM_BACK			= (0x0405),
	CELL_GCM_FRONT_AND_BACK	= (0x0408),

	// SetShadeMode
	CELL_GCM_FLAT	= (0x1D00),
	CELL_GCM_SMOOTH	= (0x1D01),

	// SetFrontFace
	CELL_GCM_CW		= (0x0900),
	CELL_GCM_CCW	= (0x0901),

	// SetLogicOp
	CELL_GCM_CLEAR			= (0x1500),
	CELL_GCM_AND			= (0x1501),
	CELL_GCM_AND_REVERSE	= (0x1502),
	CELL_GCM_COPY			= (0x1503),
	CELL_GCM_AND_INVERTED	= (0x1504),
	CELL_GCM_NOOP			= (0x1505),
	CELL_GCM_XOR			= (0x1506),
	CELL_GCM_OR				= (0x1507),
	CELL_GCM_NOR			= (0x1508),
	CELL_GCM_EQUIV			= (0x1509),
	CELL_GCM_INVERT			= (0x150A),
	CELL_GCM_OR_REVERSE		= (0x150B),
	CELL_GCM_COPY_INVERTED	= (0x150C),
	CELL_GCM_OR_INVERTED	= (0x150D),
	CELL_GCM_NAND			= (0x150E),
	CELL_GCM_SET			= (0x150F),

	// SetStencilOp
	CELL_GCM_KEEP		= (0x1E00),
	CELL_GCM_REPLACE	= (0x1E01),
	CELL_GCM_INCR		= (0x1E02),
	CELL_GCM_DECR		= (0x1E03),
	CELL_GCM_INCR_WRAP	= (0x8507),
	CELL_GCM_DECR_WRAP	= (0x8508),

	// SetDrawIndexArray
	CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32	= (0),
	CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16	= (1),

	// SetTransfer
	CELL_GCM_TRANSFER_LOCAL_TO_LOCAL	= (0),
	CELL_GCM_TRANSFER_MAIN_TO_LOCAL		= (1),
	CELL_GCM_TRANSFER_LOCAL_TO_MAIN		= (2),

	// SetInvalidateTextureCache
	CELL_GCM_INVALIDATE_TEXTURE			= (1),
	CELL_GCM_INVALIDATE_VERTEX_TEXTURE	= (2),

	// SetFrequencyDividerOperation
	CELL_GCM_FREQUENCY_MODULO	= (1),
	CELL_GCM_FREQUENCY_DIVIDE	= (0),

	// SetTile, SetZCull
	CELL_GCM_COMPMODE_DISABLED					= (0),
	CELL_GCM_COMPMODE_C32_2X1					= (7),
	CELL_GCM_COMPMODE_C32_2X2					= (8),
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL			= (9),
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REG		= (10),
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL_REGULAR	= (10),
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL	= (11),
	CELL_GCM_COMPMODE_Z32_SEPSTENCIL_ROTATED	= (12),

	// SetZCull
	CELL_GCM_ZCULL_Z16		= (1),
	CELL_GCM_ZCULL_Z24S8	= (2),
	CELL_GCM_ZCULL_MSB		= (0),
	CELL_GCM_ZCULL_LONES	= (1),
	CELL_GCM_ZCULL_LESS		= (0),
	CELL_GCM_ZCULL_GREATER	= (1),

	CELL_GCM_SCULL_SFUNC_NEVER          = (0),
	CELL_GCM_SCULL_SFUNC_LESS           = (1),
	CELL_GCM_SCULL_SFUNC_EQUAL          = (2),
	CELL_GCM_SCULL_SFUNC_LEQUAL         = (3),
	CELL_GCM_SCULL_SFUNC_GREATER        = (4),
	CELL_GCM_SCULL_SFUNC_NOTEQUAL       = (5),
	CELL_GCM_SCULL_SFUNC_GEQUAL         = (6),
	CELL_GCM_SCULL_SFUNC_ALWAYS         = (7),

	CELL_GCM_DISPLAY_HSYNC	= (1),
	CELL_GCM_DISPLAY_VSYNC	= (2),

	CELL_GCM_TYPE_B		= (1),
	CELL_GCM_TYPE_C		= (2),
	CELL_GCM_TYPE_RSX	= (3),

	// MRT
	CELL_GCM_MRT_MAXCOUNT	= (4),

	// max display id
	CELL_GCM_DISPLAY_MAXID	= (8),

	// Debug output level
	CELL_GCM_DEBUG_LEVEL0		= (0),
	CELL_GCM_DEBUG_LEVEL1		= (1),
	CELL_GCM_DEBUG_LEVEL2		= (2),

	// SetRenderEnable
	CELL_GCM_CONDITIONAL		= (2),

	// SetClearReport, SetReport, GetReport
	CELL_GCM_ZPASS_PIXEL_CNT	= (1),
	CELL_GCM_ZCULL_STATS		= (2),
	CELL_GCM_ZCULL_STATS1		= (3),
	CELL_GCM_ZCULL_STATS2		= (4),
	CELL_GCM_ZCULL_STATS3		= (5),

	// SetPointSpriteControl
	CELL_GCM_POINT_SPRITE_RMODE_ZERO       = (0),
	CELL_GCM_POINT_SPRITE_RMODE_FROM_R     = (1),
	CELL_GCM_POINT_SPRITE_RMODE_FROM_S     = (2),

	CELL_GCM_POINT_SPRITE_TEX0             = (1<<8),
	CELL_GCM_POINT_SPRITE_TEX1             = (1<<9),
	CELL_GCM_POINT_SPRITE_TEX2             = (1<<10),
	CELL_GCM_POINT_SPRITE_TEX3             = (1<<11),
	CELL_GCM_POINT_SPRITE_TEX4             = (1<<12),
	CELL_GCM_POINT_SPRITE_TEX5             = (1<<13),
	CELL_GCM_POINT_SPRITE_TEX6             = (1<<14),
	CELL_GCM_POINT_SPRITE_TEX7             = (1<<15),
	CELL_GCM_POINT_SPRITE_TEX8             = (1<<16),
	CELL_GCM_POINT_SPRITE_TEX9             = (1<<17),

	// SetUserClipPlaneControl
	CELL_GCM_USER_CLIP_PLANE_DISABLE       = (0),
	CELL_GCM_USER_CLIP_PLANE_ENABLE_LT     = (1),
	CELL_GCM_USER_CLIP_PLANE_ENABLE_GE     = (2),

	// SetFrontPolygonMode, SetBackPolygonMode
	CELL_GCM_POLYGON_MODE_POINT            = (0x1b00),
	CELL_GCM_POLYGON_MODE_LINE             = (0x1b01),
	CELL_GCM_POLYGON_MODE_FILL             = (0x1b02),

	// SetTextureOptimization
	CELL_GCM_TEXTURE_ISO_LOW	= (0),
	CELL_GCM_TEXTURE_ISO_HIGH	= (1),
	CELL_GCM_TEXTURE_ANISO_LOW	= (0),
	CELL_GCM_TEXTURE_ANISO_HIGH	= (1),

	// SetDepthFormat
	CELL_GCM_DEPTH_FORMAT_FIXED	= (0),
	CELL_GCM_DEPTH_FORMAT_FLOAT	= (1),

	CELL_GCM_MAX_METHOD_COUNT = (2047)
};

#define CELL_GCM_METHOD_FLAG_NON_INCREMENT  (0x40000000)
#define CELL_GCM_METHOD_FLAG_JUMP           (0x20000000)
#define CELL_GCM_METHOD_FLAG_CALL           (0x00000002)
#define CELL_GCM_METHOD_FLAG_RETURN         (0x00020000)

#endif	// !__PPU

enum // make this look like xenon texture remap (xenon calls it 'swizzle')
{
	CELL_GCM_TEXTURE_REMAP_R = (CELL_GCM_TEXTURE_REMAP_REMAP << 8) | CELL_GCM_TEXTURE_REMAP_FROM_R,
	CELL_GCM_TEXTURE_REMAP_G = (CELL_GCM_TEXTURE_REMAP_REMAP << 8) | CELL_GCM_TEXTURE_REMAP_FROM_G,
	CELL_GCM_TEXTURE_REMAP_B = (CELL_GCM_TEXTURE_REMAP_REMAP << 8) | CELL_GCM_TEXTURE_REMAP_FROM_B,
	CELL_GCM_TEXTURE_REMAP_A = (CELL_GCM_TEXTURE_REMAP_REMAP << 8) | CELL_GCM_TEXTURE_REMAP_FROM_A,
	CELL_GCM_TEXTURE_REMAP_0 = (CELL_GCM_TEXTURE_REMAP_ZERO  << 8),
	CELL_GCM_TEXTURE_REMAP_1 = (CELL_GCM_TEXTURE_REMAP_ONE   << 8),

	CELL_GCM_TEXTURE_REMAP_MASK  = 0x00000303,
	CELL_GCM_TEXTURE_LINEAR_MASK = CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_LN,
};
#define CELL_GCM_TEXTURE_REMAP(x,y,z,w) \
( \
	((CELL_GCM_TEXTURE_REMAP_##x)<<6) | \
	((CELL_GCM_TEXTURE_REMAP_##y)<<4) | \
	((CELL_GCM_TEXTURE_REMAP_##z)<<2) | \
	((CELL_GCM_TEXTURE_REMAP_##w)<<0)   \
) \
// end.

namespace rage {

#if __PPU
extern __THREAD CellGcmContextData *g_grcCurrentContext ;

#define GCM		::cell::Gcm::Inline
#define GCMU	::cell::Gcm::UnsafeInline

#define GCM_CONTEXT		::rage::g_grcCurrentContext

#if GCM_HUD
void grcCellGcmHudInitialise(CellVideoOutBufferColorFormat format, bool enable);
void grcCellGcmHudSetEnable(bool enable);
extern bool grcCellGcmHudIsEnabled;
#endif

BANK_ONLY(extern bool g_AreRenderTargetsBound;)

#endif // __PPU

namespace gcm {

extern u32 g_Finisher;
extern u8 *g_LocalAddress;	// from CellGcmConfig; origin of local memory mapped into PPU address space.
extern u8 *g_IoAddress;		// from CellGcmConfig; base address of the first byte of PPU memory visible from RSX
extern u32 g_LocalSize;		// from CellGcmConfig; size of local memory
extern u32 g_IoSize;		// from CellGcmConfig; size of host memory

// Convert a pointer to an offset relative to CELL_GCM_LOCATION_MAIN
inline u32 MainOffset(const void* const GCM_ONLY(ptr))
{
	u32 offset = 0;
	GCM_ONLY(AssertVerify(cellGcmAddressToOffset(ptr, &offset)ASSERT_ONLY(==CELL_OK)));
	return offset;
}

// Convert a pointer to an offset relative to CELL_GCM_LOCATION_LOCAL
inline u32 LocalOffset(const void* const ptr)
{
	Assertf(ptr >= g_LocalAddress && ptr < g_LocalAddress + g_LocalSize,"gcm Local ptr %p outside range %p,%p (ignoring this will probably cause rsx crash 262)",ptr,g_LocalAddress,g_LocalAddress + g_LocalSize);
	return static_cast<u32>(reinterpret_cast<const u8* const>(ptr) - g_LocalAddress);
}

// Convert an offset relative to CELL_GCM_LOCATION_LOCAL to a valid PPU-addressable pointer.
inline void *LocalPtr(u32 offset)
{
	Assertf(g_LocalAddress + offset < g_LocalAddress + g_LocalSize,"gcm Local offset %x beyond %x",offset,g_LocalSize);
	return g_LocalAddress + offset;
}

// Convert an offset relative to CELL_GCM_LOCATION_MAIN to a valid PPU-addressable pointer.
inline void *MainPtr(u32 GCM_ONLY(offset))
{
	void* ptr = NULL;
	GCM_ONLY(AssertVerify(cellGcmIoOffsetToAddress(offset, &ptr)ASSERT_ONLY(==CELL_OK)));
	return ptr;
}

inline bool IsLocalPtr(const void* const ptr)
{
	return reinterpret_cast<const u8* const>(ptr) >= g_LocalAddress && reinterpret_cast<const u8* const>(ptr) < g_LocalAddress + g_LocalSize;
}

inline bool IsMainPtr(const void* const ptr)
{
	return reinterpret_cast<const u8* const>(ptr) >= g_IoAddress && reinterpret_cast<const u8* const>(ptr) < g_IoAddress + g_IoSize;
}

inline bool IsValidMainOffset(u32 GCM_ONLY(offset))
{
#if __GCM
	void* ptr = NULL;
	return cellGcmIoOffsetToAddress(offset, &ptr) == CELL_OK;
#else
	return false;
#endif // __GCM
}

inline bool IsValidLocalOffset(u32 offset)
{
	return offset < g_LocalSize;
}

inline bool IsValidTextureOffset(const CellGcmTexture* texture)
{
	if (texture->location == CELL_GCM_LOCATION_MAIN)
	{
		return IsValidMainOffset(texture->offset);
	}
	else
	{
		return texture->location == CELL_GCM_LOCATION_LOCAL && IsValidLocalOffset(texture->offset);
	}
}


inline u32 EncodeOffset(const void* const ptr)
{
	if (IsLocalPtr(ptr))
		return LocalOffset(ptr);
	else
		return MainOffset(ptr) | 0x80000000;
}

inline void* DecodeOffset(u32 offset)
{
	if (offset & 0x80000000)
		return MainPtr(offset);
	else
		return LocalPtr(offset);
}

inline u32 GetOffset(const void* const ptr)
{
	if (IsLocalPtr(ptr))
	{
		return LocalOffset(ptr);
	}
	else
	{
		FastAssert(IsMainPtr(ptr));
		return MainOffset(ptr);
	}
}


inline void * GetPtr(u32 offset, bool isLocal)
{
	if (isLocal)
	{
		FastAssert(IsValidLocalOffset(offset));
		return LocalPtr(offset);
	}
	else
	{
		FastAssert(IsValidMainOffset(offset));
		return MainPtr(offset);
	}
}

inline void* GetTextureAddress(const CellGcmTexture* texture)
{
	if (texture->location == CELL_GCM_LOCATION_LOCAL)
	{
		return LocalPtr(texture->offset);
	}
	else
	{
		FastAssert(texture->location == CELL_GCM_LOCATION_MAIN);
		return MainPtr(texture->offset);
	}
	
}

void Init();
void Shutdown();

// Help functions for surface and texture formats
u32 TextureFormatBitsPerPixel(u8 format);
u32 TextureFormatLinesPerPitch(u8 format);
bool TextureFormatSupportsSrgb(u8 format);
u32 SurfaceFormatBitsPerPixel(u8 format, bool isDepthFormat);
u8 TextureToSurfaceFormat(u8 format);
bool IsFloatingPointTextureFormat(u8 format);
bool IsFloatingPointColorSurfaceFormat(u8 format);
u32 GetSurfaceAlignment(bool inTiledMemory, bool isZCull);
u32 GetSurfaceWidth(u16 width);
u32 GetSurfaceHeight(u32 height, bool inLocalMemory);
u32 GetSurfaceTiledPitch(u16 width, u32 bitsPerPixel, bool isSwizzled);
u32 GetSurfacePitch(u16 width, u32 bitsPerPixel, bool isSwizzled);
u32 GetSurfaceMipMapCount(u16 width, u16 height, bool isSwizzled);
u32 GetSharedSurfaceSizeForPitch(u16 width, u16 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets);
u32 GetSharedSurfaceSize(u16 width, u16 height, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32 surfaceCount, u32* memoryOffsets);
u32 GetSurfaceSize(u16 width, u16 height, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets);
u32 GetSurfaceSizeForPitch(u16 width, u16 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets);
u32 GetTextureSize(u32 width, u32 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool isSwizzled, bool isCubeMap, u32* memoryOffsets, u32 linesPerPitch);

#if RAGE_ENABLE_RAGE_NEW
#define AllocatePtr(s,d) AllocatePtr_((s),(d),__FILE__,__LINE__)
inline void* AllocatePtr_(size_t size,bool dynamic,const char *file,int line)
{
	void *ptr = dynamic ? new (16,file,line) char[size] : physical_new_(size,16,file,line);
	Assertf(ptr, "Out of PS3 %s", dynamic ? "host memory?" : "VRAM? Maybe your PS3 is set to 1080p? That won't work on dev builds.");
	return ptr;
}
#else
inline void* AllocatePtr(size_t size,bool dynamic)
{
	void *ptr = dynamic ? rage_aligned_new (16) char[size] : physical_new(size,16);
	Assertf(ptr, "Out of PS3 %s", dynamic ? "host memory?" : "VRAM? Maybe your PS3 is set to 1080p? That won't work on dev builds.");
	return ptr;
}
#endif

inline void FreePtr(void *ptr)
{
#if __PPU
	if (IsLocalPtr(ptr))
	{
		physical_delete(ptr);
	}
	else
	{
		FastAssert(IsMainPtr(ptr));
		delete[] reinterpret_cast<char*>(ptr);
	}
#else	// Resource compiler!
	delete[] reinterpret_cast<char*>(ptr);
#endif
}

inline u8 StripTextureFormat(u8 format)
{
	return format & ~(CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR | CELL_GCM_TEXTURE_UN);
}

#if __PPU
inline uint32_t *AllocateFifo_Secret(uint32_t wordCount)
{
	CellGcmContextData *c = GCM_CONTEXT;
	if (Unlikely(c->current + wordCount > c->end)) {
		AssertVerify(c->callback(c, wordCount) == CELL_OK);
	}
	if (Unlikely(c->current + wordCount > c->end)) {
		return NULL; // Request is larger than segment
	}
	uint32_t *result = c->current;
	c->current += wordCount;
	return result;
}
#endif		// __PPU

enum gcmAttrToSemantic {
	POSITION, BLENDWEIGHT, NORMAL, COLOR0,
	COLOR1, TANGENT1, BINORMAL1, BLENDINDICES,
	TEXCOORD0, TEXCOORD1, TEXCOORD2, TEXCOORD3,
	TEXCOORD4, TEXCOORD5, TANGENT0, BINORMAL0
};

void HexDump(const char *fn,int line,const char *desc,uint32_t *start,uint32_t *end);

template <typename T, u32 _Min, u32 _Max>
class LabelRegistrar
{
public:
	// This will always allocate indices in contiguous blocks
	// You must check the return value to see if it is valid
	// using IsValid()
	static u32 Allocate(u32 count = 1);
	static void Free(u32 index, u32 count = 1);
	static bool Reserve(u32 index, u32 count = 1);

	// Check whether the given index is a valid time stamp index.
	// Useful for checking whether Allocate() succeeded or not.
	static inline bool IsValid(u32 index)
	{
		return index <= _Max;
	}

private:
	CompileTimeAssert(_Max > _Min);
	CompileTimeAssert(_Max != 0xffffffff); // Reserved for invalid label
	typedef atFixedBitSet<_Max - _Min> BitSet;
	static BitSet sm_Bits;
};

template <typename _Type, u32 _Min, u32 _Max>
typename LabelRegistrar<_Type, _Min, _Max>::BitSet LabelRegistrar<_Type, _Min, _Max>::sm_Bits PPU_ONLY(__attribute__((init_priority(101)))) ; // Need this init'd before any global objects that allocate labels (such as grcGpuTimers) 

template <typename _Type, u32 _Min, u32 _Max>
u32 LabelRegistrar<_Type, _Min, _Max>::Allocate(u32 count /*= 1*/)
{
	// FastAssert(count >= 0);

	u32 index = 0;
	u32 bitCount = 0;
	for (u32 i = 0; i < BitSet::NUM_BITS && bitCount < count; ++i)
	{

		if (sm_Bits.IsSet(i))
		{
			index = i + 1;
			bitCount = 0;
		}
		else
		{
			++bitCount;
		}
	}

	if (bitCount == count)
	{
		const u32 indexCount = count + index;
		for (u32 i = index; i < indexCount; ++i)
		{
			sm_Bits.Set(i);
		}
		// We found a contiguous range of bits
		return index + _Min;
	}
	else
	{
		// We couldn't find enough bits, try allocating multiple chunks
		return 0xffffffff;
	}
}

template <typename _Type, u32 _Min, u32 _Max>
void LabelRegistrar<_Type, _Min, _Max>::Free(u32 index, u32 count /*= 1*/)
{
	ASSERT_ONLY(if (count > 0))
	{
		FastAssert(index <= _Max);
		index -= _Min; // Make index zero based;
		Assertf((index + count - 1) <= BitSet::NUM_BITS, "base: %d, count %d (min: %d)", index + _Min, count, _Min);

		for (u32 i = index; i < index + count; ++i)
		{
			FastAssert(sm_Bits.IsSet(i));
			sm_Bits.Clear(i);
		}
	}
}

template <typename _Type, u32 _Min, u32 _Max>
bool LabelRegistrar<_Type, _Min, _Max>::Reserve(u32 index, u32 count /*= 1*/)
{
	ASSERT_ONLY(if (count > 0))
	{
		FastAssert(index >= _Min && index <= _Max);
		index -= _Min; // Make index zero based;
		Assertf((index + count - 1) <= BitSet::NUM_BITS, "base: %d, count %d (min: %d)", index + _Min, count, _Min);

		for (u32 i = index; i < count; ++i)
		{
			if (sm_Bits.IsSet(i))
			{
				return false;
			}
			else
			{
				sm_Bits.Set(i);
			}
		}
	}

	return true;
}

// struct TimeStampLabelType {};
// typedef LabelRegistrar<TimeStampLabelType, 0, 2047> TimeStampRegistrar;
// According to docs, time stamps and reports come from the same address space.
#define TimeStampRegistrar ReportRegistrar
struct RsxSemaphoreLabelType {};
typedef LabelRegistrar<RsxSemaphoreLabelType, 64, 255> RsxSemaphoreRegistrar;
struct ReportLabelType {};
typedef LabelRegistrar<ReportLabelType, 0, GCM_REPORT_COUNT-1> ReportRegistrar;
struct DmaTagType {};
typedef LabelRegistrar<DmaTagType, 0, 31> DmaTagRegistrar;

namespace Shadow {
	enum {
		SetTransformBranchBits,
		SetVertexTexture, vt1, vt2, vt3,
		SetVertexTextureBorderColor, vtbc1, vtbc2, vtbc3,
		SetTexture, st1, st2, st3, st4, st5, st6, st7, st8, st9, st10, st11, st12, st13, st14, st15,
		SetTextureBorderColor, stbc1, stbc2, stbc3, stbc4, stbc5, stbc6, stbc7, stbc8, stbc9, stbc10, stbc11, stbc12, stbc13, stbc14, stbc15,
		SetDepthTestEnable,
		SetFrontPolygonMode,
		SetBackPolygonMode,
		SetDepthMask,
		SetAlphaTestEnable,
		SetCullFaceEnable,
		SetCullFace,
		SetDepthFunc,
		SetBlendEnable,
		SetStencilTestEnable,
		SetColorMask,
		SetColorMaskMrt,
		SetPolygonOffsetFillEnable,
		SetStencilMask,
		SetClearColor,
		SetClearDepthStencil,
		SetDepthFormat,
		SetBlendOptimization,
		SetFrequencyDividerOperation,
		COUNT
	};
	extern u32 g_State[COUNT];
};


#if __PS3

// cellGcmSetTransferData is NOT safe to be called from the PPU.  The problem is
// that it calls CELL_GCM_RESERVE multiple times, allowing the command to be
// split up into seperate PPU FIFO segments.  drawablespu can then insert
// gpuMemCpy blts in between, which can cause the cellGcmSetTransferData setup
// on the PPU to use the wrong src and or dst DMA contexts (ie, main/local).
//
// This custom function determines the total amount of command buffer space
// required, and does one reserve up front.  This must be used on the PPU
// instead of cellGcmSetTransferData.
//
void SetTransferData(CellGcmContextData *ctx,
	u8 mode, u32 dstOffset, s32 dstPitch, u32 srcOffset,
	s32 srcPitch, s32 bytesPerRow, s32 rowCount);

#define cellGcmSetTransferData              ERROR "use gcm::SetTransferData instead"
#define cellGcmSetTransferDataInline        ERROR "use gcm::SetTransferData instead"
#define cellGcmSetTransferDataUnsafe        ERROR "use gcm::SetTransferData instead"
#define cellGcmSetTransferDataUnsafeInline  ERROR "use gcm::SetTransferData instead"

#endif // __PS3


#if __PPU
extern CellGcmControl volatile *g_ControlRegister;

#if !__FINAL
void GcmTextureMemSet(const CellGcmTexture *target, int sampleID, u32 value);
#endif // !__FINAL

#endif

}	// namespace gcm

extern void gcmInsertString(const char *msg);
}	// namespace rage

#if 0
#define GCM_DEBUG(cmd) do { \
	static bool didIt; \
	uint32_t *begin = cellGcmGetCurrentBuffer(GCM_CONTEXT); \
	cmd; \
	uint32_t *end = cellGcmGetCurrentBuffer(GCM_CONTEXT); \
	if (!didIt) { \
		didIt=true; \
		rage::gcm::HexDump(__FILE__,__LINE__,#cmd,begin,end); \
	} \
} while (0)
#elif 0	// Enable this version for libgcmdbg-like functionality except with more control.
#define GCM_DEBUG(cmd)	do { cmd; ++::rage::gcm::g_Finisher; cellGcmFinish(::rage::gcm::g_Finisher); } while (0)
#elif 0	// inserts debug "spew" into command buffer for diagnosing crashes without too much performance impact.
#define GCM_DEBUG(cmd)	do { gcmInsertString("before " #cmd); cmd; gcmInsertString("after " #cmd); } while (0)
#elif 0	// issues flush after each command.
#define GCM_DEBUG(cmd)	do { cmd; rageFlush(GCM_CONTEXT); } while (0)
#else
#define GCM_DEBUG(cmd) cmd
#endif

// Use this macro internally to make sure that we haven't run past the end of the reserved area.
#define GCM_CHECK(ctxt)	Assertf((u32)ctxt->current < (((u32)ctxt->end + 255) & ~255),"gcm fifo overflow, %p > %p",ctxt->current,ctxt->end)

#if 1
#define GCM_STATE(cmd,value)			GCM_DEBUG(GCM::cellGcm##cmd(GCM_CONTEXT,value))
#define GCM_STATEN(cmd,idx,type,value)	GCM_DEBUG(GCM::cellGcm##cmd(GCM_CONTEXT,idx,(type)value))
#else
#define GCM_STATE(cmd,value)			do { ::rage::u32 v = ::rage::u32(value); if (v != ::rage::gcm::Shadow::g_State[::rage::gcm::Shadow::cmd]) GCM_DEBUG(GCM::cellGcm##cmd(GCM_CONTEXT,::rage::gcm::Shadow::g_State[::rage::gcm::Shadow::cmd] = v)); } while (0)
#define GCM_STATEN(cmd,idx,type,value)	do { ::rage::u32 v = ::rage::u32(value); ::rage::u32 i = (idx); if (v != ::rage::gcm::Shadow::g_State[::rage::gcm::Shadow::cmd+i]) GCM_DEBUG(GCM::cellGcm##cmd(GCM_CONTEXT,i,(type)(::rage::gcm::Shadow::g_State[::rage::gcm::Shadow::cmd+i] = v))); } while (0)
#endif

#elif __SPU

#include "grcore/channel.h"

#define CELL_GCM_DENY_IMPLICIT_ARG
#include <cell/gcm_spu.h>
#include <cell/gcm/gcm_method_data.h>

#define GCM				::cell::Gcm::Inline
#define GCMU			::cell::Gcm::UnsafeInline
#define GCM_CONTEXT		(&ctxt)
#define GCM_DEBUG(cmd)	cmd
#define GCM_STATE(cmd,value)			GCM::cellGcm##cmd(GCM_CONTEXT,value)
#define GCM_STATEN(cmd,idx,type,value)	GCM::cellGcm##cmd(GCM_CONTEXT,idx,(type)value)


// Write CELL_GCM_METHOD_NOPs up to the next qword aligned boundary.
template<class T>
__forceinline T *MethodNopToQwordAlignment(T *cmd)
{
	// Load the first quadword we're pointing at.
	// Do a masked shift right of a one's mask based on the 4 LSB's
	// and do a complemented and to clear the necessary bits (turning them into GCM nop's)
	CompileTimeAssert(CELL_GCM_METHOD_NOP == 0);
	qword writePtr = si_from_ptr(cmd);
	qword prevQuad = si_lqd(writePtr,0);
	qword mask = si_rotqmby(si_il(-1),si_sfi(si_andi(writePtr,15),0));
	qword newQuad = si_andc(prevQuad,mask);
	si_stqd(newQuad,writePtr,0);
	return (T*)(((uintptr_t)cmd+15)&~15);
}

// Make sure that there is enough room in the command buffer for at least
// 'wordCount' words.  The ctx->current pointer is left pointing to the start of
// the reserved space.
__forceinline uint32_t *EnsureMethodSpaceWords(CellGcmContextData *ctxt,uint32_t wordCount)
{
	uint32_t *cmd = ctxt->current;
	if (Unlikely(cmd + wordCount > ctxt->end))
	{
		gcmCallback(ctxt, wordCount);
		cmd = ctxt->current;
	}
	return cmd;
}

// Padd command buffer pointer to a qword aligned boundary with
// CELL_GCM_METHOD_NOPs, and allocate room in the command buffer for 'quadCount'
// qwords.  The ctx->current pointer is left pointing to the end of the
// allocated space.
__forceinline qword *ReserveMethodSizeAligned(CellGcmContextData *ctxt,uint32_t quadCount)
{
	const uint32_t wordCount = (quadCount * 4) + 4;
	const uint32_t byteCount = quadCount * 16;

	if (Unlikely(ctxt->current + wordCount > ctxt->end))
	{
		gcmCallback(ctxt,wordCount);
	}

	uint32_t *writePtr = ctxt->current;
	uint32_t *alignedWritePtr = MethodNopToQwordAlignment(writePtr);
	ctxt->current = (uint32_t*)((uintptr_t)alignedWritePtr+byteCount);

	return (qword*) alignedWritePtr;
}

#elif __PSP2

// This is a little weird, but CellGcmTexture is already big enough to hold a SceGxmTexture in its entirety.
// We have two other words left over for our own nefarious purposes
struct CellGcmTexture
{
	rage::u32 opaque[4];
	rage::u8 _padding, _padding2[2], mipcount;
	rage::u16 width, height;
};
#endif	// __PPU || __WIN32PC

#if __PS3
namespace rage
{
namespace gcm
{

inline const char *RegisterName(int reg) {
	switch (reg) {
	case CELL_GCM_NV406E_SET_REFERENCE	: return "SET_REFERENCE";
	case CELL_GCM_NV406E_SET_CONTEXT_DMA_SEMAPHORE	: return "SET_CONTEXT_DMA_SEMAPHORE";
	case CELL_GCM_NV406E_SEMAPHORE_OFFSET	: return "SEMAPHORE_OFFSET";
	case CELL_GCM_NV406E_SEMAPHORE_ACQUIRE	: return "SEMAPHORE_ACQUIRE";
	case CELL_GCM_NV406E_SEMAPHORE_RELEASE	: return "SEMAPHORE_RELEASE";
	case CELL_GCM_NV4097_SET_OBJECT		: return "NOP (SET_OBJECT)";
	case CELL_GCM_NV4097_NO_OPERATION	: return "NO_OPERATION";
	case CELL_GCM_NV4097_WAIT_FOR_IDLE	: return "WAIT_FOR_IDLE";
	case CELL_GCM_NV4097_PM_TRIGGER	: return "PM_TRIGGER";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_NOTIFIES	: return "SET_CONTEXT_DMA_NOTIFIES";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_A	: return "SET_CONTEXT_DMA_A";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_B	: return "SET_CONTEXT_DMA_B";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_B	: return "SET_CONTEXT_DMA_COLOR_B";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_STATE	: return "SET_CONTEXT_DMA_STATE";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_A	: return "SET_CONTEXT_DMA_COLOR_A";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_ZETA	: return "SET_CONTEXT_DMA_ZETA";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_A	: return "SET_CONTEXT_DMA_VERTEX_A";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_VERTEX_B	: return "SET_CONTEXT_DMA_VERTEX_B";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_SEMAPHORE	: return "SET_CONTEXT_DMA_SEMAPHORE";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_REPORT	: return "SET_CONTEXT_DMA_REPORT";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_CLIP_ID	: return "SET_CONTEXT_DMA_CLIP_ID";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_CULL_DATA	: return "SET_CONTEXT_DMA_CULL_DATA";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_C	: return "SET_CONTEXT_DMA_COLOR_C";
	case CELL_GCM_NV4097_SET_CONTEXT_DMA_COLOR_D	: return "SET_CONTEXT_DMA_COLOR_D";
	case CELL_GCM_NV4097_SET_SURFACE_CLIP_HORIZONTAL	: return "SET_SURFACE_CLIP_HORIZONTAL";
	case CELL_GCM_NV4097_SET_SURFACE_CLIP_VERTICAL	: return "SET_SURFACE_CLIP_VERTICAL";
	case CELL_GCM_NV4097_SET_SURFACE_FORMAT	: return "SET_SURFACE_FORMAT";
	case CELL_GCM_NV4097_SET_SURFACE_PITCH_A	: return "SET_SURFACE_PITCH_A";
	case CELL_GCM_NV4097_SET_SURFACE_COLOR_AOFFSET	: return "SET_SURFACE_COLOR_AOFFSET";
	case CELL_GCM_NV4097_SET_SURFACE_ZETA_OFFSET	: return "SET_SURFACE_ZETA_OFFSET";
	case CELL_GCM_NV4097_SET_SURFACE_COLOR_BOFFSET	: return "SET_SURFACE_COLOR_BOFFSET";
	case CELL_GCM_NV4097_SET_SURFACE_PITCH_B	: return "SET_SURFACE_PITCH_B";
	case CELL_GCM_NV4097_SET_SURFACE_COLOR_TARGET	: return "SET_SURFACE_COLOR_TARGET";
	case CELL_GCM_NV4097_SET_SURFACE_PITCH_Z	: return "SET_SURFACE_PITCH_Z";
	case CELL_GCM_NV4097_INVALIDATE_ZCULL	: return "INVALIDATE_ZCULL";
	case CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP	: return "SET_CYLINDRICAL_WRAP";
	case CELL_GCM_NV4097_SET_CYLINDRICAL_WRAP1	: return "SET_CYLINDRICAL_WRAP1";
	case CELL_GCM_NV4097_SET_SURFACE_PITCH_C	: return "SET_SURFACE_PITCH_C";
	case CELL_GCM_NV4097_SET_SURFACE_PITCH_D	: return "SET_SURFACE_PITCH_D";
	case CELL_GCM_NV4097_SET_SURFACE_COLOR_COFFSET	: return "SET_SURFACE_COLOR_COFFSET";
	case CELL_GCM_NV4097_SET_SURFACE_COLOR_DOFFSET	: return "SET_SURFACE_COLOR_DOFFSET";
	case CELL_GCM_NV4097_SET_WINDOW_OFFSET	: return "SET_WINDOW_OFFSET";
	case CELL_GCM_NV4097_SET_WINDOW_CLIP_TYPE	: return "SET_WINDOW_CLIP_TYPE";
	case CELL_GCM_NV4097_SET_WINDOW_CLIP_HORIZONTAL	: return "SET_WINDOW_CLIP_HORIZONTAL";
	case CELL_GCM_NV4097_SET_WINDOW_CLIP_VERTICAL	: return "SET_WINDOW_CLIP_VERTICAL";
	case CELL_GCM_NV4097_SET_DITHER_ENABLE	: return "SET_DITHER_ENABLE";
	case CELL_GCM_NV4097_SET_ALPHA_TEST_ENABLE	: return "SET_ALPHA_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_ALPHA_FUNC	: return "SET_ALPHA_FUNC";
	case CELL_GCM_NV4097_SET_ALPHA_REF	: return "SET_ALPHA_REF";
	case CELL_GCM_NV4097_SET_BLEND_ENABLE	: return "SET_BLEND_ENABLE";
	case CELL_GCM_NV4097_SET_BLEND_FUNC_SFACTOR	: return "SET_BLEND_FUNC_SFACTOR";
	case CELL_GCM_NV4097_SET_BLEND_FUNC_DFACTOR	: return "SET_BLEND_FUNC_DFACTOR";
	case CELL_GCM_NV4097_SET_BLEND_COLOR	: return "SET_BLEND_COLOR";
	case CELL_GCM_NV4097_SET_BLEND_EQUATION	: return "SET_BLEND_EQUATION";
	case CELL_GCM_NV4097_SET_COLOR_MASK	: return "SET_COLOR_MASK";
	case CELL_GCM_NV4097_SET_STENCIL_TEST_ENABLE	: return "SET_STENCIL_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_STENCIL_MASK	: return "SET_STENCIL_MASK";
	case CELL_GCM_NV4097_SET_STENCIL_FUNC	: return "SET_STENCIL_FUNC";
	case CELL_GCM_NV4097_SET_STENCIL_FUNC_REF	: return "SET_STENCIL_FUNC_REF";
	case CELL_GCM_NV4097_SET_STENCIL_FUNC_MASK	: return "SET_STENCIL_FUNC_MASK";
	case CELL_GCM_NV4097_SET_STENCIL_OP_FAIL	: return "SET_STENCIL_OP_FAIL";
	case CELL_GCM_NV4097_SET_STENCIL_OP_ZFAIL	: return "SET_STENCIL_OP_ZFAIL";
	case CELL_GCM_NV4097_SET_STENCIL_OP_ZPASS	: return "SET_STENCIL_OP_ZPASS";
	case CELL_GCM_NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE	: return "SET_TWO_SIDED_STENCIL_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_MASK	: return "SET_BACK_STENCIL_MASK";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC	: return "SET_BACK_STENCIL_FUNC";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_REF	: return "SET_BACK_STENCIL_FUNC_REF";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_FUNC_MASK	: return "SET_BACK_STENCIL_FUNC_MASK";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_FAIL	: return "SET_BACK_STENCIL_OP_FAIL";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZFAIL	: return "SET_BACK_STENCIL_OP_ZFAIL";
	case CELL_GCM_NV4097_SET_BACK_STENCIL_OP_ZPASS	: return "SET_BACK_STENCIL_OP_ZPASS";
	case CELL_GCM_NV4097_SET_SHADE_MODE	: return "SET_SHADE_MODE";
	case CELL_GCM_NV4097_SET_BLEND_ENABLE_MRT	: return "SET_BLEND_ENABLE_MRT";
	case CELL_GCM_NV4097_SET_COLOR_MASK_MRT	: return "SET_COLOR_MASK_MRT";
	case CELL_GCM_NV4097_SET_LOGIC_OP_ENABLE	: return "SET_LOGIC_OP_ENABLE";
	case CELL_GCM_NV4097_SET_LOGIC_OP	: return "SET_LOGIC_OP";
	case CELL_GCM_NV4097_SET_BLEND_COLOR2	: return "SET_BLEND_COLOR2";
	case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE	: return "SET_DEPTH_BOUNDS_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MIN	: return "SET_DEPTH_BOUNDS_MIN";
	case CELL_GCM_NV4097_SET_DEPTH_BOUNDS_MAX	: return "SET_DEPTH_BOUNDS_MAX";
	case CELL_GCM_NV4097_SET_CLIP_MIN	: return "SET_CLIP_MIN";
	case CELL_GCM_NV4097_SET_CLIP_MAX	: return "SET_CLIP_MAX";
	case CELL_GCM_NV4097_SET_CONTROL0	: return "SET_CONTROL0";
	case CELL_GCM_NV4097_SET_LINE_WIDTH	: return "SET_LINE_WIDTH";
	case CELL_GCM_NV4097_SET_LINE_SMOOTH_ENABLE	: return "SET_LINE_SMOOTH_ENABLE";
	case CELL_GCM_NV4097_SET_ANISO_SPREAD       	: return "SET_ANISO_SPREAD       ";
	case CELL_GCM_NV4097_SET_SCISSOR_HORIZONTAL	: return "SET_SCISSOR_HORIZONTAL";
	case CELL_GCM_NV4097_SET_SCISSOR_VERTICAL	: return "SET_SCISSOR_VERTICAL";
	case CELL_GCM_NV4097_SET_FOG_MODE	: return "SET_FOG_MODE";
	case CELL_GCM_NV4097_SET_FOG_PARAMS	: return "SET_FOG_PARAMS";
	case CELL_GCM_NV4097_SET_SHADER_PROGRAM	: return "SET_SHADER_PROGRAM";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_OFFSET	: return "SET_VERTEX_TEXTURE_OFFSET";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FORMAT	: return "SET_VERTEX_TEXTURE_FORMAT";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_ADDRESS	: return "SET_VERTEX_TEXTURE_ADDRESS";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL0	: return "SET_VERTEX_TEXTURE_CONTROL0";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_CONTROL3	: return "SET_VERTEX_TEXTURE_CONTROL3";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_FILTER	: return "SET_VERTEX_TEXTURE_FILTER";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT	: return "SET_VERTEX_TEXTURE_IMAGE_RECT";
	case CELL_GCM_NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR	: return "SET_VERTEX_TEXTURE_BORDER_COLOR";
	case CELL_GCM_NV4097_SET_VIEWPORT_HORIZONTAL	: return "SET_VIEWPORT_HORIZONTAL";
	case CELL_GCM_NV4097_SET_VIEWPORT_VERTICAL	: return "SET_VIEWPORT_VERTICAL";
	case CELL_GCM_NV4097_SET_POINT_CENTER_MODE	: return "SET_POINT_CENTER_MODE";
	case CELL_GCM_NV4097_SET_VIEWPORT_OFFSET	: return "SET_VIEWPORT_OFFSET";
	case CELL_GCM_NV4097_SET_VIEWPORT_SCALE	: return "SET_VIEWPORT_SCALE";
	case CELL_GCM_NV4097_SET_POLY_OFFSET_POINT_ENABLE	: return "SET_POLY_OFFSET_POINT_ENABLE";
	case CELL_GCM_NV4097_SET_POLY_OFFSET_LINE_ENABLE	: return "SET_POLY_OFFSET_LINE_ENABLE";
	case CELL_GCM_NV4097_SET_POLY_OFFSET_FILL_ENABLE	: return "SET_POLY_OFFSET_FILL_ENABLE";
	case CELL_GCM_NV4097_SET_DEPTH_FUNC	: return "SET_DEPTH_FUNC";
	case CELL_GCM_NV4097_SET_DEPTH_MASK	: return "SET_DEPTH_MASK";
	case CELL_GCM_NV4097_SET_DEPTH_TEST_ENABLE	: return "SET_DEPTH_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR	: return "SET_POLYGON_OFFSET_SCALE_FACTOR";
	case CELL_GCM_NV4097_SET_POLYGON_OFFSET_BIAS	: return "SET_POLYGON_OFFSET_BIAS";
	case CELL_GCM_NV4097_SET_VERTEX_DATA_SCALED4S_M	: return "SET_VERTEX_DATA_SCALED4S_M";
	case CELL_GCM_NV4097_SET_TEXTURE_CONTROL2	: return "SET_TEXTURE_CONTROL2";
	case CELL_GCM_NV4097_SET_TEX_COORD_CONTROL	: return "SET_TEX_COORD_CONTROL";
	case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM	: return "SET_TRANSFORM_PROGRAM";
	case CELL_GCM_NV4097_SET_SPECULAR_ENABLE	: return "SET_SPECULAR_ENABLE";
	case CELL_GCM_NV4097_SET_TWO_SIDE_LIGHT_EN	: return "SET_TWO_SIDE_LIGHT_EN";
	case CELL_GCM_NV4097_CLEAR_ZCULL_SURFACE	: return "CLEAR_ZCULL_SURFACE";
	case CELL_GCM_NV4097_SET_PERFORMANCE_PARAMS	: return "SET_PERFORMANCE_PARAMS";
	case CELL_GCM_NV4097_SET_FLAT_SHADE_OP	: return "SET_FLAT_SHADE_OP";
	case CELL_GCM_NV4097_SET_EDGE_FLAG	: return "SET_EDGE_FLAG";
	case CELL_GCM_NV4097_SET_USER_CLIP_PLANE_CONTROL	: return "SET_USER_CLIP_PLANE_CONTROL";
	case CELL_GCM_NV4097_SET_POLYGON_STIPPLE	: return "SET_POLYGON_STIPPLE";
	case CELL_GCM_NV4097_SET_POLYGON_STIPPLE_PATTERN	: return "SET_POLYGON_STIPPLE_PATTERN";
	case CELL_GCM_NV4097_SET_VERTEX_DATA3F_M	: return "SET_VERTEX_DATA3F_M";
	case CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET	: return "SET_VERTEX_DATA_ARRAY_OFFSET";
	case CELL_GCM_NV4097_INVALIDATE_VERTEX_CACHE_FILE	: return "INVALIDATE_VERTEX_CACHE_FILE";
	case CELL_GCM_NV4097_INVALIDATE_VERTEX_FILE	: return "INVALIDATE_VERTEX_FILE";
	case CELL_GCM_NV4097_PIPE_NOP	: return "PIPE_NOP";
	case CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_OFFSET	: return "SET_VERTEX_DATA_BASE_OFFSET";
	case CELL_GCM_NV4097_SET_VERTEX_DATA_BASE_INDEX	: return "SET_VERTEX_DATA_BASE_INDEX";
	case CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT	: return "SET_VERTEX_DATA_ARRAY_FORMAT";
	case CELL_GCM_NV4097_CLEAR_REPORT_VALUE	: return "CLEAR_REPORT_VALUE";
	case CELL_GCM_NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE	: return "SET_ZPASS_PIXEL_COUNT_ENABLE";
	case CELL_GCM_NV4097_GET_REPORT	: return "GET_REPORT";
	case CELL_GCM_NV4097_SET_ZCULL_STATS_ENABLE	: return "SET_ZCULL_STATS_ENABLE";
	case CELL_GCM_NV4097_SET_BEGIN_END	: return "SET_BEGIN_END";
	case CELL_GCM_NV4097_ARRAY_ELEMENT16	: return "ARRAY_ELEMENT16";
	case CELL_GCM_NV4097_ARRAY_ELEMENT32	: return "ARRAY_ELEMENT32";
	case CELL_GCM_NV4097_DRAW_ARRAYS	: return "DRAW_ARRAYS";
	case CELL_GCM_NV4097_INLINE_ARRAY	: return "INLINE_ARRAY";
	case CELL_GCM_NV4097_SET_INDEX_ARRAY_ADDRESS	: return "SET_INDEX_ARRAY_ADDRESS";
	case CELL_GCM_NV4097_SET_INDEX_ARRAY_DMA	: return "SET_INDEX_ARRAY_DMA";
	case CELL_GCM_NV4097_DRAW_INDEX_ARRAY	: return "DRAW_INDEX_ARRAY";
	case CELL_GCM_NV4097_SET_FRONT_POLYGON_MODE	: return "SET_FRONT_POLYGON_MODE";
	case CELL_GCM_NV4097_SET_BACK_POLYGON_MODE	: return "SET_BACK_POLYGON_MODE";
	case CELL_GCM_NV4097_SET_CULL_FACE	: return "SET_CULL_FACE";
	case CELL_GCM_NV4097_SET_FRONT_FACE	: return "SET_FRONT_FACE";
	case CELL_GCM_NV4097_SET_POLY_SMOOTH_ENABLE	: return "SET_POLY_SMOOTH_ENABLE";
	case CELL_GCM_NV4097_SET_CULL_FACE_ENABLE	: return "SET_CULL_FACE_ENABLE";
	case CELL_GCM_NV4097_SET_TEXTURE_CONTROL3	: return "SET_TEXTURE_CONTROL3";
	case CELL_GCM_NV4097_SET_VERTEX_DATA2F_M	: return "SET_VERTEX_DATA2F_M";
	case CELL_GCM_NV4097_SET_VERTEX_DATA2S_M	: return "SET_VERTEX_DATA2S_M";
	case CELL_GCM_NV4097_SET_VERTEX_DATA4UB_M	: return "SET_VERTEX_DATA4UB_M";
	case CELL_GCM_NV4097_SET_VERTEX_DATA4S_M	: return "SET_VERTEX_DATA4S_M";
	case CELL_GCM_NV4097_SET_TEXTURE_OFFSET	: return "SET_TEXTURE_OFFSET";
	case CELL_GCM_NV4097_SET_TEXTURE_FORMAT	: return "SET_TEXTURE_FORMAT";
	case CELL_GCM_NV4097_SET_TEXTURE_ADDRESS	: return "SET_TEXTURE_ADDRESS";
	case CELL_GCM_NV4097_SET_TEXTURE_CONTROL0	: return "SET_TEXTURE_CONTROL0";
	case CELL_GCM_NV4097_SET_TEXTURE_CONTROL1	: return "SET_TEXTURE_CONTROL1";
	case CELL_GCM_NV4097_SET_TEXTURE_FILTER	: return "SET_TEXTURE_FILTER";
	case CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT	: return "SET_TEXTURE_IMAGE_RECT";
	case CELL_GCM_NV4097_SET_TEXTURE_BORDER_COLOR	: return "SET_TEXTURE_BORDER_COLOR";
	case CELL_GCM_NV4097_SET_VERTEX_DATA4F_M	: return "SET_VERTEX_DATA4F_M";
	case CELL_GCM_NV4097_SET_COLOR_KEY_COLOR	: return "SET_COLOR_KEY_COLOR";
	case CELL_GCM_NV4097_SET_SHADER_CONTROL	: return "SET_SHADER_CONTROL";
	case CELL_GCM_NV4097_SET_INDEXED_CONSTANT_READ_LIMITS	: return "SET_INDEXED_CONSTANT_READ_LIMITS";
	case CELL_GCM_NV4097_SET_SEMAPHORE_OFFSET	: return "SET_SEMAPHORE_OFFSET";
	case CELL_GCM_NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE	: return "BACK_END_WRITE_SEMAPHORE_RELEASE";
	case CELL_GCM_NV4097_TEXTURE_READ_SEMAPHORE_RELEASE	: return "TEXTURE_READ_SEMAPHORE_RELEASE";
	case CELL_GCM_NV4097_SET_ZMIN_MAX_CONTROL	: return "SET_ZMIN_MAX_CONTROL";
	case CELL_GCM_NV4097_SET_ANTI_ALIASING_CONTROL	: return "SET_ANTI_ALIASING_CONTROL";
	case CELL_GCM_NV4097_SET_SURFACE_COMPRESSION	: return "SET_SURFACE_COMPRESSION";
	case CELL_GCM_NV4097_SET_ZCULL_EN	: return "SET_ZCULL_EN";
	case CELL_GCM_NV4097_SET_SHADER_WINDOW	: return "SET_SHADER_WINDOW";
	case CELL_GCM_NV4097_SET_ZSTENCIL_CLEAR_VALUE	: return "SET_ZSTENCIL_CLEAR_VALUE";
	case CELL_GCM_NV4097_SET_COLOR_CLEAR_VALUE	: return "SET_COLOR_CLEAR_VALUE";
	case CELL_GCM_NV4097_CLEAR_SURFACE	: return "CLEAR_SURFACE";
	case CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL	: return "SET_CLEAR_RECT_HORIZONTAL";
	case CELL_GCM_NV4097_SET_CLEAR_RECT_VERTICAL	: return "SET_CLEAR_RECT_VERTICAL";
	case CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE	: return "SET_CLIP_ID_TEST_ENABLE";
	case CELL_GCM_NV4097_SET_RESTART_INDEX_ENABLE	: return "SET_RESTART_INDEX_ENABLE";
	case CELL_GCM_NV4097_SET_RESTART_INDEX	: return "SET_RESTART_INDEX";
	case CELL_GCM_NV4097_SET_LINE_STIPPLE	: return "SET_LINE_STIPPLE";
	case CELL_GCM_NV4097_SET_LINE_STIPPLE_PATTERN	: return "SET_LINE_STIPPLE_PATTERN";
	case CELL_GCM_NV4097_SET_VERTEX_DATA1F_M	: return "SET_VERTEX_DATA1F_M";
	case CELL_GCM_NV4097_SET_TRANSFORM_EXECUTION_MODE	: return "SET_TRANSFORM_EXECUTION_MODE";
	case CELL_GCM_NV4097_SET_RENDER_ENABLE	: return "SET_RENDER_ENABLE";
	case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_LOAD	: return "SET_TRANSFORM_PROGRAM_LOAD";
	case CELL_GCM_NV4097_SET_TRANSFORM_PROGRAM_START	: return "SET_TRANSFORM_PROGRAM_START";
	case CELL_GCM_NV4097_SET_ZCULL_CONTROL0	: return "SET_ZCULL_CONTROL0";
	case CELL_GCM_NV4097_SET_ZCULL_CONTROL1	: return "SET_ZCULL_CONTROL1";
	case CELL_GCM_NV4097_SET_SCULL_CONTROL	: return "SET_SCULL_CONTROL";
	case CELL_GCM_NV4097_SET_POINT_SIZE	: return "SET_POINT_SIZE";
	case CELL_GCM_NV4097_SET_POINT_PARAMS_ENABLE	: return "SET_POINT_PARAMS_ENABLE";
	case CELL_GCM_NV4097_SET_POINT_SPRITE_CONTROL	: return "SET_POINT_SPRITE_CONTROL";
	case CELL_GCM_NV4097_SET_TRANSFORM_TIMEOUT	: return "SET_TRANSFORM_TIMEOUT";
	case CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT_LOAD	: return "SET_TRANSFORM_CONSTANT_LOAD";
	case CELL_GCM_NV4097_SET_TRANSFORM_CONSTANT	: return "SET_TRANSFORM_CONSTANT";
	case CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION	: return "SET_FREQUENCY_DIVIDER_OPERATION";
	case CELL_GCM_NV4097_SET_ATTRIB_COLOR	: return "SET_ATTRIB_COLOR";
	case CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD	: return "SET_ATTRIB_TEX_COORD";
	case CELL_GCM_NV4097_SET_ATTRIB_TEX_COORD_EX	: return "SET_ATTRIB_TEX_COORD_EX";
	case CELL_GCM_NV4097_SET_ATTRIB_UCLIP0	: return "SET_ATTRIB_UCLIP0";
	case CELL_GCM_NV4097_SET_ATTRIB_UCLIP1	: return "SET_ATTRIB_UCLIP1";
	case CELL_GCM_NV4097_INVALIDATE_L2	: return "INVALIDATE_L2";
	case CELL_GCM_NV4097_SET_REDUCE_DST_COLOR	: return "SET_REDUCE_DST_COLOR";
	case CELL_GCM_NV4097_SET_SHADER_PACKER	: return "SET_SHADER_PACKER";
	case CELL_GCM_NV4097_SET_VERTEX_ATTRIB_INPUT_MASK	: return "SET_VERTEX_ATTRIB_INPUT_MASK";
	case CELL_GCM_NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK	: return "SET_VERTEX_ATTRIB_OUTPUT_MASK";
	case CELL_GCM_NV4097_SET_TRANSFORM_BRANCH_BITS	: return "SET_TRANSFORM_BRANCH_BITS";
	default: return "******unknown******";
	}
}

inline void DumpFifo(uint32_t *begin,uint32_t *stop) {
	while (begin < stop) {
		if (*begin & CELL_GCM_METHOD_FLAG_JUMP)
			grcDisplayf("JUMP to %x",*begin & ~CELL_GCM_METHOD_FLAG_JUMP);
		else if (*begin & CELL_GCM_METHOD_FLAG_CALL)
			grcDisplayf("CALL to %x",*begin & ~CELL_GCM_METHOD_FLAG_CALL);
		else if (*begin & CELL_GCM_METHOD_FLAG_RETURN)
			grcDisplayf("RETURN (%x)",*begin);
		else {
			grcDisplayf("WRITE %x bytes to %x (%s)",(*begin>>16)&(2047<<2),*begin&0xFFFF,RegisterName(*begin&0xFFFF));
			int count = (*begin >> 18) & 2047;
			for (int i=0; i<count; i++,begin++) {
				if ((i & 7) == 0)
					Printf("    ");
				Printf("%08x ",begin[1]);
				// 				if ((i & 7) == 7)
				// 					grcDisplayf("");
			}
			// 			if (count & 7)
			// 				grcDisplayf("");
		}
		++begin;
	}
}

} // namespace gcm
} // namespace rage
#endif // __PS3

#endif // GRCORE_WRAPPER_GCM_H
